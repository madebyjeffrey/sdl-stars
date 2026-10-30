#include "acutest.h"

#include "stars_test.h"

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
    int16_t cch;

    TEST_CHECK(strcmp(PszFromLong(1234567, NULL), "1234567") == 0);
    cch = -1;
    TEST_CHECK(strcmp(PszFromLong(-42, &cch), "-42") == 0);
    TEST_CHECK_(cch == 3, "cch %d", cch);
}

// StarsCopyFile closed the stream's hf instead of its destination, leaving
// the copy open with exclusive sharing, and its failed-open return skipped
// restoring recovery state and fFileErrSilent.
static void test_StarsCopyFile(void) {
    char    szCwd[MAX_PATH];
    char    szSrc[MAX_PATH];
    char    szDst[MAX_PATH];
    char    szBad[MAX_PATH];
    char    rgb[16];
    FILE   *fp;
    int16_t fSilentSav;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(getcwd(szCwd, sizeof(szCwd)) != NULL);
    snprintf(szSrc, sizeof(szSrc), "%s%scopy-src.txt", szCwd, szDirSep);
    snprintf(szDst, sizeof(szDst), "%s%scopy-dst.txt", szCwd, szDirSep);
    snprintf(szBad, sizeof(szBad), "%s%sno-such-dir%scopy-dst.txt", szCwd, szDirSep, szDirSep);
    remove(szDst);
    fp = fopen(szSrc, "wb");
    TEST_ASSERT(fp != NULL);
    fputs("stars copy", fp);
    fclose(fp);

    TEST_CHECK(!FStarsRecoveryActive());
    fSilentSav = fFileErrSilent;
    StarsCopyFile(szSrc, szDst);
    TEST_CHECK(!FStarsRecoveryActive());
    fp = fopen(szDst, "rb");
    TEST_ASSERT_(fp != NULL, "copy still open");
    TEST_CHECK(fgets(rgb, sizeof(rgb), fp) != NULL && strcmp(rgb, "stars copy") == 0);
    fclose(fp);

    StarsCopyFile(szSrc, szBad);
    TEST_CHECK_(!FStarsRecoveryActive(), "recovery scope not restored");
    TEST_CHECK(fFileErrSilent == fSilentSav);
}

// Literal vectors use an independently evaluated numeric recurrence, fixing
// ciphertext and seed consumption without deriving either from XorFileBuf.
static void test_XorFileBuf_unaligned_bytes(void) {
    const uint8_t golden[35] = {0x79, 0xf9, 0x98, 0x94, 0x6e, 0x0e, 0x12, 0xb6, 0x99, 0xee, 0x2b, 0x49, 0xcb, 0x14, 0xa9, 0x08, 0xb0, 0xa7, 0x29, 0xc5, 0x18, 0xb4, 0x3e, 0x62, 0xb9, 0x52, 0xaa, 0x0e, 0x66, 0xa5, 0x58, 0xf7, 0x1d, 0x9b, 0x6c};
    const int32_t seeds[10][2] = {{123456, 789012}, {645001258, 2041708718}, {622877478, 1520895743}, {125172514, 2112981774}, {721306280, 514018446}, {170401200, 2097781771}, {173304275, 470715282}, {372834923, 941819463}, {48296761, 528849842}, {1958871517, 26629285}};
    uint8_t rgb[42];
    int32_t lSeed1, lSeed2;

    for (int ib = 0; ib < 4; ib++) for (int cb = 0; cb <= 35; cb++) {
        memset(rgb, 0xa5, sizeof(rgb));
        for (int i = 0; i < cb; i++) rgb[ib + i] = (uint8_t)(i * 17 + 5);
        SetFileSeeds(123456, 789012);
        XorFileBuf((char *)rgb + ib, cb);
        GetFileSeeds(&lSeed1, &lSeed2);
        TEST_CHECK(!memcmp(rgb + ib, golden, cb));
        TEST_CHECK(lSeed1 == seeds[(cb + 3) / 4][0] && lSeed2 == seeds[(cb + 3) / 4][1]);
        for (int i = 0; i < ib; i++) TEST_CHECK(rgb[i] == 0xa5);
        for (int i = ib + cb; i < (int)sizeof(rgb); i++) TEST_CHECK(rgb[i] == 0xa5);
        SetFileSeeds(123456, 789012);
        XorFileBuf((char *)rgb + ib, cb);
        for (int i = 0; i < cb; i++) TEST_CHECK(rgb[ib + i] == (uint8_t)(i * 17 + 5));
    }
}

TEST_LIST = {{"XorFileBuf retains bytes, unaligned access and seed consumption", test_XorFileBuf_unaligned_bytes},
             {"StarsCopyFile", test_StarsCopyFile},
             {"PszFromLong", test_PszFromLong},
             {"FIntersectCircleLine horizontal", test_FIntersectCircleLine_horizontal},
             {"FIntersectCircleLine diagonal", test_FIntersectCircleLine_diagonal},
             {"FIntersectCircleLine vertical", test_FIntersectCircleLine_vertical},
             {"FIntersectCircleLine miss", test_FIntersectCircleLine_miss},
             {NULL, NULL}};
