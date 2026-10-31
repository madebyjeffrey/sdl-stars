/* C11 adaptation of Free API src/winuser_window.cpp and WindowRegistry.
 * Copyright (c) 2026 Robert Vokac. MIT license: ../LICENSE.free-api.
 * Stars adaptation: retained class data, independent handles and reentrant teardown.
 */
#include "internal.h"
#include "cleanup.h"
#include <stars_compat.h>
#include <stdlib.h>
#include <new>
#include <string.h>

static COMPATCLASS *lpClasses;
static HWND lpWindows, hwndFocus, hwndActive, hwndFirstTop;
static unsigned int atomNext = 1;
static uint64_t idInspectNext;
static BOOL fVideoOwned, fShutdown, fResizeWatch, fLiveResize;
static BOOL fDialogSequence;
static HWND hwndSequenceOwner;
static NATIVEOWNER<SDL_Window, FreeCompatWindow> lpSequenceWindow;
static NATIVEOWNER<SDL_Renderer, FreeCompatRenderer> lpSequenceRenderer;
static NATIVEOWNER<SDL_Texture, FreeCompatTexture> lpSequenceTexture;
static NATIVEOWNER<SDL_Surface, FreeCompatSurface> lpSequenceCanvas, lpSequenceComposite;
static int iSequenceScale;
static BOOL fSequenceOwnerEnabled;
static HWND hwndSequenceFocus;

void FreeCompatWindow(SDL_Window *lpWindow) noexcept {
    // Cocoa can expose hidden application windows when an attached dialog is
    // destroyed. HWND teardown already handles owned windows and focus.
    if (lpWindow && SDL_GetWindowParent(lpWindow) &&
        !(SDL_GetWindowFlags(lpWindow) & (SDL_WINDOW_POPUP_MENU | SDL_WINDOW_TOOLTIP)))
        SDL_SetWindowParent(lpWindow, NULL);
    SDL_DestroyWindow(lpWindow);
}

void FreeCompatRenderer(SDL_Renderer *lp) noexcept { SDL_DestroyRenderer(lp); }
void FreeCompatTexture(SDL_Texture *lp) noexcept { SDL_DestroyTexture(lp); }

static void DestroySequenceWindow(void) {
    lpSequenceTexture.reset();
    lpSequenceCanvas.reset();
    lpSequenceComposite.reset();
    lpSequenceRenderer.reset();
    lpSequenceWindow.reset();
    iSequenceScale = 0;
}

static char *PszNativeTitle(LPCSTR pszTitle) {
    return FUtf8Valid(pszTitle, strlen(pszTitle)) ? SDL_strdup(pszTitle) : NULL;
}

BOOL FCompatNativeTitle(SDL_Window *lpWindow, LPCSTR pszTitle) {
    if (!FUtf8Valid(pszTitle, strlen(pszTitle))) return FALSE;
    if (SDL_GetWindowFlags(lpWindow) & (SDL_WINDOW_POPUP_MENU | SDL_WINDOW_TOOLTIP)) return TRUE;
    return SDL_SetWindowTitle(lpWindow, pszTitle);
}

BOOL StarsCompatSetWindowTitleUtf8(HWND hwnd, const char *psz) {
    return SetWindowTextA(hwnd, psz);
}

static BOOL FNativePositionable(SDL_Window *lpWindow) {
    const char *pszDriver = SDL_GetCurrentVideoDriver();
    return !pszDriver || strcmp(pszDriver, "wayland") ||
           (SDL_GetWindowFlags(lpWindow) & (SDL_WINDOW_POPUP_MENU | SDL_WINDOW_TOOLTIP));
}

static BOOL FNativePosition(SDL_Window *lpWindow, int *lpx, int *lpy) {
    int x, y;
    int64_t xWide = 0, yWide = 0;

    do {
        // Wayland has no desktop origin for toplevels. Use an owner-local
        // origin; popup positions still come from the compositor.
        x = y = 0;
        if (FNativePositionable(lpWindow) && !SDL_GetWindowPosition(lpWindow, &x, &y)) return FALSE;
        xWide += x; yWide += y;
        if (!(SDL_GetWindowFlags(lpWindow) & (SDL_WINDOW_POPUP_MENU | SDL_WINDOW_TOOLTIP))) break;
        lpWindow = SDL_GetWindowParent(lpWindow);
    } while (lpWindow);
    if (xWide < INT32_MIN || xWide > INT32_MAX || yWide < INT32_MIN || yWide > INT32_MAX) return FALSE;
    *lpx = (int)xWide; *lpy = (int)yWide;
    return TRUE;
}

void CompatRestoreOwnerFocus(HWND hwndOwner, HWND hwndSaved) {
    HWND hwnd, hwndRoot, hwndCurrent;
    MSG msg;

    if (fShutdown || PeekMessageA(&msg, NULL, WM_QUIT, WM_QUIT, PM_NOREMOVE)) return;
    hwnd = IsWindowVisible(hwndSaved) && IsWindowEnabled(hwndSaved) ? hwndSaved : hwndOwner;
    hwndRoot = HwndCompatRoot(hwnd);
    if (!hwndRoot || hwndRoot->fDestroying || !IsWindowVisible(hwndRoot) ||
        !IsWindowEnabled(hwndRoot) || IsIconic(hwndRoot) || (hwndRoot->dwExStyle & WS_EX_NOACTIVATE)) return;
    // Teardown/enable callbacks may have opened another window. Keep its focus.
    hwndCurrent = GetActiveWindow();
    if (hwndCurrent && hwndCurrent != hwndRoot) return;
    hwndCurrent = GetFocus();
    if (hwndCurrent != hwndRoot && HwndCompatRoot(hwndCurrent) == hwndRoot) hwnd = hwndCurrent;
    // SetFocus updates wrapper state; Cocoa also needs native activation after
    // the modal owner becomes focusable again.
    SDL_RaiseWindow(hwndRoot->lpWindow.get());
    if (IsWindow(hwnd) && IsWindowEnabled(hwnd)) SetFocus(hwnd);
}

BOOL StarsCompatBeginDialogSequence(HWND hwndOwner) {
    if (!SDL_IsMainThread() || fDialogSequence || (hwndOwner && !IsWindow(hwndOwner))) return FALSE;
    fDialogSequence = TRUE;
    hwndSequenceOwner = hwndOwner ? HwndCompatRoot(hwndOwner) : NULL;
    fSequenceOwnerEnabled = hwndSequenceOwner && IsWindowEnabled(hwndSequenceOwner);
    hwndSequenceFocus = GetFocus();
    return TRUE;
}

BOOL FCompatPendingDialogSequence(HWND hwndOwner) {
    return fDialogSequence && lpSequenceWindow.get() && hwndOwner == hwndSequenceOwner;
}

void StarsCompatEndDialogSequence(void) {
    HWND hwndOwner = hwndSequenceOwner, hwndFocus = hwndSequenceFocus;
    BOOL fEnable = fSequenceOwnerEnabled;

    if (!SDL_IsMainThread()) return;
    DestroySequenceWindow();
    hwndSequenceOwner = NULL;
    fDialogSequence = FALSE;
    fSequenceOwnerEnabled = FALSE;
    hwndSequenceFocus = NULL;
    // Owner notifications can start another modal sequence. Retire this one's
    // state before callbacks so they cannot borrow already-destroyed resources.
    if (!fShutdown) {
        if (fEnable && IsWindow(hwndOwner)) EnableWindow(hwndOwner, TRUE);
        if (!fDialogSequence) CompatRestoreOwnerFocus(hwndOwner, hwndFocus);
    }
}

static void UpdateCompatLiveResize(SDL_Event *lpevent) {
    HWND lpwin;
    RECT rc;

    if (!SDL_IsMainThread() || fShutdown || fLiveResize ||
        lpevent->type != SDL_EVENT_WINDOW_EXPOSED) return;
    lpwin = LpCompatWindowFromId(lpevent->window.windowID);
    if (!lpwin || !lpwin->fCreated || !GetClientRect(lpwin, &rc)) return;
    if (!lpwin->fVisible) {
        // A reused wizard window stays native-visible while its next logical
        // page initializes. Exposure must restore the retained completed frame.
        if (lpwin->fDeferPresent && !(SDL_GetWindowFlags(lpwin->lpWindow.get()) & SDL_WINDOW_HIDDEN))
            FCompatRefreshPresent(lpwin);
        return;
    }
    if (lpevent->window.data1 != 1 || CCompatDispatch() != 0) {
        FCompatRefreshPresent(lpwin);
        return;
    }
    // SDL permits application drawing from a watcher during native live resize.
    StarsRestore<BOOL> restoreResize(fLiveResize);
    COMPATPRESENTSCOPE deferPresent(lpwin);
    fLiveResize = TRUE;
    SendMessageA(lpwin, WM_SIZE, IsZoomed(lpwin) ? SIZE_MAXIMIZED : SIZE_RESTORED,
                 MAKELPARAM(rc.right, rc.bottom));
    if (IsWindow(lpwin) && !FStarsErrorPending()) {
        InvalidateRect(lpwin, NULL, TRUE);
        FCompatPaintTree(lpwin);
        if (IsWindow(lpwin) && !FStarsErrorPending()) {
            deferPresent.Close();
            FCompatPresent(lpwin);
        }
    }
}

static bool SDLCALL FWatchLiveResize(void *lpv, SDL_Event *lpevent) noexcept {
    (void)lpv;
    CompatInvokeCleanup([](void *lpvEvent) { UpdateCompatLiveResize(static_cast<SDL_Event *>(lpvEvent)); }, lpevent, NULL);
    return true;
}

BOOL FCompatVideo(void) {
    if (!SDL_IsMainThread() || fShutdown) return FALSE;
    if (!(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)) {
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) return FALSE;
        fVideoOwned = TRUE;
    }
    if (!fResizeWatch) {
        if (!SDL_AddEventWatch(FWatchLiveResize, NULL)) return FALSE;
        fResizeWatch = TRUE;
    }
    CompatStartAsyncMessages();
    return TRUE;
}

HWND LpCompatWindow(HWND hwnd) {
    HWND lpwin;

    if (!SDL_IsMainThread())
        return NULL;
    for (lpwin = lpWindows; lpwin; lpwin = lpwin->lpNext)
        if (lpwin == hwnd && lpwin->fAlive)
            return lpwin;
    return NULL;
}

HWND HwndCompatRegistry(void) { return lpWindows; }

HWND LpCompatWindowFromId(SDL_WindowID id) {
    HWND lpwin;

    for (lpwin = lpWindows; lpwin; lpwin = lpwin->lpNext)
        if (lpwin->fAlive && lpwin->lpWindow.get() && lpwin->id == id)
            return lpwin;
    return NULL;
}

BOOL CompatPeekPaint(LPMSG lpmsg, HWND hwnd, UINT msgMin, UINT msgMax) {
    HWND lpwin;

    if ((INT_PTR)hwnd == -1 || ((msgMin != 0 || msgMax != 0) &&
        (WM_PAINT < msgMin || WM_PAINT > msgMax)))
        return FALSE;
    for (lpwin = lpWindows; lpwin; lpwin = lpwin->lpNext) {
        if (!lpwin->fAlive || !IsWindowVisible(lpwin) || !lpwin->fUpdate || lpwin->hdcPaint ||
            (hwnd && hwnd != lpwin && !IsChild(hwnd, lpwin)))
            continue;
        memset(lpmsg, 0, sizeof(*lpmsg));
        lpmsg->hwnd = lpwin;
        lpmsg->message = WM_PAINT;
        lpmsg->time = GetTickCount();
        return TRUE;
    }
    return FALSE;
}

static COMPATCLASS *LpFindClass(LPCSTR pszClass, HINSTANCE hInstance) {
    COMPATCLASS *lpclass;
    UINT_PTR idClass = (UINT_PTR)pszClass;

    if (!pszClass)
        return NULL;
    for (lpclass = lpClasses; lpclass; lpclass = lpclass->lpNext)
        if (lpclass->wc.hInstance == hInstance &&
            ((idClass <= UINT16_MAX && lpclass->atom == idClass) ||
             (idClass > UINT16_MAX && SDL_strcasecmp(lpclass->pszClass.get(), pszClass) == 0)))
            return lpclass;
    return NULL;
}

ATOM WINAPI RegisterClassA(const WNDCLASSA *lpwc) {
    COMPATCLASS *lpclass;

    if (!SDL_IsMainThread() || fShutdown || !lpwc || !lpwc->lpfnWndProc ||
        (UINT_PTR)lpwc->lpszClassName <= UINT16_MAX ||
        !FUtf8Valid(lpwc->lpszClassName, strlen(lpwc->lpszClassName)) ||
        lpwc->cbWndExtra < 0 || lpwc->cbClsExtra < 0 || atomNext > UINT16_MAX)
        return 0;
    if (LpFindClass(lpwc->lpszClassName, lpwc->hInstance)) {
        SDL_SetError("Win32 class already registered");
        return 0;
    }
    lpclass = new (std::nothrow) COMPATCLASS{};
    if (!lpclass)
        return 0;
    lpclass->wc = *lpwc;
    lpclass->pszClass.reset(SDL_strdup(lpwc->lpszClassName));
    if ((UINT_PTR)lpwc->lpszMenuName > UINT16_MAX)
        lpclass->pszMenu.reset(SDL_strdup(lpwc->lpszMenuName));
    if (lpwc->cbClsExtra != 0)
        lpclass->rgbExtra.reset((BYTE *)calloc(1, (size_t)lpwc->cbClsExtra));
    if (!lpclass->pszClass || ((UINT_PTR)lpwc->lpszMenuName > UINT16_MAX && !lpclass->pszMenu) ||
        (lpwc->cbClsExtra != 0 && !lpclass->rgbExtra)) {
        delete lpclass;
        return 0;
    }
    lpclass->wc.lpszClassName = lpclass->pszClass.get();
    if (lpclass->pszMenu)
        lpclass->wc.lpszMenuName = lpclass->pszMenu.get();
    lpclass->atom = (ATOM)atomNext++;
    lpclass->lpNext = lpClasses;
    lpClasses = lpclass;
    return lpclass->atom;
}

static void UnlinkCompatZ(HWND hwnd) {
    HWND *lphwndFirst = hwnd->hwndParent ? &hwnd->hwndParent->hwndFirstChild : &hwndFirstTop;
    if (hwnd->hwndZPrev) hwnd->hwndZPrev->hwndZNext = hwnd->hwndZNext;
    else if (*lphwndFirst == hwnd) *lphwndFirst = hwnd->hwndZNext;
    if (hwnd->hwndZNext) hwnd->hwndZNext->hwndZPrev = hwnd->hwndZPrev;
    hwnd->hwndZPrev = hwnd->hwndZNext = NULL;
}

static void InsertCompatZ(HWND hwnd, HWND hwndAfter) {
    HWND *lphwndFirst = hwnd->hwndParent ? &hwnd->hwndParent->hwndFirstChild : &hwndFirstTop;
    hwnd->hwndZPrev = hwndAfter;
    hwnd->hwndZNext = hwndAfter ? hwndAfter->hwndZNext : *lphwndFirst;
    if (hwnd->hwndZNext) hwnd->hwndZNext->hwndZPrev = hwnd;
    if (hwndAfter) hwndAfter->hwndZNext = hwnd;
    else *lphwndFirst = hwnd;
}

static HWND HwndCompatLast(HWND hwndFirst) {
    if (hwndFirst) while (hwndFirst->hwndZNext) hwndFirst = hwndFirst->hwndZNext;
    return hwndFirst;
}

static void DestroyCompatWindow(HWND lpwin, BOOL fDestroyMessage, BOOL fNotify) {
    HWND lpchild;
    HWND hwndParent = lpwin->hwndParent;
    HWND hwndOwner = lpwin->hwndOwner, hwndSaved = lpwin->hwndRestoreFocus;
    BOOL fRestore = fNotify && !hwndParent && lpwin->fVisible && !(lpwin->dwExStyle & WS_EX_NOACTIVATE) &&
        (GetActiveWindow() == lpwin || (lpwin->lpWindow.get() && SDL_GetKeyboardFocus() == lpwin->lpWindow.get()));

    lpwin->fDestroying = TRUE;
    if (fNotify && (GetCapture() == lpwin || IsChild(lpwin, GetCapture()))) ReleaseCapture();
    if (fNotify && (GetFocus() == lpwin || IsChild(lpwin, GetFocus()))) SetFocus(NULL);
    lpwin->fVisible = FALSE;
    if (fDestroyMessage && !(lpwin->dwExStyle & WS_EX_NOPARENTNOTIFY) && hwndParent && !hwndParent->fDestroying)
        SendMessageA(hwndParent, WM_PARENTNOTIFY, MAKEWPARAM(WM_DESTROY, lpwin->idChild), (LPARAM)lpwin);
    if (fDestroyMessage)
        SendMessageA(lpwin, WM_DESTROY, 0, 0);
    // WM_DESTROY sees live children; WM_NCDESTROY follows their teardown.
    for (lpchild = lpWindows; lpchild; lpchild = lpchild->lpNext)
        if (lpchild->fAlive && !lpchild->fDestroying &&
            (lpchild->hwndParent == lpwin || lpchild->hwndOwner == lpwin)) {
            if (fNotify) DestroyWindow(lpchild);
            else DestroyCompatWindow(lpchild, FALSE, FALSE);
        }
    if (fNotify) SendMessageA(lpwin, WM_NCDESTROY, 0, 0);
    CompatPurgeMessages(lpwin);
    CompatPurgeTimers(lpwin);
    if (hwndFocus == lpwin)
        hwndFocus = NULL;
    if (hwndActive == lpwin)
        hwndActive = NULL;
    CompatDestroyWindowMenu(lpwin);
    if (lpwin->fKeepNative && fDialogSequence && lpwin->hwndOwner == hwndSequenceOwner &&
        lpwin->lpWindow.get() && !lpSequenceWindow.get()) {
        SDL_StopTextInput(lpwin->lpWindow.get());
        lpSequenceWindow.reset(lpwin->lpWindow.release());
        lpSequenceRenderer.reset(lpwin->lpRenderer.release());
        // Keep the completed frame available while the next page is built.
        lpSequenceTexture.reset(lpwin->lpTexture.release());
        lpSequenceCanvas.reset(lpwin->lpCanvas.release());
        lpSequenceComposite.reset(lpwin->lpComposite.release());
        iSequenceScale = lpwin->iCanvasScale;
    }
    CompatDestroyPaint(lpwin);
    CompatDestroyDialog(lpwin);
    // Silent shutdown skips application callbacks; control storage still closes.
    // Control-owned storage must still be released on that silent path.
    CompatDestroyList(lpwin);
    CompatDestroyToolbar(lpwin);
    lpwin->lpcombo.reset();
    SDL_free(lpwin->pszEditComposition); lpwin->pszEditComposition = NULL;
    SDL_free(lpwin->pszEditUndo);
    lpwin->pszEditUndo = NULL;
    if (lpwin->lpWindow) SDL_StopTextInput(lpwin->lpWindow.get());
    lpwin->lpWindow.reset();
    free(lpwin->rgbExtra);
    lpwin->rgbExtra = NULL;
    SDL_free(lpwin->pszTitle);
    lpwin->pszTitle = NULL;
    UnlinkCompatZ(lpwin);
    lpwin->fAlive = FALSE;
    if (fRestore) CompatRestoreOwnerFocus(hwndOwner, hwndSaved);
    if (fNotify && IsWindowVisible(hwndParent) && !hwndParent->fDestroying)
        FCompatPresent(hwndParent);
    // Keep a small handle token until shutdown so stale HWNDs cannot alias a new window.
}

static BOOL FFullscreenPopup(DWORD dwStyle, DWORD dwExStyle, HMENU hMenu, int x, int y, int dx, int dy) {
    SDL_Rect rc = {x, y, dx, dy}, rcDisplay;
    SDL_DisplayID id;

    if (!(dwStyle & WS_POPUP) || (dwStyle & (WS_CHILD | WS_CAPTION | WS_THICKFRAME)) ||
        (dwExStyle & WS_EX_NOACTIVATE) || hMenu || dx <= 0 || dy <= 0) return FALSE;
    id = SDL_GetDisplayForRect(&rc);
    return id && SDL_GetDisplayBounds(id, &rcDisplay) &&
           x == rcDisplay.x && y == rcDisplay.y && dx == rcDisplay.w && dy == rcDisplay.h;
}

HWND WINAPI CreateWindowExA(DWORD dwExStyle, LPCSTR pszClass, LPCSTR pszTitle, DWORD dwStyle,
                            int x, int y, int dx, int dy, HWND hwndParent, HMENU hMenu,
                            HINSTANCE hInstance, LPVOID lpParam) {
    COMPATCLASS *lpclass;
    HWND lpwin, lpparent, lpancestor;
    int cDepth = 0;
    BOOL fChild = (dwStyle & WS_CHILD) != 0;
    BOOL fReuse = FALSE;
    BOOL fNativePopup = FALSE;
    char *pszNative;
    int cxReuse, cyReuse;
    RECT rcOwner;
    SDL_WindowFlags flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    CREATESTRUCTA cs;
    RECT rcClient, rcWindow;
    LRESULT lr;

    if (pszTitle && !FUtf8Valid(pszTitle, strlen(pszTitle))) {
        SDL_SetError("Window text is invalid UTF-8");
        return NULL;
    }
    if (!SDL_IsMainThread() || fShutdown || idInspectNext == UINT64_MAX)
        return NULL;
    lpparent = LpCompatWindow(hwndParent);
    if ((hwndParent && (!lpparent || lpparent->fDestroying)) ||
        (fChild && !lpparent) || (!fChild && hMenu && !IsMenu(hMenu)) || (dwExStyle & ~(WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE))) {
        SDL_SetError("Invalid parent, top-level menu or unsupported extended style");
        return NULL;
    }
    for (lpancestor = lpparent; lpancestor; lpancestor =
         lpancestor->hwndParent ? lpancestor->hwndParent : lpancestor->hwndOwner)
        if (++cDepth >= 64) {
            SDL_SetError("Window nesting limit exceeded");
            return NULL;
        }
    lpclass = LpFindClass(pszClass, hInstance);
    if (!lpclass && FCompatControlClass(pszClass, hInstance)) lpclass = LpFindClass(pszClass, hInstance);
    if (!lpclass) {
        SDL_SetError("Win32 class not registered");
        return NULL;
    }
    if (!FCompatVideo()) return NULL;
    if (dx == CW_USEDEFAULT) {
        // Default width selects both dimensions; Stars passes zero height
        // with its initial frame's CW_USEDEFAULT width.
        dx = (dwStyle & (WS_CHILD | WS_POPUP)) ? 0 : 640;
        dy = (dwStyle & (WS_CHILD | WS_POPUP)) ? 0 : 480;
    }
    if (dy == CW_USEDEFAULT)
        dy = fChild ? 0 : 480;
    if (fChild) {
        if (x == CW_USEDEFAULT) x = 0;
        if (y == CW_USEDEFAULT) y = 0;
    }
    if (dx < 0) dx = 0;
    if (dy < 0) dy = 0;
    if (dwStyle & WS_THICKFRAME)
        flags |= SDL_WINDOW_RESIZABLE;
    // WS_CAPTION combines WS_BORDER and WS_DLGFRAME; a border alone has no title bar.
    if ((dwStyle & WS_POPUP) && (dwStyle & WS_CAPTION) != WS_CAPTION)
        flags |= SDL_WINDOW_BORDERLESS;
    lpwin = new (std::nothrow) HWND__{};
    if (!lpwin)
        return NULL;
    if (lpclass->wc.cbWndExtra != 0)
        lpwin->rgbExtra = (BYTE *)(calloc(1, (size_t)lpclass->wc.cbWndExtra));
    if (lpclass->wc.cbWndExtra != 0 && !lpwin->rgbExtra) {
        delete lpwin;
        return NULL;
    }
    lpwin->pszTitle = SDL_strdup(pszTitle ? pszTitle : "");
    if (!lpwin->pszTitle) {
        free(lpwin->rgbExtra);
        delete lpwin;
        return NULL;
    }
    if (fChild) {
        if ((int64_t)x + dx > INT32_MAX || (int64_t)y + dy > INT32_MAX) {
            SDL_free(lpwin->pszTitle);
            free(lpwin->rgbExtra);
            delete lpwin;
            return NULL;
        }
        lpwin->hwndParent = lpparent;
        lpwin->idChild = (LONG_PTR)hMenu;
        SetRect(&lpwin->rcChild, x, y, x + dx, y + dy);
    } else {
        lpwin->hwndOwner = lpparent ? HwndCompatRoot(lpparent) : NULL;
        if (lpwin->hwndOwner && HwndCompatRoot(GetFocus()) == lpwin->hwndOwner)
            lpwin->hwndRestoreFocus = GetFocus();
        if (lpwin->hwndOwner && (lpwin->hwndOwner->dwExStyle & WS_EX_TOPMOST)) dwExStyle |= WS_EX_TOPMOST;
        if (dwExStyle & WS_EX_TOPMOST) flags |= SDL_WINDOW_ALWAYS_ON_TOP;
        if ((dwExStyle & WS_EX_NOACTIVATE) || (dwStyle & WS_DISABLED)) flags |= SDL_WINDOW_NOT_FOCUSABLE;
        if (fDialogSequence && lpSequenceWindow.get() && lpwin->hwndOwner == hwndSequenceOwner &&
            (UINT_PTR)pszClass > UINT16_MAX && !strcmp(pszClass, "#32770") &&
            SDL_GetWindowSize(lpSequenceWindow.get(), &cxReuse, &cyReuse) && cxReuse == dx && cyReuse == dy &&
            (SDL_GetWindowFlags(lpSequenceWindow.get()) & (SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE |
             SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_NOT_FOCUSABLE)) ==
            (flags & (SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_NOT_FOCUSABLE))) {
            lpwin->lpWindow.reset(lpSequenceWindow.release());
            lpwin->lpRenderer.reset(lpSequenceRenderer.release());
            lpwin->lpTexture.reset(lpSequenceTexture.release());
            lpwin->lpCanvas.reset(lpSequenceCanvas.release());
            lpwin->lpComposite.reset(lpSequenceComposite.release());
            lpwin->iCanvasScale = iSequenceScale;
            iSequenceScale = 0;
            lpwin->fDeferPresent = fReuse = TRUE;
        } else {
            if (lpSequenceWindow.get() && lpwin->hwndOwner == hwndSequenceOwner &&
                (UINT_PTR)pszClass > UINT16_MAX && !strcmp(pszClass, "#32770")) {
                DestroySequenceWindow();
            }
            // Menus, combo lists and tooltips need compositor popup roles so
            // their placement remains relative to the owner without focus loss.
            fNativePopup = lpwin->hwndOwner && (UINT_PTR)pszClass > UINT16_MAX &&
                (!strcmp(pszClass, "StarsCompatPopup") || !strcmp(pszClass, "StarsComboPopup") ||
                 !strcmp(pszClass, "starstt") || !strcmp(pszClass, "starspopup") ||
                 !strcmp(pszClass, "StarsCompatToolbarTip")) &&
                strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0;
            if (fNativePopup) flags |= !strcmp(pszClass, "starstt") || !strcmp(pszClass, "starspopup") ||
                !strcmp(pszClass, "StarsCompatToolbarTip") ?
                SDL_WINDOW_TOOLTIP : SDL_WINDOW_POPUP_MENU;
            // Cocoa otherwise gives the unpainted popup view an opaque black
            // backing before the renderer's first drawable reaches the screen.
            if (fNativePopup && !strcmp(SDL_GetCurrentVideoDriver(), "cocoa")) flags |= SDL_WINDOW_TRANSPARENT;
            // Native captions and control text share the same UTF-8 representation.
            pszNative = fNativePopup ? NULL : PszNativeTitle(lpwin->pszTitle);
            if (fNativePopup && GetWindowRect(lpwin->hwndOwner, &rcOwner))
                lpwin->lpWindow.reset(SDL_CreatePopupWindow(lpwin->hwndOwner->lpWindow.get(),
                    x - rcOwner.left, y - rcOwner.top, StarsMax(1, dx), StarsMax(1, dy), flags));
            else if (pszNative && !fNativePopup)
                lpwin->lpWindow.reset(SDL_CreateWindow(pszNative, StarsMax(1, dx), StarsMax(1, dy), flags));
            SDL_free(pszNative);
        }
        if (!lpwin->lpWindow.get() ||
            (!fReuse && !fNativePopup && FNativePositionable(lpwin->lpWindow.get()) && !SDL_SetWindowPosition(lpwin->lpWindow.get(),
                                  x == CW_USEDEFAULT ? (int)SDL_WINDOWPOS_CENTERED : x,
                                  y == CW_USEDEFAULT ? (int)SDL_WINDOWPOS_CENTERED : y)) ||
            (!fReuse && !SDL_SyncWindow(lpwin->lpWindow.get())) ||
            // Cocoa can expose an owned popup during creation despite SDL_WINDOW_HIDDEN.
            (!fReuse && !SDL_HideWindow(lpwin->lpWindow.get())) ||
            // A screen-covering borderless popup must be native fullscreen so
            // desktop panels cannot cover its controls. Dialogs stay windowed.
            (FFullscreenPopup(dwStyle, dwExStyle, hMenu, x, y, dx, dy) &&
             !SDL_SetWindowFullscreen(lpwin->lpWindow.get(), true)) ||
            (!fReuse && !fNativePopup && lpwin->hwndOwner && lpwin->hwndOwner->lpWindow.get() &&
             ((SDL_GetWindowFlags(lpwin->hwndOwner->lpWindow.get()) & SDL_WINDOW_FULLSCREEN) ||
              ((UINT_PTR)pszClass > UINT16_MAX && !strcmp(pszClass, "#32770"))) &&
             // Native dialogs need a transient parent even when the owner is windowed.
             // The dummy driver has no native window hierarchy; HWND ownership
             // still applies there, but SDL_SetWindowParent is unsupported.
             strcmp(SDL_GetCurrentVideoDriver(), "dummy") != 0 &&
             !SDL_SetWindowParent(lpwin->lpWindow.get(), lpwin->hwndOwner->lpWindow.get()))) {
            CompatDestroyPaint(lpwin);
            lpwin->lpWindow.reset();
            SDL_free(lpwin->pszTitle);
            free(lpwin->rgbExtra);
            delete lpwin;
            return NULL;
        }
        if (!fReuse) CompatSetWindowIcon(lpwin->lpWindow.get(), lpclass->wc.hIcon);
        lpwin->id = SDL_GetWindowID(lpwin->lpWindow.get());
        lpwin->fZeroX = dx == 0; lpwin->fZeroY = dy == 0;
    }
    lpwin->lpclass = lpclass;
    lpwin->lpfnWndProc = lpclass->wc.lpfnWndProc;
    lpwin->dwStyle = dwStyle;
    lpwin->dwExStyle = dwExStyle;
    lpwin->idInspect = ++idInspectNext;
    lpwin->fAlive = TRUE;
    lpwin->rgscr[0].iMax = lpwin->rgscr[1].iMax = 100;
    lpwin->lpNext = lpWindows;
    lpWindows = lpwin;
    lpwin->swState = SW_SHOWNORMAL;
    CompatRememberNormal(lpwin);
    if (lpwin->hwndParent) InsertCompatZ(lpwin, HwndCompatLast(lpwin->hwndParent->hwndFirstChild));
    else {
        HWND hwndAfter = NULL, lp;
        for (lp = hwndFirstTop; !(dwExStyle & WS_EX_TOPMOST) && lp && (lp->dwExStyle & WS_EX_TOPMOST);
             lp = lp->hwndZNext) hwndAfter = lp;
        InsertCompatZ(lpwin, hwndAfter);
    }
    memset(&cs, 0, sizeof(cs));
    cs.lpCreateParams = lpParam;
    cs.hInstance = hInstance;
    cs.hwndParent = hwndParent;
    if (!fChild && !hMenu && lpclass->wc.lpszMenuName) {
        hMenu = LoadMenuA(lpclass->wc.hInstance, lpclass->wc.lpszMenuName);
        if (!hMenu) {
            DestroyCompatWindow(lpwin, FALSE, TRUE);
            return NULL;
        }
    }
    if (!fChild && hMenu && !SetMenu(lpwin, hMenu)) {
        DestroyCompatWindow(lpwin, FALSE, TRUE);
        return NULL;
    }
    cs.hMenu = hMenu;
    cs.cx = dx; cs.cy = dy; cs.x = x; cs.y = y;
    cs.style = (LONG)dwStyle;
    cs.dwExStyle = dwExStyle;
    cs.lpszName = pszTitle;
    cs.lpszClass = pszClass;
    lr = SendMessageA(lpwin, WM_NCCREATE, 0, (LPARAM)&cs);
    if (!lpwin->fAlive)
        return NULL;
    if (!lr) {
        DestroyCompatWindow(lpwin, FALSE, TRUE);
        return NULL;
    }
    lr = SendMessageA(lpwin, WM_CREATE, 0, (LPARAM)&cs);
    if (!lpwin->fAlive)
        return NULL;
    if (lr == -1) {
        // A rejected creation receives WM_NCDESTROY, not ordinary WM_DESTROY.
        DestroyCompatWindow(lpwin, FALSE, TRUE);
        return NULL;
    }
    lpwin->fCreated = TRUE;
    // Register the new page before a native caption update can expose it.
    if (fReuse && !FCompatNativeTitle(lpwin->lpWindow.get(), lpwin->pszTitle)) {
        DestroyWindow(lpwin);
        return NULL;
    }
    CompatLayoutScrollbars(lpwin);
    if (!lpwin->fAlive) return NULL;
    if (lpwin->hwndParent) {
        SendMessageA(lpwin, WM_MOVE, 0, MAKELPARAM(x, y));
        if (!lpwin->fAlive) return NULL;
        GetClientRect(lpwin, &rcClient);
        SendMessageA(lpwin, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rcClient.right, rcClient.bottom));
        if (!lpwin->fAlive) return NULL;
        if (!(lpwin->dwExStyle & WS_EX_NOPARENTNOTIFY))
            SendMessageA(lpwin->hwndParent, WM_PARENTNOTIFY, MAKEWPARAM(WM_CREATE, lpwin->idChild), (LPARAM)lpwin);
        if (!lpwin->fAlive) return NULL;
    } else {
        // Win32 initializes top-level client layout even when the native
        // window is hidden and SDL has not emitted a resize event.
        if (GetWindowRect(lpwin, &rcWindow))
            SendMessageA(lpwin, WM_MOVE, 0, MAKELPARAM(rcWindow.left, rcWindow.top));
        if (!lpwin->fAlive) return NULL;
        if (GetClientRect(lpwin, &rcClient))
            SendMessageA(lpwin, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rcClient.right, rcClient.bottom));
        if (!lpwin->fAlive) return NULL;
    }
    if (lpwin->lpWindow.get())
        SDL_StartTextInput(lpwin->lpWindow.get());
    if (dwStyle & WS_VISIBLE)
        ShowWindow(lpwin, dwStyle & WS_MINIMIZE ? SW_SHOWMINIMIZED :
                   dwStyle & WS_MAXIMIZE ? SW_SHOWMAXIMIZED : SW_SHOW);
    return lpwin->fAlive ? lpwin : NULL;
}

BOOL WINAPI DestroyWindow(HWND hwnd) {
    HWND lpwin = LpCompatWindow(hwnd);

    if (!lpwin || lpwin->fDestroying)
        return FALSE;
    DestroyCompatWindow(lpwin, TRUE, TRUE);
    return TRUE;
}

LONG_PTR WINAPI GetWindowLongPtrA(HWND hwnd, int ib) {
    HWND lpwin = LpCompatWindow(hwnd);
    LONG_PTR l = 0;

    if (!lpwin)
        return 0;
    if (lpwin->fDialog && ib == DWLP_MSGRESULT) return lpwin->lDlgMsgResult;
    if (lpwin->fDialog && ib == DWLP_DLGPROC) { memcpy(&l, &lpwin->lpfnDlg, sizeof(l)); return l; }
    if (ib == GWLP_USERDATA)
        return lpwin->lUserData;
    if (ib == GWLP_WNDPROC) {
        static_assert(sizeof(l) == sizeof(lpwin->lpfnWndProc), "native callback width");
        memcpy(&l, &lpwin->lpfnWndProc, sizeof(l));
        return l;
    }
    if (ib == GWLP_ID)
        return lpwin->idChild;
    if (ib == GWLP_HINSTANCE)
        return (LONG_PTR)lpwin->lpclass->wc.hInstance;
    if (ib == GWL_STYLE)
        return (LONG)lpwin->dwStyle;
    if (ib == GWL_EXSTYLE)
        return (LONG)lpwin->dwExStyle;
    if (ib >= 0 && ib <= lpwin->lpclass->wc.cbWndExtra - (int)sizeof(l)) {
        memcpy(&l, lpwin->rgbExtra + ib, sizeof(l));
        return l;
    }
    SDL_SetError("Unsupported or out-of-bounds window data index");
    return 0;
}

LONG_PTR WINAPI SetWindowLongPtrA(HWND hwnd, int ib, LONG_PTR lNew) {
    HWND lpwin = LpCompatWindow(hwnd);
    LONG_PTR lOld;
    WNDPROC lpfn;

    if (!lpwin)
        return 0;
    if (lpwin->fDialog && ib == DWLP_MSGRESULT) { lOld = lpwin->lDlgMsgResult; lpwin->lDlgMsgResult = lNew; return lOld; }
    if (lpwin->fDialog && ib == DWLP_DLGPROC) {
        memcpy(&lOld, &lpwin->lpfnDlg, sizeof(lOld)); memcpy(&lpwin->lpfnDlg, &lNew, sizeof(lNew)); return lOld;
    }
    if (ib == GWLP_USERDATA) {
        lOld = lpwin->lUserData;
        lpwin->lUserData = lNew;
        return lOld;
    }
    if (ib == GWLP_ID && lpwin->hwndParent) {
        lOld = lpwin->idChild;
        lpwin->idChild = lNew;
        return lOld;
    }
    if (ib == GWLP_WNDPROC) {
        memcpy(&lpfn, &lNew, sizeof(lpfn));
        if (!lpfn)
            return 0;
        lOld = GetWindowLongPtrA(hwnd, ib);
        lpwin->lpfnWndProc = lpfn;
        return lOld;
    }
    if (ib >= 0 && ib <= lpwin->lpclass->wc.cbWndExtra - (int)sizeof(lNew)) {
        memcpy(&lOld, lpwin->rgbExtra + ib, sizeof(lOld));
        memcpy(lpwin->rgbExtra + ib, &lNew, sizeof(lNew));
        return lOld;
    }
    SDL_SetError("Unsupported or out-of-bounds window data index");
    return 0;
}

void CompatRememberNormal(HWND hwnd) {
    HWND lpwin = LpCompatWindow(hwnd);
    if (!lpwin || lpwin->swState != SW_SHOWNORMAL) return;
    if (lpwin->hwndParent) lpwin->rcNormal = lpwin->rcChild;
    else if (!(SDL_GetWindowFlags(lpwin->lpWindow.get()) & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_MAXIMIZED)))
        GetWindowRect(hwnd, &lpwin->rcNormal);
}

void CompatWindowState(HWND hwnd, UINT sw, BOOL fNotify) {
    HWND lpwin = LpCompatWindow(hwnd);
    RECT rc;
    UINT swOld;
    if (!lpwin) return;
    swOld = lpwin->swState;
    if (swOld == sw && !fNotify) return;
    if (swOld != sw) CompatRememberNormal(hwnd);
    if (sw != swOld && sw == SW_SHOWMINIMIZED) lpwin->fRestoreMax = swOld == SW_SHOWMAXIMIZED;
    else if (sw == SW_SHOWNORMAL) lpwin->fRestoreMax = FALSE;
    lpwin->swState = sw;
    lpwin->dwStyle &= ~(WS_MINIMIZE | WS_MAXIMIZE);
    if (sw == SW_SHOWMINIMIZED) lpwin->dwStyle |= WS_MINIMIZE;
    else if (sw == SW_SHOWMAXIMIZED) lpwin->dwStyle |= WS_MAXIMIZE;
    if (fNotify && GetClientRect(hwnd, &rc))
        PostMessageA(hwnd, WM_SIZE, sw == SW_SHOWMINIMIZED ? SIZE_MINIMIZED :
                     sw == SW_SHOWMAXIMIZED ? SIZE_MAXIMIZED : SIZE_RESTORED,
                     MAKELPARAM(rc.right, rc.bottom));
}

