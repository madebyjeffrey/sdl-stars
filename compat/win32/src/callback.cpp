// Contain exceptions before returning through the C SDL/Win32 runtime.
#include <windows.h>
#include "errors.h"
#include <new>

extern "C" void CompatAbortCallback(HWND hwnd);

static void CaptureCallbackError(HWND hwnd) noexcept {
    StarsCapturePendingError();
    CompatAbortCallback(hwnd);
}

extern "C" LRESULT CompatInvokeWindowProc(WNDPROC lpfn, HWND hwnd, UINT msg, WPARAM w, LPARAM l) noexcept {
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

extern "C" void CompatInvokeTimerProc(TIMERPROC lpfn, HWND hwnd, UINT msg, UINT_PTR id, DWORD tick) noexcept {
    try {
        lpfn(hwnd, msg, id, tick);
    } catch (const std::bad_alloc &) {
        try { StarsThrowError(starsErrorMemory, 0); }
        catch (...) { CaptureCallbackError(hwnd); }
    } catch (...) {
        CaptureCallbackError(hwnd);
    }
}

extern "C" HWND CompatInvokeInspector(HWND (*pfn)(void), HWND hwnd) noexcept {
    try { return pfn(); }
    catch (...) { CaptureCallbackError(hwnd); }
    return nullptr;
}
