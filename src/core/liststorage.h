#ifndef STARS_LISTSTORAGE_H
#define STARS_LISTSTORAGE_H

#include <cstddef>
#include <cstring>
#include <new>
#include <type_traits>

// Reuse the pool's complete physical slot, including rounded spare capacity.
// Default initialization starts real objects without clearing retained bytes.
// Header reserved bits and the allocator's allocation/failure policy survive.
template<class T, class TItem>
T *LpStartPl(void *lp, std::size_t cb) noexcept {
    static_assert(4 % alignof(TItem) == 0);
    static_assert(sizeof(T) == 4 && std::is_trivially_copyable_v<T> && std::is_trivially_default_constructible_v<T>);
    static_assert(std::is_trivially_default_constructible_v<TItem> && std::is_trivially_destructible_v<TItem>);
    std::byte rgbHeader[4];
    std::memcpy(rgbHeader, lp, 4);
    T *lppl = ::new (lp) T;
    std::memcpy(lppl, rgbHeader, 4);
    ::new (static_cast<void *>(static_cast<std::byte *>(lp) + 4)) TItem[(cb - 4) / sizeof(TItem)];
    return lppl;
}

#endif
