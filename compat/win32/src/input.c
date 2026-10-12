/* SDL input routing adapted from Free API internal/FreeApiMessageQueue.cpp.
 * Copyright (c) 2026 Robert Vokac. MIT license: ../LICENSE.free-api.
 * Stars adaptation: exact window IDs, per-window buttons and normal focus loss.
 */
#include "internal.h"
#include <stars_compat.h>
#include <math.h>
#include <string.h>

static HWND hwndCapture, rghwndClick[3];
static unsigned int rgcClick[3];
static BYTE rgbInput[256], rgbRead[256], rgbRecent[256], rgbAsync[256];
static POINT ptCursor;
static HWND hwndCursor;
static BOOL fCursor, fInputKeys;

void CompatCopyInput(BYTE *rgb, LPPOINT lppt) {
    memcpy(rgb, rgbInput, sizeof(rgbInput));
    if (fCursor) *lppt = ptCursor;
}
void CompatReadInput(const BYTE *rgb) { memcpy(rgbRead, rgb, sizeof(rgbRead)); }
BOOL WINAPI SetKeyboardState(BYTE *rgb) {
    if (!SDL_IsMainThread() || !rgb) return FALSE;
    CompatReadInput(rgb);
    return TRUE;
}
void CompatResetInput(void) {
    memset(rgbInput, 0, sizeof(rgbInput)); memset(rgbRead, 0, sizeof(rgbRead));
    memset(rgbRecent, 0, sizeof(rgbRecent)); memset(rgbAsync, 0, sizeof(rgbAsync));
    fCursor = fInputKeys = FALSE;
    hwndCursor = NULL;
    memset(rghwndClick, 0, sizeof(rghwndClick)); memset(rgcClick, 0, sizeof(rgcClick));
}

SHORT WINAPI GetKeyState(int vk) {
    BYTE b;
    if (!SDL_IsMainThread() || vk < 0 || vk > 255) return 0;
    b = rgbRead[vk];
    return (SHORT)((b & 0x80 ? 0x8000 : 0) | (b & 1));
}

static void SetKeyDown(UINT vk, BOOL fDown) {
    if (fDown && !(rgbInput[vk] & 0x80)) rgbRecent[vk] = 1;
    rgbInput[vk] = (BYTE)((rgbInput[vk] & 1) | (fDown ? 0x80 : 0));
}

static void InputModifiers(SDL_Keymod mod) {
    SetKeyDown(VK_LSHIFT, mod & SDL_KMOD_LSHIFT); SetKeyDown(VK_RSHIFT, mod & SDL_KMOD_RSHIFT);
    SetKeyDown(VK_LCONTROL, mod & SDL_KMOD_LCTRL); SetKeyDown(VK_RCONTROL, mod & SDL_KMOD_RCTRL);
    SetKeyDown(VK_LMENU, mod & SDL_KMOD_LALT); SetKeyDown(VK_RMENU, mod & SDL_KMOD_RALT);
    SetKeyDown(VK_SHIFT, mod & SDL_KMOD_SHIFT); SetKeyDown(VK_CONTROL, mod & SDL_KMOD_CTRL);
    SetKeyDown(VK_MENU, mod & SDL_KMOD_ALT);
    rgbInput[VK_CAPITAL] = (BYTE)((rgbInput[VK_CAPITAL] & 0x80) | !!(mod & SDL_KMOD_CAPS));
    rgbInput[VK_NUMLOCK] = (BYTE)((rgbInput[VK_NUMLOCK] & 0x80) | !!(mod & SDL_KMOD_NUM));
    rgbInput[VK_SCROLL] = (BYTE)((rgbInput[VK_SCROLL] & 0x80) | !!(mod & SDL_KMOD_SCROLL));
}

static BOOL FInputPoint(float fx, float fy, int *lpx, int *lpy) {
    if (!isfinite(fx) || !isfinite(fy) || (double)fx < INT32_MIN || (double)fx > INT32_MAX ||
        (double)fy < INT32_MIN || (double)fy > INT32_MAX) return FALSE;
    *lpx = (int)fx; *lpy = (int)fy;
    return TRUE;
}

static WPARAM WInputButtons(void) {
    return (rgbInput[VK_LBUTTON] & 0x80 ? MK_LBUTTON : 0) |
           (rgbInput[VK_RBUTTON] & 0x80 ? MK_RBUTTON : 0) |
           (rgbInput[VK_MBUTTON] & 0x80 ? MK_MBUTTON : 0);
}

