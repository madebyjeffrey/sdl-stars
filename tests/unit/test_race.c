#include "acutest.h"

#include "stars_test.h"

static void test_LInnateRaceHabitability_double_rounding(void) {
    PLAYER plr = {0};
    int8_t rgAttr[16] = {10, 10, 10, 10, 10, 5, 10, 0, 1, 1, 1, 1, 1, 1, 9, 0};
    int8_t rgLo[3] = {31, 61, 9};
    int8_t rgHi[3] = {71, 97, 73};
    int i;

    TEST_ASSERT(FStarsTestInit());
    memcpy(plr.rgAttr, rgAttr, sizeof(rgAttr));
    plr.pctIdealGrowth = 10;
    for (i = 0; i < 3; i++) {
        plr.rgEnvVarMin[i] = rgLo[i];
        plr.rgEnvVarMax[i] = rgHi[i];
        plr.rgEnvVar[i] = (rgLo[i] + rgHi[i]) / 2;
    }
    // Extended intermediates returned 1470286; binary64 crosses this boundary.
    TEST_CHECK(LInnateRaceHabitability(&plr) == 1470287);
    TEST_CHECK(CAdvantagePoints(&plr) == 541);
}

static void test_RaceGrbit_high_bit(void) {
    PLAYER plr = {0};

    SetRaceGrbit(&plr, 0, TRUE);
    SetRaceGrbit(&plr, 31, TRUE);
    TEST_CHECK(plr.grbitAttr == UINT32_C(0x80000001));
    TEST_CHECK(GetRaceGrbit(&plr, 31));
    SetRaceGrbit(&plr, 31, FALSE);
    TEST_CHECK(plr.grbitAttr == 1);
    TEST_CHECK(!GetRaceGrbit(&plr, 31));
    TEST_CHECK(GetRaceGrbit(&plr, 0));
}

// Race file corruption: the race wizard reads a name into a buffer that still
// holds a longer one, and IRaceChecksum included the stale bytes after the
// terminator. They aren't saved, so the file failed its checksum on load.
static void test_IRaceChecksum_ignores_stale_name_bytes(void) {
    char     szDir[MAX_PATH];
    char     szFile[MAX_PATH];
    PLAYER   plr;
    uint16_t icksum;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("IRaceChecksum_stale_name", szDir, sizeof(szDir)));
    TEST_ASSERT(FWasRaceFile("data" szDirSep "humanoid.r1", FALSE));
    plr = vplr;
    strcpy(plr.szName, "Longname");
    strcpy(plr.szNames, "Longnames");
    // A shorter name typed over them, as GetDlgItemText leaves the buffers.
    strcpy(plr.szName, "Bo");
    strcpy(plr.szNames, "Bos");

    // Save as FSaveRace does.
    snprintf(szFile, sizeof(szFile), "%s%sshort.r1", szDir, szDirSep);
    TEST_ASSERT(FCreateFile(dtRace, iplrNone, szFile));
    WriteRtPlr(&plr, NULL);
    icksum = IRaceChecksum(&plr);
    WriteRt(rtEOF, 2, &icksum);
    StreamClose();

    TEST_CHECK(FWasRaceFile(szFile, FALSE));
    TEST_CHECK(strcmp(vplr.szNames, "Bos") == 0);
}

// ReadRtPlr took the relation count and uncompressed name lengths from the
// record, so a corrupt one overran rgmdRelation into szName and the names
// into the rest of the PLAYER.
static void test_ReadRtPlr_bounds_record_lengths(void) {
    struct {
        PLAYER plr;
        char   rgchGuard[64];
    } buf;
    uint8_t rgb[300];
    char    szLong[33];
    int16_t iOff;
    int16_t cOut;
    int     i;

    // 40 relations, an uncompressed "Bob" and a 60-character plural name.
    memset(rgb, 0, sizeof(rgb));
    ((PLAYER *)rgb)->det = detAll;
    iOff = offsetof(PLAYER, rgmdRelation);
    rgb[iOff++] = 40;
    memset(&rgb[iOff], 1, 40);
    iOff += 40;
    iOff++;
    strcpy((char *)&rgb[iOff], "Bob");
    iOff += 4;
    memset(&rgb[iOff + 1], 'N', 60);
    wVersFile = 0;
    ((VERS *)&wVersFile)->verMajor = 2;
    ((VERS *)&wVersFile)->verMinor = 85;

    memset(&buf, 0x55, sizeof(buf));
    ReadRtPlr(&buf.plr, rgb);
    for (i = 0; i < 16; i++)
        TEST_CHECK(buf.plr.rgmdRelation[i] == 1);
    TEST_CHECK(strcmp(buf.plr.szName, "Bob") == 0);
    for (i = 4; i < (int)sizeof(buf.plr.szName); i++)
        TEST_CHECK(buf.plr.szName[i] == 0);
    TEST_CHECK(buf.plr.szNames[0] == 0);
    TEST_CHECK(idsFileError == idsGameFileAppearsCorruptUnableLoadFile);
    for (i = 0; i < (int)sizeof(buf.rgchGuard); i++)
        TEST_CHECK(buf.rgchGuard[i] == 0x55);

    // A compressed plural name of 32 characters, one more than a save holds.
    memset(szLong, 'a', 32);
    szLong[32] = 0;
    cOut = 64;
    TEST_ASSERT(FCompressUserString(szLong, (char *)&rgb[iOff + 1], &cOut));
    rgb[iOff] = (uint8_t)cOut;
    memset(&buf, 0x55, sizeof(buf));
    ReadRtPlr(&buf.plr, rgb);
    TEST_CHECK(buf.plr.szNames[0] == 0);
    for (i = 0; i < (int)sizeof(buf.rgchGuard); i++)
        TEST_CHECK(buf.rgchGuard[i] == 0x55);
}

