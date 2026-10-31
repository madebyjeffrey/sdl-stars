#include "win.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"

INT_PTR  RelationsDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int16_t     i;
    RECT        rc;
    HDC         hdc;
    PAINTSTRUCT ps;
    RECT        rcGBox;
    ScanView    mdSBase;

    switch (message) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyRelationsDlg, 1);
        win32::CheckRadioButton(hwnd, IDC_RELATIONS_NEUTRAL, IDC_RELATIONS_ENEMY, rgplr[idPlayer].rgmdRelation[idPlayer == 0] + 2004);
        for (i = 0; i < game.cPlayer; i++) {
            if (i != idPlayer) {
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST), win32::LB_ADDSTRING, 0, (LPARAM)TextIO().pfnPlayerName(i, 0, 0, 0, 0, NULL));
            }
        }
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST), win32::LB_SETCURSEL, 0, 0);
        fDirtyPlan = 0;
        /* fallthrough */
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_RELATIONS_FRIEND), &rcGBox);
        win32::ScreenToClient(hwnd, (POINT *)&rcGBox);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_RELATIONS_ENEMY), &rc);
        win32::ScreenToClient(hwnd, (POINT *)&rc.right);
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        _Draw3dFrame(hdc, &rcGBox, -1);
        win32::SetBkColor(hdc, crButtonFace);
        win32::SelectObject(hdc, rghfontArial8[1]);
        i = TextIO().pfnCchGetString(idsRelation, szWork);
        win32::TextOutA(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, i);
        win32::SelectObject(hdc, rghfontArial8[0]);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        if ((HWND)lParam != ControlIO().pfnGetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST)) {
            win32::SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) {
            ControlIO().pfnStickyDlgPos(hwnd, &ptStickyRelationsDlg, 0);
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST), win32::LB_GETCURSEL, 0, 0));
            if (i >= idPlayer) {
                i++;
            }
            ControlIO().pfnEndDialog(hwnd, i + 3);
            return 1;
        }
        if (LOWORD(wParam) >= IDC_RELATIONS_NEUTRAL && LOWORD(wParam) <= IDC_RELATIONS_ENEMY) {
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST), win32::LB_GETCURSEL, 0, 0));
            if (i >= idPlayer) {
                i++;
            }
            rgplr[idPlayer].rgmdRelation[i] = LOWORD(wParam) - 2004;
            fDirtyPlan = 1;
        } else if (LOWORD(wParam) == IDC_RELATIONS_PLAYER_LIST) {
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST), win32::LB_GETCURSEL, 0, 0));
            if (i >= idPlayer) {
                i++;
            }
            win32::CheckRadioButton(hwnd, IDC_RELATIONS_NEUTRAL, IDC_RELATIONS_ENEMY, rgplr[idPlayer].rgmdRelation[i] + 2004);
        } else if (LOWORD(wParam) == IDC_HELP) {
            win32::WinHelpA(hwnd, szHelpFile, win32::HELP_CONTEXT, idhPlayerRelationsDialog);
            return 1;
        }
        break;
    case win32::WM_DESTROY:
        if (!fDirtyPlan)
            break;
        mdSBase = static_cast<ScanView>(grbitScan & grbitScanViewMask);
        LogChangeRelations();
        ControlIO().pfnInvalidateRect(hwndScanner, NULL, 1);
        break;
    }
    return 0;
}

INT_PTR  NewPlanNameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    RECT rc;

    switch (message) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnSetWindowPos(hwnd, NULL, ptStickyBattlePlansDlg.x + 70, ptStickyBattlePlansDlg.y + 70, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
        ui::SendDlgItemMessage(hwnd, IDC_EDIT1, win32::EM_LIMITTEXT, 0x1f, 0);
        win32::SetDlgItemTextA(hwnd, IDC_EDIT1, btlplan.szName);
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
                win32::GetDlgItemTextA(hwnd, IDC_EDIT1, btlplan.szName, 32);
                fDirtyPlan = 1;
            }
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            win32::WinHelpA(hwnd, szHelpFile, win32::HELP_CONTEXT, idhBattlePlansDialog);
            return 1;
        }
        break;
    }
    return 0;
}

