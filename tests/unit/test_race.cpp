#include "acutest.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include <array>
#include <string_view>
#include <string>
#include "record_io.h"
#include "text_io.h"

static int cRaceInputOpen;
static std::string szRaceInputOpened;
static void OpenRaceInputAndMutate(APPSESSION &app, const char *psz, MdOpenFlags md) {
    ++cRaceInputOpen;
    szRaceInputOpened = psz;
    StreamOpen(app, psz, md);
    CchCopyUtf8(std::span{app.state.szWork}, "changed-by-open");
    CchCopyUtf8(std::span{app.state.szBase}, "changed-by-open");
}
[[maybe_unused]] static char szTestLiteral2[] = "New-race-pass";
[[maybe_unused]] static char szTestLiteral3[] = "Replacement";
[[maybe_unused]] static char szTestLiteral4[] = "Shortened-race";

#include "stars_test.h"
[[maybe_unused]] static auto szTestLiteral1 = [] {
    auto sz = std::to_array("data/humanoid.r1");
    sz[4] = chDirSep;
    return sz;
}();

#ifdef STARS_UI_SDL
#include <stars_compat.h>
#include <SDL3/SDL_clipboard.h>
#include "control_io.h"
#endif

static void test_RaceFile_owned_input(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    TEST_ASSERT(FStarsTestInit(app));
    RECORDIO io = RecordIO();
    io.pfnStreamOpen = OpenRaceInputAndMutate;
    RECORDIOSCOPE recordScope(io);
    for (int i = 0; i < 2; ++i) {
        char *szFile = i == 0 ? app.state.szWork : app.state.szBase;
        TEST_ASSERT(CchCopyUtf8(std::span{szFile, i == 0 ? sizeof(app.state.szWork) : sizeof(app.state.szBase)},
            szTestLiteral1.data()) >= 0);
        cRaceInputOpen = 0;
        TEST_ASSERT(FWasRaceFile(app, szFile, 0));
        TEST_CHECK(cRaceInputOpen == 1 && szRaceInputOpened == szTestLiteral1.data());
        TEST_ASSERT_(std::string_view(app.state.szRaceFile) == "humanoid.r1", "race filename changed to [%s] during loading", app.state.szRaceFile);
        TEST_CHECK(app.state.hf == -1 && app.state.fFileErrSilent == 1);
        TEST_CHECK(!FStarsRecoveryActive() && !FStarsErrorPending());
    }
    const std::string szGuarded = std::string(szTestLiteral1.data()) + "!";
    TEST_ASSERT(FWasRaceFile(app, std::string_view{szGuarded.data(), szGuarded.size() - 1}, 0));
    TEST_CHECK(szRaceInputOpened == szTestLiteral1.data());
    TEST_CHECK(std::string_view(app.state.szRaceFile) == "humanoid.r1");
}

static int cRaceInputClose;
static std::string szRaceInputDiagnostic;
static void CaptureRaceInput(APPSESSION &, const char *psz, MdOpenFlags md) {
    TEST_CHECK(md == mdRead);
    ++cRaceInputOpen;
    szRaceInputOpened = psz;
}
static void ReadInvalidRaceInput(APPSESSION &app) {
    app.state.hdrCur = {};
    app.state.hdrCur.rt = rtEOF;
    CchCopyUtf8(std::span{app.state.szWork}, "changed-by-read");
}
static void CloseRaceInput(APPSESSION &app) {
    ++cRaceInputClose;
    StreamClose(app);
    CchCopyUtf8(std::span{app.state.szWork}, "changed-by-close");
}
static std::array<char, 256> CaptureRaceInputDiagnostic(APPSESSION &app, StringId ids, std::span<const std::int16_t> pParams) {
    TEST_CHECK(ids == idsFileDoesBelongVersionStars && pParams.empty());
    szRaceInputDiagnostic = app.state.szWork;
    return PszFormatIds(app, ids, pParams);
}
static void test_RaceFile_input_bounds(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    TEST_ASSERT(FStarsTestInit(app));
    RECORDIO io = RecordIO();
    io.pfnStreamOpen = CaptureRaceInput;
    io.pfnReadRt = ReadInvalidRaceInput;
    io.pfnStreamClose = CloseRaceInput;
    RECORDIOSCOPE recordScope(io);
    TEXTIO text = TextIO();
    text.pfnFormat = CaptureRaceInputDiagnostic;
    TEXTIOSCOPE textScope(text);
    app.state.fFileErrSilent = 0;
    CchCopyUtf8(std::span{app.state.szWork}, "preserved");
    const std::array<std::string_view, 3> rgszBad = {"", "\xff", std::string_view{"race.r1\0tail", 12}};
    cRaceInputOpen = cRaceInputClose = 0;
    for (const auto sz : rgszBad) {
        TEST_CHECK(FWasRaceFile(app, sz, 0) == 0);
        TEST_CHECK(cRaceInputOpen == 0 && cRaceInputClose == 0 && cStarsTestAlert == 0);
        TEST_CHECK(std::string_view(app.state.szWork) == "preserved" && app.state.fFileErrSilent == 0);
    }
    const std::array<std::string, 3> rgszFile = {"bad-Ω.r1", std::string(358, 'a') + "Ω.r1", std::string(1024, 'b') + ".r1"};
    for (const auto &sz : rgszFile) {
        const auto vplrSav = app.state.vplr;
        cRaceInputOpen = cRaceInputClose = 0;
        TEST_CHECK(FWasRaceFile(app, sz, 0) == -1);
        TEST_CHECK(cRaceInputOpen == 1 && cRaceInputClose == 1 && szRaceInputOpened == sz);
        TEST_CHECK(szRaceInputDiagnostic == sz.substr(0, CchUtf8Prefix(sz, sizeof(app.state.szWork) - 1)));
        TEST_CHECK(FUtf8Valid(szRaceInputDiagnostic));
        TEST_CHECK(std::memcmp(&app.state.vplr, &vplrSav, sizeof(vplrSav)) == 0);
        TEST_CHECK(app.state.hf == -1 && app.state.fFileErrSilent == 0);
        TEST_CHECK(!FStarsRecoveryActive() && !FStarsErrorPending());
    }
}

