#include "acutest.h"
[[maybe_unused]] static char szTestLiteral2[] = "New-race-pass";
[[maybe_unused]] static char szTestLiteral3[] = "Replacement";
[[maybe_unused]] static char szTestLiteral4[] = "Shortened-race";

#include "stars_test.h"
[[maybe_unused]] static char szTestLiteral1[] = "data" szDirSep "humanoid.r1";

#ifdef STARS_UI_SDL
#include <stars_compat.h>
#include <SDL3/SDL_clipboard.h>
#endif

static int16_t FWriteLegacyRaceFixture(char *szFile, int16_t verMinor, const char *szName, const char *szNames,
                                       int16_t fPlural, int16_t fCompressed, int32_t lSalt) {
    RTBOF rtbof = {};
    std::byte rgbBof[cbWireBof];
    PLAYER plr;
    uint8_t rgb[sizeof(PLAYER) + 80];
    uint8_t *pb;
    int16_t cOut;
    int16_t i;
    uint16_t icksum;

    plr = vplr;
    plr.det = detAll;
    plr.lSalt = lSalt;
    memset(plr.szName, 0, sizeof(plr.szName));
    memset(plr.szNames, 0, sizeof(plr.szNames));
    strcpy(plr.szName, szName);
    if (fPlural) strcpy(plr.szNames, szNames);
    icksum = IRaceChecksum(&plr);

    memmove(rgb, &plr, offsetof(PLAYER, rgmdRelation));
    pb = rgb + offsetof(PLAYER, rgmdRelation);
    for (i = 15; i >= 0 && plr.rgmdRelation[i] == 0; i--) {
    }
    *pb++ = (uint8_t)(i + 1);
    memmove(pb, plr.rgmdRelation, i + 1);
    pb += i + 1;
    cOut = 31;
    if (fCompressed && FCompressUserString((char *)szName, (char *)pb + 1, &cOut)) {
        *pb = (uint8_t)cOut;
        pb += cOut + 1;
    } else {
        *pb++ = 0;
        strcpy((char *)pb, szName);
        pb += strlen(szName) + 1;
    }
    if (fPlural) {
        cOut = 31;
        if (fCompressed && FCompressUserString((char *)szNames, (char *)pb + 1, &cOut)) {
            *pb = (uint8_t)cOut;
            pb += cOut + 1;
        } else {
            *pb++ = 0;
            strcpy((char *)pb, szNames);
            pb += strlen(szNames) + 1;
        }
    }

    strncpy(rtbof.rgid, "J3J3", sizeof(rtbof.rgid));
    rtbof.verMajor = 2;
    rtbof.verMinor = verMinor;
    rtbof.dt = dtRace;
    StreamOpen(szFile, mdCreate);
    FEncodeBof(rtbof, rgbBof);
    WriteRt(rtBOF, cbWireBof, rgbBof);
    WriteRt(rtPlr, (int16_t)(pb - rgb), rgb);
    WriteRtEof(icksum);
    StreamClose();
    return TRUE;
}

static size_t CbReadFixture(const char *szFile, uint8_t *rgb, size_t cbMax) {
    FILE *pf;
    size_t cb;
    pf = fopen(szFile, "rb");
    if (!pf) return 0;
    cb = fread(rgb, 1, cbMax, pf);
    if (!feof(pf)) cb = 0;
    fclose(pf);
    return cb;
}

