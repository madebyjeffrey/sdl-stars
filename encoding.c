#include "encoding.h"
#include <string.h>

static const uint16_t rgchCp1252[32] = {
    0x20ac,0x0081,0x201a,0x0192,0x201e,0x2026,0x2020,0x2021,
    0x02c6,0x2030,0x0160,0x2039,0x0152,0x008d,0x017d,0x008f,
    0x0090,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,
    0x02dc,0x2122,0x0161,0x203a,0x0153,0x009d,0x017e,0x0178
};

int CbUtf8Char(const char *psz, size_t cb, uint32_t *lpch) {
    const unsigned char *rgb = (const unsigned char *)psz;
    uint32_t ch;
    int c, i;
    if (cb == 0) return 0;
    if (rgb[0] < 0x80) { *lpch = rgb[0]; return 1; }
    if (rgb[0] >= 0xc2 && rgb[0] <= 0xdf) { c = 2; ch = rgb[0] & 0x1f; }
    else if (rgb[0] >= 0xe0 && rgb[0] <= 0xef) { c = 3; ch = rgb[0] & 15; }
    else if (rgb[0] >= 0xf0 && rgb[0] <= 0xf4) { c = 4; ch = rgb[0] & 7; }
    else return 0;
    if (cb < (size_t)c) return 0;
    for (i = 1; i < c; i++) {
        if ((rgb[i] & 0xc0) != 0x80) return 0;
        ch = (ch << 6) | (rgb[i] & 0x3f);
    }
    if ((c == 3 && ch < 0x800) || (c == 4 && ch < 0x10000) || ch > 0x10ffff ||
        (ch >= 0xd800 && ch <= 0xdfff)) return 0;
    *lpch = ch;
    return c;
}

size_t CchUtf8Prefix(const char *psz, size_t cchMax) {
    size_t cb = strlen(psz), i = 0;
    uint32_t ch;
    int c;
    while (i < cb && i < cchMax) {
        c = CbUtf8Char(psz + i, cb - i, &ch);
        // POSIX allows arbitrary filename bytes; preserve malformed bytes.
        if (c == 0) c = 1;
        if ((size_t)c > cchMax - i) break;
        i += c;
    }
    return i;
}

int CchUtf8ToLegacy(char *pszOut, size_t cbOut, const char *psz, size_t cb) {
    size_t i = 0, cch = 0, j;
    uint32_t ch;
    int c, result;
    if (cbOut == 0) return -3;
    pszOut[0] = 0;
    while (i < cb) {
        c = CbUtf8Char(psz + i, cb - i, &ch);
        if (c == 0 || ch == 0) { result = -1; goto fail; }
        i += c;
        if (ch < 0x80 || (ch >= 0xa0 && ch <= 0xff)) j = ch;
        else {
            for (j = 0; j < 32 && rgchCp1252[j] != ch; j++) {}
            if (j == 32) { result = -2; goto fail; }
            j += 0x80;
        }
        if (cch + 1 >= cbOut) { result = -3; goto fail; }
        pszOut[cch++] = (char)j;
    }
    pszOut[cch] = 0;
    return (int)cch;
fail:
    pszOut[0] = 0;
    return result;
}

int CchLegacyToUtf8(char *pszOut, size_t cbOut, const char *psz, size_t cb) {
    size_t i, cch = 0;
    uint32_t ch;
    unsigned char b;
    int c;
    if (cbOut == 0) return -3;
    pszOut[0] = 0;
    for (i = 0; i < cb; i++) {
        b = (unsigned char)psz[i];
        ch = b >= 0x80 && b < 0xa0 ? rgchCp1252[b - 0x80] : b;
        if (ch == 0) { pszOut[0] = 0; return -1; }
        c = ch < 0x80 ? 1 : ch < 0x800 ? 2 : 3;
        if ((size_t)c >= cbOut - cch) { pszOut[0] = 0; return -3; }
        if (c == 1) pszOut[cch++] = (char)ch;
        else if (c == 2) {
            pszOut[cch++] = (char)(0xc0 | (ch >> 6)); pszOut[cch++] = (char)(0x80 | (ch & 63));
        } else {
            pszOut[cch++] = (char)(0xe0 | (ch >> 12)); pszOut[cch++] = (char)(0x80 | ((ch >> 6) & 63));
            pszOut[cch++] = (char)(0x80 | (ch & 63));
        }
    }
    pszOut[cch] = 0;
    return (int)cch;
}
