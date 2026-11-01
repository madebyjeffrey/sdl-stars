#include "acutest.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#include "control_io.h"
#include "reportui.h"
#endif
#include "callbackcontext.h"
[[maybe_unused]] static char szTestLiteral1[] = "\x5cw \x5ci \x5cu";
[[maybe_unused]] static char szTestLiteral2[] = "\x5ch";
#include "stars_test.h"
#include "text_io.h"
#include <iterator>

static void TestCatalogOwnership(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    char sz[64];
    TEST_ASSERT(FStarsTestInit(app));
    const auto szFirst = TextIO().pfnCompressed(app, idsHave2);
    const auto szSecond = TextIO().pfnCompressed(app, idsAre);
    CchSprintf(sz, "%s|%s", szFirst, szSecond);
    TEST_CHECK(strcmp(sz, " have| are") == 0);
    CchSprintf(sz, "%s|%s", TextIO().pfnCompressed(app, idsHave2), TextIO().pfnCompressed(app, idsAre));
    TEST_CHECK(strcmp(sz, " have| are") == 0);
}

// Independent reference: UTF-8 bytes from text/strings.txt, including empty
// IDs, hashed with their little-endian 16-bit IDs and terminating NULs.
static void TestCatalogBytes(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    uint64_t uHash = 14695981039346656037ULL;
    char sz[256];
    TEST_ASSERT(FStarsTestInit(app));
    for (uint16_t ids = 0; ids <= idsLibraryZipNameEncoding; ids++) {
        const auto cch = CchGetString(app, static_cast<StringId>(ids), std::span{sz});
        TEST_CHECK(cch >= 0 && cch < 256 && FUtf8Valid(sz, cch));
        for (uint8_t b : {static_cast<uint8_t>(ids), static_cast<uint8_t>(ids >> 8)})
            uHash = (uHash ^ b) * 1099511628211ULL;
        for (int16_t i = 0; i <= cch; i++)
            uHash = (uHash ^ static_cast<uint8_t>(sz[i])) * 1099511628211ULL;
    }
    TEST_CHECK(uHash == 0xb7fccea32b3950e5ULL);
}

static std::array<char, cchCatalogText> PszTestCatalog(APPSESSION &, StringId) {
    std::array<char, cchCatalogText> sz{};
    CchCopyUtf8Prefix(std::span{sz}, "ab\xc3\xa9\xf0\x9f\x98\x80z");
    return sz;
}

static void TestEncoding(void) {
    char sz[32], szSmall[4];
    const char *rgszBad[] = {"\xc0\xaf", "\xe2\x82", "\xed\xa0\x80", "\xf4\x90\x80\x80", "a\xff"};
    const unsigned char rgbUtf16[] = {0xa9, 0, 0x3d, 0xd8, 0, 0xde};
    const unsigned char rgbBadUtf16[] = {0x3d, 0xd8};
    const unsigned char rgbCp1252[] = {0x52, 0xe9, 0x80, 0x99};
    size_t j;
    TEST_CHECK(CchCopyUtf8(sz, sizeof(sz), "\xc3\xa9\xe2\x82\xac", 5) == 5);
    TEST_CHECK(strcmp(sz, "\xc3\xa9\xe2\x82\xac") == 0);
    TEST_CHECK(CchCopyUtf8(szSmall, 1, "a", 1) == -3 && szSmall[0] == 0);
    TEST_CHECK(CchCopyUtf8(szSmall, sizeof(szSmall), "\xf0\x9f\x98\x80", 4) == -3 && szSmall[0] == 0);
    TEST_CHECK(CchUtf16leToUtf8(sz, sizeof(sz), rgbUtf16, 3) == 6);
    TEST_CHECK(strcmp(sz, "\xc2\xa9\xf0\x9f\x98\x80") == 0);
    TEST_CHECK(CchCp1252ToUtf8(sz, sizeof(sz), rgbCp1252, sizeof(rgbCp1252)) == 9);
    TEST_CHECK(strcmp(sz, "R\xc3\xa9\xe2\x82\xac\xe2\x84\xa2") == 0);
    TEST_CHECK(CchCp1252ToUtf8(szSmall, sizeof(szSmall), rgbCp1252, sizeof(rgbCp1252)) == -3 && szSmall[0] == 0);
    TEST_CHECK(CchUtf16leToUtf8(sz, sizeof(sz), rgbBadUtf16, 1) == -1 && sz[0] == 0);
    for (j = 0; j < sizeof(rgszBad) / sizeof(*rgszBad); j++) {
        strcpy(szSmall, "old");
        TEST_CHECK(!FUtf8Valid(rgszBad[j], strlen(rgszBad[j])));
        TEST_CHECK(CchCopyUtf8(szSmall, sizeof(szSmall), rgszBad[j], strlen(rgszBad[j])) == -1);
        TEST_CHECK(szSmall[0] == 0);
    }
    TEST_CHECK(!FUtf8Valid("a\0b", 3));
    TEST_CHECK(CchUtf8Prefix("12345678901234\xce\xa9.r1", 15) == 14);
    TEST_CHECK(CchUtf8Prefix("\xf0\x9f\x98\x80", 3) == 0);
    TEST_CHECK(CchUtf8Prefix("\xf0\x9f\x98\x80", 4) == 4);
    TEST_CHECK(CchUtf8Prefix("a\xffz", 2) == 1);
}

