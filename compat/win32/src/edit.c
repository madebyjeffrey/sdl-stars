/* Narrow edit controls store CP1252 byte offsets, as the Stars UI does. */
#include "internal.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum { editUndoSeparate, editUndoTyping, editUndoBackspace, editUndoDelete, editUndoReplace };

static void NotifyEdit(HWND hwnd, UINT code) {
    HWND lp = LpCompatWindow(hwnd);
    if (lp && lp->hwndParent) SendMessageA(lp->hwndParent, WM_COMMAND, MAKEWPARAM(lp->idChild, code), (LPARAM)hwnd);
}

static void SyncEditScroll(HWND hwnd);

void CompatEditComposition(HWND hwnd, const char *pszUtf, int ich, int cch) {
    char *psz, *pch;
    const char *pchUtf;
    DWORD ch;
    int i, c;
    if (!IsWindow(hwnd) || hwnd->iControl != 3 || (hwnd->dwStyle & ES_READONLY)) return;
    if (!pszUtf) pszUtf = "";
    psz = SDL_malloc(strlen(pszUtf) + 1);
    if (!psz) return;
    for (pch = psz, pchUtf = pszUtf; *pchUtf;) {
        ch = SDL_StepUTF8(&pchUtf, NULL);
        for (i = 1; i < 256 && DwCompatAnsiChar((BYTE)i) != ch; i++) {}
        *pch++ = i < 256 ? (char)i : '?';
    }
    *pch = 0; c = (int)(pch - psz);
    SDL_free(hwnd->pszEditComposition); hwnd->pszEditComposition = psz;
    hwnd->ichComposition = max(0, min(ich, c));
    hwnd->cchComposition = max(0, min(cch, c - hwnd->ichComposition));
    InvalidateRect(hwnd, NULL, TRUE);
}

static UINT CchEdit(HWND hwnd) { return (UINT)strlen(hwnd->pszTitle); }

static void SelectEdit(HWND hwnd, UINT ichAnchor, UINT ichCaret) {
    UINT cch = CchEdit(hwnd);
    hwnd->ichAnchor = min(ichAnchor, cch);
    hwnd->ichCaret = min(ichCaret, cch);
    hwnd->fEditTyping = FALSE;
    hwnd->iEditUndoAction = editUndoSeparate;
    hwnd->fEditManualScroll = FALSE;
    InvalidateRect(hwnd, NULL, TRUE);
}

static void ChangedEdit(HWND hwnd, BOOL fNotify) {
    SyncEditScroll(hwnd);
    if (!IsWindow(hwnd)) return;
    InvalidateRect(hwnd, NULL, TRUE);
    if (fNotify) {
        NotifyEdit(hwnd, EN_UPDATE);
        if (IsWindow(hwnd)) NotifyEdit(hwnd, EN_CHANGE);
    }
}

static void EmptyEditUndo(HWND hwnd) {
    SDL_free(hwnd->pszEditUndo);
    hwnd->pszEditUndo = NULL;
    hwnd->fEditTyping = FALSE; hwnd->iEditUndoAction = editUndoSeparate;
}

static BYTE ChEditCase(BYTE ch, BOOL fUpper) {
    static const BYTE rgLower[] = {0x9a, 0x9c, 0x9e, 0xff};
    static const BYTE rgUpper[] = {0x8a, 0x8c, 0x8e, 0x9f};
    size_t i;
    if (fUpper && ((ch >= 'a' && ch <= 'z') || (ch >= 0xe0 && ch <= 0xfe && ch != 0xf7))) return ch - 32;
    if (!fUpper && ((ch >= 'A' && ch <= 'Z') || (ch >= 0xc0 && ch <= 0xde && ch != 0xd7))) return ch + 32;
    for (i = 0; i < SDL_arraysize(rgLower); i++)
        if (ch == (fUpper ? rgLower[i] : rgUpper[i])) return fUpper ? rgUpper[i] : rgLower[i];
    return ch;
}

static void NormalizeEditCase(HWND hwnd) {
    char *pch;
    if (!(hwnd->dwStyle & (ES_UPPERCASE | ES_LOWERCASE))) return;
    for (pch = hwnd->pszTitle; *pch; pch++)
        *pch = (char)ChEditCase((BYTE)*pch, (hwnd->dwStyle & ES_UPPERCASE) != 0);
}

