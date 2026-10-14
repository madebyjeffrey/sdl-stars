#include "acutest.h"
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

static void TestNativeFiles(void) {
    char szDir[64], szFile[512], szOther[512], szLong[200], rgb[16];
    int16_t hfFile, fMissing;
    int i;
    snprintf(szDir, sizeof(szDir), "stars-native-%u-\xce\xa9", (unsigned)DwTickCount());
    MakeDir(szDir); TEST_ASSERT(FFileExists(szDir));
    snprintf(szFile, sizeof(szFile), "%s%s\xe6\x9d\xb1\xe4\xba\xac-\xc3\xa9.bin", szDir, szDirSep);
    snprintf(szOther, sizeof(szOther), "%s%s\xf0\x9f\x98\x80.bin", szDir, szDirSep);
    hfFile = HfOpenFile(szFile, mdCreate, &fMissing);
    TEST_ASSERT(hfFile != -1 && !fMissing);
    TEST_CHECK(CbWriteFile(hfFile, "abcdef", 6) == 6 && CbFileSize(hfFile) == 6);
    TEST_CHECK(LSeekFile(hfFile, 2, 0) == 2 && CbReadFile(hfFile, rgb, 4) == 4 && memcmp(rgb, "cdef", 4) == 0);
    CloseFile(hfFile);
    TEST_CHECK(FFileExists(szFile) && !FFileReadOnly(szFile));
    TEST_CHECK(RenameNativeFile(szFile, szOther) == 0 && !FFileExists(szFile) && FFileExists(szOther));
    TEST_CHECK(RemoveNativeFile(szOther) == 0 && !FFileExists(szOther));
    TEST_CHECK(HfOpenFile(szOther, mdRead, &fMissing) == -1 && fMissing);
    for (i = 0; i < 150; i++) szLong[i] = 'x';
    strcpy(szLong + 150, "-\xce\xa9.bin");
    snprintf(szFile, sizeof(szFile), "%s%s%s", szDir, szDirSep, szLong);
    hfFile = HfOpenFile(szFile, mdCreate, &fMissing);
    TEST_ASSERT(hfFile != -1);
    TEST_CHECK(CbWriteFile(hfFile, "Z", 1) == 1); CloseFile(hfFile);
    TEST_CHECK(FFileExists(szFile) && RemoveNativeFile(szFile) == 0);
    TEST_CHECK(HfOpenFile("\xff", mdRead, &fMissing) == -1 && !fMissing && errno == EILSEQ);
    TEST_CHECK(RemoveNativeFile("\xff") == -1 && errno == EILSEQ);
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
