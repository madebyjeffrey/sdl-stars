#ifndef STARS_STREAMVIEWS_H
#define STARS_STREAMVIEWS_H

#include "streamcodec.h"
#include "byteview.h"

class NATIVEHEADERVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    std::uint16_t cb() const noexcept { return GetBits<0, 0, 10>(); }
    std::uint16_t rt() const noexcept { return GetBits<0, 10, 6>(); }
};
inline NATIVEHEADERVIEW HdvFromPrefix(std::uint8_t *lp) noexcept {
    return {std::as_writable_bytes(std::span(lp, cbWireHdr)), 0};
}

class NATIVEBOFVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    std::int32_t lidGame() const noexcept { return GetI32(4); }
    std::uint16_t wVersion() const noexcept { return GetU16(8); }
    std::uint16_t verInc() const noexcept { return GetBits<8, 0, 5>(); }
    std::uint16_t verMinor() const noexcept { return GetBits<8, 5, 7>(); }
    std::uint16_t verMajor() const noexcept { return GetBits<8, 12, 4>(); }
    std::uint16_t turn() const noexcept { return GetU16(10); }
    std::int16_t iPlayer() const noexcept { return I16FromWireBits(GetBits<12, 0, 5>(), 5); }
    std::int16_t lSaltTime() const noexcept { return I16FromWireBits(GetBits<12, 5, 11>(), 11); }
    std::uint16_t dt() const noexcept { return GetBits<14, 0, 8>(); }
    std::uint16_t fDone() const noexcept { return GetBits<14, 8, 1>(); }
    std::uint16_t fInUse() const noexcept { return GetBits<14, 9, 1>(); }
    std::uint16_t fMulti() const noexcept { return GetBits<14, 10, 1>(); }
    std::uint16_t fGameOverMan() const noexcept { return GetBits<14, 11, 1>(); }
    std::uint16_t fCrippled() const noexcept { return GetBits<14, 12, 1>(); }
    std::uint16_t wGen() const noexcept { return GetBits<14, 13, 3>(); }
};
inline NATIVEBOFVIEW BfvFromPrefix(void *lp, std::size_t cb) noexcept {
    return {std::span(static_cast<std::byte *>(lp), cb), 0};
}

class NATIVEPLANETRECORDVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    std::int16_t id() const noexcept { return I16FromWireBits(GetBits<0, 0, 11>(), 11); }
    std::int16_t iPlayer() const noexcept { return I16FromWireBits(GetBits<0, 11, 5>(), 5); }
    void SetId(std::int16_t id) noexcept { SetBits<0, 0, 11>(static_cast<std::uint16_t>(id)); }
    void SetIPlayer(std::int16_t iPlayer) noexcept { SetBits<0, 11, 5>(static_cast<std::uint16_t>(iPlayer)); }
    std::uint16_t det() const noexcept { return GetBits<2, 0, 7>(); }
    void SetDet(std::uint16_t value) noexcept { SetBits<2, 0, 7>(value); }
    std::uint16_t fHomeworld() const noexcept { return GetBits<2, 7, 1>(); }
    void SetFHomeworld(std::uint16_t value) noexcept { SetBits<2, 7, 1>(value); }
    std::uint16_t fInclude() const noexcept { return GetBits<2, 8, 1>(); }
    void SetFInclude(std::uint16_t value) noexcept { SetBits<2, 8, 1>(value); }
    std::uint16_t fStarbase() const noexcept { return GetBits<2, 9, 1>(); }
    void SetFStarbase(std::uint16_t value) noexcept { SetBits<2, 9, 1>(value); }
    std::uint16_t fIncEVO() const noexcept { return GetBits<2, 10, 1>(); }
    void SetFIncEVO(std::uint16_t value) noexcept { SetBits<2, 10, 1>(value); }
    std::uint16_t fIncImp() const noexcept { return GetBits<2, 11, 1>(); }
    void SetFIncImp(std::uint16_t value) noexcept { SetBits<2, 11, 1>(value); }
    std::uint16_t fIsArtifact() const noexcept { return GetBits<2, 12, 1>(); }
    void SetFIsArtifact(std::uint16_t value) noexcept { SetBits<2, 12, 1>(value); }
    std::uint16_t fIncSurfMin() const noexcept { return GetBits<2, 13, 1>(); }
    void SetFIncSurfMin(std::uint16_t value) noexcept { SetBits<2, 13, 1>(value); }
    std::uint16_t fRouting() const noexcept { return GetBits<2, 14, 1>(); }
    void SetFRouting(std::uint16_t value) noexcept { SetBits<2, 14, 1>(value); }
    std::uint16_t fFirstYear() const noexcept { return GetBits<2, 15, 1>(); }
    void SetFFirstYear(std::uint16_t value) noexcept { SetBits<2, 15, 1>(value); }
};
inline NATIVEPLANETRECORDVIEW PpvFromPrefix(void *lp, std::size_t cb) noexcept {
    return {std::span(static_cast<std::byte *>(lp), cb), 0};
}

class NATIVEPLAYERRECORDVIEW : public NATIVEBYTEWINDOW {
public:
    using NATIVEBYTEWINDOW::NATIVEBYTEWINDOW;
    std::int8_t iPlayer() const noexcept { return std::bit_cast<std::int8_t>(GetU8(0)); }
    std::uint16_t det() const noexcept { return GetBits<6, 0, 3>(); }
    std::uint16_t fAi() const noexcept { return GetBits<6, 9, 1>(); }
    std::uint16_t idAi() const noexcept { return GetBits<6, 13, 3>(); }
    std::int32_t lSalt() const noexcept { return GetI32(12); }
    void SetFAi(std::uint16_t value) noexcept { SetBits<6, 9, 1>(value); }
    void SetIdAi(std::uint16_t value) noexcept { SetBits<6, 13, 3>(value); }
    void SetLSalt(std::int32_t value) noexcept { SetI32(12, value); }
};
inline NATIVEPLAYERRECORDVIEW PlvFromPrefix(void *lp, std::size_t cb) noexcept {
    return {std::span(static_cast<std::byte *>(lp), cb), 0};
}

#endif