static void ReplaceEdit(HWND hwnd, LPCSTR psz, BOOL fUser, BOOL fUndo, UINT iUndoAction) {
    UINT ichFirst = min(hwnd->ichAnchor, hwnd->ichCaret), ichLast = max(hwnd->ichAnchor, hwnd->ichCaret);
    size_t cchOld = CchEdit(hwnd), cch = strlen(psz), cchBase = cchOld - (ichLast - ichFirst), i;
    char *pszNew, *pszUndo = NULL;
    BOOL fLimit, fGroup;
    fGroup = iUndoAction != editUndoSeparate && hwnd->pszEditUndo &&
             iUndoAction == hwnd->iEditUndoAction &&
             (iUndoAction == editUndoBackspace ? ichLast == hwnd->ichTypingEnd : ichFirst == hwnd->ichTypingEnd);
    /* Adjacent removals share an undo range even when Delete and Backspace
       alternate. Cursor/selection changes still terminate the group. */
    if (hwnd->pszEditUndo && hwnd->cchUndoAfter == 0 &&
        (iUndoAction == editUndoBackspace || iUndoAction == editUndoDelete) &&
        (hwnd->iEditUndoAction == editUndoBackspace || hwnd->iEditUndoAction == editUndoDelete) &&
        (ichFirst == hwnd->ichUndoPosition || ichLast == hwnd->ichUndoPosition)) fGroup = TRUE;
    if (hwnd->pszEditUndo && iUndoAction == editUndoTyping && hwnd->iEditUndoAction == editUndoReplace &&
        ichFirst == ichLast && ichFirst == hwnd->ichTypingEnd) fGroup = TRUE;
    if (fUser && (hwnd->dwStyle & ES_READONLY)) return;
    fLimit = cch > (hwnd->cchLimit > cchBase ? hwnd->cchLimit - cchBase : 0);
    if (fLimit) cch = hwnd->cchLimit > cchBase ? hwnd->cchLimit - cchBase : 0;
    if (fLimit && cch == 0 && ichFirst == ichLast) {
        // The SDK still notifies at the limit, but no text entered the undo buffer.
        hwnd->fModified = TRUE; NotifyEdit(hwnd, EN_MAXTEXT);
        if (IsWindow(hwnd)) ChangedEdit(hwnd, TRUE);
        return;
    }
    if (cchBase > INT_MAX || cch > INT_MAX - cchBase) return;
    pszNew = SDL_malloc(cchBase + cch + 1);
    if (!pszNew) { NotifyEdit(hwnd, EN_ERRSPACE); return; }
    if (fUndo && !fGroup && !(pszUndo = SDL_strdup(hwnd->pszTitle))) {
        SDL_free(pszNew); NotifyEdit(hwnd, EN_ERRSPACE); return;
    }
    memcpy(pszNew, hwnd->pszTitle, ichFirst);
    memcpy(pszNew + ichFirst, psz, cch);
    memcpy(pszNew + ichFirst + cch, hwnd->pszTitle + ichLast, cchOld - ichLast + 1);
    for (i = ichFirst; i < ichFirst + cch; i++) {
        if (hwnd->dwStyle & ES_UPPERCASE) pszNew[i] = (char)ChEditCase((BYTE)pszNew[i], TRUE);
        else if (hwnd->dwStyle & ES_LOWERCASE) pszNew[i] = (char)ChEditCase((BYTE)pszNew[i], FALSE);
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
    hwnd->fEditManualScroll = FALSE;
    SDL_free(hwnd->pszTitle); hwnd->pszTitle = pszNew;
    hwnd->ichAnchor = hwnd->ichCaret = ichFirst + (UINT)cch;
    hwnd->fModified = TRUE;
    if (fLimit) NotifyEdit(hwnd, EN_MAXTEXT);
    if (IsWindow(hwnd)) ChangedEdit(hwnd, TRUE);
}

static BOOL UndoEdit(HWND hwnd) {
    char *psz;
    UINT ichFirst, ichLast, cchBefore;
    hwnd->fEditTyping = FALSE;
    hwnd->iEditUndoAction = editUndoSeparate;
    if (hwnd->dwStyle & ES_READONLY) return !(hwnd->dwStyle & ES_MULTILINE);
    if (!hwnd->pszEditUndo) {
        // Clearing history retains its position; an empty undo still notifies.
        SelectEdit(hwnd, hwnd->ichUndoPosition, hwnd->ichUndoPosition);
        ChangedEdit(hwnd, FALSE);
        if (IsWindow(hwnd)) NotifyEdit(hwnd, EN_CHANGE);
        return TRUE;
    }
    psz = hwnd->pszTitle; hwnd->pszTitle = hwnd->pszEditUndo; hwnd->pszEditUndo = psz;
    ichFirst = hwnd->ichUndoPosition; ichLast = ichFirst + hwnd->cchUndoBefore;
    cchBefore = hwnd->cchUndoBefore; hwnd->cchUndoBefore = hwnd->cchUndoAfter; hwnd->cchUndoAfter = cchBefore;
    hwnd->fModified = TRUE;
    SelectEdit(hwnd, ichLast, ichLast);
    ChangedEdit(hwnd, TRUE);
    if (IsWindow(hwnd)) {
        // The final change notification observes the restored range, not just its caret.
        SelectEdit(hwnd, ichFirst, ichLast);
        NotifyEdit(hwnd, EN_CHANGE);
    }
    return TRUE;
}

static BOOL CopyEdit(HWND hwnd) {
    UINT ichFirst = min(hwnd->ichAnchor, hwnd->ichCaret), ichLast = max(hwnd->ichAnchor, hwnd->ichCaret), i;
    char *psz, *pch;
    BOOL fOk;
    size_t cch = ichLast - ichFirst;
    if (ichFirst == ichLast || (hwnd->dwStyle & ES_PASSWORD)) return FALSE;
    if (cch > (SIZE_MAX - 1) / 4) return FALSE;
    psz = SDL_malloc(cch * 4 + 1);
    if (!psz) return FALSE;
    for (i = ichFirst, pch = psz; i < ichLast; i++) pch = SDL_UCS4ToUTF8(DwCompatAnsiChar((BYTE)hwnd->pszTitle[i]), pch);
    *pch = 0;
    fOk = SDL_SetClipboardText(psz);
    SDL_free(psz);
    return fOk;
}

static void PasteEdit(HWND hwnd) {
    char *pszUtf, *psz, *pch;
    const char *pchUtf;
    DWORD ch;
    int i;
    if (hwnd->dwStyle & ES_READONLY) return;
    pszUtf = SDL_GetClipboardText();
    if (!pszUtf) return;
    psz = SDL_malloc(strlen(pszUtf) + 1);
    if (!psz) { SDL_free(pszUtf); return; }
    for (pch = psz, pchUtf = pszUtf; *pchUtf;) {
        ch = SDL_StepUTF8(&pchUtf, NULL);
        if (!(hwnd->dwStyle & ES_MULTILINE) && (ch == '\r' || ch == '\n')) break;
        for (i = 1; i < 256 && DwCompatAnsiChar((BYTE)i) != ch; i++) {}
        *pch++ = i < 256 ? (char)i : '?';
    }
    *pch = 0;
    ReplaceEdit(hwnd, psz, TRUE, TRUE, editUndoSeparate);
    SDL_free(psz); SDL_free(pszUtf);
}

static void EditFormat(HWND hwnd, RECT *lprc) {
    if (hwnd->fEditFormat) *lprc = hwnd->rcEditFormat;
    else { GetClientRect(hwnd, lprc); InflateRect(lprc, -2, -2); }
    lprc->right = max(lprc->left + 1, lprc->right);
    lprc->bottom = max(lprc->top + 1, lprc->bottom);
}

static UINT IchLineEnd(HWND hwnd, UINT ich) {
    UINT cch = CchEdit(hwnd), ichEnd = ich, lo, hi, mid, i;
    HDC hdc;
    RECT rc;
    SIZE size;
    while (ichEnd < cch && hwnd->pszTitle[ichEnd] != '\r' && hwnd->pszTitle[ichEnd] != '\n') ichEnd++;
    if (!(hwnd->dwStyle & ES_MULTILINE) || ((hwnd->dwStyle & ES_AUTOHSCROLL) || hwnd->fEditH) || ichEnd == ich) return ichEnd;
    EditFormat(hwnd, &rc);
    hdc = GetDC(hwnd);
    if (!hdc) return ichEnd;
    if (hwnd->hfontControl) SelectObject(hdc, hwnd->hfontControl);
    lo = ich; hi = ichEnd;
    while (lo < hi) {
        mid = lo + (hi - lo + 1) / 2;
        GetTextExtentPoint32A(hdc, hwnd->pszTitle + ich, (int)(mid - ich), &size);
        if (size.cx <= rc.right - rc.left) lo = mid; else hi = mid - 1;
    }
    ReleaseDC(hwnd, hdc);
    if (lo == ichEnd) return ichEnd;
    lo = max(ich + 1, lo);
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
    ich = min(ich, cch);
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
    HDC hdc = GetDC(hwnd);
    TEXTMETRICA tm = {0};
    if (hdc) {
        if (hwnd->hfontControl) SelectObject(hdc, hwnd->hfontControl);
        GetTextMetricsA(hdc, &tm); ReleaseDC(hwnd, hdc);
    }
    return max(1, tm.tmHeight);
}

static void SyncEditScroll(HWND hwnd) {
    SCROLLINFO si = {sizeof(si), SIF_RANGE | SIF_PAGE | SIF_POS, 0, 0, 0, 0, 0};
    RECT rc;
    HDC hdc;
    SIZE size;
    UINT ich, next, end, cch;
    int cy, cxMax = 0;
    if (!(hwnd->dwStyle & ES_MULTILINE) || hwnd->fEditScrollSync || hwnd->fDestroying) return;
    hwnd->fEditScrollSync = TRUE;
    EditFormat(hwnd, &rc); cy = CyEdit(hwnd); cch = CchEdit(hwnd);
    if (hwnd->fEditV) {
        si.nMax = (int)IEditLine(hwnd, cch); si.nPage = (UINT)max(1, (rc.bottom - rc.top) / cy);
        si.nPos = hwnd->yEditScroll / cy;
        hwnd->yEditScroll = SetScrollInfo(hwnd, SB_VERT, &si, TRUE) * cy;
        if (!IsWindow(hwnd)) return;
    }
    if (hwnd->fEditH) {
        EditFormat(hwnd, &rc); hdc = GetDC(hwnd);
        if (hdc) {
            if (hwnd->hfontControl) SelectObject(hdc, hwnd->hfontControl);
            for (ich = 0; ich <= cch; ich = next) {
                end = IchLineEnd(hwnd, ich);
                if (GetTextExtentPoint32A(hdc, hwnd->pszTitle + ich, (int)(end - ich), &size)) cxMax = max(cxMax, size.cx);
                next = IchNextLine(hwnd, ich); if (next <= ich || end == cch) break;
            }
            ReleaseDC(hwnd, hdc);
        }
        si.nMax = max(0, cxMax - 1); si.nPage = (UINT)max(1, rc.right - rc.left); si.nPos = hwnd->xEditScroll;
        hwnd->xEditScroll = SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
        if (!IsWindow(hwnd)) return;
    }
    hwnd->fEditScrollSync = FALSE;
}

static void ScrollEdit(HWND hwnd, int dx, int dy) {
    RECT rc;
    int cy = CyEdit(hwnd), cLine = (int)IEditLine(hwnd, CchEdit(hwnd)) + 1;
    EditFormat(hwnd, &rc);
    hwnd->yEditScroll = (int)max((int64_t)0, min((int64_t)max(0, cLine - max(1, (rc.bottom - rc.top) / cy)) * cy,
                                (int64_t)hwnd->yEditScroll + (int64_t)dy * cy));
    hwnd->xEditScroll = (int)min((int64_t)INT_MAX, max((int64_t)0, (int64_t)hwnd->xEditScroll + (int64_t)dx * 8));
    hwnd->fEditManualScroll = TRUE;
    SyncEditScroll(hwnd);
    if (!IsWindow(hwnd)) return;
    InvalidateRect(hwnd, NULL, TRUE);
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
        while (ich < cch && ChEditClass((BYTE)hwnd->pszTitle[ich]) == kind) ich++;
        while (ich < cch && SDL_isspace((BYTE)hwnd->pszTitle[ich])) ich++;
    } else {
        while (ich != 0 && SDL_isspace((BYTE)hwnd->pszTitle[ich - 1])) ich--;
        kind = ich != 0 ? ChEditClass((BYTE)hwnd->pszTitle[ich - 1]) : 0;
        while (ich != 0 && ChEditClass((BYTE)hwnd->pszTitle[ich - 1]) == kind) ich--;
    }
    return ich;
}

