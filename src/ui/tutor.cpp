#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
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
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HMENU   hmenu;
    RECT    rc;
    std::int16_t fRet;

    switch (message) {
    case win32::WM_INITDIALOG:
        app.state.tutor.hwnd = hwnd;
        ControlIO().pfnSetWindowPos(hwnd, reinterpret_cast<HWND>(-1), 0, 0, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOMOVE);
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyTutorDlg, 1);
        return 1;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_PAINT:
        DrawTutorText(app, hwnd);
        return 1;
    case win32::WM_CHAR:
        win32::PostMessageA(app.lpui->hwndFrame, win32::WM_CHAR, wParam, lParam);
        return 0;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_TUTOR_PANIC:
            fRet = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_PANIC), hwnd, PanicDlg, 0);
            if (!fRet) {
                return 0;
            }
            app.state.tutor.fAutoComplete = fRet == 2506;
            win32::PostMessageA(app.lpui->hwndFrame, WM_STARS_CONTINUE, fRet, 0);
            return 0;
        case IDCANCEL:
            ShowTutor(app, 0);
            if (app.state.tutor.fShowHidMsg) {
                AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsMakeTutorialReappearCompleteTaskChooseTutorial, nullptr), win32::MB_ICONASTERISK);
                app.state.tutor.fShowHidMsg = 0;
            }
            return 1;
        case IDC_TUTOR_HINT:
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, static_cast<std::int16_t>(app.state.tutor.idh));
            return 1;
        default:
            return 0;
        }
    case win32::WM_DESTROY:
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyTutorDlg, 0);
        app.state.tutor.hwnd = nullptr;
        hmenu = GetASubMenu(app, app.lpui->hwndFrame, menuHelp);
        win32::CheckMenuItem(hmenu, IDM_HELP_TUTORIAL, win32::MF_UNCHECKED);
        ControlIO().pfnEndDialog(hwnd, 1);
        EndTutor(app, 1);
        return 1;
    default:
        return 0;
    }
}

INT_PTR  PanicDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    RECT rc;

    switch (message) {
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
        if (message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            ControlIO().pfnEndDialog(hwnd, 0);
            return 1;
        case IDC_HELP:
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, static_cast<std::int16_t>(app.state.tutor.idh));
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

void ShowTutor(APPSESSION &app, std::int16_t fShow) {
    if (app.state.tutor.hwnd) {
        // Instruction updates call this while the tutor is already visible;
        // activating it again takes focus from the control the player is using.
        if (!fShow || !win32::IsWindowVisible(app.state.tutor.hwnd))
            ui::ShowWindow(app.state.tutor.hwnd, !fShow ? win32::SW_HIDE : win32::SW_SHOW);
        app.state.tutor.fVisible = fShow;
    }
    return;
}

void DrawTutorText(APPSESSION &app, HWND hwnd) {
    HDC         hdc;
    std::int16_t     yTop;
    std::int16_t     fPara;
    PAINTSTRUCT ps;
    std::int16_t     didt;
    std::int16_t     xLeft;
    std::int16_t     cch;
    char        rgch[256];
    RECT        rcBtn;
    RECT        rc;

    hdc = win32::BeginPaint(hwnd, &ps);
    win32::SetBkColor(hdc, app.lpui->crButtonFace);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
    win32::GetWindowRect(hwnd, &rc);
    win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDCANCEL), &rcBtn);
    rc.bottom = rcBtn.top;
    win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rc));
    win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rc.right));
    rc.top += app.state.dyArial8 * 2;
    rc.bottom -= static_cast<std::int16_t>((app.state.dyArial8 * 2)) / 3;
    rc.left += static_cast<std::int16_t>((app.state.dyArial8 * 2)) / 3;
    rc.right -= static_cast<std::int16_t>((app.state.dyArial8 * 2)) / 3;
    win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
    win32::PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, 1, win32::PATCOPY);
    win32::PatBlt(hdc, rc.left, rc.top, 1, rc.bottom - rc.top, win32::PATCOPY);
    win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
    win32::PatBlt(hdc, rc.left, rc.bottom - 1, rc.right - rc.left, 1, win32::PATCOPY);
    win32::PatBlt(hdc, rc.right - 1, rc.top, 1, rc.bottom - rc.top, win32::PATCOPY);
    ExpandRc(&rc, -(app.state.dyArial8 / 2), -(app.state.dyArial8 / 2));
    yTop = rc.top;
    win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
    win32::SetTextColor(hdc, app.lpui->crButtonText);
    for (didt = 0; didt < 8; didt++) {
        cch = CchTutorString(app, rgch, static_cast<TutorId>(app.state.tutor.idt + didt));
        if (cch == 1)
            break;
        fPara = std::isupper(rgch[0]);
        if (fPara) {
            xLeft = rc.left;
            if (didt != 0) {
                yTop += app.state.dyArial8 / 2;
            }
        }
        if (app.state.tutor.idt + didt == app.state.tutor.idtBold) {
            win32::SetTextColor(hdc, app.lpui->crButtonFace);
            win32::SetBkColor(hdc, app.lpui->crButtonText);
        }
        WrapTextOut(app, hdc, &xLeft, &yTop, rgch, cch, rc.left, rc.right - rc.left, nullptr, didt != 0 && fPara != 0, 1);
        if (app.state.tutor.idt + didt == app.state.tutor.idtBold) {
            win32::SetTextColor(hdc, app.lpui->crButtonText);
            win32::SetBkColor(hdc, app.lpui->crButtonFace);
        }
    }
    win32::EndPaint(hwnd, &ps);
    return;
}

void StartTutor(APPSESSION &app, std::int16_t fRestart) {
    std::int16_t cx;
    std::int16_t cch;

    if (!app.state.gd.fTutorial) {
        app.state.tutor = {};
        if (!app.lpui->lpfnTutorDlgProc) {
            app.lpui->lpfnTutorDlgProc = TutorDlg;
        }
        app.state.gd.fTutorial = 1;
        SaveGameState(app);
        if (!app.lpui->fFreeingTitle) {
            app.lpui->fFreeingTitle = 1;
            ControlIO().pfnDestroyWindow(app.lpui->hwndTitle);
            app.lpui->hwndTitle = nullptr;
            ui::ShowWindow(app.lpui->hwndFrame, win32::SW_SHOW);
        }
        app.lpui->grbitScan = static_cast<GrbitScan>(grbitScanCoverage | grbitScanMineFields | grbitScanFleetPaths | grbitScanPlanetNames);
        cx = win32::GetSystemMetrics(win32::SM_CXSCREEN);
        if (cx >= 1280) {
            app.state.iScanZoom = zoom200;
        } else if (cx >= 1024) {
            app.state.iScanZoom = zoom150;
        } else if (cx >= 800) {
            app.state.iScanZoom = zoom125;
        } else {
            app.state.iScanZoom = zoom100;
        }
        if (app.state.game.lid == 0) {
            cch = TextIO().pfnCchGetString(app, idsTutorial, std::span{app.state.szBase});
            if (!fRestart) {
                std::strcat(app.state.szBase, ".xy");
                if (FFileExists(app.state.szBase) &&
                    AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsTutorialHasRunBeforeWouldLikeDestroy, nullptr), win32::MB_YESNO | win32::MB_ICONQUESTION | win32::MB_TASKMODAL) != win32::IDYES) {
                    app.state.szBase[cch] = 0;
                    std::strcat(app.state.szBase, ".m1");
                    app.state.ini.fStartupFile = 1;
                    if (FOpenGame(app, app.lpui->hwndFrame, 0) > 0) {
                        CreateChildWindows(app);
                    }
                    app.state.ini.fStartupFile = 0;
                }
                app.state.szBase[cch] = 0;
            }
            if (app.state.game.lid == 0) {
                CreateTutorWorld(app);
                app.state.vrgZipProd[0].SetZpq1({});
                app.state.vrgZipProd[0].fValid = 1;
                app.state.gd.fChgZipProd = 1;
            }
            InitializeMenu(app, nullptr);
            win32::PostMessageA(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_FRAME_POST_OPEN, 0);
            if (!app.lpui->fFreeingTitle) {
                app.lpui->fFreeingTitle = 1;
                ControlIO().pfnDestroyWindow(app.lpui->hwndTitle);
                app.lpui->hwndTitle = nullptr;
            }
            ui::ShowWindow(app.lpui->hwndFrame, win32::SW_SHOW);
        }
        app.state.tutor.idsError = -1;
        app.state.tutor.fShowHidMsg = 1;
        app.state.tutor.idt = idtWelcomeStarsTutorialWillGuideThrough36;
        app.state.tutor.idtBold = idtWelcomeStarsTutorialWillGuideThrough36;
        app.state.tutor.fProgress = 0;
        while (FTutorTaskDone(app) && !app.state.tutor.fTurnDone) {
            app.state.tutor.idt = static_cast<TutorId>(app.state.tutor.idt + 8);
        }
        if (app.state.tutor.fTutorDone) {
            EndTutor(app, 0);
            return;
        }
        if (!app.state.tutor.hwnd) {
            ui::CreateDialog(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_TUTOR), app.lpui->hwndFrame, app.lpui->lpfnTutorDlgProc);
        }
        if (app.state.tutor.idt != idtWelcomeStarsTutorialWillGuideThrough36) {
            if (!app.state.tutor.fTurnDone) {
                app.state.tutor.idt = static_cast<TutorId>(app.state.tutor.idt - 8);
            }
            app.state.tutor.idtBold = app.state.tutor.idt;
            PresentationIO().pfnTutor(app);
        }
    }
    ShowTutor(app, 1);
    ControlIO().pfnInvalidateRect(app.state.tutor.hwnd, nullptr, 1);
    return;
}

void AdvanceTutor(APPSESSION &app) {
    char    szTitle[50];
    std::int16_t fRedraw;
    std::int16_t idtT;
    std::int16_t fTaskDone;
    RECT    rc;

    fRedraw = 0;
    app.state.tutor.fChange = 0;
    idtT = app.state.tutor.idtBold;
    fTaskDone = FTutorTaskDone(app);
    fRedraw = idtT != app.state.tutor.idtBold;
    if (!fTaskDone) {
        if (fRedraw)
            goto LUpdatePage;
        return;
    }
    if (!app.state.tutor.fTurnDone) {
    SkipToNext:
        app.state.tutor.idt = static_cast<TutorId>(app.state.tutor.idt + 8);
        app.state.tutor.idtBold = app.state.tutor.idt;
        app.state.tutor.idsError = -1;
        app.state.tutor.fNoErrors = 1;
        app.state.tutor.fProgress = 0;
        if (FTutorTaskDone(app) && !app.state.tutor.fTurnDone)
            goto SkipToNext;
        fRedraw = 1;
        app.state.tutor.fNoErrors = 0;
    } else {
        app.state.tutor.idh = static_cast<HelpContextId>(3510);
    }
    if (static_cast<std::int16_t>(app.state.tutor.idt) >= 640 || app.state.tutor.fTutorDone) {
        if (app.state.tutor.idsError != 522) {
            TutorError(app, idsTutorialFinishedCanContinuePlayGameStart);
        }
        EndTutor(app, 0);
        return;
    }
    if (!fRedraw) {
        return;
    }
LUpdatePage:
    win32::wsprintfA(szTitle, TextIO().pfnCompressed(app, idsStarsTutorPageD80).data(), static_cast<std::int16_t>(app.state.tutor.idt) / 8 + 1);
    ControlIO().pfnSetWindowText(app.state.tutor.hwnd, szTitle);
    ShowTutor(app, 1);
    win32::GetWindowRect(app.state.tutor.hwnd, &rc);
    win32::ScreenToClient(app.state.tutor.hwnd, reinterpret_cast<POINT *>(&rc));
    win32::ScreenToClient(app.state.tutor.hwnd, reinterpret_cast<POINT *>(&rc.right));
    ExpandRc(&rc, -app.state.dyArial8, -2 * app.state.dyArial8);
    ControlIO().pfnInvalidateRect(app.state.tutor.hwnd, &rc, 1);
    return;
}

void EndTutor(APPSESSION &app, std::int16_t fClose) {
    if (app.state.gd.fTutorial) {
        app.state.gd.fTutorial = 0;
        if (app.state.tutor.hwnd) {
            ControlIO().pfnDestroyWindow(app.state.tutor.hwnd);
        }
        app.state.game.fTutorial = 0;
        if (fClose) {
            RestoreGameState(app);
        } else {
            app.state.tutor.fFreeing = 1;
        }
        app.state.tutor = {};
        Randomize2(app, win32::GetTickCount());
    }
    return;
}

void SaveGameState(APPSESSION &app) {
    HMENU hmenu;

    app.state.tutor.fGameSaved = 0;
    app.state.tutor.grbitScan = app.lpui->grbitScan;
    app.state.tutor.iScanZoom = app.state.iScanZoom;
    app.state.tutor.fTBVis = app.state.gd.fToolbar;
    app.state.tutor.zpq = app.state.vrgZipProd[0].zpq1();
    app.state.tutor.fValidQ = app.state.vrgZipProd[0].fValid;
    app.state.vrgZipProd[0].SetZpq1(app.state.vrgZipProd[4].zpq1());
    app.state.vrgZipProd[0].fValid = app.state.vrgZipProd[4].fValid;
    if (!app.state.gd.fToolbar) {
        hmenu = GetASubMenu(app, app.lpui->hwndFrame, menuView);
        app.state.gd.fToolbar = app.state.gd.fToolbar == 0;
        win32::CheckMenuItem(hmenu, IDM_VIEW_TOOLBAR, !app.state.gd.fToolbar ? win32::MF_UNCHECKED : win32::MF_CHECKED);
        ControlIO().pfnRefitFrameChildren(app);
    }
    app.state.tutor.icolFSort = app.state.vrptFleet.icolSort;
    if (app.state.vrptFleet.icolSort != 1 || !app.state.vrptFleet.fAscending) {
        app.state.vrptFleet.icolSort = 1;
        app.state.vrptFleet.fAscending = 1;
        PresentationIO().pfnInvalidateReport(app, rptFleets, 0);
    }
    if (app.state.game.lid != 0 && !app.state.game.fTutorial) {
        DestroyCurGame(app);
    }
    return;
}

void RestoreGameState(APPSESSION &app) {
    HMENU hmenu;

    if (app.state.tutor.fGameSaved) {
    }
    app.lpui->grbitScan = static_cast<GrbitScan>(app.state.tutor.grbitScan);
    app.state.iScanZoom = app.state.tutor.iScanZoom;
    app.state.vrgZipProd[4].SetZpq1(app.state.vrgZipProd[0].zpq1());
    app.state.vrgZipProd[4].fValid = app.state.vrgZipProd[0].fValid;
    app.state.vrgZipProd[0].SetZpq1(app.state.tutor.zpq);
    app.state.vrgZipProd[0].fValid = app.state.tutor.fValidQ;
    if (app.state.gd.fToolbar != app.state.tutor.fTBVis) {
        hmenu = GetASubMenu(app, app.lpui->hwndFrame, menuView);
        app.state.gd.fToolbar = app.state.gd.fToolbar == 0;
        win32::CheckMenuItem(hmenu, IDM_VIEW_TOOLBAR, !app.state.gd.fToolbar ? win32::MF_UNCHECKED : win32::MF_CHECKED);
        ControlIO().pfnRefitFrameChildren(app);
    }
    if (app.state.vrptFleet.icolSort != app.state.tutor.icolFSort) {
        app.state.vrptFleet.icolSort = app.state.tutor.icolFSort;
        PresentationIO().pfnInvalidateReport(app, rptFleets, 1);
    }
    return;
}

std::int16_t FAskKillTutor(APPSESSION &app) {
    if (app.state.game.turn >= 30 || AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsCurrentlyRunningStarsTutorialDoWantExit, nullptr), win32::MB_YESNO | win32::MB_ICONQUESTION | win32::MB_TASKMODAL) == win32::IDYES) {
        EndTutor(app, 1);
        return 1;
    }
    return 0;
}

