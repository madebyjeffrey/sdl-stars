#include "win.h"
#include <stars_compat.h>

int16_t RaceCreationWizard(HWND hwndParent, int16_t fReadOnly, int16_t fDontWrite) {
    WizardButton mdRet;
    FARPROC      lpProc;
    RECT         rgrcStack[17];
    int16_t      cpts;
    int16_t      fResult;
    BOOL         fSequence;

    vrgrcRCW = rgrcStack;
    fRCWReadOnly = fReadOnly;
    hwndRaceParent = hwndParent;
    fSequence = StarsCompatBeginDialogSequence(hwndParent);
Step1:
    iPanelActive = rwPageRace;
    lpProc = MakeProcInstance(RaceWizardDlg1, hInst);
    mdRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_RACE_WIZARD_1), hwndRaceParent, lpProc);
    FreeProcInstance(lpProc);
    if (mdRet == wizCancel || mdRet == (WizardButton)-1)
        goto LCancel;
    if (mdRet == wizFinish)
        goto Finish;
Step2:
    iPanelActive = rwPagePrimaryTrait;
    lpProc = MakeProcInstance(RaceWizardDlg4, hInst);
    mdRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_RACE_WIZARD_4), hwndRaceParent, lpProc);
    FreeProcInstance(lpProc);
    if (mdRet == wizCancel || mdRet == (WizardButton)-1)
        goto LCancel;
    if (mdRet == wizBack)
        goto Step1;
    if (mdRet == wizFinish)
        goto Finish;
Step3:
    iPanelActive = rwPageLesserTraits;
    lpProc = MakeProcInstance(RaceWizardDlg5, hInst);
    mdRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_RACE_WIZARD_5), hwndRaceParent, lpProc);
    FreeProcInstance(lpProc);
    if (mdRet == wizCancel || mdRet == (WizardButton)-1)
        goto LCancel;
    if (mdRet == wizBack)
        goto Step2;
    if (mdRet == wizFinish)
        goto Finish;
Step4:
    iPanelActive = rwPageHabitability;
    lpProc = MakeProcInstance(RaceWizardDlg2, hInst);
    mdRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_RACE_WIZARD_2), hwndRaceParent, lpProc);
    FreeProcInstance(lpProc);
    if (mdRet == wizCancel || mdRet == (WizardButton)-1)
        goto LCancel;
    if (mdRet == wizBack)
        goto Step3;
    if (mdRet == wizFinish)
        goto Finish;
Step5:
    iPanelActive = rwPageEconomy;
    lpProc = MakeProcInstance(RaceWizardDlg3, hInst);
    mdRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_RACE_WIZARD_3), hwndRaceParent, lpProc);
    FreeProcInstance(lpProc);
    if (mdRet == wizCancel || mdRet == (WizardButton)-1)
        goto LCancel;
    if (mdRet == wizBack)
        goto Step4;
    if (mdRet == wizFinish)
        goto Finish;
Step6:
    iPanelActive = rwPageResearch;
    lpProc = MakeProcInstance(RaceWizardDlg6, hInst);
    mdRet = DialogBox(hInst, MAKEINTRESOURCE(IDD_RACE_WIZARD_6), hwndRaceParent, lpProc);
    FreeProcInstance(lpProc);
    if (mdRet == wizCancel || mdRet == (WizardButton)-1)
        goto LCancel;
    if (mdRet == wizBack)
        goto Step5;
    if (mdRet == wizFinish)
        goto Finish;
