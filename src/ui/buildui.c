#include "win.h"

int16_t ShipBuilder(POINT16 ptDlgSize) {
    FARPROC lpProcSlot;
    int16_t fSuccess;

    ptslotGlob = ptDlgSize;
    if (gd.mdScreenSize > 0) {
        ptslotGlob.y += 3 * dyArial8;
    }
    fStarbaseMode = FALSE;
    lpshdefBuild = NthValidShdef(0);
    lpProcSlot = MakeProcInstance(SlotDlg, hInst);
    fSuccess = DialogBox(hInst, MAKEINTRESOURCE(IDD_SLOT), hwndFrame, lpProcSlot);
    FreeProcInstance(lpProcSlot);
    if (sel.grobj == grobjPlanet && sel.pl.lpplprod) {
        FillPlanetProdLB(hwndPlanetProdLB, sel.pl.lpplprod, NULL);
    }
    return 0;
}

void ShowMainControls(HWND hwnd, int16_t sw) {
    ShowWindow(GetDlgItem(hwnd, IDC_IMPORT), sw);
    ShowWindow(GetDlgItem(hwnd, IDC_EDIT), sw);
    ShowWindow(GetDlgItem(hwnd, IDC_DELETE), sw);
    ShowWindow(GetDlgItem(hwnd, IDC_DESIGNER_SHIPS), sw);
    ShowWindow(GetDlgItem(hwnd, IDC_DESIGNER_STARBASES), sw);
    ShowWindow(GetDlgItem(hwnd, IDC_DESIGNER_EXISTING), sw);
    ShowWindow(GetDlgItem(hwnd, IDC_DESIGNER_HULLS), sw);
    ShowWindow(GetDlgItem(hwnd, IDC_DESIGNER_ENEMY_HULLS), sw);
    ShowWindow(GetDlgItem(hwnd, IDC_DESIGNER_COMPONENTS), sw);
    ShowWindow(GetDlgItem(hwnd, IDOK), sw == SW_SHOW ? SW_HIDE : SW_SHOW);
    SetDlgItemText(hwnd, IDCANCEL, PszGetCompressedString(sw == SW_SHOW ? idsDone : idsCancel));
    return;
}

int16_t FCheckQueuedShip(HWND hwnd, SHDEF *lpshdef, int16_t fEdit) {
    char     rgch[40];
    int16_t  fProgress;
    int16_t  id;
    StringId ids;
    int16_t  cshQueued;

    cshQueued = CshQueued(lpshdef->ishdef, &fProgress, fEdit);
    if (lpshdef->cExist > 0 || cshQueued != 0) {
        if (fEdit) {
            ids = idsCurrentlyHaveDSSIfDelete2;
        } else {
            ids = !fStarbaseMode ? idsCurrentlyHaveDSSDProduction : idsCurrentlyHaveDSSDProduction2;
        }
        CchGetString(idsWorkDone, rgch);
        if (lpshdef->cExist > 0 && cshQueued != 0) {
            wsprintf(szWork, PszGetCompressedString(ids), LOWORD(lpshdef->cExist), lpshdef->hul.szClass, lpshdef->cExist == 1 ? "" : "s", cshQueued,
                     !fProgress ? "" : rgch);
        } else if (cshQueued != 0) {
            wsprintf(szWork, PszGetCompressedString(ids + 1), cshQueued, lpshdef->hul.szClass, cshQueued == 1 ? "" : "s", !fProgress ? "" : rgch);
        } else {
            wsprintf(szWork, PszGetCompressedString(ids + 2), LOWORD(lpshdef->cExist), lpshdef->hul.szClass, lpshdef->cExist == 1 ? "" : "s");
        }
        id = MessageBox(GetFocus(), szWork, PszGetCompressedString(idsDeleteDesign + fEdit), MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL);
        SetFocus(hwnd);
        if (id == 7) {
            return FALSE;
        }
        if (lpshdef->cExist > 0) {
            DestroyAllIshdef(lpshdef->ishdef, idPlayer);
            InvalidateRect(hwndScanner, NULL, TRUE);
            InvalidateRect(hwndMessage, NULL, TRUE);
        } else {
            RemoveIshdefFromAllQueues(lpshdef->ishdef, fEdit);
        }
    }
    return TRUE;
}

