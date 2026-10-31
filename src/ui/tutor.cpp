#include "win.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"
#include "alert_io.h"

ITEMACTION rgiaUnloadAllCol[5] = {{0}, {0}, {0}, IaMake(0, iActionUnloadAll)};
ITEMACTION rgiaQuikDrop[5] = {IaMake(0, iActionUnloadAll),
                              IaMake(0, iActionUnloadAll),
                              IaMake(0, iActionUnloadAll),
                              IaMake(0, iActionUnloadAll),
                              IaMake(0, iActionLoadDunnage)};
ITEMACTION rgiaQuikLoad[5] = {IaMake(0, iActionLoadAll), IaMake(0, iActionLoadAll), IaMake(0, iActionLoadAll), {0}, IaMake(0, iActionLoadDunnage)};
ITEMACTION rgiaLoadAllCol[5] = {{0}, {0}, {0}, IaMake(0, iActionLoadAll)};
ZIPPRODQ1  rgzpqTut[2] = {
    {
         .fNoResearch = 1,
         .cpq = 2,
         .rgpq = {PqMake(iobjFactory, 3), PqMake(0, 3)},
    },
    {
         .fNoResearch = 1,
         .cpq = 3,
         .rgpq = {PqMake(iobjMinTerraform, 2), PqMake(iobjFactory, 3), PqMake(0, 3)},
    }};

INT_PTR  TutorDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HMENU   hmenu;
    RECT    rc;
    FARPROC lpProc;
    int16_t fRet;

    switch (message) {
    case win32::WM_INITDIALOG:
        tutor.hwnd = hwnd;
        ControlIO().pfnSetWindowPos(hwnd, (HWND)-1, 0, 0, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOMOVE);
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyTutorDlg, 1);
        return 1;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case win32::WM_PAINT:
        DrawTutorText(hwnd);
        return 1;
    case win32::WM_CHAR:
        win32::PostMessageA(hwndFrame, win32::WM_CHAR, wParam, lParam);
        return 0;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_TUTOR_PANIC:
            lpProc = (PanicDlg);
            fRet = ControlIO().pfnDialogBox(hInst, win32::MAKEINTRESOURCE(IDD_PANIC), hwnd, lpProc, 0);
            if (!fRet) {
                return 0;
            }
            tutor.fAutoComplete = fRet == 2506;
            win32::PostMessageA(hwndFrame, WM_STARS_CONTINUE, fRet, 0);
            return 0;
        case IDCANCEL:
            ShowTutor(0);
            if (tutor.fShowHidMsg) {
                AlertIO().pfnAlertSz(TextIO().pfnFormat(idsMakeTutorialReappearCompleteTaskChooseTutorial, NULL), win32::MB_ICONASTERISK);
                tutor.fShowHidMsg = 0;
            }
            return 1;
        case IDC_TUTOR_HINT:
            win32::WinHelpA(hwnd, szHelpFile, win32::HELP_CONTEXT, (int16_t)tutor.idh);
            return 1;
        default:
            return 0;
        }
    case win32::WM_DESTROY:
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyTutorDlg, 0);
        tutor.hwnd = 0;
        hmenu = GetASubMenu(hwndFrame, menuHelp);
        win32::CheckMenuItem(hmenu, IDM_HELP_TUTORIAL, win32::MF_UNCHECKED);
        ControlIO().pfnEndDialog(hwnd, 1);
        EndTutor(1);
        return 1;
    default:
        return 0;
    }
}

INT_PTR  PanicDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    RECT rc;

    switch (message) {
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
        if (message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            ControlIO().pfnEndDialog(hwnd, 0);
            return 1;
        case IDC_HELP:
            win32::WinHelpA(hwnd, szHelpFile, win32::HELP_CONTEXT, (int16_t)tutor.idh);
            return 1;
        case IDC_PANIC_REDO_TURN:
        case IDC_PANIC_COMPLETE_TURN:
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam));
            return 1;
        }
        break;
    }
    return 0;
}

void ShowTutor(int16_t fShow) {
    if (tutor.hwnd) {
        // Instruction updates call this while the tutor is already visible;
        // activating it again takes focus from the control the player is using.
        if (!fShow || !win32::IsWindowVisible(tutor.hwnd))
            ui::ShowWindow(tutor.hwnd, !fShow ? win32::SW_HIDE : win32::SW_SHOW);
        tutor.fVisible = fShow;
    }
    return;
}

void DrawTutorText(HWND hwnd) {
    HDC         hdc;
    int16_t     yTop;
    int16_t     fPara;
    PAINTSTRUCT ps;
    int16_t     didt;
    int16_t     xLeft;
    int16_t     cch;
    char        rgch[256];
    RECT        rcBtn;
    RECT        rc;

    hdc = win32::BeginPaint(hwnd, &ps);
    win32::SetBkColor(hdc, crButtonFace);
    win32::SelectObject(hdc, rghfontArial8[0]);
    win32::GetWindowRect(hwnd, &rc);
    win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDCANCEL), &rcBtn);
    rc.bottom = rcBtn.top;
    win32::ScreenToClient(hwnd, (POINT *)&rc);
    win32::ScreenToClient(hwnd, (POINT *)&rc.right);
    rc.top += dyArial8 * 2;
    rc.bottom -= (int16_t)(dyArial8 * 2) / 3;
    rc.left += (int16_t)(dyArial8 * 2) / 3;
    rc.right -= (int16_t)(dyArial8 * 2) / 3;
    win32::SelectObject(hdc, hbrButtonShadow);
    win32::PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, 1, win32::PATCOPY);
    win32::PatBlt(hdc, rc.left, rc.top, 1, rc.bottom - rc.top, win32::PATCOPY);
    win32::SelectObject(hdc, hbrButtonHilite);
    win32::PatBlt(hdc, rc.left, rc.bottom - 1, rc.right - rc.left, 1, win32::PATCOPY);
    win32::PatBlt(hdc, rc.right - 1, rc.top, 1, rc.bottom - rc.top, win32::PATCOPY);
    ExpandRc(&rc, -(dyArial8 / 2), -(dyArial8 / 2));
    yTop = rc.top;
    win32::FillRect(hdc, &rc, hbrButtonFace);
    win32::SetTextColor(hdc, crButtonText);
    for (didt = 0; didt < 8; didt++) {
        cch = CchTutorString(rgch, static_cast<TutorId>(tutor.idt + didt));
        if (cch == 1)
            break;
        fPara = isupper(rgch[0]);
        if (fPara) {
            xLeft = rc.left;
            if (didt != 0) {
                yTop += dyArial8 / 2;
            }
        }
        if (tutor.idt + didt == tutor.idtBold) {
            win32::SetTextColor(hdc, crButtonFace);
            win32::SetBkColor(hdc, crButtonText);
        }
        WrapTextOut(hdc, &xLeft, &yTop, rgch, cch, rc.left, rc.right - rc.left, NULL, didt != 0 && fPara != 0, 1);
        if (tutor.idt + didt == tutor.idtBold) {
            win32::SetTextColor(hdc, crButtonText);
            win32::SetBkColor(hdc, crButtonFace);
        }
    }
    win32::EndPaint(hwnd, &ps);
    return;
}

