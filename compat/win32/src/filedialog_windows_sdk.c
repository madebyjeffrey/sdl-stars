/* NATIVE: Keep system Unicode dialogs separate from the wrapper API namespace. */
#include <windows.h>
#include <commdlg.h>
#include <stdlib.h>
#include <string.h>
#include "filedialog_windows_bridge.h"
#include "../../../src/platform/errors.h"
#include "../../../src/platform/test_impl.h"

typedef struct WINDOWSCHOOSER {
    int (*pfnPump)(void *);
    void *lpv;
    int fStopped;
} WINDOWSCHOOSER;

static wchar_t *LpwszChooserText(const char *psz, UINT cp) {
    wchar_t *pwsz;
    int cch;

    cch = MultiByteToWideChar(cp, cp == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, psz, -1, NULL, 0);
    if (cch == 0) return NULL;
    pwsz = (wchar_t *)malloc((size_t)cch * sizeof(*pwsz));
    if (pwsz) MultiByteToWideChar(cp, cp == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, psz, -1, pwsz, cch);
    return pwsz;
}

static UINT_PTR CALLBACK ChooserHook(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    WINDOWSCHOOSER *lp;

    try {
    if (msg == WM_INITDIALOG) {
        lp = (WINDOWSCHOOSER *)((OPENFILENAMEW *)l)->lCustData;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)lp);
        SetTimer(hwnd, 1, 20, NULL);
    } else if (msg == WM_TIMER && w == 1) {
        lp = (WINDOWSCHOOSER *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (lp && !lp->pfnPump(lp->lpv)) {
            lp->fStopped = 1;
            PostMessageW(GetParent(hwnd), WM_COMMAND, IDCANCEL, 0);
        }
    } else if (msg == WM_DESTROY) KillTimer(hwnd, 1);
    } catch (...) {
        // Retain failure delivery until the native modal window has cleaned up.
        StarsCapturePendingError();
        PostMessageW(GetParent(hwnd), WM_COMMAND, IDCANCEL, 0);
    }
    return 0;
}

int STARS_TEST_IMPL(IChooseWindowsFile)(void *lpWindow, int fSave, const char *pszTitle, const char *pszFolder,
                       const char *pszName, const WINDOWSFILEFILTER *rgfilter, int cFilter,
                       int *lpiFilter, char *pszPath, size_t cbPath,
                       int (*pfnPump)(void *), void *lpv) {
    OPENFILENAMEW of = {};
    WINDOWSCHOOSER chooser = {pfnPump, lpv, 0};
    wchar_t *pwszTitle = NULL, *pwszFolder = NULL, *pwszName = NULL, *pwszFilter = NULL;
    wchar_t *pwszFile = NULL, *pwszCwd = NULL, *pwszLabel, *pwszPattern;
    size_t cchFilter = 1, cchLabel, cchPattern;
    DWORD cchCwd, dwError;
    int i, cb, iResult = -1;
    BOOL fSelected;

    pwszTitle = LpwszChooserText(pszTitle, CP_UTF8);
    pwszFolder = LpwszChooserText(pszFolder, CP_UTF8);
    pwszName = LpwszChooserText(pszName, CP_UTF8);
    pwszFile = (wchar_t *)calloc(32768, sizeof(*pwszFile));
    cchCwd = GetCurrentDirectoryW(0, NULL);
    pwszCwd = cchCwd != 0 ? (wchar_t *)malloc((size_t)cchCwd * sizeof(*pwszCwd)) : NULL;
    if (!pwszTitle || !pwszFolder || !pwszName || !pwszFile || !pwszCwd ||
        !GetCurrentDirectoryW(cchCwd, pwszCwd) || wcslen(pwszName) >= 32768) goto done;
    wcscpy(pwszFile, pwszName);
    for (i = 0; i < cFilter; i++) {
        pwszLabel = LpwszChooserText(rgfilter[i].pszName, CP_UTF8);
        pwszPattern = LpwszChooserText(rgfilter[i].pszPattern, CP_UTF8);
        if (!pwszLabel || !pwszPattern) { free(pwszLabel); free(pwszPattern); goto done; }
        cchLabel = wcslen(pwszLabel) + 1; cchPattern = wcslen(pwszPattern) + 1;
        {
            wchar_t *pwszNew = (wchar_t *)realloc(pwszFilter, (cchFilter + cchLabel + cchPattern) * sizeof(*pwszFilter));
            if (!pwszNew) { free(pwszLabel); free(pwszPattern); goto done; }
            pwszFilter = pwszNew;
        }
        memcpy(pwszFilter + cchFilter - 1, pwszLabel, cchLabel * sizeof(*pwszFilter));
        memcpy(pwszFilter + cchFilter - 1 + cchLabel, pwszPattern, cchPattern * sizeof(*pwszFilter));
        cchFilter += cchLabel + cchPattern;
        pwszFilter[cchFilter - 1] = 0;
        free(pwszLabel); free(pwszPattern);
    }
    of.lStructSize = sizeof(of); of.hwndOwner = (HWND)lpWindow;
    of.lpstrTitle = pwszTitle; of.lpstrInitialDir = pwszFolder;
    of.lpstrFilter = pwszFilter; of.nFilterIndex = (DWORD)*lpiFilter + 1;
    of.lpstrFile = pwszFile; of.nMaxFile = 32768;
    /* Stars validates extensions, existence and overwrite after selection. */
    of.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_NOTESTFILECREATE;
    if (pfnPump) {
        of.Flags |= OFN_ENABLEHOOK;
        of.lpfnHook = ChooserHook; of.lCustData = (LPARAM)&chooser;
    }
    fSelected = fSave ? GetSaveFileNameW(&of) : GetOpenFileNameW(&of);
    dwError = fSelected ? 0 : CommDlgExtendedError();
    /* Open dialogs can change CWD despite OFN_NOCHANGEDIR. */
    if (!SetCurrentDirectoryW(pwszCwd)) goto done;
    if (chooser.fStopped || !fSelected) { iResult = dwError == 0 ? 0 : -1; goto done; }
    cb = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pwszFile, -1, NULL, 0, NULL, NULL);
    if (cb == 0 || (size_t)cb > cbPath) goto done;
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pwszFile, -1, pszPath, cb, NULL, NULL)) goto done;
    for (i = 0; pszPath[i]; i++) if (pszPath[i] == '\\') pszPath[i] = '/';
    if (of.nFilterIndex > 0 && of.nFilterIndex <= (DWORD)cFilter) *lpiFilter = (int)of.nFilterIndex - 1;
    iResult = 1;
done:
    free(pwszTitle); free(pwszFolder); free(pwszName); free(pwszFilter); free(pwszFile); free(pwszCwd);
    return iResult;
}
