/* Import two external races through the actual new-game roster, then edit
 * the first. Shared long prefixes must not redirect its save to the second. */
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
    static int iStep, iPage;
    static uint16_t icksA;
    static const DLGPROC rglpfn[] = {RaceWizardDlg1, RaceWizardDlg4, RaceWizardDlg5, RaceWizardDlg2, RaceWizardDlg3, RaceWizardDlg6};
    static const char *rgszName[] = {"Roster-shared-prefix-A.r1", "Roster-shared-prefix-B.r1"};
    HWND hwnd;
    size_t ich;
    char szSource[512], szManaged[512], szOther[512], szSnapshot[512];
    const char *pszRoot = getenv("STARS_SETTINGS_DIR");
    static char szChosen[512];

    CchSnprintf(szManaged, sizeof(szManaged), "%s%sRace Designs%s%s", pszRoot, szDirSep, szDirSep, rgszName[0]);
    CchSnprintf(szOther, sizeof(szOther), "%s%sRace Designs%s%s", pszRoot, szDirSep, szDirSep, rgszName[1]);
    CchSnprintf(szSnapshot, sizeof(szSnapshot), "%s%sB.snapshot", pszRoot, szDirSep);
    if (iStep == 0) {
        if (!IsWindowVisible(hwndTitle)) return TRUE;
        fChecked = TRUE;
        UpdateWindow(hwndTitle);
        fTitlePainted = FCompatPresent(hwndTitle);
        if (!fTitlePainted) return FALSE;
        if (!FSaveLibraryRace(hwndTitle, "Seed.r1", &vrgplrDef[0])) return FALSE;
        CchSnprintf(szSource, sizeof(szSource), "%s%sRace Designs%sSeed.r1", pszRoot, szDirSep, szDirSep);
        CchSnprintf(szChosen, sizeof(szChosen), "%s%s%s", pszRoot, szDirSep, rgszName[0]);
        if (!FTestCopyFile(szSource, szChosen) || !FSaveLibraryRace(hwndTitle, "Seed.r1", &vrgplrDef[1])) return FALSE;
        CchSnprintf(szChosen, sizeof(szChosen), "%s%s%s", pszRoot, szDirSep, rgszName[1]);
        if (!FTestCopyFile(szSource, szChosen)) return FALSE;
        iStep++;
        return FPushClick(rghwndBtnSplash[0]);
    }
    if (iStep == 1) {
        hwnd = HwndRosterDialog(SimpleNewGameDlg);
        if (!hwnd) return TRUE;
        iStep++;
        return FPushClick(GetDlgItem(hwnd, IDC_SIMPLE_NEW_GAME_ADVANCED));
    }
    if (iStep == 2) {
        hwnd = HwndRosterDialog(NewGameDlg);
        if (!hwnd) return TRUE;
        iStep++;
        return FPushClick(GetDlgItem(hwnd, IDC_NEXT));
    }
    if (iStep == 3 || iStep == 5) {
        hwnd = HwndRosterDialog(NewGameDlg2);
        if (!hwnd) return TRUE;
        CchSnprintf(szChosen, sizeof(szChosen), "%s%s%s", pszRoot, szDirSep, rgszName[iStep == 3 ? 0 : 1]);
        pszRosterSource = szChosen;
        iStep++;
        return FChooseRosterAction(hwnd, iStep == 4 ? 0 : 1, 1);
    }
    if (iStep == 4 || iStep == 6) {
        hwnd = HwndRosterDialog(LibraryNameDlg);
        if (!hwnd) return TRUE;
        iStep++;
        return FPushClick(GetDlgItem(hwnd, IDOK));
    }
    if (iStep == 7) {
        hwnd = HwndRosterDialog(NewGameDlg2);
        if (!hwnd) return TRUE;
        if (!FTestCopyFile(szOther, szSnapshot) || FWasRaceFile(szManaged, FALSE) <= 0) return FALSE;
        icksA = IRaceChecksum(&vplr);
        iStep++;
        return FChooseRosterAction(hwnd, 0, 2);
    }
    if (iStep == 8 || iStep == 12) {
        if (iPage < 6) {
            hwnd = HwndRosterDialog(rglpfn[iPage]);
            if (!hwnd) return TRUE;
            if (iPage == 0 && !FPushClick(GetDlgItem(hwnd, IDC_RACE_HUMANOID + 2))) return FALSE;
            return FPushClick(GetDlgItem(hwnd, iPage++ == 5 ? IDC_FINISH : IDC_NEXT));
        }
        hwnd = HwndRosterDialog(NewGameDlg2);
        if (!hwnd) return TRUE;
        if (!FTestFilesEqual(szOther, szSnapshot)) return FALSE;
        if (iStep == 8) {
            if (FWasRaceFile(szManaged, FALSE) <= 0 || IRaceChecksum(&vplr) == icksA) return FALSE;
            CchSnprintf(szSource, sizeof(szSource), "%s%sA.snapshot", pszRoot, szDirSep);
            if (!FTestCopyFile(szManaged, szSource)) return FALSE;
            /* Reopen the managed design using the picker's forward slashes. */
            CchSnprintf(szChosen, sizeof(szChosen), "%s", szManaged);
            for (ich = 0; szChosen[ich]; ich++) if (szChosen[ich] == '\\') szChosen[ich] = '/';
            pszRosterSource = szChosen;
            iStep++;
            return FChooseRosterAction(hwnd, 0, 1);
        }
        CchSnprintf(szSource, sizeof(szSource), "%s%sA.snapshot", pszRoot, szDirSep);
        if (!FTestFilesEqual(szManaged, szSource)) return FALSE;
        fPassed = TRUE;
        puts("New-game roster edits preserve the selected race destination and unrelated designs");
        PostQuitMessage(0);
        return TRUE;
    }
    if (iStep == 9) {
        hwnd = HwndRosterDialog(NewGameDlg2);
        if (!hwnd) return TRUE;
        if (strcmp(PszCurrentLibraryRaceName(), rgszName[0]) || (vrgplrTypeNew[0] & 3) != 2) return FALSE;
        iStep++;
        return FChooseRosterAction(hwnd, 0, 16);
    }
    if (iStep == 10) {
        hwnd = HwndRosterDialog(NewGameDlg2);
        if (!hwnd) return TRUE;
        iStep = 12;
        iPage = 0;
        return FChooseRosterAction(hwnd, 0, 2);
    }
    return TRUE;
}