static int CxEdit(HDC hdc, LPCSTR psz, UINT cch, BOOL fPassword) {
    SIZE size;
    UINT i;
    int cx = 0;
    if (!fPassword) return GetTextExtentPoint32A(hdc, psz, (int)cch, &size) ? size.cx : 0;
    if (!GetTextExtentPoint32A(hdc, "*", 1, &size)) return 0;
    for (i = 0; i < cch; i++) cx += size.cx;
    return cx;
}

static void PaintEdit(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    HBRUSH hbr, hbrOwn = NULL;
    HFONT hfontOld = NULL;
    TEXTMETRICA tm;
    RECT rc, rcSel, format;
    UINT cch, ich, ichEnd, ichNext, ichFirst, ichLast, i;
    int x, y = 2, cy, cx, cxCaret, cxShift = 0, yCaret = 2, cLine = 0;
    BOOL fPassword = (hwnd->dwStyle & ES_PASSWORD) != 0, fFocus = GetFocus() == hwnd;
    COLORREF crText, crBk;
    char *pszDisplay;
    if (!hdc) return;
    GetClientRect(hwnd, &rc);
    SetTextColor(hdc, GetSysColor(IsWindowEnabled(hwnd) ? COLOR_WINDOWTEXT : COLOR_GRAYTEXT));
    SetBkColor(hdc, GetSysColor(COLOR_WINDOW)); SetBkMode(hdc, TRANSPARENT);
    if (hwnd->hfontControl) hfontOld = (HFONT)SelectObject(hdc, hwnd->hfontControl);
    hbr = (HBRUSH)SendMessageA(hwnd->hwndParent,
          (hwnd->dwStyle & ES_READONLY) || !IsWindowEnabled(hwnd) ? WM_CTLCOLORSTATIC : WM_CTLCOLOREDIT,
          (WPARAM)hdc, (LPARAM)hwnd);
    if (!IsWindow(hwnd)) return;
    if (!hbr) hbr = hbrOwn = CreateSolidBrush(GetBkColor(hdc));
    FillRect(hdc, &rc, hbr);
    if ((hwnd->dwStyle & WS_BORDER) || (hwnd->dwExStyle & WS_EX_CLIENTEDGE)) CompatDrawClassicEdge(hdc, &rc, TRUE, TRUE);
    EditFormat(hwnd, &format);
    IntersectClipRect(hdc, format.left, format.top, format.right, format.bottom);
    y = yCaret = format.top;
    GetTextMetricsA(hdc, &tm); cy = max(1, tm.tmHeight);
    cch = CchEdit(hwnd); pszDisplay = SDL_strdup(hwnd->pszTitle);
    if (!pszDisplay) goto done;
    if (fPassword) for (i = 0; i < cch; i++) if (pszDisplay[i] != '\r' && pszDisplay[i] != '\n') pszDisplay[i] = '*';
    ichFirst = min(hwnd->ichAnchor, hwnd->ichCaret); ichLast = max(hwnd->ichAnchor, hwnd->ichCaret);
    crText = SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    SetTextColor(hdc, crText); crBk = GetBkColor(hdc);
    cxCaret = CxEdit(hdc, pszDisplay + IchLineStart(hwnd, hwnd->ichCaret), hwnd->ichCaret - IchLineStart(hwnd, hwnd->ichCaret), FALSE);
    if (!(hwnd->dwStyle & ES_MULTILINE) && (hwnd->dwStyle & ES_AUTOHSCROLL)) cxShift = max(0, cxCaret - max(0, rc.right - 6));
    if (hwnd->dwStyle & ES_MULTILINE) {
        cLine = (int)IEditLine(hwnd, hwnd->ichCaret);
        if ((hwnd->dwStyle & ES_AUTOVSCROLL) && !hwnd->fEditManualScroll) {
            if (cLine * cy < hwnd->yEditScroll) hwnd->yEditScroll = cLine * cy;
            else if (cLine * cy + cy > hwnd->yEditScroll + max(0, format.bottom - format.top))
                hwnd->yEditScroll = max(0, cLine * cy + cy - max(0, format.bottom - format.top));
        }
        y -= hwnd->yEditScroll;
    }
    if ((hwnd->dwStyle & ES_MULTILINE) && (hwnd->dwStyle & (ES_AUTOHSCROLL | WS_HSCROLL))) cxShift = hwnd->xEditScroll;
    hwnd->xEditScroll = cxShift;
    SyncEditScroll(hwnd);
    if (!IsWindow(hwnd)) { SDL_free(pszDisplay); return; }
    for (ich = 0; ich <= cch; ich = ichNext) {
        ichEnd = hwnd->dwStyle & ES_MULTILINE ? IchLineEnd(hwnd, ich) : cch;
        ichNext = ichEnd;
        if (ichNext < cch && hwnd->pszTitle[ichNext] == '\r') ichNext++;
        if (ichNext < cch && hwnd->pszTitle[ichNext] == '\n') ichNext++;
        cx = CxEdit(hdc, pszDisplay + ich, ichEnd - ich, FALSE);
        x = format.left - cxShift;
        if (hwnd->dwStyle & ES_RIGHT) x += max(0, format.right - format.left - cx);
        else if (hwnd->dwStyle & ES_CENTER) x += max(0, (format.right - format.left - cx) / 2);
        TextOutA(hdc, x, y, pszDisplay + ich, (int)(ichEnd - ich));
        if ((fFocus || (hwnd->dwStyle & ES_NOHIDESEL)) && ichFirst < ichEnd && ichLast > ich) {
            UINT ichLo = max(ichFirst, ich), ichHi = min(ichLast, ichEnd);
            SetRect(&rcSel, x + CxEdit(hdc, pszDisplay + ich, ichLo - ich, FALSE), y,
                    x + CxEdit(hdc, pszDisplay + ich, ichHi - ich, FALSE), y + cy);
            hbr = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT)); FillRect(hdc, &rcSel, hbr); DeleteObject(hbr);
            SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT)); SetBkMode(hdc, TRANSPARENT);
            TextOutA(hdc, rcSel.left, y, pszDisplay + ichLo, (int)(ichHi - ichLo)); SetTextColor(hdc, crText); SetBkColor(hdc, crBk);
        }
        if (hwnd->ichCaret >= ich && hwnd->ichCaret <= ichEnd) {
            cxCaret = x + CxEdit(hdc, pszDisplay + ich, hwnd->ichCaret - ich, FALSE); yCaret = y;
        }
        if (ichEnd == cch || ichNext == ich) break;
        y += cy;
    }
    if (fFocus && hwnd->pszEditComposition && *hwnd->pszEditComposition) {
        UINT cchComp = (UINT)strlen(hwnd->pszEditComposition);
        char *pszComp = SDL_strdup(hwnd->pszEditComposition);
        if (pszComp) {
            if (fPassword) memset(pszComp, '*', cchComp);
            cx = CxEdit(hdc, pszComp, cchComp, FALSE);
            SetRect(&rcSel, cxCaret, yCaret, cxCaret + cx, yCaret + cy);
            hbr = CreateSolidBrush(crBk); FillRect(hdc, &rcSel, hbr); DeleteObject(hbr);
            TextOutA(hdc, cxCaret, yCaret, pszComp, (int)cchComp);
            MoveToEx(hdc, cxCaret, yCaret + cy - 1, NULL); LineTo(hdc, cxCaret + cx, yCaret + cy - 1);
            SDL_free(pszComp);
        }
    } else if (fFocus && hwnd->fEditCaret && ichFirst == ichLast) {
        HPEN hpenOld = (HPEN)SelectObject(hdc, GetStockObject(BLACK_PEN));
        MoveToEx(hdc, cxCaret, yCaret, NULL); LineTo(hdc, cxCaret, yCaret + cy);
        SelectObject(hdc, hpenOld);
    }
    SDL_free(pszDisplay);
