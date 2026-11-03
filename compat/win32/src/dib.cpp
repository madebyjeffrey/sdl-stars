/* Bounded DIB decoding, shared by embedded resources and GDI input. */
#include "internal.h"
#include "render_io.h"
#include "resourcecodec.h"
#include <cstring>
#include <limits>

static bool FCompatDibInfo(const BITMAPINFOHEADER &bi, const BYTE *rgb, std::size_t cbInfo,
                           const BYTE *lpBits, std::size_t cbBits, COMPATDIB &dib) {
    std::size_t i;

    if (bi.biPlanes != 1 || bi.biCompression != win32::BI_RGB || bi.biWidth <= 0 ||
        bi.biHeight == 0 || bi.biHeight == std::numeric_limits<int>::min() ||
        (bi.biBitCount != 1 && bi.biBitCount != 4 && bi.biBitCount != 8 &&
         bi.biBitCount != 24 && bi.biBitCount != 32))
        return SDL_SetError("Unsupported DIB dimensions, compression or pixel format");
    // Bound allocation and work for malformed resource headers before touching bits.
    if (bi.biWidth > 16384 || bi.biHeight > 16384 || bi.biHeight < -16384 ||
        static_cast<std::uint64_t>(bi.biWidth) * static_cast<std::uint64_t>((bi.biHeight < 0 ? -bi.biHeight : bi.biHeight)) > std::uint64_t{67108864})
        return SDL_SetError("DIB exceeds the initial 16384-axis / 64M-pixel limit");
    dib = {};
    dib.cx = bi.biWidth;
    dib.cy = bi.biHeight < 0 ? -bi.biHeight : bi.biHeight;
    dib.fTopDown = bi.biHeight < 0;
    dib.cBits = bi.biBitCount;
    dib.cColors = bi.biBitCount <= 8 ? static_cast<int>((bi.biClrUsed ? bi.biClrUsed : 1u << bi.biBitCount)) : 0;
    if (bi.biClrUsed > 256 || (dib.cColors != 0 && dib.cColors > (1 << bi.biBitCount)) ||
        (bi.biBitCount > 8 && bi.biClrUsed != 0))
        return SDL_SetError("Invalid DIB color table");
    dib.cbInfo = cbWireDibHeader + static_cast<std::size_t>(dib.cColors) * cbWireRgbQuad;
    dib.cbRow = ((static_cast<std::size_t>(dib.cx) * dib.cBits + 31) / 32) * 4;
    dib.cbBits = dib.cbRow * static_cast<std::size_t>(dib.cy);
    if (cbInfo < dib.cbInfo || cbBits < dib.cbBits || !lpBits ||
        (bi.biSizeImage != 0 && bi.biSizeImage < dib.cbBits))
        return SDL_SetError("Truncated DIB color table or pixel buffer");
    for (i = 0; i < static_cast<std::size_t>(dib.cColors); i++)
        dib.rgcr[i] = win32::RGB(rgb[cbWireDibHeader + i * 4 + 2], rgb[cbWireDibHeader + i * 4 + 1], rgb[cbWireDibHeader + i * 4]);
    dib.lpBits = lpBits;
    return true;
}

bool FCompatDib(std::span<const BYTE> rgbInfo, std::span<const BYTE> rgbBits, COMPATDIB &dib) {
    BITMAPINFOHEADER bi;
    std::uint32_t cbHeader;

    if (rgbInfo.size() < 4) return false;
    NATIVEBYTEREADER(std::as_bytes(rgbInfo.first(4))).FReadU32Le(cbHeader);
    if (cbHeader != cbWireDibHeader || rgbInfo.size() < cbWireDibHeader)
        return SDL_SetError("DIB input requires a BITMAPINFOHEADER");
    FDecodeDibHeader(std::as_bytes(rgbInfo.first(cbWireDibHeader)), bi);
    return FCompatDibInfo(bi, rgbInfo.data(), rgbInfo.size(), rgbBits.data(), rgbBits.size(), dib);
}

// Genuine GDI callers supply native SDK headers/palette storage. Keep this
// native-layout seam separate from embedded-resource decoding.
bool FCompatNativeDib(const BITMAPINFOHEADER &bi, const BYTE *rgbInfo, const BYTE *lpBits, COMPATDIB &dib) {
    static_assert(sizeof(BITMAPINFOHEADER) == cbWireDibHeader && sizeof(RGBQUAD) == cbWireRgbQuad);
    if (bi.biSize != sizeof(bi))
        return SDL_SetError("DIB input requires a BITMAPINFOHEADER");
    return FCompatDibInfo(bi, rgbInfo, std::numeric_limits<std::size_t>::max(),
                          lpBits, std::numeric_limits<std::size_t>::max(), dib);
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
        rgb = lpdib->lpBits + static_cast<std::size_t>((lpdib->fTopDown ? y : lpdib->cy - 1 - y)) * lpdib->cbRow;
        lpout = static_cast<BYTE *>(lpsurface->pixels) + static_cast<std::size_t>(y) * lpsurface->pitch;
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