static void test_FWasRaceFile_imports_unprotected_legacy_designs(void) {
    char szDir[MAX_PATH], szFile[MAX_PATH + 64], szSaved[MAX_PATH + 64], szNewGameRace[MAX_PATH + 64], szDef[MAX_PATH + 64];
    uint8_t rgbBefore[512], rgbAfter[512];
    size_t cbBefore;
    PLAYER plrExpected;
    FILE *pf;
    uint16_t icksum;
    int i;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("legacy_race_import", szDir, sizeof(szDir)));
    TEST_ASSERT(FWasRaceFile(szTestLiteral1, FALSE));
    plrExpected = vplr;
    snprintf(szFile, sizeof(szFile), "%s%srace249.r1", szDir, szDirSep);
    TEST_ASSERT(FWriteLegacyRaceFixture(szFile, 49, "R\xe9", "", FALSE, FALSE, 0));
    cbBefore = CbReadFixture(szFile, rgbBefore, sizeof(rgbBefore));
    TEST_ASSERT(cbBefore > 0);
    TEST_CHECK(FWasRaceFile(szFile, FALSE) == 1);
    TEST_CHECK(strcmp(vplr.szName, "R\xc3\xa9") == 0);
    TEST_CHECK(strcmp(vplr.szNames, "R\xc3\xa9s") == 0);
    TEST_CHECK(memcmp(vplr.rgAttr, plrExpected.rgAttr, sizeof(vplr.rgAttr)) == 0);
    TEST_CHECK(vplr.grbitAttr == plrExpected.grbitAttr && vplr.pctIdealGrowth == plrExpected.pctIdealGrowth);
    TEST_CHECK(CbReadFixture(szFile, rgbAfter, sizeof(rgbAfter)) == cbBefore && memcmp(rgbBefore, rgbAfter, cbBefore) == 0);

    snprintf(szFile, sizeof(szFile), "%s%srace254.r1", szDir, szDirSep);
    TEST_ASSERT(FWriteLegacyRaceFixture(szFile, 54, "R\xe9", "", FALSE, TRUE, 0));
    TEST_CHECK(FWasRaceFile(szFile, FALSE) == 1);
    TEST_CHECK(strcmp(vplr.szName, "R\xc3\xa9") == 0 && strcmp(vplr.szNames, "R\xc3\xa9s") == 0);

    snprintf(szFile, sizeof(szFile), "%s%srace255.r1", szDir, szDirSep);
    TEST_ASSERT(FWriteLegacyRaceFixture(szFile, 55, "R\xe9", "R\xe9s", TRUE, TRUE, 0));
    TEST_CHECK(FWasRaceFile(szFile, FALSE) == 1);
    TEST_CHECK(strcmp(vplr.szName, "R\xc3\xa9") == 0 && strcmp(vplr.szNames, "R\xc3\xa9s") == 0);

    snprintf(szFile, sizeof(szFile), "%s%srace284.r1", szDir, szDirSep);
    strcpy(szNewGameRace, szFile);
    TEST_ASSERT(FWriteLegacyRaceFixture(szFile, 84, "R\xe9", "R\xe9s", TRUE, FALSE, 0));
    TEST_CHECK(FWasRaceFile(szFile, FALSE) == 1);
    TEST_CHECK(strcmp(vplr.szName, "R\xc3\xa9") == 0 && strcmp(vplr.szNames, "R\xc3\xa9s") == 0);

    snprintf(szSaved, sizeof(szSaved), "%s%ssaved.r1", szDir, szDirSep);
    TEST_ASSERT(FCreateFile(dtRace, iplrNone, szSaved));
    WriteRtPlr(&vplr, NULL);
    icksum = IRaceChecksum(&vplr);
    WriteRtEof(icksum);
    StreamClose();
    TEST_CHECK(FWasRaceFile(szSaved, FALSE) == 1);
    TEST_CHECK(strcmp(vplr.szName, "R\xc3\xa9") == 0 && strcmp(vplr.szNames, "R\xc3\xa9s") == 0);

    snprintf(szDef, sizeof(szDef), "%s%slegacy.def", szDir, szDirSep);
    pf = fopen(szDef, "wb"); TEST_ASSERT(pf);
    fprintf(pf, "Legacy Race\r\n0 0 1 12345\r\n0 0 0 0 0 0 0\r\n1\r\n%s\r\n1 60\r\n1 26 4\r\n", szNewGameRace);
    for (i = 0; i < 8; i++) fprintf(pf, "0\r\n");
    fprintf(pf, "%s%slegacy.xy\r\n", szDir, szDirSep);
    fclose(pf);
    TEST_CHECK(GenNewGameFromFile(szDef));
    TEST_CHECK(strcmp(rgplr[0].szName, "R\xc3\xa9") == 0);
    TEST_CHECK(strcmp(rgplr[0].szNames, "R\xc3\xa9s") == 0);

    snprintf(szFile, sizeof(szFile), "%s%sexpand.r1", szDir, szDirSep);
    {
        char szLongName[21];
        memset(szLongName, (char)0xe9, 20);
        szLongName[20] = 0;
        TEST_ASSERT(FWriteLegacyRaceFixture(szFile, 84, szLongName, "Expanded", TRUE, FALSE, 0));
    }
    pf = fopen(szDef, "wb"); TEST_ASSERT(pf);
    fprintf(pf, "Legacy Race\r\n0 0 1 12345\r\n0 0 0 0 0 0 0\r\n1\r\n%s\r\n1 60\r\n1 26 4\r\n", szFile);
    for (i = 0; i < 8; i++) fprintf(pf, "0\r\n");
    fprintf(pf, "%s%slegacy.xy\r\n", szDir, szDirSep);
    fclose(pf);
    iStarsTestAlertResponse = IDNO;
    TEST_CHECK(!GenNewGameFromFile(szDef));

    TEST_ASSERT(FWriteLegacyRaceFixture(szFile, 84, "Protected", "Protecteds", TRUE, FALSE, 1234));
    pf = fopen(szDef, "wb"); TEST_ASSERT(pf);
    fprintf(pf, "Legacy Race\r\n0 0 1 12345\r\n0 0 0 0 0 0 0\r\n1\r\n%s\r\n1 60\r\n1 26 4\r\n", szFile);
    for (i = 0; i < 8; i++) fprintf(pf, "0\r\n");
    fprintf(pf, "%s%slegacy.xy\r\n", szDir, szDirSep);
    fclose(pf);
    iStarsTestAlertResponse = 0;
    fStarsTestAcceptLegacyRacePassword = TRUE;
    strcpy(szStarsTestLegacyRacePassword, "New-race-pass");
    TEST_CHECK(GenNewGameFromFile(szDef));
    TEST_CHECK(rgplr[0].lSalt == LSaltFromSz(szTestLiteral2));
    TEST_CHECK(rgplr[0].lSalt != 1234);
    fStarsTestAcceptLegacyRacePassword = FALSE;
    TEST_CHECK(!GenNewGameFromFile(szDef));
    TEST_CHECK(cStarsTestLegacyRacePasswordPrompt == 2);
}

