#ifndef STARS_ERRORS_H
#define STARS_ERRORS_H
#include <stdint.h>
#ifdef __cplusplus
#include <type_traits>
#endif

#ifdef __cplusplus
extern "C" {
#endif
int FStarsRecoveryActive(void);
int FStarsErrorPending(void);
#ifdef __cplusplus
}

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
#endif