static void TestEnvironmentText(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    const char *psz;
    for (int16_t i = 0; i <= 100; i++) {
        psz = PszCalcEnvVar(app, Temperature, i);
        TEST_CHECK(FUtf8Valid(psz, strlen(psz)));
        TEST_CHECK(!strcmp(psz + strlen(psz) - 3, "ºC"));
    }
    TEST_CHECK(!strcmp(PszCalcEnvVar(app, Temperature, 0), "-200ºC"));
    TEST_CHECK(!strcmp(PszCalcEnvVar(app, Temperature, 50), "0ºC"));
    TEST_CHECK(!strcmp(PszCalcEnvVar(app, Temperature, 100), "200ºC"));
    TEST_CHECK(!strcmp(PszCalcEnvVar(app, Gravity, 50), "1.00g"));
    TEST_CHECK(!strcmp(PszCalcEnvVar(app, Radiation, 50), "50mR"));
}

static void TestDisplayCopyBounds(void) {
    const char szText[] = "ab\xc3\xa9\xf0\x9f\x98\x80z";
    const size_t rgcch[] = {0, 0, 1, 2, 2, 4, 4, 4, 4, 8, 9, 9, 9};
    char rgb[16], expected[16];
    size_t cb, cch, cchBefore;
    for (cb = 0; cb < std::size(rgcch); cb++) {
        memset(rgb, '!', sizeof(rgb));
        memset(expected, '!', sizeof(expected));
        TEST_CHECK(CchCopyUtf8Prefix(std::span{rgb}.subspan(2, cb), szText) == rgcch[cb]);
        if (cb != 0) {
            memcpy(expected + 2, szText, rgcch[cb]);
            expected[2 + rgcch[cb]] = 0;
        }
        TEST_CHECK(!memcmp(rgb, expected, sizeof(rgb)));
        memset(rgb, '!', sizeof(rgb));
        memset(expected, '!', sizeof(expected));
        cchBefore = CchCopyUtf8Prefix(std::span{rgb}.subspan(2, cb), "Q");
        TEST_CHECK(cchBefore == (cb >= 2 ? 1u : 0u));
        cch = rgcch[cb - cchBefore];
        TEST_CHECK(CchAppendUtf8Prefix(std::span{rgb}.subspan(2, cb), szText) == cch);
        if (cb != 0) {
            if (cchBefore != 0) expected[2] = 'Q';
            memcpy(expected + 2 + cchBefore, szText, cch);
            expected[2 + cchBefore + cch] = 0;
            TEST_CHECK(FUtf8Valid(rgb + 2, cchBefore + cch));
        }
        TEST_CHECK(!memcmp(rgb, expected, sizeof(rgb)));
    }
    const char rgch[] = {'a', 'b', '\xc3', '\xa9'};
    TEST_CHECK(CchCopyUtf8Prefix(rgb, std::string_view(rgch, sizeof(rgch))) == 4 && !strcmp(rgb, "ab\xc3\xa9"));
    TEST_CHECK(CchCopyUtf8Prefix(rgb, "ab\xffz") == 2 && !strcmp(rgb, "ab"));
    TEST_CHECK(CchCopyUtf8Prefix(rgb, std::string_view("a\0b", 3)) == 1 && !strcmp(rgb, "a"));
    strcpy(rgb, "ab\xc3\xa9z");
    TEST_CHECK(CchCopyUtf8Prefix(std::span{rgb}.subspan(1), std::string_view(rgb, 5)) == 5 && !strcmp(rgb, "aab\xc3\xa9z"));
    strcpy(rgb, "abc");
    TEST_CHECK(CchAppendUtf8Prefix(rgb, std::string_view(rgb, 3)) == 3 && !strcmp(rgb, "abcabc"));
    memset(rgb, '!', sizeof(rgb));
    TEST_CHECK(CchAppendUtf8Prefix(std::span{rgb}.subspan(2, 4), "x") == 0 && rgb[2] == 0);
    TEST_CHECK(rgb[1] == '!' && rgb[3] == '!' && rgb[5] == '!' && rgb[6] == '!');
    TEST_CHECK(CchCopyUtf8Prefix(std::span<char>{}, std::string_view{}) == 0);
    TEST_CHECK(CchAppendUtf8Prefix(std::span<char>{}, std::string_view{}) == 0);
}