std::int16_t FTutorTaskDone(APPSESSION &app) {
    HS hs1;
    HS hs;
    HS hs2;

    switch (app.state.game.turn) {
    case 0:
        switch (app.state.tutor.idt) {
        case idtWelcomeStarsTutorialWillGuideThrough36:
            app.state.tutor.idtBold = idtReadMessages;
            return FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0);
        case idtExamineTilesCommandPaneUpperLeftPortion:
            app.state.tutor.idtBold = idtPressTilesGotoButtonCommandArmedProbe;
            if (!FCheckSelection(app, grobjFleet, 0)) {
                return 0;
            }
            app.state.tutor.idtBold = idtHoldShiftKeyClickLeftMouseButton;
            return FCheckFleetWP(app, 0, 1, grobjPlanet, 12, grTaskNone, iWarpAny);
        case idtAccordingFleetWaypointsTileWillTake2:
            app.state.tutor.idtBold = idtHitNKeyLookFleet;
            if (!FCheckSelection(app, grobjFleet, 1)) {
                app.state.tutor.idh = idhKeyboardShortcuts;
                return 0;
            }
            app.state.tutor.idtBold = idtHoldShiftKeyLeftClickPlanet90210;
            return FCheckFleetWP(app, 1, 1, grobjPlanet, 16, grTaskNone, iWarpAny);
        case idtLetsMoveOurFleet:
            if (FCheckSelection(app, grobjFleet, 4)) {
                app.state.tutor.idtBold = idtHoldShiftKeySelectAlexander;
                return FCheckFleetWP(app, 4, 1, grobjPlanet, 15, grTaskNone, iWarpAny);
            }
            if (FCheckSelection(app, grobjFleet, 3)) {
                app.state.tutor.idtBold = idtPress2;
            } else if (FCheckSelection(app, grobjFleet, 2)) {
                app.state.tutor.idtBold = idtPress;
            } else {
                app.state.tutor.idtBold = idtTimePressButtonTileShowingLongRange;
            }
            return 0;
        case idtHitNKey:
            if (FCheckSelection(app, grobjFleet, 4) && !app.state.tutor.fProgress && !app.state.tutor.fAutoComplete) {
                app.state.tutor.idtBold = idtHitNKey;
            } else if (FCheckSelection(app, grobjFleet, 5) && !app.state.tutor.fProgress && !app.state.tutor.fAutoComplete) {
                app.state.tutor.idtBold = idtHitNKey2;
            } else {
                if (FCheckResearch(app, Weapons, TechFieldCount, 15)) {
                    app.state.tutor.idtBold = idtThatsTurnHitF9GenerateYear;
                    app.state.tutor.fTurnDone = 1;
                    return 1;
                }
                app.state.tutor.idtBold = app.lpui->pctResGlob == -1 ? idtChooseResearchCommandsMenu : idtChangeFieldStudyWeaponsPressDone;
            }
            return 0;
        default:
            return 0;
        }
    case 1:
        if (app.state.tutor.idt != idtReadMessageMessagesPaneWeveGotPlenty) {
            return 1;
        }
        if (!FCheckQueue(app, 13, 0, grobjPlanet, mdIdleFactory, 20, 0)) {
            app.state.tutor.idtBold = !app.state.lpplProdGlob ? idtPressChangeButtonProductionTile : idtSelectFactoryLeftHandListboxHoldShift;
            return 0;
        }
        app.state.tutor.idtBold = idtHitF9KeyGenerateYear;
        app.state.tutor.fTurnDone = 1;
        return 1;
    case 2:
        switch (app.state.tutor.idt) {
        case idtReadFirstMessagePressGotoMessagesPane:
            if (FCheckFleetWP(app, 1, 1, grobjPlanet, 21, grTaskNone, iWarpAny)) {
                return 1;
            }
            app.state.tutor.idtBold = !FCheckSelection(app, grobjFleet, 0) ? idtReadFirstMessagePressGotoMessagesPane : idtHoldShiftKeyLeftClickHiho;
            if (!FCheckFleetWP(app, 0, 1, grobjPlanet, 9, grTaskNone, iWarpAny)) {
                return 0;
            }
            app.state.tutor.idtBold = idtVacancy;
            if (!FCheckFleetWP(app, 0, 2, grobjPlanet, 3, grTaskNone, iWarpAny)) {
                return 0;
            }
            app.state.tutor.idtBold = idtSlime;
            if (!FCheckFleetWP(app, 0, 3, grobjPlanet, 8, grTaskNone, iWarpAny)) {
                return 0;
            }
            app.state.tutor.idtBold = idtWallaby;
            if (!FCheckFleetWP(app, 0, 4, grobjPlanet, 5, grTaskNone, iWarpAny)) {
                return 0;
            }
            app.state.tutor.idtBold = idtOxygen;
            if (!FCheckFleetWP(app, 0, 5, grobjPlanet, 2, grTaskNone, iWarpAny)) {
                return 0;
            }
            app.state.tutor.idtBold = idtReadMessagePressGotoCommandLongRange;
            return FCheckSelection(app, grobjFleet, 1);
        case idtWeWantSendFleetExploreAreaAbove:
            if (FCheckFleetWP(app, 4, 1, grobjPlanet, 14, grTaskNone, iWarpAny)) {
                return 1;
            }
            app.state.tutor.idtBold = idtHoldShiftKeySelectDwarte;
            if (!FCheckFleetWP(app, 1, 1, grobjPlanet, 21, grTaskNone, iWarpAny)) {
                return 0;
            }
            app.state.tutor.idtBold = idtMobius;
            if (!FCheckFleetWP(app, 1, 2, grobjPlanet, 19, grTaskNone, iWarpAny)) {
                return 0;
            }
            app.state.tutor.idtBold = idtCastle;
            if (!FCheckFleetWP(app, 1, 3, grobjPlanet, 20, grTaskNone, iWarpAny)) {
                return 0;
            }
            app.state.tutor.idtBold = idtMoholdi;
            if (!FCheckFleetWP(app, 1, 4, grobjPlanet, 7, grTaskNone, iWarpAny)) {
                return 0;
            }
            app.state.tutor.idtBold = !FCheckMessages(app, 2, static_cast<MessageId>(idmNone), 0) ? idtReadMessage : idtGotoStalwartDefender5;
            return FCheckSelection(app, grobjFleet, 4);
        case idtHoldShiftKeySelectShaggyDog:
            if (FCheckFleetWP(app, 5, 1, grobjPlanet, 12, grTaskMine, iWarpAny)) {
                return 1;
            }
            app.state.tutor.idtBold = idtHoldShiftKeySelectShaggyDog;
            if (!FCheckFleetWP(app, 4, 1, grobjPlanet, 14, grTaskNone, iWarpAny)) {
                return 0;
            }
            app.state.tutor.idtBold = idtSeaSquared;
            if (!FCheckFleetWP(app, 4, 2, grobjPlanet, 17, grTaskNone, iWarpAny)) {
                return 0;
            }
            app.state.tutor.idtBold = idtRedStorm;
            if (!FCheckFleetWP(app, 4, 3, grobjPlanet, 18, grTaskNone, iWarpAny)) {
                return 0;
            }
            app.state.tutor.idtBold = idtBloop;
            if (!FCheckFleetWP(app, 4, 4, grobjPlanet, 23, grTaskNone, iWarpAny)) {
                return 0;
            }
            app.state.tutor.idtBold = idtKalamazoo;
            if (!FCheckFleetWP(app, 4, 5, grobjPlanet, 22, grTaskNone, iWarpAny)) {
                return 0;
            }
            app.state.tutor.idtBold = !FCheckMessages(app, 4, static_cast<MessageId>(idmNone), 0) ? idtReadTwoMessages : idtPressGotoDisplayStatsPruneSummaryPane;
            return FCheckSummary(app, grobjPlanet, 12);
        case idtTopGraphSummaryPaneShowsPruneHas:
            if (FCheckColonizeWP(app, 2, 16, iWarpAny)) {
                return 1;
            }
            if (FCheckFleetWP(app, 5, 1, grobjPlanet, 12, grTaskMine, iWarpAny)) {
                app.state.tutor.fProgress = 0;
                return 1;
            }
            if (FCheckFleetWP(app, 5, 1, grobjPlanet, 12, grTaskNone, iWarpAny)) {
                app.state.tutor.idtBold = idtClickDropdownChangeTaskRemoteMining;
            } else if (FCheckSelection(app, grobjFleet, 5)) {
                app.state.tutor.idtBold = idtShiftClickPrune;
            } else {
                app.state.tutor.idtBold = idtClickRightMouseButtonStoveTopSelect;
            }
            return 0;
        case idtMoveMessageGotoAlexander:
            if (FCheckColonizeWP(app, 2, 16, iWarpAny)) {
                return 1;
            }
            if (FCheckSummary(app, grobjPlanet, 16)) {
                return 1;
            }
            if (FCheckSummary(app, grobjPlanet, 15)) {
                if (FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
                    app.state.tutor.idtBold = idtGotoPlanet90210;
                } else {
                    app.state.tutor.idtBold = !app.state.tutor.fProgress ? idtClickVariousPlacesSummaryPaneGetPopup : idtReadMessage2;
                }
            }
            return 0;
        case idtSince90210FinePlanetHighMineralConcentrations:
            if (!FCheckCargo(app, LpflFromId(app, 2), 0, 0, 0, 25)) {
                if (FCheckSelection(app, grobjFleet, 2)) {
                    if (app.lpui->mdXferDlg == mdXferNone) {
                        app.state.tutor.idh = idhLocationTile;
                        app.state.tutor.idtBold = idtClickXferButtonTileLabeledOrbitingStove;
                    } else {
                        app.state.tutor.idtBold = idtClickDragColonistsGaugeFillingHold25kt;
                    }
                } else {
                    app.state.tutor.idtBold = idtRightClickStoveTopSelectSantaMaria;
                }
                return 0;
            }
            if (!FCheckColonizeWP(app, 2, 16, iWarpAny)) {
                if (!FCheckFleetWP(app, 2, 1, grobjPlanet, 16, grTaskNone, iWarpAny)) {
                    app.state.tutor.idtBold = !FCheckSelection(app, grobjFleet, 2) ? idtRightClickStoveTopSelectSantaMaria : idtShiftClick90210;
                } else {
                    app.state.tutor.idtBold = idtSelectColonizeDropdownWaypointTaskTile;
                }
                return 0;
            }
            app.state.tutor.idtBold = idtHitF9GenerateYear;
            app.state.tutor.fTurnDone = 1;
            return 1;
        default:
            return 1;
        }
    case 3:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtFirstMessageQuiteCommonWeDontNeed:
            if (!FCheckMessages(app, imsgAny, idmHaveBuiltFactories, 1)) {
                app.state.tutor.idtBold = idtFilterClickingBlueCheckMarkUpperLeft;
                return 0;
            }
            if (!FCheckQueue(app, 13, 0, grobjPlanet, iobjFactory, 30, 0)) {
                if (!FCheckMessages(app, 1, static_cast<MessageId>(idmNone), 0) || !FCheckSelection(app, grobjPlanet, 13)) {
                    app.state.tutor.idtBold = idtMoveMessageGotoStoveTop;
                } else {
                    app.state.tutor.idtBold = !app.state.lpplProdGlob ? idtPressChangeButtonProductionTile2 : idtSelectFactoriesAutoBuildLeftHandListbox;
                }
                return 0;
            }
            return 1;
        case idtReadTwoMessagesGoto90210:
            if (!FCheckQueue(app, 16, 0, grobjPlanet, mdIdleFactory, 3, 1) || !FCheckQueue(app, 16, 1, grobjPlanet, mdIdleMine, 3, 1)) {
                if (!FCheckMessages(app, 3, static_cast<MessageId>(idmNone), 0) || !FCheckSelection(app, grobjPlanet, 16)) {
                    app.state.tutor.idtBold = idtReadTwoMessagesGoto90210;
                } else {
                    app.state.tutor.idtBold = !app.state.lpplProdGlob ? idtHitQKey : idtDoubleClickFactory3TimesMine3;
                }
                return 0;
            }
            if (!FCheckCargo(app, LpflFromId(app, 3), 0, 0, 0, 210)) {
                if (FCheckSelection(app, grobjFleet, 3)) {
                    app.state.tutor.idtBold = app.lpui->mdXferDlg == mdXferNone ? idtClickXferButtonCommandPane : idtFillHoldColonistsHitOk;
                } else {
                    app.state.tutor.idtBold = idtRightClickStoveTopSelectTeamster4;
                }
                return 0;
            }
            return 1;
        case idtShiftClick902102:
            if (!FCheckXferWP(app, 3, 1, 16, iWarpAny, rgiaQuikDrop)) {
                if (!FCheckFleetWP(app, 3, 1, grobjPlanet, 16, grTaskXfer, iWarpAny)) {
                    if (!FCheckFleetWP(app, 3, 1, grobjPlanet, 16, grTaskAny, iWarpAny)) {
                        app.state.tutor.idtBold = idtShiftClick902102;
                    } else {
                        app.state.tutor.idtBold = idtChangeWaypointTaskTransport;
                    }
                } else {
                    app.state.tutor.idtBold = idtRightClickBlueDiamondWaypointTaskTile;
                }
                return 0;
            }
            if (LpflFromId(app, 0)->cord < 6) {
                return 1;
            }
            if (FCheckSelection(app, grobjFleet, 0)) {
                return 1;
            }
            app.state.tutor.idtBold = !FCheckSummary(app, grobjPlanet, 9) ? idtReadMessageGotoHiho : idtDoubleClickArmedProbe1;
            return 0;
        case idtArmedProbe1DoesntNeedGoWay:
            if (LpflFromId(app, 0)->cord == 6) {
                app.state.tutor.idtBold = !FCheckSummary(app, grobjPlanet, 9) ? idtClickHiho : idtHitDeleteKey;
                return 0;
            }
            app.state.tutor.idtBold = idtSelectGenerateTurnMenu;
            app.state.tutor.fTurnDone = 1;
            app.state.tutor.fProgress = 0;
            return 1;
        }
    case 4:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoShaggyDog:
            if (LpflFromId(app, 4)->cord == 6) {
                if (FCheckSelection(app, grobjFleet, 4)) {
                    app.state.tutor.idtBold = !FCheckSummary(app, grobjPlanet, 14) ? idtSelectWaypointShaggyDog : idtPressDeleteKey;
                } else if (FCheckSummary(app, grobjPlanet, 14) || FCheckSummary(app, grobjFleet, 4)) {
                    app.state.tutor.idtBold = idtDoubleClickStalwartDefender5JustAbove;
                } else {
                    app.state.tutor.idtBold = idtReadFirstMessageGotoShaggyDog;
                }
                return 0;
            }
            if (LpplFromId(app, 13)->lpplprod->iprodMac == 3) {
                return 1;
            }
            if (FCheckSelection(app, grobjPlanet, 13)) {
                return 1;
            }
            app.state.tutor.idtBold = !FCheckSummary(app, grobjPlanet, 21) ? idtReadMessageGotoDwarte : idtDoubleClickStoveTop;
            return 0;
        case idtHitChangeButtonProductionTileOpenStove:
            if (LpplFromId(app, 13)->lpplprod->iprodMac != 3 || !FCheckQueue(app, 13, 1, grobjFleet, 2, 1, 0)) {
                app.state.tutor.idtBold = !app.state.lpplProdGlob ? idtHitChangeButtonProductionTileOpenStove : idtDoubleClickSantaMariaLeftHandListbox;
                return 0;
            }
            app.state.tutor.idtBold = idtGenerateWhenReady;
            app.state.tutor.fTurnDone = 1;
            return 1;
        }
    case 5:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoNewSantaMaria:
            if (FCheckCargo(app, LpflFromId(app, 2), 0, 0, 0, 25)) {
                if (!FCheckFleetWP(app, 2, 1, grobjPlanet, 14, grTaskAny, iWarpAny)) {
                    app.state.tutor.idtBold = !FCheckScanner(app, 3, iZoomAny) ? idtClickButtonToolbarShowPlanetsHowHabitable : idtShiftClickBigGreenShaggyDogBelow;
                    return 0;
                }
                return 1;
            }
            if (!FCheckSelection(app, grobjFleet, 2)) {
                app.state.tutor.idtBold = idtReadFirstMessageGotoNewSantaMaria;
            } else {
                app.state.tutor.idtBold = app.lpui->mdXferDlg == mdXferNone ? idtClickCargoGaugeFuelCargoTile : idtFillHoldFullColonistsHitOk;
            }
            return 0;
        case idtSetWaypointTaskColonize:
            if (!FCheckColonizeWP(app, 2, 14, iWarpAny)) {
                app.state.tutor.idtBold = idtSetWaypointTaskColonize;
                return 0;
            }
            if (!FCheckFleetWP(app, 3, 1, grobjPlanet, 13, grTaskAny, iWarpAny)) {
                if (!FCheckScanner(app, 0, iZoomAny)) {
                    app.state.tutor.idtBold = idtSwitchScannerBackNormalViewClickingLeftmost;
                } else {
                    app.state.tutor.idtBold = !FCheckSelection(app, grobjFleet, 3) ? idtRead2MessagesGotoTeamster4 : idtShiftClickStoveTopSendHome;
                }
                return 0;
            }
            if (LpflFromId(app, 0)->cord == 5) {
                if (FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
                    app.state.tutor.idtBold = idtReadMessageDeleteArmedProbe1sWaypoint;
                } else {
                    app.state.tutor.idtBold = !FCheckSelection(app, grobjPlanet, 16) ? idtSelect90210PressingGotoButtonTileLabeled : idtReadMessageDeleteArmedProbe1sWaypoint;
                }
                return 0;
            }
            app.state.tutor.idtBold = idtThatsYearGenerateWhenReady;
            app.state.tutor.fTurnDone = 1;
            return 1;
        }
    case 6:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoTeamster4:
            if (!FCheckXferWP(app, 3, 1, 12, iWarpAny, rgiaQuikLoad)) {
                if (!FCheckFleetWP(app, 3, 1, grobjPlanet, 12, grTaskXfer, iWarpAny)) {
                    if (!FCheckFleetWP(app, 3, 1, grobjPlanet, 12, grTaskAny, iWarpAny)) {
                        app.state.tutor.idtBold = !FCheckSelection(app, grobjFleet, 3) ? idtReadFirstMessageGotoTeamster4 : idtShiftClickPrune2;
                    } else {
                        app.state.tutor.idtBold = idtSetWaypointTaskTransport;
                    }
                } else {
                    app.state.tutor.idtBold = idtRightClickBlueDiamondSelectQuikloadZip;
                }
                return 0;
            }
            app.state.tutor.idtBold = idtShiftClickBackStoveTop;
            return FCheckFleetWP(app, 3, 2, grobjPlanet, 13, grTaskAny, iWarpAny);
        case idtNoticeWaypointTaskHasCopiedPreviousWaypoint:
            app.state.tutor.fNoErrors = 1;
            if (!FCheckXferWP(app, 3, 2, 13, iWarpAny, rgiaQuikDrop)) {
                app.state.tutor.fNoErrors = 0;
                app.state.tutor.idtBold = idtRightClickBlueDiamondSelectQuikdropZip;
                return 0;
            }
            app.state.tutor.fNoErrors = 0;
            app.state.tutor.idh = idhFleetWaypointsTile;
            app.state.tutor.idtBold = idtClickRepeatOrdersCheckboxFleetWaypointsTile;
            if (!LpflFromId(app, 3)->fRepOrders) {
                return 0;
            }
            app.state.tutor.idh = idhProductionDialog;
            if (LpplFromId(app, 13)->lpplprod->iprodMac != 3 || !FCheckQueue(app, 13, 1, grobjFleet, 2, 1, 0)) {
                app.state.tutor.idtBold = !FCheckSelection(app, grobjPlanet, 13) ? idtSelectStoveTop : idtAddSantaMariaProductionQueue;
                return 0;
            }
            app.state.tutor.idtBold = idtGoAheadGenerateIDare;
            app.state.tutor.fTurnDone = 1;
            return 1;
        }
    case 7:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoNewSantaMaria2:
            if (!FCheckCargo(app, LpflFromId(app, 6), 0, 0, 0, 25)) {
                app.state.tutor.idtBold = idtReadFirstMessageGotoNewSantaMaria2;
                return 0;
            }
            if (!FCheckColonizeWP(app, 6, 18, iWarpAny)) {
                app.state.tutor.idtBold = !FCheckScanner(app, 3, iZoomAny) ? idtClickToolbarButtonPutScannerPlanetValue : idtGiveSantaMaria7ColonizeTaskRed;
                return 0;
            }
            app.state.tutor.idh = idhProductionTile;
            if (LpplFromId(app, 13)->lpplprod->iprodMac < 3 || !FCheckQueue(app, 13, 1, grobjFleet, 2, 3, 0)) {
                app.state.tutor.idtBold = idtAddThreeSantaMariasStoveTopsProduction;
                return 0;
            }
            if (!FCheckScanner(app, 0, iZoomAny)) {
                app.state.tutor.idtBold = idtPressLeftmostToolbarButtonPutScannerBack;
                return 0;
            }
            return 1;
        case idtReadMessage3:
            if (!FCheckMessages(app, imsgAny, idmHaveBuiltMines, 1)) {
                app.state.tutor.idtBold = !FCheckMessages(app, 3, static_cast<MessageId>(idmNone), 0) ? idtReadMessage3 : idtFilterThemClickingBlueCheckMarkMessages;
                return 0;
            }
            if (!FCheckQueue(app, 16, 0, grobjPlanet, iobjFactory, 10, 1) || !FCheckQueue(app, 16, 1, grobjPlanet, iobjMine, 10, 1)) {
                if (!FCheckSelection(app, grobjPlanet, 16)) {
                    app.state.tutor.idtBold = !FCheckMessages(app, 4, static_cast<MessageId>(idmNone), 0) ? idtGoMessage : idtGoto90210OpenProductionQueue;
                } else {
                    app.state.tutor.idtBold = !app.state.lpplProdGlob ? idtGoto90210OpenProductionQueue : idtShiftDoubleClickFactoriesAutoBuildMines;
                }
                return 0;
            }
            app.state.tutor.idtBold = idtReadRestMessages;
            app.state.tutor.fProgress = 0;
            return FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0);
        case idtClickRedTriangleBetweenSlimeVacancy:
            if (!app.state.tutor.fProgress && !app.state.tutor.fAutoComplete) {
                if (FCheckSummary(app, grobjFleet, 512)) {
                    app.state.tutor.fProgress = 1;
                } else {
                    app.state.tutor.idtBold = idtClickRedTriangleBetweenSlimeVacancy;
                    return 0;
                }
            }
            app.state.tutor.idh = idhProductionTile;
            if (LpplFromId(app, 13)->lpplprod->iprodMac < 4 || !FCheckQueue(app, 13, 2, grobjFleet, 0, 2, 0)) {
                app.state.tutor.idtBold = idtAddTwoArmedProbesStoveTopsQueue;
                return 0;
            }
            app.state.tutor.idtBold = idtGenerateWhenReady2;
            app.state.tutor.fTurnDone = 1;
            return 1;
        }
    case 8:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoNewColonyShips:
            if (app.state.rgplr[0].cFleet == 11 && FCheckColonizeWP(app, 10, 8, iWarpAny)) {
                return 1;
            }
            if (app.state.rgplr[0].cFleet == 9 || app.lpui->mdXferDlg == mdXferShips) {
                if (!FCheckSelection(app, grobjFleet, 7)) {
                    app.state.tutor.idtBold = idtReadFirstMessageGotoNewColonyShips;
                } else {
                    app.state.tutor.idtBold = app.lpui->mdXferDlg == mdXferShips ? idtMoveOneSantaMariasFleet10Hit : idtHitSplitButtonFleetCompositionTile;
                }
            } else if (!FCheckCargo(app, LpflFromId(app, 7), 0, 0, 0, 50) || !FCheckFleetWP(app, 7, 1, grobjPlanet, 8, grTaskColonize, iWarpAny)) {
                app.state.tutor.idtBold = idtLoadFleetColonistsGiveColonizeTaskSlime;
            } else {
                app.state.tutor.idtBold = idtWeDontWantBothColonizersGoSlime;
            }
            return 0;
        case idtNoticeFleetHasOneSantaMariaOther:
            app.state.tutor.fNoErrors = 1;
            if (!FCheckColonizeWP(app, 7, 17, iWarpAny)) {
                app.state.tutor.fNoErrors = 0;
                FCheckColonizeWP(app, 7, 8, iWarpAny);
                app.state.tutor.idtBold = idtClickWaypointSlimeDragSeaSquared;
                return 0;
            }
            app.state.tutor.fNoErrors = 0;
            if (!FCheckFleetWP(app, 8, 1, grobjPlanet, 9, grTaskAny, iWarpAny)) {
                app.state.tutor.idtBold = !FCheckSelection(app, grobjFleet, 8) ? idtReadMessageGotoNewArmedScouts : idtLetsTryHeadThemOffPass;
                return 0;
            }
            app.state.tutor.fProgress = 0;
            return 1;
        case idtReadMessageFilter:
            app.state.tutor.fNoErrors = 0;
            if (!FCheckMessages(app, imsgAny, idmHasUnloaded, 1)) {
                app.state.tutor.fProgress = 0;
                app.state.tutor.idtBold = idtReadMessageFilter;
                return 0;
            }
            app.state.tutor.fNoErrors = 1;
            if (FCheckResearch(app, Weapons, TechFieldCount, 30)) {
                app.state.tutor.fNoErrors = 0;
                return 1;
            }
            if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtReadLastMessageGotoWallaby;
                app.state.tutor.fProgress = 0;
                return 0;
            }
            if (!app.state.tutor.fProgress && !app.state.tutor.fAutoComplete) {
                if (FCheckSummary(app, grobjPlanet, 5)) {
                    app.state.tutor.idtBold = idtClickGreenRadiationBarSummaryPaneRead;
                } else {
                    app.state.tutor.idtBold = idtReadLastMessageGotoWallaby;
                }
                return 0;
            }
            app.state.tutor.idtBold = idtHitF5OpenResearchDialog;
            if (app.lpui->pctResGlob != -1) {
                app.state.tutor.fProgress = 0;
                return 1;
            }
            return 0;
        case idtRightRadiationTerraform7OneExpectedBenefits:
            if (app.lpui->pctResGlob != -1) {
                if (!app.state.tutor.fProgress) {
                    app.state.tutor.idtBold = idtClickWordRadiationDialogSeeRequirements;
                } else {
                    app.state.tutor.idtBold = idtIncreaseResourcesBudgetedResearch30HitDone;
                }
                return 0;
            }
            if (!FCheckResearch(app, Weapons, TechFieldCount, 30)) {
                app.state.tutor.idtBold = idtIncreaseResourcesBudgetedResearch30HitDone;
                return 0;
            }
            app.state.tutor.idtBold = idtGenerateWhenReady3;
            app.state.tutor.fTurnDone = 1;
            return 1;
        }
    case 9:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoResearchDialog:
            app.state.tutor.fNoErrors = 1;
            if (!FCheckResearch(app, Weapons, Construction, 30)) {
                app.state.tutor.idtBold = app.lpui->pctResGlob == -1 ? idtReadFirstMessageGotoResearchDialog : idtChangeFieldResearchConstructionHitDone;
                app.state.tutor.fNoErrors = 0;
                return 0;
            }
            app.state.tutor.fNoErrors = 0;
            app.state.tutor.idtBold = idtReadMessageFilter2;
            return FCheckMessages(app, imsgAny, idmHasLoadedMiningRobotsWorking, 1);
        case idtReadMessageGotoOxygen:
            if (!FCheckColonizeWP(app, 9, 2, iWarpAny)) {
                if (FCheckSelection(app, grobjFleet, 9)) {
                    app.state.tutor.idtBold = !FCheckCargo(app, LpflFromId(app, 9), 0, 0, 0, 25) ? idtLoadColonists : idtSendColonizeOxygen;
                } else {
                    app.state.tutor.idtBold = !FCheckSummary(app, grobjPlanet, 2) ? idtReadMessageGotoOxygen : idtRightClickStoveTopSelectSantaMaria2;
                }
                return 0;
            }
            app.state.tutor.fNoErrors = 1;
            if (!FCheckFleetWP(app, 0, 1, grobjPlanet, 4, grTaskAny, iWarpAny)) {
                FCheckFleetWP(app, 0, 1, grobjPlanet, 2, grTaskAny, iWarpAny);
                app.state.tutor.idtBold = idtSelectArmedProbe1DragWaypointOxygen;
                app.state.tutor.fNoErrors = 0;
                return 0;
            }
            app.state.tutor.fNoErrors = 0;
            app.state.tutor.idtBold = idtGenerateWhenReady4;
            app.state.tutor.fTurnDone = 1;
            return 1;
        }
    case 10:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtFilterMessageAboutDismantlingColonizer:
            if (!FCheckMessages(app, imsgAny, idmHasDismantledKtMineralsWhichHaveDeposited, 1)) {
                app.state.tutor.idtBold = idtFilterMessageAboutDismantlingColonizer;
                app.state.tutor.fProgress = 0;
                return 0;
            }
            if (!FCheckQueue(app, 14, 0, grobjPlanet, iobjFactory, 3, 1) || !FCheckQueue(app, 14, 1, grobjPlanet, iobjMine, 3, 1)) {
                if (FCheckSelection(app, grobjPlanet, 14)) {
                    app.state.tutor.idtBold = !app.state.lpplProdGlob ? idtOpenShaggyDogsProductionQueue : idtAdd3FactoriesAutoBuild3Mines;
                } else {
                    app.state.tutor.idtBold = idtReadMessageGotoShaggyDog;
                }
                return 0;
            }
            if (FCheckTemplate(app, 0)) {
                return 1;
            }
            if (!app.state.lpplProdGlob) {
                app.state.tutor.idtBold = idtOpenProductionQueue;
                return 0;
            }
            if (app.lpui->vyZPDStatic != -1) {
                return 1;
            }
            app.state.tutor.idtBold = idtRightClickBlueDiamondSelectCustomize;
            return 0;
        case idtHitImportButtonCopyShaggyDogsQueue:
            if (!FCheckTemplate(app, 0)) {
                app.state.tutor.idtBold = idtHitImportButtonCopyShaggyDogsQueue;
                return 0;
            }
            if (!app.state.tutor.fProgress && app.state.lpplProdGlob) {
                app.state.tutor.idtBold = idtOkProductionDialog;
                return 0;
            }
            if (LpplFromId(app, 13)->lpplprod->iprodMac < 3 || !FCheckQueue(app, 13, 1, grobjFleet, 2, 1, 0)) {
                app.state.tutor.idh = idhProductionDialog;
                if (!app.state.tutor.fProgress && !FCheckSelection(app, grobjPlanet, 13)) {
                    if (FCheckSummary(app, grobjPlanet, 23)) {
                        app.state.tutor.fProgress = 1;
                        app.state.tutor.idtBold = idtAddSantaMariaStoveTopsQueue;
                    } else {
                        app.state.tutor.idtBold = idtReadMessageGotoBloopLooksLikeNice;
                    }
                } else {
                    app.state.tutor.idtBold = idtAddSantaMariaStoveTopsQueue;
                }
                return 0;
            }
            if (LpplFromId(app, 13)->lpplprod->iprodMac < 4 || !FCheckQueue(app, 13, 2, grobjFleet, 3, 1, 0)) {
                app.state.tutor.idh = idhProductionDialog;
                app.state.tutor.idtBold = idtAddNewTeamsterStoveTopsQueue;
                return 0;
            }
            app.state.tutor.idtBold = idtGenerateWhenReady5;
            app.state.tutor.fTurnDone = 1;
            return 1;
        }
    case 11:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoArmedProbe1:
            if (!FCheckFleetWP(app, 0, 1, grobjPlanet, 10, grTaskAny, iWarpAny)) {
                app.state.tutor.idtBold = !FCheckSelection(app, grobjFleet, 0) ? idtReadFirstMessageGotoArmedProbe1 : idtShiftClickHacker;
                return 0;
            }
            if (!FCheckFleetWP(app, 1, 1, grobjPlanet, 13, grTaskScrap, iWarpAny)) {
                app.state.tutor.idtBold = !FCheckSelection(app, grobjFleet, 1) ? idtReadMessageGotoLongRangeScout2 : idtShiftClickStoveTopChangeWaypointTask;
                return 0;
            }
            if (!FCheckFleetWP(app, 4, 1, grobjPlanet, 13, grTaskAny, iWarpAny)) {
                app.state.tutor.idtBold = idtReadMessageSendStalwartDefender5Stove;
                return 0;
            }
            if (!FCheckFleetWP(app, 8, 1, grobjFleet, 512, grTaskAny, iWarpAny)) {
                app.state.tutor.idtBold = idtReadMessageGotoArmedProbe9Shift;
                return 0;
            }
            return 1;
        case idtReadMessageGotoNewSantaMaria:
            if (!FCheckColonizeWP(app, 2, 4, iWarpAny)) {
                app.state.tutor.idtBold = !FCheckSelection(app, grobjFleet, 2) ? idtReadMessageGotoNewSantaMaria : idtGiveSantaMaria3OrdersColonizeDont;
                return 0;
            }
            if (!FCheckCargo(app, LpflFromId(app, 11), 0, 0, 0, 210)) {
                app.state.tutor.idtBold = !FCheckSelection(app, grobjFleet, 11) ? idtReadMessageGotoTeamster12 : idtLoadColonistsAssignWaypointWallaby;
                return 0;
            }
            if (FCheckFleetWP(app, 11, 1, grobjPlanet, 5, grTaskAny, iWarpAny)) {
                if (FCheckFleetWP(app, 11, 1, grobjPlanet, 5, grTaskXfer, iWarpAny)) {
                    if (FCheckXferWP(app, 11, 1, 5, iWarpAny, rgiaUnloadAllCol)) {
                        app.state.tutor.fProgress = 0;
                        return 1;
                    }
                    if (FCheckSelection(app, grobjFleet, 11) && app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].grTask == grTaskXfer &&
                        ControlIO().pfnSendMessage(app.lpui->rghwndOrderDD[1], win32::CB_GETCURSEL, 0, 0) == 4) {
                        app.state.tutor.idtBold = idtThirdUnload;
                    } else {
                        app.state.tutor.idtBold = idtSetSecondDropdownWaypointTaskTileColonists;
                    }
                } else {
                    app.state.tutor.idtBold = idtChangeWaypointTaskTransport2;
                }
            } else {
                app.state.tutor.idtBold = idtLoadColonistsAssignWaypointWallaby;
            }
            return 0;
        case idtReadMessageAdd70MinesTopStove:
            if (LpplFromId(app, 13)->lpplprod->iprodMac < 2 || !FCheckQueue(app, 13, 0, grobjPlanet, mdIdleMine, 70, 0)) {
                app.state.tutor.idtBold = idtReadMessageAdd70MinesTopStove;
            } else if (!FCheckResearch(app, Construction, Biotechnology, 30)) {
                app.state.tutor.idtBold = app.lpui->pctResGlob == -1 ? idtReadMessageGotoResearchDialog : idtLeaveFieldStudyConstructionChangeFieldResearch;
            } else {
                if (FCheckSelection(app, grobjFleet, 8)) {
                    return 1;
                }
                if (!app.state.tutor.fProgress || !FCheckMessages(app, 11, static_cast<MessageId>(idmNone), 0) || app.lpui->hwndBrowser) {
                    if (!app.lpui->hwndBrowser) {
                        app.state.tutor.idtBold = idtReadMessageHitGotoOpenTechnologyBrowser;
                    } else {
                        app.state.tutor.idtBold = idtWhenDoneReadingAboutBetaTorpedoClose;
                        app.state.tutor.fProgress = 1;
                    }
                } else {
                    if (!FCheckMessages(app, 13, static_cast<MessageId>(idmNone), 0)) {
                        app.state.tutor.idtBold = idtReadTwoMessagesLookingTechBrowserIf;
                    } else {
                        app.state.tutor.idtBold = idtReadMessageGotoArmedProbe;
                    }
                }
            }
            return 0;
        case idtGollyNailedOneThemNoticeButtonNormally:
            if (!app.state.tutor.fProgress && !app.state.tutor.fAutoComplete) {
                app.state.tutor.idtBold = !app.state.vrgtok ? idtPressViewOpenBattleVcr : idtUseVcrControlsWatchPlaybackBattleHit;
                return 0;
            }
            app.state.tutor.idtBold = !FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0) ? idtReadRestMessages2 : idtGenerateWhenReady6;
            app.state.tutor.fTurnDone = 1;
            return 1;
        }
    case 12:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoButtonDisabledI:
            if (LpplFromId(app, 8)->lpplprod->iprodMac > 2) {
                return 1;
            }
            if (FCheckSummary(app, grobjFleet, 3)) {
                return 1;
            }
            if (FCheckSummary(app, grobjThing, idAny)) {
                app.state.tutor.idtBold = idtSelectViewFindTypeTeamster4Hit;
            } else if (FCheckMessages(app, 1, static_cast<MessageId>(idmNone), 0) && FCheckSelection(app, grobjFleet, 8)) {
                app.state.tutor.idtBold = idtRightClickArmedProbe9SelectSalvage;
            } else {
                app.state.tutor.idtBold = idtReadMessageGotoArmedProbe9;
            }
            return 0;
        case idtIfCantFindWhereYellowSelectionArrow:
            if (!FCheckMessages(app, 5, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtReadMessage4;
                return 0;
            }
            if (!FCheckMessages(app, 6, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtWatchSadBattleIfWantMoveMessage;
                return 0;
            }
            app.state.tutor.idtBold = idtReadMessageGotoRedStorm;
            return FCheckSelection(app, grobjPlanet, 18);
        case idtOtherBitShortColonistsRedStormDoing:
            if (LpplFromId(app, 13)->lpplprod->iprodMac < 3 || !FCheckQueue(app, 13, 1, grobjFleet, 3, 2, 0)) {
                if (LpplFromId(app, 8)->lpplprod->iprodMac < 3 || !FCheckQueue(app, 8, 0, grobjPlanet, mdIdleTerraform, 2, 1)) {
                    app.state.tutor.idtBold = !FCheckSelection(app, grobjPlanet, 8) ? idtReadMessageGotoSlime : idtOpenSlimesProductionQueueAddTwoTerraform;
                } else {
                    app.state.tutor.idtBold = idtAddTwoTeamstersStoveTopsProductionQueue;
                }
                return 0;
            }
            app.state.tutor.idtBold = idtGenerateWhenReady7;
            app.state.tutor.fTurnDone = 1;
            return 1;
        }
    case 13:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageLoadTeamster1Colonists:
            if (app.state.rgplr[0].cShDef == 7) {
                return 1;
            }
            if (!FCheckXferWP(app, 0, 1, 8, iWarpAny, rgiaUnloadAllCol)) {
                if (!FCheckCargo(app, LpflFromId(app, 0), 0, 0, 0, 210)) {
                    app.state.tutor.idtBold = idtReadFirstMessageLoadTeamster1Colonists;
                } else {
                    app.state.tutor.idtBold = idtSendSlimeOrdersUnloadThem;
                }
                app.state.tutor.fProgress = 0;
                return 0;
            }
            if (!FCheckMessages(app, 3, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtReadMessage5;
                return 0;
            }
            if (!FCheckResearch(app, Biotechnology, Propulsion, 30)) {
                app.state.tutor.idtBold = idtReadMessageOpenResearchDialogChangeField;
                return 0;
            }
            if (!app.state.tutor.fProgress && !app.state.tutor.fAutoComplete) {
                if (!app.lpui->hwndBrowser) {
                    app.state.tutor.idtBold = idtReadMessageCheckRoboMinerTechBrowser;
                } else {
                    app.state.tutor.idtBold = idtHitF4OpenShipDesigner;
                    app.state.tutor.fProgress = 1;
                }
                return 0;
            }
            app.state.tutor.idtBold = idtHitF4OpenShipDesigner;
            if (app.lpui->hwndSlotDlg) {
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
            if (!app.lpui->hwndSlotDlg && app.state.rgplr[0].cShDef == 7) {
                return 1;
            }
            if (!FCheckShipBuilder(app, 4, iShipAny)) {
                if (FCheckShipBuilder(app, 1, 7)) {
                    app.state.tutor.idtBold = idtHitCopySelectedDesign;
                } else {
                    app.state.tutor.idtBold = !FCheckShipBuilder(app, 1, iShipAny) ? idtSelectAvailableHullTypes : idtChooseMiniMinerDropdown;
                }
                return 0;
            }
            app.state.tutor.idh = idhDesigningANewShipFromScratch;
            if (app.lpui->lpshdefBuild->hul.rghs[0].cItem == 0 || !FCheckBuilderPart(app, 0, &hs, 1)) {
                app.state.tutor.idtBold = idtDragLongHump6EnginePartsList;
                return 0;
            }
            if (app.lpui->lpshdefBuild->hul.rghs[1].cItem == 0 || !FCheckBuilderPart(app, 1, &hs1, 1)) {
                app.state.tutor.idtBold = idtDragRhinoScannerScannerElectMechSlot;
                return 0;
            }
            if (app.lpui->lpshdefBuild->hul.rghs[2].cItem == 0 || app.lpui->lpshdefBuild->hul.rghs[3].cItem == 0) {
                if (!FCheckShipBuilder(app, 4, 8)) {
                    app.state.tutor.idtBold = idtSelectMiningRobotsPartsCategoryDropdown;
                    return 0;
                }
                app.state.tutor.idtBold = idtDragRoboMinerEachMiningSlots;
                return 0;
            }
            return 1;
        case idtShipDesignNameImageJustFine:
            if (app.lpui->hwndSlotDlg) {
                app.state.tutor.idtBold = !FCheckShipBuilder(app, 4, iShipAny) ? idtDoneCloseDesigner : idtHitOkFinishEditingDesign;
                return 0;
            }
            if (LpplFromId(app, 13)->lpplprod->iprodMac < 3 || (LpplFromId(app, 13)->lpplprod->iprodMac == 3 && !FCheckQueue(app, 13, 1, grobjFleet, 6, 1, 0))) {
                app.state.tutor.idtBold = idtAddOneNewMiniMinersStoveTops;
                return 0;
            }
            if (LpplFromId(app, 13)->lpplprod->iprodMac < 4 || !FCheckQueue(app, 13, 0, grobjPlanet, mdIdleMine, 100, 0)) {
                app.state.tutor.idtBold = !app.state.lpplProdGlob ? idtOpenStoveTopsQueue : idtSelectMineLeftHandListboxTopQueue;
                app.state.tutor.fProgress = 0;
                return 0;
            }
            return 1;
        case idtClickEachItemsProductionTileMiniMiner:
            if (!app.state.tutor.fProgress && !app.state.tutor.fAutoComplete) {
                app.state.tutor.idtBold = idtClickEachItemsProductionTileMiniMiner;
                return 0;
            }
            if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtReadFinalMessage;
                return 0;
            }
            if (!FCheckFleetWP(app, 8, 1, grobjPlanet, 13, grTaskAny, iWarpAny)) {
                app.state.tutor.idtBold = idtSendArmedProbe9BackStoveTop;
                return 0;
            }
            app.state.tutor.idtBold = idtGenerateNewYear;
            app.state.tutor.fTurnDone = 1;
            return 1;
        }
    case 14:
        if (app.state.tutor.idt != idtReadFirstMessageGotoSeaSquared) {
            return 1;
        }
        if (LpplFromId(app, 2)->lpplprod->iprodMac < 3 || !FCheckQueue(app, 2, 0, grobjPlanet, iobjMinTerraform, 2, 1) || !FCheckTemplate(app, 1)) {
            if (FCheckSelection(app, grobjPlanet, 2)) {
                app.state.tutor.idtBold = idtAddMinTerraform2OxygensQueueRight;
            } else if (FCheckSelection(app, grobjPlanet, 17)) {
                app.state.tutor.idtBold = idtReadFinalMessageGotoOxygen;
            } else {
                app.state.tutor.idtBold = idtReadFirstMessageGotoSeaSquared;
            }
            return 0;
        }
        app.state.tutor.idtBold = idtGenerateWhenReady8;
        app.state.tutor.fTurnDone = 1;
        return 1;
    case 15:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageSendArmedProbe9:
            if (!FCheckFleetWP(app, 8, 1, grobjPlanet, 11, grTaskAny, iWarpAny)) {
                app.state.tutor.idtBold = idtReadFirstMessageSendArmedProbe9;
                return 0;
            }
            if (!FCheckCargo(app, LpflFromId(app, 6), 0, 0, 0, 210)) {
                app.state.tutor.idtBold = idtReadMessageGotoNewTeamsterFillColonists;
                app.state.tutor.fProgress = 0;
                return 0;
            }
            if (!app.state.tutor.fAutoComplete && !app.state.tutor.fProgress) {
                if (!app.lpui->vprptCur) {
                    app.state.tutor.idtBold = idtChoosePlanetsReportMenu;
                } else {
                    app.state.tutor.fProgress = 1;
                    if (app.lpui->vprptCur->icolSort == 4) {
                        app.state.tutor.idtBold = idtHitEscKeyClosePlanetSummaryReport;
                    } else {
                        app.state.tutor.idtBold = idtClickTitleValueColumnSortValue;
                    }
                }
                return 0;
            }
            if (!FCheckXferWP(app, 6, 1, 5, iWarpAny, rgiaUnloadAllCol)) {
                app.state.tutor.idtBold = idtSendTeamster7WallabyUnloadColonists;
                return 0;
            }
            return 1;
        case idtReadTwoMessagesOpenResearchDialog:
            if (!app.state.tutor.fAutoComplete && !app.state.tutor.fProgress) {
                if (app.lpui->pctResGlob != -1) {
                    app.state.tutor.fProgress = 1;
                    app.state.tutor.idtBold = idtCloseDialog;
                } else {
                    app.state.tutor.idtBold = idtReadTwoMessagesOpenResearchDialog;
                }
                return 0;
            }
            if (!FCheckFleetWP(app, 11, 1, grobjPlanet, 13, grTaskAny, iWarpAny)) {
                app.state.tutor.idtBold = idtReadRemainingMessagesSendTeamster12Back;
                return 0;
            }
            app.state.tutor.idtBold = idtGenerateNewYear2;
            app.state.tutor.fTurnDone = 1;
            return 1;
        }
    case 16:
        if (app.state.tutor.idt != idtReadFirstMessageSendStalwartDefender5) {
            return 1;
        }
        if (!FCheckFleetWP(app, 4, 1, grobjPlanet, 5, grTaskAny, iWarpAny)) {
            app.state.tutor.idtBold = idtReadFirstMessageSendStalwartDefender5;
            return 0;
        }
        app.state.tutor.fNoErrors = 1;
        if (LpflFromId(app, 7)->cord == 1) {
            app.state.tutor.idtBold = !FCheckSelection(app, grobjFleet, 7) ? idtReadMessageGotoNewMiniMiner : idtShiftClickPruneSetWaypointTaskMerge;
            return 0;
        }
        if (FCheckFleetWP(app, 7, 1, grobjPlanet, 12, grTaskAny, iWarpAny)) {
            app.state.tutor.fNoErrors = 0;
            app.state.tutor.idtBold = idtShiftClickPruneSetWaypointTaskMerge;
            return 0;
        }
        app.state.tutor.fNoErrors = 0;
        if (!FCheckFleetWP(app, 7, 1, grobjFleet, 5, grTaskMerge, iWarpAny)) {
            app.state.tutor.idtBold = idtShiftClickPruneSetWaypointTaskMerge;
            return 0;
        }
        if (LpplFromId(app, 13)->lpplprod->iprodMac < 2 || !FCheckQueue(app, 13, 0, grobjPlanet, iobjFactory, 60, 0) ||
            !FCheckQueue(app, 13, 1, grobjPlanet, iobjMine, 60, 0)) {
            app.state.tutor.idtBold = !app.state.lpplProdGlob ? idtReadMessageOpenStoveTopsProductionQueue : idtIncreaseNumberAutoBuildFactories60Add;
            return 0;
        }
        if (!FCheckResearch(app, Propulsion, Construction, 30)) {
            app.state.tutor.idtBold = idtReadTwoMessagesChangeFieldResearchConstruction;
            return 0;
        }
        app.state.tutor.idtBold = idtReadFinalMessageGenerate;
        app.state.tutor.fTurnDone = 1;
        return 1;
    case 17:
        if (app.state.tutor.idt != idtReadMessages2) {
            return 1;
        }
        if (!FCheckFleetWP(app, 0, 1, grobjPlanet, 13, grTaskAny, iWarpAny)) {
            app.state.tutor.idtBold = !FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0) ? idtReadMessages2 : idtSendTeamster1BackStoveTop;
            return 0;
        }
        app.state.tutor.idtBold = idtGenerateNewYear3;
        app.state.tutor.fTurnDone = 1;
        return 1;
    case 18:
        if (app.state.tutor.idt != idtReadFirstMessageAddTeamsterStoveTops) {
            return 1;
        }
        if (LpplFromId(app, 13)->lpplprod->iprodMac < 3 || !FCheckQueue(app, 13, 0, grobjFleet, 3, 1, 0)) {
            app.state.tutor.idtBold = idtReadFirstMessageAddTeamsterStoveTops;
            return 0;
        }
        if (!FCheckMessages(app, 5, static_cast<MessageId>(idmNone), 0)) {
            app.state.tutor.fProgress = 0;
            app.state.tutor.idtBold = idtReadMessageGoto90210;
            return 0;
        }
        if (!app.state.tutor.fAutoComplete && !app.state.tutor.fProgress) {
            if (FCheckSelection(app, grobjPlanet, 16)) {
                if (app.lpui->pctResGlob != -1) {
                    app.state.tutor.idtBold = idtClickDifferentItemsListedExpectedBenefitsBox;
                } else {
                    app.state.tutor.idtBold = idtReadTwoMessagesOpenResearchDialog2;
                }
            } else {
                app.state.tutor.idtBold = idtReadMessageGoto90210;
            }
            return 0;
        }
        if (app.lpui->pctResGlob != -1) {
            app.state.tutor.idtBold = idtCloseDialogWithoutMakingAnyChanges;
            return 0;
        }
        app.state.tutor.idtBold = idtReadRemainingMessagesGenerateYear;
        app.state.tutor.fTurnDone = 1;
        return 1;
    case 19:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageLoadTeamster12Colonists:
            if (!FCheckCargo(app, LpflFromId(app, 11), 0, 0, 0, 210)) {
                app.state.tutor.idtBold = idtReadFirstMessageLoadTeamster12Colonists;
                return 0;
            }
            if (!FCheckXferWP(app, 11, 1, 5, iWarpAny, rgiaUnloadAllCol)) {
                app.state.tutor.idtBold = idtAddWaypointWallabyUnloadColonists;
                return 0;
            }
            app.state.tutor.fNoErrors = 1;
            if (!FCheckXferWP(app, 11, 2, 13, iWarpAny, rgiaLoadAllCol)) {
                app.state.tutor.fNoErrors = 0;
                app.state.tutor.idtBold = idtShiftClickBackStoveTopChangeTask;
                return 0;
            }
            app.state.tutor.fNoErrors = 0;
            app.state.tutor.idh = idhFleetWaypointsTile;
            app.state.tutor.idtBold = idtClickRepeatOrdersCheckboxFleetWaypointsTile2;
            if (!LpflFromId(app, 11)->fRepOrders) {
                return 0;
            }
            if (!FCheckCargo(app, LpflFromId(app, 1), 0, 0, 0, 210)) {
                app.state.tutor.idtBold = !FCheckSelection(app, grobjFleet, 1) ? idtReadMessageGotoNewTeamster : idtLoadColonists2;
                return 0;
            }
            return 1;
        case idtAddWaypointOxygenUnloadColonists:
            if (!FCheckXferWP(app, 1, 1, 2, iWarpAny, rgiaUnloadAllCol)) {
                app.state.tutor.idtBold = idtAddWaypointOxygenUnloadColonists;
                return 0;
            }
            app.state.tutor.fNoErrors = 1;
            if (!FCheckXferWP(app, 1, 2, 13, iWarpAny, rgiaLoadAllCol)) {
                app.state.tutor.fNoErrors = 0;
                app.state.tutor.idtBold = idtShiftClickBackStoveTopChangeTask2;
                return 0;
            }
            app.state.tutor.fNoErrors = 0;
            app.state.tutor.idh = idhFleetWaypointsTile;
            app.state.tutor.idtBold = idtClickRepeatOrdersCheckboxFleetWaypointsTile3;
            if (!LpflFromId(app, 1)->fRepOrders) {
                return 0;
            }
            if (LpplFromId(app, 16)->lpplprod->iprodMac < 3 || !FCheckQueue(app, 16, 2, grobjPlanet, iobjMaxTerraform, 1, 1)) {
                app.state.tutor.idtBold = idtReadMessageAddMaxTerraformAutoBuild;
                return 0;
            }
            if (!FCheckFleetWP(app, 6, 1, grobjPlanet, 13, grTaskAny, iWarpAny)) {
                app.state.tutor.idtBold = idtRead3MessagesSendTeamster7Back;
                return 0;
            }
            app.state.tutor.idtBold = idtReadLastMessageGenerateYear;
            app.state.tutor.fTurnDone = 1;
            return 1;
        }
    case 20:
        if (app.state.tutor.idt != idtReadFirstTwoMessagesSendArmedProbe) {
            return 1;
        }
        if (!FCheckFleetWP(app, 8, 1, grobjPlanet, 6, grTaskAny, iWarpAny)) {
            app.state.tutor.idtBold = idtReadFirstTwoMessagesSendArmedProbe;
            return 0;
        }
        if (!FCheckResearch(app, Construction, Weapons, 30)) {
            app.state.tutor.idtBold = idtRead3MessagesChangeFieldResearchWeapons;
            return 0;
        }
        if (app.state.rgplr[0].cshdefSB == 1 || app.lpui->hwndSlotDlg) {
            if (!app.lpui->hwndSlotDlg && app.state.rgplr[0].cshdefSB == 1) {
                app.state.tutor.idtBold = idtReadFinalMessageHitF4OpenShip;
            } else if ((!FCheckShipBuilder(app, 4, iShipAny) || !app.state.fStarbaseMode) && app.state.rgplr[0].cshdefSB == 1) {
                app.state.tutor.idtBold = idtSelectStarbasesCopySelectedDesign;
            } else if (app.lpui->lpshdefBuild->hul.rghs[0].cItem == 0) {
                app.state.tutor.idtBold = idtSelectOrbitalPartsCategoryDragStargate100;
            } else {
                app.state.tutor.idtBold = idtChangeDesignNameGaterClickRightArrow;
            }
            return 0;
        }
        if (LpplFromId(app, 13)->lpplprod->iprodMac < 4 || !FCheckQueue(app, 13, 1, grobjFleet, 17, 1, 0)) {
            app.state.tutor.idtBold = idtAddGaterStoveTopsQueue;
            return 0;
        }
        app.state.tutor.idtBold = idtGenerate;
        app.state.tutor.fTurnDone = 1;
        return 1;
    case 21:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageLoadTeamster1Colonists2:
            if (!FCheckCargo(app, LpflFromId(app, 0), 0, 0, 0, 210)) {
                app.state.tutor.idtBold = idtReadFirstMessageLoadTeamster1Colonists2;
                return 0;
            }
            if (!FCheckXferWP(app, 0, 1, 5, iWarpAny, rgiaUnloadAllCol)) {
                app.state.tutor.idtBold = idtSendUnloadColonistsWallaby;
                return 0;
            }
            if (!FCheckZip(app, 0, rgiaUnloadAllCol, idsDropcol)) {
                app.state.tutor.idtBold = !app.lpui->hwndZipOrderDlg ? idtRightClickBlueDiamondWaypointTaskTile2 : idtHitImportNameOrderDropcolOkBoth;
                return 0;
            }
            app.state.tutor.fNoErrors = 1;
            if (!FCheckXferWP(app, 0, 2, 13, iWarpAny, rgiaLoadAllCol)) {
                app.state.tutor.fNoErrors = 0;
                app.state.tutor.idtBold = idtShiftClickStoveTopChangeTransportOption;
                return 0;
            }
            app.state.tutor.fNoErrors = 0;
            app.state.tutor.idh = idhFleetWaypointsTile;
            app.state.tutor.idtBold = idtClickRepeatOrders;
            if (!LpflFromId(app, 0)->fRepOrders) {
                return 0;
            }
            return 1;
        case idtReadRestMessages3:
            if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtReadRestMessages3;
                return 0;
            }
            if (!app.state.tutor.fAutoComplete && !app.state.tutor.fProgress) {
                if (!app.lpui->vprptCur) {
                    app.state.tutor.idtBold = idtHitF3OpenPlanetSummaryReport;
                } else if (app.lpui->vprptCur->icolSort == 11 && !app.lpui->vprptCur->fAscending && app.lpui->vprptCur->iSubsort == 3) {
                    app.state.tutor.idtBold = idtGenerateWhenReady9;
                    app.state.tutor.fProgress = 1;
                } else {
                    app.state.tutor.idtBold = idtFindMinConcColumnRightClickReverse;
                }
                return 0;
            }
            app.state.tutor.idtBold = idtGenerateWhenReady9;
            app.state.tutor.fTurnDone = 1;
            return 1;
        }
    case 22:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoTeamster42:
            if (LpflFromId(app, 3)->rgwtMin[4] != 383) {
                if (!FCheckSelection(app, grobjFleet, 3)) {
                    app.state.tutor.idtBold = idtReadFirstMessageGotoTeamster42;
                } else {
                    app.state.tutor.idtBold = idtClickDragFuelGaugeOtherFleetsHere;
                    app.state.tutor.idh = idhOtherFleetsHereTile;
                }
                return 0;
            }
            return 1;
        case idtAddTeamsterStoveTopsQueue:
            if (LpplFromId(app, 13)->lpplprod->iprodMac < 4 || !FCheckQueue(app, 13, 1, grobjFleet, 3, 1, 0)) {
                app.state.tutor.idtBold = idtAddTeamsterStoveTopsQueue;
                return 0;
            }
            if (!FCheckResearch(app, Weapons, Propulsion, 30)) {
                app.state.tutor.idtBold = idtRead4MessagesChangeFieldResearchPropulsion;
                return 0;
            }
            if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtReadRemainingMessagesOpenShipDesigner;
                return 0;
            }
            if (app.state.rgplr[0].cShDef < 8 || app.lpui->hwndSlotDlg) {
                hs.grhst = hstEngine;
                hs.iItem = iengineDaddyLongLegs7;
                hs.cItem = 1;
                hs1.grhst = hstMines;
                hs1.iItem = iminesMineDispenser50;
                hs1.cItem = 3;
                if (!app.lpui->hwndSlotDlg) {
                    app.state.tutor.idtBold = idtReadRemainingMessagesOpenShipDesigner;
                    app.state.tutor.idh = static_cast<HelpContextId>(1001);
                    return 0;
                }
                if (app.state.rgplr[0].cShDef < 8 && !FCheckShipBuilder(app, 4, iShipAny)) {
                    app.state.tutor.idtBold = idtViewAvailableHullTypesSelectFrigateDropdown;
                    return 0;
                }
                app.state.tutor.idh = idhDesigningANewShipFromScratch;
                if (app.lpui->lpshdefBuild->hul.rghs[0].cItem == 0 || !FCheckBuilderPart(app, 0, &hs, 1)) {
                    app.state.tutor.idtBold = idtDragDaddyLongLegs7EngineSlot;
                } else if (app.lpui->lpshdefBuild->hul.rghs[2].cItem == 0 || !FCheckBuilderPart(app, 2, &hs1, 1)) {
                    app.state.tutor.idtBold = idtSelectMineLayersDropdownDrag3Mine;
                } else {
                    app.state.tutor.idtBold = idtChangeDesignNameMineLayerOkDesign;
                }
                return 0;
            }
            if (LpplFromId(app, 13)->lpplprod->iprodMac < 5 || !FCheckQueue(app, 13, 2, grobjFleet, 7, 1, 0)) {
                app.state.tutor.idtBold = idtAddMineLayerStoveTopsQueue;
                return 0;
            }
            app.state.tutor.idtBold = idtAddMineLayerStoveTopsQueue;
            return 1;
        case idtClickRedTriangleWallaby:
            if (!FCheckFleetWP(app, 4, 1, grobjFleet, 516, grTaskAny, iWarpAny)) {
                if (FCheckSelection(app, grobjFleet, 4)) {
                    app.state.tutor.idtBold = idtShiftClickEnemyFleet;
                } else if (FCheckSummary(app, grobjFleet, 516)) {
                    app.state.tutor.idtBold = idtRightClickWallabySelectStalwartDefender5;
                } else {
                    app.state.tutor.idtBold = idtClickRedTriangleWallaby;
                }
                return 0;
            }
            app.state.tutor.idtBold = idtGenerateWhenReady10;
            app.state.tutor.fTurnDone = 1;
            app.state.tutor.fProgress = 0;
            return 1;
        }
    case 23:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoStalwartDefender5:
            if (FCheckCargo(app, LpflFromId(app, 6), 0, 0, 0, 210)) {
                return 1;
            }
            if (!FCheckMessages(app, 1, static_cast<MessageId>(idmNone), 0) && !FCheckSelection(app, grobjFleet, 4)) {
                app.state.tutor.idtBold = idtReadFirstMessageGotoStalwartDefender5;
                app.state.tutor.fProgress = 0;
                return 0;
            }
            if (!FCheckMessages(app, 2, static_cast<MessageId>(idmNone), 0) && !FCheckSelection(app, grobjFleet, 6)) {
                app.state.tutor.idtBold = idtReadMessageGotoTeamster7;
                return 0;
            }
            if (!app.state.tutor.fAutoComplete && !app.state.tutor.fProgress) {
                if (!app.lpui->vprptCur) {
                    app.state.tutor.idtBold = idtOpenPlanetSummaryReportSortPopulation;
                } else if (app.lpui->vprptCur->icolSort == 2) {
                    app.state.tutor.idtBold = idtHitEscCloseReport;
                    app.state.tutor.fProgress = 1;
                } else {
                    app.state.tutor.idtBold = idtOpenPlanetSummaryReportSortPopulation;
                }
                return 0;
            }
            return 1;
        case idtLoadTeamster7ColonistsSendSeaSquared:
            if (!FCheckCargo(app, LpflFromId(app, 6), 0, 0, 0, 210)) {
                app.state.tutor.idtBold = idtLoadTeamster7ColonistsSendSeaSquared;
                return 0;
            }
            if (!FCheckFleetWP(app, 6, 1, grobjPlanet, 17, grTaskAny, iWarpAny)) {
                app.state.tutor.idtBold = idtLoadTeamster7ColonistsSendSeaSquared;
                return 0;
            }
            if (!FCheckFleetWP(app, 6, 1, grobjPlanet, 17, grTaskXfer, iWarpAny)) {
                app.state.tutor.idtBold = idtSetWaypointTaskTransport2;
                return 0;
            }
            if (!FCheckXferWP(app, 6, 1, 17, iWarpAny, rgiaUnloadAllCol)) {
                app.state.tutor.idtBold = idtRightClickBlueDiamondChooseDropcol;
                app.state.tutor.idh = idhTransport;
                return 0;
            }
            if (!LpflFromId(app, 2)) {
                app.state.tutor.fProgress = 0;
                return 1;
            }
            if (!FCheckMessages(app, 3, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtReadTwoMessagesGotoNewTeamster;
            } else if (!FCheckSelection(app, grobjFleet, 3)) {
                app.state.tutor.idtBold = !FCheckSelection(app, grobjFleet, 2) ? idtReadTwoMessagesGotoNewTeamster : idtSelectTeamster4ListboxOtherFleetsHere;
                app.state.tutor.idh = idhOtherFleetsHereTile;
            } else if (!app.state.vrgiflMerge) {
                app.state.tutor.idtBold = idtPressMergeButtonFleetCompositionTile;
                app.state.tutor.idh = idhFleetCompositionTile;
            } else {
                app.state.tutor.idh = idhMergeFleetsDialog;
                app.state.tutor.idtBold = idtClickTeamster3MergeFleetsDialogHit;
            }
            return 0;
        case idtReadMessageSetMineLayer8sTask:
            if (!FCheckFleetWP(app, 7, 0, grobjPlanet, 13, grTaskLayMines, iWarpAny)) {
                app.state.tutor.idtBold = idtReadMessageSetMineLayer8sTask;
                return 0;
            }
            if (LpplFromId(app, 13)->lpplprod->iprodMac < 3 || !FCheckQueue(app, 13, 0, grobjFleet, 6, 1, 0)) {
                app.state.tutor.idtBold = idtReadMessageAddMiniMinerStoveTops;
                app.state.tutor.fProgress = 0;
                return 0;
            }
            if (!FCheckMessages(app, 14, static_cast<MessageId>(idmNone), 0) || (!FCheckSelection(app, grobjFleet, 4) && !FCheckSelection(app, grobjFleet, 8)) ||
                (!app.state.tutor.fAutoComplete && !app.state.tutor.fProgress)) {
                if (app.state.vrgtok) {
                    app.state.tutor.fProgress = 1;
                }
                app.state.tutor.idtBold = idtRead2MessagesWatchBattle;
                return 0;
            }
            if (LpflFromId(app, 4)->lpplord->rgord()[0].grobj != grobjFleet) {
                app.state.tutor.idtBold = idtRightClickBlueDiamondFleetWaypointsTile;
                app.state.tutor.idh = idhFleetWaypointsTile;
                return 0;
            }
            if (LpflFromId(app, 8)->cord < 3) {
                if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0) || !FCheckSelection(app, grobjFleet, 8)) {
                    app.state.tutor.idtBold = idtReadLastMessageDoubleClickArmedProbe;
                } else {
                    app.state.tutor.fNoErrors = 1;
                    if (FCheckFleetWP(app, 8, 1, grobjPlanet, 1, grTaskAny, iWarpAny)) {
                        app.state.tutor.idtBold = idtShiftClickLeverGenerate;
                    } else {
                        app.state.tutor.fNoErrors = 0;
                        FCheckFleetWP(app, 8, 1, grobjPlanet, 6, grTaskAny, iWarpAny);
                        app.state.tutor.idtBold = idtDragWaypointLaTeDaSpeedBump;
                    }
                    app.state.tutor.fNoErrors = 0;
                }
                return 0;
            }
            if (!FCheckFleetWP(app, 8, 2, grobjPlanet, 0, grTaskAny, iWarpAny)) {
                app.state.tutor.idtBold = idtShiftClickLeverGenerate;
                return 0;
            }
            app.state.tutor.idtBold = idtShiftClickLeverGenerate;
            app.state.tutor.fTurnDone = 1;
            return 1;
        }
    case 24:
        if (app.state.tutor.idt != idtReadFirstMessageSendStalwartDefender52) {
            return 1;
        }
        if (!FCheckFleetWP(app, 4, 1, grobjPlanet, 5, grTaskAny, iWarpAny)) {
            app.state.tutor.idtBold = idtReadFirstMessageSendStalwartDefender52;
            return 0;
        }
        app.state.tutor.fNoErrors = 1;
        if (!FCheckFleetWP(app, 2, 1, grobjFleet, 5, grTaskMerge, iWarpAny)) {
            app.state.tutor.fNoErrors = 0;
            FCheckFleetWP(app, 2, 1, grobjPlanet, 12, grTaskAny, iWarpAny);
            app.state.tutor.idtBold = idtReadMessageGotoMiniMiner3Send;
            return 0;
        }
        app.state.tutor.fNoErrors = 0;
        if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
            app.state.tutor.idtBold = idtReadRemainingMessages;
            return 0;
        }
        app.state.tutor.idtBold = idtGenerateWill;
        app.state.tutor.fTurnDone = 1;
        return 1;
    case 25:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstTwoMessages:
            if (!FCheckMessages(app, 3, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtReadFirstTwoMessages;
                return 0;
            }
            if (app.state.rgshdef[2].hul.rghs[0].iItem != iengineDaddyLongLegs7 || app.lpui->hwndSlotDlg) {
                hs.grhst = hstEngine;
                hs.iItem = iengineDaddyLongLegs7;
                hs.cItem = 1;
                hs2.grhst = hstSpecialM;
                hs2.iItem = ispecialMColonizationModule;
                hs2.cItem = 1;
                if (!FCheckScanner(app, 3, iZoomAny)) {
                    app.state.tutor.idtBold = idtClickButtonToolbar;
                    return 0;
                }
                if (!app.lpui->hwndSlotDlg) {
                    app.state.tutor.idtBold = idtHitF4OpenShipDesigner2;
                    app.state.tutor.idh = static_cast<HelpContextId>(1001);
                    return 0;
                }
                if (app.state.rgshdef[2].hul.rghs[0].iItem == iengineDaddyLongLegs7) {
                    app.state.tutor.idtBold = idtOkDesignHitDoneCloseDialog;
                    return 0;
                }
                if (!FCheckShipBuilder(app, 4, iShipAny)) {
                    if (!FCheckShipBuilder(app, 0, 2)) {
                        app.state.tutor.idtBold = idtSelectSantaMariaDropdown;
                    } else {
                        app.state.tutor.idtBold = idtHitEditSelectedDesign;
                    }
                } else if (app.lpui->lpshdefBuild->hul.rghs[0].cItem == 0 || app.lpui->lpshdefBuild->hul.rghs[0].iItem != iengineDaddyLongLegs7 || !FCheckBuilderPart(app, 1, &hs2, 1)) {
                    app.state.tutor.idtBold = idtDragLongHump6EngineDesignParts;
                } else {
                    app.state.tutor.idtBold = idtOkDesignHitDoneCloseDialog;
                }
                return 0;
            }
            return 1;
        case idtAdd3ImprovedSantaMariasStoveTops:
            if (LpplFromId(app, 13)->lpplprod->iprodMac < 3 || !FCheckQueue(app, 13, 0, grobjFleet, 2, 3, 0)) {
                app.state.tutor.idtBold = idtAdd3ImprovedSantaMariasStoveTops;
                return 0;
            }
            if (!FCheckMessages(app, 4, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtReadMessageGotoSeaSquared;
                return 0;
            }
            if (LpplFromId(app, 17)->lpplprod->iprodMac != 3 || !FCheckQueue(app, 17, 2, grobjPlanet, iobjMaxTerraform, 2, 1)) {
                if (!FCheckSelection(app, grobjPlanet, 17)) {
                    app.state.tutor.idtBold = idtReadMessageGotoSeaSquared;
                } else {
                    app.state.tutor.idtBold = idtAddMaxTerraformAutoBuild2End;
                }
                return 0;
            }
            if (!FCheckMessages(app, 6, static_cast<MessageId>(idmNone), 0) || !FCheckResearch(app, Propulsion, Construction, 30)) {
                app.state.tutor.idtBold = idtRead2MessagesChangeFieldResearchConstruction;
                return 0;
            }
            if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtReadRestMessages4;
                return 0;
            }
            app.state.tutor.idtBold = idtGenerateWhenReady11;
            app.state.tutor.fTurnDone = 1;
            app.state.tutor.fProgress = 0;
            return 1;
        }
    case 26:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstThreeMessages:
            if (!FCheckMessages(app, 4, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtReadFirstThreeMessages;
                return 0;
            }
            if (app.state.rgplr[0].cFleet == 10 && !FCheckColonizeWP(app, 9, 0, iWarpAny)) {
                app.state.tutor.idtBold = !FCheckSelection(app, grobjFleet, 9) ? idtGoto3NewSantaMarias : idtLoadThemColonistsSendThemColonizeLever;
                return 0;
            }
            if (app.state.rgplr[0].cFleet == 10) {
                app.state.tutor.idtBold = idtHitSplitButtonFleetCompositionTile2;
                return 0;
            }
            app.state.tutor.fNoErrors = 1;
            if (!FCheckColonizeWP(app, 9, 1, iWarpAny)) {
                app.state.tutor.fNoErrors = 0;
                FCheckColonizeWP(app, 9, 0, iWarpAny);
                app.state.tutor.idtBold = idtDragSantaMaria10sWaypointSpeedBump;
                return 0;
            }
            if (!FCheckColonizeWP(app, 10, 23, iWarpAny)) {
                app.state.tutor.fNoErrors = 0;
                FCheckColonizeWP(app, 10, 0, iWarpAny);
                app.state.tutor.idtBold = idtSantaMaria11sBloop;
                return 0;
            }
            app.state.tutor.fNoErrors = 0;
            if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtReadRestMessages5;
                return 0;
            }
            return 1;
        case idtTeamster12WillArriveStoveTopYear:
            if (LpplFromId(app, 13)->lpplprod->iprodMac < 4 || !FCheckQueue(app, 13, 1, grobjFleet, 3, 3, 0)) {
                app.state.tutor.idtBold = idtAdd3TeamstersStoveTopsQueue;
                return 0;
            }
            app.state.tutor.fNoErrors = 1;
            if (!FCheckFleetWP(app, 4, 1, grobjFleet, 517, grTaskAny, iWarpAny)) {
                app.state.tutor.fNoErrors = 0;
                FCheckColonizeWP(app, 4, 5, iWarpAny);
                if (FCheckSummary(app, grobjFleet, 517) || FCheckSummary(app, grobjFleet, 4) || FCheckSelection(app, grobjFleet, 4)) {
                    app.state.tutor.idtBold = idtSelectStalwartDefender5DragDestinationEnemy;
                } else {
                    app.state.tutor.idtBold = idtClickEnemyShipNearWallaby;
                }
                return 0;
            }
            app.state.tutor.fNoErrors = 0;
            app.state.tutor.idtBold = idtGenerateWhenYoureReady;
            app.state.tutor.fTurnDone = 1;
            return 1;
        }
    case 27:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoStalwartDefender52:
            app.state.tutor.fNoErrors = 1;
            if (!FCheckFleetWP(app, 4, 0, grobjFleet, 517, grTaskAny, iWarpAny)) {
                app.state.tutor.fNoErrors = 0;
                if (FCheckSelection(app, grobjFleet, 4)) {
                    app.state.tutor.idh = idhFleetWaypointsTile;
                    app.state.tutor.idtBold = idtRightClickBlueDiamondFleetWaypointsTile2;
                } else {
                    app.state.tutor.idtBold = idtReadFirstMessageGotoStalwartDefender52;
                }
                return 0;
            }
            app.state.tutor.fNoErrors = 0;
            if (!FCheckMessages(app, 1, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtReadMessageGotoArmedProbe92;
                return 0;
            }
            if (!LpflFromId(app, 12) || LpflFromId(app, 12)->fDead) {
                return 1;
            }
            if (!FCheckCargo(app, LpflFromId(app, 12), 0, 0, 0, 630)) {
                if (FCheckSelection(app, grobjFleet, 12)) {
                    app.state.tutor.idtBold = idtFillColonists;
                } else {
                    app.state.tutor.idtBold = !FCheckSelection(app, grobjFleet, 8) ? idtReadMessageGotoArmedProbe92 : idtReadMessageGotoNewFleet;
                }
                return 0;
            }
            return 1;
        case idtWeWantMergeNewTeamstersOtherFleet:
            if (LpflFromId(app, 12)) {
                if (!FCheckSelection(app, grobjFleet, 11)) {
                    app.state.tutor.idtBold = idtSelectTeamster12PressMergeButtonFleet;
                } else if (!app.state.vrgiflMerge) {
                    app.state.tutor.idtBold = idtSelectTeamster12PressMergeButtonFleet;
                    app.state.tutor.idh = idhFleetCompositionTile;
                } else {
                    app.state.tutor.idtBold = idtSelectTeamster12PressMergeButtonFleet;
                }
                return 0;
            }
            if (!FCheckMessages(app, 8, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtReadMessage6;
                return 0;
            }
            if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtReadRestMessagesViewingBattleColonizerIf;
                return 0;
            }
            app.state.tutor.idtBold = idtHitF4OpenShipDesigner3;
            app.state.tutor.idh = idhKeyboardShortcuts;
            if (app.lpui->hwndSlotDlg || app.state.rgplr[0].cShDef == 9) {
                return 1;
            }
            return 0;
        case idtWeWantPowerfulWeAlsoWantWeigh:
            if (app.lpui->hwndSlotDlg || app.state.rgplr[0].cShDef < 9) {
                if (!FCheckShipBuilder(app, 4, iShipAny)) {
                    app.state.tutor.idtBold = app.state.rgplr[0].cShDef >= 9 ? idtClickRightArrowButtonBelowShipImage : idtSelectAvailableHullTypesChooseDestroyerDropdown;
                } else if (app.lpui->lpshdefBuild->hul.rghs[0].cItem != 1 || app.lpui->lpshdefBuild->hul.rghs[1].cItem != 1 || app.lpui->lpshdefBuild->hul.rghs[2].cItem != 1 ||
                           app.lpui->lpshdefBuild->hul.rghs[3].cItem != 1 || app.lpui->lpshdefBuild->hul.rghs[4].cItem != 2) {
                    app.state.tutor.idtBold = idtAddRadiatingHydroRamScoop2Carbonic;
                } else if (app.lpui->lpshdefBuild->hul.rghs[5].cItem != 1 || app.lpui->lpshdefBuild->hul.rghs[6].cItem != 1) {
                    app.state.tutor.idtBold = idtAddFuelTankMechanicalSlotBattleComputer;
                } else {
                    app.state.tutor.idtBold = idtClickRightArrowButtonBelowShipImage;
                }
                return 0;
            }
            if (LpplFromId(app, 13)->lpplprod->iprodMac < 3 || !FCheckQueue(app, 13, 0, grobjFleet, 8, 10, 0)) {
                app.state.tutor.idtBold = idtPut10DestroyersStoveTopsQueue;
                return 0;
            }
            app.state.tutor.idtBold = idtGenerate2;
            app.state.tutor.fTurnDone = 1;
            return 1;
        }
    case 28:
        if (app.state.tutor.idt != idtReadFirstMessageGotoStalwartDefender53) {
            return 1;
        }
        if (!FCheckFleetWP(app, 4, 1, grobjPlanet, 5, grTaskAny, iWarpAny)) {
            if (!FCheckSelection(app, grobjFleet, 4)) {
                app.state.tutor.idtBold = idtReadFirstMessageGotoStalwartDefender53;
            } else {
                app.state.tutor.idtBold = idtSendWallaby;
            }
            return 0;
        }
        if (!FCheckMessages(app, 6, static_cast<MessageId>(idmNone), 0)) {
            app.state.tutor.idtBold = idtRead4MessagesGotoNewDestroyerArmada;
            return 0;
        }
        if (!FCheckFleetWP(app, 12, 1, grobjPlanet, 10, grTaskAny, iWarpAny)) {
            if (!FCheckSelection(app, grobjFleet, 12)) {
                app.state.tutor.idtBold = idtRead4MessagesGotoNewDestroyerArmada;
            } else {
                app.state.tutor.idtBold = idtSendWreakHavocBerserkerStarbaseHacker;
            }
            return 0;
        }
        if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
            app.state.tutor.idtBold = idtReadRestMessages6;
            return 0;
        }
        app.state.tutor.idtBold = idtGenerateTurn;
        app.state.tutor.fTurnDone = 1;
        app.state.tutor.fProgress = 0;
        return 1;
    case 29:
        switch (app.state.tutor.idt) {
        default:
            return 1;
        case idtReadFirstMessageGotoTeamster43:
            app.state.tutor.fNoErrors = 1;
            if (!FCheckFleetWP(app, 3, 1, grobjPlanet, 13, grTaskAny, 5)) {
                app.state.tutor.fNoErrors = 0;
                if (!FCheckSelection(app, grobjFleet, 3)) {
                    app.state.tutor.idtBold = idtReadFirstMessageGotoTeamster43;
                } else {
                    app.state.tutor.idtBold = idtClickStoveTopFleetWaypointsTileDecrease;
                }
                return 0;
            }
            app.state.tutor.fNoErrors = 0;
            if (!FCheckMessages(app, 6, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtRead4Messages;
                return 0;
            }
            if (!FCheckFleetWP(app, 13, 1, grobjPlanet, 10, grTaskAny, iWarpAny)) {
                app.state.tutor.idtBold = idtSendNewDestroyerHackerWell;
                return 0;
            }
            if (!FCheckPlanetRoute(app, 13, 10)) {
                app.state.tutor.idtBold = idtSelectStoveTopControlClickHacker;
                return 0;
            }
            return 1;
        case idtNoticeProductionTileNewShipsWillRouted:
            if (!FCheckMessages(app, 17, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtRead3Messages;
                return 0;
            }
            if (!FCheckResearch(app, Construction, Energy, 30)) {
                app.state.tutor.idtBold = idtOpenResearchDialogSetFieldResearchEnergy;
                return 0;
            }
            if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
                app.state.tutor.idtBold = idtReadRestMessages7;
                return 0;
            }
            app.state.tutor.fNoErrors = 1;
            if (!FCheckFleetWP(app, 6, 0, grobjPlanet, 17, grTaskScrap, iWarpAny)) {
                app.state.tutor.fNoErrors = 0;
                if (!FCheckSelection(app, grobjFleet, 6)) {
                    app.state.tutor.idtBold = idtGotoTeamster7;
                } else {
                    app.state.tutor.idtBold = idtGiveTeamster7OrdersScrapFleet;
                }
                return 0;
            }
            app.state.tutor.fNoErrors = 0;
            return 1;
        case idtLetsFinishOffBerserkersOnceBuildingBombing:
            if (app.lpui->hwndSlotDlg) {
                if (app.state.rgshdef[9].fFree || FCheckShipBuilder(app, 4, iShipAny)) {
                    if (FCheckShipBuilder(app, 4, iShipAny)) {
                        if (app.lpui->lpshdefBuild->hul.rghs[0].cItem != 2) {
                            app.state.tutor.idtBold = idtAddRadiatingHydroRamScoopEngines;
                        } else if (app.lpui->lpshdefBuild->hul.rghs[1].cItem != 4 || app.lpui->lpshdefBuild->hul.rghs[2].cItem != 4 || app.lpui->lpshdefBuild->hul.rghs[3].cItem != 1) {
                            app.state.tutor.idtBold = idtHoldShiftKeyDrag4BlackCat;
                        } else {
                            app.state.tutor.idtBold = idtOkDesignCloseShipDesigner;
                        }
                    } else {
                        app.state.tutor.idtBold = idtSelectAvailableHullTypesChooseB17;
                    }
                } else {
                    app.state.tutor.idtBold = idtOkDesignCloseShipDesigner;
                }
                return 0;
            }
            if (app.state.rgplr[0].cShDef < 10) {
                app.state.tutor.idtBold = idtHitF4OpenShipDesigner4;
                app.state.tutor.idh = idhKeyboardShortcuts;
                return 0;
            }
            if (LpplFromId(app, 13)->lpplprod->iprodMac < 3 || !FCheckQueue(app, 13, 0, grobjFleet, 9, 10, 0)) {
                app.state.tutor.idtBold = idtAdd10B17BombersStoveTops;
                return 0;
            }
            app.state.tutor.idtBold = idtGenerateWhenReady12;
            app.state.tutor.fTurnDone = 1;
            app.state.tutor.fProgress = 0;
            return 1;
        }
    case 30:
        if (app.state.tutor.idt != idtCongratulationsYouveDeclaredWinner) {
            return 1;
        }
        if (!FCheckMessages(app, 5, static_cast<MessageId>(idmNone), 0)) {
            app.state.tutor.idtBold = idtReadFirst3MessagesGotoNewB;
            return 0;
        }
        if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
            if (!FCheckSelection(app, grobjFleet, 14)) {
                app.state.tutor.idtBold = idtReadFirst3MessagesGotoNewB;
            } else {
                app.state.tutor.idtBold = idtReadRestMessages8;
            }
            return 0;
        }
        app.state.tutor.idtBold = idtGenerateWhenYoureReady2;
        app.state.tutor.fTurnDone = 1;
        app.state.tutor.fProgress = 0;
        return 1;
    case 31:
        if (app.state.tutor.idt != idtReadFirst4MessagesGotoWallaby) {
            return 1;
        }
        if (!FCheckMessages(app, 6, static_cast<MessageId>(idmNone), 0)) {
            app.state.tutor.idtBold = idtReadFirst4MessagesGotoWallaby;
            return 0;
        }
        if (!FCheckSelection(app, grobjPlanet, 5)) {
            app.state.tutor.idtBold = idtReadFirst4MessagesGotoWallaby;
            return 0;
        }
        if (LpplFromId(app, 5)->lpplprod->iprodMac < 4 || !FCheckQueue(app, 5, 0, grobjPlanet, mdIdleMine, 100, 1)) {
            app.state.tutor.idtBold = idtAdd100MinesWallabysQueue;
            return 0;
        }
        if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
            app.state.tutor.idtBold = idtReadRestMessages9;
            return 0;
        }
        app.state.tutor.idtBold = idtGenerateWill2;
        app.state.tutor.fTurnDone = 1;
        app.state.tutor.fProgress = 0;
        return 1;
    case 32:
        if (app.state.tutor.idt != idtReadFirst3MessagesGotoDestroyer13) {
            return 1;
        }
        if (!FCheckMessages(app, 2, static_cast<MessageId>(idmNone), 0)) {
            app.state.tutor.idtBold = idtReadFirst3MessagesGotoDestroyer13;
            return 0;
        }
        if (!app.state.tutor.fProgress && !app.state.tutor.fAutoComplete) {
            app.state.tutor.idtBold = idtClickDestroyerFleetCompositionTile;
            return 0;
        }
        if (!FCheckMessages(app, 18, static_cast<MessageId>(idmNone), 0)) {
            if (!FCheckSelection(app, grobjFleet, 12)) {
                app.state.tutor.idtBold = idtReadFirst3MessagesGotoDestroyer13;
            } else {
                app.state.tutor.idtBold = idtRead8MessagesViewAssaultEnemyStarbase;
            }
            return 0;
        }
        app.state.tutor.idtBold = idtReadRestMessagesGenerate;
        if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
            return 0;
        }
        app.state.tutor.fTurnDone = 1;
        app.state.tutor.fProgress = 0;
        return 1;
    case 33:
        if (app.state.tutor.idt != idtReadFirst6MessagesGotoStoveTop) {
            return 1;
        }
        if (!FCheckMessages(app, 10, static_cast<MessageId>(idmNone), 0)) {
            app.state.tutor.idtBold = idtReadFirst6MessagesGotoStoveTop;
            return 0;
        }
        if (LpplFromId(app, 13)->lpplprod->iprodMac < 3 || !FCheckQueue(app, 13, 0, grobjFleet, 9, 10, 0)) {
            if (!FCheckSelection(app, grobjPlanet, 13)) {
                app.state.tutor.idtBold = idtReadFirst6MessagesGotoStoveTop;
            } else {
                app.state.tutor.idtBold = idtAddAnother10B17BombersProduction;
            }
            return 0;
        }
        if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
            app.state.tutor.idtBold = idtReadRestMessagesWatchBattles;
            return 0;
        }
        app.state.tutor.idtBold = idtGenerateWhenReady13;
        app.state.tutor.fTurnDone = 1;
        app.state.tutor.fProgress = 0;
        return 1;
    case 34:
        if (app.state.tutor.idt != idtNothingMuchHappeningYearViewBattleHacker) {
            return 1;
        }
        app.state.tutor.idtBold = idtReadMessagesGenerateWhenReady;
        if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
            return 0;
        }
        app.state.tutor.fTurnDone = 1;
        app.state.tutor.fProgress = 0;
        return 1;
    case 35:
        if (app.state.tutor.idt != idtReadThroughMessages) {
            return 1;
        }
        if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
            app.state.tutor.idtBold = idtReadThroughMessages;
            return 0;
        }
        app.state.tutor.idtBold = idtGenerateWhenReady14;
        app.state.tutor.fTurnDone = 1;
        app.state.tutor.fProgress = 0;
        return 1;
    case 36:
        if (app.state.tutor.idt != idtCongratulationsHaveReachedEndTutorial) {
            return 1;
        }
        if (!FCheckMessages(app, 9999, static_cast<MessageId>(idmNone), 0)) {
            app.state.tutor.idtBold = idtReadMessages3;
            return 0;
        }
        app.state.tutor.idtBold = idtWhenGenerateYoureOwn;
        app.state.tutor.fTurnDone = 1;
        app.state.tutor.fProgress = 0;
        return 1;
    default:
        app.state.tutor.fTurnDone = 1;
        app.state.tutor.fTutorDone = 1;
        return 1;
    }
}

std::int16_t FCheckZip(APPSESSION &app, std::int16_t iZip, ITEMACTION *lpiaGoal, StringId ids) {
    ITEMACTION *piaCur;
    std::int16_t     i;
    char        szT[33];
    std::int16_t     idhSav;

    idhSav = app.state.tutor.idh;
    app.state.tutor.idh = idhCustomZipOrdersDialog;
    if (app.state.tutor.fAutoComplete) {
        app.state.vrgZip[iZip].fValid = 1;
        piaCur = app.state.vrgZip[iZip].txp.rgia;
        i = 0;
        while (i < 5) {
            piaCur->SetIAction(lpiaGoal->iAction());
            i++;
            piaCur++;
            lpiaGoal++;
        }
        TextIO().pfnCchGetString(app, ids, std::span{szT});
        std::strcpy(app.state.vrgZip[iZip].szName, szT);
        return 1;
    }
    if (!app.state.vrgZip[iZip].fValid) {
        if (!app.lpui->hwndZipOrderDlg) {
            app.state.tutor.idh = idhTransport;
        }
        return 0;
    }
    piaCur = app.state.vrgZip[iZip].txp.rgia;
    app.state.tutor.idh = idhWaypointTaskTile;
    i = 0;
    while (i < 5) {
        if (piaCur->iAction() != lpiaGoal->iAction() && piaCur->iAction() != iActionNone) {
            return 0;
        }
        i++;
        piaCur++;
        lpiaGoal++;
    }
    TextIO().pfnCchGetString(app, ids, std::span{szT});
    if (strcmpi(szT, app.state.vrgZip[iZip].szName) == 0) {
        app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
        return 1;
    }
    TutorError(app, idsTutorialHaventGivenZipOrderRightName);
    return 0;
}

