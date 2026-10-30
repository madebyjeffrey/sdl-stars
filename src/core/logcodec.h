#ifndef STARS_LOGCODEC_H
#define STARS_LOGCODEC_H

#include "structs.h"
#include "wire.h"

inline constexpr std::size_t cbWireChangeName = 4;
inline constexpr std::size_t cbWireResearch = 2;
inline constexpr std::size_t cbWirePassword = 4;
inline constexpr std::size_t cbWireLogHdr = 17;

inline bool FDecodeChangeNamePrefix(std::span<const std::byte> rgb, RTCHGNAME &name) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::int16_t id, grobj;
    if (!input.FReadI16Le(id) || !input.FReadI16Le(grobj)) return false;
    name.id = id; name.grobj = grobj;
    return true;
}
inline bool FEncodeChangeNamePrefix(const RTCHGNAME &name, std::span<std::byte> rgb) noexcept {
    if (rgb.size() < cbWireChangeName) return false;
    const std::int16_t id = name.id, grobj = name.grobj;
    NATIVEBYTEWRITER output(rgb);
    return output.FWriteI16Le(id) && output.FWriteI16Le(grobj);
}
inline bool FDecodeResearch(std::span<const std::byte> rgb, RTRESEARCH &research) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::uint8_t pct, tech;
    if (!input.FReadU8(pct) || !input.FReadU8(tech)) return false;
    research.pctResearch = std::bit_cast<std::int8_t>(pct);
    research.iTechCur = std::bit_cast<std::int8_t>(tech);
    return true;
}
inline bool FEncodeResearch(RTRESEARCH research, std::span<std::byte> rgb) noexcept {
    if (rgb.size() < cbWireResearch) return false;
    NATIVEBYTEWRITER output(rgb);
    return output.FWriteU8(std::bit_cast<std::uint8_t>(research.pctResearch)) &&
           output.FWriteU8(std::bit_cast<std::uint8_t>(research.iTechCur));
}
inline bool FDecodeLogHdr(std::span<const std::byte> rgb, RTLOGHDR &header) noexcept {
    NATIVEBYTEREADER input(rgb);
    RTLOGHDR value{};
    std::int16_t cb;
    if (!input.FReadI16Le(cb) || !input.FReadBytes(std::as_writable_bytes(std::span(value.rgbUnused)))) return false;
    value.cbLog = cb;
    header = value;
    return true;
}
inline bool FEncodeLogHdr(RTLOGHDR header, std::span<std::byte> rgb) noexcept {
    if (rgb.size() < cbWireLogHdr) return false;
    NATIVEBYTEWRITER output(rgb);
    return output.FWriteI16Le(header.cbLog) && output.FWriteBytes(std::as_bytes(std::span(header.rgbUnused)));
}

#endif