INT_PTR CALLBACK SlotDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    RECT               rcWindow;
    HDC                hdc;
    RECT               rcGBox;
    SHDEF             *lpshdef;
    int16_t            left;
    PAINTSTRUCT        ps;
    HWND               hwndItem;
    int16_t            cch;
    int32_t            lSel;
    RECT               rc;
    DRAWITEMSTRUCT    *lpdis;
    MEASUREITEMSTRUCT *lpmis;
    int16_t            i;
    POINT16            pt;
    int16_t            fProtoSB;
    int16_t            fProgress;
    PART               part;
    int16_t            cshQueued;
    int16_t            j;

    switch (message) {
    case WM_INITDIALOG:
        fHullCopy = FALSE;
        hwndSlotDlg = hwnd;
        GetWindowRect(hwnd, &rcWindow);
        GetClientRect(hwnd, &rc);
        SetWindowPos(hwnd, NULL, 0, 0, ptslotGlob.x + rcWindow.right - rcWindow.left - rc.right, ptslotGlob.y + rcWindow.bottom - rcWindow.top - rc.bottom,
                     SWP_NOMOVE | SWP_NOZORDER);
        StickyDlgPos(hwnd, &ptStickySlotDlg, TRUE);
        UpdateSlotGlobals();
        hwndItem = GetDlgItem(hwnd, IDC_DESIGNER_COMPONENT_LIST);
        SetWindowPos(hwndItem, NULL, ptslotGlob.x - 256, 32, 240, 266, SWP_NOZORDER);
        FillBuildPartsLB(hwndItem, rggrbitParts[0]);
        yBuildInfoSum = 340;
        lpfnRealListProc = (WNDPROC)GetWindowLongPtr(hwndItem, GWLP_WNDPROC);
        SetWindowLongPtr(hwndItem, GWLP_WNDPROC, (LONG_PTR)lpfnFakeListProc);
        CheckRadioButton(hwnd, IDC_DESIGNER_SHIPS, IDC_DESIGNER_STARBASES, IDC_DESIGNER_SHIPS);
        CheckRadioButton(hwnd, IDC_DESIGNER_EXISTING, IDC_DESIGNER_COMPONENTS, IDC_DESIGNER_EXISTING);
        mdBuild = mdBuildShdef;
        hwndItem = GetDlgItem(hwnd, IDC_COMBOBOX);
        SetWindowPos(hwndItem, NULL, ptslotGlob.x - 264, 8, 240, 100, SWP_NOZORDER);
        FillBuildDD(hwndItem, mdBuild);
        SetWindowPos(GetDlgItem(hwnd, IDOK), NULL, ptslotGlob.x - 226, ptslotGlob.y - (int16_t)(3 * dyArial8) / 2 - 6, 68, (int16_t)(3 * dyArial8) / 2,
                     SWP_NOZORDER | SWP_HIDEWINDOW);
        SetWindowPos(GetDlgItem(hwnd, IDCANCEL), NULL, ptslotGlob.x - 148, ptslotGlob.y - (int16_t)(3 * dyArial8) / 2 - 6, 68, (int16_t)(3 * dyArial8) / 2,
                     SWP_NOZORDER);
        SetWindowPos(GetDlgItem(hwnd, IDC_HELP), NULL, ptslotGlob.x - 74, ptslotGlob.y - (int16_t)(3 * dyArial8) / 2 - 6, 68, (int16_t)(3 * dyArial8) / 2,
                     SWP_NOZORDER);
        SetDlgItemText(hwnd, IDCANCEL, PszGetCompressedString(idsDone));
        if (gd.fTutorial) {
            AdvanceTutor();
        }
        return 1;
    case WM_DRAWITEM:
        lpdis = (DRAWITEMSTRUCT *)lParam;
        if (lpdis->itemID == -1) {
            HandleFocusState(lpdis, -2);
        } else {
            switch (lpdis->itemAction) {
            case ODA_DRAWENTIRE:
                DrawDlgLBEntireItem(lpdis, -4);
                break;
            case ODA_SELECT:
                DrawDlgLBEntireItem(lpdis, -4);
                break;
            case ODA_FOCUS:
                DrawDlgLBEntireItem(lpdis, -4);
            }
        }
        return 1;
    case WM_MEASUREITEM:
        lpmis = (MEASUREITEMSTRUCT *)lParam;
        lpmis->itemHeight = 66;
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        for (i = 2064; i <= 2069 && (HWND)lParam != GetDlgItem(hwnd, i); i++) {
        }
        if (i <= 2069 || (HWND)lParam == GetDlgItem(hwnd, IDC_SHIPLIST)) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (PtInRect(rgrcBuildSpin, PointFrom16(pt)) == 0 && PtInRect(&rgrcBuildSpin[1], PointFrom16(pt)) == 0)
            break;
        SetCursor(hcurHand);
        return 1;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        if (mdBuild != mdBuildEdit) {
            GetWindowRect(GetDlgItem(hwnd, IDC_DESIGNER_SHIPS), &rcGBox);
            ScreenToClient(hwnd, (POINT *)&rcGBox);
            GetWindowRect(GetDlgItem(hwnd, IDC_DESIGNER_STARBASES), &rc);
            ScreenToClient(hwnd, (POINT *)&rc.right);
            rcGBox.right = rc.right;
            rcGBox.bottom = rc.bottom;
            ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
            _Draw3dFrame(hdc, &rcGBox, -1);
            SetBkColor(hdc, crButtonFace);
            SelectObject(hdc, rghfontArial8[1]);
            cch = CchGetString(idsDesign, szWork);
            TextOut(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, cch);
            SelectObject(hdc, rghfontArial8[0]);
            GetWindowRect(GetDlgItem(hwnd, IDC_DESIGNER_EXISTING), &rcGBox);
            ScreenToClient(hwnd, (POINT *)&rcGBox);
            GetWindowRect(GetDlgItem(hwnd, IDC_DESIGNER_COMPONENTS), &rc);
            ScreenToClient(hwnd, (POINT *)&rc.right);
            rcGBox.right = rc.right;
            rcGBox.bottom = rc.bottom;
            ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
            _Draw3dFrame(hdc, &rcGBox, -1);
            SetBkColor(hdc, crButtonFace);
            SelectObject(hdc, rghfontArial8[1]);
            cch = CchGetString(idsView, szWork);
            TextOut(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, cch);
            SelectObject(hdc, rghfontArial8[0]);
        }
        GetClientRect(hwnd, &rc);
        DrawSlotDlg(hwnd, hdc, &rc, -1);
        DrawBuildSelComp(hwnd, hdc, -1);
        DrawBuildSelHull(hwnd, hdc, -1, NULL);
        EndPaint(hwnd, &ps);
        return 1;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
        return FTrackSlot(hwnd, LOWORD(lParam), HIWORD(lParam), wParam, FALSE, message == WM_RBUTTONDOWN);
    case WM_COMMAND:
        if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_DESIGNER_SHIPS && LOWORD(wParam) <= IDC_DESIGNER_STARBASES) {
            fStarbaseMode = LOWORD(wParam) - 2064;
            wParam = mdBuild + 2066;
            GetClientRect(hwnd, &rc);
            rc.left = rc.right >> 1 >= rc.right - 352 ? rc.right - 352 : rc.right >> 1;
            InvalidateRect(hwnd, &rc, TRUE);
            lpshdefBuild = NULL;
            fHullCopy = FALSE;
            lSel = !fStarbaseMode ? (uint32_t)rggrbitParts[0] : (uint32_t)rggrbitPartsSB[0];
            FillBuildPartsLB(GetDlgItem(hwnd, IDC_DESIGNER_COMPONENT_LIST), LOWORD(lSel));
            hwndItem = GetDlgItem(hwnd, IDC_COMBOBOX);
            FillBuildDD(hwndItem, mdBuild);
            SendMessage(hwndItem, CB_SETCURSEL, 0, 0);
            goto FixupShip;
        } else if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_DESIGNER_EXISTING && LOWORD(wParam) <= IDC_DESIGNER_COMPONENTS) {
            lSel = 0;
        LRestart:
            lpshdefBuild = NULL;
            fHullCopy = FALSE;
            mdBuild = LOWORD(wParam) - 2066;
            hwndItem = GetDlgItem(hwnd, IDC_COMBOBOX);
            UpdateSlotGlobals();
            FillBuildDD(hwndItem, mdBuild);
            SendMessage(hwndItem, CB_SETCURSEL, LOWORD(lSel), 0);
            SetWindowPos(hwndItem, NULL, ptslotGlob.x - 256 - (LOWORD(wParam) == IDC_DESIGNER_COMPONENTS ? 0 : 8), 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            GetClientRect(hwnd, &rc);
            left = rc.left;
            rc.left = rc.right >> 1 >= rc.right - 352 ? rc.right - 352 : rc.right >> 1;
            InvalidateRect(hwnd, &rc, TRUE);
            rc.right = rc.left;
            rc.left = left;
            rc.top = yBuildInfoSum;
            InvalidateRect(hwnd, &rc, TRUE);
            hwndItem = GetDlgItem(hwnd, IDC_DESIGNER_COMPONENT_LIST);
            ShowWindow(hwndItem, mdBuild == mdBuildComp ? SW_SHOW : SW_HIDE);
            if (LOWORD(wParam) != IDC_DESIGNER_COMPONENTS)
                goto FixupShip;
            if (gd.fTutorial) {
                AdvanceTutor();
            }
        } else if (LOWORD(wParam) == IDC_EDITNAME && HIWORD(wParam) == 0x400 && !fInEditUpdate) {
            fInEditUpdate = TRUE;
            GetWindowText((HWND)lParam, szWork, 250);
            lSel = SendMessage((HWND)lParam, EM_GETSEL, 0, 0);
            if (!FStringFitsScreen(szWork, 160)) {
                SetWindowText((HWND)lParam, szWork);
                SendMessage((HWND)lParam, EM_SETSEL, LOWORD(lSel), (int16_t)HIWORD(lSel));
            }
            lstrcpy(lpshdefBuild->hul.szClass, szWork);
            DrawBuildSelHull(hwnd, NULL, 256, NULL);
            fInEditUpdate = FALSE;
            if (gd.fTutorial) {
                AdvanceTutor();
            }
        } else if (LOWORD(wParam) == IDC_COMBOBOX) {
            switch (HIWORD(wParam)) {
            case 1:
            FixupShip:
                hwndItem = GetDlgItem(hwnd, IDC_COMBOBOX);
                lSel = SendMessage(hwndItem, CB_GETCURSEL, 0, 0);
                if (mdBuild == mdBuildComp || mdBuild == mdBuildEdit) {
                    if (lSel == -1) {
                        lSel = 0;
                    } else {
                        lSel = !fStarbaseMode ? (uint32_t)rggrbitParts[lSel] : (uint32_t)rggrbitPartsSB[lSel];
                    }
                    FillBuildPartsLB(GetDlgItem(hwnd, IDC_DESIGNER_COMPONENT_LIST), LOWORD(lSel));
                } else if (mdBuild == mdBuildShdef || mdBuild == mdBuildHuldef || mdBuild == mdBuildEnemyShdef) {
                    if (lSel == -1)
                        goto LClearSelection;
                    if (mdBuild == mdBuildShdef) {
                        fProtoSB = fStarbaseMode != 0 && GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh;
                        lpshdefBuild = NthValidShdef(LOWORD(lSel));
                        CshQueued(lpshdefBuild->ishdef, &fProgress, FALSE);
                        EnableWindow(GetDlgItem(hwndSlotDlg, IDC_EDIT), lpshdefBuild->cExist == 0 && !fProgress && (!fProtoSB || lSel > 0));
                        if (!fProtoSB)
                            goto LClearSelection;
                        EnableWindow(GetDlgItem(hwndSlotDlg, IDC_DELETE), lSel > 0 && lpshdefBuild->cExist == 0);
                        goto LClearSelection;
                    } else if (mdBuild == mdBuildEnemyShdef) {
                        lpshdefBuild = NthValidEnemyShdef(LOWORD(lSel));
                        if (lpshdefBuild->det == detAll)
                            goto LClearSelection;
                        i = lpshdefBuild->hul.ihuldef;
                        goto LClearSelection;
                    } else {
                        if (fStarbaseMode) {
                            part.hs.grhst = hstSBHull;
                            for (i = 0; i < 5; i++) {
                                part.hs.iItem = i;
                                if (FLookupPart(&part) == mdPartAvailAvailable && lSel-- <= 0)
                                    break;
                            }
                            i += 32;
                        } else {
                            part.hs.grhst = hstHull;
                            for (i = 0; i < 32; i++) {
                                part.hs.iItem = i;
                                if (FLookupPart(&part) == mdPartAvailAvailable && lSel-- <= 0)
                                    break;
                            }
                        }
                        shdefBuild.hul = LphuldefFromId(i)->hul;
                        shdefBuild.hul.ihuldef = i & 0xff;
                        for (i = 0; i < shdefBuild.hul.chs; i++) {
                            shdefBuild.hul.rghs[i].cItem = 0;
                        }
                        lpshdefBuild = &shdefBuild;
                        UpdateShdefCost(lpshdefBuild);
                    }
                LClearSelection:
                    UpdateSlotGlobals();
                    GetClientRect(hwnd, &rc);
                    rc.left = rc.right >> 1 >= rc.right - 352 ? rc.right - 352 : rc.right >> 1;
                    InvalidateRect(hwnd, &rc, TRUE);
                }
                SetBuildSelection(-2);
                DrawBuildSelHull(hwnd, NULL, -1, NULL);
                if (!lpshdefBuild) {
                    EnableWindow(GetDlgItem(hwndSlotDlg, IDC_DELETE), FALSE);
                    EnableWindow(GetDlgItem(hwndSlotDlg, IDC_EDIT), FALSE);
                    EnableWindow(GetDlgItem(hwndSlotDlg, IDC_IMPORT), FALSE);
                }
                if (gd.fTutorial) {
                    AdvanceTutor();
                }
                break;
            }
        } else if (LOWORD(wParam) == IDC_DESIGNER_COMPONENT_LIST) {
            if (HIWORD(wParam) == 1) {
                SetBuildSelection(-1);
            }
        } else if (LOWORD(wParam) == IDC_DELETE) {
            if (HIWORD(wParam) != 0)
                break;
            fProgress = FALSE;
            cshQueued = 0;
            if (gd.fTutorial && !FTutorialEnabledShipBuilder(tutsbDelete))
                break;
            hwndItem = GetDlgItem(hwnd, IDC_COMBOBOX);
            lSel = SendMessage(hwndItem, CB_GETCURSEL, 0, 0);
            lpshdef = NthValidShdef(LOWORD(lSel));
            if (lSel < 0 || !lpshdef)
                break;
            if (fStarbaseMode && lSel == 0 && GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh)
                break;
            if (!FCheckQueuedShip(hwnd, lpshdef, FALSE))
                break;
            lpshdef->fFree = TRUE;
            lpshdef->cBuilt = 0;
            lpshdef->cExist = 0;
            if (fStarbaseMode) {
                rgplr[idPlayer].cshdefSB += 15;
            } else {
                rgplr[idPlayer].cShDef--;
            }
            LogChangeShDef(lpshdef);
            FillBuildDD(hwndItem, mdBuild);
            lpshdefBuild = NthValidShdef(0);
            if ((fStarbaseMode && GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh) || !lpshdefBuild) {
                EnableWindow(GetDlgItem(hwndSlotDlg, IDC_DELETE), FALSE);
                EnableWindow(GetDlgItem(hwndSlotDlg, IDC_EDIT), FALSE);
                if (!lpshdefBuild) {
                    EnableWindow(GetDlgItem(hwndSlotDlg, IDC_IMPORT), FALSE);
                }
            }
            UpdateSlotGlobals();
            GetClientRect(hwnd, &rc);
            rc.left = rc.right >> 1 >= rc.right - 352 ? rc.right - 352 : rc.right >> 1;
            InvalidateRect(hwnd, &rc, TRUE);
            if (fHullCopy)
                goto LRestart;
        } else if (LOWORD(wParam) == IDC_IMPORT) {
            if (HIWORD(wParam) == 0) {
                if (gd.fTutorial && !FTutorialEnabledShipBuilder(tutsbCopy))
                    break;
                hwndItem = GetDlgItem(hwnd, IDC_COMBOBOX);
                lSel = SendMessage(hwndItem, CB_GETCURSEL, 0, 0);
                if (lSel < 0)
                    break;
                if (fStarbaseMode) {
                    for (i = 0; i < 10 && rglpshdefSB[idPlayer][i].fFree == 0; i++) {
                    }
                } else {
                    for (i = 0; i < 16 && rgshdef[i].fFree == 0; i++) {
                    }
                }
                if (mdBuild == mdBuildShdef || mdBuild == mdBuildEnemyShdef) {
                    if (mdBuild == mdBuildShdef) {
                        lpshdef = NthValidShdef(LOWORD(lSel));
                        if (lpshdef->fGift)
                            goto LStripDown;
                    } else {
                        lpshdef = NthValidEnemyShdef(LOWORD(lSel));
                    LStripDown:
                        if (fStarbaseMode) {
                            part.hs.grhst = hstSBHull;
                            part.hs.iItem = lpshdef->hul.ihuldef - ihuldefOrbitalFort;
                        } else {
                            part.hs.grhst = hstHull;
                            part.hs.iItem = lpshdef->hul.ihuldef;
                        }
                        if (FLookupPart(&part) != mdPartAvailAvailable) {
                            AlertSz(PszFormatIds(idsCantCopyShipDesignBecauseCantBuild, NULL), MB_ICONHAND);
                            return 0;
                        }
                    }
                    if (lSel < 0 || !lpshdef) {
                        return 0;
                    }
                    if (fStarbaseMode) {
                        rglpshdefSB[idPlayer][i] = *lpshdef;
                        lpshdef = rglpshdefSB[idPlayer] + i;
                    } else {
                        rgshdef[i] = *lpshdef;
                        lpshdef = &rgshdef[i];
                    }
                    lpshdef->cExist = 0;
                    lpshdef->cBuilt = 0;
                    if (mdBuild == mdBuildShdef && !lpshdef->fGift) {
                        MakeNewName(lpshdef->hul.szClass);
                    } else {
                        lpshdef->fGift = FALSE;
                        for (j = 0; j < lpshdef->hul.chs; j++) {
                            if (lpshdef->hul.rghs[j].cItem > 0) {
                                part.hs = lpshdef->hul.rghs[j];
                                if (FLookupPart(&part) != mdPartAvailAvailable) {
                                    lpshdef->hul.rghs[j].cItem = 0;
                                }
                            }
                        }
                    }
                } else {
                    if (fStarbaseMode) {
                        part.hs.grhst = hstSBHull;
                    } else {
                        part.hs.grhst = hstHull;
                    }
                    for (j = 0; j < (!fStarbaseMode ? 32 : 5); j++) {
                        part.hs.iItem = j;
                        if (FLookupPart(&part) == mdPartAvailAvailable && lSel-- <= 0)
                            break;
                    }
                    if (fStarbaseMode) {
                        lpshdef = rglpshdefSB[idPlayer] + i;
                        j += 32;
                    } else {
                        lpshdef = &rgshdef[i];
                    }
                    memset(lpshdef, 0, sizeof(SHDEF));
                    lpshdef->hul = LphuldefFromId(j)->hul;
                    lpshdef->det = detAll;
                    memset(lpshdef->hul.rghs, 0, 64);
                }
                CheckRadioButton(hwnd, IDC_DESIGNER_EXISTING, IDC_DESIGNER_COMPONENTS, IDC_DESIGNER_EXISTING);
                lpshdef->turn = game.turn;
                lpshdef->ishdef = (!fStarbaseMode ? 0 : 16) + i;
                UpdateShdefCost(lpshdef);
                if (fStarbaseMode) {
                    rgplr[idPlayer].cshdefSB++;
                } else {
                    rgplr[idPlayer].cShDef++;
                }
                LogChangeShDef(lpshdef);
                FillBuildDD(hwndItem, mdBuild);
                SendMessage(hwndItem, CB_SETCURSEL, i, 0);
                EnableWindow(GetDlgItem(hwndSlotDlg, IDC_EDIT), TRUE);
                lpshdefBuild = lpshdef;
                UpdateSlotGlobals();
                fHullCopy = TRUE;
                goto EditDesign;
            } else if (gd.fTutorial) {
                AdvanceTutor();
            }
        } else if (LOWORD(wParam) == IDC_EDIT) {
            if (gd.fTutorial && !FTutorialEnabledShipBuilder(tutsbEdit))
                break;
            if (fStarbaseMode && lpshdefBuild->ishdef == 16 && GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh)
                break;
            hwndItem = GetDlgItem(hwnd, IDC_COMBOBOX);
            lSel = SendMessage(hwndItem, CB_GETCURSEL, 0, 0);
            lpshdef = NthValidShdef(LOWORD(lSel));
            if (lSel < 0 || !lpshdef)
                break;
            if (!fStarbaseMode && !FCheckQueuedShip(hwnd, lpshdef, TRUE))
                break;
        EditDesign:
            if (!lpshdefBuild)
                break;
            InvalidateRect(hwnd, NULL, TRUE);
            mdBuild = mdBuildEdit;
            ishdefBuild = lpshdefBuild->ishdef;
            shdefBuild = *lpshdefBuild;
            lpshdefBuild = &shdefBuild;
            FillBuildPartsLB(GetDlgItem(hwnd, IDC_DESIGNER_COMPONENT_LIST), !fStarbaseMode ? 6655 : 2620);
            FillBuildDD(GetDlgItem(hwnd, IDC_COMBOBOX), mdBuild);
            ShowMainControls(hwnd, SW_HIDE);
            SetWindowPos(GetDlgItem(hwnd, IDC_DESIGNER_COMPONENT_LIST), NULL, 16, 32, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(hwnd, IDC_COMBOBOX), NULL, 16, 8, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
            SetWindowPos(GetDlgItem(hwnd, IDC_EDITNAME), NULL, ptslotGlob.x - 264, 8, 240, 3 * dyArial8 >> 1, SWP_NOZORDER | SWP_SHOWWINDOW);
            SetWindowText(GetDlgItem(hwnd, IDC_EDITNAME), shdefBuild.hul.szClass);
            SendMessage(GetDlgItem(hwnd, IDC_EDITNAME), EM_LIMITTEXT, 0x1f, 0);
            if (gd.fTutorial) {
                AdvanceTutor();
            }
        } else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            if (mdBuild == mdBuildEdit) {
                lSel = 0;
                if (LOWORD(wParam) == IDOK) {
                    if (!fStarbaseMode && shdefBuild.hul.rghs[0].cItem == 0) {
                        AlertSz(PszFormatIds(idsShipDesignDoesHaveAnyEnginesMust, NULL), MB_ICONHAND);
                        return 0;
                    }
                    if (gd.fTutorial && !FTutorialEnabledShipBuilder(tutsbAccept)) {
                        break;
                    }
                    GetWindowText(GetDlgItem(hwnd, IDC_EDITNAME), shdefBuild.hul.szClass, 32);
                    shdefBuild.cBuilt = 0;
                    shdefBuild.cExist = 0;
                    shdefBuild.fFree = FALSE;
                    UpdateShdefCost(&shdefBuild);
                    if (fStarbaseMode) {
                        ishdefBuild -= 16;
                        rglpshdefSB[idPlayer][ishdefBuild] = shdefBuild;
                        LogChangeShDef(&shdefBuild);
                        for (i = 0; i < ishdefBuild; i++) {
                            if (!rglpshdefSB[idPlayer][i].fFree) {
                                lSel++;
                            }
                        }
                        ishdefBuild += 16;
                    } else {
                        rgshdef[ishdefBuild] = shdefBuild;
                        LogChangeShDef(&rgshdef[ishdefBuild]);
                        for (i = 0; i < ishdefBuild; i++) {
                            if (!rgshdef[i].fFree) {
                                lSel++;
                            }
                        }
                    }
                } else if (gd.fTutorial && !FTutorialEnabledShipBuilder(tutsbCancelEdit)) {
                    break;
                }
                InvalidateRect(hwnd, NULL, TRUE);
                ShowMainControls(hwnd, SW_SHOW);
                ShowWindow(GetDlgItem(hwnd, IDC_EDITNAME), SW_HIDE);
                hwndItem = GetDlgItem(hwnd, IDC_DESIGNER_COMPONENT_LIST);
                FillBuildPartsLB(hwndItem, !fStarbaseMode ? rggrbitParts[0] : rggrbitPartsSB[0]);
                SetWindowPos(hwndItem, NULL, ptslotGlob.x - 256, 32, 240, 266, SWP_NOZORDER);
                ShowWindow(GetDlgItem(hwnd, IDC_DESIGNER_COMPONENT_LIST), SW_HIDE);
                if (fHullCopy) {
                    if (LOWORD(wParam) != IDCANCEL && !fStarbaseMode && lpshdefBuild->hul.rghs[0].cItem == 0) {
                        wParam = IDCANCEL;
                    }
                    if (LOWORD(wParam) == IDCANCEL) {
                        if (fStarbaseMode) {
                            shdefBuild.fFree = TRUE;
                            rglpshdefSB[idPlayer][ishdefBuild - 16] = shdefBuild;
                            rgplr[idPlayer].cshdefSB += 15;
                            LogChangeShDef(&shdefBuild);
                        } else {
                            rgshdef[ishdefBuild].fFree = TRUE;
                            rgplr[idPlayer].cShDef--;
                            LogChangeShDef(&rgshdef[ishdefBuild]);
                        }
                    }
                }
                wParam = IDC_DESIGNER_EXISTING;
                goto LRestart;
            } else {
                SetBuildSelection(-2);
                StickyDlgPos(hwnd, &ptStickySlotDlg, FALSE);
                hwndSlotDlg = 0;
                EndDialog(hwnd, LOWORD(wParam) == IDOK);
                if (gd.fTutorial) {
                    AdvanceTutor();
                }
                return 1;
            }
        } else if (LOWORD(wParam) == IDC_HELP) {
            WinHelp(hwnd, szHelpFile, HELP_CONTEXT, (uint32_t)(mdBuild == mdBuildEdit ? 3039 : 1066));
            return 1;
        }
        break;
    }
    return 0;
}

