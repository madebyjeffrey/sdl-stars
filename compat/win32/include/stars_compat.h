#ifndef STARS_COMPAT_H
#define STARS_COMPAT_H

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include "fileio.h"
#include "messagepayload.h"
/* Keep one native window and its completed frame across same-sized modal
 * wizard pages. Logical HWNDs and controls still have independent lifetimes.
 * End the sequence on every exit path; nested sequences are rejected without changing state.
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
struct STARSCOMPATWINDOW {
    std::uint64_t id, idParent, idOwner;
    LONG_PTR idControl;
    DWORD dwStyle, dwExStyle;
    UINT iControl;
    BOOL fDialog, fVisible, fEnabled, fFocused, fCaptured;
    RECT rcWindow, rcClient;
    char *pszClass, *pszText;
    BOOL fUtf8Text;
};
struct STARSCOMPATSNAPSHOT {
    STARSCOMPATWINDOW *rgWindow;
    std::size_t cWindow;
};
BOOL StarsCompatCaptureHierarchy(STARSCOMPATSNAPSHOT *lpsnap);
void StarsCompatFreeHierarchy(STARSCOMPATSNAPSHOT *lpsnap);
/* JSON uses UTF-8 text and decimal strings for IDs, to preserve 64-bit values. */
BOOL StarsCompatWriteHierarchy(const STARSCOMPATSNAPSHOT *lpsnap, NATIVEFILE *lpfile);
std::uint64_t StarsCompatWindowId(HWND hwnd);
HWND StarsCompatResolveWindow(std::uint64_t id);
/* Unique immediate child; zero means absent or ambiguous. NULL class matches any. */
std::uint64_t StarsCompatFindControl(std::uint64_t idParent, LONG_PTR idControl, LPCSTR pszClass);
/* Modeless, read-only viewer. Ctrl+Shift+F12 opens/refreshes it in the SDL client. */
HWND StarsCompatShowInspector(void);
/* Optional application-owned semantic object viewer; NULL removes the button. */
void StarsCompatSetObjectInspector(HWND (*pfnShow)(void));

/* Release classes, handle tokens, messages and the runtime's SDL video reference.
 * Call on the UI thread after the message loop, never from inside a WNDPROC.
 * All handles and class atoms must be discarded before using the runtime again.
 */
void StarsCompatShutdown(void);
BOOL StarsCompatCanShutdown() noexcept;
/* Non-owning numeric messages: the caller keeps HWND valid until delivery.
 * Delayed chooser results use the owned logical-ID handoff below. */
BOOL StarsCompatPostMessageAsync(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* Move-only one-shot payload. Ownership is consumed on entry, including failure.
 * Capture a logical window ID on the UI thread before starting a foreign dialog.
 * wParam is an opaque delivery token; borrow its payload only during dispatch.
 * Destroying the target, dropping delivery or shutting down releases the payload.
 */
BOOL StarsCompatPostOwnedMessageAsync(std::uint64_t idWindow, UINT msg, STARSCOMPATPAYLOAD payload, LPARAM lParam);
void *PvCompatMessagePayload(WPARAM id) noexcept;

/* Stop native callbacks, timers and windows without calling window procedures.
 * Session teardown calls this before releasing game state; process entry also
 * calls it as an idempotent fallback. Handles become invalid. UI thread only,
 * outside a window procedure.
 */
void StarsCompatProcessExit(void);

BOOL StarsCompatTextInputRejected(HWND hwnd);
BOOL StarsCompatSetWindowTitleUtf8(HWND hwnd, const char *psz);

/* Prepare a hidden top-level window's first ShowWindow(SW_SHOW) without
 * exposing it to the desktop while the application initializes.
 */
BOOL StarsCompatSetInitialShowCommand(HWND hwnd, int sw);



#endif