static void test_FWasRaceFile_legacy_replaces_protected_password_and_confirms_shortening(void) {
    char szDir[MAX_PATH], szFile[MAX_PATH + 64], szSaved[MAX_PATH + 64];
    char szName[32], szNames[32];
    uint8_t rgbBefore[512], rgbAfter[512];
    size_t cbBefore;
    FILE *pf;
    int ch;
    HDR hdr;
    uint16_t icksum;
    int i;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("legacy_race_decisions", szDir, sizeof(szDir)));
    TEST_ASSERT(FWasRaceFile(szTestLiteral1, FALSE));
    fFileErrSilent = FALSE;
    snprintf(szFile, sizeof(szFile), "%s%sprotected.r1", szDir, szDirSep);
    TEST_ASSERT(FWriteLegacyRaceFixture(szFile, 84, "Protected", "Protecteds", TRUE, FALSE, 1234));
    cbBefore = CbReadFixture(szFile, rgbBefore, sizeof(rgbBefore));
    TEST_ASSERT(cbBefore > 0);
    fFileErrSilent = TRUE;
    TEST_CHECK(FWasRaceFile(szFile, FALSE) == -1);
    TEST_CHECK(cStarsTestLegacyRacePasswordPrompt == 1);
    fStarsTestAcceptLegacyRacePassword = TRUE;
    strcpy(szStarsTestLegacyRacePassword, "Replacement");
    TEST_CHECK(FWasRaceFile(szFile, FALSE) == 1);
    TEST_CHECK(vplr.lSalt == LSaltFromSz(szTestLiteral3) && vplr.lSalt != 1234);
    TEST_CHECK(strcmp(szRacePass, "Replacement") == 0);
    TEST_CHECK(CbReadFixture(szFile, rgbAfter, sizeof(rgbAfter)) == cbBefore);
    TEST_CHECK(memcmp(rgbBefore, rgbAfter, cbBefore) == 0);
    snprintf(szSaved, sizeof(szSaved), "%s%sreplaced.r1", szDir, szDirSep);
    TEST_ASSERT(FCreateFile(dtRace, iplrNone, szSaved));
    WriteRtPlr(&vplr, NULL);
    icksum = IRaceChecksum(&vplr);
    WriteRtEof(icksum);
    StreamClose();
    strcpy(vszDefPass, "Replacement");
    TEST_CHECK(FWasRaceFile(szSaved, TRUE) == 1);
    TEST_CHECK(vplr.lSalt == LSaltFromSz(szTestLiteral3));
    vszDefPass[0] = 0;

    snprintf(szFile, sizeof(szFile), "%s%sbadsum.r1", szDir, szDirSep);
    TEST_ASSERT(FWriteLegacyRaceFixture(szFile, 84, "BadSum", "BadSums", TRUE, FALSE, 1234));
    pf = fopen(szFile, "r+b"); TEST_ASSERT(pf);
    TEST_ASSERT(fseek(pf, -1, SEEK_END) == 0);
    ch = fgetc(pf); TEST_ASSERT(ch != EOF);
    TEST_ASSERT(fseek(pf, -1, SEEK_END) == 0);
    TEST_ASSERT(fputc(ch ^ 1, pf) != EOF);
    fclose(pf);
    cStarsTestLegacyRacePasswordPrompt = 0;
    TEST_CHECK(FWasRaceFile(szFile, FALSE) == 0);
    TEST_CHECK(cStarsTestLegacyRacePasswordPrompt == 0);

    snprintf(szFile, sizeof(szFile), "%s%struncated.r1", szDir, szDirSep);
    TEST_ASSERT(FWriteLegacyRaceFixture(szFile, 84, "Truncated", "Truncateds", TRUE, TRUE, 0));
    pf = fopen(szFile, "r+b"); TEST_ASSERT(pf);
    memset(&hdr, 0, sizeof(hdr));
    hdr.cb = 3;
    hdr.rt = rtEOF;
    TEST_ASSERT(fseek(pf, -4, SEEK_END) == 0);
    TEST_ASSERT(fwrite(&hdr, sizeof(hdr), 1, pf) == 1);
    fclose(pf);
    TEST_CHECK(FWasRaceFile(szFile, FALSE) == 0);

    for (i = 0; i < 20; i++) szName[i] = (char)0xe9;
    szName[20] = 0;
    for (i = 0; i < 21; i++) szNames[i] = (char)0xe9;
    szNames[21] = 0;
    snprintf(szFile, sizeof(szFile), "%s%sexpand.r1", szDir, szDirSep);
    TEST_ASSERT(FWriteLegacyRaceFixture(szFile, 84, szName, szNames, TRUE, FALSE, 0));
    iStarsTestAlertResponse = IDYES;
    TEST_CHECK(FWasRaceFile(szFile, FALSE) == 1);
    TEST_CHECK(strlen(vplr.szName) <= 31 && strlen(vplr.szNames) <= 31);
    TEST_CHECK(FUtf8Valid(vplr.szName, strlen(vplr.szName)) && FUtf8Valid(vplr.szNames, strlen(vplr.szNames)));
    TEST_CHECK(strlen(vplr.szName) == 30 && strlen(vplr.szNames) == 30);
    TEST_CHECK(strstr(szStarsTestAlert, "Singular:") != NULL);

    // When both decisions are needed, accept the shortened names before
    // accepting the password; cancelling the names stops before that prompt.
    TEST_ASSERT(FWriteLegacyRaceFixture(szFile, 84, szName, szNames, TRUE, FALSE, 1234));
    iStarsTestAlertResponse = IDYES;
    fStarsTestAcceptLegacyRacePassword = TRUE;
    strcpy(szStarsTestLegacyRacePassword, "Shortened-race");
    cStarsTestLegacyRacePasswordPrompt = 0;
    TEST_CHECK(FWasRaceFile(szFile, FALSE) == 1);
    TEST_CHECK(cStarsTestLegacyRacePasswordPrompt == 1);
    TEST_CHECK(vplr.lSalt == LSaltFromSz(szTestLiteral4));

    TEST_ASSERT(FWriteLegacyRaceFixture(szFile, 84, szName, szNames, TRUE, FALSE, 1234));
    iStarsTestAlertResponse = IDNO;
    cStarsTestLegacyRacePasswordPrompt = 0;
    fStarsTestAcceptLegacyRacePassword = FALSE;
    TEST_CHECK(FWasRaceFile(szFile, FALSE) == -1);
    TEST_CHECK(cStarsTestLegacyRacePasswordPrompt == 0);

    iStarsTestAlertResponse = IDNO;
    fStarsTestAcceptLegacyRacePassword = FALSE;
    TEST_CHECK(FWasRaceFile(szFile, FALSE) == -1);
}

