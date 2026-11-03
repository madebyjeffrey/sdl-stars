/* Decode standard dialog resources before creating any window or control. */
#include "internal.h"
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

static BOOL FDlgWord(std::span<const BYTE> rgb, std::size_t &ib, WORD &w) {
    if (ib > rgb.size() || rgb.size() - ib < 2) return 0;
    w = static_cast<WORD>((rgb[ib] | (rgb[ib + 1] << 8)));
    ib += 2;
    return 1;
}
static BOOL FDlgDword(std::span<const BYTE> rgb, std::size_t &ib, DWORD &dw) {
    WORD lo, hi;
    if (!FDlgWord(rgb, ib, lo) || !FDlgWord(rgb, ib, hi)) return 0;
    dw = static_cast<DWORD>(lo) | (static_cast<DWORD>(hi) << 16);
    return 1;
}
static BOOL FDlgRect(std::span<const BYTE> rgb, std::size_t &ib, std::int16_t &x, std::int16_t &y,
                     std::int16_t &cx, std::int16_t &cy) {
    WORD wX, wY, wCx, wCy;
    if (!FDlgWord(rgb, ib, wX) || !FDlgWord(rgb, ib, wY) ||
        !FDlgWord(rgb, ib, wCx) || !FDlgWord(rgb, ib, wCy)) return 0;
    x = static_cast<std::int16_t>(wX); y = static_cast<std::int16_t>(wY);
    cx = static_cast<std::int16_t>(wCx); cy = static_cast<std::int16_t>(wCy);
    return cx >= 0 && cy >= 0;
}
static BOOL FDlgField(std::span<const BYTE> rgb, std::size_t &ib, WORD &id,
                      std::optional<std::string> &sz) {
    WORD w;
    std::size_t ibStart = ib, cch = 0;
    if (!FDlgWord(rgb, ib, w)) return 0;
    if (w == 65535) {
        sz.reset();
        return FDlgWord(rgb, ib, id);
    }
    while (w) {
        cch++;
        if (!FDlgWord(rgb, ib, w)) return 0;
    }
    if (cch > (static_cast<std::size_t>(std::numeric_limits<int>::max()) - 1) / 3) return 0;
    std::string szField(cch * 3 + 1, '\0');
    int cchUtf8 = CchUtf16leToUtf8(std::span{szField}, std::as_bytes(rgb.subspan(ibStart, cch * sizeof(WORD))));
    if (cchUtf8 < 0) return 0;
    szField.resize(static_cast<std::size_t>(cchUtf8));
    sz = std::move(szField);
    return 1;
}
void CompatFreeDialogTemplate(COMPATDLGTEMPLATE *lpdlg) {
    if (!lpdlg) return;
    *lpdlg = {};
}
BOOL FCompatDialogTemplate(std::span<const BYTE> rgb, COMPATDLGTEMPLATE &dlgResult) {
    try {
    COMPATDLGTEMPLATE dlg = {};
    COMPATDLGITEM *lpitem;
    std::size_t ib = 0;
    WORD i, idFont = 0;
    if ((rgb.size() >= 4 && rgb[2] == 255 && rgb[3] == 255) ||
        !FDlgDword(rgb, ib, dlg.dwStyle) ||
        !FDlgDword(rgb, ib, dlg.dwExStyle) ||
        !FDlgWord(rgb, ib, dlg.cItems) ||
        !FDlgRect(rgb, ib, dlg.x, dlg.y, dlg.cx, dlg.cy) ||
        !FDlgField(rgb, ib, dlg.idMenu, dlg.szMenu) ||
        !FDlgField(rgb, ib, dlg.idClass, dlg.szClass) ||
        !FDlgField(rgb, ib, idFont, dlg.szTitle) || idFont) goto fail;
    if (dlg.dwStyle & win32::DS_SETFONT) {
        if (!FDlgWord(rgb, ib, dlg.cPoints) ||
            !FDlgField(rgb, ib, idFont, dlg.szFont) || idFont) goto fail;
    }
    // Even an ordinal class/title needs a complete fixed header and three fields.
    if (ib > rgb.size() || dlg.cItems > (rgb.size() - ib) / 24) goto fail;
    dlg.rgItems.resize(dlg.cItems);
    for (i = 0; i < dlg.cItems; i++) {
        ib += (4 - ib % 4) % 4;
        lpitem = &dlg.rgItems[i];
        if (!FDlgDword(rgb, ib, lpitem->dwStyle) || !FDlgDword(rgb, ib, lpitem->dwExStyle) ||
            !FDlgRect(rgb, ib, lpitem->x, lpitem->y, lpitem->cx, lpitem->cy) ||
            !FDlgWord(rgb, ib, lpitem->id) ||
            !FDlgField(rgb, ib, lpitem->idClass, lpitem->szClass) ||
            !FDlgField(rgb, ib, lpitem->idTitle, lpitem->szTitle) ||
            !FDlgWord(rgb, ib, lpitem->cbCreate)) goto fail;
        if (lpitem->cbCreate) {
            // The byte count includes its own WORD in a standard template.
            if (lpitem->cbCreate < 2 || lpitem->cbCreate - 2u > rgb.size() - ib) goto fail;
            lpitem->rgbCreate = rgb.data() + ib; ib += lpitem->cbCreate - 2;
        }
    }
    if (ib > rgb.size() || rgb.size() - ib > 3) goto fail;
    while (ib < rgb.size()) if (rgb[ib++]) goto fail;
    dlgResult = std::move(dlg); return 1;
fail:
    return 0;
    } catch (const std::bad_alloc &) {
        return 0;
    } catch (const std::length_error &) {
        return 0;
    }
}
