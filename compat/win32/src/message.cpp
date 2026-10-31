/* C11 adaptation of Free API message queue and winuser_message.cpp.
 * Copyright (c) 2026 Robert Vokac. MIT license: ../LICENSE.free-api.
 * Stars adaptation: real filters, synchronous sends and no fallback window routing.
 */
#include "internal.h"
#include "message_alloc_io.h"
#include <stars_compat.h>
#include <stdlib.h>
#include "ownership.h"
#include "locking.h"
#include <atomic>
#include <initializer_list>
#include <new>



typedef struct COMPATMESSAGE {
    win32::MSG msg;
    STARSCOMPATPAYLOAD payload;
    BOOL fTimer, fInput;
    BYTE rgbKey[256];
    struct COMPATMESSAGE *lpNext;
} COMPATMESSAGE;

static COMPATMESSAGE *lpFirst, *lpLast;
static void FreeMessage(COMPATMESSAGE *lp) noexcept {
    if (lp) { lp->~COMPATMESSAGE(); free(lp); }
}
using COMPATMESSAGEOWNER = NATIVEOWNER<COMPATMESSAGE, FreeMessage>;
static COMPATMESSAGE *lpPending, *lpDispatchPayload;

static COMPATMESSAGEOWNER TakePayloadMessage(const win32::MSG &msg) noexcept {
    for (auto **first : {&lpPending, &lpFirst}) {
        for (auto **link = first; *link; link = &(*link)->lpNext) {
            auto *lp = *link;
            if (lp->payload && lp->msg.hwnd == msg.hwnd && lp->msg.message == msg.message &&
                lp->msg.wParam == msg.wParam && lp->msg.lParam == msg.lParam) {
                *link = lp->lpNext;
                if (lpLast == lp) {
                    lpLast = lpFirst;
                    if (lpLast) while (lpLast->lpNext) lpLast = lpLast->lpNext;
                }
                lp->lpNext = nullptr;
                return COMPATMESSAGEOWNER(lp);
            }
        }
    }
    return {};
}

void *PvCompatMessagePayload(WPARAM id) noexcept {
    return lpDispatchPayload && lpDispatchPayload->msg.wParam == id ? lpDispatchPayload->payload.get() : nullptr;
}

static int cDispatch;

class COMPATDISPATCHSCOPE {
public:
    COMPATDISPATCHSCOPE() noexcept { cDispatch++; }
    ~COMPATDISPATCHSCOPE() noexcept { cDispatch--; }
    COMPATDISPATCHSCOPE(const COMPATDISPATCHSCOPE &) = delete;
    COMPATDISPATCHSCOPE &operator=(const COMPATDISPATCHSCOPE &) = delete;
};

int CCompatDispatch(void) { return cDispatch; }

BOOL WINAPI win32::PostMessageA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    COMPATMESSAGE *lpmsg;

    if (!SDL_IsMainThread() || (hwnd && !LpCompatWindow(hwnd)))
        return FALSE;
    lpmsg = (COMPATMESSAGE *)(CompatMessageAllocIO().pfnAllocateZero(1, sizeof(*lpmsg)));
    if (!lpmsg) {
        SDL_SetError("Win32 message allocation failed");
        return FALSE;
    }
    lpmsg = ::new (lpmsg) COMPATMESSAGE{};
    COMPATMESSAGEOWNER message(lpmsg);
    lpmsg->msg.hwnd = hwnd;
    lpmsg->msg.message = msg;
    lpmsg->msg.wParam = wParam;
    lpmsg->msg.lParam = lParam;
    lpmsg->msg.time = win32::GetTickCount();
    if (lpLast)
        lpLast->lpNext = lpmsg;
    else
        lpFirst = lpmsg;
    lpLast = lpmsg;
    message.release();
    return TRUE;
}

struct COMPATASYNCPOST {
    HWND hwnd = nullptr;
    uint64_t idWindow = 0;
    UINT msg = 0;
    WPARAM wParam = 0;
    LPARAM lParam = 0;
    STARSCOMPATPAYLOAD payload;
    std::atomic<unsigned> cRef{2}; // Producer and registry; SDL only borrows.
    uintptr_t id = 0;
    COMPATASYNCPOST *lpNext = nullptr;
};

