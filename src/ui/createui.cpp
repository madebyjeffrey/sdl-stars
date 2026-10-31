#include "win.h"
#include "text_io.h"
#include "control_io.h"
#include "alert_io.h"
#include "random_io.h"

/* UI-only names retain full UTF-8 library filenames beside the Win16 cache. */
static char rgszLibraryRaceNew[16][256];

typedef struct {
    HMENU hmenu;
    int cRace;
    const char *pszSelected;
} NEWGAMERACEMENU;

static int16_t AddNewGameLibraryRace(void *lpData, const char *pszName) {
    NEWGAMERACEMENU *pnrm = (NEWGAMERACEMENU *)lpData;
    char szLabel[512];
    const char *pch;
    char *pchLabel = szLabel;
    UINT grf;

    if (pnrm->cRace >= INT16_MAX - 1000) return 0;
    // Menu mnemonics must not consume literal ampersands in library filenames.
    for (pch = pszName; *pch; pch++) {
        *pchLabel++ = *pch;
        if (*pch == '&') *pchLabel++ = '&';
    }
    *pchLabel = 0;
    grf = pnrm->pszSelected && !strcmp(pszName, pnrm->pszSelected) ? win32::MF_CHECKED : 0;
    if (!win32::AppendMenuA(pnrm->hmenu, grf, 16000 + pnrm->cRace, szLabel)) return 0;
    pnrm->cRace++;
    return 1;
}

