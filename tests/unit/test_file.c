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

// UnpackBattlePlan copied raw names past its 32-byte field and into the
// following plan. Keep the order fields and the next object's bytes intact.
static void test_UnpackBattlePlan_bounds_uncompressed_name(void) {
    struct {
        BTLPLAN btlplan;
        uint8_t rgbGuard[80];
    } buf;
    uint8_t rgb[128];
    int16_t rgcch[] = {0, 31, 32, 64};
    int16_t i;
    int16_t j;
    int16_t cch;

    for (i = 0; i < 4; i++) {
        memset(rgb, 0, sizeof(rgb));
        ((BTLPLAN *)rgb)->iplr = 1;
        ((BTLPLAN *)rgb)->mdTactic = 3;
        ((BTLPLAN *)rgb)->mdTarget1 = 2;
        ((BTLPLAN *)rgb)->mdTarget2 = 4;
        ((BTLPLAN *)rgb)->iplrAttack = 5;
        memset(rgb + 5, 'N', rgcch[i]);
        memset(&buf, 0x55, sizeof(buf));

        UnpackBattlePlan(rgb, &buf.btlplan, 6);
        cch = min(rgcch[i], sizeof(buf.btlplan.szName) - 1);
        TEST_CHECK_(strnlen(buf.btlplan.szName, sizeof(buf.btlplan.szName)) == cch, "name length %d", rgcch[i]);
        TEST_CHECK(buf.btlplan.szName[cch] == 0);
        TEST_CHECK(buf.btlplan.iplr == 1);
        TEST_CHECK(buf.btlplan.iplan == 6);
        TEST_CHECK(buf.btlplan.mdTactic == 3);
        TEST_CHECK(buf.btlplan.mdTarget1 == 2);
        TEST_CHECK(buf.btlplan.mdTarget2 == 4);
        TEST_CHECK(buf.btlplan.iplrAttack == 5);
        for (j = 0; j < sizeof(buf.rgbGuard); j++)
            TEST_CHECK_(buf.rgbGuard[j] == 0x55, "name length %d, guard byte %d", rgcch[i], j);
    }
}

// The decoder's limit counts characters, not the trailing NUL. FReadShDef
// passed 32 for szClass[32] and ignored a failed decode of still longer names.
static void test_FReadShDef_reserves_compressed_name_terminator(void) {
    uint8_t rgb[sizeof(RTSHDEF) + 80];
    RTSHDEF *lprt;
    SHDEF rgshdef[16];
    SHDEF shdefSav;
    uint8_t *pb;
    char szName[65];
    int16_t rgcch[] = {31, 32, 64};
    int16_t rgdet[] = {detSome, detAll};
    int16_t cOut;
    int16_t fRead;
    int16_t i;
    int16_t j;

    idPlayer = iplrNone;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 3; j++) {
            memset(rgb, 0, sizeof(rgb));
            memset(rgshdef, 0, sizeof(rgshdef));
            rgshdef[0].fFree = TRUE;
            shdefSav = rgshdef[0];
            lprt = (RTSHDEF *)rgb;
            lprt->det = rgdet[i];
            lprt->ihuldef = ihuldefSmallFreighter;
            lprt->ibmp = LphuldefFromId(lprt->ihuldef)->hul.ibmp;
            lprt->wtEmpty = 1234;
            pb = lprt->det == detAll ? (uint8_t *)lprt->rghs : &lprt->chs;
            memset(szName, 'a', rgcch[j]);
            szName[rgcch[j]] = 0;
            cOut = 64;
            TEST_ASSERT(FCompressUserString(szName, (char *)pb + 1, &cOut));
            *pb = cOut;

            fRead = FReadShDef(lprt, rgshdef, iplrNone);
            if (rgcch[j] == 31) {
                TEST_ASSERT(fRead);
                TEST_CHECK(strcmp(rgshdef[0].hul.szClass, szName) == 0);
                if (lprt->det == detAll) {
                    TEST_CHECK(rgshdef[0].hul.dp == 1234);
                } else {
                    TEST_CHECK(rgshdef[0].hul.wtEmpty == 1234);
                }
            } else {
                TEST_CHECK_(!fRead, "detail %d, compressed name length %d", rgdet[i], rgcch[j]);
                TEST_CHECK(memcmp(&rgshdef[0], &shdefSav, sizeof(shdefSav)) == 0);
            }
        }
    }
}

TEST_LIST = {{"FReadShDef bounds uncompressed name", test_FReadShDef_bounds_uncompressed_name},
             {"UnpackBattlePlan bounds uncompressed name", test_UnpackBattlePlan_bounds_uncompressed_name},
             {"FReadShDef reserves compressed name terminator", test_FReadShDef_reserves_compressed_name_terminator},
             {NULL, NULL}};
