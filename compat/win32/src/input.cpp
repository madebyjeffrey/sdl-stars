/* SDL input routing adapted from Free API internal/FreeApiMessageQueue.cpp.
 * Copyright (c) 2026 Robert Vokac. MIT license: ../LICENSE.free-api.
 * Stars adaptation: exact window IDs, per-window buttons and normal focus loss.
 */
#include "internal.h"
#include "input_io.h"
#include <stars_compat.h>
#include <math.h>
#include <stdlib.h>
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
BOOL win32::SetKeyboardState(BYTE *rgb) {
    if (!SDL_IsMainThread() || !rgb) return 0;
    CompatReadInput(rgb);
    return 1;
}
void CompatResetInput(void) {
    memset(rgbInput, 0, sizeof(rgbInput)); memset(rgbRead, 0, sizeof(rgbRead));
    memset(rgbRecent, 0, sizeof(rgbRecent)); memset(rgbAsync, 0, sizeof(rgbAsync));
    fCursor = fInputKeys = 0;
    hwndCursor = NULL;
    memset(rghwndClick, 0, sizeof(rghwndClick)); memset(rgcClick, 0, sizeof(rgcClick));
}

SHORT win32::GetKeyState(int vk) {
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
    SetKeyDown(win32::VK_LSHIFT, mod & SDL_KMOD_LSHIFT); SetKeyDown(win32::VK_RSHIFT, mod & SDL_KMOD_RSHIFT);
    SetKeyDown(win32::VK_LCONTROL, mod & SDL_KMOD_LCTRL); SetKeyDown(win32::VK_RCONTROL, mod & SDL_KMOD_RCTRL);
    SetKeyDown(win32::VK_LMENU, mod & SDL_KMOD_LALT); SetKeyDown(win32::VK_RMENU, mod & SDL_KMOD_RALT);
#ifdef __APPLE__
    SetKeyDown(win32::VK_LWIN, mod & SDL_KMOD_LGUI); SetKeyDown(win32::VK_RWIN, mod & SDL_KMOD_RGUI);
#endif
    SetKeyDown(win32::VK_SHIFT, mod & SDL_KMOD_SHIFT); SetKeyDown(win32::VK_CONTROL, mod & SDL_KMOD_CTRL);
    SetKeyDown(win32::VK_MENU, mod & SDL_KMOD_ALT);
    rgbInput[win32::VK_CAPITAL] = (BYTE)((rgbInput[win32::VK_CAPITAL] & 0x80) | !!(mod & SDL_KMOD_CAPS));
    rgbInput[win32::VK_NUMLOCK] = (BYTE)((rgbInput[win32::VK_NUMLOCK] & 0x80) | !!(mod & SDL_KMOD_NUM));
    rgbInput[win32::VK_SCROLL] = (BYTE)((rgbInput[win32::VK_SCROLL] & 0x80) | !!(mod & SDL_KMOD_SCROLL));
}

static BOOL FInputPoint(float fx, float fy, int *lpx, int *lpy) {
    if (!isfinite(fx) || !isfinite(fy) || (double)fx < INT32_MIN || (double)fx > INT32_MAX ||
        (double)fy < INT32_MIN || (double)fy > INT32_MAX) return 0;
    *lpx = (int)fx; *lpy = (int)fy;
    return 1;
}

static WPARAM WInputButtons(void) {
    return (rgbInput[win32::VK_LBUTTON] & 0x80 ? win32::MK_LBUTTON : 0) |
           (rgbInput[win32::VK_RBUTTON] & 0x80 ? win32::MK_RBUTTON : 0) |
           (rgbInput[win32::VK_MBUTTON] & 0x80 ? win32::MK_MBUTTON : 0);
}

static void InputMouse(HWND hwnd, int x, int y, WPARAM buttons) {
    POINT pt = {x, y - CyCompatMenuBar(hwnd)};
    if (!fInputKeys) InputModifiers(CompatInputIO().pfnGetModState());
    SetKeyDown(win32::VK_LBUTTON, buttons & win32::MK_LBUTTON);
    SetKeyDown(win32::VK_RBUTTON, buttons & win32::MK_RBUTTON);
    SetKeyDown(win32::VK_MBUTTON, buttons & win32::MK_MBUTTON);
    if (win32::ClientToScreen(hwnd, &pt)) { ptCursor = pt; hwndCursor = hwnd; fCursor = 1; }
    CompatCursorPoint(hwnd, x, y);
}

