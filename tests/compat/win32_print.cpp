#include "../../compat/win32/src/external_io.h"
#include <string>
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
static BOOL fViewerAccept = 1;
static const char *pszViewerExpected;
static char szViewerUri[16384];
static LRESULT  OwnerProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    HWND dlg;
    char sz[64];
    if (msg != win32::WM_TIMER) return win32::DefWindowProcA(hwnd, msg, w, l);
    for (dlg = win32::GetWindow(hwnd, win32::GW_HWNDFIRST); dlg; dlg = win32::GetWindow(dlg, win32::GW_HWNDNEXT)) {
        win32::GetWindowTextA(dlg, sz, sizeof(sz));
        if (strcmp(sz, "Print to PDF") == 0 && win32::GetDlgItem(dlg, 0x480)) {
            if (win32::DeleteDC(hdcChoosing) || win32::GetLastError() != win32::ERROR_BUSY) cErrors++;
            win32::SetWindowTextA(win32::GetDlgItem(dlg, 0x480), szChosen); win32::PostMessageA(dlg, win32::WM_COMMAND, fChooseCancel ? win32::IDCANCEL : win32::IDOK, 0); break;
        }
    }
    return 0;
}
static int InitPrinter(PRINTDLG *lp, const char *pszFile) {
    memset(lp, 0, sizeof(*lp)); lp->lStructSize = sizeof(*lp); lp->Flags = win32::PD_RETURNDEFAULT | win32::PD_RETURNDC;
    if (win32::IsWindow(hwndOwner)) lp->hwndOwner = hwndOwner;
    if (pszFile) SDL_setenv_unsafe("STARS_SDL_PRINT_FILE", pszFile, 1); else SDL_unsetenv_unsafe("STARS_SDL_PRINT_FILE");
    pszViewerExpected = pszFile ? pszFile : szChosen;
    return win32::PrintDlgA(lp);
}
static void FreePrinter(PRINTDLG *lp) { win32::DeleteDC(lp->hDC); win32::GlobalFree(lp->hDevMode); win32::GlobalFree(lp->hDevNames); }
static int FPrefix(const char *pszFile, const char *psz) {
    NATIVEFILE *lp = PfNativeOpen(pszFile, NATIVEFILEMODE::read);
    char sz[16] = {};
    size_t cb = strlen(psz), cbRead;
    if (!lp) return 0;
    if (cb > sizeof(sz)) { ICloseNativeFile(lp); return 0; }
    cbRead = CReadNative(std::span{sz}.first(cb), lp); ICloseNativeFile(lp); return cbRead == cb && memcmp(sz, psz, cb) == 0;
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
    int query = win32::NEWFRAME;
    std::string szPrinter;
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
    SDL_unsetenv_unsafe("STARS_SDL_PRINTER"); SDL_unsetenv_unsafe("STARS_SDL_PRINT_PAPER"); SDL_unsetenv_unsafe("STARS_SDL_PRINT_ORIENTATION");
    // DEVNAMES keeps the complete UTF-8 printer name, including at its byte limit.
    for (int i = 0; i < 85; i++) szPrinter += "\xe4\xb8\xad";
    SDL_setenv_unsafe("STARS_SDL_PRINTER", szPrinter.c_str(), 1);
    pd = {}; pd.lStructSize = sizeof(pd); pd.Flags = win32::PD_RETURNDEFAULT;
    CHECK(win32::PrintDlgA(&pd) && !pd.hDC && pd.hDevMode && pd.hDevNames);
    lpn = static_cast<DEVNAMES *>(win32::GlobalLock(pd.hDevNames)); CHECK(lpn && lpn->wDefault == 1);
    CHECK(lpn->wDriverOffset == sizeof(*lpn));
    CHECK(strcmp(reinterpret_cast<char *>(lpn) + lpn->wDriverOffset, "Stars SDL3/Cairo") == 0);
    CHECK(lpn->wDeviceOffset == lpn->wDriverOffset + sizeof("Stars SDL3/Cairo"));
    CHECK(strcmp(reinterpret_cast<char *>(lpn) + lpn->wDeviceOffset, szPrinter.c_str()) == 0);
    CHECK(lpn->wOutputOffset == lpn->wDeviceOffset + szPrinter.size() + 1);
    CHECK(strcmp(reinterpret_cast<char *>(lpn) + lpn->wOutputOffset, "CUPS:") == 0);
    CHECK(win32::GlobalSize(pd.hDevNames) >= lpn->wOutputOffset + sizeof("CUPS:"));
    win32::GlobalUnlock(pd.hDevNames); FreePrinter(&pd);
    szPrinter += 'x'; SDL_setenv_unsafe("STARS_SDL_PRINTER", szPrinter.c_str(), 1);
    pd = {}; pd.lStructSize = sizeof(pd); pd.Flags = win32::PD_RETURNDEFAULT;
    CHECK(!win32::PrintDlgA(&pd) && !pd.hDevMode && !pd.hDevNames && !pd.hDC);
    CHECK(win32::GetLastError() == win32::ERROR_INVALID_PARAMETER);
    SDL_unsetenv_unsafe("STARS_SDL_PRINTER");
    CHECK(InitPrinter(&pd, szPages) && pd.hDC && pd.hDevMode && pd.hDevNames && pd.nCopies == 1);
    CHECK(win32::CommDlgExtendedError() == 0);
    lpdm = (DEVMODE*)(win32::GlobalLock(pd.hDevMode)); CHECK(lpdm && lpdm->dmSize == sizeof(*lpdm) && lpdm->mode.printer.dmPaperSize == win32::DMPAPER_LETTER);
    CHECK(lpdm->mode.printer.dmOrientation == win32::DMORIENT_PORTRAIT && lpdm->mode.printer.dmPrintQuality == 300); win32::GlobalUnlock(pd.hDevMode);
    lpn = (DEVNAMES*)(win32::GlobalLock(pd.hDevNames)); CHECK(lpn && lpn->wDefault == 1);
    CHECK(strcmp((char *)lpn + lpn->wDeviceOffset, "Stars PDF") == 0); win32::GlobalUnlock(pd.hDevNames);
    CHECK(win32::GetDeviceCaps(pd.hDC, win32::HORZRES) == 2400 && win32::GetDeviceCaps(pd.hDC, win32::VERTRES) == 3150);
    CHECK(win32::GetDeviceCaps(pd.hDC, win32::LOGPIXELSX) == 300 && win32::GetDeviceCaps(pd.hDC, win32::TECHNOLOGY) == win32::DT_RASPRINTER);
    CHECK(win32::Escape(pd.hDC, win32::GETPRINTINGOFFSET, 0, NULL, &pt) == 1 && pt.x == 75 && pt.y == 75);
    CHECK(win32::Escape(pd.hDC, win32::GETPHYSPAGESIZE, 0, NULL, &pt) == 1 && pt.x == 2550 && pt.y == 3300);
    CHECK(win32::Escape(pd.hDC, win32::QUERYESCSUPPORT, sizeof(query), (char *)&query, NULL) == 1);
    CHECK(win32::Escape(pd.hDC, win32::ENDDOC, 0, NULL, NULL) == win32::SP_ERROR);
    lpfile = PfNativeOpen(szPages, NATIVEFILEMODE::writeTruncate); CHECK(lpfile); CchNativePrintf(lpfile, "%s", "sentinel"); ICloseNativeFile(lpfile);
    CHECK(win32::Escape(pd.hDC, win32::STARTDOC, 9, "Page test", NULL) > 0);
    CHECK(FPrefix(szPages, "sentinel"));
    hbr = win32::CreateSolidBrush(win32::RGB(255, 0, 0)); CHECK(hbr); rc = (RECT){100, 100, 500, 400}; CHECK(win32::FillRect(pd.hDC, &rc, hbr)); win32::DeleteObject(hbr);
    lf.lfHeight = -33; strcpy(lf.lfFaceName, "sans-serif");
    hfont = win32::CreateFontIndirectA(&lf); CHECK(hfont); hfontOld = win32::SelectObject(pd.hDC, hfont); CHECK(hfontOld);
    CHECK(win32::TextOutA(pd.hDC, 700, 100, "Stars map page one", 18));
    CHECK(win32::Escape(pd.hDC, win32::NEWFRAME, 0, NULL, NULL) == 1 && FPrefix(szPages, "sentinel") && cViewer == 0);
    CHECK(win32::GetPixel(pd.hDC, 120, 120) == win32::RGB(255, 255, 255));
    // The game repeats STARTDOC for every map page. Its rejected second call
    // must leave the original job intact, rather than replace its first page.
    CHECK(win32::Escape(pd.hDC, win32::STARTDOC, 9, "Page test", NULL) == win32::SP_ERROR);
    hbr = win32::CreateSolidBrush(win32::RGB(0, 0, 255)); CHECK(hbr); rc = (RECT){400, 600, 800, 900}; CHECK(win32::FillRect(pd.hDC, &rc, hbr)); win32::DeleteObject(hbr);
    CHECK(win32::Escape(pd.hDC, win32::NEWFRAME, 0, NULL, NULL) == 1);
    CHECK(win32::Escape(pd.hDC, win32::ENDDOC, 0, NULL, NULL) == 1 && FPrefix(szPages, "%PDF-") && cViewer == 1);
    CHECK(win32::SelectObject(pd.hDC, hfontOld)); CHECK(win32::DeleteObject(hfont));
    hdcOther = win32::CreateCompatibleDC(pd.hDC); CHECK(hdcOther);
    hbmp = win32::CreateCompatibleBitmap(pd.hDC, 10, 10); CHECK(hbmp);
    CHECK(!win32::SelectObject(pd.hDC, hbmp)); CHECK(win32::DeleteObject(hbmp)); CHECK(win32::DeleteDC(hdcOther));
    CHECK(win32::Escape(pd.hDC, 99999, 0, NULL, NULL) == 0); FreePrinter(&pd);
    CHECK(InitPrinter(&pd, szAbort));
    lpfile = PfNativeOpen(szAbort, NATIVEFILEMODE::writeTruncate); CHECK(lpfile); CchNativePrintf(lpfile, "%s", "sentinel"); ICloseNativeFile(lpfile);
    CHECK(win32::Escape(pd.hDC, win32::STARTDOC, 5, "Abort", NULL) > 0); CHECK(win32::Escape(pd.hDC, win32::NEWFRAME, 0, NULL, NULL) == 1);
    CHECK(win32::Escape(pd.hDC, win32::ABORTDOC, 0, NULL, NULL) == 1 && FPrefix(szAbort, "sentinel"));
    CHECK(win32::Escape(pd.hDC, win32::STARTDOC, 5, "Abort", NULL) > 0); FreePrinter(&pd); CHECK(FPrefix(szAbort, "sentinel"));
    CHECK(InitPrinter(&pd, szImplicit)); CHECK(win32::Escape(pd.hDC, win32::STARTDOC, 8, "Implicit", NULL) > 0);
    CHECK(win32::SetPixel(pd.hDC, 10, 10, win32::RGB(0, 200, 0)) == win32::RGB(0, 200, 0));
    CHECK(win32::Escape(pd.hDC, win32::ENDDOC, 0, NULL, NULL) == 1); FreePrinter(&pd);
    SDL_setenv_unsafe("STARS_SDL_PRINT_PAPER", "a4", 1); SDL_setenv_unsafe("STARS_SDL_PRINT_ORIENTATION", "landscape", 1);
    CHECK(InitPrinter(&pd, szPaper)); CHECK(win32::GetDeviceCaps(pd.hDC, win32::HORZRES) == 3358 && win32::GetDeviceCaps(pd.hDC, win32::VERTRES) == 2330);
    CHECK(win32::Escape(pd.hDC, win32::STARTDOC, 8, "A4 paper", NULL) > 0); CHECK(win32::Escape(pd.hDC, win32::NEWFRAME, 0, NULL, NULL) == 1);
    CHECK(win32::Escape(pd.hDC, win32::ENDDOC, 0, NULL, NULL) == 1); FreePrinter(&pd);
    SDL_unsetenv_unsafe("STARS_SDL_PRINT_PAPER"); SDL_unsetenv_unsafe("STARS_SDL_PRINT_ORIENTATION");
    wc.lpszClassName = "PrintOwner"; wc.lpfnWndProc = OwnerProc; wc.hInstance = win32::GetModuleHandleA(NULL); CHECK(win32::RegisterClassA(&wc));
    hwndOwner = win32::CreateWindowExA(0, wc.lpszClassName, "Printer owner", win32::WS_OVERLAPPEDWINDOW | win32::WS_VISIBLE, 0, 0, 260, 150, NULL, NULL, win32::GetModuleHandleA(NULL), NULL); CHECK(hwndOwner);
    snprintf(szChosen, sizeof(szChosen), "%s/chosen.pdf", pszDir); IUnlinkNativePath(szChosen);
    CHECK(InitPrinter(&pd, NULL)); hdcChoosing = pd.hDC; CHECK(win32::SetTimer(hwndOwner, 1, 20, NULL));
    CHECK(win32::Escape(pd.hDC, win32::STARTDOC, 6, "Chosen", NULL) > 0); CHECK(win32::Escape(pd.hDC, win32::ENDDOC, 0, NULL, NULL) == 1);
    CHECK(FPrefix(szChosen, "%PDF-")); FreePrinter(&pd); win32::KillTimer(hwndOwner, 1);
    fChooseCancel = 1;
    CHECK(InitPrinter(&pd, NULL)); hdcChoosing = pd.hDC;
    CHECK(win32::SetTimer(hwndOwner, 2, 20, NULL));
    CHECK(win32::Escape(pd.hDC, win32::STARTDOC, 6, "Cancel", NULL) == win32::SP_APPABORT);
    win32::KillTimer(hwndOwner, 2); FreePrinter(&pd); fChooseCancel = 0;
    CHECK(cViewer == 4); win32::DestroyWindow(hwndOwner);
#ifndef _WIN32
    // CUPS queue submission is a Unix path; Windows uses the PDF backend.
    lpfile = PfNativeOpen(szMock, NATIVEFILEMODE::writeTruncate); CHECK(lpfile);
    CchNativePrintf(lpfile, "%s", "#!/bin/sh\nprintf '%s\\n' \"$@\" > \"$STARS_TEST_PRINT_LOG\"\nfor szPdf do :; done\ncp -- \"$szPdf\" \"$STARS_TEST_PRINT_CAPTURE\"\nexit \"${STARS_TEST_PRINT_EXIT:-0}\"\n");
    ICloseNativeFile(lpfile); CHECK(chmod(szMock, 0700) == 0);
    snprintf(szPath, sizeof(szPath), "%s:%s", pszDir, pszOldPath ? pszOldPath : "/usr/bin:/bin");
    SDL_setenv_unsafe("PATH", szPath, 1); SDL_setenv_unsafe("STARS_TEST_PRINT_LOG", szLog, 1); SDL_setenv_unsafe("STARS_TEST_PRINT_CAPTURE", szCapture, 1);
    SDL_setenv_unsafe("STARS_SDL_PRINTER", "QA printer;false", 1);
    CHECK(InitPrinter(&pd, NULL)); CHECK(win32::Escape(pd.hDC, win32::STARTDOC, 5, "Spool", NULL) > 0);
    CHECK(win32::Escape(pd.hDC, win32::NEWFRAME, 0, NULL, NULL) == 1); CHECK(win32::Escape(pd.hDC, win32::ENDDOC, 0, NULL, NULL) == 1); FreePrinter(&pd);
    CHECK(FPrefix(szCapture, "%PDF-")); lpfile = PfNativeOpen(szLog, NATIVEFILEMODE::read); CHECK(lpfile);
    CHECK(CchGetNativeLine(std::span{szArg}, lpfile) && strcmp(szArg, "-d\n") == 0);
    CHECK(CchGetNativeLine(std::span{szArg}, lpfile) && strcmp(szArg, "QA printer;false\n") == 0); ICloseNativeFile(lpfile);
    SDL_setenv_unsafe("STARS_TEST_PRINT_EXIT", "1", 1);
    CHECK(InitPrinter(&pd, NULL)); CHECK(win32::Escape(pd.hDC, win32::STARTDOC, 4, "Fail", NULL) > 0);
    CHECK(win32::Escape(pd.hDC, win32::ENDDOC, 0, NULL, NULL) == win32::SP_ERROR && win32::GetLastError() == win32::ERROR_WRITE_FAULT);
    lpfile = PfNativeOpen(szLog, NATIVEFILEMODE::read); CHECK(lpfile); szRetained[0] = 0;
    while (CchGetNativeLine(std::span{szArg}, lpfile)) strcpy(szRetained, szArg);
    ICloseNativeFile(lpfile); szRetained[strcspn(szRetained, "\r\n")] = 0;
    CHECK(FPrefix(szRetained, "%PDF-")); FreePrinter(&pd); CHECK(FPrefix(szRetained, "%PDF-")); CHECK(IUnlinkNativePath(szRetained) == 0);
    SDL_unsetenv_unsafe("STARS_SDL_PRINTER");
#endif
    CHECK(InitPrinter(&pd, "/stars-nonexistent-output-dir/out.pdf"));
    CHECK(win32::Escape(pd.hDC, win32::STARTDOC, 5, "Error", NULL) == win32::SP_ERROR); FreePrinter(&pd);
    snprintf(szChosen, sizeof(szChosen), "%s/blocked-target", pszDir);
    CHECK(ICreateNativeDirectory(szChosen, 0700) == 0);
    CHECK(InitPrinter(&pd, szChosen));
    CHECK(win32::Escape(pd.hDC, win32::STARTDOC, 6, "Failed", NULL) > 0);
    CHECK(win32::Escape(pd.hDC, win32::ENDDOC, 0, NULL, NULL) == win32::SP_ERROR && cViewer == 4);
    FreePrinter(&pd); CHECK(IRemoveNativePath(szChosen, 1) == 0);
    memset(&pd, 0, sizeof(pd)); CHECK(!win32::PrintDlgA(&pd) && win32::CommDlgExtendedError() == win32::CDERR_STRUCTSIZE);
    pd.lStructSize = sizeof(pd); pd.Flags = win32::PD_RETURNDC; CHECK(!win32::PrintDlgA(&pd) && win32::CommDlgExtendedError() == win32::CDERR_INITIALIZATION);
    snprintf(szChosen, sizeof(szChosen), "%s/viewer-\xc3\xa9 #%%.pdf", pszDir);
    fViewerAccept = 0;
    CHECK(InitPrinter(&pd, szChosen));
    CHECK(win32::Escape(pd.hDC, win32::STARTDOC, 6, "Viewer", NULL) > 0);
    CHECK(win32::Escape(pd.hDC, win32::ENDDOC, 0, NULL, NULL) == 1 && FPrefix(szChosen, "%PDF-"));
    FreePrinter(&pd);
    CHECK(cNotice == 1 && strstr(szViewerUri, "%C3%A9%20%23%25.pdf") && cErrors == 0);
    StarsCompatShutdown(); CHECK(cErrors == 0); puts("Printer metadata, pages, abort, atomic output and supported-platform spooling checks passed"); return 0;
}
