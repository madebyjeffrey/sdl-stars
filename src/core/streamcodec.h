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

inline constexpr std::size_t cbWireBof = 16;
inline constexpr std::uint16_t VerInc(std::uint16_t w) noexcept { return w & 31; }
inline constexpr std::uint16_t VerMinor(std::uint16_t w) noexcept { return (w >> 5) & 127; }
inline constexpr std::uint16_t VerMajor(std::uint16_t w) noexcept { return w >> 12; }
inline constexpr std::int16_t IBofSignedBits(std::uint16_t w, unsigned cBits) noexcept {
    return static_cast<std::int16_t>((w & (1u << (cBits - 1))) ? std::int32_t(w) - (1 << cBits) : std::int32_t(w));
}
inline bool FDecodeBof(std::span<const std::byte> rgb, RTBOF &bof) noexcept {
    NATIVEBYTEREADER input(rgb);
    RTBOF value{};
    std::int32_t lid;
    std::uint16_t version, turn, playerSalt, flags;
    if (!input.FReadBytes(std::as_writable_bytes(std::span(value.rgid))) || !input.FReadI32Le(lid) ||
        !input.FReadU16Le(version) || !input.FReadU16Le(turn) || !input.FReadU16Le(playerSalt) || !input.FReadU16Le(flags)) return false;
    value.lidGame = lid; value.verInc = VerInc(version); value.verMinor = VerMinor(version); value.verMajor = VerMajor(version);
    value.turn = turn; value.iPlayer = IBofSignedBits(playerSalt & 31, 5); value.lSaltTime = IBofSignedBits(playerSalt >> 5, 11);
    value.dt = flags & 255; value.fDone = (flags >> 8) & 1; value.fInUse = (flags >> 9) & 1;
    value.fMulti = (flags >> 10) & 1; value.fGameOverMan = (flags >> 11) & 1; value.fCrippled = (flags >> 12) & 1; value.wGen = flags >> 13;
    bof = value;
    return true;
}
inline bool FEncodeBof(const RTBOF &bof, std::span<std::byte> rgb) noexcept {
    if (rgb.size() < cbWireBof) return false;
    const std::uint16_t playerSalt = static_cast<std::uint16_t>((std::uint16_t(bof.iPlayer) & 31) | ((std::uint16_t(bof.lSaltTime) & 2047) << 5));
    const std::uint16_t flags = static_cast<std::uint16_t>(bof.dt | (bof.fDone << 8) | (bof.fInUse << 9) | (bof.fMulti << 10) |
        (bof.fGameOverMan << 11) | (bof.fCrippled << 12) | (bof.wGen << 13));
    NATIVEBYTEWRITER output(rgb);
    return output.FWriteBytes(std::as_bytes(std::span(bof.rgid))) && output.FWriteI32Le(bof.lidGame) && output.FWriteU16Le(bof.wVersion()) &&
           output.FWriteU16Le(bof.turn) && output.FWriteU16Le(playerSalt) && output.FWriteU16Le(flags);
}

#endif
