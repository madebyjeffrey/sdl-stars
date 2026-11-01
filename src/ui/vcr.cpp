#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"
#include "heap_io.h"
#include "part_query_io.h"
#include "alert_io.h"

void BattleVCR(APPSESSION &app, std::int16_t iBattle) {
    FARPROC  lpProc;
    HB      *lphb;
    std::uint8_t  cTokAlloc;

    lpProc = nullptr;
    app.lpui->viStepVCRCur = -1;
    app.state.gd.fVCRTimer = 0;
    if (app.state.gd.mdScreenSize >= 2) {
        app.lpui->dxyVCRBoard = 673;
        app.lpui->dxyVCRSquare = 64;
    } else {
        app.lpui->dxyVCRBoard = 353;
        app.lpui->dxyVCRSquare = 32;
    }
    lphb = app.state.rglphb[11];
    app.lpui->vlpbdVCR = reinterpret_cast<std::uint8_t *>(lphb) + (sizeof(HB) + 2);
    while (BdvFromLp(app, app.lpui->vlpbdVCR).id() != iBattle) {
        if (BdvFromLp(app, app.lpui->vlpbdVCR).id() == 0xffff) {
            lphb = lphb->lphbNext;
            if (!lphb) {
                return;
            }
            app.lpui->vlpbdVCR = reinterpret_cast<std::uint8_t *>(lphb) + (sizeof(HB) + 2);
        } else {
            app.lpui->vlpbdVCR = static_cast<std::uint8_t *>(app.lpui->vlpbdVCR) + BdvFromLp(app, app.lpui->vlpbdVCR).cbData();
        }
    }
    app.lpui->vlpbdVCRNext = static_cast<std::uint8_t *>(app.lpui->vlpbdVCR) + BdvFromLp(app, app.lpui->vlpbdVCR).cbData();
    StarsRecoveryScope scopeRecovery;
    try {
        cTokAlloc = BdvFromLp(app, app.lpui->vlpbdVCR).ctok();
        app.state.vrgtok = LpAlloc<TOK>(app, cTokAlloc * sizeof(TOK), htMisc);
        cTokAlloc = BdvFromLp(app, app.lpui->vlpbdVCR).ctok();
        app.lpui->vrgdpVCR = LpAlloc<std::int32_t>(app, cTokAlloc * 4, htMisc);
        app.lpui->vcStepVCR = SetVCRBoard(app, 30000) - 1;
        app.lpui->vcRound = app.lpui->viRound;
        SetVCRBoard(app, -1);
        if (app.state.gd.fTutorial) {
            PresentationIO().pfnTutor(app);
        }
        lpProc = (VCRDlg);
        ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_VCR), app.lpui->hwndFrame, lpProc, 0);

    } catch (const StarsError &) {
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsMemory, nullptr), win32::MB_ICONHAND);
    }
LCleanup:
    if (lpProc) {
    }
    if (app.state.vrgtok) {
        HeapIO().pfnFree(app, app.state.vrgtok, htMisc);
    }
    if (app.lpui->vrgdpVCR) {
        HeapIO().pfnFree(app, app.lpui->vrgdpVCR, htMisc);
    }
    app.state.vrgtok = nullptr;
    app.lpui->vrgdpVCR = nullptr;
    return;
}

std::uint8_t *BtlDataGet(APPSESSION &app, std::int16_t i) {
    std::uint8_t *lpbd;
    HB      *lphb;

    lphb = app.state.rglphb[11];
    if (!lphb) {
        return nullptr;
    }
    lpbd = reinterpret_cast<std::uint8_t *>(lphb) + (sizeof(HB) + 2);
    while (1) {
        if (BdvFromLp(app, lpbd).id() == 0xffff) {
            lphb = lphb->lphbNext;
            if (!lphb || lphb->ibTop <= sizeof(HB))
                break;
            lpbd = reinterpret_cast<std::uint8_t *>(lphb) + (sizeof(HB) + 2);
        } else {
            if (BdvFromLp(app, lpbd).cbData() == 0) {
                return nullptr;
            }
            if (i-- <= 0) {
                return lpbd;
            }
            lpbd = static_cast<std::uint8_t *>(lpbd) + BdvFromLp(app, lpbd).cbData();
        }
    }
    return nullptr;
}

std::int32_t CBattleUnits(APPSESSION &app, std::uint8_t *lpbd, BattleUnitFlags grbitBU) {
    std::uint8_t     *lptok;
    std::int16_t      ctok;
    std::int32_t      lUnits;
    std::int16_t      i;
    HullCategory imd;

    ctok = BdvFromLp(app, lpbd).ctok();
    lUnits = 0;
    for (i = 0; i < ctok; i++) {
        lptok = BdvFromLp(app, lpbd).rgtok(i).Lpb();
        if (BtvFromLp(app, lptok).iplr() == app.state.idPlayer) {
            if (!(grbitBU & grBuOurUnits))
                continue;
        } else if (!(grbitBU & grBuTheirUnits)) {
            continue;
        }
        if (!(grbitBU & grBuIncludeSb) && BtvFromLp(app, lptok).ishdef() >= 16)
            continue;
        if ((grbitBU & grBuClassAll) != 0xf8 && BtvFromLp(app, lptok).ishdef() < 16) {
            imd = static_cast<HullCategory>(PartQueryIO().pfnHull(app.state.rglpshdef[BtvFromLp(app, lptok).iplr()][BtvFromLp(app, lptok).ishdef()].hul.ihuldef)->imdCategory);
            if (static_cast<std::int16_t>(imd) <= hullCatFreighter || static_cast<std::int16_t>(imd) >= hullCatMiner) {
                if (!(grbitBU & grBuClassUnarmed))
                    continue;
            } else if (imd == hullCatScout) {
                if (!(grbitBU & grBuClassScout))
                    continue;
            } else if (imd == hullCatWarship) {
                if (!(grbitBU & grBuClassWarship))
                    continue;
            } else if (imd == hullCatBomber) {
                if (!(grbitBU & grBuClassBomber))
                    continue;
            } else if (imd == hullCatUtility) {
                if (!(grbitBU & grBuClassUtility))
                    continue;
            }
        }
        lUnits += static_cast<std::uint32_t>(BtvFromLp(app, lptok).csh());
    }
    return lUnits;
}

std::int32_t CBattleKills(APPSESSION &app, std::uint8_t *lpbd, std::int16_t fOurDead) {
    std::int32_t  cKilled;
    std::uint8_t *lpbdNext;
    std::int16_t  i;
    std::uint8_t  *lpbr;
    std::int16_t  cKill;

    lpbr = BdvFromLp(app, lpbd).FirstEvent();
    lpbdNext = static_cast<std::uint8_t *>(lpbd) + BdvFromLp(app, lpbd).cbData();
    cKilled = 0;
    for (; lpbr < lpbdNext; lpbr = BevFromLp(app, lpbr).Next()) {
        cKill = BevFromLp(app, lpbr).ctok();
        for (i = 0; i < cKill; i++) {
            if (BevFromLp(app, lpbr).rgkill(i).cshKill() > 0) {
                if (BdvFromLp(app, lpbd).rgtok(BevFromLp(app, lpbr).rgkill(i).itok()).iplr() == app.state.idPlayer) {
                    if (fOurDead) {
                        cKilled += static_cast<std::uint32_t>(BevFromLp(app, lpbr).rgkill(i).cshKill());
                    }
                } else if (!fOurDead) {
                    cKilled += static_cast<std::uint32_t>(BevFromLp(app, lpbr).rgkill(i).cshKill());
                }
            }
        }
    }
    return cKilled;
}

std::int32_t LdpFromItokDv(APPSESSION &app, std::int16_t itok, DV *lpdv) {
    DV       dv;
    std::uint16_t dpShdef;
    std::int16_t  csh;
    std::int32_t  dp;

    if (lpdv) {
        dv.dp = lpdv->dp;
    } else {
        dv.dp = 0;
    }
    dpShdef = LpshdefFromTok(app, app.state.vrgtok + itok)->hul.dp;
    dp = static_cast<std::uint32_t>((static_cast<std::uint32_t>(dpShdef) * static_cast<std::uint32_t>(app.state.vrgtok[itok].csh)));
    if (dv.dp != 0) {
        csh = LOWORD(static_cast<std::int32_t>((static_cast<std::uint32_t>(app.state.vrgtok[itok].csh) * dv.pctSh())) / 100);
        if (csh <= 0) {
            csh = 1;
        }
        dp -= static_cast<std::int32_t>((static_cast<std::int32_t>((static_cast<std::uint32_t>(dpShdef) * dv.pctDp())) / 10 * csh)) / 50;
    }
    return dp;
}

