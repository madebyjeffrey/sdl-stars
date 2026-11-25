#ifndef STARS_THINGVIEWS_H
#define STARS_THINGVIEWS_H

#include "byteview.h"
#include "enums.h"
#include "native.h"

// A call-scoped field view. Read-only variants hide both setters and the
// mutable byte-window interface, including when borrowing const storage.
template<bool fMutable>
class NATIVETHINGPOINTVIEW : protected NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    NATIVETHINGPOINTVIEW(const NATIVETHINGPOINTVIEW &) = default;
    NATIVETHINGPOINTVIEW &operator=(const NATIVETHINGPOINTVIEW &) = delete;
    std::int16_t x() const noexcept { return GetI16(0); }
    std::int16_t y() const noexcept { return GetI16(2); }
    operator POINT16() const noexcept { return {x(), y()}; }
    void SetX(std::int16_t x) noexcept requires(fMutable) { SetI16(0, x); }
    void SetY(std::int16_t y) noexcept requires(fMutable) { SetI16(2, y); }
    void Set(POINT16 pt) noexcept requires(fMutable) { SetX(pt.x); SetY(pt.y); }
};

template<bool fMutable>
class NATIVETHINGMINEVIEW : protected NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    std::int32_t cMines() const noexcept { return GetI32(0); }
    void SetCMines(std::int32_t cMines) noexcept requires(fMutable) { SetI32(0, cMines); }
    std::uint16_t grbitPlr() const noexcept { return GetU16(4); }
    void SetGrbitPlr(std::uint16_t grbitPlr) noexcept requires(fMutable) { SetU16(4, grbitPlr); }
    std::uint8_t iType() const noexcept { return GetU8(6); }
    void SetIType(std::uint8_t iType) noexcept requires(fMutable) { SetU8(6, iType); }
    std::uint8_t fDetonate() const noexcept { return GetU8(7); }
    void SetFDetonate(std::uint8_t fDetonate) noexcept requires(fMutable) { SetU8(7, fDetonate); }
    std::uint16_t grbitPlrNow() const noexcept { return GetU16(8); }
    void SetGrbitPlrNow(std::uint16_t grbitPlrNow) noexcept requires(fMutable) { SetU16(8, grbitPlrNow); }
};

template<bool fMutable>
class NATIVETHINGPACKVIEW : protected NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    std::uint16_t idPlanet() const noexcept { return GetBits<0, 0, 10>(); }
    void SetIdPlanet(std::uint16_t idPlanet) noexcept requires(fMutable) { SetBits<0, 0, 10>(idPlanet); }
    std::uint16_t iWarp() const noexcept { return GetBits<0, 10, 4>(); }
    void SetIWarp(std::uint16_t iWarp) noexcept requires(fMutable) { SetBits<0, 10, 4>(iWarp); }
    std::uint16_t fMoved() const noexcept { return GetBits<0, 14, 1>(); }
    void SetFMoved(std::uint16_t fMoved) noexcept requires(fMutable) { SetBits<0, 14, 1>(fMoved); }
    std::uint16_t fInclude() const noexcept { return GetBits<0, 15, 1>(); }
    void SetFInclude(std::uint16_t fInclude) noexcept requires(fMutable) { SetBits<0, 15, 1>(fInclude); }
    std::uint16_t wtMax() const noexcept { return GetBits<8, 0, 14>(); }
    void SetWtMax(std::uint16_t wtMax) noexcept requires(fMutable) { SetBits<8, 0, 14>(wtMax); }
    std::uint16_t iDecayRate() const noexcept { return GetBits<8, 14, 2>(); }
    void SetIDecayRate(std::uint16_t iDecayRate) noexcept requires(fMutable) { SetBits<8, 14, 2>(iDecayRate); }
    std::int16_t rgwtMin(int i) const noexcept { return GetI16(2 + i * 2); }
    void SetRgwtMin(int i, std::int16_t value) noexcept requires(fMutable) { SetI16(2 + i * 2, value); }
};

