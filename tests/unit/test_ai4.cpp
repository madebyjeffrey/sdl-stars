#include "acutest.h"

#include "stars_test.h"

#define iplrCyber   1
#define ishSBDefend 14

// CshDesign counts iPlr's ships of design ishdef in the loaded game.
static int32_t CshDesign(int16_t iPlr, int16_t ishdef) {
    int32_t csh;
    int16_t ifl;

    csh = 0;
    for (ifl = 0; ifl < cFleet && rglpfl[ifl]; ifl++) {
        if (rglpfl[ifl]->iPlayer == iPlr && !rglpfl[ifl]->fDead)
            csh += rglpfl[ifl]->rgcsh[ishdef];
    }
    return csh;
}

// The Cybertron AI keeps its newest starbase-defender design. The original
// tested iLatestDestroyer after the defender CheckAiShdefStatus call, so an
// old but newest defender design was marked for recycling and its fleets at
// a starbase were scrapped.
static void test_cyber_keeps_latest_sb_defender(void) {
    char        szDir[MAX_PATH];
    const char *rgszAi[] = {"#5 4"};
    PLANET     *lppl;
    SHDEF      *lpshdef;
    int16_t     ish;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("cyber_keeps_latest_sb_defender", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost());

    lppl = LpplStarsTestHomeworld(iplrCyber);
    TEST_ASSERT(lppl != NULL);
    TEST_ASSERT(lppl->fStarbase);

    // Give the Cybertron one starbase-defender design, older than its
    // recycle period, and one ship of it at its homeworld starbase.
    for (ish = 0; ish < ishSBDefend && rglpshdef[iplrCyber][ish].fFree; ish++) {
    }
    TEST_ASSERT(ish < ishSBDefend);
    lpshdef = &rglpshdef[iplrCyber][ishSBDefend];
    *lpshdef = rglpshdef[iplrCyber][ish];
    lpshdef->ishdef = ishSBDefend;
    lpshdef->turn = (uint16_t)(game.turn - 60);
    lpshdef->cExist = 0;
    lpshdef->cBuilt = 0;
    rglpshdef[iplrCyber][ishSBDefend + 1].fFree = TRUE;
    LpflStarsTestAddFleet(iplrCyber, lppl->id, ishSBDefend, 1);
    TEST_ASSERT(FStarsTestSaveHost());

    // The AI gives its orders while one turn generates; the next carries them out.
    TEST_ASSERT(FStarsTestGenerate());
    TEST_ASSERT(FStarsTestGenerate());
    TEST_ASSERT(FStarsTestLoadHost());
    TEST_CHECK_(CshDesign(iplrCyber, ishSBDefend) == 1, "defenders %ld", (long)CshDesign(iplrCyber, ishSBDefend));
}

TEST_LIST = {{"Cybertron keeps its latest starbase defender", test_cyber_keeps_latest_sb_defender}, {NULL, NULL}};
