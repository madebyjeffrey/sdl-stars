#include "acutest.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
[[maybe_unused]] static char szTestLiteral1[] = "\x5cw \x5ci \x5cu";
[[maybe_unused]] static char szTestLiteral2[] = "\x5ch";
#include "stars_test.h"
#include "text_io.h"
#include <iterator>

static char *PszTestCatalog(APPSESSION &, StringId) {
    static char sz[] = "ab\xc3\xa9\xf0\x9f\x98\x80z";
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
    for (i = 0; i < 64; i++) TEST_CHECK(CWriteNative(szPadding, 1, sizeof(szPadding), pf) == sizeof(szPadding));
    ICloseNativeFile(pf);
    TEST_CHECK(!GenNewGameFromFile(app, szFile));
    TEST_CHECK(app.state.idsFileError == idsUniverseCreationFileAppearsInvalid);
    TEST_CHECK(!FStarsRecoveryActive() && app.state.lpbDefUni == NULL);
    pf = PfNativeOpen(szFile, "wb"); TEST_ASSERT(pf);
    TEST_CHECK(CWriteNative("Title\0hidden\r\n", 1, 14, pf) == 14); ICloseNativeFile(pf);
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
    TEXTIO io;
    io.pfnCompressed = PszTestCatalog;
    TEXTIOSCOPE textScope(io);
    const std::size_t rgcch[] = {0, 0, 1, 2, 2, 4, 4, 4, 4, 8, 9};
    char rgb[16];
    for (std::size_t cb = 0; cb < std::size(rgcch); cb++) {
        memset(rgb, '!', sizeof(rgb));
        TEST_CHECK(CchGetString(app, idsName, std::span{rgb}.subspan(2, cb)) == static_cast<std::int16_t>(rgcch[cb]));
        TEST_CHECK(!memcmp(rgb + 2, PszTestCatalog(app, idsName), rgcch[cb]));
        if (cb != 0) {
            TEST_CHECK(rgb[2 + rgcch[cb]] == 0);
            TEST_CHECK(FUtf8Valid(rgb + 2, rgcch[cb]));
        }
        TEST_CHECK(rgb[0] == '!' && rgb[1] == '!');
        for (std::size_t i = 2 + cb; i < sizeof(rgb); i++) TEST_CHECK(rgb[i] == '!');
    }
}

#ifdef STARS_UI_SDL
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
    ISeekNativeFile(pf, 0, 0); cb = CReadNative(sz, 1, sizeof(sz) - 1, pf); sz[cb] = 0;
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
#ifdef STARS_UI_SDL
             {"scanner snapshots preserve UTF-8 and reject malformed names", TestScannerSnapshotUtf8},
#endif
             {"invalid UTF-8 definition cleans up the error handler", TestInvalidDefinition},
             {"strict UTF-8 text and UTF-16 boundaries", TestEncoding},
             {"UTF-8 passwords and filename byte capacities", TestPasswords}, {NULL, NULL}};
