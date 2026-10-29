/* Stars' wildcard filters need a browser independent of SDL's extension-only API. */
#include "filedialog_backend.h"
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define ID_FILE_NAME 0x480
#define ID_FILE_LIST 0x460
#define ID_FILE_FILTER 0x470
#define ID_FILE_PATH 0x440
#define ID_FILE_UP 0x400
#define ID_FILE_BACK 0x401
#define ID_FILE_HOME 0x402
#define ID_FILE_HIDDEN 0x403
#define ID_FILE_NEW 0x404
#define MAX_FILE_HISTORY 32
#define MAX_FILE_FILTERS 64

typedef struct COMPATFILEENTRY { char *psz; BOOL fDir; } COMPATFILEENTRY;
typedef struct COMPATFILEDIALOG {
    OPENFILENAMEA *lpof;
    BOOL fSave;
    char szDir[PATH_MAX], szResult[PATH_MAX];
    COMPATFILEFILTER rgfilter[MAX_FILE_FILTERS];
    int cFilter, iFilter;
    COMPATFILEENTRY *rgentry;
    size_t cEntry;
    char *rgpszHistory[MAX_FILE_HISTORY];
    int cHistory;
    BOOL fHidden;
    DWORD dwError;
} COMPATFILEDIALOG;
static thread_local DWORD dwDialogError;
DWORD WINAPI CommDlgExtendedError(void) { return dwDialogError; }
void CompatSetDialogError(DWORD dw) { dwDialogError = dw; }

