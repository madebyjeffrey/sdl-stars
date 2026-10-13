/* Exercise the shared WinMain, embedded resources and actual title painting. */
#include "win.h"
#include <stars_compat.h>
#include "internal.h"
#include <stdio.h>
#include <stdlib.h>

int16_t StarsTestReal_InitInstance(int16_t nCmdShow);
static BOOL fChecked, fPassed;
static int cAlerts;
static UINT_PTR idTimer;
static int iStage, cWait;
static HWND hwndWizard;
static HWND hwndRace;
static int iRacePage;
static SDL_WindowID idRaceWindow;
static HWND hwndRacePrevious;
static BOOL fRaceBack;
static BOOL fHabPainted;
static int cHabShow;
static const char *pszContinue;
static HWND hwndBrowserPrevious;
static BOOL fTitlePainted;
static int cTitleShow;
static BOOL fInitComplete;
static int cEarlyFrameShow;
static BOOL fToolbarMenu, fToolbarAnchor;
static int xToolbarMenu, yToolbarMenu, cToolbarMenu;
static unsigned cRenderPresent;

bool SDL_RenderPresent(SDL_Renderer *lprenderer);
bool SDLCALL StarsTest_SDL_RenderPresent(SDL_Renderer *lprenderer) {
    cRenderPresent++;
    return SDL_RenderPresent(lprenderer);
}

static BOOL FCheckScannerDrag(void) {
    HWND hwndBar = hwndScanner->rghwndScroll[SB_HORZ];
    SCROLLINFO si = {sizeof(si), SIF_ALL, 0, 0, 0, 0, 0};
    RECT rc;
    int i, xGrab, dxThumb, iPosSav;
    int16_t iZoomSav = iScanZoom, xSav = xScanTop, ySav = yScanTop;
    unsigned cPresentSav;
    Uint64 nsStart;
    BOOL fSinglePresent;

    /* Large test displays can fit the whole map at the saved zoom. */
    if (!IsWindow(hwndBar) || !IsWindowVisible(hwndBar)) {
        SendMessage(hwndFrame, WM_COMMAND, zoom400 + 3905, 0);
        hwndBar = hwndScanner->rghwndScroll[SB_HORZ];
    }
    if (!IsWindow(hwndBar) || !IsWindowVisible(hwndBar)) return FALSE;
    if (!GetScrollInfo(hwndScanner, SB_HORZ, &si) || !GetClientRect(hwndBar, &rc)) return FALSE;
    iPosSav = GetScrollPos(hwndScanner, SB_HORZ);
    SendMessage(hwndScanner, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, si.nMin), 0);
    UpdateWindow(hwndScanner); UpdateWindow(hwndBar);
    dxThumb = max(8, (int)((int64_t)(rc.right - 34) * si.nPage / (si.nMax - si.nMin + 1)));
    xGrab = 17 + dxThumb / 2;
    SendMessage(hwndBar, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(xGrab, 8));
    // Each simulated drag tick includes the completed window-tree paint pass.
    if (!FCompatPaintTree(hwndFrame)) return FALSE;
    cPresentSav = cRenderPresent; nsStart = SDL_GetPerformanceCounter();
    for (i = 1; i <= 10; i++) {
        SendMessage(hwndBar, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(xGrab + i * 4, 8));
        if (GetCapture() != hwndBar || !FCompatPaintTree(hwndFrame)) return FALSE;
    }
    printf("Scanner drag: %u presentations for 10 updates, %.2f ms\n", cRenderPresent - cPresentSav,
           (double)(SDL_GetPerformanceCounter() - nsStart) * 1000 / SDL_GetPerformanceFrequency());
    fSinglePresent = cRenderPresent - cPresentSav == 10;
    SendMessage(hwndBar, WM_LBUTTONUP, 0, MAKELPARAM(xGrab + 40, 8));
    SendMessage(hwndScanner, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, iPosSav), 0);
    if (iScanZoom != iZoomSav) {
        SendMessage(hwndFrame, WM_COMMAND, iZoomSav + 3905, 0);
        SendMessage(hwndScanner, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, xSav), 0);
        SendMessage(hwndScanner, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, ySav), 0);
    }
    return fSinglePresent;
}

static int iTransportPreset = -1, cTransportMenus;
BOOL StarsTestReal_TrackPopupMenu(HMENU hmenu, UINT flags, int x, int y, int reserved, HWND hwnd, const RECT *prc);
BOOL StarsTest_TrackPopupMenu(HMENU hmenu, UINT flags, int x, int y, int reserved, HWND hwnd, const RECT *prc) {
    if (iTransportPreset >= 0) {
        SDL_Event event = {0};
        HWND hwndRoot = HwndCompatRoot(hwnd);
        BOOL fResult;
        int i, iPreset = iTransportPreset;
        const SDL_Scancode rgsc[] = {SDL_SCANCODE_HOME, SDL_SCANCODE_DOWN, SDL_SCANCODE_RETURN};
        cTransportMenus++;
        for (i = 0; i < iPreset + 2; i++) {
            event.type = SDL_EVENT_KEY_DOWN; event.key.windowID = hwndRoot->id;
            event.key.scancode = rgsc[i == 0 ? 0 : i == iPreset + 1 ? 2 : 1];
            event.key.key = SDL_GetKeyFromScancode(event.key.scancode, SDL_KMOD_NONE, false);
            event.key.down = true;
            if (!SDL_PushEvent(&event)) return FALSE;
            event.type = SDL_EVENT_KEY_UP; event.key.down = false;
            if (!SDL_PushEvent(&event)) return FALSE;
        }
        fResult = StarsTestReal_TrackPopupMenu(hmenu, flags, x, y, reserved, hwnd, prc);
        iTransportPreset = -1;
        return fResult;
    }
    if (!fToolbarMenu) return StarsTestReal_TrackPopupMenu(hmenu, flags, x, y, reserved, hwnd, prc);
    cToolbarMenu++;
    fToolbarAnchor = x == xToolbarMenu && y == yToolbarMenu && !(flags & TPM_CENTERALIGN);
    return FALSE;
}

static BOOL FCheckMapInspection(void) {
    SCANSNAPSHOT snap = {0}, fresh = {0};
    size_t i, j, cPlanetFound = 0;
    POINT16 pt;
    POINT screen;
    SCAN scanSav = sel.scan;
    SEL selSav = sel;
    BOOL fPassed = FALSE;
    HWND hwndInspector, hwndMap, hwndChild, hwndGrandchild;
    char szNameSav[sizeof(fresh.rgobj[0].szName)];
    char szExport[32768];
    size_t cb;
    const char *pszViewerCapture = getenv("STARS_MAP_VIEWER_CAPTURE");
    FILE *lpfile;
    const char *pszCapture = getenv("STARS_MAP_CAPTURE");
    int16_t fDirty = game.fDirty;
    if (!FCaptureScanner(&snap) || snap.cObject < (size_t)game.cPlanMax ||
        snap.lidGame != game.lid || snap.idPlayer != idPlayer || snap.turn != game.turn) goto LDone;
    for (i = 0; i < snap.cObject; i++) {
        if (!snap.rgobj[i].szName[0]) goto LDone;
        pt = PointTo16(snap.rgobj[i].ptUniverse); LogicalToScan(&pt);
        screen = PointFrom16(pt); ClientToScreen(hwndScanner, &screen);
        if (pt.x != snap.rgobj[i].ptClient.x || pt.y != snap.rgobj[i].ptClient.y ||
            screen.x != snap.rgobj[i].ptScreen.x || screen.y != snap.rgobj[i].ptScreen.y) goto LDone;
        if (snap.rgobj[i].grType == grobjPlanet) cPlanetFound++;
        for (j = i + 1; j < snap.cObject; j++)
            if (snap.rgobj[i].grType == snap.rgobj[j].grType && snap.rgobj[i].id == snap.rgobj[j].id) goto LDone;
    }
    if (cPlanetFound != (size_t)game.cPlanMax || game.fDirty != fDirty || memcmp(&sel, &selSav, sizeof(sel))) goto LDone;
    snap.turn++;
    if (FSelectScannerObject(&snap, grobjPlanet, 0, TRUE)) goto LDone;
    snap.turn--;
    if (FSelectScannerObject(&snap, grobjPlanet, UINT16_MAX, TRUE) ||
        !FSelectScannerObject(&snap, grobjPlanet, 0, TRUE) || sel.scan.grobj != grobjPlanet || sel.scan.idpl != 0) goto LDone;
    if (!FCaptureScanner(&fresh) || !fresh.rgobj[0].fOnScreen || !fresh.rgobj[0].fScanSelected || game.fDirty != fDirty) goto LDone;
    memcpy(szNameSav, fresh.rgobj[0].szName, sizeof(szNameSav));
    strcpy(fresh.rgobj[0].szName, "Quoted \"\xe9\x80\"\r\n");
    lpfile = tmpfile();
    if (!lpfile) goto LDone;
    if (!FWriteScannerSnapshot(&fresh, lpfile)) { fclose(lpfile); goto LDone; }
    rewind(lpfile); cb = fread(szExport, 1, sizeof(szExport) - 1, lpfile); szExport[cb] = 0; fclose(lpfile);
    if (!strstr(szExport, "\xc3\xa9\xe2\x82\xac") || !strstr(szExport, "\\u000d\\u000a")) goto LDone;
    memcpy(fresh.rgobj[0].szName, szNameSav, sizeof(szNameSav));
    if (pszCapture) {
        lpfile = fopen(pszCapture, "wb");
        if (!lpfile) goto LDone;
        if (!FWriteScannerSnapshot(&fresh, lpfile)) { fclose(lpfile); goto LDone; }
        if (fclose(lpfile) != 0) goto LDone;
    }
    hwndInspector = StarsCompatShowInspector();
    if (!hwndInspector || !GetDlgItem(hwndInspector,4)) goto LDone;
    SendMessage(GetDlgItem(hwndInspector,4), BM_CLICK,0,0);
    hwndMap = ShowScannerInspector();
    if (!hwndMap || SendMessage(GetDlgItem(hwndMap,1),LB_GETCOUNT,0,0) != (LRESULT)fresh.cObject ||
        !UpdateWindow(hwndMap) || !FCompatPresent(hwndMap)) goto LDone;
    for (hwndChild = GetWindow(hwndMap, GW_CHILD); hwndChild; hwndChild = GetWindow(hwndChild, GW_HWNDNEXT)) {
        UpdateWindow(hwndChild);
        for (hwndGrandchild = GetWindow(hwndChild, GW_CHILD); hwndGrandchild; hwndGrandchild = GetWindow(hwndGrandchild, GW_HWNDNEXT))
            UpdateWindow(hwndGrandchild);
    }
    if (!FCompatPresent(hwndMap) || (pszViewerCapture && !SDL_SaveBMP(hwndMap->lpComposite, pszViewerCapture))) goto LDone;
    SendMessage(GetDlgItem(hwndMap, 1), LB_SETCURSEL, 1, 0);
    SendMessage(GetDlgItem(hwndMap, 4), BM_CLICK, 0, 0);
    if (sel.scan.grobj != grobjPlanet || sel.scan.idpl != 1 || game.fDirty != fDirty) goto LDone;
    SendMessage(hwndMap, WM_CLOSE,0,0);
    if (IsWindow(hwndMap)) goto LDone;
    hwndMap = ShowScannerInspector();
    if (!hwndMap) goto LDone;
    DestroyWindow(hwndMap); DestroyWindow(hwndInspector);
    ChangeScanSel(&scanSav,2);
    fPassed = TRUE;
LDone:
    FreeScannerSnapshot(&snap); FreeScannerSnapshot(&fresh);
    return fPassed;
}

