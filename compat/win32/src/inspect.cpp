/* Logical-window inspection for the native UI and in-process tutorial harness. */
#include "internal.h"
#include <algorithm>
#include <limits>
#include <utility>
#include <iterator>
#include <stars_compat.h>
#include <commctrl.h>
#include <cinttypes>
#include <span>
#include <string>
#include <string_view>
#include <new>
#include <stdexcept>
#include "native.h"
#include "inspect_styles.h"

static std::uint64_t idInspector;
static HWND (*pfnObjectInspector)(void);
void StarsCompatSetObjectInspector(HWND (*pfnShow)(void)) {
    if (SDL_IsMainThread()) pfnObjectInspector = pfnShow;
}
static bool fInspectorClass;
static STARSCOMPATSNAPSHOT snapInspector;

static void AppendStyle(std::span<char> rgch, std::size_t &cch, std::string_view pszName) {
    if (rgch.empty()) return;
    if (cch != 0) cch += CchCopyUtf8Prefix(rgch.subspan(cch), " | ");
    cch += CchCopyUtf8Prefix(rgch.subspan(cch), pszName);
}

template<std::size_t c>
static void DecodeStyles(std::span<char> rgch, std::size_t &cch, DWORD *lpdw, const STYLEFIELD (&rg)[c]) {
    std::size_t i;
    DWORD dwFields = 0, dwOriginal = *lpdw;
    for (i = 0; i < c; i++) {
        if (!(dwFields & rg[i].dwMask) && (dwOriginal & rg[i].dwMask) == rg[i].dwValue) {
            AppendStyle(rgch, cch, rg[i].psz);
            dwFields |= rg[i].dwMask;
        }
    }
    *lpdw &= ~dwFields;
}

static std::size_t FormatStyles(const STARSCOMPATWINDOW *lp, BOOL fExtended, std::span<char> rgch) {
    DWORD dw = fExtended ? lp->dwExStyle : lp->dwStyle;
    char szUnknown[32];
    std::size_t cch = 0;
    int cchUnknown;
    if (rgch.empty()) return cch;
    rgch[0] = 0;
    if (fExtended) {
        DecodeStyles(rgch, cch, &dw, rgstyleExtended);
    } else {
        if (!(dw & (win32::WS_CHILD | win32::WS_POPUP))) AppendStyle(rgch, cch, "WS_OVERLAPPED");
        DecodeStyles(rgch, cch, &dw, rgstyleWindow);
        if (dw & win32::WS_GROUP) AppendStyle(rgch, cch, lp->dwStyle & win32::WS_CHILD ? "WS_GROUP" : "WS_MINIMIZEBOX");
        if (dw & win32::WS_TABSTOP) AppendStyle(rgch, cch, lp->dwStyle & win32::WS_CHILD ? "WS_TABSTOP" : "WS_MAXIMIZEBOX");
        dw &= ~(win32::WS_GROUP | win32::WS_TABSTOP);
        if (lp->fDialog) { DecodeStyles(rgch, cch, &dw, rgstyleDialog); }
        else switch (lp->iControl) {
        case 1: DecodeStyles(rgch, cch, &dw, rgstyleButton); break;
        case 2: DecodeStyles(rgch, cch, &dw, rgstyleStatic); break;
        case 3: DecodeStyles(rgch, cch, &dw, rgstyleEdit); break;
        case 4: DecodeStyles(rgch, cch, &dw, rgstyleList); break;
        case 5:
            DecodeStyles(rgch, cch, &dw, rgstyleScroll);
            if (dw & win32::SBS_TOPALIGN) AppendStyle(rgch, cch, lp->dwStyle & win32::SBS_SIZEBOX ?
                "SBS_SIZEBOXTOPLEFTALIGN" : lp->dwStyle & win32::SBS_VERT ? "SBS_LEFTALIGN" : "SBS_TOPALIGN");
            if (dw & win32::SBS_BOTTOMALIGN) AppendStyle(rgch, cch, lp->dwStyle & win32::SBS_SIZEBOX ?
                "SBS_SIZEBOXBOTTOMRIGHTALIGN" : lp->dwStyle & win32::SBS_VERT ? "SBS_RIGHTALIGN" : "SBS_BOTTOMALIGN");
            dw &= ~(win32::SBS_TOPALIGN | win32::SBS_BOTTOMALIGN);
            break;
        case 6: DecodeStyles(rgch, cch, &dw, rgstyleCombo); break;
        case 7: DecodeStyles(rgch, cch, &dw, rgstyleToolbar); break;
        }
    }
    if (dw) {
        cchUnknown = CchSnprintf(std::span{szUnknown}, "0x%08x", static_cast<unsigned>(dw));
        AppendStyle(rgch, cch, std::string_view{szUnknown, static_cast<std::size_t>(cchUnknown)});
    }
    if (cch == 0) cch = CchCopyUtf8Prefix(rgch, "0");
    return cch;
}

