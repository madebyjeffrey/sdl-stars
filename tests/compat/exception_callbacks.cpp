// Throw through application callbacks; the dispatcher must finish modal cleanup.
#include <windows.h>
#include <stars_compat.h>
#include "errors.h"
#include "../../res/resource.h"
#include <cstdio>
#include <new>

#define CHECK(f) do { if (!(f)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static int cDestroyed;
static int cUnexpected, cModelessInit, mdModelessFailure;
static bool fDisableFailure;
static UINT msgFailure;
static HWND hwndFailed;
static HWND hwndQueueOwner;
struct CallbackGuard { ~CallbackGuard() { ++cDestroyed; } };

static auto pfnRawDialog = &win32::DialogBoxParamA;
static auto pfnRawCreateDialog = &win32::CreateDialogParamA;
static auto pfnRawDispatch = &win32::DispatchMessageA;
#include <wincompat_calls.h>

static LRESULT  OwnerProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == win32::WM_ENABLE && !w && fDisableFailure) StarsThrowError(starsErrorFile, 890);
    if (msgFailure != 0 && msg == msgFailure) { CallbackGuard guard; StarsThrowError(starsErrorFile, 891); }
    if (msg == win32::WM_APP + 40) { CallbackGuard guard; StarsThrowError(starsErrorFile, 777); }
    return ui::DefWindowProc(hwnd, msg, w, l);
}
static INT_PTR  ThrowingDialog(HWND hwnd, UINT msg, WPARAM, LPARAM mode) {
    if (msg == win32::WM_INITDIALOG) {
        CallbackGuard guard;
        if (mode == 1) {
            ui::DialogBox(win32::GetModuleHandleA(nullptr), win32::MAKEINTRESOURCE(IDD_PASSWORD), hwnd, ThrowingDialog, 0);
            StarsRethrowPendingError();
        }
        hwndFailed = hwnd;
        if (mode == 2) throw std::bad_alloc();
        StarsThrowError(starsErrorMemory, 123);
    }
    return 0;
}
static INT_PTR  FailedModeless(HWND hwnd, UINT msg, WPARAM, LPARAM) {
    if (msg == win32::WM_SETFONT && mdModelessFailure == 1) {
        CallbackGuard guard; hwndFailed = hwnd;
        StarsThrowError(starsErrorFile, 889);
    }
    if (msg == win32::WM_INITDIALOG) {
        cModelessInit++;
        if (mdModelessFailure == 1 || mdModelessFailure == 3) return 1;
        CallbackGuard guard; hwndFailed = hwnd;
        if (mdModelessFailure == 2) throw std::bad_alloc();
        StarsThrowError(starsErrorMemory, 123);
    }
    return 0;
}
static void  ThrowingTimer(HWND, UINT, UINT_PTR, DWORD) {
    CallbackGuard guard;
    StarsThrowError(starsErrorFile, 456);
}
static INT_PTR  QueuedFailureDialog(HWND hwnd, UINT msg, WPARAM, LPARAM mode) {
    if (msg == win32::WM_INITDIALOG) {
        hwndFailed = hwnd;
        if (mode == 0) win32::PostMessageA(nullptr, win32::WM_TIMER, 77, (LPARAM)ThrowingTimer);
        else win32::PostMessageA(hwndQueueOwner, win32::WM_APP + 40, 0, 0);
        win32::PostMessageA(hwnd, win32::WM_APP + 41, 0, 0);
        return 1;
    }
    if (msg == win32::WM_APP + 41) {
        ++cUnexpected;
        win32::EndDialog(hwnd, 9);
        return 1;
    }
    return 0;
}