BOOL WINAPI IsIconic(HWND hwnd) {
    HWND lpwin = LpCompatWindow(hwnd);
    return lpwin && lpwin->swState == SW_SHOWMINIMIZED;
}

BOOL WINAPI IsZoomed(HWND hwnd) {
    HWND lpwin = LpCompatWindow(hwnd);
    return lpwin && lpwin->swState == SW_SHOWMAXIMIZED;
}

BOOL WINAPI GetWindowPlacement(HWND hwnd, WINDOWPLACEMENT *lpwndpl) {
    HWND lpwin = LpCompatWindow(hwnd);
    WINDOWPLACEMENT wndpl = {};
    SDL_Rect rcDisplay, rcWork;
    SDL_DisplayID idDisplay;
    int64_t dx, dy, xLeft, xRight, yTop, yBottom;
    if (!lpwin) { SetLastError(ERROR_INVALID_WINDOW_HANDLE); return FALSE; }
    // Stars retains the Win16 length value 22 while using our native ABI.
    if (!lpwndpl || (lpwndpl->length != sizeof(wndpl) && lpwndpl->length != 22)) {
        SetLastError(ERROR_INVALID_PARAMETER); return FALSE;
    }
    CompatRememberNormal(hwnd);
    wndpl.length = sizeof(wndpl);
    wndpl.showCmd = lpwin->swState;
    wndpl.ptMinPosition.x = wndpl.ptMinPosition.y = -1;
    wndpl.ptMaxPosition.x = wndpl.ptMaxPosition.y = -1;
    wndpl.rcNormalPosition = lpwin->rcNormal;
    if (!lpwin->hwndParent) {
        idDisplay = SDL_GetDisplayForWindow(lpwin->lpWindow.get());
        if (!idDisplay || !SDL_GetDisplayBounds(idDisplay, &rcDisplay) ||
            !SDL_GetDisplayUsableBounds(idDisplay, &rcWork)) return FALSE;
        dx = (int64_t)rcWork.x - rcDisplay.x;
        dy = (int64_t)rcWork.y - rcDisplay.y;
        xLeft = (int64_t)wndpl.rcNormalPosition.left - dx;
        xRight = (int64_t)wndpl.rcNormalPosition.right - dx;
        yTop = (int64_t)wndpl.rcNormalPosition.top - dy;
        yBottom = (int64_t)wndpl.rcNormalPosition.bottom - dy;
        if (xLeft < INT32_MIN || xLeft > INT32_MAX || xRight < INT32_MIN || xRight > INT32_MAX ||
            yTop < INT32_MIN || yTop > INT32_MAX || yBottom < INT32_MIN || yBottom > INT32_MAX) {
            SetLastError(ERROR_ARITHMETIC_OVERFLOW); return FALSE;
        }
        SetRect(&wndpl.rcNormalPosition, (LONG)xLeft, (LONG)yTop, (LONG)xRight, (LONG)yBottom);
    }
    *lpwndpl = wndpl;
    return TRUE;
}

BOOL StarsCompatSetInitialShowCommand(HWND hwnd, int sw) {
    HWND lpwin = LpCompatWindow(hwnd);

    if (!lpwin || lpwin->hwndParent || lpwin->fVisible ||
        (sw != SW_HIDE && sw != SW_SHOW && sw != SW_SHOWNORMAL && sw != SW_RESTORE &&
         sw != SW_SHOWMINIMIZED && sw != SW_SHOWMAXIMIZED)) return FALSE;
    lpwin->swInitial = (UINT)sw;
    return TRUE;
}

static BOOL ShowCompatWindow(HWND hwnd, int sw, BOOL fRedraw, BOOL fNotify) {
    HWND lpwin = LpCompatWindow(hwnd);
    BOOL fWasVisible, fVisible, fOk, fPrepare;
    UINT swNew, swOld;
    RECT rc, rcOld, rcSaved;

    if (!lpwin)
        return FALSE;
    fWasVisible = lpwin->fVisible;
    fVisible = sw != SW_HIDE;
    fPrepare = !lpwin->hwndParent && fVisible && !fWasVisible;
    if (sw != SW_HIDE && sw != SW_SHOW && sw != SW_SHOWNORMAL && sw != SW_RESTORE &&
        sw != SW_SHOWMINIMIZED && sw != SW_SHOWMAXIMIZED) return FALSE;
    if (sw == SW_SHOW && lpwin->swInitial != 0) {
        sw = (int)lpwin->swInitial;
        // The headless driver cannot maximize/minimize a native window. Keep
        // the deferred startup show visible so client workflows can run there.
        if (!strcmp(SDL_GetCurrentVideoDriver(), "dummy") &&
            (sw == SW_SHOWMAXIMIZED || sw == SW_SHOWMINIMIZED)) sw = SW_SHOWNORMAL;
    }
    swOld = lpwin->swState;
    swNew = sw == SW_RESTORE && swOld == SW_SHOWMINIMIZED && lpwin->fRestoreMax ? SW_SHOWMAXIMIZED :
            sw == SW_RESTORE || sw == SW_SHOWNORMAL ? SW_SHOWNORMAL :
            sw == SW_SHOWMINIMIZED || sw == SW_SHOWMAXIMIZED ? (UINT)sw : swOld;
    CompatRememberNormal(hwnd);
    rcSaved = lpwin->rcNormal;
    // Renderer setup can recreate SDL's native window. Do it while hidden so
    // an owner's first paint during a modal loop cannot steal native focus.
    if (!lpwin->hwndParent && fVisible && !lpwin->lpRenderer.get() && !FCompatCanvas(hwnd)) return FALSE;
    if (fPrepare) {
        // Show-time layout must see logical visibility, while the native window
        // stays hidden until its background and visible descendants are painted.
        COMPATPRESENTSCOPE deferPresent(lpwin);
        lpwin->fVisible = TRUE;
        lpwin->dwStyle |= WS_VISIBLE;
        if (fNotify) SendMessageA(hwnd, WM_SHOWWINDOW, TRUE, 0);
        if (!IsWindow(hwnd) || FStarsErrorPending()) return FALSE;
        InvalidateRect(hwnd, NULL, TRUE);
        fOk = FCompatPaintTree(hwnd);
        if (!IsWindow(hwnd) || FStarsErrorPending()) return FALSE;
        deferPresent.Close();
        if (!fOk || !FCompatPresent(hwnd)) {
            lpwin->fVisible = FALSE;
            lpwin->dwStyle &= ~WS_VISIBLE;
            return FALSE;
        }
    }
    if (lpwin->hwndParent)
        fOk = TRUE;
    else if (sw == SW_HIDE)
        fOk = SDL_HideWindow(lpwin->lpWindow.get());
    else if (sw == SW_SHOW || (sw == SW_SHOWNORMAL && swOld == SW_SHOWNORMAL))
        fOk = SDL_ShowWindow(lpwin->lpWindow.get());
    else if (sw == SW_RESTORE || sw == SW_SHOWNORMAL)
        fOk = SDL_RestoreWindow(lpwin->lpWindow.get()) &&
              (swNew != SW_SHOWMAXIMIZED || SDL_MaximizeWindow(lpwin->lpWindow.get())) && SDL_ShowWindow(lpwin->lpWindow.get());
    else if (sw == SW_SHOWMINIMIZED)
        fOk = SDL_MinimizeWindow(lpwin->lpWindow.get()) && SDL_ShowWindow(lpwin->lpWindow.get());
    else if (sw == SW_SHOWMAXIMIZED)
        fOk = SDL_MaximizeWindow(lpwin->lpWindow.get()) && SDL_ShowWindow(lpwin->lpWindow.get());
    else {
        SDL_SetError("Unsupported ShowWindow command");
        return FALSE;
    }
    if (!fOk) {
        if (fPrepare) {
            lpwin->fVisible = FALSE;
            lpwin->dwStyle &= ~WS_VISIBLE;
        }
        return FALSE;
    }
    if (fVisible) lpwin->swInitial = 0;
    CompatWindowState(hwnd, swNew, FALSE);
    if (swOld == SW_SHOWNORMAL) lpwin->rcNormal = rcSaved;
    if (lpwin->hwndParent && swNew != swOld) {
        rcOld = lpwin->rcChild;
        rc = lpwin->rcNormal;
        if (swNew == SW_SHOWMAXIMIZED && !GetClientRect(lpwin->hwndParent, &rc)) return FALSE;
        if (swNew != SW_SHOWMINIMIZED &&
            !SetWindowPos(hwnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                          SWP_NOZORDER | SWP_NOACTIVATE)) {
            CompatWindowState(hwnd, swOld, FALSE);
            if (lpwin->fAlive) lpwin->rcNormal = rcSaved;
            return FALSE;
        }
        if (!lpwin->fAlive) return fWasVisible;
        if (swNew == SW_SHOWMINIMIZED ||
            (rcOld.right - rcOld.left == rc.right - rc.left && rcOld.bottom - rcOld.top == rc.bottom - rc.top)) {
            GetClientRect(hwnd, &rc);
            SendMessageA(hwnd, WM_SIZE, swNew == SW_SHOWMINIMIZED ? SIZE_MINIMIZED :
                         swNew == SW_SHOWMAXIMIZED ? SIZE_MAXIMIZED : SIZE_RESTORED,
                         MAKELPARAM(rc.right, rc.bottom));
            if (!lpwin->fAlive) return fWasVisible;
        }
    }
    lpwin->fVisible = fVisible;
    if (fVisible)
        lpwin->dwStyle |= WS_VISIBLE;
    else
        lpwin->dwStyle &= ~WS_VISIBLE;
    if (fRedraw && fVisible && !fWasVisible && lpwin->hwndParent)
        InvalidateRect(hwnd, NULL, TRUE);
    if (fNotify && !fPrepare && fVisible != fWasVisible)
        SendMessageA(hwnd, WM_SHOWWINDOW, (WPARAM)fVisible, 0);
    // A native renderer can acquire a new drawable when the hidden window is
    // shown. Submit the completed frame there without waiting for queued exposure.
    if (fPrepare && lpwin->fAlive) {
        COMPATPRESENTSCOPE deferPresent(lpwin);
        FCompatPaintTree(hwnd);
        if (!IsWindow(hwnd) || FStarsErrorPending()) return fWasVisible;
        deferPresent.Close();
        FCompatPresent(hwnd);
    }
    if (lpwin->fAlive && fVisible && !lpwin->hwndParent && fNotify &&
        swNew != SW_SHOWMINIMIZED && !(lpwin->dwStyle & WS_DISABLED) && !(lpwin->dwExStyle & WS_EX_NOACTIVATE)) {
        if (lpwin->hwndOwner && HwndCompatRoot(GetFocus()) == lpwin->hwndOwner)
            lpwin->hwndRestoreFocus = GetFocus();
        // Showing a Cocoa window alone does not grant native keyboard focus.
        // ShowWindow activates ordinary windows, including modeless dialogs.
        if (SDL_RaiseWindow(lpwin->lpWindow.get())) {
            if (HwndCompatRoot(GetFocus()) != hwnd) SetFocus(hwnd);
        }
    }
    if (fRedraw && lpwin->fAlive && lpwin->hwndParent)
        FCompatPresent(lpwin->hwndParent);
    return fWasVisible;
}

