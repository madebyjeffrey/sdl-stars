#ifndef STARS_DECOMPILED_REPORTUI_H
#define STARS_DECOMPILED_REPORTUI_H

#include "win.h"
#include <cstdint>
#include <windows.h>

LRESULT  ReportDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void             SetHScrollBar(APPSESSION &app);
void             DrawReport(APPSESSION &app, HWND hwnd, HDC hdc, RECT *prc);
INT_PTR  ScoreXDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             InitScoreDlg(APPSESSION &app, HWND hwnd, std::int16_t fVictory);
void             DrawVCReport(APPSESSION &app, HDC hdc);
void             DrawScoreReport(APPSESSION &app, HDC hdc);
void             DrawHistoryReport(APPSESSION &app, HDC hdc);
std::int32_t          LFetchScoreXVal(SCOREX *lpsx, std::int16_t iVal);
std::int16_t          DxReportColHdr(APPSESSION &app, ReportType irpt, std::int16_t iCol, char *psz, HDC hdc);
void             DrawReportItem(APPSESSION &app, HDC hdc, RECT *prc, ReportType irpt, std::int16_t irow, std::int16_t icol);
void             DrawMineralItem(APPSESSION &app, HDC hdc, std::int16_t x, std::int16_t y, std::int16_t iMineral, std::int32_t l);
void             SortReportCache(APPSESSION &app, ReportType irpt, std::int16_t icol);
int              ICompReport(APPSESSION &app, std::uint16_t *pid1, std::uint16_t *pid2);
void             ReportColumnPopup(APPSESSION &app, POINT16 pt, std::int16_t icol, std::int16_t fRightBtn);
void             ExecuteReportClick(APPSESSION &app, POINT16 pt, ReportType irpt, std::int16_t icol, std::int16_t irow);
INT_PTR  PrintMapDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif
