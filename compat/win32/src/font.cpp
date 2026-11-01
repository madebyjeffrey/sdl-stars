/* Fontconfig chooses installed faces; FreeType measures/rasterizes UTF-8 text. */
#include "internal.h"
#include "encoding.h"
#include "ownership.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>
#include <climits>
#include <new>
#include <utility>
#include <cstdlib>
#include <cstring>
#include <strings.h>

struct COMPATGLYPH {
    NATIVEARRAYOWNER<BYTE, std::free> rgb;
    int cx, cy, x, y, dx;
    BOOL fLoaded;
    DWORD ch;
    struct COMPATGLYPH *lpNext;
};

static void FreeCompatGlyphs(COMPATGLYPH *lp) noexcept {
    COMPATGLYPH *lpNext;
    while (lp) { lpNext = lp->lpNext; delete lp; lp = lpNext; }
}

struct COMPATFONT {
    // A face keeps its library alive even when module shutdown drops its root.
    std::shared_ptr<FT_LibraryRec_> library;
    LOGFONTA lf;
    TEXTMETRICA tm;
    NATIVEOWNER<FT_FaceRec_, FT_Done_Face> face;
    NATIVEOWNER<COMPATGLYPH, FreeCompatGlyphs> lpGlyphUtf8;
    COMPATGLYPH rgglyph[256];
    UINT cGlyphUtf8;
    NATIVEOWNER<COMPATFONT, FreeCompatFont> lpScaled;
    int iScale;
};

static std::shared_ptr<FT_LibraryRec_> library;
static NATIVEOWNER<FcConfig, FcConfigDestroy> lpConfig;

static COMPATGLYPH *LpLoadGlyph(COMPATFONT *lpfont, FT_Face face, DWORD ch, COMPATGLYPH *lp) {
    FT_Bitmap *lpbm;
    int x, y;
    const BYTE *rgbRow;

    if (lp->fLoaded) return lp;
    if (FT_Load_Char(face, ch, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL)) return nullptr;
    lpbm = &face->glyph->bitmap;
    lp->cx = static_cast<int>(lpbm->width); lp->cy = static_cast<int>(lpbm->rows);
    lp->x = face->glyph->bitmap_left;
    lp->y = lpfont->tm.tmAscent - face->glyph->bitmap_top;
    lp->dx = static_cast<int>(((face->glyph->advance.x + 32) >> 6));
    if (lp->cx && lp->cy) {
        lp->rgb.reset(static_cast<BYTE *>(std::malloc(static_cast<std::size_t>(lp->cx) * lp->cy)));
        if (!lp->rgb) return nullptr;
        for (y = 0; y < lp->cy; y++) {
            rgbRow = lpbm->buffer + (lpbm->pitch >= 0 ? y : lp->cy - 1 - y) * std::abs(lpbm->pitch);
            for (x = 0; x < lp->cx; x++)
                lp->rgb[static_cast<std::size_t>(y) * lp->cx + x] = lpbm->pixel_mode == FT_PIXEL_MODE_MONO ?
                    (rgbRow[x / 8] & (0x80 >> (x % 8)) ? 255 : 0) :
                    lpbm->pixel_mode == FT_PIXEL_MODE_BGRA ? rgbRow[x * 4 + 3] : rgbRow[x];
        }
    }
    lp->fLoaded = 1;
    return lp;
}

static COMPATGLYPH *LpGlyph(COMPATFONT *lpfont, BYTE ch) {
    return LpLoadGlyph(lpfont, lpfont->face.get(), ch, &lpfont->rgglyph[ch]);
}

static BOOL FFontConfig(void) {
    if (lpConfig) return 1;
    lpConfig.reset(FcConfigCreate());
    if (!lpConfig || !FcConfigParseAndLoad(lpConfig.get(), nullptr, FcTrue) || !FcConfigBuildFonts(lpConfig.get())) {
        lpConfig.reset();
        return 0;
    }
    return 1;
}

static void SetCompatFontDefaults(FcPattern *lppattern) {
#if FC_VERSION >= 21700
    // The older entry point initializes a second, process-global configuration.
    // Default substitution must use the same configuration that matching owns.
    FcConfigSetDefaultSubstitute(lpConfig.get(), lppattern);
#else
    FcDefaultSubstitute(lppattern);
#endif
}