done:
    if (hfontOld) SelectObject(hdc, hfontOld);
    if (hbrOwn) DeleteObject(hbrOwn);
    EndPaint(hwnd, &ps);
}

static UINT IchEditPoint(HWND hwnd, LPARAM l) {
    HDC hdc = GetDC(hwnd);
    HFONT hfontOld = NULL;
    TEXTMETRICA tm;
    UINT ich = 0, ichEnd, cch = CchEdit(hwnd);
    int x = (int16_t)LOWORD(l) + hwnd->xEditScroll;
    int y = (int16_t)HIWORD(l) + hwnd->yEditScroll, cx, cxPrev = 0;
    RECT rc;
    if (!hdc) return hwnd->ichCaret;
    if (hwnd->hfontControl) hfontOld = (HFONT)SelectObject(hdc, hwnd->hfontControl);
    GetTextMetricsA(hdc, &tm);
    EditFormat(hwnd, &rc); x -= rc.left; y -= rc.top;
    if (hwnd->dwStyle & ES_MULTILINE) while (y >= max(1, tm.tmHeight) && ich < cch) {
        ich = IchLineEnd(hwnd, ich);
        if (ich < cch && hwnd->pszTitle[ich] == '\r') ich++;
        if (ich < cch && hwnd->pszTitle[ich] == '\n') ich++;
        y -= max(1, tm.tmHeight);
    }
    ichEnd = hwnd->dwStyle & ES_MULTILINE ? IchLineEnd(hwnd, ich) : cch;
    EditFormat(hwnd, &rc);
    cx = CxEdit(hdc, hwnd->pszTitle + ich, ichEnd - ich, (hwnd->dwStyle & ES_PASSWORD) != 0);
    if (hwnd->dwStyle & ES_RIGHT) x -= max(0, rc.right - rc.left - cx);
    else if (hwnd->dwStyle & ES_CENTER) x -= max(0, (rc.right - rc.left - cx) / 2);
    for (; ich < ichEnd; ich++) {
        cx = CxEdit(hdc, hwnd->pszTitle + ich, 1, (hwnd->dwStyle & ES_PASSWORD) != 0);
        if (x < cxPrev + cx / 2) break;
        cxPrev += cx;
    }
    if (hfontOld) SelectObject(hdc, hfontOld);
    ReleaseDC(hwnd, hdc);
    return ich;
}