void NewGameWizard(HWND hwnd, int16_t fReadOnly) {
    int16_t iStepMaxSoFar;
    int16_t mdRet;
    FARPROC lpProc;
    int16_t fIdleSav;
    int16_t rgplrbmp[16];
    int16_t i;
    int16_t c;
    char    szFile[256];
    AiRace  idAi;
    char    szFileLocal[208];
    int16_t j;
    RECT    rgrcStack[20];
    PLAYER  rgplrLocal[16];
    int16_t lvlAi;
    GAME    gameT;

    iStepMaxSoFar = 0;
    TextIO().pfnCchGetString(idsGameXy, szFile);
    vrgrcRCW = rgrcStack;
    iPanelActive = rwPageNone;
    fRCWReadOnly = fReadOnly;
    fIdleSav = gd.fNoIdleChecks;
    gd.fNoIdleChecks = 1;
    vrgplrNew = rgplrLocal;
    vrgszFileNew = szFileLocal;
    vcplrNew = 0;
    memset(rgszLibraryRaceNew, 0, sizeof(rgszLibraryRaceNew));
    memset(vrgplrTypeNew, 0, 16);
    if (!fReadOnly) {
        gameT = game;
        memset(&game, 0, sizeof(GAME));
        game.cPlanMax = gameT.cPlanMax;
        game.cPlayer = gameT.cPlayer;
        lvlAi = lvlAiStandard;
        game.mdSize = sizeSmall;
        game.mdDensity = densityNormal;
        game.mdStartDist = startDistModerate;
        vrgplrTypeNew[0] = 1;
        game.rgvc[0] = 136;
        game.rgvc[1] = 142;
        game.rgvc[2] = 130;
        game.rgvc[3] = 10;
        game.rgvc[4] = 136;
        game.rgvc[5] = 9;
        game.rgvc[6] = 9;
        game.rgvc[7] = 7;
        game.rgvc[8] = 1;
        TextIO().pfnCchGetString(static_cast<StringId>(idsShootingFishBarrel + 5 * lvlAi + game.mdSize), game.szName);
        goto Step1;
    }
    lvlAi = lvlAiStandard;
    for (i = 0; i < game.cPlayer; i++) {
        if (rgplr[i].fInclude) {
            vrgplrTypeNew[i] = i << 2 | 2;
            vrgplrNew[i] = rgplr[i];
            vrgszFileNew[i * 13] = 0;
        } else {
            vrgplrTypeNew[i] = 25;
        }
    }
Step1:
    lpProc = (NewGameDlg);
    mdRet = ControlIO().pfnDialogBox(hInst, win32::MAKEINTRESOURCE(IDD_NEW_GAME_1), hwnd, lpProc, (LPARAM)szFile);
    if (mdRet <= 0) {
    Cancel:
        if (!fRCWReadOnly) {
            game = gameT;
        }
        gd.fNoIdleChecks = fIdleSav;
        return;
    }
    InitNewGamePlr(iStepMaxSoFar, static_cast<AiLevel>(lvlAi));
    if (mdRet == 3)
        goto Finish;
Step2:
    lpProc = (NewGameDlg2);
    mdRet = ControlIO().pfnDialogBox(hInst, win32::MAKEINTRESOURCE(IDD_NEW_GAME_2), hwnd, lpProc, (LPARAM)szFile);
    if (mdRet <= 0)
        goto Cancel;
    if (iStepMaxSoFar < 1) {
        iStepMaxSoFar = 1;
    }
    InitNewGamePlr(iStepMaxSoFar, static_cast<AiLevel>(lvlAi));
    if (mdRet == 1)
        goto Step1;
    if (mdRet == 3)
        goto Finish;
    lpProc = (NewGameDlg3);
    mdRet = ControlIO().pfnDialogBox(hInst, win32::MAKEINTRESOURCE(IDD_NEW_GAME_3), hwnd, lpProc, (LPARAM)szFile);
    if (mdRet <= 0)
        goto Cancel;
    if (iStepMaxSoFar < 2) {
        iStepMaxSoFar = 2;
    }
    if (mdRet == 1)
        goto Step2;
    if (mdRet == 3)
        goto Finish;
Finish:
    if (fRCWReadOnly)
        goto Cancel;
    gameT = game;
    gameT.turn = 0;
    DestroyCurGame();
    game = gameT;
    strcpy(szBase, szFile);
    szBase[strlen(szBase) - 3] = 0;
    for (i = 0; i < 16 && vrgplrTypeNew[i] != 0; i++) {
        switch (vrgplrTypeNew[i] & 3) {
        case 1:
            if (vrgplrTypeNew[i] >> 2 > 6) {
                c = RandomIO().pfnRandom(7);
                rgplr[i] = vrgplrDef[c];
                rgplr[i].fAi = 1;
                rgplr[i].lvlAi = 0;
                rgplr[i].idAi = idAiMaid;
                rgplr[i].lSalt = -1;
            } else {
                c = vrgplrTypeNew[i] >> 2;
                rgplr[i] = vrgplrDef[c];
            }
            TextIO().pfnCchGetString(static_cast<StringId>(idsHumanoid + c), rgplr[i].szName);
            win32::wsprintfA(rgplr[i].szNames, "%ss", rgplr[i].szName);
            break;
        case 2:
            rgplr[i] = rgplrLocal[vrgplrTypeNew[i] >> 2];
            break;
        case 3:
            idAi = static_cast<AiRace>(vrgplrTypeNew[i] >> 2 & 7);
            lvlAi = vrgplrTypeNew[i] >> 5;
            if (lvlAi >= 4) {
                lvlAi = RandomIO().pfnRandom(4);
            }
            if ((int16_t)idAi >= idAiRandom) {
                idAi = static_cast<AiRace>(RandomIO().pfnRandom(6));
            }
            rgplr[i] = *LpplrComp(idAi, static_cast<AiLevel>(lvlAi));
            rgplr[i].fAi = 1;
            rgplr[i].lvlAi = lvlAi;
            rgplr[i].idAi = idAi;
        }
    }
    game.cPlayer = i;
    for (i = 0; i < game.cPlayer; i++) {
        if (!rgplr[i].fAi && CAdvantagePoints(&rgplr[i]) < 0) {
            rgplr[i] = vrgplrDef[0];
            rgplr[i].fHacker = 1;
        }
        if (rgplr[i].szName[0] == 0) {
            TextIO().pfnCchGetString(static_cast<StringId>(idsBerserker + RandomIO().pfnRandom(24)), rgplr[i].szName);
        }
        if (rgplr[i].szNames[0] == 0) {
            win32::wsprintfA(rgplr[i].szNames, "%ss", rgplr[i].szName);
        }
    }
    for (i = 1; i < game.cPlayer; i++) {
        for (j = 0; j < i && strcmp(rgplr[i].szName, rgplr[j].szName) != 0; j++) {
        }
        if (j < i) {
            c = RandomIO().pfnRandom(24);
            while (1) {
                for (j = 0; j < game.cPlayer && strcmp(rgplr[j].szName, TextIO().pfnCompressed(static_cast<StringId>(idsBerserker + c))) != 0; j++) {
                }
                if (j == game.cPlayer)
                    break;
                c++;
                if (c >= 24) {
                    c = 0;
                }
            }
            TextIO().pfnCchGetString(static_cast<StringId>(idsBerserker + c), rgplr[i].szName);
            strcpy(rgplr[i].szNames, rgplr[i].szName);
            strcat(rgplr[i].szNames, "s");
        }
    }
    for (i = 0; i < game.cPlayer; i++) {
        rgplrbmp[i] = rgplr[i].iPlrBmp;
        if (rgplrbmp[i] < 0 || rgplrbmp[i] >= 32 || rgplr[i].fAi) {
            rgplrbmp[i] = -1;
        }
    }
    for (i = 1; i < game.cPlayer; i++) {
        if (rgplrbmp[i] != -1) {
            for (j = 0; j < i && rgplrbmp[j] != rgplrbmp[i]; j++) {
            }
            if (j < i) {
                rgplrbmp[RandomIO().pfnRandom(2) == 0 ? j : i] = -1;
            }
        }
    }
    for (i = 0; i < game.cPlayer; i++) {
        if (rgplrbmp[i] == -1) {
            rgplrbmp[i] = RandomIO().pfnRandom(32);
            while (1) {
                for (j = 0; j < game.cPlayer && (j == i || rgplrbmp[i] != rgplrbmp[j]); j++) {
                }
                if (j == game.cPlayer)
                    break;
                rgplrbmp[i]++;
                if (rgplrbmp[i] >= 32) {
                    rgplrbmp[i] = 0;
                }
            }
        }
    }
    for (i = 0; i < game.cPlayer; i++) {
        rgplr[i].iPlrBmp = rgplrbmp[i];
    }
    if (!fFreeingTitle) {
        fFreeingTitle = 1;
        ControlIO().pfnDestroyWindow(hwndTitle);
        hwndTitle = 0;
        ui::ShowWindow(hwndFrame, win32::SW_SHOW);
    }
    if (!FFinishLibraryGame(hwnd, GenerateWorld(0))) {
        DestroyCurGame();
        ShowTitleScreen();
        gd.fNoIdleChecks = fIdleSav;
        return;
    }
    if (idPlayer == iplrNone) {
        ControlIO().pfnBringUpHostDlg();
    }
    gd.fNoIdleChecks = fIdleSav;
    return;
}

int16_t FGetNewGameName(HWND hwnd, char *szFileSuggest) {
    return FChooseLibraryGameName(hwnd, szFileSuggest, game.szName);
}

