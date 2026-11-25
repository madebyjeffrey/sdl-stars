#ifndef STARS_COMPAT_MESSAGEPAYLOAD_H
#define STARS_COMPAT_MESSAGEPAYLOAD_H

#include <memory>

// Release routines must be thread-safe and must not invoke application callbacks.
// Rejection/cancellation can release the payload on the posting thread.
struct STARSCOMPATPAYLOADDELETE {
    void (*pfnFree)(void *) noexcept = nullptr;
    void operator()(void *pv) const noexcept { if (pv && pfnFree) pfnFree(pv); }
};
using STARSCOMPATPAYLOAD = std::unique_ptr<void, STARSCOMPATPAYLOADDELETE>;

#endif
