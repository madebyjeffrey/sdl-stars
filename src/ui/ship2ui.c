#include "win.h"

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
    XferActionType iAction;
    FARPROC        lpProc;

    switch (message) {
    case WM_INITDIALOG:
        SetWindowText(hwnd, PszGetCompressedString(idsCustomizeZipOrders));
        ShowWindow(GetDlgItem(hwnd, IDC_ZIP_PROD_QUEUE), SW_HIDE);
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
                psz = PszGetCompressedString(idsUnusedD);
                wsprintf(szWork, psz, i - 1072);
                psz = szWork;
            }
            hwndRad = GetDlgItem(hwnd, i);
            SetWindowText(hwndRad, psz);
        }
        StickyDlgPos(hwnd, &ptStickyZipOrderDlg, TRUE);
        if (gd.fTutorial) {
            AdvanceTutor();
        }
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
        for (i = 1073; i <= 1076 && (HWND)lParam != GetDlgItem(hwnd, i); i++) {
        }
        if (i <= 1076) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rc);
        GetWindowRect(GetDlgItem(hwnd, IDC_ZIP_PROD_PRESET_1), &rcGBox);
        ScreenToClient(hwnd, (POINT *)&rcGBox);
        GetWindowRect(GetDlgItem(hwnd, IDC_ZIP_PROD_PRESET_4), &rc);
        ScreenToClient(hwnd, (POINT *)&rc.right);
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        _Draw3dFrame(hdc, &rcGBox, -1);
        SelectObject(hdc, rghfontArial8[1]);
        SetBkColor(hdc, crButtonFace);
        cch = CchGetString(idsCustomOrders, szWork);
        TextOut(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, cch);
        rcGBox.top = rcGBox.bottom + 8;
        if (vrgZip[iResTechNow].fValid) {
            xCtr = LOWORD(GetTextExtent(hdc, rgszMinerals[2], 9)) + 8;
            for (i = 0; i < 5; i++) {
                SetTextColor(hdc, rgcrMinerals[i]);
                RightTextOut(hdc, xCtr, rcGBox.top, rgszMinerals[i], 0, 0);
                SetTextColor(hdc, 0);
                iAction = vrgZip[iResTechNow].txp.rgia[i].iAction;
                cch = CchGetString(idsAction + iAction, szWork);
                if (szWork[cch - 1] == '.') {
                    wsprintf(&szWork[cch - 3], " %dkT", vrgZip[iResTechNow].txp.rgia[i].cQuan);
                }
                TextOut(hdc, xCtr + 6, rcGBox.top, szWork, strlen(szWork));
                rcGBox.top += dyArial8;
            }
        } else {
            cch = CchGetString(idsEmptyCustomSlot, szWork);
            TextOut(hdc, 12, rcGBox.top, szWork, cch);
        }
        EndPaint(hwnd, &ps);
        return 1;
    case WM_COMMAND:
        if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_ZIP_PROD_PRESET_1 && LOWORD(wParam) <= IDC_ZIP_PROD_PRESET_4) {
            iResTechNow = LOWORD(wParam) - 1073;
            EnableZipBtns(hwnd, iResTechNow);
            InvalidateRect(hwnd, NULL, TRUE);
        } else {
            switch (LOWORD(wParam)) {
            case IDOK:
            case IDCANCEL:
                hwndZipOrderDlg = 0;
                StickyDlgPos(hwnd, &ptStickyZipOrderDlg, FALSE);
                EndDialog(hwnd, LOWORD(wParam) == IDOK);
                if (gd.fTutorial) {
                    AdvanceTutor();
                }
                return 1;
            case IDC_IMPORT:
            case IDC_RENAME:
                if (vrgZip[iResTechNow].fValid) {
                    strcpy(szWork, vrgZip[iResTechNow].szName);
                } else {
                    wsprintf(szWork, PszGetCompressedString(idsCustomD), iResTechNow);
                }
                lpProc = MakeProcInstance(RenameZipDlg, hInst);
                if (DialogBox(hInst, MAKEINTRESOURCE(IDD_RENAME), hwndFrame, lpProc) != 0) {
                    if (szWork[0] == 0) {
                        wsprintf(szWork, PszGetCompressedString(idsCustomD), iResTechNow);
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
                    SetWindowText(GetDlgItem(hwnd, iResTechNow + 1073), &szWork[64]);
                    if (LOWORD(wParam) == IDC_IMPORT) {
                        vrgZip[iResTechNow].fValid = TRUE;
                        vrgZip[iResTechNow].txp = sel.fl.lpplord->rgord[sel.iwpAct].txp;
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                    EnableZipBtns(hwnd, iResTechNow);
                }
                FreeProcInstance(lpProc);
                SetFocus(hwnd);
                gd.fChgZipOrd = TRUE;
                break;
            case IDC_DELETE:
                vrgZip[iResTechNow].fValid = FALSE;
                wsprintf(szWork, PszGetCompressedString(idsUnusedD), iResTechNow + 1);
                SetWindowText(GetDlgItem(hwnd, iResTechNow + 1073), szWork);
                InvalidateRect(hwnd, NULL, TRUE);
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
    EnableWindow(GetDlgItem(hwnd, IDC_DELETE), fEnabled);
    EnableWindow(GetDlgItem(hwnd, IDC_RENAME), fEnabled);
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
        SetWindowText(hwnd, PszGetCompressedString(ids));
        SetWindowPos(hwnd, NULL, ptStickyRenameDlg.x + 70, ptStickyRenameDlg.y + 70, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        SendDlgItemMessage(hwnd, IDC_EDIT1, EM_LIMITTEXT, 0xc, 0);
        SetWindowText(GetDlgItem(hwnd, IDC_EDIT1), szWork);
        StickyDlgPos(hwnd, &ptStickyRenameDlg, TRUE);
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
            StickyDlgPos(hwnd, &ptStickyRenameDlg, FALSE);
            EndDialog(hwnd, LOWORD(wParam) == IDOK);
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
        SetWindowText(hwnd, PszGetCompressedString(idsRenameFleet));
        SetWindowPos(hwnd, NULL, ptStickyRenameDlg.x + 70, ptStickyRenameDlg.y + 70, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        SendDlgItemMessage(hwnd, IDC_EDIT1, EM_LIMITTEXT, 0x1f, 0);
        SetWindowText(GetDlgItem(hwnd, IDC_EDIT1), szWork);
        StickyDlgPos(hwnd, &ptStickyRenameDlg, TRUE);
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
            StickyDlgPos(hwnd, &ptStickyRenameDlg, FALSE);
            EndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_EDIT1:
            if (HIWORD(wParam) == 0x400 && !fInEditUpdate) {
                fInEditUpdate = TRUE;
                GetWindowText((HWND)lParam, szWork, 250);
                lSel = SendMessage((HWND)lParam, EM_GETSEL, 0, 0);
                if (!FStringFitsScreen(szWork, 160)) {
                    SetWindowText((HWND)lParam, szWork);
                    SendMessage((HWND)lParam, EM_SETSEL, LOWORD(lSel), (int16_t)HIWORD(lSel));
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
        StickyDlgPos(hwnd, &ptStickyMergeFleetsDlg, TRUE);
        for (i = 0; i < vcflMerge; i++) {
            psz = PszGetFleetName(rglpfl[vrgiflMerge[i]]->id);
            strcpy(szT, psz);
            if (rglpfl[vrgiflMerge[i]]->cord > 1) {
                strcat(szT, " *");
            }
            SendMessage(GetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST), LB_ADDSTRING, 0, (LPARAM)szT);
            SendMessage(GetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST), LB_SETSEL, vcflMerge == 2 || rglpfl[vrgiflMerge[i]]->id == sel.fl.id, i);
        }
        if (gd.fTutorial) {
            AdvanceTutor();
        }
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
        if ((HWND)lParam != GetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST)) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            for (i = 0; i < vcflMerge; i++) {
                if (SendMessage(GetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST), LB_GETSEL, i, 0) == 0) {
                    vrgiflMerge[i] = iflNone;
                }
            }
            if (LOWORD(wParam) == IDOK && gd.fTutorial && !FOKMergeDialog()) {
                return 1;
            }
            StickyDlgPos(hwnd, &ptStickyMergeFleetsDlg, FALSE);
            EndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhMergeFleetsDialog);
            return 1;
        case IDC_MERGE_FLEETS_SELECT_ALL:
        case IDC_MERGE_FLEETS_UNSELECT_ALL:
            for (i = 0; i < vcflMerge; i++) {
                SendMessage(GetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST), LB_SETSEL, LOWORD(wParam) == IDC_MERGE_FLEETS_SELECT_ALL, i);
            }
            return 1;
        }
    }
    return 0;
}
