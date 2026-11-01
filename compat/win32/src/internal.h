#ifndef STARS_COMPAT_INTERNAL_H
#define STARS_COMPAT_INTERNAL_H
#include <windows.h>
#include <callbackcontext.h>
#include "encoding.h"
#include "errors.h"
#include "fileio.h"
#include "ownership.h"
#include <SDL3/SDL.h>
#include "platform.h"
void CompatAbortCallback(HWND hwnd) noexcept;

struct COMPATPRINT;

void CompatConsumeAsyncMessage(const SDL_Event *event);
void CompatStartAsyncMessages() noexcept;
void CompatStopAsyncMessages() noexcept;
inline constexpr auto COMPAT_ASYNC_POST_CODE = 0x53544152;
HDC HdcCompatPrinter(COMPATPRINT *lpprint, int cx, int cy);
COMPATPRINT *LpCompatPrinter(HDC hdc);
SDL_Surface *LpCompatPrinterSurface(HDC hdc);
int ICompatPrinterCaps(COMPATPRINT *lpprint, int index);
void CompatDeletePrinter(COMPATPRINT *lpprint) noexcept;
BOOL FCompatPrinterBusy(COMPATPRINT *lpprint);
void CompatSetDialogError(DWORD dw);

struct COMPATFONT;

struct COMPATLIST;

struct COMPATTOOLBAR;

void CompatDestroyToolbar(HWND hwnd);
LRESULT  CompatToolbarProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);
LRESULT  CompatToolbarTipProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);
struct COMPATCOMBO {
    HWND hwndPopup, hwndEdit, hwndCapture;
    WNDPROC lpfnEdit;
    int cyDrop, cyField, cxDrop, iSave, iHot;
    BOOL fDropped, fOpening, fClosing, fSync, fExtended, fSizing;
};
struct COMPATSCROLL { int iMin, iMax, iPos; UINT cPage; BOOL fDisable; };
struct COMPATFONTDATA { const BYTE *rgb; size_t cb; };
extern const COMPATFONTDATA rgCompatFontData[12];
COMPATFONT *LpNewCompatFont(const LOGFONTA *lplf);
void FreeCompatFont(COMPATFONT *lpfont) noexcept;
BOOL FCompatFontSize(COMPATFONT *lpfont, LPCSTR psz, int cch, LPSIZE lpsize);
BOOL FCompatFontMetrics(COMPATFONT *lpfont, LPTEXTMETRICA lptm);
BOOL FCompatFontDraw(COMPATFONT *lpfont, LPCSTR psz, int cch, const INT *rgdx,
                    void (*pfn)(void *, int64_t, int64_t, BYTE), void *lpv);
BOOL FCompatFontSizeUtf8(COMPATFONT *lpfont, LPCSTR psz, int cch, LPSIZE lpsize);
BOOL FCompatFontDrawUtf8(COMPATFONT *lpfont, LPCSTR psz, int cch,
                        void (*pfn)(void *, int64_t, int64_t, BYTE), void *lpv);
BOOL FCompatFontDrawScaled(COMPATFONT *lpfont, LPCSTR psz, int cch, const INT *rgdx, int iScale,
                          void (*pfn)(void *, int64_t, int64_t, BYTE), void *lpv);
void CompatShutdownFonts(void);

struct COMPATCLASS {
    WNDCLASSA wc;
    ATOM atom;
    NATIVEOWNER<char, SDL_free> pszClass;
    NATIVEOWNER<char, SDL_free> pszMenu;
    NATIVEARRAYOWNER<BYTE, std::free> rgbExtra;
    struct COMPATCLASS *lpNext;
};

// These native deleters keep member types stable across private SDK test redirects.
void FreeCompatSurface(SDL_Surface *lp) noexcept;
void FreeCompatWindow(SDL_Window *lp) noexcept;
void FreeCompatRenderer(SDL_Renderer *lp) noexcept;
void FreeCompatTexture(SDL_Texture *lp) noexcept;

