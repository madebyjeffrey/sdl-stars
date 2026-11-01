#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "transportpresets.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"

INT_PTR  ZipOrderDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HDC            hdc;
    std::int16_t        i;
    PAINTSTRUCT    ps;
    RECT           rc;
    HWND           hwndRad;
    char          *psz;
    char          *pszT;
    RECT           rcGBox;
    std::int16_t        cch;
    std::int16_t        xCtr;
    FARPROC        lpProc;

    switch (message) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnSetWindowText(hwnd, TextIO().pfnCompressed(app, idsCustomizeZipOrders));
        ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_ZIP_PROD_QUEUE), win32::SW_HIDE);
        app.lpui->hwndZipOrderDlg = hwnd;
        win32::CheckRadioButton(hwnd, IDC_ZIP_PROD_PRESET_1, IDC_ZIP_PROD_PRESET_4, IDC_ZIP_PROD_PRESET_1);
        EnableZipBtns(app, hwnd, 0);
        app.lpui->iResTechNow = Energy;
        for (i = 1073; i <= 1076; i++) {
            if (app.state.vrgZip[i - 1073].fValid) {
                pszT = app.state.szWork;
                psz = app.state.vrgZip[i - 1073].szName;
                while (*psz != 0) {
                    if ((*pszT++ = *psz++) == '&') {
                        *pszT++ = '&';
                    }
                }
                *pszT = 0;
                psz = app.state.szWork;
            } else {
                psz = TextIO().pfnCompressed(app, idsUnusedD);
                win32::wsprintfA(app.state.szWork, psz, i - 1072);
                psz = app.state.szWork;
            }
            hwndRad = ControlIO().pfnGetDlgItem(hwnd, i);
            ControlIO().pfnSetWindowText(hwndRad, psz);
        }
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyZipOrderDlg, 1);
        if (app.state.gd.fTutorial) {
            PresentationIO().pfnTutor(app);
        }
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
        for (i = 1073; i <= 1076 && reinterpret_cast<HWND>(lParam) != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 1076) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_ZIP_PROD_PRESET_1), &rcGBox);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcGBox));
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_ZIP_PROD_PRESET_4), &rc);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rc.right));
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, app.state.dyArial8, app.state.dyArial8 >> 1);
        _Draw3dFrame(app, hdc, &rcGBox, -1);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        win32::SetBkColor(hdc, app.lpui->crButtonFace);
        cch = TextIO().pfnCchGetString(app, idsCustomOrders, app.state.szWork);
        win32::TextOutA(hdc, rcGBox.left + 8, rcGBox.top - (app.state.dyArial8 >> 1), app.state.szWork, cch);
        rcGBox.top = rcGBox.bottom + 8;
        if (app.state.vrgZip[app.lpui->iResTechNow].fValid) {
            xCtr = LOWORD(GetTextExtent(hdc, app.state.rgszMinerals[2], 9)) + 8;
            for (i = 0; i < 5; i++) {
                win32::SetTextColor(hdc, app.lpui->rgcrMinerals[i]);
                RightTextOut(app, hdc, xCtr, rcGBox.top, app.state.rgszMinerals[i], 0, 0);
                win32::SetTextColor(hdc, 0);
                FormatTransportPresetAction(app, i);
                win32::TextOutA(hdc, xCtr + 6, rcGBox.top, app.state.szWork, std::strlen(app.state.szWork));
                rcGBox.top += app.state.dyArial8;
            }
        } else {
            cch = TextIO().pfnCchGetString(app, idsEmptyCustomSlot, app.state.szWork);
            win32::TextOutA(hdc, 12, rcGBox.top, app.state.szWork, cch);
        }
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_COMMAND:
        if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_ZIP_PROD_PRESET_1 && LOWORD(wParam) <= IDC_ZIP_PROD_PRESET_4) {
            app.lpui->iResTechNow = static_cast<TechFieldType>(LOWORD(wParam) - 1073);
            EnableZipBtns(app, hwnd, app.lpui->iResTechNow);
            ControlIO().pfnInvalidateRect(hwnd, nullptr, 1);
        } else {
            switch (LOWORD(wParam)) {
            case IDOK:
            case IDCANCEL:
                app.lpui->hwndZipOrderDlg = 0;
                ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyZipOrderDlg, 0);
                ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
                if (app.state.gd.fTutorial) {
                    PresentationIO().pfnTutor(app);
                }
                return 1;
            case IDC_IMPORT:
            case IDC_RENAME:
                if (app.state.vrgZip[app.lpui->iResTechNow].fValid) {
                    std::strcpy(app.state.szWork, app.state.vrgZip[app.lpui->iResTechNow].szName);
                } else {
                    win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsCustomD), app.lpui->iResTechNow);
                }
                lpProc = (RenameZipDlg);
                if (ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_RENAME), app.lpui->hwndFrame, lpProc, 0) != 0) {
                    if (app.state.szWork[0] == 0) {
                        win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsCustomD), app.lpui->iResTechNow);
                    }
                    std::strcpy(app.state.vrgZip[app.lpui->iResTechNow].szName, app.state.szWork);
                    pszT = &app.state.szWork[64];
                    psz = app.state.szWork;
                    while (*psz != 0) {
                        if ((*pszT++ = *psz++) == '&') {
                            *pszT++ = '&';
                        }
                    }
                    *pszT = 0;
                    ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, app.lpui->iResTechNow + 1073), &app.state.szWork[64]);
                    if (LOWORD(wParam) == IDC_IMPORT) {
                        app.state.vrgZip[app.lpui->iResTechNow].fValid = 1;
                        app.state.vrgZip[app.lpui->iResTechNow].txp = app.state.sel.fl.lpplord->rgord()[app.state.sel.iwpAct].txp;
                        ControlIO().pfnInvalidateRect(hwnd, nullptr, 1);
                    }
                    EnableZipBtns(app, hwnd, app.lpui->iResTechNow);
                }
                ControlIO().pfnSetFocus(hwnd);
                app.state.gd.fChgZipOrd = 1;
                break;
            case IDC_DELETE:
                app.state.vrgZip[app.lpui->iResTechNow].fValid = 0;
                win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsUnusedD), app.lpui->iResTechNow + 1);
                ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, app.lpui->iResTechNow + 1073), app.state.szWork);
                ControlIO().pfnInvalidateRect(hwnd, nullptr, 1);
                app.state.gd.fChgZipOrd = 1;
                break;
            case IDC_HELP:
                win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhCustomZipOrdersDialog);
                return 1;
            }
        }
    }
    return 0;
}