void DrawSlotDlg(HWND hwnd, HDC hdc, RECT *prc, int16_t iDraw) {
    int16_t  yTop;
    int16_t  iMax;
    int16_t  cSlot;
    int16_t  fCreatedDC;
    HDC      hdcMem;
    int16_t  c;
    int16_t  i;
    int16_t  bkMode;
    int16_t  j;
    int16_t  cItem;
    int16_t  ibmp;
    HBITMAP  hbmpSav;
    int16_t  xLeft;
    PART     part;
    HULDEF  *lphuldef;
    RECT     rc;
    int16_t  iInventSel;
    HPEN     hpenSav;
    HBRUSH   hbrSav;
    COLORREF crBkSav;

    fCreatedDC = FALSE;
    if (mdBuild != mdBuildComp && lpshdefBuild) {
        lphuldef = LphuldefFromId(lpshdefBuild->hul.ihuldef);
        cSlot = lphuldef->hul.chs;
        if (!hdc) {
            fCreatedDC = TRUE;
            hdc = GetDC(hwnd);
        }
        if (!hwndSlotDlg) {
            xLeft = 4;
            yTop = 6;
        } else {
            xLeft = ptslotGlob.x - 338;
            yTop = 6;
        }
        DrawFleetBitmap(NULL, hdc, xLeft, yTop, TRUE, lpshdefBuild->hul.ibmp, 0, FALSE, -1, 0);
        hdcMem = CreateCompatibleDC(hdc);
        hbmpSav = SelectObject(hdcMem, hbmpScanner);
        iInventSel = 0;
        if (mdBuild != mdBuildEdit) {
            rgrcBuildSpin[1].top = -5;
            rgrcBuildSpin[0].top = -5;
            rgrcBuildSpin[1].bottom = -6;
            rgrcBuildSpin[0].bottom = -6;
        } else {
            SetRect(rgrcBuildSpin, xLeft + 21, yTop + 69, xLeft + 35, yTop + 83);
            rgrcBuildSpin[1] = rgrcBuildSpin[0];
            OffsetRect(&rgrcBuildSpin[1], 14, 0);
            for (i = 0; i < 2; i++) {
                DrawBtn(hdc, &rgrcBuildSpin[i], (i == 0 ? 2 : 3) | 0x20, FALSE, NULL);
            }
        }
        SelectObject(hdc, rghfontArial8[1]);
        bkMode = SetBkMode(hdc, TRANSPARENT);
        if (iDraw == -1) {
            i = 0;
            iMax = cSlot;
        } else {
            i = iDraw;
            iMax = iDraw + 1;
        }
        if (lphuldef->hul.wtCargoMax != 0) {
            rc = rcCargo;
            if (fStarbaseMode && (lphuldef->hul.ihuldef == ihuldefSpaceDock || lphuldef->hul.ihuldef == ihuldefDeathStart)) {
                hbrSav = SelectObject(hdc, hbrDock);
                hpenSav = SelectObject(hdc, GetStockObject(BLACK_PEN));
                Ellipse(hdc, rc.left - 12, rc.top - 12, rc.right + 12, rc.bottom + 12);
                SelectObject(hdc, hpenSav);
                SelectObject(hdc, hbrSav);
            } else {
                FillRect(hdc, &rcCargo, GetStockObject(BLACK_BRUSH));
                rc.top++;
                rc.bottom--;
                rc.left++;
                rc.right--;
                FillRect(hdc, &rc, !fStarbaseMode ? hbrCargo : hbrDock);
            }
            rc.bottom = (int16_t)(rc.bottom - rc.top) / 2 + rc.top;
            if (!fStarbaseMode) {
                RcCtrTextOut(hdc, &rc, PszGetCompressedString(idsCargo3), 0);
                c = wsprintf(szWork, PCTDKT, WtMaxShdefStat(lpshdefBuild, 2));
                RcCtrTextOut(hdc, &rcCargo, szWork, 0);
            } else {
                if ((uint32_t)lphuldef->hul.wtCargoMax == 0xffff) {
                    RcCtrTextOut(hdc, &rc, PszGetCompressedString(idsUnlimited), 0);
                } else {
                    c = wsprintf(szWork, PCTDKT, lphuldef->hul.wtCargoMax);
                    RcCtrTextOut(hdc, &rc, szWork, 0);
                }
                RcCtrTextOut(hdc, &rcCargo, PszGetCompressedString(idsSpace), 0);
            }
            rc.top = rc.bottom;
            rc.bottom = rcCargo.bottom - 1;
            if (fStarbaseMode) {
                RcCtrTextOut(hdc, &rc, PszGetCompressedString(idsDock), 0);
            } else {
                RcCtrTextOut(hdc, &rc, PszGetCompressedString(idsMax), 0);
            }
        }
        for (; i < iMax; i++) {
            FillRect(hdc, &vrgrcSlot[i], hbr50Screen);
            cItem = lpshdefBuild->hul.rghs[i].cItem;
            if (cItem > 0) {
                part.hs = lpshdefBuild->hul.rghs[i];
                FLookupPart(&part);
                ibmp = part.pcom->ibmp;
                iInventSel = ibmp >> 5;
                DibBlt(hdc, vrgrcSlot[i].left, vrgrcSlot[i].top, 64, 64, rghdibInventory[iInventSel], (ibmp & 7) * 0x40, (3 - (ibmp >> 3 & 3)) * 0x40, 64, 64,
                       13369376);
                if (mdBuild != mdBuildEdit) {
                    SelectObject(hdcMem, hbmpScanner);
                    for (j = 0; j < 4; j++) {
                        BitBlt(hdc, (j >= 2 ? 57 : 3) + vrgrcSlot[i].left, (!(j & 1) ? 57 : 3) + vrgrcSlot[i].top, 4, 4, hdcMem, 22, 33, SRCCOPY);
                    }
                }
                if (cItem == 1 && lphuldef->hul.rghs[i].cItem == 1 && part.hs.grhst == hstSpecialSB) {
                    szWork[0] = 0;
                    c = 0;
                } else {
                    c = wsprintf(szWork, PszGetCompressedString(idsDD), cItem, lphuldef->hul.rghs[i].cItem);
                }
            } else {
                SelectObject(hdcMem, hbmpBackBld);
                ibmp = IEmptyBmpFromGrhst(lphuldef->hul.rghs[i].grhst);
                BitBlt(hdc, vrgrcSlot[i].left, vrgrcSlot[i].top, 64, 64, hdcMem, (ibmp & 7) * 0x40, (ibmp >> 3 & 3) * 0x40, SRCCOPY);
                iInventSel = -1;
                if (lphuldef->hul.rghs[i].grhst & hstEngine) {
                    c = wsprintf(szWork, PszGetCompressedString(idsNeedsD), lphuldef->hul.rghs[i].cItem);
                } else {
                    c = wsprintf(szWork, PszGetCompressedString(idsD3), lphuldef->hul.rghs[i].cItem);
                }
            }
            CtrTextOut(hdc, vrgrcSlot[i].left + 32, vrgrcSlot[i].bottom - dyArial6 - 4, szWork, c);
            if (iselSlot == i) {
                crBkSav = SetBkColor(hdc, 0xffffff);
                FrameRect(hdc, &vrgrcSlot[i], hbr50Screen);
                ExpandRc(&vrgrcSlot[i], -1, -1);
                FrameRect(hdc, &vrgrcSlot[i], hbr50Screen);
                ExpandRc(&vrgrcSlot[i], 1, 1);
                SetBkColor(hdc, crBkSav);
            }
        }
        if ((!hwndPopup || !GlobalPD.fHideCounts) && mdBuild == mdBuildShdef) {
            SetRect(&rc, ptPlaque.x, ptPlaque.y, ptPlaque.x + 60, ptPlaque.y + 30);
            SelectPalette(hdc, vhpal, FALSE);
            RealizePalette(hdc);
            DibBlt(hdc, ptPlaque.x, ptPlaque.y, 60, 30, hdibPlaque, 0, 0, 60, 30, 13369376);
            c = wsprintf(szWork, PszGetCompressedString(idsLdLd), lpshdefBuild->cExist, lpshdefBuild->cBuilt);
            SelectObject(hdc, rghfontArial8[1]);
            if (LOWORD(GetTextExtent(hdc, szWork, c)) > 50) {
                SelectObject(hdc, rghfontArial7[0]);
                if (LOWORD(GetTextExtent(hdc, szWork, c)) > 50) {
                    SelectObject(hdc, rghfontArial6[0]);
                }
            }
            RcCtrTextOut(hdc, &rc, szWork, c);
        }
        SetBkMode(hdc, bkMode);
        SelectObject(hdcMem, hbmpSav);
        DeleteDC(hdcMem);
        if (fCreatedDC) {
            ReleaseDC(hwnd, hdc);
        }
    }
    return;
}

