/* Semantic inspection belongs to the scanner UI, not the generic HWND runtime. */
#include "win.h"
#include "text_io.h"
#include "control_io.h"
#include "scaninspect.h"
#include <stdlib.h>
#include <stars_compat.h>

static HWND hwndScanInspector;
static SCANSNAPSHOT snapScanInspector;
static BOOL fScanInspectorClass;

static BOOL FScannerReady(void) {
    if (StarsCompatWindowId(hwndScanner) == 0) return 0;
    return win32::IsWindow(hwndScanner) && win32::IsWindowVisible(hwndScanner) && game.lid != 0 &&
           idPlayer >= 0 && idPlayer < game.cPlayer && rglpfl &&
           game.cPlanMax > 0 && cFleet >= 0 && cThing >= 0;
}

void FreeScannerSnapshot(SCANSNAPSHOT *lpsnap) {
    if (!lpsnap) return;
    *lpsnap = {};
}

static void ScanObjectPosition(SCANSNAPSHOT *lpsnap, SCANOBJECT *lpobj, POINT16 pt) {
    lpobj->ptUniverse = PointFrom16(pt);
    LogicalToScan(&pt);
    lpobj->ptClient = PointFrom16(pt);
    lpobj->ptScreen = lpobj->ptClient;
    win32::ClientToScreen(hwndScanner, &lpobj->ptScreen);
    lpobj->fOnScreen = win32::PtInRect(&lpsnap->rcViewport, lpobj->ptClient);
}

BOOL FCaptureScanner(SCANSNAPSHOT *lpsnap) {
    SCANOBJECT *lpobj;
    PLANET *lppl;
    FLEET *lpfl;
    THING *lpth;
    size_t c, i;
    unsigned grRelation;
    char szWorkSav[sizeof(szWork)];
    if (!lpsnap || lpsnap->rgobj || lpsnap->cObject != 0 || !FScannerReady()) return 0;
    c = (size_t)game.cPlanMax + (size_t)cFleet + (size_t)cThing;
    if (c > SIZE_MAX / sizeof(*lpobj)) return 0;
    lpsnap->rgobj.reset(static_cast<SCANOBJECT *>(calloc(c, sizeof(*lpobj))));
    if (!lpsnap->rgobj) return 0;
    lpsnap->lidGame = game.lid; lpsnap->turn = game.turn;
    lpsnap->idPlayer = idPlayer; lpsnap->iZoom = iScanZoom;
    ControlIO().pfnGetClientRect(hwndScanner, &lpsnap->rcViewport);
    lpsnap->rcViewport.bottom = StarsMax(0, lpsnap->rcViewport.bottom - dySBar);
    /* Name helpers use the game's scratch buffer; a read-only capture preserves it. */
    memcpy(szWorkSav, szWork, sizeof(szWork));
    for (i = 0; i < (size_t)game.cPlanMax; i++) {
        lpobj = &lpsnap->rgobj[lpsnap->cObject++];
        lpobj->grType = grobjPlanet; lpobj->id = (uint16_t)i; lpobj->ith = ithNone;
        lppl = LpplFromId((int16_t)i);
        lpobj->iPlayer = lppl ? lppl->iPlayer : iplrNone;
        lpobj->det = lppl ? lppl->det : 0;
        CchSprintf(lpobj->szName, "%s", TextIO().pfnPlanetName((int16_t)i));
        ScanObjectPosition(lpsnap, lpobj, rgptPlan[i]);
        lpobj->fScanSelected = sel.scan.grobj == grobjPlanet && sel.scan.idpl == (int16_t)i;
        lpobj->fMainSelected = sel.grobj == grobjPlanet && sel.id == (int16_t)i;
    }
    for (i = 0; i < (size_t)cFleet && rglpfl[i]; i++) {
        lpfl = rglpfl[i];
        if (lpfl->fDead) continue;
        lpobj = &lpsnap->rgobj[lpsnap->cObject++];
        lpobj->grType = grobjFleet; lpobj->id = (uint16_t)lpfl->id; lpobj->ith = ithNone;
        lpobj->iPlayer = lpfl->iPlayer; lpobj->det = lpfl->det;
        CchSprintf(lpobj->szName, "%s", PszGetFleetName(lpfl->id));
        ScanObjectPosition(lpsnap, lpobj, lpfl->pt);
        lpobj->fScanSelected = sel.scan.grobj == grobjFleet && sel.scan.ifl == (int16_t)i;
        lpobj->fMainSelected = sel.grobj == grobjFleet && sel.id == lpfl->id;
        lpobj->fFiltered = (grbitScan & grbitScanViewMask) == scanViewNoPlayerInfo ||
            (CShipsScanVis(lpfl) == 0 && !lpobj->fScanSelected && !lpobj->fMainSelected);
    }
    for (i = 0; lpThings && i < (size_t)cThing; i++) {
        lpth = &lpThings[i];
        lpobj = &lpsnap->rgobj[lpsnap->cObject++];
        lpobj->grType = grobjThing; lpobj->id = lpth->idFull(); lpobj->ith = lpth->ith();
        lpobj->iPlayer = lpth->iplr(); lpobj->det = 0;
        CchSprintf(lpobj->szName, "%s", PszGetThingName(lpth->idFull()));
        ScanObjectPosition(lpsnap, lpobj, lpth->pt());
        lpobj->fScanSelected = sel.scan.grobj == grobjThing && sel.scan.ith == (int16_t)i;
        lpobj->fFiltered = (grbitScan & grbitScanViewMask) == scanViewNoPlayerInfo;
        if (lpth->ith() == ithMinefield) {
            grRelation = lpth->iplr() == idPlayer ? 1 :
                rgplr[idPlayer].rgmdRelation[lpth->iplr()] == 1 ? 2 :
                rgplr[idPlayer].rgmdRelation[lpth->iplr()] == 0 ? 4 : 8;
            lpobj->fFiltered = !(grbitScan & grbitScanMineFields) || (!lpobj->fScanSelected && !(grbitScanMines & grRelation));
        }
    }
    memcpy(szWork, szWorkSav, sizeof(szWork));
    return 1;
}

