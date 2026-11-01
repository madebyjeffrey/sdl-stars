/* UTF-8 edit controls retain byte offsets at the game-to-UI boundary. */
#include "internal.h"
#include "choice_io.h"
#include <stars_compat.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum { editUndoSeparate, editUndoTyping, editUndoBackspace, editUndoDelete, editUndoReplace };

static void NotifyEdit(HWND hwnd, UINT code) {
    HWND lp = LpCompatWindow(hwnd);
    if (lp && lp->hwndParent) win32::SendMessageA(lp->hwndParent, win32::WM_COMMAND, win32::MAKEWPARAM(lp->idChild, code), (LPARAM)hwnd);
}

static void SyncEditScroll(HWND hwnd);

void CompatEditComposition(HWND hwnd, const char *pszUtf, int ich, int cch) {
    char *psz;
    (void)ich; (void)cch;
    if (!win32::IsWindow(hwnd) || hwnd->iControl != 3 || (hwnd->dwStyle & win32::ES_READONLY)) return;
    if (!pszUtf) pszUtf = "";
    if (!FUtf8Valid(pszUtf, strlen(pszUtf)) || !(psz = SDL_strdup(pszUtf))) return;
    SDL_free(hwnd->pszEditComposition); hwnd->pszEditComposition = psz;
    hwnd->ichComposition = hwnd->cchComposition = 0;
    win32::InvalidateRect(hwnd, nullptr, 1);
}

static UINT CchEdit(HWND hwnd) { return (UINT)strlen(hwnd->pszTitle); }

static UINT IchEditBoundary(HWND hwnd, UINT ich) {
    UINT i = 0, next, cch = CchEdit(hwnd);
    uint32_t ch;
    int c;
    ich = StarsMin(ich, cch);
    while (i < ich) {
        c = CbUtf8Char(hwnd->pszTitle + i, cch - i, &ch);
        next = i + (c == 0 ? 1 : (UINT)c);
        if (next > ich) break;
        i = next;
    }
    return i;
}

static UINT IchEditNext(HWND hwnd, UINT ich) {
    UINT cch = CchEdit(hwnd);
    uint32_t ch;
    int c;
    if (ich >= cch) return cch;
    c = CbUtf8Char(hwnd->pszTitle + ich, cch - ich, &ch);
    return ich + (c == 0 ? 1 : (UINT)c);
}

static UINT IchEditPrev(HWND hwnd, UINT ich) {
    return ich == 0 ? 0 : IchEditBoundary(hwnd, ich - 1);
}

static void SelectEdit(HWND hwnd, UINT ichAnchor, UINT ichCaret) {
    UINT cch = CchEdit(hwnd);
    hwnd->ichAnchor = IchEditBoundary(hwnd, StarsMin(ichAnchor, cch));
    hwnd->ichCaret = IchEditBoundary(hwnd, StarsMin(ichCaret, cch));
    hwnd->fEditTyping = 0;
    hwnd->iEditUndoAction = editUndoSeparate;
    hwnd->fEditManualScroll = 0;
    win32::InvalidateRect(hwnd, nullptr, 1);
}

static void ChangedEdit(HWND hwnd, BOOL fNotify) {
    if (fNotify) hwnd->fEditInputRejected = 0;
    SyncEditScroll(hwnd);
    if (!win32::IsWindow(hwnd)) return;
    win32::InvalidateRect(hwnd, nullptr, 1);
    if (fNotify) {
        NotifyEdit(hwnd, win32::EN_UPDATE);
        if (win32::IsWindow(hwnd)) NotifyEdit(hwnd, win32::EN_CHANGE);
    }
}

static void EmptyEditUndo(HWND hwnd) {
    SDL_free(hwnd->pszEditUndo);
    hwnd->pszEditUndo = nullptr;
    hwnd->fEditTyping = 0; hwnd->iEditUndoAction = editUndoSeparate;
}

static BYTE ChEditCase(BYTE ch, BOOL fUpper) {
    if (fUpper && ch >= 'a' && ch <= 'z') return ch - 32;
    if (!fUpper && ch >= 'A' && ch <= 'Z') return ch + 32;
    return ch;
}

static void NormalizeEditCase(HWND hwnd) {
    char *pch;
    if (!(hwnd->dwStyle & (win32::ES_UPPERCASE | win32::ES_LOWERCASE))) return;
    for (pch = hwnd->pszTitle; *pch; pch++)
        *pch = (char)ChEditCase((BYTE)*pch, (hwnd->dwStyle & win32::ES_UPPERCASE) != 0);
}

static void ReplaceEdit(HWND hwnd, LPCSTR psz, BOOL fUser, BOOL fUndo, UINT iUndoAction) {
    UINT ichFirst = StarsMin(hwnd->ichAnchor, hwnd->ichCaret), ichLast = StarsMax(hwnd->ichAnchor, hwnd->ichCaret);
    size_t cchOld = CchEdit(hwnd), cch = strlen(psz), cchBase = cchOld - (ichLast - ichFirst), i;
    char *pszNew, *pszUndo = nullptr;
    BOOL fLimit, fGroup;
    fGroup = iUndoAction != editUndoSeparate && hwnd->pszEditUndo &&
             iUndoAction == hwnd->iEditUndoAction &&
             (iUndoAction == editUndoBackspace ? ichLast == hwnd->ichTypingEnd : ichFirst == hwnd->ichTypingEnd);
    /* Adjacent removals share an undo range even when Delete and Backspace
       alternate. Cursor/selection changes still terminate the group. */
    if (hwnd->pszEditUndo && hwnd->cchUndoAfter == 0 &&
        (iUndoAction == editUndoBackspace || iUndoAction == editUndoDelete) &&
        (hwnd->iEditUndoAction == editUndoBackspace || hwnd->iEditUndoAction == editUndoDelete) &&
        (ichFirst == hwnd->ichUndoPosition || ichLast == hwnd->ichUndoPosition)) fGroup = 1;
    if (hwnd->pszEditUndo && iUndoAction == editUndoTyping && hwnd->iEditUndoAction == editUndoReplace &&
        ichFirst == ichLast && ichFirst == hwnd->ichTypingEnd) fGroup = 1;
    if (!FUtf8Valid(psz, strlen(psz))) {
        hwnd->fEditInputRejected = 1;
        return;
    }
    if (fUser && (hwnd->dwStyle & win32::ES_READONLY)) return;
    fLimit = cch > (hwnd->cchLimit > cchBase ? hwnd->cchLimit - cchBase : 0);
    if (fLimit && fUser && (hwnd->dwStyle & win32::ES_PASSWORD)) {
        hwnd->fEditInputRejected = 1;
        hwnd->fModified = 1;
        NotifyEdit(hwnd, win32::EN_MAXTEXT);
        return;
    }
    if (fLimit) cch = hwnd->cchLimit > cchBase ? hwnd->cchLimit - cchBase : 0;
    cch = CchUtf8Prefix(psz, cch);
    if (fLimit && cch == 0 && ichFirst == ichLast) {
        // The SDK still notifies at the limit, but no text entered the undo buffer.
        hwnd->fModified = 1; NotifyEdit(hwnd, win32::EN_MAXTEXT);
        if (win32::IsWindow(hwnd)) ChangedEdit(hwnd, 1);
        return;
    }
    if (cchBase > INT_MAX || cch > INT_MAX - cchBase) return;
    pszNew = (char *)(SDL_malloc(cchBase + cch + 1));
    if (!pszNew) { NotifyEdit(hwnd, win32::EN_ERRSPACE); return; }
    if (fUndo && !fGroup && !(pszUndo = SDL_strdup(hwnd->pszTitle))) {
        SDL_free(pszNew); NotifyEdit(hwnd, win32::EN_ERRSPACE); return;
    }
    memcpy(pszNew, hwnd->pszTitle, ichFirst);
    memcpy(pszNew + ichFirst, psz, cch);
    memcpy(pszNew + ichFirst + cch, hwnd->pszTitle + ichLast, cchOld - ichLast + 1);
    for (i = ichFirst; i < ichFirst + cch; i++) {
        if (hwnd->dwStyle & win32::ES_UPPERCASE) pszNew[i] = (char)ChEditCase((BYTE)pszNew[i], 1);
        else if (hwnd->dwStyle & win32::ES_LOWERCASE) pszNew[i] = (char)ChEditCase((BYTE)pszNew[i], 0);
    }
    if (!fGroup) {
        EmptyEditUndo(hwnd); hwnd->pszEditUndo = pszUndo;
        hwnd->ichUndoPosition = ichFirst;
        hwnd->cchUndoBefore = ichLast - ichFirst; hwnd->cchUndoAfter = (UINT)cch;
    } else {
        if (iUndoAction == editUndoBackspace) hwnd->ichUndoPosition = ichFirst;
        hwnd->cchUndoBefore += ichLast - ichFirst; hwnd->cchUndoAfter += (UINT)cch;
    }
    hwnd->fEditTyping = iUndoAction == editUndoTyping;
    hwnd->iEditUndoAction = iUndoAction;
    hwnd->ichTypingEnd = ichFirst + (UINT)cch;
    hwnd->fEditManualScroll = 0;
    SDL_free(hwnd->pszTitle); hwnd->pszTitle = pszNew;
    hwnd->ichAnchor = hwnd->ichCaret = ichFirst + (UINT)cch;
    hwnd->fModified = 1;
    if (fLimit) NotifyEdit(hwnd, win32::EN_MAXTEXT);
    if (win32::IsWindow(hwnd)) ChangedEdit(hwnd, 1);
}

