#include "win.h"
#include "control_io.h"
#include "ownership.h"

constexpr auto wmRefitPlanetPane = (win32::WM_USER + 37);

typedef struct PLANETPANE {
    HWND hwndContent;
    int x, y, dxContent, dyContent, dxOuter, dyOuter;
    int idSelection, grobjSelection;
    int16_t dWheel;
    BOOL fRefitting, fPending;
} PLANETPANE;

static PLANETPANE *LpPlanetPane(HWND hwnd) {
    return (PLANETPANE *)win32::GetWindowLongPtrA(hwnd, win32::GWLP_USERDATA);
}

static void SetPlanetPaneScroll(HWND hwnd, int iAxis, SCROLLINFO *lpsi) {
    SCROLLINFO si = {sizeof(si), win32::SIF_RANGE | win32::SIF_PAGE | win32::SIF_POS, 0, 0, 0, 0, 0};

    if (!win32::GetScrollInfo(hwnd, iAxis, &si) || si.nMin != lpsi->nMin || si.nMax != lpsi->nMax ||
        si.nPage != lpsi->nPage || si.nPos != lpsi->nPos)
        win32::SetScrollInfo(hwnd, iAxis, lpsi, 1);
}

static void RefitPlanetPane(HWND hwnd) {
    PLANETPANE *lp = LpPlanetPane(hwnd);
    TILE *rgtile;
    RECT rc, rcOuter, rcContent;
    POINT pt;
    SCROLLINFO si = {sizeof(si), win32::SIF_RANGE | win32::SIF_PAGE | win32::SIF_POS, 0, 0, 0, 0, 0};
    int dx = 198, dy = 0, cTile, i, dxOuter, dyOuter, dxBar, dyBar;
    BOOL fVertical, fHorizontal;

    if (!lp || lp->fRefitting || !win32::IsWindow(lp->hwndContent)) return;
    lp->fRefitting = 1; lp->fPending = 0;
    rgtile = sel.grobj == grobjFleet ? rgtileShip : rgtilePlanet;
    cTile = sel.grobj == grobjFleet ? 7 : 6;
    if (sel.id != -1) {
        for (i = 0; i < cTile; i++) {
            dx = (((dx) > ((rgtile[i].iCol + 1) * 198)) ? (dx) : ((rgtile[i].iCol + 1) * 198));
            dy = (((dy) > (rgtile[i].yTop + (rgtile[i].fPopped ? rgtile[i].dyFull : dyArial8 + 3) + 4)) ? (dy) : (rgtile[i].yTop + (rgtile[i].fPopped ? rgtile[i].dyFull : dyArial8 + 3) + 4));
        }
    }
    win32::GetWindowRect(hwnd, &rcOuter);
    dxOuter = rcOuter.right - rcOuter.left; dyOuter = rcOuter.bottom - rcOuter.top;
    if (dx != lp->dxContent || dy != lp->dyContent || dxOuter != lp->dxOuter || dyOuter != lp->dyOuter) {
        dxBar = win32::GetSystemMetrics(win32::SM_CXVSCROLL); dyBar = win32::GetSystemMetrics(win32::SM_CYHSCROLL);
        fHorizontal = dx > dxOuter; fVertical = dy > dyOuter;
        // One bar can make the other necessary. Start with the full viewport
        // rather than its old client size, so both bars can disappear again.
        for (i = 0; i < 2; i++) {
            fHorizontal = dx > dxOuter - (fVertical ? dxBar : 0);
            fVertical = dy > dyOuter - (fHorizontal ? dyBar : 0);
        }
        win32::ShowScrollBar(hwnd, win32::SB_VERT, fVertical);
        win32::ShowScrollBar(hwnd, win32::SB_HORZ, fHorizontal);
        lp->dxContent = dx; lp->dyContent = dy; lp->dxOuter = dxOuter; lp->dyOuter = dyOuter;
    }
    ControlIO().pfnGetClientRect(hwnd, &rc);
    if (lp->idSelection != sel.id || lp->grobjSelection != sel.grobj) {
        lp->x = lp->y = 0; lp->dWheel = 0;
        lp->idSelection = sel.id; lp->grobjSelection = sel.grobj;
    }
    lp->x = (((0) > (StarsMin(lp->x, dx - rc.right))) ? (0) : (StarsMin(lp->x, dx - rc.right))); lp->y = (((0) > (StarsMin(lp->y, dy - rc.bottom))) ? (0) : (StarsMin(lp->y, dy - rc.bottom)));
    si.nMax = StarsMax(0, dx - 1); si.nPage = StarsMax(1, rc.right); si.nPos = lp->x;
    SetPlanetPaneScroll(hwnd, win32::SB_HORZ, &si);
    si.nMax = StarsMax(0, dy - 1); si.nPage = StarsMax(1, rc.bottom); si.nPos = lp->y;
    SetPlanetPaneScroll(hwnd, win32::SB_VERT, &si);
    win32::GetWindowRect(lp->hwndContent, &rcContent);
    pt = POINT{rcContent.left, rcContent.top};
    win32::ScreenToClient(hwnd, &pt);
    if (rcContent.right - rcContent.left != StarsMax(dx, rc.right) || rcContent.bottom - rcContent.top != StarsMax(dy, rc.bottom)) {
        for (i = 0; i < cTile; i++) rgtile[i].fFixCtls = 1;
        ui::MoveWindow(lp->hwndContent, -lp->x, -lp->y, StarsMax(dx, rc.right), StarsMax(dy, rc.bottom), 1);
    } else if (pt.x != -lp->x || pt.y != -lp->y) {
        ControlIO().pfnSetWindowPos(lp->hwndContent, NULL, -lp->x, -lp->y, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
    }
    lp->fRefitting = 0;
}

LRESULT  PlanetPaneWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PLANETPANE *lp = LpPlanetPane(hwnd);
    RECT rc;
    int iAxis, iCode, iPosition, d, c;

    switch (message) {
    case win32::WM_CREATE: {
        NATIVEOWNER<PLANETPANE, std::free> pane(static_cast<PLANETPANE *>(calloc(1, sizeof(*lp))));
        lp = pane.get();
        if (!lp) return -1;
        win32::SetWindowLongPtrA(hwnd, win32::GWLP_USERDATA, (LONG_PTR)lp);
        pane.release();
        lp->hwndContent = ui::CreateWindowEx(0, szPlanet, NULL, win32::WS_CHILD | win32::WS_VISIBLE, 0, 0, 198, 1, hwnd, NULL, hInst, NULL);
        return lp->hwndContent ? 0 : -1;
    }
    case win32::WM_SIZE:
    case wmRefitPlanetPane:
        RefitPlanetPane(hwnd);
        return 0;
    case win32::WM_HSCROLL:
    case win32::WM_VSCROLL:
        if (!lp) return 0;
        iAxis = message == win32::WM_VSCROLL ? win32::SB_VERT : win32::SB_HORZ;
        iCode = LOWORD(wParam); iPosition = iAxis == win32::SB_VERT ? lp->y : lp->x;
        ControlIO().pfnGetClientRect(hwnd, &rc); d = iAxis == win32::SB_VERT ? rc.bottom : rc.right;
        if (iCode == win32::SB_LINEUP) iPosition -= StarsMax(1, dyArial8 + 4);
        else if (iCode == win32::SB_LINEDOWN) iPosition += StarsMax(1, dyArial8 + 4);
        else if (iCode == win32::SB_PAGEUP) iPosition -= d;
        else if (iCode == win32::SB_PAGEDOWN) iPosition += d;
        else if (iCode == win32::SB_TOP) iPosition = 0;
        else if (iCode == win32::SB_BOTTOM) iPosition = iAxis == win32::SB_VERT ? lp->dyContent : lp->dxContent;
        else if (iCode == win32::SB_THUMBPOSITION || iCode == win32::SB_THUMBTRACK) iPosition = HIWORD(wParam);
        else return 0;
        if (iAxis == win32::SB_VERT) lp->y = iPosition; else lp->x = iPosition;
        RefitPlanetPane(hwnd);
        return 0;
    case win32::WM_MOUSEWHEEL:
    case win32::WM_MOUSEHWHEEL:
        if (!lp) return 0;
        c = CWheelNotches(&lp->dWheel, (int16_t)HIWORD(wParam));
        if (c == 0) return 0;
        ControlIO().pfnGetClientRect(hwnd, &rc);
        d = StarsMax(1, dyArial8 + 4);
        iAxis = message == win32::WM_MOUSEHWHEEL || (LOWORD(wParam) & win32::MK_SHIFT) ? win32::SB_HORZ : win32::SB_VERT;
        d = c * CWheelLines((((1) > ((iAxis == win32::SB_VERT ? rc.bottom : rc.right) / d)) ? (1) : ((iAxis == win32::SB_VERT ? rc.bottom : rc.right) / d))) * d;
        if (message == win32::WM_MOUSEHWHEEL) lp->x += d;
        else if (LOWORD(wParam) & win32::MK_SHIFT) lp->x -= d;
        else lp->y -= d;
        RefitPlanetPane(hwnd);
        return 0;
    case win32::WM_DESTROY: {
        NATIVEOWNER<PLANETPANE, std::free> pane(lp);
        win32::SetWindowLongPtrA(hwnd, win32::GWLP_USERDATA, 0);
        return 0;
    }
    }
    return ui::DefWindowProc(hwnd, message, wParam, lParam);
}

