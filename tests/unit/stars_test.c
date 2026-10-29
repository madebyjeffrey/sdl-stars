#include "stars_test.h"
[[maybe_unused]] static char szTestLiteral1[] = "hst";

#ifdef _WIN32
#include <io.h>
#else
#include <dirent.h>
#endif

int  cStarsTestAlert;
char szStarsTestAlert[512];
int16_t iStarsTestAlertResponse;
int16_t fStarsTestAcceptLegacyRacePassword;
int16_t cStarsTestLegacyRacePasswordPrompt;
char szStarsTestLegacyRacePassword[16];

int16_t FStarsTestPromptLegacyRacePassword(char *szPassword) {
    cStarsTestLegacyRacePasswordPrompt++;
    if (!fStarsTestAcceptLegacyRacePassword)
        return FALSE;
    strcpy(szPassword, szStarsTestLegacyRacePassword);
    return TRUE;
}

// FStarsTestAlert records a message box and gives the answer that lets the
// game carry on.
static int16_t FStarsTestAlert(const char *sz, int16_t mbType) {
    cStarsTestAlert++;
    snprintf(szStarsTestAlert, sizeof(szStarsTestAlert), "%s", sz);
    if (((mbType & 0x000f) == MB_YESNO || (mbType & 0x000f) == MB_YESNOCANCEL) && iStarsTestAlertResponse != 0)
        return iStarsTestAlertResponse;
    return (mbType & 0x000f) == MB_YESNO || (mbType & 0x000f) == MB_YESNOCANCEL ? IDYES : IDOK;
}

#ifdef STARS_UI_SDL
int16_t StarsTest_AlertSz(const char *sz, int16_t mbType) { return FStarsTestAlert(sz, mbType); }
int16_t StarsTest_FPromptLegacyRacePassword(char *szPassword) {
    return FStarsTestPromptLegacyRacePassword(szPassword);
}
#else
// The native tests link hostui.c for the rest of ui.h.
int16_t IdAlertBox(const char *sz, int16_t mbType) { return FStarsTestAlert(sz, mbType); }
#endif

int16_t FStarsTestInit(void) {
    iStarsTestAlertResponse = 0;
    fStarsTestAcceptLegacyRacePassword = FALSE;
    cStarsTestLegacyRacePasswordPrompt = 0;
    szStarsTestLegacyRacePassword[0] = 0;
#ifdef STARS_UI_SDL
    hInst = GetModuleHandle(NULL);
    memset(&vtimer, 0, sizeof(TIMER));
#endif
    szBase[0] = 0;
    ini.wFlags = 0;
    memset(&tutor, 0, sizeof(TUTOR));
    Randomize2(12345);
    fFileErrSilent = TRUE;
    idPlayer = iplrNone;
#ifdef STARS_UI_SDL
    return FCreateStuff();
#else
    InitGameStuff();
    return TRUE;
#endif
}

// DeleteGameFiles removes szDir's game.* files from an earlier run.
static void DeleteGameFiles(const char *szDir) {
    char          szFile[MAX_PATH];
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
    char szCwd[MAX_PATH];

    if (getcwd(szCwd, sizeof(szCwd)) == NULL)
        return FALSE;
    snprintf(szDir, cchDir, "%s%swork", szCwd, szDirSep);
    MakeDir(szDir);
    snprintf(szDir, cchDir, "%s%swork%s%s", szCwd, szDirSep, szDirSep, pszName);
    MakeDir(szDir);
    DeleteGameFiles(szDir);
    return TRUE;
}

// FCopyRace copies the test race, which CMake places in data/, next to the game.
static int16_t FCopyRace(const char *szDir) {
    char   szDst[MAX_PATH];
    char   rgb[4096];
    size_t cb;
    FILE  *pfSrc;
    FILE  *pfDst;

    snprintf(szDst, sizeof(szDst), "%s%shuman.r1", szDir, szDirSep);
    pfSrc = fopen("data" szDirSep "humanoid.r1", "rb");
    if (pfSrc == NULL)
        return FALSE;
    pfDst = fopen(szDst, "wb");
    if (pfDst == NULL) {
        fclose(pfSrc);
        return FALSE;
    }
    while ((cb = fread(rgb, 1, sizeof(rgb), pfSrc)) > 0)
        fwrite(rgb, 1, cb, pfDst);
    fclose(pfSrc);
    return fclose(pfDst) == 0;
}

int16_t FStarsTestNewGame(const char *szDir, uint32_t lSeed, const char **rgszAi, int16_t cAi) {
    char    szDef[MAX_PATH];
    FILE   *fp;
    int16_t i;

    if (!FCopyRace(szDir))
        return FALSE;
    snprintf(szDef, sizeof(szDef), "%s%sgame.def", szDir, szDirSep);
    fp = fopen(szDef, "wb");
    if (fp == NULL)
        return FALSE;
    // Tiny universe, sparse, distant positions; all game options off.
    fprintf(fp, "Unit Test\r\n0 0 1 %lu\r\n0 0 0 0 0 1 0\r\n%d\r\n%s%shuman.r1\r\n", (unsigned long)lSeed, 1 + cAi, szDir, szDirSep);
    for (i = 0; i < cAi; i++)
        fprintf(fp, "%s\r\n", rgszAi[i]);
    // Victory conditions off.
    for (i = 0; i < 8; i++)
        fprintf(fp, "0\r\n");
    fprintf(fp, "%s%sgame.xy\r\n", szDir, szDirSep);
    fclose(fp);

    if (!GenNewGameFromFile(szDef))
        return FALSE;
    DestroyCurGame();
    snprintf(szBase, sizeof(szBase), "%s%sgame", szDir, szDirSep);
    return TRUE;
}

int16_t FStarsTestLoadHost(void) {
    DestroyCurGame();
    idPlayer = iplrNone;
    return FLoadGame(szBase, szTestLiteral1);
}

int16_t FStarsTestLoadPlayer(int16_t iPlr) {
    char szExt[16];

    DestroyCurGame();
    idPlayer = iPlr;
    snprintf(szExt, sizeof(szExt), "m%d", iPlr + 1);
    return FLoadGame(szBase, szExt);
}

PLANET *LpplStarsTestHomeworld(int16_t iPlr) {
    int16_t i;

    for (i = 0; i < cPlanet; i++) {
        if (lpPlanets[i].iPlayer == iPlr && lpPlanets[i].fHomeworld)
            return &lpPlanets[i];
    }
    return NULL;
}

FLEET *LpflStarsTestAddFleet(int16_t iPlr, int16_t idPlanet, int16_t ishdef, int16_t csh) {
    FLEET *lpfl;

    lpfl = LpflNew(iPlr, idPlanet);
    lpfl->rgcsh[ishdef] = csh;
    rglpshdef[iPlr][ishdef].cExist += csh;
    rglpshdef[iPlr][ishdef].cBuilt += csh;
    return lpfl;
}

int16_t FStarsTestSaveHost(void) {
    idPlayer = iplrNone;
    return FWriteDataFile(szBase, -1, FALSE);
}

int16_t FStarsTestSaveGame(void) {
    int16_t i;

    for (i = 0; i < game.cPlayer; i++) {
        idPlayer = iplrNone;
        if (!FWriteDataFile(szBase, i, FALSE))
            return FALSE;
    }
    return FStarsTestSaveHost();
}

int16_t FStarsTestGenerate(void) {
    DestroyCurGame();
    idPlayer = iplrNone;
    EnsureAis();
    return FGenerateTurn();
}
