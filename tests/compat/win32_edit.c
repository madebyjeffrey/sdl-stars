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
                rgUndoSel[cUndoNotice] = (DWORD)SendMessage((HWND)l, EM_GETSEL, 0, 0);
                rgUndoModified[cUndoNotice] = (BOOL)SendMessage((HWND)l, EM_GETMODIFY, 0, 0);
            }
            cUndoNotice++;
        }
        if (HIWORD(w) == EN_CHANGE) cChange++;
        if (HIWORD(w) == EN_MAXTEXT) cMax++;
        if (HIWORD(w) == EN_UPDATE) {
            cUpdate++;
            if (fDestroy) { fDestroy = FALSE; DestroyWindow((HWND)l); }
        }
    }
    return DefWindowProc(hwnd, msg, w, l);
}
static BOOL FText(HWND hwnd, LPCSTR psz) {
    char sz[128];
    int cch = GetWindowText(hwnd, sz, sizeof(sz));
    if (cch == (int)strlen(psz) && strcmp(sz, psz) == 0) return TRUE;
    fprintf(stderr, "edit text: expected [%s], got [%s]\n", psz, sz);
    return FALSE;
}
static int TestCaseStyles(HWND hwndParent, DWORD style) {
    HWND hwnd = CreateWindow("Edit", "Abc dEF", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | style,
                             0, 210, 180, style & ES_MULTILINE ? 80 : 25, hwndParent, (HMENU)(UINT_PTR)111, NULL, NULL);
    LPCSTR pszCase = style & ES_UPPERCASE ? "ABC DEF" : "abc def";
    CHECK(hwnd);
    CHECK(FText(hwnd, pszCase));
    SendMessage(hwnd, EM_LIMITTEXT, 3, 0);
    cChange = cUpdate = 0;
    CHECK(SetWindowText(hwnd, "Abc dEF") && FText(hwnd, pszCase));
    CHECK(!SendMessage(hwnd, EM_GETMODIFY, 0, 0) && !SendMessage(hwnd, EM_CANUNDO, 0, 0));
    CHECK(cUpdate == (style & ES_MULTILINE ? 0u : 1u) && cChange == cUpdate);
    SendMessage(hwnd, EM_LIMITTEXT, 100, 0); SendMessage(hwnd, EM_SETSEL, 0, -1);
    SendMessage(hwnd, EM_REPLACESEL, TRUE, (LPARAM)"GhI");
    CHECK(FText(hwnd, style & ES_UPPERCASE ? "GHI" : "ghi"));
    CHECK(SendMessage(hwnd, EM_UNDO, 0, 0) && FText(hwnd, pszCase));
    CHECK(SendMessage(hwnd, EM_SETREADONLY, TRUE, 0));
    CHECK(SetWindowText(hwnd, "Abc dEF") && FText(hwnd, pszCase));
    SendMessage(hwnd, EM_SETSEL, 0, -1); SendMessage(hwnd, EM_REPLACESEL, TRUE, (LPARAM)"GhI");
    CHECK(FText(hwnd, style & ES_UPPERCASE ? "GHI" : "ghi"));
    CHECK(DestroyWindow(hwnd)); return 0;
}
static int TestEditBoundaries(HWND hwndParent, DWORD style) {
    HWND hwnd = CreateWindow("Edit", "abc", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | style,
                             0, 210, 180, style & ES_MULTILINE ? 80 : 25, hwndParent, (HMENU)(UINT_PTR)110, NULL, NULL);
    CHECK(hwnd);
    SetFocus(hwnd);
    SendMessage(hwnd, EM_LIMITTEXT, 3, 0); SendMessage(hwnd, EM_SETSEL, 3, 3);
    SendMessage(hwnd, EM_SETMODIFY, FALSE, 0); cChange = cUpdate = cMax = 0;
    SendMessage(hwnd, WM_CHAR, 'x', 1);
    CHECK(FText(hwnd, "abc") && cUpdate == 1 && cChange == 1 && cMax == 1);
    CHECK(SendMessage(hwnd, EM_GETMODIFY, 0, 0) && !SendMessage(hwnd, EM_CANUNDO, 0, 0));
    CHECK(SetWindowText(hwnd, "abcd")); SendMessage(hwnd, EM_LIMITTEXT, 100, 0); SendMessage(hwnd, EM_SETSEL, 4, 4);
    SendMessage(hwnd, WM_CHAR, '\b', 1); SendMessage(hwnd, WM_CHAR, '\b', 1); SendMessage(hwnd, EM_UNDO, 0, 0);
    CHECK(FText(hwnd, "abcd") && SendMessage(hwnd, EM_GETSEL, 0, 0) == MAKELONG(2, 4));
    CHECK(SendMessage(hwnd, EM_UNDO, 0, 0) && FText(hwnd, "ab"));
    CHECK(SetWindowText(hwnd, "abcd")); SendMessage(hwnd, EM_SETSEL, 1, 1);
    SendMessage(hwnd, WM_KEYDOWN, VK_DELETE, 1); SendMessage(hwnd, WM_KEYDOWN, VK_DELETE, 1);
    SendMessage(hwnd, EM_UNDO, 0, 0);
    CHECK(FText(hwnd, "abcd") && SendMessage(hwnd, EM_GETSEL, 0, 0) == MAKELONG(1, 3));
    CHECK(SetWindowText(hwnd, "ab")); SendMessage(hwnd, EM_LIMITTEXT, 3, 0); SendMessage(hwnd, EM_SETSEL, 2, 2);
    SendMessage(hwnd, WM_CHAR, 'c', 1); SendMessage(hwnd, EM_SETSEL, 3, 3); SendMessage(hwnd, WM_CHAR, 'x', 1);
    CHECK(SendMessage(hwnd, EM_UNDO, 0, 0) && FText(hwnd, "ab"));
    CHECK(SetWindowText(hwnd, "abcd")); SendMessage(hwnd, EM_LIMITTEXT, 100, 0); SendMessage(hwnd, EM_SETSEL, 4, 4);
    SendMessage(hwnd, WM_CHAR, '\b', 1); SendMessage(hwnd, EM_SETSEL, 2, 2); SendMessage(hwnd, WM_CHAR, '\b', 1);
    CHECK(SendMessage(hwnd, EM_UNDO, 0, 0) && FText(hwnd, "abc"));
    if (style & ES_MULTILINE) {
        CHECK(SetWindowText(hwnd, "x\r\nab")); SendMessage(hwnd, EM_SETSEL, 5, 5);
        SendMessage(hwnd, WM_CHAR, '\b', 1); SendMessage(hwnd, WM_CHAR, '\b', 1); SendMessage(hwnd, WM_CHAR, '\b', 1);
        CHECK(FText(hwnd, "x") && SendMessage(hwnd, EM_UNDO, 0, 0) && FText(hwnd, "x\r\nab"));
        CHECK(SendMessage(hwnd, EM_GETSEL, 0, 0) == MAKELONG(1, 5));
    }
    CHECK(DestroyWindow(hwnd)); return 0;
}
static int TestMixedUndo(HWND hwndParent, DWORD style) {
    HWND hwnd = CreateWindow("Edit", "abcde", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | style,
                             0, 210, 180, style & ES_MULTILINE ? 30 : 25, hwndParent, (HMENU)(UINT_PTR)113, NULL, NULL);
    CHECK(hwnd);
    SetFocus(hwnd); SendMessage(hwnd, EM_SETSEL, 2, 2);
    SendMessage(hwnd, WM_KEYDOWN, VK_DELETE, 1);
    SendMessage(hwnd, WM_CHAR, '\b', 1);
    CHECK(FText(hwnd, "ade"));
    CHECK(SendMessage(hwnd, EM_UNDO, 0, 0) && FText(hwnd, "abcde"));
    CHECK(SetWindowText(hwnd, "abcde")); SendMessage(hwnd, EM_SETSEL, 2, 2);
    SendMessage(hwnd, WM_CHAR, '\b', 1); SendMessage(hwnd, WM_KEYDOWN, VK_DELETE, 1);
    CHECK(FText(hwnd, "ade") && SendMessage(hwnd, EM_UNDO, 0, 0) && FText(hwnd, "abcde"));
    CHECK(SetWindowText(hwnd, "abcde")); SendMessage(hwnd, EM_SETSEL, 1, 3);
    SendMessage(hwnd, EM_REPLACESEL, TRUE, (LPARAM)"XY");
    CHECK(FText(hwnd, "aXYde"));
    SendMessage(hwnd, WM_CHAR, 'z', 1);
    CHECK(FText(hwnd, "aXYzde"));
    CHECK(SendMessage(hwnd, EM_UNDO, 0, 0) && FText(hwnd, "abcde"));
    CHECK(SendMessage(hwnd, EM_UNDO, 0, 0) && FText(hwnd, "aXYzde"));
    CHECK(SetWindowText(hwnd, "abcde")); SendMessage(hwnd, EM_SETSEL, 1, 4);
    SendMessage(hwnd, EM_REPLACESEL, FALSE, (LPARAM)"X");
    CHECK(FText(hwnd, "aXe") && !SendMessage(hwnd, EM_CANUNDO, 0, 0));
    CHECK(DestroyWindow(hwnd)); return 0;
}
static int TestUndoNoops(HWND hwndParent, DWORD style) {
    HWND hwnd = CreateWindow("Edit", "abcdef", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | style,
                             0, 210, 180, style & ES_MULTILINE ? 80 : 25, hwndParent, (HMENU)(UINT_PTR)112, NULL, NULL);
    UINT rgmsg[] = {EM_UNDO, WM_UNDO};
    BYTE rgKeySav[256], rgKey[256];
    int iKey;
    size_t i;
    LRESULT result;
    CHECK(hwnd);
    for (i = 0; i < sizeof(rgmsg) / sizeof(rgmsg[0]); i++) {
        CHECK(SetWindowText(hwnd, "abcdef"));
        SendMessage(hwnd, EM_SETSEL, 2, 4); SendMessage(hwnd, EM_SETMODIFY, FALSE, 0);
        cChange = cUpdate = cMax = 0;
        for (iKey = 0; iKey < 256; iKey++)
            rgKeySav[iKey] = ((GetKeyState(iKey) & 0x8000) ? 0x80 : 0) | (GetKeyState(iKey) & 1);
        memcpy(rgKey, rgKeySav, sizeof(rgKey)); rgKey[VK_CONTROL] = 0x80;
        CHECK(SetKeyboardState(rgKey));
        SendMessage(hwnd, WM_KEYDOWN, 'Z', 1); SendMessage(hwnd, WM_CHAR, 26, 1); SendMessage(hwnd, WM_KEYUP, 'Z', 1);
        CHECK(SetKeyboardState(rgKeySav));
        CHECK(FText(hwnd, "abcdef") && SendMessage(hwnd, EM_GETSEL, 0, 0) == (i == 0 ? 0 : MAKELONG(6, 6)));
        CHECK(cUpdate == 0 && cChange == 1 && cMax == 0);
        cChange = cUpdate = cMax = 0;
        result = SendMessage(hwnd, rgmsg[i], 0, 0);
        CHECK(result && SendMessage(hwnd, EM_GETSEL, 0, 0) == (i == 0 ? 0 : MAKELONG(6, 6)));
        CHECK(!SendMessage(hwnd, EM_GETMODIFY, 0, 0) && !SendMessage(hwnd, EM_CANUNDO, 0, 0));
        CHECK(cUpdate == 0 && cChange == 1 && cMax == 0);
        CHECK(FText(hwnd, "abcdef"));
        CHECK(SendMessage(hwnd, EM_SETREADONLY, TRUE, 0)); cChange = cUpdate = cMax = 0;
        CHECK(!!SendMessage(hwnd, rgmsg[i], 0, 0) == !(style & ES_MULTILINE));
        CHECK(!SendMessage(hwnd, EM_CANUNDO, 0, 0) && !SendMessage(hwnd, EM_GETMODIFY, 0, 0));
        CHECK(cUpdate == 0 && cChange == 0 && cMax == 0);
        CHECK(SendMessage(hwnd, EM_SETREADONLY, FALSE, 0));
        SendMessage(hwnd, EM_SETSEL, 6, 6); SendMessage(hwnd, WM_CHAR, 'x', 1);
        CHECK(SendMessage(hwnd, EM_CANUNDO, 0, 0));
        SendMessage(hwnd, EM_SETSEL, 1, 2); CHECK(SendMessage(hwnd, EM_SETREADONLY, TRUE, 0));
        cChange = cUpdate = cMax = 0;
        result = SendMessage(hwnd, rgmsg[i], 0, 0);
        CHECK(!!result == !(style & ES_MULTILINE));
        CHECK(SendMessage(hwnd, EM_GETSEL, 0, 0) == MAKELONG(1, 2) && SendMessage(hwnd, EM_GETMODIFY, 0, 0));
        CHECK(cUpdate == 0 && cChange == 0 && cMax == 0);
        CHECK(FText(hwnd, "abcdefx") && SendMessage(hwnd, EM_CANUNDO, 0, 0));
        CHECK(SendMessage(hwnd, EM_SETREADONLY, FALSE, 0));
        CHECK(SendMessage(hwnd, rgmsg[i], 0, 0) && FText(hwnd, "abcdef"));
    }
    SendMessage(hwnd, EM_EMPTYUNDOBUFFER, 0, 0); SendMessage(hwnd, EM_SETSEL, 2, 4);
    cChange = cUpdate = cMax = 0;
    CHECK(SendMessage(hwnd, EM_UNDO, 0, 0) && SendMessage(hwnd, EM_GETSEL, 0, 0) == MAKELONG(6, 6));
    CHECK(cUpdate == 0 && cChange == 1 && cMax == 0 && !SendMessage(hwnd, EM_CANUNDO, 0, 0));
    CHECK(SetWindowText(hwnd, "a"));
    CHECK(SendMessage(hwnd, WM_UNDO, 0, 0) && FText(hwnd, "a"));
    CHECK(SendMessage(hwnd, EM_GETSEL, 0, 0) == MAKELONG(1, 1));
    CHECK(DestroyWindow(hwnd)); return 0;
}

