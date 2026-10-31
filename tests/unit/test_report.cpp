#include "acutest.h"

#include "stars_test.h"

// The planet dump passed the 32-bit mine and factory counts as two 16-bit
// words each, as the Win16 binary pushed them, so the Factories column
// printed 0 and the factory count shifted into Def %.
static void test_DumpPlanets_mines_and_factories(void) {
    char    szDir[MAX_PATH];
    char    szFile[MAX_PATH];
    char    szLine[1024];
    char   *rgpszField[16];
    char   *psz;
    char    szName[64];
    PLANET *lppl;
    FILE   *fp;
    int     cField;
    int     fFound;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("DumpPlanets_mines_and_factories", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadPlayer(0));
    lppl = LpplStarsTestHomeworld(0);
    TEST_ASSERT(lppl != NULL);
    TEST_ASSERT(lppl->cMines != lppl->cFactories || lppl->cMines != 0);
    strcpy(szName, PszGetCompressedPlanet(rgidPlan[lppl->id]));

    DumpPlanets();
    snprintf(szFile, sizeof(szFile), "%s.pla", szBase);
    fp = fopen(szFile, "rb");
    TEST_ASSERT_(fp != NULL, "open %s", szFile);
    fFound = FALSE;
    while (fgets(szLine, sizeof(szLine), fp)) {
        cField = 0;
        for (psz = strtok(szLine, "\t\r\n"); psz && cField < 16; psz = strtok(NULL, "\t\r\n"))
            rgpszField[cField++] = psz;
        if (cField < 10 || strcmp(rgpszField[0], szName) != 0)
            continue;
        fFound = TRUE;
        // Planet Name, Owner, Starbase Type, Report Age, Population, Value,
        // Production Queue, Mines, Factories, Def %.
        TEST_CHECK_(atol(rgpszField[7]) == lppl->cMines, "mines %s, expected %d", rgpszField[7], lppl->cMines);
        TEST_CHECK_(atol(rgpszField[8]) == lppl->cFactories, "factories %s, expected %d", rgpszField[8], lppl->cFactories);
        TEST_CHECK_(strchr(rgpszField[9], '%') != NULL, "def %s", rgpszField[9]);
    }
    fclose(fp);
    TEST_CHECK_(fFound, "no row for %s", szName);
}

// FSameAsGolden compares a dump with its expected copy in
// tests/unit/golden, or writes the copy when STARS_UPDATE_GOLDEN is set.
static int16_t FSameAsGolden(const char *szExt) {
    char   szFile[MAX_PATH];
    char   szGolden[MAX_PATH];
    char   rgb1[4096];
    char   rgb2[4096];
    size_t cb1;
    size_t cb2;
    FILE  *pf1;
    FILE  *pf2;
    int    fSame;

    snprintf(szFile, sizeof(szFile), "%s.%s", szBase, szExt);
    snprintf(szGolden, sizeof(szGolden), "%s/dumps.%s", STARS_TEST_GOLDEN_DIR, szExt);
    pf1 = fopen(szFile, "rb");
    if (pf1 == NULL)
        return FALSE;
    pf2 = fopen(szGolden, getenv("STARS_UPDATE_GOLDEN") ? "wb" : "rb");
    if (pf2 == NULL) {
        fclose(pf1);
        return FALSE;
    }
    fSame = TRUE;
    if (getenv("STARS_UPDATE_GOLDEN")) {
        while ((cb1 = fread(rgb1, 1, sizeof(rgb1), pf1)) > 0)
            fwrite(rgb1, 1, cb1, pf2);
    } else {
        do {
            cb1 = fread(rgb1, 1, sizeof(rgb1), pf1);
            cb2 = fread(rgb2, 1, sizeof(rgb2), pf2);
            if (cb1 != cb2 || memcmp(rgb1, rgb2, cb1) != 0)
                fSame = FALSE;
        } while (fSame && cb1 > 0);
    }
    fclose(pf1);
    fclose(pf2);
    return fSame;
}

// The universe, planet and fleet dumps (-dm, -dp, -df) are what TotalHost
// reads for its movies, and stars-host writes them off Windows. They must
// match the Windows build's, kept in tests/unit/golden, in both the
// original layout and the per-player one ([Misc] NewReports=1).
static void test_Dumps_match_golden_files(void) {
    char        szDir[MAX_PATH];
    const char *rgszAi[] = {"#1 4"};
    int16_t     fPerPlayer;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("Dumps_match_golden_files", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestGenerate());
    TEST_ASSERT(FStarsTestGenerate());
    TEST_ASSERT(FStarsTestLoadPlayer(0));
    for (fPerPlayer = FALSE; fPerPlayer <= TRUE; fPerPlayer++) {
        gd.fPerPlayerDumps = fPerPlayer;
        DumpUniverse();
        DumpPlanets();
        DumpFleets();
    }
    TEST_CHECK_(FSameAsGolden("map"), "game.map differs from tests/unit/golden/dumps.map");
    TEST_CHECK_(FSameAsGolden("pla"), "game.pla differs from tests/unit/golden/dumps.pla");
    TEST_CHECK_(FSameAsGolden("fle"), "game.fle differs from tests/unit/golden/dumps.fle");
    TEST_CHECK_(FSameAsGolden("p1"), "game.p1 differs from tests/unit/golden/dumps.p1");
    TEST_CHECK_(FSameAsGolden("f1"), "game.f1 differs from tests/unit/golden/dumps.f1");
}

TEST_LIST = {
    {"DumpPlanets mines and factories", test_DumpPlanets_mines_and_factories}, {"Dumps match golden files", test_Dumps_match_golden_files}, {NULL, NULL}};
