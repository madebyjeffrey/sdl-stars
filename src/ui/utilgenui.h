#ifndef STARS_DECOMPILED_UTILGENUI_H
#define STARS_DECOMPILED_UTILGENUI_H

#include <stdint.h>
#include <windows.h>

INT_PTR CALLBACK RandomSeedDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             CtrTextOut(HDC hdc, int16_t x, int16_t y, char *psz, int16_t cLen);
int16_t          DxStreamTextOut(HDC hdc, int16_t *px, int16_t y, char *psz, int16_t cLen, int16_t fPrint);
void WrapTextOut(HDC hdc, int16_t *px, int16_t *py, char *psz, int16_t cLen, int16_t xLeft, int16_t dxWidth, int16_t *pxMax, int16_t fNewLine, int16_t fPrint);
void AddBackTrailingSpaces(char **ppch, char *pchEnd);
void ChopLastWord(char *pBeg, char **ppEnd);
void ChopTrailingSpaces(char *pBeg, char **ppEnd);
void RcCtrTextOut(HDC hdc, RECT *prc, char *psz, int16_t cLen);
void RightTextOut(HDC hdc, int16_t x, int16_t y, char *psz, int16_t cLen, int16_t dxErase);
void DiaganolTextOut(HDC hdc, RECT *prc, char *psz, int16_t cLen);
void ExpandRc(RECT *prc, int16_t dx, int16_t dy);
void OffsetRc(RECT *prc, int16_t dx, int16_t dy);
void StickyDlgPos(HWND hwnd, POINT16 *ppt, int16_t fInit);
int32_t  LDrawGauge(HDC hdc, RECT *prc, int16_t cSegs, int32_t *rgSize, HBRUSH *rghbr, int32_t cTot);
void     _Draw3dFrame(HDC hdc, RECT *prc, int16_t fErase);
void     InitBtnTrack(BTNT *pbtnt, HWND hwnd, HDC hdc, RECT *prc, int16_t btf, int16_t dTimer, int16_t fInitDown, int16_t fNoEndRedraw, char *szText);
int16_t  FTrackBtn(BTNT *pbtnt);
void     DrawBtn(HDC hdc, RECT *prc, int16_t bt, int16_t fDown, char *szText);
int16_t  FGetMouseMove(POINT16 *ppt);
int16_t  FGetRMouseMove(POINT16 *ppt);
void     DrawFuzzyBorder(HDC hdc, RECT *prc);
int16_t  FStringFitsScreen(char *lpsz, int16_t dxMax);
HBRUSH   HbrGet(COLORREF cr);
void     FreeHbr(HBRUSH hbr);
uint16_t DibNumColors(void *pv);
HPALETTE HpalFromDib(HGLOBAL hdib);
HPALETTE HpalBlackReserved();
uint16_t PaletteSize(void *pv);
int16_t  DibBlt(HDC hdc, int16_t x0, int16_t y0, int16_t dx, int16_t dy, HGLOBAL hdib, int16_t x1, int16_t y1, int16_t dxSrc, int16_t dySrc, int32_t rop);
HGLOBAL  DibFromBitmap(HBITMAP hbm, uint32_t biStyle, uint16_t biBits, HPALETTE hpal);
HGLOBAL  HdibLoadBigResource(BitmapId idb);
INT_PTR CALLBACK PasswordDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NewPasswordDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             ShowProgressGauge();
void             HideProgressGauge();
INT_PTR CALLBACK ProgressGaugeDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             DrawProgressGauge(HDC hdcOrig, int16_t fFull, int16_t iNumOnly);
HFONT            HfontPrinterCreate(HDC hdc, int16_t iSize, int16_t *pdyFont);

#endif