static int16_t FWriteLegacyRaceFixture(APPSESSION &app, char *szFile, int16_t verMinor, const char *szName, const char *szNames,
                                       int16_t fPlural, int16_t fCompressed, int32_t lSalt) {
    RTBOF rtbof = {};
    std::byte rgbBof[cbWireBof];
    PLAYER plr;
    uint8_t rgb[sizeof(PLAYER) + 80];
    uint8_t *pb;
    int16_t cOut;
    int16_t i;
    uint16_t icksum;

    plr = app.state.vplr;
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
    const auto cbPacked1 = fCompressed ? FCompressUserString(app, (char *)szName, std::as_writable_bytes(std::span((char *)pb + 1, static_cast<std::size_t>(cOut)))) : std::nullopt;
    if (cbPacked1) cOut = static_cast<std::int16_t>(*cbPacked1);
    if (fCompressed && cbPacked1) {
        *pb = (uint8_t)cOut;
        pb += cOut + 1;
    } else {
        *pb++ = 0;
        strcpy((char *)pb, szName);
        pb += strlen(szName) + 1;
    }
    if (fPlural) {
        cOut = 31;
        const auto cbPacked2 = fCompressed ? FCompressUserString(app, (char *)szNames, std::as_writable_bytes(std::span((char *)pb + 1, static_cast<std::size_t>(cOut)))) : std::nullopt;
        if (cbPacked2) cOut = static_cast<std::int16_t>(*cbPacked2);
        if (fCompressed && cbPacked2) {
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
    StreamOpen(app, szFile, mdCreate);
    FEncodeBof(rtbof, rgbBof);
    WriteRt(app, rtBOF, std::as_bytes(std::span{rgbBof}).first(static_cast<std::size_t>(cbWireBof)));
    WriteRt(app, rtPlr, std::as_bytes(std::span{rgb}).first(static_cast<std::size_t>(pb - rgb)));
    WriteRtEof(app, icksum);
    StreamClose(app);
    return 1;
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
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[cbTestPath], szFile[cbTestPath + 64], szSaved[cbTestPath + 64], szNewGameRace[cbTestPath + 64], szDef[cbTestPath + 64];
    uint8_t rgbBefore[512], rgbAfter[512];
    size_t cbBefore;
    PLAYER plrExpected;
    FILE *pf;
    uint16_t icksum;
    int i;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("legacy_race_import", szDir, sizeof(szDir)));
    TEST_ASSERT(FWasRaceFile(app, szTestLiteral1.data(), 0));
    plrExpected = app.state.vplr;
    snprintf(szFile, sizeof(szFile), "%s%srace249.r1", szDir, szDirSep);
    TEST_ASSERT(FWriteLegacyRaceFixture(app, szFile, 49, "R\xe9", "", 0, 0, 0));
    cbBefore = CbReadFixture(szFile, rgbBefore, sizeof(rgbBefore));
    TEST_ASSERT(cbBefore > 0);
    TEST_CHECK(FWasRaceFile(app, szFile, 0) == 1);
    TEST_CHECK(strcmp(app.state.vplr.szName, "R\xc3\xa9") == 0);
    TEST_CHECK(strcmp(app.state.vplr.szNames, "R\xc3\xa9s") == 0);
    TEST_CHECK(memcmp(app.state.vplr.rgAttr, plrExpected.rgAttr, sizeof(app.state.vplr.rgAttr)) == 0);
    TEST_CHECK(app.state.vplr.grbitAttr == plrExpected.grbitAttr && app.state.vplr.pctIdealGrowth == plrExpected.pctIdealGrowth);
    TEST_CHECK(CbReadFixture(szFile, rgbAfter, sizeof(rgbAfter)) == cbBefore && memcmp(rgbBefore, rgbAfter, cbBefore) == 0);

    snprintf(szFile, sizeof(szFile), "%s%srace254.r1", szDir, szDirSep);
    TEST_ASSERT(FWriteLegacyRaceFixture(app, szFile, 54, "R\xe9", "", 0, 1, 0));
    TEST_CHECK(FWasRaceFile(app, szFile, 0) == 1);
    TEST_CHECK(strcmp(app.state.vplr.szName, "R\xc3\xa9") == 0 && strcmp(app.state.vplr.szNames, "R\xc3\xa9s") == 0);

    snprintf(szFile, sizeof(szFile), "%s%srace255.r1", szDir, szDirSep);
    TEST_ASSERT(FWriteLegacyRaceFixture(app, szFile, 55, "R\xe9", "R\xe9s", 1, 1, 0));
    TEST_CHECK(FWasRaceFile(app, szFile, 0) == 1);
    TEST_CHECK(strcmp(app.state.vplr.szName, "R\xc3\xa9") == 0 && strcmp(app.state.vplr.szNames, "R\xc3\xa9s") == 0);

    snprintf(szFile, sizeof(szFile), "%s%srace284.r1", szDir, szDirSep);
    strcpy(szNewGameRace, szFile);
    TEST_ASSERT(FWriteLegacyRaceFixture(app, szFile, 84, "R\xe9", "R\xe9s", 1, 0, 0));
    TEST_CHECK(FWasRaceFile(app, szFile, 0) == 1);
    TEST_CHECK(strcmp(app.state.vplr.szName, "R\xc3\xa9") == 0 && strcmp(app.state.vplr.szNames, "R\xc3\xa9s") == 0);

    snprintf(szSaved, sizeof(szSaved), "%s%ssaved.r1", szDir, szDirSep);
    TEST_ASSERT(FCreateFile(app, dtRace, iplrNone, szSaved));
    WriteRtPlr(app, &app.state.vplr);
    icksum = IRaceChecksum(&app.state.vplr);
    WriteRtEof(app, icksum);
    StreamClose(app);
    TEST_CHECK(FWasRaceFile(app, szSaved, 0) == 1);
    TEST_CHECK(strcmp(app.state.vplr.szName, "R\xc3\xa9") == 0 && strcmp(app.state.vplr.szNames, "R\xc3\xa9s") == 0);

    snprintf(szDef, sizeof(szDef), "%s%slegacy.def", szDir, szDirSep);
    pf = fopen(szDef, "wb"); TEST_ASSERT(pf);
    fprintf(pf, "Legacy Race\r\n0 0 1 12345\r\n0 0 0 0 0 0 0\r\n1\r\n%s\r\n1 60\r\n1 26 4\r\n", szNewGameRace);
    for (i = 0; i < 8; i++) fprintf(pf, "0\r\n");
    fprintf(pf, "%s%slegacy.xy\r\n", szDir, szDirSep);
    fclose(pf);
    TEST_CHECK(GenNewGameFromFile(app, szDef));
    TEST_CHECK(strcmp(app.state.rgplr[0].szName, "R\xc3\xa9") == 0);
    TEST_CHECK(strcmp(app.state.rgplr[0].szNames, "R\xc3\xa9s") == 0);

    snprintf(szFile, sizeof(szFile), "%s%sexpand.r1", szDir, szDirSep);
    {
        char szLongName[21];
        memset(szLongName, (char)0xe9, 20);
        szLongName[20] = 0;
        TEST_ASSERT(FWriteLegacyRaceFixture(app, szFile, 84, szLongName, "Expanded", 1, 0, 0));
    }
    pf = fopen(szDef, "wb"); TEST_ASSERT(pf);
    fprintf(pf, "Legacy Race\r\n0 0 1 12345\r\n0 0 0 0 0 0 0\r\n1\r\n%s\r\n1 60\r\n1 26 4\r\n", szFile);
    for (i = 0; i < 8; i++) fprintf(pf, "0\r\n");
    fprintf(pf, "%s%slegacy.xy\r\n", szDir, szDirSep);
    fclose(pf);
    iStarsTestAlertResponse = win32::IDNO;
    TEST_CHECK(!GenNewGameFromFile(app, szDef));

    TEST_ASSERT(FWriteLegacyRaceFixture(app, szFile, 84, "Protected", "Protecteds", 1, 0, 1234));
    pf = fopen(szDef, "wb"); TEST_ASSERT(pf);
    fprintf(pf, "Legacy Race\r\n0 0 1 12345\r\n0 0 0 0 0 0 0\r\n1\r\n%s\r\n1 60\r\n1 26 4\r\n", szFile);
    for (i = 0; i < 8; i++) fprintf(pf, "0\r\n");
    fprintf(pf, "%s%slegacy.xy\r\n", szDir, szDirSep);
    fclose(pf);
    iStarsTestAlertResponse = 0;
    fStarsTestAcceptLegacyRacePassword = 1;
    strcpy(szStarsTestLegacyRacePassword, "New-race-pass");
    TEST_CHECK(GenNewGameFromFile(app, szDef));
    TEST_CHECK(app.state.rgplr[0].lSalt == LSaltFromSz(szTestLiteral2));
    TEST_CHECK(app.state.rgplr[0].lSalt != 1234);
    fStarsTestAcceptLegacyRacePassword = 0;
    TEST_CHECK(!GenNewGameFromFile(app, szDef));
    TEST_CHECK(cStarsTestLegacyRacePasswordPrompt == 2);
}

static void test_FWasRaceFile_legacy_replaces_protected_password_and_confirms_shortening(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[cbTestPath], szFile[cbTestPath + 64], szSaved[cbTestPath + 64];
    char szName[32], szNames[32];
    uint8_t rgbBefore[512], rgbAfter[512];
    size_t cbBefore;
    FILE *pf;
    int ch;
    HDR hdr;
    uint16_t icksum;
    int i;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("legacy_race_decisions", szDir, sizeof(szDir)));
    TEST_ASSERT(FWasRaceFile(app, szTestLiteral1.data(), 0));
    app.state.fFileErrSilent = 0;
    snprintf(szFile, sizeof(szFile), "%s%sprotected.r1", szDir, szDirSep);
    TEST_ASSERT(FWriteLegacyRaceFixture(app, szFile, 84, "Protected", "Protecteds", 1, 0, 1234));
    cbBefore = CbReadFixture(szFile, rgbBefore, sizeof(rgbBefore));
    TEST_ASSERT(cbBefore > 0);
    app.state.fFileErrSilent = 1;
    TEST_CHECK(FWasRaceFile(app, szFile, 0) == -1);
    TEST_CHECK(cStarsTestLegacyRacePasswordPrompt == 1);
    fStarsTestAcceptLegacyRacePassword = 1;
    strcpy(szStarsTestLegacyRacePassword, "Replacement");
    TEST_CHECK(FWasRaceFile(app, szFile, 0) == 1);
    TEST_CHECK(app.state.vplr.lSalt == LSaltFromSz(szTestLiteral3) && app.state.vplr.lSalt != 1234);
    TEST_CHECK(strcmp(app.state.szRacePass, "Replacement") == 0);
    TEST_CHECK(CbReadFixture(szFile, rgbAfter, sizeof(rgbAfter)) == cbBefore);
    TEST_CHECK(memcmp(rgbBefore, rgbAfter, cbBefore) == 0);
    snprintf(szSaved, sizeof(szSaved), "%s%sreplaced.r1", szDir, szDirSep);
    TEST_ASSERT(FCreateFile(app, dtRace, iplrNone, szSaved));
    WriteRtPlr(app, &app.state.vplr);
    icksum = IRaceChecksum(&app.state.vplr);
    WriteRtEof(app, icksum);
    StreamClose(app);
    strcpy(app.state.vszDefPass, "Replacement");
    TEST_CHECK(FWasRaceFile(app, szSaved, 1) == 1);
    TEST_CHECK(app.state.vplr.lSalt == LSaltFromSz(szTestLiteral3));
    app.state.vszDefPass[0] = 0;

    snprintf(szFile, sizeof(szFile), "%s%sbadsum.r1", szDir, szDirSep);
    TEST_ASSERT(FWriteLegacyRaceFixture(app, szFile, 84, "BadSum", "BadSums", 1, 0, 1234));
    pf = fopen(szFile, "r+b"); TEST_ASSERT(pf);
    TEST_ASSERT(fseek(pf, -1, SEEK_END) == 0);
    ch = fgetc(pf); TEST_ASSERT(ch != EOF);
    TEST_ASSERT(fseek(pf, -1, SEEK_END) == 0);
    TEST_ASSERT(fputc(ch ^ 1, pf) != EOF);
    fclose(pf);
    cStarsTestLegacyRacePasswordPrompt = 0;
    TEST_CHECK(FWasRaceFile(app, szFile, 0) == 0);
    TEST_CHECK(cStarsTestLegacyRacePasswordPrompt == 0);

    snprintf(szFile, sizeof(szFile), "%s%struncated.r1", szDir, szDirSep);
    TEST_ASSERT(FWriteLegacyRaceFixture(app, szFile, 84, "Truncated", "Truncateds", 1, 1, 0));
    pf = fopen(szFile, "r+b"); TEST_ASSERT(pf);
    memset(&hdr, 0, sizeof(hdr));
    hdr.cb = 3;
    hdr.rt = rtEOF;
    TEST_ASSERT(fseek(pf, -4, SEEK_END) == 0);
    TEST_ASSERT(fwrite(&hdr, sizeof(hdr), 1, pf) == 1);
    fclose(pf);
    TEST_CHECK(FWasRaceFile(app, szFile, 0) == 0);

    for (i = 0; i < 20; i++) szName[i] = (char)0xe9;
    szName[20] = 0;
    for (i = 0; i < 21; i++) szNames[i] = (char)0xe9;
    szNames[21] = 0;
    snprintf(szFile, sizeof(szFile), "%s%sexpand.r1", szDir, szDirSep);
    TEST_ASSERT(FWriteLegacyRaceFixture(app, szFile, 84, szName, szNames, 1, 0, 0));
    iStarsTestAlertResponse = win32::IDYES;
    TEST_CHECK(FWasRaceFile(app, szFile, 0) == 1);
    TEST_CHECK(strlen(app.state.vplr.szName) <= 31 && strlen(app.state.vplr.szNames) <= 31);
    TEST_CHECK(FUtf8Valid(app.state.vplr.szName) && FUtf8Valid(app.state.vplr.szNames));
    TEST_CHECK(strlen(app.state.vplr.szName) == 30 && strlen(app.state.vplr.szNames) == 30);
    TEST_CHECK(strstr(szStarsTestAlert, "Singular:") != NULL);

    // When both decisions are needed, accept the shortened names before
    // accepting the password; cancelling the names stops before that prompt.
    TEST_ASSERT(FWriteLegacyRaceFixture(app, szFile, 84, szName, szNames, 1, 0, 1234));
    iStarsTestAlertResponse = win32::IDYES;
    fStarsTestAcceptLegacyRacePassword = 1;
    strcpy(szStarsTestLegacyRacePassword, "Shortened-race");
    cStarsTestLegacyRacePasswordPrompt = 0;
    TEST_CHECK(FWasRaceFile(app, szFile, 0) == 1);
    TEST_CHECK(cStarsTestLegacyRacePasswordPrompt == 1);
    TEST_CHECK(app.state.vplr.lSalt == LSaltFromSz(szTestLiteral4));

    TEST_ASSERT(FWriteLegacyRaceFixture(app, szFile, 84, szName, szNames, 1, 0, 1234));
    iStarsTestAlertResponse = win32::IDNO;
    cStarsTestLegacyRacePasswordPrompt = 0;
    fStarsTestAcceptLegacyRacePassword = 0;
    TEST_CHECK(FWasRaceFile(app, szFile, 0) == -1);
    TEST_CHECK(cStarsTestLegacyRacePasswordPrompt == 0);

    iStarsTestAlertResponse = win32::IDNO;
    fStarsTestAcceptLegacyRacePassword = 0;
    TEST_CHECK(FWasRaceFile(app, szFile, 0) == -1);
}

