// Contain exceptions before returning through foreign SDL/native callbacks.
#include "internal.h"
#include <callbackcontext.h>
#include <new>

void CompatCaptureCallbackError(HWND hwnd) noexcept {
    StarsCapturePendingError();
    CompatAbortCallback(hwnd);
}

LRESULT CompatInvokeWindowProc(WNDPROC lpfn, HWND hwnd, UINT msg, WPARAM w, LPARAM l) noexcept {
    HWND lpwin = LpCompatWindow(hwnd);
    COMPATCALLBACKCONTEXTSCOPE context(lpwin ? lpwin->lpvCallbackContext : PvCompatCallbackContext());
    try {
        return lpfn(hwnd, msg, w, l);
    } catch (const std::bad_alloc &) {
        try { StarsThrowError(starsErrorMemory, 0); }
        catch (...) { CompatCaptureCallbackError(hwnd); }
    } catch (...) {
        CompatCaptureCallbackError(hwnd);
    }
    return 0;
}

void CompatInvokeTimerProc(TIMERPROC lpfn, HWND hwnd, UINT msg, UINT_PTR id, DWORD tick) noexcept {
    HWND lpwin = LpCompatWindow(hwnd);
    COMPATCALLBACKCONTEXTSCOPE context(lpwin ? lpwin->lpvCallbackContext : PvCompatCallbackContext());
    try {
        lpfn(hwnd, msg, id, tick);
    } catch (const std::bad_alloc &) {
        try { StarsThrowError(starsErrorMemory, 0); }
        catch (...) { CompatCaptureCallbackError(hwnd); }
    } catch (...) {
        CompatCaptureCallbackError(hwnd);
    }
}

HWND CompatInvokeInspector(HWND (*pfn)(void), HWND hwnd) noexcept {
    HWND lpwin = LpCompatWindow(hwnd);
    COMPATCALLBACKCONTEXTSCOPE context(lpwin ? lpwin->lpvCallbackContext : PvCompatCallbackContext());
    try { return pfn(); }
    catch (...) { CompatCaptureCallbackError(hwnd); }
    return nullptr;
}
