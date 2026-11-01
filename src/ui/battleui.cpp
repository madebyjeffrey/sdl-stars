#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"

INT_PTR  RelationsDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    std::int16_t     i;
    RECT        rc;
    HDC         hdc;
    PAINTSTRUCT ps;
    RECT        rcGBox;
    ScanView    mdSBase;

    switch (message) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRelationsDlg, 1);
        win32::CheckRadioButton(hwnd, IDC_RELATIONS_NEUTRAL, IDC_RELATIONS_ENEMY, app.state.rgplr[app.state.idPlayer].rgmdRelation[app.state.idPlayer == 0] + 2004);
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (i != app.state.idPlayer) {
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST), win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TextIO().pfnPlayerName(app, i, 0, 0, 0, 0, nullptr).data()));
            }
        }
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST), win32::LB_SETCURSEL, 0, 0);
        app.lpui->fDirtyPlan = 0;
        /* fallthrough */
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_RELATIONS_FRIEND), &rcGBox);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcGBox));
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_RELATIONS_ENEMY), &rc);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rc.right));
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, app.state.dyArial8, app.state.dyArial8 >> 1);
        _Draw3dFrame(app, hdc, &rcGBox, -1);
        win32::SetBkColor(hdc, app.lpui->crButtonFace);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        i = TextIO().pfnCchGetString(app, idsRelation, std::span{app.state.szWork});
        win32::TextOutA(hdc, rcGBox.left + 8, rcGBox.top - (app.state.dyArial8 >> 1), app.state.szWork, i);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        if (reinterpret_cast<HWND>(lParam) != ControlIO().pfnGetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST)) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) {
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyRelationsDlg, 0);
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST), win32::LB_GETCURSEL, 0, 0));
            if (i >= app.state.idPlayer) {
                i++;
            }
            ControlIO().pfnEndDialog(hwnd, i + 3);
            return 1;
        }
        if (LOWORD(wParam) >= IDC_RELATIONS_NEUTRAL && LOWORD(wParam) <= IDC_RELATIONS_ENEMY) {
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST), win32::LB_GETCURSEL, 0, 0));
            if (i >= app.state.idPlayer) {
                i++;
            }
            app.state.rgplr[app.state.idPlayer].rgmdRelation[i] = LOWORD(wParam) - 2004;
            app.lpui->fDirtyPlan = 1;
        } else if (LOWORD(wParam) == IDC_RELATIONS_PLAYER_LIST) {
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_RELATIONS_PLAYER_LIST), win32::LB_GETCURSEL, 0, 0));
            if (i >= app.state.idPlayer) {
                i++;
            }
            win32::CheckRadioButton(hwnd, IDC_RELATIONS_NEUTRAL, IDC_RELATIONS_ENEMY, app.state.rgplr[app.state.idPlayer].rgmdRelation[i] + 2004);
        } else if (LOWORD(wParam) == IDC_HELP) {
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhPlayerRelationsDialog);
            return 1;
        }
        break;
    case win32::WM_DESTROY:
        if (!app.lpui->fDirtyPlan)
            break;
        mdSBase = static_cast<ScanView>(app.lpui->grbitScan & grbitScanViewMask);
        LogChangeRelations(app);
        ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, nullptr, 1);
        break;
    }
    return 0;
}

INT_PTR  NewPlanNameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    RECT rc;

    switch (message) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnSetWindowPos(hwnd, nullptr, app.lpui->ptStickyBattlePlansDlg.x + 70, app.lpui->ptStickyBattlePlansDlg.y + 70, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
        ui::SendDlgItemMessage(hwnd, IDC_EDIT1, win32::EM_LIMITTEXT, 0x1f, 0);
        win32::SetDlgItemTextA(hwnd, IDC_EDIT1, app.lpui->btlplan.szName);
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
                win32::GetDlgItemTextA(hwnd, IDC_EDIT1, app.lpui->btlplan.szName, 32);
                app.lpui->fDirtyPlan = 1;
            }
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhBattlePlansDialog);
            return 1;
        }
        break;
    }
    return 0;
}

