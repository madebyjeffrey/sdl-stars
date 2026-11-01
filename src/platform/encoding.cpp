#include "encoding.h"
#include <cstring>

int CbUtf8Char(const char *psz, std::size_t cb, std::uint32_t *lpch) {
    const unsigned char *rgb = reinterpret_cast<const unsigned char *>(psz);
    std::uint32_t ch;
    int c, i;
    if (cb == 0) return 0;
    if (rgb[0] < 0x80) { *lpch = rgb[0]; return 1; }
    if (rgb[0] >= 0xc2 && rgb[0] <= 0xdf) { c = 2; ch = rgb[0] & 0x1f; }
    else if (rgb[0] >= 0xe0 && rgb[0] <= 0xef) { c = 3; ch = rgb[0] & 15; }
    else if (rgb[0] >= 0xf0 && rgb[0] <= 0xf4) { c = 4; ch = rgb[0] & 7; }
    else return 0;
    if (cb < static_cast<std::size_t>(c)) return 0;
    for (i = 1; i < c; i++) {
        if ((rgb[i] & 0xc0) != 0x80) return 0;
        ch = (ch << 6) | (rgb[i] & 0x3f);
    }
    if ((c == 3 && ch < 0x800) || (c == 4 && ch < 0x10000) || ch > 0x10ffff ||
        (ch >= 0xd800 && ch <= 0xdfff)) return 0;
    *lpch = ch;
    return c;
}

std::size_t CchUtf8Prefix(const char *psz, std::size_t cchMax) {
    std::size_t cb = std::strlen(psz), i = 0;
    std::uint32_t ch;
    int c;
    while (i < cb && i < cchMax) {
        c = CbUtf8Char(psz + i, cb - i, &ch);
        if (c == 0) break;
        if (static_cast<std::size_t>(c) > cchMax - i) break;
        i += c;
    }
    return i;
}

int FUtf8Valid(const char *psz, std::size_t cb) {
    std::size_t i = 0;
    std::uint32_t ch;
    int c;
    while (i < cb) {
        c = CbUtf8Char(psz + i, cb - i, &ch);
        if (c == 0 || ch == 0) return 0;
        i += c;
    }
    return 1;
}

int CchCopyUtf8(char *pszOut, std::size_t cbOut, const char *psz, std::size_t cb) {
    if (cbOut == 0) return -3;
    if (!FUtf8Valid(psz, cb)) { pszOut[0] = 0; return -1; }
    if (cb >= cbOut) { pszOut[0] = 0; return -3; }
    std::memmove(pszOut, psz, cb);
    pszOut[cb] = 0;
    return static_cast<int>(cb);
}

int CchCp1252ToUtf8(char *pszOut, std::size_t cbOut, const unsigned char *rgb, std::size_t cb) {
    static const std::uint16_t rgwCp1252[32] = {
        0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
        0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x017d, 0x008f,
        0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
        0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178
    };
    std::size_t i, cOut = 0;
    std::uint32_t ch;
    int c;

    if (cbOut == 0) return -3;
    pszOut[0] = 0;
    for (i = 0; i < cb; i++) {
        ch = rgb[i];
        if (ch == 0) goto malformed;
        if (ch >= 0x80 && ch < 0xa0) ch = rgwCp1252[ch - 0x80];
        c = ch < 0x80 ? 1 : ch < 0x800 ? 2 : 3;
        if (static_cast<std::size_t>(c) >= cbOut - cOut) { pszOut[0] = 0; return -3; }
        if (c == 1) pszOut[cOut++] = static_cast<char>(ch);
        else if (c == 2) {
            pszOut[cOut++] = static_cast<char>((0xc0 | (ch >> 6)));
            pszOut[cOut++] = static_cast<char>((0x80 | (ch & 0x3f)));
        } else {
            pszOut[cOut++] = static_cast<char>((0xe0 | (ch >> 12)));
            pszOut[cOut++] = static_cast<char>((0x80 | ((ch >> 6) & 0x3f)));
            pszOut[cOut++] = static_cast<char>((0x80 | (ch & 0x3f)));
        }
    }
    pszOut[cOut] = 0;
    return static_cast<int>(cOut);
malformed:
    pszOut[0] = 0;
    return -1;
}

int CchUtf16leToUtf8(char *pszOut, std::size_t cbOut, const unsigned char *rgb, std::size_t cch) {
    std::size_t i, cb = 0;
    std::uint32_t ch, chLow;
    int c;
    if (cbOut == 0) return -3;
    pszOut[0] = 0;
    for (i = 0; i < cch; i++) {
        ch = rgb[i * 2] | (static_cast<std::uint32_t>(rgb[i * 2 + 1]) << 8);
        if (ch >= 0xd800 && ch <= 0xdbff) {
            if (++i >= cch) goto malformed;
            chLow = rgb[i * 2] | (static_cast<std::uint32_t>(rgb[i * 2 + 1]) << 8);
            if (chLow < 0xdc00 || chLow > 0xdfff) goto malformed;
            ch = 0x10000 + ((ch - 0xd800) << 10) + chLow - 0xdc00;
        } else if (ch == 0 || (ch >= 0xdc00 && ch <= 0xdfff)) goto malformed;
        c = ch < 0x80 ? 1 : ch < 0x800 ? 2 : ch < 0x10000 ? 3 : 4;
        if (static_cast<std::size_t>(c) >= cbOut - cb) { pszOut[0] = 0; return -3; }
        if (c == 1) pszOut[cb++] = static_cast<char>(ch);
        else {
            if (c == 4) pszOut[cb++] = static_cast<char>((0xf0 | (ch >> 18)));
            else if (c == 3) pszOut[cb++] = static_cast<char>((0xe0 | (ch >> 12)));
            else pszOut[cb++] = static_cast<char>((0xc0 | (ch >> 6)));
            if (c == 4) pszOut[cb++] = static_cast<char>((0x80 | ((ch >> 12) & 63)));
            if (c >= 3) pszOut[cb++] = static_cast<char>((0x80 | ((ch >> 6) & 63)));
            pszOut[cb++] = static_cast<char>((0x80 | (ch & 63)));
        }
    }
    pszOut[cb] = 0;
    return static_cast<int>(cb);
malformed:
    pszOut[0] = 0;
    return -1;
}