static BOOL UndoEdit(HWND hwnd) {
    char *psz;
    UINT ichFirst, ichLast, cchBefore;
    hwnd->fEditTyping = 0;
    hwnd->iEditUndoAction = editUndoSeparate;
    if (hwnd->dwStyle & win32::ES_READONLY) return !(hwnd->dwStyle & win32::ES_MULTILINE);
    if (!hwnd->pszEditUndo) {
        // Clearing history retains its position; an empty undo still notifies.
        SelectEdit(hwnd, hwnd->ichUndoPosition, hwnd->ichUndoPosition);
        ChangedEdit(hwnd, 0);
        if (win32::IsWindow(hwnd)) NotifyEdit(hwnd, win32::EN_CHANGE);
        return 1;
    }
    psz = hwnd->pszTitle; hwnd->pszTitle = hwnd->pszEditUndo; hwnd->pszEditUndo = psz;
    ichFirst = hwnd->ichUndoPosition; ichLast = ichFirst + hwnd->cchUndoBefore;
    cchBefore = hwnd->cchUndoBefore; hwnd->cchUndoBefore = hwnd->cchUndoAfter; hwnd->cchUndoAfter = cchBefore;
    hwnd->fModified = 1;
    SelectEdit(hwnd, ichLast, ichLast);
    ChangedEdit(hwnd, 1);
    if (win32::IsWindow(hwnd)) {
        // The final change notification observes the restored range, not just its caret.
        SelectEdit(hwnd, ichFirst, ichLast);
        NotifyEdit(hwnd, win32::EN_CHANGE);
    }
    return 1;
}

static BOOL CopyEdit(HWND hwnd) {
    UINT ichFirst = StarsMin(hwnd->ichAnchor, hwnd->ichCaret), ichLast = StarsMax(hwnd->ichAnchor, hwnd->ichCaret);
    char *psz;
    BOOL fOk;
    size_t cch = ichLast - ichFirst;
    if (ichFirst == ichLast || (hwnd->dwStyle & win32::ES_PASSWORD)) return 0;
    psz = (char *)(SDL_malloc(cch + 1));
    if (!psz) return 0;
    memcpy(psz, hwnd->pszTitle + ichFirst, cch); psz[cch] = 0;
    fOk = SDL_SetClipboardText(psz);
    SDL_free(psz);
    return fOk;
}

static void PasteEdit(HWND hwnd) {
    char *pszUtf, *pch;
    if (hwnd->dwStyle & win32::ES_READONLY) return;
    pszUtf = SDL_GetClipboardText();
    if (!pszUtf) return;
    if (!FUtf8Valid(pszUtf, strlen(pszUtf))) {
        hwnd->fEditInputRejected = 1;
        SDL_free(pszUtf); return;
    }
    pch = strpbrk(pszUtf, "\r\n");
    if (pch && !(hwnd->dwStyle & win32::ES_MULTILINE)) *pch = 0;
    ReplaceEdit(hwnd, pszUtf, 1, 1, editUndoSeparate);
    SDL_free(pszUtf);
}

static void EditFormat(HWND hwnd, RECT *lprc) {
    if (hwnd->fEditFormat) *lprc = hwnd->rcEditFormat;
    else { win32::GetClientRect(hwnd, lprc); win32::InflateRect(lprc, -2, -2); }
    lprc->right = StarsMax(lprc->left + 1, lprc->right);
    lprc->bottom = StarsMax(lprc->top + 1, lprc->bottom);
}

static UINT IchLineEnd(HWND hwnd, UINT ich) {
    UINT cch = CchEdit(hwnd), ichEnd = ich, lo, hi, mid, i;
    HDC hdc;
    RECT rc;
    SIZE size;
    while (ichEnd < cch && hwnd->pszTitle[ichEnd] != '\r' && hwnd->pszTitle[ichEnd] != '\n') ichEnd++;
    if (!(hwnd->dwStyle & win32::ES_MULTILINE) || ((hwnd->dwStyle & win32::ES_AUTOHSCROLL) || hwnd->fEditH) || ichEnd == ich) return ichEnd;
    EditFormat(hwnd, &rc);
    hdc = win32::GetDC(hwnd);
    if (!hdc) return ichEnd;
    if (hwnd->hfontControl) win32::SelectObject(hdc, hwnd->hfontControl);
    lo = ich; hi = ichEnd;
    while (lo < hi) {
        mid = lo + (hi - lo + 1) / 2;
        if (!win32::GetTextExtentPoint32A(hdc, hwnd->pszTitle + ich,
                                  (int)CchUtf8Prefix(hwnd->pszTitle + ich, mid - ich), &size)) {
            win32::ReleaseDC(hwnd, hdc);
            return ichEnd;
        }
        if (size.cx <= rc.right - rc.left) lo = mid; else hi = mid - 1;
    }
    win32::ReleaseDC(hwnd, hdc);
    if (lo == ichEnd) return ichEnd;
    lo = IchEditBoundary(hwnd, lo);
    if (lo == ich) lo = IchEditNext(hwnd, ich);
    for (i = lo; i > ich; i--) if (hwnd->pszTitle[i - 1] == ' ' || hwnd->pszTitle[i - 1] == '\t') return i;
    return lo;
}

