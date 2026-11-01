#ifndef STARS_ENCODING_H
#define STARS_ENCODING_H
#include <stddef.h>
#include <stdint.h>
#include <cstddef>
#include <span>
#include <string_view>
// All decoded application text and supported paths are UTF-8.
// Strict copies/conversions return bytes written, -1 malformed or -3 capacity.
// A failed operation leaves an empty destination.
int FUtf8Valid(const char *psz, size_t cb);
int CchCopyUtf8(char *pszOut, size_t cbOut, const char *psz, size_t cb);
int CchCp1252ToUtf8(char *pszOut, size_t cbOut, const unsigned char *rgb, size_t cb);
int CchUtf16leToUtf8(char *pszOut, size_t cbOut, const unsigned char *rgb, size_t cch);
int CbUtf8Char(const char *psz, size_t cb, uint32_t *lpch);
size_t CchUtf8Prefix(const char *psz, size_t cchMax);

// Views borrow only for this call. They can describe a non-NUL-terminated
// slice; text still rejects embedded NUL and output capacity includes its NUL.
inline int FUtf8Valid(std::string_view sz) { return FUtf8Valid(sz.data(), sz.size()); }
inline int CbUtf8Char(std::string_view sz, uint32_t *lpch) { return CbUtf8Char(sz.data(), sz.size(), lpch); }
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
inline size_t CchUtf8Prefix(std::string_view sz, size_t cchMax) {
    size_t i = 0;
    uint32_t ch;
    int c;
    while (i < sz.size() && i < cchMax) {
        c = CbUtf8Char(sz.substr(i), &ch);
        if (c == 0 || ch == 0 || static_cast<size_t>(c) > cchMax - i) break;
        i += c;
    }
    return i;
}

#endif