static BOOL FCheckReportScrollbars(void) {
    SCROLLINFO si = {sizeof(si), SIF_ALL, 0, 0, 0, 0, 0};
    BOOL fPassed = TRUE;

    SendMessage(hwndFrame, WM_COMMAND, IDM_REPORT_PLANET, 0);
    if (!IsWindow(hwndReportDlg) || !vprptCur) return FALSE;
    SetWindowPos(hwndReportDlg, NULL, 0, 0, 300, 220, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    if (IsWindowVisible(vprptCur->hwndVScroll)) {
        fPassed = GetScrollInfo(vprptCur->hwndVScroll, SB_CTL, &si) &&
            si.nPage == (UINT)max(1, vprptCur->cRowsVis) &&
            si.nMax - (int)si.nPage + 1 == vprptCur->cRows - vprptCur->cRowsVis;
    }
    if (IsWindowVisible(vprptCur->hwndHScroll)) {
        fPassed = fPassed && GetScrollInfo(vprptCur->hwndHScroll, SB_CTL, &si) &&
            si.nPage != 0 && si.nMax - (int)si.nPage + 1 == vprptCur->cColScroll;
    }
    SendMessage(hwndReportDlg, WM_CLOSE, 0, 0);
    return fPassed && !IsWindow(hwndReportDlg);
}

static BOOL FCheckToolbarPlacement(void) {
    static const ToolbarButton rgitb[] = {tbMineFields, tbShipDesignFilterMenu, tbEnemyClassFilterMenu, tbZoomMenu};
    POINT pt;
    RECT rc, rcTip, rcDrop, rcClient;
    SCROLLINFO si = {sizeof(si), SIF_ALL, 0, 0, 0, 0, 0};
    int cPage, iMax;
    HWND hwndFocus = GetFocus();
    HWND hwndRoot = HwndCompatRoot(hwndTb);
    HWND hwndButtons;
    char szClass[64];
    SDL_Event event = {0};
    int i, x, iRange;

    if (!GetClientRect(hwndScanner, &rcClient) || !GetScrollInfo(hwndScanner, SB_HORZ, &si)) return FALSE;
    cPage = max(1, ScanToPt(rcClient.right));
    iMax = (max(1000, dGalInv - 1000 - cPage) + 3) & ~3;
    if (si.nPage != (UINT)cPage || si.nMax - (int)si.nPage + 1 != iMax) return FALSE;
    if (!GetScrollInfo(hwndScanner, SB_VERT, &si)) return FALSE;
    cPage = max(1, ScanToPt(rcClient.bottom - dySBar));
    iMax = (max(1000, dGalInv - 1000 - cPage) + 3) & ~3;
    if (si.nPage != (UINT)cPage || si.nMax - (int)si.nPage + 1 != iMax) return FALSE;
    if (!GetWindowRect(hwndTBRadar, &rc)) return FALSE;
    if (!GetClientRect(hwndTBRadar, &rcClient) || rcClient.right != rc.right - rc.left ||
        hwndTBRadar->rghwndScroll[SB_VERT] || hwndTBRadar->rghwndScroll[SB_HORZ]) return FALSE;
    pt = (POINT){(rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2};
    hwndButtons = WindowFromPoint(pt);
    if (hwndButtons != hwndTBRadar && GetParent(hwndButtons) != hwndTBRadar) return FALSE;
    iRange = (int)SendMessage(hwndTBRadar, CB_GETCURSEL, 0, 0);
    SetFocus(hwndTBRadar);
    SendMessage(hwndTBRadar, CB_SHOWDROPDOWN, TRUE, 0);
    if (!hwndTBRadar->lpcombo || !hwndTBRadar->lpcombo->hwndPopup ||
        !GetWindowRect(hwndTBRadar->lpcombo->hwndPopup, &rcDrop) ||
        rcDrop.left != rc.left || rcDrop.top != rc.bottom ||
        !hwndTBRadar->lpcombo->hwndPopup->lpCanvas ||
        SendMessage(hwndTBRadar, CB_GETCOUNT, 0, 0) < 2) return FALSE;
    if (strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0 &&
        (!(SDL_GetWindowFlags(hwndTBRadar->lpcombo->hwndPopup->lpWindow) & SDL_WINDOW_POPUP_MENU) ||
         SDL_GetWindowParent(hwndTBRadar->lpcombo->hwndPopup->lpWindow) != hwndRoot->lpWindow)) return FALSE;
    SendMessage(hwndTBRadar, WM_KEYDOWN, VK_DOWN, 0);
    SendMessage(hwndTBRadar, WM_KEYDOWN, VK_RETURN, 0);
    if (SendMessage(hwndTBRadar, CB_GETDROPPEDSTATE, 0, 0) || GetCapture() == hwndTBRadar) return FALSE;
    SendMessage(hwndTBRadar, CB_SETCURSEL, iRange, 0);
    SetFocus(hwndFocus);
    for (i = 0; i < 4; i++) {
        x = rgitb[i];
        if (x == tbShipDesignFilterMenu) x = tbShipDesignFilter;
        else if (x == tbEnemyClassFilterMenu) x = tbEnemyClassFilter;
        hwndButtons = GetDlgItem(hwndTb, IDC_STARS_TOOLBAR);
        if (!SendMessage(hwndButtons, TB_GETRECT, IDC_STARS_TOOLBUTTON_FIRST + x, (LPARAM)&rc)) return FALSE;
        pt = (POINT){rc.left, rc.bottom};
        if (!ClientToScreen(hwndButtons, &pt)) return FALSE;
        xToolbarMenu = pt.x; yToolbarMenu = pt.y;
        cToolbarMenu = 0; fToolbarAnchor = FALSE; fToolbarMenu = TRUE;
        if (!GetClassName(hwndButtons, szClass, sizeof(szClass)) || strcmp(szClass, TOOLBARCLASSNAME)) return FALSE;
        SendMessage(hwndButtons, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(rc.right - 3, (rc.top + rc.bottom) / 2));
        SendMessage(hwndButtons, WM_LBUTTONUP, 0, MAKELPARAM(rc.right - 3, (rc.top + rc.bottom) / 2));
        fToolbarMenu = FALSE;
        if (cToolbarMenu != 1 || !fToolbarAnchor) return FALSE;
    }
    SetRect(&rc, 4, 4, 33, 32);
    MapWindowPoints(hwndTb, NULL, (POINT *)&rc, 2);
    for (i = 0; i < 2; i++) {
        pt = (POINT){rc.left + 2 + i * 10, rc.top + 2 + i * 10};
        if (!strcmp(SDL_GetCurrentVideoDriver(), "dummy") || !strcmp(SDL_GetCurrentVideoDriver(), "wayland")) {
            if (!ScreenToClient(hwndRoot, &pt)) return FALSE;
            event.type = SDL_EVENT_MOUSE_MOTION;
            event.motion.windowID = hwndRoot->id;
            event.motion.x = (float)pt.x;
            event.motion.y = (float)(pt.y + CyCompatMenuBar(hwndRoot));
            CompatRouteEvent(&event);
        } else if (!SDL_WarpMouseGlobal((float)pt.x, (float)pt.y)) return FALSE;
        ShowTooltip(idsNormalView, &rc);
        if (!hwndTooltip) return FALSE;
        TooltipWndProc(hwndTooltip, 1523, TRUE, 0);
        if (!IsWindowVisible(hwndTooltip) || !GetWindowRect(hwndTooltip, &rcTip) ||
            rcTip.left != rc.left || rcTip.top != rc.bottom + 2 || GetFocus() != hwndFocus) return FALSE;
        if (strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0 &&
            (!(SDL_GetWindowFlags(hwndTooltip->lpWindow) & SDL_WINDOW_TOOLTIP) ||
             SDL_GetWindowParent(hwndTooltip->lpWindow) != hwndRoot->lpWindow)) return FALSE;
        ShowTooltip(idsNoString, NULL);
    }
    return TRUE;
}

int16_t StarsTestReal_PromptPassword(void);
int16_t StarsTest_PromptPassword(void) {
    // The frozen regression race is password protected. This test qualifies
    // file loading and client creation; password validation is outside its scope.
    return pszContinue ? TRUE : StarsTestReal_PromptPassword();
}

bool SDL_ShowWindow(SDL_Window *lpWindow);
bool SDLCALL StarsTest_SDL_ShowWindow(SDL_Window *lpWindow) {
    HWND hwnd;
    BYTE r, g, b, a;
    COLORREF cr;
    BYTE *rgb;
    size_t cb, i;

    hwnd = LpCompatWindowFromId(SDL_GetWindowID(lpWindow));
    if (hwnd == hwndFrame && !fInitComplete) cEarlyFrameShow++;
    if (hwnd && hwnd->lpfnWndProc == TitleWndProc && cTitleShow++ == 0) {
        if (hwnd->lpCanvas && hwnd->lpComposite) {
            rgb = hwnd->lpCanvas->pixels;
            cb = (size_t)hwnd->lpCanvas->pitch * hwnd->lpCanvas->h;
            for (i = 4; i + 4 <= cb; i += 4) {
                if (memcmp(rgb, rgb + i, 4)) { fTitlePainted = TRUE; break; }
            }
        }
    }

    for (hwnd = GetWindow(hwndTitle, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
        if (hwnd->lpWindow != lpWindow || hwnd->lpfnDlg != RaceWizardDlg2) continue;
        if (cHabShow++ == 0) {
            cr = GetSysColor(COLOR_BTNFACE);
            fHabPainted = hwnd->lpCanvas && hwnd->lpComposite &&
                SDL_ReadSurfacePixel(hwnd->lpCanvas, 0, 0, &r, &g, &b, &a) &&
                RGB(r, g, b) == cr;
        }
    }
    return SDL_ShowWindow(lpWindow);
}

static BOOL FPushClick(HWND hwnd) {
    RECT rc, rcPane;
    POINT pt, ptEnd;
    HWND hwndAncestor, hwndPane;
    int i, iPosition;
    HWND lpTitle = HwndCompatRoot(hwnd);
    SDL_Event event = {0};
    for (hwndAncestor = hwnd; hwndAncestor; hwndAncestor = GetParent(hwndAncestor)) {
        if (hwndAncestor != hwndPlanet) continue;
        hwndPane = GetParent(hwndPlanet);
        for (i = 0; i < 2; i++) {
            GetWindowRect(hwnd, &rc); GetClientRect(hwndPane, &rcPane);
            pt = (POINT){rc.left, rc.top}; ptEnd = (POINT){rc.right, rc.bottom};
            ScreenToClient(hwndPane, &pt); ScreenToClient(hwndPane, &ptEnd);
            iPosition = GetScrollPos(hwndPane, i == 0 ? SB_VERT : SB_HORZ);
            if ((i == 0 ? pt.y : pt.x) < 0) iPosition += i == 0 ? pt.y : pt.x;
            else if ((i == 0 ? ptEnd.y : ptEnd.x) > (i == 0 ? rcPane.bottom : rcPane.right))
                iPosition += i == 0 ? ptEnd.y - rcPane.bottom : ptEnd.x - rcPane.right;
            SendMessage(hwndPane, i == 0 ? WM_VSCROLL : WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, max(0, iPosition)), 0);
        }
        break;
    }
    if (!lpTitle || !GetWindowRect(hwnd, &rc)) return FALSE;
    pt = (POINT){(rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2};
    if (!ScreenToClient(lpTitle, &pt)) return FALSE;
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.windowID = lpTitle->id;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.down = true;
    event.button.x = (float)pt.x;
    event.button.y = (float)(pt.y + CyCompatMenuBar(lpTitle));
    if (!SDL_PushEvent(&event)) return FALSE;
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.down = false;
    return SDL_PushEvent(&event);
}

static BOOL FPushComboRow(HWND hwnd, int iRow) {
    HWND hwndPopup = hwnd->lpcombo ? hwnd->lpcombo->hwndPopup : NULL;
    HWND hwndRoot = HwndCompatRoot(hwndPopup);
    POINT pt;
    RECT rc;
    SDL_Event event = {0};
    int cy, iTop;
    if (!hwndPopup || !hwndRoot || !GetClientRect(hwndPopup, &rc)) return FALSE;
    cy = (int)CompatListProc(hwnd, LB_GETITEMHEIGHT, 0, 0);
    iTop = (int)CompatListProc(hwnd, LB_GETTOPINDEX, 0, 0);
    pt = (POINT){rc.right / 2, 1 + (iRow - iTop) * cy + cy / 2};
    if (cy <= 0 || pt.y < 1 || pt.y >= rc.bottom - 1 || !ClientToScreen(hwndPopup, &pt) ||
        !ScreenToClient(hwndRoot, &pt)) return FALSE;
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.windowID = hwndRoot->id;
    event.button.button = SDL_BUTTON_LEFT; event.button.down = true;
    event.button.x = (float)pt.x; event.button.y = (float)(pt.y + CyCompatMenuBar(hwndRoot));
    if (!SDL_PushEvent(&event)) return FALSE;
    event.type = SDL_EVENT_MOUSE_BUTTON_UP; event.button.down = false;
    return SDL_PushEvent(&event);
}

static BOOL FPushListRow(HWND hwnd, int iRow) {
    HWND hwndRoot = HwndCompatRoot(hwnd);
    RECT rc;
    POINT pt;
    SDL_Event event = {0};
    if (!hwndRoot || SendMessage(hwnd, LB_GETITEMRECT, iRow, (LPARAM)&rc) == LB_ERR) return FALSE;
    pt = (POINT){(rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2};
    if (!ClientToScreen(hwnd, &pt) || !ScreenToClient(hwndRoot, &pt)) return FALSE;
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.windowID = hwndRoot->id;
    event.button.button = SDL_BUTTON_LEFT; event.button.down = true;
    event.button.x = (float)pt.x; event.button.y = (float)(pt.y + CyCompatMenuBar(hwndRoot));
    if (!SDL_PushEvent(&event)) return FALSE;
    event.type = SDL_EVENT_MOUSE_BUTTON_UP; event.button.down = false;
    return SDL_PushEvent(&event);
}

static BOOL FPushClientRect(HWND hwnd, const RECT *lprc, Uint8 button) {
    HWND hwndRoot = HwndCompatRoot(hwnd);
    RECT rc;
    POINT pt;
    SDL_Event event = {0};
    memcpy(&rc, lprc, sizeof(rc));
    pt = (POINT){(rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2};
    if (!hwndRoot || !ClientToScreen(hwnd, &pt) || !ScreenToClient(hwndRoot, &pt)) return FALSE;
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.windowID = hwndRoot->id;
    event.button.button = button; event.button.down = true;
    event.button.x = (float)pt.x; event.button.y = (float)(pt.y + CyCompatMenuBar(hwndRoot));
    if (!SDL_PushEvent(&event)) return FALSE;
    event.type = SDL_EVENT_MOUSE_BUTTON_UP; event.button.down = false;
    return SDL_PushEvent(&event);
}

static int iWorkflow, iWorkflowStep, cWorkflowWait, iResearchTarget, cProductionBefore;
static BOOL fWorkflowPassed, fWorkflowBusy;
static uint64_t idWorkflowDialog;

static HWND HwndWorkflowDialog(void) {
    STARSCOMPATSNAPSHOT snap = {0};
    uint64_t idOwner = StarsCompatWindowId(hwndFrame), idDialog = 0;
    size_t i;
    if (!StarsCompatCaptureHierarchy(&snap)) return NULL;
    for (i = 0; i < snap.cWindow; i++) {
        if (!snap.rgWindow[i].fDialog || !snap.rgWindow[i].fVisible || snap.rgWindow[i].idOwner != idOwner) continue;
        if (idDialog) { idDialog = 0; break; }
        idDialog = snap.rgWindow[i].id;
    }
    StarsCompatFreeHierarchy(&snap);
    return StarsCompatResolveWindow(idDialog);
}

static HWND HwndWorkflowControl(HWND hwnd, int id, LPCSTR pszClass) {
    return StarsCompatResolveWindow(StarsCompatFindControl(StarsCompatWindowId(hwnd), id, pszClass));
}

static int CProductionItems(void) {
    int i, c = 0;
    if (!lpplProdGlob) return -1;
    for (i = 0; i < lpplProdGlob->iprodMac; i++) c += lpplProdGlob->rgprod[i].cItem;
    return c;
}

static int ishdefWorkflow, cShdefWorkflow;
static const char szCopyName[] = "Compat Copy";
static const char szRenameName[] = "Compat Renamed";

static BOOL FCheckShipWorkflow(HWND hwndDialog) {
    HWND hwndControl;
    SDL_Event event = {0};
    LPCSTR pszName = iWorkflow == 10 ? "Discarded edit" : iWorkflow == 11 ? szRenameName : szCopyName;
    int iStep = iWorkflowStep, iRow;
    char sz[32];
    BOOL fCopy = iWorkflow == 8 || iWorkflow == 9;
    if (hwndDialog->lpfnDlg != SlotDlg || fStarbaseMode) return FALSE;
    if (!fCopy && iStep < 3) {
        hwndControl = HwndWorkflowControl(hwndDialog, IDC_COMBOBOX, "ComboBox");
        if (!hwndControl || mdBuild != mdBuildShdef) return FALSE;
        if (iStep == 0) return FPushClick(hwndControl);
        if (iStep == 1) {
            iRow = (int)SendMessage(hwndControl, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)rgshdef[ishdefWorkflow].hul.szClass);
            if (iRow < 0 || !SendMessage(hwndControl, CB_GETDROPPEDSTATE, 0, 0)) return FALSE;
            CompatListProc(hwndControl, LB_SETTOPINDEX, iRow, 0);
            return FPushComboRow(hwndControl, iRow);
        }
        if (!lpshdefBuild || lpshdefBuild != &rgshdef[ishdefWorkflow]) return FALSE;
        hwndControl = HwndWorkflowControl(hwndDialog, iWorkflow == 12 ? IDC_DELETE : IDC_EDIT, "Button");
        return hwndControl && IsWindowEnabled(hwndControl) && FPushClick(hwndControl);
    }
    if (!fCopy) iStep -= 2;
    if (iWorkflow == 12) {
        if (!rgshdef[ishdefWorkflow].fFree || rgplr[idPlayer].cShDef != cShdefWorkflow) return FALSE;
        hwndControl = HwndWorkflowControl(hwndDialog, IDCANCEL, "Button");
        fWorkflowPassed = hwndControl && FPushClick(hwndControl);
        return fWorkflowPassed;
    }
    if (iStep == 0) {
        if (mdBuild != mdBuildShdef || rgplr[idPlayer].cShDef != cShdefWorkflow) return FALSE;
        hwndControl = HwndWorkflowControl(hwndDialog, IDC_IMPORT, "Button");
        return hwndControl && IsWindowEnabled(hwndControl) && FPushClick(hwndControl);
    }
    if (iStep <= 3) {
        if (mdBuild != mdBuildEdit || ishdefBuild != ishdefWorkflow || lpshdefBuild != &shdefBuild) return FALSE;
        hwndControl = HwndWorkflowControl(hwndDialog, IDC_EDITNAME, "Edit");
        if (!hwndControl || !IsWindowVisible(hwndControl)) return FALSE;
        if (iStep == 1) return FPushClick(hwndControl);
        if (iStep == 2) {
            if (GetFocus() != hwndControl) return FALSE;
            SendMessage(hwndControl, EM_SETSEL, 0, -1);
            event.type = SDL_EVENT_TEXT_INPUT;
            event.text.windowID = HwndCompatRoot(hwndControl)->id;
            event.text.text = pszName;
            return SDL_PushEvent(&event);
        }
        GetWindowText(hwndControl, sz, sizeof(sz));
        if (strcmp(sz, pszName) || strcmp(shdefBuild.hul.szClass, pszName)) return FALSE;
        hwndControl = HwndWorkflowControl(hwndDialog, iWorkflow == 8 || iWorkflow == 10 ? IDCANCEL : IDOK, "Button");
        return hwndControl && IsWindowVisible(hwndControl) && FPushClick(hwndControl);
    }
    if (mdBuild != mdBuildShdef) return FALSE;
    if (iWorkflow == 8) {
        if (!rgshdef[ishdefWorkflow].fFree || rgplr[idPlayer].cShDef != cShdefWorkflow) return FALSE;
    } else if (rgshdef[ishdefWorkflow].fFree || rgplr[idPlayer].cShDef != cShdefWorkflow + 1 ||
               strcmp(rgshdef[ishdefWorkflow].hul.szClass, iWorkflow == 11 ? szRenameName : szCopyName)) return FALSE;
    hwndControl = HwndWorkflowControl(hwndDialog, IDCANCEL, "Button");
    fWorkflowPassed = hwndControl && FPushClick(hwndControl);
    return fWorkflowPassed;
}

static BOOL FCheckHullWorkflow(HWND hwndDialog);

static BOOL FCheckMergeWorkflow(HWND hwndDialog) {
    HWND hwndList = HwndWorkflowControl(hwndDialog, IDC_MERGE_FLEETS_LIST, "ListBox"), hwndControl;
    int cSel;
    if (hwndDialog->lpfnDlg != MergeFleetsDlg || !hwndList || vcflMerge < 2 ||
        SendMessage(hwndList, LB_GETCOUNT, 0, 0) != vcflMerge) return FALSE;
    cSel = (int)SendMessage(hwndList, LB_GETSELCOUNT, 0, 0);
    if (iWorkflowStep == 0 || iWorkflowStep == 1) {
        if (iWorkflowStep == 1 && cSel != vcflMerge) return FALSE;
        hwndControl = HwndWorkflowControl(hwndDialog, iWorkflowStep == 0 ? IDC_MERGE_FLEETS_SELECT_ALL : IDC_MERGE_FLEETS_UNSELECT_ALL, "Button");
        return hwndControl && FPushClick(hwndControl);
    }
    if (iWorkflowStep == 2) return cSel == 0 && FPushListRow(hwndList, 0);
    if (iWorkflowStep == 3) {
        if (cSel != 1 || !SendMessage(hwndList, LB_GETSEL, 0, 0) || SendMessage(hwndList, LB_GETSEL, 1, 0)) return FALSE;
        if (iWorkflow == 14) return FPushListRow(hwndList, 1);
    } else if (cSel != 2 || !SendMessage(hwndList, LB_GETSEL, 0, 0) || !SendMessage(hwndList, LB_GETSEL, 1, 0)) return FALSE;
    hwndControl = HwndWorkflowControl(hwndDialog, iWorkflow == 13 ? IDCANCEL : IDOK, "Button");
    fWorkflowPassed = hwndControl && FPushClick(hwndControl);
    return fWorkflowPassed;
}

static int ishXferWorkflow, cXferWorkflow, idXferWorkflow;

static BOOL FCheckSplitWorkflow(HWND hwndDialog) {
    HWND hwndControl;
    int i;
    if (hwndDialog->lpfnDlg != TransferDlg || mdXferDlg != mdXferShips || !pxfer || !rgbtnXfer ||
        pxfer[0].id != sel.fl.id || pxfer[1].grobj != grobjFleet) return FALSE;
    if (iWorkflowStep == 0) {
        for (i = 0; i < crgbtnXfer; i++) {
            if (rgbtnXfer[i].fVisible && !(rgbtnXfer[i].bt & 4) && rgbtnXfer[i].iSide == 1 &&
                rgbtnXfer[i].iVal >= 0 && rgbtnXfer[i].iVal < 16 && pxfer[0].fl.rgcsh[rgbtnXfer[i].iVal] > 0) break;
        }
        if (i == crgbtnXfer) return FALSE;
        ishXferWorkflow = rgbtnXfer[i].iVal;
        cXferWorkflow = pxfer[0].fl.rgcsh[ishXferWorkflow];
        idXferWorkflow = pxfer[1].id;
        return pxfer[1].fl.rgcsh[ishXferWorkflow] == 0 && FPushClientRect(hwndDialog, &rgbtnXfer[i].rc, SDL_BUTTON_LEFT);
    }
    if (pxfer[1].fl.rgcsh[ishXferWorkflow] != 1 || pxfer[0].fl.rgcsh[ishXferWorkflow] != cXferWorkflow - 1) return FALSE;
    hwndControl = HwndWorkflowControl(hwndDialog, iWorkflow == 15 ? IDCANCEL : IDOK, "Button");
    fWorkflowPassed = hwndControl && FPushClick(hwndControl);
    return fWorkflowPassed;
}

static int iSupplyWorkflow, cCargoFleetWorkflow, cCargoPlanetWorkflow;

static BOOL FCheckCargoWorkflow(HWND hwndDialog) {
    HWND hwndControl;
    int i, iSide = iWorkflow == 19 ? 1 : 0;
    int dCargo = iSide == 0 ? 1 : -1;
    if (hwndDialog->lpfnDlg != TransferDlg || mdXferDlg != mdXferCargo || !pxfer || !rgbtnXfer ||
        pxfer[0].id != sel.fl.id || pxfer[0].grobj != grobjFleet || pxfer[1].grobj != grobjPlanet ||
        pxfer[1].id != sel.fl.idPlanet) return FALSE;
    if (iWorkflowStep == 0) {
        for (i = 0; i < crgbtnXfer; i++) {
            if (rgbtnXfer[i].fVisible && !(rgbtnXfer[i].bt & 4) && rgbtnXfer[i].iSide == iSide &&
                rgbtnXfer[i].iVal >= 0 && rgbtnXfer[i].iVal < 3 &&
                (iWorkflow != 19 || rgbtnXfer[i].iVal == iSupplyWorkflow)) break;
        }
        if (i == crgbtnXfer) return FALSE;
        iSupplyWorkflow = rgbtnXfer[i].iVal;
        cCargoFleetWorkflow = pxfer[0].fl.rgwtMin[iSupplyWorkflow];
        cCargoPlanetWorkflow = pxfer[1].pl.rgwtMin[iSupplyWorkflow];
        return FPushClientRect(hwndDialog, &rgbtnXfer[i].rc, SDL_BUTTON_LEFT);
    }
    if (pxfer[0].fl.rgwtMin[iSupplyWorkflow] != cCargoFleetWorkflow + dCargo ||
        pxfer[1].pl.rgwtMin[iSupplyWorkflow] != cCargoPlanetWorkflow - dCargo) return FALSE;
    hwndControl = HwndWorkflowControl(hwndDialog, iWorkflow == 17 ? IDCANCEL : IDOK, "Button");
    fWorkflowPassed = hwndControl && FPushClick(hwndControl);
    return fWorkflowPassed;
}

static void CALLBACK CheckWorkflow(HWND hwnd, UINT msg, UINT_PTR id, DWORD dwTime) {
    HWND hwndDialog, hwndControl;
    (void)hwnd; (void)msg; (void)dwTime;
    if (fWorkflowBusy) return;
    fWorkflowBusy = TRUE;
    hwndDialog = HwndWorkflowDialog();
    if (++cWorkflowWait > 120) goto LFail;
    if (!hwndDialog && !idWorkflowDialog && !fWorkflowPassed) { fWorkflowBusy = FALSE; return; }
    if (!hwndDialog || IsWindowEnabled(hwndFrame)) goto LFail;
    idWorkflowDialog = StarsCompatWindowId(hwndDialog);
    if (fWorkflowPassed) { fWorkflowBusy = FALSE; return; }
    fprintf(stderr, "Gameplay workflow %d step %d\n", iWorkflow, iWorkflowStep);
    if (iWorkflow == 1) {
        if (hwndDialog->lpfnDlg != ResearchDlg) goto LFail;
        if (iWorkflowStep == 0) {
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_RESEARCH_ENERGY + rgplr[idPlayer].iTechNow, "Button");
            if (!hwndControl || SendMessage(hwndControl, BM_GETCHECK, 0, 0) != BST_CHECKED) goto LFail;
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_RESEARCH_NEXT_FIELD, "ComboBox");
            if (!hwndControl || SendMessage(hwndControl, CB_GETCOUNT, 0, 0) != 8) goto LFail;
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_RESEARCH_ENERGY + iResearchTarget, "Button");
            if (!hwndControl || !IsWindowEnabled(hwndControl) || !FPushClick(hwndControl)) goto LFail;
        } else {
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_RESEARCH_ENERGY + iResearchTarget, "Button");
            if (iResTechNow != iResearchTarget || SendMessage(hwndControl, BM_GETCHECK, 0, 0) != BST_CHECKED) goto LFail;
            hwndControl = HwndWorkflowControl(hwndDialog, IDCANCEL, "Button");
            if (!hwndControl || !FPushClick(hwndControl)) goto LFail;
            fWorkflowPassed = TRUE;
        }
    } else if (iWorkflow == 5) {
        if (hwndDialog->lpfnDlg != BattlePlansDlg || iPlanSelDlg != 0) { fprintf(stderr, "Battle dialog: plan %d handler %d\n", iPlanSelDlg, hwndDialog->lpfnDlg == BattlePlansDlg); goto LFail; }
        hwndControl = HwndWorkflowControl(hwndDialog, IDC_BATTLE_PLAN_DUMP_CARGO, "Button");
        if (!hwndControl) goto LFail;
        if (iWorkflowStep == 0) {
            if (btlplan.fDumpCargo != rglpbtlplan[idPlayer][0].fDumpCargo ||
                SendMessage(hwndControl, BM_GETCHECK, 0, 0) != btlplan.fDumpCargo || !FPushClick(hwndControl)) goto LFail;
        } else {
            if (btlplan.fDumpCargo != iResearchTarget || !fDirtyPlan) { fprintf(stderr, "Dump cargo: value %d target %d check %ld dirty %d style %lx\n", btlplan.fDumpCargo, iResearchTarget, (long)SendMessage(hwndControl, BM_GETCHECK, 0, 0), fDirtyPlan, (unsigned long)GetWindowLongPtr(hwndControl, GWL_STYLE)); goto LFail; }
            hwndControl = HwndWorkflowControl(hwndDialog, IDOK, "Button");
            if (!hwndControl || !FPushClick(hwndControl)) goto LFail;
            fWorkflowPassed = TRUE;
        }
    } else if (iWorkflow == 6) {
        if (hwndDialog->lpfnDlg != RelationsDlg) goto LFail;
        if (iWorkflowStep == 0) {
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_RELATIONS_PLAYER_LIST, "ListBox");
            if (!hwndControl || SendMessage(hwndControl, LB_GETCOUNT, 0, 0) != game.cPlayer - 1 ||
                SendMessage(hwndControl, LB_GETCURSEL, 0, 0) != 0) goto LFail;
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_RELATIONS_NEUTRAL + rgplr[idPlayer].rgmdRelation[idPlayer == 0], "Button");
            if (!hwndControl || SendMessage(hwndControl, BM_GETCHECK, 0, 0) != BST_CHECKED) goto LFail;
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_RELATIONS_NEUTRAL + iResearchTarget, "Button");
            if (!hwndControl || !FPushClick(hwndControl)) goto LFail;
        } else {
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_RELATIONS_NEUTRAL + iResearchTarget, "Button");
            if (!hwndControl || SendMessage(hwndControl, BM_GETCHECK, 0, 0) != BST_CHECKED ||
                rgplr[idPlayer].rgmdRelation[idPlayer == 0] != iResearchTarget || !fDirtyPlan) goto LFail;
            hwndControl = HwndWorkflowControl(hwndDialog, IDCANCEL, "Button");
            if (!hwndControl || !FPushClick(hwndControl)) goto LFail;
            fWorkflowPassed = TRUE;
        }
    } else if (iWorkflow == 7) {
        if (hwndDialog->lpfnDlg != BattlePlansDlg || iPlanSelDlg != 0) { fprintf(stderr, "Battle dialog: plan %d handler %d\n", iPlanSelDlg, hwndDialog->lpfnDlg == BattlePlansDlg); goto LFail; }
        hwndControl = HwndWorkflowControl(hwndDialog, IDC_BATTLE_PLAN_TACTIC, "ComboBox");
        if (!hwndControl) goto LFail;
        if (iWorkflowStep == 0) {
            if (SendMessage(hwndControl, CB_GETCURSEL, 0, 0) != rglpbtlplan[idPlayer][0].mdTactic ||
                SendMessage(hwndControl, CB_GETCOUNT, 0, 0) <= iResearchTarget || !FPushClick(hwndControl)) goto LFail;
        } else if (iWorkflowStep == 1) {
            if (!SendMessage(hwndControl, CB_GETDROPPEDSTATE, 0, 0) ||
                btlplan.mdTactic != rglpbtlplan[idPlayer][0].mdTactic || !FPushComboRow(hwndControl, iResearchTarget)) goto LFail;
        } else {
            if (SendMessage(hwndControl, CB_GETDROPPEDSTATE, 0, 0) || btlplan.mdTactic != iResearchTarget || !fDirtyPlan) goto LFail;
            hwndControl = HwndWorkflowControl(hwndDialog, IDOK, "Button");
            if (!hwndControl || !FPushClick(hwndControl)) goto LFail;
            fWorkflowPassed = TRUE;
        }
    } else if (iWorkflow == 13 || iWorkflow == 14) {
        if (!FCheckMergeWorkflow(hwndDialog)) goto LFail;
    } else if (iWorkflow == 15 || iWorkflow == 16) {
        if (!FCheckSplitWorkflow(hwndDialog)) goto LFail;
    } else if (iWorkflow >= 17 && iWorkflow <= 19) {
        if (!FCheckCargoWorkflow(hwndDialog)) goto LFail;
    } else if (iWorkflow >= 20) {
        if (!FCheckHullWorkflow(hwndDialog)) goto LFail;
    } else if (iWorkflow >= 8) {
        if (!FCheckShipWorkflow(hwndDialog)) goto LFail;
    } else {
        if (hwndDialog->lpfnDlg != ProductionDlg || !lpplProdGlob) goto LFail;
        if (iWorkflowStep == 0) {
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_PRODUCTION_AVAILABLE_ITEMS, "ListBox");
            if (!hwndControl || SendMessage(hwndControl, LB_GETCOUNT, 0, 0) <= 0 ||
                SendMessage(hwndControl, LB_GETCURSEL, 0, 0) != 0) goto LFail;
            cProductionBefore = CProductionItems();
            if (iWorkflow == 4) {
                if (cProductionBefore != iResearchTarget) goto LFail;
                hwndControl = HwndWorkflowControl(hwndDialog, IDCANCEL, "Button");
                if (!hwndControl || !FPushClick(hwndControl)) goto LFail;
                fWorkflowPassed = TRUE;
                iWorkflowStep++; fWorkflowBusy = FALSE; return;
            }
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_PRODUCTION_ADD, "Button");
            if (!hwndControl || !IsWindowEnabled(hwndControl) || !FPushClick(hwndControl)) goto LFail;
        } else {
            if (CProductionItems() != cProductionBefore + 1) goto LFail;
            hwndControl = HwndWorkflowControl(hwndDialog, iWorkflow == 3 ? IDOK : IDCANCEL, "Button");
            if (!hwndControl || !FPushClick(hwndControl)) goto LFail;
            fWorkflowPassed = TRUE;
        }
    }
    iWorkflowStep++; fWorkflowBusy = FALSE;
    return;
