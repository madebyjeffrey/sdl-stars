/* Legacy printer escapes spool the same GDI raster canvas to atomic PDF output. */
#include "internal.h"
#include "ownership.h"
#include <cairo-pdf.h>
#include <errno.h>
#include <limits.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    lp->szTemp[0] = 0; lp->fActive = FALSE;
}
COMPATPRINT::~COMPATPRINT() noexcept { AbortPrint(this); }
void CompatDeletePrinter(COMPATPRINT *lp) noexcept { delete lp; }
BOOL FCompatPrinterBusy(COMPATPRINT *lp) { return lp->fEscaping; }
int ICompatPrinterCaps(COMPATPRINT *lp, int index) {
    switch (index) {
    case TECHNOLOGY: return DT_RASPRINTER;
    case LOGPIXELSX: case LOGPIXELSY: return 300;
    case HORZRES: return lp->cx;
    case VERTRES: return lp->cy;
    case PHYSICALWIDTH: return lp->cxPhysical;
    case PHYSICALHEIGHT: return lp->cyPhysical;
    case PHYSICALOFFSETX: case PHYSICALOFFSETY: return 75;
    case HORZSIZE: return (lp->cx * 254 + 1500) / 3000;
    case VERTSIZE: return (lp->cy * 254 + 1500) / 3000;
    case BITSPIXEL: return 32;
    case PLANES: return 1;
    default: return 0;
    }
}
static HGLOBAL HglobPrintNames(COMPATPRINT *lp) {
    const char *pszDriver = "Stars SDL3/Cairo", *pszDevice = lp->fSpool ? lp->szPrinter : "Stars PDF", *pszPort = lp->fSpool ? "CUPS:" : "FILE:";
    size_t cb = sizeof(DEVNAMES) + strlen(pszDriver) + strlen(pszDevice) + strlen(pszPort) + 3;
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, cb);
    DEVNAMES *lpn;
    char *psz;
    if (!h) return NULL;
    lpn = (DEVNAMES *)(GlobalLock(h)); if (!lpn) { GlobalFree(h); return NULL; }
    psz = (char *)lpn + sizeof(*lpn);
    lpn->wDriverOffset = (WORD)(psz - (char *)lpn); strcpy(psz, pszDriver); psz += strlen(psz) + 1;
    lpn->wDeviceOffset = (WORD)(psz - (char *)lpn); strcpy(psz, pszDevice); psz += strlen(psz) + 1;
    lpn->wOutputOffset = (WORD)(psz - (char *)lpn); strcpy(psz, pszPort); lpn->wDefault = 1;
    GlobalUnlock(h); return h;
}
BOOL WINAPI PrintDlgA(LPPRINTDLGA lppd) {
    COMPATPRINT *lp = NULL;
    HGLOBAL hMode = NULL, hNames = NULL;
    HDC hdc = NULL;
    DEVMODEA *lpdm;
    LPCSTR pszPrinter = SDL_getenv("STARS_SDL_PRINTER"), pszPaper = SDL_getenv("STARS_SDL_PRINT_PAPER");
    LPCSTR pszOrientation = SDL_getenv("STARS_SDL_PRINT_ORIENTATION"), pszFile = SDL_getenv("STARS_SDL_PRINT_FILE");
    DWORD dwError = CDERR_INITIALIZATION;
    int temp;
    NATIVEOWNER<COMPATPRINT, CompatDeletePrinter> lpprintOwner;
    CompatSetDialogError(0);
    if (!lppd || lppd->lStructSize != sizeof(*lppd)) { CompatSetDialogError(CDERR_STRUCTSIZE); return FALSE; }
    if (lppd->hDevMode || lppd->hDevNames) { CompatSetDialogError(PDERR_RETDEFFAILURE); return FALSE; }
    if (!(lppd->Flags & PD_RETURNDEFAULT) || (lppd->Flags & ~(PD_RETURNDEFAULT | PD_RETURNDC)) ||
        !SDL_IsMainThread() || (lppd->hwndOwner && !IsWindow(lppd->hwndOwner))) { SetLastError(ERROR_NOT_SUPPORTED); goto fail; }
    lpprintOwner.reset(new (std::nothrow) COMPATPRINT{}); lp = lpprintOwner.get(); if (!lp) { dwError = CDERR_MEMALLOCFAILURE; goto fail; }
    lp->hwndOwner = lppd->hwndOwner ? HwndCompatRoot(lppd->hwndOwner) : GetActiveWindow();
    lp->cCopies = 1; lp->idPaper = DMPAPER_LETTER; lp->cxPhysical = 2550; lp->cyPhysical = 3300;
    if (pszPaper && *pszPaper && SDL_strcasecmp(pszPaper, "letter") != 0) {
        if (SDL_strcasecmp(pszPaper, "a4") != 0) { SetLastError(ERROR_INVALID_PARAMETER); goto fail; }
        lp->idPaper = DMPAPER_A4; lp->cxPhysical = 2480; lp->cyPhysical = 3508;
    }
    if (pszOrientation && *pszOrientation && SDL_strcasecmp(pszOrientation, "portrait") != 0) {
        if (SDL_strcasecmp(pszOrientation, "landscape") != 0) { SetLastError(ERROR_INVALID_PARAMETER); goto fail; }
        temp = lp->cxPhysical; lp->cxPhysical = lp->cyPhysical; lp->cyPhysical = temp;
    }
    lp->cx = lp->cxPhysical - 150; lp->cy = lp->cyPhysical - 150;
    if (pszPrinter && *pszPrinter && SDL_strcasecmp(pszPrinter, "pdf") != 0) {
        if (!FUtf8Valid(pszPrinter, strlen(pszPrinter))) { SetLastError(ERROR_INVALID_PARAMETER); goto fail; }
        if (SDL_strlcpy(lp->szPrinter, pszPrinter, sizeof(lp->szPrinter)) >= sizeof(lp->szPrinter)) { SetLastError(ERROR_INVALID_PARAMETER); goto fail; }
        lp->fSpool = TRUE;
    }
    if (pszFile && *pszFile && (!FUtf8Valid(pszFile, strlen(pszFile)) ||
        SDL_strlcpy(lp->szTarget, pszFile, sizeof(lp->szTarget)) >= sizeof(lp->szTarget))) {
        SetLastError(ERROR_INVALID_PARAMETER); goto fail;
    }
    hMode = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(*lpdm));
    hNames = HglobPrintNames(lp);
    if (!hMode || !hNames) { dwError = CDERR_MEMALLOCFAILURE; goto fail; }
    lpdm = (DEVMODEA *)(GlobalLock(hMode)); if (!lpdm) goto fail;
    CchCopyUtf8(lpdm->dmDeviceName, sizeof(lpdm->dmDeviceName), lp->fSpool ? lp->szPrinter : "Stars PDF",
                CchUtf8Prefix(lp->fSpool ? lp->szPrinter : "Stars PDF", sizeof(lpdm->dmDeviceName) - 1));
    lpdm->dmSize = sizeof(*lpdm); lpdm->dmSpecVersion = 0x401; lpdm->dmDriverVersion = 1;
    lpdm->dmFields = DM_ORIENTATION | DM_PAPERSIZE | DM_COPIES | DM_PRINTQUALITY | DM_YRESOLUTION;
    lpdm->dmOrientation = lp->cxPhysical > lp->cyPhysical ? DMORIENT_LANDSCAPE : DMORIENT_PORTRAIT;
    lpdm->dmPaperSize = lp->idPaper;
    lpdm->dmCopies = 1; lpdm->dmPrintQuality = lpdm->dmYResolution = 300;
    GlobalUnlock(hMode);
    if (lppd->Flags & PD_RETURNDC) {
        hdc = HdcCompatPrinter(lp, lp->cx, lp->cy);
        if (!hdc) { dwError = CDERR_MEMALLOCFAILURE; goto fail; }
        lpprintOwner.release(); lp = NULL;
    }
    lppd->hDevMode = hMode; lppd->hDevNames = hNames; lppd->nCopies = 1;
    if (lppd->Flags & PD_RETURNDC) lppd->hDC = hdc;
    return TRUE;
