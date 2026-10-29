#ifndef STARS_VALUE_H
#define STARS_VALUE_H
#include <type_traits>

// Preserve the conditional operator's usual conversions for mixed scalar types.
// Calls with observable operand evaluation remain explicit conditionals.
template<class A, class B> constexpr auto StarsMin(A a, B b) { using V = std::common_type_t<A, B>; return static_cast<V>(a) < static_cast<V>(b) ? static_cast<V>(a) : static_cast<V>(b); }
template<class A, class B> constexpr auto StarsMax(A a, B b) { using V = std::common_type_t<A, B>; return static_cast<V>(a) > static_cast<V>(b) ? static_cast<V>(a) : static_cast<V>(b); }

#endif
