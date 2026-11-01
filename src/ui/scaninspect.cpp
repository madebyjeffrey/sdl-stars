/* Semantic inspection belongs to the scanner UI, not the generic HWND runtime. */
#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "text_io.h"
#include "control_io.h"
#include "scaninspect.h"
#include <stdlib.h>
#include <stars_compat.h>

static HWND hwndScanInspector;
static SCANSNAPSHOT snapScanInspector;
static BOOL fScanInspectorClass;

static BOOL FScannerReady(APPSESSION &app) {
    if (StarsCompatWindowId(app.lpui->hwndScanner) == 0) return 0;
    return win32::IsWindow(app.lpui->hwndScanner) && win32::IsWindowVisible(app.lpui->hwndScanner) && app.state.game.lid != 0 &&
           app.state.idPlayer >= 0 && app.state.idPlayer < app.state.game.cPlayer && app.state.rglpfl &&
           app.state.game.cPlanMax > 0 && app.state.cFleet >= 0 && app.state.cThing >= 0;
}

void FreeScannerSnapshot(SCANSNAPSHOT *lpsnap) {
    if (!lpsnap) return;
    *lpsnap = {};
}

static void ScanObjectPosition(APPSESSION &app, SCANSNAPSHOT *lpsnap, SCANOBJECT *lpobj, POINT16 pt) {
    lpobj->ptUniverse = PointFrom16(pt);
    LogicalToScan(app, &pt);
    lpobj->ptClient = PointFrom16(pt);
    lpobj->ptScreen = lpobj->ptClient;
    win32::ClientToScreen(app.lpui->hwndScanner, &lpobj->ptScreen);
    lpobj->fOnScreen = win32::PtInRect(&lpsnap->rcViewport, lpobj->ptClient);
}

