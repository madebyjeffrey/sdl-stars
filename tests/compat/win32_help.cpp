#include "../../compat/win32/src/path_io.h"
#include "../../compat/win32/src/external_io.h"
/* Packaged UTF-8 guide selection and launch contracts; no browser is opened. */
#include <windows.h>
#include <SDL3/SDL.h>
#include <limits.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "help_contexts.h"

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s (error %lu)\n", __LINE__, #f, (unsigned long)win32::GetLastError()); return 1; } } while (0)
static char szBase[PATH_MAX], szUri[PATH_MAX * 3 + 8];
static BOOL fBase = 1, fAccept = 1;
static unsigned cOpen;

extern "C" const char *SDLCALL StarsTest_SDL_GetBasePath(void) { return fBase ? szBase : NULL; }
extern "C" bool SDLCALL StarsTest_SDL_OpenURL(const char *pszUri) {
    cOpen++;
    if (strlen(pszUri) >= sizeof(szUri)) return false;
    strcpy(szUri, pszUri);
    return fAccept;
}

extern "C" char *SDLCALL StarsTest_SDL_GetPrefPath(const char *pszOrg, const char *pszApp) {
    char sz[PATH_MAX + sizeof("../export/")];
    (void)pszOrg; (void)pszApp;
    snprintf(sz, sizeof(sz), "%s../export/", szBase);
    mkdir(sz, 0700);
    return SDL_strdup(sz);
}

static BOOL FCopy(const char *pszSource, const char *pszTarget) {
    FILE *lpSource, *lpTarget;
    char rgb[16384];
    size_t cb;
    BOOL fOk;

    lpSource = fopen(pszSource, "rb");
    if (!lpSource) return 0;
    lpTarget = fopen(pszTarget, "wb");
    if (!lpTarget) { fclose(lpSource); return 0; }
    fOk = 1;
    while ((cb = fread(rgb, 1, sizeof(rgb), lpSource)) != 0) {
        if (fwrite(rgb, 1, cb, lpTarget) != cb) { fOk = 0; break; }
    }
    if (ferror(lpSource)) fOk = 0;
    fclose(lpSource);
    if (fclose(lpTarget)) fOk = 0;
    return fOk;
}

