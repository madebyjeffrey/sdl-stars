/* WinHelp's separate viewer is adapted to the packaged original HTML guide. */
#include "internal.h"
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

static BOOL FHelpArchive(const char *pszFile) {
    FILE *lpFile;
    unsigned char rgb[16384];
    size_t cb, cbRead;
    uint32_t cbLeft = STARS_HELP_ARCHIVE_SIZE, dwCrc = 0;
    BOOL fRead;

    lpFile = fopen(pszFile, "rb");
    if (!lpFile) {
        SetLastError(errno == EACCES ? ERROR_ACCESS_DENIED : ERROR_FILE_NOT_FOUND);
        return FALSE;
    }
    while (cbLeft != 0) {
        cb = cbLeft < sizeof(rgb) ? cbLeft : sizeof(rgb);
        cbRead = fread(rgb, 1, cb, lpFile);
        if (cbRead != cb) break;
        dwCrc = (uint32_t)crc32(dwCrc, rgb, (uInt)cbRead);
        cbLeft -= (uint32_t)cbRead;
    }
    fRead = !ferror(lpFile);
    fclose(lpFile);
    if (!fRead || cbLeft != 0 || dwCrc != STARS_HELP_ARCHIVE_CRC) {
        SetLastError(ERROR_BAD_FORMAT);
        return FALSE;
    }
    return TRUE;
}

static BOOL FHelpAssets(const char *pszDirectory) {
    char szFile[PATH_MAX], szStamp[66];
    struct stat st;
    size_t i;
    int cch;
    FILE *lpFile;
    BOOL fStamp;

    for (i = 0; i < sizeof(rgHelpFile) / sizeof(rgHelpFile[0]); i++) {
        cch = snprintf(szFile, sizeof(szFile), "%s/%s", pszDirectory, rgHelpFile[i]);
        if (cch < 0 || (size_t)cch >= sizeof(szFile)) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            return FALSE;
        }
        if (stat(szFile, &st) || !S_ISREG(st.st_mode) || st.st_size <= 0) {
            SetLastError(ERROR_FILE_NOT_FOUND);
            return FALSE;
        }
    }
    snprintf(szFile, sizeof(szFile), "%s/complete.stamp", pszDirectory);
    lpFile = fopen(szFile, "rb");
    if (!lpFile) {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    fStamp = fgets(szStamp, sizeof(szStamp), lpFile) && !strcmp(szStamp, STARS_HELP_SHA "\n");
    fclose(lpFile);
    if (!fStamp) SetLastError(ERROR_BAD_FORMAT);
    return fStamp;
}

static char *PszHelpDocument(const char *pszDirectory, const char *pszPage) {
    static const char szMarker[] = "__STARS_HELP_START__";
    char szSource[PATH_MAX], szTarget[PATH_MAX], szTemp[PATH_MAX];
    char *pszBase, *pszBody = NULL, *pszMarker, *pszResult = NULL;
    FILE *lpSource = NULL, *lpTarget = NULL;
    size_t cbPrefix, cbRead;
    int fd, cch;
    BOOL fOk;

    cch = snprintf(szSource, sizeof(szSource), "%s/offline.html", pszDirectory);
    if (cch < 0 || (size_t)cch >= sizeof(szSource)) goto fail;
    lpSource = fopen(szSource, "rb");
    pszBody = malloc((size_t)STARS_HELP_BUNDLE_SIZE + 1);
    if (!lpSource || !pszBody) goto fail;
    cbRead = fread(pszBody, 1, STARS_HELP_BUNDLE_SIZE, lpSource);
    fOk = cbRead == STARS_HELP_BUNDLE_SIZE && !ferror(lpSource) && fgetc(lpSource) == EOF;
    fclose(lpSource); lpSource = NULL;
    if (!fOk || (uint32_t)crc32(0, (const BYTE *)pszBody, (uInt)cbRead) != STARS_HELP_BUNDLE_CRC) {
        SetLastError(ERROR_BAD_FORMAT); goto done;
    }
    pszBody[cbRead] = 0;
    pszMarker = strstr(pszBody, szMarker);
    if (!pszMarker || strstr(pszMarker + sizeof(szMarker) - 1, szMarker)) {
        SetLastError(ERROR_BAD_FORMAT); goto done;
    }
    // Put the requested context inside the exported file: OpenFile portals
    // receive a descriptor and cannot carry a browser URL fragment with it.
    pszBase = SDL_GetPrefPath("stars", "help");
    if (!pszBase) goto fail;
    cch = snprintf(szTarget, sizeof(szTarget), "%sguide-%08x-%s", pszBase, STARS_HELP_BUNDLE_CRC, pszPage);
    SDL_free(pszBase);
    if (cch < 0 || (size_t)cch >= sizeof(szTarget)) goto fail;
    cch = snprintf(szTemp, sizeof(szTemp), "%s.tmp-XXXXXX", szTarget);
    if (cch < 0 || (size_t)cch >= sizeof(szTemp)) goto fail;
    fd = mkstemp(szTemp);
    if (fd < 0) goto fail;
    lpTarget = fdopen(fd, "wb");
    if (!lpTarget) { close(fd); unlink(szTemp); goto fail; }
    cbPrefix = (size_t)(pszMarker - pszBody);
    fOk = fwrite(pszBody, 1, cbPrefix, lpTarget) == cbPrefix &&
          fputs(pszPage, lpTarget) >= 0 &&
          fwrite(pszMarker + sizeof(szMarker) - 1, 1, cbRead - cbPrefix - sizeof(szMarker) + 1, lpTarget) == cbRead - cbPrefix - sizeof(szMarker) + 1;
    if (fflush(lpTarget) || fsync(fileno(lpTarget))) fOk = FALSE;
    if (fclose(lpTarget)) fOk = FALSE;
    lpTarget = NULL;
    if (!fOk || rename(szTemp, szTarget)) { unlink(szTemp); goto fail; }
    pszResult = strdup(szTarget);
    if (!pszResult) goto fail;
    goto done;
fail:
    SetLastError(ERROR_ACCESS_DENIED);
done:
    if (lpSource) fclose(lpSource);
    free(pszBody);
    return pszResult;
}