static void test_LInnateRaceHabitability_double_rounding(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    PLAYER plr = {};
    int8_t rgAttr[16] = {10, 10, 10, 10, 10, 5, 10, 0, 1, 1, 1, 1, 1, 1, 9, 0};
    int8_t rgLo[3] = {31, 61, 9};
    int8_t rgHi[3] = {71, 97, 73};
    int i;

    TEST_ASSERT(FStarsTestInit(app));
    memcpy(plr.rgAttr, rgAttr, sizeof(rgAttr));
    plr.pctIdealGrowth = 10;
    for (i = 0; i < 3; i++) {
        plr.rgEnvVarMin[i] = rgLo[i];
        plr.rgEnvVarMax[i] = rgHi[i];
        plr.rgEnvVar[i] = (rgLo[i] + rgHi[i]) / 2;
    }
    // Extended intermediates returned 1470286; binary64 crosses this boundary.
    TEST_CHECK(LInnateRaceHabitability(app, &plr) == 1470287);
    TEST_CHECK(CAdvantagePoints(app, &plr) == 541);
}

static void test_RaceGrbit_high_bit(void) {
    PLAYER plr = {};

    SetRaceGrbit(&plr, (RaceGrbit)(0), 1);
    SetRaceGrbit(&plr, (RaceGrbit)(31), 1);
    TEST_CHECK(plr.grbitAttr == UINT32_C(0x80000001));
    TEST_CHECK(GetRaceGrbit(&plr, (RaceGrbit)(31)));
    SetRaceGrbit(&plr, (RaceGrbit)(31), 0);
    TEST_CHECK(plr.grbitAttr == 1);
    TEST_CHECK(!GetRaceGrbit(&plr, (RaceGrbit)(31)));
    TEST_CHECK(GetRaceGrbit(&plr, (RaceGrbit)(0)));
}

