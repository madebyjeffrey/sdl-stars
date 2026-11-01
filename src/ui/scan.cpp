#include "win.h"
#include <algorithm>
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "fleet_query_io.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"
#include "heap_io.h"
#include "colony_io.h"
#include "planet_query_io.h"
#include "part_query_io.h"
#include "race_query_io.h"
#include "alert_io.h"

std::uint32_t rgcrScanMine[3] = {16711680, 65535, 255};
std::int16_t  vrgPopRad[19] = {25, 50, 100, 200, 400, 800, 1000, 1500, 2250, 3000, 4000, 5000, 6000, 7500, 9000, 11000, 14000, 18000, 25000};

// Wheel deltas short of a notch, for zooming and sideways scrolling.
static std::int16_t dWheelZoom;
static std::int16_t dWheelScroll;

// Dragging the scanner with the left button pans it. fPanDown is set while
// a plain left press may still become a pan, fPanning once it has. The
// drag is measured from ptPan, where the scanner's top was xPanTop, yPanTop.
static std::int16_t fPanDown;
static std::int16_t fPanning;
static POINT16 ptPan;
static std::int16_t xPanTop;
static std::int16_t yPanTop;

// FAltDown tells whether Alt is down; Wine's Mac driver sends Alt for Cmd.
// Alt+click (Cmd+click) adds a waypoint at the fastest useful speed.
static std::int16_t FAltDown(void) { return ControlIO().pfnGetKeyState(win32::VK_MENU) < 0; }

LRESULT  ScannerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HDC         hdc;
    POINT16     pt;
    PAINTSTRUCT ps;
    RECT        rc;
    ScanZoom    iScanNew;
    HPEN        hpenSav;
    std::int16_t     iRopSav;
    std::int16_t     i;
    std::uint32_t    tick;
    PLANET      plT;
    std::int16_t     fChgScan;
    SCAN        scan;
    std::int16_t     c;
    THING      *lpth;
    FLEET      *lpfl;
    std::int16_t     fSep;
    std::int32_t    *rgid;
    std::int32_t     idSel;
    std::int16_t     iChecked;
    std::int16_t     iSel;
    THING      *lpthMac;
    std::int16_t     id;
    std::int16_t     d;
    std::int16_t     dy;
    std::int16_t     dx;

    switch (msg) {
    case win32::WM_MDIACTIVATE:
        app.lpui->hwndActive = (lParam == reinterpret_cast<LPARAM>(hwnd)) == 0 ? nullptr : hwnd;
        break;
    case win32::WM_CREATE:
        app.lpui->yScanTop = 1000;
        app.lpui->xScanTop = 1000;
        break;
    case win32::WM_CHAR:
        if (wParam == 'v' || wParam == 'V') {
            hdc = win32::GetDC(app.lpui->hwndScanner);
            pt = app.state.sel.scan.pt;
            LogicalToScan(app, &pt);
            iRopSav = win32::SetROP2(hdc, win32::R2_XORPEN);
            hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, app.lpui->hpenYellow));
            for (i = 0; i < 2; i++) {
                win32::MoveToEx(hdc, pt.x - 300, pt.y - 300, nullptr);
                win32::LineTo(hdc, pt.x + 300, pt.y + 300);
                win32::MoveToEx(hdc, pt.x - 300, pt.y + 300, nullptr);
                win32::LineTo(hdc, pt.x + 300, pt.y - 300);
                if (i == 0) {
                    tick = win32::GetTickCount();
                    while (win32::GetTickCount() < tick + 150) {
                    }
                }
            }
            win32::SelectObject(hdc, hpenSav);
            win32::SetROP2(hdc, iRopSav);
            win32::ReleaseDC(app.lpui->hwndScanner, hdc);
            break;
        }
        if (wParam == '-') {
            iScanNew = static_cast<ScanZoom>(app.state.iScanZoom - 1);
            if (iScanNew < zoom25) {
                iScanNew = zoom25;
            }
        } else {
            iScanNew = static_cast<ScanZoom>(app.state.iScanZoom + 1);
            if (iScanNew > zoom400) {
                iScanNew = zoom400;
            }
        }
        if (iScanNew != app.state.iScanZoom) {
            ControlIO().pfnSendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, iScanNew + 3905, 0);
        }
        break;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        if (app.state.rglpfl && !app.state.gd.fNoScannerDraw) {
            ControlIO().pfnGetClientRect(hwnd, &rc);
            DrawScannerSBar(app, hdc, &ps.rcPaint, nullptr, 1);
            rc.bottom -= app.lpui->dySBar;
            DrawScanner(app, hdc, &ps.rcPaint);
        }
        win32::EndPaint(hwnd, &ps);
        break;
    case win32::WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(app.lpui->hwndScanner, &pt);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        if (win32::PtInRect(&rc, PointFrom16(pt)) == 0)
            goto Default;
        rc.bottom -= app.lpui->dySBar;
        if (win32::PtInRect(&rc, PointFrom16(pt)) == 0) {
            win32::SetCursor(win32::LoadCursorA(nullptr, win32::MAKEINTRESOURCE(32512)));
        } else if (app.state.sel.grobj == grobjFleet && ((ControlIO().pfnGetAsyncKeyState(win32::VK_SHIFT) & 0xfffe) || (app.lpui->grbitScan & grbitScanAddWaypoints) || FAltDown())) {
            win32::SetCursor(app.lpui->hcurScanAdd);
        } else if (FNearAWayPoint(app, pt, 0)) {
            win32::SetCursor(app.lpui->hcurOpenGrab);
        } else if (app.state.gd.fSetMassMode || app.state.gd.fSetRouteMode ||
                   (app.state.sel.grobj == grobjPlanet &&
                    (((ControlIO().pfnGetAsyncKeyState(win32::VK_SHIFT) & 0xfffe) && ColonyIO().pfnWarp(app, &app.state.sel.pl, nullptr) > 0) || (ControlIO().pfnGetAsyncKeyState(win32::VK_CONTROL) & 0xfffe)))) {
            win32::SetCursor(app.lpui->hcurScanAdd);
        } else {
            win32::SetCursor(app.lpui->hcurScanner);
        }
        return 1;
    case win32::WM_LBUTTONDOWN:
    case win32::WM_LBUTTONDBLCLK:
    case win32::WM_RBUTTONDOWN:
    case win32::WM_MBUTTONDOWN:
        ControlIO().pfnSetFocus(app.lpui->hwndFrame);
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        if (pt.y >= rc.bottom - app.lpui->dySBar) {
            if (msg != win32::WM_LBUTTONDOWN || pt.y >= rc.bottom - (app.lpui->dySBar >> 1) || !(app.state.sel.scan.grobjFull & (grobjPlanet | grobjFleet)))
                break;
            if (app.state.sel.scan.grobj == grobjFleet) {
                app.lpui->GlobalPD.grPopup = grPopupFleet;
                app.lpui->GlobalPD.lpfl = app.state.rglpfl[app.state.sel.scan.ifl];
            } else {
                app.lpui->GlobalPD.grPopup = grPopupUnknownObj;
            }
            Popup(app, hwnd, pt.x, pt.y);
            break;
        }
        // A plain left press acts as a click and, if the mouse then moves,
        // pans. Shift, Ctrl and the waypoint, mass driver and route modes
        // keep the press for themselves; a waypoint drag ends with the
        // button up, which WM_MOUSEMOVE notices.
        fPanDown = msg == win32::WM_LBUTTONDOWN && !(wParam & (win32::MK_SHIFT | win32::MK_CONTROL)) && !app.state.gd.fSetMassMode && !app.state.gd.fSetRouteMode &&
                   !(app.state.sel.grobj == grobjFleet && ((app.lpui->grbitScan & grbitScanAddWaypoints) || FAltDown()));
        ptPan = pt;
        ScanToLogical(app, &pt);
        FFindNearestObject(app, pt, static_cast<GrobjClass>(app.state.gd.fSetMassMode != 0 || app.state.gd.fSetRouteMode ? grobjPlanet : grobjPlanet | grobjFleet | grobjOther | grobjThing), &scan);
        if ((app.state.gd.fSetMassMode || (app.state.sel.grobj == grobjPlanet && (wParam & 4) && ColonyIO().pfnWarp(app, &app.state.sel.pl, nullptr) > 0)) && msg == win32::WM_LBUTTONDOWN) {
            DrawShipScanPath(app, nullptr, 0);
            if (scan.idpl == app.state.sel.pl.id) {
                app.state.sel.pl.idFling = 0;
            } else {
                app.state.sel.pl.idFling = scan.idpl + 1;
            }
            PlanetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.pl);
            app.state.gd.fSetMassMode = 0;
            DrawShipScanPath(app, nullptr, 1);
            ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(tileStarbaseOrWaypoint | tileMinimized));
            break;
        }
        if ((app.state.gd.fSetRouteMode || (app.state.sel.grobj == grobjPlanet && (wParam & 8))) && msg == win32::WM_LBUTTONDOWN) {
            DrawShipScanPath(app, nullptr, 0);
            if (scan.idpl == app.state.sel.pl.id) {
                app.state.sel.pl.idRoute = 0;
            } else {
                app.state.sel.pl.idRoute = scan.idpl + 1;
            }
            PlanetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.pl);
            app.state.gd.fSetRouteMode = 0;
            DrawShipScanPath(app, nullptr, 1);
            ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(tileProductionOrOrbit | tileMinimized));
            break;
        }
        if (msg == win32::WM_MBUTTONDOWN || (msg == win32::WM_RBUTTONDOWN && (wParam & 4))) {
            FHandleMeasuringTape(app, &scan, pt);
            break;
        }
        if (msg == win32::WM_RBUTTONDOWN) {
            iChecked = -1;
            pt = scan.pt;
            /* Room for every fleet and thing here, a planet and a separator.
               The original stopped at 100 entries, so objects past them
               couldn't be picked (target list overload). */
            rgid = LpAlloc<std::int32_t>(app, (app.state.cFleet + app.state.cThing + 3) * sizeof(std::int32_t), htMisc);
            if (scan.grobjFull & grobjPlanet) {
                rgid[0] = scan.idpl;
                rgid[1] = -1;
                c = 2;
                if (app.state.sel.grobj == grobjPlanet && app.state.sel.id == scan.idpl) {
                    iChecked = 0;
                }
            } else {
                c = 0;
            }
            for (i = 0; i < app.state.cFleet; i++) {
                lpfl = app.state.rglpfl[i];
                if (!app.state.rglpfl[i])
                    break;
                if (pt.x == lpfl->pt.x && pt.y == lpfl->pt.y) {
                    if (app.state.sel.grobj == grobjFleet && lpfl->id == app.state.sel.id) {
                        iChecked = c;
                    }
                    rgid[c++] = lpfl->id | 0x80000000;
                }
            }
            if (c == 2 && (scan.grobjFull & grobjPlanet)) {
                c = 1;
            }
            fSep = c == 0;
            lpth = app.state.lpThings;
            lpthMac = app.state.lpThings + app.state.cThing;
            for (; lpth < lpthMac; lpth++) {
                if (pt.x == lpth->pt().x() && pt.y == lpth->pt().y()) {
                    if (!fSep) {
                        rgid[c++] = -1;
                        fSep = 1;
                    }
                    rgid[c++] = static_cast<std::uint32_t>(static_cast<std::uint16_t>(lpth->idFull())) | 0x20000000;
                }
            }
            LogicalToScan(app, &pt);
            iSel = ControlIO().pfnPopupMenu(app, hwnd, pt.x, pt.y, c, rgid, nullptr, iChecked, 1);
            idSel = iSel >= 0 ? rgid[iSel] : 0;
            HeapIO().pfnFree(app, rgid, htMisc);
            if (iSel < 0)
                break;
            if (idSel & 0x80000000) {
                scan.grobj = grobjFleet;
                id = LOWORD(idSel);
                for (i = 0; i < app.state.cFleet; i++) {
                    lpfl = app.state.rglpfl[i];
                    if (!app.state.rglpfl[i] || lpfl->id == id)
                        break;
                }
                scan.ifl = i;
            } else if (idSel & 0x20000000) {
                scan.grobj = grobjThing;
                lpth = app.state.lpThings;
                lpthMac = app.state.lpThings + app.state.cThing;
                for (; lpth < lpthMac && lpth->idFull() != LOWORD(idSel); lpth++) {
                }
                scan.ith = static_cast<std::int16_t>((reinterpret_cast<std::uint8_t *>(lpth) - reinterpret_cast<std::uint8_t *>(app.state.lpThings))) / 18;
            } else {
                scan.grobj = grobjPlanet;
            }
            ChangeScanSel(app, &scan, 2);
            if (scan.grobj == grobjPlanet && (!PlanetQueryIO().pfnLookup(app, scan.idpl, &plT) || plT.iPlayer != app.state.idPlayer))
                break;
            goto DblClick;
        } else {
            if (app.state.sel.grobj == grobjFleet && ((wParam & 4) || (app.lpui->grbitScan & grbitScanAddWaypoints) || FAltDown())) {
                FAddWayPoint(app, pt, &scan);
                break;
            }
            fChgScan = scan.pt.x == app.state.sel.scan.pt.x && scan.pt.y == app.state.sel.scan.pt.y;
            ChangeScanSel(app, &scan, 1);
            if ((FNearAWayPoint(app, pt, 1) && FHandleWayPointDrag(app, pt)) || !fChgScan || (scan.grobj != grobjFleet && scan.grobj != grobjPlanet))
                break;
            if (scan.pt.x == app.state.sel.pt.x && scan.pt.y == app.state.sel.pt.y) {
                if (!FGetNextObjHere(app, &scan, 1))
                    break;
            } else if (scan.grobj == grobjPlanet) {
                if (!PlanetQueryIO().pfnLookup(app, scan.idpl, &plT))
                    break;
                if (plT.iPlayer != app.state.idPlayer || ((scan.grobjFull & grobjFleet) && app.state.sel.grobj == grobjPlanet && scan.idpl == app.state.sel.id)) {
                    if (!(scan.grobjFull & grobjFleet))
                        break;
                    scan.grobj = grobjFleet;
                }
            }
        }
    DblClick:
        if ((scan.grobj == grobjFleet && app.state.rglpfl[scan.ifl]->iPlayer != app.state.idPlayer) || scan.grobj == grobjThing)
            break;
        if (scan.grobj == grobjFleet) {
            scan.iwp = iwpNone;
        }
        ChangeScanSel(app, &scan, 1);
        RedrawScanSel(app, nullptr, 0);
        ChangeMainObjSel(app, scan.grobj, scan.grobj == grobjPlanet ? scan.idpl : app.state.rglpfl[scan.ifl]->id);
        RedrawScanSel(app, nullptr, 1);
        if (scan.grobj != grobjFleet || !(scan.grobjFull & grobjPlanet))
            break;
        scan.grobj = grobjPlanet;
        ChangeScanSel(app, &scan, 1);
        break;
    case win32::WM_MOUSEMOVE:
        if (!fPanDown)
            goto Default;
        if (!(wParam & win32::MK_LBUTTON)) {
            fPanDown = 0;
            goto Default;
        }
        pt.x = static_cast<short>(LOWORD(lParam));
        pt.y = static_cast<short>(HIWORD(lParam));
        if (!fPanning) {
            if (std::abs(pt.x - ptPan.x) < win32::GetSystemMetrics(win32::SM_CXDRAG) && std::abs(pt.y - ptPan.y) < win32::GetSystemMetrics(win32::SM_CYDRAG))
                break;
            fPanning = 1;
            xPanTop = app.lpui->xScanTop;
            yPanTop = app.lpui->yScanTop;
            win32::SetCapture(hwnd);
            win32::SetCursor(app.lpui->hcurCloseGrab);
        }
        // The galaxy follows the mouse. Scanner tops are multiples of 4, so
        // the new tops are rounded to them from where the drag began.
        d = ((xPanTop - ScanToPt(app, pt.x - ptPan.x)) + 2) & 0xfffc;
        if (d != app.lpui->xScanTop) {
            ControlIO().pfnSendMessage(hwnd, win32::WM_HSCROLL, win32::MAKEWPARAM(win32::SB_THUMBPOSITION, static_cast<WORD>(d)), 0);
        }
        d = ((yPanTop - ScanToPt(app, pt.y - ptPan.y)) + 2) & 0xfffc;
        if (d != app.lpui->yScanTop) {
            ControlIO().pfnSendMessage(hwnd, win32::WM_VSCROLL, win32::MAKEWPARAM(win32::SB_THUMBPOSITION, static_cast<WORD>(d)), 0);
        }
        break;
    case win32::WM_LBUTTONUP:
        fPanDown = 0;
        if (fPanning) {
            win32::ReleaseCapture();
        }
        goto Default;
    case win32::WM_CAPTURECHANGED:
        if (fPanning) {
            fPanning = 0;
            fPanDown = 0;
        }
        goto Default;
    case win32::WM_MOUSEWHEEL:
    case win32::WM_MOUSEHWHEEL:
        // The wheel zooms at the cursor, as map viewers do; Ctrl+wheel, a
        // touchpad pinch, zooms too. The tilt wheel and Shift+wheel scroll
        // sideways, wheel down to the right.
        d = win32::GET_WHEEL_DELTA_WPARAM(wParam);
        if (msg == win32::WM_MOUSEWHEEL && !(win32::GET_KEYSTATE_WPARAM(wParam) & win32::MK_SHIFT)) {
            c = CWheelNotches(&dWheelZoom, d);
            if (c == 0)
                break;
            iScanNew = static_cast<ScanZoom>(app.state.iScanZoom + c);
            if (iScanNew < zoom25) {
                iScanNew = zoom25;
            } else if (iScanNew > zoom400) {
                iScanNew = zoom400;
            }
            if (iScanNew != app.state.iScanZoom) {
                // A pan in progress is measured at the old zoom.
                if (fPanning) {
                    win32::ReleaseCapture();
                }
                fPanDown = 0;
                pt.x = static_cast<short>(LOWORD(lParam));
                pt.y = static_cast<short>(HIWORD(lParam));
                ScreenToClient16(hwnd, &pt);
                ZoomScanAt(app, iScanNew, pt);
            }
            break;
        }
        c = CWheelNotches(&dWheelScroll, msg == win32::WM_MOUSEWHEEL ? -d : d);
        if (c == 0)
            break;
        d = c * CWheelLines(app.lpui->dScanPage / app.lpui->dScanInc) * app.lpui->dScanInc;
        ControlIO().pfnSendMessage(hwnd, win32::WM_HSCROLL, win32::MAKEWPARAM(win32::SB_THUMBPOSITION, static_cast<WORD>((app.lpui->xScanTop + d))), 0);
        break;
    case win32::WM_SIZE:
        SetScanScrollBars(app, hwnd);
        // A queued resize must not replay a zoom that changed before dispatch.
        win32::PostMessageA(hwnd, WM_STARS_SCAN_RESIZE, 0, 0);
        goto Default;
    case WM_STARS_SCAN_RESIZE:
        ControlIO().pfnSendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, app.state.iScanZoom + 3905, 0);
        break;
    case win32::WM_HSCROLL:
    case win32::WM_VSCROLL:
        if (LOWORD(wParam) <= win32::SB_THUMBTRACK) {
            switch (LOWORD(wParam)) {
            case win32::SB_LINEUP:
                d = -app.lpui->dScanInc;
                break;
            case win32::SB_LINEDOWN:
                d = app.lpui->dScanInc;
                break;
            case win32::SB_PAGEUP:
                d = -app.lpui->dScanPage;
                break;
            case win32::SB_PAGEDOWN:
                d = app.lpui->dScanPage;
                break;
            case win32::SB_THUMBPOSITION:
            case win32::SB_THUMBTRACK:
                d = static_cast<short>(HIWORD(wParam)) - (msg == win32::WM_VSCROLL ? app.lpui->yScanTop : app.lpui->xScanTop);
                d &= 0xfffc;
            }
        } else {
            d = 0;
        }
        if (d == 0)
            break;
        if (msg == win32::WM_VSCROLL) {
            dy = app.lpui->yScanTop;
            win32::SetScrollPos(hwnd, win32::SB_VERT, app.lpui->yScanTop + d, 1);
            app.lpui->yScanTop = win32::GetScrollPos(hwnd, win32::SB_VERT);
            ScrollScanner(app, 0, PtToScan(app, dy - app.lpui->yScanTop));
            break;
        }
        dx = app.lpui->xScanTop;
        win32::SetScrollPos(hwnd, win32::SB_HORZ, app.lpui->xScanTop + d, 1);
        app.lpui->xScanTop = win32::GetScrollPos(hwnd, win32::SB_HORZ);
        ScrollScanner(app, PtToScan(app, dx - app.lpui->xScanTop), 0);
        break;
    default:
    Default:
        return ui::DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

