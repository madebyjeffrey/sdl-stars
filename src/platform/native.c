#ifdef _WIN32
#include <windows.h>
#endif

#include "common.h"

#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <errno.h>
#include <fcntl.h>
#include <share.h>
#include <shellapi.h>
#include <sys/stat.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

// qsortSwap16 exchanges complete elements, including native-width pointers.
static void qsortSwap16(unsigned char *a, unsigned char *b, size_t width) {
    while (width != 0) {
        --width;
        unsigned char value = a[width];
        a[width] = b[width];
        b[width] = value;
    }
}

// qsort16 preserves the ordering of equal keys in Stars' Win16 CRT qsort.
// Reconstructed from 0024:0866-09c6, matching qsort.asm in C700 and MSVC
// MLIBCEW.LIB. The initial sorted check, first-element pivot, and strict
// partition boundaries matter: ICompLong compares planet X coordinates only.
void qsort16(void *base, size_t count, size_t width, int (*compare)(const void *, const void *)) {
    if (count < 2 || width == 0)
        return;

    unsigned char *items = (unsigned char *)base;
    size_t         i;
    for (i = 1; i < count; ++i) {
        if ((int16_t)compare(items + i * width, items + (i - 1) * width) < 0)
            break;
    }
    if (i == count)
        return;

    // Process the smaller partition first, bounding pending ranges by log2(count).
    size_t lows[sizeof(size_t) * CHAR_BIT], highs[sizeof(size_t) * CHAR_BIT];
    size_t pending = 0, lo = 0, hi = count - 1;
    for (;;) {
        if (lo < hi) {
            size_t left = lo, right = hi + 1;
            size_t lastLess = lo, firstGreater = hi;
            for (;;) {
                while (++left != hi) {
                    int order = (int16_t)compare(items + left * width, items + lo * width);
                    if (order > 0)
                        break;
                    if (order < 0)
                        lastLess = left;
                }
                for (;;) {
                    --right;
                    int order = (int16_t)compare(items + lo * width, items + right * width);
                    if (order > 0)
                        break;
                    if (order < 0)
                        firstGreater = right;
                    else if (right == lo)
                        break;
                }
                if (right <= left)
                    break;
                qsortSwap16(items + left * width, items + right * width, width);
                firstGreater = right;
                lastLess = left;
            }
            qsortSwap16(items + lo * width, items + right * width, width);

            if (hi - firstGreater >= lastLess - lo) {
                lows[pending] = firstGreater;
                highs[pending++] = hi;
                hi = lastLess;
            } else {
                lows[pending] = lo;
                highs[pending++] = lastLess;
                lo = firstGreater;
            }
        } else {
            if (pending == 0)
                return;
            lo = lows[--pending];
            hi = highs[pending];
        }
    }
}

// CchSprintf drops the l from each %ld, %li, %lu, %lx or %lX in szFormat
// before formatting, since its arguments are 32-bit. Like wsprintf, it
// writes at most 1024 bytes, the terminator included, and returns the
// characters it wrote.
int CchSprintf(char *sz, const char *szFormat, ...) {
    char        szFmtSmall[256];
    char       *szFmt;
    size_t      cbFmt;
    const char *pchIn;
    char       *pchOut;
    int         cch;
    va_list     args;

    // Dropping l only shortens the format, so a copy its size holds it.
    cbFmt = strlen(szFormat) + 1;
    szFmt = cbFmt <= sizeof(szFmtSmall) ? szFmtSmall : malloc(cbFmt);
    if (!szFmt) {
        sz[0] = 0;
        return 0;
    }
    pchIn = szFormat;
    pchOut = szFmt;
    while (*pchIn != 0) {
        *pchOut++ = *pchIn;
        if (*pchIn++ != '%') {
            continue;
        }
        while (*pchIn != 0 && strchr("-+ #0123456789.", *pchIn)) {
            *pchOut++ = *pchIn++;
        }
        if (*pchIn == 'l' && pchIn[1] != 0 && strchr("diuxX", pchIn[1])) {
            pchIn++;
        }
        if (*pchIn != 0) {
            *pchOut++ = *pchIn++;
        }
    }
    *pchOut = 0;
    va_start(args, szFormat);
    cch = vsnprintf(sz, 1024, szFmt, args);
    va_end(args);
    if (szFmt != szFmtSmall) {
        free(szFmt);
    }
    if (cch < 0) {
        sz[0] = 0;
        return 0;
    }
    if (cch >= 1024) {
        cch = (int)CchUtf8Prefix(sz, 1023);
        sz[cch] = 0;
    }
    return cch;
}

