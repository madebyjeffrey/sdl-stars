/* Choose saved designs through the real roster and preserve unrelated versions. */
static HWND HwndRosterDialog(DLGPROC lpfn) {
    HWND hwnd;
    for (hwnd = GetWindow(hwndTitle, GW_HWNDFIRST); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
        if (hwnd->fDialog && hwnd->lpfnDlg == lpfn && IsWindowVisible(hwnd) && IsWindowEnabled(hwnd)) return hwnd;
    return NULL;
}

static BOOL FChooseRosterAction(HWND hwnd, int iSlot, int iMenu) {
    RECT rc;
    UpdateWindow(hwnd);
    iRosterMenu = iMenu;
    SetRect(&rc, xNewGameDiamond, 6 + iSlot * (dyArial8 + 4),
            xNewGameDiamond + dyArial8 + 1, 6 + iSlot * (dyArial8 + 4) + dyArial8 + 1);
    return FPushClientRect(hwnd, &rc, SDL_BUTTON_LEFT);
}

static BOOL FCheckRaceRosterWorkflow(void) {
    static int iStep, iPage, iSlot;
    static uint16_t icksA;
    static const DLGPROC rglpfn[] = {RaceWizardDlg1, RaceWizardDlg4, RaceWizardDlg5, RaceWizardDlg2, RaceWizardDlg3, RaceWizardDlg6};
    static const char *rgszName[] = {"Roster-shared-prefix-A & \xce\xa9.r1", "Roster-shared-prefix-B.r1"};
    HWND hwnd;
    char szManaged[512], szOther[512], szSnapshot[512], szFirstSnapshot[512], szCaption[128];
    const char *pszRoot = PszCompatEnvironment("STARS_SETTINGS_DIR");

    CchSnprintf(szManaged, sizeof(szManaged), "%s%sRace Designs%s%s", pszRoot, szDirSep, szDirSep, rgszName[0]);
    CchSnprintf(szOther, sizeof(szOther), "%s%sRace Designs%s%s", pszRoot, szDirSep, szDirSep, rgszName[1]);
    CchSnprintf(szSnapshot, sizeof(szSnapshot), "%s%sB.snapshot", pszRoot, szDirSep);
    CchSnprintf(szFirstSnapshot, sizeof(szFirstSnapshot), "%s%sA.snapshot", pszRoot, szDirSep);
    if (iStep == 0) {
        if (!IsWindowVisible(hwndTitle)) return TRUE;
        fChecked = TRUE;
        UpdateWindow(hwndTitle);
        fTitlePainted = FCompatPresent(hwndTitle);
        if (!fTitlePainted || !FSaveLibraryRace(hwndTitle, rgszName[0], &vrgplrDef[0]) ||
            !FSaveLibraryRace(hwndTitle, rgszName[1], &vrgplrDef[1])) return FALSE;
        iStep++;
        return FPushClick(rghwndBtnSplash[0]);
    }
    if (iStep == 1) {
        hwnd = HwndRosterDialog(NewGameDlg);
        if (!hwnd) return TRUE;
        GetWindowText(hwnd, szCaption, sizeof(szCaption));
        if (strcmp(szCaption, "New Game - Step 1 of 3")) return FALSE;
        iStep++;
        return FPushClick(GetDlgItem(hwnd, IDC_NEXT));
    }
    if (iStep == 2 || iStep == 3) {
        hwnd = HwndRosterDialog(NewGameDlg2);
        if (!hwnd) return TRUE;
        if (iStep == 3 && (vrgplrTypeNew[0] & 3) != 2) return FALSE;
        pszRosterRace = rgszName[iStep - 2];
        iStep++;
        return FChooseRosterAction(hwnd, iStep == 3 ? 0 : 1, 1);
    }
    if (iStep == 4) {
        hwnd = HwndRosterDialog(NewGameDlg2);
        if (!hwnd) return TRUE;
        if (!FTestCopyFile(szOther, szSnapshot) || FWasRaceFile(szManaged, FALSE) <= 0) return FALSE;
        icksA = IRaceChecksum(&vplr);
        iStep++;
        return FChooseRosterAction(hwnd, 0, 2);
    }
    if (iStep == 5 || iStep == 8) {
        if (iPage < 6) {
            hwnd = HwndRosterDialog(rglpfn[iPage]);
            if (!hwnd) return TRUE;
            if (iPage == 0 && !FPushClick(GetDlgItem(hwnd, IDC_RACE_HUMANOID + 2))) return FALSE;
            return FPushClick(GetDlgItem(hwnd, iPage++ == 5 ? IDC_FINISH : IDC_NEXT));
        }
        hwnd = HwndRosterDialog(NewGameDlg2);
        if (!hwnd) return TRUE;
        if (!FTestFilesEqual(szOther, szSnapshot)) return FALSE;
        if (iStep == 5) {
            if (FWasRaceFile(szManaged, FALSE) <= 0 || IRaceChecksum(&vplr) == icksA ||
                !FTestCopyFile(szManaged, szFirstSnapshot)) return FALSE;
            pszRosterRace = rgszName[0];
            iStep++;
            return FChooseRosterAction(hwnd, 0, 1);
        }
        if (!FTestFilesEqual(szManaged, szFirstSnapshot)) return FALSE;
        iStep++;
        return FChooseRosterAction(hwnd, 1, 56);
    }
    if (iStep == 6) {
        hwnd = HwndRosterDialog(NewGameDlg2);
        if (!hwnd) return TRUE;
        if (strcmp(PszCurrentLibraryRaceName(), rgszName[0]) || (vrgplrTypeNew[0] & 3) != 2) return FALSE;
        iStep++;
        return FChooseRosterAction(hwnd, 0, 16);
    }
    if (iStep == 7) {
        hwnd = HwndRosterDialog(NewGameDlg2);
        if (!hwnd) return TRUE;
        if (vrgplrTypeNew[0] != 1) return FALSE;
        iStep++;
        iPage = 0;
        return FChooseRosterAction(hwnd, 0, 2);
    }
    if (iStep == 9) {
        hwnd = HwndRosterDialog(NewGameDlg2);
        if (!hwnd) return TRUE;
        if (vrgplrTypeNew[1] != 35) return FALSE;
        iStep++;
        return FChooseRosterAction(hwnd, 15, 3);
    }
    if (iStep == 10) {
        hwnd = HwndRosterDialog(NewGameDlg2);
        if (!hwnd) return TRUE;
        if (vrgplrTypeNew[15] != 0) return FALSE;
        iStep++;
    }
    if (iStep == 11) {
        hwnd = HwndRosterDialog(NewGameDlg2);
        if (!hwnd) return TRUE;
        if (iSlot < 20) {
            pszRosterRace = rgszName[iSlot & 1];
            return FChooseRosterAction(hwnd, iSlot++ % 16, 1);
        }
        if (vcplrNew != 16 || (vrgplrTypeNew[3] & 3) != 2 ||
            !FTestFilesEqual(szManaged, szFirstSnapshot) || !FTestFilesEqual(szOther, szSnapshot)) return FALSE;
        iStep++;
        return FPushClick(GetDlgItem(hwnd, IDCANCEL));
    }
    if (!IsWindowVisible(hwndTitle) || !IsWindowEnabled(hwndTitle)) return TRUE;
    if (game.lid != 0) return FALSE;
    fPassed = TRUE;
    puts("Direct setup, predefined/library/computer selection, literal UTF-8 filenames, full roster replacement and isolated race edits passed without a file picker");
    PostQuitMessage(0);
    return TRUE;
}
