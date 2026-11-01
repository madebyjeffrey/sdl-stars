/* Logical-window inspection for the native UI and in-process tutorial harness. */
#include "internal.h"
#include <utility>
#include <stars_compat.h>
#include <commctrl.h>
#include <cinttypes>
#include <cstring>
#include "inspect_styles.h"

static std::uint64_t idInspector;
static HWND (*pfnObjectInspector)(void);
void StarsCompatSetObjectInspector(HWND (*pfnShow)(void)) {
    if (SDL_IsMainThread()) pfnObjectInspector = pfnShow;
}
static BOOL fInspectorClass;
static STARSCOMPATSNAPSHOT snapInspector;

static void AppendStyle(char *psz, std::size_t cb, const char *pszName) {
    if (*psz) SDL_strlcat(psz, " | ", cb);
    SDL_strlcat(psz, pszName, cb);
}

template<std::size_t c>
static void DecodeStyles(char *psz, std::size_t cb, DWORD *lpdw, const STYLEFIELD (&rg)[c]) {
    std::size_t i;
    DWORD dwFields = 0, dwOriginal = *lpdw;
    for (i = 0; i < c; i++) {
        if (!(dwFields & rg[i].dwMask) && (dwOriginal & rg[i].dwMask) == rg[i].dwValue) {
            AppendStyle(psz, cb, rg[i].psz);
            dwFields |= rg[i].dwMask;
        }
    }
    *lpdw &= ~dwFields;
}

static void FormatStyles(const STARSCOMPATWINDOW *lp, BOOL fExtended, char *psz, std::size_t cb) {
    DWORD dw = fExtended ? lp->dwExStyle : lp->dwStyle;
    char szUnknown[32];
    *psz = 0;
    if (fExtended) {
        DecodeStyles(psz, cb, &dw, rgstyleExtended);
    } else {
        if (!(dw & (win32::WS_CHILD | win32::WS_POPUP))) AppendStyle(psz, cb, "WS_OVERLAPPED");
        DecodeStyles(psz, cb, &dw, rgstyleWindow);
        if (dw & win32::WS_GROUP) AppendStyle(psz, cb, lp->dwStyle & win32::WS_CHILD ? "WS_GROUP" : "WS_MINIMIZEBOX");
        if (dw & win32::WS_TABSTOP) AppendStyle(psz, cb, lp->dwStyle & win32::WS_CHILD ? "WS_TABSTOP" : "WS_MAXIMIZEBOX");
        dw &= ~(win32::WS_GROUP | win32::WS_TABSTOP);
        if (lp->fDialog) { DecodeStyles(psz, cb, &dw, rgstyleDialog); }
        else switch (lp->iControl) {
        case 1: DecodeStyles(psz, cb, &dw, rgstyleButton); break;
        case 2: DecodeStyles(psz, cb, &dw, rgstyleStatic); break;
        case 3: DecodeStyles(psz, cb, &dw, rgstyleEdit); break;
        case 4: DecodeStyles(psz, cb, &dw, rgstyleList); break;
        case 5:
            DecodeStyles(psz, cb, &dw, rgstyleScroll);
            if (dw & win32::SBS_TOPALIGN) AppendStyle(psz, cb, lp->dwStyle & win32::SBS_SIZEBOX ?
                "SBS_SIZEBOXTOPLEFTALIGN" : lp->dwStyle & win32::SBS_VERT ? "SBS_LEFTALIGN" : "SBS_TOPALIGN");
            if (dw & win32::SBS_BOTTOMALIGN) AppendStyle(psz, cb, lp->dwStyle & win32::SBS_SIZEBOX ?
                "SBS_SIZEBOXBOTTOMRIGHTALIGN" : lp->dwStyle & win32::SBS_VERT ? "SBS_RIGHTALIGN" : "SBS_BOTTOMALIGN");
            dw &= ~(win32::SBS_TOPALIGN | win32::SBS_BOTTOMALIGN);
            break;
        case 6: DecodeStyles(psz, cb, &dw, rgstyleCombo); break;
        case 7: DecodeStyles(psz, cb, &dw, rgstyleToolbar); break;
        }
    }
    if (dw) {
        std::snprintf(szUnknown, sizeof(szUnknown), "0x%08x", static_cast<unsigned>(dw));
        AppendStyle(psz, cb, szUnknown);
    }
    if (!*psz) SDL_strlcpy(psz, "0", cb);
}