struct win32::HWND__ {
    COMPATCLASS *lpclass;
    NATIVEOWNER<SDL_Window, FreeCompatWindow> lpWindow;
    SDL_WindowID id;
    uint64_t idInspect;
    WNDPROC lpfnWndProc;
    void *lpvCallbackContext;
    LONG_PTR lUserData, idChild;
    HWND hwndParent, hwndOwner, hwndRestoreFocus;
    HMENU hmenu;
    HBITMAP hbmpMenu;
    NATIVEARRAYOWNER<RECT, std::free> rgrcMenu;
    int cMenu, cyMenu, cxMenu, iMenuHot;
    WPARAM vkMenuSuppress;
    BOOL fMenuDirty, fMenuLayoutDirty, fCreated, fMenuAltUsed;
    HWND hwndFirstChild, hwndZPrev, hwndZNext;
    BOOL fZeroX, fZeroY;
    RECT rcChild;
    char *pszTitle;
    BYTE *rgbExtra;
    DWORD dwStyle, dwExStyle;
    UINT iControl, bstCheck;
    BOOL fPressed, fTracking, fSpace;
    HFONT hfontControl;
    UINT ichAnchor, ichCaret, cchLimit;
    int xEditScroll, yEditScroll;
    RECT rcEditFormat;
    BOOL fEditFormat, fEditManualScroll, fEditTyping, fEditCaret;
    BOOL fEditScrollSync, fEditV, fEditH;
    UINT ichTypingEnd;
    UINT iEditUndoAction;
    UINT_PTR idEditCaretTimer;
    BOOL fModified, fEditInputRejected;
    char *pszEditUndo, *pszEditComposition;
    int ichComposition, cchComposition;
    UINT ichUndoPosition, cchUndoBefore, cchUndoAfter;
    UINT swState;
    UINT swInitial;
    RECT rcNormal;
    BOOL fRestoreMax;
    BOOL fCaptionActive;
    BOOL fDialog, fModal, fEndDialog;
    BOOL fSilentAlerts;
    UINT cNativeFileDialogs;
    BOOL fKeepNative, fDeferPresent;
    UINT cPresentDefer;
    BOOL fPresentPending;
    DLGPROC lpfnDlg;
    INT_PTR lDlgResult, lDlgMsgResult;
    HFONT hfontDialog;
    int cxDlgUnit, cyDlgUnit;
    COMPATSCROLL rgscr[2];
    HWND rghwndScroll[2];
    BOOL fScrollLayout, fNcScroll;
    UINT sbScrollTrack;
    int iScrollTrack, dScrollGrab;
    UINT_PTR idScrollTimer;
    LPARAM lScrollPoint;
    // Callback frames retain control storage after immediate logical teardown.
    std::shared_ptr<COMPATTOOLBAR> lptoolbar;
    std::shared_ptr<COMPATLIST> lplist;
    std::shared_ptr<COMPATCOMBO> lpcombo;
    WPARAM wMouseButtons;
    double dWheelX, dWheelY;
    BOOL fAlive, fDestroying, fVisible;
    NATIVEOWNER<SDL_Surface, FreeCompatSurface> lpCanvas, lpComposite;
    int iCanvasScale;
    NATIVEOWNER<SDL_Renderer, FreeCompatRenderer> lpRenderer;
    NATIVEOWNER<SDL_Texture, FreeCompatTexture> lpTexture;
    RECT rcUpdate;
    NATIVEARRAYOWNER<RECT, std::free> rgrcUpdate;
    size_t crcUpdate;
    BOOL fUpdate, fErase;
    HDC hdcPaint;
    struct win32::HWND__ *lpNext;
};

struct COMPATRESOURCE {
    const char *pszName;
    WORD id;
    const BYTE *rgb;
    size_t cb;
    WORD type;
};

extern const COMPATRESOURCE rgCompatResources[];
extern const size_t cCompatResources;
extern const COMPATRESOURCE rgCompatUIResources[];
extern const size_t cCompatUIResources;
extern const COMPATRESOURCE rgCompatImageResources[];
extern const size_t cCompatImageResources;
// Keep resource member types identical when a test substitutes an SDL call.
void FreeCompatCursor(SDL_Cursor *lp) noexcept;