BOOL FCaptureScanner(APPSESSION &app, SCANSNAPSHOT *lpsnap) {
    SCANOBJECT *lpobj;
    PLANET *lppl;
    FLEET *lpfl;
    THING *lpth;
    size_t c, i;
    unsigned grRelation;
    char szWorkSav[sizeof(app.state.szWork)];
    if (!lpsnap || lpsnap->rgobj || lpsnap->cObject != 0 || !FScannerReady(app)) return 0;
    c = static_cast<size_t>(app.state.game.cPlanMax) + static_cast<size_t>(app.state.cFleet) + static_cast<size_t>(app.state.cThing);
    if (c > SIZE_MAX / sizeof(*lpobj)) return 0;
    lpsnap->rgobj.reset(static_cast<SCANOBJECT *>(calloc(c, sizeof(*lpobj))));
    if (!lpsnap->rgobj) return 0;
    lpsnap->lidGame = app.state.game.lid; lpsnap->turn = app.state.game.turn;
    lpsnap->idPlayer = app.state.idPlayer; lpsnap->iZoom = app.state.iScanZoom;
    ControlIO().pfnGetClientRect(app.lpui->hwndScanner, &lpsnap->rcViewport);
    lpsnap->rcViewport.bottom = StarsMax(0, lpsnap->rcViewport.bottom - app.lpui->dySBar);
    /* Name helpers use the game's scratch buffer; a read-only capture preserves it. */
    memcpy(szWorkSav, app.state.szWork, sizeof(app.state.szWork));
    for (i = 0; i < static_cast<size_t>(app.state.game.cPlanMax); i++) {
        lpobj = &lpsnap->rgobj[lpsnap->cObject++];
        lpobj->grType = grobjPlanet; lpobj->id = static_cast<uint16_t>(i); lpobj->ith = ithNone;
        lppl = LpplFromId(app, static_cast<int16_t>(i));
        lpobj->iPlayer = lppl ? lppl->iPlayer : iplrNone;
        lpobj->det = lppl ? lppl->det : 0;
        CchSprintf(lpobj->szName, "%s", TextIO().pfnPlanetName(app, static_cast<int16_t>(i)));
        ScanObjectPosition(app, lpsnap, lpobj, app.state.rgptPlan[i]);
        lpobj->fScanSelected = app.state.sel.scan.grobj == grobjPlanet && app.state.sel.scan.idpl == static_cast<int16_t>(i);
        lpobj->fMainSelected = app.state.sel.grobj == grobjPlanet && app.state.sel.id == static_cast<int16_t>(i);
    }
    for (i = 0; i < static_cast<size_t>(app.state.cFleet) && app.state.rglpfl[i]; i++) {
        lpfl = app.state.rglpfl[i];
        if (lpfl->fDead) continue;
        lpobj = &lpsnap->rgobj[lpsnap->cObject++];
        lpobj->grType = grobjFleet; lpobj->id = static_cast<uint16_t>(lpfl->id); lpobj->ith = ithNone;
        lpobj->iPlayer = lpfl->iPlayer; lpobj->det = lpfl->det;
        CchSprintf(lpobj->szName, "%s", PszGetFleetName(app, lpfl->id));
        ScanObjectPosition(app, lpsnap, lpobj, lpfl->pt);
        lpobj->fScanSelected = app.state.sel.scan.grobj == grobjFleet && app.state.sel.scan.ifl == static_cast<int16_t>(i);
        lpobj->fMainSelected = app.state.sel.grobj == grobjFleet && app.state.sel.id == lpfl->id;
        lpobj->fFiltered = (app.lpui->grbitScan & grbitScanViewMask) == scanViewNoPlayerInfo ||
            (CShipsScanVis(app, lpfl) == 0 && !lpobj->fScanSelected && !lpobj->fMainSelected);
    }
    for (i = 0; app.state.lpThings && i < static_cast<size_t>(app.state.cThing); i++) {
        lpth = &app.state.lpThings[i];
        lpobj = &lpsnap->rgobj[lpsnap->cObject++];
        lpobj->grType = grobjThing; lpobj->id = lpth->idFull(); lpobj->ith = lpth->ith();
        lpobj->iPlayer = lpth->iplr(); lpobj->det = 0;
        CchSprintf(lpobj->szName, "%s", PszGetThingName(app, lpth->idFull()));
        ScanObjectPosition(app, lpsnap, lpobj, lpth->pt());
        lpobj->fScanSelected = app.state.sel.scan.grobj == grobjThing && app.state.sel.scan.ith == static_cast<int16_t>(i);
        lpobj->fFiltered = (app.lpui->grbitScan & grbitScanViewMask) == scanViewNoPlayerInfo;
        if (lpth->ith() == ithMinefield) {
            grRelation = lpth->iplr() == app.state.idPlayer ? 1 :
                app.state.rgplr[app.state.idPlayer].rgmdRelation[lpth->iplr()] == 1 ? 2 :
                app.state.rgplr[app.state.idPlayer].rgmdRelation[lpth->iplr()] == 0 ? 4 : 8;
            lpobj->fFiltered = !(app.lpui->grbitScan & grbitScanMineFields) || (!lpobj->fScanSelected && !(app.lpui->grbitScanMines & grRelation));
        }
    }
    memcpy(app.state.szWork, szWorkSav, sizeof(app.state.szWork));
    return 1;
}