static const char *PszControlKind(UINT iControl) {
    static const char * const rgpsz[] = { "window", "button", "static", "edit", "listbox", "scrollbar", "combobox", "toolbar" };
    return iControl < SDL_arraysize(rgpsz) ? rgpsz[iControl] : "unknown";
}

std::uint64_t StarsCompatWindowId(HWND hwnd) {
    HWND lpwin;
    if (!SDL_IsMainThread()) return 0;
    lpwin = LpCompatWindow(hwnd);
    return lpwin && !lpwin->fDestroying ? lpwin->idInspect : 0;
}

HWND StarsCompatResolveWindow(std::uint64_t id) {
    HWND lpwin;
    if (!SDL_IsMainThread() || id == 0) return nullptr;
    for (lpwin = HwndCompatRegistry(); lpwin; lpwin = lpwin->lpNext)
        if (lpwin->fAlive && !lpwin->fDestroying && lpwin->idInspect == id) return lpwin;
    return nullptr;
}

std::uint64_t StarsCompatFindControl(std::uint64_t idParent, LONG_PTR idControl, LPCSTR pszClass) {
    HWND hwndParent = StarsCompatResolveWindow(idParent), lpwin;
    std::uint64_t id = 0;
    if (!hwndParent) return 0;
    for (lpwin = hwndParent->hwndFirstChild; lpwin; lpwin = lpwin->hwndZNext)
        if (lpwin->fAlive && !lpwin->fDestroying && lpwin->idChild == idControl &&
            (!pszClass || SDL_strcasecmp(pszClass, lpwin->lpclass->pszClass.get()) == 0)) {
            if (id != 0) return 0;
            id = lpwin->idInspect;
        }
    return id;
}

void StarsCompatFreeHierarchy(STARSCOMPATSNAPSHOT *lpsnap) {
    std::size_t i;
    if (!lpsnap) return;
    for (i = 0; i < lpsnap->cWindow; i++) {
        SDL_free(lpsnap->rgWindow[i].pszClass);
        SDL_free(lpsnap->rgWindow[i].pszText);
    }
    SDL_free(lpsnap->rgWindow);
    *lpsnap = {};
}