// Race file corruption: the race wizard reads a name into a buffer that still
// holds a longer one, and IRaceChecksum included the stale bytes after the
// terminator. They aren't saved, so the file failed its checksum on load.
static void test_IRaceChecksum_ignores_stale_name_bytes(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char     szDir[cbTestPath];
    char     szFile[cbTestPath + 64];
    PLAYER   plr;
    uint16_t icksum;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("IRaceChecksum_stale_name", szDir, sizeof(szDir)));
    TEST_ASSERT(FWasRaceFile(app, szTestLiteral1.data(), 0));
    plr = app.state.vplr;
    strcpy(plr.szName, "Longname");
    strcpy(plr.szNames, "Longnames");
    // A shorter name typed over them, as GetDlgItemText leaves the buffers.
    strcpy(plr.szName, "Bo");
    strcpy(plr.szNames, "Bos");

    // Save as FSaveRace does.
    snprintf(szFile, sizeof(szFile), "%s%sshort.r1", szDir, szDirSep);
    TEST_ASSERT(FCreateFile(app, dtRace, iplrNone, szFile));
    WriteRtPlr(app, &plr);
    icksum = IRaceChecksum(&plr);
    WriteRtEof(app, icksum);
    StreamClose(app);

    TEST_CHECK(FWasRaceFile(app, szFile, 0));
    TEST_CHECK(strcmp(app.state.vplr.szNames, "Bos") == 0);
}

