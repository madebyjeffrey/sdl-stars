#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "text_io.h"
#include "control_io.h"
#include "alert_io.h"
#include "random_io.h"

/* UI-only names retain full UTF-8 library filenames beside the Win16 cache. */
static char rgszLibraryRaceNew[16][256];

struct NEWGAMERACEMENU {
    HMENU hmenu;
    int cRace;
    const char *pszSelected;
};

static int16_t AddNewGameLibraryRace(APPSESSION &, void *lpData, const char *pszName) {
    NEWGAMERACEMENU *pnrm = static_cast<NEWGAMERACEMENU *>(lpData);
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

void NewGameWizard(APPSESSION &app, HWND hwnd, int16_t fReadOnly) {
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
    TextIO().pfnCchGetString(app, idsGameXy, szFile);
    app.lpui->vrgrcRCW = rgrcStack;
    app.lpui->iPanelActive = rwPageNone;
    app.state.fRCWReadOnly = fReadOnly;
    fIdleSav = app.state.gd.fNoIdleChecks;
    app.state.gd.fNoIdleChecks = 1;
    app.lpui->vrgplrNew = rgplrLocal;
    app.lpui->vrgszFileNew = szFileLocal;
    app.lpui->vcplrNew = 0;
    memset(rgszLibraryRaceNew, 0, sizeof(rgszLibraryRaceNew));
    memset(app.state.vrgplrTypeNew, 0, 16);
    if (!fReadOnly) {
        gameT = app.state.game;
        memset(&app.state.game, 0, sizeof(GAME));
        app.state.game.cPlanMax = gameT.cPlanMax;
        app.state.game.cPlayer = gameT.cPlayer;
        lvlAi = lvlAiStandard;
        app.state.game.mdSize = sizeSmall;
        app.state.game.mdDensity = densityNormal;
        app.state.game.mdStartDist = startDistModerate;
        app.state.vrgplrTypeNew[0] = 1;
        app.state.game.rgvc[0] = 136;
        app.state.game.rgvc[1] = 142;
        app.state.game.rgvc[2] = 130;
        app.state.game.rgvc[3] = 10;
        app.state.game.rgvc[4] = 136;
        app.state.game.rgvc[5] = 9;
        app.state.game.rgvc[6] = 9;
        app.state.game.rgvc[7] = 7;
        app.state.game.rgvc[8] = 1;
        TextIO().pfnCchGetString(app, static_cast<StringId>(idsShootingFishBarrel + 5 * lvlAi + app.state.game.mdSize), app.state.game.szName);
        goto Step1;
    }
    lvlAi = lvlAiStandard;
    for (i = 0; i < app.state.game.cPlayer; i++) {
        if (app.state.rgplr[i].fInclude) {
            app.state.vrgplrTypeNew[i] = i << 2 | 2;
            app.lpui->vrgplrNew[i] = app.state.rgplr[i];
            app.lpui->vrgszFileNew[i * 13] = 0;
        } else {
            app.state.vrgplrTypeNew[i] = 25;
        }
    }
Step1:
    lpProc = (NewGameDlg);
    mdRet = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_NEW_GAME_1), hwnd, lpProc, reinterpret_cast<LPARAM>(szFile));
    if (mdRet <= 0) {
    Cancel:
        if (!app.state.fRCWReadOnly) {
            app.state.game = gameT;
        }
        app.state.gd.fNoIdleChecks = fIdleSav;
        return;
    }
    InitNewGamePlr(app, iStepMaxSoFar, static_cast<AiLevel>(lvlAi));
    if (mdRet == 3)
        goto Finish;
