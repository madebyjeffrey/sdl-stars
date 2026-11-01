#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"
#include "colony_io.h"
#include "planet_query_io.h"
#include "research_io.h"
#include "part_query_io.h"
#include "alert_io.h"
#include "capacity_io.h"

// Wheel deltas short of a notch, for rows and for columns.
static std::int16_t dWheelRows;
static std::int16_t dWheelCols;

static void SetReportScrollInfo(HWND hwnd, int iMax, int cPage, int iPos, BOOL fRedraw) {
    SCROLLINFO si = {sizeof(si), win32::SIF_RANGE | win32::SIF_PAGE | win32::SIF_POS, 0, 0, 0, 0, 0};

    /* Legacy report ranges describe the last row/column position. */
    si.nPage = static_cast<UINT>(StarsMax(1, cPage));
    si.nMax = StarsMax(0, iMax) + static_cast<int>(si.nPage) - 1;
    si.nPos = iPos;
    win32::SetScrollInfo(hwnd, win32::SB_CTL, &si, fRedraw);
}

LRESULT  ReportDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HDC         hdc;
    HMENU       hmenu;
    RECT        rc;
    std::int16_t     i;
    std::int16_t     dx;
    std::uint16_t    swp;
    std::int16_t     cRow;
    POINT16     pt;
    std::int16_t     ibit;
    std::int16_t     iCol;
    std::int16_t     iRow;
    std::int16_t     xCur;
    std::int16_t     iCur;
    std::int16_t     iNew;
    PAINTSTRUCT ps;
    MessageId   idm;

    switch (msg) {
    case win32::WM_CREATE:
        app.lpui->hwndReportDlg = hwnd;
        hdc = win32::GetDC(hwnd);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        for (i = 0; i < app.lpui->vprptCur->cFields; i++) {
            dx = DxReportColHdr(app, app.lpui->vprptCur->irpt, i, app.state.szWork, hdc);
            app.lpui->vprptCur->rgbdx[i] = dx / 2;
        }
        win32::ReleaseDC(hwnd, hdc);
        SortReportCache(app, app.lpui->vprptCur->irpt, app.lpui->vprptCur->icolSort);
        ControlIO().pfnSetWindowPos(hwnd, nullptr, 0, 0, app.lpui->vprptCur->ptSize.x, app.lpui->vprptCur->ptSize.y, win32::SWP_NOMOVE | win32::SWP_NOZORDER | win32::SWP_NOREDRAW);
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->vprptCur->ptDlg, 1);
        app.lpui->vprptCur->hwndVScroll = ui::CreateWindowEx(0, "SCROLLBAR", nullptr, win32::SBS_VERT | win32::WS_CHILD, 0, 0, 50, 50, hwnd, nullptr, app.lpui->hInst, nullptr);
        app.lpui->vprptCur->hwndHScroll = ui::CreateWindowEx(0, "SCROLLBAR", nullptr, win32::WS_CHILD, 0, 0, 50, 50, hwnd, nullptr, app.lpui->hInst, nullptr);
        if (app.state.gd.fTutorial) {
            PresentationIO().pfnTutor(app);
        }
        /* fallthrough */
    case win32::WM_SIZE:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        cRow = static_cast<std::int16_t>((rc.bottom - 36)) / (app.state.dyArial8 + 4);
        app.lpui->vprptCur->cRowsVis = cRow >= app.lpui->vprptCur->cRows ? app.lpui->vprptCur->cRows : cRow;
        if (app.lpui->vprptCur->cRowsVis >= app.lpui->vprptCur->cRows) {
            swp = win32::SWP_NOZORDER | win32::SWP_HIDEWINDOW;
            app.lpui->vprptCur->irowFirst = 0;
            win32::SetScrollPos(app.lpui->vprptCur->hwndVScroll, win32::SB_CTL, 0, 0);
        } else {
            swp = win32::SWP_NOZORDER | win32::SWP_SHOWWINDOW;
            if (app.lpui->vprptCur->irowFirst + app.lpui->vprptCur->cRowsVis > app.lpui->vprptCur->cRows && app.lpui->vprptCur->irowFirst > 0) {
                app.lpui->vprptCur->irowFirst = app.lpui->vprptCur->cRows - app.lpui->vprptCur->cRowsVis;
                if (app.lpui->vprptCur->irowFirst < 0) {
                    app.lpui->vprptCur->irowFirst = 0;
                }
            }
            SetReportScrollInfo(app.lpui->vprptCur->hwndVScroll, app.lpui->vprptCur->cRows - app.lpui->vprptCur->cRowsVis,
                                app.lpui->vprptCur->cRowsVis, app.lpui->vprptCur->irowFirst, 1);
        }
        dx = win32::GetSystemMetrics(win32::SM_CXVSCROLL);
        ControlIO().pfnSetWindowPos(app.lpui->vprptCur->hwndVScroll, nullptr, rc.right - dx, app.state.dyArial8 + 6, dx, (app.state.dyArial8 + 4) * app.lpui->vprptCur->cRowsVis + 1, swp);
        SetHScrollBar(app);
        if (msg == win32::WM_CREATE) {
            return 1;
        }
        return 0;
    case win32::WM_GETMINMAXINFO:
        (reinterpret_cast<MINMAXINFO *>(lParam))->ptMinTrackSize.x = 300;
        (reinterpret_cast<MINMAXINFO *>(lParam))->ptMinTrackSize.y = 220;
        return 0;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_LBUTTONDOWN:
    case win32::WM_LBUTTONDBLCLK:
    case win32::WM_RBUTTONDOWN:
        xCur = 2;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        if (pt.y < 2 || pt.x < 2)
            break;
        if (pt.y < app.state.dyArial8 + 6) {
            iRow = -1;
        } else {
            iRow = static_cast<std::int16_t>((pt.y - 2 - (app.state.dyArial8 + 4))) / (app.state.dyArial8 + 4);
            if (iRow >= app.lpui->vprptCur->cRowsVis)
                break;
            iRow += app.lpui->vprptCur->irowFirst;
        }
        iCol = -1;
        i = 0;
        ibit = 1;
        while (i < app.lpui->vprptCur->cFields) {
            if ((ibit & app.lpui->vprptCur->grbitVisible) && (i == 0 || i >= app.lpui->vprptCur->cFieldFirst)) {
                xCur += app.lpui->vprptCur->rgbdx[i] * 2;
                if (xCur > pt.x) {
                    iCol = i;
                    break;
                }
            }
            i++;
            ibit *= 2;
        }
        if (iCol == -1)
            break;
        if (iRow == -1) {
            ReportColumnPopup(app, pt, iCol, msg == win32::WM_RBUTTONDOWN);
        } else {
            ExecuteReportClick(app, pt, app.lpui->vprptCur->irpt, iCol, iRow);
        }
        if (app.state.gd.fTutorial) {
            PresentationIO().pfnTutor(app);
        }
        break;
    case win32::WM_MOUSEWHEEL:
    case win32::WM_MOUSEHWHEEL:
        // The wheel scrolls rows, as many a notch as the player's mouse
        // settings say. The tilt wheel and Shift+wheel scroll a column a
        // notch, wheel down to the right. Both go through the scroll bars'
        // paths, which clamp and redraw.
        i = win32::GET_WHEEL_DELTA_WPARAM(wParam);
        if (msg == win32::WM_MOUSEWHEEL && !(win32::GET_KEYSTATE_WPARAM(wParam) & win32::MK_SHIFT)) {
            cRow = CWheelNotches(&dWheelRows, i);
            if (cRow != 0) {
                iNew = win32::GetScrollPos(app.lpui->vprptCur->hwndVScroll, win32::SB_CTL) - cRow * CWheelLines(app.lpui->vprptCur->cRowsVis - 1);
                ControlIO().pfnSendMessage(hwnd, win32::WM_VSCROLL, win32::MAKEWPARAM(win32::SB_THUMBPOSITION, static_cast<WORD>(iNew)), reinterpret_cast<LPARAM>(app.lpui->vprptCur->hwndVScroll));
            }
            return 0;
        }
        iCol = CWheelNotches(&dWheelCols, msg == win32::WM_MOUSEWHEEL ? -i : i);
        if (iCol != 0) {
            iNew = win32::GetScrollPos(app.lpui->vprptCur->hwndHScroll, win32::SB_CTL) + iCol;
            ControlIO().pfnSendMessage(hwnd, win32::WM_HSCROLL, win32::MAKEWPARAM(win32::SB_THUMBPOSITION, static_cast<WORD>(iNew)), reinterpret_cast<LPARAM>(app.lpui->vprptCur->hwndHScroll));
        }
        return 0;
    case win32::WM_VSCROLL:
        iCur = win32::GetScrollPos(reinterpret_cast<HWND>(lParam), win32::SB_CTL);
        iNew = iCur;
        if (LOWORD(wParam) <= win32::SB_BOTTOM) {
            switch (LOWORD(wParam)) {
            case win32::SB_BOTTOM:
                iNew = 2000;
                break;
            case win32::SB_LINEDOWN:
                iNew++;
                break;
            case win32::SB_LINEUP:
                iNew--;
                break;
            case win32::SB_PAGEDOWN:
                iNew += app.lpui->vprptCur->cRowsVis - 1;
                break;
            case win32::SB_PAGEUP:
                iNew -= app.lpui->vprptCur->cRowsVis - 1;
                break;
            case win32::SB_THUMBPOSITION:
            case win32::SB_THUMBTRACK:
                iNew = static_cast<short>(HIWORD(wParam));
                break;
            case win32::SB_TOP:
                iNew = 0;
            }
        }
        if (iNew > app.lpui->vprptCur->cRows - app.lpui->vprptCur->cRowsVis) {
            iNew = app.lpui->vprptCur->cRows - app.lpui->vprptCur->cRowsVis;
        }
        if (iNew < 0) {
            iNew = 0;
        }
        if (iNew != iCur) {
            app.lpui->vprptCur->irowFirst = iNew;
            ControlIO().pfnGetClientRect(hwnd, &rc);
            rc.left = 2;
            rc.right -= win32::GetSystemMetrics(win32::SM_CXVSCROLL);
            rc.top = app.state.dyArial8 + 6;
            rc.bottom = (app.state.dyArial8 + 4) * app.lpui->vprptCur->cRowsVis + rc.top;
            win32::ScrollWindow(hwnd, 0, (app.state.dyArial8 + 4) * (iCur - iNew), &rc, &rc);
            win32::SetScrollPos(reinterpret_cast<HWND>(lParam), win32::SB_CTL, iNew, 1);
            ControlIO().pfnUpdateWindow(hwnd);
        }
        return 0;
    case win32::WM_HSCROLL:
        iCur = win32::GetScrollPos(reinterpret_cast<HWND>(lParam), win32::SB_CTL);
        iNew = iCur;
        if (LOWORD(wParam) <= win32::SB_BOTTOM) {
            switch (LOWORD(wParam)) {
            case win32::SB_BOTTOM:
                iNew = 2000;
                break;
            case win32::SB_LINEDOWN:
                iNew++;
                break;
            case win32::SB_LINEUP:
                iNew--;
                break;
            case win32::SB_PAGEDOWN:
                iNew += 3;
                break;
            case win32::SB_PAGEUP:
                iNew -= 3;
                break;
            case win32::SB_THUMBPOSITION:
            case win32::SB_THUMBTRACK:
                iNew = static_cast<short>(HIWORD(wParam));
                break;
            case win32::SB_TOP:
                iNew = 0;
            }
        }
        if (iNew > app.lpui->vprptCur->cColScroll) {
            iNew = app.lpui->vprptCur->cColScroll;
        }
        if (iNew < 0) {
            iNew = 0;
        }
        if (iNew != iCur) {
            win32::SetScrollPos(reinterpret_cast<HWND>(lParam), win32::SB_CTL, iNew, 1);
            iNew = win32::GetScrollPos(reinterpret_cast<HWND>(lParam), win32::SB_CTL);
            if (iNew != iCur) {
                i = 1;
                for (ibit = 2; i < app.lpui->vprptCur->cFields && (!(ibit & app.lpui->vprptCur->grbitVisible) || iNew-- > 0); ibit *= 2) {
                    i++;
                }
                app.lpui->vprptCur->cFieldFirst = i;
                ControlIO().pfnInvalidateRect(hwnd, nullptr, 1);
                ControlIO().pfnUpdateWindow(hwnd);
            }
        }
        return 0;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        DrawReport(app, hwnd, hdc, &ps.rcPaint);
        win32::EndPaint(hwnd, &ps);
        app.state.gd.fRptSafeDraw = 0;
        return 1;
    case win32::WM_DESTROY:
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->vprptCur->ptDlg, 0);
        win32::GetWindowRect(hwnd, &rc);
        app.lpui->vprptCur->ptSize.x = rc.right - rc.left;
        app.lpui->vprptCur->ptSize.y = rc.bottom - rc.top;
        app.lpui->hwndReportDlg = nullptr;
        app.lpui->fBrowserValid = 0;
        hmenu = GetASubMenu(app, app.lpui->hwndFrame, menuReport);
        switch (app.lpui->vprptCur->irpt) {
        case rptFleets:
            idm = static_cast<MessageId>(2303);
            break;
        case rptEnemyFleets:
            idm = static_cast<MessageId>(2304);
            break;
        case rptPlanets:
            idm = static_cast<MessageId>(2301);
            break;
        case rptBattles:
            idm = static_cast<MessageId>(2305);
        }
        win32::CheckMenuItem(hmenu, idm, win32::MF_UNCHECKED);
        app.lpui->vprptCur = nullptr;
        if (app.state.gd.fTutorial) {
            PresentationIO().pfnTutor(app);
        }
        break;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) {
            ControlIO().pfnDestroyWindow(hwnd);
            return 1;
        }
        break;
    }
    return ui::DefWindowProc(hwnd, msg, wParam, lParam);
}

void SetHScrollBar(APPSESSION &app) {
    std::uint16_t swp;
    std::int16_t  dy;
    std::int16_t  ccolSkipped;
    std::int16_t  ccolHidden;
    std::int16_t  ccolVisible;
    std::int16_t  i;
    std::int16_t  ibit;
    std::int16_t  xRight;
    std::int16_t  dx;
    std::int16_t  xTitle;
    RECT     rc;

    ControlIO().pfnGetClientRect(app.lpui->hwndReportDlg, &rc);
    dx = win32::GetSystemMetrics(win32::SM_CXVSCROLL);
    xTitle = app.lpui->vprptCur->rgbdx[0] * 2 + 2;
    xRight = rc.right - xTitle - 2 - dx;
    ccolSkipped = 0;
    ccolHidden = 0;
    ccolVisible = 0;
    i = app.lpui->vprptCur->cFields - 1;
    ibit = 1 << app.lpui->vprptCur->cFields;
    while (i > 0) {
        if (ibit & app.lpui->vprptCur->grbitVisible) {
            ccolVisible++;
            if (i < app.lpui->vprptCur->cFieldFirst) {
                ccolSkipped++;
            }
            xRight -= app.lpui->vprptCur->rgbdx[i] * 2;
            if (xRight < 0) {
                ccolHidden++;
            }
        }
        i--;
        ibit >>= 1;
    }
    app.lpui->vprptCur->cColScroll = 0;
    if (ccolHidden == 0) {
        swp = win32::SWP_NOZORDER | win32::SWP_HIDEWINDOW;
        app.lpui->vprptCur->cFieldFirst = 1;
        win32::SetScrollPos(app.lpui->vprptCur->hwndHScroll, win32::SB_CTL, 0, 0);
    } else {
        swp = win32::SWP_NOZORDER | win32::SWP_SHOWWINDOW;
        if (ccolSkipped > ccolHidden) {
            ccolSkipped = 0;
            app.lpui->vprptCur->cFieldFirst = 1;
        }
        SetReportScrollInfo(app.lpui->vprptCur->hwndHScroll, ccolHidden, ccolVisible - ccolHidden, ccolSkipped, 1);
        app.lpui->vprptCur->cColScroll = ccolHidden;
    }
    dy = win32::GetSystemMetrics(win32::SM_CYHSCROLL);
    ControlIO().pfnSetWindowPos(app.lpui->vprptCur->hwndHScroll, nullptr, xTitle, app.state.dyArial8 + 6 + (app.state.dyArial8 + 4) * app.lpui->vprptCur->cRowsVis + 1, rc.right - dx - xTitle, dy, swp);
    return;
}