INT_PTR  BattlePlansDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    std::int16_t idc;
    std::int16_t i;
    std::int16_t fRet;
    RECT    rc;
    std::int16_t cLen;

    switch (message) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyBattlePlansDlg, 1);
        app.lpui->iPlanSelDlg = 0;
        if (app.state.sel.grobj == grobjFleet) {
            app.lpui->iPlanSelDlg = app.state.sel.fl.iplan;
        }
        app.lpui->btlplan = app.state.rglpbtlplan[app.state.idPlayer][app.lpui->iPlanSelDlg];
        for (i = 0; i < app.state.rgcbtlplan[app.state.idPlayer]; i++) {
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(app.state.rglpbtlplan[app.state.idPlayer][i].szName));
        }
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_SETCURSEL, app.lpui->iPlanSelDlg, 0);
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_RENAME), app.lpui->iPlanSelDlg > 0);
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DELETE), app.lpui->iPlanSelDlg > 0);
        for (i = idsDisengage; i <= idsMaximizeDamage; i++) {
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_TACTIC), win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TextIO().pfnCompressed(app, static_cast<StringId>(i)).data()));
        }
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_TACTIC), win32::CB_SETCURSEL, app.lpui->btlplan.mdTactic, 0);
        for (i = idsNoneDisengage; i <= idsFreighters; i++) {
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_PRIMARY_TARGET), win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TextIO().pfnCompressed(app, static_cast<StringId>(i)).data()));
        }
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_PRIMARY_TARGET), win32::CB_SETCURSEL, app.lpui->btlplan.mdTarget1, 0);
        if (!app.state.game.fSinglePlr) {
            for (i = idsNobody; i <= idsEveryone; i++) {
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TextIO().pfnCompressed(app, static_cast<StringId>(i)).data()));
            }
            for (i = 0; i < app.state.game.cPlayer; i++) {
                if (i != app.state.idPlayer) {
                    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TextIO().pfnPlayerName(app, i, 0, 1, 0, 0, nullptr).data()));
                }
            }
            i = app.lpui->btlplan.iplrAttack;
            if (i >= app.state.idPlayer + 4) {
                i--;
            }
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), win32::CB_SETCURSEL, i, 0);
        } else {
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TextIO().pfnCompressed(app, idsEveryone).data()));
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), win32::CB_SETCURSEL, 0, 0);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), 0);
        }
        for (i = idsNoneDisengage; i <= idsFreighters; i++) {
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SECONDARY_TARGET), win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TextIO().pfnCompressed(app, static_cast<StringId>(i)).data()));
        }
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SECONDARY_TARGET), win32::CB_SETCURSEL, app.lpui->btlplan.mdTarget2, 0);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_DUMP_CARGO), win32::BM_SETCHECK, app.lpui->btlplan.fDumpCargo, 0);
        app.lpui->fDirtyPlan = 0;
        if (app.state.gd.fTutorial) {
            PresentationIO().pfnTutor(app);
        }
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        for (idc = 1053; idc <= 1058 && reinterpret_cast<HWND>(lParam) != ControlIO().pfnGetDlgItem(hwnd, idc); idc++) {
        }
        if (idc >= 1053 || message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            if (app.lpui->fDirtyPlan) {
                app.state.rglpbtlplan[app.state.idPlayer][app.lpui->iPlanSelDlg] = app.lpui->btlplan;
                LogChangeBtlplan(app, &app.lpui->btlplan);
            }
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyBattlePlansDlg, 0);
            ControlIO().pfnEndDialog(hwnd, app.lpui->iPlanSelDlg);
            if (app.state.sel.grobj == grobjFleet) {
                FillBattleDD(app, app.state.sel.fl.iplan + 1);
            }
            app.lpui->iPlanSelDlg = -1;
            return 1;
        case IDC_BATTLE_PLAN_DUMP_CARGO:
            app.lpui->btlplan.fDumpCargo = LOWORD(ui::SendDlgItemMessage(hwnd, IDC_BATTLE_PLAN_DUMP_CARGO, win32::BM_GETCHECK, 0, 0));
            app.lpui->fDirtyPlan = 1;
            break;
        case IDC_DELETE:
            if (app.lpui->fDirtyPlan) {
                app.state.rglpbtlplan[app.state.idPlayer][app.lpui->iPlanSelDlg] = app.lpui->btlplan;
                LogChangeBtlplan(app, &app.lpui->btlplan);
                app.lpui->fDirtyPlan = 0;
            }
            app.lpui->btlplan.fDelete = 1;
            app.state.rglpbtlplan[app.state.idPlayer][app.lpui->iPlanSelDlg] = app.lpui->btlplan;
            app.lpui->btlplan.iplan = app.lpui->iPlanSelDlg;
            if (FDeleteBattlePlan(app, app.lpui->iPlanSelDlg, 1)) {
                LogChangeBtlplan(app, &app.lpui->btlplan);
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_SETCURSEL, app.lpui->iPlanSelDlg - 1, 0);
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_RESETCONTENT, 0, 0);
                for (i = 0; i < app.state.rgcbtlplan[app.state.idPlayer]; i++) {
                    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(app.state.rglpbtlplan[app.state.idPlayer][i].szName));
                }
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_SETCURSEL, app.lpui->iPlanSelDlg - 1, 0);
                goto LSelectName;
            }
            app.lpui->btlplan.fDelete = 0;
            app.state.rglpbtlplan[app.state.idPlayer][app.lpui->iPlanSelDlg] = app.lpui->btlplan;
            break;
        case IDC_BATTLE_PLAN_PRIMARY_TARGET:
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, LOWORD(wParam)), win32::CB_GETCURSEL, 0, 0));
            app.lpui->btlplan.mdTarget1 = i;
            app.lpui->fDirtyPlan = 1;
            break;
        case IDC_BATTLE_PLAN_SECONDARY_TARGET:
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, LOWORD(wParam)), win32::CB_GETCURSEL, 0, 0));
            app.lpui->btlplan.mdTarget2 = i;
            app.lpui->fDirtyPlan = 1;
            break;
        case IDC_BATTLE_PLAN_ATTACK_WHO:
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, LOWORD(wParam)), win32::CB_GETCURSEL, 0, 0));
            if (app.state.game.fSinglePlr) {
                i = 3;
            } else if (i >= app.state.idPlayer + 4) {
                i++;
            }
            app.lpui->btlplan.iplrAttack = i;
            app.lpui->fDirtyPlan = 1;
            break;
        case IDC_BATTLE_PLAN_TACTIC:
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, LOWORD(wParam)), win32::CB_GETCURSEL, 0, 0));
            app.lpui->btlplan.mdTactic = i;
            app.lpui->fDirtyPlan = 1;
            break;
        case IDC_RENAME:
        LRename:
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyBattlePlansDlg, 0);
            fRet = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_RENAME), app.lpui->hwndFrame, NewPlanNameDlg, 0);
            ControlIO().pfnSetFocus(hwnd);
            if (fRet) {
                app.state.rglpbtlplan[app.state.idPlayer][app.lpui->iPlanSelDlg] = app.lpui->btlplan;
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_RESETCONTENT, 0, 0);
                for (i = 0; i < app.state.rgcbtlplan[app.state.idPlayer]; i++) {
                    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(app.state.rglpbtlplan[app.state.idPlayer][i].szName));
                }
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_SETCURSEL, app.lpui->iPlanSelDlg, 0);
            }
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_RENAME), app.lpui->iPlanSelDlg > 0);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DELETE), app.lpui->iPlanSelDlg > 0);
            break;
        case IDC_BATTLE_PLAN_COPY:
            if (app.state.rgcbtlplan[app.state.idPlayer] == 15) {
                return 0;
            }
            if (app.lpui->fDirtyPlan) {
                app.state.rglpbtlplan[app.state.idPlayer][app.lpui->iPlanSelDlg] = app.lpui->btlplan;
                LogChangeBtlplan(app, &app.lpui->btlplan);
                app.lpui->fDirtyPlan = 0;
            }
            app.lpui->iPlanSelDlg = app.state.rgcbtlplan[app.state.idPlayer]++;
            cLen = std::strlen(app.lpui->btlplan.szName);
            if (cLen <= 27) {
                if (app.lpui->btlplan.szName[cLen - 1] != ')' || std::isdigit(app.lpui->btlplan.szName[cLen - 2]) == 0 || app.lpui->btlplan.szName[cLen - 3] != '(') {
                    std::strcpy(&app.lpui->btlplan.szName[cLen], " (2)");
                } else if (app.lpui->btlplan.szName[cLen - 2] == '9') {
                    app.lpui->btlplan.szName[cLen - 2] = '0';
                } else {
                    app.lpui->btlplan.szName[cLen - 2] = app.lpui->btlplan.szName[cLen - 2] + 1;
                }
            }
            app.lpui->btlplan.iplan = app.lpui->iPlanSelDlg;
            app.state.rglpbtlplan[app.state.idPlayer][app.lpui->iPlanSelDlg] = app.lpui->btlplan;
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_TACTIC), win32::CB_SETCURSEL, app.lpui->btlplan.mdTactic, 0);
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_RESETCONTENT, 0, 0);
            for (i = 0; i < app.state.rgcbtlplan[app.state.idPlayer]; i++) {
                ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(app.state.rglpbtlplan[app.state.idPlayer][i].szName));
            }
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_SETCURSEL, app.lpui->iPlanSelDlg, 0);
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_PRIMARY_TARGET), win32::CB_SETCURSEL, app.lpui->btlplan.mdTarget1, 0);
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SECONDARY_TARGET), win32::CB_SETCURSEL, app.lpui->btlplan.mdTarget2, 0);
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_DUMP_CARGO), win32::BM_SETCHECK, app.lpui->btlplan.fDumpCargo, 0);
            i = app.lpui->btlplan.iplrAttack;
            if (i >= app.state.idPlayer + 4) {
                i--;
            }
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), win32::CB_SETCURSEL, i, 0);
            app.lpui->fDirtyPlan = 1;
            wParam = IDC_BATTLE_PLAN_PRIMARY_TARGET;
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_RENAME), 1);
            goto LRename;
        case IDC_BATTLE_PLAN_SELECT:
        LSelectName:
            i = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SELECT), win32::CB_GETCURSEL, 0, 0));
            if (i == app.lpui->iPlanSelDlg)
                break;
            if (app.lpui->fDirtyPlan) {
                app.state.rglpbtlplan[app.state.idPlayer][app.lpui->iPlanSelDlg] = app.lpui->btlplan;
                LogChangeBtlplan(app, &app.lpui->btlplan);
                app.lpui->fDirtyPlan = 0;
            }
            app.lpui->iPlanSelDlg = i;
            app.lpui->btlplan = app.state.rglpbtlplan[app.state.idPlayer][app.lpui->iPlanSelDlg];
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_PRIMARY_TARGET), win32::CB_SETCURSEL, app.lpui->btlplan.mdTarget1, 0);
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_SECONDARY_TARGET), win32::CB_SETCURSEL, app.lpui->btlplan.mdTarget2, 0);
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_DUMP_CARGO), win32::BM_SETCHECK, app.lpui->btlplan.fDumpCargo, 0);
            wParam = IDC_BATTLE_PLAN_PRIMARY_TARGET;
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_RENAME), app.lpui->iPlanSelDlg > 0);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DELETE), app.lpui->iPlanSelDlg > 0);
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_TACTIC), win32::CB_SETCURSEL, app.lpui->btlplan.mdTactic, 0);
            i = app.lpui->btlplan.iplrAttack;
            if (i >= app.state.idPlayer + 4) {
                i--;
            }
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BATTLE_PLAN_ATTACK_WHO), win32::CB_SETCURSEL, i, 0);
            break;
        case IDC_HELP:
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhBattlePlansDialog);
            return 1;
        }
        break;
    }
    return 0;
}
