#ifndef STARS_COMPAT_H
#define STARS_COMPAT_H

#include <windows.h>
#include <stdint.h>
#include <stdio.h>

/* Keep one native window across an application's same-sized modal wizard pages.
 * Logical HWNDs and controls still have independent lifetimes. End the sequence
 * on every exit path; nested sequences are rejected without changing state.
 */
BOOL StarsCompatBeginDialogSequence(HWND hwndOwner);
void StarsCompatEndDialogSequence(void);

/* Message boxes owned by this window, its children or its owned windows can
 * be silent. Other windows and explicit MessageBeep calls keep their sounds.
 */
BOOL StarsCompatSetWindowAlertSounds(HWND hwnd, BOOL fEnable);

/* UI-thread inspection. IDs identify logical windows, never native SDL windows,
 * and are not reused during the process lifetime (including runtime restarts).
 * Snapshot strings are UTF-8; fUtf8Text is always true.
 * Strings are copied without calling application procedures.
 * Password text is redacted. Initialize snapshots to zero; free before reuse.
 */
typedef struct STARSCOMPATWINDOW {
    uint64_t id, idParent, idOwner;
    LONG_PTR idControl;
    DWORD dwStyle, dwExStyle;
    UINT iControl;
    BOOL fDialog, fVisible, fEnabled, fFocused, fCaptured;
    RECT rcWindow, rcClient;
    char *pszClass, *pszText;
    BOOL fUtf8Text;
} STARSCOMPATWINDOW;
typedef struct STARSCOMPATSNAPSHOT {
    STARSCOMPATWINDOW *rgWindow;
    size_t cWindow;
} STARSCOMPATSNAPSHOT;
BOOL StarsCompatCaptureHierarchy(STARSCOMPATSNAPSHOT *lpsnap);
void StarsCompatFreeHierarchy(STARSCOMPATSNAPSHOT *lpsnap);
/* JSON uses UTF-8 text and decimal strings for IDs, to preserve 64-bit values. */
BOOL StarsCompatWriteHierarchy(const STARSCOMPATSNAPSHOT *lpsnap, FILE *lpfile);
uint64_t StarsCompatWindowId(HWND hwnd);
HWND StarsCompatResolveWindow(uint64_t id);
/* Unique immediate child; zero means absent or ambiguous. NULL class matches any. */
uint64_t StarsCompatFindControl(uint64_t idParent, LONG_PTR idControl, LPCSTR pszClass);
/* Modeless, read-only viewer. Ctrl+Shift+F12 opens/refreshes it in the SDL client. */
HWND StarsCompatShowInspector(void);
/* Optional application-owned semantic object viewer; NULL removes the button. */
void StarsCompatSetObjectInspector(HWND (*pfnShow)(void));

/* Release classes, handle tokens, messages and the runtime's SDL video reference.
 * Call on the UI thread after the message loop, never from inside a WNDPROC.
 * All handles and class atoms must be discarded before using the runtime again.
 */
void StarsCompatShutdown(void);
/* Post an application message from an SDL dialog callback, which may run off-thread. */
BOOL StarsCompatPostMessageAsync(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* Final process cleanup after WinMain has released application state.
 * Unlike normal shutdown, no window procedure is called. All handles become
 * invalid; call only on the UI thread and outside a window procedure.
 */
void StarsCompatProcessExit(void);

BOOL StarsCompatTextInputRejected(HWND hwnd);
BOOL StarsCompatSetWindowTitleUtf8(HWND hwnd, const char *psz);

/* Prepare a hidden top-level window's first ShowWindow(SW_SHOW) without
 * exposing it to the desktop while the application initializes.
 */
BOOL StarsCompatSetInitialShowCommand(HWND hwnd, int sw);

#endif
