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

inline constexpr std::size_t cbWireProd = 4;
inline constexpr std::size_t cbWireChgProdQ = 2;

inline bool FDecodeProd(std::span<const std::byte> rgb, PROD &prod) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::uint32_t dw;
    if (!input.FReadU32Le(dw)) return false;
    prod.cItem = dw & 0x3ff;
    prod.iItem = (dw >> 10) & 0x7f;
    prod.grobj = (dw >> 17) & 7;
    prod.pct = (dw >> 20) & 0x7f;
    prod.unused = dw >> 27;
    return true;
}

inline bool FEncodeProd(PROD prod, std::span<std::byte> rgb) noexcept {
    NATIVEBYTEWRITER output(rgb);
    return output.FWriteU32Le(std::uint32_t(prod.cItem) | (std::uint32_t(prod.iItem) << 10) | (std::uint32_t(prod.grobj) << 17) |
                             (std::uint32_t(prod.pct) << 20) | (std::uint32_t(prod.unused) << 27));
}

inline bool FEncodeProdQueue(std::span<const PROD> rgprod, std::span<std::byte> rgb) noexcept {
    if (rgprod.size() > rgb.size() / cbWireProd) return false;
    for (std::size_t i = 0; i < rgprod.size(); i++) FEncodeProd(rgprod[i], rgb.subspan(i * cbWireProd, cbWireProd));
    return true;
}

// A short final row used to overwrite just its leading bytes in spare runtime
// capacity. Merge that prefix through the codec to preserve the same effect
// without relying on the native PROD representation or rejecting the record.
inline bool FDecodeProdBytes(std::span<const std::byte> rgb, std::span<PROD> rgprod) noexcept {
    std::size_t cProd = rgb.size() / cbWireProd;
    std::size_t cbExtra = rgb.size() % cbWireProd;
    std::byte rgbLast[cbWireProd];
    if (cProd > rgprod.size() || (cbExtra != 0 && cProd == rgprod.size())) return false;
    for (std::size_t i = 0; i < cProd; i++) FDecodeProd(rgb.subspan(i * cbWireProd, cbWireProd), rgprod[i]);
    if (cbExtra != 0) {
        FEncodeProd(rgprod[cProd], rgbLast);
        std::memcpy(rgbLast, rgb.data() + cProd * cbWireProd, cbExtra);
        FDecodeProd(rgbLast, rgprod[cProd]);
    }
    return true;
}

inline bool FDecodeRtChgProdQ(std::span<const std::byte> rgb, RTCHGPRODQ &rtcpq) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::int16_t id;
    if (!input.FReadI16Le(id)) return false;
    rtcpq.id = id;
    return true;
}

#endif