std::int16_t DrawScanner(APPSESSION &app, HDC hdc, RECT *prc) {
    std::array<char, 360> szObjectName{};
    std::int16_t  xOff;
    std::int16_t  dExpand;
    HPEN     hpenSav;
    FLEET   *lpflT;
    std::int16_t  j;
    std::int16_t  yTop;
    std::int16_t  xMax;
    POINT16  pt;
    std::int16_t  id;
    COLORREF crFore;
    std::int16_t  iBkPrev;
    POINT16  ptD;
    PLANET  *lpplMac;
    std::int16_t  yBmp;
    std::int16_t  dy;
    HBITMAP  hbmpXSav;
    HBITMAP  hbmpScreen;
    std::int16_t  id2;
    HDC      hdcScreen;
    PLANET  *lppl;
    std::int16_t  yMax;
    char     rgWhatsHere[999];
    HDC      hdcMem;
    THING   *lpth;
    FLEET   *lpfl;
    std::int16_t  i;
    std::int16_t  xMin;
    HBITMAP  hbmpSav;
    std::int16_t  iord;
    RECT     rcClip;
    std::int16_t  yOff;
    RECT     rcDraw;
    std::int16_t  dRange;
    std::int16_t  idP;
    POINT16  ptO;
    std::int16_t  fSelected;
    std::int16_t  fMA;
    std::int16_t  fStarbase;
    POINT16  ptSelMain;
    THING   *lpthMac;
    std::int16_t  fStargate;
    ScanView mdScanBase;
    std::int16_t  yMin;
    std::int16_t  dx;
    HBRUSH   hbrSav;
    std::int16_t  xLeft;
    std::int16_t  fDoDraw;
    POINT16  ptOrigin;
    RECT     rc;
    std::int32_t  l;
    COLORREF crBack;
    std::int16_t  rgy[250];
    std::int16_t  fPlanetScanner;
    std::int16_t  rgx[250];
    std::int16_t  rgrad[250];
    DRAWCIR  dc;
    std::int16_t  dPlanRange;
    std::int16_t  dThingRange;
    std::int16_t  ropSav;
    std::int16_t  fDetonating;
    POINT16  pt2;
    THING   *lpthDest;
    HBITMAP  hbmpTrSav;
    std::int16_t  fTerra;
    std::int16_t  dRad;
    std::int16_t  pctDesire;
    HBRUSH   hbr;
    std::int16_t  iOff;
    std::int16_t  xOut;
    std::int16_t  fConc;
    std::int16_t  yOut;
    std::int16_t  iRel;
    std::int32_t  lPop;
    COLORREF cr;

    mdScanBase = static_cast<ScanView>(app.lpui->grbitScan & grbitScanViewMask);
    hdcScreen = nullptr;
    hbmpScreen = nullptr;
    if (!app.state.rglpfl || app.state.gd.fNoScannerDraw || app.state.gd.fGeneratingTurn) {
        return 0;
    }
    xLeft = app.lpui->xScanTop;
    yTop = app.lpui->yScanTop;
    ptOrigin.x = PtToScan(app, 0xfa0 - app.lpui->xScanTop) & 7;
    ptOrigin.y = PtToScan(app, 0xfa0 - app.lpui->yScanTop) & 7;
    ControlIO().pfnGetClientRect(app.lpui->hwndScanner, &rc);
    win32::ExcludeClipRect(hdc, 0, rc.bottom - app.lpui->dySBar, rc.right, rc.bottom);
    l = static_cast<std::int16_t>((prc->right - prc->left));
    l = static_cast<std::uint32_t>((l * static_cast<std::int16_t>((prc->bottom - prc->top))));
    if (l < 48000) {
        hdcMem = win32::CreateCompatibleDC(hdc);
        if (hdcMem) {
            hbmpScreen = win32::CreateCompatibleBitmap(hdc, prc->right - (prc->left & 0xfff8), prc->bottom - (prc->top & 0xfff8));
            if (!hbmpScreen) {
                win32::DeleteDC(hdcMem);
            }
        }
        if (hbmpScreen) {
            hdcScreen = hdc;
            hdc = hdcMem;
            hbmpXSav = static_cast<HBITMAP>(win32::SelectObject(hdc, hbmpScreen));
            win32::SetWindowOrgEx(hdc, prc->left & 0xfff8, prc->top & 0xfff8, nullptr);
            pt.y = 0;
            pt.x = 0;
            ClientToScreen16(app.lpui->hwndScanner, &pt);
            ptOrigin.x = (ptOrigin.x + 8 - (pt.x & 7)) & 7;
            ptOrigin.y = (ptOrigin.y + 8 - (pt.y & 7)) & 7;
            rcDraw = *prc;
        }
    }
    win32::FillRect(hdc, prc, static_cast<HBRUSH>(win32::GetStockObject(win32::BLACK_BRUSH)));
    rcClip = *prc;
    dExpand = (app.state.iScanZoom >= zoom200 && mdScanBase == scanViewPopulation) ||
                      (app.state.iScanZoom >= zoom100 && (mdScanBase == scanViewSurfaceMinerals || mdScanBase == scanViewMineralConc))
                  ? 20
                  : 9;
    ExpandRc(prc, dExpand, dExpand);
    prc->bottom += 14;
    dx = ScanToPt(app, prc->right - prc->left);
    dy = ScanToPt(app, prc->bottom - prc->top);
    xMin = ScanToPt(app, prc->left) + xLeft;
    xMax = xMin + dx;
    yMax = app.state.dGalInv - yTop - ScanToPt(app, prc->top);
    yMin = yMax - dy;
    xOff = -xLeft;
    yOff = app.state.dGalInv - yTop;
    i = 0;
    id = 1;
    while (i < 16) {
        if (app.state.rgshdef[i].fFree) {
            app.lpui->grbitScanShip &= ~(id & app.lpui->grbitScanShip);
        }
        i++;
        id *= 2;
    }
    hdcMem = win32::CreateCompatibleDC(hdc);
    hbmpSav = static_cast<HBITMAP>(win32::SelectObject(hdcMem, app.lpui->hbmpScanner));
    if (app.state.sel.grobj != grobjNone) {
        ptSelMain = app.state.sel.pt;
    } else {
        ptSelMain.y = -2;
        ptSelMain.x = -2;
    }
    if (!app.state.gd.fFleetLinkValid) {
        LinkFleets(app, 0);
    } else {
        for (i = 0; i < app.state.cFleet; i++) {
            lpfl = app.state.rglpfl[i];
            if (!app.state.rglpfl[i])
                break;
            lpfl->fDone = 0;
        }
    }
    if (app.lpui->grbitScan & grbitScanCoverage) {
        fPlanetScanner = 0;
        dc.rgx = rgx;
        dc.rgy = rgy;
        dc.rgrad = rgrad;
        dc.cCur = 0;
        dc.cMax = 250;
        dc.hdc = hdc;
        dc.rcClip = *prc;
        dc.fCovered = 0;
        dc.fHollowOut = 0;
        win32::IntersectClipRect(hdc, rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);
        hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrRadar));
        hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, app.lpui->hpenRadar));
        lppl = app.state.lpPlanets;
        lpplMac = app.state.lpPlanets + app.state.cPlanet;
        for (; lppl < lpplMac; lppl++) {
            if (lppl->iPlayer == app.state.idPlayer) {
                dRange = GetPlanetScannerRange(app, lppl, &dPlanRange);
                if (dPlanRange > 0) {
                    fPlanetScanner = 1;
                }
                if (app.lpui->vpctRadarView < 100) {
                    dRange = win32::MulDiv(dRange, app.lpui->vpctRadarView, 100);
                }
                id = lppl->id;
                rc.left = PtToScan(app, xOff + app.state.rgptPlan[id].x - dRange);
                rc.top = PtToScan(app, yOff - app.state.rgptPlan[id].y - dRange);
                rc.right = PtToScan(app, xOff + app.state.rgptPlan[id].x + dRange);
                rc.bottom = PtToScan(app, yOff - app.state.rgptPlan[id].y + dRange);
                DrawRadarCircle(&dc, &rc);
            }
        }
        for (i = 0; i < app.state.cFleet; i++) {
            lpfl = app.state.rglpfl[i];
            if (!app.state.rglpfl[i])
                break;
            if (lpfl->iPlayer == app.state.idPlayer) {
                dRange = GetFleetScannerRange(app, lpfl, &dPlanRange, nullptr, nullptr);
                if (dPlanRange > 0) {
                    fPlanetScanner |= 2;
                }
                if (dRange > 0) {
                    if (app.lpui->vpctRadarView < 100) {
                        dRange = win32::MulDiv(dRange, app.lpui->vpctRadarView, 100);
                    }
                    rc.left = PtToScan(app, xOff + lpfl->pt.x - dRange);
                    rc.top = PtToScan(app, yOff - lpfl->pt.y - dRange);
                    rc.right = PtToScan(app, xOff + lpfl->pt.x + dRange);
                    rc.bottom = PtToScan(app, yOff - lpfl->pt.y + dRange);
                    DrawRadarCircle(&dc, &rc);
                }
            }
        }
        DrawRadarCircle(&dc, nullptr);
        if (!app.lpui->hbrRadarNear) {
            app.lpui->hbrRadarNear = HbrGet(app, app.lpui->vcScreenColors > 8 ? 24672 : 32639);
        }
        if (!app.lpui->hpenRadarNear) {
            app.lpui->hpenRadarNear = win32::CreatePen(0, 1, app.lpui->vcScreenColors > 8 ? 24672 : 32639);
        }
        win32::SelectObject(hdc, app.lpui->hbrRadarNear);
        win32::SelectObject(hdc, app.lpui->hpenRadarNear);
        if (fPlanetScanner & 1) {
            lppl = app.state.lpPlanets;
            lpplMac = app.state.lpPlanets + app.state.cPlanet;
            for (; lppl < lpplMac; lppl++) {
                if (lppl->iPlayer == app.state.idPlayer) {
                    dRange = GetPlanetScannerRange(app, lppl, &dPlanRange);
                    if (dPlanRange > 0) {
                        dRange >>= 1;
                        if (app.lpui->vpctRadarView < 100) {
                            dRange = win32::MulDiv(dRange, app.lpui->vpctRadarView, 100);
                        }
                        id = lppl->id;
                        rc.left = PtToScan(app, xOff + app.state.rgptPlan[id].x - dRange);
                        rc.top = PtToScan(app, yOff - app.state.rgptPlan[id].y - dRange);
                        rc.right = PtToScan(app, xOff + app.state.rgptPlan[id].x + dRange);
                        rc.bottom = PtToScan(app, yOff - app.state.rgptPlan[id].y + dRange);
                        DrawRadarCircle(&dc, &rc);
                    }
                }
            }
        }
        if (fPlanetScanner & 2) {
            for (i = 0; i < app.state.cFleet; i++) {
                lpfl = app.state.rglpfl[i];
                if (!app.state.rglpfl[i])
                    break;
                if (lpfl->iPlayer == app.state.idPlayer) {
                    dRange = GetFleetScannerRange(app, lpfl, &dPlanRange, nullptr, nullptr);
                    if (dPlanRange > 0) {
                        if (app.lpui->vpctRadarView < 100) {
                            dPlanRange = win32::MulDiv(dPlanRange, app.lpui->vpctRadarView, 100);
                        }
                        rc.left = PtToScan(app, xOff + lpfl->pt.x - dPlanRange);
                        rc.top = PtToScan(app, yOff - lpfl->pt.y - dPlanRange);
                        rc.right = PtToScan(app, xOff + lpfl->pt.x + dPlanRange);
                        rc.bottom = PtToScan(app, yOff - lpfl->pt.y + dPlanRange);
                        DrawRadarCircle(&dc, &rc);
                    }
                }
            }
        }
        if (RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMassAccel) {
            lpth = app.state.lpThings;
            lpthMac = app.state.lpThings + app.state.cThing;
            for (; lpth < lpthMac; lpth++) {
                if (lpth->ith() == ithMineralPacket && lpth->iplr() == app.state.idPlayer && lpth->thp().iWarp() != 0) {
                    dThingRange = lpth->thp().iWarp() + 4;
                    dThingRange *= dThingRange;
                    if (app.lpui->vpctRadarView < 100) {
                        dThingRange = win32::MulDiv(dThingRange, app.lpui->vpctRadarView, 100);
                    }
                    rc.left = PtToScan(app, xOff + lpth->pt().x() - dThingRange);
                    rc.top = PtToScan(app, yOff - lpth->pt().y() - dThingRange);
                    rc.right = PtToScan(app, xOff + lpth->pt().x() + dThingRange);
                    rc.bottom = PtToScan(app, yOff - lpth->pt().y() + dThingRange);
                    DrawRadarCircle(&dc, &rc);
                }
            }
        }
        DrawRadarCircle(&dc, nullptr);
        win32::SelectObject(hdc, app.lpui->hbrRadar);
        win32::SelectObject(hdc, app.lpui->hpenRadar);
        if (mdScanBase != scanViewNoPlayerInfo) {
            id = app.state.sel.grobj == grobjFleet ? app.state.sel.fl.id : idflNone;
            id2 = app.state.sel.scan.grobj == grobjFleet ? app.state.rglpfl[app.state.sel.scan.ifl]->id : idflNone;
            win32::SelectObject(hdcMem, app.lpui->hbmpScanShip);
            win32::SetTextColor(hdc, 0);
            win32::SetBkColor(hdc, 0xffffff);
            for (i = 0; i < app.state.cFleet; i++) {
                lpfl = app.state.rglpfl[i];
                if (!app.state.rglpfl[i])
                    break;
                if (lpfl->idPlanet == idPlanetDeepSpace && (CShipsScanVis(app, lpfl) > 0 || lpfl->id == id || lpfl->id == id2)) {
                    pt = lpfl->pt;
                    if (pt.x >= xMin && pt.x < xMax && pt.y >= yMin && pt.y < yMax) {
                        pt.x = PtToScan(app, xOff + pt.x);
                        pt.y = PtToScan(app, yOff - pt.y);
                        fSelected = lpfl->pt.x == ptSelMain.x && lpfl->pt.y == ptSelMain.y;
                        if (fSelected) {
                            win32::SelectObject(hdcMem, app.lpui->hbmpScanner);
                            win32::BitBlt(hdc, pt.x - 5, pt.y - 5, 11, 11, hdcMem, 11, 80, win32::SRCAND);
                            win32::SelectObject(hdcMem, app.lpui->hbmpScanShip);
                        } else {
                            GetScanFleetOrientation(app, lpfl, &ptO, &ptD);
                            win32::BitBlt(hdc, pt.x - ptD.x / 2, pt.y - ptD.y / 2, ptD.x, ptD.y, hdcMem, ptO.x, ptO.y, win32::SRCAND);
                        }
                    }
                }
            }
            win32::SelectObject(hdcMem, app.lpui->hbmpScanner);
        }
        win32::SelectObject(hdc, hpenSav);
        win32::SelectObject(hdc, hbrSav);
    }
    if (app.lpui->grbitScan & grbitScanMineFields) {
        dc.rgx = rgx;
        dc.rgy = rgy;
        dc.rgrad = rgrad;
        dc.cCur = 0;
        dc.cMax = 250;
        dc.hdc = hdc;
        dc.rcClip = *prc;
        dc.fCovered = 0;
        dc.fHollowOut = 1;
        for (i = 0; i < 3; i++) {
            win32::UnrealizeObject(app.lpui->rghbrPat[i]);
        }
        win32::SetBrushOrgEx(hdc, ptOrigin.x, ptOrigin.y, nullptr);
        win32::IntersectClipRect(hdc, rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);
        hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->rghbrPat[0]));
        hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, win32::GetStockObject(win32::NULL_PEN)));
        ropSav = win32::GetROP2(hdc);
        win32::SetBkColor(hdc, 0);
        for (j = 0; j < 3; j++) {
            if ((j != 0 || (app.lpui->grbitScanMines & 1)) && (j != 1 || (app.lpui->grbitScanMines & 2)) && (j != 2 || (app.lpui->grbitScanMines & 0xc))) {
                for (i = 0; i < 3; i++) {
                    for (fDetonating = 0; fDetonating <= (i == 0); fDetonating++) {
                        win32::SetBrushOrgEx(hdc, ptOrigin.x, ptOrigin.y, nullptr);
                        win32::SelectObject(hdc, app.lpui->rghbrPat[i]);
                        win32::SetTextColor(hdc, !fDetonating ? rgcrScanMine[j] : 16711935);
                        lpth = app.state.lpThings;
                        lpthMac = app.state.lpThings + app.state.cThing;
                        for (; lpth < lpthMac; lpth++) {
                            if (lpth->ith() == ithMinefield && lpth->thm().iType() == i && (j != 0 || lpth->iplr() == app.state.idPlayer) &&
                                (j < 1 || (lpth->iplr() != app.state.idPlayer && (app.state.rgplr[app.state.idPlayer].rgmdRelation[lpth->iplr()] == 1) != (j == 2)))) {
                                if (j == 2 && (app.lpui->grbitScanMines & 0xc) != 0xc) {
                                    if (app.state.rgplr[app.state.idPlayer].rgmdRelation[lpth->iplr()] == 0) {
                                        if (!(app.lpui->grbitScanMines & 4))
                                            continue;
                                    } else if (!(app.lpui->grbitScanMines & 8)) {
                                        continue;
                                    }
                                }
                                if (lpth->thm().fDetonate() == fDetonating) {
                                    pt = lpth->pt();
                                    dRange = LOWORD(static_cast<std::int32_t>(std::sqrt(static_cast<double>(lpth->thm().cMines()))));
                                    rc.left = PtToScan(app, xOff + pt.x - dRange);
                                    rc.top = PtToScan(app, yOff - pt.y - dRange);
                                    rc.right = PtToScan(app, xOff + pt.x + dRange);
                                    rc.bottom = PtToScan(app, yOff - pt.y + dRange);
                                    DrawRadarCircle(&dc, &rc);
                                    for (id = 0; id < app.state.game.cPlanMax && (app.state.rgptPlan[id].x != pt.x || app.state.rgptPlan[id].y != pt.y); id++) {
                                    }
                                    if (id == app.state.game.cPlanMax) {
                                        pt.x = PtToScan(app, xOff + pt.x);
                                        pt.y = PtToScan(app, yOff - pt.y);
                                        dRange = PtToScan(app, 1);
                                        if (dRange < 1) {
                                            dRange = 1;
                                        } else if (dRange > 3) {
                                            dRange = 3;
                                        }
                                        win32::SetRect(&rc, pt.x - dRange, pt.y - dRange, pt.x + dRange + 1, pt.y + dRange + 1);
                                        win32::SetBkColor(hdc, rgcrScanMine[j]);
                                        win32::ExtTextOutA(hdc, rc.left, rc.top, win32::ETO_OPAQUE, &rc, nullptr, 0, nullptr);
                                        win32::SetBkColor(hdc, 0);
                                    }
                                }
                            }
                        }
                        DrawRadarCircle(&dc, nullptr);
                    }
                }
            }
        }
        if (app.state.sel.scan.grobj == grobjThing) {
            lpth = app.state.lpThings + app.state.sel.scan.ith;
            if (lpth->ith() == ithMinefield) {
                win32::SetBrushOrgEx(hdc, ptOrigin.x, ptOrigin.y, nullptr);
                win32::SelectObject(hdc, app.lpui->rghbrPat[lpth->thm().iType()]);
                win32::SetTextColor(hdc, 16776960);
                pt = lpth->pt();
                dRange = LOWORD(static_cast<std::int32_t>(std::sqrt(static_cast<double>(lpth->thm().cMines()))));
                rc.left = PtToScan(app, xOff + pt.x - dRange);
                rc.top = PtToScan(app, yOff - pt.y - dRange);
                rc.right = PtToScan(app, xOff + pt.x + dRange);
                rc.bottom = PtToScan(app, yOff - pt.y + dRange);
                DrawRadarCircle(&dc, &rc);
                DrawRadarCircle(&dc, nullptr);
            }
        }
        win32::SetROP2(hdc, ropSav);
        win32::SelectObject(hdc, hpenSav);
        win32::SelectObject(hdc, hbrSav);
    }
    if (app.state.cThing != 0 && mdScanBase != scanViewNoPlayerInfo) {
        win32::IntersectClipRect(hdc, rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);
        hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrShip));
        hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, app.lpui->hpenDkPurple));
        lpth = app.state.lpThings;
        lpthMac = app.state.lpThings + app.state.cThing;
        for (; lpth < lpthMac; lpth++) {
            switch (lpth->ith()) {
            case ithMineralPacket:
            case ithWormhole:
            case ithMysteryTrader:
                pt = lpth->pt();
                LogicalToScan(app, &pt);
                if (lpth->ith() == ithWormhole && lpth->idFull() < lpth->thw().idPartner() && (1 << app.state.idPlayer & lpth->thw().grbitPlrTrav())) {
                    lpthDest = LpthFromId(app, lpth->thw().idPartner());
                    if (lpthDest) {
                        win32::MoveToEx(hdc, pt.x, pt.y, nullptr);
                        pt2 = lpthDest->pt();
                        LogicalToScan(app, &pt2);
                        win32::LineTo(hdc, pt2.x, pt2.y);
                        win32::LineTo(hdc, pt2.x, pt2.y + 1);
                    }
                }
                if (lpth->ith() == ithWormhole) {
                    win32::BitBlt(hdc, pt.x - 4, pt.y - 4, 9, 9, hdcMem, 9, 92, win32::SRCAND);
                    win32::BitBlt(hdc, pt.x - 4, pt.y - 4, 9, 9, hdcMem, 0, 92, win32::SRCPAINT);
                } else if (lpth->ith() == ithMysteryTrader) {
                    hbmpTrSav = static_cast<HBITMAP>(win32::SelectObject(hdcMem, app.lpui->hbmpScanShip));
                    crFore = win32::SetTextColor(hdc, 16776960);
                    crBack = win32::SetBkColor(hdc, 0);
                    GetDxDyOrientation(app, lpth->tht().ptDest().x() - lpth->pt().x(), lpth->tht().ptDest().y() - lpth->pt().y(), &ptO, &ptD);
                    win32::BitBlt(hdc, pt.x - ptD.x / 2, pt.y - ptD.y / 2, ptD.x, ptD.y, hdcMem, ptO.x, ptO.y, win32::SRCPAINT);
                    win32::SetTextColor(hdc, crFore);
                    win32::SetBkColor(hdc, crBack);
                    win32::SelectObject(hdcMem, hbmpTrSav);
                } else {
                    dRange = app.state.iScanZoom <= zoom100 ? 2 : app.state.iScanZoom > zoom150 ? 5 : 3;
                    dx = dRange * 2 + 1;
                    if (lpth->thp().iWarp() == 0) {
                        win32::SelectObject(hdc, app.lpui->hpenYellow);
                        win32::MoveToEx(hdc, pt.x, pt.y - dRange - 1, nullptr);
                        win32::LineTo(hdc, pt.x - dRange - 1, pt.y);
                        win32::LineTo(hdc, pt.x, pt.y + dRange + 1);
                        win32::LineTo(hdc, pt.x + dRange + 1, pt.y);
                        win32::LineTo(hdc, pt.x, pt.y - dRange - 1);
                        win32::SelectObject(hdc, app.lpui->hpenDkPurple);
                    } else {
                        if (lpth->iplr() != app.state.idPlayer) {
                            win32::SelectObject(hdc, app.lpui->hbrRed);
                        }
                        win32::PatBlt(hdc, pt.x - dRange, pt.y - dRange, dx, 1, win32::PATCOPY);
                        win32::PatBlt(hdc, pt.x - dRange, pt.y - dRange, 1, dx, win32::PATCOPY);
                        win32::PatBlt(hdc, pt.x - dRange, pt.y + dRange, dx, 1, win32::PATCOPY);
                        win32::PatBlt(hdc, pt.x + dRange, pt.y - dRange, 1, dx, win32::PATCOPY);
                        if (lpth->iplr() != app.state.idPlayer) {
                            win32::SelectObject(hdc, app.lpui->hbrShip);
                        }
                    }
                }
            }
        }
        win32::SelectObject(hdc, hbrSav);
        win32::SelectObject(hdc, hpenSav);
    }
    if ((app.lpui->grbitScan & grbitScanFleetPaths) && mdScanBase != scanViewNoPlayerInfo) {
        id = app.state.sel.grobj == grobjFleet ? app.state.sel.fl.id : idflNone;
        id2 = app.state.sel.scan.grobj == grobjFleet ? app.state.rglpfl[app.state.sel.scan.ifl]->id : idflNone;
        hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, app.lpui->hpenStarbase));
        for (i = 0; i < app.state.cFleet; i++) {
            lpfl = app.state.rglpfl[i];
            if (!app.state.rglpfl[i])
                break;
            if (!lpfl->fDead && lpfl->det >= detAll && lpfl->cord > 1 && (CShipsScanVis(app, lpfl) > 0 || lpfl->id == id || lpfl->id == id2)) {
                for (iord = 0; iord < lpfl->cord; iord++) {
                    pt = lpfl->lpplord->rgord()[iord].pt;
                    pt.x = PtToScan(app, xOff + pt.x);
                    pt.y = PtToScan(app, yOff - pt.y);
                    if (iord == 0) {
                        win32::MoveToEx(hdc, pt.x, pt.y, nullptr);
                    } else {
                        win32::LineTo(hdc, pt.x, pt.y);
                    }
                }
            }
        }
        win32::SelectObject(hdc, hpenSav);
    }
    win32::SelectClipRgn(hdc, app.lpui->hrgnHuge);
    ControlIO().pfnGetClientRect(app.lpui->hwndScanner, &rc);
    win32::ExcludeClipRect(hdc, 0, rc.bottom - app.lpui->dySBar, rc.right, rc.bottom);
    std::ranges::fill(rgWhatsHere, 0);
    switch (app.state.iScanZoom) {
    default:
        break;
    case zoom400:
        win32::SelectObject(hdc, app.lpui->rghfontArial10[1]);
        break;
    case zoom200:
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        break;
    case zoom100:
    case zoom125:
    case zoom150:
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        break;
    case zoom75:
        win32::SelectObject(hdc, app.lpui->rghfontArial6[0]);
    }
    crFore = win32::SetTextColor(hdc, 0xffffff);
    crBack = win32::SetBkColor(hdc, 0);
    iBkPrev = win32::SetBkMode(hdc, win32::TRANSPARENT);
    j = app.state.iScanZoom >= zoom200 && mdScanBase == scanViewPopulation ? 11 : 0;
    for (i = 0; i < app.state.game.cPlanMax; i++) {
        if (app.state.rgptPlan[i].x >= xMin && app.state.rgptPlan[i].x < xMax && app.state.rgptPlan[i].y >= yMin && app.state.rgptPlan[i].y < yMax) {
            fDoDraw = 1;

        LBailIn:
            pt.x = PtToScan(app, xOff + app.state.rgptPlan[i].x);
            pt.y = PtToScan(app, yOff - app.state.rgptPlan[i].y);
            if (fDoDraw) {
                if (ptSelMain.x == app.state.rgptPlan[i].x && ptSelMain.y == app.state.rgptPlan[i].y && mdScanBase <= scanViewMineralConc) {
                    win32::BitBlt(hdc, pt.x - 5, pt.y - 5, 11, 11, hdcMem, 0, 69, win32::SRCAND);
                    win32::BitBlt(hdc, pt.x - 5, pt.y - 5, 11, 11, hdcMem, 0, 33, win32::SRCPAINT);
                } else {
                    win32::BitBlt(hdc, pt.x - 1, pt.y - 1, 3, 3, hdcMem, 11, 15, win32::SRCCOPY);
                }
            }
            if ((app.lpui->grbitScan & grbitScanPlanetNames) && app.state.iScanZoom >= zoom75 && app.state.rgptPlan[i].x >= xMin - 50 && app.state.rgptPlan[i].x < xMax + 50 &&
                app.state.rgptPlan[i].y >= yMin - 20 && app.state.rgptPlan[i].y < yMax + 20) {
                szObjectName = TextIO().pfnPlanetName(app, i);
                CchCopyUtf8Prefix(std::span{app.state.szWork}, szObjectName.data());
                if (app.lpui->grbitScan & grbitScanPlayerColors) {
                    lppl = LpplFromId(app, i);
                    if (lppl && lppl->iPlayer != iplrNone) {
                        win32::SetTextColor(hdc, lppl->iPlayer == app.state.idPlayer ? 0xffffff : app.lpui->rgcrPlrHistory[lppl->iPlayer]);
                    }
                }
                CtrTextOut(hdc, pt.x, pt.y + 5 + j, app.state.szWork, 0);
                if (app.lpui->grbitScan & grbitScanPlayerColors) {
                    win32::SetTextColor(hdc, 0xffffff);
                }
            }
        } else if (app.lpui->grbitScan & grbitScanPlanetNames) {
            fDoDraw = 0;
            goto LBailIn;
        }
    }
    win32::SetBkMode(hdc, iBkPrev);
    win32::SetBkColor(hdc, crBack);
    win32::SetTextColor(hdc, crFore);
    j = app.state.iScanZoom >= zoom200 && mdScanBase == scanViewPopulation ? 11 : 0;
    if (app.state.sel.grobj != grobjNone && app.state.sel.pt.x == app.state.sel.scan.pt.x && app.state.sel.pt.y == app.state.sel.scan.pt.y) {
        pt = app.state.sel.pt;
        LogicalToScan(app, &pt);
        win32::BitBlt(hdc, pt.x - 5, pt.y + 11 + j, 11, 12, hdcMem, 0, 80, win32::SRCAND);
        win32::BitBlt(hdc, pt.x - 5, pt.y + 11 + j, 11, 12, hdcMem, 0, 57, win32::SRCPAINT);
    } else if (app.state.sel.scan.grobj != grobjNone) {
        pt = app.state.sel.scan.pt;
        LogicalToScan(app, &pt);
        win32::BitBlt(hdc, pt.x - 3, pt.y + 7 + j, 7, 8, hdcMem, 22, 80, win32::SRCAND);
        win32::BitBlt(hdc, pt.x - 3, pt.y + 7 + j, 7, 8, hdcMem, 22, 49, win32::SRCPAINT);
    }
    if (app.state.cPlanet != 0 && mdScanBase != scanViewNoPlayerInfo) {
        lppl = app.state.lpPlanets;
        i = 0;
        while (i < app.state.cPlanet) {
            id = lppl->id;
            fStarbase = lppl->fStarbase != 0 && lppl->iPlayer != iplrNone;
            if (fStarbase) {
                if (app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].hul.ihuldef == ihuldefOrbitalFort) {
                    fStarbase = 2;
                }
                fMA = ColonyIO().pfnWarp(app, lppl, nullptr) > 0;
                fStargate = IStargateFromLppl(app, lppl) != -1;
            } else {
                fMA = 0;
                fStargate = 0;
            }
            if (app.state.rgptPlan[id].x >= xMin && app.state.rgptPlan[id].x < xMax && app.state.rgptPlan[id].y >= yMin && app.state.rgptPlan[id].y < yMax) {
                pt.x = PtToScan(app, xOff + app.state.rgptPlan[id].x);
                pt.y = PtToScan(app, yOff - app.state.rgptPlan[id].y);
                switch (mdScanBase) {
                case scanViewPlanetValue:
                    fTerra = 0;
                    if (lppl->det < detSome)
                        break;
                    pctDesire = PlanetQueryIO().pfnDesirability(app, lppl, app.state.idPlayer);
                    if (pctDesire < 0 || RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raTerra) {
                        pctDesire = PctPlanetOptValue(app, lppl, app.state.idPlayer);
                        if (pctDesire >= 0 && RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) != raTerra) {
                            fTerra = 1;
                        }
                    }
                    hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, pctDesire < 0 ? app.lpui->hbrRadar : fTerra ? app.lpui->hbrDkYellow : app.lpui->hbrGreen));
                    hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, pctDesire < 0 ? app.lpui->hpenRadar : fTerra ? app.lpui->hpenDkYellow : app.lpui->hpenDkGreen));
                    if (pctDesire >= 0) {
                        dRad = pctDesire / 11 + 2;
                    } else {
                        dRad = static_cast<std::int16_t>(-pctDesire) / 5 + 2;
                    }
                    if (dRad > 10) {
                        dRad = 10;
                    }
                    win32::Ellipse(hdc, pt.x - dRad, pt.y - dRad, pt.x + dRad + 1, pt.y + dRad + 1);
                    dRad -= 2;
                    if (dRad < 3) {
                        dRad++;
                    }
                    if (dRad < 1) {
                        dRad = 1;
                    }
                    win32::SelectObject(hdc, pctDesire < 0 ? app.lpui->hbrEnemy : fTerra ? app.lpui->hbrYellow : app.lpui->hbrShip);
                    win32::SelectObject(hdc, pctDesire < 0 ? app.lpui->hpenEnemy : fTerra ? app.lpui->hpenYellow : app.lpui->hpenShip);
                    win32::Ellipse(hdc, pt.x - dRad, pt.y - dRad, pt.x + dRad + 1, pt.y + dRad + 1);
                    if (lppl->iPlayer != iplrNone) {
                        rc.left = pt.x - 1;
                        rc.right = rc.left + 9;
                        rc.top = pt.y - 20;
                        rc.bottom = rc.top + 8;
                        win32::FillRect(hdc, &rc, static_cast<HBRUSH>(win32::GetStockObject(win32::BLACK_BRUSH)));
                        if (lppl->iPlayer == app.state.idPlayer) {
                            hbr = app.lpui->hbrBBlue;
                        } else if (app.state.rgplr[app.state.idPlayer].rgmdRelation[lppl->iPlayer] == 1) {
                            hbr = app.lpui->hbrStarbase;
                        } else if (app.state.rgplr[app.state.idPlayer].rgmdRelation[lppl->iPlayer] == 0) {
                            hbr = app.lpui->hbrRadar;
                        } else {
                            hbr = app.lpui->hbrEnemy;
                        }
                        win32::SelectObject(hdc, hbr);
                        win32::PatBlt(hdc, pt.x, pt.y - 19, 1, 21, win32::PATCOPY);
                        win32::PatBlt(hdc, pt.x, pt.y - 19, 7, 6, win32::PATCOPY);
                    }
                    win32::SelectObject(hdc, hpenSav);
                    win32::SelectObject(hdc, hbrSav);
                    break;
                case scanViewSurfaceMinerals:
                case scanViewMineralConc:
                    fConc = mdScanBase == scanViewMineralConc;
                    iOff = app.state.iScanZoom < zoom100;
                    if (lppl->det < detMore && (!fConc || lppl->det < detSome))
                        goto LNormalScannerMode;
                    xOut = pt.x - app.lpui->vrgScanPO[iOff][0];
                    yOut = pt.y - app.lpui->vrgScanPO[iOff][1];
                    hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrButtonFace));
                    win32::PatBlt(hdc, xOut - 2, yOut, app.lpui->vrgScanPO[iOff][2], 1, win32::PATCOPY);
                    win32::PatBlt(hdc, xOut - 2, yOut - app.lpui->vrgScanPO[iOff][2] + 1, 1, app.lpui->vrgScanPO[iOff][2], win32::PATCOPY);
                    for (j = 0; j < 3; j++) {
                        if (fConc) {
                            l = static_cast<std::int16_t>((static_cast<std::int16_t>(lppl->rgMinConc[j]) / 5));
                            if (l > 20) {
                                l = 20;
                            }
                        } else {
                            l = lppl->rgwtMin[j];
                            l = static_cast<std::int32_t>(((static_cast<std::int16_t>((app.lpui->cMinGrafMax / 40)) + l) / static_cast<std::int16_t>((app.lpui->cMinGrafMax / 20))));
                            if (l > 20) {
                                l = 20;
                            }
                        }
                        if (iOff != 0) {
                            l = static_cast<std::int32_t>((l / 2));
                        }
                        if (l > 0) {
                            win32::SelectObject(hdc, app.lpui->rghbrMineral[j]);
                            win32::PatBlt(hdc, xOut, yOut - LOWORD(l), app.lpui->vrgScanPO[iOff][3], LOWORD(l), win32::PATCOPY);
                        }
                        xOut += app.lpui->vrgScanPO[iOff][4];
                    }
                    win32::SelectObject(hdc, hbrSav);
                    goto LNormalScannerMode;
                case scanViewPopulation:
                    fTerra = 0;
                    if (lppl->det >= detSome && lppl->iPlayer != iplrNone) {
                        if (lppl->iPlayer == app.state.idPlayer) {
                            lPop = lppl->rgwtMin[3];
                        } else {
                            lPop = static_cast<std::int32_t>((lppl->uPopGuess * 4));
                        }
                        for (dRad = 0; dRad < 19 && vrgPopRad[dRad] <= lPop; dRad++) {
                        }
                        dRad += 2;
                        if (lppl->iPlayer == app.state.idPlayer) {
                            iRel = 0;
                        } else if (app.state.rgplr[app.state.idPlayer].rgmdRelation[lppl->iPlayer] == 1) {
                            iRel = 1;
                        } else {
                            iRel = 3;
                        }
                        hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, iRel == 0 ? app.lpui->hbrShip : iRel == 3 ? app.lpui->hbrEnemy : app.lpui->hbrYellow));
                        hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, iRel == 0 ? app.lpui->hpenDkGreen : iRel == 3 ? app.lpui->hpenEnemy : app.lpui->hpenDkYellow));
                        if (app.state.iScanZoom < zoom200) {
                            dRad = (dRad + 1) >> 1;
                        }
                        win32::Ellipse(hdc, pt.x - dRad, pt.y - dRad, pt.x + dRad + 1, pt.y + dRad + 1);
                        win32::SelectObject(hdc, hpenSav);
                        win32::SelectObject(hdc, hbrSav);
                        break;
                    }
                    if (lppl->iPlayer != iplrNone)
                        break;
                    /* fallthrough */
                default:
                LNormalScannerMode:
                    if (lppl->iPlayer == iplrNone) {
                        if (app.state.rgptPlan[id].x != ptSelMain.x || app.state.rgptPlan[id].y != ptSelMain.y) {
                            win32::BitBlt(hdc, pt.x - 1, pt.y - 1, 3, 3, hdcMem, 11, 18, win32::SRCCOPY);
                        }
                    } else if (app.state.rgptPlan[id].x == ptSelMain.x && app.state.rgptPlan[id].y == ptSelMain.y) {
                        if (lppl->iPlayer == app.state.idPlayer) {
                            yBmp = 0;
                        } else if (app.state.rgplr[app.state.idPlayer].rgmdRelation[lppl->iPlayer] == 1) {
                            yBmp = 22;
                        } else {
                            yBmp = 11;
                        }
                        win32::BitBlt(hdc, pt.x - 5, pt.y - 5, 11, 11, hdcMem, 0, 69, win32::SRCAND);
                        win32::BitBlt(hdc, pt.x - 5, pt.y - 5, 11, 11, hdcMem, 0, yBmp, win32::SRCPAINT);
                        if (fStarbase) {
                            hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, fStarbase == 2 ? app.lpui->hbrBlue : app.lpui->hbrYellow));
                            win32::PatBlt(hdc, pt.x + 4, pt.y - 6, 5, 5, win32::BLACKNESS);
                            win32::PatBlt(hdc, pt.x + 5, pt.y - 6, 3, 5, win32::PATCOPY);
                            win32::PatBlt(hdc, pt.x + 4, pt.y - 5, 5, 3, win32::PATCOPY);
                            win32::SelectObject(hdc, hbrSav);
                        }
                        if (fStargate) {
                            hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrGreen));
                            win32::PatBlt(hdc, pt.x - 7, pt.y - 6, 5, 5, win32::BLACKNESS);
                            win32::PatBlt(hdc, pt.x - 6, pt.y - 6, 3, 5, win32::PATCOPY);
                            win32::PatBlt(hdc, pt.x - 7, pt.y - 5, 5, 3, win32::PATCOPY);
                            win32::SelectObject(hdc, hbrSav);
                        }
                        if (fMA) {
                            hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrPurple));
                            win32::PatBlt(hdc, pt.x - 2, pt.y - 9, 5, 5, win32::BLACKNESS);
                            win32::PatBlt(hdc, pt.x - 1, pt.y - 9, 3, 5, win32::PATCOPY);
                            win32::PatBlt(hdc, pt.x - 2, pt.y - 8, 5, 3, win32::PATCOPY);
                            win32::SelectObject(hdc, hbrSav);
                        }
                    } else {
                        if (lppl->iPlayer == app.state.idPlayer) {
                            yBmp = 0;
                        } else if (app.state.rgplr[app.state.idPlayer].rgmdRelation[lppl->iPlayer] == 1) {
                            yBmp = 10;
                        } else {
                            yBmp = 5;
                        }
                        win32::BitBlt(hdc, pt.x - 2, pt.y - 2, 5, 5, hdcMem, 11, yBmp, win32::SRCCOPY);
                        if (fStarbase) {
                            hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, fStarbase == 2 ? app.lpui->hbrBlue : app.lpui->hbrYellow));
                            win32::PatBlt(hdc, pt.x + 3, pt.y - 4, 3, 3, win32::PATCOPY);
                            win32::SelectObject(hdc, hbrSav);
                        }
                        if (fStargate) {
                            hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrGreen));
                            win32::PatBlt(hdc, pt.x - 5, pt.y - 4, 3, 3, win32::BLACKNESS);
                            win32::PatBlt(hdc, pt.x - 4, pt.y - 4, 1, 3, win32::PATCOPY);
                            win32::PatBlt(hdc, pt.x - 5, pt.y - 3, 3, 1, win32::PATCOPY);
                            win32::SelectObject(hdc, hbrSav);
                        }
                        if (fMA) {
                            hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrPurple));
                            win32::PatBlt(hdc, pt.x - 1, pt.y - 6, 3, 3, win32::BLACKNESS);
                            win32::PatBlt(hdc, pt.x, pt.y - 6, 1, 3, win32::PATCOPY);
                            win32::PatBlt(hdc, pt.x - 1, pt.y - 5, 3, 1, win32::PATCOPY);
                            win32::SelectObject(hdc, hbrSav);
                        }
                    }
                }
            }
            i++;
            lppl++;
        }
    }
    if (app.state.cFleet != 0 && mdScanBase != scanViewNoPlayerInfo) {
        lpflT = *app.state.rglpfl;
        hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrShip));
        id = app.state.sel.grobj == grobjFleet ? app.state.sel.fl.id : idflNone;
        id2 = app.state.sel.scan.grobj == grobjFleet ? app.state.rglpfl[app.state.sel.scan.ifl]->id : idflNone;
        for (i = 0; i < app.state.cFleet; i++) {
            lpflT = app.state.rglpfl[i];
            if (!app.state.rglpfl[i])
                break;
            if (CShipsScanVis(app, lpflT) > 0 || lpflT->id == id || lpflT->id == id2) {
                idP = lpflT->idPlanet;
                if (idP != idPlanetDeepSpace) {
                    pt = app.state.rgptPlan[idP];
                } else {
                    pt = lpflT->pt;
                }
                if (pt.x >= xMin && pt.x < xMax && pt.y >= yMin && pt.y < yMax) {
                    pt.x = PtToScan(app, xOff + pt.x);
                    pt.y = PtToScan(app, yOff - pt.y);
                    yBmp = lpflT->iPlayer != app.state.idPlayer;
                    fSelected = lpflT->pt.x == ptSelMain.x && lpflT->pt.y == ptSelMain.y;
                    if (idP != idPlanetDeepSpace) {
                        if (rgWhatsHere[idP] != 3 && rgWhatsHere[idP] != yBmp + 1 && mdScanBase <= scanViewMineralConc) {
                            rgWhatsHere[idP] += yBmp + 1;
                            yBmp = rgWhatsHere[idP] - 1;
                            if (fSelected) {
                                win32::BitBlt(hdc, pt.x - 9, pt.y - 9, 19, 19, hdcMem, 29, 69, win32::SRCAND);
                                win32::BitBlt(hdc, pt.x - 9, pt.y - 9, 19, 19, hdcMem, 29, 19 * yBmp, win32::SRCPAINT);
                            } else {
                                win32::BitBlt(hdc, pt.x - 5, pt.y - 5, 11, 11, hdcMem, 16, 69, win32::SRCAND);
                                win32::BitBlt(hdc, pt.x - 5, pt.y - 5, 11, 11, hdcMem, 16, 11 * yBmp, win32::SRCPAINT);
                            }
                            if ((app.lpui->grbitScan & grbitScanShipCounts) && !lpflT->fDone) {
                                DrawScanFleetCount(app, lpflT, pt.x, pt.y - (!fSelected ? 5 : 9) - 2, hdc, hdcMem);
                            }
                        }
                    } else if (fSelected) {
                        win32::BitBlt(hdc, pt.x - 5, pt.y - 5, 11, 11, hdcMem, 11, 11 * yBmp + 36, win32::SRCPAINT);
                        if ((app.lpui->grbitScan & grbitScanShipCounts) && !lpflT->fDone) {
                            DrawScanFleetCount(app, lpflT, pt.x, pt.y - 7, hdc, hdcMem);
                        }
                    } else {
                        win32::SelectObject(hdcMem, app.lpui->hbmpScanShip);
                        if (yBmp == 0) {
                            cr = 16711680;
                        } else if (app.state.rgplr[app.state.idPlayer].rgmdRelation[lpflT->iPlayer] == 1) {
                            cr = 0xffff;
                        } else {
                            cr = 0xff;
                        }
                        win32::SetTextColor(hdc, cr);
                        win32::SetBkColor(hdc, 0);
                        GetScanFleetOrientation(app, lpflT, &ptO, &ptD);
                        win32::BitBlt(hdc, pt.x - ptD.x / 2, pt.y - ptD.y / 2, ptD.x, ptD.y, hdcMem, ptO.x, ptO.y, win32::SRCPAINT);
                        if ((app.lpui->grbitScan & grbitScanShipCounts) && !lpflT->fDone) {
                            DrawScanFleetCount(app, lpflT, pt.x, pt.y - ptD.y / 2 - 2, hdc, hdcMem);
                        }
                        win32::SelectObject(hdcMem, app.lpui->hbmpScanner);
                    }
                }
            }
        }
        win32::SelectObject(hdc, hbrSav);
    }
    ExpandRc(prc, -dExpand, -dExpand);
    prc->bottom -= 14;
    crFore = win32::SetTextColor(hdc, 0);
    crBack = win32::GetBkColor(hdc);
    if (app.state.sel.grobj == grobjFleet || app.state.sel.scan.grobj == grobjFleet || app.state.sel.scan.grobj == grobjThing || (app.state.sel.grobj == grobjPlanet && app.state.sel.pl.fStarbase)) {
        win32::IntersectClipRect(hdc, prc->left, prc->top, prc->right, prc->bottom);
        app.lpui->fOrdersVis = 0;
        DrawShipScanPath(app, hdc, 1);
        win32::SelectClipRgn(hdc, app.lpui->hrgnHuge);
    }
    win32::SetBkColor(hdc, crBack);
    win32::SetTextColor(hdc, crFore);
    win32::SelectObject(hdcMem, hbmpSav);
    win32::DeleteDC(hdcMem);
    if (hdcScreen) {
        win32::SetWindowOrgEx(hdc, 0, 0, nullptr);
        win32::BitBlt(hdcScreen, prc->left, prc->top, prc->right - prc->left, prc->bottom - prc->top, hdc, prc->left & 7, prc->top & 7, win32::SRCCOPY);
        win32::SelectObject(hdc, hbmpXSav);
        win32::DeleteObject(hbmpScreen);
        win32::DeleteDC(hdc);
    }
    return 0;
}

