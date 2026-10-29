/* Fontconfig chooses installed faces; FreeType measures/rasterizes UTF-8 text. */
#include "internal.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

typedef struct COMPATGLYPH {
    BYTE *rgb;
    int cx, cy, x, y, dx;
    BOOL fLoaded;
    DWORD ch;
    struct COMPATGLYPH *lpNext;
} COMPATGLYPH;

struct COMPATFONT {
    LOGFONTA lf;
    TEXTMETRICA tm;
    FT_Face face;
    COMPATGLYPH rgglyph[256], *lpGlyphUtf8;
    UINT cGlyphUtf8;
    struct COMPATFONT *lpScaled;
    int iScale;
};

static FT_Library library;
static FcConfig *lpConfig;

static COMPATGLYPH *LpLoadGlyph(COMPATFONT *lpfont, FT_Face face, DWORD ch, COMPATGLYPH *lp) {
    FT_Bitmap *lpbm;
    int x, y;
    const BYTE *rgbRow;

    if (lp->fLoaded) return lp;
    if (FT_Load_Char(face, ch, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL)) return NULL;
    lpbm = &face->glyph->bitmap;
    lp->cx = (int)lpbm->width; lp->cy = (int)lpbm->rows;
    lp->x = face->glyph->bitmap_left;
    lp->y = lpfont->tm.tmAscent - face->glyph->bitmap_top;
    lp->dx = (int)((face->glyph->advance.x + 32) >> 6);
    if (lp->cx && lp->cy) {
        lp->rgb = (BYTE *)(malloc((size_t)lp->cx * lp->cy));
        if (!lp->rgb) return NULL;
        for (y = 0; y < lp->cy; y++) {
            rgbRow = lpbm->buffer + (lpbm->pitch >= 0 ? y : lp->cy - 1 - y) * abs(lpbm->pitch);
            for (x = 0; x < lp->cx; x++)
                lp->rgb[(size_t)y * lp->cx + x] = lpbm->pixel_mode == FT_PIXEL_MODE_MONO ?
                    (rgbRow[x / 8] & (0x80 >> (x % 8)) ? 255 : 0) :
                    lpbm->pixel_mode == FT_PIXEL_MODE_BGRA ? rgbRow[x * 4 + 3] : rgbRow[x];
        }
    }
    lp->fLoaded = TRUE;
    return lp;
}

static COMPATGLYPH *LpGlyph(COMPATFONT *lpfont, BYTE ch) {
    return LpLoadGlyph(lpfont, lpfont->face, ch, &lpfont->rgglyph[ch]);
}

static BOOL FFontConfig(void) {
    if (lpConfig) return TRUE;
    lpConfig = FcConfigCreate();
    if (!lpConfig || !FcConfigParseAndLoad(lpConfig, NULL, FcTrue) || !FcConfigBuildFonts(lpConfig)) {
        if (lpConfig) FcConfigDestroy(lpConfig);
        lpConfig = NULL;
        return FALSE;
    }
    return TRUE;
}

