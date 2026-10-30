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

#endif