static void EditContextMenu(HWND hwnd, LPARAM l) {
    HMENU hmenu;
    POINT pt = {(int16_t)LOWORD(l), (int16_t)HIWORD(l)};
    BOOL fSelection = hwnd->ichAnchor != hwnd->ichCaret;
    BOOL fWrite = !(hwnd->dwStyle & ES_READONLY), fCopy = fSelection && !(hwnd->dwStyle & ES_PASSWORD);
    UINT id;
    hmenu = CreatePopupMenu();
    if (!hmenu) return;
    if (l == -1) { pt = (POINT){2, 2}; ClientToScreen(hwnd, &pt); }
    AppendMenuA(hmenu, MF_STRING | (hwnd->pszEditUndo && fWrite ? 0 : MF_GRAYED), WM_UNDO, "&Undo");
    AppendMenuA(hmenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hmenu, MF_STRING | (fCopy && fWrite ? 0 : MF_GRAYED), WM_CUT, "Cu&t");
    AppendMenuA(hmenu, MF_STRING | (fCopy ? 0 : MF_GRAYED), WM_COPY, "&Copy");
    AppendMenuA(hmenu, MF_STRING | (fWrite && SDL_HasClipboardText() ? 0 : MF_GRAYED), WM_PASTE, "&Paste");
    AppendMenuA(hmenu, MF_STRING | (fSelection && fWrite ? 0 : MF_GRAYED), WM_CLEAR, "&Delete");
    AppendMenuA(hmenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hmenu, MF_STRING | (CchEdit(hwnd) != 0 ? 0 : MF_GRAYED), 0xff00, "Select &All");
    id = (UINT)TrackPopupMenu(hmenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hmenu);
    if (!IsWindow(hwnd)) return;
    if (id == 0xff00) SendMessageA(hwnd, EM_SETSEL, 0, -1);
    else if (id != 0) SendMessageA(hwnd, id, 0, 0);
}

