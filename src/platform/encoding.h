#ifndef STARS_ENCODING_H
#define STARS_ENCODING_H
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string_view>
// All decoded application text and supported paths are UTF-8.
// Strict copies/conversions return bytes written, -1 malformed or -3 capacity.
// A failed operation leaves an empty destination.
int CchCp1252ToUtf8(std::span<char> rgchOut, std::span<const std::byte> rgb);
int CchUtf16leToUtf8(std::span<char> rgchOut, std::span<const std::byte> rgb);
struct UTF8CHAR { std::uint32_t ch; std::size_t cch; };
std::optional<UTF8CHAR> DecodeUtf8Char(std::string_view sz) noexcept;
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
inline bool FUtf8Valid(std::string_view sz) {
    std::size_t i = 0;
    while (i < sz.size()) {
        const auto uch = DecodeUtf8Char(sz.substr(i));
        if (!uch || uch->ch == 0) return false;
        i += uch->cch;
    }
    return true;
}
inline int CchCopyUtf8(std::span<char> rgchOut, std::string_view sz) {
    if (rgchOut.empty()) return -3;
    if (!FUtf8Valid(sz)) { rgchOut[0] = 0; return -1; }
    if (sz.size() >= rgchOut.size()) { rgchOut[0] = 0; return -3; }
    if (!sz.empty()) std::memmove(rgchOut.data(), sz.data(), sz.size());
    rgchOut[sz.size()] = 0;
    return static_cast<int>(sz.size());
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
