// stars_scenario builds small games to open in Stars! by hand, for checks
// the unit tests can't make, such as a popup menu waiting for a click.
//
//   stars_scenario.exe                  list the scenarios
//   stars_scenario.exe <name> <dir>     build one into <dir> (game.xy, .hst, .mN)
//
// Run it from the build's tests directory (it needs data\humanoid.r1); the
// Makefile's scenario target does this. A Unix path for <dir> is taken as
// Wine's Z: drive. Add a scenario by writing a builder below and listing it
// in rgscen: start from FStarsTestNewGame, change the loaded host game with
// the stars_test.h helpers, and finish with FStarsTestSaveGame.

#include "stars_test.h"

#include <direct.h>

typedef struct {
    const char *pszName;
    const char *pszDesc;
    int16_t (*pfnBuild)(const char *szDir);
} SCENARIO;

// IshFirstDesign returns iPlr's first ship design, or -1.
static int16_t IshFirstDesign(int16_t iPlr) {
    int16_t ish;

    for (ish = 0; ish < 16 && rglpshdef[iPlr][ish].fFree; ish++) {
    }
    return ish < 16 ? ish : -1;
}

static int16_t FBuildNewGame(const char *szDir) {
    return FStarsTestNewGame(szDir, 12345, NULL, 0) && FStarsTestLoadHost() && FStarsTestSaveGame();
}

// 150 one-ship fleets orbit the homeworld. Right-click the homeworld in the
// scanner: the popup lists the planet and all 150 fleets, and choosing one
// past the hundredth selects it. Select a fleet and right-click its first
// waypoint's target in the fleet orders: the other 149 are listed.
static int16_t FBuildPopupOverload(const char *szDir) {
    PLANET *lppl;
    int16_t ish;
    int16_t i;

    if (!FStarsTestNewGame(szDir, 12345, NULL, 0) || !FStarsTestLoadHost())
        return FALSE;
    lppl = LpplStarsTestHomeworld(0);
    ish = IshFirstDesign(0);
    if (lppl == NULL || ish < 0)
        return FALSE;
    for (i = 0; i < 150; i++) {
        LpflStarsTestAddFleet(0, lppl->id, ish, 1);
    }
    return FStarsTestSaveGame();
}

static const SCENARIO rgscen[] = {
    {"new-game", "a new one-player tiny universe, unchanged", FBuildNewGame},
    {"popup-overload", "150 fleets at the homeworld, for the target popups", FBuildPopupOverload},
};

int main(int argc, char **argv) {
    char    szDir[MAX_PATH];
    char   *pch;
    size_t  i;
    int16_t iscen;

    if (argc != 3) {
        printf("usage: stars_scenario <name> <dir>\n\nScenarios:\n");
        for (i = 0; i < sizeof(rgscen) / sizeof(rgscen[0]); i++) {
            printf("  %-16s %s\n", rgscen[i].pszName, rgscen[i].pszDesc);
        }
        return argc == 1 ? 0 : 2;
    }
    for (iscen = 0; iscen < (int16_t)(sizeof(rgscen) / sizeof(rgscen[0])) && strcmp(argv[1], rgscen[iscen].pszName) != 0; iscen++) {
    }
    if (iscen == (int16_t)(sizeof(rgscen) / sizeof(rgscen[0]))) {
        fprintf(stderr, "unknown scenario %s\n", argv[1]);
        return 2;
    }
    // Wine maps the Unix root to Z:.
    snprintf(szDir, sizeof(szDir), "%s%s", argv[2][0] == '/' ? "Z:" : "", argv[2]);
    for (pch = szDir; *pch != 0; pch++) {
        if (*pch == '/')
            *pch = '\\';
    }
    _mkdir(szDir);
    if (!FStarsTestInit() || !rgscen[iscen].pfnBuild(szDir)) {
        fprintf(stderr, "%s failed%s%s\n", argv[1], cStarsTestAlert ? ": " : "", cStarsTestAlert ? szStarsTestAlert : "");
        return 1;
    }
    DestroyCurGame();
    printf("%s: %s\\game.xy, game.hst and game.m1\n", argv[1], szDir);
    return 0;
}