void DrawScanFleetCount(APPSESSION &app, FLEET *lpfl, std::int16_t x, std::int16_t y, HDC hdc, HDC hdcMem) {
    std::int32_t  l2;
    std::int16_t  f999;
    COLORREF cr;
    FLEET   *lpflWalk;
    std::int16_t  iPlr;
    HBITMAP  hbmpSav;
    std::int32_t  l;

    lpflWalk = lpfl;
    iPlr = !(app.lpui->grbitScan & grbitScanPlayerColors) ? -2 : -1;
    l = 0;
    f999 = 0;
    do {
        l2 = CShipsScanVis(app, lpflWalk);
        if (l2 > 0) {
            l += l2;
            if (lpflWalk->iPlayer != iPlr && iPlr != -2) {
                if (iPlr == iplrNone) {
                    iPlr = lpflWalk->iPlayer;
                } else {
                    iPlr = -2;
                }
            }
        }
        lpflWalk->fDone = 1;
        lpflWalk = lpflWalk->lpflNext;
    } while (lpflWalk != lpfl);
    if (l > 999) {
        l = 999;
    } else if (l <= 0) {
        return;
    }
    hbmpSav = static_cast<HBITMAP>(win32::SelectObject(hdcMem, app.lpui->hbmpNumbers));
    win32::SetBkColor(hdc, 0);
    win32::SetTextColor(hdc, 0xffffff);
    if (iPlr >= 0 && iPlr != app.state.idPlayer) {
        cr = app.lpui->rgcrPlrHistory[iPlr];
    } else {
        cr = 0xffffff;
    }
    x--;
    y -= 7;
    if (l > 99) {
        f999 = 1;
        x -= 5;
        if (cr != 0xffffff) {
            win32::BitBlt(hdc, x, y, 4, 7, hdcMem, static_cast<std::int16_t>(LOWORD(l)) / 100 * 4, 0, 2229030);
            win32::SetTextColor(hdc, cr);
        }
        win32::BitBlt(hdc, x, y, 4, 7, hdcMem, static_cast<std::int16_t>(LOWORD(l)) / 100 * 4, 0, win32::SRCPAINT);
        if (cr != 0xffffff) {
            win32::SetTextColor(hdc, 0xffffff);
        }
        x += 8;
        l = static_cast<std::int32_t>((l % 100));
    }
    if (l > 9 || f999) {
        x -= 3;
        if (cr != 0xffffff) {
            win32::BitBlt(hdc, x, y, 4, 7, hdcMem, static_cast<std::int16_t>(LOWORD(l)) / 10 * 4, 0, 2229030);
            win32::SetTextColor(hdc, cr);
        }
        win32::BitBlt(hdc, x, y, 4, 7, hdcMem, static_cast<std::int16_t>(LOWORD(l)) / 10 * 4, 0, win32::SRCPAINT);
        if (cr != 0xffffff) {
            win32::SetTextColor(hdc, 0xffffff);
        }
        x += 5;
        l = static_cast<std::int32_t>((l % 10));
    }
    if (cr != 0xffffff) {
        win32::BitBlt(hdc, x, y, 4, 7, hdcMem, LOWORD(l) * 4, 0, 2229030);
        win32::SetTextColor(hdc, cr);
    }
    win32::BitBlt(hdc, x, y, 4, 7, hdcMem, LOWORD(l) * 4, 0, win32::SRCPAINT);
    if (cr != 0xffffff) {
        win32::SetTextColor(hdc, 0xffffff);
    }
    win32::SelectObject(hdcMem, hbmpSav);
    return;
}