Finish:
    if (fRCWReadOnly) {
        goto LCancel;
    }
    cpts = CAdvantagePoints(&vplr);
    if (cpts < 0) {
        wsprintf(szWork, PszGetCompressedString(idsAdvantagePointsCurrentlyHoleDPointsCannot), -cpts);
        AlertSz(szWork, MB_ICONHAND);
        switch (iPanelActive) {
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
    lSaltCur = LSaltFromSz(szRacePass);
    lSaltLast = -5;
    if (!FCheckPassword())
        goto Step1;
    if (!fDontWrite) {
        if (FHasCurrentLibraryRace()) {
            if (!FSaveCurrentLibraryRace(hwndRaceParent, &vplr)) goto Step1;
        } else if (!FSaveRace((char *)(szRaceFile[0] == 0 ? "stars.r1" : szRaceFile), &vplr)) goto Step1;
    }
    if (fDontWrite) {
        fResult = TRUE;
        goto LDone;
    }
    fResult = TRUE;
    goto LDone;
LCancel:
    fResult = FALSE;
LDone:
    if (fSequence) StarsCompatEndDialogSequence();
    return fResult;
}

INT_PTR CALLBACK RaceWizardDlg1(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int16_t     i;
    RECT        rc;
    int16_t     iPlrBmp;
    HWND        hwndCB;
    char       *psz;
    POINT16     pt;
    HDC         hdc;
    BTNT        btnt;
    int16_t     bt;
    RECT       *prc;
    int16_t     iDir;
    int16_t     iCur;
    int16_t     iOffset;
    PLAYER     *pplr;
    PAINTSTRUCT ps;
    int16_t     j;
    int16_t     cch;
    RECT        rcGBox;
    int16_t     k;
    char        szBuf[32];

    switch (message) {
    case WM_INITDIALOG:
        iPlrBmp = vplr.iPlrBmp;
        SetRCWTitle(hwnd, iPanelActive);
        SetDlgItemText(hwnd, IDC_RACE_NAME, vplr.szName);
        SetDlgItemText(hwnd, IDC_RACE_PLURAL_NAME, vplr.szNames);
        if (vplr.szName[0] == 0) {
            GetDlgItemText(hwnd, IDC_RACE_HUMANOID, vplr.szName, 16);
            SetDlgItemText(hwnd, IDC_RACE_NAME, vplr.szName);
        }
        StickyDlgPos(hwnd, &ptStickyRaceDlg, TRUE);
        if (game.fTutorial && idPlayer == 0 && fRCWReadOnly) {
            i = 0;
        } else {
            for (i = 0; i < 7; i++) {
                vplr.iPlrBmp = vrgplrDef[i].iPlrBmp;
                if (memcmp(&vplr, &vrgplrDef[i], 128) == 0)
                    break;
            }
            vplr.iPlrBmp = iPlrBmp;
        }
        CheckRadioButton(hwnd, IDC_RACE_HUMANOID, IDC_RACE_CUSTOM, i + 271);
        SendDlgItemMessage(hwnd, IDC_RACE_NAME, EM_LIMITTEXT, 0xf, 0);
        SendDlgItemMessage(hwnd, IDC_RACE_PLURAL_NAME, EM_LIMITTEXT, 0xf, 0);
        SendDlgItemMessage(hwnd, IDC_RACE_PASSWORD, EM_LIMITTEXT, 0xf, 0);
        hwndCB = GetDlgItem(hwnd, IDC_COMBOBOX);
        for (i = idsSurfaceMinerals; i <= idsDefenses3; i++) {
            psz = PszGetCompressedString(i);
            SendMessage(hwndCB, CB_ADDSTRING, 0, (LPARAM)psz);
        }
        i = GetRaceStat(&vplr, rsUseLeftover);
        SendMessage(hwndCB, CB_SETCURSEL, i, 0);
        if (vplr.lSalt != 0) {
            FSetUtf8WindowText(GetDlgItem(hwnd, IDC_RACE_PASSWORD), szRacePass);
        }
        if (fRCWReadOnly) {
            for (i = 268; i <= 269; i++) {
                EnableWindow(GetDlgItem(hwnd, i), FALSE);
            }
            EnableWindow(GetDlgItem(hwnd, IDC_RACE_PLURAL_NAME), FALSE);
            for (i = 271; i <= 278; i++) {
                EnableWindow(GetDlgItem(hwnd, i), FALSE);
            }
            EnableWindow(hwndCB, FALSE);
        }
        return 1;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        if (fRCWReadOnly || (PtInRect(rgrcBuildSpin, PointFrom16(pt)) == 0 && PtInRect(&rgrcBuildSpin[1], PointFrom16(pt)) == 0))
            break;
        if (PtInRect(rgrcBuildSpin, PointFrom16(pt)) != 0) {
            iDir = -1;
            bt = 34;
            prc = rgrcBuildSpin;
        } else {
            iDir = 1;
            bt = 35;
            prc = &rgrcBuildSpin[1];
        }
        iCur = vplr.iPlrBmp;
        if (iCur >= 32) {
            iCur = 0;
        }
        hdc = GetDC(hwnd);
        SelectPalette(hdc, vhpal, FALSE);
        RealizePalette(hdc);
        InitBtnTrack(&btnt, hwnd, NULL, prc, bt, 80, FALSE, FALSE, NULL);
        while (FTrackBtn(&btnt)) {
            iCur = (int16_t)(iCur + 32 + iDir) % 32;
            DibBlt(hdc, rgrcBuildSpin[0].left - 2, rgrcBuildSpin[0].top - 35, 32, 32, hdibRaces, (iCur & 7) * 0x20, (3 - (iCur >> 3)) * 0x20, 32, 32, 13369376);
        }
        vplr.iPlrBmp = iCur;
        ReleaseDC(hwnd, hdc);
        return 1;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        for (i = 271; i <= 278 && (HWND)lParam != GetDlgItem(hwnd, i); i++) {
        }
        if (i <= 278 || message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        for (j = 271; j <= 278 && IsDlgButtonChecked(hwnd, j) == 0; j++) {
        }
        if (j <= 277) {
            k = j - 271;
            pplr = &vrgplrDef[k];
        } else {
            pplr = &vplr;
        }
        GetClientRect(hwnd, &rc);
        DrawRaceAdvantagePoints(hdc, &rc, pplr);
        GetWindowRect(GetDlgItem(hwnd, IDC_RACE_HUMANOID), &rcGBox);
        ScreenToClient(hwnd, (POINT *)&rcGBox);
        GetWindowRect(GetDlgItem(hwnd, IDC_RACE_CUSTOM), &rc);
        ScreenToClient(hwnd, (POINT *)&rc.right);
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        _Draw3dFrame(hdc, &rcGBox, -1);
        SelectObject(hdc, rghfontArial8[1]);
        SetBkColor(hdc, crButtonFace);
        cch = CchGetString(idsPredefinedRaces, szWork);
        TextOut(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, cch);
        GetWindowRect(GetDlgItem(hwnd, IDC_COMBOBOX), &rcGBox);
        ScreenToClient(hwnd, (POINT *)&rcGBox.right);
        pt.x = rcGBox.right + 32;
        pt.y = rcGBox.bottom - 32;
        iOffset = vplr.iPlrBmp;
        if (iOffset >= 32) {
            iOffset = 0;
        }
        SelectPalette(hdc, vhpal, FALSE);
        RealizePalette(hdc);
        DibBlt(hdc, pt.x, pt.y, 32, 32, hdibRaces, (iOffset & 7) * 0x20, (3 - (iOffset >> 3)) * 0x20, 32, 32, 13369376);
        if (!fRCWReadOnly) {
            SetRect(rgrcBuildSpin, pt.x + 2, pt.y + 35, pt.x + 16, pt.y + 49);
            rgrcBuildSpin[1] = rgrcBuildSpin[0];
            OffsetRect(&rgrcBuildSpin[1], 14, 0);
            for (i = 0; i < 2; i++) {
                DrawBtn(hdc, &rgrcBuildSpin[i], (i == 0 ? 2 : 3) | 0x20, FALSE, NULL);
            }
        }
        EndPaint(hwnd, &ps);
        return 1;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhStep1BasicDefinition);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            if (LOWORD(wParam) != IDCANCEL) {
                iPlrBmp = vplr.iPlrBmp;
                for (j = 271; j <= 278 && IsDlgButtonChecked(hwnd, j) == 0; j++) {
                }
                if (j <= 277) {
                    k = j - 271;
                    vplr = vrgplrDef[k];
                } else {
                    /* A custom race isn't random. The original kept the
                       random-race bit of a race that had been Random, and
                       GenerateWorld replaced the customized race. */
                    SetRaceGrbit(&vplr, ibitRaceAIPlayer, FALSE);
                }
                GetDlgItemText(hwnd, IDC_RACE_NAME, vplr.szName, 32);
                GetDlgItemText(hwnd, IDC_RACE_PLURAL_NAME, vplr.szNames, 32);
                GetRaceStat(&vplr, rsUseLeftover);
                if (CchGetUtf8WindowText(GetDlgItem(hwnd, IDC_RACE_PASSWORD), szRacePass, 16) < 0) {
                    AlertSz(PszGetCompressedString(idsNativePasswordReenter), MB_ICONHAND);
                    return TRUE;
                }
                j = LOWORD(SendMessage(GetDlgItem(hwnd, IDC_COMBOBOX), CB_GETCURSEL, 0, 0));
                SetRaceStat(&vplr, rsUseLeftover, j);
                vplr.lSalt = LSaltFromSz(szRacePass);
                vplr.iPlrBmp = iPlrBmp;
            }
            StickyDlgPos(hwnd, &ptStickyRaceDlg, FALSE);
            EndDialog(hwnd, i);
            return 1;
        }
        if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_RACE_HUMANOID && LOWORD(wParam) <= IDC_RACE_CUSTOM) {
            memset(vplr.szName, 0, 32);
            GetDlgItemText(hwnd, IDC_RACE_NAME, vplr.szName, 32);
            memset(vplr.szNames, 0, 32);
            GetDlgItemText(hwnd, IDC_RACE_PLURAL_NAME, vplr.szNames, 32);
            GetDlgItemText(hwnd, LOWORD(wParam), szBuf, 32);
            for (i = 0; i < 7 && strcmp(vplr.szName, PszGetCompressedString(idsHumanoid + i)) != 0; i++) {
            }
            if (i < 7 && LOWORD(wParam) < IDC_RACE_CUSTOM) {
                memset(vplr.szName, 0, 32);
                CchGetString(idsHumanoid + LOWORD(wParam) - IDC_RACE_HUMANOID, vplr.szName);
                SetDlgItemText(hwnd, IDC_RACE_NAME, vplr.szName);
                memset(vplr.szNames, 0, 32);
                psz = PszPlayerName(0, TRUE, TRUE, FALSE, 0, &vplr);
                strcpy(vplr.szNames, psz);
                SetDlgItemText(hwnd, IDC_RACE_PLURAL_NAME, vplr.szNames);
            }
            if (LOWORD(wParam) <= IDC_RACE_RANDOM) {
                pplr = &vrgplrDef[LOWORD(wParam) - 271];
            } else {
                pplr = &vplr;
            }
            InvalidateAdvPtsRect(hwnd);
            i = GetRaceStat(pplr, rsUseLeftover);
            SendMessage(GetDlgItem(hwnd, IDC_COMBOBOX), CB_SETCURSEL, i, 0);
            EnableWindow(GetDlgItem(hwnd, IDC_NEXT), LOWORD(wParam) != IDC_RACE_RANDOM);
            vplr.iPlrBmp = pplr->iPlrBmp;
            GetWindowRect(GetDlgItem(hwnd, IDC_COMBOBOX), &rc);
            ScreenToClient(hwnd, (POINT *)&rc.right);
            rc.left = rc.right + 32;
            rc.top = rc.bottom - 32;
            rc.right = rc.left + 32;
            rc.bottom = rc.top + 32;
            InvalidateRect(hwnd, &rc, TRUE);
        }
    }
    return 0;
}

