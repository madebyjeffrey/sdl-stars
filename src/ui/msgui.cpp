#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"
#include "heap_io.h"
#include "part_query_io.h"
#include "record_io.h"

static std::int16_t dWheelMessages;

LRESULT  MessageWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HDC         hdc;
    std::int16_t     i;
    char       *psz;
    PAINTSTRUCT ps;
    std::int16_t     dy;
    std::int16_t     dx;
    RECT        rc;
    POINT16     pt;
    HCURSOR     hcs;
    HtMsgType   ht;
    std::int16_t     fSet;
    MessageId   idm;
    MSGPLR     *lpmp;
    MSGPLR     *lpmpSrc;
    char       *lpsz;
    HBRUSH      hbrSav;
    COLORREF    crFore;
    std::int16_t     dxMax;
    COLORREF    crBack;
    RECT        rcActual;
    std::int16_t     cch;
    std::int16_t     iMode;
    char        szT[32];
    MSGPLR     *lpmsgplr;
    THING      *lpth;
    SCAN        scan;
    FARPROC     lpProc;
    std::int16_t     fRet;

    switch (message) {
    case win32::WM_CREATE:
        dWheelMessages = 0;
        for (i = 0; i < 4; i++) {
            app.lpui->hwndMessage = hwnd;
            app.lpui->rghwndMsgBtn[i] = ui::CreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(app, static_cast<StringId>(idsPrev2 + i)), win32::WS_CHILD, 100, 100, i == 3 ? 50 : 44, (3 * app.state.dyArial8 >> 1) - 1, hwnd,
                                           nullptr, app.lpui->hInst, nullptr);
            ControlIO().pfnSendMessage(app.lpui->rghwndMsgBtn[i], win32::WM_SETFONT, reinterpret_cast<WPARAM>(app.lpui->rghfontArial8[1]), 0);
        }
        app.lpui->hwndMsgDrop = ui::CreateWindowEx(0, "COMBOBOX", "MsgDD", win32::CBS_DROPDOWNLIST | win32::WS_CHILD | win32::WS_VSCROLL, 100, 100, 200, 80, hwnd, nullptr, app.lpui->hInst, nullptr);
        ControlIO().pfnSendMessage(app.lpui->hwndMsgDrop, win32::WM_SETFONT, reinterpret_cast<WPARAM>(app.lpui->rghfontArial8[1]), 0);
        app.lpui->hwndMsgEdit = ui::CreateWindowEx(0, "EDIT", nullptr, win32::ES_MULTILINE | win32::ES_AUTOVSCROLL | win32::WS_CHILD | win32::WS_BORDER, 100, 100, 200, 50, hwnd, nullptr, app.lpui->hInst, nullptr);
        ControlIO().pfnSendMessage(app.lpui->hwndMsgEdit, win32::EM_LIMITTEXT, 0x3c8, 0);
        ControlIO().pfnSendMessage(app.lpui->hwndMsgEdit, win32::WM_SETFONT, reinterpret_cast<WPARAM>(app.lpui->rghfontArial8[1]), 0);
        app.lpui->hwndMsgScroll = ui::CreateWindowEx(0, "EDIT", nullptr, win32::ES_MULTILINE | win32::ES_AUTOVSCROLL | win32::ES_READONLY | win32::WS_CHILD | win32::WS_BORDER | win32::WS_VSCROLL, 100, 100, 200, 50, hwnd,
                                     nullptr, app.lpui->hInst, nullptr);
        ControlIO().pfnSetMsgTitle(app, hwnd);
        ControlIO().pfnSendMessage(app.lpui->hwndMsgDrop, win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TextIO().pfnCompressed(app, idsEverybody)));
        for (i = 0; i < app.state.game.cPlayer; i++) {
            psz = TextIO().pfnPlayerName(app, i, 1, 1, 1, 0, nullptr);
            ControlIO().pfnSendMessage(app.lpui->hwndMsgDrop, win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(psz));
        }
        ControlIO().pfnSendMessage(app.lpui->hwndMsgDrop, win32::CB_SETCURSEL, 0, 0);
        break;
    case win32::WM_SIZE:
        dx = LOWORD(lParam);
        dy = HIWORD(lParam);
        for (i = 0; i < 3; i++) {
            ControlIO().pfnSetWindowPos(app.lpui->rghwndMsgBtn[i], nullptr, dx - 48, ((3 * app.state.dyArial8 >> 1) + 2) * i + 3 + app.state.dyArial8 * 2, 0, 0,
                         win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE | win32::SWP_SHOWWINDOW);
        }
        win32::SetRect(&app.lpui->rcMsgText, 4, app.state.dyArial8 * 2 + 3, dx - 52, dy - 4);
        win32::SetRect(&app.lpui->rcMsgTitle, 4, 4, dx - 4, app.state.dyArial8 * 2 - 4);
        rc = app.lpui->rcMsgText;
        ExpandRc(&rc, -4, -4);
        ControlIO().pfnSetWindowPos(app.lpui->hwndMsgDrop, nullptr, rc.left + 30, rc.top, rc.right - rc.left - 84, rc.bottom - rc.top, win32::SWP_NOZORDER);
        ControlIO().pfnSetWindowPos(app.lpui->rghwndMsgBtn[3], nullptr, rc.right - 50, rc.top, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER);
        rc.top += app.lpui->dyShipDD + 3;
        ControlIO().pfnSetWindowPos(app.lpui->hwndMsgEdit, nullptr, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, win32::SWP_NOZORDER);
        goto Default;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_SETCURSOR:
        hcs = 0;
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (HtMsgBox(app, pt) == htMsgNone)
            goto Default;
        win32::SetCursor(app.lpui->hcurHand);
        return 1;
    case win32::WM_LBUTTONDOWN:
    case win32::WM_LBUTTONDBLCLK:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        ht = HtMsgBox(app, pt);
        if (ht == htMsgCurrent) {
        CheckBox:
            if (app.state.iMsgCur < 0)
                break;
            idm = static_cast<MessageId>(PresentationIO().pfnMessageId(app, app.state.iMsgCur));
            fSet = (app.state.bitfMsgFiltered[idm >> 3] & 1 << (idm & 7)) != 0;
            SetFilteringGroups(app, idm, fSet == 0);
            RecordIO().pfnDirtyGame(app, 1);
            if (app.state.gd.fTutorial) {
                PresentationIO().pfnTutor(app);
            }
            ControlIO().pfnInvalidateRect(app.lpui->hwndMessage, nullptr, 1);
            ControlIO().pfnSetMsgTitle(app, hwnd);
        } else if (ht == htMsgZoom) {
        ZoomBox:
            app.state.fViewFilteredMsg = app.state.fViewFilteredMsg == 0;
            if (app.state.iMsgCur < 0 || ((app.state.bitfMsgFiltered[PresentationIO().pfnMessageId(app, app.state.iMsgCur) >> 3] & 1 << (PresentationIO().pfnMessageId(app, app.state.iMsgCur) & 7)) != 0) != app.state.fViewFilteredMsg) {
                i = IMsgNext(app, app.state.fViewFilteredMsg);
                if (i == imsgNone) {
                    i = IMsgPrev(app, app.state.fViewFilteredMsg);
                }
                app.state.iMsgCur = i;
            }
            ControlIO().pfnInvalidateRect(app.lpui->hwndMessage, nullptr, 1);
            ControlIO().pfnSetMsgTitle(app, hwnd);
        } else if (ht == htMsgMode) {
        ToggleMsgMode:
            if (app.state.gd.fSendMsgMode) {
                FFinishPlrMsgEntry(app, 0);
            } else if (app.state.iMsgCur >= app.state.cMsg) {
                lpmpSrc = app.state.vlpmsgplrIn;
                i = app.state.iMsgCur - app.state.cMsg;
                while (i-- != 0) {
                    lpmpSrc = lpmpSrc->lpmsgplrNext;
                }
                lpmp = app.state.vlpmsgplrOut;
                app.state.iMsgSendCur = 0;
                while (lpmp && (lpmp->iPlrTo - 1 != lpmpSrc->iPlrFrom || lpmp->iInRe != app.state.iMsgCur)) {
                    lpmp = lpmp->lpmsgplrNext;
                    app.state.iMsgSendCur++;
                }
                app.lpui->viInRe = lpmpSrc->iPlrFrom + 1;
            } else {
                app.lpui->viInRe = 0;
            }
            app.state.gd.fSendMsgMode = app.state.gd.fSendMsgMode == 0;
            ControlIO().pfnInvalidateRect(app.lpui->hwndMessage, nullptr, 1);
            ControlIO().pfnSetMsgTitle(app, hwnd);
            ControlIO().pfnSetFocus(app.lpui->hwndMsgEdit);
        }
        break;
    case win32::WM_GETMINMAXINFO:
        (reinterpret_cast<MINMAXINFO *>(lParam))->ptMinTrackSize.x = app.lpui->dxWinFrame * 2 + 198;
        (reinterpret_cast<MINMAXINFO *>(lParam))->ptMinTrackSize.y = (0xd * app.state.dyArial8 >> 1) + 0x16;
        goto Default;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        if (reinterpret_cast<HWND>(lParam) != app.lpui->hwndMsgScroll)
            goto Default;
        win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
        return reinterpret_cast<LRESULT>(app.lpui->hbrButtonFace);
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        _Draw3dFrame(app, hdc, &app.lpui->rcMsgTitle, 0);
        crFore = win32::SetTextColor(hdc, app.lpui->crButtonText);
        crBack = win32::SetBkColor(hdc, app.lpui->crButtonFace);
        cch = std::strlen(app.lpui->szMsgTitle);
        dxMax = app.lpui->rcMsgTitle.right - app.lpui->rcMsgTitle.left - 48;
        while (cch > 0 && static_cast<std::int16_t>(LOWORD(GetTextExtent(hdc, app.lpui->szMsgTitle, cch))) > dxMax) {
            cch = static_cast<std::int16_t>(CchUtf8Prefix(app.lpui->szMsgTitle, cch - 1));
        }
        RcCtrTextOut(app, hdc, &app.lpui->rcMsgTitle, app.lpui->szMsgTitle, cch);
        DecorateMsgTitleBar(app, hdc, &app.lpui->rcMsgTitle);
        rc = app.lpui->rcMsgText;
        dx = rc.right - rc.left;
        dy = rc.bottom - rc.top;
        hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrButtonShadow));
        win32::PatBlt(hdc, rc.left, rc.top, dx, 1, win32::PATCOPY);
        win32::PatBlt(hdc, rc.left, rc.top, 1, dy, win32::PATCOPY);
        win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
        win32::PatBlt(hdc, rc.left, rc.bottom - 1, dx, 1, win32::PATCOPY);
        win32::PatBlt(hdc, rc.right - 1, rc.top, 1, dy, win32::PATCOPY);
        win32::SelectObject(hdc, hbrSav);
        ExpandRc(&rc, -4, -4);
        if (!app.state.gd.fSendMsgMode) {
            if (app.state.iMsgCur >= app.state.cMsg) {
                lpmsgplr = app.state.vlpmsgplrIn;
                for (i = app.state.cMsg; i < app.state.iMsgCur; i++) {
                    lpmsgplr = lpmsgplr->lpmsgplrNext;
                }
                if (TextIO().pfnCchGetString(app, idsSCC, szT) >= 32) {
                }
                cch = win32::wsprintfA(reinterpret_cast<LPSTR>(app.lpui->lpb2k), szT, TextIO().pfnPlayerName(app, lpmsgplr->iPlrFrom, 1, 1, 1, 0, nullptr), 13, 10);
                if (TextIO().pfnCchGetString(app, idsSCC2, szT) >= 32) {
                }
                cch += win32::wsprintfA(reinterpret_cast<LPSTR>(app.lpui->lpb2k) + cch, szT,
                                lpmsgplr->iPlrTo == 0 ? TextIO().pfnCompressed(app, idsEverybody) : TextIO().pfnPlayerName(app, lpmsgplr->iPlrTo - 1, 1, 1, 1, 0, nullptr),
                                13, 10);
                if (lpmsgplr->cLen >= 0) {
                    i = 1000;
                    FDecompressUserString(app, reinterpret_cast<char *>(lpmsgplr->rgbMsg()), lpmsgplr->cLen, reinterpret_cast<char *>(app.lpui->lpb2k) + cch, &i);
                } else {
                    std::strcpy(reinterpret_cast<char *>(app.lpui->lpb2k) + cch, reinterpret_cast<const char *>(lpmsgplr->rgbMsg()));
                }
                lpsz = reinterpret_cast<char *>(app.lpui->lpb2k);
            } else {
                idm = static_cast<MessageId>(PresentationIO().pfnMessageId(app, app.state.iMsgCur));
                if (app.state.iMsgCur < 0 && app.state.cMsg > 0) {
                    lpsz = TextIO().pfnCompressed(app, idsMessagesHaveSentYearFilteredIfWant);
                } else if (app.state.iMsgCur >= 0 && (app.state.bitfMsgFiltered[idm >> 3] & 1 << (idm & 7)) && !app.state.fViewFilteredMsg) {
                    lpsz = TextIO().pfnCompressed(app, idsMessageTypeHasFilteredWillShownDefault);
                } else {
                    lpsz = PszGetMessageN(app, app.state.iMsgCur);
                }
            }
            win32::SetTextColor(hdc, 0xffffff);
            iMode = win32::SetBkMode(hdc, win32::TRANSPARENT);
            if ((app.state.iMsgCur < 0 && app.state.cMsg > 0) ||
                (app.state.iMsgCur >= 0 && app.state.iMsgCur < app.state.cMsg && (app.state.bitfMsgFiltered[PresentationIO().pfnMessageId(app, app.state.iMsgCur) >> 3] & 1 << (PresentationIO().pfnMessageId(app, app.state.iMsgCur) & 7)))) {
                cch = TextIO().pfnCchGetString(app, idsFiltered, app.state.szWork);
                DiaganolTextOut(app, hdc, &rc, app.state.szWork, cch);
                lpsz = PszGetMessageN(app, app.state.iMsgCur);
            }
            win32::SetTextColor(hdc, app.lpui->crButtonText);
            rcActual = rc;
            win32::DrawTextA(hdc, lpsz, std::strlen(lpsz), &rcActual, win32::DT_WORDBREAK | win32::DT_CALCRECT | win32::DT_NOPREFIX);
            if (rcActual.bottom <= rc.bottom && rcActual.right <= rc.right) {
                ui::ShowWindow(app.lpui->hwndMsgScroll, win32::SW_HIDE);
                win32::DrawTextA(hdc, lpsz, std::strlen(lpsz), &rc, win32::DT_WORDBREAK | win32::DT_NOPREFIX);
            } else {
                ControlIO().pfnSetWindowText(app.lpui->hwndMsgScroll, lpsz);
                ExpandRc(&rc, 4, 4);
                ControlIO().pfnSetWindowPos(app.lpui->hwndMsgScroll, nullptr, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, win32::SWP_NOZORDER | win32::SWP_SHOWWINDOW);
            }
            win32::SetBkMode(hdc, iMode);
        } else {
            iMode = win32::SetBkMode(hdc, win32::TRANSPARENT);
            win32::SetTextColor(hdc, app.lpui->crButtonText);
            cch = TextIO().pfnCchGetString(app, idsTo3, szT);
            RightTextOut(app, hdc, rc.left + 26, rc.top, szT, cch, 0);
            win32::SetBkMode(hdc, iMode);
        }
        win32::SetTextColor(hdc, crFore);
        win32::SetBkColor(hdc, crBack);
        win32::EndPaint(hwnd, &ps);
        break;
    case win32::WM_MOUSEWHEEL:
        if (app.state.gd.fSendMsgMode || (LOWORD(wParam) & (win32::MK_CONTROL | win32::MK_SHIFT))) return 0;
        i = CWheelNotches(&dWheelMessages, static_cast<std::int16_t>(HIWORD(wParam)));
        while (i > 0) { ControlIO().pfnSendMessage(hwnd, win32::WM_KEYDOWN, win32::VK_UP, 1); i--; }
        while (i < 0) { ControlIO().pfnSendMessage(hwnd, win32::WM_KEYDOWN, win32::VK_DOWN, 1); i++; }
        return 0;
    case win32::WM_KEYDOWN:
        if (wParam == win32::VK_DOWN)
            goto NextMsg;
        if (wParam == win32::VK_UP)
            goto PrevMsg;
        if (app.state.gd.fSendMsgMode)
            goto Default;
        if (wParam == win32::VK_HOME) {
            app.state.iMsgCur = imsgNone;
            goto NextMsg;
        }
        if (wParam != win32::VK_END) {
            return 0;
        }
        app.state.iMsgCur = app.state.cMsg + app.state.vcmsgplrIn;
        goto PrevMsg;
    case win32::WM_CHAR:
        switch (wParam) {
        case '+':
            goto CheckBox;
        case '-':
            for (i = 0; static_cast<std::uint16_t>(i) < 49 && (app.state.bitfMsgSent[i] & app.state.bitfMsgFiltered[i]) == 0; i++) {
            }
            if (i != 49)
                goto ZoomBox;
            break;
        case '\r':
            goto GotoMsg;
        }
        return 0;
    case win32::WM_COMMAND:
        if (HIWORD(wParam) == 0) {
            ControlIO().pfnSetFocus(app.lpui->hwndFrame);
        }
        if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndMsgBtn[0] && HIWORD(wParam) == 0) {
        PrevMsg:
            if (app.state.gd.fSendMsgMode) {
                FFinishPlrMsgEntry(app, -1);
                goto SetupNewMsg;
            }
            if (ControlIO().pfnGetAsyncKeyState(win32::VK_SHIFT) & 0xfffe) {
                app.state.iMsgCur = imsgNone;
                i = IMsgNext(app, 0);
            } else {
                i = IMsgPrev(app, 0);
            }
            if (i != imsgNone) {
                app.state.iMsgCur = i;
                goto SetupNewMsg;
            }
            if (app.state.iMsgCur != app.state.cMsg + app.state.vcmsgplrIn)
                break;
            app.state.iMsgCur--;
        SetupNewMsg:
            app.state.gd.fGotoVCR = 0;
            ControlIO().pfnSetMsgTitle(app, hwnd);
            ControlIO().pfnInvalidateRect(hwnd, &app.lpui->rcMsgText, 1);
            if (app.state.gd.fTutorial) {
                app.state.tutor.fChange = 1;
                PresentationIO().pfnTutor(app);
            }
            break;
        } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndMsgBtn[2] && HIWORD(wParam) == 0) {
        NextMsg:
            if (app.state.gd.fSendMsgMode) {
                FFinishPlrMsgEntry(app, 1);
            } else {
                if (ControlIO().pfnGetAsyncKeyState(win32::VK_SHIFT) & 0xfffe) {
                    app.state.iMsgCur = app.state.cMsg + app.state.vcmsgplrIn;
                    i = IMsgPrev(app, 0);
                } else {
                    i = IMsgNext(app, 0);
                }
                if (i == imsgNone)
                    break;
                app.state.iMsgCur = i;
            }
            goto SetupNewMsg;
        } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndMsgBtn[3] && HIWORD(wParam) == 0) {
            FFinishPlrMsgEntry(app, 1000);
            goto SetupNewMsg;
        } else if (reinterpret_cast<HWND>(lParam) == app.lpui->rghwndMsgBtn[1] && HIWORD(wParam) == 0) {
        GotoMsg:
            if (app.state.gd.fSendMsgMode || app.state.iMsgCur >= app.state.cMsg)
                goto ToggleMsgMode;
            if (app.lpui->mdMsgObj <= mdMsgObjBattleReport) {
                switch (app.lpui->mdMsgObj) {
                default:
                    break;
                case mdMsgObjPlanet:
                    PresentationIO().pfnSelectPlanet(app, 0, app.lpui->idMsgObj);
                    ControlIO().pfnUpdateWindow(app.lpui->hwndScanner);
                    ControlIO().pfnSendMessage(app.lpui->hwndScanner, win32::WM_CHAR, 'v', 0);
                    idm = static_cast<MessageId>(PresentationIO().pfnMessageId(app, app.state.iMsgCur));
                    if (idm != idmHasCompletedOrdersProductionQueueEmpty && idm != idmProductionQueueEmpty &&
                        (idm < idmHasBuiltManyMinesCurrentPopulationCan || idm > idmHasBuiltManyDefensesPlanetCanSupport))
                        break;
                    if (!app.state.gd.fGotoVCR) {
                        app.state.gd.fGotoVCR = 1;
                        ControlIO().pfnSetMsgTitle(app, hwnd);
                        break;
                    }
                    if (app.state.sel.grobj != grobjPlanet || app.state.sel.id != app.lpui->idMsgObj)
                        break;
                    ChangeProduction(app, 0);
                    break;
                case mdMsgObjFleet:
                    PresentationIO().pfnSelectFleet(app, 0, app.lpui->idMsgObj);
                    ControlIO().pfnUpdateWindow(app.lpui->hwndScanner);
                    ControlIO().pfnSendMessage(app.lpui->hwndScanner, win32::WM_CHAR, 'v', 0);
                    break;
                case mdMsgObjThing:
                    lpth = LpthFromId(app, app.lpui->vptMsg.x);
                    if (!lpth)
                        break;
                    scan.pt = lpth->pt();
                    scan.grobj = grobjThing;
                    ChangeScanSel(app, &scan, 0);
                    CtrPointScan(app, scan.pt, 1);
                    break;
                case mdMsgObjBattle:
                    SelectOursAtObject(app, &app.lpui->vptMsg);
                    if (app.state.gd.fGotoVCR) {
                        BattleVCR(app, app.lpui->idMsgObj);
                        break;
                    }
                    app.state.gd.fGotoVCR = 1;
                    ControlIO().pfnSetMsgTitle(app, hwnd);
                    break;
                case mdMsgObjResearch:
                    win32::PostMessageA(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_GAME_RESEARCH, 0);
                    break;
                case mdMsgObjScore:
                    win32::PostMessageA(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_GAME_SCORE, 0);
                    break;
                case mdMsgObjShipDesign:
                    win32::PostMessageA(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_GAME_SHIP_BUILDER, 0);
                    break;
                case mdMsgObjRelations:
                    win32::PostMessageA(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_GAME_RELATIONS2, 0);
                    break;
                case mdMsgObjBattleReport:
                    if (app.lpui->hwndReportDlg && app.lpui->vprptCur == &app.state.vrptBattle)
                        break;
                    win32::PostMessageA(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_REPORT_BATTLE, 0);
                    break;
                case mdMsgObjPart:
                    app.lpui->vpartBrowser.hs.grhst = static_cast<HullSlotType>(1 << (app.lpui->idMsgObj >> 8 & 0xf));
                    app.lpui->vpartBrowser.hs.iItem = app.lpui->idMsgObj & 0xff;
                    PartQueryIO().pfnLookup(app, &app.lpui->vpartBrowser);
                    if (app.lpui->hwndBrowser) {
                        ControlIO().pfnInvalidateRect(app.lpui->hwndBrowserChild, nullptr, 1);
                    } else {
                        app.lpui->fBrowserValid = 1;
                        win32::PostMessageA(app.lpui->hwndFrame, win32::WM_COMMAND, IDM_VIEW_BROWSER_TOGGLE2, 0);
                    }
                }
            }
            if (!app.state.gd.fTutorial)
                break;
            app.state.tutor.fChange = 1;
            PresentationIO().pfnTutor(app);
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

void DecorateMsgTitleBar(APPSESSION &app, HDC hdc, RECT *prc) {
    std::int16_t   xDst;
    std::int16_t   ySrcMask;
    HBRUSH    hbrSav;
    HBITMAP   hbmpSav;
    std::int16_t   dySrc;
    std::int16_t   i;
    HDC       hdcMem;
    MessageId idm;
    std::int16_t   ySrc;
    std::int16_t   yDst;
    std::int16_t   dxSrc;
    std::int16_t   xyStart;
    COLORREF  crBkSav;
    COLORREF  crTextSav;

    hdcMem = win32::CreateCompatibleDC(hdc);
    hbmpSav = static_cast<HBITMAP>(win32::SelectObject(hdcMem, app.lpui->hbmpMono));
    if (app.state.gd.fSendMsgMode)
        goto Cleanup;
    xyStart = static_cast<std::int16_t>((prc->bottom - prc->top - 11)) / 2 + prc->top;
    if (app.state.iMsgCur < 0 || app.state.iMsgCur >= app.state.cMsg)
        goto DoMinMax;
    idm = static_cast<MessageId>(PresentationIO().pfnMessageId(app, app.state.iMsgCur));
    if (app.state.bitfMsgFiltered[idm >> 3] & 1 << (idm & 7)) {
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
    win32::BitBlt(hdc, xDst, yDst, dxSrc, dySrc, hdcMem, 0, ySrcMask, win32::SRCAND);
    hbmpSav = static_cast<HBITMAP>(win32::SelectObject(hdcMem, app.lpui->hbmpMsg));
    win32::BitBlt(hdc, xDst, yDst, dxSrc, dySrc, hdcMem, 0, ySrc, win32::SRCPAINT);
DoMinMax:
    for (i = 0; static_cast<std::uint16_t>(i) < 49 && (app.state.bitfMsgSent[i] & app.state.bitfMsgFiltered[i]) == 0; i++) {
    }
    if (i == 49) {
        app.state.fViewFilteredMsg = 0;
        goto Cleanup;
    }
    if (app.state.fViewFilteredMsg) {
        ySrc = 26;
        ySrcMask = 54;
    } else {
        ySrc = 41;
        ySrcMask = 69;
    }
    xDst = prc->right - (prc->bottom - prc->top) - 1;
    hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrButtonShadow));
    win32::PatBlt(hdc, xDst, prc->top, 1, prc->bottom - prc->top, win32::PATCOPY);
    win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
    win32::PatBlt(hdc, xDst + 1, prc->top, 1, prc->bottom - prc->top, win32::PATCOPY);
    win32::SelectObject(hdc, hbrSav);
    yDst = static_cast<std::int16_t>((prc->bottom - prc->top - 15)) / 2 + prc->top;
    xDst = prc->right - (prc->bottom - yDst);
    win32::SelectObject(hdcMem, app.lpui->hbmpMono);
    win32::BitBlt(hdc, xDst, yDst, 15, 15, hdcMem, 0, ySrcMask, win32::SRCAND);
    win32::SelectObject(hdcMem, app.lpui->hbmpMsg);
    win32::BitBlt(hdc, xDst, yDst, 15, 15, hdcMem, 0, ySrc, win32::SRCPAINT);
Cleanup:
    win32::SetTextColor(hdc, crTextSav);
    win32::SetBkColor(hdc, crBkSav);
    if (!app.state.game.fSinglePlr) {
        win32::SelectObject(hdcMem, app.lpui->hbmpMsg);
        xDst = prc->right - 45;
        yDst = static_cast<std::int16_t>((prc->bottom - prc->top - 7)) / 2 + prc->top;
        win32::PatBlt(hdc, xDst - 1, yDst - 1, 17, 11, win32::BLACKNESS);
        win32::BitBlt(hdc, xDst, yDst, 15, 9, hdcMem, 0, 56, win32::SRCCOPY);
    }
    win32::SelectObject(hdcMem, hbmpSav);
    win32::DeleteDC(hdcMem);
    return;
}

