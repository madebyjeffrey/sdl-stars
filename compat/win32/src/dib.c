/* Bounded DIB decoding, shared by embedded resources and GDI input. */
#include "internal.h"
#include <limits.h>
#include <string.h>

BOOL FCompatDib(const void *lpInfo, size_t cbInfo, const void *lpBits, size_t cbBits, COMPATDIB *lpdib) {
    BITMAPINFOHEADER bi;
    DWORD cbHeader;
    size_t i;
    const BYTE *rgb = (const BYTE *)(lpInfo);

    if (!rgb || !lpdib || cbInfo < sizeof(cbHeader))
        return FALSE;
    memcpy(&cbHeader, rgb, sizeof(cbHeader));
    if (cbHeader != sizeof(bi) || cbInfo < sizeof(bi))
        return SDL_SetError("DIB input requires a BITMAPINFOHEADER");
    memcpy(&bi, rgb, sizeof(bi));
    if (bi.biPlanes != 1 || bi.biCompression != BI_RGB || bi.biWidth <= 0 ||
        bi.biHeight == 0 || bi.biHeight == INT_MIN ||
        (bi.biBitCount != 1 && bi.biBitCount != 4 && bi.biBitCount != 8 &&
         bi.biBitCount != 24 && bi.biBitCount != 32))
        return SDL_SetError("Unsupported DIB dimensions, compression or pixel format");
    // Bound allocation and work for malformed resource headers before touching bits.
    if (bi.biWidth > 16384 || bi.biHeight > 16384 || bi.biHeight < -16384 ||
        (uint64_t)bi.biWidth * (uint64_t)(bi.biHeight < 0 ? -bi.biHeight : bi.biHeight) > UINT64_C(67108864))
        return SDL_SetError("DIB exceeds the initial 16384-axis / 64M-pixel limit");
    memset(lpdib, 0, sizeof(*lpdib));
    lpdib->cx = bi.biWidth;
    lpdib->cy = bi.biHeight < 0 ? -bi.biHeight : bi.biHeight;
    lpdib->fTopDown = bi.biHeight < 0;
    lpdib->cBits = bi.biBitCount;
    lpdib->cColors = bi.biBitCount <= 8 ? (int)(bi.biClrUsed ? bi.biClrUsed : 1u << bi.biBitCount) : 0;
    if (bi.biClrUsed > 256 || (lpdib->cColors != 0 && lpdib->cColors > (1 << bi.biBitCount)) ||
        (bi.biBitCount > 8 && bi.biClrUsed != 0))
        return SDL_SetError("Invalid DIB color table");
    lpdib->cbInfo = sizeof(bi) + (size_t)lpdib->cColors * sizeof(RGBQUAD);
    lpdib->cbRow = (((size_t)lpdib->cx * lpdib->cBits + 31) / 32) * 4;
    lpdib->cbBits = lpdib->cbRow * (size_t)lpdib->cy;
    if (cbInfo < lpdib->cbInfo || cbBits < lpdib->cbBits || !lpBits ||
        (bi.biSizeImage != 0 && bi.biSizeImage < lpdib->cbBits))
        return SDL_SetError("Truncated DIB color table or pixel buffer");
    for (i = 0; i < (size_t)lpdib->cColors; i++)
        lpdib->rgcr[i] = RGB(rgb[sizeof(bi) + i * 4 + 2], rgb[sizeof(bi) + i * 4 + 1], rgb[sizeof(bi) + i * 4]);
    lpdib->lpBits = (const BYTE *)(lpBits);
    return TRUE;
}

SDL_Surface *LpCompatDibSurface(const COMPATDIB *lpdib) {
    SDL_Surface *lpsurface;
    int x, y, iColor;
    COLORREF cr;
    const BYTE *rgb;
    BYTE *lpout;

    lpsurface = SDL_CreateSurface(lpdib->cx, lpdib->cy, SDL_PIXELFORMAT_RGBA32);
    if (!lpsurface)
        return NULL;
    for (y = 0; y < lpdib->cy; y++) {
        rgb = lpdib->lpBits + (size_t)(lpdib->fTopDown ? y : lpdib->cy - 1 - y) * lpdib->cbRow;
        lpout = (BYTE *)lpsurface->pixels + (size_t)y * lpsurface->pitch;
        for (x = 0; x < lpdib->cx; x++) {
            if (lpdib->cBits <= 8) {
                iColor = lpdib->cBits == 1 ? (rgb[x / 8] >> (7 - x % 8)) & 1 :
                         lpdib->cBits == 4 ? (rgb[x / 2] >> (x % 2 == 0 ? 4 : 0)) & 15 : rgb[x];
                if (iColor >= lpdib->cColors) {
                    SDL_DestroySurface(lpsurface);
                    SDL_SetError("DIB pixel index is outside its color table");
                    return NULL;
                }
                cr = lpdib->rgcr[iColor];
            } else {
                cr = RGB(rgb[x * (lpdib->cBits / 8) + 2], rgb[x * (lpdib->cBits / 8) + 1], rgb[x * (lpdib->cBits / 8)]);
            }
            lpout[x * 4] = (BYTE)cr;
            lpout[x * 4 + 1] = (BYTE)(cr >> 8);
            lpout[x * 4 + 2] = (BYTE)(cr >> 16);
            lpout[x * 4 + 3] = 255;
        }
    }
    return lpsurface;
}
