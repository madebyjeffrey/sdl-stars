#ifndef STARS_MESSAGEVIEWS_H
#define STARS_MESSAGEVIEWS_H

#include "messagecodec.h"
#include "byteview.h"

// A file header is twelve numeric bytes, independent of the native list link.
// Readers keep their original scratch-buffer and incremental publication rules.
class NATIVEPLAYERMESSAGEVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    std::int16_t iPlrFrom() const noexcept { return GetI16(4); }
    std::int16_t iPlrTo() const noexcept { return GetI16(6); }
    std::int16_t iInRe() const noexcept { return GetI16(8); }
    std::int16_t cLen() const noexcept { return GetI16(10); }
    std::uint8_t *RgbMsg() const noexcept { return Shift(cbWirePlrMsg).Lpb(); }
};

#endif
