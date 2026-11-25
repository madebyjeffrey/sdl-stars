/* Decode standard dialog resources before creating any window or control. */
#include "internal.h"
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

struct DIALOGRECTRESULT { std::int16_t x, y, cx, cy; };
struct DIALOGFIELDRESULT { WORD id; std::optional<std::string> sz; };

static std::optional<WORD> WDialogWord(std::span<const BYTE> rgb, std::size_t &ib) {
    WORD w;
    if (ib > rgb.size() || rgb.size() - ib < 2) return std::nullopt;
    w = static_cast<WORD>((rgb[ib] | (rgb[ib + 1] << 8)));
    ib += 2;
    return w;
}
static std::optional<DWORD> DwDialogDword(std::span<const BYTE> rgb, std::size_t &ib) {
    std::optional<WORD> wLo = WDialogWord(rgb, ib), wHi;
    if (!wLo) return std::nullopt;
    wHi = WDialogWord(rgb, ib);
    if (!wHi) return std::nullopt;
    return static_cast<DWORD>(*wLo) | (static_cast<DWORD>(*wHi) << 16);
}
static std::optional<DIALOGRECTRESULT> RcdlgTemplate(std::span<const BYTE> rgb, std::size_t &ib) {
    std::optional<WORD> wX = WDialogWord(rgb, ib), wY, wCx, wCy;
    DIALOGRECTRESULT result;
    if (!wX) return std::nullopt;
    wY = WDialogWord(rgb, ib); if (!wY) return std::nullopt;
    wCx = WDialogWord(rgb, ib); if (!wCx) return std::nullopt;
    wCy = WDialogWord(rgb, ib); if (!wCy) return std::nullopt;
    result = {static_cast<std::int16_t>(*wX), static_cast<std::int16_t>(*wY),
              static_cast<std::int16_t>(*wCx), static_cast<std::int16_t>(*wCy)};
    if (result.cx < 0 || result.cy < 0) return std::nullopt;
    return result;
}
static std::optional<DIALOGFIELDRESULT> FDialogTemplateField(std::span<const BYTE> rgb, std::size_t &ib) {
    std::size_t ibStart = ib, cch = 0;
    std::optional<WORD> w = WDialogWord(rgb, ib);
    if (!w) return std::nullopt;
    if (*w == 65535) {
        w = WDialogWord(rgb, ib);
        if (!w) return std::nullopt;
        return DIALOGFIELDRESULT{*w, std::nullopt};
    }
    while (*w) {
        cch++;
        w = WDialogWord(rgb, ib);
        if (!w) return std::nullopt;
    }
    if (cch > (static_cast<std::size_t>(std::numeric_limits<int>::max()) - 1) / 3) return std::nullopt;
    std::string szField(cch * 3 + 1, '\0');
    int cchUtf8 = CchUtf16leToUtf8(std::span{szField}, std::as_bytes(rgb.subspan(ibStart, cch * sizeof(WORD))));
    if (cchUtf8 < 0) return std::nullopt;
    szField.resize(static_cast<std::size_t>(cchUtf8));
    return DIALOGFIELDRESULT{0, std::move(szField)};
}
void CompatFreeDialogTemplate(COMPATDLGTEMPLATE *lpdlg) {
    if (!lpdlg) return;
    *lpdlg = {};
}
bool FCompatDialogTemplate(std::span<const BYTE> rgb, COMPATDLGTEMPLATE &dlgResult) {
    try {
    COMPATDLGTEMPLATE dlg = {};
    COMPATDLGITEM *lpitem;
    std::size_t ib = 0;
    WORD i;
    std::optional<WORD> w, idFont;
    std::optional<DWORD> dw;
    std::optional<DIALOGRECTRESULT> rc;
    std::optional<DIALOGFIELDRESULT> field;
    if (rgb.size() >= 4 && rgb[2] == 255 && rgb[3] == 255) goto fail;
    dw = DwDialogDword(rgb, ib); if (!dw) goto fail; dlg.dwStyle = *dw;
    dw = DwDialogDword(rgb, ib); if (!dw) goto fail; dlg.dwExStyle = *dw;
    w = WDialogWord(rgb, ib); if (!w) goto fail; dlg.cItems = *w;
    rc = RcdlgTemplate(rgb, ib); if (!rc) goto fail;
    dlg.x = rc->x; dlg.y = rc->y; dlg.cx = rc->cx; dlg.cy = rc->cy;
    field = FDialogTemplateField(rgb, ib); if (!field) goto fail;
    dlg.idMenu = field->id; dlg.szMenu = std::move(field->sz);
    field = FDialogTemplateField(rgb, ib); if (!field) goto fail;
    dlg.idClass = field->id; dlg.szClass = std::move(field->sz);
    field = FDialogTemplateField(rgb, ib); if (!field) goto fail;
    idFont = field->id; dlg.szTitle = std::move(field->sz);
    if (idFont.value_or(0) != 0) goto fail;
    if (dlg.dwStyle & win32::DS_SETFONT) {
        w = WDialogWord(rgb, ib); if (!w) goto fail; dlg.cPoints = *w;
        field = FDialogTemplateField(rgb, ib); if (!field) goto fail;
        idFont = field->id; dlg.szFont = std::move(field->sz);
        if (idFont.value_or(0) != 0) goto fail;
    }
    // Even an ordinal class/title needs a complete fixed header and three fields.
    if (ib > rgb.size() || dlg.cItems > (rgb.size() - ib) / 24) goto fail;
    dlg.rgItems.resize(dlg.cItems);
    for (i = 0; i < dlg.cItems; i++) {
        ib += (4 - ib % 4) % 4;
        lpitem = &dlg.rgItems[i];
        dw = DwDialogDword(rgb, ib); if (!dw) goto fail; lpitem->dwStyle = *dw;
        dw = DwDialogDword(rgb, ib); if (!dw) goto fail; lpitem->dwExStyle = *dw;
        rc = RcdlgTemplate(rgb, ib); if (!rc) goto fail;
        lpitem->x = rc->x; lpitem->y = rc->y; lpitem->cx = rc->cx; lpitem->cy = rc->cy;
        w = WDialogWord(rgb, ib); if (!w) goto fail; lpitem->id = *w;
        field = FDialogTemplateField(rgb, ib); if (!field) goto fail;
        lpitem->idClass = field->id; lpitem->szClass = std::move(field->sz);
        field = FDialogTemplateField(rgb, ib); if (!field) goto fail;
        lpitem->idTitle = field->id; lpitem->szTitle = std::move(field->sz);
        w = WDialogWord(rgb, ib); if (!w) goto fail; lpitem->cbCreate = *w;
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
