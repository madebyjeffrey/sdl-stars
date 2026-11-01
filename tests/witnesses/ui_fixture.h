#ifndef STARS_UI_FIXTURE_H
#define STARS_UI_FIXTURE_H
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <tuple>
#include <type_traits>
#include <utility>
#include "alert_io.h"
#include "colony_io.h"
#include "control_io.h"
#include "liststorage.h"
#include "part_query_io.h"
#include "planet_query_io.h"
#include "presentation_io.h"
#include "production_io.h"
#include "race_query_io.h"
#include "text_io.h"
#include "win.h"
inline HWND                   HwndWitness(int id) { return reinterpret_cast<HWND>(static_cast<intptr_t>(id)); }
template <class T, class U> T WitnessValue(U &&value) {
    using V = std::remove_reference_t<U>;
    if constexpr (std::is_pointer_v<T> && std::is_pointer_v<V>)
        return reinterpret_cast<T>(const_cast<void *>(reinterpret_cast<const void *>(value)));
    else if constexpr (std::is_pointer_v<T>)
        return reinterpret_cast<T>(static_cast<intptr_t>(value));
    else if constexpr (std::is_pointer_v<V>)
        return static_cast<T>(reinterpret_cast<intptr_t>(value));
    else
        return static_cast<T>(value);
}
template <class T> struct WITNESSPARAMS;
template <class R, class... A> struct WITNESSPARAMS<R (*)(A...)> {
    using Args = std::tuple<A...>;
    using Result = R;
};
template <auto pfn, class T> struct WITNESSADAPTER;
template <auto pfn, class R, class... A> struct WITNESSADAPTER<pfn, R (*)(A...)> {
    template <size_t... I> static auto Invoke(std::index_sequence<I...>, const std::tuple<A...> &args) {
        return pfn(WitnessValue<std::tuple_element_t<I, typename WITNESSPARAMS<decltype(pfn)>::Args>>(std::get<I>(args))...);
    }
    static R Call(A... args) {
        if constexpr (std::is_void_v<typename WITNESSPARAMS<decltype(pfn)>::Result>) {
            Invoke(std::make_index_sequence<std::tuple_size_v<typename WITNESSPARAMS<decltype(pfn)>::Args>>{}, std::tuple<A...>{args...});
            if constexpr (!std::is_void_v<R>)
                return WitnessValue<R>(1);
        } else {
            if constexpr (std::is_void_v<R>)
                Invoke(std::make_index_sequence<std::tuple_size_v<typename WITNESSPARAMS<decltype(pfn)>::Args>>{}, std::tuple<A...>{args...});
            else
                return WitnessValue<R>(
                    Invoke(std::make_index_sequence<std::tuple_size_v<typename WITNESSPARAMS<decltype(pfn)>::Args>>{}, std::tuple<A...>{args...}));
        }
    }
};
#endif
