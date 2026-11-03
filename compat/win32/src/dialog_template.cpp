/* Decode standard dialog resources before creating any window or control. */
#include "internal.h"
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

static BOOL FDlgWord(const BYTE *rgb, std::size_t cb, std::size_t *lpib, WORD *lpw) {
    if (*lpib > cb || cb - *lpib < 2) return 0;
    *lpw = static_cast<WORD>((rgb[*lpib] | (rgb[*lpib + 1] << 8)));
    *lpib += 2;
    return 1;
}
static BOOL FDlgDword(const BYTE *rgb, std::size_t cb, std::size_t *lpib, DWORD *lpdw) {
    WORD lo, hi;
    if (!FDlgWord(rgb, cb, lpib, &lo) || !FDlgWord(rgb, cb, lpib, &hi)) return 0;
    *lpdw = static_cast<DWORD>(lo) | (static_cast<DWORD>(hi) << 16);
    return 1;
}
static BOOL FDlgRect(const BYTE *rgb, std::size_t cb, std::size_t *lpib, std::int16_t *lpx, std::int16_t *lpy, std::int16_t *lpcx, std::int16_t *lpcy) {
    WORD x, y, cx, cy;
    if (!FDlgWord(rgb, cb, lpib, &x) || !FDlgWord(rgb, cb, lpib, &y) ||
        !FDlgWord(rgb, cb, lpib, &cx) || !FDlgWord(rgb, cb, lpib, &cy)) return 0;
    *lpx = static_cast<std::int16_t>(x); *lpy = static_cast<std::int16_t>(y); *lpcx = static_cast<std::int16_t>(cx); *lpcy = static_cast<std::int16_t>(cy);
    return *lpcx >= 0 && *lpcy >= 0;
}
static BOOL FDlgField(const BYTE *rgb, std::size_t cb, std::size_t *lpib, WORD *lpid,
                      std::optional<std::string> *lpsz) {
    WORD w;
    std::size_t ibStart = *lpib, cch = 0;
    if (!FDlgWord(rgb, cb, lpib, &w)) return 0;
    if (w == 65535) {
        lpsz->reset();
        return FDlgWord(rgb, cb, lpib, lpid);
    }
    while (w) {
        cch++;
        if (!FDlgWord(rgb, cb, lpib, &w)) return 0;
    }
    if (cch > (static_cast<std::size_t>(std::numeric_limits<int>::max()) - 1) / 3) return 0;
    std::string sz(cch * 3 + 1, '\0');
    int cchUtf8 = CchUtf16leToUtf8(std::span{sz},
                                   std::span{reinterpret_cast<const std::byte *>(rgb + ibStart), cch * sizeof(WORD)});
    if (cchUtf8 < 0) return 0;
    sz.resize(static_cast<std::size_t>(cchUtf8));
    *lpsz = std::move(sz);
    return 1;
}
void CompatFreeDialogTemplate(COMPATDLGTEMPLATE *lpdlg) {
    if (!lpdlg) return;
    *lpdlg = {};
}
BOOL FCompatDialogTemplate(const BYTE *rgb, std::size_t cb, COMPATDLGTEMPLATE *lpdlg) {
    try {
    COMPATDLGTEMPLATE dlg = {};
    COMPATDLGITEM *lpitem;
    std::size_t ib = 0;
    WORD i, idFont = 0;
    if (!rgb || !lpdlg || (cb >= 4 && rgb[2] == 255 && rgb[3] == 255) ||
        !FDlgDword(rgb, cb, &ib, &dlg.dwStyle) ||
        !FDlgDword(rgb, cb, &ib, &dlg.dwExStyle) ||
        !FDlgWord(rgb, cb, &ib, &dlg.cItems) ||
        !FDlgRect(rgb, cb, &ib, &dlg.x, &dlg.y, &dlg.cx, &dlg.cy) ||
        !FDlgField(rgb, cb, &ib, &dlg.idMenu, &dlg.szMenu) ||
        !FDlgField(rgb, cb, &ib, &dlg.idClass, &dlg.szClass) ||
        !FDlgField(rgb, cb, &ib, &idFont, &dlg.szTitle) || idFont) goto fail;
    if (dlg.dwStyle & win32::DS_SETFONT) {
        if (!FDlgWord(rgb, cb, &ib, &dlg.cPoints) ||
            !FDlgField(rgb, cb, &ib, &idFont, &dlg.szFont) || idFont) goto fail;
    }
    // Even an ordinal class/title needs a complete fixed header and three fields.
    if (dlg.cItems > (cb - ib) / 24) goto fail;
    dlg.rgItems.resize(dlg.cItems);
    for (i = 0; i < dlg.cItems; i++) {
        ib += (4 - ib % 4) % 4;
        lpitem = &dlg.rgItems[i];
        if (!FDlgDword(rgb, cb, &ib, &lpitem->dwStyle) || !FDlgDword(rgb, cb, &ib, &lpitem->dwExStyle) ||
            !FDlgRect(rgb, cb, &ib, &lpitem->x, &lpitem->y, &lpitem->cx, &lpitem->cy) ||
            !FDlgWord(rgb, cb, &ib, &lpitem->id) ||
            !FDlgField(rgb, cb, &ib, &lpitem->idClass, &lpitem->szClass) ||
            !FDlgField(rgb, cb, &ib, &lpitem->idTitle, &lpitem->szTitle) ||
            !FDlgWord(rgb, cb, &ib, &lpitem->cbCreate)) goto fail;
        if (lpitem->cbCreate) {
            // The byte count includes its own WORD in a standard template.
            if (lpitem->cbCreate < 2 || lpitem->cbCreate - 2u > cb - ib) goto fail;
            lpitem->rgbCreate = rgb + ib; ib += lpitem->cbCreate - 2;
        }
    }
    if (ib > cb || cb - ib > 3) goto fail;
    while (ib < cb) if (rgb[ib++]) goto fail;
    *lpdlg = std::move(dlg); return 1;
fail:
    return 0;
    } catch (const std::bad_alloc &) {
        return 0;
    } catch (const std::length_error &) {
        return 0;
    }
}
