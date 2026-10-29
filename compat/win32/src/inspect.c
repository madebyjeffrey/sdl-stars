/* Logical-window inspection for the native UI and in-process tutorial harness. */
#include "internal.h"
#include <stars_compat.h>
#include <commctrl.h>
#include <inttypes.h>
#include <string.h>
#include "inspect_styles.h"

static uint64_t idInspector;
static HWND (*pfnObjectInspector)(void);
void StarsCompatSetObjectInspector(HWND (*pfnShow)(void)) {
    if (SDL_IsMainThread()) pfnObjectInspector = pfnShow;
}
static BOOL fInspectorClass;
static STARSCOMPATSNAPSHOT snapInspector;

static void AppendStyle(char *psz, size_t cb, const char *pszName) {
    if (*psz) SDL_strlcat(psz, " | ", cb);
    SDL_strlcat(psz, pszName, cb);
}

static void DecodeStyles(char *psz, size_t cb, DWORD *lpdw, const STYLEFIELD *rg, size_t c) {
    size_t i;
    DWORD dwFields = 0, dwOriginal = *lpdw;
    for (i = 0; i < c; i++) {
        if (!(dwFields & rg[i].dwMask) && (dwOriginal & rg[i].dwMask) == rg[i].dwValue) {
            AppendStyle(psz, cb, rg[i].psz);
            dwFields |= rg[i].dwMask;
        }
    }
    *lpdw &= ~dwFields;
}

static void FormatStyles(const STARSCOMPATWINDOW *lp, BOOL fExtended, char *psz, size_t cb) {
    DWORD dw = fExtended ? lp->dwExStyle : lp->dwStyle;
    char szUnknown[32];
    *psz = 0;
#define DECODE(kind) DecodeStyles(psz, cb, &dw, rgstyle##kind, SDL_arraysize(rgstyle##kind))
    if (fExtended) {
        DECODE(Extended);
    } else {
        if (!(dw & (WS_CHILD | WS_POPUP))) AppendStyle(psz, cb, "WS_OVERLAPPED");
        DECODE(Window);
        if (dw & WS_GROUP) AppendStyle(psz, cb, lp->dwStyle & WS_CHILD ? "WS_GROUP" : "WS_MINIMIZEBOX");
        if (dw & WS_TABSTOP) AppendStyle(psz, cb, lp->dwStyle & WS_CHILD ? "WS_TABSTOP" : "WS_MAXIMIZEBOX");
        dw &= ~(WS_GROUP | WS_TABSTOP);
        if (lp->fDialog) { DECODE(Dialog); }
        else switch (lp->iControl) {
        case 1: DECODE(Button); break;
        case 2: DECODE(Static); break;
        case 3: DECODE(Edit); break;
        case 4: DECODE(List); break;
        case 5:
            DECODE(Scroll);
            if (dw & SBS_TOPALIGN) AppendStyle(psz, cb, lp->dwStyle & SBS_SIZEBOX ?
                "SBS_SIZEBOXTOPLEFTALIGN" : lp->dwStyle & SBS_VERT ? "SBS_LEFTALIGN" : "SBS_TOPALIGN");
            if (dw & SBS_BOTTOMALIGN) AppendStyle(psz, cb, lp->dwStyle & SBS_SIZEBOX ?
                "SBS_SIZEBOXBOTTOMRIGHTALIGN" : lp->dwStyle & SBS_VERT ? "SBS_RIGHTALIGN" : "SBS_BOTTOMALIGN");
            dw &= ~(SBS_TOPALIGN | SBS_BOTTOMALIGN);
            break;
        case 6: DECODE(Combo); break;
        case 7: DECODE(Toolbar); break;
        }
    }
#undef DECODE
    if (dw) {
        snprintf(szUnknown, sizeof(szUnknown), "0x%08x", (unsigned)dw);
        AppendStyle(psz, cb, szUnknown);
    }
    if (!*psz) SDL_strlcpy(psz, "0", cb);
}