BOOL StarsCompatCaptureHierarchy(STARSCOMPATSNAPSHOT *lpsnap) {
    HWND lpwin;
    STARSCOMPATWINDOW *lpitem;
    std::size_t c = 0, i;
    if (!SDL_IsMainThread() || !lpsnap || lpsnap->rgWindow || lpsnap->cWindow != 0) return 0;
    for (lpwin = HwndCompatRegistry(); lpwin; lpwin = lpwin->lpNext)
        if (lpwin->fAlive && !lpwin->fDestroying) c++;
    if (c == 0) return 1;
    if (c > SIZE_MAX / sizeof(*lpitem)) return 0;
    lpsnap->rgWindow = static_cast<STARSCOMPATWINDOW *>((SDL_calloc(c, sizeof(*lpitem))));
    if (!lpsnap->rgWindow) return 0;
    lpsnap->cWindow = c;
    i = c;
    // No SendMessage, enumeration callbacks or event pumping: a capture cannot
    // run an application's subclass procedure or observe a partially freed HWND.
    for (lpwin = HwndCompatRegistry(); lpwin; lpwin = lpwin->lpNext) {
        if (!lpwin->fAlive || lpwin->fDestroying) continue;
        lpitem = &lpsnap->rgWindow[--i];
        lpitem->id = lpwin->idInspect;
        lpitem->idParent = StarsCompatWindowId(lpwin->hwndParent);
        lpitem->idOwner = StarsCompatWindowId(lpwin->hwndOwner);
        lpitem->idControl = lpwin->idChild;
        lpitem->dwStyle = lpwin->dwStyle; lpitem->dwExStyle = lpwin->dwExStyle;
        lpitem->iControl = lpwin->iControl; lpitem->fDialog = lpwin->fDialog;
        lpitem->fVisible = win32::IsWindowVisible(lpwin); lpitem->fEnabled = win32::IsWindowEnabled(lpwin);
        lpitem->fFocused = win32::GetFocus() == lpwin; lpitem->fCaptured = win32::GetCapture() == lpwin;
        lpitem->fUtf8Text = 1;
        lpitem->pszClass = SDL_strdup(lpwin->lpclass->pszClass.get());
        lpitem->pszText = SDL_strdup(lpwin->iControl == 3 && (lpwin->dwStyle & win32::ES_PASSWORD) ?
                                    "[password]" : lpwin->pszTitle);
        if (!lpitem->pszClass || !lpitem->pszText || !win32::GetWindowRect(lpwin, &lpitem->rcWindow) ||
            !win32::GetClientRect(lpwin, &lpitem->rcClient)) {
            StarsCompatFreeHierarchy(lpsnap);
            return 0;
        }
    }
    return 1;
}

static BOOL FWriteString(NATIVEFILE *lpfile, const char *psz, BOOL fUtf8) {
    const BYTE *lpch = reinterpret_cast<const BYTE *>(psz);
    char rgb[4], *lpEnd;
    DWORD ch;
    std::size_t cb = std::strlen(psz);
    int c;
    static_cast<void>(fUtf8);
    if (ChPutNativeFile('"', lpfile) == EOF) return 0;
    while (*lpch) {
        c = CbUtf8Char(reinterpret_cast<const char *>(lpch), cb, &ch);
        if (c == 0) return 0;
        lpch += c; cb -= c;
        if (ch == '"' || ch == '\\') {
            if (ChPutNativeFile('\\', lpfile) == EOF || ChPutNativeFile(static_cast<int>(ch), lpfile) == EOF) return 0;
        } else if (ch < 32) {
            if (CchNativePrintf(lpfile, "\\u%04x", static_cast<unsigned>(ch)) < 0) return 0;
        } else {
            lpEnd = SDL_UCS4ToUTF8(ch, rgb);
            if (CWriteNative(std::span{rgb}.first(static_cast<std::size_t>((lpEnd - rgb))), lpfile) != static_cast<std::size_t>((lpEnd - rgb))) return 0;
        }
    }
    return ChPutNativeFile('"', lpfile) != EOF;
}

