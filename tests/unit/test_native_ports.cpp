// Native-port checks: player-message serialization and both readers,
// legacy link bytes, recipient filtering, maximum text length, static-control
// color dispatch, and the Win16 battle heap rollover boundary. CMake builds
// private test implementations of WriteRt, ReadRt, FCreateFile, StreamOpen,
// StreamClose and DirtyGame.

#include "acutest.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#include "logui.h"
#include "mdi.h"
#endif
#include "callbackcontext.h"
[[maybe_unused]] static char szTestLiteral1[] = "native-port-test";

#include <stdlib.h>

#include "stars_test.h"
#include "../../src/core/record_io.h"

#include "liststorage.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#include <stars_compat.h>
#include <errno.h>
std::size_t CCompatMemoryBlocks();
#endif

// Module resource bytes are borrowed, and cleanup must retain the primary error.
#ifdef STARS_UI_SDL
static void FailTutorialStreamOpen(APPSESSION &, const char *, MdOpenFlags) {
    win32::SetLastError(win32::ERROR_DISK_FULL);
    StarsThrowError(starsErrorFile, 0);
}
static void TestTutorialResourceBorrow(void) {
    APPSESSION &app = ui::app;
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    TEST_ASSERT(FStarsTestInit(app));
    win32::SetLastError(0x4321);
    TEST_ASSERT(FWriteTutorialMFile(app, 1000) == 2);
    TEST_CHECK(win32::GetLastError() == 0x4321);
    TEST_CHECK(app.state.vlpMemStream == nullptr);
    RECORDIO io = RecordIO();
    io.pfnStreamOpen = FailTutorialStreamOpen;
    RECORDIOSCOPE recordScope(io);
    TEST_CHECK(FWriteTutorialMFile(app, 0) == 0);
    TEST_CHECK(win32::GetLastError() == win32::ERROR_DISK_FULL);
    TEST_CHECK(app.state.vlpMemStream == nullptr && !FStarsErrorPending());
}
static void TestTitleBitmapRelease(void) {
    APPSESSION &app = ui::app;
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(!app.lpui->vhdibTitle);
    const std::size_t cMemory = CCompatMemoryBlocks();
    app.lpui->vhdibTitle = HdibLoadBigResource(app, IDDIB_SPLASH);
    TEST_ASSERT(app.lpui->vhdibTitle);
    TEST_CHECK(CCompatMemoryBlocks() == cMemory + 1);
    StarsRestore fFreeingTitle(app.lpui->fFreeingTitle);
    app.lpui->fFreeingTitle = 1;
    TitleWndProc(nullptr, win32::WM_DESTROY, 0, 0);
    TEST_CHECK(!app.lpui->vhdibTitle && CCompatMemoryBlocks() == cMemory);
    TitleWndProc(nullptr, win32::WM_DESTROY, 0, 0);
    TEST_CHECK(CCompatMemoryBlocks() == cMemory);
}
#endif

static uint8_t rgRecord[4][1024];
static int16_t rgcbRecord[4];
static RecordType rgrtRecord[4];
static int     cRecord;
static int     iRead;
static int     fLogRead;
static int     cDirty;

void StarsTest_WriteRt(APPSESSION &, RecordType rt, std::span<const std::byte> rg) {
    const auto cb = rg.size();
    if (rt != rtPlrMsg && rt != rtMsg)
        return;
    TEST_ASSERT(cRecord < 4 && cb >= (rt == rtPlrMsg ? std::size_t{12} : std::size_t{0}) && cb <= 1023);
    rgrtRecord[cRecord] = rt;
    rgcbRecord[cRecord] = static_cast<int16_t>(cb);
    memcpy(rgRecord[cRecord++], rg.data(), cb);
}

int16_t StarsTest_FCreateFile(APPSESSION &, DtFileType dt, int16_t iPlayer, char *psz) { return 1; }
void    StarsTest_StreamClose(APPSESSION &) {}
void    StarsTest_StreamOpen(APPSESSION &, const char *szFile, MdOpenFlags mdOpen) {}
void    StarsTest_DirtyGame(APPSESSION &, int16_t fDirty) { if (fDirty) cDirty++; }

void StarsTest_ReadRt(APPSESSION &app) {
    memset(app.state.rgbCur, 0, sizeof(app.state.rgbCur));
    if (fLogRead && iRead == -2) {
        app.state.hdrCur.rt = rtBOF;
        app.state.hdrCur.cb = 16;
        RawStore32(app.state.rgbCur + 4, app.state.game.lid);
        RawStore16(app.state.rgbCur + 8, (2 << 12) | (85 << 5));
        RawStore16(app.state.rgbCur + 10, app.state.game.turn);
        RawStore16(app.state.rgbCur + 14, app.state.game.wGen << 13);
    } else if (fLogRead && iRead == -1) {
        app.state.hdrCur.rt = 9;
        app.state.hdrCur.cb = 17;
    } else if (iRead < cRecord) {
        app.state.hdrCur.rt = rgrtRecord[iRead];
        app.state.hdrCur.cb = rgcbRecord[iRead];
        memcpy(app.state.rgbCur, rgRecord[iRead], app.state.hdrCur.cb);
    } else {
        app.state.hdrCur.rt = rtEOF;
        app.state.hdrCur.cb = 0;
    }
    iRead++;
}

