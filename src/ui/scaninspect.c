/* Semantic inspection belongs to the scanner UI, not the generic HWND runtime. */
#include "win.h"
#include "scaninspect.h"
#include <stdlib.h>
#include <stars_compat.h>

static HWND hwndScanInspector;
static SCANSNAPSHOT snapScanInspector;
static BOOL fScanInspectorClass;

static BOOL FScannerReady(void) {
    if (StarsCompatWindowId(hwndScanner) == 0) return FALSE;
    return IsWindow(hwndScanner) && IsWindowVisible(hwndScanner) && game.lid != 0 &&
           idPlayer >= 0 && idPlayer < game.cPlayer && rgptPlan && rgidPlan && rglpfl &&
           game.cPlanMax > 0 && cFleet >= 0 && cThing >= 0;
}

void FreeScannerSnapshot(SCANSNAPSHOT *lpsnap) {
    if (!lpsnap) return;
    free(lpsnap->rgobj); memset(lpsnap, 0, sizeof(*lpsnap));
}

static void ScanObjectPosition(SCANSNAPSHOT *lpsnap, SCANOBJECT *lpobj, POINT16 pt) {
    lpobj->ptUniverse = PointFrom16(pt);
    LogicalToScan(&pt);
    lpobj->ptClient = PointFrom16(pt);
    lpobj->ptScreen = lpobj->ptClient;
    ClientToScreen(hwndScanner, &lpobj->ptScreen);
    lpobj->fOnScreen = PtInRect(&lpsnap->rcViewport, lpobj->ptClient);
}

BOOL FCaptureScanner(SCANSNAPSHOT *lpsnap) {
    SCANOBJECT *lpobj;
    PLANET *lppl;
    FLEET *lpfl;
    THING *lpth;
    size_t c, i;
    unsigned grRelation;
    char szWorkSav[sizeof(szWork)];
    if (!lpsnap || lpsnap->rgobj || lpsnap->cObject != 0 || !FScannerReady()) return FALSE;
    c = (size_t)game.cPlanMax + (size_t)cFleet + (size_t)cThing;
    if (c > SIZE_MAX / sizeof(*lpobj)) return FALSE;
    lpsnap->rgobj = calloc(c, sizeof(*lpobj));
    if (!lpsnap->rgobj) return FALSE;
    lpsnap->lidGame = game.lid; lpsnap->turn = game.turn;
    lpsnap->idPlayer = idPlayer; lpsnap->iZoom = iScanZoom;
    GetClientRect(hwndScanner, &lpsnap->rcViewport);
    lpsnap->rcViewport.bottom = max(0, lpsnap->rcViewport.bottom - dySBar);
    /* Name helpers use the game's scratch buffer; a read-only capture preserves it. */
    memcpy(szWorkSav, szWork, sizeof(szWork));
    for (i = 0; i < (size_t)game.cPlanMax; i++) {
        lpobj = &lpsnap->rgobj[lpsnap->cObject++];
        lpobj->grType = grobjPlanet; lpobj->id = (uint16_t)i; lpobj->ith = ithNone;
        lppl = LpplFromId((int16_t)i);
        lpobj->iPlayer = lppl ? lppl->iPlayer : iplrNone;
        lpobj->det = lppl ? lppl->det : 0;
        CchSprintf(lpobj->szName, "%s", PszGetPlanetName((int16_t)i));
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
        lpobj->grType = grobjThing; lpobj->id = lpth->idFull; lpobj->ith = lpth->ith;
        lpobj->iPlayer = lpth->iplr; lpobj->det = 0;
        CchSprintf(lpobj->szName, "%s", PszGetThingName(lpth->idFull));
        ScanObjectPosition(lpsnap, lpobj, lpth->pt);
        lpobj->fScanSelected = sel.scan.grobj == grobjThing && sel.scan.ith == (int16_t)i;
        lpobj->fFiltered = (grbitScan & grbitScanViewMask) == scanViewNoPlayerInfo;
        if (lpth->ith == ithMinefield) {
            grRelation = lpth->iplr == idPlayer ? 1 :
                rgplr[idPlayer].rgmdRelation[lpth->iplr] == 1 ? 2 :
                rgplr[idPlayer].rgmdRelation[lpth->iplr] == 0 ? 4 : 8;
            lpobj->fFiltered = !(grbitScan & grbitScanMineFields) || (!lpobj->fScanSelected && !(grbitScanMines & grRelation));
        }
    }
    memcpy(szWork, szWorkSav, sizeof(szWork));
    return TRUE;
}

