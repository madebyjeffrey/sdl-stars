#include "win.h"
#include "transportpresets.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"

INT_PTR  ZipOrderDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
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
    case win32::WM_INITDIALOG:
        ControlIO().pfnSetWindowText(hwnd, TextIO().pfnCompressed(idsCustomizeZipOrders));
        ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_ZIP_PROD_QUEUE), win32::SW_HIDE);
        hwndZipOrderDlg = hwnd;
        win32::CheckRadioButton(hwnd, IDC_ZIP_PROD_PRESET_1, IDC_ZIP_PROD_PRESET_4, IDC_ZIP_PROD_PRESET_1);
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
                win32::wsprintfA(szWork, psz, i - 1072);
                psz = szWork;
            }
            hwndRad = ControlIO().pfnGetDlgItem(hwnd, i);
            ControlIO().pfnSetWindowText(hwndRad, psz);
        }
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyZipOrderDlg, 1);
        if (gd.fTutorial) {
            PresentationIO().pfnTutor();
        }
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
        for (i = 1073; i <= 1076 && (HWND)lParam != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 1076) {
            win32::SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_ZIP_PROD_PRESET_1), &rcGBox);
        win32::ScreenToClient(hwnd, (POINT *)&rcGBox);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_ZIP_PROD_PRESET_4), &rc);
        win32::ScreenToClient(hwnd, (POINT *)&rc.right);
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        _Draw3dFrame(hdc, &rcGBox, -1);
        win32::SelectObject(hdc, rghfontArial8[1]);
        win32::SetBkColor(hdc, crButtonFace);
        cch = TextIO().pfnCchGetString(idsCustomOrders, szWork);
        win32::TextOutA(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, cch);
        rcGBox.top = rcGBox.bottom + 8;
        if (vrgZip[iResTechNow].fValid) {
            xCtr = LOWORD(GetTextExtent(hdc, rgszMinerals[2], 9)) + 8;
            for (i = 0; i < 5; i++) {
                win32::SetTextColor(hdc, rgcrMinerals[i]);
                RightTextOut(hdc, xCtr, rcGBox.top, rgszMinerals[i], 0, 0);
                win32::SetTextColor(hdc, 0);
                FormatTransportPresetAction(i);
                win32::TextOutA(hdc, xCtr + 6, rcGBox.top, szWork, strlen(szWork));
                rcGBox.top += dyArial8;
            }
        } else {
            cch = TextIO().pfnCchGetString(idsEmptyCustomSlot, szWork);
            win32::TextOutA(hdc, 12, rcGBox.top, szWork, cch);
        }
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_COMMAND:
        if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_ZIP_PROD_PRESET_1 && LOWORD(wParam) <= IDC_ZIP_PROD_PRESET_4) {
            iResTechNow = static_cast<TechFieldType>(LOWORD(wParam) - 1073);
            EnableZipBtns(hwnd, iResTechNow);
            ControlIO().pfnInvalidateRect(hwnd, NULL, 1);
        } else {
            switch (LOWORD(wParam)) {
            case IDOK:
            case IDCANCEL:
                hwndZipOrderDlg = 0;
                ControlIO().pfnStickyDlgPos(hwnd, &ptStickyZipOrderDlg, 0);
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
                    win32::wsprintfA(szWork, TextIO().pfnCompressed(idsCustomD), iResTechNow);
                }
                lpProc = (RenameZipDlg);
                if (ControlIO().pfnDialogBox(hInst, win32::MAKEINTRESOURCE(IDD_RENAME), hwndFrame, lpProc, 0) != 0) {
                    if (szWork[0] == 0) {
                        win32::wsprintfA(szWork, TextIO().pfnCompressed(idsCustomD), iResTechNow);
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
                    ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, iResTechNow + 1073), &szWork[64]);
                    if (LOWORD(wParam) == IDC_IMPORT) {
                        vrgZip[iResTechNow].fValid = 1;
                        vrgZip[iResTechNow].txp = sel.fl.lpplord->rgord()[sel.iwpAct].txp;
                        ControlIO().pfnInvalidateRect(hwnd, NULL, 1);
                    }
                    EnableZipBtns(hwnd, iResTechNow);
                }
                ControlIO().pfnSetFocus(hwnd);
                gd.fChgZipOrd = 1;
                break;
            case IDC_DELETE:
                vrgZip[iResTechNow].fValid = 0;
                win32::wsprintfA(szWork, TextIO().pfnCompressed(idsUnusedD), iResTechNow + 1);
                ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, iResTechNow + 1073), szWork);
                ControlIO().pfnInvalidateRect(hwnd, NULL, 1);
                gd.fChgZipOrd = 1;
                break;
            case IDC_HELP:
                win32::WinHelpA(hwnd, szHelpFile, win32::HELP_CONTEXT, idhCustomZipOrdersDialog);
                return 1;
            }
        }
    }
    return 0;
}

void EnableZipBtns(HWND hwnd, int16_t iSel) {
    int16_t fEnabled;

    fEnabled = vrgZip[iSel].fValid;
    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DELETE), fEnabled);
    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_RENAME), fEnabled);
    return;
}