HWND CreatePlanetPane(HWND hwndParent) {
    HWND hwnd;
    PLANETPANE *lp;

    hwnd = ui::CreateWindowEx(0, "StarsPlanetPane", NULL, win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_HSCROLL | win32::WS_VSCROLL | win32::WS_CLIPCHILDREN,
                        -500, -500, 10, 10, hwndParent, NULL, hInst, NULL);
    lp = hwnd ? LpPlanetPane(hwnd) : NULL;
    return lp ? lp->hwndContent : NULL;
}

void MovePlanetPane(int x, int y, int dx, int dy) {
    if (win32::IsWindow(hwndPlanet)) ui::MoveWindow(win32::GetParent(hwndPlanet), x, y, dx, dy, 1);
}

void QueuePlanetPaneLayout(void) {
    HWND hwnd;
    PLANETPANE *lp;

    if (!win32::IsWindow(hwndPlanet)) return;
    hwnd = win32::GetParent(hwndPlanet); lp = LpPlanetPane(hwnd);
    // Painting can hold a DC on the old canvas. Defer size changes until that
    // DC has been released, and coalesce repeated tile draws into one refit.
    if (lp && !lp->fPending && !lp->fRefitting) {
        lp->fPending = win32::PostMessageA(hwnd, wmRefitPlanetPane, 0, 0);
    }
}

void ClosePlanetPane(void) {
    HWND hwnd;

    if (!win32::IsWindow(hwndPlanet)) return;
    hwnd = win32::GetParent(hwndPlanet);
    // WM_QUIT can leave the frame alive. Destroy the owned pane while game
    // state and brushes still exist; final SDL cleanup does not call WNDPROCs.
    ControlIO().pfnDestroyWindow(hwnd);
    hwndPlanet = NULL;
}
