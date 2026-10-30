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


inline constexpr std::size_t cbWireHs = 4;
inline constexpr std::size_t cbWireShDefPartial = 6;
inline constexpr std::size_t cbWireShDefFull = 17;
inline constexpr std::size_t cbWireChgShDef = 2;

inline bool FDecodeHs(std::span<const std::byte> rgb, HS &hs) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::uint16_t grhst;
    std::uint8_t iItem, cItem;
    if (!input.FReadU16Le(grhst) || !input.FReadU8(iItem) || !input.FReadU8(cItem)) return false;
    hs.grhst = static_cast<HullSlotType>(grhst);
    hs.iItem = iItem;
    hs.cItem = cItem;
    return true;
}

inline bool FEncodeHs(HS hs, std::span<std::byte> rgb) noexcept {
    NATIVEBYTEWRITER output(rgb);
    if (rgb.size() < cbWireHs) return false;
    return output.FWriteU16Le(static_cast<std::uint16_t>(hs.grhst)) && output.FWriteU8(hs.iItem) && output.FWriteU8(hs.cItem);
}

// A partial design places its string marker at byte six. That marker also
// populated hul.chs in the original reader; do not normalize it to a slot count.
// Full wire slots start at the unaligned byte seventeen and become real HS values.
inline bool FDecodeShDefFixed(std::span<const std::byte> rgb, SHDEF &shdef, std::size_t &ibName) noexcept {
    NATIVEBYTEREADER input(rgb);
    SHDEF shdefRead{};
    std::uint16_t wFlags, dp, turn;
    std::uint8_t ihuldef, ibmp, chs;
    std::uint32_t cBuilt, cExist;
    std::size_t ibNameRead;
    if (!input.FReadU16Le(wFlags) || !input.FReadU8(ihuldef) || !input.FReadU8(ibmp) ||
        !input.FReadU16Le(dp) || !input.FReadU8(chs)) return false;
    shdefRead.det = wFlags & 0xff;
    shdefRead.fInclude = (wFlags >> 8) & 1;
    shdefRead.fFree = (wFlags >> 9) & 1;
    shdefRead.ishdef = (wFlags >> 10) & 0x1f;
    shdefRead.fGift = wFlags >> 15;
    shdefRead.hul.ihuldef = static_cast<HulDef>(ihuldef);
    shdefRead.hul.ibmp = ibmp;
    shdefRead.hul.chs = chs;
    if (shdefRead.det == detAll) {
        if (chs > 16 || rgb.size() < cbWireShDefFull + chs * cbWireHs) return false;
        input.FReadU16Le(turn);
        input.FReadU32Le(cBuilt);
        input.FReadU32Le(cExist);
        shdefRead.hul.dp = dp;
        shdefRead.turn = turn;
        shdefRead.cBuilt = cBuilt;
        shdefRead.cExist = cExist;
        for (std::size_t i = 0; i < chs; i++) FDecodeHs(rgb.subspan(cbWireShDefFull + i * cbWireHs, cbWireHs), shdefRead.hul.rghs[i]);
        ibNameRead = cbWireShDefFull + chs * cbWireHs;
    } else {
        shdefRead.hul.wtEmpty = dp;
        ibNameRead = cbWireShDefPartial;
    }
    shdef = shdefRead;
    ibName = ibNameRead;
    return true;
}

// Stage only the fields present at this detail level. Native packing, inactive
// aliases and pointer size never define output bytes, including overlapping output.
inline bool FEncodeShDefFixed(const SHDEF &shdef, std::span<std::byte> rgb, std::size_t &ibName) noexcept {
    std::byte rgbFixed[cbWireShDefFull + 16 * cbWireHs];
    NATIVEBYTEWRITER output(rgbFixed);
    std::size_t cb = shdef.det == detAll ? cbWireShDefFull + shdef.hul.chs * cbWireHs : cbWireShDefPartial;
    if ((shdef.det == detAll && shdef.hul.chs > 16) || rgb.size() < cb) return false;
    output.FWriteU16Le(shdef.wFlags());
    output.FWriteU8(static_cast<std::uint8_t>(shdef.hul.ihuldef));
    output.FWriteU8(static_cast<std::uint8_t>(shdef.hul.ibmp));
    output.FWriteU16Le(shdef.det == detAll ? shdef.hul.dp : shdef.hul.wtEmpty);
    if (shdef.det == detAll) {
        output.FWriteU8(shdef.hul.chs);
        output.FWriteU16Le(shdef.turn);
        output.FWriteU32Le(shdef.cBuilt);
        output.FWriteU32Le(shdef.cExist);
        for (std::size_t i = 0; i < shdef.hul.chs; i++) FEncodeHs(shdef.hul.rghs[i], std::span(rgbFixed).subspan(cbWireShDefFull + i * cbWireHs, cbWireHs));
    }
    std::memmove(rgb.data(), rgbFixed, cb);
    ibName = cb;
    return true;
}

