#ifndef STARS_WIRE_H
#define STARS_WIRE_H

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

// Borrow bytes only for the lifetime of this operation. Each failed primitive
// leaves its cursor and output unchanged; no native object layout is a codec.
class NATIVEBYTEREADER {
    std::span<const std::byte> rgb;
    std::size_t ib = 0;
public:
    explicit NATIVEBYTEREADER(std::span<const std::byte> rgb) noexcept : rgb(rgb) {}
    std::size_t CbRead() const noexcept { return ib; }
    std::size_t CbRemaining() const noexcept { return rgb.size() - ib; }
    bool FReadU8(std::uint8_t &ch) noexcept {
        if (CbRemaining() < 1) return false;
        ch = std::to_integer<std::uint8_t>(rgb[ib++]);
        return true;
    }
    bool FReadU16Le(std::uint16_t &w) noexcept {
        if (CbRemaining() < 2) return false;
        w = static_cast<std::uint16_t>(std::to_integer<std::uint16_t>(rgb[ib]) |
                                      (std::to_integer<std::uint16_t>(rgb[ib + 1]) << 8));
        ib += 2;
        return true;
    }
    bool FReadU32Le(std::uint32_t &dw) noexcept {
        std::uint32_t dwRead = 0;
        if (CbRemaining() < 4) return false;
        for (unsigned i = 0; i < 4; i++) dwRead |= std::to_integer<std::uint32_t>(rgb[ib + i]) << (8 * i);
        dw = dwRead;
        ib += 4;
        return true;
    }
    bool FReadI16Le(std::int16_t &w) noexcept {
        std::uint16_t u;
        if (!FReadU16Le(u)) return false;
        w = std::bit_cast<std::int16_t>(u);
        return true;
    }
    bool FReadI32Le(std::int32_t &dw) noexcept {
        std::uint32_t u;
        if (!FReadU32Le(u)) return false;
        dw = std::bit_cast<std::int32_t>(u);
        return true;
    }
    bool FReadBytes(std::span<std::byte> rgbOut) noexcept {
        if (rgbOut.size() > CbRemaining()) return false;
        if (!rgbOut.empty()) std::memmove(rgbOut.data(), rgb.data() + ib, rgbOut.size());
        ib += rgbOut.size();
        return true;
    }
    bool FSkip(std::size_t cb) noexcept {
        if (cb > CbRemaining()) return false;
        ib += cb;
        return true;
    }
};

class NATIVEBYTEWRITER {
    std::span<std::byte> rgb;
    std::size_t ib = 0;
public:
    explicit NATIVEBYTEWRITER(std::span<std::byte> rgb) noexcept : rgb(rgb) {}
    std::size_t CbWritten() const noexcept { return ib; }
    std::size_t CbRemaining() const noexcept { return rgb.size() - ib; }
    bool FWriteU8(std::uint8_t ch) noexcept {
        if (CbRemaining() < 1) return false;
        rgb[ib++] = static_cast<std::byte>(ch);
        return true;
    }
    bool FWriteU16Le(std::uint16_t w) noexcept {
        if (CbRemaining() < 2) return false;
        for (unsigned i = 0; i < 2; i++) rgb[ib + i] = static_cast<std::byte>((w >> (8 * i)) & 0xff);
        ib += 2;
        return true;
    }
    bool FWriteU32Le(std::uint32_t dw) noexcept {
        if (CbRemaining() < 4) return false;
        for (unsigned i = 0; i < 4; i++) rgb[ib + i] = static_cast<std::byte>((dw >> (8 * i)) & 0xff);
        ib += 4;
        return true;
    }
    bool FWriteI16Le(std::int16_t w) noexcept { return FWriteU16Le(std::bit_cast<std::uint16_t>(w)); }
    bool FWriteI32Le(std::int32_t dw) noexcept { return FWriteU32Le(std::bit_cast<std::uint32_t>(dw)); }
    bool FWriteBytes(std::span<const std::byte> rgbIn) noexcept {
        if (rgbIn.size() > CbRemaining()) return false;
        if (!rgbIn.empty()) std::memmove(rgb.data() + ib, rgbIn.data(), rgbIn.size());
        ib += rgbIn.size();
        return true;
    }
};

#endif