BOOL FSelectScannerObject(const SCANSNAPSHOT *lpsnap, uint16_t grType, uint16_t id, BOOL fReveal) {
    SCAN scan;
    int16_t i;
    POINT16 pt;
    if (!lpsnap || !FScannerReady() || lpsnap->lidGame != game.lid || lpsnap->turn != game.turn || lpsnap->idPlayer != idPlayer) return 0;
    memset(&scan, 0, sizeof(scan)); scan.idpl = idplNone; scan.ifl = iflNone; scan.ith = ithNone; scan.iwp = iwpNone;
    if (grType == grobjPlanet) {
        if (id >= (uint16_t)game.cPlanMax) return 0;
        scan.idpl = (int16_t)id; pt = rgptPlan[id];
    } else if (grType == grobjFleet) {
        for (i = 0; i < cFleet && rglpfl[i]; i++) if ((uint16_t)rglpfl[i]->id == id && !rglpfl[i]->fDead) break;
        if (i == cFleet || !rglpfl[i]) return 0;
        scan.ifl = i; pt = rglpfl[i]->pt;
    } else if (grType == grobjThing) {
        for (i = 0; lpThings && i < cThing; i++) if (lpThings[i].idFull() == id) break;
        if (!lpThings || i == cThing) return 0;
        scan.ith = i; pt = lpThings[i].pt();
    } else return 0;
    /* Find co-located categories, then override the object identity: multiple
       fleets at one planet must not all resolve to the nearest fleet. */
    FFindNearestObject(pt, static_cast<GrobjClass>(grobjPlanet | grobjFleet | grobjThing | mdExact), &scan);
    scan.grobj = static_cast<GrobjClass>(grType); scan.grobjFull = static_cast<GrobjClass>(scan.grobjFull | grType); scan.pt = pt; scan.iwp = iwpNone;
    if (grType == grobjPlanet) scan.idpl = (int16_t)id;
    else if (grType == grobjFleet) scan.ifl = i;
    else scan.ith = i;
    ChangeScanSel(&scan, 2);
    if (fReveal) FEnsurePointOnScreen(pt, 1);
    ControlIO().pfnInvalidateRect(hwndScanner, NULL, 0);
    return 1;
}

static BOOL FWriteScanString(NATIVEFILE *lpfile, const char *psz) {
    unsigned ch;

    if (!FUtf8Valid(psz, strlen(psz)) || ChPutNativeFile('"', lpfile) == EOF) return 0;
    while (*psz) {
        ch = (unsigned char)*psz++;
        if (ch == '"' || ch == '\\') { if (ChPutNativeFile('\\', lpfile) == EOF || ChPutNativeFile((int)ch, lpfile) == EOF) return 0; }
        else if (ch < 32) { if (CchNativePrintf(lpfile, "\\u%04x", ch) < 0) return 0; }
        else if (ChPutNativeFile((int)ch, lpfile) == EOF) return 0;
    }
    return ChPutNativeFile('"', lpfile) != EOF;
}