inline bool FDecodeRtChgShDefPrefix(std::span<const std::byte> rgb, RTCHGSHDEF &rtchg) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::uint16_t w;
    if (!input.FReadU16Le(w)) return false;
    rtchg.mdChg = w & 0x0f;
    rtchg.iPlr = (w >> 4) & 0x0f;
    rtchg.ishdef = (w >> 8) & 0x1f;
    rtchg.junk = w >> 13;
    return true;
}

inline bool FEncodeRtChgShDefPrefix(RTCHGSHDEF rtchg, std::span<std::byte> rgb) noexcept {
    return NATIVEBYTEWRITER(rgb).FWriteU16Le(static_cast<std::uint16_t>(rtchg.mdChg | (rtchg.iPlr << 4) | (rtchg.ishdef << 8) | (rtchg.junk << 13)));
}


inline constexpr std::size_t cbWireFleetHeader = 14;
inline constexpr std::size_t cbWireFleetIds = 0;
inline constexpr std::size_t cbWireFleetId = 2;
inline constexpr std::size_t cbWireXferF = 7;

inline bool FReadFleetId(RTFLEETIDS ids, std::size_t i, std::int16_t &id) noexcept {
    if (i >= ids.rgid.size() / cbWireFleetId) return false;
    return NATIVEBYTEREADER(ids.rgid.subspan(i * cbWireFleetId, cbWireFleetId)).FReadI16Le(id);
}

inline bool FEncodeFleetIds(std::span<const std::uint16_t> rgid, std::span<std::byte> rgb) noexcept {
    if (rgid.size() > rgb.size() / cbWireFleetId) return false;
    NATIVEBYTEWRITER output(rgb);
    for (auto id : rgid) output.FWriteU16Le(id);
    return true;
}

inline bool FEncodeRtFleetHeader(const FLEET &fl, bool fByte, std::uint16_t grbitCsh, std::span<std::byte> rgb) noexcept {
    std::byte rgbHeader[cbWireFleetHeader];
    NATIVEBYTEWRITER output(rgbHeader);
    std::uint16_t w;
    if (rgb.size() < cbWireFleetHeader) return false;
    w = static_cast<std::uint16_t>(fl.det | (fl.fInclude << 8) | (fl.fRepOrders << 9) | (fl.fDead << 10) | (fByte << 11) |
                                   (fl.fBombed << 12) | (fl.fHereAllTurn << 13) | (fl.fNoHeal << 14) | (fl.fMark << 15));
    output.FWriteI16Le(fl.id);
    output.FWriteI16Le(fl.iPlayer);
    output.FWriteU16Le(w);
    output.FWriteI16Le(fl.idPlanet);
    output.FWriteI16Le(fl.pt.x);
    output.FWriteI16Le(fl.pt.y);
    output.FWriteU16Le(grbitCsh);
    std::memmove(rgb.data(), rgbHeader, cbWireFleetHeader);
    return true;
}

inline bool FDecodeRtFleetHeader(std::span<const std::byte> rgb, FLEET &fl, std::uint16_t &grbitCsh) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::int16_t id, iPlayer, idPlanet, x, y;
    std::uint16_t w, mask;
    if (!input.FReadI16Le(id) || !input.FReadI16Le(iPlayer) || !input.FReadU16Le(w) || !input.FReadI16Le(idPlanet) ||
        !input.FReadI16Le(x) || !input.FReadI16Le(y) || !input.FReadU16Le(mask)) return false;
    fl.id = id;
    fl.iPlayer = iPlayer;
    fl.det = w & 0xff;
    fl.fInclude = (w >> 8) & 1;
    fl.fRepOrders = (w >> 9) & 1;
    fl.fDead = (w >> 10) & 1;
    fl.fDone = (w >> 11) & 1;
    fl.fBombed = (w >> 12) & 1;
    fl.fHereAllTurn = (w >> 13) & 1;
    fl.fNoHeal = (w >> 14) & 1;
    fl.fMark = w >> 15;
    fl.idPlanet = idPlanet;
    fl.pt.x = x;
    fl.pt.y = y;
    grbitCsh = mask;
    return true;
}

