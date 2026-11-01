#ifndef STARS_NATIVE_FILESESSION_H
#define STARS_NATIVE_FILESESSION_H
#include "fileio.h"
#include <array>
#include <memory>
#include <mutex>

struct NATIVEFILEDELETE {
    void operator()(NATIVEFILE *pf) const noexcept;
};
class NATIVEFILESESSION {
    std::mutex lock;
    std::array<std::unique_ptr<NATIVEFILE, NATIVEFILEDELETE>, INT16_MAX + 1> rg;
    friend NATIVEFILE *LegacyFile(std::int16_t hf);
    friend std::int16_t HfNativeOpenLegacy(const char *, std::uint16_t, std::int16_t *);
    friend std::int32_t CbNativeReadLegacy(std::int16_t, void *, std::uint16_t);
    friend std::int32_t CbNativeWriteLegacy(std::int16_t, const void *, std::uint16_t);
    friend std::int32_t LNativeSeekLegacy(std::int16_t, std::int32_t, std::int16_t);
    friend std::int32_t CbNativeSizeLegacy(std::int16_t);
    friend void CloseNativeLegacy(std::int16_t);
public:
    NATIVEFILESESSION() noexcept = default;
    ~NATIVEFILESESSION() noexcept;
    NATIVEFILESESSION(const NATIVEFILESESSION &) = delete;
    NATIVEFILESESSION &operator=(const NATIVEFILESESSION &) = delete;
    void Close() noexcept;
};
// Bind only before admitting file work; stop all borrowers before restoring.
NATIVEFILESESSION *PBindNativeFileSession(NATIVEFILESESSION *session) noexcept;
#endif
