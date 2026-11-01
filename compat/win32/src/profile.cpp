#include "locking.h"
#include <algorithm>
#include "ownership.h"
/* File-backed UTF-8 profiles. Preserve unrelated lines when updating settings. */
#include <windows.h>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include "platform.h"
#include "encoding.h"

#include "profile_io.h"
#include "profile_text_io.h"

static NATIVEFLAGLOCK fProfileLock;

static void FreeProfile(COMPATPROFILE *lp) {
    COMPATPROFILE *lpNext;
    while (lp) {
        lpNext = lp->lpNext;
        std::free(lp->szRaw); std::free(lp->szParsed); std::free(lp);
        lp = lpNext;
    }
}

static char *PszTrimProfile(char *psz) {
    char *pszEnd;
    while (*psz && std::isspace(static_cast<unsigned char>(*psz))) psz++;
    pszEnd = psz + std::strlen(psz);
    while (pszEnd > psz && std::isspace(static_cast<unsigned char>(pszEnd[-1]))) *--pszEnd = 0;
    return psz;
}

static COMPATPROFILE *LpProfileLine(const char *pszRaw) {
    COMPATPROFILE *lp = static_cast<COMPATPROFILE *>(std::calloc(1, sizeof(*lp)));
    char *psz, *pszEnd;

    NATIVEOWNER<COMPATPROFILE, FreeProfile> profileOwner(lp);
    if (!lp) return nullptr;
    lp->szRaw = strdup(pszRaw); lp->szParsed = strdup(pszRaw);
    if (!lp->szRaw || !lp->szParsed) return nullptr;
    psz = PszTrimProfile(lp->szParsed);
    if (*psz == '[' && (pszEnd = std::strchr(psz + 1, ']'))) {
        *pszEnd = 0;
        lp->pszSection = PszTrimProfile(psz + 1);
    } else if (*psz && *psz != ';' && (pszEnd = std::strchr(psz, '='))) {
        *pszEnd = 0;
        lp->pszKey = PszTrimProfile(psz);
        lp->pszValue = PszTrimProfile(pszEnd + 1);
    }
    return profileOwner.release();
}

static BOOL FLoadProfile(const char *pszPath, COMPATPROFILE **lplp) {
    const PROFILEIO &io = ProfileIO();
    NATIVEFILE *fp = nullptr;
    char *sz = nullptr;
    std::size_t cb = 0, cbTotal = 0;
    ssize_t cbLine;
    COMPATPROFILE **lplpNext = lplp;
    BOOL fRet = 0;
    NATIVECLEANUPSCOPE releaseProfile([&]() noexcept {
        std::free(sz);
        if (fp) io.pfnClose(fp);
        if (!fRet) { FreeProfile(*lplp); *lplp = nullptr; }
    });

    *lplp = nullptr;
    fp = io.pfnOpen(pszPath, "rb");
    if (!fp) {
        win32::SetLastError(DwProfileError());
        return errno == ENOENT;
    }
    while ((cbLine = io.pfnReadLine(&sz, &cb, fp)) >= 0) {
        cbTotal += static_cast<std::size_t>(cbLine);
        if (cbTotal > 8 * 1024 * 1024 || !FUtf8Valid(sz, static_cast<std::size_t>(cbLine))) {
            win32::SetLastError(win32::ERROR_BAD_FORMAT); goto LReturn;
        }
        if (cbTotal == static_cast<std::size_t>(cbLine) && cbLine >= 3 && !std::memcmp(sz, "\xef\xbb\xbf", 3))
            std::memmove(sz, sz + 3, static_cast<std::size_t>(cbLine) - 2);
        *lplpNext = LpProfileLine(sz);
        if (!*lplpNext) { win32::SetLastError(win32::ERROR_NOT_ENOUGH_MEMORY); goto LReturn; }
        lplpNext = &(*lplpNext)->lpNext;
    }
    if (FNativeStreamError(fp)) { win32::SetLastError(win32::ERROR_READ_FAULT); goto LReturn; }
    fRet = 1;
LReturn:
    return fRet;
}

