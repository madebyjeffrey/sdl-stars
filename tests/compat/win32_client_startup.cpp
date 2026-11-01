/* Exercise the shared WinMain, embedded resources and actual title painting. */
#include "win.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include "../../src/core/alert_io.h"
#include "../../src/core/player_choice_io.h"
#include "../../src/ui/desktop_choice_io.h"
#include "../../compat/win32/src/choice_io.h"
#include "../../compat/win32/src/render_io.h"
#include "../../compat/win32/src/window_io.h"
#include "../../compat/win32/src/path_io.h"
#include "../../compat/win32/src/folder_io.h"
#include "../../src/core/record_io.h"

#include "native_publish.h"
#include <stars_compat.h>
#include "internal.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" char *StarsTest_SDL_GetPrefPath(const char *pszOrg, const char *pszApp) {
    const char *pszPath = PszCompatEnvironment("STARS_TEST_PREF_PATH");
    if (pszPath) return SDL_strdup(pszPath);
    return SDL_GetPrefPath(pszOrg, pszApp);
}

static int64_t cbTestFile;
static char szReplacementOrder[] = "Replacement order";
static BOOL FTestCopyFile(const char *pszSource, const char *pszTarget) {
    NATIVEFILE *pfSource, *pfTarget;
    unsigned char rgb[4096];
    size_t cb;
    BOOL fOkay;
    pfSource = PfNativeOpen(pszSource, NATIVEFILEMODE::read);
    if (!pfSource) return 0;
    pfTarget = PfNativeOpen(pszTarget, NATIVEFILEMODE::writeTruncate);
    if (!pfTarget) { ICloseNativeFile(pfSource); return 0; }
    fOkay = 1;
    while ((cb = CReadNative(std::span{rgb}, pfSource)) != 0)
        if (CWriteNative(std::span{rgb}.first(cb), pfTarget) != cb) { fOkay = 0; break; }
    if (FNativeStreamError(pfSource)) fOkay = 0;
    if (ICloseNativeFile(pfSource) != 0) fOkay = 0;
    if (ICloseNativeFile(pfTarget) != 0) fOkay = 0;
    return fOkay;
}

static BOOL FTestWriteFile(const char *pszFile, const char *pszText) {
    NATIVEFILE *pf;
    size_t cbText = strlen(pszText);
    BOOL fOkay;
    pf = PfNativeOpen(pszFile, NATIVEFILEMODE::writeTruncate);
    if (!pf) return 0;
    fOkay = CWriteNative(std::span{pszText, cbText}, pf) == cbText;
    if (ICloseNativeFile(pf) != 0) fOkay = 0;
    return fOkay;
}

static BOOL FTestFilesEqual(const char *pszFile1, const char *pszFile2) {
    NATIVEFILE *pfFile1, *pfFile2;
    unsigned char rgb1[4096], rgb2[4096];
    size_t cb1, cb2;
    BOOL fEqual;
    pfFile1 = PfNativeOpen(pszFile1, NATIVEFILEMODE::read);
    if (!pfFile1) return 0;
    pfFile2 = PfNativeOpen(pszFile2, NATIVEFILEMODE::read);
    if (!pfFile2) { ICloseNativeFile(pfFile1); return 0; }
    fEqual = 1;
    do {
        cb1 = CReadNative(std::span{rgb1}, pfFile1);
        cb2 = CReadNative(std::span{rgb2}, pfFile2);
        if (cb1 != cb2 || memcmp(rgb1, rgb2, cb1) != 0) { fEqual = 0; break; }
    } while (cb1 != 0);
    if (FNativeStreamError(pfFile1) || FNativeStreamError(pfFile2)) fEqual = 0;
    if (ICloseNativeFile(pfFile1) != 0) fEqual = 0;
    if (ICloseNativeFile(pfFile2) != 0) fEqual = 0;
    return fEqual;
}

static BOOL FTestReadLibraryMetadata(const char *pszFile, int *piVersion, char *pszName,
                                     size_t cchName, int *piplr, long long *plPlayed,
                                     char *pszExchangeName, size_t cchExchangeName) {
    NATIVEFILE *pf;
    int cchNameRead, cchExchangeRead;
    BOOL fRead;
    pf = PfNativeOpen(pszFile, NATIVEFILEMODE::read);
    if (!pf) return 0;
    fRead = CNativeScan(pf, "%d\n", piVersion) == 1 && PszGetNativeLine(pszName, (int)cchName, pf) &&
            CNativeScan(pf, "%d\n%lld\n", piplr, plPlayed) == 2 &&
            PszGetNativeLine(pszExchangeName, (int)cchExchangeName, pf) != NULL;
    ICloseNativeFile(pf);
    if (!fRead) return 0;
    pszName[strcspn(pszName, "\r\n")] = 0;
    pszExchangeName[strcspn(pszExchangeName, "\r\n")] = 0;
    cchNameRead = (int)strlen(pszName);
    cchExchangeRead = (int)strlen(pszExchangeName);
    return cchNameRead > 0 && (size_t)cchNameRead < cchName &&
           cchExchangeRead > 0 && (size_t)cchExchangeRead < cchExchangeName;
}

static BOOL FTestLibraryTimestamp(APPSESSION &app, long long *plPlayed) {
    char szDir[512], szFile[512], szMetadataName[256];
    char *pch;
    NATIVEFILE *pf;
    int iVersion, iPlayer;
    strcpy(szDir, app.state.szBase);
    pch = strrchr(szDir, chDirSep);
    if (!pch) return 0;
    *pch = 0;
    CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s.library", szDir, szDirSep);
    pf = PfNativeOpen(szFile, NATIVEFILEMODE::read);
    if (!pf) return 0;
    if (CNativeScan(pf, "%d\n", &iVersion) != 1 || !PszGetNativeLine(szMetadataName, sizeof(szMetadataName), pf) ||
        CNativeScan(pf, "%d\n%lld", &iPlayer, plPlayed) != 2) {
        ICloseNativeFile(pf);
        return 0;
    }
    ICloseNativeFile(pf);
    return iVersion == 2 && iPlayer == app.state.idPlayer;
}

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
static int16_t idHostOrderConflictResponse = win32::IDYES;
static int iRosterMenu = -1;
static const char *pszRosterRace;
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
static SDL_WindowID idRacePresentation;
static BOOL fRacePresentation = 1;

extern "C" bool SDLCALL StarsTest_SDL_SetWindowTitle(SDL_Window *lpWindow, const char *pszTitle) {
    HWND hwnd = LpCompatWindowFromId(SDL_GetWindowID(lpWindow));
    SDL_Event event = {};
    unsigned cPresentBefore = cRenderPresent;
    bool fResult = SDL_SetWindowTitle(lpWindow, pszTitle);

    if (hwnd && hwnd->fDeferPresent && hwnd->lpTexture.get() &&
        SDL_GetBooleanProperty(SDL_GetTextureProperties(hwnd->lpTexture.get()), "stars.test.race_frame", false)) {
        // Cocoa can expose the reused window while the next page initializes.
        event.type = SDL_EVENT_WINDOW_EXPOSED;
        event.window.windowID = hwnd->id;
        SDL_PushEvent(&event);
        if (cRenderPresent == cPresentBefore) {
            fprintf(stderr, "Race wizard exposure did not restore the previous frame during page initialization\n");
            fRacePresentation = 0;
        }
    }
    return fResult;
}

bool SDL_RenderPresent(SDL_Renderer *lprenderer);
extern "C" bool SDLCALL StarsTest_SDL_RenderPresent(SDL_Renderer *lprenderer) {
    HWND hwnd;
    static unsigned cRaceFrame;
    RECT rc;
    BYTE r, g, b, a;
    int x, y;
    BOOL fButton;

    cRenderPresent++;
    for (hwnd = HwndCompatRegistry(); hwnd; hwnd = hwnd->lpNext) {
        if (!hwnd->fAlive || hwnd->lpRenderer.get() != lprenderer || !hwnd->lpfnDlg ||
            (hwnd->lpfnDlg != RaceWizardDlg1 && hwnd->lpfnDlg != RaceWizardDlg2 &&
             hwnd->lpfnDlg != RaceWizardDlg3 && hwnd->lpfnDlg != RaceWizardDlg4 &&
             hwnd->lpfnDlg != RaceWizardDlg5 && hwnd->lpfnDlg != RaceWizardDlg6) ||
            !hwnd->lpComposite.get() || hwnd->cxDlgUnit == 0) continue;
        rc = (RECT){66, 218, 110, 232};
        win32::MapDialogRect(hwnd, &rc);
        fButton = 0;
        for (y = rc.top * hwnd->iCanvasScale; y < rc.bottom * hwnd->iCanvasScale; y++)
            for (x = rc.left * hwnd->iCanvasScale; x < rc.right * hwnd->iCanvasScale; x++)
                if (SDL_ReadSurfacePixel(hwnd->lpComposite.get(), x, y, &r, &g, &b, &a) &&
                    (r != 192 || g != 192 || b != 192)) fButton = 1;
        if (!fButton) {
            fprintf(stderr, "Race frame %u has no Cancel button (visible %d, destroying %d, deferred %d)\n",
                    cRaceFrame, hwnd->fVisible, hwnd->fDestroying, hwnd->fDeferPresent);
            fRacePresentation = 0;
        }
        cRaceFrame++;
    }
    return SDL_RenderPresent(lprenderer);
}

static BOOL FCheckScannerDrag(APPSESSION &app) {
    HWND hwndBar = app.lpui->hwndScanner->rghwndScroll[win32::SB_HORZ];
    SCROLLINFO si = {sizeof(si), win32::SIF_ALL, 0, 0, 0, 0, 0};
    RECT rc;
    int i, xGrab, dxThumb, iPosSav;
    int16_t iZoomSav = app.state.iScanZoom, xSav = app.lpui->xScanTop, ySav = app.lpui->yScanTop;
    unsigned cPresentSav;
    Uint64 nsStart;
    BOOL fSinglePresent;

    /* Large test displays can fit the whole map at the saved zoom. */
    if (!win32::IsWindow(hwndBar) || !win32::IsWindowVisible(hwndBar)) {
        ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, zoom400 + 3905, 0);
        hwndBar = app.lpui->hwndScanner->rghwndScroll[win32::SB_HORZ];
    }
    if (!win32::IsWindow(hwndBar) || !win32::IsWindowVisible(hwndBar)) return 0;
    if (!win32::GetScrollInfo(app.lpui->hwndScanner, win32::SB_HORZ, &si) || !win32::GetClientRect(hwndBar, &rc)) return 0;
    iPosSav = win32::GetScrollPos(app.lpui->hwndScanner, win32::SB_HORZ);
    ui::SendMessage(app.lpui->hwndScanner, win32::WM_HSCROLL, win32::MAKEWPARAM(win32::SB_THUMBPOSITION, si.nMin), 0);
    ui::UpdateWindow(app.lpui->hwndScanner); ui::UpdateWindow(hwndBar);
    dxThumb = (((8) > ((int)((int64_t)(rc.right - 34) * si.nPage / (si.nMax - si.nMin + 1)))) ? (8) : ((int)((int64_t)(rc.right - 34) * si.nPage / (si.nMax - si.nMin + 1))));
    xGrab = 17 + dxThumb / 2;
    ui::SendMessage(hwndBar, win32::WM_LBUTTONDOWN, win32::MK_LBUTTON, win32::MAKELPARAM(xGrab, 8));
    // Each simulated drag tick includes the completed window-tree paint pass.
    if (!FCompatPaintTree(app.lpui->hwndFrame)) return 0;
    cPresentSav = cRenderPresent; nsStart = SDL_GetPerformanceCounter();
    for (i = 1; i <= 10; i++) {
        ui::SendMessage(hwndBar, win32::WM_MOUSEMOVE, win32::MK_LBUTTON, win32::MAKELPARAM(xGrab + i * 4, 8));
        if (win32::GetCapture() != hwndBar || !FCompatPaintTree(app.lpui->hwndFrame)) return 0;
    }
    printf("Scanner drag: %u presentations for 10 updates, %.2f ms\n", cRenderPresent - cPresentSav,
           (double)(SDL_GetPerformanceCounter() - nsStart) * 1000 / SDL_GetPerformanceFrequency());
    fSinglePresent = cRenderPresent - cPresentSav == 10;
    ui::SendMessage(hwndBar, win32::WM_LBUTTONUP, 0, win32::MAKELPARAM(xGrab + 40, 8));
    ui::SendMessage(app.lpui->hwndScanner, win32::WM_HSCROLL, win32::MAKEWPARAM(win32::SB_THUMBPOSITION, iPosSav), 0);
    if (app.state.iScanZoom != iZoomSav) {
        ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, iZoomSav + 3905, 0);
        ui::SendMessage(app.lpui->hwndScanner, win32::WM_HSCROLL, win32::MAKEWPARAM(win32::SB_THUMBPOSITION, xSav), 0);
        ui::SendMessage(app.lpui->hwndScanner, win32::WM_VSCROLL, win32::MAKEWPARAM(win32::SB_THUMBPOSITION, ySav), 0);
    }
    return fSinglePresent;
}

static int iTransportPreset = -1, cTransportMenus;
BOOL StarsTest_TrackPopupMenu(HMENU hmenu, UINT flags, int x, int y, int reserved, HWND hwnd, const RECT *prc) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    if (iRosterMenu >= 0) {
        HMENU hmenuLibrary = win32::GetSubMenu(hmenu, 1);
        char szLabel[512];
        int i, cch;
        const char *pch;
        char *pchLabel;

        if (!win32::GetSubMenu(hmenu, 0) || !hmenuLibrary || !win32::GetSubMenu(hmenu, 2) ||
            win32::GetMenuStringA(hmenuLibrary, 15001, szLabel, sizeof(szLabel), win32::MF_BYCOMMAND) != 0) {
            win32::PostQuitMessage(93);
            return 0;
        }
        app.lpui->iPopMenuSel = iRosterMenu;
        if (pszRosterRace) {
            pchLabel = szLabel;
            for (pch = pszRosterRace; *pch; pch++) {
                *pchLabel++ = *pch;
                if (*pch == '&') *pchLabel++ = '&';
            }
            *pchLabel = 0;
            for (i = 0; i < win32::GetMenuItemCount(hmenuLibrary); i++) {
                char szItem[512];

                cch = win32::GetMenuStringA(hmenuLibrary, i, szItem, sizeof(szItem), win32::MF_BYPOSITION);
                if (cch != 0 && !strcmp(szItem, szLabel)) break;
            }
            if (i == win32::GetMenuItemCount(hmenuLibrary)) { win32::PostQuitMessage(93); return 0; }
            app.lpui->iPopMenuSel = win32::GetMenuItemID(hmenuLibrary, i) - 15000;
            pszRosterRace = NULL;
        }
        iRosterMenu = -1;
        return 1;
    }
    if (iTransportPreset >= 0) {
        SDL_Event event = {};
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
            if (!SDL_PushEvent(&event)) return 0;
            event.type = SDL_EVENT_KEY_UP; event.key.down = false;
            if (!SDL_PushEvent(&event)) return 0;
        }
        fResult = win32::TrackPopupMenu(hmenu, flags, x, y, reserved, hwnd, prc);
        iTransportPreset = -1;
        return fResult;
    }
    if (!fToolbarMenu) return win32::TrackPopupMenu(hmenu, flags, x, y, reserved, hwnd, prc);
    cToolbarMenu++;
    fToolbarAnchor = x == xToolbarMenu && y == yToolbarMenu && !(flags & win32::TPM_CENTERALIGN);
    return 0;
}

static BOOL FCheckMapInspection(APPSESSION &app) {
    SCANSNAPSHOT snap = {}, fresh = {};
    size_t i, j, cPlanetFound = 0;
    POINT16 pt;
    POINT screen;
    SCAN scanSav = app.state.sel.scan;
    SEL selSav = app.state.sel;
    BOOL fPassed = 0;
    HWND hwndInspector, hwndMap, hwndChild, hwndGrandchild;
    char szNameSav[sizeof(fresh.rgobj[0].szName)];
    char szExport[32768];
    size_t cb;
    const char *pszViewerCapture = PszCompatEnvironment("STARS_MAP_VIEWER_CAPTURE");
    NATIVEFILE *lpfile;
    const char *pszCapture = PszCompatEnvironment("STARS_MAP_CAPTURE");
    int16_t fDirty = app.state.game.fDirty;
    if (!FCaptureScanner(app, &snap) || snap.cObject < (size_t)app.state.game.cPlanMax ||
        snap.lidGame != app.state.game.lid || snap.idPlayer != app.state.idPlayer || snap.turn != app.state.game.turn) goto LDone;
    for (i = 0; i < snap.cObject; i++) {
        if (!snap.rgobj[i].szName[0]) goto LDone;
        pt = PointTo16(snap.rgobj[i].ptUniverse); LogicalToScan(app, &pt);
        screen = PointFrom16(pt); win32::ClientToScreen(app.lpui->hwndScanner, &screen);
        if (pt.x != snap.rgobj[i].ptClient.x || pt.y != snap.rgobj[i].ptClient.y ||
            screen.x != snap.rgobj[i].ptScreen.x || screen.y != snap.rgobj[i].ptScreen.y) goto LDone;
        if (snap.rgobj[i].grType == grobjPlanet) cPlanetFound++;
        for (j = i + 1; j < snap.cObject; j++)
            if (snap.rgobj[i].grType == snap.rgobj[j].grType && snap.rgobj[i].id == snap.rgobj[j].id) goto LDone;
    }
    if (cPlanetFound != (size_t)app.state.game.cPlanMax || app.state.game.fDirty != fDirty || memcmp(&app.state.sel, &selSav, sizeof(app.state.sel))) goto LDone;
    snap.turn++;
    if (FSelectScannerObject(app, &snap, grobjPlanet, 0, 1)) goto LDone;
    snap.turn--;
    if (FSelectScannerObject(app, &snap, grobjPlanet, UINT16_MAX, 1) ||
        !FSelectScannerObject(app, &snap, grobjPlanet, 0, 1) || app.state.sel.scan.grobj != grobjPlanet || app.state.sel.scan.idpl != 0) goto LDone;
    if (!FCaptureScanner(app, &fresh) || !fresh.rgobj[0].fOnScreen || !fresh.rgobj[0].fScanSelected || app.state.game.fDirty != fDirty) goto LDone;
    memcpy(szNameSav, fresh.rgobj[0].szName, sizeof(szNameSav));
    strcpy(fresh.rgobj[0].szName, "Quoted \"\xc3\xa9\xe2\x82\xac\"\r\n");
    lpfile = PfNativeTemporary();
    if (!lpfile) goto LDone;
    if (!FWriteScannerSnapshot(&fresh, lpfile)) { ICloseNativeFile(lpfile); goto LDone; }
    ISeekNativeFile(lpfile, 0, NATIVESEEKORIGIN::begin); cb = CReadNative(std::span{szExport}.first(sizeof(szExport) - 1), lpfile); szExport[cb] = 0; ICloseNativeFile(lpfile);
    if (!strstr(szExport, "\xc3\xa9\xe2\x82\xac") || !strstr(szExport, "\\u000d\\u000a")) goto LDone;
    memcpy(fresh.rgobj[0].szName, szNameSav, sizeof(szNameSav));
    if (pszCapture) {
        lpfile = PfNativeOpen(pszCapture, NATIVEFILEMODE::writeTruncate);
        if (!lpfile) goto LDone;
        if (!FWriteScannerSnapshot(&fresh, lpfile)) { ICloseNativeFile(lpfile); goto LDone; }
        if (ICloseNativeFile(lpfile) != 0) goto LDone;
    }
    hwndInspector = StarsCompatShowInspector();
    if (!hwndInspector || !win32::GetDlgItem(hwndInspector,4)) goto LDone;
    ui::SendMessage(win32::GetDlgItem(hwndInspector,4), win32::BM_CLICK,0,0);
    hwndMap = ShowScannerInspector();
    if (!hwndMap || ui::SendMessage(win32::GetDlgItem(hwndMap,1),win32::LB_GETCOUNT,0,0) != (LRESULT)fresh.cObject ||
        !ui::UpdateWindow(hwndMap) || !FCompatPresent(hwndMap)) goto LDone;
    for (hwndChild = win32::GetWindow(hwndMap, win32::GW_CHILD); hwndChild; hwndChild = win32::GetWindow(hwndChild, win32::GW_HWNDNEXT)) {
        ui::UpdateWindow(hwndChild);
        for (hwndGrandchild = win32::GetWindow(hwndChild, win32::GW_CHILD); hwndGrandchild; hwndGrandchild = win32::GetWindow(hwndGrandchild, win32::GW_HWNDNEXT))
            ui::UpdateWindow(hwndGrandchild);
    }
    if (!FCompatPresent(hwndMap) || (pszViewerCapture && !SDL_SaveBMP(hwndMap->lpComposite.get(), pszViewerCapture))) goto LDone;
    ui::SendMessage(win32::GetDlgItem(hwndMap, 1), win32::LB_SETCURSEL, 1, 0);
    ui::SendMessage(win32::GetDlgItem(hwndMap, 4), win32::BM_CLICK, 0, 0);
    if (app.state.sel.scan.grobj != grobjPlanet || app.state.sel.scan.idpl != 1 || app.state.game.fDirty != fDirty) goto LDone;
    ui::SendMessage(hwndMap, win32::WM_CLOSE,0,0);
    if (win32::IsWindow(hwndMap)) goto LDone;
    hwndMap = ShowScannerInspector();
    if (!hwndMap) goto LDone;
    ui::DestroyWindow(hwndMap); ui::DestroyWindow(hwndInspector);
    ChangeScanSel(app, &scanSav,2);
    fPassed = 1;
LDone:
    FreeScannerSnapshot(&snap); FreeScannerSnapshot(&fresh);
    return fPassed;
}

static BOOL FCheckReportScrollbars(APPSESSION &app) {
    SCROLLINFO si = {sizeof(si), win32::SIF_ALL, 0, 0, 0, 0, 0};
    BOOL fPassed = 1;

    ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_REPORT_PLANET, 0);
    if (!win32::IsWindow(app.lpui->hwndReportDlg) || !app.lpui->vprptCur) return 0;
    ui::SetWindowPos(app.lpui->hwndReportDlg, NULL, 0, 0, 300, 220, win32::SWP_NOMOVE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
    if (win32::IsWindowVisible(app.lpui->vprptCur->hwndVScroll)) {
        fPassed = win32::GetScrollInfo(app.lpui->vprptCur->hwndVScroll, win32::SB_CTL, &si) &&
            si.nPage == (UINT)(((1) > (app.lpui->vprptCur->cRowsVis)) ? (1) : (app.lpui->vprptCur->cRowsVis)) &&
            si.nMax - (int)si.nPage + 1 == app.lpui->vprptCur->cRows - app.lpui->vprptCur->cRowsVis;
    }
    if (win32::IsWindowVisible(app.lpui->vprptCur->hwndHScroll)) {
        fPassed = fPassed && win32::GetScrollInfo(app.lpui->vprptCur->hwndHScroll, win32::SB_CTL, &si) &&
            si.nPage != 0 && si.nMax - (int)si.nPage + 1 == app.lpui->vprptCur->cColScroll;
    }
    ui::SendMessage(app.lpui->hwndReportDlg, win32::WM_CLOSE, 0, 0);
    return fPassed && !win32::IsWindow(app.lpui->hwndReportDlg);
}

