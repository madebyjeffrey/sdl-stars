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
#include "version.h"
#include <string>
#include <type_traits>

static_assert(std::is_same_v<decltype(STARS_VERSION_MAJOR), const int>);
static_assert(std::is_same_v<decltype(STARS_VERSION_MINOR), const int>);
static_assert(std::is_same_v<decltype(STARS_VERSION_PATCH), const int>);
static_assert(std::is_same_v<decltype(STARS_VERSION_BUILD), const int>);
static_assert(std::is_array_v<decltype(STARS_VERSION_STRING)>);
static_assert(std::is_array_v<decltype(STARS_VERSION_DISPLAY)>);
static_assert(std::is_array_v<decltype(STARS_VERSION_COMMIT)>);
static_assert(std::is_array_v<decltype(STARS_VERSION_PLATFORM)>);

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

// Independent source bytes include holes, literal backslashes and NULs;
// IDs are hashed as little-endian 16-bit values before each message.
static void TestMessageCatalogBytes(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    std::uint64_t uHash = 14695981039346656037ULL;
    TEST_ASSERT(FStarsTestInit(app));
    app.state.iLastMsgGet = -1;
    for (std::uint16_t id = 0; id < 387; id++) {
        const auto szMessage = PszGetCompressedMessage(app, static_cast<MessageId>(id));
        const auto *psz = szMessage.data();
        TEST_CHECK(std::strlen(psz) < sizeof(app.state.szLastMsgGet));
        TEST_CHECK(FUtf8Valid(psz, std::strlen(psz)));
        for (std::uint8_t b : {static_cast<std::uint8_t>(id), static_cast<std::uint8_t>(id >> 8)})
            uHash = (uHash ^ b) * 1099511628211ULL;
        for (std::size_t i = 0; i <= std::strlen(psz); i++)
            uHash = (uHash ^ static_cast<std::uint8_t>(psz[i])) * 1099511628211ULL;
        TEST_CHECK(std::string_view(app.state.szLastMsgGet) == psz);
    }
    TEST_CHECK(uHash == 0x340443fddde7c605ULL);
    CchCopyUtf8Prefix(std::span{app.state.szLastMsgGet}, "Cache-\xce\xa9");
    CchCopyUtf8Prefix(std::span{app.state.szMsgBuf}, "unchanged message");
    CchCopyUtf8Prefix(std::span{app.state.szWork}, "unchanged work");
    app.state.iLastMsgGet = 127;
    TEST_CHECK(std::string_view(PszGetCompressedMessage(app, static_cast<MessageId>(127)).data()) == "Cache-\xce\xa9");
    TEST_CHECK(std::string_view(app.state.szMsgBuf) == "unchanged message");
    TEST_CHECK(std::string_view(app.state.szWork) == "unchanged work");
    app.state.iLastMsgGet = -1;
}

