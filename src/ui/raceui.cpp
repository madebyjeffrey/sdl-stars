#include "win.h"
#include <algorithm>
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "text_io.h"
#include "control_io.h"
#include "race_query_io.h"
#include "alert_io.h"
#include "record_io.h"
#include "desktop_choice_io.h"
#include "ownership.h"
#include <stars_compat.h>

std::int16_t RaceCreationWizard(APPSESSION &app, HWND hwndParent, std::int16_t fReadOnly, std::int16_t fDontWrite) {
    WizardButton mdRet;
    RECT         rgrcStack[17];
    std::int16_t      cpts;
    std::int16_t      fResult;
    BOOL         fSequence;

    app.lpui->vrgrcRCW = rgrcStack;
    app.state.fRCWReadOnly = fReadOnly;
    app.lpui->hwndRaceParent = hwndParent;
    fSequence = StarsCompatBeginDialogSequence(hwndParent);
Step1:
    app.lpui->iPanelActive = rwPageRace;
    mdRet = static_cast<WizardButton>(ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_RACE_WIZARD_1), app.lpui->hwndRaceParent, RaceWizardDlg1, 0));
    if (mdRet == wizCancel || mdRet == static_cast<WizardButton>(-1))
        goto LCancel;
    if (mdRet == wizFinish)
        goto Finish;
Step2:
    app.lpui->iPanelActive = rwPagePrimaryTrait;
    mdRet = static_cast<WizardButton>(ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_RACE_WIZARD_4), app.lpui->hwndRaceParent, RaceWizardDlg4, 0));
    if (mdRet == wizCancel || mdRet == static_cast<WizardButton>(-1))
        goto LCancel;
    if (mdRet == wizBack)
        goto Step1;
    if (mdRet == wizFinish)
        goto Finish;
Step3:
    app.lpui->iPanelActive = rwPageLesserTraits;
    mdRet = static_cast<WizardButton>(ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_RACE_WIZARD_5), app.lpui->hwndRaceParent, RaceWizardDlg5, 0));
    if (mdRet == wizCancel || mdRet == static_cast<WizardButton>(-1))
        goto LCancel;
    if (mdRet == wizBack)
        goto Step2;
    if (mdRet == wizFinish)
        goto Finish;
Step4:
    app.lpui->iPanelActive = rwPageHabitability;
    mdRet = static_cast<WizardButton>(ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_RACE_WIZARD_2), app.lpui->hwndRaceParent, RaceWizardDlg2, 0));
    if (mdRet == wizCancel || mdRet == static_cast<WizardButton>(-1))
        goto LCancel;
    if (mdRet == wizBack)
        goto Step3;
    if (mdRet == wizFinish)
        goto Finish;
Step5:
    app.lpui->iPanelActive = rwPageEconomy;
    mdRet = static_cast<WizardButton>(ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_RACE_WIZARD_3), app.lpui->hwndRaceParent, RaceWizardDlg3, 0));
    if (mdRet == wizCancel || mdRet == static_cast<WizardButton>(-1))
        goto LCancel;
    if (mdRet == wizBack)
        goto Step4;
    if (mdRet == wizFinish)
        goto Finish;
Step6:
    app.lpui->iPanelActive = rwPageResearch;
    mdRet = static_cast<WizardButton>(ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_RACE_WIZARD_6), app.lpui->hwndRaceParent, RaceWizardDlg6, 0));
    if (mdRet == wizCancel || mdRet == static_cast<WizardButton>(-1))
        goto LCancel;
    if (mdRet == wizBack)
        goto Step5;
    if (mdRet == wizFinish)
        goto Finish;
Finish:
    if (app.state.fRCWReadOnly) {
        goto LCancel;
    }
    cpts = CAdvantagePoints(app, &app.state.vplr);
    if (cpts < 0) {
        win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsAdvantagePointsCurrentlyHoleDPointsCannot), -cpts);
        AlertIO().pfnAlertSz(app, app.state.szWork, win32::MB_ICONHAND);
        switch (app.lpui->iPanelActive) {
        default:
            goto Step1;
        case rwPagePrimaryTrait:
            goto Step2;
        case rwPageLesserTraits:
            goto Step3;
        case rwPageHabitability:
            goto Step4;
        case rwPageEconomy:
            goto Step5;
        case rwPageResearch:
            goto Step6;
        }
    }
    app.state.lSaltCur = LSaltFromSz(app.state.szRacePass);
    app.state.lSaltLast = -5;
    if (!FCheckPassword(app))
        goto Step1;
    if (!fDontWrite) {
        if (FHasCurrentLibraryRace()) {
            if (!FSaveCurrentLibraryRace(app, app.lpui->hwndRaceParent, &app.state.vplr)) goto Step1;
        } else if (!FSaveRace(app, const_cast<char *>(reinterpret_cast<const char*>((app.state.szRaceFile[0] == 0 ? "stars.r1" : app.state.szRaceFile))), &app.state.vplr)) goto Step1;
    }
    if (fDontWrite) {
        fResult = 1;
        goto LDone;
    }
    fResult = 1;
    goto LDone;
LCancel:
    fResult = 0;
LDone:
    if (fSequence) StarsCompatEndDialogSequence();
    return fResult;
}

