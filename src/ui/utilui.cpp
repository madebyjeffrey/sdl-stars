#include "win.h"
#include "appstate.h"
#include "uistate.h"
#include "control_io.h"
#include "presentation_io.h"
#include "random_io.h"

void SelectOursAtObject(APPSESSION &app, POINT16 *ppt) {
    int16_t id;
    POINT16 pt;
    int16_t ish;
    int16_t i;
    FLEET  *lpfl;
    SCAN    scan;

    if (ppt->x == -1) {
        if (ppt->y & 0x8000) {
            PresentationIO().pfnSelectFleet(app, 0, ppt->y & 0x7fff);
            return;
        }
        pt = app.state.rgptPlan[ppt->y];
    } else {
        pt = *ppt;
    }
    id = idflNone;
    for (ish = 0; ish < app.state.cFleet; ish++) {
        lpfl = app.state.rglpfl[ish];
        if (!app.state.rglpfl[ish])
            break;
        if (pt.x == lpfl->pt.x && pt.y == lpfl->pt.y) {
            if (lpfl->iPlayer == app.state.idPlayer) {
                PresentationIO().pfnSelectFleet(app, 0, lpfl->id);
                return;
            }
            if (id == idflNone) {
                id = lpfl->id;
            }
        }
    }
    for (i = 0; i < app.state.game.cPlanMax; i++) {
        if (app.state.rgptPlan[i].x == pt.x && app.state.rgptPlan[i].y == pt.y) {
            PresentationIO().pfnSelectPlanet(app, 0, i);
            return;
        }
    }
    scan.iwp = iwpNone;
    if (FFindNearestObject(app, pt, grobjThing, &scan) && scan.grobj == grobjThing && scan.ith != ithNone && app.state.lpThings[scan.ith].ith() == ithMineralPacket &&
        app.state.lpThings[scan.ith].thp().iWarp() == 0) {
        ChangeScanSel(app, &scan, 1);
        FEnsurePointOnScreen(app, scan.pt, 1);
        ControlIO().pfnUpdateWindow(app.lpui->hwndScanner);
        ControlIO().pfnSendMessage(app.lpui->hwndScanner, win32::WM_CHAR, 'v', 0);
    } else if (id != idflNone) {
        PresentationIO().pfnSelectFleet(app, 0, id);
    }
    return;
}

void DrawABunchOfStars(APPSESSION &app, HDC hdc, RECT *prc) {
    int32_t lPixTot;
    int16_t iMax;
    int16_t dy;
    int16_t i;
    int16_t iClr;
    int16_t dx;
    RECT    rcOut;
    RECT    rc;

    rc = *prc;
    PushRandom(app, 17, 11);
    win32::InflateRect(&rc, -3, -3);
    dx = rc.right - rc.left;
    dy = rc.bottom - rc.top;
    lPixTot = static_cast<uint32_t>((dx * static_cast<int16_t>((rc.bottom - rc.top))));
    for (iClr = 0; iClr < 5; iClr++) {
        iMax = LOWORD(static_cast<int32_t>((lPixTot / rgDSDivCnt[iClr])));
        for (i = 0; i < iMax; i++) {
            rcOut.left = RandomIO().pfnRandom(app, dx) + rc.left;
            rcOut.top = RandomIO().pfnRandom(app, dy) + rc.top;
            rcOut.right = rcOut.left + 1;
            rcOut.bottom = rcOut.top + 1;
            win32::SetBkColor(hdc, rgcrDrawStars[iClr]);
            win32::ExtTextOutA(hdc, 0, 0, win32::ETO_OPAQUE, &rcOut, nullptr, 0, nullptr);
        }
    }
    for (iClr = 0; iClr < 4; iClr++) {
        iMax = LOWORD(static_cast<int32_t>((lPixTot / rgDSDivCnt2[iClr]))) + 1;
        for (i = 0; i < iMax; i++) {
            rcOut.left = RandomIO().pfnRandom(app, dx) + rc.left;
            rcOut.top = RandomIO().pfnRandom(app, dy) + rc.top;
            rcOut.right = rcOut.left + 3;
            rcOut.bottom = rcOut.top + 3;
            win32::SetBkColor(hdc, rgcrDrawStars2b[iClr]);
            win32::ExtTextOutA(hdc, 0, 0, win32::ETO_OPAQUE, &rcOut, nullptr, 0, nullptr);
            win32::SetBkColor(hdc, rgcrDrawStars2a[iClr]);
            win32::InflateRect(&rcOut, -1, 0);
            win32::ExtTextOutA(hdc, 0, 0, win32::ETO_OPAQUE, &rcOut, nullptr, 0, nullptr);
            win32::InflateRect(&rcOut, 1, -1);
            win32::ExtTextOutA(hdc, 0, 0, win32::ETO_OPAQUE, &rcOut, nullptr, 0, nullptr);
        }
    }
    PopRandom(app);
    return;
}

void DrawPlanetPrintDot(HDC hdc, int16_t x, int16_t y, int16_t iSize) {
    if (iSize == 0) {
        win32::PatBlt(hdc, x - 3, y - 1, 7, 3, win32::BLACKNESS);
        win32::PatBlt(hdc, x - 1, y - 3, 3, 7, win32::BLACKNESS);
        win32::PatBlt(hdc, x - 2, y - 2, 5, 5, win32::BLACKNESS);
    } else {
        win32::PatBlt(hdc, x - 5, y - 2, 11, 5, win32::BLACKNESS);
        win32::PatBlt(hdc, x - 2, y - 5, 5, 11, win32::BLACKNESS);
        win32::PatBlt(hdc, x - 4, y - 3, 9, 7, win32::BLACKNESS);
        win32::PatBlt(hdc, x - 3, y - 4, 7, 9, win32::BLACKNESS);
    }
    return;
}