std::int16_t FCheckTemplate(APPSESSION &app, std::int16_t iTemplate) {
    std::int16_t i;

    app.state.tutor.idh = idhProductionTemplates;
    if (app.state.tutor.fAutoComplete) {
        app.state.vrgZipProd[0].fValid = 1;
        app.state.vrgZipProd[0].SetZpq1(rgzpqTut[iTemplate]);
        app.state.gd.fChgZipProd = 1;
        return 1;
    }
    if (!app.state.vrgZipProd[0].fValid) {
        return 0;
    }
    if (app.state.vrgZipProd[0].fNoResearch != rgzpqTut[iTemplate].fNoResearch) {
        return 0;
    }
    if (app.state.vrgZipProd[0].cpq != rgzpqTut[iTemplate].cpq) {
        return 0;
    }
    for (i = 0; i < rgzpqTut[iTemplate].cpq; i++) {
        if (app.state.vrgZipProd[0].rgpq[i].w != rgzpqTut[iTemplate].rgpq[i].w) {
            return 0;
        }
    }
    app.state.gd.fChgZipProd = 1;
    return 1;
}

void TutorError(APPSESSION &app, StringId idsError) {
    if (app.state.tutor.fNoErrors) {
        app.state.tutor.idsError = -1;
    } else if (app.state.tutor.idsError != idsError || app.state.tutor.cError++ >= 3) {
        app.state.tutor.cError = 0;
        app.state.tutor.idsError = idsError;
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsError, nullptr), win32::MB_ICONHAND);
    }
    return;
}

