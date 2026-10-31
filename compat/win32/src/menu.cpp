#include "display_io.h"
#include "cleanup.h"
#include "ownership.h"
#include <limits.h>
#include <new>
#include <utility>
#include <stdlib.h>
#include <string.h>

typedef struct COMPATMENUITEM {
    UINT flags;
    UINT_PTR id;
    NATIVEOWNER<char, SDL_free> psz;
    HMENU hsub;
} COMPATMENUITEM;
struct win32::HMENU__ {
    BOOL fAlive;
    std::unique_ptr<COMPATMENUITEM[]> rgitem;
    int citem;
    HMENU hparent, hnext;
    HWND hwndOwner;
};
static HMENU hmenus;

static HMENU LpMenu(HMENU hmenu) {
    HMENU lp;
    for (lp = hmenus; lp; lp = lp->hnext)
        if (lp == hmenu) return lp->fAlive ? lp : NULL;
    return NULL;
}
static COMPATMENUITEM *LpItem(HMENU hmenu, UINT id, UINT flags, HMENU *lphmenu, int *lpi) {
    HMENU lp = LpMenu(hmenu);
    COMPATMENUITEM *lpitem;
    int i;
    if (!lp) return NULL;
    if (flags & MF_BYPOSITION) {
        if (id >= (UINT)lp->citem) return NULL;
        *lphmenu = lp; *lpi = (int)id;
        return &lp->rgitem[id];
    }
    for (i = 0; i < lp->citem; i++) {
        if (!lp->rgitem[i].hsub && lp->rgitem[i].id == id) {
            *lphmenu = lp; *lpi = i;
            return &lp->rgitem[i];
        }
        if (lp->rgitem[i].hsub) {
            lpitem = LpItem(lp->rgitem[i].hsub, id, flags, lphmenu, lpi);
            if (lpitem) return lpitem;
        }
    }
    return NULL;
}
static BOOL FMenuDepth(HMENU hmenu, int cDepth) {
    HMENU lp = LpMenu(hmenu);
    int i;
    if (!lp || cDepth >= 64) return FALSE;
    for (i = 0; i < lp->citem; i++)
        if (lp->rgitem[i].hsub && !FMenuDepth(lp->rgitem[i].hsub, cDepth + 1)) return FALSE;
    return TRUE;
}
HMENU win32::CreatePopupMenu(void) {
    HMENU lp = new (std::nothrow) win32::HMENU__{};
    if (!lp) return NULL;
    lp->fAlive = TRUE; lp->hnext = hmenus; hmenus = lp;
    return lp;
}
HMENU win32::CreateMenu(void) { return win32::CreatePopupMenu(); }
BOOL win32::IsMenu(HMENU hmenu) { return LpMenu(hmenu) != NULL; }
int win32::GetMenuItemCount(HMENU hmenu) {
    HMENU lp = LpMenu(hmenu);
    return lp ? lp->citem : -1;
}
HMENU win32::GetSubMenu(HMENU hmenu, int i) {
    HMENU lp = LpMenu(hmenu);
    return lp && i >= 0 && i < lp->citem ? LpMenu(lp->rgitem[i].hsub) : NULL;
}
UINT win32::GetMenuItemID(HMENU hmenu, int i) {
    HMENU lp = LpMenu(hmenu);
    return lp && i >= 0 && i < lp->citem && !lp->rgitem[i].hsub ? (UINT)lp->rgitem[i].id : UINT_MAX;
}
BOOL win32::InsertMenuA(HMENU hmenu, UINT pos, UINT flags, UINT_PTR id, LPCSTR psz) {
    HMENU lp = LpMenu(hmenu), hsub = NULL, hp;
    COMPATMENUITEM item = {};
    std::unique_ptr<COMPATMENUITEM[]> rg;
    int i, j, cDepth = 0;
    if (!lp || (flags & ~(MF_BYPOSITION | MF_POPUP | MF_SEPARATOR | MF_CHECKED | MF_GRAYED | MF_DISABLED)))
        return FALSE;
    if (pos == UINT_MAX) i = lp->citem;
    else if (flags & MF_BYPOSITION) i = pos > (UINT)lp->citem ? lp->citem : (int)pos;
    else if (!LpItem(lp, pos, 0, &lp, &i)) return FALSE;
    if (flags & MF_SEPARATOR) flags = MF_SEPARATOR | MF_DISABLED | MF_GRAYED;
    else {
        if (psz && !FUtf8Valid(psz, strlen(psz))) return FALSE;
        if (flags & MF_POPUP) {
            hsub = LpMenu((HMENU)id);
            if (!hsub || hsub->hparent || hsub->hwndOwner) return FALSE;
            for (hp = lp; hp; hp = hp->hparent)
                if (hp == hsub || ++cDepth >= 64) return FALSE;
            if (!FMenuDepth(hsub, cDepth)) return FALSE;
        }
        item.psz.reset(SDL_strdup(psz ? psz : ""));
        if (!item.psz) return FALSE;
    }
    if (lp->citem == INT_MAX || (size_t)lp->citem + 1 > SIZE_MAX / sizeof(COMPATMENUITEM)) return FALSE;
    rg.reset(new (std::nothrow) COMPATMENUITEM[(size_t)lp->citem + 1]{});
    if (!rg) return FALSE;
    item.id = id; item.flags = flags & ~MF_BYPOSITION; item.hsub = hsub;
    // Finish all allocations before moving installed labels or submenu links.
    for (j = 0; j < lp->citem; j++) rg[j < i ? j : j + 1] = std::move(lp->rgitem[j]);
    rg[i] = std::move(item); lp->citem++;
    lp->rgitem = std::move(rg);
    if (hsub) hsub->hparent = lp;
    return TRUE;
}
BOOL win32::AppendMenuA(HMENU hmenu, UINT flags, UINT_PTR id, LPCSTR psz) {
    return win32::InsertMenuA(hmenu, UINT_MAX, flags | MF_BYPOSITION, id, psz);
}
BOOL win32::DestroyMenu(HMENU hmenu) {
    HMENU lp = LpMenu(hmenu);
    int i;
    if (!lp) return FALSE;
    lp->fAlive = FALSE;
    if (LpCompatWindow(lp->hwndOwner) && lp->hwndOwner->hmenu == lp) lp->hwndOwner->hmenu = NULL;
    lp->hwndOwner = NULL;
    for (i = 0; i < lp->citem; i++) {
        if (lp->rgitem[i].hsub) win32::DestroyMenu(lp->rgitem[i].hsub);
    }
    lp->rgitem.reset(); lp->citem = 0; lp->hparent = NULL;
    return TRUE;
}
static BOOL FRemoveMenu(HMENU hmenu, UINT pos, UINT flags, BOOL fDestroy) {
    HMENU lp;
    COMPATMENUITEM *lpitem;
    int i;
    lpitem = LpItem(hmenu, pos, flags, &lp, &i);
    if (!lpitem) return FALSE;
    if (lpitem->hsub) {
        if (fDestroy) win32::DestroyMenu(lpitem->hsub);
        else if (LpMenu(lpitem->hsub)) lpitem->hsub->hparent = NULL;
    }
    lpitem->psz.reset();
    for (; i + 1 < lp->citem; i++) lp->rgitem[i] = std::move(lp->rgitem[i + 1]);
    lp->rgitem[--lp->citem] = COMPATMENUITEM{};
    return TRUE;
}
BOOL win32::DeleteMenu(HMENU hmenu, UINT pos, UINT flags) { return FRemoveMenu(hmenu, pos, flags, TRUE); }
BOOL win32::RemoveMenu(HMENU hmenu, UINT pos, UINT flags) { return FRemoveMenu(hmenu, pos, flags, FALSE); }
DWORD win32::CheckMenuItem(HMENU hmenu, UINT id, UINT flags) {
    HMENU lp;
    COMPATMENUITEM *lpitem;
    int i;
    DWORD old;
    lpitem = LpItem(hmenu, id, flags, &lp, &i);
    if (!lpitem) return UINT_MAX;
    old = lpitem->flags & MF_CHECKED;
    lpitem->flags = (lpitem->flags & ~MF_CHECKED) | (flags & MF_CHECKED);
    return old;
}
BOOL win32::EnableMenuItem(HMENU hmenu, UINT id, UINT flags) {
    HMENU lp;
    COMPATMENUITEM *lpitem;
    int i;
    UINT old;
    lpitem = LpItem(hmenu, id, flags, &lp, &i);
    if (!lpitem) return -1;
    old = lpitem->flags & (MF_DISABLED | MF_GRAYED);
    lpitem->flags = (lpitem->flags & ~(MF_DISABLED | MF_GRAYED)) | (flags & (MF_DISABLED | MF_GRAYED));
    return (BOOL)old;
}
UINT win32::GetMenuState(HMENU hmenu, UINT id, UINT flags) {
    HMENU lp;
    COMPATMENUITEM *lpitem;
    int i;
    lpitem = LpItem(hmenu, id, flags, &lp, &i);
    if (!lpitem) return UINT_MAX;
    return lpitem->flags | (lpitem->hsub ? (UINT)(win32::GetMenuItemCount(lpitem->hsub) & 0xff) << 8 : 0);
}
int win32::GetMenuStringA(HMENU hmenu, UINT id, LPSTR psz, int cch, UINT flags) {
    HMENU lp;
    COMPATMENUITEM *lpitem;
    size_t c;
    int i;
    lpitem = LpItem(hmenu, id, flags, &lp, &i);
    if (!lpitem || !lpitem->psz) { if (psz && cch > 0) psz[0] = 0; return 0; }
    c = strlen(lpitem->psz.get());
    if (!psz || cch <= 0) return (int)(((c) < ((size_t)INT_MAX)) ? (c) : ((size_t)INT_MAX));
    c = CchUtf8Prefix(lpitem->psz.get(), (size_t)cch - 1);
    memcpy(psz, lpitem->psz.get(), c); psz[c] = 0;
    return (int)c;
}
void CompatShutdownMenus(void) {
    HMENU lp, next;
    for (lp = hmenus; lp; lp = lp->hnext) if (lp->fAlive) win32::DestroyMenu(lp);
    for (lp = hmenus; lp; lp = next) { next = lp->hnext; delete lp; }
    hmenus = NULL;
}

