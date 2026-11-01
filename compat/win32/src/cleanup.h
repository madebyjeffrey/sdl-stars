#ifndef STARS_COMPAT_CLEANUP_H
#define STARS_COMPAT_CLEANUP_H

#include "internal.h"
#include <type_traits>
#include <utility>

// Unlike a resource deleter, modal cleanup may notify application code. Keep
// its failure contained, preserving an earlier pending or unwinding exception.
template<class F> class COMPATCLEANUPSCOPE {
    F fnCleanup;
    HWND hwndAbort;
    bool fActive = true;
public:
    explicit COMPATCLEANUPSCOPE(F fn, HWND hwnd = nullptr) noexcept(std::is_nothrow_move_constructible_v<F>)
        : fnCleanup(std::move(fn)), hwndAbort(hwnd) {}
    ~COMPATCLEANUPSCOPE() noexcept { Close(); }
    COMPATCLEANUPSCOPE(const COMPATCLEANUPSCOPE &) = delete;
    COMPATCLEANUPSCOPE &operator=(const COMPATCLEANUPSCOPE &) = delete;
    void Close() noexcept {
        if (!std::exchange(fActive, false)) return;
        CompatInvokeCleanup([](void *lpv) { (*static_cast<F *>(lpv))(); }, &fnCleanup, hwndAbort);
    }
    void Release() noexcept { fActive = false; }
};

// Window tokens survive logical destruction until runtime shutdown. Balance
// only this scope's increment, preserving nested presentation batches.
class COMPATPRESENTSCOPE {
    HWND hwnd;
    bool fActive = true;
public:
    explicit COMPATPRESENTSCOPE(HWND hwnd) noexcept : hwnd(hwnd) { hwnd->cPresentDefer++; }
    ~COMPATPRESENTSCOPE() noexcept { Close(); }
    COMPATPRESENTSCOPE(const COMPATPRESENTSCOPE &) = delete;
    COMPATPRESENTSCOPE &operator=(const COMPATPRESENTSCOPE &) = delete;
    void Close() noexcept { if (std::exchange(fActive, false)) hwnd->cPresentDefer--; }
};

#endif