Step2:
    lpProc = (NewGameDlg2);
    mdRet = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_NEW_GAME_2), hwnd, lpProc, reinterpret_cast<LPARAM>(szFile));
    if (mdRet <= 0)
        goto Cancel;
    if (iStepMaxSoFar < 1) {
        iStepMaxSoFar = 1;
    }
    InitNewGamePlr(app, iStepMaxSoFar, static_cast<AiLevel>(lvlAi));
    if (mdRet == 1)
        goto Step1;
    if (mdRet == 3)
        goto Finish;
    lpProc = (NewGameDlg3);
    mdRet = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_NEW_GAME_3), hwnd, lpProc, reinterpret_cast<LPARAM>(szFile));
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
    if (app.state.fRCWReadOnly)
        goto Cancel;
    gameT = app.state.game;
    gameT.turn = 0;
    DestroyCurGame(app);
    app.state.game = gameT;
    strcpy(app.state.szBase, szFile);
    app.state.szBase[strlen(app.state.szBase) - 3] = 0;
    for (i = 0; i < 16 && app.state.vrgplrTypeNew[i] != 0; i++) {
        switch (app.state.vrgplrTypeNew[i] & 3) {
        case 1:
            if (app.state.vrgplrTypeNew[i] >> 2 > 6) {
                c = RandomIO().pfnRandom(app, 7);
                app.state.rgplr[i] = vrgplrDef[c];
                app.state.rgplr[i].fAi = 1;
                app.state.rgplr[i].lvlAi = 0;
                app.state.rgplr[i].idAi = idAiMaid;
                app.state.rgplr[i].lSalt = -1;
            } else {
                c = app.state.vrgplrTypeNew[i] >> 2;
                app.state.rgplr[i] = vrgplrDef[c];
            }
            TextIO().pfnCchGetString(app, static_cast<StringId>(idsHumanoid + c), app.state.rgplr[i].szName);
            win32::wsprintfA(app.state.rgplr[i].szNames, "%ss", app.state.rgplr[i].szName);
            break;
        case 2:
            app.state.rgplr[i] = rgplrLocal[app.state.vrgplrTypeNew[i] >> 2];
            break;
        case 3:
            idAi = static_cast<AiRace>(app.state.vrgplrTypeNew[i] >> 2 & 7);
            lvlAi = app.state.vrgplrTypeNew[i] >> 5;
            if (lvlAi >= 4) {
                lvlAi = RandomIO().pfnRandom(app, 4);
            }
            if (static_cast<int16_t>(idAi) >= idAiRandom) {
                idAi = static_cast<AiRace>(RandomIO().pfnRandom(app, 6));
            }
            app.state.rgplr[i] = *LpplrComp(idAi, static_cast<AiLevel>(lvlAi));
            app.state.rgplr[i].fAi = 1;
            app.state.rgplr[i].lvlAi = lvlAi;
            app.state.rgplr[i].idAi = idAi;
        }
    }
    app.state.game.cPlayer = i;
    for (i = 0; i < app.state.game.cPlayer; i++) {
        if (!app.state.rgplr[i].fAi && CAdvantagePoints(app, &app.state.rgplr[i]) < 0) {
            app.state.rgplr[i] = vrgplrDef[0];
            app.state.rgplr[i].fHacker = 1;
        }
        if (app.state.rgplr[i].szName[0] == 0) {
            TextIO().pfnCchGetString(app, static_cast<StringId>(idsBerserker + RandomIO().pfnRandom(app, 24)), app.state.rgplr[i].szName);
        }
        if (app.state.rgplr[i].szNames[0] == 0) {
            win32::wsprintfA(app.state.rgplr[i].szNames, "%ss", app.state.rgplr[i].szName);
        }
    }
    for (i = 1; i < app.state.game.cPlayer; i++) {
        for (j = 0; j < i && strcmp(app.state.rgplr[i].szName, app.state.rgplr[j].szName) != 0; j++) {
        }
        if (j < i) {
            c = RandomIO().pfnRandom(app, 24);
            while (1) {
                for (j = 0; j < app.state.game.cPlayer && strcmp(app.state.rgplr[j].szName, TextIO().pfnCompressed(app, static_cast<StringId>(idsBerserker + c))) != 0; j++) {
                }
                if (j == app.state.game.cPlayer)
                    break;
                c++;
                if (c >= 24) {
                    c = 0;
                }
            }
            TextIO().pfnCchGetString(app, static_cast<StringId>(idsBerserker + c), app.state.rgplr[i].szName);
            strcpy(app.state.rgplr[i].szNames, app.state.rgplr[i].szName);
            strcat(app.state.rgplr[i].szNames, "s");
        }
    }
    for (i = 0; i < app.state.game.cPlayer; i++) {
        rgplrbmp[i] = app.state.rgplr[i].iPlrBmp;
        if (rgplrbmp[i] < 0 || rgplrbmp[i] >= 32 || app.state.rgplr[i].fAi) {
            rgplrbmp[i] = -1;
        }
    }
    for (i = 1; i < app.state.game.cPlayer; i++) {
        if (rgplrbmp[i] != -1) {
            for (j = 0; j < i && rgplrbmp[j] != rgplrbmp[i]; j++) {
            }
            if (j < i) {
                rgplrbmp[RandomIO().pfnRandom(app, 2) == 0 ? j : i] = -1;
            }
        }
    }
    for (i = 0; i < app.state.game.cPlayer; i++) {
        if (rgplrbmp[i] == -1) {
            rgplrbmp[i] = RandomIO().pfnRandom(app, 32);
            while (1) {
                for (j = 0; j < app.state.game.cPlayer && (j == i || rgplrbmp[i] != rgplrbmp[j]); j++) {
                }
                if (j == app.state.game.cPlayer)
                    break;
                rgplrbmp[i]++;
                if (rgplrbmp[i] >= 32) {
                    rgplrbmp[i] = 0;
                }
            }
        }
    }
    for (i = 0; i < app.state.game.cPlayer; i++) {
        app.state.rgplr[i].iPlrBmp = rgplrbmp[i];
    }
    if (!app.lpui->fFreeingTitle) {
        app.lpui->fFreeingTitle = 1;
        ControlIO().pfnDestroyWindow(app.lpui->hwndTitle);
        app.lpui->hwndTitle = 0;
        ui::ShowWindow(app.lpui->hwndFrame, win32::SW_SHOW);
    }
    if (!FFinishLibraryGame(app, hwnd, GenerateWorld(app, 0))) {
        DestroyCurGame(app);
        ShowTitleScreen(app);
        app.state.gd.fNoIdleChecks = fIdleSav;
        return;
    }
    if (app.state.idPlayer == iplrNone) {
        ControlIO().pfnBringUpHostDlg(app);
    }
    app.state.gd.fNoIdleChecks = fIdleSav;
    return;
}

int16_t FGetNewGameName(APPSESSION &app, HWND hwnd, char *szFileSuggest) {
    return FChooseLibraryGameName(app, hwnd, szFileSuggest, app.state.game.szName);
}