int16_t FTrackSlot(HWND hwnd, int16_t x, int16_t y, int16_t fkb, int16_t fListBox, int16_t fRightBtn) {
    HDC     hdc;
    POINT16 ptOld;
    POINT16 ptTileSize;
    int16_t ibmpY;
    POINT16 pt;
    int16_t cSlot;
    int16_t iSrc;
    POINT16 ptDNew;
    int16_t ibmpX;
    HDC     hdcMem;
    int16_t i;
    HBITMAP hbmpFullSav;
    RECT    rcStart;
    int16_t fUseMem;
    HBITMAP hbmpScreen;
    int16_t ibmp;
    HBITMAP hbmpOld;
    HDC     hdcMemFull;
    POINT16 ptD;
    int16_t iSel;
    HBITMAP hbmpSav;
    HS      hs;
    int16_t fFirst;
    PART    part;
    RECT    rc;
    int16_t iDir;
    int16_t yTop;
    int16_t bt;
    BTNT    btnt;
    RECT   *prc;
    int16_t iBase;
    int16_t iCur;
    int16_t xLeft;
    int16_t dyStart;
    int16_t dxStart;

    fFirst = TRUE;
    if (!lpshdefBuild) {
        return FALSE;
    }
    pt.x = x;
    pt.y = y;
    cSlot = LphuldefFromId(lpshdefBuild->hul.ihuldef)->hul.chs;
    if (!fRightBtn && hwndSlotDlg && (PtInRect(rgrcBuildSpin, PointFrom16(pt)) != 0 || PtInRect(&rgrcBuildSpin[1], PointFrom16(pt)) != 0)) {
        if (PtInRect(rgrcBuildSpin, PointFrom16(pt)) != 0) {
            iDir = -1;
            bt = 34;
            prc = rgrcBuildSpin;
        } else {
            iDir = 1;
            bt = 35;
            prc = &rgrcBuildSpin[1];
        }
        iBase = lpshdefBuild->hul.ibmp;
        iCur = iBase & 3;
        iBase -= iCur;
        xLeft = ptslotGlob.x - 336;
        yTop = 8;
        InitBtnTrack(&btnt, hwnd, NULL, prc, bt, 80, FALSE, FALSE, NULL);
        while (FTrackBtn(&btnt)) {
            iCur = (iCur + 4 + iDir) & 3;
            DrawFleetBitmap(NULL, btnt.hdc, xLeft, yTop, FALSE, iBase + iCur, 0, FALSE, -1, 0);
        }
        lpshdefBuild->hul.ibmp = iBase + iCur;
        return TRUE;
    }
    if (!fListBox) {
        GetClientRect(hwnd, &rc);
        for (iSrc = 0; iSrc < cSlot && PtInRect(&vrgrcSlot[iSrc], PointFrom16(pt)) == 0; iSrc++) {
        }
        if (iSrc == cSlot) {
            return FALSE;
        }
        hs = lpshdefBuild->hul.rghs[iSrc];
        if (hs.cItem <= 0) {
            SetBuildSelection(iSrc);
            return FALSE;
        }
        part.hs = hs;
        FLookupPart(&part);
        ibmp = part.pcom->ibmp;
        rcStart = vrgrcSlot[iSrc];
    } else {
        iSel = LOWORD(SendMessage(hwnd, LB_GETCURSEL, 0, 0));
        if (iSel == -1) {
            return FALSE;
        }
        SendMessage(hwnd, LB_GETTEXT, iSel, (LPARAM)szWork);
        ibmp = szWork[2] - 'A' + (szWork[3] - 'A') * 26;
        iSrc = -1;
        hs.grhst = 1 << (szWork[0] - 'A');
        hs.iItem = szWork[1] - 'A';
        hs.cItem = 1;
        rcStart.left = 2;
        rcStart.right = 66;
        rcStart.top = y / 66 * 66 + 1;
        rcStart.bottom = rcStart.top + 64;
        ClientToScreen(hwnd, (POINT *)&rcStart);
        ClientToScreen(hwnd, (POINT *)&rcStart.right);
        ClientToScreen16(hwnd, &pt);
        hwnd = hwndSlotDlg;
        ScreenToClient(hwnd, (POINT *)&rcStart);
        ScreenToClient(hwnd, (POINT *)&rcStart.right);
        ScreenToClient16(hwnd, &pt);
        x = pt.x;
        y = pt.y;
        if (hs.grhst == hstEngine && hs.iItem == iengineSettlersDelight && lpshdefBuild->hul.ihuldef != ihuldefMiniColonyShip) {
            GlobalPD.grPopup = grPopupString;
            GlobalPD.dxOut = 180;
            GlobalPD.psz = szPopupBuffer;
            CchGetString(idsSettlersDelightEngineMayMountedDesignsBased, szPopupBuffer);
            Popup(hwnd, x, y);
            return TRUE;
        }
        if (hs.grhst == hstSpecialM && hs.iItem == ispecialMOrbitalConstructionModule && lpshdefBuild->hul.ihuldef != ihuldefColonyShip) {
            GlobalPD.grPopup = grPopupString;
            GlobalPD.dxOut = 180;
            GlobalPD.psz = szPopupBuffer;
            CchGetString(idsOrbitalConstructionModuleMayMountedDesignsBased, szPopupBuffer);
            Popup(hwnd, x, y);
            return TRUE;
        }
        if (hs.grhst == hstSpecialE && hs.iItem == ispecialETransportCloaking && LphuldefFromId(lpshdefBuild->hul.ihuldef)->imdAttack != hullAttackNone) {
            GlobalPD.grPopup = grPopupString;
            GlobalPD.dxOut = 180;
            GlobalPD.psz = szPopupBuffer;
            CchGetString(idsTransportCloakingModuleMayPlacedHullCould, szPopupBuffer);
            Popup(hwnd, x, y);
            return TRUE;
        }
    }
    SetBuildSelection(iSrc);
    if (fRightBtn) {
        GlobalPD.part = part;
        GlobalPD.grPopup = grPopupComponent;
        Popup(hwnd, x, y);
        return TRUE;
    }
    if (mdBuild != mdBuildEdit) {
        return FALSE;
    }
    ptTileSize.y = 64;
    ptTileSize.x = 64;
    ibmpX = ibmp & 7;
    ibmpY = ibmp >> 3 & 3;
    hdc = GetDC(hwnd);
    SelectPalette(hdc, vhpal, FALSE);
    RealizePalette(hdc);
    hdcMem = CreateCompatibleDC(hdc);
    hdcMemFull = CreateCompatibleDC(hdc);
    SelectPalette(hdcMemFull, vhpal, FALSE);
    RealizePalette(hdcMemFull);
    hbmpOld = CreateCompatibleBitmap(hdc, ptTileSize.x, ptTileSize.y);
    hbmpSav = SelectObject(hdcMem, hbmpOld);
    hbmpScreen = CreateCompatibleBitmap(hdc, 3 * ptTileSize.x, 3 * ptTileSize.y);
    hbmpFullSav = SelectObject(hdcMemFull, hbmpScreen);
    SetCapture(hwnd);
    ptOld.y = -1;
    ptOld.x = -1;
    while (FGetMouseMove(&pt)) {
        if (pt.x != ptOld.x || pt.y != ptOld.y) {
            if (fFirst) {
                fUseMem = FALSE;
                fFirst = FALSE;
            } else {
                SelectObject(hdcMem, hbmpOld);
                ptDNew.x = pt.x - ptOld.x;
                ptDNew.y = pt.y - ptOld.y;
                fUseMem = abs(ptDNew.x) < ptTileSize.x && abs(ptDNew.y) < ptTileSize.y;
                if (!fUseMem) {
                    BitBlt(hdc, rcStart.left + ptD.x, rcStart.top + ptD.y, ptTileSize.x, ptTileSize.y, hdcMem, 0, 0, SRCCOPY);
                } else {
                    BitBlt(hdcMemFull, 0, 0, 3 * ptTileSize.x, 3 * ptTileSize.y, hdc, rcStart.left + ptD.x - ptTileSize.x, rcStart.top + ptD.y - ptTileSize.y,
                           SRCCOPY);
                    BitBlt(hdcMemFull, ptTileSize.x, ptTileSize.y, ptTileSize.x, ptTileSize.y, hdcMem, 0, 0, SRCCOPY);
                }
            }
            ptOld = pt;
            ptD.x = pt.x - x;
            ptD.y = pt.y - y;
            SelectObject(hdcMem, hbmpOld);
            if (!fUseMem) {
                BitBlt(hdcMem, 0, 0, ptTileSize.x, ptTileSize.y, hdc, rcStart.left + ptD.x, rcStart.top + ptD.y, SRCCOPY);
                DibBlt(hdc, rcStart.left + ptD.x, rcStart.top + ptD.y, 64, 64, rghdibInventory[ibmp / 32], ibmpX * 64, (3 - ibmpY) * 64, 64, 64, 13369376);
            } else {
                dxStart = 0;
                dyStart = 0;
                BitBlt(hdcMem, 0, 0, ptTileSize.x, ptTileSize.y, hdcMemFull, ptTileSize.x + ptDNew.x, ptTileSize.y + ptDNew.y, SRCCOPY);
                DibBlt(hdcMemFull, ptTileSize.x + ptDNew.x, ptTileSize.y + ptDNew.y, 64, 64, rghdibInventory[ibmp / 32], ibmpX * 64, (3 - ibmpY) * 64, 64, 64,
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
                BitBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hdcMemFull, ptTileSize.x + ptDNew.x - dxStart,
                       ptTileSize.y + ptDNew.y - dyStart, SRCCOPY);
            }
            i = IDropPart(pt, hs, iSrc, TRUE);
            if (i < 0 || i == 2) {
                SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(32512)));
            } else if (i == 1 && iSrc >= 0) {
                SetCursor(hcurTrashCan);
            } else {
                SetCursor(hcurNoWay);
            }
        }
    }
    if (!fFirst) {
        SelectObject(hdcMem, hbmpOld);
        BitBlt(hdc, rcStart.left + ptD.x, rcStart.top + ptD.y, 64, 64, hdcMem, 0, 0, SRCCOPY);
    }
    ReleaseCapture();
    SelectObject(hdcMem, hbmpSav);
    SelectObject(hdcMemFull, hbmpFullSav);
    DeleteObject(hbmpOld);
    DeleteObject(hbmpScreen);
    DeleteDC(hdcMem);
    DeleteDC(hdcMemFull);
    i = IDropPart(pt, hs, iSrc, FALSE);
    ReleaseDC(hwnd, hdc);
    return TRUE;
}

