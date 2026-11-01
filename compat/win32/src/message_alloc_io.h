#ifndef STARS_COMPATMESSAGEALLOCIO_H
#define STARS_COMPATMESSAGEALLOCIO_H
#include <cstdlib>

// Effects belong to this module. Defaults delegate to the original functions;
// scoped bindings own a snapshot and restore the enclosing thread's binding.
// CRT noexcept/attribute annotations vary by platform; effects keep one signature.
struct COMPATMESSAGEALLOCIO {
    void *(*pfnAllocate)(std::size_t) = std::malloc;
    void *(*pfnAllocateZero)(std::size_t, std::size_t) = std::calloc;
};
const COMPATMESSAGEALLOCIO &CompatMessageAllocIO() noexcept;
class COMPATMESSAGEALLOCIOSCOPE {
    COMPATMESSAGEALLOCIO        io;
    const COMPATMESSAGEALLOCIO *lpPrev;

  public:
    explicit COMPATMESSAGEALLOCIOSCOPE(const COMPATMESSAGEALLOCIO &io) noexcept;
    ~COMPATMESSAGEALLOCIOSCOPE() noexcept;
    COMPATMESSAGEALLOCIOSCOPE(const COMPATMESSAGEALLOCIOSCOPE &) = delete;
    COMPATMESSAGEALLOCIOSCOPE &operator=(const COMPATMESSAGEALLOCIOSCOPE &) = delete;
};
#endif