static BOOL FCheckToolbarPlacement(APPSESSION &app) {
    static const ToolbarButton rgitb[] = {tbMineFields, tbShipDesignFilterMenu, tbEnemyClassFilterMenu, tbZoomMenu};
    POINT pt;
    RECT rc, rcTip, rcDrop, rcClient;
    SCROLLINFO si = {sizeof(si), win32::SIF_ALL, 0, 0, 0, 0, 0};
    int cPage, iMax;
    HWND hwndFocus = win32::GetFocus();
    HWND hwndRoot = HwndCompatRoot(app.lpui->hwndTb);
    HWND hwndButtons;
    char szClass[64];
    SDL_Event event = {};
    int i, x, iRange;

    if (!win32::GetClientRect(app.lpui->hwndScanner, &rcClient) || !win32::GetScrollInfo(app.lpui->hwndScanner, win32::SB_HORZ, &si)) return 0;
    cPage = (((1) > (ScanToPt(app, rcClient.right))) ? (1) : (ScanToPt(app, rcClient.right)));
    iMax = ((((1000) > (app.state.dGalInv - 1000 - cPage)) ? (1000) : (app.state.dGalInv - 1000 - cPage)) + 3) & ~3;
    if (si.nPage != (UINT)cPage || si.nMax - (int)si.nPage + 1 != iMax) return 0;
    if (!win32::GetScrollInfo(app.lpui->hwndScanner, win32::SB_VERT, &si)) return 0;
    cPage = (((1) > (ScanToPt(app, rcClient.bottom - app.lpui->dySBar))) ? (1) : (ScanToPt(app, rcClient.bottom - app.lpui->dySBar)));
    iMax = ((((1000) > (app.state.dGalInv - 1000 - cPage)) ? (1000) : (app.state.dGalInv - 1000 - cPage)) + 3) & ~3;
    if (si.nPage != (UINT)cPage || si.nMax - (int)si.nPage + 1 != iMax) return 0;
    if (!win32::GetWindowRect(app.lpui->hwndTBRadar, &rc)) return 0;
    if (!win32::GetClientRect(app.lpui->hwndTBRadar, &rcClient) || rcClient.right != rc.right - rc.left ||
        app.lpui->hwndTBRadar->rghwndScroll[win32::SB_VERT] || app.lpui->hwndTBRadar->rghwndScroll[win32::SB_HORZ]) return 0;
    pt = (POINT){(rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2};
    hwndButtons = win32::WindowFromPoint(pt);
    if (hwndButtons != app.lpui->hwndTBRadar && win32::GetParent(hwndButtons) != app.lpui->hwndTBRadar) return 0;
    iRange = (int)ui::SendMessage(app.lpui->hwndTBRadar, win32::CB_GETCURSEL, 0, 0);
    ui::SetFocus(app.lpui->hwndTBRadar);
    ui::SendMessage(app.lpui->hwndTBRadar, win32::CB_SHOWDROPDOWN, 1, 0);
    if (!app.lpui->hwndTBRadar->lpcombo || !app.lpui->hwndTBRadar->lpcombo->hwndPopup ||
        !win32::GetWindowRect(app.lpui->hwndTBRadar->lpcombo->hwndPopup, &rcDrop) ||
        rcDrop.left != rc.left || rcDrop.top != rc.bottom ||
        !app.lpui->hwndTBRadar->lpcombo->hwndPopup->lpCanvas.get() ||
        ui::SendMessage(app.lpui->hwndTBRadar, win32::CB_GETCOUNT, 0, 0) < 2) return 0;
    if (strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0 &&
        (!(SDL_GetWindowFlags(app.lpui->hwndTBRadar->lpcombo->hwndPopup->lpWindow.get()) & SDL_WINDOW_POPUP_MENU) ||
         SDL_GetWindowParent(app.lpui->hwndTBRadar->lpcombo->hwndPopup->lpWindow.get()) != hwndRoot->lpWindow.get())) return 0;
    ui::SendMessage(app.lpui->hwndTBRadar, win32::WM_KEYDOWN, win32::VK_DOWN, 0);
    ui::SendMessage(app.lpui->hwndTBRadar, win32::WM_KEYDOWN, win32::VK_RETURN, 0);
    if (ui::SendMessage(app.lpui->hwndTBRadar, win32::CB_GETDROPPEDSTATE, 0, 0) || win32::GetCapture() == app.lpui->hwndTBRadar) return 0;
    ui::SendMessage(app.lpui->hwndTBRadar, win32::CB_SETCURSEL, iRange, 0);
    ui::SetFocus(hwndFocus);
    for (i = 0; i < 4; i++) {
        x = rgitb[i];
        if (x == tbShipDesignFilterMenu) x = tbShipDesignFilter;
        else if (x == tbEnemyClassFilterMenu) x = tbEnemyClassFilter;
        hwndButtons = win32::GetDlgItem(app.lpui->hwndTb, IDC_STARS_TOOLBAR);
        if (!ui::SendMessage(hwndButtons, win32::TB_GETRECT, IDC_STARS_TOOLBUTTON_FIRST + x, (LPARAM)&rc)) return 0;
        pt = (POINT){rc.left, rc.bottom};
        if (!win32::ClientToScreen(hwndButtons, &pt)) return 0;
        xToolbarMenu = pt.x; yToolbarMenu = pt.y;
        cToolbarMenu = 0; fToolbarAnchor = 0; fToolbarMenu = 1;
        if (!win32::GetClassNameA(hwndButtons, szClass, sizeof(szClass)) || strcmp(szClass, win32::TOOLBARCLASSNAMEA)) return 0;
        ui::SendMessage(hwndButtons, win32::WM_LBUTTONDOWN, win32::MK_LBUTTON, win32::MAKELPARAM(rc.right - 3, (rc.top + rc.bottom) / 2));
        ui::SendMessage(hwndButtons, win32::WM_LBUTTONUP, 0, win32::MAKELPARAM(rc.right - 3, (rc.top + rc.bottom) / 2));
        fToolbarMenu = 0;
        if (cToolbarMenu != 1 || !fToolbarAnchor) return 0;
    }
    win32::SetRect(&rc, 4, 4, 33, 32);
    win32::MapWindowPoints(app.lpui->hwndTb, NULL, (POINT *)&rc, 2);
    for (i = 0; i < 2; i++) {
        pt = (POINT){rc.left + 2 + i * 10, rc.top + 2 + i * 10};
        if (!strcmp(SDL_GetCurrentVideoDriver(), "dummy") || !strcmp(SDL_GetCurrentVideoDriver(), "wayland")) {
            if (!win32::ScreenToClient(hwndRoot, &pt)) return 0;
            event.type = SDL_EVENT_MOUSE_MOTION;
            event.motion.windowID = hwndRoot->id;
            event.motion.x = (float)pt.x;
            event.motion.y = (float)(pt.y + CyCompatMenuBar(hwndRoot));
            CompatRouteEvent(&event);
        } else if (!SDL_WarpMouseGlobal((float)pt.x, (float)pt.y)) return 0;
        ShowTooltip(app, idsNormalView, &rc);
        if (!app.lpui->hwndTooltip) return 0;
        TooltipWndProc(app.lpui->hwndTooltip, 1523, 1, 0);
        if (!win32::IsWindowVisible(app.lpui->hwndTooltip) || !win32::GetWindowRect(app.lpui->hwndTooltip, &rcTip) ||
            rcTip.left != rc.left || rcTip.top != rc.bottom + 2 || win32::GetFocus() != hwndFocus) return 0;
        if (strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0 &&
            (!(SDL_GetWindowFlags(app.lpui->hwndTooltip->lpWindow.get()) & SDL_WINDOW_TOOLTIP) ||
             SDL_GetWindowParent(app.lpui->hwndTooltip->lpWindow.get()) != hwndRoot->lpWindow.get())) return 0;
        ShowTooltip(app, (StringId)(idsNoString), NULL);
    }
    return 1;
}

static BOOL fRejectRacePassword;
static BOOL fTestLegacyRaceImport, fAcceptLegacyRacePassword;
static int cLegacyRacePasswords;

int16_t StarsTest_PromptPassword(APPSESSION &app) {
    if (fRejectRacePassword) return 0;
    // The frozen regression race is password protected. These tests qualify
    // file loading and client creation; password validation is outside scope.
    return pszContinue || PszCompatEnvironment("STARS_CLIENT_EXPECT_PLAYER") || PszCompatEnvironment("STARS_CLIENT_HOST_LIBRARY_TEST") || PszCompatEnvironment("STARS_CLIENT_STARTUP_MANAGED") ||
           PszCompatEnvironment("STARS_CLIENT_EXPORT_GAME") ? 1 : PromptPassword(app);
}

int16_t StarsTest_FPromptLegacyRacePassword(APPSESSION &app, char *szPassword) {
    if (!fTestLegacyRaceImport) return FPromptLegacyRacePassword(app, szPassword);
    cLegacyRacePasswords++;
    if (!fAcceptLegacyRacePassword) return 0;
    strcpy(szPassword, "new-\xc3\xa9");
    return 1;
}

BOOL StarsTest_FNativeOpenFileName(OPENFILENAMEA *lpof) {
    const char *pszSource = PszCompatEnvironment("STARS_CLIENT_RACE_IMPORT_SOURCE");
    const char *pszExternalGame = PszCompatEnvironment("STARS_CLIENT_EXTERNAL_OPEN_GAME");
    if (!pszExternalGame) pszExternalGame = PszCompatEnvironment("STARS_CLIENT_EXTERNAL_OPEN_RACE");
    if (PszCompatEnvironment("STARS_CLIENT_STARTUP_SOURCE")) {
        fPassed = 0;
        fprintf(stderr, "Startup unexpectedly reopened the file chooser\n");
        win32::PostQuitMessage(92);
        return 0;
    }
    if (PszCompatEnvironment("STARS_CLIENT_RACE_ROSTER_TEST")) {
        fprintf(stderr, "New-game roster opened a file picker\n");
        win32::PostQuitMessage(93);
        return 0;
    }
    if (pszExternalGame) {
        if (strlen(pszExternalGame) >= lpof->nMaxFile) return 0;
        strcpy(lpof->lpstrFile, pszExternalGame);
        return 1;
    }
    if (PszCompatEnvironment("STARS_CLIENT_RACE_LIBRARY_TEST") && pszSource) {
        if (strlen(pszSource) >= lpof->nMaxFile) return 0;
        strcpy(lpof->lpstrFile, pszSource);
        return 1;
    }
    if (PszCompatEnvironment("STARS_CLIENT_HOST_LIBRARY_TEST")) {
        const char *pszHostOrderPicker;
        cHostOrderPickerCalls++;
        if (fHostOrderPickerCancel) return 0;
        pszHostOrderPicker = PszCompatEnvironment("STARS_CLIENT_HOST_ORDER_PICKER");
        if (pszHostOrderPicker) {
            size_t cchHostOrderPicker = strlen(pszHostOrderPicker);
            if (cchHostOrderPicker >= (size_t)lpof->nMaxFile) return 0;
            memcpy(lpof->lpstrFile, pszHostOrderPicker, cchHostOrderPicker + 1);
            return 1;
        }
        return FNativeOpenFileName(lpof);
    }
    return FNativeOpenFileName(lpof);
}

bool SDL_ShowWindow(SDL_Window *lpWindow);
extern "C" bool SDLCALL StarsTest_SDL_ShowWindow(SDL_Window *lpWindow) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HWND hwnd;
    bool fResult;
    BYTE r, g, b, a;
    COLORREF cr;
    BYTE *rgb;
    size_t cb, i;

    hwnd = LpCompatWindowFromId(SDL_GetWindowID(lpWindow));
    if (hwnd && (hwnd->lpfnDlg == RaceWizardDlg1 || hwnd->lpfnDlg == RaceWizardDlg2 ||
                 hwnd->lpfnDlg == RaceWizardDlg3 || hwnd->lpfnDlg == RaceWizardDlg4 ||
                 hwnd->lpfnDlg == RaceWizardDlg5 || hwnd->lpfnDlg == RaceWizardDlg6)) {
        SDL_PropertiesID idProperties = hwnd->lpTexture.get() ? SDL_GetTextureProperties(hwnd->lpTexture.get()) : 0;
        if (idRacePresentation == hwnd->id && !SDL_GetBooleanProperty(idProperties, "stars.test.race_frame", false)) {
            fprintf(stderr, "Race wizard discarded its displayed frame between pages\n");
            fRacePresentation = 0;
        }
        idRacePresentation = hwnd->id;
        SDL_SetBooleanProperty(idProperties, "stars.test.race_frame", true);
    }
    if (hwnd == app.lpui->hwndFrame && !fInitComplete) cEarlyFrameShow++;
    if (hwnd && hwnd->lpfnWndProc == TitleWndProc && cTitleShow++ == 0) {
        if (hwnd->lpCanvas.get() && hwnd->lpComposite.get()) {
            rgb = (BYTE *)(hwnd->lpCanvas->pixels);
            cb = (size_t)hwnd->lpCanvas->pitch * hwnd->lpCanvas->h;
            for (i = 4; i + 4 <= cb; i += 4) {
                if (memcmp(rgb, rgb + i, 4)) { fTitlePainted = 1; break; }
            }
        }
    }

    for (hwnd = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwnd; hwnd = win32::GetWindow(hwnd, win32::GW_HWNDNEXT)) {
        if (hwnd->lpWindow.get() != lpWindow || hwnd->lpfnDlg != RaceWizardDlg2) continue;
        if (cHabShow++ == 0) {
            cr = win32::GetSysColor(win32::COLOR_BTNFACE);
            fHabPainted = hwnd->lpCanvas.get() && hwnd->lpComposite.get() &&
                SDL_ReadSurfacePixel(hwnd->lpCanvas.get(), 0, 0, &r, &g, &b, &a) &&
                win32::RGB(r, g, b) == cr;
        }
    }
    if (PszCompatEnvironment("STARS_DEBUG_LIBRARY_STARTUP") && hwnd && hwnd->lpfnWndProc == TitleWndProc)
        fprintf(stderr, "[DEBUG-library-startup] SDL_ShowWindow title before visible=%d id=%u\n",
                win32::IsWindowVisible(hwnd), SDL_GetWindowID(lpWindow));
    fResult = SDL_ShowWindow(lpWindow);
    if (PszCompatEnvironment("STARS_DEBUG_LIBRARY_STARTUP") && hwnd && hwnd->lpfnWndProc == TitleWndProc)
        fprintf(stderr, "[DEBUG-library-startup] SDL_ShowWindow title result=%d visible=%d\n",
                fResult, win32::IsWindowVisible(hwnd));
    return fResult;
}

static BOOL FPushClick(APPSESSION &app, HWND hwnd) {
    RECT rc, rcPane;
    POINT pt, ptEnd;
    HWND hwndAncestor, hwndPane;
    int i, iPosition;
    HWND lpTitle = HwndCompatRoot(hwnd);
    SDL_Event event = {};
    for (hwndAncestor = hwnd; hwndAncestor; hwndAncestor = win32::GetParent(hwndAncestor)) {
        if (hwndAncestor != app.lpui->hwndPlanet) continue;
        hwndPane = win32::GetParent(app.lpui->hwndPlanet);
        for (i = 0; i < 2; i++) {
            win32::GetWindowRect(hwnd, &rc); win32::GetClientRect(hwndPane, &rcPane);
            pt = (POINT){rc.left, rc.top}; ptEnd = (POINT){rc.right, rc.bottom};
            win32::ScreenToClient(hwndPane, &pt); win32::ScreenToClient(hwndPane, &ptEnd);
            iPosition = win32::GetScrollPos(hwndPane, i == 0 ? win32::SB_VERT : win32::SB_HORZ);
            if ((i == 0 ? pt.y : pt.x) < 0) iPosition += i == 0 ? pt.y : pt.x;
            else if ((i == 0 ? ptEnd.y : ptEnd.x) > (i == 0 ? rcPane.bottom : rcPane.right))
                iPosition += i == 0 ? ptEnd.y - rcPane.bottom : ptEnd.x - rcPane.right;
            ui::SendMessage(hwndPane, i == 0 ? win32::WM_VSCROLL : win32::WM_HSCROLL, win32::MAKEWPARAM(win32::SB_THUMBPOSITION, (((0) > (iPosition)) ? (0) : (iPosition))), 0);
        }
        break;
    }
    if (!lpTitle || !win32::GetWindowRect(hwnd, &rc)) return 0;
    pt = (POINT){(rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2};
    if (!win32::ScreenToClient(lpTitle, &pt)) return 0;
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.windowID = lpTitle->id;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.down = true;
    event.button.x = (float)pt.x;
    event.button.y = (float)(pt.y + CyCompatMenuBar(lpTitle));
    if (!SDL_PushEvent(&event)) return 0;
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.down = false;
    return SDL_PushEvent(&event);
}

static BOOL FSelectClientRaceName(HWND hwndTitle, const char *pszName) {
    HWND hwndList = win32::GetDlgItem(hwndTitle, IDC_LIBRARY_RACES);
    char szName[256];
    LRESULT i, c;
    c = ui::SendMessage(hwndList, win32::LB_GETCOUNT, 0, 0);
    for (i = 0; i < c; i++) {
        if (ui::SendMessage(hwndList, win32::LB_GETTEXT, i, (LPARAM)szName) >= 0 && !strcmp(szName, pszName))
            return ui::SendMessage(hwndList, win32::LB_SETCURSEL, i, 0) != win32::LB_ERR;
    }
    return 0;
}

static BOOL FSelectClientGameName(HWND hwndTitle, const char *pszName) {
    HWND hwndList = win32::GetDlgItem(hwndTitle, IDC_LIBRARY_GAMES);
    LRESULT i = ui::SendMessage(hwndList, win32::LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pszName);
    if (i == win32::LB_ERR || ui::SendMessage(hwndList, win32::LB_SETCURSEL, i, 0) == win32::LB_ERR) return 0;
    ui::SendMessage(hwndTitle, win32::WM_COMMAND, win32::MAKEWPARAM(IDC_LIBRARY_GAMES, win32::LBN_SELCHANGE), (LPARAM)hwndList);
    return 1;
}

static BOOL FSelectClientLibraryPlayer(HWND hwndTitle, int iplr) {
    HWND hwndPlayers = win32::GetDlgItem(hwndTitle, IDC_LIBRARY_PLAYERS);
    LRESULT i, c = ui::SendMessage(hwndPlayers, win32::CB_GETCOUNT, 0, 0);
    for (i = 0; i < c; i++) {
        if (ui::SendMessage(hwndPlayers, win32::CB_GETITEMDATA, i, 0) != iplr) continue;
        if (ui::SendMessage(hwndPlayers, win32::CB_SETCURSEL, i, 0) == win32::CB_ERR) return 0;
        ui::SendMessage(hwndTitle, win32::WM_COMMAND, win32::MAKEWPARAM(IDC_LIBRARY_PLAYERS, win32::CBN_SELCHANGE), (LPARAM)hwndPlayers);
        return 1;
    }
    return 0;
}

static BOOL FClientLibraryPlayerYear(HWND hwndTitle, int iplr, int year) {
    HWND hwndPlayers = win32::GetDlgItem(hwndTitle, IDC_LIBRARY_PLAYERS);
    LRESULT i, c = ui::SendMessage(hwndPlayers, win32::CB_GETCOUNT, 0, 0);
    char szPlayer[64], szExpected[64];
    CchSnprintf(std::span{szExpected, sizeof(szExpected)}, "Player %d (%d)", iplr + 1, year);
    for (i = 0; i < c; i++) {
        if (ui::SendMessage(hwndPlayers, win32::CB_GETITEMDATA, i, 0) == iplr &&
            ui::SendMessage(hwndPlayers, win32::CB_GETLBTEXT, i, (LPARAM)szPlayer) != win32::CB_ERR && !strcmp(szPlayer, szExpected)) return 1;
    }
    return 0;
}

static BOOL FPushComboRow(HWND hwnd, int iRow) {
    HWND hwndPopup = hwnd->lpcombo ? hwnd->lpcombo->hwndPopup : NULL;
    HWND hwndRoot = HwndCompatRoot(hwndPopup);
    POINT pt;
    RECT rc;
    SDL_Event event = {};
    int cy, iTop;
    if (!hwndPopup || !hwndRoot || !win32::GetClientRect(hwndPopup, &rc)) return 0;
    cy = (int)CompatListProc(hwnd, win32::LB_GETITEMHEIGHT, 0, 0);
    iTop = (int)CompatListProc(hwnd, win32::LB_GETTOPINDEX, 0, 0);
    pt = (POINT){rc.right / 2, 1 + (iRow - iTop) * cy + cy / 2};
    if (cy <= 0 || pt.y < 1 || pt.y >= rc.bottom - 1 || !win32::ClientToScreen(hwndPopup, &pt) ||
        !win32::ScreenToClient(hwndRoot, &pt)) return 0;
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.windowID = hwndRoot->id;
    event.button.button = SDL_BUTTON_LEFT; event.button.down = true;
    event.button.x = (float)pt.x; event.button.y = (float)(pt.y + CyCompatMenuBar(hwndRoot));
    if (!SDL_PushEvent(&event)) return 0;
    event.type = SDL_EVENT_MOUSE_BUTTON_UP; event.button.down = false;
    return SDL_PushEvent(&event);
}

static BOOL FPushListRow(HWND hwnd, int iRow) {
    HWND hwndRoot = HwndCompatRoot(hwnd);
    RECT rc;
    POINT pt;
    SDL_Event event = {};
    if (!hwndRoot || ui::SendMessage(hwnd, win32::LB_GETITEMRECT, iRow, (LPARAM)&rc) == win32::LB_ERR) return 0;
    pt = (POINT){(rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2};
    if (!win32::ClientToScreen(hwnd, &pt) || !win32::ScreenToClient(hwndRoot, &pt)) return 0;
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.windowID = hwndRoot->id;
    event.button.button = SDL_BUTTON_LEFT; event.button.down = true;
    event.button.x = (float)pt.x; event.button.y = (float)(pt.y + CyCompatMenuBar(hwndRoot));
    if (!SDL_PushEvent(&event)) return 0;
    event.type = SDL_EVENT_MOUSE_BUTTON_UP; event.button.down = false;
    return SDL_PushEvent(&event);
}

static BOOL FPushClientRect(HWND hwnd, const RECT *lprc, Uint8 button) {
    HWND hwndRoot = HwndCompatRoot(hwnd);
    RECT rc;
    POINT pt;
    SDL_Event event = {};
    memcpy(&rc, lprc, sizeof(rc));
    pt = (POINT){(rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2};
    if (!hwndRoot || !win32::ClientToScreen(hwnd, &pt) || !win32::ScreenToClient(hwndRoot, &pt)) return 0;
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.windowID = hwndRoot->id;
    event.button.button = button; event.button.down = true;
    event.button.x = (float)pt.x; event.button.y = (float)(pt.y + CyCompatMenuBar(hwndRoot));
    if (!SDL_PushEvent(&event)) return 0;
    event.type = SDL_EVENT_MOUSE_BUTTON_UP; event.button.down = false;
    return SDL_PushEvent(&event);
}

static int iWorkflow, iWorkflowStep, cWorkflowWait, iResearchTarget, cProductionBefore;
static BOOL fWorkflowPassed, fWorkflowBusy;
static uint64_t idWorkflowDialog;

static HWND HwndWorkflowDialog(APPSESSION &app) {
    STARSCOMPATSNAPSHOT snap = {};
    uint64_t idOwner = StarsCompatWindowId(app.lpui->hwndFrame), idDialog = 0;
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

static int CProductionItems(APPSESSION &app) {
    int i, c = 0;
    if (!app.state.lpplProdGlob) return -1;
    for (i = 0; i < app.state.lpplProdGlob->iprodMac; i++) c += app.state.lpplProdGlob->rgprod()[i].cItem;
    return c;
}

static int ishdefWorkflow, cShdefWorkflow;
static const char szCopyName[] = "Compat Copy";
static const char szRenameName[] = "Compat Renamed";

static BOOL FCheckShipWorkflow(APPSESSION &app, HWND hwndDialog) {
    HWND hwndControl;
    SDL_Event event = {};
    LPCSTR pszName = iWorkflow == 10 ? "Discarded edit" : iWorkflow == 11 ? szRenameName : szCopyName;
    int iStep = iWorkflowStep, iRow;
    char sz[32];
    BOOL fCopy = iWorkflow == 8 || iWorkflow == 9;
    if (hwndDialog->lpfnDlg != SlotDlg || app.state.fStarbaseMode) return 0;
    if (!fCopy && iStep < 3) {
        hwndControl = HwndWorkflowControl(hwndDialog, IDC_COMBOBOX, "ComboBox");
        if (!hwndControl || app.lpui->mdBuild != mdBuildShdef) return 0;
        if (iStep == 0) return FPushClick(app, hwndControl);
        if (iStep == 1) {
            iRow = (int)ui::SendMessage(hwndControl, win32::CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)app.state.rgshdef[ishdefWorkflow].hul.szClass);
            if (iRow < 0 || !ui::SendMessage(hwndControl, win32::CB_GETDROPPEDSTATE, 0, 0)) return 0;
            CompatListProc(hwndControl, win32::LB_SETTOPINDEX, iRow, 0);
            return FPushComboRow(hwndControl, iRow);
        }
        if (!app.lpui->lpshdefBuild || app.lpui->lpshdefBuild != &app.state.rgshdef[ishdefWorkflow]) return 0;
        hwndControl = HwndWorkflowControl(hwndDialog, iWorkflow == 12 ? IDC_DELETE : IDC_EDIT, "Button");
        return hwndControl && win32::IsWindowEnabled(hwndControl) && FPushClick(app, hwndControl);
    }
    if (!fCopy) iStep -= 2;
    if (iWorkflow == 12) {
        if (!app.state.rgshdef[ishdefWorkflow].fFree || app.state.rgplr[app.state.idPlayer].cShDef != cShdefWorkflow) return 0;
        hwndControl = HwndWorkflowControl(hwndDialog, IDCANCEL, "Button");
        fWorkflowPassed = hwndControl && FPushClick(app, hwndControl);
        return fWorkflowPassed;
    }
    if (iStep == 0) {
        if (app.lpui->mdBuild != mdBuildShdef || app.state.rgplr[app.state.idPlayer].cShDef != cShdefWorkflow) return 0;
        hwndControl = HwndWorkflowControl(hwndDialog, IDC_IMPORT, "Button");
        return hwndControl && win32::IsWindowEnabled(hwndControl) && FPushClick(app, hwndControl);
    }
    if (iStep <= 3) {
        if (app.lpui->mdBuild != mdBuildEdit || app.lpui->ishdefBuild != ishdefWorkflow || app.lpui->lpshdefBuild != &app.state.shdefBuild) return 0;
        hwndControl = HwndWorkflowControl(hwndDialog, IDC_EDITNAME, "Edit");
        if (!hwndControl || !win32::IsWindowVisible(hwndControl)) return 0;
        if (iStep == 1) return FPushClick(app, hwndControl);
        if (iStep == 2) {
            if (win32::GetFocus() != hwndControl) return 0;
            ui::SendMessage(hwndControl, win32::EM_SETSEL, 0, -1);
            event.type = SDL_EVENT_TEXT_INPUT;
            event.text.windowID = HwndCompatRoot(hwndControl)->id;
            event.text.text = pszName;
            return SDL_PushEvent(&event);
        }
        win32::GetWindowTextA(hwndControl, sz, sizeof(sz));
        if (strcmp(sz, pszName) || strcmp(app.state.shdefBuild.hul.szClass, pszName)) return 0;
        hwndControl = HwndWorkflowControl(hwndDialog, iWorkflow == 8 || iWorkflow == 10 ? IDCANCEL : IDOK, "Button");
        return hwndControl && win32::IsWindowVisible(hwndControl) && FPushClick(app, hwndControl);
    }
    if (app.lpui->mdBuild != mdBuildShdef) return 0;
    if (iWorkflow == 8) {
        if (!app.state.rgshdef[ishdefWorkflow].fFree || app.state.rgplr[app.state.idPlayer].cShDef != cShdefWorkflow) return 0;
    } else if (app.state.rgshdef[ishdefWorkflow].fFree || app.state.rgplr[app.state.idPlayer].cShDef != cShdefWorkflow + 1 ||
               strcmp(app.state.rgshdef[ishdefWorkflow].hul.szClass, iWorkflow == 11 ? szRenameName : szCopyName)) return 0;
    hwndControl = HwndWorkflowControl(hwndDialog, IDCANCEL, "Button");
    fWorkflowPassed = hwndControl && FPushClick(app, hwndControl);
    return fWorkflowPassed;
}

static BOOL FCheckHullWorkflow(APPSESSION &app, HWND hwndDialog);

static BOOL FCheckMergeWorkflow(APPSESSION &app, HWND hwndDialog) {
    HWND hwndList = HwndWorkflowControl(hwndDialog, IDC_MERGE_FLEETS_LIST, "ListBox"), hwndControl;
    int cSel;
    if (hwndDialog->lpfnDlg != MergeFleetsDlg || !hwndList || app.state.vcflMerge < 2 ||
        ui::SendMessage(hwndList, win32::LB_GETCOUNT, 0, 0) != app.state.vcflMerge) return 0;
    cSel = (int)ui::SendMessage(hwndList, win32::LB_GETSELCOUNT, 0, 0);
    if (iWorkflowStep == 0 || iWorkflowStep == 1) {
        if (iWorkflowStep == 1 && cSel != app.state.vcflMerge) return 0;
        hwndControl = HwndWorkflowControl(hwndDialog, iWorkflowStep == 0 ? IDC_MERGE_FLEETS_SELECT_ALL : IDC_MERGE_FLEETS_UNSELECT_ALL, "Button");
        return hwndControl && FPushClick(app, hwndControl);
    }
    if (iWorkflowStep == 2) return cSel == 0 && FPushListRow(hwndList, 0);
    if (iWorkflowStep == 3) {
        if (cSel != 1 || !ui::SendMessage(hwndList, win32::LB_GETSEL, 0, 0) || ui::SendMessage(hwndList, win32::LB_GETSEL, 1, 0)) return 0;
        if (iWorkflow == 14) return FPushListRow(hwndList, 1);
    } else if (cSel != 2 || !ui::SendMessage(hwndList, win32::LB_GETSEL, 0, 0) || !ui::SendMessage(hwndList, win32::LB_GETSEL, 1, 0)) return 0;
    hwndControl = HwndWorkflowControl(hwndDialog, iWorkflow == 13 ? IDCANCEL : IDOK, "Button");
    fWorkflowPassed = hwndControl && FPushClick(app, hwndControl);
    return fWorkflowPassed;
}

static int ishXferWorkflow, cXferWorkflow, idXferWorkflow;

static BOOL FCheckSplitWorkflow(APPSESSION &app, HWND hwndDialog) {
    HWND hwndControl;
    int i;
    if (hwndDialog->lpfnDlg != TransferDlg || app.lpui->mdXferDlg != mdXferShips || !app.state.pxfer || !app.lpui->rgbtnXfer ||
        app.state.pxfer[0].id != app.state.sel.fl.id || app.state.pxfer[1].grobj != grobjFleet) return 0;
    if (iWorkflowStep == 0) {
        for (i = 0; i < app.lpui->crgbtnXfer; i++) {
            if (app.lpui->rgbtnXfer[i].fVisible && !(app.lpui->rgbtnXfer[i].bt & 4) && app.lpui->rgbtnXfer[i].iSide == 1 &&
                app.lpui->rgbtnXfer[i].iVal >= 0 && app.lpui->rgbtnXfer[i].iVal < 16 && app.state.pxfer[0].fl().rgcsh[app.lpui->rgbtnXfer[i].iVal] > 0) break;
        }
        if (i == app.lpui->crgbtnXfer) return 0;
        ishXferWorkflow = app.lpui->rgbtnXfer[i].iVal;
        cXferWorkflow = app.state.pxfer[0].fl().rgcsh[ishXferWorkflow];
        idXferWorkflow = app.state.pxfer[1].id;
        return app.state.pxfer[1].fl().rgcsh[ishXferWorkflow] == 0 && FPushClientRect(hwndDialog, &app.lpui->rgbtnXfer[i].rc, SDL_BUTTON_LEFT);
    }
    if (app.state.pxfer[1].fl().rgcsh[ishXferWorkflow] != 1 || app.state.pxfer[0].fl().rgcsh[ishXferWorkflow] != cXferWorkflow - 1) return 0;
    hwndControl = HwndWorkflowControl(hwndDialog, iWorkflow == 15 ? IDCANCEL : IDOK, "Button");
    fWorkflowPassed = hwndControl && FPushClick(app, hwndControl);
    return fWorkflowPassed;
}

static int iSupplyWorkflow, cCargoFleetWorkflow, cCargoPlanetWorkflow;

static BOOL FCheckCargoWorkflow(APPSESSION &app, HWND hwndDialog) {
    HWND hwndControl;
    int i, iSide = iWorkflow == 19 ? 1 : 0;
    int dCargo = iSide == 0 ? 1 : -1;
    if (hwndDialog->lpfnDlg != TransferDlg || app.lpui->mdXferDlg != mdXferCargo || !app.state.pxfer || !app.lpui->rgbtnXfer ||
        app.state.pxfer[0].id != app.state.sel.fl.id || app.state.pxfer[0].grobj != grobjFleet || app.state.pxfer[1].grobj != grobjPlanet ||
        app.state.pxfer[1].id != app.state.sel.fl.idPlanet) return 0;
    if (iWorkflowStep == 0) {
        for (i = 0; i < app.lpui->crgbtnXfer; i++) {
            if (app.lpui->rgbtnXfer[i].fVisible && !(app.lpui->rgbtnXfer[i].bt & 4) && app.lpui->rgbtnXfer[i].iSide == iSide &&
                app.lpui->rgbtnXfer[i].iVal >= 0 && app.lpui->rgbtnXfer[i].iVal < 3 &&
                (iWorkflow != 19 || app.lpui->rgbtnXfer[i].iVal == iSupplyWorkflow)) break;
        }
        if (i == app.lpui->crgbtnXfer) return 0;
        iSupplyWorkflow = app.lpui->rgbtnXfer[i].iVal;
        cCargoFleetWorkflow = app.state.pxfer[0].fl().rgwtMin[iSupplyWorkflow];
        cCargoPlanetWorkflow = app.state.pxfer[1].pl().rgwtMin[iSupplyWorkflow];
        return FPushClientRect(hwndDialog, &app.lpui->rgbtnXfer[i].rc, SDL_BUTTON_LEFT);
    }
    if (app.state.pxfer[0].fl().rgwtMin[iSupplyWorkflow] != cCargoFleetWorkflow + dCargo ||
        app.state.pxfer[1].pl().rgwtMin[iSupplyWorkflow] != cCargoPlanetWorkflow - dCargo) return 0;
    hwndControl = HwndWorkflowControl(hwndDialog, iWorkflow == 17 ? IDCANCEL : IDOK, "Button");
    fWorkflowPassed = hwndControl && FPushClick(app, hwndControl);
    return fWorkflowPassed;
}

static BOOL FPostOwnedTestFolder(HWND hwnd, char *pszFolder) {
    STARSCOMPATPAYLOAD folder(pszFolder,{[](void *pv) noexcept { SDL_free(pv); }});
    return StarsCompatPostOwnedMessageAsync(StarsCompatWindowId(hwnd),WM_STARS_IMPORT_FOLDER,std::move(folder),0);
}

static void  CheckWorkflow(HWND hwnd, UINT msg, UINT_PTR id, DWORD dwTime) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HWND hwndDialog, hwndControl;
    (void)hwnd; (void)msg; (void)dwTime;
    if (fWorkflowBusy) return;
    fWorkflowBusy = 1;
    hwndDialog = HwndWorkflowDialog(app);
    if (++cWorkflowWait > 120) goto LFail;
    if (!hwndDialog && !idWorkflowDialog && !fWorkflowPassed) { fWorkflowBusy = 0; return; }
    if (!hwndDialog || win32::IsWindowEnabled(app.lpui->hwndFrame)) goto LFail;
    idWorkflowDialog = StarsCompatWindowId(hwndDialog);
    if (fWorkflowPassed) { fWorkflowBusy = 0; return; }
    fprintf(stderr, "Gameplay workflow %d step %d\n", iWorkflow, iWorkflowStep);
    if (iWorkflow == 1) {
        if (hwndDialog->lpfnDlg != ResearchDlg) goto LFail;
        if (iWorkflowStep == 0) {
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_RESEARCH_ENERGY + app.state.rgplr[app.state.idPlayer].iTechNow(), "Button");
            if (!hwndControl || ui::SendMessage(hwndControl, win32::BM_GETCHECK, 0, 0) != win32::BST_CHECKED) goto LFail;
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_RESEARCH_NEXT_FIELD, "ComboBox");
            if (!hwndControl || ui::SendMessage(hwndControl, win32::CB_GETCOUNT, 0, 0) != 8) goto LFail;
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_RESEARCH_ENERGY + iResearchTarget, "Button");
            if (!hwndControl || !win32::IsWindowEnabled(hwndControl) || !FPushClick(app, hwndControl)) goto LFail;
        } else {
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_RESEARCH_ENERGY + iResearchTarget, "Button");
            if (app.lpui->iResTechNow != iResearchTarget || ui::SendMessage(hwndControl, win32::BM_GETCHECK, 0, 0) != win32::BST_CHECKED) goto LFail;
            hwndControl = HwndWorkflowControl(hwndDialog, IDCANCEL, "Button");
            if (!hwndControl || !FPushClick(app, hwndControl)) goto LFail;
            fWorkflowPassed = 1;
        }
    } else if (iWorkflow == 5) {
        if (hwndDialog->lpfnDlg != BattlePlansDlg || app.lpui->iPlanSelDlg != 0) { fprintf(stderr, "Battle dialog: plan %d handler %d\n", app.lpui->iPlanSelDlg, hwndDialog->lpfnDlg == BattlePlansDlg); goto LFail; }
        hwndControl = HwndWorkflowControl(hwndDialog, IDC_BATTLE_PLAN_DUMP_CARGO, "Button");
        if (!hwndControl) goto LFail;
        if (iWorkflowStep == 0) {
            if (app.lpui->btlplan.fDumpCargo != app.state.rglpbtlplan[app.state.idPlayer][0].fDumpCargo ||
                ui::SendMessage(hwndControl, win32::BM_GETCHECK, 0, 0) != app.lpui->btlplan.fDumpCargo || !FPushClick(app, hwndControl)) goto LFail;
        } else {
            if (app.lpui->btlplan.fDumpCargo != iResearchTarget || !app.lpui->fDirtyPlan) { fprintf(stderr, "Dump cargo: value %d target %d check %ld dirty %d style %lx\n", app.lpui->btlplan.fDumpCargo, iResearchTarget, (long)ui::SendMessage(hwndControl, win32::BM_GETCHECK, 0, 0), app.lpui->fDirtyPlan, (unsigned long)win32::GetWindowLongPtrA(hwndControl, win32::GWL_STYLE)); goto LFail; }
            hwndControl = HwndWorkflowControl(hwndDialog, IDOK, "Button");
            if (!hwndControl || !FPushClick(app, hwndControl)) goto LFail;
            fWorkflowPassed = 1;
        }
    } else if (iWorkflow == 6) {
        if (hwndDialog->lpfnDlg != RelationsDlg) goto LFail;
        if (iWorkflowStep == 0) {
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_RELATIONS_PLAYER_LIST, "ListBox");
            if (!hwndControl || ui::SendMessage(hwndControl, win32::LB_GETCOUNT, 0, 0) != app.state.game.cPlayer - 1 ||
                ui::SendMessage(hwndControl, win32::LB_GETCURSEL, 0, 0) != 0) goto LFail;
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_RELATIONS_NEUTRAL + app.state.rgplr[app.state.idPlayer].rgmdRelation[app.state.idPlayer == 0], "Button");
            if (!hwndControl || ui::SendMessage(hwndControl, win32::BM_GETCHECK, 0, 0) != win32::BST_CHECKED) goto LFail;
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_RELATIONS_NEUTRAL + iResearchTarget, "Button");
            if (!hwndControl || !FPushClick(app, hwndControl)) goto LFail;
        } else {
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_RELATIONS_NEUTRAL + iResearchTarget, "Button");
            if (!hwndControl || ui::SendMessage(hwndControl, win32::BM_GETCHECK, 0, 0) != win32::BST_CHECKED ||
                app.state.rgplr[app.state.idPlayer].rgmdRelation[app.state.idPlayer == 0] != iResearchTarget || !app.lpui->fDirtyPlan) goto LFail;
            hwndControl = HwndWorkflowControl(hwndDialog, IDCANCEL, "Button");
            if (!hwndControl || !FPushClick(app, hwndControl)) goto LFail;
            fWorkflowPassed = 1;
        }
    } else if (iWorkflow == 7) {
        if (hwndDialog->lpfnDlg != BattlePlansDlg || app.lpui->iPlanSelDlg != 0) { fprintf(stderr, "Battle dialog: plan %d handler %d\n", app.lpui->iPlanSelDlg, hwndDialog->lpfnDlg == BattlePlansDlg); goto LFail; }
        hwndControl = HwndWorkflowControl(hwndDialog, IDC_BATTLE_PLAN_TACTIC, "ComboBox");
        if (!hwndControl) goto LFail;
        if (iWorkflowStep == 0) {
            if (ui::SendMessage(hwndControl, win32::CB_GETCURSEL, 0, 0) != app.state.rglpbtlplan[app.state.idPlayer][0].mdTactic ||
                ui::SendMessage(hwndControl, win32::CB_GETCOUNT, 0, 0) <= iResearchTarget || !FPushClick(app, hwndControl)) goto LFail;
        } else if (iWorkflowStep == 1) {
            if (!ui::SendMessage(hwndControl, win32::CB_GETDROPPEDSTATE, 0, 0) ||
                app.lpui->btlplan.mdTactic != app.state.rglpbtlplan[app.state.idPlayer][0].mdTactic || !FPushComboRow(hwndControl, iResearchTarget)) goto LFail;
        } else {
            if (ui::SendMessage(hwndControl, win32::CB_GETDROPPEDSTATE, 0, 0) || app.lpui->btlplan.mdTactic != iResearchTarget || !app.lpui->fDirtyPlan) goto LFail;
            hwndControl = HwndWorkflowControl(hwndDialog, IDOK, "Button");
            if (!hwndControl || !FPushClick(app, hwndControl)) goto LFail;
            fWorkflowPassed = 1;
        }
    } else if (iWorkflow == 13 || iWorkflow == 14) {
        if (!FCheckMergeWorkflow(app, hwndDialog)) goto LFail;
    } else if (iWorkflow == 15 || iWorkflow == 16) {
        if (!FCheckSplitWorkflow(app, hwndDialog)) goto LFail;
    } else if (iWorkflow >= 17 && iWorkflow <= 19) {
        if (!FCheckCargoWorkflow(app, hwndDialog)) goto LFail;
    } else if (iWorkflow >= 20) {
        if (!FCheckHullWorkflow(app, hwndDialog)) goto LFail;
    } else if (iWorkflow >= 8) {
        if (!FCheckShipWorkflow(app, hwndDialog)) goto LFail;
    } else {
        if (hwndDialog->lpfnDlg != ProductionDlg || !app.state.lpplProdGlob) goto LFail;
        if (iWorkflowStep == 0) {
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_PRODUCTION_AVAILABLE_ITEMS, "ListBox");
            if (!hwndControl || ui::SendMessage(hwndControl, win32::LB_GETCOUNT, 0, 0) <= 0 ||
                ui::SendMessage(hwndControl, win32::LB_GETCURSEL, 0, 0) != 0) goto LFail;
            cProductionBefore = CProductionItems(app);
            if (iWorkflow == 4) {
                if (cProductionBefore != iResearchTarget) goto LFail;
                hwndControl = HwndWorkflowControl(hwndDialog, IDCANCEL, "Button");
                if (!hwndControl || !FPushClick(app, hwndControl)) goto LFail;
                fWorkflowPassed = 1;
                iWorkflowStep++; fWorkflowBusy = 0; return;
            }
            hwndControl = HwndWorkflowControl(hwndDialog, IDC_PRODUCTION_ADD, "Button");
            if (!hwndControl || !win32::IsWindowEnabled(hwndControl) || !FPushClick(app, hwndControl)) goto LFail;
        } else {
            if (CProductionItems(app) != cProductionBefore + 1) goto LFail;
            hwndControl = HwndWorkflowControl(hwndDialog, iWorkflow == 3 ? IDOK : IDCANCEL, "Button");
            if (!hwndControl || !FPushClick(app, hwndControl)) goto LFail;
            fWorkflowPassed = 1;
        }
    }
    iWorkflowStep++; fWorkflowBusy = 0;
    return;
LFail:
    fprintf(stderr, "Gameplay workflow %d step %d failed\n", iWorkflow, iWorkflowStep);
    win32::KillTimer(NULL, id); fWorkflowBusy = fWorkflowPassed = 0;
    if (hwndDialog) win32::EndDialog(hwndDialog, 0);
    else win32::PostQuitMessage(93);
}

static BOOL FRunWorkflow(APPSESSION &app, int iKind, int iTarget) {
    UINT_PTR id;
    BOOL fPassed;
    win32::MSG msg;
    HWND hwndTrigger;
    fprintf(stderr, "Starting gameplay workflow %d\n", iKind);
    iWorkflow = iKind; iResearchTarget = iTarget; iWorkflowStep = cWorkflowWait = 0;
    fWorkflowPassed = fWorkflowBusy = 0; idWorkflowDialog = 0;
    id = win32::SetTimer(NULL, 0, 50, CheckWorkflow);
    if (!id) return 0;
    if (iKind >= 13 && iKind <= 19) {
        hwndTrigger = app.lpui->rghwndBtn[iKind <= 14 ? 10 : iKind <= 16 ? 8 : 7];
        ui::UpdateWindow(win32::GetParent(hwndTrigger));
        if (!win32::IsWindowVisible(hwndTrigger) || !win32::IsWindowEnabled(hwndTrigger) || !FPushClick(app, hwndTrigger)) {
            fprintf(stderr, "Fleet trigger: visible %d enabled %d fleet %d point %d,%d\n", win32::IsWindowVisible(hwndTrigger), win32::IsWindowEnabled(hwndTrigger), app.state.sel.fl.id, app.state.sel.fl.pt.x, app.state.sel.fl.pt.y);
            win32::KillTimer(NULL, id); iWorkflow = 0; return 0;
        }
        while (!fWorkflowPassed || HwndWorkflowDialog(app) || !win32::IsWindowEnabled(app.lpui->hwndFrame)) {
            if (ui::GetMessage(&msg, NULL, 0, 0) <= 0) break;
            win32::TranslateMessage(&msg); ui::DispatchMessage(&msg);
        }
    }
    else if (iKind == 1) ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_GAME_RESEARCH, 0);
    else if (iKind == 5 || iKind == 7) ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_GAME_BATTLE_PLANS1, 0);
    else if (iKind == 6) ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_GAME_RELATIONS, 0);
    else if (iKind >= 8) ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_GAME_SHIP_BUILDER, 0);
    else ui::SendMessage(app.lpui->hwndFrame, win32::WM_CHAR, 'q', 1);
    win32::KillTimer(NULL, id);
    fPassed = fWorkflowPassed && win32::IsWindowEnabled(app.lpui->hwndFrame) && !HwndWorkflowDialog(app) &&
              idWorkflowDialog && !StarsCompatResolveWindow(idWorkflowDialog);
    if (!fPassed) fprintf(stderr, "Workflow %d returned: passed %d enabled %d dialog %p\n", iKind, fWorkflowPassed, win32::IsWindowEnabled(app.lpui->hwndFrame), (void *)HwndWorkflowDialog(app));
    iWorkflow = 0;
    return fPassed;
}

static int CFleetTotals(APPSESSION &app, int64_t *rgShip, int64_t *rgCargo) {
    int i, j, c = 0;
    FLEET *lpfl;
    memset(rgShip, 0, 16 * sizeof(*rgShip)); memset(rgCargo, 0, 5 * sizeof(*rgCargo));
    for (i = 0; i < app.state.cFleet; i++) {
        lpfl = app.state.rglpfl[i];
        if (!lpfl || lpfl->fDead || lpfl->iPlayer != app.state.idPlayer) continue;
        c++;
        for (j = 0; j < 16; j++) rgShip[j] += lpfl->rgcsh[j];
        for (j = 0; j < 5; j++) rgCargo[j] += lpfl->rgwtMin[j];
    }
    return c;
}

static BOOL FCheckRepeatOrders(APPSESSION &app, int idFleet) {
    win32::MSG msg;
    UINT_PTR id;
    Uint64 ullStart;
    BOOL fRepeat = app.state.sel.fl.fRepOrders, fPassed = 0;
    FLEET *lpfl;
    int i;
    ui::UpdateWindow(win32::GetParent(app.lpui->hwndRepCB));
    if (!win32::IsWindowVisible(app.lpui->hwndRepCB) || !win32::IsWindowEnabled(app.lpui->hwndRepCB) ||
        ui::SendMessage(app.lpui->hwndRepCB, win32::BM_GETCHECK, 0, 0) != fRepeat) {
        fprintf(stderr, "Repeat initial visible %d enabled %d check %ld expected %d\n", win32::IsWindowVisible(app.lpui->hwndRepCB), win32::IsWindowEnabled(app.lpui->hwndRepCB), (long)ui::SendMessage(app.lpui->hwndRepCB, win32::BM_GETCHECK, 0, 0), fRepeat);
        return 0;
    }
    id = win32::SetTimer(NULL, 0, 25, NULL);
    if (!id) return 0;
    for (i = 0; i < 2; i++) {
        if (!FPushClick(app, app.lpui->hwndRepCB)) goto LDone;
        ullStart = SDL_GetTicks();
        while (app.state.sel.fl.fRepOrders != (i == 0 ? !fRepeat : fRepeat) && SDL_GetTicks() - ullStart < 3000) {
            if (ui::GetMessage(&msg, NULL, 0, 0) <= 0) goto LDone;
            win32::TranslateMessage(&msg); ui::DispatchMessage(&msg);
        }
        lpfl = LpflFromId(app, (int16_t)idFleet);
        if (!lpfl || lpfl->fRepOrders != (i == 0 ? !fRepeat : fRepeat) ||
            app.state.sel.fl.fRepOrders != lpfl->fRepOrders || ui::SendMessage(app.lpui->hwndRepCB, win32::BM_GETCHECK, 0, 0) != lpfl->fRepOrders) goto LDone;
        SelectAdjFleet(app, 0, (int16_t)idFleet);
        ui::UpdateWindow(win32::GetParent(app.lpui->hwndRepCB));
        if (ui::SendMessage(app.lpui->hwndRepCB, win32::BM_GETCHECK, 0, 0) != lpfl->fRepOrders) goto LDone;
    }
    fPassed = 1;
LDone:
    if (!fPassed) fprintf(stderr, "Repeat step %d check %ld selected %d expected %d\n", i, (long)ui::SendMessage(app.lpui->hwndRepCB, win32::BM_GETCHECK, 0, 0), app.state.sel.fl.fRepOrders, i == 0 ? !fRepeat : fRepeat);
    win32::KillTimer(NULL, id);
    return fPassed;
}

static BOOL FChooseWorkflowCombo(APPSESSION &app, HWND hwnd, int iRow) {
    win32::MSG msg;
    UINT_PTR id;
    Uint64 ullStart;
    BOOL fPassed = 0;
    if (!win32::IsWindowVisible(hwnd) || !win32::IsWindowEnabled(hwnd) || ui::SendMessage(hwnd, win32::CB_GETCOUNT, 0, 0) <= iRow) return 0;
    id = win32::SetTimer(NULL, 0, 25, NULL);
    if (!id) return 0;
    if (!FPushClick(app, hwnd)) goto LDone;
    ullStart = SDL_GetTicks();
    while (!ui::SendMessage(hwnd, win32::CB_GETDROPPEDSTATE, 0, 0) && SDL_GetTicks() - ullStart < 3000) {
        if (ui::GetMessage(&msg, NULL, 0, 0) <= 0) goto LDone;
        win32::TranslateMessage(&msg); ui::DispatchMessage(&msg);
    }
    if (!ui::SendMessage(hwnd, win32::CB_GETDROPPEDSTATE, 0, 0) || !FPushComboRow(hwnd, iRow)) goto LDone;
    ullStart = SDL_GetTicks();
    while (ui::SendMessage(hwnd, win32::CB_GETDROPPEDSTATE, 0, 0) && SDL_GetTicks() - ullStart < 3000) {
        if (ui::GetMessage(&msg, NULL, 0, 0) <= 0) goto LDone;
        win32::TranslateMessage(&msg); ui::DispatchMessage(&msg);
    }
    fPassed = !ui::SendMessage(hwnd, win32::CB_GETDROPPEDSTATE, 0, 0) && ui::SendMessage(hwnd, win32::CB_GETCURSEL, 0, 0) == iRow && !HwndWorkflowDialog(app);
LDone:
    win32::KillTimer(NULL, id);
    return fPassed;
}

static BOOL FCheckTransportPreset(APPSESSION &app, int iPreset) {
    win32::MSG msg;
    UINT_PTR id;
    Uint64 ullStart;
    BOOL fPassed = 0;
    int cBefore = cTransportMenus;
    if (app.lpui->rgrcRef[5].bottom <= app.lpui->rgrcRef[5].top) return 0;
    id = win32::SetTimer(NULL, 0, 25, NULL);
    if (!id) return 0;
    iTransportPreset = iPreset;
    if (!FPushClientRect(app.lpui->hwndPlanet, &app.lpui->rgrcRef[5], SDL_BUTTON_RIGHT)) goto LDone;
    ullStart = SDL_GetTicks();
    while ((cTransportMenus == cBefore || iTransportPreset >= 0) && SDL_GetTicks() - ullStart < 3000) {
        if (ui::GetMessage(&msg, NULL, 0, 0) <= 0) goto LDone;
        win32::TranslateMessage(&msg); ui::DispatchMessage(&msg);
    }
    fPassed = cTransportMenus == cBefore + 1 && iTransportPreset == -1 && !HwndWorkflowDialog(app);
LDone:
    iTransportPreset = -1;
    win32::KillTimer(NULL, id);
    return fPassed;
}

static BOOL FCheckWaypointTask(APPSESSION &app, int idFleet) {
    FLEET *lpfl;
    ORDER ordSav;
    int i;
    int iwp = app.state.sel.iwpAct;
    if (app.state.sel.grobj != grobjFleet || app.state.sel.fl.id != idFleet || iwp != 0 || app.state.sel.fl.cord == 0) return 0;
    ordSav = app.state.sel.fl.lpplord->rgord()[iwp];
    if (ordSav.grTask != grTaskNone) return 0;
    ui::UpdateWindow(win32::GetParent(app.lpui->rghwndOrderDD[0]));
    if (!FChooseWorkflowCombo(app, app.lpui->rghwndOrderDD[0], grTaskXfer)) return 0;
    lpfl = LpflFromId(app, (int16_t)idFleet);
    if (!lpfl || lpfl->lpplord->rgord()[iwp].grTask != grTaskXfer || app.state.sel.fl.lpplord->rgord()[iwp].grTask != grTaskXfer) return 0;
    if (!FCheckTransportPreset(app, 0)) return 0;
    for (i = 0; i < 5; i++)
        if (lpfl->lpplord->rgord()[iwp].txp.rgia[i].iAction() != (i < 3 ? iActionLoadAll : i == 3 ? iActionNone : iActionLoadDunnage)) return 0;
    if (!FCheckTransportPreset(app, 1)) return 0;
    for (i = 0; i < 5; i++)
        if (lpfl->lpplord->rgord()[iwp].txp.rgia[i].iAction() != (i < 4 ? iActionUnloadAll : iActionLoadDunnage)) return 0;
    if (!FCheckTransportPreset(app, 2)) return 0;
    for (i = 0; i < 5; i++) {
        if (lpfl->lpplord->rgord()[iwp].txp.rgia[i].iAction() != (i < 3 ? iActionWaitPercent : i == 3 ? iActionNone : iActionLoadDunnage)) return 0;
        if (i < 3 && lpfl->lpplord->rgord()[iwp].txp.rgia[i].cQuan() != 100) return 0;
    }
    if (!FCheckTransportPreset(app, 3)) return 0;
    for (i = 0; i < 5; i++)
        if (lpfl->lpplord->rgord()[iwp].txp.rgia[i].iAction() != iActionNone || lpfl->lpplord->rgord()[iwp].txp.rgia[i].cQuan() != 0) return 0;
    SelectAdjFleet(app, 0, (int16_t)idFleet);
    ui::UpdateWindow(win32::GetParent(app.lpui->rghwndOrderDD[0]));
    if (ui::SendMessage(app.lpui->rghwndOrderDD[0], win32::CB_GETCURSEL, 0, 0) != grTaskXfer ||
        !FChooseWorkflowCombo(app, app.lpui->rghwndOrderDD[0], grTaskNone)) return 0;
    SelectAdjFleet(app, 0, (int16_t)idFleet);
    ui::UpdateWindow(win32::GetParent(app.lpui->rghwndOrderDD[0]));
    if (ui::SendMessage(app.lpui->rghwndOrderDD[0], win32::CB_GETCURSEL, 0, 0) != grTaskNone ||
        memcmp(&ordSav, &lpfl->lpplord->rgord()[iwp], sizeof(ordSav))) return 0;
    puts("Waypoint task commit, reselection and restoration passed");
    return 1;
}

static BOOL FPushWorkflowKey(HWND hwnd, SDL_Scancode sc, BOOL fDown) {
    HWND hwndRoot = HwndCompatRoot(hwnd);
    SDL_Event event = {};
    if (!hwndRoot) return 0;
    event.type = fDown ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.windowID = hwndRoot->id; event.key.scancode = sc;
    event.key.key = SDL_GetKeyFromScancode(sc, SDL_KMOD_NONE, false); event.key.down = !!fDown;
    return SDL_PushEvent(&event);
}

static BOOL FPushWorkflowDrag(HWND hwnd, POINT ptFrom, POINT ptTo) {
    HWND hwndRoot = HwndCompatRoot(hwnd);
    SDL_Event event = {};
    if (!hwndRoot || !win32::ClientToScreen(hwnd, &ptFrom) || !win32::ScreenToClient(hwndRoot, &ptFrom) ||
        !win32::ClientToScreen(hwnd, &ptTo) || !win32::ScreenToClient(hwndRoot, &ptTo)) return 0;
    ptFrom.y += CyCompatMenuBar(hwndRoot); ptTo.y += CyCompatMenuBar(hwndRoot);
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.windowID = hwndRoot->id;
    event.button.button = SDL_BUTTON_LEFT; event.button.down = true;
    event.button.x = (float)ptFrom.x; event.button.y = (float)ptFrom.y;
    if (!SDL_PushEvent(&event)) return 0;
    event.type = SDL_EVENT_MOUSE_MOTION; event.motion.windowID = hwndRoot->id;
    event.motion.state = SDL_BUTTON_LMASK; event.motion.x = (float)ptTo.x; event.motion.y = (float)ptTo.y;
    event.motion.xrel = (float)(ptTo.x - ptFrom.x); event.motion.yrel = (float)(ptTo.y - ptFrom.y);
    if (!SDL_PushEvent(&event)) return 0;
    event = (SDL_Event){0}; event.type = SDL_EVENT_MOUSE_BUTTON_UP; event.button.windowID = hwndRoot->id;
    event.button.button = SDL_BUTTON_LEFT; event.button.down = false;
    event.button.x = (float)ptTo.x; event.button.y = (float)ptTo.y;
    return SDL_PushEvent(&event);
}

static void DispatchWorkflowInput(APPSESSION &app, win32::MSG *lpmsg) {
    win32::TranslateMessage(lpmsg);
    if (((lpmsg->message != win32::WM_KEYDOWN && lpmsg->message != win32::WM_KEYUP) ||
        !FHandleKey(app, lpmsg->hwnd, lpmsg->message, lpmsg->wParam, lpmsg->lParam)) &&
        (lpmsg->message != win32::WM_CHAR || !FHandleChar(app, lpmsg->hwnd, lpmsg->wParam, lpmsg->lParam))) ui::DispatchMessage(lpmsg);
}

static BOOL FCheckRouteAppendDelete(APPSESSION &app, int idFleet) {
    SCANSNAPSHOT snap = {};
    SCANOBJECT obj, objDrag;
    POINT ptFrom;
    FLEET *lpfl = LpflFromId(app, (int16_t)idFleet);
    ORDER ordSav;
    RECT rc;
    win32::MSG msg;
    UINT_PTR id;
    Uint64 ullStart;
    size_t i;
    BOOL fPassed = 0;
    if (!lpfl || lpfl->cord != 1) return 0;
    ordSav = lpfl->lpplord->rgord()[0];
    if (!FCaptureScanner(app, &snap)) return 0;
    for (i = 0; i < snap.cObject; i++) {
        obj = snap.rgobj[i];
        if (obj.grType == grobjPlanet && obj.id != lpfl->idPlanet && obj.fOnScreen && !obj.fFiltered &&
            obj.ptClient.x > 3 && obj.ptClient.y > 3 && obj.ptClient.x < snap.rcViewport.right - 3 &&
            obj.ptClient.y < snap.rcViewport.bottom - 3) break;
    }
    fPassed = i < snap.cObject;
    FreeScannerSnapshot(&snap);
    if (!fPassed) return 0;
    fPassed = 0;
    rc = (RECT){obj.ptClient.x - 1, obj.ptClient.y - 1, obj.ptClient.x + 1, obj.ptClient.y + 1};
    id = win32::SetTimer(NULL, 0, 25, NULL);
    if (!id) return 0;
    if (!FPushWorkflowKey(app.lpui->hwndFrame, SDL_SCANCODE_LSHIFT, 1) || !FPushClientRect(app.lpui->hwndScanner, &rc, SDL_BUTTON_LEFT) ||
        !FPushWorkflowKey(app.lpui->hwndFrame, SDL_SCANCODE_LSHIFT, 0)) goto LDone;
    ullStart = SDL_GetTicks();
    while (lpfl->cord != 2 && SDL_GetTicks() - ullStart < 3000) {
        if (ui::GetMessage(&msg, NULL, 0, 0) <= 0) goto LDone;
        DispatchWorkflowInput(app, &msg);
    }
    if (lpfl->cord != 2 || lpfl->lpplord->rgord()[1].grobj != grobjPlanet || lpfl->lpplord->rgord()[1].id != obj.id ||
        lpfl->lpplord->rgord()[1].pt.x != obj.ptUniverse.x || lpfl->lpplord->rgord()[1].pt.y != obj.ptUniverse.y) goto LDone;
    // Re-capture after the append: the UI may have changed the scanner view.
    if (!FCaptureScanner(app, &snap)) goto LDone;
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
    fPassed = 0;
    if (!FPushWorkflowDrag(app.lpui->hwndScanner, ptFrom, objDrag.ptClient)) goto LDone;
    ullStart = SDL_GetTicks();
    while (lpfl->lpplord->rgord()[1].id != objDrag.id && SDL_GetTicks() - ullStart < 3000) {
        if (ui::GetMessage(&msg, NULL, 0, 0) <= 0) goto LDone;
        DispatchWorkflowInput(app, &msg);
    }
    if (lpfl->cord != 2 || lpfl->lpplord->rgord()[1].grobj != grobjPlanet || lpfl->lpplord->rgord()[1].id != objDrag.id ||
        lpfl->lpplord->rgord()[1].pt.x != objDrag.ptUniverse.x || lpfl->lpplord->rgord()[1].pt.y != objDrag.ptUniverse.y) goto LDone;
    ui::UpdateWindow(app.lpui->hwndPlanet);
    if (!FPushListRow(app.lpui->hwndShipLB, 1) || !FPushWorkflowKey(app.lpui->hwndFrame, SDL_SCANCODE_DELETE, 1) ||
        !FPushWorkflowKey(app.lpui->hwndFrame, SDL_SCANCODE_DELETE, 0)) goto LDone;
    ullStart = SDL_GetTicks();
    while (lpfl->cord != 1 && SDL_GetTicks() - ullStart < 3000) {
        if (ui::GetMessage(&msg, NULL, 0, 0) <= 0) goto LDone;
        DispatchWorkflowInput(app, &msg);
    }
    fPassed = lpfl->cord == 1 && app.state.sel.fl.cord == 1 && !memcmp(&ordSav, &lpfl->lpplord->rgord()[0], sizeof(ordSav));
LDone:
    win32::KillTimer(NULL, id);
    if (!fPassed) fprintf(stderr, "Route append/delete failed: count %d selected %d waypoint %d\n", lpfl->cord, app.state.sel.fl.cord, app.state.sel.iwpAct);
    else puts("Scanner Shift-click append, destination drag and waypoint-list Delete passed");
    return fPassed;
}

static BOOL FCheckCargoWorkflows(APPSESSION &app) {
    FLEET *lpfl = NULL;
    PLANET *lppl = NULL;
    int32_t rgCargoFleet[5], rgCargoPlanet[4];
    int i, j, idFleet, idPlanet;
    for (i = 0; i < app.state.cFleet; i++) {
        lpfl = app.state.rglpfl[i];
        if (!lpfl || lpfl->fDead || lpfl->iPlayer != app.state.idPlayer || lpfl->idPlanet == idPlanetDeepSpace || GetCargoFree(app, lpfl) <= 0) continue;
        lppl = LpplFromId(app, lpfl->idPlanet);
        if (!lppl || lppl->iPlayer != app.state.idPlayer) continue;
        for (j = 0; j < 3 && lppl->rgwtMin[j] <= 0; j++) {}
        if (j != 3) break;
    }
    if (i == app.state.cFleet) return 0;
    idFleet = lpfl->id; idPlanet = lppl->id;
    memcpy(rgCargoFleet, lpfl->rgwtMin, sizeof(rgCargoFleet));
    memcpy(rgCargoPlanet, lppl->rgwtMin, sizeof(rgCargoPlanet));
    SelectAdjFleet(app, 0, (int16_t)idFleet);
    if (!FRunWorkflow(app, 17, 0)) return 0;
    lpfl = LpflFromId(app, (int16_t)idFleet); lppl = LpplFromId(app, (int16_t)idPlanet);
    if (!lpfl || !lppl || memcmp(rgCargoFleet, lpfl->rgwtMin, sizeof(rgCargoFleet)) ||
        memcmp(rgCargoPlanet, lppl->rgwtMin, sizeof(rgCargoPlanet))) return 0;
    if (!FRunWorkflow(app, 18, 0)) return 0;
    lpfl = LpflFromId(app, (int16_t)idFleet); lppl = LpplFromId(app, (int16_t)idPlanet);
    if (!lpfl || !lppl) return 0;
    for (i = 0; i < 5; i++) {
        if (lpfl->rgwtMin[i] != rgCargoFleet[i] + (i == iSupplyWorkflow ? 1 : 0)) return 0;
        if (i < 4 && lppl->rgwtMin[i] != rgCargoPlanet[i] - (i == iSupplyWorkflow ? 1 : 0)) return 0;
    }
    if (!FRunWorkflow(app, 19, 0)) return 0;
    lpfl = LpflFromId(app, (int16_t)idFleet); lppl = LpplFromId(app, (int16_t)idPlanet);
    if (!lpfl || !lppl || memcmp(rgCargoFleet, lpfl->rgwtMin, sizeof(rgCargoFleet)) ||
        memcmp(rgCargoPlanet, lppl->rgwtMin, sizeof(rgCargoPlanet))) return 0;
    puts("Planet/fleet cargo transfer cancel, accept and reverse workflows passed");
    return 1;
}

static BOOL FCheckFleetWorkflows(APPSESSION &app) {
    int64_t rgShipSav[16], rgCargoSav[5], rgShip[16], rgCargo[5];
    FLEET *lpfl, *rgflSav;
    int i, cFleetSav = app.state.cFleet, cOwned, idFleet = idflNone;
    BOOL fPassed = 0;
    cOwned = CFleetTotals(app, rgShipSav, rgCargoSav);
    if (cOwned < 2) return 0;
    rgflSav = (FLEET *)(SDL_calloc((size_t)app.state.cFleet, sizeof(*rgflSav)));
    if (!rgflSav) return 0;
    for (i = 0; i < app.state.cFleet; i++) {
        lpfl = app.state.rglpfl[i];
        if (!lpfl) goto LDone;
        rgflSav[i] = *lpfl;

        if (idFleet == idflNone && lpfl->iPlayer == app.state.idPlayer && !lpfl->fDead) idFleet = lpfl->id;
    }
    // The small startup frame clips the lower orders controls; use a normal gameplay size.
    ui::SetWindowPos(app.lpui->hwndFrame, NULL, 0, 0, 900, 700, win32::SWP_NOMOVE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
    if (!FCheckCargoWorkflows(app)) goto LDone;
    // Select through the game's normal fleet UI entry; only copied fixtures are edited.
    SelectAdjFleet(app, 0, (int16_t)idFleet);
    if (app.state.sel.grobj != grobjFleet || app.state.sel.fl.id != idFleet || !FCheckRepeatOrders(app, idFleet) || !FCheckWaypointTask(app, idFleet) || !FCheckRouteAppendDelete(app, idFleet) || !FRunWorkflow(app, 13, 0) || app.state.cFleet != cFleetSav ||
        CFleetTotals(app, rgShip, rgCargo) != cOwned || memcmp(rgShip, rgShipSav, sizeof(rgShip)) ||
        memcmp(rgCargo, rgCargoSav, sizeof(rgCargo))) goto LDone;
    for (i = 0; i < app.state.cFleet; i++) {
        lpfl = app.state.rglpfl[i];
        if (!lpfl || lpfl->id != rgflSav[i].id || lpfl->fDead != rgflSav[i].fDead || lpfl->cord != rgflSav[i].cord ||
            memcmp(lpfl->rgcsh, rgflSav[i].rgcsh, sizeof(lpfl->rgcsh)) ||
            memcmp(lpfl->rgwtMin, rgflSav[i].rgwtMin, sizeof(lpfl->rgwtMin))) goto LDone;
    }
    if (!FRunWorkflow(app, 14, 0) || CFleetTotals(app, rgShip, rgCargo) != cOwned - 1 ||
        memcmp(rgShip, rgShipSav, sizeof(rgShip)) || memcmp(rgCargo, rgCargoSav, sizeof(rgCargo))) goto LDone;
    if (!FRunWorkflow(app, 15, 0) || CFleetTotals(app, rgShip, rgCargo) != cOwned - 1 ||
        memcmp(rgShip, rgShipSav, sizeof(rgShip)) || memcmp(rgCargo, rgCargoSav, sizeof(rgCargo))) goto LDone;
    lpfl = LpflFromId(app, (int16_t)idXferWorkflow);
    if (lpfl && !lpfl->fDead) goto LDone;
    if (!FRunWorkflow(app, 16, 0) || CFleetTotals(app, rgShip, rgCargo) != cOwned ||
        memcmp(rgShip, rgShipSav, sizeof(rgShip)) || memcmp(rgCargo, rgCargoSav, sizeof(rgCargo))) goto LDone;
    lpfl = LpflFromId(app, (int16_t)idXferWorkflow);
    if (!lpfl || lpfl->fDead || lpfl->rgcsh[ishXferWorkflow] != 1) goto LDone;
    puts("Fleet repeat-orders, merge and split/cancel workflows passed");
    fPassed = 1;
LDone:
    SDL_free(rgflSav);
    return fPassed;
}

#include "win32_raceroster_workflow.h"
#include "win32_designer_workflow.h"
#include "win32_ui_polish.h"

static BOOL FCheckGameplayWorkflows(APPSESSION &app) {
    PROD rgprodSav[64];
    SHDEF rgshdefSav[16];
    BTLPLAN btlplanSav = app.state.rglpbtlplan[app.state.idPlayer][0];
    int iRelationSav = app.state.rgplr[app.state.idPlayer].rgmdRelation[app.state.idPlayer == 0];
    int iResearchSav = app.state.rgplr[app.state.idPlayer].iTechNow(), cProdSav, cProdItemsSav = 0, i, iResearchNew = (iResearchSav + 1) % 6;
    int16_t fNoResearchSav, fSinglePlrSav;
    const char *pszWorkflow = PszCompatEnvironment("STARS_CLIENT_WORKFLOW");
    if (pszWorkflow) {
        if (strcmp(pszWorkflow, "polish") == 0) return FCheckMessageWheel(app) && FCheckPlanetPane(app);
        if (strcmp(pszWorkflow, "designer") == 0) return FCheckDesignerWorkflows(app) && FCheckSaveReloadWorkflow(app);
        return strcmp(pszWorkflow, "fleet") == 0 && FCheckFleetWorkflows(app);
    }
    if (app.state.sel.grobj != grobjPlanet || app.state.sel.pl.iPlayer != app.state.idPlayer) {
        fprintf(stderr, "Workflow selection: grobj %u planet %d owner %d player %d\n", app.state.sel.grobj, app.state.sel.pl.id, app.state.sel.pl.iPlayer, app.state.idPlayer);
        return 0;
    }
    if (!FRunWorkflow(app, 1, iResearchNew) || app.state.rgplr[app.state.idPlayer].iTechNow() != iResearchNew) {
        fprintf(stderr, "Research commit: wanted %d got %d\n", iResearchNew, app.state.rgplr[app.state.idPlayer].iTechNow()); return 0;
    }
    if (!FRunWorkflow(app, 1, iResearchSav) || app.state.rgplr[app.state.idPlayer].iTechNow() != iResearchSav) {
        fprintf(stderr, "Research restore: wanted %d got %d\n", iResearchSav, app.state.rgplr[app.state.idPlayer].iTechNow()); return 0;
    }
    cProdSav = app.state.sel.pl.lpplprod ? app.state.sel.pl.lpplprod->iprodMac : 0;
    if (cProdSav > (int)SDL_arraysize(rgprodSav)) { fprintf(stderr, "Production count %d\n", cProdSav); return 0; }
    if (cProdSav != 0) memcpy(rgprodSav, app.state.sel.pl.lpplprod->rgprod(), (size_t)cProdSav * sizeof(PROD));
    for (i = 0; i < cProdSav; i++) cProdItemsSav += rgprodSav[i].cItem;
    fNoResearchSav = app.state.sel.pl.fNoResearch;
    if (!FRunWorkflow(app, 2, 0) || app.state.lpplProdGlob || app.lpui->hwndProdDlg || app.lpui->fDlgUp || app.state.sel.pl.fNoResearch != fNoResearchSav ||
        (app.state.sel.pl.lpplprod ? app.state.sel.pl.lpplprod->iprodMac : 0) != cProdSav ||
        (cProdSav != 0 && memcmp(rgprodSav, app.state.sel.pl.lpplprod->rgprod(), (size_t)cProdSav * sizeof(PROD)))) { fprintf(stderr, "Production cancel: passed %d glob %p dialog %p up %d research %d/%d count %d/%d\n", fWorkflowPassed, (void *)app.state.lpplProdGlob, (void *)app.lpui->hwndProdDlg, app.lpui->fDlgUp, app.state.sel.pl.fNoResearch, fNoResearchSav, app.state.sel.pl.lpplprod ? app.state.sel.pl.lpplprod->iprodMac : 0, cProdSav); return 0; }
    if (!FRunWorkflow(app, 3, 0) || !app.state.sel.pl.lpplprod) return 0;
    for (i = 0, cProdSav = 0; i < app.state.sel.pl.lpplprod->iprodMac; i++) cProdSav += app.state.sel.pl.lpplprod->rgprod()[i].cItem;
    if (cProdSav != cProdItemsSav + 1 || !FRunWorkflow(app, 4, cProdSav)) return 0;
    if (!FRunWorkflow(app, 5, !btlplanSav.fDumpCargo) || app.state.rglpbtlplan[app.state.idPlayer][0].fDumpCargo != !btlplanSav.fDumpCargo ||
        !FRunWorkflow(app, 5, btlplanSav.fDumpCargo) || memcmp(&btlplanSav, &app.state.rglpbtlplan[app.state.idPlayer][0], sizeof(btlplanSav))) return 0;
    if (!FRunWorkflow(app, 7, (btlplanSav.mdTactic + 1) % 6) || app.state.rglpbtlplan[app.state.idPlayer][0].mdTactic != (btlplanSav.mdTactic + 1) % 6 ||
        !FRunWorkflow(app, 7, btlplanSav.mdTactic) || memcmp(&btlplanSav, &app.state.rglpbtlplan[app.state.idPlayer][0], sizeof(btlplanSav))) return 0;
    if (app.state.game.cPlayer < 2) return 0;
    fSinglePlrSav = app.state.game.fSinglePlr;
    if (fSinglePlrSav) {
        ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_GAME_RELATIONS, 0);
        if (HwndWorkflowDialog(app) || !win32::IsWindowEnabled(app.lpui->hwndFrame)) return 0;
    }
    // The copied human/AI fixture disables diplomacy in single-player mode.
    // Exercise its multiplayer UI in memory, then restore the game-mode flag.
    app.state.game.fSinglePlr = 0;
    fWorkflowPassed = FRunWorkflow(app, 6, (iRelationSav + 1) % 3) &&
        FRunWorkflow(app, 6, iRelationSav) && app.state.rgplr[app.state.idPlayer].rgmdRelation[app.state.idPlayer == 0] == iRelationSav;
    app.state.game.fSinglePlr = fSinglePlrSav;
    if (!fWorkflowPassed) return 0;
    memcpy(rgshdefSav, app.state.rgshdef, sizeof(rgshdefSav));
    cShdefWorkflow = app.state.rgplr[app.state.idPlayer].cShDef;
    for (ishdefWorkflow = 0; ishdefWorkflow < 16 && !app.state.rgshdef[ishdefWorkflow].fFree; ishdefWorkflow++) {}
    if (ishdefWorkflow == 16 || !FRunWorkflow(app, 8, 0) || !FRunWorkflow(app, 9, 0) ||
        !FRunWorkflow(app, 10, 0) || !FRunWorkflow(app, 11, 0) || !FRunWorkflow(app, 12, 0)) return 0;
    for (i = 0; i < 16; i++) {
        // Drawing the designer materializes its lazy combat-power cache.
        if (!rgshdefSav[i].fFree && rgshdefSav[i].lPower == -1 && app.state.rgshdef[i].lPower != -1)
            rgshdefSav[i].lPower = LComputePower(app, &rgshdefSav[i]);
        if (rgshdefSav[i].fFree != app.state.rgshdef[i].fFree ||
            (!rgshdefSav[i].fFree && memcmp(&rgshdefSav[i], &app.state.rgshdef[i], sizeof(SHDEF)))) {
            fprintf(stderr, "Ship restore mismatch: design %d\n", i);
            return 0;
        }
    }
    printf("Research, production, battle-plan, relation and ship-design workflows passed\n");
    return 1;
}

INT_PTR StarsTest_AskSaveDialog(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());
    const char *pszChoice = PszCompatEnvironment("STARS_CLIENT_SWITCH_CHOICE");
    if (pszChoice && message == win32::WM_INITDIALOG) {
        if (!strcmp(pszChoice, "save")) win32::EndDialog(hwnd, 1);
        else if (!strcmp(pszChoice, "discard")) win32::EndDialog(hwnd, 2);
        else if (!strcmp(pszChoice, "submit")) win32::EndDialog(hwnd, -1);
        else if (!strcmp(pszChoice, "cancel")) win32::EndDialog(hwnd, -2);
        else return 0;
        return 1;
    }
    if (PszCompatEnvironment("STARS_CLIENT_SWITCH_CANCEL_TEST") && message == win32::WM_INITDIALOG) {
        fSwitchCancelSent = 1;
        win32::EndDialog(hwnd, -2);
        return 1;
    }
    INT_PTR fRet = AskSaveDialog(hwnd, message, wParam, lParam);
    if (message == win32::WM_INITDIALOG && PszCompatEnvironment("STARS_CLIENT_HOST_LIBRARY_TEST")) {
        HWND hwndSave = win32::GetDlgItem(hwnd, IDC_SAVE);
        HWND hwndSubmit = win32::GetDlgItem(hwnd, IDC_SAVESUBMIT);
        HWND hwndDontSave = win32::GetDlgItem(hwnd, IDC_NO_DON_T_SAVE);
        if (hwndSave && hwndDontSave && (app.state.game.fSinglePlr ? !hwndSubmit : hwndSubmit != NULL)) {
            fHostSavePromptSeen = 1;
            win32::PostMessageA(hwnd, win32::WM_COMMAND, IDC_NO_DON_T_SAVE, (LPARAM)hwndDontSave);
        }
    }
    return fRet;
}

int16_t StarsTest_AlertSz(APPSESSION &, const char *sz, int16_t mbType) {
    if (fRejectRacePassword) return IDOK;
    if (PszCompatEnvironment("STARS_CLIENT_RACE_VERSION_TEST") && strstr(sz, "was unable to save your race data file")) {
        fRaceVersionSaveFailed = 1;
        return IDOK;
    }
    if (PszCompatEnvironment("STARS_CLIENT_GAME_DELETE_MODE") &&
        strstr(sz, "The selected managed game could not be completely deleted.")) {
        fLibraryGameDeleteFailureAlert = 1;
        fprintf(stderr, "Startup alert: %s\n", sz);
        cAlerts++;
        return IDOK;
    }
    if (PszCompatEnvironment("STARS_CLIENT_HOST_LIBRARY_TEST") && (mbType & win32::MB_YESNO)) {
        cHostOrderConflictPrompts++;
        return idHostOrderConflictResponse;
    }
    if (PszCompatEnvironment("STARS_CLIENT_HOST_LIBRARY_TEST") &&
        strstr(sz, "Auto Generate is disabled because all of the human players are dead or inactive")) return IDOK;
    if (PszCompatEnvironment("STARS_CLIENT_RACE_LIBRARY_TEST") && strstr(sz, "race design could not be deleted")) return IDOK;
    if (fExpectRaceExportFailureAlert && strstr(sz, "race design could not be exported")) return IDOK;
    if (PszCompatEnvironment("STARS_CLIENT_EXPORT_EXPECTED") && strstr(sz, "game could not be exported")) {
        cAlerts++;
        return IDOK;
    }
    fprintf(stderr, "Startup alert: %s\n", sz);
    cAlerts++;
    if (PszCompatEnvironment("STARS_CLIENT_HOST_LIBRARY_TEST")) return IDOK;
    if (PszCompatEnvironment("STARS_CLIENT_LIBRARY_FAIL_WRITE") || PszCompatEnvironment("STARS_CLIENT_SWITCH_FAIL_SAVE")) return IDOK;
    if (PszCompatEnvironment("STARS_CLIENT_IMPORT_INVALID_EXPECTED")) {
        fChecked = 1;
        fPassed = 1;
        win32::PostQuitMessage(0);
        return IDOK;
    }
    if (PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_EXPECTED")) {
        if (PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_FAIL_CONFLICT_RESTORE") &&
            strstr(sz, "the managed game and its orders were preserved")) fConflictRestoreAlert = 1;
        return IDOK;
    }
    win32::PostQuitMessage(91);
    return IDOK;
}

int StarsTest_MessageBoxA(HWND hwnd, LPCSTR pszText, LPCSTR pszCaption, UINT type) {
    (void)hwnd; (void)pszCaption;
    if (pszText && strstr(pszText, "Import games from your recent files")) {
        cRecentMigrationPrompts++;
        fPassed = 0;
        win32::PostQuitMessage(92);
        return win32::IDNO;
    }
    if (fExpectGameRenameFailure) { fExpectGameRenameFailure = 0; return IDOK; }
    if (pszText && strstr(pszText, "Delete the managed race design Managed.r2?") && (type & win32::MB_YESNO))
        return cRaceDeleteR2++ == 0 ? win32::IDNO : win32::IDYES;
    if (pszText && strstr(pszText, "Delete the managed race design Managed.r3?") && (type & win32::MB_YESNO)) {
        fFailRaceDelete = 1;
        return win32::IDYES;
    }
    if (pszText && strstr(pszText, "Delete managed game selected-game?") && (type & win32::MB_YESNO)) {
        fLibraryGameDeletePrompt = strstr(pszText, "all player files") && strstr(pszText, "host state") &&
                                   strstr(pszText, "history") && strstr(pszText, "backups") &&
                                   strstr(pszText, "Export the complete game first");
        return PszCompatEnvironment("STARS_CLIENT_GAME_DELETE_MODE") &&
               strcmp(PszCompatEnvironment("STARS_CLIENT_GAME_DELETE_MODE"), "cancel") == 0 ? win32::IDNO : win32::IDYES;
    }
    if (pszText && strstr(pszText, "The selected managed game could not be completely deleted."))
        fLibraryGameDeleteFailureAlert = 1;
    return win32::MessageBoxA(hwnd, pszText, pszCaption, type);
}

static const NATIVEPUBLISHIO publishNative;
static int IRemoveClientFile(const char *pszFile, int fDirectory) {
    if (fDirectory) return publishNative.pfnRemove(pszFile, fDirectory);
    static int fFailImportOrdersRemove;
    if (!fFailLibraryGameDelete && PszCompatEnvironment("STARS_CLIENT_GAME_DELETE_MODE") &&
        strcmp(PszCompatEnvironment("STARS_CLIENT_GAME_DELETE_MODE"), "failure") == 0 && strstr(pszFile, "selected-game")) {
        fFailLibraryGameDelete = 1;
        return -1;
    }
    if (fFailRaceDelete) {
        fFailRaceDelete = 0;
        return -1;
    }
    if (!fFailImportOrdersRemove && PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_FAIL_ORDERS_REMOVE") &&
        strstr(pszFile, ".x1")) {
        fFailImportOrdersRemove = 1;
        return -1;
    }
    return publishNative.pfnRemove(pszFile, fDirectory);
}

static int IRenameClientFile(const char *pszOld, const char *pszNew, int fReplace, int fSync) {
    static int fFailImportReplace, fFailConflictPublish, fFailConflictRestore;
    if (!fSync && fReplace) {
        if (fFailRaceVersionSave && strstr(pszOld, ".stars-race-stage-")) {
            fFailRaceVersionSave = 0;
            return -1;
        }
        if (!fFailImportReplace && PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_FAIL_REPLACE") && strstr(pszNew, ".m1")) {
            fFailImportReplace = 1;
            return -1;
        }
        if (PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_FAIL_ROLLBACK") && strstr(pszOld, ".previous.") &&
            strstr(pszNew, ".m1")) return -1;
        if (fFailRaceExportReplace && !SDL_strcasecmp(pszNew, szRaceExportFailTarget)) {
            fFailRaceExportReplace = 0;
            return -1;
        }
    } else if (!fSync) {
        if (!fFailConflictPublish && PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_FAIL_CONFLICT_PUBLISH") &&
            strstr(pszOld, ".stars-game-stage-") && strstr(pszNew, "managed-copy-duplicate")) {
            fFailConflictPublish = 1;
            return -1;
        }
        if (!fFailConflictRestore && PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_FAIL_CONFLICT_RESTORE") &&
            strstr(pszOld, ".stars-game-old-") && strstr(pszNew, "managed-copy-duplicate")) {
            fFailConflictRestore = 1;
            return -1;
        }
        if (fFailGameRenamePublish && !SDL_strcasecmp(pszNew, szGameRenameFailTarget)) {
            fFailGameRenamePublish = 0;
            return -1;
        }
    }
    return publishNative.pfnRename(pszOld, pszNew, fReplace, fSync);
}

#include "win32_raceversion_workflow.h"
#include "win32_newgame_workflow.h"
#include "win32_game_export_workflow.h"
#ifdef __APPLE__
#include "win32_macos_shortcuts.h"
#endif

static BOOL FCopyOrderWithByteChange(const char *pszSource, const char *pszTarget, long ibChange, BYTE bMask) {
    NATIVEFILE *pfSource, *pfTarget;
    int ch;
    long ib;
    BOOL fChanged, fOkay;

    pfSource = PfNativeOpen(pszSource, NATIVEFILEMODE::read);
    if (!pfSource) return 0;
    pfTarget = PfNativeOpen(pszTarget, NATIVEFILEMODE::writeTruncate);
    if (!pfTarget) { ICloseNativeFile(pfSource); return 0; }
    fChanged = 0;
    fOkay = 1;
    ib = 0;
    while ((ch = ChGetNativeFile(pfSource)) != EOF) {
        if (ib == ibChange) { ch ^= bMask; fChanged = 1; }
        if (ChPutNativeFile(ch, pfTarget) == EOF) { fOkay = 0; break; }
        ib++;
    }
    if (FNativeStreamError(pfSource)) fOkay = 0;
    if (ICloseNativeFile(pfSource) != 0) fOkay = 0;
    if (ICloseNativeFile(pfTarget) != 0) fOkay = 0;
    return fOkay && fChanged;
}

