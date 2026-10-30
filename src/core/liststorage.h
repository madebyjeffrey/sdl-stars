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
    static_assert(sizeof(T) % alignof(TItem) == 0);
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_default_constructible_v<T>);
    static_assert(std::is_trivially_default_constructible_v<TItem> && std::is_trivially_destructible_v<TItem>);
    std::byte rgbHeader[sizeof(T)];
    std::memcpy(rgbHeader, lp, sizeof(T));
    T *lppl = ::new (lp) T;
    std::memcpy(lppl, rgbHeader, sizeof(T));
    ::new (static_cast<void *>(static_cast<std::byte *>(lp) + sizeof(T))) TItem[(cb - sizeof(T)) / sizeof(TItem)];
    return lppl;
}

#endif