int main() {
    WNDCLASS wc = {};
    wc.lpfnWndProc = OwnerProc;
    wc.lpszClassName = "ExceptionOwner";
    wc.hInstance = win32::GetModuleHandleA(nullptr);
    CHECK(win32::RegisterClassA(&wc));
    HWND hwndOwner = ui::CreateWindowEx(0, wc.lpszClassName, "Owner", win32::WS_OVERLAPPEDWINDOW,
                                  0, 0, 200, 100, nullptr, nullptr, wc.hInstance, nullptr);
    CHECK(hwndOwner);
    for (int i = 0; i < 3; ++i) {
        INT_PTR result = pfnRawDialog(wc.hInstance, win32::MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, ThrowingDialog, i);
        CHECK(result == 0 && win32::IsWindowEnabled(hwndOwner));
        CHECK(!win32::IsWindow(hwndFailed) && FStarsErrorPending());
        try { StarsRethrowPendingError(); CHECK(false); }
        catch (const StarsError &error) { CHECK(error.mdError == starsErrorMemory && error.ids == (i == 2 ? 0 : 123)); }
        CHECK(!FStarsErrorPending());
    }
    CHECK(cDestroyed == 4);
    try {
        ui::DialogBox(wc.hInstance, win32::MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, ThrowingDialog, 0);
        CHECK(false);
    } catch (const StarsError &error) { CHECK(error.mdError == starsErrorMemory && error.ids == 123); }
    CHECK(win32::IsWindowEnabled(hwndOwner) && !FStarsErrorPending() && cDestroyed == 5);
    try { ui::SendMessage(hwndOwner, win32::WM_APP + 40, 0, 0); CHECK(false); }
    catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == 777); }
    CHECK(!FStarsErrorPending() && cDestroyed == 6);
    win32::MSG msg = {};
    msg.hwnd = hwndOwner; msg.message = win32::WM_TIMER;
    msg.lParam = (LPARAM)ThrowingTimer;
    pfnRawDispatch(&msg);
    CHECK(FStarsErrorPending() && cDestroyed == 7);
    try { StarsRethrowPendingError(); CHECK(false); }
    catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == 456); }
    CHECK(!FStarsErrorPending());
    hwndQueueOwner = hwndOwner;
    for (int i = 0; i < 2; ++i) {
        INT_PTR result = pfnRawDialog(wc.hInstance, win32::MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, QueuedFailureDialog, i);
        CHECK(result == -1 && cUnexpected == 0 && win32::IsWindowEnabled(hwndOwner));
        CHECK(!win32::IsWindow(hwndFailed) && FStarsErrorPending());
        try { StarsRethrowPendingError(); CHECK(false); }
        catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == (i == 0 ? 456 : 777)); }
        try {
            ui::DialogBox(wc.hInstance, win32::MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, QueuedFailureDialog, i);
            CHECK(false);
        } catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == (i == 0 ? 456 : 777)); }
        CHECK(!FStarsErrorPending() && !win32::IsWindow(hwndFailed) && cUnexpected == 0 && win32::IsWindowEnabled(hwndOwner));
    }
    CHECK(cDestroyed == 11);
    for (int i = 0; i < 3; i++) {
        mdModelessFailure = i;
        int cInitSaved = cModelessInit;
        hwndFailed = nullptr;
        HWND hwndCreated = pfnRawCreateDialog(wc.hInstance, win32::MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, FailedModeless, 0);
        CHECK(!hwndCreated && !win32::IsWindow(hwndFailed) && FStarsErrorPending() && win32::IsWindowEnabled(hwndOwner));
        CHECK(cModelessInit == cInitSaved + (i == 1 ? 0 : 1));
        try { StarsRethrowPendingError(); CHECK(false); }
        catch (const StarsError &error) {
            CHECK(error.mdError == (i == 1 ? starsErrorFile : starsErrorMemory));
            CHECK(error.ids == (i == 1 ? 889 : i == 2 ? 0 : 123));
        }
        CHECK(!FStarsErrorPending() && cDestroyed == 12 + i);
    }
    mdModelessFailure = 0;
    try { ui::CreateDialog(wc.hInstance, win32::MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, FailedModeless, 0); CHECK(false); }
    catch (const StarsError &error) { CHECK(error.mdError == starsErrorMemory && error.ids == 123); }
    CHECK(!win32::IsWindow(hwndFailed) && !FStarsErrorPending() && win32::IsWindowEnabled(hwndOwner) && cDestroyed == 15);
    mdModelessFailure = 3;
    int cInitSaved = cModelessInit;
    HWND hwndCreated = ui::CreateDialog(wc.hInstance, win32::MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, FailedModeless, 0);
    CHECK(hwndCreated && win32::IsWindow(hwndCreated) && !FStarsErrorPending() && cModelessInit == cInitSaved + 1);
    CHECK(ui::DestroyWindow(hwndCreated));
    fDisableFailure = true;
    cInitSaved = cModelessInit;
    CHECK(pfnRawDialog(wc.hInstance, win32::MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, FailedModeless, 0) == -1);
    CHECK(cModelessInit == cInitSaved && win32::IsWindowEnabled(hwndOwner) && FStarsErrorPending());
    CHECK(pfnRawDialog(wc.hInstance, win32::MAKEINTRESOURCE(IDD_PASSWORD), hwndOwner, FailedModeless, 0) == -1);
    CHECK(cModelessInit == cInitSaved && win32::IsWindowEnabled(hwndOwner));
    try { StarsRethrowPendingError(); CHECK(false); }
    catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == 890); }
    fDisableFailure = false;
    struct CALLFAULT {
        UINT msg;
        BOOL (*pfn)(HWND);
    };
    const CALLFAULT rgFault[] = {
        {win32::WM_SETTEXT, [](HWND hwnd) { return ui::SetWindowText(hwnd, "callback failure"); }},
        {win32::WM_WINDOWPOSCHANGED, [](HWND hwnd) { return ui::MoveWindow(hwnd, 50, 60, 220, 140, 0); }},
        {win32::WM_ENABLE, [](HWND hwnd) { return ui::EnableWindow(hwnd, 0); }},
    };
    for (int i = 0; i < 3; i++) {
        msgFailure = rgFault[i].msg;
        try { rgFault[i].pfn(hwndOwner); CHECK(false); }
        catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == 891); }
        CHECK(!FStarsErrorPending() && win32::IsWindow(hwndOwner) && cDestroyed == 16 + i);
        msgFailure = 0;
        ui::EnableWindow(hwndOwner, 1);
        CHECK(win32::IsWindowEnabled(hwndOwner));
    }
    CHECK(ui::DestroyWindow(hwndOwner));
    StarsCompatShutdown();
    std::puts("Callback exceptions: modal cleanup, nested ownership, timer, checked property calls and C++ destruction passed");
    return 0;
}
