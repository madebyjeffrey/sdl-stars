/* WinHelp's separate viewer is adapted to the packaged original HTML guide. */
#include <windows.h>
#include <SDL3/SDL.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
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

static char *PszHelpUri(const char *pszPath) {
    static const char rgchHex[] = "0123456789ABCDEF";
    const unsigned char *lpch;
    char *pszUri, *lpchOut;
    unsigned char ch;

    pszUri = malloc(strlen(pszPath) * 3 + 8);
    if (!pszUri) return NULL;
    memcpy(pszUri, "file://", 7);
    lpchOut = pszUri + 7;
    for (lpch = (const unsigned char *)pszPath; *lpch; lpch++) {
        ch = *lpch;
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') || strchr("/-._~", ch)) {
            *lpchOut++ = (char)ch;
        } else {
            *lpchOut++ = '%';
            *lpchOut++ = rgchHex[ch >> 4];
            *lpchOut++ = rgchHex[ch & 15];
        }
    }
    *lpchOut = 0;
    return pszUri;
}

BOOL WINAPI WinHelpA(HWND hWndMain, LPCSTR pszHelp, UINT uCommand, ULONG_PTR dwData) {
    const char *pszBase, *pszName, *pszPage = NULL;
    char szFile[PATH_MAX], szDirectory[PATH_MAX];
    char *pszDirectory, *pszUri;
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
    cch = snprintf(szFile, sizeof(szFile), "%s/%s", pszDirectory, pszPage);
    free(pszDirectory);
    if (cch < 0 || (size_t)cch >= sizeof(szFile)) {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return FALSE;
    }
    pszUri = PszHelpUri(szFile);
    if (!pszUri) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    fLaunched = SDL_OpenURL(pszUri);
    free(pszUri);
    SetLastError(fLaunched ? NO_ERROR : ERROR_GEN_FAILURE);
    return fLaunched;
}
