#ifndef STARS_DECOMPILED_REPORTUI_H
#define STARS_DECOMPILED_REPORTUI_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

LRESULT  ReportDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void             SetHScrollBar(APPSESSION &app);
void             DrawReport(APPSESSION &app, HWND hwnd, HDC hdc, RECT *prc);
INT_PTR  ScoreXDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             InitScoreDlg(APPSESSION &app, HWND hwnd, int16_t fVictory);
void             DrawVCReport(APPSESSION &app, HDC hdc);
void             DrawScoreReport(APPSESSION &app, HDC hdc);
void             DrawHistoryReport(APPSESSION &app, HDC hdc);
int32_t          LFetchScoreXVal(SCOREX *lpsx, int16_t iVal);
int16_t          DxReportColHdr(APPSESSION &app, ReportType irpt, int16_t iCol, char *psz, HDC hdc);
void             DrawReportItem(APPSESSION &app, HDC hdc, RECT *prc, ReportType irpt, int16_t irow, int16_t icol);
void             DrawMineralItem(APPSESSION &app, HDC hdc, int16_t x, int16_t y, int16_t iMineral, int32_t l);
void             SortReportCache(APPSESSION &app, ReportType irpt, int16_t icol);
int              ICompReport(APPSESSION &app, uint16_t *pid1, uint16_t *pid2);
void             ReportColumnPopup(APPSESSION &app, POINT16 pt, int16_t icol, int16_t fRightBtn);
void             ExecuteReportClick(APPSESSION &app, POINT16 pt, ReportType irpt, int16_t icol, int16_t irow);
INT_PTR  PrintMapDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif
