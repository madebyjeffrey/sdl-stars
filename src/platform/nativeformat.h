#ifndef STARS_NATIVEFORMAT_H
#define STARS_NATIVEFORMAT_H

// The Win32 facade can be included inside a caller's packed record scope.
// These transient argument values always use the formatter's native layout.
#pragma pack(push)
#pragma pack()

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstddef>
#include <cerrno>
#include <span>
#include <string_view>
#include <type_traits>
#include <variant>

// Values and text are borrowed only for the synchronous formatting call.
// Character pointers stay pointers until the conversion is known: %p must
// not attempt to read an arbitrary address as a string.
struct NATIVEFORMATARG {
    using COUNT = std::variant<signed char *, short *, int *, long *, long long *>;
    using VALUE = std::variant<std::int64_t, std::uint64_t, double, long double,
                               const char *, std::string_view, const wchar_t *, const void *, COUNT>;
    VALUE value;

    template<class T> requires std::is_integral_v<T>
    NATIVEFORMATARG(T v) : value(Numeric(v)) {}
    template<class T> requires std::is_enum_v<T>
    NATIVEFORMATARG(T v) : NATIVEFORMATARG(static_cast<std::underlying_type_t<T>>(v)) {}
    NATIVEFORMATARG(double v) : value(v) {}
    NATIVEFORMATARG(long double v) : value(v) {}
    NATIVEFORMATARG(const char *psz) : value(psz) {}
    NATIVEFORMATARG(char *psz) : value(static_cast<const char *>(psz)) {}
    NATIVEFORMATARG(std::nullptr_t) : value(static_cast<const char *>(nullptr)) {}
    NATIVEFORMATARG(std::string_view sz) : value(sz) {}
    template<std::size_t N>
    NATIVEFORMATARG(const std::array<char, N> &sz)
        : value(std::string_view(sz.data(), static_cast<std::size_t>(std::find(sz.begin(), sz.end(), 0) - sz.begin()))) {}
    template<class T> requires (std::is_object_v<T> || std::is_void_v<T>)
    NATIVEFORMATARG(T *lp) : value(Pointer(lp)) {}

private:
    template<class T> static VALUE Numeric(T v) {
        static_assert(sizeof(T) <= sizeof(std::uint64_t));
        if constexpr (std::is_signed_v<T>) return static_cast<std::int64_t>(v);
        else return static_cast<std::uint64_t>(v);
    }
    template<class T> static VALUE Pointer(T *lp) {
        if constexpr (std::is_same_v<std::remove_const_t<T>, wchar_t>) return static_cast<const wchar_t *>(lp);
        else if constexpr (std::is_same_v<T, signed char> || std::is_same_v<T, short> ||
                           std::is_same_v<T, int> || std::is_same_v<T, long> || std::is_same_v<T, long long>) return COUNT{lp};
        else return static_cast<const void *>(lp);
    }
};

// Standard printf widths and byte counts, with a typed argument list. The CRT
// still performs each conversion, retaining its locale and numeric rounding.
// Like snprintf, extra values are ignored and the return is the required count.
// Missing/incompatible values fail with EINVAL instead of reading C varargs.
int CchFormatNative(std::span<char> rgch, std::string_view szFormat,
                    std::span<const NATIVEFORMATARG> rgArgs) noexcept;
using NATIVEFORMATPROC = int (*)(std::span<char>, std::string_view, std::span<const NATIVEFORMATARG>);

template<class... ARGS>
int CchFormatNativeArgs(NATIVEFORMATPROC pfnFormat, std::span<char> rgch, const char *pszFormat, const ARGS &... args) {
    if (!pszFormat) {
        if (!rgch.empty()) rgch[0] = 0;
        errno = EINVAL;
        return -1;
    }
    const std::array<NATIVEFORMATARG, sizeof...(ARGS)> rgArgs = {NATIVEFORMATARG{args}...};
    return pfnFormat(rgch, std::string_view(pszFormat), rgArgs);
}

// A concrete typed provider keeps fault injection at the real format boundary.
struct NATIVEFORMATTER {
    NATIVEFORMATPROC pfnFormat;
    constexpr NATIVEFORMATTER(NATIVEFORMATPROC pfnFormat = CchFormatNative) : pfnFormat(pfnFormat) {}
    template<class... ARGS>
    int operator()(std::span<char> rgch, const char *pszFormat, ARGS... args) const {
        return CchFormatNativeArgs(pfnFormat, rgch, pszFormat, args...);
    }
};

#pragma pack(pop)
#endif