INT_PTR  RaceWizardDlg1(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    std::int16_t     i;
    RECT        rc;
    std::int16_t     iPlrBmp;
    HWND        hwndCB;
    char       *psz;
    POINT16     pt;
    HDC         hdc;
    BTNT        btnt;
    std::int16_t     bt;
    RECT       *prc;
    std::int16_t     iDir;
    std::int16_t     iCur;
    std::int16_t     iOffset;
    PLAYER     *pplr;
    PAINTSTRUCT ps;
    std::int16_t     j;
    std::int16_t     cch;
    RECT        rcGBox;
    std::int16_t     k;
    char        szBuf[32];

    switch (message) {
    case win32::WM_INITDIALOG:
        iPlrBmp = app.state.vplr.iPlrBmp;
        SetRCWTitle(app, hwnd, app.lpui->iPanelActive);
        win32::SetDlgItemTextA(hwnd, IDC_RACE_NAME, app.state.vplr.szName);
        win32::SetDlgItemTextA(hwnd, IDC_RACE_PLURAL_NAME, app.state.vplr.szNames);
        if (app.state.vplr.szName[0] == 0) {
            win32::GetDlgItemTextA(hwnd, IDC_RACE_HUMANOID, app.state.vplr.szName, 16);
            win32::SetDlgItemTextA(hwnd, IDC_RACE_NAME, app.state.vplr.szName);
        }
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRaceDlg, 1);
        if (app.state.game.fTutorial && app.state.idPlayer == 0 && app.state.fRCWReadOnly) {
            i = 0;
        } else {
            for (i = 0; i < 7; i++) {
                app.state.vplr.iPlrBmp = vrgplrDef[i].iPlrBmp;
                std::byte rgb[cbWirePlayer], rgbDef[cbWirePlayer];
                FEncodePlayer(app.state.vplr, rgb); FEncodePlayer(vrgplrDef[i], rgbDef);
                if (std::memcmp(rgb, rgbDef, ibWirePlayerNames) == 0)
                    break;
            }
            app.state.vplr.iPlrBmp = iPlrBmp;
        }
        win32::CheckRadioButton(hwnd, IDC_RACE_HUMANOID, IDC_RACE_CUSTOM, i + 271);
        ui::SendDlgItemMessage(hwnd, IDC_RACE_NAME, win32::EM_LIMITTEXT, 0xf, 0);
        ui::SendDlgItemMessage(hwnd, IDC_RACE_PLURAL_NAME, win32::EM_LIMITTEXT, 0xf, 0);
        ui::SendDlgItemMessage(hwnd, IDC_RACE_PASSWORD, win32::EM_LIMITTEXT, 0xf, 0);
        hwndCB = ControlIO().pfnGetDlgItem(hwnd, IDC_COMBOBOX);
        for (i = idsSurfaceMinerals; i <= idsDefenses3; i++) {
            psz = TextIO().pfnCompressed(app, static_cast<StringId>(i));
            ControlIO().pfnSendMessage(hwndCB, win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(psz));
        }
        i = RaceQueryIO().pfnStat(&app.state.vplr, rsUseLeftover);
        ControlIO().pfnSendMessage(hwndCB, win32::CB_SETCURSEL, i, 0);
        if (app.state.vplr.lSalt != 0) {
            FSetUtf8WindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_RACE_PASSWORD), app.state.szRacePass);
        }
        if (app.state.fRCWReadOnly) {
            for (i = 268; i <= 269; i++) {
                ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, i), 0);
            }
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_RACE_PLURAL_NAME), 0);
            for (i = 271; i <= 278; i++) {
                ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, i), 0);
            }
            ControlIO().pfnEnableWindow(hwndCB, 0);
        }
        return 1;
    case win32::WM_LBUTTONDOWN:
    case win32::WM_LBUTTONDBLCLK:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        if (app.state.fRCWReadOnly || (win32::PtInRect(app.lpui->rgrcBuildSpin, PointFrom16(pt)) == 0 && win32::PtInRect(&app.lpui->rgrcBuildSpin[1], PointFrom16(pt)) == 0))
            break;
        if (win32::PtInRect(app.lpui->rgrcBuildSpin, PointFrom16(pt)) != 0) {
            iDir = -1;
            bt = 34;
            prc = app.lpui->rgrcBuildSpin;
        } else {
            iDir = 1;
            bt = 35;
            prc = &app.lpui->rgrcBuildSpin[1];
        }
        iCur = app.state.vplr.iPlrBmp;
        if (iCur >= 32) {
            iCur = 0;
        }
        hdc = win32::GetDC(hwnd);
        win32::SelectPalette(hdc, app.lpui->vhpal, 0);
        win32::RealizePalette(hdc);
        InitBtnTrack(&btnt, hwnd, nullptr, prc, bt, 80, 0, 0, nullptr);
        while (FTrackBtn(app, &btnt)) {
            iCur = static_cast<std::int16_t>((iCur + 32 + iDir)) % 32;
            DibBlt(hdc, app.lpui->rgrcBuildSpin[0].left - 2, app.lpui->rgrcBuildSpin[0].top - 35, 32, 32, app.lpui->hdibRaces, (iCur & 7) * 0x20, (3 - (iCur >> 3)) * 0x20, 32, 32, 13369376);
        }
        app.state.vplr.iPlrBmp = iCur;
        win32::ReleaseDC(hwnd, hdc);
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
        for (i = 271; i <= 278 && reinterpret_cast<HWND>(lParam) != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 278 || message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        for (j = 271; j <= 278 && win32::IsDlgButtonChecked(hwnd, j) == 0; j++) {
        }
        if (j <= 277) {
            k = j - 271;
            pplr = &vrgplrDef[k];
        } else {
            pplr = &app.state.vplr;
        }
        ControlIO().pfnGetClientRect(hwnd, &rc);
        DrawRaceAdvantagePoints(app, hdc, &rc, pplr);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_RACE_HUMANOID), &rcGBox);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcGBox));
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_RACE_CUSTOM), &rc);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rc.right));
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, app.state.dyArial8, app.state.dyArial8 >> 1);
        _Draw3dFrame(app, hdc, &rcGBox, -1);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        win32::SetBkColor(hdc, app.lpui->crButtonFace);
        cch = TextIO().pfnCchGetString(app, idsPredefinedRaces, std::span{app.state.szWork});
        win32::TextOutA(hdc, rcGBox.left + 8, rcGBox.top - (app.state.dyArial8 >> 1), app.state.szWork, cch);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_COMBOBOX), &rcGBox);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcGBox.right));
        pt.x = rcGBox.right + 32;
        pt.y = rcGBox.bottom - 32;
        iOffset = app.state.vplr.iPlrBmp;
        if (iOffset >= 32) {
            iOffset = 0;
        }
        win32::SelectPalette(hdc, app.lpui->vhpal, 0);
        win32::RealizePalette(hdc);
        DibBlt(hdc, pt.x, pt.y, 32, 32, app.lpui->hdibRaces, (iOffset & 7) * 0x20, (3 - (iOffset >> 3)) * 0x20, 32, 32, 13369376);
        if (!app.state.fRCWReadOnly) {
            win32::SetRect(app.lpui->rgrcBuildSpin, pt.x + 2, pt.y + 35, pt.x + 16, pt.y + 49);
            app.lpui->rgrcBuildSpin[1] = app.lpui->rgrcBuildSpin[0];
            win32::OffsetRect(&app.lpui->rgrcBuildSpin[1], 14, 0);
            for (i = 0; i < 2; i++) {
                DrawBtn(app, hdc, &app.lpui->rgrcBuildSpin[i], (i == 0 ? 2 : 3) | 0x20, 0, nullptr);
            }
        }
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhStep1BasicDefinition);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != app.lpui->rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            if (LOWORD(wParam) != IDCANCEL) {
                iPlrBmp = app.state.vplr.iPlrBmp;
                for (j = 271; j <= 278 && win32::IsDlgButtonChecked(hwnd, j) == 0; j++) {
                }
                if (j <= 277) {
                    k = j - 271;
                    app.state.vplr = vrgplrDef[k];
                } else {
                    /* A custom race isn't random. The original kept the
                       random-race bit of a race that had been Random, and
                       GenerateWorld replaced the customized race. */
                    SetRaceGrbit(&app.state.vplr, ibitRaceAIPlayer, 0);
                }
                win32::GetDlgItemTextA(hwnd, IDC_RACE_NAME, app.state.vplr.szName, 32);
                win32::GetDlgItemTextA(hwnd, IDC_RACE_PLURAL_NAME, app.state.vplr.szNames, 32);
                RaceQueryIO().pfnStat(&app.state.vplr, rsUseLeftover);
                if (CchGetUtf8WindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_RACE_PASSWORD), app.state.szRacePass, 16) < 0) {
                    AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsNativePasswordReenter), win32::MB_ICONHAND);
                    return 1;
                }
                j = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_COMBOBOX), win32::CB_GETCURSEL, 0, 0));
                SetRaceStat(&app.state.vplr, rsUseLeftover, j);
                app.state.vplr.lSalt = LSaltFromSz(app.state.szRacePass);
                app.state.vplr.iPlrBmp = iPlrBmp;
            }
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRaceDlg, 0);
            ControlIO().pfnEndDialog(hwnd, i);
            return 1;
        }
        if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_RACE_HUMANOID && LOWORD(wParam) <= IDC_RACE_CUSTOM) {
            std::ranges::fill(app.state.vplr.szName, 0);
            win32::GetDlgItemTextA(hwnd, IDC_RACE_NAME, app.state.vplr.szName, 32);
            std::ranges::fill(app.state.vplr.szNames, 0);
            win32::GetDlgItemTextA(hwnd, IDC_RACE_PLURAL_NAME, app.state.vplr.szNames, 32);
            win32::GetDlgItemTextA(hwnd, LOWORD(wParam), szBuf, 32);
            for (i = 0; i < 7 && std::strcmp(app.state.vplr.szName, TextIO().pfnCompressed(app, static_cast<StringId>(idsHumanoid + i))) != 0; i++) {
            }
            if (i < 7 && LOWORD(wParam) < IDC_RACE_CUSTOM) {
                std::ranges::fill(app.state.vplr.szName, 0);
                TextIO().pfnCchGetString(app, static_cast<StringId>(idsHumanoid + LOWORD(wParam) - IDC_RACE_HUMANOID), std::span{app.state.vplr.szName});
                win32::SetDlgItemTextA(hwnd, IDC_RACE_NAME, app.state.vplr.szName);
                std::ranges::fill(app.state.vplr.szNames, 0);
                psz = TextIO().pfnPlayerName(app, 0, 1, 1, 0, 0, &app.state.vplr);
                CchCopyUtf8Prefix(std::span{app.state.vplr.szNames}, psz);
                win32::SetDlgItemTextA(hwnd, IDC_RACE_PLURAL_NAME, app.state.vplr.szNames);
            }
            if (LOWORD(wParam) <= IDC_RACE_RANDOM) {
                pplr = &vrgplrDef[LOWORD(wParam) - 271];
            } else {
                pplr = &app.state.vplr;
            }
            InvalidateAdvPtsRect(app, hwnd);
            i = RaceQueryIO().pfnStat(pplr, rsUseLeftover);
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_COMBOBOX), win32::CB_SETCURSEL, i, 0);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_NEXT), LOWORD(wParam) != IDC_RACE_RANDOM);
            app.state.vplr.iPlrBmp = pplr->iPlrBmp;
            win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_COMBOBOX), &rc);
            win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rc.right));
            rc.left = rc.right + 32;
            rc.top = rc.bottom - 32;
            rc.right = rc.left + 32;
            rc.bottom = rc.top + 32;
            ControlIO().pfnInvalidateRect(hwnd, &rc, 1);
        }
    }
    return 0;
}

INT_PTR  RaceWizardDlg2(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    std::int16_t     i;
    RECT        rc;
    HDC         hdc;
    std::int16_t     yTop;
    std::int16_t     dy;
    std::int16_t     dxMiddle;
    std::int16_t     dxLabel;
    std::int16_t     cch;
    char        szTemp[20];
    HFONT       hfontSav;
    PAINTSTRUCT ps;
    POINT16     pt;
    std::int16_t     iVar;

    switch (message) {
    case win32::WM_INITDIALOG:
        SetRCWTitle(app, hwnd, app.lpui->iPanelActive);
        app.lpui->viStore = -1;
        hdc = win32::GetDC(hwnd);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        hfontSav = static_cast<HFONT>(win32::SelectObject(hdc, app.lpui->rghfontArial8[1]));
        cch = TextIO().pfnCchGetString(app, idsTemperature, std::span{szTemp});
        dxLabel = LOWORD(GetTextExtent(hdc, szTemp, cch)) + 10;
        dxMiddle = LOWORD(GetTextExtent(hdc, "200mR", 5)) + 10;
        dxMiddle = rc.right - dxLabel - dxMiddle;
        dy = static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
        yTop = 3 * app.state.dyArial8;
        win32::SetRect(app.lpui->vrgrcRCW, dxLabel, yTop, dxLabel + dy, yTop + dy);
        win32::SetRect(app.lpui->vrgrcRCW + 1, dxLabel + dy + 6, yTop, dxLabel + dxMiddle - dy - 6, yTop + dy);
        win32::SetRect(app.lpui->vrgrcRCW + 2, dxLabel + dxMiddle - dy, yTop, dxLabel + dxMiddle, yTop + dy);
        win32::SetRect(app.lpui->vrgrcRCW + 3, dxLabel, yTop + dy + 4, 3 * dy + dxLabel, dy * 2 + yTop + 4);
        win32::SetRect(app.lpui->vrgrcRCW + 4, dxLabel + dxMiddle - 3 * dy, yTop + dy + 4, dxLabel + dxMiddle, dy * 2 + yTop + 4);
        for (i = 0; i < 5; i++) {
            app.lpui->vrgrcRCW[i + 5] = app.lpui->vrgrcRCW[i];
            win32::OffsetRect(reinterpret_cast<RECT *>(&app.lpui->vrgrcRCW[i + 5].left), 0, 3 * dy + app.state.dyArial8 / 2);
            app.lpui->vrgrcRCW[i + 10] = app.lpui->vrgrcRCW[i];
            win32::OffsetRect(reinterpret_cast<RECT *>(&app.lpui->vrgrcRCW[i + 0xa].left), 0, 6 * dy + app.state.dyArial8);
        }
        for (i = 0; i < 3; i++) {
            ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, i + 291), nullptr, 3 * dy + dxLabel + 6, app.lpui->vrgrcRCW[5 * i + 3].top, dxMiddle - 6 * dy - 12, dy, win32::SWP_NOZORDER);
            win32::CheckDlgButton(hwnd, i + 291, app.state.vplr.rgEnvVarMax[i] < 0);
        }
        cch = TextIO().pfnCchGetString(app, idsMaximumColonistGrowthRatePerYear, std::span{app.state.szWork});
        app.lpui->vrgrcRCW[15].left = dxLabel + LOWORD(GetTextExtent(hdc, app.state.szWork, cch)) + LOWORD(GetTextExtent(hdc, "15%", 3)) + 4;
        app.lpui->vrgrcRCW[15].top = 9 * dy + yTop - 3 + 3 * (app.state.dyArial8 / 2);
        app.lpui->vrgrcRCW[15].right = app.lpui->vrgrcRCW[15].left + 15;
        app.lpui->vrgrcRCW[15].bottom = (app.state.dyArial8 >> 1) + app.lpui->vrgrcRCW[15].top + 3;
        app.lpui->vrgrcRCW[16] = app.lpui->vrgrcRCW[15];
        win32::OffsetRect(app.lpui->vrgrcRCW + 16, 0, app.lpui->vrgrcRCW[15].bottom - app.lpui->vrgrcRCW[15].top - 1);
        app.lpui->crcRCW = 17;
        win32::SelectObject(hdc, hfontSav);
        win32::ReleaseDC(hwnd, hdc);
        if (app.state.fRCWReadOnly) {
            for (i = 291; i <= 293; i++) {
                ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, i), 0);
            }
        }
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRaceDlg, 1);
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
        for (i = 291; i <= 293 && reinterpret_cast<HWND>(lParam) != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 293 || message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        app.lpui->viStore = -2;
        DrawRace2(app, hwnd, hdc, -1);
        win32::EndPaint(hwnd, &ps);
        return 1;
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
        return FTrackRaceDlg2(app, hwnd, pt, wParam);
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhStep4PopulationGrowthFactors);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != app.lpui->rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRaceDlg, 0);
            ControlIO().pfnEndDialog(hwnd, i);
            return 1;
        }
        if (LOWORD(wParam) >= IDC_IMMUNE_TO_GRAVITY && LOWORD(wParam) <= IDC_IMMUNE_TO_RADIATION) {
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, LOWORD(wParam)), win32::BM_GETCHECK, 0, 0));
            iVar = LOWORD(wParam) - 291;
            if (i == 1) {
                app.state.vplr.rgEnvVar[iVar] = envImmune;
                app.state.vplr.rgEnvVarMax[iVar] = envImmune;
                app.state.vplr.rgEnvVarMin[iVar] = envImmune;
            } else {
                app.state.vplr.rgEnvVarMin[iVar] = 20;
                app.state.vplr.rgEnvVarMax[iVar] = 80;
                app.state.vplr.rgEnvVar[iVar] = 50;
            }
            DrawRace2(app, hwnd, nullptr, 1 << iVar | 0xff00);
        }
    }
    return 0;
}

