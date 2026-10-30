#ifndef STARS_MESSAGECODEC_H
#define STARS_MESSAGECODEC_H

#include "structs.h"
#include "wire.h"

inline constexpr std::size_t cbWirePlrMsg = 12;

inline bool FDecodePlrMsgPrefix(std::span<const std::byte> rgb, RTPLRMSG &msg) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::uint32_t link;
    std::int16_t from, to, inRe, len;
    if (!input.FReadU32Le(link) || !input.FReadI16Le(from) || !input.FReadI16Le(to) ||
        !input.FReadI16Le(inRe) || !input.FReadI16Le(len)) return false;
    msg.lpmsgplrNext = link; msg.iPlrFrom = from; msg.iPlrTo = to; msg.iInRe = inRe; msg.cLen = len;
    return true;
}
inline bool FEncodePlrMsgPrefix(RTPLRMSG msg, std::span<std::byte> rgb) noexcept {
    if (rgb.size() < cbWirePlrMsg) return false;
    NATIVEBYTEWRITER output(rgb);
    return output.FWriteU32Le(msg.lpmsgplrNext) && output.FWriteI16Le(msg.iPlrFrom) && output.FWriteI16Le(msg.iPlrTo) &&
           output.FWriteI16Le(msg.iInRe) && output.FWriteI16Le(msg.cLen);
}

inline constexpr std::size_t cbWireMsgHdr = 4;
inline constexpr std::size_t cbWireMsgTurn = 5;

inline bool FDecodeMsgHdr(std::span<const std::byte> rgb, MSGHDR &mh) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::uint16_t w;
    std::int16_t go;
    if (!input.FReadU16Le(w) || !input.FReadI16Le(go)) return false;
    mh.iMsg = w & 0x1ff; mh.grWord = w >> 9; mh.wGoto = static_cast<MsgGoto>(go);
    return true;
}
inline bool FEncodeMsgHdr(MSGHDR mh, std::span<std::byte> rgb) noexcept {
    if (rgb.size() < cbWireMsgHdr) return false;
    NATIVEBYTEWRITER output(rgb);
    return output.FWriteU16Le(static_cast<std::uint16_t>(mh.iMsg | (mh.grWord << 9))) && output.FWriteI16Le(static_cast<std::int16_t>(mh.wGoto));
}
inline bool FDecodeMsgTurn(std::span<const std::byte> rgb, MSGTURN &mt) noexcept {
    if (rgb.size() < cbWireMsgTurn) return false;
    MSGTURN value{};
    const std::uint8_t ch = std::to_integer<std::uint8_t>(rgb[0]);
    value.iPlr = ch & 15; value.cbParams = ch >> 4;
    FDecodeMsgHdr(rgb.subspan(1, cbWireMsgHdr), value.msghdr);
    mt = value;
    return true;
}
inline bool FEncodeMsgTurn(MSGTURN mt, std::span<std::byte> rgb) noexcept {
    if (rgb.size() < cbWireMsgTurn) return false;
    rgb[0] = static_cast<std::byte>(mt.iPlr | (mt.cbParams << 4));
    return FEncodeMsgHdr(mt.msghdr, rgb.subspan(1, cbWireMsgHdr));
}
inline std::uint16_t WMsgParam(const std::uint8_t *lpb) noexcept {
    std::uint16_t w = 0;
    NATIVEBYTEREADER(std::as_bytes(std::span(lpb, 2))).FReadU16Le(w);
    return w;
}
inline void SetMsgParam(std::uint8_t *lpb, std::uint16_t w) noexcept {
    NATIVEBYTEWRITER(std::as_writable_bytes(std::span(lpb, 2))).FWriteU16Le(w);
}

#endif