static NATIVEFLAGLOCK lockPosts;
static COMPATASYNCPOST *lpPosts;
static bool fAcceptPosts = true;
static uintptr_t idPostNext = 1;
static WPARAM idPayloadNext = 1;

static void ReleasePost(COMPATASYNCPOST *lp) noexcept {
    if (lp->cRef.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        lp->~COMPATASYNCPOST();
        free(lp);
    }
}

using COMPATASYNCOWNER = NATIVEOWNER<COMPATASYNCPOST, ReleasePost>;

static COMPATASYNCOWNER TakePost(const void *pv, uintptr_t id) noexcept {
    std::unique_lock guard(lockPosts);
    for (auto **link = &lpPosts; *link; link = &(*link)->lpNext) {
        auto *lp = *link;
        if (lp == pv && lp->id == id) {
            *link = lp->lpNext;
            lp->lpNext = nullptr;
            return COMPATASYNCOWNER(lp);
        }
    }
    return {};
}

void CompatStartAsyncMessages() noexcept {
    std::unique_lock guard(lockPosts);
    fAcceptPosts = true;
}

void CompatStopAsyncMessages() noexcept {
    COMPATASYNCPOST *lp;
    {
        std::unique_lock guard(lockPosts);
        fAcceptPosts = false;
        lp = std::exchange(lpPosts, nullptr);
    }
    while (lp) {
        auto *next = lp->lpNext;
        COMPATASYNCOWNER post(lp);
        lp = next;
    }
}

static BOOL PostOwnedMessage(uint64_t idWindow, UINT msg, STARSCOMPATPAYLOAD payload, LPARAM lParam) {
    {
        std::unique_lock guard(lockPosts);
        if (!fAcceptPosts) return FALSE;
    }
    HWND hwnd = StarsCompatResolveWindow(idWindow);
    if (!hwnd || !payload || idPayloadNext == 0) return FALSE;
    WPARAM id = idPayloadNext++;
    if (!win32::PostMessageA(hwnd, msg, id, lParam)) return FALSE;
    lpLast->payload = std::move(payload);
    return TRUE;
}

static BOOL PushAsyncPost(COMPATASYNCPOST *lp) noexcept {
    COMPATASYNCOWNER producer(lp);
    {
        std::unique_lock guard(lockPosts);
        if (!fAcceptPosts || idPostNext == 0) {
            // Only the producer owns a record that was never registered.
            lp->cRef.store(1, std::memory_order_relaxed);
            return FALSE;
        }
        lp->id = idPostNext++;
        lp->lpNext = lpPosts;
        lpPosts = lp;
    }
    SDL_Event event{};
    event.type = SDL_EVENT_USER;
    event.user.code = COMPAT_ASYNC_POST_CODE;
    event.user.data1 = lp;
    event.user.data2 = reinterpret_cast<void *>(lp->id);
    BOOL fOk = FALSE;
    // Do not hold the registry lock across SDL event-watch callbacks.
    try { fOk = SDL_PushEvent(&event); } catch (...) {}
    if (!fOk) {
        auto registered = TakePost(lp, lp->id);
    }
    return fOk;
}

BOOL StarsCompatPostMessageAsync(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (SDL_IsMainThread()) return win32::PostMessageA(hwnd, msg, wParam, lParam);
    void *pv = CompatMessageAllocIO().pfnAllocate(sizeof(COMPATASYNCPOST));
    if (!pv) return FALSE;
    auto *lp = ::new (pv) COMPATASYNCPOST;
    lp->hwnd = hwnd; lp->msg = msg; lp->wParam = wParam; lp->lParam = lParam;
    return PushAsyncPost(lp);
}

BOOL StarsCompatPostOwnedMessageAsync(uint64_t idWindow, UINT msg, STARSCOMPATPAYLOAD payload, LPARAM lParam) {
    if (SDL_IsMainThread()) return PostOwnedMessage(idWindow, msg, std::move(payload), lParam);
    if (!payload) return FALSE;
    void *pv = CompatMessageAllocIO().pfnAllocate(sizeof(COMPATASYNCPOST));
    if (!pv) return FALSE;
    auto *lp = ::new (pv) COMPATASYNCPOST;
    lp->idWindow = idWindow; lp->msg = msg; lp->lParam = lParam;
    lp->payload = std::move(payload);
    return PushAsyncPost(lp);
}