BOOL FWriteScannerSnapshot(const SCANSNAPSHOT *lpsnap, NATIVEFILE *lpfile) {
    size_t i;
    const SCANOBJECT *lp;
    if (!lpsnap || !lpfile || (lpsnap->cObject != 0 && !lpsnap->rgobj)) return 0;
    if (CchNativePrintf(lpfile, "{\"schema\":1,\"game\":%ld,\"turn\":%u,\"player\":%d,\"zoom\":%d,\"viewport\":[%ld,%ld,%ld,%ld],\"objects\":[\n",
        (long)lpsnap->lidGame, lpsnap->turn, lpsnap->idPlayer, lpsnap->iZoom,
        (long)lpsnap->rcViewport.left,(long)lpsnap->rcViewport.top,(long)lpsnap->rcViewport.right,(long)lpsnap->rcViewport.bottom) < 0) return 0;
    for (i = 0; i < lpsnap->cObject; i++) {
        lp = &lpsnap->rgobj[i];
        if (!memchr(lp->szName, 0, sizeof(lp->szName))) return 0;
        if (CchNativePrintf(lpfile, "%s{\"kind\":%u,\"id\":%u,\"owner\":%d,\"detail\":%d,\"thingType\":%d,\"universe\":[%ld,%ld],\"client\":[%ld,%ld],\"screen\":[%ld,%ld],"
            "\"onScreen\":%s,\"filtered\":%s,\"scanSelected\":%s,\"mainSelected\":%s,\"name\":", i ? ",\n" : "", lp->grType,lp->id,lp->iPlayer,lp->det,lp->ith,
            (long)lp->ptUniverse.x,(long)lp->ptUniverse.y,(long)lp->ptClient.x,(long)lp->ptClient.y,(long)lp->ptScreen.x,(long)lp->ptScreen.y,
            lp->fOnScreen ? "true":"false",lp->fFiltered ? "true":"false",lp->fScanSelected ? "true":"false",lp->fMainSelected ? "true":"false") < 0 ||
            !FWriteScanString(lpfile,lp->szName) || ChPutNativeFile('}',lpfile) == EOF) return 0;
    }
    return IPutNativeString("\n]}\n",lpfile) != EOF && !FNativeStreamError(lpfile);
}

static void ScannerInspectDetails(HWND hwnd) {
    int i = (int)ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, 1), win32::LB_GETCURSEL, 0, 0);
    SCANOBJECT *lp;
    char sz[768];
    if (i < 0 || (size_t)i >= snapScanInspector.cObject) { ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd,2), "No active player scanner. Load a game and refresh."); return; }
    lp = &snapScanInspector.rgobj[i];
    CchSprintf(sz,"%s\r\nKind %u, ID %u, owner %d, detail %d\r\nUniverse (%ld, %ld)\r\nScanner (%ld, %ld), screen (%ld, %ld)\r\nOn screen: %s; filtered: %s\r\nScanner selection: %s; main selection: %s\r\nGame %ld, turn %u, player %d",
        lp->szName,lp->grType,lp->id,lp->iPlayer,lp->det,(long)lp->ptUniverse.x,(long)lp->ptUniverse.y,
        (long)lp->ptClient.x,(long)lp->ptClient.y,(long)lp->ptScreen.x,(long)lp->ptScreen.y,
        lp->fOnScreen?"yes":"no",lp->fFiltered?"yes":"no",lp->fScanSelected?"yes":"no",lp->fMainSelected?"yes":"no",
        (long)snapScanInspector.lidGame,snapScanInspector.turn,snapScanInspector.idPlayer);
    ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd,2),sz);
}

static void RefreshScannerInspector(HWND hwnd) {
    size_t i;
    char sz[424];
    const SCANOBJECT *lp;

    FreeScannerSnapshot(&snapScanInspector);
    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, 1), win32::LB_RESETCONTENT, 0, 0);
    if (FCaptureScanner(&snapScanInspector)) {
        for (i = 0; i < snapScanInspector.cObject; i++) {
            lp = &snapScanInspector.rgobj[i];
            CchSprintf(sz, "%s %u: %s%s", lp->grType == grobjPlanet ? "Planet" : lp->grType == grobjFleet ? "Fleet" : "Thing",
                       lp->id, lp->szName, lp->fOnScreen ? "" : " [offscreen]");
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, 1), win32::LB_ADDSTRING, 0, (LPARAM)sz);
        }
    }
    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, 1), win32::LB_SETCURSEL, 0, 0);
    ScannerInspectDetails(hwnd);
}