static std::int32_t LdpFromItokDv(APPSESSION &app, std::int16_t itok, DV dv) {
    return LdpFromItokDv(app, itok, &dv);
}

std::int16_t SetVCRBoard(APPSESSION &app, std::int16_t iStep) {
    TOK    *ptok;
    std::int16_t i;
    std::int16_t itok;

    if (iStep == app.lpui->viStepVCRCur && app.lpui->viStepVCRCur != -1) {
        return iStep;
    }
    if (app.lpui->viStepVCRCur > iStep || app.lpui->viStepVCRCur == -1) {
        for (i = 0; i < BdvFromLp(app, app.lpui->vlpbdVCR).ctok(); i++) {
            app.state.vrgtok[i] = BdvFromLp(app, app.lpui->vlpbdVCR).rgtok(i).ReadValue();
            app.lpui->vrgdpVCR[i] = LdpFromItokDv(app, i, &app.state.vrgtok[i].dv);
        }
        app.lpui->viStepVCRCur = -1;
        app.lpui->viRound = 0;
        app.lpui->viVCRFocus = 0;
        app.lpui->vbrcVCRFocus = app.state.vrgtok->brc;
        app.lpui->vlpbrVCR = BdvFromLp(app, app.lpui->vlpbdVCR).FirstEvent();
    }
    for (; app.lpui->viStepVCRCur < iStep && app.lpui->vlpbrVCR < app.lpui->vlpbdVCRNext; app.lpui->viStepVCRCur++) {
        if (app.lpui->viStepVCRCur >= 0) {
            for (i = 0; i < BevFromLp(app, app.lpui->vlpbrVCR).ctok(); i++) {
                itok = BevFromLp(app, app.lpui->vlpbrVCR).rgkill(i).itok();
                ptok = app.state.vrgtok + itok;
                ptok->csh -= BevFromLp(app, app.lpui->vlpbrVCR).rgkill(i).cshKill();
                ptok->dv.dp = BevFromLp(app, app.lpui->vlpbrVCR).rgkill(i).dv().dp();
                app.lpui->vrgdpVCR[itok] = LdpFromItokDv(app, itok, BevFromLp(app, app.lpui->vlpbrVCR).rgkill(i).dv().ReadValue());
                if (ptok->dpShield != 0 && BevFromLp(app, app.lpui->vlpbrVCR).rgkill(i).dpShield() != 0) {
                    if (static_cast<std::int32_t>(static_cast<std::uint32_t>((static_cast<std::uint32_t>(ptok->dpShield) * static_cast<std::uint32_t>(ptok->csh)))) >
                        static_cast<std::int32_t>((static_cast<std::uint32_t>((BevFromLp(app, app.lpui->vlpbrVCR).rgkill(i).dpShield() & 0x1fff)) << (BevFromLp(app, app.lpui->vlpbrVCR).rgkill(i).dpShield() >> 0xd << 1)))) {
                        ptok->dpShield -= LOWORD(static_cast<std::int32_t>((static_cast<std::uint32_t>((BevFromLp(app, app.lpui->vlpbrVCR).rgkill(i).dpShield() & 0x1fff)) << (BevFromLp(app, app.lpui->vlpbrVCR).rgkill(i).dpShield() >> 0xd << 1))) /
                                                 static_cast<std::int32_t>(ptok->csh));
                    } else {
                        ptok->dpShield = 0;
                    }
                }
            }
            app.lpui->vlpbrVCR = BevFromLp(app, app.lpui->vlpbrVCR).Next();
            if (BevFromLp(app, app.lpui->vlpbrVCR).iRound() > static_cast<std::uint16_t>(app.lpui->viRound)) {
                app.lpui->viRound = BevFromLp(app, app.lpui->vlpbrVCR).iRound();
                ptok = app.state.vrgtok;
                i = 0;
                while (i < BdvFromLp(app, app.lpui->vlpbdVCR).ctok()) {
                    if (ptok->fRegen) {
                        RegenShield(app, ptok);
                    }
                    ptok->fMoved = 0;
                    i++;
                    ptok++;
                }
            }
        }
        if (app.lpui->vlpbrVCR < app.lpui->vlpbdVCRNext) {
            app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itok()].brc = BevFromLp(app, app.lpui->vlpbrVCR).brcDest();
            app.lpui->vbrcVCRFocus = app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itok()].brc;
            app.lpui->viVCRFocus = BevFromLp(app, app.lpui->vlpbrVCR).itok();
            app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itok()].dzDis = BevFromLp(app, app.lpui->vlpbrVCR).dzDis();
            if (app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itok()].dzDis == 4) {
                app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itok()].mdTactic = mdTacticDisengage;
            }
        }
    }
    EnableVCRButtons(app);
    return app.lpui->viStepVCRCur;
}