static UINT IchNextLine(HWND hwnd, UINT ich) {
    ich = IchLineEnd(hwnd, ich);
    if (hwnd->pszTitle[ich] == '\r') ich++;
    if (hwnd->pszTitle[ich] == '\n') ich++;
    return ich;
}

static UINT IchLineStart(HWND hwnd, UINT ich) {
    UINT first = 0, next, cch = CchEdit(hwnd);
    ich = StarsMin(ich, cch);
    while (first < cch) {
        next = IchNextLine(hwnd, first);
        if (next <= first || next > ich || (next == cch && (cch == 0 || hwnd->pszTitle[cch - 1] != '\n'))) break;
        first = next;
    }
    return first;
}

static UINT IchEditLine(HWND hwnd, UINT iLine) {
    UINT ich = 0, next, i, cch = CchEdit(hwnd);
    for (i = 0; i < iLine; i++) {
        next = IchNextLine(hwnd, ich);
        if (next <= ich || (next == cch && (cch == 0 || hwnd->pszTitle[cch - 1] != '\n'))) return UINT_MAX;
        ich = next;
    }
    return ich;
}

static UINT IEditLine(HWND hwnd, UINT ich) {
    UINT first = 0, next, line = 0, cch = CchEdit(hwnd);
    while (first < cch) {
        next = IchNextLine(hwnd, first);
        if (next <= first || next > ich || (next == cch && (cch == 0 || hwnd->pszTitle[cch - 1] != '\n'))) break;
        first = next; line++;
    }
    return line;
}

static int CyEdit(HWND hwnd) {
    HDC hdc = win32::GetDC(hwnd);
    TEXTMETRICA tm = {};
    if (hdc) {
        if (hwnd->hfontControl) win32::SelectObject(hdc, hwnd->hfontControl);
        win32::GetTextMetricsA(hdc, &tm); win32::ReleaseDC(hwnd, hdc);
    }
    return StarsMax(1, tm.tmHeight);
}

static void SyncEditScroll(HWND hwnd) {
    SCROLLINFO si = {sizeof(si), win32::SIF_RANGE | win32::SIF_PAGE | win32::SIF_POS, 0, 0, 0, 0, 0};
    RECT rc;
    HDC hdc;
    SIZE size;
    UINT ich, next, end, cch;
    int cy, cxMax = 0;
    if (!(hwnd->dwStyle & win32::ES_MULTILINE) || hwnd->fEditScrollSync || hwnd->fDestroying) return;
    StarsRestore<BOOL> restoreSync(hwnd->fEditScrollSync);
    hwnd->fEditScrollSync = 1;
    EditFormat(hwnd, &rc); cy = CyEdit(hwnd); cch = CchEdit(hwnd);
    if (hwnd->fEditV) {
        si.nMax = (int)IEditLine(hwnd, cch); si.nPage = (UINT)(((1) > ((rc.bottom - rc.top) / cy)) ? (1) : ((rc.bottom - rc.top) / cy));
        si.nPos = hwnd->yEditScroll / cy;
        hwnd->yEditScroll = win32::SetScrollInfo(hwnd, win32::SB_VERT, &si, 1) * cy;
        if (!win32::IsWindow(hwnd)) return;
    }
    if (hwnd->fEditH) {
        EditFormat(hwnd, &rc); hdc = win32::GetDC(hwnd);
        if (hdc) {
            if (hwnd->hfontControl) win32::SelectObject(hdc, hwnd->hfontControl);
            for (ich = 0; ich <= cch; ich = next) {
                end = IchLineEnd(hwnd, ich);
                if (win32::GetTextExtentPoint32A(hdc, hwnd->pszTitle + ich, (int)(end - ich), &size)) cxMax = StarsMax(cxMax, size.cx);
                next = IchNextLine(hwnd, ich); if (next <= ich || end == cch) break;
            }
            win32::ReleaseDC(hwnd, hdc);
        }
        si.nMax = StarsMax(0, cxMax - 1); si.nPage = (UINT)StarsMax(1, rc.right - rc.left); si.nPos = hwnd->xEditScroll;
        hwnd->xEditScroll = win32::SetScrollInfo(hwnd, win32::SB_HORZ, &si, 1);
        if (!win32::IsWindow(hwnd)) return;
    }
}

static void ScrollEdit(HWND hwnd, int dx, int dy) {
    RECT rc;
    int cy = CyEdit(hwnd), cLine = (int)IEditLine(hwnd, CchEdit(hwnd)) + 1;
    EditFormat(hwnd, &rc);
    hwnd->yEditScroll = (int)((((int64_t)0) > (((((int64_t)(((0) > (cLine - (((1) > ((rc.bottom - rc.top) / cy)) ? (1) : ((rc.bottom - rc.top) / cy)))) ? (0) : (cLine - (((1) > ((rc.bottom - rc.top) / cy)) ? (1) : ((rc.bottom - rc.top) / cy)))) * cy) < ((int64_t)hwnd->yEditScroll + (int64_t)dy * cy)) ? ((int64_t)(((0) > (cLine - (((1) > ((rc.bottom - rc.top) / cy)) ? (1) : ((rc.bottom - rc.top) / cy)))) ? (0) : (cLine - (((1) > ((rc.bottom - rc.top) / cy)) ? (1) : ((rc.bottom - rc.top) / cy)))) * cy) : ((int64_t)hwnd->yEditScroll + (int64_t)dy * cy)))) ? ((int64_t)0) : (((((int64_t)(((0) > (cLine - (((1) > ((rc.bottom - rc.top) / cy)) ? (1) : ((rc.bottom - rc.top) / cy)))) ? (0) : (cLine - (((1) > ((rc.bottom - rc.top) / cy)) ? (1) : ((rc.bottom - rc.top) / cy)))) * cy) < ((int64_t)hwnd->yEditScroll + (int64_t)dy * cy)) ? ((int64_t)(((0) > (cLine - (((1) > ((rc.bottom - rc.top) / cy)) ? (1) : ((rc.bottom - rc.top) / cy)))) ? (0) : (cLine - (((1) > ((rc.bottom - rc.top) / cy)) ? (1) : ((rc.bottom - rc.top) / cy)))) * cy) : ((int64_t)hwnd->yEditScroll + (int64_t)dy * cy))));
    hwnd->xEditScroll = (int)((((int64_t)INT_MAX) < (((((int64_t)0) > ((int64_t)hwnd->xEditScroll + (int64_t)dx * 8)) ? ((int64_t)0) : ((int64_t)hwnd->xEditScroll + (int64_t)dx * 8)))) ? ((int64_t)INT_MAX) : (((((int64_t)0) > ((int64_t)hwnd->xEditScroll + (int64_t)dx * 8)) ? ((int64_t)0) : ((int64_t)hwnd->xEditScroll + (int64_t)dx * 8))));
    hwnd->fEditManualScroll = 1;
    SyncEditScroll(hwnd);
    if (!win32::IsWindow(hwnd)) return;
    win32::InvalidateRect(hwnd, nullptr, 1);
}

