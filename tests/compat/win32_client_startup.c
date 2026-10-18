/* Exercise the shared WinMain, embedded resources and actual title painting. */
#include "win.h"
#include <stars_compat.h>
#include "internal.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>

char *StarsTest_SDL_GetPrefPath(const char *pszOrg, const char *pszApp) {
    const char *pszPath = getenv("STARS_TEST_PREF_PATH");
    if (pszPath) return SDL_strdup(pszPath);
    return SDL_GetPrefPath(pszOrg, pszApp);
}

static BOOL FTestCopyFile(const char *pszSource, const char *pszTarget) {
    FILE *pfSource, *pfTarget;
    unsigned char rgb[4096];
    size_t cb;
    BOOL fOkay;
    pfSource = fopen(pszSource, "rb");
    if (!pfSource) return FALSE;
    pfTarget = fopen(pszTarget, "wb");
    if (!pfTarget) { fclose(pfSource); return FALSE; }
    fOkay = TRUE;
    while ((cb = fread(rgb, 1, sizeof(rgb), pfSource)) != 0)
        if (fwrite(rgb, 1, cb, pfTarget) != cb) { fOkay = FALSE; break; }
    if (ferror(pfSource)) fOkay = FALSE;
    if (fclose(pfSource) != 0) fOkay = FALSE;
    if (fclose(pfTarget) != 0) fOkay = FALSE;
    return fOkay;
}

static BOOL FTestWriteFile(const char *pszFile, const char *pszText) {
    FILE *pf;
    size_t cbText = strlen(pszText);
    BOOL fOkay;
    pf = fopen(pszFile, "wb");
    if (!pf) return FALSE;
    fOkay = fwrite(pszText, 1, cbText, pf) == cbText;
    if (fclose(pf) != 0) fOkay = FALSE;
    return fOkay;
}

static BOOL FTestFilesEqual(const char *pszFile1, const char *pszFile2) {
    FILE *pfFile1, *pfFile2;
    unsigned char rgb1[4096], rgb2[4096];
    size_t cb1, cb2;
    BOOL fEqual;
    pfFile1 = fopen(pszFile1, "rb");
    if (!pfFile1) return FALSE;
    pfFile2 = fopen(pszFile2, "rb");
    if (!pfFile2) { fclose(pfFile1); return FALSE; }
    fEqual = TRUE;
    do {
        cb1 = fread(rgb1, 1, sizeof(rgb1), pfFile1);
        cb2 = fread(rgb2, 1, sizeof(rgb2), pfFile2);
        if (cb1 != cb2 || memcmp(rgb1, rgb2, cb1) != 0) { fEqual = FALSE; break; }
    } while (cb1 != 0);
    if (ferror(pfFile1) || ferror(pfFile2)) fEqual = FALSE;
    if (fclose(pfFile1) != 0) fEqual = FALSE;
    if (fclose(pfFile2) != 0) fEqual = FALSE;
    return fEqual;
}

static BOOL FTestReadLibraryMetadata(const char *pszFile, int *piVersion, char *pszName,
                                     size_t cchName, int *piplr, long long *plPlayed,
                                     char *pszExchangeName, size_t cchExchangeName) {
    FILE *pf;
    int cchNameRead, cchExchangeRead;
    BOOL fRead;
    pf = fopen(pszFile, "rb");
    if (!pf) return FALSE;
    fRead = fscanf(pf, "%d\n", piVersion) == 1 && fgets(pszName, (int)cchName, pf) &&
            fscanf(pf, "%d\n%lld\n", piplr, plPlayed) == 2 &&
            fgets(pszExchangeName, (int)cchExchangeName, pf) != NULL;
    fclose(pf);
    if (!fRead) return FALSE;
    pszName[strcspn(pszName, "\r\n")] = 0;
    pszExchangeName[strcspn(pszExchangeName, "\r\n")] = 0;
    cchNameRead = (int)strlen(pszName);
    cchExchangeRead = (int)strlen(pszExchangeName);
    return cchNameRead > 0 && (size_t)cchNameRead < cchName &&
           cchExchangeRead > 0 && (size_t)cchExchangeRead < cchExchangeName;
}

static BOOL FTestLibraryTimestamp(long long *plPlayed) {
    char szDir[512], szFile[512], szMetadataName[256];
    char *pch;
    FILE *pf;
    int iVersion, iPlayer;
    strcpy(szDir, szBase);
    pch = strrchr(szDir, chDirSep);
    if (!pch) return FALSE;
    *pch = 0;
    CchSnprintf(szFile, sizeof(szFile), "%s%s.library", szDir, szDirSep);
    pf = fopen(szFile, "rb");
    if (!pf) return FALSE;
    if (fscanf(pf, "%d\n", &iVersion) != 1 || !fgets(szMetadataName, sizeof(szMetadataName), pf) ||
        fscanf(pf, "%d\n%lld", &iPlayer, plPlayed) != 2) {
        fclose(pf);
        return FALSE;
    }
    fclose(pf);
    return iVersion == 2 && iPlayer == idPlayer;
}

int16_t StarsTestReal_InitInstance(int16_t nCmdShow);
static BOOL fChecked, fPassed;
static BOOL fSwitchCancelSent;
static int cSwitchLogWrites, cSwitchHistoryWrites;
static int cAlerts;
static BOOL fConflictRestoreAlert;
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
static const char *pszImportSource;
static int iImportStage;
static int iImportUpdateStage;
static int iRaceLibraryTestStage;
static int iHostLibraryTestStage;
static int cHostOrderConflictPrompts;
static int cHostOrderPickerCalls;
static BOOL fHostOrderPickerCancel;
static BOOL fHostSavePromptSeen;
static int16_t idHostOrderFleet;
static int16_t idHostOrderConflictResponse = IDYES;
static int iRosterMenu = -1;
static const char *pszRosterSource;
static int cRecentMigrationPrompts;
static int iRaceLibraryWizardPage;
static uint16_t icksumRaceLibrary;
static uint16_t icksRaceLibraryOriginal;
static uint16_t icksRaceSaveStage;
static uint16_t icksRaceImportStage;
static int cRaceDeleteR2;
static BOOL fFailRaceDelete;
static BOOL fLibraryGameDeletePrompt;
static BOOL fLibraryGameDeleteFailureAlert;
static BOOL fFailLibraryGameDelete;
static BOOL fFailRaceExportReplace;
static BOOL fFailRaceVersionSave, fRaceVersionSaveFailed;
static BOOL fExpectRaceExportFailureAlert;
static BOOL fFailGameRenamePublish;
static BOOL fExpectGameRenameFailure;
static char szGameRenameFailTarget[512];
static char szRaceExportFailTarget[512];
static GAME gameImportSav;
static int16_t idPlayerImportSav;
static char szBaseImportSav[256];
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
    if (iRosterMenu >= 0) {
        iPopMenuSel = iRosterMenu;
        iRosterMenu = -1;
        return TRUE;
    }
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
    strcpy(fresh.rgobj[0].szName, "Quoted \"\xc3\xa9\xe2\x82\xac\"\r\n");
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

static BOOL fRejectRacePassword;
static BOOL fTestLegacyRaceImport, fAcceptLegacyRacePassword;
static int cLegacyRacePasswords;

int16_t StarsTestReal_PromptPassword(void);
int16_t StarsTest_PromptPassword(void) {
    if (fRejectRacePassword) return FALSE;
    // The frozen regression race is password protected. These tests qualify
    // file loading and client creation; password validation is outside scope.
    return pszContinue || getenv("STARS_CLIENT_EXPECT_PLAYER") || getenv("STARS_CLIENT_HOST_LIBRARY_TEST") || getenv("STARS_CLIENT_STARTUP_MANAGED") ||
           getenv("STARS_CLIENT_EXPORT_GAME") ? TRUE : StarsTestReal_PromptPassword();
}

int16_t StarsTestReal_FPromptLegacyRacePassword(char *szPassword);
int16_t StarsTest_FPromptLegacyRacePassword(char *szPassword) {
    if (!fTestLegacyRaceImport) return StarsTestReal_FPromptLegacyRacePassword(szPassword);
    cLegacyRacePasswords++;
    if (!fAcceptLegacyRacePassword) return FALSE;
    strcpy(szPassword, "new-\xc3\xa9");
    return TRUE;
}

BOOL StarsTestReal_FNativeOpenFileName(OPENFILENAMEA *lpof);
BOOL StarsTest_FNativeOpenFileName(OPENFILENAMEA *lpof) {
    const char *pszSource = getenv("STARS_CLIENT_RACE_IMPORT_SOURCE");
    const char *pszExternalGame = getenv("STARS_CLIENT_EXTERNAL_OPEN_GAME");
    if (!pszExternalGame) pszExternalGame = getenv("STARS_CLIENT_EXTERNAL_OPEN_RACE");
    if (getenv("STARS_CLIENT_STARTUP_SOURCE")) {
        fPassed = FALSE;
        fprintf(stderr, "Startup unexpectedly reopened the file chooser\n");
        PostQuitMessage(92);
        return FALSE;
    }
    if (pszRosterSource) {
        if (strlen(pszRosterSource) >= lpof->nMaxFile) return FALSE;
        strcpy(lpof->lpstrFile, pszRosterSource);
        return TRUE;
    }
    if (pszExternalGame) {
        if (strlen(pszExternalGame) >= lpof->nMaxFile) return FALSE;
        strcpy(lpof->lpstrFile, pszExternalGame);
        return TRUE;
    }
    if (getenv("STARS_CLIENT_RACE_LIBRARY_TEST") && pszSource) {
        if (strlen(pszSource) >= lpof->nMaxFile) return FALSE;
        strcpy(lpof->lpstrFile, pszSource);
        return TRUE;
    }
    if (getenv("STARS_CLIENT_HOST_LIBRARY_TEST")) {
        const char *pszHostOrderPicker;
        cHostOrderPickerCalls++;
        if (fHostOrderPickerCancel) return FALSE;
        pszHostOrderPicker = getenv("STARS_CLIENT_HOST_ORDER_PICKER");
        if (pszHostOrderPicker) {
            size_t cchHostOrderPicker = strlen(pszHostOrderPicker);
            if (cchHostOrderPicker >= (size_t)lpof->nMaxFile) return FALSE;
            memcpy(lpof->lpstrFile, pszHostOrderPicker, cchHostOrderPicker + 1);
            return TRUE;
        }
        return StarsTestReal_FNativeOpenFileName(lpof);
    }
    return StarsTestReal_FNativeOpenFileName(lpof);
}

bool SDL_ShowWindow(SDL_Window *lpWindow);
bool SDLCALL StarsTest_SDL_ShowWindow(SDL_Window *lpWindow) {
    HWND hwnd;
    bool fResult;
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
    if (getenv("STARS_DEBUG_LIBRARY_STARTUP") && hwnd && hwnd->lpfnWndProc == TitleWndProc)
        fprintf(stderr, "[DEBUG-library-startup] SDL_ShowWindow title before visible=%d id=%u\n",
                IsWindowVisible(hwnd), SDL_GetWindowID(lpWindow));
    fResult = SDL_ShowWindow(lpWindow);
    if (getenv("STARS_DEBUG_LIBRARY_STARTUP") && hwnd && hwnd->lpfnWndProc == TitleWndProc)
        fprintf(stderr, "[DEBUG-library-startup] SDL_ShowWindow title result=%d visible=%d\n",
                fResult, IsWindowVisible(hwnd));
    return fResult;
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

static BOOL FSelectClientRaceName(HWND hwndTitle, const char *pszName) {
    HWND hwndList = GetDlgItem(hwndTitle, IDC_LIBRARY_RACES);
    char szName[256];
    LRESULT i, c;
    c = SendMessage(hwndList, LB_GETCOUNT, 0, 0);
    for (i = 0; i < c; i++) {
        if (SendMessage(hwndList, LB_GETTEXT, i, (LPARAM)szName) >= 0 && !strcmp(szName, pszName))
            return SendMessage(hwndList, LB_SETCURSEL, i, 0) != LB_ERR;
    }
    return FALSE;
}

static BOOL FSelectClientGameName(HWND hwndTitle, const char *pszName) {
    HWND hwndList = GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES);
    char szRow[320];
    size_t cchName = strlen(pszName);
    LRESULT i, c;
    c = SendMessage(hwndList, LB_GETCOUNT, 0, 0);
    for (i = 0; i < c; i++) {
        if (SendMessage(hwndList, LB_GETTEXT, i, (LPARAM)szRow) >= 0 &&
            !strncmp(szRow, pszName, cchName) && !strncmp(szRow + cchName, " - Player ", 10))
            return SendMessage(hwndList, LB_SETCURSEL, i, 0) != LB_ERR;
    }
    return FALSE;
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

#include "win32_raceroster_workflow.h"
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

INT_PTR StarsTestReal_AskSaveDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR StarsTest_AskSaveDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    const char *pszChoice = getenv("STARS_CLIENT_SWITCH_CHOICE");
    if (pszChoice && message == WM_INITDIALOG) {
        if (!strcmp(pszChoice, "save")) EndDialog(hwnd, 1);
        else if (!strcmp(pszChoice, "discard")) EndDialog(hwnd, 2);
        else if (!strcmp(pszChoice, "submit")) EndDialog(hwnd, -1);
        else if (!strcmp(pszChoice, "cancel")) EndDialog(hwnd, -2);
        else return 0;
        return 1;
    }
    if (getenv("STARS_CLIENT_SWITCH_CANCEL_TEST") && message == WM_INITDIALOG) {
        fSwitchCancelSent = TRUE;
        EndDialog(hwnd, -2);
        return 1;
    }
    INT_PTR fRet = StarsTestReal_AskSaveDialog(hwnd, message, wParam, lParam);
    if (message == WM_INITDIALOG && getenv("STARS_CLIENT_HOST_LIBRARY_TEST")) {
        HWND hwndSave = GetDlgItem(hwnd, IDC_SAVE);
        HWND hwndSubmit = GetDlgItem(hwnd, IDC_SAVESUBMIT);
        HWND hwndDontSave = GetDlgItem(hwnd, IDC_NO_DON_T_SAVE);
        if (hwndSave && hwndDontSave && (game.fSinglePlr ? !hwndSubmit : hwndSubmit != NULL)) {
            fHostSavePromptSeen = TRUE;
            PostMessage(hwnd, WM_COMMAND, IDC_NO_DON_T_SAVE, (LPARAM)hwndDontSave);
        }
    }
    return fRet;
}

int16_t StarsTest_AlertSz(char *sz, int16_t mbType) {
    if (fRejectRacePassword) return IDOK;
    if (getenv("STARS_CLIENT_RACE_VERSION_TEST") && strstr(sz, "was unable to save your race data file")) {
        fRaceVersionSaveFailed = TRUE;
        return IDOK;
    }
    if (getenv("STARS_CLIENT_GAME_DELETE_MODE") &&
        strstr(sz, "The selected managed game could not be completely deleted.")) {
        fLibraryGameDeleteFailureAlert = TRUE;
        fprintf(stderr, "Startup alert: %s\n", sz);
        cAlerts++;
        return IDOK;
    }
    if (getenv("STARS_CLIENT_HOST_LIBRARY_TEST") && (mbType & MB_YESNO)) {
        cHostOrderConflictPrompts++;
        return idHostOrderConflictResponse;
    }
    if (getenv("STARS_CLIENT_HOST_LIBRARY_TEST") &&
        strstr(sz, "Auto Generate is disabled because all of the human players are dead or inactive")) return IDOK;
    if (getenv("STARS_CLIENT_RACE_LIBRARY_TEST") && strstr(sz, "race design could not be deleted")) return IDOK;
    if (fExpectRaceExportFailureAlert && strstr(sz, "race design could not be exported")) return IDOK;
    if (getenv("STARS_CLIENT_EXPORT_EXPECTED") && strstr(sz, "game could not be exported")) {
        cAlerts++;
        return IDOK;
    }
    fprintf(stderr, "Startup alert: %s\n", sz);
    cAlerts++;
    if (getenv("STARS_CLIENT_HOST_LIBRARY_TEST")) return IDOK;
    if (getenv("STARS_CLIENT_LIBRARY_FAIL_WRITE") || getenv("STARS_CLIENT_SWITCH_FAIL_SAVE")) return IDOK;
    if (getenv("STARS_CLIENT_IMPORT_INVALID_EXPECTED")) {
        fChecked = TRUE;
        fPassed = TRUE;
        PostQuitMessage(0);
        return IDOK;
    }
    if (getenv("STARS_CLIENT_IMPORT_UPDATE_EXPECTED")) {
        if (getenv("STARS_CLIENT_IMPORT_UPDATE_FAIL_CONFLICT_RESTORE") &&
            strstr(sz, "the managed game and its orders were preserved")) fConflictRestoreAlert = TRUE;
        return IDOK;
    }
    PostQuitMessage(91);
    return IDOK;
}

int WINAPI StarsTestReal_MessageBoxA(HWND hwnd, LPCSTR pszText, LPCSTR pszCaption, UINT type);
int WINAPI StarsTest_MessageBoxA(HWND hwnd, LPCSTR pszText, LPCSTR pszCaption, UINT type) {
    (void)hwnd; (void)pszCaption;
    if (pszText && strstr(pszText, "Import games from your recent files")) {
        cRecentMigrationPrompts++;
        fPassed = FALSE;
        PostQuitMessage(92);
        return IDNO;
    }
    if (fExpectGameRenameFailure) { fExpectGameRenameFailure = FALSE; return IDOK; }
    if (pszText && strstr(pszText, "Delete the managed race design Managed.r2?") && (type & MB_YESNO))
        return cRaceDeleteR2++ == 0 ? IDNO : IDYES;
    if (pszText && strstr(pszText, "Delete the managed race design Managed.r3?") && (type & MB_YESNO)) {
        fFailRaceDelete = TRUE;
        return IDYES;
    }
    if (pszText && strstr(pszText, "Delete managed game selected-game?") && (type & MB_YESNO)) {
        fLibraryGameDeletePrompt = strstr(pszText, "all player files") && strstr(pszText, "host state") &&
                                   strstr(pszText, "history") && strstr(pszText, "backups") &&
                                   strstr(pszText, "Export the complete game first");
        return getenv("STARS_CLIENT_GAME_DELETE_MODE") &&
               strcmp(getenv("STARS_CLIENT_GAME_DELETE_MODE"), "cancel") == 0 ? IDNO : IDYES;
    }
    if (pszText && strstr(pszText, "The selected managed game could not be completely deleted."))
        fLibraryGameDeleteFailureAlert = TRUE;
    return StarsTestReal_MessageBoxA(hwnd, pszText, pszCaption, type);
}

int StarsTestReal_RemoveNativeFile(const char *pszFile);
int StarsTest_RemoveNativeFile(const char *pszFile) {
    static int fFailImportOrdersRemove;
    if (!fFailLibraryGameDelete && getenv("STARS_CLIENT_GAME_DELETE_MODE") &&
        strcmp(getenv("STARS_CLIENT_GAME_DELETE_MODE"), "failure") == 0 && strstr(pszFile, "selected-game")) {
        fFailLibraryGameDelete = TRUE;
        return -1;
    }
    if (fFailRaceDelete) {
        fFailRaceDelete = FALSE;
        return -1;
    }
    if (!fFailImportOrdersRemove && getenv("STARS_CLIENT_IMPORT_UPDATE_FAIL_ORDERS_REMOVE") &&
        strstr(pszFile, ".x1")) {
        fFailImportOrdersRemove = TRUE;
        return -1;
    }
    return StarsTestReal_RemoveNativeFile(pszFile);
}

int StarsTestReal_ReplaceNativeFile(const char *pszOld, const char *pszNew);
int StarsTest_ReplaceNativeFile(const char *pszOld, const char *pszNew) {
    static int fFailImportReplace;
    if (fFailRaceVersionSave && strstr(pszOld, ".stars-race-stage-")) {
        fFailRaceVersionSave = FALSE;
        return -1;
    }
    if (!fFailImportReplace && getenv("STARS_CLIENT_IMPORT_UPDATE_FAIL_REPLACE") && strstr(pszNew, ".m1")) {
        fFailImportReplace = TRUE;
        return -1;
    }
    if (getenv("STARS_CLIENT_IMPORT_UPDATE_FAIL_ROLLBACK") && strstr(pszOld, ".previous.") &&
        strstr(pszNew, ".m1")) return -1;
    if (fFailRaceExportReplace && !SDL_strcasecmp(pszNew, szRaceExportFailTarget)) {
        fFailRaceExportReplace = FALSE;
        return -1;
    }
    return StarsTestReal_ReplaceNativeFile(pszOld, pszNew);
}

int StarsTestReal_RenameNativeFile(const char *pszOld, const char *pszNew);
int StarsTest_RenameNativeFile(const char *pszOld, const char *pszNew) {
    static int fFailConflictPublish, fFailConflictRestore;
    if (!fFailConflictPublish && getenv("STARS_CLIENT_IMPORT_UPDATE_FAIL_CONFLICT_PUBLISH") &&
        strstr(pszOld, ".stars-game-stage-") && strstr(pszNew, "managed-copy-duplicate")) {
        fFailConflictPublish = TRUE;
        return -1;
    }
    if (!fFailConflictRestore && getenv("STARS_CLIENT_IMPORT_UPDATE_FAIL_CONFLICT_RESTORE") &&
        strstr(pszOld, ".stars-game-old-") && strstr(pszNew, "managed-copy-duplicate")) {
        fFailConflictRestore = TRUE;
        return -1;
    }
    if (fFailGameRenamePublish && !SDL_strcasecmp(pszNew, szGameRenameFailTarget)) {
        fFailGameRenamePublish = FALSE;
        return -1;
    }
    return StarsTestReal_RenameNativeFile(pszOld, pszNew);
}

#include "win32_raceversion_workflow.h"
#include "win32_newgame_workflow.h"
#include "win32_game_export_workflow.h"

static BOOL FCopyOrderWithByteChange(const char *pszSource, const char *pszTarget, long ibChange, BYTE bMask) {
    FILE *pfSource, *pfTarget;
    int ch;
    long ib;
    BOOL fChanged, fOkay;

    pfSource = fopen(pszSource, "rb");
    if (!pfSource) return FALSE;
    pfTarget = fopen(pszTarget, "wb");
    if (!pfTarget) { fclose(pfSource); return FALSE; }
    fChanged = FALSE;
    fOkay = TRUE;
    ib = 0;
    while ((ch = fgetc(pfSource)) != EOF) {
        if (ib == ibChange) { ch ^= bMask; fChanged = TRUE; }
        if (fputc(ch, pfTarget) == EOF) { fOkay = FALSE; break; }
        ib++;
    }
    if (ferror(pfSource)) fOkay = FALSE;
    if (fclose(pfSource) != 0) fOkay = FALSE;
    if (fclose(pfTarget) != 0) fOkay = FALSE;
    return fOkay && fChanged;
}

/* Cancellation must never bind the previous race to a different save path. */
static BOOL FTestRaceCancellation(void) {
    PLAYER plrSav;
    char szProtected[512], szSnapshot[512], szExport[512], szImport[512], szDuplicate[512];
    const char *pszSettings = getenv("STARS_SETTINGS_DIR");
    int16_t fOkay;

    if (!FLoadLibraryRace(0, "Managed.r1")) return FALSE;
    vplr.lSalt = LSaltFromSz("cancel-test");
    if (!FSaveLibraryRace(0, "Protected.r1", &vplr) || !FLoadLibraryRace(0, "Managed.r1")) return FALSE;
    plrSav = vplr;
    CchSnprintf(szProtected, sizeof(szProtected), "%s%sRace Designs%sProtected.r1", pszSettings, szDirSep, szDirSep);
    CchSnprintf(szSnapshot, sizeof(szSnapshot), "%s.snapshot", szProtected);
    CchSnprintf(szExport, sizeof(szExport), "%s.export.r1", szProtected);
    CchSnprintf(szImport, sizeof(szImport), "%s%sRace Designs%sCancelled.r1", pszSettings, szDirSep, szDirSep);
    CchSnprintf(szDuplicate, sizeof(szDuplicate), "%s%sRace Designs%sProtected.r2", pszSettings, szDirSep, szDirSep);
    if (!FTestCopyFile(szProtected, szSnapshot)) return FALSE;
    fRejectRacePassword = TRUE;
    lSaltLast = -5;
    fOkay = !FLoadLibraryRace(0, "Protected.r1") && !memcmp(&vplr, &plrSav, sizeof(vplr)) &&
            !strcmp(PszCurrentLibraryRaceName(), "Managed.r1");
    lSaltLast = -5;
    fOkay = fOkay && !FEditLibraryRace(0, "Protected.r1");
    lSaltLast = -5;
    fOkay = fOkay && !FDuplicateLibraryRace(0, "Protected.r1");
    lSaltLast = -5;
    fOkay = fOkay && !FExportLibraryRace(0, "Protected.r1", szExport);
    lSaltLast = -5;
    fOkay = fOkay && !FImportLibraryRaceAs(0, szProtected, "Cancelled.r1");
    fRejectRacePassword = FALSE;
    fOkay = fOkay && !memcmp(&vplr, &plrSav, sizeof(vplr)) &&
            !strcmp(PszCurrentLibraryRaceName(), "Managed.r1") && FTestFilesEqual(szProtected, szSnapshot) &&
            !FFileExists(szExport) && !FFileExists(szImport) && !FFileExists(szDuplicate);
    RemoveNativeFile(szProtected);
    RemoveNativeFile(szSnapshot);
    return fOkay;
}

/* Inspect the managed bytes without completing the optional race wizard. */
static BOOL FTestLegacyRaceImport(void) {
    RTBOF rtbof = {0};
    PLAYER plr, plrSav;
    char szSource[512], szSnapshot[512], szImported[512], szCancelled[512];
    const char *pszSettings = getenv("STARS_SETTINGS_DIR");
    const char *rgszName[] = {"Legacy.r1", "Legacy-protected.r1"};
    uint16_t icksum;
    int i;
    BOOL fOkay = FALSE;

    if (!FLoadLibraryRace(0, "Managed.r1")) return FALSE;
    plrSav = vplr;
    CchSnprintf(szSource, sizeof(szSource), "%s%slegacy-source.r1", pszSettings, szDirSep);
    CchSnprintf(szSnapshot, sizeof(szSnapshot), "%s.snapshot", szSource);
    CchSnprintf(szCancelled, sizeof(szCancelled), "%s%sRace Designs%sLegacy-cancelled.r1", pszSettings, szDirSep, szDirSep);
    fTestLegacyRaceImport = TRUE;
    fAcceptLegacyRacePassword = TRUE;
    for (i = 0; i < 2; i++) {
        plr = plrSav;
        strcpy(plr.szName, "R\xe9");
        strcpy(plr.szNames, "R\xe9s");
        plr.lSalt = i == 0 ? 0 : LSaltFromSz("old-password");
        memcpy(rtbof.rgid, "J3J3", 4);
        rtbof.verMajor = 2;
        rtbof.verMinor = 84;
        rtbof.dt = dtRace;
        StreamOpen(szSource, mdCreate);
        WriteRt(rtBOF, sizeof(rtbof), &rtbof);
        WriteRtPlr(&plr, NULL);
        icksum = IRaceChecksum(&plr);
        WriteRt(rtEOF, sizeof(icksum), &icksum);
        StreamClose();
        if (!FTestCopyFile(szSource, szSnapshot)) goto LDone;
        cLegacyRacePasswords = 0;
        if (!FImportLibraryRaceAs(0, szSource, rgszName[i]) || cLegacyRacePasswords != i ||
            !FTestFilesEqual(szSource, szSnapshot)) goto LDone;
        CchSnprintf(szImported, sizeof(szImported), "%s%sRace Designs%s%s", pszSettings, szDirSep, szDirSep, rgszName[i]);
        if (FWasRaceFile(szImported, FALSE) != 1 || ((VERS *)&wVersFile)->verMinor != 85 ||
            strcmp(vplr.szName, "R\xc3\xa9") || strcmp(vplr.szNames, "R\xc3\xa9s") ||
            vplr.lSalt != (i == 0 ? 0 : LSaltFromSz("new-\xc3\xa9")) || cLegacyRacePasswords != i) goto LDone;
    }
    fAcceptLegacyRacePassword = FALSE;
    plrSav = vplr;
    if (FImportLibraryRaceAs(0, szSource, "Legacy-cancelled.r1") ||
        FFileExists(szCancelled) || memcmp(&vplr, &plrSav, sizeof(vplr)) ||
        !FTestFilesEqual(szSource, szSnapshot)) goto LDone;
    fOkay = TRUE;
LDone:
    fTestLegacyRaceImport = FALSE;
    RemoveNativeFile(szSource);
    RemoveNativeFile(szSnapshot);
    return fOkay;
}