INT_PTR CALLBACK RaceWizardDlg2(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int16_t     i;
    RECT        rc;
    HDC         hdc;
    int16_t     yTop;
    int16_t     dy;
    int16_t     dxMiddle;
    int16_t     dxLabel;
    int16_t     cch;
    char        szTemp[20];
    HFONT       hfontSav;
    PAINTSTRUCT ps;
    POINT16     pt;
    int16_t     iVar;

    switch (message) {
    case WM_INITDIALOG:
        SetRCWTitle(hwnd, iPanelActive);
        viStore = -1;
        hdc = GetDC(hwnd);
        GetClientRect(hwnd, &rc);
        hfontSav = (HFONT)SelectObject(hdc, rghfontArial8[1]);
        cch = CchGetString(idsTemperature, szTemp);
        dxLabel = LOWORD(GetTextExtent(hdc, szTemp, cch)) + 10;
        dxMiddle = LOWORD(GetTextExtent(hdc, "200mR", 5)) + 10;
        dxMiddle = rc.right - dxLabel - dxMiddle;
        dy = (int16_t)(3 * dyArial8) / 2;
        yTop = 3 * dyArial8;
        SetRect(vrgrcRCW, dxLabel, yTop, dxLabel + dy, yTop + dy);
        SetRect(vrgrcRCW + 1, dxLabel + dy + 6, yTop, dxLabel + dxMiddle - dy - 6, yTop + dy);
        SetRect(vrgrcRCW + 2, dxLabel + dxMiddle - dy, yTop, dxLabel + dxMiddle, yTop + dy);
        SetRect(vrgrcRCW + 3, dxLabel, yTop + dy + 4, 3 * dy + dxLabel, dy * 2 + yTop + 4);
        SetRect(vrgrcRCW + 4, dxLabel + dxMiddle - 3 * dy, yTop + dy + 4, dxLabel + dxMiddle, dy * 2 + yTop + 4);
        for (i = 0; i < 5; i++) {
            vrgrcRCW[i + 5] = vrgrcRCW[i];
            OffsetRect((RECT *)&vrgrcRCW[i + 5].left, 0, 3 * dy + dyArial8 / 2);
            vrgrcRCW[i + 10] = vrgrcRCW[i];
            OffsetRect((RECT *)&vrgrcRCW[i + 0xa].left, 0, 6 * dy + dyArial8);
        }
        for (i = 0; i < 3; i++) {
            SetWindowPos(GetDlgItem(hwnd, i + 291), NULL, 3 * dy + dxLabel + 6, vrgrcRCW[5 * i + 3].top, dxMiddle - 6 * dy - 12, dy, SWP_NOZORDER);
            CheckDlgButton(hwnd, i + 291, vplr.rgEnvVarMax[i] < 0);
        }
        cch = CchGetString(idsMaximumColonistGrowthRatePerYear, szWork);
        vrgrcRCW[15].left = dxLabel + LOWORD(GetTextExtent(hdc, szWork, cch)) + LOWORD(GetTextExtent(hdc, "15%", 3)) + 4;
        vrgrcRCW[15].top = 9 * dy + yTop - 3 + 3 * (dyArial8 / 2);
        vrgrcRCW[15].right = vrgrcRCW[15].left + 15;
        vrgrcRCW[15].bottom = (dyArial8 >> 1) + vrgrcRCW[15].top + 3;
        vrgrcRCW[16] = vrgrcRCW[15];
        OffsetRect(vrgrcRCW + 16, 0, vrgrcRCW[15].bottom - vrgrcRCW[15].top - 1);
        crcRCW = 17;
        SelectObject(hdc, hfontSav);
        ReleaseDC(hwnd, hdc);
        if (fRCWReadOnly) {
            for (i = 291; i <= 293; i++) {
                EnableWindow(GetDlgItem(hwnd, i), FALSE);
            }
        }
        StickyDlgPos(hwnd, &ptStickyRaceDlg, TRUE);
        return 1;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        for (i = 291; i <= 293 && (HWND)lParam != GetDlgItem(hwnd, i); i++) {
        }
        if (i <= 293 || message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        viStore = -2;
        DrawRace2(hwnd, hdc, -1);
        EndPaint(hwnd, &ps);
        return 1;
    case WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (IrcRaceDlgHitTest(pt) < 0)
            break;
        SetCursor(hcurHand);
        return 1;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        return FTrackRaceDlg2(hwnd, pt, wParam);
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhStep4PopulationGrowthFactors);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            StickyDlgPos(hwnd, &ptStickyRaceDlg, FALSE);
            EndDialog(hwnd, i);
            return 1;
        }
        if (LOWORD(wParam) >= IDC_IMMUNE_TO_GRAVITY && LOWORD(wParam) <= IDC_IMMUNE_TO_RADIATION) {
            i = LOWORD(SendMessage(GetDlgItem(hwnd, LOWORD(wParam)), BM_GETCHECK, 0, 0));
            iVar = LOWORD(wParam) - 291;
            if (i == 1) {
                vplr.rgEnvVar[iVar] = envImmune;
                vplr.rgEnvVarMax[iVar] = envImmune;
                vplr.rgEnvVarMin[iVar] = envImmune;
            } else {
                vplr.rgEnvVarMin[iVar] = 20;
                vplr.rgEnvVarMax[iVar] = 80;
                vplr.rgEnvVar[iVar] = 50;
            }
            DrawRace2(hwnd, NULL, 1 << iVar | 0xff00);
        }
    }
    return 0;
}