void DrawReport(APPSESSION &app, HWND hwnd, HDC hdc, RECT *prc) {
    char    szTit[40];
    std::int16_t irowLast;
    std::int16_t j;
    std::int16_t i;
    std::int16_t yRow;
    std::int16_t ibit;
    std::int16_t dx;
    std::int16_t xCol;
    RECT    rc;

    ibit = 1;
    xCol = 2;
    yRow = 2;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    win32::SetBkMode(hdc, win32::TRANSPARENT);
    if (prc->top > app.state.dyArial8 + 4 + yRow)
        goto NoHdrDraw;

    i = 0;
    while (i < app.lpui->vprptCur->cFields) {
        if ((ibit & app.lpui->vprptCur->grbitVisible) && (i == 0 || i >= app.lpui->vprptCur->cFieldFirst)) {
            dx = DxReportColHdr(app, app.lpui->vprptCur->irpt, i, szTit, hdc);
            app.lpui->vprptCur->rgbdx[i] = dx / 2;
            win32::SetRect(&rc, xCol, yRow, xCol + dx - 1, app.state.dyArial8 + 4 + yRow);
            if (app.state.gd.fRptSafeDraw) {
                win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
            }
            if (i == 0) {
                win32::TextOutA(hdc, rc.left + 3, rc.top + 2, szTit, std::strlen(szTit));
            } else {
                CtrTextOut(hdc, static_cast<std::int16_t>((rc.right - rc.left)) / 2 + rc.left, rc.top + 2, szTit, 0);
            }
            _Draw3dFrame(app, hdc, &rc, 0);
            xCol += dx;
        }
        i++;
        ibit *= 2;
    }

NoHdrDraw:
    irowLast = app.lpui->vprptCur->irowFirst + app.lpui->vprptCur->cRowsVis;
    if (irowLast > app.lpui->vprptCur->cRows) {
        irowLast = app.lpui->vprptCur->cRows;
    }
    for (i = app.lpui->vprptCur->irowFirst; i < irowLast; i++) {
        yRow += app.state.dyArial8 + 4;
        xCol = 2;
        if (yRow >= prc->top && yRow <= prc->bottom) {
            win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
            win32::PatBlt(hdc, xCol, yRow, 1, app.state.dyArial8 + 4, win32::PATCOPY);
        }
        j = 0;
        ibit = 1;
        while (j < app.lpui->vprptCur->cFields) {
            if ((ibit & app.lpui->vprptCur->grbitVisible) && (j == 0 || j >= app.lpui->vprptCur->cFieldFirst)) {
                dx = app.lpui->vprptCur->rgbdx[j] * 2;
                if (yRow >= prc->top - (app.state.dyArial8 + 4) && yRow <= prc->bottom) {
                    win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
                    win32::PatBlt(hdc, xCol + dx - 1, yRow, 1, app.state.dyArial8 + 4, win32::PATCOPY);
                    win32::PatBlt(hdc, xCol, app.state.dyArial8 + 4 + yRow, dx, 1, win32::PATCOPY);
                    win32::SetRect(&rc, xCol + 2, yRow + 2, xCol + dx - 3, app.state.dyArial8 + 4 + yRow - 1);
                    if (app.state.gd.fRptSafeDraw) {
                        win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
                    }
                    DrawReportItem(app, hdc, &rc, app.lpui->vprptCur->irpt, i, j);
                }
                xCol += dx;
            }
            j++;
            ibit *= 2;
        }
    }
    return;
}

INT_PTR  ScoreXDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    std::int16_t     i;
    RECT        rc;
    HDC         hdc;
    PAINTSTRUCT ps;
    POINT16     pt;
    char        szT[40];
    std::int16_t     cchHistory;
    char       *rgszScan[20];
    std::int16_t     c;
    std::int32_t     rgid[12];
    char       *psz;
    std::int16_t     iSel;
    std::int16_t     cch;

    switch (message) {
    case win32::WM_INITDIALOG:
        InitScoreDlg(app, hwnd, app.state.gd.fScoreVictory);
        app.lpui->fInScoreDialog = 1;
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyScoreXDlg, 1);
        app.lpui->hwndScoreXDlg = hwnd;
        if (app.state.gd.fTutorial) {
            PresentationIO().pfnTutor(app);
        }
        return 1;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        if (app.state.gd.fScoreVictory == 2) {
            DrawHistoryReport(app, hdc);
        } else if (app.state.gd.fScoreVictory) {
            DrawVCReport(app, hdc);
        } else {
            DrawScoreReport(app, hdc);
        }
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_SETCURSOR:
        if (app.state.gd.fScoreVictory != 2) {
            return 0;
        }
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (pt.y >= app.lpui->dyArial10 + app.state.dyArial8 - 2 || pt.y <= 2) {
            return 0;
        }
        win32::SetCursor(app.lpui->hcurHand);
        return 1;
    case win32::WM_LBUTTONDOWN:
    case win32::WM_RBUTTONDOWN:
        c = 0;
        if (app.state.gd.fScoreVictory != 2 || HIWORD(lParam) <= 2 || HIWORD(lParam) >= static_cast<std::uint16_t>((app.lpui->dyArial10 + app.state.dyArial8 - 2))) {
            return 0;
        }
        cchHistory = TextIO().pfnCchGetString(app, idsHistory, std::span{szT});
        for (i = 0; i < 8; i++) {
            CchCopyUtf8Prefix(std::span{app.state.szWork}.subspan(static_cast<std::size_t>(i) * 40, 40), szT);
            psz = &app.state.szWork[i * 40 + cchHistory];
            cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsPlanets + i), std::span{app.state.szWork}.subspan(static_cast<std::size_t>(i) * 40, 40).subspan(static_cast<std::size_t>(cchHistory)));
            if (cch > 0) psz[CchUtf8Prefix(std::string_view(psz, cch), cch - 1)] = 0;
            rgid[c] = static_cast<std::uint32_t>((app.state.gd.iCurGraph == i));
            rgszScan[c++] = &app.state.szWork[i * 40];
        }
        iSel = ControlIO().pfnPopupMenu(app, hwnd, LOWORD(lParam), HIWORD(lParam), c, rgid, rgszScan, -2, 0);
        if (iSel == -1) {
            return 0;
        }
        app.state.gd.iCurGraph = iSel;
        app.state.gd.fChgReports = 1;
        ControlIO().pfnInvalidateRect(hwnd, nullptr, 1);
        return 0;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyScoreXDlg, 0);
            ControlIO().pfnEndDialog(hwnd, i);
            app.lpui->fInScoreDialog = 0;
            app.lpui->hwndScoreXDlg = nullptr;
            if (app.state.gd.fTutorial) {
                PresentationIO().pfnTutor(app);
            }
            return 1;
        case IDC_HELP:
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhScoreSheet);
            return 1;
        case IDC_SCORE_SWITCH:
            app.state.gd.fScoreVictory = static_cast<std::uint32_t>((app.state.gd.fScoreVictory + 1)) % 3;
            ControlIO().pfnInvalidateRect(hwnd, nullptr, 1);
            InitScoreDlg(app, hwnd, app.state.gd.fScoreVictory);
            if (app.state.gd.fTutorial) {
                PresentationIO().pfnTutor(app);
            }
        }
        /* fallthrough */
    default:
        return 0;
    }
}

void InitScoreDlg(APPSESSION &app, HWND hwnd, std::int16_t fVictory) {
    std::array<char, cchCatalogText> szCatalogPsz{};
    HDC     hdc;
    std::int16_t dxDig;
    std::int16_t dy;
    std::int16_t dyFrame;
    std::int16_t dxFrame;
    RECT    rcWindow;
    char   *psz;
    std::int16_t dx;
    RECT    rc;

    hdc = win32::GetDC(hwnd);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    dxDig = LOWORD(GetTextExtent(hdc, "9", 1));
    if (fVictory == 2) {
        dx = 600;
        dy = 400;
    } else if (fVictory) {
        szCatalogPsz = TextIO().pfnCompressed(app, idsExceedsSecondPlaceScore);
        psz = szCatalogPsz.data();
        app.lpui->vdxScoreX = static_cast<std::uint32_t>((LOWORD(GetTextExtent(hdc, psz, std::strlen(psz))) * 3)) / 2 + 6 * dxDig;
        dx = static_cast<std::int16_t>(((4 <= app.state.game.cPlayer ? app.state.game.cPlayer : 4) * app.state.dyArial8 * 3)) / 2 + app.lpui->vdxScoreX + 8;
        dy = static_cast<std::int16_t>((11 * app.state.dyArial8 * 3)) / 2 + 88;
    } else {
        szCatalogPsz = TextIO().pfnCompressed(app, idsUnarmedShips2);
        psz = szCatalogPsz.data();
        app.lpui->vdxScoreX = LOWORD(GetTextExtent(hdc, psz, std::strlen(psz))) + 8;
        dx = (4 <= app.state.game.cPlayer ? app.state.game.cPlayer : 4) * dxDig * 5 + app.lpui->vdxScoreX + 8;
        dy = static_cast<std::int16_t>((11 * app.state.dyArial8 * 3)) / 2 + 88;
    }
    win32::ReleaseDC(hwnd, hdc);
    win32::GetWindowRect(hwnd, &rcWindow);
    ControlIO().pfnGetClientRect(hwnd, &rc);
    dxFrame = rcWindow.right - rcWindow.left - rc.right;
    dyFrame = rcWindow.bottom - rcWindow.top - rc.bottom;
    ControlIO().pfnSetWindowPos(hwnd, nullptr, 0, 0, dxFrame + dx, dyFrame + dy, win32::SWP_NOMOVE | win32::SWP_NOZORDER);
    win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDCANCEL), &rc);
    win32::MapWindowPoints(nullptr, hwnd, reinterpret_cast<POINT *>(&rc), 2);
    win32::OffsetRect(&rc, 0, dy - 4 - rc.bottom);
    dx = static_cast<std::int16_t>((dx - (rc.right - rc.left) * 3)) / 4;
    ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_SCORE_SWITCH), nullptr, dx, rc.top, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER);
    ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDCANCEL), nullptr, dx * 2 + (rc.right - rc.left), rc.top, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER);
    ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_HELP), nullptr, 3 * dx + (rc.right - rc.left) * 2, rc.top, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER);
    ControlIO().pfnSetWindowText(hwnd, TextIO().pfnCompressed(app, static_cast<StringId>(idsPlayerScores + fVictory)).data());
    return;
}

void DrawVCReport(APPSESSION &app, HDC hdc) {
    std::array<char, 50> szPlayerName{};
    std::int16_t  grbitVC;
    std::int16_t  xStart;
    std::int16_t  dxDig;
    std::int16_t  yTop;
    POINT16  pt;
    std::int16_t  cCurSav;
    StringId ids;
    COLORREF cr;
    std::int16_t  cCur;
    HDC      hdcMem;
    std::int16_t  j;
    std::int16_t  i;
    std::int16_t  iPass;
    char    *psz;
    HBITMAP  hbmpSav;
    std::int16_t  cch;
    std::int16_t  xLeft;
    std::int32_t  l;
    StringId idsT;
    std::int16_t  vcVal;

    yTop = 88;
    win32::SetBkMode(hdc, win32::TRANSPARENT);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[4]);
    xLeft = app.lpui->vdxScoreX;
    for (i = 0; i < app.state.game.cPlayer; i++) {
        szPlayerName = TextIO().pfnPlayerName(app, i, 1, 1, 1, 0, nullptr);
        psz = szPlayerName.data();
        cch = std::strlen(psz);
        l = GetTextExtent(hdc, psz, cch);
        dxDig = LOWORD(static_cast<std::int32_t>((static_cast<double>(static_cast<std::uint32_t>(LOWORD(l))) / 1.4142)));
        if (app.state.rgplr[i].fInclude && app.state.rgplr[i].fDead) {
            cr = 8355711;
        } else if (app.state.vlprgScoreX[i].fWinner) {
            cr = 16711680;
        } else {
            cr = 0;
        }
        win32::SetTextColor(hdc, cr);
        win32::TextOutA(hdc, xLeft - dxDig + static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2, yTop - dxDig - app.state.dyArial8 / 2 - 4, psz, cch);
        xLeft += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
    }
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
    dxDig = LOWORD(GetTextExtent(hdc, "9", 1));
    ids = idsOwns;
    cCur = 0;
    hdcMem = win32::CreateCompatibleDC(hdc);
    hbmpSav = static_cast<HBITMAP>(win32::SelectObject(hdcMem, app.lpui->hbmpMono));
    win32::SetTextColor(hdc, 0);
    win32::SetBkColor(hdc, 0xffffff);
    xLeft = app.lpui->vdxScoreX + 4;
    pt.x = static_cast<std::int16_t>((static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 - 12)) / 2;
    pt.y = static_cast<std::int16_t>((static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 - 11)) / 2 - app.state.dyArial8 / 4;
    for (i = 0; i < app.state.game.cPlayer; i++) {
        grbitVC = app.state.vlprgScoreX[i].grbitVC;
        for (j = 0; j < 7; j++, grbitVC >>= 1) {
            if (grbitVC & 1) {
                win32::BitBlt(hdc, xLeft + pt.x, yTop + pt.y + static_cast<std::int16_t>((j * app.state.dyArial8 * 3)) / 2, 14, 12, hdcMem, 0, 0, win32::SRCAND);
                if ((app.state.rgplr[i].fInclude && app.state.rgplr[i].fDead) || GetVCCheck(&app.state.game, static_cast<VictoryCondition>((j > 1) + j)) == 0) {
                    win32::SetTextColor(hdc, app.lpui->crButtonShadow);
                LOrIt:
                    win32::SetBkColor(hdc, 0);
                    win32::BitBlt(hdc, xLeft + pt.x, yTop + pt.y + static_cast<std::int16_t>((j * app.state.dyArial8 * 3)) / 2, 14, 12, hdcMem, 0, 0, win32::SRCPAINT);
                    win32::SetTextColor(hdc, 0);
                    win32::SetBkColor(hdc, 0xffffff);
                } else if (app.state.vlprgScoreX[i].fWinner) {
                    win32::SetTextColor(hdc, 16711680);
                    goto LOrIt;
                }
            }
        }
        win32::PatBlt(hdc, xLeft, yTop - app.state.dyArial8 / 4, 1, static_cast<std::int16_t>((7 * app.state.dyArial8 * 3)) / 2 + 1, win32::PATCOPY);
        xLeft += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
    }
    win32::PatBlt(hdc, xLeft, yTop - app.state.dyArial8 / 4, 1, static_cast<std::int16_t>((7 * app.state.dyArial8 * 3)) / 2 + 1, win32::PATCOPY);
    win32::SelectObject(hdcMem, hbmpSav);
    win32::DeleteDC(hdcMem);
    for (i = 0; i < 9; i++) {
        for (iPass = 0; iPass < 2; iPass++) {
            if (iPass == 0) {
                cCurSav = cCur;
                xStart = 0;
            } else {
                ids = static_cast<StringId>(ids - 3);
                cCur = cCurSav;
                if (i < 7) {
                    xStart = app.lpui->vdxScoreX - xLeft;
                } else {
                    xStart = 8;
                }
            }
            xLeft = xStart;
            if (i <= 7 && iPass == 1) {
                win32::PatBlt(hdc, app.lpui->vdxScoreX + 4, yTop - app.state.dyArial8 / 4, static_cast<std::int16_t>((app.state.dyArial8 * app.state.game.cPlayer * 3)) / 2, 1, win32::PATCOPY);
                if (i == 7) {
                    yTop += app.state.dyArial8 / 2;
                }
            }
            cch = TextIO().pfnCchGetString(app, ids, std::span{app.state.szWork});
            ids = static_cast<StringId>(ids + 1);
            if (iPass == 1) {
                win32::SetTextColor(hdc, i >= 7 || GetVCCheck(&app.state.game, static_cast<VictoryCondition>(cCur)) != 0 ? 0 : 8355711);
                win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, cch);
            }
            xLeft += LOWORD(GetTextExtent(hdc, app.state.szWork, cch));
            for (j = 0; j < 2; j++) {
                if (j == 1 && i != 1) {
                    ids = static_cast<StringId>(ids + 1);
                    break;
                }
                vcVal = GetVCVal(app, &app.state.game, static_cast<VictoryCondition>(cCur), 0);
                if (i == 0) {
                    idsT = idsPlanets3;
                    vcVal = LOWORD(static_cast<std::int32_t>((vcVal * app.state.game.cPlanMax)) / 100);
                } else {
                    idsT = ids;
                }
                cch = win32::wsprintfA(app.state.szWork, app.state.PCTD, vcVal);
                if (i == 3) {
                    CchAppendUtf8Prefix(std::span{app.state.szWork}, "%");
                    cch++;
                }
                if (iPass == 1) {
                    win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, cch);
                }
                xLeft += LOWORD(GetTextExtent(hdc, app.state.szWork, cch));
                if (i != 2 && i != 3) {
                    xLeft += 4;
                }
                cch = TextIO().pfnCchGetString(app, idsT, std::span{app.state.szWork});
                if (iPass == 1) {
                    win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, cch);
                }
                xLeft += LOWORD(GetTextExtent(hdc, app.state.szWork, cch));
                cCur++;
                ids = static_cast<StringId>(ids + 1);
            }
            if (iPass == 1) {
                yTop += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
            }
        }
    }
    return;
}

