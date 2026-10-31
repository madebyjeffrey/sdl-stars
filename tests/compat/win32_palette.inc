/* Logical palettes on true-color DCs, shared with the Windows SDK. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

int main(void) {
    struct { WORD version, count; PALETTEENTRY rgpe[3]; } pal = {
        0x300, 3, {{12, 34, 56, 0}, {210, 180, 90, 0}, {40, 80, 120, 0}}
    };
    struct { BITMAPINFOHEADER bi; WORD rgi[2]; } info = {
        {sizeof(BITMAPINFOHEADER), 2, 1, 1, 1, BI_RGB, 4, 0, 0, 2, 0}, {1, 0}
    };
    BYTE rgbBits[8] = {0x40, 0, 0, 0};
    PALETTEENTRY rgpe[4] = {0};
    HDC hscreen, hdc, hdcOther;
    HPALETTE hpal, hpalOther, hold, holdOther;
    HBITMAP hbm, hbmOther;
    HGDIOBJ hbmOld, hbmOtherOld, hpenOld;
    HBRUSH hbr, hbrPattern;
    HPEN hpen;
    WORD c = 0;
    RECT rc = {0, 0, 2, 2};
    COLORREF cr;
    int i;
    const WORD rgcBits[] = {1, 4, 8};

    hscreen = GetDC(NULL); CHECK(hscreen);
    CHECK(!(GetDeviceCaps(hscreen, RASTERCAPS) & RC_PALETTE));
    hdc = CreateCompatibleDC(hscreen); hdcOther = CreateCompatibleDC(hscreen);
    hbm = CreateCompatibleBitmap(hscreen, 2, 2); hbmOther = CreateCompatibleBitmap(hscreen, 2, 2);
    CHECK(hdc && hdcOther && hbm && hbmOther);
    hbmOld = SelectObject(hdc, hbm); CHECK(hbmOld);
    hbmOtherOld = SelectObject(hdcOther, hbmOther); CHECK(hbmOtherOld);
    hpal = CreatePalette((const LOGPALETTE *)&pal); CHECK(hpal);
    CHECK(GetObjectA(hpal, sizeof(c), &c) == sizeof(c) && c == 3);
    CHECK(GetPaletteEntries(hpal, 0, 0, NULL) == 3);
    CHECK(GetPaletteEntries(hpal, 1, 4, rgpe) == 2 && rgpe[0].peRed == 210 && rgpe[1].peBlue == 120);
    pal.rgpe[0].peRed = 200;
    hpalOther = CreatePalette((const LOGPALETTE *)&pal); CHECK(hpalOther);
    CHECK(GetPaletteEntries(hpal, 0, 1, rgpe) == 1 && rgpe[0].peRed == 12);
    hold = SelectPalette(hdc, hpal, 0); holdOther = SelectPalette(hdcOther, hpalOther, 1);
    CHECK(hold && holdOther && hold == (HPALETTE)GetStockObject(DEFAULT_PALETTE));
    CHECK(GetPaletteEntries(hold, 0, 0, NULL) == 20);
    CHECK(GetPaletteEntries(hold, 0, 1, rgpe) == 1 && rgpe[0].peRed == 0 && rgpe[0].peGreen == 0 && rgpe[0].peBlue == 0);
    CHECK(GetPaletteEntries(hold, 19, 1, rgpe) == 1 && rgpe[0].peRed == 255 && rgpe[0].peGreen == 255 && rgpe[0].peBlue == 255);
    CHECK(RealizePalette(hdc) == 0 && RealizePalette(hdcOther) == 0);
    CHECK(UnrealizeObject(hpal) && RealizePalette(hdc) == 0);
    hbr = CreateSolidBrush(PALETTEINDEX(0)); CHECK(hbr);
    CHECK(FillRect(hdc, &rc, hbr) && GetPixel(hdc, 0, 0) == RGB(12, 34, 56));
    CHECK(SelectPalette(hdc, hpalOther, 1) == hpal);
    CHECK(FillRect(hdc, &rc, hbr) && GetPixel(hdc, 0, 0) == RGB(200, 34, 56));
    CHECK(SelectPalette(hdc, hpal, 0) == hpalOther);
    CHECK(FillRect(hdcOther, &rc, hbr) && GetPixel(hdcOther, 0, 0) == RGB(200, 34, 56));
    CHECK(SetPixel(hdc, 0, 0, PALETTEINDEX(1)) == RGB(210, 180, 90));
    CHECK(SetPixel(hdc, 1, 0, PALETTERGB(17, 18, 19)) == RGB(17, 18, 19));
    hpen = CreatePen(PS_SOLID, 1, PALETTEINDEX(2)); CHECK(hpen);
    hpenOld = SelectObject(hdc, hpen); CHECK(hpenOld);
    CHECK(MoveToEx(hdc, 0, 1, NULL) && LineTo(hdc, 2, 1));
    CHECK(GetPixel(hdc, 0, 1) == RGB(40, 80, 120));
    CHECK(SelectObject(hdc, hpenOld) == hpen && DeleteObject(hpen));
    CHECK(UnrealizeObject(hbr) && FillRect(hdc, &rc, hbr));
    CHECK(SetBkColor(hdc, PALETTEINDEX(1)) != CLR_INVALID);
    CHECK(GetBkColor(hdc) == PALETTEINDEX(1));
    CHECK(ExtTextOutA(hdc, 0, 0, ETO_OPAQUE, &rc, "", 0, NULL));
    CHECK(GetPixel(hdc, 1, 1) == RGB(210, 180, 90));
    CHECK(StretchDIBits(hdc, 0, 0, 2, 1, 0, 0, 2, 1, rgbBits, (const BITMAPINFO *)&info, DIB_PAL_COLORS, SRCCOPY) == 1);
    CHECK(GetPixel(hdc, 0, 0) == RGB(210, 180, 90) && GetPixel(hdc, 1, 0) == RGB(12, 34, 56));
    info.bi.biHeight = -2; info.bi.biSizeImage = 8;
    for (i = 0; i < 3; i++) {
        info.bi.biBitCount = rgcBits[i];
        memset(rgbBits, 0, sizeof(rgbBits));
        if (rgcBits[i] == 1) { rgbBits[0] = 0x40; rgbBits[4] = 0x80; }
        else if (rgcBits[i] == 4) { rgbBits[0] = 0x01; rgbBits[4] = 0x10; }
        else { rgbBits[1] = 1; rgbBits[4] = 1; }
        CHECK(StretchDIBits(hdc, 0, 0, 2, 2, 0, 0, 2, 2, rgbBits, (const BITMAPINFO *)&info, DIB_PAL_COLORS, SRCCOPY) == 2);
        CHECK(GetPixel(hdc, 0, 0) == RGB(210, 180, 90) && GetPixel(hdc, 1, 0) == RGB(12, 34, 56));
        CHECK(GetPixel(hdc, 0, 1) == RGB(12, 34, 56) && GetPixel(hdc, 1, 1) == RGB(210, 180, 90));
    }
    hbrPattern = CreatePatternBrush(hbm); CHECK(hbrPattern);
    CHECK(SetBrushOrgEx(hdcOther, 1, 0, NULL) && FillRect(hdcOther, &rc, hbrPattern));
    cr = GetPixel(hdcOther, 0, 0); CHECK(cr == RGB(12, 34, 56));
    CHECK(UnrealizeObject(hbrPattern) && FillRect(hdcOther, &rc, hbrPattern));
    CHECK(GetPixel(hdcOther, 0, 0) == cr && DeleteObject(hbrPattern));
    info.bi.biBitCount = 1; info.bi.biHeight = 1; info.bi.biSizeImage = 4; rgbBits[0] = 0x40;
    // Changing the palette affects the next conversion, not already drawn RGB pixels.
    CHECK(SelectPalette(hdc, hpalOther, 0) == hpal);
    CHECK(GetPixel(hdc, 1, 0) == RGB(12, 34, 56));
    CHECK(StretchDIBits(hdc, 0, 0, 2, 1, 0, 0, 2, 1, rgbBits, (const BITMAPINFO *)&info, DIB_PAL_COLORS, SRCCOPY) == 1);
    CHECK(GetPixel(hdc, 1, 0) == RGB(200, 34, 56));
    CHECK(SelectPalette(hdcOther, hpal, 0) == hpalOther);
    CHECK(SelectPalette(hdc, hpal, 0) == hpalOther);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(!DeleteObject(hpal));
    CHECK(!SelectPalette(hdc, (HPALETTE)hbr, 0));
    CHECK(RealizePalette(NULL) == GDI_ERROR);
    CHECK(!CreatePalette(NULL));
    pal.version = 0x200; CHECK(!CreatePalette((const LOGPALETTE *)&pal)); pal.version = 0x300;
    CHECK(!UnrealizeObject(hbm));
    info.rgi[1] = 3;
    CHECK(!StretchDIBits(hdc, 0, 0, 2, 1, 0, 0, 2, 1, rgbBits, (const BITMAPINFO *)&info, DIB_PAL_COLORS, SRCCOPY));
    CHECK(GetPixel(hdc, 1, 0) == RGB(200, 34, 56));
#endif
    CHECK(SelectPalette(hdc, hold, 0) == hpal);
    CHECK(SelectPalette(hdcOther, holdOther, 0) == hpal);
    CHECK(DeleteObject(hpal) && DeleteObject(hpalOther) && DeleteObject(hbr));
    CHECK(SelectObject(hdc, hbmOld) == hbm && DeleteObject(hbm));
    CHECK(SelectObject(hdcOther, hbmOtherOld) == hbmOther && DeleteObject(hbmOther));
    CHECK(DeleteDC(hdc) && DeleteDC(hdcOther) && ReleaseDC(NULL, hscreen));
    cr = RGB(1, 2, 3); CHECK(PALETTERGB(1, 2, 3) == (cr | 0x02000000));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(!UnrealizeObject(hpal) && !GetPaletteEntries(hpal, 0, 1, rgpe));
    StarsCompatShutdown();
#endif
    puts("Palette ownership, DC selection, true-color drawing and indexed DIB input passed");
    return 0;
}
