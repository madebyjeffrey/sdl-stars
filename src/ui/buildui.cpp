#include "win.h"
#include <algorithm>
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"
#include "part_query_io.h"
#include "race_query_io.h"
#include "choice_io.h"
#include "alert_io.h"

std::int16_t ShipBuilder(APPSESSION &app, POINT16 ptDlgSize) {
    std::int16_t fSuccess;

    app.lpui->ptslotGlob = ptDlgSize;
    if (app.state.gd.mdScreenSize > 0) {
        app.lpui->ptslotGlob.y += 3 * app.state.dyArial8;
    }
    app.state.fStarbaseMode = 0;
    app.lpui->lpshdefBuild = NthValidShdef(app, 0);
    fSuccess = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_SLOT), app.lpui->hwndFrame, SlotDlg, 0);
    if (app.state.sel.grobj == grobjPlanet && app.state.sel.pl.lpplprod) {
        ControlIO().pfnFillPlanetProdLB(app, app.lpui->hwndPlanetProdLB, app.state.sel.pl.lpplprod, nullptr);
    }
    return 0;
}

void ShowMainControls(APPSESSION &app, HWND hwnd, std::int16_t sw) {
    ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_IMPORT), sw);
    ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_EDIT), sw);
    ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DELETE), sw);
    ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_SHIPS), sw);
    ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_STARBASES), sw);
    ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_EXISTING), sw);
    ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_HULLS), sw);
    ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_ENEMY_HULLS), sw);
    ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_COMPONENTS), sw);
    ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDOK), sw == win32::SW_SHOW ? win32::SW_HIDE : win32::SW_SHOW);
    win32::SetDlgItemTextA(hwnd, IDCANCEL, TextIO().pfnCompressed(app, sw == win32::SW_SHOW ? idsDone : idsCancel).data());
    return;
}

std::int16_t FCheckQueuedShip(APPSESSION &app, HWND hwnd, SHDEF *lpshdef, std::int16_t fEdit) {
    char     rgch[40];
    std::int16_t  fProgress;
    std::int16_t  id;
    StringId ids;
    std::int16_t  cshQueued;

    cshQueued = CshQueued(app, lpshdef->ishdef, &fProgress, fEdit);
    if (lpshdef->cExist > 0 || cshQueued != 0) {
        if (fEdit) {
            ids = idsCurrentlyHaveDSSIfDelete2;
        } else {
            ids = !app.state.fStarbaseMode ? idsCurrentlyHaveDSSDProduction : idsCurrentlyHaveDSSDProduction2;
        }
        TextIO().pfnCchGetString(app, idsWorkDone, std::span{rgch});
        if (lpshdef->cExist > 0 && cshQueued != 0) {
            win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, ids).data(), LOWORD(lpshdef->cExist), lpshdef->hul.szClass, lpshdef->cExist == 1 ? "" : "s", cshQueued,
                     !fProgress ? "" : rgch);
        } else if (cshQueued != 0) {
            win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, static_cast<StringId>(ids + 1)).data(), cshQueued, lpshdef->hul.szClass, cshQueued == 1 ? "" : "s", !fProgress ? "" : rgch);
        } else {
            win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, static_cast<StringId>(ids + 2)).data(), LOWORD(lpshdef->cExist), lpshdef->hul.szClass, lpshdef->cExist == 1 ? "" : "s");
        }
        id = CompatChoiceIO().pfnMessageBoxA(win32::GetFocus(), app.state.szWork, TextIO().pfnCompressed(app, static_cast<StringId>(idsDeleteDesign + fEdit)).data(), win32::MB_YESNO | win32::MB_ICONQUESTION | win32::MB_TASKMODAL);
        ControlIO().pfnSetFocus(hwnd);
        if (id == 7) {
            return 0;
        }
        if (lpshdef->cExist > 0) {
            DestroyAllIshdef(app, lpshdef->ishdef, app.state.idPlayer);
            ControlIO().pfnInvalidateRect(app.lpui->hwndScanner, nullptr, 1);
            ControlIO().pfnInvalidateRect(app.lpui->hwndMessage, nullptr, 1);
        } else {
            RemoveIshdefFromAllQueues(app, lpshdef->ishdef, fEdit);
        }
    }
    return 1;
}