#ifdef STARS_UI_SDL
// Random race: the Random template is only the random-race bit, which
// GenerateWorld replaces with a random race. Choosing Custom on the first
// wizard page keeps the race being edited, so a race that had been Random
// kept the bit and was still replaced, however it was customized.
static void test_RaceWizardDlg1_custom_clears_random(void) {
    HWND hwnd;
    HWND hwndCtl;

    TEST_ASSERT(FStarsTestInit());
    vplr = vrgplrDef[6];
    TEST_ASSERT(GetRaceGrbit(&vplr, ibitRaceAIPlayer) != 0);
    strcpy(vplr.szName, "Randomite");
    strcpy(vplr.szNames, "Randomites");
    hwnd = CreateDialogParamA(hInst, MAKEINTRESOURCE(IDD_RACE_WIZARD_1), NULL, RaceWizardDlg1, 0);
    TEST_ASSERT(hwnd != NULL);
    CheckRadioButton(hwnd, IDC_RACE_HUMANOID, IDC_RACE_CUSTOM, IDC_RACE_CUSTOM);
    hwndCtl = GetDlgItem(hwnd, IDC_RACE_CUSTOM);
    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_RACE_CUSTOM, BN_CLICKED), (LPARAM)hwndCtl);
    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_NEXT, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, IDC_NEXT));
    DestroyWindow(hwnd);
    TEST_CHECK_(GetRaceGrbit(&vplr, ibitRaceAIPlayer) == 0, "the custom race is still random");
}

static void test_RaceWizardDlg2_paints_habitat(void) {
    RECT rgrc[17];
    HWND hwnd;
    HDC hdc;
    int16_t i;

    TEST_ASSERT(FStarsTestInit());
    hdc = GetDC(NULL);
    TEST_ASSERT(hdc != NULL);
    TEST_ASSERT(FCreateFonts(hdc));
    ReleaseDC(NULL, hdc);
    vplr = vrgplrDef[0];
    vrgrcRCW = rgrc;
    iPanelActive = rwPageHabitability;
    fRCWReadOnly = FALSE;
    hwnd = CreateDialogParamA(hInst, MAKEINTRESOURCE(IDD_RACE_WIZARD_2), NULL, RaceWizardDlg2, 0);
    TEST_ASSERT(hwnd != NULL);
    TEST_CHECK(crcRCW == 17);
    ShowWindow(hwnd, SW_SHOW);
    TEST_CHECK(UpdateWindow(hwnd));
    for (i = 0; i < 3; i++) {
        CheckDlgButton(hwnd, IDC_IMMUNE_TO_GRAVITY + i, BST_CHECKED);
        SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_IMMUNE_TO_GRAVITY + i, BN_CLICKED),
                    (LPARAM)GetDlgItem(hwnd, IDC_IMMUNE_TO_GRAVITY + i));
        TEST_CHECK(vplr.rgEnvVarMax[i] == envImmune);
    }
    DrawRace2(hwnd, NULL, -1);
    for (i = 0; i < 3; i++) {
        CheckDlgButton(hwnd, IDC_IMMUNE_TO_GRAVITY + i, BST_UNCHECKED);
        SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_IMMUNE_TO_GRAVITY + i, BN_CLICKED),
                    (LPARAM)GetDlgItem(hwnd, IDC_IMMUNE_TO_GRAVITY + i));
        TEST_CHECK(vplr.rgEnvVarMin[i] == 20 && vplr.rgEnvVarMax[i] == 80);
    }
    InvalidateRect(hwnd, NULL, TRUE);
    TEST_CHECK(UpdateWindow(hwnd));
    DestroyWindow(hwnd);
    vrgrcRCW = NULL;
}
#endif

TEST_LIST = {{"race attribute high bit", test_RaceGrbit_high_bit},
             {"LInnateRaceHabitability binary64 rounding", test_LInnateRaceHabitability_double_rounding},
             {"IRaceChecksum ignores stale name bytes", test_IRaceChecksum_ignores_stale_name_bytes},
             {"ReadRtPlr bounds record lengths", test_ReadRtPlr_bounds_record_lengths},
#ifdef STARS_UI_SDL
             {"RaceWizardDlg1 Custom clears the random-race bit", test_RaceWizardDlg1_custom_clears_random},
             {"RaceWizardDlg2 paints habitat bars", test_RaceWizardDlg2_paints_habitat},
#endif
             {NULL, NULL}};