HtMsgType HtMsgBox(APPSESSION &app, POINT16 pt) {
    std::int16_t i;

    if (win32::PtInRect(&app.lpui->rcMsgTitle, PointFrom16(pt)) != 0) {
        if (pt.x < app.lpui->rcMsgTitle.bottom - app.lpui->rcMsgTitle.top + app.lpui->rcMsgTitle.left && app.state.iMsgCur >= 0 && app.state.iMsgCur < app.state.cMsg && !app.state.gd.fSendMsgMode) {
            return htMsgCurrent;
        }
        if (pt.x >= app.lpui->rcMsgTitle.right - (app.lpui->rcMsgTitle.bottom - app.lpui->rcMsgTitle.top) && !app.state.gd.fSendMsgMode) {
            for (i = 0; static_cast<std::uint16_t>(i) < 49 && (app.state.bitfMsgSent[i] & app.state.bitfMsgFiltered[i]) == 0; i++) {
            }
            if (i != 49) {
                return htMsgZoom;
            }
        } else if (!app.state.game.fSinglePlr && pt.x >= app.lpui->rcMsgTitle.right - (app.lpui->rcMsgTitle.bottom - app.lpui->rcMsgTitle.top) - 24) {
            return htMsgMode;
        }
    }
    return htMsgNone;
}