// The caller starts a zeroed fleet. Keep the original incremental publication:
// counts/cargo precede the full-record tag check, damage/plan/orders follow it.
// File loading passes the reusable scratch backing, not the declared length.
inline bool FDecodeRtFleet(std::span<const std::byte> rgb, FLEET &fl, bool fTypeA, std::int16_t &iDamage) noexcept {
    std::uint16_t mask, w;
    std::uint8_t ch;
    std::int16_t csh;
    std::int32_t dw;
    unsigned i = 0;
    unsigned cish = 0;
    if (!FDecodeRtFleetHeader(rgb, fl, mask)) return false;
    NATIVEBYTEREADER input(rgb.subspan(cbWireFleetHeader));
    for (; mask != 0; mask >>= 1, i++) {
        if (mask & 1) {
            if (fl.fDone) {
                if (!input.FReadU8(ch)) return false;
                fl.rgcsh[i] = ch;
            } else {
                if (!input.FReadI16Le(csh)) return false;
                fl.rgcsh[i] = csh;
            }
            if (fl.rgcsh[i] != 0) cish++;
        }
    }
    if (cish == 0) fl.fDead = TRUE;
    if (fl.det >= detMore) {
        if (!input.FReadU16Le(mask)) return false;
        for (i = 0; i < 5; i++, mask >>= 2) {
            switch (mask & 3) {
            case 0: break;
            case 1:
                if (!input.FReadU8(ch)) return false;
                fl.rgwtMin[i] = ch;
                break;
            case 2:
                if (!input.FReadU16Le(w)) return false;
                fl.rgwtMin[i] = w;
                break;
            case 3:
                if (!input.FReadI32Le(dw)) return false;
                fl.rgwtMin[i] = dw;
                break;
            }
        }
    }
    if (fl.det < detAll) {
        if (!input.FReadI32Le(dw)) return false;
        fl.dirLong = dw;
        if (!input.FReadI32Le(dw)) return false;
        fl.SetWtFleet(dw);
        return true;
    }
    if (!fTypeA) return false;
    if (!input.FReadU16Le(mask)) return false;
    i = 0;
    for (; mask != 0; mask >>= 1, i++) {
        if (mask & 1) {
            if (!input.FReadU16Le(w)) return false;
            fl.rgdv[i].dp = w;
            if (fl.rgdv[i].pctDp() >= 500) fl.rgdv[i].SetPctDp(499);
        }
    }
    iDamage = static_cast<std::int16_t>(i);
    if (!input.FReadU8(ch)) return false;
    fl.iplan = ch;
    if (!input.FReadU8(ch)) return false;
    fl.cord = ch;
    return true;
}