static const char *PszControlKind(UINT iControl) {
    static const char * const rgpsz[] = { "window", "button", "static", "edit", "listbox", "scrollbar", "combobox", "toolbar" };
    return iControl < SDL_arraysize(rgpsz) ? rgpsz[iControl] : "unknown";
}

uint64_t StarsCompatWindowId(HWND hwnd) {
    HWND lpwin;
    if (!SDL_IsMainThread()) return 0;
    lpwin = LpCompatWindow(hwnd);
    return lpwin && !lpwin->fDestroying ? lpwin->idInspect : 0;
}

HWND StarsCompatResolveWindow(uint64_t id) {
    HWND lpwin;
    if (!SDL_IsMainThread() || id == 0) return NULL;
    for (lpwin = HwndCompatRegistry(); lpwin; lpwin = lpwin->lpNext)
        if (lpwin->fAlive && !lpwin->fDestroying && lpwin->idInspect == id) return lpwin;
    return NULL;
}

uint64_t StarsCompatFindControl(uint64_t idParent, LONG_PTR idControl, LPCSTR pszClass) {
    HWND hwndParent = StarsCompatResolveWindow(idParent), lpwin;
    uint64_t id = 0;
    if (!hwndParent) return 0;
    for (lpwin = hwndParent->hwndFirstChild; lpwin; lpwin = lpwin->hwndZNext)
        if (lpwin->fAlive && !lpwin->fDestroying && lpwin->idChild == idControl &&
            (!pszClass || SDL_strcasecmp(pszClass, lpwin->lpclass->pszClass) == 0)) {
            if (id != 0) return 0;
            id = lpwin->idInspect;
        }
    return id;
}

void StarsCompatFreeHierarchy(STARSCOMPATSNAPSHOT *lpsnap) {
    size_t i;
    if (!lpsnap) return;
    for (i = 0; i < lpsnap->cWindow; i++) {
        SDL_free(lpsnap->rgWindow[i].pszClass);
        SDL_free(lpsnap->rgWindow[i].pszText);
    }
    SDL_free(lpsnap->rgWindow);
    memset(lpsnap, 0, sizeof(*lpsnap));
}

