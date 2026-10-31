/* WinHelp's separate viewer is adapted to the packaged original HTML guide. */
#include "internal.h"
#include "path_io.h"
#include <SDL3/SDL.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    NATIVEFILEINFO st;
    size_t i;
    int cch;
    NATIVEFILE *lpFile;
    BOOL fStamp;

    for (i = 0; i < sizeof(rgHelpFile) / sizeof(rgHelpFile[0]); i++) {
        cch = snprintf(szFile, sizeof(szFile), "%s/%s", pszDirectory, rgHelpFile[i]);
        if (cch < 0 || (size_t)cch >= sizeof(szFile)) {
            win32::SetLastError(ERROR_FILENAME_EXCED_RANGE);
            return FALSE;
        }
        if (IGetNativeFileInfo(szFile, &st, TRUE) || !st.fRegular || st.cb <= 0) {
            win32::SetLastError(ERROR_FILE_NOT_FOUND);
            return FALSE;
        }
    }
    snprintf(szFile, sizeof(szFile), "%s/complete.stamp", pszDirectory);
    lpFile = PfNativeOpen(szFile, "rb");
    if (!lpFile) {
        win32::SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    fStamp = PszGetNativeLine(szStamp, sizeof(szStamp), lpFile) && !strcmp(szStamp, STARS_HELP_SHA "\n");
    ICloseNativeFile(lpFile);
    if (!fStamp) win32::SetLastError(ERROR_BAD_FORMAT);
    return fStamp;
}

static char *PszHelpDocument(const char *pszDirectory, const char *pszPage) {
    static const char szMarker[] = "__STARS_HELP_START__";
    char szSource[PATH_MAX], szTarget[PATH_MAX], szTemp[PATH_MAX];
    char *pszBase, *pszBody = NULL, *pszMarker, *pszResult = NULL;
    NATIVEFILE *lpSource = NULL, *lpTarget = NULL;
    size_t cbPrefix, cbRead;
    int cch;
    BOOL fOk;

    cch = snprintf(szSource, sizeof(szSource), "%s/offline.html", pszDirectory);
    if (cch < 0 || (size_t)cch >= sizeof(szSource)) goto fail;
    lpSource = PfNativeOpen(szSource, "rb");
    pszBody = (char *)(malloc((size_t)STARS_HELP_BUNDLE_SIZE + 1));
    if (!lpSource || !pszBody) goto fail;
    cbRead = CReadNative(pszBody, 1, STARS_HELP_BUNDLE_SIZE, lpSource);
    fOk = cbRead == STARS_HELP_BUNDLE_SIZE && !FNativeStreamError(lpSource) && ChGetNativeFile(lpSource) == EOF;
    ICloseNativeFile(lpSource); lpSource = NULL;
    if (!fOk || (uint32_t)crc32(0, (const BYTE *)pszBody, (uInt)cbRead) != STARS_HELP_BUNDLE_CRC) {
        win32::SetLastError(ERROR_BAD_FORMAT); goto done;
    }
    pszBody[cbRead] = 0;
    pszMarker = strstr(pszBody, szMarker);
    if (!pszMarker || strstr(pszMarker + sizeof(szMarker) - 1, szMarker)) {
        win32::SetLastError(ERROR_BAD_FORMAT); goto done;
    }
    // Put the requested context inside the exported file: OpenFile portals
    // receive a descriptor and cannot carry a browser URL fragment with it.
    pszBase = CompatPathIO().pfnGetPrefPath("stars", "help");
    if (!pszBase) goto fail;
    cch = snprintf(szTarget, sizeof(szTarget), "%sguide-%08x-%s", pszBase, STARS_HELP_BUNDLE_CRC, pszPage);
    SDL_free(pszBase);
    if (cch < 0 || (size_t)cch >= sizeof(szTarget)) goto fail;
    cch = snprintf(szTemp, sizeof(szTemp), "%s.tmp-XXXXXX", szTarget);
    if (cch < 0 || (size_t)cch >= sizeof(szTemp)) goto fail;
    lpTarget = PfNativeCreateTemp(szTemp);
    if (!lpTarget) goto fail;
    cbPrefix = (size_t)(pszMarker - pszBody);
    fOk = CWriteNative(pszBody, 1, cbPrefix, lpTarget) == cbPrefix &&
          IPutNativeString(pszPage, lpTarget) >= 0 &&
          CWriteNative(pszMarker + sizeof(szMarker) - 1, 1, cbRead - cbPrefix - sizeof(szMarker) + 1, lpTarget) == cbRead - cbPrefix - sizeof(szMarker) + 1;
    if (IFlushNativeFile(lpTarget) || ISyncNativeFile(lpTarget)) fOk = FALSE;
    if (ICloseNativeFile(lpTarget)) fOk = FALSE;
    lpTarget = NULL;
    if (!fOk || IRenameNativePath(szTemp, szTarget, TRUE, TRUE)) { IUnlinkNativePath(szTemp); goto fail; }
    pszResult = strdup(szTarget);
    if (!pszResult) goto fail;
    goto done;
fail:
    win32::SetLastError(ERROR_ACCESS_DENIED);
done:
    if (lpSource) ICloseNativeFile(lpSource);
    free(pszBody);
    return pszResult;
}

