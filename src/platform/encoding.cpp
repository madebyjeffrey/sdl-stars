#include "encoding.h"
#include <algorithm>
#include <cstring>

std::optional<UTF8CHAR> DecodeUtf8Char(std::string_view sz) noexcept {
    const auto *rgb = reinterpret_cast<const unsigned char *>(sz.data());
    std::uint32_t ch;
    std::size_t c;
    std::size_t i;
    if (sz.empty()) return std::nullopt;
    if (rgb[0] < 0x80) return UTF8CHAR{rgb[0], 1};
    if (rgb[0] >= 0xc2 && rgb[0] <= 0xdf) { c = 2; ch = rgb[0] & 0x1f; }
    else if (rgb[0] >= 0xe0 && rgb[0] <= 0xef) { c = 3; ch = rgb[0] & 15; }
    else if (rgb[0] >= 0xf0 && rgb[0] <= 0xf4) { c = 4; ch = rgb[0] & 7; }
    else return std::nullopt;
    if (sz.size() < c) return std::nullopt;
    for (i = 1; i < c; i++) {
        if ((rgb[i] & 0xc0) != 0x80) return std::nullopt;
        ch = (ch << 6) | (rgb[i] & 0x3f);
    }
    if ((c == 3 && ch < 0x800) || (c == 4 && ch < 0x10000) || ch > 0x10ffff ||
        (ch >= 0xd800 && ch <= 0xdfff)) return std::nullopt;
    return UTF8CHAR{ch, c};
}

bool FAppendUtf8(std::span<char> rgchOut, std::string_view sz) {
    const auto pchEnd = std::find(rgchOut.begin(), rgchOut.end(), 0);
    std::size_t cch;

    if (pchEnd == rgchOut.end()) return false;
    cch = static_cast<std::size_t>(pchEnd - rgchOut.begin());
    if (!FUtf8Valid(std::string_view(rgchOut.data(), cch)) || !FUtf8Valid(sz) || sz.size() >= rgchOut.size() - cch) return false;
    if (!sz.empty()) std::memmove(rgchOut.data() + cch, sz.data(), sz.size());
    rgchOut[cch + sz.size()] = 0;
    return true;
}

std::size_t CchCopyUtf8Prefix(std::span<char> rgchOut, std::string_view sz) {
    std::size_t cch;
    if (rgchOut.empty()) return 0;
    cch = CchUtf8Prefix(sz, rgchOut.size() - 1);
    if (cch != 0) std::memmove(rgchOut.data(), sz.data(), cch);
    rgchOut[cch] = 0;
    return cch;
}

std::size_t CchAppendUtf8Prefix(std::span<char> rgchOut, std::string_view sz) {
    const auto pchEnd = std::find(rgchOut.begin(), rgchOut.end(), 0);
    if (pchEnd == rgchOut.end()) {
        if (!rgchOut.empty()) rgchOut[0] = 0;
        return 0;
    }
    return CchCopyUtf8Prefix(rgchOut.subspan(static_cast<std::size_t>(pchEnd - rgchOut.begin())), sz);
}

int CchCp1252ToUtf8(std::span<char> rgchOut, std::span<const std::byte> rgb) {
    static const std::uint16_t rgwCp1252[32] = {
        0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
        0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x017d, 0x008f,
        0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
        0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178
    };
    std::size_t i, cOut = 0;
    std::uint32_t ch;
    int c;

    if (rgchOut.empty()) return -3;
    rgchOut[0] = 0;
    for (i = 0; i < rgb.size(); i++) {
        ch = std::to_integer<unsigned char>(rgb[i]);
        if (ch == 0) goto malformed;
        if (ch >= 0x80 && ch < 0xa0) ch = rgwCp1252[ch - 0x80];
        c = ch < 0x80 ? 1 : ch < 0x800 ? 2 : 3;
        if (static_cast<std::size_t>(c) >= rgchOut.size() - cOut) { rgchOut[0] = 0; return -3; }
        if (c == 1) rgchOut[cOut++] = static_cast<char>(ch);
        else if (c == 2) {
            rgchOut[cOut++] = static_cast<char>((0xc0 | (ch >> 6)));
            rgchOut[cOut++] = static_cast<char>((0x80 | (ch & 0x3f)));
        } else {
            rgchOut[cOut++] = static_cast<char>((0xe0 | (ch >> 12)));
            rgchOut[cOut++] = static_cast<char>((0x80 | ((ch >> 6) & 0x3f)));
            rgchOut[cOut++] = static_cast<char>((0x80 | (ch & 0x3f)));
        }
    }
    rgchOut[cOut] = 0;
    return static_cast<int>(cOut);
malformed:
    rgchOut[0] = 0;
    return -1;
}

int CchUtf16leToUtf8(std::span<char> rgchOut, std::span<const std::byte> rgb) {
    std::size_t i, cb = 0;
    std::uint32_t ch, chLow;
    int c;
    if (rgchOut.empty()) return -3;
    rgchOut[0] = 0;
    if (rgb.size() % 2 != 0) goto malformed;
    for (i = 0; i < rgb.size() / 2; i++) {
        ch = std::to_integer<unsigned char>(rgb[i * 2]) |
             (static_cast<std::uint32_t>(std::to_integer<unsigned char>(rgb[i * 2 + 1])) << 8);
        if (ch >= 0xd800 && ch <= 0xdbff) {
            if (++i >= rgb.size() / 2) goto malformed;
            chLow = std::to_integer<unsigned char>(rgb[i * 2]) |
                    (static_cast<std::uint32_t>(std::to_integer<unsigned char>(rgb[i * 2 + 1])) << 8);
            if (chLow < 0xdc00 || chLow > 0xdfff) goto malformed;
            ch = 0x10000 + ((ch - 0xd800) << 10) + chLow - 0xdc00;
        } else if (ch == 0 || (ch >= 0xdc00 && ch <= 0xdfff)) goto malformed;
        c = ch < 0x80 ? 1 : ch < 0x800 ? 2 : ch < 0x10000 ? 3 : 4;
        if (static_cast<std::size_t>(c) >= rgchOut.size() - cb) { rgchOut[0] = 0; return -3; }
        if (c == 1) rgchOut[cb++] = static_cast<char>(ch);
        else {
            if (c == 4) rgchOut[cb++] = static_cast<char>((0xf0 | (ch >> 18)));
            else if (c == 3) rgchOut[cb++] = static_cast<char>((0xe0 | (ch >> 12)));
            else rgchOut[cb++] = static_cast<char>((0xc0 | (ch >> 6)));
            if (c == 4) rgchOut[cb++] = static_cast<char>((0x80 | ((ch >> 12) & 63)));
            if (c >= 3) rgchOut[cb++] = static_cast<char>((0x80 | ((ch >> 6) & 63)));
            rgchOut[cb++] = static_cast<char>((0x80 | (ch & 63)));
        }
    }
    rgchOut[cb] = 0;
    return static_cast<int>(cb);
malformed:
    rgchOut[0] = 0;
    return -1;
}