// LMulDiv returns lNumber * lNumerator / lDenominator rounded half away from
// zero, or -1 for a zero denominator or a result out of range.
int32_t LMulDiv(int32_t lNumber, int32_t lNumerator, int32_t lDenominator) {
    int64_t l;

    if (lDenominator == 0) {
        return -1;
    }
    if (lDenominator < 0) {
        lNumber = -lNumber;
        lDenominator = -lDenominator;
    }
    if ((lNumber < 0) == (lNumerator < 0)) {
        l = ((int64_t)lNumber * lNumerator + lDenominator / 2) / lDenominator;
    } else {
        l = ((int64_t)lNumber * lNumerator - lDenominator / 2) / lDenominator;
    }
    if (l > 2147483647 || l < -2147483647) {
        return -1;
    }
    return (int32_t)l;
}

#ifdef _WIN32

char *PszNativeCommandLine(void) {
    const WCHAR *pwsz = GetCommandLineW();
    char *psz;
    int cb;
    if (*pwsz == L'"') {
        for (pwsz++; *pwsz && *pwsz != L'"'; pwsz++) {}
        if (*pwsz) pwsz++;
    } else {
        while (*pwsz && *pwsz != L' ' && *pwsz != L'\t') pwsz++;
    }
    while (*pwsz == L' ' || *pwsz == L'\t') pwsz++;
    cb = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pwsz, -1, NULL, 0, NULL, NULL);
    if (cb == 0 || !(psz = malloc(cb))) return NULL;
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pwsz, -1, psz, cb, NULL, NULL)) {
        free(psz); return NULL;
    }
    return psz;
}

char **RgpszNativeArguments(int *lpc) {
    WCHAR **rgpwsz = CommandLineToArgvW(GetCommandLineW(), lpc);
    char **rgpsz, *pch;
    size_t cb;
    int i, c;
    if (!rgpwsz) return NULL;
    cb = ((size_t)*lpc + 1) * sizeof(*rgpsz);
    for (i = 0; i < *lpc; i++) {
        c = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, rgpwsz[i], -1, NULL, 0, NULL, NULL);
        if (c == 0) { LocalFree(rgpwsz); return NULL; }
        cb += c;
    }
    rgpsz = malloc(cb);
    if (!rgpsz) { LocalFree(rgpwsz); return NULL; }
    pch = (char *)(rgpsz + *lpc + 1);
    for (i = 0; i < *lpc; i++) {
        c = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, rgpwsz[i], -1, NULL, 0, NULL, NULL);
        rgpsz[i] = pch;
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, rgpwsz[i], -1, pch, c, NULL, NULL);
        pch += c;
    }
    rgpsz[*lpc] = NULL; LocalFree(rgpwsz);
    return rgpsz;
}

// CRT descriptors keep the core's signed 16-bit handles independent of HANDLE.
static wchar_t *LpwszNativePath(const char *psz) {
    wchar_t *pwsz;
    int cch;
    cch = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, psz, -1, NULL, 0);
    if (cch == 0) { errno = EILSEQ; return NULL; }
    pwsz = malloc((size_t)cch * sizeof(*pwsz));
    if (!pwsz) { errno = ENOMEM; return NULL; }
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, psz, -1, pwsz, cch)) {
        free(pwsz); errno = EILSEQ; return NULL;
    }
    return pwsz;
}