BOOL WINAPI ShowWindow(HWND hwnd, int sw) { return ShowCompatWindow(hwnd, sw, TRUE, TRUE); }

BOOL WINAPI IsWindowVisible(HWND hwnd) {
    HWND lpwin = LpCompatWindow(hwnd);
    if (!lpwin) return FALSE;
    for (; lpwin; lpwin = lpwin->hwndParent)
        if (!lpwin->fAlive || !lpwin->fVisible) return FALSE;
    return TRUE;
}

BOOL CompatGetSurfaceRect(HWND hwnd, RECT *lprc) {
    HWND lpwin = LpCompatWindow(hwnd);
    int dx, dy;

    if (!lpwin || !lprc) return FALSE;
    if (IsIconic(hwnd)) return SetRect(lprc, 0, 0, 0, 0);
    if (lpwin->hwndParent) {
        dx = lpwin->rcChild.right - lpwin->rcChild.left;
        dy = lpwin->rcChild.bottom - lpwin->rcChild.top;
    } else if (!SDL_GetWindowSize(lpwin->lpWindow.get(), &dx, &dy))
        return FALSE;
    if (!lpwin->hwndParent) {
        if (lpwin->fZeroX && dx == 1) dx = 0;
        else lpwin->fZeroX = FALSE;
        if (lpwin->fZeroY && dy == 1) dy = 0;
        else lpwin->fZeroY = FALSE;
    }
    if (!lpwin->hwndParent) dy = (((0) > (dy - CyCompatMenuBar(lpwin))) ? (0) : (dy - CyCompatMenuBar(lpwin)));
    return SetRect(lprc, 0, 0, dx, dy);
}

static BOOL FCompatPosition(HWND hwnd, WINDOWPOS *lpwp) {
    RECT rc;
    if (!GetClientRect(hwnd, &rc)) return FALSE;
    lpwp->cx = rc.right; lpwp->cy = rc.bottom + CyCompatMenuBar(hwnd);
    if (hwnd->hwndParent) {
        lpwp->x = hwnd->rcChild.left; lpwp->y = hwnd->rcChild.top;
        return TRUE;
    }
    return FNativePosition(hwnd->lpWindow.get(), &lpwp->x, &lpwp->y);
}

static void MarkCompatTopmost(HWND hwnd, BOOL fTopmost) {
    HWND lp;
    BOOL fWasTopmost = (hwnd->dwExStyle & WS_EX_TOPMOST) != 0;
    if (fTopmost) hwnd->dwExStyle |= WS_EX_TOPMOST;
    else hwnd->dwExStyle &= ~WS_EX_TOPMOST;
    if (hwnd->lpWindow.get() && fTopmost != fWasTopmost) SDL_SetWindowAlwaysOnTop(hwnd->lpWindow.get(), fTopmost);
    for (lp = lpWindows; lp; lp = lp->lpNext)
        if (lp->fAlive && lp->hwndOwner == hwnd) MarkCompatTopmost(lp, fTopmost);
}

static void NormalizeCompatTopmost(void) {
    HWND lp, lpNext, hwndAfter = NULL;
    for (lp = hwndFirstTop; lp; lp = lpNext) {
        lpNext = lp->hwndZNext;
        if (!(lp->dwExStyle & WS_EX_TOPMOST)) continue;
        UnlinkCompatZ(lp);
        InsertCompatZ(lp, hwndAfter);
        hwndAfter = lp;
    }
}

static void KeepCompatOwnedAbove(HWND hwnd) {
    HWND lp, lpPrev, lpNext;
    BOOL fBelow;
    for (lp = hwndFirstTop, fBelow = FALSE; lp; lp = lpNext) {
        lpNext = lp->hwndZNext;
        if (lp == hwnd) fBelow = TRUE;
        if (lp->hwndOwner == hwnd) {
            if (fBelow) {
                lpPrev = hwnd->hwndZPrev;
                UnlinkCompatZ(lp); InsertCompatZ(lp, lpPrev);
            }
            KeepCompatOwnedAbove(lp);
        }
    }
}

static BOOL FCompatInsertAfter(HWND hwnd, HWND hwndAfter) {
    HWND lp;
    if (hwndAfter == HWND_TOP || hwndAfter == HWND_BOTTOM ||
        hwndAfter == HWND_TOPMOST || hwndAfter == HWND_NOTOPMOST) return TRUE;
    lp = LpCompatWindow(hwndAfter);
    return lp && !lp->fDestroying && lp->hwndParent == hwnd->hwndParent;
}

static void ReorderCompatWindow(HWND hwnd, HWND hwndAfter) {
    HWND lp, hwndPrev;
    BOOL fTopmost;
    if (hwndAfter == hwnd) return;
    if (hwnd->hwndParent) {
        UnlinkCompatZ(hwnd);
        if (hwndAfter == HWND_BOTTOM) hwndAfter = HwndCompatLast(hwnd->hwndParent->hwndFirstChild);
        else if (hwndAfter == HWND_TOPMOST || hwndAfter == HWND_NOTOPMOST) hwndAfter = NULL;
        InsertCompatZ(hwnd, hwndAfter);
        return;
    }
    fTopmost = (hwnd->dwExStyle & WS_EX_TOPMOST) != 0;
    if (hwndAfter == HWND_TOPMOST) fTopmost = TRUE;
    else if (hwndAfter == HWND_BOTTOM || hwndAfter == HWND_NOTOPMOST) fTopmost = FALSE;
    else if (hwndAfter && (hwndAfter->dwExStyle & WS_EX_TOPMOST)) fTopmost = TRUE;
    MarkCompatTopmost(hwnd, fTopmost);
    if (!fTopmost)
        for (lp = hwnd->hwndOwner; lp; lp = lp->hwndOwner) MarkCompatTopmost(lp, FALSE);
    UnlinkCompatZ(hwnd);
    if (hwndAfter == HWND_BOTTOM) hwndAfter = HwndCompatLast(hwndFirstTop);
    else if (hwndAfter == HWND_TOPMOST || (hwndAfter == HWND_TOP && fTopmost)) hwndAfter = NULL;
    else if (hwndAfter == HWND_TOP || hwndAfter == HWND_NOTOPMOST) {
        hwndPrev = NULL;
        for (lp = hwndFirstTop; lp && (lp->dwExStyle & WS_EX_TOPMOST); lp = lp->hwndZNext) hwndPrev = lp;
        hwndAfter = hwndPrev;
    }
    InsertCompatZ(hwnd, hwndAfter);
    NormalizeCompatTopmost();
    for (lp = hwndFirstTop; lp; lp = lp->hwndZNext)
        if (!lp->hwndOwner) KeepCompatOwnedAbove(lp);
}

