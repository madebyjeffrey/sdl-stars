#include "win.h"

static int16_t dWheelMessages;

LRESULT CALLBACK MessageWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC         hdc;
    int16_t     i;
    char       *psz;
    PAINTSTRUCT ps;
    int16_t     dy;
    int16_t     dx;
    RECT        rc;
    POINT16     pt;
    HCURSOR     hcs;
    HtMsgType   ht;
    int16_t     fSet;
    MessageId   idm;
    MSGPLR     *lpmp;
    MSGPLR     *lpmpSrc;
    char       *lpsz;
    HBRUSH      hbrSav;
    COLORREF    crFore;
    int16_t     dxMax;
    COLORREF    crBack;
    RECT        rcActual;
    int16_t     cch;
    int16_t     iMode;
    char        szT[32];
    MSGPLR     *lpmsgplr;
    THING      *lpth;
    SCAN        scan;
    FARPROC     lpProc;
    int16_t     fRet;

    switch (message) {
    case WM_CREATE:
        dWheelMessages = 0;
        for (i = 0; i < 4; i++) {
            hwndMessage = hwnd;
            rghwndMsgBtn[i] = CreateWindow("BUTTON", PszGetCompressedString(static_cast<StringId>(idsPrev2 + i)), WS_CHILD, 100, 100, i == 3 ? 50 : 44, (3 * dyArial8 >> 1) - 1, hwnd,
                                           NULL, hInst, NULL);
            SendMessage(rghwndMsgBtn[i], WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        }
        hwndMsgDrop = CreateWindow("COMBOBOX", "MsgDD", CBS_DROPDOWNLIST | WS_CHILD | WS_VSCROLL, 100, 100, 200, 80, hwnd, NULL, hInst, NULL);
        SendMessage(hwndMsgDrop, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        hwndMsgEdit = CreateWindow("EDIT", NULL, ES_MULTILINE | ES_AUTOVSCROLL | WS_CHILD | WS_BORDER, 100, 100, 200, 50, hwnd, NULL, hInst, NULL);
        SendMessage(hwndMsgEdit, EM_LIMITTEXT, 0x3c8, 0);
        SendMessage(hwndMsgEdit, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        hwndMsgScroll = CreateWindow("EDIT", NULL, ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_CHILD | WS_BORDER | WS_VSCROLL, 100, 100, 200, 50, hwnd,
                                     NULL, hInst, NULL);
        SetMsgTitle(hwnd);
        SendMessage(hwndMsgDrop, CB_ADDSTRING, 0, (LPARAM)PszGetCompressedString(idsEverybody));
        for (i = 0; i < game.cPlayer; i++) {
            psz = PszPlayerName(i, TRUE, TRUE, TRUE, 0, NULL);
            SendMessage(hwndMsgDrop, CB_ADDSTRING, 0, (LPARAM)psz);
        }
        SendMessage(hwndMsgDrop, CB_SETCURSEL, 0, 0);
        break;
    case WM_SIZE:
        dx = LOWORD(lParam);
        dy = HIWORD(lParam);
        for (i = 0; i < 3; i++) {
            SetWindowPos(rghwndMsgBtn[i], NULL, dx - 48, ((3 * dyArial8 >> 1) + 2) * i + 3 + dyArial8 * 2, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
        SetRect(&rcMsgText, 4, dyArial8 * 2 + 3, dx - 52, dy - 4);
        SetRect(&rcMsgTitle, 4, 4, dx - 4, dyArial8 * 2 - 4);
        rc = rcMsgText;
        ExpandRc(&rc, -4, -4);
        SetWindowPos(hwndMsgDrop, NULL, rc.left + 30, rc.top, rc.right - rc.left - 84, rc.bottom - rc.top, SWP_NOZORDER);
        SetWindowPos(rghwndMsgBtn[3], NULL, rc.right - 50, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        rc.top += dyShipDD + 3;
        SetWindowPos(hwndMsgEdit, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
        goto Default;
    case WM_ERASEBKGND:
        GetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_SETCURSOR:
        hcs = 0;
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (HtMsgBox(pt) == htMsgNone)
            goto Default;
        SetCursor(hcurHand);
        return 1;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        ht = HtMsgBox(pt);
        if (ht == htMsgCurrent) {
        CheckBox:
            if (iMsgCur < 0)
                break;
            idm = static_cast<MessageId>(IdmGetMessageN(iMsgCur));
            fSet = (bitfMsgFiltered[idm >> 3] & 1 << (idm & 7)) != 0;
            SetFilteringGroups(idm, fSet == 0);
            DirtyGame(TRUE);
            if (gd.fTutorial) {
                AdvanceTutor();
            }
            InvalidateRect(hwndMessage, NULL, TRUE);
            SetMsgTitle(hwnd);
        } else if (ht == htMsgZoom) {
        ZoomBox:
            fViewFilteredMsg = fViewFilteredMsg == 0;
            if (iMsgCur < 0 || ((bitfMsgFiltered[IdmGetMessageN(iMsgCur) >> 3] & 1 << (IdmGetMessageN(iMsgCur) & 7)) != 0) != fViewFilteredMsg) {
                i = IMsgNext(fViewFilteredMsg);
                if (i == imsgNone) {
                    i = IMsgPrev(fViewFilteredMsg);
                }
                iMsgCur = i;
            }
            InvalidateRect(hwndMessage, NULL, TRUE);
            SetMsgTitle(hwnd);
        } else if (ht == htMsgMode) {
        ToggleMsgMode:
            if (gd.fSendMsgMode) {
                FFinishPlrMsgEntry(0);
            } else if (iMsgCur >= cMsg) {
                lpmpSrc = vlpmsgplrIn;
                i = iMsgCur - cMsg;
                while (i-- != 0) {
                    lpmpSrc = lpmpSrc->lpmsgplrNext;
                }
                lpmp = vlpmsgplrOut;
                iMsgSendCur = 0;
                while (lpmp && (lpmp->iPlrTo - 1 != lpmpSrc->iPlrFrom || lpmp->iInRe != iMsgCur)) {
                    lpmp = lpmp->lpmsgplrNext;
                    iMsgSendCur++;
                }
                viInRe = lpmpSrc->iPlrFrom + 1;
            } else {
                viInRe = 0;
            }
            gd.fSendMsgMode = gd.fSendMsgMode == 0;
            InvalidateRect(hwndMessage, NULL, TRUE);
            SetMsgTitle(hwnd);
            SetFocus(hwndMsgEdit);
        }
        break;
    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)lParam)->ptMinTrackSize.x = dxWinFrame * 2 + 198;
        ((MINMAXINFO *)lParam)->ptMinTrackSize.y = (0xd * dyArial8 >> 1) + 0x16;
        goto Default;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        if ((HWND)lParam != hwndMsgScroll)
            goto Default;
        SetBkColor((HDC)wParam, crButtonFace);
        return (LRESULT)hbrButtonFace;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        _Draw3dFrame(hdc, &rcMsgTitle, 0);
        crFore = SetTextColor(hdc, crButtonText);
        crBack = SetBkColor(hdc, crButtonFace);
        cch = strlen(szMsgTitle);
        dxMax = rcMsgTitle.right - rcMsgTitle.left - 48;
        while (cch > 0 && (int16_t)LOWORD(GetTextExtent(hdc, szMsgTitle, cch)) > dxMax) {
            cch = (int16_t)CchUtf8Prefix(szMsgTitle, cch - 1);
        }
        RcCtrTextOut(hdc, &rcMsgTitle, szMsgTitle, cch);
        DecorateMsgTitleBar(hdc, &rcMsgTitle);
        rc = rcMsgText;
        dx = rc.right - rc.left;
        dy = rc.bottom - rc.top;
        hbrSav = (HBRUSH)SelectObject(hdc, hbrButtonShadow);
        PatBlt(hdc, rc.left, rc.top, dx, 1, PATCOPY);
        PatBlt(hdc, rc.left, rc.top, 1, dy, PATCOPY);
        SelectObject(hdc, hbrButtonHilite);
        PatBlt(hdc, rc.left, rc.bottom - 1, dx, 1, PATCOPY);
        PatBlt(hdc, rc.right - 1, rc.top, 1, dy, PATCOPY);
        SelectObject(hdc, hbrSav);
        ExpandRc(&rc, -4, -4);
        if (!gd.fSendMsgMode) {
            if (iMsgCur >= cMsg) {
                lpmsgplr = vlpmsgplrIn;
                for (i = cMsg; i < iMsgCur; i++) {
                    lpmsgplr = lpmsgplr->lpmsgplrNext;
                }
                if (CchGetString(idsSCC, szT) >= 32) {
                }
                cch = wsprintf((LPSTR)lpb2k, szT, PszPlayerName(lpmsgplr->iPlrFrom, TRUE, TRUE, TRUE, 0, NULL), 13, 10);
                if (CchGetString(idsSCC2, szT) >= 32) {
                }
                cch += wsprintf((LPSTR)lpb2k + cch, szT,
                                lpmsgplr->iPlrTo == 0 ? PszGetCompressedString(idsEverybody) : PszPlayerName(lpmsgplr->iPlrTo - 1, TRUE, TRUE, TRUE, 0, NULL),
                                13, 10);
                if (lpmsgplr->cLen >= 0) {
                    i = 1000;
                    FDecompressUserString((char *)lpmsgplr->rgbMsg(), lpmsgplr->cLen, (char *)lpb2k + cch, &i);
                } else {
                    strcpy((char *)lpb2k + cch, (const char *)lpmsgplr->rgbMsg());
                }
                lpsz = (char *)lpb2k;
            } else {
                idm = static_cast<MessageId>(IdmGetMessageN(iMsgCur));
                if (iMsgCur < 0 && cMsg > 0) {
                    lpsz = PszGetCompressedString(idsMessagesHaveSentYearFilteredIfWant);
                } else if (iMsgCur >= 0 && (bitfMsgFiltered[idm >> 3] & 1 << (idm & 7)) && !fViewFilteredMsg) {
                    lpsz = PszGetCompressedString(idsMessageTypeHasFilteredWillShownDefault);
                } else {
                    lpsz = PszGetMessageN(iMsgCur);
                }
            }
            SetTextColor(hdc, 0xffffff);
            iMode = SetBkMode(hdc, TRANSPARENT);
            if ((iMsgCur < 0 && cMsg > 0) ||
                (iMsgCur >= 0 && iMsgCur < cMsg && (bitfMsgFiltered[IdmGetMessageN(iMsgCur) >> 3] & 1 << (IdmGetMessageN(iMsgCur) & 7)))) {
                cch = CchGetString(idsFiltered, szWork);
                DiaganolTextOut(hdc, &rc, szWork, cch);
                lpsz = PszGetMessageN(iMsgCur);
            }
            SetTextColor(hdc, crButtonText);
            rcActual = rc;
            DrawText(hdc, lpsz, strlen(lpsz), &rcActual, DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);
            if (rcActual.bottom <= rc.bottom && rcActual.right <= rc.right) {
                ShowWindow(hwndMsgScroll, SW_HIDE);
                DrawText(hdc, lpsz, strlen(lpsz), &rc, DT_WORDBREAK | DT_NOPREFIX);
            } else {
                SetWindowText(hwndMsgScroll, lpsz);
                ExpandRc(&rc, 4, 4);
                SetWindowPos(hwndMsgScroll, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_SHOWWINDOW);
            }
            SetBkMode(hdc, iMode);
        } else {
            iMode = SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, crButtonText);
            cch = CchGetString(idsTo3, szT);
            RightTextOut(hdc, rc.left + 26, rc.top, szT, cch, 0);
            SetBkMode(hdc, iMode);
        }
        SetTextColor(hdc, crFore);
        SetBkColor(hdc, crBack);
        EndPaint(hwnd, &ps);
        break;
    case WM_MOUSEWHEEL:
        if (gd.fSendMsgMode || (LOWORD(wParam) & (MK_CONTROL | MK_SHIFT))) return 0;
        i = CWheelNotches(&dWheelMessages, (int16_t)HIWORD(wParam));
        while (i > 0) { SendMessage(hwnd, WM_KEYDOWN, VK_UP, 1); i--; }
        while (i < 0) { SendMessage(hwnd, WM_KEYDOWN, VK_DOWN, 1); i++; }
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_DOWN)
            goto NextMsg;
        if (wParam == VK_UP)
            goto PrevMsg;
        if (gd.fSendMsgMode)
            goto Default;
        if (wParam == VK_HOME) {
            iMsgCur = imsgNone;
            goto NextMsg;
        }
        if (wParam != VK_END) {
            return 0;
        }
        iMsgCur = cMsg + vcmsgplrIn;
        goto PrevMsg;
    case WM_CHAR:
        switch (wParam) {
        case '+':
            goto CheckBox;
        case '-':
            for (i = 0; (uint16_t)i < 49 && (bitfMsgSent[i] & bitfMsgFiltered[i]) == 0; i++) {
            }
            if (i != 49)
                goto ZoomBox;
            break;
        case '\r':
            goto GotoMsg;
        }
        return 0;
    case WM_COMMAND:
        if (HIWORD(wParam) == 0) {
            SetFocus(hwndFrame);
        }
        if ((HWND)lParam == rghwndMsgBtn[0] && HIWORD(wParam) == 0) {
        PrevMsg:
            if (gd.fSendMsgMode) {
                FFinishPlrMsgEntry(-1);
                goto SetupNewMsg;
            }
            if (GetAsyncKeyState(VK_SHIFT) & 0xfffe) {
                iMsgCur = imsgNone;
                i = IMsgNext(FALSE);
            } else {
                i = IMsgPrev(FALSE);
            }
            if (i != imsgNone) {
                iMsgCur = i;
                goto SetupNewMsg;
            }
            if (iMsgCur != cMsg + vcmsgplrIn)
                break;
            iMsgCur--;
        SetupNewMsg:
            gd.fGotoVCR = FALSE;
            SetMsgTitle(hwnd);
            InvalidateRect(hwnd, &rcMsgText, TRUE);
            if (gd.fTutorial) {
                tutor.fChange = TRUE;
                AdvanceTutor();
            }
            break;
        } else if ((HWND)lParam == rghwndMsgBtn[2] && HIWORD(wParam) == 0) {
        NextMsg:
            if (gd.fSendMsgMode) {
                FFinishPlrMsgEntry(1);
            } else {
                if (GetAsyncKeyState(VK_SHIFT) & 0xfffe) {
                    iMsgCur = cMsg + vcmsgplrIn;
                    i = IMsgPrev(FALSE);
                } else {
                    i = IMsgNext(FALSE);
                }
                if (i == imsgNone)
                    break;
                iMsgCur = i;
            }
            goto SetupNewMsg;
        } else if ((HWND)lParam == rghwndMsgBtn[3] && HIWORD(wParam) == 0) {
            FFinishPlrMsgEntry(1000);
            goto SetupNewMsg;
        } else if ((HWND)lParam == rghwndMsgBtn[1] && HIWORD(wParam) == 0) {
        GotoMsg:
            if (gd.fSendMsgMode || iMsgCur >= cMsg)
                goto ToggleMsgMode;
            if (mdMsgObj <= mdMsgObjBattleReport) {
                switch (mdMsgObj) {
                default:
                    break;
                case mdMsgObjPlanet:
                    SelectAdjPlanet(0, idMsgObj);
                    UpdateWindow(hwndScanner);
                    SendMessage(hwndScanner, WM_CHAR, 'v', 0);
                    idm = static_cast<MessageId>(IdmGetMessageN(iMsgCur));
                    if (idm != idmHasCompletedOrdersProductionQueueEmpty && idm != idmProductionQueueEmpty &&
                        (idm < idmHasBuiltManyMinesCurrentPopulationCan || idm > idmHasBuiltManyDefensesPlanetCanSupport))
                        break;
                    if (!gd.fGotoVCR) {
                        gd.fGotoVCR = TRUE;
                        SetMsgTitle(hwnd);
                        break;
                    }
                    if (sel.grobj != grobjPlanet || sel.id != idMsgObj)
                        break;
                    ChangeProduction(FALSE);
                    break;
                case mdMsgObjFleet:
                    SelectAdjFleet(0, idMsgObj);
                    UpdateWindow(hwndScanner);
                    SendMessage(hwndScanner, WM_CHAR, 'v', 0);
                    break;
                case mdMsgObjThing:
                    lpth = LpthFromId(vptMsg.x);
                    if (!lpth)
                        break;
                    scan.pt = lpth->pt();
                    scan.grobj = grobjThing;
                    ChangeScanSel(&scan, 0);
                    CtrPointScan(scan.pt, TRUE);
                    break;
                case mdMsgObjBattle:
                    SelectOursAtObject(&vptMsg);
                    if (gd.fGotoVCR) {
                        BattleVCR(idMsgObj);
                        break;
                    }
                    gd.fGotoVCR = TRUE;
                    SetMsgTitle(hwnd);
                    break;
                case mdMsgObjResearch:
                    PostMessage(hwndFrame, WM_COMMAND, IDM_GAME_RESEARCH, 0);
                    break;
                case mdMsgObjScore:
                    PostMessage(hwndFrame, WM_COMMAND, IDM_GAME_SCORE, 0);
                    break;
                case mdMsgObjShipDesign:
                    PostMessage(hwndFrame, WM_COMMAND, IDM_GAME_SHIP_BUILDER, 0);
                    break;
                case mdMsgObjRelations:
                    PostMessage(hwndFrame, WM_COMMAND, IDM_GAME_RELATIONS2, 0);
                    break;
                case mdMsgObjBattleReport:
                    if (hwndReportDlg && vprptCur == &vrptBattle)
                        break;
                    PostMessage(hwndFrame, WM_COMMAND, IDM_REPORT_BATTLE, 0);
                    break;
                case mdMsgObjPart:
                    vpartBrowser.hs.grhst = static_cast<HullSlotType>(1 << (idMsgObj >> 8 & 0xf));
                    vpartBrowser.hs.iItem = idMsgObj & 0xff;
                    FLookupPart(&vpartBrowser);
                    if (hwndBrowser) {
                        InvalidateRect(hwndBrowserChild, NULL, TRUE);
                    } else {
                        fBrowserValid = TRUE;
                        PostMessage(hwndFrame, WM_COMMAND, IDM_VIEW_BROWSER_TOGGLE2, 0);
                    }
                }
            }
            if (!gd.fTutorial)
                break;
            tutor.fChange = TRUE;
            AdvanceTutor();
            break;
        } else {
            goto Default;
        }
    default:
    Default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

void DecorateMsgTitleBar(HDC hdc, RECT *prc) {
    int16_t   xDst;
    int16_t   ySrcMask;
    HBRUSH    hbrSav;
    HBITMAP   hbmpSav;
    int16_t   dySrc;
    int16_t   i;
    HDC       hdcMem;
    MessageId idm;
    int16_t   ySrc;
    int16_t   yDst;
    int16_t   dxSrc;
    int16_t   xyStart;
    COLORREF  crBkSav;
    COLORREF  crTextSav;

    hdcMem = CreateCompatibleDC(hdc);
    hbmpSav = (HBITMAP)SelectObject(hdcMem, hbmpMono);
    if (gd.fSendMsgMode)
        goto Cleanup;
    xyStart = (int16_t)(prc->bottom - prc->top - 11) / 2 + prc->top;
    if (iMsgCur < 0 || iMsgCur >= cMsg)
        goto DoMinMax;
    idm = static_cast<MessageId>(IdmGetMessageN(iMsgCur));
    if (bitfMsgFiltered[idm >> 3] & 1 << (idm & 7)) {
        ySrc = 14;
        dxSrc = 14;
        dySrc = 12;
        xDst = xyStart - 1;
        yDst = xyStart;
        ySrcMask = 42;
    } else {
        ySrc = 0;
        dxSrc = 15;
        dySrc = 14;
        xDst = xyStart;
        yDst = xyStart - 3;
        ySrcMask = 28;
    }
    crTextSav = SetTextColor(hdc, 0);
    crBkSav = SetBkColor(hdc, 0xffffff);
    BitBlt(hdc, xDst, yDst, dxSrc, dySrc, hdcMem, 0, ySrcMask, SRCAND);
    hbmpSav = (HBITMAP)SelectObject(hdcMem, hbmpMsg);
    BitBlt(hdc, xDst, yDst, dxSrc, dySrc, hdcMem, 0, ySrc, SRCPAINT);
DoMinMax:
    for (i = 0; (uint16_t)i < 49 && (bitfMsgSent[i] & bitfMsgFiltered[i]) == 0; i++) {
    }
    if (i == 49) {
        fViewFilteredMsg = FALSE;
        goto Cleanup;
    }
    if (fViewFilteredMsg) {
        ySrc = 26;
        ySrcMask = 54;
    } else {
        ySrc = 41;
        ySrcMask = 69;
    }
    xDst = prc->right - (prc->bottom - prc->top) - 1;
    hbrSav = (HBRUSH)SelectObject(hdc, hbrButtonShadow);
    PatBlt(hdc, xDst, prc->top, 1, prc->bottom - prc->top, PATCOPY);
    SelectObject(hdc, hbrButtonHilite);
    PatBlt(hdc, xDst + 1, prc->top, 1, prc->bottom - prc->top, PATCOPY);
    SelectObject(hdc, hbrSav);
    yDst = (int16_t)(prc->bottom - prc->top - 15) / 2 + prc->top;
    xDst = prc->right - (prc->bottom - yDst);
    SelectObject(hdcMem, hbmpMono);
    BitBlt(hdc, xDst, yDst, 15, 15, hdcMem, 0, ySrcMask, SRCAND);
    SelectObject(hdcMem, hbmpMsg);
    BitBlt(hdc, xDst, yDst, 15, 15, hdcMem, 0, ySrc, SRCPAINT);
Cleanup:
    SetTextColor(hdc, crTextSav);
    SetBkColor(hdc, crBkSav);
    if (!game.fSinglePlr) {
        SelectObject(hdcMem, hbmpMsg);
        xDst = prc->right - 45;
        yDst = (int16_t)(prc->bottom - prc->top - 7) / 2 + prc->top;
        PatBlt(hdc, xDst - 1, yDst - 1, 17, 11, BLACKNESS);
        BitBlt(hdc, xDst, yDst, 15, 9, hdcMem, 0, 56, SRCCOPY);
    }
    SelectObject(hdcMem, hbmpSav);
    DeleteDC(hdcMem);
    return;
}

HtMsgType HtMsgBox(POINT16 pt) {
    int16_t i;

    if (PtInRect(&rcMsgTitle, PointFrom16(pt)) != 0) {
        if (pt.x < rcMsgTitle.bottom - rcMsgTitle.top + rcMsgTitle.left && iMsgCur >= 0 && iMsgCur < cMsg && !gd.fSendMsgMode) {
            return htMsgCurrent;
        }
        if (pt.x >= rcMsgTitle.right - (rcMsgTitle.bottom - rcMsgTitle.top) && !gd.fSendMsgMode) {
            for (i = 0; (uint16_t)i < 49 && (bitfMsgSent[i] & bitfMsgFiltered[i]) == 0; i++) {
            }
            if (i != 49) {
                return htMsgZoom;
            }
        } else if (!game.fSinglePlr && pt.x >= rcMsgTitle.right - (rcMsgTitle.bottom - rcMsgTitle.top) - 24) {
            return htMsgMode;
        }
    }
    return htMsgNone;
}

int16_t FFinishPlrMsgEntry(int16_t dInc) {
    uint8_t *lpbMsg;
    int16_t  i;
    int16_t  cbNew;
    int16_t  iPlrTo;
    MSGPLR  *lpmpCur;
    MSGPLR **lpmpPrev;
    int16_t  cb;

    lpmpPrev = &vlpmsgplrOut;
    i = iMsgSendCur;
    while (i-- > 0) {
        lpmpPrev = &(*lpmpPrev)->lpmsgplrNext;
    }
    lpmpCur = *lpmpPrev;
    if (dInc == 1000) {
        cb = 0;
        dInc = 0;
    } else {
        cb = GetWindowText(hwndMsgEdit, (LPSTR)lpb2k, 1000);
    }
    if (cb == 0) {
        if (lpmpCur) {
            DirtyGame(TRUE);
            *lpmpPrev = lpmpCur->lpmsgplrNext;
            FreeLp(lpmpCur, htPlrMsg);
            vcmsgplrOut--;
            if (iMsgSendCur > 0) {
                iMsgSendCur--;
            }
        }
        if (dInc == -1 && iMsgSendCur > 0) {
            iMsgSendCur--;
        } else if (dInc == 1 && iMsgSendCur < vcmsgplrOut) {
            iMsgSendCur++;
        }
        return FALSE;
    }
    cbNew = cb;
    if (FCompressUserString((char *)lpb2k, (char *)lpb2k + 1024, &cbNew)) {
        cb = cbNew;
        lpbMsg = lpb2k + 1024;
    } else {
        cb = -cb - 1;
        lpbMsg = lpb2k;
    }
    cbNew = abs(cb) + 12;
    iPlrTo = LOWORD(SendMessage(hwndMsgDrop, CB_GETCURSEL, 0, 0));
    if (lpmpCur) {
        if (cb != lpmpCur->cLen || lpmpCur->iPlrTo != iPlrTo || memcmp(lpmpCur->rgbMsg(), lpbMsg, abs(cb)) != 0) {
            DirtyGame(TRUE);
        }
        lpmpCur = LpmsgplrReAlloc(lpmpCur, cbNew + (sizeof(MSGPLR) - 12));
    } else {
        DirtyGame(TRUE);
        lpmpCur = LpmsgplrAlloc(cbNew + (sizeof(MSGPLR) - 12));
        lpmpCur->lpmsgplrNext = NULL;
        vcmsgplrOut++;
        lpmpCur->iInRe = iMsgCur;
    }
    *lpmpPrev = lpmpCur;
    lpmpCur->iPlrFrom = idPlayer;
    lpmpCur->iPlrTo = iPlrTo;
    lpmpCur->cLen = cb;
    memmove(lpmpCur->rgbMsg(), lpbMsg, abs(cb));
    iMsgSendCur += dInc;
    if (iMsgSendCur < 0) {
        iMsgSendCur = 0;
    }
    return TRUE;
}

void SetMsgTitle(HWND hwnd) {
    int16_t cMsgTot;
    int16_t i;
    MSGBIG  mb;
    char    ch;
    char    szT[80];
    int16_t sw;
    MSGPLR *lpmp;
    RECT    rc;

    if (hwnd && !fAi) {
        if (game.fDirty) {
            ch = '*';
        } else {
            ch = ' ';
        }
        sw = !gd.fSendMsgMode ? 0 : 5;
        ShowWindow(hwndMsgEdit, sw);
        ShowWindow(hwndMsgDrop, sw);
        ShowWindow(rghwndMsgBtn[3], sw);
        if (gd.fSendMsgMode) {
            ShowWindow(hwndMsgScroll, SW_HIDE);
        }
        cMsgTot = cMsg + vcmsgplrIn;
        i = gd.fSendMsgMode ? idsDone : iMsgCur >= cMsg ? idsReply : gd.fGotoVCR ? idsView : idsGoto3;
        SetWindowText(rghwndMsgBtn[1], PszGetCompressedString(static_cast<StringId>(i)));
        if (gd.fSendMsgMode) {
            wsprintf(szWork, PszGetCompressedString(idsSendMessagesDD), iMsgSendCur + 1, vcmsgplrOut);
            rc = rcMsgText;
            ExpandRc(&rc, -4, -4);
            SetWindowPos(hwndMsgDrop, NULL, rc.left + 30, rc.top, rc.right - rc.left - 84, rc.bottom - rc.top, SWP_NOZORDER | SWP_SHOWWINDOW);
            SetWindowPos(rghwndMsgBtn[3], NULL, rc.right - 50, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
            rc.top += dyShipDD + 3;
            SetWindowPos(hwndMsgEdit, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_SHOWWINDOW);
            EnableWindow(rghwndMsgBtn[0], iMsgSendCur > 0);
            EnableWindow(rghwndMsgBtn[1], TRUE);
            EnableWindow(rghwndMsgBtn[2], TRUE);
            lpmp = vlpmsgplrOut;
            i = iMsgSendCur;
            while (i-- > 0) {
                lpmp = lpmp->lpmsgplrNext;
            }
            if (lpmp) {
                SendMessage(hwndMsgDrop, CB_SETCURSEL, lpmp->iPlrTo, 0);
                if (lpmp->cLen < 0) {
                    SetWindowText(hwndMsgEdit, (LPCSTR)lpmp->rgbMsg());
                    goto FinishUp;
                } else {
                    i = 1000;
                    FDecompressUserString((char *)lpmp->rgbMsg(), lpmp->cLen, (char *)lpb2k, &i);
                    SetWindowText(hwndMsgEdit, (LPCSTR)lpb2k);
                }
                goto FinishUp;
            } else {
                SendMessage(hwndMsgDrop, CB_SETCURSEL, viInRe, 0);
                SetWindowText(hwndMsgEdit, "");
                goto FinishUp;
            }
        }
        if (cMsgTot != 0) {
            CchGetString(idsYearDCMessagesDD, szT);
            wsprintf(szWork, szT, game.turn + 2400, ch, iMsgCur + 1, cMsgTot);
        } else {
            CchGetString(idsYearDCMessagesNone, szT);
            wsprintf(szWork, szT, game.turn + 2400, ch);
        }
        EnableWindow(rghwndMsgBtn[0], IMsgPrev(FALSE) != imsgNone);
        EnableWindow(rghwndMsgBtn[2], IMsgNext(FALSE) != imsgNone);
        if (iMsgCur >= cMsg) {
            EnableWindow(rghwndMsgBtn[1], TRUE);
            goto FinishUp;
        }
        if (cMsg != 0 && iMsgCur < cMsg && FGetNMsgbig(static_cast<MessageId>(iMsgCur), &mb)) {
            if (mb.wGoto != gotoNone) {
                idMsgObj = mb.wGoto & 0x7fff;
                switch (mb.wGoto) {
                case gotoResearch:
                    mdMsgObj = mdMsgObjResearch;
                    break;
                case gotoShipDesign:
                    mdMsgObj = mdMsgObjShipDesign;
                    break;
                case gotoScore:
                    mdMsgObj = mdMsgObjScore;
                    break;
                case gotoThing:
                    mdMsgObj = mdMsgObjThing;
                    vptMsg.x = mb.rgParam[0];
                    break;
                case gotoBattleReport:
                    mdMsgObj = mdMsgObjBattleReport;
                    break;
                default:
                    if ((mb.wGoto & 0xc000) == 0xc000) {
                        mdMsgObj = mdMsgObjPart;
                    } else if (mb.wGoto & 0x4000) {
                        idMsgObj = mb.wGoto & 0x3fff;
                        if (idMsgObj == 2048) {
                            mdMsgObj = mdMsgObjRelations;
                        } else {
                            mdMsgObj = mdMsgObjBattle;
                            vptMsg.x = mb.rgParam[0];
                            vptMsg.y = mb.rgParam[1];
                        }
                    } else if (mb.wGoto < 0) {
                        if (LpflFromId(idMsgObj) != 0) {
                            mdMsgObj = mdMsgObjFleet;
                        } else {
                            mdMsgObj = mdMsgObjNone;
                        }
                    } else {
                        mdMsgObj = mdMsgObjPlanet;
                    }
                }
            } else {
                mdMsgObj = mdMsgObjNone;
            }
        }
        if (mdMsgObj != mdMsgObjNone &&
            (iMsgCur < 0 || (!fViewFilteredMsg && (bitfMsgFiltered[IdmGetMessageN(iMsgCur) >> 3] & 1 << (IdmGetMessageN(iMsgCur) & 7))))) {
            mdMsgObj = mdMsgObjNone;
        }
        EnableWindow(rghwndMsgBtn[1], mdMsgObj != mdMsgObjNone);
    FinishUp:
        strcpy(szMsgTitle, szWork);
        InvalidateRect(hwndMessage, &rcMsgTitle, TRUE);
    }
    return;
}

// UpdateMsgTitle refreshes the message window's title bar, as when the
// game becomes dirty.
void UpdateMsgTitle() {
    SetMsgTitle(hwndMessage);
    return;
}