static FT_Face FaceFallback(COMPATFONT *lpfont, DWORD ch) {
    FcPattern *pattern, *match;
    FcCharSet *charset;
    FcResult result;
    FcChar8 *pszFile;
    FT_Face face = nullptr;
    NATIVEOWNER<FT_FaceRec_, FT_Done_Face> lpfaceOwner;
    FT_Matrix matrix;
    FT_Vector delta;
    int index = 0;
    NATIVEOWNER<FcCharSet, FcCharSetDestroy> lpcharsetOwner;
    NATIVEOWNER<FcPattern, FcPatternDestroy> lppatternOwner, lpmatchOwner;
    if (!FFontConfig()) return nullptr;
    lppatternOwner.reset(FcPatternCreate()); pattern = lppatternOwner.get();
    lpcharsetOwner.reset(FcCharSetCreate()); charset = lpcharsetOwner.get();
    if (!pattern || !charset) goto done;
    FcCharSetAddChar(charset, ch); FcPatternAddCharSet(pattern, FC_CHARSET, charset);
    FcPatternAddString(pattern, FC_FAMILY, reinterpret_cast<const FcChar8 *>(lpfont->lf.lfFaceName));
    FcPatternAddInteger(pattern, FC_WEIGHT, lpfont->tm.tmWeight >= 700 ? FC_WEIGHT_BOLD : FC_WEIGHT_REGULAR);
    FcPatternAddInteger(pattern, FC_SLANT, lpfont->tm.tmItalic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
    FcConfigSubstitute(lpConfig.get(), pattern, FcMatchPattern); SetCompatFontDefaults(pattern);
    lpmatchOwner.reset(FcFontMatch(lpConfig.get(), pattern, &result)); match = lpmatchOwner.get();
    if (match && FcPatternGetString(match, FC_FILE, 0, &pszFile) == FcResultMatch &&
        FcPatternGetInteger(match, FC_INDEX, 0, &index) == FcResultMatch) {
        int iError = FT_New_Face(lpfont->library.get(), reinterpret_cast<const char *>(pszFile), index, &face);
        lpfaceOwner.reset(face);
        if (iError || !FT_Get_Char_Index(face, ch) || FT_Set_Pixel_Sizes(face, 0, lpfont->face->size->metrics.y_ppem))
            lpfaceOwner.reset();
        else {
            FT_Get_Transform(lpfont->face.get(), &matrix, &delta);
            FT_Set_Transform(face, &matrix, &delta);
        }
    }
done:
    return lpfaceOwner.release();
}

static COMPATGLYPH *LpGlyphUtf8(COMPATFONT *lpfont, DWORD ch) {
    COMPATGLYPH *lp, *lpPrev;
    FT_Face face;
    std::unique_ptr<COMPATGLYPH> lpglyphOwner;
    NATIVEOWNER<FT_FaceRec_, FT_Done_Face> lpfaceOwner;
    if (ch < 256) return LpGlyph(lpfont, static_cast<BYTE>(ch));
    for (lp = lpfont->lpGlyphUtf8.get(); lp; lp = lp->lpNext) if (lp->ch == ch) return lp;
    // Bound the extra cache independently of cached code points and filename length.
    if (lpfont->cGlyphUtf8 == 256) {
        lpPrev = nullptr;
        for (lp = lpfont->lpGlyphUtf8.get(); lp->lpNext; lp = lp->lpNext) lpPrev = lp;
        if (lpPrev) { lpPrev->lpNext = nullptr; delete lp; }
        else lpfont->lpGlyphUtf8.reset();
        lpfont->cGlyphUtf8--;
    }
    lpglyphOwner.reset(new (std::nothrow) COMPATGLYPH{}); lp = lpglyphOwner.get();
    if (!lp) return nullptr;
    lpfaceOwner.reset(FT_Get_Char_Index(lpfont->face.get(), ch) ? nullptr : FaceFallback(lpfont, ch)); face = lpfaceOwner.get();
    if (!LpLoadGlyph(lpfont, face ? face : lpfont->face.get(), ch, lp)) return nullptr;
    lpfaceOwner.reset();
    lp->ch = ch; lp->lpNext = lpfont->lpGlyphUtf8.release();
    lpfont->lpGlyphUtf8.reset(lp); lpfont->cGlyphUtf8++;
    return lpglyphOwner.release();
}

void FreeCompatFont(COMPATFONT *lpfont) noexcept {
    delete lpfont;
}

static COMPATFONT *LpNewFontRaster(const LOGFONTA *lplf, int cMax) {
    COMPATFONT *lpfont;
    FcPattern *pattern, *match;
    FcResult result;
    FcChar8 *pszFile;
    char szFamily[win32::LF_FACESIZE];
    char *psz;
    int index = 0, cHeight, i, cWidth = 0, dxMax = 0, iFaceError;
    FT_Face face = nullptr;
    FT_Library libraryNew = nullptr;
    NATIVEOWNER<FT_LibraryRec_, FT_Done_FreeType> libraryOwner;
    BOOL fBold, fItalic;
    COMPATGLYPH *lpglyph;
    FT_Matrix matrix = {65536, 0, 0, 65536};
    NATIVEOWNER<COMPATFONT, FreeCompatFont> lpfontOwner;
    NATIVEOWNER<FcPattern, FcPatternDestroy> lppatternOwner, lpmatchOwner;

    if (!lplf || static_cast<std::int64_t>(lplf->lfHeight) < -cMax || lplf->lfHeight > cMax ||
        lplf->lfWidth < 0 || lplf->lfWidth > cMax ||
        !std::memchr(lplf->lfFaceName, 0, sizeof(lplf->lfFaceName)) ||
        !FUtf8Valid(lplf->lfFaceName, std::strlen(lplf->lfFaceName))) return nullptr;
    if (!library) {
        iFaceError = FT_Init_FreeType(&libraryNew);
        libraryOwner.reset(libraryNew);
        if (iFaceError) return nullptr;
        try { library = std::shared_ptr<FT_LibraryRec_>(std::move(libraryOwner)); }
        catch (const std::bad_alloc &) { return nullptr; }
    }
    lpfontOwner.reset(new (std::nothrow) COMPATFONT{}); lpfont = lpfontOwner.get();
    if (!lpfont) return nullptr;
    lpfont->library = library;
    lpfont->lf = *lplf; lpfont->lf.lfFaceName[win32::LF_FACESIZE - 1] = 0;
    CchCopyUtf8Prefix(std::span{szFamily}, lpfont->lf.lfFaceName);
    fBold = lplf->lfWeight >= 600;
    fItalic = lplf->lfItalic;
    // The game stores styled face names rather than setting LOGFONT style flags.
    psz = std::strstr(szFamily, " Bold");
    if (psz) { *psz = 0; fBold = 1; fItalic |= std::strstr(psz + 1, "Italic") != nullptr; }
    else if ((psz = std::strstr(szFamily, " Italic"))) { *psz = 0; fItalic = 1; }
    i = -1;
    if (!*szFamily || !strcasecmp(szFamily, "Arial") || !strcasecmp(szFamily, "Liberation Sans") ||
        !strcasecmp(szFamily, "sans-serif") || !strcasecmp(szFamily, "Tahoma") || !strcasecmp(szFamily, "MS Sans Serif")) i = 0;
    else if (!strcasecmp(szFamily, "Times New Roman") || !strcasecmp(szFamily, "Liberation Serif") || !strcasecmp(szFamily, "serif")) i = 4;
    else if (!strcasecmp(szFamily, "Courier New") || !strcasecmp(szFamily, "Liberation Mono") || !strcasecmp(szFamily, "monospace")) i = 8;
    if (i >= 0) {
        i += (fBold ? 1 : 0) + (fItalic ? 2 : 0);
        iFaceError = FT_New_Memory_Face(lpfont->library.get(), rgCompatFontData[i].rgb, static_cast<FT_Long>(rgCompatFontData[i].cb), 0, &face);
        lpfont->face.reset(face);
        if (iFaceError) goto LFail;
    } else {
        if (!FFontConfig()) goto LFail;
        lppatternOwner.reset(FcPatternCreate()); pattern = lppatternOwner.get();
        if (!pattern) goto LFail;
        FcPatternAddString(pattern, FC_FAMILY, reinterpret_cast<FcChar8 *>(szFamily));
        FcPatternAddInteger(pattern, FC_WEIGHT, fBold ? FC_WEIGHT_BOLD : FC_WEIGHT_REGULAR);
        FcPatternAddInteger(pattern, FC_SLANT, fItalic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
        FcConfigSubstitute(lpConfig.get(), pattern, FcMatchPattern); SetCompatFontDefaults(pattern);
        lpmatchOwner.reset(FcFontMatch(lpConfig.get(), pattern, &result)); match = lpmatchOwner.get(); lppatternOwner.reset();
        if (!match) goto LFail;
        FcPatternGetInteger(match, FC_INDEX, 0, &index);
        if (FcPatternGetString(match, FC_FILE, 0, &pszFile) != FcResultMatch) goto LFail;
        iFaceError = FT_New_Face(lpfont->library.get(), reinterpret_cast<const char *>(pszFile), index, &face);
        lpfont->face.reset(face);
        if (iFaceError) goto LFail;
        lpmatchOwner.reset();
    }
    cHeight = lplf->lfHeight < 0 ? -lplf->lfHeight : lplf->lfHeight;
    if (cHeight == 0) cHeight = 16;
    if (lplf->lfHeight > 0 && lpfont->face->ascender - lpfont->face->descender > 0)
        cHeight = static_cast<int>((static_cast<std::int64_t>(cHeight) * lpfont->face->units_per_EM /
                       (lpfont->face->ascender - lpfont->face->descender)));
    if (FT_Set_Pixel_Sizes(lpfont->face.get(), 0, cHeight > 0 ? static_cast<unsigned>(cHeight) : 1)) goto LFail;
    lpfont->tm.tmAscent = static_cast<LONG>(((lpfont->face->size->metrics.ascender + 63) >> 6));
    lpfont->tm.tmDescent = static_cast<LONG>(((-lpfont->face->size->metrics.descender + 63) >> 6));
    lpfont->tm.tmHeight = lpfont->tm.tmAscent + lpfont->tm.tmDescent;
    lpfont->tm.tmExternalLeading = static_cast<LONG>(((lpfont->face->size->metrics.height + 63) >> 6)) - lpfont->tm.tmHeight;
    if (lpfont->tm.tmExternalLeading < 0) lpfont->tm.tmExternalLeading = 0;
    lpfont->tm.tmWeight = fBold ? 700 : 400; lpfont->tm.tmItalic = fItalic;
    lpfont->tm.tmUnderlined = lplf->lfUnderline; lpfont->tm.tmStruckOut = lplf->lfStrikeOut;
    lpfont->tm.tmFirstChar = 0; lpfont->tm.tmLastChar = 255;
    lpfont->tm.tmDefaultChar = '?'; lpfont->tm.tmBreakChar = ' ';
    lpfont->tm.tmCharSet = lplf->lfCharSet;
    lpfont->tm.tmPitchAndFamily = (lpfont->face->face_flags & FT_FACE_FLAG_FIXED_WIDTH ? 0 : 1) | 4;
    lpfont->tm.tmDigitizedAspectX = lpfont->tm.tmDigitizedAspectY = 96;
    for (i = 'A'; i <= 'z'; i++) {
        if (i > 'Z' && i < 'a') continue;
        lpglyph = LpGlyph(lpfont, static_cast<BYTE>(i)); if (!lpglyph) goto LFail;
        cWidth += lpglyph->dx;
    }
    lpfont->tm.tmAveCharWidth = (cWidth + 26) / 52;
    if (lplf->lfWidth && lpfont->tm.tmAveCharWidth) {
        matrix.xx = static_cast<FT_Fixed>((static_cast<std::int64_t>(lplf->lfWidth) * 65536 / lpfont->tm.tmAveCharWidth));
        FT_Set_Transform(lpfont->face.get(), &matrix, nullptr);
        for (i = 0; i < 256; i++) lpfont->rgglyph[i] = COMPATGLYPH{};
        lpfont->tm.tmAveCharWidth = lplf->lfWidth;
    }
    for (i = 32; i < 256; i++) {
        lpglyph = LpGlyph(lpfont, static_cast<BYTE>(i)); if (!lpglyph) goto LFail;
        if (lpglyph->dx > dxMax) dxMax = lpglyph->dx;
    }
    lpfont->tm.tmMaxCharWidth = dxMax;
    return lpfontOwner.release();
LFail:
    return nullptr;
}

COMPATFONT *LpNewCompatFont(const LOGFONTA *lplf) {
    return LpNewFontRaster(lplf, 512);
}

BOOL FCompatFontSize(COMPATFONT *lpfont, LPCSTR psz, int cch, LPSIZE lpsize) {
    return FCompatFontSizeUtf8(lpfont, psz, cch, lpsize);
}

BOOL FCompatFontMetrics(COMPATFONT *lpfont, LPTEXTMETRICA lptm) {
    if (!lpfont || !lptm) return 0;
    *lptm = lpfont->tm;
    return 1;
}

BOOL FCompatFontDraw(COMPATFONT *lpfont, LPCSTR psz, int cch, const INT *rgdx,
                    void (*pfn)(void *, std::int64_t, std::int64_t, BYTE), void *lpv) {
    return FCompatFontDrawScaled(lpfont, psz, cch, rgdx, 1, pfn, lpv);
}

BOOL FCompatFontSizeUtf8(COMPATFONT *lpfont, LPCSTR psz, int cch, LPSIZE lpsize) {
    int i = 0, c;
    std::uint32_t ch;
    std::int64_t dx = 0;
    COMPATGLYPH *lp;
    if (!lpfont || !lpsize || cch < 0 || cch > 1024 * 1024 || (!psz && cch != 0)) return 0;
    while (i < cch) {
        c = CbUtf8Char(psz + i, static_cast<std::size_t>((cch - i)), &ch);
        if (c == 0) return 0;
        lp = LpGlyphUtf8(lpfont, ch); if (!lp) return 0;
        dx += lp->dx; i += c;
    }
    if (dx > INT_MAX) return 0;
    *lpsize = SIZE{static_cast<LONG>(dx), lpfont->tm.tmHeight};
    return 1;
}

BOOL FCompatFontDrawUtf8(COMPATFONT *lpfont, LPCSTR psz, int cch,
                        void (*pfn)(void *, std::int64_t, std::int64_t, BYTE), void *lpv) {
    int i = 0, c, x, y;
    std::uint32_t ch;
    std::int64_t dx = 0;
    SIZE size;
    COMPATGLYPH *lp;
    if (!FCompatFontSizeUtf8(lpfont, psz, cch, &size)) return 0;
    while (i < cch) {
        c = CbUtf8Char(psz + i, static_cast<std::size_t>((cch - i)), &ch);
        if (c == 0) return 0;
        lp = LpGlyphUtf8(lpfont, ch); if (!lp) return 0;
        for (y = 0; y < lp->cy; y++) for (x = 0; x < lp->cx; x++)
            if (lp->rgb[static_cast<std::size_t>(y) * lp->cx + x]) pfn(lpv, dx + lp->x + x, lp->y + y, lp->rgb[static_cast<std::size_t>(y) * lp->cx + x]);
        dx += lp->dx; i += c;
    }
    return 1;
}

BOOL FCompatFontDrawScaled(COMPATFONT *lpfont, LPCSTR psz, int cch, const INT *rgdx, int iScale,
                          void (*pfn)(void *, std::int64_t, std::int64_t, BYTE), void *lpv) {
    LOGFONTA lf;
    SIZE size;
    COMPATGLYPH *lp, *lpRaster;
    int i = 0, c, j, x, y;
    DWORD ch;
    std::int64_t dx = 0;

    if (iScale < 1 || iScale > 4 || !FCompatFontSizeUtf8(lpfont, psz, cch, &size)) return 0;
    if (cch == 0) return 1;
    if (iScale != 1 && lpfont->iScale != iScale) {
        lf = lpfont->lf;
        lf.lfHeight = -static_cast<int>(lpfont->face->size->metrics.y_ppem) * iScale;
        lf.lfWidth *= iScale;
        lpfont->lpScaled.reset();
        lpfont->lpScaled.reset(LpNewFontRaster(&lf, 2048));
        lpfont->iScale = lpfont->lpScaled ? iScale : 0;
        if (!lpfont->lpScaled) return 0;
    }
    while (i < cch) {
        c = CbUtf8Char(psz + i, static_cast<std::size_t>((cch - i)), &ch);
        if (c == 0) return 0;
        lp = LpGlyphUtf8(lpfont, ch);
        lpRaster = iScale == 1 ? lp : LpGlyphUtf8(lpfont->lpScaled.get(), ch);
        if (!lp || !lpRaster) return 0;
        for (y = 0; y < lpRaster->cy; y++) for (x = 0; x < lpRaster->cx; x++)
            if (lpRaster->rgb[static_cast<std::size_t>(y) * lpRaster->cx + x])
                pfn(lpv, dx * iScale + lpRaster->x + x,
                    lpfont->tm.tmAscent * iScale - (iScale == 1 ? lpfont : lpfont->lpScaled.get())->tm.tmAscent + lpRaster->y + y,
                    lpRaster->rgb[static_cast<std::size_t>(y) * lpRaster->cx + x]);
        // Keep logical advances and baselines: Retina rasterization must not reflow the UI.
        if (rgdx) for (j = 0; j < c; j++) dx += rgdx[i + j];
        else dx += lp->dx;
        i += c;
    }
    return 1;
}

void CompatShutdownFonts(void) {
    lpConfig.reset();
    library.reset();
}