INT_PTR  SlotDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    RECT               rcWindow;
    HDC                hdc;
    RECT               rcGBox;
    SHDEF             *lpshdef;
    std::int16_t            left;
    PAINTSTRUCT        ps;
    HWND               hwndItem;
    std::int16_t            cch;
    std::int32_t            lSel;
    RECT               rc;
    DRAWITEMSTRUCT    *lpdis;
    MEASUREITEMSTRUCT *lpmis;
    std::int16_t            i;
    POINT16            pt;
    std::int16_t            fProtoSB;
    std::int16_t            fProgress;
    PART               part;
    std::int16_t            cshQueued;
    std::int16_t            j;

    switch (message) {
    case win32::WM_INITDIALOG:
        app.lpui->fHullCopy = 0;
        app.lpui->hwndSlotDlg = hwnd;
        win32::GetWindowRect(hwnd, &rcWindow);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        ControlIO().pfnSetWindowPos(hwnd, nullptr, 0, 0, app.lpui->ptslotGlob.x + rcWindow.right - rcWindow.left - rc.right, app.lpui->ptslotGlob.y + rcWindow.bottom - rcWindow.top - rc.bottom,
                     win32::SWP_NOMOVE | win32::SWP_NOZORDER);
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickySlotDlg, 1);
        UpdateSlotGlobals(app);
        hwndItem = ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_COMPONENT_LIST);
        ControlIO().pfnSetWindowPos(hwndItem, nullptr, app.lpui->ptslotGlob.x - 256, 32, 240, 266, win32::SWP_NOZORDER);
        FillBuildPartsLB(app, hwndItem, rggrbitParts[0]);
        app.lpui->yBuildInfoSum = 340;
        app.lpui->lpfnRealListProc = reinterpret_cast<WNDPROC>(win32::GetWindowLongPtrA(hwndItem, win32::GWLP_WNDPROC));
        win32::SetWindowLongPtrA(hwndItem, win32::GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(app.lpui->lpfnFakeListProc));
        win32::CheckRadioButton(hwnd, IDC_DESIGNER_SHIPS, IDC_DESIGNER_STARBASES, IDC_DESIGNER_SHIPS);
        win32::CheckRadioButton(hwnd, IDC_DESIGNER_EXISTING, IDC_DESIGNER_COMPONENTS, IDC_DESIGNER_EXISTING);
        app.lpui->mdBuild = mdBuildShdef;
        hwndItem = ControlIO().pfnGetDlgItem(hwnd, IDC_COMBOBOX);
        ControlIO().pfnSetWindowPos(hwndItem, nullptr, app.lpui->ptslotGlob.x - 264, 8, 240, 100, win32::SWP_NOZORDER);
        FillBuildDD(app, hwndItem, app.lpui->mdBuild);
        ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDOK), nullptr, app.lpui->ptslotGlob.x - 226, app.lpui->ptslotGlob.y - static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 - 6, 68, static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2,
                     win32::SWP_NOZORDER | win32::SWP_HIDEWINDOW);
        ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDCANCEL), nullptr, app.lpui->ptslotGlob.x - 148, app.lpui->ptslotGlob.y - static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 - 6, 68, static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2,
                     win32::SWP_NOZORDER);
        ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_HELP), nullptr, app.lpui->ptslotGlob.x - 74, app.lpui->ptslotGlob.y - static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 - 6, 68, static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2,
                     win32::SWP_NOZORDER);
        win32::SetDlgItemTextA(hwnd, IDCANCEL, TextIO().pfnCompressed(app, idsDone).data());
        if (app.state.gd.fTutorial) {
            PresentationIO().pfnTutor(app);
        }
        return 1;
    case win32::WM_DRAWITEM:
        lpdis = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
        if (lpdis->itemID == -1) {
            HandleFocusState(app, lpdis, -2);
        } else {
            switch (lpdis->itemAction) {
            case win32::ODA_DRAWENTIRE:
                DrawDlgLBEntireItem(app, lpdis, -4);
                break;
            case win32::ODA_SELECT:
                DrawDlgLBEntireItem(app, lpdis, -4);
                break;
            case win32::ODA_FOCUS:
                DrawDlgLBEntireItem(app, lpdis, -4);
            }
        }
        return 1;
    case win32::WM_MEASUREITEM:
        lpmis = reinterpret_cast<MEASUREITEMSTRUCT *>(lParam);
        lpmis->itemHeight = 66;
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        for (i = 2064; i <= 2069 && reinterpret_cast<HWND>(lParam) != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 2069 || reinterpret_cast<HWND>(lParam) == ControlIO().pfnGetDlgItem(hwnd, IDC_SHIPLIST)) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (win32::PtInRect(app.lpui->rgrcBuildSpin, PointFrom16(pt)) == 0 && win32::PtInRect(&app.lpui->rgrcBuildSpin[1], PointFrom16(pt)) == 0)
            break;
        win32::SetCursor(app.lpui->hcurHand);
        return 1;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        if (app.lpui->mdBuild != mdBuildEdit) {
            win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_SHIPS), &rcGBox);
            win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcGBox));
            win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_STARBASES), &rc);
            win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rc.right));
            rcGBox.right = rc.right;
            rcGBox.bottom = rc.bottom;
            ExpandRc(&rcGBox, app.state.dyArial8, app.state.dyArial8 >> 1);
            _Draw3dFrame(app, hdc, &rcGBox, -1);
            win32::SetBkColor(hdc, app.lpui->crButtonFace);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            cch = TextIO().pfnCchGetString(app, idsDesign, std::span{app.state.szWork});
            win32::TextOutA(hdc, rcGBox.left + 8, rcGBox.top - (app.state.dyArial8 >> 1), app.state.szWork, cch);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_EXISTING), &rcGBox);
            win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcGBox));
            win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_COMPONENTS), &rc);
            win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rc.right));
            rcGBox.right = rc.right;
            rcGBox.bottom = rc.bottom;
            ExpandRc(&rcGBox, app.state.dyArial8, app.state.dyArial8 >> 1);
            _Draw3dFrame(app, hdc, &rcGBox, -1);
            win32::SetBkColor(hdc, app.lpui->crButtonFace);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            cch = TextIO().pfnCchGetString(app, idsView, std::span{app.state.szWork});
            win32::TextOutA(hdc, rcGBox.left + 8, rcGBox.top - (app.state.dyArial8 >> 1), app.state.szWork, cch);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        }
        ControlIO().pfnGetClientRect(hwnd, &rc);
        DrawSlotDlg(app, hwnd, hdc, &rc, -1);
        DrawBuildSelComp(app, hwnd, hdc, -1);
        DrawBuildSelHull(app, hwnd, hdc, -1, nullptr);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_LBUTTONDOWN:
    case win32::WM_LBUTTONDBLCLK:
    case win32::WM_RBUTTONDOWN:
        return FTrackSlot(app, hwnd, LOWORD(lParam), HIWORD(lParam), wParam, 0, message == win32::WM_RBUTTONDOWN);
    case win32::WM_COMMAND:
        if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_DESIGNER_SHIPS && LOWORD(wParam) <= IDC_DESIGNER_STARBASES) {
            app.state.fStarbaseMode = LOWORD(wParam) - 2064;
            wParam = app.lpui->mdBuild + 2066;
            ControlIO().pfnGetClientRect(hwnd, &rc);
            rc.left = rc.right >> 1 >= rc.right - 352 ? rc.right - 352 : rc.right >> 1;
            ControlIO().pfnInvalidateRect(hwnd, &rc, 1);
            app.lpui->lpshdefBuild = nullptr;
            app.lpui->fHullCopy = 0;
            lSel = !app.state.fStarbaseMode ? static_cast<std::uint32_t>(rggrbitParts[0]) : static_cast<std::uint32_t>(rggrbitPartsSB[0]);
            FillBuildPartsLB(app, ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_COMPONENT_LIST), LOWORD(lSel));
            hwndItem = ControlIO().pfnGetDlgItem(hwnd, IDC_COMBOBOX);
            FillBuildDD(app, hwndItem, app.lpui->mdBuild);
            ControlIO().pfnSendMessage(hwndItem, win32::CB_SETCURSEL, 0, 0);
            goto FixupShip;
        } else if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_DESIGNER_EXISTING && LOWORD(wParam) <= IDC_DESIGNER_COMPONENTS) {
            lSel = 0;
        LRestart:
            app.lpui->lpshdefBuild = nullptr;
            app.lpui->fHullCopy = 0;
            app.lpui->mdBuild = static_cast<MdBuild>(LOWORD(wParam) - 2066);
            hwndItem = ControlIO().pfnGetDlgItem(hwnd, IDC_COMBOBOX);
            UpdateSlotGlobals(app);
            FillBuildDD(app, hwndItem, app.lpui->mdBuild);
            ControlIO().pfnSendMessage(hwndItem, win32::CB_SETCURSEL, LOWORD(lSel), 0);
            ControlIO().pfnSetWindowPos(hwndItem, nullptr, app.lpui->ptslotGlob.x - 256 - (LOWORD(wParam) == IDC_DESIGNER_COMPONENTS ? 0 : 8), 8, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER);
            ControlIO().pfnGetClientRect(hwnd, &rc);
            left = rc.left;
            rc.left = rc.right >> 1 >= rc.right - 352 ? rc.right - 352 : rc.right >> 1;
            ControlIO().pfnInvalidateRect(hwnd, &rc, 1);
            rc.right = rc.left;
            rc.left = left;
            rc.top = app.lpui->yBuildInfoSum;
            ControlIO().pfnInvalidateRect(hwnd, &rc, 1);
            hwndItem = ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_COMPONENT_LIST);
            ui::ShowWindow(hwndItem, app.lpui->mdBuild == mdBuildComp ? win32::SW_SHOW : win32::SW_HIDE);
            if (LOWORD(wParam) != IDC_DESIGNER_COMPONENTS)
                goto FixupShip;
            if (app.state.gd.fTutorial) {
                PresentationIO().pfnTutor(app);
            }
        } else if (LOWORD(wParam) == IDC_EDITNAME && HIWORD(wParam) == 0x400 && !app.lpui->fInEditUpdate) {
            app.lpui->fInEditUpdate = 1;
            ControlIO().pfnGetWindowText(reinterpret_cast<HWND>(lParam), app.state.szWork, 250);
            lSel = ControlIO().pfnSendMessage(reinterpret_cast<HWND>(lParam), win32::EM_GETSEL, 0, 0);
            if (!FStringFitsScreen(app, app.state.szWork, 160)) {
                ControlIO().pfnSetWindowText(reinterpret_cast<HWND>(lParam), app.state.szWork);
                ControlIO().pfnSendMessage(reinterpret_cast<HWND>(lParam), win32::EM_SETSEL, LOWORD(lSel), static_cast<std::int16_t>(HIWORD(lSel)));
            }
            win32::lstrcpyA(app.lpui->lpshdefBuild->hul.szClass, app.state.szWork);
            DrawBuildSelHull(app, hwnd, nullptr, 256, nullptr);
            app.lpui->fInEditUpdate = 0;
            if (app.state.gd.fTutorial) {
                PresentationIO().pfnTutor(app);
            }
        } else if (LOWORD(wParam) == IDC_COMBOBOX) {
            switch (HIWORD(wParam)) {
            case 1:
            FixupShip:
                hwndItem = ControlIO().pfnGetDlgItem(hwnd, IDC_COMBOBOX);
                lSel = ControlIO().pfnSendMessage(hwndItem, win32::CB_GETCURSEL, 0, 0);
                if (app.lpui->mdBuild == mdBuildComp || app.lpui->mdBuild == mdBuildEdit) {
                    if (lSel == -1) {
                        lSel = 0;
                    } else {
                        lSel = !app.state.fStarbaseMode ? static_cast<std::uint32_t>(rggrbitParts[lSel]) : static_cast<std::uint32_t>(rggrbitPartsSB[lSel]);
                    }
                    FillBuildPartsLB(app, ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_COMPONENT_LIST), LOWORD(lSel));
                } else if (app.lpui->mdBuild == mdBuildShdef || app.lpui->mdBuild == mdBuildHuldef || app.lpui->mdBuild == mdBuildEnemyShdef) {
                    if (lSel == -1)
                        goto LClearSelection;
                    if (app.lpui->mdBuild == mdBuildShdef) {
                        fProtoSB = app.state.fStarbaseMode != 0 && RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh;
                        app.lpui->lpshdefBuild = NthValidShdef(app, LOWORD(lSel));
                        CshQueued(app, app.lpui->lpshdefBuild->ishdef, &fProgress, 0);
                        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(app.lpui->hwndSlotDlg, IDC_EDIT), app.lpui->lpshdefBuild->cExist == 0 && !fProgress && (!fProtoSB || lSel > 0));
                        if (!fProtoSB)
                            goto LClearSelection;
                        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(app.lpui->hwndSlotDlg, IDC_DELETE), lSel > 0 && app.lpui->lpshdefBuild->cExist == 0);
                        goto LClearSelection;
                    } else if (app.lpui->mdBuild == mdBuildEnemyShdef) {
                        app.lpui->lpshdefBuild = NthValidEnemyShdef(app, LOWORD(lSel));
                        if (app.lpui->lpshdefBuild->det == detAll)
                            goto LClearSelection;
                        i = app.lpui->lpshdefBuild->hul.ihuldef;
                        goto LClearSelection;
                    } else {
                        if (app.state.fStarbaseMode) {
                            part.hs.grhst = hstSBHull;
                            for (i = 0; i < 5; i++) {
                                part.hs.iItem = i;
                                if (PartQueryIO().pfnLookup(app, &part) == mdPartAvailAvailable && lSel-- <= 0)
                                    break;
                            }
                            i += 32;
                        } else {
                            part.hs.grhst = hstHull;
                            for (i = 0; i < 32; i++) {
                                part.hs.iItem = i;
                                if (PartQueryIO().pfnLookup(app, &part) == mdPartAvailAvailable && lSel-- <= 0)
                                    break;
                            }
                        }
                        app.state.shdefBuild.hul = PartQueryIO().pfnHull(i)->hul;
                        app.state.shdefBuild.hul.ihuldef = static_cast<HulDef>(i & 0xff);
                        for (i = 0; i < app.state.shdefBuild.hul.chs; i++) {
                            app.state.shdefBuild.hul.rghs[i].cItem = 0;
                        }
                        app.lpui->lpshdefBuild = &app.state.shdefBuild;
                        UpdateShdefCost(app, app.lpui->lpshdefBuild);
                    }
                LClearSelection:
                    UpdateSlotGlobals(app);
                    ControlIO().pfnGetClientRect(hwnd, &rc);
                    rc.left = rc.right >> 1 >= rc.right - 352 ? rc.right - 352 : rc.right >> 1;
                    ControlIO().pfnInvalidateRect(hwnd, &rc, 1);
                }
                SetBuildSelection(app, -2);
                DrawBuildSelHull(app, hwnd, nullptr, -1, nullptr);
                if (!app.lpui->lpshdefBuild) {
                    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(app.lpui->hwndSlotDlg, IDC_DELETE), 0);
                    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(app.lpui->hwndSlotDlg, IDC_EDIT), 0);
                    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(app.lpui->hwndSlotDlg, IDC_IMPORT), 0);
                }
                if (app.state.gd.fTutorial) {
                    PresentationIO().pfnTutor(app);
                }
                break;
            }
        } else if (LOWORD(wParam) == IDC_DESIGNER_COMPONENT_LIST) {
            if (HIWORD(wParam) == 1) {
                SetBuildSelection(app, -1);
            }
        } else if (LOWORD(wParam) == IDC_DELETE) {
            if (HIWORD(wParam) != 0)
                break;
            fProgress = 0;
            cshQueued = 0;
            if (app.state.gd.fTutorial && !FTutorialEnabledShipBuilder(app, tutsbDelete))
                break;
            hwndItem = ControlIO().pfnGetDlgItem(hwnd, IDC_COMBOBOX);
            lSel = ControlIO().pfnSendMessage(hwndItem, win32::CB_GETCURSEL, 0, 0);
            lpshdef = NthValidShdef(app, LOWORD(lSel));
            if (lSel < 0 || !lpshdef)
                break;
            if (app.state.fStarbaseMode && lSel == 0 && RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh)
                break;
            if (!FCheckQueuedShip(app, hwnd, lpshdef, 0))
                break;
            lpshdef->fFree = 1;
            lpshdef->cBuilt = 0;
            lpshdef->cExist = 0;
            if (app.state.fStarbaseMode) {
                app.state.rgplr[app.state.idPlayer].cshdefSB += 15;
            } else {
                app.state.rgplr[app.state.idPlayer].cShDef--;
            }
            LogChangeShDef(app, lpshdef);
            FillBuildDD(app, hwndItem, app.lpui->mdBuild);
            app.lpui->lpshdefBuild = NthValidShdef(app, 0);
            if ((app.state.fStarbaseMode && RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh) || !app.lpui->lpshdefBuild) {
                ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(app.lpui->hwndSlotDlg, IDC_DELETE), 0);
                ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(app.lpui->hwndSlotDlg, IDC_EDIT), 0);
                if (!app.lpui->lpshdefBuild) {
                    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(app.lpui->hwndSlotDlg, IDC_IMPORT), 0);
                }
            }
            UpdateSlotGlobals(app);
            ControlIO().pfnGetClientRect(hwnd, &rc);
            rc.left = rc.right >> 1 >= rc.right - 352 ? rc.right - 352 : rc.right >> 1;
            ControlIO().pfnInvalidateRect(hwnd, &rc, 1);
            if (app.lpui->fHullCopy)
                goto LRestart;
        } else if (LOWORD(wParam) == IDC_IMPORT) {
            if (HIWORD(wParam) == 0) {
                if (app.state.gd.fTutorial && !FTutorialEnabledShipBuilder(app, tutsbCopy))
                    break;
                hwndItem = ControlIO().pfnGetDlgItem(hwnd, IDC_COMBOBOX);
                lSel = ControlIO().pfnSendMessage(hwndItem, win32::CB_GETCURSEL, 0, 0);
                if (lSel < 0)
                    break;
                if (app.state.fStarbaseMode) {
                    for (i = 0; i < 10 && app.state.rglpshdefSB[app.state.idPlayer][i].fFree == 0; i++) {
                    }
                } else {
                    for (i = 0; i < 16 && app.state.rgshdef[i].fFree == 0; i++) {
                    }
                }
                if (app.lpui->mdBuild == mdBuildShdef || app.lpui->mdBuild == mdBuildEnemyShdef) {
                    if (app.lpui->mdBuild == mdBuildShdef) {
                        lpshdef = NthValidShdef(app, LOWORD(lSel));
                        if (lpshdef->fGift)
                            goto LStripDown;
                    } else {
                        lpshdef = NthValidEnemyShdef(app, LOWORD(lSel));
                    LStripDown:
                        if (app.state.fStarbaseMode) {
                            part.hs.grhst = hstSBHull;
                            part.hs.iItem = lpshdef->hul.ihuldef - ihuldefOrbitalFort;
                        } else {
                            part.hs.grhst = hstHull;
                            part.hs.iItem = lpshdef->hul.ihuldef;
                        }
                        if (PartQueryIO().pfnLookup(app, &part) != mdPartAvailAvailable) {
                            AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsCantCopyShipDesignBecauseCantBuild, nullptr), win32::MB_ICONHAND);
                            return 0;
                        }
                    }
                    if (lSel < 0 || !lpshdef) {
                        return 0;
                    }
                    if (app.state.fStarbaseMode) {
                        app.state.rglpshdefSB[app.state.idPlayer][i] = *lpshdef;
                        lpshdef = app.state.rglpshdefSB[app.state.idPlayer] + i;
                    } else {
                        app.state.rgshdef[i] = *lpshdef;
                        lpshdef = &app.state.rgshdef[i];
                    }
                    lpshdef->cExist = 0;
                    lpshdef->cBuilt = 0;
                    if (app.lpui->mdBuild == mdBuildShdef && !lpshdef->fGift) {
                        MakeNewName(app, lpshdef->hul.szClass);
                    } else {
                        lpshdef->fGift = 0;
                        for (j = 0; j < lpshdef->hul.chs; j++) {
                            if (lpshdef->hul.rghs[j].cItem > 0) {
                                part.hs = lpshdef->hul.rghs[j];
                                if (PartQueryIO().pfnLookup(app, &part) != mdPartAvailAvailable) {
                                    lpshdef->hul.rghs[j].cItem = 0;
                                }
                            }
                        }
                    }
                } else {
                    if (app.state.fStarbaseMode) {
                        part.hs.grhst = hstSBHull;
                    } else {
                        part.hs.grhst = hstHull;
                    }
                    for (j = 0; j < (!app.state.fStarbaseMode ? 32 : 5); j++) {
                        part.hs.iItem = j;
                        if (PartQueryIO().pfnLookup(app, &part) == mdPartAvailAvailable && lSel-- <= 0)
                            break;
                    }
                    if (app.state.fStarbaseMode) {
                        lpshdef = app.state.rglpshdefSB[app.state.idPlayer] + i;
                        j += 32;
                    } else {
                        lpshdef = &app.state.rgshdef[i];
                    }
                    *lpshdef = {};
                    lpshdef->hul = PartQueryIO().pfnHull(j)->hul;
                    lpshdef->det = detAll;
                    std::fill_n(lpshdef->hul.rghs, 16, HS{});
                }
                win32::CheckRadioButton(hwnd, IDC_DESIGNER_EXISTING, IDC_DESIGNER_COMPONENTS, IDC_DESIGNER_EXISTING);
                lpshdef->turn = app.state.game.turn;
                lpshdef->ishdef = (!app.state.fStarbaseMode ? 0 : 16) + i;
                UpdateShdefCost(app, lpshdef);
                if (app.state.fStarbaseMode) {
                    app.state.rgplr[app.state.idPlayer].cshdefSB++;
                } else {
                    app.state.rgplr[app.state.idPlayer].cShDef++;
                }
                LogChangeShDef(app, lpshdef);
                FillBuildDD(app, hwndItem, app.lpui->mdBuild);
                ControlIO().pfnSendMessage(hwndItem, win32::CB_SETCURSEL, i, 0);
                ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(app.lpui->hwndSlotDlg, IDC_EDIT), 1);
                app.lpui->lpshdefBuild = lpshdef;
                UpdateSlotGlobals(app);
                app.lpui->fHullCopy = 1;
                goto EditDesign;
            } else if (app.state.gd.fTutorial) {
                PresentationIO().pfnTutor(app);
            }
        } else if (LOWORD(wParam) == IDC_EDIT) {
            if (app.state.gd.fTutorial && !FTutorialEnabledShipBuilder(app, tutsbEdit))
                break;
            if (app.state.fStarbaseMode && app.lpui->lpshdefBuild->ishdef == 16 && RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh)
                break;
            hwndItem = ControlIO().pfnGetDlgItem(hwnd, IDC_COMBOBOX);
            lSel = ControlIO().pfnSendMessage(hwndItem, win32::CB_GETCURSEL, 0, 0);
            lpshdef = NthValidShdef(app, LOWORD(lSel));
            if (lSel < 0 || !lpshdef)
                break;
            if (!app.state.fStarbaseMode && !FCheckQueuedShip(app, hwnd, lpshdef, 1))
                break;
        EditDesign:
            if (!app.lpui->lpshdefBuild)
                break;
            ControlIO().pfnInvalidateRect(hwnd, nullptr, 1);
            app.lpui->mdBuild = mdBuildEdit;
            app.lpui->ishdefBuild = app.lpui->lpshdefBuild->ishdef;
            app.state.shdefBuild = *app.lpui->lpshdefBuild;
            app.lpui->lpshdefBuild = &app.state.shdefBuild;
            FillBuildPartsLB(app, ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_COMPONENT_LIST), !app.state.fStarbaseMode ? 6655 : 2620);
            FillBuildDD(app, ControlIO().pfnGetDlgItem(hwnd, IDC_COMBOBOX), app.lpui->mdBuild);
            ShowMainControls(app, hwnd, win32::SW_HIDE);
            ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_COMPONENT_LIST), nullptr, 16, 32, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_SHOWWINDOW);
            ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_COMBOBOX), nullptr, 16, 8, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_SHOWWINDOW);
            ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_EDITNAME), nullptr, app.lpui->ptslotGlob.x - 264, 8, 240, 3 * app.state.dyArial8 >> 1, win32::SWP_NOZORDER | win32::SWP_SHOWWINDOW);
            ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_EDITNAME), app.state.shdefBuild.hul.szClass);
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_EDITNAME), win32::EM_LIMITTEXT, 0x1f, 0);
            if (app.state.gd.fTutorial) {
                PresentationIO().pfnTutor(app);
            }
        } else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            if (app.lpui->mdBuild == mdBuildEdit) {
                lSel = 0;
                if (LOWORD(wParam) == IDOK) {
                    if (!app.state.fStarbaseMode && app.state.shdefBuild.hul.rghs[0].cItem == 0) {
                        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsShipDesignDoesHaveAnyEnginesMust, nullptr), win32::MB_ICONHAND);
                        return 0;
                    }
                    if (app.state.gd.fTutorial && !FTutorialEnabledShipBuilder(app, tutsbAccept)) {
                        break;
                    }
                    ControlIO().pfnGetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_EDITNAME), app.state.shdefBuild.hul.szClass, 32);
                    app.state.shdefBuild.cBuilt = 0;
                    app.state.shdefBuild.cExist = 0;
                    app.state.shdefBuild.fFree = 0;
                    UpdateShdefCost(app, &app.state.shdefBuild);
                    if (app.state.fStarbaseMode) {
                        app.lpui->ishdefBuild -= 16;
                        app.state.rglpshdefSB[app.state.idPlayer][app.lpui->ishdefBuild] = app.state.shdefBuild;
                        LogChangeShDef(app, &app.state.shdefBuild);
                        for (i = 0; i < app.lpui->ishdefBuild; i++) {
                            if (!app.state.rglpshdefSB[app.state.idPlayer][i].fFree) {
                                lSel++;
                            }
                        }
                        app.lpui->ishdefBuild += 16;
                    } else {
                        app.state.rgshdef[app.lpui->ishdefBuild] = app.state.shdefBuild;
                        LogChangeShDef(app, &app.state.rgshdef[app.lpui->ishdefBuild]);
                        for (i = 0; i < app.lpui->ishdefBuild; i++) {
                            if (!app.state.rgshdef[i].fFree) {
                                lSel++;
                            }
                        }
                    }
                } else if (app.state.gd.fTutorial && !FTutorialEnabledShipBuilder(app, tutsbCancelEdit)) {
                    break;
                }
                ControlIO().pfnInvalidateRect(hwnd, nullptr, 1);
                ShowMainControls(app, hwnd, win32::SW_SHOW);
                ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_EDITNAME), win32::SW_HIDE);
                hwndItem = ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_COMPONENT_LIST);
                FillBuildPartsLB(app, hwndItem, !app.state.fStarbaseMode ? rggrbitParts[0] : rggrbitPartsSB[0]);
                ControlIO().pfnSetWindowPos(hwndItem, nullptr, app.lpui->ptslotGlob.x - 256, 32, 240, 266, win32::SWP_NOZORDER);
                ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DESIGNER_COMPONENT_LIST), win32::SW_HIDE);
                if (app.lpui->fHullCopy) {
                    if (LOWORD(wParam) != IDCANCEL && !app.state.fStarbaseMode && app.lpui->lpshdefBuild->hul.rghs[0].cItem == 0) {
                        wParam = IDCANCEL;
                    }
                    if (LOWORD(wParam) == IDCANCEL) {
                        if (app.state.fStarbaseMode) {
                            app.state.shdefBuild.fFree = 1;
                            app.state.rglpshdefSB[app.state.idPlayer][app.lpui->ishdefBuild - 16] = app.state.shdefBuild;
                            app.state.rgplr[app.state.idPlayer].cshdefSB += 15;
                            LogChangeShDef(app, &app.state.shdefBuild);
                        } else {
                            app.state.rgshdef[app.lpui->ishdefBuild].fFree = 1;
                            app.state.rgplr[app.state.idPlayer].cShDef--;
                            LogChangeShDef(app, &app.state.rgshdef[app.lpui->ishdefBuild]);
                        }
                    }
                }
                wParam = IDC_DESIGNER_EXISTING;
                goto LRestart;
            } else {
                SetBuildSelection(app, -2);
                ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickySlotDlg, 0);
                app.lpui->hwndSlotDlg = nullptr;
                ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
                if (app.state.gd.fTutorial) {
                    PresentationIO().pfnTutor(app);
                }
                return 1;
            }
        } else if (LOWORD(wParam) == IDC_HELP) {
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, static_cast<std::uint32_t>((app.lpui->mdBuild == mdBuildEdit ? 3039 : 1066)));
            return 1;
        }
        break;
    }
    return 0;
}