void StartTutor(int16_t fRestart) {
    int16_t cx;
    int16_t cch;

    if (!gd.fTutorial) {
        memset(&tutor, 0, sizeof(TUTOR));
        if (!lpfnTutorDlgProc) {
            lpfnTutorDlgProc = (TutorDlg);
            if (!lpfnTutorDlgProc) {
                AlertIO().pfnAlertSz(TextIO().pfnFormat(idsMemory, NULL), win32::MB_ICONHAND);
                return;
            }
        }
        gd.fTutorial = 1;
        SaveGameState();
        if (!fFreeingTitle) {
            fFreeingTitle = 1;
            ControlIO().pfnDestroyWindow(hwndTitle);
            hwndTitle = 0;
            ui::ShowWindow(hwndFrame, win32::SW_SHOW);
        }
        grbitScan = static_cast<GrbitScan>(grbitScanCoverage | grbitScanMineFields | grbitScanFleetPaths | grbitScanPlanetNames);
        cx = win32::GetSystemMetrics(win32::SM_CXSCREEN);
        if (cx >= 1280) {
            iScanZoom = zoom200;
        } else if (cx >= 1024) {
            iScanZoom = zoom150;
        } else if (cx >= 800) {
            iScanZoom = zoom125;
        } else {
            iScanZoom = zoom100;
        }
        if (game.lid == 0) {
            cch = TextIO().pfnCchGetString(idsTutorial, szBase);
            if (!fRestart) {
                strcat(szBase, ".xy");
                if (FFileExists(szBase) &&
                    AlertIO().pfnAlertSz(TextIO().pfnFormat(idsTutorialHasRunBeforeWouldLikeDestroy, NULL), win32::MB_YESNO | win32::MB_ICONQUESTION | win32::MB_TASKMODAL) != win32::IDYES) {
                    szBase[cch] = 0;
                    strcat(szBase, ".m1");
                    ini.fStartupFile = 1;
                    if (FOpenGame(hwndFrame, 0) > 0) {
                        CreateChildWindows();
                    }
                    ini.fStartupFile = 0;
                }
                szBase[cch] = 0;
            }
            if (game.lid == 0) {
                CreateTutorWorld();
                memset((uint8_t *)(ZIPPRODQ *)vrgZipProd + 14, 0, 26);
                vrgZipProd[0].fValid = 1;
                gd.fChgZipProd = 1;
            }
            InitializeMenu(NULL);
            win32::PostMessageA(hwndFrame, win32::WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
            if (!fFreeingTitle) {
                fFreeingTitle = 1;
                ControlIO().pfnDestroyWindow(hwndTitle);
                hwndTitle = 0;
            }
            ui::ShowWindow(hwndFrame, win32::SW_SHOW);
        }
        tutor.idsError = -1;
        tutor.fShowHidMsg = 1;
        tutor.idt = idtWelcomeStarsTutorialWillGuideThrough36;
        tutor.idtBold = idtWelcomeStarsTutorialWillGuideThrough36;
        tutor.fProgress = 0;
        while (FTutorTaskDone() && !tutor.fTurnDone) {
            tutor.idt = static_cast<TutorId>(tutor.idt + 8);
        }
        if (tutor.fTutorDone) {
            EndTutor(0);
            return;
        }
        if (!tutor.hwnd) {
            ui::CreateDialog(hInst, win32::MAKEINTRESOURCE(IDD_TUTOR), hwndFrame, lpfnTutorDlgProc);
        }
        if (tutor.idt != idtWelcomeStarsTutorialWillGuideThrough36) {
            if (!tutor.fTurnDone) {
                tutor.idt = static_cast<TutorId>(tutor.idt - 8);
            }
            tutor.idtBold = tutor.idt;
            PresentationIO().pfnTutor();
        }
    }
    ShowTutor(1);
    ControlIO().pfnInvalidateRect(tutor.hwnd, NULL, 1);
    return;
}

void AdvanceTutor() {
    char    szTitle[50];
    int16_t fRedraw;
    int16_t idtT;
    int16_t fTaskDone;
    RECT    rc;

    fRedraw = 0;
    tutor.fChange = 0;
    idtT = tutor.idtBold;
    fTaskDone = FTutorTaskDone();
    fRedraw = idtT != tutor.idtBold;
    if (!fTaskDone) {
        if (fRedraw)
            goto LUpdatePage;
        return;
    }
    if (!tutor.fTurnDone) {
    SkipToNext:
        tutor.idt = static_cast<TutorId>(tutor.idt + 8);
        tutor.idtBold = tutor.idt;
        tutor.idsError = -1;
        tutor.fNoErrors = 1;
        tutor.fProgress = 0;
        if (FTutorTaskDone() && !tutor.fTurnDone)
            goto SkipToNext;
        fRedraw = 1;
        tutor.fNoErrors = 0;
    } else {
        tutor.idh = static_cast<HelpContextId>(3510);
    }
    if ((int16_t)tutor.idt >= 640 || tutor.fTutorDone) {
        if (tutor.idsError != 522) {
            TutorError(idsTutorialFinishedCanContinuePlayGameStart);
        }
        EndTutor(0);
        return;
    }
    if (!fRedraw) {
        return;
    }
LUpdatePage:
    win32::wsprintfA(szTitle, TextIO().pfnCompressed(idsStarsTutorPageD80), (int16_t)tutor.idt / 8 + 1);
    ControlIO().pfnSetWindowText(tutor.hwnd, szTitle);
    ShowTutor(1);
    win32::GetWindowRect(tutor.hwnd, &rc);
    win32::ScreenToClient(tutor.hwnd, (POINT *)&rc);
    win32::ScreenToClient(tutor.hwnd, (POINT *)&rc.right);
    ExpandRc(&rc, -dyArial8, -2 * dyArial8);
    ControlIO().pfnInvalidateRect(tutor.hwnd, &rc, 1);
    return;
}

void EndTutor(int16_t fClose) {
    if (gd.fTutorial) {
        gd.fTutorial = 0;
        if (tutor.hwnd) {
            ControlIO().pfnDestroyWindow(tutor.hwnd);
        }
        game.fTutorial = 0;
        if (fClose) {
            RestoreGameState();
        } else {
            tutor.fFreeing = 1;
        }
        memset(&tutor, 0, sizeof(TUTOR));
        Randomize2(win32::GetTickCount());
    }
    return;
}

void SaveGameState() {
    HMENU hmenu;

    tutor.fGameSaved = 0;
    tutor.grbitScan = grbitScan;
    tutor.iScanZoom = iScanZoom;
    tutor.fTBVis = gd.fToolbar;
    tutor.zpq = vrgZipProd[0].zpq1();
    tutor.fValidQ = vrgZipProd[0].fValid;
    vrgZipProd[0].SetZpq1(vrgZipProd[4].zpq1());
    vrgZipProd[0].fValid = vrgZipProd[4].fValid;
    if (!gd.fToolbar) {
        hmenu = GetASubMenu(hwndFrame, menuView);
        gd.fToolbar = gd.fToolbar == 0;
        win32::CheckMenuItem(hmenu, IDM_VIEW_TOOLBAR, !gd.fToolbar ? win32::MF_UNCHECKED : win32::MF_CHECKED);
        ControlIO().pfnRefitFrameChildren();
    }
    tutor.icolFSort = vrptFleet.icolSort;
    if (vrptFleet.icolSort != 1 || !vrptFleet.fAscending) {
        vrptFleet.icolSort = 1;
        vrptFleet.fAscending = 1;
        PresentationIO().pfnInvalidateReport(rptFleets, 0);
    }
    if (game.lid != 0 && !game.fTutorial) {
        DestroyCurGame();
    }
    return;
}

void RestoreGameState() {
    HMENU hmenu;

    if (tutor.fGameSaved) {
    }
    grbitScan = static_cast<GrbitScan>(tutor.grbitScan);
    iScanZoom = tutor.iScanZoom;
    vrgZipProd[4].SetZpq1(vrgZipProd[0].zpq1());
    vrgZipProd[4].fValid = vrgZipProd[0].fValid;
    vrgZipProd[0].SetZpq1(tutor.zpq);
    vrgZipProd[0].fValid = tutor.fValidQ;
    if (gd.fToolbar != tutor.fTBVis) {
        hmenu = GetASubMenu(hwndFrame, menuView);
        gd.fToolbar = gd.fToolbar == 0;
        win32::CheckMenuItem(hmenu, IDM_VIEW_TOOLBAR, !gd.fToolbar ? win32::MF_UNCHECKED : win32::MF_CHECKED);
        ControlIO().pfnRefitFrameChildren();
    }
    if (vrptFleet.icolSort != tutor.icolFSort) {
        vrptFleet.icolSort = tutor.icolFSort;
        PresentationIO().pfnInvalidateReport(rptFleets, 1);
    }
    return;
}

int16_t FAskKillTutor() {
    if (game.turn >= 30 || AlertIO().pfnAlertSz(TextIO().pfnFormat(idsCurrentlyRunningStarsTutorialDoWantExit, NULL), win32::MB_YESNO | win32::MB_ICONQUESTION | win32::MB_TASKMODAL) == win32::IDYES) {
        EndTutor(1);
        return 1;
    }
    return 0;
}

int16_t FTutorTaskDone() {
    HS hs1;
    HS hs;
    HS hs2;

    switch (game.turn) {
    case 0:
        switch (tutor.idt) {
        case idtWelcomeStarsTutorialWillGuideThrough36:
            tutor.idtBold = idtReadMessages;
            return FCheckMessages(9999, static_cast<MessageId>(idmNone), 0);
        case idtExamineTilesCommandPaneUpperLeftPortion:
            tutor.idtBold = idtPressTilesGotoButtonCommandArmedProbe;
            if (!FCheckSelection(grobjFleet, 0)) {
                return 0;
            }
            tutor.idtBold = idtHoldShiftKeyClickLeftMouseButton;
            return FCheckFleetWP(0, 1, grobjPlanet, 12, grTaskNone, iWarpAny);
        case idtAccordingFleetWaypointsTileWillTake2:
            tutor.idtBold = idtHitNKeyLookFleet;
            if (!FCheckSelection(grobjFleet, 1)) {
                tutor.idh = idhKeyboardShortcuts;
                return 0;
            }
            tutor.idtBold = idtHoldShiftKeyLeftClickPlanet90210;
            return FCheckFleetWP(1, 1, grobjPlanet, 16, grTaskNone, iWarpAny);
        case idtLetsMoveOurFleet:
            if (FCheckSelection(grobjFleet, 4)) {
                tutor.idtBold = idtHoldShiftKeySelectAlexander;
                return FCheckFleetWP(4, 1, grobjPlanet, 15, grTaskNone, iWarpAny);
            }
            if (FCheckSelection(grobjFleet, 3)) {
                tutor.idtBold = idtPress2;
            } else if (FCheckSelection(grobjFleet, 2)) {
                tutor.idtBold = idtPress;
            } else {
                tutor.idtBold = idtTimePressButtonTileShowingLongRange;
            }
            return 0;
        case idtHitNKey:
            if (FCheckSelection(grobjFleet, 4) && !tutor.fProgress && !tutor.fAutoComplete) {
                tutor.idtBold = idtHitNKey;
            } else if (FCheckSelection(grobjFleet, 5) && !tutor.fProgress && !tutor.fAutoComplete) {
                tutor.idtBold = idtHitNKey2;
            } else {
                if (FCheckResearch(Weapons, TechFieldCount, 15)) {
                    tutor.idtBold = idtThatsTurnHitF9GenerateYear;
                    tutor.fTurnDone = 1;
                    return 1;
                }
                tutor.idtBold = pctResGlob == -1 ? idtChooseResearchCommandsMenu : idtChangeFieldStudyWeaponsPressDone;
            }
            return 0;
        default:
            return 0;
        }
    case 1:
        if (tutor.idt != idtReadMessageMessagesPaneWeveGotPlenty) {
            return 1;
        }
        if (!FCheckQueue(13, 0, grobjPlanet, mdIdleFactory, 20, 0)) {
            tutor.idtBold = !lpplProdGlob ? idtPressChangeButtonProductionTile : idtSelectFactoryLeftHandListboxHoldShift;
            return 0;
        }
        tutor.idtBold = idtHitF9KeyGenerateYear;
        tutor.fTurnDone = 1;
        return 1;
    case 2:
        switch (tutor.idt) {
        case idtReadFirstMessagePressGotoMessagesPane:
            if (FCheckFleetWP(1, 1, grobjPlanet, 21, grTaskNone, iWarpAny)) {
                return 1;
            }
            tutor.idtBold = !FCheckSelection(grobjFleet, 0) ? idtReadFirstMessagePressGotoMessagesPane : idtHoldShiftKeyLeftClickHiho;
            if (!FCheckFleetWP(0, 1, grobjPlanet, 9, grTaskNone, iWarpAny)) {
                return 0;
            }
            tutor.idtBold = idtVacancy;
            if (!FCheckFleetWP(0, 2, grobjPlanet, 3, grTaskNone, iWarpAny)) {
                return 0;
            }
            tutor.idtBold = idtSlime;
            if (!FCheckFleetWP(0, 3, grobjPlanet, 8, grTaskNone, iWarpAny)) {
                return 0;
            }
            tutor.idtBold = idtWallaby;
            if (!FCheckFleetWP(0, 4, grobjPlanet, 5, grTaskNone, iWarpAny)) {
                return 0;
            }
            tutor.idtBold = idtOxygen;
            if (!FCheckFleetWP(0, 5, grobjPlanet, 2, grTaskNone, iWarpAny)) {
                return 0;
            }
            tutor.idtBold = idtReadMessagePressGotoCommandLongRange;
            return FCheckSelection(grobjFleet, 1);
        case idtWeWantSendFleetExploreAreaAbove:
            if (FCheckFleetWP(4, 1, grobjPlanet, 14, grTaskNone, iWarpAny)) {
                return 1;
            }
            tutor.idtBold = idtHoldShiftKeySelectDwarte;
            if (!FCheckFleetWP(1, 1, grobjPlanet, 21, grTaskNone, iWarpAny)) {
                return 0;
            }
            tutor.idtBold = idtMobius;
            if (!FCheckFleetWP(1, 2, grobjPlanet, 19, grTaskNone, iWarpAny)) {
                return 0;
            }
            tutor.idtBold = idtCastle;
            if (!FCheckFleetWP(1, 3, grobjPlanet, 20, grTaskNone, iWarpAny)) {
                return 0;
            }
            tutor.idtBold = idtMoholdi;
            if (!FCheckFleetWP(1, 4, grobjPlanet, 7, grTaskNone, iWarpAny)) {
                return 0;
            }
            tutor.idtBold = !FCheckMessages(2, static_cast<MessageId>(idmNone), 0) ? idtReadMessage : idtGotoStalwartDefender5;
            return FCheckSelection(grobjFleet, 4);
        case idtHoldShiftKeySelectShaggyDog:
            if (FCheckFleetWP(5, 1, grobjPlanet, 12, grTaskMine, iWarpAny)) {
                return 1;
            }
            tutor.idtBold = idtHoldShiftKeySelectShaggyDog;
            if (!FCheckFleetWP(4, 1, grobjPlanet, 14, grTaskNone, iWarpAny)) {
                return 0;
            }
            tutor.idtBold = idtSeaSquared;
            if (!FCheckFleetWP(4, 2, grobjPlanet, 17, grTaskNone, iWarpAny)) {
                return 0;
            }
            tutor.idtBold = idtRedStorm;
            if (!FCheckFleetWP(4, 3, grobjPlanet, 18, grTaskNone, iWarpAny)) {
                return 0;
            }
            tutor.idtBold = idtBloop;
            if (!FCheckFleetWP(4, 4, grobjPlanet, 23, grTaskNone, iWarpAny)) {
                return 0;
            }
            tutor.idtBold = idtKalamazoo;
            if (!FCheckFleetWP(4, 5, grobjPlanet, 22, grTaskNone, iWarpAny)) {
                return 0;
            }
            tutor.idtBold = !FCheckMessages(4, static_cast<MessageId>(idmNone), 0) ? idtReadTwoMessages : idtPressGotoDisplayStatsPruneSummaryPane;
            return FCheckSummary(grobjPlanet, 12);
        case idtTopGraphSummaryPaneShowsPruneHas:
            if (FCheckColonizeWP(2, 16, iWarpAny)) {
                return 1;
            }
            if (FCheckFleetWP(5, 1, grobjPlanet, 12, grTaskMine, iWarpAny)) {
                tutor.fProgress = 0;
                return 1;
            }
            if (FCheckFleetWP(5, 1, grobjPlanet, 12, grTaskNone, iWarpAny)) {
                tutor.idtBold = idtClickDropdownChangeTaskRemoteMining;
            } else if (FCheckSelection(grobjFleet, 5)) {
                tutor.idtBold = idtShiftClickPrune;
            } else {
                tutor.idtBold = idtClickRightMouseButtonStoveTopSelect;
            }
            return 0;
        case idtMoveMessageGotoAlexander:
            if (FCheckColonizeWP(2, 16, iWarpAny)) {
                return 1;
            }
            if (FCheckSummary(grobjPlanet, 16)) {
                return 1;
            }
            if (FCheckSummary(grobjPlanet, 15)) {
                if (FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
                    tutor.idtBold = idtGotoPlanet90210;
                } else {
                    tutor.idtBold = !tutor.fProgress ? idtClickVariousPlacesSummaryPaneGetPopup : idtReadMessage2;
                }
            }
            return 0;
        case idtSince90210FinePlanetHighMineralConcentrations:
            if (!FCheckCargo(LpflFromId(2), 0, 0, 0, 25)) {
                if (FCheckSelection(grobjFleet, 2)) {
                    if (mdXferDlg == mdXferNone) {
                        tutor.idh = idhLocationTile;
                        tutor.idtBold = idtClickXferButtonTileLabeledOrbitingStove;
                    } else {
                        tutor.idtBold = idtClickDragColonistsGaugeFillingHold25kt;
                    }
                } else {
                    tutor.idtBold = idtRightClickStoveTopSelectSantaMaria;
                }
                return 0;
            }
            if (!FCheckColonizeWP(2, 16, iWarpAny)) {
                if (!FCheckFleetWP(2, 1, grobjPlanet, 16, grTaskNone, iWarpAny)) {
                    tutor.idtBold = !FCheckSelection(grobjFleet, 2) ? idtRightClickStoveTopSelectSantaMaria : idtShiftClick90210;
                } else {
                    tutor.idtBold = idtSelectColonizeDropdownWaypointTaskTile;
                }
                return 0;
            }
            tutor.idtBold = idtHitF9GenerateYear;
            tutor.fTurnDone = 1;
            return 1;
        default:
            return 1;
        }
    case 3:
        switch (tutor.idt) {
        default:
            return 1;
        case idtFirstMessageQuiteCommonWeDontNeed:
            if (!FCheckMessages(imsgAny, idmHaveBuiltFactories, 1)) {
                tutor.idtBold = idtFilterClickingBlueCheckMarkUpperLeft;
                return 0;
            }
            if (!FCheckQueue(13, 0, grobjPlanet, iobjFactory, 30, 0)) {
                if (!FCheckMessages(1, static_cast<MessageId>(idmNone), 0) || !FCheckSelection(grobjPlanet, 13)) {
                    tutor.idtBold = idtMoveMessageGotoStoveTop;
                } else {
                    tutor.idtBold = !lpplProdGlob ? idtPressChangeButtonProductionTile2 : idtSelectFactoriesAutoBuildLeftHandListbox;
                }
                return 0;
            }
            return 1;
        case idtReadTwoMessagesGoto90210:
            if (!FCheckQueue(16, 0, grobjPlanet, mdIdleFactory, 3, 1) || !FCheckQueue(16, 1, grobjPlanet, mdIdleMine, 3, 1)) {
                if (!FCheckMessages(3, static_cast<MessageId>(idmNone), 0) || !FCheckSelection(grobjPlanet, 16)) {
                    tutor.idtBold = idtReadTwoMessagesGoto90210;
                } else {
                    tutor.idtBold = !lpplProdGlob ? idtHitQKey : idtDoubleClickFactory3TimesMine3;
                }
                return 0;
            }
            if (!FCheckCargo(LpflFromId(3), 0, 0, 0, 210)) {
                if (FCheckSelection(grobjFleet, 3)) {
                    tutor.idtBold = mdXferDlg == mdXferNone ? idtClickXferButtonCommandPane : idtFillHoldColonistsHitOk;
                } else {
                    tutor.idtBold = idtRightClickStoveTopSelectTeamster4;
                }
                return 0;
            }
            return 1;
        case idtShiftClick902102:
            if (!FCheckXferWP(3, 1, 16, iWarpAny, rgiaQuikDrop)) {
                if (!FCheckFleetWP(3, 1, grobjPlanet, 16, grTaskXfer, iWarpAny)) {
                    if (!FCheckFleetWP(3, 1, grobjPlanet, 16, grTaskAny, iWarpAny)) {
                        tutor.idtBold = idtShiftClick902102;
                    } else {
                        tutor.idtBold = idtChangeWaypointTaskTransport;
                    }
                } else {
                    tutor.idtBold = idtRightClickBlueDiamondWaypointTaskTile;
                }
                return 0;
            }
            if (LpflFromId(0)->cord < 6) {
                return 1;
            }
            if (FCheckSelection(grobjFleet, 0)) {
                return 1;
            }
            tutor.idtBold = !FCheckSummary(grobjPlanet, 9) ? idtReadMessageGotoHiho : idtDoubleClickArmedProbe1;
            return 0;
        case idtArmedProbe1DoesntNeedGoWay:
            if (LpflFromId(0)->cord == 6) {
                tutor.idtBold = !FCheckSummary(grobjPlanet, 9) ? idtClickHiho : idtHitDeleteKey;
                return 0;
            }
            tutor.idtBold = idtSelectGenerateTurnMenu;
            tutor.fTurnDone = 1;
            tutor.fProgress = 0;
            return 1;
        }
    case 4:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoShaggyDog:
            if (LpflFromId(4)->cord == 6) {
                if (FCheckSelection(grobjFleet, 4)) {
                    tutor.idtBold = !FCheckSummary(grobjPlanet, 14) ? idtSelectWaypointShaggyDog : idtPressDeleteKey;
                } else if (FCheckSummary(grobjPlanet, 14) || FCheckSummary(grobjFleet, 4)) {
                    tutor.idtBold = idtDoubleClickStalwartDefender5JustAbove;
                } else {
                    tutor.idtBold = idtReadFirstMessageGotoShaggyDog;
                }
                return 0;
            }
            if (LpplFromId(13)->lpplprod->iprodMac == 3) {
                return 1;
            }
            if (FCheckSelection(grobjPlanet, 13)) {
                return 1;
            }
            tutor.idtBold = !FCheckSummary(grobjPlanet, 21) ? idtReadMessageGotoDwarte : idtDoubleClickStoveTop;
            return 0;
        case idtHitChangeButtonProductionTileOpenStove:
            if (LpplFromId(13)->lpplprod->iprodMac != 3 || !FCheckQueue(13, 1, grobjFleet, 2, 1, 0)) {
                tutor.idtBold = !lpplProdGlob ? idtHitChangeButtonProductionTileOpenStove : idtDoubleClickSantaMariaLeftHandListbox;
                return 0;
            }
            tutor.idtBold = idtGenerateWhenReady;
            tutor.fTurnDone = 1;
            return 1;
        }
    case 5:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoNewSantaMaria:
            if (FCheckCargo(LpflFromId(2), 0, 0, 0, 25)) {
                if (!FCheckFleetWP(2, 1, grobjPlanet, 14, grTaskAny, iWarpAny)) {
                    tutor.idtBold = !FCheckScanner(3, iZoomAny) ? idtClickButtonToolbarShowPlanetsHowHabitable : idtShiftClickBigGreenShaggyDogBelow;
                    return 0;
                }
                return 1;
            }
            if (!FCheckSelection(grobjFleet, 2)) {
                tutor.idtBold = idtReadFirstMessageGotoNewSantaMaria;
            } else {
                tutor.idtBold = mdXferDlg == mdXferNone ? idtClickCargoGaugeFuelCargoTile : idtFillHoldFullColonistsHitOk;
            }
            return 0;
        case idtSetWaypointTaskColonize:
            if (!FCheckColonizeWP(2, 14, iWarpAny)) {
                tutor.idtBold = idtSetWaypointTaskColonize;
                return 0;
            }
            if (!FCheckFleetWP(3, 1, grobjPlanet, 13, grTaskAny, iWarpAny)) {
                if (!FCheckScanner(0, iZoomAny)) {
                    tutor.idtBold = idtSwitchScannerBackNormalViewClickingLeftmost;
                } else {
                    tutor.idtBold = !FCheckSelection(grobjFleet, 3) ? idtRead2MessagesGotoTeamster4 : idtShiftClickStoveTopSendHome;
                }
                return 0;
            }
            if (LpflFromId(0)->cord == 5) {
                if (FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
                    tutor.idtBold = idtReadMessageDeleteArmedProbe1sWaypoint;
                } else {
                    tutor.idtBold = !FCheckSelection(grobjPlanet, 16) ? idtSelect90210PressingGotoButtonTileLabeled : idtReadMessageDeleteArmedProbe1sWaypoint;
                }
                return 0;
            }
            tutor.idtBold = idtThatsYearGenerateWhenReady;
            tutor.fTurnDone = 1;
            return 1;
        }
    case 6:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoTeamster4:
            if (!FCheckXferWP(3, 1, 12, iWarpAny, rgiaQuikLoad)) {
                if (!FCheckFleetWP(3, 1, grobjPlanet, 12, grTaskXfer, iWarpAny)) {
                    if (!FCheckFleetWP(3, 1, grobjPlanet, 12, grTaskAny, iWarpAny)) {
                        tutor.idtBold = !FCheckSelection(grobjFleet, 3) ? idtReadFirstMessageGotoTeamster4 : idtShiftClickPrune2;
                    } else {
                        tutor.idtBold = idtSetWaypointTaskTransport;
                    }
                } else {
                    tutor.idtBold = idtRightClickBlueDiamondSelectQuikloadZip;
                }
                return 0;
            }
            tutor.idtBold = idtShiftClickBackStoveTop;
            return FCheckFleetWP(3, 2, grobjPlanet, 13, grTaskAny, iWarpAny);
        case idtNoticeWaypointTaskHasCopiedPreviousWaypoint:
            tutor.fNoErrors = 1;
            if (!FCheckXferWP(3, 2, 13, iWarpAny, rgiaQuikDrop)) {
                tutor.fNoErrors = 0;
                tutor.idtBold = idtRightClickBlueDiamondSelectQuikdropZip;
                return 0;
            }
            tutor.fNoErrors = 0;
            tutor.idh = idhFleetWaypointsTile;
            tutor.idtBold = idtClickRepeatOrdersCheckboxFleetWaypointsTile;
            if (!LpflFromId(3)->fRepOrders) {
                return 0;
            }
            tutor.idh = idhProductionDialog;
            if (LpplFromId(13)->lpplprod->iprodMac != 3 || !FCheckQueue(13, 1, grobjFleet, 2, 1, 0)) {
                tutor.idtBold = !FCheckSelection(grobjPlanet, 13) ? idtSelectStoveTop : idtAddSantaMariaProductionQueue;
                return 0;
            }
            tutor.idtBold = idtGoAheadGenerateIDare;
            tutor.fTurnDone = 1;
            return 1;
        }
    case 7:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoNewSantaMaria2:
            if (!FCheckCargo(LpflFromId(6), 0, 0, 0, 25)) {
                tutor.idtBold = idtReadFirstMessageGotoNewSantaMaria2;
                return 0;
            }
            if (!FCheckColonizeWP(6, 18, iWarpAny)) {
                tutor.idtBold = !FCheckScanner(3, iZoomAny) ? idtClickToolbarButtonPutScannerPlanetValue : idtGiveSantaMaria7ColonizeTaskRed;
                return 0;
            }
            tutor.idh = idhProductionTile;
            if (LpplFromId(13)->lpplprod->iprodMac < 3 || !FCheckQueue(13, 1, grobjFleet, 2, 3, 0)) {
                tutor.idtBold = idtAddThreeSantaMariasStoveTopsProduction;
                return 0;
            }
            if (!FCheckScanner(0, iZoomAny)) {
                tutor.idtBold = idtPressLeftmostToolbarButtonPutScannerBack;
                return 0;
            }
            return 1;
        case idtReadMessage3:
            if (!FCheckMessages(imsgAny, idmHaveBuiltMines, 1)) {
                tutor.idtBold = !FCheckMessages(3, static_cast<MessageId>(idmNone), 0) ? idtReadMessage3 : idtFilterThemClickingBlueCheckMarkMessages;
                return 0;
            }
            if (!FCheckQueue(16, 0, grobjPlanet, iobjFactory, 10, 1) || !FCheckQueue(16, 1, grobjPlanet, iobjMine, 10, 1)) {
                if (!FCheckSelection(grobjPlanet, 16)) {
                    tutor.idtBold = !FCheckMessages(4, static_cast<MessageId>(idmNone), 0) ? idtGoMessage : idtGoto90210OpenProductionQueue;
                } else {
                    tutor.idtBold = !lpplProdGlob ? idtGoto90210OpenProductionQueue : idtShiftDoubleClickFactoriesAutoBuildMines;
                }
                return 0;
            }
            tutor.idtBold = idtReadRestMessages;
            tutor.fProgress = 0;
            return FCheckMessages(9999, static_cast<MessageId>(idmNone), 0);
        case idtClickRedTriangleBetweenSlimeVacancy:
            if (!tutor.fProgress && !tutor.fAutoComplete) {
                if (FCheckSummary(grobjFleet, 512)) {
                    tutor.fProgress = 1;
                } else {
                    tutor.idtBold = idtClickRedTriangleBetweenSlimeVacancy;
                    return 0;
                }
            }
            tutor.idh = idhProductionTile;
            if (LpplFromId(13)->lpplprod->iprodMac < 4 || !FCheckQueue(13, 2, grobjFleet, 0, 2, 0)) {
                tutor.idtBold = idtAddTwoArmedProbesStoveTopsQueue;
                return 0;
            }
            tutor.idtBold = idtGenerateWhenReady2;
            tutor.fTurnDone = 1;
            return 1;
        }
    case 8:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoNewColonyShips:
            if (rgplr[0].cFleet == 11 && FCheckColonizeWP(10, 8, iWarpAny)) {
                return 1;
            }
            if (rgplr[0].cFleet == 9 || mdXferDlg == mdXferShips) {
                if (!FCheckSelection(grobjFleet, 7)) {
                    tutor.idtBold = idtReadFirstMessageGotoNewColonyShips;
                } else {
                    tutor.idtBold = mdXferDlg == mdXferShips ? idtMoveOneSantaMariasFleet10Hit : idtHitSplitButtonFleetCompositionTile;
                }
            } else if (!FCheckCargo(LpflFromId(7), 0, 0, 0, 50) || !FCheckFleetWP(7, 1, grobjPlanet, 8, grTaskColonize, iWarpAny)) {
                tutor.idtBold = idtLoadFleetColonistsGiveColonizeTaskSlime;
            } else {
                tutor.idtBold = idtWeDontWantBothColonizersGoSlime;
            }
            return 0;
        case idtNoticeFleetHasOneSantaMariaOther:
            tutor.fNoErrors = 1;
            if (!FCheckColonizeWP(7, 17, iWarpAny)) {
                tutor.fNoErrors = 0;
                FCheckColonizeWP(7, 8, iWarpAny);
                tutor.idtBold = idtClickWaypointSlimeDragSeaSquared;
                return 0;
            }
            tutor.fNoErrors = 0;
            if (!FCheckFleetWP(8, 1, grobjPlanet, 9, grTaskAny, iWarpAny)) {
                tutor.idtBold = !FCheckSelection(grobjFleet, 8) ? idtReadMessageGotoNewArmedScouts : idtLetsTryHeadThemOffPass;
                return 0;
            }
            tutor.fProgress = 0;
            return 1;
        case idtReadMessageFilter:
            tutor.fNoErrors = 0;
            if (!FCheckMessages(imsgAny, idmHasUnloaded, 1)) {
                tutor.fProgress = 0;
                tutor.idtBold = idtReadMessageFilter;
                return 0;
            }
            tutor.fNoErrors = 1;
            if (FCheckResearch(Weapons, TechFieldCount, 30)) {
                tutor.fNoErrors = 0;
                return 1;
            }
            if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtReadLastMessageGotoWallaby;
                tutor.fProgress = 0;
                return 0;
            }
            if (!tutor.fProgress && !tutor.fAutoComplete) {
                if (FCheckSummary(grobjPlanet, 5)) {
                    tutor.idtBold = idtClickGreenRadiationBarSummaryPaneRead;
                } else {
                    tutor.idtBold = idtReadLastMessageGotoWallaby;
                }
                return 0;
            }
            tutor.idtBold = idtHitF5OpenResearchDialog;
            if (pctResGlob != -1) {
                tutor.fProgress = 0;
                return 1;
            }
            return 0;
        case idtRightRadiationTerraform7OneExpectedBenefits:
            if (pctResGlob != -1) {
                if (!tutor.fProgress) {
                    tutor.idtBold = idtClickWordRadiationDialogSeeRequirements;
                } else {
                    tutor.idtBold = idtIncreaseResourcesBudgetedResearch30HitDone;
                }
                return 0;
            }
            if (!FCheckResearch(Weapons, TechFieldCount, 30)) {
                tutor.idtBold = idtIncreaseResourcesBudgetedResearch30HitDone;
                return 0;
            }
            tutor.idtBold = idtGenerateWhenReady3;
            tutor.fTurnDone = 1;
            return 1;
        }
    case 9:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoResearchDialog:
            tutor.fNoErrors = 1;
            if (!FCheckResearch(Weapons, Construction, 30)) {
                tutor.idtBold = pctResGlob == -1 ? idtReadFirstMessageGotoResearchDialog : idtChangeFieldResearchConstructionHitDone;
                tutor.fNoErrors = 0;
                return 0;
            }
            tutor.fNoErrors = 0;
            tutor.idtBold = idtReadMessageFilter2;
            return FCheckMessages(imsgAny, idmHasLoadedMiningRobotsWorking, 1);
        case idtReadMessageGotoOxygen:
            if (!FCheckColonizeWP(9, 2, iWarpAny)) {
                if (FCheckSelection(grobjFleet, 9)) {
                    tutor.idtBold = !FCheckCargo(LpflFromId(9), 0, 0, 0, 25) ? idtLoadColonists : idtSendColonizeOxygen;
                } else {
                    tutor.idtBold = !FCheckSummary(grobjPlanet, 2) ? idtReadMessageGotoOxygen : idtRightClickStoveTopSelectSantaMaria2;
                }
                return 0;
            }
            tutor.fNoErrors = 1;
            if (!FCheckFleetWP(0, 1, grobjPlanet, 4, grTaskAny, iWarpAny)) {
                FCheckFleetWP(0, 1, grobjPlanet, 2, grTaskAny, iWarpAny);
                tutor.idtBold = idtSelectArmedProbe1DragWaypointOxygen;
                tutor.fNoErrors = 0;
                return 0;
            }
            tutor.fNoErrors = 0;
            tutor.idtBold = idtGenerateWhenReady4;
            tutor.fTurnDone = 1;
            return 1;
        }
    case 10:
        switch (tutor.idt) {
        default:
            return 1;
        case idtFilterMessageAboutDismantlingColonizer:
            if (!FCheckMessages(imsgAny, idmHasDismantledKtMineralsWhichHaveDeposited, 1)) {
                tutor.idtBold = idtFilterMessageAboutDismantlingColonizer;
                tutor.fProgress = 0;
                return 0;
            }
            if (!FCheckQueue(14, 0, grobjPlanet, iobjFactory, 3, 1) || !FCheckQueue(14, 1, grobjPlanet, iobjMine, 3, 1)) {
                if (FCheckSelection(grobjPlanet, 14)) {
                    tutor.idtBold = !lpplProdGlob ? idtOpenShaggyDogsProductionQueue : idtAdd3FactoriesAutoBuild3Mines;
                } else {
                    tutor.idtBold = idtReadMessageGotoShaggyDog;
                }
                return 0;
            }
            if (FCheckTemplate(0)) {
                return 1;
            }
            if (!lpplProdGlob) {
                tutor.idtBold = idtOpenProductionQueue;
                return 0;
            }
            if (vyZPDStatic != -1) {
                return 1;
            }
            tutor.idtBold = idtRightClickBlueDiamondSelectCustomize;
            return 0;
        case idtHitImportButtonCopyShaggyDogsQueue:
            if (!FCheckTemplate(0)) {
                tutor.idtBold = idtHitImportButtonCopyShaggyDogsQueue;
                return 0;
            }
            if (!tutor.fProgress && lpplProdGlob) {
                tutor.idtBold = idtOkProductionDialog;
                return 0;
            }
            if (LpplFromId(13)->lpplprod->iprodMac < 3 || !FCheckQueue(13, 1, grobjFleet, 2, 1, 0)) {
                tutor.idh = idhProductionDialog;
                if (!tutor.fProgress && !FCheckSelection(grobjPlanet, 13)) {
                    if (FCheckSummary(grobjPlanet, 23)) {
                        tutor.fProgress = 1;
                        tutor.idtBold = idtAddSantaMariaStoveTopsQueue;
                    } else {
                        tutor.idtBold = idtReadMessageGotoBloopLooksLikeNice;
                    }
                } else {
                    tutor.idtBold = idtAddSantaMariaStoveTopsQueue;
                }
                return 0;
            }
            if (LpplFromId(13)->lpplprod->iprodMac < 4 || !FCheckQueue(13, 2, grobjFleet, 3, 1, 0)) {
                tutor.idh = idhProductionDialog;
                tutor.idtBold = idtAddNewTeamsterStoveTopsQueue;
                return 0;
            }
            tutor.idtBold = idtGenerateWhenReady5;
            tutor.fTurnDone = 1;
            return 1;
        }
    case 11:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoArmedProbe1:
            if (!FCheckFleetWP(0, 1, grobjPlanet, 10, grTaskAny, iWarpAny)) {
                tutor.idtBold = !FCheckSelection(grobjFleet, 0) ? idtReadFirstMessageGotoArmedProbe1 : idtShiftClickHacker;
                return 0;
            }
            if (!FCheckFleetWP(1, 1, grobjPlanet, 13, grTaskScrap, iWarpAny)) {
                tutor.idtBold = !FCheckSelection(grobjFleet, 1) ? idtReadMessageGotoLongRangeScout2 : idtShiftClickStoveTopChangeWaypointTask;
                return 0;
            }
            if (!FCheckFleetWP(4, 1, grobjPlanet, 13, grTaskAny, iWarpAny)) {
                tutor.idtBold = idtReadMessageSendStalwartDefender5Stove;
                return 0;
            }
            if (!FCheckFleetWP(8, 1, grobjFleet, 512, grTaskAny, iWarpAny)) {
                tutor.idtBold = idtReadMessageGotoArmedProbe9Shift;
                return 0;
            }
            return 1;
        case idtReadMessageGotoNewSantaMaria:
            if (!FCheckColonizeWP(2, 4, iWarpAny)) {
                tutor.idtBold = !FCheckSelection(grobjFleet, 2) ? idtReadMessageGotoNewSantaMaria : idtGiveSantaMaria3OrdersColonizeDont;
                return 0;
            }
            if (!FCheckCargo(LpflFromId(11), 0, 0, 0, 210)) {
                tutor.idtBold = !FCheckSelection(grobjFleet, 11) ? idtReadMessageGotoTeamster12 : idtLoadColonistsAssignWaypointWallaby;
                return 0;
            }
            if (FCheckFleetWP(11, 1, grobjPlanet, 5, grTaskAny, iWarpAny)) {
                if (FCheckFleetWP(11, 1, grobjPlanet, 5, grTaskXfer, iWarpAny)) {
                    if (FCheckXferWP(11, 1, 5, iWarpAny, rgiaUnloadAllCol)) {
                        tutor.fProgress = 0;
                        return 1;
                    }
                    if (FCheckSelection(grobjFleet, 11) && sel.fl.lpplord->rgord()[sel.iwpAct].grTask == grTaskXfer &&
                        ControlIO().pfnSendMessage(rghwndOrderDD[1], win32::CB_GETCURSEL, 0, 0) == 4) {
                        tutor.idtBold = idtThirdUnload;
                    } else {
                        tutor.idtBold = idtSetSecondDropdownWaypointTaskTileColonists;
                    }
                } else {
                    tutor.idtBold = idtChangeWaypointTaskTransport2;
                }
            } else {
                tutor.idtBold = idtLoadColonistsAssignWaypointWallaby;
            }
            return 0;
        case idtReadMessageAdd70MinesTopStove:
            if (LpplFromId(13)->lpplprod->iprodMac < 2 || !FCheckQueue(13, 0, grobjPlanet, mdIdleMine, 70, 0)) {
                tutor.idtBold = idtReadMessageAdd70MinesTopStove;
            } else if (!FCheckResearch(Construction, Biotechnology, 30)) {
                tutor.idtBold = pctResGlob == -1 ? idtReadMessageGotoResearchDialog : idtLeaveFieldStudyConstructionChangeFieldResearch;
            } else {
                if (FCheckSelection(grobjFleet, 8)) {
                    return 1;
                }
                if (!tutor.fProgress || !FCheckMessages(11, static_cast<MessageId>(idmNone), 0) || hwndBrowser) {
                    if (!hwndBrowser) {
                        tutor.idtBold = idtReadMessageHitGotoOpenTechnologyBrowser;
                    } else {
                        tutor.idtBold = idtWhenDoneReadingAboutBetaTorpedoClose;
                        tutor.fProgress = 1;
                    }
                } else {
                    if (!FCheckMessages(13, static_cast<MessageId>(idmNone), 0)) {
                        tutor.idtBold = idtReadTwoMessagesLookingTechBrowserIf;
                    } else {
                        tutor.idtBold = idtReadMessageGotoArmedProbe;
                    }
                }
            }
            return 0;
        case idtGollyNailedOneThemNoticeButtonNormally:
            if (!tutor.fProgress && !tutor.fAutoComplete) {
                tutor.idtBold = !vrgtok ? idtPressViewOpenBattleVcr : idtUseVcrControlsWatchPlaybackBattleHit;
                return 0;
            }
            tutor.idtBold = !FCheckMessages(9999, static_cast<MessageId>(idmNone), 0) ? idtReadRestMessages2 : idtGenerateWhenReady6;
            tutor.fTurnDone = 1;
            return 1;
        }
    case 12:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoButtonDisabledI:
            if (LpplFromId(8)->lpplprod->iprodMac > 2) {
                return 1;
            }
            if (FCheckSummary(grobjFleet, 3)) {
                return 1;
            }
            if (FCheckSummary(grobjThing, idAny)) {
                tutor.idtBold = idtSelectViewFindTypeTeamster4Hit;
            } else if (FCheckMessages(1, static_cast<MessageId>(idmNone), 0) && FCheckSelection(grobjFleet, 8)) {
                tutor.idtBold = idtRightClickArmedProbe9SelectSalvage;
            } else {
                tutor.idtBold = idtReadMessageGotoArmedProbe9;
            }
            return 0;
        case idtIfCantFindWhereYellowSelectionArrow:
            if (!FCheckMessages(5, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtReadMessage4;
                return 0;
            }
            if (!FCheckMessages(6, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtWatchSadBattleIfWantMoveMessage;
                return 0;
            }
            tutor.idtBold = idtReadMessageGotoRedStorm;
            return FCheckSelection(grobjPlanet, 18);
        case idtOtherBitShortColonistsRedStormDoing:
            if (LpplFromId(13)->lpplprod->iprodMac < 3 || !FCheckQueue(13, 1, grobjFleet, 3, 2, 0)) {
                if (LpplFromId(8)->lpplprod->iprodMac < 3 || !FCheckQueue(8, 0, grobjPlanet, mdIdleTerraform, 2, 1)) {
                    tutor.idtBold = !FCheckSelection(grobjPlanet, 8) ? idtReadMessageGotoSlime : idtOpenSlimesProductionQueueAddTwoTerraform;
                } else {
                    tutor.idtBold = idtAddTwoTeamstersStoveTopsProductionQueue;
                }
                return 0;
            }
            tutor.idtBold = idtGenerateWhenReady7;
            tutor.fTurnDone = 1;
            return 1;
        }
    case 13:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageLoadTeamster1Colonists:
            if (rgplr[0].cShDef == 7) {
                return 1;
            }
            if (!FCheckXferWP(0, 1, 8, iWarpAny, rgiaUnloadAllCol)) {
                if (!FCheckCargo(LpflFromId(0), 0, 0, 0, 210)) {
                    tutor.idtBold = idtReadFirstMessageLoadTeamster1Colonists;
                } else {
                    tutor.idtBold = idtSendSlimeOrdersUnloadThem;
                }
                tutor.fProgress = 0;
                return 0;
            }
            if (!FCheckMessages(3, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtReadMessage5;
                return 0;
            }
            if (!FCheckResearch(Biotechnology, Propulsion, 30)) {
                tutor.idtBold = idtReadMessageOpenResearchDialogChangeField;
                return 0;
            }
            if (!tutor.fProgress && !tutor.fAutoComplete) {
                if (!hwndBrowser) {
                    tutor.idtBold = idtReadMessageCheckRoboMinerTechBrowser;
                } else {
                    tutor.idtBold = idtHitF4OpenShipDesigner;
                    tutor.fProgress = 1;
                }
                return 0;
            }
            tutor.idtBold = idtHitF4OpenShipDesigner;
            if (hwndSlotDlg) {
                return 1;
            }
            return 0;
        case idtSelectAvailableHullTypes:
            hs.grhst = hstEngine;
            hs.iItem = iengineLongHump6;
            hs.cItem = 1;
            hs1.grhst = hstScanner;
            hs1.iItem = iscannerRhinoScanner;
            hs1.cItem = 1;
            if (!hwndSlotDlg && rgplr[0].cShDef == 7) {
                return 1;
            }
            if (!FCheckShipBuilder(4, iShipAny)) {
                if (FCheckShipBuilder(1, 7)) {
                    tutor.idtBold = idtHitCopySelectedDesign;
                } else {
                    tutor.idtBold = !FCheckShipBuilder(1, iShipAny) ? idtSelectAvailableHullTypes : idtChooseMiniMinerDropdown;
                }
                return 0;
            }
            tutor.idh = idhDesigningANewShipFromScratch;
            if (lpshdefBuild->hul.rghs[0].cItem == 0 || !FCheckBuilderPart(0, &hs, 1)) {
                tutor.idtBold = idtDragLongHump6EnginePartsList;
                return 0;
            }
            if (lpshdefBuild->hul.rghs[1].cItem == 0 || !FCheckBuilderPart(1, &hs1, 1)) {
                tutor.idtBold = idtDragRhinoScannerScannerElectMechSlot;
                return 0;
            }
            if (lpshdefBuild->hul.rghs[2].cItem == 0 || lpshdefBuild->hul.rghs[3].cItem == 0) {
                if (!FCheckShipBuilder(4, 8)) {
                    tutor.idtBold = idtSelectMiningRobotsPartsCategoryDropdown;
                    return 0;
                }
                tutor.idtBold = idtDragRoboMinerEachMiningSlots;
                return 0;
            }
            return 1;
        case idtShipDesignNameImageJustFine:
            if (hwndSlotDlg) {
                tutor.idtBold = !FCheckShipBuilder(4, iShipAny) ? idtDoneCloseDesigner : idtHitOkFinishEditingDesign;
                return 0;
            }
            if (LpplFromId(13)->lpplprod->iprodMac < 3 || (LpplFromId(13)->lpplprod->iprodMac == 3 && !FCheckQueue(13, 1, grobjFleet, 6, 1, 0))) {
                tutor.idtBold = idtAddOneNewMiniMinersStoveTops;
                return 0;
            }
            if (LpplFromId(13)->lpplprod->iprodMac < 4 || !FCheckQueue(13, 0, grobjPlanet, mdIdleMine, 100, 0)) {
                tutor.idtBold = !lpplProdGlob ? idtOpenStoveTopsQueue : idtSelectMineLeftHandListboxTopQueue;
                tutor.fProgress = 0;
                return 0;
            }
            return 1;
        case idtClickEachItemsProductionTileMiniMiner:
            if (!tutor.fProgress && !tutor.fAutoComplete) {
                tutor.idtBold = idtClickEachItemsProductionTileMiniMiner;
                return 0;
            }
            if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtReadFinalMessage;
                return 0;
            }
            if (!FCheckFleetWP(8, 1, grobjPlanet, 13, grTaskAny, iWarpAny)) {
                tutor.idtBold = idtSendArmedProbe9BackStoveTop;
                return 0;
            }
            tutor.idtBold = idtGenerateNewYear;
            tutor.fTurnDone = 1;
            return 1;
        }
    case 14:
        if (tutor.idt != idtReadFirstMessageGotoSeaSquared) {
            return 1;
        }
        if (LpplFromId(2)->lpplprod->iprodMac < 3 || !FCheckQueue(2, 0, grobjPlanet, iobjMinTerraform, 2, 1) || !FCheckTemplate(1)) {
            if (FCheckSelection(grobjPlanet, 2)) {
                tutor.idtBold = idtAddMinTerraform2OxygensQueueRight;
            } else if (FCheckSelection(grobjPlanet, 17)) {
                tutor.idtBold = idtReadFinalMessageGotoOxygen;
            } else {
                tutor.idtBold = idtReadFirstMessageGotoSeaSquared;
            }
            return 0;
        }
        tutor.idtBold = idtGenerateWhenReady8;
        tutor.fTurnDone = 1;
        return 1;
    case 15:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageSendArmedProbe9:
            if (!FCheckFleetWP(8, 1, grobjPlanet, 11, grTaskAny, iWarpAny)) {
                tutor.idtBold = idtReadFirstMessageSendArmedProbe9;
                return 0;
            }
            if (!FCheckCargo(LpflFromId(6), 0, 0, 0, 210)) {
                tutor.idtBold = idtReadMessageGotoNewTeamsterFillColonists;
                tutor.fProgress = 0;
                return 0;
            }
            if (!tutor.fAutoComplete && !tutor.fProgress) {
                if (!vprptCur) {
                    tutor.idtBold = idtChoosePlanetsReportMenu;
                } else {
                    tutor.fProgress = 1;
                    if (vprptCur->icolSort == 4) {
                        tutor.idtBold = idtHitEscKeyClosePlanetSummaryReport;
                    } else {
                        tutor.idtBold = idtClickTitleValueColumnSortValue;
                    }
                }
                return 0;
            }
            if (!FCheckXferWP(6, 1, 5, iWarpAny, rgiaUnloadAllCol)) {
                tutor.idtBold = idtSendTeamster7WallabyUnloadColonists;
                return 0;
            }
            return 1;
        case idtReadTwoMessagesOpenResearchDialog:
            if (!tutor.fAutoComplete && !tutor.fProgress) {
                if (pctResGlob != -1) {
                    tutor.fProgress = 1;
                    tutor.idtBold = idtCloseDialog;
                } else {
                    tutor.idtBold = idtReadTwoMessagesOpenResearchDialog;
                }
                return 0;
            }
            if (!FCheckFleetWP(11, 1, grobjPlanet, 13, grTaskAny, iWarpAny)) {
                tutor.idtBold = idtReadRemainingMessagesSendTeamster12Back;
                return 0;
            }
            tutor.idtBold = idtGenerateNewYear2;
            tutor.fTurnDone = 1;
            return 1;
        }
    case 16:
        if (tutor.idt != idtReadFirstMessageSendStalwartDefender5) {
            return 1;
        }
        if (!FCheckFleetWP(4, 1, grobjPlanet, 5, grTaskAny, iWarpAny)) {
            tutor.idtBold = idtReadFirstMessageSendStalwartDefender5;
            return 0;
        }
        tutor.fNoErrors = 1;
        if (LpflFromId(7)->cord == 1) {
            tutor.idtBold = !FCheckSelection(grobjFleet, 7) ? idtReadMessageGotoNewMiniMiner : idtShiftClickPruneSetWaypointTaskMerge;
            return 0;
        }
        if (FCheckFleetWP(7, 1, grobjPlanet, 12, grTaskAny, iWarpAny)) {
            tutor.fNoErrors = 0;
            tutor.idtBold = idtShiftClickPruneSetWaypointTaskMerge;
            return 0;
        }
        tutor.fNoErrors = 0;
        if (!FCheckFleetWP(7, 1, grobjFleet, 5, grTaskMerge, iWarpAny)) {
            tutor.idtBold = idtShiftClickPruneSetWaypointTaskMerge;
            return 0;
        }
        if (LpplFromId(13)->lpplprod->iprodMac < 2 || !FCheckQueue(13, 0, grobjPlanet, iobjFactory, 60, 0) ||
            !FCheckQueue(13, 1, grobjPlanet, iobjMine, 60, 0)) {
            tutor.idtBold = !lpplProdGlob ? idtReadMessageOpenStoveTopsProductionQueue : idtIncreaseNumberAutoBuildFactories60Add;
            return 0;
        }
        if (!FCheckResearch(Propulsion, Construction, 30)) {
            tutor.idtBold = idtReadTwoMessagesChangeFieldResearchConstruction;
            return 0;
        }
        tutor.idtBold = idtReadFinalMessageGenerate;
        tutor.fTurnDone = 1;
        return 1;
    case 17:
        if (tutor.idt != idtReadMessages2) {
            return 1;
        }
        if (!FCheckFleetWP(0, 1, grobjPlanet, 13, grTaskAny, iWarpAny)) {
            tutor.idtBold = !FCheckMessages(9999, static_cast<MessageId>(idmNone), 0) ? idtReadMessages2 : idtSendTeamster1BackStoveTop;
            return 0;
        }
        tutor.idtBold = idtGenerateNewYear3;
        tutor.fTurnDone = 1;
        return 1;
    case 18:
        if (tutor.idt != idtReadFirstMessageAddTeamsterStoveTops) {
            return 1;
        }
        if (LpplFromId(13)->lpplprod->iprodMac < 3 || !FCheckQueue(13, 0, grobjFleet, 3, 1, 0)) {
            tutor.idtBold = idtReadFirstMessageAddTeamsterStoveTops;
            return 0;
        }
        if (!FCheckMessages(5, static_cast<MessageId>(idmNone), 0)) {
            tutor.fProgress = 0;
            tutor.idtBold = idtReadMessageGoto90210;
            return 0;
        }
        if (!tutor.fAutoComplete && !tutor.fProgress) {
            if (FCheckSelection(grobjPlanet, 16)) {
                if (pctResGlob != -1) {
                    tutor.idtBold = idtClickDifferentItemsListedExpectedBenefitsBox;
                } else {
                    tutor.idtBold = idtReadTwoMessagesOpenResearchDialog2;
                }
            } else {
                tutor.idtBold = idtReadMessageGoto90210;
            }
            return 0;
        }
        if (pctResGlob != -1) {
            tutor.idtBold = idtCloseDialogWithoutMakingAnyChanges;
            return 0;
        }
        tutor.idtBold = idtReadRemainingMessagesGenerateYear;
        tutor.fTurnDone = 1;
        return 1;
    case 19:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageLoadTeamster12Colonists:
            if (!FCheckCargo(LpflFromId(11), 0, 0, 0, 210)) {
                tutor.idtBold = idtReadFirstMessageLoadTeamster12Colonists;
                return 0;
            }
            if (!FCheckXferWP(11, 1, 5, iWarpAny, rgiaUnloadAllCol)) {
                tutor.idtBold = idtAddWaypointWallabyUnloadColonists;
                return 0;
            }
            tutor.fNoErrors = 1;
            if (!FCheckXferWP(11, 2, 13, iWarpAny, rgiaLoadAllCol)) {
                tutor.fNoErrors = 0;
                tutor.idtBold = idtShiftClickBackStoveTopChangeTask;
                return 0;
            }
            tutor.fNoErrors = 0;
            tutor.idh = idhFleetWaypointsTile;
            tutor.idtBold = idtClickRepeatOrdersCheckboxFleetWaypointsTile2;
            if (!LpflFromId(11)->fRepOrders) {
                return 0;
            }
            if (!FCheckCargo(LpflFromId(1), 0, 0, 0, 210)) {
                tutor.idtBold = !FCheckSelection(grobjFleet, 1) ? idtReadMessageGotoNewTeamster : idtLoadColonists2;
                return 0;
            }
            return 1;
        case idtAddWaypointOxygenUnloadColonists:
            if (!FCheckXferWP(1, 1, 2, iWarpAny, rgiaUnloadAllCol)) {
                tutor.idtBold = idtAddWaypointOxygenUnloadColonists;
                return 0;
            }
            tutor.fNoErrors = 1;
            if (!FCheckXferWP(1, 2, 13, iWarpAny, rgiaLoadAllCol)) {
                tutor.fNoErrors = 0;
                tutor.idtBold = idtShiftClickBackStoveTopChangeTask2;
                return 0;
            }
            tutor.fNoErrors = 0;
            tutor.idh = idhFleetWaypointsTile;
            tutor.idtBold = idtClickRepeatOrdersCheckboxFleetWaypointsTile3;
            if (!LpflFromId(1)->fRepOrders) {
                return 0;
            }
            if (LpplFromId(16)->lpplprod->iprodMac < 3 || !FCheckQueue(16, 2, grobjPlanet, iobjMaxTerraform, 1, 1)) {
                tutor.idtBold = idtReadMessageAddMaxTerraformAutoBuild;
                return 0;
            }
            if (!FCheckFleetWP(6, 1, grobjPlanet, 13, grTaskAny, iWarpAny)) {
                tutor.idtBold = idtRead3MessagesSendTeamster7Back;
                return 0;
            }
            tutor.idtBold = idtReadLastMessageGenerateYear;
            tutor.fTurnDone = 1;
            return 1;
        }
    case 20:
        if (tutor.idt != idtReadFirstTwoMessagesSendArmedProbe) {
            return 1;
        }
        if (!FCheckFleetWP(8, 1, grobjPlanet, 6, grTaskAny, iWarpAny)) {
            tutor.idtBold = idtReadFirstTwoMessagesSendArmedProbe;
            return 0;
        }
        if (!FCheckResearch(Construction, Weapons, 30)) {
            tutor.idtBold = idtRead3MessagesChangeFieldResearchWeapons;
            return 0;
        }
        if (rgplr[0].cshdefSB == 1 || hwndSlotDlg) {
            if (!hwndSlotDlg && rgplr[0].cshdefSB == 1) {
                tutor.idtBold = idtReadFinalMessageHitF4OpenShip;
            } else if ((!FCheckShipBuilder(4, iShipAny) || !fStarbaseMode) && rgplr[0].cshdefSB == 1) {
                tutor.idtBold = idtSelectStarbasesCopySelectedDesign;
            } else if (lpshdefBuild->hul.rghs[0].cItem == 0) {
                tutor.idtBold = idtSelectOrbitalPartsCategoryDragStargate100;
            } else {
                tutor.idtBold = idtChangeDesignNameGaterClickRightArrow;
            }
            return 0;
        }
        if (LpplFromId(13)->lpplprod->iprodMac < 4 || !FCheckQueue(13, 1, grobjFleet, 17, 1, 0)) {
            tutor.idtBold = idtAddGaterStoveTopsQueue;
            return 0;
        }
        tutor.idtBold = idtGenerate;
        tutor.fTurnDone = 1;
        return 1;
    case 21:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageLoadTeamster1Colonists2:
            if (!FCheckCargo(LpflFromId(0), 0, 0, 0, 210)) {
                tutor.idtBold = idtReadFirstMessageLoadTeamster1Colonists2;
                return 0;
            }
            if (!FCheckXferWP(0, 1, 5, iWarpAny, rgiaUnloadAllCol)) {
                tutor.idtBold = idtSendUnloadColonistsWallaby;
                return 0;
            }
            if (!FCheckZip(0, rgiaUnloadAllCol, idsDropcol)) {
                tutor.idtBold = !hwndZipOrderDlg ? idtRightClickBlueDiamondWaypointTaskTile2 : idtHitImportNameOrderDropcolOkBoth;
                return 0;
            }
            tutor.fNoErrors = 1;
            if (!FCheckXferWP(0, 2, 13, iWarpAny, rgiaLoadAllCol)) {
                tutor.fNoErrors = 0;
                tutor.idtBold = idtShiftClickStoveTopChangeTransportOption;
                return 0;
            }
            tutor.fNoErrors = 0;
            tutor.idh = idhFleetWaypointsTile;
            tutor.idtBold = idtClickRepeatOrders;
            if (!LpflFromId(0)->fRepOrders) {
                return 0;
            }
            return 1;
        case idtReadRestMessages3:
            if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtReadRestMessages3;
                return 0;
            }
            if (!tutor.fAutoComplete && !tutor.fProgress) {
                if (!vprptCur) {
                    tutor.idtBold = idtHitF3OpenPlanetSummaryReport;
                } else if (vprptCur->icolSort == 11 && !vprptCur->fAscending && vprptCur->iSubsort == 3) {
                    tutor.idtBold = idtGenerateWhenReady9;
                    tutor.fProgress = 1;
                } else {
                    tutor.idtBold = idtFindMinConcColumnRightClickReverse;
                }
                return 0;
            }
            tutor.idtBold = idtGenerateWhenReady9;
            tutor.fTurnDone = 1;
            return 1;
        }
    case 22:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoTeamster42:
            if (LpflFromId(3)->rgwtMin[4] != 383) {
                if (!FCheckSelection(grobjFleet, 3)) {
                    tutor.idtBold = idtReadFirstMessageGotoTeamster42;
                } else {
                    tutor.idtBold = idtClickDragFuelGaugeOtherFleetsHere;
                    tutor.idh = idhOtherFleetsHereTile;
                }
                return 0;
            }
            return 1;
        case idtAddTeamsterStoveTopsQueue:
            if (LpplFromId(13)->lpplprod->iprodMac < 4 || !FCheckQueue(13, 1, grobjFleet, 3, 1, 0)) {
                tutor.idtBold = idtAddTeamsterStoveTopsQueue;
                return 0;
            }
            if (!FCheckResearch(Weapons, Propulsion, 30)) {
                tutor.idtBold = idtRead4MessagesChangeFieldResearchPropulsion;
                return 0;
            }
            if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtReadRemainingMessagesOpenShipDesigner;
                return 0;
            }
            if (rgplr[0].cShDef < 8 || hwndSlotDlg) {
                hs.grhst = hstEngine;
                hs.iItem = iengineDaddyLongLegs7;
                hs.cItem = 1;
                hs1.grhst = hstMines;
                hs1.iItem = iminesMineDispenser50;
                hs1.cItem = 3;
                if (!hwndSlotDlg) {
                    tutor.idtBold = idtReadRemainingMessagesOpenShipDesigner;
                    tutor.idh = static_cast<HelpContextId>(1001);
                    return 0;
                }
                if (rgplr[0].cShDef < 8 && !FCheckShipBuilder(4, iShipAny)) {
                    tutor.idtBold = idtViewAvailableHullTypesSelectFrigateDropdown;
                    return 0;
                }
                tutor.idh = idhDesigningANewShipFromScratch;
                if (lpshdefBuild->hul.rghs[0].cItem == 0 || !FCheckBuilderPart(0, &hs, 1)) {
                    tutor.idtBold = idtDragDaddyLongLegs7EngineSlot;
                } else if (lpshdefBuild->hul.rghs[2].cItem == 0 || !FCheckBuilderPart(2, &hs1, 1)) {
                    tutor.idtBold = idtSelectMineLayersDropdownDrag3Mine;
                } else {
                    tutor.idtBold = idtChangeDesignNameMineLayerOkDesign;
                }
                return 0;
            }
            if (LpplFromId(13)->lpplprod->iprodMac < 5 || !FCheckQueue(13, 2, grobjFleet, 7, 1, 0)) {
                tutor.idtBold = idtAddMineLayerStoveTopsQueue;
                return 0;
            }
            tutor.idtBold = idtAddMineLayerStoveTopsQueue;
            return 1;
        case idtClickRedTriangleWallaby:
            if (!FCheckFleetWP(4, 1, grobjFleet, 516, grTaskAny, iWarpAny)) {
                if (FCheckSelection(grobjFleet, 4)) {
                    tutor.idtBold = idtShiftClickEnemyFleet;
                } else if (FCheckSummary(grobjFleet, 516)) {
                    tutor.idtBold = idtRightClickWallabySelectStalwartDefender5;
                } else {
                    tutor.idtBold = idtClickRedTriangleWallaby;
                }
                return 0;
            }
            tutor.idtBold = idtGenerateWhenReady10;
            tutor.fTurnDone = 1;
            tutor.fProgress = 0;
            return 1;
        }
    case 23:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoStalwartDefender5:
            if (FCheckCargo(LpflFromId(6), 0, 0, 0, 210)) {
                return 1;
            }
            if (!FCheckMessages(1, static_cast<MessageId>(idmNone), 0) && !FCheckSelection(grobjFleet, 4)) {
                tutor.idtBold = idtReadFirstMessageGotoStalwartDefender5;
                tutor.fProgress = 0;
                return 0;
            }
            if (!FCheckMessages(2, static_cast<MessageId>(idmNone), 0) && !FCheckSelection(grobjFleet, 6)) {
                tutor.idtBold = idtReadMessageGotoTeamster7;
                return 0;
            }
            if (!tutor.fAutoComplete && !tutor.fProgress) {
                if (!vprptCur) {
                    tutor.idtBold = idtOpenPlanetSummaryReportSortPopulation;
                } else if (vprptCur->icolSort == 2) {
                    tutor.idtBold = idtHitEscCloseReport;
                    tutor.fProgress = 1;
                } else {
                    tutor.idtBold = idtOpenPlanetSummaryReportSortPopulation;
                }
                return 0;
            }
            return 1;
        case idtLoadTeamster7ColonistsSendSeaSquared:
            if (!FCheckCargo(LpflFromId(6), 0, 0, 0, 210)) {
                tutor.idtBold = idtLoadTeamster7ColonistsSendSeaSquared;
                return 0;
            }
            if (!FCheckFleetWP(6, 1, grobjPlanet, 17, grTaskAny, iWarpAny)) {
                tutor.idtBold = idtLoadTeamster7ColonistsSendSeaSquared;
                return 0;
            }
            if (!FCheckFleetWP(6, 1, grobjPlanet, 17, grTaskXfer, iWarpAny)) {
                tutor.idtBold = idtSetWaypointTaskTransport2;
                return 0;
            }
            if (!FCheckXferWP(6, 1, 17, iWarpAny, rgiaUnloadAllCol)) {
                tutor.idtBold = idtRightClickBlueDiamondChooseDropcol;
                tutor.idh = idhTransport;
                return 0;
            }
            if (LpflFromId(2) == 0) {
                tutor.fProgress = 0;
                return 1;
            }
            if (!FCheckMessages(3, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtReadTwoMessagesGotoNewTeamster;
            } else if (!FCheckSelection(grobjFleet, 3)) {
                tutor.idtBold = !FCheckSelection(grobjFleet, 2) ? idtReadTwoMessagesGotoNewTeamster : idtSelectTeamster4ListboxOtherFleetsHere;
                tutor.idh = idhOtherFleetsHereTile;
            } else if (!vrgiflMerge) {
                tutor.idtBold = idtPressMergeButtonFleetCompositionTile;
                tutor.idh = idhFleetCompositionTile;
            } else {
                tutor.idh = idhMergeFleetsDialog;
                tutor.idtBold = idtClickTeamster3MergeFleetsDialogHit;
            }
            return 0;
        case idtReadMessageSetMineLayer8sTask:
            if (!FCheckFleetWP(7, 0, grobjPlanet, 13, grTaskLayMines, iWarpAny)) {
                tutor.idtBold = idtReadMessageSetMineLayer8sTask;
                return 0;
            }
            if (LpplFromId(13)->lpplprod->iprodMac < 3 || !FCheckQueue(13, 0, grobjFleet, 6, 1, 0)) {
                tutor.idtBold = idtReadMessageAddMiniMinerStoveTops;
                tutor.fProgress = 0;
                return 0;
            }
            if (!FCheckMessages(14, static_cast<MessageId>(idmNone), 0) || (!FCheckSelection(grobjFleet, 4) && !FCheckSelection(grobjFleet, 8)) ||
                (!tutor.fAutoComplete && !tutor.fProgress)) {
                if (vrgtok) {
                    tutor.fProgress = 1;
                }
                tutor.idtBold = idtRead2MessagesWatchBattle;
                return 0;
            }
            if (LpflFromId(4)->lpplord->rgord()[0].grobj != grobjFleet) {
                tutor.idtBold = idtRightClickBlueDiamondFleetWaypointsTile;
                tutor.idh = idhFleetWaypointsTile;
                return 0;
            }
            if (LpflFromId(8)->cord < 3) {
                if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0) || !FCheckSelection(grobjFleet, 8)) {
                    tutor.idtBold = idtReadLastMessageDoubleClickArmedProbe;
                } else {
                    tutor.fNoErrors = 1;
                    if (FCheckFleetWP(8, 1, grobjPlanet, 1, grTaskAny, iWarpAny)) {
                        tutor.idtBold = idtShiftClickLeverGenerate;
                    } else {
                        tutor.fNoErrors = 0;
                        FCheckFleetWP(8, 1, grobjPlanet, 6, grTaskAny, iWarpAny);
                        tutor.idtBold = idtDragWaypointLaTeDaSpeedBump;
                    }
                    tutor.fNoErrors = 0;
                }
                return 0;
            }
            if (!FCheckFleetWP(8, 2, grobjPlanet, 0, grTaskAny, iWarpAny)) {
                tutor.idtBold = idtShiftClickLeverGenerate;
                return 0;
            }
            tutor.idtBold = idtShiftClickLeverGenerate;
            tutor.fTurnDone = 1;
            return 1;
        }
    case 24:
        if (tutor.idt != idtReadFirstMessageSendStalwartDefender52) {
            return 1;
        }
        if (!FCheckFleetWP(4, 1, grobjPlanet, 5, grTaskAny, iWarpAny)) {
            tutor.idtBold = idtReadFirstMessageSendStalwartDefender52;
            return 0;
        }
        tutor.fNoErrors = 1;
        if (!FCheckFleetWP(2, 1, grobjFleet, 5, grTaskMerge, iWarpAny)) {
            tutor.fNoErrors = 0;
            FCheckFleetWP(2, 1, grobjPlanet, 12, grTaskAny, iWarpAny);
            tutor.idtBold = idtReadMessageGotoMiniMiner3Send;
            return 0;
        }
        tutor.fNoErrors = 0;
        if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
            tutor.idtBold = idtReadRemainingMessages;
            return 0;
        }
        tutor.idtBold = idtGenerateWill;
        tutor.fTurnDone = 1;
        return 1;
    case 25:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstTwoMessages:
            if (!FCheckMessages(3, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtReadFirstTwoMessages;
                return 0;
            }
            if (rgshdef[2].hul.rghs[0].iItem != iengineDaddyLongLegs7 || hwndSlotDlg) {
                hs.grhst = hstEngine;
                hs.iItem = iengineDaddyLongLegs7;
                hs.cItem = 1;
                hs2.grhst = hstSpecialM;
                hs2.iItem = ispecialMColonizationModule;
                hs2.cItem = 1;
                if (!FCheckScanner(3, iZoomAny)) {
                    tutor.idtBold = idtClickButtonToolbar;
                    return 0;
                }
                if (!hwndSlotDlg) {
                    tutor.idtBold = idtHitF4OpenShipDesigner2;
                    tutor.idh = static_cast<HelpContextId>(1001);
                    return 0;
                }
                if (rgshdef[2].hul.rghs[0].iItem == iengineDaddyLongLegs7) {
                    tutor.idtBold = idtOkDesignHitDoneCloseDialog;
                    return 0;
                }
                if (!FCheckShipBuilder(4, iShipAny)) {
                    if (!FCheckShipBuilder(0, 2)) {
                        tutor.idtBold = idtSelectSantaMariaDropdown;
                    } else {
                        tutor.idtBold = idtHitEditSelectedDesign;
                    }
                } else if (lpshdefBuild->hul.rghs[0].cItem == 0 || lpshdefBuild->hul.rghs[0].iItem != iengineDaddyLongLegs7 || !FCheckBuilderPart(1, &hs2, 1)) {
                    tutor.idtBold = idtDragLongHump6EngineDesignParts;
                } else {
                    tutor.idtBold = idtOkDesignHitDoneCloseDialog;
                }
                return 0;
            }
            return 1;
        case idtAdd3ImprovedSantaMariasStoveTops:
            if (LpplFromId(13)->lpplprod->iprodMac < 3 || !FCheckQueue(13, 0, grobjFleet, 2, 3, 0)) {
                tutor.idtBold = idtAdd3ImprovedSantaMariasStoveTops;
                return 0;
            }
            if (!FCheckMessages(4, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtReadMessageGotoSeaSquared;
                return 0;
            }
            if (LpplFromId(17)->lpplprod->iprodMac != 3 || !FCheckQueue(17, 2, grobjPlanet, iobjMaxTerraform, 2, 1)) {
                if (!FCheckSelection(grobjPlanet, 17)) {
                    tutor.idtBold = idtReadMessageGotoSeaSquared;
                } else {
                    tutor.idtBold = idtAddMaxTerraformAutoBuild2End;
                }
                return 0;
            }
            if (!FCheckMessages(6, static_cast<MessageId>(idmNone), 0) || !FCheckResearch(Propulsion, Construction, 30)) {
                tutor.idtBold = idtRead2MessagesChangeFieldResearchConstruction;
                return 0;
            }
            if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtReadRestMessages4;
                return 0;
            }
            tutor.idtBold = idtGenerateWhenReady11;
            tutor.fTurnDone = 1;
            tutor.fProgress = 0;
            return 1;
        }
    case 26:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstThreeMessages:
            if (!FCheckMessages(4, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtReadFirstThreeMessages;
                return 0;
            }
            if (rgplr[0].cFleet == 10 && !FCheckColonizeWP(9, 0, iWarpAny)) {
                tutor.idtBold = !FCheckSelection(grobjFleet, 9) ? idtGoto3NewSantaMarias : idtLoadThemColonistsSendThemColonizeLever;
                return 0;
            }
            if (rgplr[0].cFleet == 10) {
                tutor.idtBold = idtHitSplitButtonFleetCompositionTile2;
                return 0;
            }
            tutor.fNoErrors = 1;
            if (!FCheckColonizeWP(9, 1, iWarpAny)) {
                tutor.fNoErrors = 0;
                FCheckColonizeWP(9, 0, iWarpAny);
                tutor.idtBold = idtDragSantaMaria10sWaypointSpeedBump;
                return 0;
            }
            if (!FCheckColonizeWP(10, 23, iWarpAny)) {
                tutor.fNoErrors = 0;
                FCheckColonizeWP(10, 0, iWarpAny);
                tutor.idtBold = idtSantaMaria11sBloop;
                return 0;
            }
            tutor.fNoErrors = 0;
            if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtReadRestMessages5;
                return 0;
            }
            return 1;
        case idtTeamster12WillArriveStoveTopYear:
            if (LpplFromId(13)->lpplprod->iprodMac < 4 || !FCheckQueue(13, 1, grobjFleet, 3, 3, 0)) {
                tutor.idtBold = idtAdd3TeamstersStoveTopsQueue;
                return 0;
            }
            tutor.fNoErrors = 1;
            if (!FCheckFleetWP(4, 1, grobjFleet, 517, grTaskAny, iWarpAny)) {
                tutor.fNoErrors = 0;
                FCheckColonizeWP(4, 5, iWarpAny);
                if (FCheckSummary(grobjFleet, 517) || FCheckSummary(grobjFleet, 4) || FCheckSelection(grobjFleet, 4)) {
                    tutor.idtBold = idtSelectStalwartDefender5DragDestinationEnemy;
                } else {
                    tutor.idtBold = idtClickEnemyShipNearWallaby;
                }
                return 0;
            }
            tutor.fNoErrors = 0;
            tutor.idtBold = idtGenerateWhenYoureReady;
            tutor.fTurnDone = 1;
            return 1;
        }
    case 27:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoStalwartDefender52:
            tutor.fNoErrors = 1;
            if (!FCheckFleetWP(4, 0, grobjFleet, 517, grTaskAny, iWarpAny)) {
                tutor.fNoErrors = 0;
                if (FCheckSelection(grobjFleet, 4)) {
                    tutor.idh = idhFleetWaypointsTile;
                    tutor.idtBold = idtRightClickBlueDiamondFleetWaypointsTile2;
                } else {
                    tutor.idtBold = idtReadFirstMessageGotoStalwartDefender52;
                }
                return 0;
            }
            tutor.fNoErrors = 0;
            if (!FCheckMessages(1, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtReadMessageGotoArmedProbe92;
                return 0;
            }
            if (LpflFromId(12) == 0 || LpflFromId(12)->fDead) {
                return 1;
            }
            if (!FCheckCargo(LpflFromId(12), 0, 0, 0, 630)) {
                if (FCheckSelection(grobjFleet, 12)) {
                    tutor.idtBold = idtFillColonists;
                } else {
                    tutor.idtBold = !FCheckSelection(grobjFleet, 8) ? idtReadMessageGotoArmedProbe92 : idtReadMessageGotoNewFleet;
                }
                return 0;
            }
            return 1;
        case idtWeWantMergeNewTeamstersOtherFleet:
            if (LpflFromId(12) != 0) {
                if (!FCheckSelection(grobjFleet, 11)) {
                    tutor.idtBold = idtSelectTeamster12PressMergeButtonFleet;
                } else if (!vrgiflMerge) {
                    tutor.idtBold = idtSelectTeamster12PressMergeButtonFleet;
                    tutor.idh = idhFleetCompositionTile;
                } else {
                    tutor.idtBold = idtSelectTeamster12PressMergeButtonFleet;
                }
                return 0;
            }
            if (!FCheckMessages(8, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtReadMessage6;
                return 0;
            }
            if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtReadRestMessagesViewingBattleColonizerIf;
                return 0;
            }
            tutor.idtBold = idtHitF4OpenShipDesigner3;
            tutor.idh = idhKeyboardShortcuts;
            if (hwndSlotDlg || rgplr[0].cShDef == 9) {
                return 1;
            }
            return 0;
        case idtWeWantPowerfulWeAlsoWantWeigh:
            if (hwndSlotDlg || rgplr[0].cShDef < 9) {
                if (!FCheckShipBuilder(4, iShipAny)) {
                    tutor.idtBold = rgplr[0].cShDef >= 9 ? idtClickRightArrowButtonBelowShipImage : idtSelectAvailableHullTypesChooseDestroyerDropdown;
                } else if (lpshdefBuild->hul.rghs[0].cItem != 1 || lpshdefBuild->hul.rghs[1].cItem != 1 || lpshdefBuild->hul.rghs[2].cItem != 1 ||
                           lpshdefBuild->hul.rghs[3].cItem != 1 || lpshdefBuild->hul.rghs[4].cItem != 2) {
                    tutor.idtBold = idtAddRadiatingHydroRamScoop2Carbonic;
                } else if (lpshdefBuild->hul.rghs[5].cItem != 1 || lpshdefBuild->hul.rghs[6].cItem != 1) {
                    tutor.idtBold = idtAddFuelTankMechanicalSlotBattleComputer;
                } else {
                    tutor.idtBold = idtClickRightArrowButtonBelowShipImage;
                }
                return 0;
            }
            if (LpplFromId(13)->lpplprod->iprodMac < 3 || !FCheckQueue(13, 0, grobjFleet, 8, 10, 0)) {
                tutor.idtBold = idtPut10DestroyersStoveTopsQueue;
                return 0;
            }
            tutor.idtBold = idtGenerate2;
            tutor.fTurnDone = 1;
            return 1;
        }
    case 28:
        if (tutor.idt != idtReadFirstMessageGotoStalwartDefender53) {
            return 1;
        }
        if (!FCheckFleetWP(4, 1, grobjPlanet, 5, grTaskAny, iWarpAny)) {
            if (!FCheckSelection(grobjFleet, 4)) {
                tutor.idtBold = idtReadFirstMessageGotoStalwartDefender53;
            } else {
                tutor.idtBold = idtSendWallaby;
            }
            return 0;
        }
        if (!FCheckMessages(6, static_cast<MessageId>(idmNone), 0)) {
            tutor.idtBold = idtRead4MessagesGotoNewDestroyerArmada;
            return 0;
        }
        if (!FCheckFleetWP(12, 1, grobjPlanet, 10, grTaskAny, iWarpAny)) {
            if (!FCheckSelection(grobjFleet, 12)) {
                tutor.idtBold = idtRead4MessagesGotoNewDestroyerArmada;
            } else {
                tutor.idtBold = idtSendWreakHavocBerserkerStarbaseHacker;
            }
            return 0;
        }
        if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
            tutor.idtBold = idtReadRestMessages6;
            return 0;
        }
        tutor.idtBold = idtGenerateTurn;
        tutor.fTurnDone = 1;
        tutor.fProgress = 0;
        return 1;
    case 29:
        switch (tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoTeamster43:
            tutor.fNoErrors = 1;
            if (!FCheckFleetWP(3, 1, grobjPlanet, 13, grTaskAny, 5)) {
                tutor.fNoErrors = 0;
                if (!FCheckSelection(grobjFleet, 3)) {
                    tutor.idtBold = idtReadFirstMessageGotoTeamster43;
                } else {
                    tutor.idtBold = idtClickStoveTopFleetWaypointsTileDecrease;
                }
                return 0;
            }
            tutor.fNoErrors = 0;
            if (!FCheckMessages(6, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtRead4Messages;
                return 0;
            }
            if (!FCheckFleetWP(13, 1, grobjPlanet, 10, grTaskAny, iWarpAny)) {
                tutor.idtBold = idtSendNewDestroyerHackerWell;
                return 0;
            }
            if (!FCheckPlanetRoute(13, 10)) {
                tutor.idtBold = idtSelectStoveTopControlClickHacker;
                return 0;
            }
            return 1;
        case idtNoticeProductionTileNewShipsWillRouted:
            if (!FCheckMessages(17, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtRead3Messages;
                return 0;
            }
            if (!FCheckResearch(Construction, Energy, 30)) {
                tutor.idtBold = idtOpenResearchDialogSetFieldResearchEnergy;
                return 0;
            }
            if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
                tutor.idtBold = idtReadRestMessages7;
                return 0;
            }
            tutor.fNoErrors = 1;
            if (!FCheckFleetWP(6, 0, grobjPlanet, 17, grTaskScrap, iWarpAny)) {
                tutor.fNoErrors = 0;
                if (!FCheckSelection(grobjFleet, 6)) {
                    tutor.idtBold = idtGotoTeamster7;
                } else {
                    tutor.idtBold = idtGiveTeamster7OrdersScrapFleet;
                }
                return 0;
            }
            tutor.fNoErrors = 0;
            return 1;
        case idtLetsFinishOffBerserkersOnceBuildingBombing:
            if (hwndSlotDlg) {
                if (rgshdef[9].fFree || FCheckShipBuilder(4, iShipAny)) {
                    if (FCheckShipBuilder(4, iShipAny)) {
                        if (lpshdefBuild->hul.rghs[0].cItem != 2) {
                            tutor.idtBold = idtAddRadiatingHydroRamScoopEngines;
                        } else if (lpshdefBuild->hul.rghs[1].cItem != 4 || lpshdefBuild->hul.rghs[2].cItem != 4 || lpshdefBuild->hul.rghs[3].cItem != 1) {
                            tutor.idtBold = idtHoldShiftKeyDrag4BlackCat;
                        } else {
                            tutor.idtBold = idtOkDesignCloseShipDesigner;
                        }
                    } else {
                        tutor.idtBold = idtSelectAvailableHullTypesChooseB17;
                    }
                } else {
                    tutor.idtBold = idtOkDesignCloseShipDesigner;
                }
                return 0;
            }
            if (rgplr[0].cShDef < 10) {
                tutor.idtBold = idtHitF4OpenShipDesigner4;
                tutor.idh = idhKeyboardShortcuts;
                return 0;
            }
            if (LpplFromId(13)->lpplprod->iprodMac < 3 || !FCheckQueue(13, 0, grobjFleet, 9, 10, 0)) {
                tutor.idtBold = idtAdd10B17BombersStoveTops;
                return 0;
            }
            tutor.idtBold = idtGenerateWhenReady12;
            tutor.fTurnDone = 1;
            tutor.fProgress = 0;
            return 1;
        }
    case 30:
        if (tutor.idt != idtCongratulationsYouveDeclaredWinner) {
            return 1;
        }
        if (!FCheckMessages(5, static_cast<MessageId>(idmNone), 0)) {
            tutor.idtBold = idtReadFirst3MessagesGotoNewB;
            return 0;
        }
        if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
            if (!FCheckSelection(grobjFleet, 14)) {
                tutor.idtBold = idtReadFirst3MessagesGotoNewB;
            } else {
                tutor.idtBold = idtReadRestMessages8;
            }
            return 0;
        }
        tutor.idtBold = idtGenerateWhenYoureReady2;
        tutor.fTurnDone = 1;
        tutor.fProgress = 0;
        return 1;
    case 31:
        if (tutor.idt != idtReadFirst4MessagesGotoWallaby) {
            return 1;
        }
        if (!FCheckMessages(6, static_cast<MessageId>(idmNone), 0)) {
            tutor.idtBold = idtReadFirst4MessagesGotoWallaby;
            return 0;
        }
        if (!FCheckSelection(grobjPlanet, 5)) {
            tutor.idtBold = idtReadFirst4MessagesGotoWallaby;
            return 0;
        }
        if (LpplFromId(5)->lpplprod->iprodMac < 4 || !FCheckQueue(5, 0, grobjPlanet, mdIdleMine, 100, 1)) {
            tutor.idtBold = idtAdd100MinesWallabysQueue;
            return 0;
        }
        if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
            tutor.idtBold = idtReadRestMessages9;
            return 0;
        }
        tutor.idtBold = idtGenerateWill2;
        tutor.fTurnDone = 1;
        tutor.fProgress = 0;
        return 1;
    case 32:
        if (tutor.idt != idtReadFirst3MessagesGotoDestroyer13) {
            return 1;
        }
        if (!FCheckMessages(2, static_cast<MessageId>(idmNone), 0)) {
            tutor.idtBold = idtReadFirst3MessagesGotoDestroyer13;
            return 0;
        }
        if (!tutor.fProgress && !tutor.fAutoComplete) {
            tutor.idtBold = idtClickDestroyerFleetCompositionTile;
            return 0;
        }
        if (!FCheckMessages(18, static_cast<MessageId>(idmNone), 0)) {
            if (!FCheckSelection(grobjFleet, 12)) {
                tutor.idtBold = idtReadFirst3MessagesGotoDestroyer13;
            } else {
                tutor.idtBold = idtRead8MessagesViewAssaultEnemyStarbase;
            }
            return 0;
        }
        tutor.idtBold = idtReadRestMessagesGenerate;
        if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
            return 0;
        }
        tutor.fTurnDone = 1;
        tutor.fProgress = 0;
        return 1;
    case 33:
        if (tutor.idt != idtReadFirst6MessagesGotoStoveTop) {
            return 1;
        }
        if (!FCheckMessages(10, static_cast<MessageId>(idmNone), 0)) {
            tutor.idtBold = idtReadFirst6MessagesGotoStoveTop;
            return 0;
        }
        if (LpplFromId(13)->lpplprod->iprodMac < 3 || !FCheckQueue(13, 0, grobjFleet, 9, 10, 0)) {
            if (!FCheckSelection(grobjPlanet, 13)) {
                tutor.idtBold = idtReadFirst6MessagesGotoStoveTop;
            } else {
                tutor.idtBold = idtAddAnother10B17BombersProduction;
            }
            return 0;
        }
        if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
            tutor.idtBold = idtReadRestMessagesWatchBattles;
            return 0;
        }
        tutor.idtBold = idtGenerateWhenReady13;
        tutor.fTurnDone = 1;
        tutor.fProgress = 0;
        return 1;
    case 34:
        if (tutor.idt != idtNothingMuchHappeningYearViewBattleHacker) {
            return 1;
        }
        tutor.idtBold = idtReadMessagesGenerateWhenReady;
        if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
            return 0;
        }
        tutor.fTurnDone = 1;
        tutor.fProgress = 0;
        return 1;
    case 35:
        if (tutor.idt != idtReadThroughMessages) {
            return 1;
        }
        if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
            tutor.idtBold = idtReadThroughMessages;
            return 0;
        }
        tutor.idtBold = idtGenerateWhenReady14;
        tutor.fTurnDone = 1;
        tutor.fProgress = 0;
        return 1;
    case 36:
        if (tutor.idt != idtCongratulationsHaveReachedEndTutorial) {
            return 1;
        }
        if (!FCheckMessages(9999, static_cast<MessageId>(idmNone), 0)) {
            tutor.idtBold = idtReadMessages3;
            return 0;
        }
        tutor.idtBold = idtWhenGenerateYoureOwn;
        tutor.fTurnDone = 1;
        tutor.fProgress = 0;
        return 1;
    default:
        tutor.fTurnDone = 1;
        tutor.fTutorDone = 1;
        return 1;
    }
}

