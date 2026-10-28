#include "acutest.h"

#include "stars_test.h"

// An uncompressed design name used to overwrite the hull statistics after
// szClass. Check both the partial and full record layouts at the name limit.
static void test_FReadShDef_bounds_uncompressed_name(void) {
    uint8_t rgb[sizeof(RTSHDEF) + 80];
    RTSHDEF *lprt;
    SHDEF rgshdef[16];
    uint8_t *pb;
    int16_t rgcch[] = {0, 31, 32, 64};
    int16_t rgdet[] = {detSome, detAll};
    int16_t i;
    int16_t j;
    int16_t cch;

    idPlayer = iplrNone;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 4; j++) {
            memset(rgb, 0, sizeof(rgb));
            memset(rgshdef, 0, sizeof(rgshdef));
            rgshdef[0].fFree = TRUE;
            lprt = (RTSHDEF *)rgb;
            lprt->det = rgdet[i];
            lprt->ihuldef = ihuldefSmallFreighter;
            lprt->ibmp = LphuldefFromId(lprt->ihuldef)->hul.ibmp;
            lprt->wtEmpty = 1234;
            pb = lprt->det == detAll ? (uint8_t *)lprt->rghs : &lprt->chs;
            memset(pb + 1, 'N', rgcch[j]);
            pb[rgcch[j] + 1] = 0;

            TEST_ASSERT(FReadShDef(lprt, rgshdef, iplrNone));
            cch = min(rgcch[j], sizeof(rgshdef[0].hul.szClass) - 1);
            TEST_CHECK_(strnlen(rgshdef[0].hul.szClass, sizeof(rgshdef[0].hul.szClass)) == cch, "detail %d, name length %d", rgdet[i], rgcch[j]);
            TEST_CHECK(rgshdef[0].hul.szClass[cch] == 0);
            if (lprt->det == detAll) {
                TEST_CHECK(rgshdef[0].hul.dp == 1234);
            } else {
                TEST_CHECK(rgshdef[0].hul.wtEmpty == 1234);
            }
            TEST_CHECK(rgshdef[0].hul.resCost == 0);
            TEST_CHECK(rgshdef[0].hul.rghs[0].grhst == 0);
        }
    }
}

TEST_LIST = {{"FReadShDef bounds uncompressed name", test_FReadShDef_bounds_uncompressed_name}, {NULL, NULL}};