INT_PTR  VCRDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HDC         hdc;
    std::int16_t     i;
    std::int16_t     ibtn;
    RECT        rc;
    std::int16_t     dyFrame;
    RECT        rcWindow;
    std::int16_t     dx;
    POINT16     pt;
    std::uint8_t     brc;
    std::int16_t     bkMode;
    std::int16_t     bt;
    RECT       *prc;
    std::int16_t     iDir;
    std::int16_t     iCur;
    COLORREF    crBkSav;
    BTNT        btnt;
    std::int16_t     iSel;
    PAINTSTRUCT ps;
    std::int16_t     iStep;
    std::int16_t     dStep;

    switch (message) {
    case win32::WM_INITDIALOG:
        app.lpui->hwndVCRDlg = hwnd;
        win32::GetWindowRect(hwnd, &rcWindow);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        dyFrame = rcWindow.bottom - rcWindow.top - rc.bottom;
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_VCR_REW_ALL), &rc);
        ControlIO().pfnSetWindowPos(hwnd, nullptr, 0, 0, app.lpui->dxyVCRBoard + 250, dyFrame + 24 + app.lpui->dxyVCRBoard + (rc.bottom - rc.top), win32::SWP_NOMOVE | win32::SWP_NOZORDER);
        for (i = 0; i < 7; i++) {
            if (i < 5) {
                ibtn = i + 161;
            } else if (i == 5) {
                ibtn = 1;
            } else if (i == 6) {
                ibtn = 118;
            }
            win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, ibtn), &rc);
            win32::MapWindowPoints(nullptr, hwnd, reinterpret_cast<POINT *>(&rc), 2);
            if (app.lpui->dxyVCRSquare >= 64) {
                dx = rc.right - rc.left;
                dx = app.lpui->dxyVCRBoard / 2 + 8 - static_cast<std::int16_t>((7 * dx)) / 2 - 24 + (dx + 8) * i;
                dx -= rc.left;
            } else {
                dx = 0;
            }
            win32::OffsetRect(&rc, dx, app.lpui->dxyVCRBoard + 16 - rc.top);
            ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, ibtn), nullptr, rc.left, rc.top, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER);
        }
        EnableVCRButtons(app);
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyVCRDlg, 1);
        app.lpui->fAnimate = 1;
        return 1;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (pt.x > 8 && pt.x < (app.lpui->dxyVCRSquare + 3) * 10 + 8 && pt.y >= 8 && pt.y < (app.lpui->dxyVCRSquare + 3) * 10 + 8) {
            win32::SetCursor(app.lpui->hcurHand);
            return 1;
        }
        return 0;
    case win32::WM_LBUTTONDOWN:
    case win32::WM_RBUTTONDOWN:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        if (win32::PtInRect(app.lpui->rgrcBuildSpin, PointFrom16(pt)) != 0 || win32::PtInRect(&app.lpui->rgrcBuildSpin[1], PointFrom16(pt)) != 0) {
            if (win32::PtInRect(app.lpui->rgrcBuildSpin, PointFrom16(pt)) != 0) {
                iDir = -1;
                bt = 34;
                prc = app.lpui->rgrcBuildSpin;
            } else {
                iDir = 1;
                bt = 35;
                prc = &app.lpui->rgrcBuildSpin[1];
            }
            iCur = app.lpui->viSpeedVCR;
            if (iCur > 4) {
                iCur = 4;
            } else if (iCur < 0) {
                iCur = 0;
            }
            hdc = win32::GetDC(hwnd);
            bkMode = win32::SetBkMode(hdc, win32::OPAQUE);
            crBkSav = win32::SetBkColor(hdc, app.lpui->crButtonFace);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            InitBtnTrack(&btnt, hwnd, nullptr, prc, bt, 80, 0, 0, nullptr);
            while (FTrackBtn(app, &btnt)) {
                if ((iDir == -1 && iCur > 0) || (iDir == 1 && iCur < 4)) {
                    iCur += iDir;
                    bt = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsPlaybackSpeedD), iCur + 1);
                    win32::TextOutA(hdc, app.lpui->ptSpeedVCR.x, app.lpui->ptSpeedVCR.y, app.state.szWork, bt);
                }
            }
            app.lpui->viSpeedVCR = iCur;
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            win32::SetBkColor(hdc, crBkSav);
            win32::ReleaseDC(hwnd, hdc);
            if (app.state.gd.fVCRTimer) {
                win32::KillTimer(hwnd, 2668);
                app.state.gd.fVCRTimer = win32::SetTimer(hwnd, 2668, 570 - 120 * app.lpui->viSpeedVCR, nullptr) != 0;
            }
            return 1;
        }
        pt.x = static_cast<std::int16_t>((pt.x - 8)) / (app.lpui->dxyVCRSquare + 3);
        pt.y = static_cast<std::int16_t>((pt.y - 8)) / (app.lpui->dxyVCRSquare + 3);
        if (pt.x >= 10 && pt.y >= 2 && pt.y < 10 && app.lpui->viVCRFocus >= 0) {
            app.lpui->GlobalPD.grPopup = grPopupShdef;
            if (app.state.vrgtok[app.lpui->viVCRFocus].grobj == grobjPlanet) {
                app.lpui->GlobalPD.lpshdef = app.state.rglpshdefSB[app.state.vrgtok[app.lpui->viVCRFocus].iplr] + (app.state.vrgtok[app.lpui->viVCRFocus].ishdef - 16);
            } else {
                app.lpui->GlobalPD.lpshdef = app.state.rglpshdef[app.state.vrgtok[app.lpui->viVCRFocus].iplr] + app.state.vrgtok[app.lpui->viVCRFocus].ishdef;
            }
            app.lpui->GlobalPD.fShowDamage = 1;
            app.lpui->GlobalPD.SetFToken(1);
            app.lpui->GlobalPD.fHideCounts = app.state.vrgtok[app.lpui->viVCRFocus].iplr != app.state.idPlayer;
            Popup(app, hwnd, LOWORD(lParam), HIWORD(lParam));
            return 0;
        }
        if (pt.x < 0 || pt.y < 0 || pt.x >= 10 || pt.y >= 10) {
            return 0;
        }
        brc = (pt.y & 0xf) << 4 | (pt.x & 0xf);
        if (message == win32::WM_RBUTTONDOWN) {
            iSel = PopupVCRMenu(app, hwnd, LOWORD(lParam), HIWORD(lParam), brc);
            if (iSel < 0) {
                return 0;
            }
            app.lpui->vbrcVCRFocus = brc;
            app.lpui->viVCRFocus = iSel;
        } else {
            if (brc != app.lpui->vbrcVCRFocus || app.lpui->viVCRFocus == -1) {
                app.lpui->vbrcVCRFocus = brc;
                app.lpui->viVCRFocus = BdvFromLp(app, app.lpui->vlpbdVCR).ctok() - 1;
            }
            i = app.lpui->viVCRFocus;
            do {
                i++;
                if (i == BdvFromLp(app, app.lpui->vlpbdVCR).ctok()) {
                    i = 0;
                }
                if (app.state.vrgtok[i].brc == brc && app.state.vrgtok[i].csh > 0) {
                    app.lpui->viVCRFocus = i;
                    goto GoodSel;
                }
            } while (i != app.lpui->viVCRFocus);
            app.lpui->viVCRFocus = -1;
        }
    GoodSel:
        DrawVCR(app, nullptr, -2, -1);
        return 0;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        DrawVCR(app, hdc, -1, -1);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_TIMER:
        if (!app.state.gd.fVCRTimer) {
            return 0;
        }
        if (app.lpui->viStepVCRCur != app.lpui->vcStepVCR)
            goto NextBtn;
        i = 2;
        goto KillTime;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) >= IDC_VCR_REW_ALL && LOWORD(wParam) <= IDC_VCR_FWD_ALL) {
            i = LOWORD(wParam) - 161;
            if (app.state.gd.fVCRTimer) {
            KillTime:
                win32::KillTimer(hwnd, 2668);
                app.state.gd.fVCRTimer = 0;
                if (i == 2) {
                    return 0;
                }
            }
            if (ControlIO().pfnGetAsyncKeyState(win32::VK_CONTROL) < 0) {
                dStep = 100;
            } else if (ControlIO().pfnGetAsyncKeyState(win32::VK_SHIFT) < 0) {
                dStep = 10;
            } else {
                dStep = 1;
            }
            app.lpui->fAnimate = 0;
            switch (i) {
            case 0:
                iStep = -1;
                break;
            case 1:
                iStep = app.lpui->viStepVCRCur - dStep;
                if (iStep >= -1)
                    break;
                iStep = -1;
                break;
            case 2:
                app.state.gd.fVCRTimer = win32::SetTimer(hwnd, 2668, 570 - 120 * app.lpui->viSpeedVCR, nullptr) != 0;
                /* fallthrough */
            case 3:
            NextBtn:
                if (i == 3) {
                    iStep = app.lpui->viStepVCRCur + dStep;
                } else {
                    iStep = app.lpui->viStepVCRCur + 1;
                }
                if (iStep > app.lpui->vcStepVCR) {
                    iStep = app.lpui->vcStepVCR;
                }
                app.lpui->fAnimate = app.lpui->viSpeedVCR < 4;
                break;
            case 4:
                iStep = app.lpui->vcStepVCR;
                break;
            }
            SetVCRBoard(app, iStep);
            DrawVCR(app, nullptr, -2, -1);
        } else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            if (app.state.gd.fVCRTimer) {
                app.state.gd.fVCRTimer = 0;
                win32::KillTimer(hwnd, 2668);
            }
            if (app.state.gd.fTutorial) {
                app.state.tutor.fProgress = 1;
            }
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyVCRDlg, 0);
            ControlIO().pfnEndDialog(hwnd, i);
            return 1;
        } else if (LOWORD(wParam) == IDC_HELP) {
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhBattleVCR);
            return 1;
        }
        break;
    case win32::WM_DESTROY:
        app.lpui->hwndVCRDlg = 0;
        break;
    }
    return 0;
}

