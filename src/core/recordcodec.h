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

inline constexpr std::size_t cbWireItemAction = 2;

inline bool FDecodeItemAction(std::span<const std::byte> rgb, ITEMACTION &ia) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::uint16_t w;
    if (!input.FReadU16Le(w)) return false;
    ia.w = w;
    return true;
}

inline bool FEncodeItemAction(ITEMACTION ia, std::span<std::byte> rgb) noexcept {
    NATIVEBYTEWRITER output(rgb);
    return output.FWriteU16Le(ia.w);
}

inline constexpr std::size_t cbWireOrderHeader = 8;
inline constexpr std::size_t cbWireOrder = 18;
inline constexpr std::size_t cbWireWaypt = 4;
inline constexpr std::size_t cbWireShipInt = 4;

inline bool FEncodeOrderHeader(const ORDER &ord, std::span<std::byte> rgb) noexcept {
    NATIVEBYTEWRITER output(rgb);
    std::int16_t x, y, id;
    std::uint16_t w;
    if (rgb.size() < cbWireOrderHeader) return false;
    x = ord.pt.x;
    y = ord.pt.y;
    id = ord.id;
    w = static_cast<std::uint16_t>(ord.grTask | (ord.iWarp << 4) | (ord.grobj << 8) |
                                   (ord.fValidTask << 12) | (ord.fNoAutoTrack << 13) | (ord.fUnused << 14));
    return output.FWriteI16Le(x) && output.FWriteI16Le(y) && output.FWriteI16Le(id) && output.FWriteU16Le(w);
}

inline bool FDecodeOrderHeader(std::span<const std::byte> rgb, ORDER &ord) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::int16_t x, y, id;
    std::uint16_t w;
    if (!input.FReadI16Le(x) || !input.FReadI16Le(y) || !input.FReadI16Le(id) || !input.FReadU16Le(w)) return false;
    ord.pt.x = x;
    ord.pt.y = y;
    ord.id = id;
    ord.grTask = w & 0x0f;
    ord.iWarp = (w >> 4) & 0x0f;
    ord.grobj = (w >> 8) & 0x0f;
    ord.fValidTask = (w >> 12) & 1;
    ord.fNoAutoTrack = (w >> 13) & 1;
    ord.fUnused = w >> 14;
    return true;
}

inline bool FEncodeOrder(ORDER ord, std::span<std::byte> rgb) noexcept {
    if (rgb.size() < cbWireOrder) return false;
    FEncodeOrderHeader(ord, rgb);
    NATIVEBYTEWRITER task(rgb.subspan(cbWireOrderHeader));
    for (unsigned i = 0; i < 5; i++) task.FWriteU16Le(ord.txp.rgia[i].w);
    return true;
}

inline bool FDecodeOrder(std::span<const std::byte> rgb, ORDER &ord) noexcept {
    ORDER ordRead{};
    std::uint16_t w;
    if (rgb.size() < cbWireOrder) return false;
    FDecodeOrderHeader(rgb, ordRead);
    NATIVEBYTEREADER input(rgb.subspan(cbWireOrderHeader));
    for (unsigned i = 0; i < 5; i++) {
        input.FReadU16Le(w);
        ordRead.txp.rgia[i].w = w;
    }
    ord = ordRead;
    return true;
}

inline bool FDecodeOrderPrefix(std::span<const std::byte> rgb, ORDER &ord) noexcept {
    std::byte rgbOrder[cbWireOrder];
    if (rgb.size() > cbWireOrder) return false;
    if (rgb.size() == cbWireOrder) return FDecodeOrder(rgb, ord);
    FEncodeOrder(ord, rgbOrder);
    if (!rgb.empty()) std::memcpy(rgbOrder, rgb.data(), rgb.size());
    return FDecodeOrder(rgbOrder, ord);
}

inline bool FEncodeRtWaypt(RTWAYPT rtwp, std::span<std::byte> rgb) noexcept {
    NATIVEBYTEWRITER output(rgb);
    if (rgb.size() < cbWireWaypt + cbWireOrder) return false;
    output.FWriteI16Le(rtwp.id);
    output.FWriteI16Le(rtwp.iWaypt);
    return FEncodeOrder(rtwp.order, rgb.subspan(cbWireWaypt));
}

inline bool FDecodeRtWayptPrefix(std::span<const std::byte> rgb, RTWAYPT &rtwp) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::int16_t id, iWaypt;
    if (!input.FReadI16Le(id) || !input.FReadI16Le(iWaypt)) return false;
    rtwp.id = id;
    rtwp.iWaypt = iWaypt;
    return true;
}

inline bool FEncodeRtShipInt(RTSHIPINT rtsi, std::span<std::byte> rgb) noexcept {
    NATIVEBYTEWRITER output(rgb);
    if (rgb.size() < cbWireShipInt) return false;
    return output.FWriteI16Le(rtsi.id) && output.FWriteI16Le(rtsi.i);
}

#endif
