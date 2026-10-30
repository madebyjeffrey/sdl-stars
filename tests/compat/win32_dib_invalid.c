/* Exercise explicit bounds unavailable in the public Win32 buffer signatures. */
#include "internal.h"
#include "resourcecodec.h"
#include <array>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "decoder line %d: %s\n", __LINE__, #f); return 1; } } while (0)

static int TestDibHeaderBytes(void) {
    const std::array<std::byte, 40> golden = {
        std::byte{0x44}, std::byte{0x33}, std::byte{0x22}, std::byte{0x11},
        std::byte{1}, std::byte{0}, std::byte{0}, std::byte{0x80},
        std::byte{0xfe}, std::byte{0xff}, std::byte{0xff}, std::byte{0x7f},
        std::byte{0x34}, std::byte{0x12}, std::byte{0xcd}, std::byte{0xab},
        std::byte{0x98}, std::byte{0xba}, std::byte{0xdc}, std::byte{0xfe},
        std::byte{0x67}, std::byte{0x45}, std::byte{0x23}, std::byte{1},
        std::byte{0xfe}, std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
        std::byte{0}, std::byte{0x80}, std::byte{0xff}, std::byte{0xff},
        std::byte{0xef}, std::byte{0xcd}, std::byte{0xab}, std::byte{0x89},
        std::byte{0x34}, std::byte{0x12}, std::byte{0xf0}, std::byte{0xa5}};
    BITMAPINFOHEADER bi{};
    std::array<std::byte, 42> rgb;

    rgb.fill(std::byte{0xa5});
    memcpy(rgb.data() + 1, golden.data(), golden.size());
    CHECK(FDecodeDibHeader(std::span(rgb).subspan(1, 40), bi));
    CHECK(bi.biSize == 0x11223344u && bi.biWidth == -2147483647 && bi.biHeight == 2147483646);
    CHECK(bi.biPlanes == 0x1234 && bi.biBitCount == 0xabcd && bi.biCompression == 0xfedcba98u);
    CHECK(bi.biSizeImage == 0x01234567u && bi.biXPelsPerMeter == -2 && bi.biYPelsPerMeter == -32768);
    CHECK(bi.biClrUsed == 0x89abcdefu && bi.biClrImportant == 0xa5f01234u);
    rgb.fill(std::byte{0xa5});
    CHECK(FEncodeDibHeader(bi, std::span(rgb).subspan(1, 40)) && !memcmp(rgb.data() + 1, golden.data(), golden.size()));
    CHECK(rgb.front() == std::byte{0xa5} && rgb.back() == std::byte{0xa5});
    for (std::size_t cb = 0; cb < 40; cb++) {
        BITMAPINFOHEADER unchanged{}; unchanged.biSize = 123;
        CHECK(!FDecodeDibHeader(std::span(golden).first(cb), unchanged) && unchanged.biSize == 123);
        rgb.fill(std::byte{0xa5});
        CHECK(!FEncodeDibHeader(bi, std::span(rgb).subspan(1, cb)));
        for (auto ch : rgb) CHECK(ch == std::byte{0xa5});
    }
    return 0;
}

int TestInvalidDibs(void) {
    struct { BITMAPINFOHEADER bi; RGBQUAD rgcr[2]; } info = {};
    BYTE rgb[8] = {0};
    COMPATDIB dib;
    SDL_Surface *lpsurface;

    CHECK(TestDibHeaderBytes() == 0);
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