void GetVCRStats(APPSESSION &app, std::int16_t itok, std::int32_t *pdpArmor, DV *pdv, std::int32_t *pdpShields, std::int16_t *pcsh) {
    std::int16_t  cshT;
    DV       dv;
    std::int32_t  dpShields;
    std::int32_t  dpArmor;
    std::int16_t  i;
    std::int16_t  cshKill;
    std::uint16_t dpShdef;

    dpArmor = 0;
    dpShields = 0;
    cshKill = 0;
    dv.dp = 0xffff;
    dpArmor = app.lpui->vrgdpVCR[itok];
    for (i = 0; i < BevFromLp(app, app.lpui->vlpbrVCR).ctok(); i++) {
        if (BevFromLp(app, app.lpui->vlpbrVCR).rgkill(i).itok() == itok) {
            cshKill += BevFromLp(app, app.lpui->vlpbrVCR).rgkill(i).cshKill();
            dpShields += static_cast<std::int32_t>((static_cast<std::uint32_t>((BevFromLp(app, app.lpui->vlpbrVCR).rgkill(i).dpShield() & 0x1fff)) << (BevFromLp(app, app.lpui->vlpbrVCR).rgkill(i).dpShield() >> 0xd << 1)));
            dv.dp = BevFromLp(app, app.lpui->vlpbrVCR).rgkill(i).dv().dp();
        }
    }
    if (dv.dp != 0xffff) {
        dpShdef = LpshdefFromTok(app, app.state.vrgtok + itok)->hul.dp;
        cshT = app.state.vrgtok[itok].csh - cshKill;
        dpArmor = static_cast<std::uint32_t>((static_cast<std::uint32_t>(dpShdef) * cshT));
        cshT = LOWORD(static_cast<std::int32_t>((cshT * dv.pctSh())) / 100);
        if (cshT <= 0) {
            cshT = 1;
        }
        dpArmor -= static_cast<std::int32_t>((static_cast<std::int32_t>((static_cast<std::uint32_t>(dpShdef) * dv.pctDp())) / 10 * cshT)) / 50;
    } else {
        dv.dp = app.state.vrgtok[itok].dv.dp;
        if (dv.pctDp() > 499) {
            dv.SetPctDp(499);
        }
    }
    cshT = app.state.vrgtok[itok].csh - cshKill;
    if (cshT < 1) {
        cshT = 0;
        dpArmor = 0;
        dpShields = static_cast<std::uint32_t>((static_cast<std::uint32_t>(app.state.vrgtok[itok].dpShield) * static_cast<std::uint32_t>(app.state.vrgtok[itok].csh)));
    }
    if (pdv) {
        pdv->dp = dv.dp;
    }
    if (pcsh) {
        *pcsh = cshT;
    }
    if (pdpArmor) {
        *pdpArmor = dpArmor;
    }
    if (pdpShields) {
        *pdpShields = dpShields;
    }
    return;
}