// ReadRtPlr took the relation count and uncompressed name lengths from the
// record, so a corrupt one overran rgmdRelation into szName and the names
// into the rest of the PLAYER.
static void test_ReadRtPlr_bounds_record_lengths(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

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
    app.state.wVersFile = 0;
    app.state.wVersFile = (app.state.wVersFile & 31) | (85u << 5) | (2u << 12);

    memset(&buf, 0x55, sizeof(buf));
    ReadRtPlr(app, &buf.plr, rgb, sizeof(rgb));
    for (i = 0; i < 16; i++)
        TEST_CHECK(buf.plr.rgmdRelation[i] == 1);
    TEST_CHECK(strcmp(buf.plr.szName, "Bob") == 0);
    for (i = 4; i < (int)sizeof(buf.plr.szName); i++)
        TEST_CHECK(buf.plr.szName[i] == 0);
    TEST_CHECK(buf.plr.szNames[0] == 0);
    TEST_CHECK(app.state.idsFileError == idsGameFileAppearsCorruptUnableLoadFile);
    for (i = 0; i < (int)sizeof(buf.rgchGuard); i++)
        TEST_CHECK(buf.rgchGuard[i] == 0x55);

    // A compressed plural name of 32 characters, one more than a save holds.
    memset(szLong, 'a', 32);
    szLong[32] = 0;
    cOut = 64;
    const auto cbPacked3 = FCompressUserString(app, szLong, std::as_writable_bytes(std::span((char *)&rgb[iOff + 1], static_cast<std::size_t>(cOut))));
    if (cbPacked3) cOut = static_cast<std::int16_t>(*cbPacked3);
    TEST_ASSERT(cbPacked3);
    rgb[iOff] = (uint8_t)cOut;
    memset(&buf, 0x55, sizeof(buf));
    ReadRtPlr(app, &buf.plr, rgb, sizeof(rgb));
    TEST_CHECK(buf.plr.szNames[0] == 0);
    for (i = 0; i < (int)sizeof(buf.rgchGuard); i++)
        TEST_CHECK(buf.rgchGuard[i] == 0x55);
}

