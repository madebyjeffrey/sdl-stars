/* Edit message contracts run against both the local runtime and Windows SDK. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
#include "internal.h"
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static UINT cChange, cUpdate, cMax;
static BOOL fDestroy, fObserveUndo;
static UINT cUndoNotice, rgUndoNotice[4];
static DWORD rgUndoSel[4];
static BOOL rgUndoModified[4];
static LRESULT CALLBACK ParentProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_COMMAND) {
        if (fObserveUndo && (HIWORD(w) == EN_UPDATE || HIWORD(w) == EN_CHANGE)) {
            if (cUndoNotice < 4) {
                rgUndoNotice[cUndoNotice] = HIWORD(w);
                rgUndoSel[cUndoNotice] = (DWORD)SendMessageA((HWND)l, EM_GETSEL, 0, 0);
                rgUndoModified[cUndoNotice] = (BOOL)SendMessageA((HWND)l, EM_GETMODIFY, 0, 0);
            }
            cUndoNotice++;
        }
        if (HIWORD(w) == EN_CHANGE) cChange++;
        if (HIWORD(w) == EN_MAXTEXT) cMax++;
        if (HIWORD(w) == EN_UPDATE) {
            cUpdate++;
            if (fDestroy) { fDestroy = 0; DestroyWindow((HWND)l); }
        }
    }
    return DefWindowProcA(hwnd, msg, w, l);
}
static BOOL FText(HWND hwnd, LPCSTR psz) {
    char sz[128];
    int cch = GetWindowTextA(hwnd, sz, sizeof(sz));
    if (cch == (int)strlen(psz) && strcmp(sz, psz) == 0) return 1;
    fprintf(stderr, "edit text: expected [%s], got [%s]\n", psz, sz);
    return 0;
}
static int TestCaseStyles(HWND hwndParent, DWORD style) {
    HWND hwnd = CreateWindowExA(0, "Edit", "Abc dEF", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | style,
                             0, 210, 180, style & ES_MULTILINE ? 80 : 25, hwndParent, (HMENU)(UINT_PTR)111, NULL, NULL);
    LPCSTR pszCase = style & ES_UPPERCASE ? "ABC DEF" : "abc def";
    CHECK(hwnd);
    CHECK(FText(hwnd, pszCase));
    SendMessageA(hwnd, EM_LIMITTEXT, 3, 0);
    cChange = cUpdate = 0;
    CHECK(SetWindowTextA(hwnd, "Abc dEF") && FText(hwnd, pszCase));
    CHECK(!SendMessageA(hwnd, EM_GETMODIFY, 0, 0) && !SendMessageA(hwnd, EM_CANUNDO, 0, 0));
    CHECK(cUpdate == (style & ES_MULTILINE ? 0u : 1u) && cChange == cUpdate);
    SendMessageA(hwnd, EM_LIMITTEXT, 100, 0); SendMessageA(hwnd, EM_SETSEL, 0, -1);
    SendMessageA(hwnd, EM_REPLACESEL, 1, (LPARAM)"GhI");
    CHECK(FText(hwnd, style & ES_UPPERCASE ? "GHI" : "ghi"));
    CHECK(SendMessageA(hwnd, EM_UNDO, 0, 0) && FText(hwnd, pszCase));
    CHECK(SendMessageA(hwnd, EM_SETREADONLY, 1, 0));
    CHECK(SetWindowTextA(hwnd, "Abc dEF") && FText(hwnd, pszCase));
    SendMessageA(hwnd, EM_SETSEL, 0, -1); SendMessageA(hwnd, EM_REPLACESEL, 1, (LPARAM)"GhI");
    CHECK(FText(hwnd, style & ES_UPPERCASE ? "GHI" : "ghi"));
    CHECK(DestroyWindow(hwnd)); return 0;
}
static int TestEditBoundaries(HWND hwndParent, DWORD style) {
    HWND hwnd = CreateWindowExA(0, "Edit", "abc", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | style,
                             0, 210, 180, style & ES_MULTILINE ? 80 : 25, hwndParent, (HMENU)(UINT_PTR)110, NULL, NULL);
    CHECK(hwnd);
    SetFocus(hwnd);
    SendMessageA(hwnd, EM_LIMITTEXT, 3, 0); SendMessageA(hwnd, EM_SETSEL, 3, 3);
    SendMessageA(hwnd, EM_SETMODIFY, 0, 0); cChange = cUpdate = cMax = 0;
    SendMessageA(hwnd, WM_CHAR, 'x', 1);
    CHECK(FText(hwnd, "abc") && cUpdate == 1 && cChange == 1 && cMax == 1);
    CHECK(SendMessageA(hwnd, EM_GETMODIFY, 0, 0) && !SendMessageA(hwnd, EM_CANUNDO, 0, 0));
    CHECK(SetWindowTextA(hwnd, "abcd")); SendMessageA(hwnd, EM_LIMITTEXT, 100, 0); SendMessageA(hwnd, EM_SETSEL, 4, 4);
    SendMessageA(hwnd, WM_CHAR, '\b', 1); SendMessageA(hwnd, WM_CHAR, '\b', 1); SendMessageA(hwnd, EM_UNDO, 0, 0);
    CHECK(FText(hwnd, "abcd") && SendMessageA(hwnd, EM_GETSEL, 0, 0) == MAKELONG(2, 4));
    CHECK(SendMessageA(hwnd, EM_UNDO, 0, 0) && FText(hwnd, "ab"));
    CHECK(SetWindowTextA(hwnd, "abcd")); SendMessageA(hwnd, EM_SETSEL, 1, 1);
    SendMessageA(hwnd, WM_KEYDOWN, VK_DELETE, 1); SendMessageA(hwnd, WM_KEYDOWN, VK_DELETE, 1);
    SendMessageA(hwnd, EM_UNDO, 0, 0);
    CHECK(FText(hwnd, "abcd") && SendMessageA(hwnd, EM_GETSEL, 0, 0) == MAKELONG(1, 3));
    CHECK(SetWindowTextA(hwnd, "ab")); SendMessageA(hwnd, EM_LIMITTEXT, 3, 0); SendMessageA(hwnd, EM_SETSEL, 2, 2);
    SendMessageA(hwnd, WM_CHAR, 'c', 1); SendMessageA(hwnd, EM_SETSEL, 3, 3); SendMessageA(hwnd, WM_CHAR, 'x', 1);
    CHECK(SendMessageA(hwnd, EM_UNDO, 0, 0) && FText(hwnd, "ab"));
    CHECK(SetWindowTextA(hwnd, "abcd")); SendMessageA(hwnd, EM_LIMITTEXT, 100, 0); SendMessageA(hwnd, EM_SETSEL, 4, 4);
    SendMessageA(hwnd, WM_CHAR, '\b', 1); SendMessageA(hwnd, EM_SETSEL, 2, 2); SendMessageA(hwnd, WM_CHAR, '\b', 1);
    CHECK(SendMessageA(hwnd, EM_UNDO, 0, 0) && FText(hwnd, "abc"));
    if (style & ES_MULTILINE) {
        CHECK(SetWindowTextA(hwnd, "x\r\nab")); SendMessageA(hwnd, EM_SETSEL, 5, 5);
        SendMessageA(hwnd, WM_CHAR, '\b', 1); SendMessageA(hwnd, WM_CHAR, '\b', 1); SendMessageA(hwnd, WM_CHAR, '\b', 1);
        CHECK(FText(hwnd, "x") && SendMessageA(hwnd, EM_UNDO, 0, 0) && FText(hwnd, "x\r\nab"));
        CHECK(SendMessageA(hwnd, EM_GETSEL, 0, 0) == MAKELONG(1, 5));
    }
    CHECK(DestroyWindow(hwnd)); return 0;
}
static int TestMixedUndo(HWND hwndParent, DWORD style) {
    HWND hwnd = CreateWindowExA(0, "Edit", "abcde", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | style,
                             0, 210, 180, style & ES_MULTILINE ? 30 : 25, hwndParent, (HMENU)(UINT_PTR)113, NULL, NULL);
    CHECK(hwnd);
    SetFocus(hwnd); SendMessageA(hwnd, EM_SETSEL, 2, 2);
    SendMessageA(hwnd, WM_KEYDOWN, VK_DELETE, 1);
    SendMessageA(hwnd, WM_CHAR, '\b', 1);
    CHECK(FText(hwnd, "ade"));
    CHECK(SendMessageA(hwnd, EM_UNDO, 0, 0) && FText(hwnd, "abcde"));
    CHECK(SetWindowTextA(hwnd, "abcde")); SendMessageA(hwnd, EM_SETSEL, 2, 2);
    SendMessageA(hwnd, WM_CHAR, '\b', 1); SendMessageA(hwnd, WM_KEYDOWN, VK_DELETE, 1);
    CHECK(FText(hwnd, "ade") && SendMessageA(hwnd, EM_UNDO, 0, 0) && FText(hwnd, "abcde"));
    CHECK(SetWindowTextA(hwnd, "abcde")); SendMessageA(hwnd, EM_SETSEL, 1, 3);
    SendMessageA(hwnd, EM_REPLACESEL, 1, (LPARAM)"XY");
    CHECK(FText(hwnd, "aXYde"));
    SendMessageA(hwnd, WM_CHAR, 'z', 1);
    CHECK(FText(hwnd, "aXYzde"));
    CHECK(SendMessageA(hwnd, EM_UNDO, 0, 0) && FText(hwnd, "abcde"));
    CHECK(SendMessageA(hwnd, EM_UNDO, 0, 0) && FText(hwnd, "aXYzde"));
    CHECK(SetWindowTextA(hwnd, "abcde")); SendMessageA(hwnd, EM_SETSEL, 1, 4);
    SendMessageA(hwnd, EM_REPLACESEL, 0, (LPARAM)"X");
    CHECK(FText(hwnd, "aXe") && !SendMessageA(hwnd, EM_CANUNDO, 0, 0));
    CHECK(DestroyWindow(hwnd)); return 0;
}
static int TestUndoNoops(HWND hwndParent, DWORD style) {
    HWND hwnd = CreateWindowExA(0, "Edit", "abcdef", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | style,
                             0, 210, 180, style & ES_MULTILINE ? 80 : 25, hwndParent, (HMENU)(UINT_PTR)112, NULL, NULL);
    UINT rgmsg[] = {EM_UNDO, WM_UNDO};
    BYTE rgKeySav[256], rgKey[256];
    int iKey;
    size_t i;
    LRESULT result;
    CHECK(hwnd);
    for (i = 0; i < sizeof(rgmsg) / sizeof(rgmsg[0]); i++) {
        CHECK(SetWindowTextA(hwnd, "abcdef"));
        SendMessageA(hwnd, EM_SETSEL, 2, 4); SendMessageA(hwnd, EM_SETMODIFY, 0, 0);
        cChange = cUpdate = cMax = 0;
        for (iKey = 0; iKey < 256; iKey++)
            rgKeySav[iKey] = ((GetKeyState(iKey) & 0x8000) ? 0x80 : 0) | (GetKeyState(iKey) & 1);
        memcpy(rgKey, rgKeySav, sizeof(rgKey)); rgKey[VK_CONTROL] = 0x80;
        CHECK(SetKeyboardState(rgKey));
        SendMessageA(hwnd, WM_KEYDOWN, 'Z', 1); SendMessageA(hwnd, WM_CHAR, 26, 1); SendMessageA(hwnd, WM_KEYUP, 'Z', 1);
        CHECK(SetKeyboardState(rgKeySav));
        CHECK(FText(hwnd, "abcdef") && SendMessageA(hwnd, EM_GETSEL, 0, 0) == (i == 0 ? 0 : MAKELONG(6, 6)));
        CHECK(cUpdate == 0 && cChange == 1 && cMax == 0);
        cChange = cUpdate = cMax = 0;
        result = SendMessageA(hwnd, rgmsg[i], 0, 0);
        CHECK(result && SendMessageA(hwnd, EM_GETSEL, 0, 0) == (i == 0 ? 0 : MAKELONG(6, 6)));
        CHECK(!SendMessageA(hwnd, EM_GETMODIFY, 0, 0) && !SendMessageA(hwnd, EM_CANUNDO, 0, 0));
        CHECK(cUpdate == 0 && cChange == 1 && cMax == 0);
        CHECK(FText(hwnd, "abcdef"));
        CHECK(SendMessageA(hwnd, EM_SETREADONLY, 1, 0)); cChange = cUpdate = cMax = 0;
        CHECK(!!SendMessageA(hwnd, rgmsg[i], 0, 0) == !(style & ES_MULTILINE));
        CHECK(!SendMessageA(hwnd, EM_CANUNDO, 0, 0) && !SendMessageA(hwnd, EM_GETMODIFY, 0, 0));
        CHECK(cUpdate == 0 && cChange == 0 && cMax == 0);
        CHECK(SendMessageA(hwnd, EM_SETREADONLY, 0, 0));
        SendMessageA(hwnd, EM_SETSEL, 6, 6); SendMessageA(hwnd, WM_CHAR, 'x', 1);
        CHECK(SendMessageA(hwnd, EM_CANUNDO, 0, 0));
        SendMessageA(hwnd, EM_SETSEL, 1, 2); CHECK(SendMessageA(hwnd, EM_SETREADONLY, 1, 0));
        cChange = cUpdate = cMax = 0;
        result = SendMessageA(hwnd, rgmsg[i], 0, 0);
        CHECK(!!result == !(style & ES_MULTILINE));
        CHECK(SendMessageA(hwnd, EM_GETSEL, 0, 0) == MAKELONG(1, 2) && SendMessageA(hwnd, EM_GETMODIFY, 0, 0));
        CHECK(cUpdate == 0 && cChange == 0 && cMax == 0);
        CHECK(FText(hwnd, "abcdefx") && SendMessageA(hwnd, EM_CANUNDO, 0, 0));
        CHECK(SendMessageA(hwnd, EM_SETREADONLY, 0, 0));
        CHECK(SendMessageA(hwnd, rgmsg[i], 0, 0) && FText(hwnd, "abcdef"));
    }
    SendMessageA(hwnd, EM_EMPTYUNDOBUFFER, 0, 0); SendMessageA(hwnd, EM_SETSEL, 2, 4);
    cChange = cUpdate = cMax = 0;
    CHECK(SendMessageA(hwnd, EM_UNDO, 0, 0) && SendMessageA(hwnd, EM_GETSEL, 0, 0) == MAKELONG(6, 6));
    CHECK(cUpdate == 0 && cChange == 1 && cMax == 0 && !SendMessageA(hwnd, EM_CANUNDO, 0, 0));
    CHECK(SetWindowTextA(hwnd, "a"));
    CHECK(SendMessageA(hwnd, WM_UNDO, 0, 0) && FText(hwnd, "a"));
    CHECK(SendMessageA(hwnd, EM_GETSEL, 0, 0) == MAKELONG(1, 1));
    CHECK(DestroyWindow(hwnd)); return 0;
}

static BOOL FUndoNotices(DWORD dwCaret, DWORD dwSelection) {
    return cUndoNotice == 3 && rgUndoNotice[0] == EN_UPDATE && rgUndoNotice[1] == EN_CHANGE && rgUndoNotice[2] == EN_CHANGE &&
           rgUndoSel[0] == dwCaret && rgUndoSel[1] == dwCaret && rgUndoSel[2] == dwSelection &&
           rgUndoModified[0] && rgUndoModified[1] && rgUndoModified[2];
}
static int TestUndoChanges(HWND hwndParent, DWORD style) {
    HWND hwnd = CreateWindowExA(0, "Edit", "abc", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | style,
                             0, 210, 180, style & ES_MULTILINE ? 80 : 25, hwndParent, (HMENU)(UINT_PTR)113, NULL, NULL);
    UINT rgmsg[] = {EM_UNDO, WM_UNDO};
    size_t i;
    CHECK(hwnd);
    for (i = 0; i < sizeof(rgmsg) / sizeof(rgmsg[0]); i++) {
        CHECK(SetWindowTextA(hwnd, "abc")); SendMessageA(hwnd, EM_SETSEL, 3, 3);
        SendMessageA(hwnd, WM_CHAR, 'd', 1); CHECK(FText(hwnd, "abcd"));
        SendMessageA(hwnd, EM_SETMODIFY, 0, 0); cChange = cUpdate = cMax = cUndoNotice = 0; fObserveUndo = 1;
        CHECK(SendMessageA(hwnd, rgmsg[i], 0, 0) && FText(hwnd, "abc")); fObserveUndo = 0;
        CHECK(SendMessageA(hwnd, EM_GETMODIFY, 0, 0) && SendMessageA(hwnd, EM_GETSEL, 0, 0) == MAKELONG(3, 3));
        CHECK(cUpdate == 1 && cChange == 2 && cMax == 0 && FUndoNotices(MAKELONG(3, 3), MAKELONG(3, 3)));
        SendMessageA(hwnd, EM_SETMODIFY, 0, 0); cChange = cUpdate = cMax = cUndoNotice = 0; fObserveUndo = 1;
        CHECK(SendMessageA(hwnd, rgmsg[i], 0, 0) && FText(hwnd, "abcd")); fObserveUndo = 0;
        CHECK(SendMessageA(hwnd, EM_GETMODIFY, 0, 0) && SendMessageA(hwnd, EM_GETSEL, 0, 0) == MAKELONG(3, 4));
        CHECK(cUpdate == 1 && cChange == 2 && cMax == 0 && FUndoNotices(MAKELONG(4, 4), MAKELONG(3, 4)));
        CHECK(SetWindowTextA(hwnd, "aaaa")); SendMessageA(hwnd, EM_SETSEL, 1, 3);
        SendMessageA(hwnd, EM_REPLACESEL, 1, (LPARAM)"aa");
        CHECK(SendMessageA(hwnd, EM_CANUNDO, 0, 0));
        CHECK(SendMessageA(hwnd, rgmsg[i], 0, 0) && FText(hwnd, "aaaa") && SendMessageA(hwnd, EM_GETSEL, 0, 0) == MAKELONG(1, 3));
        CHECK(SendMessageA(hwnd, rgmsg[i], 0, 0) && FText(hwnd, "aaaa") && SendMessageA(hwnd, EM_GETSEL, 0, 0) == MAKELONG(1, 3));
        CHECK(SetWindowTextA(hwnd, "aaabbbccc")); SendMessageA(hwnd, EM_SETSEL, 3, 6);
        SendMessageA(hwnd, EM_REPLACESEL, 1, (LPARAM)""); CHECK(FText(hwnd, "aaaccc"));
        CHECK(SendMessageA(hwnd, rgmsg[i], 0, 0) && FText(hwnd, "aaabbbccc") && SendMessageA(hwnd, EM_GETSEL, 0, 0) == MAKELONG(3, 6));
        CHECK(SendMessageA(hwnd, rgmsg[i], 0, 0) && FText(hwnd, "aaaccc") && SendMessageA(hwnd, EM_GETSEL, 0, 0) == MAKELONG(3, 3));
    }
    fDestroy = 1;
    CHECK(SendMessageA(hwnd, EM_UNDO, 0, 0) && !IsWindow(hwnd) && !fDestroy);
    return 0;
}

static int TestEditing(HWND hwndEdit, HWND hwndMulti) {
    char sz[16];
    WORD cchMax = 3;
    CHECK(SetWindowTextA(hwndEdit, "alpha beta"));
    SendMessageA(hwndEdit, EM_LIMITTEXT, 100, 0);
    SendMessageA(hwndEdit, EM_SETSEL, 0, 5);
    SendMessageA(hwndEdit, WM_COPY, 0, 0);
    SendMessageA(hwndEdit, EM_SETSEL, 10, 10);
    SendMessageA(hwndEdit, WM_PASTE, 0, 0);
    CHECK(FText(hwndEdit, "alpha betaalpha"));
    CHECK(SendMessageA(hwndEdit, EM_CANUNDO, 0, 0));
    CHECK(SendMessageA(hwndEdit, EM_UNDO, 0, 0) && FText(hwndEdit, "alpha beta"));
    CHECK(SendMessageA(hwndEdit, WM_UNDO, 0, 0) && FText(hwndEdit, "alpha betaalpha"));
    SendMessageA(hwndEdit, EM_EMPTYUNDOBUFFER, 0, 0);
    CHECK(!SendMessageA(hwndEdit, EM_CANUNDO, 0, 0));
    SendMessageA(hwndEdit, EM_SETSEL, 0, 5);
    SendMessageA(hwndEdit, WM_CUT, 0, 0);
    CHECK(FText(hwndEdit, " betaalpha"));
    CHECK(SendMessageA(hwndEdit, EM_UNDO, 0, 0) && FText(hwndEdit, "alpha betaalpha"));
    CHECK(SetWindowTextA(hwndEdit, "reset") && !SendMessageA(hwndEdit, EM_CANUNDO, 0, 0));
    SendMessageA(hwndEdit, EM_SETSEL, 0, -1);
    SendMessageA(hwndEdit, EM_REPLACESEL, 0, (LPARAM)"replacement");
    CHECK(!SendMessageA(hwndEdit, EM_CANUNDO, 0, 0));
    CHECK(SetWindowTextA(hwndMulti, "aaa\r\nbbb\r\nc"));
    CHECK(SendMessageA(hwndMulti, EM_GETLINECOUNT, 0, 0) == 3);
    CHECK(SendMessageA(hwndMulti, EM_LINEINDEX, 1, 0) == 5);
    CHECK(SendMessageA(hwndMulti, EM_LINEINDEX, 3, 0) == -1);
    CHECK(SendMessageA(hwndMulti, EM_LINEFROMCHAR, 7, 0) == 1);
    CHECK(SendMessageA(hwndMulti, EM_LINELENGTH, 7, 0) == 3);
    memcpy(sz, &cchMax, sizeof(cchMax));
    CHECK(SendMessageA(hwndMulti, EM_GETLINE, 1, (LPARAM)sz) == 3 && memcmp(sz, "bbb", 3) == 0);
    SendMessageA(hwndMulti, EM_SETSEL, 2, 2);
    SendMessageA(hwndMulti, WM_KEYDOWN, VK_DOWN, 1);
    CHECK(SendMessageA(hwndMulti, EM_GETSEL, 0, 0) == MAKELONG(7, 7));
    SendMessageA(hwndMulti, WM_KEYDOWN, VK_UP, 1);
    CHECK(SendMessageA(hwndMulti, EM_GETSEL, 0, 0) == MAKELONG(2, 2));
    return 0;
}
static int TestSelectionArrows(HWND hwnd, LPCSTR psz) {
    static const struct {
        UINT ichAnchor, ichCaret, vk;
        BOOL fShift;
        DWORD dwSel;
    } rgtest[] = {
        {1, 4, VK_LEFT, 0, MAKELONG(1, 1)},
        {1, 4, VK_RIGHT, 0, MAKELONG(4, 4)},
        {4, 1, VK_LEFT, 0, MAKELONG(1, 1)},
        {4, 1, VK_RIGHT, 0, MAKELONG(4, 4)},
        {1, 4, VK_LEFT, 1, MAKELONG(1, 3)},
        {1, 4, VK_RIGHT, 1, MAKELONG(1, 5)},
        {4, 1, VK_LEFT, 1, MAKELONG(0, 4)},
        {4, 1, VK_RIGHT, 1, MAKELONG(3, 4)},
        {3, 3, VK_LEFT, 0, MAKELONG(1, 1)},
        {3, 3, VK_RIGHT, 0, MAKELONG(4, 4)},
        {0, 5, VK_LEFT, 0, MAKELONG(0, 0)},
        {0, 5, VK_RIGHT, 0, MAKELONG(5, 5)}
    };
    BYTE rgbSav[256], rgb[256] = {0};
    int iKey;
    size_t i;

    for (iKey = 0; iKey < 256; iKey++)
        rgbSav[iKey] = ((GetKeyState(iKey) & 0x8000) ? 0x80 : 0) | (GetKeyState(iKey) & 1);
    CHECK(SetWindowTextA(hwnd, psz));
    for (i = 0; i < sizeof(rgtest) / sizeof(rgtest[0]); i++) {
        rgb[VK_SHIFT] = rgtest[i].fShift ? 0x80 : 0;
        CHECK(SetKeyboardState(rgb));
        SendMessageA(hwnd, EM_SETSEL, rgtest[i].ichAnchor, rgtest[i].ichCaret);
        SendMessageA(hwnd, WM_KEYDOWN, rgtest[i].vk, 1);
        CHECK(SendMessageA(hwnd, EM_GETSEL, 0, 0) == rgtest[i].dwSel);
    }
    CHECK(SetKeyboardState(rgbSav));
    CHECK(FText(hwnd, psz) && !SendMessageA(hwnd, EM_GETMODIFY, 0, 0));
    return 0;
}

static int TestWrapping(HWND hwndParent) {
    HWND hwnd = CreateWindowExA(0, "Edit", "one two three four five six seven eight nine ten eleven twelve",
                             WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL,
                             0, 160, 70, 45, hwndParent, NULL, NULL, NULL);
    RECT rc = {3, 3, 60, 35}, got;
    int cLine, i, cch, ich, cchTotal = 0;
    WORD cchMax = 128;
    char sz[130], text[256] = {0};
    CHECK(hwnd);
    SendMessageA(hwnd, EM_SETRECT, 0, (LPARAM)&rc);
    SendMessageA(hwnd, EM_GETRECT, 0, (LPARAM)&got);
    CHECK(got.left == rc.left && got.top == rc.top && got.right == rc.right && got.bottom == rc.bottom);
    cLine = (int)SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0); CHECK(cLine > 3);
    for (i = 0; i < cLine; i++) {
        ich = (int)SendMessageA(hwnd, EM_LINEINDEX, i, 0); CHECK(ich == cchTotal);
        memcpy(sz, &cchMax, sizeof(cchMax));
        cch = (int)SendMessageA(hwnd, EM_GETLINE, i, (LPARAM)sz); CHECK(cch > 0 && cch < 128);
        memcpy(text + cchTotal, sz, (size_t)cch); cchTotal += cch;
        CHECK(SendMessageA(hwnd, EM_LINEFROMCHAR, ich, 0) == i);
    }
    CHECK(FText(hwnd, text));
    CHECK(SendMessageA(hwnd, EM_LINESCROLL, 0, 2));
    CHECK(SendMessageA(hwnd, EM_GETFIRSTVISIBLELINE, 0, 0) == 2);
    SendMessageA(hwnd, EM_SETSEL, 0, 0); SendMessageA(hwnd, EM_SCROLLCARET, 0, 0); UpdateWindow(hwnd);
    CHECK(SendMessageA(hwnd, EM_GETFIRSTVISIBLELINE, 0, 0) == 0);
    rc.right = 400; SendMessageA(hwnd, EM_SETRECT, 0, (LPARAM)&rc);
    CHECK(SendMessageA(hwnd, EM_GETLINECOUNT, 0, 0) < cLine);
    CHECK(DestroyWindow(hwnd)); return 0;
}
#if !defined(_WIN32) || defined(STARS_UI_SDL)
static int TestWordNavigation(HWND hwndEdit, HWND hwndMulti) {
    static const struct {
        SDL_Scancode sc;
        BOOL fShift;
        UINT ichAnchor, ichCaret;
        DWORD dwSel;
    } rgkey[] = {
        {SDL_SCANCODE_LEFT, 0, 6, 6, MAKELONG(4, 4)},
        {SDL_SCANCODE_RIGHT, 0, 6, 6, MAKELONG(8, 8)},
        {SDL_SCANCODE_LEFT, 1, 6, 6, MAKELONG(4, 6)},
        {SDL_SCANCODE_RIGHT, 1, 6, 6, MAKELONG(6, 8)},
        {SDL_SCANCODE_LEFT, 1, 6, 8, MAKELONG(4, 6)},
        {SDL_SCANCODE_RIGHT, 1, 6, 4, MAKELONG(6, 8)},
        {SDL_SCANCODE_LEFT, 1, 3, 3, MAKELONG(0, 3)},
        {SDL_SCANCODE_RIGHT, 1, 0, 0, MAKELONG(0, 4)},
        {SDL_SCANCODE_LEFT, 0, 0, 0, MAKELONG(0, 0)},
        {SDL_SCANCODE_RIGHT, 0, 13, 13, MAKELONG(13, 13)}
    };
    static const struct {
        SDL_Keymod mod;
        SDL_Scancode sc;
    } rgmod[] = {
#ifdef __APPLE__
        {SDL_KMOD_LALT, SDL_SCANCODE_LALT}, {SDL_KMOD_RALT, SDL_SCANCODE_RALT},
#endif
        {SDL_KMOD_LCTRL, SDL_SCANCODE_LCTRL}, {SDL_KMOD_RCTRL, SDL_SCANCODE_RCTRL}
    };
    HWND rghwnd[] = {hwndEdit, hwndMulti};
    SDL_Event event = {0};
    APPMSG msg;
    size_t i, j, k;

    for (i = 0; i < 2; i++) {
        SetFocus(rghwnd[i]);
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        SendMessageA(rghwnd[i], EM_LIMITTEXT, 100, 0);
        CHECK(SetWindowTextA(rghwnd[i], "o\xc3\xa9 two three"));
        for (k = 0; k < sizeof(rgmod) / sizeof(rgmod[0]); k++) {
            for (j = 0; j < sizeof(rgkey) / sizeof(rgkey[0]); j++) {
                SendMessageA(rghwnd[i], EM_SETSEL, rgkey[j].ichAnchor, rgkey[j].ichCaret);
                event.type = SDL_EVENT_KEY_DOWN;
                event.key.windowID = HwndCompatRoot(rghwnd[i])->id;
                event.key.scancode = rgmod[k].sc;
                event.key.mod = rgmod[k].mod | (rgkey[j].fShift ? SDL_KMOD_LSHIFT : 0);
                CHECK(SDL_PushEvent(&event));
                event.key.scancode = rgkey[j].sc;
                CHECK(SDL_PushEvent(&event));
                event.type = SDL_EVENT_KEY_UP;
                CHECK(SDL_PushEvent(&event));
                event.key.scancode = rgmod[k].sc;
                event.key.mod = SDL_KMOD_NONE;
                CHECK(SDL_PushEvent(&event));
                while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg); DispatchMessageA(&msg);
                }
                CHECK(SendMessageA(rghwnd[i], EM_GETSEL, 0, 0) == rgkey[j].dwSel);
            }
        }
        CHECK(FText(rghwnd[i], "o\xc3\xa9 two three") && !SendMessageA(rghwnd[i], EM_GETMODIFY, 0, 0));
    }
    return 0;
}
#endif

#ifdef __APPLE__
static int TestMacNavigation(HWND hwndEdit, HWND hwndMulti) {
    static const struct {
        SDL_Scancode sc;
        SDL_Keymod mod;
        DWORD rgdwSel[2];
    } rgkey[] = {
        {SDL_SCANCODE_LEFT, SDL_KMOD_LGUI, {MAKELONG(0, 0), MAKELONG(7, 7)}},
        {SDL_SCANCODE_RIGHT, SDL_KMOD_RGUI, {MAKELONG(5, 5), MAKELONG(12, 12)}},
        {SDL_SCANCODE_LEFT, SDL_KMOD_LGUI | SDL_KMOD_LSHIFT, {MAKELONG(0, 3), MAKELONG(7, 10)}},
        {SDL_SCANCODE_RIGHT, SDL_KMOD_RGUI | SDL_KMOD_RSHIFT, {MAKELONG(3, 5), MAKELONG(10, 12)}},
        {SDL_SCANCODE_LEFT, SDL_KMOD_NONE, {MAKELONG(1, 1), MAKELONG(8, 8)}},
        {SDL_SCANCODE_RIGHT, SDL_KMOD_NONE, {MAKELONG(4, 4), MAKELONG(11, 11)}},
        {SDL_SCANCODE_LEFT, SDL_KMOD_LCTRL, {MAKELONG(0, 0), MAKELONG(7, 7)}},
        {SDL_SCANCODE_RIGHT, SDL_KMOD_RCTRL, {MAKELONG(5, 5), MAKELONG(14, 14)}}
    };
    HWND rghwnd[] = {hwndEdit, hwndMulti};
    const char *rgpsz[] = {"A\xc3\xa9" "BC", "first\r\nA\xc3\xa9" "BC\r\nlast"};
    SDL_Event event = {0};
    APPMSG msg;
    UINT ichBase;
    size_t i, j;

    for (i = 0; i < 2; i++) {
        SetFocus(rghwnd[i]);
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        SendMessageA(rghwnd[i], EM_LIMITTEXT, 100, 0);
        CHECK(SetWindowTextA(rghwnd[i], rgpsz[i]));
        ichBase = i == 0 ? 0 : 7;
        for (j = 0; j < sizeof(rgkey) / sizeof(rgkey[0]); j++) {
            // Continue the preceding Shift selection to check its anchor survives.
            if (j != 3) SendMessageA(rghwnd[i], EM_SETSEL, ichBase + 3, ichBase + 3);
            event.type = SDL_EVENT_KEY_DOWN;
            event.key.windowID = HwndCompatRoot(rghwnd[i])->id;
            event.key.scancode = rgkey[j].sc;
            event.key.mod = rgkey[j].mod;
            CHECK(SDL_PushEvent(&event));
            event.type = SDL_EVENT_KEY_UP;
            event.key.mod = rgkey[j].mod & SDL_KMOD_GUI ? rgkey[j].mod : SDL_KMOD_NONE;
            CHECK(SDL_PushEvent(&event));
            // Release Command before dispatch, exercising the queued key snapshot.
            if (rgkey[j].mod & SDL_KMOD_GUI) {
                event.key.scancode = rgkey[j].mod & SDL_KMOD_LGUI ? SDL_SCANCODE_LGUI : SDL_SCANCODE_RGUI;
                event.key.mod = SDL_KMOD_NONE;
                CHECK(SDL_PushEvent(&event));
            }
            while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg); DispatchMessageA(&msg);
            }
            if (SendMessageA(rghwnd[i], EM_GETSEL, 0, 0) != rgkey[j].rgdwSel[i]) {
                fprintf(stderr, "Mac navigation: field %zu key %zu, selection %lu expected %lu\n", i, j,
                        (unsigned long)SendMessageA(rghwnd[i], EM_GETSEL, 0, 0),
                        (unsigned long)rgkey[j].rgdwSel[i]);
                return 1;
            }
            CHECK(FText(rghwnd[i], rgpsz[i]));
            CHECK(!(GetKeyState(VK_LWIN) & 0x8000) && !(GetKeyState(VK_RWIN) & 0x8000));
        }
    }
    return 0;
}
#endif

int main(void) {
    WNDCLASS wc = {0};
    HWND hwnd, hwndEdit, hwndMulti, hwndRead, hwndKill;
    DWORD ichFirst, ichLast;
    UINT c;
    wc.lpfnWndProc = ParentProc; wc.lpszClassName = "CompatEditParent";
    CHECK(RegisterClassA(&wc));
    hwnd = CreateWindowExA(0, wc.lpszClassName, "Edit test", WS_POPUP | WS_VISIBLE, 50, 50, 400, 300, NULL, NULL, NULL, NULL);
    CHECK(hwnd);
    CHECK(TestCaseStyles(hwnd, ES_UPPERCASE) == 0 && TestCaseStyles(hwnd, ES_LOWERCASE) == 0);
    CHECK(TestCaseStyles(hwnd, ES_UPPERCASE | ES_MULTILINE) == 0 && TestCaseStyles(hwnd, ES_LOWERCASE | ES_MULTILINE) == 0);
    CHECK(TestUndoChanges(hwnd, 0) == 0 && TestUndoChanges(hwnd, ES_MULTILINE) == 0);
    CHECK(TestUndoNoops(hwnd, 0) == 0 && TestUndoNoops(hwnd, ES_MULTILINE) == 0);
    CHECK(TestEditBoundaries(hwnd, 0) == 0 && TestEditBoundaries(hwnd, ES_MULTILINE) == 0);
    CHECK(TestMixedUndo(hwnd, 0) == 0 && TestMixedUndo(hwnd, ES_MULTILINE | ES_AUTOVSCROLL) == 0);
    CHECK(TestWrapping(hwnd) == 0);
    hwndEdit = CreateWindowExA(0, "Edit", "abc", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                            10, 10, 180, 30, hwnd, (HMENU)(UINT_PTR)100, NULL, NULL);
    hwndMulti = CreateWindowExA(0, "Edit", "", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL,
                             10, 50, 180, 100, hwnd, (HMENU)(UINT_PTR)101, NULL, NULL);
    hwndRead = CreateWindowExA(0, "Edit", "read", WS_CHILD | WS_VISIBLE | ES_READONLY | ES_PASSWORD,
                            210, 10, 180, 30, hwnd, (HMENU)(UINT_PTR)102, NULL, NULL);
    CHECK(hwndEdit && hwndMulti && hwndRead);
    {
        HWND rghwnd[] = {hwndEdit, hwndMulti, hwndRead};
        HCURSOR hcurArrow = LoadCursorA(NULL, MAKEINTRESOURCE(32512));
        HCURSOR hcurText = LoadCursorA(NULL, MAKEINTRESOURCE(32513));
        UINT i;
        CHECK(hcurArrow && hcurText);
        for (i = 0; i < sizeof(rghwnd) / sizeof(rghwnd[0]); i++) {
            SetCursor(hcurArrow);
            SendMessageA(rghwnd[i], WM_SETCURSOR, (WPARAM)rghwnd[i], MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
            CHECK(GetCursor() == hcurText);
            SendMessageA(rghwnd[i], WM_SETCURSOR, (WPARAM)rghwnd[i], MAKELPARAM(HTBORDER, WM_MOUSEMOVE));
            CHECK(GetCursor() == hcurArrow);
        }
        {
            const char *rgpszClass[] = {"Button", "Button", "ComboBox"};
            DWORD rgstyle[] = {BS_AUTOCHECKBOX, BS_AUTORADIOBUTTON, CBS_DROPDOWNLIST};
            HWND hwndControl;
            for (i = 0; i < sizeof(rgstyle) / sizeof(rgstyle[0]); i++) {
                hwndControl = CreateWindowExA(0, rgpszClass[i], "", WS_CHILD | WS_VISIBLE | rgstyle[i],
                                           0, 200, 100, 25, hwnd, NULL, NULL, NULL);
                CHECK(hwndControl);
                SendMessageA(hwndEdit, WM_SETCURSOR, (WPARAM)hwndEdit, MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
                CHECK(GetCursor() == hcurText);
                SendMessageA(hwndControl, WM_SETCURSOR, (WPARAM)hwndControl, MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
                CHECK(GetCursor() == hcurArrow);
                CHECK(DestroyWindow(hwndControl));
            }
        }
    }
    CHECK(TestEditing(hwndEdit, hwndMulti) == 0);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(TestSelectionArrows(hwndEdit, "A\xc3\xa9" "BC") == 0);
#endif
    CHECK(TestSelectionArrows(hwndMulti, "a\r\nbc") == 0);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(TestWordNavigation(hwndEdit, hwndMulti) == 0);
#endif
#ifdef __APPLE__
    CHECK(TestMacNavigation(hwndEdit, hwndMulti) == 0);
#endif
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(SetWindowTextA(hwndEdit, ""));
    SendMessageA(hwndEdit, WM_CHAR, 'a', 1); SendMessageA(hwndEdit, WM_CHAR, 'b', 1); SendMessageA(hwndEdit, WM_CHAR, 'c', 1);
    CHECK(SendMessageA(hwndEdit, EM_UNDO, 0, 0) && FText(hwndEdit, ""));
    CHECK(SendMessageA(hwndEdit, EM_UNDO, 0, 0) && FText(hwndEdit, "abc"));
    {
        HWND hwndCase = CreateWindowExA(0, "Edit", "\xc3\xa9\xc3\xbf", WS_CHILD | ES_UPPERCASE, 0, 0, 100, 25, hwnd, NULL, NULL, NULL);
        CHECK(hwndCase && FText(hwndCase, "\xc3\xa9\xc3\xbf"));
        CHECK(SetWindowTextA(hwndCase, "a\xc3\xa9\xc3\xbf") && FText(hwndCase, "A\xc3\xa9\xc3\xbf"));
        CHECK(SetWindowTextA(hwndCase, ""));
        SendMessageA(hwndCase, WM_CHAR, 0xe9, 1); SendMessageA(hwndCase, WM_CHAR, 0xff, 1);
        CHECK(FText(hwndCase, "\xc3\xa9\xc3\xbf")); CHECK(DestroyWindow(hwndCase));
    }
    SetFocus(hwndEdit); CHECK(hwndEdit->fEditCaret && hwndEdit->idEditCaretTimer);
    {
        SDL_Event event = {0};
        event.type = SDL_EVENT_TEXT_EDITING; event.edit.windowID = HwndCompatRoot(hwndEdit)->id;
        event.edit.text = "\xc3\xa9"; event.edit.start = 0; event.edit.length = 1;
        c = cChange; CompatRouteEvent(&event);
        CHECK(FText(hwndEdit, "abc") && cChange == c && !strcmp(hwndEdit->pszEditComposition, "\xc3\xa9"));
        CHECK(UpdateWindow(hwndEdit));
        event.edit.text = ""; CompatRouteEvent(&event);
        CHECK(FText(hwndEdit, "abc") && cChange == c && !*hwndEdit->pszEditComposition);
    }
    SendMessageA(hwndEdit, WM_TIMER, hwndEdit->idEditCaretTimer, 0); CHECK(!hwndEdit->fEditCaret);
    SendMessageA(hwndEdit, WM_TIMER, hwndEdit->idEditCaretTimer, 0); CHECK(hwndEdit->fEditCaret);
    CHECK(PostMessageA(hwndEdit, WM_KEYDOWN, VK_ESCAPE, 0));
    SendMessageA(hwndEdit, WM_CONTEXTMENU, (WPARAM)hwndEdit, -1);
    CHECK(FText(hwndEdit, "abc") && !GetCapture());
    SetFocus(hwndMulti); CHECK(!hwndEdit->idEditCaretTimer);
#endif
    CHECK(SetWindowTextA(hwndEdit, "abc"));
    CHECK(SendMessageA(hwndEdit, WM_GETDLGCODE, 0, 0) & DLGC_HASSETSEL);
    SendMessageA(hwndEdit, EM_SETSEL, 3, 1);
    CHECK(SendMessageA(hwndEdit, EM_GETSEL, (WPARAM)&ichFirst, (LPARAM)&ichLast) == MAKELONG(1, 3));
    CHECK(ichFirst == 1 && ichLast == 3);
    SendMessageA(hwndEdit, EM_REPLACESEL, 0, (LPARAM)"XYZ");
    CHECK(FText(hwndEdit, "aXYZ") && SendMessageA(hwndEdit, EM_GETMODIFY, 0, 0));
    CHECK(SendMessageA(hwndEdit, EM_GETSEL, 0, 0) == MAKELONG(4, 4));
    SendMessageA(hwndEdit, EM_SETMODIFY, 0, 0);
    CHECK(!SendMessageA(hwndEdit, EM_GETMODIFY, 0, 0));
    SendMessageA(hwndEdit, EM_LIMITTEXT, 5, 0);
    CHECK(SendMessageA(hwndEdit, EM_GETLIMITTEXT, 0, 0) == 5);
    c = cMax; SendMessageA(hwndEdit, WM_CHAR, '1', 1); SendMessageA(hwndEdit, WM_CHAR, '2', 1);
    CHECK(FText(hwndEdit, "aXYZ1") && cMax > c);
    CHECK(SetWindowTextA(hwndEdit, "longer than limit") && FText(hwndEdit, "longer than limit"));
    CHECK(!SendMessageA(hwndEdit, EM_GETMODIFY, 0, 0));
    SendMessageA(hwndEdit, EM_SETSEL, 0, -1);
    SendMessageA(hwndEdit, WM_CHAR, 'Q', 1);
    CHECK(FText(hwndEdit, "Q"));
    SendMessageA(hwndEdit, WM_CHAR, 'R', 1);
    SendMessageA(hwndEdit, WM_CHAR, '\b', 1);
    CHECK(FText(hwndEdit, "Q"));
    SendMessageA(hwndEdit, EM_SETSEL, 0, 0); SendMessageA(hwndEdit, WM_KEYDOWN, VK_DELETE, 1);
    CHECK(FText(hwndEdit, ""));
    SendMessageA(hwndRead, WM_CHAR, 'X', 1);
    CHECK(FText(hwndRead, "read"));
    CHECK(SendMessageA(hwndRead, EM_SETREADONLY, 0, 0));
    SendMessageA(hwndRead, EM_SETSEL, 0, -1); SendMessageA(hwndRead, WM_CHAR, 'X', 1);
    CHECK(FText(hwndRead, "X"));
    c = cChange; CHECK(SetWindowTextA(hwndMulti, "line1\r\nline2") && cChange == c);
    SendMessageA(hwndMulti, EM_SETSEL, 0, -1); SendMessageA(hwndMulti, WM_CHAR, 'a', 1);
    SendMessageA(hwndMulti, WM_CHAR, '\r', 1); SendMessageA(hwndMulti, WM_CHAR, 'b', 1);
    CHECK(FText(hwndMulti, "a\r\nb"));
    SendMessageA(hwndMulti, EM_SETSEL, 1, 1); SendMessageA(hwndMulti, WM_KEYDOWN, VK_RIGHT, 1);
    CHECK(SendMessageA(hwndMulti, EM_GETSEL, 0, 0) == MAKELONG(3, 3));
    SendMessageA(hwndMulti, WM_CHAR, '\b', 1); CHECK(FText(hwndMulti, "ab"));
    SendMessageA(hwndEdit, EM_LIMITTEXT, 30, 0); CHECK(SetWindowTextA(hwndEdit, "abcd"));
    SendMessageA(hwndEdit, EM_SETSEL, 1, 3); SendMessageA(hwndEdit, WM_KEYDOWN, VK_LEFT, 1);
    CHECK(SendMessageA(hwndEdit, EM_GETSEL, 0, 0) == MAKELONG(1, 1));
    SetFocus(hwndEdit); CHECK(UpdateWindow(hwndEdit) && UpdateWindow(hwndMulti) && UpdateWindow(hwndRead));
    CHECK(cChange != 0 && cUpdate != 0);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    {
        SDL_Event event = {0};
        APPMSG msg;
        HWND hwndPass = CreateWindowExA(0, "Edit", "Y", WS_CHILD | WS_VISIBLE | ES_PASSWORD,
                                    210, 50, 180, 30, hwnd, NULL, NULL, NULL);
        CHECK(hwndPass && UpdateWindow(hwndPass));
        CHECK(memcmp(hwndPass->lpCanvas->pixels, hwndRead->lpCanvas->pixels,
                     (size_t)hwndRead->lpCanvas->pitch * hwndRead->lpCanvas->h) == 0);
        CHECK(DestroyWindow(hwndPass));
        SendMessageA(hwndEdit, EM_SETSEL, 0, -1);
        event.type = SDL_EVENT_TEXT_INPUT; event.text.windowID = HwndCompatRoot(hwndEdit)->id;
        event.text.text = "A\xe2\x82\xac"; CompatRouteEvent(&event);
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageA(&msg); }
        CHECK(FText(hwndEdit, "A\xe2\x82\xac"));
    }
#endif
    hwndKill = CreateWindowExA(0, "Edit", "", WS_CHILD | ES_AUTOHSCROLL, 0, 0, 100, 25, hwnd, (HMENU)(UINT_PTR)103, NULL, NULL);
    CHECK(hwndKill); fDestroy = 1;
    SetWindowTextA(hwndKill, "destroy during update");
    CHECK(!IsWindow(hwndKill));
    CHECK(DestroyWindow(hwnd));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    StarsCompatShutdown();
#endif
    puts("Win32 edit controls passed");
    return 0;
}