static void CALLBACK CheckStartup(HWND hwnd, UINT msg, UINT_PTR id, DWORD dwTime) {
    HWND lpFrame, lpTitle;
    char szRaceName[256], szDuplicate[512], szExport[512], szManaged[512], szExportSnapshot[512];
    const char *pszSettings = getenv("STARS_SETTINGS_DIR");
    const char *pszRaceSource = getenv("STARS_CLIENT_RACE_IMPORT_SOURCE");
    const char *pszRaceSnapshot = getenv("STARS_CLIENT_RACE_IMPORT_SNAPSHOT");
    const char *pszCapture = getenv("STARS_CLIENT_CAPTURE");
    const char *pszWizardCapture = getenv("STARS_CLIENT_WIZARD_CAPTURE");
    const char *pszRaceCapture = getenv("STARS_CLIENT_RACE_CAPTURE");
    const char *pszGameCapture = getenv("STARS_CLIENT_GAME_CAPTURE");
    const char *pszExportGame = getenv("STARS_CLIENT_EXPORT_GAME");
    static const DLGPROC rglpfnRace[] = {RaceWizardDlg1, RaceWizardDlg4, RaceWizardDlg5, RaceWizardDlg2, RaceWizardDlg3, RaceWizardDlg6};
    BYTE *rgb;
    size_t cb, i;
    int iButton;
    int16_t fImported;
    int iTarget, c;
    HWND hwndList, hwndListTarget;
    char szFind[320], szName[256];
    const char *pszExpected, *pszTarget, *pszYear, *pszCopyName;
    const char *pszHostSource, *pszHostPlayerSource, *pszHostOrderBase, *pszHostPickerBase;
    const char *pszDeleteMode;
    SDL_Event event = {0};
    BOOL fDifferent = FALSE;
    RECT rcFrame;
    RECT rcScanner;
    HWND hwndDeleteList;
    LRESULT iDeleteSelected, iDeleteOther;
    POINT ptLibraryButton;
    static const int rgidLibraryButton[] = {
        IDC_LIBRARY_IMPORT, IDC_LIBRARY_IMPORT_FOLDER, IDC_LIBRARY_GAME_EXPORT,
        IDC_LIBRARY_DUPLICATE, IDC_LIBRARY_RENAME
    };
    char szHostSourceHost[512], szHostSourceUniverse[512], szHostManagedHost[512], szHostManagedUniverse[512];
    char szHostManagedBase[512], szHostOrderFile[512], szHostManagedOrders[512];
    char szHostOrderIdentity[512], szHostOrderPlayer[512], szHostOrderTurn[512], szHostOrderAbsentPlayer[512];
    char szHostOrderGeneration[512], szHostOrderDifferent[512], szHostPickerOrder[512], szSavedBase[sizeof(szBase)];
    char szFleetNameSav[64];
    int cchHostOrder;
    int64_t cbHostFile;
    HWND hwndHostDialog;
    char *pszHostFolder;
    FLEET *lpflHostOrder;
    int iFleetHostOrder;
    BOOL fHadFleetName;

    (void)hwnd; (void)msg; (void)dwTime;
    if (getenv("STARS_CLIENT_HOST_LIBRARY_TEST")) {
        pszHostSource = getenv("STARS_CLIENT_HOST_IMPORT_SOURCE");
        pszHostPlayerSource = getenv("STARS_CLIENT_HOST_PLAYER_IMPORT_SOURCE");
        pszHostOrderBase = getenv("STARS_CLIENT_HOST_ORDER_BASE");
        pszHostPickerBase = getenv("STARS_CLIENT_HOST_ORDER_PICKER_BASE");
        cchHostOrder = CchSnprintf(szHostManagedBase, sizeof(szHostManagedBase),
                                   "%s%sGames%sHosted%sHosted", pszSettings, szDirSep, szDirSep, szDirSep);
        if (cchHostOrder <= 0 || (size_t)cchHostOrder >= sizeof(szHostManagedBase)) goto LDone;
        if (!pszHostOrderBase || !pszHostPickerBase ||
            CchSnprintf(szHostOrderFile, sizeof(szHostOrderFile), "%s.x1", pszHostOrderBase) <= 0 ||
            CchSnprintf(szHostOrderDifferent, sizeof(szHostOrderDifferent), "%s-different.x1", pszHostOrderBase) <= 0 ||
            CchSnprintf(szHostPickerOrder, sizeof(szHostPickerOrder), "%s.x1", pszHostPickerBase) <= 0 ||
            CchSnprintf(szHostManagedOrders, sizeof(szHostManagedOrders), "%s.x1", szHostManagedBase) <= 0) goto LDone;
        if (iHostLibraryTestStage == 0) {
            if (!IsWindowVisible(hwndTitle) || !GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES) ||
                !GetDlgItem(hwndTitle, IDC_LIBRARY_HOST_GAMES) ||
                !GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES_VIEW) ||
                !GetDlgItem(hwndTitle, IDC_LIBRARY_HOST_VIEW) ||
                !GetDlgItem(hwndTitle, IDC_LIBRARY_IMPORT_ORDERS) ||
                (GetWindowLongPtr(GetDlgItem(hwndTitle, IDC_LIBRARY_HOST_GAMES), GWL_STYLE) & WS_VISIBLE) ||
                (GetWindowLongPtr(GetDlgItem(hwndTitle, IDC_LIBRARY_IMPORT_ORDERS), GWL_STYLE) & WS_VISIBLE) ||
                !pszHostSource) goto LDone;
            if (!FPushClick(GetDlgItem(hwndTitle, IDC_LIBRARY_HOST_VIEW))) goto LDone;
            iHostLibraryTestStage = 20;
            return;
        }
        if (iHostLibraryTestStage == 20) {
            pszHostFolder = SDL_strdup(pszHostSource);
            if (!(GetWindowLongPtr(GetDlgItem(hwndTitle, IDC_LIBRARY_HOST_GAMES), GWL_STYLE) & WS_VISIBLE) ||
                (GetWindowLongPtr(GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES), GWL_STYLE) & WS_VISIBLE) ||
                !(GetWindowLongPtr(GetDlgItem(hwndTitle, IDC_LIBRARY_IMPORT_ORDERS), GWL_STYLE) & WS_VISIBLE) ||
                !pszHostFolder || !StarsCompatPostMessageAsync(hwndTitle, WM_STARS_IMPORT_FOLDER, (WPARAM)pszHostFolder, 0)) {
                SDL_free(pszHostFolder);
                goto LDone;
            }
            iHostLibraryTestStage = 21;
            return;
        }
        if (iHostLibraryTestStage == 21) {
            for (hwndHostDialog = GetWindow(hwndTitle, GW_HWNDFIRST); hwndHostDialog;
                 hwndHostDialog = GetWindow(hwndHostDialog, GW_HWNDNEXT)) {
                if (!hwndHostDialog->fDialog || hwndHostDialog->lpfnDlg != LibraryNameDlg) continue;
                if (!SetWindowTextA(GetDlgItem(hwndHostDialog, IDC_EDIT1), "Hosted") ||
                    !FPushClick(GetDlgItem(hwndHostDialog, IDOK))) goto LDone;
                iHostLibraryTestStage = 22;
                return;
            }
            return;
        }
        if (iHostLibraryTestStage == 22) {
            RefreshLibraryGames(hwndTitle);
            RefreshLibraryHostGames(hwndTitle);
            if (SendMessage(GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES), LB_GETCOUNT, 0, 0) != 0 ||
                SendMessage(GetDlgItem(hwndTitle, IDC_LIBRARY_HOST_GAMES), LB_GETCOUNT, 0, 0) != 1 ||
                !(GetWindowLongPtr(GetDlgItem(hwndTitle, IDC_LIBRARY_HOST_GAMES), GWL_STYLE) & WS_VISIBLE) ||
                CchSnprintf(szHostSourceHost, sizeof(szHostSourceHost), "%s%sgame.hst", pszHostSource, szDirSep) <= 0 ||
                CchSnprintf(szHostSourceUniverse, sizeof(szHostSourceUniverse), "%s%sgame.xy", pszHostSource, szDirSep) <= 0 ||
                CchSnprintf(szHostManagedHost, sizeof(szHostManagedHost), "%s%sGames%sHosted%sHosted.hst", pszSettings, szDirSep, szDirSep, szDirSep) <= 0 ||
                CchSnprintf(szHostManagedUniverse, sizeof(szHostManagedUniverse), "%s%sGames%sHosted%sHosted.xy", pszSettings, szDirSep, szDirSep, szDirSep) <= 0 ||
                !FTestFilesEqual(szHostSourceHost, szHostManagedHost) ||
                !FTestFilesEqual(szHostSourceUniverse, szHostManagedUniverse) ||
                !pszHostPlayerSource) goto LDone;
            if (!IsWindowEnabled(GetDlgItem(hwndTitle, IDC_LIBRARY_GAME_EXPORT)) ||
                !IsWindowEnabled(GetDlgItem(hwndTitle, IDC_LIBRARY_DUPLICATE)) ||
                !IsWindowEnabled(GetDlgItem(hwndTitle, IDC_LIBRARY_RENAME))) {
                fprintf(stderr, "Host-only game actions are disabled\n");
                goto LDone;
            }
            pszHostFolder = SDL_strdup(pszHostPlayerSource);
            if (!pszHostFolder || !StarsCompatPostMessageAsync(hwndTitle, WM_STARS_IMPORT_FOLDER,
                                                                (WPARAM)pszHostFolder, 0)) {
                SDL_free(pszHostFolder);
                goto LDone;
            }
            iHostLibraryTestStage = 25;
            return;
        }
        if (iHostLibraryTestStage == 25) {
            for (hwndHostDialog = GetWindow(hwndTitle, GW_HWNDFIRST); hwndHostDialog;
                 hwndHostDialog = GetWindow(hwndHostDialog, GW_HWNDNEXT)) {
                if (!hwndHostDialog->fDialog || hwndHostDialog->lpfnDlg != LibraryNameDlg) continue;
                if (!SetWindowTextA(GetDlgItem(hwndHostDialog, IDC_EDIT1), "PlayerCopy") ||
                    !FPushClick(GetDlgItem(hwndHostDialog, IDOK))) goto LDone;
                iHostLibraryTestStage = 26;
                return;
            }
            return;
        }
        if (iHostLibraryTestStage == 26) {
            RefreshLibraryGames(hwndTitle);
            RefreshLibraryHostGames(hwndTitle);
            if (SendMessage(GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES), LB_GETCOUNT, 0, 0) != 1 ||
                SendMessage(GetDlgItem(hwndTitle, IDC_LIBRARY_HOST_GAMES), LB_GETCOUNT, 0, 0) != 2 ||
                !(GetWindowLongPtr(GetDlgItem(hwndTitle, IDC_LIBRARY_HOST_GAMES), GWL_STYLE) & WS_VISIBLE)) goto LDone;
            hwndList = GetDlgItem(hwndTitle, IDC_LIBRARY_HOST_GAMES);
            if (SendMessage(hwndList, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)"Hosted") == LB_ERR ||
                !FExportSelectedLibraryGame(hwndTitle, pszSettings) ||
                CchSnprintf(szExport, sizeof(szExport), "%s%sHosted%sHosted.hst", pszSettings, szDirSep, szDirSep) <= 0 ||
                CchSnprintf(szHostSourceHost, sizeof(szHostSourceHost), "%s%sgame.hst", pszHostSource, szDirSep) <= 0 ||
                !FTestFilesEqual(szExport, szHostSourceHost)) {
                fprintf(stderr, "Host export did not use the visible selection\n");
                goto LDone;
            }
            iHostLibraryTestStage = 31;
            if (!FPushClick(GetDlgItem(hwndTitle, IDC_LIBRARY_DUPLICATE))) goto LDone;
            return;
        }
        if (iHostLibraryTestStage == 31 || iHostLibraryTestStage == 33) {
            for (hwndHostDialog = GetWindow(hwndTitle, GW_HWNDFIRST); hwndHostDialog;
                 hwndHostDialog = GetWindow(hwndHostDialog, GW_HWNDNEXT)) {
                if (!hwndHostDialog->fDialog || hwndHostDialog->lpfnDlg != LibraryNameDlg) continue;
                if (!SetWindowTextA(GetDlgItem(hwndHostDialog, IDC_EDIT1),
                                    iHostLibraryTestStage == 31 ? "HostCopy" : "HostRenamed") ||
                    !FPushClick(GetDlgItem(hwndHostDialog, IDOK))) goto LDone;
                iHostLibraryTestStage++;
                return;
            }
            return;
        }
        if (iHostLibraryTestStage == 32) {
            hwndList = GetDlgItem(hwndTitle, IDC_LIBRARY_HOST_GAMES);
            if (SendMessage(hwndList, LB_GETCOUNT, 0, 0) != 3 ||
                SendMessage(hwndList, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)"HostCopy") == LB_ERR) goto LDone;
            iHostLibraryTestStage = 33;
            if (!FPushClick(GetDlgItem(hwndTitle, IDC_LIBRARY_RENAME))) goto LDone;
            return;
        }
        if (iHostLibraryTestStage == 34) {
            hwndList = GetDlgItem(hwndTitle, IDC_LIBRARY_HOST_GAMES);
            if (SendMessage(hwndList, LB_GETCOUNT, 0, 0) != 3 ||
                SendMessage(hwndList, LB_FINDSTRING, (WPARAM)-1, (LPARAM)"HostCopy") != LB_ERR ||
                SendMessage(hwndList, LB_FINDSTRING, (WPARAM)-1, (LPARAM)"HostRenamed") == LB_ERR ||
                SendMessage(GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES), LB_GETCOUNT, 0, 0) != 1) goto LDone;
            if (!FPushClick(GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES_VIEW))) goto LDone;
            iHostLibraryTestStage = 23;
            return;
        }
        if (iHostLibraryTestStage == 23) {
            hwndList = GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES);
            if (!(GetWindowLongPtr(hwndList, GWL_STYLE) & WS_VISIBLE) ||
                SendMessage(hwndList, LB_SELECTSTRING, (WPARAM)-1,
                            (LPARAM)"PlayerCopy - Player 1") == LB_ERR) goto LDone;
            for (iButton = 0; iButton < (int)(sizeof(rgidLibraryButton) / sizeof(rgidLibraryButton[0])); iButton++) {
                hwndListTarget = GetDlgItem(hwndTitle, rgidLibraryButton[iButton]);
                if (!GetWindowRect(hwndListTarget, &rcFrame)) goto LDone;
                ptLibraryButton = (POINT){(rcFrame.left + rcFrame.right) / 2, (rcFrame.top + rcFrame.bottom) / 2};
                if (WindowFromPoint(ptLibraryButton) != hwndListTarget) {
                    fprintf(stderr, "Library button %d is covered by another control\n", rgidLibraryButton[iButton]);
                    goto LDone;
                }
            }
            iHostLibraryTestStage = 27;
            fFreeingTitle = FALSE;
            if (!FPushClick(rghwndBtnSplash[1])) goto LDone;
            return;
        }
        if (iHostLibraryTestStage == 27) {
            if (IsWindow(hwndTitle)) return;
            if (!lpPlanets || idPlayer == iplrNone) goto LDone;
            if (!hwndReportDlg) {
                SendMessage(hwndFrame, WM_COMMAND, IDM_REPORT_FLEET, 0);
                return;
            }
            if (vprptCur != &vrptFleet || vprptCur->cRows == 0) goto LDone;
            if (sel.grobj != grobjFleet) {
                if (!StarsCompatPostMessageAsync(hwndReportDlg, WM_LBUTTONDOWN, MK_LBUTTON,
                                                 MAKELPARAM(4, 20))) goto LDone;
                return;
            }
            iHostLibraryTestStage = 28;
            if (!StarsCompatPostMessageAsync(hwndFrame, WM_CHAR, 'r', 0)) goto LDone;
            return;
        }
        if (iHostLibraryTestStage == 28) {
            for (hwndHostDialog = GetWindow(hwndFrame, GW_HWNDFIRST); hwndHostDialog;
                 hwndHostDialog = GetWindow(hwndHostDialog, GW_HWNDNEXT)) {
                if (!hwndHostDialog->fDialog || hwndHostDialog->lpfnDlg != RenameDlg) continue;
                if (!SetWindowTextA(GetDlgItem(hwndHostDialog, IDC_EDIT1), "Prompt save order") ||
                    !FPushClick(GetDlgItem(hwndHostDialog, IDOK))) goto LDone;
                iHostLibraryTestStage = 29;
                return;
            }
            return;
        }
        if (iHostLibraryTestStage == 29) {
            if (hwndReportDlg) SendMessage(hwndReportDlg, WM_CLOSE, 0, 0);
            iHostLibraryTestStage = 30;
            SendMessage(hwndFrame, WM_COMMAND, IDM_FILE_RETURN_TO_TITLE, 0);
            return;
        }
        if (iHostLibraryTestStage == 30) {
            if (!fHostSavePromptSeen || !IsWindowVisible(hwndTitle)) return;
            if (!FPushClick(GetDlgItem(hwndTitle, IDC_LIBRARY_HOST_VIEW))) goto LDone;
            iHostLibraryTestStage = 24;
            return;
        }
        if (iHostLibraryTestStage == 24) {
            if (!(GetWindowLongPtr(GetDlgItem(hwndTitle, IDC_LIBRARY_HOST_GAMES), GWL_STYLE) & WS_VISIBLE) ||
                !(GetWindowLongPtr(GetDlgItem(hwndTitle, IDC_LIBRARY_IMPORT_ORDERS), GWL_STYLE) & WS_VISIBLE)) goto LDone;
            hwndList = GetDlgItem(hwndTitle, IDC_LIBRARY_HOST_GAMES);
            if (SendMessage(hwndList, LB_SELECTSTRING, (WPARAM)-1, (LPARAM)"Hosted") == LB_ERR) goto LDone;
            iHostLibraryTestStage = 7;
            fFreeingTitle = TRUE;
            if (!StarsCompatPostMessageAsync(hwndTitle, WM_COMMAND,
                                             MAKEWPARAM(IDC_LIBRARY_HOST_GAMES, LBN_DBLCLK),
                                             (LPARAM)hwndList)) goto LDone;
            return;
        }
        if (iHostLibraryTestStage == 7 || iHostLibraryTestStage == 8 || iHostLibraryTestStage == 9 ||
            iHostLibraryTestStage == 14) {
            for (hwndHostDialog = GetWindow(hwndFrame, GW_HWNDFIRST); hwndHostDialog;
                 hwndHostDialog = GetWindow(hwndHostDialog, GW_HWNDNEXT)) {
                if (!hwndHostDialog->fDialog || hwndHostDialog->lpfnDlg != HostModeDialog) continue;
                if (iHostLibraryTestStage == 7) {
                    if (
                        (cchHostOrder = CchSnprintf(szSavedBase, sizeof(szSavedBase), "%s", szBase)) <= 0 ||
                        (size_t)cchHostOrder >= sizeof(szSavedBase) ||
                        (cchHostOrder = CchSnprintf(szBase, sizeof(szBase), "%s", pszHostOrderBase)) <= 0 ||
                        (size_t)cchHostOrder >= sizeof(szBase)) { fprintf(stderr, "Host order paths invalid\n"); goto LDone; }
                    if (!FWriteLogFile(szBase, 0)) { fprintf(stderr, "Host FWriteLogFile failed: %s\n", SDL_GetError()); goto LDone; }
                    strcpy(szBase, szSavedBase);
                    if (CchSnprintf(szHostOrderFile, sizeof(szHostOrderFile), "%s.x1", pszHostOrderBase) <= 0 ||
                        CchSnprintf(szHostManagedOrders, sizeof(szHostManagedOrders), "%s.x1", szHostManagedBase) <= 0 ||
                        CchSnprintf(szHostOrderIdentity, sizeof(szHostOrderIdentity), "%s.identity.x1", pszHostOrderBase) <= 0 ||
                        CchSnprintf(szHostOrderPlayer, sizeof(szHostOrderPlayer), "%s.player.x1", pszHostOrderBase) <= 0 ||
                        CchSnprintf(szHostOrderTurn, sizeof(szHostOrderTurn), "%s.turn.x1", pszHostOrderBase) <= 0 ||
                        CchSnprintf(szHostOrderAbsentPlayer, sizeof(szHostOrderAbsentPlayer), "%s.absent.x16", pszHostOrderBase) <= 0 ||
                        CchSnprintf(szHostOrderGeneration, sizeof(szHostOrderGeneration), "%s.generation.x1", pszHostOrderBase) <= 0 ||
                        !FCopyOrderWithByteChange(szHostOrderFile, szHostOrderIdentity, 6, 1) ||
                        !FCopyOrderWithByteChange(szHostOrderFile, szHostOrderPlayer, 14, 1) ||
                        !FCopyOrderWithByteChange(szHostOrderFile, szHostOrderAbsentPlayer, 14, 15) ||
                        !FCopyOrderWithByteChange(szHostOrderFile, szHostOrderTurn, 12, 1) ||
                        !FCopyOrderWithByteChange(szHostOrderFile, szHostOrderGeneration, 17, 0x20)) { fprintf(stderr, "Host order variants failed: %s\n", SDL_GetError()); goto LDone; }
                    for (iFleetHostOrder = 0; iFleetHostOrder < cFleet; iFleetHostOrder++) {
                        lpflHostOrder = rglpfl[iFleetHostOrder];
                        if (((uint16_t)lpflHostOrder->id >> 9 & 0xf) == 0) break;
                    }
                    if (iFleetHostOrder == cFleet) { fprintf(stderr, "Host order fixture has no player fleet\n"); goto LDone; }
                    idHostOrderFleet = lpflHostOrder->id;
                    fHadFleetName = lpflHostOrder->lpszName != NULL;
                    if (fHadFleetName) CchSnprintf(szFleetNameSav, sizeof(szFleetNameSav), "%s", lpflHostOrder->lpszName);
                    LogChangeName(grobjFleet, lpflHostOrder->id, "Replacement order");
                    if (lpflHostOrder->lpszName) FreeLp(lpflHostOrder->lpszName, htString);
                    lpflHostOrder->lpszName = NULL;
                    if (fHadFleetName) {
                        lpflHostOrder->lpszName = LpAlloc(strlen(szFleetNameSav) + 1, htString);
                        if (!lpflHostOrder->lpszName) goto LDone;
                        strcpy(lpflHostOrder->lpszName, szFleetNameSav);
                    }
                    if (CchSnprintf(szHostOrderDifferent, sizeof(szHostOrderDifferent), "%s-different.x1", pszHostOrderBase) <= 0 ||
                        CchSnprintf(szBase, sizeof(szBase), "%s-different", pszHostOrderBase) <= 0 ||
                        !FWriteLogFile(szBase, 0)) { fprintf(stderr, "Host changed order write failed: %s\n", SDL_GetError()); goto LDone; }
                    strcpy(szBase, szSavedBase);
                    if (FImportLibraryOrders(hwndTitle, szHostOrderIdentity)) { fprintf(stderr, "Host order identity mismatch accepted\n"); goto LDone; }
                    if (FImportLibraryOrders(hwndTitle, szHostOrderPlayer)) { fprintf(stderr, "Host order player mismatch accepted\n"); goto LDone; }
                    if (FImportLibraryOrders(hwndTitle, szHostOrderTurn)) { fprintf(stderr, "Host order turn mismatch accepted\n"); goto LDone; }
                    if (FImportLibraryOrders(hwndTitle, szHostOrderGeneration)) { fprintf(stderr, "Host order generation mismatch accepted\n"); goto LDone; }
                    if (FImportLibraryOrders(hwndTitle, szHostOrderAbsentPlayer)) { fprintf(stderr, "Host order for nonexistent player accepted\n"); goto LDone; }
                    if (!FImportLibraryOrders(hwndTitle, szHostOrderFile)) { fprintf(stderr, "Host order valid file rejected\n"); goto LDone; }
                    if (!FTestFilesEqual(szHostOrderFile, szHostManagedOrders)) {
                        fprintf(stderr, "Host order target differs after import: target=%s\n", szHostManagedOrders);
                        if (FNativeFileSize(szHostManagedOrders, &cbHostFile)) fprintf(stderr, "target size=%lld\n", (long long)cbHostFile);
                        else fprintf(stderr, "target missing\n");
                        goto LDone;
                    }
                    if (!FImportLibraryOrders(hwndTitle, szHostOrderFile)) { fprintf(stderr, "Host order identical file rejected\n"); goto LDone; }
                    if (cHostOrderConflictPrompts != 0) { fprintf(stderr, "Host order no-op prompted\n"); goto LDone; }
                    idHostOrderConflictResponse = IDNO;
                    if (FImportLibraryOrders(hwndTitle, szHostOrderDifferent) || cHostOrderConflictPrompts != 1 ||
                        !FTestFilesEqual(szHostOrderFile, szHostManagedOrders)) goto LDone;
                    idHostOrderConflictResponse = IDYES;
                    if (!FImportLibraryOrders(hwndTitle, szHostOrderDifferent) || cHostOrderConflictPrompts != 2 ||
                        !FTestFilesEqual(szHostOrderDifferent, szHostManagedOrders)) goto LDone;
                    iHostLibraryTestStage = 8;
                    if (!FPushClick(GetDlgItem(hwndHostDialog, IDC_HOST_GENERATE_NOW))) goto LDone;
                    return;
                }
                if (iHostLibraryTestStage == 8) {
                    CchSnprintf(szHostOrderFile, sizeof(szHostOrderFile), "%s.m1", szHostManagedBase);
                    if (!FNativeFileSize(szHostOrderFile, &cbHostFile)) goto LDone;
                    CchSnprintf(szHostOrderFile, sizeof(szHostOrderFile), "%s.x1", szHostManagedBase);
                    lpflHostOrder = LpflFromId(idHostOrderFleet);
                    if (FNativeFileSize(szHostOrderFile, &cbHostFile) || !lpflHostOrder ||
                        !lpflHostOrder->lpszName || strcmp(lpflHostOrder->lpszName, "Replacement order")) goto LDone;
                    if ((cchHostOrder = CchSnprintf(szSavedBase, sizeof(szSavedBase), "%s", szBase)) <= 0 ||
                        (size_t)cchHostOrder >= sizeof(szSavedBase) ||
                        (cchHostOrder = CchSnprintf(szBase, sizeof(szBase), "%s", pszHostPickerBase)) <= 0 ||
                        (size_t)cchHostOrder >= sizeof(szBase) || !FWriteLogFile(szBase, 0)) goto LDone;
                    strcpy(szBase, szSavedBase);
                    iHostLibraryTestStage = 9;
                    gd.fClose = FALSE;
                    if (!FPushClick(GetDlgItem(hwndHostDialog, IDCANCEL))) goto LDone;
                    return;
                }
                return;
            }
            if (iHostLibraryTestStage == 9) iHostLibraryTestStage = 10;
            else return;
        }
        if (iHostLibraryTestStage == 10) {
            if (!IsWindowVisible(hwndTitle) ||
                !(GetWindowLongPtr(GetDlgItem(hwndTitle, IDC_LIBRARY_IMPORT_ORDERS), GWL_STYLE) & WS_VISIBLE)) goto LDone;
            fHostOrderPickerCancel = TRUE;
            SendMessage(hwndTitle, WM_COMMAND, MAKEWPARAM(IDC_LIBRARY_IMPORT_ORDERS, BN_CLICKED),
                        (LPARAM)GetDlgItem(hwndTitle, IDC_LIBRARY_IMPORT_ORDERS));
            if (cHostOrderPickerCalls != 1 || FNativeFileSize(szHostManagedOrders, &cbHostFile)) goto LDone;
            fHostOrderPickerCancel = FALSE;
            iHostLibraryTestStage = 11;
            return;
        }
        if (iHostLibraryTestStage == 11) {
            SendMessage(hwndTitle, WM_COMMAND, MAKEWPARAM(IDC_LIBRARY_IMPORT_ORDERS, BN_CLICKED),
                        (LPARAM)GetDlgItem(hwndTitle, IDC_LIBRARY_IMPORT_ORDERS));
            if (cHostOrderPickerCalls != 2 || !FTestFilesEqual(szHostPickerOrder, szHostManagedOrders)) goto LDone;
            fFreeingTitle = FALSE;
            fChecked = TRUE;
            fPassed = TRUE;
            puts("Host Games visible selection, unsaved-work prompt, order picker, validation/conflicts and generation passed");
            PostQuitMessage(0);
            return;
        }
    }
    if (getenv("STARS_CLIENT_STARTUP_SOURCE")) {
        HWND hwndDialog;
        for (hwndDialog = GetWindow(hwndFrame, GW_HWNDFIRST); hwndDialog; hwndDialog = GetWindow(hwndDialog, GW_HWNDNEXT)) {
            if (hwndDialog->fDialog && hwndDialog->lpfnDlg == LibraryNameDlg && IsWindowVisible(hwndDialog)) {
                if (!SetDlgItemText(hwndDialog, IDC_EDIT1, "startup-import") || !FPushClick(GetDlgItem(hwndDialog, IDOK))) goto LDone;
                return;
            }
        }
        if (getenv("STARS_CLIENT_STARTUP_MANAGED")) {
            if (game.lid == 0 || !lpPlanets || idPlayer == iplrNone || hwndTitle) {
                fprintf(stderr, "Managed startup: game %ld planets %p player %d title %p base %s\n",
                        (long)game.lid, (void *)lpPlanets, idPlayer, (void *)hwndTitle, szBase);
                goto LDone;
            }
        } else {
            if (!IsWindowVisible(hwndTitle)) return;
            UpdateWindow(hwndTitle);
            if (!FCompatPresent(hwndTitle) || game.lid != 0 || idPlayer != iplrNone ||
                SendMessage(GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES), LB_FINDSTRING, (WPARAM)-1, (LPARAM)"startup-import") == LB_ERR) goto LDone;
        }
        fChecked = fPassed = fTitlePainted = TRUE;
        puts("Desktop startup uses the supplied game path without reopening the chooser");
        PostQuitMessage(0);
        return;
    }
    if (getenv("STARS_CLIENT_RACE_ROSTER_TEST") || getenv("STARS_CLIENT_RACE_LIBRARY_TEST")) {
        HWND hwndSave = HwndRosterDialog(LibraryRaceSaveDlg);
        if (hwndSave) {
            if (!FPushClick(GetDlgItem(hwndSave, IDC_LIBRARY_RACE_UPDATE))) goto LDone;
            return;
        }
    }
    if (getenv("STARS_CLIENT_RACE_VERSION_TEST")) {
        if (!FCheckRaceVersionWorkflow()) goto LDone;
        return;
    }
    if (getenv("STARS_CLIENT_RACE_ROSTER_TEST")) {
        if (!FCheckRaceRosterWorkflow()) goto LDone;
        return;
    }
    if (getenv("STARS_CLIENT_RACE_NAME_CANCEL")) {
        HWND hwndTop;
        BOOL fWizard = !strcmp(getenv("STARS_CLIENT_RACE_NAME_CANCEL"), "wizard");
        if (iStage == 0) {
            if (!IsWindowVisible(hwndTitle)) return;
            fTitlePainted = fChecked = TRUE;
            if (!SetWindowPos(hwndTitle, NULL, 55, 95, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) goto LDone;
            iStage = 1;
            if (!FPushClick(GetDlgItem(hwndTitle, IDC_LIBRARY_RACE_NEW))) goto LDone;
            return;
        }
        if (iStage == 1 || (fWizard && iStage == 2)) {
            for (hwndTop = HwndCompatFirstTop(); hwndTop; hwndTop = GetWindow(hwndTop, GW_HWNDNEXT)) {
                if (!hwndTop->fDialog || !IsWindowVisible(hwndTop) ||
                    hwndTop->lpfnDlg != (iStage == 1 ? RaceWizardDlg1 : RaceWizardDlg4)) {
                    if (hwndTop->fDialog && hwndTop->lpfnDlg == LibraryNameDlg) goto LDone;
                    continue;
                }
                if (!(SDL_GetWindowFlags(hwndFrame->lpWindow) & SDL_WINDOW_HIDDEN)) goto LDone;
                if (!FPushClick(GetDlgItem(hwndTop, iStage == 1 && fWizard ? IDC_NEXT : IDCANCEL))) goto LDone;
                iStage = fWizard && iStage == 1 ? 2 : 4;
                return;
            }
            return;
        }
        if (!IsWindowEnabled(hwndTitle)) return;
        // Let queued native activation/show events settle after the modal closes.
        if (iStage++ < 6) return;
        for (hwndTop = HwndCompatFirstTop(); hwndTop; hwndTop = GetWindow(hwndTop, GW_HWNDNEXT)) {
            if (!hwndTop->lpWindow || (SDL_GetWindowFlags(hwndTop->lpWindow) & SDL_WINDOW_HIDDEN)) continue;
            if (hwndTop != hwndTitle) {
                fprintf(stderr, "Cancelling the race dialog exposed another native window (frame %d, visible %d)\n",
                        hwndTop == hwndFrame, IsWindowVisible(hwndTop));
                goto LDone;
            }
        }
        if (IsWindowVisible(hwndFrame) || !IsWindowVisible(hwndTitle) ||
            (SDL_GetWindowFlags(hwndTitle->lpWindow) & SDL_WINDOW_HIDDEN) ||
            game.lid != 0 || idPlayer != iplrNone) goto LDone;
        fPassed = TRUE;
        puts("Race dialog cancels back to the single library window");
        PostQuitMessage(0);
        return;
    }
    if (getenv("STARS_CLIENT_RECENT_STARTUP")) {
        HWND hwndTop;
        int cVisible = 0;
        if (!IsWindowVisible(hwndTitle)) return;
        for (hwndTop = HwndCompatFirstTop(); hwndTop; hwndTop = GetWindow(hwndTop, GW_HWNDNEXT)) {
            if (!hwndTop->lpWindow || (SDL_GetWindowFlags(hwndTop->lpWindow) & SDL_WINDOW_HIDDEN)) continue;
            if (hwndTop != hwndTitle) goto LDone;
            cVisible++;
        }
        if (cVisible != 1 || IsWindowVisible(hwndFrame) || cRecentMigrationPrompts != 0 ||
            game.lid != 0 || idPlayer != iplrNone || !GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES) ||
            !GetDlgItem(hwndTitle, IDC_LIBRARY_RACES)) goto LDone;
        UpdateWindow(hwndTitle);
        if (!FCompatPresent(hwndTitle)) goto LDone;
        fTitlePainted = fChecked = fPassed = TRUE;
        puts("Startup with recent files shows only the library, without an import prompt");
        PostQuitMessage(0);
        return;
    }
    if (getenv("STARS_CLIENT_EXTERNAL_OPEN_GAME") || getenv("STARS_CLIENT_EXTERNAL_OPEN_RACE")) {
        HWND hwndDialog;
        if (iStage == 0) {
            if (!IsWindowVisible(hwndTitle)) return;
            UpdateWindow(hwndTitle);
            if (!FCompatPresent(hwndTitle)) goto LDone;
            fTitlePainted = TRUE;
            fChecked = TRUE;
            iStage = 1;
            SendMessage(hwndFrame, WM_COMMAND, IDM_FILE_OPEN_GAME, 0);
            return;
        }
        if (iStage == 1) {
            for (hwndDialog = GetWindow(hwndTitle, GW_HWNDFIRST); hwndDialog; hwndDialog = GetWindow(hwndDialog, GW_HWNDNEXT)) {
                if (!hwndDialog->fDialog || hwndDialog->lpfnDlg != LibraryNameDlg) continue;
                if (!SetWindowTextA(GetDlgItem(hwndDialog, IDC_EDIT1),
                                    getenv("STARS_CLIENT_IMPORT_NAME") ? getenv("STARS_CLIENT_IMPORT_NAME") : "external-open-game") ||
                    !FPushClick(GetDlgItem(hwndDialog, IDOK))) goto LDone;
                iStage = 2;
                return;
            }
            return;
        }
        if (getenv("STARS_CLIENT_EXTERNAL_OPEN_RACE")) {
            if (iStage == 2) {
                for (hwndDialog = GetWindow(hwndTitle, GW_HWNDFIRST); hwndDialog; hwndDialog = GetWindow(hwndDialog, GW_HWNDNEXT)) {
                    if (!hwndDialog->fDialog || hwndDialog->lpfnDlg != RaceWizardDlg1 || !IsWindowVisible(hwndDialog)) continue;
                    iStage = 3;
                    if (!FPushClick(GetDlgItem(hwndDialog, IDCANCEL))) goto LDone;
                    return;
                }
                return;
            }
            if (!IsWindowEnabled(hwndTitle)) return;
            if (SendMessage(GetDlgItem(hwndTitle, IDC_LIBRARY_RACES), LB_FINDSTRING, (WPARAM)-1,
                            (LPARAM)getenv("STARS_CLIENT_IMPORT_NAME")) == LB_ERR || game.lid != 0 || idPlayer != iplrNone) goto LDone;
            fPassed = TRUE;
            puts("Desktop Open Game imports an external race and opens its managed edit wizard");
            PostQuitMessage(0);
            return;
        }
        if (SendMessage(GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES), LB_FINDSTRING, (WPARAM)-1,
                        (LPARAM)(getenv("STARS_CLIENT_IMPORT_NAME") ? getenv("STARS_CLIENT_IMPORT_NAME") : "external-open-game")) == LB_ERR ||
            game.lid != 0 || idPlayer != iplrNone) goto LDone;
        fPassed = TRUE;
        puts("Desktop Open Game imports the external file into the managed library");
        PostQuitMessage(0);
        return;
    }
    if (pszExportGame) {
        if (!FCheckGameExportWorkflow(id)) goto LDone;
        return;
    }
    if (getenv("STARS_CLIENT_RACE_LIBRARY_TEST")) {
        HWND hwndRace;
        static const DLGPROC rglpfnRaceLibrary[] = {RaceWizardDlg1, RaceWizardDlg4, RaceWizardDlg5, RaceWizardDlg2, RaceWizardDlg3, RaceWizardDlg6};
        if (iRaceLibraryTestStage == 0) {
            if (!IsWindowVisible(hwndTitle) || !GetDlgItem(hwndTitle, IDC_LIBRARY_RACES) ||
                !GetDlgItem(hwndTitle, IDC_LIBRARY_RACE_NEW) || !GetDlgItem(hwndTitle, IDC_LIBRARY_RACE_EDIT) ||
                !GetDlgItem(hwndTitle, IDC_LIBRARY_RACE_IMPORT)) {
                goto LDone;
            }
            if (!FPushClick(GetDlgItem(hwndTitle, IDC_LIBRARY_RACE_NEW))) goto LDone;
            iRaceLibraryTestStage = 1;
            return;
        }
        if (iRaceLibraryTestStage == 1) {
            for (hwndRace = GetWindow(hwndTitle, GW_HWNDFIRST); hwndRace; hwndRace = GetWindow(hwndRace, GW_HWNDNEXT)) {
                if (!hwndRace->fDialog) continue;
                if (hwndRace->lpfnDlg != RaceWizardDlg1 ||
                    !SetDlgItemText(hwndRace, IDC_RACE_NAME, "Managed") ||
                    !SetDlgItemText(hwndRace, IDC_RACE_PLURAL_NAME, "Manageds")) goto LDone;
                iRaceLibraryTestStage = 2;
                return;
            }
            return;
        }
        if (iRaceLibraryTestStage == 2) {
            for (hwndRace = GetWindow(hwndTitle, GW_HWNDFIRST); hwndRace; hwndRace = GetWindow(hwndRace, GW_HWNDNEXT)) {
                if (!hwndRace->fDialog) continue;
                if (!IsWindowVisible(hwndRace)) return;
                i = (size_t)iRaceLibraryWizardPage;
                if (i >= sizeof(rglpfnRaceLibrary) / sizeof(rglpfnRaceLibrary[0])) goto LDone;
                if (!FPushClick(GetDlgItem(hwndRace, i == 5 ? IDC_FINISH : IDC_NEXT))) goto LDone;
                iRaceLibraryWizardPage++;
                return;
            }
            if (iRaceLibraryWizardPage == 6 && IsWindowEnabled(hwndTitle)) iRaceLibraryTestStage = 3;
            return;
        }
        if (iRaceLibraryTestStage == 3) {
            if (!IsWindowVisible(hwndTitle)) return;
            if (!FLoadLibraryRace(hwndTitle, "Managed.r1")) goto LDone;
            CchSnprintf(szManaged, sizeof(szManaged), "%s%sRace Designs%sManaged.r1", pszSettings, szDirSep, szDirSep);
            if (!FTestCopyFile(szManaged, pszRaceSource) || !FTestCopyFile(pszRaceSource, pszRaceSnapshot)) goto LDone;
            if (!FSaveLibraryRace(hwndTitle, "Managed.r1.pending.r1", &vplr)) goto LDone;
            icksRaceSaveStage = IRaceChecksum(&vplr);
            if (!FLoadLibraryRace(hwndTitle, "Managed.r1") || IRaceChecksum(&vplr) != icksRaceSaveStage) goto LDone;
            RefreshLibraryRaces(hwndTitle);
            if (SendMessage(GetDlgItem(hwndTitle, IDC_LIBRARY_RACES), LB_GETCOUNT, 0, 0) != 2 ||
                !FSelectClientRaceName(hwndTitle, "Managed.r1")) goto LDone;
            icksRaceLibraryOriginal = IRaceChecksum(&vplr);
            icksumRaceLibrary = icksRaceLibraryOriginal;
            iRaceLibraryWizardPage = 0;
            iRaceLibraryTestStage = 4;
            if (!FPushClick(GetDlgItem(hwndTitle, IDC_LIBRARY_RACE_EDIT))) goto LDone;
            return;
        }
        if (iRaceLibraryTestStage == 4) {
            for (hwndRace = GetWindow(hwndTitle, GW_HWNDFIRST); hwndRace; hwndRace = GetWindow(hwndRace, GW_HWNDNEXT)) {
                if (!hwndRace->fDialog) continue;
                if (iRaceLibraryWizardPage < 6) {
                    i = (size_t)iRaceLibraryWizardPage;
                    if (i == 0 && !FPushClick(GetDlgItem(hwndRace, IDC_RACE_HUMANOID + 1))) goto LDone;
                    if (!FPushClick(GetDlgItem(hwndRace, i == 5 ? IDC_FINISH : IDC_NEXT))) goto LDone;
                    iRaceLibraryWizardPage++;
                    return;
                }
                goto LDone;
            }
            if (iRaceLibraryWizardPage == 6 && IsWindowEnabled(hwndTitle)) iRaceLibraryTestStage = 5;
            return;
        }
        if (iRaceLibraryTestStage == 5) {
            if (!IsWindowEnabled(hwndTitle)) return;
            if (!FLoadLibraryRace(hwndTitle, "Managed.r1") || IRaceChecksum(&vplr) == icksRaceLibraryOriginal) goto LDone;
            icksumRaceLibrary = IRaceChecksum(&vplr);
            RefreshLibraryRaces(hwndTitle);
            if (!FSelectClientRaceName(hwndTitle, "Managed.r1")) goto LDone;
            iRaceLibraryWizardPage = 0;
            if (!FPushClick(GetDlgItem(hwndTitle, IDC_LIBRARY_RACE_EDIT))) goto LDone;
            iRaceLibraryTestStage = 6;
            return;
        }
        if (iRaceLibraryTestStage == 6) {
            for (hwndRace = GetWindow(hwndTitle, GW_HWNDFIRST); hwndRace; hwndRace = GetWindow(hwndRace, GW_HWNDNEXT)) {
                if (!hwndRace->fDialog) continue;
                if (!FPushClick(GetDlgItem(hwndRace, IDCANCEL))) goto LDone;
                iRaceLibraryTestStage = 7;
                return;
            }
            return;
        }
        if (iRaceLibraryTestStage == 7) {
            if (!IsWindowEnabled(hwndTitle)) return;
            if (!FLoadLibraryRace(hwndTitle, "Managed.r1") || IRaceChecksum(&vplr) != icksumRaceLibrary) goto LDone;
            if (FImportLibraryRaceAs(hwndTitle, getenv("STARS_CLIENT_RACE_IMPORT_SOURCE"), "CON.r1") ||
                FImportLibraryRaceAs(hwndTitle, getenv("STARS_CLIENT_RACE_IMPORT_SOURCE"), "COM1.r2") ||
                FImportLibraryRaceAs(hwndTitle, getenv("STARS_CLIENT_RACE_IMPORT_SOURCE"), "LPT9.r3") ||
                FImportLibraryRaceAs(hwndTitle, getenv("STARS_CLIENT_RACE_IMPORT_SOURCE"), "CLOCK$.r1") ||
                FImportLibraryRaceAs(hwndTitle, getenv("STARS_CLIENT_RACE_IMPORT_SOURCE"), "managed.R1")) goto LDone;
            if (!FSaveLibraryRace(hwndTitle, "Imported.r2.pending.r1", &vplr)) goto LDone;
            icksRaceImportStage = IRaceChecksum(&vplr);
            if (!FLoadLibraryRace(hwndTitle, "Managed.r1") || IRaceChecksum(&vplr) != icksumRaceLibrary) goto LDone;
            if (!FPushClick(GetDlgItem(hwndTitle, IDC_LIBRARY_RACE_IMPORT))) goto LDone;
            iRaceLibraryTestStage = 8;
            return;
        }
        if (iRaceLibraryTestStage == 8) {
            for (hwndRace = GetWindow(hwndTitle, GW_HWNDFIRST); hwndRace; hwndRace = GetWindow(hwndRace, GW_HWNDNEXT)) {
                if (!hwndRace->fDialog || !GetDlgItem(hwndRace, IDC_EDIT1)) continue;
                if (!SetWindowTextA(GetDlgItem(hwndRace, IDC_EDIT1), "Imported.r2") || !FPushClick(GetDlgItem(hwndRace, IDOK))) goto LDone;
                iRaceLibraryTestStage = 9;
                return;
            }
            return;
        }
        if (iRaceLibraryTestStage == 9) {
            for (hwndRace = GetWindow(hwndTitle, GW_HWNDFIRST); hwndRace; hwndRace = GetWindow(hwndRace, GW_HWNDNEXT)) {
                if (!hwndRace->fDialog) continue;
                if (!FPushClick(GetDlgItem(hwndRace, IDCANCEL))) goto LDone;
                iRaceLibraryTestStage = 10;
                return;
            }
            return;
        }
        if (iRaceLibraryTestStage == 10) {
            if (!IsWindowEnabled(hwndTitle)) return;
            RefreshLibraryRaces(hwndTitle);
            if (SendMessage(GetDlgItem(hwndTitle, IDC_LIBRARY_RACES), LB_GETCOUNT, 0, 0) != 4 ||
                !FSelectClientRaceName(hwndTitle, "Imported.r2") || !FSelectLibraryRace(hwndTitle, szRaceName) ||
                strcmp(szRaceName, "Imported.r2") || !FLoadLibraryRace(hwndTitle, szRaceName) || !IRaceChecksum(&vplr) ||
                !FLoadLibraryRace(hwndTitle, "Imported.r2.pending.r1") || IRaceChecksum(&vplr) != icksRaceImportStage) goto LDone;
            if (!FDuplicateLibraryRace(hwndTitle, "Managed.r1")) goto LDone;
            if (!FDuplicateLibraryRace(hwndTitle, "Managed.r2")) goto LDone;
            CchSnprintf(szDuplicate, sizeof(szDuplicate), "%s%sRace Designs%sManaged.r2", pszSettings, szDirSep, szDirSep);
            CchSnprintf(szManaged, sizeof(szManaged), "%s%sRace Designs%sManaged.r1", pszSettings, szDirSep, szDirSep);
            CchSnprintf(szExport, sizeof(szExport), "%s%sRace Designs%smanaged-export.r1", pszSettings, szDirSep, szDirSep);
            if (!FTestFilesEqual(szManaged, szDuplicate)) goto LDone;
            CchSnprintf(szDuplicate, sizeof(szDuplicate), "%s%sRace Designs%sManaged.r3", pszSettings, szDirSep, szDirSep);
            if (!FTestFilesEqual(szManaged, szDuplicate)) goto LDone;
            if (!FExportLibraryRace(hwndTitle, "Managed.r1", szExport) ||
                !FLoadLibraryRace(hwndTitle, "Managed.r1") || !FLoadLibraryRace(hwndTitle, "managed-export.r1")) goto LDone;
            if (!FTestFilesEqual(szManaged, szExport)) goto LDone;
            CchSnprintf(szExport, sizeof(szExport), "%s%sRace Designs%smanaged-export-failure.r1", pszSettings, szDirSep, szDirSep);
            CchSnprintf(szExportSnapshot, sizeof(szExportSnapshot), "%s%sRace Designs%smanaged-export-failure.snapshot", pszSettings, szDirSep, szDirSep);
            if (!FTestWriteFile(szExport, "preserve prior export") || !FTestCopyFile(szExport, szExportSnapshot)) goto LDone;
            CchSnprintf(szRaceExportFailTarget, sizeof(szRaceExportFailTarget), "%s", szExport);
            fFailRaceExportReplace = TRUE;
            fExpectRaceExportFailureAlert = TRUE;
            if (FExportLibraryRace(hwndTitle, "Managed.r1", szExport)) {
                fExpectRaceExportFailureAlert = FALSE;
                goto LDone;
            }
            fExpectRaceExportFailureAlert = FALSE;
            if (fFailRaceExportReplace || !FTestFilesEqual(szExport, szExportSnapshot)) goto LDone;
            CchSnprintf(szDuplicate, sizeof(szDuplicate), "%s%sRace Designs%sManaged.r2", pszSettings, szDirSep, szDirSep);
            CchSnprintf(szExport, sizeof(szExport), "%s%sRace Designs%sManaged.r3", pszSettings, szDirSep, szDirSep);
            if (FDeleteLibraryRace(hwndTitle, "Managed.r2") || !FNativeFileSize(szDuplicate, &(int64_t){0}) ||
                !FNativeFileSize(szExport, &(int64_t){0}) || !FNativeFileSize(szManaged, &(int64_t){0})) goto LDone;
            if (FDeleteLibraryRace(hwndTitle, "Managed.r3") || !FNativeFileSize(szExport, &(int64_t){0}) ||
                !FNativeFileSize(szDuplicate, &(int64_t){0}) || !FNativeFileSize(szManaged, &(int64_t){0})) goto LDone;
            if (!FDeleteLibraryRace(hwndTitle, "Managed.r2") || FNativeFileSize(szDuplicate, &(int64_t){0}) ||
                !FNativeFileSize(szExport, &(int64_t){0}) || !FNativeFileSize(szManaged, &(int64_t){0})) goto LDone;
            fChecked = TRUE;
            fPassed = TRUE;
            if (!FTestRaceCancellation()) {
                fprintf(stderr, "Cancelled race operation changed the active design or its destination\n");
                fPassed = FALSE;
                goto LDone;
            }
            if (!FTestLegacyRaceImport()) {
                fprintf(stderr, "Legacy race import did not retain its converted names and password\n");
                fPassed = FALSE;
                goto LDone;
            }
            puts("Managed race created, edited, cancelled, imported, duplicated, exported, and reloaded");
            PostQuitMessage(0);
            return;
        }
        return;
    }
    if (getenv("STARS_CLIENT_GAME_DELETE_MODE")) {
        pszDeleteMode = getenv("STARS_CLIENT_GAME_DELETE_MODE");
        hwndDeleteList = GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES);
        iDeleteSelected = SendMessage(hwndDeleteList, LB_FINDSTRING, (WPARAM)-1, (LPARAM)"selected-game - Player 1 (2400)");
        iDeleteOther = SendMessage(hwndDeleteList, LB_FINDSTRING, (WPARAM)-1, (LPARAM)"other-game - Player 1 (2400)");
        if (!IsWindowVisible(hwndTitle) || !GetDlgItem(hwndTitle, IDC_LIBRARY_GAME_DELETE) || iDeleteOther == LB_ERR) goto LDone;
        if (iStage == 0) {
            if (iDeleteSelected == LB_ERR) goto LDone;
            SendMessage(hwndDeleteList, LB_SETCURSEL, iDeleteSelected, 0);
            iStage = 1;
            if (!FPushClick(GetDlgItem(hwndTitle, IDC_LIBRARY_GAME_DELETE))) goto LDone;
            return;
        }
        if (!fLibraryGameDeletePrompt) goto LDone;
        RefreshLibraryGames(hwndTitle);
        hwndDeleteList = GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES);
        iDeleteSelected = SendMessage(hwndDeleteList, LB_FINDSTRING, (WPARAM)-1, (LPARAM)"selected-game - Player 1 (2400)");
        iDeleteOther = SendMessage(hwndDeleteList, LB_FINDSTRING, (WPARAM)-1, (LPARAM)"other-game - Player 1 (2400)");
        fPassed = pszDeleteMode && strcmp(pszDeleteMode, "confirm") == 0 ? iDeleteSelected == LB_ERR && iDeleteOther != LB_ERR :
                  iDeleteSelected != LB_ERR && iDeleteOther != LB_ERR &&
                  (strcmp(pszDeleteMode, "cancel") == 0 || fLibraryGameDeleteFailureAlert);
        if (!fPassed || (!strcmp(pszDeleteMode, "confirm") && IsWindowEnabled(rghwndBtnSplash[2]))) goto LDone;
        fChecked = TRUE;
        puts("Managed game deletion confirmed, cancelled, or failed through the Games view");
        PostQuitMessage(0);
        return;
    }
    if (getenv("STARS_CLIENT_GAME_LIBRARY_TEST")) {
        const char *pszMode = getenv("STARS_CLIENT_GAME_LIBRARY_TEST");
        const char *pszRoot = getenv("STARS_LIBRARY_DIR");
        const char *pszSource = getenv("STARS_CLIENT_GAME_SOURCE_NAME");
        const char *pszCopy = getenv("STARS_CLIENT_GAME_COPY_NAME");
        const char *pszRename = getenv("STARS_CLIENT_GAME_RENAME_NAME");
        const char *pszCollision = getenv("STARS_CLIENT_GAME_COLLISION_NAME");
        const char *pszBaseline = getenv("STARS_CLIENT_GAME_BASELINE_NAME");
        char szSource[512], szCopy[512], szRenamed[512], szPath[512];
        char szBaseline[512];
        char szLongName[256];
        BOOL fDuplicate = pszMode && strcmp(pszMode, "duplicate") == 0;
        BOOL fRename = pszMode && strcmp(pszMode, "rename") == 0;
        char szMetadataName[256], szExchangeName[256];
        int iVersion, iplr;
        long long lPlayed;
        memset(szLongName, 'x', sizeof(szLongName) - 1);
        szLongName[sizeof(szLongName) - 1] = 0;
        if (!IsWindowVisible(hwndTitle) || !pszRoot || !pszSource || !pszCopy || !pszRename || !pszCollision || !pszBaseline ||
            (!fDuplicate && !fRename) ||
            !GetDlgItem(hwndTitle, IDC_LIBRARY_DUPLICATE) || !GetDlgItem(hwndTitle, IDC_LIBRARY_RENAME) ||
            CchSnprintf(szSource, sizeof(szSource), "%s%sGames%s%s", pszRoot, szDirSep, szDirSep, pszSource) <= 0 ||
            CchSnprintf(szCopy, sizeof(szCopy), "%s%sGames%s%s", pszRoot, szDirSep, szDirSep, pszCopy) <= 0 ||
            CchSnprintf(szRenamed, sizeof(szRenamed), "%s%sGames%s%s", pszRoot, szDirSep, szDirSep, pszRename) <= 0 ||
            CchSnprintf(szBaseline, sizeof(szBaseline), "%s%sGames%s%s", pszRoot, szDirSep, szDirSep, pszBaseline) <= 0) goto LDone;
        if (getenv("STARS_CLIENT_GAME_REJECT")) {
            fExpectGameRenameFailure = TRUE;
            fImported = fDuplicate ? FDuplicateLibraryGame(hwndTitle, pszSource, pszCopy) :
                                     FRenameLibraryGame(hwndTitle, pszSource, pszRename);
            fExpectGameRenameFailure = FALSE;
            if (fImported) {
                fprintf(stderr, "Unsafe managed-game action unexpectedly succeeded\n");
                goto LDone;
            }
            fChecked = TRUE;
            fPassed = TRUE;
            puts("Unsafe managed-game action rejected");
            PostQuitMessage(0);
            return;
        }
        if (fDuplicate && !FDuplicateLibraryGame(hwndTitle, pszSource, pszCopy)) goto LDone;
        if (fDuplicate) {
            CchSnprintf(szPath, sizeof(szPath), "%s%s%s.xy", szSource, szDirSep, pszSource);
            {
                char szOther[512];
                CchSnprintf(szOther, sizeof(szOther), "%s%s%s.xy", szCopy, szDirSep, pszCopy);
                if (!FTestFilesEqual(szPath, szOther)) { fprintf(stderr, "duplicate XY mismatch\n"); goto LDone; }
            }
            CchSnprintf(szPath, sizeof(szPath), "%s%s%s.m1", szSource, szDirSep, pszSource);
            {
                char szOther[512];
                CchSnprintf(szOther, sizeof(szOther), "%s%s%s.m1", szCopy, szDirSep, pszCopy);
                if (!FTestFilesEqual(szPath, szOther)) { fprintf(stderr, "duplicate M1 mismatch\n"); goto LDone; }
            }
            {
                char szOther[512];
                CchSnprintf(szPath, sizeof(szPath), "%s%sbackup.000%s%s.m1", szSource, szDirSep, szDirSep, pszSource);
                CchSnprintf(szOther, sizeof(szOther), "%s%sbackup.000%s%s.m1", szCopy, szDirSep, szDirSep, pszCopy);
                if (!FTestFilesEqual(szPath, szOther)) { fprintf(stderr, "duplicate backup mismatch\n"); goto LDone; }
            }
            if (FDuplicateLibraryGame(hwndTitle, pszSource, "CON") ||
                FDuplicateLibraryGame(hwndTitle, pszSource, pszCollision) ||
                FDuplicateLibraryGame(hwndTitle, pszSource, pszCopy) ||
                FDuplicateLibraryGame(hwndTitle, pszSource, szLongName)) { fprintf(stderr, "duplicate invalid/collision/path accepted\n"); goto LDone; }
        }
        if (fRename && FRenameLibraryGame(hwndTitle, pszSource, pszCollision)) {
            fprintf(stderr, "case-insensitive rename collision accepted\n"); goto LDone;
        }
        if (fRename && FRenameLibraryGame(hwndTitle, pszSource, szLongName)) {
            fprintf(stderr, "overlong rename path accepted\n"); goto LDone;
        }
        if (fDuplicate && FRenameLibraryGame(hwndTitle, pszCopy, pszSource)) {
            fprintf(stderr, "rename over source accepted\n"); goto LDone;
        }
        CchSnprintf(szPath, sizeof(szPath), "%s%s.library", szCopy, szDirSep);
        if (fRename) CchSnprintf(szPath, sizeof(szPath), "%s%s.library", szSource, szDirSep);
        iVersion = 0; iplr = 0; lPlayed = -1;
        if (!FTestReadLibraryMetadata(szPath, &iVersion, szMetadataName, sizeof(szMetadataName),
                                      &iplr, &lPlayed, szExchangeName, sizeof(szExchangeName))) {
            fprintf(stderr, "duplicate metadata parse failed\n"); goto LDone;
        }
        if (iVersion != 2 || strcmp(szMetadataName, fDuplicate ? pszCopy : pszSource) ||
            iplr != (fDuplicate ? -1 : 1) || lPlayed != (fDuplicate ? 0 : 123456) ||
            strcmp(szExchangeName, pszBaseline)) { fprintf(stderr, "duplicate metadata mismatch v=%d name=%s player=%d played=%lld exchange=%s\n", iVersion, szMetadataName, iplr, lPlayed, szExchangeName); goto LDone; }
        if (fDuplicate) {
            fChecked = TRUE;
            fPassed = TRUE;
            puts("Managed game duplicated with independent metadata");
            PostQuitMessage(0);
            return;
        }
        CchSnprintf(szGameRenameFailTarget, sizeof(szGameRenameFailTarget), "%s", szRenamed);
        fFailGameRenamePublish = TRUE;
        fExpectGameRenameFailure = TRUE;
        if (FRenameLibraryGame(hwndTitle, pszSource, pszRename) || fFailGameRenamePublish) {
            fExpectGameRenameFailure = FALSE;
            fprintf(stderr, "rename publication failure was not rolled back\n");
            goto LDone;
        }
        fExpectGameRenameFailure = FALSE;
        CchSnprintf(szPath, sizeof(szPath), "%s%s%s.xy", szSource, szDirSep, pszSource);
        if (!FNativeFileSize(szPath, &(int64_t){0}) || FNativeDirectoryExists(szRenamed)) goto LDone;
        if (!FRenameLibraryGame(hwndTitle, pszSource, pszRename)) { fprintf(stderr, "rename failed\n"); goto LDone; }
        CchSnprintf(szPath, sizeof(szPath), "%s%s%s.xy", szRenamed, szDirSep, pszRename);
        {
            char szOther[512];
            CchSnprintf(szOther, sizeof(szOther), "%s%s%s.xy", szBaseline, szDirSep, pszBaseline);
            if (!FTestFilesEqual(szPath, szOther)) { fprintf(stderr, "renamed XY mismatch\n"); goto LDone; }
        }
        CchSnprintf(szPath, sizeof(szPath), "%s%s%s.m1", szRenamed, szDirSep, pszRename);
        {
            char szOther[512];
            CchSnprintf(szOther, sizeof(szOther), "%s%s%s.m1", szBaseline, szDirSep, pszBaseline);
            if (!FTestFilesEqual(szPath, szOther)) { fprintf(stderr, "renamed M1 mismatch\n"); goto LDone; }
        }
        {
            char szOther[512];
            CchSnprintf(szPath, sizeof(szPath), "%s%sbackup.000%s%s.m1", szRenamed, szDirSep, szDirSep, pszRename);
            CchSnprintf(szOther, sizeof(szOther), "%s%sbackup.000%s%s.m1", szBaseline, szDirSep, szDirSep, pszBaseline);
            if (!FTestFilesEqual(szPath, szOther)) { fprintf(stderr, "renamed backup mismatch\n"); goto LDone; }
        }
        CchSnprintf(szPath, sizeof(szPath), "%s%s.library", szRenamed, szDirSep);
        iVersion = 0; iplr = 0; lPlayed = -1;
        if (!FTestReadLibraryMetadata(szPath, &iVersion, szMetadataName, sizeof(szMetadataName),
                                      &iplr, &lPlayed, szExchangeName, sizeof(szExchangeName))) {
            fprintf(stderr, "renamed metadata parse failed\n"); goto LDone;
        }
        if (iVersion != 2 || strcmp(szMetadataName, pszRename) || iplr != 1 || lPlayed != 123456 ||
            strcmp(szExchangeName, pszBaseline)) { fprintf(stderr, "renamed metadata mismatch %d %s\n", iVersion, szMetadataName); goto LDone; }
        CchSnprintf(szPath, sizeof(szPath), "%s%sexchange.map", szRenamed, szDirSep);
        if (!FNativeFileSize(szPath, &(int64_t){0})) { fprintf(stderr, "renamed sidecar missing %s\n", szPath); goto LDone; }
        fChecked = TRUE;
        fPassed = TRUE;
        puts("Managed game renamed with session metadata and backup contents");
        PostQuitMessage(0);
        return;
    }
    if (pszImportSource) {
        if (iImportStage == 0) {
            if (!IsWindowVisible(hwndTitle)) return;
            iImportStage = 1;
            gameImportSav = game;
            idPlayerImportSav = idPlayer;
            strcpy(szBaseImportSav, szBase);
            if (!FImportLibraryGame(hwndTitle, pszImportSource)) goto LDone;
            if (memcmp(&gameImportSav, &game, sizeof(game)) || idPlayerImportSav != idPlayer || strcmp(szBaseImportSav, szBase)) goto LDone;
            iImportStage = 2;
            RefreshLibraryGames(hwndTitle);
            {
                int cExpected = getenv("STARS_CLIENT_IMPORT_EXPECTED_PLAYERS") ?
                    atoi(getenv("STARS_CLIENT_IMPORT_EXPECTED_PLAYERS")) : 2;
                if (SendMessage(GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES), LB_GETCOUNT, 0, 0) != cExpected) goto LDone;
            }
            {
                HWND hwndList = GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES);
                char szPlayer[320];
                int iplrTest;
                BOOL fPlayers[16] = {FALSE};
                int cExpected = getenv("STARS_CLIENT_IMPORT_EXPECTED_PLAYERS") ?
                    atoi(getenv("STARS_CLIENT_IMPORT_EXPECTED_PLAYERS")) : 2;
                for (iplrTest = 0; iplrTest < cExpected; iplrTest++) {
                    SendMessage(hwndList, LB_GETTEXT, iplrTest, (LPARAM)szPlayer);
                    if (strstr(szPlayer, "Player ") && strstr(szPlayer, "(2400)")) {
                        int iplrFound;
                        for (iplrFound = 0; iplrFound < cExpected; iplrFound++) {
                            char szExpected[32];
                            CchSnprintf(szExpected, sizeof(szExpected), "Player %d (2400)", iplrFound + 1);
                            if (strstr(szPlayer, szExpected)) fPlayers[iplrFound] = TRUE;
                        }
                    }
                }
                for (iplrTest = 0; iplrTest < cExpected; iplrTest++) if (!fPlayers[iplrTest]) goto LDone;
            }
            fChecked = TRUE;
            fPassed = TRUE;
            puts("Game family imported into the managed library");
            PostQuitMessage(0);
            return;
        }
        if (iImportStage == 1) {
            for (hwnd = GetWindow(hwndTitle, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
                if (!hwnd->fDialog || hwnd->lpfnDlg != LibraryNameDlg) continue;
                if (!SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT1), getenv("STARS_CLIENT_IMPORT_NAME") ? getenv("STARS_CLIENT_IMPORT_NAME") : "imported-game")) goto LDone;
                FPushClick(GetDlgItem(hwnd, IDOK));
                return;
            }
        }
        return;
    }
    if (getenv("STARS_CLIENT_IMPORT_UPDATE_SOURCE")) {
        pszExpected = getenv("STARS_CLIENT_IMPORT_UPDATE_EXPECTED");
        if (iImportUpdateStage == 0) {
            if (!IsWindowVisible(hwndTitle)) return;
            iImportUpdateStage = 1;
            cAlerts = 0;
            fImported = FImportLibraryGame(hwndTitle, getenv("STARS_CLIENT_IMPORT_UPDATE_SOURCE"));
            fPassed = pszExpected && ((!strcmp(pszExpected, "fail") && !fImported && cAlerts > 0) ||
                ((!strcmp(pszExpected, "identical") || !strcmp(pszExpected, "later") ||
                  !strcmp(pszExpected, "player16") ||
                  !strcmp(pszExpected, "adjacent") || !strcmp(pszExpected, "replace-conflict") ||
                  !strcmp(pszExpected, "separate-conflict")) && fImported && cAlerts == 0) ||
                (!strcmp(pszExpected, "fail-conflict") && !fImported && cAlerts > 0 &&
                 (!getenv("STARS_CLIENT_IMPORT_UPDATE_FAIL_CONFLICT_RESTORE") || fConflictRestoreAlert)) ||
                ((!strcmp(pszExpected, "cancel") || !strcmp(pszExpected, "cancel-conflict")) && !fImported && cAlerts == 0));
            if (fPassed && (!strcmp(pszExpected, "later") || !strcmp(pszExpected, "adjacent"))) {
                pszYear = !strcmp(pszExpected, "adjacent") ? "2408" : "2425";
                hwndList = GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES);
                pszTarget = getenv("STARS_CLIENT_IMPORT_UPDATE_TARGET");
                RefreshLibraryGames(hwndTitle);
                CchSnprintf(szFind, sizeof(szFind), "%s - Player 1 (%s)", pszTarget, pszYear);
                fPassed = pszTarget && SendMessage(hwndList, LB_FINDSTRING, (WPARAM)-1, (LPARAM)szFind) != LB_ERR;
                CchSnprintf(szFind, sizeof(szFind), "%s - Player 2 (2400)", pszTarget);
                fPassed = fPassed && SendMessage(hwndList, LB_FINDSTRING, (WPARAM)-1, (LPARAM)szFind) != LB_ERR;
                if (!strcmp(pszExpected, "later")) {
                    fPassed = fPassed && SendMessage(hwndList, LB_FINDSTRING, (WPARAM)-1,
                        (LPARAM)"managed-copy - Player 1 (2408)") != LB_ERR;
                }
            }
            fChecked = TRUE;
            PostQuitMessage(fPassed ? 0 : 93);
            return;
        }
        if (iImportUpdateStage == 1) {
            for (hwnd = GetWindow(hwndTitle, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
                if (!hwnd->fDialog || hwnd->lpfnDlg != LibraryTurnTargetDlg) continue;
                hwndListTarget = GetDlgItem(hwnd, IDC_LIBRARY_TURN_TARGET_LIST);
                if (IsWindowEnabled(GetDlgItem(hwnd, IDOK))) goto LDone;
                pszTarget = getenv("STARS_CLIENT_IMPORT_UPDATE_TARGET");
                c = (int)SendMessage(hwndListTarget, LB_GETCOUNT, 0, 0);
                for (iTarget = 0; iTarget < c; iTarget++) {
                    SendMessage(hwndListTarget, LB_GETTEXT, iTarget, (LPARAM)szName);
                    if (pszTarget && !strcmp(pszTarget, szName)) break;
                }
                if (iTarget == c) { FPushClick(GetDlgItem(hwnd, IDCANCEL)); return; }
                SendMessage(hwndListTarget, LB_SETCURSEL, iTarget, 0);
                SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_LIBRARY_TURN_TARGET_LIST, LBN_SELCHANGE),
                            (LPARAM)hwndListTarget);
                FPushClick(GetDlgItem(hwnd, !strcmp(pszExpected, "cancel") ? IDCANCEL : IDOK));
                return;
            }
            for (hwnd = GetWindow(hwndTitle, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
                if (!hwnd->fDialog || hwnd->lpfnDlg != LibraryTurnConflictDlg) continue;
                if (pszExpected && !strcmp(pszExpected, "cancel-conflict"))
                    FPushClick(GetDlgItem(hwnd, IDCANCEL));
                else if (pszExpected && !strcmp(pszExpected, "replace-conflict"))
                    FPushClick(GetDlgItem(hwnd, IDC_LIBRARY_TURN_REPLACE));
                else if (pszExpected && !strcmp(pszExpected, "fail-conflict"))
                    FPushClick(GetDlgItem(hwnd, IDC_LIBRARY_TURN_REPLACE));
                else if (pszExpected && !strcmp(pszExpected, "separate-conflict"))
                    FPushClick(GetDlgItem(hwnd, IDC_LIBRARY_TURN_SEPARATE));
                else goto LDone;
                return;
            }
            if (pszExpected && !strcmp(pszExpected, "separate-conflict")) {
                for (hwnd = GetWindow(hwndTitle, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
                    if (!hwnd->fDialog || hwnd->lpfnDlg != LibraryNameDlg) continue;
                    pszCopyName = getenv("STARS_CLIENT_IMPORT_COPY_NAME");
                    if (pszCopyName) SetWindowTextA(GetDlgItem(hwnd, IDC_EDIT1), pszCopyName);
                    FPushClick(GetDlgItem(hwnd, IDOK));
                    return;
                }
            }
        }
        return;
    }
    if (getenv("STARS_CLIENT_LIBRARY_OPEN")) {
        const char *pszName = getenv("STARS_CLIENT_LIBRARY_OPEN");
        HWND hwndList, hwndDialog;
        LRESULT iGame;
        if (iStage == 0) {
            const char *pszPlayer = getenv("STARS_CLIENT_LIBRARY_OPEN_PLAYER");
            int iWanted = pszPlayer ? atoi(pszPlayer) - 1 : -1;
            int iRow;
            char szRow[320], szPlayerPrefix[320];
            if (!IsWindowVisible(hwndTitle)) goto LDone;
            hwndList = GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES);
            iGame = SendMessage(hwndList, LB_FINDSTRING, (WPARAM)-1, (LPARAM)pszName);
            if (iGame == LB_ERR) goto LDone;
            if (iWanted >= 0) {
                CchSnprintf(szPlayerPrefix, sizeof(szPlayerPrefix), "%s - Player %d (", pszName, iWanted + 1);
                for (iRow = (int)iGame; iRow < SendMessage(hwndList, LB_GETCOUNT, 0, 0); iRow++) {
                    if (SendMessage(hwndList, LB_GETTEXT, iRow, (LPARAM)szRow) == LB_ERR ||
                        strncmp(szRow, szPlayerPrefix, strlen(szPlayerPrefix))) continue;
                    iGame = iRow;
                    break;
                }
                if (iRow >= SendMessage(hwndList, LB_GETCOUNT, 0, 0)) goto LDone;
            }
            if (getenv("STARS_DEBUG_LIBRARY_STARTUP"))
                fprintf(stderr, "[DEBUG-library-startup] Opening library row %lld: %s\n", (long long)iGame, pszName);
            SendMessage(hwndList, LB_SETCURSEL, iGame, 0);
            fChecked = TRUE;
            iStage = 1;
            if (getenv("STARS_CLIENT_LIBRARY_DOUBLECLICK")) {
                if (!PostMessage(hwndTitle, WM_COMMAND, MAKEWPARAM(IDC_LIBRARY_GAMES, LBN_DBLCLK),
                                  (LPARAM)hwndList)) goto LDone;
            } else if (!FPushClick(rghwndBtnSplash[1])) goto LDone;
            return;
        }
        if (getenv("STARS_CLIENT_SWITCH_CHOICE") && iStage == 20 && IsWindow(hwndTitle)) {
            HWND hwndList = GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES);
            const char *pszName = getenv("STARS_CLIENT_LIBRARY_OPEN");
            int iRow;
            char szRow[320], szPlayerPrefix[320];
            CchSnprintf(szPlayerPrefix, sizeof(szPlayerPrefix), "%s - Player 1 (", pszName);
            for (iRow = 0; iRow < SendMessage(hwndList, LB_GETCOUNT, 0, 0); iRow++) {
                if (SendMessage(hwndList, LB_GETTEXT, iRow, (LPARAM)szRow) == LB_ERR ||
                    strncmp(szRow, szPlayerPrefix, strlen(szPlayerPrefix))) continue;
                SendMessage(hwndList, LB_SETCURSEL, iRow, 0);
                iStage = 21;
                if (!FPushClick(rghwndBtnSplash[1])) goto LDone;
                return;
            }
            goto LDone;
        }
        if (IsWindow(hwndTitle)) {
            if (getenv("STARS_CLIENT_LIBRARY_DOUBLECLICK")) {
                for (hwndDialog = GetWindow(hwndTitle, GW_HWNDFIRST); hwndDialog;
                     hwndDialog = GetWindow(hwndDialog, GW_HWNDNEXT)) {
                    if (!hwndDialog->fDialog) continue;
                    fprintf(stderr, "Game double-click unexpectedly opened a dialog\n");
                    EndDialog(hwndDialog, FALSE);
                    goto LDone;
                }
            }
            return;
        }
        if (!lpPlanets || game.lid == 0 || idPlayer == iplrNone || !IsWindowVisible(hwndFrame)) goto LDone;
        if (getenv("STARS_CLIENT_SWITCH_CHOICE") && iStage == 21) {
            const char *pszExpected = getenv("STARS_CLIENT_EXPECT_SWITCHED_PLAYER");
            if (pszExpected && idPlayer != atoi(pszExpected) - 1) goto LDone;
        } else if (getenv("STARS_CLIENT_EXPECT_PLAYER") &&
                   idPlayer != atoi(getenv("STARS_CLIENT_EXPECT_PLAYER")) - 1) goto LDone;
        if (getenv("STARS_CLIENT_SWITCH_CANCEL_TEST")) {
            int16_t idPlayerBefore = idPlayer;
            uint16_t turnBefore = game.turn;
            long long lPlayedBefore, lPlayedAfter;
            if (!FTestLibraryTimestamp(&lPlayedBefore)) goto LDone;
            game.fDirty = TRUE;
            SendMessage(hwndFrame, WM_COMMAND, IDM_FILE_RETURN_TO_TITLE, 0);
            if (!fSwitchCancelSent || IsWindow(hwndTitle) || !lpPlanets || idPlayer != idPlayerBefore ||
                game.turn != turnBefore || !game.fDirty || !FTestLibraryTimestamp(&lPlayedAfter) ||
                lPlayedAfter != lPlayedBefore) goto LDone;
            fPassed = TRUE;
            puts("Canceling a player switch keeps the active player turn loaded");
            PostQuitMessage(0);
            return;
        }
        if (getenv("STARS_CLIENT_SWITCH_CHOICE")) {
            const char *pszChoice = getenv("STARS_CLIENT_SWITCH_CHOICE");
            const char *pszFailure = getenv("STARS_CLIENT_SWITCH_FAIL_SAVE");
            int fSave = !strcmp(pszChoice, "save") || !strcmp(pszChoice, "submit");
            int fSubmit = !strcmp(pszChoice, "submit");
            if (iStage != 21) {
                char szBlocked[256];
                int16_t idPlayerBefore = idPlayer;
                int16_t iResearch = (rgplr[idPlayer].iTechNow + 1) % 6;
                PLANET *lpPlanetsBefore = lpPlanets;
                if (pszFailure) {
                    if (!FRunWorkflow(1, iResearch)) goto LDone;
                    SetSzWorkFromDt(!strcmp(pszFailure, "orders") ? dtLog : dtHist, idPlayer);
                    strcpy(szBlocked, szWork);
                    if (!SDL_CreateDirectory(szBlocked)) goto LDone;
                    game.fDirty = TRUE;
                    gd.fSubmit = FALSE;
                    SendMessage(hwndFrame, WM_COMMAND, IDM_FILE_RETURN_TO_TITLE, 0);
                    if (IsWindow(hwndTitle) || lpPlanets != lpPlanetsBefore || idPlayer != idPlayerBefore ||
                        !game.fDirty || gd.fSubmit || rgplr[idPlayer].iTechNow != iResearch || cAlerts != 2)
                        goto LDone;
                    if (RemoveNativePath(szBlocked) != 0) goto LDone;
                    puts("Failed save preserves the active player and pending research; retrying the save");
                }
                cSwitchLogWrites = 0;
                cSwitchHistoryWrites = 0;
                game.fDirty = TRUE;
                gd.fSubmit = FALSE;
                SendMessage(hwndFrame, WM_COMMAND, IDM_FILE_RETURN_TO_TITLE, 0);
                if (!IsWindow(hwndTitle) || idPlayer != iplrNone || game.fDirty ||
                    cSwitchLogWrites != fSave || cSwitchHistoryWrites != fSave || gd.fSubmit != fSubmit)
                    goto LDone;
                iStage = 20;
                return;
            }
            if (!lpPlanets || idPlayer != 0) goto LDone;
            fPassed = TRUE;
            printf("Switch prompt %s %s log and history; switched to Player 1\n", pszChoice,
                   fSave ? "saved" : "discarded");
            PostQuitMessage(0);
            return;
        }
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
    if (pszContinue && getenv("STARS_CLIENT_EXPECT_PLAYER")) {
        char szContinue[320];
        const char *pszGame = getenv("STARS_CLIENT_EXPECT_CONTINUE_GAME");
        const char *pszYear = getenv("STARS_CLIENT_EXPECT_CONTINUE_YEAR");
        char szPlayer[32];
        GetWindowTextA(rghwndBtnSplash[2], szContinue, sizeof(szContinue));
        if (pszGame ? !strstr(szContinue, pszGame) : !strstr(szContinue, "Continue: managed-copy")) goto LDone;
        CchSnprintf(szPlayer, sizeof(szPlayer), "Player %s",
                    getenv("STARS_CLIENT_EXPECT_PLAYER") ? getenv("STARS_CLIENT_EXPECT_PLAYER") : "2");
        if (!strstr(szContinue, szPlayer)) goto LDone;
        if (pszYear) {
            char szYear[32];
            CchSnprintf(szYear, sizeof(szYear), "(%s)", pszYear);
            if (!strstr(szContinue, szYear)) goto LDone;
        }
    }
    if (pszContinue && getenv("STARS_CLIENT_EXPECT_CONTINUE_DISABLED")) {
        if (IsWindowEnabled(rghwndBtnSplash[2])) goto LDone;
        fPassed = TRUE;
        puts("Continue is disabled when the managed game has no available player turns");
        PostQuitMessage(0);
        return;
    }
    if (pszContinue && getenv("STARS_CLIENT_EXPECT_SELECTOR")) {
        char szLabel[256];
        HWND hwndList = GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES);
        if (!IsWindowEnabled(rghwndBtnSplash[2])) goto LDone;
        GetWindowTextA(rghwndBtnSplash[2], szLabel, sizeof(szLabel));
        if (!strstr(szLabel, "Choose Player") || !FPushClick(rghwndBtnSplash[2])) goto LDone;
        if (!hwndList || SendMessage(hwndList, LB_GETTEXT,
            SendMessage(hwndList, LB_GETCURSEL, 0, 0), (LPARAM)szLabel) == LB_ERR ||
            !strstr(szLabel, "managed-copy - Player 1 (") || !IsWindow(hwndTitle)) goto LDone;
        fPassed = TRUE;
        puts("Continue selects the managed game when its remembered player is unavailable");
        PostQuitMessage(0);
        return;
    }
    if (!FPushClick(rghwndBtnSplash[pszContinue ? 2 : 0])) goto LDone;
    iStage = pszContinue ? 5 : 1;
    return;
