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

inline constexpr std::size_t cbWireCargoXfer = 6;

inline bool FDecodeCargoXferPrefix(std::span<const std::byte> rgb, RTXFER &transfer) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::uint16_t id1, id2;
    std::uint8_t grobj, items;
    if (!input.FReadU16Le(id1) || !input.FReadU16Le(id2) || !input.FReadU8(grobj) || !input.FReadU8(items)) return false;
    transfer.id1 = id1; transfer.id2 = id2;
    transfer.grobj1 = grobj & 15; transfer.grobj2 = grobj >> 4; transfer.grbitItems = items;
    return true;
}
inline bool FEncodeCargoXferPrefix(RTXFER transfer, std::span<std::byte> rgb) noexcept {
    if (rgb.size() < cbWireCargoXfer) return false;
    NATIVEBYTEWRITER output(rgb);
    return output.FWriteU16Le(transfer.id1) && output.FWriteU16Le(transfer.id2) &&
           output.FWriteU8(static_cast<std::uint8_t>(transfer.grobj1 | (transfer.grobj2 << 4))) && output.FWriteU8(transfer.grbitItems);
}

inline constexpr std::size_t cbWirePlanetRouting = 6;
inline constexpr std::size_t cbWireLogThing = 4;

inline bool FDecodePlanetRouting(std::span<const std::byte> rgb, RTCHGPLANETLONG &routing) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::int16_t id;
    std::uint32_t ul;
    if (!input.FReadI16Le(id) || !input.FReadU32Le(ul)) return false;
    routing.id = id; routing.fNoResearch = ul & 1; routing.idFling = (ul >> 1) & 1023;
    routing.iWarpFling = (ul >> 11) & 15; routing.idRoute = (ul >> 15) & 1023; routing.unused = ul >> 25;
    return true;
}
inline bool FEncodePlanetRouting(const RTCHGPLANETLONG &routing, std::span<std::byte> rgb) noexcept {
    if (rgb.size() < cbWirePlanetRouting) return false;
    NATIVEBYTEWRITER output(rgb);
    return output.FWriteI16Le(routing.id) && output.FWriteU32Le(routing.ul());
}
inline bool FDecodeLogThing(std::span<const std::byte> rgb, RTLOGTHING &thing) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::uint16_t id;
    std::int16_t fDetonate;
    if (!input.FReadU16Le(id) || !input.FReadI16Le(fDetonate)) return false;
    thing.idFull = id; thing.fDetonate = fDetonate;
    return true;
}
inline bool FEncodeLogThing(RTLOGTHING thing, std::span<std::byte> rgb) noexcept {
    if (rgb.size() < cbWireLogThing) return false;
    NATIVEBYTEWRITER output(rgb);
    return output.FWriteU16Le(thing.idFull) && output.FWriteI16Le(thing.fDetonate);
}

#endif