std::int16_t FFinishPlrMsgEntry(APPSESSION &app, std::int16_t dInc) {
    std::uint8_t *lpbMsg;
    std::int16_t  i;
    std::int16_t  cbNew;
    std::int16_t  iPlrTo;
    MSGPLR  *lpmpCur;
    MSGPLR **lpmpPrev;
    std::int16_t  cb;

    lpmpPrev = &app.state.vlpmsgplrOut;
    i = app.state.iMsgSendCur;
    while (i-- > 0) {
        lpmpPrev = &(*lpmpPrev)->lpmsgplrNext;
    }
    lpmpCur = *lpmpPrev;
    if (dInc == 1000) {
        cb = 0;
        dInc = 0;
    } else {
        cb = ControlIO().pfnGetWindowText(app.lpui->hwndMsgEdit, reinterpret_cast<LPSTR>(app.lpui->lpb2k), 1000);
    }
    if (cb == 0) {
        if (lpmpCur) {
            RecordIO().pfnDirtyGame(app, 1);
            *lpmpPrev = lpmpCur->lpmsgplrNext;
            HeapIO().pfnFree(app, lpmpCur, htPlrMsg);
            app.state.vcmsgplrOut--;
            if (app.state.iMsgSendCur > 0) {
                app.state.iMsgSendCur--;
            }
        }
        if (dInc == -1 && app.state.iMsgSendCur > 0) {
            app.state.iMsgSendCur--;
        } else if (dInc == 1 && app.state.iMsgSendCur < app.state.vcmsgplrOut) {
            app.state.iMsgSendCur++;
        }
        return 0;
    }
    cbNew = cb;
    if (FCompressUserString(app, reinterpret_cast<char *>(app.lpui->lpb2k), reinterpret_cast<char *>(app.lpui->lpb2k) + 1024, &cbNew)) {
        cb = cbNew;
        lpbMsg = app.lpui->lpb2k + 1024;
    } else {
        cb = -cb - 1;
        lpbMsg = app.lpui->lpb2k;
    }
    cbNew = std::abs(cb) + 12;
    iPlrTo = LOWORD(ControlIO().pfnSendMessage(app.lpui->hwndMsgDrop, win32::CB_GETCURSEL, 0, 0));
    if (lpmpCur) {
        if (cb != lpmpCur->cLen || lpmpCur->iPlrTo != iPlrTo || std::memcmp(lpmpCur->rgbMsg(), lpbMsg, std::abs(cb)) != 0) {
            RecordIO().pfnDirtyGame(app, 1);
        }
        lpmpCur = LpmsgplrReAlloc(app, lpmpCur, cbNew + (sizeof(MSGPLR) - 12));
    } else {
        RecordIO().pfnDirtyGame(app, 1);
        lpmpCur = LpmsgplrAlloc(app, cbNew + (sizeof(MSGPLR) - 12));
        lpmpCur->lpmsgplrNext = nullptr;
        app.state.vcmsgplrOut++;
        lpmpCur->iInRe = app.state.iMsgCur;
    }
    *lpmpPrev = lpmpCur;
    lpmpCur->iPlrFrom = app.state.idPlayer;
    lpmpCur->iPlrTo = iPlrTo;
    lpmpCur->cLen = cb;
    std::memmove(lpmpCur->rgbMsg(), lpbMsg, std::abs(cb));
    app.state.iMsgSendCur += dInc;
    if (app.state.iMsgSendCur < 0) {
        app.state.iMsgSendCur = 0;
    }
    return 1;
}