void DrawScoreReport(APPSESSION &app, HDC hdc) {
    std::array<char, 12> szNumber{};
    std::array<char, 50> szPlayerName{};
    std::int16_t  dxDig;
    std::int16_t  yTop;
    POINT16  pt;
    std::int16_t  dx45;
    StringId ids;
    COLORREF cr;
    std::int32_t  lMax;
    std::int16_t  j;
    std::int16_t  i;
    std::int16_t  iPass;
    char    *psz;
    std::int32_t  lVal;
    std::int16_t  cch;
    std::int16_t  xLeft;
    std::int32_t  l;

    yTop = 88;
    win32::SetBkMode(hdc, win32::TRANSPARENT);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    dxDig = LOWORD(GetTextExtent(hdc, "9", 1));
    win32::SelectObject(hdc, app.lpui->rghfontArial8[4]);
    xLeft = static_cast<std::int16_t>((5 * dxDig - static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2)) / 2 + app.lpui->vdxScoreX;
    for (i = 0; i < app.state.game.cPlayer; i++) {
        szPlayerName = TextIO().pfnPlayerName(app, i, 1, 1, 1, 0, nullptr);
        psz = szPlayerName.data();
        cch = std::strlen(psz);
        l = GetTextExtent(hdc, psz, cch);
        dx45 = LOWORD(static_cast<std::int32_t>((static_cast<double>(static_cast<std::uint32_t>(LOWORD(l))) / 1.4142)));
        if (app.state.rgplr[i].fInclude && app.state.rgplr[i].fDead) {
            cr = 8355711;
        } else if (app.state.vlprgScoreX[i].fWinner) {
            cr = 16711680;
        } else {
            cr = 0;
        }
        win32::SetTextColor(hdc, cr);
        win32::TextOutA(hdc, xLeft - dx45 + static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2, yTop - dx45 - app.state.dyArial8 / 2 - 4, psz, cch);
        xLeft += 5 * dxDig;
    }
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
    ids = idsPlanets;
    xLeft = app.lpui->vdxScoreX + 4;
    pt.x = static_cast<std::int16_t>((static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 - 12)) / 2;
    pt.y = static_cast<std::int16_t>((static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 - 11)) / 2 - app.state.dyArial8 / 4;
    for (i = 0; i <= app.state.game.cPlayer; i++) {
        win32::PatBlt(hdc, xLeft, yTop - app.state.dyArial8 / 4, 1, static_cast<std::int16_t>((9 * app.state.dyArial8 * 3)) / 2 + 1, win32::PATCOPY);
        xLeft += 5 * dxDig;
    }
    win32::SetTextColor(hdc, 0);
    for (i = 0; i < 9; i++) {
        win32::PatBlt(hdc, app.lpui->vdxScoreX + 4, yTop - app.state.dyArial8 / 4, app.state.game.cPlayer * dxDig * 5, 1, win32::PATCOPY);
        cch = TextIO().pfnCchGetString(app, ids, std::span{app.state.szWork});
        ids = static_cast<StringId>(ids + 1);
        win32::SetTextColor(hdc, 0);
        RightTextOut(app, hdc, app.lpui->vdxScoreX, yTop, app.state.szWork, cch, 0);
        xLeft = 5 * dxDig + app.lpui->vdxScoreX + 2;
        lMax = 0;
        for (iPass = 0; iPass < 2; iPass++) {
            for (j = 0; j < app.state.game.cPlayer; j++) {
                if (!app.state.vlprgScoreX[j].fValid) {
                    lVal = -1;
                } else {
                    switch (i) {
                    case 0:
                        lVal = app.state.vlprgScoreX[j].score.cPlanet;
                        break;
                    case 1:
                        lVal = app.state.vlprgScoreX[j].score.cStarbase;
                        break;
                    case 2:
                    case 3:
                    case 4:
                        lVal = static_cast<std::int32_t>((static_cast<std::uint32_t>((app.state.vlprgScoreX[j].score.rgcsh[i - 2] & 0x1fff)) << (app.state.vlprgScoreX[j].score.rgcsh[i - 2] >> 0xd << 1)));
                        break;
                    case 5:
                        lVal = app.state.vlprgScoreX[j].score.cTechLevels;
                        break;
                    case 6:
                        lVal = app.state.vlprgScoreX[j].score.cResources;
                        break;
                    case 7:
                        lVal = app.state.vlprgScoreX[j].score.lScore;
                        break;
                    case 8:
                        lVal = static_cast<std::int16_t>(app.state.vlprgScoreX[j].turn);
                    }
                }
                if (iPass == 0) {
                    if (lVal > lMax) {
                        lMax = lVal;
                    }
                } else if (lVal >= 0 && (!app.state.rgplr[j].fInclude || !app.state.rgplr[j].fDead)) {
                    if (i == 8) {
                        win32::SetTextColor(hdc, lVal == 1 ? 16711680 : 0);
                    } else {
                        win32::SetTextColor(hdc, lVal == lMax ? 16711680 : 0);
                    }
                    szNumber = PszFromLongK(app, lVal);
                    psz = szNumber.data();
                    cch = static_cast<std::int16_t>(std::strlen(psz));
                    RightTextOut(app, hdc, xLeft, yTop, psz, cch, 0);
                }
                if (iPass == 1) {
                    xLeft += 5 * dxDig;
                }
            }
        }
        yTop += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
    }
    win32::PatBlt(hdc, app.lpui->vdxScoreX + 4, yTop - app.state.dyArial8 / 4, app.state.game.cPlayer * dxDig * 5, 1, win32::PATCOPY);
    return;
}

void DrawHistoryReport(APPSESSION &app, HDC hdc) {
    std::array<char, 50> szPlayerName{};
    char     szT[100];
    RECT     rcChart;
    std::uint16_t dYear;
    POINT16  pt;
    std::int16_t  dy;
    std::int32_t  cYears;
    std::int32_t  cCur;
    std::uint16_t iYearBase;
    std::int16_t  j;
    std::int16_t  i;
    std::int16_t  yCur;
    std::int16_t  cDrawn;
    char    *psz;
    std::int16_t  dx;
    std::int32_t  cScaleMax;
    std::int16_t  xCur;
    std::int32_t  cInc;
    std::int16_t  cch;
    RECT     rcDiamond;
    RECT     rc;
    HPEN     hpenSav;
    HPEN     hpen;
    SCOREX  *lpsx;

    win32::SetBkMode(hdc, win32::TRANSPARENT);
    ControlIO().pfnGetClientRect(app.lpui->hwndScoreXDlg, &rcChart);
    ExpandRc(&rcChart, -48, 0);
    rcChart.top = app.lpui->dyArial10 + app.state.dyArial8;
    rcChart.bottom -= app.state.dyArial8 * 4;
    rc = rcChart;
    rc.top = app.state.dyArial8 / 2;
    rc.bottom = rc.top + app.lpui->dyArial10;
    cch = TextIO().pfnCchGetString(app, idsHistory, std::span{szT});
    psz = &szT[cch];
    cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsPlanets + app.state.gd.iCurGraph), std::span{szT}.subspan(static_cast<std::size_t>(psz - szT)));
    if (cch > 0) psz[CchUtf8Prefix(std::string_view(psz, cch), cch - 1)] = 0;
    win32::SelectObject(hdc, app.lpui->rghfontArial10[1]);
    win32::SetTextColor(hdc, app.lpui->crButtonText);
    RcCtrTextOut(app, hdc, &rc, szT, 0);
    win32::SetRect(&rcDiamond, 15, rc.top, app.lpui->dyArial10 + 15, rc.top + app.lpui->dyArial10);
    DrawDiamond(app, hdc, &rcDiamond, app.lpui->hbrBBlue);
    dx = rcChart.right - rcChart.left;
    dy = rcChart.bottom - rcChart.top;
    win32::PatBlt(hdc, rcChart.left, rcChart.top, dx, dy, win32::BLACKNESS);
    win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
    win32::PatBlt(hdc, rcChart.left - 1, rcChart.bottom, dx + 3, 1, win32::PATCOPY);
    win32::PatBlt(hdc, rcChart.left - 2, rcChart.bottom + 1, dx + 4, 1, win32::PATCOPY);
    win32::PatBlt(hdc, rcChart.right, rcChart.top - 1, 1, dy + 3, win32::PATCOPY);
    win32::PatBlt(hdc, rcChart.right + 1, rcChart.top - 2, 1, dy + 4, win32::PATCOPY);
    win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
    win32::PatBlt(hdc, rcChart.left - 2, rcChart.top - 2, dx + 3, 1, win32::PATCOPY);
    win32::PatBlt(hdc, rcChart.left - 2, rcChart.top - 1, dx + 2, 1, win32::PATCOPY);
    win32::PatBlt(hdc, rcChart.left - 2, rcChart.top - 2, 1, dy + 3, win32::PATCOPY);
    win32::PatBlt(hdc, rcChart.left - 1, rcChart.top - 2, 1, dy + 2, win32::PATCOPY);
    dx -= 4;
    dy -= 4;
    ExpandRc(&rcChart, -2, -2);
    if (app.state.game.turn <= 100) {
        iYearBase = 0;
    } else {
        iYearBase = app.state.game.turn - 100;
    }
    cYears = static_cast<std::uint32_t>((static_cast<std::int32_t>((app.state.game.turn + 4)) / 5 * 5));
    if (cYears > 100) {
        cYears = 100;
    } else if (cYears > 50) {
        cYears = static_cast<std::uint32_t>((static_cast<std::int32_t>(((cYears + 5) / 10)) * 10));
    }
    xCur = rcChart.left;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    if (cYears <= 50) {
        j = 5;
    } else {
        j = 10;
    }
    cDrawn = static_cast<std::int16_t>(LOWORD(cYears)) / j;
    if (cDrawn > 0) {
        for (i = 0; i <= cDrawn; i++) {
            xCur = static_cast<std::int16_t>((dx * i)) / cDrawn + rcChart.left;
            cch = win32::wsprintfA(szT, app.state.PCTD, iYearBase + 2400 + i * j);
            CtrTextOut(hdc, xCur, rcChart.bottom + 6, szT, cch);
            if (i > 0 && i < cDrawn) {
                win32::PatBlt(hdc, xCur, rcChart.top - 2, 1, dy + 4, win32::PATCOPY);
            }
        }
    }
    cScaleMax = -1;
    for (i = 0; i < app.state.game.cPlayer; i++) {
        if (app.state.rgsxPlr[i]) {
            for (j = 0; j < app.state.rgcsxPlr[i]; j++) {
                cCur = LFetchScoreXVal(app.state.rgsxPlr[i] + j, app.state.gd.iCurGraph);
                if (cCur > cScaleMax) {
                    cScaleMax = cCur;
                }
            }
        }
    }
    if (cScaleMax >= 0) {
        if (cScaleMax < 5) {
            cScaleMax = 5;
        }
        if (cScaleMax <= 12) {
            cInc = 1;
        } else if (cScaleMax <= 25) {
            cInc = 2;
        } else if (cScaleMax <= 60) {
            cInc = 5;
        } else if (cScaleMax <= 120) {
            cInc = 10;
        } else if (cScaleMax <= 300) {
            cInc = 25;
        } else if (cScaleMax <= 600) {
            cInc = 50;
        } else if (cScaleMax <= 1200) {
            cInc = 100;
        } else if (cScaleMax <= 6000) {
            cInc = 500;
        } else if (cScaleMax <= 12000) {
            cInc = 1000;
        } else {
            cInc = static_cast<std::uint32_t>((static_cast<std::int32_t>((cScaleMax / 12)) / 500 * 500));
        }
        xCur = rcChart.left - 6;
        for (cCur = cInc; cCur < cScaleMax; cCur += cInc) {
            yCur = rcChart.bottom - LOWORD(static_cast<std::int32_t>((static_cast<std::int32_t>((dy * cCur)) / cScaleMax)));
            if (yCur < app.state.dyArial8 / 2 + rcChart.top)
                break;
            cch = win32::wsprintfA(app.state.szWork, app.state.PCTLD, cCur);
            RightTextOut(app, hdc, xCur, yCur - app.state.dyArial8 / 2, app.state.szWork, cch, 0);
            win32::PatBlt(hdc, rcChart.left - 2, yCur, dx + 4, 1, win32::PATCOPY);
        }
        xCur = rcChart.left + 6;
        yCur = rcChart.top + 6;
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (app.state.rgsxPlr[i]) {
                szPlayerName = TextIO().pfnPlayerName(app, i, 1, 1, 0, 0, nullptr);
                psz = szPlayerName.data();
                win32::SetTextColor(hdc, i == app.state.idPlayer ? 0xffffff : app.lpui->rgcrPlrHistory[i]);
                win32::TextOutA(hdc, xCur, yCur, psz, std::strlen(psz));
                yCur += app.state.dyArial8;
            }
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (app.state.rgsxPlr[i]) {
                cDrawn = 0;
                hpen = win32::CreatePen(0, 1, i == app.state.idPlayer ? 0xffffff : app.lpui->rgcrPlrHistory[i]);
                hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, hpen));
                for (j = 0; j < app.state.rgcsxPlr[i]; j++) {
                    lpsx = app.state.rgsxPlr[i] + j;
                    if (lpsx->turn >= iYearBase && lpsx->turn <= static_cast<std::uint16_t>((LOWORD(cYears) + iYearBase))) {
                        dYear = lpsx->turn - iYearBase;
                        cCur = LFetchScoreXVal(lpsx, app.state.gd.iCurGraph);
                        pt.x = LOWORD(static_cast<std::int32_t>((static_cast<std::int32_t>((static_cast<std::uint32_t>(dYear) * dx)) / cYears))) + rcChart.left;
                        pt.y = rcChart.bottom - LOWORD(static_cast<std::int32_t>((static_cast<std::int32_t>((cCur * dy)) / cScaleMax)));
                        if (cDrawn == 0) {
                            win32::MoveToEx(hdc, pt.x, pt.y, nullptr);
                        } else {
                            win32::LineTo(hdc, pt.x, pt.y);
                        }
                        cDrawn++;
                    }
                }
                if (cDrawn == 1) {
                    win32::SetPixel(hdc, pt.x, pt.y, i == app.state.idPlayer ? 0xffffff : app.lpui->rgcrPlrHistory[i]);
                }
                win32::SelectObject(hdc, hpenSav);
                win32::DeleteObject(hpen);
            }
        }
    }
    return;
}