INT_PTR  BattlePlansDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    FARPROC lpProc;
    int16_t idc;
    int16_t i;
    int16_t fRet;
    RECT    rc;
    int16_t cLen;

    switch (message) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyBattlePlansDlg, 1);
        iPlanSelDlg = 0;
        if (sel.grobj == grobjFleet) {
            iPlanSelDlg = sel.fl.iplan;
        }
        btlplan = rglpbtlplan[idPlayer][iPlanSelDlg];
        for (i = 0; i < rgcbtlplan[idPlayer]; i++) {
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_ADDSTRING, 0, (LPARAM)rglpbtlplan[idPlayer][i].szName);
        }
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_SETCURSEL, iPlanSelDlg, 0);
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_RENAME), iPlanSelDlg > 0);
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DELETE), iPlanSelDlg > 0);
        for (i = idsDisengage; i <= idsMaximizeDamage; i++) {
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_TACTIC), win32::CB_ADDSTRING, 0, (LPARAM)TextIO().pfnCompressed(static_cast<StringId>(i)));
        }
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_TACTIC), win32::CB_SETCURSEL, btlplan.mdTactic, 0);
        for (i = idsNoneDisengage; i <= idsFreighters; i++) {
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_PRIMARY_TARGET), win32::CB_ADDSTRING, 0, (LPARAM)TextIO().pfnCompressed(static_cast<StringId>(i)));
        }
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_PRIMARY_TARGET), win32::CB_SETCURSEL, btlplan.mdTarget1, 0);
        if (!game.fSinglePlr) {
            for (i = idsNobody; i <= idsEveryone; i++) {
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), win32::CB_ADDSTRING, 0, (LPARAM)TextIO().pfnCompressed(static_cast<StringId>(i)));
            }
            for (i = 0; i < game.cPlayer; i++) {
                if (i != idPlayer) {
                    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), win32::CB_ADDSTRING, 0, (LPARAM)TextIO().pfnPlayerName(i, 0, 1, 0, 0, NULL));
                }
            }
            i = btlplan.iplrAttack;
            if (i >= idPlayer + 4) {
                i--;
            }
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), win32::CB_SETCURSEL, i, 0);
        } else {
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), win32::CB_ADDSTRING, 0, (LPARAM)TextIO().pfnCompressed(idsEveryone));
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), win32::CB_SETCURSEL, 0, 0);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), 0);
        }
        for (i = idsNoneDisengage; i <= idsFreighters; i++) {
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SECONDARY_TARGET), win32::CB_ADDSTRING, 0, (LPARAM)TextIO().pfnCompressed(static_cast<StringId>(i)));
        }
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SECONDARY_TARGET), win32::CB_SETCURSEL, btlplan.mdTarget2, 0);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_DUMP_CARGO), win32::BM_SETCHECK, btlplan.fDumpCargo, 0);
        fDirtyPlan = 0;
        if (gd.fTutorial) {
            PresentationIO().pfnTutor();
        }
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        for (idc = 1053; idc <= 1058 && (HWND)lParam != ControlIO().pfnGetDlgItem(hwnd, idc); idc++) {
        }
        if (idc >= 1053 || message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            if (fDirtyPlan) {
                rglpbtlplan[idPlayer][iPlanSelDlg] = btlplan;
                LogChangeBtlplan(&btlplan);
            }
            ControlIO().pfnStickyDlgPos(hwnd, &ptStickyBattlePlansDlg, 0);
            ControlIO().pfnEndDialog(hwnd, iPlanSelDlg);
            if (sel.grobj == grobjFleet) {
                FillBattleDD(sel.fl.iplan + 1);
            }
            iPlanSelDlg = -1;
            return 1;
        case IDC_BATTLE_PLAN_DUMP_CARGO:
            btlplan.fDumpCargo = LOWORD(ui::SendDlgItemMessage(hwnd, IDC_BATTLE_PLAN_DUMP_CARGO, win32::BM_GETCHECK, 0, 0));
            fDirtyPlan = 1;
            break;
        case IDC_DELETE:
            if (fDirtyPlan) {
                rglpbtlplan[idPlayer][iPlanSelDlg] = btlplan;
                LogChangeBtlplan(&btlplan);
                fDirtyPlan = 0;
            }
            btlplan.fDelete = 1;
            rglpbtlplan[idPlayer][iPlanSelDlg] = btlplan;
            btlplan.iplan = iPlanSelDlg;
            if (FDeleteBattlePlan(iPlanSelDlg, 1)) {
                LogChangeBtlplan(&btlplan);
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_SETCURSEL, iPlanSelDlg - 1, 0);
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_RESETCONTENT, 0, 0);
                for (i = 0; i < rgcbtlplan[idPlayer]; i++) {
                    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_ADDSTRING, 0, (LPARAM)rglpbtlplan[idPlayer][i].szName);
                }
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_SETCURSEL, iPlanSelDlg - 1, 0);
                goto LSelectName;
            }
            btlplan.fDelete = 0;
            rglpbtlplan[idPlayer][iPlanSelDlg] = btlplan;
            break;
        case IDC_BATTLE_PLAN_PRIMARY_TARGET:
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, LOWORD(wParam)), win32::CB_GETCURSEL, 0, 0));
            btlplan.mdTarget1 = i;
            fDirtyPlan = 1;
            break;
        case IDC_BATTLE_PLAN_SECONDARY_TARGET:
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, LOWORD(wParam)), win32::CB_GETCURSEL, 0, 0));
            btlplan.mdTarget2 = i;
            fDirtyPlan = 1;
            break;
        case IDC_BATTLE_PLAN_ATTACK_WHO:
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, LOWORD(wParam)), win32::CB_GETCURSEL, 0, 0));
            if (game.fSinglePlr) {
                i = 3;
            } else if (i >= idPlayer + 4) {
                i++;
            }
            btlplan.iplrAttack = i;
            fDirtyPlan = 1;
            break;
        case IDC_BATTLE_PLAN_TACTIC:
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, LOWORD(wParam)), win32::CB_GETCURSEL, 0, 0));
            btlplan.mdTactic = i;
            fDirtyPlan = 1;
            break;
        case IDC_RENAME:
        LRename:
            ControlIO().pfnStickyDlgPos(hwnd, &ptStickyBattlePlansDlg, 0);
            lpProc = (NewPlanNameDlg);
            fRet = ControlIO().pfnDialogBox(hInst, win32::MAKEINTRESOURCE(IDD_RENAME), hwndFrame, lpProc, 0);
            ControlIO().pfnSetFocus(hwnd);
            if (fRet) {
                rglpbtlplan[idPlayer][iPlanSelDlg] = btlplan;
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_RESETCONTENT, 0, 0);
                for (i = 0; i < rgcbtlplan[idPlayer]; i++) {
                    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_ADDSTRING, 0, (LPARAM)rglpbtlplan[idPlayer][i].szName);
                }
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_SETCURSEL, iPlanSelDlg, 0);
            }
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_RENAME), iPlanSelDlg > 0);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DELETE), iPlanSelDlg > 0);
            break;
        case IDC_BATTLE_PLAN_COPY:
            if (rgcbtlplan[idPlayer] == 15) {
                return 0;
            }
            if (fDirtyPlan) {
                rglpbtlplan[idPlayer][iPlanSelDlg] = btlplan;
                LogChangeBtlplan(&btlplan);
                fDirtyPlan = 0;
            }
            iPlanSelDlg = rgcbtlplan[idPlayer]++;
            cLen = strlen(btlplan.szName);
            if (cLen <= 27) {
                if (btlplan.szName[cLen - 1] != ')' || isdigit(btlplan.szName[cLen - 2]) == 0 || btlplan.szName[cLen - 3] != '(') {
                    strcpy(&btlplan.szName[cLen], " (2)");
                } else if (btlplan.szName[cLen - 2] == '9') {
                    btlplan.szName[cLen - 2] = '0';
                } else {
                    btlplan.szName[cLen - 2] = btlplan.szName[cLen - 2] + 1;
                }
            }
            btlplan.iplan = iPlanSelDlg;
            rglpbtlplan[idPlayer][iPlanSelDlg] = btlplan;
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_TACTIC), win32::CB_SETCURSEL, btlplan.mdTactic, 0);
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_RESETCONTENT, 0, 0);
            for (i = 0; i < rgcbtlplan[idPlayer]; i++) {
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_ADDSTRING, 0, (LPARAM)rglpbtlplan[idPlayer][i].szName);
            }
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_SETCURSEL, iPlanSelDlg, 0);
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_PRIMARY_TARGET), win32::CB_SETCURSEL, btlplan.mdTarget1, 0);
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SECONDARY_TARGET), win32::CB_SETCURSEL, btlplan.mdTarget2, 0);
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_DUMP_CARGO), win32::BM_SETCHECK, btlplan.fDumpCargo, 0);
            i = btlplan.iplrAttack;
            if (i >= idPlayer + 4) {
                i--;
            }
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), win32::CB_SETCURSEL, i, 0);
            fDirtyPlan = 1;
            wParam = IDC_BATTLE_PLAN_PRIMARY_TARGET;
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_RENAME), 1);
            goto LRename;
        case IDC_BATTLE_PLAN_SELECT:
        LSelectName:
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_GETCURSEL, 0, 0));
            if (i == iPlanSelDlg)
                break;
            if (fDirtyPlan) {
                rglpbtlplan[idPlayer][iPlanSelDlg] = btlplan;
                LogChangeBtlplan(&btlplan);
                fDirtyPlan = 0;
            }
            iPlanSelDlg = i;
            btlplan = rglpbtlplan[idPlayer][iPlanSelDlg];
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_PRIMARY_TARGET), win32::CB_SETCURSEL, btlplan.mdTarget1, 0);
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SECONDARY_TARGET), win32::CB_SETCURSEL, btlplan.mdTarget2, 0);
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_DUMP_CARGO), win32::BM_SETCHECK, btlplan.fDumpCargo, 0);
            wParam = IDC_BATTLE_PLAN_PRIMARY_TARGET;
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_RENAME), iPlanSelDlg > 0);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DELETE), iPlanSelDlg > 0);
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_TACTIC), win32::CB_SETCURSEL, btlplan.mdTactic, 0);
            i = btlplan.iplrAttack;
            if (i >= idPlayer + 4) {
                i--;
            }
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), win32::CB_SETCURSEL, i, 0);
            break;
        case IDC_HELP:
            win32::WinHelpA(hwnd, szHelpFile, win32::HELP_CONTEXT, idhBattlePlansDialog);
            return 1;
        }
        break;
    }
    return 0;
}