int16_t FCheckZip(int16_t iZip, ITEMACTION *lpiaGoal, StringId ids) {
    ITEMACTION *piaCur;
    int16_t     i;
    char        szT[33];
    int16_t     idhSav;

    idhSav = tutor.idh;
    tutor.idh = idhCustomZipOrdersDialog;
    if (tutor.fAutoComplete) {
        vrgZip[iZip].fValid = 1;
        piaCur = vrgZip[iZip].txp.rgia;
        i = 0;
        while (i < 5) {
            piaCur->SetIAction(lpiaGoal->iAction());
            i++;
            piaCur++;
            lpiaGoal++;
        }
        TextIO().pfnCchGetString(ids, szT);
        strcpy(vrgZip[iZip].szName, szT);
        return 1;
    }
    if (!vrgZip[iZip].fValid) {
        if (!hwndZipOrderDlg) {
            tutor.idh = idhTransport;
        }
        return 0;
    }
    piaCur = vrgZip[iZip].txp.rgia;
    tutor.idh = idhWaypointTaskTile;
    i = 0;
    while (i < 5) {
        if (piaCur->iAction() != lpiaGoal->iAction() && piaCur->iAction() != iActionNone) {
            return 0;
        }
        i++;
        piaCur++;
        lpiaGoal++;
    }
    TextIO().pfnCchGetString(ids, szT);
    if (strcmpi(szT, vrgZip[iZip].szName) == 0) {
        tutor.idh = static_cast<HelpContextId>(idhSav);
        return 1;
    }
    TutorError(idsTutorialHaventGivenZipOrderRightName);
    return 0;
}

