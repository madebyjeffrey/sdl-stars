/* Legacy printer escapes spool the same GDI raster canvas to atomic PDF output. */
#include "internal.h"
#include "external_io.h"
#include "time_io.h"
#include "ownership.h"
#include <cairo-pdf.h>
#include <cerrno>
#include <climits>
#include <new>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

struct COMPATPRINT {
    ~COMPATPRINT() noexcept;
    HWND hwndOwner;
    int cx, cy, cxPhysical, cyPhysical, cPages, cCopies, idPaper;
    BOOL fSpool, fActive, fKeep, fEscaping;
    char szPrinter[256], szTarget[PATH_MAX], szTemp[PATH_MAX], szTitle[4097];
    NATIVEOWNER<NATIVEFILE, ICloseNativeFile> lpfile;
    NATIVEOWNER<cairo_surface_t, cairo_surface_destroy> lpPDF;
};
static int idPrintNext = 1;

static void AbortPrint(COMPATPRINT *lp) {
    lp->lpPDF.reset(); lp->lpfile.reset();
    if (*lp->szTemp && !lp->fKeep) IUnlinkNativePath(lp->szTemp);
    lp->szTemp[0] = 0; lp->fActive = 0;
}
COMPATPRINT::~COMPATPRINT() noexcept { AbortPrint(this); }
void CompatDeletePrinter(COMPATPRINT *lp) noexcept { delete lp; }
BOOL FCompatPrinterBusy(COMPATPRINT *lp) { return lp->fEscaping; }
int ICompatPrinterCaps(COMPATPRINT *lp, int index) {
    switch (index) {
    case win32::TECHNOLOGY: return win32::DT_RASPRINTER;
    case win32::LOGPIXELSX: case win32::LOGPIXELSY: return 300;
    case win32::HORZRES: return lp->cx;
    case win32::VERTRES: return lp->cy;
    case win32::PHYSICALWIDTH: return lp->cxPhysical;
    case win32::PHYSICALHEIGHT: return lp->cyPhysical;
    case win32::PHYSICALOFFSETX: case win32::PHYSICALOFFSETY: return 75;
    case win32::HORZSIZE: return (lp->cx * 254 + 1500) / 3000;
    case win32::VERTSIZE: return (lp->cy * 254 + 1500) / 3000;
    case win32::BITSPIXEL: return 32;
    case win32::PLANES: return 1;
    default: return 0;
    }
}
static HGLOBAL HglobPrintNames(COMPATPRINT *lp) {
    const char *pszDriver = "Stars SDL3/Cairo", *pszDevice = lp->fSpool ? lp->szPrinter : "Stars PDF", *pszPort = lp->fSpool ? "CUPS:" : "FILE:";
    std::size_t cb = sizeof(DEVNAMES) + std::strlen(pszDriver) + std::strlen(pszDevice) + std::strlen(pszPort) + 3;
    HGLOBAL h = win32::GlobalAlloc(win32::GMEM_MOVEABLE | win32::GMEM_ZEROINIT, cb);
    DEVNAMES *lpn;
    char *psz;
    if (!h) return nullptr;
    lpn = static_cast<DEVNAMES *>((win32::GlobalLock(h))); if (!lpn) { win32::GlobalFree(h); return nullptr; }
    psz = reinterpret_cast<char *>(lpn) + sizeof(*lpn);
    const auto rgchNames = std::span{reinterpret_cast<char *>(lpn), cb};
    lpn->wDriverOffset = static_cast<WORD>((psz - reinterpret_cast<char *>(lpn))); CchCopyUtf8(rgchNames.subspan(lpn->wDriverOffset), pszDriver); psz += std::strlen(psz) + 1;
    lpn->wDeviceOffset = static_cast<WORD>((psz - reinterpret_cast<char *>(lpn))); CchCopyUtf8(rgchNames.subspan(lpn->wDeviceOffset), pszDevice); psz += std::strlen(psz) + 1;
    lpn->wOutputOffset = static_cast<WORD>((psz - reinterpret_cast<char *>(lpn))); CchCopyUtf8(rgchNames.subspan(lpn->wOutputOffset), pszPort); lpn->wDefault = 1;
    win32::GlobalUnlock(h); return h;
}
BOOL win32::PrintDlgA(LPPRINTDLGA lppd) {
    COMPATPRINT *lp = nullptr;
    HGLOBAL hMode = nullptr, hNames = nullptr;
    HDC hdc = nullptr;
    DEVMODEA *lpdm;
    LPCSTR pszPrinter = SDL_getenv("STARS_SDL_PRINTER"), pszPaper = SDL_getenv("STARS_SDL_PRINT_PAPER");
    LPCSTR pszOrientation = SDL_getenv("STARS_SDL_PRINT_ORIENTATION"), pszFile = SDL_getenv("STARS_SDL_PRINT_FILE");
    const char *pszDeviceName;
    DWORD dwError = win32::CDERR_INITIALIZATION;
    int temp;
    NATIVEOWNER<COMPATPRINT, CompatDeletePrinter> lpprintOwner;
    CompatSetDialogError(0);
    if (!lppd || lppd->lStructSize != sizeof(*lppd)) { CompatSetDialogError(win32::CDERR_STRUCTSIZE); return 0; }
    if (lppd->hDevMode || lppd->hDevNames) { CompatSetDialogError(win32::PDERR_RETDEFFAILURE); return 0; }
    if (!(lppd->Flags & win32::PD_RETURNDEFAULT) || (lppd->Flags & ~(win32::PD_RETURNDEFAULT | win32::PD_RETURNDC)) ||
        !SDL_IsMainThread() || (lppd->hwndOwner && !win32::IsWindow(lppd->hwndOwner))) { win32::SetLastError(win32::ERROR_NOT_SUPPORTED); goto fail; }
    lpprintOwner.reset(new (std::nothrow) COMPATPRINT{}); lp = lpprintOwner.get(); if (!lp) { dwError = win32::CDERR_MEMALLOCFAILURE; goto fail; }
    lp->hwndOwner = lppd->hwndOwner ? HwndCompatRoot(lppd->hwndOwner) : win32::GetActiveWindow();
    lp->cCopies = 1; lp->idPaper = win32::DMPAPER_LETTER; lp->cxPhysical = 2550; lp->cyPhysical = 3300;
    if (pszPaper && *pszPaper && SDL_strcasecmp(pszPaper, "letter") != 0) {
        if (SDL_strcasecmp(pszPaper, "a4") != 0) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); goto fail; }
        lp->idPaper = win32::DMPAPER_A4; lp->cxPhysical = 2480; lp->cyPhysical = 3508;
    }
    if (pszOrientation && *pszOrientation && SDL_strcasecmp(pszOrientation, "portrait") != 0) {
        if (SDL_strcasecmp(pszOrientation, "landscape") != 0) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); goto fail; }
        temp = lp->cxPhysical; lp->cxPhysical = lp->cyPhysical; lp->cyPhysical = temp;
    }
    lp->cx = lp->cxPhysical - 150; lp->cy = lp->cyPhysical - 150;
    if (pszPrinter && *pszPrinter && SDL_strcasecmp(pszPrinter, "pdf") != 0) {
        if (!FUtf8Valid(pszPrinter)) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); goto fail; }
        if (SDL_strlcpy(lp->szPrinter, pszPrinter, sizeof(lp->szPrinter)) >= sizeof(lp->szPrinter)) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); goto fail; }
        lp->fSpool = 1;
    }
    if (pszFile && *pszFile && (!FUtf8Valid(pszFile) ||
        SDL_strlcpy(lp->szTarget, pszFile, sizeof(lp->szTarget)) >= sizeof(lp->szTarget))) {
        win32::SetLastError(win32::ERROR_INVALID_PARAMETER); goto fail;
    }
    hMode = win32::GlobalAlloc(win32::GMEM_MOVEABLE | win32::GMEM_ZEROINIT, sizeof(*lpdm));
    hNames = HglobPrintNames(lp);
    if (!hMode || !hNames) { dwError = win32::CDERR_MEMALLOCFAILURE; goto fail; }
    lpdm = static_cast<DEVMODEA *>((win32::GlobalLock(hMode))); if (!lpdm) goto fail;
    pszDeviceName = lp->fSpool ? lp->szPrinter : "Stars PDF";
    CchCopyUtf8(std::span{lpdm->dmDeviceName},
                std::string_view{pszDeviceName, CchUtf8Prefix(pszDeviceName, sizeof(lpdm->dmDeviceName) - 1)});
    lpdm->dmSize = sizeof(*lpdm); lpdm->dmSpecVersion = 0x401; lpdm->dmDriverVersion = 1;
    lpdm->dmFields = win32::DM_ORIENTATION | win32::DM_PAPERSIZE | win32::DM_COPIES | win32::DM_PRINTQUALITY | win32::DM_YRESOLUTION;
    lpdm->mode.printer.dmOrientation = lp->cxPhysical > lp->cyPhysical ? win32::DMORIENT_LANDSCAPE : win32::DMORIENT_PORTRAIT;
    lpdm->mode.printer.dmPaperSize = lp->idPaper;
    lpdm->mode.printer.dmCopies = 1; lpdm->mode.printer.dmPrintQuality = lpdm->dmYResolution = 300;
    win32::GlobalUnlock(hMode);
    if (lppd->Flags & win32::PD_RETURNDC) {
        hdc = HdcCompatPrinter(lp, lp->cx, lp->cy);
        if (!hdc) { dwError = win32::CDERR_MEMALLOCFAILURE; goto fail; }
        lpprintOwner.release(); lp = nullptr;
    }
    lppd->hDevMode = hMode; lppd->hDevNames = hNames; lppd->nCopies = 1;
    if (lppd->Flags & win32::PD_RETURNDC) lppd->hDC = hdc;
    return 1;
