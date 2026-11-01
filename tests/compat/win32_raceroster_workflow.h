#include "appstate.h"
#include "uistate.h"
/* Choose saved designs through the real roster and preserve unrelated versions. */
static HWND HwndRosterDialog(APPSESSION &app, DLGPROC lpfn) {
    HWND hwnd;
    for (hwnd = win32::GetWindow(app.lpui->hwndTitle, win32::GW_HWNDFIRST); hwnd; hwnd = win32::GetWindow(hwnd, win32::GW_HWNDNEXT))
        if (hwnd->fDialog && hwnd->lpfnDlg == lpfn && win32::IsWindowVisible(hwnd) && win32::IsWindowEnabled(hwnd)) return hwnd;
    return NULL;
}

static BOOL FChooseRosterAction(APPSESSION &app, HWND hwnd, int iSlot, int iMenu) {
    RECT rc;
    win32::UpdateWindow(hwnd);
    iRosterMenu = iMenu;
    win32::SetRect(&rc, app.lpui->xNewGameDiamond, 6 + iSlot * (app.state.dyArial8 + 4),
            app.lpui->xNewGameDiamond + app.state.dyArial8 + 1, 6 + iSlot * (app.state.dyArial8 + 4) + app.state.dyArial8 + 1);
    return FPushClientRect(hwnd, &rc, SDL_BUTTON_LEFT);
}

