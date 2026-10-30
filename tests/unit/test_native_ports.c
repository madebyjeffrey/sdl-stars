// Native-port checks: player-message serialization and both readers,
// legacy link bytes, recipient filtering, maximum text length, static-control
// color dispatch, and the Win16 battle heap rollover boundary. CMake builds
// private test implementations of WriteRt, ReadRt, FCreateFile, StreamOpen,
// StreamClose and DirtyGame.

#include "acutest.h"
[[maybe_unused]] static char szTestLiteral1[] = "native-port-test";

#include <stdlib.h>

#include "stars_test.h"
#include "liststorage.h"

static uint8_t rgRecord[4][1024];
static int16_t rgcbRecord[4];
static RecordType rgrtRecord[4];
static int     cRecord;
static int     iRead;
static int     fLogRead;
static int     cDirty;

void StarsTest_WriteRt(RecordType rt, int16_t cb, void *rg) {
    if (rt != rtPlrMsg && rt != rtMsg)
        return;
    TEST_ASSERT(cRecord < 4 && cb >= (rt == rtPlrMsg ? 12 : 0) && cb <= 1023);
    rgrtRecord[cRecord] = rt;
    rgcbRecord[cRecord] = cb;
    memcpy(rgRecord[cRecord++], rg, cb);
}

int16_t StarsTest_FCreateFile(DtFileType dt, int16_t iPlayer, char *psz) { return TRUE; }
void    StarsTest_StreamClose(void) {}
void    StarsTest_StreamOpen(const char *szFile, MdOpenFlags mdOpen) {}
void    StarsTest_DirtyGame(int16_t fDirty) { if (fDirty) cDirty++; }

