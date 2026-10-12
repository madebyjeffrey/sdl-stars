#include "win.h"
#ifdef STARS_UI_SDL
#include <stars_compat.h>
#endif

DWORD GetTextExtent(HDC hdc, LPCSTR str, int len) {
    SIZE size;

    if (!GetTextExtentPoint32A(hdc, str, len, &size))
        return 0;

    return MAKELONG((WORD)size.cx, (WORD)size.cy);
}

/*
 * Frame restore under Wine
 *
 * FrameWndProc handles SC_RESTORE and SC_MAXIMIZE before DefWindowProc
 * restores the window, and while a submitted turn waits it asks, in a task
 * modal MessageBox, whether to load the new turn or unsubmit. Win16 delivered
 * those commands from the message loop. Wine's macOS driver sends SC_RESTORE
 * from inside its handler for a Dock un-minimize and takes no more input
 * events until that send returns, so the MessageBox can never be answered.
 * FrameWndProcDeferred, the frame class's procedure, posts those commands
 * back to the message loop and restores the window before FrameWndProc sees
 * them, so the box shows over the restored frame and gets input.
 */

#define WM_STARS_SYSCOMMAND (WM_APP + 1)

// FrameWndProcDeferred defers FrameWndProc's restore and maximize commands
// to the message loop and passes every other message straight through.
LRESULT CALLBACK FrameWndProcDeferred(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_RESTORE || (wParam & 0xfff0) == SC_MAXIMIZE) {
            PostMessage(hwnd, WM_STARS_SYSCOMMAND, wParam, lParam);
            return 0;
        }
        break;
    case WM_STARS_SYSCOMMAND:
        if (IsIconic(hwnd))
            ShowWindow(hwnd, SW_RESTORE);
        return FrameWndProc(hwnd, WM_SYSCOMMAND, wParam, lParam);
    }
    return FrameWndProc(hwnd, msg, wParam, lParam);
}

/*
 * Mouse wheel routing
 *
 * Windows sends wheel messages to the focus window, and the scanner, planet
 * and message panes hand focus back to the frame when clicked, so the wheel
 * would reach none of them. Windows 10's "scroll inactive windows" setting
 * sends the wheel to the window under the cursor instead; Wine and older
 * Windows do not. The message loop calls FRouteMouseWheel to do the same.
 */

// FIsComboBox tells whether hwnd is a combo box.
static int16_t FIsComboBox(HWND hwnd) {
    char szClass[16];

    return hwnd && GetClassNameA(hwnd, szClass, sizeof(szClass)) && lstrcmpiA(szClass, "ComboBox") == 0;
}

// FRouteMouseWheel points a wheel message at the window under the cursor.
// It returns FALSE when the message should be dropped.
int16_t FRouteMouseWheel(MSG *pmsg) {
    POINT pt;
    HWND  hwnd;

    // The message's own cursor position, not lParam: Wine sends the point
    // of WM_MOUSEHWHEEL in client coordinates rather than screen ones.
    pt = pmsg->pt;
    hwnd = WindowFromPoint(pt);
    if (!hwnd || hwnd == pmsg->hwnd || GetWindowThreadProcessId(hwnd, NULL) != GetCurrentThreadId())
        return TRUE;
    // A closed drop-down list changes its selection on the wheel. One the
    // player only points at must not change an order or a battle plan; the
    // edit of a CBS_DROPDOWN box is a child of the box.
    if (FIsComboBox(hwnd) || FIsComboBox(GetParent(hwnd)))
        return FALSE;
    pmsg->hwnd = hwnd;
    return TRUE;
}

// CWheelNotches adds a wheel delta to *pdWheel and returns the whole notches
// (WHEEL_DELTA each) gathered, keeping the remainder. Touchpads and fine
// wheels send part notches. A change of direction starts over.
int16_t CWheelNotches(int16_t *pdWheel, int16_t dWheel) {
    int32_t d;
    int16_t c;

    d = *pdWheel;
    if ((d < 0 && dWheel > 0) || (d > 0 && dWheel < 0))
        d = 0;
    d += dWheel;
    c = (int16_t)(d / WHEEL_DELTA);
    *pdWheel = (int16_t)(d - c * WHEEL_DELTA);
    return c;
}

// CWheelLines returns how many lines a wheel notch scrolls, from the
// player's mouse settings, or cPage when they scroll a page a notch.
int16_t CWheelLines(int16_t cPage) {
    UINT cLines;

    if (cPage < 1) {
        cPage = 1;
    }
    if (!SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &cLines, 0))
        cLines = 3;
    if (cLines == WHEEL_PAGESCROLL || cLines > (UINT)cPage)
        return cPage;
    return (int16_t)cLines;
}

#if defined(_WIN32) && !defined(STARS_UI_SDL)
static DWORD dwNativeFileDialogError;

