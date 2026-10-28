#include "win.h"

#define wmRefitPlanetPane (WM_USER + 37)

typedef struct PLANETPANE {
    HWND hwndContent;
    int x, y, dxContent, dyContent, dxOuter, dyOuter;
    int idSelection, grobjSelection;
    int16_t dWheel;
    BOOL fRefitting, fPending;
} PLANETPANE;

static PLANETPANE *LpPlanetPane(HWND hwnd) {
    return (PLANETPANE *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

static void SetPlanetPaneScroll(HWND hwnd, int iAxis, SCROLLINFO *lpsi) {
    SCROLLINFO si = {sizeof(si), SIF_RANGE | SIF_PAGE | SIF_POS, 0, 0, 0, 0, 0};

    if (!GetScrollInfo(hwnd, iAxis, &si) || si.nMin != lpsi->nMin || si.nMax != lpsi->nMax ||
        si.nPage != lpsi->nPage || si.nPos != lpsi->nPos)
        SetScrollInfo(hwnd, iAxis, lpsi, TRUE);
}

static void RefitPlanetPane(HWND hwnd) {
    PLANETPANE *lp = LpPlanetPane(hwnd);
    TILE *rgtile;
    RECT rc, rcOuter, rcContent;
    POINT pt;
    SCROLLINFO si = {sizeof(si), SIF_RANGE | SIF_PAGE | SIF_POS, 0, 0, 0, 0, 0};
    int dx = 198, dy = 0, cTile, i, dxOuter, dyOuter, dxBar, dyBar;
    BOOL fVertical, fHorizontal;

    if (!lp || lp->fRefitting || !IsWindow(lp->hwndContent)) return;
    lp->fRefitting = TRUE; lp->fPending = FALSE;
    rgtile = sel.grobj == grobjFleet ? rgtileShip : rgtilePlanet;
    cTile = sel.grobj == grobjFleet ? 7 : 6;
    if (sel.id != -1) {
        for (i = 0; i < cTile; i++) {
            dx = max(dx, (rgtile[i].iCol + 1) * 198);
            dy = max(dy, rgtile[i].yTop + (rgtile[i].fPopped ? rgtile[i].dyFull : dyArial8 + 3) + 4);
        }
    }
    GetWindowRect(hwnd, &rcOuter);
    dxOuter = rcOuter.right - rcOuter.left; dyOuter = rcOuter.bottom - rcOuter.top;
    if (dx != lp->dxContent || dy != lp->dyContent || dxOuter != lp->dxOuter || dyOuter != lp->dyOuter) {
        dxBar = GetSystemMetrics(SM_CXVSCROLL); dyBar = GetSystemMetrics(SM_CYHSCROLL);
        fHorizontal = dx > dxOuter; fVertical = dy > dyOuter;
        // One bar can make the other necessary. Start with the full viewport
        // rather than its old client size, so both bars can disappear again.
        for (i = 0; i < 2; i++) {
            fHorizontal = dx > dxOuter - (fVertical ? dxBar : 0);
            fVertical = dy > dyOuter - (fHorizontal ? dyBar : 0);
        }
        ShowScrollBar(hwnd, SB_VERT, fVertical);
        ShowScrollBar(hwnd, SB_HORZ, fHorizontal);
        lp->dxContent = dx; lp->dyContent = dy; lp->dxOuter = dxOuter; lp->dyOuter = dyOuter;
    }
    GetClientRect(hwnd, &rc);
    if (lp->idSelection != sel.id || lp->grobjSelection != sel.grobj) {
        lp->x = lp->y = 0; lp->dWheel = 0;
        lp->idSelection = sel.id; lp->grobjSelection = sel.grobj;
    }
    lp->x = max(0, min(lp->x, dx - rc.right)); lp->y = max(0, min(lp->y, dy - rc.bottom));
    si.nMax = max(0, dx - 1); si.nPage = max(1, rc.right); si.nPos = lp->x;
    SetPlanetPaneScroll(hwnd, SB_HORZ, &si);
    si.nMax = max(0, dy - 1); si.nPage = max(1, rc.bottom); si.nPos = lp->y;
    SetPlanetPaneScroll(hwnd, SB_VERT, &si);
    GetWindowRect(lp->hwndContent, &rcContent);
    pt = (POINT){rcContent.left, rcContent.top};
    ScreenToClient(hwnd, &pt);
    if (rcContent.right - rcContent.left != max(dx, rc.right) || rcContent.bottom - rcContent.top != max(dy, rc.bottom)) {
        for (i = 0; i < cTile; i++) rgtile[i].fFixCtls = TRUE;
        MoveWindow(lp->hwndContent, -lp->x, -lp->y, max(dx, rc.right), max(dy, rc.bottom), TRUE);
    } else if (pt.x != -lp->x || pt.y != -lp->y) {
        SetWindowPos(lp->hwndContent, NULL, -lp->x, -lp->y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    lp->fRefitting = FALSE;
}

LRESULT CALLBACK PlanetPaneWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PLANETPANE *lp = LpPlanetPane(hwnd);
    RECT rc;
    int iAxis, iCode, iPosition, d, c;

    switch (message) {
    case WM_CREATE:
        lp = (PLANETPANE *)calloc(1, sizeof(*lp));
        if (!lp) return -1;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lp);
        lp->hwndContent = CreateWindow(szPlanet, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 198, 1, hwnd, NULL, hInst, NULL);
        return lp->hwndContent ? 0 : -1;
    case WM_SIZE:
    case wmRefitPlanetPane:
        RefitPlanetPane(hwnd);
        return 0;
    case WM_HSCROLL:
    case WM_VSCROLL:
        if (!lp) return 0;
        iAxis = message == WM_VSCROLL ? SB_VERT : SB_HORZ;
        iCode = LOWORD(wParam); iPosition = iAxis == SB_VERT ? lp->y : lp->x;
        GetClientRect(hwnd, &rc); d = iAxis == SB_VERT ? rc.bottom : rc.right;
        if (iCode == SB_LINEUP) iPosition -= max(1, dyArial8 + 4);
        else if (iCode == SB_LINEDOWN) iPosition += max(1, dyArial8 + 4);
        else if (iCode == SB_PAGEUP) iPosition -= d;
        else if (iCode == SB_PAGEDOWN) iPosition += d;
        else if (iCode == SB_TOP) iPosition = 0;
        else if (iCode == SB_BOTTOM) iPosition = iAxis == SB_VERT ? lp->dyContent : lp->dxContent;
        else if (iCode == SB_THUMBPOSITION || iCode == SB_THUMBTRACK) iPosition = HIWORD(wParam);
        else return 0;
        if (iAxis == SB_VERT) lp->y = iPosition; else lp->x = iPosition;
        RefitPlanetPane(hwnd);
        return 0;
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
        if (!lp) return 0;
        c = CWheelNotches(&lp->dWheel, (int16_t)HIWORD(wParam));
        if (c == 0) return 0;
        GetClientRect(hwnd, &rc);
        d = max(1, dyArial8 + 4);
        iAxis = message == WM_MOUSEHWHEEL || (LOWORD(wParam) & MK_SHIFT) ? SB_HORZ : SB_VERT;
        d = c * CWheelLines(max(1, (iAxis == SB_VERT ? rc.bottom : rc.right) / d)) * d;
        if (message == WM_MOUSEHWHEEL) lp->x += d;
        else if (LOWORD(wParam) & MK_SHIFT) lp->x -= d;
        else lp->y -= d;
        RefitPlanetPane(hwnd);
        return 0;
    case WM_DESTROY:
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        free(lp);
        return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

HWND CreatePlanetPane(HWND hwndParent) {
    HWND hwnd;
    PLANETPANE *lp;

    hwnd = CreateWindow("StarsPlanetPane", NULL, WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_CLIPCHILDREN,
                        -500, -500, 10, 10, hwndParent, NULL, hInst, NULL);
    lp = hwnd ? LpPlanetPane(hwnd) : NULL;
    return lp ? lp->hwndContent : NULL;
}

void MovePlanetPane(int x, int y, int dx, int dy) {
    if (IsWindow(hwndPlanet)) MoveWindow(GetParent(hwndPlanet), x, y, dx, dy, TRUE);
}

void QueuePlanetPaneLayout(void) {
    HWND hwnd;
    PLANETPANE *lp;

    if (!IsWindow(hwndPlanet)) return;
    hwnd = GetParent(hwndPlanet); lp = LpPlanetPane(hwnd);
    // Painting can hold a DC on the old canvas. Defer size changes until that
    // DC has been released, and coalesce repeated tile draws into one refit.
    if (lp && !lp->fPending && !lp->fRefitting) {
        lp->fPending = PostMessage(hwnd, wmRefitPlanetPane, 0, 0);
    }
}

void ClosePlanetPane(void) {
    HWND hwnd;

    if (!IsWindow(hwndPlanet)) return;
    hwnd = GetParent(hwndPlanet);
    // WM_QUIT can leave the frame alive. Destroy the owned pane while game
    // state and brushes still exist; final SDL cleanup does not call WNDPROCs.
    DestroyWindow(hwnd);
    hwndPlanet = NULL;
}
