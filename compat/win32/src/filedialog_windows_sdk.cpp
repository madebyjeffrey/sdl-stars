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
#include <cerrno>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>
#include <windows.h>
#include <commdlg.h>
#include "filedialog_windows_bridge.h"
#include "../../../src/platform/errors.h"

struct WINDOWSCHOOSER {
    int (*pfnPump)(void *);
    void *lpv;
    int fStopped;
};

static std::optional<std::wstring> SzChooserText(const char *psz, UINT cp) {
    int cch;

    cch = MultiByteToWideChar(cp, cp == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, psz, -1, nullptr, 0);
    if (cch == 0) return std::nullopt;
    std::wstring sz(static_cast<std::size_t>(cch), L'\0');
    if (!MultiByteToWideChar(cp, cp == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, psz, -1, sz.data(), cch)) return std::nullopt;
    sz.resize(static_cast<std::size_t>(cch - 1));
    return sz;
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
    try {
    OPENFILENAMEW of = {};
    WINDOWSCHOOSER chooser = {pfnPump, lpv, 0};
    std::optional<std::wstring> szTitle, szFolder, szName;
    std::wstring szFilter;
    std::vector<wchar_t> rgwszFile(32768), rgwszCwd;
    DWORD cchCwd, dwError;
    int i, cb, iResult = -1;
    BOOL fSelected;

    szTitle = SzChooserText(pszTitle, CP_UTF8);
    szFolder = SzChooserText(pszFolder, CP_UTF8);
    szName = SzChooserText(pszName, CP_UTF8);
    cchCwd = GetCurrentDirectoryW(0, nullptr);
    if (!szTitle || !szFolder || !szName || cchCwd == 0 || szName->size() >= rgwszFile.size()) goto done;
    rgwszCwd.resize(cchCwd);
    if (!GetCurrentDirectoryW(cchCwd, rgwszCwd.data())) goto done;
    {
        const auto rgchFile = std::span{rgwszFile}.first(szName->size() + 1);
        std::copy_n(szName->c_str(), rgchFile.size(), rgchFile.begin());
    }
    for (i = 0; i < cFilter; i++) {
        auto szLabel = SzChooserText(rgfilter[i].pszName, CP_UTF8);
        auto szPattern = SzChooserText(rgfilter[i].pszPattern, CP_UTF8);
        if (!szLabel || !szPattern) goto done;
        szFilter.append(*szLabel);
        szFilter.push_back(L'\0');
        szFilter.append(*szPattern);
        szFilter.push_back(L'\0');
    }
    if (!szFilter.empty()) szFilter.push_back(L'\0');
    of.lStructSize = sizeof(of); of.hwndOwner = static_cast<HWND>(lpWindow);
    of.lpstrTitle = szTitle->c_str(); of.lpstrInitialDir = szFolder->c_str();
    of.lpstrFilter = szFilter.empty() ? nullptr : szFilter.c_str(); of.nFilterIndex = static_cast<DWORD>(*lpiFilter) + 1;
    of.lpstrFile = rgwszFile.data(); of.nMaxFile = static_cast<DWORD>(rgwszFile.size());
    /* Stars validates extensions, existence and overwrite after selection. */
    of.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_NOTESTFILECREATE;
    if (pfnPump) {
        of.Flags |= OFN_ENABLEHOOK;
        of.lpfnHook = ChooserHook; of.lCustData = reinterpret_cast<LPARAM>(&chooser);
    }
    fSelected = fSave ? GetSaveFileNameW(&of) : GetOpenFileNameW(&of);
    dwError = fSelected ? 0 : CommDlgExtendedError();
    /* Open dialogs can change CWD despite OFN_NOCHANGEDIR. */
    if (!SetCurrentDirectoryW(rgwszCwd.data())) goto done;
    if (chooser.fStopped || !fSelected) { iResult = dwError == 0 ? 0 : -1; goto done; }
    cb = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, rgwszFile.data(), -1, nullptr, 0, nullptr, nullptr);
    if (cb == 0 || static_cast<std::size_t>(cb) > cbPath) goto done;
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, rgwszFile.data(), -1, pszPath, cb, nullptr, nullptr)) goto done;
    for (i = 0; pszPath[i]; i++) if (pszPath[i] == '\\') pszPath[i] = '/';
    if (of.nFilterIndex > 0 && of.nFilterIndex <= static_cast<DWORD>(cFilter)) *lpiFilter = static_cast<int>(of.nFilterIndex) - 1;
    iResult = 1;
done:
    return iResult;
    } catch (const std::bad_alloc &) {
        errno = ENOMEM;
        return -1;
    } catch (const std::length_error &) {
        errno = EOVERFLOW;
        return -1;
    }
}