static void InputMouse(HWND hwnd, int x, int y, WPARAM buttons) {
    POINT pt = {x, y - CyCompatMenuBar(hwnd)};
    if (!fInputKeys) InputModifiers(SDL_GetModState());
    SetKeyDown(VK_LBUTTON, buttons & MK_LBUTTON);
    SetKeyDown(VK_RBUTTON, buttons & MK_RBUTTON);
    SetKeyDown(VK_MBUTTON, buttons & MK_MBUTTON);
    if (ClientToScreen(hwnd, &pt)) { ptCursor = pt; hwndCursor = hwnd; fCursor = TRUE; }
    CompatCursorPoint(hwnd, x, y);
}

HWND HwndCompatPointerRoot(void) {
    SDL_Window *lpWindow = SDL_GetMouseFocus();
    HWND hwnd = lpWindow ? LpCompatWindowFromId(SDL_GetWindowID(lpWindow)) : NULL;
    return HwndCompatRoot(hwnd ? hwnd : LpCompatWindow(hwndCursor));
}

BOOL WINAPI GetCursorPos(LPPOINT lppt) {
    const char *pszDriver;
    float x, y;
    HWND hwnd;
    SDL_Window *lpWindow;
    POINT pt;
    if (!SDL_IsMainThread() || !lppt || !(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    pszDriver = SDL_GetCurrentVideoDriver();
    if (pszDriver && strcmp(pszDriver, "dummy") == 0) {
        if (!fCursor) { SetLastError(ERROR_NOT_SUPPORTED); return FALSE; }
        *lppt = ptCursor;
        return TRUE;
    }
    if (pszDriver && strcmp(pszDriver, "wayland") == 0) {
        lpWindow = SDL_GetMouseFocus();
        hwnd = lpWindow ? LpCompatWindowFromId(SDL_GetWindowID(lpWindow)) : NULL;
        if (hwnd) {
            SDL_GetMouseState(&x, &y);
            if (!FInputPoint(x, y, &pt.x, &pt.y)) return FALSE;
            pt.y -= CyCompatMenuBar(hwnd);
            if (!ClientToScreen(hwnd, &pt)) return FALSE;
            *lppt = pt;
            return TRUE;
        }
        // Retain the last delivered point during captured or queued input;
        // no global position outside our surfaces exists on Wayland.
        if (fCursor && IsWindow(hwndCursor)) { *lppt = ptCursor; return TRUE; }
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }
    if (!pszDriver || (strcmp(pszDriver, "x11") != 0 && strcmp(pszDriver, "cocoa") != 0)) { SetLastError(ERROR_NOT_SUPPORTED); return FALSE; }
    SDL_GetGlobalMouseState(&x, &y);
    if (!isfinite(x) || !isfinite(y) || (double)x < INT32_MIN || (double)x > INT32_MAX ||
        (double)y < INT32_MIN || (double)y > INT32_MAX) return FALSE;
    lppt->x = (LONG)x; lppt->y = (LONG)y;
    return TRUE;
}

HWND WINAPI GetCapture(void) { return LpCompatWindow(hwndCapture); }

HWND WINAPI SetCapture(HWND hwnd) {
    HWND hwndOld, lpwin = LpCompatWindow(hwnd);

    if (!SDL_IsMainThread() || (hwnd && (!lpwin || lpwin->fDestroying)))
        return NULL;
    hwndOld = GetCapture();
    hwndCapture = hwnd;
    // SDL capture is optional on headless backends; logical child routing still works.
    SDL_CaptureMouse(hwnd && HwndCompatRoot(GetFocus()) == HwndCompatRoot(hwnd));
    if (hwndOld) SendMessageA(hwndOld, WM_CAPTURECHANGED, 0, (LPARAM)hwnd);
    return hwndOld;
}

BOOL WINAPI ReleaseCapture(void) {
    if (!SDL_IsMainThread()) return FALSE;
    SetCapture(NULL);
    return TRUE;
}

static HWND HwndMouseTarget(HWND hwndRoot, int *lpx, int *lpy) {
    HWND hwnd = GetCapture(), hwndHit;
    POINT pt;
    int x, y;

    *lpy -= CyCompatMenuBar(hwndRoot);
    x = *lpx; y = *lpy;
    if (!hwnd) return HwndCompatMouse(hwndRoot, lpx, lpy);
    if (HwndCompatRoot(GetFocus()) != HwndCompatRoot(hwnd)) {
        hwndHit = HwndCompatMouse(hwndRoot, &x, &y);
        if (hwndHit != hwnd && !IsChild(hwnd, hwndHit))
            return HwndCompatMouse(hwndRoot, lpx, lpy);
    }
    pt.x = *lpx; pt.y = *lpy;
    if (!ClientToScreen(hwndRoot, &pt) || !ScreenToClient(hwnd, &pt)) return NULL;
    *lpx = pt.x; *lpy = pt.y;
    return hwnd;
}

static BOOL FAnsiInput(const char **lppsz, BYTE *lpch, BOOL fStrict) {
    const unsigned char *psz = (const unsigned char *)*lppsz;
    DWORD ch;
    int c, i;
    if (*psz < 0x80) { *lpch = *psz; *lppsz += 1; return TRUE; }
    if (*psz >= 0xc2 && *psz <= 0xdf) { c = 2; ch = *psz & 0x1f; }
    else if (*psz >= 0xe0 && *psz <= 0xef) { c = 3; ch = *psz & 0x0f; }
    else if (*psz >= 0xf0 && *psz <= 0xf4) { c = 4; ch = *psz & 7; }
    else return FALSE;
    for (i = 1; i < c; i++) {
        if ((psz[i] & 0xc0) != 0x80) return FALSE;
        ch = (ch << 6) | (psz[i] & 0x3f);
    }
    if ((c == 3 && ch < 0x800) || (c == 4 && ch < 0x10000) || ch > 0x10ffff ||
        (ch >= 0xd800 && ch <= 0xdfff)) return FALSE;
    *lppsz += c;
    if (ch >= 0xa0 && ch <= 0xff) *lpch = (BYTE)ch;
    else {
        *lpch = '?';
        for (i = 0x80; i < 0xa0; i++)
            if (DwCompatAnsiChar((BYTE)i) == ch) { *lpch = (BYTE)i; break; }
        if (fStrict && i == 0xa0) return FALSE;
    }
    return TRUE;
}

void CompatTextInput(HWND hwnd, const char *pszText) {
    const char *psz = pszText;
    BYTE ch;
    BOOL fStrict;
    if (!IsWindow(hwnd)) return;
    fStrict = !hwnd->fUtf8Text && (hwnd->dwStyle & ES_PASSWORD);
    // Validate the whole event before posting, so malformed UTF-8 adds no prefix.
    while (*psz)
        if (!FAnsiInput(&psz, &ch, fStrict)) {
            if (fStrict) hwnd->fEditInputRejected = TRUE;
            SDL_SetError("Text input is malformed or not representable in this password field"); return;
        }
    psz = pszText;
    while (*psz) {
        if (hwnd->fUtf8Text) {
            DWORD chUtf8 = SDL_StepUTF8(&psz, NULL);
            CompatPostInput(hwnd, WM_CHAR, chUtf8, 1);
        } else {
            if (!FAnsiInput(&psz, &ch, FALSE)) return;
            CompatPostInput(hwnd, WM_CHAR, ch, 1);
        }
    }
}

static UINT VkFromScancode(SDL_Scancode sc) {
    if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
        return (UINT)('A' + sc - SDL_SCANCODE_A);
    if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_9)
        return (UINT)('1' + sc - SDL_SCANCODE_1);
    if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F12)
        return (UINT)(VK_F1 + sc - SDL_SCANCODE_F1);
    switch (sc) {
    case SDL_SCANCODE_0: return '0';
    case SDL_SCANCODE_RETURN: return VK_RETURN;
    case SDL_SCANCODE_ESCAPE: return VK_ESCAPE;
    case SDL_SCANCODE_BACKSPACE: return VK_BACK;
    case SDL_SCANCODE_TAB: return VK_TAB;
    case SDL_SCANCODE_SPACE: return VK_SPACE;
    case SDL_SCANCODE_LEFT: return VK_LEFT;
    case SDL_SCANCODE_RIGHT: return VK_RIGHT;
    case SDL_SCANCODE_UP: return VK_UP;
    case SDL_SCANCODE_DOWN: return VK_DOWN;
    case SDL_SCANCODE_HOME: return VK_HOME;
    case SDL_SCANCODE_END: return VK_END;
    case SDL_SCANCODE_PAGEUP: return VK_PRIOR;
    case SDL_SCANCODE_PAGEDOWN: return VK_NEXT;
    case SDL_SCANCODE_INSERT: return VK_INSERT;
    case SDL_SCANCODE_DELETE: return VK_DELETE;
    case SDL_SCANCODE_CAPSLOCK: return VK_CAPITAL;
    case SDL_SCANCODE_NUMLOCKCLEAR: return VK_NUMLOCK;
    case SDL_SCANCODE_SCROLLLOCK: return VK_SCROLL;
    case SDL_SCANCODE_COMMA: return VK_OEM_COMMA;
    case SDL_SCANCODE_PERIOD: return VK_OEM_PERIOD;
    case SDL_SCANCODE_LEFTBRACKET: return VK_OEM_4;
    case SDL_SCANCODE_RIGHTBRACKET: return VK_OEM_6;
    case SDL_SCANCODE_SEMICOLON: return VK_OEM_1;
    case SDL_SCANCODE_EQUALS: return VK_OEM_PLUS;
    case SDL_SCANCODE_MINUS: return VK_OEM_MINUS;
    case SDL_SCANCODE_SLASH: return VK_OEM_2;
    case SDL_SCANCODE_GRAVE: return VK_OEM_3;
    case SDL_SCANCODE_BACKSLASH: return VK_OEM_5;
    case SDL_SCANCODE_APOSTROPHE: return VK_OEM_7;
    case SDL_SCANCODE_LSHIFT: case SDL_SCANCODE_RSHIFT: return VK_SHIFT;
    case SDL_SCANCODE_LCTRL: case SDL_SCANCODE_RCTRL: return VK_CONTROL;
    case SDL_SCANCODE_LALT: case SDL_SCANCODE_RALT: return VK_MENU;
    default: return 0;
    }
}