std::int32_t LFetchScoreXVal(SCOREX *lpsx, std::int16_t iVal) {
    switch (iVal) {
    case 0:
        return lpsx->score.cPlanet;
    case 1:
        return lpsx->score.cStarbase;
    case 2:
    case 3:
    case 4:
        return static_cast<std::int32_t>((static_cast<std::uint32_t>((lpsx->score.rgcsh[iVal - 2] & 0x1fff)) << (lpsx->score.rgcsh[iVal - 2] >> 0xd << 1)));
    case 5:
        return lpsx->score.cTechLevels;
    case 6:
        return lpsx->score.cResources;
    case 7:
        return lpsx->score.lScore;
    default:
        return 0;
    }
}

std::int16_t DxReportColHdr(APPSESSION &app, ReportType irpt, std::int16_t iCol, std::span<char> psz, HDC hdc) {
    char     szT[40];
    StringId ids;
    std::int16_t  dxDigit;
    std::int16_t  dx;
    std::int16_t  cch;
    std::int16_t  dx2;

    if (psz.empty()) return 0;
    szT[0] = '8';
    dxDigit = LOWORD(GetTextExtent(hdc, szT, 1));
    switch (irpt) {
    default:
        psz[0] = 0;
        return 0;
    case rptPlanets:
        cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsPlanetName + iCol), psz);
        dx = LOWORD(GetTextExtent(hdc, psz.data(), cch));
        if (static_cast<std::uint16_t>(iCol) > colPlanetRoutingDest)
            break;
        switch (iCol) {
        case colPlanetStarbase:
        case colPlanetDriverDest:
        case colPlanetRoutingDest:
            dx2 = 15 * dxDigit;
            goto DxChk;
        case colPlanetName:
            dx *= 2;
            break;
        case colPlanetProduction:
            dx = 3 * dx + 20;
            break;
        case colPlanetValue:
            ids = idsN100100;
        ChkAltString:
            cch = TextIO().pfnCchGetString(app, ids, std::span{szT});
            dx2 = LOWORD(GetTextExtent(hdc, szT, cch));
        DxChk:
            if (dx2 <= dx)
                break;
            dx = dx2;
            break;

        case colPlanetResources:
            ids = idsN10001000;
            goto ChkAltString;
        case colPlanetMines:
        case colPlanetFactories:
            dx2 = 5 * dxDigit;
            goto DxChk;
        case colPlanetMiningRate:
        case colPlanetMinConc:
            dx2 = 11 * dxDigit;
            goto DxChk;
        case colPlanetMinerals:
            dx2 = 14 * dxDigit;
            goto DxChk;
        case colPlanetCapacity:
            dx2 = dxDigit * 4 + 2;
            goto DxChk;
        }
        break;
    case rptFleets:
        cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsFleetName + iCol), psz);
        dx = LOWORD(GetTextExtent(hdc, psz.data(), cch));
        if (static_cast<std::uint16_t>(iCol) > colFleetMass)
            break;
        switch (iCol) {
        case colFleetLocation:
            dx = dx * 2 + dx / 2;
            break;
        case colFleetName:
        case colFleetId:
        case colFleetDestination:
        case colFleetComposition:
        case colFleetBattlePlan:
        case colFleetMass:
            dx *= 2;
            break;
        case colFleetEta:
            dx = dx * 2 - dxDigit;
            break;
        case colFleetTask:
            dx = dx * 4 + dx / 2;
            break;
        case colFleetFuel:
            dx2 = 5 * dxDigit;
            if (dx2 <= dx)
                break;
            dx = dx2;
            break;
        case colFleetCargo:
            dx2 = 19 * dxDigit;
            if (dx2 > dx) {
                dx = dx2;
            }
        }
        break;
    case rptEnemyFleets:
        cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsFleetName2 + iCol), psz);
        dx = LOWORD(GetTextExtent(hdc, psz.data(), cch));
        switch (iCol) {
        case colEnemyFleetLocation:
            dx = dx * 2 + dx / 2;
            break;
        case colEnemyFleetName:
            dx = 3 * dx;
            break;
        case colEnemyFleetId:
        case colEnemyFleetComposition:
            dx *= 2;
        }
        break;
    case rptBattles:
        cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsLocation4 + iCol), psz);
        dx = LOWORD(GetTextExtent(hdc, psz.data(), cch));
        if (iCol == colBattleLocation) {
            dx = dx * 2 + dx / 2;
        }
    }
    dx += 5;
    return static_cast<std::int16_t>((dx + 1)) / 2 * 2;
}

void DrawReportItem(APPSESSION &app, HDC hdc, RECT *prc, ReportType irpt, std::int16_t irow, std::int16_t icol) {
    std::array<char, 360> szDisplayLabel{};
    std::array<char, 360> szObjectName{};
    std::array<char, 360> szReportText{};
    std::uint8_t *lpbd;
    char     szT[100];
    char     chT;
    const char *lpsz;
    PLANET  *lppl;
    std::int16_t  j;
    std::int16_t  i;
    FLEET   *lpfl;
    std::int16_t  dx;
    const char *psz;
    std::int16_t  xCur;
    std::int16_t  cch;
    std::int32_t  l;
    HBRUSH   hbr;
    RECT     rc;
    float    pct;
    std::int32_t  rgl[4];
    std::int16_t  iItem;
    PLANET   pl;
    std::int16_t  fEnough;

    szT[0] = '8';
    dx = LOWORD(GetTextExtent(hdc, szT, 1));
    switch (irpt) {
    case rptPlanets:
        lppl = app.state.lpPlanets + app.state.vlprgidPlanet[irow];
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        win32::SetTextColor(hdc, 0);
        win32::SetBkMode(hdc, win32::TRANSPARENT);
        if (static_cast<std::uint16_t>(icol) > colPlanetRoutingDest)
            break;
        switch (icol) {
        case colPlanetStarbase:
            if (!lppl->fStarbase) {
                lpsz = app.state.szDblDash;
            } else {
                lpsz = app.state.rglpshdefSB[app.state.idPlayer][lppl->isb].hul.szClass;
            }
            win32::ExtTextOutA(hdc, prc->left, prc->top, win32::ETO_CLIPPED, prc, lpsz, std::strlen(lpsz), nullptr);
            break;
        case colPlanetRoutingDest:
            if (lppl->idRoute == 0) {
                psz = app.state.szDblDash;
            } else {
                szObjectName = TextIO().pfnPlanetName(app, lppl->idRoute - 1);
                psz = szObjectName.data();
            }
            win32::ExtTextOutA(hdc, prc->left, prc->top, win32::ETO_CLIPPED, prc, psz, std::strlen(psz), nullptr);
            break;
        case colPlanetDriverDest:
            if (lppl->idFling == 0) {
                psz = app.state.szDblDash;
            } else {
                szObjectName = TextIO().pfnPlanetName(app, lppl->idFling - 1);
                psz = szObjectName.data();
            }
            win32::ExtTextOutA(hdc, prc->left, prc->top, win32::ETO_CLIPPED, prc, psz, std::strlen(psz), nullptr);
            break;
        case colPlanetName:
            if (app.state.sel.grobj == grobjPlanet && app.state.sel.pl.id == lppl->id) {
                win32::SetTextColor(hdc, 127);
            }
            szObjectName = TextIO().pfnPlanetName(app, lppl->id);
            psz = szObjectName.data();
            win32::ExtTextOutA(hdc, prc->left, prc->top, win32::ETO_CLIPPED, prc, psz, std::strlen(psz), nullptr);
            if (!lppl->fStarbase)
                break;
            rc = *prc;
            dx = static_cast<std::int16_t>((rc.bottom - rc.top)) / 3 - 1;
            rc.left = rc.right - dx;
            if (PartQueryIO().pfnHull(app.state.rglpshdefSB[app.state.idPlayer][lppl->isb].hul.ihuldef)->hul.wtCargoMax != 0) {
                hbr = app.lpui->hbrYellow;
            } else {
                hbr = app.lpui->hbrBlue;
            }
            rc.top = prc->top;
            rc.bottom = rc.top + dx;
            win32::FillRect(hdc, &rc, hbr);
            if (ColonyIO().pfnWarp(app, lppl, nullptr) > 0) {
                rc.top = prc->top + dx + 1;
                rc.bottom = rc.top + dx;
                win32::FillRect(hdc, &rc, app.lpui->hbrPurple);
            }
            if (IStargateFromLppl(app, lppl) == -1)
                break;
            rc.top = (dx + 1) * 2 + prc->top;
            rc.bottom = rc.top + dx;
            win32::FillRect(hdc, &rc, app.lpui->hbrGreen);
            break;
        case colPlanetPopulation:
            if (CalcPlanetMaxPop(app, lppl->id, lppl->iPlayer) < lppl->rgwtMin[3]) {
                win32::SetTextColor(hdc, 0xff);
            }
            cch = CchCopyUtf8Prefix(std::span{szT}, CommaFormatLong(app, static_cast<std::uint32_t>((lppl->rgwtMin[3] * 100))).data());
            RightTextOut(app, hdc, prc->right, prc->top, szT, cch, 0);
            break;
        case colPlanetCapacity:
            cch = win32::wsprintfA(szT, app.state.PCTDPCTPCT, CapacityIO().pfnPctPlanetCapacity(app, lppl));
            RightTextOut(app, hdc, prc->right, prc->top, szT, cch, 0);
            break;
        case colPlanetDefense:
            i = lppl->cDefenses;
            j = ColonyIO().pfnOperableDefenses(app, lppl, app.state.idPlayer, 0);
            if (i > 0) {
                CalcPctSurvive(app, lppl, &pct, nullptr);
                pct = static_cast<float>((static_cast<double>(1.0) - pct));
                cch = win32::wsprintfA(szT, app.state.PCTDXPCTDPCTPCT, LOWORD(static_cast<std::int32_t>((static_cast<double>(pct) * 100))),
                               LOWORD(static_cast<std::int32_t>(((pct - static_cast<double>(static_cast<std::int16_t>(LOWORD(static_cast<std::int32_t>((static_cast<double>(pct) * 100))))) / 100.0) * 10000))));
                goto DrawPlusDef;
            }
            szT[2] = '-';
            szT[1] = '-';
            szT[0] = '-';
            cch = 3;
            goto DrawPlusDef;
        case colPlanetFactories:
            i = lppl->cFactories;
            j = ColonyIO().pfnOperableFactories(app, lppl, app.state.idPlayer, 0);
            goto DrawMineFact;
        case colPlanetMines:
            i = lppl->cMines;
            j = ColonyIO().pfnOperableMines(app, lppl, app.state.idPlayer, 0);
        DrawMineFact:
            cch = CchCopyUtf8Prefix(std::span{szT}, CommaFormatLong(app, i).data());
        DrawPlusDef:
            if (i >= j) {
                win32::SetTextColor(hdc, i == j ? 32512 : 0xff);
            }
            RightTextOut(app, hdc, prc->right, prc->top, szT, cch, 0);
            break;
        case colPlanetValue:
            cch = TextIO().pfnCchGetString(app, idsN100, std::span{szT});
            dx = LOWORD(GetTextExtent(hdc, szT, cch));
            i = PlanetQueryIO().pfnDesirability(app, lppl, app.state.idPlayer);
            cch = win32::wsprintfA(szT, app.state.PCTDPCTPCT, i);
            if (i <= 10) {
                win32::SetTextColor(hdc, i >= 0 ? 32639 : 0xff);
            }
            RightTextOut(app, hdc, prc->left + dx, prc->top, szT, cch, 0);
            j = PctPlanetOptValue(app, lppl, app.state.idPlayer);
            if (j <= i)
                break;
            if (j > 0) {
                win32::SetTextColor(hdc, j > 10 ? 0 : 32639);
            }
            cch = win32::wsprintfA(szT, "(%d%%)", j);
            RightTextOut(app, hdc, prc->right, prc->top, szT, cch, 0);
            break;
        case colPlanetMinerals:
        case colPlanetMiningRate:
            xCur = dx * 4 + prc->left;
            if (icol == colPlanetMiningRate) {
                xCur -= dx;
                ColonyIO().pfnEstimateMining(app, lppl, rgl, -1, 0);
            }
            for (i = 0; i < 3; i++) {
                if (icol == colPlanetMinerals) {
                    l = lppl->rgwtMin[i];
                } else {
                    l = rgl[i];
                }
                DrawMineralItem(app, hdc, xCur, prc->top, i, l);
                if (icol == colPlanetMiningRate) {
                    xCur += dx * 4;
                } else {
                    xCur += 5 * dx;
                }
            }
            break;
        case colPlanetMinConc:
            xCur = 3 * dx + prc->left;
            for (i = 0; i < 3; i++) {
                iItem = lppl->rgMinConc[i];
                DrawMineralItem(app, hdc, xCur, prc->top, i, iItem);
                xCur += dx * 4;
            }
            break;
        case colPlanetProduction:
            pl = app.state.sel.pl;
            app.state.sel.pl = *lppl;
            ControlIO().pfnFillPlanetProdLB(app, nullptr, nullptr, lppl);
            app.state.sel.pl = pl;
            DrawProductionItem(app, hdc, prc, app.state.szWork, 0, 0, 2);
            break;
        case colPlanetResources:
            cch = TextIO().pfnCchGetString(app, idsN1000, std::span{szT});
            dx = LOWORD(GetTextExtent(hdc, szT, cch));
            i = j = ResearchIO().pfnResources(app, lppl, app.state.idPlayer);
            if (!lppl->fNoResearch) {
                i -= win32::MulDiv(i, app.state.rgplr[app.state.idPlayer].pctResearch, 100);
            }
            TextIO().pfnCchGetString(app, idsD4, std::span{szT});
            cch = win32::wsprintfA(app.state.szWork, szT, i);
            RightTextOut(app, hdc, prc->left + dx, prc->top, app.state.szWork, cch, 0);
            cch = win32::wsprintfA(szT, app.state.PCTD, j);
            RightTextOut(app, hdc, prc->right, prc->top, szT, cch, 0);
        }
        break;

    case rptFleets:
        lpfl = app.state.rglpfl[app.state.vlprgidFleet[irow]];
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        win32::SetTextColor(hdc, 0);
        win32::SetBkMode(hdc, win32::TRANSPARENT);
        if (static_cast<std::uint16_t>(icol) > colFleetMass)
            break;
        switch (icol) {
        case colFleetBattlePlan:
            i = lpfl->iplan;
            CchCopyUtf8Prefix(std::span{szT}, app.state.rglpbtlplan[lpfl->iplr()][lpfl->iplan].szName);
            psz = szT;
            win32::ExtTextOutA(hdc, prc->left, prc->top, win32::ETO_CLIPPED, prc, psz, std::strlen(psz), nullptr);
            break;
        case colFleetName:
            if (app.state.sel.grobj == grobjFleet && app.state.sel.fl.id == lpfl->id) {
                win32::SetTextColor(hdc, 127);
            }
            szObjectName = PszGetFleetName(app, lpfl->id);
            psz = szObjectName.data();
            win32::ExtTextOutA(hdc, prc->left, prc->top, win32::ETO_CLIPPED, prc, psz, std::strlen(psz), nullptr);
            break;
        case colFleetLocation:
            if (lpfl->idPlanet != idPlanetDeepSpace) {
                szObjectName = TextIO().pfnPlanetName(app, lpfl->idPlanet);
                psz = szObjectName.data();
            } else {
                psz = szT;
                win32::wsprintfA(szT, TextIO().pfnCompressed(app, idsSpaceDD).data(), lpfl->pt.x, lpfl->pt.y);
            }
            win32::ExtTextOutA(hdc, prc->left, prc->top, win32::ETO_CLIPPED, prc, psz, std::strlen(psz), nullptr);
            break;
        case colFleetDestination:
            szReportText = PszGetDestName(app, lpfl, hdc);
            psz = szReportText.data();
            win32::ExtTextOutA(hdc, prc->left, prc->top, win32::ETO_CLIPPED, prc, psz, std::strlen(psz), nullptr);
            break;
        case colFleetTask:
            szReportText = PszGetTaskName(app, lpfl, &i);
            psz = szReportText.data();
            if (i != -1) {
                win32::SetTextColor(hdc, app.lpui->rgcrMinerals[i]);
            }
            win32::ExtTextOutA(hdc, prc->left, prc->top, win32::ETO_CLIPPED, prc, psz, std::strlen(psz), nullptr);
            break;
        case colFleetEta:
            szDisplayLabel = PszGetETA(app, hdc, lpfl, nullptr);
            psz = szDisplayLabel.data();
            RightTextOut(app, hdc, prc->right, prc->top, psz, std::strlen(psz), 0);
            break;
        case colFleetCargo:
            xCur = dx * 4 + prc->left;
            for (i = 0; i <= 3; i++) {
                DrawMineralItem(app, hdc, xCur, prc->top, i, lpfl->rgwtMin[i]);
                xCur += 5 * dx;
            }
            break;
        case colFleetFuel:
            fEnough = lpfl->cord <= 1 || lpfl->rgwtMin[4] >= EstFuelUse(app, lpfl, 0, lpfl->lpplord->rgord()[1].iWarp, -1, 0);
            DrawMineralItem(app, hdc, dx * 4 + prc->left, prc->top, -fEnough, lpfl->rgwtMin[4]);
            break;
        case colFleetComposition:
            i = IshdefPrimaryFromLpfl(app, lpfl, &j);
            if (lpfl->rgdv[i].dp != 0) {
                win32::SetTextColor(hdc, 0xff);
            }
            prc->right -= 6 * dx;
            psz = app.state.rgshdef[i].hul.szClass;
            win32::ExtTextOutA(hdc, prc->left, prc->top, win32::ETO_CLIPPED, prc, psz, std::strlen(psz), nullptr);
            psz = szT;
            cch = win32::wsprintfA(szT, app.state.PCTD, lpfl->rgcsh[i]);
            chT = '+';
            RightTextOut(app, hdc, 6 * dx + prc->right - LOWORD(GetTextExtent(hdc, &chT, 1)), prc->top, psz, cch, 0);
            if (j <= 1)
                break;
            win32::TextOutA(hdc, 6 * dx + prc->right - LOWORD(GetTextExtent(hdc, &chT, 1)), prc->top, &chT, 1);
            break;
        case colFleetCloak:
            i = PctCloakFromLpfl(app, lpfl);
            if (i == 0) {
                cch = std::strlen(app.state.szDblDash);
                psz = app.state.szDblDash;
            } else {
                psz = szT;
                cch = win32::wsprintfA(szT, app.state.PCTDPCTPCT, i);
            }
            RightTextOut(app, hdc, prc->right - 5, prc->top, psz, cch, 0);
            break;
        case colFleetMass:
            DrawMineralItem(app, hdc, prc->right - 2, prc->top, -1, WtFromLpfl(app, lpfl));
            break;
        case colFleetId:
            psz = szT;
            cch = win32::wsprintfA(szT, "%d", lpfl->ifl() + 1);
            RightTextOut(app, hdc, prc->right - 2, prc->top, psz, cch, 0);
        }
        break;
    case rptBattles:
        lpbd = BtlDataGet(app, app.lpui->vlprgidMisc[irow]);
        if (!lpbd)
            break;
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        win32::SetTextColor(hdc, 0);
        win32::SetBkMode(hdc, win32::TRANSPARENT);
        if (static_cast<std::uint16_t>(icol) > colBattleTheirsLeft)
            break;
        switch (icol) {
        case colBattleLocation:
            if (BdvFromLp(app, lpbd).idPlanet() != idPlanetNone) {
                szObjectName = TextIO().pfnPlanetName(app, BdvFromLp(app, lpbd).idPlanet());
                psz = szObjectName.data();
            } else {
                psz = szT;
                win32::wsprintfA(szT, TextIO().pfnCompressed(app, idsSpaceDD).data(), BdvFromLp(app, lpbd).pt().x(), BdvFromLp(app, lpbd).pt().y());
            }
            if (BdvFromLp(app, lpbd).pt().x() == app.state.sel.scan.pt.x && BdvFromLp(app, lpbd).pt().y() == app.state.sel.scan.pt.y) {
                win32::SetTextColor(hdc, 127);
            }
            win32::ExtTextOutA(hdc, prc->left, prc->top, win32::ETO_CLIPPED, prc, psz, std::strlen(psz), nullptr);
            break;
        case colBattleSides:
            DrawMineralItem(app, hdc, prc->right - 2, prc->top, -1, static_cast<std::uint32_t>(BdvFromLp(app, lpbd).cplr()));
            break;
        case colBattleStarbase:
            if (CBattleUnits(app, lpbd, static_cast<BattleUnitFlags>(grBuOurUnits | grBuIncludeSb)) != 0) {
                chT = 'O';
            } else if (CBattleUnits(app, lpbd, static_cast<BattleUnitFlags>(grBuTheirUnits | grBuIncludeSb)) != 0) {
                chT = 'T';
            } else {
                chT = ' ';
            }
            CtrTextOut(hdc, static_cast<std::int16_t>((prc->right - prc->left)) / 2 + prc->left, prc->top, &chT, 1);
            break;
        case colBattleUnarmed:
            i = 11;
            goto BtlUnitsCom;
        case colBattleScout:
            i = 19;
            goto BtlUnitsCom;
        case colBattleWarship:
            i = 35;
            goto BtlUnitsCom;
        case colBattleBomber:
            i = 67;
            goto BtlUnitsCom;
        case colBattleUtility:
            i = 131;
            goto BtlUnitsCom;
        case colBattleUnits:
            i = 255;
            goto BtlUnitsCom;
        case colBattleOurs:
            i = 253;
            goto BtlUnitsCom;
        case colBattleTheirs:
            i = 254;
        BtlUnitsCom:
            DrawMineralItem(app, hdc, prc->right - 2, prc->top, -1, CBattleUnits(app, lpbd, static_cast<BattleUnitFlags>(i)));
            break;
        case colBattleOurDead:
        case colBattleTheirDead:
            l = CBattleKills(app, lpbd, icol == colBattleOurDead);
            DrawMineralItem(app, hdc, prc->right - 2, prc->top, -1, l);
            break;
        case colBattleOursLeft:
            i = 253;
        LUnitsLeft:
            l = CBattleUnits(app, lpbd, static_cast<BattleUnitFlags>(i));
            l -= CBattleKills(app, lpbd, icol == colBattleOursLeft);
            DrawMineralItem(app, hdc, prc->right - 2, prc->top, -1, l);
            break;
        case colBattleTheirsLeft:
            i = 254;
            goto LUnitsLeft;
        }
        break;

    case rptEnemyFleets:
        lpfl = app.state.rglpfl[app.lpui->vlprgidMisc[irow]];
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        win32::SetTextColor(hdc, 0);
        win32::SetBkMode(hdc, win32::TRANSPARENT);
        if (static_cast<std::uint16_t>(icol) <= colEnemyFleetUtility) {
            switch (icol) {
            case colEnemyFleetName:
                if (app.state.sel.scan.grobj == grobjFleet && app.state.rglpfl[app.state.sel.scan.ifl]->id == lpfl->id) {
                    win32::SetTextColor(hdc, 127);
                }
                szObjectName = PszGetFleetName(app, lpfl->id);
                psz = szObjectName.data();
                win32::ExtTextOutA(hdc, prc->left, prc->top, win32::ETO_CLIPPED, prc, psz, std::strlen(psz), nullptr);
                break;
            case colEnemyFleetLocation:
                if (lpfl->idPlanet != idPlanetDeepSpace) {
                    szObjectName = TextIO().pfnPlanetName(app, lpfl->idPlanet);
                    psz = szObjectName.data();
                } else {
                    psz = szT;
                    win32::wsprintfA(szT, TextIO().pfnCompressed(app, idsSpaceDD).data(), lpfl->pt.x, lpfl->pt.y);
                }
                win32::ExtTextOutA(hdc, prc->left, prc->top, win32::ETO_CLIPPED, prc, psz, std::strlen(psz), nullptr);
                break;
            case colEnemyFleetComposition:
                i = IshdefPrimaryFromLpfl(app, lpfl, &j);
                prc->right -= 6 * dx;
                lpsz = app.state.rglpshdef[lpfl->iPlayer][i].hul.szClass;
                win32::ExtTextOutA(hdc, prc->left, prc->top, win32::ETO_CLIPPED, prc, lpsz, std::strlen(lpsz), nullptr);
                psz = szT;
                cch = win32::wsprintfA(szT, app.state.PCTD, lpfl->rgcsh[i]);
                chT = '+';
                RightTextOut(app, hdc, 6 * dx + prc->right - LOWORD(GetTextExtent(hdc, &chT, 1)), prc->top, psz, cch, 0);
                if (j <= 1)
                    break;
                win32::TextOutA(hdc, 6 * dx + prc->right - LOWORD(GetTextExtent(hdc, &chT, 1)), prc->top, &chT, 1);
                break;
            case colEnemyFleetShips:
                l = 0;
                for (i = 0; i < 16; i++) {
                    l += lpfl->rgcsh[i];
                }
                DrawMineralItem(app, hdc, prc->right - 2, prc->top, -1, l);
                break;
            case colEnemyFleetUnarmed:
                l = 0;
                for (i = 0; i < 16; i++) {
                    if (lpfl->rgcsh[i] != 0) {
                        j = PartQueryIO().pfnHull(app.state.rglpshdef[lpfl->iPlayer][i].hul.ihuldef)->imdCategory;
                        if (j <= 1 || j >= 6) {
                            l += lpfl->rgcsh[i];
                        }
                    }
                }
                DrawMineralItem(app, hdc, prc->right - 2, prc->top, -1, l);
                break;
            case colEnemyFleetScout:
                j = 2;
            LEFleetCount:
                l = 0;
                for (i = 0; i < 16; i++) {
                    if (lpfl->rgcsh[i] != 0 && PartQueryIO().pfnHull(app.state.rglpshdef[lpfl->iPlayer][i].hul.ihuldef)->imdCategory == j) {
                        l += lpfl->rgcsh[i];
                    }
                }
                DrawMineralItem(app, hdc, prc->right - 2, prc->top, -1, l);
                break;
            case colEnemyFleetWarship:
                j = 3;
                goto LEFleetCount;
            case colEnemyFleetBomber:
                j = 5;
                goto LEFleetCount;
            case colEnemyFleetUtility:
                j = 4;
                goto LEFleetCount;
            case colEnemyFleetMass:
                DrawMineralItem(app, hdc, prc->right - 2, prc->top, -1, lpfl->wtFleet());
                break;
            case colEnemyFleetId:
                psz = szT;
                cch = win32::wsprintfA(szT, "%d", lpfl->ifl() + 1);
                RightTextOut(app, hdc, prc->right - 2, prc->top, psz, cch, 0);
                break;
            case colEnemyFleetWarp:
                if (!lpfl->fdirValid() || lpfl->iwarpFlt() <= 0) {
                    psz = "--";
                    cch = 2;
                } else {
                    psz = szT;
                    cch = win32::wsprintfA(szT, "%d", lpfl->iwarpFlt());
                }
                RightTextOut(app, hdc, prc->right - 2, prc->top, psz, cch, 0);
            }
            break;
        }
    }
    return;
}

void DrawMineralItem(APPSESSION &app, HDC hdc, std::int16_t x, std::int16_t y, std::int16_t iMineral, std::int32_t l) {
    std::array<char, 12> szNumber{};
    const char *psz;
    std::int16_t cch;

    if (iMineral < 0) {
        win32::SetTextColor(hdc, 0);
    } else {
        win32::SetTextColor(hdc, app.lpui->rgcrMinerals[iMineral]);
    }
    if (l >= 0) {
        szNumber = PszFromLongK(app, l);
        psz = szNumber.data();
        cch = static_cast<std::int16_t>(std::strlen(psz));
    } else {
        cch = std::strlen(app.state.szDblDash);
        psz = app.state.szDblDash;
    }
    RightTextOut(app, hdc, x, y, psz, cch, 0);
    return;
}

