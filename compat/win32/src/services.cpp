/* Narrow strings and arithmetic for UI callers; core native.c stays authoritative. */
#include <windows.h>
#include "encoding.h"
#include "fileio.h"
#include <SDL3/SDL.h>
#include <io.h>
#include <direct.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#ifdef __APPLE__
#include <pthread.h>
#elif !defined(_WIN32)
#include <sys/syscall.h>
#endif
#include <time.h>
#include <unistd.h>

int WINAPI MulDiv(int nNumber, int nNumerator, int nDenominator) {
    int64_t lProduct = (int64_t)nNumber * nNumerator;
    int64_t lDenominator = nDenominator, lResult;
    BOOL fNegative;

    if (nDenominator == 0)
        return -1;
    fNegative = (lProduct < 0) != (lDenominator < 0);
    if (lProduct < 0) lProduct = -lProduct;
    if (lDenominator < 0) lDenominator = -lDenominator;
    lResult = (lProduct + lDenominator / 2) / lDenominator;
    if (fNegative) lResult = -lResult;
    // The SDK rejects an INT_MIN result as overflow as well as larger magnitudes.
    return lResult <= INT32_MIN || lResult > INT32_MAX ? -1 : (int)lResult;
}

DWORD WINAPI GetCurrentThreadId(void) {
#ifdef _WIN32
    return (DWORD)SDL_GetCurrentThreadID();
#elif defined(__APPLE__)
    uint64_t id;
    return pthread_threadid_np(NULL, &id) == 0 ? (DWORD)id : 0;
#else
    return (DWORD)syscall(SYS_gettid);
#endif
}

LPSTR WINAPI lstrcatA(LPSTR pszDest, LPCSTR pszSource) { return strcat(pszDest, pszSource); }
LPSTR WINAPI lstrcpyA(LPSTR pszDest, LPCSTR pszSource) {
    if (!pszDest || !pszSource) return NULL;
    return strcpy(pszDest, pszSource);
}
int WINAPI lstrlenA(LPCSTR psz) { return psz ? (int)strlen(psz) : 0; }
int WINAPI lstrcmpiA(LPCSTR psz1, LPCSTR psz2) { return strcasecmp(psz1, psz2); }

int WINAPIV wsprintfA(LPSTR pszDest, LPCSTR pszFormat, ...) {
    char szFormat[1024];
    const char *psz = pszFormat, *pszStart;
    size_t cch = 0;
    int cchResult;
    char chLength, chType;
    va_list args;

    if (!pszDest || !pszFormat) return 0;
    while (*psz && cch < sizeof(szFormat) - 1) {
        if (*psz != '%') { szFormat[cch++] = *psz++; continue; }
        szFormat[cch++] = *psz++;
        if (*psz == '%') {
            if (cch >= sizeof(szFormat) - 1) goto LInvalid;
            szFormat[cch++] = *psz++;
            continue;
        }
        pszStart = psz;
        while (*psz == '-' || *psz == '#' || *psz == '0') psz++;
        while (isdigit((unsigned char)*psz)) psz++;
        if (*psz == '.') {
            psz++;
            while (isdigit((unsigned char)*psz)) psz++;
        }
        if (cch + (size_t)(psz - pszStart) + 2 >= sizeof(szFormat)) goto LInvalid;
        memcpy(szFormat + cch, pszStart, (size_t)(psz - pszStart));
        cch += (size_t)(psz - pszStart);
        chLength = 0;
        if (*psz == 'h' || *psz == 'l') chLength = *psz++;
        chType = *psz;
        if (!chType || !strchr("diuxXcs", chType)) goto LInvalid;
        // Win32 LONG is 32-bit even when this process uses Linux's LP64 ABI.
        if (chLength == 'l' && (chType == 'c' || chType == 's')) goto LInvalid;
        if (chLength == 'h' && strchr("diuxX", chType)) szFormat[cch++] = 'h';
        szFormat[cch++] = *psz++;
    }
    if (*psz) goto LInvalid;
    szFormat[cch] = 0;
    va_start(args, pszFormat);
    cchResult = vsnprintf(pszDest, 1024, szFormat, args);
    va_end(args);
    if (cchResult < 0) { pszDest[0] = 0; return 0; }
    if (cchResult > 1023) {
        cchResult = (int)CchUtf8Prefix(pszDest, 1023);
        pszDest[cchResult] = 0;
    }
    return cchResult;
LInvalid:
    pszDest[0] = 0;
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
}

#ifndef _WIN32
int _access(const char *pszPath, int mdAccess) { return IAccessNativePath(pszPath, mdAccess); }
int _mkdir(const char *pszPath) { return ICreateNativeDirectory(pszPath, 0777); }
char *_getcwd(char *pszPath, int cchPath) {
    if (cchPath <= 0) { errno = EINVAL; return NULL; }
    return PszNativeCwd(pszPath, (size_t)cchPath);
}
int _stricmp(const char *psz1, const char *psz2) { return strcasecmp(psz1, psz2); }
int _strnicmp(const char *psz1, const char *psz2, size_t cb) { return strncasecmp(psz1, psz2, cb); }
int strcmpi(const char *psz1, const char *psz2) { return _stricmp(psz1, psz2); }
int strnicmp(const char *psz1, const char *psz2, size_t cb) { return _strnicmp(psz1, psz2, cb); }
char *_strlwr(char *sz) {
    char *psz;
    for (psz = sz; *psz; psz++) if (*psz >= 'A' && *psz <= 'Z') *psz += 'a' - 'A';
    return sz;
}
char *strlwr(char *sz) { return _strlwr(sz); }

static char *SzTime(char *sz, const char *pszFormat) {
    time_t tmNow = time(NULL);
    struct tm tmLocal;

    if (!sz) { errno = EINVAL; return NULL; }
    if (!localtime_r(&tmNow, &tmLocal) || strftime(sz, 9, pszFormat, &tmLocal) == 0) {
        sz[0] = 0;
        return NULL;
    }
    return sz;
}

char *_strdate(char *szDate) { return SzTime(szDate, "%m/%d/%y"); }
char *_strtime(char *szTime) { return SzTime(szTime, "%H:%M:%S"); }

char *itoa(int n, char *sz, int radix) {
    char rgch[33];
    unsigned int u = (unsigned int)n;
    int cch = 0, i = 0;

    if (!sz || radix < 2 || radix > 36) { errno = EINVAL; return NULL; }
    if (n < 0 && radix == 10) { sz[i++] = '-'; u = 0u - u; }
    do { rgch[cch++] = "0123456789abcdefghijklmnopqrstuvwxyz"[u % (unsigned)radix]; u /= (unsigned)radix; } while (u);
    while (cch != 0) sz[i++] = rgch[--cch];
    sz[i] = 0;
    return sz;
}
#endif
