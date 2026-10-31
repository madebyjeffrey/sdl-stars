#include "win.h"
#include "transportpresets.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"

INT_PTR CALLBACK ZipOrderDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC            hdc;
    int16_t        i;
    PAINTSTRUCT    ps;
    RECT           rc;
    HWND           hwndRad;
    char          *psz;
    char          *pszT;
    RECT           rcGBox;
    int16_t        cch;
    int16_t        xCtr;
    FARPROC        lpProc;

    switch (message) {
    case WM_INITDIALOG:
        ControlIO().pfnFUiSetWindowText(hwnd, TextIO().pfnCompressed(idsCustomizeZipOrders));
        FUiShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_ZIP_PROD_QUEUE), SW_HIDE);
        hwndZipOrderDlg = hwnd;
        CheckRadioButton(hwnd, IDC_ZIP_PROD_PRESET_1, IDC_ZIP_PROD_PRESET_4, IDC_ZIP_PROD_PRESET_1);
        EnableZipBtns(hwnd, 0);
        iResTechNow = Energy;
        for (i = 1073; i <= 1076; i++) {
            if (vrgZip[i - 1073].fValid) {
                pszT = szWork;
                psz = vrgZip[i - 1073].szName;
                while (*psz != 0) {
                    if ((*pszT++ = *psz++) == '&') {
                        *pszT++ = '&';
                    }
                }
                *pszT = 0;
                psz = szWork;
            } else {
                psz = TextIO().pfnCompressed(idsUnusedD);
                wsprintf(szWork, psz, i - 1072);
                psz = szWork;
            }
            hwndRad = ControlIO().pfnGetDlgItem(hwnd, i);
            ControlIO().pfnFUiSetWindowText(hwndRad, psz);
        }
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyZipOrderDlg, TRUE);
        if (gd.fTutorial) {
            PresentationIO().pfnTutor();
        }
        return 1;
    case WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        for (i = 1073; i <= 1076 && (HWND)lParam != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 1076) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_ZIP_PROD_PRESET_1), &rcGBox);
        ScreenToClient(hwnd, (POINT *)&rcGBox);
        GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_ZIP_PROD_PRESET_4), &rc);
        ScreenToClient(hwnd, (POINT *)&rc.right);
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        _Draw3dFrame(hdc, &rcGBox, -1);
        SelectObject(hdc, rghfontArial8[1]);
        SetBkColor(hdc, crButtonFace);
        cch = TextIO().pfnCchGetString(idsCustomOrders, szWork);
        TextOut(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, cch);
        rcGBox.top = rcGBox.bottom + 8;
        if (vrgZip[iResTechNow].fValid) {
            xCtr = LOWORD(GetTextExtent(hdc, rgszMinerals[2], 9)) + 8;
            for (i = 0; i < 5; i++) {
                SetTextColor(hdc, rgcrMinerals[i]);
                RightTextOut(hdc, xCtr, rcGBox.top, rgszMinerals[i], 0, 0);
                SetTextColor(hdc, 0);
                FormatTransportPresetAction(i);
                TextOut(hdc, xCtr + 6, rcGBox.top, szWork, strlen(szWork));
                rcGBox.top += dyArial8;
            }
        } else {
            cch = TextIO().pfnCchGetString(idsEmptyCustomSlot, szWork);
            TextOut(hdc, 12, rcGBox.top, szWork, cch);
        }
        EndPaint(hwnd, &ps);
        return 1;
    case WM_COMMAND:
        if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_ZIP_PROD_PRESET_1 && LOWORD(wParam) <= IDC_ZIP_PROD_PRESET_4) {
            iResTechNow = static_cast<TechFieldType>(LOWORD(wParam) - 1073);
            EnableZipBtns(hwnd, iResTechNow);
            ControlIO().pfnInvalidateRect(hwnd, NULL, TRUE);
        } else {
            switch (LOWORD(wParam)) {
            case IDOK:
            case IDCANCEL:
                hwndZipOrderDlg = 0;
                ControlIO().pfnStickyDlgPos(hwnd, &ptStickyZipOrderDlg, FALSE);
                ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
                if (gd.fTutorial) {
                    PresentationIO().pfnTutor();
                }
                return 1;
            case IDC_IMPORT:
            case IDC_RENAME:
                if (vrgZip[iResTechNow].fValid) {
                    strcpy(szWork, vrgZip[iResTechNow].szName);
                } else {
                    wsprintf(szWork, TextIO().pfnCompressed(idsCustomD), iResTechNow);
                }
                lpProc = MakeProcInstance(RenameZipDlg, hInst);
                if (ControlIO().pfnLUiDialogBox(hInst, MAKEINTRESOURCE(IDD_RENAME), hwndFrame, lpProc, 0) != 0) {
                    if (szWork[0] == 0) {
                        wsprintf(szWork, TextIO().pfnCompressed(idsCustomD), iResTechNow);
                    }
                    strcpy(vrgZip[iResTechNow].szName, szWork);
                    pszT = &szWork[64];
                    psz = szWork;
                    while (*psz != 0) {
                        if ((*pszT++ = *psz++) == '&') {
                            *pszT++ = '&';
                        }
                    }
                    *pszT = 0;
                    ControlIO().pfnFUiSetWindowText(ControlIO().pfnGetDlgItem(hwnd, iResTechNow + 1073), &szWork[64]);
                    if (LOWORD(wParam) == IDC_IMPORT) {
                        vrgZip[iResTechNow].fValid = TRUE;
                        vrgZip[iResTechNow].txp = sel.fl.lpplord->rgord()[sel.iwpAct].txp;
                        ControlIO().pfnInvalidateRect(hwnd, NULL, TRUE);
                    }
                    EnableZipBtns(hwnd, iResTechNow);
                }
                FreeProcInstance(lpProc);
                ControlIO().pfnHwndUiSetFocus(hwnd);
                gd.fChgZipOrd = TRUE;
                break;
            case IDC_DELETE:
                vrgZip[iResTechNow].fValid = FALSE;
                wsprintf(szWork, TextIO().pfnCompressed(idsUnusedD), iResTechNow + 1);
                ControlIO().pfnFUiSetWindowText(ControlIO().pfnGetDlgItem(hwnd, iResTechNow + 1073), szWork);
                ControlIO().pfnInvalidateRect(hwnd, NULL, TRUE);
                gd.fChgZipOrd = TRUE;
                break;
            case IDC_HELP:
                WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhCustomZipOrdersDialog);
                return 1;
            }
        }
    }
    return 0;
}

