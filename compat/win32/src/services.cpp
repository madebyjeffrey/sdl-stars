/* Narrow strings and arithmetic for UI callers; core native.c stays authoritative. */
#include <limits>
#include <windows.h>
#include "encoding.h"
#include "fileio.h"
#include <SDL3/SDL.h>
#include <io.h>
#include <cerrno>
#include <cstdio>
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
    return lResult <= std::numeric_limits<std::int32_t>::min() || lResult > std::numeric_limits<std::int32_t>::max() ? -1 : static_cast<int>(lResult);
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

int win32::lstrcmpiA(LPCSTR psz1, LPCSTR psz2) { return strcasecmp(psz1, psz2); }

#ifndef _WIN32
int _access(const char *pszPath, int mdAccess) { return IAccessNativePath(pszPath, mdAccess); }
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

#endif
