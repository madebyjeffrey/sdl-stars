// Throw through application callbacks; the C dispatcher must finish cleanup.
#include <windows.h>
#include <stars_compat.h>
#include "errors.h"
#include "../../res/resource.h"
#include <cstdio>
#include <new>

#define CHECK(f) do { if (!(f)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static int cDestroyed;
static HWND hwndFailed;
struct CallbackGuard { ~CallbackGuard() { ++cDestroyed; } };

static auto pfnRawDialog = &DialogBoxParamA;
static auto pfnRawDispatch = &DispatchMessageA;
#include "../../src/ui/win_calls.h"

static LRESULT CALLBACK OwnerProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_APP + 40) { CallbackGuard guard; StarsThrowError(starsErrorFile, 777); }
    return DefWindowProc(hwnd, msg, w, l);
}
static INT_PTR CALLBACK ThrowingDialog(HWND hwnd, UINT msg, WPARAM, LPARAM mode) {
    if (msg == WM_INITDIALOG) {
        CallbackGuard guard;
        if (mode == 1) {
            DialogBoxParam(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_PASSWORD), hwnd, ThrowingDialog, 0);
            StarsRethrowPendingError();
        }
        hwndFailed = hwnd;
        if (mode == 2) throw std::bad_alloc();
        StarsThrowError(starsErrorMemory, 123);
    }
    return FALSE;
}
static void CALLBACK ThrowingTimer(HWND, UINT, UINT_PTR, DWORD) {
    CallbackGuard guard;
    StarsThrowError(starsErrorFile, 456);
}

int main() {
    WNDCLASS wc = {};
    wc.lpfnWndProc = OwnerProc;
    wc.lpszClassName = "ExceptionOwner";
    wc.hInstance = GetModuleHandle(nullptr);
    CHECK(RegisterClass(&wc));
    HWND hwndOwner = CreateWindow(wc.lpszClassName, "Owner", WS_OVERLAPPEDWINDOW,
                                  0, 0, 200, 100, nullptr, nullptr, wc.hInstance, nullptr);
    CHECK(hwndOwner);
    for (int i = 0; i < 3; ++i) {
        INT_PTR result = pfnRawDialog(wc.hInstance, MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, ThrowingDialog, i);
        CHECK(result == 0 && IsWindowEnabled(hwndOwner));
        CHECK(!IsWindow(hwndFailed) && FStarsErrorPending());
        try { StarsRethrowPendingError(); CHECK(false); }
        catch (const StarsError &error) { CHECK(error.mdError == starsErrorMemory && error.ids == (i == 2 ? 0 : 123)); }
        CHECK(!FStarsErrorPending());
    }
    CHECK(cDestroyed == 4);
    try {
        DialogBoxParam(wc.hInstance, MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, ThrowingDialog, 0);
        CHECK(false);
    } catch (const StarsError &error) { CHECK(error.mdError == starsErrorMemory && error.ids == 123); }
    CHECK(IsWindowEnabled(hwndOwner) && !FStarsErrorPending() && cDestroyed == 5);
    try { SendMessage(hwndOwner, WM_APP + 40, 0, 0); CHECK(false); }
    catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == 777); }
    CHECK(!FStarsErrorPending() && cDestroyed == 6);
    MSG msg = {};
    msg.hwnd = hwndOwner; msg.message = WM_TIMER;
    msg.lParam = (LPARAM)ThrowingTimer;
    pfnRawDispatch(&msg);
    CHECK(FStarsErrorPending() && cDestroyed == 7);
    try { StarsRethrowPendingError(); CHECK(false); }
    catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == 456); }
    CHECK(!FStarsErrorPending());
    CHECK(DestroyWindow(hwndOwner));
    StarsCompatShutdown();
    std::puts("Callback exceptions: modal cleanup, nested ownership, timer and C++ destruction passed");
    return 0;
}