void EnableZipBtns(APPSESSION &app, HWND hwnd, std::int16_t iSel) {
    std::int16_t fEnabled;

    fEnabled = app.state.vrgZip[iSel].fValid;
    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DELETE), fEnabled);
    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_RENAME), fEnabled);
    return;
}

INT_PTR  RenameZipDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    StringId ids;
    RECT     rc;

    switch (message) {
    case win32::WM_INITDIALOG:
        if (app.lpui->hwndZipOrderDlg) {
            ids = idsRenameZipOrder;
        } else {
            ids = idsRenameProductionTemplate;
        }
        ControlIO().pfnSetWindowText(hwnd, TextIO().pfnCompressed(app, ids));
        ControlIO().pfnSetWindowPos(hwnd, nullptr, app.lpui->ptStickyRenameDlg.x + 70, app.lpui->ptStickyRenameDlg.y + 70, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
        ui::SendDlgItemMessage(hwnd, IDC_EDIT1, win32::EM_LIMITTEXT, 0xc, 0);
        ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_EDIT1), app.state.szWork);
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRenameDlg, 1);
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
        if (message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            if (LOWORD(wParam) == IDOK) {
                win32::GetDlgItemTextA(hwnd, IDC_EDIT1, app.state.szWork, 14);
            }
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRenameDlg, 0);
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, static_cast<std::uint32_t>((!app.lpui->hwndZipOrderDlg ? 1106 : 3103)));
            return 1;
        }
    }
    return 0;
}