std::int16_t FCheckScanner(APPSESSION &app, std::int16_t md, std::int16_t iZoom) {
    std::int16_t idhSav;

    idhSav = app.state.tutor.idh;
    if (app.state.tutor.fAutoComplete) {
        return 1;
    }
    app.state.tutor.idh = idhChoosingYourViewOfTheUniverse;
    if (md != mdScanAny && app.lpui->grbitScan != md) {
        if (md < 6) {
            if ((app.lpui->grbitScan & grbitScanViewMask) != md) {
                return 0;
            }
        } else if ((md & app.lpui->grbitScan) != md) {
            return 0;
        }
    }
    if (iZoom != iZoomAny && iZoom != app.state.iScanZoom) {
        app.state.tutor.idh = idhZooming;
        return 0;
    }
    app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
    return 1;
}

std::int16_t FCheckFleetName(APPSESSION &app, std::int16_t id, StringId ids) {
    FLEET  *lpfl;
    char    szT[33];
    std::int16_t idhSav;

    idhSav = app.state.tutor.idh;
    app.state.tutor.idh = idhNamingFleets;
    lpfl = LpflFromId(app, id);
    if (!lpfl) {
        return 1;
    }
    if (!lpfl->lpszName) {
        if (ids == static_cast<StringId>(idsNoString)) {
            app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
            return 1;
        }
        if (ids == static_cast<StringId>(idsNoString)) {
            return 1;
        }
        return 0;
    }
    TextIO().pfnCchGetString(app, ids, std::span{szT});
    if (_stricmp(szT, lpfl->lpszName) == 0) {
        app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
        return 1;
    }
    TutorError(app, idsTutorialHaveGivenFleetWrongNamePlease);
    return 0;
}

