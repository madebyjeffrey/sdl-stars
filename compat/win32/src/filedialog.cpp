/* Stars' wildcard filters need a browser independent of SDL's extension-only API. */
#include "filedialog_backend.h"
#include "file_selection.h"
#include "choice_io.h"
#include "chooser_io.h"
#include "cleanup.h"
#include "native.h"
#include <dirent.h>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <array>
#include <string_view>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>

constexpr auto ID_FILE_NAME = 0x480;
constexpr auto ID_FILE_LIST = 0x460;
constexpr auto ID_FILE_FILTER = 0x470;
constexpr auto ID_FILE_PATH = 0x440;
constexpr auto ID_FILE_UP = 0x400;
constexpr auto ID_FILE_BACK = 0x401;
constexpr auto ID_FILE_HOME = 0x402;
constexpr auto ID_FILE_HIDDEN = 0x403;
constexpr auto ID_FILE_NEW = 0x404;
constexpr auto MAX_FILE_HISTORY = 32;
constexpr auto MAX_FILE_FILTERS = cCompatFileFiltersMax;

struct COMPATFILEENTRY { char *psz; BOOL fDir; };
struct COMPATFILEDIALOG {
    const win32::files::FILESELECTIONREQUEST *lpRequest;
    BOOL fSave;
    char szDir[PATH_MAX], szResult[PATH_MAX];
    int cFilter, iFilter;
    COMPATFILEENTRY *rgentry;
    std::size_t cEntry;
    std::array<NATIVEOWNER<char, std::free>, MAX_FILE_HISTORY> rgpszHistory;
    int cHistory;
    BOOL fHidden;
    DWORD dwError;
    std::size_t cbRequiredPath;
};
static thread_local DWORD dwDialogError;
DWORD win32::CommDlgExtendedError(void) { return dwDialogError; }
void CompatSetDialogError(DWORD dw) { dwDialogError = dw; }

