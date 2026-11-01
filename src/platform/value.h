#ifndef STARS_VALUE_H
#define STARS_VALUE_H
#include <type_traits>
#include <cstdint>

// By-value scalar arguments also accept the original packed bit-field expressions.
template<class T> constexpr std::uintptr_t UlpValueBits(T value) {
    if constexpr (std::is_pointer_v<T>)
        return reinterpret_cast<std::uintptr_t>(value);
    else if constexpr (std::is_null_pointer_v<T>)
        return 0;
    else
        return static_cast<std::uintptr_t>(value);
}
template<class T> constexpr std::uint16_t LOWORD(T value) { return static_cast<std::uint16_t>(UlpValueBits(value)); }
template<class T> constexpr std::uint16_t HIWORD(T value) { return static_cast<std::uint16_t>(UlpValueBits(value) >> 16); }
template<class T> constexpr std::uint8_t LOBYTE(T value) { return static_cast<std::uint8_t>(UlpValueBits(value)); }
template<class T> constexpr std::uint8_t HIBYTE(T value) { return static_cast<std::uint8_t>(UlpValueBits(value) >> 8); }
template<class A, class B> constexpr std::int32_t MAKELONG(A lo, B hi) {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(LOWORD(lo)) |
                                (static_cast<std::uint32_t>(LOWORD(hi)) << 16));
}

// Preserve the conditional operator's usual conversions for mixed scalar types.
// Calls with observable operand evaluation remain explicit conditionals.
template<class A, class B> constexpr auto StarsMin(A a, B b) { using V = std::common_type_t<A, B>; return static_cast<V>(a) < static_cast<V>(b) ? static_cast<V>(a) : static_cast<V>(b); }
template<class A, class B> constexpr auto StarsMax(A a, B b) { using V = std::common_type_t<A, B>; return static_cast<V>(a) > static_cast<V>(b) ? static_cast<V>(a) : static_cast<V>(b); }

#endif