static int16_t FChooseNewGameName(APPSESSION &app, HWND hwnd) {
    char *pszFile = reinterpret_cast<char *>(win32::GetWindowLongPtrA(hwnd, win32::DWLP_USER));

    // Reserve the library entry before closing the wizard so failures keep it open.
    return app.state.fRCWReadOnly || (pszFile && FGetNewGameName(app, hwnd, pszFile));
}

INT_PTR  NewGameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

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
        SetNGWTitle(app, hwnd, 1);
        win32::CheckRadioButton(hwnd, IDC_NEW_GAME_TINY, IDC_NEW_GAME_HUGE, app.state.game.mdSize + 1000);
        win32::CheckRadioButton(hwnd, IDC_NEW_GAME_SPARSE, IDC_NEW_GAME_PACKED, app.state.game.mdDensity + 1005);
        win32::CheckRadioButton(hwnd, IDC_NEW_GAME_CLOSE, IDC_NEW_GAME_DISTANT, app.state.game.mdStartDist + 1009);
        ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_NAME), app.state.game.szName);
        ui::SendDlgItemMessage(hwnd, IDC_NEW_GAME_NAME, win32::EM_LIMITTEXT, 0x1f, 0);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_MAX_MINERALS), win32::BM_SETCHECK, app.state.game.fExtraFuel, 0);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_SLOWER_TECH), win32::BM_SETCHECK, app.state.game.fSlowTech, 0);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_ACCELERATED_BBS), win32::BM_SETCHECK, app.state.game.fBBSPlay, 0);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_NO_RANDOM_EVENTS), win32::BM_SETCHECK, app.state.game.fNoRandom, 0);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_AI_ALLIANCES), win32::BM_SETCHECK, app.state.game.fAisBand, 0);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_PUBLIC_SCORES), win32::BM_SETCHECK, app.state.game.fVisScores, 0);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_GALAXY_CLUMPING), win32::BM_SETCHECK, app.state.game.fClumping, 0);
        if (app.state.fRCWReadOnly) {
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
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyNewDlg, 1);
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        for (i = 1000; i <= 1021 && reinterpret_cast<HWND>(lParam) != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 1021 || message == win32::WM_CTLCOLORSTATIC || reinterpret_cast<HWND>(lParam) == ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_GALAXY_CLUMPING)) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_TINY), &rcGBox);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcGBox));
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_HUGE), &rc);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rc.right));
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, app.state.dyArial8, app.state.dyArial8 >> 1);
        _Draw3dFrame(app, hdc, &rcGBox, -1);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        win32::SetBkColor(hdc, app.lpui->crButtonFace);
        c = TextIO().pfnCchGetString(app, idsUniverseSize, app.state.szWork);
        win32::TextOutA(hdc, rcGBox.left + 8, rcGBox.top - (app.state.dyArial8 >> 1), app.state.szWork, c);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_SPARSE), &rcGBox);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcGBox));
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_PACKED), &rc);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rc.right));
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, app.state.dyArial8, app.state.dyArial8 >> 1);
        _Draw3dFrame(app, hdc, &rcGBox, -1);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        win32::SetBkColor(hdc, app.lpui->crButtonFace);
        c = TextIO().pfnCchGetString(app, idsDensity, app.state.szWork);
        win32::TextOutA(hdc, rcGBox.left + 8, rcGBox.top - (app.state.dyArial8 >> 1), app.state.szWork, c);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_CLOSE), &rcGBox);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcGBox));
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_DISTANT), &rc);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rc.right));
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, app.state.dyArial8, app.state.dyArial8 >> 1);
        _Draw3dFrame(app, hdc, &rcGBox, -1);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        win32::SetBkColor(hdc, app.lpui->crButtonFace);
        c = TextIO().pfnCchGetString(app, idsPlayerPositions, app.state.szWork);
        win32::TextOutA(hdc, rcGBox.left + 8, rcGBox.top - (app.state.dyArial8 >> 1), app.state.szWork, c);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhStep1SpecifyingTheUniverse);
            return 1;
        }
        for (iRet = 0; iRet < 4 && LOWORD(wParam) != app.lpui->rgidRaceBtn[iRet]; iRet++) {
        }
        if (iRet < 4) {
            if (iRet != 0) {
                for (i = 1000; i <= 1004 && win32::IsDlgButtonChecked(hwnd, i) == 0; i++) {
                }
                app.state.game.mdSize = static_cast<UniverseSize>(i - 1000);
                for (i = 1005; i <= 1008 && win32::IsDlgButtonChecked(hwnd, i) == 0; i++) {
                }
                app.state.game.mdDensity = static_cast<UniverseDensity>(i - 1005);
                for (i = 1009; i <= 1012 && win32::IsDlgButtonChecked(hwnd, i) == 0; i++) {
                }
                app.state.game.mdStartDist = static_cast<StartDistance>(i - 1009);
                app.state.game.fExtraFuel = win32::IsDlgButtonChecked(hwnd, IDC_NEW_GAME_MAX_MINERALS);
                app.state.game.fSlowTech = win32::IsDlgButtonChecked(hwnd, IDC_NEW_GAME_SLOWER_TECH);
                app.state.game.fBBSPlay = win32::IsDlgButtonChecked(hwnd, IDC_NEW_GAME_ACCELERATED_BBS);
                app.state.game.fNoRandom = win32::IsDlgButtonChecked(hwnd, IDC_NEW_GAME_NO_RANDOM_EVENTS);
                app.state.game.fAisBand = win32::IsDlgButtonChecked(hwnd, IDC_NEW_GAME_AI_ALLIANCES);
                app.state.game.fVisScores = win32::IsDlgButtonChecked(hwnd, IDC_NEW_GAME_PUBLIC_SCORES);
                app.state.game.fClumping = win32::IsDlgButtonChecked(hwnd, IDC_NEW_GAME_GALAXY_CLUMPING);
                i = ControlIO().pfnGetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_NEW_GAME_NAME), app.state.game.szName, 32);
                if (i == 0) {
                    strcpy(app.state.game.szName, app.state.szBase);
                }
            }
            if (iRet == 3 && !FChooseNewGameName(app, hwnd)) return 1;
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyNewDlg, 0);
            ControlIO().pfnEndDialog(hwnd, iRet);
            return 1;
        }
        break;
    }
    return 0;
}