#ifdef STARS_UI_SDL
static INT_PTR idLegacyPasswordDialog;
static const char *pszLegacyPasswordDialog;
static INT_PTR LegacyPasswordDialog(HINSTANCE hinst, LPCSTR pszTemplate, HWND hwndParent, DLGPROC lpfn, LPARAM lParam) {
    HWND hwnd;

    if (idLegacyPasswordDialog < 0) return idLegacyPasswordDialog;
    hwnd = ui::CreateDialog(hinst, pszTemplate, hwndParent, lpfn, lParam);
    TEST_CHECK(hwnd != nullptr);
    if (!hwnd) return -1;
    if (idLegacyPasswordDialog > 0) {
        TEST_CHECK(ui::SetWindowText(win32::GetDlgItem(hwnd, IDC_EDIT1), pszLegacyPasswordDialog));
        TEST_CHECK(ui::SetWindowText(win32::GetDlgItem(hwnd, IDC_PASSWORD_CONFIRM), pszLegacyPasswordDialog));
    }
    ui::SendMessage(hwnd, win32::WM_COMMAND, win32::MAKEWPARAM(idLegacyPasswordDialog > 0 ? IDOK : IDCANCEL, win32::BN_CLICKED), 0);
    ui::DestroyWindow(hwnd);
    return idLegacyPasswordDialog;
}

static void test_LegacyPassword_owned_result(void) {
    APPSESSION &app = ui::app;
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    TEST_ASSERT(FStarsTestInit(app));
    CONTROLIO control;
    control.pfnDialogBox = LegacyPasswordDialog;
    CONTROLIOSCOPE controlScope(control);
    idLegacyPasswordDialog = 1;
    pszLegacyPasswordDialog = "1234567890123\xc3\xa9";
    const auto szFirst = FPromptLegacyRacePassword(app);
    TEST_ASSERT(szFirst);
    TEST_CHECK(std::string_view(szFirst->data()) == pszLegacyPasswordDialog);
    TEST_CHECK(szFirst->back() == 0);
    idLegacyPasswordDialog = 0;
    TEST_CHECK(!FPromptLegacyRacePassword(app));
    idLegacyPasswordDialog = -1;
    TEST_CHECK(!FPromptLegacyRacePassword(app));
    idLegacyPasswordDialog = 1;
    pszLegacyPasswordDialog = "new-\xc3\xa9";
    const auto szSecond = FPromptLegacyRacePassword(app);
    TEST_ASSERT(szSecond);
    TEST_CHECK(std::string_view(szSecond->data()) == pszLegacyPasswordDialog);
    TEST_CHECK(std::string_view(szFirst->data()) == "1234567890123\xc3\xa9");
}