void DrawRace2(HWND hwnd, HDC hdc, int16_t iDraw) {
    int16_t iPit;
    int16_t bt;
    int16_t iMax;
    char    szT[32];
    int16_t dy;
    int16_t iMin;
    int16_t bkMode;
    int16_t fCreatedDC;
    int16_t xRLabel;
    int16_t i;
    int16_t iMod;
    char   *psz;
    int16_t dx;
    int16_t cch;
    int16_t bt1;
    RECT    rc;
    int32_t l2;
    int16_t iStore;
    int32_t l;

    fCreatedDC = FALSE;
    bt1 = !fRCWReadOnly ? 0 : 4;
    if (!hdc) {
        fCreatedDC = TRUE;
        hdc = GetDC(hwnd);
    }
    GetClientRect(hwnd, &rc);
    DrawRaceAdvantagePoints(hdc, &rc, NULL);
    bkMode = SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, rghfontArial8[1]);
    xRLabel = (int16_t)(rc.right - vrgrcRCW[2].right) / 2 + vrgrcRCW[2].right;
    for (i = 0; i < 3; i++) {
        if (1 << i & iDraw) {
            if (iDraw != -1) {
                SelectObject(hdc, hbrButtonFace);
                PatBlt(hdc, vrgrcRCW[2].right, vrgrcRCW[5 * i + 2].top, rc.right - vrgrcRCW[2].right, vrgrcRCW[5 * i + 4].bottom - vrgrcRCW[5 * i + 2].top,
                       PATCOPY);
            }
            if (vplr.rgEnvVarMax[i] < 0) {
                cch = CchGetString(idsN2, szT);
                CtrTextOut(hdc, xRLabel, vrgrcRCW[5 * i + 2].top + 1 + dyArial8, szT, cch);
            } else {
                psz = PszCalcEnvVar(i, vplr.rgEnvVarMin[i]);
                CtrTextOut(hdc, xRLabel, vrgrcRCW[5 * i + 2].top + 1, psz, 0);
                cch = CchGetString(idsTo2, szT);
                CtrTextOut(hdc, xRLabel, vrgrcRCW[5 * i + 2].top + 1 + dyArial8, szT, cch);
                psz = PszCalcEnvVar(i, vplr.rgEnvVarMax[i]);
                CtrTextOut(hdc, xRLabel, vrgrcRCW[5 * i + 2].top + 1 + dyArial8 * 2, psz, 0);
            }
            if (iDraw == -1) {
                RightTextOut(hdc, vrgrcRCW[5 * i].left - 4, dyArial8 / 4 + vrgrcRCW[5 * i].top, rgszPlanetAttr[i], 0, 0);
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
        bt = vplr.rgEnvVarMax[i / 5] >= 0 ? 0 : 4;
        switch (iMod) {
        case 0:
        case 2:
            if (!(iDraw & 0xfff0))
                break;
            DrawBtn(hdc, vrgrcRCW + i, (iMod == 0 ? 2 : 3) | bt | bt1, FALSE, NULL);
            break;
        default:
            if (!(iDraw & 0xfff0))
                break;
            DrawBtn(hdc, vrgrcRCW + i, 8 | bt | bt1, FALSE, (char *)(iMod == 3 ? "<<     >>" : ">>     <<"));
            break;
        case 1:
            if (1 << iPit & iDraw) {
                PatBlt(hdc, vrgrcRCW[i].left, vrgrcRCW[i].top, vrgrcRCW[i].right - vrgrcRCW[i].left, vrgrcRCW[i].bottom - vrgrcRCW[i].top, BLACKNESS);
                if (bt == 0) {
                    iMin = vplr.rgEnvVarMin[iPit];
                    iMax = vplr.rgEnvVarMax[iPit];
                    dx = vrgrcRCW[i].right - vrgrcRCW[i].left - 2;
                    dy = vrgrcRCW[i].bottom - vrgrcRCW[i].top - 2;
                    SelectObject(hdc, rghbrPlanetAttr[iPit][0]);
                    PatBlt(hdc, MulDiv(iMin, dx, 100) + vrgrcRCW[i].left + 1, vrgrcRCW[i].top + 1, MulDiv(iMax - iMin, dx, 100), dy, PATCOPY);
                }
            }
            iPit++;
        }
        i++;
        iMod++;
    }
    if (iDraw & 8) {
        if (iDraw == -1) {
            cch = CchGetString(idsMaximumColonistGrowthRatePerYear, szWork);
            TextOut(hdc, vrgrcRCW->left, vrgrcRCW[15].top + 3, szWork, cch);
            DrawBtn(hdc, vrgrcRCW + 15, 0xa0 | bt1, FALSE, NULL);
            DrawBtn(hdc, vrgrcRCW + 16, 0xa1 | bt1, FALSE, NULL);
        } else {
            dx = LOWORD(GetTextExtent(hdc, "15%", 3));
            SelectObject(hdc, hbrButtonFace);
            PatBlt(hdc, vrgrcRCW[15].left - 4 - dx, vrgrcRCW[15].top + 3, dx, dyArial8, PATCOPY);
        }
        cch = wsprintf(szWork, PCTDPCTPCT, vplr.pctIdealGrowth);
        RightTextOut(hdc, vrgrcRCW[15].left - 4, vrgrcRCW[15].top + 3, szWork, cch, 0);
    }
    if (iDraw & 7) {
        l = 1;
        for (i = 0; i < 3; i++) {
            if (vplr.rgEnvVarMax[i] < 0 || vplr.rgEnvVarMax[i] - vplr.rgEnvVarMin[i] == 100) {
                l = (uint32_t)(l * 100);
            } else if (i == 2) {
                l = (uint32_t)(l * (int16_t)(vplr.rgEnvVarMax[i] - vplr.rgEnvVarMin[i]));
            } else {
                l2 = 0;
                for (iStore = vplr.rgEnvVarMin[i]; iStore <= vplr.rgEnvVarMax[i]; iStore++) {
                    if (iStore < 10) {
                        l2 += iStore;
                    } else if (iStore < 90) {
                        l2 += 10;
                    } else {
                        l2 += (int16_t)(100 - iStore);
                    }
                }
                l = (int32_t)(l * l2) / 9;
            }
        }
        if (l < 1) {
            l = 1;
        }
        l2 = (int32_t)(((int32_t)(l >> 1) + 0xf4240) / l);
        iStore = LOWORD(l2);
        if (l == 1000000) {
            iStore = 0;
        }
        if (iStore != viStore) {
            viStore = iStore;
            if (l2 == 1) {
                cch = CchGetString(l == 1000000 ? idsPlanetsWillHabitableRace : idsVirtuallyPlanetsWillHabitableRace, szWork);
            } else {
                // The explanation exceeds szT's 32 bytes; format from the text
                // table instead of corrupting the habitat page's stack.
                cch = wsprintf(szWork, PszGetCompressedString(idsCanExpect1DPlanetsWillHabitable), l2);
            }
            rc.left = vrgrcRCW->left;
            rc.top = vrgrcRCW[15].top + dyArial8 + 8;
            rc.right = vrgrcRCW[15].right + 20;
            rc.bottom = 3 * dyArial8 + rc.top;
            SelectObject(hdc, hbrButtonFace);
            PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY);
            DrawText(hdc, szWork, cch, &rc, DT_WORDBREAK);
        }
    }
    SetBkMode(hdc, bkMode);
    if (fCreatedDC) {
        ReleaseDC(hwnd, hdc);
    }
    return;
}

int16_t IrcRaceDlgHitTest(POINT16 pt) {
    int16_t i;

    if (fRCWReadOnly) {
        return -1;
    }
    for (i = 0; i < crcRCW && PtInRect((RECT *)&vrgrcRCW[i].left, PointFrom16(pt)) == 0; i++) {
    }
    if (i < crcRCW) {
        if (iPanelActive == rwPageHabitability && vplr.rgEnvVarMax[i / 5] < 0) {
            return -1;
        }
        return i;
    }
    return -1;
}

