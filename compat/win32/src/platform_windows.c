/* NATIVE: Keep the actual Windows SDK isolated from the SDL wrapper's source API. */
#include <windows.h>
#include <bcrypt.h>
#include <direct.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdint.h>

struct COMPATDIRENT { char d_name[PATH_MAX]; };
typedef struct COMPATDIR {
    _WDIR *lpdir;
    struct COMPATDIRENT entry;
} COMPATDIR;

static wchar_t *LpwszPath(const char *psz) {
    wchar_t *pwsz;
    int cch;

    cch = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, psz, -1, NULL, 0);
    if (cch == 0) { errno = EINVAL; return NULL; }
    pwsz = malloc((size_t)cch * sizeof(*pwsz));
    if (!pwsz) return NULL;
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, psz, -1, pwsz, cch);
    return pwsz;
}

char *PszCompatRealPath(const char *pszPath, char *pszOut) {
    wchar_t *pwsz = LpwszPath(pszPath), *pwszFull;
    char *psz, *pch;
    DWORD cch;
    int cb;

    if (!pwsz) return NULL;
    cch = GetFullPathNameW(pwsz, 0, NULL, NULL);
    pwszFull = cch != 0 ? malloc((size_t)cch * sizeof(*pwszFull)) : NULL;
    if (!pwszFull) { free(pwsz); return NULL; }
    if (GetFullPathNameW(pwsz, cch, pwszFull, NULL) == 0 ||
        GetFileAttributesW(pwszFull) == INVALID_FILE_ATTRIBUTES) {
        free(pwsz); free(pwszFull); errno = ENOENT; return NULL;
    }
    free(pwsz);
    cb = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pwszFull, -1, NULL, 0, NULL, NULL);
    if (cb == 0 || cb > PATH_MAX) { free(pwszFull); errno = ENAMETOOLONG; return NULL; }
    psz = pszOut ? pszOut : malloc((size_t)cb);
    if (psz) {
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pwszFull, -1, psz, cb, NULL, NULL);
        for (pch = psz; *pch; pch++) if (*pch == '\\') *pch = '/';
    }
    free(pwszFull);
    return psz;
}

int ICompatMakeDir(const char *pszPath, int md) {
    wchar_t *pwsz = LpwszPath(pszPath);
    int ret;

    (void)md;
    if (!pwsz) return -1;
    ret = _wmkdir(pwsz); free(pwsz);
    return ret;
}

int ICompatRename(const char *pszOld, const char *pszNew) {
    wchar_t *pwszOld = LpwszPath(pszOld), *pwszNew = LpwszPath(pszNew);
    int ret;

    if (!pwszOld || !pwszNew) { free(pwszOld); free(pwszNew); return -1; }
    // POSIX rename replaces the destination; the Windows CRT's rename does not.
    ret = MoveFileExW(pwszOld, pwszNew, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) ? 0 : -1;
    if (ret != 0) errno = EACCES;
    free(pwszOld); free(pwszNew);
    return ret;
}