/* Cancellation must never bind the previous race to a different save path. */
static BOOL FTestRaceCancellation(APPSESSION &app) {
    char szCancelPassword[] = "cancel-test";
    PLAYER plrSav;
    char szProtected[512], szSnapshot[512], szExport[512], szImport[512], szDuplicate[512];
    const char *pszSettings = PszCompatEnvironment("STARS_SETTINGS_DIR");
    int16_t fOkay;

    if (!FLoadLibraryRace(app, 0, "Managed.r1")) return 0;
    app.state.vplr.lSalt = LSaltFromSz(szCancelPassword);
    if (!FSaveLibraryRace(app, 0, "Protected.r1", &app.state.vplr) || !FLoadLibraryRace(app, 0, "Managed.r1")) return 0;
    plrSav = app.state.vplr;
    CchSnprintf(std::span{szProtected, sizeof(szProtected)}, "%s%sRace Designs%sProtected.r1", pszSettings, szDirSep, szDirSep);
    CchSnprintf(std::span{szSnapshot, sizeof(szSnapshot)}, "%s.snapshot", szProtected);
    CchSnprintf(std::span{szExport, sizeof(szExport)}, "%s.export.r1", szProtected);
    CchSnprintf(std::span{szImport, sizeof(szImport)}, "%s%sRace Designs%sCancelled.r1", pszSettings, szDirSep, szDirSep);
    CchSnprintf(std::span{szDuplicate, sizeof(szDuplicate)}, "%s%sRace Designs%sProtected.r2", pszSettings, szDirSep, szDirSep);
    if (!FTestCopyFile(szProtected, szSnapshot)) return 0;
    fRejectRacePassword = 1;
    app.state.lSaltLast = -5;
    fOkay = !FLoadLibraryRace(app, 0, "Protected.r1") && !memcmp(&app.state.vplr, &plrSav, sizeof(app.state.vplr)) &&
            !strcmp(PszCurrentLibraryRaceName(), "Managed.r1");
    app.state.lSaltLast = -5;
    fOkay = fOkay && !FEditLibraryRace(app, 0, "Protected.r1");
    app.state.lSaltLast = -5;
    fOkay = fOkay && !FDuplicateLibraryRace(app, 0, "Protected.r1");
    app.state.lSaltLast = -5;
    fOkay = fOkay && !FExportLibraryRace(app, 0, "Protected.r1", szExport);
    app.state.lSaltLast = -5;
    fOkay = fOkay && !FImportLibraryRaceAs(app, 0, szProtected, "Cancelled.r1");
    fRejectRacePassword = 0;
    fOkay = fOkay && !memcmp(&app.state.vplr, &plrSav, sizeof(app.state.vplr)) &&
            !strcmp(PszCurrentLibraryRaceName(), "Managed.r1") && FTestFilesEqual(szProtected, szSnapshot) &&
            !FFileExists(szExport) && !FFileExists(szImport) && !FFileExists(szDuplicate);
    RemoveNativeFile(szProtected);
    RemoveNativeFile(szSnapshot);
    return fOkay;
}

/* Inspect the managed bytes without completing the optional race wizard. */
static BOOL FTestLegacyRaceImport(APPSESSION &app) {
    char szOldPassword[] = "old-password", szNewPassword[] = "new-\xc3\xa9";
    RTBOF rtbof = {};
    std::byte rgbBof[cbWireBof];
    std::byte rgbPlr[264];
    PLAYER plr, plrSav;
    char szSource[512], szSnapshot[512], szImported[512], szCancelled[512];
    const char *pszSettings = PszCompatEnvironment("STARS_SETTINGS_DIR");
    const char *rgszName[] = {"Legacy.r1", "Legacy-protected.r1"};
    uint16_t icksum;
    int i, iRelation;
    BOOL fOkay = 0;

    if (!FLoadLibraryRace(app, 0, "Managed.r1")) return 0;
    plrSav = app.state.vplr;
    CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%slegacy-source.r1", pszSettings, szDirSep);
    CchSnprintf(std::span{szSnapshot, sizeof(szSnapshot)}, "%s.snapshot", szSource);
    CchSnprintf(std::span{szCancelled, sizeof(szCancelled)}, "%s%sRace Designs%sLegacy-cancelled.r1", pszSettings, szDirSep, szDirSep);
    fTestLegacyRaceImport = 1;
    fAcceptLegacyRacePassword = 1;
    for (i = 0; i < 2; i++) {
        plr = plrSav;
        strcpy(plr.szName, "R\xe9");
        strcpy(plr.szNames, "R\xe9s");
        plr.lSalt = i == 0 ? 0 : LSaltFromSz(szOldPassword);
        memcpy(rtbof.rgid, "J3J3", 4);
        rtbof.verMajor = 2;
        rtbof.verMinor = 84;
        rtbof.dt = dtRace;
        StreamOpen(app, szSource, mdCreate);
        FEncodeBof(rtbof, rgbBof);
        WriteRt(app, rtBOF, std::as_bytes(std::span{rgbBof}).first(static_cast<std::size_t>(cbWireBof)));
        // This fixture deliberately writes CP1252, outside the UTF-8-only
        // application writer. Import must still convert these original bytes.
        FEncodePlayer(plr, rgbPlr);
        auto rgbTail = std::span{rgbPlr}.subspan(plr.det == detAll ? cbWirePlayerPrefix : cbWirePlayerPartial);
        NATIVEBYTEWRITER writer(rgbTail);
        if (plr.det == detAll) {
            for (iRelation = 15; iRelation >= 0 && plr.rgmdRelation[iRelation] == 0; iRelation--) {
            }
            iRelation++;
            writer.FWriteU8(static_cast<uint8_t>(iRelation));
            writer.FWriteBytes(std::as_bytes(std::span{plr.rgmdRelation}.first(iRelation)));
        }
        writer.FWriteU8(0);
        writer.FWriteBytes(std::as_bytes(std::span{plr.szName}.first(strlen(plr.szName) + 1)));
        writer.FWriteU8(0);
        writer.FWriteBytes(std::as_bytes(std::span{plr.szNames}.first(strlen(plr.szNames) + 1)));
        WriteRt(app, rtPlr, std::as_bytes(std::span{rgbPlr}).first(static_cast<std::size_t>(sizeof(rgbPlr) - rgbTail.size() + writer.CbWritten())));
        icksum = IRaceChecksum(&plr);
        WriteRtEof(app, icksum);
        StreamClose(app);
        if (!FTestCopyFile(szSource, szSnapshot)) goto LDone;
        cLegacyRacePasswords = 0;
        if (!FImportLibraryRaceAs(app, 0, szSource, rgszName[i]) || cLegacyRacePasswords != i ||
            !FTestFilesEqual(szSource, szSnapshot)) goto LDone;
        CchSnprintf(std::span{szImported, sizeof(szImported)}, "%s%sRace Designs%s%s", pszSettings, szDirSep, szDirSep, rgszName[i]);
        if (FWasRaceFile(app, szImported, 0) != 1 || VerMinor(app.state.wVersFile) != 85 ||
            strcmp(app.state.vplr.szName, "R\xc3\xa9") || strcmp(app.state.vplr.szNames, "R\xc3\xa9s") ||
            app.state.vplr.lSalt != (i == 0 ? 0 : LSaltFromSz(szNewPassword)) || cLegacyRacePasswords != i) goto LDone;
    }
    fAcceptLegacyRacePassword = 0;
    plrSav = app.state.vplr;
    if (FImportLibraryRaceAs(app, 0, szSource, "Legacy-cancelled.r1") ||
        FFileExists(szCancelled) || memcmp(&app.state.vplr, &plrSav, sizeof(app.state.vplr)) ||
        !FTestFilesEqual(szSource, szSnapshot)) goto LDone;
    fOkay = 1;
LDone:
    fTestLegacyRaceImport = 0;
    RemoveNativeFile(szSource);
    RemoveNativeFile(szSnapshot);
    return fOkay;
}

static void  CheckStartup(HWND hwnd, UINT msg, UINT_PTR id, DWORD dwTime) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HWND lpFrame, lpTitle;
    char szRaceName[256], szDuplicate[512], szExport[512], szManaged[512], szExportSnapshot[512];
    const char *pszSettings = PszCompatEnvironment("STARS_SETTINGS_DIR");
    const char *pszRaceSource = PszCompatEnvironment("STARS_CLIENT_RACE_IMPORT_SOURCE");
    const char *pszRaceSnapshot = PszCompatEnvironment("STARS_CLIENT_RACE_IMPORT_SNAPSHOT");
    const char *pszCapture = PszCompatEnvironment("STARS_CLIENT_CAPTURE");
    const char *pszWizardCapture = PszCompatEnvironment("STARS_CLIENT_WIZARD_CAPTURE");
    const char *pszRaceCapture = PszCompatEnvironment("STARS_CLIENT_RACE_CAPTURE");
    const char *pszGameCapture = PszCompatEnvironment("STARS_CLIENT_GAME_CAPTURE");
    const char *pszExportGame = PszCompatEnvironment("STARS_CLIENT_EXPORT_GAME");
    static const DLGPROC rglpfnRace[] = {RaceWizardDlg1, RaceWizardDlg4, RaceWizardDlg5, RaceWizardDlg2, RaceWizardDlg3, RaceWizardDlg6};
    BYTE *rgb;
    size_t cb, i;
    int iButton;
    int16_t fImported;
    int iTarget, c;
    HWND hwndList, hwndListTarget;
    char szName[256];
    const char *pszExpected, *pszTarget, *pszYear, *pszCopyName;
    const char *pszHostSource, *pszHostPlayerSource, *pszHostOrderBase, *pszHostPickerBase;
    const char *pszDeleteMode;
    SDL_Event event = {};
    BOOL fDifferent = 0;
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
    char szHostOrderGeneration[512], szHostOrderDifferent[512], szHostPickerOrder[512], szSavedBase[sizeof(app.state.szBase)];
    char szFleetNameSav[64];
    int cchHostOrder;
    int64_t cbHostFile;
    HWND hwndHostDialog;
    FLEET *lpflHostOrder;
    int iFleetHostOrder;
    BOOL fHadFleetName;

    (void)hwnd; (void)msg; (void)dwTime;
#ifdef __APPLE__
    if (PszCompatEnvironment("STARS_CLIENT_MAC_SHORTCUTS_TEST")) {
        win32::KillTimer(NULL, id);
        if (!FCheckMacShortcuts(app)) goto LDone;
        fTitlePainted = fChecked = fPassed = 1;
        puts("Mac Command shortcuts, menu hints and text editing passed");
        win32::PostQuitMessage(0);
        return;
    }