static COMPATPROFILE *LpProfileSection(COMPATPROFILE *lp, LPCSTR pszSection) {
    if (!pszSection) return nullptr;
    for (; lp; lp = lp->lpNext)
        if (lp->pszSection && !strcasecmp(lp->pszSection, pszSection)) return lp;
    return nullptr;
}

static COMPATPROFILE *LpProfileKey(COMPATPROFILE *lp, LPCSTR pszKey) {
    if (!lp || !pszKey) return nullptr;
    for (lp = lp->lpNext; lp && !lp->pszSection; lp = lp->lpNext)
        if (lp->pszKey && !strcasecmp(lp->pszKey, pszKey)) return lp;
    return nullptr;
}

static const char *PszProfileValue(COMPATPROFILE *lp, std::size_t *pcch) {
    const char *psz = lp->pszValue;
    *pcch = std::strlen(psz);
    if (*pcch >= 2 && (psz[0] == '\'' || psz[0] == '"') && psz[*pcch - 1] == psz[0]) {
        psz++; *pcch -= 2;
    }
    return psz;
}

static char *SzProfilePath(LPCSTR pszFile) {
    const char *pszDir, *pszHome;
    char *sz;
    std::size_t cb;

    if (!pszFile || !*pszFile) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return nullptr; }
    // Explicit paths remain literal. Bare settings names use the user config directory.
    if (std::strchr(pszFile, '/')
#ifdef _WIN32
        || std::strchr(pszFile, '\\') || std::strchr(pszFile, ':')
#endif
    ) { sz = strdup(pszFile); goto LReturn; }
    pszDir = PszCompatEnvironment("STARS_PROFILE_DIR");
    if (!pszDir || !*pszDir) pszDir = PszCompatEnvironment("STARS_SETTINGS_DIR");
    if (pszDir && *pszDir) {
        cb = std::strlen(pszDir) + std::strlen(pszFile) + 2;
        sz = static_cast<char *>((std::malloc(cb)));
        if (sz) std::snprintf(sz, cb, "%s/%s", pszDir, pszFile);
        goto LReturn;
    }
    pszDir = PszCompatEnvironment("XDG_CONFIG_HOME");
    pszHome = PszCompatEnvironment("HOME");
#ifdef _WIN32
    pszDir = PszCompatEnvironment("LOCALAPPDATA");
    if (pszDir && *pszDir) {
        cb = std::strlen(pszDir) + std::strlen(pszFile) + 8;
        sz = static_cast<char *>(std::malloc(cb));
        if (sz) std::snprintf(sz, cb, "%s/stars/%s", pszDir, pszFile);
    } else sz = strdup(pszFile);
    static_cast<void>(pszHome);
#else
    if (pszDir && *pszDir == '/') {
        cb = std::strlen(pszDir) + std::strlen(pszFile) + 8;
        sz = static_cast<char *>((std::malloc(cb)));
        if (sz) std::snprintf(sz, cb, "%s/stars/%s", pszDir, pszFile);
    } else if (pszHome && *pszHome == '/') {
        cb = std::strlen(pszHome) + std::strlen(pszFile) + 16;
        sz = static_cast<char *>((std::malloc(cb)));
        if (sz) std::snprintf(sz, cb, "%s/.config/stars/%s", pszHome, pszFile);
    } else {
        sz = strdup(pszFile);
    }
#endif
LReturn:
    if (!sz) win32::SetLastError(win32::ERROR_NOT_ENOUGH_MEMORY);
    else if (!FUtf8Valid(sz, std::strlen(sz))) { std::free(sz); sz = nullptr; win32::SetLastError(win32::ERROR_INVALID_PARAMETER); }
    return sz;
}

static BOOL FMakeProfileDir(char *szPath) {
    char *psz;
    char ch;
    int err;

    for (psz = szPath + 1; *psz; psz++) {
        if (*psz != '/'
#ifdef _WIN32
            && *psz != '\\'
#endif
        ) continue;
#ifdef _WIN32
        if (psz == szPath + 2 && szPath[1] == ':') continue;
#endif
        ch = *psz;
        *psz = 0;
        err = ICreateNativeDirectory(szPath, 0700);
        *psz = ch;
        if (err != 0 && errno != EEXIST) { win32::SetLastError(DwProfileError()); return 0; }
    }
    return 1;
}