static int16_t FChooseNewGameName(HWND hwnd) {
    char *pszFile = (char *)win32::GetWindowLongPtrA(hwnd, win32::DWLP_USER);

    // Reserve the library entry before closing the wizard so failures keep it open.
    return fRCWReadOnly || (pszFile && FGetNewGameName(hwnd, pszFile));
}

INT_PTR  NewGameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int16_t     i;
    RECT        rc;
    HDC         hdc;
    RECT        rcGBox;
    int16_t     c;
    PAINTSTRUCT ps;
    int16_t     iRet;

    switch (message) {
    case win32::WM_INITDIALOG:
        win32::SetWindowLongPtrA(hwnd, win32::DWLP_USER, lParam);
        SetNGWTitle(hwnd, 1);
        win32::CheckRadioButton(hwnd, IDC_NEW_GAME_TINY, IDC_NEW_GAME_HUGE, game.mdSize + 1000);
        win32::CheckRadioButton(hwnd, IDC_NEW_GAME_SPARSE, IDC_NEW_GAME_PACKED, game.mdDensity + 1005);
        win32::CheckRadioButton(hwnd, IDC_NEW_GAME_CLOSE, IDC_NEW_GAME_DISTANT, game.mdStartDist + 1009);
        ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_NAME), game.szName);
        ui::SendDlgItemMessage(hwnd, IDC_NEW_GAME_NAME, win32::EM_LIMITTEXT, 0x1f, 0);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_MAX_MINERALS), win32::BM_SETCHECK, game.fExtraFuel, 0);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_SLOWER_TECH), win32::BM_SETCHECK, game.fSlowTech, 0);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_ACCELERATED_BBS), win32::BM_SETCHECK, game.fBBSPlay, 0);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_NO_RANDOM_EVENTS), win32::BM_SETCHECK, game.fNoRandom, 0);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_AI_ALLIANCES), win32::BM_SETCHECK, game.fAisBand, 0);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_PUBLIC_SCORES), win32::BM_SETCHECK, game.fVisScores, 0);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_GALAXY_CLUMPING), win32::BM_SETCHECK, game.fClumping, 0);
        if (fRCWReadOnly) {
            for (i = 1000; i <= 1004; i++) {
                ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, i), 0);
            }
            for (i = 1005; i <= 1008; i++) {
                ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, i), 0);
            }
            for (i = 1009; i <= 1012; i++) {
                ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, i), 0);
            }
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_MAX_MINERALS), 0);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_SLOWER_TECH), 0);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_ACCELERATED_BBS), 0);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_NO_RANDOM_EVENTS), 0);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_GALAXY_CLUMPING), 0);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_AI_ALLIANCES), 0);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_NAME), 0);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_PUBLIC_SCORES), 0);
        }
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyNewDlg, 1);
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        for (i = 1000; i <= 1021 && (HWND)lParam != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 1021 || message == win32::WM_CTLCOLORSTATIC || (HWND)lParam == ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_GALAXY_CLUMPING)) {
            win32::SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_TINY), &rcGBox);
        win32::ScreenToClient(hwnd, (POINT *)&rcGBox);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_HUGE), &rc);
        win32::ScreenToClient(hwnd, (POINT *)&rc.right);
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        _Draw3dFrame(hdc, &rcGBox, -1);
        win32::SelectObject(hdc, rghfontArial8[1]);
        win32::SetBkColor(hdc, crButtonFace);
        c = TextIO().pfnCchGetString(idsUniverseSize, szWork);
        win32::TextOutA(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, c);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_SPARSE), &rcGBox);
        win32::ScreenToClient(hwnd, (POINT *)&rcGBox);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_PACKED), &rc);
        win32::ScreenToClient(hwnd, (POINT *)&rc.right);
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        _Draw3dFrame(hdc, &rcGBox, -1);
        win32::SelectObject(hdc, rghfontArial8[1]);
        win32::SetBkColor(hdc, crButtonFace);
        c = TextIO().pfnCchGetString(idsDensity, szWork);
        win32::TextOutA(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, c);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_CLOSE), &rcGBox);
        win32::ScreenToClient(hwnd, (POINT *)&rcGBox);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_DISTANT), &rc);
        win32::ScreenToClient(hwnd, (POINT *)&rc.right);
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        _Draw3dFrame(hdc, &rcGBox, -1);
        win32::SelectObject(hdc, rghfontArial8[1]);
        win32::SetBkColor(hdc, crButtonFace);
        c = TextIO().pfnCchGetString(idsPlayerPositions, szWork);
        win32::TextOutA(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, c);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            win32::WinHelpA(hwnd, szHelpFile, win32::HELP_CONTEXT, idhStep1SpecifyingTheUniverse);
            return 1;
        }
        for (iRet = 0; iRet < 4 && LOWORD(wParam) != rgidRaceBtn[iRet]; iRet++) {
        }
        if (iRet < 4) {
            if (iRet != 0) {
                for (i = 1000; i <= 1004 && win32::IsDlgButtonChecked(hwnd, i) == 0; i++) {
                }
                game.mdSize = static_cast<UniverseSize>(i - 1000);
                for (i = 1005; i <= 1008 && win32::IsDlgButtonChecked(hwnd, i) == 0; i++) {
                }
                game.mdDensity = static_cast<UniverseDensity>(i - 1005);
                for (i = 1009; i <= 1012 && win32::IsDlgButtonChecked(hwnd, i) == 0; i++) {
                }
                game.mdStartDist = static_cast<StartDistance>(i - 1009);
                game.fExtraFuel = win32::IsDlgButtonChecked(hwnd, IDC_NEW_GAME_MAX_MINERALS);
                game.fSlowTech = win32::IsDlgButtonChecked(hwnd, IDC_NEW_GAME_SLOWER_TECH);
                game.fBBSPlay = win32::IsDlgButtonChecked(hwnd, IDC_NEW_GAME_ACCELERATED_BBS);
                game.fNoRandom = win32::IsDlgButtonChecked(hwnd, IDC_NEW_GAME_NO_RANDOM_EVENTS);
                game.fAisBand = win32::IsDlgButtonChecked(hwnd, IDC_NEW_GAME_AI_ALLIANCES);
                game.fVisScores = win32::IsDlgButtonChecked(hwnd, IDC_NEW_GAME_PUBLIC_SCORES);
                game.fClumping = win32::IsDlgButtonChecked(hwnd, IDC_NEW_GAME_GALAXY_CLUMPING);
                i = ControlIO().pfnGetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_NAME), game.szName, 32);
                if (i == 0) {
                    strcpy(game.szName, szBase);
                }
            }
            if (iRet == 3 && !FChooseNewGameName(hwnd)) return 1;
            ControlIO().pfnStickyDlgPos(hwnd, &ptStickyNewDlg, 0);
            ControlIO().pfnEndDialog(hwnd, iRet);
            return 1;
        }
        break;
    }
    return 0;
}