void DrawRace2(APPSESSION &app, HWND hwnd, HDC hdc, std::int16_t iDraw) {
    std::int16_t iPit;
    std::int16_t bt;
    std::int16_t iMax;
    char    szT[32];
    std::int16_t dy;
    std::int16_t iMin;
    std::int16_t bkMode;
    std::int16_t fCreatedDC;
    std::int16_t xRLabel;
    std::int16_t i;
    std::int16_t iMod;
    char   *psz;
    std::int16_t dx;
    std::int16_t cch;
    std::int16_t bt1;
    RECT    rc;
    std::int32_t l2;
    std::int16_t iStore;
    std::int32_t l;

    fCreatedDC = 0;
    bt1 = !app.state.fRCWReadOnly ? 0 : 4;
    if (!hdc) {
        fCreatedDC = 1;
        hdc = win32::GetDC(hwnd);
    }
    ControlIO().pfnGetClientRect(hwnd, &rc);
    DrawRaceAdvantagePoints(app, hdc, &rc, nullptr);
    bkMode = win32::SetBkMode(hdc, win32::TRANSPARENT);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    xRLabel = static_cast<std::int16_t>((rc.right - app.lpui->vrgrcRCW[2].right)) / 2 + app.lpui->vrgrcRCW[2].right;
    for (i = 0; i < 3; i++) {
        if (1 << i & iDraw) {
            if (iDraw != -1) {
                win32::SelectObject(hdc, app.lpui->hbrButtonFace);
                win32::PatBlt(hdc, app.lpui->vrgrcRCW[2].right, app.lpui->vrgrcRCW[5 * i + 2].top, rc.right - app.lpui->vrgrcRCW[2].right, app.lpui->vrgrcRCW[5 * i + 4].bottom - app.lpui->vrgrcRCW[5 * i + 2].top,
                       win32::PATCOPY);
            }
            if (app.state.vplr.rgEnvVarMax[i] < 0) {
                cch = TextIO().pfnCchGetString(app, idsN2, std::span{szT});
                CtrTextOut(hdc, xRLabel, app.lpui->vrgrcRCW[5 * i + 2].top + 1 + app.state.dyArial8, szT, cch);
            } else {
                psz = PszCalcEnvVar(app, static_cast<EnvType>(i), app.state.vplr.rgEnvVarMin[i]);
                CtrTextOut(hdc, xRLabel, app.lpui->vrgrcRCW[5 * i + 2].top + 1, psz, 0);
                cch = TextIO().pfnCchGetString(app, idsTo2, std::span{szT});
                CtrTextOut(hdc, xRLabel, app.lpui->vrgrcRCW[5 * i + 2].top + 1 + app.state.dyArial8, szT, cch);
                psz = PszCalcEnvVar(app, static_cast<EnvType>(i), app.state.vplr.rgEnvVarMax[i]);
                CtrTextOut(hdc, xRLabel, app.lpui->vrgrcRCW[5 * i + 2].top + 1 + app.state.dyArial8 * 2, psz, 0);
            }
            if (iDraw == -1) {
                RightTextOut(app, hdc, app.lpui->vrgrcRCW[5 * i].left - 4, app.state.dyArial8 / 4 + app.lpui->vrgrcRCW[5 * i].top, app.state.rgszPlanetAttr[i], 0, 0);
            }
        }
    }
    iPit = 0;
    iMod = 0;
    i = 0;
    while (i < 15) {
        if (iMod >= 5) {
            iMod = 0;
        }
        bt = app.state.vplr.rgEnvVarMax[i / 5] >= 0 ? 0 : 4;
        switch (iMod) {
        case 0:
        case 2:
            if (!(iDraw & 0xfff0))
                break;
            DrawBtn(app, hdc, app.lpui->vrgrcRCW + i, (iMod == 0 ? 2 : 3) | bt | bt1, 0, nullptr);
            break;
        default:
            if (!(iDraw & 0xfff0))
                break;
            DrawBtn(app, hdc, app.lpui->vrgrcRCW + i, 8 | bt | bt1, 0, const_cast<char *>(reinterpret_cast<const char*>((iMod == 3 ? "<<     >>" : ">>     <<"))));
            break;
        case 1:
            if (1 << iPit & iDraw) {
                win32::PatBlt(hdc, app.lpui->vrgrcRCW[i].left, app.lpui->vrgrcRCW[i].top, app.lpui->vrgrcRCW[i].right - app.lpui->vrgrcRCW[i].left, app.lpui->vrgrcRCW[i].bottom - app.lpui->vrgrcRCW[i].top, win32::BLACKNESS);
                if (bt == 0) {
                    iMin = app.state.vplr.rgEnvVarMin[iPit];
                    iMax = app.state.vplr.rgEnvVarMax[iPit];
                    dx = app.lpui->vrgrcRCW[i].right - app.lpui->vrgrcRCW[i].left - 2;
                    dy = app.lpui->vrgrcRCW[i].bottom - app.lpui->vrgrcRCW[i].top - 2;
                    win32::SelectObject(hdc, app.lpui->rghbrPlanetAttr[iPit][0]);
                    win32::PatBlt(hdc, win32::MulDiv(iMin, dx, 100) + app.lpui->vrgrcRCW[i].left + 1, app.lpui->vrgrcRCW[i].top + 1, win32::MulDiv(iMax - iMin, dx, 100), dy, win32::PATCOPY);
                }
            }
            iPit++;
        }
        i++;
        iMod++;
    }
    if (iDraw & 8) {
        if (iDraw == -1) {
            cch = TextIO().pfnCchGetString(app, idsMaximumColonistGrowthRatePerYear, std::span{app.state.szWork});
            win32::TextOutA(hdc, app.lpui->vrgrcRCW->left, app.lpui->vrgrcRCW[15].top + 3, app.state.szWork, cch);
            DrawBtn(app, hdc, app.lpui->vrgrcRCW + 15, 0xa0 | bt1, 0, nullptr);
            DrawBtn(app, hdc, app.lpui->vrgrcRCW + 16, 0xa1 | bt1, 0, nullptr);
        } else {
            dx = LOWORD(GetTextExtent(hdc, "15%", 3));
            win32::SelectObject(hdc, app.lpui->hbrButtonFace);
            win32::PatBlt(hdc, app.lpui->vrgrcRCW[15].left - 4 - dx, app.lpui->vrgrcRCW[15].top + 3, dx, app.state.dyArial8, win32::PATCOPY);
        }
        cch = win32::wsprintfA(app.state.szWork, app.state.PCTDPCTPCT, app.state.vplr.pctIdealGrowth);
        RightTextOut(app, hdc, app.lpui->vrgrcRCW[15].left - 4, app.lpui->vrgrcRCW[15].top + 3, app.state.szWork, cch, 0);
    }
    if (iDraw & 7) {
        l = 1;
        for (i = 0; i < 3; i++) {
            if (app.state.vplr.rgEnvVarMax[i] < 0 || app.state.vplr.rgEnvVarMax[i] - app.state.vplr.rgEnvVarMin[i] == 100) {
                l = static_cast<std::uint32_t>((l * 100));
            } else if (i == 2) {
                l = static_cast<std::uint32_t>((l * static_cast<std::int16_t>((app.state.vplr.rgEnvVarMax[i] - app.state.vplr.rgEnvVarMin[i]))));
            } else {
                l2 = 0;
                for (iStore = app.state.vplr.rgEnvVarMin[i]; iStore <= app.state.vplr.rgEnvVarMax[i]; iStore++) {
                    if (iStore < 10) {
                        l2 += iStore;
                    } else if (iStore < 90) {
                        l2 += 10;
                    } else {
                        l2 += static_cast<std::int16_t>((100 - iStore));
                    }
                }
                l = static_cast<std::int32_t>((l * l2)) / 9;
            }
        }
        if (l < 1) {
            l = 1;
        }
        l2 = static_cast<std::int32_t>(((static_cast<std::int32_t>((l >> 1)) + 0xf4240) / l));
        iStore = LOWORD(l2);
        if (l == 1000000) {
            iStore = 0;
        }
        if (iStore != app.lpui->viStore) {
            app.lpui->viStore = iStore;
            if (l2 == 1) {
                cch = TextIO().pfnCchGetString(app, l == 1000000 ? idsPlanetsWillHabitableRace : idsVirtuallyPlanetsWillHabitableRace, std::span{app.state.szWork});
            } else {
                // The explanation exceeds szT's 32 bytes; format from the text
                // table instead of corrupting the habitat page's stack.
                cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsCanExpect1DPlanetsWillHabitable), l2);
            }
            rc.left = app.lpui->vrgrcRCW->left;
            rc.top = app.lpui->vrgrcRCW[15].top + app.state.dyArial8 + 8;
            rc.right = app.lpui->vrgrcRCW[15].right + 20;
            rc.bottom = 3 * app.state.dyArial8 + rc.top;
            win32::SelectObject(hdc, app.lpui->hbrButtonFace);
            win32::PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, win32::PATCOPY);
            win32::DrawTextA(hdc, app.state.szWork, cch, &rc, win32::DT_WORDBREAK);
        }
    }
    win32::SetBkMode(hdc, bkMode);
    if (fCreatedDC) {
        win32::ReleaseDC(hwnd, hdc);
    }
    return;
}

std::int16_t IrcRaceDlgHitTest(APPSESSION &app, POINT16 pt) {
    std::int16_t i;

    if (app.state.fRCWReadOnly) {
        return -1;
    }
    for (i = 0; i < app.lpui->crcRCW && win32::PtInRect(reinterpret_cast<RECT *>(&app.lpui->vrgrcRCW[i].left), PointFrom16(pt)) == 0; i++) {
    }
    if (i < app.lpui->crcRCW) {
        if (app.lpui->iPanelActive == rwPageHabitability && app.state.vplr.rgEnvVarMax[i / 5] < 0) {
            return -1;
        }
        return i;
    }
    return -1;
}