void SortReportCache(APPSESSION &app, ReportType irpt, std::int16_t icol) {
    std::uint16_t rgidRep[1024];
    PLANET  *lpplMac;
    std::int16_t  cRows;
    std::uint16_t iItem;
    PLANET  *lppl;
    FLEET   *lpfl;
    std::int16_t  i;

    cRows = 0;
    iItem = 0;
    if (app.lpui->vprptCur->icolSort != icol) {
        app.lpui->vicolSortPrev = app.lpui->vprptCur->icolSort;
        app.lpui->viSubsortPrev = app.lpui->vprptCur->iSubsort;
        app.lpui->vfAscendingPrev = app.lpui->vprptCur->fAscending;
        app.lpui->vprptCur->icolSort = icol;
        app.state.gd.fChgReports = 1;
    }
    if (app.lpui->hwndReportDlg || !app.lpui->vprptCur->fCached) {
        switch (irpt) {
        case rptFleets:
            app.lpui->vlprgidRep = app.state.vlprgidFleet;
            for (iItem = 0; static_cast<std::int16_t>(iItem) < app.state.cFleet; iItem++) {
                lpfl = app.state.rglpfl[iItem];
                if (!app.state.rglpfl[iItem])
                    break;
                if (lpfl->iplr() == app.state.idPlayer) {
                    rgidRep[cRows++] = iItem;
                }
            }
            break;
        case rptEnemyFleets:
            app.lpui->vlprgidRep = app.lpui->vlprgidMisc;
            app.state.vrptBattle.fCached = 0;
            for (iItem = 0; static_cast<std::int16_t>(iItem) < app.state.cFleet; iItem++) {
                lpfl = app.state.rglpfl[iItem];
                if (!app.state.rglpfl[iItem])
                    break;
                if (lpfl->iplr() != app.state.idPlayer) {
                    rgidRep[cRows++] = iItem;
                }
                if (cRows >= 1020)
                    break;
            }
            break;
        case rptPlanets:
            app.lpui->vlprgidRep = app.state.vlprgidPlanet;
            lppl = app.state.lpPlanets;
            lpplMac = app.state.lpPlanets + app.state.cPlanet;
            for (; lppl < lpplMac; lppl++) {
                if (lppl->iPlayer == app.state.idPlayer && lppl->det == detAll) {
                    rgidRep[cRows++] = iItem;
                }
                iItem++;
            }
            break;
        case rptBattles:
            app.lpui->vlprgidRep = app.lpui->vlprgidMisc;
            app.state.vrptEFleet.fCached = 0;
            cRows = CBattles(app);
            for (i = 0; i < cRows; i++) {
                rgidRep[i] = i;
            }
            break;
        default:
            return;
        }
        app.lpui->vprptCur->cRows = cRows;
        qsort16(app, rgidRep, cRows, sizeof(std::uint16_t), reinterpret_cast<APPQSORTCOMPARE>(ICompReport));
        std::memcpy(app.lpui->vlprgidRep, rgidRep, cRows * 2);
        app.lpui->vprptCur->fCached = 1;
    }
    return;
}

int ICompReport(APPSESSION &app, std::uint16_t *pid1, std::uint16_t *pid2) {
    std::array<char, 360> szObjectName{};
    std::array<char, 360> szReportText{};
    char         szT[80];
    std::int32_t      l2;
    std::int16_t      fAscending;
    std::int16_t      icolSort;
    std::int16_t      i1;
    HullCategory j;
    std::int16_t      i;
    std::int32_t      l1;
    std::int16_t      iSubsort;
    const char        *psz;
    std::int16_t      iRet;
    std::int16_t      i2;
    std::int16_t      fTier2;
    ReportType   irpt;
    PLANET      *lppl2;
    PLANET      *lppl1;
    float        pct2;
    float        pct1;
    std::int16_t      iFirst;
    std::int32_t      rgl[4];
    std::int16_t      iLast;
    FLEET       *lpfl2;
    FLEET       *lpfl1;
    std::uint8_t     *lpbd1;
    std::int16_t      ibtl2;
    std::int16_t      ibtl1;
    std::uint8_t     *lpbd2;

    iRet = 0;
    iSubsort = app.lpui->vprptCur->iSubsort;
    irpt = app.lpui->vprptCur->irpt;
    icolSort = app.lpui->vprptCur->icolSort;
    fAscending = app.lpui->vprptCur->fAscending;
    fTier2 = 0;
TryTier2:
    switch (irpt) {
    case rptPlanets:
        lppl1 = app.state.lpPlanets + *pid1;
        lppl2 = app.state.lpPlanets + *pid2;
        if (static_cast<std::uint16_t>(icolSort) > colPlanetRoutingDest)
            break;
        switch (icolSort) {
        case colPlanetStarbase:
            if (!lppl1->fStarbase) {
                if (!lppl2->fStarbase) {
                    iRet = 0;
                    break;
                }
                iRet = 1;
                break;
            }
            if (!lppl2->fStarbase) {
                iRet = -1;
                break;
            }
            iRet = std::strcmp(app.state.rglpshdefSB[app.state.idPlayer][lppl1->isb].hul.szClass, app.state.rglpshdefSB[app.state.idPlayer][lppl2->isb].hul.szClass);
            break;
        case colPlanetRoutingDest:
            if (lppl1->idRoute == 0) {
                if (lppl2->idRoute == 0) {
                    iRet = 0;
                    break;
                }
                iRet = 1;
                break;
            }
            if (lppl2->idRoute == 0) {
                iRet = -1;
                break;
            }
            szObjectName = TextIO().pfnPlanetName(app, lppl1->idRoute - 1);
            psz = szObjectName.data();
            CchCopyUtf8Prefix(std::span{szT}, psz);
            szObjectName = TextIO().pfnPlanetName(app, lppl2->idRoute - 1);
            psz = szObjectName.data();
            iRet = std::strcmp(szT, psz);
            break;
        case colPlanetDriverDest:
            if (lppl1->idFling == 0) {
                if (lppl2->idFling == 0) {
                    iRet = 0;
                    break;
                }
                iRet = 1;
                break;
            }
            if (lppl2->idFling == 0) {
                iRet = -1;
                break;
            }
            szObjectName = TextIO().pfnPlanetName(app, lppl1->idFling - 1);
            psz = szObjectName.data();
            CchCopyUtf8Prefix(std::span{szT}, psz);
            szObjectName = TextIO().pfnPlanetName(app, lppl2->idFling - 1);
            psz = szObjectName.data();
            iRet = std::strcmp(szT, psz);
            break;
        case colPlanetName:
            szObjectName = TextIO().pfnPlanetName(app, lppl1->id);
            psz = szObjectName.data();
            CchCopyUtf8Prefix(std::span{szT}, psz);
            szObjectName = TextIO().pfnPlanetName(app, lppl2->id);
            psz = szObjectName.data();
            iRet = std::strcmp(szT, psz);
            break;
        case colPlanetProduction:
            ControlIO().pfnFillPlanetProdLB(app, nullptr, nullptr, lppl1);
            CchCopyUtf8Prefix(std::span{szT}, app.state.szWork);
            ControlIO().pfnFillPlanetProdLB(app, nullptr, nullptr, lppl2);
            iRet = std::strcmp(&szT[6], &app.state.szWork[6]);
            break;
        case colPlanetCapacity:
            iRet = CapacityIO().pfnPctPlanetCapacity(app, lppl1) - CapacityIO().pfnPctPlanetCapacity(app, lppl2);
            break;
        case colPlanetPopulation:
            iRet = LOWORD(lppl1->rgwtMin[3]) - LOWORD(lppl2->rgwtMin[3]);
            break;
        case colPlanetValue:
            iRet = PlanetQueryIO().pfnDesirability(app, lppl1, app.state.idPlayer) - PlanetQueryIO().pfnDesirability(app, lppl2, app.state.idPlayer);
            break;
        case colPlanetMinConc:
            if (iSubsort == 3) {
                i2 = 0;
                i1 = 0;
                for (i = 0; i < 3; i++) {
                    i1 += lppl1->rgMinConc[i];
                    i2 += lppl2->rgMinConc[i];
                }
            } else {
                i1 = lppl1->rgMinConc[iSubsort];
                i2 = lppl2->rgMinConc[iSubsort];
            }
            iRet = i1 - i2;
            break;
        case colPlanetDefense:
            if (lppl1->cDefenses == 0) {
                pct1 = static_cast<float>(0);
            } else {
                CalcPctSurvive(app, lppl1, &pct1, nullptr);
                pct1 = static_cast<float>((static_cast<double>(1.0) - pct1));
            }
            if (lppl2->cDefenses == 0) {
                pct2 = static_cast<float>(0);
            } else {
                CalcPctSurvive(app, lppl2, &pct2, nullptr);
                pct2 = static_cast<float>((static_cast<double>(1.0) - pct2));
            }
            if (static_cast<double>(pct1) < static_cast<double>(pct2)) {
                iRet = -1;
                break;
            }
            if (static_cast<double>(pct1) > static_cast<double>(pct2)) {
                iRet = 1;
                break;
            }
            iRet = 0;
            break;
        case colPlanetFactories:
            iRet = lppl1->cFactories - lppl2->cFactories;
            break;
        case colPlanetMines:
            iRet = lppl1->cMines - lppl2->cMines;
            break;
        case colPlanetMiningRate:
            if (iSubsort == 3) {
                iFirst = 0;
                iLast = 2;
            } else {
                iLast = iSubsort;
                iFirst = iSubsort;
            }
            ColonyIO().pfnEstimateMining(app, lppl1, rgl, -1, 0);
            l1 = 0;
            for (i = iFirst; i <= iLast; i++) {
                l1 += rgl[i];
            }
            ColonyIO().pfnEstimateMining(app, lppl2, rgl, -1, 0);
            l2 = 0;
            for (i = iFirst; i <= iLast; i++) {
                l2 += rgl[i];
            }
            iRet = LOWORD(l1) - LOWORD(l2);
            break;
        case colPlanetMinerals:
            if (iSubsort == 3) {
                iFirst = 0;
                iLast = 2;
            } else {
                iLast = iSubsort;
                iFirst = iSubsort;
            }
            l2 = 0;
            l1 = 0;
            for (i = iFirst; i <= iLast; i++) {
                l1 += lppl1->rgwtMin[i];
                l2 += lppl2->rgwtMin[i];
            }
            l1 -= l2;
            if (l1 < 0) {
                iRet = -1;
                break;
            }
            if (l1 > 0) {
                iRet = 1;
                break;
            }
            iRet = 0;
            break;
        case colPlanetResources:
            iRet = ResearchIO().pfnResources(app, lppl1, app.state.idPlayer) - ResearchIO().pfnResources(app, lppl2, app.state.idPlayer);
        }
        break;
    case rptFleets:
        lpfl1 = app.state.rglpfl[*pid1];
        lpfl2 = app.state.rglpfl[*pid2];
        if (static_cast<std::uint16_t>(icolSort) > colFleetMass)
            break;
        switch (icolSort) {
        case colFleetName:
            szObjectName = PszGetFleetName(app, lpfl1->id);
            psz = szObjectName.data();
            CchCopyUtf8Prefix(std::span{szT}, psz);
            szObjectName = PszGetFleetName(app, lpfl2->id);
            psz = szObjectName.data();
            iRet = std::strcmp(szT, psz);
            break;
        case colFleetLocation:
            if (lpfl1->idPlanet != idPlanetDeepSpace) {
                szObjectName = TextIO().pfnPlanetName(app, lpfl1->idPlanet);
                psz = szObjectName.data();
                CchCopyUtf8Prefix(std::span{szT}, psz);
            } else {
                win32::wsprintfA(szT, TextIO().pfnCompressed(app, idsSpaceDD).data(), lpfl1->pt.x, lpfl1->pt.y);
            }
            if (lpfl2->idPlanet != idPlanetDeepSpace) {
                szObjectName = TextIO().pfnPlanetName(app, lpfl2->idPlanet);
                psz = szObjectName.data();
            } else {
                win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsSpaceDD).data(), lpfl2->pt.x, lpfl2->pt.y);
                psz = app.state.szWork;
            }
            iRet = std::strcmp(szT, psz);
            break;
        case colFleetBattlePlan:
            iRet = lpfl1->iplan - lpfl2->iplan;
            break;
        case colFleetDestination:
            szReportText = PszGetDestName(app, lpfl1, nullptr);
            psz = szReportText.data();
            CchCopyUtf8Prefix(std::span{szT}, psz);
            szReportText = PszGetDestName(app, lpfl2, nullptr);
            psz = szReportText.data();
            iRet = std::strcmp(szT, psz);
            if (iRet < 0) {
                iRet = -1;
                break;
            }
            if (iRet <= 0)
                break;
            iRet = 1;
            break;
        case colFleetEta:
            PszGetETA(app, nullptr, lpfl1, &i1);
            PszGetETA(app, nullptr, lpfl2, &i2);
            iRet = i1 - i2;
            break;
        case colFleetCloak:
            l1 = PctCloakFromLpfl(app, lpfl1);
            l2 = PctCloakFromLpfl(app, lpfl2);
            goto LRetDiff;
        case colFleetComposition:
            l1 = IshdefPrimaryFromLpfl(app, lpfl1, &i1);
            l2 = IshdefPrimaryFromLpfl(app, lpfl2, &i2);
            if (l1 == l2) {
                iRet = lpfl1->rgcsh[l1] - lpfl2->rgcsh[l2];
            } else {
                iRet = std::strcmp(app.state.rgshdef[l1].hul.szClass, app.state.rgshdef[l2].hul.szClass);
                if (iRet == 0) {
                    iRet = lpfl1->rgcsh[l1] - lpfl2->rgcsh[l2];
                }
                if (iRet == 0) {
                    iRet = LOWORD(l1) - LOWORD(l2);
                }
            }
            if (iRet != 0)
                break;
            iRet = i1 - i2;
            break;
        case colFleetTask:
            szReportText = PszGetTaskName(app, lpfl1, &i1);
            psz = szReportText.data();
            CchCopyUtf8Prefix(std::span{szT}, psz);
            szReportText = PszGetTaskName(app, lpfl2, &i2);
            psz = szReportText.data();
            iRet = std::strcmp(szT, psz);
            if (iRet < 0) {
                iRet = -1;
                break;
            }
            if (iRet > 0) {
                iRet = 1;
                break;
            }
            if (i1 > i2) {
                iRet = 1;
                break;
            }
            if (i1 >= i2)
                break;
            iRet = -1;
            break;
        case colFleetFuel:
            iSubsort = -1;
            /* fallthrough */
        case colFleetCargo:
            if (iSubsort == 4) {
                l2 = 0;
                l1 = 0;
                for (i = 0; i <= 3; i++) {
                    l1 += lpfl1->rgwtMin[i];
                    l2 += lpfl2->rgwtMin[i];
                }
                goto LRetDiff;
            }
            if (iSubsort == -1) {
                iSubsort = 4;
            }
            l1 = lpfl1->rgwtMin[iSubsort];
            l2 = lpfl2->rgwtMin[iSubsort];
        LRetDiff:
            l1 -= l2;
            if (l1 < 0) {
                iRet = -1;
            } else if (l1 > 0) {
                iRet = 1;
            } else {
                iRet = 0;
            }
            break;
        case colFleetMass:
            l1 = WtFromLpfl(app, lpfl1);
            l2 = WtFromLpfl(app, lpfl2);
            goto LRetDiff;
        case colFleetId:
            l1 = lpfl1->id;
            l2 = lpfl2->id;
            goto LRetDiff;
        }
        break;
    case rptBattles:
        ibtl1 = *pid1;
        ibtl2 = *pid2;
        lpbd1 = BtlDataGet(app, ibtl1);
        lpbd2 = BtlDataGet(app, ibtl2);
        if (!lpbd1 || !lpbd2) {
            iRet = 0;
            break;
        }
        if (static_cast<std::uint16_t>(icolSort) > colBattleTheirsLeft)
            break;
        switch (icolSort) {
        case colBattleLocation:
            if (BdvFromLp(app, lpbd1).idPlanet() != idPlanetNone) {
                szObjectName = TextIO().pfnPlanetName(app, BdvFromLp(app, lpbd1).idPlanet());
                psz = szObjectName.data();
                CchCopyUtf8Prefix(std::span{szT}, psz);
            } else {
                win32::wsprintfA(szT, TextIO().pfnCompressed(app, idsSpaceDD).data(), BdvFromLp(app, lpbd1).pt().x(), BdvFromLp(app, lpbd1).pt().y());
            }
            if (BdvFromLp(app, lpbd2).idPlanet() != idPlanetNone) {
                szObjectName = TextIO().pfnPlanetName(app, BdvFromLp(app, lpbd2).idPlanet());
                psz = szObjectName.data();
            } else {
                win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsSpaceDD).data(), BdvFromLp(app, lpbd2).pt().x(), BdvFromLp(app, lpbd2).pt().y());
                psz = app.state.szWork;
            }
            iRet = std::strcmp(szT, psz);
            break;
        case colBattleStarbase:
            l1 = CBattleUnits(app, lpbd1, static_cast<BattleUnitFlags>(grBuOurUnits | grBuIncludeSb));
            if (l1 == 0) {
                l1 = static_cast<std::int32_t>((CBattleUnits(app, lpbd1, static_cast<BattleUnitFlags>(grBuTheirUnits | grBuIncludeSb)) * 2));
            }
            l2 = CBattleUnits(app, lpbd2, static_cast<BattleUnitFlags>(grBuOurUnits | grBuIncludeSb));
            if (l2 != 0)
                goto LRetDiff;
            l2 = static_cast<std::int32_t>((CBattleUnits(app, lpbd2, static_cast<BattleUnitFlags>(grBuTheirUnits | grBuIncludeSb)) * 2));
            goto LRetDiff;
        case colBattleSides:
            l1 = static_cast<std::uint32_t>(BdvFromLp(app, lpbd1).cplr());
            l2 = static_cast<std::uint32_t>(BdvFromLp(app, lpbd2).cplr());
            goto LRetDiff;
        case colBattleUnarmed:
            i = 11;
            goto BtlUnitsCom;
        case colBattleScout:
            i = 19;
            goto BtlUnitsCom;
        case colBattleWarship:
            i = 35;
            goto BtlUnitsCom;
        case colBattleBomber:
            i = 67;
            goto BtlUnitsCom;
        case colBattleUtility:
            i = 131;
            goto BtlUnitsCom;
        case colBattleUnits:
            i = 255;
            goto BtlUnitsCom;
        case colBattleOurs:
            i = 253;
            goto BtlUnitsCom;
        case colBattleTheirs:
            i = 254;
        BtlUnitsCom:
            l1 = CBattleUnits(app, lpbd1, static_cast<BattleUnitFlags>(i));
            l2 = CBattleUnits(app, lpbd2, static_cast<BattleUnitFlags>(i));
            goto LRetDiff;
        case colBattleOurDead:
        case colBattleTheirDead:
            l1 = CBattleKills(app, lpbd1, icolSort == colBattleOurDead);
            l2 = CBattleKills(app, lpbd2, icolSort == colBattleOurDead);
            goto LRetDiff;
        case colBattleOursLeft:
            i = 253;
        LUnitsLeft:
            l1 = CBattleUnits(app, lpbd1, static_cast<BattleUnitFlags>(i));
            l2 = CBattleUnits(app, lpbd2, static_cast<BattleUnitFlags>(i));
            l1 -= CBattleKills(app, lpbd1, icolSort == colBattleOursLeft);
            l2 -= CBattleKills(app, lpbd2, icolSort == colBattleOursLeft);
            goto LRetDiff;
        case colBattleTheirsLeft:
            i = 254;
            goto LUnitsLeft;
        }
        break;

    case rptEnemyFleets:
        lpfl1 = app.state.rglpfl[*pid1];
        lpfl2 = app.state.rglpfl[*pid2];
        if (static_cast<std::uint16_t>(icolSort) <= colEnemyFleetUtility) {
            switch (icolSort) {
            case colEnemyFleetName:
                szObjectName = PszGetFleetName(app, lpfl1->id);
                psz = szObjectName.data();
                CchCopyUtf8Prefix(std::span{szT}, psz);
                szObjectName = PszGetFleetName(app, lpfl2->id);
                psz = szObjectName.data();
                iRet = std::strcmp(szT, psz);
                break;
            case colEnemyFleetLocation:
                if (lpfl1->idPlanet != idPlanetDeepSpace) {
                    szObjectName = TextIO().pfnPlanetName(app, lpfl1->idPlanet);
                    psz = szObjectName.data();
                    CchCopyUtf8Prefix(std::span{szT}, psz);
                } else {
                    win32::wsprintfA(szT, TextIO().pfnCompressed(app, idsSpaceDD).data(), lpfl1->pt.x, lpfl1->pt.y);
                }
                if (lpfl2->idPlanet != idPlanetDeepSpace) {
                    szObjectName = TextIO().pfnPlanetName(app, lpfl2->idPlanet);
                    psz = szObjectName.data();
                } else {
                    win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsSpaceDD).data(), lpfl2->pt.x, lpfl2->pt.y);
                    psz = app.state.szWork;
                }
                iRet = std::strcmp(szT, psz);
                break;
            case colEnemyFleetComposition:
                l1 = IshdefPrimaryFromLpfl(app, lpfl1, &i1);
                l2 = IshdefPrimaryFromLpfl(app, lpfl2, &i2);
                if (l1 == l2) {
                    iRet = lpfl1->rgcsh[l1] - lpfl2->rgcsh[l2];
                } else {
                    iRet = std::strcmp(app.state.rglpshdef[lpfl1->iPlayer][l1].hul.szClass, app.state.rglpshdef[lpfl2->iPlayer][l2].hul.szClass);
                    if (iRet == 0) {
                        iRet = lpfl1->rgcsh[l1] - lpfl2->rgcsh[l2];
                    }
                    if (iRet == 0) {
                        iRet = LOWORD(l1) - LOWORD(l2);
                    }
                }
                if (iRet != 0)
                    break;
                iRet = i1 - i2;
                break;
            case colEnemyFleetMass:
                l1 = lpfl1->wtFleet();
                l2 = lpfl2->wtFleet();
                goto LRetDiff;
            case colEnemyFleetId:
                l1 = lpfl1->ifl();
                l2 = lpfl2->ifl();
                goto LRetDiff;
            case colEnemyFleetShips:
                l2 = 0;
                l1 = 0;
                for (i = 0; i < 16; i++) {
                    l1 += lpfl1->rgcsh[i];
                    l2 += lpfl2->rgcsh[i];
                }
                goto LRetDiff;
            case colEnemyFleetUnarmed:
                l2 = 0;
                l1 = 0;
                for (i = 0; i < 16; i++) {
                    if (lpfl1->rgcsh[i] != 0) {
                        j = static_cast<HullCategory>(PartQueryIO().pfnHull(app.state.rglpshdef[lpfl1->iPlayer][i].hul.ihuldef)->imdCategory);
                        if (static_cast<std::int16_t>(j) <= hullCatFreighter || static_cast<std::int16_t>(j) >= hullCatMiner) {
                            l1 += lpfl1->rgcsh[i];
                        }
                    }
                    if (lpfl2->rgcsh[i] != 0) {
                        j = static_cast<HullCategory>(PartQueryIO().pfnHull(app.state.rglpshdef[lpfl2->iPlayer][i].hul.ihuldef)->imdCategory);
                        if (static_cast<std::int16_t>(j) <= hullCatFreighter || static_cast<std::int16_t>(j) >= hullCatMiner) {
                            l2 += lpfl2->rgcsh[i];
                        }
                    }
                }
                goto LRetDiff;
            case colEnemyFleetScout:
                j = hullCatScout;
            LEFleetCount:
                l2 = 0;
                l1 = 0;
                for (i = 0; i < 16; i++) {
                    if (lpfl1->rgcsh[i] != 0 && PartQueryIO().pfnHull(app.state.rglpshdef[lpfl1->iPlayer][i].hul.ihuldef)->imdCategory == j) {
                        l1 += lpfl1->rgcsh[i];
                    }
                    if (lpfl2->rgcsh[i] != 0 && PartQueryIO().pfnHull(app.state.rglpshdef[lpfl2->iPlayer][i].hul.ihuldef)->imdCategory == j) {
                        l2 += lpfl2->rgcsh[i];
                    }
                }
                goto LRetDiff;
            case colEnemyFleetWarship:
                j = hullCatWarship;
                goto LEFleetCount;
            case colEnemyFleetBomber:
                j = hullCatBomber;
                goto LEFleetCount;
            case colEnemyFleetUtility:
                j = hullCatUtility;
                goto LEFleetCount;
            case colEnemyFleetWarp:
                if (lpfl1->fdirValid()) {
                    l1 = lpfl1->iwarpFlt();
                } else {
                    l1 = -1;
                }
                if (lpfl2->fdirValid()) {
                    l2 = lpfl2->iwarpFlt();
                    goto LRetDiff;
                }
                l2 = -1;
                goto LRetDiff;
            }
            break;
        }
    }
    if (!fAscending) {
        iRet = -iRet;
    }
    if (iRet == 0 && !fTier2 && (icolSort != app.lpui->vicolSortPrev || iSubsort != app.lpui->viSubsortPrev) && app.lpui->vicolSortPrev >= 0) {
        icolSort = app.lpui->vicolSortPrev;
        iSubsort = app.lpui->viSubsortPrev;
        fAscending = app.lpui->vfAscendingPrev;
        fTier2 = 1;
        goto TryTier2;
    }
    return iRet;
}