int16_t FCheckTemplate(int16_t iTemplate) {
    int16_t i;

    tutor.idh = idhProductionTemplates;
    if (tutor.fAutoComplete) {
        vrgZipProd[0].fValid = 1;
        vrgZipProd[0].SetZpq1(rgzpqTut[iTemplate]);
        gd.fChgZipProd = 1;
        return 1;
    }
    if (!vrgZipProd[0].fValid) {
        return 0;
    }
    if (vrgZipProd[0].fNoResearch != rgzpqTut[iTemplate].fNoResearch) {
        return 0;
    }
    if (vrgZipProd[0].cpq != rgzpqTut[iTemplate].cpq) {
        return 0;
    }
    for (i = 0; i < rgzpqTut[iTemplate].cpq; i++) {
        if (vrgZipProd[0].rgpq[i].w != rgzpqTut[iTemplate].rgpq[i].w) {
            return 0;
        }
    }
    gd.fChgZipProd = 1;
    return 1;
}

void TutorError(StringId idsError) {
    if (tutor.fNoErrors) {
        tutor.idsError = -1;
    } else if (tutor.idsError != idsError || tutor.cError++ >= 3) {
        tutor.cError = 0;
        tutor.idsError = idsError;
        AlertIO().pfnAlertSz(TextIO().pfnFormat(idsError, NULL), win32::MB_ICONHAND);
    }
    return;
}