void DrawSlotDlg(APPSESSION &app, HWND hwnd, HDC hdc, RECT *prc, std::int16_t iDraw) {
    std::int16_t  yTop;
    std::int16_t  iMax;
    std::int16_t  cSlot;
    std::int16_t  fCreatedDC;
    HDC      hdcMem;
    std::int16_t  c;
    std::int16_t  i;
    std::int16_t  bkMode;
    std::int16_t  j;
    std::int16_t  cItem;
    std::int16_t  ibmp;
    HBITMAP  hbmpSav;
    std::int16_t  xLeft;
    PART     part;
    HULDEF  *lphuldef;
    RECT     rc;
    std::int16_t  iInventSel;
    HPEN     hpenSav;
    HBRUSH   hbrSav;
    COLORREF crBkSav;

    fCreatedDC = 0;
    if (app.lpui->mdBuild != mdBuildComp && app.lpui->lpshdefBuild) {
        lphuldef = PartQueryIO().pfnHull(app.lpui->lpshdefBuild->hul.ihuldef);
        cSlot = lphuldef->hul.chs;
        if (!hdc) {
            fCreatedDC = 1;
            hdc = win32::GetDC(hwnd);
        }
        if (!app.lpui->hwndSlotDlg) {
            xLeft = 4;
            yTop = 6;
        } else {
            xLeft = app.lpui->ptslotGlob.x - 338;
            yTop = 6;
        }
        DrawFleetBitmap(app, nullptr, hdc, xLeft, yTop, 1, app.lpui->lpshdefBuild->hul.ibmp, 0, 0, -1, 0);
        hdcMem = win32::CreateCompatibleDC(hdc);
        hbmpSav = static_cast<HBITMAP>(win32::SelectObject(hdcMem, app.lpui->hbmpScanner));
        iInventSel = 0;
        if (app.lpui->mdBuild != mdBuildEdit) {
            app.lpui->rgrcBuildSpin[1].top = -5;
            app.lpui->rgrcBuildSpin[0].top = -5;
            app.lpui->rgrcBuildSpin[1].bottom = -6;
            app.lpui->rgrcBuildSpin[0].bottom = -6;
        } else {
            win32::SetRect(app.lpui->rgrcBuildSpin, xLeft + 21, yTop + 69, xLeft + 35, yTop + 83);
            app.lpui->rgrcBuildSpin[1] = app.lpui->rgrcBuildSpin[0];
            win32::OffsetRect(&app.lpui->rgrcBuildSpin[1], 14, 0);
            for (i = 0; i < 2; i++) {
                DrawBtn(app, hdc, &app.lpui->rgrcBuildSpin[i], (i == 0 ? 2 : 3) | 0x20, 0, nullptr);
            }
        }
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        bkMode = win32::SetBkMode(hdc, win32::TRANSPARENT);
        if (iDraw == -1) {
            i = 0;
            iMax = cSlot;
        } else {
            i = iDraw;
            iMax = iDraw + 1;
        }
        if (lphuldef->hul.wtCargoMax != 0) {
            rc = app.lpui->rcCargo;
            if (app.state.fStarbaseMode && (lphuldef->hul.ihuldef == ihuldefSpaceDock || lphuldef->hul.ihuldef == ihuldefDeathStart)) {
                hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrDock));
                hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, win32::GetStockObject(win32::BLACK_PEN)));
                win32::Ellipse(hdc, rc.left - 12, rc.top - 12, rc.right + 12, rc.bottom + 12);
                win32::SelectObject(hdc, hpenSav);
                win32::SelectObject(hdc, hbrSav);
            } else {
                win32::FillRect(hdc, &app.lpui->rcCargo, static_cast<HBRUSH>(win32::GetStockObject(win32::BLACK_BRUSH)));
                rc.top++;
                rc.bottom--;
                rc.left++;
                rc.right--;
                win32::FillRect(hdc, &rc, !app.state.fStarbaseMode ? app.lpui->hbrCargo : app.lpui->hbrDock);
            }
            rc.bottom = static_cast<std::int16_t>((rc.bottom - rc.top)) / 2 + rc.top;
            if (!app.state.fStarbaseMode) {
                RcCtrTextOut(app, hdc, &rc, TextIO().pfnCompressed(app, idsCargo3).data(), 0);
                c = win32::wsprintfA(app.state.szWork, app.lpui->PCTDKT, WtMaxShdefStat(app.lpui->lpshdefBuild, 2));
                RcCtrTextOut(app, hdc, &app.lpui->rcCargo, app.state.szWork, 0);
            } else {
                if (static_cast<std::uint32_t>(lphuldef->hul.wtCargoMax) == 0xffff) {
                    RcCtrTextOut(app, hdc, &rc, TextIO().pfnCompressed(app, idsUnlimited).data(), 0);
                } else {
                    c = win32::wsprintfA(app.state.szWork, app.lpui->PCTDKT, lphuldef->hul.wtCargoMax);
                    RcCtrTextOut(app, hdc, &rc, app.state.szWork, 0);
                }
                RcCtrTextOut(app, hdc, &app.lpui->rcCargo, TextIO().pfnCompressed(app, idsSpace).data(), 0);
            }
            rc.top = rc.bottom;
            rc.bottom = app.lpui->rcCargo.bottom - 1;
            if (app.state.fStarbaseMode) {
                RcCtrTextOut(app, hdc, &rc, TextIO().pfnCompressed(app, idsDock).data(), 0);
            } else {
                RcCtrTextOut(app, hdc, &rc, TextIO().pfnCompressed(app, idsMax).data(), 0);
            }
        }
        for (; i < iMax; i++) {
            win32::FillRect(hdc, &app.lpui->vrgrcSlot[i], app.lpui->hbr50Screen);
            cItem = app.lpui->lpshdefBuild->hul.rghs[i].cItem;
            if (cItem > 0) {
                part.hs = app.lpui->lpshdefBuild->hul.rghs[i];
                PartQueryIO().pfnLookup(app, &part);
                ibmp = part.pcom().ibmp();
                iInventSel = ibmp >> 5;
                DibBlt(hdc, app.lpui->vrgrcSlot[i].left, app.lpui->vrgrcSlot[i].top, 64, 64, app.lpui->rghdibInventory[iInventSel], (ibmp & 7) * 0x40, (3 - (ibmp >> 3 & 3)) * 0x40, 64, 64,
                       13369376);
                if (app.lpui->mdBuild != mdBuildEdit) {
                    win32::SelectObject(hdcMem, app.lpui->hbmpScanner);
                    for (j = 0; j < 4; j++) {
                        win32::BitBlt(hdc, (j >= 2 ? 57 : 3) + app.lpui->vrgrcSlot[i].left, (!(j & 1) ? 57 : 3) + app.lpui->vrgrcSlot[i].top, 4, 4, hdcMem, 22, 33, win32::SRCCOPY);
                    }
                }
                if (cItem == 1 && lphuldef->hul.rghs[i].cItem == 1 && part.hs.grhst == hstSpecialSB) {
                    app.state.szWork[0] = 0;
                    c = 0;
                } else {
                    c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDD).data(), cItem, lphuldef->hul.rghs[i].cItem);
                }
            } else {
                win32::SelectObject(hdcMem, app.lpui->hbmpBackBld);
                ibmp = IEmptyBmpFromGrhst(app, lphuldef->hul.rghs[i].grhst);
                win32::BitBlt(hdc, app.lpui->vrgrcSlot[i].left, app.lpui->vrgrcSlot[i].top, 64, 64, hdcMem, (ibmp & 7) * 0x40, (ibmp >> 3 & 3) * 0x40, win32::SRCCOPY);
                iInventSel = -1;
                if (lphuldef->hul.rghs[i].grhst & hstEngine) {
                    c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsNeedsD).data(), lphuldef->hul.rghs[i].cItem);
                } else {
                    c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsD3).data(), lphuldef->hul.rghs[i].cItem);
                }
            }
            CtrTextOut(hdc, app.lpui->vrgrcSlot[i].left + 32, app.lpui->vrgrcSlot[i].bottom - app.lpui->dyArial6 - 4, app.state.szWork, c);
            if (app.lpui->iselSlot == i) {
                crBkSav = win32::SetBkColor(hdc, 0xffffff);
                win32::FrameRect(hdc, &app.lpui->vrgrcSlot[i], app.lpui->hbr50Screen);
                ExpandRc(&app.lpui->vrgrcSlot[i], -1, -1);
                win32::FrameRect(hdc, &app.lpui->vrgrcSlot[i], app.lpui->hbr50Screen);
                ExpandRc(&app.lpui->vrgrcSlot[i], 1, 1);
                win32::SetBkColor(hdc, crBkSav);
            }
        }
        if ((!app.lpui->hwndPopup || !app.lpui->GlobalPD.fHideCounts) && app.lpui->mdBuild == mdBuildShdef) {
            win32::SetRect(&rc, app.lpui->ptPlaque.x, app.lpui->ptPlaque.y, app.lpui->ptPlaque.x + 60, app.lpui->ptPlaque.y + 30);
            win32::SelectPalette(hdc, app.lpui->vhpal, 0);
            win32::RealizePalette(hdc);
            DibBlt(hdc, app.lpui->ptPlaque.x, app.lpui->ptPlaque.y, 60, 30, app.lpui->hdibPlaque, 0, 0, 60, 30, 13369376);
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsLdLd).data(), app.lpui->lpshdefBuild->cExist, app.lpui->lpshdefBuild->cBuilt);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            if (LOWORD(GetTextExtent(hdc, app.state.szWork, c)) > 50) {
                win32::SelectObject(hdc, app.lpui->rghfontArial7[0]);
                if (LOWORD(GetTextExtent(hdc, app.state.szWork, c)) > 50) {
                    win32::SelectObject(hdc, app.lpui->rghfontArial6[0]);
                }
            }
            RcCtrTextOut(app, hdc, &rc, app.state.szWork, c);
        }
        win32::SetBkMode(hdc, bkMode);
        win32::SelectObject(hdcMem, hbmpSav);
        win32::DeleteDC(hdcMem);
        if (fCreatedDC) {
            win32::ReleaseDC(hwnd, hdc);
        }
    }
    return;
}