static int ChEditClass(BYTE ch) {
    if (SDL_isspace(ch)) return 0;
    return SDL_isalnum(ch) || ch >= 128 || ch == '_' ? 1 : 2;
}

static UINT IchEditWord(HWND hwnd, UINT ich, BOOL fRight) {
    UINT cch = CchEdit(hwnd);
    int kind;
    if (fRight) {
        kind = ChEditClass((BYTE)hwnd->pszTitle[ich]);
        while (ich < cch && ChEditClass((BYTE)hwnd->pszTitle[ich]) == kind) ich = IchEditNext(hwnd, ich);
        while (ich < cch && SDL_isspace((BYTE)hwnd->pszTitle[ich])) ich = IchEditNext(hwnd, ich);
    } else {
        while (ich != 0 && SDL_isspace((BYTE)hwnd->pszTitle[ich - 1])) ich = IchEditPrev(hwnd, ich);
        kind = ich != 0 ? ChEditClass((BYTE)hwnd->pszTitle[ich - 1]) : 0;
        while (ich != 0 && ChEditClass((BYTE)hwnd->pszTitle[ich - 1]) == kind) ich = IchEditPrev(hwnd, ich);
    }
    return ich;
}

static int CxEdit(HDC hdc, LPCSTR psz, UINT cch, BOOL fPassword) {
    SIZE size;
    UINT i;
    int cx = 0;
    if (!fPassword) return win32::GetTextExtentPoint32A(hdc, psz, (int)cch, &size) ? size.cx : 0;
    if (!win32::GetTextExtentPoint32A(hdc, "*", 1, &size)) return 0;
    for (i = 0; i < cch; i++) cx += size.cx;
    return cx;
}

static void PaintEdit(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = win32::BeginPaint(hwnd, &ps);
    HBRUSH hbr, hbrOwn = nullptr;
    HFONT hfontOld = nullptr;
    TEXTMETRICA tm;
    RECT rc, rcSel, format;
    UINT cch, ich, ichEnd, ichNext, ichFirst, ichLast, i;
    int x, y = 2, cy, cx, cxCaret, cxShift = 0, yCaret = 2, cLine = 0;
    BOOL fPassword = (hwnd->dwStyle & win32::ES_PASSWORD) != 0, fFocus = win32::GetFocus() == hwnd;
    COLORREF crText, crBk;
    char *pszDisplay;
    if (!hdc) return;
    win32::GetClientRect(hwnd, &rc);
    win32::SetTextColor(hdc, win32::GetSysColor(win32::IsWindowEnabled(hwnd) ? win32::COLOR_WINDOWTEXT : win32::COLOR_GRAYTEXT));
    win32::SetBkColor(hdc, win32::GetSysColor(win32::COLOR_WINDOW)); win32::SetBkMode(hdc, win32::TRANSPARENT);
    if (hwnd->hfontControl) hfontOld = (HFONT)win32::SelectObject(hdc, hwnd->hfontControl);
    hbr = (HBRUSH)win32::SendMessageA(hwnd->hwndParent,
          (hwnd->dwStyle & win32::ES_READONLY) || !win32::IsWindowEnabled(hwnd) ? win32::WM_CTLCOLORSTATIC : win32::WM_CTLCOLOREDIT,
          (WPARAM)hdc, (LPARAM)hwnd);
    if (!win32::IsWindow(hwnd)) return;
    if (!hbr) hbr = hbrOwn = win32::CreateSolidBrush(win32::GetBkColor(hdc));
    win32::FillRect(hdc, &rc, hbr);
    if ((hwnd->dwStyle & win32::WS_BORDER) || (hwnd->dwExStyle & win32::WS_EX_CLIENTEDGE)) CompatDrawClassicEdge(hdc, &rc, 1, 1);
    EditFormat(hwnd, &format);
    win32::IntersectClipRect(hdc, format.left, format.top, format.right, format.bottom);
    y = yCaret = format.top;
    win32::GetTextMetricsA(hdc, &tm); cy = StarsMax(1, tm.tmHeight);
    cch = CchEdit(hwnd); pszDisplay = SDL_strdup(hwnd->pszTitle);
    if (!pszDisplay) goto done;
    if (fPassword) for (i = 0; i < cch; i++) if (pszDisplay[i] != '\r' && pszDisplay[i] != '\n') pszDisplay[i] = '*';
    ichFirst = StarsMin(hwnd->ichAnchor, hwnd->ichCaret); ichLast = StarsMax(hwnd->ichAnchor, hwnd->ichCaret);
    crText = win32::SetTextColor(hdc, win32::GetSysColor(win32::COLOR_WINDOWTEXT));
    win32::SetTextColor(hdc, crText); crBk = win32::GetBkColor(hdc);
    cxCaret = CxEdit(hdc, pszDisplay + IchLineStart(hwnd, hwnd->ichCaret), hwnd->ichCaret - IchLineStart(hwnd, hwnd->ichCaret), 0);
    if (!(hwnd->dwStyle & win32::ES_MULTILINE) && (hwnd->dwStyle & win32::ES_AUTOHSCROLL)) cxShift = (((0) > (cxCaret - StarsMax(0, rc.right - 6))) ? (0) : (cxCaret - StarsMax(0, rc.right - 6)));
    if (hwnd->dwStyle & win32::ES_MULTILINE) {
        cLine = (int)IEditLine(hwnd, hwnd->ichCaret);
        if ((hwnd->dwStyle & win32::ES_AUTOVSCROLL) && !hwnd->fEditManualScroll) {
            if (cLine * cy < hwnd->yEditScroll) hwnd->yEditScroll = cLine * cy;
            else if (cLine * cy + cy > hwnd->yEditScroll + StarsMax(0, format.bottom - format.top))
                hwnd->yEditScroll = (((0) > (cLine * cy + cy - StarsMax(0, format.bottom - format.top))) ? (0) : (cLine * cy + cy - StarsMax(0, format.bottom - format.top)));
        }
        y -= hwnd->yEditScroll;
    }
    if ((hwnd->dwStyle & win32::ES_MULTILINE) && (hwnd->dwStyle & (win32::ES_AUTOHSCROLL | win32::WS_HSCROLL))) cxShift = hwnd->xEditScroll;
    hwnd->xEditScroll = cxShift;
    SyncEditScroll(hwnd);
    if (!win32::IsWindow(hwnd)) { SDL_free(pszDisplay); return; }
    for (ich = 0; ich <= cch; ich = ichNext) {
        ichEnd = hwnd->dwStyle & win32::ES_MULTILINE ? IchLineEnd(hwnd, ich) : cch;
        ichNext = ichEnd;
        if (ichNext < cch && hwnd->pszTitle[ichNext] == '\r') ichNext++;
        if (ichNext < cch && hwnd->pszTitle[ichNext] == '\n') ichNext++;
        cx = CxEdit(hdc, pszDisplay + ich, ichEnd - ich, 0);
        x = format.left - cxShift;
        if (hwnd->dwStyle & win32::ES_RIGHT) x += StarsMax(0, format.right - format.left - cx);
        else if (hwnd->dwStyle & win32::ES_CENTER) x += (((0) > ((format.right - format.left - cx) / 2)) ? (0) : ((format.right - format.left - cx) / 2));
        win32::TextOutA(hdc, x, y, pszDisplay + ich, (int)(ichEnd - ich));
        if ((fFocus || (hwnd->dwStyle & win32::ES_NOHIDESEL)) && ichFirst < ichEnd && ichLast > ich) {
            UINT ichLo = StarsMax(ichFirst, ich), ichHi = StarsMin(ichLast, ichEnd);
            win32::SetRect(&rcSel, x + CxEdit(hdc, pszDisplay + ich, ichLo - ich, 0), y,
                    x + CxEdit(hdc, pszDisplay + ich, ichHi - ich, 0), y + cy);
            hbr = win32::CreateSolidBrush(win32::GetSysColor(win32::COLOR_HIGHLIGHT)); win32::FillRect(hdc, &rcSel, hbr); win32::DeleteObject(hbr);
            win32::SetTextColor(hdc, win32::GetSysColor(win32::COLOR_HIGHLIGHTTEXT)); win32::SetBkMode(hdc, win32::TRANSPARENT);
            win32::TextOutA(hdc, rcSel.left, y, pszDisplay + ichLo, (int)(ichHi - ichLo)); win32::SetTextColor(hdc, crText); win32::SetBkColor(hdc, crBk);
        }
        if (hwnd->ichCaret >= ich && hwnd->ichCaret <= ichEnd) {
            cxCaret = x + CxEdit(hdc, pszDisplay + ich, hwnd->ichCaret - ich, 0); yCaret = y;
        }
        if (ichEnd == cch || ichNext == ich) break;
        y += cy;
    }
    if (fFocus && hwnd->pszEditComposition && *hwnd->pszEditComposition) {
        UINT cchComp = (UINT)strlen(hwnd->pszEditComposition);
        char *pszComp = SDL_strdup(hwnd->pszEditComposition);
        if (pszComp) {
            if (fPassword) memset(pszComp, '*', cchComp);
            cx = CxEdit(hdc, pszComp, cchComp, 0);
            win32::SetRect(&rcSel, cxCaret, yCaret, cxCaret + cx, yCaret + cy);
            hbr = win32::CreateSolidBrush(crBk); win32::FillRect(hdc, &rcSel, hbr); win32::DeleteObject(hbr);
            win32::TextOutA(hdc, cxCaret, yCaret, pszComp, (int)cchComp);
            win32::MoveToEx(hdc, cxCaret, yCaret + cy - 1, nullptr); win32::LineTo(hdc, cxCaret + cx, yCaret + cy - 1);
            SDL_free(pszComp);
        }
    } else if (fFocus && hwnd->fEditCaret && ichFirst == ichLast) {
        HPEN hpenOld = (HPEN)win32::SelectObject(hdc, win32::GetStockObject(win32::BLACK_PEN));
        win32::MoveToEx(hdc, cxCaret, yCaret, nullptr); win32::LineTo(hdc, cxCaret, yCaret + cy);
        win32::SelectObject(hdc, hpenOld);
    }
    SDL_free(pszDisplay);
done:
    if (hfontOld) win32::SelectObject(hdc, hfontOld);
    if (hbrOwn) win32::DeleteObject(hbrOwn);
    win32::EndPaint(hwnd, &ps);
}

