#ifndef STARS_BYTEVIEW_H
#define STARS_BYTEVIEW_H

#include "wire.h"
#include <cassert>

// Borrow an arena's bytes only for this operation. The position may denote an
// end boundary; reading a field requires only that field's physical bytes.
// Declared counts/lengths do not become a new admission policy here.
class NATIVEBYTEWINDOW {
protected:
    std::span<std::byte> rgb;
    std::ptrdiff_t ib = 0;
    std::span<std::byte> RgbField(std::ptrdiff_t dib, std::size_t cb) const noexcept {
        std::ptrdiff_t ibField = ib + dib;
        if (ibField < 0 || static_cast<std::size_t>(ibField) > rgb.size() || cb > rgb.size() - static_cast<std::size_t>(ibField)) {
            assert(false && "byte field outside its physical arena");
            return {};
        }
        return rgb.subspan(static_cast<std::size_t>(ibField), cb);
    }
    std::uint8_t GetU8(std::ptrdiff_t dib) const noexcept {
        std::uint8_t ch = 0;
        if (!NATIVEBYTEREADER(RgbField(dib, 1)).FReadU8(ch)) assert(false);
        return ch;
    }
    std::uint16_t GetU16(std::ptrdiff_t dib) const noexcept {
        std::uint16_t w = 0;
        if (!NATIVEBYTEREADER(RgbField(dib, 2)).FReadU16Le(w)) assert(false);
        return w;
    }
    std::int16_t GetI16(std::ptrdiff_t dib) const noexcept {
        std::int16_t w = 0;
        if (!NATIVEBYTEREADER(RgbField(dib, 2)).FReadI16Le(w)) assert(false);
        return w;
    }
    std::int32_t GetI32(std::ptrdiff_t dib) const noexcept {
        std::int32_t value = 0;
        if (!NATIVEBYTEREADER(RgbField(dib, 4)).FReadI32Le(value)) assert(false);
        return value;
    }
    std::int32_t SetI32(std::ptrdiff_t dib, std::int32_t value) noexcept {
        if (!NATIVEBYTEWRITER(RgbField(dib, 4)).FWriteI32Le(value)) assert(false);
        return value;
    }
    std::uint8_t SetU8(std::ptrdiff_t dib, std::uint8_t ch) noexcept {
        if (!NATIVEBYTEWRITER(RgbField(dib, 1)).FWriteU8(ch)) assert(false);
        return ch;
    }
    std::uint16_t SetU16(std::ptrdiff_t dib, std::uint16_t w) noexcept {
        if (!NATIVEBYTEWRITER(RgbField(dib, 2)).FWriteU16Le(w)) assert(false);
        return w;
    }
    std::int16_t SetI16(std::ptrdiff_t dib, std::int16_t w) noexcept {
        if (!NATIVEBYTEWRITER(RgbField(dib, 2)).FWriteI16Le(w)) assert(false);
        return w;
    }
    template<std::ptrdiff_t ibWord, unsigned iBit, unsigned cBits>
    std::uint16_t GetBits() const noexcept {
        static_assert(cBits > 0 && iBit + cBits <= 16);
        constexpr unsigned i = iBit % 8;
        constexpr unsigned cb = (i + cBits + 7) / 8;
        constexpr std::uint16_t mask = static_cast<std::uint16_t>((std::uint32_t{1} << cBits) - 1);
        std::uint16_t w;
        if constexpr (cb == 1) w = GetU8(ibWord + iBit / 8);
        else w = GetU16(ibWord + iBit / 8);
        return static_cast<std::uint16_t>((w >> i) & mask);
    }
    template<std::ptrdiff_t ibWord, unsigned iBit, unsigned cBits>
    std::uint16_t SetBits(std::uint16_t value) noexcept {
        static_assert(cBits > 0 && iBit + cBits <= 16);
        constexpr unsigned i = iBit % 8;
        constexpr unsigned cb = (i + cBits + 7) / 8;
        constexpr std::uint16_t mask = static_cast<std::uint16_t>((std::uint32_t{1} << cBits) - 1);
        std::uint16_t w;
        if constexpr (cb == 1) w = GetU8(ibWord + iBit / 8);
        else w = GetU16(ibWord + iBit / 8);
        w = static_cast<std::uint16_t>((w & ~(std::uint32_t(mask) << i)) | (std::uint32_t(value & mask) << i));
        if constexpr (cb == 1) SetU8(ibWord + iBit / 8, static_cast<std::uint8_t>(w));
        else SetU16(ibWord + iBit / 8, w);
        return value & mask;
    }
    NATIVEBYTEWINDOW Shift(std::ptrdiff_t dib) const noexcept { return {rgb, ib + dib}; }
public:
    NATIVEBYTEWINDOW() = default;
    NATIVEBYTEWINDOW(std::span<std::byte> rgb, std::ptrdiff_t ib = 0) noexcept : rgb(rgb), ib(ib) {
        assert(ib >= 0 && static_cast<std::size_t>(ib) <= rgb.size());
    }
    explicit operator bool() const noexcept { return rgb.data() != nullptr; }
    std::uint8_t *Lpb() const noexcept {
        if (!rgb.data()) return nullptr;
        assert(ib >= 0 && static_cast<std::size_t>(ib) <= rgb.size());
        return reinterpret_cast<std::uint8_t *>(rgb.data() + ib);
    }
    std::span<std::byte> Rgb() const noexcept { return RgbField(0, rgb.size() - static_cast<std::size_t>(ib)); }
};

#endif