LDone:
    if (!fPassed) {
        if (getenv("STARS_CLIENT_HOST_LIBRARY_TEST"))
            fprintf(stderr, "Host library workflow failed at stage %d\n", iHostLibraryTestStage);
        fprintf(stderr, "Client startup/wizard check failed at stage %d (new-game step %d, choices %d, creating %d): %s\n",
                iStage, iNewGameStep, cNewGameChoices, fCreatingGame, SDL_GetError());
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
    char szCommand[256] = "";
    int nResult;
    if (getenv("STARS_CLIENT_STARTUP_SOURCE"))
        CchSnprintf(szCommand, sizeof(szCommand), "%s", getenv("STARS_CLIENT_STARTUP_SOURCE"));
    pszNewGame = getenv("STARS_CLIENT_NEW_GAME");
    pszContinue = getenv("STARS_CLIENT_CONTINUE");
    pszImportSource = getenv("STARS_CLIENT_IMPORT_SOURCE");
    if (pszContinue && !WritePrivateProfileString("Files", "File1", pszContinue, "StarsUtf8.ini")) return 1;
    if (pszContinue && (!WritePrivateProfileString("Windows", "PlanetTiles", "ABE*FGH", "StarsUtf8.ini") ||
        !WritePrivateProfileString("Windows", "ShipTiles", "AFDE*BJI", "StarsUtf8.ini"))) return 1;
    setvbuf(stdout, NULL, _IONBF, 0);
    nResult = WinMain(GetModuleHandle(NULL), NULL, szCommand, SW_SHOWNORMAL);
    if (IsWindow(hwndTitle) && !pszImportSource && !getenv("STARS_CLIENT_RACE_LIBRARY_TEST") &&
        !getenv("STARS_CLIENT_RECENT_STARTUP") && !getenv("STARS_CLIENT_RACE_NAME_CANCEL") &&
        !getenv("STARS_CLIENT_RACE_VERSION_TEST") &&
        !getenv("STARS_CLIENT_EXTERNAL_OPEN_GAME") &&
        !getenv("STARS_CLIENT_RACE_ROSTER_TEST") && !getenv("STARS_CLIENT_STARTUP_SOURCE") &&
        !getenv("STARS_CLIENT_EXTERNAL_OPEN_RACE") && !getenv("STARS_CLIENT_EXPORT_GAME") &&
        !getenv("STARS_CLIENT_IMPORT_UPDATE_SOURCE") && !getenv("STARS_CLIENT_GAME_LIBRARY_TEST") &&
        !getenv("STARS_CLIENT_GAME_DELETE_MODE") &&
        !getenv("STARS_CLIENT_HOST_LIBRARY_TEST") &&
        !getenv("STARS_CLIENT_EXPECT_SELECTOR") && !getenv("STARS_CLIENT_EXPECT_CONTINUE_DISABLED") &&
        !getenv("STARS_CLIENT_SWITCH_CHOICE")) fPassed = FALSE;
    if (idTimer) KillTimer(NULL, idTimer);
    StarsCompatProcessExit();
    SDL_Quit();
    return nResult == 0 && fChecked && fPassed && fTitlePainted &&
           cAlerts == (getenv("STARS_CLIENT_GAME_DELETE_MODE") &&
                       !strcmp(getenv("STARS_CLIENT_GAME_DELETE_MODE"), "failure") ? 1 :
                       getenv("STARS_CLIENT_HOST_LIBRARY_TEST") ? 5 :
                       getenv("STARS_CLIENT_LIBRARY_FAIL_WRITE") || getenv("STARS_CLIENT_SWITCH_FAIL_SAVE") ? 2 :
                       getenv("STARS_CLIENT_EXPORT_EXPECTED") ? 1 :
                       getenv("STARS_CLIENT_IMPORT_INVALID_EXPECTED") ? 1 :
                       getenv("STARS_CLIENT_IMPORT_UPDATE_EXPECTED") &&
                       (!strcmp(getenv("STARS_CLIENT_IMPORT_UPDATE_EXPECTED"), "fail") ||
                        !strcmp(getenv("STARS_CLIENT_IMPORT_UPDATE_EXPECTED"), "fail-conflict")) ? 1 : 0) && cEarlyFrameShow == 0 ? 0 : 1;
}