static MSGPLR *MakeMessage(int16_t from, int16_t to, int16_t len) {
    MSGPLR *mp;
    int     i;

    mp = (MSGPLR *)malloc(sizeof(MSGPLR) + abs(len));
    TEST_ASSERT(mp != NULL);
    mp = LpStartPl<MSGPLR, uint8_t>(mp, sizeof(MSGPLR) + abs(len));
    mp->lpmsgplrNext = NULL;
    mp->iPlrFrom = from;
    mp->iPlrTo = to;
    mp->iInRe = 123;
    mp->cLen = len;
    for (i = 0; i < abs(len); i++)
        mp->rgbMsg()[i] = len < 0 ? 'A' + from : 0;
    if (len < 0) mp->rgbMsg()[-len - 1] = 0;
    return mp;
}

static void CheckRecord(int i, MSGPLR *mp) {
    TEST_ASSERT(rgcbRecord[i] == abs(mp->cLen) + 12);
    TEST_ASSERT(RawLoad32(rgRecord[i]) == 0);
    TEST_ASSERT(memcmp(rgRecord[i] + 4, &mp->iPlrFrom, 8) == 0);
    TEST_ASSERT(memcmp(rgRecord[i] + 12, mp->rgbMsg(), abs(mp->cLen)) == 0);
}

static void CheckReadMessage(MSGPLR *actual, MSGPLR *expected) {
    TEST_ASSERT(actual != NULL);
    TEST_ASSERT(actual->iPlrFrom == expected->iPlrFrom);
    TEST_ASSERT(actual->iPlrTo == expected->iPlrTo);
    TEST_ASSERT(actual->iInRe == expected->iInRe);
    TEST_ASSERT(actual->cLen == expected->cLen);
    TEST_ASSERT(memcmp(actual->rgbMsg(), expected->rgbMsg(), abs(expected->cLen)) == 0);
}

static void TestMessages(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    MSGPLR *mp;
    MSGPLR *mp2;
    MSGPLR *mp3;
    uint8_t rgbMsgBuffer[16];

    mp = MakeMessage(0, 0, 31);
    TEST_ASSERT(sizeof(mp->iPlrFrom) == 2 && sizeof(mp->cLen) == 2);
    TEST_ASSERT((uint8_t *)mp->rgbMsg() - (uint8_t *)&mp->iPlrFrom == 8);
    mp2 = MakeMessage(1, 2, -1000);
    mp3 = MakeMessage(2, 3, 7);
    mp->lpmsgplrNext = mp2;
    mp2->lpmsgplrNext = mp3;
    app.state.vlpmsgplrOut = mp;
    app.state.vcmsgplrOut = 3;
    app.state.lpMsg = rgbMsgBuffer;
    app.state.imemMsgCur = 0;
    app.state.imemLogCur = 0;
    app.state.idPlayer = -1;
    app.state.hdrPrev.rt = rtLogPlayerZpq1;

    cRecord = 0;
    WritePlayerMessages(app, 1);
    TEST_ASSERT(cRecord == 2);
    CheckRecord(0, mp);
    CheckRecord(1, mp2);
    cRecord = 0;
    WritePlayerMessages(app, 0);
    TEST_ASSERT(cRecord == 0);
    WritePlayerMessages(app, -1);
    TEST_ASSERT(cRecord == 0);

    cRecord = 0;
    TEST_ASSERT(FWriteLogFile(app, szTestLiteral1, 0) == 1);
    TEST_ASSERT(cRecord == 3);
    CheckRecord(0, mp);
    CheckRecord(1, mp2);
    CheckRecord(2, mp3);

    /* Old saves contain an arbitrary four-byte link. It must be ignored. */
    RawStore32(rgRecord[0], 0xdeadbeef);
    RawStore32(rgRecord[1], 0xffffffff);
    iRead = 0;
    fLogRead = 0;
    StarsTest_ReadRt(app);
    app.state.vlpmsgplrIn = NULL;
    app.state.vcmsgplrIn = 0;
    app.state.cMsg = 0;
    ReadPlayerMessages(app);
    TEST_ASSERT(app.state.vcmsgplrIn == 3);
    CheckReadMessage(app.state.vlpmsgplrIn, mp);
    CheckReadMessage(app.state.vlpmsgplrIn->lpmsgplrNext, mp2);
    CheckReadMessage(app.state.vlpmsgplrIn->lpmsgplrNext->lpmsgplrNext, mp3);
    TEST_ASSERT(app.state.vlpmsgplrIn->lpmsgplrNext->lpmsgplrNext->lpmsgplrNext == NULL);

    app.state.vlpmsgplrOut = NULL;
    app.state.vcmsgplrOut = 0;
    iRead = -2;
    fLogRead = 1;
    TEST_ASSERT(FLoadLogFile(app, szTestLiteral1) == 1);
    TEST_ASSERT(app.state.vcmsgplrOut == 3);
    CheckReadMessage(app.state.vlpmsgplrOut, mp);
    CheckReadMessage(app.state.vlpmsgplrOut->lpmsgplrNext, mp2);
    CheckReadMessage(app.state.vlpmsgplrOut->lpmsgplrNext->lpmsgplrNext, mp3);
    TEST_ASSERT(app.state.vlpmsgplrOut->lpmsgplrNext->lpmsgplrNext->lpmsgplrNext == NULL);
    // An invalid first record does not prevent later records from appending
    // to an existing list, and the old tail remains the publication slot.
    rgRecord[0][10] = 0xff; rgRecord[0][11] = 0x7f;
    fLogRead = 0; iRead = 0; StarsTest_ReadRt(app);
    ReadPlayerMessages(app);
    TEST_CHECK(app.state.vcmsgplrIn == 5);
    CheckReadMessage(app.state.vlpmsgplrIn->lpmsgplrNext->lpmsgplrNext->lpmsgplrNext, mp2);
    CheckReadMessage(app.state.vlpmsgplrIn->lpmsgplrNext->lpmsgplrNext->lpmsgplrNext->lpmsgplrNext, mp3);
    free(mp);
    free(mp2);
    free(mp3);
    FreeHb(app, app.state.rglphb[htPlrMsg]);
    app.state.rglphb[htPlrMsg] = NULL;
    app.state.vlpmsgplrIn = app.state.vlpmsgplrOut = NULL;
}

