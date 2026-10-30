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

#endif