void SetMsgTitle(APPSESSION &app, HWND hwnd) {
    std::int16_t cMsgTot;
    std::int16_t i;
    MSGBIG  mb;
    char    ch;
    char    szT[80];
    std::int16_t sw;
    MSGPLR *lpmp;
    RECT    rc;

    if (hwnd && !app.state.fAi) {
        if (app.state.game.fDirty) {
            ch = '*';
        } else {
            ch = ' ';
        }
        sw = !app.state.gd.fSendMsgMode ? 0 : 5;
        ui::ShowWindow(app.lpui->hwndMsgEdit, sw);
        ui::ShowWindow(app.lpui->hwndMsgDrop, sw);
        ui::ShowWindow(app.lpui->rghwndMsgBtn[3], sw);
        if (app.state.gd.fSendMsgMode) {
            ui::ShowWindow(app.lpui->hwndMsgScroll, win32::SW_HIDE);
        }
        cMsgTot = app.state.cMsg + app.state.vcmsgplrIn;
        i = app.state.gd.fSendMsgMode ? idsDone : app.state.iMsgCur >= app.state.cMsg ? idsReply : app.state.gd.fGotoVCR ? idsView : idsGoto3;
        ControlIO().pfnSetWindowText(app.lpui->rghwndMsgBtn[1], TextIO().pfnCompressed(app, static_cast<StringId>(i)));
        if (app.state.gd.fSendMsgMode) {
            win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsSendMessagesDD), app.state.iMsgSendCur + 1, app.state.vcmsgplrOut);
            rc = app.lpui->rcMsgText;
            ExpandRc(&rc, -4, -4);
            ControlIO().pfnSetWindowPos(app.lpui->hwndMsgDrop, nullptr, rc.left + 30, rc.top, rc.right - rc.left - 84, rc.bottom - rc.top, win32::SWP_NOZORDER | win32::SWP_SHOWWINDOW);
            ControlIO().pfnSetWindowPos(app.lpui->rghwndMsgBtn[3], nullptr, rc.right - 50, rc.top, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_SHOWWINDOW);
            rc.top += app.lpui->dyShipDD + 3;
            ControlIO().pfnSetWindowPos(app.lpui->hwndMsgEdit, nullptr, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, win32::SWP_NOZORDER | win32::SWP_SHOWWINDOW);
            ControlIO().pfnEnableWindow(app.lpui->rghwndMsgBtn[0], app.state.iMsgSendCur > 0);
            ControlIO().pfnEnableWindow(app.lpui->rghwndMsgBtn[1], 1);
            ControlIO().pfnEnableWindow(app.lpui->rghwndMsgBtn[2], 1);
            lpmp = app.state.vlpmsgplrOut;
            i = app.state.iMsgSendCur;
            while (i-- > 0) {
                lpmp = lpmp->lpmsgplrNext;
            }
            if (lpmp) {
                ControlIO().pfnSendMessage(app.lpui->hwndMsgDrop, win32::CB_SETCURSEL, lpmp->iPlrTo, 0);
                if (lpmp->cLen < 0) {
                    ControlIO().pfnSetWindowText(app.lpui->hwndMsgEdit, reinterpret_cast<LPCSTR>(lpmp->rgbMsg()));
                    goto FinishUp;
                } else {
                    i = 1000;
                    FDecompressUserString(app, reinterpret_cast<char *>(lpmp->rgbMsg()), lpmp->cLen, reinterpret_cast<char *>(app.lpui->lpb2k), &i);
                    ControlIO().pfnSetWindowText(app.lpui->hwndMsgEdit, reinterpret_cast<LPCSTR>(app.lpui->lpb2k));
                }
                goto FinishUp;
            } else {
                ControlIO().pfnSendMessage(app.lpui->hwndMsgDrop, win32::CB_SETCURSEL, app.lpui->viInRe, 0);
                ControlIO().pfnSetWindowText(app.lpui->hwndMsgEdit, "");
                goto FinishUp;
            }
        }
        if (cMsgTot != 0) {
            TextIO().pfnCchGetString(app, idsYearDCMessagesDD, szT);
            win32::wsprintfA(app.state.szWork, szT, app.state.game.turn + 2400, ch, app.state.iMsgCur + 1, cMsgTot);
        } else {
            TextIO().pfnCchGetString(app, idsYearDCMessagesNone, szT);
            win32::wsprintfA(app.state.szWork, szT, app.state.game.turn + 2400, ch);
        }
        ControlIO().pfnEnableWindow(app.lpui->rghwndMsgBtn[0], IMsgPrev(app, 0) != imsgNone);
        ControlIO().pfnEnableWindow(app.lpui->rghwndMsgBtn[2], IMsgNext(app, 0) != imsgNone);
        if (app.state.iMsgCur >= app.state.cMsg) {
            ControlIO().pfnEnableWindow(app.lpui->rghwndMsgBtn[1], 1);
            goto FinishUp;
        }
        if (app.state.cMsg != 0 && app.state.iMsgCur < app.state.cMsg && FGetNMsgbig(app, static_cast<MessageId>(app.state.iMsgCur), &mb)) {
            if (mb.wGoto != gotoNone) {
                app.lpui->idMsgObj = mb.wGoto & 0x7fff;
                switch (mb.wGoto) {
                case gotoResearch:
                    app.lpui->mdMsgObj = mdMsgObjResearch;
                    break;
                case gotoShipDesign:
                    app.lpui->mdMsgObj = mdMsgObjShipDesign;
                    break;
                case gotoScore:
                    app.lpui->mdMsgObj = mdMsgObjScore;
                    break;
                case gotoThing:
                    app.lpui->mdMsgObj = mdMsgObjThing;
                    app.lpui->vptMsg.x = mb.rgParam[0];
                    break;
                case gotoBattleReport:
                    app.lpui->mdMsgObj = mdMsgObjBattleReport;
                    break;
                default:
                    if ((mb.wGoto & 0xc000) == 0xc000) {
                        app.lpui->mdMsgObj = mdMsgObjPart;
                    } else if (mb.wGoto & 0x4000) {
                        app.lpui->idMsgObj = mb.wGoto & 0x3fff;
                        if (app.lpui->idMsgObj == 2048) {
                            app.lpui->mdMsgObj = mdMsgObjRelations;
                        } else {
                            app.lpui->mdMsgObj = mdMsgObjBattle;
                            app.lpui->vptMsg.x = mb.rgParam[0];
                            app.lpui->vptMsg.y = mb.rgParam[1];
                        }
                    } else if (mb.wGoto < 0) {
                        if (LpflFromId(app, app.lpui->idMsgObj) != 0) {
                            app.lpui->mdMsgObj = mdMsgObjFleet;
                        } else {
                            app.lpui->mdMsgObj = mdMsgObjNone;
                        }
                    } else {
                        app.lpui->mdMsgObj = mdMsgObjPlanet;
                    }
                }
            } else {
                app.lpui->mdMsgObj = mdMsgObjNone;
            }
        }
        if (app.lpui->mdMsgObj != mdMsgObjNone &&
            (app.state.iMsgCur < 0 || (!app.state.fViewFilteredMsg && (app.state.bitfMsgFiltered[PresentationIO().pfnMessageId(app, app.state.iMsgCur) >> 3] & 1 << (PresentationIO().pfnMessageId(app, app.state.iMsgCur) & 7))))) {
            app.lpui->mdMsgObj = mdMsgObjNone;
        }
        ControlIO().pfnEnableWindow(app.lpui->rghwndMsgBtn[1], app.lpui->mdMsgObj != mdMsgObjNone);
    FinishUp:
        std::strcpy(app.lpui->szMsgTitle, app.state.szWork);
        ControlIO().pfnInvalidateRect(app.lpui->hwndMessage, &app.lpui->rcMsgTitle, 1);
    }
    return;
}

// UpdateMsgTitle refreshes the message window's title bar, as when the
// game becomes dirty.
void UpdateMsgTitle(APPSESSION &app) {
    ControlIO().pfnSetMsgTitle(app, app.lpui->hwndMessage);
    return;
}

void ResetMessageInputSession() noexcept { dWheelMessages = 0; }