DWORD win32::GetPrivateProfileStringA(LPCSTR pszSection, LPCSTR pszKey, LPCSTR pszDefault,
                                    LPSTR szResult, DWORD cchMax, LPCSTR pszFile) {
    char *szPath = nullptr;
    COMPATPROFILE *lpAll = nullptr, *lp, *lpKey;
    const char *psz;
    std::size_t cch, cchOut = 0, cchCopy;
    BOOL fMulti = !pszSection || !pszKey;
    std::unique_lock lockProfile(fProfileLock, std::defer_lock);
    NATIVECLEANUPSCOPE releaseProfile([&]() noexcept { FreeProfile(lpAll); std::free(szPath); });

    if (!szResult || cchMax == 0) return 0;
    szResult[0] = 0;
    if (fMulti && cchMax > 1) szResult[1] = 0;
    szPath = SzProfilePath(pszFile);
    if (!szPath) return 0;
    lockProfile.lock();
    FLoadProfile(szPath, &lpAll);
    if (fMulti) {
        lp = pszSection ? LpProfileSection(lpAll, pszSection) : lpAll;
        if (pszSection && lp) lp = lp->lpNext;
        for (; lp; lp = lp->lpNext) {
            if (pszSection && lp->pszSection) break;
            psz = pszSection ? lp->pszKey : lp->pszSection;
            if (!psz || !*psz) continue;
            cch = std::strlen(psz);
            if (cchMax < 2) break;
            if (cchOut > cchMax - 2) { cchOut = cchMax - 2; break; }
            cchCopy = CchUtf8Prefix(psz, cch < cchMax - 2 - cchOut ? cch : cchMax - 2 - cchOut);
            std::memcpy(szResult + cchOut, psz, cchCopy);
            cchOut += cchCopy;
            if (cchCopy < cch || cchOut + 1 >= cchMax) {
                std::fill_n(szResult + cchOut, cchMax - cchOut, 0);
                cchOut = cchMax - 2; break;
            }
            szResult[cchOut++] = 0;
            szResult[cchOut] = 0;
        }
        // The reference API reports nSize-2 even when a list exactly fills the buffer.
        if (cchMax >= 2 && cchOut == cchMax - 1) cchOut = cchMax - 2;
    } else {
        lpKey = LpProfileKey(LpProfileSection(lpAll, pszSection), pszKey);
        if (lpKey) psz = PszProfileValue(lpKey, &cch);
        else {
            psz = pszDefault ? pszDefault : "";
            cch = std::strlen(psz);
            while (cch != 0 && psz[cch - 1] == ' ') cch--;
        }
        cchOut = CchUtf8Prefix(psz, cch < cchMax - 1 ? cch : cchMax - 1);
        std::memcpy(szResult, psz, cchOut); szResult[cchOut] = 0;
    }
    releaseProfile.Close();
    lockProfile.unlock();
    return static_cast<DWORD>(cchOut);
}

UINT win32::GetPrivateProfileIntA(LPCSTR pszSection, LPCSTR pszKey, INT nDefault, LPCSTR pszFile) {
    char sz[128], *psz = sz;
    UINT u = 0, digit, radix = 10;
    BOOL fNegative = 0;

    if (ProfileTextIO().pfnReadString(pszSection, pszKey, "", sz, sizeof(sz), pszFile) == 0)
        return static_cast<UINT>(nDefault);
    while (std::isspace(static_cast<unsigned char>(*psz))) psz++;
    if (*psz == '+' || *psz == '-') { fNegative = *psz == '-'; psz++; }
    if (psz[0] == '0' && (psz[1] == 'x' || psz[1] == 'X')) { radix = 16; psz += 2; }
    for (; *psz; psz++) {
        if (*psz >= '0' && *psz <= '9') digit = static_cast<UINT>((*psz - '0'));
        else if (*psz >= 'a' && *psz <= 'f') digit = static_cast<UINT>((*psz - 'a' + 10));
        else if (*psz >= 'A' && *psz <= 'F') digit = static_cast<UINT>((*psz - 'A' + 10));
        else break;
        if (digit >= radix) break;
        u = u * radix + digit;
    }
    return fNegative ? 0u - u : u;
}

