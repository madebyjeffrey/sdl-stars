#include "acutest.h"

#include "stars_test.h"

#ifdef STARS_UI_SDL
// The battle plans dialog needs the Windows or SDL client UI.
// OpenBattlePlans loads player 1's turn file and opens the real Battle Plans
// dialog on its first plan.
static HWND OpenBattlePlans(const char *pszTest) {
    char szDir[MAX_PATH];
    HWND hwnd;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir(pszTest, szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadPlayer(0));
    hwnd = CreateDialogParamA(hInst, MAKEINTRESOURCE(IDD_BATTLE_PLANS), NULL, BattlePlansDlg, 0);
    TEST_ASSERT(hwnd != NULL);
    return hwnd;
}

// ChooseIn selects entry i of drop-down idc and notifies the dialog, as a
// player's choice does.
static void ChooseIn(HWND hwnd, int idc, int i) {
    HWND hwndCtl;

    hwndCtl = GetDlgItem(hwnd, idc);
    TEST_ASSERT(hwndCtl != NULL);
    TEST_ASSERT(SendMessage(hwndCtl, CB_SETCURSEL, i, 0) == i);
    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(idc, CBN_SELCHANGE), (LPARAM)hwndCtl);
}

// The Battle Plans drop-downs read their selection with the Win16 message
// number for CB_GETCURSEL, which Win32 combo boxes ignore, so every choice
// was stored as 0.
static void test_BattlePlansDlg_stores_choices(void) {
    HWND hwnd;

    hwnd = OpenBattlePlans("BattlePlansDlg_stores_choices");
    ChooseIn(hwnd, IDC_BATTLE_PLAN_PRIMARY_TARGET, 2);
    TEST_CHECK_(btlplan.mdTarget1 == 2, "primary target %d", btlplan.mdTarget1);
    ChooseIn(hwnd, IDC_BATTLE_PLAN_SECONDARY_TARGET, 3);
    TEST_CHECK_(btlplan.mdTarget2 == 3, "secondary target %d", btlplan.mdTarget2);
    ChooseIn(hwnd, IDC_BATTLE_PLAN_TACTIC, 1);
    TEST_CHECK_(btlplan.mdTactic == 1, "tactic %d", btlplan.mdTactic);
    DestroyWindow(hwnd);
}
#endif

// Starbase friendly fire: in Win16, a starbase whose battle plan attacked
// everyone or one player set the attack mask of whichever player iplrCur
// last held, not its own. Check, for each way to cast three players as
// the starbase owner, its friend and its target, that the starbase attacks
// only the target and nobody attacks the friend or is attacked by it.
static void test_CplrBattle_starbase_attacks_only_its_target(void) {
    char                 szDir[MAX_PATH];
    const char          *rgszAi[] = {"#1 4", "#2 4"};
    static const int16_t rgrgiplr[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    PLANET              *lppl;
    FLEET               *lpfl;
    uint16_t             rggrfAttack[16];
    uint16_t             grfPlayer;
    uint16_t             grfSpectator;
    int16_t              icase;
    int16_t              iplrSB;
    int16_t              iplrFriend;
    int16_t              iplrTarget;
    int16_t              i;
    int16_t              ish;

    for (icase = 0; icase < 6; icase++) {
        iplrSB = rgrgiplr[icase][0];
        iplrFriend = rgrgiplr[icase][1];
        iplrTarget = rgrgiplr[icase][2];
        TEST_CASE_("starbase %d, friend %d, target %d", iplrSB, iplrFriend, iplrTarget);
        TEST_ASSERT(FStarsTestInit());
        TEST_ASSERT(FStarsTestDir("CplrBattle_starbase_target", szDir, sizeof(szDir)));
        TEST_ASSERT(FStarsTestNewGame(szDir, 12345, rgszAi, 2));
        TEST_ASSERT(FStarsTestLoadHost());
        lppl = LpplStarsTestHomeworld(iplrSB);
        TEST_ASSERT(lppl != NULL && lppl->fStarbase);
        TEST_ASSERT(FHullHasTeeth(&rglpshdefSB[iplrSB][lppl->isb].hul));
        for (i = 0; i < 3; i++) {
            rglpbtlplan[i][0].iplrAttack = iplrAttackNobody;
        }
        rglpbtlplan[iplrSB][0].iplrAttack = 4 + iplrTarget;
        rgplr[iplrSB].rgmdRelation[iplrFriend] = 1;
        rgplr[iplrFriend].rgmdRelation[iplrSB] = 1;
        for (i = 0; i < 3; i++) {
            if (i == iplrSB)
                continue;
            for (ish = 0; ish < 16 && rglpshdef[i][ish].fFree; ish++) {
            }
            TEST_ASSERT(ish < 16);
            lpfl = LpflStarsTestAddFleet(i, lppl->id, ish, 1);
        }
        LinkFleets(FALSE);
        lpfl = LpflFromId(lpfl->id);
        CplrBattle(lpfl, rggrfAttack, &grfPlayer, &grfSpectator);
        TEST_CHECK_(rggrfAttack[iplrSB] == 1 << iplrTarget, "the starbase attacks %#x", rggrfAttack[iplrSB]);
        for (i = 0; i < 3; i++) {
            TEST_CHECK_(!(rggrfAttack[i] & (1 << iplrFriend)), "player %d attacks the friend", i);
        }
        TEST_CHECK_(!(rggrfAttack[iplrFriend] & (1 << iplrSB)), "the friend attacks the starbase");
    }
}

TEST_LIST = {{"CplrBattle starbase attacks only its target", test_CplrBattle_starbase_attacks_only_its_target},
#ifdef STARS_UI_SDL
             {"BattlePlansDlg stores drop-down choices", test_BattlePlansDlg_stores_choices},
#endif
             {NULL, NULL}};
