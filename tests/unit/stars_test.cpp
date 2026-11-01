#include "stars_test.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#ifdef STARS_UI_SDL
#include "../../src/core/alert_io.h"
#endif

[[maybe_unused]] static char szTestLiteral1[] = "hst";

#ifdef _WIN32
#include <io.h>
#else
#include <dirent.h>
#endif

int  cStarsTestAlert;
void (*pfnStarsTestAlertObserver)();
char szStarsTestAlert[512];
int16_t iStarsTestAlertResponse;
int16_t fStarsTestAcceptLegacyRacePassword;
int16_t cStarsTestLegacyRacePasswordPrompt;
char szStarsTestLegacyRacePassword[16];

int16_t FStarsTestPromptLegacyRacePassword(APPSESSION &, char *szPassword) {
    cStarsTestLegacyRacePasswordPrompt++;
    if (!fStarsTestAcceptLegacyRacePassword)
        return 0;
    strcpy(szPassword, szStarsTestLegacyRacePassword);
    return 1;
}

// FStarsTestAlert records a message box and gives the answer that lets the
// game carry on.
static int16_t FStarsTestAlert(const char *sz, int16_t mbType) {
    if (pfnStarsTestAlertObserver) pfnStarsTestAlertObserver();
    cStarsTestAlert++;
    snprintf(szStarsTestAlert, sizeof(szStarsTestAlert), "%s", sz);
    if (((mbType & 0x000f) == win32::MB_YESNO || (mbType & 0x000f) == win32::MB_YESNOCANCEL) && iStarsTestAlertResponse != 0)
        return iStarsTestAlertResponse;
    return (mbType & 0x000f) == win32::MB_YESNO || (mbType & 0x000f) == win32::MB_YESNOCANCEL ? win32::IDYES : IDOK;
}

#ifdef STARS_UI_SDL
int16_t StarsTest_AlertSz(APPSESSION &, const char *sz, int16_t mbType) { return FStarsTestAlert(sz, mbType); }
#else
// The native tests link hostui.c for the rest of ui.h.
int16_t IdAlertBox(const char *sz, int16_t mbType) { return FStarsTestAlert(sz, mbType); }
#endif

int16_t FStarsTestInit(APPSESSION &app) {
    iStarsTestAlertResponse = 0;
    fStarsTestAcceptLegacyRacePassword = 0;
    cStarsTestLegacyRacePasswordPrompt = 0;
    szStarsTestLegacyRacePassword[0] = 0;
#ifdef STARS_UI_SDL
    app.lpui->hInst = win32::GetModuleHandleA(NULL);
    memset(&app.lpui->vtimer, 0, sizeof(TIMER));
#endif
    app.state.szBase[0] = 0;
    app.state.ini.SetWFlags(0);
    memset(&app.state.tutor, 0, sizeof(TUTOR));
    Randomize2(app, 12345);
    app.state.fFileErrSilent = 1;
    app.state.idPlayer = iplrNone;
#ifdef STARS_UI_SDL
    return FCreateStuff(app);
#else
    InitGameStuff(app);
    return 1;
#endif
}

// DeleteGameFiles removes szDir's game.* files from an earlier run.
static void DeleteGameFiles(const char *szDir) {
    char          szFile[cbTestPath];
#ifdef _WIN32
    struct _finddata_t fd;
    intptr_t hFind;

    snprintf(szFile, sizeof(szFile), "%s%sgame.*", szDir, szDirSep);
    hFind = _findfirst(szFile, &fd);
    if (hFind == -1) return;
    do {
        if (!(fd.attrib & _A_SUBDIR)) {
            snprintf(szFile, sizeof(szFile), "%s%s%s", szDir, szDirSep, fd.name);
            remove(szFile);
        }
    } while (_findnext(hFind, &fd) == 0);
    _findclose(hFind);
#else
    DIR           *pdir;
    struct dirent *pde;

    pdir = opendir(szDir);
    if (pdir == NULL)
        return;
    while ((pde = readdir(pdir)) != NULL) {
        if (strncmp(pde->d_name, "game.", 5) == 0) {
            snprintf(szFile, sizeof(szFile), "%s/%s", szDir, pde->d_name);
            remove(szFile);
        }
    }
    closedir(pdir);
#endif
}

int16_t FStarsTestDir(const char *pszName, char *szDir, size_t cchDir) {
    char szCwd[cbTestPath];

    if (PszStarsTestCwd(szCwd, sizeof(szCwd)) == NULL)
        return 0;
    snprintf(szDir, cchDir, "%s%swork", szCwd, szDirSep);
    MakeDir(szDir);
    snprintf(szDir, cchDir, "%s%swork%s%s", szCwd, szDirSep, szDirSep, pszName);
    MakeDir(szDir);
    DeleteGameFiles(szDir);
    return 1;
}