void CompatConsumeAsyncMessage(const SDL_Event *event) {
    auto post = TakePost(event->user.data1, reinterpret_cast<uintptr_t>(event->user.data2));
    if (!post) return; // Canceled/stale SDL events never dereference their old data.
    auto *lp = post.get();
    if (lp->payload) PostOwnedMessage(lp->idWindow, lp->msg, std::move(lp->payload), lp->lParam);
    else win32::PostMessageA(lp->hwnd, lp->msg, lp->wParam, lp->lParam);
}

BOOL CompatPostInput(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    /* Keep the newest consecutive motion, without merging across button/key
       changes or application-posted messages. Old pointer positions otherwise
       accumulate behind expensive map redraws during captured thumb dragging. */
    if (SDL_IsMainThread() && msg == WM_MOUSEMOVE && lpLast && lpLast->fInput &&
        lpLast->msg.hwnd == hwnd && lpLast->msg.message == msg && lpLast->msg.wParam == w) {
        lpLast->msg.lParam = l;
        lpLast->msg.time = win32::GetTickCount();
        CompatCopyInput(lpLast->rgbKey, &lpLast->msg.pt);
        return TRUE;
    }
    if (!win32::PostMessageA(hwnd, msg, w, l)) return FALSE;
    lpLast->fInput = TRUE;
    CompatCopyInput(lpLast->rgbKey, &lpLast->msg.pt);
    return TRUE;
}

BOOL CompatQueueTimer(HWND hwnd, UINT_PTR id, TIMERPROC lpfn) {
    if (!win32::PostMessageA(hwnd, WM_TIMER, id, (LPARAM)lpfn)) return FALSE;
    lpLast->fTimer = TRUE;
    return TRUE;
}

static void PurgeMessageChain(COMPATMESSAGE *&first, HWND hwnd, bool fAll) noexcept {
    for (auto **link = &first; *link;) {
        auto *lp = *link;
        if (fAll || lp->msg.hwnd == hwnd) {
            *link = lp->lpNext;
            FreeMessage(lp);
        } else link = &lp->lpNext;
    }
}

void CompatPurgeMessages(HWND hwnd) {
    PurgeMessageChain(lpFirst, hwnd, false);
    PurgeMessageChain(lpPending, hwnd, false);
    lpLast = lpFirst;
    if (lpLast) while (lpLast->lpNext) lpLast = lpLast->lpNext;
}

void CompatClearMessages() {
    CompatStopAsyncMessages();
    PurgeMessageChain(lpFirst, nullptr, true);
    PurgeMessageChain(lpPending, nullptr, true);
    lpLast = nullptr;
}

static BOOL FMessageMatches(const win32::MSG *lpmsg, HWND hwnd, UINT msgMin, UINT msgMax) {
    if (lpmsg->message == WM_QUIT)
        return TRUE;
    if ((INT_PTR)hwnd == -1) {
        if (lpmsg->hwnd)
            return FALSE;
    } else if (hwnd && lpmsg->hwnd != hwnd && !win32::IsChild(hwnd, lpmsg->hwnd))
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
            if (lpmsg->payload) {
                lpmsg->lpNext = lpPending;
                lpPending = lpmsg;
            } else FreeMessage(lpmsg);
        }
        return TRUE;
    }
    return FALSE;
}