BOOL StarsCompatCaptureHierarchy(STARSCOMPATSNAPSHOT *lpsnap) {
    HWND lpwin;
    STARSCOMPATWINDOW *lpitem;
    size_t c = 0, i;
    if (!SDL_IsMainThread() || !lpsnap || lpsnap->rgWindow || lpsnap->cWindow != 0) return FALSE;
    for (lpwin = HwndCompatRegistry(); lpwin; lpwin = lpwin->lpNext)
        if (lpwin->fAlive && !lpwin->fDestroying) c++;
    if (c == 0) return TRUE;
    if (c > SIZE_MAX / sizeof(*lpitem)) return FALSE;
    lpsnap->rgWindow = (STARSCOMPATWINDOW *)(SDL_calloc(c, sizeof(*lpitem)));
    if (!lpsnap->rgWindow) return FALSE;
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
        lpitem->fVisible = IsWindowVisible(lpwin); lpitem->fEnabled = IsWindowEnabled(lpwin);
        lpitem->fFocused = GetFocus() == lpwin; lpitem->fCaptured = GetCapture() == lpwin;
        lpitem->fUtf8Text = TRUE;
        lpitem->pszClass = SDL_strdup(lpwin->lpclass->pszClass);
        lpitem->pszText = SDL_strdup(lpwin->iControl == 3 && (lpwin->dwStyle & ES_PASSWORD) ?
                                    "[password]" : lpwin->pszTitle);
        if (!lpitem->pszClass || !lpitem->pszText || !GetWindowRect(lpwin, &lpitem->rcWindow) ||
            !GetClientRect(lpwin, &lpitem->rcClient)) {
            StarsCompatFreeHierarchy(lpsnap);
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL FWriteString(NATIVEFILE *lpfile, const char *psz, BOOL fUtf8) {
    const BYTE *lpch = (const BYTE *)psz;
    char rgb[4], *lpEnd;
    DWORD ch;
    size_t cb = strlen(psz);
    int c;
    (void)fUtf8;
    if (ChPutNativeFile('"', lpfile) == EOF) return FALSE;
    while (*lpch) {
        c = CbUtf8Char((const char *)lpch, cb, &ch);
        if (c == 0) return FALSE;
        lpch += c; cb -= c;
        if (ch == '"' || ch == '\\') {
            if (ChPutNativeFile('\\', lpfile) == EOF || ChPutNativeFile((int)ch, lpfile) == EOF) return FALSE;
        } else if (ch < 32) {
            if (CchNativePrintf(lpfile, "\\u%04x", (unsigned)ch) < 0) return FALSE;
        } else {
            lpEnd = SDL_UCS4ToUTF8(ch, rgb);
            if (CWriteNative(rgb, 1, (size_t)(lpEnd - rgb), lpfile) != (size_t)(lpEnd - rgb)) return FALSE;
        }
    }
    return ChPutNativeFile('"', lpfile) != EOF;
}

BOOL StarsCompatWriteHierarchy(const STARSCOMPATSNAPSHOT *lpsnap, NATIVEFILE *lpfile) {
    size_t i;
    const STARSCOMPATWINDOW *lp;
    char szStyle[2048], szExStyle[2048];
    if (!lpsnap || !lpfile || (lpsnap->cWindow != 0 && !lpsnap->rgWindow)) return FALSE;
    if (IPutNativeString("{\"schema\":1,\"windows\":[\n", lpfile) == EOF) return FALSE;
    for (i = 0; i < lpsnap->cWindow; i++) {
        lp = &lpsnap->rgWindow[i];
        FormatStyles(lp, FALSE, szStyle, sizeof(szStyle));
        FormatStyles(lp, TRUE, szExStyle, sizeof(szExStyle));
        if (CchNativePrintf(lpfile, "%s{\"id\":\"%" PRIu64 "\",\"parent\":\"%" PRIu64
                    "\",\"owner\":\"%" PRIu64 "\",\"controlId\":\"%" PRIdPTR
                    "\",\"style\":%u,\"exStyle\":%u,\"controlKind\":%u,\"dialog\":%s,"
                    "\"visible\":%s,\"enabled\":%s,\"focused\":%s,\"captured\":%s,"
                    "\"windowRect\":[%d,%d,%d,%d],\"clientRect\":[%d,%d,%d,%d],\"class\":",
                    i != 0 ? ",\n" : "", lp->id, lp->idParent, lp->idOwner, (intptr_t)lp->idControl,
                    (unsigned)lp->dwStyle, (unsigned)lp->dwExStyle, lp->iControl,
                    lp->fDialog ? "true" : "false", lp->fVisible ? "true" : "false",
                    lp->fEnabled ? "true" : "false", lp->fFocused ? "true" : "false", lp->fCaptured ? "true" : "false",
                    lp->rcWindow.left, lp->rcWindow.top, lp->rcWindow.right, lp->rcWindow.bottom,
                    lp->rcClient.left, lp->rcClient.top, lp->rcClient.right, lp->rcClient.bottom) < 0 ||
            !FWriteString(lpfile, lp->pszClass, FALSE) || IPutNativeString(",\"text\":", lpfile) == EOF ||
            !FWriteString(lpfile, lp->pszText, lp->fUtf8Text) || IPutNativeString(",\"styleNames\":", lpfile) == EOF ||
            !FWriteString(lpfile, szStyle, FALSE) || IPutNativeString(",\"exStyleNames\":", lpfile) == EOF ||
            !FWriteString(lpfile, szExStyle, FALSE) || IPutNativeString(",\"controlKindName\":", lpfile) == EOF ||
            !FWriteString(lpfile, PszControlKind(lp->iControl), FALSE) || ChPutNativeFile('}', lpfile) == EOF) return FALSE;
    }
    return IPutNativeString("\n]}\n", lpfile) != EOF && !FNativeStreamError(lpfile);
}

static void InspectDetails(HWND hwnd) {
    HWND hwndList = GetDlgItem(hwnd, 1);
    int i = (int)SendMessageA(hwndList, LB_GETCURSEL, 0, 0);
    LRESULT iItem;
    const STARSCOMPATWINDOW *lp;
    char *psz;
    char szStyle[2048], szExStyle[2048];
    size_t cb;
    if (i == LB_ERR) return;
    iItem = SendMessageA(hwndList, LB_GETITEMDATA, (WPARAM)i, 0);
    if (iItem < 0 || (size_t)iItem >= snapInspector.cWindow) return;
    lp = &snapInspector.rgWindow[iItem];
    FormatStyles(lp, FALSE, szStyle, sizeof(szStyle));
    FormatStyles(lp, TRUE, szExStyle, sizeof(szExStyle));
    cb = strlen(lp->pszClass) + strlen(lp->pszText) + strlen(szStyle) + strlen(szExStyle);
    if (cb > SIZE_MAX - 640 || !(psz = (char *)(SDL_malloc(cb + 640)))) return;
    snprintf(psz, cb + 640,
        "ID: %" PRIu64 "\r\nParent: %" PRIu64 "\r\nOwner: %" PRIu64 "\r\nControl ID: %" PRIdPTR
        "\r\nClass: %s\r\nControl kind: %u (%s)\r\nDialog: %d\r\nVisible: %d\r\nEnabled: %d\r\nFocus: %d"
        "\r\nCapture: %d\r\nStyle: 0x%08x\r\n%s\r\nExStyle: 0x%08x\r\n%s"
        "\r\nWindow: %d,%d - %d,%d\r\nClient: %d,%d - %d,%d\r\n\r\nText:\r\n%s",
        lp->id, lp->idParent, lp->idOwner, (intptr_t)lp->idControl, lp->pszClass, lp->iControl,
        PszControlKind(lp->iControl),
        lp->fDialog, lp->fVisible, lp->fEnabled, lp->fFocused, lp->fCaptured,
        (unsigned)lp->dwStyle, szStyle, (unsigned)lp->dwExStyle, szExStyle,
        lp->rcWindow.left, lp->rcWindow.top, lp->rcWindow.right, lp->rcWindow.bottom,
        lp->rcClient.left, lp->rcClient.top, lp->rcClient.right, lp->rcClient.bottom, lp->pszText);
    SetWindowTextA(GetDlgItem(hwnd, 2), psz);
    SDL_free(psz);
}

static void InspectRows(HWND hwndList, uint64_t idParent, int cDepth) {
    size_t i;
    int iRow;
    char sz[512], szText[180], *lpch, *pszUtf;
    size_t cb, cch;
    const STARSCOMPATWINDOW *lp;
    if (cDepth >= 64) return;
    for (i = 0; i < snapInspector.cWindow; i++) {
        lp = &snapInspector.rgWindow[i];
        if (lp->idParent != idParent) continue;
        cb = strlen(lp->pszText);
        if (cb > (SIZE_MAX - 1) / 3) continue;
        pszUtf = (char *)(SDL_malloc(cb * 3 + 1));
        if (!pszUtf) continue;
        memcpy(pszUtf, lp->pszText, cb + 1);
        cch = CchUtf8Prefix(pszUtf, sizeof(szText) - 1);
        memcpy(szText, pszUtf, cch); szText[cch] = 0; SDL_free(pszUtf);
        for (lpch = szText; *lpch; lpch++) if (*lpch == '\r' || *lpch == '\n' || *lpch == '\t') *lpch = ' ';
        snprintf(sz, sizeof(sz), "%*s[%" PRIu64 "] %s #%" PRIdPTR " %s%s", cDepth * 2, "",
                 lp->id, lp->pszClass, (intptr_t)lp->idControl, szText, lp->fVisible ? "" : " (hidden)");
        iRow = (int)SendMessageA(hwndList, LB_ADDSTRING, 0, (LPARAM)sz);
        if (iRow >= 0) SendMessageA(hwndList, LB_SETITEMDATA, (WPARAM)iRow, (LPARAM)i);
        InspectRows(hwndList, lp->id, cDepth + 1);
    }
}

static BOOL RefreshInspector(HWND hwnd) {
    STARSCOMPATSNAPSHOT snap = {};
    HWND hwndList = GetDlgItem(hwnd, 1);
    if (!StarsCompatCaptureHierarchy(&snap)) return FALSE;
    SendMessageA(hwndList, LB_RESETCONTENT, 0, 0);
    StarsCompatFreeHierarchy(&snapInspector);
    snapInspector = snap;
    InspectRows(hwndList, 0, 0);
    SendMessageA(hwndList, LB_SETCURSEL, 0, 0);
    InspectDetails(hwnd);
    return TRUE;
}

static LRESULT CALLBACK InspectorProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    RECT rc;
    HBRUSH hbr;
    int cxList, fPainted;
    switch (msg) {
    case WM_CREATE:
        if (!CreateWindow("ListBox", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY,
                           8, 40, 400, 500, hwnd, (HMENU)(INT_PTR)1, NULL, NULL) ||
            !CreateWindow("Edit", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP |
                           ES_READONLY | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                           420, 40, 360, 500, hwnd, (HMENU)(INT_PTR)2, NULL, NULL) ||
            !CreateWindow("Button", "Refresh", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                           8, 8, 100, 24, hwnd, (HMENU)(INT_PTR)3, NULL, NULL)) return -1;
        if (pfnObjectInspector) CreateWindow("Button", "Map objects", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                              120, 8, 110, 24, hwnd, (HMENU)(INT_PTR)4, NULL, NULL);
        return 0;
    case WM_ERASEBKGND:
        if (!GetClientRect(hwnd, &rc) || !(hbr = CreateSolidBrush(GetSysColor(COLOR_BTNFACE)))) return 0;
        fPainted = FillRect((HDC)w, &rc, hbr);
        DeleteObject(hbr);
        return fPainted;
    case WM_SIZE:
        GetClientRect(hwnd, &rc); cxList = StarsMax(0, rc.right / 2 - 12);
        MoveWindow(GetDlgItem(hwnd, 1), 8, 40, cxList, StarsMax(0, rc.bottom - 48), TRUE);
        MoveWindow(GetDlgItem(hwnd, 2), rc.right / 2, 40, StarsMax(0, rc.right / 2 - 8), StarsMax(0, rc.bottom - 48), TRUE);
        return 0;
    case WM_COMMAND:
        if (LOWORD(w) == 3 && HIWORD(w) == BN_CLICKED) RefreshInspector(hwnd);
        else if (LOWORD(w) == 4 && HIWORD(w) == BN_CLICKED && pfnObjectInspector) CompatInvokeInspector(pfnObjectInspector, hwnd);
        else if (LOWORD(w) == 1 && HIWORD(w) == LBN_SELCHANGE) InspectDetails(hwnd);
        return 0;
    case WM_DESTROY:
        idInspector = 0; StarsCompatFreeHierarchy(&snapInspector); return 0;
    default: return DefWindowProcA(hwnd, msg, w, l);
    }
}

HWND StarsCompatShowInspector(void) {
    HWND hwnd;
    WNDCLASSA wc = {};
    if (!SDL_IsMainThread()) return NULL;
    hwnd = StarsCompatResolveWindow(idInspector);
    if (!hwnd) {
        wc.lpfnWndProc = InspectorProc; wc.lpszClassName = "StarsCompatInspector";
        if (!fInspectorClass) {
            if (!RegisterClassA(&wc)) return NULL;
            fInspectorClass = TRUE;
        }
        hwnd = CreateWindow(wc.lpszClassName, "Stars window inspector", WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, 900, 600, NULL, NULL, NULL, NULL);
        if (!hwnd) return NULL;
        idInspector = StarsCompatWindowId(hwnd);
    }
    if (!RefreshInspector(hwnd)) { DestroyWindow(hwnd); return NULL; }
    ShowWindow(hwnd, SW_SHOWNORMAL);
    SetFocus(GetDlgItem(hwnd, 1));
    return hwnd;
}

void CompatShutdownInspector(void) {
    idInspector = 0; fInspectorClass = FALSE; pfnObjectInspector = NULL;
    StarsCompatFreeHierarchy(&snapInspector);
}
