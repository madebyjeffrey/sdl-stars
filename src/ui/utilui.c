#include "win.h"

void SelectOursAtObject(POINT16 *ppt) {
    int16_t id;
    POINT16 pt;
    int16_t ish;
    int16_t i;
    FLEET  *lpfl;
    SCAN    scan;

    if (ppt->x == -1) {
        if (ppt->y & 0x8000) {
            SelectAdjFleet(0, ppt->y & 0x7fff);
            return;
        }
        pt = rgptPlan[ppt->y];
    } else {
        pt = *ppt;
    }
    id = idflNone;
    for (ish = 0; ish < cFleet; ish++) {
        lpfl = rglpfl[ish];
        if (!rglpfl[ish])
            break;
        if (pt.x == lpfl->pt.x && pt.y == lpfl->pt.y) {
            if (lpfl->iPlayer == idPlayer) {
                SelectAdjFleet(0, lpfl->id);
                return;
            }
            if (id == idflNone) {
                id = lpfl->id;
            }
        }
    }
    for (i = 0; i < game.cPlanMax; i++) {
        if (rgptPlan[i].x == pt.x && rgptPlan[i].y == pt.y) {
            SelectAdjPlanet(0, i);
            return;
        }
    }
    scan.iwp = iwpNone;
    if (FFindNearestObject(pt, grobjThing, &scan) && scan.grobj == grobjThing && scan.ith != ithNone && lpThings[scan.ith].ith == ithMineralPacket &&
        lpThings[scan.ith].thp.iWarp == 0) {
        ChangeScanSel(&scan, 1);
        FEnsurePointOnScreen(scan.pt, TRUE);
        UpdateWindow(hwndScanner);
        SendMessage(hwndScanner, WM_CHAR, 'v', 0);
    } else if (id != idflNone) {
        SelectAdjFleet(0, id);
    }
    return;
}

void DrawABunchOfStars(HDC hdc, RECT *prc) {
    int32_t lPixTot;
    int16_t iMax;
    int16_t dy;
    int16_t i;
    int16_t iClr;
    int16_t dx;
    RECT    rcOut;
    RECT    rc;

    rc = *prc;
    PushRandom(17, 11);
    InflateRect(&rc, -3, -3);
    dx = rc.right - rc.left;
    dy = rc.bottom - rc.top;
    lPixTot = (uint32_t)(dx * (int16_t)(rc.bottom - rc.top));
    for (iClr = 0; iClr < 5; iClr++) {
        iMax = LOWORD((int32_t)(lPixTot / rgDSDivCnt[iClr]));
        for (i = 0; i < iMax; i++) {
            rcOut.left = Random(dx) + rc.left;
            rcOut.top = Random(dy) + rc.top;
            rcOut.right = rcOut.left + 1;
            rcOut.bottom = rcOut.top + 1;
            SetBkColor(hdc, rgcrDrawStars[iClr]);
            ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcOut, NULL, 0, NULL);
        }
    }
    for (iClr = 0; iClr < 4; iClr++) {
        iMax = LOWORD((int32_t)(lPixTot / rgDSDivCnt2[iClr])) + 1;
        for (i = 0; i < iMax; i++) {
            rcOut.left = Random(dx) + rc.left;
            rcOut.top = Random(dy) + rc.top;
            rcOut.right = rcOut.left + 3;
            rcOut.bottom = rcOut.top + 3;
            SetBkColor(hdc, rgcrDrawStars2b[iClr]);
            ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcOut, NULL, 0, NULL);
            SetBkColor(hdc, rgcrDrawStars2a[iClr]);
            InflateRect(&rcOut, -1, 0);
            ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcOut, NULL, 0, NULL);
            InflateRect(&rcOut, 1, -1);
            ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rcOut, NULL, 0, NULL);
        }
    }
    PopRandom();
    return;
}

void DrawPlanetPrintDot(HDC hdc, int16_t x, int16_t y, int16_t iSize) {
    if (iSize == 0) {
        PatBlt(hdc, x - 3, y - 1, 7, 3, BLACKNESS);
        PatBlt(hdc, x - 1, y - 3, 3, 7, BLACKNESS);
        PatBlt(hdc, x - 2, y - 2, 5, 5, BLACKNESS);
    } else {
        PatBlt(hdc, x - 5, y - 2, 11, 5, BLACKNESS);
        PatBlt(hdc, x - 2, y - 5, 5, 11, BLACKNESS);
        PatBlt(hdc, x - 4, y - 3, 9, 7, BLACKNESS);
        PatBlt(hdc, x - 3, y - 4, 7, 9, BLACKNESS);
    }
    return;
}