int ICompatTempFile(char *pszTemplate) {
    static const char rgch[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    size_t cch = strlen(pszTemplate);
    unsigned char rgb[6];
    wchar_t *pwsz;
    int fd, err, i, iTry;

    if (cch < 6 || strcmp(pszTemplate + cch - 6, "XXXXXX") != 0) { errno = EINVAL; return -1; }
    for (iTry = 0; iTry < 128; iTry++) {
        if (BCryptGenRandom(NULL, rgb, sizeof(rgb), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) { errno = EIO; return -1; }
        for (i = 0; i < 6; i++) pszTemplate[cch - 6 + i] = rgch[rgb[i] % 62];
        pwsz = LpwszPath(pszTemplate);
        if (!pwsz) return -1;
        err = _wsopen_s(&fd, pwsz, _O_CREAT | _O_EXCL | _O_RDWR | _O_NOINHERIT | _O_BINARY,
                       _SH_DENYRW, _S_IREAD | _S_IWRITE);
        free(pwsz);
        if (err == 0) return fd;
        errno = err;
        if (err != EEXIST) return -1;
    }
    errno = EEXIST;
    return -1;
}

ssize_t CbCompatGetLine(char **lpsz, size_t *lpcb, FILE *lpfile) {
    char *psz;
    size_t cb = 0, cbNew;
    int ch;

    while ((ch = fgetc(lpfile)) != EOF) {
        if (cb + 1 >= *lpcb) {
            cbNew = *lpcb != 0 ? *lpcb * 2 : 128;
            if (cbNew <= *lpcb) { errno = ENOMEM; return -1; }
            psz = realloc(*lpsz, cbNew);
            if (!psz) return -1;
            *lpsz = psz; *lpcb = cbNew;
        }
        (*lpsz)[cb++] = (char)ch;
        if (ch == '\n') break;
    }
    if (cb == 0) return -1;
    (*lpsz)[cb] = 0;
    return (ssize_t)cb;
}

FILE *LpCompatOpenFile(const char *pszPath, const char *pszMode) {
    wchar_t *pwszPath = LpwszPath(pszPath), *pwszMode = LpwszPath(pszMode);
    FILE *lpfile = NULL;

    if (pwszPath && pwszMode) lpfile = _wfopen(pwszPath, pwszMode);
    free(pwszPath); free(pwszMode);
    return lpfile;
}

int ICompatUnlink(const char *pszPath) {
    wchar_t *pwsz = LpwszPath(pszPath);
    int ret;

    if (!pwsz) return -1;
    ret = _wunlink(pwsz); free(pwsz);
    return ret;
}

int ICompatAccess(const char *pszPath, int md) {
    wchar_t *pwsz = LpwszPath(pszPath);
    int ret;

    if (!pwsz) return -1;
    ret = _waccess(pwsz, md); free(pwsz);
    return ret;
}

int ICompatOpen(const char *pszPath, int flags, ...) {
    wchar_t *pwsz = LpwszPath(pszPath);
    int fd, md = 0;
    va_list args;

    if (!pwsz) return -1;
    if (flags & _O_CREAT) {
        va_start(args, flags); md = va_arg(args, int); va_end(args);
    }
    fd = _wopen(pwsz, flags | _O_BINARY, md); free(pwsz);
    return fd;
}

int ICompatStat(const char *pszPath, struct stat *lpst) {
    wchar_t *pwsz = LpwszPath(pszPath);
    int ret;

    if (!pwsz) return -1;
    ret = wstat(pwsz, lpst); free(pwsz);
    return ret;
}

char *PszCompatCwd(char *pszOut, size_t cb) {
    wchar_t *pwsz = _wgetcwd(NULL, 0);
    char *pch;
    int cch;

    if (!pwsz) return NULL;
    cch = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pwsz, -1, NULL, 0, NULL, NULL);
    if (cch == 0 || (size_t)cch > cb) { free(pwsz); errno = ERANGE; return NULL; }
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pwsz, -1, pszOut, cch, NULL, NULL);
    free(pwsz);
    for (pch = pszOut; *pch; pch++) if (*pch == '\\') *pch = '/';
    return pszOut;
}

int ICompatChdir(const char *pszPath) {
    wchar_t *pwsz = LpwszPath(pszPath);
    int ret;

    if (!pwsz) return -1;
    ret = _wchdir(pwsz); free(pwsz);
    return ret;
}

COMPATDIR *LpCompatOpenDir(const char *pszPath) {
    wchar_t *pwsz = LpwszPath(pszPath);
    COMPATDIR *lp = calloc(1, sizeof(*lp));

    if (lp && pwsz) lp->lpdir = _wopendir(pwsz);
    free(pwsz);
    if (!lp || !lp->lpdir) { free(lp); return NULL; }
    return lp;
}

struct COMPATDIRENT *LpCompatReadDir(COMPATDIR *lpdir) {
    struct _wdirent *lpentry = _wreaddir(lpdir->lpdir);
    int cb;

    if (!lpentry) return NULL;
    cb = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, lpentry->d_name, -1,
                            lpdir->entry.d_name, sizeof(lpdir->entry.d_name), NULL, NULL);
    if (cb == 0) { errno = EILSEQ; return NULL; }
    return &lpdir->entry;
}

int ICompatCloseDir(COMPATDIR *lpdir) {
    int ret = _wclosedir(lpdir->lpdir);

    free(lpdir);
    return ret;
}

int FCompatFileIdentity(int fd, const char *pszPath, uint64_t *lpdev, uint64_t *lpino) {
    HANDLE h;
    BY_HANDLE_FILE_INFORMATION info;
    wchar_t *pwsz;
    int fOk;

    h = fd >= 0 ? (HANDLE)_get_osfhandle(fd) : INVALID_HANDLE_VALUE;
    if (fd < 0) {
        pwsz = LpwszPath(pszPath);
        if (!pwsz) return 0;
        h = CreateFileW(pwsz, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        free(pwsz);
    }
    fOk = h != INVALID_HANDLE_VALUE && GetFileInformationByHandle(h, &info);
    if (fd < 0 && h != INVALID_HANDLE_VALUE) CloseHandle(h);
    if (fOk) {
        *lpdev = info.dwVolumeSerialNumber;
        *lpino = ((uint64_t)info.nFileIndexHigh << 32) | info.nFileIndexLow;
    }
    return fOk;
}