static LRESULT  ScannerInspectorProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    RECT rc;
    int i;
    SCANOBJECT *lp;

    switch (msg) {
    case win32::WM_CREATE:
        if (!ui::CreateWindowEx(0, "ListBox", "", win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP | win32::WS_BORDER | win32::WS_VSCROLL | win32::LBS_NOTIFY | win32::LBS_NOINTEGRALHEIGHT,
                          8, 40, 300, 300, hwnd, (HMENU)(INT_PTR)1, hInst, NULL) ||
            !ui::CreateWindowEx(0, "Edit", "", win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP | win32::WS_BORDER | win32::ES_MULTILINE | win32::ES_READONLY | win32::ES_AUTOVSCROLL,
                          320, 40, 320, 300, hwnd, (HMENU)(INT_PTR)2, hInst, NULL) ||
            !ui::CreateWindowEx(0, "Button", "Refresh", win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                          8, 8, 90, 24, hwnd, (HMENU)(INT_PTR)3, hInst, NULL) ||
            !ui::CreateWindowEx(0, "Button", "Select / reveal", win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                          108, 8, 130, 24, hwnd, (HMENU)(INT_PTR)4, hInst, NULL)) return -1;
        return 0;
    case win32::WM_SIZE:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        ui::MoveWindow(ControlIO().pfnGetDlgItem(hwnd, 1), 8, 40, StarsMax(0, rc.right / 2 - 12), StarsMax(0, rc.bottom - 48), 1);
        ui::MoveWindow(ControlIO().pfnGetDlgItem(hwnd, 2), rc.right / 2, 40, StarsMax(0, rc.right / 2 - 8), StarsMax(0, rc.bottom - 48), 1);
        return 0;
    case win32::WM_COMMAND:
        if (LOWORD(w) == 3 && HIWORD(w) == win32::BN_CLICKED) RefreshScannerInspector(hwnd);
        else if (LOWORD(w) == 1 && HIWORD(w) == win32::LBN_SELCHANGE) ScannerInspectDetails(hwnd);
        else if (LOWORD(w) == 4 && HIWORD(w) == win32::BN_CLICKED) {
            i = (int)ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, 1), win32::LB_GETCURSEL, 0, 0);
            if (i >= 0 && (size_t)i < snapScanInspector.cObject) {
                lp = &snapScanInspector.rgobj[i];
                if (!FSelectScannerObject(&snapScanInspector, lp->grType, lp->id, 1)) ControlIO().pfnMessageBeep(win32::MB_ICONEXCLAMATION);
                RefreshScannerInspector(hwnd);
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, 1), win32::LB_SETCURSEL, i, 0);
                ScannerInspectDetails(hwnd);
            }
        }
        return 0;
    case win32::WM_DESTROY:
        hwndScanInspector = NULL;
        FreeScannerSnapshot(&snapScanInspector);
        return 0;
    default:
        return ui::DefWindowProc(hwnd, msg, w, l);
    }
}

HWND ShowScannerInspector(void) {
    WNDCLASS wc = {0};

    if (!win32::IsWindow(hwndFrame)) return NULL;
    if (StarsCompatWindowId(hwndFrame) == 0) return NULL;
    wc.lpszClassName = "StarsScannerInspector";
    if (!win32::IsWindow(hwndScanInspector)) {
        if (!fScanInspectorClass) {
            wc.lpfnWndProc = ScannerInspectorProc;
            wc.hInstance = hInst;
            wc.hbrBackground = (HBRUSH)(INT_PTR)(win32::COLOR_BTNFACE + 1);
            if (!win32::RegisterClassA(&wc)) return NULL;
            fScanInspectorClass = 1;
        }
        hwndScanInspector = ui::CreateWindowEx(0, wc.lpszClassName, "Stars scanner objects", win32::WS_OVERLAPPEDWINDOW,
                                         win32::CW_USEDEFAULT, win32::CW_USEDEFAULT, 800, 450, hwndFrame, NULL, hInst, NULL);
        if (!hwndScanInspector) return NULL;
    }
    RefreshScannerInspector(hwndScanInspector);
    ui::ShowWindow(hwndScanInspector, win32::SW_SHOWNORMAL);
    return hwndScanInspector;
}

void CloseScannerInspector(void) {
    if (win32::IsWindow(hwndScanInspector)) ControlIO().pfnDestroyWindow(hwndScanInspector);
    hwndScanInspector = NULL;
    FreeScannerSnapshot(&snapScanInspector);
}

void ResetScannerInspectorSession() noexcept {
    hwndScanInspector = NULL;
    FreeScannerSnapshot(&snapScanInspector);
    fScanInspectorClass = 0;
}
