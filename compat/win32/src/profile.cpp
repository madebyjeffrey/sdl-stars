#include "locking.h"
#include <algorithm>
#include "ownership.h"
/* File-backed UTF-8 profiles. Preserve unrelated lines when updating settings. */
#include <windows.h>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include "platform.h"
#include "encoding.h"
#include "native.h"

#include "profile_io.h"
#include "profile_text_io.h"

static NATIVEFLAGLOCK fProfileLock;

static void FreeProfile(COMPATPROFILE *lp) {
    COMPATPROFILE *lpNext;
    while (lp) {
        lpNext = lp->lpNext;
        delete lp;
        lp = lpNext;
    }
}

static std::string_view SzTrimProfile(std::string_view sz) {
    while (!sz.empty() && std::isspace(static_cast<unsigned char>(sz.front()))) sz.remove_prefix(1);
    while (!sz.empty() && std::isspace(static_cast<unsigned char>(sz.back()))) sz.remove_suffix(1);
    return sz;
}

static BOOL FEqualProfileName(std::string_view szLeft, const char *pszRight) {
    const std::string_view szRight{pszRight};
    if (szLeft.size() != szRight.size()) return 0;
    return std::equal(szLeft.begin(), szLeft.end(), szRight.begin(), [](char chLeft, char chRight) {
        return std::tolower(static_cast<unsigned char>(chLeft)) == std::tolower(static_cast<unsigned char>(chRight));
    });
}

static COMPATPROFILE *LpProfileLine(const char *pszRaw) {
    COMPATPROFILE *lp = new (std::nothrow) COMPATPROFILE{};
    std::string_view szParsed;
    std::size_t ichEnd, ichEquals;

    NATIVEOWNER<COMPATPROFILE, FreeProfile> profileOwner(lp);
    if (!lp) return nullptr;
    try { lp->szRaw = pszRaw; lp->szParsed = pszRaw; }
    catch (const std::bad_alloc &) { return nullptr; }
    szParsed = lp->szParsed;
    ichEnd = szParsed.find('\0');
    if (ichEnd != std::string_view::npos) szParsed = szParsed.substr(0, ichEnd);
    szParsed = SzTrimProfile(szParsed);
    if (!szParsed.empty() && szParsed.front() == '[' && (ichEnd = szParsed.find(']', 1)) != std::string_view::npos) {
        lp->pszSection = SzTrimProfile(szParsed.substr(1, ichEnd - 1));
    } else if (!szParsed.empty() && szParsed.front() != ';' && (ichEquals = szParsed.find('=')) != std::string_view::npos) {
        lp->pszKey = SzTrimProfile(szParsed.substr(0, ichEquals));
        lp->pszValue = SzTrimProfile(szParsed.substr(ichEquals + 1));
    }
    return profileOwner.release();
}

