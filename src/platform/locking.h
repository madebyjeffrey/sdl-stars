#ifndef STARS_LOCKING_H
#define STARS_LOCKING_H

#include <atomic>
#include <mutex>

// Keep the wrapper registries' existing acquire/release spin policy, while
// letting standard lock guards release a held lock during C++ unwinding.
class NATIVEFLAGLOCK {
    std::atomic_flag fLocked = ATOMIC_FLAG_INIT;
public:
    void lock() noexcept {
        while (fLocked.test_and_set(std::memory_order_acquire)) {}
    }
    void unlock() noexcept { fLocked.clear(std::memory_order_release); }
};

#endif