static void TestPasswords(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szCmd[] = "-p\xc3\xa9 game.hst";
    char szUtf8[] = "-p\xe6\x9d\xb1\xe4\xba\xac game.hst";
    char szBad[] = "-p\xed\xa0\x80 game.hst";
    char szTooLong[] = "-p\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9 game.hst";
    char szPath[sizeof(app.state.szBase) + 4];
    int16_t fSeed = 0;
    uint32_t lSeed = 0;
    TEST_ASSERT(FStarsTestInit(app));
    TEST_CHECK(ParseCmdLine(app, szCmd, &fSeed, &lSeed));
    TEST_CHECK(strcmp(app.state.szPassLast, "\xc3\xa9") == 0);
    TEST_CHECK(app.state.lSaltLast == 5307 && app.state.lSaltLast == LSaltFromSz(app.state.szPassLast) && strcmp(app.state.szBase, "game.hst") == 0);
    TEST_CHECK(ParseCmdLine(app, szUtf8, &fSeed, &lSeed));
    TEST_CHECK(strcmp(app.state.szPassLast, "\xe6\x9d\xb1\xe4\xba\xac") == 0 && app.state.lSaltLast == LSaltFromSz(app.state.szPassLast));
    TEST_CHECK(!ParseCmdLine(app, szBad, &fSeed, &lSeed));
    TEST_CHECK(app.state.szBase[0] == 0 && !app.state.ini.fCmdLine && cStarsTestAlert == 1);
    TEST_CHECK(!ParseCmdLine(app, szTooLong, &fSeed, &lSeed));
    TEST_CHECK(app.state.szBase[0] == 0 && !app.state.ini.fCmdLine && cStarsTestAlert == 2);
    memset(szPath, 'x', sizeof(szPath) - 1); szPath[sizeof(szPath) - 1] = 0;
    TEST_CHECK(!ParseCmdLine(app, szPath, &fSeed, &lSeed));
    TEST_CHECK(app.state.szBase[0] == 0 && !app.state.ini.fCmdLine && cStarsTestAlert == 3);
    SetSzRaceFile(app, "12345678901234\xce\xa9.r1");
    TEST_CHECK(strcmp(app.state.szRaceFile, "12345678901234") == 0);
}

static void TestInvalidDefinition(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[cbTestPath], szFile[cbTestPath + 64];
    char szPadding[1024];
    NATIVEFILE *pf;
    int i;
    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("utf8_invalid_definition", szDir, sizeof(szDir)));
    snprintf(szFile, sizeof(szFile), "%s%sbad.def", szDir, szDirSep);
    pf = PfNativeOpen(szFile, "wb"); TEST_ASSERT(pf);
    IPutNativeString("\xe9\r\n", pf); ICloseNativeFile(pf);
    TEST_CHECK(!GenNewGameFromFile(app, szFile));
    TEST_CHECK(!FStarsRecoveryActive() && app.state.lpbDefUni == NULL);
    pf = PfNativeOpen(szFile, "wb"); TEST_ASSERT(pf);
    IPutNativeString("Title\r\n\xff\r\n", pf); ICloseNativeFile(pf);
    TEST_CHECK(!GenNewGameFromFile(app, szFile));
    TEST_CHECK(app.state.idsFileError == idsUniverseCreationFileAppearsInvalid);
    TEST_CHECK(!FStarsRecoveryActive() && app.state.lpbDefUni == NULL);
    pf = PfNativeOpen(szFile, "wb"); TEST_ASSERT(pf);
    IPutNativeString("Title\r\n", pf);
    memset(szPadding, 'a', sizeof(szPadding));
    for (i = 0; i < 64; i++) TEST_CHECK(CWriteNative(std::span{szPadding}, pf) == sizeof(szPadding));
    ICloseNativeFile(pf);
    TEST_CHECK(!GenNewGameFromFile(app, szFile));
    TEST_CHECK(app.state.idsFileError == idsUniverseCreationFileAppearsInvalid);
    TEST_CHECK(!FStarsRecoveryActive() && app.state.lpbDefUni == NULL);
    pf = PfNativeOpen(szFile, "wb"); TEST_ASSERT(pf);
    TEST_CHECK(CWriteNative(std::span{"Title\0hidden\r\n"}.first(14), pf) == 14); ICloseNativeFile(pf);
    TEST_CHECK(!GenNewGameFromFile(app, szFile));
    TEST_CHECK(app.state.idsFileError == idsUniverseCreationFileAppearsInvalid);
    TEST_CHECK(!FStarsRecoveryActive() && app.state.lpbDefUni == NULL);
}

