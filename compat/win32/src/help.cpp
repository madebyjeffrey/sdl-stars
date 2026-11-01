/* WinHelp's separate viewer is adapted to the packaged original HTML guide. */
#include "internal.h"
#include "path_io.h"
#include <SDL3/SDL.h>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>
#include "help_contexts.h"

#ifndef STARS_INSTALLED_DATA
#define STARS_INSTALLED_DATA "../share/stars"
#endif

static BOOL FHelpAssets(const char *pszDirectory) {
    char szFile[PATH_MAX], szStamp[66];
    std::optional<NATIVEFILEINFO> st;
    std::size_t i;
    int cch;
    NATIVEFILE *lpFile;
    BOOL fStamp;

    for (i = 0; i < sizeof(rgHelpFile) / sizeof(rgHelpFile[0]); i++) {
        cch = std::snprintf(szFile, sizeof(szFile), "%s/%s", pszDirectory, rgHelpFile[i]);
        if (cch < 0 || static_cast<std::size_t>(cch) >= sizeof(szFile)) {
            win32::SetLastError(win32::ERROR_FILENAME_EXCED_RANGE);
            return 0;
        }
        if (!(st = IGetNativeFileInfo(szFile, NATIVELINKMODE::follow)) || !st->fRegular || st->cb <= 0) {
            win32::SetLastError(win32::ERROR_FILE_NOT_FOUND);
            return 0;
        }
    }
    std::snprintf(szFile, sizeof(szFile), "%s/complete.stamp", pszDirectory);
    lpFile = PfNativeOpen(szFile, NATIVEFILEMODE::read);
    if (!lpFile) {
        win32::SetLastError(win32::ERROR_ACCESS_DENIED);
        return 0;
    }
    fStamp = PszGetNativeLine(szStamp, sizeof(szStamp), lpFile) && !std::strcmp(szStamp, STARS_HELP_STAMP);
    ICloseNativeFile(lpFile);
    if (!fStamp) win32::SetLastError(win32::ERROR_BAD_FORMAT);
    return fStamp;
}

static char *PszHelpDocument(const char *pszDirectory, const char *pszPage) {
    static const char szMarker[] = "__STARS_HELP_START__";
    char szSource[PATH_MAX], szTarget[PATH_MAX], szTemp[PATH_MAX];
    char *pszBase, *pszBody = nullptr, *pszMarker, *pszResult = nullptr;
    NATIVEFILE *lpSource = nullptr, *lpTarget = nullptr;
    std::size_t cbPrefix, cbRead;
    int cch;
    BOOL fOk;
    NATIVEOWNER<char, std::free> bodyOwner;
    NATIVEOWNER<char, SDL_free> baseOwner;
    NATIVEOWNER<NATIVEFILE, ICloseNativeFile> sourceOwner, targetOwner;

    cch = std::snprintf(szSource, sizeof(szSource), "%s/offline.html", pszDirectory);
    if (cch < 0 || static_cast<std::size_t>(cch) >= sizeof(szSource)) goto fail;
    lpSource = PfNativeOpen(szSource, NATIVEFILEMODE::read);
    sourceOwner.reset(lpSource);
    pszBody = static_cast<char *>((std::malloc(static_cast<std::size_t>(STARS_HELP_BUNDLE_SIZE) + 1)));
    bodyOwner.reset(pszBody);
    if (!lpSource || !pszBody) goto fail;
    cbRead = CReadNative(std::span{pszBody, static_cast<std::size_t>(STARS_HELP_BUNDLE_SIZE)}, lpSource);
    fOk = cbRead == STARS_HELP_BUNDLE_SIZE && !FNativeStreamError(lpSource) && ChGetNativeFile(lpSource) == EOF;
    sourceOwner.reset(); lpSource = nullptr;
    if (!fOk || static_cast<std::uint32_t>(crc32(0, reinterpret_cast<const BYTE *>(pszBody), static_cast<uInt>(cbRead))) != STARS_HELP_BUNDLE_CRC) {
        win32::SetLastError(win32::ERROR_BAD_FORMAT); goto done;
    }
    pszBody[cbRead] = 0;
    pszMarker = std::strstr(pszBody, szMarker);
    if (!pszMarker || std::strstr(pszMarker + sizeof(szMarker) - 1, szMarker)) {
        win32::SetLastError(win32::ERROR_BAD_FORMAT); goto done;
    }
    // Put the requested context inside the exported file: OpenFile portals
    // receive a descriptor and cannot carry a browser URL fragment with it.
    pszBase = CompatPathIO().pfnGetPrefPath("stars", "help");
    baseOwner.reset(pszBase);
    if (!pszBase) goto fail;
    cch = std::snprintf(szTarget, sizeof(szTarget), "%sguide-%08x-%s", pszBase, STARS_HELP_BUNDLE_CRC, pszPage);
    baseOwner.reset();
    if (cch < 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget)) goto fail;
    cch = std::snprintf(szTemp, sizeof(szTemp), "%s.tmp-XXXXXX", szTarget);
    if (cch < 0 || static_cast<std::size_t>(cch) >= sizeof(szTemp)) goto fail;
    lpTarget = PfNativeCreateTemp(szTemp);
    targetOwner.reset(lpTarget);
    if (!lpTarget) goto fail;
    cbPrefix = static_cast<std::size_t>((pszMarker - pszBody));
    fOk = CWriteNative(std::span{pszBody, static_cast<std::size_t>(STARS_HELP_BUNDLE_SIZE)}.first(cbPrefix), lpTarget) == cbPrefix &&
          IPutNativeString(pszPage, lpTarget) >= 0 &&
          CWriteNative(std::span{pszBody, cbRead}.subspan(cbPrefix + sizeof(szMarker) - 1), lpTarget) == cbRead - cbPrefix - sizeof(szMarker) + 1;
    if (IFlushNativeFile(lpTarget) || ISyncNativeFile(lpTarget)) fOk = 0;
    if (ICloseNativeFile(targetOwner.release())) fOk = 0;
    lpTarget = nullptr;
    if (!fOk || IRenameNativePath(szTemp, szTarget, 1, 1)) { IUnlinkNativePath(szTemp); goto fail; }
    pszResult = strdup(szTarget);
    if (!pszResult) goto fail;
    goto done;