int16_t FTrackRaceDlg2(HWND hwnd, POINT16 pt, int16_t kbd) {
    BTNT    btnt;
    int16_t bt;
    int16_t dShift;
    int8_t  iMax;
    int8_t  iMin;
    int16_t i;
    int16_t irc;
    int16_t iMod;
    char   *psz;
    int16_t dWidth;
    int16_t dx;

    irc = IrcRaceDlgHitTest(pt);
    if (irc < 0) {
        return FALSE;
    }
    iMod = irc % 5;
    i = irc / 5;
    if (iMod == 1 && irc < 15) {
        SetCapture(hwnd);
        SetCursor(hcurCloseGrab);
        dWidth = (int16_t)(vplr.rgEnvVarMax[i] - vplr.rgEnvVarMin[i]) / 2;
        while (FGetMouseMove(&pt)) {
            if (pt.x < vrgrcRCW[irc].left) {
                pt.x = vrgrcRCW[irc].left;
            }
            if (pt.x > vrgrcRCW[irc].right) {
                pt.x = vrgrcRCW[irc].right;
            }
            dShift = MulDiv(pt.x - vrgrcRCW[irc].left, 100, vrgrcRCW[irc].right - vrgrcRCW[irc].left);
            if (dWidth > (dShift >= 100 - dWidth ? 100 - dWidth : dShift)) {
                dShift = dWidth;
            } else if (dShift >= 100 - dWidth) {
                dShift = 100 - dWidth;
            }
            if (vplr.rgEnvVarMax[i] != dShift + dWidth) {
                vplr.rgEnvVarMin[i] = dShift - dWidth;
                vplr.rgEnvVarMax[i] = dShift + dWidth;
                vplr.rgEnvVar[i] = vplr.rgEnvVarMin[i] + (int16_t)(vplr.rgEnvVarMax[i] - vplr.rgEnvVarMin[i]) / 2;
                DrawRace2(hwnd, NULL, 1 << i);
            }
        }
        ReleaseCapture();
        return TRUE;
    }
    dWidth = 0;
    dShift = 0;
    psz = 0;
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
            psz = vrgszRCWWidth[0];
            break;
        default:
            dWidth = -1;
            bt = 8;
            psz = vrgszRCWWidth[1];
        }
    }
    InitBtnTrack(&btnt, hwnd, NULL, vrgrcRCW + irc, bt, 80, FALSE, FALSE, psz);
    if (kbd & 4) {
        dWidth = 10 * dWidth;
        dShift = 10 * dShift;
    }
    while (FTrackBtn(&btnt)) {
        if (irc == 15 || irc == 16) {
            iMin = vplr.pctIdealGrowth + dShift;
            iMin = min(20, max(1, iMin));
            if (iMin != vplr.pctIdealGrowth) {
                vplr.pctIdealGrowth = iMin;
                DrawRace2(hwnd, btnt.hdc, 8);
            }
        } else {
            iMin = vplr.rgEnvVarMin[i] - (dWidth - dShift);
            iMax = vplr.rgEnvVarMax[i] + (dWidth + dShift);
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
            if (vplr.rgEnvVarMin[i] != iMin || vplr.rgEnvVarMax[i] != iMax) {
                vplr.rgEnvVarMin[i] = iMin;
                vplr.rgEnvVarMax[i] = iMax;
                vplr.rgEnvVar[i] = vplr.rgEnvVarMin[i] + (int16_t)(vplr.rgEnvVarMax[i] - vplr.rgEnvVarMin[i]) / 2;
                DrawRace2(hwnd, btnt.hdc, 1 << i);
            }
        }
    }
    if (irc < 15) {
        vplr.rgEnvVar[i] = vplr.rgEnvVarMin[i] + (int16_t)(vplr.rgEnvVarMax[i] - vplr.rgEnvVarMin[i]) / 2;
    }
    return TRUE;
}

INT_PTR CALLBACK RaceWizardDlg3(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int16_t     i;
    RECT        rc;
    POINT16     pt;
    HDC         hdc;
    PAINTSTRUCT ps;

    switch (message) {
    case WM_INITDIALOG:
        SetRCWTitle(hwnd, iPanelActive);
        SendMessage(GetDlgItem(hwnd, IDC_RACE_FACTORY_GERMANIUM_DISCOUNT), BM_SETCHECK, GetRaceGrbit(&vplr, ibitRaceCheapFact), 0);
        if (fRCWReadOnly || GetRaceStat(&vplr, rsMajorAdv) == raMacintosh) {
            EnableWindow(GetDlgItem(hwnd, IDC_RACE_FACTORY_GERMANIUM_DISCOUNT), FALSE);
        }
        StickyDlgPos(hwnd, &ptStickyRaceDlg, TRUE);
        return 1;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        if ((HWND)lParam == GetDlgItem(hwnd, IDC_RACE_FACTORY_GERMANIUM_DISCOUNT) || message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (IrcRaceDlgHitTest(pt) < 0)
            break;
        SetCursor(hcurHand);
        return 1;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        return FTrackRaceDlg3(hwnd, pt, wParam);
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        DrawRace3(hwnd, hdc, -1);
        EndPaint(hwnd, &ps);
        return 1;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhStep5PopulationEfficiencyPlayerRace);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            StickyDlgPos(hwnd, &ptStickyRaceDlg, FALSE);
            EndDialog(hwnd, i);
            return 1;
        }
        if (LOWORD(wParam) == IDC_RACE_FACTORY_GERMANIUM_DISCOUNT) {
            i = LOWORD(SendMessage(GetDlgItem(hwnd, IDC_RACE_FACTORY_GERMANIUM_DISCOUNT), BM_GETCHECK, 0, 0));
            SetRaceGrbit(&vplr, ibitRaceCheapFact, i);
            DrawRace3(hwnd, NULL, 99);
        }
    }
    return 0;
}