BOOL WINAPI SetWindowPos(HWND hwnd, HWND hwndAfter, int x, int y, int dx, int dy, UINT flags) {
    HWND lpwin = LpCompatWindow(hwnd);
    WINDOWPOS wp, wpOld;
    BOOL fShow, fHide, fZorder, fChanged, fRedraw;
    int xNative, yNative;
    const UINT flagsAllowed = SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW |
        SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_HIDEWINDOW | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING;

    if (!lpwin || lpwin->fDestroying || (flags & ~flagsAllowed) ||
        (!(flags & SWP_NOZORDER) && !FCompatInsertAfter(lpwin, hwndAfter))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    wp = WINDOWPOS{hwnd, hwndAfter, x, y, dx, dy, flags};
    wpOld = wp;
    if (!FCompatPosition(lpwin, &wpOld)) return FALSE;
    if (!(flags & SWP_NOSENDCHANGING)) SendMessageA(hwnd, WM_WINDOWPOSCHANGING, 0, (LPARAM)&wp);
    if (!lpwin->fAlive || lpwin->fDestroying) return FALSE;
    wp.hwnd = hwnd;
    if ((wp.flags & ~flagsAllowed) ||
        (!(wp.flags & SWP_NOZORDER) && !FCompatInsertAfter(lpwin, wp.hwndInsertAfter))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (wp.flags & SWP_NOMOVE) { wp.x = wpOld.x; wp.y = wpOld.y; }
    if (wp.flags & SWP_NOSIZE) { wp.cx = wpOld.cx; wp.cy = wpOld.cy; }
    if (wp.cx < 0) wp.cx = 0;
    if (wp.cy < 0) wp.cy = 0;
    if ((int64_t)wp.x + wp.cx > INT32_MAX || (int64_t)wp.y + wp.cy > INT32_MAX) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (wp.x == wpOld.x && wp.y == wpOld.y) wp.flags |= SWP_NOMOVE;
    if (wp.cx == wpOld.cx && wp.cy == wpOld.cy) wp.flags |= SWP_NOSIZE;
    fShow = (wp.flags & SWP_SHOWWINDOW) && !lpwin->fVisible;
    fHide = (wp.flags & SWP_HIDEWINDOW) && lpwin->fVisible;
    fZorder = !(wp.flags & SWP_NOZORDER) && wp.hwndInsertAfter != hwnd;
    fChanged = !(wp.flags & SWP_NOMOVE) || !(wp.flags & SWP_NOSIZE) || fShow || fHide || fZorder;
    fRedraw = !(wp.flags & SWP_NOREDRAW);
    if (lpwin->hwndParent)
        SetRect(&lpwin->rcChild, wp.x, wp.y, wp.x + wp.cx, wp.y + wp.cy);
    else {
        if (!(wp.flags & SWP_NOMOVE)) {
            xNative = yNative = 0;
            if ((SDL_GetWindowFlags(lpwin->lpWindow.get()) & (SDL_WINDOW_POPUP_MENU | SDL_WINDOW_TOOLTIP)) &&
                !FNativePosition(SDL_GetWindowParent(lpwin->lpWindow.get()), &xNative, &yNative)) return FALSE;
            if (FNativePositionable(lpwin->lpWindow.get()) &&
                !SDL_SetWindowPosition(lpwin->lpWindow.get(), wp.x - xNative, wp.y - yNative)) return FALSE;
        }
        if (!(wp.flags & SWP_NOSIZE)) {
            if (!SDL_SetWindowSize(lpwin->lpWindow.get(), StarsMax(1, wp.cx), StarsMax(1, wp.cy))) return FALSE;
            lpwin->fZeroX = wp.cx == 0; lpwin->fZeroY = wp.cy == 0;
        }
        if (!SDL_SyncWindow(lpwin->lpWindow.get())) return FALSE;
        FCompatPosition(lpwin, &wp);
    }
    CompatRememberNormal(hwnd);
    if (fZorder) ReorderCompatWindow(lpwin, wp.hwndInsertAfter);
    if (fShow || fHide) ShowCompatWindow(hwnd, fHide ? SW_HIDE : SW_SHOW, FALSE, FALSE);
    if (!lpwin->fAlive) return TRUE;
    if (!(wp.flags & SWP_NOACTIVATE) && !lpwin->hwndParent && lpwin->fVisible && IsWindowEnabled(hwnd) &&
        lpwin->swState != SW_SHOWMINIMIZED && !(lpwin->dwExStyle & WS_EX_NOACTIVATE)) {
        if (lpwin->hwndOwner && HwndCompatRoot(GetFocus()) == lpwin->hwndOwner)
            lpwin->hwndRestoreFocus = GetFocus();
        SDL_RaiseWindow(lpwin->lpWindow.get());
        SetFocus(hwnd);
        if (!lpwin->fAlive) return TRUE;
    }
    if (fChanged) SendMessageA(hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)&wp);
    if (!lpwin->fAlive) return TRUE;
    if (fRedraw && fChanged) {
        InvalidateRect(hwnd, NULL, TRUE);
        if (lpwin->hwndParent) InvalidateRect(lpwin->hwndParent, NULL, TRUE);
        FCompatPresent(hwnd);
    }
    return TRUE;
}

BOOL WINAPI MoveWindow(HWND hwnd, int x, int y, int dx, int dy, BOOL fRepaint) {
    if (!SetWindowPos(hwnd, NULL, x, y, dx, dy, SWP_NOZORDER | SWP_NOACTIVATE |
                      (fRepaint ? 0 : SWP_NOREDRAW))) return FALSE;
    return !fRepaint || !IsWindow(hwnd) || UpdateWindow(hwnd);
}

BOOL WINAPI SetWindowTextA(HWND hwnd, LPCSTR psz) {
    return psz && (BOOL)SendMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)psz);
}

int WINAPI GetWindowTextA(HWND hwnd, LPSTR psz, int cchMax) {
    if (!psz || cchMax <= 0)
        return 0;
    return (int)SendMessageA(hwnd, WM_GETTEXT, (WPARAM)cchMax, (LPARAM)psz);
}

int WINAPI GetClassNameA(HWND hwnd, LPSTR psz, int cchMax) {
    HWND lpwin = LpCompatWindow(hwnd);
    size_t cch;

    if (!lpwin || !psz || cchMax <= 0)
        return 0;
    cch = CchUtf8Prefix(lpwin->lpclass->pszClass.get(), (size_t)cchMax - 1);
    memcpy(psz, lpwin->lpclass->pszClass.get(), cch); psz[cch] = 0;
    return (int)cch;
}

HWND HwndCompatRoot(HWND hwnd) {
    HWND lpwin = LpCompatWindow(hwnd);
    if (!lpwin) return NULL;
    while (lpwin->hwndParent) lpwin = lpwin->hwndParent;
    return lpwin;
}

BOOL WINAPI IsWindow(HWND hwnd) { return LpCompatWindow(hwnd) != NULL; }

BOOL WINAPI IsChild(HWND hwndParent, HWND hwnd) {
    HWND lpwin = LpCompatWindow(hwnd);
    if (!LpCompatWindow(hwndParent)) return FALSE;
    for (lpwin = lpwin ? lpwin->hwndParent : NULL; lpwin; lpwin = lpwin->hwndParent)
        if (lpwin == hwndParent) return TRUE;
    return FALSE;
}

HWND WINAPI GetParent(HWND hwnd) {
    HWND lpwin = LpCompatWindow(hwnd);
    return lpwin ? lpwin->hwndParent ? lpwin->hwndParent : lpwin->hwndOwner : NULL;
}

HWND WINAPI GetWindow(HWND hwnd, UINT cmd) {
    HWND lpwin = LpCompatWindow(hwnd), hwndFirst;
    if (!lpwin) return NULL;
    hwndFirst = lpwin->hwndParent ? lpwin->hwndParent->hwndFirstChild : hwndFirstTop;
    switch (cmd) {
    case GW_OWNER: return lpwin->hwndOwner;
    case GW_CHILD: return lpwin->hwndFirstChild;
    case GW_HWNDFIRST: return hwndFirst;
    case GW_HWNDLAST: return HwndCompatLast(hwndFirst);
    case GW_HWNDPREV: return lpwin->hwndZPrev;
    case GW_HWNDNEXT: return lpwin->hwndZNext;
    default: return NULL;
    }
}
HWND HwndCompatFirstTop(void) { return hwndFirstTop; }

HWND WINAPI GetDlgItem(HWND hwnd, int id) {
    HWND lp;
    for (lp = GetWindow(hwnd, GW_CHILD); lp; lp = GetWindow(lp, GW_HWNDNEXT))
        if ((int)lp->idChild == id) return lp;
    return NULL;
}

int WINAPI GetDlgCtrlID(HWND hwnd) {
    HWND lpwin = LpCompatWindow(hwnd);
    return lpwin ? (int)lpwin->idChild : 0;
}

BOOL WINAPI GetClientRect(HWND hwnd, LPRECT lprc) {
    if (!CompatGetSurfaceRect(hwnd, lprc)) return FALSE;
    if (hwnd->lpcombo) return TRUE;
    if (hwnd->dwStyle & WS_VSCROLL) lprc->right = (((0) > (lprc->right - GetSystemMetrics(SM_CXVSCROLL))) ? (0) : (lprc->right - GetSystemMetrics(SM_CXVSCROLL)));
    if (hwnd->dwStyle & WS_HSCROLL) lprc->bottom = (((0) > (lprc->bottom - GetSystemMetrics(SM_CYHSCROLL))) ? (0) : (lprc->bottom - GetSystemMetrics(SM_CYHSCROLL)));
    return TRUE;
}

static BOOL FClientOrigin(HWND hwnd, int64_t *lpx, int64_t *lpy) {
    HWND lpwin = LpCompatWindow(hwnd);
    int x, y;
    if (!lpwin) return FALSE;
    *lpx = *lpy = 0;
    while (lpwin->hwndParent) {
        *lpx += lpwin->rcChild.left;
        *lpy += lpwin->rcChild.top;
        lpwin = lpwin->hwndParent;
    }
    if (!FNativePosition(lpwin->lpWindow.get(), &x, &y)) return FALSE;
    *lpx += x; *lpy += y + CyCompatMenuBar(lpwin);
    return TRUE;
}

static BOOL FConvertPoint(HWND hwnd, LPPOINT lppt, BOOL fToScreen) {
    int64_t x, y;
    if (!lppt || !FClientOrigin(hwnd, &x, &y)) return FALSE;
    x = (int64_t)lppt->x + (fToScreen ? x : -x);
    y = (int64_t)lppt->y + (fToScreen ? y : -y);
    if (x < INT32_MIN || x > INT32_MAX || y < INT32_MIN || y > INT32_MAX) return FALSE;
    lppt->x = (LONG)x; lppt->y = (LONG)y;
    return TRUE;
}

BOOL WINAPI ClientToScreen(HWND hwnd, LPPOINT lppt) { return FConvertPoint(hwnd, lppt, TRUE); }
BOOL WINAPI ScreenToClient(HWND hwnd, LPPOINT lppt) { return FConvertPoint(hwnd, lppt, FALSE); }

int WINAPI MapWindowPoints(HWND hwndFrom, HWND hwndTo, LPPOINT rgpt, UINT cpt) {
    int64_t xFrom = 0, yFrom = 0, xTo = 0, yTo = 0, dx, dy, x, y;
    UINT i;
    if (!SDL_IsMainThread() || (cpt != 0 && !rgpt)) {
        SetLastError(ERROR_INVALID_PARAMETER); return 0;
    }
    if ((hwndFrom && !FClientOrigin(hwndFrom, &xFrom, &yFrom)) ||
        (hwndTo && !FClientOrigin(hwndTo, &xTo, &yTo))) {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE); return 0;
    }
    dx = xFrom - xTo; dy = yFrom - yTo;
    /* Validate every coordinate before writing: a failed array mapping is atomic. */
    for (i = 0; i < cpt; i++) {
        x = (int64_t)rgpt[i].x + dx; y = (int64_t)rgpt[i].y + dy;
        if (x < INT32_MIN || x > INT32_MAX || y < INT32_MIN || y > INT32_MAX) {
            SetLastError(ERROR_ARITHMETIC_OVERFLOW); return 0;
        }
    }
    for (i = 0; i < cpt; i++) { rgpt[i].x = (LONG)((int64_t)rgpt[i].x + dx); rgpt[i].y = (LONG)((int64_t)rgpt[i].y + dy); }
    return (int)(int32_t)MAKELONG((WORD)dx, (WORD)dy);
}

BOOL WINAPI GetWindowRect(HWND hwnd, LPRECT lprc) {
    RECT rc;
    int64_t x, y;
    if (!lprc || !CompatGetSurfaceRect(hwnd, &rc) || !FClientOrigin(hwnd, &x, &y)) return FALSE;
    y -= CyCompatMenuBar(hwnd); rc.bottom += CyCompatMenuBar(hwnd);
    if (x < INT32_MIN || y < INT32_MIN || x + rc.right > INT32_MAX || y + rc.bottom > INT32_MAX)
        return FALSE;
    return SetRect(lprc, (LONG)x, (LONG)y, (LONG)(x + rc.right), (LONG)(y + rc.bottom));
}

