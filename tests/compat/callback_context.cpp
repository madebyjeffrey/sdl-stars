#include <windows.h>
#include <callbackcontext.h>
#include <stars_compat.h>
#include "internal.h"
#include "time_io.h"
#include <stdio.h>

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "callback context line %d: %s\n", __LINE__, #f); return 1; } } while (0)

static int cErrors, cNested, cTimer, cDestroy;
static int ownerA, ownerB, caller;
static void *pvTimerExpected;
static HWND hwndA, hwndB;
static uint64_t msNow;
static constexpr UINT msgNested = win32::WM_APP + 123;
static constexpr UINT msgThrow = win32::WM_APP + 124;

static uint64_t GetTicks() { return msNow; }

static LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == msgNested) {
        if (hwnd == hwndA) {
            if (PvCompatCallbackContext() != &ownerA) ++cErrors;
            win32::SendMessageA(hwndB, msgNested, 0, 0);
            if (PvCompatCallbackContext() != &ownerA) ++cErrors;
        } else {
            if (PvCompatCallbackContext() != &ownerB) ++cErrors;
            ++cNested;
        }
        return 1;
    }
    if (msg == msgThrow) {
        if (PvCompatCallbackContext() != &ownerB) ++cErrors;
        StarsThrowError(starsErrorFile, 919);
    }
    if (msg == win32::WM_NCDESTROY) {
        if (PvCompatCallbackContext() != (hwnd == hwndA ? &ownerA : &ownerB)) ++cErrors;
        ++cDestroy;
    }
    return win32::DefWindowProcA(hwnd, msg, w, l);
}

static void TimerProc(HWND, UINT, UINT_PTR, DWORD) {
    if (PvCompatCallbackContext() != pvTimerExpected) ++cErrors;
    ++cTimer;
}

int main() {
    WNDCLASS wc{};
    win32::MSG msg{}, msgFirst{}, msgSecond{};
    UINT_PTR id;

    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = "StarsCallbackContext";
    CHECK(win32::RegisterClassA(&wc));
    {
        COMPATCALLBACKCONTEXTSCOPE context(&ownerA);
        hwndA = win32::CreateWindowExA(0, wc.lpszClassName, "A", 0, 0, 0, 100, 100, nullptr, nullptr, nullptr, nullptr);
    }
    {
        COMPATCALLBACKCONTEXTSCOPE context(&ownerB);
        hwndB = win32::CreateWindowExA(0, wc.lpszClassName, "B", 0, 0, 0, 100, 100, nullptr, nullptr, nullptr, nullptr);
    }
    CHECK(hwndA && hwndB && !PvCompatCallbackContext());
    {
        COMPATCALLBACKCONTEXTSCOPE context(&caller);
        CHECK(win32::SendMessageA(hwndA, msgNested, 0, 0) == 1);
        CHECK(PvCompatCallbackContext() == &caller && cNested == 1 && cErrors == 0);
        win32::SendMessageA(hwndB, msgThrow, 0, 0);
        CHECK(PvCompatCallbackContext() == &caller && FStarsErrorPending());
        try { StarsRethrowPendingError(); CHECK(false); }
        catch (const StarsError &error) { CHECK(error.mdError == starsErrorFile && error.ids == 919); }
        CHECK(PvCompatCallbackContext() == &caller);
    }
    CHECK(!PvCompatCallbackContext());
    {
        auto effects = CompatTimeIO();
        effects.pfnGetTicks = GetTicks;
        COMPATTIMEIOSCOPE time(effects);
        {
            COMPATCALLBACKCONTEXTSCOPE context(&ownerA);
            id = win32::SetTimer(nullptr, 0, 10, TimerProc);
        }
        CHECK(id != 0);
        msNow = 10;
        CHECK(win32::PeekMessageA(&msg, nullptr, win32::WM_TIMER, win32::WM_TIMER, win32::PM_REMOVE));
        CHECK(win32::KillTimer(nullptr, id));
        COMPATCALLBACKCONTEXTSCOPE context(&caller);
        pvTimerExpected = &ownerA;
        win32::DispatchMessageA(&msg);
        CHECK(cTimer == 1 && cErrors == 0 && PvCompatCallbackContext() == &caller);

        // Dequeue two messages from the same timer with different captured owners.
        {
            COMPATCALLBACKCONTEXTSCOPE first(&ownerA);
            id = win32::SetTimer(nullptr, 0, 10, TimerProc);
        }
        CHECK(id != 0);
        msNow = 20;
        CHECK(win32::PeekMessageA(&msgFirst, nullptr, win32::WM_TIMER, win32::WM_TIMER, win32::PM_REMOVE));
        {
            COMPATCALLBACKCONTEXTSCOPE second(&ownerB);
            CHECK(win32::SetTimer(nullptr, id, 10, TimerProc) == id);
        }
        msNow = 30;
        CHECK(win32::PeekMessageA(&msgSecond, nullptr, win32::WM_TIMER, win32::WM_TIMER, win32::PM_REMOVE));
        CHECK(win32::KillTimer(nullptr, id));
        pvTimerExpected = &ownerA;
        win32::DispatchMessageA(&msgFirst);
        pvTimerExpected = &ownerB;
        win32::DispatchMessageA(&msgSecond);
        CHECK(cTimer == 3 && cErrors == 0 && PvCompatCallbackContext() == &caller);

        // A caller can dispatch dequeued messages in a different order.
        {
            COMPATCALLBACKCONTEXTSCOPE first(&ownerA);
            id = win32::SetTimer(nullptr, 0, 10, TimerProc);
        }
        CHECK(id != 0);
        msNow = 40;
        CHECK(win32::PeekMessageA(&msgFirst, nullptr, win32::WM_TIMER, win32::WM_TIMER, win32::PM_REMOVE));
        {
            COMPATCALLBACKCONTEXTSCOPE second(&ownerB);
            CHECK(win32::SetTimer(nullptr, id, 10, TimerProc) == id);
        }
        msNow = 50;
        CHECK(win32::PeekMessageA(&msgSecond, nullptr, win32::WM_TIMER, win32::WM_TIMER, win32::PM_REMOVE));
        CHECK(win32::KillTimer(nullptr, id));
        pvTimerExpected = &ownerB;
        win32::DispatchMessageA(&msgSecond);
        pvTimerExpected = &ownerA;
        win32::DispatchMessageA(&msgFirst);
        CHECK(cTimer == 5 && cErrors == 0 && PvCompatCallbackContext() == &caller);
    }
    CHECK(!PvCompatCallbackContext());
    CHECK(win32::DestroyWindow(hwndB) && win32::DestroyWindow(hwndA));
    CHECK(cDestroy == 2 && cErrors == 0 && !PvCompatCallbackContext());
    puts("Target-bound owners survive nested dispatch, callback failure, queued timer ownership and destruction");
    return 0;
}