static void test_NewPasswordDlg_utf8_limit(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    HWND hwnd;
    HWND hwndEdit;
    HWND hwndOrdinary;
    char szPass[20];
    std::array<char, 16> szReplacement{};
    const char *pszTooLong = "1234567890123456";
    const char *pszUnicodeTooLong = "\xf0\x9f\x98\x80\xf0\x9f\x98\x80\xf0\x9f\x98\x80\xf0\x9f\x98\x80";
    const char *pszExact = "1234567890123\xc3\xa9";
    int16_t idPlayerSave;
    int16_t fLogOffSave;

    TEST_ASSERT(FStarsTestInit(app));
    idPlayerSave = app.state.idPlayer;
    fLogOffSave = app.state.fLogOff;
    app.state.idPlayer = 0;
    app.state.fLogOff = 1;
    hwnd = ui::CreateDialog(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_NEW_PASSWORD), NULL, NewPasswordDlg, 0);
    TEST_ASSERT(hwnd != NULL);
    hwndEdit = win32::GetDlgItem(hwnd, IDC_EDIT1);
    TEST_ASSERT(hwndEdit != NULL);
    TEST_ASSERT(SDL_SetClipboardText(pszTooLong));
    ui::SendMessage(hwndEdit, win32::WM_PASTE, 0, 0);
    TEST_CHECK(win32::GetWindowTextA(hwndEdit, szPass, sizeof(szPass)) == 0);
    TEST_CHECK(StarsCompatTextInputRejected(hwndEdit));
    ui::SendMessage(hwndEdit, win32::WM_CHAR, '\b', 0);
    ui::SendMessage(hwndEdit, win32::WM_KEYDOWN, win32::VK_DELETE, 0);
    TEST_CHECK(StarsCompatTextInputRejected(hwndEdit));
    cStarsTestAlert = 0;
    ui::SendMessage(hwnd, win32::WM_COMMAND, win32::MAKEWPARAM(IDOK, win32::BN_CLICKED), (LPARAM)win32::GetDlgItem(hwnd, IDOK));
    TEST_CHECK(cStarsTestAlert == 1);
    TEST_CHECK(strstr(szStarsTestAlert, "enter it again") != NULL);
    TEST_CHECK(win32::IsWindow(hwnd));

    TEST_ASSERT(SDL_SetClipboardText(pszExact));
    ui::SendMessage(hwndEdit, win32::WM_PASTE, 0, 0);
    TEST_CHECK(win32::GetWindowTextA(hwndEdit, szPass, sizeof(szPass)) == 15);
    TEST_CHECK(FUtf8Valid(szPass));
    TEST_CHECK(!StarsCompatTextInputRejected(hwndEdit));

    hwndEdit = win32::GetDlgItem(hwnd, IDC_PASSWORD_CONFIRM);
    TEST_ASSERT(hwndEdit != NULL);
    TEST_ASSERT(SDL_SetClipboardText(pszUnicodeTooLong));
    ui::SendMessage(hwndEdit, win32::WM_PASTE, 0, 0);
    TEST_CHECK(win32::GetWindowTextA(hwndEdit, szPass, sizeof(szPass)) == 0);
    TEST_CHECK(StarsCompatTextInputRejected(hwndEdit));
    ui::SendMessage(hwndEdit, win32::WM_CHAR, '\b', 0);
    ui::SendMessage(hwndEdit, win32::WM_KEYDOWN, win32::VK_DELETE, 0);
    TEST_CHECK(StarsCompatTextInputRejected(hwndEdit));
    ui::SendMessage(hwnd, win32::WM_COMMAND, win32::MAKEWPARAM(IDOK, win32::BN_CLICKED), (LPARAM)win32::GetDlgItem(hwnd, IDOK));
    TEST_CHECK(cStarsTestAlert == 2);
    TEST_CHECK(strstr(szStarsTestAlert, "enter it again") != NULL);
    TEST_CHECK(win32::IsWindow(hwnd));

    TEST_ASSERT(SDL_SetClipboardText(pszExact));
    ui::SendMessage(hwndEdit, win32::WM_PASTE, 0, 0);
    TEST_CHECK(win32::GetWindowTextA(hwndEdit, szPass, sizeof(szPass)) == 15);
    TEST_CHECK(FUtf8Valid(szPass));
    TEST_CHECK(!StarsCompatTextInputRejected(hwndEdit));

    TEST_ASSERT(ui::SetWindowText(hwndEdit, "different"));
    ui::SendMessage(hwnd, win32::WM_COMMAND, win32::MAKEWPARAM(IDOK, win32::BN_CLICKED), (LPARAM)win32::GetDlgItem(hwnd, IDOK));
    TEST_CHECK(cStarsTestAlert == 3);
    TEST_CHECK(strstr(szStarsTestAlert, "not the same") != NULL);
    TEST_CHECK(win32::IsWindow(hwnd));
    TEST_ASSERT(ui::SetWindowText(hwndEdit, pszExact));
    ui::SendMessage(hwnd, win32::WM_COMMAND, win32::MAKEWPARAM(IDOK, win32::BN_CLICKED), (LPARAM)win32::GetDlgItem(hwnd, IDOK));
    TEST_CHECK(cStarsTestAlert == 3);
    TEST_CHECK(win32::IsWindow(hwnd));
    hwndOrdinary = ui::CreateWindowEx(0, "EDIT", "", win32::WS_CHILD, 0, 0, 100, 24, hwnd, NULL, app.lpui->hInst, NULL);
    TEST_ASSERT(hwndOrdinary != NULL);
    ui::SendMessage(hwndOrdinary, win32::EM_LIMITTEXT, 15, 0);
    TEST_ASSERT(SDL_SetClipboardText(pszTooLong));
    ui::SendMessage(hwndOrdinary, win32::WM_PASTE, 0, 0);
    TEST_CHECK(win32::GetWindowTextA(hwndOrdinary, szPass, sizeof(szPass)) == 15);
    TEST_CHECK(!StarsCompatTextInputRejected(hwndOrdinary));
    hwndEdit = ui::CreateDialog(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_NEW_PASSWORD), NULL, NewPasswordDlg,
                                  reinterpret_cast<LPARAM>(&szReplacement));
    TEST_ASSERT(hwndEdit != NULL);
    TEST_ASSERT(ui::SetWindowText(win32::GetDlgItem(hwndEdit, IDC_EDIT1), pszExact));
    TEST_ASSERT(ui::SetWindowText(win32::GetDlgItem(hwndEdit, IDC_PASSWORD_CONFIRM), pszExact));
    ui::SendMessage(hwndEdit, win32::WM_COMMAND, win32::MAKEWPARAM(IDOK, win32::BN_CLICKED), (LPARAM)win32::GetDlgItem(hwndEdit, IDOK));
    TEST_CHECK(strcmp(szReplacement.data(), pszExact) == 0);
    ui::DestroyWindow(hwndEdit);
    szReplacement[0] = 0;
    hwndEdit = ui::CreateDialog(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_NEW_PASSWORD), NULL, NewPasswordDlg,
                                  reinterpret_cast<LPARAM>(&szReplacement));
    TEST_ASSERT(hwndEdit != NULL);
    cStarsTestAlert = 0;
    ui::SendMessage(hwndEdit, win32::WM_COMMAND, win32::MAKEWPARAM(IDOK, win32::BN_CLICKED), (LPARAM)win32::GetDlgItem(hwndEdit, IDOK));
    TEST_CHECK(cStarsTestAlert == 1);
    TEST_CHECK(strstr(szStarsTestAlert, "replacement password is required") != NULL);
    TEST_CHECK(win32::IsWindow(hwndEdit));
    ui::DestroyWindow(hwndEdit);
    app.state.idPlayer = idPlayerSave;
    app.state.fLogOff = fLogOffSave;
    ui::DestroyWindow(hwnd);
}

// Random race: the Random template is only the random-race bit, which
// GenerateWorld replaces with a random race. Choosing Custom on the first
// wizard page keeps the race being edited, so a race that had been Random
// kept the bit and was still replaced, however it was customized.
static void test_RaceWizardDlg1_custom_clears_random(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    HWND hwnd;
    HWND hwndCtl;

    TEST_ASSERT(FStarsTestInit(app));
    app.state.vplr = vrgplrDef[6];
    TEST_ASSERT(GetRaceGrbit(&app.state.vplr, ibitRaceAIPlayer) != 0);
    strcpy(app.state.vplr.szName, "Randomite");
    strcpy(app.state.vplr.szNames, "Randomites");
    hwnd = ui::CreateDialog(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_RACE_WIZARD_1), NULL, RaceWizardDlg1, 0);
    TEST_ASSERT(hwnd != NULL);
    win32::CheckRadioButton(hwnd, IDC_RACE_HUMANOID, IDC_RACE_CUSTOM, IDC_RACE_CUSTOM);
    hwndCtl = win32::GetDlgItem(hwnd, IDC_RACE_CUSTOM);
    ui::SendMessage(hwnd, win32::WM_COMMAND, win32::MAKEWPARAM(IDC_RACE_CUSTOM, win32::BN_CLICKED), (LPARAM)hwndCtl);
    ui::SendMessage(hwnd, win32::WM_COMMAND, win32::MAKEWPARAM(IDC_NEXT, win32::BN_CLICKED), (LPARAM)win32::GetDlgItem(hwnd, IDC_NEXT));
    ui::DestroyWindow(hwnd);
    TEST_CHECK_(GetRaceGrbit(&app.state.vplr, ibitRaceAIPlayer) == 0, "the custom race is still random");
}