std::int32_t CShipsScanVis(APPSESSION &app, FLEET *lpfl) {
    std::int16_t  j;
    std::int32_t  csh;
    std::int16_t  k;
    std::uint16_t grbitSh;

    csh = 0;
    if (app.lpui->grbitScan & grbitScanIdleFleets) {
        if (lpfl->iPlayer == app.state.idPlayer &&
            (lpfl->cord != 1 || lpfl->lpplord->rgord()[0].grTask == grTaskLayMines || lpfl->lpplord->rgord()[0].grTask == grTaskScrap ||
             lpfl->lpplord->rgord()[0].grTask == grTaskMine || lpfl->lpplord->rgord()[0].grTask == grTaskPatrol)) {
            return 0;
        }
        if (lpfl->iPlayer != app.state.idPlayer && lpfl->iwarpFlt() == 0) {
            return 0;
        }
    }
    if ((app.lpui->grbitScan & grbitScanDesignFilter) && lpfl->iPlayer == app.state.idPlayer) {
        grbitSh = app.lpui->grbitScanShip;
        j = 0;
        for (; grbitSh != 0; grbitSh >>= 1) {
            if ((grbitSh & 1) && lpfl->rgcsh[j] > 0) {
                csh += lpfl->rgcsh[j];
            }
            j++;
        }
    } else if ((app.lpui->grbitScan & grbitScanEnemyFilter) && lpfl->iPlayer != app.state.idPlayer) {
        grbitSh = app.lpui->grbitScanEShip;
        j = 0;
        for (; grbitSh != 0; grbitSh >>= 1) {
            if (grbitSh & 1) {
                for (k = 0; k < 16; k++) {
                    if (lpfl->rgcsh[k] > 0 && PartQueryIO().pfnHull(app.state.rglpshdef[lpfl->iPlayer][k].hul.ihuldef)->imdCategory == j) {
                        csh += lpfl->rgcsh[k];
                    }
                }
            }
            j++;
        }
    } else {
        for (j = 0; j < 16; j++) {
            csh += lpfl->rgcsh[j];
        }
    }
    return csh;
}

