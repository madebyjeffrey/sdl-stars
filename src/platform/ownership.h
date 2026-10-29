#ifndef STARS_OWNERSHIP_H
#define STARS_OWNERSHIP_H

#include <cstdlib>
#include <memory>

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

#endif