BOOL WINAPI win32::PeekMessageA(LPMSG lpOut, HWND hwnd, UINT msgMin, UINT msgMax, UINT flags) {
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

BOOL WINAPI win32::GetMessageA(LPMSG lpmsg, HWND hwnd, UINT msgMin, UINT msgMax) {
    SDL_Event event;
    int cMillis;
    BOOL fMessage;

    if (!lpmsg || !FValidFilter(hwnd, msgMin, msgMax))
        return -1;
    for (;;) {
        // Thread timers have no enclosing HWND to abort. Let every modal
        // pump finish its C cleanup before the C++ caller receives the error.
        if (FStarsErrorPending()) return -1;
        fMessage = win32::PeekMessageA(lpmsg, hwnd, msgMin, msgMax, PM_REMOVE);
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

BOOL WINAPI win32::WaitMessage(void) {
    win32::MSG msg;
    SDL_Event event;
    int cMillis;

    if (!SDL_IsMainThread())
        return FALSE;
    while (!win32::PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE)) {
        cMillis = CMillisCompatTimerWait(NULL, 0, 0);
        if (!(SDL_WasInit(SDL_INIT_EVENTS) & SDL_INIT_EVENTS)) return FALSE;
        if (SDL_WaitEventTimeout(&event, cMillis)) CompatRouteEvent(&event);
        else if (cMillis == -1) return FALSE;
    }
    return TRUE;
}

VOID WINAPI win32::PostQuitMessage(int code) {
    win32::PostMessageA(NULL, WM_QUIT, (WPARAM)code, 0);
}

LRESULT WINAPI win32::CallWindowProcA(WNDPROC lpfn, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!SDL_IsMainThread() || !lpfn)
        return 0;
    COMPATDISPATCHSCOPE dispatch;
    return CompatInvokeWindowProc(lpfn, hwnd, msg, wParam, lParam);
}

LRESULT WINAPI win32::SendMessageA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HWND lpwin = LpCompatWindow(hwnd);

    if (!lpwin)
        return 0;
    if (msg == WM_SIZE) CompatLayoutScrollbars(hwnd);
    if (!win32::IsWindow(hwnd)) return 0;
    return win32::CallWindowProcA(lpwin->lpfnWndProc, hwnd, msg, wParam, lParam);
}

LRESULT WINAPI win32::DispatchMessageA(const win32::MSG *lpmsg) {
    if (!lpmsg || !SDL_IsMainThread() || lpmsg->message == WM_QUIT) return 0;
    auto payload = TakePayloadMessage(*lpmsg);
    StarsRestore restorePayload(lpDispatchPayload);
    lpDispatchPayload = payload.get();
    if (lpmsg->message == WM_TIMER && lpmsg->lParam) {
        if (lpmsg->hwnd && !LpCompatWindow(lpmsg->hwnd)) return 0;
        COMPATDISPATCHSCOPE dispatch;
        CompatInvokeTimerProc((TIMERPROC)lpmsg->lParam, lpmsg->hwnd, WM_TIMER, lpmsg->wParam, win32::GetTickCount());
        return 0;
    }
    if (!lpmsg->hwnd) return 0;
    return win32::SendMessageA(lpmsg->hwnd, lpmsg->message, lpmsg->wParam, lpmsg->lParam);
}

BOOL WINAPI win32::TranslateMessage(const win32::MSG *lpmsg) {
    // SDL text events supply printable characters, but omit control characters.
    if (lpmsg && (lpmsg->message == WM_KEYDOWN || lpmsg->message == WM_SYSKEYDOWN) &&
        (lpmsg->wParam == VK_BACK || lpmsg->wParam == VK_TAB ||
         lpmsg->wParam == VK_RETURN || lpmsg->wParam == VK_ESCAPE))
        win32::PostMessageA(lpmsg->hwnd, lpmsg->message == WM_KEYDOWN ? WM_CHAR : WM_SYSCHAR,
                     lpmsg->wParam, lpmsg->lParam);
    return lpmsg && (lpmsg->message == WM_KEYDOWN || lpmsg->message == WM_KEYUP ||
                     lpmsg->message == WM_SYSKEYDOWN || lpmsg->message == WM_SYSKEYUP);
}

