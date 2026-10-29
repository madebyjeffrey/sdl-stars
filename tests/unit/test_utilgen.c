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

TEST_LIST = {{"StarsCopyFile", test_StarsCopyFile},
             {"PszFromLong", test_PszFromLong},
             {"FIntersectCircleLine horizontal", test_FIntersectCircleLine_horizontal},
             {"FIntersectCircleLine diagonal", test_FIntersectCircleLine_diagonal},
             {"FIntersectCircleLine vertical", test_FIntersectCircleLine_vertical},
             {"FIntersectCircleLine miss", test_FIntersectCircleLine_miss},
             {NULL, NULL}};
