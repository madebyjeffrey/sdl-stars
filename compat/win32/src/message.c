/* C11 adaptation of Free API message queue and winuser_message.cpp.
 * Copyright (c) 2026 Robert Vokac. MIT license: ../LICENSE.free-api.
 * Stars adaptation: real filters, synchronous sends and no fallback window routing.
 */
#include "internal.h"
#include <stars_compat.h>
#include <stdlib.h>

typedef struct COMPATMESSAGE {
    MSG msg;
    BOOL fTimer, fInput;
    BYTE rgbKey[256];
    struct COMPATMESSAGE *lpNext;
} COMPATMESSAGE;

static COMPATMESSAGE *lpFirst, *lpLast;
static int cDispatch;

int CCompatDispatch(void) { return cDispatch; }

BOOL WINAPI PostMessageA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    COMPATMESSAGE *lpmsg;

    if (!SDL_IsMainThread() || (hwnd && !LpCompatWindow(hwnd)))
        return FALSE;
    lpmsg = (COMPATMESSAGE *)(calloc(1, sizeof(*lpmsg)));
    if (!lpmsg) {
        SDL_SetError("Win32 message allocation failed");
        return FALSE;
    }
    lpmsg->msg.hwnd = hwnd;
    lpmsg->msg.message = msg;
    lpmsg->msg.wParam = wParam;
    lpmsg->msg.lParam = lParam;
    lpmsg->msg.time = GetTickCount();
    if (lpLast)
        lpLast->lpNext = lpmsg;
    else
        lpFirst = lpmsg;
    lpLast = lpmsg;
    return TRUE;
}

BOOL StarsCompatPostMessageAsync(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    COMPATASYNCPOST *lpPost;
    SDL_Event event = {};

    if (SDL_IsMainThread()) return PostMessageA(hwnd, msg, wParam, lParam);
    lpPost = (COMPATASYNCPOST *)(malloc(sizeof(*lpPost)));
    if (!lpPost) return FALSE;
    lpPost->hwnd = hwnd;
    lpPost->msg = msg;
    lpPost->wParam = wParam;
    lpPost->lParam = lParam;
    event.type = SDL_EVENT_USER;
    event.user.code = COMPAT_ASYNC_POST_CODE;
    event.user.data1 = lpPost;
    if (!SDL_PushEvent(&event)) {
        free(lpPost);
        return FALSE;
    }
    return TRUE;
}

BOOL CompatPostInput(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    /* Keep the newest consecutive motion, without merging across button/key
       changes or application-posted messages. Old pointer positions otherwise
       accumulate behind expensive map redraws during captured thumb dragging. */
    if (SDL_IsMainThread() && msg == WM_MOUSEMOVE && lpLast && lpLast->fInput &&
        lpLast->msg.hwnd == hwnd && lpLast->msg.message == msg && lpLast->msg.wParam == w) {
        lpLast->msg.lParam = l;
        lpLast->msg.time = GetTickCount();
        CompatCopyInput(lpLast->rgbKey, &lpLast->msg.pt);
        return TRUE;
    }
    if (!PostMessageA(hwnd, msg, w, l)) return FALSE;
    lpLast->fInput = TRUE;
    CompatCopyInput(lpLast->rgbKey, &lpLast->msg.pt);
    return TRUE;
}

BOOL CompatQueueTimer(HWND hwnd, UINT_PTR id, TIMERPROC lpfn) {
    if (!PostMessageA(hwnd, WM_TIMER, id, (LPARAM)lpfn)) return FALSE;
    lpLast->fTimer = TRUE;
    return TRUE;
}

void CompatPurgeMessages(HWND hwnd) {
    COMPATMESSAGE **lplpmsg = &lpFirst;
    COMPATMESSAGE *lpmsg;

    lpLast = NULL;
    while (*lplpmsg) {
        lpmsg = *lplpmsg;
        if (lpmsg->msg.hwnd == hwnd) {
            *lplpmsg = lpmsg->lpNext;
            free(lpmsg);
        } else {
            lpLast = lpmsg;
            lplpmsg = &lpmsg->lpNext;
        }
    }
}

void CompatClearMessages(void) {
    COMPATMESSAGE *lpmsg;

    while (lpFirst) {
        lpmsg = lpFirst;
        lpFirst = lpmsg->lpNext;
        free(lpmsg);
    }
    lpLast = NULL;
}

static BOOL FMessageMatches(const MSG *lpmsg, HWND hwnd, UINT msgMin, UINT msgMax) {
    if (lpmsg->message == WM_QUIT)
        return TRUE;
    if ((INT_PTR)hwnd == -1) {
        if (lpmsg->hwnd)
            return FALSE;
    } else if (hwnd && lpmsg->hwnd != hwnd && !IsChild(hwnd, lpmsg->hwnd))
        return FALSE;
    return (msgMin == 0 && msgMax == 0) || (lpmsg->message >= msgMin && lpmsg->message <= msgMax);
}

static BOOL FValidFilter(HWND hwnd, UINT msgMin, UINT msgMax) {
    if (!SDL_IsMainThread() || (hwnd && (INT_PTR)hwnd != -1 && !LpCompatWindow(hwnd)) || msgMin > msgMax) {
        SDL_SetError("Invalid window or message filter on Win32 UI thread");
        return FALSE;
    }
    return TRUE;
}

static BOOL FQueuedMessage(LPMSG lpOut, HWND hwnd, UINT msgMin, UINT msgMax, UINT flags, BOOL fTimer) {
    COMPATMESSAGE *lpmsg, *lpPrev = NULL;

    for (lpmsg = lpFirst; lpmsg; lpPrev = lpmsg, lpmsg = lpmsg->lpNext) {
        if (lpmsg->fTimer != fTimer || !FMessageMatches(&lpmsg->msg, hwnd, msgMin, msgMax))
            continue;
        *lpOut = lpmsg->msg;
        if (flags & PM_REMOVE) {
            if (lpmsg->fInput) CompatReadInput(lpmsg->rgbKey);
            if (lpPrev) lpPrev->lpNext = lpmsg->lpNext;
            else lpFirst = lpmsg->lpNext;
            if (lpLast == lpmsg) lpLast = lpPrev;
            if (lpmsg->fTimer) CompatTimerDequeued(lpmsg->msg.hwnd, lpmsg->msg.wParam);
            free(lpmsg);
        }
        return TRUE;
    }
    return FALSE;
}

BOOL WINAPI PeekMessageA(LPMSG lpOut, HWND hwnd, UINT msgMin, UINT msgMax, UINT flags) {
    SDL_Event event;

    if (!lpOut || !FValidFilter(hwnd, msgMin, msgMax)) return FALSE;
    if (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)
        while (SDL_PollEvent(&event)) CompatRouteEvent(&event);
    if (FQueuedMessage(lpOut, hwnd, msgMin, msgMax, flags, FALSE)) return TRUE;
    if (CompatPeekPaint(lpOut, hwnd, msgMin, msgMax)) return TRUE;
    // A caller may validate after drawing with GetDC instead of entering WM_PAINT.
    CompatFlushPresent();
    CompatGenerateTimer(hwnd, msgMin, msgMax);
    return FQueuedMessage(lpOut, hwnd, msgMin, msgMax, flags, TRUE);
}

BOOL WINAPI GetMessageA(LPMSG lpmsg, HWND hwnd, UINT msgMin, UINT msgMax) {
    SDL_Event event;
    int cMillis;
    BOOL fMessage;

    if (!lpmsg || !FValidFilter(hwnd, msgMin, msgMax))
        return -1;
    for (;;) {
        // Thread timers have no enclosing HWND to abort. Let every modal
        // pump finish its C cleanup before the C++ caller receives the error.
        if (FStarsErrorPending()) return -1;
        fMessage = PeekMessageA(lpmsg, hwnd, msgMin, msgMax, PM_REMOVE);
        // Peeking also routes SDL events, which can invoke application code.
        if (FStarsErrorPending()) return -1;
        if (fMessage)
            return lpmsg->message == WM_QUIT ? FALSE : TRUE;
        cMillis = CMillisCompatTimerWait(hwnd, msgMin, msgMax);
        if (!(SDL_WasInit(SDL_INIT_EVENTS) & SDL_INIT_EVENTS)) return -1;
        if (SDL_WaitEventTimeout(&event, cMillis)) CompatRouteEvent(&event);
        else if (cMillis == -1) return -1;
    }
}