static std::string_view SzvControlKind(UINT iControl) {
    static constexpr std::string_view rgsz[] = { "window", "button", "static", "edit", "listbox", "scrollbar", "combobox", "toolbar" };
    return iControl < std::size(rgsz) ? rgsz[iControl] : "unknown";
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

std::optional<STARSCOMPATSNAPSHOT> StarsCompatCaptureHierarchy() {
    HWND lpwin;
    STARSCOMPATWINDOW item{};
    STARSCOMPATSNAPSHOT rgWindow;
    std::size_t c = 0;
    if (!SDL_IsMainThread()) return std::nullopt;
    for (lpwin = HwndCompatRegistry(); lpwin; lpwin = lpwin->lpNext)
        if (lpwin->fAlive && !lpwin->fDestroying) c++;
    try {
        rgWindow.reserve(c);
        // No SendMessage, enumeration callbacks or event pumping: a capture cannot
        // run an application's subclass procedure or observe a partially freed HWND.
        for (lpwin = HwndCompatRegistry(); lpwin; lpwin = lpwin->lpNext) {
            if (!lpwin->fAlive || lpwin->fDestroying) continue;
            item.id = lpwin->idInspect;
            item.idParent = StarsCompatWindowId(lpwin->hwndParent);
            item.idOwner = StarsCompatWindowId(lpwin->hwndOwner);
            item.idControl = lpwin->idChild;
            item.dwStyle = lpwin->dwStyle; item.dwExStyle = lpwin->dwExStyle;
            item.iControl = lpwin->iControl; item.fDialog = lpwin->fDialog;
            item.fVisible = win32::IsWindowVisible(lpwin); item.fEnabled = win32::IsWindowEnabled(lpwin);
            item.fFocused = win32::GetFocus() == lpwin; item.fCaptured = win32::GetCapture() == lpwin;
            if (!win32::GetWindowRect(lpwin, &item.rcWindow) || !win32::GetClientRect(lpwin, &item.rcClient))
                return std::nullopt;
            item.szClass = lpwin->lpclass->pszClass.get();
            item.szText = lpwin->iControl == 3 && (lpwin->dwStyle & win32::ES_PASSWORD) ?
                          "[password]" : lpwin->pszTitle;
            rgWindow.push_back(std::move(item));
            item = {};
        }
    } catch (const std::bad_alloc &) {
        return std::nullopt;
    } catch (const std::length_error &) {
        return std::nullopt;
    }
    std::reverse(rgWindow.begin(), rgWindow.end());
    return std::move(rgWindow);
}

static bool FWriteString(NATIVEFILE *lpfile, std::string_view szValue) {
    char rgb[4], *lpEnd;
    std::optional<UTF8CHAR> uch;
    if (ChPutNativeFile('"', lpfile) == EOF) return 0;
    while (!szValue.empty()) {
        uch = DecodeUtf8Char(szValue);
        if (!uch) return 0;
        szValue.remove_prefix(uch->cch);
        if (uch->ch == '"' || uch->ch == '\\') {
            if (ChPutNativeFile('\\', lpfile) == EOF || ChPutNativeFile(static_cast<int>(uch->ch), lpfile) == EOF) return 0;
        } else if (uch->ch < 32) {
            if (CchNativePrintf(lpfile, "\\u%04x", static_cast<unsigned>(uch->ch)) < 0) return 0;
        } else {
            lpEnd = SDL_UCS4ToUTF8(uch->ch, rgb);
            if (CWriteNative(std::span{rgb}.first(static_cast<std::size_t>((lpEnd - rgb))), lpfile) != static_cast<std::size_t>((lpEnd - rgb))) return 0;
        }
    }
    return ChPutNativeFile('"', lpfile) != EOF;
}

bool StarsCompatWriteHierarchy(const STARSCOMPATSNAPSHOT &rgWindow, NATIVEFILE *lpfile) {
    std::size_t i;
    const STARSCOMPATWINDOW *lp;
    char szStyle[2048], szExStyle[2048];
    if (!lpfile) return 0;
    if (IPutNativeString("{\"schema\":1,\"windows\":[\n", lpfile) == EOF) return 0;
    for (i = 0; i < rgWindow.size(); i++) {
        lp = &rgWindow[i];
        FormatStyles(lp, 0, std::span{szStyle});
        FormatStyles(lp, 1, std::span{szExStyle});
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
            !FWriteString(lpfile, lp->szClass) || IPutNativeString(",\"text\":", lpfile) == EOF ||
            !FWriteString(lpfile, lp->szText) || IPutNativeString(",\"styleNames\":", lpfile) == EOF ||
            !FWriteString(lpfile, szStyle) || IPutNativeString(",\"exStyleNames\":", lpfile) == EOF ||
            !FWriteString(lpfile, szExStyle) || IPutNativeString(",\"controlKindName\":", lpfile) == EOF ||
            !FWriteString(lpfile, SzvControlKind(lp->iControl)) || ChPutNativeFile('}', lpfile) == EOF) return 0;
    }
    return IPutNativeString("\n]}\n", lpfile) != EOF && !FNativeStreamError(lpfile);
}

