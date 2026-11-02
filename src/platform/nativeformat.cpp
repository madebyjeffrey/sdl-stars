#include "nativeformat.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <limits>
#include <new>
#include <string>
#include <utility>

namespace {
enum class LENGTH { normal, hh, h, l, ll, j, z, t, L };

int InvalidFormat() { errno = EINVAL; return -1; }

template<class T> bool FInteger(const NATIVEFORMATARG &arg, T &v) {
    const auto *pl = std::get_if<std::int64_t>(&arg.value);
    const auto *pul = std::get_if<std::uint64_t>(&arg.value);
    if (pl) { v = static_cast<T>(*pl); return true; }
    if (pul) { v = static_cast<T>(*pul); return true; }
    return false;
}
template<class T> int CchInteger(std::span<char> rgch, const char *pszFormat, const NATIVEFORMATARG &arg) {
    T v;
    if (!FInteger(arg, v)) return InvalidFormat();
    return std::snprintf(rgch.data(), rgch.size(), pszFormat, v);
}
template<class T> int CchFloat(std::span<char> rgch, const char *pszFormat, const NATIVEFORMATARG &arg) {
    const auto *pd = std::get_if<double>(&arg.value);
    const auto *pdl = std::get_if<long double>(&arg.value);
    if (pd) return std::snprintf(rgch.data(), rgch.size(), pszFormat, static_cast<T>(*pd));
    if (pdl) return std::snprintf(rgch.data(), rgch.size(), pszFormat, static_cast<T>(*pdl));
    return InvalidFormat();
}
template<class T> int StoreCount(const NATIVEFORMATARG &arg, int cch) {
    const auto *count = std::get_if<NATIVEFORMATARG::COUNT>(&arg.value);
    const auto *pp = count ? std::get_if<T *>(count) : nullptr;
    if (!pp || !*pp) return InvalidFormat();
    // Use a read-only CRT format: fortified runtimes reject %n in writable
    // format storage. The typed destination has already been validated.
    if constexpr (std::is_same_v<T, signed char>) return std::snprintf(nullptr, 0, "%*s%hhn", cch, "", *pp) < 0 ? -1 : 0;
    else if constexpr (std::is_same_v<T, short>) return std::snprintf(nullptr, 0, "%*s%hn", cch, "", *pp) < 0 ? -1 : 0;
    else if constexpr (std::is_same_v<T, int>) return std::snprintf(nullptr, 0, "%*s%n", cch, "", *pp) < 0 ? -1 : 0;
    else if constexpr (std::is_same_v<T, long>) return std::snprintf(nullptr, 0, "%*s%ln", cch, "", *pp) < 0 ? -1 : 0;
    else return std::snprintf(nullptr, 0, "%*s%lln", cch, "", *pp) < 0 ? -1 : 0;
}
int CchCount(const NATIVEFORMATARG &arg, LENGTH mdLength, int cch) {
    switch (mdLength) {
    case LENGTH::hh: return StoreCount<signed char>(arg, cch);
    case LENGTH::h: return StoreCount<short>(arg, cch);
    case LENGTH::normal: return StoreCount<int>(arg, cch);
    case LENGTH::l: return StoreCount<long>(arg, cch);
    case LENGTH::ll: return StoreCount<long long>(arg, cch);
    case LENGTH::j: return StoreCount<std::intmax_t>(arg, cch);
    case LENGTH::z: return StoreCount<std::make_signed_t<std::size_t>>(arg, cch);
    case LENGTH::t: return StoreCount<std::ptrdiff_t>(arg, cch);
    default: return InvalidFormat();
    }
}

int CchConvert(std::span<char> rgch, std::string szFormat, const NATIVEFORMATARG &arg,
               LENGTH mdLength, char chType, int cch) {
    const bool fSigned = chType == 'd' || chType == 'i';
    const auto *ppsz = std::get_if<const char *>(&arg.value);
    const auto *psz = std::get_if<std::string_view>(&arg.value);
    const auto *ppwsz = std::get_if<const wchar_t *>(&arg.value);
    if (fSigned || chType == 'o' || chType == 'u' || chType == 'x' || chType == 'X') {
        switch (mdLength) {
        case LENGTH::normal: case LENGTH::h: case LENGTH::hh:
            return fSigned ? CchInteger<int>(rgch, szFormat.c_str(), arg) : CchInteger<unsigned>(rgch, szFormat.c_str(), arg);
        case LENGTH::l:
            return fSigned ? CchInteger<long>(rgch, szFormat.c_str(), arg) : CchInteger<unsigned long>(rgch, szFormat.c_str(), arg);
        case LENGTH::ll:
            return fSigned ? CchInteger<long long>(rgch, szFormat.c_str(), arg) : CchInteger<unsigned long long>(rgch, szFormat.c_str(), arg);
        case LENGTH::j:
            return fSigned ? CchInteger<std::intmax_t>(rgch, szFormat.c_str(), arg) : CchInteger<std::uintmax_t>(rgch, szFormat.c_str(), arg);
        case LENGTH::z:
            return fSigned ? CchInteger<std::make_signed_t<std::size_t>>(rgch, szFormat.c_str(), arg) : CchInteger<std::size_t>(rgch, szFormat.c_str(), arg);
        case LENGTH::t:
            return fSigned ? CchInteger<std::ptrdiff_t>(rgch, szFormat.c_str(), arg) : CchInteger<std::make_unsigned_t<std::ptrdiff_t>>(rgch, szFormat.c_str(), arg);
        default: return InvalidFormat();
        }
    }
    if (std::strchr("fFeEgGaA", chType)) {
        if (mdLength == LENGTH::L) return CchFloat<long double>(rgch, szFormat.c_str(), arg);
        if (mdLength == LENGTH::normal || mdLength == LENGTH::l) return CchFloat<double>(rgch, szFormat.c_str(), arg);
        return InvalidFormat();
    }
    if (chType == 'c') {
        if (mdLength == LENGTH::normal) return CchInteger<int>(rgch, szFormat.c_str(), arg);
        if (mdLength == LENGTH::l) return CchInteger<std::wint_t>(rgch, szFormat.c_str(), arg);
        return InvalidFormat();
    }
    if (chType == 's') {
        if (mdLength == LENGTH::normal) {
            if (ppsz)
                return std::snprintf(rgch.data(), rgch.size(), szFormat.c_str(), *ppsz);
            if (psz) {
                // A view need not have a trailing NUL. Keep %s's embedded-NUL
                // and byte-precision rules while owning its CRT staging copy.
                const std::string sz(*psz);
                return std::snprintf(rgch.data(), rgch.size(), szFormat.c_str(), sz.c_str());
            }
        } else if (mdLength == LENGTH::l) {
            if (ppwsz) return std::snprintf(rgch.data(), rgch.size(), szFormat.c_str(), *ppwsz);
        }
        return InvalidFormat();
    }
    if (chType == 'p' && mdLength == LENGTH::normal) {
        return std::visit([&](const auto &v) -> int {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_pointer_v<T>)
                return std::snprintf(rgch.data(), rgch.size(), szFormat.c_str(), static_cast<const void *>(v));
            else if constexpr (std::is_same_v<T, NATIVEFORMATARG::COUNT>)
                return std::visit([&](auto lp) { return std::snprintf(rgch.data(), rgch.size(), szFormat.c_str(), static_cast<const void *>(lp)); }, v);
            else return InvalidFormat();
        }, arg.value);
    }
    if (chType == 'n') return CchCount(arg, mdLength, cch);
    return InvalidFormat();
}