fail:
    if (hdc) DeleteDC(hdc);
    GlobalFree(hMode); GlobalFree(hNames); CompatSetDialogError(dwError); return FALSE;
}
static cairo_status_t WritePDF(void *lpv, const unsigned char *rgb, unsigned cb) {
    COMPATPRINT *lp = (COMPATPRINT *)(lpv);
    return CWriteNative(rgb, 1, cb, lp->lpfile.get()) == cb ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
}
static int StartPrint(COMPATPRINT *lp, int cb, LPCSTR pszTitle) {
    OPENFILENAMEA ofn = {};
    char szFile[PATH_MAX] = "Stars map.pdf";
    if (lp->fActive || cb < 0 || cb > 4096 || (cb != 0 && !pszTitle)) { SetLastError(ERROR_INVALID_PARAMETER); return SP_ERROR; }
    if (cb != 0 && !FUtf8Valid(pszTitle, (size_t)cb)) { SetLastError(ERROR_INVALID_PARAMETER); return SP_ERROR; }
    if (cb) memcpy(lp->szTitle, pszTitle, cb);
    lp->szTitle[cb] = 0;
    if (!lp->fSpool && !*lp->szTarget) {
        ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = IsWindow(lp->hwndOwner) ? lp->hwndOwner : NULL;
        ofn.lpstrFile = szFile; ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "PDF documents\0*.pdf\0"; ofn.nFilterIndex = 1; ofn.lpstrDefExt = "pdf";
        ofn.lpstrTitle = "Print to PDF"; ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
        if (!GetSaveFileNameA(&ofn)) return CommDlgExtendedError() ? SP_ERROR : SP_APPABORT;
        strcpy(lp->szTarget, szFile);
    }
    lp->fKeep = FALSE;
    if (lp->fSpool) strcpy(lp->szTemp, "/tmp/stars-print-XXXXXX");
    else if (snprintf(lp->szTemp, sizeof(lp->szTemp), "%s.tmp-XXXXXX", lp->szTarget) >= (int)sizeof(lp->szTemp)) {
        lp->szTemp[0] = 0; SetLastError(ERROR_INVALID_PARAMETER); return SP_ERROR;
    }
    lp->lpfile.reset(PfNativeCreateTemp(lp->szTemp));
    if (!lp->lpfile) { lp->szTemp[0] = 0; SetLastError(ERROR_ACCESS_DENIED); return SP_ERROR; }
    lp->lpPDF.reset(cairo_pdf_surface_create_for_stream(WritePDF, lp, lp->cxPhysical * 72.0 / 300, lp->cyPhysical * 72.0 / 300));
    if (cairo_surface_status(lp->lpPDF.get()) != CAIRO_STATUS_SUCCESS) { AbortPrint(lp); SetLastError(ERROR_NOT_ENOUGH_MEMORY); return SP_ERROR; }
    cairo_pdf_surface_set_metadata(lp->lpPDF.get(), CAIRO_PDF_METADATA_TITLE, lp->szTitle);
    cairo_pdf_surface_set_metadata(lp->lpPDF.get(), CAIRO_PDF_METADATA_CREATOR, "Stars SDL3 compatibility printer");
    lp->cPages = 0; lp->fActive = TRUE;
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
            return FALSE;
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
        SDL_FillSurfaceRect(lpSource, NULL, SDL_MapSurfaceRGB(lpSource, 255, 255, 255));
    }
    return fOk;
}
static BOOL FPrintMarked(SDL_Surface *lpSurface) {
    const BYTE *rgb;
    int x, y;
    for (y = 0; y < lpSurface->h; y++) {
        rgb = (BYTE *)lpSurface->pixels + (size_t)y * lpSurface->pitch;
        for (x = 0; x < lpSurface->w; x++, rgb += 4) if (rgb[0] != 255 || rgb[1] != 255 || rgb[2] != 255) return TRUE;
    }
    return FALSE;
}
static BOOL FSubmitPrint(COMPATPRINT *lp) {
    const char *rgArgs[16];
    char szCopies[16];
    SDL_Process *lpProcess;
    Uint64 ticks;
    int cArgs = 0, code = -1;
    snprintf(szCopies, sizeof(szCopies), "%d", lp->cCopies);
    rgArgs[cArgs++] = "lp";
    if (strcmp(lp->szPrinter, "default") != 0) { rgArgs[cArgs++] = "-d"; rgArgs[cArgs++] = lp->szPrinter; }
    rgArgs[cArgs++] = "-n"; rgArgs[cArgs++] = szCopies;
    rgArgs[cArgs++] = "-t"; rgArgs[cArgs++] = lp->szTitle;
    rgArgs[cArgs++] = "-o"; rgArgs[cArgs++] = lp->idPaper == DMPAPER_A4 ? "media=A4" : "media=Letter";
    rgArgs[cArgs++] = "-o"; rgArgs[cArgs++] = lp->cxPhysical > lp->cyPhysical ? "orientation-requested=4" : "orientation-requested=3";
    rgArgs[cArgs++] = "--"; rgArgs[cArgs++] = lp->szTemp; rgArgs[cArgs] = NULL;
    NATIVEOWNER<SDL_Process, SDL_DestroyProcess> lpprocessOwner(SDL_CreateProcess(rgArgs, false));
    lpProcess = lpprocessOwner.get();
    if (!lpProcess) return FALSE;
    ticks = SDL_GetTicks();
    while (!SDL_WaitProcess(lpProcess, false, &code)) {
        if (SDL_GetTicks() - ticks >= 30000) { SDL_KillProcess(lpProcess, true); SDL_WaitProcess(lpProcess, true, &code); code = -1; break; }
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
    if (idEscape == QUERYESCSUPPORT) {
        if (!pszIn || cb < (int)sizeof(int)) return 0;
        memcpy(&id, pszIn, sizeof(id));
        return id == STARTDOC || id == NEWFRAME || id == ENDDOC || id == ABORTDOC || id == QUERYESCSUPPORT || id == GETPHYSPAGESIZE || id == GETPRINTINGOFFSET;
    }
    if (idEscape == GETPHYSPAGESIZE || idEscape == GETPRINTINGOFFSET) {
        if (!lpOut) { SetLastError(ERROR_INVALID_PARAMETER); return SP_ERROR; }
        *(POINT *)lpOut = idEscape == GETPHYSPAGESIZE ? (POINT){lp->cxPhysical, lp->cyPhysical} : (POINT){75, 75}; return 1;
    }
    lpSurface = LpCompatPrinterSurface(hdc);
    if (idEscape == STARTDOC) {
        id = StartPrint(lp, cb, pszIn);
        if (id > 0) SDL_FillSurfaceRect(lpSurface, NULL, SDL_MapSurfaceRGB(lpSurface, 255, 255, 255));
        return id;
    }
    if (idEscape != NEWFRAME && idEscape != ENDDOC && idEscape != ABORTDOC) { SetLastError(ERROR_NOT_SUPPORTED); return 0; }
    if (!lp->fActive) { SetLastError(ERROR_INVALID_PARAMETER); return SP_ERROR; }
    if (idEscape == ABORTDOC) { AbortPrint(lp); return 1; }
    if (idEscape == NEWFRAME) {
        if (FPrintPage(lp, lpSurface)) return 1;
        AbortPrint(lp); SetLastError(ERROR_WRITE_FAULT); return SP_ERROR;
    }
    if ((lp->cPages == 0 || FPrintMarked(lpSurface)) && !FPrintPage(lp, lpSurface)) {
        AbortPrint(lp); SetLastError(ERROR_WRITE_FAULT); return SP_ERROR;
    }
    cairo_surface_finish(lp->lpPDF.get()); fOk = cairo_surface_status(lp->lpPDF.get()) == CAIRO_STATUS_SUCCESS;
    lp->lpPDF.reset();
    if (IFlushNativeFile(lp->lpfile.get()) != 0 || ISyncNativeFile(lp->lpfile.get()) != 0) fOk = FALSE;
    if (ICloseNativeFile(lp->lpfile.release()) != 0) fOk = FALSE;
    lp->fActive = FALSE;
    if (!fOk) { AbortPrint(lp); SetLastError(ERROR_WRITE_FAULT); return SP_ERROR; }
    if (lp->fSpool) {
        if (!FSubmitPrint(lp)) {
            lp->fKeep = TRUE; SetLastError(ERROR_WRITE_FAULT);
            SDL_Log("Printer submission failed; PDF retained at %s", lp->szTemp);
            return SP_ERROR;
        }
        IUnlinkNativePath(lp->szTemp); lp->szTemp[0] = 0;
    } else {
        if (IRenameNativePath(lp->szTemp, lp->szTarget, TRUE, TRUE) != 0) { AbortPrint(lp); SetLastError(ERROR_WRITE_FAULT); return SP_ERROR; }
        lp->szTemp[0] = 0;
        // The completed PDF remains valid even when no viewer accepts it.
        if (!FCompatOpenDocument(lp->szTarget)) {
            char szMessage[PATH_MAX + 160];
            HWND hwndOwner = IsWindow(lp->hwndOwner) ? lp->hwndOwner : NULL;
            char *pszAbsolute = PszCanonicalNativePath(lp->szTarget, NULL, 0);
            snprintf(szMessage, sizeof(szMessage), "The PDF was saved, but its viewer could not be opened.\n\nOpen this file in a PDF viewer to print it:\n%s", pszAbsolute ? pszAbsolute : lp->szTarget);
            SDL_Log("%s", szMessage);
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "PDF saved", szMessage,
                                     hwndOwner ? hwndOwner->lpWindow.get() : NULL);
            free(pszAbsolute);
        }
    }
    return 1;
}
int WINAPI Escape(HDC hdc, int idEscape, int cb, LPCSTR pszIn, LPVOID lpOut) {
    COMPATPRINT *lp = LpCompatPrinter(hdc);
    if (!lp) { SetLastError(ERROR_INVALID_HANDLE); return SP_ERROR; }
    if (lp->fEscaping) { SetLastError(ERROR_BUSY); return SP_ERROR; }
    // The PDF destination picker dispatches caller timers. Keep this DC alive
    // until its synchronous escape returns, even if a timer tries to delete it.
    StarsRestore<BOOL> escaping(lp->fEscaping);
    lp->fEscaping = TRUE;
    return IEscapePrinter(lp, hdc, idEscape, cb, pszIn, lpOut);
}