void DrawBuildSelComp(HWND hwnd, HDC hdc, int16_t iDraw) {
    uint16_t grhst;
    HS       hsShip;
    uint16_t rgCosts[4];
    int16_t  fCreatedDC;
    int16_t  c;
    int16_t  i;
    COLORREF crForeSav;
    int16_t  fPlural;
    int16_t  k;
    char     szWord[80];
    COLORREF crBackSav;
    HS       hsHul;
    int16_t  cch;
    PART     part;
    int16_t  x;
    int16_t  dxkT;
    RECT     rc;
    int16_t  iSel;
    char    *pch;
    int16_t  yTop; /* NATIVE: RECT.top is 32-bit; WrapTextOut takes int16_t * */

    fCreatedDC = FALSE;
    if (!hdc) {
        fCreatedDC = TRUE;
        hdc = GetDC(hwnd);
    }
    GetClientRect(hwndSlotDlg, &rc);
    rc.bottom -= 8;
    rc.left = 8;
    rc.right = rc.left + 256;
    rc.top = yBuildInfoSum;
    SelectObject(hdc, rghfontArial8[1]);
    crForeSav = SetTextColor(hdc, 0);
    crBackSav = SetBkColor(hdc, crButtonFace);
    FillRect(hdc, &rc, hbrButtonFace);
    if (fStarbaseMode) {
        rc.top += dyArial8;
    }
    if (iselSlot != -2) {
        if (iselSlot == -1) {
            iSel = LOWORD(SendMessage(GetDlgItem(hwndSlotDlg, IDC_DESIGNER_COMPONENT_LIST), LB_GETCURSEL, 0, 0));
            if (iSel == -1)
                goto Restore;
            SendMessage(GetDlgItem(hwndSlotDlg, IDC_DESIGNER_COMPONENT_LIST), LB_GETTEXT, iSel, (LPARAM)szWork);
            hsShip.cItem = 1;
            hsShip.grhst = 1 << (szWork[0] - 'A');
            hsShip.iItem = szWork[1] - 'A';
            goto HullPart;
        } else {
            if (!lpshdefBuild)
                goto Restore;
            hsShip = lpshdefBuild->hul.rghs[iselSlot];
            hsHul = LphuldefFromId(lpshdefBuild->hul.ihuldef)->hul.rghs[iselSlot];
            if (hsShip.cItem == 0) {
                i = CchGetString(!(hsHul.grhst & hstEngine) ? idsCanHold : idsRequiresExactly, szWork);
                fPlural = hsHul.cItem != 1;
                if (!fPlural) {
                    i += CchGetString(idsOne, &szWork[i]);
                } else {
                    i += wsprintf(&szWork[i], "%d ", hsHul.cItem);
                }
                grhst = hsHul.grhst;
                while (grhst != 0) {
                    for (i = 0; i < 14; i++) {
                        if ((grhst & rghstCat[i]) == rghstCat[i]) {
                            grhst &= ~rghstCat[i];
                            break;
                        }
                    }
                    cch = CchGetString(rgidsCat[i], szWord);
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
                                strcpy(pch + -2, pch + -1);
                            }
                        }
                    }
                    if (grhst != 0) {
                        if ((grhst - 1) & grhst) {
                            strcat(szWord, ", ");
                        } else {
                            strcat(szWord, PszGetCompressedString(idsOr));
                        }
                    }
                    strcat(szWork, szWord);
                }
                x = rc.left;
                /* NATIVE: original passed &rc.top */
                yTop = rc.top;
                WrapTextOut(hdc, &x, &yTop, szWork, 0, rc.left, rc.right - rc.left, NULL, FALSE, TRUE);
                rc.top = yTop;
                rc.top += dyArial8;
                goto Restore;
            }
        }
    HullPart:
        part.hs = hsShip;
        FLookupPart(&part);
        dxkT = LOWORD(GetTextExtent(hdc, PszGetCompressedString(idsKt), 2));
        fPlural = hsShip.cItem != 1;
        if (!fPlural) {
            CchGetString(idsOne, szWord);
        } else {
            wsprintf(szWord, "%d ", hsShip.cItem);
        }
        strcat(szWord, part.pcom->szName);
        if (fPlural) {
            strcat(szWord, "s");
        }
        cch = wsprintf(szWork, PszGetCompressedString(idsCostS), szWord);
        TextOut(hdc, rc.left, rc.top, szWork, cch);
        rc.left += 8;
        rc.right -= 8;
        c = hsShip.cItem;
        GetTruePartCost(idPlayer, &part, rgCosts);
        for (k = 0; k < 3; k++) {
            rc.top += dyArial8;
            SelectObject(hdc, rghfontArial8[1]);
            SetTextColor(hdc, rgcrMinerals[k]);
            TextOut(hdc, rc.left, rc.top, rgszMinerals[k], lstrlen(rgszMinerals[k]));
            SelectObject(hdc, rghfontArial8[0]);
            SetTextColor(hdc, crWindowText);
            cch = wsprintf(szWork, PCTLD, (uint32_t)(c * (uint32_t)rgCosts[k]));
            RightTextOut(hdc, rc.right - dxkT - 64, rc.top, szWork, cch, dxMaxMineralQuan);
            TextOut(hdc, rc.right - dxkT - 64, rc.top, PszGetCompressedString(idsKt), 2);
        }
        rc.top += dyArial8;
        SelectObject(hdc, rghfontArial8[1]);
        SetTextColor(hdc, rgcrMinerals[5]);
        TextOut(hdc, rc.left, rc.top, rgszMinerals[5], lstrlen(rgszMinerals[5]));
        SelectObject(hdc, rghfontArial8[0]);
        SetTextColor(hdc, crWindowText);
        cch = wsprintf(szWork, PCTLD, (uint32_t)(c * (uint32_t)rgCosts[3]));
        RightTextOut(hdc, rc.right - dxkT - 64, rc.top, szWork, cch, dxMaxMineralQuan);
        if (!fStarbaseMode) {
            rc.left -= 8;
            rc.top += dyArial8;
            SelectObject(hdc, rghfontArial8[1]);
            cch = wsprintf(szWork, PszGetCompressedString(idsMassLdkt), (uint32_t)(c * part.pcom->cMass));
            TextOut(hdc, rc.left, rc.top, szWork, cch);
        }
    }