static BOOL FUndoNotices(DWORD dwCaret, DWORD dwSelection) {
    return cUndoNotice == 3 && rgUndoNotice[0] == EN_UPDATE && rgUndoNotice[1] == EN_CHANGE && rgUndoNotice[2] == EN_CHANGE &&
           rgUndoSel[0] == dwCaret && rgUndoSel[1] == dwCaret && rgUndoSel[2] == dwSelection &&
           rgUndoModified[0] && rgUndoModified[1] && rgUndoModified[2];
}
static int TestUndoChanges(HWND hwndParent, DWORD style) {
    HWND hwnd = CreateWindow("Edit", "abc", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | style,
                             0, 210, 180, style & ES_MULTILINE ? 80 : 25, hwndParent, (HMENU)(UINT_PTR)113, NULL, NULL);
    UINT rgmsg[] = {EM_UNDO, WM_UNDO};
    size_t i;
    CHECK(hwnd);
    for (i = 0; i < sizeof(rgmsg) / sizeof(rgmsg[0]); i++) {
        CHECK(SetWindowText(hwnd, "abc")); SendMessage(hwnd, EM_SETSEL, 3, 3);
        SendMessage(hwnd, WM_CHAR, 'd', 1); CHECK(FText(hwnd, "abcd"));
        SendMessage(hwnd, EM_SETMODIFY, FALSE, 0); cChange = cUpdate = cMax = cUndoNotice = 0; fObserveUndo = TRUE;
        CHECK(SendMessage(hwnd, rgmsg[i], 0, 0) && FText(hwnd, "abc")); fObserveUndo = FALSE;
        CHECK(SendMessage(hwnd, EM_GETMODIFY, 0, 0) && SendMessage(hwnd, EM_GETSEL, 0, 0) == MAKELONG(3, 3));
        CHECK(cUpdate == 1 && cChange == 2 && cMax == 0 && FUndoNotices(MAKELONG(3, 3), MAKELONG(3, 3)));
        SendMessage(hwnd, EM_SETMODIFY, FALSE, 0); cChange = cUpdate = cMax = cUndoNotice = 0; fObserveUndo = TRUE;
        CHECK(SendMessage(hwnd, rgmsg[i], 0, 0) && FText(hwnd, "abcd")); fObserveUndo = FALSE;
        CHECK(SendMessage(hwnd, EM_GETMODIFY, 0, 0) && SendMessage(hwnd, EM_GETSEL, 0, 0) == MAKELONG(3, 4));
        CHECK(cUpdate == 1 && cChange == 2 && cMax == 0 && FUndoNotices(MAKELONG(4, 4), MAKELONG(3, 4)));
        CHECK(SetWindowText(hwnd, "aaaa")); SendMessage(hwnd, EM_SETSEL, 1, 3);
        SendMessage(hwnd, EM_REPLACESEL, TRUE, (LPARAM)"aa");
        CHECK(SendMessage(hwnd, EM_CANUNDO, 0, 0));
        CHECK(SendMessage(hwnd, rgmsg[i], 0, 0) && FText(hwnd, "aaaa") && SendMessage(hwnd, EM_GETSEL, 0, 0) == MAKELONG(1, 3));
        CHECK(SendMessage(hwnd, rgmsg[i], 0, 0) && FText(hwnd, "aaaa") && SendMessage(hwnd, EM_GETSEL, 0, 0) == MAKELONG(1, 3));
        CHECK(SetWindowText(hwnd, "aaabbbccc")); SendMessage(hwnd, EM_SETSEL, 3, 6);
        SendMessage(hwnd, EM_REPLACESEL, TRUE, (LPARAM)""); CHECK(FText(hwnd, "aaaccc"));
        CHECK(SendMessage(hwnd, rgmsg[i], 0, 0) && FText(hwnd, "aaabbbccc") && SendMessage(hwnd, EM_GETSEL, 0, 0) == MAKELONG(3, 6));
        CHECK(SendMessage(hwnd, rgmsg[i], 0, 0) && FText(hwnd, "aaaccc") && SendMessage(hwnd, EM_GETSEL, 0, 0) == MAKELONG(3, 3));
    }
    fDestroy = TRUE;
    CHECK(SendMessage(hwnd, EM_UNDO, 0, 0) && !IsWindow(hwnd) && !fDestroy);
    return 0;
}

