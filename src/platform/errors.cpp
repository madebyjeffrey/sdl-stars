#include "errors.h"
#include <exception>
#include <utility>

static thread_local unsigned cRecovery;
static thread_local unsigned cPrimaryErrors;
static thread_local std::exception_ptr errorPending;

int FStarsRecoveryActive(void) { return cRecovery != 0; }
int FStarsErrorPending(void) { return errorPending != nullptr; }
StarsRecoveryScope::StarsRecoveryScope() noexcept { ++cRecovery; }
StarsRecoveryScope::~StarsRecoveryScope() noexcept { --cRecovery; }

StarsPrimaryErrorScope::StarsPrimaryErrorScope(bool fPrimary) noexcept : fPrimary(fPrimary) {
    if (fPrimary) ++cPrimaryErrors;
}
StarsPrimaryErrorScope::~StarsPrimaryErrorScope() noexcept { if (fPrimary) --cPrimaryErrors; }

void StarsThrowError(StarsErrorKind mdError, uint16_t ids) { throw StarsError{mdError, ids}; }
void StarsCapturePendingError() noexcept {
    // An in-flight C++ exception is already the primary failure. A callback
    // caught from its cleanup must not leave a secondary error for later delivery.
    if (!errorPending && cPrimaryErrors == 0 && std::uncaught_exceptions() == 0) errorPending = std::current_exception();
}
void StarsRethrowPendingError() {
    if (errorPending) std::rethrow_exception(std::exchange(errorPending, nullptr));
}