std::int16_t FCheckSummary(APPSESSION &app, GrobjClass grobj, std::int16_t id) {
    std::int16_t fRet;

    if (app.state.gd.fGeneratingTurn) {
        return 1;
    }
    fRet = 0;
    switch (grobj) {
    default:
        break;
    case grobjFleet:
        if (app.state.sel.scan.grobj == grobjFleet && app.state.sel.scan.ifl != iflNone && app.state.rglpfl[app.state.sel.scan.ifl]->id == id) {
            fRet = 1;
            break;
        }
        fRet = 0;
        break;
    case grobjPlanet:
        if (app.state.sel.scan.grobj == grobjPlanet && app.state.sel.scan.idpl == id) {
            fRet = 1;
            break;
        }
        fRet = 0;
        break;
    case grobjThing:
        fRet = app.state.sel.scan.grobj == grobjThing && (id == idAny || app.state.lpThings[app.state.sel.scan.ith].idFull() == id);
    }
    if (!fRet) {
        app.state.tutor.idh = idhKeyToTheScanner;
    }
    return fRet;
}

std::int16_t FCheckSelection(APPSESSION &app, GrobjClass grobj, std::int16_t id) {
    std::int16_t fRet;
    std::int16_t idhSav;

    idhSav = app.state.tutor.idh;
    if (app.state.tutor.fAutoComplete || app.state.gd.fGeneratingTurn) {
        return 1;
    }
    if (app.lpui->mdMsgObj == (grobj == grobjFleet ? 2 : 1) && id == app.lpui->idMsgObj) {
        app.state.tutor.idh = idhMessagesPane;
    } else if (grobj == grobjPlanet && app.state.sel.grobj == grobjFleet && app.state.sel.fl.idPlanet == id) {
        app.state.tutor.idh = idhLocationTile;
    } else if (grobj == grobjFleet && app.state.sel.grobj == grobjPlanet && LpflFromId(app, id)->idPlanet == app.state.sel.pl.id) {
        app.state.tutor.idh = idhFleetsInOrbitTile;
    } else {
        app.state.tutor.idh = idhSelectingAnObjectToCommand;
    }
    fRet = 0;
    if (grobj == grobjPlanet) {
        fRet = app.state.sel.grobj == grobjPlanet && app.state.sel.pl.id == id;
    } else if (grobj == grobjFleet) {
        fRet = app.state.sel.grobj == grobjFleet && app.state.sel.fl.id == id;
    }
    if (fRet) {
        app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
    }
    return fRet;
}