static int TestEditing(HWND hwndEdit, HWND hwndMulti) {
    char sz[16];
    WORD cchMax = 3;
    CHECK(SetWindowText(hwndEdit, "alpha beta"));
    SendMessage(hwndEdit, EM_LIMITTEXT, 100, 0);
    SendMessage(hwndEdit, EM_SETSEL, 0, 5);
    SendMessage(hwndEdit, WM_COPY, 0, 0);
    SendMessage(hwndEdit, EM_SETSEL, 10, 10);
    SendMessage(hwndEdit, WM_PASTE, 0, 0);
    CHECK(FText(hwndEdit, "alpha betaalpha"));
    CHECK(SendMessage(hwndEdit, EM_CANUNDO, 0, 0));
    CHECK(SendMessage(hwndEdit, EM_UNDO, 0, 0) && FText(hwndEdit, "alpha beta"));
    CHECK(SendMessage(hwndEdit, WM_UNDO, 0, 0) && FText(hwndEdit, "alpha betaalpha"));
    SendMessage(hwndEdit, EM_EMPTYUNDOBUFFER, 0, 0);
    CHECK(!SendMessage(hwndEdit, EM_CANUNDO, 0, 0));
    SendMessage(hwndEdit, EM_SETSEL, 0, 5);
    SendMessage(hwndEdit, WM_CUT, 0, 0);
    CHECK(FText(hwndEdit, " betaalpha"));
    CHECK(SendMessage(hwndEdit, EM_UNDO, 0, 0) && FText(hwndEdit, "alpha betaalpha"));
    CHECK(SetWindowText(hwndEdit, "reset") && !SendMessage(hwndEdit, EM_CANUNDO, 0, 0));
    SendMessage(hwndEdit, EM_SETSEL, 0, -1);
    SendMessage(hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)"replacement");
    CHECK(!SendMessage(hwndEdit, EM_CANUNDO, 0, 0));
    CHECK(SetWindowText(hwndMulti, "aaa\r\nbbb\r\nc"));
    CHECK(SendMessage(hwndMulti, EM_GETLINECOUNT, 0, 0) == 3);
    CHECK(SendMessage(hwndMulti, EM_LINEINDEX, 1, 0) == 5);
    CHECK(SendMessage(hwndMulti, EM_LINEINDEX, 3, 0) == -1);
    CHECK(SendMessage(hwndMulti, EM_LINEFROMCHAR, 7, 0) == 1);
    CHECK(SendMessage(hwndMulti, EM_LINELENGTH, 7, 0) == 3);
    memcpy(sz, &cchMax, sizeof(cchMax));
    CHECK(SendMessage(hwndMulti, EM_GETLINE, 1, (LPARAM)sz) == 3 && memcmp(sz, "bbb", 3) == 0);
    SendMessage(hwndMulti, EM_SETSEL, 2, 2);
    SendMessage(hwndMulti, WM_KEYDOWN, VK_DOWN, 1);
    CHECK(SendMessage(hwndMulti, EM_GETSEL, 0, 0) == MAKELONG(7, 7));
    SendMessage(hwndMulti, WM_KEYDOWN, VK_UP, 1);
    CHECK(SendMessage(hwndMulti, EM_GETSEL, 0, 0) == MAKELONG(2, 2));
    return 0;
}
static int TestWrapping(HWND hwndParent) {
    HWND hwnd = CreateWindow("Edit", "one two three four five six seven eight nine ten eleven twelve",
                             WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL,
                             0, 160, 70, 45, hwndParent, NULL, NULL, NULL);
    RECT rc = {3, 3, 60, 35}, got;
    int cLine, i, cch, ich, cchTotal = 0;
    WORD cchMax = 128;
    char sz[130], text[256] = {0};
    CHECK(hwnd);
    SendMessage(hwnd, EM_SETRECT, 0, (LPARAM)&rc);
    SendMessage(hwnd, EM_GETRECT, 0, (LPARAM)&got);
    CHECK(got.left == rc.left && got.top == rc.top && got.right == rc.right && got.bottom == rc.bottom);
    cLine = (int)SendMessage(hwnd, EM_GETLINECOUNT, 0, 0); CHECK(cLine > 3);
    for (i = 0; i < cLine; i++) {
        ich = (int)SendMessage(hwnd, EM_LINEINDEX, i, 0); CHECK(ich == cchTotal);
        memcpy(sz, &cchMax, sizeof(cchMax));
        cch = (int)SendMessage(hwnd, EM_GETLINE, i, (LPARAM)sz); CHECK(cch > 0 && cch < 128);
        memcpy(text + cchTotal, sz, (size_t)cch); cchTotal += cch;
        CHECK(SendMessage(hwnd, EM_LINEFROMCHAR, ich, 0) == i);
    }
    CHECK(FText(hwnd, text));
    CHECK(SendMessage(hwnd, EM_LINESCROLL, 0, 2));
    CHECK(SendMessage(hwnd, EM_GETFIRSTVISIBLELINE, 0, 0) == 2);
    SendMessage(hwnd, EM_SETSEL, 0, 0); SendMessage(hwnd, EM_SCROLLCARET, 0, 0); UpdateWindow(hwnd);
    CHECK(SendMessage(hwnd, EM_GETFIRSTVISIBLELINE, 0, 0) == 0);
    rc.right = 400; SendMessage(hwnd, EM_SETRECT, 0, (LPARAM)&rc);
    CHECK(SendMessage(hwnd, EM_GETLINECOUNT, 0, 0) < cLine);
    CHECK(DestroyWindow(hwnd)); return 0;
}
int main(void) {
    WNDCLASS wc = {0};
    HWND hwnd, hwndEdit, hwndMulti, hwndRead, hwndKill;
    DWORD ichFirst, ichLast;
    UINT c;
    wc.lpfnWndProc = ParentProc; wc.lpszClassName = "CompatEditParent";
    CHECK(RegisterClass(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "Edit test", WS_POPUP | WS_VISIBLE, 50, 50, 400, 300, NULL, NULL, NULL, NULL);
    CHECK(hwnd);
    CHECK(TestCaseStyles(hwnd, ES_UPPERCASE) == 0 && TestCaseStyles(hwnd, ES_LOWERCASE) == 0);
    CHECK(TestCaseStyles(hwnd, ES_UPPERCASE | ES_MULTILINE) == 0 && TestCaseStyles(hwnd, ES_LOWERCASE | ES_MULTILINE) == 0);
    CHECK(TestUndoChanges(hwnd, 0) == 0 && TestUndoChanges(hwnd, ES_MULTILINE) == 0);
    CHECK(TestUndoNoops(hwnd, 0) == 0 && TestUndoNoops(hwnd, ES_MULTILINE) == 0);
    CHECK(TestEditBoundaries(hwnd, 0) == 0 && TestEditBoundaries(hwnd, ES_MULTILINE) == 0);
    CHECK(TestMixedUndo(hwnd, 0) == 0 && TestMixedUndo(hwnd, ES_MULTILINE | ES_AUTOVSCROLL) == 0);
    CHECK(TestWrapping(hwnd) == 0);
    hwndEdit = CreateWindow("Edit", "abc", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                            10, 10, 180, 30, hwnd, (HMENU)(UINT_PTR)100, NULL, NULL);
    hwndMulti = CreateWindow("Edit", "", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL,
                             10, 50, 180, 100, hwnd, (HMENU)(UINT_PTR)101, NULL, NULL);
    hwndRead = CreateWindow("Edit", "read", WS_CHILD | WS_VISIBLE | ES_READONLY | ES_PASSWORD,
                            210, 10, 180, 30, hwnd, (HMENU)(UINT_PTR)102, NULL, NULL);
    CHECK(hwndEdit && hwndMulti && hwndRead);
    CHECK(TestEditing(hwndEdit, hwndMulti) == 0);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(SetWindowText(hwndEdit, ""));
    SendMessage(hwndEdit, WM_CHAR, 'a', 1); SendMessage(hwndEdit, WM_CHAR, 'b', 1); SendMessage(hwndEdit, WM_CHAR, 'c', 1);
    CHECK(SendMessage(hwndEdit, EM_UNDO, 0, 0) && FText(hwndEdit, ""));
    CHECK(SendMessage(hwndEdit, EM_UNDO, 0, 0) && FText(hwndEdit, "abc"));
    {
        HWND hwndCase = CreateWindow("Edit", "\xc3\xa9\xc3\xbf", WS_CHILD | ES_UPPERCASE, 0, 0, 100, 25, hwnd, NULL, NULL, NULL);
        CHECK(hwndCase && FText(hwndCase, "\xc3\xa9\xc3\xbf"));
        CHECK(SetWindowText(hwndCase, "a\xc3\xa9\xc3\xbf") && FText(hwndCase, "A\xc3\xa9\xc3\xbf"));
        CHECK(SetWindowText(hwndCase, ""));
        SendMessage(hwndCase, WM_CHAR, 0xe9, 1); SendMessage(hwndCase, WM_CHAR, 0xff, 1);
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
    SendMessage(hwndEdit, WM_TIMER, hwndEdit->idEditCaretTimer, 0); CHECK(!hwndEdit->fEditCaret);
    SendMessage(hwndEdit, WM_TIMER, hwndEdit->idEditCaretTimer, 0); CHECK(hwndEdit->fEditCaret);
    CHECK(PostMessage(hwndEdit, WM_KEYDOWN, VK_ESCAPE, 0));
    SendMessage(hwndEdit, WM_CONTEXTMENU, (WPARAM)hwndEdit, -1);
    CHECK(FText(hwndEdit, "abc") && !GetCapture());
    SetFocus(hwndMulti); CHECK(!hwndEdit->idEditCaretTimer);
#endif
    CHECK(SetWindowText(hwndEdit, "abc"));
    CHECK(SendMessage(hwndEdit, WM_GETDLGCODE, 0, 0) & DLGC_HASSETSEL);
    SendMessage(hwndEdit, EM_SETSEL, 3, 1);
    CHECK(SendMessage(hwndEdit, EM_GETSEL, (WPARAM)&ichFirst, (LPARAM)&ichLast) == MAKELONG(1, 3));
    CHECK(ichFirst == 1 && ichLast == 3);
    SendMessage(hwndEdit, EM_REPLACESEL, FALSE, (LPARAM)"XYZ");
    CHECK(FText(hwndEdit, "aXYZ") && SendMessage(hwndEdit, EM_GETMODIFY, 0, 0));
    CHECK(SendMessage(hwndEdit, EM_GETSEL, 0, 0) == MAKELONG(4, 4));
    SendMessage(hwndEdit, EM_SETMODIFY, FALSE, 0);
    CHECK(!SendMessage(hwndEdit, EM_GETMODIFY, 0, 0));
    SendMessage(hwndEdit, EM_LIMITTEXT, 5, 0);
    CHECK(SendMessage(hwndEdit, EM_GETLIMITTEXT, 0, 0) == 5);
    c = cMax; SendMessage(hwndEdit, WM_CHAR, '1', 1); SendMessage(hwndEdit, WM_CHAR, '2', 1);
    CHECK(FText(hwndEdit, "aXYZ1") && cMax > c);
    CHECK(SetWindowText(hwndEdit, "longer than limit") && FText(hwndEdit, "longer than limit"));
    CHECK(!SendMessage(hwndEdit, EM_GETMODIFY, 0, 0));
    SendMessage(hwndEdit, EM_SETSEL, 0, -1);
    SendMessage(hwndEdit, WM_CHAR, 'Q', 1);
    CHECK(FText(hwndEdit, "Q"));
    SendMessage(hwndEdit, WM_CHAR, 'R', 1);
    SendMessage(hwndEdit, WM_CHAR, '\b', 1);
    CHECK(FText(hwndEdit, "Q"));
    SendMessage(hwndEdit, EM_SETSEL, 0, 0); SendMessage(hwndEdit, WM_KEYDOWN, VK_DELETE, 1);
    CHECK(FText(hwndEdit, ""));
    SendMessage(hwndRead, WM_CHAR, 'X', 1);
    CHECK(FText(hwndRead, "read"));
    CHECK(SendMessage(hwndRead, EM_SETREADONLY, FALSE, 0));
    SendMessage(hwndRead, EM_SETSEL, 0, -1); SendMessage(hwndRead, WM_CHAR, 'X', 1);
    CHECK(FText(hwndRead, "X"));
    c = cChange; CHECK(SetWindowText(hwndMulti, "line1\r\nline2") && cChange == c);
    SendMessage(hwndMulti, EM_SETSEL, 0, -1); SendMessage(hwndMulti, WM_CHAR, 'a', 1);
    SendMessage(hwndMulti, WM_CHAR, '\r', 1); SendMessage(hwndMulti, WM_CHAR, 'b', 1);
    CHECK(FText(hwndMulti, "a\r\nb"));
    SendMessage(hwndMulti, EM_SETSEL, 1, 1); SendMessage(hwndMulti, WM_KEYDOWN, VK_RIGHT, 1);
    CHECK(SendMessage(hwndMulti, EM_GETSEL, 0, 0) == MAKELONG(3, 3));
    SendMessage(hwndMulti, WM_CHAR, '\b', 1); CHECK(FText(hwndMulti, "ab"));
    SendMessage(hwndEdit, EM_LIMITTEXT, 30, 0); CHECK(SetWindowText(hwndEdit, "abcd"));
    SendMessage(hwndEdit, EM_SETSEL, 1, 3); SendMessage(hwndEdit, WM_KEYDOWN, VK_LEFT, 1);
    CHECK(SendMessage(hwndEdit, EM_GETSEL, 0, 0) == MAKELONG(2, 2));
    SetFocus(hwndEdit); CHECK(UpdateWindow(hwndEdit) && UpdateWindow(hwndMulti) && UpdateWindow(hwndRead));
    CHECK(cChange != 0 && cUpdate != 0);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    {
        SDL_Event event = {0};
        MSG msg;
        HWND hwndPass = CreateWindow("Edit", "Y", WS_CHILD | WS_VISIBLE | ES_PASSWORD,
                                    210, 50, 180, 30, hwnd, NULL, NULL, NULL);
        CHECK(hwndPass && UpdateWindow(hwndPass));
        CHECK(memcmp(hwndPass->lpCanvas->pixels, hwndRead->lpCanvas->pixels,
                     (size_t)hwndRead->lpCanvas->pitch * hwndRead->lpCanvas->h) == 0);
        CHECK(DestroyWindow(hwndPass));
        SendMessage(hwndEdit, EM_SETSEL, 0, -1);
        event.type = SDL_EVENT_TEXT_INPUT; event.text.windowID = HwndCompatRoot(hwndEdit)->id;
        event.text.text = "A\xe2\x82\xac"; CompatRouteEvent(&event);
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); }
        CHECK(FText(hwndEdit, "A\xe2\x82\xac"));
    }
#endif
    hwndKill = CreateWindow("Edit", "", WS_CHILD | ES_AUTOHSCROLL, 0, 0, 100, 25, hwnd, (HMENU)(UINT_PTR)103, NULL, NULL);
    CHECK(hwndKill); fDestroy = TRUE;
    SetWindowText(hwndKill, "destroy during update");
    CHECK(!IsWindow(hwndKill));
    CHECK(DestroyWindow(hwnd));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    StarsCompatShutdown();
#endif
    puts("Win32 edit controls passed");
    return 0;
}