HMENU win32::GetMenu(HWND hwnd) {
    HWND lpwin = LpCompatWindow(hwnd);
    return lpwin && !lpwin->hwndParent ? LpMenu(lpwin->hmenu) : NULL;
}
void CompatFreeMenuBar(HWND hwnd) {
    if (hwnd->hbmpMenu) win32::DeleteObject(hwnd->hbmpMenu);
    hwnd->hbmpMenu = NULL;
    hwnd->rgrcMenu.reset();
    hwnd->cyMenu = hwnd->cxMenu = hwnd->cMenu = 0;
    hwnd->fMenuDirty = hwnd->fMenuLayoutDirty = TRUE;
}
int CyCompatMenuBar(HWND hwnd) {
    HMENU lp;
    HDC hdc;
    HGDIOBJ hfont;
    RECT *rgrc, rc;
    TEXTMETRICA tm = {};
    int cx, cy, x = 0, y = 0, dx, i;
    NATIVEOWNER<RECT, free> rgrcOwner;

    if (!hwnd || hwnd->hwndParent || !(lp = LpMenu(hwnd->hmenu)) || lp->citem == 0) return 0;
    if (!SDL_GetWindowSize(hwnd->lpWindow.get(), &cx, &cy)) return 0;
    if (!hwnd->fMenuLayoutDirty && hwnd->cxMenu == cx) return hwnd->cyMenu;
    rgrcOwner.reset((RECT *)calloc((size_t)lp->citem, sizeof(*rgrc))); rgrc = rgrcOwner.get();
    if (!rgrc) return 0;
    hdc = win32::GetDC(NULL);
    if (!hdc) return 0;
    hfont = win32::SelectObject(hdc, win32::GetStockObject(DEFAULT_GUI_FONT));
    win32::GetTextMetricsA(hdc, &tm); cy = StarsMax(20, tm.tmHeight + 8);
    for (i = 0; i < lp->citem; i++) {
        win32::SetRect(&rc, 0, 0, 0, 0);
        win32::DrawTextA(hdc, lp->rgitem[i].psz ? lp->rgitem[i].psz.get() : "", -1, &rc, DT_SINGLELINE | DT_CALCRECT);
        dx = (((cx) < (StarsMax(18, rc.right + 18))) ? (cx) : (StarsMax(18, rc.right + 18)));
        if (x && dx > cx - x) { x = 0; y += cy; }
        if (y > 32000 - cy) break;
        win32::SetRect(&rgrc[i], x, y, x + dx, y + cy); x += dx;
    }
    win32::SelectObject(hdc, hfont); win32::ReleaseDC(NULL, hdc);
    if (i != lp->citem) return 0;
    hwnd->rgrcMenu.reset(rgrcOwner.release());
    hwnd->cyMenu = y + cy; hwnd->cxMenu = cx; hwnd->cMenu = lp->citem;
    hwnd->fMenuLayoutDirty = FALSE; hwnd->fMenuDirty = TRUE;
    return hwnd->cyMenu;
}
BOOL FCompatDrawMenuBar(HWND hwnd, SDL_Surface *lpSurface) {
    HMENU lp = LpMenu(hwnd->hmenu);
    HDC hdc, hdcScreen;
    HGDIOBJ hfont, hOld;
    HBITMAP hbmp;
    HBRUSH hbr;
    RECT rc;
    int i, cy = CyCompatMenuBar(hwnd), iScale = StarsMax(1, hwnd->iCanvasScale);
    BOOL fSelected, fDisabled;
    SDL_Surface *lpBar;

    if (cy == 0) return TRUE;
    if (!lp) return FALSE;
    lpBar = hwnd->hbmpMenu ? LpCompatBitmapSurface(hwnd->hbmpMenu) : NULL;
    if (hwnd->fMenuDirty || !lpBar || lpBar->w != hwnd->cxMenu * iScale || lpBar->h != cy * iScale) {
        hdc = win32::CreateCompatibleDC(NULL);
        hdcScreen = win32::GetDC(NULL);
        hbmp = win32::CreateCompatibleBitmap(hdcScreen, hwnd->cxMenu * iScale, cy * iScale);
        if (hdcScreen) win32::ReleaseDC(NULL, hdcScreen);
        if (!hdc || !hbmp) { if (hdc) win32::DeleteDC(hdc); if (hbmp) win32::DeleteObject(hbmp); return FALSE; }
        hOld = win32::SelectObject(hdc, hbmp);
        FCompatSetDCScale(hdc, iScale);
        hfont = win32::SelectObject(hdc, win32::GetStockObject(DEFAULT_GUI_FONT));
        win32::SetBkMode(hdc, TRANSPARENT);
        win32::SetRect(&rc, 0, 0, hwnd->cxMenu, cy);
        hbr = win32::CreateSolidBrush(win32::GetSysColor(COLOR_MENU));
        if (hbr) { win32::FillRect(hdc, &rc, hbr); win32::DeleteObject(hbr); }
        for (i = 0; i < StarsMin(hwnd->cMenu, lp->citem); i++) {
            rc = hwnd->rgrcMenu[i];
            fSelected = hwnd->iMenuHot == i;
            fDisabled = (lp->rgitem[i].flags & (MF_DISABLED | MF_GRAYED | MF_SEPARATOR)) != 0;
            if (fSelected) {
                hbr = win32::CreateSolidBrush(win32::GetSysColor(COLOR_HIGHLIGHT));
                if (hbr) { win32::FillRect(hdc, &rc, hbr); win32::DeleteObject(hbr); }
            }
            win32::SetTextColor(hdc, win32::GetSysColor(fDisabled ? COLOR_GRAYTEXT : fSelected ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT));
            win32::DrawTextA(hdc, lp->rgitem[i].psz ? lp->rgitem[i].psz.get() : "", -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        }
        win32::SelectObject(hdc, hfont); win32::SelectObject(hdc, hOld); win32::DeleteDC(hdc);
        if (hwnd->hbmpMenu) win32::DeleteObject(hwnd->hbmpMenu);
        hwnd->hbmpMenu = hbmp; hwnd->fMenuDirty = FALSE;
    }
    lpBar = LpCompatBitmapSurface(hwnd->hbmpMenu);
    return lpBar && SDL_BlitSurface(lpBar, NULL, lpSurface, NULL);
}
BOOL win32::DrawMenuBar(HWND hwnd) {
    HWND lpwin = LpCompatWindow(hwnd);
    RECT rc;
    int cyOld;
    if (!lpwin || lpwin->hwndParent || lpwin->fDestroying) return FALSE;
    cyOld = lpwin->cyMenu;
    lpwin->fMenuLayoutDirty = lpwin->fMenuDirty = TRUE;
    CyCompatMenuBar(lpwin);
    if (!lpwin->hmenu) lpwin->cyMenu = 0;
    win32::InvalidateRect(hwnd, NULL, FALSE);
    if (lpwin->fCreated && cyOld != lpwin->cyMenu && win32::GetClientRect(hwnd, &rc))
        win32::SendMessageA(hwnd, WM_SIZE, win32::IsIconic(hwnd) ? SIZE_MINIMIZED : win32::IsZoomed(hwnd) ? SIZE_MAXIMIZED : SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
    return LpCompatWindow(hwnd) && FCompatPresent(hwnd);
}
BOOL win32::GetMenuItemRect(HWND hwnd, HMENU hmenu, UINT i, LPRECT lprc) {
    HMENU lp = LpMenu(hmenu);
    HWND lpwin = LpCompatWindow(lp ? lp->hwndOwner : NULL);
    POINT pt = {0, 0};
    (void)hwnd; // Attached bar geometry belongs to the menu's window.
    if (!lpwin || !lprc || CyCompatMenuBar(lpwin) == 0 || i >= (UINT)lpwin->cMenu ||
        !win32::ClientToScreen(lpwin, &pt)) return FALSE;
    *lprc = lpwin->rgrcMenu[i];
    return win32::OffsetRect(lprc, pt.x, pt.y - lpwin->cyMenu);
}
BOOL win32::SetMenu(HWND hwnd, HMENU hmenu) {
    HWND lpwin = LpCompatWindow(hwnd);
    HMENU lp = LpMenu(hmenu), old;
    if (!lpwin || lpwin->fDestroying || lpwin->hwndParent ||
        (hmenu && (!lp || lp->hparent || (lp->hwndOwner && lp->hwndOwner != hwnd)))) return FALSE;
    old = LpMenu(lpwin->hmenu);
    if (old) old->hwndOwner = NULL;
    lpwin->hmenu = lp;
    if (lp) lp->hwndOwner = hwnd;
    lpwin->iMenuHot = -1;
    lpwin->fMenuLayoutDirty = lpwin->fMenuDirty = TRUE;
    if (lpwin->fCreated) return win32::DrawMenuBar(hwnd);
    win32::InvalidateRect(hwnd, NULL, FALSE);
    return TRUE;
}
void CompatDestroyWindowMenu(HWND hwnd) {
    HMENU hmenu = win32::GetMenu(hwnd);
    hwnd->hmenu = NULL;
    CompatFreeMenuBar(hwnd);
    if (hmenu) win32::DestroyMenu(hmenu);
}

typedef struct COMPATTRACK COMPATTRACK;
typedef struct COMPATPOPUP {
    COMPATTRACK *lptrack;
    HMENU hmenu;
    HWND hwnd;
    NATIVEARRAYOWNER<RECT, std::free> rgrc;
    int cItem, iSel, cx, cy;
} COMPATPOPUP;
struct COMPATTRACK {
    HWND hwndOwner;
    HMENU hmenuRoot, hmenuBar;
    int iBar;
    UINT flags, id;
    BOOL fDone, fFailed;
    int cLevel;
    COMPATPOPUP rgpopup[64];
    COMPATTRACK *lpPrevious;
};
static COMPATTRACK *lptrack;

static void PopupPaint(COMPATPOPUP *lppop) {
    PAINTSTRUCT ps;
    HDC hdc;
    HBRUSH hbr;
    HPEN hpen, hpenOld;
    HMENU lp;
    RECT rc, text;
    char *psz, *pszTab;
    int i, y;
    BOOL fDisabled, fSelected;
    HGDIOBJ hfont;
    hdc = win32::BeginPaint(lppop->hwnd, &ps);
    if (!hdc) return;
    hfont = win32::SelectObject(hdc, win32::GetStockObject(DEFAULT_GUI_FONT));
    win32::SetBkMode(hdc, TRANSPARENT);
    win32::GetClientRect(lppop->hwnd, &rc);
    hbr = win32::CreateSolidBrush(win32::GetSysColor(COLOR_MENU));
    if (hbr) { win32::FillRect(hdc, &rc, hbr); win32::DeleteObject(hbr); }
    win32::SelectObject(hdc, win32::GetStockObject(NULL_BRUSH));
    CompatDrawClassicEdge(hdc, &rc, FALSE, TRUE);
    lp = LpMenu(lppop->hmenu);
    for (i = 0; lp && i < StarsMin(lppop->cItem, lp->citem); i++) {
        rc = lppop->rgrc[i];
        y = (rc.top + rc.bottom) / 2;
        if (lp->rgitem[i].flags & MF_SEPARATOR) {
            text = RECT{rc.left + 6, y, rc.right - 6, y + 1};
            hbr = win32::CreateSolidBrush(win32::GetSysColor(COLOR_BTNSHADOW)); win32::FillRect(hdc, &text, hbr); win32::DeleteObject(hbr);
            win32::OffsetRect(&text, 0, 1);
            hbr = win32::CreateSolidBrush(win32::GetSysColor(COLOR_BTNHIGHLIGHT)); win32::FillRect(hdc, &text, hbr); win32::DeleteObject(hbr); continue;
        }
        fDisabled = (lp->rgitem[i].flags & (MF_DISABLED | MF_GRAYED)) != 0;
        fSelected = i == lppop->iSel;
        if (fSelected) {
            hbr = win32::CreateSolidBrush(win32::GetSysColor(COLOR_HIGHLIGHT));
            if (hbr) { win32::FillRect(hdc, &rc, hbr); win32::DeleteObject(hbr); }
        }
        win32::SetTextColor(hdc, win32::GetSysColor(fDisabled ? COLOR_GRAYTEXT : fSelected ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT));
        hpen = win32::CreatePen(PS_SOLID, 1, win32::GetSysColor(fDisabled ? COLOR_GRAYTEXT : fSelected ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT));
        hpenOld = (HPEN)(win32::SelectObject(hdc, hpen));
        if (lp->rgitem[i].flags & MF_CHECKED) {
            win32::MoveToEx(hdc, rc.left + 5, y, NULL); win32::LineTo(hdc, rc.left + 8, y + 3); win32::LineTo(hdc, rc.left + 14, y - 4);
        }
        if (lp->rgitem[i].hsub) {
            win32::MoveToEx(hdc, rc.right - 12, y - 4, NULL); win32::LineTo(hdc, rc.right - 8, y); win32::LineTo(hdc, rc.right - 12, y + 4);
        }
        win32::SelectObject(hdc, hpenOld); win32::DeleteObject(hpen);
        NATIVEOWNER<char, SDL_free> pszOwner(SDL_strdup(lp->rgitem[i].psz ? lp->rgitem[i].psz.get() : ""));
        psz = pszOwner.get();
        if (!psz) continue;
        pszTab = strchr(psz, '\t');
        if (pszTab) *pszTab++ = 0;
        text = rc; text.left += 24; text.right -= 24;
        win32::DrawTextA(hdc, psz, -1, &text, DT_SINGLELINE | DT_VCENTER);
        if (pszTab) win32::DrawTextA(hdc, pszTab, -1, &text, DT_SINGLELINE | DT_VCENTER | DT_RIGHT | DT_NOPREFIX);
    }
    win32::SelectObject(hdc, hfont);
    win32::EndPaint(lppop->hwnd, &ps);
}
static LRESULT CALLBACK PopupProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    COMPATPOPUP *lppop = (COMPATPOPUP *)win32::GetWindowLongPtrA(hwnd, 0);
    if (msg == WM_NCCREATE) {
        lppop = (COMPATPOPUP *)(((CREATESTRUCTA *)l)->lpCreateParams);
        win32::SetWindowLongPtrA(hwnd, 0, (LONG_PTR)lppop);
    }
    if (lppop && msg == WM_PAINT) { PopupPaint(lppop); return 0; }
    if (lppop && (msg == WM_CLOSE || msg == WM_CANCELMODE)) { lppop->lptrack->fDone = TRUE; return 0; }
    return win32::DefWindowProcA(hwnd, msg, w, l);
}
static void ClosePopups(COMPATTRACK *lpt, int cKeep) {
    COMPATPOPUP *lp;
    while (lpt->cLevel > cKeep) {
        lp = &lpt->rgpopup[--lpt->cLevel];
        if (win32::IsWindow(lp->hwnd)) win32::DestroyWindow(lp->hwnd);
        lp->rgrc.reset(); lp->hwnd = NULL;
        if (!(lpt->flags & TPM_NONOTIFY) && win32::IsWindow(lpt->hwndOwner))
            win32::SendMessageA(lpt->hwndOwner, WM_UNINITMENUPOPUP, (WPARAM)lp->hmenu, 0);
    }
}
static BOOL OpenPopup(COMPATTRACK *lpt, HMENU hmenu, int x, int y, int iParent) {
    HMENU lp;
    COMPATPOPUP *lppop;
    HDC hdc;
    HGDIOBJ hfont;
    SIZE size;
    TEXTMETRICA tm = {};
    SDL_Rect bounds;
    SDL_Point point = {x, y};
    int i, cyRow, cx = 80, cy = 2, dy;
    RECT rcParent;
    int64_t xWide = x, yWide = y;
    WNDCLASSA wc = {};
    if (lpt->cLevel >= 64) return FALSE;
    if (!(lpt->flags & TPM_NONOTIFY))
        win32::SendMessageA(lpt->hwndOwner, WM_INITMENUPOPUP, (WPARAM)hmenu, MAKELPARAM(iParent, FALSE));
    lp = LpMenu(hmenu);
    if (!lp || !win32::IsWindow(lpt->hwndOwner) || lpt->fDone || lp->citem == 0) return FALSE;
    lppop = &lpt->rgpopup[lpt->cLevel];
    *lppop = COMPATPOPUP{};
    lppop->lptrack = lpt; lppop->hmenu = lp; lppop->iSel = -1; lppop->cItem = lp->citem;
    lppop->rgrc.reset((RECT *)calloc((size_t)lp->citem, sizeof(RECT)));
    if (!lppop->rgrc) return FALSE;
    hdc = win32::GetDC(NULL);
    if (!hdc) { lppop->rgrc.reset(); return FALSE; }
    hfont = win32::SelectObject(hdc, win32::GetStockObject(DEFAULT_GUI_FONT));
    win32::GetTextMetricsA(hdc, &tm); cyRow = StarsMax(20, tm.tmHeight + 8);
    for (i = 0; i < lp->citem; i++) {
        size.cx = 0;
        if (lp->rgitem[i].psz && !win32::GetTextExtentPoint32A(hdc, lp->rgitem[i].psz.get(), (int)strlen(lp->rgitem[i].psz.get()), &size)) break;
        if (size.cx > 8100 || cy > 32000 - cyRow) break;
        cx = StarsMax(cx, size.cx + 72);
        dy = lp->rgitem[i].flags & MF_SEPARATOR ? 8 : cyRow;
        win32::SetRect(&lppop->rgrc[i], 2, cy, 0, cy + dy); cy += dy;
    }
    win32::SelectObject(hdc, hfont); win32::ReleaseDC(NULL, hdc);
    if (i != lp->citem) { lppop->rgrc.reset(); return FALSE; }
    cy += 2; lppop->cx = cx; lppop->cy = cy;
    for (i = 0; i < lp->citem; i++) lppop->rgrc[i].right = cx - 2;
    if (lpt->cLevel == 0) {
        if (lpt->flags & TPM_RIGHTALIGN) xWide -= cx;
        else if (lpt->flags & TPM_CENTERALIGN) xWide -= cx / 2;
        if (lpt->flags & TPM_BOTTOMALIGN) yWide -= cy;
        else if (lpt->flags & TPM_VCENTERALIGN) yWide -= cy / 2;
    }
    if (CompatDisplayIO().pfnGetDisplayUsableBounds(SDL_GetDisplayForPoint(&point), &bounds)) {
        if (lpt->cLevel > 0 && xWide + cx > (int64_t)bounds.x + bounds.w &&
            win32::GetWindowRect(lpt->rgpopup[lpt->cLevel - 1].hwnd, &rcParent)) xWide = (int64_t)rcParent.left - cx + 2;
        xWide = ((((int64_t)bounds.x) > ((((xWide) < ((int64_t)bounds.x + StarsMax(0, bounds.w - cx))) ? (xWide) : ((int64_t)bounds.x + StarsMax(0, bounds.w - cx))))) ? ((int64_t)bounds.x) : ((((xWide) < ((int64_t)bounds.x + StarsMax(0, bounds.w - cx))) ? (xWide) : ((int64_t)bounds.x + StarsMax(0, bounds.w - cx)))));
        yWide = ((((int64_t)bounds.y) > ((((yWide) < ((int64_t)bounds.y + StarsMax(0, bounds.h - cy))) ? (yWide) : ((int64_t)bounds.y + StarsMax(0, bounds.h - cy))))) ? ((int64_t)bounds.y) : ((((yWide) < ((int64_t)bounds.y + StarsMax(0, bounds.h - cy))) ? (yWide) : ((int64_t)bounds.y + StarsMax(0, bounds.h - cy)))));
    }
    if (xWide < INT_MIN || xWide > INT_MAX || yWide < INT_MIN || yWide > INT_MAX) {
        lppop->rgrc.reset(); return FALSE;
    }
    x = (int)xWide; y = (int)yWide;
    wc.lpfnWndProc = PopupProc; wc.cbWndExtra = sizeof(LONG_PTR); wc.lpszClassName = "StarsCompatPopup";
    win32::RegisterClassA(&wc);
    lppop->hwnd = win32::CreateWindowExA(WS_EX_TOPMOST | WS_EX_NOACTIVATE, wc.lpszClassName, "", WS_POPUP, x, y, cx, cy,
                                  lpt->hwndOwner, NULL, NULL, lppop);
    if (!lppop->hwnd) { lppop->rgrc.reset(); return FALSE; }
    lpt->cLevel++;
    SDL_SetWindowFocusable(lppop->hwnd->lpWindow.get(), false);
    win32::ShowWindow(lppop->hwnd, SW_SHOW);
    win32::InvalidateRect(lppop->hwnd, NULL, FALSE); win32::UpdateWindow(lppop->hwnd);
    return TRUE;
}
static BOOL FSelectable(HMENU lp, int i) {
    return lp && i >= 0 && i < lp->citem && !(lp->rgitem[i].flags & (MF_SEPARATOR | MF_DISABLED | MF_GRAYED));
}
static void SelectPopup(COMPATTRACK *lpt, int iLevel, int i, BOOL fMouse) {
    COMPATPOPUP *lppop = &lpt->rgpopup[iLevel];
    HMENU lp = LpMenu(lppop->hmenu);
    UINT flags, id;
    if (!lp || i < 0 || i >= lp->citem || (lp->rgitem[i].flags & MF_SEPARATOR)) i = -1;
    if (i == lppop->iSel) return;
    ClosePopups(lpt, iLevel + 1);
    lppop->iSel = i;
    win32::InvalidateRect(lppop->hwnd, NULL, FALSE); win32::UpdateWindow(lppop->hwnd);
    if (i >= 0 && !(lpt->flags & TPM_NONOTIFY) && (lp = LpMenu(lppop->hmenu)) && i < lp->citem) {
        flags = lp->rgitem[i].flags | MF_HILITE | (fMouse ? MF_MOUSESELECT : 0);
        id = lp->rgitem[i].hsub ? (UINT)i : (UINT)lp->rgitem[i].id;
        win32::SendMessageA(lpt->hwndOwner, WM_MENUSELECT, MAKEWPARAM(id, flags), (LPARAM)lp);
    }
}
static void ActivatePopup(COMPATTRACK *lpt, int iLevel, BOOL fKeyboard) {
    COMPATPOPUP *lppop = &lpt->rgpopup[iLevel];
    HMENU lp = LpMenu(lppop->hmenu), hsub;
    int i = lppop->iSel;
    POINT pt;
    if (!FSelectable(lp, i)) return;
    hsub = lp->rgitem[i].hsub;
    if (!hsub) { lpt->id = (UINT)lp->rgitem[i].id; lpt->fDone = TRUE; return; }
    if (lpt->cLevel > iLevel + 1 && lpt->rgpopup[iLevel + 1].hmenu == hsub) return;
    ClosePopups(lpt, iLevel + 1);
    pt.x = lppop->cx - 2; pt.y = lppop->rgrc[i].top;
    if (!win32::ClientToScreen(lppop->hwnd, &pt) || !OpenPopup(lpt, hsub, pt.x, pt.y, i)) lpt->fFailed = lpt->fDone = TRUE;
    else if (fKeyboard && !lpt->fDone) {
        lp = LpMenu(hsub);
        for (i = 0; lp && i < lp->citem; i++)
            if (!(lp->rgitem[i].flags & MF_SEPARATOR)) { SelectPopup(lpt, iLevel + 1, i, FALSE); break; }
    }
}
static BOOL FMenuMnemonic(const COMPATMENUITEM *lpitem, WPARAM vk) {
    const char *psz = lpitem->psz.get();
    if (!psz || vk > 255) return FALSE;
    for (; *psz; psz++) if (*psz == '&' && psz[1]) {
        if (psz[1] == '&') { psz++; continue; }
        if (SDL_toupper((unsigned char)psz[1]) == SDL_toupper((int)vk)) return TRUE;
    }
    return FALSE;
}
static int IMenuBarPoint(HWND hwnd, POINT pt) {
    HWND lpwin = LpCompatWindow(hwnd);
    RECT rc;
    int i;
    if (!lpwin) return -1;
    CyCompatMenuBar(lpwin);
    for (i = 0; i < lpwin->cMenu; i++)
        if (win32::GetMenuItemRect(hwnd, lpwin->hmenu, (UINT)i, &rc) && win32::PtInRect(&rc, pt)) return i;
    return -1;
}
static BOOL OpenMenuBar(COMPATTRACK *lpt, int i, BOOL fKeyboard) {
    HWND lpwin = LpCompatWindow(lpt->hwndOwner);
    HMENU lp = LpMenu(lpt->hmenuBar), hsub;
    RECT rc;
    int j;
    if (!lpwin || win32::GetMenu(lpwin) != lp || !FSelectable(lp, i)) return FALSE;
    hsub = lp->rgitem[i].hsub;
    if (!hsub) { lpt->id = (UINT)lp->rgitem[i].id; lpt->fDone = TRUE; return TRUE; }
    if (!win32::GetMenuItemRect(lpwin, lp, (UINT)i, &rc)) return FALSE;
    ClosePopups(lpt, 0);
    if (!win32::IsWindow(lpt->hwndOwner) || win32::GetMenu(lpt->hwndOwner) != lp) return FALSE;
    lpt->iBar = i; lpwin->iMenuHot = i; lpwin->fMenuDirty = TRUE; FCompatPresent(lpwin);
    win32::SendMessageA(lpwin, WM_MENUSELECT, MAKEWPARAM(i, MF_POPUP), (LPARAM)lp);
    if (!OpenPopup(lpt, hsub, rc.left, rc.bottom, i)) return FALSE;
    if (fKeyboard) {
        lp = LpMenu(hsub);
        for (j = 0; lp && j < lp->citem; j++) if (!(lp->rgitem[j].flags & MF_SEPARATOR)) {
            SelectPopup(lpt, 0, j, FALSE); break;
        }
    }
    return TRUE;
}
static void StepMenuBar(COMPATTRACK *lpt, int dir) {
    HMENU lp = LpMenu(lpt->hmenuBar);
    int j, i = lpt->iBar;
    if (!lp || lp->citem == 0) return;
    for (j = 0; j < lp->citem; j++) {
        i = (i + dir + lp->citem) % lp->citem;
        if (FSelectable(lp, i)) {
            if (!OpenMenuBar(lpt, i, TRUE)) lpt->fFailed = lpt->fDone = TRUE;
            return;
        }
    }
}
static BOOL PopupInput(COMPATTRACK *lpt, const win32::MSG *lpmsg) {
    COMPATPOPUP *lppop;
    HMENU lp;
    POINT pt;
    int iLevel, i, j, dir, cMatch;
    BOOL fMouse = lpmsg->message >= WM_MOUSEMOVE && lpmsg->message <= WM_MOUSEWHEEL;
    if (lpmsg->message == WM_CANCELMODE || (lpmsg->message == WM_ACTIVATE && LOWORD(lpmsg->wParam) == WA_INACTIVE && lpmsg->hwnd == lpt->hwndOwner)) {
        lpt->fDone = TRUE; return TRUE;
    }
    if (!lpt->cLevel) return FALSE;
    if (lpt->hmenuBar && lpmsg->message == WM_SYSCHAR && LpCompatWindow(lpt->hwndOwner) &&
        lpt->hwndOwner->vkMenuSuppress && SDL_toupper((int)lpmsg->wParam) == SDL_toupper((int)lpt->hwndOwner->vkMenuSuppress)) {
        lpt->hwndOwner->vkMenuSuppress = 0;
        return TRUE;
    }
    if (fMouse) {
        if (lpmsg->message == WM_MOUSEWHEEL) return TRUE;
        pt.x = (SHORT)LOWORD(lpmsg->lParam); pt.y = (SHORT)HIWORD(lpmsg->lParam);
        if (!win32::ClientToScreen(lpmsg->hwnd, &pt)) return TRUE;
        if (lpt->hmenuBar && (i = IMenuBarPoint(lpt->hwndOwner, pt)) >= 0) {
            if (i != lpt->iBar && FSelectable(LpMenu(lpt->hmenuBar), i)) {
                if (!OpenMenuBar(lpt, i, FALSE)) lpt->fFailed = lpt->fDone = TRUE;
            } else if (lpmsg->message == WM_LBUTTONDOWN) lpt->fDone = TRUE;
            return TRUE;
        }
        for (iLevel = lpt->cLevel - 1; iLevel >= 0; iLevel--) {
            POINT local = pt;
            lppop = &lpt->rgpopup[iLevel];
            if (!win32::ScreenToClient(lppop->hwnd, &local)) continue;
            for (i = 0; i < lppop->cItem; i++) if (win32::PtInRect(&lppop->rgrc[i], local)) break;
            if (i == lppop->cItem) continue;
            SelectPopup(lpt, iLevel, i, TRUE);
            if (lpt->fDone || !win32::IsWindow(lpt->hwndOwner)) return TRUE;
            if (lpmsg->message == WM_MOUSEMOVE) {
                lp = LpMenu(lppop->hmenu);
                if (FSelectable(lp, lppop->iSel) && lp->rgitem[lppop->iSel].hsub) ActivatePopup(lpt, iLevel, FALSE);
            } else if (lpmsg->message == WM_LBUTTONUP ||
                       (lpmsg->message == WM_RBUTTONUP && (lpt->flags & TPM_RIGHTBUTTON))) ActivatePopup(lpt, iLevel, FALSE);
            return TRUE;
        }
        if (lpmsg->message == WM_LBUTTONDOWN || lpmsg->message == WM_RBUTTONDOWN) lpt->fDone = TRUE;
        return TRUE;
    }
    if (lpmsg->message != WM_KEYDOWN && lpmsg->message != WM_SYSKEYDOWN && lpmsg->message != WM_CHAR && lpmsg->message != WM_SYSCHAR) return FALSE;
    iLevel = lpt->cLevel - 1; lppop = &lpt->rgpopup[iLevel]; lp = LpMenu(lppop->hmenu);
    if (!lp) { lpt->fDone = TRUE; return TRUE; }
    if ((lpmsg->message == WM_CHAR || lpmsg->message == WM_SYSCHAR) && lpmsg->wParam >= 32) {
        cMatch = 0;
        for (i = 0; i < lp->citem; i++)
            if (FSelectable(lp, i) && FMenuMnemonic(&lp->rgitem[i], lpmsg->wParam)) cMatch++;
        for (j = 1; cMatch != 0 && j <= lp->citem; j++) {
            i = (lppop->iSel + j + lp->citem) % lp->citem;
            if (FSelectable(lp, i) && FMenuMnemonic(&lp->rgitem[i], lpmsg->wParam)) {
                SelectPopup(lpt, iLevel, i, FALSE);
                if (cMatch == 1) ActivatePopup(lpt, iLevel, TRUE);
                break;
            }
        }
        return TRUE;
    }
    switch (lpmsg->wParam) {
    case VK_ESCAPE: if (iLevel != 0) ClosePopups(lpt, iLevel); else lpt->fDone = TRUE; break;
    case VK_LEFT: if (iLevel != 0) ClosePopups(lpt, iLevel); else if (lpt->hmenuBar) StepMenuBar(lpt, -1); break;
    case VK_RIGHT:
        if (FSelectable(lp, lppop->iSel) && lp->rgitem[lppop->iSel].hsub) ActivatePopup(lpt, iLevel, TRUE);
        else if (lpt->hmenuBar) StepMenuBar(lpt, 1);
        break;
    case VK_RETURN: ActivatePopup(lpt, iLevel, TRUE); break;
    case VK_UP: case VK_DOWN: case VK_HOME: case VK_END:
        dir = lpmsg->wParam == VK_UP || lpmsg->wParam == VK_END ? -1 : 1;
        i = lpmsg->wParam == VK_HOME || lpmsg->wParam == VK_END || lppop->iSel < 0 ? (dir > 0 ? -1 : 0) : lppop->iSel;
        for (j = 0; j < lp->citem; j++) { i = (i + dir + lp->citem) % lp->citem; if (!(lp->rgitem[i].flags & MF_SEPARATOR)) break; }
        if (j != lp->citem) SelectPopup(lpt, iLevel, i, FALSE);
        break;
    default:
        break;
    }
    return TRUE;

}
BOOL win32::EndMenu(void) {
    if (!lptrack) return FALSE;
    lptrack->fDone = TRUE; return TRUE;
}
static BOOL FMenuTracked(HMENU hmenu) {
    COMPATTRACK *lpt;
    int i;
    for (lpt = lptrack; lpt; lpt = lpt->lpPrevious) {
        if (lpt->hmenuRoot == hmenu) return TRUE;
        for (i = 0; i < lpt->cLevel; i++) if (lpt->rgpopup[i].hmenu == hmenu) return TRUE;
    }
    return FALSE;
}
static BOOL TrackMenu(HMENU hmenu, UINT flags, int x, int y, int reserved, HWND hwnd, const RECT *lprc, HMENU hbar, int iBar, BOOL fKeyboard) {
    COMPATTRACK track = {};
    HWND hwndCapture;
    win32::MSG msg;
    int result;
    (void)lprc;
    if (!SDL_IsMainThread() || !win32::IsMenu(hmenu) || FMenuTracked(hmenu) || !win32::IsWindow(hwnd) || reserved != 0 ||
        (lptrack && !(flags & TPM_RECURSE)) ||
        (flags & ~(TPM_RECURSE | TPM_RIGHTBUTTON | TPM_CENTERALIGN | TPM_RIGHTALIGN | TPM_VCENTERALIGN |
                   TPM_BOTTOMALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_NOANIMATION))) return FALSE;
    track.hwndOwner = hwnd; track.hmenuRoot = hbar ? hbar : hmenu; track.hmenuBar = hbar; track.iBar = iBar; track.flags = flags; track.lpPrevious = lptrack; lptrack = &track;
    hwndCapture = win32::GetCapture();
    COMPATCLEANUPSCOPE restoreTrack([&] {
        lptrack = track.lpPrevious;
        if (hbar && LpCompatWindow(hwnd)) { hwnd->iMenuHot = -1; hwnd->fMenuDirty = TRUE; FCompatPresent(hwnd); }
    }, hwnd);
    COMPATCLEANUPSCOPE exitNotifications([&] {
        if (!(flags & TPM_NONOTIFY) && win32::IsWindow(hwnd)) {
            win32::SendMessageA(hwnd, WM_MENUSELECT, MAKEWPARAM(0, 0xffff), 0);
            if (win32::IsWindow(hwnd)) win32::SendMessageA(hwnd, WM_EXITMENULOOP, !hbar, 0);
        }
    }, hwnd);
    COMPATCLEANUPSCOPE restoreCapture([&] {
        if (win32::GetCapture() == hwnd) {
            if (win32::IsWindow(hwndCapture)) win32::SetCapture(hwndCapture); else win32::ReleaseCapture();
        }
    }, hwnd);
    COMPATCLEANUPSCOPE closePopups([&] { ClosePopups(&track, 0); }, hwnd);
    if (!(flags & TPM_NONOTIFY)) {
        win32::SendMessageA(hwnd, WM_ENTERMENULOOP, !hbar, 0);
        if (win32::IsWindow(hwnd) && !track.fDone) win32::SendMessageA(hwnd, WM_INITMENU, (WPARAM)(hbar ? hbar : hmenu), 0);
    }
    if (!track.fDone && win32::IsWindow(hwnd) && win32::IsMenu(hmenu)) {
        if (!(hbar ? OpenMenuBar(&track, iBar, fKeyboard) : OpenPopup(&track, hmenu, x, y, 0))) track.fFailed = TRUE;
        else {
            win32::SetCapture(hwnd);
            while (!track.fDone && win32::IsWindow(hwnd) && win32::IsMenu(track.hmenuRoot) && !track.fFailed && win32::GetCapture() == hwnd) {
                result = win32::GetMessageA(&msg, NULL, 0, 0);
                if (result <= 0) { if (result == 0) win32::PostQuitMessage((int)msg.wParam); track.fFailed = TRUE; break; }
                if (!PopupInput(&track, &msg)) { win32::TranslateMessage(&msg); win32::DispatchMessageA(&msg); }
            }
        }
    }
    closePopups.Close();
    restoreCapture.Close();
    exitNotifications.Close();
    restoreTrack.Close();
    // Popup teardown can fail after selection; that operation must not issue a command.
    if (FStarsErrorPending()) return FALSE;
    if (!track.fFailed && track.id != 0 && !(flags & (TPM_NONOTIFY | TPM_RETURNCMD)) && win32::IsWindow(hwnd))
        win32::PostMessageA(hwnd, WM_COMMAND, MAKEWPARAM(track.id, 0), 0);
    return flags & TPM_RETURNCMD ? (BOOL)track.id : !track.fFailed;
}

BOOL win32::TrackPopupMenu(HMENU hmenu, UINT flags, int x, int y, int reserved, HWND hwnd, const RECT *lprc) {
    return TrackMenu(hmenu, flags, x, y, reserved, hwnd, lprc, NULL, -1, FALSE);
}
BOOL FCompatMenuMessage(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND lpwin = LpCompatWindow(hwnd);
    HMENU lp;
    POINT pt;
    int i = -1, j, dir;
    BOOL fOpen = FALSE;
    if (!lpwin) return FALSE;
    if (lpwin->hwndParent) {
        HWND hwndRoot = HwndCompatRoot(hwnd);
        if (msg == WM_SYSCHAR || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP ||
            (msg == WM_KEYDOWN && (w == VK_F10 || hwndRoot->iMenuHot >= 0)))
            return FCompatMenuMessage(hwndRoot, msg, w, l);
        return FALSE;
    }
    if (!(lp = LpMenu(lpwin->hmenu)) || !win32::IsWindowEnabled(hwnd) || lp->citem == 0) return FALSE;
    if (msg == WM_SYSCHAR && lpwin->vkMenuSuppress && SDL_toupper((int)w) == SDL_toupper((int)lpwin->vkMenuSuppress)) {
        lpwin->vkMenuSuppress = 0; return TRUE;
    }
    if (msg == WM_NCLBUTTONDOWN && w == HTMENU) {
        pt.x = (SHORT)LOWORD(l); pt.y = (SHORT)HIWORD(l);
        i = IMenuBarPoint(hwnd, pt); fOpen = TRUE;
    } else if (msg == WM_SYSKEYDOWN && w != VK_MENU && w != VK_F10 && ((UINT_PTR)l & (UINT32_C(1) << 29)) &&
               !(win32::GetKeyState(VK_CONTROL) & 0x8000)) {
        // SDL backends may omit text-input events for Alt-letter combinations.
        for (j = 0; j < lp->citem; j++) if (FSelectable(lp, j) && FMenuMnemonic(&lp->rgitem[j], w)) { i = j; break; }
        if (i < 0) return FALSE;
        fOpen = TRUE; lpwin->fMenuAltUsed = TRUE; lpwin->vkMenuSuppress = w;
    } else if (msg == WM_SYSCHAR || (msg == WM_SYSCOMMAND && (w & 0xfff0) == SC_KEYMENU && l)) {
        for (j = 0; j < lp->citem; j++) if (FSelectable(lp, j) && FMenuMnemonic(&lp->rgitem[j], msg == WM_SYSCHAR ? w : (WPARAM)l)) { i = j; break; }
        fOpen = TRUE; lpwin->fMenuAltUsed = TRUE;
    } else if (msg == WM_SYSKEYDOWN && w == VK_MENU) {
        lpwin->fMenuAltUsed = FALSE; lpwin->vkMenuSuppress = 0; return TRUE;
    } else if (((msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) && w == VK_F10) || (msg == WM_SYSKEYUP && w == VK_MENU) ||
               (msg == WM_SYSCOMMAND && (w & 0xfff0) == SC_KEYMENU)) {
        if (msg == WM_SYSKEYUP) lpwin->vkMenuSuppress = 0;
        if (msg == WM_SYSKEYUP && lpwin->fMenuAltUsed) return TRUE;
        if (lpwin->iMenuHot >= 0) { lpwin->iMenuHot = -1; lpwin->fMenuDirty = TRUE; FCompatPresent(hwnd); return TRUE; }
        for (j = 0; j < lp->citem; j++) if (FSelectable(lp, j)) { i = j; break; }
    } else if (msg == WM_KEYDOWN && lpwin->iMenuHot >= 0) {
        i = lpwin->iMenuHot;
        if (w == VK_ESCAPE) i = -1;
        else if (w == VK_DOWN || w == VK_RETURN) fOpen = TRUE;
        else if (w == VK_LEFT || w == VK_RIGHT) {
            dir = w == VK_RIGHT ? 1 : -1;
            for (j = 0; j < lp->citem; j++) { i = (i + dir + lp->citem) % lp->citem; if (FSelectable(lp, i)) break; }
        } else return FALSE;
    } else return FALSE;
    if (i >= 0 && FSelectable(lp, i)) {
        if (fOpen) TrackMenu(lp->rgitem[i].hsub ? lp->rgitem[i].hsub : lp, 0, 0, 0, 0, hwnd, NULL, lp, i, msg != WM_NCLBUTTONDOWN);
        else { lpwin->iMenuHot = i; lpwin->fMenuDirty = TRUE; FCompatPresent(hwnd); }
    } else if (i < 0 && !fOpen) { lpwin->iMenuHot = -1; lpwin->fMenuDirty = TRUE; FCompatPresent(hwnd); }
    return TRUE;
}

BOOL FCompatMenuFocus(HWND hwnd, BOOL fGain) {
    COMPATTRACK *lpt;
    SDL_Window *lpFocus = SDL_GetKeyboardFocus();
    int i;
    for (lpt = lptrack; lpt; lpt = lpt->lpPrevious) {
        for (i = 0; i < lpt->cLevel; i++) {
            HWND hp = LpCompatWindow(lpt->rgpopup[i].hwnd);
            if (!hp) continue;
            // Some SDL backends briefly focus/recreate the popup during rendering.
            // Menu windows do not become the application's logical active window.
            if (hwnd == hp) {
                if (!fGain && !lpFocus) { lpt->fDone = TRUE; CompatFocusWindow(NULL); }
                return TRUE;
            }
            if (!fGain && hwnd == HwndCompatRoot(lpt->hwndOwner) && hp->lpWindow.get() == lpFocus) return TRUE;
        }
    }
    return FALSE;
}

/* Return a copied ancestor path so owner callbacks may replace menu storage. */
static int CAccelPath(HMENU hmenu, UINT cmd, HMENU *rgh, int *rgi, int depth, BOOL *lpfEnabled) {
    HMENU lp = LpMenu(hmenu);
    int i, c;
    BOOL fEnabled;
    if (!lp || depth >= 64) return 0;
    for (i = 0; i < lp->citem; i++) {
        fEnabled = !(lp->rgitem[i].flags & (MF_DISABLED | MF_GRAYED | MF_SEPARATOR));
        rgh[depth] = lp; rgi[depth] = i;
        if (!lp->rgitem[i].hsub && lp->rgitem[i].id == cmd) {
            *lpfEnabled = fEnabled;
            return depth + 1;
        }
        if (lp->rgitem[i].hsub) {
            c = CAccelPath(lp->rgitem[i].hsub, cmd, rgh, rgi, depth + 1, lpfEnabled);
            if (c != 0) { *lpfEnabled = *lpfEnabled && fEnabled; return c; }
        }
    }
    return 0;
}
int ICompatAccelMenu(HWND hwnd, UINT cmd, BOOL fNotify) {
    HMENU rgh[64], hmenu, hsub;
    int rgi[64], c, i;
    BOOL fEnabled;
    HWND lpwin = LpCompatWindow(hwnd);

    if (!lpwin || !lpwin->hmenu) return 0;
    hmenu = lpwin->hmenu;
    c = CAccelPath(hmenu, cmd, rgh, rgi, 0, &fEnabled);
    if (c == 0) return 0;
    if (!fNotify) return fEnabled ? 1 : 2;
    win32::SendMessageA(hwnd, WM_INITMENU, (WPARAM)hmenu, 0);
    for (i = 0; i < 63; i++) {
        lpwin = LpCompatWindow(hwnd);
        if (!lpwin) return 2;
        c = CAccelPath(lpwin->hmenu, cmd, rgh, rgi, 0, &fEnabled);
        if (c == 0) return 0;
        if (i >= c - 1) return fEnabled ? 1 : 2;
        hsub = win32::GetSubMenu(rgh[i], rgi[i]);
        win32::SendMessageA(hwnd, WM_INITMENUPOPUP, (WPARAM)hsub, MAKELPARAM(rgi[i], 0));
    }
    lpwin = LpCompatWindow(hwnd);
    if (!lpwin) return 2;
    c = CAccelPath(lpwin->hmenu, cmd, rgh, rgi, 0, &fEnabled);
    return c == 0 ? 0 : (fEnabled ? 1 : 2);
}