LRESULT WINAPI win32::DefWindowProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HWND lpwin = LpCompatWindow(hwnd);
    size_t cch;
    const char *psz;
    char *pszNew;

    if (!lpwin)
        return 0;
    if (FCompatMenuMessage(hwnd, msg, wParam, lParam)) return 0;
    switch (msg) {
    case WM_SETCURSOR:
        if (lpwin->hwndParent && win32::SendMessageA(lpwin->hwndParent, msg, wParam, lParam)) return TRUE;
        if (!LpCompatWindow(hwnd)) return FALSE;
        if (LOWORD(lParam) != HTCLIENT) { win32::SetCursor(win32::LoadCursorA(NULL, MAKEINTRESOURCE(32512))); return TRUE; }
        if (lpwin->lpclass->wc.hCursor) { win32::SetCursor(lpwin->lpclass->wc.hCursor); return TRUE; }
        return FALSE;
    case WM_NCACTIVATE:
        lpwin->fCaptionActive = !!wParam;
        return TRUE;
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
        if (lpwin->hwndParent) return win32::SendMessageA(lpwin->hwndParent, msg, wParam, lParam);
        return 0;
    case WM_CANCELMODE:
        if (win32::GetCapture() == hwnd) win32::ReleaseCapture();
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        if (win32::BeginPaint(hwnd, &ps))
            win32::EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        if (lpwin->lpclass->wc.hbrBackground) {
            RECT rc;
            win32::GetClientRect(hwnd, &rc);
            return win32::FillRect((HDC)wParam, &rc, lpwin->lpclass->wc.hbrBackground);
        }
        return 0;
    case WM_WINDOWPOSCHANGING: {
        WINDOWPOS *lpwp = (WINDOWPOS *)lParam;
        MINMAXINFO mmi = {};
        const SDL_DisplayMode *lpMode;
        if (!lpwp || (lpwp->flags & SWP_NOSIZE) || lpwin->hwndParent ||
            ((lpwin->dwStyle & WS_POPUP) && !(lpwin->dwStyle & WS_THICKFRAME))) return 0;
        lpMode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(lpwin->lpWindow.get()));
        if (lpMode) {
            mmi.ptMaxSize.x = mmi.ptMaxTrackSize.x = lpMode->w;
            mmi.ptMaxSize.y = mmi.ptMaxTrackSize.y = lpMode->h;
        }
        win32::SendMessageA(hwnd, WM_GETMINMAXINFO, 0, (LPARAM)&mmi);
        if (!LpCompatWindow(hwnd)) return 0;
        if (mmi.ptMinTrackSize.x > 0) lpwp->cx = StarsMax(lpwp->cx, mmi.ptMinTrackSize.x);
        if (mmi.ptMinTrackSize.y > 0) lpwp->cy = StarsMax(lpwp->cy, mmi.ptMinTrackSize.y);
        if (mmi.ptMaxTrackSize.x > 0) lpwp->cx = StarsMin(lpwp->cx, mmi.ptMaxTrackSize.x);
        if (mmi.ptMaxTrackSize.y > 0) lpwp->cy = StarsMin(lpwp->cy, mmi.ptMaxTrackSize.y);
        return 0;
    }
    case WM_WINDOWPOSCHANGED: {
        const WINDOWPOS *lpwp = (const WINDOWPOS *)lParam;
        RECT rc;
        POINT pt = {0, 0};
        UINT flags;
        if (!lpwp) return 0;
        flags = lpwp->flags;
        if (!(flags & SWP_NOSIZE) && win32::GetClientRect(hwnd, &rc))
            win32::SendMessageA(hwnd, WM_SIZE, win32::IsIconic(hwnd) ? SIZE_MINIMIZED :
                         win32::IsZoomed(hwnd) ? SIZE_MAXIMIZED : SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
        if (!(flags & SWP_NOMOVE) && LpCompatWindow(hwnd)) {
            if (lpwin->hwndParent) {
                pt.x = lpwin->rcChild.left; pt.y = lpwin->rcChild.top;
            } else win32::ClientToScreen(hwnd, &pt);
            win32::SendMessageA(hwnd, WM_MOVE, 0, MAKELPARAM(pt.x, pt.y));
        }
        return 0;
    }
    case WM_NCCREATE:
        return TRUE;
    case WM_CLOSE:
        win32::DestroyWindow(hwnd);
        return 0;
    case WM_SETTEXT:
        if (!lParam || !FUtf8Valid((const char *)lParam, strlen((const char *)lParam)) ||
            !(pszNew = SDL_strdup((const char *)lParam))) return FALSE;
        if (lpwin->lpWindow.get() && !FCompatNativeTitle(lpwin->lpWindow.get(), pszNew)) {
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