Restore:
    SelectObject(hdc, rghfontArial8[0]);
    SetTextColor(hdc, crForeSav);
    SetBkColor(hdc, crBackSav);
    if (fCreatedDC) {
        ReleaseDC(hwnd, hdc);
    }
    return;
}

void DrawBuildSelHull(HWND hwnd, HDC hdc, int16_t iDraw, RECT *prc) {
    char     rgch[20];
    DV       dv;
    uint16_t rgCosts[4];
    int16_t  fCreatedDC;
    COLORREF crForeSav;
    int16_t  dxMineral;
    int16_t  k;
    COLORREF crBackSav;
    int16_t  csh;
    HUL     *lphul;
    int32_t  dpShield;
    int16_t  cch;
    int32_t  dp;
    int16_t  dxkT;
    RECT     rc;
    int32_t  lwt;
    int16_t  i;
    int16_t  j;
    int16_t  dPlanRange;
    int16_t  dRange;
    int16_t  pctDetect;
    int16_t  pct;

    fCreatedDC = FALSE;
    if (mdBuild != mdBuildComp) {
        if (!hdc) {
            fCreatedDC = TRUE;
            hdc = GetDC(hwnd);
        }
        if (!prc) {
            GetClientRect(hwnd, &rc);
            rc.bottom -= 32;
            rc.right -= 4;
            rc.left = rc.right - 320;
            rc.top = (!fStarbaseMode ? 0 : dyArial8) + yBuildInfoSum;
            FillRect(hdc, &rc, hbrButtonFace);
        } else {
            rc = *prc;
        }
        if (!lpshdefBuild)
            goto LReleaseDC;
        lphul = &lpshdefBuild->hul;
        SelectObject(hdc, rghfontArial8[0]);
        dxkT = LOWORD(GetTextExtent(hdc, PszGetCompressedString(idsKt), 2));
        SelectObject(hdc, rghfontArial8[1]);
        dxMineral = LOWORD(GetTextExtent(hdc, rgszMinerals[2], strlen(rgszMinerals[2]))) + 6;
        crForeSav = SetTextColor(hdc, 0);
        crBackSav = SetBkColor(hdc, crButtonFace);
        if (hwndPopup && GlobalPD.grPopup == grPopupShdef && GlobalPD.fToken) {
            GetVCRStats(viVCRFocus, &dp, &dv, &dpShield, &csh);
            dpShield = (uint32_t)vrgtok[viVCRFocus].dpShield - dpShield;
            if (dpShield < 0) {
                dpShield = 0;
            }
            dpShield = (uint32_t)(dpShield * csh);
            if (csh == 0)
                goto LDeadToken;
        } else {
            dp = (uint32_t)lphul->dp;
            dpShield = DpShieldOfShdef(lpshdefBuild, idPlayer);
        }
        if (hwndPopup && fStarbaseMode) {
            rc.top += dyArial8;
            cch = CchGetString(idsCost, szWork);
        } else {
            CchGetString(idsHull, rgch);
            cch = wsprintf(szWork, PszGetCompressedString(idsCostOneSS), lphul->szClass, mdBuild == mdBuildHuldef ? rgch : "");
        }
        TextOut(hdc, rc.left, rc.top, szWork, cch);
        rc.left += 8;
        rc.right -= 8;
        GetTrueHullCost(idPlayer, lphul, rgCosts);
        if (fStarbaseMode && (GetRaceGrbit(&rgplr[idPlayer], ibitRaceISB) != 0 || GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh)) {
            for (k = 0; k < 4; k++) {
                rgCosts[k] -= (uint32_t)rgCosts[k] / 5;
            }
        }
        if (fStarbaseMode) {
            for (k = 0; k < 4; k++) {
                rgCosts[k] -= (uint32_t)rgCosts[k] / 2;
            }
        }
        for (k = 0; k <= 5; k++) {
            if (k == 3) {
                k = 5;
            }
            rc.top += dyArial8;
            SelectObject(hdc, rghfontArial8[1]);
            SetTextColor(hdc, rgcrMinerals[k]);
            TextOut(hdc, rc.left, rc.top, rgszMinerals[k], lstrlen(rgszMinerals[k]));
            SelectObject(hdc, rghfontArial8[0]);
            SetTextColor(hdc, crWindowText);
            cch = wsprintf(szWork, PCTD, k == 5 ? rgCosts[3] : rgCosts[k]);
            RightTextOut(hdc, rc.left + dxMineral + dxMaxMineralQuan - dxkT, rc.top, szWork, cch, dxMaxMineralQuan);
            if (k < 5) {
                TextOut(hdc, rc.left + dxMineral + dxMaxMineralQuan - dxkT, rc.top, PszGetCompressedString(idsKt), 2);
            }
        }
        rc.left -= 8;
        rc.top += dyArial8;
        SelectObject(hdc, rghfontArial8[1]);
        if (!fStarbaseMode) {
            if (hwndPopup && GlobalPD.grPopup == grPopupShdef && GlobalPD.fToken) {
                lwt = (uint32_t)vrgtok[viVCRFocus].wt;
            } else {
                lwt = (uint32_t)lphul->wtEmpty;
            }
            cch = wsprintf(szWork, PszGetCompressedString(idsMassLdkt), lwt);
            TextOut(hdc, rc.left, rc.top, szWork, cch);
        }
        rc.top -= dyArial8 * 4;
        rc.left += dxMineral + dxMaxMineralQuan + 24;
        if (!fStarbaseMode && (!hwndPopup || GlobalPD.grPopup != grPopupShdef || !GlobalPD.fToken)) {
            cch = wsprintf(szWork, PszGetCompressedString(idsDmg), WtMaxShdefStat(lpshdefBuild, 1));
            RightTextOut(hdc, rc.right - 8, rc.top, szWork, cch, dxMaxMineralQuan);
            cch = CchGetString(idsMaxFuel, szWork);
            TextOut(hdc, rc.left, rc.top, szWork, cch);
            rc.top += dyArial8;
        }
        cch = wsprintf(szWork, PszGetCompressedString(idsLddp), dp);
        RightTextOut(hdc, rc.right - 8, rc.top, szWork, cch, dxMaxMineralQuan + 10);
        cch = CchGetString(idsArmor, szWork);
        TextOut(hdc, rc.left, rc.top, szWork, cch);
        rc.top += dyArial8;
        if (mdBuild != mdBuildHuldef) {
            cch = wsprintf(szWork, dpShield == 0 ? PszGetCompressedString(idsNone) : "%lddp", dpShield);
            RightTextOut(hdc, rc.right - 8, rc.top, szWork, cch, dxMaxMineralQuan + 10);
            cch = CchGetString(idsShields, szWork);
            TextOut(hdc, rc.left, rc.top, szWork, cch);
        }
        rc.top += dyArial8;
        if (mdBuild != mdBuildHuldef) {
            lpshdefBuild->lPower = LComputePower(lpshdefBuild);
            if (lpshdefBuild->lPower != 0) {
                cch = wsprintf(szWork, PCTLD, lpshdefBuild->lPower);
                RightTextOut(hdc, rc.right - 8, rc.top, szWork, cch, dxMaxMineralQuan);
                cch = CchGetString(idsRating, szWork);
                TextOut(hdc, rc.left, rc.top, szWork, cch);
                rc.top += dyArial8;
            }
        }
        if (gd.mdScreenSize > 0 && mdBuild != mdBuildHuldef) {
            if (mdBuild != mdBuildEnemyShdef) {
                i = idPlayer;
            } else {
                i = -1;
            }
            i = PctCloakFromHuldef(&lpshdefBuild->hul, i, NULL);
            j = PctJammerFromHul(&lpshdefBuild->hul);
            cch = wsprintf(szWork, PszGetCompressedString(idsDD4), i, j);
            RightTextOut(hdc, rc.right - 8, rc.top, szWork, cch, dxMaxMineralQuan);
            cch = CchGetString(idsCloakJam, szWork);
            TextOut(hdc, rc.left, rc.top, szWork, cch);
            rc.top += dyArial8;
            i = InitFromHuldef(&lpshdefBuild->hul, NULL);
            if (fStarbaseMode || lpshdefBuild->hul.rghs[0].cItem == 0) {
                j = 0;
            } else {
                j = SpdOfShip(NULL, 0, NULL, FALSE, lpshdefBuild) + 1;
            }
            cch = wsprintf(szWork, PszGetCompressedString(idsDS), i, &rgszSpeed[j * 3]);
            RightTextOut(hdc, rc.right - 8, rc.top, szWork, cch, dxMaxMineralQuan);
            cch = CchGetString(idsInitiativeMoves + (dyArial8 > 14), szWork);
            TextOut(hdc, rc.left, rc.top, szWork, cch);
            rc.top += dyArial8;
            if (!fStarbaseMode) {
                if (mdBuild != mdBuildEnemyShdef) {
                    i = idPlayer;
                } else {
                    i = -1;
                }
                dRange = GetShdefScannerRange(lpshdefBuild, i, &dPlanRange, &pctDetect, NULL);
                if (dRange > 0) {
                    if (pctDetect >= 100) {
                        cch = wsprintf(szWork, PszGetCompressedString(idsDD6), dRange, dPlanRange);
                    } else {
                        cch = wsprintf(szWork, PszGetCompressedString(idsDDD), dRange, dPlanRange, pctDetect);
                    }
                    RightTextOut(hdc, rc.right - 8, rc.top, szWork, cch, dxMaxMineralQuan + 40);
                    cch = CchGetString(idsScannerRange2 + (dyArial8 > 14), szWork);
                    TextOut(hdc, rc.left, rc.top, szWork, cch);
                    rc.top += dyArial8;
                }
            } else if (GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh) {
                cch = CommaFormatLong(szWork, (uint32_t)(rglPopMac[lpshdefBuild->hul.ihuldef - ihuldefOrbitalFort] * 100));
                RightTextOut(hdc, rc.right - 8, rc.top, szWork, cch, dxMaxMineralQuan + 16);
                cch = CchGetString(idsMaxPopulation + (dyArial8 > 14), szWork);
                TextOut(hdc, rc.left, rc.top, szWork, cch);
                rc.top += dyArial8;
            }
        }
        if (hwndPopup && GlobalPD.grPopup == grPopupShdef && GlobalPD.fShowDamage) {
            if (!GlobalPD.fToken) {
                if (!fStarbaseMode) {
                    if (GlobalPD.fSummary) {
                        k = lpshdefBuild->ishdef;
                        csh = rglpfl[sel.scan.ifl]->rgcsh[k];
                        dv.dp = rglpfl[sel.scan.ifl]->rgdv[k].dp;
                    } else {
                        k = lpshdefBuild->ishdef;
                        csh = sel.fl.rgcsh[k];
                        dv.dp = sel.fl.rgdv[k].dp;
                    }
                } else {
                    dv.dp = 0;
                    if (GlobalPD.fSummary) {
                        dv.pctDp = LpplFromId(sel.scan.idpl)->pctDp;
                    } else {
                        dv.pctDp = sel.pl.pctDp;
                    }
                    csh = 1;
                    if (dv.pctDp != 0) {
                        dv.pctSh = 100;
                    }
                }
            }
            if (dv.dp != 0) {
                SetTextColor(hdc, 127);
                pct = dv.pctDp / 5;
                if (pct <= 0) {
                    pct = 1;
                }
                csh = LOWORD((int32_t)(csh * dv.pctSh) / 100);
                if (csh <= 0) {
                    csh = 1;
                }
                if (fStarbaseMode) {
                    cch = wsprintf(szWork, PCTDPCTPCT, pct);
                } else {
                    cch = wsprintf(szWork, PszGetCompressedString(idsLdD), csh, pct);
                }
                dp = 1;
            } else {
                dp = 0;
            }
            if (dp != 0) {
                RightTextOut(hdc, rc.right - 8, rc.top, szWork, cch, dxMaxMineralQuan + 15);
                cch = CchGetString(idsDamage, szWork);
                TextOut(hdc, rc.left, rc.top, szWork, cch);
            }
        }
    LDeadToken:
        SelectObject(hdc, rghfontArial8[0]);
        SetTextColor(hdc, crForeSav);
        SetBkColor(hdc, crBackSav);
    LReleaseDC:
        if (fCreatedDC) {
            ReleaseDC(hwnd, hdc);
        }
    }
    return;
}