INT_PTR  NewGameDlg2(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

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
        SetNGWTitle(app, hwnd, 2);
        dy = (app.state.dyArial8 + 4) * 16 + 8;
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, app.lpui->rgidRaceBtn[0]), &rc);
        dyBut = rc.bottom - rc.top;
        win32::GetWindowRect(hwnd, &rcT);
        dyCur = rcT.bottom - rcT.top;
        if (dyCur < dy + dyBut + 6) {
            for (i = 0; i < 5; i++) {
                hwndBtn = ControlIO().pfnGetDlgItem(hwnd, app.lpui->rgidRaceBtn[i]);
                win32::GetWindowRect(hwndBtn, &rc);
                win32::MapWindowPoints(nullptr, hwnd, reinterpret_cast<POINT *>(&rc), 2);
                win32::OffsetRect(&rc, 0, dy - rc.top);
                ControlIO().pfnSetWindowPos(hwndBtn, nullptr, rc.left, rc.top, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER);
            }
            dy = rc.bottom + 6;
            ControlIO().pfnGetClientRect(hwnd, &rc);
            if (dyCur < dy) {
                ControlIO().pfnSetWindowPos(hwnd, nullptr, 0, 0, rcT.right - rcT.left, dy + rcT.bottom - rcT.top - rc.bottom, win32::SWP_NOMOVE | win32::SWP_NOZORDER);
            }
        }
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyNewDlg, 1);
        return 1;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        for (i = 401; i <= 448 && reinterpret_cast<HWND>(lParam) != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 448 || message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_SETCURSOR:
        if (app.state.fRCWReadOnly)
            break;
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (pt.x < app.lpui->xNewGameDiamond || pt.x >= app.lpui->xNewGameDiamond + app.state.dyArial8 + 1 || pt.y < 6)
            break;
        iDiamond = static_cast<int16_t>((pt.y - 6)) / (app.state.dyArial8 + 4);
        if (iDiamond >= 16 || static_cast<int16_t>((pt.y - 6)) % (app.state.dyArial8 + 4) >= app.state.dyArial8 + 1)
            break;
        win32::SetCursor(app.lpui->hcurHand);
        return 1;
    case win32::WM_LBUTTONDOWN:
    case win32::WM_RBUTTONDOWN:
        if (app.state.fRCWReadOnly)
            break;
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        if (pt.x < app.lpui->xNewGameDiamond || pt.x >= app.lpui->xNewGameDiamond + app.state.dyArial8 + 1 || pt.y < 6)
            break;
        iDiamond = static_cast<int16_t>((pt.y - 6)) / (app.state.dyArial8 + 4);
        if (iDiamond >= 16 || static_cast<int16_t>((pt.y - 6)) % (app.state.dyArial8 + 4) >= app.state.dyArial8 + 1)
            break;
        iCurVal = app.state.vrgplrTypeNew[iDiamond];
        ClientToScreen16(hwnd, &pt);
        rghmenuSubPopup[0] = win32::CreatePopupMenu();
        for (i = 0; i < 6; i++) {
            iChecked = iCurVal == i * 4 + 1 ? 8 : 0;
            psz = TextIO().pfnCompressed(app, static_cast<StringId>(idsHumanoid + i));
            win32::AppendMenuA(rghmenuSubPopup[0], iChecked, i + 15016, psz);
        }
        win32::AppendMenuA(rghmenuSubPopup[0], iCurVal == 25 ? win32::MF_CHECKED : 0, 15022, TextIO().pfnCompressed(app, idsRandom));
        win32::AppendMenuA(rghmenuSubPopup[0], iCurVal == 29 ? win32::MF_CHECKED : 0, 15023, TextIO().pfnCompressed(app, idsExpansionPlayer));
        rghmenuSubPopup[1] = win32::CreatePopupMenu();
        nrm.hmenu = rghmenuSubPopup[1];
        nrm.cRace = 0;
        nrm.pszSelected = (iCurVal & 3) == 2 ? rgszLibraryRaceNew[iCurVal >> 2] : nullptr;
        FEnumerateLibraryRaces(app, AddNewGameLibraryRace, &nrm);
        if (nrm.cRace == 0)
            win32::AppendMenuA(nrm.hmenu, win32::MF_GRAYED, 0, TextIO().pfnCompressed(app, idsLibraryNoRaces));
        rghmenuSubPopup[2] = win32::CreatePopupMenu();
        for (i = 0; i <= 6; i++) {
            rghmenuSubPopup[i + 5] = win32::CreatePopupMenu();
            for (j = 0; j < 5; j++) {
                iChecked = iCurVal == j * 32 + i * 4 + 3 ? 8 : 0;
                win32::AppendMenuA(rghmenuSubPopup[i + 5], iChecked, i + 15048 + j * 8, app.lpui->vrgszComputerLevel[j]);
            }
            iChecked = (iCurVal & 0x1f) == i * 4 + 3 ? 8 : 0;
            win32::AppendMenuA(rghmenuSubPopup[2], 0x10 | iChecked, reinterpret_cast<UINT_PTR>(rghmenuSubPopup[i + 5]), app.lpui->vrgszComputerPlayers[i]);
        }
        hmenuPopup = win32::CreatePopupMenu();
        app.lpui->iPopMenuSel = -1;
        iChecked = (iCurVal & 3) == 1 ? 8 : 0;
        win32::AppendMenuA(hmenuPopup, 0x10 | iChecked, reinterpret_cast<UINT_PTR>(rghmenuSubPopup[0]), TextIO().pfnCompressed(app, idsPredefinedRace));
        iChecked = (iCurVal & 3) == 2 ? 8 : 0;
        win32::AppendMenuA(hmenuPopup, 0x10 | iChecked, reinterpret_cast<UINT_PTR>(rghmenuSubPopup[1]), TextIO().pfnCompressed(app, idsLibraryRaceMenu));
        iChecked = (iCurVal & 3) == 3 ? 8 : 0;
        win32::AppendMenuA(hmenuPopup, 0x10 | iChecked, reinterpret_cast<UINT_PTR>(rghmenuSubPopup[2]), TextIO().pfnCompressed(app, idsComputerPlayer));
        win32::AppendMenuA(hmenuPopup, win32::MF_SEPARATOR, 0, nullptr);
        win32::AppendMenuA(hmenuPopup, win32::MF_BYCOMMAND, 15000, TextIO().pfnCompressed(app, idsLibraryNewRace));
        if (((iCurVal & 3) == 1 && (iCurVal >> 2) < 7) || (iCurVal & 3) == 2)
            win32::AppendMenuA(hmenuPopup, win32::MF_BYCOMMAND, 15002, TextIO().pfnCompressed(app, idsEditRace));
        iChecked = !(iCurVal & 3) ? 8 : 0;
        win32::AppendMenuA(hmenuPopup, iChecked, 0x3a9b, TextIO().pfnCompressed(app, idsPlayer));
        tpm = message == win32::WM_LBUTTONDOWN ? win32::TPM_LEFTBUTTON : win32::TPM_RIGHTBUTTON;
        ui::TrackPopupMenu(hmenuPopup, win32::TPM_CENTERALIGN | tpm, pt.x, pt.y, 0, hwnd, nullptr);
        if (ui::PeekMessage(&msg, hwnd, 273, 273, 2) && msg.wParam >= 15000 && msg.wParam < 15000 + INT16_MAX) {
            app.lpui->iPopMenuSel = msg.wParam - 15000;
        }
        szLibraryRace[0] = 0;
        if (app.lpui->iPopMenuSel >= 1000 && app.lpui->iPopMenuSel < 1000 + nrm.cRace) {
            win32::GetMenuStringA(nrm.hmenu, 15000 + app.lpui->iPopMenuSel, szLibraryLabel, sizeof(szLibraryLabel), win32::MF_BYCOMMAND);
            for (pch = szLibraryLabel, pchName = szLibraryRace; *pch; pch++) {
                *pchName++ = *pch;
                if (*pch == '&' && pch[1] == '&') pch++;
            }
            *pchName = 0;
        }
        win32::DestroyMenu(hmenuPopup);
        iNewVal = -1;
        if (app.lpui->iPopMenuSel != -1) {
            if (app.lpui->iPopMenuSel == 0) {
                app.state.vplr = vrgplrDef[0];
                TextIO().pfnCchGetString(app, idsHumanoid, app.state.vplr.szName);
                win32::wsprintfA(app.state.vplr.szNames, "%ss", app.state.vplr.szName);
                szLibraryRace[0] = 0;
                app.state.szRacePass[0] = 0;
                prcSav = app.lpui->vrgrcRCW;
                if (FCustomizeLibraryRace(app, hwnd, szLibraryRace)) {
                    app.lpui->vrgrcRCW = prcSav;
                    goto PlaceNew;
                }
                app.lpui->vrgrcRCW = prcSav;
            }
            if (app.lpui->iPopMenuSel >= 1000 && app.lpui->iPopMenuSel < 1000 + nrm.cRace) {
                if (FLoadLibraryRace(app, hwnd, szLibraryRace)) {
                PlaceNew:
                    if (app.lpui->vcplrNew < 16) {
                        iNewVal = app.lpui->vcplrNew++;
                    } else {
                        for (iNewVal = 15; iNewVal >= 0; iNewVal--) {
                            for (i = 0; i < 16 && (i == iDiamond || (app.state.vrgplrTypeNew[i] & 3) != 2 || (app.state.vrgplrTypeNew[i] >> 2 & 0xf) != iNewVal); i++) {
                            }
                            if (i == 16)
                                break;
                        }
                    }
                    if (iNewVal < 0) goto FinishClick;
                    app.lpui->vrgplrNew[iNewVal] = app.state.vplr;
                    CchCopyUtf8(app.lpui->vrgszFileNew + iNewVal * 13, 13, app.state.szRaceFile, strlen(app.state.szRaceFile));
                    strcpy(rgszLibraryRaceNew[iNewVal], szLibraryRace);
                    iNewVal = iNewVal << 2 | 2;
                }
            } else if (app.lpui->iPopMenuSel == 2) {
                if ((iCurVal & 3) == 1) {
                    app.state.vplr = vrgplrDef[iCurVal >> 2];
                    TextIO().pfnCchGetString(app, static_cast<StringId>((iCurVal >> 2) + 0x567), app.state.vplr.szName);
                    win32::wsprintfA(app.state.vplr.szNames, "%ss", app.state.vplr.szName);
                } else {
                    app.state.vplr = app.lpui->vrgplrNew[iCurVal >> 2];
                    strcpy(app.state.szRaceFile, app.lpui->vrgszFileNew + (iCurVal >> 2) * 13);
                }
                app.state.lSaltCur = app.state.vplr.lSalt;
                app.state.lSaltLast = 0;
                if (!FCheckPassword(app))
                    goto FinishClick;
                if (app.state.vplr.lSalt != 0) {
                    strcpy(app.state.szRacePass, app.state.szPassLast);
                } else {
                    app.state.szRacePass[0] = 0;
                }
                CchSnprintf(szLibraryRace, sizeof(szLibraryRace), "%s",
                              (iCurVal & 3) == 2 ? rgszLibraryRaceNew[iCurVal >> 2] : "");
                prcSav = app.lpui->vrgrcRCW;
                if (FCustomizeLibraryRace(app, hwnd, szLibraryRace)) {
                    app.lpui->vrgrcRCW = prcSav;
                    if ((iCurVal & 3) == 1 || strcmp(szLibraryRace, rgszLibraryRaceNew[iCurVal >> 2]) != 0)
                        goto PlaceNew;
                    app.lpui->vrgplrNew[iCurVal >> 2] = app.state.vplr;
                    CchCopyUtf8(app.lpui->vrgszFileNew + (iCurVal >> 2) * 13, 13, app.state.szRaceFile, strlen(app.state.szRaceFile));
                    strcpy(rgszLibraryRaceNew[iCurVal >> 2], szLibraryRace);
                    iNewVal = iCurVal;
                    iCurVal = -1;
                }
                app.lpui->vrgrcRCW = prcSav;
            } else if (app.lpui->iPopMenuSel == 3) {
                iNewVal = 0;
            } else if (app.lpui->iPopMenuSel >= 16 && app.lpui->iPopMenuSel < 32) {
                iNewVal = (app.lpui->iPopMenuSel - 0x10) << 2 | 1;
            } else if (app.lpui->iPopMenuSel >= 48 && app.lpui->iPopMenuSel < 88) {
                iNewVal = (app.lpui->iPopMenuSel - 0x30) << 2 | 3;
            }
        }
        if (iNewVal != -1 && iNewVal != iCurVal) {
            app.state.vrgplrTypeNew[iDiamond] = iNewVal;
            if (iNewVal == 0) {
                while (++iDiamond < 16) {
                    app.state.vrgplrTypeNew[iDiamond] = 0;
                }
            } else if (iDiamond > 0) {
                while (--iDiamond >= 0 && app.state.vrgplrTypeNew[iDiamond] == 0) {
                    app.state.vrgplrTypeNew[iDiamond] = iNewVal;
                }
            }
            ControlIO().pfnInvalidateRect(hwnd, nullptr, 1);
        }
    FinishClick:
        ControlIO().pfnSetFocus(hwnd);
        break;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        DrawNewGame2(app, hwnd, hdc, -1);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhStep2SpecifyingThePlayers);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != app.lpui->rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            if (i != 0 && app.state.vrgplrTypeNew[0] == 0 && !app.state.fRCWReadOnly) {
                AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsMustHaveLeastOnePlayerGame, nullptr), win32::MB_ICONHAND);
            } else {
                if (i == 3 && !FChooseNewGameName(app, hwnd)) return 1;
                ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyNewDlg, 0);
                ControlIO().pfnEndDialog(hwnd, i);
                return 1;
            }
        }
    }
    return 0;
}

