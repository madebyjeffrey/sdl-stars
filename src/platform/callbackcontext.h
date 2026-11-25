#ifndef STARS_CALLBACKCONTEXT_H
#define STARS_CALLBACKCONTEXT_H

// Foreign callback signatures cannot carry the application's explicit owner.
// Windows and queued timers borrow this context for their existing lifetimes.
class APPSESSION;
void *PvCompatCallbackContext() noexcept;
void *PBindCompatCallbackContext(void *lpv) noexcept;
inline APPSESSION &AppCompatCallbackContext() noexcept { return *static_cast<APPSESSION *>(PvCompatCallbackContext()); }

class COMPATCALLBACKCONTEXTSCOPE {
    void *lpvPrevious;
public:
    explicit COMPATCALLBACKCONTEXTSCOPE(void *lpv) noexcept : lpvPrevious(PBindCompatCallbackContext(lpv)) {}
    ~COMPATCALLBACKCONTEXTSCOPE() noexcept { PBindCompatCallbackContext(lpvPrevious); }
    COMPATCALLBACKCONTEXTSCOPE(const COMPATCALLBACKCONTEXTSCOPE &) = delete;
    COMPATCALLBACKCONTEXTSCOPE &operator=(const COMPATCALLBACKCONTEXTSCOPE &) = delete;
};

#endif
