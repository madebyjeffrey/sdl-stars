#ifndef STARS_LISTSTORAGE_H
#define STARS_LISTSTORAGE_H

#include <cassert>
#include <cstddef>
#include <cstring>
#include <new>
#include <span>
#include <type_traits>

// Reuse the pool's complete physical slot, including rounded spare capacity.
// C++20 lifetime operations start real objects and retain their representations.
// Header reserved bits and the allocator's allocation/failure policy survive.
template<class T, class TItem>
T *LpStartPl(std::span<std::byte> rgb) noexcept {
    static_assert(sizeof(T) % alignof(TItem) == 0);
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_default_constructible_v<T>);
    static_assert(std::is_trivially_copyable_v<TItem> && std::is_trivially_default_constructible_v<TItem>);
    assert(rgb.size() >= sizeof(T));
    std::byte rgbHeader[sizeof(T)];
    std::memcpy(rgbHeader, rgb.data(), sizeof(T));
    T *lppl = ::new (rgb.data()) T;
    std::memcpy(lppl, rgbHeader, sizeof(T));
    auto rgbItems = rgb.subspan(sizeof(T));
    std::memmove(rgbItems.data(), rgbItems.data(), rgbItems.size());
    return lppl;
}

#endif
