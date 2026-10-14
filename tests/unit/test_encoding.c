#include "acutest.h"
#include "stars_test.h"

static void TestEncoding(void) {
    char sz[32], szSmall[4];
    const char *rgszBad[] = {"\xc0\xaf", "\xe2\x82", "\xed\xa0\x80", "\xf4\x90\x80\x80", "a\xff"};
    const unsigned char rgbUtf16[] = {0xa9, 0, 0x3d, 0xd8, 0, 0xde};
    const unsigned char rgbBadUtf16[] = {0x3d, 0xd8};
    size_t j;
    TEST_CHECK(CchCopyUtf8(sz, sizeof(sz), "\xc3\xa9\xe2\x82\xac", 5) == 5);
    TEST_CHECK(strcmp(sz, "\xc3\xa9\xe2\x82\xac") == 0);
    TEST_CHECK(CchCopyUtf8(szSmall, 1, "a", 1) == -3 && szSmall[0] == 0);
    TEST_CHECK(CchCopyUtf8(szSmall, sizeof(szSmall), "\xf0\x9f\x98\x80", 4) == -3 && szSmall[0] == 0);
    TEST_CHECK(CchUtf16leToUtf8(sz, sizeof(sz), rgbUtf16, 3) == 6);
    TEST_CHECK(strcmp(sz, "\xc2\xa9\xf0\x9f\x98\x80") == 0);
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
    char szCmd[] = "-p\xc3\xa9 game.hst";
    char szUtf8[] = "-p\xe6\x9d\xb1\xe4\xba\xac game.hst";
    char szBad[] = "-p\xed\xa0\x80 game.hst";
    char szTooLong[] = "-p\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9 game.hst";
    char szPath[sizeof(szBase) + 4];
    int16_t fSeed = FALSE;
    uint32_t lSeed = 0;
    TEST_ASSERT(FStarsTestInit());
    TEST_CHECK(ParseCmdLine(szCmd, &fSeed, &lSeed));
    TEST_CHECK(strcmp(szPassLast, "\xc3\xa9") == 0);
    TEST_CHECK(lSaltLast == 5307 && lSaltLast == LSaltFromSz(szPassLast) && strcmp(szBase, "game.hst") == 0);
    TEST_CHECK(ParseCmdLine(szUtf8, &fSeed, &lSeed));
    TEST_CHECK(strcmp(szPassLast, "\xe6\x9d\xb1\xe4\xba\xac") == 0 && lSaltLast == LSaltFromSz(szPassLast));
    TEST_CHECK(!ParseCmdLine(szBad, &fSeed, &lSeed));
    TEST_CHECK(szBase[0] == 0 && !ini.fCmdLine && cStarsTestAlert == 1);
    TEST_CHECK(!ParseCmdLine(szTooLong, &fSeed, &lSeed));
    TEST_CHECK(szBase[0] == 0 && !ini.fCmdLine && cStarsTestAlert == 2);
    memset(szPath, 'x', sizeof(szPath) - 1); szPath[sizeof(szPath) - 1] = 0;
    TEST_CHECK(!ParseCmdLine(szPath, &fSeed, &lSeed));
    TEST_CHECK(szBase[0] == 0 && !ini.fCmdLine && cStarsTestAlert == 3);
    SetSzRaceFile("12345678901234\xce\xa9.r1");
    TEST_CHECK(strcmp(szRaceFile, "12345678901234") == 0);
}

static void TestInvalidDefinition(void) {
    char szDir[MAX_PATH], szFile[MAX_PATH];
    FILE *pf;
    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("utf8_invalid_definition", szDir, sizeof(szDir)));
    snprintf(szFile, sizeof(szFile), "%s%sbad.def", szDir, szDirSep);
    pf = fopen(szFile, "wb"); TEST_ASSERT(pf);
    fputs("\xe9\r\n", pf); fclose(pf);
    TEST_CHECK(!GenNewGameFromFile(szFile));
    TEST_CHECK(penvMem == NULL && lpbDefUni == NULL);
}

TEST_LIST = {{"invalid UTF-8 title cleans up the error handler", TestInvalidDefinition},
             {"strict UTF-8 text and UTF-16 boundaries", TestEncoding},
             {"UTF-8 passwords and filename byte capacities", TestPasswords}, {NULL, NULL}};
