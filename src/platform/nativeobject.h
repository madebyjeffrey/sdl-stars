#ifndef STARS_NATIVEOBJECT_H
#define STARS_NATIVEOBJECT_H

#include <cassert>
#include <cstddef>
#include <cstring>
#include <new>
#include <type_traits>

// Runtime object storage, never a wire image. C++20 memmove creates implicit-
// lifetime objects in its destination; self-copy retains every physical byte.
// A different As<T>() invalidates old typed borrows. Prefix access does not
// switch the enclosing native object, including during nested UI callbacks.
template<std::size_t cbStorage, std::size_t cbAlign>
class alignas(cbAlign) NATIVEOBJECTSTORAGE {
    static_assert(cbStorage > 0);
    mutable std::byte rgbStorage[cbStorage];
    template<class T> T *LpBegin() const noexcept {
        static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_default_constructible_v<T>);
        static_assert(sizeof(T) <= cbStorage && alignof(T) <= cbAlign && cbAlign % alignof(T) == 0);
        return std::launder(static_cast<T *>(std::memmove(rgbStorage, rgbStorage, sizeof(T))));
    }
public:
    template<class T> T &As() noexcept { return *LpBegin<T>(); }
    template<class T> const T &As() const noexcept { return *LpBegin<T>(); }
    template<class T> T ReadPrefix(std::size_t ib) const noexcept {
        static_assert(std::is_integral_v<T>);
        assert(ib <= cbStorage && sizeof(T) <= cbStorage - ib);
        T value;
        std::memcpy(&value, rgbStorage + ib, sizeof(value));
        return value;
    }
    template<class T> void WritePrefix(std::size_t ib, T value) noexcept {
        static_assert(std::is_integral_v<T>);
        assert(ib <= cbStorage && sizeof(T) <= cbStorage - ib);
        std::memcpy(rgbStorage + ib, &value, sizeof(value));
    }
};

#endif