void DrawVCR(APPSESSION &app, HDC hdc, std::int16_t iStart, std::int16_t iEnd) {
    std::int16_t ctok;
    std::int16_t ibmpRace;
    std::int16_t bkMode;
    HBRUSH  hbrSav;
    std::int32_t dpShields;
    std::int16_t itokT;
    std::int32_t dpT;
    std::int16_t fCreatedDC;
    std::int32_t dpArmor;
    std::int16_t y;
    std::uint8_t rgfSeen[256];
    std::int16_t c;
    std::int16_t i;
    std::uint8_t brcT;
    SHDEF  *lpshdef;
    std::int16_t ibmp;
    std::int16_t csh;
    char   *psz;
    std::int16_t dx;
    std::int16_t j;
    char    szT[96];
    std::int16_t fJam;
    RECT    rc;
    std::int16_t x;
    std::int16_t cshT;
    std::int32_t dpShT;
    DV      dv;
    std::int16_t xT;
    std::int16_t cshNew;

    fCreatedDC = hdc == nullptr;
    if (fCreatedDC) {
        hdc = win32::GetDC(app.lpui->hwndVCRDlg);
    }
    hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrButtonFace));
    bkMode = win32::SetBkMode(hdc, win32::TRANSPARENT);
    std::memset(rgfSeen, 0, 0x100);
    ControlIO().pfnGetClientRect(app.lpui->hwndVCRDlg, &rc);
    if (iStart == -2) {
        win32::PatBlt(hdc, app.lpui->dxyVCRBoard + 10, 0, rc.right - app.lpui->dxyVCRBoard - 10, app.lpui->dxyVCRBoard + 8, win32::PATCOPY);
        iStart = -1;
    }
    win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
    if (iStart == -1) {
        win32::PatBlt(hdc, 8, 8, app.lpui->dxyVCRBoard, 2, win32::PATCOPY);
        win32::PatBlt(hdc, 8, 8, 2, app.lpui->dxyVCRBoard, win32::PATCOPY);
        win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
        win32::PatBlt(hdc, 9, app.lpui->dxyVCRBoard + 6, app.lpui->dxyVCRBoard - 1, 1, win32::PATCOPY);
        win32::PatBlt(hdc, 8, app.lpui->dxyVCRBoard + 7, app.lpui->dxyVCRBoard, 1, win32::PATCOPY);
        win32::PatBlt(hdc, app.lpui->dxyVCRBoard + 6, 9, 1, app.lpui->dxyVCRBoard - 3, win32::PATCOPY);
        win32::PatBlt(hdc, app.lpui->dxyVCRBoard + 7, 8, 1, app.lpui->dxyVCRBoard - 2, win32::PATCOPY);
        win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
        for (i = 1; i < 10; i++) {
            win32::PatBlt(hdc, (app.lpui->dxyVCRSquare + 3) * i + 9, 10, 1, app.lpui->dxyVCRBoard - 4, win32::PATCOPY);
            win32::PatBlt(hdc, 10, (app.lpui->dxyVCRSquare + 3) * i + 9, app.lpui->dxyVCRBoard - 4, 1, win32::PATCOPY);
        }
        x = app.lpui->dxyVCRBoard + 14;
        y = 8;
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsPhaseDDRoundDD), app.lpui->viStepVCRCur + 2, app.lpui->vcStepVCR + 2, app.lpui->viRound + 1, app.lpui->vcRound + 1);
        win32::TextOutA(hdc, x, y, app.state.szWork, c);
        y += app.state.dyArial8 + 4;
        c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsPlaybackSpeedD), app.lpui->viSpeedVCR + 1);
        dx = LOWORD(GetTextExtent(hdc, app.state.szWork, c));
        win32::TextOutA(hdc, x, y, app.state.szWork, c);
        app.lpui->ptSpeedVCR.x = x;
        app.lpui->ptSpeedVCR.y = y;
        win32::SetRect(app.lpui->rgrcBuildSpin, x + dx, y, x + dx + 14, y + 14);
        app.lpui->rgrcBuildSpin[1] = app.lpui->rgrcBuildSpin[0];
        win32::OffsetRect(&app.lpui->rgrcBuildSpin[1], 14, 0);
        for (i = 0; i < 2; i++) {
            DrawBtn(app, hdc, &app.lpui->rgrcBuildSpin[i], (i == 0 ? 2 : 3) | 0x20, 0, nullptr);
        }
        if (app.lpui->viStepVCRCur >= 0) {
            y += app.state.dyArial8 + 4;
            psz = TextIO().pfnPlayerName(app, app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itok()].iplr, 1, 1, 1, 0, nullptr);
            win32::TextOutA(hdc, x, y, app.state.szWork, std::strlen(psz));
            y += app.state.dyArial8;
            if (BevFromLp(app, app.lpui->vlpbrVCR).itok() == app.lpui->viVCRFocus) {
                win32::SetTextColor(hdc, 8323072);
            }
            if (app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itok()].grobj == grobjPlanet) {
                lpshdef = app.state.rglpshdefSB[app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itok()].iplr] + (app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itok()].ishdef - 16);
            } else {
                lpshdef = app.state.rglpshdef[app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itok()].iplr] + app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itok()].ishdef;
            }
            csh = app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itok()].csh;
            if (csh > 1) {
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsSD), lpshdef->hul.szClass, csh);
            } else {
                std::strcpy(app.state.szWork, lpshdef->hul.szClass);
                c = std::strlen(app.state.szWork);
            }
            win32::TextOutA(hdc, x, y, app.state.szWork, c);
            y += app.state.dyArial8;
            win32::SetTextColor(hdc, app.lpui->crButtonText);
            fJam = 0;
            if (BevFromLp(app, app.lpui->vlpbrVCR).ctok() > 0) {
                psz = TextIO().pfnPlayerName(app, app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itokAttack()].iplr, 0, 1, 1, 0, nullptr);
                c = win32::wsprintfA(szT, TextIO().pfnCompressed(app, idsAttacksS), psz);
                win32::TextOutA(hdc, x, y, szT, c);
                y += app.state.dyArial8;
                if (BevFromLp(app, app.lpui->vlpbrVCR).rgkill(0).dv().dp() != 0) {
                    win32::SetTextColor(hdc, 127);
                }
                if (app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itokAttack()].grobj == grobjPlanet) {
                    lpshdef = app.state.rglpshdefSB[app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itokAttack()].iplr] + (app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itokAttack()].ishdef - 16);
                } else {
                    lpshdef = app.state.rglpshdef[app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itokAttack()].iplr] + app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itokAttack()].ishdef;
                }
                csh = app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itokAttack()].csh;
                if (csh > 1) {
                    c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsSD), lpshdef->hul.szClass, csh);
                } else {
                    std::strcpy(app.state.szWork, lpshdef->hul.szClass);
                    c = std::strlen(app.state.szWork);
                }
                win32::TextOutA(hdc, x, y, app.state.szWork, c);
                y += app.state.dyArial8;
                win32::SetTextColor(hdc, app.lpui->crButtonText);
                brcT = app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itokAttack()].brc;
                dpArmor = 0;
                dpShields = 0;
                j = 0;
                for (i = BevFromLp(app, app.lpui->vlpbrVCR).ctok(); i > 0; i--) {
                    itokT = BevFromLp(app, app.lpui->vlpbrVCR).rgkill(i - 1).itok();
                    if (itokT == BevFromLp(app, app.lpui->vlpbrVCR).itokAttack() && !fJam) {
                        fJam |= BevFromLp(app, app.lpui->vlpbrVCR).rgkill(i - 1).grfWeapon() & (bitFNoHit | bitFDeflected);
                    }
                    j += BevFromLp(app, app.lpui->vlpbrVCR).rgkill(i - 1).cshKill();
                    if (!rgfSeen[itokT]) {
                        rgfSeen[itokT] = 1;
                        GetVCRStats(app, itokT, &dpT, nullptr, &dpShT, &cshT);
                        dpArmor += app.lpui->vrgdpVCR[itokT] - dpT;
                        dpShields += dpShT;
                    }
                }
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDDDoing), brcT & 0xf, brcT >> 4);
                win32::TextOutA(hdc, x, y, app.state.szWork, c);
                y += app.state.dyArial8;
                if (dpShields != 0) {
                    TextIO().pfnCchGetString(app, idsAnd, szT);
                    c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsLdDamageShieldsS), dpShields, dpArmor > 0 ? szT : j > 0 ? "," : ".");
                    win32::TextOutA(hdc, x, y, app.state.szWork, c);
                    y += app.state.dyArial8;
                }
                if (dpArmor != 0) {
                    c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsLdDamageArmorC), dpArmor, j <= 0 ? 46 : 44);
                    win32::TextOutA(hdc, x, y, app.state.szWork, c);
                    y += app.state.dyArial8;
                }
                if ((fJam & 0x40) && dpShields == 0 && dpArmor == 0) {
                    psz = TextIO().pfnCompressed(app, idsDamage3);
                    win32::TextOutA(hdc, x, y, psz, std::strlen(psz));
                    y += app.state.dyArial8;
                }
                if (j > 0) {
                    c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDestroyingDShip), j);
                    if (j == 1) {
                        std::strcpy(&app.state.szWork[c], ".");
                        c++;
                    } else {
                        std::strcpy(&app.state.szWork[c], "s.");
                        c += 2;
                    }
                    win32::TextOutA(hdc, x, y, app.state.szWork, c);
                    y += app.state.dyArial8;
                }
            }
            if (fJam) {
                win32::SetTextColor(hdc, 127);
                c = TextIO().pfnCchGetString(app, (fJam & 0x40) && dpArmor == 0 ? idsTorpedoesDeflected2 : idsTorpedoesDeflected, app.state.szWork);
                win32::TextOutA(hdc, x, y, app.state.szWork, c);
                y += app.state.dyArial8;
                win32::SetTextColor(hdc, app.lpui->crButtonText);
            }
        }
        if (app.lpui->vbrcVCRFocus != 0xff) {
            y = 200;
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsSelectionDD), app.lpui->vbrcVCRFocus & 0xf, app.lpui->vbrcVCRFocus >> 4);
            win32::TextOutA(hdc, x, y, app.state.szWork, c);
            y += app.state.dyArial8;
            if (app.lpui->viVCRFocus >= 0) {
                csh = 0;
                psz = TextIO().pfnPlayerName(app, app.state.vrgtok[app.lpui->viVCRFocus].iplr, 1, 1, 1, 0, nullptr);
                c = std::strlen(psz);
                win32::TextOutA(hdc, x, y, app.state.szWork, c);
                y += app.state.dyArial8;
                if (app.state.vrgtok[app.lpui->viVCRFocus].grobj == grobjPlanet) {
                    lpshdef = app.state.rglpshdefSB[app.state.vrgtok[app.lpui->viVCRFocus].iplr] + (app.state.vrgtok[app.lpui->viVCRFocus].ishdef - 16);
                } else {
                    lpshdef = app.state.rglpshdef[app.state.vrgtok[app.lpui->viVCRFocus].iplr] + app.state.vrgtok[app.lpui->viVCRFocus].ishdef;
                }
                csh = app.state.vrgtok[app.lpui->viVCRFocus].csh;
                GetVCRStats(app, app.lpui->viVCRFocus, &dpT, &dv, &dpShields, &cshT);
                cshNew = cshT;
                cshT = csh - cshT;
                if (csh > 1 || cshT != 0) {
                    c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsSD), lpshdef->hul.szClass, csh);
                } else {
                    std::strcpy(app.state.szWork, lpshdef->hul.szClass);
                    c = std::strlen(app.state.szWork);
                }
                if (cshT != 0) {
                    c += win32::wsprintfA(&app.state.szWork[c], " (-%d)", cshT);
                }
                win32::SetTextColor(hdc, 8323072);
                win32::TextOutA(hdc, x, y, app.state.szWork, c);
                y += app.state.dyArial8;
                xT = static_cast<std::int16_t>((rc.right - (app.lpui->dxyVCRBoard + 14))) / 2 + x;
                win32::SetTextColor(hdc, app.lpui->crButtonText);
                if (cshNew > 0) {
                    if (app.state.vrgtok[app.lpui->viVCRFocus].grobj == grobjPlanet) {
                        i = 0;
                    } else {
                        i = app.state.vrgtok[app.lpui->viVCRFocus].spd + 1;
                    }
                    c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsInitiativeD), app.state.vrgtok[app.lpui->viVCRFocus].initMin >= 0xff ? 0 : app.state.vrgtok[app.lpui->viVCRFocus].initMin);
                    win32::TextOutA(hdc, x, y, app.state.szWork, c);
                    c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsMovementS), &app.lpui->rgszSpeed[i * 3]);
                    win32::TextOutA(hdc, xT, y, app.state.szWork, c);
                    y += app.state.dyArial8;
                    c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsArmorLd), dpT);
                    win32::TextOutA(hdc, x, y, app.state.szWork, c);
                    if (dv.dp == 0) {
                        c = TextIO().pfnCchGetString(app, idsDamageNone, app.state.szWork);
                    } else {
                        csh -= cshT;
                        csh = LOWORD(static_cast<std::int32_t>((dv.pctSh() * csh)) / 100);
                        if (csh <= 0) {
                            csh = 1;
                        }
                        dpT = static_cast<std::uint32_t>((dv.pctDp() / 5));
                        if (dpT == 0) {
                            dpT = 1;
                        }
                        win32::SetTextColor(hdc, 127);
                        if (app.state.vrgtok[app.lpui->viVCRFocus].grobj == grobjPlanet) {
                            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDamageD), LOWORD(dpT));
                        } else {
                            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDamageLdD), csh, LOWORD(dpT));
                        }
                    }
                    win32::TextOutA(hdc, xT, y, app.state.szWork, c);
                    win32::SetTextColor(hdc, app.lpui->crButtonText);
                    y += app.state.dyArial8;
                    if (static_cast<std::uint32_t>((static_cast<std::uint32_t>(app.state.vrgtok[app.lpui->viVCRFocus].dpShield) * cshNew)) - dpShields <= 0) {
                        c = TextIO().pfnCchGetString(app, idsShieldsNone, app.state.szWork);
                    } else {
                        c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsShieldsLd), static_cast<std::uint32_t>((static_cast<std::uint32_t>(app.state.vrgtok[app.lpui->viVCRFocus].dpShield) * cshNew)) - dpShields);
                    }
                    win32::TextOutA(hdc, x, y, app.state.szWork, c);
                    y += app.state.dyArial8;
                    if (app.state.vrgtok[app.lpui->viVCRFocus].pctJam != 0) {
                        c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsJammingD), app.state.vrgtok[app.lpui->viVCRFocus].pctJam);
                        win32::TextOutA(hdc, x, y, app.state.szWork, c);
                        y += app.state.dyArial8;
                    }
                    if (app.state.vrgtok[app.lpui->viVCRFocus].grobj != grobjPlanet) {
                        if (app.lpui->vbrcVCRFocus != 0xff) {
                            i = idsDisengage + app.state.vrgtok[app.lpui->viVCRFocus].mdTactic;
                        } else {
                            i = idsDisengage2;
                        }
                        if (i == idsDisengage) {
                            TextIO().pfnCchGetString(app, idsTacticSDMoves, szT);
                            c = win32::wsprintfA(app.state.szWork, szT, TextIO().pfnCompressed(app, static_cast<StringId>(i)), app.state.vrgtok[app.lpui->viVCRFocus].dzDis);
                        } else {
                            TextIO().pfnCchGetString(app, idsTacticS, szT);
                            c = win32::wsprintfA(app.state.szWork, szT, TextIO().pfnCompressed(app, static_cast<StringId>(i)));
                        }
                        win32::TextOutA(hdc, x, y, app.state.szWork, c);
                        y += app.state.dyArial8;
                    }
                    if (i != idsDisengage2) {
                        TextIO().pfnCchGetString(app, idsPrimayTargetS, szT);
                        i = idsNoneDisengage + app.state.vrgtok[app.lpui->viVCRFocus].mdTarget1;
                        c = win32::wsprintfA(app.state.szWork, szT, TextIO().pfnCompressed(app, static_cast<StringId>(i)));
                        win32::TextOutA(hdc, x, y, app.state.szWork, c);
                        y += app.state.dyArial8;
                        TextIO().pfnCchGetString(app, idsSecondaryTargetS, szT);
                        i = idsNoneDisengage + app.state.vrgtok[app.lpui->viVCRFocus].mdTarget2;
                        c = win32::wsprintfA(app.state.szWork, szT, TextIO().pfnCompressed(app, static_cast<StringId>(i)));
                        win32::TextOutA(hdc, x, y, app.state.szWork, c);
                        y += app.state.dyArial8;
                    }
                } else {
                    c = TextIO().pfnCchGetString(app, idsDead3, app.state.szWork);
                    win32::TextOutA(hdc, x, y, app.state.szWork, c);
                    y += 5 * app.state.dyArial8;
                }
                win32::SetRect(&rc, x, y + 4, x + app.state.dyArial8 + 4, y + app.state.dyArial8 + 8);
                DrawBtn(app, hdc, &rc, 8, 0, "?");
            }
        }
        iStart = 0;
        iEnd = 99;
    }
    for (i = iStart; i <= iEnd; i++) {
        x = i % 10;
        y = i / 10;
        win32::PatBlt(hdc, (app.lpui->dxyVCRSquare + 3) * x + 10, (app.lpui->dxyVCRSquare + 3) * y + 10, app.lpui->dxyVCRSquare + 2, 1, win32::BLACKNESS);
        win32::PatBlt(hdc, (app.lpui->dxyVCRSquare + 3) * x + 10, (app.lpui->dxyVCRSquare + 3) * y + 10, 1, app.lpui->dxyVCRSquare + 2, win32::BLACKNESS);
        win32::PatBlt(hdc, (app.lpui->dxyVCRSquare + 3) * x + 10 + app.lpui->dxyVCRSquare + 1, (app.lpui->dxyVCRSquare + 3) * y + 10, 1, app.lpui->dxyVCRSquare + 2, win32::BLACKNESS);
        win32::PatBlt(hdc, (app.lpui->dxyVCRSquare + 3) * x + 10, (app.lpui->dxyVCRSquare + 3) * y + 10 + app.lpui->dxyVCRSquare + 1, app.lpui->dxyVCRSquare + 2, 1, win32::BLACKNESS);
        ctok = 0;
        ibmp = -1;
        dpT = 0;
        for (j = 0; j < BdvFromLp(app, app.lpui->vlpbdVCR).ctok(); j++) {
            if (app.state.vrgtok[j].brc == (((y & 0xf) << 4 | (x & 0xf)) & 0xff) && app.state.vrgtok[j].csh > 0) {
                ctok++;
                dpT += static_cast<std::uint32_t>(app.state.vrgtok[j].csh);
                if (ibmp == -1 || j == app.lpui->viVCRFocus) {
                    if (app.state.vrgtok[j].grobj == grobjPlanet) {
                        ibmp = app.state.rglpshdefSB[app.state.vrgtok[j].iplr][app.state.vrgtok[j].ishdef - 16].hul.ibmp;
                    } else {
                        ibmp = app.state.rglpshdef[app.state.vrgtok[j].iplr][app.state.vrgtok[j].ishdef].hul.ibmp;
                    }
                    ibmpRace = app.state.rgplr[app.state.vrgtok[j].iplr].iPlrBmp;
                }
            }
        }
        if (dpT < 32767) {
            csh = LOWORD(dpT);
        } else {
            csh = 32767;
        }
        if (ctok > 0) {
            win32::PatBlt(hdc, (app.lpui->dxyVCRSquare + 3) * x + 10, (app.lpui->dxyVCRSquare + 3) * y + 10, app.lpui->dxyVCRSquare + 2, 1, win32::BLACKNESS);
            win32::PatBlt(hdc, (app.lpui->dxyVCRSquare + 3) * x + 10, (app.lpui->dxyVCRSquare + 3) * y + 10, 1, app.lpui->dxyVCRSquare + 2, win32::BLACKNESS);
            win32::PatBlt(hdc, (app.lpui->dxyVCRSquare + 3) * x + 10 + app.lpui->dxyVCRSquare + 1, (app.lpui->dxyVCRSquare + 3) * y + 10, 1, app.lpui->dxyVCRSquare + 2, win32::BLACKNESS);
            win32::PatBlt(hdc, (app.lpui->dxyVCRSquare + 3) * x + 10, (app.lpui->dxyVCRSquare + 3) * y + 10 + app.lpui->dxyVCRSquare + 1, app.lpui->dxyVCRSquare + 2, 1, win32::BLACKNESS);
            DrawFleetBitmap(app, nullptr, hdc, (app.lpui->dxyVCRSquare + 3) * x + 11, (app.lpui->dxyVCRSquare + 3) * y + 11, 0, ibmp, ctok, app.lpui->dxyVCRSquare < 64, ibmpRace, csh);
        } else {
            win32::PatBlt(hdc, (app.lpui->dxyVCRSquare + 3) * x + 10, (app.lpui->dxyVCRSquare + 3) * y + 10, app.lpui->dxyVCRSquare + 2, app.lpui->dxyVCRSquare + 2, win32::BLACKNESS);
        }
        if ((app.lpui->vbrcVCRFocus == (((y & 0xf) << 4 | (x & 0xf)) & 0xff) && app.lpui->viVCRFocus == -1) ||
            (app.lpui->viVCRFocus >= 0 && app.state.vrgtok[app.lpui->viVCRFocus].brc == (((y & 0xf) << 4 | (x & 0xf)) & 0xff) && app.state.vrgtok[app.lpui->viVCRFocus].csh > 0)) {
            hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrBlue));
            win32::PatBlt(hdc, (app.lpui->dxyVCRSquare + 3) * x + 10, (app.lpui->dxyVCRSquare + 3) * y + 10, app.lpui->dxyVCRSquare + 1, 2, win32::PATCOPY);
            win32::PatBlt(hdc, (app.lpui->dxyVCRSquare + 3) * x + 10, (app.lpui->dxyVCRSquare + 3) * y + 10, 2, app.lpui->dxyVCRSquare + 1, win32::PATCOPY);
            win32::PatBlt(hdc, (app.lpui->dxyVCRSquare + 3) * x + 10, (app.lpui->dxyVCRSquare + 3) * y + 10 + app.lpui->dxyVCRSquare, app.lpui->dxyVCRSquare + 1, 2, win32::PATCOPY);
            win32::PatBlt(hdc, (app.lpui->dxyVCRSquare + 3) * x + 10 + app.lpui->dxyVCRSquare, (app.lpui->dxyVCRSquare + 3) * y + 10, 2, app.lpui->dxyVCRSquare + 1, win32::PATCOPY);
            win32::SelectObject(hdc, hbrSav);
        }
    }
    if (BevFromLp(app, app.lpui->vlpbrVCR).ctok() > 0) {
        AnimateAttack(app, hdc);
    }
    win32::SetBkMode(hdc, bkMode);
    win32::SelectObject(hdc, hbrSav);
    if (fCreatedDC) {
        win32::ReleaseDC(app.lpui->hwndVCRDlg, hdc);
    }
    return;
}

