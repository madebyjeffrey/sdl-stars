#include "acutest.h"
#include "stars_test.h"

static void TestEncoding(void) {
    char szLegacy[256], szUtf8[768], szRound[256], szSmall[4];
    int i, cch;
    const char *rgszBad[] = {"\xc0\xaf", "\xe2\x82", "\xed\xa0\x80", "\xf4\x90\x80\x80", "a\xff"};
    size_t j;
    for (i = 1; i < 256; i++) szLegacy[i - 1] = (char)i;
    szLegacy[255] = 0;
    cch = CchLegacyToUtf8(szUtf8, sizeof(szUtf8), szLegacy, 255);
    TEST_ASSERT(cch > 255);
    TEST_CHECK(CchUtf8ToLegacy(szRound, sizeof(szRound), szUtf8, (size_t)cch) == 255);
    TEST_CHECK(memcmp(szLegacy, szRound, sizeof(szLegacy)) == 0);
    TEST_CHECK(CchUtf8ToLegacy(szSmall, sizeof(szSmall), "\xc3\xa9\xe2\x82\xac", 5) == 2);
    TEST_CHECK((unsigned char)szSmall[0] == 0xe9 && (unsigned char)szSmall[1] == 0x80);
    TEST_CHECK(CchUtf8ToLegacy(szSmall, sizeof(szSmall), "\xce\xa9", 2) == -2 && szSmall[0] == 0);
    TEST_CHECK(CchUtf8ToLegacy(szSmall, 1, "a", 1) == -3 && szSmall[0] == 0);
    TEST_CHECK(CchLegacyToUtf8(szSmall, 2, "\xe9", 1) == -3 && szSmall[0] == 0);
    for (j = 0; j < sizeof(rgszBad) / sizeof(*rgszBad); j++) {
        strcpy(szSmall, "old");
        TEST_CHECK(CchUtf8ToLegacy(szSmall, sizeof(szSmall), rgszBad[j], strlen(rgszBad[j])) == -1);
        TEST_CHECK(szSmall[0] == 0);
    }
    TEST_CHECK(CchUtf8Prefix("12345678901234\xce\xa9.r1", 15) == 14);
    TEST_CHECK(CchUtf8Prefix("\xf0\x9f\x98\x80", 3) == 0);
    TEST_CHECK(CchUtf8Prefix("\xf0\x9f\x98\x80", 4) == 4);
    TEST_CHECK(CchUtf8Prefix("a\xffz", 2) == 2);
}

static void TestPasswords(void) {
    char szCmd[] = "-p\xc3\xa9 game.hst";
    char szLong[] = "-p\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9 game.hst";
    char szBad[] = "-p\xe6\x9d\xb1\xe4\xba\xac game.hst";
    char szTooLong[] = "-p1234567890123456 game.hst";
    char szPath[sizeof(szBase) + 4];
    char szExpected[16];
    int16_t fSeed = FALSE;
    uint32_t lSeed = 0;
    TEST_ASSERT(FStarsTestInit());
    ParseCmdLine(szCmd, &fSeed, &lSeed);
    TEST_CHECK((unsigned char)szPassLast[0] == 0xe9 && szPassLast[1] == 0);
    TEST_CHECK(lSaltLast == -23 && lSaltLast == LSaltFromSz("\xe9") && strcmp(szBase, "game.hst") == 0);
    memset(szExpected, 0xe9, 15); szExpected[15] = 0;
    ParseCmdLine(szLong, &fSeed, &lSeed);
    TEST_CHECK(strcmp(szPassLast, szExpected) == 0 && lSaltLast == 2033583528 && lSaltLast == LSaltFromSz(szExpected));
    ParseCmdLine(szBad, &fSeed, &lSeed);
    TEST_CHECK(szBase[0] == 0 && !ini.fCmdLine && cStarsTestAlert == 1);
    ParseCmdLine(szTooLong, &fSeed, &lSeed);
    TEST_CHECK(szBase[0] == 0 && !ini.fCmdLine && cStarsTestAlert == 2);
    memset(szPath, 'x', sizeof(szPath) - 1); szPath[sizeof(szPath) - 1] = 0;
    ParseCmdLine(szPath, &fSeed, &lSeed);
    TEST_CHECK(szBase[0] == 0 && !ini.fCmdLine && cStarsTestAlert == 3);
    SetSzRaceFile("12345678901234\xce\xa9.r1");
    TEST_CHECK(strcmp(szRaceFile, "12345678901234") == 0);
}

TEST_LIST = {{"strict legacy/UTF-8 boundaries", TestEncoding},
             {"native passwords and filename capacities", TestPasswords}, {NULL, NULL}};
