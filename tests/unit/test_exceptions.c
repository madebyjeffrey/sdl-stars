#include "common.h"
#include "acutest.h"
#include "stars_test.h"

static void TestFileFailureUnwinds(void) {
    char szDir[MAX_PATH], szFile[MAX_PATH + 64];
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
    char szDir[MAX_PATH], szFile[MAX_PATH + 64];
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

struct PENDINGCLEANUP {
    ~PENDINGCLEANUP() noexcept {
        try { StarsThrowError(starsErrorMemory, 418); }
        catch (...) { StarsCapturePendingError(); }
    }
};

static void TestCleanupKeepsInFlightFailure(void) {
    bool fCaught = false;
    TEST_ASSERT(!FStarsErrorPending());
    try {
        PENDINGCLEANUP cleanup;
        StarsThrowError(starsErrorFile, 417);
    } catch (const StarsError &error) {
        fCaught = error.mdError == starsErrorFile && error.ids == 417;
    }
    TEST_CHECK(fCaught && !FStarsErrorPending());
    // Drain a failing implementation so one assertion cannot poison later cases.
    try { StarsRethrowPendingError(); } catch (...) {}
}

static void TestCleanupDeliversWithoutPrimary(void) {
    bool fCaught = false;
    TEST_ASSERT(!FStarsErrorPending());
    { PENDINGCLEANUP cleanup; }
    TEST_CHECK(FStarsErrorPending());
    try { StarsRethrowPendingError(); }
    catch (const StarsError &error) { fCaught = error.mdError == starsErrorMemory && error.ids == 418; }
    TEST_CHECK(fCaught && !FStarsErrorPending());
}

static void TestPendingKeepsFirst(void) {
    bool fCaught = false;
    TEST_ASSERT(!FStarsErrorPending());
    try { StarsThrowError(starsErrorFile, 417); } catch (...) { StarsCapturePendingError(); }
    { PENDINGCLEANUP cleanup; }
    try { StarsRethrowPendingError(); }
    catch (const StarsError &error) { fCaught = error.mdError == starsErrorFile && error.ids == 417; }
    TEST_CHECK(fCaught && !FStarsErrorPending());
}

TEST_LIST = {{"file failure restores scope and flags", TestFileFailureUnwinds},
             {"short read closes the owned stream", TestShortReadClosesStream},
             {"nested recovery retains its parent", TestNestedRecoveryRestoresParent},
             {"cleanup retains an in-flight failure", TestCleanupKeepsInFlightFailure},
             {"standalone cleanup failures remain deliverable", TestCleanupDeliversWithoutPrimary},
             {"pending delivery keeps the first failure", TestPendingKeepsFirst},
             {NULL, NULL}};