HWND HwndCompatPointerRoot(void) {
    SDL_Window *lpWindow = SDL_GetMouseFocus();
    HWND hwnd = lpWindow ? LpCompatWindowFromId(SDL_GetWindowID(lpWindow)) : NULL;
    return HwndCompatRoot(hwnd ? hwnd : LpCompatWindow(hwndCursor));
}

BOOL win32::GetCursorPos(LPPOINT lppt) {
    const char *pszDriver;
    float x, y;
    HWND hwnd;
    SDL_Window *lpWindow;
    POINT pt;
    if (!SDL_IsMainThread() || !lppt || !(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)) {
        win32::SetLastError(win32::ERROR_INVALID_PARAMETER);
        return 0;
    }
    pszDriver = CompatInputIO().pfnGetCurrentVideoDriver();
    if (pszDriver && strcmp(pszDriver, "dummy") == 0) {
        if (!fCursor) { win32::SetLastError(win32::ERROR_NOT_SUPPORTED); return 0; }
        *lppt = ptCursor;
        return 1;
    }
    if (pszDriver && strcmp(pszDriver, "wayland") == 0) {
        lpWindow = SDL_GetMouseFocus();
        hwnd = lpWindow ? LpCompatWindowFromId(SDL_GetWindowID(lpWindow)) : NULL;
        if (hwnd) {
            SDL_GetMouseState(&x, &y);
            if (!FInputPoint(x, y, &pt.x, &pt.y)) return 0;
            pt.y -= CyCompatMenuBar(hwnd);
            if (!win32::ClientToScreen(hwnd, &pt)) return 0;
            *lppt = pt;
            return 1;
        }
        // Retain the last delivered point during captured or queued input;
        // no global position outside our surfaces exists on Wayland.
        if (fCursor && win32::IsWindow(hwndCursor)) { *lppt = ptCursor; return 1; }
        win32::SetLastError(win32::ERROR_NOT_SUPPORTED);
        return 0;
    }
    if (!pszDriver || (strcmp(pszDriver, "x11") != 0 && strcmp(pszDriver, "cocoa") != 0 &&
                       strcmp(pszDriver, "windows") != 0)) { win32::SetLastError(win32::ERROR_NOT_SUPPORTED); return 0; }
    CompatInputIO().pfnGetGlobalMouseState(&x, &y);
    if (!isfinite(x) || !isfinite(y) || (double)x < INT32_MIN || (double)x > INT32_MAX ||
        (double)y < INT32_MIN || (double)y > INT32_MAX) return 0;
    lppt->x = (LONG)x; lppt->y = (LONG)y;
    return 1;
}

HWND win32::GetCapture(void) { return LpCompatWindow(hwndCapture); }

static BOOL FCaptureForeground(HWND hwnd) {
    HWND hwndRoot = HwndCompatRoot(hwnd);
    HWND hwndFocusRoot = HwndCompatRoot(win32::GetFocus());

    // A nonactivating detail card captures on behalf of its focused owner.
    return hwndRoot && hwndFocusRoot && (hwndRoot == hwndFocusRoot ||
        ((hwndRoot->dwExStyle & win32::WS_EX_NOACTIVATE) && hwndRoot->hwndOwner == hwndFocusRoot));
}

HWND win32::SetCapture(HWND hwnd) {
    HWND hwndOld, lpwin = LpCompatWindow(hwnd);

    if (!SDL_IsMainThread() || (hwnd && (!lpwin || lpwin->fDestroying)))
        return NULL;
    hwndOld = win32::GetCapture();
    hwndCapture = hwnd;
    // SDL capture is optional on headless backends; logical child routing still works.
    SDL_CaptureMouse(hwnd && FCaptureForeground(hwnd));
    if (hwndOld) win32::SendMessageA(hwndOld, win32::WM_CAPTURECHANGED, 0, (LPARAM)hwnd);
    return hwndOld;
}