static BOOL FFilePattern(LPCSTR psz, LPCSTR pszPattern, size_t cpat) {
    LPCSTR pszStar = NULL, pszRetry = NULL, pszEnd = pszPattern + cpat;
    if (cpat == 3 && memcmp(pszPattern, "*.*", 3) == 0) return TRUE;
    while (*psz) {
        if (pszPattern < pszEnd && (*pszPattern == '?' || SDL_tolower((unsigned char)*pszPattern) == SDL_tolower((unsigned char)*psz))) { pszPattern++; psz++; }
        else if (pszPattern < pszEnd && *pszPattern == '*') { pszStar = ++pszPattern; pszRetry = psz; }
        else if (pszStar) { pszPattern = pszStar; psz = ++pszRetry; }
        else return FALSE;
    }
    while (pszPattern < pszEnd && *pszPattern == '*') pszPattern++;
    return pszPattern == pszEnd;
}
static BOOL FFileFilter(COMPATFILEDIALOG *lp, LPCSTR psz) {
    LPCSTR pszPattern, pszNext;
    if (lp->cFilter == 0) return TRUE;
    pszPattern = lp->rgfilter[lp->iFilter].pszPattern;
    while (*pszPattern) {
        pszNext = strchr(pszPattern, ';'); if (!pszNext) pszNext = pszPattern + strlen(pszPattern);
        if (FFilePattern(psz, pszPattern, (size_t)(pszNext - pszPattern))) return TRUE;
        pszPattern = *pszNext ? pszNext + 1 : pszNext;
    }
    return FALSE;
}
static void FreeFileEntries(COMPATFILEDIALOG *lp) {
    size_t i;
    for (i = 0; i < lp->cEntry; i++) free(lp->rgentry[i].psz);
    free(lp->rgentry); lp->rgentry = NULL; lp->cEntry = 0;
}
static int CmpFileEntry(const void *a, const void *b) {
    const COMPATFILEENTRY *lpa = (const COMPATFILEENTRY *)(a), *lpb = (const COMPATFILEENTRY *)(b);
    if (lpa->fDir != lpb->fDir) return lpa->fDir ? -1 : 1;
    return SDL_strcasecmp(lpa->psz, lpb->psz);
}
static BOOL FJoinFile(char *pszOut, LPCSTR pszDir, LPCSTR pszName) {
#ifdef _WIN32
    char *pch;
#endif
    int cch;

    if (*pszName == '/'
#ifdef _WIN32
        || *pszName == '\\' || (pszName[0] && pszName[1] == ':')
#endif
    ) {
        if (SDL_strlcpy(pszOut, pszName, PATH_MAX) >= PATH_MAX) return FALSE;
    } else {
        cch = snprintf(pszOut, PATH_MAX, "%s%s%s", pszDir, strcmp(pszDir, "/") == 0 ? "" : "/", pszName);
        if (cch < 0 || cch >= PATH_MAX) return FALSE;
    }
#ifdef _WIN32
    // The browser splits paths with '/', including native suggestions and input.
    for (pch = pszOut; *pch; pch++) if (*pch == '\\') *pch = '/';
#endif
    return FUtf8Valid(pszOut, strlen(pszOut));
}
static void FileError(HWND hwnd, LPCSTR psz) { MessageBoxA(hwnd, psz, "File selection", MB_OK | MB_ICONEXCLAMATION); }
static BOOL FRefreshFiles(HWND hwnd, COMPATFILEDIALOG *lp) {
    NATIVEDIR *lpdir;
    const char *pszEntry;
    NATIVEFILEINFO st;
    char szPath[PATH_MAX], szLabel[PATH_MAX + 4];
    COMPATFILEENTRY *rg;
    size_t i;
    LRESULT iEntry;
    HWND hwndList = GetDlgItem(hwnd, ID_FILE_LIST);
    lpdir = PdirNativeOpen(lp->szDir);
    if (!lpdir) { FileError(hwnd, "This directory cannot be read."); return FALSE; }
    FreeFileEntries(lp);
    for (;;) {
        errno = 0; pszEntry = PszNativeReadDir(lpdir);
        if (!pszEntry) { if (errno != 0) lp->dwError = CDERR_INITIALIZATION; break; }
        if (!FUtf8Valid(pszEntry, strlen(pszEntry))) {
            FileError(hwnd, "A filename in this directory cannot be represented losslessly in UTF-8.");
            continue;
        }
        if (strcmp(pszEntry, ".") == 0 || strcmp(pszEntry, "..") == 0) continue;
        if (!lp->fHidden && pszEntry[0] == '.') continue;
        if (!FJoinFile(szPath, lp->szDir, pszEntry) || IGetNativeFileInfo(szPath, &st, TRUE) != 0) continue;
        if (!st.fDirectory && (!st.fRegular || !FFileFilter(lp, pszEntry))) continue;
        rg = (COMPATFILEENTRY *)(realloc(lp->rgentry, (lp->cEntry + 1) * sizeof(*rg)));
        if (!rg) { lp->dwError = CDERR_MEMALLOCFAILURE; break; }
        lp->rgentry = rg;
        rg[lp->cEntry].psz = strdup(pszEntry); rg[lp->cEntry].fDir = st.fDirectory;
        if (!rg[lp->cEntry].psz) { lp->dwError = CDERR_MEMALLOCFAILURE; break; }
        lp->cEntry++;
    }
    ICloseNativeDir(lpdir);
    if (lp->dwError) { EndDialog(hwnd, 0); return FALSE; }
    if (lp->cEntry > 1) qsort(lp->rgentry, lp->cEntry, sizeof(*lp->rgentry), CmpFileEntry);
    SendMessageA(hwndList, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < lp->cEntry; i++) {
        snprintf(szLabel, sizeof(szLabel), lp->rgentry[i].fDir ? "[%s]" : "%s", lp->rgentry[i].psz);
        iEntry = SendMessageA(hwndList, LB_ADDSTRING, 0, (LPARAM)szLabel);
        if (iEntry < 0 || SendMessageA(hwndList, LB_SETITEMDATA, (WPARAM)iEntry, (LPARAM)i) == LB_ERR) {
            lp->dwError = CDERR_MEMALLOCFAILURE; EndDialog(hwnd, 0); return FALSE;
        }
    }
    SetWindowTextA(GetDlgItem(hwnd, ID_FILE_PATH), lp->szDir);
    return TRUE;
}
static BOOL FChangeFileDirectory(HWND hwnd, COMPATFILEDIALOG *lp, LPCSTR psz, BOOL fHistory) {
    char szPath[PATH_MAX], szReal[PATH_MAX], szOld[PATH_MAX];
    NATIVEFILEINFO st;
    char *pszPrevious = NULL;
    if (!FJoinFile(szPath, lp->szDir, psz) || !PszCanonicalNativePath(szPath, szReal, sizeof(szReal)) || IGetNativeFileInfo(szReal, &st, TRUE) != 0 || !st.fDirectory) {
        FileError(hwnd, "The directory does not exist."); return FALSE;
    }
    if (!FUtf8Valid(szReal, strlen(szReal))) {
        FileError(hwnd, "The directory cannot be represented losslessly in UTF-8."); return FALSE;
    }
    if (strcmp(szReal, lp->szDir) == 0) {
        SetWindowTextA(GetDlgItem(hwnd, ID_FILE_PATH), lp->szDir); return TRUE;
    }
    if (fHistory) {
        pszPrevious = strdup(lp->szDir);
        if (!pszPrevious) { FileError(hwnd, "There is not enough memory to navigate."); return FALSE; }
    }
    strcpy(szOld, lp->szDir); strcpy(lp->szDir, szReal);
    if (!FRefreshFiles(hwnd, lp)) {
        strcpy(lp->szDir, szOld); free(pszPrevious);
        if (IsWindow(hwnd)) SetWindowTextA(GetDlgItem(hwnd, ID_FILE_PATH), lp->szDir);
        return FALSE;
    }
    if (pszPrevious) {
        if (lp->cHistory == MAX_FILE_HISTORY) {
            free(lp->rgpszHistory[0]);
            memmove(lp->rgpszHistory, lp->rgpszHistory + 1, (MAX_FILE_HISTORY - 1) * sizeof(char *));
            lp->cHistory--;
        }
        lp->rgpszHistory[lp->cHistory++] = pszPrevious;
    }
    EnableWindow(GetDlgItem(hwnd, ID_FILE_BACK), lp->cHistory != 0);
    SetWindowTextA(GetDlgItem(hwnd, ID_FILE_NAME), ""); return TRUE;
}
static int IAcceptFilePath(HWND hwnd, COMPATFILEDIALOG *lp, LPCSTR pszName) {
    char szPath[PATH_MAX], szParent[PATH_MAX], szReal[PATH_MAX];
    char *pszBase, *pszExt;
    const char *pszDef = lp->lpof->lpstrDefExt;
    NATIVEFILEINFO st, stParent;
    size_t cch;
    BOOL fExists;
    NATIVEFILE *lpfile;
    if (!*pszName) return 0;
    if (!FJoinFile(szPath, lp->szDir, pszName)) { FileError(hwnd, "The filename is too long."); return 0; }
    if (IGetNativeFileInfo(szPath, &st, TRUE) == 0 && st.fDirectory) return 2;
    pszBase = strrchr(szPath, '/'); pszBase = pszBase ? pszBase + 1 : szPath;
    pszExt = strrchr(pszBase, '.');
    if (!pszExt && pszDef && *pszDef) {
        cch = strlen(szPath);
        if (!FUtf8Valid(pszDef, strlen(pszDef))) { FileError(hwnd, "The extension is invalid UTF-8."); return 0; }
        if (snprintf(szPath + cch, sizeof(szPath) - cch, ".%.*s", (int)CchUtf8Prefix(pszDef, 3), pszDef) >= (int)(sizeof(szPath) - cch)) {
            FileError(hwnd, "The filename is too long."); return 0;
        }
    }
    strcpy(szParent, szPath); pszBase = strrchr(szParent, '/');
    if (!pszBase || !pszBase[1] || strpbrk(pszBase + 1, "*?")) { FileError(hwnd, "Enter a valid filename."); return 0; }
    if (pszBase == szParent) pszBase[1] = 0; else *pszBase = 0;
    if (!PszCanonicalNativePath(szParent, szReal, sizeof(szReal)) || IGetNativeFileInfo(szReal, &stParent, TRUE) != 0 || !stParent.fDirectory) {
        FileError(hwnd, "The destination directory does not exist."); return 0;
    }
    pszBase = strrchr(szPath, '/') + 1;
    if (!FJoinFile(szParent, szReal, pszBase)) { FileError(hwnd, "The filename is too long."); return 0; }
    fExists = IGetNativeFileInfo(szParent, &st, TRUE) == 0;
    if (fExists && !st.fRegular) { FileError(hwnd, "Select a regular file."); return 0; }
    if (!fExists && (lp->lpof->Flags & OFN_FILEMUSTEXIST)) { FileError(hwnd, "The selected file does not exist."); return 0; }
    if (lp->fSave && (lp->lpof->Flags & OFN_NOREADONLYRETURN)) {
        if ((fExists && (!(st.grfMode & 0222) || IAccessNativePath(szParent, W_OK) != 0)) ||
            (!fExists && (!(stParent.grfMode & 0222) || IAccessNativePath(szReal, W_OK) != 0))) {
            FileError(hwnd, "The selected destination is read-only."); return 0;
        }
    }
    // Probe read access without modifying a selected file or creating a new one.
    if (!lp->fSave && fExists) {
        lpfile = PfNativeOpen(szParent, "rb");
        if (!lpfile) { FileError(hwnd, "The selected file cannot be read."); return 0; }
        ICloseNativeFile(lpfile);
    }
    if (lp->fSave && fExists && (lp->lpof->Flags & OFN_OVERWRITEPROMPT) &&
        MessageBoxA(hwnd, "This file already exists. Replace it?", "Confirm save", MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2) != IDYES) return 0;
    if (hwnd && !IsWindow(hwnd)) return 0;
    strcpy(lp->szResult, szParent);
    cch = strlen(lp->szResult) + 1;
    if (cch > lp->lpof->nMaxFile) {
        lp->dwError = FNERR_BUFFERTOOSMALL;
        if (lp->lpof->nMaxFile >= sizeof(WORD)) { WORD required = (WORD)StarsMin(cch, UINT16_MAX); memcpy(lp->lpof->lpstrFile, &required, sizeof(required)); }
        return -1;
    }
    return 1;
}
static void AcceptFile(HWND hwnd, COMPATFILEDIALOG *lp) {
    char szName[PATH_MAX];
    int iResult;
    GetWindowTextA(GetDlgItem(hwnd, ID_FILE_NAME), szName, sizeof(szName));
    iResult = IAcceptFilePath(hwnd, lp, szName);
    if (iResult == 2) FChangeFileDirectory(hwnd, lp, szName, TRUE);
    else if (iResult != 0) EndDialog(hwnd, iResult > 0 ? IDOK : 0);
    else if (IsWindow(hwnd)) SetFocus(GetDlgItem(hwnd, ID_FILE_NAME));
}
static void LayoutFileDialog(HWND hwnd) {
    RECT rc;
    int cx, cy;
    GetClientRect(hwnd, &rc); cx = rc.right; cy = rc.bottom;
    SetWindowPos(GetDlgItem(hwnd, ID_FILE_PATH), NULL, 80, 12, cx - 92, 24, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(GetDlgItem(hwnd, ID_FILE_LIST), NULL, 12, 108, cx - 24, cy - 252, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(GetDlgItem(hwnd, 0x441), NULL, 12, cy - 128, 80, 24, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(GetDlgItem(hwnd, ID_FILE_NAME), NULL, 96, cy - 132, cx - 108, 26, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(GetDlgItem(hwnd, 0x442), NULL, 12, cy - 92, 80, 24, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(GetDlgItem(hwnd, ID_FILE_FILTER), NULL, 96, cy - 96, cx - 108, 120, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(GetDlgItem(hwnd, IDOK), NULL, cx - 188, cy - 52, 80, 28, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(GetDlgItem(hwnd, IDCANCEL), NULL, cx - 96, cy - 52, 80, 28, SWP_NOZORDER | SWP_NOACTIVATE);
}
static HWND HwndFileControl(HWND hwnd, LPCSTR pszClass, LPCSTR pszText, DWORD style, int x, int y, int cx, int cy, int id);
static INT_PTR CALLBACK FolderDialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    char *pszName = (char *)GetWindowLongPtrA(hwnd, DWLP_USER);
    if (msg == WM_INITDIALOG) { SetWindowLongPtrA(hwnd, DWLP_USER, l); return TRUE; }
    if (msg == WM_CLOSE || (msg == WM_COMMAND && LOWORD(w) == IDCANCEL)) {
        EndDialog(hwnd, IDCANCEL); return TRUE;
    }
    if (msg == WM_COMMAND && LOWORD(w) == IDOK && pszName) {
        GetWindowTextA(GetDlgItem(hwnd, ID_FILE_NAME), pszName, PATH_MAX);
        if (!*pszName || strchr(pszName, '/') || strcmp(pszName, ".") == 0 || strcmp(pszName, "..") == 0) {
            FileError(hwnd, "Enter a folder name without a path."); return TRUE;
        }
        EndDialog(hwnd, IDOK); return TRUE;
    }
    return FALSE;
}
static void NewFileFolder(HWND hwnd, COMPATFILEDIALOG *lp) {
    char szName[PATH_MAX], szPath[PATH_MAX];
    RECT rc, work;
    HWND hwndFolder, hwndFocus;
    MSG msg;
    int iRet;
    BOOL fAccepted = FALSE;

    if (!SystemParametersInfoA(SPI_GETWORKAREA, 0, &work, 0)) return;
    rc.left = work.left + (work.right - work.left - 360) / 2;
    rc.top = work.top + (work.bottom - work.top - 156) / 2;
    rc.right = rc.left + 360; rc.bottom = rc.top + 156;
    hwndFolder = HwndCompatDynamicDialog("New folder", WS_POPUP | WS_CAPTION | WS_SYSMENU, 0, &rc, hwnd, FolderDialogProc);
    if (!hwndFolder) { FileError(hwnd, "The new folder dialog could not be opened."); return; }
    if (!HwndFileControl(hwndFolder, "Static", "Folder &name:", 0, 12, 12, 320, 24, 0x441) ||
        !HwndFileControl(hwndFolder, "Edit", "New folder", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 12, 40, 336, 26, ID_FILE_NAME) ||
        !HwndFileControl(hwndFolder, "Button", "&Create", WS_TABSTOP | BS_DEFPUSHBUTTON, 172, 96, 80, 28, IDOK) ||
        !HwndFileControl(hwndFolder, "Button", "Cancel", WS_TABSTOP, 268, 96, 80, 28, IDCANCEL)) {
        DestroyWindow(hwndFolder); FileError(hwnd, "The new folder dialog could not be initialized."); return;
    }
    SendDlgItemMessageA(hwndFolder, ID_FILE_NAME, EM_LIMITTEXT, PATH_MAX - 1, 0);
    SendMessageA(hwndFolder, WM_INITDIALOG, 0, (LPARAM)szName);
    hwndFocus = GetFocus(); EnableWindow(hwnd, FALSE);
    ShowWindow(hwndFolder, SW_SHOW); SetFocus(GetDlgItem(hwndFolder, ID_FILE_NAME));
    SendDlgItemMessageA(hwndFolder, ID_FILE_NAME, EM_SETSEL, 0, -1);
    while (IsWindow(hwndFolder) && !hwndFolder->fEndDialog) {
        iRet = GetMessageA(&msg, NULL, 0, 0);
        if (iRet <= 0) { if (iRet == 0) PostQuitMessage((int)msg.wParam); break; }
        if (!IsDialogMessageA(hwndFolder, &msg)) { TranslateMessage(&msg); DispatchMessageA(&msg); }
    }
    if (IsWindow(hwndFolder)) {
        fAccepted = hwndFolder->fEndDialog && hwndFolder->lDlgResult == IDOK;
        DestroyWindow(hwndFolder);
    }
    if (!IsWindow(hwnd)) return;
    EnableWindow(hwnd, TRUE);
    if (IsWindow(hwndFocus)) SetFocus(hwndFocus);
    if (!fAccepted) return;
    if (!FJoinFile(szPath, lp->szDir, szName)) { FileError(hwnd, "The folder name is too long."); return; }
    if (ICreateNativeDirectory(szPath, 0777) != 0) {
        FileError(hwnd, errno == EEXIST ? "A file or folder already has this name." : "The folder could not be created."); return;
    }
    FChangeFileDirectory(hwnd, lp, szName, TRUE);
}
static INT_PTR CALLBACK FileDialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    COMPATFILEDIALOG *lp = (COMPATFILEDIALOG *)GetWindowLongPtrA(hwnd, DWLP_USER);
    LRESULT i, data;
    char szPath[PATH_MAX];
    const char *pszHome;
    if (msg == WM_GETMINMAXINFO) {
        ((MINMAXINFO *)l)->ptMinTrackSize = (POINT){420, 320}; return TRUE;
    }
    if (msg == WM_INITDIALOG) { SetWindowLongPtrA(hwnd, DWLP_USER, l); return TRUE; }
    if (!lp) return FALSE;
    if (msg == WM_SIZE) { LayoutFileDialog(hwnd); return TRUE; }
    if (msg == WM_CLOSE) { EndDialog(hwnd, IDCANCEL); return TRUE; }
    if (msg != WM_COMMAND) return FALSE;
    switch (LOWORD(w)) {
    case IDCANCEL: EndDialog(hwnd, IDCANCEL); return TRUE;
    case IDOK:
        if (GetFocus() == GetDlgItem(hwnd, ID_FILE_PATH)) {
            GetWindowTextA(GetDlgItem(hwnd, ID_FILE_PATH), szPath, sizeof(szPath));
            FChangeFileDirectory(hwnd, lp, szPath, TRUE);
        } else AcceptFile(hwnd, lp);
        return TRUE;
    case ID_FILE_NEW: NewFileFolder(hwnd, lp); return TRUE;
    case ID_FILE_BACK:
        if (lp->cHistory != 0 && FChangeFileDirectory(hwnd, lp, lp->rgpszHistory[lp->cHistory - 1], FALSE)) {
            free(lp->rgpszHistory[--lp->cHistory]);
            EnableWindow(GetDlgItem(hwnd, ID_FILE_BACK), lp->cHistory != 0);
        }
        return TRUE;
    case ID_FILE_HOME:
        pszHome = PszCompatEnvironment("HOME");
#ifdef _WIN32
        pszHome = PszCompatEnvironment("USERPROFILE");
#endif
        if (pszHome && *pszHome) FChangeFileDirectory(hwnd, lp, pszHome, TRUE);
        else FileError(hwnd, "The home directory is not available.");
        return TRUE;
    case ID_FILE_HIDDEN:
        lp->fHidden = SendDlgItemMessageA(hwnd, ID_FILE_HIDDEN, BM_GETCHECK, 0, 0) == BST_CHECKED;
        FRefreshFiles(hwnd, lp); return TRUE;
    case ID_FILE_UP: FChangeFileDirectory(hwnd, lp, "..", TRUE); return TRUE;
    case ID_FILE_FILTER:
        if (HIWORD(w) == CBN_SELCHANGE) {
            i = SendDlgItemMessageA(hwnd, ID_FILE_FILTER, CB_GETCURSEL, 0, 0);
            if (i >= 0 && i < lp->cFilter) { lp->iFilter = (int)i; FRefreshFiles(hwnd, lp); }
        }
        return TRUE;
    case ID_FILE_LIST:
        if (HIWORD(w) != LBN_SELCHANGE && HIWORD(w) != LBN_DBLCLK) return TRUE;
        i = SendDlgItemMessageA(hwnd, ID_FILE_LIST, LB_GETCURSEL, 0, 0);
        data = i == LB_ERR ? LB_ERR : SendDlgItemMessageA(hwnd, ID_FILE_LIST, LB_GETITEMDATA, (WPARAM)i, 0);
        if (data < 0 || (size_t)data >= lp->cEntry) return TRUE;
        if (!lp->rgentry[data].fDir) SetWindowTextA(GetDlgItem(hwnd, ID_FILE_NAME), lp->rgentry[data].psz);
        if (HIWORD(w) == LBN_DBLCLK) {
            if (lp->rgentry[data].fDir) FChangeFileDirectory(hwnd, lp, lp->rgentry[data].psz, TRUE);
            else AcceptFile(hwnd, lp);
        }
        return TRUE;
    default: return FALSE;
    }
}
static HWND HwndFileControl(HWND hwnd, LPCSTR pszClass, LPCSTR pszText, DWORD style, int x, int y, int cx, int cy, int id) {
    HWND hwndControl = CreateWindowExA(0, pszClass, pszText, WS_CHILD | WS_VISIBLE | style, x, y, cx, cy, hwnd, (HMENU)(INT_PTR)id, GetModuleHandleA(NULL), NULL);
    if (hwndControl) {
        SendMessageA(hwndControl, WM_SETFONT, (WPARAM)hwnd->hfontDialog, FALSE);
    }
    return hwndControl;
}
#if defined(STARS_FILE_DIALOG_PORTAL) || defined(STARS_FILE_DIALOG_COCOA) || defined(STARS_FILE_DIALOG_WINDOWS)
static void RememberFileSelection(COMPATFILEDIALOG *lp, LPCSTR pszPath, char *pszName) {
    char szParent[PATH_MAX], szReal[PATH_MAX], *pch;
    NATIVEFILEINFO st;
    /* A validation error or declined overwrite must not reset navigation. */
    if (IGetNativeFileInfo(pszPath, &st, TRUE) == 0 && st.fDirectory) {
        if (PszCanonicalNativePath(pszPath, szReal, sizeof(szReal))) { strcpy(lp->szDir, szReal); pszName[0] = 0; }
        return;
    }
    SDL_strlcpy(szParent, pszPath, sizeof(szParent));
    pch = strrchr(szParent, '/');
    if (!pch) return;
    SDL_strlcpy(pszName, pch + 1, PATH_MAX);
    if (pch == szParent) pch[1] = 0; else *pch = 0;
    if (PszCanonicalNativePath(szParent, szReal, sizeof(szReal)) && IGetNativeFileInfo(szReal, &st, TRUE) == 0 && st.fDirectory) strcpy(lp->szDir, szReal);
}
#endif
static BOOL FFileDialog(OPENFILENAMEA *lpof, BOOL fSave) {
    COMPATFILEDIALOG dlg = {};
    LPCSTR psz, pszPattern;
    HWND hwnd = NULL, hwndRoot = NULL, hwndFocus;
    char szInitial[PATH_MAX], szCwd[PATH_MAX], szPath[PATH_MAX], *pch;
    RECT rc, work;
    MSG msg;
    int i, iRet, cx, cy;
    BOOL fEnable = FALSE, fRet = FALSE, fNative = FALSE;
    DWORD grFlagsAllowed = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT |
                    OFN_NOREADONLYRETURN | OFN_NOCHANGEDIR | OFN_EXPLORER | OFN_EXTENSIONDIFFERENT;
    dwDialogError = 0;
    if (!lpof || lpof->lStructSize != sizeof(*lpof)) { dwDialogError = CDERR_STRUCTSIZE; return FALSE; }
    if (!lpof->lpstrFile || lpof->nMaxFile == 0 || !memchr(lpof->lpstrFile, 0, lpof->nMaxFile)) {
        dwDialogError = FNERR_INVALIDFILENAME; return FALSE;
    }
    if (!FUtf8Valid(lpof->lpstrFile, strlen(lpof->lpstrFile)) ||
        (lpof->lpstrInitialDir && !FUtf8Valid(lpof->lpstrInitialDir, strlen(lpof->lpstrInitialDir)))) {
        dwDialogError = FNERR_INVALIDFILENAME; return FALSE;
    }
    if ((lpof->Flags & ~grFlagsAllowed) || lpof->lpstrCustomFilter || lpof->lpfnHook || lpof->FlagsEx ||
        !SDL_IsMainThread() || !FCompatVideo()) { dwDialogError = CDERR_INITIALIZATION; SetLastError(ERROR_NOT_SUPPORTED); return FALSE; }
    if (lpof->hwndOwner && !IsWindow(lpof->hwndOwner)) { dwDialogError = CDERR_INITIALIZATION; SetLastError(ERROR_INVALID_WINDOW_HANDLE); return FALSE; }
    dlg.lpof = lpof; dlg.fSave = fSave;
    for (psz = lpof->lpstrFilter; psz && *psz; psz = pszPattern + strlen(pszPattern) + 1) {
        if (dlg.cFilter == MAX_FILE_FILTERS || SDL_strnlen(psz, 4097) > 4096) { dlg.dwError = CDERR_INITIALIZATION; goto done; }
        pszPattern = psz + strlen(psz) + 1;
        if (!*pszPattern || SDL_strnlen(pszPattern, 4097) > 4096) { dlg.dwError = CDERR_INITIALIZATION; goto done; }
        dlg.rgfilter[dlg.cFilter].pszName = strdup(psz); dlg.rgfilter[dlg.cFilter].pszPattern = strdup(pszPattern);
        dlg.cFilter++;
        if (!dlg.rgfilter[dlg.cFilter - 1].pszName || !dlg.rgfilter[dlg.cFilter - 1].pszPattern) { dlg.dwError = CDERR_MEMALLOCFAILURE; goto done; }
    }
    dlg.iFilter = lpof->nFilterIndex > 0 && lpof->nFilterIndex <= (DWORD)dlg.cFilter ? (int)lpof->nFilterIndex - 1 : 0;
    if (!PszNativeCwd(szCwd, sizeof(szCwd))) { dlg.dwError = CDERR_INITIALIZATION; goto done; }
    if (!FUtf8Valid(szCwd, strlen(szCwd))) { dlg.dwError = FNERR_INVALIDFILENAME; goto done; }
    strcpy(dlg.szDir, szCwd); szInitial[0] = 0;
    if (lpof->lpstrInitialDir && *lpof->lpstrInitialDir) {
        NATIVEFILEINFO st;
        if (!FJoinFile(szPath, szCwd, lpof->lpstrInitialDir) || !PszCanonicalNativePath(szPath, dlg.szDir, sizeof(dlg.szDir)) ||
            IGetNativeFileInfo(dlg.szDir, &st, TRUE) != 0 || !st.fDirectory) strcpy(dlg.szDir, szCwd);
    }
    if (*lpof->lpstrFile) {
        if (!FJoinFile(szPath, dlg.szDir, lpof->lpstrFile)) { dlg.dwError = FNERR_INVALIDFILENAME; goto done; }
        pch = strrchr(szPath, '/');
        if (pch) {
            SDL_strlcpy(szInitial, pch + 1, sizeof(szInitial));
            if (pch == szPath) pch[1] = 0; else *pch = 0;
            if (!PszCanonicalNativePath(szPath, dlg.szDir, sizeof(dlg.szDir))) { dlg.dwError = FNERR_INVALIDFILENAME; goto done; }
        }
    }
    if (!FUtf8Valid(dlg.szDir, strlen(dlg.szDir))) { dlg.dwError = FNERR_INVALIDFILENAME; goto done; }
    hwndFocus = GetFocus(); hwndRoot = lpof->hwndOwner ? HwndCompatRoot(lpof->hwndOwner) : NULL;
#if defined(STARS_FILE_DIALOG_PORTAL) || defined(STARS_FILE_DIALOG_COCOA) || defined(STARS_FILE_DIALOG_WINDOWS)
    fNative = !PszCompatEnvironment("STARS_FILE_DIALOG_BACKEND") || strcmp(PszCompatEnvironment("STARS_FILE_DIALOG_BACKEND"), "compat") != 0;
    if (fNative && hwndRoot) hwndRoot->cNativeFileDialogs++;
#endif
    fEnable = hwndRoot && IsWindowEnabled(hwndRoot);
    if (fEnable) EnableWindow(hwndRoot, FALSE);
    if (hwndRoot && !IsWindow(hwndRoot)) { dlg.dwError = CDERR_INITIALIZATION; goto restore; }
#if defined(STARS_FILE_DIALOG_PORTAL) || defined(STARS_FILE_DIALOG_COCOA) || defined(STARS_FILE_DIALOG_WINDOWS)
    /* Headless control tests opt into the compatibility browser explicitly. */
    if (fNative) {
        for (;;) {
            iRet = ICompatChooseFile(hwndRoot, fSave, lpof->lpstrTitle ? lpof->lpstrTitle : fSave ? "Save As" : "Open",
                                    dlg.szDir, szInitial, dlg.rgfilter, dlg.cFilter, &dlg.iFilter, szPath, sizeof(szPath));
            if (iRet < 0) {
                dlg.dwError = CDERR_INITIALIZATION;
                if (!hwndRoot || IsWindow(hwndRoot)) FileError(hwndRoot, "Unable to complete file selection. Please try again.");
                goto restore;
            }
            if (iRet == 0) goto restore;
            iRet = IAcceptFilePath(hwndRoot, &dlg, szPath);
            if (hwndRoot && !IsWindow(hwndRoot)) goto restore;
            if (dlg.dwError) {
                if (dlg.dwError == FNERR_BUFFERTOOSMALL)
                    FileError(hwndRoot, "The selected path is too long for this application. Please choose a shorter path.");
                goto restore;
            }
            if (iRet == 1) goto selected;
            if (iRet == 2) FileError(hwndRoot, "Select a regular file.");
            if (hwndRoot && !IsWindow(hwndRoot)) goto restore;
            RememberFileSelection(&dlg, szPath, szInitial);
        }
    }
#endif
    if (!SystemParametersInfoA(SPI_GETWORKAREA, 0, &work, 0)) { dlg.dwError = CDERR_INITIALIZATION; goto restore; }
    cx = StarsMin(640, work.right - work.left - 32); cy = StarsMin(440, work.bottom - work.top - 64);
    if (cx < 420 || cy < 320) { dlg.dwError = CDERR_INITIALIZATION; goto restore; }
    rc = (RECT){work.left + (work.right - work.left - cx) / 2, work.top + (work.bottom - work.top - cy) / 2, 0, 0};
    rc.right = rc.left + cx; rc.bottom = rc.top + cy;
    hwnd = HwndCompatDynamicDialog(lpof->lpstrTitle ? lpof->lpstrTitle : fSave ? "Save As" : "Open", WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, 0, &rc, hwndRoot, FileDialogProc);
    if (!hwnd) { dlg.dwError = CDERR_MEMALLOCFAILURE; goto restore; }
    if (!HwndFileControl(hwnd, "Static", "&Look in:", 0, 12, 14, 64, 22, 0x443) ||
        !HwndFileControl(hwnd, "Edit", dlg.szDir, WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 80, 12, cx - 92, 24, ID_FILE_PATH) ||
        !HwndFileControl(hwnd, "Button", "&Back", WS_TABSTOP, 12, 44, 64, 26, ID_FILE_BACK) ||
        !HwndFileControl(hwnd, "Button", "&Up", WS_TABSTOP, 84, 44, 64, 26, ID_FILE_UP) ||
        !HwndFileControl(hwnd, "Button", "&Home", WS_TABSTOP, 156, 44, 64, 26, ID_FILE_HOME) ||
        !HwndFileControl(hwnd, "Button", "New &folder...", WS_TABSTOP, 228, 44, 116, 26, ID_FILE_NEW) ||
        !HwndFileControl(hwnd, "Button", "Show &hidden files", WS_TABSTOP | BS_AUTOCHECKBOX, 12, 72, 160, 26, ID_FILE_HIDDEN) ||
        !HwndFileControl(hwnd, "ListBox", "", WS_TABSTOP | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT, 12, 42, cx - 24, cy - 182, ID_FILE_LIST) ||
        !HwndFileControl(hwnd, "Static", "File &name:", 0, 12, cy - 130, 80, 24, 0x441) ||
        !HwndFileControl(hwnd, "Edit", szInitial, WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 96, cy - 134, cx - 108, 26, ID_FILE_NAME) ||
        !HwndFileControl(hwnd, "Static", "File &type:", 0, 12, cy - 94, 80, 24, 0x442) ||
        !HwndFileControl(hwnd, "ComboBox", "", WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 96, cy - 98, cx - 108, 120, ID_FILE_FILTER) ||
        !HwndFileControl(hwnd, "Button", fSave ? "&Save" : "&Open", WS_TABSTOP | BS_DEFPUSHBUTTON, cx - 188, cy - 58, 80, 28, IDOK) ||
        !HwndFileControl(hwnd, "Button", "Cancel", WS_TABSTOP, cx - 96, cy - 58, 80, 28, IDCANCEL)) {
        dlg.dwError = CDERR_MEMALLOCFAILURE; goto restore;
    }
    SendDlgItemMessageA(hwnd, ID_FILE_NAME, EM_LIMITTEXT, PATH_MAX - 1, 0);
    for (i = 0; i < dlg.cFilter; i++) SendDlgItemMessageA(hwnd, ID_FILE_FILTER, CB_ADDSTRING, 0, (LPARAM)dlg.rgfilter[i].pszName);
    if (dlg.cFilter) SendDlgItemMessageA(hwnd, ID_FILE_FILTER, CB_SETCURSEL, dlg.iFilter, 0);
    SendMessageA(hwnd, WM_INITDIALOG, 0, (LPARAM)&dlg);
    EnableWindow(GetDlgItem(hwnd, ID_FILE_BACK), FALSE);
    SendDlgItemMessageA(hwnd, ID_FILE_PATH, EM_LIMITTEXT, PATH_MAX - 1, 0);
    LayoutFileDialog(hwnd);
    if (!FRefreshFiles(hwnd, &dlg)) goto restore;
    ShowWindow(hwnd, SW_SHOW); SetFocus(GetDlgItem(hwnd, ID_FILE_NAME));
    while (IsWindow(hwnd) && !hwnd->fEndDialog) {
        iRet = GetMessageA(&msg, NULL, 0, 0);
        if (iRet <= 0) { if (iRet == 0) PostQuitMessage((int)msg.wParam); break; }
        if (!IsDialogMessageA(hwnd, &msg)) { TranslateMessage(&msg); DispatchMessageA(&msg); }
    }
    if (!IsWindow(hwnd) || !hwnd->fEndDialog || hwnd->lDlgResult != IDOK) goto restore;
#if defined(STARS_FILE_DIALOG_PORTAL) || defined(STARS_FILE_DIALOG_COCOA) || defined(STARS_FILE_DIALOG_WINDOWS)
selected:
#endif
    {
        strcpy(szPath, dlg.szResult); pch = strrchr(szPath, '/');
        if (pch == szPath) pch[1] = 0; else *pch = 0;
        if (!(lpof->Flags & OFN_NOCHANGEDIR) && IChangeNativeCwd(szPath) != 0) { dlg.dwError = CDERR_INITIALIZATION; goto restore; }
        strcpy(lpof->lpstrFile, dlg.szResult);
        pch = strrchr(dlg.szResult, '/') + 1;
        lpof->nFileOffset = (WORD)(pch - dlg.szResult);
        if (lpof->lpstrFileTitle && lpof->nMaxFileTitle)
            CchCopyUtf8(lpof->lpstrFileTitle, lpof->nMaxFileTitle, pch, CchUtf8Prefix(pch, lpof->nMaxFileTitle - 1));
        pch = strrchr(pch, '.'); lpof->nFileExtension = pch ? (WORD)(pch - dlg.szResult + 1) : 0;
        lpof->nFilterIndex = dlg.cFilter ? (DWORD)dlg.iFilter + 1 : 0;
        lpof->Flags &= ~OFN_EXTENSIONDIFFERENT;
        if (lpof->lpstrDefExt && *lpof->lpstrDefExt && (!pch || SDL_strcasecmp(pch + 1, lpof->lpstrDefExt) != 0)) lpof->Flags |= OFN_EXTENSIONDIFFERENT;
        fRet = TRUE;
    }
restore:
    if (IsWindow(hwnd)) DestroyWindow(hwnd);
    if (fEnable && IsWindow(hwndRoot)) EnableWindow(hwndRoot, TRUE);
    if (fNative && IsWindow(hwndRoot)) hwndRoot->cNativeFileDialogs--;
    if (IsWindow(hwndFocus) && IsWindowEnabled(hwndFocus)) SetFocus(hwndFocus);
done:
    FreeFileEntries(&dlg);
    for (i = 0; i < dlg.cHistory; i++) free(dlg.rgpszHistory[i]);
    for (i = 0; i < dlg.cFilter; i++) { free(dlg.rgfilter[i].pszName); free(dlg.rgfilter[i].pszPattern); }
    dwDialogError = dlg.dwError;
    return fRet;
}
BOOL WINAPI GetOpenFileNameA(LPOPENFILENAMEA lpof) { return FFileDialog(lpof, FALSE); }
BOOL WINAPI GetSaveFileNameA(LPOPENFILENAMEA lpof) {
    BOOL fResult = FFileDialog(lpof, TRUE);
    if (!fResult && dwDialogError != 0)
        SDL_Log("Save dialog failed (common-dialog error %lu)", (unsigned long)dwDialogError);
    return fResult;
}