template<bool fMutable>
class NATIVETHINGWORMVIEW : protected NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    std::uint16_t iStable() const noexcept { return GetBits<0, 0, 2>(); }
    void SetIStable(std::uint16_t iStable) noexcept requires(fMutable) { SetBits<0, 0, 2>(iStable); }
    std::uint16_t cLastMove() const noexcept { return GetBits<0, 2, 10>(); }
    void SetCLastMove(std::uint16_t cLastMove) noexcept requires(fMutable) { SetBits<0, 2, 10>(cLastMove); }
    std::uint16_t fDestKnown() const noexcept { return GetBits<0, 12, 1>(); }
    void SetFDestKnown(std::uint16_t fDestKnown) noexcept requires(fMutable) { SetBits<0, 12, 1>(fDestKnown); }
    std::uint16_t fInclude() const noexcept { return GetBits<0, 13, 1>(); }
    void SetFInclude(std::uint16_t fInclude) noexcept requires(fMutable) { SetBits<0, 13, 1>(fInclude); }
    std::uint16_t grbitPlr() const noexcept { return GetU16(2); }
    void SetGrbitPlr(std::uint16_t grbitPlr) noexcept requires(fMutable) { SetU16(2, grbitPlr); }
    std::uint16_t grbitPlrTrav() const noexcept { return GetU16(4); }
    void SetGrbitPlrTrav(std::uint16_t grbitPlrTrav) noexcept requires(fMutable) { SetU16(4, grbitPlrTrav); }
    std::uint16_t idPartner() const noexcept { return GetU16(6); }
    void SetIdPartner(std::uint16_t idPartner) noexcept requires(fMutable) { SetU16(6, idPartner); }
};

template<bool fMutable>
class NATIVETHINGTRADERVIEW : protected NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    std::uint16_t iWarp() const noexcept { return GetBits<4, 0, 4>(); }
    void SetIWarp(std::uint16_t iWarp) noexcept requires(fMutable) { SetBits<4, 0, 4>(iWarp); }
    std::uint16_t fInclude() const noexcept { return GetBits<4, 4, 1>(); }
    void SetFInclude(std::uint16_t fInclude) noexcept requires(fMutable) { SetBits<4, 4, 1>(fInclude); }
    std::uint16_t unused() const noexcept { return GetBits<4, 5, 11>(); }
    void SetUnused(std::uint16_t unused) noexcept requires(fMutable) { SetBits<4, 5, 11>(unused); }
    std::uint16_t grbitPlr() const noexcept { return GetU16(6); }
    void SetGrbitPlr(std::uint16_t grbitPlr) noexcept requires(fMutable) { SetU16(6, grbitPlr); }
    GrbitTrader grbitTrader() const noexcept { return static_cast<GrbitTrader>(GetU16(8)); }
    void SetGrbitTrader(GrbitTrader grbitTrader) noexcept requires(fMutable) { SetU16(8, static_cast<std::uint16_t>(grbitTrader)); }
    NATIVETHINGPOINTVIEW<fMutable> ptDest() const noexcept { return {rgb, ib}; }
};

template<bool fMutable>
class NATIVETHINGHEADERVIEW : protected NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    std::uint16_t idFull() const noexcept { return GetU16(0); }
    void SetIdFull(std::uint16_t idFull) noexcept requires(fMutable) { SetU16(0, idFull); }
    std::uint16_t id() const noexcept { return GetBits<0, 0, 9>(); }
    void SetId(std::uint16_t id) noexcept requires(fMutable) { SetBits<0, 0, 9>(id); }
    std::uint16_t iplr() const noexcept { return GetBits<0, 9, 4>(); }
    void SetIplr(std::uint16_t iplr) noexcept requires(fMutable) { SetBits<0, 9, 4>(iplr); }
    std::uint16_t ith() const noexcept { return GetBits<0, 13, 3>(); }
    void SetIth(std::uint16_t ith) noexcept requires(fMutable) { SetBits<0, 13, 3>(ith); }
    std::uint16_t turn() const noexcept { return GetU16(16); }
    void SetTurn(std::uint16_t turn) noexcept requires(fMutable) { SetU16(16, turn); }
};

NATIVEBYTEWINDOW TbwFromLp(APPSESSION &app, void *lp) noexcept;

#endif