bool FDigit(char ch) { return ch >= '0' && ch <= '9'; }
int CchFormat(std::span<char> rgch, std::string_view szFormat, std::span<const NATIVEFORMATARG> rgArgs) {
    std::size_t i = 0, iArg = 0, start;
    int cch = 0, dx, precision, cchPart;
    char chType;
    LENGTH mdLength;
    if (!rgch.empty()) rgch[0] = 0;
    auto append = [&](std::string_view sz) -> bool {
        std::size_t cb;
        if (sz.size() > static_cast<std::size_t>(std::numeric_limits<int>::max() - cch)) { errno = EOVERFLOW; return false; }
        if (static_cast<std::size_t>(cch) < rgch.size()) {
            cb = std::min(sz.size(), rgch.size() - static_cast<std::size_t>(cch) - 1);
            std::memmove(rgch.data() + cch, sz.data(), cb);
            rgch[cch + cb] = 0;
        }
        cch += static_cast<int>(sz.size());
        return true;
    };
    while (i < szFormat.size() && szFormat[i] != 0) {
        if (szFormat[i] != '%') {
            start = i;
            while (i < szFormat.size() && szFormat[i] != '%' && szFormat[i] != 0) i++;
            if (!append(szFormat.substr(start, i - start))) return -1;
            continue;
        }
        i++;
        if (i < szFormat.size() && szFormat[i] == '%') {
            if (!append("%")) return -1;
            i++;
            continue;
        }
        std::string szSpec = "%";
        while (i < szFormat.size() && szFormat[i] != 0 && std::strchr("-+ #0'", szFormat[i])) szSpec += szFormat[i++];
        if (i < szFormat.size() && szFormat[i] == '*') {
            if (iArg == rgArgs.size() || !FInteger(rgArgs[iArg++], dx)) return InvalidFormat();
            // Use a signed 64-bit magnitude so INT_MIN never overflows here.
            if (dx < 0) szSpec += '-';
            szSpec += std::to_string(dx < 0 ? -static_cast<std::int64_t>(dx) : dx);
            i++;
        } else while (i < szFormat.size() && FDigit(szFormat[i])) szSpec += szFormat[i++];
        if (i < szFormat.size() && szFormat[i] == '.') {
            i++;
            if (i < szFormat.size() && szFormat[i] == '*') {
                if (iArg == rgArgs.size() || !FInteger(rgArgs[iArg++], precision)) return InvalidFormat();
                if (precision >= 0) szSpec += '.' + std::to_string(precision);
                i++;
            } else {
                szSpec += '.';
                while (i < szFormat.size() && FDigit(szFormat[i])) szSpec += szFormat[i++];
            }
        }
        mdLength = LENGTH::normal;
        if (i < szFormat.size()) {
            switch (szFormat[i]) {
            case 'h':
                szSpec += szFormat[i++]; mdLength = LENGTH::h;
                if (i < szFormat.size() && szFormat[i] == 'h') { szSpec += szFormat[i++]; mdLength = LENGTH::hh; }
                break;
            case 'l':
                szSpec += szFormat[i++]; mdLength = LENGTH::l;
                if (i < szFormat.size() && szFormat[i] == 'l') { szSpec += szFormat[i++]; mdLength = LENGTH::ll; }
                break;
            case 'j': mdLength = LENGTH::j; szSpec += szFormat[i++]; break;
            case 'z': mdLength = LENGTH::z; szSpec += szFormat[i++]; break;
            case 't': mdLength = LENGTH::t; szSpec += szFormat[i++]; break;
            case 'L': mdLength = LENGTH::L; szSpec += szFormat[i++]; break;
            default: break;
            }
        }
        if (i == szFormat.size() || szFormat[i] == 0 || iArg == rgArgs.size()) return InvalidFormat();
        chType = szFormat[i++];
        szSpec += chType;
        const auto out = static_cast<std::size_t>(cch) < rgch.size() ? rgch.subspan(cch) : std::span<char>{};
        cchPart = CchConvert(out, std::move(szSpec), rgArgs[iArg++], mdLength, chType, cch);
        if (cchPart < 0) return -1;
        if (cchPart > std::numeric_limits<int>::max() - cch) { errno = EOVERFLOW; return -1; }
        cch += cchPart;
    }
    return cch;
}
}

int CchFormatNative(std::span<char> rgch, std::string_view szFormat, std::span<const NATIVEFORMATARG> rgArgs) noexcept {
    try { return CchFormat(rgch, szFormat, rgArgs); }
    catch (const std::bad_alloc &) { errno = ENOMEM; }
    catch (...) { errno = EINVAL; }
    return -1;
}
