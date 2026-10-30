#ifndef STARS_BATTLEVIEWS_H
#define STARS_BATTLEVIEWS_H

#include "battlecodec.h"
#include <cassert>

// Borrow an arena's bytes only for this operation. The position may denote an
// end boundary; reading a field requires only that field's physical bytes.
// Declared counts/lengths do not become a new admission policy here.
class NATIVEBATTLEWINDOW {
protected:
    std::span<std::byte> rgb;
    std::ptrdiff_t ib = 0;
    std::span<std::byte> RgbField(std::ptrdiff_t dib, std::size_t cb) const noexcept {
        std::ptrdiff_t ibField = ib + dib;
        if (ibField < 0 || static_cast<std::size_t>(ibField) > rgb.size() || cb > rgb.size() - static_cast<std::size_t>(ibField)) {
            assert(false && "battle field outside its physical arena");
            return {};
        }
        return rgb.subspan(static_cast<std::size_t>(ibField), cb);
    }
    std::uint8_t GetU8(std::ptrdiff_t dib) const noexcept {
        std::uint8_t ch = 0;
        if (!NATIVEBYTEREADER(RgbField(dib, 1)).FReadU8(ch)) assert(false);
        return ch;
    }
    std::uint16_t GetU16(std::ptrdiff_t dib) const noexcept {
        std::uint16_t w = 0;
        if (!NATIVEBYTEREADER(RgbField(dib, 2)).FReadU16Le(w)) assert(false);
        return w;
    }
    std::int16_t GetI16(std::ptrdiff_t dib) const noexcept {
        std::int16_t w = 0;
        if (!NATIVEBYTEREADER(RgbField(dib, 2)).FReadI16Le(w)) assert(false);
        return w;
    }
    std::uint8_t SetU8(std::ptrdiff_t dib, std::uint8_t ch) noexcept {
        if (!NATIVEBYTEWRITER(RgbField(dib, 1)).FWriteU8(ch)) assert(false);
        return ch;
    }
    std::uint16_t SetU16(std::ptrdiff_t dib, std::uint16_t w) noexcept {
        if (!NATIVEBYTEWRITER(RgbField(dib, 2)).FWriteU16Le(w)) assert(false);
        return w;
    }
    std::int16_t SetI16(std::ptrdiff_t dib, std::int16_t w) noexcept {
        if (!NATIVEBYTEWRITER(RgbField(dib, 2)).FWriteI16Le(w)) assert(false);
        return w;
    }
    template<std::ptrdiff_t ibWord, unsigned iBit, unsigned cBits>
    std::uint16_t GetBits() const noexcept {
        static_assert(cBits > 0 && iBit + cBits <= 16);
        constexpr unsigned i = iBit % 8;
        constexpr unsigned cb = (i + cBits + 7) / 8;
        constexpr std::uint16_t mask = static_cast<std::uint16_t>((std::uint32_t{1} << cBits) - 1);
        std::uint16_t w;
        if constexpr (cb == 1) w = GetU8(ibWord + iBit / 8);
        else w = GetU16(ibWord + iBit / 8);
        return static_cast<std::uint16_t>((w >> i) & mask);
    }
    template<std::ptrdiff_t ibWord, unsigned iBit, unsigned cBits>
    std::uint16_t SetBits(std::uint16_t value) noexcept {
        static_assert(cBits > 0 && iBit + cBits <= 16);
        constexpr unsigned i = iBit % 8;
        constexpr unsigned cb = (i + cBits + 7) / 8;
        constexpr std::uint16_t mask = static_cast<std::uint16_t>((std::uint32_t{1} << cBits) - 1);
        std::uint16_t w;
        if constexpr (cb == 1) w = GetU8(ibWord + iBit / 8);
        else w = GetU16(ibWord + iBit / 8);
        w = static_cast<std::uint16_t>((w & ~(std::uint32_t(mask) << i)) | (std::uint32_t(value & mask) << i));
        if constexpr (cb == 1) SetU8(ibWord + iBit / 8, static_cast<std::uint8_t>(w));
        else SetU16(ibWord + iBit / 8, w);
        return value & mask;
    }
    NATIVEBATTLEWINDOW Shift(std::ptrdiff_t dib) const noexcept { return {rgb, ib + dib}; }
public:
    NATIVEBATTLEWINDOW() = default;
    NATIVEBATTLEWINDOW(std::span<std::byte> rgb, std::ptrdiff_t ib = 0) noexcept : rgb(rgb), ib(ib) {
        assert(ib >= 0 && static_cast<std::size_t>(ib) <= rgb.size());
    }
    explicit operator bool() const noexcept { return rgb.data() != nullptr; }
    std::uint8_t *Lpb() const noexcept {
        if (!rgb.data()) return nullptr;
        assert(ib >= 0 && static_cast<std::size_t>(ib) <= rgb.size());
        return reinterpret_cast<std::uint8_t *>(rgb.data() + ib);
    }
    std::span<std::byte> Rgb() const noexcept { return RgbField(0, rgb.size() - static_cast<std::size_t>(ib)); }
};