void DrawRadarCircle(DRAWCIR *pdc, RECT *prc) {
    std::int16_t  y2;
    std::int32_t  r2;
    COLORREF crSav;
    std::int16_t  dy;
    std::int16_t  y;
    std::int16_t  iFree;
    std::int16_t  i;
    std::int16_t  dx;
    std::int16_t  x2;
    std::int16_t  rad;
    std::int32_t  l;
    std::int16_t  x;
    RECT     rc;

    if (!prc || (!pdc->fCovered && win32::IntersectRect(&rc, prc, &pdc->rcClip) != 0)) {
        if (prc) {
            rad = (prc->right - prc->left) >> 1;
            x = prc->left + rad;
            y = prc->top + rad;
            r2 = static_cast<std::uint32_t>((rad * rad));
            for (i = 0; i < 4; i++) {
                x2 = i >= 2 ? pdc->rcClip.right : pdc->rcClip.left;
                y2 = !(i & 1) ? pdc->rcClip.bottom : pdc->rcClip.top;
                dx = x2 - x;
                dy = y2 - y;
                if (dx > r2 || dy > r2 || static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy)) > r2)
                    break;
            }
            if (i == 4) {
                pdc->fCovered = 1;
                pdc->cCur = 0;
            DrawEllipse:
                if (pdc->fHollowOut) {
                    win32::SetROP2(pdc->hdc, win32::R2_MASKPEN);
                    crSav = win32::SetBkColor(pdc->hdc, 0xffffff);
                    win32::Ellipse(pdc->hdc, prc->left, prc->top, prc->right, prc->bottom);
                    win32::SetBkColor(pdc->hdc, crSav);
                    win32::SetROP2(pdc->hdc, win32::R2_MERGEPEN);
                }
                win32::Ellipse(pdc->hdc, prc->left, prc->top, prc->right, prc->bottom);
                return;
            } else if (pdc->cCur == pdc->cMax) {
                goto DrawEllipse;
            } else {
                iFree = pdc->cCur;
                for (i = 0; i < pdc->cCur; i++) {
                    if (pdc->rgrad[i] <= 0) {
                        iFree = i;
                    } else {
                        x2 = pdc->rgx[i];
                        y2 = pdc->rgy[i];
                        dx = x - x2;
                        dy = y - y2;
                        l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                        if (pdc->rgrad[i] < rad) {
                            if (static_cast<double>(std::sqrt(static_cast<double>(l))) + pdc->rgrad[i] <= static_cast<double>(rad)) {
                                pdc->rgrad[i] = 0;
                                iFree = i;
                            }
                        } else if (pdc->rgrad[i] > rad) {
                            if (static_cast<double>(std::sqrt(static_cast<double>(l))) + rad <= static_cast<double>(pdc->rgrad[i])) {
                                return;
                            }
                        } else if (x2 == x && y2 == y) {
                            return;
                        }
                    }
                }
                pdc->rgx[iFree] = x;
                pdc->rgy[iFree] = y;
                pdc->rgrad[iFree] = rad;
                if (iFree != pdc->cCur) {
                    return;
                }
                pdc->cCur++;
                return;
            }
        } else {
            if (pdc->fHollowOut) {
                win32::SetROP2(pdc->hdc, win32::R2_MASKPEN);
                crSav = win32::SetBkColor(pdc->hdc, 0xffffff);
                l = 1;
            }
        L2ndEl:
            for (i = 0; i < pdc->cCur; i++) {
                if (pdc->rgrad[i] > 0) {
                    x = pdc->rgx[i];
                    y = pdc->rgy[i];
                    rad = pdc->rgrad[i];
                    win32::Ellipse(pdc->hdc, x - rad, y - rad, x + rad + 1, y + rad + 1);
                }
            }
            if (pdc->fHollowOut && l == 1) {
                l = 0;
                win32::SetBkColor(pdc->hdc, crSav);
                win32::SetROP2(pdc->hdc, win32::R2_MERGEPEN);
                goto L2ndEl;
            }

            pdc->fCovered = 0;
            pdc->cCur = 0;
        }
    }
    return;
}

// DrawPathYearTicks draws a tick across the selected fleet's leg from
// ptFrom to ptTo, logical points, where each year at iWarp ends: a fleet
// moves iWarp squared light years a year and stops at each waypoint. Ticks
// closer than 6 pixels are left out. It leaves the pen at ptTo. The path is
// drawn with R2_XORPEN, so drawing it again erases the ticks too; each tick
// is two halves that skip the path's own pixel, which XOR would clear.
static void DrawPathYearTicks(APPSESSION &app, HDC hdc, POINT16 ptFrom, POINT16 ptTo, std::int16_t iWarp) {
    POINT16 ptA;
    POINT16 ptB;
    POINT16 pt;
    double  dLeg;
    double  dScan;
    double  dxTick;
    double  dyTick;
    std::int16_t dYear;
    std::int16_t i;
    std::int16_t iSide;

    ptA = ptFrom;
    LogicalToScan(app, &ptA);
    ptB = ptTo;
    LogicalToScan(app, &ptB);
    if (iWarp >= 1 && iWarp <= 10) {
        dYear = iWarp * iWarp;
        dLeg = std::hypot(ptTo.x - ptFrom.x, ptTo.y - ptFrom.y);
        dScan = std::hypot(ptB.x - ptA.x, ptB.y - ptA.y);
        if (dLeg > dYear && dScan * dYear / dLeg >= 6) {
            // A unit step across the leg on screen.
            dxTick = -(ptB.y - ptA.y) / dScan;
            dyTick = (ptB.x - ptA.x) / dScan;
            for (i = 1; i * dYear < dLeg; i++) {
                pt.x = static_cast<std::int16_t>(std::floor(ptA.x + (ptB.x - ptA.x) * i * dYear / dLeg + 0.5));
                pt.y = static_cast<std::int16_t>(std::floor(ptA.y + (ptB.y - ptA.y) * i * dYear / dLeg + 0.5));
                for (iSide = -1; iSide <= 1; iSide += 2) {
                    win32::MoveToEx(hdc, pt.x + static_cast<std::int16_t>(std::floor(iSide * dxTick + 0.5)), pt.y + static_cast<std::int16_t>(std::floor(iSide * dyTick + 0.5)), nullptr);
                    win32::LineTo(hdc, pt.x + static_cast<std::int16_t>(std::floor(iSide * 4 * dxTick + 0.5)), pt.y + static_cast<std::int16_t>(std::floor(iSide * 4 * dyTick + 0.5)));
                }
            }
        }
    }
    win32::MoveToEx(hdc, ptB.x, ptB.y, nullptr);
}

// SetScanPathWarp sets the warp of the selected fleet's waypoint iwp. Its
// path's year ticks depend on the warp, so a path on screen is erased
// first and drawn again after.
void SetScanPathWarp(APPSESSION &app, std::int16_t iwp, std::int16_t iWarp) {
    std::int16_t fVis;

    fVis = app.lpui->fOrdersVis;
    if (fVis) {
        DrawShipScanPath(app, nullptr, 0);
    }
    app.state.sel.fl.lpplord->rgord()[iwp].iWarp = iWarp;
    if (fVis) {
        DrawShipScanPath(app, nullptr, 1);
    }
    return;
}

void DrawShipScanPath(APPSESSION &app, HDC hdc, std::int16_t fShow) {
    ORDER  *lpord2;
    std::int16_t rgDup[87];
    std::int16_t j;
    HPEN    hpenSav;
    POINT16 pt2;
    POINT16 pt;
    std::int16_t iRopSav;
    std::int16_t dy;
    ORDER  *lpord1;
    POINT16 ptCur;
    FLEET  *lpfl;
    std::int16_t i;
    std::int16_t fHdc;
    std::int32_t lWarp2;
    std::int16_t dRad;
    std::int16_t dx;
    RECT    rc;
    THING  *lpth;
    double  dAngle;
    POINT16 rgptArrow[2];
    std::int16_t dx5;
    POINT16 ptTick;
    std::int16_t dy5;
    double  m;
    std::int16_t id;
    std::int16_t fDoneRoute;

    fHdc = 0;
    if ((app.state.sel.grobj == grobjFleet || app.state.sel.scan.grobj == grobjFleet || app.state.sel.scan.grobj == grobjThing ||
         (app.state.sel.grobj == grobjPlanet && ((app.state.sel.pl.fStarbase && app.state.sel.pl.idFling != 0) || app.state.sel.pl.idRoute != 0))) &&
        (fShow != app.lpui->fOrdersVis && !app.state.gd.fNoScannerDraw)) {
        app.lpui->fOrdersVis = fShow;
        if (!hdc) {
            hdc = win32::GetDC(app.lpui->hwndScanner);
            fHdc = 1;
        }
        if (app.state.sel.scan.grobj == grobjThing && app.state.sel.scan.ith != ithNone) {
            lpth = app.state.lpThings + app.state.sel.scan.ith;
            ptCur = lpth->pt();
            if (lpth->ith() == ithMysteryTrader) {
                dx = lpth->tht().ptDest().x() - ptCur.x;
                dy = lpth->tht().ptDest().y() - ptCur.y;
                lWarp2 = lpth->tht().iWarp();
            } else {
                if (lpth->ith() != ithMineralPacket || lpth->thp().iWarp() == 0)
                    goto LNextCheck;
                dx = app.state.rgptPlan[lpth->thp().idPlanet()].x - ptCur.x;
                dy = app.state.rgptPlan[lpth->thp().idPlanet()].y - ptCur.y;
                lWarp2 = static_cast<std::uint32_t>((lpth->thp().iWarp() + 4));
            }
            if (dx != 0 || dy != 0) {
                lWarp2 = static_cast<std::uint32_t>((lWarp2 * static_cast<std::uint32_t>((lWarp2 * 5))));
                lpfl = nullptr;
                goto LCommonLineCode;
            }
        }
    LNextCheck:
        if (app.state.sel.scan.grobj != grobjFleet)
            goto LNoObjPath;
        lpfl = app.state.rglpfl[app.state.sel.scan.ifl];
        ptCur = lpfl->pt;
        if (!lpfl->fdirValid() || lpfl->iwarpFlt() <= 0)
            goto LNoObjPath;
        dx = lpfl->dirFltX() - 127;
        dy = lpfl->dirFltY() - 127;
        if (dx == 0 && dy == 0)
            goto LNoObjPath;
        lWarp2 = static_cast<std::uint32_t>((lpfl->iwarpFlt() * lpfl->iwarpFlt() * 5));
    LCommonLineCode:
        ControlIO().pfnGetClientRect(app.lpui->hwndScanner, &rc);
        win32::ExcludeClipRect(hdc, rc.left, rc.bottom - app.lpui->dySBar, rc.right, rc.bottom);
        hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, app.lpui->hpenStarbase));
        iRopSav = win32::SetROP2(hdc, win32::R2_XORPEN);
        if (dx == 0) {
            dx5 = 0;
            dy5 = dy >= 0 ? LOWORD(lWarp2) : -LOWORD(lWarp2);
        } else {
            lWarp2 = static_cast<std::uint32_t>((lWarp2 * lWarp2));
            m = static_cast<double>((static_cast<double>(dy) / dx));
            dx5 = LOWORD(static_cast<std::int32_t>(std::sqrt(static_cast<double>((lWarp2 / (static_cast<double>(m) * m + 1))))));
            if (dx < 0) {
                dx5 = -dx5;
            }
            dy5 = LOWORD(static_cast<std::int32_t>((static_cast<std::int32_t>((dx5 * dy)) / dx)));
        }
        LogicalToScan(app, &ptCur);
        dx5 = PtToScan(app, dx5);
        dy5 = -PtToScan(app, dy5);
        win32::MoveToEx(hdc, ptCur.x - dx5, ptCur.y - dy5, nullptr);
        win32::LineTo(hdc, ptCur.x + dx5, ptCur.y + dy5);
        if (dy == 0) {
            ptTick.x = 0;
            ptTick.y = dx >= 0 ? 4 : -4;
        } else {
            m = static_cast<double>((static_cast<double>(static_cast<std::int16_t>(-dx)) / dy));
            ptTick.x = LOWORD(static_cast<std::int32_t>(std::sqrt(static_cast<double>((24.0 / (static_cast<double>(m) * m + 1))))));
            if (ptTick.x == 0) {
                ptTick.y = dy >= 0 ? 4 : -4;
            } else {
                if (dx > 0) {
                    ptTick.x = -ptTick.x;
                }
                ptTick.y = LOWORD(static_cast<std::int32_t>((static_cast<std::int32_t>((ptTick.x * dx)) / dy)));
            }
        }
        dAngle = static_cast<double>((static_cast<double>(std::atan2(static_cast<double>(static_cast<std::int16_t>(-dy)), static_cast<double>(static_cast<std::int16_t>(-dx)))) - 0.7853982));
        for (i = 0; i < 2; i++) {
            rgptArrow[i].x = LOWORD(static_cast<std::int32_t>((static_cast<double>(5) * std::cos(dAngle) + 0.5)));
            rgptArrow[i].y = LOWORD(static_cast<std::int32_t>((static_cast<double>(5) * std::sin(dAngle) + 0.5)));
            dAngle = static_cast<double>((static_cast<double>(dAngle) + 1.5707964));
        }
        j = -5;
        for (i = -5; i <= 5; i++) {
            if (i == 0) {
                j = 5;
            } else {
                pt.x = LOWORD(static_cast<std::int32_t>(((static_cast<std::int32_t>((static_cast<std::uint32_t>((dx5 * i)) * 2)) + j) / 10))) + ptCur.x;
                pt.y = LOWORD(static_cast<std::int32_t>(((static_cast<std::int32_t>((static_cast<std::uint32_t>((dy5 * i)) * 2)) + j) / 10))) + ptCur.y;
                if (i > 0) {
                    for (j = 0; j < 2; j++) {
                        win32::MoveToEx(hdc, pt.x + rgptArrow[j].x, pt.y - rgptArrow[j].y, nullptr);
                        win32::LineTo(hdc, pt.x, pt.y);
                    }
                } else {
                    win32::MoveToEx(hdc, pt.x + ptTick.x, pt.y + ptTick.y, nullptr);
                    win32::LineTo(hdc, pt.x - ptTick.x, pt.y - ptTick.y);
                    win32::LineTo(hdc, pt.x - ptTick.x, pt.y - ptTick.y - 1);
                }
            }
        }
        win32::SetROP2(hdc, iRopSav);
        win32::SelectObject(hdc, hpenSav);
    LNoObjPath:
        if (app.state.sel.grobj == grobjPlanet) {
            fDoneRoute = 0;
            if (app.state.sel.pl.fStarbase && app.state.sel.pl.idFling != 0) {
                hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, app.lpui->hpenDkPurple));
                id = app.state.sel.pl.idFling - 1;
            LDrawPath:
                iRopSav = win32::SetROP2(hdc, win32::R2_XORPEN);
                ControlIO().pfnGetClientRect(app.lpui->hwndScanner, &rc);
                win32::ExcludeClipRect(hdc, rc.left, rc.bottom - app.lpui->dySBar, rc.right, rc.bottom);
                pt = app.state.rgptPlan[app.state.sel.pl.id];
                LogicalToScan(app, &pt);
                win32::MoveToEx(hdc, pt.x, pt.y, nullptr);
                pt = app.state.rgptPlan[id];
                LogicalToScan(app, &pt);
                win32::LineTo(hdc, pt.x, pt.y);
                win32::SetROP2(hdc, iRopSav);
                win32::SelectObject(hdc, hpenSav);
            }
            if (app.state.sel.pl.idRoute == 0 || fDoneRoute)
                goto LFinishUp;
            fDoneRoute = 1;
            id = app.state.sel.pl.idRoute - 1;
            hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, app.lpui->hpenDkGreen));
            goto LDrawPath;
        }
        if (app.state.sel.grobj == grobjFleet && app.state.sel.fl.cord > 1) {
            std::fill_n(rgDup, app.state.sel.fl.cord, 0);
            lpord1 = app.state.sel.fl.lpplord->rgord();
            pt = lpord1->pt;
            i = 1;
            while (i < app.state.sel.fl.cord) {
                lpord2 = lpord1 + 1;
                pt2 = lpord2->pt;
                if (rgDup[i] != 0)
                    goto DoNext;

                j = i + 1;
                while (j < app.state.sel.fl.cord) {
                    if ((pt.x == lpord2->pt.x && pt.y == lpord2->pt.y && pt2.x == lpord2[1].pt.x && pt2.y == lpord2[1].pt.y) ||
                        (pt2.x == lpord2->pt.x && pt2.y == lpord2->pt.y && pt.x == lpord2[1].pt.x && pt.y == lpord2[1].pt.y)) {
                        rgDup[i] = 1;
                        rgDup[j] = 2;
                    }
                    j++;
                    lpord2++;
                }

            DoNext:
                pt = pt2;
                i++;
                lpord1++;
            }
            ControlIO().pfnGetClientRect(app.lpui->hwndScanner, &rc);
            win32::ExcludeClipRect(hdc, rc.left, rc.bottom - app.lpui->dySBar, rc.right, rc.bottom);
            if (fShow && (app.lpui->grbitScan & grbitScanFleetPaths)) {
                hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, app.lpui->hpenStarbase));
                pt = app.state.sel.fl.lpplord->rgord()[0].pt;
                LogicalToScan(app, &pt);
                win32::MoveToEx(hdc, pt.x, pt.y, nullptr);
                for (i = 1; i < app.state.sel.fl.cord; i++) {
                    pt2 = app.state.sel.fl.lpplord->rgord()[i].pt;
                    LogicalToScan(app, &pt2);
                    win32::LineTo(hdc, pt2.x, pt2.y);
                    pt = pt2;
                }
                win32::SelectObject(hdc, hpenSav);
            }
            hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, app.lpui->hpenShip));
            iRopSav = win32::SetROP2(hdc, win32::R2_XORPEN);
            pt = app.state.sel.fl.lpplord->rgord()[0].pt;
            dRad = pt.x == app.state.sel.pt.x && pt.y == app.state.sel.pt.y ? 5 : 5;
            LogicalToScan(app, &pt);
            win32::ExcludeClipRect(hdc, pt.x - dRad, pt.y - dRad, pt.x + dRad + 1, pt.y + dRad + 1);
            win32::MoveToEx(hdc, pt.x, pt.y, nullptr);
            for (i = 1; i < app.state.sel.fl.cord; i++) {
                pt2 = app.state.sel.fl.lpplord->rgord()[i].pt;
                dRad = pt2.x == app.state.sel.pt.x && pt2.y == app.state.sel.pt.y ? 5 : 5;
                LogicalToScan(app, &pt2);
                win32::ExcludeClipRect(hdc, pt2.x - dRad, pt2.y - dRad, pt2.x + dRad + 1, pt2.y + dRad + 1);
                if (rgDup[i] == 2) {
                    win32::MoveToEx(hdc, pt2.x, pt2.y, nullptr);
                } else {
                    if (rgDup[i] == 1) {
                        win32::SelectObject(hdc, (app.lpui->grbitScan & grbitScanFleetPaths) ? app.lpui->hpenYellow : win32::GetStockObject(win32::WHITE_PEN));
                    }
                    win32::LineTo(hdc, pt2.x, pt2.y);
                    if (rgDup[i] == 1) {
                        win32::SelectObject(hdc, app.lpui->hpenShip);
                    }
                    DrawPathYearTicks(app, hdc, app.state.sel.fl.lpplord->rgord()[i - 1].pt, app.state.sel.fl.lpplord->rgord()[i].pt, app.state.sel.fl.lpplord->rgord()[i].iWarp);
                }
                pt = pt2;
            }
            win32::SetROP2(hdc, iRopSav);
            win32::SelectObject(hdc, hpenSav);
            win32::SelectClipRgn(hdc, app.lpui->hrgnHuge);
        }
    LFinishUp:
        if (fHdc) {
            win32::ReleaseDC(app.lpui->hwndScanner, hdc);
        }
    }
    return;
}

