#include "common.h"
#include "appstate.h"

// The user interface (ui.h) for programs without windows: stars-host and the
// native unit tests, which each supply IdAlertBox. Nothing is drawn, and
// questions get the cautious answer.

#ifdef STARS_TEST_GOLDEN_DIR
std::optional<std::array<char, 16>> FStarsTestPromptLegacyRacePassword(APPSESSION &app);
#endif

// PromptPassword refuses: a host takes a file's password with -p.
std::int16_t PromptPassword(APPSESSION &) { return 0; }

// The host has no interactive UI for replacing a legacy race password.
std::optional<std::array<char, 16>> FPromptLegacyRacePassword([[maybe_unused]] APPSESSION &app) {
#ifdef STARS_TEST_GOLDEN_DIR
    return FStarsTestPromptLegacyRacePassword(app);
#else
    return std::nullopt;
#endif
}

// PromptSaveGame leaves a player's unsaved orders unsaved.
void PromptSaveGame(APPSESSION &) { return; }

void UpdateProgressGauge(APPSESSION &, ProgressStep pctX10) { return; }

void CreateChildWindows(APPSESSION &) { return; }

void PostOpenGame(APPSESSION &) { return; }

void ShowTitleScreen(APPSESSION &) { return; }

void AddMRUFile(APPSESSION &, std::string_view, std::string_view) { return; }

void CloseGameWindows(APPSESSION &) { return; }

std::int16_t FFinishPlrMsgEntry(APPSESSION &, std::int16_t dInc) { return 1; }

void InvalidateReport(APPSESSION &, ReportType irpt, std::int16_t fReload) { return; }

void ShowPlanetSel(APPSESSION &) { return; }

void ShowFleetSel(APPSESSION &) { return; }

void ShowMainObjSel(APPSESSION &, std::int16_t fSameType, std::int16_t idSkip) { return; }

void ShowScanSel(APPSESSION &, std::int16_t fVis) { return; }

void ShowScanSelChange(APPSESSION &, SCAN *pscanOld, SCAN *pscan, std::int16_t fChgWp) { return; }

void ShowSelAt(APPSESSION &, POINT16 pt) { return; }

void FillShipDD(APPSESSION &, std::int16_t idSkip) { return; }

void FillFleetCompLB(APPSESSION &) { return; }

void FillSelProdLB(APPSESSION &) { return; }

void SetFleetDropDownSel(APPSESSION &, std::int16_t id) { return; }

void RedrawPlanShip(APPSESSION &, TileBits grbit) { return; }

void InvalidateMine(APPSESSION &) { return; }

void UpdateMsgTitle(APPSESSION &) { return; }

void AdvanceTutor(APPSESSION &) { return; }

// LpbLoadTutorLog has no tutorial to replay; the tutorial plays only in the
// Windows game.
std::span<const std::byte> LpbLoadTutorLog(APPSESSION &) { return {}; }

void SetHdcTextColor(HDC hdc, std::uint32_t cr) { return; }

void SetSzRaceFile(APPSESSION &app, std::string_view szFile) {
    CchCopyUtf8Prefix(std::span{app.state.szRaceFile}, szFile);
}

std::int16_t FConfirmLegacyRaceNames(APPSESSION &app, std::string_view szName, std::string_view szNames) {
    char szPrompt[512];
    CchSprintf(szPrompt, PszGetCompressedString(app, idsNativeLegacyRaceNameShortened).data(), szName, szNames);
    return IdAlertBox(szPrompt, win32::MB_YESNO | win32::MB_ICONQUESTION) == win32::IDYES;
}

APPSESSION host::app;