std::int16_t FCheckMessages(APPSESSION &app, std::int16_t imsg, MessageId idm, std::int16_t fFilter) {
    std::int16_t idhSav;

    idhSav = app.state.tutor.idh;
    if (app.state.gd.fGeneratingTurn) {
        return 1;
    }
    app.state.tutor.idh = idhMessagesPane;
    if (imsg == 9999 && IMsgNext(app, 0) != imsgNone) {
        return 0;
    }
    if (imsg != 9999 && imsg != imsgAny && app.state.iMsgCur < imsg) {
        return 0;
    }
    if (idm != static_cast<MessageId>(idmNone)) {
        if (fFilter && app.state.tutor.fAutoComplete && !(app.state.bitfMsgFiltered[idm >> 3] & 1 << (idm & 7))) {
            SetFilteringGroups(app, idm, 1);
            app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
            return 1;
        }
        if (fFilter && !(app.state.bitfMsgFiltered[idm >> 3] & 1 << (idm & 7))) {
            return 0;
        }
        if (!fFilter && PresentationIO().pfnMessageId(app, app.state.iMsgCur) != idm) {
            return 0;
        }
    }
    app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
    return 1;
}

std::int16_t FCheckResearch(APPSESSION &app, TechFieldType iTech, TechFieldType iTechNext, std::int16_t pct) {
    if (app.state.rgplr[0].iTechNow() == iTech && app.state.rgplr[0].iTechNext() == iTechNext && app.state.rgplr[0].pctResearch == pct) {
        return 1;
    }
    app.state.tutor.idh = idhResearchDialog;
    return 0;
}

std::int16_t FCheckFleetWP(APPSESSION &app, std::uint16_t ifl, std::int16_t iord, GrobjClass grobj, std::int16_t id, TaskType grTask, std::uint16_t iWarp) {
    ORDER   ord;
    std::int16_t fRet;
    FLEET  *lpfl;
    std::int16_t idh;
    std::int16_t idhSav;

    fRet = 0;
    idhSav = app.state.tutor.idh;
    lpfl = LpflFromId(app, ifl);
    app.state.tutor.idh = idhSelectingAnObjectToCommand;
    if (!lpfl)
        goto LReturn;
    app.state.tutor.idh = idhAddingFleetWaypoints;
    if (lpfl->cord < iord + 1)
        goto LReturn;
    ord = lpfl->lpplord->rgord()[iord];
    if ((id & 0x7fff) != 0x7fff && (ord.grobj != grobj || ord.id != id)) {
        TutorError(app, idsTutorialHaveGivenFleetWrongDestinationPress);
        app.state.tutor.idh = idhMovingFleetWaypoints;
    } else if (ord.grTask != grTask && grTask != grTaskAny) {
        app.state.tutor.idh = idhWaypointTaskTile;
        if (ord.grTask != grTaskNone) {
            TutorError(app, grTask == grTaskNone ? idsTutorialHaveGivenFleetTaskDestinationWaypoint : idsTutorialHaveGivenFleetWrongTaskDestination);
        }
    } else {
        app.state.tutor.idh = idhFleetWaypointsTile;
        if (iWarp != iWarpAny) {
            fRet = ord.iWarp == iWarp;
        } else {
            fRet = 1;
        }
    }
LReturn:
    idh = app.state.tutor.idh;
    if (!fRet && FCheckSelection(app, grobjFleet, ifl)) {
        app.state.tutor.idh = static_cast<HelpContextId>(idh);
    }
    if (fRet) {
        app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
    }
    return fRet;
}

std::int16_t FCheckPlanetRoute(APPSESSION &app, std::int16_t idpl, std::int16_t idplRoute) {
    PLANET *lppl;
    std::int16_t idhSav;

    idhSav = app.state.tutor.idh;
    lppl = LpplFromId(app, idpl);
    if (!lppl) {
        return 0;
    }
    app.state.tutor.idh = idhRoute;
    if (lppl->idRoute != idplRoute + 1) {
        return 0;
    }
    app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
    return 1;
}

std::int16_t FCheckLayingWP(APPSESSION &app, std::uint16_t ifl, std::int16_t iord, std::int16_t id, std::int16_t iYears) {
    FLEET     *lpfl;
    std::int16_t    idhSav;
    GrobjClass grobj;

    idhSav = app.state.tutor.idh;
    lpfl = LpflFromId(app, ifl);
    if (!lpfl) {
        return 0;
    }
    app.state.tutor.idh = idhLayMineFields;
    grobj = !(id & 0x8000) ? grobjPlanet : grobjFleet;
    if (!FCheckFleetWP(app, ifl, iord, grobj, id & 0x7fff, grTaskLayMines, iWarpAny)) {
        return 0;
    }
    if (lpfl->lpplord->rgord()[iord].tsell().iPlrX != iYears) {
        return 0;
    }
    app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
    return 1;
}

std::int16_t FCheckColonizeWP(APPSESSION &app, std::uint16_t ifl, std::int16_t id, std::uint16_t iWarp) {
    std::int16_t ish;
    FLEET  *lpfl;
    std::int16_t csh;
    std::int16_t idhSav;

    idhSav = app.state.tutor.idh;
    lpfl = LpflFromId(app, ifl);
    if (!lpfl) {
        return 0;
    }
    app.state.tutor.idh = idhColonize;
    csh = 0;
    for (ish = 0; ish < 16; ish++) {
        csh += lpfl->rgcsh[ish];
    }
    if (lpfl->idPlanet == 13 && !FCheckCargo(app, lpfl, 0, 0, 0, 25 * csh)) {
        return 0;
    }
    if (FCheckFleetWP(app, ifl, 1, grobjPlanet, id, grTaskColonize, iWarp)) {
        app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
        return 1;
    }
    return 0;
}

std::int16_t FCheckPatrolWP(APPSESSION &app, std::uint16_t ifl, std::int16_t iord, std::int16_t id, std::uint16_t iWarp, std::uint16_t iPlan, std::uint16_t iDist) {
    FLEET     *lpfl;
    std::int16_t    idhSav;
    GrobjClass grobj;

    idhSav = app.state.tutor.idh;
    lpfl = LpflFromId(app, ifl);
    if (!lpfl) {
        return 0;
    }
    app.state.tutor.idh = idhPatroling;
    grobj = !(id & 0x8000) ? grobjPlanet : grobjFleet;
    if (!FCheckFleetWP(app, ifl, iord, grobj, id & 0x7fff, grTaskPatrol, iWarp)) {
        return 0;
    }
    if (iDist != iDistAny && lpfl->lpplord->rgord()[iord].tptl().iDist != iDist) {
        app.state.tutor.idh = idhWaypointTaskTile;
        return 0;
    }
    app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
    return 1;
}