void DrawScannerSBar(APPSESSION &app, HDC hdc, RECT *prc, SBAR *psbar, std::int16_t fFullRedraw) {
    std::array<char, 360> szObjectName{};
    std::int16_t    fhdc;
    COLORREF   crText;
    POINT16    pt2;
    std::int16_t    id;
    POINT16    pt;
    std::int16_t    grReal;
    std::int16_t    iBkPrev;
    std::int16_t    c;
    COLORREF   crBk;
    std::int16_t    dxHole;
    HFONT      hfontSav;
    RECT       rcClip;
    char      *psz;
    HBRUSH     hbrSav;
    std::int16_t    fDoName;
    std::int32_t    l;
    GrobjClass grobj;
    RECT       rcT;
    RECT       rc;
    char       szBuf[100];

    fhdc = 0;
    fDoName = 1;
    ControlIO().pfnGetClientRect(app.lpui->hwndScanner, &rc);
    rc.top = rc.bottom - app.lpui->dySBar;
    if (!prc || win32::IntersectRect(&rcT, prc, &rc) != 0) {
        if (!hdc) {
            fhdc = 1;
            hdc = win32::GetDC(app.lpui->hwndScanner);
        }
        hfontSav = static_cast<HFONT>(win32::SelectObject(hdc, app.lpui->rghfontArial8[1]));
        if (psbar) {
            grobj = static_cast<GrobjClass>(psbar->grbit);
        } else {
            grobj = app.state.sel.scan.grobj;
        }
        iBkPrev = win32::SetBkMode(hdc, win32::TRANSPARENT);
        crBk = win32::SetBkColor(hdc, app.lpui->crButtonFace);
        crText = win32::SetTextColor(hdc, app.lpui->crButtonText);
        hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrButtonFace));
        if (fFullRedraw) {
            win32::PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, app.lpui->dySBar, win32::PATCOPY);
            win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
            win32::PatBlt(hdc, rc.left, rc.top, 1, app.lpui->dySBar, win32::PATCOPY);
            win32::PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, 1, win32::PATCOPY);
        }
        rc.bottom -= app.lpui->dySBar >> 1;
        rcT = rc;
        rcT.top += 4;
        rcT.bottom -= 4;
        rcT.left += 4;
        if (rc.right < 360)
            goto DrawTheName;

        l = GetTextExtent(hdc, "ID #000", 7);
        dxHole = LOWORD(l) + 6;
        rcT.right = LOWORD(l) + 6 + rcT.left;
        DrawLockLight(app, hdc, &rcT, fFullRedraw);
        if (grobj & grobjPlanet) {
            if (psbar) {
                id = psbar->id;
            } else {
                id = app.state.sel.scan.idpl;
            }
            if (id != idplNone) {
                c = win32::wsprintfA(app.state.szWork, "ID #%d", id + 1);
                win32::SetBkMode(hdc, win32::OPAQUE);
                win32::TextOutA(hdc, rcT.left + 3, rcT.top + 2, app.state.szWork, c);
            }
        } else if (grobj == grobjOther) {
            if (psbar) {
                id = psbar->id;
            } else {
                id = app.state.sel.scan.iwp;
            }
            c = win32::wsprintfA(app.state.szWork, "WP #%d", id);
            win32::TextOutA(hdc, rcT.left + 3, rcT.top + 2, app.state.szWork, c);
        }
        l = GetTextExtent(hdc, "X: 8888", 7);
        rcT.left += dxHole + 4;
        dxHole = LOWORD(l) + 6;
        rcT.right = LOWORD(l) + 6 + rcT.left;
        DrawLockLight(app, hdc, &rcT, fFullRedraw);
        if (psbar) {
            pt = psbar->pt;
            goto GotCoords;
        } else if (grobj != grobjNone) {
            pt = app.state.sel.scan.pt;
            goto GotCoords;
        } else {
            pt.y = 0;
            pt.x = 0;
        }
    GotCoords:
        if (pt.x > 0) {
            c = win32::wsprintfA(app.state.szWork, "X: %d", pt.x);
            win32::SetBkMode(hdc, win32::OPAQUE);
            win32::TextOutA(hdc, rcT.left + 3, rcT.top + 2, app.state.szWork, c);
        }
        rcT.left += dxHole + 4;
        dxHole = LOWORD(l) + 6;
        rcT.right = LOWORD(l) + 6 + rcT.left;
        DrawLockLight(app, hdc, &rcT, fFullRedraw);
        if (pt.y > 0) {
            c = win32::wsprintfA(app.state.szWork, "Y: %d", pt.y);
            win32::SetBkMode(hdc, win32::OPAQUE);
            win32::TextOutA(hdc, rcT.left + 3, rcT.top + 2, app.state.szWork, c);
        }
        rcT.left += dxHole + 4;

    DrawTheName:
        dxHole = rc.right - rcT.left - 4;
        rcT.right = rc.right - rcT.left - 4 + rcT.left;
        DrawLockLight(app, hdc, &rcT, fFullRedraw);
        rcClip = rcT;
        ExpandRc(&rcClip, -2, -2);
        grReal = grobj == grobjOther ? app.state.sel.scan.grobjFull : grobj;
        if (psbar && psbar->psz) {
            CchCopyUtf8Prefix(std::span{app.state.szWork}, psbar->psz);
        } else if (grReal & 1) {
            szObjectName = TextIO().pfnPlanetName(app, !psbar ? app.state.sel.scan.idpl : psbar->id);
            CchCopyUtf8Prefix(std::span{app.state.szWork}, szObjectName.data());
        } else if (grReal & 2) {
            szObjectName = PszGetFleetName(app, !psbar ? app.state.rglpfl[app.state.sel.scan.ifl]->id : psbar->id);
            CchCopyUtf8Prefix(std::span{app.state.szWork}, szObjectName.data());
        } else if (grReal & 8) {
            szObjectName = PszGetThingName(app, !psbar ? app.state.lpThings[app.state.sel.scan.ith].idFull() : psbar->id);
            CchCopyUtf8Prefix(std::span{app.state.szWork}, szObjectName.data());
        } else if (grReal == 0 && grobj == grobjOther) {
            TextIO().pfnCchGetString(app, idsDeepSpaceWaypoint, std::span{app.state.szWork});
        } else {
            fDoName = 0;
        }
        if (fDoName) {
            win32::ExtTextOutA(hdc, rcT.left + 3, rcT.top + 2, win32::ETO_CLIPPED, &rcClip, app.state.szWork, win32::lstrlenA(app.state.szWork), nullptr);
        }
        OffsetRc(&rc, 0, app.lpui->dySBar >> 1);
        rcT = rc;
        rcT.top += 4;
        rcT.bottom -= 4;
        rcT.left += 4;
        rcT.right -= 4;
        DrawLockLight(app, hdc, &rcT, fFullRedraw);
        rcClip = rcT;
        ExpandRc(&rcClip, -2, -2);
        if (psbar) {
            pt = psbar->pt;
        } else if (grobj != grobjNone) {
            pt = app.state.sel.scan.pt;
        } else {
            pt.y = 0;
            pt.x = 0;
        }
        if (psbar && psbar->pscan) {
            pt2 = psbar->pscan->pt;
        } else if (app.state.sel.grobj == grobjFleet || app.state.sel.grobj == grobjPlanet) {
            pt2 = app.state.sel.pt;
        } else {
            pt2.x = -1;
        }
        if (pt.x != -1 && pt2.x != -1 && (pt.x != pt2.x || pt.y != pt2.y)) {
            CchCopyUtf8Prefix(std::span{szBuf}, PszGetDistance(app, pt.x, pt.y, pt2.x, pt2.y).data());
            for (psz = szBuf; *psz != ' '; psz++) {
            }
            TextIO().pfnCchGetString(app, static_cast<StringId>(idsLy + (rc.right >= 350)), std::span{szBuf}.subspan(static_cast<std::size_t>(psz - szBuf + 1)));
            if (!psbar || !psbar->pscan) {
                TextIO().pfnCchGetString(app, idsFrom, std::span{szBuf}.subspan(static_cast<std::size_t>(psz - szBuf + std::strlen(psz))));
                std::strcat(psz + 8, PszGetLocName(app, app.state.sel.grobj, app.state.sel.id, pt2.x, pt2.y).data());
            }
            win32::ExtTextOutA(hdc, rcT.left + 3, rcT.top + 2, win32::ETO_CLIPPED, &rcClip, szBuf, std::strlen(szBuf), nullptr);
        }
        win32::SelectObject(hdc, hbrSav);
        win32::SetBkMode(hdc, iBkPrev);
        win32::SetTextColor(hdc, app.lpui->crButtonText);
        win32::SetBkColor(hdc, crBk);
        win32::SelectObject(hdc, hfontSav);
        if (fhdc) {
            win32::ReleaseDC(app.lpui->hwndScanner, hdc);
        }
    }
    return;
}

void DrawLockLight(APPSESSION &app, HDC hdc, RECT *prc, std::int16_t fFullRedraw) {
    std::int16_t dy;
    std::int16_t dx;
    RECT    rc;

    dx = prc->right - prc->left;
    dy = prc->bottom - prc->top;
    if (fFullRedraw) {
        win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
        win32::PatBlt(hdc, prc->left, prc->top, 1, dy, win32::PATCOPY);
        win32::PatBlt(hdc, prc->left, prc->top, dx, 1, win32::PATCOPY);
        win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
        win32::PatBlt(hdc, prc->right, prc->top, 1, dy + 1, win32::PATCOPY);
        win32::PatBlt(hdc, prc->left, prc->bottom, dx, 1, win32::PATCOPY);
    } else {
        rc = *prc;
        ExpandRc(&rc, -2, -2);
        win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
    }
    return;
}

void SetScanScrollBars(APPSESSION &app, HWND hwnd) {
    std::int16_t xMax;
    std::int16_t dy;
    std::int16_t yMax;
    std::int16_t dx;
    RECT    rc;
    SCROLLINFO si = {sizeof(si), win32::SIF_RANGE | win32::SIF_PAGE | win32::SIF_POS, 1000, 0, 0, 0, 0};

    app.lpui->fInScrollSet = 1;
    ControlIO().pfnGetClientRect(hwnd, &rc);
    dx = ScanToPt(app, rc.right);
    dy = ScanToPt(app, rc.bottom - app.lpui->dySBar);
    xMax = ((1000 <= app.state.dGalInv - 1000 - dx ? app.state.dGalInv - 0x3e8 - dx : 0x3e8) + 3) & 0xfffc;
    yMax = ((1000 <= app.state.dGalInv - 1000 - dy ? app.state.dGalInv - 0x3e8 - dy : 0x3e8) + 3) & 0xfffc;
    /* The old maxima are the last scroll positions, not content extents.
       Include the visible span while retaining those exact pan limits. */
    si.nPage = static_cast<UINT>(StarsMax(1, dx));
    si.nMax = xMax + static_cast<int>(si.nPage) - 1;
    si.nPos = app.lpui->xScanTop;
    win32::SetScrollInfo(hwnd, win32::SB_HORZ, &si, 1);
    if (app.lpui->fInScrollSet) {
        si.nPage = static_cast<UINT>(StarsMax(1, dy));
        si.nMax = yMax + static_cast<int>(si.nPage) - 1;
        si.nPos = app.lpui->yScanTop;
        win32::SetScrollInfo(hwnd, win32::SB_VERT, &si, 1);
        if (app.lpui->fInScrollSet) {
            app.lpui->dScanPage = static_cast<std::int16_t>((dx >= dy ? dy : dx)) / 3 & 0xfffc;
            app.lpui->dScanInc = (app.lpui->dScanPage / 8 + 2) & 0xfffc;
            app.lpui->fInScrollSet = 0;
        }
    }
    return;
}

void ScrollScanner(APPSESSION &app, std::int16_t dx, std::int16_t dy) {
    HDC     hdc;
    RECT    rcUpd;
    RECT    rcUpd2;
    RECT    rc;
    std::int16_t fPending;

    if ((dx != 0 || dy != 0) && win32::IsWindowVisible(app.lpui->hwndScanner) != 0 && !app.state.gd.fNoScannerDraw) {
        hdc = win32::GetDC(app.lpui->hwndScanner);
        ControlIO().pfnGetClientRect(app.lpui->hwndScanner, &rc);
        // The scroll handlers move the scanner's top before calling here, so
        // a repaint still pending would be drawn at the new position and
        // then scrolled with the rest, leaving a band drawn out of place.
        // A wheel zoom invalidates the whole scanner, and a trackpad's next
        // scroll comes before its WM_PAINT. Redraw the map instead.
        fPending = win32::GetUpdateRect(app.lpui->hwndScanner, nullptr, 0) != 0;
        if (std::abs(dx) > rc.right >> 1 || std::abs(dy) > rc.bottom >> 1 || app.lpui->fDlgUp || app.lpui->hwndBrowser || fPending) {
            DrawScanner(app, hdc, &rc);
            if (fPending) {
                rc.bottom -= app.lpui->dySBar;
                win32::ValidateRect(app.lpui->hwndScanner, &rc);
            }
            goto RelDC;
        }
        rc.bottom -= app.lpui->dySBar;
        ControlIO().pfnUpdateWindow(app.lpui->hwndScanner);
        win32::ScrollWindow(app.lpui->hwndScanner, dx, dy, &rc, &rc);
        if (dy > 0) {
            win32::SetRect(&rcUpd2, 0, 0, rc.right, dy);
            rc.top += dy;
        } else if (dy < 0) {
            win32::SetRect(&rcUpd2, 0, rc.bottom + dy, rc.right, rc.bottom);
            rc.bottom += dy;
        }
        if (dx > 0) {
            win32::SetRect(&rcUpd, 0, rc.top, dx, rc.bottom);
            rc.right -= dx;
        } else if (dx < 0) {
            rcUpd = rc;
            rcUpd.left = rc.right + dx;
            rc.left -= dx;
        }
        if (dx != 0) {
            win32::ValidateRect(app.lpui->hwndScanner, &rcUpd);
        }
        if (dy != 0) {
            win32::ValidateRect(app.lpui->hwndScanner, &rcUpd2);
        }
        if (dx != 0) {
            DrawScanner(app, hdc, &rcUpd);
        }
        if (dy != 0) {
            DrawScanner(app, hdc, &rcUpd2);
        }
        ControlIO().pfnUpdateWindow(app.lpui->hwndScanner);
    RelDC:
        win32::ReleaseDC(app.lpui->hwndScanner, hdc);
    }
    return;
}

void RedrawScanSel(APPSESSION &app, HDC hdc, std::int16_t fVis) {
    std::int16_t sel_grobj;
    std::int16_t fhdc;
    std::int16_t dOff;
    POINT16 pt;
    std::int16_t sel_id;
    std::int16_t fNoSelRedraw;
    SCAN    sel_scan;
    RECT    rc;
    std::int16_t sel_grobjFull;

    fhdc = 0;
    sel_id = app.state.sel.id;
    sel_grobj = app.state.sel.grobj;
    sel_grobjFull = app.state.sel.grobjFull;
    sel_scan = app.state.sel.scan;
    if (app.lpui->hwndScanner && win32::IsWindowVisible(app.lpui->hwndScanner) != 0) {
        if (fVis == -1) {
            fVis = 0;
            fNoSelRedraw = 1;
        } else {
            fNoSelRedraw = 0;
        }
        if (!hdc) {
            fhdc = 1;
            hdc = win32::GetDC(app.lpui->hwndScanner);
        }
        DrawShipScanPath(app, hdc, fVis);
        if (!fVis) {
            app.state.sel.scan.iwp = iwpNone;
            app.state.sel.scan.ifl = iflNone;
            app.state.sel.scan.idpl = idplNone;
            app.state.sel.id = -1;
            app.state.sel.grobjFull = grobjNone;
            app.state.sel.grobj = grobjNone;
            app.state.sel.scan.grobjFull = grobjNone;
            app.state.sel.scan.grobj = grobjNone;
        }
        dOff = app.state.iScanZoom >= zoom200 && (app.lpui->grbitScan & grbitScanViewMask) == 4 ? 11 : (app.lpui->grbitScan & grbitScanShipCounts) ? 7 : 0;
        if (sel_grobj != 0 && !fNoSelRedraw) {
            pt = app.state.sel.pt;
            LogicalToScan(app, &pt);
            win32::SetRect(&rc, pt.x - 11 - dOff, pt.y - 11 - dOff, pt.x + 12 + dOff, pt.y + 23 + dOff);
            DrawScanner(app, hdc, &rc);
        }
        if (sel_scan.grobj != grobjNone && (sel_scan.pt.x != app.state.sel.pt.x || sel_scan.pt.y != app.state.sel.pt.y)) {
            pt = sel_scan.pt;
            LogicalToScan(app, &pt);
            win32::SetRect(&rc, pt.x - 6 - dOff, pt.y - 6 - dOff, pt.x + 7 + dOff, pt.y + 15 + dOff);
            DrawScanner(app, hdc, &rc);
        }
        if (!fVis) {
            app.state.sel.id = sel_id;
            app.state.sel.grobj = static_cast<GrobjClass>(sel_grobj);
            app.state.sel.grobjFull = static_cast<GrobjClass>(sel_grobjFull);
            app.state.sel.scan = sel_scan;
        }
        if (fhdc) {
            win32::ReleaseDC(app.lpui->hwndScanner, hdc);
        }
    }
    return;
}

std::int16_t FEnsurePointOnScreen(APPSESSION &app, POINT16 pt, std::int16_t fScroll) {
    std::int16_t cy;
    std::int16_t fFix;
    std::int16_t cx;
    POINT16 ptCtr;
    RECT    rc;

    fFix = 0;
    ControlIO().pfnGetClientRect(app.lpui->hwndScanner, &rc);
    rc.bottom -= app.lpui->dySBar;
    cx = ScanToPt(app, rc.right);
    cy = ScanToPt(app, rc.bottom);
    rc.left = app.lpui->xScanTop + 10;
    rc.right = app.lpui->xScanTop + cx - 20;
    rc.bottom = app.state.dGalInv - app.lpui->yScanTop - 10;
    rc.top = app.state.dGalInv - app.lpui->yScanTop - 10 - cy + 20;
    if (win32::PtInRect(&rc, PointFrom16(pt)) != 0) {
        return 1;
    }
    ptCtr.x = (cx >> 1) + app.lpui->xScanTop;
    ptCtr.y = app.state.dGalInv - app.lpui->yScanTop - (cy >> 1);
    if (pt.x < rc.left) {
        ptCtr.x -= rc.left - pt.x;
    } else if (pt.x > rc.right) {
        ptCtr.x += pt.x - rc.right;
    }
    if (pt.y < rc.top) {
        ptCtr.y -= rc.top - pt.y;
    } else if (pt.y > rc.bottom) {
        ptCtr.y += pt.y - rc.bottom;
    }
    CtrPointScan(app, ptCtr, fScroll);
    return 0;
}

