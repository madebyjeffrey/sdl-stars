#include "acutest.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"

#include "stars_test.h"
#include <string>

static void test_IntToRoman_bounded(void) {
    const char *rgszOnes[] = {"", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX"};
    char szRoman[3282];

    // Preserve the original repeated-X notation over its entire input range.
    for (int i = -32768; i <= 32767; i++) {
        const auto szExpected = i > 0 ? std::string(static_cast<std::size_t>(i / 10), 'X') + rgszOnes[i % 10] : std::string{};
        const auto cch = IntToRoman(static_cast<std::int16_t>(i), std::span{szRoman});
        TEST_CHECK(cch && *cch == szExpected.size() && szExpected == szRoman);
    }
    for (int16_t i = 0; i <= 16; i++) {
        const auto szExpected = std::string(static_cast<std::size_t>(i / 10), 'X') + rgszOnes[i % 10];
        for (std::size_t cb = 0; cb <= 7; cb++) {
            std::fill(std::begin(szRoman), std::end(szRoman), '!');
            const auto cch = IntToRoman(i, std::span{szRoman}.subspan(1, cb));
            TEST_CHECK(cch.has_value() == (cb > szExpected.size()));
            TEST_CHECK(szRoman[0] == '!' && szRoman[cb + 1] == '!');
            if (cch) TEST_CHECK(szExpected == szRoman + 1);
            else if (cb != 0) TEST_CHECK(szRoman[1] == 0);
        }
    }
}

static void test_UserString_codec_contract(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    char szPacked[1032], szRound[1032], szInput[2049];
    int16_t cPacked, cRound;
    const unsigned char rgbExpected[] = {0x01, 0xb0};

    TEST_ASSERT(FStarsTestInit(app));
    for (int16_t cb = 0; cb <= 4; cb++) {
        memset(szPacked, '!', sizeof(szPacked));
        cPacked = cb;
        const auto cbPacked1 = FCompressUserString(app, " aA", std::as_writable_bytes(std::span(szPacked + 2, static_cast<std::size_t>(cPacked))));
        if (cbPacked1) cPacked = static_cast<std::int16_t>(*cbPacked1);
        TEST_CHECK(cbPacked1.has_value() == (cb >= 2));
        TEST_CHECK(szPacked[1] == '!' && szPacked[2 + cb] == '!');
        if (cb < 2) TEST_CHECK(cPacked == cb && szPacked[2] == '!');
        else TEST_CHECK(cPacked == 2 && memcmp(szPacked + 2, rgbExpected, 2) == 0);
    }
    for (int16_t cb = 0; cb <= 5; cb++) {
        memset(szRound, '!', sizeof(szRound));
        cRound = cb;
        TEST_CHECK(FDecompressUserString(app, std::as_bytes(std::span{rgbExpected}), std::span(szRound + 2, static_cast<std::size_t>(cRound) + 1)).has_value() == (cb >= 3));
        TEST_CHECK(cRound == cb && szRound[1] == '!' && szRound[3 + cb] == '!');
        if (cb < 3) TEST_CHECK(szRound[2] == 0 && szRound[3] == '!');
        else TEST_CHECK(strcmp(szRound + 2, " aA") == 0);
    }
    for (int i = 1; i <= 255; i++) szInput[i - 1] = static_cast<char>(i);
    szInput[255] = 0;
    cPacked = 1024;
    const auto cbPacked2 = FCompressUserString(app, szInput, std::as_writable_bytes(std::span(szPacked, static_cast<std::size_t>(cPacked))));
    if (cbPacked2) cPacked = static_cast<std::int16_t>(*cbPacked2);
    TEST_ASSERT(cbPacked2);
    cRound = 1023;
    TEST_ASSERT(FDecompressUserStringBytes(app, std::as_bytes(std::span(szPacked, static_cast<std::size_t>(cPacked))), std::span(szRound, static_cast<std::size_t>(cRound) + 1)));
    TEST_CHECK(memcmp(szInput, szRound, 256) == 0);
    TEST_CHECK(!FDecompressUserString(app, std::as_bytes(std::span(szPacked, static_cast<std::size_t>(cPacked))), std::span(szRound, static_cast<std::size_t>(cRound) + 1)) && szRound[0] == 0);
    memset(szInput, ' ', sizeof(szInput));
    szInput[2047] = 0;
    cPacked = 1024;
    const auto cbPacked3 = FCompressUserString(app, szInput, std::as_writable_bytes(std::span(szPacked, static_cast<std::size_t>(cPacked))));
    if (cbPacked3) cPacked = static_cast<std::int16_t>(*cbPacked3);
    TEST_CHECK(cbPacked3 && cPacked == 1024);
    szInput[2047] = ' '; szInput[2048] = 0;
    cPacked = 1024;
    const auto cbPacked4 = FCompressUserString(app, szInput, std::as_writable_bytes(std::span(szPacked, static_cast<std::size_t>(cPacked))));
    if (cbPacked4) cPacked = static_cast<std::int16_t>(*cbPacked4);
    TEST_CHECK(!cbPacked4 && cPacked == 1024);
    szInput[1023] = 0;
    const auto cbPacked5 = FCompressUserString(app, szInput, std::as_writable_bytes(std::span(szPacked, static_cast<std::size_t>(cPacked))));
    if (cbPacked5) cPacked = static_cast<std::int16_t>(*cbPacked5);
    TEST_ASSERT(cbPacked5);
    cRound = 1023;
    TEST_CHECK(FDecompressUserString(app, std::as_bytes(std::span(szPacked, static_cast<std::size_t>(cPacked))), std::span(szRound, static_cast<std::size_t>(cRound) + 1)) && strlen(szRound) == 1023);
    szInput[1023] = ' '; szInput[1024] = 0; cPacked = 1024;
    const auto cbPacked6 = FCompressUserString(app, szInput, std::as_writable_bytes(std::span(szPacked, static_cast<std::size_t>(cPacked))));
    if (cbPacked6) cPacked = static_cast<std::int16_t>(*cbPacked6);
    TEST_ASSERT(cbPacked6);
    TEST_CHECK(!FDecompressUserString(app, std::as_bytes(std::span(szPacked, static_cast<std::size_t>(cPacked))), std::span(szRound, static_cast<std::size_t>(cRound) + 1)) && szRound[0] == 0);
    const auto cbEmpty = FCompressUserString(app, std::string_view{}, std::span<std::byte>{});
    TEST_CHECK(cbEmpty && *cbEmpty == 0);
    const auto cchEmpty = FDecompressUserString(app, std::span<const std::byte>{}, std::span{szRound}.first(1));
    TEST_CHECK(cchEmpty && *cchEmpty == 0 && szRound[0] == 0);
    memset(szRound, '!', sizeof(szRound));
    const auto cchDecoded = FDecompressUserString(app, std::as_bytes(std::span{rgbExpected}), std::span{szRound}.first(4));
    TEST_CHECK(cchDecoded && *cchDecoded == 3 && strcmp(szRound, " aA") == 0 && szRound[4] == '!');
    TEST_CHECK(!FDecompressUserString(app, std::span<const std::byte>{}, std::span<char>{szRound, 0}) && szRound[0] == ' ');
    TEST_CHECK(!FDecompressUserString(app, std::span<const std::byte>{}, std::span{szRound}.first(1025)) && szRound[0] == 0 && szRound[1] == 'a');
    memset(szPacked, '!', sizeof(szPacked));
    TEST_CHECK(!FCompressUserString(app, std::string_view("a\0b", 3), std::as_writable_bytes(std::span{szPacked})) && szPacked[0] == '!');
}

static POINT16 Pt(int16_t x, int16_t y) {
    POINT16 pt;

    pt.x = x;
    pt.y = y;
    return pt;
}

// CheckCross checks that a route crosses a radius-10 field centered on it
// for 20 ly, from 40 to 60 ly along a 100 ly route.
static void CheckCross(POINT16 ptFrom, POINT16 ptTo, POINT16 ptField) {
    int16_t dStart;
    int16_t dEnd;

    dStart = dEnd = -1;
    TEST_ASSERT(FIntersectCircleLine(ptFrom, ptTo, ptField, 100, 100, &dStart, &dEnd));
    TEST_CHECK_(dStart == 40, "start %d", dStart);
    TEST_CHECK_(dEnd == 60, "end %d", dEnd);
}

static void test_FIntersectCircleLine_horizontal(void) {
    CheckCross(Pt(0, 0), Pt(100, 0), Pt(50, 0));
    CheckCross(Pt(100, 0), Pt(0, 0), Pt(50, 0));
}

static void test_FIntersectCircleLine_diagonal(void) {
    CheckCross(Pt(0, 0), Pt(60, 80), Pt(30, 40));
    CheckCross(Pt(60, 80), Pt(0, 0), Pt(30, 40));
}

// North/South minefield immunity: a vertical route measured the field from
// the route's start point instead of its closest point.
static void test_FIntersectCircleLine_vertical(void) {
    CheckCross(Pt(0, 0), Pt(0, 100), Pt(0, 50));
    CheckCross(Pt(0, 100), Pt(0, 0), Pt(0, 50));
    CheckCross(Pt(5, 0), Pt(5, 100), Pt(5, 50));
}

static void test_FIntersectCircleLine_miss(void) {
    int16_t dStart;
    int16_t dEnd;

    TEST_CHECK(!FIntersectCircleLine(Pt(0, 0), Pt(100, 0), Pt(50, 20), 100, 100, &dStart, &dEnd));
    // A field behind the route's start.
    TEST_CHECK(!FIntersectCircleLine(Pt(0, 0), Pt(100, 0), Pt(-50, 0), 100, 100, &dStart, &dEnd));
}

// PszFromLong tested *pcch instead of pcch, so a NULL count pointer (the
// report dumps' calls) was dereferenced.
static void test_PszFromLong(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    int16_t cch;

    TEST_CHECK(strcmp(PszFromLong(app, 1234567, NULL), "1234567") == 0);
    cch = -1;
    TEST_CHECK(strcmp(PszFromLong(app, -42, &cch), "-42") == 0);
    TEST_CHECK_(cch == 3, "cch %d", cch);
}

// StarsCopyFile closed the stream's hf instead of its destination, leaving
// the copy open with exclusive sharing, and its failed-open return skipped
// restoring recovery state and fFileErrSilent.
static void test_StarsCopyFile(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char    szCwd[cbTestPath];
    char    szSrc[cbTestPath];
    char    szDst[cbTestPath];
    char    szBad[cbTestPath];
    char    rgb[16];
    FILE   *fp;
    int16_t fSilentSav;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(PszStarsTestCwd(szCwd, sizeof(szCwd)) != NULL);
    snprintf(szSrc, sizeof(szSrc), "%s%scopy-src.txt", szCwd, szDirSep);
    snprintf(szDst, sizeof(szDst), "%s%scopy-dst.txt", szCwd, szDirSep);
    snprintf(szBad, sizeof(szBad), "%s%sno-such-dir%scopy-dst.txt", szCwd, szDirSep, szDirSep);
    remove(szDst);
    fp = fopen(szSrc, "wb");
    TEST_ASSERT(fp != NULL);
    fputs("stars copy", fp);
    fclose(fp);

    TEST_CHECK(!FStarsRecoveryActive());
    fSilentSav = app.state.fFileErrSilent;
    StarsCopyFile(app, szSrc, szDst);
    TEST_CHECK(!FStarsRecoveryActive());
    fp = fopen(szDst, "rb");
    TEST_ASSERT_(fp != NULL, "copy still open");
    TEST_CHECK(fgets(rgb, sizeof(rgb), fp) != NULL && strcmp(rgb, "stars copy") == 0);
    fclose(fp);

    StarsCopyFile(app, szSrc, szBad);
    TEST_CHECK_(!FStarsRecoveryActive(), "recovery scope not restored");
    TEST_CHECK(app.state.fFileErrSilent == fSilentSav);
}

// Literal vectors use an independently evaluated numeric recurrence, fixing
// ciphertext and seed consumption without deriving either from XorFileBuf.
static void test_XorFileBuf_unaligned_bytes(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    const uint8_t golden[35] = {0x79, 0xf9, 0x98, 0x94, 0x6e, 0x0e, 0x12, 0xb6, 0x99, 0xee, 0x2b, 0x49, 0xcb, 0x14, 0xa9, 0x08, 0xb0, 0xa7, 0x29, 0xc5, 0x18, 0xb4, 0x3e, 0x62, 0xb9, 0x52, 0xaa, 0x0e, 0x66, 0xa5, 0x58, 0xf7, 0x1d, 0x9b, 0x6c};
    const int32_t seeds[10][2] = {{123456, 789012}, {645001258, 2041708718}, {622877478, 1520895743}, {125172514, 2112981774}, {721306280, 514018446}, {170401200, 2097781771}, {173304275, 470715282}, {372834923, 941819463}, {48296761, 528849842}, {1958871517, 26629285}};
    uint8_t rgb[42];
    int32_t lSeed1, lSeed2;

    for (int ib = 0; ib < 4; ib++) for (int cb = 0; cb <= 35; cb++) {
        memset(rgb, 0xa5, sizeof(rgb));
        for (int i = 0; i < cb; i++) rgb[ib + i] = (uint8_t)(i * 17 + 5);
        SetFileSeeds(app, 123456, 789012);
        XorFileBuf(app, (char *)rgb + ib, cb);
        GetFileSeeds(app, &lSeed1, &lSeed2);
        TEST_CHECK(!memcmp(rgb + ib, golden, cb));
        TEST_CHECK(lSeed1 == seeds[(cb + 3) / 4][0] && lSeed2 == seeds[(cb + 3) / 4][1]);
        for (int i = 0; i < ib; i++) TEST_CHECK(rgb[i] == 0xa5);
        for (int i = ib + cb; i < (int)sizeof(rgb); i++) TEST_CHECK(rgb[i] == 0xa5);
        SetFileSeeds(app, 123456, 789012);
        XorFileBuf(app, (char *)rgb + ib, cb);
        for (int i = 0; i < cb; i++) TEST_CHECK(rgb[ib + i] == (uint8_t)(i * 17 + 5));
    }
}

// Compound assignment evaluates the RNG call before reading its destination.
// Keep that order even when a caller supplies one of the actual seed objects.
static void test_XorFileBuf_seed_alias_order(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    int32_t lSeed1, lSeed2;

    SetFileSeeds(app, 123456, 789012);
    XorFileBuf(app, reinterpret_cast<char *>(&app.state.lFileSeed1), 4);
    GetFileSeeds(app, &lSeed1, &lSeed2);
    TEST_CHECK(lSeed1 == -1966203050 && lSeed2 == 2041708718);
    SetFileSeeds(app, 123456, 789012);
    XorFileBuf(app, reinterpret_cast<char *>(&app.state.lFileSeed2), 4);
    GetFileSeeds(app, &lSeed1, &lSeed2);
    TEST_CHECK(lSeed1 == 645001258 && lSeed2 == -720506926);
}

TEST_LIST = {{"Roman numerals retain all int16 values within actual capacity", test_IntToRoman_bounded},
             {"XorFileBuf retains RHS-before-destination evaluation", test_XorFileBuf_seed_alias_order},
             {"user-string codecs retain bytes, limits and failure effects", test_UserString_codec_contract},
             {"XorFileBuf retains bytes, unaligned access and seed consumption", test_XorFileBuf_unaligned_bytes},
             {"StarsCopyFile", test_StarsCopyFile},
             {"PszFromLong", test_PszFromLong},
             {"FIntersectCircleLine horizontal", test_FIntersectCircleLine_horizontal},
             {"FIntersectCircleLine diagonal", test_FIntersectCircleLine_diagonal},
             {"FIntersectCircleLine vertical", test_FIntersectCircleLine_vertical},
             {"FIntersectCircleLine miss", test_FIntersectCircleLine_miss},
             {NULL, NULL}};