BOOL win32::ReleaseCapture(void) {
    if (!SDL_IsMainThread()) return 0;
    win32::SetCapture(NULL);
    return 1;
}

static HWND HwndMouseTarget(HWND hwndRoot, int *lpx, int *lpy) {
    HWND hwnd = win32::GetCapture(), hwndHit;
    POINT pt;
    int x, y;

    *lpy -= CyCompatMenuBar(hwndRoot);
    x = *lpx; y = *lpy;
    if (!hwnd) return HwndCompatMouse(hwndRoot, lpx, lpy);
    if (!FCaptureForeground(hwnd)) {
        hwndHit = HwndCompatMouse(hwndRoot, &x, &y);
        if (hwndHit != hwnd && !win32::IsChild(hwnd, hwndHit))
            return HwndCompatMouse(hwndRoot, lpx, lpy);
    }
    pt.x = *lpx; pt.y = *lpy;
    if (!win32::ClientToScreen(hwndRoot, &pt) || !win32::ScreenToClient(hwnd, &pt)) return NULL;
    *lpx = pt.x; *lpy = pt.y;
    return hwnd;
}

void CompatTextInput(HWND hwnd, const char *pszText) {
    const char *psz = pszText;
    DWORD ch;
    if (!win32::IsWindow(hwnd)) return;
    if (!FUtf8Valid(pszText, strlen(pszText))) {
        hwnd->fEditInputRejected = 1;
        SDL_SetError("Text input is malformed UTF-8");
        return;
    }
    while (*psz) {
        ch = SDL_StepUTF8(&psz, NULL);
        CompatPostInput(hwnd, win32::WM_CHAR, ch, 1);
    }
}

static UINT VkFromScancode(SDL_Scancode sc) {
    if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
        return (UINT)('A' + sc - SDL_SCANCODE_A);
    if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_9)
        return (UINT)('1' + sc - SDL_SCANCODE_1);
    if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F12)
        return (UINT)(win32::VK_F1 + sc - SDL_SCANCODE_F1);
    switch (sc) {
    case SDL_SCANCODE_0: return '0';
    case SDL_SCANCODE_RETURN: return win32::VK_RETURN;
    case SDL_SCANCODE_ESCAPE: return win32::VK_ESCAPE;
    case SDL_SCANCODE_BACKSPACE: return win32::VK_BACK;
    case SDL_SCANCODE_TAB: return win32::VK_TAB;
    case SDL_SCANCODE_SPACE: return win32::VK_SPACE;
    case SDL_SCANCODE_LEFT: return win32::VK_LEFT;
    case SDL_SCANCODE_RIGHT: return win32::VK_RIGHT;
    case SDL_SCANCODE_UP: return win32::VK_UP;
    case SDL_SCANCODE_DOWN: return win32::VK_DOWN;
    case SDL_SCANCODE_HOME: return win32::VK_HOME;
    case SDL_SCANCODE_END: return win32::VK_END;
    case SDL_SCANCODE_PAGEUP: return win32::VK_PRIOR;
    case SDL_SCANCODE_PAGEDOWN: return win32::VK_NEXT;
    case SDL_SCANCODE_INSERT: return win32::VK_INSERT;
    case SDL_SCANCODE_DELETE: return win32::VK_DELETE;
    case SDL_SCANCODE_CAPSLOCK: return win32::VK_CAPITAL;
    case SDL_SCANCODE_NUMLOCKCLEAR: return win32::VK_NUMLOCK;
    case SDL_SCANCODE_SCROLLLOCK: return win32::VK_SCROLL;
    case SDL_SCANCODE_COMMA: return win32::VK_OEM_COMMA;
    case SDL_SCANCODE_PERIOD: return win32::VK_OEM_PERIOD;
    case SDL_SCANCODE_LEFTBRACKET: return win32::VK_OEM_4;
    case SDL_SCANCODE_RIGHTBRACKET: return win32::VK_OEM_6;
    case SDL_SCANCODE_SEMICOLON: return win32::VK_OEM_1;
    case SDL_SCANCODE_EQUALS: return win32::VK_OEM_PLUS;
    case SDL_SCANCODE_MINUS: return win32::VK_OEM_MINUS;
    case SDL_SCANCODE_SLASH: return win32::VK_OEM_2;
    case SDL_SCANCODE_GRAVE: return win32::VK_OEM_3;
    case SDL_SCANCODE_BACKSLASH: return win32::VK_OEM_5;
    case SDL_SCANCODE_APOSTROPHE: return win32::VK_OEM_7;
    case SDL_SCANCODE_LSHIFT: case SDL_SCANCODE_RSHIFT: return win32::VK_SHIFT;
    case SDL_SCANCODE_LCTRL: case SDL_SCANCODE_RCTRL: return win32::VK_CONTROL;
    case SDL_SCANCODE_LALT: case SDL_SCANCODE_RALT: return win32::VK_MENU;
#ifdef __APPLE__
    case SDL_SCANCODE_LGUI: return win32::VK_LWIN;
    case SDL_SCANCODE_RGUI: return win32::VK_RWIN;
#endif
    default: return 0;
    }
}

