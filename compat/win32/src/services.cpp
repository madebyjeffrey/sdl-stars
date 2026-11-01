/* Narrow strings and arithmetic for UI callers; core native.c stays authoritative. */
#include <windows.h>
#include "encoding.h"
#include "fileio.h"
#include <SDL3/SDL.h>
#include <io.h>
#include <direct.h>
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <sys/stat.h>
#ifdef __APPLE__
#include <pthread.h>
#elif !defined(_WIN32)
#include <sys/syscall.h>
#endif
#include <ctime>
#include <unistd.h>

int win32::MulDiv(int nNumber, int nNumerator, int nDenominator) {
    std::int64_t lProduct = static_cast<std::int64_t>(nNumber) * nNumerator;
    std::int64_t lDenominator = nDenominator, lResult;
    BOOL fNegative;

    if (nDenominator == 0)
        return -1;
    fNegative = (lProduct < 0) != (lDenominator < 0);
    if (lProduct < 0) lProduct = -lProduct;
    if (lDenominator < 0) lDenominator = -lDenominator;
    lResult = (lProduct + lDenominator / 2) / lDenominator;
    if (fNegative) lResult = -lResult;
    // The SDK rejects an INT_MIN result as overflow as well as larger magnitudes.
    return lResult <= INT32_MIN || lResult > INT32_MAX ? -1 : static_cast<int>(lResult);
}

DWORD win32::GetCurrentThreadId(void) {
#ifdef _WIN32
    return static_cast<DWORD>(SDL_GetCurrentThreadID());
#elif defined(__APPLE__)
    std::uint64_t id;
    return pthread_threadid_np(nullptr, &id) == 0 ? static_cast<DWORD>(id) : 0;
#else
    return static_cast<DWORD>(syscall(SYS_gettid));
#endif
}

LPSTR win32::lstrcatA(LPSTR pszDest, LPCSTR pszSource) { return std::strcat(pszDest, pszSource); }
LPSTR win32::lstrcpyA(LPSTR pszDest, LPCSTR pszSource) {
    if (!pszDest || !pszSource) return nullptr;
    return std::strcpy(pszDest, pszSource);
}
int win32::lstrlenA(LPCSTR psz) { return psz ? static_cast<int>(std::strlen(psz)) : 0; }
int win32::lstrcmpiA(LPCSTR psz1, LPCSTR psz2) { return strcasecmp(psz1, psz2); }

int win32::wsprintfAFields(std::span<char> pszDest, LPCSTR pszFormat, std::span<const NATIVEFORMATARG> args) {
    char szFormat[1024];
    const char *psz = pszFormat, *pszStart;
    std::size_t cch = 0;
    int cchResult;
    char chLength, chType;
    const std::size_t cb = std::min(pszDest.size(), std::size_t{1024});

    if (pszDest.empty() || !pszFormat) return 0;
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
        while (std::isdigit(static_cast<unsigned char>(*psz))) psz++;
        if (*psz == '.') {
            psz++;
            while (std::isdigit(static_cast<unsigned char>(*psz))) psz++;
        }
        if (cch + static_cast<std::size_t>((psz - pszStart)) + 2 >= sizeof(szFormat)) goto LInvalid;
        std::memcpy(szFormat + cch, pszStart, static_cast<std::size_t>((psz - pszStart)));
        cch += static_cast<std::size_t>((psz - pszStart));
        chLength = 0;
        if (*psz == 'h' || *psz == 'l') chLength = *psz++;
        chType = *psz;
        if (!chType || !std::strchr("diuxXcs", chType)) goto LInvalid;
        // Win32 LONG is 32-bit even when this process uses Linux's LP64 ABI.
        if (chLength == 'l' && (chType == 'c' || chType == 's')) goto LInvalid;
        if (chLength == 'h' && std::strchr("diuxX", chType)) szFormat[cch++] = 'h';
        szFormat[cch++] = *psz++;
    }
    if (*psz) goto LInvalid;
    szFormat[cch] = 0;
    cchResult = CchFormatNative(pszDest.first(cb), szFormat, args);
    if (cchResult < 0) { pszDest[0] = 0; return 0; }
    if (static_cast<std::size_t>(cchResult) >= cb) {
        cchResult = static_cast<int>(CchUtf8Prefix(std::string_view(pszDest.data(), cb - 1), cb - 1));
        pszDest[cchResult] = 0;
    }
    return cchResult;
LInvalid:
    pszDest[0] = 0;
    win32::SetLastError(win32::ERROR_INVALID_PARAMETER);
    return 0;
}

#ifndef _WIN32
int _access(const char *pszPath, int mdAccess) { return IAccessNativePath(pszPath, mdAccess); }
int _mkdir(const char *pszPath) { return ICreateNativeDirectory(pszPath, 0777); }
char *_getcwd(char *pszPath, int cchPath) {
    if (cchPath <= 0) { errno = EINVAL; return nullptr; }
    return PszNativeCwd(pszPath, static_cast<std::size_t>(cchPath));
}
int _stricmp(const char *psz1, const char *psz2) { return strcasecmp(psz1, psz2); }
int _strnicmp(const char *psz1, const char *psz2, std::size_t cb) { return strncasecmp(psz1, psz2, cb); }
int strcmpi(const char *psz1, const char *psz2) { return _stricmp(psz1, psz2); }
int strnicmp(const char *psz1, const char *psz2, std::size_t cb) { return _strnicmp(psz1, psz2, cb); }
char *_strlwr(char *sz) {
    char *psz;
    for (psz = sz; *psz; psz++) if (*psz >= 'A' && *psz <= 'Z') *psz += 'a' - 'A';
    return sz;
}
char *strlwr(char *sz) { return _strlwr(sz); }

static char *SzTime(char *sz, const char *pszFormat) {
    std::time_t tmNow = std::time(nullptr);
    struct std::tm tmLocal;

    if (!sz) { errno = EINVAL; return nullptr; }
    if (!localtime_r(&tmNow, &tmLocal) || std::strftime(sz, 9, pszFormat, &tmLocal) == 0) {
        sz[0] = 0;
        return nullptr;
    }
    return sz;
}

char *_strdate(char *szDate) { return SzTime(szDate, "%m/%d/%y"); }
char *_strtime(char *szTime) { return SzTime(szTime, "%H:%M:%S"); }

char *itoa(int n, char *sz, int radix) {
    char rgch[33];
    unsigned int u = static_cast<unsigned int>(n);
    int cch = 0, i = 0;

    if (!sz || radix < 2 || radix > 36) { errno = EINVAL; return nullptr; }
    if (n < 0 && radix == 10) { sz[i++] = '-'; u = 0u - u; }
    do { rgch[cch++] = "0123456789abcdefghijklmnopqrstuvwxyz"[u % static_cast<unsigned>(radix)]; u /= static_cast<unsigned>(radix); } while (u);
    while (cch != 0) sz[i++] = rgch[--cch];
    sz[i] = 0;
    return sz;
}
#endif
