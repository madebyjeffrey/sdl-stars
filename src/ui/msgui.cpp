#include "win.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"
#include "heap_io.h"
#include "part_query_io.h"
#include "record_io.h"

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
            rghwndMsgBtn[i] = ui::CreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(static_cast<StringId>(idsPrev2 + i)), WS_CHILD, 100, 100, i == 3 ? 50 : 44, (3 * dyArial8 >> 1) - 1, hwnd,
                                           NULL, hInst, NULL);
            ControlIO().pfnSendMessage(rghwndMsgBtn[i], WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        }
        hwndMsgDrop = ui::CreateWindowEx(0, "COMBOBOX", "MsgDD", CBS_DROPDOWNLIST | WS_CHILD | WS_VSCROLL, 100, 100, 200, 80, hwnd, NULL, hInst, NULL);
        ControlIO().pfnSendMessage(hwndMsgDrop, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        hwndMsgEdit = ui::CreateWindowEx(0, "EDIT", NULL, ES_MULTILINE | ES_AUTOVSCROLL | WS_CHILD | WS_BORDER, 100, 100, 200, 50, hwnd, NULL, hInst, NULL);
        ControlIO().pfnSendMessage(hwndMsgEdit, EM_LIMITTEXT, 0x3c8, 0);
        ControlIO().pfnSendMessage(hwndMsgEdit, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        hwndMsgScroll = ui::CreateWindowEx(0, "EDIT", NULL, ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_CHILD | WS_BORDER | WS_VSCROLL, 100, 100, 200, 50, hwnd,
                                     NULL, hInst, NULL);
        ControlIO().pfnSetMsgTitle(hwnd);
        ControlIO().pfnSendMessage(hwndMsgDrop, CB_ADDSTRING, 0, (LPARAM)TextIO().pfnCompressed(idsEverybody));
        for (i = 0; i < game.cPlayer; i++) {
            psz = TextIO().pfnPlayerName(i, TRUE, TRUE, TRUE, 0, NULL);
            ControlIO().pfnSendMessage(hwndMsgDrop, CB_ADDSTRING, 0, (LPARAM)psz);
        }
        ControlIO().pfnSendMessage(hwndMsgDrop, CB_SETCURSEL, 0, 0);
        break;
    case WM_SIZE:
        dx = LOWORD(lParam);
        dy = HIWORD(lParam);
        for (i = 0; i < 3; i++) {
            ControlIO().pfnSetWindowPos(rghwndMsgBtn[i], NULL, dx - 48, ((3 * dyArial8 >> 1) + 2) * i + 3 + dyArial8 * 2, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
        win32::SetRect(&rcMsgText, 4, dyArial8 * 2 + 3, dx - 52, dy - 4);
        win32::SetRect(&rcMsgTitle, 4, 4, dx - 4, dyArial8 * 2 - 4);
        rc = rcMsgText;
        ExpandRc(&rc, -4, -4);
        ControlIO().pfnSetWindowPos(hwndMsgDrop, NULL, rc.left + 30, rc.top, rc.right - rc.left - 84, rc.bottom - rc.top, SWP_NOZORDER);
        ControlIO().pfnSetWindowPos(rghwndMsgBtn[3], NULL, rc.right - 50, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        rc.top += dyShipDD + 3;
        ControlIO().pfnSetWindowPos(hwndMsgEdit, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);
        goto Default;
    case WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_SETCURSOR:
        hcs = 0;
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (HtMsgBox(pt) == htMsgNone)
            goto Default;
        win32::SetCursor(hcurHand);
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
            idm = static_cast<MessageId>(PresentationIO().pfnMessageId(iMsgCur));
            fSet = (bitfMsgFiltered[idm >> 3] & 1 << (idm & 7)) != 0;
            SetFilteringGroups(idm, fSet == 0);
            RecordIO().pfnDirtyGame(TRUE);
            if (gd.fTutorial) {
                PresentationIO().pfnTutor();
            }
            ControlIO().pfnInvalidateRect(hwndMessage, NULL, TRUE);
            ControlIO().pfnSetMsgTitle(hwnd);
        } else if (ht == htMsgZoom) {
        ZoomBox:
            fViewFilteredMsg = fViewFilteredMsg == 0;
            if (iMsgCur < 0 || ((bitfMsgFiltered[PresentationIO().pfnMessageId(iMsgCur) >> 3] & 1 << (PresentationIO().pfnMessageId(iMsgCur) & 7)) != 0) != fViewFilteredMsg) {
                i = IMsgNext(fViewFilteredMsg);
                if (i == imsgNone) {
                    i = IMsgPrev(fViewFilteredMsg);
                }
                iMsgCur = i;
            }
            ControlIO().pfnInvalidateRect(hwndMessage, NULL, TRUE);
            ControlIO().pfnSetMsgTitle(hwnd);
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
            ControlIO().pfnInvalidateRect(hwndMessage, NULL, TRUE);
            ControlIO().pfnSetMsgTitle(hwnd);
            ControlIO().pfnSetFocus(hwndMsgEdit);
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
        win32::SetBkColor((HDC)wParam, crButtonFace);
        return (LRESULT)hbrButtonFace;
    case WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        _Draw3dFrame(hdc, &rcMsgTitle, 0);
        crFore = win32::SetTextColor(hdc, crButtonText);
        crBack = win32::SetBkColor(hdc, crButtonFace);
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
        hbrSav = (HBRUSH)win32::SelectObject(hdc, hbrButtonShadow);
        win32::PatBlt(hdc, rc.left, rc.top, dx, 1, PATCOPY);
        win32::PatBlt(hdc, rc.left, rc.top, 1, dy, PATCOPY);
        win32::SelectObject(hdc, hbrButtonHilite);
        win32::PatBlt(hdc, rc.left, rc.bottom - 1, dx, 1, PATCOPY);
        win32::PatBlt(hdc, rc.right - 1, rc.top, 1, dy, PATCOPY);
        win32::SelectObject(hdc, hbrSav);
        ExpandRc(&rc, -4, -4);
        if (!gd.fSendMsgMode) {
            if (iMsgCur >= cMsg) {
                lpmsgplr = vlpmsgplrIn;
                for (i = cMsg; i < iMsgCur; i++) {
                    lpmsgplr = lpmsgplr->lpmsgplrNext;
                }
                if (TextIO().pfnCchGetString(idsSCC, szT) >= 32) {
                }
                cch = win32::wsprintf((LPSTR)lpb2k, szT, TextIO().pfnPlayerName(lpmsgplr->iPlrFrom, TRUE, TRUE, TRUE, 0, NULL), 13, 10);
                if (TextIO().pfnCchGetString(idsSCC2, szT) >= 32) {
                }
                cch += win32::wsprintf((LPSTR)lpb2k + cch, szT,
                                lpmsgplr->iPlrTo == 0 ? TextIO().pfnCompressed(idsEverybody) : TextIO().pfnPlayerName(lpmsgplr->iPlrTo - 1, TRUE, TRUE, TRUE, 0, NULL),
                                13, 10);
                if (lpmsgplr->cLen >= 0) {
                    i = 1000;
                    FDecompressUserString((char *)lpmsgplr->rgbMsg(), lpmsgplr->cLen, (char *)lpb2k + cch, &i);
                } else {
                    strcpy((char *)lpb2k + cch, (const char *)lpmsgplr->rgbMsg());
                }
                lpsz = (char *)lpb2k;
            } else {
                idm = static_cast<MessageId>(PresentationIO().pfnMessageId(iMsgCur));
                if (iMsgCur < 0 && cMsg > 0) {
                    lpsz = TextIO().pfnCompressed(idsMessagesHaveSentYearFilteredIfWant);
                } else if (iMsgCur >= 0 && (bitfMsgFiltered[idm >> 3] & 1 << (idm & 7)) && !fViewFilteredMsg) {
                    lpsz = TextIO().pfnCompressed(idsMessageTypeHasFilteredWillShownDefault);
                } else {
                    lpsz = PszGetMessageN(iMsgCur);
                }
            }
            win32::SetTextColor(hdc, 0xffffff);
            iMode = win32::SetBkMode(hdc, TRANSPARENT);
            if ((iMsgCur < 0 && cMsg > 0) ||
                (iMsgCur >= 0 && iMsgCur < cMsg && (bitfMsgFiltered[PresentationIO().pfnMessageId(iMsgCur) >> 3] & 1 << (PresentationIO().pfnMessageId(iMsgCur) & 7)))) {
                cch = TextIO().pfnCchGetString(idsFiltered, szWork);
                DiaganolTextOut(hdc, &rc, szWork, cch);
                lpsz = PszGetMessageN(iMsgCur);
            }
            win32::SetTextColor(hdc, crButtonText);
            rcActual = rc;
            win32::DrawText(hdc, lpsz, strlen(lpsz), &rcActual, DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);
            if (rcActual.bottom <= rc.bottom && rcActual.right <= rc.right) {
                ui::ShowWindow(hwndMsgScroll, SW_HIDE);
                win32::DrawText(hdc, lpsz, strlen(lpsz), &rc, DT_WORDBREAK | DT_NOPREFIX);
            } else {
                ControlIO().pfnSetWindowText(hwndMsgScroll, lpsz);
                ExpandRc(&rc, 4, 4);
                ControlIO().pfnSetWindowPos(hwndMsgScroll, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_SHOWWINDOW);
            }
            win32::SetBkMode(hdc, iMode);
        } else {
            iMode = win32::SetBkMode(hdc, TRANSPARENT);
            win32::SetTextColor(hdc, crButtonText);
            cch = TextIO().pfnCchGetString(idsTo3, szT);
            RightTextOut(hdc, rc.left + 26, rc.top, szT, cch, 0);
            win32::SetBkMode(hdc, iMode);
        }
        win32::SetTextColor(hdc, crFore);
        win32::SetBkColor(hdc, crBack);
        win32::EndPaint(hwnd, &ps);
        break;
    case WM_MOUSEWHEEL:
        if (gd.fSendMsgMode || (LOWORD(wParam) & (MK_CONTROL | MK_SHIFT))) return 0;
        i = CWheelNotches(&dWheelMessages, (int16_t)HIWORD(wParam));
        while (i > 0) { ControlIO().pfnSendMessage(hwnd, WM_KEYDOWN, VK_UP, 1); i--; }
        while (i < 0) { ControlIO().pfnSendMessage(hwnd, WM_KEYDOWN, VK_DOWN, 1); i++; }
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
            ControlIO().pfnSetFocus(hwndFrame);
        }
        if ((HWND)lParam == rghwndMsgBtn[0] && HIWORD(wParam) == 0) {
        PrevMsg:
            if (gd.fSendMsgMode) {
                FFinishPlrMsgEntry(-1);
                goto SetupNewMsg;
            }
            if (ControlIO().pfnGetAsyncKeyState(VK_SHIFT) & 0xfffe) {
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
            ControlIO().pfnSetMsgTitle(hwnd);
            ControlIO().pfnInvalidateRect(hwnd, &rcMsgText, TRUE);
            if (gd.fTutorial) {
                tutor.fChange = TRUE;
                PresentationIO().pfnTutor();
            }
            break;
        } else if ((HWND)lParam == rghwndMsgBtn[2] && HIWORD(wParam) == 0) {
        NextMsg:
            if (gd.fSendMsgMode) {
                FFinishPlrMsgEntry(1);
            } else {
                if (ControlIO().pfnGetAsyncKeyState(VK_SHIFT) & 0xfffe) {
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
                    PresentationIO().pfnSelectPlanet(0, idMsgObj);
                    ControlIO().pfnUpdateWindow(hwndScanner);
                    ControlIO().pfnSendMessage(hwndScanner, WM_CHAR, 'v', 0);
                    idm = static_cast<MessageId>(PresentationIO().pfnMessageId(iMsgCur));
                    if (idm != idmHasCompletedOrdersProductionQueueEmpty && idm != idmProductionQueueEmpty &&
                        (idm < idmHasBuiltManyMinesCurrentPopulationCan || idm > idmHasBuiltManyDefensesPlanetCanSupport))
                        break;
                    if (!gd.fGotoVCR) {
                        gd.fGotoVCR = TRUE;
                        ControlIO().pfnSetMsgTitle(hwnd);
                        break;
                    }
                    if (sel.grobj != grobjPlanet || sel.id != idMsgObj)
                        break;
                    ChangeProduction(FALSE);
                    break;
                case mdMsgObjFleet:
                    PresentationIO().pfnSelectFleet(0, idMsgObj);
                    ControlIO().pfnUpdateWindow(hwndScanner);
                    ControlIO().pfnSendMessage(hwndScanner, WM_CHAR, 'v', 0);
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
                    ControlIO().pfnSetMsgTitle(hwnd);
                    break;
                case mdMsgObjResearch:
                    win32::PostMessage(hwndFrame, WM_COMMAND, IDM_GAME_RESEARCH, 0);
                    break;
                case mdMsgObjScore:
                    win32::PostMessage(hwndFrame, WM_COMMAND, IDM_GAME_SCORE, 0);
                    break;
                case mdMsgObjShipDesign:
                    win32::PostMessage(hwndFrame, WM_COMMAND, IDM_GAME_SHIP_BUILDER, 0);
                    break;
                case mdMsgObjRelations:
                    win32::PostMessage(hwndFrame, WM_COMMAND, IDM_GAME_RELATIONS2, 0);
                    break;
                case mdMsgObjBattleReport:
                    if (hwndReportDlg && vprptCur == &vrptBattle)
                        break;
                    win32::PostMessage(hwndFrame, WM_COMMAND, IDM_REPORT_BATTLE, 0);
                    break;
                case mdMsgObjPart:
                    vpartBrowser.hs.grhst = static_cast<HullSlotType>(1 << (idMsgObj >> 8 & 0xf));
                    vpartBrowser.hs.iItem = idMsgObj & 0xff;
                    PartQueryIO().pfnLookup(&vpartBrowser);
                    if (hwndBrowser) {
                        ControlIO().pfnInvalidateRect(hwndBrowserChild, NULL, TRUE);
                    } else {
                        fBrowserValid = TRUE;
                        win32::PostMessage(hwndFrame, WM_COMMAND, IDM_VIEW_BROWSER_TOGGLE2, 0);
                    }
                }
            }
            if (!gd.fTutorial)
                break;
            tutor.fChange = TRUE;
            PresentationIO().pfnTutor();
            break;
        } else {
            goto Default;
        }
    default:
    Default:
        return ui::DefWindowProc(hwnd, message, wParam, lParam);
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

    hdcMem = win32::CreateCompatibleDC(hdc);
    hbmpSav = (HBITMAP)win32::SelectObject(hdcMem, hbmpMono);
    if (gd.fSendMsgMode)
        goto Cleanup;
    xyStart = (int16_t)(prc->bottom - prc->top - 11) / 2 + prc->top;
    if (iMsgCur < 0 || iMsgCur >= cMsg)
        goto DoMinMax;
    idm = static_cast<MessageId>(PresentationIO().pfnMessageId(iMsgCur));
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
    crTextSav = win32::SetTextColor(hdc, 0);
    crBkSav = win32::SetBkColor(hdc, 0xffffff);
    win32::BitBlt(hdc, xDst, yDst, dxSrc, dySrc, hdcMem, 0, ySrcMask, SRCAND);
    hbmpSav = (HBITMAP)win32::SelectObject(hdcMem, hbmpMsg);
    win32::BitBlt(hdc, xDst, yDst, dxSrc, dySrc, hdcMem, 0, ySrc, SRCPAINT);
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
    hbrSav = (HBRUSH)win32::SelectObject(hdc, hbrButtonShadow);
    win32::PatBlt(hdc, xDst, prc->top, 1, prc->bottom - prc->top, PATCOPY);
    win32::SelectObject(hdc, hbrButtonHilite);
    win32::PatBlt(hdc, xDst + 1, prc->top, 1, prc->bottom - prc->top, PATCOPY);
    win32::SelectObject(hdc, hbrSav);
    yDst = (int16_t)(prc->bottom - prc->top - 15) / 2 + prc->top;
    xDst = prc->right - (prc->bottom - yDst);
    win32::SelectObject(hdcMem, hbmpMono);
    win32::BitBlt(hdc, xDst, yDst, 15, 15, hdcMem, 0, ySrcMask, SRCAND);
    win32::SelectObject(hdcMem, hbmpMsg);
    win32::BitBlt(hdc, xDst, yDst, 15, 15, hdcMem, 0, ySrc, SRCPAINT);
Cleanup:
    win32::SetTextColor(hdc, crTextSav);
    win32::SetBkColor(hdc, crBkSav);
    if (!game.fSinglePlr) {
        win32::SelectObject(hdcMem, hbmpMsg);
        xDst = prc->right - 45;
        yDst = (int16_t)(prc->bottom - prc->top - 7) / 2 + prc->top;
        win32::PatBlt(hdc, xDst - 1, yDst - 1, 17, 11, BLACKNESS);
        win32::BitBlt(hdc, xDst, yDst, 15, 9, hdcMem, 0, 56, SRCCOPY);
    }
    win32::SelectObject(hdcMem, hbmpSav);
    win32::DeleteDC(hdcMem);
    return;
}

HtMsgType HtMsgBox(POINT16 pt) {
    int16_t i;

    if (win32::PtInRect(&rcMsgTitle, PointFrom16(pt)) != 0) {
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
        cb = ControlIO().pfnGetWindowText(hwndMsgEdit, (LPSTR)lpb2k, 1000);
    }
    if (cb == 0) {
        if (lpmpCur) {
            RecordIO().pfnDirtyGame(TRUE);
            *lpmpPrev = lpmpCur->lpmsgplrNext;
            HeapIO().pfnFree(lpmpCur, htPlrMsg);
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
    iPlrTo = LOWORD(ControlIO().pfnSendMessage(hwndMsgDrop, CB_GETCURSEL, 0, 0));
    if (lpmpCur) {
        if (cb != lpmpCur->cLen || lpmpCur->iPlrTo != iPlrTo || memcmp(lpmpCur->rgbMsg(), lpbMsg, abs(cb)) != 0) {
            RecordIO().pfnDirtyGame(TRUE);
        }
        lpmpCur = LpmsgplrReAlloc(lpmpCur, cbNew + (sizeof(MSGPLR) - 12));
    } else {
        RecordIO().pfnDirtyGame(TRUE);
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
        ui::ShowWindow(hwndMsgEdit, sw);
        ui::ShowWindow(hwndMsgDrop, sw);
        ui::ShowWindow(rghwndMsgBtn[3], sw);
        if (gd.fSendMsgMode) {
            ui::ShowWindow(hwndMsgScroll, SW_HIDE);
        }
        cMsgTot = cMsg + vcmsgplrIn;
        i = gd.fSendMsgMode ? idsDone : iMsgCur >= cMsg ? idsReply : gd.fGotoVCR ? idsView : idsGoto3;
        ControlIO().pfnSetWindowText(rghwndMsgBtn[1], TextIO().pfnCompressed(static_cast<StringId>(i)));
        if (gd.fSendMsgMode) {
            win32::wsprintf(szWork, TextIO().pfnCompressed(idsSendMessagesDD), iMsgSendCur + 1, vcmsgplrOut);
            rc = rcMsgText;
            ExpandRc(&rc, -4, -4);
            ControlIO().pfnSetWindowPos(hwndMsgDrop, NULL, rc.left + 30, rc.top, rc.right - rc.left - 84, rc.bottom - rc.top, SWP_NOZORDER | SWP_SHOWWINDOW);
            ControlIO().pfnSetWindowPos(rghwndMsgBtn[3], NULL, rc.right - 50, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
            rc.top += dyShipDD + 3;
            ControlIO().pfnSetWindowPos(hwndMsgEdit, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_SHOWWINDOW);
            ControlIO().pfnEnableWindow(rghwndMsgBtn[0], iMsgSendCur > 0);
            ControlIO().pfnEnableWindow(rghwndMsgBtn[1], TRUE);
            ControlIO().pfnEnableWindow(rghwndMsgBtn[2], TRUE);
            lpmp = vlpmsgplrOut;
            i = iMsgSendCur;
            while (i-- > 0) {
                lpmp = lpmp->lpmsgplrNext;
            }
            if (lpmp) {
                ControlIO().pfnSendMessage(hwndMsgDrop, CB_SETCURSEL, lpmp->iPlrTo, 0);
                if (lpmp->cLen < 0) {
                    ControlIO().pfnSetWindowText(hwndMsgEdit, (LPCSTR)lpmp->rgbMsg());
                    goto FinishUp;
                } else {
                    i = 1000;
                    FDecompressUserString((char *)lpmp->rgbMsg(), lpmp->cLen, (char *)lpb2k, &i);
                    ControlIO().pfnSetWindowText(hwndMsgEdit, (LPCSTR)lpb2k);
                }
                goto FinishUp;
            } else {
                ControlIO().pfnSendMessage(hwndMsgDrop, CB_SETCURSEL, viInRe, 0);
                ControlIO().pfnSetWindowText(hwndMsgEdit, "");
                goto FinishUp;
            }
        }
        if (cMsgTot != 0) {
            TextIO().pfnCchGetString(idsYearDCMessagesDD, szT);
            win32::wsprintf(szWork, szT, game.turn + 2400, ch, iMsgCur + 1, cMsgTot);
        } else {
            TextIO().pfnCchGetString(idsYearDCMessagesNone, szT);
            win32::wsprintf(szWork, szT, game.turn + 2400, ch);
        }
        ControlIO().pfnEnableWindow(rghwndMsgBtn[0], IMsgPrev(FALSE) != imsgNone);
        ControlIO().pfnEnableWindow(rghwndMsgBtn[2], IMsgNext(FALSE) != imsgNone);
        if (iMsgCur >= cMsg) {
            ControlIO().pfnEnableWindow(rghwndMsgBtn[1], TRUE);
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
            (iMsgCur < 0 || (!fViewFilteredMsg && (bitfMsgFiltered[PresentationIO().pfnMessageId(iMsgCur) >> 3] & 1 << (PresentationIO().pfnMessageId(iMsgCur) & 7))))) {
            mdMsgObj = mdMsgObjNone;
        }
        ControlIO().pfnEnableWindow(rghwndMsgBtn[1], mdMsgObj != mdMsgObjNone);
    FinishUp:
        strcpy(szMsgTitle, szWork);
        ControlIO().pfnInvalidateRect(hwndMessage, &rcMsgTitle, TRUE);
    }
    return;
}

// UpdateMsgTitle refreshes the message window's title bar, as when the
// game becomes dirty.
void UpdateMsgTitle() {
    ControlIO().pfnSetMsgTitle(hwndMessage);
    return;
}

void ResetMessageInputSession() noexcept { dWheelMessages = 0; }
