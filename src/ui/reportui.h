#ifndef STARS_DECOMPILED_REPORTUI_H
#define STARS_DECOMPILED_REPORTUI_H

#include <stdint.h>
#include <windows.h>

LRESULT CALLBACK ReportDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void             SetHScrollBar();
void             DrawReport(HWND hwnd, HDC hdc, RECT *prc);
INT_PTR CALLBACK ScoreXDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             InitScoreDlg(HWND hwnd, int16_t fVictory);
void             DrawVCReport(HDC hdc);
void             DrawScoreReport(HDC hdc);
void             DrawHistoryReport(HDC hdc);
int32_t          LFetchScoreXVal(SCOREX *lpsx, int16_t iVal);
int16_t          DxReportColHdr(ReportType irpt, int16_t iCol, char *psz, HDC hdc);
void             DrawReportItem(HDC hdc, RECT *prc, ReportType irpt, int16_t irow, int16_t icol);
void             DrawMineralItem(HDC hdc, int16_t x, int16_t y, int16_t iMineral, int32_t l);
void             SortReportCache(ReportType irpt, int16_t icol);
int              ICompReport(uint16_t *pid1, uint16_t *pid2);
void             ReportColumnPopup(POINT16 pt, int16_t icol, int16_t fRightBtn);
void             ExecuteReportClick(POINT16 pt, ReportType irpt, int16_t icol, int16_t irow);
INT_PTR CALLBACK PrintMapDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif
