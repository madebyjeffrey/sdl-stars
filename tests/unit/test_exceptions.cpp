#include "common.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include "acutest.h"
#include "stars_test.h"

static void TestFileFailureUnwinds(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[cbTestPath], szFile[cbTestPath + 64];
    int16_t fSilentSav;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("recovery", szDir, sizeof(szDir)));
    snprintf(szFile, sizeof(szFile), "%s%smissing", szDir, szDirSep);
    fSilentSav = app.state.fFileErrSilent;
    try {
        StarsRecoveryScope scopeRecovery;
        StarsRestore<int16_t> restoreSilent(app.state.fFileErrSilent);
        StarsFileGuard closeStream(app.state.hf);
        app.state.fFileErrSilent = 1;
        StreamOpen(app, szFile, mdRead);
        TEST_CHECK(0);
    } catch (const StarsError &error) {
        TEST_CHECK(error.mdError == starsErrorFile);
        TEST_CHECK(app.state.hf == -1);
        TEST_CHECK(app.state.fFileErrSilent == fSilentSav);
        TEST_CHECK(!FStarsRecoveryActive());
    }
}

static void TestShortReadClosesStream(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[cbTestPath], szFile[cbTestPath + 64];
    char rgb[2] = {42, 0};
    int16_t hfWrite;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("short-read", szDir, sizeof(szDir)));
    snprintf(szFile, sizeof(szFile), "%s%sone-byte", szDir, szDirSep);
    hfWrite = HfOpenFile(szFile, {.mdAccess = NATIVEFILEACCESS::readWrite, .mdShare = NATIVEFILESHARE::exclusive, .fCreate = true, .fTruncate = true}).hf;
    TEST_ASSERT(hfWrite != -1);
    TEST_ASSERT(CbWriteFile(hfWrite, rgb, 1) == 1);
    CloseFile(hfWrite);
    try {
        StarsRecoveryScope scopeRecovery;
        StarsRestore<int16_t> restoreSilent(app.state.fFileErrSilent);
        StarsFileGuard closeStream(app.state.hf);
        app.state.fFileErrSilent = 1;
        StreamOpen(app, szFile, mdRead);
        RgFromStream(app, rgb, sizeof(rgb));
        TEST_CHECK(0);
    } catch (const StarsError &error) {
        TEST_CHECK(error.mdError == starsErrorFile);
        TEST_CHECK(error.ids == idsGameFileAppearsCorruptUnableLoadFile);
        TEST_CHECK(app.state.hf == -1 && !FStarsRecoveryActive());
    }
    TEST_CHECK(RemoveNativeFile(szFile) == 0);
}

static HDR hdrAtAlert;
static uint8_t rgbAtAlert[3];
static int cHeaderAlerts;

static void CaptureReadFailure() {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    hdrAtAlert = app.state.hdrCur;
    memcpy(rgbAtAlert, app.state.rgbCur, sizeof(rgbAtAlert));
    cHeaderAlerts++;
}

static void TestPartialRecordPublication() {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[cbTestPath], szFile[cbTestPath + 64];
    const uint8_t rgbFile[] = {3, 0xfc, 0x37}; // unknown type 63, three-byte payload
    const int rgcb[] = {0, 1, 3};
    int16_t hfWrite;
    bool fCaught;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("record-publication", szDir, sizeof(szDir)));
    snprintf(szFile, sizeof(szFile), "%s%spartial", szDir, szDirSep);
    for (int cb : rgcb) {
        hfWrite = HfOpenFile(szFile, {.mdAccess = NATIVEFILEACCESS::readWrite, .mdShare = NATIVEFILESHARE::exclusive, .fCreate = true, .fTruncate = true}).hf;
        TEST_ASSERT(hfWrite != -1);
        if (cb != 0) TEST_ASSERT(CbWriteFile(hfWrite, rgbFile, cb) == cb);
        CloseFile(hfWrite);
        app.state.hdrCur.cb = 0x155; app.state.hdrCur.rt = 0x2a;
        memset(app.state.rgbCur, 0xa5, sizeof(app.state.rgbCur));
        cHeaderAlerts = 0; fCaught = false;
        try {
            StarsRecoveryScope scopeRecovery;
            StarsRestore<int16_t> restoreSilent(app.state.fFileErrSilent);
            StarsRestore<decltype(pfnStarsTestAlertObserver)> restoreObserver(pfnStarsTestAlertObserver);
            StarsFileGuard closeStream(app.state.hf);
            app.state.fFileErrSilent = 0; app.state.gd.fGeneratingTurn = 0;
            pfnStarsTestAlertObserver = CaptureReadFailure;
            StreamOpen(app, szFile, mdRead);
            ReadRt(app);
        } catch (const StarsError &error) {
            fCaught = error.mdError == starsErrorFile && error.ids == idsGameFileAppearsCorruptUnableLoadFile;
        }
        TEST_CHECK(fCaught && cHeaderAlerts == 1 && app.state.hf == -1 && !FStarsRecoveryActive());
        TEST_CHECK(hdrAtAlert.cb == (cb == 0 ? 0x155 : cb == 1 ? 0x103 : 3));
        TEST_CHECK(hdrAtAlert.rt == (cb == 3 ? 63 : 0x2a));
        TEST_CHECK(rgbAtAlert[0] == (cb == 3 ? 0x37 : 0xa5) && rgbAtAlert[1] == 0xa5 && rgbAtAlert[2] == 0xa5);
        TEST_CHECK(app.state.hdrCur.cb == hdrAtAlert.cb && app.state.hdrCur.rt == hdrAtAlert.rt && !pfnStarsTestAlertObserver);
    }
    TEST_CHECK(RemoveNativeFile(szFile) == 0);
}

