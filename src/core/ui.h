#ifndef STARS_DECOMPILED_UI_H
#define STARS_DECOMPILED_UI_H

#include "common.h"
// The game code calls these to reach the player. The Windows game
// implements them in its UI files; stars-host implements them in host.c,
// where there is no one to ask. See docs/ROADMAP.md (2.9).

#include <stdint.h>

// Messages and questions.
int16_t IdAlertBox(const char *sz, int16_t mbType);
int16_t PromptPassword(APPSESSION &app);
int16_t FPromptLegacyRacePassword(APPSESSION &app, char *szPassword);
void    PromptSaveGame(APPSESSION &app);

// Progress during turn generation and universe creation.
void UpdateProgressGauge(APPSESSION &app, ProgressStep pctX10);

// Windows that show the game. The host has none.
void    CreateChildWindows(APPSESSION &app);
void    PostOpenGame(APPSESSION &app);
void    ShowTitleScreen(APPSESSION &app);
void    SetSzRaceFile(APPSESSION &app, const char *pszFile);
int16_t FConfirmLegacyRaceNames(APPSESSION &app, char *szName, char *szNames);
void    AddMRUFile(APPSESSION &app, const char *pszFileName, const char *pszExt);
void    CloseGameWindows(APPSESSION &app);
int16_t FFinishPlrMsgEntry(APPSESSION &app, int16_t dInc);

// The selection's views. ChangeMainObjSel, ChangeScanSel and the code
// that changes the selected objects call these after changing sel.
void InvalidateReport(APPSESSION &app, ReportType irpt, int16_t fReload);
void ShowPlanetSel(APPSESSION &app);
void ShowFleetSel(APPSESSION &app);
void ShowMainObjSel(APPSESSION &app, int16_t fSameType, int16_t idSkip);
void ShowScanSel(APPSESSION &app, int16_t fVis);
void ShowScanSelChange(APPSESSION &app, SCAN *pscanOld, SCAN *pscan, int16_t fChgWp);
void ShowSelAt(APPSESSION &app, POINT16 pt);
void FillShipDD(APPSESSION &app, int16_t idSkip);
void FillFleetCompLB(APPSESSION &app);
void FillSelProdLB(APPSESSION &app);
void SetFleetDropDownSel(APPSESSION &app, int16_t id);
void RedrawPlanShip(APPSESSION &app, TileBits grbit);
void InvalidateMine(APPSESSION &app);
void UpdateMsgTitle(APPSESSION &app);

// Report text: the fleet report's helpers colour the text they return when
// they draw it into hdc.
void SetHdcTextColor(HDC hdc, uint32_t cr);

// The tutorial.
void     AdvanceTutor(APPSESSION &app);
uint8_t *LpbLoadTutorLog(APPSESSION &app);

#endif