static void TestMessageTextBounds(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    RTPLRMSG msg{};

    TEST_ASSERT(FStarsTestInit(app));
    memset(app.state.rgbCur, 0, sizeof(app.state.rgbCur));
    app.state.hdrCur.cb = cbWirePlrMsg + 3;
    msg.cLen = -3;
    FEncodePlrMsgPrefix(msg, std::as_writable_bytes(std::span(app.state.rgbCur)));
    memcpy((app.state.rgbCur + cbWirePlrMsg), "\xc3\xa9\0", 3);
    TEST_CHECK(LpmsgplrFromRt(app) != NULL);
    (app.state.rgbCur + cbWirePlrMsg)[2] = 'x';
    TEST_CHECK(LpmsgplrFromRt(app) == NULL);
    (app.state.rgbCur + cbWirePlrMsg)[2] = 0; (app.state.rgbCur + cbWirePlrMsg)[0] = 0xff;
    TEST_CHECK(LpmsgplrFromRt(app) == NULL);
    msg.cLen = 1; FEncodePlrMsgPrefix(msg, std::as_writable_bytes(std::span(app.state.rgbCur))); (app.state.rgbCur + cbWirePlrMsg)[0] = 0x0b;
    app.state.hdrCur.cb = cbWirePlrMsg + 1;
    TEST_CHECK(LpmsgplrFromRt(app) == NULL);
}

