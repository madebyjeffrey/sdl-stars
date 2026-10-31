#ifndef STARS_AIVIEWS_H
#define STARS_AIVIEWS_H

#include "aicodec.h"
#include "byteview.h"

// History, persistent Cybertron words and its temporary words share the same
// physical allocation. Keep their original byte positions and live updates;
// declared counts do not impose a new validation or rollback policy.
NATIVEBYTEWINDOW AbwFromLp(void *lp) noexcept;

class NATIVEAISTARBASEVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    explicit NATIVEAISTARBASEVIEW(NATIVEBYTEWINDOW window) noexcept : NATIVEBYTEWINDOW(window) {}
    std::int16_t idPlanet() const noexcept { return GetI16(0); }
    std::int16_t SetIdPlanet(std::int16_t value) noexcept { return SetI16(0, value); }
    std::int16_t cFreighter() const noexcept { return GetI16(2); }
    std::int16_t SetCFreighter(std::int16_t value) noexcept { return SetI16(2, value); }
    std::int16_t rgflid(std::int16_t i) const noexcept { return GetI16(4 + std::ptrdiff_t(i) * 2); }
    std::int16_t SetRgflid(std::int16_t i, std::int16_t value) noexcept { return SetI16(4 + std::ptrdiff_t(i) * 2, value); }
    void SetNextFlid(std::int16_t id) noexcept {
        // The old subscript postincrement published the count before the ID.
        std::int16_t i = cFreighter();
        SetCFreighter(static_cast<std::int16_t>(i + 1));
        SetRgflid(i, id);
    }
    AISTARBASE ReadValue() const noexcept {
        AISTARBASE asb{};
        if (!FDecodeAiStarbase(RgbField(0, cbWireAiStarbase), asb)) assert(false);
        return asb;
    }
    void SetValue(AISTARBASE asb) noexcept {
        if (!FEncodeAiStarbase(asb, RgbField(0, cbWireAiStarbase))) assert(false);
    }
};
class NATIVEAIHISTORYVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    explicit NATIVEAIHISTORYVIEW(NATIVEBYTEWINDOW window) noexcept : NATIVEBYTEWINDOW(window) {}
    std::uint16_t cbAiHist() const noexcept { return GetU16(0); }
    std::uint16_t SetCbAiHist(std::uint16_t value) noexcept { return SetU16(0, value); }
    std::int16_t cStarbase() const noexcept { return GetI16(2); }
    std::int16_t SetCStarbase(std::int16_t value) noexcept { return SetI16(2, value); }
    NATIVEAISTARBASEVIEW rgasb(std::int16_t i) const noexcept {
        return NATIVEAISTARBASEVIEW(Shift(std::ptrdiff_t(cbWireAiHistHeader) + std::ptrdiff_t(i) * std::ptrdiff_t(cbWireAiStarbase)));
    }
};
class NATIVEAICYBERVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    explicit NATIVEAICYBERVIEW(NATIVEBYTEWINDOW window) noexcept : NATIVEBYTEWINDOW(window) {}
    NATIVEAICYBERVIEW At(std::int16_t i) const noexcept { return NATIVEAICYBERVIEW(Shift(std::ptrdiff_t(i) * 2)); }
    std::uint16_t wInfo() const noexcept { return GetU16(0); }
    std::uint16_t SetWInfo(std::uint16_t value) noexcept { return SetU16(0, value); }
    std::uint16_t iLstPktDir() const noexcept { return GetBits<0, 0, 3>(); }
    std::uint16_t SetILstPktDir(std::uint16_t value) noexcept { return SetBits<0, 0, 3>(value); }
    std::uint16_t fBltColony() const noexcept { return GetBits<0, 3, 1>(); }
    std::uint16_t SetFBltColony(std::uint16_t value) noexcept { return SetBits<0, 3, 1>(value); }
    std::uint16_t fLaunchedPkt() const noexcept { return GetBits<0, 4, 1>(); }
    std::uint16_t SetFLaunchedPkt(std::uint16_t value) noexcept { return SetBits<0, 4, 1>(value); }
    std::uint16_t iPktTarget() const noexcept { return GetBits<0, 5, 2>(); }
    std::uint16_t SetIPktTarget(std::uint16_t value) noexcept { return SetBits<0, 5, 2>(value); }
    std::uint16_t fNeedScanPkt() const noexcept { return GetBits<0, 7, 1>(); }
    std::uint16_t SetFNeedScanPkt(std::uint16_t value) noexcept { return SetBits<0, 7, 1>(value); }
    std::uint16_t unused() const noexcept { return GetBits<0, 8, 8>(); }
    std::uint16_t SetUnused(std::uint16_t value) noexcept { return SetBits<0, 8, 8>(value); }
};
class NATIVEAICYBERTEMPVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    explicit NATIVEAICYBERTEMPVIEW(NATIVEBYTEWINDOW window) noexcept : NATIVEBYTEWINDOW(window) {}
    NATIVEAICYBERTEMPVIEW At(std::int16_t i) const noexcept { return NATIVEAICYBERTEMPVIEW(Shift(std::ptrdiff_t(i) * 2)); }
    std::uint16_t wInfo1() const noexcept { return GetU16(0); }
    std::uint16_t SetWInfo1(std::uint16_t value) noexcept { return SetU16(0, value); }
    std::uint16_t fIdleColonizers() const noexcept { return GetBits<0, 0, 1>(); }
    std::uint16_t SetFIdleColonizers(std::uint16_t value) noexcept { return SetBits<0, 0, 1>(value); }
    std::uint16_t cIdleFreighters() const noexcept { return GetBits<0, 1, 2>(); }
    std::uint16_t SetCIdleFreighters(std::uint16_t value) noexcept { return SetBits<0, 1, 2>(value); }
    std::uint16_t cFreightersDst() const noexcept { return GetBits<0, 3, 2>(); }
    std::uint16_t SetCFreightersDst(std::uint16_t value) noexcept { return SetBits<0, 3, 2>(value); }
    std::uint16_t fNeedDefenders() const noexcept { return GetBits<0, 5, 1>(); }
    std::uint16_t SetFNeedDefenders(std::uint16_t value) noexcept { return SetBits<0, 5, 1>(value); }
    std::uint16_t fDefended() const noexcept { return GetBits<0, 6, 1>(); }
    std::uint16_t SetFDefended(std::uint16_t value) noexcept { return SetBits<0, 6, 1>(value); }
    std::uint16_t fUnderAttack() const noexcept { return GetBits<0, 7, 1>(); }
    std::uint16_t SetFUnderAttack(std::uint16_t value) noexcept { return SetBits<0, 7, 1>(value); }
    std::uint16_t fNeedsMin1() const noexcept { return GetBits<0, 8, 1>(); }
    std::uint16_t SetFNeedsMin1(std::uint16_t value) noexcept { return SetBits<0, 8, 1>(value); }
    std::uint16_t fNeedsMin2() const noexcept { return GetBits<0, 9, 1>(); }
    std::uint16_t SetFNeedsMin2(std::uint16_t value) noexcept { return SetBits<0, 9, 1>(value); }
    std::uint16_t fNeedsMin3() const noexcept { return GetBits<0, 10, 1>(); }
    std::uint16_t SetFNeedsMin3(std::uint16_t value) noexcept { return SetBits<0, 10, 1>(value); }
    std::uint16_t unused() const noexcept { return GetBits<0, 11, 5>(); }
    std::uint16_t SetUnused(std::uint16_t value) noexcept { return SetBits<0, 11, 5>(value); }
};
// Cybertron reads its two-byte length as signed; ordinary AI logging reads
// the same bytes as unsigned. Preserve both original domains explicitly.
class NATIVEAIHISTSIZEVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    explicit NATIVEAIHISTSIZEVIEW(NATIVEBYTEWINDOW window) noexcept : NATIVEBYTEWINDOW(window) {}
    std::int16_t value() const noexcept { return GetI16(0); }
    void SetValue(std::int16_t value) noexcept { SetI16(0, value); }
};
class NATIVEAIMINERALVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    explicit NATIVEAIMINERALVIEW(NATIVEBYTEWINDOW window) noexcept : NATIVEBYTEWINDOW(window) {}
    std::int32_t value() const noexcept { return GetI32(0); }
    void SetValue(std::int32_t value) noexcept { SetI32(0, value); }
};
inline NATIVEAIMINERALVIEW MivFromLp(void *lp) noexcept { return NATIVEAIMINERALVIEW(AbwFromLp(lp)); }
inline NATIVEAIHISTORYVIEW AhvFromLp(void *lp) noexcept { return NATIVEAIHISTORYVIEW(AbwFromLp(lp)); }
inline NATIVEAICYBERVIEW CivFromLp(void *lp) noexcept { return NATIVEAICYBERVIEW(AbwFromLp(lp)); }
inline NATIVEAICYBERTEMPVIEW CtvFromLp(void *lp) noexcept { return NATIVEAICYBERTEMPVIEW(AbwFromLp(lp)); }
inline NATIVEAIHISTSIZEVIEW AivFromLp(void *lp) noexcept { return NATIVEAIHISTSIZEVIEW(AbwFromLp(lp)); }

#endif