static UINT IchEditPoint(HWND hwnd, LPARAM l) {
    HDC hdc = win32::GetDC(hwnd);
    HFONT hfontOld = nullptr;
    TEXTMETRICA tm;
    UINT ich = 0, ichEnd, cch = CchEdit(hwnd);
    int x = (int16_t)LOWORD(l) + hwnd->xEditScroll;
    int y = (int16_t)HIWORD(l) + hwnd->yEditScroll, cx, cxPrev = 0;
    RECT rc;
    if (!hdc) return hwnd->ichCaret;
    if (hwnd->hfontControl) hfontOld = (HFONT)win32::SelectObject(hdc, hwnd->hfontControl);
    win32::GetTextMetricsA(hdc, &tm);
    EditFormat(hwnd, &rc); x -= rc.left; y -= rc.top;
    if (hwnd->dwStyle & win32::ES_MULTILINE) while (y >= StarsMax(1, tm.tmHeight) && ich < cch) {
        ich = IchLineEnd(hwnd, ich);
        if (ich < cch && hwnd->pszTitle[ich] == '\r') ich++;
        if (ich < cch && hwnd->pszTitle[ich] == '\n') ich++;
        y -= StarsMax(1, tm.tmHeight);
    }
    ichEnd = hwnd->dwStyle & win32::ES_MULTILINE ? IchLineEnd(hwnd, ich) : cch;
    EditFormat(hwnd, &rc);
    cx = CxEdit(hdc, hwnd->pszTitle + ich, ichEnd - ich, (hwnd->dwStyle & win32::ES_PASSWORD) != 0);
    if (hwnd->dwStyle & win32::ES_RIGHT) x -= StarsMax(0, rc.right - rc.left - cx);
    else if (hwnd->dwStyle & win32::ES_CENTER) x -= (((0) > ((rc.right - rc.left - cx) / 2)) ? (0) : ((rc.right - rc.left - cx) / 2));
    for (; ich < ichEnd; ich = IchEditNext(hwnd, ich)) {
        cx = CxEdit(hdc, hwnd->pszTitle + ich, IchEditNext(hwnd, ich) - ich, (hwnd->dwStyle & win32::ES_PASSWORD) != 0);
        if (x < cxPrev + cx / 2) break;
        cxPrev += cx;
    }
    if (hfontOld) win32::SelectObject(hdc, hfontOld);
    win32::ReleaseDC(hwnd, hdc);
    return ich;
}