static void TestRawMessageEdit(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    HWND hwnd;
    const char *psz = "\xe6\x9d\xb1\xe4\xba\xac";

    TEST_ASSERT(FStarsTestInit(app));
    hwnd = ui::CreateWindowEx(0, "STATIC", "", win32::WS_OVERLAPPEDWINDOW, 0, 0, 200, 100, NULL, NULL, app.lpui->hInst, NULL);
    TEST_ASSERT(hwnd != NULL);
    app.lpui->hwndMsgEdit = ui::CreateWindowEx(0, "EDIT", "", win32::WS_CHILD | win32::ES_MULTILINE, 0, 0, 100, 30, hwnd, NULL, app.lpui->hInst, NULL);
    app.lpui->hwndMsgDrop = ui::CreateWindowEx(0, "COMBOBOX", "", win32::WS_CHILD | win32::CBS_DROPDOWNLIST, 0, 40, 100, 30, hwnd, NULL, app.lpui->hInst, NULL);
    TEST_ASSERT(app.lpui->hwndMsgEdit != NULL && app.lpui->hwndMsgDrop != NULL);
    ui::SendMessage(app.lpui->hwndMsgDrop, win32::CB_ADDSTRING, 0, (LPARAM)"Everyone");
    ui::SendMessage(app.lpui->hwndMsgDrop, win32::CB_SETCURSEL, 0, 0);
    app.state.vlpmsgplrOut = NULL; app.state.vcmsgplrOut = 0; app.state.iMsgSendCur = 0;
    TEST_ASSERT(ui::SetWindowText(app.lpui->hwndMsgEdit, psz));
    TEST_ASSERT(FFinishPlrMsgEntry(app, 0));
    TEST_ASSERT(app.state.vlpmsgplrOut != NULL && app.state.vlpmsgplrOut->cLen == -7);
    cDirty = 0;
    TEST_ASSERT(FFinishPlrMsgEntry(app, 0));
    TEST_CHECK(cDirty == 0 && app.state.vcmsgplrOut == 1);
    TEST_CHECK(!strcmp((char *)app.state.vlpmsgplrOut->rgbMsg(), psz));
    TEST_ASSERT(ui::SetWindowText(app.lpui->hwndMsgEdit, "\xe4\xba\xac\xe6\x9d\xb1"));
    TEST_ASSERT(FFinishPlrMsgEntry(app, 0));
    TEST_CHECK(cDirty == 1 && !strcmp((char *)app.state.vlpmsgplrOut->rgbMsg(), "\xe4\xba\xac\xe6\x9d\xb1"));
    app.state.iMsgSendCur = app.state.vcmsgplrOut;
    TEST_ASSERT(ui::SetWindowText(app.lpui->hwndMsgEdit, "Second"));
    TEST_ASSERT(FFinishPlrMsgEntry(app, 0));
    TEST_CHECK(app.state.vcmsgplrOut == 2 && app.state.vlpmsgplrOut->lpmsgplrNext != NULL);
    TEST_CHECK(!FFinishPlrMsgEntry(app, 1000));
    TEST_CHECK(app.state.vcmsgplrOut == 1 && app.state.vlpmsgplrOut->lpmsgplrNext == NULL && app.state.iMsgSendCur == 0);
    TEST_CHECK(!FFinishPlrMsgEntry(app, 1000));
    TEST_CHECK(app.state.vcmsgplrOut == 0 && app.state.vlpmsgplrOut == NULL);
    ui::DestroyWindow(hwnd); app.lpui->hwndMsgEdit = app.lpui->hwndMsgDrop = NULL;
}

static void TestStaticColor(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    app.lpui->hbrButtonFace = (HBRUSH)(uintptr_t)0x1234;
    TEST_ASSERT(RandomSeedDlg(NULL, win32::WM_CTLCOLORSTATIC, 0, 0x1234) == (INT_PTR)app.lpui->hbrButtonFace);
    TEST_ASSERT(RandomSeedDlg(NULL, win32::WM_CTLCOLORMSGBOX, 0, 0x60001) == 0);
}

static void TestBattleRollover(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    FLEET    fl;
    uint16_t grfAttack[16];
    uint8_t *lpbStart;
    uint8_t *lpbScratch;
    TOK      rgtok[256];

    memset(&fl, 0, sizeof(fl));
    memset(grfAttack, 0, sizeof(grfAttack));
    fl.idPlanet = -1;
    fl.lpflNext = &fl;
    app.state.vrgtok = rgtok;
    app.state.lpbBattleLog = (uint8_t *)(LpAlloc(app, 0xffc8, htBattle));
    app.state.lpbBattleT = (uint8_t *)(LpAlloc(app, 0xffc8, htBattle));
    lpbStart = app.state.lpbBattleLog;
    lpbScratch = app.state.lpbBattleT;

    /* Win16: data starts at offset 18, leaving 65462 bytes to the limit. */
    app.state.lpbBattleCur = lpbStart + 65448;
    TEST_ASSERT(FDoCoolBattle(app, &fl, 0, grfAttack, 0, 0) == 1);
    TEST_ASSERT(app.state.lpbBattleT == lpbScratch);
    TEST_ASSERT(app.state.lpbBattleCur == lpbStart + 65462);
    TEST_ASSERT(BdvFromLp(app, lpbStart + 65448).cbData() == 14);
    TEST_ASSERT(FDoCoolBattle(app, &fl, 0, grfAttack, 0, 0) == 1);
    TEST_ASSERT(app.state.lpbBattleT == NULL);
    TEST_ASSERT(RawLoad16(lpbStart + 65462) == 0xffff);
    TEST_ASSERT(app.state.lpbBattleCur == lpbScratch + 14);
    FreeHb(app, app.state.rglphb[htBattle]);
    app.state.rglphb[htBattle] = NULL;
    app.state.lpbBattleLog = app.state.lpbBattleCur = app.state.lpbBattleT = NULL;
    app.state.vrgtok = NULL;
}