static std::string SzMessageBytes(const char *psz) { return psz; }
static void TestMessageAndVersionText(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    const char *pszBattle = "You have received one battle recording this year.";
    std::int16_t rgParams[2] = {-123, 55};
    TEST_ASSERT(FStarsTestInit(app));
    CchCopyUtf8Prefix(std::span{app.state.szWork}, "World-\xce\xa9");
    const auto szFormat = PszFormatString(app, "\\w \\i \\u", rgParams);
    const auto szCompressed = PszGetCompressedMessage(app, idmHaveReceivedOneBattleRecordingYear);
    TEST_CHECK(SzMessageBytes(PszFormatString(app, "\\w \\i \\u", rgParams).data()) == "World-\xce\xa9 -123 55");
    TEST_CHECK(std::string_view(app.state.szMsgBuf) == "World-\xce\xa9 -123 55");
    TEST_CHECK(SzMessageBytes(PszFormatMessage(app, idmHaveReceivedOneBattleRecordingYear, nullptr).data()) == pszBattle);
    TEST_CHECK(std::string_view(app.state.szMsgBuf) == pszBattle);
    const auto szExpanded = PszFormatMessage(app, idmHaveReceivedOneBattleRecordingYear, nullptr);
    TEST_CHECK(SzMessageBytes(PszFormatIds(app, idsNewTurnAvailable, nullptr).data()) == "The new turn is available.");
    const auto szIds = TextIO().pfnFormat(app, idsNewTurnAvailable, nullptr);
    TEST_CHECK(std::string_view(app.state.szWork) == "World-\xce\xa9");
    ResetMessages(app);
    TEST_CHECK(SzMessageBytes(PszGetMessageN(app, -1).data()).empty());
    TEST_CHECK(app.state.szMsgBuf[0] == 0);
    TEST_ASSERT(FSendPlrMsg2XGen(app, 0, idmHaveReceivedOneBattleRecordingYear, gotoNone, 0, 0));
    TEST_CHECK(SzMessageBytes(PszGetMessageN(app, 0).data()) == pszBattle);
    const auto szQueued = PszGetMessageN(app, 0);
    TEST_CHECK(SzMessageBytes(PszGetMessageN(app, 1).data()).empty());
    TEST_CHECK(SzMessageBytes(SzVersion(app).data()) == std::string("Version ") + STARS_VERSION_DISPLAY);
    TEST_CHECK(std::string_view(app.state.szWork) == std::string("Version ") + STARS_VERSION_DISPLAY);
    const auto szVersion = SzVersion(app);
    CchCopyUtf8Prefix(std::span{app.state.szMsgBuf}, "changed message");
    CchCopyUtf8Prefix(std::span{app.state.szLastMsgGet}, "changed cache");
    CchCopyUtf8Prefix(std::span{app.state.szWork}, "changed work");
    TEST_CHECK(std::string_view(szFormat.data()) == "World-\xce\xa9 -123 55");
    TEST_CHECK(std::string_view(szCompressed.data()) == pszBattle);
    TEST_CHECK(std::string_view(szExpanded.data()) == pszBattle);
    TEST_CHECK(std::string_view(szIds.data()) == "The new turn is available.");
    TEST_CHECK(std::string_view(szQueued.data()) == pszBattle);
    TEST_CHECK(std::string_view(szVersion.data()) == std::string("Version ") + STARS_VERSION_DISPLAY);
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
    std::array<char, 360> szDisplayLabel{};
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    const char *psz;
    std::int16_t rgParams[1] = {static_cast<std::int16_t>(Temperature << 8 | 50)};
    const auto szTemperature = PszCalcEnvVar(app, Temperature, 0);
    const auto szGravity = PszCalcGravity(app, 50);
    for (int16_t i = 0; i <= 100; i++) {
        szDisplayLabel = PszCalcEnvVar(app, Temperature, i);
        psz = szDisplayLabel.data();
        TEST_CHECK(FUtf8Valid(psz, strlen(psz)));
        TEST_CHECK(!strcmp(psz + strlen(psz) - 3, "ºC"));
    }
    TEST_CHECK(!strcmp(PszCalcEnvVar(app, Temperature, 0).data(), "-200ºC"));
    TEST_CHECK(!strcmp(PszCalcEnvVar(app, Temperature, 50).data(), "0ºC"));
    TEST_CHECK(!strcmp(PszCalcEnvVar(app, Temperature, 100).data(), "200ºC"));
    TEST_CHECK(!strcmp(PszCalcEnvVar(app, Gravity, 50).data(), "1.00g"));
    TEST_CHECK(!strcmp(PszCalcEnvVar(app, Radiation, 50).data(), "50mR"));
    CchCopyUtf8Prefix(std::span{app.state.szWork}, "changed");
    TEST_CHECK(std::string_view(szTemperature.data()) == "-200ºC");
    TEST_CHECK(std::string_view(szGravity.data()) == "1.00g");
    TEST_CHECK(std::string_view(PszFormatString(app, "\\E|\\w", rgParams).data()) == "0ºC|0ºC");
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
    pf = PfNativeOpen(szFile, NATIVEFILEMODE::writeTruncate); TEST_ASSERT(pf);
    IPutNativeString("\xe9\r\n", pf); ICloseNativeFile(pf);
    TEST_CHECK(!GenNewGameFromFile(app, szFile));
    TEST_CHECK(!FStarsRecoveryActive() && app.state.hf == -1);
    pf = PfNativeOpen(szFile, NATIVEFILEMODE::writeTruncate); TEST_ASSERT(pf);
    IPutNativeString("Title\r\n\xff\r\n", pf); ICloseNativeFile(pf);
    TEST_CHECK(!GenNewGameFromFile(app, szFile));
    TEST_CHECK(app.state.idsFileError == idsUniverseCreationFileAppearsInvalid);
    TEST_CHECK(!FStarsRecoveryActive() && app.state.hf == -1);
    pf = PfNativeOpen(szFile, NATIVEFILEMODE::writeTruncate); TEST_ASSERT(pf);
    IPutNativeString("Title\r\n", pf);
    memset(szPadding, 'a', sizeof(szPadding));
    for (i = 0; i < 64; i++) TEST_CHECK(CWriteNative(std::span{szPadding}, pf) == sizeof(szPadding));
    ICloseNativeFile(pf);
    TEST_CHECK(!GenNewGameFromFile(app, szFile));
    TEST_CHECK(app.state.idsFileError == idsUniverseCreationFileAppearsInvalid);
    TEST_CHECK(!FStarsRecoveryActive() && app.state.hf == -1);
    pf = PfNativeOpen(szFile, NATIVEFILEMODE::writeTruncate); TEST_ASSERT(pf);
    TEST_CHECK(CWriteNative(std::span{"Title\0hidden\r\n"}.first(14), pf) == 14); ICloseNativeFile(pf);
    TEST_CHECK(!GenNewGameFromFile(app, szFile));
    TEST_CHECK(app.state.idsFileError == idsUniverseCreationFileAppearsInvalid);
    TEST_CHECK(!FStarsRecoveryActive() && app.state.hf == -1);
}

