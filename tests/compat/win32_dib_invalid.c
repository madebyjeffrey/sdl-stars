/* Exercise explicit bounds unavailable in the public Win32 buffer signatures. */
#include "internal.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "decoder line %d: %s\n", __LINE__, #f); return 1; } } while (0)

int TestInvalidDibs(void) {
    struct { BITMAPINFOHEADER bi; RGBQUAD rgcr[2]; } info = {0};
    BYTE rgb[8] = {0};
    COMPATDIB dib;
    SDL_Surface *lpsurface;

    info.bi.biSize = sizeof(info.bi); info.bi.biWidth = 3; info.bi.biHeight = 2;
    info.bi.biPlanes = 1; info.bi.biBitCount = 1;
    CHECK(!FCompatDib(NULL, 0, rgb, sizeof(rgb), &dib));
    CHECK(!FCompatDib(&info, 3, rgb, sizeof(rgb), &dib));
    CHECK(!FCompatDib(&info, sizeof(info) - 1, rgb, sizeof(rgb), &dib));
    CHECK(!FCompatDib(&info, sizeof(info), rgb, sizeof(rgb) - 1, &dib));
    CHECK(FCompatDib(&info, sizeof(info), rgb, sizeof(rgb), &dib));
    lpsurface = LpCompatDibSurface(&dib);
    CHECK(lpsurface); SDL_DestroySurface(lpsurface);
    info.bi.biClrUsed = 1; rgb[0] = 0x80;
    CHECK(FCompatDib(&info, sizeof(info), rgb, sizeof(rgb), &dib));
    CHECK(!LpCompatDibSurface(&dib));
    info.bi.biClrUsed = 3;
    CHECK(!FCompatDib(&info, sizeof(info), rgb, sizeof(rgb), &dib));
    info.bi.biClrUsed = 0; info.bi.biHeight = INT_MIN;
    CHECK(!FCompatDib(&info, sizeof(info), rgb, sizeof(rgb), &dib));
    info.bi.biHeight = 2; info.bi.biWidth = INT_MAX;
    CHECK(!FCompatDib(&info, sizeof(info), rgb, sizeof(rgb), &dib));
    info.bi.biWidth = 3; info.bi.biCompression = BI_RLE8;
    CHECK(!FCompatDib(&info, sizeof(info), rgb, sizeof(rgb), &dib));
    info.bi.biCompression = BI_RGB; info.bi.biBitCount = 16;
    CHECK(!FCompatDib(&info, sizeof(info), rgb, sizeof(rgb), &dib));
    info.bi.biBitCount = 1; info.bi.biSize = 12;
    CHECK(!FCompatDib(&info, sizeof(info), rgb, sizeof(rgb), &dib));
    CHECK(!HbmpCompatResource((const BYTE *)&info, sizeof(info)));
    CHECK(!HbmpCompatResource(NULL, sizeof(info)));
    return 0;
}