// A fleet's goto has its high bit set, so it is negative as a MsgGoto. The
// stored word was compared unsigned, so fleet messages never matched.
static void TestFleetMessageGoto(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    const uint8_t rgb[] = {1, (uint8_t)(idmHasCompletedAssignedOrders & 255), (uint8_t)(idmHasCompletedAssignedOrders >> 8), 5, 0x80,
                          1, (uint8_t)(idmHasCompletedAssignedOrders & 255), (uint8_t)(idmHasCompletedAssignedOrders >> 8), 5, 0};
    MsgGoto iObj = static_cast<MsgGoto>(static_cast<int16_t>(0x8005));

    TEST_ASSERT(FStarsTestInit(app));
    memcpy(app.state.lpMsg, rgb, sizeof(rgb));
    app.state.imemMsgCur = sizeof(rgb);
    TEST_CHECK(FFindPlayerMessage(app, 1, idmHasCompletedAssignedOrders, iObj));
    TEST_CHECK(FRemovePlayerMessage(app, 1, idmHasCompletedAssignedOrders, iObj) == 1);
    TEST_CHECK(MtvFromLp(app, app.state.lpMsg).msghdr().iMsg() == 0x1ff);
    TEST_CHECK(MtvFromLp(app, app.state.lpMsg + 5).msghdr().iMsg() == idmHasCompletedAssignedOrders);
    TEST_CHECK(!FFindPlayerMessage(app, 1, idmHasCompletedAssignedOrders, iObj));
    app.state.imemMsgCur = 0;
}

// A valid raw string may carry trailing record bytes beyond its first NUL;
// the reader admits and retains those bytes, and ignores arbitrary link markers.
static void TestPlayerMessagePrefixAndTail(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    const uint8_t wire[] = {0xef, 0xbe, 0xad, 0xde, 0, 0x80, 0xff, 0x7f, 0xff, 0xff, 0xfe, 0xff, 'A', 0, 0xff, 0x80};
    MSGPLR *lpmp;

    TEST_ASSERT(FStarsTestInit(app));
    memcpy(app.state.rgbCur, wire, sizeof(wire)); app.state.hdrCur.cb = sizeof(wire);
    lpmp = LpmsgplrFromRt(app);
    TEST_ASSERT(lpmp != NULL);
    TEST_CHECK(lpmp->lpmsgplrNext == NULL && lpmp->iPlrFrom == -32768 && lpmp->iPlrTo == 32767 && lpmp->iInRe == -1 && lpmp->cLen == -2);
    TEST_CHECK(memcmp(lpmp->rgbMsg(), wire + 12, 4) == 0);
    TEST_CHECK(CbPlSlot(app, lpmp) >= sizeof(MSGPLR) + 4);
    FreeLp(app, lpmp, htPlrMsg);
    app.state.hdrCur.cb = 11;
    TEST_CHECK(LpmsgplrFromRt(app) == NULL);
    app.state.hdrCur.cb = 12; app.state.rgbCur[10] = 0; app.state.rgbCur[11] = (char)0x80;
    TEST_CHECK(LpmsgplrFromRt(app) == NULL);
}

// Restart header/byte-array lifetimes without changing physical slot capacity,
// the list link, retained spare bytes or the allocator's move/shrink policy.
static void TestPlayerMessageStorage(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    MSGPLR *lpmp, *lpmpOld, *lpmpNext;
    uint8_t rgb[128];
    uint16_t cb, i;

    TEST_ASSERT(FStarsTestInit(app));
    lpmp = LpmsgplrAlloc(app, sizeof(MSGPLR) + 17);
    lpmpNext = LpmsgplrAlloc(app, sizeof(MSGPLR) + 1);
    lpmp->lpmsgplrNext = lpmpNext;
    lpmp->iPlrFrom = 3; lpmp->iPlrTo = 4; lpmp->iInRe = -1; lpmp->cLen = -17;
    cb = CbPlSlot(app, lpmp) - sizeof(MSGPLR);
    TEST_ASSERT(cb <= sizeof(rgb));
    for (i = 0; i < cb; i++) rgb[i] = lpmp->rgbMsg()[i] = (uint8_t)(i * 7);
    lpmpOld = lpmp;
    lpmp = LpmsgplrReAlloc(app, lpmp, sizeof(MSGPLR) + cb + 32);
    TEST_CHECK(lpmp != lpmpOld && lpmp->lpmsgplrNext == lpmpNext);
    TEST_CHECK(lpmp->iPlrFrom == 3 && lpmp->iPlrTo == 4 && lpmp->iInRe == -1 && lpmp->cLen == -17);
    TEST_CHECK(memcmp(lpmp->rgbMsg(), rgb, cb) == 0);
    lpmpOld = lpmp;
    lpmp = LpmsgplrReAlloc(app, lpmp, sizeof(MSGPLR) + 1);
    TEST_CHECK(lpmp == lpmpOld && lpmp->lpmsgplrNext == lpmpNext && memcmp(lpmp->rgbMsg(), rgb, cb) == 0);
    FreeLp(app, lpmp, htPlrMsg); FreeLp(app, lpmpNext, htPlrMsg);
}

