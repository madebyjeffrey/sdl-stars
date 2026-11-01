#ifndef STARS_OWNERSHIP_H
#define STARS_OWNERSHIP_H

#include <cstdlib>
#include <memory>
#include <type_traits>
#include <utility>

// Only non-callback release routines belong here. A borrowed or selected object
// never enters an owner; release() marks transfer into its persistent owner.
template<class T, auto pfnFree> struct NATIVEDELETER {
    void operator()(T *lp) const noexcept {
        if (lp) pfnFree(lp);
    }
};
template<class T, auto pfnFree>
using NATIVEOWNER = std::unique_ptr<T, NATIVEDELETER<T, pfnFree>>;

template<class T, auto pfnFree>
using NATIVEARRAYOWNER = std::unique_ptr<T[], NATIVEDELETER<T, pfnFree>>;

// Some native APIs replace a caller's buffer slot. Scope the release of that
// slot without owning a stale pre-reallocation pointer. No application callback
// or fallible publication belongs in this non-throwing resource cleanup.
template<class F> class NATIVECLEANUPSCOPE {
    static_assert(std::is_nothrow_invocable_v<F &>);
    F fnCleanup;
    bool fActive = true;
public:
    explicit NATIVECLEANUPSCOPE(F fn) noexcept(std::is_nothrow_move_constructible_v<F>)
        : fnCleanup(std::move(fn)) {}
    ~NATIVECLEANUPSCOPE() noexcept { Close(); }
    NATIVECLEANUPSCOPE(const NATIVECLEANUPSCOPE &) = delete;
    NATIVECLEANUPSCOPE &operator=(const NATIVECLEANUPSCOPE &) = delete;
    void Close() noexcept { if (std::exchange(fActive, false)) fnCleanup(); }
};

#endif