static UINT VkSideFromScancode(SDL_Scancode sc) {
    switch (sc) {
    case SDL_SCANCODE_LSHIFT: return win32::VK_LSHIFT;
    case SDL_SCANCODE_RSHIFT: return win32::VK_RSHIFT;
    case SDL_SCANCODE_LCTRL: return win32::VK_LCONTROL;
    case SDL_SCANCODE_RCTRL: return win32::VK_RCONTROL;
    case SDL_SCANCODE_LALT: return win32::VK_LMENU;
    case SDL_SCANCODE_RALT: return win32::VK_RMENU;
    default: return 0;
    }
}

SHORT win32::GetAsyncKeyState(int vk) {
    const bool *rgf;
    BYTE rgb[256] = {};
    SDL_Keymod mod;
    SDL_MouseButtonFlags buttons;
    int cKeys, i;
    UINT key;
    SHORT w;
    if (!SDL_IsMainThread() || vk < 0 || vk > 255 ||
        !(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO)) return 0;
    SDL_PumpEvents();
    rgf = CompatInputIO().pfnGetKeyboardState(&cKeys);
    for (i = 0; rgf && i < cKeys; i++) {
        key = VkFromScancode((SDL_Scancode)i);
        if (key != 0 && rgf[i]) rgb[key] = 0x80;
        key = VkSideFromScancode((SDL_Scancode)i);
        if (key != 0 && rgf[i]) rgb[key] = 0x80;
    }
    mod = CompatInputIO().pfnGetModState();
    rgb[win32::VK_LSHIFT] |= (mod & SDL_KMOD_LSHIFT) ? 0x80 : 0;
    rgb[win32::VK_RSHIFT] |= (mod & SDL_KMOD_RSHIFT) ? 0x80 : 0;
    rgb[win32::VK_LCONTROL] |= (mod & SDL_KMOD_LCTRL) ? 0x80 : 0;
    rgb[win32::VK_RCONTROL] |= (mod & SDL_KMOD_RCTRL) ? 0x80 : 0;
    rgb[win32::VK_LMENU] |= (mod & SDL_KMOD_LALT) ? 0x80 : 0;
    rgb[win32::VK_RMENU] |= (mod & SDL_KMOD_RALT) ? 0x80 : 0;
    rgb[win32::VK_SHIFT] |= rgb[win32::VK_LSHIFT] | rgb[win32::VK_RSHIFT];
    rgb[win32::VK_CONTROL] |= rgb[win32::VK_LCONTROL] | rgb[win32::VK_RCONTROL];
    rgb[win32::VK_MENU] |= rgb[win32::VK_LMENU] | rgb[win32::VK_RMENU];
    buttons = CompatInputIO().pfnGetCurrentVideoDriver() && !strcmp(CompatInputIO().pfnGetCurrentVideoDriver(), "wayland") ?
              SDL_GetMouseState(NULL, NULL) : CompatInputIO().pfnGetGlobalMouseState(NULL, NULL);
    rgb[win32::VK_LBUTTON] = (buttons & SDL_BUTTON_LMASK) ? 0x80 : 0;
    rgb[win32::VK_RBUTTON] = (buttons & SDL_BUTTON_RMASK) ? 0x80 : 0;
    rgb[win32::VK_MBUTTON] = (buttons & SDL_BUTTON_MMASK) ? 0x80 : 0;
    for (i = 0; i < 256; i++) {
        if (rgb[i] && !rgbAsync[i]) rgbRecent[i] = 1;
        rgbAsync[i] = rgb[i];
    }
    w = (SHORT)((rgb[vk] ? 0x8000 : 0) | rgbRecent[vk]);
    rgbRecent[vk] = 0;
    return w;
}