BOOL WINAPI IsWindowEnabled(HWND hwnd) {
    HWND lpwin = LpCompatWindow(hwnd);
    return lpwin && !(lpwin->dwStyle & WS_DISABLED);
}

static void CompatSetActive(HWND hwnd) {
    HWND lpwin;
    if (hwndActive == hwnd) return;
    lpwin = LpCompatWindow(hwndActive);
    if (lpwin) lpwin->fCaptionActive = FALSE;
    hwndActive = hwnd;
    lpwin = LpCompatWindow(hwnd);
    if (lpwin) lpwin->fCaptionActive = TRUE;
}

BOOL WINAPI FlashWindow(HWND hwnd, BOOL fInvert) {
    HWND lpwin = LpCompatWindow(hwnd);
    BOOL fWasActive, fCaptionActive;
    SDL_FlashOperation operation;
    if (!lpwin || lpwin->fDestroying) {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE); return FALSE;
    }
    if (lpwin->hwndParent) { SetLastError(ERROR_NOT_SUPPORTED); return FALSE; }
    fWasActive = lpwin->fCaptionActive;
    fCaptionActive = fInvert ? !fWasActive : GetActiveWindow() == hwnd;
    operation = fCaptionActive && GetActiveWindow() != hwnd ? SDL_FLASH_BRIEFLY : SDL_FLASH_CANCEL;
    if (!SDL_FlashWindow(lpwin->lpWindow.get(), operation)) {
        SetLastError(ERROR_NOT_SUPPORTED); return FALSE;
    }
    lpwin->fCaptionActive = fCaptionActive;
    // Caption callbacks can destroy the HWND; retain the prior value locally.
    SendMessageA(hwnd, WM_NCACTIVATE, (WPARAM)fCaptionActive, 0);
    return fWasActive;
}

HWND WINAPI SetFocus(HWND hwnd) {
    HWND lpwin = LpCompatWindow(hwnd), lp, hwndOld = GetFocus();
    if (hwnd && (!lpwin || lpwin->fDestroying)) return NULL;
    for (lp = lpwin; lp; lp = lp->hwndParent)
        if (lp->dwStyle & WS_DISABLED) return NULL;
    if (hwndOld == hwnd) return hwndOld;
    // Removing keyboard focus does not deactivate the top-level window.
    if (hwnd) CompatSetActive(HwndCompatRoot(hwnd));
    hwndFocus = hwnd;
    if (hwndOld) SendMessageA(hwndOld, WM_KILLFOCUS, (WPARAM)hwnd, 0);
    // A focus callback can destroy or redirect the requested target.
    if (hwndFocus == hwnd && LpCompatWindow(hwnd))
        SendMessageA(hwnd, WM_SETFOCUS, (WPARAM)hwndOld, 0);
    return hwndOld;
}

BOOL WINAPI EnableWindow(HWND hwnd, BOOL fEnable) {
    HWND lpwin = LpCompatWindow(hwnd);
    BOOL fWasDisabled;
    if (!lpwin || lpwin->fDestroying) return FALSE;
    fWasDisabled = (lpwin->dwStyle & WS_DISABLED) != 0;
    fEnable = !!fEnable;
    if (fWasDisabled == !fEnable) return fWasDisabled;
    if (!fEnable) {
        SendMessageA(hwnd, WM_CANCELMODE, 0, 0);
        if (!lpwin->fAlive) return fWasDisabled;
        lpwin->dwStyle |= WS_DISABLED;
        if (GetFocus() == hwnd || IsChild(hwnd, GetFocus())) SetFocus(NULL);
    } else lpwin->dwStyle &= ~WS_DISABLED;
    /* External modal pickers own focus. Changing native focusability here can
       make a compositor withdraw the fullscreen title while the picker opens. */
    if (lpwin->fAlive && lpwin->lpWindow.get() && lpwin->cNativeFileDialogs == 0)
        SDL_SetWindowFocusable(lpwin->lpWindow.get(), fEnable && !(lpwin->dwExStyle & WS_EX_NOACTIVATE));
    if (lpwin->fAlive) SendMessageA(hwnd, WM_ENABLE, (WPARAM)fEnable, 0);
    return fWasDisabled;
}

static HWND HwndCompatHit(HWND hwnd, int *lpx, int *lpy, BOOL fPoint) {
    HWND lp;
    RECT rc, client;
    if (!IsWindowEnabled(hwnd)) return NULL;
    if (!CompatGetSurfaceRect(hwnd, &rc) || *lpx < 0 || *lpy < 0 || *lpx >= rc.right || *lpy >= rc.bottom)
        return hwnd;
    GetClientRect(hwnd, &client);
    for (lp = GetWindow(hwnd, GW_CHILD); lp; lp = GetWindow(lp, GW_HWNDNEXT)) {
        if ((!lp->fNcScroll && (*lpx >= client.right || *lpy >= client.bottom)) || !IsWindowVisible(lp) || !IsWindowEnabled(lp) ||
            (lp->iControl == 2 && (fPoint || !(lp->dwStyle & SS_NOTIFY))) ||
            (lp->iControl == 1 && (lp->dwStyle & BS_TYPEMASK) == BS_GROUPBOX) ||
            *lpx < lp->rcChild.left || *lpx >= lp->rcChild.right ||
            *lpy < lp->rcChild.top || *lpy >= lp->rcChild.bottom) continue;
        *lpx -= lp->rcChild.left; *lpy -= lp->rcChild.top;
        return HwndCompatHit(lp, lpx, lpy, fPoint);
    }
    return hwnd;
}

HWND HwndCompatMouse(HWND hwnd, int *lpx, int *lpy) { return HwndCompatHit(hwnd, lpx, lpy, FALSE); }

HWND WINAPI WindowFromPoint(POINT pt) {
    HWND hwnd, hwndPointer = NULL;
    const char *pszDriver;
    RECT rc;
    POINT ptClient;
    if (!SDL_IsMainThread()) return NULL;
    pszDriver = SDL_GetCurrentVideoDriver();
    if (pszDriver && !strcmp(pszDriver, "wayland")) hwndPointer = HwndCompatPointerRoot();
    for (hwnd = hwndFirstTop; hwnd; hwnd = hwnd->hwndZNext) {
        // Owner-local coordinates cannot distinguish independent Wayland roots.
        // Route pointer queries through the surface that supplied the event.
        if (hwndPointer && hwnd != hwndPointer) continue;
        if (!IsWindowVisible(hwnd) || !IsWindowEnabled(hwnd) || !GetWindowRect(hwnd, &rc) ||
            !PtInRect(&rc, pt)) continue;
        ptClient = pt;
        if (ScreenToClient(hwnd, &ptClient)) {
            if (ptClient.y < 0 && ptClient.y >= -CyCompatMenuBar(hwnd) && ptClient.x >= 0 &&
                ptClient.x < hwnd->cxMenu) return hwnd;
            return HwndCompatHit(hwnd, &ptClient.x, &ptClient.y, TRUE);
        }
    }
    return NULL;
}

HWND WINAPI GetFocus(void) { return LpCompatWindow(hwndFocus); }

HWND WINAPI GetActiveWindow(void) { return LpCompatWindow(hwndActive); }

void CompatFocusWindow(HWND hwnd) {
    HWND hwndOld = hwndFocus;

    // Queued SDL activation must not give a disabled modal owner input focus.
    if (hwnd && !IsWindowEnabled(hwnd)) return;
    CompatSetActive(HwndCompatRoot(hwnd));
    if (hwnd == hwndOld)
        return;
    hwndFocus = hwnd;
    if (LpCompatWindow(hwndOld)) {
        CompatPostInput(hwndOld, WM_KILLFOCUS, (WPARAM)hwnd, 0);
        CompatPostInput(hwndOld, WM_ACTIVATE, WA_INACTIVE, 0);
    }
    if (hwnd) {
        CompatPostInput(hwnd, WM_SETFOCUS, (WPARAM)hwndOld, 0);
        CompatPostInput(hwnd, WM_ACTIVATE, WA_ACTIVE, 0);
    }
}

static void ShutdownCompatRuntime(BOOL fNotify) {
    HWND lpwin, lpwinNext;
    COMPATCLASS *lpclass, *lpclassNext;

    if (!SDL_IsMainThread())
        return;
    if (CCompatDispatch() != 0) {
        SDL_SetError("StarsCompatShutdown cannot run inside a window procedure");
        return;
    }
    fShutdown = TRUE;
    CompatStopAsyncMessages();
    if (fResizeWatch) SDL_RemoveEventWatch(FWatchLiveResize, NULL);
    fResizeWatch = FALSE;
    StarsCompatEndDialogSequence();
    for (lpwin = lpWindows; lpwin; lpwin = lpwin->lpNext)
        if (lpwin->fAlive && !lpwin->fDestroying) {
            if (fNotify) DestroyWindow(lpwin);
            else DestroyCompatWindow(lpwin, FALSE, FALSE);
        }
    CompatShutdownInspector();
    CompatClearMessages();
    CompatResetInput();
    CompatClearTimers();
    CompatShutdownImages();
    for (lpwin = lpWindows; lpwin; lpwin = lpwinNext) {
        lpwinNext = lpwin->lpNext;
        delete lpwin;
    }
    for (lpclass = lpClasses; lpclass; lpclass = lpclassNext) {
        lpclassNext = lpclass->lpNext;
        delete lpclass;
    }
    CompatShutdownGdi();
    CompatShutdownMemory();
    CompatShutdownFiles();
    CompatShutdownMenus();
    CompatShutdownAccelerators();
    CompatShutdownAudio();
    lpWindows = NULL;
    lpClasses = NULL;
    hwndFocus = NULL;
    hwndActive = NULL;
    hwndFirstTop = NULL;
    atomNext = 1;
    if (fVideoOwned)
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    fVideoOwned = FALSE;
    fShutdown = FALSE;
}

BOOL StarsCompatCanShutdown() noexcept { return SDL_IsMainThread() && CCompatDispatch() == 0; }

void StarsCompatShutdown(void) { ShutdownCompatRuntime(TRUE); }
void StarsCompatProcessExit(void) { ShutdownCompatRuntime(FALSE); }