void CtrPointScan(APPSESSION &app, POINT16 pt, std::int16_t fScroll) {
    std::int16_t dxCur;
    std::int16_t cy;
    std::int16_t y;
    std::int16_t cx;
    std::int16_t dyCur;
    std::int16_t x;
    RECT    rc;

    x = pt.x;
    y = pt.y;
    if (app.lpui->hwndScanner) {
        ControlIO().pfnGetClientRect(app.lpui->hwndScanner, &rc);
        rc.bottom -= app.lpui->dySBar;
        cx = ScanToPt(app, rc.right);
        cy = ScanToPt(app, rc.bottom);
        x = x - (cx >> 1) <= 1000 ? 1000 : x - (cx >> 1);
        y = (cy >> 1) + y >= app.state.dGalInv - 1000 ? app.state.dGalInv - 1000 : (cy >> 1) + y;
        x = x >= app.state.dGalInv - 1000 - cx ? app.state.dGalInv - 1000 - cx : x;
        y = app.state.dGalInv - (y <= cy + 1000 ? cy + 1000 : y);
        x = x <= 1000 ? 1000 : x;
        y = y <= 1000 ? 1000 : y;
        dxCur = app.lpui->xScanTop;
        dyCur = app.lpui->yScanTop;
        x = (x + 2) & 0xfffc;
        y = (y + 2) & 0xfffc;
        if (dxCur != x || dyCur != y) {
            app.lpui->xScanTop = x;
            win32::SetScrollPos(app.lpui->hwndScanner, win32::SB_HORZ, x, 1);
            app.lpui->yScanTop = y;
            win32::SetScrollPos(app.lpui->hwndScanner, win32::SB_VERT, y, 1);
            if (fScroll) {
                ScrollScanner(app, PtToScan(app, dxCur - x), PtToScan(app, dyCur - y));
            } else {
                ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, nullptr, 0);
            }
        }
    }
    return;
}

// ZoomScanAt sets the scanner zoom to iScanNew and keeps the galaxy point
// under ptScan, a scanner client point, where it is on screen. The menu's
// zoom command centers on the selection instead.
void ZoomScanAt(APPSESSION &app, ScanZoom iScanNew, POINT16 ptScan) {
    POINT16 pt;
    RECT    rc;

    ControlIO().pfnGetClientRect(app.lpui->hwndScanner, &rc);
    rc.bottom -= app.lpui->dySBar;
    if (ptScan.y >= rc.bottom) {
        ptScan.y = rc.bottom - 1;
    }
    pt = ptScan;
    ScanToLogical(app, &pt);
    ControlIO().pfnSendMessage(app.lpui->hwndFrame, win32::WM_COMMAND, iScanNew + 3905, 0);
    if (app.state.iScanZoom != iScanNew)
        return;
    pt.x += (ScanToPt(app, rc.right) >> 1) - ScanToPt(app, ptScan.x);
    pt.y += ScanToPt(app, ptScan.y) - (ScanToPt(app, rc.bottom) >> 1);
    CtrPointScan(app, pt, 0);
    return;
}

void LogicalToScan(APPSESSION &app, POINT16 *ppt) {
    ppt->x = PtToScan(app, ppt->x - app.lpui->xScanTop);
    ppt->y = PtToScan(app, app.state.dGalInv - ppt->y - app.lpui->yScanTop);
    return;
}

void ScanToLogical(APPSESSION &app, POINT16 *ppt) {
    ppt->x = ScanToPt(app, ppt->x) + app.lpui->xScanTop;
    ppt->y = app.state.dGalInv - (ScanToPt(app, ppt->y) + app.lpui->yScanTop);
    if (ppt->x > app.state.dGal + 1000) {
        ppt->x = app.state.dGal + 1000;
    }
    if (ppt->y < 1000) {
        ppt->y = 1000;
    }
    return;
}

std::int16_t FAddWayPoint(APPSESSION &app, POINT16 ptIn, SCAN *pscan) {
    HDC     hdc;
    std::int16_t id;
    std::int16_t dy;
    ORDER  *lpord;
    std::int16_t lDist;
    POINT16 rgpt[3];
    std::int16_t dx;
    std::int16_t cpt;
    std::int16_t ipt;
    RECT    rc;

    if (app.state.sel.fl.cord == 87) {
        ControlIO().pfnMessageBeep(win32::MB_ICONASTERISK);
        win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsCantHaveDWaypoints).data(), 86);
        AlertIO().pfnAlertSz(app, app.state.szWork, win32::MB_ICONHAND);
        return 0;
    }
    if (app.lpui->grbitScan & grbitScanFleetPaths) {
        rgpt[0] = ptIn;
        rgpt[1] = pscan->pt;
        if (app.state.sel.iwpAct < app.state.sel.fl.cord - 1) {
            cpt = 3;
            rgpt[2] = app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct + 1].pt;
        } else {
            cpt = 2;
        }
    }
    dx = ptIn.x - pscan->pt.x;
    dy = ptIn.y - pscan->pt.y;
    lDist = ScanToPt(app, 20);
    if (static_cast<std::int32_t>((static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy)))) > static_cast<std::int16_t>((lDist * lDist))) {
        pscan->grobjFull = grobjOther;
        pscan->grobj = grobjOther;
        pscan->idpl = idplNone;
        pscan->ifl = iflNone;
        pscan->iwp = app.state.sel.iwpAct + 1;
        pscan->pt = ptIn;
    }
    lpord = &app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct];
    if (pscan->pt.x == lpord->pt.x && pscan->pt.y == lpord->pt.y) {
        return 0;
    }
    if (app.state.sel.iwpAct < app.state.sel.fl.cord - 1 && pscan->pt.x == lpord[1].pt.x && pscan->pt.y == lpord[1].pt.y) {
        return 0;
    }
    hdc = win32::GetDC(app.lpui->hwndScanner);
    DrawShipScanPath(app, hdc, 0);
    if (app.state.sel.fl.lpplord->iordMax == app.state.sel.fl.cord) {
        app.state.sel.fl.lpplord = LpplReAlloc(app, app.state.sel.fl.lpplord, app.state.sel.fl.cord + 3);
        lpord = &app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct + 1];
    } else {
        lpord++;
    }
    if (app.state.sel.iwpAct != app.state.sel.fl.cord - 1) {
        std::memmove(lpord + 1, lpord, (app.state.sel.fl.cord - app.state.sel.iwpAct - 1) * sizeof(ORDER));
    }
    *lpord = *(lpord - 1);
    lpord->pt = pscan->pt;
    switch (pscan->grobj) {
    default:
        break;
    case grobjPlanet:
        id = pscan->idpl;
        break;
    case grobjOther:
        id = pscan->iwp;
        break;
    case grobjFleet:
        id = app.state.rglpfl[pscan->ifl]->id;
        break;
    case grobjThing:
        id = app.state.lpThings[pscan->ith].idFull();
    }
    lpord->id = id;
    lpord->grobj = pscan->grobj;
    app.state.sel.fl.cord++;
    app.state.sel.fl.lpplord->iordMac++;
    lpord->iWarp = FAltDown() ? IWarpFastestForWaypoint(app, &app.state.sel.fl, lpord) : IWarpBestForWaypoint(app, &app.state.sel.fl, lpord);
    pscan->grobj = grobjOther;
    pscan->grobjFull = static_cast<GrobjClass>(pscan->grobjFull | grobjOther);
    pscan->iwp = app.state.sel.iwpAct + 1;
    RedrawScanSel(app, nullptr, 0);
    FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
    if (lpord[-1].grTask == grTaskLayMines && lpord[-1].tsell().iPlrX == 5) {
        lpord[-1].txp = {};
        lpord[-1].grTask = grTaskNone;
        FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
    }
    ChangeScanSel(app, pscan, 1);
    win32::ReleaseDC(app.lpui->hwndScanner, hdc);
    if (app.lpui->grbitScan & grbitScanFleetPaths) {
        for (ipt = 0; ipt < cpt; ipt++) {
            LogicalToScan(app, &rgpt[ipt]);
        }
        BoundPoints(&rc, rgpt, cpt);
        ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, &rc, 0);
    }
    return 1;
}

std::int16_t FNearAWayPoint(APPSESSION &app, POINT16 pt, std::int16_t fLogical) {
    ORDER  *lpord;
    std::int16_t i;
    SCAN    scan;

    if (app.state.sel.grobj != grobjFleet) {
        return 0;
    }
    if (!fLogical) {
        ScanToLogical(app, &pt);
    }
    if (!FFindNearestObject(app, pt, static_cast<GrobjClass>(grobjPlanet | grobjFleet | grobjOther | grobjThing | mdScanRadius), &scan)) {
        return 0;
    }
    if (scan.grobjFull & grobjOther) {
        if (scan.pt.x == app.state.sel.pt.x && scan.pt.y == app.state.sel.pt.y) {
            lpord = &app.state.sel.fl.lpplord->rgord()[1];
            i = 1;
            for (; i < app.state.sel.fl.cord && (lpord->pt.x != scan.pt.x || lpord->pt.y != scan.pt.y); lpord++) {
                i++;
            }
            if (i != app.state.sel.fl.cord) {
                return 1;
            }
            return 0;
        }
        return 1;
    }
    return 0;
}

std::int16_t FHandleWayPointDrag(APPSESSION &app, POINT16 pt) {
    std::int16_t fChg;
    HDC     hdc;
    HPEN    hpenSav;
    SBAR    sbar;
    std::int16_t fMarker;
    char    szDeepSpace[40];
    std::int16_t fDup;
    std::int16_t grTypeIn;
    HCURSOR hcurSav;
    ORDER  *lpord;
    std::int16_t i;
    POINT16 ptLogical;
    POINT16 ptNext;
    std::int16_t fDel;
    POINT16 rgpt[4];
    POINT16 ptNew;
    std::int16_t cpt;
    POINT16 ptPrev;
    SCAN    scan;
    std::int16_t fFirst;
    RECT    rc;

    fFirst = 1;
    fMarker = 0;
    LogicalToScan(app, &pt);
    if (app.state.sel.iwpAct == 0) {
        lpord = &app.state.sel.fl.lpplord->rgord()[1];
        i = 1;
        for (; i < app.state.sel.fl.cord && (app.state.sel.fl.pt.x != lpord->pt.x || app.state.sel.fl.pt.y != lpord->pt.y); lpord++) {
            i++;
        }
        SetScanWp(app, i);
    }
    rgpt[2] = app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct - 1].pt;
    ptPrev = rgpt[2];
    if (app.state.sel.iwpAct == app.state.sel.fl.cord - 1) {
        cpt = 3;
    } else {
        cpt = 4;
        rgpt[3] = app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct + 1].pt;
        ptNext = rgpt[3];
    }
    rgpt[0] = app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].pt;
    rgpt[1] = rgpt[0];
    for (i = 0; i < cpt; i++) {
        LogicalToScan(app, &rgpt[i]);
    }
    ControlIO().pfnGetClientRect(app.lpui->hwndScanner, &rc);
    rc.bottom -= app.lpui->dySBar;
    hdc = win32::GetDC(app.lpui->hwndScanner);
    hcurSav = win32::SetCursor(app.lpui->hcurCloseGrab);
    win32::SetCapture(app.lpui->hwndScanner);
    ptNew = pt;
    while (FGetMouseMove(&ptNew)) {
        ptNew.x = (((0) > (StarsMin(rc.right, ptNew.x))) ? (0) : (StarsMin(rc.right, ptNew.x)));
        ptNew.y = (((0) > (StarsMin(rc.bottom, ptNew.y))) ? (0) : (StarsMin(rc.bottom, ptNew.y)));
        if (pt.x != ptNew.x || pt.y != ptNew.y) {
            if (fFirst && FNearAWayPoint(app, ptNew, 0))
                goto DoNext;

            fFirst = 0;
            ptLogical = ptNew;
            ScanToLogical(app, &ptLogical);
            grTypeIn = !(ControlIO().pfnGetAsyncKeyState(win32::VK_SHIFT) & 0xfffe) ? 79 : 143;
            FFindNearestObject(app, ptLogical, static_cast<GrobjClass>(grTypeIn), &scan);
            DrawScanXorLines(app, hdc, rgpt, cpt);
            if (scan.grobj == grobjNone) {
                rgpt[0] = ptLogical;
                sbar.id = -1;
                TextIO().pfnCchGetString(app, idsDeepSpace, std::span{szDeepSpace});
                sbar.psz = szDeepSpace;
            } else {
                rgpt[0] = scan.pt;
                switch (scan.grobj) {
                case grobjPlanet:
                    sbar.id = scan.idpl;
                    break;
                case grobjFleet:
                    sbar.id = app.state.rglpfl[scan.ifl]->id;
                    break;
                case grobjThing:
                    sbar.id = app.state.lpThings[scan.ith].idFull();
                    break;
                default:
                    sbar.id = scan.iwp;
                }
                sbar.psz = nullptr;
            }
            sbar.pt = rgpt[0];
            sbar.grbit = scan.grobj;
            LogicalToScan(app, rgpt);
            DrawScanXorLines(app, hdc, rgpt, cpt);
            sbar.pscan = nullptr;
            DrawScannerSBar(app, hdc, nullptr, &sbar, 0);

        DoNext:
            pt = ptNew;
        }
    }
    fChg = rgpt[0].x != rgpt[1].x || rgpt[0].y != rgpt[1].y;
    if (fChg) {
        if (scan.grobj == grobjNone) {
            scan.pt = ptLogical;
            scan.grobj = grobjOther;
            scan.iwp = app.state.sel.iwpAct;
        }
        fDup = scan.pt.x == ptPrev.x && scan.pt.y == ptPrev.y;
        if (!fDup && app.state.sel.iwpAct < app.state.sel.fl.cord - 1) {
            fDup = (scan.pt.x == app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct + 1].pt.x && scan.pt.y == app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct + 1].pt.y) * 2;
        }
        ControlIO().pfnGetClientRect(app.lpui->hwndScanner, &rc);
        if (fDup) {
            fDel = AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsSureWantDeleteCurrentWaypoint, {}).data(), win32::MB_YESNO | win32::MB_ICONQUESTION | win32::MB_TASKMODAL) == win32::IDYES;
            if (app.lpui->grbitScan & grbitScanFleetPaths) {
                hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, app.lpui->hpenStarbase));
                win32::MoveToEx(hdc, rgpt[2].x, rgpt[2].y, nullptr);
                win32::LineTo(hdc, rgpt[0].x, rgpt[0].y);
                if (cpt > 3) {
                    win32::ExcludeClipRect(hdc, 0, rc.bottom - app.lpui->dySBar, rc.right, rc.bottom);
                    win32::LineTo(hdc, rgpt[3].x, rgpt[3].y);
                }
                win32::SelectObject(hdc, hpenSav);
            }
            DrawScanXorLines(app, hdc, rgpt, cpt);
            rgpt[0] = rgpt[1];
            if (app.lpui->grbitScan & grbitScanFleetPaths) {
                hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, app.lpui->hpenStarbase));
                win32::MoveToEx(hdc, rgpt[2].x, rgpt[2].y, nullptr);
                win32::LineTo(hdc, rgpt[0].x, rgpt[0].y);
                if (cpt > 3) {
                    win32::ExcludeClipRect(hdc, 0, rc.bottom - app.lpui->dySBar, rc.right, rc.bottom);
                    win32::LineTo(hdc, rgpt[3].x, rgpt[3].y);
                }
                win32::SelectObject(hdc, hpenSav);
            }
            DrawScanXorLines(app, hdc, rgpt, cpt);
            if (!fDel)
                goto Done;
            DeleteCurWayPoint(app, fDup == 1);
            goto Done;
        }
        if (app.lpui->grbitScan & grbitScanFleetPaths) {
            win32::ExcludeClipRect(hdc, 0, rc.bottom - app.lpui->dySBar, rc.right, rc.bottom);
            hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, app.lpui->hpenStarbase));
            win32::MoveToEx(hdc, rgpt[2].x, rgpt[2].y, nullptr);
            win32::LineTo(hdc, rgpt[0].x, rgpt[0].y);
            if (cpt > 3) {
                win32::LineTo(hdc, rgpt[3].x, rgpt[3].y);
            }
            win32::SelectObject(hdc, hpenSav);
        }
        DrawScanXorLines(app, hdc, rgpt, cpt);
        rgpt[0] = rgpt[1];
        if (app.lpui->grbitScan & grbitScanFleetPaths) {
            win32::ExcludeClipRect(hdc, 0, rc.bottom - app.lpui->dySBar, rc.right, rc.bottom);
            hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, app.lpui->hpenStarbase));
            win32::MoveToEx(hdc, rgpt[2].x, rgpt[2].y, nullptr);
            win32::LineTo(hdc, rgpt[0].x, rgpt[0].y);
            if (cpt > 3) {
                win32::LineTo(hdc, rgpt[3].x, rgpt[3].y);
            }
            win32::SelectObject(hdc, hpenSav);
        }
        DrawScanXorLines(app, hdc, rgpt, cpt);
        RedrawScanSel(app, nullptr, 0);
        switch (scan.grobj) {
        case grobjPlanet:
            i = scan.idpl;
            break;
        case grobjFleet:
            i = app.state.rglpfl[scan.ifl]->id;
            break;
        case grobjThing:
            i = app.state.lpThings[scan.ith].idFull();
            break;
        default:
            i = scan.iwp;
        }
        lpord = &app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct];
        lpord->grobj = scan.grobj;
        lpord->id = i;
        lpord->pt = scan.pt;
        lpord->iWarp = FAltDown() ? IWarpFastestForWaypoint(app, &app.state.sel.fl, lpord) : IWarpBestForWaypoint(app, &app.state.sel.fl, lpord);
        FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
        scan.iwp = app.state.sel.iwpAct;
        scan.grobjFull = static_cast<GrobjClass>(scan.grobjFull | grobjOther);
        app.state.sel.iwpAct = -2;
        ChangeScanSel(app, &scan, 1);
    }
    DrawScannerSBar(app, hdc, nullptr, nullptr, 0);
    win32::ReleaseCapture();
    win32::SetCursor(hcurSav);
    ControlIO().pfnInvalidateRect(app.lpui->hwndMine, nullptr, 1);
    SetMineralTitleBar(app, app.lpui->hwndMine);
Done:
    win32::ReleaseDC(app.lpui->hwndScanner, hdc);
    if (fChg && (app.lpui->grbitScan & grbitScanFleetPaths)) {
        rgpt[1] = ptNew;
        BoundPoints(&rc, rgpt, cpt);
        hdc = win32::GetDC(app.lpui->hwndScanner);
        DrawScanner(app, hdc, &rc);
        win32::ReleaseDC(app.lpui->hwndScanner, hdc);
    }
    return fChg;
}

