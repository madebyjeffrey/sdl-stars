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
    friend NATIVEFILE *LegacyFile(int16_t hf);
    friend int16_t HfNativeOpenLegacy(const char *, uint16_t, int16_t *);
    friend int32_t CbNativeReadLegacy(int16_t, void *, uint16_t);
    friend int32_t CbNativeWriteLegacy(int16_t, const void *, uint16_t);
    friend int32_t LNativeSeekLegacy(int16_t, int32_t, int16_t);
    friend int32_t CbNativeSizeLegacy(int16_t);
    friend void CloseNativeLegacy(int16_t);
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