static UINT VkSideFromScancode(SDL_Scancode sc) {
    switch (sc) {
    case SDL_SCANCODE_LSHIFT: return VK_LSHIFT;
    case SDL_SCANCODE_RSHIFT: return VK_RSHIFT;
    case SDL_SCANCODE_LCTRL: return VK_LCONTROL;
    case SDL_SCANCODE_RCTRL: return VK_RCONTROL;
    case SDL_SCANCODE_LALT: return VK_LMENU;
    case SDL_SCANCODE_RALT: return VK_RMENU;
    default: return 0;
    }
}

SHORT WINAPI GetAsyncKeyState(int vk) {
    const bool *rgf;
    BYTE rgb[256] = {0};
    SDL_Keymod mod;
    SDL_MouseButtonFlags buttons;
    int cKeys, i;
    UINT key;
    SHORT w;
    if (!SDL_IsMainThread() || vk < 0 || vk > 255 ||
        !(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)) return 0;
    SDL_PumpEvents();
    rgf = SDL_GetKeyboardState(&cKeys);
    for (i = 0; rgf && i < cKeys; i++) {
        key = VkFromScancode((SDL_Scancode)i);
        if (key != 0 && rgf[i]) rgb[key] = 0x80;
        key = VkSideFromScancode((SDL_Scancode)i);
        if (key != 0 && rgf[i]) rgb[key] = 0x80;
    }
    mod = SDL_GetModState();
    rgb[VK_LSHIFT] |= (mod & SDL_KMOD_LSHIFT) ? 0x80 : 0;
    rgb[VK_RSHIFT] |= (mod & SDL_KMOD_RSHIFT) ? 0x80 : 0;
    rgb[VK_LCONTROL] |= (mod & SDL_KMOD_LCTRL) ? 0x80 : 0;
    rgb[VK_RCONTROL] |= (mod & SDL_KMOD_RCTRL) ? 0x80 : 0;
    rgb[VK_LMENU] |= (mod & SDL_KMOD_LALT) ? 0x80 : 0;
    rgb[VK_RMENU] |= (mod & SDL_KMOD_RALT) ? 0x80 : 0;
    rgb[VK_SHIFT] |= rgb[VK_LSHIFT] | rgb[VK_RSHIFT];
    rgb[VK_CONTROL] |= rgb[VK_LCONTROL] | rgb[VK_RCONTROL];
    rgb[VK_MENU] |= rgb[VK_LMENU] | rgb[VK_RMENU];
    buttons = SDL_GetCurrentVideoDriver() && !strcmp(SDL_GetCurrentVideoDriver(), "wayland") ?
              SDL_GetMouseState(NULL, NULL) : SDL_GetGlobalMouseState(NULL, NULL);
    rgb[VK_LBUTTON] = (buttons & SDL_BUTTON_LMASK) ? 0x80 : 0;
    rgb[VK_RBUTTON] = (buttons & SDL_BUTTON_RMASK) ? 0x80 : 0;
    rgb[VK_MBUTTON] = (buttons & SDL_BUTTON_MMASK) ? 0x80 : 0;
    for (i = 0; i < 256; i++) {
        if (rgb[i] && !rgbAsync[i]) rgbRecent[i] = 1;
        rgbAsync[i] = rgb[i];
    }
    w = (SHORT)((rgb[vk] ? 0x8000 : 0) | rgbRecent[vk]);
    rgbRecent[vk] = 0;
    return w;
}

