#ifndef STARS_AICODEC_H
#define STARS_AICODEC_H

#include "structs.h"
#include "wire.h"

inline constexpr std::size_t cbWireAiHistHeader = 4;
inline constexpr std::size_t cbWireAiStarbase = 20;
inline constexpr std::size_t cbWireCyberInfo = 2;

inline bool FDecodeAiStarbase(std::span<const std::byte> rgb, AISTARBASE &asb) noexcept {
    NATIVEBYTEREADER input(rgb);
    AISTARBASE value{};
    std::int16_t id, cFreighter, rgflid[8];
    if (!input.FReadI16Le(id) || !input.FReadI16Le(cFreighter)) return false;
    for (auto &flid : rgflid) if (!input.FReadI16Le(flid)) return false;
    value.idPlanet = id; value.cFreighter = cFreighter;
    for (unsigned i = 0; i < 8; i++) value.rgflid[i] = rgflid[i];
    asb = value;
    return true;
}
inline bool FEncodeAiStarbase(AISTARBASE asb, std::span<std::byte> rgb) noexcept {
    if (rgb.size() < cbWireAiStarbase) return false;
    NATIVEBYTEWRITER output(rgb);
    output.FWriteI16Le(asb.idPlanet); output.FWriteI16Le(asb.cFreighter);
    for (unsigned i = 0; i < 8; i++) output.FWriteI16Le(asb.rgflid[i]);
    return true;
}

#endif