std::int16_t FTrackSlot(APPSESSION &app, HWND hwnd, std::int16_t x, std::int16_t y, std::int16_t fkb, std::int16_t fListBox, std::int16_t fRightBtn) {
    HDC     hdc;
    POINT16 ptOld;
    POINT16 ptTileSize;
    std::int16_t ibmpY;
    POINT16 pt;
    std::int16_t cSlot;
    std::int16_t iSrc;
    POINT16 ptDNew;
    std::int16_t ibmpX;
    HDC     hdcMem;
    std::int16_t i;
    HBITMAP hbmpFullSav;
    RECT    rcStart;
    std::int16_t fUseMem;
    HBITMAP hbmpScreen;
    std::int16_t ibmp;
    HBITMAP hbmpOld;
    HDC     hdcMemFull;
    POINT16 ptD;
    std::int16_t iSel;
    HBITMAP hbmpSav;
    HS      hs;
    std::int16_t fFirst;
    PART    part;
    RECT    rc;
    std::int16_t iDir;
    std::int16_t yTop;
    std::int16_t bt;
    BTNT    btnt;
    RECT   *prc;
    std::int16_t iBase;
    std::int16_t iCur;
    std::int16_t xLeft;
    std::int16_t dyStart;
    std::int16_t dxStart;

    fFirst = 1;
    if (!app.lpui->lpshdefBuild) {
        return 0;
    }
    pt.x = x;
    pt.y = y;
    cSlot = PartQueryIO().pfnHull(app.lpui->lpshdefBuild->hul.ihuldef)->hul.chs;
    if (!fRightBtn && app.lpui->hwndSlotDlg && (win32::PtInRect(app.lpui->rgrcBuildSpin, PointFrom16(pt)) != 0 || win32::PtInRect(&app.lpui->rgrcBuildSpin[1], PointFrom16(pt)) != 0)) {
        if (win32::PtInRect(app.lpui->rgrcBuildSpin, PointFrom16(pt)) != 0) {
            iDir = -1;
            bt = 34;
            prc = app.lpui->rgrcBuildSpin;
        } else {
            iDir = 1;
            bt = 35;
            prc = &app.lpui->rgrcBuildSpin[1];
        }
        iBase = app.lpui->lpshdefBuild->hul.ibmp;
        iCur = iBase & 3;
        iBase -= iCur;
        xLeft = app.lpui->ptslotGlob.x - 336;
        yTop = 8;
        InitBtnTrack(&btnt, hwnd, nullptr, prc, bt, 80, 0, 0, nullptr);
        while (FTrackBtn(app, &btnt)) {
            iCur = (iCur + 4 + iDir) & 3;
            DrawFleetBitmap(app, nullptr, btnt.hdc, xLeft, yTop, 0, iBase + iCur, 0, 0, -1, 0);
        }
        app.lpui->lpshdefBuild->hul.ibmp = iBase + iCur;
        return 1;
    }
    if (!fListBox) {
        ControlIO().pfnGetClientRect(hwnd, &rc);
        for (iSrc = 0; iSrc < cSlot && win32::PtInRect(&app.lpui->vrgrcSlot[iSrc], PointFrom16(pt)) == 0; iSrc++) {
        }
        if (iSrc == cSlot) {
            return 0;
        }
        hs = app.lpui->lpshdefBuild->hul.rghs[iSrc];
        if (hs.cItem <= 0) {
            SetBuildSelection(app, iSrc);
            return 0;
        }
        part.hs = hs;
        PartQueryIO().pfnLookup(app, &part);
        ibmp = part.pcom().ibmp();
        rcStart = app.lpui->vrgrcSlot[iSrc];
    } else {
        iSel = LOWORD(ControlIO().pfnSendMessage(hwnd, win32::LB_GETCURSEL, 0, 0));
        if (iSel == -1) {
            return 0;
        }
        ControlIO().pfnSendMessage(hwnd, win32::LB_GETTEXT, iSel, reinterpret_cast<LPARAM>(app.state.szWork));
        ibmp = app.state.szWork[2] - 'A' + (app.state.szWork[3] - 'A') * 26;
        iSrc = -1;
        hs.grhst = static_cast<HullSlotType>(1 << (app.state.szWork[0] - 'A'));
        hs.iItem = app.state.szWork[1] - 'A';
        hs.cItem = 1;
        rcStart.left = 2;
        rcStart.right = 66;
        rcStart.top = y / 66 * 66 + 1;
        rcStart.bottom = rcStart.top + 64;
        win32::ClientToScreen(hwnd, reinterpret_cast<POINT *>(&rcStart));
        win32::ClientToScreen(hwnd, reinterpret_cast<POINT *>(&rcStart.right));
        ClientToScreen16(hwnd, &pt);
        hwnd = app.lpui->hwndSlotDlg;
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcStart));
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcStart.right));
        ScreenToClient16(hwnd, &pt);
        x = pt.x;
        y = pt.y;
        if (hs.grhst == hstEngine && hs.iItem == iengineSettlersDelight && app.lpui->lpshdefBuild->hul.ihuldef != ihuldefMiniColonyShip) {
            app.lpui->GlobalPD.grPopup = grPopupString;
            app.lpui->GlobalPD.dxOut = 180;
            app.lpui->GlobalPD.psz = app.lpui->szPopupBuffer;
            TextIO().pfnCchGetString(app, idsSettlersDelightEngineMayMountedDesignsBased, std::span{app.lpui->szPopupBuffer});
            Popup(app, hwnd, x, y);
            return 1;
        }
        if (hs.grhst == hstSpecialM && hs.iItem == ispecialMOrbitalConstructionModule && app.lpui->lpshdefBuild->hul.ihuldef != ihuldefColonyShip) {
            app.lpui->GlobalPD.grPopup = grPopupString;
            app.lpui->GlobalPD.dxOut = 180;
            app.lpui->GlobalPD.psz = app.lpui->szPopupBuffer;
            TextIO().pfnCchGetString(app, idsOrbitalConstructionModuleMayMountedDesignsBased, std::span{app.lpui->szPopupBuffer});
            Popup(app, hwnd, x, y);
            return 1;
        }
        if (hs.grhst == hstSpecialE && hs.iItem == ispecialETransportCloaking && PartQueryIO().pfnHull(app.lpui->lpshdefBuild->hul.ihuldef)->imdAttack != hullAttackNone) {
            app.lpui->GlobalPD.grPopup = grPopupString;
            app.lpui->GlobalPD.dxOut = 180;
            app.lpui->GlobalPD.psz = app.lpui->szPopupBuffer;
            TextIO().pfnCchGetString(app, idsTransportCloakingModuleMayPlacedHullCould, std::span{app.lpui->szPopupBuffer});
            Popup(app, hwnd, x, y);
            return 1;
        }
    }
    SetBuildSelection(app, iSrc);
    if (fRightBtn) {
        app.lpui->GlobalPD.part = part;
        app.lpui->GlobalPD.grPopup = grPopupComponent;
        Popup(app, hwnd, x, y);
        return 1;
    }
    if (app.lpui->mdBuild != mdBuildEdit) {
        return 0;
    }
    ptTileSize.y = 64;
    ptTileSize.x = 64;
    ibmpX = ibmp & 7;
    ibmpY = ibmp >> 3 & 3;
    hdc = win32::GetDC(hwnd);
    win32::SelectPalette(hdc, app.lpui->vhpal, 0);
    win32::RealizePalette(hdc);
    hdcMem = win32::CreateCompatibleDC(hdc);
    hdcMemFull = win32::CreateCompatibleDC(hdc);
    win32::SelectPalette(hdcMemFull, app.lpui->vhpal, 0);
    win32::RealizePalette(hdcMemFull);
    hbmpOld = win32::CreateCompatibleBitmap(hdc, ptTileSize.x, ptTileSize.y);
    hbmpSav = static_cast<HBITMAP>(win32::SelectObject(hdcMem, hbmpOld));
    hbmpScreen = win32::CreateCompatibleBitmap(hdc, 3 * ptTileSize.x, 3 * ptTileSize.y);
    hbmpFullSav = static_cast<HBITMAP>(win32::SelectObject(hdcMemFull, hbmpScreen));
    win32::SetCapture(hwnd);
    ptOld.y = -1;
    ptOld.x = -1;
    while (FGetMouseMove(&pt)) {
        if (pt.x != ptOld.x || pt.y != ptOld.y) {
            if (fFirst) {
                fUseMem = 0;
                fFirst = 0;
            } else {
                win32::SelectObject(hdcMem, hbmpOld);
                ptDNew.x = pt.x - ptOld.x;
                ptDNew.y = pt.y - ptOld.y;
                fUseMem = std::abs(ptDNew.x) < ptTileSize.x && std::abs(ptDNew.y) < ptTileSize.y;
                if (!fUseMem) {
                    win32::BitBlt(hdc, rcStart.left + ptD.x, rcStart.top + ptD.y, ptTileSize.x, ptTileSize.y, hdcMem, 0, 0, win32::SRCCOPY);
                } else {
                    win32::BitBlt(hdcMemFull, 0, 0, 3 * ptTileSize.x, 3 * ptTileSize.y, hdc, rcStart.left + ptD.x - ptTileSize.x, rcStart.top + ptD.y - ptTileSize.y,
                           win32::SRCCOPY);
                    win32::BitBlt(hdcMemFull, ptTileSize.x, ptTileSize.y, ptTileSize.x, ptTileSize.y, hdcMem, 0, 0, win32::SRCCOPY);
                }
            }
            ptOld = pt;
            ptD.x = pt.x - x;
            ptD.y = pt.y - y;
            win32::SelectObject(hdcMem, hbmpOld);
            if (!fUseMem) {
                win32::BitBlt(hdcMem, 0, 0, ptTileSize.x, ptTileSize.y, hdc, rcStart.left + ptD.x, rcStart.top + ptD.y, win32::SRCCOPY);
                DibBlt(hdc, rcStart.left + ptD.x, rcStart.top + ptD.y, 64, 64, app.lpui->rghdibInventory[ibmp / 32], ibmpX * 64, (3 - ibmpY) * 64, 64, 64, 13369376);
            } else {
                dxStart = 0;
                dyStart = 0;
                win32::BitBlt(hdcMem, 0, 0, ptTileSize.x, ptTileSize.y, hdcMemFull, ptTileSize.x + ptDNew.x, ptTileSize.y + ptDNew.y, win32::SRCCOPY);
                DibBlt(hdcMemFull, ptTileSize.x + ptDNew.x, ptTileSize.y + ptDNew.y, 64, 64, app.lpui->rghdibInventory[ibmp / 32], ibmpX * 64, (3 - ibmpY) * 64, 64, 64,
                       13369376);
                rc = rcStart;
                OffsetRc(&rc, ptD.x, ptD.y);
                if (ptDNew.x > 0) {
                    rc.left -= ptDNew.x;
                    dxStart = ptDNew.x;
                } else {
                    rc.right -= ptDNew.x;
                }
                if (ptDNew.y > 0) {
                    rc.top -= ptDNew.y;
                    dyStart = ptDNew.y;
                } else {
                    rc.bottom -= ptDNew.y;
                }
                win32::BitBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hdcMemFull, ptTileSize.x + ptDNew.x - dxStart,
                       ptTileSize.y + ptDNew.y - dyStart, win32::SRCCOPY);
            }
            i = IDropPart(app, pt, hs, iSrc, 1);
            if (i < 0 || i == 2) {
                win32::SetCursor(win32::LoadCursorA(nullptr, win32::MAKEINTRESOURCE(32512)));
            } else if (i == 1 && iSrc >= 0) {
                win32::SetCursor(app.lpui->hcurTrashCan);
            } else {
                win32::SetCursor(app.lpui->hcurNoWay);
            }
        }
    }
    if (!fFirst) {
        win32::SelectObject(hdcMem, hbmpOld);
        win32::BitBlt(hdc, rcStart.left + ptD.x, rcStart.top + ptD.y, 64, 64, hdcMem, 0, 0, win32::SRCCOPY);
    }
    win32::ReleaseCapture();
    win32::SelectObject(hdcMem, hbmpSav);
    win32::SelectObject(hdcMemFull, hbmpFullSav);
    win32::DeleteObject(hbmpOld);
    win32::DeleteObject(hbmpScreen);
    win32::DeleteDC(hdcMem);
    win32::DeleteDC(hdcMemFull);
    i = IDropPart(app, pt, hs, iSrc, 0);
    win32::ReleaseDC(hwnd, hdc);
    return 1;
}

