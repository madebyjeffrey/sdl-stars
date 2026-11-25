#ifndef STARS_COMPAT_RESOURCECODEC_H
#define STARS_COMPAT_RESOURCECODEC_H

#include <windows.h>
#include "wire.h"

inline constexpr std::size_t cbWireDibHeader = 40;
inline constexpr std::size_t cbWireRgbQuad = 4;

inline constexpr std::size_t cbWireDibCoreHeader = 12;
inline bool FDecodeDibCoreHeader(std::span<const std::byte> rgb, BITMAPCOREHEADER &bc) noexcept {
    if (rgb.size() < cbWireDibCoreHeader) return false;
    NATIVEBYTEREADER input(rgb);
    BITMAPCOREHEADER value{};
    std::uint32_t ul;
    std::uint16_t w;
    input.FReadU32Le(ul); value.bcSize = ul;
    input.FReadU16Le(w); value.bcWidth = w;
    input.FReadU16Le(w); value.bcHeight = w;
    input.FReadU16Le(w); value.bcPlanes = w;
    input.FReadU16Le(w); value.bcBitCount = w;
    bc = value;
    return true;
}

// SDK header objects are adapter values. Embedded resource bytes have their
// own fixed little-endian layout, independent of native alignment/endian order.
inline bool FDecodeDibHeader(std::span<const std::byte> rgb, BITMAPINFOHEADER &bi) noexcept {
    if (rgb.size() < cbWireDibHeader) return false;
    NATIVEBYTEREADER input(rgb);
    BITMAPINFOHEADER value{};
    std::uint32_t ul;
    std::int32_t l;
    std::uint16_t w;
    input.FReadU32Le(ul); value.biSize = ul;
    input.FReadI32Le(l); value.biWidth = l;
    input.FReadI32Le(l); value.biHeight = l;
    input.FReadU16Le(w); value.biPlanes = w;
    input.FReadU16Le(w); value.biBitCount = w;
    input.FReadU32Le(ul); value.biCompression = ul;
    input.FReadU32Le(ul); value.biSizeImage = ul;
    input.FReadI32Le(l); value.biXPelsPerMeter = l;
    input.FReadI32Le(l); value.biYPelsPerMeter = l;
    input.FReadU32Le(ul); value.biClrUsed = ul;
    input.FReadU32Le(ul); value.biClrImportant = ul;
    bi = value;
    return true;
}
inline bool FEncodeDibHeader(BITMAPINFOHEADER bi, std::span<std::byte> rgb) noexcept {
    if (rgb.size() < cbWireDibHeader) return false;
    NATIVEBYTEWRITER output(rgb);
    return output.FWriteU32Le(bi.biSize) && output.FWriteI32Le(bi.biWidth) && output.FWriteI32Le(bi.biHeight) &&
        output.FWriteU16Le(bi.biPlanes) && output.FWriteU16Le(bi.biBitCount) && output.FWriteU32Le(bi.biCompression) &&
        output.FWriteU32Le(bi.biSizeImage) && output.FWriteI32Le(bi.biXPelsPerMeter) && output.FWriteI32Le(bi.biYPelsPerMeter) &&
        output.FWriteU32Le(bi.biClrUsed) && output.FWriteU32Le(bi.biClrImportant);
}

#endif