// FCopyRace copies the test race, which CMake places in data/, next to the game.
static int16_t FCopyRace(const char *szDir) {
    char   szDst[cbTestPath];
    char   szSrc[] = "data/humanoid.r1";
    char   rgb[4096];
    size_t cb;
    FILE  *pfSrc;
    FILE  *pfDst;

    snprintf(szDst, sizeof(szDst), "%s%shuman.r1", szDir, szDirSep);
    szSrc[4] = chDirSep;
    pfSrc = fopen(szSrc, "rb");
    if (pfSrc == NULL)
        return 0;
    pfDst = fopen(szDst, "wb");
    if (pfDst == NULL) {
        fclose(pfSrc);
        return 0;
    }
    while ((cb = fread(rgb, 1, sizeof(rgb), pfSrc)) > 0)
        fwrite(rgb, 1, cb, pfDst);
    fclose(pfSrc);
    return fclose(pfDst) == 0;
}

int16_t FStarsTestNewGame(APPSESSION &app, const char *szDir, uint32_t lSeed, const char **rgszAi, int16_t cAi) {
    char    szDef[cbTestPath];
    FILE   *fp;
    int16_t i;

    if (!FCopyRace(szDir))
        return 0;
    snprintf(szDef, sizeof(szDef), "%s%sgame.def", szDir, szDirSep);
    fp = fopen(szDef, "wb");
    if (fp == NULL)
        return 0;
    // Tiny universe, sparse, distant positions; all game options off.
    fprintf(fp, "Unit Test\r\n0 0 1 %lu\r\n0 0 0 0 0 1 0\r\n%d\r\n%s%shuman.r1\r\n", (unsigned long)lSeed, 1 + cAi, szDir, szDirSep);
    for (i = 0; i < cAi; i++)
        fprintf(fp, "%s\r\n", rgszAi[i]);
    // Victory conditions off.
    for (i = 0; i < 8; i++)
        fprintf(fp, "0\r\n");
    fprintf(fp, "%s%sgame.xy\r\n", szDir, szDirSep);
    fclose(fp);

    if (!GenNewGameFromFile(app, szDef))
        return 0;
    DestroyCurGame(app);
    snprintf(app.state.szBase, sizeof(app.state.szBase), "%s%sgame", szDir, szDirSep);
    return 1;
}

int16_t FStarsTestLoadHost(APPSESSION &app) {
    DestroyCurGame(app);
    app.state.idPlayer = iplrNone;
    return FLoadGame(app, app.state.szBase, szTestLiteral1);
}

int16_t FStarsTestLoadPlayer(APPSESSION &app, int16_t iPlr) {
    char szExt[16];

    DestroyCurGame(app);
    app.state.idPlayer = iPlr;
    snprintf(szExt, sizeof(szExt), "m%d", iPlr + 1);
    return FLoadGame(app, app.state.szBase, szExt);
}

PLANET *LpplStarsTestHomeworld(APPSESSION &app, int16_t iPlr) {
    int16_t i;

    for (i = 0; i < app.state.cPlanet; i++) {
        if (app.state.lpPlanets[i].iPlayer == iPlr && app.state.lpPlanets[i].fHomeworld)
            return &app.state.lpPlanets[i];
    }
    return NULL;
}

FLEET *LpflStarsTestAddFleet(APPSESSION &app, int16_t iPlr, int16_t idPlanet, int16_t ishdef, int16_t csh) {
    FLEET *lpfl;

    lpfl = LpflNew(app, iPlr, idPlanet);
    lpfl->rgcsh[ishdef] = csh;
    app.state.rglpshdef[iPlr][ishdef].cExist += csh;
    app.state.rglpshdef[iPlr][ishdef].cBuilt += csh;
    return lpfl;
}

int16_t FStarsTestSaveHost(APPSESSION &app) {
    app.state.idPlayer = iplrNone;
    return FWriteDataFile(app, app.state.szBase, -1, 0);
}

int16_t FStarsTestSaveGame(APPSESSION &app) {
    int16_t i;

    for (i = 0; i < app.state.game.cPlayer; i++) {
        app.state.idPlayer = iplrNone;
        if (!FWriteDataFile(app, app.state.szBase, i, 0))
            return 0;
    }
    return FStarsTestSaveHost(app);
}

int16_t FStarsTestGenerate(APPSESSION &app) {
    DestroyCurGame(app);
    app.state.idPlayer = iplrNone;
    EnsureAis(app);
    return FGenerateTurn(app);
}

#ifdef STARS_UI_SDL
static const ALERTIO io_alertio = [] { ALERTIO io; io.pfnAlertSz = StarsTest_AlertSz; return io; }();
static ALERTIOSCOPE scope_alertio(io_alertio);
#endif
