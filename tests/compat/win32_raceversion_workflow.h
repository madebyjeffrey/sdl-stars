#include "appstate.h"
#include "uistate.h"
/* Save through the actual wizard, then inspect immutable earlier versions. */
static BOOL FCheckRaceVersionWorkflow(APPSESSION &app) {
    static int iRun, iPage, iPhase, cPrompts;
    static const DLGPROC rglpfn[] = {RaceWizardDlg1, RaceWizardDlg4, RaceWizardDlg5, RaceWizardDlg2, RaceWizardDlg3, RaceWizardDlg6};
    static const char *rgszPlural[] = {"Versions A", "Versions A", "Versions B", "Versions C", "Versions C", "Versions D", "Versions E", "Versions F", "Versions G"};
    const char *pszRoot = PszCompatEnvironment("STARS_SETTINGS_DIR");
    const char *pszName = iRun == 8 ? "Legacy" : iRun == 6 ? "\xc3\xa9LAN/CON" : "\xc3\x89lan/CON";
    HWND hwnd, hwndDialog;
    PLAYER plr;
    char szImported[512], szImportedSnapshot[512], szImportedVersion[512], szPrompt[512];
    char szFirst[512], szSecond[512], szFirstSnapshot[512], szSecondSnapshot[512];
    char szThird[512], szFailed[512], szThirdSnapshot[512];

    CchSnprintf(std::span{szImported, sizeof(szImported)}, "%s%sRace Designs%sImported.r8", pszRoot, szDirSep, szDirSep);
    CchSnprintf(std::span{szImportedVersion, sizeof(szImportedVersion)}, "%s%sRace Designs%sLegacy.r9", pszRoot, szDirSep, szDirSep);
    CchSnprintf(std::span{szImportedSnapshot, sizeof(szImportedSnapshot)}, "%s%simported.snapshot", pszRoot, szDirSep);
    CchSnprintf(std::span{szFirst, sizeof(szFirst)}, "%s%sRace Designs%s\xc3\x89lan_CON.r1", pszRoot, szDirSep, szDirSep);
    CchSnprintf(std::span{szSecond, sizeof(szSecond)}, "%s%sRace Designs%s\xc3\x89lan_CON.r2", pszRoot, szDirSep, szDirSep);
    CchSnprintf(std::span{szThird, sizeof(szThird)}, "%s%sRace Designs%s\xc3\xa9LAN_CON.r3", pszRoot, szDirSep, szDirSep);
    CchSnprintf(std::span{szFailed, sizeof(szFailed)}, "%s%sRace Designs%s\xc3\x89lan_CON.r4", pszRoot, szDirSep, szDirSep);
    CchSnprintf(std::span{szThirdSnapshot, sizeof(szThirdSnapshot)}, "%s%sthird.snapshot", pszRoot, szDirSep);
    CchSnprintf(std::span{szFirstSnapshot, sizeof(szFirstSnapshot)}, "%s%sfirst.snapshot", pszRoot, szDirSep);
    CchSnprintf(std::span{szSecondSnapshot, sizeof(szSecondSnapshot)}, "%s%ssecond.snapshot", pszRoot, szDirSep);
    for (hwnd = HwndCompatFirstTop(); hwnd; hwnd = win32::GetWindow(hwnd, win32::GW_HWNDNEXT))
        if (hwnd->fDialog && hwnd->lpfnDlg == LibraryNameDlg) {
            fprintf(stderr, "New Race unexpectedly asked for a filename\n");
            return 0;
        }
    if (iPhase == 0) {
        if (!win32::IsWindowVisible(app.lpui->hwndTitle) || !win32::IsWindowEnabled(app.lpui->hwndTitle)) return 1;
        fChecked = fTitlePainted = 1;
        iPhase = 1;
        if (iRun == 0 || iRun == 1 || iRun == 6 || iRun == 8) return FPushClick(app, win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RACE_NEW));
        RefreshLibraryRaces(app, app.lpui->hwndTitle);
        if (!FSelectClientRaceName(app.lpui->hwndTitle, iRun == 2 ? "\xc3\x89lan_CON.r1" : iRun == 7 ? "\xc3\xa9LAN_CON.r3" : "\xc3\x89lan_CON.r2")) return 0;
        return FPushClick(app, win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_RACE_EDIT));
    }
    if (iPhase == 1) {
        hwndDialog = HwndRosterDialog(app, LibraryRaceSaveDlg);
        if (hwndDialog) {
            cPrompts++;
            if ((iRun != 2 && iRun != 3 && iRun != 5 && iRun != 6 && iRun != 7 && iRun != 8) || cPrompts != 1) return 0;
            if (iRun == 8) {
                if (!win32::GetDlgItemTextA(hwndDialog, IDC_LIBRARY_RACE_SAVE_TEXT, szPrompt, sizeof(szPrompt)) ||
                    !strstr(szPrompt, "Imported.r8")) return 0;
            }
            if (iRun == 5) iPhase = 2;
            if (iRun == 7) { fFailRaceVersionSave = 1; iPhase = 4; }
            return FPushClick(app, win32::GetDlgItem(hwndDialog, iRun == 5 ? IDCANCEL :
                                        (iRun == 3 || iRun == 7) ? IDC_LIBRARY_RACE_UPDATE : IDC_LIBRARY_RACE_VERSION));
        }
        if (iPage < 6) {
            hwndDialog = HwndRosterDialog(app, rglpfn[iPage]);
            if (!hwndDialog) return 1;
            if (iPage == 0 && (!win32::SetDlgItemTextA(hwndDialog, IDC_RACE_NAME, pszName) ||
                              !win32::SetDlgItemTextA(hwndDialog, IDC_RACE_PLURAL_NAME, rgszPlural[iRun]))) return 0;
            return FPushClick(app, win32::GetDlgItem(hwndDialog, iPage++ == 5 ? IDC_FINISH : IDC_NEXT));
        }
        if (!win32::IsWindowEnabled(app.lpui->hwndTitle)) return 1;
        iPhase = 3;
    }
    if (iPhase == 4) {
        if (!fRaceVersionSaveFailed) return 1;
        iPhase = 2;
    }
    if (iPhase == 2) {
        hwndDialog = HwndRosterDialog(app, RaceWizardDlg1);
        if (!hwndDialog) return 1;
        iPhase = 3;
        return FPushClick(app, win32::GetDlgItem(hwndDialog, IDCANCEL));
    }
    if (!win32::IsWindowEnabled(app.lpui->hwndTitle)) return 1;
    if (iRun == 0) {
        if (!FTestCopyFile(szFirst, szFirstSnapshot)) return 0;
    } else if (!FTestFilesEqual(szFirst, szFirstSnapshot)) return 0;
    if (iRun == 1 && (FFileExists(szSecond) || cPrompts != 0)) return 0;
    if (iRun == 2 && (!FFileExists(szSecond) || FTestFilesEqual(szFirst, szSecond) || cPrompts != 1)) return 0;
    if (iRun == 3 && (!FTestCopyFile(szSecond, szSecondSnapshot) || cPrompts != 1)) return 0;
    if ((iRun == 4 || iRun == 5) && (!FTestFilesEqual(szSecond, szSecondSnapshot) ||
        FFileExists(szThird) || cPrompts != (iRun == 5 ? 1 : 0))) return 0;
    if (iRun == 6) {
        if (!FFileExists(szThird) || !FTestFilesEqual(szSecond, szSecondSnapshot) || cPrompts != 1) return 0;
        if (!FLoadLibraryRace(app, app.lpui->hwndTitle, "\xc3\xa9LAN_CON.r3") || strcmp(app.state.vplr.szName, pszName) ||
            strcmp(app.state.vplr.szNames, rgszPlural[iRun]) || !FTestCopyFile(szThird, szThirdSnapshot)) return 0;
    }
    if (iRun == 7) {
        if (!fRaceVersionSaveFailed || fFailRaceVersionSave || FFileExists(szFailed) ||
            !FTestFilesEqual(szSecond, szSecondSnapshot) || !FTestFilesEqual(szThird, szThirdSnapshot)) return 0;
        plr = vrgplrDef[0];
        strcpy(plr.szName, "Legacy");
        strcpy(plr.szNames, "Old");
        if (!FSaveLibraryRace(app, app.lpui->hwndTitle, "Imported.r8", &plr) || !FTestCopyFile(szImported, szImportedSnapshot)) return 0;
    }
    if (iRun == 8) {
        if (cPrompts != 1 || !FTestFilesEqual(szImported, szImportedSnapshot) ||
            !FFileExists(szImportedVersion) || !FLoadLibraryRace(app, app.lpui->hwndTitle, "Legacy.r9") ||
            strcmp(app.state.vplr.szName, "Legacy") || strcmp(app.state.vplr.szNames, rgszPlural[iRun])) return 0;
        fPassed = 1;
        puts("Wizard names, Update/New Version/Cancel, failed saves, UTF-8 and imported version families passed");
        win32::PostQuitMessage(0);
        return 1;
    }
    iRun++;
    iPage = iPhase = cPrompts = 0;
    return 1;
}
