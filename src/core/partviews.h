#ifndef STARS_PARTVIEWS_H
#define STARS_PARTVIEWS_H

#include "structs.h"
#include <algorithm>
#include <string_view>
#include <type_traits>
#include <variant>

template<std::size_t cch>
static std::string_view SzvPartName(const char (&sz)[cch]) noexcept {
    const char *pchEnd = std::find(sz, sz + cch, '\0');
    return {sz, static_cast<std::size_t>(pchEnd - sz)};
}

// Snapshot the selected borrow, not its PART carrier or the catalog values.
// Callbacks may replace the carrier; catalog reads must still use this object.
class NATIVEPARTCOMMONVIEW {
    friend struct _part;
    PARTTARGET lpv;
    explicit NATIVEPARTCOMMONVIEW(PARTTARGET lpv) noexcept : lpv(lpv) {}
    template<class F> decltype(auto) Visit(F fn) const noexcept {
        return std::visit([fn](auto *lpItem) -> decltype(auto) { return fn(*lpItem); }, lpv);
    }
public:
    NATIVEPARTCOMMONVIEW() noexcept : lpv(static_cast<COMPART *>(nullptr)) {}
    explicit operator bool() const noexcept { return std::visit([](auto *lpItem) { return lpItem != nullptr; }, lpv); }
    friend bool operator==(NATIVEPARTCOMMONVIEW a, NATIVEPARTCOMMONVIEW b) noexcept { return a.lpv == b.lpv; }
    template<class T> friend bool operator==(NATIVEPARTCOMMONVIEW a, T *b) noexcept {
        const auto *lpPointer = std::get_if<T *>(&a.lpv);
        return lpPointer && *lpPointer == b;
    }
    const std::int8_t *rgTech() const noexcept { return Visit([](auto &item) { return item.rgTech; }); }
    std::string_view szName() const noexcept {
        return Visit([](auto &item) {
            if constexpr (std::is_same_v<std::remove_cvref_t<decltype(item)>, HUL>) return SzvPartName(item.szClass);
            else return SzvPartName(item.szName);
        });
    }
    std::int16_t cMass() const noexcept {
        return Visit([](auto &item) -> std::int16_t {
            if constexpr (std::is_same_v<std::remove_cvref_t<decltype(item)>, HUL>) return std::bit_cast<std::int16_t>(std::uint16_t{item.wtEmpty});
            else return item.cMass;
        });
    }
    std::uint16_t resCost() const noexcept { return Visit([](auto &item) { return item.resCost; }); }
    std::int16_t rgwtOreCost(int i) const noexcept {
        return Visit([i](auto &item) -> std::int16_t {
            if constexpr (std::is_same_v<std::remove_cvref_t<decltype(item)>, HUL>) return std::bit_cast<std::int16_t>(std::uint16_t{item.rgwtOreCost[i]});
            else return item.rgwtOreCost[i];
        });
    }
    std::int16_t ibmp() const noexcept { return Visit([](auto &item) { return item.ibmp; }); }
};

inline NATIVEPARTCOMMONVIEW PART::pcom() const noexcept { return NATIVEPARTCOMMONVIEW{lpv}; }
inline void PART::SetPcom(NATIVEPARTCOMMONVIEW pcom) noexcept { lpv = pcom.lpv; }

static_assert(std::is_trivially_copyable_v<PART>);
#endif