LFail:
    fprintf(stderr, "Gameplay workflow %d step %d failed\n", iWorkflow, iWorkflowStep);
    KillTimer(NULL, id); fWorkflowBusy = fWorkflowPassed = FALSE;
    if (hwndDialog) EndDialog(hwndDialog, 0);
    else PostQuitMessage(93);
}

static BOOL FRunWorkflow(int iKind, int iTarget) {
    UINT_PTR id;
    BOOL fPassed;
    MSG msg;
    HWND hwndTrigger;
    fprintf(stderr, "Starting gameplay workflow %d\n", iKind);
    iWorkflow = iKind; iResearchTarget = iTarget; iWorkflowStep = cWorkflowWait = 0;
    fWorkflowPassed = fWorkflowBusy = FALSE; idWorkflowDialog = 0;
    id = SetTimer(NULL, 0, 50, CheckWorkflow);
    if (!id) return FALSE;
    if (iKind >= 13 && iKind <= 19) {
        hwndTrigger = rghwndBtn[iKind <= 14 ? 10 : iKind <= 16 ? 8 : 7];
        UpdateWindow(GetParent(hwndTrigger));
        if (!IsWindowVisible(hwndTrigger) || !IsWindowEnabled(hwndTrigger) || !FPushClick(hwndTrigger)) {
            fprintf(stderr, "Fleet trigger: visible %d enabled %d fleet %d point %d,%d\n", IsWindowVisible(hwndTrigger), IsWindowEnabled(hwndTrigger), sel.fl.id, sel.fl.pt.x, sel.fl.pt.y);
            KillTimer(NULL, id); iWorkflow = 0; return FALSE;
        }
        while (!fWorkflowPassed || HwndWorkflowDialog() || !IsWindowEnabled(hwndFrame)) {
            if (GetMessage(&msg, NULL, 0, 0) <= 0) break;
            TranslateMessage(&msg); DispatchMessage(&msg);
        }
    }
    else if (iKind == 1) SendMessage(hwndFrame, WM_COMMAND, IDM_GAME_RESEARCH, 0);
    else if (iKind == 5 || iKind == 7) SendMessage(hwndFrame, WM_COMMAND, IDM_GAME_BATTLE_PLANS1, 0);
    else if (iKind == 6) SendMessage(hwndFrame, WM_COMMAND, IDM_GAME_RELATIONS, 0);
    else if (iKind >= 8) SendMessage(hwndFrame, WM_COMMAND, IDM_GAME_SHIP_BUILDER, 0);
    else SendMessage(hwndFrame, WM_CHAR, 'q', 1);
    KillTimer(NULL, id);
    fPassed = fWorkflowPassed && IsWindowEnabled(hwndFrame) && !HwndWorkflowDialog() &&
              idWorkflowDialog && !StarsCompatResolveWindow(idWorkflowDialog);
    if (!fPassed) fprintf(stderr, "Workflow %d returned: passed %d enabled %d dialog %p\n", iKind, fWorkflowPassed, IsWindowEnabled(hwndFrame), (void *)HwndWorkflowDialog());
    iWorkflow = 0;
    return fPassed;
}