std::int16_t FCheckXferWP(APPSESSION &app, std::uint16_t ifl, std::int16_t iord, std::int16_t id, std::uint16_t iWarp, ITEMACTION *lpiaGoal) {
    ORDER       ord;
    std::int16_t     fRet;
    ITEMACTION *piaCur;
    std::int16_t     i;
    FLEET      *lpfl;
    std::int16_t     idh;
    GrobjClass  grobj;
    std::int16_t     idhSav;

    fRet = 0;
    idhSav = app.state.tutor.idh;
    if (id & 0x8000) {
        id &= 0x7fff;
        grobj = grobjFleet;
    } else {
        grobj = grobjPlanet;
    }
    lpfl = LpflFromId(app, ifl);
    if (!lpfl) {
        return 0;
    }
    if (!FCheckFleetWP(app, ifl, iord, grobj, id, grTaskXfer, iWarp)) {
        return 0;
    }
    ord = lpfl->lpplord->rgord()[iord];
    piaCur = ord.txp.rgia;
    app.state.tutor.idh = idhWaypointTaskTile;
    i = 0;
    while (i < 5) {
        if (piaCur->iAction() != lpiaGoal->iAction()) {
            if (piaCur->iAction() == iActionNone)
                goto LReturn;
            TutorError(app, idsTutorialHaveGivenIncorrectTransferOrderPlease);
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
    idh = app.state.tutor.idh;
    if (!fRet && FCheckSelection(app, grobjFleet, ifl)) {
        app.state.tutor.idh = static_cast<HelpContextId>(idh);
    }
    if (fRet) {
        app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
    }
    return fRet;
}

std::int16_t FCheckQueue(APPSESSION &app, std::int16_t ipl, std::int16_t iprod, GrobjClass grobj, std::uint16_t iItem, std::uint16_t cItem, std::uint16_t fNoResearch) {
    std::int16_t fRet;
    PLANET *lppl;
    PROD    prod;
    std::int16_t idh;
    std::int16_t idhSav;

    idhSav = app.state.tutor.idh;
    fRet = 0;
    if (app.state.game.turn < 2) {
        app.state.tutor.idh = idhProductionTile;
    } else {
        app.state.tutor.idh = idhProductionDialog;
    }
    lppl = LpplFromId(app, ipl);
    if (!lppl || !lppl->lpplprod || lppl->lpplprod->iprodMac <= iprod)
        goto LReturn;
    prod = lppl->lpplprod->rgprod()[iprod];
    if (prod.grobj != static_cast<std::uint32_t>(grobj) || prod.iItem != static_cast<std::uint32_t>(iItem)) {
        TutorError(app, idsTutorialProductionQueueDoesContainRequestedItem);
    } else if (prod.cItem != static_cast<std::uint32_t>(cItem)) {
        TutorError(app, idsTutorialProductionQueueDoesContainRightCount);
    } else if (fNoResearch != 0xffff && lppl->fNoResearch != static_cast<std::uint32_t>(fNoResearch)) {
        TutorError(app, idsTutorialContributeLeftoverCheckboxProductionQueu);
    } else {
        fRet = 1;
    }
LReturn:
    idh = app.state.tutor.idh;
    if (!fRet && FCheckSelection(app, grobjPlanet, ipl)) {
        app.state.tutor.idh = static_cast<HelpContextId>(idh);
    }
    if (fRet) {
        app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
    }
    return fRet;
}

std::int16_t FCheckBtlPlan(APPSESSION &app, std::int16_t ibp, std::uint16_t imdTarget, std::int16_t fSpread, std::int16_t fBomb, std::int16_t fDump, std::uint16_t mdUnarmed, std::uint16_t mdScout, std::uint16_t mdWar,
                      std::uint16_t mdBomber) {
    BTLPLAN *lpbtlplan;
    std::int16_t  idhSav;

    idhSav = app.state.tutor.idh;
    app.state.tutor.idh = idhChangingTheContentsOfABattlePlan;
    if (ibp < 0 || ibp > app.state.rgcbtlplan[0]) {
        return 0;
    }
    lpbtlplan = app.state.rglpbtlplan[0] + ibp;
    app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
    return 1;
}

std::int16_t FCheckCargo(APPSESSION &app, FLEET *lpfl, std::int16_t wtMin1, std::int16_t wtMin2, std::int16_t wtMin3, std::int16_t wtColonists) {
    std::int16_t fRet;
    std::int16_t idh;
    std::int16_t idhSav;

    idhSav = app.state.tutor.idh;
    fRet = 0;
    if (!lpfl) {
        return 0;
    }
    app.state.tutor.idh = idhCargoTransferDialogs;
    if ((wtMin1 == 0 && lpfl->rgwtMin[0] != 0) || (wtMin2 == 0 && lpfl->rgwtMin[1] != 0) || (wtMin3 == 0 && lpfl->rgwtMin[2] != 0) ||
        (wtColonists == 0 && lpfl->rgwtMin[3] != 0)) {
        TutorError(app, idsTutorialHaveLoadedWrongCargoFleetPlease);
        goto LReturn;
    }
    if (lpfl->rgwtMin[3] == wtColonists && lpfl->rgwtMin[0] == wtMin1 && lpfl->rgwtMin[1] == wtMin2 && lpfl->rgwtMin[2] == wtMin3) {
        fRet = 1;
    }
LReturn:
    idh = app.state.tutor.idh;
    if (!fRet && FCheckSelection(app, grobjFleet, lpfl->id)) {
        app.state.tutor.idh = static_cast<HelpContextId>(idh);
    }
    if (fRet) {
        app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
    }
    return fRet;
}

std::int16_t FCheckBuilderPart(APPSESSION &app, std::int16_t iSlot, HS *phs, std::uint16_t cInit) {
    std::uint16_t cItemAct;
    std::int16_t  idhSav;

    idhSav = app.state.tutor.idh;
    if (!app.lpui->hwndSlotDlg) {
        app.state.tutor.idh = idhShipDesigner;
        return 0;
    }
    app.state.tutor.idh = idhEditingAnExistingShipDesign;
    if (app.lpui->mdBuild != mdBuildEdit) {
        return 0;
    }
    cItemAct = app.lpui->lpshdefBuild->hul.rghs[iSlot].cItem;
    if (phs->cItem == 0 && cItemAct == 0) {
        return 1;
    }
    if (cInit < phs->cItem) {
        if (cItemAct < cInit || cItemAct > phs->cItem) {
        BadCnt:
            TutorError(app, idsTutorialHaventPlacedCorrectNumberComponentsSlot);
        BadCntSilent:
            app.state.tutor.idh = idhDesigningANewShipFromScratch;
            return 0;
        }
    } else if (cItemAct < phs->cItem || cItemAct > cInit) {
        goto BadCnt;
    }
    if (cItemAct != phs->cItem)
        goto BadCntSilent;
    if (phs->grhst != app.lpui->lpshdefBuild->hul.rghs[iSlot].grhst || phs->iItem != app.lpui->lpshdefBuild->hul.rghs[iSlot].iItem) {
        TutorError(app, idsTutorialHavePlacedWrongComponentSlotDrag);
        goto BadCntSilent;
    }
    app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
    return 1;
}

std::int16_t FCheckShipBuilder(APPSESSION &app, std::int16_t iCategory, std::int16_t iShip) {
    std::int16_t iSel;
    std::int16_t idhSav;

    idhSav = app.state.tutor.idh;
    app.state.tutor.idh = idhShipDesigner;
    if (!app.lpui->hwndSlotDlg) {
        return 0;
    }
    if (iCategory != iCategoryAny && iCategory != app.lpui->mdBuild) {
        return 0;
    }
    iSel = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(app.lpui->hwndSlotDlg, IDC_COMBOBOX), win32::CB_GETCURSEL, 0, 0));
    if (iShip == iShipAny || iShip == iSel) {
        app.state.tutor.idh = static_cast<HelpContextId>(idhSav);
        return 1;
    }
    return 0;
}

std::int16_t FTutorialEnabledShipBuilder(APPSESSION &app, TutorShipBuilderAction itutsbAction) {
    HS hs2;
    HS hs3;
    HS hs;
    HS hs1;
    HS hs4;

    switch (itutsbAction) {
    default:
        return 0;
    case tutsbDelete:
        TutorError(app, idsTutorialShouldDeleteShipDesignPointTutorial);
        return 0;
    case tutsbCopy:
        switch (app.state.game.turn) {
        default:
        NoCustom:
            TutorError(app, idsTutorialShouldCustomizeShipDesignPointTutorial);
            return 0;
        case 13:
            if (app.state.tutor.idt != idtSelectAvailableHullTypes)
                goto NoCustom;
            if (app.state.rgplr[0].cShDef == 7) {
                TutorError(app, idsTutorialHaveAlreadyCopiedAppropriateShipDesign);
                return 0;
            }
            if (FCheckShipBuilder(app, 1, 7))
                break;
            TutorError(app, idsTutorialDontHaveCorrectHullSelectedHull);
            return 0;
        case 20:
            if (app.state.tutor.idt != idtReadFirstTwoMessagesSendArmedProbe)
                goto NoCustom;
            if (app.state.rgplr[0].cshdefSB == 2) {
                TutorError(app, idsTutorialHaveAlreadyCopiedAppropriateShipDesign);
                return 0;
            }
            if (FCheckShipBuilder(app, 0, 0) && app.state.fStarbaseMode)
                break;
            TutorError(app, idsTutorialHaveTriedCopyWrongShipDesign);
            return 0;
        case 22:
            if (app.state.tutor.idt != idtAddTeamsterStoveTopsQueue)
                goto NoCustom;
            if (app.state.rgplr[0].cShDef == 8) {
                TutorError(app, idsTutorialHaveAlreadyCopiedAppropriateShipDesign);
                return 0;
            }
            if (FCheckShipBuilder(app, 1, 3))
                break;
            TutorError(app, idsTutorialDontHaveCorrectHullSelectedHull);
            return 0;
        case 27:
            if (app.state.tutor.idt != idtWeWantPowerfulWeAlsoWantWeigh)
                goto NoCustom;
            if (app.state.rgplr[0].cShDef == 9) {
                TutorError(app, idsTutorialHaveAlreadyCopiedAppropriateShipDesign);
                return 0;
            }
            if (FCheckShipBuilder(app, 1, 4))
                break;
            TutorError(app, idsTutorialDontHaveCorrectHullSelectedHull);
            return 0;
        case 29:
            if (app.state.tutor.idt != idtLetsFinishOffBerserkersOnceBuildingBombing)
                goto NoCustom;
            if (app.state.rgplr[0].cShDef == 10) {
                TutorError(app, idsTutorialHaveAlreadyCopiedAppropriateShipDesign);
                return 0;
            }
            if (!FCheckShipBuilder(app, 1, 8)) {
                TutorError(app, idsTutorialDontHaveCorrectHullSelectedHull);
                return 0;
            }
        }
        return 1;
    case tutsbEdit:
        if (app.state.game.turn != 25 || app.state.tutor.idt != idtReadFirstTwoMessages)
            goto NoCustom;
        /* The original broke out of the switch and fell off the end of the
           function, returning FCheckShipBuilder's TRUE in AX. */
        if (FCheckShipBuilder(app, 0, 2))
            return 1;
        TutorError(app, idsTutorialDontHaveCorrectShipSelectedShip);
        return 0;
    case tutsbAccept:
        switch (app.state.game.turn) {
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
            if (app.state.tutor.idt != idtShipDesignNameImageJustFine) {
                TutorError(app, idsTutorialHaventYetFinishedCreatingNewDesign);
                return 0;
            }
            if (FCheckBuilderPart(app, 0, &hs2, 1) && FCheckBuilderPart(app, 1, &hs, 1) && FCheckBuilderPart(app, 2, &hs3, 1) && FCheckBuilderPart(app, 3, &hs3, 1))
                break;
            TutorError(app, idsTutorialDontHaveRightPartsDesignVerify);
            return 0;
        case 20:
            if (app.state.tutor.idt == idtReadFirstTwoMessagesSendArmedProbe) {
                hs.grhst = hstSpecialSB;
                hs.iItem = ispecialSBStargate100250;
                hs.cItem = 1;
                if (!FCheckBuilderPart(app, 0, &hs, 1)) {
                    TutorError(app, idsTutorialHaventAddedRightPartDesignVerify);
                    return 0;
                }
                if (_stricmp(TextIO().pfnCompressed(app, idsGater).data(), app.lpui->lpshdefBuild->hul.szClass) != 0) {
                    TutorError(app, idsTutorialNameDesignEditboxMustGaterChange);
                    return 0;
                }
                if (app.lpui->lpshdefBuild->hul.ibmp == 137)
                    break;
                TutorError(app, idsTutorialHaventPickedCorrectImageDesignPress);
                return 0;
            }
            TutorError(app, idsTutorialHaventYetFinishedCreatingNewDesign);
            return 0;
        case 22:
            if (app.state.tutor.idt == idtAddTeamsterStoveTopsQueue) {
                hs.grhst = hstEngine;
                hs.iItem = iengineDaddyLongLegs7;
                hs.cItem = 1;
                hs1.grhst = hstMines;
                hs1.iItem = iminesMineDispenser50;
                hs1.cItem = 3;
                if (!FCheckBuilderPart(app, 0, &hs, 1) || !FCheckBuilderPart(app, 2, &hs1, 3)) {
                    TutorError(app, idsTutorialDontHaveRightPartsDesignVerify2);
                    return 0;
                }
                if (_stricmp(TextIO().pfnCompressed(app, idsMineLayer).data(), app.lpui->lpshdefBuild->hul.szClass) == 0)
                    break;
                TutorError(app, idsTutorialNameDesignEditboxMustMineLayer);
                return 0;
            }
            TutorError(app, idsTutorialHaventYetFinishedCreatingNewDesign);
            return 0;
        case 25:
            hs.grhst = hstEngine;
            hs.iItem = iengineDaddyLongLegs7;
            hs.cItem = 1;
            hs2.grhst = hstSpecialM;
            hs2.iItem = ispecialMColonizationModule;
            hs2.cItem = 1;
            if (FCheckBuilderPart(app, 0, &hs, 1) && FCheckBuilderPart(app, 1, &hs2, 1))
                break;
            TutorError(app, idsTutorialDontHaveRightPartsDesignVerify);
            return 0;
        case 27:
            if (app.state.tutor.idt == idtWeWantPowerfulWeAlsoWantWeigh) {
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
                if (!FCheckBuilderPart(app, 0, &hs, 1) || !FCheckBuilderPart(app, 1, &hs3, 1) || !FCheckBuilderPart(app, 2, &hs3, 1) || !FCheckBuilderPart(app, 3, &hs3, 1) ||
                    !FCheckBuilderPart(app, 4, &hs4, 2) || !FCheckBuilderPart(app, 5, &hs1, 1) || !FCheckBuilderPart(app, 6, &hs2, 1)) {
                    TutorError(app, idsTutorialVerifyHaveRadiatingHydroRamScoop);
                    return 0;
                }
                if (app.lpui->lpshdefBuild->hul.ibmp == 25)
                    break;
                TutorError(app, idsTutorialHaventPickedCorrectImageDesignPress);
                return 0;
            }
            TutorError(app, idsTutorialHaventYetFinishedCreatingNewDesign);
            return 0;
        case 29:
            if (app.state.tutor.idt == idtLetsFinishOffBerserkersOnceBuildingBombing) {
                hs.grhst = hstEngine;
                hs.iItem = iengineRadiatingHydroRamScoop;
                hs.cItem = 2;
                hs1.grhst = hstSpecialM;
                hs1.iItem = ispecialMFuelTank;
                hs1.cItem = 1;
                hs2.grhst = hstBomb;
                hs2.iItem = ibombBlackCatBomb;
                hs2.cItem = 4;
                if (!FCheckBuilderPart(app, 0, &hs, 2) || !FCheckBuilderPart(app, 1, &hs2, 4) || !FCheckBuilderPart(app, 2, &hs2, 4) || !FCheckBuilderPart(app, 3, &hs1, 1)) {
                    TutorError(app, idsTutorialDontHaveRightPartsDesignVerify3);
                    return 0;
                }
                break;
            }
            TutorError(app, idsTutorialHaventYetFinishedCreatingNewDesign);
            return 0;
        }
        return 1;
    case tutsbCancelEdit:
        TutorError(app, idsTutorialMustFinishTutorialTasksBeforeExiting);
        return 0;
    }
}

std::int16_t FOKMergeDialog(APPSESSION &app) {
    if (app.state.game.turn != 23) {
        if (app.state.game.turn != 27) {
            TutorError(app, idsTutorialHaventAskedMergeAnyFleetsYear);
            return 0;
        }
        if (*app.state.vrgiflMerge == iflNone && app.state.vrgiflMerge[1] == iflNone && app.state.vrgiflMerge[2] != iflNone && app.state.vrgiflMerge[3] != iflNone) {
            return 1;
        }
    } else if (*app.state.vrgiflMerge != iflNone && app.state.vrgiflMerge[1] != iflNone && app.state.vrgiflMerge[2] == iflNone && app.state.vrgiflMerge[3] == iflNone) {
        return 1;
    }
    TutorError(app, idsTutorialHaventSelectedRightFleetsMergeReread);
    app.state.tutor.idh = idhMergeFleetsDialog;
    return 0;
}