// The game heap adapter supplies the physical allocation, including its old
// spare bytes. Pure codec/view tests can instead supply an explicit byte span.
NATIVEBATTLEWINDOW BbwFromLp(void *lp) noexcept;

class NATIVEBATTLEPOINTVIEW : public NATIVEBATTLEWINDOW {
public:
    using NATIVEBATTLEWINDOW::NATIVEBATTLEWINDOW;
    NATIVEBATTLEPOINTVIEW() = default;
    explicit NATIVEBATTLEPOINTVIEW(NATIVEBATTLEWINDOW window) noexcept : NATIVEBATTLEWINDOW(window) {}
    std::int16_t x() const noexcept { return GetI16(0); }
    std::int16_t SetX(std::int16_t value) noexcept { return SetI16(0, value); }
    std::int16_t y() const noexcept { return GetI16(2); }
    std::int16_t SetY(std::int16_t value) noexcept { return SetI16(2, value); }
    POINT16 ReadValue() const noexcept { return {x(), y()}; }
};

class NATIVEBATTLEDVVIEW : public NATIVEBATTLEWINDOW {
public:
    using NATIVEBATTLEWINDOW::NATIVEBATTLEWINDOW;
    NATIVEBATTLEDVVIEW() = default;
    explicit NATIVEBATTLEDVVIEW(NATIVEBATTLEWINDOW window) noexcept : NATIVEBATTLEWINDOW(window) {}
    std::uint16_t dp() const noexcept { return GetU16(0); }
    std::uint16_t SetDp(std::uint16_t value) noexcept { return SetU16(0, value); }
    std::uint16_t pctSh() const noexcept { return GetBits<0, 0, 7>(); }
    std::uint16_t SetPctSh(std::uint16_t value) noexcept { return SetBits<0, 0, 7>(value); }
    std::uint16_t pctDp() const noexcept { return GetBits<0, 7, 9>(); }
    std::uint16_t SetPctDp(std::uint16_t value) noexcept { return SetBits<0, 7, 9>(value); }
    DV ReadValue() const noexcept { return {dp()}; }
};

class NATIVEBATTLEKILLVIEW : public NATIVEBATTLEWINDOW {
public:
    using NATIVEBATTLEWINDOW::NATIVEBATTLEWINDOW;
    NATIVEBATTLEKILLVIEW() = default;
    explicit NATIVEBATTLEKILLVIEW(NATIVEBATTLEWINDOW window) noexcept : NATIVEBATTLEWINDOW(window) {}
    std::uint8_t itok() const noexcept { return GetU8(0); }
    std::uint8_t SetItok(std::uint8_t value) noexcept { return SetU8(0, value); }
    std::uint8_t grfWeapon() const noexcept { return GetU8(1); }
    std::uint8_t SetGrfWeapon(std::uint8_t value) noexcept { return SetU8(1, value); }
    std::uint16_t cshKill() const noexcept { return GetU16(2); }
    std::uint16_t SetCshKill(std::uint16_t value) noexcept { return SetU16(2, value); }
    std::uint16_t dpShield() const noexcept { return GetU16(4); }
    std::uint16_t SetDpShield(std::uint16_t value) noexcept { return SetU16(4, value); }
    NATIVEBATTLEDVVIEW dv() const noexcept { return NATIVEBATTLEDVVIEW(Shift(6)); }
    KILL ReadValue() const noexcept {
        KILL kill{};
        if (!FDecodeKill(RgbField(0, cbWireKill), kill)) assert(false);
        return kill;
    }
};