static BOOL FCheckRaceRosterWorkflow(APPSESSION &app) {
    static int iStep, iPage, iSlot;
    static uint16_t icksA;
    static const DLGPROC rglpfn[] = {RaceWizardDlg1, RaceWizardDlg4, RaceWizardDlg5, RaceWizardDlg2, RaceWizardDlg3, RaceWizardDlg6};
    static const char *rgszName[] = {"Roster-shared-prefix-A & \xce\xa9.r1", "Roster-shared-prefix-B.r1"};
    HWND hwnd;
    char szManaged[512], szOther[512], szSnapshot[512], szFirstSnapshot[512], szCaption[128];
    const char *pszRoot = PszCompatEnvironment("STARS_SETTINGS_DIR");

    CchSnprintf(std::span{szManaged, sizeof(szManaged)}, "%s%sRace Designs%s%s", pszRoot, szDirSep, szDirSep, rgszName[0]);
    CchSnprintf(std::span{szOther, sizeof(szOther)}, "%s%sRace Designs%s%s", pszRoot, szDirSep, szDirSep, rgszName[1]);
    CchSnprintf(std::span{szSnapshot, sizeof(szSnapshot)}, "%s%sB.snapshot", pszRoot, szDirSep);
    CchSnprintf(std::span{szFirstSnapshot, sizeof(szFirstSnapshot)}, "%s%sA.snapshot", pszRoot, szDirSep);
    if (iStep == 0) {
        if (!win32::IsWindowVisible(app.lpui->hwndTitle)) return 1;
        fChecked = 1;
        win32::UpdateWindow(app.lpui->hwndTitle);
        fTitlePainted = FCompatPresent(app.lpui->hwndTitle);
        if (!fTitlePainted || !FSaveLibraryRace(app, app.lpui->hwndTitle, rgszName[0], &vrgplrDef[0]) ||
            !FSaveLibraryRace(app, app.lpui->hwndTitle, rgszName[1], &vrgplrDef[1])) return 0;
        iStep++;
        return FPushClick(app, app.lpui->rghwndBtnSplash[0]);
    }
    if (iStep == 1) {
        hwnd = HwndRosterDialog(app, NewGameDlg);
        if (!hwnd) return 1;
        win32::GetWindowTextA(hwnd, szCaption, sizeof(szCaption));
        if (strcmp(szCaption, "New Game - Step 1 of 3")) return 0;
        iStep++;
        return FPushClick(app, win32::GetDlgItem(hwnd, IDC_NEXT));
    }
    if (iStep == 2 || iStep == 3) {
        hwnd = HwndRosterDialog(app, NewGameDlg2);
        if (!hwnd) return 1;
        if (iStep == 3 && (app.state.vrgplrTypeNew[0] & 3) != 2) return 0;
        pszRosterRace = rgszName[iStep - 2];
        iStep++;
        return FChooseRosterAction(app, hwnd, iStep == 3 ? 0 : 1, 1);
    }
    if (iStep == 4) {
        hwnd = HwndRosterDialog(app, NewGameDlg2);
        if (!hwnd) return 1;
        if (!FTestCopyFile(szOther, szSnapshot) || FWasRaceFile(app, szManaged, 0) <= 0) return 0;
        icksA = IRaceChecksum(&app.state.vplr);
        iStep++;
        return FChooseRosterAction(app, hwnd, 0, 2);
    }
    if (iStep == 5 || iStep == 8) {
        if (iPage < 6) {
            hwnd = HwndRosterDialog(app, rglpfn[iPage]);
            if (!hwnd) return 1;
            if (iPage == 0 && !FPushClick(app, win32::GetDlgItem(hwnd, IDC_RACE_HUMANOID + 2))) return 0;
            return FPushClick(app, win32::GetDlgItem(hwnd, iPage++ == 5 ? IDC_FINISH : IDC_NEXT));
        }
        hwnd = HwndRosterDialog(app, NewGameDlg2);
        if (!hwnd) return 1;
        if (!FTestFilesEqual(szOther, szSnapshot)) return 0;
        if (iStep == 5) {
            if (FWasRaceFile(app, szManaged, 0) <= 0 || IRaceChecksum(&app.state.vplr) == icksA ||
                !FTestCopyFile(szManaged, szFirstSnapshot)) return 0;
            pszRosterRace = rgszName[0];
            iStep++;
            return FChooseRosterAction(app, hwnd, 0, 1);
        }
        if (!FTestFilesEqual(szManaged, szFirstSnapshot)) return 0;
        iStep++;
        return FChooseRosterAction(app, hwnd, 1, 56);
    }
    if (iStep == 6) {
        hwnd = HwndRosterDialog(app, NewGameDlg2);
        if (!hwnd) return 1;
        if (strcmp(PszCurrentLibraryRaceName(), rgszName[0]) || (app.state.vrgplrTypeNew[0] & 3) != 2) return 0;
        iStep++;
        return FChooseRosterAction(app, hwnd, 0, 16);
    }
    if (iStep == 7) {
        hwnd = HwndRosterDialog(app, NewGameDlg2);
        if (!hwnd) return 1;
        if (app.state.vrgplrTypeNew[0] != 1) return 0;
        iStep++;
        iPage = 0;
        return FChooseRosterAction(app, hwnd, 0, 2);
    }
    if (iStep == 9) {
        hwnd = HwndRosterDialog(app, NewGameDlg2);
        if (!hwnd) return 1;
        if (app.state.vrgplrTypeNew[1] != 35) return 0;
        iStep++;
        return FChooseRosterAction(app, hwnd, 15, 3);
    }
    if (iStep == 10) {
        hwnd = HwndRosterDialog(app, NewGameDlg2);
        if (!hwnd) return 1;
        if (app.state.vrgplrTypeNew[15] != 0) return 0;
        iStep++;
    }
    if (iStep == 11) {
        hwnd = HwndRosterDialog(app, NewGameDlg2);
        if (!hwnd) return 1;
        if (iSlot < 20) {
            pszRosterRace = rgszName[iSlot & 1];
            return FChooseRosterAction(app, hwnd, iSlot++ % 16, 1);
        }
        if (app.lpui->vcplrNew != 16 || (app.state.vrgplrTypeNew[3] & 3) != 2 ||
            !FTestFilesEqual(szManaged, szFirstSnapshot) || !FTestFilesEqual(szOther, szSnapshot)) return 0;
        iStep++;
        return FPushClick(app, win32::GetDlgItem(hwnd, IDCANCEL));
    }
    if (!win32::IsWindowVisible(app.lpui->hwndTitle) || !win32::IsWindowEnabled(app.lpui->hwndTitle)) return 1;
    if (app.state.game.lid != 0) return 0;
    fPassed = 1;
    puts("Direct setup, predefined/library/computer selection, literal UTF-8 filenames, full roster replacement and isolated race edits passed without a file picker");
    win32::PostQuitMessage(0);
    return 1;
}
