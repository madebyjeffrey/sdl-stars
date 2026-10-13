/* Decode standard dialog resources before creating any window or control. */
#include "internal.h"
#include <stdlib.h>
#include <string.h>

static BOOL FDlgWord(const BYTE *rgb, size_t cb, size_t *lpib, WORD *lpw) {
    if (*lpib > cb || cb - *lpib < 2) return FALSE;
    *lpw = (WORD)(rgb[*lpib] | (rgb[*lpib + 1] << 8));
    *lpib += 2;
    return TRUE;
}
static BOOL FDlgDword(const BYTE *rgb, size_t cb, size_t *lpib, DWORD *lpdw) {
    WORD lo, hi;
    if (!FDlgWord(rgb, cb, lpib, &lo) || !FDlgWord(rgb, cb, lpib, &hi)) return FALSE;
    *lpdw = (DWORD)lo | ((DWORD)hi << 16);
    return TRUE;
}
static BOOL FDlgRect(const BYTE *rgb, size_t cb, size_t *lpib, int16_t *lpx, int16_t *lpy, int16_t *lpcx, int16_t *lpcy) {
    WORD x, y, cx, cy;
    if (!FDlgWord(rgb, cb, lpib, &x) || !FDlgWord(rgb, cb, lpib, &y) ||
        !FDlgWord(rgb, cb, lpib, &cx) || !FDlgWord(rgb, cb, lpib, &cy)) return FALSE;
    *lpx = (int16_t)x; *lpy = (int16_t)y; *lpcx = (int16_t)cx; *lpcy = (int16_t)cy;
    return *lpcx >= 0 && *lpcy >= 0;
}
static BOOL FDlgField(const BYTE *rgb, size_t cb, size_t *lpib, WORD *lpid, char **lppsz) {
    WORD w;
    size_t ibStart = *lpib, cch = 0, i;
    UINT ch;
    char *psz;
    if (!FDlgWord(rgb, cb, lpib, &w)) return FALSE;
    if (w == 65535) return FDlgWord(rgb, cb, lpib, lpid);
    while (w) {
        cch++;
        if (!FDlgWord(rgb, cb, lpib, &w)) return FALSE;
    }
    psz = malloc(cch + 1);
    if (!psz) return FALSE;
    for (i = 0; i < cch; i++) {
        w = (WORD)(rgb[ibStart + i * 2] | (rgb[ibStart + i * 2 + 1] << 8));
        for (ch = 1; ch < 256 && DwCompatAnsiChar((BYTE)ch) != w; ch++) {}
        if (ch == 256) { free(psz); return FALSE; }
        psz[i] = (char)ch;
    }
    psz[cch] = 0; *lppsz = psz;
    return TRUE;
}
void CompatFreeDialogTemplate(COMPATDLGTEMPLATE *lpdlg) {
    WORD i;
    if (!lpdlg) return;
    for (i = 0; lpdlg->rgItems && i < lpdlg->cItems; i++) {
        free(lpdlg->rgItems[i].pszClass); free(lpdlg->rgItems[i].pszTitle);
    }
    free(lpdlg->rgItems); free(lpdlg->pszMenu); free(lpdlg->pszClass);
    free(lpdlg->pszTitle); free(lpdlg->pszFont);
    memset(lpdlg, 0, sizeof(*lpdlg));
}
BOOL FCompatDialogTemplate(const BYTE *rgb, size_t cb, COMPATDLGTEMPLATE *lpdlg) {
    COMPATDLGTEMPLATE dlg = {0};
    COMPATDLGITEM *lpitem;
    size_t ib = 0;
    WORD i, idFont = 0;
    if (!rgb || !lpdlg || (cb >= 4 && rgb[2] == 255 && rgb[3] == 255) ||
        !FDlgDword(rgb, cb, &ib, &dlg.dwStyle) ||
        !FDlgDword(rgb, cb, &ib, &dlg.dwExStyle) ||
        !FDlgWord(rgb, cb, &ib, &dlg.cItems) ||
        !FDlgRect(rgb, cb, &ib, &dlg.x, &dlg.y, &dlg.cx, &dlg.cy) ||
        !FDlgField(rgb, cb, &ib, &dlg.idMenu, &dlg.pszMenu) ||
        !FDlgField(rgb, cb, &ib, &dlg.idClass, &dlg.pszClass) ||
        !FDlgField(rgb, cb, &ib, &idFont, &dlg.pszTitle) || idFont) goto fail;
    if (dlg.dwStyle & DS_SETFONT) {
        if (!FDlgWord(rgb, cb, &ib, &dlg.cPoints) ||
            !FDlgField(rgb, cb, &ib, &idFont, &dlg.pszFont) || idFont) goto fail;
    }
    // Even an ordinal class/title needs a complete fixed header and three fields.
    if (dlg.cItems > (cb - ib) / 24) goto fail;
    if (dlg.cItems && !(dlg.rgItems = calloc(dlg.cItems, sizeof(*dlg.rgItems)))) goto fail;
    for (i = 0; i < dlg.cItems; i++) {
        ib += (4 - ib % 4) % 4;
        lpitem = &dlg.rgItems[i];
        if (!FDlgDword(rgb, cb, &ib, &lpitem->dwStyle) || !FDlgDword(rgb, cb, &ib, &lpitem->dwExStyle) ||
            !FDlgRect(rgb, cb, &ib, &lpitem->x, &lpitem->y, &lpitem->cx, &lpitem->cy) ||
            !FDlgWord(rgb, cb, &ib, &lpitem->id) ||
            !FDlgField(rgb, cb, &ib, &lpitem->idClass, &lpitem->pszClass) ||
            !FDlgField(rgb, cb, &ib, &lpitem->idTitle, &lpitem->pszTitle) ||
            !FDlgWord(rgb, cb, &ib, &lpitem->cbCreate)) goto fail;
        if (lpitem->cbCreate) {
            // The byte count includes its own WORD in a standard template.
            if (lpitem->cbCreate < 2 || lpitem->cbCreate - 2u > cb - ib) goto fail;
            lpitem->rgbCreate = rgb + ib; ib += lpitem->cbCreate - 2;
        }
    }
    if (ib > cb || cb - ib > 3) goto fail;
    while (ib < cb) if (rgb[ib++]) goto fail;
    *lpdlg = dlg; return TRUE;
fail:
    CompatFreeDialogTemplate(&dlg); return FALSE;
}
