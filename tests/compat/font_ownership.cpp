/* Glyph cache eviction, scaled rasters and library restart keep borrowed metrics. */
#include "internal.h"
#include <cstdio>
#include <cstring>

#define CHECK(f) do { if (!(f)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static void CountGlyph(void *lpv, int64_t x, int64_t y, BYTE alpha) {
    int *pc = static_cast<int *>(lpv);
    (void)x; (void)y;
    if (alpha) (*pc)++;
}
int main() {
    LOGFONTA lf = {};
    SIZE before, after;
    char sz[3] = {};
    int ch, cPixels = 0;
    NATIVEOWNER<COMPATFONT, FreeCompatFont> lpfont, lpfontRetained;

    lf.lfHeight = -12;
    std::strcpy(lf.lfFaceName, "Arial");
    lpfont.reset(LpNewCompatFont(&lf)); CHECK(lpfont);
    CHECK(FCompatFontSizeUtf8(lpfont.get(), "\xc4\x80", 2, &before));
    for (ch = 0x100; ch < 0x300; ch++) {
        sz[0] = static_cast<char>(0xc0 | (ch >> 6));
        sz[1] = static_cast<char>(0x80 | (ch & 0x3f));
        CHECK(FCompatFontSizeUtf8(lpfont.get(), sz, 2, &after));
    }
    CHECK(FCompatFontSizeUtf8(lpfont.get(), "\xc4\x80", 2, &after));
    CHECK(before.cx == after.cx && before.cy == after.cy);
    CHECK(FCompatFontDrawScaled(lpfont.get(), "\xc4\x80", 2, NULL, 2, CountGlyph, &cPixels));
    CHECK(cPixels > 0);
    lpfontRetained = std::move(lpfont); CompatShutdownFonts();
    // Module cleanup drops its root; a live face still owns its library.
    CHECK(FCompatFontSizeUtf8(lpfontRetained.get(), "\xc4\x80", 2, &after));
    CHECK(before.cx == after.cx && before.cy == after.cy);
    lpfont.reset(LpNewCompatFont(&lf)); CHECK(lpfont);
    CHECK(FCompatFontSizeUtf8(lpfont.get(), "\xc4\x80", 2, &after));
    CHECK(before.cx == after.cx && before.cy == after.cy);
    lpfont.reset(); CompatShutdownFonts(); lpfontRetained.reset();
    std::puts("Glyph eviction/reload, scaled pixels and font restart preserve metrics");
    return 0;
}