BOOL WINAPI WaitMessage(void) {
    MSG msg;
    SDL_Event event;
    int cMillis;

    if (!SDL_IsMainThread())
        return FALSE;
    while (!PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE)) {
        cMillis = CMillisCompatTimerWait(NULL, 0, 0);
        if (!(SDL_WasInit(SDL_INIT_EVENTS) & SDL_INIT_EVENTS)) return FALSE;
        if (SDL_WaitEventTimeout(&event, cMillis)) CompatRouteEvent(&event);
        else if (cMillis == -1) return FALSE;
    }
    return TRUE;
}

VOID WINAPI PostQuitMessage(int code) {
    PostMessageA(NULL, WM_QUIT, (WPARAM)code, 0);
}

LRESULT WINAPI CallWindowProcA(WNDPROC lpfn, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT lr;

    if (!SDL_IsMainThread() || !lpfn)
        return 0;
    cDispatch++;
    lr = CompatInvokeWindowProc(lpfn, hwnd, msg, wParam, lParam);
    cDispatch--;
    return lr;
}

LRESULT WINAPI SendMessageA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HWND lpwin = LpCompatWindow(hwnd);

    if (!lpwin)
        return 0;
    if (msg == WM_SIZE) CompatLayoutScrollbars(hwnd);
    if (!IsWindow(hwnd)) return 0;
    return CallWindowProcA(lpwin->lpfnWndProc, hwnd, msg, wParam, lParam);
}

LRESULT WINAPI DispatchMessageA(const MSG *lpmsg) {
    if (!lpmsg || !SDL_IsMainThread() || lpmsg->message == WM_QUIT) return 0;
    if (lpmsg->message == WM_TIMER && lpmsg->lParam) {
        if (lpmsg->hwnd && !LpCompatWindow(lpmsg->hwnd)) return 0;
        cDispatch++;
        CompatInvokeTimerProc((TIMERPROC)lpmsg->lParam, lpmsg->hwnd, WM_TIMER, lpmsg->wParam, GetTickCount());
        cDispatch--;
        return 0;
    }
    if (!lpmsg->hwnd) return 0;
    return SendMessageA(lpmsg->hwnd, lpmsg->message, lpmsg->wParam, lpmsg->lParam);
}

BOOL WINAPI TranslateMessage(const MSG *lpmsg) {
    // SDL text events supply printable characters, but omit control characters.
    if (lpmsg && (lpmsg->message == WM_KEYDOWN || lpmsg->message == WM_SYSKEYDOWN) &&
        (lpmsg->wParam == VK_BACK || lpmsg->wParam == VK_TAB ||
         lpmsg->wParam == VK_RETURN || lpmsg->wParam == VK_ESCAPE))
        PostMessageA(lpmsg->hwnd, lpmsg->message == WM_KEYDOWN ? WM_CHAR : WM_SYSCHAR,
                     lpmsg->wParam, lpmsg->lParam);
    return lpmsg && (lpmsg->message == WM_KEYDOWN || lpmsg->message == WM_KEYUP ||
                     lpmsg->message == WM_SYSKEYDOWN || lpmsg->message == WM_SYSKEYUP);
}

