#include "../../compat/win32/src/external_io.h"
/* Printer DC lifetime, raster pages, atomic PDF output and real process spooling. */
#include <windows.h>
#include <stars_compat.h>
#include "fileio.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#define setenv SDL_setenv_unsafe
#define unsetenv SDL_unsetenv_unsafe
#ifndef STARS_TEST_PRINT_OUTPUT
#define STARS_TEST_PRINT_OUTPUT "/tmp/stars-print-test-output"
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static char szChosen[4096];
static HWND hwndOwner;
static HDC hdcChoosing;
static int cErrors;
static BOOL fChooseCancel;
static int cViewer, cNotice;
static BOOL fViewerAccept = TRUE;
static const char *pszViewerExpected;
static char szViewerUri[16384];
static LRESULT CALLBACK OwnerProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND dlg;
    char sz[64];
    if (msg != WM_TIMER) return DefWindowProc(hwnd, msg, w, l);
    for (dlg = GetWindow(hwnd, GW_HWNDFIRST); dlg; dlg = GetWindow(dlg, GW_HWNDNEXT)) {
        GetWindowText(dlg, sz, sizeof(sz));
        if (strcmp(sz, "Print to PDF") == 0 && GetDlgItem(dlg, 0x480)) {
            if (DeleteDC(hdcChoosing) || GetLastError() != ERROR_BUSY) cErrors++;
            SetWindowText(GetDlgItem(dlg, 0x480), szChosen); PostMessage(dlg, WM_COMMAND, fChooseCancel ? IDCANCEL : IDOK, 0); break;
        }
    }
    return 0;
}
static int InitPrinter(PRINTDLG *lp, const char *pszFile) {
    memset(lp, 0, sizeof(*lp)); lp->lStructSize = sizeof(*lp); lp->Flags = PD_RETURNDEFAULT | PD_RETURNDC;
    if (IsWindow(hwndOwner)) lp->hwndOwner = hwndOwner;
    if (pszFile) setenv("STARS_SDL_PRINT_FILE", pszFile, 1); else unsetenv("STARS_SDL_PRINT_FILE");
    pszViewerExpected = pszFile ? pszFile : szChosen;
    return PrintDlg(lp);
}
static void FreePrinter(PRINTDLG *lp) { DeleteDC(lp->hDC); GlobalFree(lp->hDevMode); GlobalFree(lp->hDevNames); }
static int FPrefix(const char *pszFile, const char *psz) {
    NATIVEFILE *lp = PfNativeOpen(pszFile, "rb");
    char sz[16] = {};
    size_t cb = strlen(psz), cbRead;
    if (!lp) return 0;
    if (cb > sizeof(sz)) { ICloseNativeFile(lp); return 0; }
    cbRead = CReadNative(sz, 1, cb, lp); ICloseNativeFile(lp); return cbRead == cb && memcmp(sz, psz, cb) == 0;
}
extern "C" bool SDLCALL StarsTest_SDL_OpenURL(const char *pszUri) {
    cViewer++;
    if (!pszViewerExpected || !FPrefix(pszViewerExpected, "%PDF-") || strncmp(pszUri, "file:///", 8)) cErrors++;
    snprintf(szViewerUri, sizeof(szViewerUri), "%s", pszUri);
    return fViewerAccept;
}
extern "C" bool SDLCALL StarsTest_SDL_ShowSimpleMessageBox(SDL_MessageBoxFlags flags, const char *pszTitle, const char *pszText, SDL_Window *lpParent) {
    (void)lpParent;
    cNotice++;
    if (flags != SDL_MESSAGEBOX_ERROR || strcmp(pszTitle, "PDF saved") ||
        !strstr(pszText, pszViewerExpected)) cErrors++;
    return true;
}
int main(int argc, char **argv) {
    const char *pszDir = argc == 2 ? argv[1] : STARS_TEST_PRINT_OUTPUT;
    PRINTDLG pd;
    DEVMODE *lpdm;
    DEVNAMES *lpn;
    HDC hdcOther;
    HBITMAP hbmp;
    HFONT hfont;
    HGDIOBJ hfontOld;
    LOGFONT lf = {};
    HBRUSH hbr;
    WNDCLASS wc = {};
    char szPages[4096], szAbort[4096], szImplicit[4096], szPaper[4096];
#ifndef _WIN32
    char szMock[4096], szLog[4096], szCapture[4096], szPath[8192], szArg[4096], szRetained[4096];
    const char *pszOldPath = getenv("PATH");
#endif
    NATIVEFILE *lpfile;
    POINT pt;
    RECT rc;
    int query = NEWFRAME;
    COMPATEXTERNALIO io_compatexternalio;
    io_compatexternalio.pfnOpenURL = StarsTest_SDL_OpenURL;
    io_compatexternalio.pfnShowSimpleMessageBox = StarsTest_SDL_ShowSimpleMessageBox;
    COMPATEXTERNALIOSCOPE scope_compatexternalio(io_compatexternalio);

    CHECK(argc <= 2 && strlen(pszDir) < 2000);
    CHECK(ICreateNativeDirectory(pszDir, 0700) == 0 || IAccessNativePath(pszDir, 0) == 0);
    snprintf(szPages, sizeof(szPages), "%s/pages.pdf", pszDir);
    snprintf(szAbort, sizeof(szAbort), "%s/abort.pdf", pszDir);
    snprintf(szImplicit, sizeof(szImplicit), "%s/implicit.pdf", pszDir);
    snprintf(szPaper, sizeof(szPaper), "%s/landscape-a4.pdf", pszDir);
#ifndef _WIN32
    snprintf(szMock, sizeof(szMock), "%s/lp", pszDir);
    snprintf(szLog, sizeof(szLog), "%s/spool-args.txt", pszDir);
    snprintf(szCapture, sizeof(szCapture), "%s/spooled.pdf", pszDir);
#endif
    unsetenv("STARS_SDL_PRINTER"); unsetenv("STARS_SDL_PRINT_PAPER"); unsetenv("STARS_SDL_PRINT_ORIENTATION");
    CHECK(InitPrinter(&pd, szPages) && pd.hDC && pd.hDevMode && pd.hDevNames && pd.nCopies == 1);
    CHECK(CommDlgExtendedError() == 0);
    lpdm = (DEVMODE*)(GlobalLock(pd.hDevMode)); CHECK(lpdm && lpdm->dmSize == sizeof(*lpdm) && lpdm->mode.printer.dmPaperSize == DMPAPER_LETTER);
    CHECK(lpdm->mode.printer.dmOrientation == DMORIENT_PORTRAIT && lpdm->mode.printer.dmPrintQuality == 300); GlobalUnlock(pd.hDevMode);
    lpn = (DEVNAMES*)(GlobalLock(pd.hDevNames)); CHECK(lpn && lpn->wDefault == 1);
    CHECK(strcmp((char *)lpn + lpn->wDeviceOffset, "Stars PDF") == 0); GlobalUnlock(pd.hDevNames);
    CHECK(GetDeviceCaps(pd.hDC, HORZRES) == 2400 && GetDeviceCaps(pd.hDC, VERTRES) == 3150);
    CHECK(GetDeviceCaps(pd.hDC, LOGPIXELSX) == 300 && GetDeviceCaps(pd.hDC, TECHNOLOGY) == DT_RASPRINTER);
    CHECK(Escape(pd.hDC, GETPRINTINGOFFSET, 0, NULL, &pt) == 1 && pt.x == 75 && pt.y == 75);
    CHECK(Escape(pd.hDC, GETPHYSPAGESIZE, 0, NULL, &pt) == 1 && pt.x == 2550 && pt.y == 3300);
    CHECK(Escape(pd.hDC, QUERYESCSUPPORT, sizeof(query), (char *)&query, NULL) == 1);
    CHECK(Escape(pd.hDC, ENDDOC, 0, NULL, NULL) == SP_ERROR);
    lpfile = PfNativeOpen(szPages, "wb"); CHECK(lpfile); CchNativePrintf(lpfile, "%s", "sentinel"); ICloseNativeFile(lpfile);
    CHECK(Escape(pd.hDC, STARTDOC, 9, "Page test", NULL) > 0);
    CHECK(FPrefix(szPages, "sentinel"));
    hbr = CreateSolidBrush(RGB(255, 0, 0)); CHECK(hbr); rc = (RECT){100, 100, 500, 400}; CHECK(FillRect(pd.hDC, &rc, hbr)); DeleteObject(hbr);
    lf.lfHeight = -33; strcpy(lf.lfFaceName, "sans-serif");
    hfont = CreateFontIndirect(&lf); CHECK(hfont); hfontOld = SelectObject(pd.hDC, hfont); CHECK(hfontOld);
    CHECK(TextOut(pd.hDC, 700, 100, "Stars map page one", 18));
    CHECK(Escape(pd.hDC, NEWFRAME, 0, NULL, NULL) == 1 && FPrefix(szPages, "sentinel") && cViewer == 0);
    CHECK(GetPixel(pd.hDC, 120, 120) == RGB(255, 255, 255));
    // The game repeats STARTDOC for every map page. Its rejected second call
    // must leave the original job intact, rather than replace its first page.
    CHECK(Escape(pd.hDC, STARTDOC, 9, "Page test", NULL) == SP_ERROR);
    hbr = CreateSolidBrush(RGB(0, 0, 255)); CHECK(hbr); rc = (RECT){400, 600, 800, 900}; CHECK(FillRect(pd.hDC, &rc, hbr)); DeleteObject(hbr);
    CHECK(Escape(pd.hDC, NEWFRAME, 0, NULL, NULL) == 1);
    CHECK(Escape(pd.hDC, ENDDOC, 0, NULL, NULL) == 1 && FPrefix(szPages, "%PDF-") && cViewer == 1);
    CHECK(SelectObject(pd.hDC, hfontOld)); CHECK(DeleteObject(hfont));
    hdcOther = CreateCompatibleDC(pd.hDC); CHECK(hdcOther);
    hbmp = CreateCompatibleBitmap(pd.hDC, 10, 10); CHECK(hbmp);
    CHECK(!SelectObject(pd.hDC, hbmp)); CHECK(DeleteObject(hbmp)); CHECK(DeleteDC(hdcOther));
    CHECK(Escape(pd.hDC, 99999, 0, NULL, NULL) == 0); FreePrinter(&pd);
    CHECK(InitPrinter(&pd, szAbort));
    lpfile = PfNativeOpen(szAbort, "wb"); CHECK(lpfile); CchNativePrintf(lpfile, "%s", "sentinel"); ICloseNativeFile(lpfile);
    CHECK(Escape(pd.hDC, STARTDOC, 5, "Abort", NULL) > 0); CHECK(Escape(pd.hDC, NEWFRAME, 0, NULL, NULL) == 1);
    CHECK(Escape(pd.hDC, ABORTDOC, 0, NULL, NULL) == 1 && FPrefix(szAbort, "sentinel"));
    CHECK(Escape(pd.hDC, STARTDOC, 5, "Abort", NULL) > 0); FreePrinter(&pd); CHECK(FPrefix(szAbort, "sentinel"));
    CHECK(InitPrinter(&pd, szImplicit)); CHECK(Escape(pd.hDC, STARTDOC, 8, "Implicit", NULL) > 0);
    CHECK(SetPixel(pd.hDC, 10, 10, RGB(0, 200, 0)) == RGB(0, 200, 0));
    CHECK(Escape(pd.hDC, ENDDOC, 0, NULL, NULL) == 1); FreePrinter(&pd);
    setenv("STARS_SDL_PRINT_PAPER", "a4", 1); setenv("STARS_SDL_PRINT_ORIENTATION", "landscape", 1);
    CHECK(InitPrinter(&pd, szPaper)); CHECK(GetDeviceCaps(pd.hDC, HORZRES) == 3358 && GetDeviceCaps(pd.hDC, VERTRES) == 2330);
    CHECK(Escape(pd.hDC, STARTDOC, 8, "A4 paper", NULL) > 0); CHECK(Escape(pd.hDC, NEWFRAME, 0, NULL, NULL) == 1);
    CHECK(Escape(pd.hDC, ENDDOC, 0, NULL, NULL) == 1); FreePrinter(&pd);
    unsetenv("STARS_SDL_PRINT_PAPER"); unsetenv("STARS_SDL_PRINT_ORIENTATION");
    wc.lpszClassName = "PrintOwner"; wc.lpfnWndProc = OwnerProc; wc.hInstance = GetModuleHandle(NULL); CHECK(RegisterClass(&wc));
    hwndOwner = CreateWindow(wc.lpszClassName, "Printer owner", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 260, 150, NULL, NULL, GetModuleHandle(NULL), NULL); CHECK(hwndOwner);
    snprintf(szChosen, sizeof(szChosen), "%s/chosen.pdf", pszDir); IUnlinkNativePath(szChosen);
    CHECK(InitPrinter(&pd, NULL)); hdcChoosing = pd.hDC; CHECK(SetTimer(hwndOwner, 1, 20, NULL));
    CHECK(Escape(pd.hDC, STARTDOC, 6, "Chosen", NULL) > 0); CHECK(Escape(pd.hDC, ENDDOC, 0, NULL, NULL) == 1);
    CHECK(FPrefix(szChosen, "%PDF-")); FreePrinter(&pd); KillTimer(hwndOwner, 1);
    fChooseCancel = TRUE;
    CHECK(InitPrinter(&pd, NULL)); hdcChoosing = pd.hDC;
    CHECK(SetTimer(hwndOwner, 2, 20, NULL));
    CHECK(Escape(pd.hDC, STARTDOC, 6, "Cancel", NULL) == SP_APPABORT);
    KillTimer(hwndOwner, 2); FreePrinter(&pd); fChooseCancel = FALSE;
    CHECK(cViewer == 4); DestroyWindow(hwndOwner);
#ifndef _WIN32
    // CUPS queue submission is a Unix path; Windows uses the PDF backend.
    lpfile = PfNativeOpen(szMock, "wb"); CHECK(lpfile);
    CchNativePrintf(lpfile, "%s", "#!/bin/sh\nprintf '%s\\n' \"$@\" > \"$STARS_TEST_PRINT_LOG\"\nfor szPdf do :; done\ncp -- \"$szPdf\" \"$STARS_TEST_PRINT_CAPTURE\"\nexit \"${STARS_TEST_PRINT_EXIT:-0}\"\n");
    ICloseNativeFile(lpfile); CHECK(chmod(szMock, 0700) == 0);
    snprintf(szPath, sizeof(szPath), "%s:%s", pszDir, pszOldPath ? pszOldPath : "/usr/bin:/bin");
    setenv("PATH", szPath, 1); setenv("STARS_TEST_PRINT_LOG", szLog, 1); setenv("STARS_TEST_PRINT_CAPTURE", szCapture, 1);
    setenv("STARS_SDL_PRINTER", "QA printer;false", 1);
    CHECK(InitPrinter(&pd, NULL)); CHECK(Escape(pd.hDC, STARTDOC, 5, "Spool", NULL) > 0);
    CHECK(Escape(pd.hDC, NEWFRAME, 0, NULL, NULL) == 1); CHECK(Escape(pd.hDC, ENDDOC, 0, NULL, NULL) == 1); FreePrinter(&pd);
    CHECK(FPrefix(szCapture, "%PDF-")); lpfile = PfNativeOpen(szLog, "rb"); CHECK(lpfile);
    CHECK(PszGetNativeLine(szArg, sizeof(szArg), lpfile) && strcmp(szArg, "-d\n") == 0);
    CHECK(PszGetNativeLine(szArg, sizeof(szArg), lpfile) && strcmp(szArg, "QA printer;false\n") == 0); ICloseNativeFile(lpfile);
    setenv("STARS_TEST_PRINT_EXIT", "1", 1);
    CHECK(InitPrinter(&pd, NULL)); CHECK(Escape(pd.hDC, STARTDOC, 4, "Fail", NULL) > 0);
    CHECK(Escape(pd.hDC, ENDDOC, 0, NULL, NULL) == SP_ERROR && GetLastError() == ERROR_WRITE_FAULT);
    lpfile = PfNativeOpen(szLog, "rb"); CHECK(lpfile); szRetained[0] = 0;
    while (PszGetNativeLine(szArg, sizeof(szArg), lpfile)) strcpy(szRetained, szArg);
    ICloseNativeFile(lpfile); szRetained[strcspn(szRetained, "\r\n")] = 0;
    CHECK(FPrefix(szRetained, "%PDF-")); FreePrinter(&pd); CHECK(FPrefix(szRetained, "%PDF-")); CHECK(IUnlinkNativePath(szRetained) == 0);
    unsetenv("STARS_SDL_PRINTER");
#endif
    CHECK(InitPrinter(&pd, "/stars-nonexistent-output-dir/out.pdf"));
    CHECK(Escape(pd.hDC, STARTDOC, 5, "Error", NULL) == SP_ERROR); FreePrinter(&pd);
    snprintf(szChosen, sizeof(szChosen), "%s/blocked-target", pszDir);
    CHECK(ICreateNativeDirectory(szChosen, 0700) == 0);
    CHECK(InitPrinter(&pd, szChosen));
    CHECK(Escape(pd.hDC, STARTDOC, 6, "Failed", NULL) > 0);
    CHECK(Escape(pd.hDC, ENDDOC, 0, NULL, NULL) == SP_ERROR && cViewer == 4);
    FreePrinter(&pd); CHECK(IRemoveNativePath(szChosen, 1) == 0);
    memset(&pd, 0, sizeof(pd)); CHECK(!PrintDlg(&pd) && CommDlgExtendedError() == CDERR_STRUCTSIZE);
    pd.lStructSize = sizeof(pd); pd.Flags = PD_RETURNDC; CHECK(!PrintDlg(&pd) && CommDlgExtendedError() == CDERR_INITIALIZATION);
    snprintf(szChosen, sizeof(szChosen), "%s/viewer-\xc3\xa9 #%%.pdf", pszDir);
    fViewerAccept = FALSE;
    CHECK(InitPrinter(&pd, szChosen));
    CHECK(Escape(pd.hDC, STARTDOC, 6, "Viewer", NULL) > 0);
    CHECK(Escape(pd.hDC, ENDDOC, 0, NULL, NULL) == 1 && FPrefix(szChosen, "%PDF-"));
    FreePrinter(&pd);
    CHECK(cNotice == 1 && strstr(szViewerUri, "%C3%A9%20%23%25.pdf") && cErrors == 0);
    StarsCompatShutdown(); CHECK(cErrors == 0); puts("Printer metadata, pages, abort, atomic output and supported-platform spooling checks passed"); return 0;
}