BOOL win32::WritePrivateProfileStringA(LPCSTR pszSection, LPCSTR pszKey, LPCSTR pszValue, LPCSTR pszFile) {
    char *szPath = nullptr, *szLine = nullptr;
    COMPATPROFILE *lpAll = nullptr, *lpSection, *lpKey, *lpNew, *lp;
    COMPATPROFILE **lplp;
    BOOL fRet = 0;
    std::size_t cb;
    std::unique_lock lockProfile(fProfileLock, std::defer_lock);
    NATIVECLEANUPSCOPE releaseProfile([&]() noexcept { std::free(szLine); std::free(szPath); FreeProfile(lpAll); });

    if (!pszSection && !pszKey && !pszValue) return 0; // No cache to flush.
    if (!pszSection || !FUtf8Valid(pszSection, std::strlen(pszSection)) ||
        (pszKey && !FUtf8Valid(pszKey, std::strlen(pszKey))) ||
        (pszValue && !FUtf8Valid(pszValue, std::strlen(pszValue))) || !*pszSection || std::strchr(pszSection, '\n') || std::strchr(pszSection, '\r') ||
        std::strchr(pszSection, ']') || (pszKey && (std::strchr(pszKey, '=') || std::strchr(pszKey, '\n') || std::strchr(pszKey, '\r'))) ||
        (pszValue && (std::strchr(pszValue, '\n') || std::strchr(pszValue, '\r')))) {
        win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return 0;
    }
    szPath = SzProfilePath(pszFile);
    if (!szPath) return 0;
    lockProfile.lock();
    if (!FLoadProfile(szPath, &lpAll)) goto LReturn;
    lpSection = LpProfileSection(lpAll, pszSection);
    lpKey = LpProfileKey(lpSection, pszKey);
    if (!pszKey || !pszValue) {
        if (!lpSection || (pszKey && !lpKey)) { fRet = 1; goto LReturn; }
        for (lplp = &lpAll; *lplp && *lplp != (pszKey ? lpKey : lpSection); lplp = &(*lplp)->lpNext) {}
        do {
            lp = *lplp; *lplp = lp->lpNext; lp->lpNext = nullptr; FreeProfile(lp);
        } while (!pszKey && *lplp && !(*lplp)->pszSection);
    } else {
        if (!lpSection) {
            cb = std::strlen(pszSection) + 4;
            szLine = static_cast<char *>((std::malloc(cb)));
            if (!szLine) goto LMemory;
            std::snprintf(szLine, cb, "[%s]\n", pszSection);
            lpSection = LpProfileLine(szLine);
            std::free(szLine); szLine = nullptr;
            if (!lpSection) goto LMemory;
            for (lplp = &lpAll; *lplp; lplp = &(*lplp)->lpNext) {}
            *lplp = lpSection;
        }
        cb = std::strlen(pszKey) + std::strlen(pszValue) + 3;
        szLine = static_cast<char *>((std::malloc(cb)));
        if (!szLine) goto LMemory;
        std::snprintf(szLine, cb, "%s=%s\n", pszKey, pszValue);
        lpNew = LpProfileLine(szLine);
        if (!lpNew) goto LMemory;
        for (lplp = &lpSection->lpNext; *lplp && !(*lplp)->pszSection && *lplp != lpKey; lplp = &(*lplp)->lpNext) {}
        if (lpKey) {
            lpNew->lpNext = lpKey->lpNext; lpKey->lpNext = nullptr; FreeProfile(lpKey);
        } else lpNew->lpNext = *lplp;
        *lplp = lpNew;
    }
    if (!std::strchr(pszFile, '/') && !FMakeProfileDir(szPath)) goto LReturn;
    fRet = FSaveProfile(lpAll, szPath);
    goto LReturn;
LMemory:
    win32::SetLastError(win32::ERROR_NOT_ENOUGH_MEMORY);
LReturn:
    releaseProfile.Close();
    lockProfile.unlock();
    return fRet;
}