int16_t HfOpenFile(char *szFile, uint16_t mdOpen, int16_t *pfMissing) {
    wchar_t *pwsz;
    int fd, grf, grfShare;
    *pfMissing = FALSE;
    pwsz = LpwszNativePath(szFile);
    if (!pwsz) return -1;
    grf = (mdOpen & 3) == 1 ? _O_WRONLY : (mdOpen & 3) == 2 ? _O_RDWR : _O_RDONLY;
    grf |= _O_BINARY | _O_NOINHERIT;
    if (mdOpen & 0x1000) grf |= _O_CREAT | _O_TRUNC;
    switch (mdOpen & 0x70) {
    case 0x10: grfShare = _SH_DENYRW; break;
    case 0x20: grfShare = _SH_DENYWR; break;
    case 0x30: grfShare = _SH_DENYRD; break;
    default: grfShare = _SH_DENYNO; break;
    }
    fd = _wsopen(pwsz, grf, grfShare, _S_IREAD | _S_IWRITE);
    free(pwsz);
    *pfMissing = fd == -1 && errno == ENOENT;
    if (fd > SHRT_MAX) { _close(fd); fd = -1; errno = EMFILE; }
    return (int16_t)fd;
}

int32_t CbReadFile(int16_t hf, void *rg, uint16_t cb) { return _read(hf, rg, cb); }

int32_t CbWriteFile(int16_t hf, void *rg, uint16_t cb) {
    int64_t ib;
    // Like _lwrite, a zero-byte write truncates or extends at the current offset.
    if (cb != 0) return _write(hf, rg, cb);
    ib = _telli64(hf);
    return ib < 0 || _chsize_s(hf, ib) != 0 ? -1 : 0;
}

int32_t LSeekFile(int16_t hf, int32_t l, int16_t iOrigin) {
    int64_t ib = _lseeki64(hf, l, iOrigin == 0 ? SEEK_SET : iOrigin == 1 ? SEEK_CUR : SEEK_END);
    return ib < 0 || ib > INT32_MAX ? -1 : (int32_t)ib;
}

int32_t CbFileSize(int16_t hf) {
    int64_t cb = _filelengthi64(hf);
    return cb < 0 || cb > INT32_MAX ? -1 : (int32_t)cb;
}

void CloseFile(int16_t hf) { _close(hf); }

int16_t FFileExists(char *szFile) {
    wchar_t *pwsz = LpwszNativePath(szFile);
    int f;
    if (!pwsz) return FALSE;
    f = _waccess(pwsz, 0) == 0; free(pwsz); return f;
}

int16_t FFileReadOnly(char *szFile) {
    wchar_t *pwsz = LpwszNativePath(szFile);
    int f;
    if (!pwsz) return TRUE;
    f = _waccess(pwsz, 2) != 0; free(pwsz); return f;
}

void MakeDir(char *szDir) {
    wchar_t *pwsz = LpwszNativePath(szDir);
    if (pwsz) { _wmkdir(pwsz); free(pwsz); }
}

int CreateNativeDir(const char *pszDir) {
    wchar_t *pwsz = LpwszNativePath(pszDir);
    int result;
    if (!pwsz) return -1;
    result = _wmkdir(pwsz);
    free(pwsz); return result;
}

int16_t FNativeEnumerateDirectory(const char *pszDir, FNativeDirectoryEntryProc pfnEntry, void *lpData) {
    wchar_t *pwsz = LpwszNativePath(pszDir), *pwszSearch;
    WIN32_FIND_DATAW findData;
    HANDLE hFind;
    size_t cch;
    char szEntry[1024];
    int16_t fResult;

    if (!pwsz || !pfnEntry) { free(pwsz); return FALSE; }
    cch = wcslen(pwsz);
    pwszSearch = malloc((cch + 3) * sizeof(*pwszSearch));
    if (!pwszSearch) { free(pwsz); return FALSE; }
    memcpy(pwszSearch, pwsz, cch * sizeof(*pwszSearch));
    if (cch != 0 && pwszSearch[cch - 1] != L'/' && pwszSearch[cch - 1] != L'\\') pwszSearch[cch++] = L'\\';
    pwszSearch[cch++] = L'*'; pwszSearch[cch] = 0;
    hFind = FindFirstFileW(pwszSearch, &findData);
    free(pwsz); free(pwszSearch);
    if (hFind == INVALID_HANDLE_VALUE) return GetLastError() == ERROR_FILE_NOT_FOUND;
    fResult = TRUE;
    for (;;) {
        if (wcscmp(findData.cFileName, L".") && wcscmp(findData.cFileName, L"..")) {
            if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, findData.cFileName, -1,
                                    szEntry, sizeof(szEntry), NULL, NULL) == 0) { fResult = FALSE; break; }
            if (!pfnEntry(lpData, szEntry)) break;
        }
        if (!FindNextFileW(hFind, &findData)) {
            if (GetLastError() != ERROR_NO_MORE_FILES) fResult = FALSE;
            break;
        }
    }
    FindClose(hFind);
    return fResult;
}

