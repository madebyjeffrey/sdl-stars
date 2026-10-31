#include "acutest.h"
[[maybe_unused]] static char szTestLiteral1[] = "\xff";
#include "stars_test.h"
#include <errno.h>
#ifdef STARS_UI_SDL
#include <SDL3/SDL.h>
#endif
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

typedef struct {
    const char *pszExpected;
    int16_t fFound;
} NATIVEENTRYCHECK;

static int16_t FCheckNativeEntry(void *lpv, const char *pszEntry) {
    NATIVEENTRYCHECK *lpnec = (NATIVEENTRYCHECK *)lpv;
    if (!strcmp(pszEntry, lpnec->pszExpected)) lpnec->fFound = 1;
    return 1;
}

static void TestNativeFiles(void) {
    char szDir[64], szFile[512], szOther[512], szLong[200], szNested[512], szMeta[512], szMetaTemp[512], rgb[16];
    int64_t cbNativeFile;
    NATIVEENTRYCHECK nec;
    int16_t hfFile, fMissing;
    int i;
    snprintf(szDir, sizeof(szDir), "stars-native-%u-\xce\xa9", (unsigned)DwTickCount());
    TEST_CHECK(CreateNativeDir(szDir) == 0 && FNativeDirectoryExists(szDir));
    snprintf(szNested, sizeof(szNested), "%s%snested-\xce\xa9", szDir, szDirSep);
    TEST_CHECK(FEnsureNativeDir(szNested) && FNativeDirectoryExists(szNested));
    snprintf(szFile, sizeof(szFile), "%s%s\xe6\x9d\xb1\xe4\xba\xac-\xc3\xa9.bin", szDir, szDirSep);
    snprintf(szOther, sizeof(szOther), "%s%s\xf0\x9f\x98\x80.bin", szDir, szDirSep);
    hfFile = HfOpenFile(szFile, mdCreate, &fMissing);
    TEST_ASSERT(hfFile != -1 && !fMissing);
    TEST_CHECK(CbWriteFile(hfFile, (void *)("abcdef"), 6) == 6 && CbFileSize(hfFile) == 6);
    TEST_CHECK(LSeekFile(hfFile, 2, 0) == 2 && CbReadFile(hfFile, rgb, 4) == 4 && memcmp(rgb, "cdef", 4) == 0);
    CloseFile(hfFile);
    TEST_CHECK(FFileExists(szFile) && !FFileReadOnly(szFile));
    TEST_CHECK(FNativeFileSize(szFile, &cbNativeFile) && cbNativeFile == 6);
    nec.pszExpected = "\xe6\x9d\xb1\xe4\xba\xac-\xc3\xa9.bin"; nec.fFound = 0;
    TEST_CHECK(FNativeEnumerateDirectory(szDir, FCheckNativeEntry, &nec) && nec.fFound);
    TEST_CHECK(RenameNativeFile(szFile, szOther) == 0 && !FFileExists(szFile) && FFileExists(szOther));
    TEST_CHECK(FNativeFileSize(szOther, &cbNativeFile) && cbNativeFile == 6);
    nec.pszExpected = "\xf0\x9f\x98\x80.bin"; nec.fFound = 0;
    TEST_CHECK(FNativeEnumerateDirectory(szDir, FCheckNativeEntry, &nec) && nec.fFound);
    snprintf(szMeta, sizeof(szMeta), "%s%s\xe6\x9d\xb1\xe4\xba\xac.meta", szDir, szDirSep);
    snprintf(szMetaTemp, sizeof(szMetaTemp), "%s%s\xce\xa9.tmp", szDir, szDirSep);
    {
        NATIVEFILE *pf = PfOpenNativeFile(szMetaTemp, "wb");
        TEST_ASSERT(pf);
        TEST_CHECK(CWriteNative("metadata", 1, 8, pf) == 8 && ICloseNativeFile(pf) == 0);
    }
    TEST_CHECK(ReplaceNativeFile(szMetaTemp, szMeta) == 0 && FNativeFileSize(szMeta, &cbNativeFile) && cbNativeFile == 8);
    TEST_CHECK(RemoveNativeFile(szMeta) == 0);
    TEST_CHECK(RemoveNativeFile(szOther) == 0 && !FFileExists(szOther));
    TEST_CHECK(HfOpenFile(szOther, mdRead, &fMissing) == -1 && fMissing);
    for (i = 0; i < 150; i++) szLong[i] = 'x';
    strcpy(szLong + 150, "-\xce\xa9.bin");
    snprintf(szFile, sizeof(szFile), "%s%s%s", szDir, szDirSep, szLong);
    hfFile = HfOpenFile(szFile, mdCreate, &fMissing);
    TEST_ASSERT(hfFile != -1);
    TEST_CHECK(CbWriteFile(hfFile, (void *)("Z"), 1) == 1); CloseFile(hfFile);
    TEST_CHECK(FFileExists(szFile) && RemoveNativeFile(szFile) == 0);
    TEST_CHECK(HfOpenFile(szTestLiteral1, mdRead, &fMissing) == -1 && !fMissing && errno == EILSEQ);
    TEST_CHECK(RemoveNativeFile("\xff") == -1 && errno == EILSEQ);
    TEST_CHECK(CreateNativeDir("\xff") == -1 && errno == EILSEQ);
    TEST_CHECK(!FNativeDirectoryExists("\xff") && errno == EILSEQ);
    TEST_CHECK(!FNativeEnumerateDirectory("\xff", FCheckNativeEntry, &nec) && errno == EILSEQ);
    TEST_CHECK(!FNativeFileSize("\xff", &cbNativeFile) && errno == EILSEQ);
    TEST_CHECK(!FEnsureNativeDir("\xff") && errno == EILSEQ);
    TEST_CHECK(RemoveNativePath("\xff") == -1 && errno == EILSEQ);
    TEST_CHECK(ReplaceNativeFile("\xff", szOther) == -1 && errno == EILSEQ);
    TEST_CHECK(RemoveNativePath(szNested) == 0 && !FNativeDirectoryExists(szNested));
#ifdef _WIN32
    {
#ifdef STARS_UI_SDL
        char *pszWide = SDL_iconv_string("UTF-16LE", "UTF-8", szDir, strlen(szDir) + 1);
        TEST_ASSERT(pszWide);
        TEST_CHECK(_wrmdir((wchar_t *)pszWide) == 0);
        SDL_free(pszWide);
#else
        WCHAR wszDir[64];
        TEST_ASSERT(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, szDir, -1, wszDir, 64) > 0);
        TEST_CHECK(_wrmdir(wszDir) == 0);
#endif
    }
#else
    TEST_CHECK(rmdir(szDir) == 0);
#endif
}
TEST_LIST = {{"native Unicode paths, backups and long Windows paths", TestNativeFiles}, {NULL, NULL}};