std::int16_t FTrackRaceDlg2(APPSESSION &app, HWND hwnd, POINT16 pt, std::int16_t kbd) {
    BTNT    btnt;
    std::int16_t bt;
    std::int16_t dShift;
    std::int8_t  iMax;
    std::int8_t  iMin;
    std::int16_t i;
    std::int16_t irc;
    std::int16_t iMod;
    const char *psz;
    std::int16_t dWidth;
    std::int16_t dx;

    irc = IrcRaceDlgHitTest(app, pt);
    if (irc < 0) {
        return 0;
    }
    iMod = irc % 5;
    i = irc / 5;
    if (iMod == 1 && irc < 15) {
        win32::SetCapture(hwnd);
        win32::SetCursor(app.lpui->hcurCloseGrab);
        dWidth = static_cast<std::int16_t>((app.state.vplr.rgEnvVarMax[i] - app.state.vplr.rgEnvVarMin[i])) / 2;
        while (FGetMouseMove(&pt)) {
            if (pt.x < app.lpui->vrgrcRCW[irc].left) {
                pt.x = app.lpui->vrgrcRCW[irc].left;
            }
            if (pt.x > app.lpui->vrgrcRCW[irc].right) {
                pt.x = app.lpui->vrgrcRCW[irc].right;
            }
            dShift = win32::MulDiv(pt.x - app.lpui->vrgrcRCW[irc].left, 100, app.lpui->vrgrcRCW[irc].right - app.lpui->vrgrcRCW[irc].left);
            if (dWidth > (dShift >= 100 - dWidth ? 100 - dWidth : dShift)) {
                dShift = dWidth;
            } else if (dShift >= 100 - dWidth) {
                dShift = 100 - dWidth;
            }
            if (app.state.vplr.rgEnvVarMax[i] != dShift + dWidth) {
                app.state.vplr.rgEnvVarMin[i] = dShift - dWidth;
                app.state.vplr.rgEnvVarMax[i] = dShift + dWidth;
                app.state.vplr.rgEnvVar[i] = app.state.vplr.rgEnvVarMin[i] + static_cast<std::int16_t>((app.state.vplr.rgEnvVarMax[i] - app.state.vplr.rgEnvVarMin[i])) / 2;
                DrawRace2(app, hwnd, nullptr, 1 << i);
            }
        }
        win32::ReleaseCapture();
        return 1;
    }
    dWidth = 0;
    dShift = 0;
    psz = nullptr;
    if (irc == 15) {
        dShift = 1;
        bt = 160;
    } else if (irc == 16) {
        dShift = -1;
        bt = 161;
    } else {
        switch (iMod) {
        case 0:
            dShift = -1;
            bt = 2;
            break;
        case 2:
            dShift = 1;
            bt = 3;
            break;
        case 3:
            dWidth = 1;
            bt = 8;
            psz = app.lpui->vrgszRCWWidth[0];
            break;
        default:
            dWidth = -1;
            bt = 8;
            psz = app.lpui->vrgszRCWWidth[1];
        }
    }
    InitBtnTrack(&btnt, hwnd, nullptr, app.lpui->vrgrcRCW + irc, bt, 80, 0, 0, psz);
    if (kbd & 4) {
        dWidth = 10 * dWidth;
        dShift = 10 * dShift;
    }
    while (FTrackBtn(app, &btnt)) {
        if (irc == 15 || irc == 16) {
            iMin = app.state.vplr.pctIdealGrowth + dShift;
            iMin = (((20) < (StarsMax(1, iMin))) ? (20) : (StarsMax(1, iMin)));
            if (iMin != app.state.vplr.pctIdealGrowth) {
                app.state.vplr.pctIdealGrowth = iMin;
                DrawRace2(app, hwnd, btnt.hdc, 8);
            }
        } else {
            iMin = app.state.vplr.rgEnvVarMin[i] - (dWidth - dShift);
            iMax = app.state.vplr.rgEnvVarMax[i] + (dWidth + dShift);
            if (iMax > 100) {
                iMin -= iMax - 100;
                iMax = 100;
            }
            if (iMin < 0) {
                iMax = 100 >= iMax - iMin ? iMax - iMin : 100;
                iMin = 0;
            }
            dx = iMax - iMin;
            if (dx < 20) {
                dx = (0x14 - dx) >> 1;
                iMin -= dx;
                iMax += dx;
            }
            if (app.state.vplr.rgEnvVarMin[i] != iMin || app.state.vplr.rgEnvVarMax[i] != iMax) {
                app.state.vplr.rgEnvVarMin[i] = iMin;
                app.state.vplr.rgEnvVarMax[i] = iMax;
                app.state.vplr.rgEnvVar[i] = app.state.vplr.rgEnvVarMin[i] + static_cast<std::int16_t>((app.state.vplr.rgEnvVarMax[i] - app.state.vplr.rgEnvVarMin[i])) / 2;
                DrawRace2(app, hwnd, btnt.hdc, 1 << i);
            }
        }
    }
    if (irc < 15) {
        app.state.vplr.rgEnvVar[i] = app.state.vplr.rgEnvVarMin[i] + static_cast<std::int16_t>((app.state.vplr.rgEnvVarMax[i] - app.state.vplr.rgEnvVarMin[i])) / 2;
    }
    return 1;
}

INT_PTR  RaceWizardDlg3(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    std::int16_t     i;
    RECT        rc;
    POINT16     pt;
    HDC         hdc;
    PAINTSTRUCT ps;

    switch (message) {
    case win32::WM_INITDIALOG:
        SetRCWTitle(app, hwnd, app.lpui->iPanelActive);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_RACE_FACTORY_GERMANIUM_DISCOUNT), win32::BM_SETCHECK, RaceQueryIO().pfnGrbit(&app.state.vplr, ibitRaceCheapFact), 0);
        if (app.state.fRCWReadOnly || RaceQueryIO().pfnStat(&app.state.vplr, rsMajorAdv) == raMacintosh) {
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_RACE_FACTORY_GERMANIUM_DISCOUNT), 0);
        }
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRaceDlg, 1);
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
        if (reinterpret_cast<HWND>(lParam) == ControlIO().pfnGetDlgItem(hwnd, IDC_RACE_FACTORY_GERMANIUM_DISCOUNT) || message == win32::WM_CTLCOLORSTATIC) {
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
        return FTrackRaceDlg3(app, hwnd, pt, wParam);
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        DrawRace3(app, hwnd, hdc, -1);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhStep5PopulationEfficiencyPlayerRace);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != app.lpui->rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRaceDlg, 0);
            ControlIO().pfnEndDialog(hwnd, i);
            return 1;
        }
        if (LOWORD(wParam) == IDC_RACE_FACTORY_GERMANIUM_DISCOUNT) {
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_RACE_FACTORY_GERMANIUM_DISCOUNT), win32::BM_GETCHECK, 0, 0));
            SetRaceGrbit(&app.state.vplr, ibitRaceCheapFact, i);
            DrawRace3(app, hwnd, nullptr, 99);
        }
    }
    return 0;
}