LRESULT CALLBACK CompatEditProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND lp = LpCompatWindow(hwnd);
    UINT cch, ich, ichStart, ichEnd, iLine, cchColumn, cLine, iUndoAction;
    BOOL fShift, fCtrl;
    LRESULT result;
    WORD cchMax;
    RECT rc;
    char sz[3];
    if (!lp) return 0;
    cch = CchEdit(hwnd);
    switch (msg) {
    case WM_NCCREATE:
        lp->iControl = 3; lp->cchLimit = 30000;
        NormalizeEditCase(hwnd);
        lp->fEditV = (lp->dwStyle & WS_VSCROLL) != 0; lp->fEditH = (lp->dwStyle & WS_HSCROLL) != 0;
        return TRUE;
    case WM_GETDLGCODE: return DLGC_WANTARROWS | DLGC_WANTCHARS | DLGC_HASSETSEL;
    case EM_GETSEL:
        ichStart = min(lp->ichAnchor, lp->ichCaret); ichEnd = max(lp->ichAnchor, lp->ichCaret);
        if (w) *(DWORD *)w = ichStart;
        if (l) *(DWORD *)l = ichEnd;
        return ichStart > UINT16_MAX || ichEnd > UINT16_MAX ? -1 : (LRESULT)MAKELONG(ichStart, ichEnd);
    case EM_SETSEL:
        if ((INT_PTR)w == -1) SelectEdit(hwnd, lp->ichCaret, lp->ichCaret);
        else SelectEdit(hwnd, (UINT)w, l == -1 ? cch : (UINT)l);
        return 0;
    case EM_LIMITTEXT: lp->cchLimit = w ? min((UINT_PTR)INT_MAX, w) : INT_MAX; return 0;
    case EM_GETLIMITTEXT: return lp->cchLimit;
    case EM_GETMODIFY: return lp->fModified;
    case EM_SETMODIFY: lp->fModified = w != 0; return 0;
    case EM_SETREADONLY:
        if (w) lp->dwStyle |= ES_READONLY; else lp->dwStyle &= ~ES_READONLY;
        InvalidateRect(hwnd, NULL, TRUE); return TRUE;
    case EM_REPLACESEL: if (l) ReplaceEdit(hwnd, (LPCSTR)l, FALSE, w != 0, w ? editUndoReplace : editUndoSeparate); return 0;
    case EM_CANUNDO: return lp->pszEditUndo != NULL;
    case EM_UNDO: case WM_UNDO: return UndoEdit(hwnd);
    case EM_EMPTYUNDOBUFFER: EmptyEditUndo(hwnd); return 0;
    case WM_COPY: CopyEdit(hwnd); return 0;
    case WM_CUT:
        if (!(lp->dwStyle & ES_READONLY) && CopyEdit(hwnd)) ReplaceEdit(hwnd, "", TRUE, TRUE, editUndoSeparate);
        return 0;
    case WM_CLEAR:
        if (lp->ichAnchor != lp->ichCaret) ReplaceEdit(hwnd, "", TRUE, TRUE, editUndoSeparate);
        return 0;
    case WM_PASTE: PasteEdit(hwnd); return 0;
    case EM_GETRECT: if (l) EditFormat(hwnd, (RECT *)l); return 0;
    case EM_SETRECT: case EM_SETRECTNP:
        if (!(lp->dwStyle & ES_MULTILINE)) return 0;
        lp->fEditFormat = l != 0;
        if (l) lp->rcEditFormat = *(RECT *)l;
        if (msg == EM_SETRECT) InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    case EM_GETFIRSTVISIBLELINE: return lp->dwStyle & ES_MULTILINE ? lp->yEditScroll / CyEdit(hwnd) : 0;
    case EM_LINESCROLL:
        if (!(lp->dwStyle & ES_MULTILINE)) return FALSE;
        ScrollEdit(hwnd, (int)w, (int)l); return TRUE;
    case EM_SCROLLCARET: lp->fEditManualScroll = FALSE; InvalidateRect(hwnd, NULL, TRUE); return 0;
    case WM_HSCROLL:
        if (LOWORD(w) == SB_LINELEFT) ScrollEdit(hwnd, -1, 0);
        else if (LOWORD(w) == SB_LINERIGHT) ScrollEdit(hwnd, 1, 0);
        else if (LOWORD(w) == SB_PAGELEFT || LOWORD(w) == SB_PAGERIGHT) {
            EditFormat(hwnd, &rc); ScrollEdit(hwnd, (LOWORD(w) == SB_PAGELEFT ? -1 : 1) * max(1, (rc.right - rc.left) / 8), 0);
        } else if (LOWORD(w) == SB_THUMBTRACK || LOWORD(w) == SB_THUMBPOSITION) {
            lp->xEditScroll = IsWindow(lp->rghwndScroll[SB_HORZ]) && lp->rghwndScroll[SB_HORZ]->fTracking ?
                lp->rghwndScroll[SB_HORZ]->iScrollTrack : HIWORD(w);
            ScrollEdit(hwnd, 0, 0);
        }
        return 0;
    case EM_SCROLL: case WM_VSCROLL:
        EditFormat(hwnd, &rc); cLine = (UINT)max(1, (rc.bottom - rc.top) / CyEdit(hwnd));
        ichStart = (UINT)(lp->yEditScroll / CyEdit(hwnd));
        if (LOWORD(w) == SB_LINEUP) ScrollEdit(hwnd, 0, -1);
        else if (LOWORD(w) == SB_LINEDOWN) ScrollEdit(hwnd, 0, 1);
        else if (LOWORD(w) == SB_PAGEUP) ScrollEdit(hwnd, 0, -(int)cLine);
        else if (LOWORD(w) == SB_PAGEDOWN) ScrollEdit(hwnd, 0, (int)cLine);
        else if (LOWORD(w) == SB_TOP) ScrollEdit(hwnd, 0, -(int)ichStart);
        else if (LOWORD(w) == SB_BOTTOM) ScrollEdit(hwnd, 0, INT_MAX);
        else if (LOWORD(w) == SB_THUMBTRACK || LOWORD(w) == SB_THUMBPOSITION) {
            int iPos = IsWindow(lp->rghwndScroll[SB_VERT]) && lp->rghwndScroll[SB_VERT]->fTracking ?
                lp->rghwndScroll[SB_VERT]->iScrollTrack : HIWORD(w);
            ScrollEdit(hwnd, 0, iPos - (int)ichStart);
        }
        return MAKELONG(lp->yEditScroll / CyEdit(hwnd) - (int)ichStart, 1);
    case EM_GETLINECOUNT:
        return lp->dwStyle & ES_MULTILINE ? IEditLine(hwnd, cch) + 1 : 1;
    case EM_LINEFROMCHAR:
        ichEnd = (INT_PTR)w == -1 ? lp->ichCaret : min((UINT)w, cch);
        return lp->dwStyle & ES_MULTILINE ? IEditLine(hwnd, ichEnd) : 0;
    case EM_LINEINDEX:
        if ((INT_PTR)w == -1) return IchLineStart(hwnd, lp->ichCaret);
        ich = IchEditLine(hwnd, (UINT)w); return ich == UINT_MAX ? -1 : (LRESULT)ich;
    case EM_LINELENGTH:
        ich = (INT_PTR)w == -1 ? lp->ichCaret : min((UINT)w, cch);
        ichStart = IchLineStart(hwnd, ich);
        return IchLineEnd(hwnd, ichStart) - ichStart;
    case EM_GETLINE:
        ich = IchEditLine(hwnd, (UINT)w);
        if (!l || ich == UINT_MAX) return 0;
        memcpy(&cchMax, (void *)l, sizeof(cchMax));
        cLine = min(cchMax, IchLineEnd(hwnd, ich) - ich);
        memcpy((void *)l, lp->pszTitle + ich, cLine); return cLine;
    case WM_SETTEXT:
        result = DefWindowProcA(hwnd, msg, w, l ? l : (LPARAM)"");
        if (result) {
            NormalizeEditCase(hwnd);
            EmptyEditUndo(hwnd);
            lp->ichAnchor = lp->ichCaret = 0; lp->fModified = FALSE;
            lp->xEditScroll = lp->yEditScroll = 0;
            ChangedEdit(hwnd, !(lp->dwStyle & ES_MULTILINE));
        }
        return result;
    case WM_RBUTTONUP:
        {
            POINT pt = {(int16_t)LOWORD(l), (int16_t)HIWORD(l)};
            ClientToScreen(hwnd, &pt); EditContextMenu(hwnd, MAKELPARAM(pt.x, pt.y));
        }
        return 0;
    case WM_CONTEXTMENU: EditContextMenu(hwnd, l); return 0;
    case WM_GETFONT: return (LRESULT)lp->hfontControl;
    case WM_SETFONT: lp->hfontControl = (HFONT)w; SyncEditScroll(hwnd); if (l && IsWindow(hwnd)) InvalidateRect(hwnd, NULL, TRUE); return 0;
    case WM_PAINT: PaintEdit(hwnd); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_ENABLE: case WM_SIZE:
        SyncEditScroll(hwnd); if (IsWindow(hwnd)) InvalidateRect(hwnd, NULL, TRUE); return 0;
    case WM_SETFOCUS: case WM_KILLFOCUS:
        lp->fEditCaret = msg == WM_SETFOCUS; lp->fEditTyping = FALSE; lp->iEditUndoAction = editUndoSeparate;
        if (msg == WM_KILLFOCUS) { SDL_free(lp->pszEditComposition); lp->pszEditComposition = NULL; }
        if (lp->idEditCaretTimer) KillTimer(hwnd, lp->idEditCaretTimer);
        lp->idEditCaretTimer = msg == WM_SETFOCUS ? SetTimer(hwnd, UINTPTR_MAX - 2, 530, NULL) : 0;
        InvalidateRect(hwnd, NULL, TRUE); NotifyEdit(hwnd, msg == WM_SETFOCUS ? EN_SETFOCUS : EN_KILLFOCUS); return 0;
    case WM_TIMER:
        if (w == lp->idEditCaretTimer && GetFocus() == hwnd) { lp->fEditCaret = !lp->fEditCaret; InvalidateRect(hwnd, NULL, FALSE); }
        return 0;
    case WM_LBUTTONDBLCLK:
        SetFocus(hwnd); if (!IsWindow(hwnd)) return 0;
        ich = IchEditPoint(hwnd, l); ichStart = ich;
        if (ich < cch) {
            int kind = ChEditClass((BYTE)lp->pszTitle[ich]);
            while (ichStart != 0 && ChEditClass((BYTE)lp->pszTitle[ichStart - 1]) == kind) ichStart--;
            while (ich < cch && ChEditClass((BYTE)lp->pszTitle[ich]) == kind) ich++;
        }
        SelectEdit(hwnd, ichStart, ich); return 0;
    case WM_LBUTTONDOWN:
        SetFocus(hwnd); if (!IsWindow(hwnd)) return 0;
        ich = IchEditPoint(hwnd, l);
        SelectEdit(hwnd, w & MK_SHIFT ? lp->ichAnchor : ich, ich);
        SetCapture(hwnd); return 0;
    case WM_MOUSEMOVE:
        if (GetCapture() == hwnd && (w & MK_LBUTTON)) SelectEdit(hwnd, lp->ichAnchor, IchEditPoint(hwnd, l));
        return 0;
    case WM_LBUTTONUP: if (GetCapture() == hwnd) ReleaseCapture(); return 0;
    case WM_KEYDOWN:
        fShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        fCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        ich = lp->ichCaret;
        if (w == VK_APPS || (w == VK_F10 && fShift)) { EditContextMenu(hwnd, -1); return 0; }
        if (fCtrl && w == 'A') { SelectEdit(hwnd, 0, cch); return 0; }
        if (fCtrl && w == 'C') { CopyEdit(hwnd); return 0; }
        if (fCtrl && w == 'X') return CompatEditProc(hwnd, WM_CUT, 0, 0);
        if (fCtrl && w == 'V') { PasteEdit(hwnd); return 0; }
        if (fCtrl && w == 'Z') { UndoEdit(hwnd); return 0; }
        if (w == VK_LEFT) {
            if (fCtrl) ich = IchEditWord(hwnd, ich, FALSE);
            else if (ich != 0) { ich--; if (ich != 0 && lp->pszTitle[ich] == '\n' && lp->pszTitle[ich - 1] == '\r') ich--; }
        } else if (w == VK_RIGHT) {
            if (fCtrl) ich = IchEditWord(hwnd, ich, TRUE);
            else if (ich < cch) { if (lp->pszTitle[ich] == '\r' && lp->pszTitle[ich + 1] == '\n') ich++; ich++; }
        }
        else if (w == VK_HOME) ich = !fCtrl && (lp->dwStyle & ES_MULTILINE) ? IchLineStart(hwnd, ich) : 0;
        else if (w == VK_END) ich = !fCtrl && (lp->dwStyle & ES_MULTILINE) ? IchLineEnd(hwnd, ich) : cch;
        else if ((w == VK_UP || w == VK_DOWN || w == VK_PRIOR || w == VK_NEXT) && (lp->dwStyle & ES_MULTILINE)) {
            ichStart = IchLineStart(hwnd, ich); cchColumn = ich - ichStart;
            EditFormat(hwnd, &rc);
            iLine = IEditLine(hwnd, ich);
            cLine = w == VK_PRIOR || w == VK_NEXT ? (UINT)max(1, (rc.bottom - rc.top) / CyEdit(hwnd)) : 1;
            iLine = w == VK_UP || w == VK_PRIOR ? (iLine > cLine ? iLine - cLine : 0) :
                    min(IEditLine(hwnd, cch), iLine + cLine);
            ichStart = IchEditLine(hwnd, iLine);
            ich = min(ichStart + cchColumn, IchLineEnd(hwnd, ichStart));
        }
        else if (w == VK_DELETE) {
            if (lp->dwStyle & ES_READONLY) return 0;
            iUndoAction = lp->ichAnchor == lp->ichCaret ? editUndoDelete : editUndoSeparate;
            if (lp->ichAnchor == ich && ich < cch) {
                lp->ichCaret = ich + 1;
                if (lp->pszTitle[ich] == '\r' && lp->pszTitle[ich + 1] == '\n') lp->ichCaret++;
            }
            if (lp->ichAnchor != lp->ichCaret) ReplaceEdit(hwnd, "", TRUE, TRUE, iUndoAction);
            return 0;
        } else return 0;
        SelectEdit(hwnd, fShift ? lp->ichAnchor : ich, ich); return 0;
    case WM_CHAR:
        if (lp->dwStyle & ES_READONLY) return 0;
        if (w == '\b') {
            iUndoAction = lp->ichAnchor == lp->ichCaret ? editUndoBackspace : editUndoSeparate;
            if (lp->ichAnchor == lp->ichCaret && lp->ichCaret != 0) {
                lp->ichAnchor--;
                if (lp->ichAnchor != 0 && lp->pszTitle[lp->ichAnchor] == '\n' && lp->pszTitle[lp->ichAnchor - 1] == '\r') lp->ichAnchor--;
            }
            if (lp->ichAnchor != lp->ichCaret) ReplaceEdit(hwnd, "", TRUE, TRUE, iUndoAction);
        } else if (w == '\r' && (lp->dwStyle & ES_MULTILINE)) ReplaceEdit(hwnd, "\r\n", TRUE, TRUE, editUndoSeparate);
        else if (w >= 32 && w <= 255 && (!(lp->dwStyle & ES_NUMBER) || (w >= '0' && w <= '9'))) {
            sz[0] = (char)w; sz[1] = 0; ReplaceEdit(hwnd, sz, TRUE, TRUE, editUndoTyping);
        }
        return 0;
    default: return DefWindowProcA(hwnd, msg, w, l);
    }
}
