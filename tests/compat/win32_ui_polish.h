/* Product navigation checks use a copied year-10 game with actual messages. */
static BOOL FCheckMessageWheel(void) {
    int iFirst, iNext;

    if (cMsg + vcmsgplrIn < 2 || gd.fSendMsgMode) return FALSE;
    win32::SendMessageA(hwndMessage, WM_KEYDOWN, VK_HOME, 1);
    iFirst = iMsgCur; iNext = IMsgNext(FALSE);
    if (iFirst == imsgNone || iNext == imsgNone || iFirst == iNext) return FALSE;
    win32::SendMessageA(hwndMessage, WM_MOUSEWHEEL, MAKEWPARAM(0, (WORD)-60), 0);
    if (iMsgCur != iFirst) return FALSE;
    win32::SendMessageA(hwndMessage, WM_MOUSEWHEEL, MAKEWPARAM(0, (WORD)-60), 0);
    if (iMsgCur != iNext) return FALSE;
    win32::SendMessageA(hwndMessage, WM_MOUSEWHEEL, MAKEWPARAM(0, 120), 0);
    if (iMsgCur != iFirst) return FALSE;
    win32::SendMessageA(hwndMessage, WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, (WORD)-120), 0);
    if (iMsgCur != iFirst) return FALSE;
    puts("Message wheel: partial notches accumulate, both directions navigate and Ctrl preserves the message");
    return TRUE;
}

static BOOL FCheckPlanetPane(void) {
    HWND hwndPane = win32::GetParent(hwndPlanet);
    SCROLLINFO si = {sizeof(si), SIF_ALL, 0, 0, 0, 0, 0};
    RECT rcFrame, rcPane, rcControl;
    POINT pt;
    win32::MSG msg;
    int i, idFleet = idflNone, iPosition;
    char szClass[64];

    if (!win32::GetClassName(hwndPane, szClass, sizeof(szClass)) || strcmp(szClass, "StarsPlanetPane")) return FALSE;
    for (i = 0; i < cFleet; i++)
        if (rglpfl[i] && !rglpfl[i]->fDead && rglpfl[i]->iPlayer == idPlayer) { idFleet = rglpfl[i]->id; break; }
    if (idFleet == idflNone) return FALSE;
    win32::GetWindowRect(hwndFrame, &rcFrame);
    win32::ShowWindow(hwndFrame, SW_RESTORE);
    if (hwndFrame->lpWindow.get()) SDL_SyncWindow(hwndFrame->lpWindow.get());
    win32::SetWindowPos(hwndFrame, NULL, 0, 0, 520, 380, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    if (hwndFrame->lpWindow.get()) SDL_SyncWindow(hwndFrame->lpWindow.get());
    SelectAdjFleet(0, (int16_t)idFleet);
    win32::UpdateWindow(hwndPlanet);
    while (win32::PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) { win32::TranslateMessage(&msg); win32::DispatchMessageA(&msg); }
    win32::GetClientRect(hwndPane, &rcPane);
    if (!win32::GetScrollInfo(hwndPane, SB_VERT, &si) || si.nPage != (UINT)rcPane.bottom ||
        si.nMax < rcPane.bottom || si.nPos != 0) { fprintf(stderr, "Pane initial: page %u height %d max %d pos %d\n", si.nPage, rcPane.bottom, si.nMax, si.nPos); return FALSE; }
    win32::SendMessageA(hwndPane, WM_VSCROLL, MAKEWPARAM(SB_THUMBTRACK, 60), 0);
    if (win32::GetScrollPos(hwndPane, SB_VERT) != 60) { fprintf(stderr, "Pane track: pos %d max %d page %u\n", win32::GetScrollPos(hwndPane, SB_VERT), si.nMax, si.nPage); return FALSE; }
    pt = (POINT){0, 0}; win32::ClientToScreen(hwndPlanet, &pt); win32::ScreenToClient(hwndPane, &pt);
    if (pt.y != -60) { fprintf(stderr, "Pane origin: y %d\n", pt.y); return FALSE; }
    win32::SendMessageA(hwndPane, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
    iPosition = win32::GetScrollPos(hwndPane, SB_VERT);
    if (iPosition != si.nMax - (int)si.nPage + 1) { fprintf(stderr, "Pane bottom: pos %d max %d page %u\n", iPosition, si.nMax, si.nPage); return FALSE; }
    win32::SendMessageA(hwndPane, WM_MOUSEWHEEL, MAKEWPARAM(0, 120), 0);
    if (win32::GetScrollPos(hwndPane, SB_VERT) >= iPosition) { fprintf(stderr, "Pane wheel: pos %d previous %d\n", win32::GetScrollPos(hwndPane, SB_VERT), iPosition); return FALSE; }
    // Address the actual lower fleet controls using their hierarchy rectangles.
    win32::GetWindowRect(hwndRepCB, &rcControl);
    pt = (POINT){rcControl.left, rcControl.bottom}; win32::ScreenToClient(hwndPane, &pt);
    win32::SendMessageA(hwndPane, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, (((0) > (win32::GetScrollPos(hwndPane, SB_VERT) + pt.y - rcPane.bottom)) ? (0) : (win32::GetScrollPos(hwndPane, SB_VERT) + pt.y - rcPane.bottom))), 0);
    win32::GetWindowRect(hwndRepCB, &rcControl);
    pt = (POINT){rcControl.left, rcControl.bottom}; win32::ScreenToClient(hwndPane, &pt);
    if (!win32::IsWindowVisible(hwndRepCB) || pt.y > rcPane.bottom || pt.y < 0) { fprintf(stderr, "Pane lower: visible %d bottom %d height %d scroll %d\n", win32::IsWindowVisible(hwndRepCB), pt.y, rcPane.bottom, win32::GetScrollPos(hwndPane, SB_VERT)); return FALSE; }
    win32::SetWindowPos(hwndFrame, NULL, 0, 0, rcFrame.right - rcFrame.left, rcFrame.bottom - rcFrame.top,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    puts("Planet/fleet viewport: proportional page, live thumb, wheel, clamp and lower-control reachability passed");
    return TRUE;
}