static int CFleetTotals(int64_t *rgShip, int64_t *rgCargo) {
    int i, j, c = 0;
    FLEET *lpfl;
    memset(rgShip, 0, 16 * sizeof(*rgShip)); memset(rgCargo, 0, 5 * sizeof(*rgCargo));
    for (i = 0; i < cFleet; i++) {
        lpfl = rglpfl[i];
        if (!lpfl || lpfl->fDead || lpfl->iPlayer != idPlayer) continue;
        c++;
        for (j = 0; j < 16; j++) rgShip[j] += lpfl->rgcsh[j];
        for (j = 0; j < 5; j++) rgCargo[j] += lpfl->rgwtMin[j];
    }
    return c;
}

static BOOL FCheckRepeatOrders(int idFleet) {
    MSG msg;
    UINT_PTR id;
    Uint64 ullStart;
    BOOL fRepeat = sel.fl.fRepOrders, fPassed = FALSE;
    FLEET *lpfl;
    int i;
    UpdateWindow(GetParent(hwndRepCB));
    if (!IsWindowVisible(hwndRepCB) || !IsWindowEnabled(hwndRepCB) ||
        SendMessage(hwndRepCB, BM_GETCHECK, 0, 0) != fRepeat) {
        fprintf(stderr, "Repeat initial visible %d enabled %d check %ld expected %d\n", IsWindowVisible(hwndRepCB), IsWindowEnabled(hwndRepCB), (long)SendMessage(hwndRepCB, BM_GETCHECK, 0, 0), fRepeat);
        return FALSE;
    }
    id = SetTimer(NULL, 0, 25, NULL);
    if (!id) return FALSE;
    for (i = 0; i < 2; i++) {
        if (!FPushClick(hwndRepCB)) goto LDone;
        ullStart = SDL_GetTicks();
        while (sel.fl.fRepOrders != (i == 0 ? !fRepeat : fRepeat) && SDL_GetTicks() - ullStart < 3000) {
            if (GetMessage(&msg, NULL, 0, 0) <= 0) goto LDone;
            TranslateMessage(&msg); DispatchMessage(&msg);
        }
        lpfl = LpflFromId((int16_t)idFleet);
        if (!lpfl || lpfl->fRepOrders != (i == 0 ? !fRepeat : fRepeat) ||
            sel.fl.fRepOrders != lpfl->fRepOrders || SendMessage(hwndRepCB, BM_GETCHECK, 0, 0) != lpfl->fRepOrders) goto LDone;
        SelectAdjFleet(0, (int16_t)idFleet);
        UpdateWindow(GetParent(hwndRepCB));
        if (SendMessage(hwndRepCB, BM_GETCHECK, 0, 0) != lpfl->fRepOrders) goto LDone;
    }
    fPassed = TRUE;
LDone:
    if (!fPassed) fprintf(stderr, "Repeat step %d check %ld selected %d expected %d\n", i, (long)SendMessage(hwndRepCB, BM_GETCHECK, 0, 0), sel.fl.fRepOrders, i == 0 ? !fRepeat : fRepeat);
    KillTimer(NULL, id);
    return fPassed;
}

