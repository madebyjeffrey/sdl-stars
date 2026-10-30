#ifndef STARS_BATTLECODEC_H
#define STARS_BATTLECODEC_H

#include "structs.h"
#include "wire.h"

inline constexpr std::size_t cbWireBattleData = 14;
inline constexpr std::size_t cbWireBattleRecord = 6;
inline constexpr std::size_t cbWireKill = 8;
inline constexpr std::size_t cbWireTok = 29;

inline bool FDecodeKill(std::span<const std::byte> rgb, KILL &kill) noexcept {
    NATIVEBYTEREADER input(rgb);
    KILL killRead{};
    std::uint16_t cshKill, dpShield, dv;
    if (!input.FReadU8(killRead.itok) || !input.FReadU8(killRead.grfWeapon) || !input.FReadU16Le(cshKill) ||
        !input.FReadU16Le(dpShield) || !input.FReadU16Le(dv)) return false;
    killRead.cshKill = cshKill;
    killRead.dpShield = dpShield;
    killRead.dv.dp = dv;
    kill = killRead;
    return true;
}

inline bool FEncodeKill(KILL kill, std::span<std::byte> rgb) noexcept {
    NATIVEBYTEWRITER output(rgb);
    if (rgb.size() < cbWireKill) return false;
    return output.FWriteU8(kill.itok) && output.FWriteU8(kill.grfWeapon) && output.FWriteU16Le(kill.cshKill) &&
           output.FWriteU16Le(kill.dpShield) && output.FWriteU16Le(kill.dv.dp);
}

inline bool FDecodeBattleRecord(std::span<const std::byte> rgb, BTLREC &br) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::uint8_t itok, brcDest;
    std::int16_t ctok;
    std::uint16_t w;
    if (!input.FReadU8(itok) || !input.FReadU8(brcDest) || !input.FReadI16Le(ctok) || !input.FReadU16Le(w)) return false;
    br.itok = itok;
    br.brcDest = brcDest;
    br.ctok = ctok;
    br.iRound = w & 0x0f;
    br.dzDis = (w >> 4) & 0x0f;
    br.itokAttack = w >> 8;
    return true;
}

inline bool FDecodeBattleRecord26(std::span<const std::byte> rgb, BTLREC26 &br) noexcept {
    NATIVEBYTEREADER input(rgb);
    std::uint8_t itok, brcDest, itokAttack, ctok;
    std::uint16_t w;
    if (!input.FReadU8(itok) || !input.FReadU8(brcDest) || !input.FReadU8(itokAttack) || !input.FReadU8(ctok) || !input.FReadU16Le(w)) return false;
    br.itok = itok;
    br.brcDest = brcDest;
    br.itokAttack = itokAttack;
    br.ctok = ctok;
    br.iRound = w & 0x0f;
    br.dzDis = (w >> 4) & 0x0f;
    br.unused = w >> 8;
    return true;
}

inline bool FEncodeBattleRecord(BTLREC br, std::span<std::byte> rgb) noexcept {
    NATIVEBYTEWRITER output(rgb);
    if (rgb.size() < cbWireBattleRecord) return false;
    return output.FWriteU8(br.itok) && output.FWriteU8(br.brcDest) && output.FWriteI16Le(br.ctok) &&
           output.FWriteU16Le(static_cast<std::uint16_t>(br.iRound | (br.dzDis << 4) | (br.itokAttack << 8)));
}

inline bool FDecodeBattleData(std::span<const std::byte> rgb, BTLDATA &bd) noexcept {
    NATIVEBYTEREADER input(rgb);
    BTLDATA btldataRead{};
    std::uint16_t id, grfPlr, cbData, idPlanet;
    std::int16_t x, y;
    if (!input.FReadU16Le(id) || !input.FReadU8(btldataRead.cplr) || !input.FReadU8(btldataRead.ctok) || !input.FReadU16Le(grfPlr) ||
        !input.FReadU16Le(cbData) || !input.FReadU16Le(idPlanet) || !input.FReadI16Le(x) || !input.FReadI16Le(y)) return false;
    btldataRead.id = id;
    btldataRead.grfPlr = grfPlr;
    btldataRead.cbData = cbData;
    btldataRead.idPlanet = idPlanet;
    btldataRead.pt.x = x;
    btldataRead.pt.y = y;
    bd = btldataRead;
    return true;
}