void DrawNewGame2(APPSESSION &app, HWND hwnd, HDC hdc, int16_t iDraw) {
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
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    app.lpui->xNewGameDiamond = LOWORD(GetTextExtent(hdc, TextIO().pfnCompressed(app, idsPlayer16), 11)) + 12;
    yCur = 6;
    win32::SetRect(&rcDiamond, app.lpui->xNewGameDiamond, yCur, app.lpui->xNewGameDiamond + app.state.dyArial8 + 1, yCur + app.state.dyArial8 + 1);
    for (i = 0; i < 16 && (!app.state.fRCWReadOnly || i < app.state.game.cPlayer); i++) {
        cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsPlayerD), i + 1);
        app.state.szWork[cch++] = ':';
        app.state.szWork[cch] = 0;
        RightTextOut(app, hdc, app.lpui->xNewGameDiamond - 6, yCur, app.state.szWork, cch, 0);
        DrawDiamond(app, hdc, &rcDiamond, app.lpui->hbrBBlue);
        iPlr = app.state.vrgplrTypeNew[i] >> 2 & 0xf;
        if (app.state.fRCWReadOnly) {
            if (app.state.rgplr[i].fInclude) {
                TextIO().pfnPlayerName(app, i, 1, 1, 1, 0, nullptr);
                if (!app.state.rgplr[i].fDead)
                    goto DisplayName;
                win32::wsprintfA(&app.state.szWork[strlen(app.state.szWork)], " (%s)", TextIO().pfnCompressed(app, idsDeceased));
                win32::SetTextColor(hdc, 127);
                goto DisplayName;
            }
            TextIO().pfnCchGetString(app, idsUnknownPlayer, app.state.szWork);
            goto DisplayName;
        }
        switch (app.state.vrgplrTypeNew[i] & 3) {
        case 0:
            TextIO().pfnCchGetString(app, idsPlayer2, app.state.szWork);
            break;
        case 1:
            if (iPlr >= 6) {
                ids = iPlr == 6 ? (app.state.fRCWReadOnly ? idsUnknownPlayer : idsRandom) : idsExpansionPlayer;
                TextIO().pfnCchGetString(app, ids, app.state.szWork);
                break;
            }
            TextIO().pfnCchGetString(app, idsS, szT);
            win32::wsprintfA(app.state.szWork, szT, TextIO().pfnCompressed(app, static_cast<StringId>(idsHumanoid + iPlr)));
            break;
        case 2:
            pszFile = rgszLibraryRaceNew[iPlr][0] ? rgszLibraryRaceNew[iPlr] : app.lpui->vrgszFileNew + iPlr * 13;
            if (!app.state.fRCWReadOnly && app.state.gd.fNoHostNames && pszFile[0] != 0) {
                win32::wsprintfA(app.state.szWork, " %s", pszFile);
                break;
            }
            if (pszFile[0] != 0) {
                win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, !app.state.fRCWReadOnly ? idsSS2 : idsS), app.lpui->vrgplrNew[iPlr].szNames, pszFile);
                break;
            }
            win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsS), app.lpui->vrgplrNew[iPlr].szNames);
            break;
        case 3:
            win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsSSComputerPlayer), app.lpui->vrgszComputerPlayers[iPlr & 7], app.lpui->vrgszComputerLevel[app.state.vrgplrTypeNew[i] >> 5]);
        }
    DisplayName:
        win32::TextOutA(hdc, rcDiamond.right + 6, yCur, app.state.szWork, strlen(app.state.szWork));
        win32::SetTextColor(hdc, app.lpui->crButtonText);
        win32::OffsetRect(&rcDiamond, 0, app.state.dyArial8 + 4);
        yCur += app.state.dyArial8 + 4;
    }
    win32::SetBkMode(hdc, bkMode);
    if (fCreatedDC) {
        win32::ReleaseDC(hwnd, hdc);
    }
    return;
}