static WPARAM WMouseModifiers(WPARAM buttons) {
    if (rgbInput[VK_SHIFT] & 0x80)
        buttons |= MK_SHIFT;
    if (rgbInput[VK_CONTROL] & 0x80)
        buttons |= MK_CONTROL;
    return buttons;
}

static void PostCompatWheel(HWND hwnd, UINT msg, float dWheel, double *lpdRem, WPARAM buttons) {
    int d, delta;
    if (!isfinite(dWheel) || fabs((double)dWheel) > INT32_MAX / (double)WHEEL_DELTA) return;
    *lpdRem += (double)dWheel * WHEEL_DELTA;
    if (*lpdRem < INT32_MIN || *lpdRem > INT32_MAX) { *lpdRem = 0; return; }
    d = (int)*lpdRem;
    *lpdRem -= d;
    while (d != 0) {
        delta = max(INT16_MIN, min(INT16_MAX, d));
        CompatPostInput(hwnd, msg, MAKEWPARAM(WMouseModifiers(buttons), (WORD)delta),
                        MAKELPARAM(ptCursor.x, ptCursor.y));
        d -= delta;
    }
}

static DWORD DwKeyScan(SDL_Scancode sc) {
    static const BYTE rgScanLetters[] = {
        0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
        0x31, 0x18, 0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c
    };
    DWORD scan = 0, fExtended = 0;

    if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
        scan = rgScanLetters[sc - SDL_SCANCODE_A];
    else if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_9)
        scan = (DWORD)(sc - SDL_SCANCODE_1 + 2);
    else if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F10)
        scan = (DWORD)(sc - SDL_SCANCODE_F1 + 0x3b);
    else {
        switch (sc) {
        case SDL_SCANCODE_0: scan = 0x0b; break;
        case SDL_SCANCODE_RETURN: scan = 0x1c; break;
        case SDL_SCANCODE_ESCAPE: scan = 0x01; break;
        case SDL_SCANCODE_BACKSPACE: scan = 0x0e; break;
        case SDL_SCANCODE_TAB: scan = 0x0f; break;
        case SDL_SCANCODE_SPACE: scan = 0x39; break;
        case SDL_SCANCODE_CAPSLOCK: scan = 0x3a; break;
        case SDL_SCANCODE_NUMLOCKCLEAR: scan = 0x45; fExtended = 1; break;
        case SDL_SCANCODE_SCROLLLOCK: scan = 0x46; break;
        case SDL_SCANCODE_COMMA: scan = 0x33; break;
        case SDL_SCANCODE_PERIOD: scan = 0x34; break;
        case SDL_SCANCODE_LEFTBRACKET: scan = 0x1a; break;
        case SDL_SCANCODE_RIGHTBRACKET: scan = 0x1b; break;
        case SDL_SCANCODE_SEMICOLON: scan = 0x27; break;
        case SDL_SCANCODE_EQUALS: scan = 0x0d; break;
        case SDL_SCANCODE_MINUS: scan = 0x0c; break;
        case SDL_SCANCODE_SLASH: scan = 0x35; break;
        case SDL_SCANCODE_GRAVE: scan = 0x29; break;
        case SDL_SCANCODE_BACKSLASH: scan = 0x2b; break;
        case SDL_SCANCODE_APOSTROPHE: scan = 0x28; break;
        case SDL_SCANCODE_F11: scan = 0x57; break;
        case SDL_SCANCODE_F12: scan = 0x58; break;
        case SDL_SCANCODE_LSHIFT: scan = 0x2a; break;
        case SDL_SCANCODE_RSHIFT: scan = 0x36; break;
        case SDL_SCANCODE_LCTRL: scan = 0x1d; break;
        case SDL_SCANCODE_RCTRL: scan = 0x1d; fExtended = 1; break;
        case SDL_SCANCODE_LALT: scan = 0x38; break;
        case SDL_SCANCODE_RALT: scan = 0x38; fExtended = 1; break;
        case SDL_SCANCODE_LEFT: scan = 0x4b; fExtended = 1; break;
        case SDL_SCANCODE_RIGHT: scan = 0x4d; fExtended = 1; break;
        case SDL_SCANCODE_UP: scan = 0x48; fExtended = 1; break;
        case SDL_SCANCODE_DOWN: scan = 0x50; fExtended = 1; break;
        case SDL_SCANCODE_HOME: scan = 0x47; fExtended = 1; break;
        case SDL_SCANCODE_END: scan = 0x4f; fExtended = 1; break;
        case SDL_SCANCODE_PAGEUP: scan = 0x49; fExtended = 1; break;
        case SDL_SCANCODE_PAGEDOWN: scan = 0x51; fExtended = 1; break;
        case SDL_SCANCODE_INSERT: scan = 0x52; fExtended = 1; break;
        case SDL_SCANCODE_DELETE: scan = 0x53; fExtended = 1; break;
        default: break;
        }
    }
    return (scan << 16) | (fExtended << 24);
}