void SetBuildSelection(int16_t iSrc) {
    int16_t iSelOld;
    RECT    rc;

    if (iSrc != iselSlot) {
        iSelOld = iselSlot;
        iselSlot = iSrc;
        GetClientRect(hwndSlotDlg, &rc);
        if (iSelOld >= 0) {
            DrawSlotDlg(hwndSlotDlg, NULL, &rc, iSelOld);
        }
        if (iselSlot >= 0) {
            DrawSlotDlg(hwndSlotDlg, NULL, &rc, iselSlot);
        }
    RedrawSel:
        DrawBuildSelComp(hwndSlotDlg, NULL, -1);
    } else if (iSrc == -1) {
        goto RedrawSel;
    }
    return;
}

int16_t IDropPart(POINT16 pt, HS hsSrc, int16_t iSrc, int16_t fNoModify) {
    int16_t cSlot;
    int16_t cNew;
    int16_t i;
    HS      hsHul;
    HS      hsDst;
    RECT    rc;

    GetClientRect(hwndSlotDlg, &rc);
    if (GetAsyncKeyState(VK_CONTROL) & 0xfffe) {
        if (iSrc < 0) {
            hsSrc.cItem = 100;
        }
    } else if (GetAsyncKeyState(VK_SHIFT) & 0xfffe) {
        if (iSrc < 0 || hsSrc.cItem > 4) {
            hsSrc.cItem = 4;
        }
    } else if (iSrc >= 0) {
        hsSrc.cItem = 1;
    }
    cSlot = LphuldefFromId(lpshdefBuild->hul.ihuldef)->hul.chs;
    for (i = 0; i < cSlot && PtInRect(&vrgrcSlot[i], PointFrom16(pt)) == 0; i++) {
    }
    if (i == cSlot) {
        if (pt.x < rc.right >> 1) {
            if (!fNoModify && iSrc >= 0) {
                if (lpshdefBuild->hul.rghs[iSrc].grhst & hstEngine) {
                    hsSrc.cItem = 100;
                }
                lpshdefBuild->hul.rghs[iSrc].cItem =
                    0 <= lpshdefBuild->hul.rghs[iSrc].cItem - hsSrc.cItem ? lpshdefBuild->hul.rghs[iSrc].cItem - hsSrc.cItem : 0;
                UpdateShdefCost(lpshdefBuild);
                GetClientRect(hwndSlotDlg, &rc);
                DrawSlotDlg(hwndSlotDlg, NULL, &rc, iSrc);
                rc.top = yBuildInfoSum;
                InvalidateRect(hwndSlotDlg, &rc, TRUE);
                DrawBuildSelComp(hwndSlotDlg, NULL, -1);
                DrawBuildSelHull(hwndSlotDlg, NULL, -1, NULL);
                if (gd.fTutorial) {
                    AdvanceTutor();
                }
            }
            return 1;
        }
        return 0;
    }
    hsDst = lpshdefBuild->hul.rghs[i];
    hsHul = LphuldefFromId(lpshdefBuild->hul.ihuldef)->hul.rghs[i];
    if (hsHul.grhst & hstEngine) {
        hsSrc.cItem = 100;
    }
    if (i == iSrc) {
        return 2;
    }
    if (hsDst.cItem >= hsHul.cItem ||
        ((hsDst.cItem > 0 && (hsDst.grhst != hsSrc.grhst || hsDst.iItem != hsSrc.iItem)) || (hsDst.cItem == 0 && !(hsSrc.grhst & hsHul.grhst)))) {
        if (!fNoModify) {
            MessageBeep(MB_OK);
        }
        return 3;
    }
    if (!fNoModify) {
        hsDst.grhst = hsSrc.grhst;
        hsDst.iItem = hsSrc.iItem;
        cNew = (uint16_t)(hsDst.cItem + hsSrc.cItem) >= hsHul.cItem ? hsHul.cItem : hsDst.cItem + hsSrc.cItem;
        if (iSrc >= 0) {
            lpshdefBuild->hul.rghs[iSrc].cItem -= cNew - hsDst.cItem;
        }
        hsDst.cItem = cNew;
        lpshdefBuild->hul.rghs[i] = hsDst;
        UpdateShdefCost(lpshdefBuild);
        SetBuildSelection(i);
        GetClientRect(hwndSlotDlg, &rc);
        rc.top = yBuildInfoSum;
        InvalidateRect(hwndSlotDlg, &rc, TRUE);
        if (gd.fTutorial) {
            AdvanceTutor();
        }
    }
    return -1;
}