static WCHAR *LpwszDialogText(const char *psz, UINT cp, int cch) {
    WCHAR *pwsz;
    int c;
    if (!psz) return NULL;
    c = MultiByteToWideChar(cp, cp == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, psz, cch, NULL, 0);
    if (c == 0) return NULL;
    pwsz = calloc((size_t)c + 1, sizeof(*pwsz));
    if (!pwsz || !MultiByteToWideChar(cp, cp == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, psz, cch, pwsz, c)) {
        free(pwsz); return NULL;
    }
    return pwsz;
}

static int CchDialogFilter(const char *psz) {
    const char *pch = psz;
    if (!psz) return 0;
    while (*pch) pch += strlen(pch) + 1;
    return (int)(pch - psz + 1);
}

static BOOL FNativeFileDialog(OPENFILENAMEA *lpof, BOOL fSave) {
    OPENFILENAMEW ofw = {0};
    WCHAR *pwszFile = NULL, *pwszInitial = NULL, *pwszTitle = NULL;
    WCHAR *pwszFilter = NULL, *pwszExt = NULL;
    char *pszFile = NULL, *pszTitle = NULL;
    BOOL fRet = FALSE;
    int cb, cbTitle, ibFile, ibExt;
    dwNativeFileDialogError = 0;
    if (!lpof || lpof->lStructSize != sizeof(*lpof)) {
        dwNativeFileDialogError = CDERR_STRUCTSIZE; return FALSE;
    }
    if (!lpof->lpstrFile || lpof->nMaxFile == 0 || lpof->nMaxFile > INT_MAX ||
        !memchr(lpof->lpstrFile, 0, lpof->nMaxFile) ||
        (lpof->Flags & (OFN_ALLOWMULTISELECT | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE | OFN_ENABLETEMPLATEHANDLE)) ||
        lpof->lpstrCustomFilter) {
        dwNativeFileDialogError = FNERR_INVALIDFILENAME; return FALSE;
    }
    pwszFile = calloc(lpof->nMaxFile, sizeof(*pwszFile));
    if (!pwszFile) { dwNativeFileDialogError = CDERR_MEMALLOCFAILURE; goto done; }
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, lpof->lpstrFile, -1, pwszFile, (int)lpof->nMaxFile)) {
        dwNativeFileDialogError = FNERR_INVALIDFILENAME; goto done;
    }
    pwszInitial = LpwszDialogText(lpof->lpstrInitialDir, CP_UTF8, -1);
    pwszTitle = LpwszDialogText(lpof->lpstrTitle, 1252, -1);
    pwszFilter = LpwszDialogText(lpof->lpstrFilter, 1252, CchDialogFilter(lpof->lpstrFilter));
    pwszExt = LpwszDialogText(lpof->lpstrDefExt, 1252, -1);
    if ((lpof->lpstrInitialDir && !pwszInitial) || (lpof->lpstrTitle && !pwszTitle) ||
        (lpof->lpstrFilter && !pwszFilter) || (lpof->lpstrDefExt && !pwszExt)) {
        dwNativeFileDialogError = FNERR_INVALIDFILENAME; goto done;
    }
    ofw.lStructSize = sizeof(ofw); ofw.hwndOwner = lpof->hwndOwner; ofw.hInstance = lpof->hInstance;
    ofw.lpstrFilter = pwszFilter; ofw.nFilterIndex = lpof->nFilterIndex;
    ofw.lpstrFile = pwszFile; ofw.nMaxFile = lpof->nMaxFile;
    ofw.lpstrInitialDir = pwszInitial; ofw.lpstrTitle = pwszTitle;
    ofw.lpstrDefExt = pwszExt; ofw.Flags = lpof->Flags; ofw.FlagsEx = lpof->FlagsEx;
    if (!(fSave ? GetSaveFileNameW(&ofw) : GetOpenFileNameW(&ofw))) {
        dwNativeFileDialogError = CommDlgExtendedError();
        if (dwNativeFileDialogError == FNERR_BUFFERTOOSMALL && lpof->nMaxFile >= 2)
            memcpy(lpof->lpstrFile, pwszFile, 2);
        goto done;
    }
    cb = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pwszFile, -1, NULL, 0, NULL, NULL);
    if (cb == 0) { dwNativeFileDialogError = FNERR_INVALIDFILENAME; goto done; }
    if ((DWORD)cb > lpof->nMaxFile) {
        WORD cbRequired = (WORD)min(cb, UINT16_MAX);
        dwNativeFileDialogError = FNERR_BUFFERTOOSMALL;
        if (lpof->nMaxFile >= 2) memcpy(lpof->lpstrFile, &cbRequired, sizeof(cbRequired));
        goto done;
    }
    pszFile = malloc(cb);
    if (!pszFile) { dwNativeFileDialogError = CDERR_MEMALLOCFAILURE; goto done; }
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pwszFile, -1, pszFile, cb, NULL, NULL)) goto invalid;
    ibFile = ofw.nFileOffset == 0 ? 0 : WideCharToMultiByte(CP_UTF8, 0, pwszFile, ofw.nFileOffset, NULL, 0, NULL, NULL);
    ibExt = ofw.nFileExtension == 0 ? 0 : WideCharToMultiByte(CP_UTF8, 0, pwszFile, ofw.nFileExtension, NULL, 0, NULL, NULL);
    if (ibFile > UINT16_MAX || ibExt > UINT16_MAX) goto invalid;
    cbTitle = cb - ibFile;
    if (lpof->lpstrFileTitle && lpof->nMaxFileTitle != 0) {
        if ((DWORD)cbTitle > lpof->nMaxFileTitle) {
            dwNativeFileDialogError = FNERR_BUFFERTOOSMALL; goto done;
        }
        pszTitle = malloc(cbTitle);
        if (!pszTitle) { dwNativeFileDialogError = CDERR_MEMALLOCFAILURE; goto done; }
        memcpy(pszTitle, pszFile + ibFile, cbTitle);
    }
    memcpy(lpof->lpstrFile, pszFile, cb);
    if (pszTitle) memcpy(lpof->lpstrFileTitle, pszTitle, cbTitle);
    lpof->nFileOffset = (WORD)ibFile; lpof->nFileExtension = (WORD)ibExt;
    lpof->nFilterIndex = ofw.nFilterIndex; lpof->Flags = ofw.Flags;
    fRet = TRUE; goto done;