BOOL FSelectScannerObject(APPSESSION &app, const SCANSNAPSHOT *lpsnap, uint16_t grType, uint16_t id, BOOL fReveal) {
    SCAN scan;
    int16_t i;
    POINT16 pt;
    if (!lpsnap || !FScannerReady(app) || lpsnap->lidGame != app.state.game.lid || lpsnap->turn != app.state.game.turn || lpsnap->idPlayer != app.state.idPlayer) return 0;
    memset(&scan, 0, sizeof(scan)); scan.idpl = idplNone; scan.ifl = iflNone; scan.ith = ithNone; scan.iwp = iwpNone;
    if (grType == grobjPlanet) {
        if (id >= static_cast<uint16_t>(app.state.game.cPlanMax)) return 0;
        scan.idpl = static_cast<int16_t>(id); pt = app.state.rgptPlan[id];
    } else if (grType == grobjFleet) {
        for (i = 0; i < app.state.cFleet && app.state.rglpfl[i]; i++) if (static_cast<uint16_t>(app.state.rglpfl[i]->id) == id && !app.state.rglpfl[i]->fDead) break;
        if (i == app.state.cFleet || !app.state.rglpfl[i]) return 0;
        scan.ifl = i; pt = app.state.rglpfl[i]->pt;
    } else if (grType == grobjThing) {
        for (i = 0; app.state.lpThings && i < app.state.cThing; i++) if (app.state.lpThings[i].idFull() == id) break;
        if (!app.state.lpThings || i == app.state.cThing) return 0;
        scan.ith = i; pt = app.state.lpThings[i].pt();
    } else return 0;
    /* Find co-located categories, then override the object identity: multiple
       fleets at one planet must not all resolve to the nearest fleet. */
    FFindNearestObject(app, pt, static_cast<GrobjClass>(grobjPlanet | grobjFleet | grobjThing | mdExact), &scan);
    scan.grobj = static_cast<GrobjClass>(grType); scan.grobjFull = static_cast<GrobjClass>(scan.grobjFull | grType); scan.pt = pt; scan.iwp = iwpNone;
    if (grType == grobjPlanet) scan.idpl = static_cast<int16_t>(id);
    else if (grType == grobjFleet) scan.ifl = i;
    else scan.ith = i;
    ChangeScanSel(app, &scan, 2);
    if (fReveal) FEnsurePointOnScreen(app, pt, 1);
    ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, nullptr, 0);
    return 1;
}

static BOOL FWriteScanString(NATIVEFILE *lpfile, const char *psz) {
    unsigned ch;

    if (!FUtf8Valid(psz, strlen(psz)) || ChPutNativeFile('"', lpfile) == EOF) return 0;
    while (*psz) {
        ch = static_cast<unsigned char>(*psz++);
        if (ch == '"' || ch == '\\') { if (ChPutNativeFile('\\', lpfile) == EOF || ChPutNativeFile(static_cast<int>(ch), lpfile) == EOF) return 0; }
        else if (ch < 32) { if (CchNativePrintf(lpfile, "\\u%04x", ch) < 0) return 0; }
        else if (ChPutNativeFile(static_cast<int>(ch), lpfile) == EOF) return 0;
    }
    return ChPutNativeFile('"', lpfile) != EOF;
}

BOOL FWriteScannerSnapshot(const SCANSNAPSHOT *lpsnap, NATIVEFILE *lpfile) {
    size_t i;
    const SCANOBJECT *lp;
    if (!lpsnap || !lpfile || (lpsnap->cObject != 0 && !lpsnap->rgobj)) return 0;
    if (CchNativePrintf(lpfile, "{\"schema\":1,\"game\":%ld,\"turn\":%u,\"player\":%d,\"zoom\":%d,\"viewport\":[%ld,%ld,%ld,%ld],\"objects\":[\n",
        static_cast<long>(lpsnap->lidGame), lpsnap->turn, lpsnap->idPlayer, lpsnap->iZoom,
        static_cast<long>(lpsnap->rcViewport.left),static_cast<long>(lpsnap->rcViewport.top),static_cast<long>(lpsnap->rcViewport.right),static_cast<long>(lpsnap->rcViewport.bottom)) < 0) return 0;
    for (i = 0; i < lpsnap->cObject; i++) {
        lp = &lpsnap->rgobj[i];
        if (!memchr(lp->szName, 0, sizeof(lp->szName))) return 0;
        if (CchNativePrintf(lpfile, "%s{\"kind\":%u,\"id\":%u,\"owner\":%d,\"detail\":%d,\"thingType\":%d,\"universe\":[%ld,%ld],\"client\":[%ld,%ld],\"screen\":[%ld,%ld],"
            "\"onScreen\":%s,\"filtered\":%s,\"scanSelected\":%s,\"mainSelected\":%s,\"name\":", i ? ",\n" : "", lp->grType,lp->id,lp->iPlayer,lp->det,lp->ith,
            static_cast<long>(lp->ptUniverse.x),static_cast<long>(lp->ptUniverse.y),static_cast<long>(lp->ptClient.x),static_cast<long>(lp->ptClient.y),static_cast<long>(lp->ptScreen.x),static_cast<long>(lp->ptScreen.y),
            lp->fOnScreen ? "true":"false",lp->fFiltered ? "true":"false",lp->fScanSelected ? "true":"false",lp->fMainSelected ? "true":"false") < 0 ||
            !FWriteScanString(lpfile,lp->szName) || ChPutNativeFile('}',lpfile) == EOF) return 0;
    }
    return IPutNativeString("\n]}\n",lpfile) != EOF && !FNativeStreamError(lpfile);
}