void EnableZipBtns(HWND hwnd, int16_t iSel) {
    int16_t fEnabled;

    fEnabled = vrgZip[iSel].fValid;
    ControlIO().pfnFUiEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DELETE), fEnabled);
    ControlIO().pfnFUiEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_RENAME), fEnabled);
    return;
}

INT_PTR CALLBACK RenameZipDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    StringId ids;
    RECT     rc;

    switch (message) {
    case WM_INITDIALOG:
        if (hwndZipOrderDlg) {
            ids = idsRenameZipOrder;
        } else {
            ids = idsRenameProductionTemplate;
        }
        ControlIO().pfnFUiSetWindowText(hwnd, TextIO().pfnCompressed(ids));
        ControlIO().pfnFUiSetWindowPos(hwnd, NULL, ptStickyRenameDlg.x + 70, ptStickyRenameDlg.y + 70, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        LUiSendDlgItemMessage(hwnd, IDC_EDIT1, EM_LIMITTEXT, 0xc, 0);
        ControlIO().pfnFUiSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_EDIT1), szWork);
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyRenameDlg, TRUE);
        return 1;
    case WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        if (message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            if (LOWORD(wParam) == IDOK) {
                GetDlgItemText(hwnd, IDC_EDIT1, szWork, 14);
            }
            ControlIO().pfnStickyDlgPos(hwnd, &ptStickyRenameDlg, FALSE);
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, (uint32_t)(!hwndZipOrderDlg ? 1106 : 3103));
            return 1;
        }
    }
    return 0;
}