BOOL WINAPI WinHelpA(HWND hWndMain, LPCSTR pszHelp, UINT uCommand, ULONG_PTR dwData) {
    const char *pszBase, *pszName, *pszPage = NULL;
    char szFile[PATH_MAX], szDirectory[PATH_MAX];
    char *pszDirectory, *pszDocument;
    size_t i;
    int cch;
    BOOL fLaunched;

    (void)hWndMain;
    if (uCommand != HELP_INDEX && uCommand != HELP_CONTEXT) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }
    if (!pszHelp || !*pszHelp || strlen(pszHelp) >= sizeof(szFile)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    pszName = strrchr(pszHelp, '/');
    pszName = pszName ? pszName + 1 : pszHelp;
    if (strcasecmp(pszName, "stars!.hlp") || strchr(pszHelp, '\\')) {
        SetLastError(ERROR_NOT_SUPPORTED);
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
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    pszBase = SDL_GetBasePath();
    if (!pszBase) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }
    // Explicit paths must exist. The game's basename also searches beside
    // the executable, so help works when launched from another directory.
    if (!FHelpArchive(pszHelp)) {
        if (strchr(pszHelp, '/') || GetLastError() != ERROR_FILE_NOT_FOUND) return FALSE;
        cch = snprintf(szFile, sizeof(szFile), "%sstars!.hlp", pszBase);
        if (cch < 0 || (size_t)cch >= sizeof(szFile)) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            return FALSE;
        }
        if (!FHelpArchive(szFile)) {
            if (GetLastError() != ERROR_FILE_NOT_FOUND) return FALSE;
            cch = snprintf(szFile, sizeof(szFile), "%s%s/stars!.hlp", pszBase, STARS_INSTALLED_DATA);
            if (cch < 0 || (size_t)cch >= sizeof(szFile)) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                return FALSE;
            }
            if (!FHelpArchive(szFile)) return FALSE;
        }
    }
    cch = snprintf(szDirectory, sizeof(szDirectory), "%s../help", pszBase);
    if (cch < 0 || (size_t)cch >= sizeof(szDirectory)) {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return FALSE;
    }
    pszDirectory = realpath(szDirectory, NULL);
    if (!pszDirectory) {
        cch = snprintf(szDirectory, sizeof(szDirectory), "%s%s/help", pszBase, STARS_INSTALLED_DATA);
        if (cch < 0 || (size_t)cch >= sizeof(szDirectory)) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            return FALSE;
        }
        pszDirectory = realpath(szDirectory, NULL);
        if (!pszDirectory) {
            SetLastError(ERROR_PATH_NOT_FOUND);
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
    SetLastError(fLaunched ? NO_ERROR : ERROR_GEN_FAILURE);
    return fLaunched;
}
