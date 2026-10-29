// Contain exceptions before returning through foreign SDL/native callbacks.
#include <windows.h>
#include "errors.h"
#include <new>

void CompatAbortCallback(HWND hwnd);

static void CaptureCallbackError(HWND hwnd) noexcept {
    StarsCapturePendingError();
    CompatAbortCallback(hwnd);
}

LRESULT CompatInvokeWindowProc(WNDPROC lpfn, HWND hwnd, UINT msg, WPARAM w, LPARAM l) noexcept {
    try {
        return lpfn(hwnd, msg, w, l);
    } catch (const std::bad_alloc &) {
        try { StarsThrowError(starsErrorMemory, 0); }
        catch (...) { CaptureCallbackError(hwnd); }
    } catch (...) {
        CaptureCallbackError(hwnd);
    }
    return 0;
}

void CompatInvokeTimerProc(TIMERPROC lpfn, HWND hwnd, UINT msg, UINT_PTR id, DWORD tick) noexcept {
    try {
        lpfn(hwnd, msg, id, tick);
    } catch (const std::bad_alloc &) {
        try { StarsThrowError(starsErrorMemory, 0); }
        catch (...) { CaptureCallbackError(hwnd); }
    } catch (...) {
        CaptureCallbackError(hwnd);
    }
}

HWND CompatInvokeInspector(HWND (*pfn)(void), HWND hwnd) noexcept {
    try { return pfn(); }
    catch (...) { CaptureCallbackError(hwnd); }
    return nullptr;
}