void DrawBuildSelComp(APPSESSION &app, HWND hwnd, HDC hdc, std::int16_t iDraw) {
    std::uint16_t grhst;
    HS       hsShip;
    std::uint16_t rgCosts[4];
    std::int16_t  fCreatedDC;
    std::int16_t  c;
    std::int16_t  i;
    COLORREF crForeSav;
    std::int16_t  fPlural;
    std::int16_t  k;
    char     szWord[80];
    COLORREF crBackSav;
    HS       hsHul;
    std::int16_t  cch;
    PART     part;
    std::int16_t  x;
    std::int16_t  dxkT;
    RECT     rc;
    std::int16_t  iSel;
    char    *pch;
    std::int16_t  yTop; /* NATIVE: RECT.top is 32-bit; WrapTextOut takes int16_t * */

    fCreatedDC = 0;
    if (!hdc) {
        fCreatedDC = 1;
        hdc = win32::GetDC(hwnd);
    }
    ControlIO().pfnGetClientRect(app.lpui->hwndSlotDlg, &rc);
    rc.bottom -= 8;
    rc.left = 8;
    rc.right = rc.left + 256;
    rc.top = app.lpui->yBuildInfoSum;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    crForeSav = win32::SetTextColor(hdc, 0);
    crBackSav = win32::SetBkColor(hdc, app.lpui->crButtonFace);
    win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
    if (app.state.fStarbaseMode) {
        rc.top += app.state.dyArial8;
    }
    if (app.lpui->iselSlot != -2) {
        if (app.lpui->iselSlot == -1) {
            iSel = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(app.lpui->hwndSlotDlg, IDC_DESIGNER_COMPONENT_LIST), win32::LB_GETCURSEL, 0, 0));
            if (iSel == -1)
                goto Restore;
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(app.lpui->hwndSlotDlg, IDC_DESIGNER_COMPONENT_LIST), win32::LB_GETTEXT, iSel, reinterpret_cast<LPARAM>(app.state.szWork));
            hsShip.cItem = 1;
            hsShip.grhst = static_cast<HullSlotType>(1 << (app.state.szWork[0] - 'A'));
            hsShip.iItem = app.state.szWork[1] - 'A';
            goto HullPart;
        } else {
            if (!app.lpui->lpshdefBuild)
                goto Restore;
            hsShip = app.lpui->lpshdefBuild->hul.rghs[app.lpui->iselSlot];
            hsHul = PartQueryIO().pfnHull(app.lpui->lpshdefBuild->hul.ihuldef)->hul.rghs[app.lpui->iselSlot];
            if (hsShip.cItem == 0) {
                i = TextIO().pfnCchGetString(app, !(hsHul.grhst & hstEngine) ? idsCanHold : idsRequiresExactly, std::span{app.state.szWork});
                fPlural = hsHul.cItem != 1;
                if (!fPlural) {
                    i += TextIO().pfnCchGetString(app, idsOne, std::span{app.state.szWork}.subspan(static_cast<std::size_t>(i)));
                } else {
                    i += win32::wsprintfA(std::span{app.state.szWork}.subspan(static_cast<std::size_t>(i)), "%d ", hsHul.cItem);
                }
                grhst = hsHul.grhst;
                while (grhst != 0) {
                    for (i = 0; i < 14; i++) {
                        if ((grhst & rghstCat[i]) == rghstCat[i]) {
                            grhst &= ~rghstCat[i];
                            break;
                        }
                    }
                    cch = TextIO().pfnCchGetString(app, rgidsCat[i], std::span{szWord});
                    if (!fPlural) {
                        if (szWord[cch - 1] == 's') {
                            if (szWord[cch - 2] == 'e' && szWord[cch - 3] == 'o') {
                                szWord[cch - 2] = 0;
                            } else {
                                szWord[cch - 1] = 0;
                            }
                        } else {
                            for (pch = &szWord[cch - 1]; pch > szWord && *pch != '('; pch--) {
                            }
                            if (*pch == '(' && pch > &szWord[1] && pch[-1] == ' ' && pch[-2] == 's') {
                                std::strcpy(pch + -2, pch + -1);
                            }
                        }
                    }
                    if (grhst != 0) {
                        if ((grhst - 1) & grhst) {
                            CchAppendUtf8Prefix(std::span{szWord}, ", ");
                        } else {
                            CchAppendUtf8Prefix(std::span{szWord}, TextIO().pfnCompressed(app, idsOr).data());
                        }
                    }
                    CchAppendUtf8Prefix(std::span{app.state.szWork}, szWord);
                }
                x = rc.left;
                /* NATIVE: original passed &rc.top */
                yTop = rc.top;
                WrapTextOut(app, hdc, &x, &yTop, app.state.szWork, 0, rc.left, rc.right - rc.left, nullptr, 0, 1);
                rc.top = yTop;
                rc.top += app.state.dyArial8;
                goto Restore;
            }
        }
    HullPart:
        part.hs = hsShip;
        PartQueryIO().pfnLookup(app, &part);
        dxkT = LOWORD(GetTextExtent(hdc, TextIO().pfnCompressed(app, idsKt).data(), 2));
        fPlural = hsShip.cItem != 1;
        if (!fPlural) {
            TextIO().pfnCchGetString(app, idsOne, std::span{szWord});
        } else {
            win32::wsprintfA(szWord, "%d ", hsShip.cItem);
        }
        CchAppendUtf8Prefix(std::span{szWord}, part.pcom().szName());
        if (fPlural) {
            CchAppendUtf8Prefix(std::span{szWord}, "s");
        }
        cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsCostS).data(), szWord);
        win32::TextOutA(hdc, rc.left, rc.top, app.state.szWork, cch);
        rc.left += 8;
        rc.right -= 8;
        c = hsShip.cItem;
        PartQueryIO().pfnTrueCost(app, app.state.idPlayer, &part, rgCosts);
        for (k = 0; k < 3; k++) {
            rc.top += app.state.dyArial8;
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            win32::SetTextColor(hdc, app.lpui->rgcrMinerals[k]);
            win32::TextOutA(hdc, rc.left, rc.top, app.state.rgszMinerals[k], win32::lstrlenA(app.state.rgszMinerals[k]));
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            win32::SetTextColor(hdc, app.lpui->crWindowText);
            cch = win32::wsprintfA(app.state.szWork, app.state.PCTLD, static_cast<std::uint32_t>((c * static_cast<std::uint32_t>(rgCosts[k]))));
            RightTextOut(app, hdc, rc.right - dxkT - 64, rc.top, app.state.szWork, cch, app.lpui->dxMaxMineralQuan);
            win32::TextOutA(hdc, rc.right - dxkT - 64, rc.top, TextIO().pfnCompressed(app, idsKt).data(), 2);
        }
        rc.top += app.state.dyArial8;
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        win32::SetTextColor(hdc, app.lpui->rgcrMinerals[5]);
        win32::TextOutA(hdc, rc.left, rc.top, app.state.rgszMinerals[5], win32::lstrlenA(app.state.rgszMinerals[5]));
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        win32::SetTextColor(hdc, app.lpui->crWindowText);
        cch = win32::wsprintfA(app.state.szWork, app.state.PCTLD, static_cast<std::uint32_t>((c * static_cast<std::uint32_t>(rgCosts[3]))));
        RightTextOut(app, hdc, rc.right - dxkT - 64, rc.top, app.state.szWork, cch, app.lpui->dxMaxMineralQuan);
        if (!app.state.fStarbaseMode) {
            rc.left -= 8;
            rc.top += app.state.dyArial8;
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsMassLdkt).data(), static_cast<std::uint32_t>((c * part.pcom().cMass())));
            win32::TextOutA(hdc, rc.left, rc.top, app.state.szWork, cch);
        }
    }