static void test_LInnateRaceHabitability_double_rounding(void) {
    PLAYER plr = {};
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
    PLAYER plr = {};

    SetRaceGrbit(&plr, (RaceGrbit)(0), TRUE);
    SetRaceGrbit(&plr, (RaceGrbit)(31), TRUE);
    TEST_CHECK(plr.grbitAttr == UINT32_C(0x80000001));
    TEST_CHECK(GetRaceGrbit(&plr, (RaceGrbit)(31)));
    SetRaceGrbit(&plr, (RaceGrbit)(31), FALSE);
    TEST_CHECK(plr.grbitAttr == 1);
    TEST_CHECK(!GetRaceGrbit(&plr, (RaceGrbit)(31)));
    TEST_CHECK(GetRaceGrbit(&plr, (RaceGrbit)(0)));
}

// Race file corruption: the race wizard reads a name into a buffer that still
// holds a longer one, and IRaceChecksum included the stale bytes after the
// terminator. They aren't saved, so the file failed its checksum on load.
static void test_IRaceChecksum_ignores_stale_name_bytes(void) {
    char     szDir[MAX_PATH];
    char     szFile[MAX_PATH + 64];
    PLAYER   plr;
    uint16_t icksum;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("IRaceChecksum_stale_name", szDir, sizeof(szDir)));
    TEST_ASSERT(FWasRaceFile(szTestLiteral1, FALSE));
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
    WriteRtEof(icksum);
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
    rgb[6] = detAll;
    iOff = cbWirePlayerPrefix;
    rgb[iOff++] = 40;
    memset(&rgb[iOff], 1, 40);
    iOff += 40;
    iOff++;
    strcpy((char *)&rgb[iOff], "Bob");
    iOff += 4;
    memset(&rgb[iOff + 1], 'N', 60);
    wVersFile = 0;
    wVersFile = (wVersFile & 31) | (85u << 5) | (2u << 12);

    memset(&buf, 0x55, sizeof(buf));
    ReadRtPlr(&buf.plr, rgb, sizeof(rgb));
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
    ReadRtPlr(&buf.plr, rgb, sizeof(rgb));
    TEST_CHECK(buf.plr.szNames[0] == 0);
    for (i = 0; i < (int)sizeof(buf.rgchGuard); i++)
        TEST_CHECK(buf.rgchGuard[i] == 0x55);
}