void DrawRace3(HWND hwnd, HDC hdc, int16_t iDraw) {
    int16_t  dxItem;
    StringId idsT;
    int16_t  fMacintosh;
    int16_t  yTop;
    int16_t  bt;
    StringId ids;
    COLORREF crBkSav;
    int16_t  bkMode;
    int16_t  fCreatedDC;
    int16_t  dxkT;
    int16_t  i;
    int16_t  irc;
    int16_t  dxDig;
    int16_t  dx;
    int16_t  cch;
    RECT     rc;

    fCreatedDC = FALSE;
    bt = !fRCWReadOnly ? 0 : 4;
    fMacintosh = GetRaceStat(&vplr, rsMajorAdv) == raMacintosh;
    if (!hdc) {
        fCreatedDC = TRUE;
        hdc = GetDC(hwnd);
    }
    GetClientRect(hwnd, &rc);
    DrawRaceAdvantagePoints(hdc, &rc, NULL);
    bkMode = SetBkMode(hdc, OPAQUE);
    crBkSav = SetBkColor(hdc, crButtonFace);
    SetTextColor(hdc, crWindowText);
    SelectObject(hdc, rghfontArial8[1]);
    yTop = 3 * dyArial8 + 6;
    dxDig = LOWORD(GetTextExtent(hdc, "9", 1));
    dxkT = LOWORD(GetTextExtent(hdc, "kT", 2));
    ids = idsOneResourceGeneratedEachYearEvery;
    irc = 0;
    for (i = 0; i < 7; i++) {
        if (i == 1 && fMacintosh) {
            SetTextColor(hdc, crButtonShadow);
            bt = 4;
        }
        if (i == 4) {
            if (iDraw == -1) {
                SetWindowPos(GetDlgItem(hwnd, IDC_RACE_FACTORY_GERMANIUM_DISCOUNT), NULL, 6, yTop, rc.right - 12, (int16_t)(3 * dyArial8) / 2, SWP_NOZORDER);
            }
            yTop += (int16_t)(5 * dyArial8) / 2;
        }
        if (fMacintosh && ids == idsOneResourceGeneratedEachYearEvery) {
            idsT = idsAnnualResourcesPlanetValueSqrtPopulationEnergy;
        } else {
            idsT = ids;
        }
        ids++;
        cch = CchGetString(idsT, szWork);
        if (iDraw == -1) {
            TextOut(hdc, 6, yTop, szWork, cch);
        }
        dx = LOWORD(GetTextExtent(hdc, szWork, cch)) + 6;
        dxItem = abs(rgRW3Width[i]) * dxDig;
        wsprintf(szWork, PCTD, GetRaceStat(&vplr, rgRW3IStat[i]));
        if (rgRW3Width[i] < 0 && (i > 0 || !fMacintosh)) {
            dxItem += dxkT;
            if (i == 0) {
                strcat(szWork, "00");
            } else {
                strcat(szWork, "kT");
            }
        }
        dx += dxItem;
        if (iDraw == -1 || iDraw == i) {
            RightTextOut(hdc, dx, yTop, szWork, 0, dxItem);
        }
        vrgrcRCW[irc].left = dx + 4;
        vrgrcRCW[irc].top = yTop - 3;
        vrgrcRCW[irc].right = vrgrcRCW[irc].left + 15;
        vrgrcRCW[irc].bottom = (dyArial8 >> 1) + vrgrcRCW[irc].top + 3;
        vrgrcRCW[irc + 1] = vrgrcRCW[irc];
        OffsetRect((RECT *)&vrgrcRCW[irc + 1].left, 0, vrgrcRCW[irc].bottom - vrgrcRCW[irc].top - 1);
        if (iDraw == -1) {
            DrawBtn(hdc, vrgrcRCW + irc, 0xa0 | bt, FALSE, NULL);
            DrawBtn(hdc, vrgrcRCW + (irc + 1), 0xa1 | bt, FALSE, NULL);
        }
        if (iDraw == -1) {
            if (fMacintosh && ids == idsColonists) {
                idsT = idsMsg0261;
            } else {
                idsT = ids;
            }
            cch = CchGetString(idsT, szWork);
            TextOut(hdc, vrgrcRCW[irc].right + 4, yTop, szWork, cch);
        }
        ids++;
        irc += 2;
        yTop += (int16_t)((rgRW3Spacing[i] + 1) * dyArial8) / 2;
    }
    if (fMacintosh) {
        crcRCW = 2;
    } else {
        crcRCW = irc;
    }
    SetBkColor(hdc, crBkSav);
    SetBkMode(hdc, bkMode);
    if (fCreatedDC) {
        ReleaseDC(hwnd, hdc);
    }
    return;
}

int16_t FTrackRaceDlg3(HWND hwnd, POINT16 pt, int16_t kbd) {
    BTNT    btnt;
    int16_t bt;
    int16_t dShift;
    int16_t i;
    int16_t irc;
    int16_t iMod;
    int16_t iStat;

    irc = IrcRaceDlgHitTest(pt);
    if (irc < 0) {
        return FALSE;
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
    InitBtnTrack(&btnt, hwnd, NULL, vrgrcRCW + irc, bt, 80, FALSE, FALSE, NULL);
    if (kbd & 4) {
        dShift = 3 * dShift;
    }
    while (FTrackBtn(&btnt)) {
        iStat = GetRaceStat(&vplr, rgRW3IStat[i]);
        if (SetRaceStat(&vplr, rgRW3IStat[i], iStat + dShift) != iStat) {
            DrawRace3(hwnd, btnt.hdc, i);
        }
    }
    return TRUE;
}

INT_PTR CALLBACK RaceWizardDlg4(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int16_t     i;
    RECT        rc;
    HDC         hdc;
    char        szT[600];
    StringId    ids;
    PAINTSTRUCT ps;
    int16_t     cch;
    RECT        rcGBox;

    switch (message) {
    case WM_INITDIALOG:
        SetRCWTitle(hwnd, iPanelActive);
        CheckRadioButton(hwnd, IDC_RACE_HYPER_EXPANSION, IDC_RACE_JACK_OF_ALL_TRADES, GetRaceStat(&vplr, rsMajorAdv) + 271);
        if (fRCWReadOnly) {
            for (i = 271; i <= 280; i++) {
                EnableWindow(GetDlgItem(hwnd, i), FALSE);
            }
        }
        StickyDlgPos(hwnd, &ptStickyRaceDlg, TRUE);
        return 1;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        for (i = 271; i <= 280 && (HWND)lParam != GetDlgItem(hwnd, i); i++) {
        }
        if (i <= 280 || message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rc);
        DrawRaceAdvantagePoints(hdc, &rc, NULL);
        SelectObject(hdc, rghfontArial8[1]);
        SetBkColor(hdc, crButtonFace);
        GetWindowRect(GetDlgItem(hwnd, IDC_RACE_HYPER_EXPANSION), &rcGBox);
        ScreenToClient(hwnd, (POINT *)&rcGBox);
        GetWindowRect(GetDlgItem(hwnd, IDC_RACE_JACK_OF_ALL_TRADES), &rc);
        ScreenToClient(hwnd, (POINT *)&rc.right);
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, dyArial8 + 2, dyArial8 >> 1);
        rcGBox.top -= 4;
        _Draw3dFrame(hdc, &rcGBox, -1);
        cch = CchGetString(idsPrimaryRacialTrait, szWork);
        TextOut(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, cch);
        GetClientRect(hwnd, &rc);
        rc.top = rcGBox.bottom + 12;
        rc.left += 12;
        rc.right -= 12;
        GetWindowRect(GetDlgItem(hwnd, IDC_HELP), &rcGBox);
        ScreenToClient(hwnd, (POINT *)&rcGBox);
        rc.bottom = rcGBox.top - 6;
        _Draw3dFrame(hdc, &rc, -1);
        cch = CchGetString(idsDescriptionTrait, szWork);
        TextOut(hdc, rc.left + 8, rc.top - (dyArial8 >> 1), szWork, cch);
        ids = idsMustExpandSurviveGivenSmallCheapColony + GetRaceStat(&vplr, rsMajorAdv) * 3;
        cch = 0;
        i = 0;
        while (i < 3) {
            cch += CchGetString(ids, &szT[cch]);
            i++;
            ids++;
        }
        ExpandRc(&rc, -dyArial8 - 2, -(dyArial8 >> 1));
        rc.top += 4;
        DrawText(hdc, szT, cch, &rc, DT_WORDBREAK);
        rcCargo = rc;
        EndPaint(hwnd, &ps);
        return 1;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhStep2PrimaryRacialTraits);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            StickyDlgPos(hwnd, &ptStickyRaceDlg, FALSE);
            EndDialog(hwnd, i);
            return 1;
        }
        if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_RACE_HYPER_EXPANSION && LOWORD(wParam) <= IDC_RACE_JACK_OF_ALL_TRADES) {
            i = LOWORD(wParam) - 271;
            SetRaceStat(&vplr, rsMajorAdv, i);
            if (GetRaceStat(&vplr, rsMajorAdv) == raMacintosh) {
                SetRaceStat(&vplr, rsFactProd, 10);
                SetRaceStat(&vplr, rsFactBuild, 10);
                SetRaceStat(&vplr, rsFactOperate, 10);
                SetRaceStat(&vplr, rsMineProd, 10);
                SetRaceStat(&vplr, rsMineBuild, 5);
                SetRaceStat(&vplr, rsMineOperate, 10);
                SetRaceGrbit(&vplr, ibitRaceCheapFact, FALSE);
            }
            InvalidateAdvPtsRect(hwnd);
            InvalidateRect(hwnd, &rcCargo, FALSE);
        }
    }
    return 0;
}