static WPARAM WMouseModifiers(WPARAM buttons) {
    if (rgbInput[win32::VK_SHIFT] & 0x80)
        buttons |= win32::MK_SHIFT;
    if (rgbInput[win32::VK_CONTROL] & 0x80)
        buttons |= win32::MK_CONTROL;
    return buttons;
}

static void PostCompatWheel(HWND hwnd, UINT msg, float dWheel, double *lpdRem, WPARAM buttons) {
    int d, delta;
    if (!isfinite(dWheel) || fabs((double)dWheel) > INT32_MAX / (double)win32::WHEEL_DELTA) return;
    *lpdRem += (double)dWheel * win32::WHEEL_DELTA;
    if (*lpdRem < INT32_MIN || *lpdRem > INT32_MAX) { *lpdRem = 0; return; }
    d = (int)*lpdRem;
    *lpdRem -= d;
    while (d != 0) {
        delta = (((INT16_MIN) > (StarsMin(INT16_MAX, d))) ? (INT16_MIN) : (StarsMin(INT16_MAX, d)));
        CompatPostInput(hwnd, msg, win32::MAKEWPARAM(WMouseModifiers(buttons), (WORD)delta),
                        win32::MAKELPARAM(ptCursor.x, ptCursor.y));
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
#ifdef __APPLE__
        case SDL_SCANCODE_LGUI: scan = 0x5b; fExtended = 1; break;
        case SDL_SCANCODE_RGUI: scan = 0x5c; fExtended = 1; break;
#endif
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
        win32::PostQuitMessage(0);
        return;
    }
    if (lpevent->type == SDL_EVENT_USER && lpevent->user.code == COMPAT_ASYNC_POST_CODE) {
        CompatConsumeAsyncMessage(lpevent);
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
        if (lpevent->type == SDL_EVENT_WINDOW_FOCUS_GAINED && (hwnd->dwExStyle & win32::WS_EX_NOACTIVATE)) return;
        if (lpevent->type == SDL_EVENT_WINDOW_FOCUS_LOST) {
            SDL_Window *lpFocus = SDL_GetKeyboardFocus();
            HWND hwndNativeFocus = lpFocus ? LpCompatWindowFromId(SDL_GetWindowID(lpFocus)) : NULL;
            // The dummy backend can focus a nonactivating popup despite its
            // SDL flags. Preserve logical focus/capture when it belongs to us;
            // an actual switch away still follows the normal cancellation path.
            if (hwndNativeFocus && (hwndNativeFocus->dwExStyle & win32::WS_EX_NOACTIVATE) &&
                hwndNativeFocus->hwndOwner == hwnd) return;
        }
        if (lpevent->type == SDL_EVENT_WINDOW_MINIMIZED ||
            lpevent->type == SDL_EVENT_WINDOW_MAXIMIZED || lpevent->type == SDL_EVENT_WINDOW_RESTORED) {
            CompatWindowState(hwnd, lpevent->type == SDL_EVENT_WINDOW_MINIMIZED ? win32::SW_SHOWMINIMIZED :
                              lpevent->type == SDL_EVENT_WINDOW_MAXIMIZED ? win32::SW_SHOWMAXIMIZED : win32::SW_SHOWNORMAL, 1);
        } else if (lpevent->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            win32::PostMessageA(hwnd, win32::WM_CLOSE, 0, 0);
        else if (lpevent->type == SDL_EVENT_WINDOW_MOVED) {
            CompatRememberNormal(hwnd);
            win32::PostMessageA(hwnd, win32::WM_MOVE, 0, win32::MAKELPARAM(lpevent->window.data1, lpevent->window.data2));
        } else if (lpevent->type == SDL_EVENT_WINDOW_RESIZED) {
            CompatRememberNormal(hwnd);
            win32::InvalidateRect(hwnd, NULL, 1);
            x = lpevent->window.data1; y = lpevent->window.data2;
            if (hwnd->fZeroX || hwnd->fZeroY) {
                RECT rc;
                // Older queued resize events must not cancel a newer zero-size request.
                if (win32::GetClientRect(hwnd, &rc)) {
                    if (hwnd->fZeroX) x = rc.right;
                    if (hwnd->fZeroY) y = rc.bottom;
                }
            }
            if (!hwnd->fZeroY) y = (((0) > (y - CyCompatMenuBar(hwnd))) ? (0) : (y - CyCompatMenuBar(hwnd)));
            if (!hwnd->fZeroX && (hwnd->dwStyle & win32::WS_VSCROLL)) x = (((0) > (x - win32::GetSystemMetrics(win32::SM_CXVSCROLL))) ? (0) : (x - win32::GetSystemMetrics(win32::SM_CXVSCROLL)));
            if (!hwnd->fZeroY && (hwnd->dwStyle & win32::WS_HSCROLL)) y = (((0) > (y - win32::GetSystemMetrics(win32::SM_CYHSCROLL))) ? (0) : (y - win32::GetSystemMetrics(win32::SM_CYHSCROLL)));
            if (win32::IsIconic(hwnd)) x = y = 0;
            win32::PostMessageA(hwnd, win32::WM_SIZE, win32::IsIconic(hwnd) ? win32::SIZE_MINIMIZED :
                         win32::IsZoomed(hwnd) ? win32::SIZE_MAXIMIZED : win32::SIZE_RESTORED, win32::MAKELPARAM(x, y));
        } else if (lpevent->type == SDL_EVENT_WINDOW_EXPOSED ||
                   lpevent->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
            win32::InvalidateRect(hwnd, NULL, 1);
        else if (lpevent->type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
            if (HwndCompatRoot(win32::GetFocus()) != hwnd || win32::GetActiveWindow() != hwnd) CompatFocusWindow(hwnd);
        } else {
            for (i = 0; i < 256; i++) rgbInput[i] &= 1;
            fInputKeys = 0;
            hwnd->wMouseButtons = 0;
            if (HwndCompatRoot(win32::GetCapture()) == hwnd) win32::ReleaseCapture();
            if (HwndCompatRoot(win32::GetFocus()) == hwnd || win32::GetActiveWindow() == hwnd) CompatFocusWindow(NULL);
        }
        return;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        hwnd = LpCompatWindowFromId(lpevent->key.windowID);
        hwndFocus = win32::GetFocus();
        if (hwndFocus && HwndCompatRoot(hwndFocus) == hwnd) hwnd = hwndFocus;
        if (hwnd && lpevent->key.scancode == SDL_SCANCODE_F12 &&
            (lpevent->key.mod & SDL_KMOD_CTRL) && (lpevent->key.mod & SDL_KMOD_SHIFT)) {
            if (lpevent->type == SDL_EVENT_KEY_DOWN && !lpevent->key.repeat) StarsCompatShowInspector();
            return;
        }
        if (!win32::IsWindowEnabled(hwnd)) return;
        vk = VkFromScancode(lpevent->key.scancode);
        if (!hwnd || vk == 0)
            return;
        fAlt = vk == win32::VK_MENU ? !!(rgbInput[win32::VK_MENU] & 0x80) : !!(lpevent->key.mod & SDL_KMOD_ALT);
        InputModifiers(lpevent->key.mod);
        fInputKeys = 1;
        fDown = lpevent->type == SDL_EVENT_KEY_DOWN;
        vkSide = VkSideFromScancode(lpevent->key.scancode);
        if (vkSide != 0) SetKeyDown(vkSide, fDown);
        if (vk == win32::VK_SHIFT) SetKeyDown(vk, (rgbInput[win32::VK_LSHIFT] | rgbInput[win32::VK_RSHIFT]) & 0x80);
        else if (vk == win32::VK_CONTROL) SetKeyDown(vk, (rgbInput[win32::VK_LCONTROL] | rgbInput[win32::VK_RCONTROL]) & 0x80);
        else if (vk == win32::VK_MENU) SetKeyDown(vk, (rgbInput[win32::VK_LMENU] | rgbInput[win32::VK_RMENU]) & 0x80);
        else SetKeyDown(vk, fDown);
        msg = lpevent->type == SDL_EVENT_KEY_DOWN ? win32::WM_KEYDOWN : win32::WM_KEYUP;
        // SDL scancodes are USB usages; Win32 lParam needs PC/AT scan codes.
        dwKey = 1 | DwKeyScan(lpevent->key.scancode);
        if ((lpevent->key.mod & SDL_KMOD_ALT) || vk == win32::VK_F10 || vk == win32::VK_MENU)
            msg = lpevent->type == SDL_EVENT_KEY_DOWN ? win32::WM_SYSKEYDOWN : win32::WM_SYSKEYUP;
#ifdef __APPLE__
        /* Option arrows edit words; the Option key must not activate Alt menus. */
        if (hwnd->iControl == 3 && (vk == win32::VK_LEFT || vk == win32::VK_RIGHT || vk == win32::VK_MENU))
            msg = lpevent->type == SDL_EVENT_KEY_DOWN ? win32::WM_KEYDOWN : win32::WM_KEYUP;
#endif
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
        hwndFocus = win32::GetFocus();
        if (hwndFocus && HwndCompatRoot(hwndFocus) == hwnd && win32::IsWindowEnabled(hwndFocus))
            CompatEditComposition(hwndFocus, lpevent->edit.text, lpevent->edit.start, lpevent->edit.length);
        return;
    case SDL_EVENT_TEXT_INPUT:
        hwnd = LpCompatWindowFromId(lpevent->text.windowID);
        hwndFocus = win32::GetFocus();
        if (hwndFocus && HwndCompatRoot(hwndFocus) == hwnd) hwnd = hwndFocus;
        if (!win32::IsWindowEnabled(hwnd) || !lpevent->text.text)
            return;
        if (!fInputKeys) InputModifiers(CompatInputIO().pfnGetModState());
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
        if (!win32::GetCapture() && y >= 0 && y < CyCompatMenuBar(hwnd)) {
            POINT pt = {x, y - CyCompatMenuBar(hwnd)};
            InputMouse(hwndRoot, x, y, 0);
            CompatPostInput(hwnd, win32::WM_SETCURSOR, (WPARAM)hwnd, win32::MAKELPARAM(win32::HTMENU, win32::WM_NCMOUSEMOVE));
            if (win32::ClientToScreen(hwnd, &pt)) CompatPostInput(hwnd, win32::WM_NCMOUSEMOVE, win32::HTMENU, win32::MAKELPARAM(pt.x, pt.y));
            return;
        }
        hwnd = HwndMouseTarget(hwnd, &x, &y);
        if (!hwnd) return;
        buttons = 0;
        if (lpevent->motion.state & SDL_BUTTON_LMASK) buttons |= win32::MK_LBUTTON;
        if (lpevent->motion.state & SDL_BUTTON_RMASK) buttons |= win32::MK_RBUTTON;
        if (lpevent->motion.state & SDL_BUTTON_MMASK) buttons |= win32::MK_MBUTTON;
        InputMouse(hwndRoot, (int)lpevent->motion.x, (int)lpevent->motion.y, buttons);
        hwndRoot->wMouseButtons = buttons;
        if (!win32::GetCapture()) CompatPostInput(hwnd, win32::WM_SETCURSOR, (WPARAM)hwnd, win32::MAKELPARAM(win32::HTCLIENT, win32::WM_MOUSEMOVE));
        CompatPostInput(hwnd, win32::WM_MOUSEMOVE, WMouseModifiers(buttons),
                     win32::MAKELPARAM(x, y));
        return;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        hwnd = LpCompatWindowFromId(lpevent->button.windowID);
        if (!hwnd)
            return;
        hwndRoot = hwnd;
        if (!FInputPoint(lpevent->button.x, lpevent->button.y, &x, &y)) return;
        if (!win32::GetCapture() && y >= 0 && y < CyCompatMenuBar(hwnd)) {
            POINT pt = {x, y - CyCompatMenuBar(hwnd)};
            if (lpevent->button.button == SDL_BUTTON_LEFT) {
                hwndRoot->wMouseButtons = lpevent->type == SDL_EVENT_MOUSE_BUTTON_DOWN ? win32::MK_LBUTTON : 0;
                InputMouse(hwndRoot, x, y, hwndRoot->wMouseButtons);
                if (win32::ClientToScreen(hwnd, &pt)) CompatPostInput(hwnd, lpevent->type == SDL_EVENT_MOUSE_BUTTON_DOWN ? win32::WM_NCLBUTTONDOWN : win32::WM_NCLBUTTONUP,
                                                            win32::HTMENU, win32::MAKELPARAM(pt.x, pt.y));
            }
            return;
        }
        hwnd = HwndMouseTarget(hwnd, &x, &y);
        if (!hwnd) return;
        if (lpevent->button.button == SDL_BUTTON_LEFT) {
            iButton = 0;
            buttons = win32::MK_LBUTTON;
            msg = lpevent->type == SDL_EVENT_MOUSE_BUTTON_DOWN ? win32::WM_LBUTTONDOWN : win32::WM_LBUTTONUP;
        } else if (lpevent->button.button == SDL_BUTTON_RIGHT) {
            iButton = 1;
            buttons = win32::MK_RBUTTON;
            msg = lpevent->type == SDL_EVENT_MOUSE_BUTTON_DOWN ? win32::WM_RBUTTONDOWN : win32::WM_RBUTTONUP;
        } else if (lpevent->button.button == SDL_BUTTON_MIDDLE) {
            iButton = 2;
            buttons = win32::MK_MBUTTON;
            msg = lpevent->type == SDL_EVENT_MOUSE_BUTTON_DOWN ? win32::WM_MBUTTONDOWN : win32::WM_MBUTTONUP;
        } else
            return;
        hwndRoot->wMouseButtons = WInputButtons();
        if (lpevent->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            if (lpevent->button.clicks <= 1 || rghwndClick[iButton] != hwnd) rgcClick[iButton] = 1;
            else rgcClick[iButton]++;
            rghwndClick[iButton] = hwnd;
            if ((hwnd->lpclass->wc.style & win32::CS_DBLCLKS) && !(rgcClick[iButton] & 1))
                msg = iButton == 0 ? win32::WM_LBUTTONDBLCLK : iButton == 1 ? win32::WM_RBUTTONDBLCLK : win32::WM_MBUTTONDBLCLK;
            hwndRoot->wMouseButtons |= buttons;
        }
        else
            hwndRoot->wMouseButtons &= ~buttons;
        InputMouse(hwndRoot, (int)lpevent->button.x, (int)lpevent->button.y, hwndRoot->wMouseButtons);
        CompatPostInput(hwnd, msg, WMouseModifiers(hwndRoot->wMouseButtons),
                     win32::MAKELPARAM(x, y));
        return;
    case SDL_EVENT_MOUSE_WHEEL:
        hwndRoot = LpCompatWindowFromId(lpevent->wheel.windowID);
        if (!hwndRoot) return;
        hwnd = win32::GetFocus();
        if (!win32::IsWindowEnabled(hwnd)) hwnd = hwndRoot;
        if (!win32::IsWindowEnabled(hwnd)) return;
        if (!FInputPoint(lpevent->wheel.mouse_x, lpevent->wheel.mouse_y, &x, &y)) return;
        buttons = WInputButtons();
        InputMouse(hwndRoot, x, y, buttons);
        dx = lpevent->wheel.x; dy = lpevent->wheel.y;
        if (lpevent->wheel.direction == SDL_MOUSEWHEEL_FLIPPED) { dx = -dx; dy = -dy; }
        PostCompatWheel(hwnd, win32::WM_MOUSEWHEEL, dy, &hwndRoot->dWheelY, buttons);
        PostCompatWheel(hwnd, win32::WM_MOUSEHWHEEL, dx, &hwndRoot->dWheelX, buttons);
        return;
    default:
        return;
    }
}