INT_PTR  NewGameDlg2(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int16_t     i;
    RECT        rc;
    RECT        rcT;
    int16_t     dyBut;
    int16_t     dy;
    int16_t     dyCur;
    HWND        hwndBtn;
    POINT16     pt;
    int16_t     iDiamond;
    int16_t     iNewVal;
    int16_t     j;
    char       *psz;
    int16_t     tpm;
    int16_t     iChecked;
    HMENU       rghmenuSubPopup[14];
    HMENU       hmenuPopup;
    win32::MSG         msg;
    int16_t     iCurVal;
    RECT       *prcSav;
    HDC         hdc;
    PAINTSTRUCT ps;
    char        szLibraryRace[256];
    char        szLibraryLabel[512];
    char       *pch, *pchName;
    NEWGAMERACEMENU nrm;

    switch (message) {
    case win32::WM_INITDIALOG:
        win32::SetWindowLongPtrA(hwnd, win32::DWLP_USER, lParam);
        SetNGWTitle(hwnd, 2);
        dy = (dyArial8 + 4) * 16 + 8;
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, rgidRaceBtn[0]), &rc);
        dyBut = rc.bottom - rc.top;
        win32::GetWindowRect(hwnd, &rcT);
        dyCur = rcT.bottom - rcT.top;
        if (dyCur < dy + dyBut + 6) {
            for (i = 0; i < 5; i++) {
                hwndBtn = ControlIO().pfnGetDlgItem(hwnd, rgidRaceBtn[i]);
                win32::GetWindowRect(hwndBtn, &rc);
                win32::MapWindowPoints(NULL, hwnd, (POINT *)&rc, 2);
                win32::OffsetRect(&rc, 0, dy - rc.top);
                ControlIO().pfnSetWindowPos(hwndBtn, NULL, rc.left, rc.top, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER);
            }
            dy = rc.bottom + 6;
            ControlIO().pfnGetClientRect(hwnd, &rc);
            if (dyCur < dy) {
                ControlIO().pfnSetWindowPos(hwnd, NULL, 0, 0, rcT.right - rcT.left, dy + rcT.bottom - rcT.top - rc.bottom, win32::SWP_NOMOVE | win32::SWP_NOZORDER);
            }
        }
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyNewDlg, 1);
        return 1;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        for (i = 401; i <= 448 && (HWND)lParam != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 448 || message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case win32::WM_SETCURSOR:
        if (fRCWReadOnly)
            break;
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (pt.x < xNewGameDiamond || pt.x >= xNewGameDiamond + dyArial8 + 1 || pt.y < 6)
            break;
        iDiamond = (int16_t)(pt.y - 6) / (dyArial8 + 4);
        if (iDiamond >= 16 || (int16_t)(pt.y - 6) % (dyArial8 + 4) >= dyArial8 + 1)
            break;
        win32::SetCursor(hcurHand);
        return 1;
    case win32::WM_LBUTTONDOWN:
    case win32::WM_RBUTTONDOWN:
        if (fRCWReadOnly)
            break;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        if (pt.x < xNewGameDiamond || pt.x >= xNewGameDiamond + dyArial8 + 1 || pt.y < 6)
            break;
        iDiamond = (int16_t)(pt.y - 6) / (dyArial8 + 4);
        if (iDiamond >= 16 || (int16_t)(pt.y - 6) % (dyArial8 + 4) >= dyArial8 + 1)
            break;
        iCurVal = vrgplrTypeNew[iDiamond];
        ClientToScreen16(hwnd, &pt);
        rghmenuSubPopup[0] = win32::CreatePopupMenu();
        for (i = 0; i < 6; i++) {
            iChecked = iCurVal == i * 4 + 1 ? 8 : 0;
            psz = TextIO().pfnCompressed(static_cast<StringId>(idsHumanoid + i));
            win32::AppendMenuA(rghmenuSubPopup[0], iChecked, i + 15016, psz);
        }
        win32::AppendMenuA(rghmenuSubPopup[0], iCurVal == 25 ? win32::MF_CHECKED : 0, 15022, TextIO().pfnCompressed(idsRandom));
        win32::AppendMenuA(rghmenuSubPopup[0], iCurVal == 29 ? win32::MF_CHECKED : 0, 15023, TextIO().pfnCompressed(idsExpansionPlayer));
        rghmenuSubPopup[1] = win32::CreatePopupMenu();
        nrm.hmenu = rghmenuSubPopup[1];
        nrm.cRace = 0;
        nrm.pszSelected = (iCurVal & 3) == 2 ? rgszLibraryRaceNew[iCurVal >> 2] : NULL;
        FEnumerateLibraryRaces(AddNewGameLibraryRace, &nrm);
        if (nrm.cRace == 0)
            win32::AppendMenuA(nrm.hmenu, win32::MF_GRAYED, 0, TextIO().pfnCompressed(idsLibraryNoRaces));
        rghmenuSubPopup[2] = win32::CreatePopupMenu();
        for (i = 0; i <= 6; i++) {
            rghmenuSubPopup[i + 5] = win32::CreatePopupMenu();
            for (j = 0; j < 5; j++) {
                iChecked = iCurVal == j * 32 + i * 4 + 3 ? 8 : 0;
                win32::AppendMenuA(rghmenuSubPopup[i + 5], iChecked, i + 15048 + j * 8, vrgszComputerLevel[j]);
            }
            iChecked = (iCurVal & 0x1f) == i * 4 + 3 ? 8 : 0;
            win32::AppendMenuA(rghmenuSubPopup[2], 0x10 | iChecked, (UINT_PTR)rghmenuSubPopup[i + 5], vrgszComputerPlayers[i]);
        }
        hmenuPopup = win32::CreatePopupMenu();
        iPopMenuSel = -1;
        iChecked = (iCurVal & 3) == 1 ? 8 : 0;
        win32::AppendMenuA(hmenuPopup, 0x10 | iChecked, (UINT_PTR)rghmenuSubPopup[0], TextIO().pfnCompressed(idsPredefinedRace));
        iChecked = (iCurVal & 3) == 2 ? 8 : 0;
        win32::AppendMenuA(hmenuPopup, 0x10 | iChecked, (UINT_PTR)rghmenuSubPopup[1], TextIO().pfnCompressed(idsLibraryRaceMenu));
        iChecked = (iCurVal & 3) == 3 ? 8 : 0;
        win32::AppendMenuA(hmenuPopup, 0x10 | iChecked, (UINT_PTR)rghmenuSubPopup[2], TextIO().pfnCompressed(idsComputerPlayer));
        win32::AppendMenuA(hmenuPopup, win32::MF_SEPARATOR, 0, NULL);
        win32::AppendMenuA(hmenuPopup, win32::MF_BYCOMMAND, 15000, TextIO().pfnCompressed(idsLibraryNewRace));
        if (((iCurVal & 3) == 1 && (iCurVal >> 2) < 7) || (iCurVal & 3) == 2)
            win32::AppendMenuA(hmenuPopup, win32::MF_BYCOMMAND, 15002, TextIO().pfnCompressed(idsEditRace));
        iChecked = !(iCurVal & 3) ? 8 : 0;
        win32::AppendMenuA(hmenuPopup, iChecked, 0x3a9b, TextIO().pfnCompressed(idsPlayer));
        tpm = message == win32::WM_LBUTTONDOWN ? win32::TPM_LEFTBUTTON : win32::TPM_RIGHTBUTTON;
        ui::TrackPopupMenu(hmenuPopup, win32::TPM_CENTERALIGN | tpm, pt.x, pt.y, 0, hwnd, NULL);
        if (ui::PeekMessage(&msg, hwnd, 273, 273, 2) && msg.wParam >= 15000 && msg.wParam < 15000 + INT16_MAX) {
            iPopMenuSel = msg.wParam - 15000;
        }
        szLibraryRace[0] = 0;
        if (iPopMenuSel >= 1000 && iPopMenuSel < 1000 + nrm.cRace) {
            win32::GetMenuStringA(nrm.hmenu, 15000 + iPopMenuSel, szLibraryLabel, sizeof(szLibraryLabel), win32::MF_BYCOMMAND);
            for (pch = szLibraryLabel, pchName = szLibraryRace; *pch; pch++) {
                *pchName++ = *pch;
                if (*pch == '&' && pch[1] == '&') pch++;
            }
            *pchName = 0;
        }
        win32::DestroyMenu(hmenuPopup);
        iNewVal = -1;
        if (iPopMenuSel != -1) {
            if (iPopMenuSel == 0) {
                vplr = vrgplrDef[0];
                TextIO().pfnCchGetString(idsHumanoid, vplr.szName);
                win32::wsprintfA(vplr.szNames, "%ss", vplr.szName);
                szLibraryRace[0] = 0;
                szRacePass[0] = 0;
                prcSav = vrgrcRCW;
                if (FCustomizeLibraryRace(hwnd, szLibraryRace)) {
                    vrgrcRCW = prcSav;
                    goto PlaceNew;
                }
                vrgrcRCW = prcSav;
            }
            if (iPopMenuSel >= 1000 && iPopMenuSel < 1000 + nrm.cRace) {
                if (FLoadLibraryRace(hwnd, szLibraryRace)) {
                PlaceNew:
                    if (vcplrNew < 16) {
                        iNewVal = vcplrNew++;
                    } else {
                        for (iNewVal = 15; iNewVal >= 0; iNewVal--) {
                            for (i = 0; i < 16 && (i == iDiamond || (vrgplrTypeNew[i] & 3) != 2 || (vrgplrTypeNew[i] >> 2 & 0xf) != iNewVal); i++) {
                            }
                            if (i == 16)
                                break;
                        }
                    }
                    if (iNewVal < 0) goto FinishClick;
                    vrgplrNew[iNewVal] = vplr;
                    CchCopyUtf8(vrgszFileNew + iNewVal * 13, 13, szRaceFile, strlen(szRaceFile));
                    strcpy(rgszLibraryRaceNew[iNewVal], szLibraryRace);
                    iNewVal = iNewVal << 2 | 2;
                }
            } else if (iPopMenuSel == 2) {
                if ((iCurVal & 3) == 1) {
                    vplr = vrgplrDef[iCurVal >> 2];
                    TextIO().pfnCchGetString(static_cast<StringId>((iCurVal >> 2) + 0x567), vplr.szName);
                    win32::wsprintfA(vplr.szNames, "%ss", vplr.szName);
                } else {
                    vplr = vrgplrNew[iCurVal >> 2];
                    strcpy(szRaceFile, vrgszFileNew + (iCurVal >> 2) * 13);
                }
                lSaltCur = vplr.lSalt;
                lSaltLast = 0;
                if (!FCheckPassword())
                    goto FinishClick;
                if (vplr.lSalt != 0) {
                    strcpy(szRacePass, szPassLast);
                } else {
                    szRacePass[0] = 0;
                }
                CchSnprintf(szLibraryRace, sizeof(szLibraryRace), "%s",
                              (iCurVal & 3) == 2 ? rgszLibraryRaceNew[iCurVal >> 2] : "");
                prcSav = vrgrcRCW;
                if (FCustomizeLibraryRace(hwnd, szLibraryRace)) {
                    vrgrcRCW = prcSav;
                    if ((iCurVal & 3) == 1 || strcmp(szLibraryRace, rgszLibraryRaceNew[iCurVal >> 2]) != 0)
                        goto PlaceNew;
                    vrgplrNew[iCurVal >> 2] = vplr;
                    CchCopyUtf8(vrgszFileNew + (iCurVal >> 2) * 13, 13, szRaceFile, strlen(szRaceFile));
                    strcpy(rgszLibraryRaceNew[iCurVal >> 2], szLibraryRace);
                    iNewVal = iCurVal;
                    iCurVal = -1;
                }
                vrgrcRCW = prcSav;
            } else if (iPopMenuSel == 3) {
                iNewVal = 0;
            } else if (iPopMenuSel >= 16 && iPopMenuSel < 32) {
                iNewVal = (iPopMenuSel - 0x10) << 2 | 1;
            } else if (iPopMenuSel >= 48 && iPopMenuSel < 88) {
                iNewVal = (iPopMenuSel - 0x30) << 2 | 3;
            }
        }
        if (iNewVal != -1 && iNewVal != iCurVal) {
            vrgplrTypeNew[iDiamond] = iNewVal;
            if (iNewVal == 0) {
                while (++iDiamond < 16) {
                    vrgplrTypeNew[iDiamond] = 0;
                }
            } else if (iDiamond > 0) {
                while (--iDiamond >= 0 && vrgplrTypeNew[iDiamond] == 0) {
                    vrgplrTypeNew[iDiamond] = iNewVal;
                }
            }
            ControlIO().pfnInvalidateRect(hwnd, NULL, 1);
        }
    FinishClick:
        ControlIO().pfnSetFocus(hwnd);
        break;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        DrawNewGame2(hwnd, hdc, -1);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            win32::WinHelpA(hwnd, szHelpFile, win32::HELP_CONTEXT, idhStep2SpecifyingThePlayers);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            if (i != 0 && vrgplrTypeNew[0] == 0 && !fRCWReadOnly) {
                AlertIO().pfnAlertSz(TextIO().pfnFormat(idsMustHaveLeastOnePlayerGame, NULL), win32::MB_ICONHAND);
            } else {
                if (i == 3 && !FChooseNewGameName(hwnd)) return 1;
                ControlIO().pfnStickyDlgPos(hwnd, &ptStickyNewDlg, 0);
                ControlIO().pfnEndDialog(hwnd, i);
                return 1;
            }
        }
    }
    return 0;
}