void DrawDlgLBEntireItem(DRAWITEMSTRUCT *lpdis, int16_t inflate) {
    COLORREF cr;
    COLORREF crForeSav;
    int16_t  ibmp;
    int16_t  bkSav;
    RECT     rc;

    CopyRect(&rc, &lpdis->rcItem);
    FillRect(lpdis->hDC, &lpdis->rcItem, GetStockObject(!(lpdis->itemState & ODS_FOCUS) ? WHITE_BRUSH : BLACK_BRUSH));
    InflateRect(&rc, -2, -1);
    SendMessage(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, (LPARAM)szWork);
    SelectPalette(lpdis->hDC, vhpal, FALSE);
    RealizePalette(lpdis->hDC);
    ibmp = szWork[2] - 'A' + (szWork[3] - 'A') * 26;
    DibBlt(lpdis->hDC, rc.left, rc.top, 64, 64, rghdibInventory[ibmp >> 5], (ibmp & 7) * 0x40, ((3 - (ibmp >> 3)) & 3) * 0x40, 64, 64, 13369376);
    cr = (lpdis->itemState & ODS_FOCUS) ? crWindow : crWindow == 0 ? 0xffffff : 0;
    crForeSav = SetTextColor(lpdis->hDC, cr);
    bkSav = SetBkMode(lpdis->hDC, TRANSPARENT);
    TextOut(lpdis->hDC, rc.left + 66, rc.top + 0x20 - (dyArial8 >> 1), &szWork[4], strlen(&szWork[4]));
    SetTextColor(lpdis->hDC, crForeSav);
    SetBkMode(lpdis->hDC, bkSav);
    HandleFocusState(lpdis, inflate + 2);
    return;
}

void FillBuildDD(HWND hwndDD, MdBuild md) {
    int16_t ishdefMac;
    int16_t fProgress;
    int16_t fAdded;
    int16_t i;
    int16_t j;
    SHDEF  *lpshdef;
    RECT    rc;
    PART    part;

    SendMessage(hwndDD, CB_RESETCONTENT, 0, 0);
    if (md != mdBuildShdef) {
        EnableWindow(GetDlgItem(hwndSlotDlg, IDC_DELETE), FALSE);
        EnableWindow(GetDlgItem(hwndSlotDlg, IDC_EDIT), FALSE);
    }
    if (fStarbaseMode) {
        ishdefMac = 10;
        lpshdef = rglpshdefSB[idPlayer];
    } else {
        ishdefMac = 16;
        lpshdef = rgshdef;
    }
    for (i = 0; i < ishdefMac && lpshdef[i].fFree == 0; i++) {
    }
    switch (md) {
    case mdBuildShdef:
    case mdBuildHuldef:
    case mdBuildEnemyShdef:
        if (i < ishdefMac) {
            fAdded = TRUE;
            break;
        }
        /* fallthrough */
    default:
        fAdded = FALSE;
    }
    EnableWindow(GetDlgItem(hwndSlotDlg, IDC_IMPORT), fAdded);
    switch (md) {
    case mdBuildShdef:
    default:
        fAdded = FALSE;
        for (i = 0; i < ishdefMac; i++) {
            if (!lpshdef[i].fFree) {
                if (!fAdded) {
                    CshQueued((!fStarbaseMode ? 0 : 16) + i, &fProgress, FALSE);
                    EnableWindow(GetDlgItem(hwndSlotDlg, IDC_EDIT), lpshdef[i].cExist == 0 && !fProgress);
                    EnableWindow(GetDlgItem(hwndSlotDlg, IDC_DELETE), TRUE);
                    fAdded = (lpshdef[i].cExist != 0) + 1;
                }
                SendMessage(hwndDD, CB_ADDSTRING, 0, (LPARAM)lpshdef[i].hul.szClass);
            }
        }
        break;
    case mdBuildEnemyShdef:
        for (i = 0; i < game.cPlayer; i++) {
            if (i != idPlayer) {
                lpshdef = !fStarbaseMode ? rglpshdef[i] : rglpshdefSB[i];
                if (lpshdef) {
                    for (j = 0; j < ishdefMac; j++) {
                        if (!lpshdef[j].fFree) {
                            if (PszPlayerName(i, TRUE, FALSE, FALSE, 0, NULL) != szWork) {
                            }
                            wsprintf(&szWork[strlen(szWork)], " %s", lpshdef[j].hul.szClass);
                            SendMessage(hwndDD, CB_ADDSTRING, 0, (LPARAM)szWork);
                        }
                    }
                }
            }
        }
        break;
    case mdBuildHuldef:
        if (fStarbaseMode) {
            part.hs.grhst = hstSBHull;
            j = 5;
        } else {
            part.hs.grhst = hstHull;
            j = 32;
        }
        for (i = 0; i < j; i++) {
            part.hs.iItem = i;
            if (FLookupPart(&part) == mdPartAvailAvailable) {
                SendMessage(hwndDD, CB_ADDSTRING, 0, (LPARAM)part.pcom->szName);
            }
        }
        break;
    case mdBuildComp:
    case mdBuildEdit:
        if (fStarbaseMode) {
            for (i = 0; i < 8; i++) {
                SendMessage(hwndDD, CB_ADDSTRING, 0, (LPARAM)PszGetCompressedString(rgidsPartsSB[i]));
            }
        } else {
            for (i = 0; i < 13; i++) {
                SendMessage(hwndDD, CB_ADDSTRING, 0, (LPARAM)PszGetCompressedString(rgidsParts[i]));
            }
        }
    }
    i = LOWORD(SendMessage(hwndDD, CB_GETCOUNT, 0, 0));
    if (i > 32) {
        i = 32;
    }
    GetWindowRect(hwndDD, &rc);
    SetWindowPos(hwndDD, NULL, 0, 0, rc.right - rc.left, (i + 1) * (dyArial8 - (dyArial8 <= 14)) + 8 + (dyArial8 > 14), SWP_NOMOVE | SWP_NOZORDER);
    SendMessage(hwndDD, CB_SETCURSEL, 0, 0);
    return;
}

void FillBuildPartsLB(HWND hwndLB, int16_t grbit) {
    mdPartAvail  mdAvail;
    int16_t      i;
    char         sz[200];
    HullSlotType grbitCur;
    PART         part;

    grbitCur = hstEngine;
    sz[0] = 'A';
    SendMessage(hwndLB, LB_RESETCONTENT, 0, 0);
    while (grbitCur != hstNone) {
        if (grbitCur & grbit) {
            i = 0;
            part.hs.grhst = grbitCur;
            while (1) {
                part.hs.iItem = i;
                mdAvail = FLookupPart(&part);
                if (mdAvail == mdPartAvailInvalid)
                    break;
                if (fStarbaseMode && grbitCur == hstSpecialE && (i == ispecialETachyonDetector || i == ispecialEAntiMatterGenerator)) {
                    mdAvail = mdPartAvailRestricted;
                }
                if (mdAvail == mdPartAvailAvailable) {
                    sz[1] = i + 'A';
                    sz[2] = part.pcom->ibmp % 26 + 'A';
                    sz[3] = part.pcom->ibmp / 26 + 'A';
                    strcpy(&sz[4], part.pcom->szName);
                    SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)sz);
                }
                i++;
            }
        }
        grbitCur *= 2;
        sz[0]++;
    }
    return;
}

LRESULT CALLBACK FakeListProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    POINT16 pt;
    int16_t iSel;

    switch (msg) {
    case WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (pt.x < 64) {
            SetCursor(hcurHand);
            return 1;
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        if (LOWORD(lParam) < 64 && (mdBuild == mdBuildEdit || msg == WM_RBUTTONDOWN)) {
            CallWindowProc(lpfnRealListProc, hwnd, WM_LBUTTONDOWN, wParam, lParam);
            CallWindowProc(lpfnRealListProc, hwnd, WM_LBUTTONUP, wParam, lParam);
            if (msg == WM_RBUTTONDOWN || mdBuild != mdBuildEdit) {
                iSel = LOWORD(SendMessage(hwnd, LB_GETCURSEL, 0, 0));
                if (iSel == -1) {
                    return 0;
                }
                SendMessage(hwnd, LB_GETTEXT, iSel, (LPARAM)szWork);
                GlobalPD.part.hs.grhst = 1 << (szWork[0] - 'A');
                GlobalPD.part.hs.iItem = szWork[1] - 'A';
                FLookupPart(&GlobalPD.part);
                GlobalPD.grPopup = grPopupComponent;
                Popup(hwnd, LOWORD(lParam), HIWORD(lParam));
                return 0;
            }
            FTrackSlot(hwnd, LOWORD(lParam), HIWORD(lParam), wParam, TRUE, FALSE);
            return 0;
        }
        break;
    }
    return CallWindowProc(lpfnRealListProc, hwnd, msg, wParam, lParam);
}

void UpdateSlotGlobals() {
    int16_t  yTop;
    int16_t  cSlot;
    int16_t  i;
    uint16_t wrc;
    int16_t  xLeft;
    HULDEF  *lphuldef;

    if (!lpshdefBuild) {
        cSlot = 0;
    } else {
        lphuldef = LphuldefFromId(lpshdefBuild->hul.ihuldef);
        if (!hwndSlotDlg) {
            xLeft = 12;
            yTop = dyArial8 + 12;
        } else {
            xLeft = ptslotGlob.x - 330;
            yTop = 32;
        }
        cSlot = lphuldef->hul.chs;
        for (i = 0; i < cSlot; i++) {
            vrgrcSlot[i].left = (lphuldef->rgbrc[i] & 0xf) * 0x20 + xLeft;
            vrgrcSlot[i].top = (lphuldef->rgbrc[i] >> 4) * 0x20 + yTop;
            vrgrcSlot[i].right = vrgrcSlot[i].left + 64;
            vrgrcSlot[i].bottom = vrgrcSlot[i].top + 64;
        }
        if (lphuldef->hul.wtCargoMax != 0) {
            wrc = lphuldef->wrcCargo;
            rcCargo.left = (wrc >> 8 & 0xff & 0xf) * 0x20 + xLeft;
            rcCargo.top = ((wrc >> 8 & 0xff) >> 4) * 0x20 + yTop;
            rcCargo.right = (wrc & 0xff & 0xff & 0xf) * 0x20 + xLeft;
            rcCargo.bottom = ((wrc & 0xff & 0xff) >> 4) * 0x20 + yTop;
        }
        ptPlaque.x = xLeft + 258;
        ptPlaque.y = yTop + 273;
    }
    return;
}

int16_t IEmptyBmpFromGrhst(int16_t grhst) {
    int16_t i;

    for (i = 0; i < 21; i++) {
        if (rgmapBuildBmps[i] == grhst) {
            return i;
        }
    }
    return 0;
}

void MakeNewName(char *lpsz) {
    int16_t cLen;

    cLen = strlen(lpsz);
    if (cLen <= 27) {
        if (lpsz[cLen - 1] != ')' || isdigit(lpsz[cLen - 2]) == 0 || lpsz[cLen - 3] != '(') {
            strcpy(lpsz + cLen, " (2)");
        } else if (lpsz[cLen - 2] == '9') {
            lpsz[cLen - 2] = '0';
        } else {
            lpsz[cLen - 2] = lpsz[cLen - 2] + 1;
        }
    }
    FStringFitsScreen(lpsz, 160);
    return;
}