static void EditContextMenu(HWND hwnd, LPARAM l) {
    HMENU hmenu;
    POINT pt = {(int16_t)LOWORD(l), (int16_t)HIWORD(l)};
    BOOL fSelection = hwnd->ichAnchor != hwnd->ichCaret;
    BOOL fWrite = !(hwnd->dwStyle & win32::ES_READONLY), fCopy = fSelection && !(hwnd->dwStyle & win32::ES_PASSWORD);
    UINT id;
    hmenu = win32::CreatePopupMenu();
    if (!hmenu) return;
    if (l == -1) { pt = POINT{2, 2}; win32::ClientToScreen(hwnd, &pt); }
    win32::AppendMenuA(hmenu, win32::MF_STRING | (hwnd->pszEditUndo && fWrite ? 0 : win32::MF_GRAYED), win32::WM_UNDO, "&Undo");
    win32::AppendMenuA(hmenu, win32::MF_SEPARATOR, 0, nullptr);
    win32::AppendMenuA(hmenu, win32::MF_STRING | (fCopy && fWrite ? 0 : win32::MF_GRAYED), win32::WM_CUT, "Cu&t");
    win32::AppendMenuA(hmenu, win32::MF_STRING | (fCopy ? 0 : win32::MF_GRAYED), win32::WM_COPY, "&Copy");
    win32::AppendMenuA(hmenu, win32::MF_STRING | (fWrite && SDL_HasClipboardText() ? 0 : win32::MF_GRAYED), win32::WM_PASTE, "&Paste");
    win32::AppendMenuA(hmenu, win32::MF_STRING | (fSelection && fWrite ? 0 : win32::MF_GRAYED), win32::WM_CLEAR, "&Delete");
    win32::AppendMenuA(hmenu, win32::MF_SEPARATOR, 0, nullptr);
    win32::AppendMenuA(hmenu, win32::MF_STRING | (CchEdit(hwnd) != 0 ? 0 : win32::MF_GRAYED), 0xff00, "Select &All");
    id = (UINT)CompatChoiceIO().pfnTrackPopupMenu(hmenu, win32::TPM_RETURNCMD | win32::TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
    win32::DestroyMenu(hmenu);
    if (!win32::IsWindow(hwnd)) return;
    if (id == 0xff00) win32::SendMessageA(hwnd, win32::EM_SETSEL, 0, -1);
    else if (id != 0) win32::SendMessageA(hwnd, id, 0, 0);
}

LRESULT  CompatEditProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND lp = LpCompatWindow(hwnd);
    UINT cch, ich, ichStart, ichEnd, iLine, cchColumn, cLine, iUndoAction;
    BOOL fShift, fCtrl, fWord, fShortcut;
    LRESULT result;
    WORD cchMax;
    RECT rc;
    char sz[8];
    char *pch;
    if (!lp) return 0;
    cch = CchEdit(hwnd);
    switch (msg) {
    case win32::WM_SETCURSOR:
        if (LOWORD(l) != win32::HTCLIENT) return win32::DefWindowProcA(hwnd, msg, w, l);
        win32::SetCursor(win32::LoadCursorA(nullptr, win32::MAKEINTRESOURCE(32513)));
        return 1;
    case win32::WM_NCCREATE:
        lp->iControl = 3; lp->cchLimit = 30000;
        NormalizeEditCase(hwnd);
        lp->fEditV = (lp->dwStyle & win32::WS_VSCROLL) != 0; lp->fEditH = (lp->dwStyle & win32::WS_HSCROLL) != 0;
        return 1;
    case win32::WM_GETDLGCODE: return win32::DLGC_WANTARROWS | win32::DLGC_WANTCHARS | win32::DLGC_HASSETSEL;
    case win32::EM_GETSEL:
        ichStart = StarsMin(lp->ichAnchor, lp->ichCaret); ichEnd = StarsMax(lp->ichAnchor, lp->ichCaret);
        if (w) *(DWORD *)w = ichStart;
        if (l) *(DWORD *)l = ichEnd;
        return ichStart > UINT16_MAX || ichEnd > UINT16_MAX ? -1 : (LRESULT)MAKELONG(ichStart, ichEnd);
    case win32::EM_SETSEL:
        if ((INT_PTR)w == -1) SelectEdit(hwnd, lp->ichCaret, lp->ichCaret);
        else SelectEdit(hwnd, (UINT)w, l == -1 ? cch : (UINT)l);
        return 0;
    case win32::EM_LIMITTEXT: lp->cchLimit = w ? ((((UINT_PTR)INT_MAX) < (w)) ? ((UINT_PTR)INT_MAX) : (w)) : INT_MAX; return 0;
    case win32::EM_GETLIMITTEXT: return lp->cchLimit;
    case win32::EM_GETMODIFY: return lp->fModified;
    case win32::EM_SETMODIFY: lp->fModified = w != 0; return 0;
    case win32::EM_SETREADONLY:
        if (w) lp->dwStyle |= win32::ES_READONLY; else lp->dwStyle &= ~win32::ES_READONLY;
        win32::InvalidateRect(hwnd, nullptr, 1); return 1;
    case win32::EM_REPLACESEL: if (l) ReplaceEdit(hwnd, (LPCSTR)l, 0, w != 0, w ? editUndoReplace : editUndoSeparate); return 0;
    case win32::EM_CANUNDO: return lp->pszEditUndo != nullptr;
    case win32::EM_UNDO: case win32::WM_UNDO: return UndoEdit(hwnd);
    case win32::EM_EMPTYUNDOBUFFER: EmptyEditUndo(hwnd); return 0;
    case win32::WM_COPY: CopyEdit(hwnd); return 0;
    case win32::WM_CUT:
        if (!(lp->dwStyle & win32::ES_READONLY) && CopyEdit(hwnd)) ReplaceEdit(hwnd, "", 1, 1, editUndoSeparate);
        return 0;
    case win32::WM_CLEAR:
        if (lp->ichAnchor != lp->ichCaret) ReplaceEdit(hwnd, "", 1, 1, editUndoSeparate);
        return 0;
    case win32::WM_PASTE: PasteEdit(hwnd); return 0;
    case win32::EM_GETRECT: if (l) EditFormat(hwnd, (RECT *)l); return 0;
    case win32::EM_SETRECT: case win32::EM_SETRECTNP:
        if (!(lp->dwStyle & win32::ES_MULTILINE)) return 0;
        lp->fEditFormat = l != 0;
        if (l) lp->rcEditFormat = *(RECT *)l;
        if (msg == win32::EM_SETRECT) win32::InvalidateRect(hwnd, nullptr, 1);
        return 0;
    case win32::EM_GETFIRSTVISIBLELINE: return lp->dwStyle & win32::ES_MULTILINE ? lp->yEditScroll / CyEdit(hwnd) : 0;
    case win32::EM_LINESCROLL:
        if (!(lp->dwStyle & win32::ES_MULTILINE)) return 0;
        ScrollEdit(hwnd, (int)w, (int)l); return 1;
    case win32::EM_SCROLLCARET: lp->fEditManualScroll = 0; win32::InvalidateRect(hwnd, nullptr, 1); return 0;
    case win32::WM_HSCROLL:
        if (LOWORD(w) == win32::SB_LINELEFT) ScrollEdit(hwnd, -1, 0);
        else if (LOWORD(w) == win32::SB_LINERIGHT) ScrollEdit(hwnd, 1, 0);
        else if (LOWORD(w) == win32::SB_PAGELEFT || LOWORD(w) == win32::SB_PAGERIGHT) {
            EditFormat(hwnd, &rc); ScrollEdit(hwnd, (LOWORD(w) == win32::SB_PAGELEFT ? -1 : 1) * (((1) > ((rc.right - rc.left) / 8)) ? (1) : ((rc.right - rc.left) / 8)), 0);
        } else if (LOWORD(w) == win32::SB_THUMBTRACK || LOWORD(w) == win32::SB_THUMBPOSITION) {
            lp->xEditScroll = win32::IsWindow(lp->rghwndScroll[win32::SB_HORZ]) && lp->rghwndScroll[win32::SB_HORZ]->fTracking ?
                lp->rghwndScroll[win32::SB_HORZ]->iScrollTrack : HIWORD(w);
            ScrollEdit(hwnd, 0, 0);
        }
        return 0;
    case win32::EM_SCROLL: case win32::WM_VSCROLL:
        EditFormat(hwnd, &rc); cLine = (UINT)(((1) > ((rc.bottom - rc.top) / CyEdit(hwnd))) ? (1) : ((rc.bottom - rc.top) / CyEdit(hwnd)));
        ichStart = (UINT)(lp->yEditScroll / CyEdit(hwnd));
        if (LOWORD(w) == win32::SB_LINEUP) ScrollEdit(hwnd, 0, -1);
        else if (LOWORD(w) == win32::SB_LINEDOWN) ScrollEdit(hwnd, 0, 1);
        else if (LOWORD(w) == win32::SB_PAGEUP) ScrollEdit(hwnd, 0, -(int)cLine);
        else if (LOWORD(w) == win32::SB_PAGEDOWN) ScrollEdit(hwnd, 0, (int)cLine);
        else if (LOWORD(w) == win32::SB_TOP) ScrollEdit(hwnd, 0, -(int)ichStart);
        else if (LOWORD(w) == win32::SB_BOTTOM) ScrollEdit(hwnd, 0, INT_MAX);
        else if (LOWORD(w) == win32::SB_THUMBTRACK || LOWORD(w) == win32::SB_THUMBPOSITION) {
            int iPos = win32::IsWindow(lp->rghwndScroll[win32::SB_VERT]) && lp->rghwndScroll[win32::SB_VERT]->fTracking ?
                lp->rghwndScroll[win32::SB_VERT]->iScrollTrack : HIWORD(w);
            ScrollEdit(hwnd, 0, iPos - (int)ichStart);
        }
        return MAKELONG(lp->yEditScroll / CyEdit(hwnd) - (int)ichStart, 1);
    case win32::EM_GETLINECOUNT:
        return lp->dwStyle & win32::ES_MULTILINE ? IEditLine(hwnd, cch) + 1 : 1;
    case win32::EM_LINEFROMCHAR:
        ichEnd = (INT_PTR)w == -1 ? lp->ichCaret : ((((UINT)w) < (cch)) ? ((UINT)w) : (cch));
        return lp->dwStyle & win32::ES_MULTILINE ? IEditLine(hwnd, ichEnd) : 0;
    case win32::EM_LINEINDEX:
        if ((INT_PTR)w == -1) return IchLineStart(hwnd, lp->ichCaret);
        ich = IchEditLine(hwnd, (UINT)w); return ich == UINT_MAX ? -1 : (LRESULT)ich;
    case win32::EM_LINELENGTH:
        ich = (INT_PTR)w == -1 ? lp->ichCaret : ((((UINT)w) < (cch)) ? ((UINT)w) : (cch));
        ichStart = IchLineStart(hwnd, ich);
        return IchLineEnd(hwnd, ichStart) - ichStart;
    case win32::EM_GETLINE:
        ich = IchEditLine(hwnd, (UINT)w);
        if (!l || ich == UINT_MAX) return 0;
        memcpy(&cchMax, (void *)l, sizeof(cchMax));
        cLine = (UINT)CchUtf8Prefix(lp->pszTitle + ich, (((cchMax) < (IchLineEnd(hwnd, ich) - ich)) ? (cchMax) : (IchLineEnd(hwnd, ich) - ich)));
        memcpy((void *)l, lp->pszTitle + ich, cLine); return cLine;
    case win32::WM_SETTEXT:
        result = win32::DefWindowProcA(hwnd, msg, w, l ? l : (LPARAM)"");
        if (result) {
            lp->fEditInputRejected = 0;
            NormalizeEditCase(hwnd);
            EmptyEditUndo(hwnd);
            lp->ichAnchor = lp->ichCaret = 0; lp->fModified = 0;
            lp->xEditScroll = lp->yEditScroll = 0;
            ChangedEdit(hwnd, !(lp->dwStyle & win32::ES_MULTILINE));
        }
        return result;
    case win32::WM_RBUTTONUP:
        {
            POINT pt = {(int16_t)LOWORD(l), (int16_t)HIWORD(l)};
            win32::ClientToScreen(hwnd, &pt); EditContextMenu(hwnd, win32::MAKELPARAM(pt.x, pt.y));
        }
        return 0;
    case win32::WM_CONTEXTMENU: EditContextMenu(hwnd, l); return 0;
    case win32::WM_GETFONT: return (LRESULT)lp->hfontControl;
    case win32::WM_SETFONT: lp->hfontControl = (HFONT)w; SyncEditScroll(hwnd); if (l && win32::IsWindow(hwnd)) win32::InvalidateRect(hwnd, nullptr, 1); return 0;
    case win32::WM_PAINT: PaintEdit(hwnd); return 0;
    case win32::WM_ERASEBKGND: return 1;
    case win32::WM_ENABLE: case win32::WM_SIZE:
        SyncEditScroll(hwnd); if (win32::IsWindow(hwnd)) win32::InvalidateRect(hwnd, nullptr, 1); return 0;
    case win32::WM_SETFOCUS: case win32::WM_KILLFOCUS:
        lp->fEditCaret = msg == win32::WM_SETFOCUS; lp->fEditTyping = 0; lp->iEditUndoAction = editUndoSeparate;
        if (msg == win32::WM_KILLFOCUS) { SDL_free(lp->pszEditComposition); lp->pszEditComposition = nullptr; }
        if (lp->idEditCaretTimer) win32::KillTimer(hwnd, lp->idEditCaretTimer);
        lp->idEditCaretTimer = msg == win32::WM_SETFOCUS ? win32::SetTimer(hwnd, UINTPTR_MAX - 2, 530, nullptr) : 0;
        win32::InvalidateRect(hwnd, nullptr, 1); NotifyEdit(hwnd, msg == win32::WM_SETFOCUS ? win32::EN_SETFOCUS : win32::EN_KILLFOCUS); return 0;
    case win32::WM_TIMER:
        if (w == lp->idEditCaretTimer && win32::GetFocus() == hwnd) { lp->fEditCaret = !lp->fEditCaret; win32::InvalidateRect(hwnd, nullptr, 0); }
        return 0;
    case win32::WM_LBUTTONDBLCLK:
        win32::SetFocus(hwnd); if (!win32::IsWindow(hwnd)) return 0;
        ich = IchEditPoint(hwnd, l); ichStart = ich;
        if (ich < cch) {
            int kind = ChEditClass((BYTE)lp->pszTitle[ich]);
            while (ichStart != 0 && ChEditClass((BYTE)lp->pszTitle[ichStart - 1]) == kind) ichStart = IchEditPrev(hwnd, ichStart);
            while (ich < cch && ChEditClass((BYTE)lp->pszTitle[ich]) == kind) ich++;
        }
        SelectEdit(hwnd, ichStart, ich); return 0;
    case win32::WM_LBUTTONDOWN:
        win32::SetFocus(hwnd); if (!win32::IsWindow(hwnd)) return 0;
        ich = IchEditPoint(hwnd, l);
        SelectEdit(hwnd, w & win32::MK_SHIFT ? lp->ichAnchor : ich, ich);
        win32::SetCapture(hwnd); return 0;
    case win32::WM_MOUSEMOVE:
        if (win32::GetCapture() == hwnd && (w & win32::MK_LBUTTON)) SelectEdit(hwnd, lp->ichAnchor, IchEditPoint(hwnd, l));
        return 0;
    case win32::WM_LBUTTONUP: if (win32::GetCapture() == hwnd) win32::ReleaseCapture(); return 0;
    case win32::WM_KEYDOWN:
        fShift = (win32::GetKeyState(win32::VK_SHIFT) & 0x8000) != 0;
        fCtrl = (win32::GetKeyState(win32::VK_CONTROL) & 0x8000) != 0;
        fWord = fCtrl;
        fShortcut = fCtrl;
#ifdef __APPLE__
        fWord = fWord || (win32::GetKeyState(win32::VK_MENU) & 0x8000);
        fShortcut = fShortcut || ((win32::GetKeyState(win32::VK_LWIN) | win32::GetKeyState(win32::VK_RWIN)) & 0x8000);
        /* Command arrows use the same line boundaries and Shift anchor as Home/End. */
        if ((win32::GetKeyState(win32::VK_LWIN) | win32::GetKeyState(win32::VK_RWIN)) & 0x8000) {
            if (w == win32::VK_LEFT) w = win32::VK_HOME;
            else if (w == win32::VK_RIGHT) w = win32::VK_END;
        }
#endif
        ich = lp->ichCaret;
        if (w == win32::VK_APPS || (w == win32::VK_F10 && fShift)) { EditContextMenu(hwnd, -1); return 0; }
        if (fShortcut && w == 'A') { SelectEdit(hwnd, 0, cch); return 0; }
        if (fShortcut && w == 'C') { CopyEdit(hwnd); return 0; }
        if (fShortcut && w == 'X') return CompatEditProc(hwnd, win32::WM_CUT, 0, 0);
        if (fShortcut && w == 'V') { PasteEdit(hwnd); return 0; }
        if (fShortcut && w == 'Z') { UndoEdit(hwnd); return 0; }
        if (w == win32::VK_LEFT) {
            if (fWord) ich = IchEditWord(hwnd, ich, 0);
            else if (!fShift && lp->ichAnchor != ich) ich = StarsMin(lp->ichAnchor, ich);
            else if (ich != 0) { ich = IchEditPrev(hwnd, ich); if (ich != 0 && lp->pszTitle[ich] == '\n' && lp->pszTitle[ich - 1] == '\r') ich--; }
        } else if (w == win32::VK_RIGHT) {
            if (fWord) ich = IchEditWord(hwnd, ich, 1);
            else if (!fShift && lp->ichAnchor != ich) ich = StarsMax(lp->ichAnchor, ich);
            else if (ich < cch) { if (lp->pszTitle[ich] == '\r' && lp->pszTitle[ich + 1] == '\n') ich++; ich = IchEditNext(hwnd, ich); }
        }
        else if (w == win32::VK_HOME) ich = !fCtrl && (lp->dwStyle & win32::ES_MULTILINE) ? IchLineStart(hwnd, ich) : 0;
        else if (w == win32::VK_END) ich = !fCtrl && (lp->dwStyle & win32::ES_MULTILINE) ? IchLineEnd(hwnd, ich) : cch;
        else if ((w == win32::VK_UP || w == win32::VK_DOWN || w == win32::VK_PRIOR || w == win32::VK_NEXT) && (lp->dwStyle & win32::ES_MULTILINE)) {
            ichStart = IchLineStart(hwnd, ich); cchColumn = ich - ichStart;
            EditFormat(hwnd, &rc);
            iLine = IEditLine(hwnd, ich);
            cLine = w == win32::VK_PRIOR || w == win32::VK_NEXT ? (UINT)(((1) > ((rc.bottom - rc.top) / CyEdit(hwnd))) ? (1) : ((rc.bottom - rc.top) / CyEdit(hwnd))) : 1;
            iLine = w == win32::VK_UP || w == win32::VK_PRIOR ? (iLine > cLine ? iLine - cLine : 0) :
                    (((IEditLine(hwnd, cch)) < (iLine + cLine)) ? (IEditLine(hwnd, cch)) : (iLine + cLine));
            ichStart = IchEditLine(hwnd, iLine);
            ich = (((ichStart + cchColumn) < (IchLineEnd(hwnd, ichStart))) ? (ichStart + cchColumn) : (IchLineEnd(hwnd, ichStart)));
        }
        else if (w == win32::VK_DELETE) {
            if (lp->dwStyle & win32::ES_READONLY) return 0;
            iUndoAction = lp->ichAnchor == lp->ichCaret ? editUndoDelete : editUndoSeparate;
            if (lp->ichAnchor == ich && ich < cch) {
                lp->ichCaret = IchEditNext(hwnd, ich);
                if (lp->pszTitle[ich] == '\r' && lp->pszTitle[ich + 1] == '\n') lp->ichCaret++;
            }
            if (lp->ichAnchor != lp->ichCaret) ReplaceEdit(hwnd, "", 1, 1, iUndoAction);
            return 0;
        } else return 0;
        SelectEdit(hwnd, fShift ? lp->ichAnchor : ich, ich); return 0;
    case win32::WM_CHAR:
        if (lp->dwStyle & win32::ES_READONLY) return 0;
        if (w == '\b') {
            iUndoAction = lp->ichAnchor == lp->ichCaret ? editUndoBackspace : editUndoSeparate;
            if (lp->ichAnchor == lp->ichCaret && lp->ichCaret != 0) {
                lp->ichAnchor = IchEditPrev(hwnd, lp->ichAnchor);
                if (lp->ichAnchor != 0 && lp->pszTitle[lp->ichAnchor] == '\n' && lp->pszTitle[lp->ichAnchor - 1] == '\r') lp->ichAnchor--;
            }
            if (lp->ichAnchor != lp->ichCaret) ReplaceEdit(hwnd, "", 1, 1, iUndoAction);
        } else if (w == '\r' && (lp->dwStyle & win32::ES_MULTILINE)) ReplaceEdit(hwnd, "\r\n", 1, 1, editUndoSeparate);
        else if (w >= 32 && w <= 0x10ffff && !(w >= 0xd800 && w <= 0xdfff) &&
                 (!(lp->dwStyle & win32::ES_NUMBER) || (w >= '0' && w <= '9'))) {
            pch = SDL_UCS4ToUTF8((Uint32)w, sz); *pch = 0; ReplaceEdit(hwnd, sz, 1, 1, editUndoTyping);
        }
        return 0;
    default: return win32::DefWindowProcA(hwnd, msg, w, l);
    }
}

BOOL StarsCompatTextInputRejected(HWND hwnd) {
    HWND lp = LpCompatWindow(hwnd);
    return lp && lp->fEditInputRejected;
}