static void TestPackedMessageParameters(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    const uint8_t expected[] = {0x46, 0x78, 0xf9, 0xff, 0, 0xff, 0, 1, 0xff, 0xff, 0, 0x80, 0xff, 0x7f, 1,
                                5, 4, 5, 0x80, 0xff, 0xff, 0xff};
    const int16_t params[] = {0, 255, 256, -1, -32768, 32767, 1};
    MSGBIG mb;
    int16_t i;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(rgcMsgArgs[idmMultitudeEnemiesHaveMountedProngAttackResulting] == 2 && rgcMsgArgs[idmAttemptedTransferSuccessfullyReceived] == 7);
    ResetMessages(app);
    TEST_ASSERT(FSendPlrMsg(app, 1, idmMultitudeEnemiesHaveMountedProngAttackResulting, static_cast<MsgGoto>(-32763), 255, -1, 0, 0, 0, 0, 0));
    TEST_ASSERT(FSendPrependedPlrMsg(app, 1, idmAttemptedTransferSuccessfullyReceived, gotoBattleReport, 0, 255, 256, -1, -32768, 32767, 1));
    TEST_ASSERT(FSendPlrMsg(app, 2, idmMultitudeEnemiesHaveMountedProngAttackResulting, gotoNone, -32768, 7, 0, 0, 0, 0, 0));
    TEST_CHECK(app.state.cMsg == 3 && app.state.imemMsgCur == 32 && app.state.lpMsg[0] == 0xb1 && app.state.lpMsg[16] == 0x31);
    cRecord = 0;
    WritePlayerMessages(app, 1);
    TEST_ASSERT(cRecord == 1 && rgrtRecord[0] == rtMsg && rgcbRecord[0] == sizeof(expected));
    TEST_CHECK(memcmp(rgRecord[0], expected, sizeof(expected)) == 0);
    ResetMessages(app); fLogRead = 0; iRead = 0; StarsTest_ReadRt(app);
    ReadPlayerMessages(app);
    TEST_CHECK(app.state.cMsg == 2 && app.state.imemMsgCur == sizeof(expected) && memcmp(app.state.lpMsg, expected, sizeof(expected)) == 0);
    TEST_ASSERT(FGetNMsgbig(app, static_cast<MessageId>(0), &mb));
    TEST_CHECK(mb.iMsg == idmAttemptedTransferSuccessfullyReceived && mb.wGoto == gotoBattleReport);
    for (i = 0; i < 7; i++) TEST_CHECK(mb.rgParam[i] == params[i]);
    TEST_ASSERT(FGetNMsgbig(app, static_cast<MessageId>(1), &mb));
    TEST_CHECK(mb.iMsg == idmMultitudeEnemiesHaveMountedProngAttackResulting && mb.wGoto == static_cast<MsgGoto>(-32763));
    TEST_CHECK(mb.rgParam[0] == 255 && mb.rgParam[1] == -1);
    // A one-byte record retains the rest of the previous physical backing;
    // the existing reader still admits the first complete stale message there.
    rgcbRecord[0] = 1;
    ResetMessages(app); iRead = 0; StarsTest_ReadRt(app); ReadPlayerMessages(app);
    TEST_CHECK(app.state.cMsg == 1 && app.state.imemMsgCur == 1);
    TEST_ASSERT(FGetNMsgbig(app, static_cast<MessageId>(0), &mb));
    TEST_CHECK(mb.iMsg == idmAttemptedTransferSuccessfullyReceived && mb.rgParam[4] == -32768 && mb.rgParam[6] == 1);
}