// Preserve the writer's positive-count and detail rules. The caller computes
// partial fleet mass in its original arithmetic order using the design catalog.
inline bool FEncodeRtFleet(const FLEET &fl, std::int32_t wtPartial, std::span<std::byte> rgb, std::size_t &cbWritten) noexcept {
    std::byte rgbFleet[134];
    std::uint16_t mask = 0, grMask = 1;
    bool fByte = true;
    for (unsigned i = 0; i < 16; i++, grMask = static_cast<std::uint16_t>(grMask * 2)) {
        if (fl.rgcsh[i] > 0) {
            mask |= grMask;
            if (fl.rgcsh[i] > 255) fByte = false;
        }
    }
    FEncodeRtFleetHeader(fl, fByte, mask, rgbFleet);
    NATIVEBYTEWRITER output{std::span(rgbFleet).subspan(cbWireFleetHeader)};
    for (unsigned i = 0; i < 16; i++) {
        if (fl.rgcsh[i] > 0) {
            if (fByte) output.FWriteU8(static_cast<std::uint8_t>(fl.rgcsh[i]));
            else output.FWriteI16Le(fl.rgcsh[i]);
        }
    }
    if (fl.det >= detMore) {
        mask = 0;
        std::size_t ibMask = cbWireFleetHeader + output.CbWritten();
        output.FWriteU16Le(0);
        for (unsigned i = 0; i < 5; i++) {
            if (fl.rgwtMin[i] > 0 && (fl.det == detAll || (i != 4 && i != 3))) {
                if (fl.rgwtMin[i] <= 255) {
                    mask |= static_cast<std::uint16_t>(1u << (2 * i));
                    output.FWriteU8(static_cast<std::uint8_t>(fl.rgwtMin[i]));
                } else if (fl.rgwtMin[i] > 65535) {
                    mask |= static_cast<std::uint16_t>(3u << (2 * i));
                    output.FWriteI32Le(fl.rgwtMin[i]);
                } else {
                    mask |= static_cast<std::uint16_t>(2u << (2 * i));
                    output.FWriteU16Le(static_cast<std::uint16_t>(fl.rgwtMin[i]));
                }
            }
        }
        NATIVEBYTEWRITER(std::span(rgbFleet).subspan(ibMask, 2)).FWriteU16Le(mask);
    }
    if (fl.det < detAll) {
        output.FWriteI32Le(fl.dirLong);
        output.FWriteI32Le(wtPartial);
    } else {
        mask = 0; grMask = 1;
        for (unsigned i = 0; i < 16; i++, grMask = static_cast<std::uint16_t>(grMask * 2)) {
            if (fl.rgdv[i].dp != 0) mask |= grMask;
        }
        output.FWriteU16Le(mask);
        for (unsigned i = 0; i < 16; i++) if (fl.rgdv[i].dp != 0) output.FWriteU16Le(fl.rgdv[i].dp);
        output.FWriteU8(fl.iplan);
        output.FWriteU8(static_cast<std::uint8_t>(fl.cord));
    }
    std::size_t cb = cbWireFleetHeader + output.CbWritten();
    if (rgb.size() < cb) return false;
    std::memmove(rgb.data(), rgbFleet, cb);
    cbWritten = cb;
    return true;
}


inline bool FEncodeRtXferFHeader(RTXFERF rtxf, std::span<std::byte> rgb) noexcept {
    std::byte rgbHeader[cbWireXferF];
    NATIVEBYTEWRITER output(rgbHeader);
    if (rgb.size() < cbWireXferF) return false;
    output.FWriteU16Le(rtxf.id1);
    output.FWriteU16Le(rtxf.id2);
    output.FWriteU8(static_cast<std::uint8_t>(rtxf.grobj1 | (rtxf.grobj2 << 4)));
    output.FWriteU16Le(rtxf.grbitItems);
    std::memmove(rgb.data(), rgbHeader, cbWireXferF);
    return true;
}

inline constexpr std::size_t cbWireGame = 64;
inline constexpr std::size_t cbWireStarpack = 4;

