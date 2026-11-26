#ifndef STARS_FILEFAMILY_H
#define STARS_FILEFAMILY_H
#include <memory>
#include <string_view>

struct NATIVEFAMILYSTATE;
// A hosted turn replaces several files and rotates backups. Hold the family
// lock until every write commits or the previous generation is restored.
class NATIVEFAMILY {
    std::unique_ptr<NATIVEFAMILYSTATE> lpState;

public:
    explicit NATIVEFAMILY(std::string_view szBase) noexcept;
    ~NATIVEFAMILY();
    explicit operator bool() const noexcept { return bool(lpState); }
    bool Rollback() noexcept;
    bool Commit() noexcept;
};
// The New Game wizard exclusively reserved this unpublished directory.
bool FDiscardNativeNewGame(std::string_view szDirectory, std::string_view szName) noexcept;
bool FCopyNativeFileDurable(std::string_view szSource, std::string_view szTarget) noexcept;
#endif