static void TestFormattedTextBounds(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szFormat[1026];
    char sz[1024];
    char *psz;
    int16_t rgParams[2] = {-123, 55};

    TEST_ASSERT(FStarsTestInit(app));
    strcpy(app.state.szWork, "World");
    TEST_CHECK(!strcmp(PszFormatString(app, szTestLiteral1, rgParams), "World -123 55"));
    memset(szFormat, 'a', 252);
    memcpy(szFormat + 252, "\xf0\x9f\x98\x80 suffix", 12);
    psz = PszFormatString(app, szFormat, NULL);
    TEST_CHECK(strlen(psz) == 252 && FUtf8Valid(psz, strlen(psz)));
    memset(app.state.szBase, 'a', sizeof(app.state.szBase) - 1); app.state.szBase[sizeof(app.state.szBase) - 1] = 0;
    psz = PszFormatString(app, szTestLiteral2, NULL);
    TEST_CHECK(strlen(psz) < sizeof(app.state.szMsgBuf) && FUtf8Valid(psz, strlen(psz)));
    memset(szFormat, 'a', 1022);
    memcpy(szFormat + 1022, "\xc3\xa9z", 4);
    TEST_CHECK(CchSprintf(sz, "%s", szFormat) == 1022);
    TEST_CHECK(strlen(sz) == 1022 && FUtf8Valid(sz, strlen(sz)));
    char rgch[12];
    memset(rgch, '!', sizeof(rgch));
    TEST_CHECK(CchSprintf(std::span{rgch}.subspan(2, 5), "%s", "ab\xc3\xa9z") == 4);
    TEST_CHECK(!strcmp(rgch + 2, "ab\xc3\xa9") && rgch[1] == '!' && rgch[7] == '!');
    memset(rgch, '!', sizeof(rgch));
    TEST_CHECK(CchSprintf(std::span{rgch}.subspan(2, 4), "%s", "ab\xc3\xa9z") == 2);
    TEST_CHECK(!strcmp(rgch + 2, "ab") && rgch[1] == '!' && rgch[6] == '!');
    TEST_CHECK(CchSprintf(std::span{rgch}.subspan(2, 1), "%d", 12345) == 0 && rgch[2] == 0 && rgch[3] == 'b');
    rgch[2] = '!';
    TEST_CHECK(CchSprintf(std::span<char>{}, "%d", 12345) == 0 && rgch[2] == '!');
    TEST_CHECK(CchSprintf(sz, "%ld|%lu|%08lx|%hd|%.2f", std::int32_t{INT32_MIN}, std::uint32_t{UINT32_MAX},
                         std::uint32_t{0x89abcdef}, -1, 3.25) == 39);
    TEST_CHECK(!strcmp(sz, "-2147483648|4294967295|89abcdef|-1|3.25"));
    struct { signed int c : 5; unsigned int u : 5; } bits = {-3, 29};
    TEST_CHECK(CchSprintf(sz, "%d %u", bits.c, bits.u) == 5 && !strcmp(sz, "-3 29"));
    errno = 0;
    TEST_CHECK(CchSprintf(sz, "%d") == 0 && sz[0] == 0 && errno == EINVAL);
    TEXTIO io;
    io.pfnCompressed = PszTestCatalog;
    TEXTIOSCOPE textScope(io);
    const std::size_t rgcch[] = {0, 0, 1, 2, 2, 4, 4, 4, 4, 8, 9};
    char rgb[16];
    for (std::size_t cb = 0; cb < std::size(rgcch); cb++) {
        memset(rgb, '!', sizeof(rgb));
        TEST_CHECK(CchGetString(app, idsName, std::span{rgb}.subspan(2, cb)) == static_cast<std::int16_t>(rgcch[cb]));
        TEST_CHECK(!memcmp(rgb + 2, PszTestCatalog(app, idsName).data(), rgcch[cb]));
        if (cb != 0) {
            TEST_CHECK(rgb[2 + rgcch[cb]] == 0);
            TEST_CHECK(FUtf8Valid(rgb + 2, rgcch[cb]));
        }
        TEST_CHECK(rgb[0] == '!' && rgb[1] == '!');
        for (std::size_t i = 2 + cb; i < sizeof(rgb); i++) TEST_CHECK(rgb[i] == '!');
    }
}

