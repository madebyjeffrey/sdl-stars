#ifndef STARS_ERRORS_H
#define STARS_ERRORS_H
#include <stdint.h>
#include <type_traits>
int FStarsRecoveryActive(void);
int FStarsErrorPending(void);


enum StarsErrorKind { starsErrorMemory, starsErrorFile };
struct StarsError {
    StarsErrorKind mdError;
    uint16_t ids;
};

// A scope records whether validation has an enclosing recovery operation;
// it never stores a jump target or bypasses C++ destruction.
class StarsRecoveryScope {
public:
    StarsRecoveryScope() noexcept;
    ~StarsRecoveryScope() noexcept;
    StarsRecoveryScope(const StarsRecoveryScope &) = delete;
    StarsRecoveryScope &operator=(const StarsRecoveryScope &) = delete;
};

// A caller already reporting a primary failure must not capture a secondary
// callback error during cleanup, even after C++ unwinding has reached a handler.
class StarsPrimaryErrorScope {
    bool fPrimary;
public:
    explicit StarsPrimaryErrorScope(bool fPrimary = true) noexcept;
    ~StarsPrimaryErrorScope() noexcept;
    StarsPrimaryErrorScope(const StarsPrimaryErrorScope &) = delete;
    StarsPrimaryErrorScope &operator=(const StarsPrimaryErrorScope &) = delete;
};

[[noreturn]] void StarsThrowError(StarsErrorKind mdError, uint16_t ids);
void StarsCapturePendingError() noexcept;
void StarsRethrowPendingError();

template <class T> class StarsRestore {
    T &v;
    T vSaved;
public:
    explicit StarsRestore(T &v) noexcept : v(v), vSaved(v) {}
    ~StarsRestore() noexcept { v = vSaved; }
    StarsRestore(const StarsRestore &) = delete;
    StarsRestore &operator=(const StarsRestore &) = delete;
};
#endif