BOOL FSelectScannerObject(const SCANSNAPSHOT *lpsnap, uint16_t grType, uint16_t id, BOOL fReveal) {
    SCAN scan;
    int16_t i;
    POINT16 pt;
    if (!lpsnap || !FScannerReady() || lpsnap->lidGame != game.lid || lpsnap->turn != game.turn || lpsnap->idPlayer != idPlayer) return FALSE;
    memset(&scan, 0, sizeof(scan)); scan.idpl = idplNone; scan.ifl = iflNone; scan.ith = ithNone; scan.iwp = iwpNone;
    if (grType == grobjPlanet) {
        if (id >= (uint16_t)game.cPlanMax) return FALSE;
        scan.idpl = (int16_t)id; pt = rgptPlan[id];
    } else if (grType == grobjFleet) {
        for (i = 0; i < cFleet && rglpfl[i]; i++) if ((uint16_t)rglpfl[i]->id == id && !rglpfl[i]->fDead) break;
        if (i == cFleet || !rglpfl[i]) return FALSE;
        scan.ifl = i; pt = rglpfl[i]->pt;
    } else if (grType == grobjThing) {
        for (i = 0; lpThings && i < cThing; i++) if (lpThings[i].idFull == id) break;
        if (!lpThings || i == cThing) return FALSE;
        scan.ith = i; pt = lpThings[i].pt;
    } else return FALSE;
    /* Find co-located categories, then override the object identity: multiple
       fleets at one planet must not all resolve to the nearest fleet. */
    FFindNearestObject(pt, grobjPlanet | grobjFleet | grobjThing | mdExact, &scan);
    scan.grobj = grType; scan.grobjFull |= grType; scan.pt = pt; scan.iwp = iwpNone;
    if (grType == grobjPlanet) scan.idpl = (int16_t)id;
    else if (grType == grobjFleet) scan.ifl = i;
    else scan.ith = i;
    ChangeScanSel(&scan, 2);
    if (fReveal) FEnsurePointOnScreen(pt, TRUE);
    InvalidateRect(hwndScanner, NULL, FALSE);
    return TRUE;
}

static BOOL FWriteScanString(FILE *lpfile, const char *psz) {
    unsigned ch;

    if (!FUtf8Valid(psz, strlen(psz)) || fputc('"', lpfile) == EOF) return FALSE;
    while (*psz) {
        ch = (unsigned char)*psz++;
        if (ch == '"' || ch == '\\') { if (fputc('\\', lpfile) == EOF || fputc((int)ch, lpfile) == EOF) return FALSE; }
        else if (ch < 32) { if (fprintf(lpfile, "\\u%04x", ch) < 0) return FALSE; }
        else if (fputc((int)ch, lpfile) == EOF) return FALSE;
    }
    return fputc('"', lpfile) != EOF;
}