void ReportColumnPopup(APPSESSION &app, POINT16 pt, std::int16_t icol, std::int16_t fRightBtn) {
    HDC     hdc;
    char    szT[50];
    char    rgsz[32][50];
    std::int16_t iBase;
    std::int16_t cSubsort;
    std::int16_t j;
    std::int16_t i;
    std::int16_t ibit;
    std::int16_t fccolChange;
    std::int16_t rgcol[32];
    char    szColTitle[50];
    std::int16_t cItems;
    char   *psz[32];
    std::int16_t cch;
    std::int16_t iRet;
    std::int16_t iHide;
    std::int16_t iSortLast;

    cSubsort = 0;
    fccolChange = 0;
    hdc = win32::GetDC(app.lpui->hwndReportDlg);
    DxReportColHdr(app, app.lpui->vprptCur->irpt, icol, szColTitle, hdc);
    cItems = 0;
    for (i = 0; i < 2; i++) {
        cch = TextIO().pfnCchGetString(app, i == 0 ? idsSort : idsReverseSort, std::span{rgsz[cItems]});
        CchCopyUtf8Prefix(std::span{rgsz[cItems]}.subspan(static_cast<std::size_t>(cch)), szColTitle);
        cItems++;
        if ((app.lpui->vprptCur->irpt == rptPlanets && (icol == colPlanetMinConc || icol == colPlanetMinerals || icol == colPlanetMiningRate)) ||
            (app.lpui->vprptCur->irpt == rptFleets && icol == colFleetCargo)) {
            CchCopyUtf8Prefix(std::span{rgsz[cItems]}, rgsz[cItems - 1]);
            rgsz[cItems - 1][0] = 0;
            cItems++;
            for (j = 0; j < (app.lpui->vprptCur->irpt == rptFleets) + 3; j++) {
                CchCopyUtf8Prefix(std::span{rgsz[cItems]}, app.state.rgszMinerals[j]);
                cItems++;
            }
            rgsz[cItems][0] = -1;
            rgsz[cItems][1] = 0;
            cItems++;
            CchCopyUtf8Prefix(std::span{rgsz[cItems]}, TextIO().pfnCompressed(app, idsWeightedAverage).data());
            cItems++;
            rgsz[cItems++][0] = 0;
            cSubsort = 5;
        }
    }
    rgsz[cItems][0] = -1;
    rgsz[cItems][1] = 0;
    cItems++;
    cch = TextIO().pfnCchGetString(app, idsHide, std::span{rgsz[cItems]});
    CchCopyUtf8Prefix(std::span{rgsz[cItems]}.subspan(static_cast<std::size_t>(cch)), szColTitle);
    cch = TextIO().pfnCchGetString(app, idsColumn, std::span{szT});
    CchAppendUtf8Prefix(std::span{rgsz[cItems]}, szT);
    iHide = cItems;
    iSortLast = cItems;
    cItems++;
    rgsz[cItems][0] = -1;
    rgsz[cItems][1] = 0;
    cItems++;
    if (icol == 0) {
        cItems -= 2;
        iHide = -1;
    }
    iBase = cItems;
    i = 0;
    ibit = 1;
    while (i < app.lpui->vprptCur->cFields) {
        if (!(ibit & app.lpui->vprptCur->grbitVisible)) {
            DxReportColHdr(app, app.lpui->vprptCur->irpt, i, szColTitle, hdc);
            cch = TextIO().pfnCchGetString(app, idsShow, std::span{rgsz[cItems]});
            CchCopyUtf8Prefix(std::span{rgsz[cItems]}.subspan(static_cast<std::size_t>(cch)), szColTitle);
            cch = TextIO().pfnCchGetString(app, idsColumn, std::span{szT});
            CchAppendUtf8Prefix(std::span{rgsz[cItems]}, szT);
            rgcol[cItems] = i;
            cItems++;
        }
        i++;
        ibit *= 2;
    }
    if (cItems == iBase) {
        cItems--;
    }
    win32::ReleaseDC(app.lpui->hwndReportDlg, hdc);
    for (i = 0; i < cItems; i++) {
        if (rgsz[i][0] != 0) {
            psz[i] = rgsz[i];
        } else {
            psz[i] = nullptr;
        }
    }
    iRet = ControlIO().pfnPopupMenu(app, app.lpui->hwndReportDlg, pt.x, pt.y, cItems, nullptr, psz, -1, fRightBtn);
    if (iRet >= 0) {
        app.state.gd.fChgReports = 1;
        if (iRet < iSortLast) {
            app.lpui->vicolSortPrev = app.lpui->vprptCur->icolSort;
            app.lpui->viSubsortPrev = app.lpui->vprptCur->iSubsort;
            app.lpui->vfAscendingPrev = app.lpui->vprptCur->fAscending;
            app.lpui->vprptCur->icolSort = icol;
            if (cSubsort == 0) {
                app.lpui->vprptCur->fAscending = iRet == 0;
            } else {
                app.lpui->vprptCur->iSubsort = static_cast<std::int16_t>((iRet - 2)) % (cSubsort + 3 + (app.lpui->vprptCur->irpt == rptFleets));
                if (app.lpui->vprptCur->iSubsort > (app.lpui->vprptCur->irpt == rptFleets) + 3) {
                    app.lpui->vprptCur->iSubsort = (app.lpui->vprptCur->irpt == rptFleets) + 3;
                }
                app.lpui->vprptCur->fAscending = iRet < cSubsort + 2;
            }
            SortReportCache(app, app.lpui->vprptCur->irpt, icol);
        } else if (iRet == iHide) {
            fccolChange = 1;
            app.lpui->vprptCur->grbitVisible &= static_cast<std::int16_t>(~(1 << icol));
        } else if (iRet >= iBase) {
            fccolChange = 1;
            app.lpui->vprptCur->grbitVisible |= static_cast<std::int16_t>((1 << rgcol[iRet]));
        }
        if (fccolChange) {
            SetHScrollBar(app);
        }
        ControlIO().pfnInvalidateRect(app.lpui->hwndReportDlg, nullptr, 1);
    }
    return;
}