int16_t FNativeFileSize(const char *pszFile, int64_t *pcbFile) {
    wchar_t *pwsz = LpwszNativePath(pszFile);
    struct _stat64 st;
    int16_t fResult;
    if (!pwsz) return FALSE;
    fResult = _wstat64(pwsz, &st) == 0 && (st.st_mode & _S_IFMT) == _S_IFREG;
    free(pwsz);
    if (fResult) *pcbFile = st.st_size;
    return fResult;
}

int16_t FNativeDirectoryExists(const char *pszDir) {
    wchar_t *pwsz = LpwszNativePath(pszDir);
    DWORD dwAttributes;
    if (!pwsz) return FALSE;
    dwAttributes = GetFileAttributesW(pwsz);
    free(pwsz);
    return dwAttributes != INVALID_FILE_ATTRIBUTES && (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static int16_t FCreateDirectoryW(wchar_t *pwszDir) {
    DWORD dwAttributes;
    if (CreateDirectoryW(pwszDir, NULL)) return TRUE;
    if (GetLastError() != ERROR_ALREADY_EXISTS) return FALSE;
    dwAttributes = GetFileAttributesW(pwszDir);
    return dwAttributes != INVALID_FILE_ATTRIBUTES && (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

int16_t FEnsureNativeDir(const char *pszDir) {
    wchar_t *pwsz = LpwszNativePath(pszDir), *pch, *pStart;
    wchar_t ch;
    if (!pwsz) return FALSE;
    pStart = pwsz;
    if (pwsz[0] == L'\\' && pwsz[1] == L'\\') {
        pch = wcschr(pwsz + 2, L'\\');
        if (pch) pch = wcschr(pch + 1, L'\\');
        pStart = pch ? pch + 1 : pwsz + wcslen(pwsz);
    } else if (pwsz[0] && pwsz[1] == L':' && (pwsz[2] == L'\\' || pwsz[2] == L'/')) {
        pStart = pwsz + 3;
    } else if (pwsz[0] == L'\\' || pwsz[0] == L'/') {
        pStart = pwsz + 1;
    }
    for (pch = pStart; *pch; pch++) {
        if (*pch != L'\\' && *pch != L'/') continue;
        ch = *pch; *pch = 0;
        if (*pwsz && !FCreateDirectoryW(pwsz)) { *pch = ch; free(pwsz); return FALSE; }
        *pch = ch;
    }
    if (!FCreateDirectoryW(pwsz)) { free(pwsz); return FALSE; }
    free(pwsz);
    return TRUE;
}


int STARS_TEST_IMPL(RemoveNativeFile)(const char *psz) {
    wchar_t *pwsz = LpwszNativePath(psz);
    int result;
    if (!pwsz) return -1;
    result = _wremove(pwsz); free(pwsz); return result;
}

int RemoveNativePath(const char *psz) {
    wchar_t *pwsz = LpwszNativePath(psz);
    int result;
    if (!pwsz) return -1;
    result = DeleteFileW(pwsz) ? 0 : RemoveDirectoryW(pwsz) ? 0 : -1;
    free(pwsz); return result;
}

// Metadata replacement must preserve the old file if installing the new one fails.
int ReplaceNativeFile(const char *pszOld, const char *pszNew) {
    wchar_t *pwszOld = LpwszNativePath(pszOld), *pwszNew;
    int result;
    if (!pwszOld) return -1;
    pwszNew = LpwszNativePath(pszNew);
    if (!pwszNew) { free(pwszOld); return -1; }
    result = MoveFileExW(pwszOld, pwszNew, MOVEFILE_REPLACE_EXISTING) ? 0 : -1;
    free(pwszOld); free(pwszNew); return result;
}

int RenameNativeFile(const char *pszOld, const char *pszNew) {
    wchar_t *pwszOld = LpwszNativePath(pszOld), *pwszNew;
    int result;
    if (!pwszOld) return -1;
    pwszNew = LpwszNativePath(pszNew);
    if (!pwszNew) { free(pwszOld); return -1; }
    result = _wrename(pwszOld, pwszNew);
    free(pwszOld); free(pwszNew); return result;
}

FILE *PfOpenNativeFile(const char *psz, const char *pszMode) {
    wchar_t *pwsz = LpwszNativePath(psz), wszMode[16];
    FILE *pf;
    size_t i, cch = strlen(pszMode);
    if (!pwsz) return NULL;
    if (cch >= sizeof(wszMode) / sizeof(*wszMode)) { free(pwsz); errno = EINVAL; return NULL; }
    for (i = 0; i <= cch; i++) wszMode[i] = (unsigned char)pszMode[i];
    pf = _wfopen(pwsz, wszMode); free(pwsz); return pf;
}

uint32_t DwTickCount() { return GetTickCount(); }

void GetDateTimeSz(char *szDate, char *szTime) {
    _strdate(szDate);
    _strtime(szTime);
}

int16_t FSzPrefixNoCase(char *sz1, char *sz2, int16_t cch) { return _strnicmp(sz1, sz2, cch) == 0; }

#else

static int FNativePath(const char *psz) {
    if (FUtf8Valid(psz, strlen(psz))) return TRUE;
    errno = EILSEQ;
    return FALSE;
}

int16_t HfOpenFile(char *szFile, uint16_t mdOpen, int16_t *pfMissing) {
    int fd;
    int grf;

    *pfMissing = FALSE;
    if (!FNativePath(szFile)) return -1;
    // OF_READ, OF_WRITE, OF_READWRITE; OF_CREATE truncates. Share modes
    // (0x0070) have no POSIX counterpart.
    switch (mdOpen & 3) {
    case 1:
        grf = O_WRONLY;
        break;
    case 2:
        grf = O_RDWR;
        break;
    default:
        grf = O_RDONLY;
        break;
    }
    if (mdOpen & 0x1000) {
        grf |= O_CREAT | O_TRUNC;
    }
    fd = open(szFile, grf, 0666);
    *pfMissing = fd == -1 && errno == ENOENT;
    if (fd > SHRT_MAX) {
        close(fd);
        fd = -1;
    }
    return (int16_t)fd;
}

int32_t CbReadFile(int16_t hf, void *rg, uint16_t cb) { return (int32_t)read(hf, rg, cb); }

int32_t CbWriteFile(int16_t hf, void *rg, uint16_t cb) { return (int32_t)write(hf, rg, cb); }

int32_t LSeekFile(int16_t hf, int32_t l, int16_t iOrigin) { return (int32_t)lseek(hf, l, iOrigin == 0 ? SEEK_SET : iOrigin == 1 ? SEEK_CUR : SEEK_END); }

int32_t CbFileSize(int16_t hf) {
    struct stat st;

    if (fstat(hf, &st) != 0) {
        return -1;
    }
    return (int32_t)st.st_size;
}

void CloseFile(int16_t hf) { close(hf); }

int16_t FFileExists(char *szFile) { return FNativePath(szFile) && access(szFile, F_OK) == 0; }

int16_t FFileReadOnly(char *szFile) { return !FNativePath(szFile) || access(szFile, W_OK) != 0; }

void MakeDir(char *szDir) { if (FNativePath(szDir)) mkdir(szDir, 0777); }

int CreateNativeDir(const char *pszDir) {
    return FNativePath(pszDir) ? mkdir(pszDir, 0777) : -1;
}

int16_t FNativeEnumerateDirectory(const char *pszDir, FNativeDirectoryEntryProc pfnEntry, void *lpData) {
    DIR *pdir;
    struct dirent *pentry;
    int16_t fResult;
    if (!pfnEntry || !FNativePath(pszDir) || !(pdir = opendir(pszDir))) return FALSE;
    fResult = TRUE;
    for (;;) {
        errno = 0;
        pentry = readdir(pdir);
        if (!pentry) {
            if (errno != 0) fResult = FALSE;
            break;
        }
        if (!strcmp(pentry->d_name, ".") || !strcmp(pentry->d_name, "..")) continue;
        if (!FUtf8Valid(pentry->d_name, strlen(pentry->d_name))) {
            errno = EILSEQ;
            fResult = FALSE;
            break;
        }
        if (!pfnEntry(lpData, pentry->d_name)) break;
    }
    closedir(pdir);
    return fResult;
}

int16_t FNativeFileSize(const char *pszFile, int64_t *pcbFile) {
    struct stat st;
    if (!pcbFile || !FNativePath(pszFile) || stat(pszFile, &st) != 0 || !S_ISREG(st.st_mode)) return FALSE;
    *pcbFile = st.st_size;
    return TRUE;
}

int16_t FNativeDirectoryExists(const char *pszDir) {
    struct stat st;
    return FNativePath(pszDir) && stat(pszDir, &st) == 0 && S_ISDIR(st.st_mode);
}

int16_t FEnsureNativeDir(const char *pszDir) {
    char *pszPath, *pch;
    char ch;
    if (!FNativePath(pszDir) || !(pszPath = strdup(pszDir))) return FALSE;
    for (pch = pszPath[0] == '/' ? pszPath + 1 : pszPath; *pch; pch++) {
        if (*pch != '/') continue;
        ch = *pch; *pch = 0;
        if (*pszPath && mkdir(pszPath, 0777) != 0 && !FNativeDirectoryExists(pszPath)) {
            free(pszPath); return FALSE;
        }
        *pch = ch;
    }
    if (mkdir(pszPath, 0777) != 0 && !FNativeDirectoryExists(pszPath)) { free(pszPath); return FALSE; }
    free(pszPath);
    return TRUE;
}

int STARS_TEST_IMPL(RemoveNativeFile)(const char *psz) { return FNativePath(psz) ? remove(psz) : -1; }
int RemoveNativePath(const char *psz) { return FNativePath(psz) ? remove(psz) : -1; }
int ReplaceNativeFile(const char *pszOld, const char *pszNew) {
    return FNativePath(pszOld) && FNativePath(pszNew) ? rename(pszOld, pszNew) : -1;
}
int RenameNativeFile(const char *pszOld, const char *pszNew) {
    return FNativePath(pszOld) && FNativePath(pszNew) ? rename(pszOld, pszNew) : -1;
}
FILE *PfOpenNativeFile(const char *psz, const char *pszMode) { return FNativePath(psz) ? fopen(psz, pszMode) : NULL; }

uint32_t DwTickCount() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000);
}

void GetDateTimeSz(char *szDate, char *szTime) {
    time_t     t;
    struct tm *ptm;

    t = time(NULL);
    ptm = localtime(&t);
    strftime(szDate, 9, "%m/%d/%y", ptm);
    strftime(szTime, 9, "%H:%M:%S", ptm);
}

int16_t FSzPrefixNoCase(char *sz1, char *sz2, int16_t cch) { return strncasecmp(sz1, sz2, cch) == 0; }

#endif
int64_t LNativeTime(void) { return (int64_t)time(NULL); }

int CchSnprintf(char *psz, size_t cb, const char *pszFormat, ...) {
    va_list args;
    int cch;
    va_start(args, pszFormat);
    cch = vsnprintf(psz, cb, pszFormat, args);
    va_end(args);
    return cch;
}
