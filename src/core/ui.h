#ifndef STARS_DECOMPILED_UI_H
#define STARS_DECOMPILED_UI_H

// The game code calls these to reach the player. The Windows game
// implements them in its UI files; stars-host implements them in host.c,
// where there is no one to ask. See docs/ROADMAP.md (2.9).

#include <stdint.h>

// Messages and questions.
int16_t IdAlertBox(char *sz, int16_t mbType);
int16_t PromptPassword();
int16_t FPromptLegacyRacePassword(char *szPassword);
void    PromptSaveGame();

// Progress during turn generation and universe creation.
void UpdateProgressGauge(ProgressStep pctX10);

// Windows that show the game. The host has none.
void    CreateChildWindows();
void    PostOpenGame();
void    ShowTitleScreen();
void    SetSzRaceFile(const char *pszFile);
int16_t FConfirmLegacyRaceNames(char *szName, char *szNames);
void    AddMRUFile(char *pszFileName, char *pszExt);
void    CloseGameWindows();
int16_t FFinishPlrMsgEntry(int16_t dInc);

// The selection's views. ChangeMainObjSel, ChangeScanSel and the code
// that changes the selected objects call these after changing sel.
void InvalidateReport(ReportType irpt, int16_t fReload);
void ShowPlanetSel();
void ShowFleetSel();
void ShowMainObjSel(int16_t fSameType, int16_t idSkip);
void ShowScanSel(int16_t fVis);
void ShowScanSelChange(SCAN *pscanOld, SCAN *pscan, int16_t fChgWp);
void ShowSelAt(POINT16 pt);
void FillShipDD(int16_t idSkip);
void FillFleetCompLB();
void FillSelProdLB();
void SetFleetDropDownSel(int16_t id);
void RedrawPlanShip(TileBits grbit);
void InvalidateMine();
void UpdateMsgTitle();

// Report text: the fleet report's helpers colour the text they return when
// they draw it into hdc.
void SetHdcTextColor(HDC hdc, uint32_t cr);

// The tutorial.
void     AdvanceTutor();
uint8_t *LpbLoadTutorLog();

#endif