static BOOL FFilePattern(LPCSTR psz, LPCSTR pszPattern, std::size_t cpat) {
    LPCSTR pszStar = nullptr, pszRetry = nullptr, pszEnd = pszPattern + cpat;
    if (cpat == 3 && std::memcmp(pszPattern, "*.*", 3) == 0) return 1;
    while (*psz) {
        if (pszPattern < pszEnd && (*pszPattern == '?' || SDL_tolower(static_cast<unsigned char>(*pszPattern)) == SDL_tolower(static_cast<unsigned char>(*psz)))) { pszPattern++; psz++; }
        else if (pszPattern < pszEnd && *pszPattern == '*') { pszStar = ++pszPattern; pszRetry = psz; }
        else if (pszStar) { pszPattern = pszStar; psz = ++pszRetry; }
        else return 0;
    }
    while (pszPattern < pszEnd && *pszPattern == '*') pszPattern++;
    return pszPattern == pszEnd;
}
static BOOL FFileFilter(COMPATFILEDIALOG *lp, LPCSTR psz) {
    LPCSTR pszPattern, pszNext;
    if (lp->cFilter == 0) return 1;
    pszPattern = lp->lpRequest->rgFilters[static_cast<std::size_t>(lp->iFilter)].szPattern.c_str();
    while (*pszPattern) {
        pszNext = std::strchr(pszPattern, ';'); if (!pszNext) pszNext = pszPattern + std::strlen(pszPattern);
        if (FFilePattern(psz, pszPattern, static_cast<std::size_t>((pszNext - pszPattern)))) return 1;
        pszPattern = *pszNext ? pszNext + 1 : pszNext;
    }
    return 0;
}
static void FreeFileEntries(COMPATFILEDIALOG *lp) {
    std::size_t i;
    for (i = 0; i < lp->cEntry; i++) std::free(lp->rgentry[i].psz);
    std::free(lp->rgentry); lp->rgentry = nullptr; lp->cEntry = 0;
}
static int CmpFileEntry(const void *a, const void *b) {
    const COMPATFILEENTRY *lpa = static_cast<const COMPATFILEENTRY *>((a)), *lpb = static_cast<const COMPATFILEENTRY *>((b));
    if (lpa->fDir != lpb->fDir) return lpa->fDir ? -1 : 1;
    return SDL_strcasecmp(lpa->psz, lpb->psz);
}
static BOOL FJoinFile(std::span<char> pszOut, std::string_view szDir, std::string_view szName) {
#ifdef _WIN32
    char *pch;
#endif
    if (pszOut.empty()) return 0;
    if (!szName.empty() && (szName[0] == '/'
#ifdef _WIN32
        || szName[0] == '\\' || (szName.size() > 1 && szName[1] == ':')
#endif
    )) {
        if (CchCopyUtf8(pszOut, szName) < 0) return 0;
    } else {
        if (CchCopyUtf8(pszOut, szDir) < 0 ||
            (szDir != "/" && !FAppendUtf8(pszOut, "/")) || !FAppendUtf8(pszOut, szName)) return 0;
    }
#ifdef _WIN32
    // The browser splits paths with '/', including native suggestions and input.
    for (pch = pszOut.data(); *pch; pch++) if (*pch == '\\') *pch = '/';
#endif
    return FUtf8Valid(std::string_view{pszOut.data()});
}
static void FileError(HWND hwnd, LPCSTR psz) { CompatChoiceIO().pfnMessageBoxA(hwnd, psz, "File selection", win32::MB_OK | win32::MB_ICONEXCLAMATION); }
static BOOL FRefreshFiles(HWND hwnd, COMPATFILEDIALOG *lp) {
    NATIVEDIR *lpdir;
    const char *pszEntry;
    std::optional<std::string> szEntry;
    std::optional<NATIVEFILEINFO> st;
    char szPath[PATH_MAX], szLabel[PATH_MAX + 4];
    COMPATFILEENTRY *rg;
    std::size_t i;
    LRESULT iEntry;
    HWND hwndList = win32::GetDlgItem(hwnd, ID_FILE_LIST);
    lpdir = PdirNativeOpen(lp->szDir);
    NATIVEOWNER<NATIVEDIR, ICloseNativeDir> directoryOwner(lpdir);
    if (!lpdir) { FileError(hwnd, "This directory cannot be read."); return 0; }
    FreeFileEntries(lp);
    for (;;) {
        errno = 0; szEntry = SzNativeReadDir(lpdir);
        if (!szEntry) { if (errno != 0) lp->dwError = win32::CDERR_INITIALIZATION; break; }
        pszEntry = szEntry->c_str();
        if (!FUtf8Valid(std::string_view{pszEntry, szEntry->size()})) {
            FileError(hwnd, "A filename in this directory cannot be represented losslessly in UTF-8.");
            continue;
        }
        if (std::strcmp(pszEntry, ".") == 0 || std::strcmp(pszEntry, "..") == 0) continue;
        if (!lp->fHidden && pszEntry[0] == '.') continue;
        if (!FJoinFile(std::span{szPath}, lp->szDir, pszEntry) || !(st = IGetNativeFileInfo(szPath, NATIVELINKMODE::follow))) continue;
        if (!st->fDirectory && (!st->fRegular || !FFileFilter(lp, pszEntry))) continue;
        rg = static_cast<COMPATFILEENTRY *>((std::realloc(lp->rgentry, (lp->cEntry + 1) * sizeof(*rg))));
        if (!rg) { lp->dwError = win32::CDERR_MEMALLOCFAILURE; break; }
        lp->rgentry = rg;
        rg[lp->cEntry].psz = strdup(pszEntry); rg[lp->cEntry].fDir = st->fDirectory;
        if (!rg[lp->cEntry].psz) { lp->dwError = win32::CDERR_MEMALLOCFAILURE; break; }
        lp->cEntry++;
    }
    directoryOwner.reset();
    if (lp->dwError) { win32::EndDialog(hwnd, 0); return 0; }
    if (lp->cEntry > 1) std::qsort(lp->rgentry, lp->cEntry, sizeof(*lp->rgentry), CmpFileEntry);
    win32::SendMessageA(hwndList, win32::LB_RESETCONTENT, 0, 0);
    for (i = 0; i < lp->cEntry; i++) {
        CchSnprintf(std::span{szLabel}, lp->rgentry[i].fDir ? "[%s]" : "%s", lp->rgentry[i].psz);
        iEntry = win32::SendMessageA(hwndList, win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(szLabel));
        if (iEntry < 0 || win32::SendMessageA(hwndList, win32::LB_SETITEMDATA, static_cast<WPARAM>(iEntry), static_cast<LPARAM>(i)) == win32::LB_ERR) {
            lp->dwError = win32::CDERR_MEMALLOCFAILURE; win32::EndDialog(hwnd, 0); return 0;
        }
    }
    win32::SetWindowTextA(win32::GetDlgItem(hwnd, ID_FILE_PATH), lp->szDir);
    return 1;
}
static BOOL FChangeFileDirectory(HWND hwnd, COMPATFILEDIALOG *lp, LPCSTR psz, BOOL fHistory) {
    char szPath[PATH_MAX], szReal[PATH_MAX], szOld[PATH_MAX];
    std::optional<NATIVEFILEINFO> st;
    char *pszPrevious = nullptr;
    NATIVEOWNER<char, std::free> previousOwner;
    if (!FJoinFile(std::span{szPath}, lp->szDir, psz) || !CchCanonicalNativePath(szPath, std::span{szReal}) || !(st = IGetNativeFileInfo(szReal, NATIVELINKMODE::follow)) || !st->fDirectory) {
        FileError(hwnd, "The directory does not exist."); return 0;
    }
    if (!FUtf8Valid(szReal)) {
        FileError(hwnd, "The directory cannot be represented losslessly in UTF-8."); return 0;
    }
    if (std::strcmp(szReal, lp->szDir) == 0) {
        win32::SetWindowTextA(win32::GetDlgItem(hwnd, ID_FILE_PATH), lp->szDir); return 1;
    }
    if (fHistory) {
        pszPrevious = strdup(lp->szDir);
        previousOwner.reset(pszPrevious);
        if (!pszPrevious) { FileError(hwnd, "There is not enough memory to navigate."); return 0; }
    }
    CchCopyUtf8(std::span{szOld}, lp->szDir); CchCopyUtf8(std::span{lp->szDir}, szReal);
    if (!FRefreshFiles(hwnd, lp)) {
        CchCopyUtf8(std::span{lp->szDir}, szOld); previousOwner.reset();
        if (win32::IsWindow(hwnd)) win32::SetWindowTextA(win32::GetDlgItem(hwnd, ID_FILE_PATH), lp->szDir);
        return 0;
    }
    if (pszPrevious) {
        if (lp->cHistory == MAX_FILE_HISTORY) {
            std::move(lp->rgpszHistory.begin() + 1, lp->rgpszHistory.end(), lp->rgpszHistory.begin());
            lp->cHistory--;
        }
        lp->rgpszHistory[lp->cHistory++] = std::move(previousOwner);
    }
    win32::EnableWindow(win32::GetDlgItem(hwnd, ID_FILE_BACK), lp->cHistory != 0);
    win32::SetWindowTextA(win32::GetDlgItem(hwnd, ID_FILE_NAME), ""); return 1;
}
static int IAcceptFilePath(HWND hwnd, COMPATFILEDIALOG *lp, LPCSTR pszName) {
    char szPath[PATH_MAX], szParent[PATH_MAX], szReal[PATH_MAX];
    char *pszBase, *pszExt;
    const char *pszDef = lp->lpRequest->szDefaultExtension.c_str();
    std::optional<NATIVEFILEINFO> st, stParent;
    std::size_t cch;
    BOOL fExists;
    NATIVEFILE *lpfile;
    if (!*pszName) return 0;
    if (!FJoinFile(std::span{szPath}, lp->szDir, pszName)) { FileError(hwnd, "The filename is too long."); return 0; }
    if ((st = IGetNativeFileInfo(szPath, NATIVELINKMODE::follow)).has_value() && st->fDirectory) return 2;
    pszBase = std::strrchr(szPath, '/'); pszBase = pszBase ? pszBase + 1 : szPath;
    pszExt = std::strrchr(pszBase, '.');
    if (!pszExt && pszDef && *pszDef) {
        cch = std::strlen(szPath);
        if (!FUtf8Valid(pszDef)) { FileError(hwnd, "The extension is invalid UTF-8."); return 0; }
        if (CchSnprintf(std::span{szPath}.subspan(cch), ".%.*s", static_cast<int>(CchUtf8Prefix(pszDef, 3)), pszDef) >= static_cast<int>((sizeof(szPath) - cch))) {
            FileError(hwnd, "The filename is too long."); return 0;
        }
    }
    CchCopyUtf8(std::span{szParent}, szPath); pszBase = std::strrchr(szParent, '/');
    if (!pszBase || !pszBase[1] || std::strpbrk(pszBase + 1, "*?")) { FileError(hwnd, "Enter a valid filename."); return 0; }
    if (pszBase == szParent) pszBase[1] = 0; else *pszBase = 0;
    if (!CchCanonicalNativePath(szParent, std::span{szReal}) || !(stParent = IGetNativeFileInfo(szReal, NATIVELINKMODE::follow)) || !stParent->fDirectory) {
        FileError(hwnd, "The destination directory does not exist."); return 0;
    }
    pszBase = std::strrchr(szPath, '/') + 1;
    if (!FJoinFile(std::span{szParent}, szReal, pszBase)) { FileError(hwnd, "The filename is too long."); return 0; }
    fExists = (st = IGetNativeFileInfo(szParent, NATIVELINKMODE::follow)).has_value();
    if (fExists && !st->fRegular) { FileError(hwnd, "Select a regular file."); return 0; }
    if (!fExists && lp->lpRequest->fMustExist) { FileError(hwnd, "The selected file does not exist."); return 0; }
    if (lp->fSave && lp->lpRequest->fRequireWritable) {
        if ((fExists && (!(st->grfMode & 0222) || IAccessNativePath(szParent, W_OK) != 0)) ||
            (!fExists && (!(stParent->grfMode & 0222) || IAccessNativePath(szReal, W_OK) != 0))) {
            FileError(hwnd, "The selected destination is read-only."); return 0;
        }
    }
    // Probe read access without modifying a selected file or creating a new one.
    if (!lp->fSave && fExists) {
        lpfile = PfNativeOpen(szParent, NATIVEFILEMODE::read);
        if (!lpfile) { FileError(hwnd, "The selected file cannot be read."); return 0; }
        ICloseNativeFile(lpfile);
    }
    if (lp->fSave && fExists && lp->lpRequest->fConfirmOverwrite &&
        CompatChoiceIO().pfnMessageBoxA(hwnd, "This file already exists. Replace it?", "Confirm save", win32::MB_YESNO | win32::MB_ICONEXCLAMATION | win32::MB_DEFBUTTON2) != win32::IDYES) return 0;
    if (hwnd && !win32::IsWindow(hwnd)) return 0;
    CchCopyUtf8(std::span{lp->szResult}, szParent);
    cch = std::strlen(lp->szResult) + 1;
    if (lp->lpRequest->cbPathMax && cch > *lp->lpRequest->cbPathMax) {
        lp->dwError = win32::FNERR_BUFFERTOOSMALL;
        lp->cbRequiredPath = cch;
        return -1;
    }
    return 1;
}
static void AcceptFile(HWND hwnd, COMPATFILEDIALOG *lp) {
    char szName[PATH_MAX];
    int iResult;
    win32::GetWindowTextA(win32::GetDlgItem(hwnd, ID_FILE_NAME), szName, sizeof(szName));
    iResult = IAcceptFilePath(hwnd, lp, szName);
    if (iResult == 2) FChangeFileDirectory(hwnd, lp, szName, 1);
    else if (iResult != 0) win32::EndDialog(hwnd, iResult > 0 ? win32::IDOK : 0);
    else if (win32::IsWindow(hwnd)) win32::SetFocus(win32::GetDlgItem(hwnd, ID_FILE_NAME));
}
static void LayoutFileDialog(HWND hwnd) {
    RECT rc;
    int cx, cy;
    win32::GetClientRect(hwnd, &rc); cx = rc.right; cy = rc.bottom;
    win32::SetWindowPos(win32::GetDlgItem(hwnd, ID_FILE_PATH), nullptr, 80, 12, cx - 92, 24, win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
    win32::SetWindowPos(win32::GetDlgItem(hwnd, ID_FILE_LIST), nullptr, 12, 108, cx - 24, cy - 252, win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
    win32::SetWindowPos(win32::GetDlgItem(hwnd, 0x441), nullptr, 12, cy - 128, 80, 24, win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
    win32::SetWindowPos(win32::GetDlgItem(hwnd, ID_FILE_NAME), nullptr, 96, cy - 132, cx - 108, 26, win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
    win32::SetWindowPos(win32::GetDlgItem(hwnd, 0x442), nullptr, 12, cy - 92, 80, 24, win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
    win32::SetWindowPos(win32::GetDlgItem(hwnd, ID_FILE_FILTER), nullptr, 96, cy - 96, cx - 108, 120, win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
    win32::SetWindowPos(win32::GetDlgItem(hwnd, win32::IDOK), nullptr, cx - 188, cy - 52, 80, 28, win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
    win32::SetWindowPos(win32::GetDlgItem(hwnd, win32::IDCANCEL), nullptr, cx - 96, cy - 52, 80, 28, win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
}
static HWND HwndFileControl(HWND hwnd, LPCSTR pszClass, LPCSTR pszText, DWORD style, int x, int y, int cx, int cy, int id);
static INT_PTR  FolderDialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    char *pszName = reinterpret_cast<char *>(win32::GetWindowLongPtrA(hwnd, win32::DWLP_USER));
    if (msg == win32::WM_INITDIALOG) { win32::SetWindowLongPtrA(hwnd, win32::DWLP_USER, l); return 1; }
    if (msg == win32::WM_CLOSE || (msg == win32::WM_COMMAND && LOWORD(w) == win32::IDCANCEL)) {
        win32::EndDialog(hwnd, win32::IDCANCEL); return 1;
    }
    if (msg == win32::WM_COMMAND && LOWORD(w) == win32::IDOK && pszName) {
        win32::GetWindowTextA(win32::GetDlgItem(hwnd, ID_FILE_NAME), pszName, PATH_MAX);
        if (!*pszName || std::strchr(pszName, '/') || std::strcmp(pszName, ".") == 0 || std::strcmp(pszName, "..") == 0) {
            FileError(hwnd, "Enter a folder name without a path."); return 1;
        }
        win32::EndDialog(hwnd, win32::IDOK); return 1;
    }
    return 0;
}
static void NewFileFolder(HWND hwnd, COMPATFILEDIALOG *lp) {
    char szName[PATH_MAX], szPath[PATH_MAX];
    RECT rc, work;
    HWND hwndFolder, hwndFocus = nullptr;
    win32::MSG msg;
    int iRet;
    BOOL fAccepted = 0, fDisabled = 0;

    if (!win32::SystemParametersInfoA(win32::SPI_GETWORKAREA, 0, &work, 0)) return;
    rc.left = work.left + (work.right - work.left - 360) / 2;
    rc.top = work.top + (work.bottom - work.top - 156) / 2;
    rc.right = rc.left + 360; rc.bottom = rc.top + 156;
    hwndFolder = HwndCompatDynamicDialog("New folder", win32::WS_POPUP | win32::WS_CAPTION | win32::WS_SYSMENU, 0, &rc, hwnd, FolderDialogProc);
    if (!hwndFolder) { FileError(hwnd, "The new folder dialog could not be opened."); return; }
    COMPATCLEANUPSCOPE restoreParent([&] {
        if (!fDisabled || !win32::IsWindow(hwnd)) return;
        win32::EnableWindow(hwnd, 1);
        if (win32::IsWindow(hwndFocus)) win32::SetFocus(hwndFocus);
    }, hwnd);
    COMPATCLEANUPSCOPE destroyFolder([&] { if (win32::IsWindow(hwndFolder)) win32::DestroyWindow(hwndFolder); }, hwnd);
    if (!HwndFileControl(hwndFolder, "Static", "Folder &name:", 0, 12, 12, 320, 24, 0x441) ||
        !HwndFileControl(hwndFolder, "Edit", "New folder", win32::WS_TABSTOP | win32::WS_BORDER | win32::ES_AUTOHSCROLL, 12, 40, 336, 26, ID_FILE_NAME) ||
        !HwndFileControl(hwndFolder, "Button", "&Create", win32::WS_TABSTOP | win32::BS_DEFPUSHBUTTON, 172, 96, 80, 28, win32::IDOK) ||
        !HwndFileControl(hwndFolder, "Button", "Cancel", win32::WS_TABSTOP, 268, 96, 80, 28, win32::IDCANCEL)) {
        destroyFolder.Close(); FileError(hwnd, "The new folder dialog could not be initialized."); return;
    }
    win32::SendDlgItemMessageA(hwndFolder, ID_FILE_NAME, win32::EM_LIMITTEXT, PATH_MAX - 1, 0);
    win32::SendMessageA(hwndFolder, win32::WM_INITDIALOG, 0, reinterpret_cast<LPARAM>(szName));
    hwndFocus = win32::GetFocus(); fDisabled = 1; win32::EnableWindow(hwnd, 0);
    win32::ShowWindow(hwndFolder, win32::SW_SHOW); win32::SetFocus(win32::GetDlgItem(hwndFolder, ID_FILE_NAME));
    win32::SendDlgItemMessageA(hwndFolder, ID_FILE_NAME, win32::EM_SETSEL, 0, -1);
    while (win32::IsWindow(hwndFolder) && !hwndFolder->fEndDialog) {
        iRet = win32::GetMessageA(&msg, nullptr, 0, 0);
        if (iRet <= 0) { if (iRet == 0) win32::PostQuitMessage(static_cast<int>(msg.wParam)); break; }
        if (!win32::IsDialogMessageA(hwndFolder, &msg)) { win32::TranslateMessage(&msg); win32::DispatchMessageA(&msg); }
    }
    if (win32::IsWindow(hwndFolder)) {
        fAccepted = hwndFolder->fEndDialog && hwndFolder->lDlgResult == win32::IDOK;
    }
    destroyFolder.Close();
    if (!win32::IsWindow(hwnd)) return;
    restoreParent.Close();
    if (!fAccepted) return;
    if (!FJoinFile(std::span{szPath}, lp->szDir, szName)) { FileError(hwnd, "The folder name is too long."); return; }
    if (ICreateNativeDirectory(szPath, 0777) != 0) {
        FileError(hwnd, errno == EEXIST ? "A file or folder already has this name." : "The folder could not be created."); return;
    }
    FChangeFileDirectory(hwnd, lp, szName, 1);
}
static INT_PTR  FileDialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    COMPATFILEDIALOG *lp = reinterpret_cast<COMPATFILEDIALOG *>(win32::GetWindowLongPtrA(hwnd, win32::DWLP_USER));
    LRESULT i, data;
    char szPath[PATH_MAX];
    const char *pszHome;
    if (msg == win32::WM_GETMINMAXINFO) {
        (reinterpret_cast<MINMAXINFO *>(l))->ptMinTrackSize = POINT{420, 320}; return 1;
    }
    if (msg == win32::WM_INITDIALOG) { win32::SetWindowLongPtrA(hwnd, win32::DWLP_USER, l); return 1; }
    if (!lp) return 0;
    if (msg == win32::WM_SIZE) { LayoutFileDialog(hwnd); return 1; }
    if (msg == win32::WM_CLOSE) { win32::EndDialog(hwnd, win32::IDCANCEL); return 1; }
    if (msg != win32::WM_COMMAND) return 0;
    switch (LOWORD(w)) {
    case win32::IDCANCEL: win32::EndDialog(hwnd, win32::IDCANCEL); return 1;
    case win32::IDOK:
        if (win32::GetFocus() == win32::GetDlgItem(hwnd, ID_FILE_PATH)) {
            win32::GetWindowTextA(win32::GetDlgItem(hwnd, ID_FILE_PATH), szPath, sizeof(szPath));
            FChangeFileDirectory(hwnd, lp, szPath, 1);
        } else AcceptFile(hwnd, lp);
        return 1;
    case ID_FILE_NEW: NewFileFolder(hwnd, lp); return 1;
    case ID_FILE_BACK:
        if (lp->cHistory != 0 && FChangeFileDirectory(hwnd, lp, lp->rgpszHistory[lp->cHistory - 1].get(), 0)) {
            lp->rgpszHistory[--lp->cHistory].reset();
            win32::EnableWindow(win32::GetDlgItem(hwnd, ID_FILE_BACK), lp->cHistory != 0);
        }
        return 1;
    case ID_FILE_HOME:
        pszHome = PszCompatEnvironment("HOME");
#ifdef _WIN32
        pszHome = PszCompatEnvironment("USERPROFILE");
#endif
        if (pszHome && *pszHome) FChangeFileDirectory(hwnd, lp, pszHome, 1);
        else FileError(hwnd, "The home directory is not available.");
        return 1;
    case ID_FILE_HIDDEN:
        lp->fHidden = win32::SendDlgItemMessageA(hwnd, ID_FILE_HIDDEN, win32::BM_GETCHECK, 0, 0) == win32::BST_CHECKED;
        FRefreshFiles(hwnd, lp); return 1;
    case ID_FILE_UP: FChangeFileDirectory(hwnd, lp, "..", 1); return 1;
    case ID_FILE_FILTER:
        if (HIWORD(w) == win32::CBN_SELCHANGE) {
            i = win32::SendDlgItemMessageA(hwnd, ID_FILE_FILTER, win32::CB_GETCURSEL, 0, 0);
            if (i >= 0 && i < lp->cFilter) { lp->iFilter = static_cast<int>(i); FRefreshFiles(hwnd, lp); }
        }
        return 1;
    case ID_FILE_LIST:
        if (HIWORD(w) != win32::LBN_SELCHANGE && HIWORD(w) != win32::LBN_DBLCLK) return 1;
        i = win32::SendDlgItemMessageA(hwnd, ID_FILE_LIST, win32::LB_GETCURSEL, 0, 0);
        data = i == win32::LB_ERR ? win32::LB_ERR : win32::SendDlgItemMessageA(hwnd, ID_FILE_LIST, win32::LB_GETITEMDATA, static_cast<WPARAM>(i), 0);
        if (data < 0 || static_cast<std::size_t>(data) >= lp->cEntry) return 1;
        if (!lp->rgentry[data].fDir) win32::SetWindowTextA(win32::GetDlgItem(hwnd, ID_FILE_NAME), lp->rgentry[data].psz);
        if (HIWORD(w) == win32::LBN_DBLCLK) {
            if (lp->rgentry[data].fDir) FChangeFileDirectory(hwnd, lp, lp->rgentry[data].psz, 1);
            else AcceptFile(hwnd, lp);
        }
        return 1;
    default: return 0;
    }
}
static HWND HwndFileControl(HWND hwnd, LPCSTR pszClass, LPCSTR pszText, DWORD style, int x, int y, int cx, int cy, int id) {
    HWND hwndControl = win32::CreateWindowExA(0, pszClass, pszText, win32::WS_CHILD | win32::WS_VISIBLE | style, x, y, cx, cy, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), win32::GetModuleHandleA(nullptr), nullptr);
    if (hwndControl) {
        win32::SendMessageA(hwndControl, win32::WM_SETFONT, reinterpret_cast<WPARAM>(hwnd->hfontDialog), 0);
    }
    return hwndControl;
}
#if defined(STARS_FILE_DIALOG_PORTAL) || defined(STARS_FILE_DIALOG_COCOA) || defined(STARS_FILE_DIALOG_WINDOWS)
static void RememberFileSelection(COMPATFILEDIALOG *lp, LPCSTR pszPath, char *pszName) {
    char szParent[PATH_MAX], szReal[PATH_MAX], *pch;
    std::optional<NATIVEFILEINFO> st;
    /* A validation error or declined overwrite must not reset navigation. */
    if ((st = IGetNativeFileInfo(pszPath, NATIVELINKMODE::follow)).has_value() && st->fDirectory) {
        if (CchCanonicalNativePath(pszPath, std::span{szReal})) { CchCopyUtf8(std::span{lp->szDir}, szReal); pszName[0] = 0; }
        return;
    }
    SDL_strlcpy(szParent, pszPath, sizeof(szParent));
    pch = std::strrchr(szParent, '/');
    if (!pch) return;
    SDL_strlcpy(pszName, pch + 1, PATH_MAX);
    if (pch == szParent) pch[1] = 0; else *pch = 0;
    if (CchCanonicalNativePath(szParent, std::span{szReal}) && (st = IGetNativeFileInfo(szReal, NATIVELINKMODE::follow)).has_value() && st->fDirectory) CchCopyUtf8(std::span{lp->szDir}, szReal);
}
#endif
static win32::files::FILESELECTIONRESULT SelectFileCore(HWND hwndOwner, win32::files::FILESELECTIONREQUEST request) {
    COMPATFILEDIALOG dlg = {};
    HWND hwnd = nullptr, hwndRoot = nullptr, hwndFocus = nullptr;
    char szInitial[PATH_MAX], szCwd[PATH_MAX], szPath[PATH_MAX], *pch;
    RECT rc, work;
    win32::MSG msg;
    int i, iRet, cx, cy;
    BOOL fEnable = 0, fNative = 0, fRestoreFocus = 0;
    COMPATCLEANUPSCOPE freeBuffers([&] {
        FreeFileEntries(&dlg);
        for (auto &pszHistory : dlg.rgpszHistory) pszHistory.reset();
    });
    COMPATCLEANUPSCOPE restoreFocus([&] { if (fRestoreFocus && win32::IsWindow(hwndFocus) && win32::IsWindowEnabled(hwndFocus)) win32::SetFocus(hwndFocus); });
    COMPATCLEANUPSCOPE nativeCounter([&] { if (fNative && win32::IsWindow(hwndRoot)) hwndRoot->cNativeFileDialogs--; });
    COMPATCLEANUPSCOPE restoreOwner([&] { if (fEnable && win32::IsWindow(hwndRoot)) win32::EnableWindow(hwndRoot, 1); });
    COMPATCLEANUPSCOPE destroyPicker([&] { if (win32::IsWindow(hwnd)) win32::DestroyWindow(hwnd); });
    if (FStarsErrorPending()) return win32::files::FILESELECTIONCANCELLED{};
    if (request.cbPathMax && *request.cbPathMax == 0) return win32::files::FILESELECTIONFAILURE{win32::files::FILESELECTIONERROR::invalidRequest};
    if (request.rgFilters.size() > MAX_FILE_FILTERS ||
        !FUtf8Valid(request.szTitle) || request.szTitle.find('\0') != std::string::npos ||
        !FUtf8Valid(request.szInitialDirectory) || request.szInitialDirectory.find('\0') != std::string::npos ||
        !FUtf8Valid(request.szInitialPath) || request.szInitialPath.find('\0') != std::string::npos ||
        !FUtf8Valid(request.szDefaultExtension) || request.szDefaultExtension.find('\0') != std::string::npos) {
        return win32::files::FILESELECTIONFAILURE{win32::files::FILESELECTIONERROR::invalidRequest};
    }
    if ((request.fRequireWritable || request.fConfirmOverwrite) && request.mode != win32::files::FILESELECTIONMODE::save) {
        return win32::files::FILESELECTIONFAILURE{win32::files::FILESELECTIONERROR::unsupportedOption};
    }
    if (!SDL_IsMainThread() || !FCompatVideo()) {
        return win32::files::FILESELECTIONFAILURE{win32::files::FILESELECTIONERROR::unsupportedOption};
    }
    if (hwndOwner && !win32::IsWindow(hwndOwner)) {
        return win32::files::FILESELECTIONFAILURE{win32::files::FILESELECTIONERROR::ownerUnavailable};
    }
    dlg.lpRequest = &request;
    dlg.fSave = request.mode == win32::files::FILESELECTIONMODE::save;
    for (const auto &filter : request.rgFilters) {
        if (filter.szName.empty() || filter.szPattern.empty() || filter.szName.size() > 4096 || filter.szPattern.size() > 4096 ||
            filter.szName.find('\0') != std::string::npos || filter.szPattern.find('\0') != std::string::npos ||
            !FUtf8Valid(filter.szName) || !FUtf8Valid(filter.szPattern)) {
            dlg.dwError = win32::CDERR_INITIALIZATION;
            goto done;
        }
        dlg.cFilter++;
    }
    dlg.iFilter = request.iFilter < static_cast<std::size_t>(dlg.cFilter) ? static_cast<int>(request.iFilter) : 0;
    if (!CchNativeCwd(std::span{szCwd})) { dlg.dwError = win32::CDERR_INITIALIZATION; goto done; }
    if (!FUtf8Valid(szCwd)) { dlg.dwError = win32::FNERR_INVALIDFILENAME; goto done; }
    CchCopyUtf8(std::span{dlg.szDir}, szCwd); szInitial[0] = 0;
    if (!request.szInitialDirectory.empty()) {
        std::optional<NATIVEFILEINFO> st;
        if (!FJoinFile(std::span{szPath}, szCwd, request.szInitialDirectory.c_str()) || !CchCanonicalNativePath(szPath, std::span{dlg.szDir}) ||
            !(st = IGetNativeFileInfo(dlg.szDir, NATIVELINKMODE::follow)) || !st->fDirectory) CchCopyUtf8(std::span{dlg.szDir}, szCwd);
    }
    if (!request.szInitialPath.empty()) {
        if (!FJoinFile(std::span{szPath}, dlg.szDir, request.szInitialPath.c_str())) { dlg.dwError = win32::FNERR_INVALIDFILENAME; goto done; }
        pch = std::strrchr(szPath, '/');
        if (pch) {
            SDL_strlcpy(szInitial, pch + 1, sizeof(szInitial));
            if (pch == szPath) pch[1] = 0; else *pch = 0;
            if (!CchCanonicalNativePath(szPath, std::span{dlg.szDir})) { dlg.dwError = win32::FNERR_INVALIDFILENAME; goto done; }
        }
    }
    if (!FUtf8Valid(dlg.szDir)) { dlg.dwError = win32::FNERR_INVALIDFILENAME; goto done; }
    hwndFocus = win32::GetFocus(); hwndRoot = hwndOwner ? HwndCompatRoot(hwndOwner) : nullptr; fRestoreFocus = 1;
#if defined(STARS_FILE_DIALOG_PORTAL) || defined(STARS_FILE_DIALOG_COCOA) || defined(STARS_FILE_DIALOG_WINDOWS)
    fNative = !PszCompatEnvironment("STARS_FILE_DIALOG_BACKEND") || std::strcmp(PszCompatEnvironment("STARS_FILE_DIALOG_BACKEND"), "compat") != 0;
    if (fNative && hwndRoot) hwndRoot->cNativeFileDialogs++;
#endif
    fEnable = hwndRoot && win32::IsWindowEnabled(hwndRoot);
    if (fEnable) win32::EnableWindow(hwndRoot, 0);
    if (FStarsErrorPending()) goto restore;
    if (hwndRoot && !win32::IsWindow(hwndRoot)) { dlg.dwError = win32::CDERR_INITIALIZATION; goto restore; }
#if defined(STARS_FILE_DIALOG_PORTAL) || defined(STARS_FILE_DIALOG_COCOA) || defined(STARS_FILE_DIALOG_WINDOWS)
    /* Headless control tests opt into the compatibility browser explicitly. */
    if (fNative) {
        for (;;) {
            auto backendRequest = request;
            win32::files::FILESELECTIONRESULT backendResult;
            backendRequest.szTitle = request.szTitle.empty() ? (dlg.fSave ? "Save As" : "Open") : request.szTitle;
            backendRequest.szInitialDirectory = dlg.szDir;
            backendRequest.szInitialPath = szInitial;
            backendRequest.iFilter = static_cast<std::size_t>(dlg.iFilter);
            backendResult = CompatChooserIO().pfnICompatChooseFile(hwndRoot, backendRequest);
            if (std::holds_alternative<win32::files::FILESELECTIONFAILURE>(backendResult)) {
                dlg.dwError = win32::CDERR_INITIALIZATION;
                if (!hwndRoot || win32::IsWindow(hwndRoot)) FileError(hwndRoot, "Unable to complete file selection. Please try again.");
                goto restore;
            }
            if (std::holds_alternative<win32::files::FILESELECTIONCANCELLED>(backendResult)) goto restore;
            const auto &backendSelected = std::get<win32::files::FILESELECTIONSELECTED>(backendResult);
            if (backendSelected.iFilter && *backendSelected.iFilter < request.rgFilters.size())
                dlg.iFilter = static_cast<int>(*backendSelected.iFilter);
            if (CchCopyUtf8(std::span{szPath}, backendSelected.szPath) < 0) {
                dlg.dwError = win32::FNERR_INVALIDFILENAME;
                goto restore;
            }
            iRet = IAcceptFilePath(hwndRoot, &dlg, szPath);
            if (hwndRoot && !win32::IsWindow(hwndRoot)) goto restore;
            if (dlg.dwError) {
                if (dlg.dwError == win32::FNERR_BUFFERTOOSMALL)
                    FileError(hwndRoot, "The selected path is too long for this application. Please choose a shorter path.");
                goto restore;
            }
            if (iRet == 1) goto selected;
            if (iRet == 2) FileError(hwndRoot, "Select a regular file.");
            if (hwndRoot && !win32::IsWindow(hwndRoot)) goto restore;
            RememberFileSelection(&dlg, szPath, szInitial);
        }
    }
#endif
    if (!win32::SystemParametersInfoA(win32::SPI_GETWORKAREA, 0, &work, 0)) { dlg.dwError = win32::CDERR_INITIALIZATION; goto restore; }
    cx = StarsMin(640, work.right - work.left - 32); cy = StarsMin(440, work.bottom - work.top - 64);
    if (cx < 420 || cy < 320) { dlg.dwError = win32::CDERR_INITIALIZATION; goto restore; }
    rc = RECT{work.left + (work.right - work.left - cx) / 2, work.top + (work.bottom - work.top - cy) / 2, 0, 0};
    rc.right = rc.left + cx; rc.bottom = rc.top + cy;
    hwnd = HwndCompatDynamicDialog(request.szTitle.empty() ? (dlg.fSave ? "Save As" : "Open") : request.szTitle.c_str(), win32::WS_POPUP | win32::WS_CAPTION | win32::WS_SYSMENU | win32::WS_THICKFRAME, 0, &rc, hwndRoot, FileDialogProc);
    if (!hwnd) { dlg.dwError = win32::CDERR_MEMALLOCFAILURE; goto restore; }
    if (!HwndFileControl(hwnd, "Static", "&Look in:", 0, 12, 14, 64, 22, 0x443) ||
        !HwndFileControl(hwnd, "Edit", dlg.szDir, win32::WS_TABSTOP | win32::WS_BORDER | win32::ES_AUTOHSCROLL, 80, 12, cx - 92, 24, ID_FILE_PATH) ||
        !HwndFileControl(hwnd, "Button", "&Back", win32::WS_TABSTOP, 12, 44, 64, 26, ID_FILE_BACK) ||
        !HwndFileControl(hwnd, "Button", "&Up", win32::WS_TABSTOP, 84, 44, 64, 26, ID_FILE_UP) ||
        !HwndFileControl(hwnd, "Button", "&Home", win32::WS_TABSTOP, 156, 44, 64, 26, ID_FILE_HOME) ||
        !HwndFileControl(hwnd, "Button", "New &folder...", win32::WS_TABSTOP, 228, 44, 116, 26, ID_FILE_NEW) ||
        !HwndFileControl(hwnd, "Button", "Show &hidden files", win32::WS_TABSTOP | win32::BS_AUTOCHECKBOX, 12, 72, 160, 26, ID_FILE_HIDDEN) ||
        !HwndFileControl(hwnd, "ListBox", "", win32::WS_TABSTOP | win32::WS_BORDER | win32::WS_VSCROLL | win32::LBS_NOTIFY | win32::LBS_NOINTEGRALHEIGHT, 12, 42, cx - 24, cy - 182, ID_FILE_LIST) ||
        !HwndFileControl(hwnd, "Static", "File &name:", 0, 12, cy - 130, 80, 24, 0x441) ||
        !HwndFileControl(hwnd, "Edit", szInitial, win32::WS_TABSTOP | win32::WS_BORDER | win32::ES_AUTOHSCROLL, 96, cy - 134, cx - 108, 26, ID_FILE_NAME) ||
        !HwndFileControl(hwnd, "Static", "File &type:", 0, 12, cy - 94, 80, 24, 0x442) ||
        !HwndFileControl(hwnd, "ComboBox", "", win32::WS_TABSTOP | win32::CBS_DROPDOWNLIST | win32::WS_VSCROLL, 96, cy - 98, cx - 108, 120, ID_FILE_FILTER) ||
        !HwndFileControl(hwnd, "Button", dlg.fSave ? "&Save" : "&Open", win32::WS_TABSTOP | win32::BS_DEFPUSHBUTTON, cx - 188, cy - 58, 80, 28, win32::IDOK) ||
        !HwndFileControl(hwnd, "Button", "Cancel", win32::WS_TABSTOP, cx - 96, cy - 58, 80, 28, win32::IDCANCEL)) {
        dlg.dwError = win32::CDERR_MEMALLOCFAILURE; goto restore;
    }
    win32::SendDlgItemMessageA(hwnd, ID_FILE_NAME, win32::EM_LIMITTEXT, PATH_MAX - 1, 0);
    for (i = 0; i < dlg.cFilter; i++) win32::SendDlgItemMessageA(hwnd, ID_FILE_FILTER, win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(request.rgFilters[static_cast<std::size_t>(i)].szName.c_str()));
    if (dlg.cFilter) win32::SendDlgItemMessageA(hwnd, ID_FILE_FILTER, win32::CB_SETCURSEL, dlg.iFilter, 0);
    win32::SendMessageA(hwnd, win32::WM_INITDIALOG, 0, reinterpret_cast<LPARAM>(&dlg));
    win32::EnableWindow(win32::GetDlgItem(hwnd, ID_FILE_BACK), 0);
    win32::SendDlgItemMessageA(hwnd, ID_FILE_PATH, win32::EM_LIMITTEXT, PATH_MAX - 1, 0);
    LayoutFileDialog(hwnd);
    if (!FRefreshFiles(hwnd, &dlg)) goto restore;
    win32::ShowWindow(hwnd, win32::SW_SHOW); win32::SetFocus(win32::GetDlgItem(hwnd, ID_FILE_NAME));
    while (win32::IsWindow(hwnd) && !hwnd->fEndDialog) {
        iRet = win32::GetMessageA(&msg, nullptr, 0, 0);
        if (iRet <= 0) { if (iRet == 0) win32::PostQuitMessage(static_cast<int>(msg.wParam)); break; }
        if (!win32::IsDialogMessageA(hwnd, &msg)) { win32::TranslateMessage(&msg); win32::DispatchMessageA(&msg); }
    }
    if (!win32::IsWindow(hwnd) || !hwnd->fEndDialog || hwnd->lDlgResult != win32::IDOK) goto restore;
#if defined(STARS_FILE_DIALOG_PORTAL) || defined(STARS_FILE_DIALOG_COCOA) || defined(STARS_FILE_DIALOG_WINDOWS)
selected:
#endif
    {
        CchCopyUtf8(std::span{szPath}, dlg.szResult); pch = std::strrchr(szPath, '/');
        if (pch == szPath) pch[1] = 0; else *pch = 0;
        {
            const char *pszBase = std::strrchr(dlg.szResult, '/') + 1;
            const char *pszExtension = std::strrchr(pszBase, '.');
            win32::files::FILESELECTIONSELECTED selected;
            selected.szPath = dlg.szResult;
            if (dlg.cFilter) selected.iFilter = static_cast<std::size_t>(dlg.iFilter);
            selected.fExtensionDifferent = !request.szDefaultExtension.empty() &&
                (!pszExtension || SDL_strcasecmp(pszExtension + 1, request.szDefaultExtension.c_str()) != 0);
            if (request.fChangeCurrentDirectory && IChangeNativeCwd(szPath) != 0) { dlg.dwError = win32::CDERR_INITIALIZATION; goto restore; }
            destroyPicker.Close();
            restoreOwner.Close();
            nativeCounter.Close();
            restoreFocus.Close();
            freeBuffers.Close();
            return selected;
        }
    }
restore:
    destroyPicker.Close();
    restoreOwner.Close();
    nativeCounter.Close();
    restoreFocus.Close();
done:
    freeBuffers.Close();
    if (dlg.dwError == win32::FNERR_INVALIDFILENAME) {
        return win32::files::FILESELECTIONFAILURE{win32::files::FILESELECTIONERROR::invalidPath};
    }
    if (dlg.dwError == win32::FNERR_BUFFERTOOSMALL) {
        return win32::files::FILESELECTIONFAILURE{win32::files::FILESELECTIONERROR::bufferTooSmall, dlg.cbRequiredPath};
    }
    if (dlg.dwError == win32::CDERR_MEMALLOCFAILURE) {
        return win32::files::FILESELECTIONFAILURE{win32::files::FILESELECTIONERROR::allocationFailure};
    }
    if (dlg.dwError != 0) {
        return win32::files::FILESELECTIONFAILURE{win32::files::FILESELECTIONERROR::backendFailure};
    }
    return win32::files::FILESELECTIONCANCELLED{};
}

namespace win32::files {
FILESELECTIONRESULT SelectFile(HWND hwndOwner, FILESELECTIONREQUEST request) {
    return SelectFileCore(hwndOwner, std::move(request));
}
} // namespace win32::files

static DWORD DwFileDialogError(win32::files::FILESELECTIONERROR error) {
    using win32::files::FILESELECTIONERROR;
    switch (error) {
    case FILESELECTIONERROR::invalidRequest: return win32::CDERR_INITIALIZATION;
    case FILESELECTIONERROR::invalidPath: return win32::FNERR_INVALIDFILENAME;
    case FILESELECTIONERROR::unsupportedOption: return win32::CDERR_INITIALIZATION;
    case FILESELECTIONERROR::ownerUnavailable: return win32::CDERR_INITIALIZATION;
    case FILESELECTIONERROR::allocationFailure: return win32::CDERR_MEMALLOCFAILURE;
    case FILESELECTIONERROR::bufferTooSmall: return win32::FNERR_BUFFERTOOSMALL;
    case FILESELECTIONERROR::backendFailure: return win32::CDERR_INITIALIZATION;
    }
    return win32::CDERR_INITIALIZATION;
}

static BOOL FFileDialog(OPENFILENAMEA *lpof, BOOL fSave) {
    DWORD grFlagsAllowed = win32::OFN_HIDEREADONLY | win32::OFN_PATHMUSTEXIST | win32::OFN_FILEMUSTEXIST | win32::OFN_OVERWRITEPROMPT |
                          win32::OFN_NOREADONLYRETURN | win32::OFN_NOCHANGEDIR | win32::OFN_EXPLORER | win32::OFN_EXTENSIONDIFFERENT;
    win32::files::FILESELECTIONREQUEST request;
    const char *psz, *pszPattern;
    win32::files::FILESELECTIONRESULT result;
    const char *pchBase, *pchExtension;

    dwDialogError = 0;
    if (FStarsErrorPending()) return 0;
    if (!lpof || lpof->lStructSize != sizeof(*lpof)) { dwDialogError = win32::CDERR_STRUCTSIZE; return 0; }
    if (!lpof->lpstrFile || lpof->nMaxFile == 0 || !std::memchr(lpof->lpstrFile, 0, lpof->nMaxFile)) {
        dwDialogError = win32::FNERR_INVALIDFILENAME; return 0;
    }
    if (!FUtf8Valid(lpof->lpstrFile) ||
        (lpof->lpstrInitialDir && !FUtf8Valid(lpof->lpstrInitialDir))) {
        dwDialogError = win32::FNERR_INVALIDFILENAME; return 0;
    }
    if ((lpof->Flags & ~grFlagsAllowed) || lpof->lpstrCustomFilter || lpof->lpfnHook || lpof->FlagsEx ||
        !SDL_IsMainThread() || !FCompatVideo()) { dwDialogError = win32::CDERR_INITIALIZATION; win32::SetLastError(win32::ERROR_NOT_SUPPORTED); return 0; }
    if (lpof->hwndOwner && !win32::IsWindow(lpof->hwndOwner)) { dwDialogError = win32::CDERR_INITIALIZATION; win32::SetLastError(win32::ERROR_INVALID_WINDOW_HANDLE); return 0; }
    try {
        request.mode = fSave ? win32::files::FILESELECTIONMODE::save : win32::files::FILESELECTIONMODE::open;
        if (lpof->lpstrTitle) request.szTitle = lpof->lpstrTitle;
        if (lpof->lpstrInitialDir) request.szInitialDirectory = lpof->lpstrInitialDir;
        request.szInitialPath = lpof->lpstrFile;
        if (lpof->lpstrDefExt) request.szDefaultExtension = lpof->lpstrDefExt;
        request.cbPathMax = lpof->nMaxFile;
        request.iFilter = lpof->nFilterIndex > 0 ? static_cast<std::size_t>(lpof->nFilterIndex - 1) : 0;
        request.fMustExist = (lpof->Flags & win32::OFN_FILEMUSTEXIST) != 0;
        request.fRequireWritable = (lpof->Flags & win32::OFN_NOREADONLYRETURN) != 0;
        request.fConfirmOverwrite = (lpof->Flags & win32::OFN_OVERWRITEPROMPT) != 0;
        request.fChangeCurrentDirectory = (lpof->Flags & win32::OFN_NOCHANGEDIR) == 0;
        for (psz = lpof->lpstrFilter; psz && *psz;) {
            if (request.rgFilters.size() == MAX_FILE_FILTERS || SDL_strnlen(psz, 4097) > 4096) {
                dwDialogError = win32::CDERR_INITIALIZATION;
                return 0;
            }
            pszPattern = psz + std::strlen(psz) + 1;
            if (!*pszPattern || SDL_strnlen(pszPattern, 4097) > 4096) {
                dwDialogError = win32::CDERR_INITIALIZATION;
                return 0;
            }
            request.rgFilters.push_back({psz, pszPattern});
            psz = pszPattern + std::strlen(pszPattern) + 1;
        }
    } catch (const std::bad_alloc &) {
        dwDialogError = win32::CDERR_MEMALLOCFAILURE;
        return 0;
    } catch (...) {
        dwDialogError = win32::CDERR_INITIALIZATION;
        return 0;
    }
    result = win32::files::SelectFile(lpof->hwndOwner, std::move(request));
    if (const auto *pSelected = std::get_if<win32::files::FILESELECTIONSELECTED>(&result)) {
        if (CchCopyUtf8(std::span{lpof->lpstrFile, static_cast<std::size_t>(lpof->nMaxFile)}, pSelected->szPath) < 0) {
            dwDialogError = win32::FNERR_INVALIDFILENAME;
            return 0;
        }
        pchBase = std::strrchr(pSelected->szPath.c_str(), '/') + 1;
        lpof->nFileOffset = static_cast<WORD>(pchBase - pSelected->szPath.c_str());
        if (lpof->lpstrFileTitle && lpof->nMaxFileTitle)
            CchCopyUtf8(std::span{lpof->lpstrFileTitle, static_cast<std::size_t>(lpof->nMaxFileTitle)},
                        std::string_view{pchBase, CchUtf8Prefix(pchBase, lpof->nMaxFileTitle - 1)});
        pchExtension = std::strrchr(pchBase, '.');
        lpof->nFileExtension = pchExtension ? static_cast<WORD>(pchExtension - pSelected->szPath.c_str() + 1) : 0;
        lpof->nFilterIndex = pSelected->iFilter ? static_cast<DWORD>(*pSelected->iFilter + 1) : 0;
        lpof->Flags &= ~win32::OFN_EXTENSIONDIFFERENT;
        if (pSelected->fExtensionDifferent) lpof->Flags |= win32::OFN_EXTENSIONDIFFERENT;
        return 1;
    }
    if (const auto *pFailure = std::get_if<win32::files::FILESELECTIONFAILURE>(&result)) {
        dwDialogError = DwFileDialogError(pFailure->error);
        if (pFailure->error == win32::files::FILESELECTIONERROR::unsupportedOption)
            win32::SetLastError(win32::ERROR_NOT_SUPPORTED);
        if (pFailure->error == win32::files::FILESELECTIONERROR::ownerUnavailable)
            win32::SetLastError(win32::ERROR_INVALID_WINDOW_HANDLE);
        if (pFailure->error == win32::files::FILESELECTIONERROR::bufferTooSmall && lpof->nMaxFile >= sizeof(WORD)) {
            WORD required = static_cast<WORD>(StarsMin(pFailure->cbRequiredPath, UINT16_MAX));
            std::memcpy(lpof->lpstrFile, &required, sizeof(required));
        }
    }
    return 0;
}

BOOL win32::GetOpenFileNameA(LPOPENFILENAMEA lpof) { return FFileDialog(lpof, 0); }
BOOL win32::GetSaveFileNameA(LPOPENFILENAMEA lpof) {
    BOOL fResult = FFileDialog(lpof, 1);
    if (!fResult && dwDialogError != 0)
        SDL_Log("Save dialog failed (common-dialog error %lu)", static_cast<unsigned long>(dwDialogError));
    return fResult;
}
