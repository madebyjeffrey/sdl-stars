/* Canonical icon/cursor resources, pixel masks and handle lifetimes vs SDK. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "image_manifest.h"
#ifndef _WIN32
#include <stars_compat.h>
#include "internal.h"
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static WORD WTest(const BYTE *rgb) { return (WORD)(rgb[0] | (rgb[1] << 8)); }
static DWORD DwTest(const BYTE *rgb) { return (DWORD)rgb[0] | ((DWORD)rgb[1] << 8) | ((DWORD)rgb[2] << 16) | ((DWORD)rgb[3] << 24); }
static COLORREF CrExpected(const BYTE *rgb, BOOL fCursor, int x, int y, COLORREF bg) {
    UINT bits, colors, row, iColor;
    const BYTE *pixels, *palette, *mask;
    COLORREF cr;
    rgb += fCursor ? 4 : 0;
    bits = WTest(rgb + 14); colors = DwTest(rgb + 32);
    if (colors == 0 && bits <= 8) colors = 1u << bits;
    palette = rgb + 40; row = ((32 * bits + 31) / 32) * 4;
    pixels = palette + colors * 4 + (31 - y) * row;
    mask = palette + colors * 4 + row * 32 + (31 - y) * 4;
    if (bits <= 8) {
        iColor = bits == 1 ? (pixels[x / 8] >> (7 - x % 8)) & 1 : bits == 4 ? (pixels[x / 2] >> (x % 2 ? 0 : 4)) & 15 : pixels[x];
        cr = RGB(palette[iColor * 4 + 2], palette[iColor * 4 + 1], palette[iColor * 4]);
    } else cr = RGB(pixels[x * (bits / 8) + 2], pixels[x * (bits / 8) + 1], pixels[x * (bits / 8)]);
    return ((mask[x / 8] & (0x80 >> (x % 8))) ? bg : 0) ^ cr;
}
static LRESULT CALLBACK ImageProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) { return DefWindowProcA(hwnd, msg, w, l); }
static void Drain(void) {
    APPMSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageA(&msg); }
}
int main(void) {
    HINSTANCE hinst = GetModuleHandleA(NULL);
    HRSRC hgroup, hrsrc;
    const BYTE *rgb;
    HICON hicon, hcopy;
    HCURSOR harrow, hwait, hcur;
    ICONINFO ii;
    BITMAP bm;
    HDC hscreen, hdc;
    HBITMAP hbmp;
    HGDIOBJ hold;
    HBRUSH hbr;
    RECT rc = {0, 0, 64, 64};
    COLORREF bg, expected, actual;
    int i, x, y, j;
    DWORD cb;
    WNDCLASS wc = {0};
    HWND hwnd, hwndOther;
#ifndef _WIN32
    COMPATIMAGE *lp;
    SDL_Surface *lpComposite;
    SDL_Event event = {0};
    BYTE *pixel;
    const char *pszDriver;
#endif
    CHECK(hinst && GetSystemMetrics(SM_CXICON) == 32 && GetSystemMetrics(SM_CYICON) == 32);
    hscreen = GetDC(NULL); hdc = CreateCompatibleDC(hscreen); hbmp = CreateCompatibleBitmap(hscreen, 64, 64);
    CHECK(hscreen && hdc && hbmp); hold = SelectObject(hdc, hbmp);
    CHECK(ReleaseDC(NULL, hscreen)); hscreen = NULL;
    for (i = 0; i < (int)(sizeof(rgImageManifest) / sizeof(rgImageManifest[0])); i++) {
        hgroup = FindResourceA(hinst, rgImageManifest[i].psz, rgImageManifest[i].fCursor ? RT_GROUP_CURSOR : RT_GROUP_ICON);
        CHECK(hgroup && SizeofResource(hinst, hgroup) == rgImageManifest[i].cbGroup);
        rgb = (const BYTE*)(LockResource(LoadResource(hinst, hgroup))); CHECK(rgb && WTest(rgb + 4) == 1);
        hrsrc = FindResourceA(hinst, MAKEINTRESOURCE(WTest(rgb + 18)), rgImageManifest[i].fCursor ? RT_CURSOR : RT_ICON);
        cb = SizeofResource(hinst, hrsrc); rgb = (const BYTE*)(LockResource(LoadResource(hinst, hrsrc)));
        CHECK(rgb && cb == rgImageManifest[i].cb && memcmp(rgb, rgImageManifest[i].rgb, cb) == 0);
        hicon = rgImageManifest[i].fCursor ? (HICON)LoadCursorA(hinst, rgImageManifest[i].psz) : LoadIconA(hinst, rgImageManifest[i].psz);
        CHECK(hicon && GetIconInfo(hicon, &ii));
        CHECK(ii.fIcon == !rgImageManifest[i].fCursor);
        CHECK(ii.xHotspot == (rgImageManifest[i].fCursor ? WTest(rgb) : 16));
        CHECK(ii.yHotspot == (rgImageManifest[i].fCursor ? WTest(rgb + 2) : 16));
        CHECK(GetObjectA(ii.hbmMask, sizeof(bm), &bm) == sizeof(bm) && bm.bmWidth == 32);
        CHECK(bm.bmHeight == (rgImageManifest[i].fCursor ? 64 : 32));
        CHECK(DeleteObject(ii.hbmMask));
        if (ii.hbmColor) CHECK(DeleteObject(ii.hbmColor));
        hcopy = CopyIcon(hicon); CHECK(hcopy && hcopy != hicon);
        for (j = 0; j < 2; j++) {
            bg = j ? RGB(91, 47, 163) : RGB(0, 0, 0); hbr = CreateSolidBrush(bg);
            CHECK(hbr && FillRect(hdc, &rc, hbr)); CHECK(DeleteObject(hbr));
            CHECK(DrawIcon(hdc, 5, 7, hcopy));
            for (y = 0; y < 32; y++) for (x = 0; x < 32; x++) {
                expected = CrExpected(rgb, rgImageManifest[i].fCursor, x, y, bg); actual = GetPixel(hdc, x + 5, y + 7);
                if (expected != actual) { fprintf(stderr, "image %d pixel %d,%d bg%06x expected%06x actual%06x\n", i, x, y, (unsigned)bg, (unsigned)expected, (unsigned)actual); return 1; }
            }
            CHECK(GetPixel(hdc, 4, 7) == bg && GetPixel(hdc, 37, 7) == bg);
        }
        CHECK(rgImageManifest[i].fCursor ? DestroyCursor((HCURSOR)hcopy) : DestroyIcon(hcopy));
#ifndef _WIN32
        CHECK(!GetIconInfo(hcopy, &ii) && !DrawIcon(hdc, 0, 0, hcopy));
        // Reject prefixes until both complete planes have arrived; some legacy
        // cursor files contain harmless trailing padding after their planes.
        lp = LpCompatImage(hicon); CHECK(lp);
        for (cb = 0; cb < (rgImageManifest[i].fCursor ? 308u : 744u); cb++)
            CHECK(!HiconCompatResource(rgb, cb, rgImageManifest[i].fCursor));
#endif
    }
    CHECK(!LoadIconA(hinst, "missing") && !LoadCursorA(hinst, "missing"));
    harrow = LoadCursorA(NULL, MAKEINTRESOURCE(32512)); hwait = LoadCursorA(NULL, MAKEINTRESOURCE(32514));
    CHECK(harrow && hwait); SetCursor(harrow); CHECK(SetCursor(hwait) == harrow && GetCursor() == hwait);
    CHECK(SetCursor(NULL) == hwait && GetCursor() == NULL); CHECK(SetCursor(harrow) == NULL);
    wc.lpfnWndProc = ImageProc; wc.hInstance = hinst; wc.lpszClassName = "ImageContract";
    wc.hCursor = harrow; wc.hIcon = LoadIconA(hinst, "StarsIco"); CHECK(wc.hIcon && RegisterClassA(&wc));
    hwnd = CreateWindowExA(0, wc.lpszClassName, "Icons and cursors", WS_OVERLAPPEDWINDOW, 80, 80, 160, 120, NULL, NULL, hinst, NULL);
    hwndOther = CreateWindowExA(0, wc.lpszClassName, "Other", WS_OVERLAPPEDWINDOW, 260, 80, 160, 120, NULL, NULL, hinst, NULL);
    CHECK(hwnd && hwndOther); ShowWindow(hwnd, SW_SHOW); ShowWindow(hwndOther, SW_SHOW); Drain();
    SetCursor(hwait); SendMessageA(hwnd, WM_SETCURSOR, (WPARAM)hwnd, MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
    CHECK(GetCursor() == harrow);
#ifndef _WIN32
    hcur = LoadCursorA(hinst, "ScannerCur"); lp = LpCompatImage((HICON)hcur); CHECK(lp);
    SetCursor(hcur); pszDriver = SDL_GetCurrentVideoDriver();
    if (pszDriver && strcmp(pszDriver, "dummy") != 0) CHECK(lp->lpCursor && SDL_GetCursor() == lp->lpCursor.get());
    // An inverted cursor is composed over the scene, never into client pixels.
    hcur = LoadCursorA(hinst, rgImageManifest[19].psz);
    lp = LpCompatImage((HICON)hcur); CHECK(lp && lp->fInvert);
    hbr = CreateSolidBrush(RGB(91, 47, 163)); CHECK(hbr);
    CHECK(GetClientRect(hwnd, &rc));
    hscreen = GetDC(hwnd); CHECK(hscreen && FillRect(hscreen, &rc, hbr)); CHECK(ReleaseDC(hwnd, hscreen)); CHECK(DeleteObject(hbr));
    SetCursor(hcur); CompatCursorPoint(hwnd, 50, 50); CHECK(FCompatPresent(hwnd));
    lpComposite = hwnd->lpComposite.get(); CHECK(lpComposite);
    for (y = 0; y < 32; y++) for (x = 0; x < 32; x++) {
        expected = CrExpected(rgImageManifest[19].rgb, 1, x, y, RGB(91, 47, 163));
        pixel = (BYTE *)lpComposite->pixels + (50 - lp->yHot + y) * lpComposite->pitch + (50 - lp->xHot + x) * 4;
        CHECK(RGB(pixel[0], pixel[1], pixel[2]) == expected);
    }
    hscreen = GetDC(hwnd); CHECK(GetPixel(hscreen, 50, 50) == RGB(91, 47, 163)); CHECK(ReleaseDC(hwnd, hscreen));
    CompatCursorPoint(hwndOther, 50, 50);
    pixel = (BYTE *)hwnd->lpComposite->pixels + 50 * hwnd->lpComposite->pitch + 50 * 4;
    CHECK(RGB(pixel[0], pixel[1], pixel[2]) == RGB(91, 47, 163));
    CompatCursorLeave(hwndOther);
    SetCursor(hwait);
    event.type = SDL_EVENT_MOUSE_MOTION; event.motion.windowID = hwnd->id; event.motion.x = 20; event.motion.y = 20;
    CompatRouteEvent(&event); Drain(); CHECK(GetCursor() == harrow);
#else
    (void)hcur;
#endif
    CHECK(DestroyWindow(hwnd) && DestroyWindow(hwndOther));
    SelectObject(hdc, hold); CHECK(DeleteDC(hdc) && DeleteObject(hbmp));
#ifndef _WIN32
    StarsCompatShutdown();
#endif
    puts("Icon/cursor resources and pixel contracts passed");
    return 0;
}