BOOL win32::WinHelpA(HWND hWndMain, LPCSTR pszHelp, UINT uCommand, ULONG_PTR dwData) {
    const char *pszBase, *pszName, *pszPage = NULL;
    char szDirectory[PATH_MAX];
    char *pszDirectory, *pszDocument;
    size_t i;
    int cch;
    BOOL fLaunched;

    (void)hWndMain;
    if (uCommand != HELP_INDEX && uCommand != HELP_CONTEXT) {
        win32::SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }
    if (!pszHelp || !*pszHelp || strlen(pszHelp) >= PATH_MAX) {
        win32::SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    pszName = strrchr(pszHelp, '/');
    pszName = pszName ? pszName + 1 : pszHelp;
    if (strcasecmp(pszName, "stars-help.html") || strchr(pszHelp, '\\') || strchr(pszHelp, '/')) {
        win32::SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }
    if (uCommand == HELP_INDEX) {
        pszPage = STARS_HELP_CONTENTS;
    } else if (dwData <= UINT32_MAX) {
        for (i = 0; i < sizeof(rgHelpContext) / sizeof(rgHelpContext[0]); i++) {
            if (rgHelpContext[i].idh == (uint32_t)dwData) {
                pszPage = rgHelpContext[i].pszPage;
                break;
            }
        }
    }
    if (!pszPage) {
        win32::SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    pszBase = CompatPathIO().pfnGetBasePath();
    if (!pszBase) {
        win32::SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }
    // The packaged UTF-8 guide is authoritative; the HLP archive is not loaded.
    cch = snprintf(szDirectory, sizeof(szDirectory), "%s../help", pszBase);
    if (cch < 0 || (size_t)cch >= sizeof(szDirectory)) {
        win32::SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return FALSE;
    }
    pszDirectory = PszCanonicalNativePath(szDirectory, NULL, 0);
    if (!pszDirectory) {
        cch = snprintf(szDirectory, sizeof(szDirectory), "%s%s/help", pszBase, STARS_INSTALLED_DATA);
        if (cch < 0 || (size_t)cch >= sizeof(szDirectory)) {
            win32::SetLastError(ERROR_FILENAME_EXCED_RANGE);
            return FALSE;
        }
        pszDirectory = PszCanonicalNativePath(szDirectory, NULL, 0);
        if (!pszDirectory) {
            win32::SetLastError(ERROR_PATH_NOT_FOUND);
            return FALSE;
        }
    }
    if (!FHelpAssets(pszDirectory)) {
        free(pszDirectory);
        return FALSE;
    }
    pszDocument = PszHelpDocument(pszDirectory, pszPage);
    free(pszDirectory);
    if (!pszDocument) return FALSE;
    fLaunched = FCompatOpenDocument(pszDocument);
    free(pszDocument);
    win32::SetLastError(fLaunched ? NO_ERROR : ERROR_GEN_FAILURE);
    return fLaunched;
}