BOOL FWriteScannerSnapshot(const SCANSNAPSHOT *lpsnap, FILE *lpfile) {
    size_t i;
    const SCANOBJECT *lp;
    if (!lpsnap || !lpfile || (lpsnap->cObject != 0 && !lpsnap->rgobj)) return FALSE;
    if (fprintf(lpfile, "{\"schema\":1,\"game\":%ld,\"turn\":%u,\"player\":%d,\"zoom\":%d,\"viewport\":[%ld,%ld,%ld,%ld],\"objects\":[\n",
        (long)lpsnap->lidGame, lpsnap->turn, lpsnap->idPlayer, lpsnap->iZoom,
        (long)lpsnap->rcViewport.left,(long)lpsnap->rcViewport.top,(long)lpsnap->rcViewport.right,(long)lpsnap->rcViewport.bottom) < 0) return FALSE;
    for (i = 0; i < lpsnap->cObject; i++) {
        lp = &lpsnap->rgobj[i];
        if (!memchr(lp->szName, 0, sizeof(lp->szName))) return FALSE;
        if (fprintf(lpfile, "%s{\"kind\":%u,\"id\":%u,\"owner\":%d,\"detail\":%d,\"thingType\":%d,\"universe\":[%ld,%ld],\"client\":[%ld,%ld],\"screen\":[%ld,%ld],"
            "\"onScreen\":%s,\"filtered\":%s,\"scanSelected\":%s,\"mainSelected\":%s,\"name\":", i ? ",\n" : "", lp->grType,lp->id,lp->iPlayer,lp->det,lp->ith,
            (long)lp->ptUniverse.x,(long)lp->ptUniverse.y,(long)lp->ptClient.x,(long)lp->ptClient.y,(long)lp->ptScreen.x,(long)lp->ptScreen.y,
            lp->fOnScreen ? "true":"false",lp->fFiltered ? "true":"false",lp->fScanSelected ? "true":"false",lp->fMainSelected ? "true":"false") < 0 ||
            !FWriteScanString(lpfile,lp->szName) || fputc('}',lpfile) == EOF) return FALSE;
    }
    return fputs("\n]}\n",lpfile) != EOF && !ferror(lpfile);
}

static void ScannerInspectDetails(HWND hwnd) {
    int i = (int)SendMessage(GetDlgItem(hwnd, 1), LB_GETCURSEL, 0, 0);
    SCANOBJECT *lp;
    char sz[768];
    if (i < 0 || (size_t)i >= snapScanInspector.cObject) { SetWindowText(GetDlgItem(hwnd,2), "No active player scanner. Load a game and refresh."); return; }
    lp = &snapScanInspector.rgobj[i];
    CchSprintf(sz,"%s\r\nKind %u, ID %u, owner %d, detail %d\r\nUniverse (%ld, %ld)\r\nScanner (%ld, %ld), screen (%ld, %ld)\r\nOn screen: %s; filtered: %s\r\nScanner selection: %s; main selection: %s\r\nGame %ld, turn %u, player %d",
        lp->szName,lp->grType,lp->id,lp->iPlayer,lp->det,(long)lp->ptUniverse.x,(long)lp->ptUniverse.y,
        (long)lp->ptClient.x,(long)lp->ptClient.y,(long)lp->ptScreen.x,(long)lp->ptScreen.y,
        lp->fOnScreen?"yes":"no",lp->fFiltered?"yes":"no",lp->fScanSelected?"yes":"no",lp->fMainSelected?"yes":"no",
        (long)snapScanInspector.lidGame,snapScanInspector.turn,snapScanInspector.idPlayer);
    SetWindowText(GetDlgItem(hwnd,2),sz);
}

static void RefreshScannerInspector(HWND hwnd) {
    size_t i;
    char sz[424];
    const SCANOBJECT *lp;

    FreeScannerSnapshot(&snapScanInspector);
    SendMessage(GetDlgItem(hwnd, 1), LB_RESETCONTENT, 0, 0);
    if (FCaptureScanner(&snapScanInspector)) {
        for (i = 0; i < snapScanInspector.cObject; i++) {
            lp = &snapScanInspector.rgobj[i];
            CchSprintf(sz, "%s %u: %s%s", lp->grType == grobjPlanet ? "Planet" : lp->grType == grobjFleet ? "Fleet" : "Thing",
                       lp->id, lp->szName, lp->fOnScreen ? "" : " [offscreen]");
            SendMessage(GetDlgItem(hwnd, 1), LB_ADDSTRING, 0, (LPARAM)sz);
        }
    }
    SendMessage(GetDlgItem(hwnd, 1), LB_SETCURSEL, 0, 0);
    ScannerInspectDetails(hwnd);
}

