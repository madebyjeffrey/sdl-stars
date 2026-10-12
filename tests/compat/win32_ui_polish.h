/* Product navigation checks use a copied year-10 game with actual messages. */
static BOOL FCheckMessageWheel(void) {
    int iFirst, iNext;

    if (cMsg + vcmsgplrIn < 2 || gd.fSendMsgMode) return FALSE;
    SendMessage(hwndMessage, WM_KEYDOWN, VK_HOME, 1);
    iFirst = iMsgCur; iNext = IMsgNext(FALSE);
    if (iFirst == imsgNone || iNext == imsgNone || iFirst == iNext) return FALSE;
    SendMessage(hwndMessage, WM_MOUSEWHEEL, MAKEWPARAM(0, (WORD)-60), 0);
    if (iMsgCur != iFirst) return FALSE;
    SendMessage(hwndMessage, WM_MOUSEWHEEL, MAKEWPARAM(0, (WORD)-60), 0);
    if (iMsgCur != iNext) return FALSE;
    SendMessage(hwndMessage, WM_MOUSEWHEEL, MAKEWPARAM(0, 120), 0);
    if (iMsgCur != iFirst) return FALSE;
    SendMessage(hwndMessage, WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, (WORD)-120), 0);
    if (iMsgCur != iFirst) return FALSE;
    puts("Message wheel: partial notches accumulate, both directions navigate and Ctrl preserves the message");
    return TRUE;
}