int16_t FCheckScanner(int16_t md, int16_t iZoom) {
    int16_t idhSav;

    idhSav = tutor.idh;
    if (tutor.fAutoComplete) {
        return 1;
    }
    tutor.idh = idhChoosingYourViewOfTheUniverse;
    if (md != mdScanAny && grbitScan != md) {
        if (md < 6) {
            if ((grbitScan & grbitScanViewMask) != md) {
                return 0;
            }
        } else if ((md & grbitScan) != md) {
            return 0;
        }
    }
    if (iZoom != iZoomAny && iZoom != iScanZoom) {
        tutor.idh = idhZooming;
        return 0;
    }
    tutor.idh = static_cast<HelpContextId>(idhSav);
    return 1;
}

int16_t FCheckFleetName(int16_t id, StringId ids) {
    FLEET  *lpfl;
    char    szT[33];
    int16_t idhSav;

    idhSav = tutor.idh;
    tutor.idh = idhNamingFleets;
    lpfl = LpflFromId(id);
    if (!lpfl) {
        return 1;
    }
    if (!lpfl->lpszName) {
        if (ids == static_cast<StringId>(idsNoString)) {
            tutor.idh = static_cast<HelpContextId>(idhSav);
            return 1;
        }
        if (ids == static_cast<StringId>(idsNoString)) {
            return 1;
        }
        return 0;
    }
    TextIO().pfnCchGetString(ids, szT);
    if (_stricmp(szT, lpfl->lpszName) == 0) {
        tutor.idh = static_cast<HelpContextId>(idhSav);
        return 1;
    }
    TutorError(idsTutorialHaveGivenFleetWrongNamePlease);
    return 0;
}