static void TestStrictAppend(void) {
    struct CASE {
        std::string_view szPrefix, szSuffix;
        std::size_t cch;
        bool fSuccess;
        std::string_view szExpected;
    };
    const CASE rgCases[] = {
        {"", "", 1, true, ""}, {"a", "\xce\xa9", 4, true, "a\xce\xa9"},
        {"a", "\xce\xa9", 3, false, ""}, {"\xce\xa9", "b", 4, true, "\xce\xa9" "b"},
        {"a", "", 2, true, "a"}, {"a", "x", 2, false, ""}, {"", "x", 0, false, ""},
        {"ab", "", 2, false, ""}, {"\xff", "x", 4, false, ""}, {"a", "\xff", 4, false, ""},
        {"a", std::string_view("x\0y", 3), 8, false, ""}, {"a", "\xe2\x82", 8, false, ""},
        {"", "\xce\xa9", 3, true, "\xce\xa9"}
    };
    for (const auto &cs : rgCases) {
        std::array<char, 16> rgch;
        rgch.fill('#');
        auto out = std::span{rgch}.subspan(1, cs.cch);
        std::ranges::copy(cs.szPrefix.substr(0, out.size()), out.begin());
        if (cs.szPrefix.size() < out.size()) out[cs.szPrefix.size()] = 0;
        const auto before = rgch;
        TEST_CHECK(FAppendUtf8(out, cs.szSuffix) == cs.fSuccess);
        TEST_CHECK(rgch.front() == '#' && rgch[cs.cch + 1] == '#');
        if (cs.fSuccess) {
            const auto pch = std::find(out.begin(), out.end(), 0);
            TEST_CHECK(pch != out.end());
            if (pch != out.end()) TEST_CHECK(std::string_view(out.data(), static_cast<std::size_t>(pch - out.begin())) == cs.szExpected);
        } else TEST_CHECK(rgch == before);
    }
    std::array<char, 8> rgch{'a', 'b', 0, 'c', 'd', 'e', '#', '#'};
    TEST_CHECK(FAppendUtf8(std::span{rgch}.first(6), std::string_view(rgch.data() + 3, 3)));
    TEST_CHECK(std::string_view(rgch.data(), 5) == "abcde" && rgch[5] == 0 && rgch[6] == '#');
}

