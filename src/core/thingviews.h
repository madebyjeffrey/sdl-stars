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
    int16_t x() const noexcept { return GetI16(0); }
    int16_t y() const noexcept { return GetI16(2); }
    operator POINT16() const noexcept { return {x(), y()}; }
    void SetX(int16_t x) noexcept requires(fMutable) { SetI16(0, x); }
    void SetY(int16_t y) noexcept requires(fMutable) { SetI16(2, y); }
    void Set(POINT16 pt) noexcept requires(fMutable) { SetX(pt.x); SetY(pt.y); }
};

template<bool fMutable>
class NATIVETHINGMINEVIEW : protected NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    int32_t cMines() const noexcept { return GetI32(0); }
    void SetCMines(int32_t cMines) noexcept requires(fMutable) { SetI32(0, cMines); }
    uint16_t grbitPlr() const noexcept { return GetU16(4); }
    void SetGrbitPlr(uint16_t grbitPlr) noexcept requires(fMutable) { SetU16(4, grbitPlr); }
    uint8_t iType() const noexcept { return GetU8(6); }
    void SetIType(uint8_t iType) noexcept requires(fMutable) { SetU8(6, iType); }
    uint8_t fDetonate() const noexcept { return GetU8(7); }
    void SetFDetonate(uint8_t fDetonate) noexcept requires(fMutable) { SetU8(7, fDetonate); }
    uint16_t grbitPlrNow() const noexcept { return GetU16(8); }
    void SetGrbitPlrNow(uint16_t grbitPlrNow) noexcept requires(fMutable) { SetU16(8, grbitPlrNow); }
};

template<bool fMutable>
class NATIVETHINGPACKVIEW : protected NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    uint16_t idPlanet() const noexcept { return GetBits<0, 0, 10>(); }
    void SetIdPlanet(uint16_t idPlanet) noexcept requires(fMutable) { SetBits<0, 0, 10>(idPlanet); }
    uint16_t iWarp() const noexcept { return GetBits<0, 10, 4>(); }
    void SetIWarp(uint16_t iWarp) noexcept requires(fMutable) { SetBits<0, 10, 4>(iWarp); }
    uint16_t fMoved() const noexcept { return GetBits<0, 14, 1>(); }
    void SetFMoved(uint16_t fMoved) noexcept requires(fMutable) { SetBits<0, 14, 1>(fMoved); }
    uint16_t fInclude() const noexcept { return GetBits<0, 15, 1>(); }
    void SetFInclude(uint16_t fInclude) noexcept requires(fMutable) { SetBits<0, 15, 1>(fInclude); }
    uint16_t wtMax() const noexcept { return GetBits<8, 0, 14>(); }
    void SetWtMax(uint16_t wtMax) noexcept requires(fMutable) { SetBits<8, 0, 14>(wtMax); }
    uint16_t iDecayRate() const noexcept { return GetBits<8, 14, 2>(); }
    void SetIDecayRate(uint16_t iDecayRate) noexcept requires(fMutable) { SetBits<8, 14, 2>(iDecayRate); }
    int16_t rgwtMin(int i) const noexcept { return GetI16(2 + i * 2); }
    void SetRgwtMin(int i, int16_t value) noexcept requires(fMutable) { SetI16(2 + i * 2, value); }
};

template<bool fMutable>
class NATIVETHINGWORMVIEW : protected NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    uint16_t iStable() const noexcept { return GetBits<0, 0, 2>(); }
    void SetIStable(uint16_t iStable) noexcept requires(fMutable) { SetBits<0, 0, 2>(iStable); }
    uint16_t cLastMove() const noexcept { return GetBits<0, 2, 10>(); }
    void SetCLastMove(uint16_t cLastMove) noexcept requires(fMutable) { SetBits<0, 2, 10>(cLastMove); }
    uint16_t fDestKnown() const noexcept { return GetBits<0, 12, 1>(); }
    void SetFDestKnown(uint16_t fDestKnown) noexcept requires(fMutable) { SetBits<0, 12, 1>(fDestKnown); }
    uint16_t fInclude() const noexcept { return GetBits<0, 13, 1>(); }
    void SetFInclude(uint16_t fInclude) noexcept requires(fMutable) { SetBits<0, 13, 1>(fInclude); }
    uint16_t grbitPlr() const noexcept { return GetU16(2); }
    void SetGrbitPlr(uint16_t grbitPlr) noexcept requires(fMutable) { SetU16(2, grbitPlr); }
    uint16_t grbitPlrTrav() const noexcept { return GetU16(4); }
    void SetGrbitPlrTrav(uint16_t grbitPlrTrav) noexcept requires(fMutable) { SetU16(4, grbitPlrTrav); }
    uint16_t idPartner() const noexcept { return GetU16(6); }
    void SetIdPartner(uint16_t idPartner) noexcept requires(fMutable) { SetU16(6, idPartner); }
};

template<bool fMutable>
class NATIVETHINGTRADERVIEW : protected NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    uint16_t iWarp() const noexcept { return GetBits<4, 0, 4>(); }
    void SetIWarp(uint16_t iWarp) noexcept requires(fMutable) { SetBits<4, 0, 4>(iWarp); }
    uint16_t fInclude() const noexcept { return GetBits<4, 4, 1>(); }
    void SetFInclude(uint16_t fInclude) noexcept requires(fMutable) { SetBits<4, 4, 1>(fInclude); }
    uint16_t unused() const noexcept { return GetBits<4, 5, 11>(); }
    void SetUnused(uint16_t unused) noexcept requires(fMutable) { SetBits<4, 5, 11>(unused); }
    uint16_t grbitPlr() const noexcept { return GetU16(6); }
    void SetGrbitPlr(uint16_t grbitPlr) noexcept requires(fMutable) { SetU16(6, grbitPlr); }
    GrbitTrader grbitTrader() const noexcept { return static_cast<GrbitTrader>(GetU16(8)); }
    void SetGrbitTrader(GrbitTrader grbitTrader) noexcept requires(fMutable) { SetU16(8, static_cast<uint16_t>(grbitTrader)); }
    NATIVETHINGPOINTVIEW<fMutable> ptDest() const noexcept { return {rgb, ib}; }
};

template<bool fMutable>
class NATIVETHINGHEADERVIEW : protected NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    uint16_t idFull() const noexcept { return GetU16(0); }
    void SetIdFull(uint16_t idFull) noexcept requires(fMutable) { SetU16(0, idFull); }
    uint16_t id() const noexcept { return GetBits<0, 0, 9>(); }
    void SetId(uint16_t id) noexcept requires(fMutable) { SetBits<0, 0, 9>(id); }
    uint16_t iplr() const noexcept { return GetBits<0, 9, 4>(); }
    void SetIplr(uint16_t iplr) noexcept requires(fMutable) { SetBits<0, 9, 4>(iplr); }
    uint16_t ith() const noexcept { return GetBits<0, 13, 3>(); }
    void SetIth(uint16_t ith) noexcept requires(fMutable) { SetBits<0, 13, 3>(ith); }
    uint16_t turn() const noexcept { return GetU16(16); }
    void SetTurn(uint16_t turn) noexcept requires(fMutable) { SetU16(16, turn); }
};

NATIVEBYTEWINDOW TbwFromLp(APPSESSION &app, void *lp) noexcept;

#endif