static BOOL FLoadProfile(const char *pszPath, COMPATPROFILE **lplp) {
    const PROFILEIO &io = ProfileIO();
    NATIVEFILE *fp = nullptr;
    std::optional<std::string> sz;
    std::size_t cbTotal = 0, cbLine;
    COMPATPROFILE **lplpNext = lplp;
    BOOL fRet = 0;
    NATIVECLEANUPSCOPE releaseProfile([&]() noexcept {
        sz.reset();
        if (fp) io.pfnClose(fp);
        if (!fRet) { FreeProfile(*lplp); *lplp = nullptr; }
    });

    *lplp = nullptr;
    fp = io.pfnOpen(pszPath, NATIVEFILEMODE::read);
    if (!fp) {
        win32::SetLastError(DwProfileError());
        return errno == ENOENT;
    }
    while ((sz = io.pfnReadLine(fp))) {
        cbLine = sz->size();
        cbTotal += cbLine;
        if (cbTotal > 8 * 1024 * 1024 || !FUtf8Valid(std::string_view{sz->data(), cbLine})) {
            win32::SetLastError(win32::ERROR_BAD_FORMAT); goto LReturn;
        }
        if (cbTotal == cbLine && cbLine >= 3 && !std::memcmp(sz->data(), "\xef\xbb\xbf", 3))
            sz->erase(0, 3);
        *lplpNext = LpProfileLine(sz->c_str());
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
        if (lp->pszSection && FEqualProfileName(*lp->pszSection, pszSection)) return lp;
    return nullptr;
}

static COMPATPROFILE *LpProfileKey(COMPATPROFILE *lp, LPCSTR pszKey) {
    if (!lp || !pszKey) return nullptr;
    for (lp = lp->lpNext; lp && !lp->pszSection; lp = lp->lpNext)
        if (lp->pszKey && FEqualProfileName(*lp->pszKey, pszKey)) return lp;
    return nullptr;
}

static std::string_view SzvProfileValue(COMPATPROFILE *lp) {
    std::string_view szValue = lp->pszValue.value_or(std::string_view{});
    if (szValue.size() >= 2 && (szValue.front() == '\'' || szValue.front() == '"') && szValue.back() == szValue.front()) {
        szValue.remove_prefix(1);
        szValue.remove_suffix(1);
    }
    return szValue;
}

static std::optional<std::string> SzProfilePath(LPCSTR pszFile) {
    const char *pszDir, *pszHome;
    std::string sz;

    if (!pszFile || !*pszFile) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return std::nullopt; }
    try {
        // Explicit paths remain literal. Bare settings names use the user config directory.
        if (std::strchr(pszFile, '/')
#ifdef _WIN32
            || std::strchr(pszFile, '\\') || std::strchr(pszFile, ':')
#endif
        ) sz = pszFile;
        else {
            pszDir = PszCompatEnvironment("STARS_PROFILE_DIR");
            if (!pszDir || !*pszDir) pszDir = PszCompatEnvironment("STARS_SETTINGS_DIR");
            if (pszDir && *pszDir) {
                sz = pszDir; sz += '/'; sz += pszFile;
            } else {
                pszDir = PszCompatEnvironment("XDG_CONFIG_HOME");
                pszHome = PszCompatEnvironment("HOME");
#ifdef _WIN32
                pszDir = PszCompatEnvironment("LOCALAPPDATA");
                if (pszDir && *pszDir) { sz = pszDir; sz += "/stars/"; sz += pszFile; }
                else sz = pszFile;
                static_cast<void>(pszHome);
#else
                if (pszDir && *pszDir == '/') { sz = pszDir; sz += "/stars/"; sz += pszFile; }
                else if (pszHome && *pszHome == '/') { sz = pszHome; sz += "/.config/stars/"; sz += pszFile; }
                else sz = pszFile;
#endif
            }
        }
    } catch (const std::bad_alloc &) {
        win32::SetLastError(win32::ERROR_NOT_ENOUGH_MEMORY);
        return std::nullopt;
    }
    if (!FUtf8Valid(sz)) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return std::nullopt; }
    return std::optional<std::string>{std::move(sz)};
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
    std::optional<std::string> szPath;
    COMPATPROFILE *lpAll = nullptr, *lp, *lpKey;
    std::string_view szName;
    std::string_view szValue;
    std::size_t cch, cchOut = 0, cchCopy;
    BOOL fMulti = !pszSection || !pszKey;
    std::unique_lock lockProfile(fProfileLock, std::defer_lock);
    NATIVECLEANUPSCOPE releaseProfile([&]() noexcept { FreeProfile(lpAll); });

    if (!szResult || cchMax == 0) return 0;
    szResult[0] = 0;
    if (fMulti && cchMax > 1) szResult[1] = 0;
    szPath = SzProfilePath(pszFile);
    if (!szPath) return 0;
    lockProfile.lock();
    FLoadProfile(szPath->c_str(), &lpAll);
    if (fMulti) {
        lp = pszSection ? LpProfileSection(lpAll, pszSection) : lpAll;
        if (pszSection && lp) lp = lp->lpNext;
        for (; lp; lp = lp->lpNext) {
            if (pszSection && lp->pszSection) break;
            const auto &pszName = pszSection ? lp->pszKey : lp->pszSection;
            if (!pszName || pszName->empty()) continue;
            szName = *pszName;
            cch = szName.size();
            if (cchMax < 2) break;
            if (cchOut > cchMax - 2) { cchOut = cchMax - 2; break; }
            cchCopy = CchUtf8Prefix(szName, cch < cchMax - 2 - cchOut ? cch : cchMax - 2 - cchOut);
            std::memcpy(szResult + cchOut, szName.data(), cchCopy);
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
        if (lpKey) szValue = SzvProfileValue(lpKey);
        else {
            szValue = pszDefault ? pszDefault : "";
            while (!szValue.empty() && szValue.back() == ' ') szValue.remove_suffix(1);
        }
        cchOut = CchUtf8Prefix(szValue.data(), szValue.size() < cchMax - 1 ? szValue.size() : cchMax - 1);
        std::memcpy(szResult, szValue.data(), cchOut); szResult[cchOut] = 0;
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
    char *szLine = nullptr;
    std::optional<std::string> szPath;
    COMPATPROFILE *lpAll = nullptr, *lpSection, *lpKey, *lpNew, *lp;
    COMPATPROFILE **lplp;
    BOOL fRet = 0;
    std::size_t cb;
    std::unique_lock lockProfile(fProfileLock, std::defer_lock);
    NATIVECLEANUPSCOPE releaseProfile([&]() noexcept { std::free(szLine); FreeProfile(lpAll); });

    if (!pszSection && !pszKey && !pszValue) return 0; // No cache to flush.
    if (!pszSection || !FUtf8Valid(pszSection) ||
        (pszKey && !FUtf8Valid(pszKey)) ||
        (pszValue && !FUtf8Valid(pszValue)) || !*pszSection || std::strchr(pszSection, '\n') || std::strchr(pszSection, '\r') ||
        std::strchr(pszSection, ']') || (pszKey && (std::strchr(pszKey, '=') || std::strchr(pszKey, '\n') || std::strchr(pszKey, '\r'))) ||
        (pszValue && (std::strchr(pszValue, '\n') || std::strchr(pszValue, '\r')))) {
        win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return 0;
    }
    szPath = SzProfilePath(pszFile);
    if (!szPath) return 0;
    lockProfile.lock();
    if (!FLoadProfile(szPath->c_str(), &lpAll)) goto LReturn;
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
            CchSnprintf(std::span{szLine, cb}, "[%s]\n", pszSection);
            lpSection = LpProfileLine(szLine);
            std::free(szLine); szLine = nullptr;
            if (!lpSection) goto LMemory;
            for (lplp = &lpAll; *lplp; lplp = &(*lplp)->lpNext) {}
            *lplp = lpSection;
        }
        cb = std::strlen(pszKey) + std::strlen(pszValue) + 3;
        szLine = static_cast<char *>((std::malloc(cb)));
        if (!szLine) goto LMemory;
        CchSnprintf(std::span{szLine, cb}, "%s=%s\n", pszKey, pszValue);
        lpNew = LpProfileLine(szLine);
        if (!lpNew) goto LMemory;
        for (lplp = &lpSection->lpNext; *lplp && !(*lplp)->pszSection && *lplp != lpKey; lplp = &(*lplp)->lpNext) {}
        if (lpKey) {
            lpNew->lpNext = lpKey->lpNext; lpKey->lpNext = nullptr; FreeProfile(lpKey);
        } else lpNew->lpNext = *lplp;
        *lplp = lpNew;
    }
    if (!std::strchr(pszFile, '/') && !FMakeProfileDir(szPath->data())) goto LReturn;
    fRet = FSaveProfile(lpAll, szPath->c_str());
    goto LReturn;
LMemory:
    win32::SetLastError(win32::ERROR_NOT_ENOUGH_MEMORY);
LReturn:
    releaseProfile.Close();
    lockProfile.unlock();
    return fRet;
}