INT_PTR CALLBACK RaceWizardDlg5(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int16_t     i;
    RECT        rc;
    HWND        hwndCtl;
    HDC         hdc;
    PAINTSTRUCT ps;
    int16_t     cch;
    RECT        rcGBox;

    switch (message) {
    case WM_INITDIALOG:
        SetRCWTitle(hwnd, iPanelActive);
        cColDrop = 0;
        for (i = 0; i <= 13; i++) {
            hwndCtl = GetDlgItem(hwnd, i + 291);
            SetWindowText(hwndCtl, PszGetCompressedString(idsImprovedFuelEfficiency + i));
            SendMessage(hwndCtl, BM_SETCHECK, GetRaceGrbit(&vplr, i), 0);
            if (fRCWReadOnly) {
                EnableWindow(hwndCtl, FALSE);
            }
        }
        StickyDlgPos(hwnd, &ptStickyRaceDlg, TRUE);
        return 1;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        for (i = 291; i <= 304 && (HWND)lParam != GetDlgItem(hwnd, i); i++) {
        }
        if (i <= 304 || message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rc);
        DrawRaceAdvantagePoints(hdc, &rc, NULL);
        SelectObject(hdc, rghfontArial8[1]);
        SetBkColor(hdc, crButtonFace);
        GetWindowRect(GetDlgItem(hwnd, IDC_RACE_REGENERATING_SHIELDS), &rcGBox);
        ScreenToClient(hwnd, (POINT *)&rcGBox.right);
        GetClientRect(hwnd, &rc);
        rc.top = rcGBox.bottom + 12;
        rc.left += 12;
        rc.right -= 12;
        GetWindowRect(GetDlgItem(hwnd, IDC_HELP), &rcGBox);
        ScreenToClient(hwnd, (POINT *)&rcGBox);
        rc.bottom = rcGBox.top - 12;
        _Draw3dFrame(hdc, &rc, -1);
        rcCargo = rc;
        rcCargo.top -= dyArial8 >> 1;
        cch = CchGetString(idsImprovedFuelEfficiency + cColDrop, szWork);
        TextOut(hdc, rc.left + 8, rc.top - (dyArial8 >> 1), szWork, cch);
        cch = CchGetString(idsGivesFuelMizerGalaxyScoopEnginesIncreases + cColDrop, szWork);
        ExpandRc(&rc, -dyArial8 - 2, -(dyArial8 >> 1));
        rc.top += 4;
        DrawText(hdc, szWork, cch, &rc, DT_WORDBREAK);
        EndPaint(hwnd, &ps);
        return 1;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhStep3LesserTraitsPlayerRace);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            StickyDlgPos(hwnd, &ptStickyRaceDlg, FALSE);
            EndDialog(hwnd, i);
            return 1;
        }
        if (LOWORD(wParam) >= IDC_RACE_IMPROVED_FUEL_EFFICIENCY && LOWORD(wParam) <= IDC_RACE_REGENERATING_SHIELDS) {
            i = LOWORD(SendMessage(GetDlgItem(hwnd, LOWORD(wParam)), BM_GETCHECK, 0, 0));
            cColDrop = LOWORD(wParam) - 291;
            SetRaceGrbit(&vplr, cColDrop, i);
            InvalidateAdvPtsRect(hwnd);
            InvalidateRect(hwnd, &rcCargo, FALSE);
        }
    }
    return 0;
}

INT_PTR CALLBACK RaceWizardDlg6(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int16_t     i;
    RECT        rc;
    HDC         hdc;
    PAINTSTRUCT ps;
    int16_t     cch;
    RECT        rcGBox;

    switch (message) {
    case WM_INITDIALOG:
        SetRCWTitle(hwnd, iPanelActive);
        for (i = 0; i < 6; i++) {
            CheckRadioButton(hwnd, 3 * i + 271, 3 * i + 273, 3 * i + 271 + GetRaceStat(&vplr, i + 8));
        }
        if (fRCWReadOnly) {
            for (i = 271; i <= 288; i++) {
                EnableWindow(GetDlgItem(hwnd, i), FALSE);
            }
        }
        wsprintf(szWork, PszGetCompressedString(idsCosts75ExtraResearchFieldsStartTech), (GetRaceStat(&vplr, rsMajorAdv) == raNone) + 3);
        SetWindowText(GetDlgItem(hwnd, IDC_RACE_START_HIGHER_TECH), szWork);
        SendMessage(GetDlgItem(hwnd, IDC_RACE_START_HIGHER_TECH), BM_SETCHECK, GetRaceGrbit(&vplr, ibitRaceTech3), 0);
        if (fRCWReadOnly) {
            EnableWindow(GetDlgItem(hwnd, IDC_RACE_START_HIGHER_TECH), FALSE);
        }
        StickyDlgPos(hwnd, &ptStickyRaceDlg, TRUE);
        return 1;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        for (i = 271; i <= 288 && (HWND)lParam != GetDlgItem(hwnd, i); i++) {
        }
        if (i <= 288 || (HWND)lParam == GetDlgItem(hwnd, IDC_RACE_START_HIGHER_TECH) || message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rc);
        DrawRaceAdvantagePoints(hdc, &rc, NULL);
        SelectObject(hdc, rghfontArial8[1]);
        SetBkColor(hdc, crButtonFace);
        for (i = 0; i < 6; i++) {
            GetWindowRect(GetDlgItem(hwnd, 3 * i + 271), &rcGBox);
            ScreenToClient(hwnd, (POINT *)&rcGBox);
            GetWindowRect(GetDlgItem(hwnd, 3 * i + 273), &rc);
            ScreenToClient(hwnd, (POINT *)&rc.right);
            rcGBox.right = rc.right;
            rcGBox.bottom = rc.bottom;
            ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
            _Draw3dFrame(hdc, &rcGBox, -1);
            cch = CchGetString(idsEnergy + i, szWork);
            cch += CchGetString(idsResearch, &szWork[cch]);
            TextOut(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, cch);
        }
        EndPaint(hwnd, &ps);
        return 1;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP) {
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhStep6ResearchCostsPlayerRace);
            return 1;
        }
        for (i = 0; i < 4 && LOWORD(wParam) != rgidRaceBtn[i]; i++) {
        }
        if (i < 4) {
            StickyDlgPos(hwnd, &ptStickyRaceDlg, FALSE);
            EndDialog(hwnd, i);
            return 1;
        }
        if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_RACE_ENERGY_COST_EXTRA && LOWORD(wParam) <= IDC_RACE_BIOTECH_COST_LESS) {
            i = LOWORD(wParam) - 271;
            SetRaceStat(&vplr, i / 3 + 8, i % 3);
            InvalidateAdvPtsRect(hwnd);
        } else if (LOWORD(wParam) == IDC_RACE_START_HIGHER_TECH) {
            i = LOWORD(SendMessage(GetDlgItem(hwnd, LOWORD(wParam)), BM_GETCHECK, 0, 0));
            SetRaceGrbit(&vplr, ibitRaceTech3, i);
            InvalidateAdvPtsRect(hwnd);
        }
    }
    return 0;
}