void DrawRace3(APPSESSION &app, HWND hwnd, HDC hdc, std::int16_t iDraw) {
    std::int16_t  dxItem;
    StringId idsT;
    std::int16_t  fMacintosh;
    std::int16_t  yTop;
    std::int16_t  bt;
    StringId ids;
    COLORREF crBkSav;
    std::int16_t  bkMode;
    std::int16_t  fCreatedDC;
    std::int16_t  dxkT;
    std::int16_t  i;
    std::int16_t  irc;
    std::int16_t  dxDig;
    std::int16_t  dx;
    std::int16_t  cch;
    RECT     rc;

    fCreatedDC = 0;
    bt = !app.state.fRCWReadOnly ? 0 : 4;
    fMacintosh = RaceQueryIO().pfnStat(&app.state.vplr, rsMajorAdv) == raMacintosh;
    if (!hdc) {
        fCreatedDC = 1;
        hdc = win32::GetDC(hwnd);
    }
    ControlIO().pfnGetClientRect(hwnd, &rc);
    DrawRaceAdvantagePoints(app, hdc, &rc, nullptr);
    bkMode = win32::SetBkMode(hdc, win32::OPAQUE);
    crBkSav = win32::SetBkColor(hdc, app.lpui->crButtonFace);
    win32::SetTextColor(hdc, app.lpui->crWindowText);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    yTop = 3 * app.state.dyArial8 + 6;
    dxDig = LOWORD(GetTextExtent(hdc, "9", 1));
    dxkT = LOWORD(GetTextExtent(hdc, "kT", 2));
    ids = idsOneResourceGeneratedEachYearEvery;
    irc = 0;
    for (i = 0; i < 7; i++) {
        if (i == 1 && fMacintosh) {
            win32::SetTextColor(hdc, app.lpui->crButtonShadow);
            bt = 4;
        }
        if (i == 4) {
            if (iDraw == -1) {
                ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_RACE_FACTORY_GERMANIUM_DISCOUNT), nullptr, 6, yTop, rc.right - 12, static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2, win32::SWP_NOZORDER);
            }
            yTop += static_cast<std::int16_t>((5 * app.state.dyArial8)) / 2;
        }
        if (fMacintosh && ids == idsOneResourceGeneratedEachYearEvery) {
            idsT = idsAnnualResourcesPlanetValueSqrtPopulationEnergy;
        } else {
            idsT = ids;
        }
        ids = static_cast<StringId>(ids + 1);
        cch = TextIO().pfnCchGetString(app, idsT, std::span{app.state.szWork});
        if (iDraw == -1) {
            win32::TextOutA(hdc, 6, yTop, app.state.szWork, cch);
        }
        dx = LOWORD(GetTextExtent(hdc, app.state.szWork, cch)) + 6;
        dxItem = std::abs(rgRW3Width[i]) * dxDig;
        win32::wsprintfA(app.state.szWork, app.state.PCTD, RaceQueryIO().pfnStat(&app.state.vplr, static_cast<RaceStat>(rgRW3IStat[i])));
        if (rgRW3Width[i] < 0 && (i > 0 || !fMacintosh)) {
            dxItem += dxkT;
            if (i == 0) {
                CchAppendUtf8Prefix(std::span{app.state.szWork}, "00");
            } else {
                CchAppendUtf8Prefix(std::span{app.state.szWork}, "kT");
            }
        }
        dx += dxItem;
        if (iDraw == -1 || iDraw == i) {
            RightTextOut(app, hdc, dx, yTop, app.state.szWork, 0, dxItem);
        }
        app.lpui->vrgrcRCW[irc].left = dx + 4;
        app.lpui->vrgrcRCW[irc].top = yTop - 3;
        app.lpui->vrgrcRCW[irc].right = app.lpui->vrgrcRCW[irc].left + 15;
        app.lpui->vrgrcRCW[irc].bottom = (app.state.dyArial8 >> 1) + app.lpui->vrgrcRCW[irc].top + 3;
        app.lpui->vrgrcRCW[irc + 1] = app.lpui->vrgrcRCW[irc];
        win32::OffsetRect(reinterpret_cast<RECT *>(&app.lpui->vrgrcRCW[irc + 1].left), 0, app.lpui->vrgrcRCW[irc].bottom - app.lpui->vrgrcRCW[irc].top - 1);
        if (iDraw == -1) {
            DrawBtn(app, hdc, app.lpui->vrgrcRCW + irc, 0xa0 | bt, 0, nullptr);
            DrawBtn(app, hdc, app.lpui->vrgrcRCW + (irc + 1), 0xa1 | bt, 0, nullptr);
        }
        if (iDraw == -1) {
            if (fMacintosh && ids == idsColonists) {
                idsT = idsMsg0261;
            } else {
                idsT = ids;
            }
            cch = TextIO().pfnCchGetString(app, idsT, std::span{app.state.szWork});
            win32::TextOutA(hdc, app.lpui->vrgrcRCW[irc].right + 4, yTop, app.state.szWork, cch);
        }
        ids = static_cast<StringId>(ids + 1);
        irc += 2;
        yTop += static_cast<std::int16_t>(((rgRW3Spacing[i] + 1) * app.state.dyArial8)) / 2;
    }
    if (fMacintosh) {
        app.lpui->crcRCW = 2;
    } else {
        app.lpui->crcRCW = irc;
    }
    win32::SetBkColor(hdc, crBkSav);
    win32::SetBkMode(hdc, bkMode);
    if (fCreatedDC) {
        win32::ReleaseDC(hwnd, hdc);
    }
    return;
}

std::int16_t FTrackRaceDlg3(APPSESSION &app, HWND hwnd, POINT16 pt, std::int16_t kbd) {
    BTNT    btnt;
    std::int16_t bt;
    std::int16_t dShift;
    std::int16_t i;
    std::int16_t irc;
    std::int16_t iMod;
    std::int16_t iStat;

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
    if (kbd & 4) {
        dShift = 3 * dShift;
    }
    while (FTrackBtn(app, &btnt)) {
        iStat = RaceQueryIO().pfnStat(&app.state.vplr, static_cast<RaceStat>(rgRW3IStat[i]));
        if (SetRaceStat(&app.state.vplr, static_cast<RaceStat>(rgRW3IStat[i]), iStat + dShift) != iStat) {
            DrawRace3(app, hwnd, btnt.hdc, i);
        }
    }
    return 1;
}

INT_PTR  RaceWizardDlg4(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    std::int16_t     i;
    RECT        rc;
    HDC         hdc;
    char        szT[600];
    StringId    ids;
    PAINTSTRUCT ps;
    std::int16_t     cch;
    RECT        rcGBox;

    switch (message) {
    case win32::WM_INITDIALOG:
        SetRCWTitle(app, hwnd, app.lpui->iPanelActive);
        win32::CheckRadioButton(hwnd, IDC_RACE_HYPER_EXPANSION, IDC_RACE_JACK_OF_ALL_TRADES, RaceQueryIO().pfnStat(&app.state.vplr, rsMajorAdv) + 271);
        if (app.state.fRCWReadOnly) {
            for (i = 271; i <= 280; i++) {
                ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, i), 0);
            }
        }
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRaceDlg, 1);
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
        for (i = 271; i <= 280 && reinterpret_cast<HWND>(lParam) != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 280 || message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        DrawRaceAdvantagePoints(app, hdc, &rc, nullptr);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        win32::SetBkColor(hdc, app.lpui->crButtonFace);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_RACE_HYPER_EXPANSION), &rcGBox);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcGBox));
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_RACE_JACK_OF_ALL_TRADES), &rc);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rc.right));
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, app.state.dyArial8 + 2, app.state.dyArial8 >> 1);
        rcGBox.top -= 4;
        _Draw3dFrame(app, hdc, &rcGBox, -1);
        cch = TextIO().pfnCchGetString(app, idsPrimaryRacialTrait, std::span{app.state.szWork});
        win32::TextOutA(hdc, rcGBox.left + 8, rcGBox.top - (app.state.dyArial8 >> 1), app.state.szWork, cch);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        rc.top = rcGBox.bottom + 12;
        rc.left += 12;
        rc.right -= 12;
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_HELP), &rcGBox);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcGBox));
        rc.bottom = rcGBox.top - 6;
        _Draw3dFrame(app, hdc, &rc, -1);
        cch = TextIO().pfnCchGetString(app, idsDescriptionTrait, std::span{app.state.szWork});
        win32::TextOutA(hdc, rc.left + 8, rc.top - (app.state.dyArial8 >> 1), app.state.szWork, cch);
        ids = static_cast<StringId>(idsMustExpandSurviveGivenSmallCheapColony + RaceQueryIO().pfnStat(&app.state.vplr, rsMajorAdv) * 3);
        cch = 0;
        i = 0;
        while (i < 3) {
            cch += TextIO().pfnCchGetString(app, ids, std::span{szT}.subspan(static_cast<std::size_t>(cch)));
            i++;
            ids = static_cast<StringId>(ids + 1);
        }
        ExpandRc(&rc, -app.state.dyArial8 - 2, -(app.state.dyArial8 >> 1));
        rc.top += 4;
        win32::DrawTextA(hdc, szT, cch, &rc, win32::DT_WORDBREAK);
        app.lpui->rcCargo = rc;
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhStep2PrimaryRacialTraits);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != app.lpui->rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRaceDlg, 0);
            ControlIO().pfnEndDialog(hwnd, i);
            return 1;
        }
        if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_RACE_HYPER_EXPANSION && LOWORD(wParam) <= IDC_RACE_JACK_OF_ALL_TRADES) {
            i = LOWORD(wParam) - 271;
            SetRaceStat(&app.state.vplr, rsMajorAdv, i);
            if (RaceQueryIO().pfnStat(&app.state.vplr, rsMajorAdv) == raMacintosh) {
                SetRaceStat(&app.state.vplr, rsFactProd, 10);
                SetRaceStat(&app.state.vplr, rsFactBuild, 10);
                SetRaceStat(&app.state.vplr, rsFactOperate, 10);
                SetRaceStat(&app.state.vplr, rsMineProd, 10);
                SetRaceStat(&app.state.vplr, rsMineBuild, 5);
                SetRaceStat(&app.state.vplr, rsMineOperate, 10);
                SetRaceGrbit(&app.state.vplr, ibitRaceCheapFact, 0);
            }
            InvalidateAdvPtsRect(app, hwnd);
            ControlIO().pfnInvalidateRect(hwnd, &app.lpui->rcCargo, 0);
        }
    }
    return 0;
}