void DrawNewGame2(HWND hwnd, HDC hdc, int16_t iDraw) {
    int16_t  fCreatedDC;
    int16_t  yCur;
    int16_t  i;
    int16_t  iPlr;
    int16_t  bkMode;
    int16_t  cch;
    RECT     rcDiamond;
    RECT     rc;
    StringId ids;
    char     szT[20];
    const char *pszFile;

    fCreatedDC = 0;
    if (!hdc) {
        fCreatedDC = 1;
        hdc = win32::GetDC(hwnd);
    }
    ControlIO().pfnGetClientRect(hwnd, &rc);
    bkMode = win32::SetBkMode(hdc, win32::TRANSPARENT);
    win32::SelectObject(hdc, rghfontArial8[1]);
    xNewGameDiamond = LOWORD(GetTextExtent(hdc, TextIO().pfnCompressed(idsPlayer16), 11)) + 12;
    yCur = 6;
    win32::SetRect(&rcDiamond, xNewGameDiamond, yCur, xNewGameDiamond + dyArial8 + 1, yCur + dyArial8 + 1);
    for (i = 0; i < 16 && (!fRCWReadOnly || i < game.cPlayer); i++) {
        cch = win32::wsprintfA(szWork, TextIO().pfnCompressed(idsPlayerD), i + 1);
        szWork[cch++] = ':';
        szWork[cch] = 0;
        RightTextOut(hdc, xNewGameDiamond - 6, yCur, szWork, cch, 0);
        DrawDiamond(hdc, &rcDiamond, hbrBBlue);
        iPlr = vrgplrTypeNew[i] >> 2 & 0xf;
        if (fRCWReadOnly) {
            if (rgplr[i].fInclude) {
                TextIO().pfnPlayerName(i, 1, 1, 1, 0, NULL);
                if (!rgplr[i].fDead)
                    goto DisplayName;
                win32::wsprintfA(&szWork[strlen(szWork)], " (%s)", TextIO().pfnCompressed(idsDeceased));
                win32::SetTextColor(hdc, 127);
                goto DisplayName;
            }
            TextIO().pfnCchGetString(idsUnknownPlayer, szWork);
            goto DisplayName;
        }
        switch (vrgplrTypeNew[i] & 3) {
        case 0:
            TextIO().pfnCchGetString(idsPlayer2, szWork);
            break;
        case 1:
            if (iPlr >= 6) {
                ids = iPlr == 6 ? (fRCWReadOnly ? idsUnknownPlayer : idsRandom) : idsExpansionPlayer;
                TextIO().pfnCchGetString(ids, szWork);
                break;
            }
            TextIO().pfnCchGetString(idsS, szT);
            win32::wsprintfA(szWork, szT, TextIO().pfnCompressed(static_cast<StringId>(idsHumanoid + iPlr)));
            break;
        case 2:
            pszFile = rgszLibraryRaceNew[iPlr][0] ? rgszLibraryRaceNew[iPlr] : vrgszFileNew + iPlr * 13;
            if (!fRCWReadOnly && gd.fNoHostNames && pszFile[0] != 0) {
                win32::wsprintfA(szWork, " %s", pszFile);
                break;
            }
            if (pszFile[0] != 0) {
                win32::wsprintfA(szWork, TextIO().pfnCompressed(!fRCWReadOnly ? idsSS2 : idsS), vrgplrNew[iPlr].szNames, pszFile);
                break;
            }
            win32::wsprintfA(szWork, TextIO().pfnCompressed(idsS), vrgplrNew[iPlr].szNames);
            break;
        case 3:
            win32::wsprintfA(szWork, TextIO().pfnCompressed(idsSSComputerPlayer), vrgszComputerPlayers[iPlr & 7], vrgszComputerLevel[vrgplrTypeNew[i] >> 5]);
        }
    DisplayName:
        win32::TextOutA(hdc, rcDiamond.right + 6, yCur, szWork, strlen(szWork));
        win32::SetTextColor(hdc, crButtonText);
        win32::OffsetRect(&rcDiamond, 0, dyArial8 + 4);
        yCur += dyArial8 + 4;
    }
    win32::SetBkMode(hdc, bkMode);
    if (fCreatedDC) {
        win32::ReleaseDC(hwnd, hdc);
    }
    return;
}

