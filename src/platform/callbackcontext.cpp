#include "callbackcontext.h"

static thread_local void *lpvCallbackContext;

void *PvCompatCallbackContext() noexcept { return lpvCallbackContext; }
void *PBindCompatCallbackContext(void *lpv) noexcept {
    void *lpvPrevious = lpvCallbackContext;
    lpvCallbackContext = lpv;
    return lpvPrevious;
}

