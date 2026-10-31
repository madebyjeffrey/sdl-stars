#include "acutest.h"

#include "stars_test.h"

#ifdef STARS_UI_SDL
// The battle plans dialog needs the Windows or SDL client UI.
// OpenBattlePlans loads player 1's turn file and opens the real Battle Plans
// dialog on its first plan.
static HWND OpenBattlePlans(const char *pszTest) {
    char szDir[cbTestPath];
    HWND hwnd;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir(pszTest, szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, NULL, 0));
    TEST_ASSERT(FStarsTestLoadPlayer(0));
    hwnd = ui::CreateDialog(hInst, win32::MAKEINTRESOURCE(IDD_BATTLE_PLANS), NULL, BattlePlansDlg, 0);
    TEST_ASSERT(hwnd != NULL);
    return hwnd;
}

// Replay uses aligned runtime tokens while its borrowed records retain the
// odd 29-byte wire stride. Exercise attack, round transition, seek and summary.
static void test_battle_replay_bytes(void) {
    char szDir[cbTestPath];
    const char *rgszAi[] = {"#1 4"};
    uint8_t *lpb;
    TOK rgtok[2]{};
    int32_t rgdp[2]{};
    int16_t ish, i;
    int32_t dpArmor, dpShields;
    int16_t csh;
    DV dv;
    BTLREC br{};
    KILL kill{};

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("battle_replay_bytes", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(szDir, 12345, rgszAi, 1));
    TEST_ASSERT(FStarsTestLoadHost());
    idPlayer = 0;
    lpb = (uint8_t *)LpAlloc(0xffc8, htBattle);
    memset(lpb, 0, 96);
    BdvFromLp(lpb).SetId(4); BdvFromLp(lpb).SetCplr(2); BdvFromLp(lpb).SetCtok(2);
    BdvFromLp(lpb).SetGrfPlr(3); BdvFromLp(lpb).SetCbData(92); BdvFromLp(lpb).SetIdPlanet(idPlanetNone);
    for (i = 0; i < 2; i++) {
        for (ish = 0; ish < 16 && rglpshdef[i][ish].fFree; ish++) {}
        TEST_ASSERT(ish < 16);
        rgtok[i].iplr = i; rgtok[i].ishdef = ish; rgtok[i].grobj = grobjFleet;
        rgtok[i].csh = i == 0 ? 10 : 8; rgtok[i].dpShield = 20; rgtok[i].brc = 0x12 + i;
        rglpshdef[i][ish].hul.dp = 100;
        TEST_ASSERT(FEncodeTok(rgtok[i], std::as_writable_bytes(std::span(lpb + 14 + i * 29, 29))));
    }
    br.itok = 0; br.brcDest = 0x34; br.ctok = 1; br.itokAttack = 1;
    TEST_ASSERT(FEncodeBattleRecord(br, std::as_writable_bytes(std::span(lpb + 72, 6))));
    kill.itok = 0; kill.cshKill = 2; kill.dpShield = 16; kill.dv.SetPctSh(50); kill.dv.SetPctDp(100);
    TEST_ASSERT(FEncodeKill(kill, std::as_writable_bytes(std::span(lpb + 78, 8))));
    br.itok = 1; br.brcDest = 0x56; br.ctok = 0; br.iRound = 1;
    TEST_ASSERT(FEncodeBattleRecord(br, std::as_writable_bytes(std::span(lpb + 86, 6))));
    lpb[92] = lpb[93] = 0xff;
    TEST_CHECK(BtlDataGet(0) == lpb && BtlDataGet(1) == NULL);
    TEST_CHECK(CBattleUnits(lpb, static_cast<BattleUnitFlags>(grBuOurUnits | grBuClassAll)) == 10);
    TEST_CHECK(CBattleUnits(lpb, static_cast<BattleUnitFlags>(grBuTheirUnits | grBuClassAll)) == 8);
    TEST_CHECK(CBattleKills(lpb, 1) == 2 && CBattleKills(lpb, 0) == 0);
    vrgtok = rgtok; vrgdpVCR = rgdp;
    vlpbdVCR = lpb; vlpbdVCRNext = lpb + 92;
    viStepVCRCur = -1; vcStepVCR = 1;
    TEST_CHECK(SetVCRBoard(-1) == -1 && rgdp[0] == 1000 && rgtok[0].brc == 0x12);
    TEST_CHECK(SetVCRBoard(0) == 0 && rgtok[0].brc == 0x34 && viVCRFocus == 0);
    GetVCRStats(0, &dpArmor, &dv, &dpShields, &csh);
    TEST_CHECK(dpArmor == 720 && dpShields == 16 && csh == 8 && dv.dp == kill.dv.dp);
    TEST_CHECK(SetVCRBoard(1) == 1 && viRound == 1 && vlpbrVCR == lpb + 86);
    TEST_CHECK(rgtok[0].csh == 8 && rgtok[0].dpShield == 18 && rgdp[0] == 720);
    TEST_CHECK(rgtok[1].brc == 0x56 && viVCRFocus == 1);
    TEST_CHECK(SetVCRBoard(2) == 2 && vlpbrVCR == vlpbdVCRNext);
    TEST_CHECK(SetVCRBoard(-1) == -1 && rgtok[0].csh == 10 && rgtok[0].dpShield == 20 && rgdp[0] == 1000);
    vrgtok = NULL; vrgdpVCR = NULL; vlpbdVCR = vlpbdVCRNext = vlpbrVCR = NULL;
    FreeHb(rglphb[htBattle]); rglphb[htBattle] = NULL;
}

// ChooseIn selects entry i of drop-down idc and notifies the dialog, as a
// player's choice does.
static void ChooseIn(HWND hwnd, int idc, int i) {
    HWND hwndCtl;

    hwndCtl = win32::GetDlgItem(hwnd, idc);
    TEST_ASSERT(hwndCtl != NULL);
    TEST_ASSERT(ui::SendMessage(hwndCtl, win32::CB_SETCURSEL, i, 0) == i);
    ui::SendMessage(hwnd, win32::WM_COMMAND, win32::MAKEWPARAM(idc, win32::CBN_SELCHANGE), (LPARAM)hwndCtl);
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
    ui::DestroyWindow(hwnd);
}
#endif

// Starbase friendly fire: in Win16, a starbase whose battle plan attacked
// everyone or one player set the attack mask of whichever player iplrCur
// last held, not its own. Check, for each way to cast three players as
// the starbase owner, its friend and its target, that the starbase attacks
// only the target and nobody attacks the friend or is attacked by it.
static void test_CplrBattle_starbase_attacks_only_its_target(void) {
    char                 szDir[cbTestPath];
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
        LinkFleets(0);
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
             {"battle replay reads exact wire bytes", test_battle_replay_bytes},
             {"BattlePlansDlg stores drop-down choices", test_BattlePlansDlg_stores_choices},
#endif
             {NULL, NULL}};