#ifdef STARS_UI_SDL
static int cHistoryMenus;
static int16_t CchHistoryCatalog(APPSESSION &, StringId ids, std::span<char> sz) {
    static const char szLong[] = "\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9"
                                 "\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9 ";
    return static_cast<int16_t>(CchCopyUtf8Prefix(sz, ids == idsHistory ? "History " : szLong));
}
static int16_t CheckHistoryRows(APPSESSION &app, HWND, int16_t, int16_t, int16_t cString, int32_t *,
                                const char *const *rgsz, int16_t, int16_t) {
    TEST_CHECK(cString == 8);
    for (int i = 0; i < cString; i++) {
        TEST_CHECK(rgsz[i] == app.state.szWork + i * 40);
        TEST_CHECK(strlen(rgsz[i]) == 36 && FUtf8Valid(rgsz[i], 36));
        TEST_CHECK(!strncmp(rgsz[i], "History ", 8));
        TEST_CHECK(rgsz[i][39] == '!');
    }
    cHistoryMenus++;
    return -1;
}
static void TestHistoryRows(void) {
    APPSESSION &app = ui::app;
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    TEST_ASSERT(FStarsTestInit(app));
    TEXTIO text;
    text.pfnCchGetString = CchHistoryCatalog;
    TEXTIOSCOPE textScope(text);
    CONTROLIO control;
    control.pfnPopupMenu = CheckHistoryRows;
    CONTROLIOSCOPE controlScope(control);
    app.lpui->dyArial10 = 20;
    app.state.dyArial8 = 10;
    app.state.gd.fScoreVictory = 2;
    memset(app.state.szWork, '!', sizeof(app.state.szWork));
    cHistoryMenus = 0;
    ScoreXDlg(nullptr, win32::WM_LBUTTONDOWN, 0, 5 << 16);
    TEST_CHECK(cHistoryMenus == 1);
    for (size_t i = 320; i < sizeof(app.state.szWork); i++) TEST_CHECK(app.state.szWork[i] == '!');
}

static void TestScannerSnapshotUtf8(void) {
    SCANSNAPSHOT snap = {};
    char sz[2048];
    size_t cb;
    NATIVEFILE *pf;
    const char *psz = "\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80";

    snap.rgobj.reset(static_cast<SCANOBJECT *>(calloc(1, sizeof(SCANOBJECT))));
    TEST_ASSERT(snap.rgobj);
    SCANOBJECT &obj = snap.rgobj[0];
    snprintf(obj.szName, sizeof(obj.szName), "Quoted \"%s\"\r\n", psz);
    snap.cObject = 1;
    pf = PfNativeTemporary(); TEST_ASSERT(pf);
    TEST_ASSERT(FWriteScannerSnapshot(&snap, pf));
    ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::begin); cb = CReadNative(std::span{sz}.first(sizeof(sz) - 1), pf); sz[cb] = 0;
    TEST_CHECK(FUtf8Valid(sz, cb));
    TEST_CHECK(strstr(sz, psz) != NULL && strstr(sz, "\\u000d\\u000a") != NULL);
    strcpy(obj.szName, "\xff");
    TEST_CHECK(!FWriteScannerSnapshot(&snap, pf));
    memset(obj.szName, 'a', sizeof(obj.szName));
    TEST_CHECK(!FWriteScannerSnapshot(&snap, pf));
    ICloseNativeFile(pf);
}
#endif

TEST_LIST = {{"formatted text stays bounded and preserves UTF-8", TestFormattedTextBounds},
             {"catalog values survive subsequent lookups", TestCatalogOwnership},
             {"decoded catalog matches every source byte", TestCatalogBytes},
             {"environment labels contain UTF-8 units", TestEnvironmentText},
             {"display copy and append preserve byte limits and UTF-8", TestDisplayCopyBounds},
#ifdef STARS_UI_SDL
             {"history menu labels stay within their logical rows", TestHistoryRows},
             {"scanner snapshots preserve UTF-8 and reject malformed names", TestScannerSnapshotUtf8},
#endif
             {"invalid UTF-8 definition cleans up the error handler", TestInvalidDefinition},
             {"strict UTF-8 text and UTF-16 boundaries", TestEncoding},
             {"UTF-8 passwords and filename byte capacities", TestPasswords}, {NULL, NULL}};