fail:
    if (hdc) win32::DeleteDC(hdc);
    win32::GlobalFree(hMode); win32::GlobalFree(hNames); CompatSetDialogError(dwError); return 0;
}
static cairo_status_t WritePDF(void *lpv, const unsigned char *rgb, unsigned cb) {
    COMPATPRINT *lp = static_cast<COMPATPRINT *>((lpv));
    return CWriteNative(std::span{rgb, cb}, lp->lpfile.get()) == cb ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
}
static int StartPrint(COMPATPRINT *lp, int cb, LPCSTR pszTitle) {
    OPENFILENAMEA ofn = {};
    char szFile[PATH_MAX] = "Stars map.pdf";
    NATIVEFILETEMPRESULT nft;
    if (lp->fActive || cb < 0 || cb > 4096 || (cb != 0 && !pszTitle)) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return win32::SP_ERROR; }
    if (cb != 0 && !FUtf8Valid(std::string_view{pszTitle, static_cast<std::size_t>(cb)})) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return win32::SP_ERROR; }
    if (cb) std::memcpy(lp->szTitle, pszTitle, cb);
    lp->szTitle[cb] = 0;
    if (!lp->fSpool && !*lp->szTarget) {
        ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = win32::IsWindow(lp->hwndOwner) ? lp->hwndOwner : nullptr;
        ofn.lpstrFile = szFile; ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "PDF documents\0*.pdf\0"; ofn.nFilterIndex = 1; ofn.lpstrDefExt = "pdf";
        ofn.lpstrTitle = "Print to PDF"; ofn.Flags = win32::OFN_HIDEREADONLY | win32::OFN_PATHMUSTEXIST | win32::OFN_NOREADONLYRETURN | win32::OFN_OVERWRITEPROMPT | win32::OFN_NOCHANGEDIR;
        if (!win32::GetSaveFileNameA(&ofn)) return win32::CommDlgExtendedError() ? win32::SP_ERROR : win32::SP_APPABORT;
        CchCopyUtf8(std::span{lp->szTarget}, szFile);
    }
    lp->fKeep = 0;
    if (lp->fSpool) CchCopyUtf8(std::span{lp->szTemp}, "/tmp/stars-print-XXXXXX");
    else if (std::snprintf(lp->szTemp, sizeof(lp->szTemp), "%s.tmp-XXXXXX", lp->szTarget) >= static_cast<int>(sizeof(lp->szTemp))) {
        lp->szTemp[0] = 0; win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return win32::SP_ERROR;
    }
    nft = NftNativeCreateTemp(lp->szTemp);
    lp->lpfile.reset(nft.lpfile.release());
    if (!lp->lpfile) { lp->szTemp[0] = 0; win32::SetLastError(win32::ERROR_ACCESS_DENIED); return win32::SP_ERROR; }
    CchCopyUtf8(std::span{lp->szTemp}, nft.szPath);
    lp->lpPDF.reset(cairo_pdf_surface_create_for_stream(WritePDF, lp, lp->cxPhysical * 72.0 / 300, lp->cyPhysical * 72.0 / 300));
    if (cairo_surface_status(lp->lpPDF.get()) != CAIRO_STATUS_SUCCESS) { AbortPrint(lp); win32::SetLastError(win32::ERROR_NOT_ENOUGH_MEMORY); return win32::SP_ERROR; }
    cairo_pdf_surface_set_metadata(lp->lpPDF.get(), CAIRO_PDF_METADATA_TITLE, lp->szTitle);
    cairo_pdf_surface_set_metadata(lp->lpPDF.get(), CAIRO_PDF_METADATA_CREATOR, "Stars SDL3 compatibility printer");
    lp->cPages = 0; lp->fActive = 1;
    if (idPrintNext == INT_MAX) idPrintNext = 1;
    return idPrintNext++;
}
static BOOL FPrintPage(COMPATPRINT *lp, SDL_Surface *lpSource) {
    cairo_surface_t *lpImage;
    cairo_t *lpcr;
    BOOL fOk;
    // Cairo may retain an image while finalizing later PDF pages. Its image
    // owns the pixels rather than borrowing a temporary SDL conversion buffer.
    {
        NATIVEOWNER<cairo_surface_t, cairo_surface_destroy> lpimageOwner(
            cairo_image_surface_create(CAIRO_FORMAT_ARGB32, lpSource->w, lpSource->h));
        lpImage = lpimageOwner.get();
        if (cairo_surface_status(lpImage) != CAIRO_STATUS_SUCCESS ||
            !SDL_ConvertPixels(lpSource->w, lpSource->h, lpSource->format, lpSource->pixels, lpSource->pitch,
                               SDL_PIXELFORMAT_ARGB8888, cairo_image_surface_get_data(lpImage), cairo_image_surface_get_stride(lpImage))) {
            return 0;
        }
        cairo_surface_mark_dirty(lpImage);
        NATIVEOWNER<cairo_t, cairo_destroy> lpcontextOwner(cairo_create(lp->lpPDF.get()));
        lpcr = lpcontextOwner.get();
        cairo_translate(lpcr, 18, 18); cairo_scale(lpcr, 72.0 / 300, 72.0 / 300);
        cairo_set_source_surface(lpcr, lpImage, 0, 0); cairo_paint(lpcr); cairo_show_page(lpcr);
        fOk = cairo_status(lpcr) == CAIRO_STATUS_SUCCESS && cairo_surface_status(lp->lpPDF.get()) == CAIRO_STATUS_SUCCESS;
    }
    if (fOk) {
        lp->cPages++;
        SDL_FillSurfaceRect(lpSource, nullptr, SDL_MapSurfaceRGB(lpSource, 255, 255, 255));
    }
    return fOk;
}
static BOOL FPrintMarked(SDL_Surface *lpSurface) {
    const BYTE *rgb;
    int x, y;
    for (y = 0; y < lpSurface->h; y++) {
        rgb = static_cast<BYTE *>(lpSurface->pixels) + static_cast<std::size_t>(y) * lpSurface->pitch;
        for (x = 0; x < lpSurface->w; x++, rgb += 4) if (rgb[0] != 255 || rgb[1] != 255 || rgb[2] != 255) return 1;
    }
    return 0;
}
static BOOL FSubmitPrint(COMPATPRINT *lp) {
    const char *rgArgs[16];
    char szCopies[16];
    SDL_Process *lpProcess;
    Uint64 ticks;
    int cArgs = 0, code = -1;
    std::snprintf(szCopies, sizeof(szCopies), "%d", lp->cCopies);
    rgArgs[cArgs++] = "lp";
    if (std::strcmp(lp->szPrinter, "default") != 0) { rgArgs[cArgs++] = "-d"; rgArgs[cArgs++] = lp->szPrinter; }
    rgArgs[cArgs++] = "-n"; rgArgs[cArgs++] = szCopies;
    rgArgs[cArgs++] = "-t"; rgArgs[cArgs++] = lp->szTitle;
    rgArgs[cArgs++] = "-o"; rgArgs[cArgs++] = lp->idPaper == win32::DMPAPER_A4 ? "media=A4" : "media=Letter";
    rgArgs[cArgs++] = "-o"; rgArgs[cArgs++] = lp->cxPhysical > lp->cyPhysical ? "orientation-requested=4" : "orientation-requested=3";
    rgArgs[cArgs++] = "--"; rgArgs[cArgs++] = lp->szTemp; rgArgs[cArgs] = nullptr;
    NATIVEOWNER<SDL_Process, SDL_DestroyProcess> lpprocessOwner(SDL_CreateProcess(rgArgs, false));
    lpProcess = lpprocessOwner.get();
    if (!lpProcess) return 0;
    ticks = CompatTimeIO().pfnGetTicks();
    while (!SDL_WaitProcess(lpProcess, false, &code)) {
        if (CompatTimeIO().pfnGetTicks() - ticks >= 30000) { SDL_KillProcess(lpProcess, true); SDL_WaitProcess(lpProcess, true, &code); code = -1; break; }
        // Escape is synchronous. Pump SDL/portal events without reentering a
        // caller that could delete this printer DC while its job is submitting.
        SDL_PumpEvents(); SDL_Delay(5);
    }
    return code == 0;
}
static int IEscapePrinter(COMPATPRINT *lp, HDC hdc, int idEscape, int cb, LPCSTR pszIn, LPVOID lpOut) {
    SDL_Surface *lpSurface;
    int id;
    BOOL fOk;
    if (idEscape == win32::QUERYESCSUPPORT) {
        if (!pszIn || cb < static_cast<int>(sizeof(int))) return 0;
        std::memcpy(&id, pszIn, sizeof(id));
        return id == win32::STARTDOC || id == win32::NEWFRAME || id == win32::ENDDOC || id == win32::ABORTDOC || id == win32::QUERYESCSUPPORT || id == win32::GETPHYSPAGESIZE || id == win32::GETPRINTINGOFFSET;
    }
    if (idEscape == win32::GETPHYSPAGESIZE || idEscape == win32::GETPRINTINGOFFSET) {
        if (!lpOut) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return win32::SP_ERROR; }
        *static_cast<POINT *>(lpOut) = idEscape == win32::GETPHYSPAGESIZE ? POINT{lp->cxPhysical, lp->cyPhysical} : POINT{75, 75}; return 1;
    }
    lpSurface = LpCompatPrinterSurface(hdc);
    if (idEscape == win32::STARTDOC) {
        id = StartPrint(lp, cb, pszIn);
        if (id > 0) SDL_FillSurfaceRect(lpSurface, nullptr, SDL_MapSurfaceRGB(lpSurface, 255, 255, 255));
        return id;
    }
    if (idEscape != win32::NEWFRAME && idEscape != win32::ENDDOC && idEscape != win32::ABORTDOC) { win32::SetLastError(win32::ERROR_NOT_SUPPORTED); return 0; }
    if (!lp->fActive) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return win32::SP_ERROR; }
    if (idEscape == win32::ABORTDOC) { AbortPrint(lp); return 1; }
    if (idEscape == win32::NEWFRAME) {
        if (FPrintPage(lp, lpSurface)) return 1;
        AbortPrint(lp); win32::SetLastError(win32::ERROR_WRITE_FAULT); return win32::SP_ERROR;
    }
    if ((lp->cPages == 0 || FPrintMarked(lpSurface)) && !FPrintPage(lp, lpSurface)) {
        AbortPrint(lp); win32::SetLastError(win32::ERROR_WRITE_FAULT); return win32::SP_ERROR;
    }
    cairo_surface_finish(lp->lpPDF.get()); fOk = cairo_surface_status(lp->lpPDF.get()) == CAIRO_STATUS_SUCCESS;
    lp->lpPDF.reset();
    if (IFlushNativeFile(lp->lpfile.get()) != 0 || ISyncNativeFile(lp->lpfile.get()) != 0) fOk = 0;
    if (ICloseNativeFile(lp->lpfile.release()) != 0) fOk = 0;
    lp->fActive = 0;
    if (!fOk) { AbortPrint(lp); win32::SetLastError(win32::ERROR_WRITE_FAULT); return win32::SP_ERROR; }
    if (lp->fSpool) {
        if (!FSubmitPrint(lp)) {
            lp->fKeep = 1; win32::SetLastError(win32::ERROR_WRITE_FAULT);
            SDL_Log("Printer submission failed; PDF retained at %s", lp->szTemp);
            return win32::SP_ERROR;
        }
        IUnlinkNativePath(lp->szTemp); lp->szTemp[0] = 0;
    } else {
        if (IRenameNativePath(lp->szTemp, lp->szTarget, 1, 1) != 0) { AbortPrint(lp); win32::SetLastError(win32::ERROR_WRITE_FAULT); return win32::SP_ERROR; }
        lp->szTemp[0] = 0;
        // The completed PDF remains valid even when no viewer accepts it.
        if (!FCompatOpenDocument(lp->szTarget)) {
            char szMessage[PATH_MAX + 160];
            HWND hwndOwner = win32::IsWindow(lp->hwndOwner) ? lp->hwndOwner : nullptr;
            const auto szAbsolute = SzCanonicalNativePath(lp->szTarget);
            std::snprintf(szMessage, sizeof(szMessage), "The PDF was saved, but its viewer could not be opened.\n\nOpen this file in a PDF viewer to print it:\n%s", szAbsolute ? szAbsolute->c_str() : lp->szTarget);
            SDL_Log("%s", szMessage);
            CompatExternalIO().pfnShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "PDF saved", szMessage,
                                     hwndOwner ? hwndOwner->lpWindow.get() : nullptr);
        }
    }
    return 1;
}
int win32::Escape(HDC hdc, int idEscape, int cb, LPCSTR pszIn, LPVOID lpOut) {
    COMPATPRINT *lp = LpCompatPrinter(hdc);
    if (!lp) { win32::SetLastError(win32::ERROR_INVALID_HANDLE); return win32::SP_ERROR; }
    if (lp->fEscaping) { win32::SetLastError(win32::ERROR_BUSY); return win32::SP_ERROR; }
    // The PDF destination picker dispatches caller timers. Keep this DC alive
    // until its synchronous escape returns, even if a timer tries to delete it.
    StarsRestore<BOOL> escaping(lp->fEscaping);
    lp->fEscaping = 1;
    return IEscapePrinter(lp, hdc, idEscape, cb, pszIn, lpOut);
}
