/* Bounded DIB decoding, shared by embedded resources and GDI input. */
#include "internal.h"
#include "render_io.h"
#include "resourcecodec.h"
#include <limits.h>
#include <string.h>

static BOOL FCompatDibInfo(const BITMAPINFOHEADER &bi, const BYTE *rgb, size_t cbInfo,
                           const void *lpBits, size_t cbBits, COMPATDIB *lpdib) {
    size_t i;

    if (bi.biPlanes != 1 || bi.biCompression != win32::BI_RGB || bi.biWidth <= 0 ||
        bi.biHeight == 0 || bi.biHeight == INT_MIN ||
        (bi.biBitCount != 1 && bi.biBitCount != 4 && bi.biBitCount != 8 &&
         bi.biBitCount != 24 && bi.biBitCount != 32))
        return SDL_SetError("Unsupported DIB dimensions, compression or pixel format");
    // Bound allocation and work for malformed resource headers before touching bits.
    if (bi.biWidth > 16384 || bi.biHeight > 16384 || bi.biHeight < -16384 ||
        static_cast<uint64_t>(bi.biWidth) * static_cast<uint64_t>((bi.biHeight < 0 ? -bi.biHeight : bi.biHeight)) > UINT64_C(67108864))
        return SDL_SetError("DIB exceeds the initial 16384-axis / 64M-pixel limit");
    memset(lpdib, 0, sizeof(*lpdib));
    lpdib->cx = bi.biWidth;
    lpdib->cy = bi.biHeight < 0 ? -bi.biHeight : bi.biHeight;
    lpdib->fTopDown = bi.biHeight < 0;
    lpdib->cBits = bi.biBitCount;
    lpdib->cColors = bi.biBitCount <= 8 ? static_cast<int>((bi.biClrUsed ? bi.biClrUsed : 1u << bi.biBitCount)) : 0;
    if (bi.biClrUsed > 256 || (lpdib->cColors != 0 && lpdib->cColors > (1 << bi.biBitCount)) ||
        (bi.biBitCount > 8 && bi.biClrUsed != 0))
        return SDL_SetError("Invalid DIB color table");
    lpdib->cbInfo = cbWireDibHeader + static_cast<size_t>(lpdib->cColors) * cbWireRgbQuad;
    lpdib->cbRow = ((static_cast<size_t>(lpdib->cx) * lpdib->cBits + 31) / 32) * 4;
    lpdib->cbBits = lpdib->cbRow * static_cast<size_t>(lpdib->cy);
    if (cbInfo < lpdib->cbInfo || cbBits < lpdib->cbBits || !lpBits ||
        (bi.biSizeImage != 0 && bi.biSizeImage < lpdib->cbBits))
        return SDL_SetError("Truncated DIB color table or pixel buffer");
    for (i = 0; i < static_cast<size_t>(lpdib->cColors); i++)
        lpdib->rgcr[i] = win32::RGB(rgb[cbWireDibHeader + i * 4 + 2], rgb[cbWireDibHeader + i * 4 + 1], rgb[cbWireDibHeader + i * 4]);
    lpdib->lpBits = static_cast<const BYTE *>((lpBits));
    return 1;
}

BOOL FCompatDib(const void *lpInfo, size_t cbInfo, const void *lpBits, size_t cbBits, COMPATDIB *lpdib) {
    BITMAPINFOHEADER bi;
    std::uint32_t cbHeader;
    const auto *rgb = static_cast<const std::byte *>(lpInfo);

    if (!rgb || !lpdib || cbInfo < 4) return 0;
    NATIVEBYTEREADER(std::span(rgb, 4)).FReadU32Le(cbHeader);
    if (cbHeader != cbWireDibHeader || cbInfo < cbWireDibHeader)
        return SDL_SetError("DIB input requires a BITMAPINFOHEADER");
    FDecodeDibHeader(std::span(rgb, cbWireDibHeader), bi);
    return FCompatDibInfo(bi, static_cast<const BYTE *>(lpInfo), cbInfo, lpBits, cbBits, lpdib);
}

// Genuine GDI callers supply native SDK headers/palette storage. Keep this
// native-layout seam separate from embedded-resource decoding.
BOOL FCompatNativeDib(const void *lpInfo, size_t cbInfo, const void *lpBits, size_t cbBits, COMPATDIB *lpdib) {
    BITMAPINFOHEADER bi;
    DWORD cbHeader;

    static_assert(sizeof(BITMAPINFOHEADER) == cbWireDibHeader && sizeof(RGBQUAD) == cbWireRgbQuad);
    if (!lpInfo || !lpdib || cbInfo < sizeof(cbHeader)) return 0;
    memcpy(&cbHeader, lpInfo, sizeof(cbHeader));
    if (cbHeader != sizeof(bi) || cbInfo < sizeof(bi))
        return SDL_SetError("DIB input requires a BITMAPINFOHEADER");
    memcpy(&bi, lpInfo, sizeof(bi));
    return FCompatDibInfo(bi, static_cast<const BYTE *>(lpInfo), cbInfo, lpBits, cbBits, lpdib);
}

SDL_Surface *LpCompatDibSurface(const COMPATDIB *lpdib) {
    SDL_Surface *lpsurface;
    NATIVEOWNER<SDL_Surface, FreeCompatSurface> lpsurfaceOwner;
    int x, y, iColor;
    COLORREF cr;
    const BYTE *rgb;
    BYTE *lpout;

    lpsurfaceOwner.reset(SDL_CreateSurface(lpdib->cx, lpdib->cy, SDL_PIXELFORMAT_RGBA32)); lpsurface = lpsurfaceOwner.get();
    if (!lpsurface)
        return nullptr;
    for (y = 0; y < lpdib->cy; y++) {
        rgb = lpdib->lpBits + static_cast<size_t>((lpdib->fTopDown ? y : lpdib->cy - 1 - y)) * lpdib->cbRow;
        lpout = static_cast<BYTE *>(lpsurface->pixels) + static_cast<size_t>(y) * lpsurface->pitch;
        for (x = 0; x < lpdib->cx; x++) {
            if (lpdib->cBits <= 8) {
                iColor = lpdib->cBits == 1 ? (rgb[x / 8] >> (7 - x % 8)) & 1 :
                         lpdib->cBits == 4 ? (rgb[x / 2] >> (x % 2 == 0 ? 4 : 0)) & 15 : rgb[x];
                if (iColor >= lpdib->cColors) {
                    lpsurfaceOwner.reset();
                    SDL_SetError("DIB pixel index is outside its color table");
                    return nullptr;
                }
                cr = lpdib->rgcr[iColor];
            } else {
                cr = win32::RGB(rgb[x * (lpdib->cBits / 8) + 2], rgb[x * (lpdib->cBits / 8) + 1], rgb[x * (lpdib->cBits / 8)]);
            }
            lpout[x * 4] = static_cast<BYTE>(cr);
            lpout[x * 4 + 1] = static_cast<BYTE>((cr >> 8));
            lpout[x * 4 + 2] = static_cast<BYTE>((cr >> 16));
            lpout[x * 4 + 3] = 255;
        }
    }
    return lpsurfaceOwner.release();
}
