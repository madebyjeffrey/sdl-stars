#include "common.h"
#include "acutest.h"
#include "stars_test.h"

static void TestFileFailureUnwinds(void) {
    char szDir[MAX_PATH], szFile[MAX_PATH];
    int16_t fSilentSav;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("recovery", szDir, sizeof(szDir)));
    snprintf(szFile, sizeof(szFile), "%s%smissing", szDir, szDirSep);
    fSilentSav = fFileErrSilent;
    try {
        StarsRecoveryScope scopeRecovery;
        StarsRestore<int16_t> restoreSilent(fFileErrSilent);
        StarsFileGuard closeStream(hf);
        fFileErrSilent = TRUE;
        StreamOpen(szFile, mdRead);
        TEST_CHECK(FALSE);
    } catch (const StarsError &error) {
        TEST_CHECK(error.mdError == starsErrorFile);
        TEST_CHECK(hf == -1);
        TEST_CHECK(fFileErrSilent == fSilentSav);
        TEST_CHECK(!FStarsRecoveryActive());
    }
}

static void TestShortReadClosesStream(void) {
    char szDir[MAX_PATH], szFile[MAX_PATH];
    char rgb[2] = {42, 0};
    int16_t hfWrite, fMissing;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(FStarsTestDir("short-read", szDir, sizeof(szDir)));
    snprintf(szFile, sizeof(szFile), "%s%sone-byte", szDir, szDirSep);
    hfWrite = HfOpenFile(szFile, mdCreate, &fMissing);
    TEST_ASSERT(hfWrite != -1);
    TEST_ASSERT(CbWriteFile(hfWrite, rgb, 1) == 1);
    CloseFile(hfWrite);
    try {
        StarsRecoveryScope scopeRecovery;
        StarsRestore<int16_t> restoreSilent(fFileErrSilent);
        StarsFileGuard closeStream(hf);
        fFileErrSilent = TRUE;
        StreamOpen(szFile, mdRead);
        RgFromStream(rgb, sizeof(rgb));
        TEST_CHECK(FALSE);
    } catch (const StarsError &error) {
        TEST_CHECK(error.mdError == starsErrorFile);
        TEST_CHECK(error.ids == idsGameFileAppearsCorruptUnableLoadFile);
        TEST_CHECK(hf == -1 && !FStarsRecoveryActive());
    }
    TEST_CHECK(RemoveNativeFile(szFile) == 0);
}

static void TestNestedRecoveryRestoresParent(void) {
    TEST_ASSERT(!FStarsRecoveryActive());
    {
        StarsRecoveryScope scopeOuter;
        try {
            StarsRecoveryScope scopeInner;
            StarsThrowError(starsErrorFile, idsCantOpenFile);
        } catch (const StarsError &) {
            TEST_CHECK(FStarsRecoveryActive());
        }
        TEST_CHECK(FStarsRecoveryActive());
    }
    TEST_CHECK(!FStarsRecoveryActive());
}

TEST_LIST = {{"file failure restores scope and flags", TestFileFailureUnwinds},
             {"short read closes the owned stream", TestShortReadClosesStream},
             {"nested recovery retains its parent", TestNestedRecoveryRestoresParent},
             {NULL, NULL}};