INT_PTR  RenameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    RECT    rc;
    std::int32_t lSel;

    switch (message) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnSetWindowText(hwnd, TextIO().pfnCompressed(app, idsRenameFleet));
        ControlIO().pfnSetWindowPos(hwnd, nullptr, app.lpui->ptStickyRenameDlg.x + 70, app.lpui->ptStickyRenameDlg.y + 70, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
        ui::SendDlgItemMessage(hwnd, IDC_EDIT1, win32::EM_LIMITTEXT, 0x1f, 0);
        ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_EDIT1), app.state.szWork);
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRenameDlg, 1);
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
        if (message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            if (LOWORD(wParam) == IDOK) {
                win32::GetDlgItemTextA(hwnd, IDC_EDIT1, app.state.szWork, 32);
                FStringFitsScreen(app, app.state.szWork, 160);
            }
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRenameDlg, 0);
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_EDIT1:
            if (HIWORD(wParam) == 0x400 && !app.lpui->fInEditUpdate) {
                app.lpui->fInEditUpdate = 1;
                ControlIO().pfnGetWindowText(reinterpret_cast<HWND>(lParam), app.state.szWork, 250);
                lSel = ControlIO().pfnSendMessage(reinterpret_cast<HWND>(lParam), win32::EM_GETSEL, 0, 0);
                if (!FStringFitsScreen(app, app.state.szWork, 160)) {
                    ControlIO().pfnSetWindowText(reinterpret_cast<HWND>(lParam), app.state.szWork);
                    ControlIO().pfnSendMessage(reinterpret_cast<HWND>(lParam), win32::EM_SETSEL, LOWORD(lSel), static_cast<std::int16_t>(HIWORD(lSel)));
                }
                app.lpui->fInEditUpdate = 0;
                break;
            }
            /* fallthrough */
        default:
            if (LOWORD(wParam) == IDC_HELP) {
                win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhRenameFleetDialog);
                return 1;
            }
        }
    }
    return 0;
}

INT_PTR  MergeFleetsDlg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    std::int16_t i;
    RECT    rc;
    char    szT[80];
    char   *psz;

    switch (msg) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyMergeFleetsDlg, 1);
        for (i = 0; i < app.state.vcflMerge; i++) {
            psz = PszGetFleetName(app, app.state.rglpfl[app.state.vrgiflMerge[i]]->id);
            std::strcpy(szT, psz);
            if (app.state.rglpfl[app.state.vrgiflMerge[i]]->cord > 1) {
                std::strcat(szT, " *");
            }
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST), win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(szT));
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST), win32::LB_SETSEL, app.state.vcflMerge == 2 || app.state.rglpfl[app.state.vrgiflMerge[i]]->id == app.state.sel.fl.id, i);
        }
        if (app.state.gd.fTutorial) {
            PresentationIO().pfnTutor(app);
        }
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
        if (reinterpret_cast<HWND>(lParam) != ControlIO().pfnGetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST)) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            for (i = 0; i < app.state.vcflMerge; i++) {
                if (ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST), win32::LB_GETSEL, i, 0) == 0) {
                    app.state.vrgiflMerge[i] = iflNone;
                }
            }
            if (LOWORD(wParam) == IDOK && app.state.gd.fTutorial && !FOKMergeDialog(app)) {
                return 1;
            }
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyMergeFleetsDlg, 0);
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhMergeFleetsDialog);
            return 1;
        case IDC_MERGE_FLEETS_SELECT_ALL:
        case IDC_MERGE_FLEETS_UNSELECT_ALL:
            for (i = 0; i < app.state.vcflMerge; i++) {
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_MERGE_FLEETS_LIST), win32::LB_SETSEL, LOWORD(wParam) == IDC_MERGE_FLEETS_SELECT_ALL, i);
            }
            return 1;
        }
    }
    return 0;
}

void FormatTransportPresetAction(APPSESSION &app, std::int16_t i) {
    std::int16_t cch;
    XferActionType iAction;
                iAction = static_cast<XferActionType>(app.state.vrgZip[app.lpui->iResTechNow].txp.rgia[i].iAction());
                cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsAction + static_cast<int>(iAction)), app.state.szWork);
                if (app.state.szWork[cch - 1] == '.') {
                    win32::wsprintfA(&app.state.szWork[cch - 3], " %dkT", app.state.vrgZip[app.lpui->iResTechNow].txp.rgia[i].cQuan());
                }
}