#endif
    if (PszCompatEnvironment("STARS_CLIENT_HOST_LIBRARY_TEST")) {
        pszHostSource = PszCompatEnvironment("STARS_CLIENT_HOST_IMPORT_SOURCE");
        pszHostPlayerSource = PszCompatEnvironment("STARS_CLIENT_HOST_PLAYER_IMPORT_SOURCE");
        pszHostOrderBase = PszCompatEnvironment("STARS_CLIENT_HOST_ORDER_BASE");
        pszHostPickerBase = PszCompatEnvironment("STARS_CLIENT_HOST_ORDER_PICKER_BASE");
        cchHostOrder = CchSnprintf(std::span{szHostManagedBase, sizeof(szHostManagedBase)},
                                   "%s%sGames%sHosted%sHosted", pszSettings, szDirSep, szDirSep, szDirSep);
        if (cchHostOrder <= 0 || (size_t)cchHostOrder >= sizeof(szHostManagedBase)) goto LDone;
        if (!pszHostOrderBase || !pszHostPickerBase ||
            CchSnprintf(std::span{szHostOrderFile, sizeof(szHostOrderFile)}, "%s.x1", pszHostOrderBase) <= 0 ||
            CchSnprintf(std::span{szHostOrderDifferent, sizeof(szHostOrderDifferent)}, "%s-different.x1", pszHostOrderBase) <= 0 ||
            CchSnprintf(std::span{szHostPickerOrder, sizeof(szHostPickerOrder)}, "%s.x1", pszHostPickerBase) <= 0 ||
            CchSnprintf(std::span{szHostManagedOrders, sizeof(szHostManagedOrders)}, "%s.x1", szHostManagedBase) <= 0) goto LDone;
        if (iHostLibraryTestStage == 0) {
            if (!win32::IsWindowVisible(app.lpui->hwndTitle) || !win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAMES) ||
                !win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_HOST_GAMES) ||
                !win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAMES_VIEW) ||
                !win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_HOST_VIEW) ||
                !win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_IMPORT_ORDERS) ||
                (win32::GetWindowLongPtrA(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_HOST_GAMES), win32::GWL_STYLE) & win32::WS_VISIBLE) ||
                (win32::GetWindowLongPtrA(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_IMPORT_ORDERS), win32::GWL_STYLE) & win32::WS_VISIBLE) ||
                !pszHostSource) goto LDone;
            if (!FPushClick(app, win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_HOST_VIEW))) goto LDone;
            iHostLibraryTestStage = 20;
            return;
        }
        if (iHostLibraryTestStage == 20) {
            if (!(win32::GetWindowLongPtrA(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_HOST_GAMES), win32::GWL_STYLE) & win32::WS_VISIBLE) ||
                (win32::GetWindowLongPtrA(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAMES), win32::GWL_STYLE) & win32::WS_VISIBLE) ||
                !(win32::GetWindowLongPtrA(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_IMPORT_ORDERS), win32::GWL_STYLE) & win32::WS_VISIBLE)) {
                goto LDone;
            }
            if (!FPostOwnedTestFolder(app.lpui->hwndTitle,SDL_strdup(pszHostSource))) goto LDone;
            iHostLibraryTestStage = 21;
            return;
        }
        if (iHostLibraryTestStage == 21) {
            for (hwndHostDialog = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwndHostDialog;
                 hwndHostDialog = win32::GetWindow(hwndHostDialog, win32::GW_HWNDNEXT)) {
                if (!hwndHostDialog->fDialog || hwndHostDialog->lpfnDlg != LibraryNameDlg) continue;
                if (!ui::SetWindowText(win32::GetDlgItem(hwndHostDialog, IDC_EDIT1), "Hosted") ||
                    !FPushClick(app, win32::GetDlgItem(hwndHostDialog, IDOK))) goto LDone;
                iHostLibraryTestStage = 22;
                return;
            }
            return;
        }
        if (iHostLibraryTestStage == 22) {
            RefreshLibraryGames(app, app.lpui->hwndTitle);
            RefreshLibraryHostGames(app, app.lpui->hwndTitle);
            if (ui::SendMessage(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAMES), win32::LB_GETCOUNT, 0, 0) != 0 ||
                ui::SendMessage(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_HOST_GAMES), win32::LB_GETCOUNT, 0, 0) != 1 ||
                !(win32::GetWindowLongPtrA(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_HOST_GAMES), win32::GWL_STYLE) & win32::WS_VISIBLE) ||
                CchSnprintf(std::span{szHostSourceHost, sizeof(szHostSourceHost)}, "%s%sgame.hst", pszHostSource, szDirSep) <= 0 ||
                CchSnprintf(std::span{szHostSourceUniverse, sizeof(szHostSourceUniverse)}, "%s%sgame.xy", pszHostSource, szDirSep) <= 0 ||
                CchSnprintf(std::span{szHostManagedHost, sizeof(szHostManagedHost)}, "%s%sGames%sHosted%sHosted.hst", pszSettings, szDirSep, szDirSep, szDirSep) <= 0 ||
                CchSnprintf(std::span{szHostManagedUniverse, sizeof(szHostManagedUniverse)}, "%s%sGames%sHosted%sHosted.xy", pszSettings, szDirSep, szDirSep, szDirSep) <= 0 ||
                !FTestFilesEqual(szHostSourceHost, szHostManagedHost) ||
                !FTestFilesEqual(szHostSourceUniverse, szHostManagedUniverse) ||
                !pszHostPlayerSource) goto LDone;
            if (!win32::IsWindowEnabled(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAME_EXPORT)) ||
                !win32::IsWindowEnabled(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_DUPLICATE)) ||
                !win32::IsWindowEnabled(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RENAME))) {
                fprintf(stderr, "Host-only game actions are disabled\n");
                goto LDone;
            }
            if (!FPostOwnedTestFolder(app.lpui->hwndTitle,SDL_strdup(pszHostPlayerSource))) goto LDone;
            iHostLibraryTestStage = 25;
            return;
        }
        if (iHostLibraryTestStage == 25) {
            for (hwndHostDialog = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwndHostDialog;
                 hwndHostDialog = win32::GetWindow(hwndHostDialog, win32::GW_HWNDNEXT)) {
                if (!hwndHostDialog->fDialog || hwndHostDialog->lpfnDlg != LibraryNameDlg) continue;
                if (!ui::SetWindowText(win32::GetDlgItem(hwndHostDialog, IDC_EDIT1), "PlayerCopy") ||
                    !FPushClick(app, win32::GetDlgItem(hwndHostDialog, IDOK))) goto LDone;
                iHostLibraryTestStage = 26;
                return;
            }
            return;
        }
        if (iHostLibraryTestStage == 26) {
            RefreshLibraryGames(app, app.lpui->hwndTitle);
            RefreshLibraryHostGames(app, app.lpui->hwndTitle);
            if (ui::SendMessage(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAMES), win32::LB_GETCOUNT, 0, 0) != 1 ||
                ui::SendMessage(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_HOST_GAMES), win32::LB_GETCOUNT, 0, 0) != 2 ||
                !(win32::GetWindowLongPtrA(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_HOST_GAMES), win32::GWL_STYLE) & win32::WS_VISIBLE)) goto LDone;
            hwndList = win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_HOST_GAMES);
            if (ui::SendMessage(hwndList, win32::LB_SELECTSTRING, (WPARAM)-1, (LPARAM)"Hosted") == win32::LB_ERR ||
                !FExportSelectedLibraryGame(app, app.lpui->hwndTitle, pszSettings) ||
                CchSnprintf(std::span{szExport, sizeof(szExport)}, "%s%sHosted%sHosted.hst", pszSettings, szDirSep, szDirSep) <= 0 ||
                CchSnprintf(std::span{szHostSourceHost, sizeof(szHostSourceHost)}, "%s%sgame.hst", pszHostSource, szDirSep) <= 0 ||
                !FTestFilesEqual(szExport, szHostSourceHost)) {
                fprintf(stderr, "Host export did not use the visible selection\n");
                goto LDone;
            }
            iHostLibraryTestStage = 31;
            if (!FPushClick(app, win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_DUPLICATE))) goto LDone;
            return;
        }
        if (iHostLibraryTestStage == 31 || iHostLibraryTestStage == 33) {
            for (hwndHostDialog = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwndHostDialog;
                 hwndHostDialog = win32::GetWindow(hwndHostDialog, win32::GW_HWNDNEXT)) {
                if (!hwndHostDialog->fDialog || hwndHostDialog->lpfnDlg != LibraryNameDlg) continue;
                if (!ui::SetWindowText(win32::GetDlgItem(hwndHostDialog, IDC_EDIT1),
                                    iHostLibraryTestStage == 31 ? "HostCopy" : "HostRenamed") ||
                    !FPushClick(app, win32::GetDlgItem(hwndHostDialog, IDOK))) goto LDone;
                iHostLibraryTestStage++;
                return;
            }
            return;
        }
        if (iHostLibraryTestStage == 32) {
            hwndList = win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_HOST_GAMES);
            if (ui::SendMessage(hwndList, win32::LB_GETCOUNT, 0, 0) != 3 ||
                ui::SendMessage(hwndList, win32::LB_SELECTSTRING, (WPARAM)-1, (LPARAM)"HostCopy") == win32::LB_ERR) goto LDone;
            iHostLibraryTestStage = 33;
            if (!FPushClick(app, win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RENAME))) goto LDone;
            return;
        }
        if (iHostLibraryTestStage == 34) {
            hwndList = win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_HOST_GAMES);
            if (ui::SendMessage(hwndList, win32::LB_GETCOUNT, 0, 0) != 3 ||
                ui::SendMessage(hwndList, win32::LB_FINDSTRING, (WPARAM)-1, (LPARAM)"HostCopy") != win32::LB_ERR ||
                ui::SendMessage(hwndList, win32::LB_FINDSTRING, (WPARAM)-1, (LPARAM)"HostRenamed") == win32::LB_ERR ||
                ui::SendMessage(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAMES), win32::LB_GETCOUNT, 0, 0) != 1) goto LDone;
            if (!FPushClick(app, win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAMES_VIEW))) goto LDone;
            iHostLibraryTestStage = 23;
            return;
        }
        if (iHostLibraryTestStage == 23) {
            hwndList = win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAMES);
            if (!(win32::GetWindowLongPtrA(hwndList, win32::GWL_STYLE) & win32::WS_VISIBLE) ||
                ui::SendMessage(hwndList, win32::LB_SELECTSTRING, (WPARAM)-1,
                            (LPARAM)"PlayerCopy") == win32::LB_ERR) goto LDone;
            for (iButton = 0; iButton < (int)(sizeof(rgidLibraryButton) / sizeof(rgidLibraryButton[0])); iButton++) {
                hwndListTarget = win32::GetDlgItem(app.lpui->hwndTitle, rgidLibraryButton[iButton]);
                if (!win32::GetWindowRect(hwndListTarget, &rcFrame)) goto LDone;
                ptLibraryButton = (POINT){(rcFrame.left + rcFrame.right) / 2, (rcFrame.top + rcFrame.bottom) / 2};
                if (win32::WindowFromPoint(ptLibraryButton) != hwndListTarget) {
                    fprintf(stderr, "Library button %d is covered by another control\n", rgidLibraryButton[iButton]);
                    goto LDone;
                }
            }
            iHostLibraryTestStage = 27;
            app.lpui->fFreeingTitle = 0;
            if (!FPushClick(app, app.lpui->rghwndBtnSplash[1])) goto LDone;
            return;
        }
        if (iHostLibraryTestStage == 27) {
            if (win32::IsWindow(app.lpui->hwndTitle)) return;
            if (!app.state.lpPlanets || app.state.idPlayer == iplrNone) goto LDone;
            if (!app.lpui->hwndReportDlg) {
                ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_REPORT_FLEET, 0);
                return;
            }
            if (app.lpui->vprptCur != &app.state.vrptFleet || app.lpui->vprptCur->cRows == 0) goto LDone;
            if (app.state.sel.grobj != grobjFleet) {
                if (!StarsCompatPostMessageAsync(app.lpui->hwndReportDlg, win32::WM_LBUTTONDOWN, win32::MK_LBUTTON,
                                                 win32::MAKELPARAM(4, 20))) goto LDone;
                return;
            }
            iHostLibraryTestStage = 28;
            if (!StarsCompatPostMessageAsync(app.lpui->hwndFrame, win32::WM_CHAR, 'r', 0)) goto LDone;
            return;
        }
        if (iHostLibraryTestStage == 28) {
            for (hwndHostDialog = win32::GetWindow(app.lpui->hwndFrame, win32::GW_HWNDFIRST); hwndHostDialog;
                 hwndHostDialog = win32::GetWindow(hwndHostDialog, win32::GW_HWNDNEXT)) {
                if (!hwndHostDialog->fDialog || hwndHostDialog->lpfnDlg != RenameDlg) continue;
                if (!ui::SetWindowText(win32::GetDlgItem(hwndHostDialog, IDC_EDIT1), "Prompt save order") ||
                    !FPushClick(app, win32::GetDlgItem(hwndHostDialog, IDOK))) goto LDone;
                iHostLibraryTestStage = 29;
                return;
            }
            return;
        }
        if (iHostLibraryTestStage == 29) {
            if (app.lpui->hwndReportDlg) ui::SendMessage(app.lpui->hwndReportDlg, win32::WM_CLOSE, 0, 0);
            iHostLibraryTestStage = 30;
            ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FILE_RETURN_TO_TITLE, 0);
            return;
        }
        if (iHostLibraryTestStage == 30) {
            if (!fHostSavePromptSeen || !win32::IsWindowVisible(app.lpui->hwndTitle)) return;
            if (!FPushClick(app, win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_HOST_VIEW))) goto LDone;
            iHostLibraryTestStage = 24;
            return;
        }
        if (iHostLibraryTestStage == 24) {
            if (!(win32::GetWindowLongPtrA(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_HOST_GAMES), win32::GWL_STYLE) & win32::WS_VISIBLE) ||
                !(win32::GetWindowLongPtrA(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_IMPORT_ORDERS), win32::GWL_STYLE) & win32::WS_VISIBLE)) goto LDone;
            hwndList = win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_HOST_GAMES);
            if (ui::SendMessage(hwndList, win32::LB_SELECTSTRING, (WPARAM)-1, (LPARAM)"Hosted") == win32::LB_ERR) goto LDone;
            iHostLibraryTestStage = 7;
            app.lpui->fFreeingTitle = 1;
            if (!StarsCompatPostMessageAsync(app.lpui->hwndTitle, win32::WM_COMMAND,
                                             win32::MAKEWPARAM(IDC_LIBRARY_HOST_GAMES, win32::LBN_DBLCLK),
                                             (LPARAM)hwndList)) goto LDone;
            return;
        }
        if (iHostLibraryTestStage == 7 || iHostLibraryTestStage == 8 || iHostLibraryTestStage == 9 ||
            iHostLibraryTestStage == 14) {
            for (hwndHostDialog = win32::GetWindow(app.lpui->hwndFrame, win32::GW_HWNDFIRST); hwndHostDialog;
                 hwndHostDialog = win32::GetWindow(hwndHostDialog, win32::GW_HWNDNEXT)) {
                if (!hwndHostDialog->fDialog || hwndHostDialog->lpfnDlg != HostModeDialog) continue;
                if (iHostLibraryTestStage == 7) {
                    if (
                        (cchHostOrder = CchSnprintf(std::span{szSavedBase, sizeof(szSavedBase)}, "%s", app.state.szBase)) <= 0 ||
                        (size_t)cchHostOrder >= sizeof(szSavedBase) ||
                        (cchHostOrder = CchSnprintf(std::span{app.state.szBase, sizeof(app.state.szBase)}, "%s", pszHostOrderBase)) <= 0 ||
                        (size_t)cchHostOrder >= sizeof(app.state.szBase)) { fprintf(stderr, "Host order paths invalid\n"); goto LDone; }
                    if (!FWriteLogFile(app, app.state.szBase, 0)) { fprintf(stderr, "Host FWriteLogFile failed: %s\n", SDL_GetError()); goto LDone; }
                    strcpy(app.state.szBase, szSavedBase);
                    if (CchSnprintf(std::span{szHostOrderFile, sizeof(szHostOrderFile)}, "%s.x1", pszHostOrderBase) <= 0 ||
                        CchSnprintf(std::span{szHostManagedOrders, sizeof(szHostManagedOrders)}, "%s.x1", szHostManagedBase) <= 0 ||
                        CchSnprintf(std::span{szHostOrderIdentity, sizeof(szHostOrderIdentity)}, "%s.identity.x1", pszHostOrderBase) <= 0 ||
                        CchSnprintf(std::span{szHostOrderPlayer, sizeof(szHostOrderPlayer)}, "%s.player.x1", pszHostOrderBase) <= 0 ||
                        CchSnprintf(std::span{szHostOrderTurn, sizeof(szHostOrderTurn)}, "%s.turn.x1", pszHostOrderBase) <= 0 ||
                        CchSnprintf(std::span{szHostOrderAbsentPlayer, sizeof(szHostOrderAbsentPlayer)}, "%s.absent.x16", pszHostOrderBase) <= 0 ||
                        CchSnprintf(std::span{szHostOrderGeneration, sizeof(szHostOrderGeneration)}, "%s.generation.x1", pszHostOrderBase) <= 0 ||
                        !FCopyOrderWithByteChange(szHostOrderFile, szHostOrderIdentity, 6, 1) ||
                        !FCopyOrderWithByteChange(szHostOrderFile, szHostOrderPlayer, 14, 1) ||
                        !FCopyOrderWithByteChange(szHostOrderFile, szHostOrderAbsentPlayer, 14, 15) ||
                        !FCopyOrderWithByteChange(szHostOrderFile, szHostOrderTurn, 12, 1) ||
                        !FCopyOrderWithByteChange(szHostOrderFile, szHostOrderGeneration, 17, 0x20)) { fprintf(stderr, "Host order variants failed: %s\n", SDL_GetError()); goto LDone; }
                    for (iFleetHostOrder = 0; iFleetHostOrder < app.state.cFleet; iFleetHostOrder++) {
                        lpflHostOrder = app.state.rglpfl[iFleetHostOrder];
                        if (((uint16_t)lpflHostOrder->id >> 9 & 0xf) == 0) break;
                    }
                    if (iFleetHostOrder == app.state.cFleet) { fprintf(stderr, "Host order fixture has no player fleet\n"); goto LDone; }
                    idHostOrderFleet = lpflHostOrder->id;
                    fHadFleetName = lpflHostOrder->lpszName != NULL;
                    if (fHadFleetName) CchSnprintf(std::span{szFleetNameSav, sizeof(szFleetNameSav)}, "%s", lpflHostOrder->lpszName);
                    if (!LogChangeName(app, grobjFleet, lpflHostOrder->id, szReplacementOrder)) goto LDone;
                    if (lpflHostOrder->lpszName) FreeLp(app, lpflHostOrder->lpszName, htString);
                    lpflHostOrder->lpszName = NULL;
                    if (fHadFleetName) {
                        lpflHostOrder->lpszName = (char *)(LpAlloc(app, strlen(szFleetNameSav) + 1, htString));
                        if (!lpflHostOrder->lpszName) goto LDone;
                        strcpy(lpflHostOrder->lpszName, szFleetNameSav);
                    }
                    if (CchSnprintf(std::span{szHostOrderDifferent, sizeof(szHostOrderDifferent)}, "%s-different.x1", pszHostOrderBase) <= 0 ||
                        CchSnprintf(std::span{app.state.szBase, sizeof(app.state.szBase)}, "%s-different", pszHostOrderBase) <= 0 ||
                        !FWriteLogFile(app, app.state.szBase, 0)) { fprintf(stderr, "Host changed order write failed: %s\n", SDL_GetError()); goto LDone; }
                    strcpy(app.state.szBase, szSavedBase);
                    if (FImportLibraryOrders(app, app.lpui->hwndTitle, szHostOrderIdentity)) { fprintf(stderr, "Host order identity mismatch accepted\n"); goto LDone; }
                    if (FImportLibraryOrders(app, app.lpui->hwndTitle, szHostOrderPlayer)) { fprintf(stderr, "Host order player mismatch accepted\n"); goto LDone; }
                    if (FImportLibraryOrders(app, app.lpui->hwndTitle, szHostOrderTurn)) { fprintf(stderr, "Host order turn mismatch accepted\n"); goto LDone; }
                    if (FImportLibraryOrders(app, app.lpui->hwndTitle, szHostOrderGeneration)) { fprintf(stderr, "Host order generation mismatch accepted\n"); goto LDone; }
                    if (FImportLibraryOrders(app, app.lpui->hwndTitle, szHostOrderAbsentPlayer)) { fprintf(stderr, "Host order for nonexistent player accepted\n"); goto LDone; }
                    if (!FImportLibraryOrders(app, app.lpui->hwndTitle, szHostOrderFile)) { fprintf(stderr, "Host order valid file rejected\n"); goto LDone; }
                    if (!FTestFilesEqual(szHostOrderFile, szHostManagedOrders)) {
                        fprintf(stderr, "Host order target differs after import: target=%s\n", szHostManagedOrders);
                        if (FNativeFileSize(szHostManagedOrders, &cbHostFile)) fprintf(stderr, "target size=%lld\n", (long long)cbHostFile);
                        else fprintf(stderr, "target missing\n");
                        goto LDone;
                    }
                    if (!FImportLibraryOrders(app, app.lpui->hwndTitle, szHostOrderFile)) { fprintf(stderr, "Host order identical file rejected\n"); goto LDone; }
                    if (cHostOrderConflictPrompts != 0) { fprintf(stderr, "Host order no-op prompted\n"); goto LDone; }
                    idHostOrderConflictResponse = win32::IDNO;
                    if (FImportLibraryOrders(app, app.lpui->hwndTitle, szHostOrderDifferent) || cHostOrderConflictPrompts != 1 ||
                        !FTestFilesEqual(szHostOrderFile, szHostManagedOrders)) goto LDone;
                    idHostOrderConflictResponse = win32::IDYES;
                    if (!FImportLibraryOrders(app, app.lpui->hwndTitle, szHostOrderDifferent) || cHostOrderConflictPrompts != 2 ||
                        !FTestFilesEqual(szHostOrderDifferent, szHostManagedOrders)) goto LDone;
                    iHostLibraryTestStage = 8;
                    if (!FPushClick(app, win32::GetDlgItem(hwndHostDialog, IDC_HOST_GENERATE_NOW))) goto LDone;
                    return;
                }
                if (iHostLibraryTestStage == 8) {
                    CchSnprintf(std::span{szHostOrderFile, sizeof(szHostOrderFile)}, "%s.m1", szHostManagedBase);
                    if (!FNativeFileSize(szHostOrderFile, &cbHostFile)) goto LDone;
                    CchSnprintf(std::span{szHostOrderFile, sizeof(szHostOrderFile)}, "%s.x1", szHostManagedBase);
                    lpflHostOrder = LpflFromId(app, idHostOrderFleet);
                    if (FNativeFileSize(szHostOrderFile, &cbHostFile) || !lpflHostOrder ||
                        !lpflHostOrder->lpszName || strcmp(lpflHostOrder->lpszName, "Replacement order")) goto LDone;
                    if ((cchHostOrder = CchSnprintf(std::span{szSavedBase, sizeof(szSavedBase)}, "%s", app.state.szBase)) <= 0 ||
                        (size_t)cchHostOrder >= sizeof(szSavedBase) ||
                        (cchHostOrder = CchSnprintf(std::span{app.state.szBase, sizeof(app.state.szBase)}, "%s", pszHostPickerBase)) <= 0 ||
                        (size_t)cchHostOrder >= sizeof(app.state.szBase) || !FWriteLogFile(app, app.state.szBase, 0)) goto LDone;
                    strcpy(app.state.szBase, szSavedBase);
                    iHostLibraryTestStage = 9;
                    app.state.gd.fClose = 0;
                    if (!FPushClick(app, win32::GetDlgItem(hwndHostDialog, IDCANCEL))) goto LDone;
                    return;
                }
                return;
            }
            if (iHostLibraryTestStage == 9) iHostLibraryTestStage = 10;
            else return;
        }
        if (iHostLibraryTestStage == 10) {
            if (!win32::IsWindowVisible(app.lpui->hwndTitle) ||
                !(win32::GetWindowLongPtrA(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_IMPORT_ORDERS), win32::GWL_STYLE) & win32::WS_VISIBLE)) goto LDone;
            fHostOrderPickerCancel = 1;
            ui::SendMessage(app.lpui->hwndTitle, win32::WM_COMMAND, win32::MAKEWPARAM(IDC_LIBRARY_IMPORT_ORDERS, win32::BN_CLICKED),
                        (LPARAM)win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_IMPORT_ORDERS));
            if (cHostOrderPickerCalls != 1 || FNativeFileSize(szHostManagedOrders, &cbHostFile)) goto LDone;
            fHostOrderPickerCancel = 0;
            iHostLibraryTestStage = 11;
            return;
        }
        if (iHostLibraryTestStage == 11) {
            ui::SendMessage(app.lpui->hwndTitle, win32::WM_COMMAND, win32::MAKEWPARAM(IDC_LIBRARY_IMPORT_ORDERS, win32::BN_CLICKED),
                        (LPARAM)win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_IMPORT_ORDERS));
            if (cHostOrderPickerCalls != 2 || !FTestFilesEqual(szHostPickerOrder, szHostManagedOrders)) goto LDone;
            app.lpui->fFreeingTitle = 0;
            fChecked = 1;
            fPassed = 1;
            puts("Host Games visible selection, unsaved-work prompt, order picker, validation/conflicts and generation passed");
            win32::PostQuitMessage(0);
            return;
        }
    }
    if (PszCompatEnvironment("STARS_CLIENT_STARTUP_SOURCE")) {
        HWND hwndDialog;
        for (hwndDialog = win32::GetWindow(app.lpui->hwndFrame, win32::GW_HWNDFIRST); hwndDialog; hwndDialog = win32::GetWindow(hwndDialog, win32::GW_HWNDNEXT)) {
            if (hwndDialog->fDialog && hwndDialog->lpfnDlg == LibraryNameDlg && win32::IsWindowVisible(hwndDialog)) {
                if (!win32::SetDlgItemTextA(hwndDialog, IDC_EDIT1, "startup-import") || !FPushClick(app, win32::GetDlgItem(hwndDialog, IDOK))) goto LDone;
                return;
            }
        }
        if (PszCompatEnvironment("STARS_CLIENT_STARTUP_MANAGED")) {
            if (app.state.game.lid == 0 || !app.state.lpPlanets || app.state.idPlayer == iplrNone || app.lpui->hwndTitle) {
                fprintf(stderr, "Managed startup: game %ld planets %p player %d title %p base %s\n",
                        (long)app.state.game.lid, (void *)app.state.lpPlanets, app.state.idPlayer, (void *)app.lpui->hwndTitle, app.state.szBase);
                goto LDone;
            }
        } else {
            if (!win32::IsWindowVisible(app.lpui->hwndTitle)) return;
            ui::UpdateWindow(app.lpui->hwndTitle);
            if (!FCompatPresent(app.lpui->hwndTitle) || app.state.game.lid != 0 || app.state.idPlayer != iplrNone ||
                ui::SendMessage(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAMES), win32::LB_FINDSTRING, (WPARAM)-1, (LPARAM)"startup-import") == win32::LB_ERR) goto LDone;
        }
        fChecked = fPassed = fTitlePainted = 1;
        puts("Desktop startup uses the supplied game path without reopening the chooser");
        win32::PostQuitMessage(0);
        return;
    }
    if (PszCompatEnvironment("STARS_CLIENT_RACE_ROSTER_TEST") || PszCompatEnvironment("STARS_CLIENT_RACE_LIBRARY_TEST")) {
        HWND hwndSave = HwndRosterDialog(app, LibraryRaceSaveDlg);
        if (hwndSave) {
            if (!FPushClick(app, win32::GetDlgItem(hwndSave, IDC_LIBRARY_RACE_UPDATE))) goto LDone;
            return;
        }
    }
    if (PszCompatEnvironment("STARS_CLIENT_RACE_VERSION_TEST")) {
        if (!FCheckRaceVersionWorkflow(app)) goto LDone;
        return;
    }
    if (PszCompatEnvironment("STARS_CLIENT_RACE_ROSTER_TEST")) {
        if (!FCheckRaceRosterWorkflow(app)) goto LDone;
        return;
    }
    if (PszCompatEnvironment("STARS_CLIENT_RACE_NAME_CANCEL")) {
        HWND hwndTop;
        BOOL fWizard = !strcmp(PszCompatEnvironment("STARS_CLIENT_RACE_NAME_CANCEL"), "wizard");
        if (iStage == 0) {
            if (!win32::IsWindowVisible(app.lpui->hwndTitle)) return;
            fTitlePainted = fChecked = 1;
            if (!ui::SetWindowPos(app.lpui->hwndTitle, NULL, 55, 95, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE)) goto LDone;
            iStage = 1;
            if (!FPushClick(app, win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RACE_NEW))) goto LDone;
            return;
        }
        if (iStage == 1 || (fWizard && iStage == 2)) {
            for (hwndTop = HwndCompatFirstTop(); hwndTop; hwndTop = win32::GetWindow(hwndTop, win32::GW_HWNDNEXT)) {
                if (!hwndTop->fDialog || !win32::IsWindowVisible(hwndTop) ||
                    hwndTop->lpfnDlg != (iStage == 1 ? RaceWizardDlg1 : RaceWizardDlg4)) {
                    if (hwndTop->fDialog && hwndTop->lpfnDlg == LibraryNameDlg) goto LDone;
                    continue;
                }
                if (!(SDL_GetWindowFlags(app.lpui->hwndFrame->lpWindow.get()) & SDL_WINDOW_HIDDEN)) goto LDone;
                if (iStage == 2) {
                    HWND hwndRadio;
                    HDC hdcRadio;
                    RECT rcRadio;
                    int idRadio, y;
                    BOOL fFrameTop, fFrameBottom;

                    // Both horizontal focus edges must remain visible inside each radio.
                    for (idRadio = IDC_RACE_HYPER_EXPANSION; idRadio <= IDC_RACE_JACK_OF_ALL_TRADES; idRadio++) {
                        hwndRadio = win32::GetDlgItem(hwndTop, idRadio);
                        ui::SetFocus(hwndRadio);
                        if (!ui::UpdateWindow(hwndRadio) || !win32::GetClientRect(hwndRadio, &rcRadio)) goto LDone;
                        hdcRadio = win32::GetDC(hwndRadio);
                        if (!hdcRadio) goto LDone;
                        fFrameTop = fFrameBottom = 0;
                        for (y = 0; y < rcRadio.bottom; y++) {
                            if (win32::GetPixel(hdcRadio, 20, y) != win32::GetSysColor(win32::COLOR_GRAYTEXT)) continue;
                            if (y < rcRadio.bottom / 2) fFrameTop = 1;
                            else fFrameBottom = 1;
                        }
                        win32::ReleaseDC(hwndRadio, hdcRadio);
                        if (!fFrameTop || !fFrameBottom) {
                            fprintf(stderr, "PRT radio %d clips its focus frame\n", idRadio);
                            goto LDone;
                        }
                    }
                    if (pszWizardCapture && (!FCompatPaintTree(hwndTop) || !FCompatPresent(hwndTop) ||
                        !SDL_SaveBMP(hwndTop->lpComposite.get(), pszWizardCapture))) goto LDone;
                }
                if (!FPushClick(app, win32::GetDlgItem(hwndTop, iStage == 1 && fWizard ? IDC_NEXT : IDCANCEL))) goto LDone;
                iStage = fWizard && iStage == 1 ? 2 : 4;
                return;
            }
            return;
        }
        if (!win32::IsWindowEnabled(app.lpui->hwndTitle)) return;
        // Let queued native activation/show events settle after the modal closes.
        if (iStage++ < 6) return;
        for (hwndTop = HwndCompatFirstTop(); hwndTop; hwndTop = win32::GetWindow(hwndTop, win32::GW_HWNDNEXT)) {
            if (!hwndTop->lpWindow.get() || (SDL_GetWindowFlags(hwndTop->lpWindow.get()) & SDL_WINDOW_HIDDEN)) continue;
            if (hwndTop != app.lpui->hwndTitle) {
                fprintf(stderr, "Cancelling the race dialog exposed another native window (frame %d, visible %d)\n",
                        hwndTop == app.lpui->hwndFrame, win32::IsWindowVisible(hwndTop));
                goto LDone;
            }
        }
        if (win32::IsWindowVisible(app.lpui->hwndFrame) || !win32::IsWindowVisible(app.lpui->hwndTitle) ||
            (SDL_GetWindowFlags(app.lpui->hwndTitle->lpWindow.get()) & SDL_WINDOW_HIDDEN) ||
            app.state.game.lid != 0 || app.state.idPlayer != iplrNone) goto LDone;
        fPassed = 1;
        puts("Race dialog cancels back to the single library window");
        win32::PostQuitMessage(0);
        return;
    }
    if (PszCompatEnvironment("STARS_CLIENT_RECENT_STARTUP")) {
        HWND hwndTop;
        int cVisible = 0;
        if (!win32::IsWindowVisible(app.lpui->hwndTitle)) return;
        for (hwndTop = HwndCompatFirstTop(); hwndTop; hwndTop = win32::GetWindow(hwndTop, win32::GW_HWNDNEXT)) {
            if (!hwndTop->lpWindow.get() || (SDL_GetWindowFlags(hwndTop->lpWindow.get()) & SDL_WINDOW_HIDDEN)) continue;
            if (hwndTop != app.lpui->hwndTitle) goto LDone;
            cVisible++;
        }
        if (cVisible != 1 || win32::IsWindowVisible(app.lpui->hwndFrame) || cRecentMigrationPrompts != 0 ||
            app.state.game.lid != 0 || app.state.idPlayer != iplrNone || !win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAMES) ||
            !win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RACES)) goto LDone;
        ui::UpdateWindow(app.lpui->hwndTitle);
        if (!FCompatPresent(app.lpui->hwndTitle)) goto LDone;
        fTitlePainted = fChecked = fPassed = 1;
        puts("Startup with recent files shows only the library, without an import prompt");
        win32::PostQuitMessage(0);
        return;
    }
    if (PszCompatEnvironment("STARS_CLIENT_EXTERNAL_OPEN_GAME") || PszCompatEnvironment("STARS_CLIENT_EXTERNAL_OPEN_RACE")) {
        HWND hwndDialog;
        if (iStage == 0) {
            if (!win32::IsWindowVisible(app.lpui->hwndTitle)) return;
            ui::UpdateWindow(app.lpui->hwndTitle);
            if (!FCompatPresent(app.lpui->hwndTitle)) goto LDone;
            fTitlePainted = 1;
            fChecked = 1;
            iStage = 1;
            ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FILE_OPEN_GAME, 0);
            return;
        }
        if (iStage == 1) {
            for (hwndDialog = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwndDialog; hwndDialog = win32::GetWindow(hwndDialog, win32::GW_HWNDNEXT)) {
                if (!hwndDialog->fDialog || hwndDialog->lpfnDlg != LibraryNameDlg) continue;
                if (!ui::SetWindowText(win32::GetDlgItem(hwndDialog, IDC_EDIT1),
                                    PszCompatEnvironment("STARS_CLIENT_IMPORT_NAME") ? PszCompatEnvironment("STARS_CLIENT_IMPORT_NAME") : "external-open-game") ||
                    !FPushClick(app, win32::GetDlgItem(hwndDialog, IDOK))) goto LDone;
                iStage = 2;
                return;
            }
            return;
        }
        if (PszCompatEnvironment("STARS_CLIENT_EXTERNAL_OPEN_RACE")) {
            if (iStage == 2) {
                for (hwndDialog = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwndDialog; hwndDialog = win32::GetWindow(hwndDialog, win32::GW_HWNDNEXT)) {
                    if (!hwndDialog->fDialog || hwndDialog->lpfnDlg != RaceWizardDlg1 || !win32::IsWindowVisible(hwndDialog)) continue;
                    iStage = 3;
                    if (!FPushClick(app, win32::GetDlgItem(hwndDialog, IDCANCEL))) goto LDone;
                    return;
                }
                return;
            }
            if (!win32::IsWindowEnabled(app.lpui->hwndTitle)) return;
            if (ui::SendMessage(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RACES), win32::LB_FINDSTRING, (WPARAM)-1,
                            (LPARAM)PszCompatEnvironment("STARS_CLIENT_IMPORT_NAME")) == win32::LB_ERR || app.state.game.lid != 0 || app.state.idPlayer != iplrNone) goto LDone;
            fPassed = 1;
            puts("Desktop Open Game imports an external race and opens its managed edit wizard");
            win32::PostQuitMessage(0);
            return;
        }
        if (ui::SendMessage(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAMES), win32::LB_FINDSTRING, (WPARAM)-1,
                        (LPARAM)(PszCompatEnvironment("STARS_CLIENT_IMPORT_NAME") ? PszCompatEnvironment("STARS_CLIENT_IMPORT_NAME") : "external-open-game")) == win32::LB_ERR ||
            app.state.game.lid != 0 || app.state.idPlayer != iplrNone) goto LDone;
        fPassed = 1;
        puts("Desktop Open Game imports the external file into the managed library");
        win32::PostQuitMessage(0);
        return;
    }
    if (pszExportGame) {
        if (!FCheckGameExportWorkflow(app, id)) goto LDone;
        return;
    }
    if (PszCompatEnvironment("STARS_CLIENT_RACE_LIBRARY_TEST")) {
        HWND hwndRace;
        static const DLGPROC rglpfnRaceLibrary[] = {RaceWizardDlg1, RaceWizardDlg4, RaceWizardDlg5, RaceWizardDlg2, RaceWizardDlg3, RaceWizardDlg6};
        if (iRaceLibraryTestStage == 0) {
            if (!win32::IsWindowVisible(app.lpui->hwndTitle) || !win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RACES) ||
                !win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RACE_NEW) || !win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RACE_EDIT) ||
                !win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RACE_IMPORT)) {
                goto LDone;
            }
            if (!FPushClick(app, win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RACE_NEW))) goto LDone;
            iRaceLibraryTestStage = 1;
            return;
        }
        if (iRaceLibraryTestStage == 1) {
            for (hwndRace = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwndRace; hwndRace = win32::GetWindow(hwndRace, win32::GW_HWNDNEXT)) {
                if (!hwndRace->fDialog) continue;
                if (hwndRace->lpfnDlg != RaceWizardDlg1 ||
                    !win32::SetDlgItemTextA(hwndRace, IDC_RACE_NAME, "Managed") ||
                    !win32::SetDlgItemTextA(hwndRace, IDC_RACE_PLURAL_NAME, "Manageds")) goto LDone;
                iRaceLibraryTestStage = 2;
                return;
            }
            return;
        }
        if (iRaceLibraryTestStage == 2) {
            for (hwndRace = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwndRace; hwndRace = win32::GetWindow(hwndRace, win32::GW_HWNDNEXT)) {
                if (!hwndRace->fDialog) continue;
                if (!win32::IsWindowVisible(hwndRace)) return;
                i = (size_t)iRaceLibraryWizardPage;
                if (i >= sizeof(rglpfnRaceLibrary) / sizeof(rglpfnRaceLibrary[0])) goto LDone;
                if (!FPushClick(app, win32::GetDlgItem(hwndRace, i == 5 ? IDC_FINISH : IDC_NEXT))) goto LDone;
                iRaceLibraryWizardPage++;
                return;
            }
            if (iRaceLibraryWizardPage == 6 && win32::IsWindowEnabled(app.lpui->hwndTitle)) iRaceLibraryTestStage = 3;
            return;
        }
        if (iRaceLibraryTestStage == 3) {
            if (!win32::IsWindowVisible(app.lpui->hwndTitle)) return;
            if (!FLoadLibraryRace(app, app.lpui->hwndTitle, "Managed.r1")) goto LDone;
            CchSnprintf(std::span{szManaged, sizeof(szManaged)}, "%s%sRace Designs%sManaged.r1", pszSettings, szDirSep, szDirSep);
            if (!FTestCopyFile(szManaged, pszRaceSource) || !FTestCopyFile(pszRaceSource, pszRaceSnapshot)) goto LDone;
            if (!FSaveLibraryRace(app, app.lpui->hwndTitle, "Managed.r1.pending.r1", &app.state.vplr)) goto LDone;
            icksRaceSaveStage = IRaceChecksum(&app.state.vplr);
            if (!FLoadLibraryRace(app, app.lpui->hwndTitle, "Managed.r1") || IRaceChecksum(&app.state.vplr) != icksRaceSaveStage) goto LDone;
            RefreshLibraryRaces(app, app.lpui->hwndTitle);
            if (ui::SendMessage(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RACES), win32::LB_GETCOUNT, 0, 0) != 2 ||
                !FSelectClientRaceName(app.lpui->hwndTitle, "Managed.r1")) goto LDone;
            icksRaceLibraryOriginal = IRaceChecksum(&app.state.vplr);
            icksumRaceLibrary = icksRaceLibraryOriginal;
            iRaceLibraryWizardPage = 0;
            iRaceLibraryTestStage = 4;
            if (!FPushClick(app, win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RACE_EDIT))) goto LDone;
            return;
        }
        if (iRaceLibraryTestStage == 4) {
            for (hwndRace = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwndRace; hwndRace = win32::GetWindow(hwndRace, win32::GW_HWNDNEXT)) {
                if (!hwndRace->fDialog) continue;
                if (iRaceLibraryWizardPage < 6) {
                    i = (size_t)iRaceLibraryWizardPage;
                    if (i == 0 && !FPushClick(app, win32::GetDlgItem(hwndRace, IDC_RACE_HUMANOID + 1))) goto LDone;
                    if (!FPushClick(app, win32::GetDlgItem(hwndRace, i == 5 ? IDC_FINISH : IDC_NEXT))) goto LDone;
                    iRaceLibraryWizardPage++;
                    return;
                }
                goto LDone;
            }
            if (iRaceLibraryWizardPage == 6 && win32::IsWindowEnabled(app.lpui->hwndTitle)) iRaceLibraryTestStage = 5;
            return;
        }
        if (iRaceLibraryTestStage == 5) {
            if (!win32::IsWindowEnabled(app.lpui->hwndTitle)) return;
            if (!FLoadLibraryRace(app, app.lpui->hwndTitle, "Managed.r1") || IRaceChecksum(&app.state.vplr) == icksRaceLibraryOriginal) goto LDone;
            icksumRaceLibrary = IRaceChecksum(&app.state.vplr);
            RefreshLibraryRaces(app, app.lpui->hwndTitle);
            if (!FSelectClientRaceName(app.lpui->hwndTitle, "Managed.r1")) goto LDone;
            iRaceLibraryWizardPage = 0;
            if (!FPushClick(app, win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RACE_EDIT))) goto LDone;
            iRaceLibraryTestStage = 6;
            return;
        }
        if (iRaceLibraryTestStage == 6) {
            for (hwndRace = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwndRace; hwndRace = win32::GetWindow(hwndRace, win32::GW_HWNDNEXT)) {
                if (!hwndRace->fDialog) continue;
                if (!FPushClick(app, win32::GetDlgItem(hwndRace, IDCANCEL))) goto LDone;
                iRaceLibraryTestStage = 7;
                return;
            }
            return;
        }
        if (iRaceLibraryTestStage == 7) {
            if (!win32::IsWindowEnabled(app.lpui->hwndTitle)) return;
            if (!FLoadLibraryRace(app, app.lpui->hwndTitle, "Managed.r1") || IRaceChecksum(&app.state.vplr) != icksumRaceLibrary) goto LDone;
            if (FImportLibraryRaceAs(app, app.lpui->hwndTitle, PszCompatEnvironment("STARS_CLIENT_RACE_IMPORT_SOURCE"), "CON.r1") ||
                FImportLibraryRaceAs(app, app.lpui->hwndTitle, PszCompatEnvironment("STARS_CLIENT_RACE_IMPORT_SOURCE"), "COM1.r2") ||
                FImportLibraryRaceAs(app, app.lpui->hwndTitle, PszCompatEnvironment("STARS_CLIENT_RACE_IMPORT_SOURCE"), "LPT9.r3") ||
                FImportLibraryRaceAs(app, app.lpui->hwndTitle, PszCompatEnvironment("STARS_CLIENT_RACE_IMPORT_SOURCE"), "CLOCK$.r1") ||
                FImportLibraryRaceAs(app, app.lpui->hwndTitle, PszCompatEnvironment("STARS_CLIENT_RACE_IMPORT_SOURCE"), "managed.R1")) goto LDone;
            if (!FSaveLibraryRace(app, app.lpui->hwndTitle, "Imported.r2.pending.r1", &app.state.vplr)) goto LDone;
            icksRaceImportStage = IRaceChecksum(&app.state.vplr);
            if (!FLoadLibraryRace(app, app.lpui->hwndTitle, "Managed.r1") || IRaceChecksum(&app.state.vplr) != icksumRaceLibrary) goto LDone;
            if (!FPushClick(app, win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RACE_IMPORT))) goto LDone;
            iRaceLibraryTestStage = 8;
            return;
        }
        if (iRaceLibraryTestStage == 8) {
            for (hwndRace = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwndRace; hwndRace = win32::GetWindow(hwndRace, win32::GW_HWNDNEXT)) {
                if (!hwndRace->fDialog || !win32::GetDlgItem(hwndRace, IDC_EDIT1)) continue;
                if (!ui::SetWindowText(win32::GetDlgItem(hwndRace, IDC_EDIT1), "Imported.r2") || !FPushClick(app, win32::GetDlgItem(hwndRace, IDOK))) goto LDone;
                iRaceLibraryTestStage = 9;
                return;
            }
            return;
        }
        if (iRaceLibraryTestStage == 9) {
            for (hwndRace = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwndRace; hwndRace = win32::GetWindow(hwndRace, win32::GW_HWNDNEXT)) {
                if (!hwndRace->fDialog) continue;
                if (!FPushClick(app, win32::GetDlgItem(hwndRace, IDCANCEL))) goto LDone;
                iRaceLibraryTestStage = 10;
                return;
            }
            return;
        }
        if (iRaceLibraryTestStage == 10) {
            if (!win32::IsWindowEnabled(app.lpui->hwndTitle)) return;
            RefreshLibraryRaces(app, app.lpui->hwndTitle);
            if (ui::SendMessage(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RACES), win32::LB_GETCOUNT, 0, 0) != 4 ||
                !FSelectClientRaceName(app.lpui->hwndTitle, "Imported.r2") || !FSelectLibraryRace(app.lpui->hwndTitle, szRaceName) ||
                strcmp(szRaceName, "Imported.r2") || !FLoadLibraryRace(app, app.lpui->hwndTitle, szRaceName) || !IRaceChecksum(&app.state.vplr) ||
                !FLoadLibraryRace(app, app.lpui->hwndTitle, "Imported.r2.pending.r1") || IRaceChecksum(&app.state.vplr) != icksRaceImportStage) goto LDone;
            if (!FDuplicateLibraryRace(app, app.lpui->hwndTitle, "Managed.r1")) goto LDone;
            if (!FDuplicateLibraryRace(app, app.lpui->hwndTitle, "Managed.r2")) goto LDone;
            CchSnprintf(std::span{szDuplicate, sizeof(szDuplicate)}, "%s%sRace Designs%sManaged.r2", pszSettings, szDirSep, szDirSep);
            CchSnprintf(std::span{szManaged, sizeof(szManaged)}, "%s%sRace Designs%sManaged.r1", pszSettings, szDirSep, szDirSep);
            CchSnprintf(std::span{szExport, sizeof(szExport)}, "%s%sRace Designs%smanaged-export.r1", pszSettings, szDirSep, szDirSep);
            if (!FTestFilesEqual(szManaged, szDuplicate)) goto LDone;
            CchSnprintf(std::span{szDuplicate, sizeof(szDuplicate)}, "%s%sRace Designs%sManaged.r3", pszSettings, szDirSep, szDirSep);
            if (!FTestFilesEqual(szManaged, szDuplicate)) goto LDone;
            if (!FExportLibraryRace(app, app.lpui->hwndTitle, "Managed.r1", szExport) ||
                !FLoadLibraryRace(app, app.lpui->hwndTitle, "Managed.r1") || !FLoadLibraryRace(app, app.lpui->hwndTitle, "managed-export.r1")) goto LDone;
            if (!FTestFilesEqual(szManaged, szExport)) goto LDone;
            CchSnprintf(std::span{szExport, sizeof(szExport)}, "%s%sRace Designs%smanaged-export-failure.r1", pszSettings, szDirSep, szDirSep);
            CchSnprintf(std::span{szExportSnapshot, sizeof(szExportSnapshot)}, "%s%sRace Designs%smanaged-export-failure.snapshot", pszSettings, szDirSep, szDirSep);
            if (!FTestWriteFile(szExport, "preserve prior export") || !FTestCopyFile(szExport, szExportSnapshot)) goto LDone;
            CchSnprintf(std::span{szRaceExportFailTarget, sizeof(szRaceExportFailTarget)}, "%s", szExport);
            fFailRaceExportReplace = 1;
            fExpectRaceExportFailureAlert = 1;
            if (FExportLibraryRace(app, app.lpui->hwndTitle, "Managed.r1", szExport)) {
                fExpectRaceExportFailureAlert = 0;
                goto LDone;
            }
            fExpectRaceExportFailureAlert = 0;
            if (fFailRaceExportReplace || !FTestFilesEqual(szExport, szExportSnapshot)) goto LDone;
            CchSnprintf(std::span{szDuplicate, sizeof(szDuplicate)}, "%s%sRace Designs%sManaged.r2", pszSettings, szDirSep, szDirSep);
            CchSnprintf(std::span{szExport, sizeof(szExport)}, "%s%sRace Designs%sManaged.r3", pszSettings, szDirSep, szDirSep);
            if (FDeleteLibraryRace(app, app.lpui->hwndTitle, "Managed.r2") || !FNativeFileSize(szDuplicate, &cbTestFile) ||
                !FNativeFileSize(szExport, &cbTestFile) || !FNativeFileSize(szManaged, &cbTestFile)) goto LDone;
            if (FDeleteLibraryRace(app, app.lpui->hwndTitle, "Managed.r3") || !FNativeFileSize(szExport, &cbTestFile) ||
                !FNativeFileSize(szDuplicate, &cbTestFile) || !FNativeFileSize(szManaged, &cbTestFile)) goto LDone;
            if (!FDeleteLibraryRace(app, app.lpui->hwndTitle, "Managed.r2") || FNativeFileSize(szDuplicate, &cbTestFile) ||
                !FNativeFileSize(szExport, &cbTestFile) || !FNativeFileSize(szManaged, &cbTestFile)) goto LDone;
            fChecked = 1;
            fPassed = 1;
            if (!FTestRaceCancellation(app)) {
                fprintf(stderr, "Cancelled race operation changed the active design or its destination\n");
                fPassed = 0;
                goto LDone;
            }
            if (!FTestLegacyRaceImport(app)) {
                fprintf(stderr, "Legacy race import did not retain its converted names and password\n");
                fPassed = 0;
                goto LDone;
            }
            puts("Managed race created, edited, cancelled, imported, duplicated, exported, and reloaded");
            win32::PostQuitMessage(0);
            return;
        }
        return;
    }
    if (PszCompatEnvironment("STARS_CLIENT_GAME_DELETE_MODE")) {
        pszDeleteMode = PszCompatEnvironment("STARS_CLIENT_GAME_DELETE_MODE");
        hwndDeleteList = win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAMES);
        iDeleteSelected = ui::SendMessage(hwndDeleteList, win32::LB_FINDSTRING, (WPARAM)-1, (LPARAM)"selected-game");
        iDeleteOther = ui::SendMessage(hwndDeleteList, win32::LB_FINDSTRING, (WPARAM)-1, (LPARAM)"other-game");
        if (!win32::IsWindowVisible(app.lpui->hwndTitle) || !win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAME_DELETE) || iDeleteOther == win32::LB_ERR) goto LDone;
        if (iStage == 0) {
            if (iDeleteSelected == win32::LB_ERR) goto LDone;
            ui::SendMessage(hwndDeleteList, win32::LB_SETCURSEL, iDeleteSelected, 0);
            iStage = 1;
            if (!FPushClick(app, win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAME_DELETE))) goto LDone;
            return;
        }
        if (!fLibraryGameDeletePrompt) goto LDone;
        RefreshLibraryGames(app, app.lpui->hwndTitle);
        hwndDeleteList = win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAMES);
        iDeleteSelected = ui::SendMessage(hwndDeleteList, win32::LB_FINDSTRING, (WPARAM)-1, (LPARAM)"selected-game");
        iDeleteOther = ui::SendMessage(hwndDeleteList, win32::LB_FINDSTRING, (WPARAM)-1, (LPARAM)"other-game");
        fPassed = pszDeleteMode && strcmp(pszDeleteMode, "confirm") == 0 ? iDeleteSelected == win32::LB_ERR && iDeleteOther != win32::LB_ERR :
                  iDeleteSelected != win32::LB_ERR && iDeleteOther != win32::LB_ERR &&
                  (strcmp(pszDeleteMode, "cancel") == 0 || fLibraryGameDeleteFailureAlert);
        if (!fPassed || (!strcmp(pszDeleteMode, "confirm") && win32::IsWindowEnabled(app.lpui->rghwndBtnSplash[2]))) goto LDone;
        fChecked = 1;
        puts("Managed game deletion confirmed, cancelled, or failed through the Games view");
        win32::PostQuitMessage(0);
        return;
    }
    if (PszCompatEnvironment("STARS_CLIENT_GAME_LIBRARY_TEST")) {
        const char *pszMode = PszCompatEnvironment("STARS_CLIENT_GAME_LIBRARY_TEST");
        const char *pszRoot = PszCompatEnvironment("STARS_LIBRARY_DIR");
        const char *pszSource = PszCompatEnvironment("STARS_CLIENT_GAME_SOURCE_NAME");
        const char *pszCopy = PszCompatEnvironment("STARS_CLIENT_GAME_COPY_NAME");
        const char *pszRename = PszCompatEnvironment("STARS_CLIENT_GAME_RENAME_NAME");
        const char *pszCollision = PszCompatEnvironment("STARS_CLIENT_GAME_COLLISION_NAME");
        const char *pszBaseline = PszCompatEnvironment("STARS_CLIENT_GAME_BASELINE_NAME");
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
        if (!win32::IsWindowVisible(app.lpui->hwndTitle) || !pszRoot || !pszSource || !pszCopy || !pszRename || !pszCollision || !pszBaseline ||
            (!fDuplicate && !fRename) ||
            !win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_DUPLICATE) || !win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RENAME) ||
            CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%sGames%s%s", pszRoot, szDirSep, szDirSep, pszSource) <= 0 ||
            CchSnprintf(std::span{szCopy, sizeof(szCopy)}, "%s%sGames%s%s", pszRoot, szDirSep, szDirSep, pszCopy) <= 0 ||
            CchSnprintf(std::span{szRenamed, sizeof(szRenamed)}, "%s%sGames%s%s", pszRoot, szDirSep, szDirSep, pszRename) <= 0 ||
            CchSnprintf(std::span{szBaseline, sizeof(szBaseline)}, "%s%sGames%s%s", pszRoot, szDirSep, szDirSep, pszBaseline) <= 0) goto LDone;
        if (PszCompatEnvironment("STARS_CLIENT_GAME_REJECT")) {
            fExpectGameRenameFailure = 1;
            fImported = fDuplicate ? FDuplicateLibraryGame(app, app.lpui->hwndTitle, pszSource, pszCopy) :
                                     FRenameLibraryGame(app, app.lpui->hwndTitle, pszSource, pszRename);
            fExpectGameRenameFailure = 0;
            if (fImported) {
                fprintf(stderr, "Unsafe managed-game action unexpectedly succeeded\n");
                goto LDone;
            }
            fChecked = 1;
            fPassed = 1;
            puts("Unsafe managed-game action rejected");
            win32::PostQuitMessage(0);
            return;
        }
        if (fDuplicate && !FDuplicateLibraryGame(app, app.lpui->hwndTitle, pszSource, pszCopy)) goto LDone;
        if (fDuplicate) {
            CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s.xy", szSource, szDirSep, pszSource);
            {
                char szOther[512];
                CchSnprintf(std::span{szOther, sizeof(szOther)}, "%s%s%s.xy", szCopy, szDirSep, pszCopy);
                if (!FTestFilesEqual(szPath, szOther)) { fprintf(stderr, "duplicate XY mismatch\n"); goto LDone; }
            }
            CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s.m1", szSource, szDirSep, pszSource);
            {
                char szOther[512];
                CchSnprintf(std::span{szOther, sizeof(szOther)}, "%s%s%s.m1", szCopy, szDirSep, pszCopy);
                if (!FTestFilesEqual(szPath, szOther)) { fprintf(stderr, "duplicate M1 mismatch\n"); goto LDone; }
            }
            {
                char szOther[512];
                CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%sbackup.000%s%s.m1", szSource, szDirSep, szDirSep, pszSource);
                CchSnprintf(std::span{szOther, sizeof(szOther)}, "%s%sbackup.000%s%s.m1", szCopy, szDirSep, szDirSep, pszCopy);
                if (!FTestFilesEqual(szPath, szOther)) { fprintf(stderr, "duplicate backup mismatch\n"); goto LDone; }
            }
            if (FDuplicateLibraryGame(app, app.lpui->hwndTitle, pszSource, "CON") ||
                FDuplicateLibraryGame(app, app.lpui->hwndTitle, pszSource, pszCollision) ||
                FDuplicateLibraryGame(app, app.lpui->hwndTitle, pszSource, pszCopy) ||
                FDuplicateLibraryGame(app, app.lpui->hwndTitle, pszSource, szLongName)) { fprintf(stderr, "duplicate invalid/collision/path accepted\n"); goto LDone; }
        }
        if (fRename && FRenameLibraryGame(app, app.lpui->hwndTitle, pszSource, pszCollision)) {
            fprintf(stderr, "case-insensitive rename collision accepted\n"); goto LDone;
        }
        if (fRename && FRenameLibraryGame(app, app.lpui->hwndTitle, pszSource, szLongName)) {
            fprintf(stderr, "overlong rename path accepted\n"); goto LDone;
        }
        if (fDuplicate && FRenameLibraryGame(app, app.lpui->hwndTitle, pszCopy, pszSource)) {
            fprintf(stderr, "rename over source accepted\n"); goto LDone;
        }
        CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s.library", szCopy, szDirSep);
        if (fRename) CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s.library", szSource, szDirSep);
        iVersion = 0; iplr = 0; lPlayed = -1;
        if (!FTestReadLibraryMetadata(szPath, &iVersion, szMetadataName, sizeof(szMetadataName),
                                      &iplr, &lPlayed, szExchangeName, sizeof(szExchangeName))) {
            fprintf(stderr, "duplicate metadata parse failed\n"); goto LDone;
        }
        if (iVersion != 2 || strcmp(szMetadataName, fDuplicate ? pszCopy : pszSource) ||
            iplr != (fDuplicate ? -1 : 1) || lPlayed != (fDuplicate ? 0 : 123456) ||
            strcmp(szExchangeName, pszBaseline)) { fprintf(stderr, "duplicate metadata mismatch v=%d name=%s player=%d played=%lld exchange=%s\n", iVersion, szMetadataName, iplr, lPlayed, szExchangeName); goto LDone; }
        if (fDuplicate) {
            fChecked = 1;
            fPassed = 1;
            puts("Managed game duplicated with independent metadata");
            win32::PostQuitMessage(0);
            return;
        }
        CchSnprintf(std::span{szGameRenameFailTarget, sizeof(szGameRenameFailTarget)}, "%s", szRenamed);
        fFailGameRenamePublish = 1;
        fExpectGameRenameFailure = 1;
        if (FRenameLibraryGame(app, app.lpui->hwndTitle, pszSource, pszRename) || fFailGameRenamePublish) {
            fExpectGameRenameFailure = 0;
            fprintf(stderr, "rename publication failure was not rolled back\n");
            goto LDone;
        }
        fExpectGameRenameFailure = 0;
        CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s.xy", szSource, szDirSep, pszSource);
        if (!FNativeFileSize(szPath, &cbTestFile) || FNativeDirectoryExists(szRenamed)) goto LDone;
        if (!FRenameLibraryGame(app, app.lpui->hwndTitle, pszSource, pszRename)) { fprintf(stderr, "rename failed\n"); goto LDone; }
        CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s.xy", szRenamed, szDirSep, pszRename);
        {
            char szOther[512];
            CchSnprintf(std::span{szOther, sizeof(szOther)}, "%s%s%s.xy", szBaseline, szDirSep, pszBaseline);
            if (!FTestFilesEqual(szPath, szOther)) { fprintf(stderr, "renamed XY mismatch\n"); goto LDone; }
        }
        CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s.m1", szRenamed, szDirSep, pszRename);
        {
            char szOther[512];
            CchSnprintf(std::span{szOther, sizeof(szOther)}, "%s%s%s.m1", szBaseline, szDirSep, pszBaseline);
            if (!FTestFilesEqual(szPath, szOther)) { fprintf(stderr, "renamed M1 mismatch\n"); goto LDone; }
        }
        {
            char szOther[512];
            CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%sbackup.000%s%s.m1", szRenamed, szDirSep, szDirSep, pszRename);
            CchSnprintf(std::span{szOther, sizeof(szOther)}, "%s%sbackup.000%s%s.m1", szBaseline, szDirSep, szDirSep, pszBaseline);
            if (!FTestFilesEqual(szPath, szOther)) { fprintf(stderr, "renamed backup mismatch\n"); goto LDone; }
        }
        CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s.library", szRenamed, szDirSep);
        iVersion = 0; iplr = 0; lPlayed = -1;
        if (!FTestReadLibraryMetadata(szPath, &iVersion, szMetadataName, sizeof(szMetadataName),
                                      &iplr, &lPlayed, szExchangeName, sizeof(szExchangeName))) {
            fprintf(stderr, "renamed metadata parse failed\n"); goto LDone;
        }
        if (iVersion != 2 || strcmp(szMetadataName, pszRename) || iplr != 1 || lPlayed != 123456 ||
            strcmp(szExchangeName, pszBaseline)) { fprintf(stderr, "renamed metadata mismatch %d %s\n", iVersion, szMetadataName); goto LDone; }
        CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%sexchange.map", szRenamed, szDirSep);
        if (!FNativeFileSize(szPath, &cbTestFile)) { fprintf(stderr, "renamed sidecar missing %s\n", szPath); goto LDone; }
        fChecked = 1;
        fPassed = 1;
        puts("Managed game renamed with session metadata and backup contents");
        win32::PostQuitMessage(0);
        return;
    }
    if (pszImportSource) {
        if (iImportStage == 0) {
            if (!win32::IsWindowVisible(app.lpui->hwndTitle)) return;
            iImportStage = 1;
            gameImportSav = app.state.game;
            idPlayerImportSav = app.state.idPlayer;
            strcpy(szBaseImportSav, app.state.szBase);
            if (!FImportLibraryGame(app, app.lpui->hwndTitle, pszImportSource)) goto LDone;
            if (memcmp(&gameImportSav, &app.state.game, sizeof(app.state.game)) || idPlayerImportSav != app.state.idPlayer || strcmp(szBaseImportSav, app.state.szBase)) goto LDone;
            iImportStage = 2;
            RefreshLibraryGames(app, app.lpui->hwndTitle);
            {
                int cExpected = PszCompatEnvironment("STARS_CLIENT_IMPORT_EXPECTED_PLAYERS") ?
                    atoi(PszCompatEnvironment("STARS_CLIENT_IMPORT_EXPECTED_PLAYERS")) : 2;
                int iplrTest;
                if (ui::SendDlgItemMessage(app.lpui->hwndTitle, IDC_LIBRARY_GAMES, win32::LB_GETCOUNT, 0, 0) != (cExpected != 0) ||
                    ui::SendDlgItemMessage(app.lpui->hwndTitle, IDC_LIBRARY_PLAYERS, win32::CB_GETCOUNT, 0, 0) != cExpected) goto LDone;
                for (iplrTest = 0; iplrTest < cExpected; iplrTest++)
                    if (!FClientLibraryPlayerYear(app.lpui->hwndTitle, iplrTest, 2400)) goto LDone;
            }
            fChecked = 1;
            fPassed = 1;
            puts("Game family imported into the managed library");
            win32::PostQuitMessage(0);
            return;
        }
        if (iImportStage == 1) {
            for (hwnd = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwnd; hwnd = win32::GetWindow(hwnd, win32::GW_HWNDNEXT)) {
                if (!hwnd->fDialog || hwnd->lpfnDlg != LibraryNameDlg) continue;
                if (!ui::SetWindowText(win32::GetDlgItem(hwnd, IDC_EDIT1), PszCompatEnvironment("STARS_CLIENT_IMPORT_NAME") ? PszCompatEnvironment("STARS_CLIENT_IMPORT_NAME") : "imported-game")) goto LDone;
                FPushClick(app, win32::GetDlgItem(hwnd, IDOK));
                return;
            }
        }
        return;
    }
    if (PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_SOURCE")) {
        pszExpected = PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_EXPECTED");
        if (iImportUpdateStage == 0) {
            if (!win32::IsWindowVisible(app.lpui->hwndTitle)) return;
            iImportUpdateStage = 1;
            cAlerts = 0;
            fImported = FImportLibraryGame(app, app.lpui->hwndTitle, PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_SOURCE"));
            fPassed = pszExpected && ((!strcmp(pszExpected, "fail") && !fImported && cAlerts > 0) ||
                ((!strcmp(pszExpected, "identical") || !strcmp(pszExpected, "later") ||
                  !strcmp(pszExpected, "player16") ||
                  !strcmp(pszExpected, "adjacent") || !strcmp(pszExpected, "replace-conflict") ||
                  !strcmp(pszExpected, "separate-conflict")) && fImported && cAlerts == 0) ||
                (!strcmp(pszExpected, "fail-conflict") && !fImported && cAlerts > 0 &&
                 (!PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_FAIL_CONFLICT_RESTORE") || fConflictRestoreAlert)) ||
                ((!strcmp(pszExpected, "cancel") || !strcmp(pszExpected, "cancel-conflict")) && !fImported && cAlerts == 0));
            if (fPassed && (!strcmp(pszExpected, "later") || !strcmp(pszExpected, "adjacent"))) {
                pszYear = !strcmp(pszExpected, "adjacent") ? "2408" : "2425";
                hwndList = win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAMES);
                pszTarget = PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_TARGET");
                RefreshLibraryGames(app, app.lpui->hwndTitle);
                fPassed = pszTarget && FSelectClientGameName(app.lpui->hwndTitle, pszTarget) &&
                    FClientLibraryPlayerYear(app.lpui->hwndTitle, 0, atoi(pszYear)) && FClientLibraryPlayerYear(app.lpui->hwndTitle, 1, 2400);
                if (!strcmp(pszExpected, "later")) {
                    fPassed = fPassed && FSelectClientGameName(app.lpui->hwndTitle, "managed-copy") &&
                        FClientLibraryPlayerYear(app.lpui->hwndTitle, 0, 2408);
                }
            }
            if (fPassed && !strcmp(pszExpected, "player16")) {
                RefreshLibraryGames(app, app.lpui->hwndTitle);
                pszTarget = PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_TARGET");
                fPassed = pszTarget && FSelectClientGameName(app.lpui->hwndTitle, pszTarget) &&
                    ui::SendDlgItemMessage(app.lpui->hwndTitle, IDC_LIBRARY_PLAYERS, win32::CB_GETCOUNT, 0, 0) == 3 &&
                    FClientLibraryPlayerYear(app.lpui->hwndTitle, 15, 2401);
            }
            fChecked = 1;
            win32::PostQuitMessage(fPassed ? 0 : 93);
            return;
        }
        if (iImportUpdateStage == 1) {
            for (hwnd = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwnd; hwnd = win32::GetWindow(hwnd, win32::GW_HWNDNEXT)) {
                if (!hwnd->fDialog || hwnd->lpfnDlg != LibraryTurnTargetDlg) continue;
                hwndListTarget = win32::GetDlgItem(hwnd, IDC_LIBRARY_TURN_TARGET_LIST);
                if (win32::IsWindowEnabled(win32::GetDlgItem(hwnd, IDOK))) goto LDone;
                pszTarget = PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_TARGET");
                c = (int)ui::SendMessage(hwndListTarget, win32::LB_GETCOUNT, 0, 0);
                for (iTarget = 0; iTarget < c; iTarget++) {
                    ui::SendMessage(hwndListTarget, win32::LB_GETTEXT, iTarget, (LPARAM)szName);
                    if (pszTarget && !strcmp(pszTarget, szName)) break;
                }
                if (iTarget == c) { FPushClick(app, win32::GetDlgItem(hwnd, IDCANCEL)); return; }
                ui::SendMessage(hwndListTarget, win32::LB_SETCURSEL, iTarget, 0);
                ui::SendMessage(hwnd, win32::WM_COMMAND, win32::MAKEWPARAM(IDC_LIBRARY_TURN_TARGET_LIST, win32::LBN_SELCHANGE),
                            (LPARAM)hwndListTarget);
                FPushClick(app, win32::GetDlgItem(hwnd, !strcmp(pszExpected, "cancel") ? IDCANCEL : IDOK));
                return;
            }
            for (hwnd = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwnd; hwnd = win32::GetWindow(hwnd, win32::GW_HWNDNEXT)) {
                if (!hwnd->fDialog || hwnd->lpfnDlg != LibraryTurnConflictDlg) continue;
                if (pszExpected && !strcmp(pszExpected, "cancel-conflict"))
                    FPushClick(app, win32::GetDlgItem(hwnd, IDCANCEL));
                else if (pszExpected && !strcmp(pszExpected, "replace-conflict"))
                    FPushClick(app, win32::GetDlgItem(hwnd, IDC_LIBRARY_TURN_REPLACE));
                else if (pszExpected && !strcmp(pszExpected, "fail-conflict"))
                    FPushClick(app, win32::GetDlgItem(hwnd, IDC_LIBRARY_TURN_REPLACE));
                else if (pszExpected && !strcmp(pszExpected, "separate-conflict"))
                    FPushClick(app, win32::GetDlgItem(hwnd, IDC_LIBRARY_TURN_SEPARATE));
                else goto LDone;
                return;
            }
            if (pszExpected && !strcmp(pszExpected, "separate-conflict")) {
                for (hwnd = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwnd; hwnd = win32::GetWindow(hwnd, win32::GW_HWNDNEXT)) {
                    if (!hwnd->fDialog || hwnd->lpfnDlg != LibraryNameDlg) continue;
                    pszCopyName = PszCompatEnvironment("STARS_CLIENT_IMPORT_COPY_NAME");
                    if (pszCopyName) ui::SetWindowText(win32::GetDlgItem(hwnd, IDC_EDIT1), pszCopyName);
                    FPushClick(app, win32::GetDlgItem(hwnd, IDOK));
                    return;
                }
            }
        }
        return;
    }
    if (PszCompatEnvironment("STARS_CLIENT_LIBRARY_OPEN")) {
        const char *pszName = PszCompatEnvironment("STARS_CLIENT_LIBRARY_OPEN");
        HWND hwndList, hwndDialog;
        LRESULT iGame;
        if (iStage == 0) {
            const char *pszPlayer = PszCompatEnvironment("STARS_CLIENT_LIBRARY_OPEN_PLAYER");
            int iWanted = pszPlayer ? atoi(pszPlayer) - 1 : -1;
            if (!win32::IsWindowVisible(app.lpui->hwndTitle) || !FSelectClientGameName(app.lpui->hwndTitle, pszName)) goto LDone;
            hwndList = win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAMES);
            iGame = ui::SendMessage(hwndList, win32::LB_GETCURSEL, 0, 0);
            if (iWanted >= 0 && !FSelectClientLibraryPlayer(app.lpui->hwndTitle, iWanted)) goto LDone;
            if (PszCompatEnvironment("STARS_DEBUG_LIBRARY_STARTUP"))
                fprintf(stderr, "[DEBUG-library-startup] Opening library row %lld: %s\n", (long long)iGame, pszName);
            fChecked = 1;
            iStage = 1;
            if (PszCompatEnvironment("STARS_CLIENT_LIBRARY_DOUBLECLICK")) {
                if (!win32::PostMessageA(app.lpui->hwndTitle, win32::WM_COMMAND, win32::MAKEWPARAM(IDC_LIBRARY_GAMES, win32::LBN_DBLCLK),
                                  (LPARAM)hwndList)) goto LDone;
            } else if (!FPushClick(app, app.lpui->rghwndBtnSplash[1])) goto LDone;
            return;
        }
        if (PszCompatEnvironment("STARS_CLIENT_SWITCH_CHOICE") && iStage == 20 && win32::IsWindow(app.lpui->hwndTitle)) {
            const char *pszName = PszCompatEnvironment("STARS_CLIENT_LIBRARY_OPEN");
            if (!FSelectClientGameName(app.lpui->hwndTitle, pszName) || !FSelectClientLibraryPlayer(app.lpui->hwndTitle, 0)) goto LDone;
            iStage = 21;
            if (!FPushClick(app, app.lpui->rghwndBtnSplash[1])) goto LDone;
            return;
        }
        if (win32::IsWindow(app.lpui->hwndTitle)) {
            if (PszCompatEnvironment("STARS_CLIENT_LIBRARY_DOUBLECLICK")) {
                for (hwndDialog = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwndDialog;
                     hwndDialog = win32::GetWindow(hwndDialog, win32::GW_HWNDNEXT)) {
                    if (!hwndDialog->fDialog) continue;
                    fprintf(stderr, "Game double-click unexpectedly opened a dialog\n");
                    win32::EndDialog(hwndDialog, 0);
                    goto LDone;
                }
            }
            return;
        }
        if (!app.state.lpPlanets || app.state.game.lid == 0 || app.state.idPlayer == iplrNone || !win32::IsWindowVisible(app.lpui->hwndFrame)) goto LDone;
        if (PszCompatEnvironment("STARS_CLIENT_SWITCH_CHOICE") && iStage == 21) {
            const char *pszExpected = PszCompatEnvironment("STARS_CLIENT_EXPECT_SWITCHED_PLAYER");
            if (pszExpected && app.state.idPlayer != atoi(pszExpected) - 1) goto LDone;
        } else if (PszCompatEnvironment("STARS_CLIENT_EXPECT_PLAYER") &&
                   app.state.idPlayer != atoi(PszCompatEnvironment("STARS_CLIENT_EXPECT_PLAYER")) - 1) goto LDone;
        if (PszCompatEnvironment("STARS_CLIENT_SWITCH_CANCEL_TEST")) {
            int16_t idPlayerBefore = app.state.idPlayer;
            uint16_t turnBefore = app.state.game.turn;
            long long lPlayedBefore, lPlayedAfter;
            if (!FTestLibraryTimestamp(app, &lPlayedBefore)) goto LDone;
            app.state.game.fDirty = 1;
            ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FILE_RETURN_TO_TITLE, 0);
            if (!fSwitchCancelSent || win32::IsWindow(app.lpui->hwndTitle) || !app.state.lpPlanets || app.state.idPlayer != idPlayerBefore ||
                app.state.game.turn != turnBefore || !app.state.game.fDirty || !FTestLibraryTimestamp(app, &lPlayedAfter) ||
                lPlayedAfter != lPlayedBefore) goto LDone;
            fPassed = 1;
            puts("Canceling a player switch keeps the active player turn loaded");
            win32::PostQuitMessage(0);
            return;
        }
        if (PszCompatEnvironment("STARS_CLIENT_SWITCH_CHOICE")) {
            const char *pszChoice = PszCompatEnvironment("STARS_CLIENT_SWITCH_CHOICE");
            const char *pszFailure = PszCompatEnvironment("STARS_CLIENT_SWITCH_FAIL_SAVE");
            int fSave = !strcmp(pszChoice, "save") || !strcmp(pszChoice, "submit");
            int fSubmit = !strcmp(pszChoice, "submit");
            if (iStage != 21) {
                char szBlocked[256];
                int16_t idPlayerBefore = app.state.idPlayer;
                int16_t iResearch = (app.state.rgplr[app.state.idPlayer].iTechNow() + 1) % 6;
                PLANET *lpPlanetsBefore = app.state.lpPlanets;
                if (pszFailure) {
                    // This startup timer also runs inside the research modal
                    // loop. Let its workflow timer finish before starting again.
                    if (iWorkflow != 0) return;
                    if (!FRunWorkflow(app, 1, iResearch)) goto LDone;
                    SetSzWorkFromDt(app, !strcmp(pszFailure, "orders") ? dtLog : dtHist, app.state.idPlayer);
                    strcpy(szBlocked, app.state.szWork);
                    if (!SDL_CreateDirectory(szBlocked)) goto LDone;
                    app.state.game.fDirty = 1;
                    app.state.gd.fSubmit = 0;
                    ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FILE_RETURN_TO_TITLE, 0);
                    if (win32::IsWindow(app.lpui->hwndTitle) || app.state.lpPlanets != lpPlanetsBefore || app.state.idPlayer != idPlayerBefore ||
                        !app.state.game.fDirty || app.state.gd.fSubmit || app.state.rgplr[app.state.idPlayer].iTechNow() != iResearch || cAlerts != 2)
                        goto LDone;
                    if (RemoveNativePath(szBlocked) != 0) goto LDone;
                    puts("Failed save preserves the active player and pending research; retrying the save");
                }
                cSwitchLogWrites = 0;
                cSwitchHistoryWrites = 0;
                app.state.game.fDirty = 1;
                app.state.gd.fSubmit = 0;
                ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FILE_RETURN_TO_TITLE, 0);
                if (!win32::IsWindow(app.lpui->hwndTitle) || app.state.idPlayer != iplrNone || app.state.game.fDirty ||
                    cSwitchLogWrites != fSave || cSwitchHistoryWrites != fSave || app.state.gd.fSubmit != fSubmit)
                    goto LDone;
                iStage = 20;
                return;
            }
            if (!app.state.lpPlanets || app.state.idPlayer != 0) goto LDone;
            fPassed = 1;
            printf("Switch prompt %s %s log and history; switched to Player 1\n", pszChoice,
                   fSave ? "saved" : "discarded");
            win32::PostQuitMessage(0);
            return;
        }
        fPassed = 1;
        puts("Managed game: visible in library and reopened after restart");
        win32::PostQuitMessage(0);
        return;
    }
    if (pszNewGame) {
        if (!FCheckNewGameWorkflow(app)) goto LDone;
        return;
    }
    if (iStage == 10) return;
    lpFrame = LpCompatWindow(app.lpui->hwndFrame);
    lpTitle = LpCompatWindow(app.lpui->hwndTitle);
    if (!win32::GetClientRect(app.lpui->hwndFrame, &rcFrame) || rcFrame.right < 320 || rcFrame.bottom < 240) goto LDone;
    if (iStage == 5) {
        // File loading pumps messages while building the client; check only
        // after the title's Continue handler has finished replacing it.
        if (win32::IsWindow(app.lpui->hwndTitle)) return;
        if (win32::IsWindow(app.lpui->hwndTitle) || !win32::IsWindowVisible(app.lpui->hwndFrame) || !win32::IsWindow(app.lpui->hwndScanner) ||
            !app.state.lpPlanets || app.state.game.lid == 0 || app.state.idPlayer == iplrNone) goto LDone;
        if (app.lpui->vfs.dx != rcFrame.right || app.lpui->vfs.dy != rcFrame.bottom ||
            !win32::GetClientRect(app.lpui->hwndScanner, &rcScanner) || rcScanner.right < 100 || rcScanner.bottom < 100) goto LDone;
        for (i = 0; i < 6; i++) if (!app.lpui->rgtilePlanet[i].fPopped) goto LDone;
        for (i = 0; i < 7; i++) if (!app.lpui->rgtileShip[i].fPopped) goto LDone;
        ui::UpdateWindow(app.lpui->hwndFrame);
        for (hwnd = win32::GetWindow(app.lpui->hwndFrame, win32::GW_CHILD); hwnd; hwnd = win32::GetWindow(hwnd, win32::GW_HWNDNEXT)) ui::UpdateWindow(hwnd);
        if (!FCompatPresent(app.lpui->hwndFrame) || (pszGameCapture && !SDL_SaveBMP(lpFrame->lpComposite.get(), pszGameCapture))) goto LDone;
        ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_VIEW_BROWSER_TOGGLE, 0);
        iStage = 6;
        return;
    }
    if (iStage == 6 || iStage == 8) {
        if (!win32::IsWindowVisible(app.lpui->hwndBrowser) || !win32::IsWindow(app.lpui->hwndBrowserChild)) goto LDone;
        ui::UpdateWindow(app.lpui->hwndBrowser);
        ui::UpdateWindow(app.lpui->hwndBrowserChild);
        if (!FCompatPresent(app.lpui->hwndBrowser)) goto LDone;
        hwndBrowserPrevious = app.lpui->hwndBrowser;
        if (iStage == 6) {
            if (!FPushClick(app, win32::GetDlgItem(app.lpui->hwndBrowser, IDCANCEL))) goto LDone;
        } else {
            event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
            event.window.windowID = app.lpui->hwndBrowser->id;
            if (!SDL_PushEvent(&event)) goto LDone;
        }
        iStage++;
        return;
    }
    if (iStage == 7 || iStage == 9) {
        if (win32::IsWindow(hwndBrowserPrevious) || app.lpui->hwndBrowser || !win32::IsWindowEnabled(app.lpui->hwndFrame)) goto LDone;
        if (iStage == 7) {
            ui::SendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_VIEW_BROWSER_TOGGLE, 0);
            iStage = 8;
        } else {
            // These checks may pump an already queued timer message.
            iStage = 10;
            win32::KillTimer(NULL, id);
            fprintf(stderr, "Starting loaded-game inspection checks\n");
            if (!FCheckToolbarPlacement(app)) { fprintf(stderr, "Toolbar check failed\n"); goto LDone; }
            if (!FCheckScannerDrag(app)) { fprintf(stderr, "Scanner drag check failed\n"); goto LDone; }
            fprintf(stderr, "Starting map inspection check\n");
            if (!FCheckMapInspection(app)) { fprintf(stderr, "Map inspection check failed\n"); goto LDone; }
            fprintf(stderr, "Starting report check\n");
            if (!FCheckReportScrollbars(app)) { fprintf(stderr, "Report scroll check failed\n"); goto LDone; }
            if (!FCheckGameplayWorkflows(app)) goto LDone;
            fPassed = 1;
            win32::PostQuitMessage(0);
        }
        return;
    }
    if (!lpFrame || !lpTitle || lpFrame == lpTitle || !win32::IsWindowVisible(app.lpui->hwndTitle) ||
        !lpFrame->lpWindow.get() || !lpTitle->lpWindow.get() || lpFrame->id == lpTitle->id ||
        !lpTitle->lpRenderer.get() || !lpTitle->lpCanvas.get() ||
        (SDL_GetWindowFlags(lpTitle->lpWindow.get()) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS)) ||
        (win32::GetWindowLongPtrA(app.lpui->hwndTitle, win32::GWL_STYLE) & win32::WS_CAPTION) != win32::WS_CAPTION ||
        !win32::GetClientRect(app.lpui->hwndTitle, &rcScanner) || rcScanner.right < 600 || rcScanner.bottom < 400 ||
        rcScanner.right > 800 || rcScanner.bottom > 600) goto LDone;
    if (iStage == 1) {
        for (hwndWizard = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwndWizard; hwndWizard = win32::GetWindow(hwndWizard, win32::GW_HWNDNEXT)) {
            if (hwndWizard->fDialog && hwndWizard->fModal && hwndWizard->hwndOwner == app.lpui->hwndTitle) break;
        }
        if (!hwndWizard) {
            if (++cWait < 20) return;
            goto LDone;
        }
        if (!win32::IsWindowVisible(hwndWizard) || win32::IsWindowEnabled(app.lpui->hwndTitle) ||
            (strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0 &&
             SDL_GetWindowParent(hwndWizard->lpWindow.get()) != lpTitle->lpWindow.get())) goto LDone;
        ui::UpdateWindow(hwndWizard);
        for (hwnd = win32::GetWindow(hwndWizard, win32::GW_CHILD); hwnd; hwnd = win32::GetWindow(hwnd, win32::GW_HWNDNEXT)) ui::UpdateWindow(hwnd);
        if (!FCompatPresent(hwndWizard) || (pszWizardCapture &&
            !SDL_SaveBMP(hwndWizard->lpComposite.get(), pszWizardCapture))) goto LDone;
        if (pszRaceCapture) {
            if (hwndWizard->lpfnDlg == NewGameDlg) {
                if (!FPushClick(app, win32::GetDlgItem(hwndWizard, IDC_NEXT))) goto LDone;
                return;
            }
            if (hwndWizard->lpfnDlg != NewGameDlg2 || !FChooseRosterAction(app, hwndWizard, 0, 2)) goto LDone;
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
        for (hwndRace = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwndRace; hwndRace = win32::GetWindow(hwndRace, win32::GW_HWNDNEXT)) {
            if (hwndRace->fDialog && hwndRace->fModal && hwndRace->hwndOwner == hwndWizard &&
                hwndRace->lpfnDlg == rglpfnRace[iRacePage]) break;
        }
        if (!hwndRace) {
            if (++cWait < 20) return;
            goto LDone;
        }
        ui::UpdateWindow(hwndRace);
        for (hwnd = win32::GetWindow(hwndRace, win32::GW_CHILD); hwnd; hwnd = win32::GetWindow(hwnd, win32::GW_HWNDNEXT)) ui::UpdateWindow(hwnd);
        if (!win32::IsWindowVisible(hwndRace) || win32::IsWindowEnabled(hwndWizard) || !fRacePresentation || !FCompatPresent(hwndRace)) goto LDone;
        if (iRacePage == 0) idRaceWindow = hwndRace->id;
        else if (hwndRace->id != idRaceWindow || hwndRace == hwndRacePrevious || win32::IsWindow(hwndRacePrevious)) goto LDone;
        hwndRacePrevious = hwndRace;
        if (iRacePage == 3 && (!fHabPainted || !SDL_SaveBMP(hwndRace->lpComposite.get(), pszRaceCapture))) goto LDone;
        if (iRacePage == 5 && !fRaceBack) {
            if (!FPushClick(app, win32::GetDlgItem(hwndRace, IDC_BACK))) goto LDone;
            fRaceBack = 1;
            iRacePage--;
            cWait = 0;
            return;
        }
        if (iRacePage < 5) {
            if (!FPushClick(app, win32::GetDlgItem(hwndRace, IDC_NEXT))) goto LDone;
            iRacePage++;
            cWait = 0;
            return;
        }
        if (!FPushClick(app, win32::GetDlgItem(hwndRace, IDCANCEL))) goto LDone;
        iStage = 4;
        cWait = 0;
        return;
    }
    if (iStage == 4) {
        if (win32::IsWindow(hwndRace)) {
            if (++cWait < 20) return;
            goto LDone;
        }
        if (SDL_GetWindowFromID(idRaceWindow) || !win32::IsWindowEnabled(hwndWizard) ||
            !FPushClick(app, win32::GetDlgItem(hwndWizard, IDCANCEL))) goto LDone;
        iStage = 2;
        cWait = 0;
        return;
    }
    if (iStage == 2) {
        if (win32::IsWindow(hwndWizard)) {
            if (++cWait < 20) return;
            goto LDone;
        }
        if (!win32::IsWindowEnabled(app.lpui->hwndTitle)) goto LDone;
        win32::KillTimer(NULL, id);
        fPassed = 1;
        event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
        event.window.windowID = lpTitle->id;
        if (!SDL_PushEvent(&event)) fPassed = 0;
        goto LDone;
    }
    fChecked = 1;
    if (!win32::IsWindowVisible(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_TUTORIAL))) goto LDone;
    ui::UpdateWindow(app.lpui->hwndTitle);
    for (iButton = 0; iButton < 4; iButton++) {
        if (!win32::IsWindow(app.lpui->rghwndBtnSplash[iButton]) || !win32::IsWindowVisible(app.lpui->rghwndBtnSplash[iButton])) goto LDone;
        ui::UpdateWindow(app.lpui->rghwndBtnSplash[iButton]);
    }
    rgb = (BYTE *)lpTitle->lpCanvas->pixels;
    cb = (size_t)lpTitle->lpCanvas->pitch * lpTitle->lpCanvas->h;
    for (i = 4; i + 4 <= cb; i += 4) {
        if (memcmp(rgb, rgb + i, 4)) { fDifferent = 1; break; }
    }
    if (!fDifferent || !FCompatPresent(lpTitle) || !lpTitle->lpComposite.get() ||
        (pszCapture && !SDL_SaveBMP(lpTitle->lpComposite.get(), pszCapture))) goto LDone;
    printf("Stars title %dx%d, renderer %s; independent frame and title windows\n",
           lpTitle->lpCanvas->w, lpTitle->lpCanvas->h, SDL_GetRendererName(lpTitle->lpRenderer.get()));
    if (pszContinue && PszCompatEnvironment("STARS_CLIENT_EXPECT_PLAYER")) {
        char szContinue[320];
        const char *pszGame = PszCompatEnvironment("STARS_CLIENT_EXPECT_CONTINUE_GAME");
        const char *pszYear = PszCompatEnvironment("STARS_CLIENT_EXPECT_CONTINUE_YEAR");
        char szPlayer[32];
        win32::GetWindowTextA(app.lpui->rghwndBtnSplash[2], szContinue, sizeof(szContinue));
        if (strcmp(szContinue, PszGetCompressedString(app, idsContinueGame).data())) goto LDone;
        win32::GetDlgItemTextA(app.lpui->hwndTitle, IDC_LIBRARY_CONTINUE_DETAILS, szContinue, sizeof(szContinue));
        if (pszGame ? !strstr(szContinue, pszGame) : !strstr(szContinue, "Continue: managed-copy")) goto LDone;
        CchSnprintf(std::span{szPlayer, sizeof(szPlayer)}, "Player %s",
                    PszCompatEnvironment("STARS_CLIENT_EXPECT_PLAYER") ? PszCompatEnvironment("STARS_CLIENT_EXPECT_PLAYER") : "2");
        if (!strstr(szContinue, szPlayer)) goto LDone;
        if (pszYear) {
            char szYear[32];
            CchSnprintf(std::span{szYear, sizeof(szYear)}, "(%s)", pszYear);
            if (!strstr(szContinue, szYear)) goto LDone;
        }
    }
    if (pszContinue && PszCompatEnvironment("STARS_CLIENT_EXPECT_CONTINUE_DISABLED")) {
        if (win32::IsWindowEnabled(app.lpui->rghwndBtnSplash[2])) goto LDone;
        fPassed = 1;
        puts("Continue is disabled when the managed game has no available player turns");
        win32::PostQuitMessage(0);
        return;
    }
    if (pszContinue && PszCompatEnvironment("STARS_CLIENT_EXPECT_SELECTOR")) {
        char szLabel[256];
        HWND hwndList = win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_GAMES);
        if (!win32::IsWindowEnabled(app.lpui->rghwndBtnSplash[2])) goto LDone;
        win32::GetDlgItemTextA(app.lpui->hwndTitle, IDC_LIBRARY_CONTINUE_DETAILS, szLabel, sizeof(szLabel));
        if (!strstr(szLabel, "Choose Player")) goto LDone;
        if (iStage != 30) {
            iStage = 30;
            if (!FPushClick(app, app.lpui->rghwndBtnSplash[2])) goto LDone;
            return;
        }
        if (!hwndList || ui::SendMessage(hwndList, win32::LB_GETTEXT,
            ui::SendMessage(hwndList, win32::LB_GETCURSEL, 0, 0), (LPARAM)szLabel) == win32::LB_ERR ||
            strcmp(szLabel, "managed-copy") || !win32::IsWindow(app.lpui->hwndTitle) ||
            win32::GetFocus() != win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_PLAYERS) ||
            ui::SendDlgItemMessage(app.lpui->hwndTitle, IDC_LIBRARY_PLAYERS, win32::CB_GETCURSEL, 0, 0) != win32::CB_ERR ||
            win32::IsWindowEnabled(app.lpui->rghwndBtnSplash[1])) goto LDone;
        fPassed = 1;
        puts("Continue selects the managed game when its remembered player is unavailable");
        win32::PostQuitMessage(0);
        return;
    }
    if (!FPushClick(app, app.lpui->rghwndBtnSplash[pszContinue ? 2 : 0])) goto LDone;
    iStage = pszContinue ? 5 : 1;
    return;