static BOOL FChooseWorkflowCombo(HWND hwnd, int iRow) {
    MSG msg;
    UINT_PTR id;
    Uint64 ullStart;
    BOOL fPassed = FALSE;
    if (!IsWindowVisible(hwnd) || !IsWindowEnabled(hwnd) || SendMessage(hwnd, CB_GETCOUNT, 0, 0) <= iRow) return FALSE;
    id = SetTimer(NULL, 0, 25, NULL);
    if (!id) return FALSE;
    if (!FPushClick(hwnd)) goto LDone;
    ullStart = SDL_GetTicks();
    while (!SendMessage(hwnd, CB_GETDROPPEDSTATE, 0, 0) && SDL_GetTicks() - ullStart < 3000) {
        if (GetMessage(&msg, NULL, 0, 0) <= 0) goto LDone;
        TranslateMessage(&msg); DispatchMessage(&msg);
    }
    if (!SendMessage(hwnd, CB_GETDROPPEDSTATE, 0, 0) || !FPushComboRow(hwnd, iRow)) goto LDone;
    ullStart = SDL_GetTicks();
    while (SendMessage(hwnd, CB_GETDROPPEDSTATE, 0, 0) && SDL_GetTicks() - ullStart < 3000) {
        if (GetMessage(&msg, NULL, 0, 0) <= 0) goto LDone;
        TranslateMessage(&msg); DispatchMessage(&msg);
    }
    fPassed = !SendMessage(hwnd, CB_GETDROPPEDSTATE, 0, 0) && SendMessage(hwnd, CB_GETCURSEL, 0, 0) == iRow && !HwndWorkflowDialog();
LDone:
    KillTimer(NULL, id);
    return fPassed;
}

static BOOL FCheckTransportPreset(int iPreset) {
    MSG msg;
    UINT_PTR id;
    Uint64 ullStart;
    BOOL fPassed = FALSE;
    int cBefore = cTransportMenus;
    if (rgrcRef[5].bottom <= rgrcRef[5].top) return FALSE;
    id = SetTimer(NULL, 0, 25, NULL);
    if (!id) return FALSE;
    iTransportPreset = iPreset;
    if (!FPushClientRect(hwndPlanet, &rgrcRef[5], SDL_BUTTON_RIGHT)) goto LDone;
    ullStart = SDL_GetTicks();
    while ((cTransportMenus == cBefore || iTransportPreset >= 0) && SDL_GetTicks() - ullStart < 3000) {
        if (GetMessage(&msg, NULL, 0, 0) <= 0) goto LDone;
        TranslateMessage(&msg); DispatchMessage(&msg);
    }
    fPassed = cTransportMenus == cBefore + 1 && iTransportPreset == -1 && !HwndWorkflowDialog();
LDone:
    iTransportPreset = -1;
    KillTimer(NULL, id);
    return fPassed;
}

static BOOL FCheckWaypointTask(int idFleet) {
    FLEET *lpfl;
    ORDER ordSav;
    int i;
    int iwp = sel.iwpAct;
    if (sel.grobj != grobjFleet || sel.fl.id != idFleet || iwp != 0 || sel.fl.cord == 0) return FALSE;
    ordSav = sel.fl.lpplord->rgord[iwp];
    if (ordSav.grTask != grTaskNone) return FALSE;
    UpdateWindow(GetParent(rghwndOrderDD[0]));
    if (!FChooseWorkflowCombo(rghwndOrderDD[0], grTaskXfer)) return FALSE;
    lpfl = LpflFromId((int16_t)idFleet);
    if (!lpfl || lpfl->lpplord->rgord[iwp].grTask != grTaskXfer || sel.fl.lpplord->rgord[iwp].grTask != grTaskXfer) return FALSE;
    if (!FCheckTransportPreset(0)) return FALSE;
    for (i = 0; i < 5; i++)
        if (lpfl->lpplord->rgord[iwp].txp.rgia[i].iAction != (i < 3 ? iActionLoadAll : i == 3 ? iActionNone : iActionLoadDunnage)) return FALSE;
    if (!FCheckTransportPreset(1)) return FALSE;
    for (i = 0; i < 5; i++)
        if (lpfl->lpplord->rgord[iwp].txp.rgia[i].iAction != (i < 4 ? iActionUnloadAll : iActionLoadDunnage)) return FALSE;
    if (!FCheckTransportPreset(2)) return FALSE;
    for (i = 0; i < 5; i++) {
        if (lpfl->lpplord->rgord[iwp].txp.rgia[i].iAction != (i < 3 ? iActionWaitPercent : i == 3 ? iActionNone : iActionLoadDunnage)) return FALSE;
        if (i < 3 && lpfl->lpplord->rgord[iwp].txp.rgia[i].cQuan != 100) return FALSE;
    }
    if (!FCheckTransportPreset(3)) return FALSE;
    for (i = 0; i < 5; i++)
        if (lpfl->lpplord->rgord[iwp].txp.rgia[i].iAction != iActionNone || lpfl->lpplord->rgord[iwp].txp.rgia[i].cQuan != 0) return FALSE;
    SelectAdjFleet(0, (int16_t)idFleet);
    UpdateWindow(GetParent(rghwndOrderDD[0]));
    if (SendMessage(rghwndOrderDD[0], CB_GETCURSEL, 0, 0) != grTaskXfer ||
        !FChooseWorkflowCombo(rghwndOrderDD[0], grTaskNone)) return FALSE;
    SelectAdjFleet(0, (int16_t)idFleet);
    UpdateWindow(GetParent(rghwndOrderDD[0]));
    if (SendMessage(rghwndOrderDD[0], CB_GETCURSEL, 0, 0) != grTaskNone ||
        memcmp(&ordSav, &lpfl->lpplord->rgord[iwp], sizeof(ordSav))) return FALSE;
    puts("Waypoint task commit, reselection and restoration passed");
    return TRUE;
}