INT_PTR  NewGameDlg3(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int16_t     i;
    RECT        rc;
    POINT16     pt;
    HDC         hdc;
    PAINTSTRUCT ps;

    switch (message) {
    case win32::WM_INITDIALOG:
        win32::SetWindowLongPtrA(hwnd, win32::DWLP_USER, lParam);
        SetNGWTitle(hwnd, 3);
        for (i = 0; i < 7; i++) {
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, i + 291), win32::BM_SETCHECK, GetVCCheck(&game, static_cast<VictoryCondition>((i >= 2) + i)), 0);
            if (fRCWReadOnly) {
                ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, i + 291), 0);
            }
        }
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyNewDlg, 1);
        return 1;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        for (i = 291; i <= 297 && (HWND)lParam != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 297 || message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case win32::WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (IrcRaceDlgHitTest(pt) < 0)
            break;
        win32::SetCursor(hcurHand);
        return 1;
    case win32::WM_LBUTTONDOWN:
    case win32::WM_LBUTTONDBLCLK:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        return FTrackNewGameDlg3(hwnd, pt, wParam);
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        DrawNewGame3(hwnd, hdc, -1);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            win32::WinHelpA(hwnd, szHelpFile, win32::HELP_CONTEXT, idhStep3VictoryConditions);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            if (i == 3 && !FChooseNewGameName(hwnd)) return 1;
            ControlIO().pfnStickyDlgPos(hwnd, &ptStickyNewDlg, 0);
            ControlIO().pfnEndDialog(hwnd, i);
            return 1;
        }
        if (LOWORD(wParam) >= IDC_VC_OWNS_PLANETS && LOWORD(wParam) <= IDC_VC_HIGHEST_SCORE) {
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, LOWORD(wParam)), win32::BM_GETCHECK, 0, 0));
            SetVCCheck(&game, static_cast<VictoryCondition>(LOWORD(wParam) - 291 + ((uint16_t)(LOWORD(wParam) - 291) >= 2)), i);
            DrawNewGame3(hwnd, NULL, 8);
        }
    }
    return 0;
}

