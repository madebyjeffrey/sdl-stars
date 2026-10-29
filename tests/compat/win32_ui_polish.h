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

static BOOL FCheckPlanetPane(void) {
    HWND hwndPane = GetParent(hwndPlanet);
    SCROLLINFO si = {sizeof(si), SIF_ALL, 0, 0, 0, 0, 0};
    RECT rcFrame, rcPane, rcControl;
    POINT pt;
    MSG msg;
    int i, idFleet = idflNone, iPosition;
    char szClass[64];

    if (!GetClassName(hwndPane, szClass, sizeof(szClass)) || strcmp(szClass, "StarsPlanetPane")) return FALSE;
    for (i = 0; i < cFleet; i++)
        if (rglpfl[i] && !rglpfl[i]->fDead && rglpfl[i]->iPlayer == idPlayer) { idFleet = rglpfl[i]->id; break; }
    if (idFleet == idflNone) return FALSE;
    GetWindowRect(hwndFrame, &rcFrame);
    ShowWindow(hwndFrame, SW_RESTORE);
    if (hwndFrame->lpWindow) SDL_SyncWindow(hwndFrame->lpWindow);
    SetWindowPos(hwndFrame, NULL, 0, 0, 520, 380, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    if (hwndFrame->lpWindow) SDL_SyncWindow(hwndFrame->lpWindow);
    SelectAdjFleet(0, (int16_t)idFleet);
    UpdateWindow(hwndPlanet);
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    GetClientRect(hwndPane, &rcPane);
    if (!GetScrollInfo(hwndPane, SB_VERT, &si) || si.nPage != (UINT)rcPane.bottom ||
        si.nMax < rcPane.bottom || si.nPos != 0) { fprintf(stderr, "Pane initial: page %u height %d max %d pos %d\n", si.nPage, rcPane.bottom, si.nMax, si.nPos); return FALSE; }
    SendMessage(hwndPane, WM_VSCROLL, MAKEWPARAM(SB_THUMBTRACK, 60), 0);
    if (GetScrollPos(hwndPane, SB_VERT) != 60) { fprintf(stderr, "Pane track: pos %d max %d page %u\n", GetScrollPos(hwndPane, SB_VERT), si.nMax, si.nPage); return FALSE; }
    pt = (POINT){0, 0}; ClientToScreen(hwndPlanet, &pt); ScreenToClient(hwndPane, &pt);
    if (pt.y != -60) { fprintf(stderr, "Pane origin: y %d\n", pt.y); return FALSE; }
    SendMessage(hwndPane, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
    iPosition = GetScrollPos(hwndPane, SB_VERT);
    if (iPosition != si.nMax - (int)si.nPage + 1) { fprintf(stderr, "Pane bottom: pos %d max %d page %u\n", iPosition, si.nMax, si.nPage); return FALSE; }
    SendMessage(hwndPane, WM_MOUSEWHEEL, MAKEWPARAM(0, 120), 0);
    if (GetScrollPos(hwndPane, SB_VERT) >= iPosition) { fprintf(stderr, "Pane wheel: pos %d previous %d\n", GetScrollPos(hwndPane, SB_VERT), iPosition); return FALSE; }
    // Address the actual lower fleet controls using their hierarchy rectangles.
    GetWindowRect(hwndRepCB, &rcControl);
    pt = (POINT){rcControl.left, rcControl.bottom}; ScreenToClient(hwndPane, &pt);
    SendMessage(hwndPane, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, (((0) > (GetScrollPos(hwndPane, SB_VERT) + pt.y - rcPane.bottom)) ? (0) : (GetScrollPos(hwndPane, SB_VERT) + pt.y - rcPane.bottom))), 0);
    GetWindowRect(hwndRepCB, &rcControl);
    pt = (POINT){rcControl.left, rcControl.bottom}; ScreenToClient(hwndPane, &pt);
    if (!IsWindowVisible(hwndRepCB) || pt.y > rcPane.bottom || pt.y < 0) { fprintf(stderr, "Pane lower: visible %d bottom %d height %d scroll %d\n", IsWindowVisible(hwndRepCB), pt.y, rcPane.bottom, GetScrollPos(hwndPane, SB_VERT)); return FALSE; }
    SetWindowPos(hwndFrame, NULL, 0, 0, rcFrame.right - rcFrame.left, rcFrame.bottom - rcFrame.top,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    puts("Planet/fleet viewport: proportional page, live thumb, wheel, clamp and lower-control reachability passed");
    return TRUE;
}