void Delay(std::int16_t ctick) {
    std::uint32_t dwTickLast;
    std::uint32_t dwTickCur;

    dwTickLast = win32::GetTickCount();
    do {
        dwTickCur = win32::GetTickCount();
    } while (dwTickCur >= dwTickLast && dwTickCur < ctick + dwTickLast);
    return;
}

void AnimateAttack(APPSESSION &app, HDC hdc) {
    TOK      *ptokSrc;
    TOK      *ptokAttack;
    POINT16   ptBeam1;
    std::int16_t   cFrame;
    std::int16_t   dyFrame;
    POINT16   ptRay2;
    POINT16   ptTop;
    std::uint32_t  dwTickLast;
    std::int16_t   dxFrame;
    std::uint32_t  dwTickCur;
    POINT16   ptBase;
    std::int16_t   dy;
    POINT16   ptRay1;
    std::int16_t   y;
    POINT16   ptRight;
    POINT16   ptDest;
    std::int16_t   iHit;
    GrfWeapon grfWeapon;
    POINT16   ptSrc;
    POINT16   ptTorp;
    POINT16   ptLeft;
    std::int16_t   iFrame;
    std::int16_t   dx;
    std::int16_t   fKill;
    POINT16   ptDestBottom;
    POINT16   ptBeam2;
    POINT16   ptBottom;
    POINT16   ptDestTop;
    POINT16   ptDestRight;
    POINT16   ptDestLeft;
    std::int16_t   x;
    HDC       hdcMem;
    HBITMAP   hbmpSav;
    HBITMAP   hbmpScreen;

    grfWeapon = static_cast<GrfWeapon>(0);
    fKill = 0;
    if (app.lpui->viStepVCRCur >= 0) {
        ptokSrc = app.state.vrgtok + BevFromLp(app, app.lpui->vlpbrVCR).itok();
        x = ptokSrc->brc & 0xf;
        y = ptokSrc->brc >> 4;
        ptSrc.x = (app.lpui->dxyVCRSquare + 3) * x + 10 + app.lpui->dxyVCRSquare / 2 + 1;
        ptSrc.y = (app.lpui->dxyVCRSquare + 3) * y + 10 + app.lpui->dxyVCRSquare / 2 + 1;
        ptBottom.x = ptTop.x = (app.lpui->dxyVCRSquare + 3) * x + 10 + app.lpui->dxyVCRSquare / 2 + 1;
        ptLeft.y = ptRight.y = (app.lpui->dxyVCRSquare + 3) * y + 10 + app.lpui->dxyVCRSquare / 2 + 1;
        ptBottom.y = app.lpui->dxyVCRSquare / 3 + ptLeft.y;
        ptTop.y = ptLeft.y - app.lpui->dxyVCRSquare / 3;
        ptLeft.x = ptBottom.x - app.lpui->dxyVCRSquare / 3;
        ptRight.x = app.lpui->dxyVCRSquare / 3 + ptBottom.x;
        iHit = 0;
    LNextTarget:
        grfWeapon = static_cast<GrfWeapon>(BevFromLp(app, app.lpui->vlpbrVCR).rgkill(iHit).grfWeapon());
        if (BevFromLp(app, app.lpui->vlpbrVCR).rgkill(iHit).cshKill() != 0) {
            fKill = 1;
        }
        for (iFrame = iHit + 1; iFrame < BevFromLp(app, app.lpui->vlpbrVCR).ctok() && BevFromLp(app, app.lpui->vlpbrVCR).rgkill(iFrame).itok() == BevFromLp(app, app.lpui->vlpbrVCR).rgkill(iHit).itok(); iFrame++) {
            grfWeapon = static_cast<GrfWeapon>(grfWeapon | BevFromLp(app, app.lpui->vlpbrVCR).rgkill(iFrame).grfWeapon());
            if (BevFromLp(app, app.lpui->vlpbrVCR).rgkill(iFrame).cshKill() != 0) {
                fKill = 1;
            }
        }
        ptokAttack = app.state.vrgtok + BevFromLp(app, app.lpui->vlpbrVCR).rgkill(iHit).itok();
        x = ptokAttack->brc & 0xf;
        y = ptokAttack->brc >> 4;
        dx = (ptokSrc->brc & 0xf) - x;
        dy = (ptokSrc->brc >> 4) - y;
        ptDest.x = (app.lpui->dxyVCRSquare + 3) * x + 10 + app.lpui->dxyVCRSquare / 2 + 1;
        ptDest.y = (app.lpui->dxyVCRSquare + 3) * y + 10 + app.lpui->dxyVCRSquare / 2 + 1;
        ptDestBottom.x = ptDestTop.x = (app.lpui->dxyVCRSquare + 3) * x + 10 + app.lpui->dxyVCRSquare / 2 + 1;
        ptDestLeft.y = ptDestRight.y = (app.lpui->dxyVCRSquare + 3) * y + 10 + app.lpui->dxyVCRSquare / 2 + 1;
        ptDestBottom.y = app.lpui->dxyVCRSquare / 3 + ptDestLeft.y;
        ptDestTop.y = ptDestLeft.y - app.lpui->dxyVCRSquare / 3;
        ptDestLeft.x = ptDestBottom.x - app.lpui->dxyVCRSquare / 3;
        ptDestRight.x = app.lpui->dxyVCRSquare / 3 + ptDestBottom.x;
        iHit = iFrame;
        if (dx != 0 || dy != 0) {
            if (dx == 0 || (std::abs(dx) == 1 && std::abs(dy) > 2)) {
                ptBeam1 = ptRight;
                ptBeam2 = ptLeft;
                if (dy > 0) {
                    ptTorp = ptTop;
                } else {
                    ptTorp = ptBottom;
                }
                if (dy > 0) {
                    ptRay2 = ptDestLeft;
                } else {
                    ptRay2 = ptDestRight;
                }
                if (dy > 0) {
                    ptRay1 = ptDestRight;
                } else {
                    ptRay1 = ptDestLeft;
                }
            } else if (dy == 0 || (std::abs(dy) == 1 && std::abs(dx) > 2)) {
                ptBeam1 = ptTop;
                ptBeam2 = ptBottom;
                if (dx > 0) {
                    ptTorp = ptLeft;
                } else {
                    ptTorp = ptRight;
                }
                if (dx > 0) {
                    ptRay1 = ptDestTop;
                } else {
                    ptRay1 = ptDestBottom;
                }
                if (dx > 0) {
                    ptRay2 = ptDestBottom;
                } else {
                    ptRay2 = ptDestTop;
                }
            } else if (dx > 0) {
                if (dy > 0) {
                    ptRay2 = ptDestBottom;
                    ptRay1 = ptDestRight;
                    ptBeam1 = ptTop;
                } else {
                    ptRay2 = ptDestRight;
                    ptRay1 = ptDestTop;
                    ptBeam1 = ptBottom;
                }
                ptBeam2 = ptLeft;
                ptTorp.x = ptLeft.x;
                ptTorp.y = ptBeam1.y;
            } else {
                if (dy > 0) {
                    ptBeam1 = ptTop;
                    ptRay1 = ptDestBottom;
                    ptRay2 = ptDestLeft;
                } else {
                    ptBeam1 = ptBottom;
                    ptRay1 = ptDestLeft;
                    ptRay2 = ptDestTop;
                }
                ptBeam2 = ptRight;
                ptTorp.x = ptRight.x;
                ptTorp.y = ptBeam1.y;
            }
            if (grfWeapon & (bitFBeamLow | bitFBeamHigh)) {
                win32::SelectObject(hdc, !(grfWeapon & bitFBeamHigh) ? app.lpui->hpenEnemy : app.lpui->hpenStarbase);
                win32::MoveToEx(hdc, ptBeam1.x, ptBeam1.y, nullptr);
                win32::LineTo(hdc, ptDest.x, ptDest.y);
                win32::MoveToEx(hdc, ptBeam2.x, ptBeam2.y, nullptr);
                win32::LineTo(hdc, ptDest.x, ptDest.y);
                win32::DrawIcon(hdc, ptDest.x - 16, ptDest.y - 16, app.lpui->rghiconVCR[0]);
            }
            if (grfWeapon & bitFTorp) {
                if (app.lpui->fAnimate) {
                    hdcMem = win32::CreateCompatibleDC(hdc);
                    if (!hdcMem)
                        goto LFinishUp;
                    hbmpScreen = win32::CreateCompatibleBitmap(hdc, 32, 32);
                    if (!hbmpScreen) {
                        win32::DeleteDC(hdcMem);
                        goto LFinishUp;
                    }
                    hbmpSav = static_cast<HBITMAP>(win32::SelectObject(hdcMem, hbmpScreen));
                    cFrame = (((std::abs(dx)) > (std::abs(dy))) ? (std::abs(dx)) : (std::abs(dy))) * (!(grfWeapon & bitFTorp) ? 4 : 8);
                    ptBase = ptTorp;
                    dxFrame = ptTorp.x - ptDest.x;
                    dyFrame = ptTorp.y - ptDest.y;
                    dwTickLast = win32::GetTickCount();
                    for (iFrame = 0; iFrame < cFrame; iFrame++) {
                        win32::BitBlt(hdcMem, 0, 0, 32, 32, hdc, ptTorp.x - 16, ptTorp.y - 16, win32::SRCCOPY);
                        win32::DrawIcon(hdc, ptTorp.x - 16, ptTorp.y - 16, app.lpui->rghiconVCR[(iFrame & 3) + 3]);
                        do {
                            dwTickCur = win32::GetTickCount();
                        } while (dwTickCur >= dwTickLast && dwTickCur < dwTickLast + 35 - static_cast<std::int16_t>((10 * app.lpui->viSpeedVCR)));
                        dwTickLast = dwTickCur;
                        win32::BitBlt(hdc, ptTorp.x - 16, ptTorp.y - 16, 32, 32, hdcMem, 0, 0, win32::SRCCOPY);
                        ptTorp.x = ptBase.x - LOWORD(static_cast<std::int32_t>((static_cast<std::int32_t>((dxFrame * iFrame)) / cFrame)));
                        ptTorp.y = ptBase.y - LOWORD(static_cast<std::int32_t>((static_cast<std::int32_t>((dyFrame * iFrame)) / cFrame)));
                    }
                    win32::SelectObject(hdcMem, hbmpSav);
                    win32::DeleteObject(hbmpScreen);
                    win32::DeleteDC(hdcMem);
                }
                if (!(grfWeapon & bitFNoHit)) {
                    win32::DrawIcon(hdc, ptDest.x - 16, ptDest.y - 16, app.lpui->rghiconVCR[1]);
                }
            }
        }
    LFinishUp:
        if (iHit < BevFromLp(app, app.lpui->vlpbrVCR).ctok())
            goto LNextTarget;
        for (iFrame = 0; iFrame < BevFromLp(app, app.lpui->vlpbrVCR).ctok(); iFrame++) {
            fKill = BevFromLp(app, app.lpui->vlpbrVCR).rgkill(iFrame).cshKill() > 0;
            ptokAttack = app.state.vrgtok + BevFromLp(app, app.lpui->vlpbrVCR).rgkill(iFrame).itok();
            ptDest.x = (ptokAttack->brc & 0xf) * (app.lpui->dxyVCRSquare + 3) + 10 + app.lpui->dxyVCRSquare / 2 + 1;
            ptDest.y = (ptokAttack->brc >> 4) * (app.lpui->dxyVCRSquare + 3) + 10 + app.lpui->dxyVCRSquare / 2 + 1;
            win32::DrawIcon(hdc, ptDest.x - 16, ptDest.y - 16, app.lpui->rghiconVCR[fKill == 0 ? 0 : 2]);
        }
        app.lpui->fAnimate = 0;
    }
    return;
}