int16_t FCheckSummary(GrobjClass grobj, int16_t id) {
    int16_t fRet;

    if (gd.fGeneratingTurn) {
        return 1;
    }
    fRet = 0;
    switch (grobj) {
    default:
        break;
    case grobjFleet:
        if (sel.scan.grobj == grobjFleet && sel.scan.ifl != iflNone && rglpfl[sel.scan.ifl]->id == id) {
            fRet = 1;
            break;
        }
        fRet = 0;
        break;
    case grobjPlanet:
        if (sel.scan.grobj == grobjPlanet && sel.scan.idpl == id) {
            fRet = 1;
            break;
        }
        fRet = 0;
        break;
    case grobjThing:
        fRet = sel.scan.grobj == grobjThing && (id == idAny || lpThings[sel.scan.ith].idFull() == id);
    }
    if (!fRet) {
        tutor.idh = idhKeyToTheScanner;
    }
    return fRet;
}

int16_t FCheckSelection(GrobjClass grobj, int16_t id) {
    int16_t fRet;
    int16_t idhSav;

    idhSav = tutor.idh;
    if (tutor.fAutoComplete || gd.fGeneratingTurn) {
        return 1;
    }
    if (mdMsgObj == (grobj == grobjFleet ? 2 : 1) && id == idMsgObj) {
        tutor.idh = idhMessagesPane;
    } else if (grobj == grobjPlanet && sel.grobj == grobjFleet && sel.fl.idPlanet == id) {
        tutor.idh = idhLocationTile;
    } else if (grobj == grobjFleet && sel.grobj == grobjPlanet && LpflFromId(id)->idPlanet == sel.pl.id) {
        tutor.idh = idhFleetsInOrbitTile;
    } else {
        tutor.idh = idhSelectingAnObjectToCommand;
    }
    fRet = 0;
    if (grobj == grobjPlanet) {
        fRet = sel.grobj == grobjPlanet && sel.pl.id == id;
    } else if (grobj == grobjFleet) {
        fRet = sel.grobj == grobjFleet && sel.fl.id == id;
    }
    if (fRet) {
        tutor.idh = static_cast<HelpContextId>(idhSav);
    }
    return fRet;
}