INT_PTR  RaceWizardDlg5(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    std::int16_t     i;
    RECT        rc;
    HWND        hwndCtl;
    HDC         hdc;
    PAINTSTRUCT ps;
    std::int16_t     cch;
    RECT        rcGBox;

    switch (message) {
    case win32::WM_INITDIALOG:
        SetRCWTitle(app, hwnd, app.lpui->iPanelActive);
        app.state.cColDrop = 0;
        for (i = 0; i <= 13; i++) {
            hwndCtl = ControlIO().pfnGetDlgItem(hwnd, i + 291);
            ControlIO().pfnSetWindowText(hwndCtl, TextIO().pfnCompressed(app, static_cast<StringId>(idsImprovedFuelEfficiency + i)));
            ControlIO().pfnSendMessage(hwndCtl, win32::BM_SETCHECK, RaceQueryIO().pfnGrbit(&app.state.vplr, static_cast<RaceGrbit>(i)), 0);
            if (app.state.fRCWReadOnly) {
                ControlIO().pfnEnableWindow(hwndCtl, 0);
            }
        }
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRaceDlg, 1);
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
        for (i = 291; i <= 304 && reinterpret_cast<HWND>(lParam) != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 304 || message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        DrawRaceAdvantagePoints(app, hdc, &rc, nullptr);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        win32::SetBkColor(hdc, app.lpui->crButtonFace);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_RACE_REGENERATING_SHIELDS), &rcGBox);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcGBox.right));
        ControlIO().pfnGetClientRect(hwnd, &rc);
        rc.top = rcGBox.bottom + 12;
        rc.left += 12;
        rc.right -= 12;
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_HELP), &rcGBox);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcGBox));
        rc.bottom = rcGBox.top - 12;
        _Draw3dFrame(app, hdc, &rc, -1);
        app.lpui->rcCargo = rc;
        app.lpui->rcCargo.top -= app.state.dyArial8 >> 1;
        cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsImprovedFuelEfficiency + app.state.cColDrop), std::span{app.state.szWork});
        win32::TextOutA(hdc, rc.left + 8, rc.top - (app.state.dyArial8 >> 1), app.state.szWork, cch);
        cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsGivesFuelMizerGalaxyScoopEnginesIncreases + app.state.cColDrop), std::span{app.state.szWork});
        ExpandRc(&rc, -app.state.dyArial8 - 2, -(app.state.dyArial8 >> 1));
        rc.top += 4;
        win32::DrawTextA(hdc, app.state.szWork, cch, &rc, win32::DT_WORDBREAK);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhStep3LesserTraitsPlayerRace);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != app.lpui->rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRaceDlg, 0);
            ControlIO().pfnEndDialog(hwnd, i);
            return 1;
        }
        if (LOWORD(wParam) >= IDC_RACE_IMPROVED_FUEL_EFFICIENCY && LOWORD(wParam) <= IDC_RACE_REGENERATING_SHIELDS) {
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, LOWORD(wParam)), win32::BM_GETCHECK, 0, 0));
            app.state.cColDrop = LOWORD(wParam) - 291;
            SetRaceGrbit(&app.state.vplr, static_cast<RaceGrbit>(app.state.cColDrop), i);
            InvalidateAdvPtsRect(app, hwnd);
            ControlIO().pfnInvalidateRect(hwnd, &app.lpui->rcCargo, 0);
        }
    }
    return 0;
}

INT_PTR  RaceWizardDlg6(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    std::int16_t     i;
    RECT        rc;
    HDC         hdc;
    PAINTSTRUCT ps;
    std::int16_t     cch;
    RECT        rcGBox;

    switch (message) {
    case win32::WM_INITDIALOG:
        SetRCWTitle(app, hwnd, app.lpui->iPanelActive);
        for (i = 0; i < 6; i++) {
            win32::CheckRadioButton(hwnd, 3 * i + 271, 3 * i + 273, 3 * i + 271 + RaceQueryIO().pfnStat(&app.state.vplr, static_cast<RaceStat>(i + 8)));
        }
        if (app.state.fRCWReadOnly) {
            for (i = 271; i <= 288; i++) {
                ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, i), 0);
            }
        }
        win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsCosts75ExtraResearchFieldsStartTech), (RaceQueryIO().pfnStat(&app.state.vplr, rsMajorAdv) == raNone) + 3);
        ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_RACE_START_HIGHER_TECH), app.state.szWork);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_RACE_START_HIGHER_TECH), win32::BM_SETCHECK, RaceQueryIO().pfnGrbit(&app.state.vplr, ibitRaceTech3), 0);
        if (app.state.fRCWReadOnly) {
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_RACE_START_HIGHER_TECH), 0);
        }
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRaceDlg, 1);
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
        for (i = 271; i <= 288 && reinterpret_cast<HWND>(lParam) != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 288 || reinterpret_cast<HWND>(lParam) == ControlIO().pfnGetDlgItem(hwnd, IDC_RACE_START_HIGHER_TECH) || message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        DrawRaceAdvantagePoints(app, hdc, &rc, nullptr);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        win32::SetBkColor(hdc, app.lpui->crButtonFace);
        for (i = 0; i < 6; i++) {
            win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, 3 * i + 271), &rcGBox);
            win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcGBox));
            win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, 3 * i + 273), &rc);
            win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rc.right));
            rcGBox.right = rc.right;
            rcGBox.bottom = rc.bottom;
            ExpandRc(&rcGBox, app.state.dyArial8, app.state.dyArial8 >> 1);
            _Draw3dFrame(app, hdc, &rcGBox, -1);
            cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsEnergy + i), std::span{app.state.szWork});
            cch += TextIO().pfnCchGetString(app, idsResearch, std::span{app.state.szWork}.subspan(static_cast<std::size_t>(cch)));
            win32::TextOutA(hdc, rcGBox.left + 8, rcGBox.top - (app.state.dyArial8 >> 1), app.state.szWork, cch);
        }
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhStep6ResearchCostsPlayerRace);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != app.lpui->rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRaceDlg, 0);
            ControlIO().pfnEndDialog(hwnd, i);
            return 1;
        }
        if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_RACE_ENERGY_COST_EXTRA && LOWORD(wParam) <= IDC_RACE_BIOTECH_COST_LESS) {
            i = LOWORD(wParam) - 271;
            SetRaceStat(&app.state.vplr, static_cast<RaceStat>(i / 3 + 8), i % 3);
            InvalidateAdvPtsRect(app, hwnd);
        } else if (LOWORD(wParam) == IDC_RACE_START_HIGHER_TECH) {
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, LOWORD(wParam)), win32::BM_GETCHECK, 0, 0));
            SetRaceGrbit(&app.state.vplr, ibitRaceTech3, i);
            InvalidateAdvPtsRect(app, hwnd);
        }
    }
    return 0;
}

