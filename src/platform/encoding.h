#ifndef STARS_ENCODING_H
#define STARS_ENCODING_H
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
// All decoded application text and supported paths are UTF-8.
// Strict copies/conversions return bytes written, -1 malformed or -3 capacity.
// A failed operation leaves an empty destination.
int FUtf8Valid(const char *psz, std::size_t cb);
int CchCopyUtf8(char *pszOut, std::size_t cbOut, const char *psz, std::size_t cb);
int CchCp1252ToUtf8(char *pszOut, std::size_t cbOut, const unsigned char *rgb, std::size_t cb);
int CchUtf16leToUtf8(char *pszOut, std::size_t cbOut, const unsigned char *rgb, std::size_t cch);
int CbUtf8Char(const char *psz, std::size_t cb, std::uint32_t *lpch);
std::size_t CchUtf8Prefix(const char *psz, std::size_t cchMax);
// Display/fixed-label copies write a terminated complete UTF-8 prefix and
// return bytes copied/appended. These are not strict path/record operations.
// An empty span is untouched; appending to an unterminated span clears it.
// Views are borrowed for this call only and may overlap the destination.
std::size_t CchCopyUtf8Prefix(std::span<char> rgchOut, std::string_view sz);
std::size_t CchAppendUtf8Prefix(std::span<char> rgchOut, std::string_view sz);

// Views borrow only for this call. They can describe a non-NUL-terminated
// slice; text still rejects embedded NUL and output capacity includes its NUL.
inline int FUtf8Valid(std::string_view sz) { return FUtf8Valid(sz.data(), sz.size()); }
inline int CbUtf8Char(std::string_view sz, std::uint32_t *lpch) { return CbUtf8Char(sz.data(), sz.size(), lpch); }
inline int CchCopyUtf8(std::span<char> rgchOut, std::string_view sz) {
    if (rgchOut.empty()) return -3;
    if (sz.empty()) { rgchOut[0] = 0; return 0; }
    return CchCopyUtf8(rgchOut.data(), rgchOut.size(), sz.data(), sz.size());
}
inline int CchCp1252ToUtf8(std::span<char> rgchOut, std::span<const std::byte> rgb) {
    return CchCp1252ToUtf8(rgchOut.data(), rgchOut.size(),
                          reinterpret_cast<const unsigned char *>(rgb.data()), rgb.size());
}
inline int CchUtf16leToUtf8(std::span<char> rgchOut, std::span<const std::byte> rgb) {
    if (rgchOut.empty()) return -3;
    if (rgb.size() % 2 != 0) { rgchOut[0] = 0; return -1; }
    return CchUtf16leToUtf8(rgchOut.data(), rgchOut.size(),
                           reinterpret_cast<const unsigned char *>(rgb.data()), rgb.size() / 2);
}
inline std::size_t CchUtf8Prefix(std::string_view sz, std::size_t cchMax) {
    std::size_t i = 0;
    std::uint32_t ch;
    int c;
    while (i < sz.size() && i < cchMax) {
        c = CbUtf8Char(sz.substr(i), &ch);
        if (c == 0 || ch == 0 || static_cast<std::size_t>(c) > cchMax - i) break;
        i += c;
    }
    return i;
}

#endif