inline bool FEncodeBattleData(BTLDATA bd, std::span<std::byte> rgb) noexcept {
    NATIVEBYTEWRITER output(rgb);
    if (rgb.size() < cbWireBattleData) return false;
    return output.FWriteU16Le(bd.id) && output.FWriteU8(bd.cplr) && output.FWriteU8(bd.ctok) && output.FWriteU16Le(bd.grfPlr) &&
           output.FWriteU16Le(bd.cbData) && output.FWriteU16Le(bd.idPlanet) && output.FWriteI16Le(bd.pt.x) && output.FWriteI16Le(bd.pt.y);
}

inline bool FDecodeTok(std::span<const std::byte> rgb, TOK &tok) noexcept {
    NATIVEBYTEREADER input(rgb);
    TOK tokRead{};
    std::uint16_t w;
    if (rgb.size() < cbWireTok) return false;
    input.FReadU16Le(w); tokRead.id = w;
    input.FReadU8(tokRead.iplr); input.FReadU8(tokRead.grobj); input.FReadU8(tokRead.ishdef); input.FReadU8(tokRead.brc);
    input.FReadU8(tokRead.initBase); input.FReadU8(tokRead.initMin); input.FReadU8(tokRead.initMac); input.FReadU8(tokRead.itokTarget);
    input.FReadU8(tokRead.pctCloak); input.FReadU8(tokRead.pctJam); input.FReadU8(tokRead.pctBC); input.FReadU8(tokRead.pctCap); input.FReadU8(tokRead.pctBeamDef);
    input.FReadU16Le(w); tokRead.wt = w;
    input.FReadU16Le(w); tokRead.dpShield = w;
    input.FReadU16Le(w); tokRead.csh = w;
    input.FReadU16Le(w); tokRead.dv.dp = w;
    input.FReadU16Le(w);
    tokRead.mdTarget1 = w & 0x0f; tokRead.mdTarget2 = (w >> 4) & 0x0f; tokRead.mdTactic = (w >> 8) & 0x0f; tokRead.mdTarget0 = w >> 12;
    input.FReadU16Le(w);
    tokRead.dxyLim = w & 0x0f; tokRead.dxyMax = (w >> 4) & 0x0f; tokRead.spd = (w >> 8) & 0x0f; tokRead.cTarget = w >> 12;
    input.FReadU16Le(w);
    tokRead.fActive = w & 1; tokRead.fDetector = (w >> 1) & 1; tokRead.fTorp = (w >> 2) & 1; tokRead.fRegen = (w >> 3) & 1;
    tokRead.fMoved = (w >> 4) & 1; tokRead.dzDis = (w >> 5) & 0x1f; tokRead.dwt = (w >> 10) & 0x0f; tokRead.dMovesLeft = w >> 14;
    tok = tokRead;
    return true;
}

inline bool FEncodeTok(TOK tok, std::span<std::byte> rgb) noexcept {
    NATIVEBYTEWRITER output(rgb);
    if (rgb.size() < cbWireTok) return false;
    output.FWriteU16Le(tok.id);
    output.FWriteU8(tok.iplr); output.FWriteU8(tok.grobj); output.FWriteU8(tok.ishdef); output.FWriteU8(tok.brc);
    output.FWriteU8(tok.initBase); output.FWriteU8(tok.initMin); output.FWriteU8(tok.initMac); output.FWriteU8(tok.itokTarget);
    output.FWriteU8(tok.pctCloak); output.FWriteU8(tok.pctJam); output.FWriteU8(tok.pctBC); output.FWriteU8(tok.pctCap); output.FWriteU8(tok.pctBeamDef);
    output.FWriteU16Le(tok.wt); output.FWriteU16Le(tok.dpShield); output.FWriteU16Le(tok.csh); output.FWriteU16Le(tok.dv.dp);
    output.FWriteU16Le(static_cast<std::uint16_t>(tok.mdTarget1 | (tok.mdTarget2 << 4) | (tok.mdTactic << 8) | (tok.mdTarget0 << 12)));
    output.FWriteU16Le(static_cast<std::uint16_t>(tok.dxyLim | (tok.dxyMax << 4) | (tok.spd << 8) | (tok.cTarget << 12)));
    output.FWriteU16Le(static_cast<std::uint16_t>(tok.fActive | (tok.fDetector << 1) | (tok.fTorp << 2) | (tok.fRegen << 3) |
                                               (tok.fMoved << 4) | (tok.dzDis << 5) | (tok.dwt << 10) | (tok.dMovesLeft << 14)));
    return true;
}

#endif
