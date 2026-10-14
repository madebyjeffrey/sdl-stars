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
    pf = fopen(szFile, "wb"); TEST_ASSERT(pf);
    fputs("Title\r\n\xff\r\n", pf); fclose(pf);
    TEST_CHECK(!GenNewGameFromFile(szFile));
    TEST_CHECK(idsFileError == idsUniverseCreationFileAppearsInvalid);
    TEST_CHECK(penvMem == NULL && lpbDefUni == NULL);
    pf = fopen(szFile, "wb"); TEST_ASSERT(pf);
    TEST_CHECK(fwrite("Title\0hidden\r\n", 1, 14, pf) == 14); fclose(pf);
    TEST_CHECK(!GenNewGameFromFile(szFile));
    TEST_CHECK(idsFileError == idsUniverseCreationFileAppearsInvalid);
    TEST_CHECK(penvMem == NULL && lpbDefUni == NULL);
}

static void TestFormattedTextBounds(void) {
    char szFormat[1026];
    char sz[1024];
    char *psz;
    int16_t rgParams[2] = {-123, 55};

    TEST_ASSERT(FStarsTestInit());
    strcpy(szWork, "World");
    TEST_CHECK(!strcmp(PszFormatString("\x5cw \x5ci \x5cu", rgParams), "World -123 55"));
    memset(szFormat, 'a', 252);
    memcpy(szFormat + 252, "\xf0\x9f\x98\x80 suffix", 12);
    psz = PszFormatString(szFormat, NULL);
    TEST_CHECK(strlen(psz) == 252 && FUtf8Valid(psz, strlen(psz)));
    memset(szBase, 'a', sizeof(szBase) - 1); szBase[sizeof(szBase) - 1] = 0;
    psz = PszFormatString("\x5ch", NULL);
    TEST_CHECK(strlen(psz) < sizeof(szMsgBuf) && FUtf8Valid(psz, strlen(psz)));
    memset(szFormat, 'a', 1022);
    memcpy(szFormat + 1022, "\xc3\xa9z", 4);
    TEST_CHECK(CchSprintf(sz, "%s", szFormat) == 1022);
    TEST_CHECK(strlen(sz) == 1022 && FUtf8Valid(sz, strlen(sz)));
}

TEST_LIST = {{"formatted text stays bounded and preserves UTF-8", TestFormattedTextBounds},
             {"invalid UTF-8 definition cleans up the error handler", TestInvalidDefinition},
             {"strict UTF-8 text and UTF-16 boundaries", TestEncoding},
             {"UTF-8 passwords and filename byte capacities", TestPasswords}, {NULL, NULL}};