fail:
    win32::SetLastError(win32::ERROR_ACCESS_DENIED);
done:
    return pszResult;
}

BOOL win32::WinHelpA(HWND hWndMain, LPCSTR pszHelp, UINT uCommand, ULONG_PTR dwData) {
    const char *pszBase, *pszName, *pszPage = nullptr;
    char szDirectory[PATH_MAX];
    char *pszDirectory, *pszDocument;
    std::size_t i;
    int cch;
    BOOL fLaunched;

    static_cast<void>(hWndMain);
    if (uCommand != win32::HELP_INDEX && uCommand != win32::HELP_CONTEXT) {
        win32::SetLastError(win32::ERROR_NOT_SUPPORTED);
        return 0;
    }
    if (!pszHelp || !*pszHelp || std::strlen(pszHelp) >= PATH_MAX) {
        win32::SetLastError(win32::ERROR_INVALID_PARAMETER);
        return 0;
    }
    pszName = std::strrchr(pszHelp, '/');
    pszName = pszName ? pszName + 1 : pszHelp;
    if (strcasecmp(pszName, "stars-help.html") || std::strchr(pszHelp, '\\') || std::strchr(pszHelp, '/')) {
        win32::SetLastError(win32::ERROR_NOT_SUPPORTED);
        return 0;
    }
    if (uCommand == win32::HELP_INDEX) {
        pszPage = STARS_HELP_CONTENTS;
    } else if (dwData <= UINT32_MAX) {
        for (i = 0; i < sizeof(rgHelpContext) / sizeof(rgHelpContext[0]); i++) {
            if (rgHelpContext[i].idh == static_cast<std::uint32_t>(dwData)) {
                pszPage = rgHelpContext[i].pszPage;
                break;
            }
        }
    }
    if (!pszPage) {
        win32::SetLastError(win32::ERROR_INVALID_PARAMETER);
        return 0;
    }
    pszBase = CompatPathIO().pfnGetBasePath();
    if (!pszBase) {
        win32::SetLastError(win32::ERROR_PATH_NOT_FOUND);
        return 0;
    }
    // The packaged UTF-8 guide is authoritative; the HLP archive is not loaded.
    cch = std::snprintf(szDirectory, sizeof(szDirectory), "%s../help", pszBase);
    if (cch < 0 || static_cast<std::size_t>(cch) >= sizeof(szDirectory)) {
        win32::SetLastError(win32::ERROR_FILENAME_EXCED_RANGE);
        return 0;
    }
    pszDirectory = PszCanonicalNativePath(szDirectory, nullptr, 0);
    if (!pszDirectory) {
        cch = std::snprintf(szDirectory, sizeof(szDirectory), "%s%s/help", pszBase, STARS_INSTALLED_DATA);
        if (cch < 0 || static_cast<std::size_t>(cch) >= sizeof(szDirectory)) {
            win32::SetLastError(win32::ERROR_FILENAME_EXCED_RANGE);
            return 0;
        }
        pszDirectory = PszCanonicalNativePath(szDirectory, nullptr, 0);
        if (!pszDirectory) {
            win32::SetLastError(win32::ERROR_PATH_NOT_FOUND);
            return 0;
        }
    }
    NATIVEOWNER<char, std::free> directoryOwner(pszDirectory);
    if (!FHelpAssets(pszDirectory)) return 0;
    pszDocument = PszHelpDocument(pszDirectory, pszPage);
    NATIVEOWNER<char, std::free> documentOwner(pszDocument);
    directoryOwner.reset();
    if (!pszDocument) return 0;
    fLaunched = FCompatOpenDocument(pszDocument);
    documentOwner.reset();
    win32::SetLastError(fLaunched ? win32::NO_ERROR : win32::ERROR_GEN_FAILURE);
    return fLaunched;
}