void InvalidateReport(APPSESSION &app, ReportType irpt, std::int16_t fReload) {
    std::int16_t   fResetRpt;
    std::int16_t   fClearRpt;
    RPT      *prptSav;
    std::uint16_t *lprgidSav;
    RECT      rc;

    fClearRpt = 0;
    fResetRpt = 0;
    if (!app.state.gd.fGeneratingTurn && !app.state.fAi) {
        if (app.lpui->hwndReportDlg && irpt == app.lpui->vprptCur->irpt) {
            ControlIO().pfnGetClientRect(app.lpui->hwndReportDlg, &rc);
            if (fReload != 2) {
                rc.top = app.state.dyArial8 + 6;
                rc.bottom = (app.state.dyArial8 + 4) * app.lpui->vprptCur->cRowsVis + rc.top;
            }
            ControlIO().pfnInvalidateRect(app.lpui->hwndReportDlg, &rc, fReload == 2);
            app.state.gd.fRptSafeDraw = 1;
        } else if (!app.lpui->vprptCur) {
            fClearRpt = fReload;
            if (irpt == rptPlanets) {
                app.lpui->vprptCur = &app.state.vrptPlanet;
            } else {
                app.lpui->vprptCur = &app.state.vrptFleet;
            }
        } else if (fReload && app.lpui->vprptCur->irpt != irpt) {
            lprgidSav = app.lpui->vlprgidRep;
            prptSav = app.lpui->vprptCur;
            fResetRpt = 1;
            if (irpt == rptPlanets) {
                app.lpui->vprptCur = &app.state.vrptPlanet;
            } else {
                app.lpui->vprptCur = &app.state.vrptFleet;
            }
        }
        app.lpui->vprptCur->fCached = 0;
        if (fReload) {
            SortReportCache(app, app.lpui->vprptCur->irpt, app.lpui->vprptCur->icolSort);
        }
        if (fClearRpt) {
            app.lpui->vprptCur = nullptr;
        } else if (fResetRpt) {
            app.lpui->vprptCur = prptSav;
            app.lpui->vlprgidRep = lprgidSav;
        } else if (fReload && app.lpui->hwndReportDlg && irpt == app.lpui->vprptCur->irpt) {
            SetReportScrollInfo(app.lpui->vprptCur->hwndVScroll, app.lpui->vprptCur->cRows - app.lpui->vprptCur->cRowsVis,
                                app.lpui->vprptCur->cRowsVis, app.lpui->vprptCur->irowFirst, 0);
            ControlIO().pfnInvalidateRect(app.lpui->hwndReportDlg, nullptr, 1);
        }
    }
    return;
}

void ExecuteReportClick(APPSESSION &app, POINT16 pt, ReportType irpt, std::int16_t icol, std::int16_t irow) {
    HDC      hdc;
    std::uint8_t *lpbd;
    PLANET  *lppl;
    std::int16_t  i;
    FLEET   *lpfl;
    std::int16_t  ibit;
    std::int32_t  rglQuan[4];
    std::int16_t  xCur;
    std::int16_t  dxOffset;
    SCAN     scan;

    hdc = win32::GetDC(app.lpui->hwndReportDlg);
    switch (irpt) {
    case rptPlanets:
        lppl = app.state.lpPlanets + app.state.vlprgidPlanet[irow];
        if (app.lpui->hwndProdDlg) {
            ControlIO().pfnMessageBeep(win32::MB_OK);
            break;
        }
        PresentationIO().pfnSelectPlanet(app, 0, lppl->id);
        PresentationIO().pfnInvalidateReport(app, rptPlanets, 0);
        switch (icol) {
        case colPlanetName:
            if (lppl->fStarbase && pt.x > app.lpui->vprptCur->rgbdx[0] * 2 - 8) {
            LShowStarbase:
                app.lpui->GlobalPD.grPopup = grPopupShdef;
                app.lpui->GlobalPD.lpshdef = app.state.rglpshdefSB[app.state.idPlayer] + app.state.sel.pl.isb;
                app.lpui->GlobalPD.fHideCounts = 0;
                app.lpui->GlobalPD.fShowDamage = 1;
                app.lpui->GlobalPD.SetFToken(0);
                app.lpui->GlobalPD.fSummary = 0;
                Popup(app, app.lpui->hwndReportDlg, pt.x, pt.y);
            }
            break;
        case colPlanetStarbase:
            if (!lppl->fStarbase)
                break;
            goto LShowStarbase;
        case colPlanetPopulation:
        case colPlanetValue:
            app.lpui->GlobalPD.grPopup = grPopupPlanet;
            app.lpui->GlobalPD.idPlanet = app.state.sel.pl.id;
            Popup(app, app.lpui->hwndReportDlg, pt.x, pt.y);
            break;
        case colPlanetDefense:
            if (app.state.sel.pl.cDefenses == 0)
                break;
            FGetBestDefensePart(app, &app.lpui->GlobalPD.part);
            app.lpui->GlobalPD.grPopup = grPopupComponent;
            Popup(app, app.lpui->hwndReportDlg, pt.x, pt.y);
            break;
        case colPlanetResources:
            app.lpui->GlobalPD.grPopup = grPopupResources;
            app.lpui->GlobalPD.idPlanet = app.state.sel.pl.id;
            app.lpui->GlobalPD.iPlanetVar = app.lpui->GlobalPD.iPlanVal = ResearchIO().pfnResources(app, &app.state.sel.pl, app.state.idPlayer);
            if (!app.state.sel.pl.fNoResearch) {
                app.lpui->GlobalPD.iPlanVal -= win32::MulDiv(app.lpui->GlobalPD.iPlanetVar, app.state.rgplr[app.state.idPlayer].pctResearch, 100);
            }
            Popup(app, app.lpui->hwndReportDlg, pt.x, pt.y);
            break;
        case colPlanetMinerals:
        case colPlanetMiningRate:
        case colPlanetMinConc:
            xCur = 2;
            i = 0;
            ibit = 1;
            while (i < icol) {
                if ((ibit & app.lpui->vprptCur->grbitVisible) && (i == 0 || i >= app.lpui->vprptCur->cFieldFirst)) {
                    xCur += app.lpui->vprptCur->rgbdx[i] * 2;
                }
                i++;
                ibit *= 2;
            }
            dxOffset = pt.x - xCur;
            for (i = 1; i <= 3 && static_cast<std::int16_t>((i * 2 * app.lpui->vprptCur->rgbdx[icol])) / 3 <= dxOffset; i++) {
            }
            i--;
            app.lpui->GlobalPD.grPopup = grPopupMineral;
            app.lpui->GlobalPD.SetRgi(0, i);
            app.lpui->GlobalPD.SetRgi(2, app.state.sel.pl.rgwtMin[i]);
            app.lpui->GlobalPD.SetRgi(3, static_cast<std::uint32_t>(app.state.sel.pl.rgMinConc[i]));
            ColonyIO().pfnEstimateMining(app, &app.state.sel.pl, rglQuan, -1, 0);
            app.lpui->GlobalPD.SetRgi(4, rglQuan[i]);
            app.lpui->GlobalPD.SetRgi(1, app.state.sel.pl.fHomeworld);
            Popup(app, app.lpui->hwndReportDlg, pt.x, pt.y);
            break;
        case colPlanetProduction:
            if (app.lpui->hwndProdDlg)
                break;
            ChangeProduction(app, 0);
            break;
        case colPlanetMines:
        case colPlanetFactories:
            app.lpui->GlobalPD.grPopup = grPopupPlanetIndustry;
            app.lpui->GlobalPD.idPlan = app.state.sel.pl.id;
            app.lpui->GlobalPD.SetFFactory(icol == colPlanetFactories);
            if (app.lpui->GlobalPD.fFactory) {
                app.lpui->GlobalPD.cMax = ColonyIO().pfnMaxFactories(app, &app.state.sel.pl, app.state.idPlayer);
                app.lpui->GlobalPD.cCur = app.state.sel.pl.cFactories;
                app.lpui->GlobalPD.cOperate = ColonyIO().pfnOperableFactories(app, &app.state.sel.pl, app.state.idPlayer, 0);
            } else {
                app.lpui->GlobalPD.cMax = ColonyIO().pfnMaxMines(app, &app.state.sel.pl, app.state.idPlayer);
                app.lpui->GlobalPD.cCur = app.state.sel.pl.cMines;
                app.lpui->GlobalPD.cOperate = ColonyIO().pfnOperableMines(app, &app.state.sel.pl, app.state.idPlayer, 0);
            }
            Popup(app, app.lpui->hwndReportDlg, pt.x, pt.y);
        }
        break;
    case rptFleets:
        lpfl = app.state.rglpfl[app.state.vlprgidFleet[irow]];
        if (app.lpui->mdXferDlg != mdXferNone) {
            ControlIO().pfnMessageBeep(win32::MB_OK);
            break;
        }
        PresentationIO().pfnSelectFleet(app, 0, lpfl->id);
        PresentationIO().pfnInvalidateReport(app, rptFleets, 0);
        switch (icol) {
        case colFleetFuel:
        case colFleetCargo:
            if (app.lpui->mdXferDlg != mdXferNone)
                break;
            if (app.state.sel.fl.idPlanet != idPlanetDeepSpace) {
                TransferStuff(app, app.state.sel.fl.id, grobjFleet, app.state.sel.fl.idPlanet, grobjPlanet, mdXferCargo);
                break;
            }
            TransferStuff(app, app.state.sel.fl.id, grobjFleet, -1, grobjOther, mdXferCargo);
            break;
        case colFleetDestination:
        case colFleetEta:
        case colFleetTask:
            if (FDestIsWP0(lpfl))
                break;
            ControlIO().pfnSendMessage(app.lpui->hwndShipLB, win32::LB_SETCURSEL, 1, 0);
            SetScanWp(app, 1);
            break;
        case colFleetComposition:
            app.lpui->GlobalPD.grPopup = grPopupFleet;
            app.lpui->GlobalPD.fRedDamage = 1;
            app.lpui->GlobalPD.grbit = 0xff;
            app.lpui->GlobalPD.lpfl = lpfl;
            Popup(app, app.lpui->hwndReportDlg, pt.x, pt.y);
        }
        break;
    case rptBattles:
        lpbd = BtlDataGet(app, app.lpui->vlprgidMisc[irow]);
        if (!lpbd)
            break;
        if (BdvFromLp(app, lpbd).pt().x() == app.state.sel.scan.pt.x && BdvFromLp(app, lpbd).pt().y() == app.state.sel.scan.pt.y)
            goto LDisplayVCRAnyway;

        scan.pt = BdvFromLp(app, lpbd).pt().ReadValue();
        scan.grobj = static_cast<GrobjClass>(grobjPlanet | grobjFleet | grobjOther | grobjThing | mdExact);
        ChangeScanSel(app, &scan, 0);
        CtrPointScan(app, scan.pt, 1);
        PresentationIO().pfnInvalidateReport(app, rptPlanets, 0);
        if (BdvFromLp(app, lpbd).pt().x() == app.state.sel.scan.pt.x && BdvFromLp(app, lpbd).pt().y() == app.state.sel.scan.pt.y)
            break;

    LDisplayVCRAnyway:
        if (app.lpui->hwndVCRDlg)
            break;
        BattleVCR(app, BdvFromLp(app, lpbd).id());
        break;
    case rptEnemyFleets:
        if (app.lpui->vprptCur == &app.state.vrptEFleet && irow >= -2) {
            if (irow < 0) {
                if (irow == -1) {
                    app.lpui->irowEFleetCur++;
                } else {
                    app.lpui->irowEFleetCur--;
                }
                if (app.lpui->irowEFleetCur >= app.lpui->vprptCur->cRows) {
                    app.lpui->irowEFleetCur = 0;
                } else if (app.lpui->irowEFleetCur < 0) {
                    app.lpui->irowEFleetCur = app.lpui->vprptCur->cRows - 1;
                }
                irow = app.lpui->irowEFleetCur;
            }
            lpfl = app.state.rglpfl[app.lpui->vlprgidMisc[irow]];
            if (app.lpui->mdXferDlg != mdXferNone) {
                ControlIO().pfnMessageBeep(win32::MB_OK);
            } else {
                FFindNearestObject(app, lpfl->pt, grobjFleet, &scan);
                scan.ifl = app.lpui->vlprgidMisc[irow];
                ChangeScanSel(app, &scan, 2);
                FEnsurePointOnScreen(app, lpfl->pt, 1);
                app.lpui->irowEFleetCur = irow;
                PresentationIO().pfnInvalidateReport(app, rptEnemyFleets, 0);
            }
        }
    }
    win32::ReleaseDC(app.lpui->hwndReportDlg, hdc);
    return;
}

INT_PTR  PrintMapDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    std::int16_t i;
    RECT    rc;
    HWND    hwndEdit;

    switch (msg) {
    case win32::WM_INITDIALOG:
        for (i = 0; i < 2; i++) {
            hwndEdit = ControlIO().pfnGetDlgItem(hwnd, i + 268);
            ControlIO().pfnSendMessage(hwndEdit, win32::EM_LIMITTEXT, 1, 0);
            ControlIO().pfnSendMessage(hwndEdit, win32::WM_SETFONT, reinterpret_cast<WPARAM>(app.lpui->rghfontArial8[1]), 0);
            app.state.szWork[0] = app.lpui->vrgcPrintMapPage[i] + '0';
            app.state.szWork[1] = 0;
            ControlIO().pfnSetWindowText(hwndEdit, app.state.szWork);
        }
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyPrintMapDlg, 1);
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
                for (i = 0; i < 2; i++) {
                    hwndEdit = ControlIO().pfnGetDlgItem(hwnd, i + 268);
                    ControlIO().pfnGetWindowText(hwndEdit, app.state.szWork, 10);
                    if (app.state.szWork[0] == 0 || app.state.szWork[1] != 0 || app.state.szWork[0] <= '0' || app.state.szWork[0] > '9') {
                        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsMustSpecifyNumberBetween19, {}).data(), win32::MB_ICONHAND);
                        ControlIO().pfnSetFocus(hwndEdit);
                        break;
                    }
                    app.lpui->vrgcPrintMapPage[i] = app.state.szWork[0] - '0';
                }
            }
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyPrintMapDlg, 0);
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhPrintingAMapOfTheUniverse);
            return 1;
        case IDC_PRINT_MAP_PAGES_X:
        case IDC_PRINT_MAP_PAGES_Y:
            if (HIWORD(wParam) == 0x400) {
                ControlIO().pfnGetWindowText(reinterpret_cast<HWND>(lParam), app.state.szWork, 10);
                if (app.state.szWork[0] != 0 && (app.state.szWork[0] <= '0' || app.state.szWork[0] > '9')) {
                    ControlIO().pfnMessageBeep(win32::MB_OK);
                    ControlIO().pfnSetWindowText(reinterpret_cast<HWND>(lParam), &app.state.szWork[1]);
                }
            }
        }
    }
    return 0;
}

// SetHdcTextColor colours the fleet report text PszGetDestName, PszGetETA
// and CchGetETA return.
void SetHdcTextColor(HDC hdc, std::uint32_t cr) {
    win32::SetTextColor(hdc, cr);
    return;
}

void ResetReportInputSession() noexcept { dWheelRows = dWheelCols = 0; }