void InvalidateAdvPtsRect(APPSESSION &app, HWND hwnd) {
    NATIVEOWNER<LOGFONT, win32::LocalFree> plfOwner;
    HDC        hdc;
    TEXTMETRIC tm;
    LOGFONT   *plf;
    std::int16_t    dyBig;
    HFONT      hfont;
    std::int16_t    dx;
    RECT       rc;
    HFONT      hfontSav;

    plf = static_cast<LOGFONT *>(win32::LocalAlloc(64, sizeof(LOGFONT)));
    plfOwner.reset(plf);
    hdc = win32::GetDC(hwnd);
    plf->lfHeight = -24;
    CchCopyUtf8Prefix(std::span{plf->lfFaceName}, app.lpui->rgszArial[1]);
    hfont = win32::CreateFontIndirectA(plf);
    hfontSav = static_cast<HFONT>(win32::SelectObject(hdc, hfont));
    win32::GetTextMetricsA(hdc, &tm);
    dyBig = tm.tmHeight + tm.tmExternalLeading;
    dx = LOWORD(GetTextExtent(hdc, "-99999", 6)) + 8;
    ControlIO().pfnGetClientRect(hwnd, &rc);
    rc.left = rc.right - dx;
    rc.bottom = rc.top + dyBig + 4;
    win32::SelectObject(hdc, hfontSav);
    win32::DeleteObject(hfont);
    plfOwner.reset();
    win32::ReleaseDC(hwnd, hdc);
    ControlIO().pfnInvalidateRect(hwnd, &rc, 1);
    return;
}

void DrawRaceAdvantagePoints(APPSESSION &app, HDC hdc, RECT *prc, PLAYER *pplr) {
    NATIVEOWNER<LOGFONT, win32::LocalFree> plfOwner;
    TEXTMETRIC tm;
    LOGFONT   *plf;
    COLORREF   crBkSav;
    std::int16_t    bkMode;
    std::int16_t    dyBig;
    char       szAdvantage[32];
    std::int16_t    c;
    COLORREF   crSav;
    HFONT      hfont;
    std::int16_t    dx;
    std::int16_t    iPts;
    std::int16_t    cch;
    RECT       rc;
    HFONT      hfontSav;

    plf = static_cast<LOGFONT *>(win32::LocalAlloc(64, sizeof(LOGFONT)));
    plfOwner.reset(plf);
    plf->lfHeight = -24;
    TextIO().pfnCchGetString(app, idsArialBold, std::span{plf->lfFaceName});
    hfont = win32::CreateFontIndirectA(plf);
    hfontSav = static_cast<HFONT>(win32::SelectObject(hdc, hfont));
    win32::GetTextMetricsA(hdc, &tm);
    dyBig = tm.tmHeight + tm.tmExternalLeading;
    dx = LOWORD(GetTextExtent(hdc, "-99999", 6)) + 8;
    rc = *prc;
    rc.left = rc.right - dx;
    rc.bottom = rc.top + dyBig + 4;
    if (!pplr) {
        pplr = &app.state.vplr;
    }
    iPts = CAdvantagePoints(app, pplr);
    bkMode = win32::SetBkMode(hdc, win32::OPAQUE);
    crSav = win32::SetTextColor(hdc, iPts < 0 ? 127 : 0);
    crBkSav = win32::SetBkColor(hdc, app.lpui->crButtonFace);
    c = win32::wsprintfA(app.state.szWork, app.state.PCTD, iPts);
    RcCtrTextOut(app, hdc, &rc, app.state.szWork, -1);
    win32::SetTextColor(hdc, 0);
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    c = TextIO().pfnCchGetString(app, idsPointsLeft, std::span{app.state.szWork});
    dx = LOWORD(GetTextExtent(hdc, app.state.szWork, c));
    cch = TextIO().pfnCchGetString(app, idsAdvantage, std::span{szAdvantage});
    RightTextOut(app, hdc, rc.left, 3, szAdvantage, cch, 0);
    RightTextOut(app, hdc, rc.left, app.state.dyArial8 + 3, app.state.szWork, c, 0);
    rc.left -= dx + 4;
    win32::PatBlt(hdc, rc.left - 1, 0, 1, rc.bottom + 1, win32::BLACKNESS);
    win32::PatBlt(hdc, rc.left - 4, 0, 2, rc.bottom + 4, win32::BLACKNESS);
    win32::PatBlt(hdc, rc.left, rc.bottom, rc.right - rc.left, 1, win32::BLACKNESS);
    win32::PatBlt(hdc, rc.left - 2, rc.bottom + 2, rc.right - rc.left + 2, 2, win32::BLACKNESS);
    win32::SetBkColor(hdc, crBkSav);
    win32::SetTextColor(hdc, crSav);
    win32::SetBkMode(hdc, bkMode);
    win32::SelectObject(hdc, hfontSav);
    win32::DeleteObject(hfont);
    plfOwner.reset();
    return;
}

static char szRaceFileSuggest[256];

void SetSzRaceFile(APPSESSION &app, const char *pszFile) {
    std::size_t cch = CchUtf8Prefix(pszFile, sizeof(app.state.szRaceFile) - 1);
    std::size_t cchSuggest = CchUtf8Prefix(pszFile, sizeof(szRaceFileSuggest) - 1);

    /* Full native paths belong to UI storage; the original cache keeps its size. */
    std::memcpy(szRaceFileSuggest, pszFile, cchSuggest);
    szRaceFileSuggest[cchSuggest] = 0;
    std::memcpy(app.state.szRaceFile, pszFile, cch);
    app.state.szRaceFile[cch] = 0;
}

std::int16_t FConfirmLegacyRaceNames(APPSESSION &app, char *szName, char *szNames) {
    char szPrompt[256];

    CchSprintf(szPrompt, TextIO().pfnCompressed(app, idsNativeLegacyRaceNameShortened), szName, szNames);
    return AlertIO().pfnAlertSz(app, szPrompt, win32::MB_YESNO | win32::MB_ICONQUESTION | win32::MB_TASKMODAL) == win32::IDYES;
}

std::int16_t FSaveRace(APPSESSION &app, char *szFileSuggest, PLAYER *pplr) {
    std::uint16_t     icksum;
    char         szFileTitle[256];
    char         szDirName[256];
    char         szFilter[256];
    std::uint16_t     i;
    char         szFile[256];
    OPENFILENAME ofn;

    if (szFileSuggest == app.state.szRaceFile && app.state.szRaceFile[0] && szRaceFileSuggest[0] &&
        std::strncmp(app.state.szRaceFile, szRaceFileSuggest, std::strlen(app.state.szRaceFile)) == 0) szFileSuggest = szRaceFileSuggest;
    if (szFileSuggest) {
        std::strcpy(szFile, szFileSuggest);
    } else {
        szFile[0] = 0;
    }
    szDirName[0] = 0;
    TextIO().pfnCchGetString(app, idsStarsRaceFilesR, std::span{szFilter});
    for (i = 0; szFilter[i] != 0; i++) {
        if (szFilter[i] == '|') {
            szFilter[i] = 0;
        }
    }
    ofn = {};
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = app.lpui->hwndRaceParent;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = 0x100;
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = 0x100;
    ofn.lpstrInitialDir = szDirName;
    ofn.lpstrDefExt = "r1";
    ofn.Flags = win32::OFN_OVERWRITEPROMPT | win32::OFN_HIDEREADONLY | win32::OFN_PATHMUSTEXIST | win32::OFN_NOREADONLYRETURN;
    if (DesktopChoiceIO().pfnFNativeSaveFileName(&ofn) != 0) {
        if (!RecordIO().pfnFCreateFile(app, dtRace, iplrNone, szFile)) {
            AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsStarsUnableSaveRaceDataFilePlease, nullptr), win32::MB_ICONHAND);
            return 0;
        }
        WriteRtPlr(app, pplr, nullptr);
        icksum = IRaceChecksum(pplr);
        WriteRtEof(app, icksum);
        RecordIO().pfnStreamClose(app);
        SetSzRaceFile(app, &szFile[ofn.nFileOffset]);
        return 1;
    }
    return 0;
}

void SetRCWTitle(APPSESSION &app, HWND hwnd, std::int16_t iStep) {
    char    szBuf[50];
    std::int16_t cch;

    cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsCustomRaceWizardStepD6 + app.state.fRCWReadOnly), std::span{szBuf});
    cch = win32::wsprintfA(app.state.szWork, szBuf, iStep);
    ControlIO().pfnSetWindowText(hwnd, app.state.szWork);
    return;
}

void ResetRaceUiSession() noexcept { szRaceFileSuggest[0] = 0; }