int16_t FCheckMessages(int16_t imsg, MessageId idm, int16_t fFilter) {
    int16_t idhSav;

    idhSav = tutor.idh;
    if (gd.fGeneratingTurn) {
        return 1;
    }
    tutor.idh = idhMessagesPane;
    if (imsg == 9999 && IMsgNext(0) != imsgNone) {
        return 0;
    }
    if (imsg != 9999 && imsg != imsgAny && iMsgCur < imsg) {
        return 0;
    }
    if (idm != static_cast<MessageId>(idmNone)) {
        if (fFilter && tutor.fAutoComplete && !(bitfMsgFiltered[idm >> 3] & 1 << (idm & 7))) {
            SetFilteringGroups(idm, 1);
            tutor.idh = static_cast<HelpContextId>(idhSav);
            return 1;
        }
        if (fFilter && !(bitfMsgFiltered[idm >> 3] & 1 << (idm & 7))) {
            return 0;
        }
        if (!fFilter && PresentationIO().pfnMessageId(iMsgCur) != idm) {
            return 0;
        }
    }
    tutor.idh = static_cast<HelpContextId>(idhSav);
    return 1;
}

int16_t FCheckResearch(TechFieldType iTech, TechFieldType iTechNext, int16_t pct) {
    if (rgplr[0].iTechNow() == iTech && rgplr[0].iTechNext() == iTechNext && rgplr[0].pctResearch == pct) {
        return 1;
    }
    tutor.idh = idhResearchDialog;
    return 0;
}

int16_t FCheckFleetWP(uint16_t ifl, int16_t iord, GrobjClass grobj, int16_t id, TaskType grTask, uint16_t iWarp) {
    ORDER   ord;
    int16_t fRet;
    FLEET  *lpfl;
    int16_t idh;
    int16_t idhSav;

    fRet = 0;
    idhSav = tutor.idh;
    lpfl = LpflFromId(ifl);
    tutor.idh = idhSelectingAnObjectToCommand;
    if (!lpfl)
        goto LReturn;
    tutor.idh = idhAddingFleetWaypoints;
    if (lpfl->cord < iord + 1)
        goto LReturn;
    ord = lpfl->lpplord->rgord()[iord];
    if ((id & 0x7fff) != 0x7fff && (ord.grobj != grobj || ord.id != id)) {
        TutorError(idsTutorialHaveGivenFleetWrongDestinationPress);
        tutor.idh = idhMovingFleetWaypoints;
    } else if (ord.grTask != grTask && grTask != grTaskAny) {
        tutor.idh = idhWaypointTaskTile;
        if (ord.grTask != grTaskNone) {
            TutorError(grTask == grTaskNone ? idsTutorialHaveGivenFleetTaskDestinationWaypoint : idsTutorialHaveGivenFleetWrongTaskDestination);
        }
    } else {
        tutor.idh = idhFleetWaypointsTile;
        if (iWarp != iWarpAny) {
            fRet = ord.iWarp == iWarp;
        } else {
            fRet = 1;
        }
    }
LReturn:
    idh = tutor.idh;
    if (!fRet && FCheckSelection(grobjFleet, ifl)) {
        tutor.idh = static_cast<HelpContextId>(idh);
    }
    if (fRet) {
        tutor.idh = static_cast<HelpContextId>(idhSav);
    }
    return fRet;
}

int16_t FCheckPlanetRoute(int16_t idpl, int16_t idplRoute) {
    PLANET *lppl;
    int16_t idhSav;

    idhSav = tutor.idh;
    lppl = LpplFromId(idpl);
    if (!lppl) {
        return 0;
    }
    tutor.idh = idhRoute;
    if (lppl->idRoute != idplRoute + 1) {
        return 0;
    }
    tutor.idh = static_cast<HelpContextId>(idhSav);
    return 1;
}

int16_t FCheckLayingWP(uint16_t ifl, int16_t iord, int16_t id, int16_t iYears) {
    FLEET     *lpfl;
    int16_t    idhSav;
    GrobjClass grobj;

    idhSav = tutor.idh;
    lpfl = LpflFromId(ifl);
    if (!lpfl) {
        return 0;
    }
    tutor.idh = idhLayMineFields;
    grobj = !(id & 0x8000) ? grobjPlanet : grobjFleet;
    if (!FCheckFleetWP(ifl, iord, grobj, id & 0x7fff, grTaskLayMines, iWarpAny)) {
        return 0;
    }
    if (lpfl->lpplord->rgord()[iord].tsell().iPlrX != iYears) {
        return 0;
    }
    tutor.idh = static_cast<HelpContextId>(idhSav);
    return 1;
}

int16_t FCheckColonizeWP(uint16_t ifl, int16_t id, uint16_t iWarp) {
    int16_t ish;
    FLEET  *lpfl;
    int16_t csh;
    int16_t idhSav;

    idhSav = tutor.idh;
    lpfl = LpflFromId(ifl);
    if (!lpfl) {
        return 0;
    }
    tutor.idh = idhColonize;
    csh = 0;
    for (ish = 0; ish < 16; ish++) {
        csh += lpfl->rgcsh[ish];
    }
    if (lpfl->idPlanet == 13 && !FCheckCargo(lpfl, 0, 0, 0, 25 * csh)) {
        return 0;
    }
    if (FCheckFleetWP(ifl, 1, grobjPlanet, id, grTaskColonize, iWarp)) {
        tutor.idh = static_cast<HelpContextId>(idhSav);
        return 1;
    }
    return 0;
}

int16_t FCheckPatrolWP(uint16_t ifl, int16_t iord, int16_t id, uint16_t iWarp, uint16_t iPlan, uint16_t iDist) {
    FLEET     *lpfl;
    int16_t    idhSav;
    GrobjClass grobj;

    idhSav = tutor.idh;
    lpfl = LpflFromId(ifl);
    if (!lpfl) {
        return 0;
    }
    tutor.idh = idhPatroling;
    grobj = !(id & 0x8000) ? grobjPlanet : grobjFleet;
    if (!FCheckFleetWP(ifl, iord, grobj, id & 0x7fff, grTaskPatrol, iWarp)) {
        return 0;
    }
    if (iDist != iDistAny && lpfl->lpplord->rgord()[iord].tptl().iDist != iDist) {
        tutor.idh = idhWaypointTaskTile;
        return 0;
    }
    tutor.idh = static_cast<HelpContextId>(idhSav);
    return 1;
}

int16_t FCheckXferWP(uint16_t ifl, int16_t iord, int16_t id, uint16_t iWarp, ITEMACTION *lpiaGoal) {
    ORDER       ord;
    int16_t     fRet;
    ITEMACTION *piaCur;
    int16_t     i;
    FLEET      *lpfl;
    int16_t     idh;
    GrobjClass  grobj;
    int16_t     idhSav;

    fRet = 0;
    idhSav = tutor.idh;
    if (id & 0x8000) {
        id &= 0x7fff;
        grobj = grobjFleet;
    } else {
        grobj = grobjPlanet;
    }
    lpfl = LpflFromId(ifl);
    if (!lpfl) {
        return 0;
    }
    if (!FCheckFleetWP(ifl, iord, grobj, id, grTaskXfer, iWarp)) {
        return 0;
    }
    ord = lpfl->lpplord->rgord()[iord];
    piaCur = ord.txp.rgia;
    tutor.idh = idhWaypointTaskTile;
    i = 0;
    while (i < 5) {
        if (piaCur->iAction() != lpiaGoal->iAction()) {
            if (piaCur->iAction() == iActionNone)
                goto LReturn;
            TutorError(idsTutorialHaveGivenIncorrectTransferOrderPlease);
            goto LReturn;
        }
        if ((piaCur->iAction() == iActionUnloadExact || piaCur->iAction() == iActionSetAmount) && piaCur->cQuan() != lpiaGoal->cQuan())
            goto LReturn;
        i++;
        piaCur++;
        lpiaGoal++;
    }
    fRet = 1;
LReturn:
    idh = tutor.idh;
    if (!fRet && FCheckSelection(grobjFleet, ifl)) {
        tutor.idh = static_cast<HelpContextId>(idh);
    }
    if (fRet) {
        tutor.idh = static_cast<HelpContextId>(idhSav);
    }
    return fRet;
}

