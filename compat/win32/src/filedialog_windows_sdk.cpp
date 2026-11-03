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
#include <climits>
#include <new>
#include <limits.h>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <windows.h>
#include <commdlg.h>
#include "filedialog_windows_bridge.h"
#include "../../../src/platform/errors.h"

struct WINDOWSCHOOSER {
    win32::files::FILESELECTIONPUMP pfnPump;
    bool fStopped;
};

static std::optional<std::wstring> SzChooserText(std::string_view sz, UINT cp) {
    int cch;

    if (sz.find('\0') != std::string_view::npos || sz.size() > static_cast<std::size_t>(INT_MAX)) return std::nullopt;
    if (sz.empty()) return std::wstring{};
    cch = MultiByteToWideChar(cp, cp == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, sz.data(), static_cast<int>(sz.size()), nullptr, 0);
    if (cch == 0) return std::nullopt;
    std::wstring szWide(static_cast<std::size_t>(cch), L'\0');
    if (!MultiByteToWideChar(cp, cp == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, sz.data(), static_cast<int>(sz.size()), szWide.data(), cch)) return std::nullopt;
    return szWide;
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
        if (lp && !lp->pfnPump()) {
            lp->fStopped = true;
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

win32::files::FILESELECTIONRESULT IChooseWindowsFile(
    void *lpWindow, const win32::files::FILESELECTIONREQUEST &request,
    const win32::files::FILESELECTIONPUMP &pfnPump) {
    try {
    OPENFILENAMEW of = {};
    WINDOWSCHOOSER chooser = {pfnPump, false};
    std::optional<std::wstring> szTitle, szFolder, szName;
    std::wstring szFilter;
    std::vector<wchar_t> rgwszFile(32768), rgwszCwd;
    DWORD cchCwd, dwError;
    int i, cb;
    BOOL fSelected;
    win32::files::FILESELECTIONSELECTED selected;

    szTitle = SzChooserText(request.szTitle, CP_UTF8);
    szFolder = SzChooserText(request.szInitialDirectory, CP_UTF8);
    szName = SzChooserText(request.szInitialPath, CP_UTF8);
    cchCwd = GetCurrentDirectoryW(0, nullptr);
    if (!szTitle || !szFolder || !szName || cchCwd == 0 || szName->size() >= rgwszFile.size()) goto done;
    rgwszCwd.resize(cchCwd);
    if (!GetCurrentDirectoryW(cchCwd, rgwszCwd.data())) goto done;
    {
        const auto rgchFile = std::span{rgwszFile}.first(szName->size() + 1);
        std::copy_n(szName->c_str(), rgchFile.size(), rgchFile.begin());
    }
    for (i = 0; i < static_cast<int>(request.rgFilters.size()); i++) {
        auto szLabel = SzChooserText(request.rgFilters[static_cast<std::size_t>(i)].szName, CP_UTF8);
        auto szPattern = SzChooserText(request.rgFilters[static_cast<std::size_t>(i)].szPattern, CP_UTF8);
        if (!szLabel || !szPattern) goto done;
        szFilter.append(*szLabel);
        szFilter.push_back(L'\0');
        szFilter.append(*szPattern);
        szFilter.push_back(L'\0');
    }
    if (!szFilter.empty()) szFilter.push_back(L'\0');
    of.lStructSize = sizeof(of); of.hwndOwner = static_cast<HWND>(lpWindow);
    of.lpstrTitle = szTitle->c_str(); of.lpstrInitialDir = szFolder->c_str();
    of.lpstrFilter = szFilter.empty() ? nullptr : szFilter.c_str();
    of.nFilterIndex = static_cast<DWORD>(request.iFilter < request.rgFilters.size() ? request.iFilter : 0) + 1;
    of.lpstrFile = rgwszFile.data(); of.nMaxFile = static_cast<DWORD>(rgwszFile.size());
    /* Stars validates extensions, existence and overwrite after selection. */
    of.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_NOTESTFILECREATE;
    if (pfnPump) {
        of.Flags |= OFN_ENABLEHOOK;
        of.lpfnHook = ChooserHook; of.lCustData = reinterpret_cast<LPARAM>(&chooser);
    }
    fSelected = request.mode == win32::files::FILESELECTIONMODE::save ? GetSaveFileNameW(&of) : GetOpenFileNameW(&of);
    dwError = fSelected ? 0 : CommDlgExtendedError();
    /* Open dialogs can change CWD despite OFN_NOCHANGEDIR. */
    if (!SetCurrentDirectoryW(rgwszCwd.data())) goto done;
    if (chooser.fStopped || !fSelected) {
        if (dwError == 0) return win32::files::FILESELECTIONCANCELLED{};
        goto done;
    }
    cb = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, rgwszFile.data(), -1, nullptr, 0, nullptr, nullptr);
    if (cb == 0 || static_cast<std::size_t>(cb) > PATH_MAX) goto done;
    selected.szPath.resize(static_cast<std::size_t>(cb));
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, rgwszFile.data(), -1, selected.szPath.data(), cb, nullptr, nullptr)) goto done;
    selected.szPath.resize(static_cast<std::size_t>(cb - 1));
    std::replace(selected.szPath.begin(), selected.szPath.end(), '\\', '/');
    if (of.nFilterIndex > 0 && of.nFilterIndex <= request.rgFilters.size())
        selected.iFilter = static_cast<std::size_t>(of.nFilterIndex) - 1;
    return selected;
done:
    return win32::files::FILESELECTIONFAILURE{win32::files::FILESELECTIONERROR::backendFailure};
    } catch (const std::bad_alloc &) {
        errno = ENOMEM;
        return win32::files::FILESELECTIONFAILURE{win32::files::FILESELECTIONERROR::allocationFailure};
    } catch (const std::length_error &) {
        errno = EOVERFLOW;
        return win32::files::FILESELECTIONFAILURE{win32::files::FILESELECTIONERROR::backendFailure};
    }
}