#ifdef STARS_UI_SDL
static void test_NewPasswordDlg_utf8_limit(void) {
    HWND hwnd;
    HWND hwndEdit;
    HWND hwndOrdinary;
    char szPass[20];
    char szReplacement[16] = "";
    const char *pszTooLong = "1234567890123456";
    const char *pszUnicodeTooLong = "\xf0\x9f\x98\x80\xf0\x9f\x98\x80\xf0\x9f\x98\x80\xf0\x9f\x98\x80";
    const char *pszExact = "1234567890123\xc3\xa9";
    int16_t idPlayerSave;
    int16_t fLogOffSave;

    TEST_ASSERT(FStarsTestInit());
    idPlayerSave = idPlayer;
    fLogOffSave = fLogOff;
    idPlayer = 0;
    fLogOff = TRUE;
    hwnd = HwndUiCreateDialog(hInst, MAKEINTRESOURCE(IDD_NEW_PASSWORD), NULL, NewPasswordDlg, 0);
    TEST_ASSERT(hwnd != NULL);
    hwndEdit = GetDlgItem(hwnd, IDC_EDIT1);
    TEST_ASSERT(hwndEdit != NULL);
    TEST_ASSERT(SDL_SetClipboardText(pszTooLong));
    LUiSendMessage(hwndEdit, WM_PASTE, 0, 0);
    TEST_CHECK(GetWindowText(hwndEdit, szPass, sizeof(szPass)) == 0);
    TEST_CHECK(StarsCompatTextInputRejected(hwndEdit));
    LUiSendMessage(hwndEdit, WM_CHAR, '\b', 0);
    LUiSendMessage(hwndEdit, WM_KEYDOWN, VK_DELETE, 0);
    TEST_CHECK(StarsCompatTextInputRejected(hwndEdit));
    cStarsTestAlert = 0;
    LUiSendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, IDOK));
    TEST_CHECK(cStarsTestAlert == 1);
    TEST_CHECK(strstr(szStarsTestAlert, "enter it again") != NULL);
    TEST_CHECK(IsWindow(hwnd));

    TEST_ASSERT(SDL_SetClipboardText(pszExact));
    LUiSendMessage(hwndEdit, WM_PASTE, 0, 0);
    TEST_CHECK(GetWindowText(hwndEdit, szPass, sizeof(szPass)) == 15);
    TEST_CHECK(FUtf8Valid(szPass, strlen(szPass)));
    TEST_CHECK(!StarsCompatTextInputRejected(hwndEdit));

    hwndEdit = GetDlgItem(hwnd, IDC_PASSWORD_CONFIRM);
    TEST_ASSERT(hwndEdit != NULL);
    TEST_ASSERT(SDL_SetClipboardText(pszUnicodeTooLong));
    LUiSendMessage(hwndEdit, WM_PASTE, 0, 0);
    TEST_CHECK(GetWindowText(hwndEdit, szPass, sizeof(szPass)) == 0);
    TEST_CHECK(StarsCompatTextInputRejected(hwndEdit));
    LUiSendMessage(hwndEdit, WM_CHAR, '\b', 0);
    LUiSendMessage(hwndEdit, WM_KEYDOWN, VK_DELETE, 0);
    TEST_CHECK(StarsCompatTextInputRejected(hwndEdit));
    LUiSendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, IDOK));
    TEST_CHECK(cStarsTestAlert == 2);
    TEST_CHECK(strstr(szStarsTestAlert, "enter it again") != NULL);
    TEST_CHECK(IsWindow(hwnd));

    TEST_ASSERT(SDL_SetClipboardText(pszExact));
    LUiSendMessage(hwndEdit, WM_PASTE, 0, 0);
    TEST_CHECK(GetWindowText(hwndEdit, szPass, sizeof(szPass)) == 15);
    TEST_CHECK(FUtf8Valid(szPass, strlen(szPass)));
    TEST_CHECK(!StarsCompatTextInputRejected(hwndEdit));

    TEST_ASSERT(FUiSetWindowText(hwndEdit, "different"));
    LUiSendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, IDOK));
    TEST_CHECK(cStarsTestAlert == 3);
    TEST_CHECK(strstr(szStarsTestAlert, "not the same") != NULL);
    TEST_CHECK(IsWindow(hwnd));
    TEST_ASSERT(FUiSetWindowText(hwndEdit, pszExact));
    LUiSendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, IDOK));
    TEST_CHECK(cStarsTestAlert == 3);
    TEST_CHECK(IsWindow(hwnd));
    hwndOrdinary = HwndUiCreateWindowEx(0, "EDIT", "", WS_CHILD, 0, 0, 100, 24, hwnd, NULL, hInst, NULL);
    TEST_ASSERT(hwndOrdinary != NULL);
    LUiSendMessage(hwndOrdinary, EM_LIMITTEXT, 15, 0);
    TEST_ASSERT(SDL_SetClipboardText(pszTooLong));
    LUiSendMessage(hwndOrdinary, WM_PASTE, 0, 0);
    TEST_CHECK(GetWindowText(hwndOrdinary, szPass, sizeof(szPass)) == 15);
    TEST_CHECK(!StarsCompatTextInputRejected(hwndOrdinary));
    hwndEdit = HwndUiCreateDialog(hInst, MAKEINTRESOURCE(IDD_NEW_PASSWORD), NULL, NewPasswordDlg,
                                  (LPARAM)szReplacement);
    TEST_ASSERT(hwndEdit != NULL);
    TEST_ASSERT(FUiSetWindowText(GetDlgItem(hwndEdit, IDC_EDIT1), pszExact));
    TEST_ASSERT(FUiSetWindowText(GetDlgItem(hwndEdit, IDC_PASSWORD_CONFIRM), pszExact));
    LUiSendMessage(hwndEdit, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)GetDlgItem(hwndEdit, IDOK));
    TEST_CHECK(strcmp(szReplacement, pszExact) == 0);
    FUiDestroyWindow(hwndEdit);
    szReplacement[0] = 0;
    hwndEdit = HwndUiCreateDialog(hInst, MAKEINTRESOURCE(IDD_NEW_PASSWORD), NULL, NewPasswordDlg,
                                  (LPARAM)szReplacement);
    TEST_ASSERT(hwndEdit != NULL);
    cStarsTestAlert = 0;
    LUiSendMessage(hwndEdit, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)GetDlgItem(hwndEdit, IDOK));
    TEST_CHECK(cStarsTestAlert == 1);
    TEST_CHECK(strstr(szStarsTestAlert, "replacement password is required") != NULL);
    TEST_CHECK(IsWindow(hwndEdit));
    FUiDestroyWindow(hwndEdit);
    idPlayer = idPlayerSave;
    fLogOff = fLogOffSave;
    FUiDestroyWindow(hwnd);
}

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
    hwnd = HwndUiCreateDialog(hInst, MAKEINTRESOURCE(IDD_RACE_WIZARD_1), NULL, RaceWizardDlg1, 0);
    TEST_ASSERT(hwnd != NULL);
    CheckRadioButton(hwnd, IDC_RACE_HUMANOID, IDC_RACE_CUSTOM, IDC_RACE_CUSTOM);
    hwndCtl = GetDlgItem(hwnd, IDC_RACE_CUSTOM);
    LUiSendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_RACE_CUSTOM, BN_CLICKED), (LPARAM)hwndCtl);
    LUiSendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_NEXT, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, IDC_NEXT));
    FUiDestroyWindow(hwnd);
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
    hwnd = HwndUiCreateDialog(hInst, MAKEINTRESOURCE(IDD_RACE_WIZARD_2), NULL, RaceWizardDlg2, 0);
    TEST_ASSERT(hwnd != NULL);
    TEST_CHECK(crcRCW == 17);
    FUiShowWindow(hwnd, SW_SHOW);
    TEST_CHECK(FUiUpdateWindow(hwnd));
    for (i = 0; i < 3; i++) {
        CheckDlgButton(hwnd, IDC_IMMUNE_TO_GRAVITY + i, BST_CHECKED);
        LUiSendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_IMMUNE_TO_GRAVITY + i, BN_CLICKED),
                    (LPARAM)GetDlgItem(hwnd, IDC_IMMUNE_TO_GRAVITY + i));
        TEST_CHECK(vplr.rgEnvVarMax[i] == envImmune);
    }
    DrawRace2(hwnd, NULL, -1);
    for (i = 0; i < 3; i++) {
        CheckDlgButton(hwnd, IDC_IMMUNE_TO_GRAVITY + i, BST_UNCHECKED);
        LUiSendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_IMMUNE_TO_GRAVITY + i, BN_CLICKED),
                    (LPARAM)GetDlgItem(hwnd, IDC_IMMUNE_TO_GRAVITY + i));
        TEST_CHECK(vplr.rgEnvVarMin[i] == 20 && vplr.rgEnvVarMax[i] == 80);
    }
    InvalidateRect(hwnd, NULL, TRUE);
    TEST_CHECK(FUiUpdateWindow(hwnd));
    FUiDestroyWindow(hwnd);
    vrgrcRCW = NULL;
}
#endif

