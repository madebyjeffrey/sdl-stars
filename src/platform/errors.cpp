#include "errors.h"
#include <exception>
#include <utility>

static thread_local unsigned cRecovery;
static thread_local std::exception_ptr errorPending;

int FStarsRecoveryActive(void) { return cRecovery != 0; }
int FStarsErrorPending(void) { return errorPending != nullptr; }
StarsRecoveryScope::StarsRecoveryScope() noexcept { ++cRecovery; }
StarsRecoveryScope::~StarsRecoveryScope() noexcept { --cRecovery; }

void StarsThrowError(StarsErrorKind mdError, uint16_t ids) { throw StarsError{mdError, ids}; }
void StarsCapturePendingError() noexcept {
    if (!errorPending) errorPending = std::current_exception();
}
void StarsRethrowPendingError() {
    if (errorPending) std::rethrow_exception(std::exchange(errorPending, nullptr));
}
