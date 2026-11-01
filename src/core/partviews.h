#ifndef STARS_PARTVIEWS_H
#define STARS_PARTVIEWS_H

#include "structs.h"
#include <cassert>
#include <cstdlib>
#include <type_traits>

// Snapshot the selected borrow, not its PART carrier or the catalog values.
// Callbacks may replace the carrier; catalog reads must still use this object.
class NATIVEPARTCOMMONVIEW {
    friend struct _part;
    void *lpv;
    PartPointerKind ppk;
    NATIVEPARTCOMMONVIEW(void *lpv, PartPointerKind ppk) noexcept : lpv(lpv), ppk(ppk) {}
    template<class F> decltype(auto) Visit(F fn) const noexcept {
        switch (ppk) {
        case ppkCommon: return fn(*static_cast<COMPART *>(lpv));
        case ppkArmor: return fn(*static_cast<ARMOR *>(lpv));
        case ppkHull: return fn(*static_cast<HUL *>(lpv));
        case ppkEngine: return fn(*static_cast<ENGINE *>(lpv));
        case ppkScanner: return fn(*static_cast<SCANNER *>(lpv));
        case ppkBeam: return fn(*static_cast<BEAM *>(lpv));
        case ppkTorp: return fn(*static_cast<TORP *>(lpv));
        case ppkBomb: return fn(*static_cast<BOMB *>(lpv));
        case ppkShield: return fn(*static_cast<SHIELD *>(lpv));
        case ppkSpecial: return fn(*static_cast<SPECIAL *>(lpv));
        case ppkSpecialSB: return fn(*static_cast<SPECIALSB *>(lpv));
        case ppkMines: return fn(*static_cast<MINES *>(lpv));
        case ppkMining: return fn(*static_cast<MINING *>(lpv));
        case ppkPlanetary: return fn(*static_cast<PLANETARY *>(lpv));
        case ppkTerra: return fn(*static_cast<TERRA *>(lpv));
        }
        assert(false && "catalog borrow kind was not initialized");
        std::abort();
    }
public:
    NATIVEPARTCOMMONVIEW() noexcept : lpv(nullptr), ppk(ppkCommon) {}
    explicit operator bool() const noexcept { return lpv != nullptr; }
    const void *Lpv() const noexcept { return lpv; }
    friend bool operator==(NATIVEPARTCOMMONVIEW a, NATIVEPARTCOMMONVIEW b) noexcept { return a.lpv == b.lpv; }
    friend bool operator==(NATIVEPARTCOMMONVIEW a, const void *b) noexcept { return a.lpv == b; }
    int8_t *rgTech() const noexcept { return Visit([](auto &item) { return item.rgTech; }); }
    char *szName() const noexcept {
        return Visit([](auto &item) {
            if constexpr (std::is_same_v<std::remove_cvref_t<decltype(item)>, HUL>) return item.szClass;
            else return item.szName;
        });
    }
    int16_t cMass() const noexcept {
        return Visit([](auto &item) -> int16_t {
            if constexpr (std::is_same_v<std::remove_cvref_t<decltype(item)>, HUL>) return std::bit_cast<int16_t>(uint16_t{item.wtEmpty});
            else return item.cMass;
        });
    }
    uint16_t resCost() const noexcept { return Visit([](auto &item) { return item.resCost; }); }
    int16_t rgwtOreCost(int i) const noexcept {
        return Visit([i](auto &item) -> int16_t {
            if constexpr (std::is_same_v<std::remove_cvref_t<decltype(item)>, HUL>) return std::bit_cast<int16_t>(uint16_t{item.rgwtOreCost[i]});
            else return item.rgwtOreCost[i];
        });
    }
    int16_t ibmp() const noexcept { return Visit([](auto &item) { return item.ibmp; }); }
};

inline NATIVEPARTCOMMONVIEW PART::pcom() const noexcept { return {lpv, ppk}; }
inline void PART::SetPcom(NATIVEPARTCOMMONVIEW pcom) noexcept { lpv = pcom.lpv; ppk = pcom.ppk; }

static_assert(std::is_trivially_copyable_v<PART>);
#endif