static void TestMessagePackingAdmissionAndPrepend(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    const uint8_t expected[] = {5, 2, 0xff, 0xff, 0, 1, 1, 5, 4, 5, 0x80, 0xff, 0xff, 0xff};
    uint8_t rgb[40];
    int16_t cb;
    int32_t seed1, seed2;
    MSGBIG mb;

    TEST_ASSERT(FStarsTestInit(app));
    ResetMessages(app); seed1 = app.state.lRandSeed1; seed2 = app.state.lRandSeed2;
    memset(rgb, 0xa5, sizeof(rgb));
    TEST_CHECK(PackageUpMsg(app, rgb, iplrNone, idmMultitudeEnemiesHaveMountedProngAttackResulting, gotoNone, 255, -1, 0, 0, 0, 0, 0) == 0);
    for (uint8_t b : rgb) TEST_CHECK(b == 0xa5);
    app.state.rgplr[1].fAi = 1; app.state.rgplr[1].idAi = idAiRobotoid;
    TEST_CHECK(PackageUpMsg(app, rgb, 1, idmMultitudeEnemiesHaveMountedProngAttackResulting, gotoNone, 255, -1, 0, 0, 0, 0, 0) == 0);
    for (uint8_t b : rgb) TEST_CHECK(b == 0xa5);
    app.state.rgplr[1].fAi = 0;
    app.state.imemMsgCur = static_cast<int16_t>(0xffc8 - 20);
    cb = PackageUpMsg(app, rgb, 1, idmMultitudeEnemiesHaveMountedProngAttackResulting, gotoNone, 255, -1, 0, 0, 0, 0, 0);
    TEST_CHECK(cb == 8);
    app.state.imemMsgCur++; memset(rgb, 0xa5, sizeof(rgb));
    TEST_CHECK(PackageUpMsg(app, rgb, 1, idmMultitudeEnemiesHaveMountedProngAttackResulting, gotoNone, 255, -1, 0, 0, 0, 0, 0) == -1);
    for (uint8_t b : rgb) TEST_CHECK(b == 0xa5);
    TEST_CHECK(app.state.lRandSeed1 == seed1 && app.state.lRandSeed2 == seed2);
    ResetMessages(app);
    TEST_ASSERT(FSendPlrMsg2XGen(app, 0, idmMultitudeEnemiesHaveMountedProngAttackResulting, static_cast<MsgGoto>(-32763), 255, -1));
    TEST_ASSERT(FSendPlrMsg2XGen(app, 1, idmMultitudeEnemiesHaveMountedProngAttackResulting, gotoNone, 256, 1));
    TEST_CHECK(app.state.imemMsgCur == sizeof(expected) && app.state.cMsg == 2 && memcmp(app.state.lpMsg, expected, sizeof(expected)) == 0);
    TEST_CHECK(app.state.bitfMsgSent[0] & (1 << 5));
    TEST_ASSERT(FGetNMsgbig(app, static_cast<MessageId>(0), &mb));
    TEST_CHECK(mb.wGoto == gotoNone && mb.rgParam[0] == 256 && mb.rgParam[1] == 1);
}


#ifdef STARS_UI_SDL
// Provided only by wrapper memory.c builds marked STARS_TEST_MEMORY_OBSERVE.
static bool fCleanupKeptState;
static int cSessionPayloadReleased;
static LRESULT  SessionCleanupProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    if (msg == win32::WM_APP + 77) {
        HB *lphb = app.state.rglphb[htPerm];
        FreeStuff(app);
        fCleanupKeptState = win32::IsWindow(hwnd) && lphb && app.state.rglphb[htPerm] == lphb;
        return 0;
    }
    return ui::DefWindowProc(hwnd,msg,w,l);
}
static void TestApplicationUiSessionClose(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[cbTestPath], szPath[cbTestPath + 64];
    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("session-cleanup",szDir,sizeof(szDir)));
    CchSnprintf(std::span{szPath, sizeof(szPath)},"%s%ssession.dat",szDir,szDirSep);
    NATIVEFILE *pf = PfNativeOpen(szPath,"wb");
    TEST_ASSERT(pf);
    TEST_ASSERT(CWriteNative("x",1,1,pf) == 1 && ICloseNativeFile(pf) == 0);
    const auto nfr = HfNativeOpenLegacy(szPath, {.mdShare = NATIVEFILESHARE::denyNone});
    int16_t hfSession = nfr.hf;
    TEST_ASSERT(hfSession >= 3 && !nfr.fMissing);
    WNDCLASS wc{};
    wc.lpfnWndProc = SessionCleanupProc;
    wc.hInstance = app.lpui->hInst;
    wc.lpszClassName = "StarsSessionCleanupTest";
    TEST_ASSERT(win32::RegisterClassA(&wc));
    HWND hwnd = ui::CreateWindowEx(0, wc.lpszClassName,"session",win32::WS_OVERLAPPEDWINDOW,0,0,160,120,NULL,NULL,app.lpui->hInst,NULL);
    TEST_ASSERT(hwnd);
    STARSCOMPATPAYLOAD payload(std::malloc(8), {[](void *pv) noexcept { ++cSessionPayloadReleased; std::free(pv); }});
    TEST_ASSERT(payload);
    const int cReleased = cSessionPayloadReleased;
    TEST_ASSERT(StarsCompatPostOwnedMessageAsync(StarsCompatWindowId(hwnd),win32::WM_APP+78,std::move(payload),0));
    ui::SendMessage(hwnd,win32::WM_APP+77,0,0);
    TEST_ASSERT(fCleanupKeptState && cSessionPayloadReleased == cReleased);
    FreeStuff(app);
    TEST_CHECK(!win32::IsWindow(hwnd) && !app.lpui->hwndFrame && !app.lpui->hwndPlanet && !app.lpui->hInst);
    TEST_CHECK(cSessionPayloadReleased == cReleased+1 && CCompatMemoryBlocks() == 0);
    for (auto *lphb : app.state.rglphb) TEST_CHECK(!lphb);
    TEST_CHECK(!app.state.lpLog && !app.state.lpMsg && !app.state.lpPlanets && app.state.lFileSeed1 == 0 && app.state.lFileSeed2 == 0);
    app.BindFiles();
    char ch = 0;
    errno = 0;
    TEST_CHECK(!CbNativeReadLegacy(hfSession, std::as_writable_bytes(std::span{&ch, 1})) && errno == EBADF);
    FreeStuff(app);
    TEST_CHECK(CCompatMemoryBlocks() == 0 && cSessionPayloadReleased == cReleased+1);
    TEST_ASSERT(FStarsTestInit(app));
    TEST_CHECK(app.lpui->hInst && app.state.rglphb[htPerm] && CCompatMemoryBlocks() != 0);
    FreeStuff(app); FreeStuff(app);
    TEST_CHECK(CCompatMemoryBlocks() == 0 && !FStarsErrorPending());
}