BOOL StarsCompatWriteHierarchy(const STARSCOMPATSNAPSHOT *lpsnap, NATIVEFILE *lpfile) {
    std::size_t i;
    const STARSCOMPATWINDOW *lp;
    char szStyle[2048], szExStyle[2048];
    if (!lpsnap || !lpfile || (lpsnap->cWindow != 0 && !lpsnap->rgWindow)) return 0;
    if (IPutNativeString("{\"schema\":1,\"windows\":[\n", lpfile) == EOF) return 0;
    for (i = 0; i < lpsnap->cWindow; i++) {
        lp = &lpsnap->rgWindow[i];
        FormatStyles(lp, 0, szStyle, sizeof(szStyle));
        FormatStyles(lp, 1, szExStyle, sizeof(szExStyle));
        if (CchNativePrintf(lpfile, "%s{\"id\":\"%" PRIu64 "\",\"parent\":\"%" PRIu64
                    "\",\"owner\":\"%" PRIu64 "\",\"controlId\":\"%" PRIdPTR
                    "\",\"style\":%u,\"exStyle\":%u,\"controlKind\":%u,\"dialog\":%s,"
                    "\"visible\":%s,\"enabled\":%s,\"focused\":%s,\"captured\":%s,"
                    "\"windowRect\":[%d,%d,%d,%d],\"clientRect\":[%d,%d,%d,%d],\"class\":",
                    i != 0 ? ",\n" : "", lp->id, lp->idParent, lp->idOwner, static_cast<std::intptr_t>(lp->idControl),
                    static_cast<unsigned>(lp->dwStyle), static_cast<unsigned>(lp->dwExStyle), lp->iControl,
                    lp->fDialog ? "true" : "false", lp->fVisible ? "true" : "false",
                    lp->fEnabled ? "true" : "false", lp->fFocused ? "true" : "false", lp->fCaptured ? "true" : "false",
                    lp->rcWindow.left, lp->rcWindow.top, lp->rcWindow.right, lp->rcWindow.bottom,
                    lp->rcClient.left, lp->rcClient.top, lp->rcClient.right, lp->rcClient.bottom) < 0 ||
            !FWriteString(lpfile, lp->pszClass, 0) || IPutNativeString(",\"text\":", lpfile) == EOF ||
            !FWriteString(lpfile, lp->pszText, lp->fUtf8Text) || IPutNativeString(",\"styleNames\":", lpfile) == EOF ||
            !FWriteString(lpfile, szStyle, 0) || IPutNativeString(",\"exStyleNames\":", lpfile) == EOF ||
            !FWriteString(lpfile, szExStyle, 0) || IPutNativeString(",\"controlKindName\":", lpfile) == EOF ||
            !FWriteString(lpfile, PszControlKind(lp->iControl), 0) || ChPutNativeFile('}', lpfile) == EOF) return 0;
    }
    return IPutNativeString("\n]}\n", lpfile) != EOF && !FNativeStreamError(lpfile);
}

static void InspectDetails(HWND hwnd) {
    HWND hwndList = win32::GetDlgItem(hwnd, 1);
    int i = static_cast<int>(win32::SendMessageA(hwndList, win32::LB_GETCURSEL, 0, 0));
    LRESULT iItem;
    const STARSCOMPATWINDOW *lp;
    char *psz;
    char szStyle[2048], szExStyle[2048];
    std::size_t cb;
    if (i == win32::LB_ERR) return;
    iItem = win32::SendMessageA(hwndList, win32::LB_GETITEMDATA, static_cast<WPARAM>(i), 0);
    if (iItem < 0 || static_cast<std::size_t>(iItem) >= snapInspector.cWindow) return;
    lp = &snapInspector.rgWindow[iItem];
    FormatStyles(lp, 0, szStyle, sizeof(szStyle));
    FormatStyles(lp, 1, szExStyle, sizeof(szExStyle));
    cb = std::strlen(lp->pszClass) + std::strlen(lp->pszText) + std::strlen(szStyle) + std::strlen(szExStyle);
    if (cb > SIZE_MAX - 640 || !(psz = static_cast<char *>((SDL_malloc(cb + 640))))) return;
    NATIVEOWNER<char, SDL_free> pszOwner(psz);
    std::snprintf(psz, cb + 640,
        "ID: %" PRIu64 "\r\nParent: %" PRIu64 "\r\nOwner: %" PRIu64 "\r\nControl ID: %" PRIdPTR
        "\r\nClass: %s\r\nControl kind: %u (%s)\r\nDialog: %d\r\nVisible: %d\r\nEnabled: %d\r\nFocus: %d"
        "\r\nCapture: %d\r\nStyle: 0x%08x\r\n%s\r\nExStyle: 0x%08x\r\n%s"
        "\r\nWindow: %d,%d - %d,%d\r\nClient: %d,%d - %d,%d\r\n\r\nText:\r\n%s",
        lp->id, lp->idParent, lp->idOwner, static_cast<std::intptr_t>(lp->idControl), lp->pszClass, lp->iControl,
        PszControlKind(lp->iControl),
        lp->fDialog, lp->fVisible, lp->fEnabled, lp->fFocused, lp->fCaptured,
        static_cast<unsigned>(lp->dwStyle), szStyle, static_cast<unsigned>(lp->dwExStyle), szExStyle,
        lp->rcWindow.left, lp->rcWindow.top, lp->rcWindow.right, lp->rcWindow.bottom,
        lp->rcClient.left, lp->rcClient.top, lp->rcClient.right, lp->rcClient.bottom, lp->pszText);
    win32::SetWindowTextA(win32::GetDlgItem(hwnd, 2), psz);
}