Restore:
    win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
    win32::SetTextColor(hdc, crForeSav);
    win32::SetBkColor(hdc, crBackSav);
    if (fCreatedDC) {
        win32::ReleaseDC(hwnd, hdc);
    }
    return;
}

void DrawBuildSelHull(APPSESSION &app, HWND hwnd, HDC hdc, std::int16_t iDraw, RECT *prc) {
    char     rgch[20];
    DV       dv;
    std::uint16_t rgCosts[4];
    std::int16_t  fCreatedDC;
    COLORREF crForeSav;
    std::int16_t  dxMineral;
    std::int16_t  k;
    COLORREF crBackSav;
    std::int16_t  csh;
    HUL     *lphul;
    std::int32_t  dpShield;
    std::int16_t  cch;
    std::int32_t  dp;
    std::int16_t  dxkT;
    RECT     rc;
    std::int32_t  lwt;
    std::int16_t  i;
    std::int16_t  j;
    std::int16_t  dPlanRange;
    std::int16_t  dRange;
    std::int16_t  pctDetect;
    std::int16_t  pct;

    fCreatedDC = 0;
    if (app.lpui->mdBuild != mdBuildComp) {
        if (!hdc) {
            fCreatedDC = 1;
            hdc = win32::GetDC(hwnd);
        }
        if (!prc) {
            ControlIO().pfnGetClientRect(hwnd, &rc);
            rc.bottom -= 32;
            rc.right -= 4;
            rc.left = rc.right - 320;
            rc.top = (!app.state.fStarbaseMode ? 0 : app.state.dyArial8) + app.lpui->yBuildInfoSum;
            win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
        } else {
            rc = *prc;
        }
        if (!app.lpui->lpshdefBuild)
            goto LReleaseDC;
        lphul = &app.lpui->lpshdefBuild->hul;
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        dxkT = LOWORD(GetTextExtent(hdc, TextIO().pfnCompressed(app, idsKt).data(), 2));
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        dxMineral = LOWORD(GetTextExtent(hdc, app.state.rgszMinerals[2], std::strlen(app.state.rgszMinerals[2]))) + 6;
        crForeSav = win32::SetTextColor(hdc, 0);
        crBackSav = win32::SetBkColor(hdc, app.lpui->crButtonFace);
        if (app.lpui->hwndPopup && app.lpui->GlobalPD.grPopup == grPopupShdef && app.lpui->GlobalPD.fToken) {
            GetVCRStats(app, app.lpui->viVCRFocus, &dp, &dv, &dpShield, &csh);
            dpShield = static_cast<std::uint32_t>(app.state.vrgtok[app.lpui->viVCRFocus].dpShield) - dpShield;
            if (dpShield < 0) {
                dpShield = 0;
            }
            dpShield = static_cast<std::uint32_t>((dpShield * csh));
            if (csh == 0)
                goto LDeadToken;
        } else {
            dp = static_cast<std::uint32_t>(lphul->dp);
            dpShield = DpShieldOfShdef(app, app.lpui->lpshdefBuild, app.state.idPlayer);
        }
        if (app.lpui->hwndPopup && app.state.fStarbaseMode) {
            rc.top += app.state.dyArial8;
            cch = TextIO().pfnCchGetString(app, idsCost, std::span{app.state.szWork});
        } else {
            TextIO().pfnCchGetString(app, idsHull, std::span{rgch});
            cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsCostOneSS).data(), lphul->szClass, app.lpui->mdBuild == mdBuildHuldef ? rgch : "");
        }
        win32::TextOutA(hdc, rc.left, rc.top, app.state.szWork, cch);
        rc.left += 8;
        rc.right -= 8;
        GetTrueHullCost(app.state.idPlayer, lphul, rgCosts);
        if (app.state.fStarbaseMode && (RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceISB) != 0 || RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh)) {
            for (k = 0; k < 4; k++) {
                rgCosts[k] -= static_cast<std::uint32_t>(rgCosts[k]) / 5;
            }
        }
        if (app.state.fStarbaseMode) {
            for (k = 0; k < 4; k++) {
                rgCosts[k] -= static_cast<std::uint32_t>(rgCosts[k]) / 2;
            }
        }
        for (k = 0; k <= 5; k++) {
            if (k == 3) {
                k = 5;
            }
            rc.top += app.state.dyArial8;
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            win32::SetTextColor(hdc, app.lpui->rgcrMinerals[k]);
            win32::TextOutA(hdc, rc.left, rc.top, app.state.rgszMinerals[k], win32::lstrlenA(app.state.rgszMinerals[k]));
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            win32::SetTextColor(hdc, app.lpui->crWindowText);
            cch = win32::wsprintfA(app.state.szWork, app.state.PCTD, k == 5 ? rgCosts[3] : rgCosts[k]);
            RightTextOut(app, hdc, rc.left + dxMineral + app.lpui->dxMaxMineralQuan - dxkT, rc.top, app.state.szWork, cch, app.lpui->dxMaxMineralQuan);
            if (k < 5) {
                win32::TextOutA(hdc, rc.left + dxMineral + app.lpui->dxMaxMineralQuan - dxkT, rc.top, TextIO().pfnCompressed(app, idsKt).data(), 2);
            }
        }
        rc.left -= 8;
        rc.top += app.state.dyArial8;
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        if (!app.state.fStarbaseMode) {
            if (app.lpui->hwndPopup && app.lpui->GlobalPD.grPopup == grPopupShdef && app.lpui->GlobalPD.fToken) {
                lwt = static_cast<std::uint32_t>(app.state.vrgtok[app.lpui->viVCRFocus].wt);
            } else {
                lwt = static_cast<std::uint32_t>(lphul->wtEmpty);
            }
            cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsMassLdkt).data(), lwt);
            win32::TextOutA(hdc, rc.left, rc.top, app.state.szWork, cch);
        }
        rc.top -= app.state.dyArial8 * 4;
        rc.left += dxMineral + app.lpui->dxMaxMineralQuan + 24;
        if (!app.state.fStarbaseMode && (!app.lpui->hwndPopup || app.lpui->GlobalPD.grPopup != grPopupShdef || !app.lpui->GlobalPD.fToken)) {
            cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDmg).data(), WtMaxShdefStat(app.lpui->lpshdefBuild, 1));
            RightTextOut(app, hdc, rc.right - 8, rc.top, app.state.szWork, cch, app.lpui->dxMaxMineralQuan);
            cch = TextIO().pfnCchGetString(app, idsMaxFuel, std::span{app.state.szWork});
            win32::TextOutA(hdc, rc.left, rc.top, app.state.szWork, cch);
            rc.top += app.state.dyArial8;
        }
        cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsLddp).data(), dp);
        RightTextOut(app, hdc, rc.right - 8, rc.top, app.state.szWork, cch, app.lpui->dxMaxMineralQuan + 10);
        cch = TextIO().pfnCchGetString(app, idsArmor, std::span{app.state.szWork});
        win32::TextOutA(hdc, rc.left, rc.top, app.state.szWork, cch);
        rc.top += app.state.dyArial8;
        if (app.lpui->mdBuild != mdBuildHuldef) {
            cch = win32::wsprintfA(app.state.szWork, dpShield == 0 ? TextIO().pfnCompressed(app, idsNone).data() : "%lddp", dpShield);
            RightTextOut(app, hdc, rc.right - 8, rc.top, app.state.szWork, cch, app.lpui->dxMaxMineralQuan + 10);
            cch = TextIO().pfnCchGetString(app, idsShields, std::span{app.state.szWork});
            win32::TextOutA(hdc, rc.left, rc.top, app.state.szWork, cch);
        }
        rc.top += app.state.dyArial8;
        if (app.lpui->mdBuild != mdBuildHuldef) {
            app.lpui->lpshdefBuild->lPower = LComputePower(app, app.lpui->lpshdefBuild);
            if (app.lpui->lpshdefBuild->lPower != 0) {
                cch = win32::wsprintfA(app.state.szWork, app.state.PCTLD, app.lpui->lpshdefBuild->lPower);
                RightTextOut(app, hdc, rc.right - 8, rc.top, app.state.szWork, cch, app.lpui->dxMaxMineralQuan);
                cch = TextIO().pfnCchGetString(app, idsRating, std::span{app.state.szWork});
                win32::TextOutA(hdc, rc.left, rc.top, app.state.szWork, cch);
                rc.top += app.state.dyArial8;
            }
        }
        if (app.state.gd.mdScreenSize > 0 && app.lpui->mdBuild != mdBuildHuldef) {
            if (app.lpui->mdBuild != mdBuildEnemyShdef) {
                i = app.state.idPlayer;
            } else {
                i = -1;
            }
            i = PctCloakFromHuldef(app, &app.lpui->lpshdefBuild->hul, i, nullptr);
            j = PctJammerFromHul(app, &app.lpui->lpshdefBuild->hul);
            cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDD4).data(), i, j);
            RightTextOut(app, hdc, rc.right - 8, rc.top, app.state.szWork, cch, app.lpui->dxMaxMineralQuan);
            cch = TextIO().pfnCchGetString(app, idsCloakJam, std::span{app.state.szWork});
            win32::TextOutA(hdc, rc.left, rc.top, app.state.szWork, cch);
            rc.top += app.state.dyArial8;
            i = InitFromHuldef(app, &app.lpui->lpshdefBuild->hul, nullptr);
            if (app.state.fStarbaseMode || app.lpui->lpshdefBuild->hul.rghs[0].cItem == 0) {
                j = 0;
            } else {
                j = SpdOfShip(app, nullptr, 0, nullptr, 0, app.lpui->lpshdefBuild) + 1;
            }
            cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDS).data(), i, &app.lpui->rgszSpeed[j * 3]);
            RightTextOut(app, hdc, rc.right - 8, rc.top, app.state.szWork, cch, app.lpui->dxMaxMineralQuan);
            cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsInitiativeMoves + (app.state.dyArial8 > 14)), std::span{app.state.szWork});
            win32::TextOutA(hdc, rc.left, rc.top, app.state.szWork, cch);
            rc.top += app.state.dyArial8;
            if (!app.state.fStarbaseMode) {
                if (app.lpui->mdBuild != mdBuildEnemyShdef) {
                    i = app.state.idPlayer;
                } else {
                    i = -1;
                }
                dRange = GetShdefScannerRange(app, app.lpui->lpshdefBuild, i, &dPlanRange, &pctDetect, nullptr);
                if (dRange > 0) {
                    if (pctDetect >= 100) {
                        cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDD6).data(), dRange, dPlanRange);
                    } else {
                        cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDDD).data(), dRange, dPlanRange, pctDetect);
                    }
                    RightTextOut(app, hdc, rc.right - 8, rc.top, app.state.szWork, cch, app.lpui->dxMaxMineralQuan + 40);
                    cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsScannerRange2 + (app.state.dyArial8 > 14)), std::span{app.state.szWork});
                    win32::TextOutA(hdc, rc.left, rc.top, app.state.szWork, cch);
                    rc.top += app.state.dyArial8;
                }
            } else if (RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh) {
                cch = CommaFormatLong(app, app.state.szWork, static_cast<std::uint32_t>((app.state.rglPopMac[app.lpui->lpshdefBuild->hul.ihuldef - ihuldefOrbitalFort] * 100)));
                RightTextOut(app, hdc, rc.right - 8, rc.top, app.state.szWork, cch, app.lpui->dxMaxMineralQuan + 16);
                cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsMaxPopulation + (app.state.dyArial8 > 14)), std::span{app.state.szWork});
                win32::TextOutA(hdc, rc.left, rc.top, app.state.szWork, cch);
                rc.top += app.state.dyArial8;
            }
        }
        if (app.lpui->hwndPopup && app.lpui->GlobalPD.grPopup == grPopupShdef && app.lpui->GlobalPD.fShowDamage) {
            if (!app.lpui->GlobalPD.fToken) {
                if (!app.state.fStarbaseMode) {
                    if (app.lpui->GlobalPD.fSummary) {
                        k = app.lpui->lpshdefBuild->ishdef;
                        csh = app.state.rglpfl[app.state.sel.scan.ifl]->rgcsh[k];
                        dv.dp = app.state.rglpfl[app.state.sel.scan.ifl]->rgdv[k].dp;
                    } else {
                        k = app.lpui->lpshdefBuild->ishdef;
                        csh = app.state.sel.fl.rgcsh[k];
                        dv.dp = app.state.sel.fl.rgdv[k].dp;
                    }
                } else {
                    dv.dp = 0;
                    if (app.lpui->GlobalPD.fSummary) {
                        dv.SetPctDp(LpplFromId(app, app.state.sel.scan.idpl)->pctDp);
                    } else {
                        dv.SetPctDp(app.state.sel.pl.pctDp);
                    }
                    csh = 1;
                    if (dv.pctDp() != 0) {
                        dv.SetPctSh(100);
                    }
                }
            }
            if (dv.dp != 0) {
                win32::SetTextColor(hdc, 127);
                pct = dv.pctDp() / 5;
                if (pct <= 0) {
                    pct = 1;
                }
                csh = LOWORD(static_cast<std::int32_t>((csh * dv.pctSh())) / 100);
                if (csh <= 0) {
                    csh = 1;
                }
                if (app.state.fStarbaseMode) {
                    cch = win32::wsprintfA(app.state.szWork, app.state.PCTDPCTPCT, pct);
                } else {
                    cch = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsLdD).data(), csh, pct);
                }
                dp = 1;
            } else {
                dp = 0;
            }
            if (dp != 0) {
                RightTextOut(app, hdc, rc.right - 8, rc.top, app.state.szWork, cch, app.lpui->dxMaxMineralQuan + 15);
                cch = TextIO().pfnCchGetString(app, idsDamage, std::span{app.state.szWork});
                win32::TextOutA(hdc, rc.left, rc.top, app.state.szWork, cch);
            }
        }
    LDeadToken:
        win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
        win32::SetTextColor(hdc, crForeSav);
        win32::SetBkColor(hdc, crBackSav);
    LReleaseDC:
        if (fCreatedDC) {
            win32::ReleaseDC(hwnd, hdc);
        }
    }
    return;
}