static void test_IRaceChecksum_wire_image(void) {
    std::byte rgb[192];
    PLAYER plr{};
    for (int i = 0; i < 192; i++) rgb[i] = static_cast<std::byte>((i * 73 + 11) & 255);
    TEST_ASSERT(FDecodePlayer(rgb, plr));
    TEST_CHECK(IRaceChecksum(&plr) == 0x80c0);
}

TEST_LIST = {{"race checksum uses the fixed 192-byte wire image", test_IRaceChecksum_wire_image},
             {"race attribute high bit", test_RaceGrbit_high_bit},
             {"LInnateRaceHabitability binary64 rounding", test_LInnateRaceHabitability_double_rounding},
             {"FWasRaceFile imports unprotected legacy race designs", test_FWasRaceFile_imports_unprotected_legacy_designs},
             {"FWasRaceFile replaces protected legacy passwords and confirms name shortening", test_FWasRaceFile_legacy_replaces_protected_password_and_confirms_shortening},
             {"IRaceChecksum ignores stale name bytes", test_IRaceChecksum_ignores_stale_name_bytes},
             {"ReadRtPlr bounds record lengths", test_ReadRtPlr_bounds_record_lengths},
#ifdef STARS_UI_SDL
             {"NewPasswordDlg keeps Unicode passwords within the login limit", test_NewPasswordDlg_utf8_limit},
             {"RaceWizardDlg1 Custom clears the random-race bit", test_RaceWizardDlg1_custom_clears_random},
             {"RaceWizardDlg2 paints habitat bars", test_RaceWizardDlg2_paints_habitat},
#endif
             {NULL, NULL}};