INT_PTR  RenameZipDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    StringId ids;
    RECT     rc;

    switch (message) {
    case win32::WM_INITDIALOG:
        if (hwndZipOrderDlg) {
            ids = idsRenameZipOrder;
        } else {
            ids = idsRenameProductionTemplate;
        }
        ControlIO().pfnSetWindowText(hwnd, TextIO().pfnCompressed(ids));
        ControlIO().pfnSetWindowPos(hwnd, NULL, ptStickyRenameDlg.x + 70, ptStickyRenameDlg.y + 70, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
        ui::SendDlgItemMessage(hwnd, IDC_EDIT1, win32::EM_LIMITTEXT, 0xc, 0);
        ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_EDIT1), szWork);
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyRenameDlg, 1);
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
        if (message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            if (LOWORD(wParam) == IDOK) {
                win32::GetDlgItemTextA(hwnd, IDC_EDIT1, szWork, 14);
            }
            ControlIO().pfnStickyDlgPos(hwnd, &ptStickyRenameDlg, 0);
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            win32::WinHelpA(hwnd, szHelpFile, win32::HELP_CONTEXT, (uint32_t)(!hwndZipOrderDlg ? 1106 : 3103));
            return 1;
        }
    }
    return 0;
}

INT_PTR  RenameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    RECT    rc;
    int32_t lSel;

    switch (message) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnSetWindowText(hwnd, TextIO().pfnCompressed(idsRenameFleet));
        ControlIO().pfnSetWindowPos(hwnd, NULL, ptStickyRenameDlg.x + 70, ptStickyRenameDlg.y + 70, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
        ui::SendDlgItemMessage(hwnd, IDC_EDIT1, win32::EM_LIMITTEXT, 0x1f, 0);
        ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_EDIT1), szWork);
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyRenameDlg, 1);
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
        if (message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            if (LOWORD(wParam) == IDOK) {
                win32::GetDlgItemTextA(hwnd, IDC_EDIT1, szWork, 32);
                FStringFitsScreen(szWork, 160);
            }
            ControlIO().pfnStickyDlgPos(hwnd, &ptStickyRenameDlg, 0);
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_EDIT1:
            if (HIWORD(wParam) == 0x400 && !fInEditUpdate) {
                fInEditUpdate = 1;
                ControlIO().pfnGetWindowText((HWND)lParam, szWork, 250);
                lSel = ControlIO().pfnSendMessage((HWND)lParam, win32::EM_GETSEL, 0, 0);
                if (!FStringFitsScreen(szWork, 160)) {
                    ControlIO().pfnSetWindowText((HWND)lParam, szWork);
                    ControlIO().pfnSendMessage((HWND)lParam, win32::EM_SETSEL, LOWORD(lSel), (int16_t)HIWORD(lSel));
                }
                fInEditUpdate = 0;
                break;
            }
            /* fallthrough */
        default:
            if (LOWORD(wParam) == IDC_HELP) {
                win32::WinHelpA(hwnd, szHelpFile, win32::HELP_CONTEXT, idhRenameFleetDialog);
                return 1;
            }
        }
    }
    return 0;
}

INT_PTR  MergeFleetsDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    int16_t i;
    RECT    rc;
    char    szT[80];
    char   *psz;

    switch (msg) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyMergeFleetsDlg, 1);
        for (i = 0; i < vcflMerge; i++) {
            psz = PszGetFleetName(rglpfl[vrgiflMerge[i]]->id);
            strcpy(szT, psz);
            if (rglpfl[vrgiflMerge[i]]->cord > 1) {
                strcat(szT, " *");
            }
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST), win32::LB_ADDSTRING, 0, (LPARAM)szT);
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST), win32::LB_SETSEL, vcflMerge == 2 || rglpfl[vrgiflMerge[i]]->id == sel.fl.id, i);
        }
        if (gd.fTutorial) {
            PresentationIO().pfnTutor();
        }
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
        if ((HWND)lParam != ControlIO().pfnGetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST)) {
            win32::SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            for (i = 0; i < vcflMerge; i++) {
                if (ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST), win32::LB_GETSEL, i, 0) == 0) {
                    vrgiflMerge[i] = iflNone;
                }
            }
            if (LOWORD(wParam) == IDOK && gd.fTutorial && !FOKMergeDialog()) {
                return 1;
            }
            ControlIO().pfnStickyDlgPos(hwnd, &ptStickyMergeFleetsDlg, 0);
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            win32::WinHelpA(hwnd, szHelpFile, win32::HELP_CONTEXT, idhMergeFleetsDialog);
            return 1;
        case IDC_MERGE_FLEETS_SELECT_ALL:
        case IDC_MERGE_FLEETS_UNSELECT_ALL:
            for (i = 0; i < vcflMerge; i++) {
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST), win32::LB_SETSEL, LOWORD(wParam) == IDC_MERGE_FLEETS_SELECT_ALL, i);
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
                    win32::wsprintfA(&szWork[cch - 3], " %dkT", vrgZip[iResTechNow].txp.rgia[i].cQuan());
                }
}