static BOOL FPushWorkflowKey(HWND hwnd, SDL_Scancode sc, BOOL fDown) {
    HWND hwndRoot = HwndCompatRoot(hwnd);
    SDL_Event event = {0};
    if (!hwndRoot) return FALSE;
    event.type = fDown ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.windowID = hwndRoot->id; event.key.scancode = sc;
    event.key.key = SDL_GetKeyFromScancode(sc, SDL_KMOD_NONE, false); event.key.down = !!fDown;
    return SDL_PushEvent(&event);
}

static BOOL FPushWorkflowDrag(HWND hwnd, POINT ptFrom, POINT ptTo) {
    HWND hwndRoot = HwndCompatRoot(hwnd);
    SDL_Event event = {0};
    if (!hwndRoot || !ClientToScreen(hwnd, &ptFrom) || !ScreenToClient(hwndRoot, &ptFrom) ||
        !ClientToScreen(hwnd, &ptTo) || !ScreenToClient(hwndRoot, &ptTo)) return FALSE;
    ptFrom.y += CyCompatMenuBar(hwndRoot); ptTo.y += CyCompatMenuBar(hwndRoot);
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.windowID = hwndRoot->id;
    event.button.button = SDL_BUTTON_LEFT; event.button.down = true;
    event.button.x = (float)ptFrom.x; event.button.y = (float)ptFrom.y;
    if (!SDL_PushEvent(&event)) return FALSE;
    event.type = SDL_EVENT_MOUSE_MOTION; event.motion.windowID = hwndRoot->id;
    event.motion.state = SDL_BUTTON_LMASK; event.motion.x = (float)ptTo.x; event.motion.y = (float)ptTo.y;
    event.motion.xrel = (float)(ptTo.x - ptFrom.x); event.motion.yrel = (float)(ptTo.y - ptFrom.y);
    if (!SDL_PushEvent(&event)) return FALSE;
    event = (SDL_Event){0}; event.type = SDL_EVENT_MOUSE_BUTTON_UP; event.button.windowID = hwndRoot->id;
    event.button.button = SDL_BUTTON_LEFT; event.button.down = false;
    event.button.x = (float)ptTo.x; event.button.y = (float)ptTo.y;
    return SDL_PushEvent(&event);
}

static void DispatchWorkflowInput(MSG *lpmsg) {
    TranslateMessage(lpmsg);
    if (((lpmsg->message != WM_KEYDOWN && lpmsg->message != WM_KEYUP) ||
        !FHandleKey(lpmsg->hwnd, lpmsg->message, lpmsg->wParam, lpmsg->lParam)) &&
        (lpmsg->message != WM_CHAR || !FHandleChar(lpmsg->hwnd, lpmsg->wParam, lpmsg->lParam))) DispatchMessage(lpmsg);
}

static BOOL FCheckRouteAppendDelete(int idFleet) {
    SCANSNAPSHOT snap = {0};
    SCANOBJECT obj, objDrag;
    POINT ptFrom;
    FLEET *lpfl = LpflFromId((int16_t)idFleet);
    ORDER ordSav;
    RECT rc;
    MSG msg;
    UINT_PTR id;
    Uint64 ullStart;
    size_t i;
    BOOL fPassed = FALSE;
    if (!lpfl || lpfl->cord != 1) return FALSE;
    ordSav = lpfl->lpplord->rgord[0];
    if (!FCaptureScanner(&snap)) return FALSE;
    for (i = 0; i < snap.cObject; i++) {
        obj = snap.rgobj[i];
        if (obj.grType == grobjPlanet && obj.id != lpfl->idPlanet && obj.fOnScreen && !obj.fFiltered &&
            obj.ptClient.x > 3 && obj.ptClient.y > 3 && obj.ptClient.x < snap.rcViewport.right - 3 &&
            obj.ptClient.y < snap.rcViewport.bottom - 3) break;
    }
    fPassed = i < snap.cObject;
    FreeScannerSnapshot(&snap);
    if (!fPassed) return FALSE;
    fPassed = FALSE;
    rc = (RECT){obj.ptClient.x - 1, obj.ptClient.y - 1, obj.ptClient.x + 1, obj.ptClient.y + 1};
    id = SetTimer(NULL, 0, 25, NULL);
    if (!id) return FALSE;
    if (!FPushWorkflowKey(hwndFrame, SDL_SCANCODE_LSHIFT, TRUE) || !FPushClientRect(hwndScanner, &rc, SDL_BUTTON_LEFT) ||
        !FPushWorkflowKey(hwndFrame, SDL_SCANCODE_LSHIFT, FALSE)) goto LDone;
    ullStart = SDL_GetTicks();
    while (lpfl->cord != 2 && SDL_GetTicks() - ullStart < 3000) {
        if (GetMessage(&msg, NULL, 0, 0) <= 0) goto LDone;
        DispatchWorkflowInput(&msg);
    }
    if (lpfl->cord != 2 || lpfl->lpplord->rgord[1].grobj != grobjPlanet || lpfl->lpplord->rgord[1].id != obj.id ||
        lpfl->lpplord->rgord[1].pt.x != obj.ptUniverse.x || lpfl->lpplord->rgord[1].pt.y != obj.ptUniverse.y) goto LDone;
    // Re-capture after the append: the UI may have changed the scanner view.
    if (!FCaptureScanner(&snap)) goto LDone;
    for (i = 0; i < snap.cObject; i++)
        if (snap.rgobj[i].grType == grobjPlanet && snap.rgobj[i].id == obj.id) { ptFrom = snap.rgobj[i].ptClient; break; }
    fPassed = i < snap.cObject;
    if (fPassed) for (i = 0; i < snap.cObject; i++) {
        objDrag = snap.rgobj[i];
        if (objDrag.grType == grobjPlanet && objDrag.id != obj.id && objDrag.id != lpfl->idPlanet &&
            objDrag.fOnScreen && !objDrag.fFiltered && objDrag.ptClient.x > 3 && objDrag.ptClient.y > 3 &&
            objDrag.ptClient.x < snap.rcViewport.right - 3 && objDrag.ptClient.y < snap.rcViewport.bottom - 3) break;
    }
    fPassed = fPassed && i < snap.cObject;
    FreeScannerSnapshot(&snap);
    if (!fPassed) goto LDone;
    fPassed = FALSE;
    if (!FPushWorkflowDrag(hwndScanner, ptFrom, objDrag.ptClient)) goto LDone;
    ullStart = SDL_GetTicks();
    while (lpfl->lpplord->rgord[1].id != objDrag.id && SDL_GetTicks() - ullStart < 3000) {
        if (GetMessage(&msg, NULL, 0, 0) <= 0) goto LDone;
        DispatchWorkflowInput(&msg);
    }
    if (lpfl->cord != 2 || lpfl->lpplord->rgord[1].grobj != grobjPlanet || lpfl->lpplord->rgord[1].id != objDrag.id ||
        lpfl->lpplord->rgord[1].pt.x != objDrag.ptUniverse.x || lpfl->lpplord->rgord[1].pt.y != objDrag.ptUniverse.y) goto LDone;
    UpdateWindow(hwndPlanet);
    if (!FPushListRow(hwndShipLB, 1) || !FPushWorkflowKey(hwndFrame, SDL_SCANCODE_DELETE, TRUE) ||
        !FPushWorkflowKey(hwndFrame, SDL_SCANCODE_DELETE, FALSE)) goto LDone;
    ullStart = SDL_GetTicks();
    while (lpfl->cord != 1 && SDL_GetTicks() - ullStart < 3000) {
        if (GetMessage(&msg, NULL, 0, 0) <= 0) goto LDone;
        DispatchWorkflowInput(&msg);
    }
    fPassed = lpfl->cord == 1 && sel.fl.cord == 1 && !memcmp(&ordSav, &lpfl->lpplord->rgord[0], sizeof(ordSav));
LDone:
    KillTimer(NULL, id);
    if (!fPassed) fprintf(stderr, "Route append/delete failed: count %d selected %d waypoint %d\n", lpfl->cord, sel.fl.cord, sel.iwpAct);
    else puts("Scanner Shift-click append, destination drag and waypoint-list Delete passed");
    return fPassed;
}

static BOOL FCheckCargoWorkflows(void) {
    FLEET *lpfl = NULL;
    PLANET *lppl = NULL;
    int32_t rgCargoFleet[5], rgCargoPlanet[4];
    int i, j, idFleet, idPlanet;
    for (i = 0; i < cFleet; i++) {
        lpfl = rglpfl[i];
        if (!lpfl || lpfl->fDead || lpfl->iPlayer != idPlayer || lpfl->idPlanet == idPlanetDeepSpace || GetCargoFree(lpfl) <= 0) continue;
        lppl = LpplFromId(lpfl->idPlanet);
        if (!lppl || lppl->iPlayer != idPlayer) continue;
        for (j = 0; j < 3 && lppl->rgwtMin[j] <= 0; j++) {}
        if (j != 3) break;
    }
    if (i == cFleet) return FALSE;
    idFleet = lpfl->id; idPlanet = lppl->id;
    memcpy(rgCargoFleet, lpfl->rgwtMin, sizeof(rgCargoFleet));
    memcpy(rgCargoPlanet, lppl->rgwtMin, sizeof(rgCargoPlanet));
    SelectAdjFleet(0, (int16_t)idFleet);
    if (!FRunWorkflow(17, 0)) return FALSE;
    lpfl = LpflFromId((int16_t)idFleet); lppl = LpplFromId((int16_t)idPlanet);
    if (!lpfl || !lppl || memcmp(rgCargoFleet, lpfl->rgwtMin, sizeof(rgCargoFleet)) ||
        memcmp(rgCargoPlanet, lppl->rgwtMin, sizeof(rgCargoPlanet))) return FALSE;
    if (!FRunWorkflow(18, 0)) return FALSE;
    lpfl = LpflFromId((int16_t)idFleet); lppl = LpplFromId((int16_t)idPlanet);
    if (!lpfl || !lppl) return FALSE;
    for (i = 0; i < 5; i++) {
        if (lpfl->rgwtMin[i] != rgCargoFleet[i] + (i == iSupplyWorkflow ? 1 : 0)) return FALSE;
        if (i < 4 && lppl->rgwtMin[i] != rgCargoPlanet[i] - (i == iSupplyWorkflow ? 1 : 0)) return FALSE;
    }
    if (!FRunWorkflow(19, 0)) return FALSE;
    lpfl = LpflFromId((int16_t)idFleet); lppl = LpplFromId((int16_t)idPlanet);
    if (!lpfl || !lppl || memcmp(rgCargoFleet, lpfl->rgwtMin, sizeof(rgCargoFleet)) ||
        memcmp(rgCargoPlanet, lppl->rgwtMin, sizeof(rgCargoPlanet))) return FALSE;
    puts("Planet/fleet cargo transfer cancel, accept and reverse workflows passed");
    return TRUE;
}