int main(int argc, char **argv) {
    char szRoot[] = "/tmp/stars help #\xc3\xa9-XXXXXX";
    char szSource[PATH_MAX], szFile[PATH_MAX], szOther[PATH_MAX];
    size_t i;
    unsigned cBefore;
    int ch;
    FILE *lpFile;
    DIR *lpDirectory;
    struct dirent *lpEntry;

    COMPATPATHIO io_compatpathio;
    io_compatpathio.pfnGetBasePath = StarsTest_SDL_GetBasePath;
    io_compatpathio.pfnGetPrefPath = StarsTest_SDL_GetPrefPath;
    COMPATPATHIOSCOPE scope_compatpathio(io_compatpathio);
    COMPATEXTERNALIO io_compatexternalio;
    io_compatexternalio.pfnOpenURL = StarsTest_SDL_OpenURL;
    COMPATEXTERNALIOSCOPE scope_compatexternalio(io_compatexternalio);

    CHECK(argc == 2 && mkdtemp(szRoot));
    CHECK(SDL_setenv_unsafe("XDG_DATA_HOME", szRoot, 1) == 0);
    snprintf(szFile, sizeof(szFile), "%s/bin", szRoot); CHECK(mkdir(szFile, 0700) == 0);
    snprintf(szBase, sizeof(szBase), "%s/bin/", szRoot);
    snprintf(szFile, sizeof(szFile), "%s/help", szRoot); CHECK(mkdir(szFile, 0700) == 0);
    snprintf(szFile, sizeof(szFile), "%s/help/fonts", szRoot); CHECK(mkdir(szFile, 0700) == 0);
    snprintf(szFile, sizeof(szFile), "%s/help/images", szRoot); CHECK(mkdir(szFile, 0700) == 0);
    for (i = 0; i < sizeof(rgHelpFile) / sizeof(rgHelpFile[0]); i++) {
        snprintf(szSource, sizeof(szSource), "%s/%s", argv[1], rgHelpFile[i]);
        snprintf(szFile, sizeof(szFile), "%s/help/%s", szRoot, rgHelpFile[i]);
        CHECK(FCopy(szSource, szFile));
    }
    CHECK(win32::WinHelpA(NULL, "stars-help.html", win32::HELP_INDEX, UINTPTR_MAX));
    CHECK(strstr(szUri, "stars%20help%20%23%C3%A9-") && strstr(szUri, "topic-0.html"));
    CHECK(win32::WinHelpA(NULL, "STARS-HELP.HTML", win32::HELP_CONTEXT, 4501) && strstr(szUri, "topic-65789.html"));
    CHECK(win32::WinHelpA(NULL, "stars-help.html", win32::HELP_CONTEXT, 1070) && strstr(szUri, "topic-3965119.html"));
    CHECK(win32::WinHelpA(NULL, "stars-help.html", win32::HELP_CONTEXT, 1017) && strstr(szUri, "topic-3442954.html"));
    for (i = 0; i < sizeof(rgHelpContext) / sizeof(rgHelpContext[0]); i++) {
        CHECK(win32::WinHelpA(NULL, "stars-help.html", win32::HELP_CONTEXT, rgHelpContext[i].idh));
        CHECK(strstr(szUri, rgHelpContext[i].pszPage));
    }
    cBefore = cOpen;
    CHECK(!win32::WinHelpA(NULL, NULL, win32::HELP_INDEX, 0) && win32::GetLastError() == win32::ERROR_INVALID_PARAMETER);
    CHECK(!win32::WinHelpA(NULL, "", win32::HELP_INDEX, 0) && win32::GetLastError() == win32::ERROR_INVALID_PARAMETER);
    CHECK(!win32::WinHelpA(NULL, "other.hlp", win32::HELP_INDEX, 0) && win32::GetLastError() == win32::ERROR_NOT_SUPPORTED);
    CHECK(!win32::WinHelpA(NULL, "stars-help.html>secondary", win32::HELP_INDEX, 0) && win32::GetLastError() == win32::ERROR_NOT_SUPPORTED);
    CHECK(!win32::WinHelpA(NULL, "stars-help.html", 2, 0) && win32::GetLastError() == win32::ERROR_NOT_SUPPORTED);
    CHECK(!win32::WinHelpA(NULL, "stars-help.html", win32::HELP_CONTEXT, 13002) && win32::GetLastError() == win32::ERROR_INVALID_PARAMETER);
    CHECK(!win32::WinHelpA(NULL, "stars-help.html", win32::HELP_CONTEXT, ((ULONG_PTR)1 << 32) | 4501) && win32::GetLastError() == win32::ERROR_INVALID_PARAMETER);
    CHECK(!win32::WinHelpA(NULL, "missing/stars-help.html", win32::HELP_INDEX, 0) && win32::GetLastError() == win32::ERROR_NOT_SUPPORTED);
    fBase = 0;
    CHECK(!win32::WinHelpA(NULL, "stars-help.html", win32::HELP_INDEX, 0) && win32::GetLastError() == win32::ERROR_PATH_NOT_FOUND);
    fBase = 1;
    CHECK(cOpen == cBefore);
    snprintf(szFile, sizeof(szFile), "%s/help/images/bm0.png", szRoot);
    snprintf(szOther, sizeof(szOther), "%s/help/images/bm0.saved", szRoot);
    CHECK(rename(szFile, szOther) == 0);
    CHECK(!win32::WinHelpA(NULL, "stars-help.html", win32::HELP_INDEX, 0) && win32::GetLastError() == win32::ERROR_FILE_NOT_FOUND && cOpen == cBefore);
    CHECK(rename(szOther, szFile) == 0);
    snprintf(szFile, sizeof(szFile), "%s/help/complete.stamp", szRoot);
    lpFile = fopen(szFile, "wb"); CHECK(lpFile);
    CHECK(fputs("wrong guide\n", lpFile) >= 0 && fclose(lpFile) == 0);
    CHECK(!win32::WinHelpA(NULL, "stars-help.html", win32::HELP_INDEX, 0) && win32::GetLastError() == win32::ERROR_BAD_FORMAT && cOpen == cBefore);
    snprintf(szSource, sizeof(szSource), "%s/complete.stamp", argv[1]); CHECK(FCopy(szSource, szFile));
    snprintf(szFile, sizeof(szFile), "%s/help/offline.html", szRoot);
    lpFile = fopen(szFile, "r+b"); CHECK(lpFile);
    CHECK(fseek(lpFile, 1000, SEEK_SET) == 0 && (ch = fgetc(lpFile)) != EOF);
    CHECK(fseek(lpFile, 1000, SEEK_SET) == 0 && fputc(ch ^ 0xff, lpFile) != EOF && fclose(lpFile) == 0);
    CHECK(!win32::WinHelpA(NULL, "stars-help.html", win32::HELP_INDEX, 0) && win32::GetLastError() == win32::ERROR_BAD_FORMAT && cOpen == cBefore);
    snprintf(szSource, sizeof(szSource), "%s/offline.html", argv[1]); CHECK(FCopy(szSource, szFile));
    fAccept = 0;
    CHECK(!win32::WinHelpA(NULL, "stars-help.html", win32::HELP_INDEX, 0) && win32::GetLastError() == win32::ERROR_GEN_FAILURE && cOpen == cBefore + 1);
    fAccept = 1;
    CHECK(win32::WinHelpA(NULL, "stars-help.html", win32::HELP_INDEX, 0));
    // The installed UTF-8 package works without the original HLP archive.
    snprintf(szFile, sizeof(szFile), "%s/share", szRoot); CHECK(mkdir(szFile, 0700) == 0);
    snprintf(szFile, sizeof(szFile), "%s/share/stars", szRoot); CHECK(mkdir(szFile, 0700) == 0);
    snprintf(szFile, sizeof(szFile), "%s/help", szRoot);
    snprintf(szOther, sizeof(szOther), "%s/share/stars/help", szRoot);
    CHECK(rename(szFile, szOther) == 0);
    CHECK(win32::WinHelpA(NULL, "stars-help.html", win32::HELP_INDEX, 0));
    CHECK(strstr(szUri, "topic-0.html"));
    CHECK(rename(szOther, szFile) == 0);
    snprintf(szFile, sizeof(szFile), "%s/share/stars", szRoot); CHECK(rmdir(szFile) == 0);
    snprintf(szFile, sizeof(szFile), "%s/share", szRoot); CHECK(rmdir(szFile) == 0);
    for (i = 0; i < sizeof(rgHelpFile) / sizeof(rgHelpFile[0]); i++) {
        snprintf(szFile, sizeof(szFile), "%s/help/%s", szRoot, rgHelpFile[i]); CHECK(unlink(szFile) == 0);
    }
    snprintf(szFile, sizeof(szFile), "%s/help/fonts", szRoot); CHECK(rmdir(szFile) == 0);
    snprintf(szFile, sizeof(szFile), "%s/help/images", szRoot); CHECK(rmdir(szFile) == 0);
    snprintf(szFile, sizeof(szFile), "%s/help", szRoot); CHECK(rmdir(szFile) == 0);
    snprintf(szFile, sizeof(szFile), "%s/bin", szRoot); CHECK(rmdir(szFile) == 0);
    snprintf(szFile, sizeof(szFile), "%s/export", szRoot);
    lpDirectory = opendir(szFile); CHECK(lpDirectory);
    while ((lpEntry = readdir(lpDirectory))) {
        if (lpEntry->d_name[0] == '.') continue;
        snprintf(szOther, sizeof(szOther), "%s/export/%s", szRoot, lpEntry->d_name);
        CHECK(unlink(szOther) == 0);
    }
    closedir(lpDirectory); CHECK(rmdir(szFile) == 0);
    CHECK(rmdir(szRoot) == 0);
    SDL_Quit();
    puts("UTF-8 guide contents, 413 contexts, package failures and encoded launch requests passed");
    return 0;
}