INT_PTR  NewGameDlg3(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    int16_t     i;
    RECT        rc;
    POINT16     pt;
    HDC         hdc;
    PAINTSTRUCT ps;

    switch (message) {
    case win32::WM_INITDIALOG:
        win32::SetWindowLongPtrA(hwnd, win32::DWLP_USER, lParam);
        SetNGWTitle(app, hwnd, 3);
        for (i = 0; i < 7; i++) {
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, i + 291), win32::BM_SETCHECK, GetVCCheck(&app.state.game, static_cast<VictoryCondition>((i >= 2) + i)), 0);
            if (app.state.fRCWReadOnly) {
                ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, i + 291), 0);
            }
        }
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyNewDlg, 1);
        return 1;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        for (i = 291; i <= 297 && reinterpret_cast<HWND>(lParam) != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 297 || message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (IrcRaceDlgHitTest(app, pt) < 0)
            break;
        win32::SetCursor(app.lpui->hcurHand);
        return 1;
    case win32::WM_LBUTTONDOWN:
    case win32::WM_LBUTTONDBLCLK:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        return FTrackNewGameDlg3(app, hwnd, pt, wParam);
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        DrawNewGame3(app, hwnd, hdc, -1);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhStep3VictoryConditions);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != app.lpui->rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            if (i == 3 && !FChooseNewGameName(app, hwnd)) return 1;
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyNewDlg, 0);
            ControlIO().pfnEndDialog(hwnd, i);
            return 1;
        }
        if (LOWORD(wParam) >= IDC_VC_OWNS_PLANETS && LOWORD(wParam) <= IDC_VC_HIGHEST_SCORE) {
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, LOWORD(wParam)), win32::BM_GETCHECK, 0, 0));
            SetVCCheck(&app.state.game, static_cast<VictoryCondition>(LOWORD(wParam) - 291 + (static_cast<uint16_t>((LOWORD(wParam) - 291)) >= 2)), i);
            DrawNewGame3(app, hwnd, nullptr, 8);
        }
    }
    return 0;
}