invalid:
    dwNativeFileDialogError = FNERR_INVALIDFILENAME;
done:
    free(pwszFile); free(pwszInitial); free(pwszTitle); free(pwszFilter); free(pwszExt);
    free(pszFile); free(pszTitle);
    return fRet;
}
static BOOL FReportNativeFileDialog(OPENFILENAMEA *lpof, BOOL fSave) {
    BOOL fRet = FNativeFileDialog(lpof, fSave);
    if (!fRet && dwNativeFileDialogError != 0)
        AlertSz(PszGetCompressedString(idsNativeFileSelectionFailed), MB_ICONHAND);
    return fRet;
}
BOOL FNativeOpenFileName(OPENFILENAMEA *lpof) { return FReportNativeFileDialog(lpof, FALSE); }
BOOL FNativeSaveFileName(OPENFILENAMEA *lpof) { return FReportNativeFileDialog(lpof, TRUE); }
DWORD DwNativeFileDialogError(void) { return dwNativeFileDialogError; }
#endif

#if !defined(_WIN32) || defined(STARS_UI_SDL)
BOOL FNativeOpenFileName(OPENFILENAMEA *lpof) { return GetOpenFileNameA(lpof); }
BOOL FNativeSaveFileName(OPENFILENAMEA *lpof) { return GetSaveFileNameA(lpof); }
DWORD DwNativeFileDialogError(void) { return CommDlgExtendedError(); }
#endif

int CchGetLegacyWindowText(HWND hwnd, char *psz, int cchMax) {
#if defined(_WIN32) && !defined(STARS_UI_SDL)
    WCHAR *pwsz;
    BOOL fDefault = FALSE;
    int cch, cb;
    if (cchMax <= 0) return -1;
    psz[0] = 0;
    cch = GetWindowTextLengthW(hwnd);
    pwsz = calloc((size_t)cch + 1, sizeof(*pwsz));
    if (!pwsz) return -1;
    GetWindowTextW(hwnd, pwsz, cch + 1);
    cb = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, pwsz, -1, psz, cchMax, NULL, &fDefault);
    free(pwsz);
    if (cb == 0 || fDefault) { psz[0] = 0; return -1; }
    return cb - 1;
#else
    if (StarsCompatTextInputRejected(hwnd)) {
        if (cchMax > 0) psz[0] = 0;
        return -1;
    }
    return GetWindowTextA(hwnd, psz, cchMax);
#endif
}

BOOL FSetLegacyWindowText(HWND hwnd, const char *psz) {
#if defined(_WIN32) && !defined(STARS_UI_SDL)
    WCHAR *pwsz = LpwszDialogText(psz, 1252, -1);
    BOOL fResult;
    if (!pwsz) return FALSE;
    fResult = SetWindowTextW(hwnd, pwsz); free(pwsz); return fResult;
#else
    return SetWindowTextA(hwnd, psz);
#endif
}

BOOL FSetNativeWindowTitle(HWND hwnd, const char *psz) {
#if defined(_WIN32) && !defined(STARS_UI_SDL)
    WCHAR *pwsz = LpwszDialogText(psz, CP_UTF8, -1);
    BOOL fResult;
    if (!pwsz) return FALSE;
    fResult = SetWindowTextW(hwnd, pwsz); free(pwsz); return fResult;
#else
    return StarsCompatSetWindowTitleUtf8(hwnd, psz);
#endif
}