static void TestDibFromBitmapNativeQuery(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    TEST_ASSERT(FStarsTestInit(app));
    HDC hdc = win32::GetDC(NULL);
    TEST_ASSERT(hdc);
    HBITMAP hbm = win32::CreateCompatibleBitmap(hdc, 2, 2);
    TEST_ASSERT(hbm);
    HGLOBAL hdib = DibFromBitmap(hbm, win32::BI_RGB, 32, NULL);
    TEST_ASSERT(hdib);
    auto *lpbi = static_cast<BITMAPINFOHEADER *>(win32::GlobalLock(hdib));
    TEST_ASSERT(lpbi);
    TEST_CHECK(lpbi->biWidth == 2 && lpbi->biHeight == 2 && lpbi->biBitCount == 32 && lpbi->biSizeImage == 16);
    win32::GlobalUnlock(hdib);
    win32::GlobalFree(hdib);
    win32::DeleteObject(hbm);
    win32::ReleaseDC(NULL, hdc);
}
static void TestDibFromBitmapFailureReleasesStorage(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    TEST_ASSERT(FStarsTestInit(app));
    HDC hdc = win32::GetDC(NULL);
    TEST_ASSERT(hdc);
    HBITMAP hbm = win32::CreateCompatibleBitmap(hdc, 2, 2);
    TEST_ASSERT(hbm);
    const size_t cMemory = CCompatMemoryBlocks();
    TEST_CHECK(DibFromBitmap(hbm, win32::BI_RLE8, 8, NULL) == NULL);
    TEST_CHECK(CCompatMemoryBlocks() == cMemory);
    win32::DeleteObject(hbm);
    win32::ReleaseDC(NULL, hdc);
}
#endif

TEST_LIST = {
#ifdef STARS_UI_SDL
    {"tutorial resource cleanup retains errors and clears borrowed input", TestTutorialResourceBorrow},
    {"title destruction releases its writable bitmap copy once", TestTitleBitmapRelease},
    {"application session stops UI before releasing files and heaps", TestApplicationUiSessionClose},
    {"bitmap converter queries the native BITMAP width", TestDibFromBitmapNativeQuery},
    {"failed bitmap conversion releases temporary global storage", TestDibFromBitmapFailureReleasesStorage},
#endif
{"packed message parameters preserve framing and scratch admission", TestPackedMessageParameters},
             {"message packing retains early returns, capacity, RNG and prepend", TestMessagePackingAdmissionAndPrepend},
             {"player-message storage retains links, capacities and spare bytes", TestPlayerMessageStorage},
             {"player-message prefix preserves signed fields and spare tail", TestPlayerMessagePrefixAndTail},
             {"player messages", TestMessages},
             {"unchanged raw UTF-8 messages compare only their stored bytes", TestRawMessageEdit},
             {"player-message text is bounded and strictly UTF-8", TestMessageTextBounds},
             {"fleet message goto", TestFleetMessageGoto},
             {"static control color", TestStaticColor},
             {"battle heap rollover", TestBattleRollover},
             {NULL, NULL}};

static const RECORDIO io_recordio = [] { RECORDIO io; io.pfnWriteRt = StarsTest_WriteRt; io.pfnReadRt = StarsTest_ReadRt; io.pfnFCreateFile = StarsTest_FCreateFile; io.pfnStreamOpen = StarsTest_StreamOpen; io.pfnStreamClose = StarsTest_StreamClose; io.pfnDirtyGame = StarsTest_DirtyGame; return io; }();
static RECORDIOSCOPE scope_recordio(io_recordio);
