#ifndef STARS_FILE_GUARD_H
#define STARS_FILE_GUARD_H
#include "native.h"

// A borrowed handle slot can transfer ownership to the caller on success.
// Destruction only releases resources; checked publication remains explicit.
class StarsFileGuard {
    int16_t &hf;
    bool fClose = true;
public:
    explicit StarsFileGuard(int16_t &hf) noexcept : hf(hf) {}
    ~StarsFileGuard() noexcept { if (fClose && hf != -1) { CloseFile(hf); hf = -1; } }
    void Release() noexcept { fClose = false; }
    StarsFileGuard(const StarsFileGuard &) = delete;
    StarsFileGuard &operator=(const StarsFileGuard &) = delete;
};
#endif
