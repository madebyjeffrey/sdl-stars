#include "windows_chooser_io.h"

static thread_local const WINDOWSCHOOSERIO *lpIO;
const WINDOWSCHOOSERIO &WindowsChooserIO() noexcept {
    static const WINDOWSCHOOSERIO io;
    return lpIO ? *lpIO : io;
}
WINDOWSCHOOSERIOSCOPE::WINDOWSCHOOSERIOSCOPE(const WINDOWSCHOOSERIO &io) noexcept : io(io), lpPrev(lpIO) { lpIO = &this->io; }
WINDOWSCHOOSERIOSCOPE::~WINDOWSCHOOSERIOSCOPE() noexcept { lpIO = lpPrev; }

/* NATIVE: Keep system Unicode dialogs separate from the wrapper API namespace. */
#include <algorithm>
#include <span>
#include <windows.h>
#include <commdlg.h>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include "filedialog_windows_bridge.h"
#include "../../../src/platform/errors.h"
#include "../../../src/platform/ownership.h"

struct WINDOWSCHOOSER {
    int (*pfnPump)(void *);
    void *lpv;
    int fStopped;
};

static wchar_t *LpwszChooserText(const char *psz, UINT cp) {
    wchar_t *pwsz;
    int cch;

    cch = MultiByteToWideChar(cp, cp == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, psz, -1, nullptr, 0);
    if (cch == 0) return nullptr;
    pwsz = static_cast<wchar_t *>(std::malloc(static_cast<std::size_t>(cch) * sizeof(*pwsz)));
    if (pwsz) MultiByteToWideChar(cp, cp == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, psz, -1, pwsz, cch);
    return pwsz;
}

static UINT_PTR CALLBACK ChooserHook(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    WINDOWSCHOOSER *lp;

    try {
    if (msg == WM_INITDIALOG) {
        lp = reinterpret_cast<WINDOWSCHOOSER *>(reinterpret_cast<OPENFILENAMEW *>(l)->lCustData);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(lp));
        SetTimer(hwnd, 1, 20, nullptr);
    } else if (msg == WM_TIMER && w == 1) {
        lp = reinterpret_cast<WINDOWSCHOOSER *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
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

int IChooseWindowsFile(void *lpWindow, int fSave, const char *pszTitle, const char *pszFolder,
                       const char *pszName, const WINDOWSFILEFILTER *rgfilter, int cFilter,
                       int *lpiFilter, char *pszPath, std::size_t cbPath,
                       int (*pfnPump)(void *), void *lpv) {
    OPENFILENAMEW of = {};
    WINDOWSCHOOSER chooser = {pfnPump, lpv, 0};
    wchar_t *pwszTitle = nullptr, *pwszFolder = nullptr, *pwszName = nullptr, *pwszFilter = nullptr;
    wchar_t *pwszFile = nullptr, *pwszCwd = nullptr, *pwszLabel, *pwszPattern;
    std::size_t cchFilter = 1, cchLabel, cchPattern;
    DWORD cchCwd, dwError;
    int i, cb, iResult = -1;
    BOOL fSelected;
    NATIVECLEANUPSCOPE chooserText([&]() noexcept {
        std::free(pwszTitle); std::free(pwszFolder); std::free(pwszName);
        std::free(pwszFilter); std::free(pwszFile); std::free(pwszCwd);
    });

    pwszTitle = LpwszChooserText(pszTitle, CP_UTF8);
    pwszFolder = LpwszChooserText(pszFolder, CP_UTF8);
    pwszName = LpwszChooserText(pszName, CP_UTF8);
    pwszFile = static_cast<wchar_t *>(std::calloc(32768, sizeof(*pwszFile)));
    cchCwd = GetCurrentDirectoryW(0, nullptr);
    pwszCwd = cchCwd != 0 ? static_cast<wchar_t *>(std::malloc(static_cast<std::size_t>(cchCwd) * sizeof(*pwszCwd))) : nullptr;
    if (!pwszTitle || !pwszFolder || !pwszName || !pwszFile || !pwszCwd ||
        !GetCurrentDirectoryW(cchCwd, pwszCwd) || std::wcslen(pwszName) >= 32768) goto done;
    {
        const auto rgchFile = std::span{pwszFile, 32768}.first(std::wcslen(pwszName) + 1);
        std::copy_n(pwszName, rgchFile.size(), rgchFile.begin());
    }
    for (i = 0; i < cFilter; i++) {
        pwszLabel = LpwszChooserText(rgfilter[i].pszName, CP_UTF8);
        pwszPattern = LpwszChooserText(rgfilter[i].pszPattern, CP_UTF8);
        NATIVEOWNER<wchar_t, std::free> labelOwner(pwszLabel), patternOwner(pwszPattern);
        if (!pwszLabel || !pwszPattern) goto done;
        cchLabel = std::wcslen(pwszLabel) + 1; cchPattern = std::wcslen(pwszPattern) + 1;
        {
            wchar_t *pwszNew = static_cast<wchar_t *>(std::realloc(pwszFilter, (cchFilter + cchLabel + cchPattern) * sizeof(*pwszFilter)));
            if (!pwszNew) goto done;
            pwszFilter = pwszNew;
        }
        std::memcpy(pwszFilter + cchFilter - 1, pwszLabel, cchLabel * sizeof(*pwszFilter));
        std::memcpy(pwszFilter + cchFilter - 1 + cchLabel, pwszPattern, cchPattern * sizeof(*pwszFilter));
        cchFilter += cchLabel + cchPattern;
        pwszFilter[cchFilter - 1] = 0;
    }
    of.lStructSize = sizeof(of); of.hwndOwner = static_cast<HWND>(lpWindow);
    of.lpstrTitle = pwszTitle; of.lpstrInitialDir = pwszFolder;
    of.lpstrFilter = pwszFilter; of.nFilterIndex = static_cast<DWORD>(*lpiFilter) + 1;
    of.lpstrFile = pwszFile; of.nMaxFile = 32768;
    /* Stars validates extensions, existence and overwrite after selection. */
    of.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_NOTESTFILECREATE;
    if (pfnPump) {
        of.Flags |= OFN_ENABLEHOOK;
        of.lpfnHook = ChooserHook; of.lCustData = reinterpret_cast<LPARAM>(&chooser);
    }
    fSelected = fSave ? GetSaveFileNameW(&of) : GetOpenFileNameW(&of);
    dwError = fSelected ? 0 : CommDlgExtendedError();
    /* Open dialogs can change CWD despite OFN_NOCHANGEDIR. */
    if (!SetCurrentDirectoryW(pwszCwd)) goto done;
    if (chooser.fStopped || !fSelected) { iResult = dwError == 0 ? 0 : -1; goto done; }
    cb = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pwszFile, -1, nullptr, 0, nullptr, nullptr);
    if (cb == 0 || static_cast<std::size_t>(cb) > cbPath) goto done;
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pwszFile, -1, pszPath, cb, nullptr, nullptr)) goto done;
    for (i = 0; pszPath[i]; i++) if (pszPath[i] == '\\') pszPath[i] = '/';
    if (of.nFilterIndex > 0 && of.nFilterIndex <= static_cast<DWORD>(cFilter)) *lpiFilter = static_cast<int>(of.nFilterIndex) - 1;
    iResult = 1;
done:
    return iResult;
}
