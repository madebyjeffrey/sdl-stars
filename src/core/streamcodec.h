#ifndef STARS_STREAMCODEC_H
#define STARS_STREAMCODEC_H

#include "structs.h"
#include "wire.h"

inline constexpr std::size_t cbWireHdr = 2;

inline bool FDecodeHdr(std::span<const std::byte> rgb, HDR &hdr) noexcept {
    std::uint16_t w;
    if (!NATIVEBYTEREADER(rgb).FReadU16Le(w)) return false;
    hdr.cb = w & 1023; hdr.rt = w >> 10;
    return true;
}
inline bool FEncodeHdr(HDR hdr, std::span<std::byte> rgb) noexcept {
    return NATIVEBYTEWRITER(rgb).FWriteU16Le(static_cast<std::uint16_t>((hdr.cb & 1023) | ((hdr.rt & 63) << 10)));
}

#endif