static void test_RaceWizardDlg2_paints_habitat(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    RECT rgrc[17];
    HWND hwnd;
    HDC hdc;
    int16_t i;

    TEST_ASSERT(FStarsTestInit(app));
    hdc = win32::GetDC(NULL);
    TEST_ASSERT(hdc != NULL);
    TEST_ASSERT(FCreateFonts(app, hdc));
    win32::ReleaseDC(NULL, hdc);
    app.state.vplr = vrgplrDef[0];
    app.lpui->vrgrcRCW = rgrc;
    app.lpui->iPanelActive = rwPageHabitability;
    app.state.fRCWReadOnly = 0;
    hwnd = ui::CreateDialog(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_RACE_WIZARD_2), NULL, RaceWizardDlg2, 0);
    TEST_ASSERT(hwnd != NULL);
    TEST_CHECK(app.lpui->crcRCW == 17);
    ui::ShowWindow(hwnd, win32::SW_SHOW);
    TEST_CHECK(ui::UpdateWindow(hwnd));
    for (i = 0; i < 3; i++) {
        win32::CheckDlgButton(hwnd, IDC_IMMUNE_TO_GRAVITY + i, win32::BST_CHECKED);
        ui::SendMessage(hwnd, win32::WM_COMMAND, win32::MAKEWPARAM(IDC_IMMUNE_TO_GRAVITY + i, win32::BN_CLICKED),
                    (LPARAM)win32::GetDlgItem(hwnd, IDC_IMMUNE_TO_GRAVITY + i));
        TEST_CHECK(app.state.vplr.rgEnvVarMax[i] == envImmune);
    }
    DrawRace2(app, hwnd, NULL, -1);
    for (i = 0; i < 3; i++) {
        win32::CheckDlgButton(hwnd, IDC_IMMUNE_TO_GRAVITY + i, win32::BST_UNCHECKED);
        ui::SendMessage(hwnd, win32::WM_COMMAND, win32::MAKEWPARAM(IDC_IMMUNE_TO_GRAVITY + i, win32::BN_CLICKED),
                    (LPARAM)win32::GetDlgItem(hwnd, IDC_IMMUNE_TO_GRAVITY + i));
        TEST_CHECK(app.state.vplr.rgEnvVarMin[i] == 20 && app.state.vplr.rgEnvVarMax[i] == 80);
    }
    win32::InvalidateRect(hwnd, NULL, 1);
    TEST_CHECK(ui::UpdateWindow(hwnd));
    ui::DestroyWindow(hwnd);
    app.lpui->vrgrcRCW = NULL;
}
#endif

static void test_IRaceChecksum_wire_image(void) {
    std::byte rgb[192];
    PLAYER plr{};
    for (int i = 0; i < 192; i++) rgb[i] = static_cast<std::byte>((i * 73 + 11) & 255);
    TEST_ASSERT(FDecodePlayer(rgb, plr));
    TEST_CHECK(IRaceChecksum(&plr) == 0x80c0);
}

TEST_LIST = {{"race file owns its filename across shared-buffer callbacks", test_RaceFile_owned_input},
             {"race file admits whole UTF-8 paths and bounds diagnostic text", test_RaceFile_input_bounds},
             {"race checksum uses the fixed 192-byte wire image", test_IRaceChecksum_wire_image},
             {"race attribute high bit", test_RaceGrbit_high_bit},
             {"LInnateRaceHabitability binary64 rounding", test_LInnateRaceHabitability_double_rounding},
             {"FWasRaceFile imports unprotected legacy race designs", test_FWasRaceFile_imports_unprotected_legacy_designs},
             {"FWasRaceFile replaces protected legacy passwords and confirms name shortening", test_FWasRaceFile_legacy_replaces_protected_password_and_confirms_shortening},
             {"IRaceChecksum ignores stale name bytes", test_IRaceChecksum_ignores_stale_name_bytes},
             {"ReadRtPlr bounds record lengths", test_ReadRtPlr_bounds_record_lengths},
#ifdef STARS_UI_SDL
             {"legacy password prompt returns owned text or cancellation", test_LegacyPassword_owned_result},
             {"NewPasswordDlg keeps Unicode passwords within the login limit", test_NewPasswordDlg_utf8_limit},
             {"RaceWizardDlg1 Custom clears the random-race bit", test_RaceWizardDlg1_custom_clears_random},
             {"RaceWizardDlg2 paints habitat bars", test_RaceWizardDlg2_paints_habitat},
#endif
             {NULL, NULL}};

#ifdef STARS_UI_SDL
#include "player_choice_io.h"
static const PLAYERCHOICEIO ioRace = [] { PLAYERCHOICEIO io; io.pfnFPromptLegacyRacePassword = FStarsTestPromptLegacyRacePassword; return io; }();
static PLAYERCHOICEIOSCOPE raceChoiceScope(ioRace);
#endif