void CompatRouteEvent(const SDL_Event *lpevent) {
    HWND hwnd, hwndRoot, hwndFocus;
    int x, y;
    UINT vk, vkSide, msg;
    BOOL fDown, fAlt;
    DWORD dwKey;
    WPARAM buttons;
    int i, iButton;
    float dx, dy;

    if (lpevent->type == SDL_EVENT_QUIT) {
        PostQuitMessage(0);
        return;
    }
    // Window/input SDL event unions all use windowID; never fall back to another HWND.
    switch (lpevent->type) {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_MOVED:
    case SDL_EVENT_WINDOW_EXPOSED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
    case SDL_EVENT_WINDOW_FOCUS_LOST:
    case SDL_EVENT_WINDOW_MINIMIZED:
    case SDL_EVENT_WINDOW_MAXIMIZED:
    case SDL_EVENT_WINDOW_RESTORED:
        hwnd = LpCompatWindowFromId(lpevent->window.windowID);
        if ((lpevent->type == SDL_EVENT_WINDOW_FOCUS_GAINED || lpevent->type == SDL_EVENT_WINDOW_FOCUS_LOST) &&
            FCompatMenuFocus(hwnd, lpevent->type == SDL_EVENT_WINDOW_FOCUS_GAINED)) return;
        if (!hwnd)
            return;
        if (lpevent->type == SDL_EVENT_WINDOW_FOCUS_GAINED && (hwnd->dwExStyle & WS_EX_NOACTIVATE)) return;
        if (lpevent->type == SDL_EVENT_WINDOW_FOCUS_LOST) {
            SDL_Window *lpFocus = SDL_GetKeyboardFocus();
            HWND hwndNativeFocus = lpFocus ? LpCompatWindowFromId(SDL_GetWindowID(lpFocus)) : NULL;
            // The dummy backend can focus a nonactivating popup despite its
            // SDL flags. Preserve logical focus/capture when it belongs to us;
            // an actual switch away still follows the normal cancellation path.
            if (hwndNativeFocus && (hwndNativeFocus->dwExStyle & WS_EX_NOACTIVATE) &&
                hwndNativeFocus->hwndOwner == hwnd) return;
        }
        if (lpevent->type == SDL_EVENT_WINDOW_MINIMIZED ||
            lpevent->type == SDL_EVENT_WINDOW_MAXIMIZED || lpevent->type == SDL_EVENT_WINDOW_RESTORED) {
            CompatWindowState(hwnd, lpevent->type == SDL_EVENT_WINDOW_MINIMIZED ? SW_SHOWMINIMIZED :
                              lpevent->type == SDL_EVENT_WINDOW_MAXIMIZED ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL, TRUE);
        } else if (lpevent->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            PostMessageA(hwnd, WM_CLOSE, 0, 0);
        else if (lpevent->type == SDL_EVENT_WINDOW_MOVED) {
            CompatRememberNormal(hwnd);
            PostMessageA(hwnd, WM_MOVE, 0, MAKELPARAM(lpevent->window.data1, lpevent->window.data2));
        } else if (lpevent->type == SDL_EVENT_WINDOW_RESIZED) {
            CompatRememberNormal(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            x = lpevent->window.data1; y = lpevent->window.data2;
            if (hwnd->fZeroX || hwnd->fZeroY) {
                RECT rc;
                // Older queued resize events must not cancel a newer zero-size request.
                if (GetClientRect(hwnd, &rc)) {
                    if (hwnd->fZeroX) x = rc.right;
                    if (hwnd->fZeroY) y = rc.bottom;
                }
            }
            if (!hwnd->fZeroY) y = max(0, y - CyCompatMenuBar(hwnd));
            if (!hwnd->fZeroX && (hwnd->dwStyle & WS_VSCROLL)) x = max(0, x - GetSystemMetrics(SM_CXVSCROLL));
            if (!hwnd->fZeroY && (hwnd->dwStyle & WS_HSCROLL)) y = max(0, y - GetSystemMetrics(SM_CYHSCROLL));
            if (IsIconic(hwnd)) x = y = 0;
            PostMessageA(hwnd, WM_SIZE, IsIconic(hwnd) ? SIZE_MINIMIZED :
                         IsZoomed(hwnd) ? SIZE_MAXIMIZED : SIZE_RESTORED, MAKELPARAM(x, y));
        } else if (lpevent->type == SDL_EVENT_WINDOW_EXPOSED ||
                   lpevent->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
            InvalidateRect(hwnd, NULL, TRUE);
        else if (lpevent->type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
            if (HwndCompatRoot(GetFocus()) != hwnd || GetActiveWindow() != hwnd) CompatFocusWindow(hwnd);
        } else {
            for (i = 0; i < 256; i++) rgbInput[i] &= 1;
            fInputKeys = FALSE;
            hwnd->wMouseButtons = 0;
            if (HwndCompatRoot(GetCapture()) == hwnd) ReleaseCapture();
            if (HwndCompatRoot(GetFocus()) == hwnd || GetActiveWindow() == hwnd) CompatFocusWindow(NULL);
        }
        return;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        hwnd = LpCompatWindowFromId(lpevent->key.windowID);
        hwndFocus = GetFocus();
        if (hwndFocus && HwndCompatRoot(hwndFocus) == hwnd) hwnd = hwndFocus;
        if (hwnd && lpevent->key.scancode == SDL_SCANCODE_F12 &&
            (lpevent->key.mod & SDL_KMOD_CTRL) && (lpevent->key.mod & SDL_KMOD_SHIFT)) {
            if (lpevent->type == SDL_EVENT_KEY_DOWN && !lpevent->key.repeat) StarsCompatShowInspector();
            return;
        }
        if (!IsWindowEnabled(hwnd)) return;
        vk = VkFromScancode(lpevent->key.scancode);
        if (!hwnd || vk == 0)
            return;
        fAlt = vk == VK_MENU ? !!(rgbInput[VK_MENU] & 0x80) : !!(lpevent->key.mod & SDL_KMOD_ALT);
        InputModifiers(lpevent->key.mod);
        fInputKeys = TRUE;
        fDown = lpevent->type == SDL_EVENT_KEY_DOWN;
        vkSide = VkSideFromScancode(lpevent->key.scancode);
        if (vkSide != 0) SetKeyDown(vkSide, fDown);
        if (vk == VK_SHIFT) SetKeyDown(vk, (rgbInput[VK_LSHIFT] | rgbInput[VK_RSHIFT]) & 0x80);
        else if (vk == VK_CONTROL) SetKeyDown(vk, (rgbInput[VK_LCONTROL] | rgbInput[VK_RCONTROL]) & 0x80);
        else if (vk == VK_MENU) SetKeyDown(vk, (rgbInput[VK_LMENU] | rgbInput[VK_RMENU]) & 0x80);
        else SetKeyDown(vk, fDown);
        msg = lpevent->type == SDL_EVENT_KEY_DOWN ? WM_KEYDOWN : WM_KEYUP;
        // SDL scancodes are USB usages; Win32 lParam needs PC/AT scan codes.
        dwKey = 1 | DwKeyScan(lpevent->key.scancode);
        if ((lpevent->key.mod & SDL_KMOD_ALT) || vk == VK_F10 || vk == VK_MENU)
            msg = lpevent->type == SDL_EVENT_KEY_DOWN ? WM_SYSKEYDOWN : WM_SYSKEYUP;
        if (fAlt)
            dwKey |= UINT32_C(1) << 29;
        if (lpevent->key.repeat || lpevent->type == SDL_EVENT_KEY_UP)
            dwKey |= UINT32_C(1) << 30;
        if (lpevent->type == SDL_EVENT_KEY_UP)
            dwKey |= UINT32_C(1) << 31;
        CompatPostInput(hwnd, msg, vk, (LPARAM)dwKey);
        return;
    case SDL_EVENT_TEXT_EDITING:
        hwnd = LpCompatWindowFromId(lpevent->edit.windowID);
        hwndFocus = GetFocus();
        if (hwndFocus && HwndCompatRoot(hwndFocus) == hwnd && IsWindowEnabled(hwndFocus))
            CompatEditComposition(hwndFocus, lpevent->edit.text, lpevent->edit.start, lpevent->edit.length);
        return;
    case SDL_EVENT_TEXT_INPUT:
        hwnd = LpCompatWindowFromId(lpevent->text.windowID);
        hwndFocus = GetFocus();
        if (hwndFocus && HwndCompatRoot(hwndFocus) == hwnd) hwnd = hwndFocus;
        if (!IsWindowEnabled(hwnd) || !lpevent->text.text)
            return;
        if (!fInputKeys) InputModifiers(SDL_GetModState());
        if (hwnd && hwnd->iControl == 3) CompatEditComposition(hwnd, "", 0, 0);
        CompatTextInput(hwnd, lpevent->text.text);
        return;
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        hwnd = LpCompatWindowFromId(lpevent->window.windowID);
        if (hwnd) CompatCursorLeave(hwnd);
        return;
    case SDL_EVENT_MOUSE_MOTION:
        hwnd = LpCompatWindowFromId(lpevent->motion.windowID);
        if (!hwnd)
            return;
        hwndRoot = hwnd;
        if (!FInputPoint(lpevent->motion.x, lpevent->motion.y, &x, &y)) return;
        if (!GetCapture() && y >= 0 && y < CyCompatMenuBar(hwnd)) {
            POINT pt = {x, y - CyCompatMenuBar(hwnd)};
            InputMouse(hwndRoot, x, y, 0);
            CompatPostInput(hwnd, WM_SETCURSOR, (WPARAM)hwnd, MAKELPARAM(HTMENU, WM_NCMOUSEMOVE));
            if (ClientToScreen(hwnd, &pt)) CompatPostInput(hwnd, WM_NCMOUSEMOVE, HTMENU, MAKELPARAM(pt.x, pt.y));
            return;
        }
        hwnd = HwndMouseTarget(hwnd, &x, &y);
        if (!hwnd) return;
        buttons = 0;
        if (lpevent->motion.state & SDL_BUTTON_LMASK) buttons |= MK_LBUTTON;
        if (lpevent->motion.state & SDL_BUTTON_RMASK) buttons |= MK_RBUTTON;
        if (lpevent->motion.state & SDL_BUTTON_MMASK) buttons |= MK_MBUTTON;
        InputMouse(hwndRoot, (int)lpevent->motion.x, (int)lpevent->motion.y, buttons);
        hwndRoot->wMouseButtons = buttons;
        if (!GetCapture()) CompatPostInput(hwnd, WM_SETCURSOR, (WPARAM)hwnd, MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
        CompatPostInput(hwnd, WM_MOUSEMOVE, WMouseModifiers(buttons),
                     MAKELPARAM(x, y));
        return;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        hwnd = LpCompatWindowFromId(lpevent->button.windowID);
        if (!hwnd)
            return;
        hwndRoot = hwnd;
        if (!FInputPoint(lpevent->button.x, lpevent->button.y, &x, &y)) return;
        if (!GetCapture() && y >= 0 && y < CyCompatMenuBar(hwnd)) {
            POINT pt = {x, y - CyCompatMenuBar(hwnd)};
            if (lpevent->button.button == SDL_BUTTON_LEFT) {
                hwndRoot->wMouseButtons = lpevent->type == SDL_EVENT_MOUSE_BUTTON_DOWN ? MK_LBUTTON : 0;
                InputMouse(hwndRoot, x, y, hwndRoot->wMouseButtons);
                if (ClientToScreen(hwnd, &pt)) CompatPostInput(hwnd, lpevent->type == SDL_EVENT_MOUSE_BUTTON_DOWN ? WM_NCLBUTTONDOWN : WM_NCLBUTTONUP,
                                                            HTMENU, MAKELPARAM(pt.x, pt.y));
            }
            return;
        }
        hwnd = HwndMouseTarget(hwnd, &x, &y);
        if (!hwnd) return;
        if (lpevent->button.button == SDL_BUTTON_LEFT) {
            iButton = 0;
            buttons = MK_LBUTTON;
            msg = lpevent->type == SDL_EVENT_MOUSE_BUTTON_DOWN ? WM_LBUTTONDOWN : WM_LBUTTONUP;
        } else if (lpevent->button.button == SDL_BUTTON_RIGHT) {
            iButton = 1;
            buttons = MK_RBUTTON;
            msg = lpevent->type == SDL_EVENT_MOUSE_BUTTON_DOWN ? WM_RBUTTONDOWN : WM_RBUTTONUP;
        } else if (lpevent->button.button == SDL_BUTTON_MIDDLE) {
            iButton = 2;
            buttons = MK_MBUTTON;
            msg = lpevent->type == SDL_EVENT_MOUSE_BUTTON_DOWN ? WM_MBUTTONDOWN : WM_MBUTTONUP;
        } else
            return;
        hwndRoot->wMouseButtons = WInputButtons();
        if (lpevent->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            if (lpevent->button.clicks <= 1 || rghwndClick[iButton] != hwnd) rgcClick[iButton] = 1;
            else rgcClick[iButton]++;
            rghwndClick[iButton] = hwnd;
            if ((hwnd->lpclass->wc.style & CS_DBLCLKS) && !(rgcClick[iButton] & 1))
                msg = iButton == 0 ? WM_LBUTTONDBLCLK : iButton == 1 ? WM_RBUTTONDBLCLK : WM_MBUTTONDBLCLK;
            hwndRoot->wMouseButtons |= buttons;
        }
        else
            hwndRoot->wMouseButtons &= ~buttons;
        InputMouse(hwndRoot, (int)lpevent->button.x, (int)lpevent->button.y, hwndRoot->wMouseButtons);
        CompatPostInput(hwnd, msg, WMouseModifiers(hwndRoot->wMouseButtons),
                     MAKELPARAM(x, y));
        return;
    case SDL_EVENT_MOUSE_WHEEL:
        hwndRoot = LpCompatWindowFromId(lpevent->wheel.windowID);
        if (!hwndRoot) return;
        hwnd = GetFocus();
        if (!IsWindowEnabled(hwnd)) hwnd = hwndRoot;
        if (!IsWindowEnabled(hwnd)) return;
        if (!FInputPoint(lpevent->wheel.mouse_x, lpevent->wheel.mouse_y, &x, &y)) return;
        buttons = WInputButtons();
        InputMouse(hwndRoot, x, y, buttons);
        dx = lpevent->wheel.x; dy = lpevent->wheel.y;
        if (lpevent->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) { dx = -dx; dy = -dy; }
        PostCompatWheel(hwnd, WM_MOUSEWHEEL, dy, &hwndRoot->dWheelY, buttons);
        PostCompatWheel(hwnd, WM_MOUSEHWHEEL, dx, &hwndRoot->dWheelX, buttons);
        return;
    default:
        return;
    }
}
