#include "common.h"

// The user interface (ui.h) for programs without windows: stars-host and the
// native unit tests, which each supply IdAlertBox. Nothing is drawn, and
// questions get the cautious answer.

// PromptPassword refuses: a host takes a file's password with -p.
int16_t PromptPassword() { return FALSE; }

// PromptSaveGame leaves a player's unsaved orders unsaved.
void PromptSaveGame() { return; }

void UpdateProgressGauge(ProgressStep pctX10) { return; }

void CreateChildWindows() { return; }

void PostOpenGame() { return; }

void ShowTitleScreen() { return; }

void AddMRUFile(char *pszFileName, char *pszExt) { return; }

void CloseGameWindows() { return; }

int16_t FFinishPlrMsgEntry(int16_t dInc) { return TRUE; }

void InvalidateReport(ReportType irpt, int16_t fReload) { return; }

void ShowPlanetSel() { return; }

void ShowFleetSel() { return; }

void ShowMainObjSel(int16_t fSameType, int16_t idSkip) { return; }

void ShowScanSel(int16_t fVis) { return; }

void ShowScanSelChange(SCAN *pscanOld, SCAN *pscan, int16_t fChgWp) { return; }

void ShowSelAt(POINT16 pt) { return; }

void FillShipDD(int16_t idSkip) { return; }

void FillFleetCompLB() { return; }

void FillSelProdLB() { return; }

void SetFleetDropDownSel(int16_t id) { return; }

void RedrawPlanShip(TileBits grbit) { return; }

void InvalidateMine() { return; }

void UpdateMsgTitle() { return; }

void AdvanceTutor() { return; }

// LpbLoadTutorLog has no tutorial to replay; the tutorial plays only in the
// Windows game.
uint8_t *LpbLoadTutorLog() { return NULL; }

void SetHdcTextColor(HDC hdc, uint32_t cr) { return; }

void SetSzRaceFile(const char *pszFile) {
    size_t cch = CchUtf8Prefix(pszFile, sizeof(szRaceFile) - 1);
    memcpy(szRaceFile, pszFile, cch); szRaceFile[cch] = 0;
}