static void InspectDetails(HWND hwnd) {
    HWND hwndList = win32::GetDlgItem(hwnd, 1);
    int i = static_cast<int>(win32::SendMessageA(hwndList, win32::LB_GETCURSEL, 0, 0));
    LRESULT iItem;
    const STARSCOMPATWINDOW *lp;
    std::string szDetails;
    char szStyle[2048], szExStyle[2048];
    std::size_t cb, cchStyle, cchExStyle;
    if (i == win32::LB_ERR) return;
    iItem = win32::SendMessageA(hwndList, win32::LB_GETITEMDATA, static_cast<WPARAM>(i), 0);
    if (iItem < 0 || static_cast<std::size_t>(iItem) >= snapInspector.size()) return;
    lp = &snapInspector[static_cast<std::size_t>(iItem)];
    cchStyle = FormatStyles(lp, 0, std::span{szStyle});
    cchExStyle = FormatStyles(lp, 1, std::span{szExStyle});
    cb = lp->szClass.size() + lp->szText.size() + cchStyle + cchExStyle;
    if (cb > std::numeric_limits<std::size_t>::max() - 640) return;
    try {
        szDetails.resize(cb + 640);
    } catch (const std::bad_alloc &) {
        return;
    } catch (const std::length_error &) {
        return;
    }
    CchSnprintf(std::span{szDetails},
        "ID: %" PRIu64 "\r\nParent: %" PRIu64 "\r\nOwner: %" PRIu64 "\r\nControl ID: %" PRIdPTR
        "\r\nClass: %s\r\nControl kind: %u (%s)\r\nDialog: %d\r\nVisible: %d\r\nEnabled: %d\r\nFocus: %d"
        "\r\nCapture: %d\r\nStyle: 0x%08x\r\n%s\r\nExStyle: 0x%08x\r\n%s"
        "\r\nWindow: %d,%d - %d,%d\r\nClient: %d,%d - %d,%d\r\n\r\nText:\r\n%s",
        lp->id, lp->idParent, lp->idOwner, static_cast<std::intptr_t>(lp->idControl), lp->szClass.c_str(), lp->iControl,
        SzvControlKind(lp->iControl),
        lp->fDialog, lp->fVisible, lp->fEnabled, lp->fFocused, lp->fCaptured,
        static_cast<unsigned>(lp->dwStyle), szStyle, static_cast<unsigned>(lp->dwExStyle), szExStyle,
        lp->rcWindow.left, lp->rcWindow.top, lp->rcWindow.right, lp->rcWindow.bottom,
        lp->rcClient.left, lp->rcClient.top, lp->rcClient.right, lp->rcClient.bottom, lp->szText.c_str());
    win32::SetWindowTextA(win32::GetDlgItem(hwnd, 2), szDetails.c_str());
}

static void InspectRows(HWND hwndList, std::uint64_t idParent, int cDepth) {
    std::size_t i;
    int iRow;
    char sz[512], szText[180], *lpch;
    std::size_t cch;
    std::string_view szSource;
    const STARSCOMPATWINDOW *lp;
    if (cDepth >= 64) return;
    for (i = 0; i < snapInspector.size(); i++) {
        lp = &snapInspector[i];
        if (lp->idParent != idParent) continue;
        szSource = lp->szText;
        cch = CchUtf8Prefix(szSource, sizeof(szText) - 1);
        std::copy_n(szSource.data(), cch, szText); szText[cch] = 0;
        for (lpch = szText; *lpch; lpch++) if (*lpch == '\r' || *lpch == '\n' || *lpch == '\t') *lpch = ' ';
        CchSnprintf(std::span{sz}, "%*s[%" PRIu64 "] %s #%" PRIdPTR " %s%s", cDepth * 2, "",
                 lp->id, lp->szClass.c_str(), static_cast<std::intptr_t>(lp->idControl), szText, lp->fVisible ? "" : " (hidden)");
        iRow = static_cast<int>(win32::SendMessageA(hwndList, win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(sz)));
        if (iRow >= 0) win32::SendMessageA(hwndList, win32::LB_SETITEMDATA, static_cast<WPARAM>(iRow), static_cast<LPARAM>(i));
        InspectRows(hwndList, lp->id, cDepth + 1);
    }
}

static bool RefreshInspector(HWND hwnd) {
    std::optional<STARSCOMPATSNAPSHOT> snap;
    HWND hwndList = win32::GetDlgItem(hwnd, 1);
    snap = StarsCompatCaptureHierarchy();
    if (!snap) return 0;
    win32::SendMessageA(hwndList, win32::LB_RESETCONTENT, 0, 0);
    snapInspector = std::move(*snap);
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
        idInspector = 0; snapInspector.clear(); return 0;
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
    snapInspector.clear();
}