static LRESULT CALLBACK ScannerInspectorProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    RECT rc;
    int i;
    SCANOBJECT *lp;

    switch (msg) {
    case WM_CREATE:
        if (!CreateWindow("ListBox", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
                          8, 40, 300, 300, hwnd, (HMENU)(INT_PTR)1, hInst, NULL) ||
            !CreateWindow("Edit", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                          320, 40, 320, 300, hwnd, (HMENU)(INT_PTR)2, hInst, NULL) ||
            !CreateWindow("Button", "Refresh", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                          8, 8, 90, 24, hwnd, (HMENU)(INT_PTR)3, hInst, NULL) ||
            !CreateWindow("Button", "Select / reveal", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                          108, 8, 130, 24, hwnd, (HMENU)(INT_PTR)4, hInst, NULL)) return -1;
        return 0;
    case WM_SIZE:
        GetClientRect(hwnd, &rc);
        MoveWindow(GetDlgItem(hwnd, 1), 8, 40, max(0, rc.right / 2 - 12), max(0, rc.bottom - 48), TRUE);
        MoveWindow(GetDlgItem(hwnd, 2), rc.right / 2, 40, max(0, rc.right / 2 - 8), max(0, rc.bottom - 48), TRUE);
        return 0;
    case WM_COMMAND:
        if (LOWORD(w) == 3 && HIWORD(w) == BN_CLICKED) RefreshScannerInspector(hwnd);
        else if (LOWORD(w) == 1 && HIWORD(w) == LBN_SELCHANGE) ScannerInspectDetails(hwnd);
        else if (LOWORD(w) == 4 && HIWORD(w) == BN_CLICKED) {
            i = (int)SendMessage(GetDlgItem(hwnd, 1), LB_GETCURSEL, 0, 0);
            if (i >= 0 && (size_t)i < snapScanInspector.cObject) {
                lp = &snapScanInspector.rgobj[i];
                if (!FSelectScannerObject(&snapScanInspector, lp->grType, lp->id, TRUE)) MessageBeep(MB_ICONEXCLAMATION);
                RefreshScannerInspector(hwnd);
                SendMessage(GetDlgItem(hwnd, 1), LB_SETCURSEL, i, 0);
                ScannerInspectDetails(hwnd);
            }
        }
        return 0;
    case WM_DESTROY:
        hwndScanInspector = NULL;
        FreeScannerSnapshot(&snapScanInspector);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, w, l);
    }
}

HWND ShowScannerInspector(void) {
    WNDCLASS wc = {0};

    if (!IsWindow(hwndFrame)) return NULL;
    if (StarsCompatWindowId(hwndFrame) == 0) return NULL;
    wc.lpszClassName = "StarsScannerInspector";
    if (!IsWindow(hwndScanInspector)) {
        if (!fScanInspectorClass) {
            wc.lpfnWndProc = ScannerInspectorProc;
            wc.hInstance = hInst;
            wc.hbrBackground = (HBRUSH)(INT_PTR)(COLOR_BTNFACE + 1);
            if (!RegisterClass(&wc)) return NULL;
            fScanInspectorClass = TRUE;
        }
        hwndScanInspector = CreateWindow(wc.lpszClassName, "Stars scanner objects", WS_OVERLAPPEDWINDOW,
                                         CW_USEDEFAULT, CW_USEDEFAULT, 800, 450, hwndFrame, NULL, hInst, NULL);
        if (!hwndScanInspector) return NULL;
    }
    RefreshScannerInspector(hwndScanInspector);
    ShowWindow(hwndScanInspector, SW_SHOWNORMAL);
    return hwndScanInspector;
}

void CloseScannerInspector(void) {
    if (IsWindow(hwndScanInspector)) DestroyWindow(hwndScanInspector);
    hwndScanInspector = NULL;
    FreeScannerSnapshot(&snapScanInspector);
}
