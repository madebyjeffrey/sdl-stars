#ifndef STARS_LOGVIEWS_H
#define STARS_LOGVIEWS_H

#include "logcodec.h"
#include "byteview.h"

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

#endif