struct COMPATIMAGE {
    BOOL fAlive, fCursor, fShared, fMono, fInvert, fAppIcon;
    // Reverse member destruction releases the cursor before its image storage.
    NATIVEARRAYOWNER<BYTE, std::free> rgbAnd;
    NATIVEOWNER<SDL_Surface, FreeCompatSurface> lpXor;
    UINT xHot, yHot, idSystem;
    NATIVEOWNER<SDL_Cursor, FreeCompatCursor> lpCursor;
    HRSRC hrsrc;
    struct COMPATIMAGE *lpNext;
};
COMPATIMAGE *LpCompatImage(HICON hicon);
HICON HiconCompatResource(const BYTE *rgb, size_t cb, BOOL fCursor);
HBITMAP HbmpCompatSurface(SDL_Surface *lpSurface, BOOL fMono);
void CompatSetWindowIcon(SDL_Window *lpWindow, HICON hicon);
extern const BYTE rgbCompatAppIcon[];
extern const size_t cbCompatAppIcon;
void CompatShutdownImages(void);
void CompatCursorPoint(HWND hwnd, int x, int y);
void CompatCursorLeave(HWND hwnd);
void CompatDrawCursor(HWND hwnd, SDL_Surface *lpSurface);
HMENU HmenuCompatTemplate(const BYTE *rgb, size_t cb);
struct COMPATDLGITEM {
    DWORD dwStyle, dwExStyle;
    int16_t x, y, cx, cy;
    WORD id, idClass, idTitle;
    char *pszClass, *pszTitle;
    const BYTE *rgbCreate;
    WORD cbCreate;
};
struct COMPATDLGTEMPLATE {
    DWORD dwStyle, dwExStyle;
    int16_t x, y, cx, cy;
    WORD cItems, idMenu, idClass, cPoints;
    char *pszMenu, *pszClass, *pszTitle, *pszFont;
    COMPATDLGITEM *rgItems;
};
BOOL FCompatDialogTemplate(const BYTE *rgb, size_t cb, COMPATDLGTEMPLATE *lpdlg);
void CompatFreeDialogTemplate(COMPATDLGTEMPLATE *lpdlg);
LRESULT  CompatDialogProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);
void CompatDestroyDialog(HWND hwnd);
void CompatDestroyList(HWND hwnd);
HWND HwndCompatDynamicDialog(LPCSTR pszTitle, DWORD dwStyle, DWORD dwExStyle, const RECT *lprc,
                             HWND hwndOwner, DLGPROC lpfn);
HWND HwndCompatFirstTop(void);
HWND HwndCompatRegistry(void);
void CompatShutdownInspector(void);
LRESULT  CompatMessageTextProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);
void CompatShutdownAudio(void) noexcept;

struct COMPATDIB {
    int cx, cy, cBits, cColors;
    BOOL fTopDown;
    size_t cbRow, cbBits, cbInfo;
    COLORREF rgcr[256];
    const BYTE *lpBits;
};

BOOL FCompatDib(const void *lpInfo, size_t cbInfo, const void *lpBits, size_t cbBits, COMPATDIB *lpdib);
BOOL FCompatNativeDib(const void *lpInfo, size_t cbInfo, const void *lpBits, size_t cbBits, COMPATDIB *lpdib);
SDL_Surface *LpCompatDibSurface(const COMPATDIB *lpdib);
HBITMAP HbmpCompatResource(const BYTE *rgb, size_t cb);