void SetBuildSelection(APPSESSION &app, std::int16_t iSrc) {
    std::int16_t iSelOld;
    RECT    rc;

    if (iSrc != app.lpui->iselSlot) {
        iSelOld = app.lpui->iselSlot;
        app.lpui->iselSlot = iSrc;
        ControlIO().pfnGetClientRect(app.lpui->hwndSlotDlg, &rc);
        if (iSelOld >= 0) {
            DrawSlotDlg(app, app.lpui->hwndSlotDlg, nullptr, &rc, iSelOld);
        }
        if (app.lpui->iselSlot >= 0) {
            DrawSlotDlg(app, app.lpui->hwndSlotDlg, nullptr, &rc, app.lpui->iselSlot);
        }
    RedrawSel:
        DrawBuildSelComp(app, app.lpui->hwndSlotDlg, nullptr, -1);
    } else if (iSrc == -1) {
        goto RedrawSel;
    }
    return;
}

std::int16_t IDropPart(APPSESSION &app, POINT16 pt, HS hsSrc, std::int16_t iSrc, std::int16_t fNoModify) {
    std::int16_t cSlot;
    std::int16_t cNew;
    std::int16_t i;
    HS      hsHul;
    HS      hsDst;
    RECT    rc;

    ControlIO().pfnGetClientRect(app.lpui->hwndSlotDlg, &rc);
    if (ControlIO().pfnGetAsyncKeyState(win32::VK_CONTROL) & 0xfffe) {
        if (iSrc < 0) {
            hsSrc.cItem = 100;
        }
    } else if (ControlIO().pfnGetAsyncKeyState(win32::VK_SHIFT) & 0xfffe) {
        if (iSrc < 0 || hsSrc.cItem > 4) {
            hsSrc.cItem = 4;
        }
    } else if (iSrc >= 0) {
        hsSrc.cItem = 1;
    }
    cSlot = PartQueryIO().pfnHull(app.lpui->lpshdefBuild->hul.ihuldef)->hul.chs;
    for (i = 0; i < cSlot && win32::PtInRect(&app.lpui->vrgrcSlot[i], PointFrom16(pt)) == 0; i++) {
    }
    if (i == cSlot) {
        if (pt.x < rc.right >> 1) {
            if (!fNoModify && iSrc >= 0) {
                if (app.lpui->lpshdefBuild->hul.rghs[iSrc].grhst & hstEngine) {
                    hsSrc.cItem = 100;
                }
                app.lpui->lpshdefBuild->hul.rghs[iSrc].cItem =
                    0 <= app.lpui->lpshdefBuild->hul.rghs[iSrc].cItem - hsSrc.cItem ? app.lpui->lpshdefBuild->hul.rghs[iSrc].cItem - hsSrc.cItem : 0;
                UpdateShdefCost(app, app.lpui->lpshdefBuild);
                ControlIO().pfnGetClientRect(app.lpui->hwndSlotDlg, &rc);
                DrawSlotDlg(app, app.lpui->hwndSlotDlg, nullptr, &rc, iSrc);
                rc.top = app.lpui->yBuildInfoSum;
                ControlIO().pfnInvalidateRect(app.lpui->hwndSlotDlg, &rc, 1);
                DrawBuildSelComp(app, app.lpui->hwndSlotDlg, nullptr, -1);
                DrawBuildSelHull(app, app.lpui->hwndSlotDlg, nullptr, -1, nullptr);
                if (app.state.gd.fTutorial) {
                    PresentationIO().pfnTutor(app);
                }
            }
            return 1;
        }
        return 0;
    }
    hsDst = app.lpui->lpshdefBuild->hul.rghs[i];
    hsHul = PartQueryIO().pfnHull(app.lpui->lpshdefBuild->hul.ihuldef)->hul.rghs[i];
    if (hsHul.grhst & hstEngine) {
        hsSrc.cItem = 100;
    }
    if (i == iSrc) {
        return 2;
    }
    if (hsDst.cItem >= hsHul.cItem ||
        ((hsDst.cItem > 0 && (hsDst.grhst != hsSrc.grhst || hsDst.iItem != hsSrc.iItem)) || (hsDst.cItem == 0 && !(hsSrc.grhst & hsHul.grhst)))) {
        if (!fNoModify) {
            ControlIO().pfnMessageBeep(win32::MB_OK);
        }
        return 3;
    }
    if (!fNoModify) {
        hsDst.grhst = hsSrc.grhst;
        hsDst.iItem = hsSrc.iItem;
        cNew = static_cast<std::uint16_t>((hsDst.cItem + hsSrc.cItem)) >= hsHul.cItem ? hsHul.cItem : hsDst.cItem + hsSrc.cItem;
        if (iSrc >= 0) {
            app.lpui->lpshdefBuild->hul.rghs[iSrc].cItem -= cNew - hsDst.cItem;
        }
        hsDst.cItem = cNew;
        app.lpui->lpshdefBuild->hul.rghs[i] = hsDst;
        UpdateShdefCost(app, app.lpui->lpshdefBuild);
        SetBuildSelection(app, i);
        ControlIO().pfnGetClientRect(app.lpui->hwndSlotDlg, &rc);
        rc.top = app.lpui->yBuildInfoSum;
        ControlIO().pfnInvalidateRect(app.lpui->hwndSlotDlg, &rc, 1);
        if (app.state.gd.fTutorial) {
            PresentationIO().pfnTutor(app);
        }
    }
    return -1;
}

void DrawDlgLBEntireItem(APPSESSION &app, DRAWITEMSTRUCT *lpdis, std::int16_t inflate) {
    COLORREF cr;
    COLORREF crForeSav;
    std::int16_t  ibmp;
    std::int16_t  bkSav;
    RECT     rc;

    win32::CopyRect(&rc, &lpdis->rcItem);
    win32::FillRect(lpdis->hDC, &lpdis->rcItem, static_cast<HBRUSH>(win32::GetStockObject(!(lpdis->itemState & win32::ODS_FOCUS) ? win32::WHITE_BRUSH : win32::BLACK_BRUSH)));
    win32::InflateRect(&rc, -2, -1);
    ControlIO().pfnSendMessage(lpdis->hwndItem, win32::LB_GETTEXT, lpdis->itemID, reinterpret_cast<LPARAM>(app.state.szWork));
    win32::SelectPalette(lpdis->hDC, app.lpui->vhpal, 0);
    win32::RealizePalette(lpdis->hDC);
    ibmp = app.state.szWork[2] - 'A' + (app.state.szWork[3] - 'A') * 26;
    DibBlt(lpdis->hDC, rc.left, rc.top, 64, 64, app.lpui->rghdibInventory[ibmp >> 5], (ibmp & 7) * 0x40, ((3 - (ibmp >> 3)) & 3) * 0x40, 64, 64, 13369376);
    cr = (lpdis->itemState & win32::ODS_FOCUS) ? app.lpui->crWindow : app.lpui->crWindow == 0 ? 0xffffff : 0;
    crForeSav = win32::SetTextColor(lpdis->hDC, cr);
    bkSav = win32::SetBkMode(lpdis->hDC, win32::TRANSPARENT);
    win32::TextOutA(lpdis->hDC, rc.left + 66, rc.top + 0x20 - (app.state.dyArial8 >> 1), &app.state.szWork[4], std::strlen(&app.state.szWork[4]));
    win32::SetTextColor(lpdis->hDC, crForeSav);
    win32::SetBkMode(lpdis->hDC, bkSav);
    HandleFocusState(app, lpdis, inflate + 2);
    return;
}

