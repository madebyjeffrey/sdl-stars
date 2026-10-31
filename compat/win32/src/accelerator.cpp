/* Accelerator handles own copied entries; module-loaded tables are cached. */
#include "internal.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct win32::HACCEL__ {
    BOOL fAlive;
    int c;
    ACCEL *rg;
    HRSRC hrsrc;
    HACCEL hnext;
};
static HACCEL haccels;
static HACCEL LpAccel(HACCEL haccel) {
    HACCEL lp;
    if (!SDL_IsMainThread()) return NULL;
    for (lp = haccels; lp; lp = lp->hnext)
        if (lp == haccel) return lp->fAlive ? lp : NULL;
    return NULL;
}
HACCEL win32::CreateAcceleratorTableA(LPACCEL rgaccel, int c) {
    HACCEL lp;
    if (!SDL_IsMainThread() || !rgaccel || c <= 0 || (size_t)c > SIZE_MAX / sizeof(ACCEL)) return NULL;
    lp = (HACCEL)(calloc(1, sizeof(*lp)));
    if (!lp) return NULL;
    lp->rg = (ACCEL *)(malloc((size_t)c * sizeof(ACCEL)));
    if (!lp->rg) { free(lp); return NULL; }
    memcpy(lp->rg, rgaccel, (size_t)c * sizeof(ACCEL));
    lp->c = c; lp->fAlive = TRUE; lp->hnext = haccels; haccels = lp;
    return lp;
}
int win32::CopyAcceleratorTableA(HACCEL haccel, LPACCEL rgaccel, int c) {
    HACCEL lp = LpAccel(haccel);
    int i;
    if (!lp) return 0;
    if (!rgaccel) return lp->c;
    if (c <= 0) return 0;
    c = StarsMin(c, lp->c);
    for (i = 0; i < c; i++) { rgaccel[i] = lp->rg[i]; rgaccel[i].fVirt &= ~0x80; }
    return c;
}
BOOL win32::DestroyAcceleratorTable(HACCEL haccel) {
    HACCEL lp = LpAccel(haccel);
    if (!lp) return FALSE;
    if (lp->hrsrc) return TRUE; // Module-loaded tables live until module shutdown.
    lp->fAlive = FALSE; free(lp->rg); lp->rg = NULL;
    return TRUE;
}
HACCEL win32::LoadAcceleratorsA(HINSTANCE hInstance, LPCSTR pszName) {
    HRSRC hrsrc = win32::FindResourceA(hInstance, pszName, RT_ACCELERATOR);
    const BYTE *rgb = (const BYTE *)(win32::LockResource(win32::LoadResource(hInstance, hrsrc)));
    DWORD cb = win32::SizeofResource(hInstance, hrsrc);
    ACCEL *rg;
    HACCEL lp;
    int i, c;
    WORD flags;
    if (!rgb || cb == 0 || cb % 8 || cb / 8 > INT_MAX) return NULL;
    for (lp = haccels; lp; lp = lp->hnext) if (lp->hrsrc == hrsrc) return lp;
    c = (int)(cb / 8);
    rg = (ACCEL *)(calloc((size_t)c, sizeof(*rg)));
    if (!rg) return NULL;
    for (i = 0; i < c; i++) {
        flags = (WORD)(rgb[i * 8] | (rgb[i * 8 + 1] << 8));
        if ((flags & ~0x9f) || ((flags & 0x80) != (i == c - 1 ? 0x80 : 0))) { free(rg); return NULL; }
        rg[i].fVirt = (BYTE)(flags & ~0x80);
        rg[i].key = (WORD)(rgb[i * 8 + 2] | (rgb[i * 8 + 3] << 8));
        rg[i].cmd = (WORD)(rgb[i * 8 + 4] | (rgb[i * 8 + 5] << 8));
    }
    lp = win32::CreateAcceleratorTableA(rg, c);
    free(rg);
    if (lp) lp->hrsrc = hrsrc;
    return lp;
}
int win32::TranslateAcceleratorA(HWND hwnd, HACCEL haccel, LPMSG lpmsg) {
    HACCEL lp = LpAccel(haccel);
    ACCEL accel;
    BYTE mod = 0;
    int i, menu;
    BOOL fVirtual, fChar;
    if (!LpCompatWindow(hwnd) || !lp || !lpmsg) return 0;
    fVirtual = lpmsg->message == WM_KEYDOWN || lpmsg->message == WM_SYSKEYDOWN;
    fChar = lpmsg->message == WM_CHAR || lpmsg->message == WM_SYSCHAR;
    if (!fVirtual && !fChar) return 0;
#ifdef __APPLE__
    /* Command aliases Control for letter commands, without changing the
       keyboard snapshot used by text navigation and game input. */
    if ((win32::GetKeyState(VK_LWIN) | win32::GetKeyState(VK_RWIN)) & 0x8000) {
        if (!fVirtual || lpmsg->wParam < 'A' || lpmsg->wParam > 'Z' || (win32::GetKeyState(VK_CONTROL) & 0x8000)) return 0;
        mod |= FCONTROL;
    }
#endif
    if (win32::GetKeyState(VK_SHIFT) & 0x8000) mod |= FSHIFT;
    if (win32::GetKeyState(VK_CONTROL) & 0x8000) mod |= FCONTROL;
    if (win32::GetKeyState(VK_MENU) & 0x8000) mod |= FALT;
    for (i = 0; i < lp->c; i++) {
        accel = lp->rg[i];
        if (accel.key != lpmsg->wParam || !!(accel.fVirt & FVIRTKEY) != fVirtual) continue;
        if ((accel.fVirt & (FSHIFT | FCONTROL | FALT)) != mod) continue;
        if (!win32::IsWindowEnabled(hwnd) || win32::GetCapture()) return 1;
        menu = ICompatAccelMenu(hwnd, accel.cmd, TRUE);
        if (menu != 2 && !(menu != 0 && win32::IsIconic(hwnd)) && win32::IsWindow(hwnd) && win32::IsWindowEnabled(hwnd) && !win32::GetCapture())
            win32::SendMessageA(hwnd, WM_COMMAND, MAKEWPARAM(accel.cmd, 1), 0);
        return 1;
    }
    return 0;
}
void CompatShutdownAccelerators(void) {
    HACCEL lp, next;
    for (lp = haccels; lp; lp = next) { next = lp->hnext; free(lp->rg); free(lp); }
    haccels = NULL;
}