class NATIVEBATTLETOKENVIEW : public NATIVEBATTLEWINDOW {
public:
    using NATIVEBATTLEWINDOW::NATIVEBATTLEWINDOW;
    NATIVEBATTLETOKENVIEW() = default;
    explicit NATIVEBATTLETOKENVIEW(NATIVEBATTLEWINDOW window) noexcept : NATIVEBATTLEWINDOW(window) {}
    std::uint16_t id() const noexcept { return GetU16(0); }
    std::uint8_t iplr() const noexcept { return GetU8(2); }
    std::uint8_t grobj() const noexcept { return GetU8(3); }
    std::uint8_t ishdef() const noexcept { return GetU8(4); }
    std::uint8_t brc() const noexcept { return GetU8(5); }
    std::uint8_t initBase() const noexcept { return GetU8(6); }
    std::uint8_t initMin() const noexcept { return GetU8(7); }
    std::uint8_t initMac() const noexcept { return GetU8(8); }
    std::uint8_t itokTarget() const noexcept { return GetU8(9); }
    std::uint8_t pctCloak() const noexcept { return GetU8(10); }
    std::uint8_t pctJam() const noexcept { return GetU8(11); }
    std::uint8_t pctBC() const noexcept { return GetU8(12); }
    std::uint8_t pctCap() const noexcept { return GetU8(13); }
    std::uint8_t pctBeamDef() const noexcept { return GetU8(14); }
    std::uint16_t wt() const noexcept { return GetU16(15); }
    std::uint16_t dpShield() const noexcept { return GetU16(17); }
    std::uint16_t csh() const noexcept { return GetU16(19); }
    std::uint16_t mdTarget1() const noexcept { return GetBits<23, 0, 4>(); }
    std::uint16_t mdTarget2() const noexcept { return GetBits<23, 4, 4>(); }
    std::uint16_t mdTactic() const noexcept { return GetBits<23, 8, 4>(); }
    std::uint16_t mdTarget0() const noexcept { return GetBits<23, 12, 4>(); }
    std::uint16_t dxyLim() const noexcept { return GetBits<25, 0, 4>(); }
    std::uint16_t dxyMax() const noexcept { return GetBits<25, 4, 4>(); }
    std::uint16_t spd() const noexcept { return GetBits<25, 8, 4>(); }
    std::uint16_t cTarget() const noexcept { return GetBits<25, 12, 4>(); }
    std::uint16_t fActive() const noexcept { return GetBits<27, 0, 1>(); }
    std::uint16_t fDetector() const noexcept { return GetBits<27, 1, 1>(); }
    std::uint16_t fTorp() const noexcept { return GetBits<27, 2, 1>(); }
    std::uint16_t fRegen() const noexcept { return GetBits<27, 3, 1>(); }
    std::uint16_t fMoved() const noexcept { return GetBits<27, 4, 1>(); }
    std::uint16_t dzDis() const noexcept { return GetBits<27, 5, 5>(); }
    std::uint16_t dwt() const noexcept { return GetBits<27, 10, 4>(); }
    std::uint16_t dMovesLeft() const noexcept { return GetBits<27, 14, 2>(); }
    std::uint16_t wFlags() const noexcept { return GetU16(27); }
    NATIVEBATTLEDVVIEW dv() const noexcept { return NATIVEBATTLEDVVIEW(Shift(21)); }
    TOK ReadValue() const noexcept {
        TOK tok{};
        if (!FDecodeTok(RgbField(0, cbWireTok), tok)) assert(false);
        return tok;
    }
};

class NATIVEBATTLEEVENTVIEW : public NATIVEBATTLEWINDOW {
public:
    using NATIVEBATTLEWINDOW::NATIVEBATTLEWINDOW;
    NATIVEBATTLEEVENTVIEW() = default;
    explicit NATIVEBATTLEEVENTVIEW(NATIVEBATTLEWINDOW window) noexcept : NATIVEBATTLEWINDOW(window) {}
    std::uint8_t itok() const noexcept { return GetU8(0); }
    std::uint8_t SetItok(std::uint8_t value) noexcept { return SetU8(0, value); }
    std::uint8_t brcDest() const noexcept { return GetU8(1); }
    std::uint8_t SetBrcDest(std::uint8_t value) noexcept { return SetU8(1, value); }
    std::int16_t ctok() const noexcept { return GetI16(2); }
    std::int16_t SetCtok(std::int16_t value) noexcept { return SetI16(2, value); }
    std::uint16_t iRound() const noexcept { return GetBits<4, 0, 4>(); }
    std::uint16_t SetIRound(std::uint16_t value) noexcept { return SetBits<4, 0, 4>(value); }
    std::uint16_t dzDis() const noexcept { return GetBits<4, 4, 4>(); }
    std::uint16_t SetDzDis(std::uint16_t value) noexcept { return SetBits<4, 4, 4>(value); }
    std::uint16_t itokAttack() const noexcept { return GetBits<4, 8, 8>(); }
    std::uint16_t SetItokAttack(std::uint16_t value) noexcept { return SetBits<4, 8, 8>(value); }
    NATIVEBATTLEKILLVIEW rgkill(std::int16_t i) const noexcept { return NATIVEBATTLEKILLVIEW(Shift(std::ptrdiff_t(cbWireBattleRecord) + std::ptrdiff_t(i) * std::ptrdiff_t(cbWireKill))); }
    std::uint8_t *Next() const noexcept { return Shift(std::ptrdiff_t(cbWireBattleRecord) + std::ptrdiff_t(ctok()) * std::ptrdiff_t(cbWireKill)).Lpb(); }
    BTLREC ReadValue() const noexcept {
        BTLREC br{};
        if (!FDecodeBattleRecord(RgbField(0, cbWireBattleRecord), br)) assert(false);
        return br;
    }
};