void DrawScanXorLines(APPSESSION &app, HDC hdc, POINT16 *rgpt, std::int16_t cpt) {
    HPEN    hpenSav;
    std::int16_t iRopSav;
    std::int16_t i;
    RECT    rc;

    ControlIO().pfnGetClientRect(app.lpui->hwndScanner, &rc);
    win32::ExcludeClipRect(hdc, 0, rc.bottom - app.lpui->dySBar, rc.right, rc.bottom);
    if (cpt == 4 && rgpt[2].x == rgpt[3].x && rgpt[2].y == rgpt[3].y) {
        cpt--;
        hpenSav = static_cast<HPEN>(win32::GetStockObject(win32::WHITE_PEN));
    } else {
        hpenSav = app.lpui->hpenShip;
    }
    for (i = 1; i < cpt; i++) {
        win32::ExcludeClipRect(hdc, rgpt[i].x - 5, rgpt[i].y - 5, rgpt[i].x + 6, rgpt[i].y + 6);
    }
    hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, hpenSav));
    iRopSav = win32::SetROP2(hdc, win32::R2_XORPEN);
    win32::MoveToEx(hdc, rgpt[2].x, rgpt[2].y, nullptr);
    win32::LineTo(hdc, rgpt->x, rgpt->y);
    if (cpt > 3) {
        win32::LineTo(hdc, rgpt[3].x, rgpt[3].y);
    }
    win32::SetROP2(hdc, iRopSav);
    win32::SelectObject(hdc, hpenSav);
    win32::SelectClipRgn(hdc, app.lpui->hrgnHuge);
    return;
}

std::int16_t SetScanWp(APPSESSION &app, std::int16_t iNew) {
    SCAN scan;

    if (iNew == app.state.sel.iwpAct) {
        return iNew;
    }
    FFindNearestObject(app, app.state.sel.fl.lpplord->rgord()[iNew].pt, grobjOther, &scan);
    scan.iwp = iNew;
    ChangeScanSel(app, &scan, 1);
    return iNew;
}

std::int16_t FGetNextObjHere(APPSESSION &app, SCAN *pscan, std::int16_t fOnlyOurs) {
    FLEET  *lpfl;
    std::int16_t i;
    std::int16_t fFound;

    fFound = app.state.sel.grobj != grobjFleet;
    for (i = 0; i < app.state.cFleet; i++) {
        lpfl = app.state.rglpfl[i];
        if (!app.state.rglpfl[i])
            break;
        if (!fFound) {
            if (lpfl->id == app.state.sel.id) {
                fFound = 1;
            }
        } else if (app.state.sel.pt.x == lpfl->pt.x && app.state.sel.pt.y == lpfl->pt.y && (!fOnlyOurs || lpfl->iPlayer == app.state.idPlayer)) {
            break;
        }
    }
    if (!fFound) {
        return 0;
    }
    if (i < app.state.cFleet) {
        pscan->ifl = i;
        pscan->grobj = grobjFleet;
    } else if ((app.state.sel.grobjFull & grobjPlanet) && app.state.sel.pl.iPlayer == app.state.idPlayer) {
        pscan->grobj = grobjPlanet;
        pscan->idpl = app.state.sel.pl.id;
    } else {
        for (i = 0; i < app.state.cFleet; i++) {
            lpfl = app.state.rglpfl[i];
            if (!app.state.rglpfl[i] || (pscan->pt.x == lpfl->pt.x && pscan->pt.y == lpfl->pt.y && (!fOnlyOurs || lpfl->iPlayer == app.state.idPlayer)))
                break;
        }
        if (i >= app.state.cFleet || lpfl->id == app.state.sel.id) {
            return 0;
        }
        pscan->ifl = i;
        pscan->grobj = grobjFleet;
    }
    return 1;
}

INT_PTR  FindDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    char szName[40];
    RECT rc;

    switch (msg) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyFindDlg, 1);
        ui::SendDlgItemMessage(hwnd, IDC_EDIT1, win32::EM_LIMITTEXT, 0x27, 0);
        return 1;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        if (msg == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            if (LOWORD(wParam) == IDOK) {
                win32::GetDlgItemTextA(hwnd, IDC_EDIT1, szName, 40);
                if (!FSelectSz(app, szName)) {
                    AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsSorryCantFindPlanetFleetName, {}).data(), win32::MB_ICONHAND);
                    ControlIO().pfnSetFocus(ControlIO().pfnGetDlgItem(hwnd, IDC_EDIT1));
                    ui::SendDlgItemMessage(hwnd, IDC_EDIT1, win32::EM_SETSEL, 0, -1);
                    return 0;
                }
            }
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyFindDlg, 0);
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhFindPlanetOrFleet);
            return 1;
        }
    }
    return 0;
}

std::int16_t FSelectSz(APPSESSION &app, char *szName) {
    char   *pch;
    std::int16_t ifl;
    FLEET  *lpfl;
    std::int16_t ipl;
    std::int16_t cch;
    char    szT[20];
    std::int16_t iplPartial;
    SCAN    scan;

    iplPartial = -1;
    scan.iwp = iwpNone;
    for (ipl = 0; ipl < app.state.game.cPlanMax && strcmpi(PszGetCompressedPlanet(app, app.state.rgidPlan[ipl]).data(), szName) != 0; ipl++) {
        if (iplPartial == -1 && strnicmp(PszGetCompressedPlanet(app, app.state.rgidPlan[ipl]).data(), szName, std::strlen(szName)) == 0) {
            iplPartial = ipl;
        }
    }
GoWithPartial:
    if (ipl != app.state.game.cPlanMax) {
        FFindNearestObject(app, app.state.rgptPlan[ipl], grobjPlanet, &scan);
        ChangeScanSel(app, &scan, 1);
        FEnsurePointOnScreen(app, scan.pt, 1);
        ControlIO().pfnUpdateWindow(app.lpui->hwndScanner);
        ControlIO().pfnSendMessage(app.lpui->hwndScanner, win32::WM_CHAR, 'v', 0);
        return 1;
    }

    cch = TextIO().pfnCchGetString(app, idsFleet, std::span{szT});
    if (strnicmp(szName, szT, cch) == 0) {
        pch = szName + 6;
    } else {
        pch = szName;
    }
    for (; *pch == ' '; pch++) {
    }
    if (*pch == '#') {
        pch++;
    }
    for (; *pch == ' '; pch++) {
    }
    if (*pch >= '1' && *pch <= '9') {
        ifl = *pch - '0';
        pch++;
        while (std::isdigit(*pch) != 0) {
            ifl = 10 * ifl + *pch - '0';
            pch++;
            if (ifl > 512)
                goto LNotAFleetId;
        }
        ifl--;
        ifl |= app.state.idPlayer << 9;
        lpfl = LpflFromId(app, ifl);
        if (lpfl) {
        LFoundFleetId:
            FFindNearestObject(app, lpfl->pt, grobjFleet, &scan);
            scan.ifl = IflFromLpfl(app, lpfl);
            ChangeScanSel(app, &scan, 2);
            FEnsurePointOnScreen(app, scan.pt, 1);
            ControlIO().pfnUpdateWindow(app.lpui->hwndScanner);
            ControlIO().pfnSendMessage(app.lpui->hwndScanner, win32::WM_CHAR, 'v', 0);
            return 1;
        }
    }
LNotAFleetId:
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (strcmpi(PszGetFleetName(app, lpfl->id).data(), szName) == 0)
            goto LFoundFleetId;
    }
    if (iplPartial == -1) {
        return 0;
    }
    ipl = iplPartial;
    goto GoWithPartial;
}

void GetScanFleetOrientation(APPSESSION &app, FLEET *lpfl, POINT16 *ppt, POINT16 *pptD) {
    std::int16_t dy;
    std::int16_t dx;

    if (lpfl->iPlayer == app.state.idPlayer) {
        if (lpfl->cord <= 1 || lpfl->lpplord->rgord()[1].iWarp == 0) {
        NoInfo:
            dx = dy = 0;
        } else {
            dx = lpfl->lpplord->rgord()[1].pt.x - lpfl->pt.x;
            dy = lpfl->lpplord->rgord()[1].pt.y - lpfl->pt.y;
        }
    } else {
        if (!lpfl->fdirValid() || lpfl->iwarpFlt() == 0)
            goto NoInfo;
        dx = lpfl->dirFltX() - 127;
        dy = lpfl->dirFltY() - 127;
    }
    GetDxDyOrientation(app, dx, dy, ppt, pptD);
    return;
}

void GetDxDyOrientation(APPSESSION &app, std::int16_t dx, std::int16_t dy, POINT16 *ppt, POINT16 *pptD) {
    double  dbl;
    std::int16_t iBmp;

    iBmp = 0;
    if (dx == 0 && dy == 0)
        goto LFinishUp;

    dbl = static_cast<double>(((static_cast<double>(std::atan2(static_cast<double>(dy), static_cast<double>(dx))) + 3.1415927) * 4.0 / 3.141592654 + 0.5));
    iBmp = 8 - (LOWORD(static_cast<std::int32_t>(dbl)) & 7);
    iBmp = (8 - (LOWORD(static_cast<std::int32_t>(dbl)) & 7) + 1) & 7;

LFinishUp:
    pptD->x = pptD->y = app.state.iScanZoom < zoom100 ? 7 : 9;
    if (app.state.iScanZoom >= zoom100) {
        ppt->x = 7;
    } else {
        ppt->x = 0;
    }
    ppt->y = iBmp * pptD->y;
    return;
}

std::int16_t FHandleMeasuringTape(APPSESSION &app, SCAN *pscan, POINT16 pt) {
    HDC     hdc;
    HPEN    hpenSav;
    SBAR    sbar;
    POINT16 ptLogLast;
    std::int16_t grTypeIn;
    POINT16 ptLogical;
    POINT16 ptBase;
    std::int16_t iropSav;
    POINT16 ptNew;
    char    szT[20];
    std::int16_t fVirgin;
    SCAN    scan;
    RECT    rc;

    fVirgin = 1;
    ptLogLast = pscan->pt;
    ptBase = pscan->pt;
    LogicalToScan(app, &ptBase);
    ControlIO().pfnGetClientRect(app.lpui->hwndScanner, &rc);
    rc.bottom -= app.lpui->dySBar;
    hdc = win32::GetDC(app.lpui->hwndScanner);
    win32::SetCapture(app.lpui->hwndScanner);
    sbar.pscan = pscan;
    hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, app.lpui->hpenShip));
    iropSav = win32::SetROP2(hdc, win32::R2_XORPEN);
    ptNew = pt;
    LogicalToScan(app, &ptNew);
    while (FGetRMouseMove(&ptNew)) {
        ptNew.x = (((0) > (StarsMin(rc.right, ptNew.x))) ? (0) : (StarsMin(rc.right, ptNew.x)));
        ptNew.y = (((0) > (StarsMin(rc.bottom, ptNew.y))) ? (0) : (StarsMin(rc.bottom, ptNew.y)));
        ptLogical = ptNew;
        ScanToLogical(app, &ptLogical);
        grTypeIn = !(ControlIO().pfnGetAsyncKeyState(win32::VK_SHIFT) & 0xfffe) ? 79 : 143;
        if (FFindNearestObject(app, ptLogical, static_cast<GrobjClass>(grTypeIn), &scan)) {
            ptLogical = scan.pt;
        }
        if (ptLogLast.x != ptLogical.x || ptLogLast.y != ptLogical.y) {
            ptNew = ptLogical;
            LogicalToScan(app, &ptNew);
            if (fVirgin) {
                if (std::abs(ptLogical.x - ptLogLast.x) < 3 && std::abs(ptLogical.y - ptLogLast.y) < 3)
                    continue;
                fVirgin = 0;
            } else {
                win32::MoveToEx(hdc, ptBase.x, ptBase.y, nullptr);
                win32::LineTo(hdc, pt.x, pt.y);
            }
            win32::MoveToEx(hdc, ptBase.x, ptBase.y, nullptr);
            win32::LineTo(hdc, ptNew.x, ptNew.y);
            if (scan.grobj == grobjNone) {
                sbar.id = -1;
                TextIO().pfnCchGetString(app, idsDeepSpace, std::span{szT});
                sbar.psz = szT;
            } else {
                if (scan.grobj == grobjPlanet) {
                    sbar.id = scan.idpl;
                } else if (scan.grobj == grobjFleet) {
                    sbar.id = app.state.rglpfl[scan.ifl]->id;
                } else if (scan.grobj == grobjThing) {
                    sbar.id = app.state.lpThings[scan.ith].idFull();
                } else {
                    sbar.id = scan.iwp;
                }
                sbar.psz = nullptr;
            }
            sbar.pt = ptLogical;
            sbar.grbit = scan.grobj;
            DrawScannerSBar(app, hdc, nullptr, &sbar, 0);
            pt = ptNew;
            ptLogLast = ptLogical;
        }
    }
    if (!fVirgin) {
        win32::MoveToEx(hdc, ptBase.x, ptBase.y, nullptr);
        win32::LineTo(hdc, pt.x, pt.y);
    }
    win32::SetROP2(hdc, iropSav);
    win32::SelectObject(hdc, hpenSav);
    DrawScannerSBar(app, hdc, nullptr, nullptr, 0);
    win32::ReleaseCapture();
    win32::ReleaseDC(app.lpui->hwndScanner, hdc);
    if (!fVirgin) {
        return 1;
    }
    return 0;
}

// ShowScanSel draws (fVis 1) or erases (0, or -1 to leave the main
// selection's marker) the scanner's selection markers.
void ShowScanSel(APPSESSION &app, std::int16_t fVis) {
    RedrawScanSel(app, nullptr, fVis);
    return;
}

// ShowScanSelChange redraws the scanner, orders and mineral views after
// ChangeScanSel moved the scanner selection from *pscanOld to *pscan.
void ShowScanSelChange(APPSESSION &app, SCAN *pscanOld, SCAN *pscan, std::int16_t fChgWp) {
    std::int16_t fMineFieldSel;
    RECT    rcMine;
    std::int16_t iRad;
    HDC     hdc;
    POINT16 ptTL; /* NATIVE: RECT corners are 32-bit; LogicalToScan takes POINT16 */
    POINT16 ptBR;

    fMineFieldSel = pscanOld->grobj == grobjThing && app.state.lpThings[pscanOld->ith].ith() == ithMinefield;
    if (fMineFieldSel) {
        iRad = LOWORD(static_cast<std::int32_t>((static_cast<double>(std::sqrt(static_cast<double>(app.state.lpThings[pscanOld->ith].thm().cMines()))) + 1.0)));
        rcMine.left = app.state.lpThings[pscanOld->ith].pt().x();
        rcMine.top = app.state.lpThings[pscanOld->ith].pt().y();
        rcMine.right = rcMine.left + iRad;
        rcMine.bottom = rcMine.top - iRad;
        rcMine.left -= iRad;
        rcMine.top += iRad;
        /* NATIVE: original passed (POINT16 *)&rcMine.left and &rcMine.right */
        ptTL.x = rcMine.left;
        ptTL.y = rcMine.top;
        LogicalToScan(app, &ptTL);
        rcMine.left = ptTL.x;
        rcMine.top = ptTL.y;
        ptBR.x = rcMine.right;
        ptBR.y = rcMine.bottom;
        LogicalToScan(app, &ptBR);
        rcMine.right = ptBR.x;
        rcMine.bottom = ptBR.y;
        win32::InflateRect(&rcMine, 1, 1);
    }
    if (fChgWp) {
        FillOrdersLB(app);
        SetOrdersLbSel(app, pscan->iwp);
        ControlIO().pfnUpdateOrdersDDs(app, 0);
        ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(0x122));
    }
    RedrawScanSel(app, nullptr, 1);
    if (fChgWp) {
        FEnsurePointOnScreen(app, pscan->pt, 1);
    }
    DrawScannerSBar(app, nullptr, nullptr, nullptr, 0);
    ControlIO().pfnInvalidateRect(app.lpui->hwndMine, nullptr, 1);
    SetMineralTitleBar(app, app.lpui->hwndMine);
    if (fMineFieldSel) {
        hdc = win32::GetDC(app.lpui->hwndScanner);
        DrawScanner(app, hdc, &rcMine);
        win32::ReleaseDC(app.lpui->hwndScanner, hdc);
    }
    fMineFieldSel = app.state.sel.scan.grobj == grobjThing && app.state.lpThings[app.state.sel.scan.ith].ith() == ithMinefield;
    if (fMineFieldSel) {
        iRad = LOWORD(static_cast<std::int32_t>((static_cast<double>(std::sqrt(static_cast<double>(app.state.lpThings[app.state.sel.scan.ith].thm().cMines()))) + 1.0)));
        rcMine.left = app.state.lpThings[app.state.sel.scan.ith].pt().x();
        rcMine.top = app.state.lpThings[app.state.sel.scan.ith].pt().y();
        rcMine.right = rcMine.left + iRad;
        rcMine.bottom = rcMine.top - iRad;
        rcMine.left -= iRad;
        rcMine.top += iRad;
        /* NATIVE: original passed (POINT16 *)&rcMine.left and &rcMine.right */
        ptTL.x = rcMine.left;
        ptTL.y = rcMine.top;
        LogicalToScan(app, &ptTL);
        rcMine.left = ptTL.x;
        rcMine.top = ptTL.y;
        ptBR.x = rcMine.right;
        ptBR.y = rcMine.bottom;
        LogicalToScan(app, &ptBR);
        rcMine.right = ptBR.x;
        rcMine.bottom = ptBR.y;
        win32::InflateRect(&rcMine, 1, 1);
    }
    if (fMineFieldSel) {
        hdc = win32::GetDC(app.lpui->hwndScanner);
        DrawScanner(app, hdc, &rcMine);
        win32::ReleaseDC(app.lpui->hwndScanner, hdc);
    }
    if (app.state.sel.pl.id != idplNone) {
        ControlIO().pfnDrawPlanShip(app, nullptr, static_cast<TileBits>(0x4002));
    }
    if (app.state.gd.fTutorial && app.state.idPlayer == 0) {
        PresentationIO().pfnTutor(app);
    }
    return;
}

// ShowSelAt scrolls the scanner to a newly selected object at pt and
// updates the scanner status bar and the mineral window.
void ShowSelAt(APPSESSION &app, POINT16 pt) {
    CtrPointScan(app, pt, 1);
    DrawScannerSBar(app, nullptr, nullptr, nullptr, 0);
    ControlIO().pfnInvalidateRect(app.lpui->hwndMine, nullptr, 1);
    SetMineralTitleBar(app, app.lpui->hwndMine);
    return;
}

void ResetScannerInputSession() noexcept { dWheelZoom = dWheelScroll = fPanDown = fPanning = 0; ptPan = POINT16{}; xPanTop = yPanTop = 0; }
