#include "common.h"
#include "appstate.h"

// The user interface (ui.h) for programs without windows: stars-host and the
// native unit tests, which each supply IdAlertBox. Nothing is drawn, and
// questions get the cautious answer.

#ifdef STARS_TEST_GOLDEN_DIR
int16_t FStarsTestPromptLegacyRacePassword(APPSESSION &app, char *szPassword);
#endif

// PromptPassword refuses: a host takes a file's password with -p.
int16_t PromptPassword(APPSESSION &) { return 0; }

// The host has no interactive UI for replacing a legacy race password.
int16_t FPromptLegacyRacePassword([[maybe_unused]] APPSESSION &app, char *szPassword) {
#ifdef STARS_TEST_GOLDEN_DIR
    return FStarsTestPromptLegacyRacePassword(app, szPassword);
#else
    (void)szPassword;
    return 0;
#endif
}

// PromptSaveGame leaves a player's unsaved orders unsaved.
void PromptSaveGame(APPSESSION &) { return; }

void UpdateProgressGauge(APPSESSION &, ProgressStep pctX10) { return; }

void CreateChildWindows(APPSESSION &) { return; }

void PostOpenGame(APPSESSION &) { return; }

void ShowTitleScreen(APPSESSION &) { return; }

void AddMRUFile(APPSESSION &, const char *pszFileName, const char *pszExt) { return; }

void CloseGameWindows(APPSESSION &) { return; }

int16_t FFinishPlrMsgEntry(APPSESSION &, int16_t dInc) { return 1; }

void InvalidateReport(APPSESSION &, ReportType irpt, int16_t fReload) { return; }

void ShowPlanetSel(APPSESSION &) { return; }

void ShowFleetSel(APPSESSION &) { return; }

void ShowMainObjSel(APPSESSION &, int16_t fSameType, int16_t idSkip) { return; }

void ShowScanSel(APPSESSION &, int16_t fVis) { return; }

void ShowScanSelChange(APPSESSION &, SCAN *pscanOld, SCAN *pscan, int16_t fChgWp) { return; }

void ShowSelAt(APPSESSION &, POINT16 pt) { return; }

void FillShipDD(APPSESSION &, int16_t idSkip) { return; }

void FillFleetCompLB(APPSESSION &) { return; }

void FillSelProdLB(APPSESSION &) { return; }

void SetFleetDropDownSel(APPSESSION &, int16_t id) { return; }

void RedrawPlanShip(APPSESSION &, TileBits grbit) { return; }

void InvalidateMine(APPSESSION &) { return; }

void UpdateMsgTitle(APPSESSION &) { return; }

void AdvanceTutor(APPSESSION &) { return; }

// LpbLoadTutorLog has no tutorial to replay; the tutorial plays only in the
// Windows game.
uint8_t *LpbLoadTutorLog(APPSESSION &) { return NULL; }

void SetHdcTextColor(HDC hdc, uint32_t cr) { return; }

void SetSzRaceFile(APPSESSION &app, const char *pszFile) {
    size_t cch = CchUtf8Prefix(pszFile, sizeof(app.state.szRaceFile) - 1);
    memcpy(app.state.szRaceFile, pszFile, cch); app.state.szRaceFile[cch] = 0;
}

int16_t FConfirmLegacyRaceNames(APPSESSION &app, char *szName, char *szNames) {
    char szPrompt[512];
    CchSprintf(szPrompt, PszGetCompressedString(app, idsNativeLegacyRaceNameShortened), szName, szNames);
    return IdAlertBox(szPrompt, win32::MB_YESNO | win32::MB_ICONQUESTION) == win32::IDYES;
}

APPSESSION host::app;
