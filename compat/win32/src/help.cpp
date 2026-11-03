/* WinHelp's separate viewer is adapted to the packaged original HTML guide. */
#include "internal.h"
#include "native.h"
#include "path_io.h"
#include <algorithm>
#include <SDL3/SDL.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <limits>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <zlib.h>
#include "help_contexts.h"
#include "installed_data.h"

static BOOL FHelpAssets(const char *pszDirectory) {
    char szFile[PATH_MAX], szStamp[66];
    std::optional<NATIVEFILEINFO> st;
    std::size_t i;
    int cch;
    NATIVEFILE *lpFile;
    BOOL fStamp;

    for (i = 0; i < sizeof(rgHelpFile) / sizeof(rgHelpFile[0]); i++) {
        cch = CchSnprintf(std::span{szFile}, "%s/%s", pszDirectory, rgHelpFile[i]);
        if (cch < 0 || static_cast<std::size_t>(cch) >= sizeof(szFile)) {
            win32::SetLastError(win32::ERROR_FILENAME_EXCED_RANGE);
            return 0;
        }
        if (!(st = IGetNativeFileInfo(szFile, NATIVELINKMODE::follow)) || !st->fRegular || st->cb <= 0) {
            win32::SetLastError(win32::ERROR_FILE_NOT_FOUND);
            return 0;
        }
    }
    CchSnprintf(std::span{szFile}, "%s/complete.stamp", pszDirectory);
    lpFile = PfNativeOpen(szFile, NATIVEFILEMODE::read);
    if (!lpFile) {
        win32::SetLastError(win32::ERROR_ACCESS_DENIED);
        return 0;
    }
    fStamp = CchGetNativeLine(std::span{szStamp}, lpFile) && !std::strcmp(szStamp, STARS_HELP_STAMP);
    ICloseNativeFile(lpFile);
    if (!fStamp) win32::SetLastError(win32::ERROR_BAD_FORMAT);
    return fStamp;
}

static std::optional<std::string> PszHelpDocument(const char *pszDirectory, const char *pszPage) {
    static const char szMarker[] = "__STARS_HELP_START__";
    char szSource[PATH_MAX], szTarget[PATH_MAX], szTemp[PATH_MAX];
    char *pszBase;
    NATIVEFILE *lpSource = nullptr, *lpTarget = nullptr;
    NATIVEFILETEMPRESULT nft;
    std::optional<std::string> szResult;
    std::string szBody;
    std::string_view szBodyView;
    std::size_t cbPrefix, cbRead, cchBody;
    int cch;
    BOOL fOk;
    NATIVEOWNER<char, SDL_free> baseOwner;
    NATIVEOWNER<NATIVEFILE, ICloseNativeFile> sourceOwner, targetOwner;

    cch = CchSnprintf(std::span{szSource}, "%s/offline.html", pszDirectory);
    if (cch < 0 || static_cast<std::size_t>(cch) >= sizeof(szSource)) goto fail;
    lpSource = PfNativeOpen(szSource, NATIVEFILEMODE::read);
    sourceOwner.reset(lpSource);
    if (!lpSource) goto fail;
    try { szBody.resize(static_cast<std::size_t>(STARS_HELP_BUNDLE_SIZE)); }
    catch (const std::bad_alloc &) { goto fail; }
    cbRead = CReadNative(std::span{szBody.data(), static_cast<std::size_t>(STARS_HELP_BUNDLE_SIZE)}, lpSource);
    fOk = cbRead == STARS_HELP_BUNDLE_SIZE && !FNativeStreamError(lpSource) && ChGetNativeFile(lpSource) == EOF;
    sourceOwner.reset(); lpSource = nullptr;
    if (!fOk || static_cast<std::uint32_t>(crc32(0, reinterpret_cast<const BYTE *>(szBody.data()), static_cast<uInt>(cbRead))) != STARS_HELP_BUNDLE_CRC) {
        win32::SetLastError(win32::ERROR_BAD_FORMAT); goto done;
    }
    szBodyView = std::string_view{szBody.data(), cbRead};
    cchBody = szBodyView.find('\0');
    if (cchBody != std::string_view::npos) szBodyView = szBodyView.substr(0, cchBody);
    cbPrefix = szBodyView.find(szMarker);
    if (cbPrefix == std::string_view::npos || szBodyView.find(szMarker, cbPrefix + sizeof(szMarker) - 1) != std::string_view::npos) {
        win32::SetLastError(win32::ERROR_BAD_FORMAT); goto done;
    }
    // Put the requested context inside the exported file: OpenFile portals
    // receive a descriptor and cannot carry a browser URL fragment with it.
    pszBase = CompatPathIO().pfnGetPrefPath("stars", "help");
    baseOwner.reset(pszBase);
    if (!pszBase) goto fail;
    cch = CchSnprintf(std::span{szTarget}, "%sguide-%08x-%s", pszBase, STARS_HELP_BUNDLE_CRC, pszPage);
    baseOwner.reset();
    if (cch < 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget)) goto fail;
    cch = CchSnprintf(std::span{szTemp}, "%s.tmp-XXXXXX", szTarget);
    if (cch < 0 || static_cast<std::size_t>(cch) >= sizeof(szTemp)) goto fail;
    nft = NftNativeCreateTemp(szTemp);
    lpTarget = nft.lpfile.release();
    targetOwner.reset(lpTarget);
    if (!lpTarget) goto fail;
    fOk = CWriteNative(std::span{szBody.data(), static_cast<std::size_t>(STARS_HELP_BUNDLE_SIZE)}.first(cbPrefix), lpTarget) == cbPrefix &&
          IPutNativeString(pszPage, lpTarget) >= 0 &&
          CWriteNative(std::span{szBody.data(), cbRead}.subspan(cbPrefix + sizeof(szMarker) - 1), lpTarget) == cbRead - cbPrefix - sizeof(szMarker) + 1;
    if (IFlushNativeFile(lpTarget) || ISyncNativeFile(lpTarget)) fOk = 0;
    if (ICloseNativeFile(targetOwner.release())) fOk = 0;
    lpTarget = nullptr;
    if (!fOk || IRenameNativePath(nft.szPath.c_str(), szTarget, true, true)) { IUnlinkNativePath(nft.szPath.c_str()); goto fail; }
    try { szResult.emplace(szTarget); }
    catch (const std::bad_alloc &) { goto fail; }
    goto done;
fail:
    win32::SetLastError(win32::ERROR_ACCESS_DENIED);
done:
    return szResult;
}

BOOL win32::WinHelpA(HWND hWndMain, LPCSTR pszHelp, UINT uCommand, ULONG_PTR dwData) {
    const char *pszBase, *pszName, *pszPage = nullptr;
    char szDirectory[PATH_MAX];
    const char *pszDirectory;
    std::string_view szHelp;
    std::size_t i, ichSlash;
    int cch;
    BOOL fLaunched;

    static_cast<void>(hWndMain);
    if (uCommand != win32::HELP_INDEX && uCommand != win32::HELP_CONTEXT) {
        win32::SetLastError(win32::ERROR_NOT_SUPPORTED);
        return 0;
    }
    if (!pszHelp) {
        win32::SetLastError(win32::ERROR_INVALID_PARAMETER);
        return 0;
    }
    szHelp = std::string_view{pszHelp, static_cast<std::size_t>(std::find(pszHelp, pszHelp + PATH_MAX, 0) - pszHelp)};
    if (szHelp.empty() || szHelp.size() >= PATH_MAX) {
        win32::SetLastError(win32::ERROR_INVALID_PARAMETER);
        return 0;
    }
    ichSlash = szHelp.rfind('/');
    pszName = ichSlash != std::string_view::npos ? pszHelp + ichSlash + 1 : pszHelp;
    if (strcasecmp(pszName, "stars-help.html") || szHelp.find_first_of("\\/") != std::string_view::npos) {
        win32::SetLastError(win32::ERROR_NOT_SUPPORTED);
        return 0;
    }
    if (uCommand == win32::HELP_INDEX) {
        pszPage = STARS_HELP_CONTENTS;
    } else if (dwData <= std::numeric_limits<std::uint32_t>::max()) {
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
    cch = CchSnprintf(std::span{szDirectory}, "%s../help", pszBase);
    if (cch < 0 || static_cast<std::size_t>(cch) >= sizeof(szDirectory)) {
        win32::SetLastError(win32::ERROR_FILENAME_EXCED_RANGE);
        return 0;
    }
    auto szDirectoryOwned = SzCanonicalNativePath(szDirectory);
    if (!szDirectoryOwned) {
        cch = CchSnprintf(std::span{szDirectory}, "%s%s/help", pszBase, szStarsInstalledData);
        if (cch < 0 || static_cast<std::size_t>(cch) >= sizeof(szDirectory)) {
            win32::SetLastError(win32::ERROR_FILENAME_EXCED_RANGE);
            return 0;
        }
        szDirectoryOwned = SzCanonicalNativePath(szDirectory);
        if (!szDirectoryOwned) {
            win32::SetLastError(win32::ERROR_PATH_NOT_FOUND);
            return 0;
        }
    }
    pszDirectory = szDirectoryOwned->c_str();
    if (!FHelpAssets(pszDirectory)) return 0;
    auto szDocument = PszHelpDocument(pszDirectory, pszPage);
    szDirectoryOwned.reset();
    if (!szDocument) return 0;
    fLaunched = FCompatOpenDocument(szDocument->c_str());
    win32::SetLastError(fLaunched ? win32::NO_ERROR : win32::ERROR_GEN_FAILURE);
    return fLaunched;
}