static BOOL FCheckFleetWorkflows(void) {
    int64_t rgShipSav[16], rgCargoSav[5], rgShip[16], rgCargo[5];
    FLEET *lpfl, *rgflSav;
    int i, cFleetSav = cFleet, cOwned, idFleet = idflNone;
    BOOL fPassed = FALSE;
    cOwned = CFleetTotals(rgShipSav, rgCargoSav);
    if (cOwned < 2) return FALSE;
    rgflSav = SDL_calloc((size_t)cFleet, sizeof(*rgflSav));
    if (!rgflSav) return FALSE;
    for (i = 0; i < cFleet; i++) {
        lpfl = rglpfl[i];
        if (!lpfl) goto LDone;
        rgflSav[i] = *lpfl;

        if (idFleet == idflNone && lpfl->iPlayer == idPlayer && !lpfl->fDead) idFleet = lpfl->id;
    }
    // The small startup frame clips the lower orders controls; use a normal gameplay size.
    SetWindowPos(hwndFrame, NULL, 0, 0, 900, 700, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    if (!FCheckCargoWorkflows()) goto LDone;
    // Select through the game's normal fleet UI entry; only copied fixtures are edited.
    SelectAdjFleet(0, (int16_t)idFleet);
    if (sel.grobj != grobjFleet || sel.fl.id != idFleet || !FCheckRepeatOrders(idFleet) || !FCheckWaypointTask(idFleet) || !FCheckRouteAppendDelete(idFleet) || !FRunWorkflow(13, 0) || cFleet != cFleetSav ||
        CFleetTotals(rgShip, rgCargo) != cOwned || memcmp(rgShip, rgShipSav, sizeof(rgShip)) ||
        memcmp(rgCargo, rgCargoSav, sizeof(rgCargo))) goto LDone;
    for (i = 0; i < cFleet; i++) {
        lpfl = rglpfl[i];
        if (!lpfl || lpfl->id != rgflSav[i].id || lpfl->fDead != rgflSav[i].fDead || lpfl->cord != rgflSav[i].cord ||
            memcmp(lpfl->rgcsh, rgflSav[i].rgcsh, sizeof(lpfl->rgcsh)) ||
            memcmp(lpfl->rgwtMin, rgflSav[i].rgwtMin, sizeof(lpfl->rgwtMin))) goto LDone;
    }
    if (!FRunWorkflow(14, 0) || CFleetTotals(rgShip, rgCargo) != cOwned - 1 ||
        memcmp(rgShip, rgShipSav, sizeof(rgShip)) || memcmp(rgCargo, rgCargoSav, sizeof(rgCargo))) goto LDone;
    if (!FRunWorkflow(15, 0) || CFleetTotals(rgShip, rgCargo) != cOwned - 1 ||
        memcmp(rgShip, rgShipSav, sizeof(rgShip)) || memcmp(rgCargo, rgCargoSav, sizeof(rgCargo))) goto LDone;
    lpfl = LpflFromId((int16_t)idXferWorkflow);
    if (lpfl && !lpfl->fDead) goto LDone;
    if (!FRunWorkflow(16, 0) || CFleetTotals(rgShip, rgCargo) != cOwned ||
        memcmp(rgShip, rgShipSav, sizeof(rgShip)) || memcmp(rgCargo, rgCargoSav, sizeof(rgCargo))) goto LDone;
    lpfl = LpflFromId((int16_t)idXferWorkflow);
    if (!lpfl || lpfl->fDead || lpfl->rgcsh[ishXferWorkflow] != 1) goto LDone;
    puts("Fleet repeat-orders, merge and split/cancel workflows passed");
    fPassed = TRUE;
LDone:
    SDL_free(rgflSav);
    return fPassed;
}

#include "win32_designer_workflow.h"
#include "win32_ui_polish.h"

static BOOL FCheckGameplayWorkflows(void) {
    PROD rgprodSav[64];
    SHDEF rgshdefSav[16];
    BTLPLAN btlplanSav = rglpbtlplan[idPlayer][0];
    int iRelationSav = rgplr[idPlayer].rgmdRelation[idPlayer == 0];
    int iResearchSav = rgplr[idPlayer].iTechNow, cProdSav, cProdItemsSav = 0, i, iResearchNew = (iResearchSav + 1) % 6;
    int16_t fNoResearchSav, fSinglePlrSav;
    const char *pszWorkflow = getenv("STARS_CLIENT_WORKFLOW");
    if (pszWorkflow) {
        if (strcmp(pszWorkflow, "polish") == 0) return FCheckMessageWheel() && FCheckPlanetPane();
        if (strcmp(pszWorkflow, "designer") == 0) return FCheckDesignerWorkflows() && FCheckSaveReloadWorkflow();
        return strcmp(pszWorkflow, "fleet") == 0 && FCheckFleetWorkflows();
    }
    if (sel.grobj != grobjPlanet || sel.pl.iPlayer != idPlayer) {
        fprintf(stderr, "Workflow selection: grobj %u planet %d owner %d player %d\n", sel.grobj, sel.pl.id, sel.pl.iPlayer, idPlayer);
        return FALSE;
    }
    if (!FRunWorkflow(1, iResearchNew) || rgplr[idPlayer].iTechNow != iResearchNew) {
        fprintf(stderr, "Research commit: wanted %d got %d\n", iResearchNew, rgplr[idPlayer].iTechNow); return FALSE;
    }
    if (!FRunWorkflow(1, iResearchSav) || rgplr[idPlayer].iTechNow != iResearchSav) {
        fprintf(stderr, "Research restore: wanted %d got %d\n", iResearchSav, rgplr[idPlayer].iTechNow); return FALSE;
    }
    cProdSav = sel.pl.lpplprod ? sel.pl.lpplprod->iprodMac : 0;
    if (cProdSav > (int)SDL_arraysize(rgprodSav)) { fprintf(stderr, "Production count %d\n", cProdSav); return FALSE; }
    if (cProdSav != 0) memcpy(rgprodSav, sel.pl.lpplprod->rgprod, (size_t)cProdSav * sizeof(PROD));
    for (i = 0; i < cProdSav; i++) cProdItemsSav += rgprodSav[i].cItem;
    fNoResearchSav = sel.pl.fNoResearch;
    if (!FRunWorkflow(2, 0) || lpplProdGlob || hwndProdDlg || fDlgUp || sel.pl.fNoResearch != fNoResearchSav ||
        (sel.pl.lpplprod ? sel.pl.lpplprod->iprodMac : 0) != cProdSav ||
        (cProdSav != 0 && memcmp(rgprodSav, sel.pl.lpplprod->rgprod, (size_t)cProdSav * sizeof(PROD)))) { fprintf(stderr, "Production cancel: passed %d glob %p dialog %p up %d research %d/%d count %d/%d\n", fWorkflowPassed, (void *)lpplProdGlob, (void *)hwndProdDlg, fDlgUp, sel.pl.fNoResearch, fNoResearchSav, sel.pl.lpplprod ? sel.pl.lpplprod->iprodMac : 0, cProdSav); return FALSE; }
    if (!FRunWorkflow(3, 0) || !sel.pl.lpplprod) return FALSE;
    for (i = 0, cProdSav = 0; i < sel.pl.lpplprod->iprodMac; i++) cProdSav += sel.pl.lpplprod->rgprod[i].cItem;
    if (cProdSav != cProdItemsSav + 1 || !FRunWorkflow(4, cProdSav)) return FALSE;
    if (!FRunWorkflow(5, !btlplanSav.fDumpCargo) || rglpbtlplan[idPlayer][0].fDumpCargo != !btlplanSav.fDumpCargo ||
        !FRunWorkflow(5, btlplanSav.fDumpCargo) || memcmp(&btlplanSav, &rglpbtlplan[idPlayer][0], sizeof(btlplanSav))) return FALSE;
    if (!FRunWorkflow(7, (btlplanSav.mdTactic + 1) % 6) || rglpbtlplan[idPlayer][0].mdTactic != (btlplanSav.mdTactic + 1) % 6 ||
        !FRunWorkflow(7, btlplanSav.mdTactic) || memcmp(&btlplanSav, &rglpbtlplan[idPlayer][0], sizeof(btlplanSav))) return FALSE;
    if (game.cPlayer < 2) return FALSE;
    fSinglePlrSav = game.fSinglePlr;
    if (fSinglePlrSav) {
        SendMessage(hwndFrame, WM_COMMAND, IDM_GAME_RELATIONS, 0);
        if (HwndWorkflowDialog() || !IsWindowEnabled(hwndFrame)) return FALSE;
    }
    // The copied human/AI fixture disables diplomacy in single-player mode.
    // Exercise its multiplayer UI in memory, then restore the game-mode flag.
    game.fSinglePlr = FALSE;
    fWorkflowPassed = FRunWorkflow(6, (iRelationSav + 1) % 3) &&
        FRunWorkflow(6, iRelationSav) && rgplr[idPlayer].rgmdRelation[idPlayer == 0] == iRelationSav;
    game.fSinglePlr = fSinglePlrSav;
    if (!fWorkflowPassed) return FALSE;
    memcpy(rgshdefSav, rgshdef, sizeof(rgshdefSav));
    cShdefWorkflow = rgplr[idPlayer].cShDef;
    for (ishdefWorkflow = 0; ishdefWorkflow < 16 && !rgshdef[ishdefWorkflow].fFree; ishdefWorkflow++) {}
    if (ishdefWorkflow == 16 || !FRunWorkflow(8, 0) || !FRunWorkflow(9, 0) ||
        !FRunWorkflow(10, 0) || !FRunWorkflow(11, 0) || !FRunWorkflow(12, 0)) return FALSE;
    for (i = 0; i < 16; i++) {
        // Drawing the designer materializes its lazy combat-power cache.
        if (!rgshdefSav[i].fFree && rgshdefSav[i].lPower == -1 && rgshdef[i].lPower != -1)
            rgshdefSav[i].lPower = LComputePower(&rgshdefSav[i]);
        if (rgshdefSav[i].fFree != rgshdef[i].fFree ||
            (!rgshdefSav[i].fFree && memcmp(&rgshdefSav[i], &rgshdef[i], sizeof(SHDEF)))) {
            fprintf(stderr, "Ship restore mismatch: design %d\n", i);
            return FALSE;
        }
    }
    printf("Research, production, battle-plan, relation and ship-design workflows passed\n");
    return TRUE;
}

int16_t StarsTest_AlertSz(char *sz, int16_t mbType) {
    (void)mbType;
    fprintf(stderr, "Startup alert: %s\n", sz);
    cAlerts++;
    if (getenv("STARS_CLIENT_LIBRARY_FAIL_WRITE")) return IDOK;
    PostQuitMessage(91);
    return IDOK;
}

#include "win32_newgame_workflow.h"

static void CALLBACK CheckStartup(HWND hwnd, UINT msg, UINT_PTR id, DWORD dwTime) {
    HWND lpFrame, lpTitle;
    const char *pszCapture = getenv("STARS_CLIENT_CAPTURE");
    const char *pszWizardCapture = getenv("STARS_CLIENT_WIZARD_CAPTURE");
    const char *pszRaceCapture = getenv("STARS_CLIENT_RACE_CAPTURE");
    const char *pszGameCapture = getenv("STARS_CLIENT_GAME_CAPTURE");
    static const DLGPROC rglpfnRace[] = {RaceWizardDlg1, RaceWizardDlg4, RaceWizardDlg5, RaceWizardDlg2, RaceWizardDlg3, RaceWizardDlg6};
    BYTE *rgb;
    size_t cb, i;
    int iButton;
    SDL_Event event = {0};
    BOOL fDifferent = FALSE;
    RECT rcFrame;
    RECT rcScanner;

    (void)hwnd; (void)msg; (void)dwTime;
    if (getenv("STARS_CLIENT_LIBRARY_OPEN")) {
        const char *pszName = getenv("STARS_CLIENT_LIBRARY_OPEN");
        HWND hwndList;
        LRESULT iGame;
        if (iStage == 0) {
            if (!IsWindowVisible(hwndTitle)) goto LDone;
            hwndList = GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES);
            iGame = SendMessage(hwndList, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pszName);
            if (iGame == LB_ERR) goto LDone;
            SendMessage(hwndList, LB_SETCURSEL, iGame, 0);
            fChecked = TRUE;
            iStage = 1;
            if (!FPushClick(rghwndBtnSplash[1])) goto LDone;
            return;
        }
        if (IsWindow(hwndTitle)) return;
        if (!lpPlanets || game.lid == 0 || idPlayer == iplrNone || !IsWindowVisible(hwndFrame)) goto LDone;
        fPassed = TRUE;
        puts("Managed game: visible in library and reopened after restart");
        PostQuitMessage(0);
        return;
    }
    if (pszNewGame) {
        if (!FCheckNewGameWorkflow()) goto LDone;
        return;
    }
    if (iStage == 10) return;
    lpFrame = LpCompatWindow(hwndFrame);
    lpTitle = LpCompatWindow(hwndTitle);
    if (!GetClientRect(hwndFrame, &rcFrame) || rcFrame.right < 320 || rcFrame.bottom < 240) goto LDone;
    if (iStage == 5) {
        // File loading pumps messages while building the client; check only
        // after the title's Continue handler has finished replacing it.
        if (IsWindow(hwndTitle)) return;
        if (IsWindow(hwndTitle) || !IsWindowVisible(hwndFrame) || !IsWindow(hwndScanner) ||
            !lpPlanets || game.lid == 0 || idPlayer == iplrNone) goto LDone;
        if (vfs.dx != rcFrame.right || vfs.dy != rcFrame.bottom ||
            !GetClientRect(hwndScanner, &rcScanner) || rcScanner.right < 100 || rcScanner.bottom < 100) goto LDone;
        for (i = 0; i < 6; i++) if (!rgtilePlanet[i].fPopped) goto LDone;
        for (i = 0; i < 7; i++) if (!rgtileShip[i].fPopped) goto LDone;
        UpdateWindow(hwndFrame);
        for (hwnd = GetWindow(hwndFrame, GW_CHILD); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) UpdateWindow(hwnd);
        if (!FCompatPresent(hwndFrame) || (pszGameCapture && !SDL_SaveBMP(lpFrame->lpComposite, pszGameCapture))) goto LDone;
        SendMessage(hwndFrame, WM_COMMAND, IDM_VIEW_BROWSER_TOGGLE, 0);
        iStage = 6;
        return;
    }
    if (iStage == 6 || iStage == 8) {
        if (!IsWindowVisible(hwndBrowser) || !IsWindow(hwndBrowserChild)) goto LDone;
        UpdateWindow(hwndBrowser);
        UpdateWindow(hwndBrowserChild);
        if (!FCompatPresent(hwndBrowser)) goto LDone;
        hwndBrowserPrevious = hwndBrowser;
        if (iStage == 6) {
            if (!FPushClick(GetDlgItem(hwndBrowser, IDCANCEL))) goto LDone;
        } else {
            event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
            event.window.windowID = hwndBrowser->id;
            if (!SDL_PushEvent(&event)) goto LDone;
        }
        iStage++;
        return;
    }
    if (iStage == 7 || iStage == 9) {
        if (IsWindow(hwndBrowserPrevious) || hwndBrowser || !IsWindowEnabled(hwndFrame)) goto LDone;
        if (iStage == 7) {
            SendMessage(hwndFrame, WM_COMMAND, IDM_VIEW_BROWSER_TOGGLE, 0);
            iStage = 8;
        } else {
            // These checks may pump an already queued timer message.
            iStage = 10;
            KillTimer(NULL, id);
            fprintf(stderr, "Starting loaded-game inspection checks\n");
            if (!FCheckToolbarPlacement()) { fprintf(stderr, "Toolbar check failed\n"); goto LDone; }
            if (!FCheckScannerDrag()) { fprintf(stderr, "Scanner drag check failed\n"); goto LDone; }
            fprintf(stderr, "Starting map inspection check\n");
            if (!FCheckMapInspection()) { fprintf(stderr, "Map inspection check failed\n"); goto LDone; }
            fprintf(stderr, "Starting report check\n");
            if (!FCheckReportScrollbars()) { fprintf(stderr, "Report scroll check failed\n"); goto LDone; }
            if (!FCheckGameplayWorkflows()) goto LDone;
            fPassed = TRUE;
            PostQuitMessage(0);
        }
        return;
    }
    if (!lpFrame || !lpTitle || lpFrame == lpTitle || !IsWindowVisible(hwndTitle) ||
        !lpFrame->lpWindow || !lpTitle->lpWindow || lpFrame->id == lpTitle->id ||
        !lpTitle->lpRenderer || !lpTitle->lpCanvas ||
        (SDL_GetWindowFlags(lpTitle->lpWindow) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS)) ||
        (GetWindowLongPtr(hwndTitle, GWL_STYLE) & WS_CAPTION) != WS_CAPTION ||
        !GetClientRect(hwndTitle, &rcScanner) || rcScanner.right < 600 || rcScanner.bottom < 400 ||
        rcScanner.right > 800 || rcScanner.bottom > 600) goto LDone;
    if (iStage == 1) {
        for (hwndWizard = GetWindow(hwndTitle, GW_HWNDFIRST); hwndWizard; hwndWizard = GetWindow(hwndWizard, GW_HWNDNEXT)) {
            if (hwndWizard->fDialog && hwndWizard->fModal && hwndWizard->hwndOwner == hwndTitle) break;
        }
        if (!hwndWizard) {
            if (++cWait < 20) return;
            goto LDone;
        }
        if (!IsWindowVisible(hwndWizard) || IsWindowEnabled(hwndTitle) ||
            (strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0 &&
             SDL_GetWindowParent(hwndWizard->lpWindow) != lpTitle->lpWindow)) goto LDone;
        UpdateWindow(hwndWizard);
        for (hwnd = GetWindow(hwndWizard, GW_CHILD); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) UpdateWindow(hwnd);
        if (!FCompatPresent(hwndWizard) || (pszWizardCapture &&
            !SDL_SaveBMP(hwndWizard->lpComposite, pszWizardCapture))) goto LDone;
        if (pszRaceCapture) {
            if (!FPushClick(GetDlgItem(hwndWizard, IDC_SIMPLE_NEW_GAME_CUSTOMIZE_RACE))) goto LDone;
            iStage = 3;
            cWait = 0;
            return;
        }
        event.type = SDL_EVENT_KEY_DOWN;
        event.key.windowID = hwndWizard->id;
        event.key.key = SDLK_ESCAPE;
        event.key.scancode = SDL_SCANCODE_ESCAPE;
        event.key.down = true;
        if (!SDL_PushEvent(&event)) goto LDone;
        event.type = SDL_EVENT_KEY_UP;
        event.key.down = false;
        if (!SDL_PushEvent(&event)) goto LDone;
        iStage = 2;
        cWait = 0;
        return;
    }
    if (iStage == 3) {
        for (hwndRace = GetWindow(hwndTitle, GW_HWNDFIRST); hwndRace; hwndRace = GetWindow(hwndRace, GW_HWNDNEXT)) {
            if (hwndRace->fDialog && hwndRace->fModal && hwndRace->hwndOwner == hwndWizard &&
                hwndRace->lpfnDlg == rglpfnRace[iRacePage]) break;
        }
        if (!hwndRace) {
            if (++cWait < 20) return;
            goto LDone;
        }
        UpdateWindow(hwndRace);
        for (hwnd = GetWindow(hwndRace, GW_CHILD); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) UpdateWindow(hwnd);
        if (!IsWindowVisible(hwndRace) || IsWindowEnabled(hwndWizard) || !FCompatPresent(hwndRace)) goto LDone;
        if (iRacePage == 0) idRaceWindow = hwndRace->id;
        else if (hwndRace->id != idRaceWindow || hwndRace == hwndRacePrevious || IsWindow(hwndRacePrevious)) goto LDone;
        hwndRacePrevious = hwndRace;
        if (iRacePage == 3 && (!fHabPainted || !SDL_SaveBMP(hwndRace->lpComposite, pszRaceCapture))) goto LDone;
        if (iRacePage == 5 && !fRaceBack) {
            if (!FPushClick(GetDlgItem(hwndRace, IDC_BACK))) goto LDone;
            fRaceBack = TRUE;
            iRacePage--;
            cWait = 0;
            return;
        }
        if (iRacePage < 5) {
            if (!FPushClick(GetDlgItem(hwndRace, IDC_NEXT))) goto LDone;
            iRacePage++;
            cWait = 0;
            return;
        }
        if (!FPushClick(GetDlgItem(hwndRace, IDCANCEL))) goto LDone;
        iStage = 4;
        cWait = 0;
        return;
    }
    if (iStage == 4) {
        if (IsWindow(hwndRace)) {
            if (++cWait < 20) return;
            goto LDone;
        }
        if (SDL_GetWindowFromID(idRaceWindow) || !IsWindowEnabled(hwndWizard) ||
            !FPushClick(GetDlgItem(hwndWizard, IDCANCEL))) goto LDone;
        iStage = 2;
        cWait = 0;
        return;
    }
    if (iStage == 2) {
        if (IsWindow(hwndWizard)) {
            if (++cWait < 20) return;
            goto LDone;
        }
        if (!IsWindowEnabled(hwndTitle)) goto LDone;
        KillTimer(NULL, id);
        fPassed = TRUE;
        event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
        event.window.windowID = lpTitle->id;
        if (!SDL_PushEvent(&event)) fPassed = FALSE;
        goto LDone;
    }
    fChecked = TRUE;
    UpdateWindow(hwndTitle);
    for (iButton = 0; iButton < 4; iButton++) {
        if (!IsWindow(rghwndBtnSplash[iButton]) || !IsWindowVisible(rghwndBtnSplash[iButton])) goto LDone;
        UpdateWindow(rghwndBtnSplash[iButton]);
    }
    rgb = lpTitle->lpCanvas->pixels;
    cb = (size_t)lpTitle->lpCanvas->pitch * lpTitle->lpCanvas->h;
    for (i = 4; i + 4 <= cb; i += 4) {
        if (memcmp(rgb, rgb + i, 4)) { fDifferent = TRUE; break; }
    }
    if (!fDifferent || !FCompatPresent(lpTitle) || !lpTitle->lpComposite ||
        (pszCapture && !SDL_SaveBMP(lpTitle->lpComposite, pszCapture))) goto LDone;
    printf("Stars title %dx%d, renderer %s; independent frame and title windows\n",
           lpTitle->lpCanvas->w, lpTitle->lpCanvas->h, SDL_GetRendererName(lpTitle->lpRenderer));
    if (!FPushClick(rghwndBtnSplash[pszContinue ? 2 : 0])) goto LDone;
    iStage = pszContinue ? 5 : 1;
    return;