LDone:
    if (!fPassed) {
        if (PszCompatEnvironment("STARS_CLIENT_HOST_LIBRARY_TEST"))
            fprintf(stderr, "Host library workflow failed at stage %d\n", iHostLibraryTestStage);
        fprintf(stderr, "Client startup/wizard check failed at stage %d (new-game step %d, choices %d, creating %d): %s\n",
                iStage, iNewGameStep, cNewGameChoices, fCreatingGame, SDL_GetError());
        win32::KillTimer(NULL, id);
        win32::PostQuitMessage(92);
    }
}

int16_t StarsTest_InitInstance(APPSESSION &app, int16_t nCmdShow) {
    int16_t fResult = InitInstance(app, nCmdShow);
    fInitComplete = 1;
    if (fResult) idTimer = win32::SetTimer(NULL, 0, 250, CheckStartup);
    return fResult && idTimer ? fResult : 0;
}

int main(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szCommand[256] = "";
    int nResult;
    ALERTIO io_alertio;
    io_alertio.pfnAlertSz = StarsTest_AlertSz;
    ALERTIOSCOPE scope_alertio(io_alertio);
    PLAYERCHOICEIO io_playerchoiceio;
    io_playerchoiceio.pfnPromptPassword = StarsTest_PromptPassword;
    io_playerchoiceio.pfnFPromptLegacyRacePassword = StarsTest_FPromptLegacyRacePassword;
    PLAYERCHOICEIOSCOPE scope_playerchoiceio(io_playerchoiceio);
    DESKTOPCHOICEIO io_desktopchoiceio;
    io_desktopchoiceio.pfnInitInstance = StarsTest_InitInstance;
    io_desktopchoiceio.pfnAskSaveDialog = StarsTest_AskSaveDialog;
    io_desktopchoiceio.pfnFNativeOpenFileName = StarsTest_FNativeOpenFileName;
    DESKTOPCHOICEIOSCOPE scope_desktopchoiceio(io_desktopchoiceio);
    COMPATCHOICEIO io_compatchoiceio;
    io_compatchoiceio.pfnTrackPopupMenu = StarsTest_TrackPopupMenu;
    io_compatchoiceio.pfnMessageBoxA = StarsTest_MessageBoxA;
    COMPATCHOICEIOSCOPE scope_compatchoiceio(io_compatchoiceio);
    COMPATRENDERIO io_compatrenderio;
    io_compatrenderio.pfnRenderPresent = StarsTest_SDL_RenderPresent;
    COMPATRENDERIOSCOPE scope_compatrenderio(io_compatrenderio);
    COMPATWINDOWIO io_compatwindowio;
    io_compatwindowio.pfnShowWindow = StarsTest_SDL_ShowWindow;
    io_compatwindowio.pfnSetWindowTitle = StarsTest_SDL_SetWindowTitle;
    COMPATWINDOWIOSCOPE scope_compatwindowio(io_compatwindowio);
    COMPATPATHIO io_compatpathio;
    io_compatpathio.pfnGetPrefPath = StarsTest_SDL_GetPrefPath;
    COMPATPATHIOSCOPE scope_compatpathio(io_compatpathio);
    COMPATFOLDERIO io_folder;
    io_folder.pfnShowOpenFolderDialog = StarsTest_SDL_ShowOpenFolderDialog;
    COMPATFOLDERIOSCOPE folderScope(io_folder);
    DESKTOPCHOICEIO io_workflow = DesktopChoiceIO();
    io_workflow.pfnNewGameWizard = StarsTest_NewGameWizard;
    DESKTOPCHOICEIOSCOPE workflowScope(io_workflow);
    RECORDIO io_newgame;
    io_newgame.pfnFCreateFile = StarsTest_FCreateFile;
    RECORDIOSCOPE newGameScope(io_newgame);

    NATIVEPUBLISHIO io;
    io.pfnRemove = IRemoveClientFile; io.pfnRename = IRenameClientFile;
    NATIVEPUBLISHIOSCOPE publicationScope(io);
    if (PszCompatEnvironment("STARS_CLIENT_STARTUP_SOURCE"))
        CchSnprintf(std::span{szCommand, sizeof(szCommand)}, "%s", PszCompatEnvironment("STARS_CLIENT_STARTUP_SOURCE"));
    pszNewGame = PszCompatEnvironment("STARS_CLIENT_NEW_GAME");
    pszContinue = PszCompatEnvironment("STARS_CLIENT_CONTINUE");
    pszImportSource = PszCompatEnvironment("STARS_CLIENT_IMPORT_SOURCE");
    if (pszContinue && !win32::WritePrivateProfileStringA("Files", "File1", pszContinue, "StarsUtf8.ini")) return 1;
    if (pszContinue && (!win32::WritePrivateProfileStringA("Windows", "PlanetTiles", "ABE*FGH", "StarsUtf8.ini") ||
        !win32::WritePrivateProfileStringA("Windows", "ShipTiles", "AFDE*BJI", "StarsUtf8.ini"))) return 1;
    setvbuf(stdout, NULL, _IONBF, 0);
    nResult = ui::WinMain(win32::GetModuleHandleA(NULL), NULL, szCommand, win32::SW_SHOWNORMAL);
    if (win32::IsWindow(app.lpui->hwndTitle) && !pszImportSource && !PszCompatEnvironment("STARS_CLIENT_RACE_LIBRARY_TEST") &&
        !PszCompatEnvironment("STARS_CLIENT_RECENT_STARTUP") && !PszCompatEnvironment("STARS_CLIENT_RACE_NAME_CANCEL") &&
        !PszCompatEnvironment("STARS_CLIENT_RACE_VERSION_TEST") &&
        !PszCompatEnvironment("STARS_CLIENT_EXTERNAL_OPEN_GAME") &&
        !PszCompatEnvironment("STARS_CLIENT_RACE_ROSTER_TEST") && !PszCompatEnvironment("STARS_CLIENT_STARTUP_SOURCE") &&
        !PszCompatEnvironment("STARS_CLIENT_EXTERNAL_OPEN_RACE") && !PszCompatEnvironment("STARS_CLIENT_EXPORT_GAME") &&
        !PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_SOURCE") && !PszCompatEnvironment("STARS_CLIENT_GAME_LIBRARY_TEST") &&
        !PszCompatEnvironment("STARS_CLIENT_GAME_DELETE_MODE") &&
        !PszCompatEnvironment("STARS_CLIENT_HOST_LIBRARY_TEST") && !PszCompatEnvironment("STARS_CLIENT_MAC_SHORTCUTS_TEST") &&
        !PszCompatEnvironment("STARS_CLIENT_EXPECT_SELECTOR") && !PszCompatEnvironment("STARS_CLIENT_EXPECT_CONTINUE_DISABLED") &&
        !PszCompatEnvironment("STARS_CLIENT_SWITCH_CHOICE")) fPassed = 0;
    if (idTimer) win32::KillTimer(NULL, idTimer);
    StarsCompatProcessExit();
    SDL_Quit();
    return nResult == 0 && fChecked && fPassed && fTitlePainted &&
           cAlerts == (PszCompatEnvironment("STARS_CLIENT_GAME_DELETE_MODE") &&
                       !strcmp(PszCompatEnvironment("STARS_CLIENT_GAME_DELETE_MODE"), "failure") ? 1 :
                       PszCompatEnvironment("STARS_CLIENT_HOST_LIBRARY_TEST") ? 5 :
                       PszCompatEnvironment("STARS_CLIENT_LIBRARY_FAIL_WRITE") || PszCompatEnvironment("STARS_CLIENT_SWITCH_FAIL_SAVE") ? 2 :
                       PszCompatEnvironment("STARS_CLIENT_EXPORT_EXPECTED") ? 1 :
                       PszCompatEnvironment("STARS_CLIENT_IMPORT_INVALID_EXPECTED") ? 1 :
                       PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_EXPECTED") &&
                       (!strcmp(PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_EXPECTED"), "fail") ||
                        !strcmp(PszCompatEnvironment("STARS_CLIENT_IMPORT_UPDATE_EXPECTED"), "fail-conflict")) ? 1 : 0) && cEarlyFrameShow == 0 ? 0 : 1;
}