inline bool FDecodeGame(std::span<const std::byte> rgb, GAME &game) noexcept {
    NATIVEBYTEREADER input(rgb);
    GAME value{};
    std::int32_t lid;
    std::int16_t cPlayer, cPlanMax, fDirty;
    std::uint16_t size, density, distance, flags, turn;
    if (!input.FReadI32Le(lid) || !input.FReadU16Le(size) || !input.FReadU16Le(density) || !input.FReadI16Le(cPlayer) ||
        !input.FReadI16Le(cPlanMax) || !input.FReadU16Le(distance) || !input.FReadI16Le(fDirty) || !input.FReadU16Le(flags) ||
        !input.FReadU16Le(turn) || !input.FReadBytes(std::as_writable_bytes(std::span(value.rgvc))) ||
        !input.FReadBytes(std::as_writable_bytes(std::span(value.szName)))) return false;
    value.lid = lid; value.mdSize = static_cast<UniverseSize>(size); value.mdDensity = static_cast<UniverseDensity>(density);
    value.cPlayer = cPlayer; value.cPlanMax = cPlanMax; value.mdStartDist = static_cast<StartDistance>(distance); value.fDirty = fDirty;
    value.fExtraFuel = flags & 1; value.fSlowTech = (flags >> 1) & 1; value.fSinglePlr = (flags >> 2) & 1;
    value.fTutorial = (flags >> 3) & 1; value.fAisBand = (flags >> 4) & 1; value.fBBSPlay = (flags >> 5) & 1;
    value.fVisScores = (flags >> 6) & 1; value.fNoRandom = (flags >> 7) & 1; value.fClumping = (flags >> 8) & 1;
    value.wGen = (flags >> 9) & 7; value.unused = flags >> 12; value.turn = turn;
    game = value;
    return true;
}
inline bool FEncodeGame(const GAME &game, std::span<std::byte> rgb) noexcept {
    if (rgb.size() < cbWireGame) return false;
    NATIVEBYTEWRITER output(rgb);
    return output.FWriteI32Le(game.lid) && output.FWriteU16Le(game.mdSize) && output.FWriteU16Le(game.mdDensity) &&
        output.FWriteI16Le(game.cPlayer) && output.FWriteI16Le(game.cPlanMax) && output.FWriteU16Le(game.mdStartDist) &&
        output.FWriteI16Le(game.fDirty) && output.FWriteU16Le(game.wCrap()) && output.FWriteU16Le(game.turn) &&
        output.FWriteBytes(std::as_bytes(std::span(game.rgvc))) && output.FWriteBytes(std::as_bytes(std::span(game.szName)));
}
inline bool FDecodeStarpack(std::span<const std::byte> rgb, STARPACK &sp) noexcept {
    std::uint32_t ul;
    if (!NATIVEBYTEREADER(rgb).FReadU32Le(ul)) return false;
    sp.dx = ul & 1023; sp.y = (ul >> 10) & 4095; sp.id = ul >> 22;
    return true;
}
inline bool FEncodeStarpack(STARPACK sp, std::span<std::byte> rgb) noexcept {
    return NATIVEBYTEWRITER(rgb).FWriteU32Le(static_cast<std::uint32_t>(sp.dx | (std::uint32_t(sp.y) << 10) | (std::uint32_t(sp.id) << 22)));
}

inline constexpr std::size_t cbWireScoreX = 24;

inline bool FDecodeScoreX(std::span<const std::byte> rgb, SCOREX &sx) noexcept {
    NATIVEBYTEREADER input(rgb);
    SCOREX value{};
    std::uint16_t flags, turn, ships[3];
    std::int32_t score, resources;
    std::int16_t planets, bases, tech;
    if (!input.FReadU16Le(flags) || !input.FReadU16Le(turn) || !input.FReadI32Le(score) || !input.FReadI32Le(resources) ||
        !input.FReadI16Le(planets) || !input.FReadI16Le(bases) || !input.FReadU16Le(ships[0]) || !input.FReadU16Le(ships[1]) ||
        !input.FReadU16Le(ships[2]) || !input.FReadI16Le(tech)) return false;
    value.iPlayer = flags & 31; value.fValid = (flags >> 5) & 1; value.grbitVC = (flags >> 6) & 255;
    value.fWinner = (flags >> 14) & 1; value.fHistory = flags >> 15; value.turn = turn;
    value.score.lScore = score; value.score.cResources = resources; value.score.cPlanet = planets; value.score.cStarbase = bases;
    for (int i = 0; i < 3; i++) value.score.rgcsh[i] = ships[i];
    value.score.cTechLevels = tech;
    sx = value;
    return true;
}
inline bool FEncodeScoreX(const SCOREX &sx, std::span<std::byte> rgb) noexcept {
    if (rgb.size() < cbWireScoreX) return false;
    NATIVEBYTEWRITER output(rgb);
    return output.FWriteU16Le(sx.wWord()) && output.FWriteU16Le(sx.turn) && output.FWriteI32Le(sx.score.lScore) &&
        output.FWriteI32Le(sx.score.cResources) && output.FWriteI16Le(sx.score.cPlanet) && output.FWriteI16Le(sx.score.cStarbase) &&
        output.FWriteU16Le(sx.score.rgcsh[0]) && output.FWriteU16Le(sx.score.rgcsh[1]) && output.FWriteU16Le(sx.score.rgcsh[2]) &&
        output.FWriteI16Le(sx.score.cTechLevels);
}

#endif
