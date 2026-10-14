#include "encoding.h"
#include <string.h>

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
        if (c == 0) break;
        if ((size_t)c > cchMax - i) break;
        i += c;
    }
    return i;
}

int FUtf8Valid(const char *psz, size_t cb) {
    size_t i = 0;
    uint32_t ch;
    int c;
    while (i < cb) {
        c = CbUtf8Char(psz + i, cb - i, &ch);
        if (c == 0 || ch == 0) return 0;
        i += c;
    }
    return 1;
}

int CchCopyUtf8(char *pszOut, size_t cbOut, const char *psz, size_t cb) {
    if (cbOut == 0) return -3;
    if (!FUtf8Valid(psz, cb)) { pszOut[0] = 0; return -1; }
    if (cb >= cbOut) { pszOut[0] = 0; return -3; }
    memmove(pszOut, psz, cb);
    pszOut[cb] = 0;
    return (int)cb;
}

int CchUtf16leToUtf8(char *pszOut, size_t cbOut, const unsigned char *rgb, size_t cch) {
    size_t i, cb = 0;
    uint32_t ch, chLow;
    int c;
    if (cbOut == 0) return -3;
    pszOut[0] = 0;
    for (i = 0; i < cch; i++) {
        ch = rgb[i * 2] | ((uint32_t)rgb[i * 2 + 1] << 8);
        if (ch >= 0xd800 && ch <= 0xdbff) {
            if (++i >= cch) goto malformed;
            chLow = rgb[i * 2] | ((uint32_t)rgb[i * 2 + 1] << 8);
            if (chLow < 0xdc00 || chLow > 0xdfff) goto malformed;
            ch = 0x10000 + ((ch - 0xd800) << 10) + chLow - 0xdc00;
        } else if (ch == 0 || (ch >= 0xdc00 && ch <= 0xdfff)) goto malformed;
        c = ch < 0x80 ? 1 : ch < 0x800 ? 2 : ch < 0x10000 ? 3 : 4;
        if ((size_t)c >= cbOut - cb) { pszOut[0] = 0; return -3; }
        if (c == 1) pszOut[cb++] = (char)ch;
        else {
            if (c == 4) pszOut[cb++] = (char)(0xf0 | (ch >> 18));
            else if (c == 3) pszOut[cb++] = (char)(0xe0 | (ch >> 12));
            else pszOut[cb++] = (char)(0xc0 | (ch >> 6));
            if (c == 4) pszOut[cb++] = (char)(0x80 | ((ch >> 12) & 63));
            if (c >= 3) pszOut[cb++] = (char)(0x80 | ((ch >> 6) & 63));
            pszOut[cb++] = (char)(0x80 | (ch & 63));
        }
    }
    pszOut[cb] = 0;
    return (int)cb;
malformed:
    pszOut[0] = 0;
    return -1;
}
