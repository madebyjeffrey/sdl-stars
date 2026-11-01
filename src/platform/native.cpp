#include "common.h"
#include "ownership.h"

#include <stdarg.h>
#include <errno.h>
#include <time.h>

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
void qsort16(APPSESSION &app, void *base, size_t count, size_t width, int (*compare)(APPSESSION &, const void *, const void *)) {
    if (count < 2 || width == 0)
        return;

    unsigned char *items = (unsigned char *)base;
    size_t         i;
    for (i = 1; i < count; ++i) {
        if ((int16_t)compare(app, items + i * width, items + (i - 1) * width) < 0)
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
                    int order = (int16_t)compare(app, items + left * width, items + lo * width);
                    if (order > 0)
                        break;
                    if (order < 0)
                        lastLess = left;
                }
                for (;;) {
                    --right;
                    int order = (int16_t)compare(app, items + lo * width, items + right * width);
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
    NATIVEOWNER<char, std::free> format;

    // Dropping l only shortens the format, so a copy its size holds it.
    cbFmt = strlen(szFormat) + 1;
    szFmt = cbFmt <= sizeof(szFmtSmall) ? szFmtSmall : (char *)malloc(cbFmt);
    format.reset(szFmt == szFmtSmall ? nullptr : szFmt);
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
    format.reset();
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


void GetDateTimeSz(char *szDate, char *szTime) {
    _strdate(szDate);
    _strtime(szTime);
}

int16_t FSzPrefixNoCase(const char *sz1, const char *sz2, int16_t cch) { return _strnicmp(sz1, sz2, cch) == 0; }

#else

uint32_t DwTickCount() {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000);
}

void GetDateTimeSz(char *szDate, char *szTime) {
    time_t     t;
    struct tm *ptm;

    t = time(nullptr);
    ptm = localtime(&t);
    strftime(szDate, 9, "%m/%d/%y", ptm);
    strftime(szTime, 9, "%H:%M:%S", ptm);
}

int16_t FSzPrefixNoCase(const char *sz1, const char *sz2, int16_t cch) { return strncasecmp(sz1, sz2, cch) == 0; }

#endif
int64_t LNativeTime(void) { return (int64_t)time(nullptr); }

int CchSnprintf(char *psz, size_t cb, const char *pszFormat, ...) {
    va_list args;
    int cch;
    va_start(args, pszFormat);
    cch = vsnprintf(psz, cb, pszFormat, args);
    va_end(args);
    return cch;
}

// Legacy entrypoints retain their small handles and test-adapter symbols;
// resource ownership, binary streams and native exceptions live in fileio.cpp.
int16_t HfOpenFile(const char *szFile, uint16_t mdOpen, int16_t *pfMissing) {
    return HfNativeOpenLegacy(szFile, mdOpen, pfMissing);
}
int32_t CbReadFile(int16_t hf, void *rg, uint16_t cb) { return CbNativeReadLegacy(hf, rg, cb); }
int32_t CbWriteFile(int16_t hf, const void *rg, uint16_t cb) { return CbNativeWriteLegacy(hf, rg, cb); }
int32_t LSeekFile(int16_t hf, int32_t l, int16_t iOrigin) { return LNativeSeekLegacy(hf, l, iOrigin); }
int32_t CbFileSize(int16_t hf) { return CbNativeSizeLegacy(hf); }
void CloseFile(int16_t hf) { CloseNativeLegacy(hf); }
int16_t FFileExists(const char *szFile) { return IAccessNativePath(szFile, 0) == 0; }
int16_t FFileReadOnly(const char *szFile) { return IAccessNativePath(szFile, 2) != 0; }
void MakeDir(const char *szDir) { ICreateNativeDirectory(szDir, 0777); }
int CreateNativeDir(const char *pszDir) { return ICreateNativeDirectory(pszDir, 0777); }
int16_t FEnsureNativeDir(const char *pszDir) { return FEnsureNativeDirectory(pszDir); }
int16_t FNativeEnumerateDirectory(APPSESSION &app, const char *pszDir, FNativeDirectoryEntryProc pfnEntry, void *lpData) {
    NATIVEDIR *pdir;
    const char *pszEntry;
    if (!pfnEntry || !(pdir = PdirNativeOpen(pszDir))) return 0;
    StarsNativeDirGuard closeDirectory(pdir);
    for (;;) {
        errno = 0;
        pszEntry = PszNativeReadDir(pdir);
        if (!pszEntry) return errno == 0;
        if (!FUtf8Valid(pszEntry, strlen(pszEntry))) { errno = EILSEQ; return 0; }
        if (!pfnEntry(app, lpData, pszEntry)) return 1;
    }
}
int16_t FNativeFileSize(const char *pszFile, int64_t *pcbFile) {
    NATIVEFILEINFO info;
    if (!pcbFile || IGetNativeFileInfo(pszFile, &info, 1) != 0 || !info.fRegular) return 0;
    *pcbFile = info.cb;
    return 1;
}
int16_t FNativeDirectoryExists(const char *pszDir) {
    NATIVEFILEINFO info;
    return IGetNativeFileInfo(pszDir, &info, 1) == 0 && info.fDirectory;
}
int16_t FNativePathInfo(const char *psz, int16_t *pfDirectory, int16_t *pfLink) {
    NATIVEFILEINFO info;
    if (IGetNativeFileInfo(psz, &info, 0) != 0) return 0;
    *pfDirectory = info.fDirectory;
    *pfLink = info.fLink;
    return 1;
}
NATIVEFILE *PfOpenNativeFile(const char *psz, const char *pszMode) { return PfNativeOpen(psz, pszMode); }
