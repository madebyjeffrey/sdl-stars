/* Glyph cache eviction, scaled rasters and library restart keep borrowed metrics. */
#include "internal.h"
#include <cstdio>
#include <cstring>

#define CHECK(f) do { if (!(f)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static int cPixels;
static void CountGlyph(COMPATTEXTDRAW &, int64_t x, int64_t y, BYTE alpha) {
    (void)x; (void)y;
    if (alpha) cPixels++;
}
int main() {
    LOGFONTA lf = {};
    std::optional<SIZE> before, after;
    char sz[3] = {};
    int ch;
    COMPATTEXTDRAW draw{};
    std::optional<TEXTMETRICA> metrics;
    NATIVEOWNER<COMPATFONT, FreeCompatFont> lpfont, lpfontRetained;

    lf.lfHeight = -12;
    std::strcpy(lf.lfFaceName, "Arial");
    lpfont.reset(LpNewCompatFont(&lf)); CHECK(lpfont);
    CHECK(!FCompatFontMetrics(nullptr));
    metrics = FCompatFontMetrics(lpfont.get()); CHECK(metrics && metrics->tmHeight > 0);
    CHECK(!FCompatFontSizeUtf8(nullptr, std::string_view{"\xc4\x80", 2}));
    CHECK(!FCompatFontSizeUtf8(lpfont.get(), std::string_view{"\xc0\x80", 2}));
    before = FCompatFontSizeUtf8(lpfont.get(), std::string_view{"\xc4\x80", 2}); CHECK(before);
    for (ch = 0x100; ch < 0x300; ch++) {
        sz[0] = static_cast<char>(0xc0 | (ch >> 6));
        sz[1] = static_cast<char>(0x80 | (ch & 0x3f));
        after = FCompatFontSizeUtf8(lpfont.get(), std::string_view{sz, 2}); CHECK(after);
    }
    after = FCompatFontSizeUtf8(lpfont.get(), std::string_view{"\xc4\x80", 2}); CHECK(after);
    CHECK(before->cx == after->cx && before->cy == after->cy);
    CHECK(FCompatFontDrawScaled(lpfont.get(), std::string_view{"\xc4\x80", 2}, {}, 2, CountGlyph, draw));
    CHECK(cPixels > 0);
    lpfontRetained = std::move(lpfont); CompatShutdownFonts();
    // Module cleanup drops its root; a live face still owns its library.
    after = FCompatFontSizeUtf8(lpfontRetained.get(), std::string_view{"\xc4\x80", 2}); CHECK(after);
    CHECK(before->cx == after->cx && before->cy == after->cy);
    lpfont.reset(LpNewCompatFont(&lf)); CHECK(lpfont);
    after = FCompatFontSizeUtf8(lpfont.get(), std::string_view{"\xc4\x80", 2}); CHECK(after);
    CHECK(before->cx == after->cx && before->cy == after->cy);
    lpfont.reset(); CompatShutdownFonts(); lpfontRetained.reset();
    std::puts("Glyph eviction/reload, scaled pixels and font restart preserve metrics");
    return 0;
}