static void InspectRows(HWND hwndList, std::uint64_t idParent, int cDepth) {
    std::size_t i;
    int iRow;
    char sz[512], szText[180], *lpch, *pszUtf;
    std::size_t cb, cch;
    const STARSCOMPATWINDOW *lp;
    if (cDepth >= 64) return;
    for (i = 0; i < snapInspector.cWindow; i++) {
        lp = &snapInspector.rgWindow[i];
        if (lp->idParent != idParent) continue;
        cb = std::strlen(lp->pszText);
        if (cb > (SIZE_MAX - 1) / 3) continue;
        pszUtf = static_cast<char *>((SDL_malloc(cb * 3 + 1)));
        NATIVEOWNER<char, SDL_free> pszOwner(pszUtf);
        if (!pszUtf) continue;
        std::memcpy(pszUtf, lp->pszText, cb + 1);
        cch = CchUtf8Prefix(pszUtf, sizeof(szText) - 1);
        std::memcpy(szText, pszUtf, cch); szText[cch] = 0; pszOwner.reset();
        for (lpch = szText; *lpch; lpch++) if (*lpch == '\r' || *lpch == '\n' || *lpch == '\t') *lpch = ' ';
        std::snprintf(sz, sizeof(sz), "%*s[%" PRIu64 "] %s #%" PRIdPTR " %s%s", cDepth * 2, "",
                 lp->id, lp->pszClass, static_cast<std::intptr_t>(lp->idControl), szText, lp->fVisible ? "" : " (hidden)");
        iRow = static_cast<int>(win32::SendMessageA(hwndList, win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(sz)));
        if (iRow >= 0) win32::SendMessageA(hwndList, win32::LB_SETITEMDATA, static_cast<WPARAM>(iRow), static_cast<LPARAM>(i));
        InspectRows(hwndList, lp->id, cDepth + 1);
    }
}

static BOOL RefreshInspector(HWND hwnd) {
    STARSCOMPATSNAPSHOT snap = {};
    HWND hwndList = win32::GetDlgItem(hwnd, 1);
    NATIVECLEANUPSCOPE snapshotOwner([&]() noexcept { if (snap.rgWindow) StarsCompatFreeHierarchy(&snap); });
    if (!StarsCompatCaptureHierarchy(&snap)) return 0;
    win32::SendMessageA(hwndList, win32::LB_RESETCONTENT, 0, 0);
    StarsCompatFreeHierarchy(&snapInspector);
    snapInspector = std::exchange(snap, STARSCOMPATSNAPSHOT{});
    InspectRows(hwndList, 0, 0);
    win32::SendMessageA(hwndList, win32::LB_SETCURSEL, 0, 0);
    InspectDetails(hwnd);
    return 1;
}

static LRESULT  InspectorProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    RECT rc;
    HBRUSH hbr;
    int cxList, fPainted;
    switch (msg) {
    case win32::WM_CREATE:
        if (!win32::CreateWindowExA(0, "ListBox", "", win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_BORDER | win32::WS_TABSTOP | win32::WS_VSCROLL | win32::LBS_NOTIFY,
                           8, 40, 400, 500, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(1)), nullptr, nullptr) ||
            !win32::CreateWindowExA(0, "Edit", "", win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_BORDER | win32::WS_TABSTOP |
                           win32::ES_READONLY | win32::ES_MULTILINE | win32::ES_AUTOVSCROLL | win32::ES_AUTOHSCROLL,
                           420, 40, 360, 500, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(2)), nullptr, nullptr) ||
            !win32::CreateWindowExA(0, "Button", "Refresh", win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP | win32::BS_PUSHBUTTON,
                           8, 8, 100, 24, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(3)), nullptr, nullptr)) return -1;
        if (pfnObjectInspector) win32::CreateWindowExA(0, "Button", "Map objects", win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                                              120, 8, 110, 24, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(4)), nullptr, nullptr);
        return 0;
    case win32::WM_ERASEBKGND:
        if (!win32::GetClientRect(hwnd, &rc) || !(hbr = win32::CreateSolidBrush(win32::GetSysColor(win32::COLOR_BTNFACE)))) return 0;
        fPainted = win32::FillRect(reinterpret_cast<HDC>(w), &rc, hbr);
        win32::DeleteObject(hbr);
        return fPainted;
    case win32::WM_SIZE:
        win32::GetClientRect(hwnd, &rc); cxList = StarsMax(0, rc.right / 2 - 12);
        win32::MoveWindow(win32::GetDlgItem(hwnd, 1), 8, 40, cxList, StarsMax(0, rc.bottom - 48), 1);
        win32::MoveWindow(win32::GetDlgItem(hwnd, 2), rc.right / 2, 40, StarsMax(0, rc.right / 2 - 8), StarsMax(0, rc.bottom - 48), 1);
        return 0;
    case win32::WM_COMMAND:
        if (LOWORD(w) == 3 && HIWORD(w) == win32::BN_CLICKED) RefreshInspector(hwnd);
        else if (LOWORD(w) == 4 && HIWORD(w) == win32::BN_CLICKED && pfnObjectInspector) CompatInvokeInspector(pfnObjectInspector, hwnd);
        else if (LOWORD(w) == 1 && HIWORD(w) == win32::LBN_SELCHANGE) InspectDetails(hwnd);
        return 0;
    case win32::WM_DESTROY:
        idInspector = 0; StarsCompatFreeHierarchy(&snapInspector); return 0;
    default: return win32::DefWindowProcA(hwnd, msg, w, l);
    }
}

HWND StarsCompatShowInspector(void) {
    HWND hwnd;
    WNDCLASSA wc = {};
    if (!SDL_IsMainThread()) return nullptr;
    hwnd = StarsCompatResolveWindow(idInspector);
    if (!hwnd) {
        wc.lpfnWndProc = InspectorProc; wc.lpszClassName = "StarsCompatInspector";
        if (!fInspectorClass) {
            if (!win32::RegisterClassA(&wc)) return nullptr;
            fInspectorClass = 1;
        }
        hwnd = win32::CreateWindowExA(0, wc.lpszClassName, "Stars window inspector", win32::WS_OVERLAPPEDWINDOW,
                             win32::CW_USEDEFAULT, win32::CW_USEDEFAULT, 900, 600, nullptr, nullptr, nullptr, nullptr);
        if (!hwnd) return nullptr;
        idInspector = StarsCompatWindowId(hwnd);
    }
    if (!RefreshInspector(hwnd)) { win32::DestroyWindow(hwnd); return nullptr; }
    win32::ShowWindow(hwnd, win32::SW_SHOWNORMAL);
    win32::SetFocus(win32::GetDlgItem(hwnd, 1));
    return hwnd;
}

void CompatShutdownInspector(void) {
    idInspector = 0; fInspectorClass = 0; pfnObjectInspector = nullptr;
    StarsCompatFreeHierarchy(&snapInspector);
}