void FillBuildDD(APPSESSION &app, HWND hwndDD, MdBuild md) {
    std::int16_t ishdefMac;
    std::int16_t fProgress;
    std::int16_t fAdded;
    std::int16_t i;
    std::int16_t j;
    SHDEF  *lpshdef;
    RECT    rc;
    PART    part;

    ControlIO().pfnSendMessage(hwndDD, win32::CB_RESETCONTENT, 0, 0);
    if (md != mdBuildShdef) {
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(app.lpui->hwndSlotDlg, IDC_DELETE), 0);
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(app.lpui->hwndSlotDlg, IDC_EDIT), 0);
    }
    if (app.state.fStarbaseMode) {
        ishdefMac = 10;
        lpshdef = app.state.rglpshdefSB[app.state.idPlayer];
    } else {
        ishdefMac = 16;
        lpshdef = app.state.rgshdef;
    }
    for (i = 0; i < ishdefMac && lpshdef[i].fFree == 0; i++) {
    }
    switch (md) {
    case mdBuildShdef:
    case mdBuildHuldef:
    case mdBuildEnemyShdef:
        if (i < ishdefMac) {
            fAdded = 1;
            break;
        }
        /* fallthrough */
    default:
        fAdded = 0;
    }
    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(app.lpui->hwndSlotDlg, IDC_IMPORT), fAdded);
    switch (md) {
    case mdBuildShdef:
    default:
        fAdded = 0;
        for (i = 0; i < ishdefMac; i++) {
            if (!lpshdef[i].fFree) {
                if (!fAdded) {
                    CshQueued(app, (!app.state.fStarbaseMode ? 0 : 16) + i, &fProgress, 0);
                    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(app.lpui->hwndSlotDlg, IDC_EDIT), lpshdef[i].cExist == 0 && !fProgress);
                    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(app.lpui->hwndSlotDlg, IDC_DELETE), 1);
                    fAdded = (lpshdef[i].cExist != 0) + 1;
                }
                ControlIO().pfnSendMessage(hwndDD, win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(lpshdef[i].hul.szClass));
            }
        }
        break;
    case mdBuildEnemyShdef:
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (i != app.state.idPlayer) {
                lpshdef = !app.state.fStarbaseMode ? app.state.rglpshdef[i] : app.state.rglpshdefSB[i];
                if (lpshdef) {
                    for (j = 0; j < ishdefMac; j++) {
                        if (!lpshdef[j].fFree) {
                            CchCopyUtf8Prefix(std::span{app.state.szWork}, TextIO().pfnPlayerName(app, i, 1, 0, 0, 0, nullptr).data());
                            win32::wsprintfA(std::span{app.state.szWork}.subspan(static_cast<std::size_t>(std::strlen(app.state.szWork))), " %s", lpshdef[j].hul.szClass);
                            ControlIO().pfnSendMessage(hwndDD, win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(app.state.szWork));
                        }
                    }
                }
            }
        }
        break;
    case mdBuildHuldef:
        if (app.state.fStarbaseMode) {
            part.hs.grhst = hstSBHull;
            j = 5;
        } else {
            part.hs.grhst = hstHull;
            j = 32;
        }
        for (i = 0; i < j; i++) {
            part.hs.iItem = i;
            if (PartQueryIO().pfnLookup(app, &part) == mdPartAvailAvailable) {
                ControlIO().pfnSendMessage(hwndDD, win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(part.pcom().szName()));
            }
        }
        break;
    case mdBuildComp:
    case mdBuildEdit:
        if (app.state.fStarbaseMode) {
            for (i = 0; i < 8; i++) {
                ControlIO().pfnSendMessage(hwndDD, win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TextIO().pfnCompressed(app, rgidsPartsSB[i]).data()));
            }
        } else {
            for (i = 0; i < 13; i++) {
                ControlIO().pfnSendMessage(hwndDD, win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TextIO().pfnCompressed(app, rgidsParts[i]).data()));
            }
        }
    }
    i = LOWORD(ControlIO().pfnSendMessage(hwndDD, win32::CB_GETCOUNT, 0, 0));
    if (i > 32) {
        i = 32;
    }
    win32::GetWindowRect(hwndDD, &rc);
    ControlIO().pfnSetWindowPos(hwndDD, nullptr, 0, 0, rc.right - rc.left, (i + 1) * (app.state.dyArial8 - (app.state.dyArial8 <= 14)) + 8 + (app.state.dyArial8 > 14), win32::SWP_NOMOVE | win32::SWP_NOZORDER);
    ControlIO().pfnSendMessage(hwndDD, win32::CB_SETCURSEL, 0, 0);
    return;
}

void FillBuildPartsLB(APPSESSION &app, HWND hwndLB, std::int16_t grbit) {
    mdPartAvail  mdAvail;
    std::int16_t      i;
    char         sz[200];
    HullSlotType grbitCur;
    PART         part;

    grbitCur = hstEngine;
    sz[0] = 'A';
    ControlIO().pfnSendMessage(hwndLB, win32::LB_RESETCONTENT, 0, 0);
    while (grbitCur != hstNone) {
        if (grbitCur & grbit) {
            i = 0;
            part.hs.grhst = grbitCur;
            while (1) {
                part.hs.iItem = i;
                mdAvail = static_cast<mdPartAvail>(PartQueryIO().pfnLookup(app, &part));
                if (mdAvail == mdPartAvailInvalid)
                    break;
                if (app.state.fStarbaseMode && grbitCur == hstSpecialE && (i == ispecialETachyonDetector || i == ispecialEAntiMatterGenerator)) {
                    mdAvail = mdPartAvailRestricted;
                }
                if (mdAvail == mdPartAvailAvailable) {
                    sz[1] = i + 'A';
                    sz[2] = part.pcom().ibmp() % 26 + 'A';
                    sz[3] = part.pcom().ibmp() / 26 + 'A';
                    std::strcpy(&sz[4], part.pcom().szName());
                    ControlIO().pfnSendMessage(hwndLB, win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(sz));
                }
                i++;
            }
        }
        grbitCur = static_cast<HullSlotType>(grbitCur * 2);
        sz[0]++;
    }
    return;
}

LRESULT  FakeListProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    POINT16 pt;
    std::int16_t iSel;

    switch (msg) {
    case win32::WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (pt.x < 64) {
            win32::SetCursor(app.lpui->hcurHand);
            return 1;
        }
        break;
    case win32::WM_LBUTTONDOWN:
    case win32::WM_RBUTTONDOWN:
        if (LOWORD(lParam) < 64 && (app.lpui->mdBuild == mdBuildEdit || msg == win32::WM_RBUTTONDOWN)) {
            ui::CallWindowProc(app.lpui->lpfnRealListProc, hwnd, win32::WM_LBUTTONDOWN, wParam, lParam);
            ui::CallWindowProc(app.lpui->lpfnRealListProc, hwnd, win32::WM_LBUTTONUP, wParam, lParam);
            if (msg == win32::WM_RBUTTONDOWN || app.lpui->mdBuild != mdBuildEdit) {
                iSel = LOWORD(ControlIO().pfnSendMessage(hwnd, win32::LB_GETCURSEL, 0, 0));
                if (iSel == -1) {
                    return 0;
                }
                ControlIO().pfnSendMessage(hwnd, win32::LB_GETTEXT, iSel, reinterpret_cast<LPARAM>(app.state.szWork));
                app.lpui->GlobalPD.part.hs.grhst = static_cast<HullSlotType>(1 << (app.state.szWork[0] - 'A'));
                app.lpui->GlobalPD.part.hs.iItem = app.state.szWork[1] - 'A';
                PartQueryIO().pfnLookup(app, &app.lpui->GlobalPD.part);
                app.lpui->GlobalPD.grPopup = grPopupComponent;
                Popup(app, hwnd, LOWORD(lParam), HIWORD(lParam));
                return 0;
            }
            FTrackSlot(app, hwnd, LOWORD(lParam), HIWORD(lParam), wParam, 1, 0);
            return 0;
        }
        break;
    }
    return ui::CallWindowProc(app.lpui->lpfnRealListProc, hwnd, msg, wParam, lParam);
}

void UpdateSlotGlobals(APPSESSION &app) {
    std::int16_t  yTop;
    std::int16_t  cSlot;
    std::int16_t  i;
    std::uint16_t wrc;
    std::int16_t  xLeft;
    HULDEF  *lphuldef;

    if (!app.lpui->lpshdefBuild) {
        cSlot = 0;
    } else {
        lphuldef = PartQueryIO().pfnHull(app.lpui->lpshdefBuild->hul.ihuldef);
        if (!app.lpui->hwndSlotDlg) {
            xLeft = 12;
            yTop = app.state.dyArial8 + 12;
        } else {
            xLeft = app.lpui->ptslotGlob.x - 330;
            yTop = 32;
        }
        cSlot = lphuldef->hul.chs;
        for (i = 0; i < cSlot; i++) {
            app.lpui->vrgrcSlot[i].left = (lphuldef->rgbrc[i] & 0xf) * 0x20 + xLeft;
            app.lpui->vrgrcSlot[i].top = (lphuldef->rgbrc[i] >> 4) * 0x20 + yTop;
            app.lpui->vrgrcSlot[i].right = app.lpui->vrgrcSlot[i].left + 64;
            app.lpui->vrgrcSlot[i].bottom = app.lpui->vrgrcSlot[i].top + 64;
        }
        if (lphuldef->hul.wtCargoMax != 0) {
            wrc = lphuldef->wrcCargo;
            app.lpui->rcCargo.left = (wrc >> 8 & 0xff & 0xf) * 0x20 + xLeft;
            app.lpui->rcCargo.top = ((wrc >> 8 & 0xff) >> 4) * 0x20 + yTop;
            app.lpui->rcCargo.right = (wrc & 0xff & 0xff & 0xf) * 0x20 + xLeft;
            app.lpui->rcCargo.bottom = ((wrc & 0xff & 0xff) >> 4) * 0x20 + yTop;
        }
        app.lpui->ptPlaque.x = xLeft + 258;
        app.lpui->ptPlaque.y = yTop + 273;
    }
    return;
}

std::int16_t IEmptyBmpFromGrhst(APPSESSION &app, std::int16_t grhst) {
    std::int16_t i;

    for (i = 0; i < 21; i++) {
        if (app.lpui->rgmapBuildBmps[i] == grhst) {
            return i;
        }
    }
    return 0;
}

void MakeNewName(APPSESSION &app, char *lpsz) {
    std::int16_t cLen;

    cLen = std::strlen(lpsz);
    if (cLen <= 27) {
        if (lpsz[cLen - 1] != ')' || std::isdigit(lpsz[cLen - 2]) == 0 || lpsz[cLen - 3] != '(') {
            std::strcpy(lpsz + cLen, " (2)");
        } else if (lpsz[cLen - 2] == '9') {
            lpsz[cLen - 2] = '0';
        } else {
            lpsz[cLen - 2] = lpsz[cLen - 2] + 1;
        }
    }
    FStringFitsScreen(app, lpsz, 160);
    return;
}
