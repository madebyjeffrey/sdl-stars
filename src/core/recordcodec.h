#ifndef STARS_RECORDCODEC_H
#define STARS_RECORDCODEC_H

#include "structs.h"
#include "wire.h"

inline constexpr std::size_t cbWireHistHdr = 4;

inline bool FDecodeRtHistHdr(std::span<const std::byte> rgb, RTHISTHDR &rthh) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::int16_t cPlanet, cPlanetExtra;
    if (!input.FReadI16Le(cPlanet) || !input.FReadI16Le(cPlanetExtra)) return false;
    rthh.cPlanet = cPlanet;
    rthh.cPlanetExtra = cPlanetExtra;
    return true;
}

// Snapshot values before writing, so a source overlapping the byte output is
// as safe as the original memmove into the record buffer.
inline bool FEncodeRtHistHdr(RTHISTHDR rthh, std::span<std::byte> rgb) noexcept {
    NATIVEBYTEWRITER output(rgb);
    if (output.CbRemaining() < cbWireHistHdr) return false;
    return output.FWriteI16Le(rthh.cPlanet) && output.FWriteI16Le(rthh.cPlanetExtra);
}

#endif
