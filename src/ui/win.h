#ifndef STARS_DECOMPILED_WIN_H
#define STARS_DECOMPILED_WIN_H

// The SDL desktop interface: wrapper API, game code (common.h) and UI headers.
// The game code includes only common.h.

#include <windows.h>
#include <commctrl.h>

#include <direct.h>
#include <io.h>

#include "common.h"

// Application messages (WM_USER + 0x64...).
#define WM_STARS_STARTUP  0x0464
#define WM_STARS_HOST     0x0465
#define WM_STARS_CONTINUE 0x0466
#define WM_STARS_SCAN_RESIZE (WM_APP + 2)

#ifdef __cplusplus
extern "C" {
#endif

#include "battleui.h"
#include "buildui.h"
#include "createui.h"
#include "fileui.h"
#include "globalsui.h"
#include "init.h"
#include "logui.h"
#include "libraryui.h"
#include "mdi.h"
#include "mineui.h"
#include "msgui.h"
#include "nativeui.h"
#include "planetui.h"
#include "popup.h"
#include "produceui.h"
#include "raceui.h"
#include "reportui.h"
#include "researchui.h"
#include "scan.h"
#include "scaninspect.h"
#include "ship2ui.h"
#include "shipui.h"
#include "stars.h"
#include "tb.h"
#include "thingui.h"
#include "tutor.h"
#include "tutor2.h"
#include "utilgenui.h"
#include "utilui.h"
#include "vcr.h"

// All window text and file paths use UTF-8.
BOOL FNativeOpenFileName(OPENFILENAMEA *lpof);
BOOL FNativeSaveFileName(OPENFILENAMEA *lpof);
DWORD DwNativeFileDialogError(void);
int CchGetUtf8WindowText(HWND hwnd, char *psz, int cchMax);
BOOL FSetUtf8WindowText(HWND hwnd, const char *psz);
BOOL FSetNativeWindowTitle(HWND hwnd, const char *psz);

#ifdef __cplusplus
// Private test implementations retain the same C ABI as their adapters.
decltype(InitInstance) STARS_TEST_IMPL(InitInstance);
decltype(AskSaveDialog) STARS_TEST_IMPL(AskSaveDialog);
decltype(PromptPassword) STARS_TEST_IMPL(PromptPassword);
decltype(FPromptLegacyRacePassword) STARS_TEST_IMPL(FPromptLegacyRacePassword);
decltype(NewGameWizard) STARS_TEST_IMPL(NewGameWizard);
decltype(FNativeSaveFileName) STARS_TEST_IMPL(FNativeSaveFileName);
decltype(FNativeOpenFileName) STARS_TEST_IMPL(FNativeOpenFileName);
}
#endif

#endif