void DrawNewGame3(HWND hwnd, HDC hdc, int16_t iDraw) {
    int16_t  yTop;
    int16_t  bt;
    int16_t  vcCur;
    int16_t  irc;
    StringId ids;
    int16_t  fCreatedDC;
    int16_t  i;
    int16_t  dxItem;
    RECT     rcCBox;
    int16_t  j;
    COLORREF crBkSav;
    int16_t  bkMode;
    int16_t  dxDig;
    int16_t  xLeft;
    int16_t  cch;
    RECT     rc;

    fCreatedDC = 0;
    bt = !fRCWReadOnly ? 0 : 4;
    if (!hdc) {
        fCreatedDC = 1;
        hdc = win32::GetDC(hwnd);
    }
    ControlIO().pfnGetClientRect(hwnd, &rc);
    bkMode = win32::SetBkMode(hdc, win32::OPAQUE);
    crBkSav = win32::SetBkColor(hdc, crButtonFace);
    win32::SelectObject(hdc, rghfontArial8[1]);
    yTop = 3 * dyArial8 + 6;
    dxDig = LOWORD(GetTextExtent(hdc, "9", 1));
    ids = idsOwns;
    irc = 0;
    vcCur = 0;
    for (i = 0; i < 9; i++) {
        if (i < 7) {
            win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, i + 291), &rcCBox);
            win32::MapWindowPoints(NULL, hwnd, (POINT *)&rcCBox, 2);
            xLeft = rcCBox.right + 2;
            yTop = (int16_t)(rcCBox.bottom - rcCBox.top) / 2 + rcCBox.top - (dyArial8 >> 1);
        } else if (i == 7) {
            xLeft = rcCBox.left;
            yTop = rcCBox.bottom + 6;
        } else {
            xLeft = rcCBox.left;
            yTop = rcCBox.bottom + 6 + (int16_t)(3 * dyArial8) / 2;
        }
        cch = TextIO().pfnCchGetString(ids, szWork);
        ids = static_cast<StringId>(ids + 1);
        if (iDraw == -1) {
            win32::TextOutA(hdc, xLeft, yTop, szWork, cch);
        }
        xLeft += LOWORD(GetTextExtent(hdc, szWork, cch));
        for (j = 0; j < 2; j++) {
            dxItem = abs(rgNG3Width[i][j]) * dxDig;
            if (dxItem == 0) {
                ids = static_cast<StringId>(ids + 1);
                break;
            }
            win32::wsprintfA(szWork, PCTD, GetVCVal(&game, static_cast<VictoryCondition>(vcCur), 0));
            if (rgNG3Width[i][j] < 0) {
                dxItem += (int16_t)(3 * dxDig) / 2;
                strcat(szWork, "%");
            }
            xLeft += dxItem;
            if (iDraw == -1 || iDraw == vcCur || vcCur == 8) {
                RightTextOut(hdc, xLeft, yTop, szWork, 0, dxItem);
            }
            vcCur++;
            vrgrcRCW[irc].left = xLeft + 4;
            vrgrcRCW[irc].top = yTop - 3;
            vrgrcRCW[irc].right = vrgrcRCW[irc].left + 15;
            vrgrcRCW[irc].bottom = (dyArial8 >> 1) + vrgrcRCW[irc].top + 3;
            vrgrcRCW[irc + 1] = vrgrcRCW[irc];
            win32::OffsetRect((RECT *)&vrgrcRCW[irc + 1].left, 0, vrgrcRCW[irc].bottom - vrgrcRCW[irc].top - 1);
            if (iDraw == -1) {
                DrawBtn(hdc, vrgrcRCW + irc, 0xa0 | bt, 0, NULL);
                DrawBtn(hdc, vrgrcRCW + (irc + 1), 0xa1 | bt, 0, NULL);
            }
            xLeft = vrgrcRCW[irc].right + 4;
            cch = TextIO().pfnCchGetString(ids, szWork);
            if (iDraw == -1) {
                win32::TextOutA(hdc, xLeft, yTop, szWork, cch);
            }
            xLeft += LOWORD(GetTextExtent(hdc, szWork, cch));
            ids = static_cast<StringId>(ids + 1);
            irc += 2;
        }
    }
    crcRCW = irc;
    win32::SetBkColor(hdc, crBkSav);
    win32::SetBkMode(hdc, bkMode);
    if (fCreatedDC) {
        win32::ReleaseDC(hwnd, hdc);
    }
    return;
}

