#ifndef STARS_LOGVIEWS_H
#define STARS_LOGVIEWS_H

#include "logcodec.h"
#include "byteview.h"

class APPSESSION;
NATIVEBYTEWINDOW LbwFromLp(APPSESSION &app, void *lp) noexcept;

class NATIVERESEARCHVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    std::int8_t pctResearch() const noexcept { return std::bit_cast<std::int8_t>(GetU8(0)); }
    std::int8_t iTechCur() const noexcept { return std::bit_cast<std::int8_t>(GetU8(1)); }
    std::uint8_t iTechNow() const noexcept { return static_cast<std::uint8_t>(GetBits<0, 8, 4>()); }
    std::uint8_t iTechNext() const noexcept { return static_cast<std::uint8_t>(GetBits<0, 12, 4>()); }
};
inline NATIVERESEARCHVIEW RsvFromPrefix(std::uint8_t *lp, std::size_t cb) noexcept {
    return {std::as_writable_bytes(std::span(lp, cb)), 0};
}

// Each caller supplies only the physical prefix needed for that field. Lookup
// failures retain their old no-read paths; logical cb is not new validation.
class NATIVECARGOXFERVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    std::uint16_t id1() const noexcept { return GetU16(0); }
    std::uint16_t id2() const noexcept { return GetU16(2); }
    std::uint8_t grobj1() const noexcept { return static_cast<std::uint8_t>(GetBits<4, 0, 4>()); }
    std::uint8_t grobj2() const noexcept { return static_cast<std::uint8_t>(GetBits<4, 4, 4>()); }
    std::uint8_t grbitItems() const noexcept { return GetU8(5); }
    std::int8_t cQuan8(std::int16_t i) const noexcept { return std::bit_cast<std::int8_t>(GetU8(6 + std::ptrdiff_t(i))); }
    std::int16_t cQuan16(std::int16_t i) const noexcept { return GetI16(6 + std::ptrdiff_t(i) * 2); }
    std::int32_t cQuan32(std::int16_t i) const noexcept { return GetI32(6 + std::ptrdiff_t(i) * 4); }
    void SetCQuan8(std::int16_t i, std::int8_t value) noexcept { SetU8(6 + std::ptrdiff_t(i), std::bit_cast<std::uint8_t>(value)); }
    void SetCQuan16(std::int16_t i, std::int16_t value) noexcept { SetI16(6 + std::ptrdiff_t(i) * 2, value); }
    void SetCQuan32(std::int16_t i, std::int32_t value) noexcept { SetI32(6 + std::ptrdiff_t(i) * 4, value); }
};
inline NATIVECARGOXFERVIEW CxvFromPrefix(std::uint8_t *lp, std::size_t cb) noexcept {
    return {std::as_writable_bytes(std::span(lp, cb)), 0};
}

class NATIVEPLANETROUTINGVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    std::int16_t id() const noexcept { return GetI16(0); }
    std::uint16_t fNoResearch() const noexcept { return static_cast<std::uint16_t>(GetBits32<2, 0, 1>()); }
    std::uint16_t idFling() const noexcept { return static_cast<std::uint16_t>(GetBits32<2, 1, 10>()); }
    std::uint16_t iWarpFling() const noexcept { return static_cast<std::uint16_t>(GetBits32<2, 11, 4>()); }
    std::uint16_t idRoute() const noexcept { return static_cast<std::uint16_t>(GetBits32<2, 15, 10>()); }
};
inline NATIVEPLANETROUTINGVIEW RlvFromPrefix(void *lp, std::size_t cb) noexcept {
    return {std::span(static_cast<std::byte *>(lp), cb), 0};
}
class NATIVELOGTHINGVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    std::uint16_t idFull() const noexcept { return GetU16(0); }
    std::int16_t fDetonate() const noexcept { return GetI16(2); }
};
inline NATIVELOGTHINGVIEW LtvFromPrefix(void *lp, std::size_t cb) noexcept {
    return {std::span(static_cast<std::byte *>(lp), cb), 0};
}

#endif