static void ScannerInspectDetails(HWND hwnd) {
    int i = static_cast<int>(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, 1), win32::LB_GETCURSEL, 0, 0));
    SCANOBJECT *lp;
    char sz[768];
    if (i < 0 || static_cast<size_t>(i) >= snapScanInspector.cObject) { ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd,2), "No active player scanner. Load a game and refresh."); return; }
    lp = &snapScanInspector.rgobj[i];
    CchSprintf(sz,"%s\r\nKind %u, ID %u, owner %d, detail %d\r\nUniverse (%ld, %ld)\r\nScanner (%ld, %ld), screen (%ld, %ld)\r\nOn screen: %s; filtered: %s\r\nScanner selection: %s; main selection: %s\r\nGame %ld, turn %u, player %d",
        lp->szName,lp->grType,lp->id,lp->iPlayer,lp->det,static_cast<long>(lp->ptUniverse.x),static_cast<long>(lp->ptUniverse.y),
        static_cast<long>(lp->ptClient.x),static_cast<long>(lp->ptClient.y),static_cast<long>(lp->ptScreen.x),static_cast<long>(lp->ptScreen.y),
        lp->fOnScreen?"yes":"no",lp->fFiltered?"yes":"no",lp->fScanSelected?"yes":"no",lp->fMainSelected?"yes":"no",
        static_cast<long>(snapScanInspector.lidGame),snapScanInspector.turn,snapScanInspector.idPlayer);
    ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd,2),sz);
}

static void RefreshScannerInspector(APPSESSION &app, HWND hwnd) {
    size_t i;
    char sz[424];
    const SCANOBJECT *lp;

    FreeScannerSnapshot(&snapScanInspector);
    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, 1), win32::LB_RESETCONTENT, 0, 0);
    if (FCaptureScanner(app, &snapScanInspector)) {
        for (i = 0; i < snapScanInspector.cObject; i++) {
            lp = &snapScanInspector.rgobj[i];
            CchSprintf(sz, "%s %u: %s%s", lp->grType == grobjPlanet ? "Planet" : lp->grType == grobjFleet ? "Fleet" : "Thing",
                       lp->id, lp->szName, lp->fOnScreen ? "" : " [offscreen]");
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, 1), win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(sz));
        }
    }
    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, 1), win32::LB_SETCURSEL, 0, 0);
    ScannerInspectDetails(hwnd);
}