int16_t FTrackNewGameDlg3(HWND hwnd, POINT16 pt, int16_t kbd) {
    int16_t bt;
    int16_t irc;
    BTNT    btnt;
    int16_t i;
    int16_t dShift;
    int16_t iMod;
    int16_t iStat;

    irc = IrcRaceDlgHitTest(pt);
    if (irc < 0) {
        return 0;
    }
    iMod = irc & 1;
    i = irc >> 1;
    if (iMod == 0) {
        dShift = 1;
        bt = 160;
    } else {
        dShift = -1;
        bt = 161;
    }
    InitBtnTrack(&btnt, hwnd, NULL, vrgrcRCW + irc, bt, 80, 0, 0, NULL);
    if (kbd & 0xc) {
        dShift = 5 * dShift;
    }
    while (FTrackBtn(&btnt)) {
        iStat = GetVCVal(&game, static_cast<VictoryCondition>(i), 1);
        if (SetVCVal(&game, static_cast<VictoryCondition>(i), iStat + dShift) != iStat) {
            DrawNewGame3(hwnd, btnt.hdc, i);
        }
    }
    return 1;
}

void SetNGWTitle(HWND hwnd, int16_t iStep) {
    int16_t cch;
    char    szBuf[50];

    cch = TextIO().pfnCchGetString(static_cast<StringId>(idsAdvancedNewGameWizardStepD3 + fRCWReadOnly), szBuf);
    cch = win32::wsprintfA(szWork, szBuf, iStep);
    ControlIO().pfnSetWindowText(hwnd, szWork);
    return;
}