LDone:
    if (!fPassed) {
        fprintf(stderr, "Client startup/wizard check failed at stage %d: %s\n", iStage, SDL_GetError());
        KillTimer(NULL, id);
        PostQuitMessage(92);
    }
}

int16_t StarsTest_InitInstance(int16_t nCmdShow) {
    int16_t fResult = StarsTestReal_InitInstance(nCmdShow);
    fInitComplete = TRUE;
    if (fResult) idTimer = SetTimer(NULL, 0, 250, CheckStartup);
    return fResult && idTimer ? fResult : FALSE;
}

int main(void) {
    char szCommand[] = "";
    int nResult;
    pszNewGame = getenv("STARS_CLIENT_NEW_GAME");
    pszContinue = getenv("STARS_CLIENT_CONTINUE");
    if (pszContinue && !WritePrivateProfileString("Files", "File1", pszContinue, "Stars.ini")) return 1;
    if (pszContinue && (!WritePrivateProfileString("Windows", "PlanetTiles", "ABE*FGH", "Stars.ini") ||
        !WritePrivateProfileString("Windows", "ShipTiles", "AFDE*BJI", "Stars.ini"))) return 1;
    setvbuf(stdout, NULL, _IONBF, 0);
    nResult = WinMain(GetModuleHandle(NULL), NULL, szCommand, SW_SHOWNORMAL);
    if (IsWindow(hwndTitle)) fPassed = FALSE;
    if (idTimer) KillTimer(NULL, idTimer);
    StarsCompatProcessExit();
    SDL_Quit();
    return nResult == 0 && fChecked && fPassed && fTitlePainted && cAlerts == (getenv("STARS_CLIENT_LIBRARY_FAIL_WRITE") ? 2 : 0) && cEarlyFrameShow == 0 ? 0 : 1;
}