void InvalidateAdvPtsRect(HWND hwnd) {
    HDC        hdc;
    TEXTMETRIC tm;
    LOGFONT   *plf;
    int16_t    dyBig;
    HFONT      hfont;
    int16_t    dx;
    RECT       rc;
    HFONT      hfontSav;

    plf = (LOGFONT *)LocalAlloc(64, sizeof(LOGFONT));
    hdc = GetDC(hwnd);
    plf->lfHeight = -24;
    strcpy(plf->lfFaceName, rgszArial[1]);
    hfont = CreateFontIndirect(plf);
    hfontSav = (HFONT)SelectObject(hdc, hfont);
    GetTextMetrics(hdc, &tm);
    dyBig = tm.tmHeight + tm.tmExternalLeading;
    dx = LOWORD(GetTextExtent(hdc, "-99999", 6)) + 8;
    GetClientRect(hwnd, &rc);
    rc.left = rc.right - dx;
    rc.bottom = rc.top + dyBig + 4;
    SelectObject(hdc, hfontSav);
    DeleteObject(hfont);
    LocalFree(plf);
    ReleaseDC(hwnd, hdc);
    InvalidateRect(hwnd, &rc, TRUE);
    return;
}

void DrawRaceAdvantagePoints(HDC hdc, RECT *prc, PLAYER *pplr) {
    TEXTMETRIC tm;
    LOGFONT   *plf;
    COLORREF   crBkSav;
    int16_t    bkMode;
    int16_t    dyBig;
    char       szAdvantage[32];
    int16_t    c;
    COLORREF   crSav;
    HFONT      hfont;
    int16_t    dx;
    int16_t    iPts;
    int16_t    cch;
    RECT       rc;
    HFONT      hfontSav;

    plf = (LOGFONT *)LocalAlloc(64, sizeof(LOGFONT));
    plf->lfHeight = -24;
    CchGetString(idsArialBold, plf->lfFaceName);
    hfont = CreateFontIndirect(plf);
    hfontSav = (HFONT)SelectObject(hdc, hfont);
    GetTextMetrics(hdc, &tm);
    dyBig = tm.tmHeight + tm.tmExternalLeading;
    dx = LOWORD(GetTextExtent(hdc, "-99999", 6)) + 8;
    rc = *prc;
    rc.left = rc.right - dx;
    rc.bottom = rc.top + dyBig + 4;
    if (!pplr) {
        pplr = &vplr;
    }
    iPts = CAdvantagePoints(pplr);
    bkMode = SetBkMode(hdc, OPAQUE);
    crSav = SetTextColor(hdc, iPts < 0 ? 127 : 0);
    crBkSav = SetBkColor(hdc, crButtonFace);
    c = wsprintf(szWork, PCTD, iPts);
    RcCtrTextOut(hdc, &rc, szWork, -1);
    SetTextColor(hdc, 0);
    SelectObject(hdc, rghfontArial8[1]);
    c = CchGetString(idsPointsLeft, szWork);
    dx = LOWORD(GetTextExtent(hdc, szWork, c));
    cch = CchGetString(idsAdvantage, szAdvantage);
    RightTextOut(hdc, rc.left, 3, szAdvantage, cch, 0);
    RightTextOut(hdc, rc.left, dyArial8 + 3, szWork, c, 0);
    rc.left -= dx + 4;
    PatBlt(hdc, rc.left - 1, 0, 1, rc.bottom + 1, BLACKNESS);
    PatBlt(hdc, rc.left - 4, 0, 2, rc.bottom + 4, BLACKNESS);
    PatBlt(hdc, rc.left, rc.bottom, rc.right - rc.left, 1, BLACKNESS);
    PatBlt(hdc, rc.left - 2, rc.bottom + 2, rc.right - rc.left + 2, 2, BLACKNESS);
    SetBkColor(hdc, crBkSav);
    SetTextColor(hdc, crSav);
    SetBkMode(hdc, bkMode);
    SelectObject(hdc, hfontSav);
    DeleteObject(hfont);
    LocalFree(plf);
    return;
}

static char szRaceFileSuggest[256];

void SetSzRaceFile(const char *pszFile) {
    size_t cch = CchUtf8Prefix(pszFile, sizeof(szRaceFile) - 1);
    size_t cchSuggest = CchUtf8Prefix(pszFile, sizeof(szRaceFileSuggest) - 1);

    /* Full native paths belong to UI storage; the original cache keeps its size. */
    memcpy(szRaceFileSuggest, pszFile, cchSuggest);
    szRaceFileSuggest[cchSuggest] = 0;
    memcpy(szRaceFile, pszFile, cch);
    szRaceFile[cch] = 0;
}

int16_t FConfirmLegacyRaceNames(char *szName, char *szNames) {
    char szPrompt[256];

    CchSprintf(szPrompt, PszGetCompressedString(idsNativeLegacyRaceNameShortened), szName, szNames);
    return AlertSz(szPrompt, MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) == IDYES;
}

int16_t FSaveRace(char *szFileSuggest, PLAYER *pplr) {
    uint16_t     icksum;
    char         szFileTitle[256];
    char         szDirName[256];
    char         szFilter[256];
    uint16_t     i;
    char         szFile[256];
    OPENFILENAME ofn;

    if (szFileSuggest == szRaceFile && szRaceFile[0] && szRaceFileSuggest[0] &&
        strncmp(szRaceFile, szRaceFileSuggest, strlen(szRaceFile)) == 0) szFileSuggest = szRaceFileSuggest;
    if (szFileSuggest) {
        strcpy(szFile, szFileSuggest);
    } else {
        szFile[0] = 0;
    }
    szDirName[0] = 0;
    CchGetString(idsStarsRaceFilesR, szFilter);
    for (i = 0; szFilter[i] != 0; i++) {
        if (szFilter[i] == '|') {
            szFilter[i] = 0;
        }
    }
    memset(&ofn, 0, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwndRaceParent;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = 0x100;
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = 0x100;
    ofn.lpstrInitialDir = szDirName;
    ofn.lpstrDefExt = "r1";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN;
    if (FNativeSaveFileName(&ofn) != 0) {
        if (!FCreateFile(dtRace, iplrNone, szFile)) {
            AlertSz(PszFormatIds(idsStarsUnableSaveRaceDataFilePlease, NULL), MB_ICONHAND);
            return FALSE;
        }
        WriteRtPlr(pplr, NULL);
        icksum = IRaceChecksum(pplr);
        WriteRt(rtEOF, 2, &icksum);
        StreamClose();
        SetSzRaceFile(&szFile[ofn.nFileOffset]);
        return TRUE;
    }
    return FALSE;
}

void SetRCWTitle(HWND hwnd, int16_t iStep) {
    char    szBuf[50];
    int16_t cch;

    cch = CchGetString(idsCustomRaceWizardStepD6 + fRCWReadOnly, szBuf);
    cch = wsprintf(szWork, szBuf, iStep);
    SetWindowText(hwnd, szWork);
    return;
}