std::int16_t PopupVCRMenu(APPSESSION &app, HWND hwnd, std::int16_t x, std::int16_t y, std::uint8_t brc) {
    std::int16_t fAttack;
    char   *rgsz[40];
    std::int16_t i;
    std::int16_t c;
    char    rgch[1536];
    SHDEF  *lpshdef;
    std::int16_t rgid[40];
    std::int16_t iChecked;
    std::int16_t iSel;
    std::int16_t j;
    char   *psz;
    std::int16_t cch;
    std::int16_t cKilled;

    c = 0;
    iChecked = -1;
    psz = rgch;
    fAttack = brc == app.state.vrgtok[BevFromLp(app, app.lpui->vlpbrVCR).itokAttack()].brc;
    for (i = 0; i < BdvFromLp(app, app.lpui->vlpbdVCR).ctok(); i++) {
        if (app.state.vrgtok[i].brc == brc && app.state.vrgtok[i].csh > 0) {
            if (TextIO().pfnPlayerName(app, app.state.vrgtok[i].iplr, 0, 0, 0, 0, nullptr) != app.state.szWork) {
            }
            cch = std::strlen(app.state.szWork);
            if (app.state.vrgtok[i].grobj == grobjPlanet) {
                lpshdef = app.state.rglpshdefSB[app.state.vrgtok[i].iplr] + (app.state.vrgtok[i].ishdef - 16);
            } else {
                lpshdef = app.state.rglpshdef[app.state.vrgtok[i].iplr] + app.state.vrgtok[i].ishdef;
            }
            cch += win32::wsprintfA(&app.state.szWork[cch], " %s * %d", lpshdef->hul.szClass, app.state.vrgtok[i].csh);
            if (fAttack && BevFromLp(app, app.lpui->vlpbrVCR).ctok() > 0 && app.lpui->viStepVCRCur >= 0) {
                cKilled = 0;
                for (j = 0; j < BevFromLp(app, app.lpui->vlpbrVCR).ctok(); j++) {
                    if (BevFromLp(app, app.lpui->vlpbrVCR).rgkill(j).itok() == i) {
                        cKilled += BevFromLp(app, app.lpui->vlpbrVCR).rgkill(j).cshKill();
                    }
                }
                if (cKilled > 0) {
                    cch += win32::wsprintfA(&app.state.szWork[cch], " (-%d)", cKilled);
                }
            }
            if (psz + (cch + 1) >= &rgch[1535] || c >= 40)
                break;
            rgsz[c] = psz;
            rgid[c] = i;
            std::strcpy(psz, app.state.szWork);
            psz += cch + 1;
            if (i == app.lpui->viVCRFocus) {
                iChecked = c;
            }
            c++;
        }
    }
    if (c == 0) {
        return -1;
    }
    iSel = ControlIO().pfnPopupMenu(app, hwnd, x, y, c, nullptr, rgsz, iChecked, 1);
    if (iSel == -1) {
        return -1;
    }
    return rgid[iSel];
}

void EnableVCRButtons(APPSESSION &app) {
    std::int16_t i;

    for (i = 161; i < 163; i++) {
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(app.lpui->hwndVCRDlg, i), app.lpui->viStepVCRCur > -1);
    }
    for (i = 163; i < 166; i++) {
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(app.lpui->hwndVCRDlg, i), app.lpui->viStepVCRCur < app.lpui->vcStepVCR);
    }
    if (app.lpui->viStepVCRCur == -1) {
        ControlIO().pfnSetFocus(ControlIO().pfnGetDlgItem(app.lpui->hwndVCRDlg, IDC_VCR_PLAY_PAUSE));
    } else if (app.lpui->viStepVCRCur == app.lpui->vcStepVCR) {
        ControlIO().pfnSetFocus(ControlIO().pfnGetDlgItem(app.lpui->hwndVCRDlg, IDOK));
    }
    return;
}
