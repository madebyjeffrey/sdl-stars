#ifndef STARS_ENCODING_H
#define STARS_ENCODING_H
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
// All decoded application text and supported paths are UTF-8.
// Strict copies/conversions return bytes written, -1 malformed or -3 capacity.
// A failed operation leaves an empty destination.
int FUtf8Valid(const char *psz, std::size_t cb);
int CchCopyUtf8(char *pszOut, std::size_t cbOut, const char *psz, std::size_t cb);
int CchCp1252ToUtf8(char *pszOut, std::size_t cbOut, const unsigned char *rgb, std::size_t cb);
int CchUtf16leToUtf8(char *pszOut, std::size_t cbOut, const unsigned char *rgb, std::size_t cch);
struct UTF8CHAR { std::uint32_t ch; std::size_t cch; };
std::optional<UTF8CHAR> DecodeUtf8Char(std::string_view sz) noexcept;
std::size_t CchUtf8Prefix(const char *psz, std::size_t cchMax);
// Display/fixed-label copies write a terminated complete UTF-8 prefix and
// return bytes copied/appended. These are not strict path/record operations.
// An empty span is untouched; appending to an unterminated span clears it.
// Views are borrowed for this call only and may overlap the destination.
std::size_t CchCopyUtf8Prefix(std::span<char> rgchOut, std::string_view sz);
std::size_t CchAppendUtf8Prefix(std::span<char> rgchOut, std::string_view sz);

// Append whole valid UTF-8 text within the destination's NUL-inclusive span.
// Both the existing prefix and input must be valid. Failure leaves every byte
// unchanged; input may overlap the destination, and an empty span cannot succeed.
bool FAppendUtf8(std::span<char> rgchOut, std::string_view sz);

// Views borrow only for this call. They can describe a non-NUL-terminated
// slice; text still rejects embedded NUL and output capacity includes its NUL.
inline int FUtf8Valid(std::string_view sz) { return FUtf8Valid(sz.data(), sz.size()); }
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
    while (i < sz.size() && i < cchMax) {
        const auto uch = DecodeUtf8Char(sz.substr(i));
        if (!uch || uch->ch == 0 || uch->cch > cchMax - i) break;
        i += uch->cch;
    }
    return i;
}

#endif
