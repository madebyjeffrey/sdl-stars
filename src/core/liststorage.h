#ifndef STARS_LISTSTORAGE_H
#define STARS_LISTSTORAGE_H

#include <cstddef>
#include <cstring>
#include <new>
#include <type_traits>

// Reuse the pool's complete physical slot, including rounded spare capacity.
// C++20 lifetime operations start real objects and retain their representations.
// Header reserved bits and the allocator's allocation/failure policy survive.
template<class T, class TItem>
T *LpStartPl(void *lp, std::size_t cb) noexcept {
    static_assert(sizeof(T) % alignof(TItem) == 0);
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_default_constructible_v<T>);
    static_assert(std::is_trivially_copyable_v<TItem> && std::is_trivially_default_constructible_v<TItem>);
    std::byte rgbHeader[sizeof(T)];
    std::memcpy(rgbHeader, lp, sizeof(T));
    T *lppl = ::new (lp) T;
    std::memcpy(lppl, rgbHeader, sizeof(T));
    void *lpItems = static_cast<std::byte *>(lp) + sizeof(T);
    std::memmove(lpItems, lpItems, cb - sizeof(T));
    return lppl;
}

#endif
