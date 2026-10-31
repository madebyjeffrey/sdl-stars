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

NATIVEBYTEWINDOW MbwFromLp(void *lp) noexcept;

class NATIVEMESSAGEHEADERVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    explicit NATIVEMESSAGEHEADERVIEW(NATIVEBYTEWINDOW window) noexcept : NATIVEBYTEWINDOW(window) {}
    std::uint16_t iMsg() const noexcept { return GetBits<0, 0, 9>(); }
    std::uint16_t SetIMsg(std::uint16_t value) noexcept { return SetBits<0, 0, 9>(value); }
    std::uint16_t grWord() const noexcept { return GetBits<0, 9, 7>(); }
    std::uint16_t SetGrWord(std::uint16_t value) noexcept { return SetBits<0, 9, 7>(value); }
    MsgGoto wGoto() const noexcept { return static_cast<MsgGoto>(GetI16(2)); }
};
class NATIVEMESSAGETURNVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    explicit NATIVEMESSAGETURNVIEW(NATIVEBYTEWINDOW window) noexcept : NATIVEBYTEWINDOW(window) {}
    std::uint8_t iPlr() const noexcept { return static_cast<std::uint8_t>(GetBits<0, 0, 4>()); }
    std::uint8_t cbParams() const noexcept { return static_cast<std::uint8_t>(GetBits<0, 4, 4>()); }
    std::uint8_t SetCbParams(std::uint8_t value) noexcept { return static_cast<std::uint8_t>(SetBits<0, 4, 4>(value)); }
    NATIVEMESSAGEHEADERVIEW msghdr() const noexcept { return NATIVEMESSAGEHEADERVIEW(Shift(1)); }
    std::uint8_t *Header() const noexcept { return Shift(1).Lpb(); }
    std::uint8_t *Next() const noexcept { return Shift(cbWireMsgTurn + cbParams()).Lpb(); }
};
inline NATIVEMESSAGEHEADERVIEW MhvFromLp(void *lp) noexcept { return NATIVEMESSAGEHEADERVIEW(MbwFromLp(lp)); }
inline NATIVEMESSAGETURNVIEW MtvFromLp(void *lp) noexcept { return NATIVEMESSAGETURNVIEW(MbwFromLp(lp)); }
inline NATIVEMESSAGEHEADERVIEW MhvFromPrefix(std::uint8_t *lp) noexcept { return {std::as_writable_bytes(std::span(lp, cbWireMsgHdr)), 0}; }
inline NATIVEMESSAGETURNVIEW MtvFromPrefix(std::uint8_t *lp) noexcept { return {std::as_writable_bytes(std::span(lp, cbWireMsgTurn)), 0}; }

#endif
