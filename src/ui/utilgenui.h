#ifndef STARS_DECOMPILED_UTILGENUI_H
#define STARS_DECOMPILED_UTILGENUI_H

#include "win.h"
#include <cstdint>
#include <span>
#include <windows.h>

INT_PTR  RandomSeedDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             CtrTextOut(HDC hdc, std::int16_t x, std::int16_t y, const char *psz, std::int16_t cLen);
std::int16_t          DxStreamTextOut(HDC hdc, std::int16_t *px, std::int16_t y, const char *psz, std::int16_t cLen, std::int16_t fPrint);
void WrapTextOut(APPSESSION &app, HDC hdc, std::int16_t *px, std::int16_t *py, const char *psz, std::int16_t cLen, std::int16_t xLeft, std::int16_t dxWidth, std::int16_t *pxMax, std::int16_t fNewLine, std::int16_t fPrint);
void AddBackTrailingSpaces(const char **ppch, const char *pchEnd);
void ChopLastWord(const char *pBeg, const char **ppEnd);
void ChopTrailingSpaces(const char *pBeg, const char **ppEnd);
void RcCtrTextOut(APPSESSION &app, HDC hdc, RECT *prc, const char *psz, std::int16_t cLen);
// The mutable overload preserves the legacy cLen == -1 output-buffer request.
void RightTextOut(APPSESSION &app, HDC hdc, std::int16_t x, std::int16_t y, const char *psz, std::int16_t cLen, std::int16_t dxErase);
void RightTextOut(APPSESSION &app, HDC hdc, std::int16_t x, std::int16_t y, std::span<char> psz, std::int16_t cLen, std::int16_t dxErase);
template<std::size_t N>
void RightTextOut(APPSESSION &app, HDC hdc, std::int16_t x, std::int16_t y, char (&psz)[N], std::int16_t cLen, std::int16_t dxErase) {
    RightTextOut(app, hdc, x, y, std::span{psz}, cLen, dxErase);
}
void DiaganolTextOut(APPSESSION &app, HDC hdc, RECT *prc, const char *psz, std::int16_t cLen);
void ExpandRc(RECT *prc, std::int16_t dx, std::int16_t dy);
void OffsetRc(RECT *prc, std::int16_t dx, std::int16_t dy);
void StickyDlgPos(HWND hwnd, POINT16 *ppt, std::int16_t fInit);
std::int32_t  LDrawGauge(APPSESSION &app, HDC hdc, RECT *prc, std::int16_t cSegs, std::int32_t *rgSize, HBRUSH *rghbr, std::int32_t cTot);
void     _Draw3dFrame(APPSESSION &app, HDC hdc, RECT *prc, std::int16_t fErase);
void     InitBtnTrack(BTNT *pbtnt, HWND hwnd, HDC hdc, RECT *prc, std::int16_t btf, std::int16_t dTimer, std::int16_t fInitDown, std::int16_t fNoEndRedraw, const char *szText);
std::int16_t  FTrackBtn(APPSESSION &app, BTNT *pbtnt);
void     DrawBtn(APPSESSION &app, HDC hdc, RECT *prc, std::int16_t bt, std::int16_t fDown, const char *szText);
std::int16_t  FGetMouseMove(POINT16 *ppt);
std::int16_t  FGetRMouseMove(POINT16 *ppt);
void     DrawFuzzyBorder(APPSESSION &app, HDC hdc, RECT *prc);
// Text must terminate inside its mutable view; rejected views stay untouched.
bool FStringFitsScreen(APPSESSION &app, std::span<char> lpsz, std::int16_t dxMax);
HBRUSH   HbrGet(APPSESSION &app, COLORREF cr);
void     FreeHbr(APPSESSION &app, HBRUSH hbr);
std::uint16_t DibNumColors(void *pv);
HPALETTE HpalFromDib(HGLOBAL hdib);
HPALETTE HpalBlackReserved();
std::uint16_t PaletteSize(void *pv);
std::int16_t  DibBlt(HDC hdc, std::int16_t x0, std::int16_t y0, std::int16_t dx, std::int16_t dy, HGLOBAL hdib, std::int16_t x1, std::int16_t y1, std::int16_t dxSrc, std::int16_t dySrc, std::int32_t rop);
HGLOBAL  DibFromBitmap(HBITMAP hbm, std::uint32_t biStyle, std::uint16_t biBits, HPALETTE hpal);
HGLOBAL  HdibLoadBigResource(APPSESSION &app, BitmapId idb);
INT_PTR  PasswordDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR  NewPasswordDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             ShowProgressGauge(APPSESSION &app);
void             HideProgressGauge(APPSESSION &app);
INT_PTR  ProgressGaugeDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DrawProgressGauge(APPSESSION &app, HDC hdcOrig, std::int16_t fFull, std::int16_t iNumOnly);
HFONT            HfontPrinterCreate(APPSESSION &app, HDC hdc, std::int16_t iSize, std::int16_t *pdyFont);

#endif