void StarsTest_ReadRt(void) {
    memset(rgbCur, 0, sizeof(rgbCur));
    if (fLogRead && iRead == -2) {
        hdrCur.rt = rtBOF;
        hdrCur.cb = 16;
        RawStore32(rgbCur + 4, game.lid);
        RawStore16(rgbCur + 8, (2 << 12) | (85 << 5));
        RawStore16(rgbCur + 10, game.turn);
        RawStore16(rgbCur + 14, game.wGen << 13);
    } else if (fLogRead && iRead == -1) {
        hdrCur.rt = 9;
        hdrCur.cb = 17;
    } else if (iRead < cRecord) {
        hdrCur.rt = rgrtRecord[iRead];
        hdrCur.cb = rgcbRecord[iRead];
        memcpy(rgbCur, rgRecord[iRead], hdrCur.cb);
    } else {
        hdrCur.rt = rtEOF;
        hdrCur.cb = 0;
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
    vlpmsgplrOut = mp;
    vcmsgplrOut = 3;
    lpMsg = rgbMsgBuffer;
    imemMsgCur = 0;
    imemLogCur = 0;
    idPlayer = -1;
    hdrPrev.rt = rtLogPlayerZpq1;

    cRecord = 0;
    WritePlayerMessages(1);
    TEST_ASSERT(cRecord == 2);
    CheckRecord(0, mp);
    CheckRecord(1, mp2);
    cRecord = 0;
    WritePlayerMessages(0);
    TEST_ASSERT(cRecord == 0);
    WritePlayerMessages(-1);
    TEST_ASSERT(cRecord == 0);

    cRecord = 0;
    TEST_ASSERT(FWriteLogFile(szTestLiteral1, 0) == TRUE);
    TEST_ASSERT(cRecord == 3);
    CheckRecord(0, mp);
    CheckRecord(1, mp2);
    CheckRecord(2, mp3);

    /* Old saves contain an arbitrary four-byte link. It must be ignored. */
    RawStore32(rgRecord[0], 0xdeadbeef);
    RawStore32(rgRecord[1], 0xffffffff);
    iRead = 0;
    fLogRead = FALSE;
    StarsTest_ReadRt();
    vlpmsgplrIn = NULL;
    vcmsgplrIn = 0;
    cMsg = 0;
    ReadPlayerMessages();
    TEST_ASSERT(vcmsgplrIn == 3);
    CheckReadMessage(vlpmsgplrIn, mp);
    CheckReadMessage(vlpmsgplrIn->lpmsgplrNext, mp2);
    CheckReadMessage(vlpmsgplrIn->lpmsgplrNext->lpmsgplrNext, mp3);
    TEST_ASSERT(vlpmsgplrIn->lpmsgplrNext->lpmsgplrNext->lpmsgplrNext == NULL);

    vlpmsgplrOut = NULL;
    vcmsgplrOut = 0;
    iRead = -2;
    fLogRead = TRUE;
    TEST_ASSERT(FLoadLogFile(szTestLiteral1) == TRUE);
    TEST_ASSERT(vcmsgplrOut == 3);
    CheckReadMessage(vlpmsgplrOut, mp);
    CheckReadMessage(vlpmsgplrOut->lpmsgplrNext, mp2);
    CheckReadMessage(vlpmsgplrOut->lpmsgplrNext->lpmsgplrNext, mp3);
    TEST_ASSERT(vlpmsgplrOut->lpmsgplrNext->lpmsgplrNext->lpmsgplrNext == NULL);
    // An invalid first record does not prevent later records from appending
    // to an existing list, and the old tail remains the publication slot.
    rgRecord[0][10] = 0xff; rgRecord[0][11] = 0x7f;
    fLogRead = FALSE; iRead = 0; StarsTest_ReadRt();
    ReadPlayerMessages();
    TEST_CHECK(vcmsgplrIn == 5);
    CheckReadMessage(vlpmsgplrIn->lpmsgplrNext->lpmsgplrNext->lpmsgplrNext, mp2);
    CheckReadMessage(vlpmsgplrIn->lpmsgplrNext->lpmsgplrNext->lpmsgplrNext->lpmsgplrNext, mp3);
    free(mp);
    free(mp2);
    free(mp3);
    FreeHb(rglphb[htPlrMsg]);
    rglphb[htPlrMsg] = NULL;
    vlpmsgplrIn = vlpmsgplrOut = NULL;
}

static void TestMessageTextBounds(void) {
    RTPLRMSG msg{};

    TEST_ASSERT(FStarsTestInit());
    memset(rgbCur, 0, sizeof(rgbCur));
    hdrCur.cb = cbWirePlrMsg + 3;
    msg.cLen = -3;
    FEncodePlrMsgPrefix(msg, std::as_writable_bytes(std::span(rgbCur)));
    memcpy((rgbCur + cbWirePlrMsg), "\xc3\xa9\0", 3);
    TEST_CHECK(LpmsgplrFromRt() != NULL);
    (rgbCur + cbWirePlrMsg)[2] = 'x';
    TEST_CHECK(LpmsgplrFromRt() == NULL);
    (rgbCur + cbWirePlrMsg)[2] = 0; (rgbCur + cbWirePlrMsg)[0] = 0xff;
    TEST_CHECK(LpmsgplrFromRt() == NULL);
    msg.cLen = 1; FEncodePlrMsgPrefix(msg, std::as_writable_bytes(std::span(rgbCur))); (rgbCur + cbWirePlrMsg)[0] = 0x0b;
    hdrCur.cb = cbWirePlrMsg + 1;
    TEST_CHECK(LpmsgplrFromRt() == NULL);
}

static void TestRawMessageEdit(void) {
    HWND hwnd;
    const char *psz = "\xe6\x9d\xb1\xe4\xba\xac";

    TEST_ASSERT(FStarsTestInit());
    hwnd = CreateWindow("STATIC", "", WS_OVERLAPPEDWINDOW, 0, 0, 200, 100, NULL, NULL, hInst, NULL);
    TEST_ASSERT(hwnd != NULL);
    hwndMsgEdit = CreateWindow("EDIT", "", WS_CHILD | ES_MULTILINE, 0, 0, 100, 30, hwnd, NULL, hInst, NULL);
    hwndMsgDrop = CreateWindow("COMBOBOX", "", WS_CHILD | CBS_DROPDOWNLIST, 0, 40, 100, 30, hwnd, NULL, hInst, NULL);
    TEST_ASSERT(hwndMsgEdit != NULL && hwndMsgDrop != NULL);
    SendMessage(hwndMsgDrop, CB_ADDSTRING, 0, (LPARAM)"Everyone");
    SendMessage(hwndMsgDrop, CB_SETCURSEL, 0, 0);
    vlpmsgplrOut = NULL; vcmsgplrOut = 0; iMsgSendCur = 0;
    TEST_ASSERT(SetWindowText(hwndMsgEdit, psz));
    TEST_ASSERT(FFinishPlrMsgEntry(0));
    TEST_ASSERT(vlpmsgplrOut != NULL && vlpmsgplrOut->cLen == -7);
    cDirty = 0;
    TEST_ASSERT(FFinishPlrMsgEntry(0));
    TEST_CHECK(cDirty == 0 && vcmsgplrOut == 1);
    TEST_CHECK(!strcmp((char *)vlpmsgplrOut->rgbMsg(), psz));
    TEST_ASSERT(SetWindowText(hwndMsgEdit, "\xe4\xba\xac\xe6\x9d\xb1"));
    TEST_ASSERT(FFinishPlrMsgEntry(0));
    TEST_CHECK(cDirty == 1 && !strcmp((char *)vlpmsgplrOut->rgbMsg(), "\xe4\xba\xac\xe6\x9d\xb1"));
    iMsgSendCur = vcmsgplrOut;
    TEST_ASSERT(SetWindowText(hwndMsgEdit, "Second"));
    TEST_ASSERT(FFinishPlrMsgEntry(0));
    TEST_CHECK(vcmsgplrOut == 2 && vlpmsgplrOut->lpmsgplrNext != NULL);
    TEST_CHECK(!FFinishPlrMsgEntry(1000));
    TEST_CHECK(vcmsgplrOut == 1 && vlpmsgplrOut->lpmsgplrNext == NULL && iMsgSendCur == 0);
    TEST_CHECK(!FFinishPlrMsgEntry(1000));
    TEST_CHECK(vcmsgplrOut == 0 && vlpmsgplrOut == NULL);
    DestroyWindow(hwnd); hwndMsgEdit = hwndMsgDrop = NULL;
}

static void TestStaticColor(void) {
    hbrButtonFace = (HBRUSH)(uintptr_t)0x1234;
    TEST_ASSERT(RandomSeedDlg(NULL, WM_CTLCOLORSTATIC, 0, 0x1234) == (INT_PTR)hbrButtonFace);
    TEST_ASSERT(RandomSeedDlg(NULL, WM_CTLCOLORMSGBOX, 0, 0x60001) == 0);
}

static void TestBattleRollover(void) {
    FLEET    fl;
    uint16_t grfAttack[16];
    uint8_t *lpbStart;
    uint8_t *lpbScratch;
    TOK      rgtok[256];

    memset(&fl, 0, sizeof(fl));
    memset(grfAttack, 0, sizeof(grfAttack));
    fl.idPlanet = -1;
    fl.lpflNext = &fl;
    vrgtok = rgtok;
    lpbBattleLog = (uint8_t *)(LpAlloc(0xffc8, htBattle));
    lpbBattleT = (uint8_t *)(LpAlloc(0xffc8, htBattle));
    lpbStart = lpbBattleLog;
    lpbScratch = lpbBattleT;

    /* Win16: data starts at offset 18, leaving 65462 bytes to the limit. */
    lpbBattleCur = lpbStart + 65448;
    TEST_ASSERT(FDoCoolBattle(&fl, 0, grfAttack, 0, 0) == 1);
    TEST_ASSERT(lpbBattleT == lpbScratch);
    TEST_ASSERT(lpbBattleCur == lpbStart + 65462);
    TEST_ASSERT(BdvFromLp(lpbStart + 65448).cbData() == 14);
    TEST_ASSERT(FDoCoolBattle(&fl, 0, grfAttack, 0, 0) == 1);
    TEST_ASSERT(lpbBattleT == NULL);
    TEST_ASSERT(RawLoad16(lpbStart + 65462) == 0xffff);
    TEST_ASSERT(lpbBattleCur == lpbScratch + 14);
    FreeHb(rglphb[htBattle]);
    rglphb[htBattle] = NULL;
    lpbBattleLog = lpbBattleCur = lpbBattleT = NULL;
    vrgtok = NULL;
}

// A fleet's goto has its high bit set, so it is negative as a MsgGoto. The
// stored word was compared unsigned, so fleet messages never matched.
static void TestFleetMessageGoto(void) {
    const uint8_t rgb[] = {1, (uint8_t)(idmHasCompletedAssignedOrders & 255), (uint8_t)(idmHasCompletedAssignedOrders >> 8), 5, 0x80,
                          1, (uint8_t)(idmHasCompletedAssignedOrders & 255), (uint8_t)(idmHasCompletedAssignedOrders >> 8), 5, 0};
    MsgGoto iObj = static_cast<MsgGoto>(static_cast<int16_t>(0x8005));

    TEST_ASSERT(FStarsTestInit());
    memcpy(lpMsg, rgb, sizeof(rgb));
    imemMsgCur = sizeof(rgb);
    TEST_CHECK(FFindPlayerMessage(1, idmHasCompletedAssignedOrders, iObj));
    TEST_CHECK(FRemovePlayerMessage(1, idmHasCompletedAssignedOrders, iObj) == 1);
    TEST_CHECK(MtvFromLp(lpMsg).msghdr().iMsg() == 0x1ff);
    TEST_CHECK(MtvFromLp(lpMsg + 5).msghdr().iMsg() == idmHasCompletedAssignedOrders);
    TEST_CHECK(!FFindPlayerMessage(1, idmHasCompletedAssignedOrders, iObj));
    imemMsgCur = 0;
}

// A valid raw string may carry trailing record bytes beyond its first NUL;
// the reader admits and retains those bytes, and ignores arbitrary link markers.
static void TestPlayerMessagePrefixAndTail(void) {
    const uint8_t wire[] = {0xef, 0xbe, 0xad, 0xde, 0, 0x80, 0xff, 0x7f, 0xff, 0xff, 0xfe, 0xff, 'A', 0, 0xff, 0x80};
    MSGPLR *lpmp;

    TEST_ASSERT(FStarsTestInit());
    memcpy(rgbCur, wire, sizeof(wire)); hdrCur.cb = sizeof(wire);
    lpmp = LpmsgplrFromRt();
    TEST_ASSERT(lpmp != NULL);
    TEST_CHECK(lpmp->lpmsgplrNext == NULL && lpmp->iPlrFrom == -32768 && lpmp->iPlrTo == 32767 && lpmp->iInRe == -1 && lpmp->cLen == -2);
    TEST_CHECK(memcmp(lpmp->rgbMsg(), wire + 12, 4) == 0);
    TEST_CHECK(CbPlSlot(lpmp) >= sizeof(MSGPLR) + 4);
    FreeLp(lpmp, htPlrMsg);
    hdrCur.cb = 11;
    TEST_CHECK(LpmsgplrFromRt() == NULL);
    hdrCur.cb = 12; rgbCur[10] = 0; rgbCur[11] = (char)0x80;
    TEST_CHECK(LpmsgplrFromRt() == NULL);
}

// Restart header/byte-array lifetimes without changing physical slot capacity,
// the list link, retained spare bytes or the allocator's move/shrink policy.
static void TestPlayerMessageStorage(void) {
    MSGPLR *lpmp, *lpmpOld, *lpmpNext;
    uint8_t rgb[128];
    uint16_t cb, i;

    TEST_ASSERT(FStarsTestInit());
    lpmp = LpmsgplrAlloc(sizeof(MSGPLR) + 17);
    lpmpNext = LpmsgplrAlloc(sizeof(MSGPLR) + 1);
    lpmp->lpmsgplrNext = lpmpNext;
    lpmp->iPlrFrom = 3; lpmp->iPlrTo = 4; lpmp->iInRe = -1; lpmp->cLen = -17;
    cb = CbPlSlot(lpmp) - sizeof(MSGPLR);
    TEST_ASSERT(cb <= sizeof(rgb));
    for (i = 0; i < cb; i++) rgb[i] = lpmp->rgbMsg()[i] = (uint8_t)(i * 7);
    lpmpOld = lpmp;
    lpmp = LpmsgplrReAlloc(lpmp, sizeof(MSGPLR) + cb + 32);
    TEST_CHECK(lpmp != lpmpOld && lpmp->lpmsgplrNext == lpmpNext);
    TEST_CHECK(lpmp->iPlrFrom == 3 && lpmp->iPlrTo == 4 && lpmp->iInRe == -1 && lpmp->cLen == -17);
    TEST_CHECK(memcmp(lpmp->rgbMsg(), rgb, cb) == 0);
    lpmpOld = lpmp;
    lpmp = LpmsgplrReAlloc(lpmp, sizeof(MSGPLR) + 1);
    TEST_CHECK(lpmp == lpmpOld && lpmp->lpmsgplrNext == lpmpNext && memcmp(lpmp->rgbMsg(), rgb, cb) == 0);
    FreeLp(lpmp, htPlrMsg); FreeLp(lpmpNext, htPlrMsg);
}

static void TestPackedMessageParameters(void) {
    const uint8_t expected[] = {0x46, 0x78, 0xf9, 0xff, 0, 0xff, 0, 1, 0xff, 0xff, 0, 0x80, 0xff, 0x7f, 1,
                                5, 4, 5, 0x80, 0xff, 0xff, 0xff};
    const int16_t params[] = {0, 255, 256, -1, -32768, 32767, 1};
    MSGBIG mb;
    int16_t i;

    TEST_ASSERT(FStarsTestInit());
    TEST_ASSERT(rgcMsgArgs[idmMultitudeEnemiesHaveMountedProngAttackResulting] == 2 && rgcMsgArgs[idmAttemptedTransferSuccessfullyReceived] == 7);
    ResetMessages();
    TEST_ASSERT(FSendPlrMsg(1, idmMultitudeEnemiesHaveMountedProngAttackResulting, static_cast<MsgGoto>(-32763), 255, -1, 0, 0, 0, 0, 0));
    TEST_ASSERT(FSendPrependedPlrMsg(1, idmAttemptedTransferSuccessfullyReceived, gotoBattleReport, 0, 255, 256, -1, -32768, 32767, 1));
    TEST_ASSERT(FSendPlrMsg(2, idmMultitudeEnemiesHaveMountedProngAttackResulting, gotoNone, -32768, 7, 0, 0, 0, 0, 0));
    TEST_CHECK(cMsg == 3 && imemMsgCur == 32 && lpMsg[0] == 0xb1 && lpMsg[16] == 0x31);
    cRecord = 0;
    WritePlayerMessages(1);
    TEST_ASSERT(cRecord == 1 && rgrtRecord[0] == rtMsg && rgcbRecord[0] == sizeof(expected));
    TEST_CHECK(memcmp(rgRecord[0], expected, sizeof(expected)) == 0);
    ResetMessages(); fLogRead = FALSE; iRead = 0; StarsTest_ReadRt();
    ReadPlayerMessages();
    TEST_CHECK(cMsg == 2 && imemMsgCur == sizeof(expected) && memcmp(lpMsg, expected, sizeof(expected)) == 0);
    TEST_ASSERT(FGetNMsgbig(static_cast<MessageId>(0), &mb));
    TEST_CHECK(mb.iMsg == idmAttemptedTransferSuccessfullyReceived && mb.wGoto == gotoBattleReport);
    for (i = 0; i < 7; i++) TEST_CHECK(mb.rgParam[i] == params[i]);
    TEST_ASSERT(FGetNMsgbig(static_cast<MessageId>(1), &mb));
    TEST_CHECK(mb.iMsg == idmMultitudeEnemiesHaveMountedProngAttackResulting && mb.wGoto == static_cast<MsgGoto>(-32763));
    TEST_CHECK(mb.rgParam[0] == 255 && mb.rgParam[1] == -1);
    // A one-byte record retains the rest of the previous physical backing;
    // the existing reader still admits the first complete stale message there.
    rgcbRecord[0] = 1;
    ResetMessages(); iRead = 0; StarsTest_ReadRt(); ReadPlayerMessages();
    TEST_CHECK(cMsg == 1 && imemMsgCur == 1);
    TEST_ASSERT(FGetNMsgbig(static_cast<MessageId>(0), &mb));
    TEST_CHECK(mb.iMsg == idmAttemptedTransferSuccessfullyReceived && mb.rgParam[4] == -32768 && mb.rgParam[6] == 1);
}

static void TestMessagePackingAdmissionAndPrepend(void) {
    const uint8_t expected[] = {5, 2, 0xff, 0xff, 0, 1, 1, 5, 4, 5, 0x80, 0xff, 0xff, 0xff};
    uint8_t rgb[40];
    int16_t cb;
    int32_t seed1, seed2;
    MSGBIG mb;

    TEST_ASSERT(FStarsTestInit());
    ResetMessages(); seed1 = lRandSeed1; seed2 = lRandSeed2;
    memset(rgb, 0xa5, sizeof(rgb));
    TEST_CHECK(PackageUpMsg(rgb, iplrNone, idmMultitudeEnemiesHaveMountedProngAttackResulting, gotoNone, 255, -1, 0, 0, 0, 0, 0) == 0);
    for (uint8_t b : rgb) TEST_CHECK(b == 0xa5);
    rgplr[1].fAi = TRUE; rgplr[1].idAi = idAiRobotoid;
    TEST_CHECK(PackageUpMsg(rgb, 1, idmMultitudeEnemiesHaveMountedProngAttackResulting, gotoNone, 255, -1, 0, 0, 0, 0, 0) == 0);
    for (uint8_t b : rgb) TEST_CHECK(b == 0xa5);
    rgplr[1].fAi = FALSE;
    imemMsgCur = static_cast<int16_t>(0xffc8 - 20);
    cb = PackageUpMsg(rgb, 1, idmMultitudeEnemiesHaveMountedProngAttackResulting, gotoNone, 255, -1, 0, 0, 0, 0, 0);
    TEST_CHECK(cb == 8);
    imemMsgCur++; memset(rgb, 0xa5, sizeof(rgb));
    TEST_CHECK(PackageUpMsg(rgb, 1, idmMultitudeEnemiesHaveMountedProngAttackResulting, gotoNone, 255, -1, 0, 0, 0, 0, 0) == -1);
    for (uint8_t b : rgb) TEST_CHECK(b == 0xa5);
    TEST_CHECK(lRandSeed1 == seed1 && lRandSeed2 == seed2);
    ResetMessages();
    TEST_ASSERT(FSendPlrMsg2XGen(FALSE, idmMultitudeEnemiesHaveMountedProngAttackResulting, static_cast<MsgGoto>(-32763), 255, -1));
    TEST_ASSERT(FSendPlrMsg2XGen(TRUE, idmMultitudeEnemiesHaveMountedProngAttackResulting, gotoNone, 256, 1));
    TEST_CHECK(imemMsgCur == sizeof(expected) && cMsg == 2 && memcmp(lpMsg, expected, sizeof(expected)) == 0);
    TEST_CHECK(bitfMsgSent[0] & (1 << 5));
    TEST_ASSERT(FGetNMsgbig(static_cast<MessageId>(0), &mb));
    TEST_CHECK(mb.wGoto == gotoNone && mb.rgParam[0] == 256 && mb.rgParam[1] == 1);
}

TEST_LIST = {{"packed message parameters preserve framing and scratch admission", TestPackedMessageParameters},
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