INT_PTR CALLBACK RenameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    RECT    rc;
    int32_t lSel;

    switch (message) {
    case WM_INITDIALOG:
        ControlIO().pfnFUiSetWindowText(hwnd, TextIO().pfnCompressed(idsRenameFleet));
        ControlIO().pfnFUiSetWindowPos(hwnd, NULL, ptStickyRenameDlg.x + 70, ptStickyRenameDlg.y + 70, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        LUiSendDlgItemMessage(hwnd, IDC_EDIT1, EM_LIMITTEXT, 0x1f, 0);
        ControlIO().pfnFUiSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_EDIT1), szWork);
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyRenameDlg, TRUE);
        return 1;
    case WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        if (message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            if (LOWORD(wParam) == IDOK) {
                GetDlgItemText(hwnd, IDC_EDIT1, szWork, 32);
                FStringFitsScreen(szWork, 160);
            }
            ControlIO().pfnStickyDlgPos(hwnd, &ptStickyRenameDlg, FALSE);
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_EDIT1:
            if (HIWORD(wParam) == 0x400 && !fInEditUpdate) {
                fInEditUpdate = TRUE;
                ControlIO().pfnGetWindowText((HWND)lParam, szWork, 250);
                lSel = ControlIO().pfnLUiSendMessage((HWND)lParam, EM_GETSEL, 0, 0);
                if (!FStringFitsScreen(szWork, 160)) {
                    ControlIO().pfnFUiSetWindowText((HWND)lParam, szWork);
                    ControlIO().pfnLUiSendMessage((HWND)lParam, EM_SETSEL, LOWORD(lSel), (int16_t)HIWORD(lSel));
                }
                fInEditUpdate = FALSE;
                break;
            }
            /* fallthrough */
        default:
            if (LOWORD(wParam) == IDC_HELP) {
                WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhRenameFleetDialog);
                return 1;
            }
        }
    }
    return 0;
}

INT_PTR CALLBACK MergeFleetsDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    int16_t i;
    RECT    rc;
    char    szT[80];
    char   *psz;

    switch (msg) {
    case WM_INITDIALOG:
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyMergeFleetsDlg, TRUE);
        for (i = 0; i < vcflMerge; i++) {
            psz = PszGetFleetName(rglpfl[vrgiflMerge[i]]->id);
            strcpy(szT, psz);
            if (rglpfl[vrgiflMerge[i]]->cord > 1) {
                strcat(szT, " *");
            }
            ControlIO().pfnLUiSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST), LB_ADDSTRING, 0, (LPARAM)szT);
            ControlIO().pfnLUiSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST), LB_SETSEL, vcflMerge == 2 || rglpfl[vrgiflMerge[i]]->id == sel.fl.id, i);
        }
        if (gd.fTutorial) {
            PresentationIO().pfnTutor();
        }
        return 1;
    case WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        if ((HWND)lParam != ControlIO().pfnGetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST)) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            for (i = 0; i < vcflMerge; i++) {
                if (ControlIO().pfnLUiSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST), LB_GETSEL, i, 0) == 0) {
                    vrgiflMerge[i] = iflNone;
                }
            }
            if (LOWORD(wParam) == IDOK && gd.fTutorial && !FOKMergeDialog()) {
                return 1;
            }
            ControlIO().pfnStickyDlgPos(hwnd, &ptStickyMergeFleetsDlg, FALSE);
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhMergeFleetsDialog);
            return 1;
        case IDC_MERGE_FLEETS_SELECT_ALL:
        case IDC_MERGE_FLEETS_UNSELECT_ALL:
            for (i = 0; i < vcflMerge; i++) {
                ControlIO().pfnLUiSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST), LB_SETSEL, LOWORD(wParam) == IDC_MERGE_FLEETS_SELECT_ALL, i);
            }
            return 1;
        }
    }
    return 0;
}

void FormatTransportPresetAction(int16_t i) {
    int16_t cch;
    XferActionType iAction;
                iAction = static_cast<XferActionType>(vrgZip[iResTechNow].txp.rgia[i].iAction());
                cch = TextIO().pfnCchGetString(static_cast<StringId>(idsAction + static_cast<int>(iAction)), szWork);
                if (szWork[cch - 1] == '.') {
                    wsprintf(&szWork[cch - 3], " %dkT", vrgZip[iResTechNow].txp.rgia[i].cQuan());
                }
}