LRESULT WINAPI DefWindowProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HWND lpwin = LpCompatWindow(hwnd);
    size_t cch;
    const char *psz;
    char *pszNew;

    if (!lpwin)
        return 0;
    if (FCompatMenuMessage(hwnd, msg, wParam, lParam)) return 0;
    switch (msg) {
    case WM_SETCURSOR:
        if (lpwin->hwndParent && SendMessageA(lpwin->hwndParent, msg, wParam, lParam)) return TRUE;
        if (!LpCompatWindow(hwnd)) return FALSE;
        if (LOWORD(lParam) != HTCLIENT) { SetCursor(LoadCursorA(NULL, MAKEINTRESOURCE(32512))); return TRUE; }
        if (lpwin->lpclass->wc.hCursor) { SetCursor(lpwin->lpclass->wc.hCursor); return TRUE; }
        return FALSE;
    case WM_NCACTIVATE:
        lpwin->fCaptionActive = !!wParam;
        return TRUE;
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
        if (lpwin->hwndParent) return SendMessageA(lpwin->hwndParent, msg, wParam, lParam);
        return 0;
    case WM_CANCELMODE:
        if (GetCapture() == hwnd) ReleaseCapture();
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        if (BeginPaint(hwnd, &ps))
            EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        if (lpwin->lpclass->wc.hbrBackground) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            return FillRect((HDC)wParam, &rc, lpwin->lpclass->wc.hbrBackground);
        }
        return 0;
    case WM_WINDOWPOSCHANGING: {
        WINDOWPOS *lpwp = (WINDOWPOS *)lParam;
        MINMAXINFO mmi = {};
        const SDL_DisplayMode *lpMode;
        if (!lpwp || (lpwp->flags & SWP_NOSIZE) || lpwin->hwndParent ||
            ((lpwin->dwStyle & WS_POPUP) && !(lpwin->dwStyle & WS_THICKFRAME))) return 0;
        lpMode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(lpwin->lpWindow));
        if (lpMode) {
            mmi.ptMaxSize.x = mmi.ptMaxTrackSize.x = lpMode->w;
            mmi.ptMaxSize.y = mmi.ptMaxTrackSize.y = lpMode->h;
        }
        SendMessageA(hwnd, WM_GETMINMAXINFO, 0, (LPARAM)&mmi);
        if (!LpCompatWindow(hwnd)) return 0;
        if (mmi.ptMinTrackSize.x > 0) lpwp->cx = max(lpwp->cx, mmi.ptMinTrackSize.x);
        if (mmi.ptMinTrackSize.y > 0) lpwp->cy = max(lpwp->cy, mmi.ptMinTrackSize.y);
        if (mmi.ptMaxTrackSize.x > 0) lpwp->cx = min(lpwp->cx, mmi.ptMaxTrackSize.x);
        if (mmi.ptMaxTrackSize.y > 0) lpwp->cy = min(lpwp->cy, mmi.ptMaxTrackSize.y);
        return 0;
    }
    case WM_WINDOWPOSCHANGED: {
        const WINDOWPOS *lpwp = (const WINDOWPOS *)lParam;
        RECT rc;
        POINT pt = {0, 0};
        UINT flags;
        if (!lpwp) return 0;
        flags = lpwp->flags;
        if (!(flags & SWP_NOSIZE) && GetClientRect(hwnd, &rc))
            SendMessageA(hwnd, WM_SIZE, IsIconic(hwnd) ? SIZE_MINIMIZED :
                         IsZoomed(hwnd) ? SIZE_MAXIMIZED : SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
        if (!(flags & SWP_NOMOVE) && LpCompatWindow(hwnd)) {
            if (lpwin->hwndParent) {
                pt.x = lpwin->rcChild.left; pt.y = lpwin->rcChild.top;
            } else ClientToScreen(hwnd, &pt);
            SendMessageA(hwnd, WM_MOVE, 0, MAKELPARAM(pt.x, pt.y));
        }
        return 0;
    }
    case WM_NCCREATE:
        return TRUE;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_SETTEXT:
        if (!lParam || !FUtf8Valid((const char *)lParam, strlen((const char *)lParam)) ||
            !(pszNew = SDL_strdup((const char *)lParam))) return FALSE;
        if (lpwin->lpWindow && !FCompatNativeTitle(lpwin->lpWindow, pszNew)) {
            SDL_free(pszNew);
            return FALSE;
        }
        SDL_free(lpwin->pszTitle);
        lpwin->pszTitle = pszNew;
        return TRUE;
    case WM_GETTEXT:
        if (!lParam || wParam == 0)
            return 0;
        psz = lpwin->pszTitle;
        cch = CchUtf8Prefix(psz, (size_t)wParam - 1);
        memcpy((char *)lParam, psz, cch); ((char *)lParam)[cch] = 0;
        return (LRESULT)cch;
    case WM_GETTEXTLENGTH:
        return (LRESULT)SDL_strlen(lpwin->pszTitle);
    default:
        // WM_DESTROY does not quit: the application's last-window policy owns WM_QUIT.
        return 0;
    }
}