static FT_Face FaceFallback(COMPATFONT *lpfont, DWORD ch) {
    FcPattern *pattern, *match;
    FcCharSet *charset;
    FcResult result;
    FcChar8 *pszFile;
    FT_Face face = NULL;
    FT_Matrix matrix;
    FT_Vector delta;
    int index = 0;
    if (!FFontConfig()) return NULL;
    pattern = FcPatternCreate(); charset = FcCharSetCreate();
    if (!pattern || !charset) goto done;
    FcCharSetAddChar(charset, ch); FcPatternAddCharSet(pattern, FC_CHARSET, charset);
    FcPatternAddString(pattern, FC_FAMILY, (const FcChar8 *)lpfont->lf.lfFaceName);
    FcPatternAddInteger(pattern, FC_WEIGHT, lpfont->tm.tmWeight >= 700 ? FC_WEIGHT_BOLD : FC_WEIGHT_REGULAR);
    FcPatternAddInteger(pattern, FC_SLANT, lpfont->tm.tmItalic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
    FcConfigSubstitute(lpConfig, pattern, FcMatchPattern); FcDefaultSubstitute(pattern);
    match = FcFontMatch(lpConfig, pattern, &result);
    if (match) {
        if (FcPatternGetString(match, FC_FILE, 0, &pszFile) == FcResultMatch &&
            FcPatternGetInteger(match, FC_INDEX, 0, &index) == FcResultMatch &&
            !FT_New_Face(library, (const char *)pszFile, index, &face)) {
            if (!FT_Get_Char_Index(face, ch) || FT_Set_Pixel_Sizes(face, 0, lpfont->face->size->metrics.y_ppem)) {
                FT_Done_Face(face); face = NULL;
            } else {
                FT_Get_Transform(lpfont->face, &matrix, &delta);
                FT_Set_Transform(face, &matrix, &delta);
            }
        }
        FcPatternDestroy(match);
    }
done:
    if (pattern) FcPatternDestroy(pattern);
    if (charset) FcCharSetDestroy(charset);
    return face;
}

static COMPATGLYPH *LpGlyphUtf8(COMPATFONT *lpfont, DWORD ch) {
    COMPATGLYPH *lp, **lpp;
    FT_Face face;
    if (ch < 256) return LpGlyph(lpfont, (BYTE)ch);
    for (lp = lpfont->lpGlyphUtf8; lp; lp = lp->lpNext) if (lp->ch == ch) return lp;
    // Bound the extra cache independently of cached code points and filename length.
    if (lpfont->cGlyphUtf8 == 256) {
        for (lpp = &lpfont->lpGlyphUtf8; (*lpp)->lpNext; lpp = &(*lpp)->lpNext) {}
        free((*lpp)->rgb); free(*lpp); *lpp = NULL; lpfont->cGlyphUtf8--;
    }
    lp = (COMPATGLYPH *)(calloc(1, sizeof(*lp)));
    if (!lp) return NULL;
    face = FT_Get_Char_Index(lpfont->face, ch) ? NULL : FaceFallback(lpfont, ch);
    if (!LpLoadGlyph(lpfont, face ? face : lpfont->face, ch, lp)) {
        if (face) FT_Done_Face(face);
        free(lp); return NULL;
    }
    if (face) FT_Done_Face(face);
    lp->ch = ch; lp->lpNext = lpfont->lpGlyphUtf8;
    lpfont->lpGlyphUtf8 = lp; lpfont->cGlyphUtf8++;
    return lp;
}

void FreeCompatFont(COMPATFONT *lpfont) {
    int i;
    COMPATGLYPH *lp, *lpNext;
    if (!lpfont) return;
    FreeCompatFont(lpfont->lpScaled);
    for (i = 0; i < 256; i++) free(lpfont->rgglyph[i].rgb);
    for (lp = lpfont->lpGlyphUtf8; lp; lp = lpNext) {
        lpNext = lp->lpNext; free(lp->rgb); free(lp);
    }
    if (lpfont->face) FT_Done_Face(lpfont->face);
    free(lpfont);
}

static COMPATFONT *LpNewFontRaster(const LOGFONTA *lplf, int cMax) {
    COMPATFONT *lpfont;
    FcPattern *pattern, *match;
    FcResult result;
    FcChar8 *pszFile;
    char szFamily[LF_FACESIZE];
    char *psz;
    int index = 0, cHeight, i, cWidth = 0, dxMax = 0;
    BOOL fBold, fItalic;
    COMPATGLYPH *lpglyph;
    FT_Matrix matrix = {65536, 0, 0, 65536};

    if (!lplf || (int64_t)lplf->lfHeight < -cMax || lplf->lfHeight > cMax ||
        lplf->lfWidth < 0 || lplf->lfWidth > cMax ||
        !memchr(lplf->lfFaceName, 0, sizeof(lplf->lfFaceName)) ||
        !FUtf8Valid(lplf->lfFaceName, strlen(lplf->lfFaceName))) return NULL;
    if (!library && FT_Init_FreeType(&library)) return NULL;
    lpfont = (COMPATFONT *)(calloc(1, sizeof(*lpfont)));
    if (!lpfont) return NULL;
    lpfont->lf = *lplf; lpfont->lf.lfFaceName[LF_FACESIZE - 1] = 0;
    strcpy(szFamily, lpfont->lf.lfFaceName);
    fBold = lplf->lfWeight >= 600;
    fItalic = lplf->lfItalic;
    // The game stores styled face names rather than setting LOGFONT style flags.
    psz = strstr(szFamily, " Bold");
    if (psz) { *psz = 0; fBold = TRUE; fItalic |= strstr(psz + 1, "Italic") != NULL; }
    else if ((psz = strstr(szFamily, " Italic"))) { *psz = 0; fItalic = TRUE; }
    i = -1;
    if (!*szFamily || !strcasecmp(szFamily, "Arial") || !strcasecmp(szFamily, "Liberation Sans") ||
        !strcasecmp(szFamily, "sans-serif") || !strcasecmp(szFamily, "Tahoma") || !strcasecmp(szFamily, "MS Sans Serif")) i = 0;
    else if (!strcasecmp(szFamily, "Times New Roman") || !strcasecmp(szFamily, "Liberation Serif") || !strcasecmp(szFamily, "serif")) i = 4;
    else if (!strcasecmp(szFamily, "Courier New") || !strcasecmp(szFamily, "Liberation Mono") || !strcasecmp(szFamily, "monospace")) i = 8;
    if (i >= 0) {
        i += (fBold ? 1 : 0) + (fItalic ? 2 : 0);
        if (FT_New_Memory_Face(library, rgCompatFontData[i].rgb, (FT_Long)rgCompatFontData[i].cb, 0, &lpfont->face)) goto LFail;
    } else {
        if (!FFontConfig()) goto LFail;
        pattern = FcPatternCreate();
        if (!pattern) goto LFail;
        FcPatternAddString(pattern, FC_FAMILY, (FcChar8 *)szFamily);
        FcPatternAddInteger(pattern, FC_WEIGHT, fBold ? FC_WEIGHT_BOLD : FC_WEIGHT_REGULAR);
        FcPatternAddInteger(pattern, FC_SLANT, fItalic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
        FcConfigSubstitute(lpConfig, pattern, FcMatchPattern); FcDefaultSubstitute(pattern);
        match = FcFontMatch(lpConfig, pattern, &result); FcPatternDestroy(pattern);
        if (!match) goto LFail;
        FcPatternGetInteger(match, FC_INDEX, 0, &index);
        if (FcPatternGetString(match, FC_FILE, 0, &pszFile) != FcResultMatch ||
            FT_New_Face(library, (const char *)pszFile, index, &lpfont->face)) {
            FcPatternDestroy(match); goto LFail;
        }
        FcPatternDestroy(match);
    }
    cHeight = lplf->lfHeight < 0 ? -lplf->lfHeight : lplf->lfHeight;
    if (cHeight == 0) cHeight = 16;
    if (lplf->lfHeight > 0 && lpfont->face->ascender - lpfont->face->descender > 0)
        cHeight = (int)((int64_t)cHeight * lpfont->face->units_per_EM /
                       (lpfont->face->ascender - lpfont->face->descender));
    if (FT_Set_Pixel_Sizes(lpfont->face, 0, cHeight > 0 ? (unsigned)cHeight : 1)) goto LFail;
    lpfont->tm.tmAscent = (LONG)((lpfont->face->size->metrics.ascender + 63) >> 6);
    lpfont->tm.tmDescent = (LONG)((-lpfont->face->size->metrics.descender + 63) >> 6);
    lpfont->tm.tmHeight = lpfont->tm.tmAscent + lpfont->tm.tmDescent;
    lpfont->tm.tmExternalLeading = (LONG)((lpfont->face->size->metrics.height + 63) >> 6) - lpfont->tm.tmHeight;
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
        lpglyph = LpGlyph(lpfont, (BYTE)i); if (!lpglyph) goto LFail;
        cWidth += lpglyph->dx;
    }
    lpfont->tm.tmAveCharWidth = (cWidth + 26) / 52;
    if (lplf->lfWidth && lpfont->tm.tmAveCharWidth) {
        matrix.xx = (FT_Fixed)((int64_t)lplf->lfWidth * 65536 / lpfont->tm.tmAveCharWidth);
        FT_Set_Transform(lpfont->face, &matrix, NULL);
        for (i = 0; i < 256; i++) { free(lpfont->rgglyph[i].rgb); memset(&lpfont->rgglyph[i], 0, sizeof(COMPATGLYPH)); }
        lpfont->tm.tmAveCharWidth = lplf->lfWidth;
    }
    for (i = 32; i < 256; i++) {
        lpglyph = LpGlyph(lpfont, (BYTE)i); if (!lpglyph) goto LFail;
        if (lpglyph->dx > dxMax) dxMax = lpglyph->dx;
    }
    lpfont->tm.tmMaxCharWidth = dxMax;
    return lpfont;
LFail:
    FreeCompatFont(lpfont);
    return NULL;
}

COMPATFONT *LpNewCompatFont(const LOGFONTA *lplf) {
    return LpNewFontRaster(lplf, 512);
}

BOOL FCompatFontSize(COMPATFONT *lpfont, LPCSTR psz, int cch, LPSIZE lpsize) {
    return FCompatFontSizeUtf8(lpfont, psz, cch, lpsize);
}

BOOL FCompatFontMetrics(COMPATFONT *lpfont, LPTEXTMETRICA lptm) {
    if (!lpfont || !lptm) return FALSE;
    *lptm = lpfont->tm;
    return TRUE;
}

BOOL FCompatFontDraw(COMPATFONT *lpfont, LPCSTR psz, int cch, const INT *rgdx,
                    void (*pfn)(void *, int64_t, int64_t, BYTE), void *lpv) {
    return FCompatFontDrawScaled(lpfont, psz, cch, rgdx, 1, pfn, lpv);
}

BOOL FCompatFontSizeUtf8(COMPATFONT *lpfont, LPCSTR psz, int cch, LPSIZE lpsize) {
    int i = 0, c;
    uint32_t ch;
    int64_t dx = 0;
    COMPATGLYPH *lp;
    if (!lpfont || !lpsize || cch < 0 || cch > 1024 * 1024 || (!psz && cch != 0)) return FALSE;
    while (i < cch) {
        c = CbUtf8Char(psz + i, (size_t)(cch - i), &ch);
        if (c == 0) return FALSE;
        lp = LpGlyphUtf8(lpfont, ch); if (!lp) return FALSE;
        dx += lp->dx; i += c;
    }
    if (dx > INT_MAX) return FALSE;
    *lpsize = (SIZE){(LONG)dx, lpfont->tm.tmHeight};
    return TRUE;
}

BOOL FCompatFontDrawUtf8(COMPATFONT *lpfont, LPCSTR psz, int cch,
                        void (*pfn)(void *, int64_t, int64_t, BYTE), void *lpv) {
    int i = 0, c, x, y;
    uint32_t ch;
    int64_t dx = 0;
    SIZE size;
    COMPATGLYPH *lp;
    if (!FCompatFontSizeUtf8(lpfont, psz, cch, &size)) return FALSE;
    while (i < cch) {
        c = CbUtf8Char(psz + i, (size_t)(cch - i), &ch);
        if (c == 0) return FALSE;
        lp = LpGlyphUtf8(lpfont, ch); if (!lp) return FALSE;
        for (y = 0; y < lp->cy; y++) for (x = 0; x < lp->cx; x++)
            if (lp->rgb[(size_t)y * lp->cx + x]) pfn(lpv, dx + lp->x + x, lp->y + y, lp->rgb[(size_t)y * lp->cx + x]);
        dx += lp->dx; i += c;
    }
    return TRUE;
}

BOOL FCompatFontDrawScaled(COMPATFONT *lpfont, LPCSTR psz, int cch, const INT *rgdx, int iScale,
                          void (*pfn)(void *, int64_t, int64_t, BYTE), void *lpv) {
    LOGFONTA lf;
    SIZE size;
    COMPATGLYPH *lp, *lpRaster;
    int i = 0, c, j, x, y;
    DWORD ch;
    int64_t dx = 0;

    if (iScale < 1 || iScale > 4 || !FCompatFontSizeUtf8(lpfont, psz, cch, &size)) return FALSE;
    if (cch == 0) return TRUE;
    if (iScale != 1 && lpfont->iScale != iScale) {
        lf = lpfont->lf;
        lf.lfHeight = -(int)lpfont->face->size->metrics.y_ppem * iScale;
        lf.lfWidth *= iScale;
        FreeCompatFont(lpfont->lpScaled);
        lpfont->lpScaled = LpNewFontRaster(&lf, 2048);
        lpfont->iScale = lpfont->lpScaled ? iScale : 0;
        if (!lpfont->lpScaled) return FALSE;
    }
    while (i < cch) {
        c = CbUtf8Char(psz + i, (size_t)(cch - i), &ch);
        if (c == 0) return FALSE;
        lp = LpGlyphUtf8(lpfont, ch);
        lpRaster = iScale == 1 ? lp : LpGlyphUtf8(lpfont->lpScaled, ch);
        if (!lp || !lpRaster) return FALSE;
        for (y = 0; y < lpRaster->cy; y++) for (x = 0; x < lpRaster->cx; x++)
            if (lpRaster->rgb[(size_t)y * lpRaster->cx + x])
                pfn(lpv, dx * iScale + lpRaster->x + x,
                    lpfont->tm.tmAscent * iScale - (iScale == 1 ? lpfont : lpfont->lpScaled)->tm.tmAscent + lpRaster->y + y,
                    lpRaster->rgb[(size_t)y * lpRaster->cx + x]);
        // Keep logical advances and baselines: Retina rasterization must not reflow the UI.
        if (rgdx) for (j = 0; j < c; j++) dx += rgdx[i + j];
        else dx += lp->dx;
        i += c;
    }
    return TRUE;
}

void CompatShutdownFonts(void) {
    if (lpConfig) FcConfigDestroy(lpConfig);
    lpConfig = NULL;
    if (library) FT_Done_FreeType(library);
    library = NULL;
}