static void TestRecordFramingBytes() {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[cbTestPath], szFile[cbTestPath + 64];
    uint8_t rgb[1024], written[1026], stream[] = {3, 0, 0x31, 0x32, 0x33};
    int16_t hfRead;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("record-framing", szDir, sizeof(szDir)));
    snprintf(szFile, sizeof(szFile), "%s%sheader", szDir, szDirSep);
    for (int i = 0; i < 1024; i++) rgb[i] = (uint8_t)i;
    StreamOpen(app, szFile, mdCreate);
    // The historical ten-bit header wraps at 1024; the physical payload is
    // still written. Encoding must not introduce a new length rejection.
    WriteRt(app, rtEOF, 1024, rgb);
    StreamClose(app);
    hfRead = HfOpenFile(szFile, {.mdShare = NATIVEFILESHARE::denyWrite}).hf;
    TEST_ASSERT(hfRead != -1);
    TEST_CHECK(CbReadFile(hfRead, written, sizeof(written)) == sizeof(written));
    CloseFile(hfRead);
    TEST_CHECK(written[0] == 0 && written[1] == 0 && !memcmp(written + 2, rgb, sizeof(rgb)));
    app.state.vlpMemStream = stream;
    ReadRt(app);
    TEST_CHECK(app.state.hdrCur.rt == rtEOF && app.state.hdrCur.cb == 3 && app.state.vlpMemStream == stream + 5 && !memcmp(app.state.rgbCur, stream + 2, 3));
    app.state.vlpMemStream = NULL;
    StreamOpen(app, szFile, mdCreate);
    WriteRtEof(app, 0x80ff);
    StreamClose(app);
    hfRead = HfOpenFile(szFile, {.mdShare = NATIVEFILESHARE::denyWrite}).hf;
    TEST_ASSERT(hfRead != -1);
    TEST_CHECK(CbReadFile(hfRead, written, sizeof(written)) == 4);
    CloseFile(hfRead);
    TEST_CHECK(written[0] == 2 && written[1] == 0 && written[2] == 0xff && written[3] == 0x80);
    TEST_CHECK(WFromStreamBytes(written + 2) == 0x80ff);
    TEST_CHECK(RemoveNativeFile(szFile) == 0);
}

static void TestBofSignedEncryptionFields() {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    uint8_t stream[] = {16, 32, 'J', '3', 'J', '3', 1, 0, 0, 0x80, 0xa0, 0x2a, 0xfe, 0xff, 0x10, 0x80, 5, 0x10};
    int32_t lSeed1, lSeed2, expected1, expected2;

    TEST_ASSERT(FStarsTestInit(app));
    // These expectations are signed five-/eleven-bit fields, independent of
    // native bitfield layout and the stream decoder under test.
    SetFileXorStream(app, -2147483647, -1024, -2, -16, 1);
    GetFileSeeds(app, &expected1, &expected2);
    app.state.vlpMemStream = stream;
    ReadRt(app);
    GetFileSeeds(app, &lSeed1, &lSeed2);
    TEST_CHECK(app.state.hdrCur.rt == rtBOF && app.state.hdrCur.cb == 16 && app.state.vlpMemStream == stream + sizeof(stream));
    TEST_CHECK(lSeed1 == expected1 && lSeed2 == expected2);
    TEST_CHECK(BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).iPlayer() == -16 && BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).lSaltTime() == -1024);
    app.state.vlpMemStream = NULL;
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

static void TestCleanupKeepsHandledPrimary(void) {
    TEST_ASSERT(!FStarsErrorPending());
    try {
        StarsThrowError(starsErrorFile, 417);
    } catch (const StarsError &error) {
        StarsPrimaryErrorScope primary;
        TEST_CHECK(error.ids == 417);
        { PENDINGCLEANUP cleanup; }
        TEST_CHECK(!FStarsErrorPending());
    }
    try { StarsThrowError(starsErrorFile, 417); } catch (...) { StarsCapturePendingError(); }
    {
        StarsPrimaryErrorScope primary;
        { PENDINGCLEANUP cleanup; }
    }
    bool fCaught = false;
    try { StarsRethrowPendingError(); }
    catch (const StarsError &error) { fCaught = error.mdError == starsErrorFile && error.ids == 417; }
    TEST_CHECK(fCaught && !FStarsErrorPending());
}

TEST_LIST = {{"cleanup retains a primary failure already being handled", TestCleanupKeepsHandledPrimary},{"BOF signed fields retain encryption seed inputs", TestBofSignedEncryptionFields},
             {"record failures publish partial bytes before alert callbacks", TestPartialRecordPublication},
             {"record headers retain explicit framing and wrapping", TestRecordFramingBytes},
             {"file failure restores scope and flags", TestFileFailureUnwinds},
             {"short read closes the owned stream", TestShortReadClosesStream},
             {"nested recovery retains its parent", TestNestedRecoveryRestoresParent},
             {"cleanup retains an in-flight failure", TestCleanupKeepsInFlightFailure},
             {"standalone cleanup failures remain deliverable", TestCleanupDeliversWithoutPrimary},
             {"pending delivery keeps the first failure", TestPendingKeepsFirst},
             {NULL, NULL}};