void DrawNewGame3(APPSESSION &app, HWND hwnd, HDC hdc, int16_t iDraw) {
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
    bt = !app.state.fRCWReadOnly ? 0 : 4;
    if (!hdc) {
        fCreatedDC = 1;
        hdc = win32::GetDC(hwnd);
    }
    ControlIO().pfnGetClientRect(hwnd, &rc);
    bkMode = win32::SetBkMode(hdc, win32::OPAQUE);
    crBkSav = win32::SetBkColor(hdc, app.lpui->crButtonFace);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    yTop = 3 * app.state.dyArial8 + 6;
    dxDig = LOWORD(GetTextExtent(hdc, "9", 1));
    ids = idsOwns;
    irc = 0;
    vcCur = 0;
    for (i = 0; i < 9; i++) {
        if (i < 7) {
            win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, i + 291), &rcCBox);
            win32::MapWindowPoints(nullptr, hwnd, reinterpret_cast<POINT *>(&rcCBox), 2);
            xLeft = rcCBox.right + 2;
            yTop = static_cast<int16_t>((rcCBox.bottom - rcCBox.top)) / 2 + rcCBox.top - (app.state.dyArial8 >> 1);
        } else if (i == 7) {
            xLeft = rcCBox.left;
            yTop = rcCBox.bottom + 6;
        } else {
            xLeft = rcCBox.left;
            yTop = rcCBox.bottom + 6 + static_cast<int16_t>((3 * app.state.dyArial8)) / 2;
        }
        cch = TextIO().pfnCchGetString(app, ids, app.state.szWork);
        ids = static_cast<StringId>(ids + 1);
        if (iDraw == -1) {
            win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, cch);
        }
        xLeft += LOWORD(GetTextExtent(hdc, app.state.szWork, cch));
        for (j = 0; j < 2; j++) {
            dxItem = abs(rgNG3Width[i][j]) * dxDig;
            if (dxItem == 0) {
                ids = static_cast<StringId>(ids + 1);
                break;
            }
            win32::wsprintfA(app.state.szWork, app.state.PCTD, GetVCVal(app, &app.state.game, static_cast<VictoryCondition>(vcCur), 0));
            if (rgNG3Width[i][j] < 0) {
                dxItem += static_cast<int16_t>((3 * dxDig)) / 2;
                strcat(app.state.szWork, "%");
            }
            xLeft += dxItem;
            if (iDraw == -1 || iDraw == vcCur || vcCur == 8) {
                RightTextOut(app, hdc, xLeft, yTop, app.state.szWork, 0, dxItem);
            }
            vcCur++;
            app.lpui->vrgrcRCW[irc].left = xLeft + 4;
            app.lpui->vrgrcRCW[irc].top = yTop - 3;
            app.lpui->vrgrcRCW[irc].right = app.lpui->vrgrcRCW[irc].left + 15;
            app.lpui->vrgrcRCW[irc].bottom = (app.state.dyArial8 >> 1) + app.lpui->vrgrcRCW[irc].top + 3;
            app.lpui->vrgrcRCW[irc + 1] = app.lpui->vrgrcRCW[irc];
            win32::OffsetRect(reinterpret_cast<RECT *>(&app.lpui->vrgrcRCW[irc + 1].left), 0, app.lpui->vrgrcRCW[irc].bottom - app.lpui->vrgrcRCW[irc].top - 1);
            if (iDraw == -1) {
                DrawBtn(app, hdc, app.lpui->vrgrcRCW + irc, 0xa0 | bt, 0, nullptr);
                DrawBtn(app, hdc, app.lpui->vrgrcRCW + (irc + 1), 0xa1 | bt, 0, nullptr);
            }
            xLeft = app.lpui->vrgrcRCW[irc].right + 4;
            cch = TextIO().pfnCchGetString(app, ids, app.state.szWork);
            if (iDraw == -1) {
                win32::TextOutA(hdc, xLeft, yTop, app.state.szWork, cch);
            }
            xLeft += LOWORD(GetTextExtent(hdc, app.state.szWork, cch));
            ids = static_cast<StringId>(ids + 1);
            irc += 2;
        }
    }
    app.lpui->crcRCW = irc;
    win32::SetBkColor(hdc, crBkSav);
    win32::SetBkMode(hdc, bkMode);
    if (fCreatedDC) {
        win32::ReleaseDC(hwnd, hdc);
    }
    return;
}

