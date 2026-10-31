/* Product navigation checks use a copied year-10 game with actual messages. */
static BOOL FCheckMessageWheel(void) {
    int iFirst, iNext;

    if (cMsg + vcmsgplrIn < 2 || gd.fSendMsgMode) return 0;
    win32::SendMessageA(hwndMessage, win32::WM_KEYDOWN, win32::VK_HOME, 1);
    iFirst = iMsgCur; iNext = IMsgNext(0);
    if (iFirst == imsgNone || iNext == imsgNone || iFirst == iNext) return 0;
    win32::SendMessageA(hwndMessage, win32::WM_MOUSEWHEEL, win32::MAKEWPARAM(0, (WORD)-60), 0);
    if (iMsgCur != iFirst) return 0;
    win32::SendMessageA(hwndMessage, win32::WM_MOUSEWHEEL, win32::MAKEWPARAM(0, (WORD)-60), 0);
    if (iMsgCur != iNext) return 0;
    win32::SendMessageA(hwndMessage, win32::WM_MOUSEWHEEL, win32::MAKEWPARAM(0, 120), 0);
    if (iMsgCur != iFirst) return 0;
    win32::SendMessageA(hwndMessage, win32::WM_MOUSEWHEEL, win32::MAKEWPARAM(win32::MK_CONTROL, (WORD)-120), 0);
    if (iMsgCur != iFirst) return 0;
    puts("Message wheel: partial notches accumulate, both directions navigate and Ctrl preserves the message");
    return 1;
}

static BOOL FCheckPlanetPane(void) {
    HWND hwndPane = win32::GetParent(hwndPlanet);
    SCROLLINFO si = {sizeof(si), win32::SIF_ALL, 0, 0, 0, 0, 0};
    RECT rcFrame, rcPane, rcControl;
    POINT pt;
    win32::MSG msg;
    int i, idFleet = idflNone, iPosition;
    char szClass[64];

    if (!win32::GetClassNameA(hwndPane, szClass, sizeof(szClass)) || strcmp(szClass, "StarsPlanetPane")) return 0;
    for (i = 0; i < cFleet; i++)
        if (rglpfl[i] && !rglpfl[i]->fDead && rglpfl[i]->iPlayer == idPlayer) { idFleet = rglpfl[i]->id; break; }
    if (idFleet == idflNone) return 0;
    win32::GetWindowRect(hwndFrame, &rcFrame);
    win32::ShowWindow(hwndFrame, win32::SW_RESTORE);
    if (hwndFrame->lpWindow.get()) SDL_SyncWindow(hwndFrame->lpWindow.get());
    win32::SetWindowPos(hwndFrame, NULL, 0, 0, 520, 380, win32::SWP_NOMOVE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
    if (hwndFrame->lpWindow.get()) SDL_SyncWindow(hwndFrame->lpWindow.get());
    SelectAdjFleet(0, (int16_t)idFleet);
    win32::UpdateWindow(hwndPlanet);
    while (win32::PeekMessageA(&msg, NULL, 0, 0, win32::PM_REMOVE)) { win32::TranslateMessage(&msg); win32::DispatchMessageA(&msg); }
    win32::GetClientRect(hwndPane, &rcPane);
    if (!win32::GetScrollInfo(hwndPane, win32::SB_VERT, &si) || si.nPage != (UINT)rcPane.bottom ||
        si.nMax < rcPane.bottom || si.nPos != 0) { fprintf(stderr, "Pane initial: page %u height %d max %d pos %d\n", si.nPage, rcPane.bottom, si.nMax, si.nPos); return 0; }
    win32::SendMessageA(hwndPane, win32::WM_VSCROLL, win32::MAKEWPARAM(win32::SB_THUMBTRACK, 60), 0);
    if (win32::GetScrollPos(hwndPane, win32::SB_VERT) != 60) { fprintf(stderr, "Pane track: pos %d max %d page %u\n", win32::GetScrollPos(hwndPane, win32::SB_VERT), si.nMax, si.nPage); return 0; }
    pt = (POINT){0, 0}; win32::ClientToScreen(hwndPlanet, &pt); win32::ScreenToClient(hwndPane, &pt);
    if (pt.y != -60) { fprintf(stderr, "Pane origin: y %d\n", pt.y); return 0; }
    win32::SendMessageA(hwndPane, win32::WM_VSCROLL, win32::MAKEWPARAM(win32::SB_BOTTOM, 0), 0);
    iPosition = win32::GetScrollPos(hwndPane, win32::SB_VERT);
    if (iPosition != si.nMax - (int)si.nPage + 1) { fprintf(stderr, "Pane bottom: pos %d max %d page %u\n", iPosition, si.nMax, si.nPage); return 0; }
    win32::SendMessageA(hwndPane, win32::WM_MOUSEWHEEL, win32::MAKEWPARAM(0, 120), 0);
    if (win32::GetScrollPos(hwndPane, win32::SB_VERT) >= iPosition) { fprintf(stderr, "Pane wheel: pos %d previous %d\n", win32::GetScrollPos(hwndPane, win32::SB_VERT), iPosition); return 0; }
    // Address the actual lower fleet controls using their hierarchy rectangles.
    win32::GetWindowRect(hwndRepCB, &rcControl);
    pt = (POINT){rcControl.left, rcControl.bottom}; win32::ScreenToClient(hwndPane, &pt);
    win32::SendMessageA(hwndPane, win32::WM_VSCROLL, win32::MAKEWPARAM(win32::SB_THUMBPOSITION, (((0) > (win32::GetScrollPos(hwndPane, win32::SB_VERT) + pt.y - rcPane.bottom)) ? (0) : (win32::GetScrollPos(hwndPane, win32::SB_VERT) + pt.y - rcPane.bottom))), 0);
    win32::GetWindowRect(hwndRepCB, &rcControl);
    pt = (POINT){rcControl.left, rcControl.bottom}; win32::ScreenToClient(hwndPane, &pt);
    if (!win32::IsWindowVisible(hwndRepCB) || pt.y > rcPane.bottom || pt.y < 0) { fprintf(stderr, "Pane lower: visible %d bottom %d height %d scroll %d\n", win32::IsWindowVisible(hwndRepCB), pt.y, rcPane.bottom, win32::GetScrollPos(hwndPane, win32::SB_VERT)); return 0; }
    win32::SetWindowPos(hwndFrame, NULL, 0, 0, rcFrame.right - rcFrame.left, rcFrame.bottom - rcFrame.top,
                 win32::SWP_NOMOVE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
    puts("Planet/fleet viewport: proportional page, live thumb, wheel, clamp and lower-control reachability passed");
    return 1;
}