BOOL FCompatControlClass(LPCSTR pszClass, HINSTANCE hInstance);
BOOL FCompatVideo(void);
BOOL FCompatNativeTitle(SDL_Window *lpWindow, LPCSTR pszTitle);
void CompatRememberNormal(HWND hwnd);
void CompatWindowState(HWND hwnd, UINT sw, BOOL fNotify);
LRESULT  CompatEditProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);
LRESULT  CompatListProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);
unsigned NCompatListMutation(HWND hwnd);
LRESULT  CompatComboProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);
LRESULT  CompatComboPopupProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);
LRESULT  CompatScrollProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);
HWND LpCompatWindow(HWND hwnd);
HWND LpCompatWindowFromId(SDL_WindowID id);
void CompatFocusWindow(HWND hwnd);
HWND HwndCompatRoot(HWND hwnd);
HWND HwndCompatMouse(HWND hwnd, int *lpx, int *lpy);
BOOL FCompatPresent(HWND hwnd);
BOOL FCompatRefreshPresent(HWND hwnd);
BOOL FCompatCanvas(HWND hwnd);
int ICompatCanvasScale(HWND hwnd);
BOOL FCompatSetDCScale(HDC hdc, int iScale);
BOOL FCompatPaintTree(HWND hwnd);
void CompatFlushPresent(void);
BOOL FCompatPendingDialogSequence(HWND hwndOwner);
void CompatRestoreOwnerFocus(HWND hwndOwner, HWND hwndSaved);
SDL_Surface *LpCompatBitmapSurface(HBITMAP hbmp);
int CyCompatMenuBar(HWND hwnd);
BOOL FCompatDrawMenuBar(HWND hwnd, SDL_Surface *lpSurface);
void CompatFreeMenuBar(HWND hwnd);
BOOL FCompatMenuMessage(HWND hwnd, UINT msg, WPARAM w, LPARAM l);
void CompatRouteEvent(const SDL_Event *lpevent);
BOOL CompatPostInput(HWND hwnd, UINT msg, WPARAM w, LPARAM l);
void CompatCopyInput(BYTE *rgb, LPPOINT lppt);
void CompatReadInput(const BYTE *rgb);
void CompatResetInput(void);
HWND HwndCompatPointerRoot(void);
void CompatTextInput(HWND hwnd, const char *psz);
void CompatPurgeMessages(HWND hwnd);
void CompatClearMessages(void);
int CCompatDispatch(void);
BOOL CompatPeekPaint(LPMSG lpmsg, HWND hwnd, UINT msgMin, UINT msgMax);
void CompatDestroyPaint(HWND hwnd);
BOOL CompatGetSurfaceRect(HWND hwnd, RECT *lprc);
void CompatLayoutScrollbars(HWND hwnd);
void CompatDrawClassicEdge(HDC hdc, const RECT *lprc, BOOL fSunken, BOOL fRight);
void CompatShutdownGdi(void);
void CompatShutdownMemory(void);
void CompatShutdownFiles(void);
void CompatShutdownMenus(void);
void CompatShutdownAccelerators(void);
int ICompatAccelMenu(HWND hwnd, UINT cmd, BOOL fNotify);
void CompatDestroyWindowMenu(HWND hwnd);
BOOL FCompatMenuFocus(HWND hwnd, BOOL fGain);
BOOL CompatQueueTimer(HWND hwnd, UINT_PTR id, TIMERPROC lpfn, void *lpvCallbackContext);
void CompatGenerateTimer(HWND hwnd, UINT msgMin, UINT msgMax);
void CompatTimerDequeued(HWND hwnd, UINT_PTR id);
int CMillisCompatTimerWait(HWND hwnd, UINT msgMin, UINT msgMax);
void CompatPurgeTimers(HWND hwnd);
void CompatClearTimers(void);
void CompatEditComposition(HWND hwnd, const char *pszUtf, int ich, int cch);

char *PszCompatFileUri(const char *pszPath);
BOOL FCompatOpenDocument(const char *pszPath);

// C++ callback thunks return errors through pending state, never through C.
void CompatInvokeCleanup(void (*lpfn)(void *), void *lpv, HWND hwnd) noexcept;
LRESULT CompatInvokeWindowProc(WNDPROC lpfn, HWND hwnd, UINT msg, WPARAM w, LPARAM l) noexcept;
void CompatInvokeTimerProc(TIMERPROC lpfn, HWND hwnd, UINT msg, UINT_PTR id, DWORD tick) noexcept;
HWND CompatInvokeInspector(HWND (*pfn)(void), HWND hwnd) noexcept;


#endif