int16_t FTrackNewGameDlg3(APPSESSION &app, HWND hwnd, POINT16 pt, int16_t kbd) {
    int16_t bt;
    int16_t irc;
    BTNT    btnt;
    int16_t i;
    int16_t dShift;
    int16_t iMod;
    int16_t iStat;

    irc = IrcRaceDlgHitTest(app, pt);
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
    InitBtnTrack(&btnt, hwnd, nullptr, app.lpui->vrgrcRCW + irc, bt, 80, 0, 0, nullptr);
    if (kbd & 0xc) {
        dShift = 5 * dShift;
    }
    while (FTrackBtn(app, &btnt)) {
        iStat = GetVCVal(app, &app.state.game, static_cast<VictoryCondition>(i), 1);
        if (SetVCVal(app, &app.state.game, static_cast<VictoryCondition>(i), iStat + dShift) != iStat) {
            DrawNewGame3(app, hwnd, btnt.hdc, i);
        }
    }
    return 1;
}

void SetNGWTitle(APPSESSION &app, HWND hwnd, int16_t iStep) {
    int16_t cch;
    char    szBuf[50];

    cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsAdvancedNewGameWizardStepD3 + app.state.fRCWReadOnly), szBuf);
    cch = win32::wsprintfA(app.state.szWork, szBuf, iStep);
    ControlIO().pfnSetWindowText(hwnd, app.state.szWork);
    return;
}