static LRESULT  ScannerInspectorProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    RECT rc;
    int i;
    SCANOBJECT *lp;

    switch (msg) {
    case win32::WM_CREATE:
        if (!ui::CreateWindowEx(0, "ListBox", "", win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP | win32::WS_BORDER | win32::WS_VSCROLL | win32::LBS_NOTIFY | win32::LBS_NOINTEGRALHEIGHT,
                          8, 40, 300, 300, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(1)), app.lpui->hInst, nullptr) ||
            !ui::CreateWindowEx(0, "Edit", "", win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP | win32::WS_BORDER | win32::ES_MULTILINE | win32::ES_READONLY | win32::ES_AUTOVSCROLL,
                          320, 40, 320, 300, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(2)), app.lpui->hInst, nullptr) ||
            !ui::CreateWindowEx(0, "Button", "Refresh", win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                          8, 8, 90, 24, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(3)), app.lpui->hInst, nullptr) ||
            !ui::CreateWindowEx(0, "Button", "Select / reveal", win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                          108, 8, 130, 24, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(4)), app.lpui->hInst, nullptr)) return -1;
        return 0;
    case win32::WM_SIZE:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        ui::MoveWindow(ControlIO().pfnGetDlgItem(hwnd, 1), 8, 40, StarsMax(0, rc.right / 2 - 12), StarsMax(0, rc.bottom - 48), 1);
        ui::MoveWindow(ControlIO().pfnGetDlgItem(hwnd, 2), rc.right / 2, 40, StarsMax(0, rc.right / 2 - 8), StarsMax(0, rc.bottom - 48), 1);
        return 0;
    case win32::WM_COMMAND:
        if (LOWORD(w) == 3 && HIWORD(w) == win32::BN_CLICKED) RefreshScannerInspector(app, hwnd);
        else if (LOWORD(w) == 1 && HIWORD(w) == win32::LBN_SELCHANGE) ScannerInspectDetails(hwnd);
        else if (LOWORD(w) == 4 && HIWORD(w) == win32::BN_CLICKED) {
            i = static_cast<int>(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, 1), win32::LB_GETCURSEL, 0, 0));
            if (i >= 0 && static_cast<size_t>(i) < snapScanInspector.cObject) {
                lp = &snapScanInspector.rgobj[i];
                if (!FSelectScannerObject(app, &snapScanInspector, lp->grType, lp->id, 1)) ControlIO().pfnMessageBeep(win32::MB_ICONEXCLAMATION);
                RefreshScannerInspector(app, hwnd);
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, 1), win32::LB_SETCURSEL, i, 0);
                ScannerInspectDetails(hwnd);
            }
        }
        return 0;
    case win32::WM_DESTROY:
        hwndScanInspector = nullptr;
        FreeScannerSnapshot(&snapScanInspector);
        return 0;
    default:
        return ui::DefWindowProc(hwnd, msg, w, l);
    }
}

HWND ShowScannerInspector(void) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    WNDCLASS wc = {0};

    if (!win32::IsWindow(app.lpui->hwndFrame)) return nullptr;
    if (StarsCompatWindowId(app.lpui->hwndFrame) == 0) return nullptr;
    wc.lpszClassName = "StarsScannerInspector";
    if (!win32::IsWindow(hwndScanInspector)) {
        if (!fScanInspectorClass) {
            wc.lpfnWndProc = ScannerInspectorProc;
            wc.hInstance = app.lpui->hInst;
            wc.hbrBackground = reinterpret_cast<HBRUSH>(static_cast<INT_PTR>((win32::COLOR_BTNFACE + 1)));
            if (!win32::RegisterClassA(&wc)) return nullptr;
            fScanInspectorClass = 1;
        }
        hwndScanInspector = ui::CreateWindowEx(0, wc.lpszClassName, "Stars scanner objects", win32::WS_OVERLAPPEDWINDOW,
                                         win32::CW_USEDEFAULT, win32::CW_USEDEFAULT, 800, 450, app.lpui->hwndFrame, nullptr, app.lpui->hInst, nullptr);
        if (!hwndScanInspector) return nullptr;
    }
    RefreshScannerInspector(app, hwndScanInspector);
    ui::ShowWindow(hwndScanInspector, win32::SW_SHOWNORMAL);
    return hwndScanInspector;
}

void CloseScannerInspector(void) {
    if (win32::IsWindow(hwndScanInspector)) ControlIO().pfnDestroyWindow(hwndScanInspector);
    hwndScanInspector = nullptr;
    FreeScannerSnapshot(&snapScanInspector);
}

void ResetScannerInspectorSession() noexcept {
    hwndScanInspector = nullptr;
    FreeScannerSnapshot(&snapScanInspector);
    fScanInspectorClass = 0;
}