static void TestDefinitionLineVariants(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    char szDir[cbTestPath], szFile[cbTestPath + 64];
    const char *rgszAi[] = {"#1 4"};
    const std::string_view rgszBreak[] = {"\r\n", "\n", "\r"};
    std::string szOriginal;
    std::string szLf;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("definition-line-variants", szDir, sizeof(szDir)));
    TEST_ASSERT(FStarsTestNewGame(app, szDir, 12345, rgszAi, 1));
    CchSnprintf(std::span{szFile}, "%s%sgame.def", szDir, szDirSep);
    {
        const auto cb = FNativeFileSize(szFile);
        NATIVEFILEOWNER lpfile(PfNativeOpen(szFile, NATIVEFILEMODE::read), ICloseNativeFile);
        TEST_ASSERT(cb && lpfile && *cb < 16000);
        szOriginal.resize(static_cast<std::size_t>(*cb));
        TEST_ASSERT(CbReadNativeFile(lpfile.get(), std::as_writable_bytes(std::span{szOriginal})) == szOriginal.size());
    }
    for (char ch : szOriginal) if (ch != '\r') szLf += ch;
    for (int i = 0; i < 8; i++) {
        const auto szBreak = rgszBreak[i % 3];
        const std::string szExpected = i == 7 ? "\xe6\x98\x9f\xe5\x9b\xbe" : "Unit Test";
        std::string szSource = szLf;
        std::string szText = i == 6 ? "\xef\xbb\xbf" : "";
        bool fStart = true;
        if (i == 7) szSource.replace(0, 9, szExpected);
        for (char ch : szSource) {
            if (i == 7 && fStart) szText += "  ";
            if (ch == '\n') { szText += szBreak; fStart = true; }
            else { szText += ch; fStart = false; }
        }
        if (i >= 3 && i < 6) szText.resize(szText.size() - szBreak.size());
        {
            NATIVEFILEOWNER lpfile(PfNativeOpen(szFile, NATIVEFILEMODE::writeTruncate), ICloseNativeFile);
            TEST_ASSERT(lpfile && CbWriteNativeFile(lpfile.get(), std::as_bytes(std::span{szText})) == szText.size());
        }
        TEST_ASSERT(GenNewGameFromFile(app, szFile));
        TEST_CHECK(std::string_view(app.state.game.szName) == szExpected && app.state.game.cPlayer == 2);
        TEST_CHECK(app.state.hf == -1 && !FStarsRecoveryActive());
        {
            NATIVEFILEOWNER lpfile(PfNativeOpen(szFile, NATIVEFILEMODE::read), ICloseNativeFile);
            std::string szRead(szText.size(), '\0');
            TEST_ASSERT(lpfile);
            TEST_CHECK(CbReadNativeFile(lpfile.get(), std::as_writable_bytes(std::span{szRead})) == szRead.size() && szRead == szText);
        }
        TEST_ASSERT(FStarsTestLoadHost(app));
        TEST_CHECK(app.state.game.turn == 0 && std::string_view(app.state.game.szName) == szExpected);
        DestroyCurGame(app);
    }
    const std::string_view rgszShort[] = {"", "\xef\xbb\xbf", "Title", "Title\r", "Title\r\n0 0 1 12345\r\n0 0 0 0 0 1 0\r\n"};
    for (auto szText : rgszShort) {
        NATIVEFILEOWNER lpfile(PfNativeOpen(szFile, NATIVEFILEMODE::writeTruncate), ICloseNativeFile);
        TEST_ASSERT(lpfile && CbWriteNativeFile(lpfile.get(), std::as_bytes(std::span{szText})) == szText.size());
        lpfile.reset();
        TEST_CHECK(!GenNewGameFromFile(app, szFile));
        TEST_CHECK(app.state.hf == -1 && !FStarsRecoveryActive() && !FStarsErrorPending());
    }
}

static void TestFormattedTextBounds(void) {
    std::array<char, 256> szMessage{};
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
    TEST_CHECK(!strcmp(PszFormatString(app, szTestLiteral1, rgParams).data(), "World -123 55"));
    memset(szFormat, 'a', 252);
    memcpy(szFormat + 252, "\xf0\x9f\x98\x80 suffix", 12);
    szMessage = PszFormatString(app, szFormat, NULL);
    psz = szMessage.data();
    TEST_CHECK(strlen(psz) == 252 && FUtf8Valid(psz, strlen(psz)));
    memset(app.state.szBase, 'a', sizeof(app.state.szBase) - 1); app.state.szBase[sizeof(app.state.szBase) - 1] = 0;
    szMessage = PszFormatString(app, szTestLiteral2, NULL);
    psz = szMessage.data();
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

TEST_LIST = {{"decoded messages match all source bytes and cache effects", TestMessageCatalogBytes}, {"message and version text retain buffer and queue contracts", TestMessageAndVersionText}, {"strict UTF-8 append fits atomically within its span", TestStrictAppend}, {"definition line variants preserve creation and source bytes", TestDefinitionLineVariants}, {"formatted text stays bounded and preserves UTF-8", TestFormattedTextBounds},
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