class NATIVEBATTLEDATAVIEW : public NATIVEBATTLEWINDOW {
public:
    using NATIVEBATTLEWINDOW::NATIVEBATTLEWINDOW;
    NATIVEBATTLEDATAVIEW() = default;
    explicit NATIVEBATTLEDATAVIEW(NATIVEBATTLEWINDOW window) noexcept : NATIVEBATTLEWINDOW(window) {}
    std::uint16_t id() const noexcept { return GetU16(0); }
    std::uint16_t SetId(std::uint16_t value) noexcept { return SetU16(0, value); }
    std::uint8_t cplr() const noexcept { return GetU8(2); }
    std::uint8_t SetCplr(std::uint8_t value) noexcept { return SetU8(2, value); }
    std::uint8_t ctok() const noexcept { return GetU8(3); }
    std::uint8_t SetCtok(std::uint8_t value) noexcept { return SetU8(3, value); }
    std::uint16_t grfPlr() const noexcept { return GetU16(4); }
    std::uint16_t SetGrfPlr(std::uint16_t value) noexcept { return SetU16(4, value); }
    std::uint16_t cbData() const noexcept { return GetU16(6); }
    std::uint16_t SetCbData(std::uint16_t value) noexcept { return SetU16(6, value); }
    std::uint16_t idPlanet() const noexcept { return GetU16(8); }
    std::uint16_t SetIdPlanet(std::uint16_t value) noexcept { return SetU16(8, value); }
    NATIVEBATTLEPOINTVIEW pt() const noexcept { return NATIVEBATTLEPOINTVIEW(Shift(10)); }
    void SetPt(POINT16 pt) noexcept { SetI16(10, pt.x); SetI16(12, pt.y); }
    NATIVEBATTLETOKENVIEW rgtok(std::int16_t i) const noexcept { return NATIVEBATTLETOKENVIEW(Shift(std::ptrdiff_t(cbWireBattleData) + std::ptrdiff_t(i) * std::ptrdiff_t(cbWireTok))); }
    std::uint8_t *FirstEvent() const noexcept { return Shift(cbWireBattleData + std::ptrdiff_t(ctok()) * cbWireTok).Lpb(); }
    std::uint8_t *Next() const noexcept { return Shift(cbData()).Lpb(); }
    BTLDATA ReadValue() const noexcept {
        BTLDATA bd{};
        if (!FDecodeBattleData(RgbField(0, cbWireBattleData), bd)) assert(false);
        return bd;
    }
};

inline NATIVEBATTLEDATAVIEW BdvFromLp(void *lp) noexcept { return NATIVEBATTLEDATAVIEW(BbwFromLp(lp)); }
inline NATIVEBATTLETOKENVIEW BtvFromLp(void *lp) noexcept { return NATIVEBATTLETOKENVIEW(BbwFromLp(lp)); }
inline NATIVEBATTLEEVENTVIEW BevFromLp(void *lp) noexcept { return NATIVEBATTLEEVENTVIEW(BbwFromLp(lp)); }
// Producers and isolated core callers already require these complete prefixes.
// No view is kept across a callback or a change to the global write cursor.
inline NATIVEBATTLEKILLVIEW BkvFromPrefix(std::uint8_t *lp) noexcept { return {std::as_writable_bytes(std::span(lp, cbWireKill)), 0}; }
inline NATIVEBATTLEEVENTVIEW BevFromPrefix(std::uint8_t *lp) noexcept { return {std::as_writable_bytes(std::span(lp, cbWireBattleRecord)), 0}; }
inline NATIVEBATTLEDATAVIEW BdvFromPrefix(std::uint8_t *lp) noexcept { return {std::as_writable_bytes(std::span(lp, cbWireBattleData)), 0}; }

#endif
