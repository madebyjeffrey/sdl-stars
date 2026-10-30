#ifndef STARS_STREAMVIEWS_H
#define STARS_STREAMVIEWS_H

#include "streamcodec.h"
#include "byteview.h"

class NATIVEHEADERVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    std::uint16_t cb() const noexcept { return GetBits<0, 0, 10>(); }
    std::uint16_t rt() const noexcept { return GetBits<0, 10, 6>(); }
};
inline NATIVEHEADERVIEW HdvFromPrefix(std::uint8_t *lp) noexcept {
    return {std::as_writable_bytes(std::span(lp, cbWireHdr)), 0};
}

class NATIVEBOFVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    std::int32_t lidGame() const noexcept { return GetI32(4); }
    std::uint16_t wVersion() const noexcept { return GetU16(8); }
    std::uint16_t verInc() const noexcept { return GetBits<8, 0, 5>(); }
    std::uint16_t verMinor() const noexcept { return GetBits<8, 5, 7>(); }
    std::uint16_t verMajor() const noexcept { return GetBits<8, 12, 4>(); }
    std::uint16_t turn() const noexcept { return GetU16(10); }
    std::int16_t iPlayer() const noexcept { return IBofSignedBits(GetBits<12, 0, 5>(), 5); }
    std::int16_t lSaltTime() const noexcept { return IBofSignedBits(GetBits<12, 5, 11>(), 11); }
    std::uint16_t dt() const noexcept { return GetBits<14, 0, 8>(); }
    std::uint16_t fDone() const noexcept { return GetBits<14, 8, 1>(); }
    std::uint16_t fInUse() const noexcept { return GetBits<14, 9, 1>(); }
    std::uint16_t fMulti() const noexcept { return GetBits<14, 10, 1>(); }
    std::uint16_t fGameOverMan() const noexcept { return GetBits<14, 11, 1>(); }
    std::uint16_t fCrippled() const noexcept { return GetBits<14, 12, 1>(); }
    std::uint16_t wGen() const noexcept { return GetBits<14, 13, 3>(); }
};
inline NATIVEBOFVIEW BfvFromPrefix(void *lp, std::size_t cb) noexcept {
    return {std::span(static_cast<std::byte *>(lp), cb), 0};
}

#endif