int16_t FCheckQueue(int16_t ipl, int16_t iprod, GrobjClass grobj, uint16_t iItem, uint16_t cItem, uint16_t fNoResearch) {
    int16_t fRet;
    PLANET *lppl;
    PROD    prod;
    int16_t idh;
    int16_t idhSav;

    idhSav = tutor.idh;
    fRet = 0;
    if (game.turn < 2) {
        tutor.idh = idhProductionTile;
    } else {
        tutor.idh = idhProductionDialog;
    }
    lppl = LpplFromId(ipl);
    if (!lppl || !lppl->lpplprod || lppl->lpplprod->iprodMac <= iprod)
        goto LReturn;
    prod = lppl->lpplprod->rgprod()[iprod];
    if (prod.grobj != (uint32_t)grobj || prod.iItem != (uint32_t)iItem) {
        TutorError(idsTutorialProductionQueueDoesContainRequestedItem);
    } else if (prod.cItem != (uint32_t)cItem) {
        TutorError(idsTutorialProductionQueueDoesContainRightCount);
    } else if (fNoResearch != 0xffff && lppl->fNoResearch != (uint32_t)fNoResearch) {
        TutorError(idsTutorialContributeLeftoverCheckboxProductionQueu);
    } else {
        fRet = 1;
    }
LReturn:
    idh = tutor.idh;
    if (!fRet && FCheckSelection(grobjPlanet, ipl)) {
        tutor.idh = static_cast<HelpContextId>(idh);
    }
    if (fRet) {
        tutor.idh = static_cast<HelpContextId>(idhSav);
    }
    return fRet;
}

int16_t FCheckBtlPlan(int16_t ibp, uint16_t imdTarget, int16_t fSpread, int16_t fBomb, int16_t fDump, uint16_t mdUnarmed, uint16_t mdScout, uint16_t mdWar,
                      uint16_t mdBomber) {
    BTLPLAN *lpbtlplan;
    int16_t  idhSav;

    idhSav = tutor.idh;
    tutor.idh = idhChangingTheContentsOfABattlePlan;
    if (ibp < 0 || ibp > rgcbtlplan[0]) {
        return 0;
    }
    lpbtlplan = rglpbtlplan[0] + ibp;
    tutor.idh = static_cast<HelpContextId>(idhSav);
    return 1;
}

int16_t FCheckCargo(FLEET *lpfl, int16_t wtMin1, int16_t wtMin2, int16_t wtMin3, int16_t wtColonists) {
    int16_t fRet;
    int16_t idh;
    int16_t idhSav;

    idhSav = tutor.idh;
    fRet = 0;
    if (!lpfl) {
        return 0;
    }
    tutor.idh = idhCargoTransferDialogs;
    if ((wtMin1 == 0 && lpfl->rgwtMin[0] != 0) || (wtMin2 == 0 && lpfl->rgwtMin[1] != 0) || (wtMin3 == 0 && lpfl->rgwtMin[2] != 0) ||
        (wtColonists == 0 && lpfl->rgwtMin[3] != 0)) {
        TutorError(idsTutorialHaveLoadedWrongCargoFleetPlease);
        goto LReturn;
    }
    if (lpfl->rgwtMin[3] == wtColonists && lpfl->rgwtMin[0] == wtMin1 && lpfl->rgwtMin[1] == wtMin2 && lpfl->rgwtMin[2] == wtMin3) {
        fRet = 1;
    }
LReturn:
    idh = tutor.idh;
    if (!fRet && FCheckSelection(grobjFleet, lpfl->id)) {
        tutor.idh = static_cast<HelpContextId>(idh);
    }
    if (fRet) {
        tutor.idh = static_cast<HelpContextId>(idhSav);
    }
    return fRet;
}

int16_t FCheckBuilderPart(int16_t iSlot, HS *phs, uint16_t cInit) {
    uint16_t cItemAct;
    int16_t  idhSav;

    idhSav = tutor.idh;
    if (!hwndSlotDlg) {
        tutor.idh = idhShipDesigner;
        return 0;
    }
    tutor.idh = idhEditingAnExistingShipDesign;
    if (mdBuild != mdBuildEdit) {
        return 0;
    }
    cItemAct = lpshdefBuild->hul.rghs[iSlot].cItem;
    if (phs->cItem == 0 && cItemAct == 0) {
        return 1;
    }
    if (cInit < phs->cItem) {
        if (cItemAct < cInit || cItemAct > phs->cItem) {
        BadCnt:
            TutorError(idsTutorialHaventPlacedCorrectNumberComponentsSlot);
        BadCntSilent:
            tutor.idh = idhDesigningANewShipFromScratch;
            return 0;
        }
    } else if (cItemAct < phs->cItem || cItemAct > cInit) {
        goto BadCnt;
    }
    if (cItemAct != phs->cItem)
        goto BadCntSilent;
    if (phs->grhst != lpshdefBuild->hul.rghs[iSlot].grhst || phs->iItem != lpshdefBuild->hul.rghs[iSlot].iItem) {
        TutorError(idsTutorialHavePlacedWrongComponentSlotDrag);
        goto BadCntSilent;
    }
    tutor.idh = static_cast<HelpContextId>(idhSav);
    return 1;
}

int16_t FCheckShipBuilder(int16_t iCategory, int16_t iShip) {
    int16_t iSel;
    int16_t idhSav;

    idhSav = tutor.idh;
    tutor.idh = idhShipDesigner;
    if (!hwndSlotDlg) {
        return 0;
    }
    if (iCategory != iCategoryAny && iCategory != mdBuild) {
        return 0;
    }
    iSel = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwndSlotDlg, IDC_COMBOBOX), win32::CB_GETCURSEL, 0, 0));
    if (iShip == iShipAny || iShip == iSel) {
        tutor.idh = static_cast<HelpContextId>(idhSav);
        return 1;
    }
    return 0;
}

int16_t FTutorialEnabledShipBuilder(TutorShipBuilderAction itutsbAction) {
    HS hs2;
    HS hs3;
    HS hs;
    HS hs1;
    HS hs4;

    switch (itutsbAction) {
    default:
        return 0;
    case tutsbDelete:
        TutorError(idsTutorialShouldDeleteShipDesignPointTutorial);
        return 0;
    case tutsbCopy:
        switch (game.turn) {
        default:
        NoCustom:
            TutorError(idsTutorialShouldCustomizeShipDesignPointTutorial);
            return 0;
        case 13:
            if (tutor.idt != idtSelectAvailableHullTypes)
                goto NoCustom;
            if (rgplr[0].cShDef == 7) {
                TutorError(idsTutorialHaveAlreadyCopiedAppropriateShipDesign);
                return 0;
            }
            if (FCheckShipBuilder(1, 7))
                break;
            TutorError(idsTutorialDontHaveCorrectHullSelectedHull);
            return 0;
        case 20:
            if (tutor.idt != idtReadFirstTwoMessagesSendArmedProbe)
                goto NoCustom;
            if (rgplr[0].cshdefSB == 2) {
                TutorError(idsTutorialHaveAlreadyCopiedAppropriateShipDesign);
                return 0;
            }
            if (FCheckShipBuilder(0, 0) && fStarbaseMode)
                break;
            TutorError(idsTutorialHaveTriedCopyWrongShipDesign);
            return 0;
        case 22:
            if (tutor.idt != idtAddTeamsterStoveTopsQueue)
                goto NoCustom;
            if (rgplr[0].cShDef == 8) {
                TutorError(idsTutorialHaveAlreadyCopiedAppropriateShipDesign);
                return 0;
            }
            if (FCheckShipBuilder(1, 3))
                break;
            TutorError(idsTutorialDontHaveCorrectHullSelectedHull);
            return 0;
        case 27:
            if (tutor.idt != idtWeWantPowerfulWeAlsoWantWeigh)
                goto NoCustom;
            if (rgplr[0].cShDef == 9) {
                TutorError(idsTutorialHaveAlreadyCopiedAppropriateShipDesign);
                return 0;
            }
            if (FCheckShipBuilder(1, 4))
                break;
            TutorError(idsTutorialDontHaveCorrectHullSelectedHull);
            return 0;
        case 29:
            if (tutor.idt != idtLetsFinishOffBerserkersOnceBuildingBombing)
                goto NoCustom;
            if (rgplr[0].cShDef == 10) {
                TutorError(idsTutorialHaveAlreadyCopiedAppropriateShipDesign);
                return 0;
            }
            if (!FCheckShipBuilder(1, 8)) {
                TutorError(idsTutorialDontHaveCorrectHullSelectedHull);
                return 0;
            }
        }
        return 1;
    case tutsbEdit:
        if (game.turn != 25 || tutor.idt != idtReadFirstTwoMessages)
            goto NoCustom;
        /* The original broke out of the switch and fell off the end of the
           function, returning FCheckShipBuilder's TRUE in AX. */
        if (FCheckShipBuilder(0, 2))
            return 1;
        TutorError(idsTutorialDontHaveCorrectShipSelectedShip);
        return 0;
    case tutsbAccept:
        switch (game.turn) {
        default:
            goto NoCustom;
        case 13:
            hs.grhst = hstScanner;
            hs.iItem = iscannerRhinoScanner;
            hs.cItem = 1;
            hs2.grhst = hstEngine;
            hs2.iItem = iengineLongHump6;
            hs2.cItem = 1;
            hs3.grhst = hstMining;
            hs3.iItem = iminingRoboMiner;
            hs3.cItem = 1;
            if (tutor.idt != idtShipDesignNameImageJustFine) {
                TutorError(idsTutorialHaventYetFinishedCreatingNewDesign);
                return 0;
            }
            if (FCheckBuilderPart(0, &hs2, 1) && FCheckBuilderPart(1, &hs, 1) && FCheckBuilderPart(2, &hs3, 1) && FCheckBuilderPart(3, &hs3, 1))
                break;
            TutorError(idsTutorialDontHaveRightPartsDesignVerify);
            return 0;
        case 20:
            if (tutor.idt == idtReadFirstTwoMessagesSendArmedProbe) {
                hs.grhst = hstSpecialSB;
                hs.iItem = ispecialSBStargate100250;
                hs.cItem = 1;
                if (!FCheckBuilderPart(0, &hs, 1)) {
                    TutorError(idsTutorialHaventAddedRightPartDesignVerify);
                    return 0;
                }
                if (_stricmp(TextIO().pfnCompressed(idsGater), lpshdefBuild->hul.szClass) != 0) {
                    TutorError(idsTutorialNameDesignEditboxMustGaterChange);
                    return 0;
                }
                if (lpshdefBuild->hul.ibmp == 137)
                    break;
                TutorError(idsTutorialHaventPickedCorrectImageDesignPress);
                return 0;
            }
            TutorError(idsTutorialHaventYetFinishedCreatingNewDesign);
            return 0;
        case 22:
            if (tutor.idt == idtAddTeamsterStoveTopsQueue) {
                hs.grhst = hstEngine;
                hs.iItem = iengineDaddyLongLegs7;
                hs.cItem = 1;
                hs1.grhst = hstMines;
                hs1.iItem = iminesMineDispenser50;
                hs1.cItem = 3;
                if (!FCheckBuilderPart(0, &hs, 1) || !FCheckBuilderPart(2, &hs1, 3)) {
                    TutorError(idsTutorialDontHaveRightPartsDesignVerify2);
                    return 0;
                }
                if (_stricmp(TextIO().pfnCompressed(idsMineLayer), lpshdefBuild->hul.szClass) == 0)
                    break;
                TutorError(idsTutorialNameDesignEditboxMustMineLayer);
                return 0;
            }
            TutorError(idsTutorialHaventYetFinishedCreatingNewDesign);
            return 0;
        case 25:
            hs.grhst = hstEngine;
            hs.iItem = iengineDaddyLongLegs7;
            hs.cItem = 1;
            hs2.grhst = hstSpecialM;
            hs2.iItem = ispecialMColonizationModule;
            hs2.cItem = 1;
            if (FCheckBuilderPart(0, &hs, 1) && FCheckBuilderPart(1, &hs2, 1))
                break;
            TutorError(idsTutorialDontHaveRightPartsDesignVerify);
            return 0;
        case 27:
            if (tutor.idt == idtWeWantPowerfulWeAlsoWantWeigh) {
                hs.grhst = hstEngine;
                hs.iItem = iengineRadiatingHydroRamScoop;
                hs.cItem = 1;
                hs1.grhst = hstSpecialM;
                hs1.iItem = ispecialMFuelTank;
                hs1.cItem = 1;
                hs2.grhst = hstSpecialE;
                hs2.iItem = ispecialEBattleComputer;
                hs2.cItem = 1;
                hs3.grhst = hstBeam;
                hs3.iItem = ibeamYakimoraLightPhaser;
                hs3.cItem = 1;
                hs4.grhst = hstArmor;
                hs4.iItem = iarmorCarbonicArmor;
                hs4.cItem = 2;
                if (!FCheckBuilderPart(0, &hs, 1) || !FCheckBuilderPart(1, &hs3, 1) || !FCheckBuilderPart(2, &hs3, 1) || !FCheckBuilderPart(3, &hs3, 1) ||
                    !FCheckBuilderPart(4, &hs4, 2) || !FCheckBuilderPart(5, &hs1, 1) || !FCheckBuilderPart(6, &hs2, 1)) {
                    TutorError(idsTutorialVerifyHaveRadiatingHydroRamScoop);
                    return 0;
                }
                if (lpshdefBuild->hul.ibmp == 25)
                    break;
                TutorError(idsTutorialHaventPickedCorrectImageDesignPress);
                return 0;
            }
            TutorError(idsTutorialHaventYetFinishedCreatingNewDesign);
            return 0;
        case 29:
            if (tutor.idt == idtLetsFinishOffBerserkersOnceBuildingBombing) {
                hs.grhst = hstEngine;
                hs.iItem = iengineRadiatingHydroRamScoop;
                hs.cItem = 2;
                hs1.grhst = hstSpecialM;
                hs1.iItem = ispecialMFuelTank;
                hs1.cItem = 1;
                hs2.grhst = hstBomb;
                hs2.iItem = ibombBlackCatBomb;
                hs2.cItem = 4;
                if (!FCheckBuilderPart(0, &hs, 2) || !FCheckBuilderPart(1, &hs2, 4) || !FCheckBuilderPart(2, &hs2, 4) || !FCheckBuilderPart(3, &hs1, 1)) {
                    TutorError(idsTutorialDontHaveRightPartsDesignVerify3);
                    return 0;
                }
                break;
            }
            TutorError(idsTutorialHaventYetFinishedCreatingNewDesign);
            return 0;
        }
        return 1;
    case tutsbCancelEdit:
        TutorError(idsTutorialMustFinishTutorialTasksBeforeExiting);
        return 0;
    }
}

int16_t FOKMergeDialog() {
    if (game.turn != 23) {
        if (game.turn != 27) {
            TutorError(idsTutorialHaventAskedMergeAnyFleetsYear);
            return 0;
        }
        if (*vrgiflMerge == iflNone && vrgiflMerge[1] == iflNone && vrgiflMerge[2] != iflNone && vrgiflMerge[3] != iflNone) {
            return 1;
        }
    } else if (*vrgiflMerge != iflNone && vrgiflMerge[1] != iflNone && vrgiflMerge[2] == iflNone && vrgiflMerge[3] == iflNone) {
        return 1;
    }
    TutorError(idsTutorialHaventSelectedRightFleetsMergeReread);
    tutor.idh = idhMergeFleetsDialog;
    return 0;
}
