#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"
#include "research_io.h"
#include "part_query_io.h"
#include "race_query_io.h"

INT_PTR  ResearchDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HDC         hdc;
    std::int16_t     y;
    std::int16_t     i;
    PAINTSTRUCT ps;
    std::int16_t     dx;
    RECT        rc;
    HWND        hwndRad;
    std::int16_t     dxCurrent;
    PLANET     *lppl;
    std::int16_t     c;
    PLANET     *lpplMac;
    HFONT       hfontSav;
    char       *psz;
    RECT        rcWindow;
    POINT16     pt;
    std::int16_t     iResTechNext;
    std::int16_t     fChg;

    switch (message) {
    case win32::WM_INITDIALOG:
        app.lpui->pctResGlob = app.state.rgplr[app.state.idPlayer].pctResearch;
        app.lpui->iResTechNow = static_cast<TechFieldType>(app.state.rgplr[app.state.idPlayer].iTechNow());
        win32::CheckRadioButton(hwnd, IDC_RESEARCH_ENERGY, IDC_RESEARCH_BIOTECH, app.lpui->iResTechNow + 1073);
        hdc = win32::GetDC(hwnd);
        hfontSav = static_cast<HFONT>(win32::SelectObject(hdc, app.lpui->rghfontArial8[1]));
        c = TextIO().pfnCchGetString(app, idsCurrent, std::span{app.state.szWork});
        dxCurrent = LOWORD(GetTextExtent(hdc, app.state.szWork, c));
        app.lpui->dxResRadio = 0;
        for (i = 1073; i <= 1078; i++) {
            c = TextIO().pfnCchGetString(app, static_cast<StringId>(idsEnergy + i - IDC_RESEARCH_ENERGY), std::span{app.state.szWork});
            dx = LOWORD(GetTextExtent(hdc, app.state.szWork, c));
            if (dx > app.lpui->dxResRadio) {
                app.lpui->dxResRadio = dx;
            }
            hwndRad = ControlIO().pfnGetDlgItem(hwnd, i);
            ControlIO().pfnSetWindowText(hwndRad, app.state.szWork);
            ControlIO().pfnSendMessage(hwndRad, win32::WM_SETFONT, reinterpret_cast<WPARAM>(app.lpui->rghfontArial8[1]), 0);
        }
        app.lpui->dxResRadio += 64;
        y = app.state.dyArial8 * 4 - (app.state.dyArial8 >> 1) + 1;
        for (i = 1073; i <= 1078; i++) {
            hwndRad = ControlIO().pfnGetDlgItem(hwnd, i);
            ControlIO().pfnSetWindowPos(hwndRad, nullptr, 16, y, app.lpui->dxResRadio, static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2, win32::SWP_NOZORDER | win32::SWP_SHOWWINDOW);
            y += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
        }
        hwndRad = ControlIO().pfnGetDlgItem(hwnd, IDC_RESEARCH_NEXT_FIELD);
        for (i = 0; i <= 7; i++) {
            psz = TextIO().pfnCompressed(app, static_cast<StringId>(idsSameField + i));
            ControlIO().pfnSendMessage(hwndRad, win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(psz));
        }
        i = app.state.rgplr[app.state.idPlayer].iTechNext();
        if (i == 6) {
            i = 0;
        } else if (i < 6) {
            i++;
        }
        ControlIO().pfnSendMessage(hwndRad, win32::CB_SETCURSEL, i, 0);
        app.lpui->dxResLeft = app.lpui->dxResRadio + dxCurrent + 40;
        app.lpui->dxResRight = 0;
        for (i = idsResourcesNeededComplete; i <= idsYearsProjectedResearchBudget; i++) {
            c = TextIO().pfnCchGetString(app, static_cast<StringId>(i), std::span{app.state.szWork});
            dx = LOWORD(GetTextExtent(hdc, app.state.szWork, c));
            if (dx > app.lpui->dxResRight) {
                app.lpui->dxResRight = dx;
            }
        }
        app.lpui->dxResStrRight = app.lpui->dxResRight;
        dx = LOWORD(GetTextExtent(hdc, TextIO().pfnCompressed(app, idsN9999992), 7));
        app.lpui->dxResRight += dx + 48;
        y += 10 * app.state.dyArial8;
        ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDCANCEL), nullptr, app.lpui->dxResLeft + app.lpui->dxResRight - 152, y - static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 - 8, 70, static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2,
                     win32::SWP_NOZORDER | win32::SWP_SHOWWINDOW);
        ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_HELP), nullptr, app.lpui->dxResLeft + app.lpui->dxResRight - 76, y - static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 - 8, 70, static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2,
                     win32::SWP_NOZORDER | win32::SWP_SHOWWINDOW);
        win32::GetWindowRect(hwnd, &rcWindow);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        ControlIO().pfnSetWindowPos(hwnd, nullptr, 0, 0, app.lpui->dxResLeft + app.lpui->dxResRight + rcWindow.right - rcWindow.left - rc.right, y + rcWindow.bottom - rcWindow.top - rc.bottom,
                     win32::SWP_NOMOVE | win32::SWP_NOZORDER);
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyResDlg, 1);
        win32::SelectObject(hdc, hfontSav);
        win32::ReleaseDC(hwnd, hdc);
        app.lpui->lResTotal = 0;
        lppl = app.state.lpPlanets;
        lpplMac = app.state.lpPlanets + app.state.cPlanet;
        for (; lppl < lpplMac; lppl++) {
            if (lppl->iPlayer == app.state.idPlayer) {
                app.lpui->lResTotal += ResearchIO().pfnResources(app, lppl, app.state.idPlayer);
            }
        }
        app.lpui->lResBudget = ProjectedResearchSpending(app, app.lpui->pctResGlob);
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
        for (i = 1073; i <= 1078 && reinterpret_cast<HWND>(lParam) != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 1078 || message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_LBUTTONDOWN:
    case win32::WM_LBUTTONDBLCLK:
        return FTrackResearchDlg(app, hwnd, LOWORD(lParam), HIWORD(lParam), wParam);
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        DrawResearchDlg(app, hwnd, hdc, &rc, -1);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (pt.x > 12 && pt.x < app.lpui->dxResLeft - 12 && pt.y >= app.lpui->yTopFutureTech && pt.y < app.lpui->cFutureTech * app.state.dyArial8 + app.lpui->yTopFutureTech) {
            win32::SetCursor(app.lpui->hcurArrowHelp);
            return 1;
        }
        if (app.lpui->yTopTechNote != -1 && pt.y >= app.lpui->yTopTechNote && pt.y < app.state.dyArial8 * 2 + app.lpui->yTopTechNote && pt.x > app.lpui->dxResLeft) {
            win32::SetCursor(app.lpui->hcurArrowHelp);
            return 1;
        }
        /* fallthrough */
    case win32::WM_COMMAND:
        if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_RESEARCH_ENERGY && LOWORD(wParam) <= IDC_RESEARCH_BIOTECH) {
            if (win32::IsDlgButtonChecked(hwnd, LOWORD(wParam)) != 0) {
                app.lpui->iResTechNow = static_cast<TechFieldType>(LOWORD(wParam) - 1073);
                ControlIO().pfnGetClientRect(hwnd, &rc);
                DrawResearchDlg(app, hwnd, nullptr, &rc, 4);
            }
        } else {
            if (LOWORD(wParam) == IDCANCEL) {
                fChg = 0;
                iResTechNext = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_RESEARCH_NEXT_FIELD), win32::CB_GETCURSEL, 0, 0));
                if (iResTechNext == 0) {
                    iResTechNext = 6;
                } else if (iResTechNext <= 6) {
                    iResTechNext--;
                }
                if (app.lpui->iResTechNow != app.state.rgplr[app.state.idPlayer].iTechNow() || iResTechNext != app.state.rgplr[app.state.idPlayer].iTechNext() || app.lpui->pctResGlob != app.state.rgplr[app.state.idPlayer].pctResearch) {
                    app.state.rgplr[app.state.idPlayer].pctResearch = app.lpui->pctResGlob;
                    app.state.rgplr[app.state.idPlayer].SetITechNow(app.lpui->iResTechNow);
                    app.state.rgplr[app.state.idPlayer].SetITechNext(iResTechNext);
                    i = app.state.rgplr[app.state.idPlayer].iTechCur * 256 + app.lpui->pctResGlob;
                    WriteMemResearch(app, i);
                    fChg = 1;
                    if (app.state.gd.fTutorial && app.state.idPlayer == 0) {
                        app.state.tutor.fChange = 1;
                        PresentationIO().pfnTutor(app);
                    }
                }
                ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyResDlg, 0);
                ControlIO().pfnEndDialog(hwnd, fChg);
                app.lpui->pctResGlob = -1;
                if (app.state.gd.fTutorial) {
                    PresentationIO().pfnTutor(app);
                }
                return 1;
            }
            if (LOWORD(wParam) == IDC_HELP) {
                win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhResearchDialog);
                return 1;
            }
        }
    }
    return 0;
}

void DrawResearchDlg(APPSESSION &app, HWND hwnd, HDC hdc, RECT *prc, std::int16_t grbitDraw) {
    std::int16_t      dxCurrent;
    char         szTemp[60];
    RECT         rcT;
    std::int16_t      iMax;
    std::int16_t      iTechSav;
    std::int16_t      iter;
    std::int16_t      fCreatedDC;
    mdPartAvail  mdAvail;
    std::int16_t      i;
    std::int16_t      c;
    HullSlotType grbitCur;
    COLORREF     crBackSav;
    COLORREF     crForeSav;
    HFONT        hfontSav;
    std::int16_t      xNum;
    std::int16_t      xCtr;
    std::int16_t      dx;
    char         szTemp2[60];
    PART         part;
    std::int32_t      l;
    std::int16_t      iMin;
    RECT         rc;
    std::int32_t      lSpent;
    HBRUSH       hbrSav;
    std::int32_t      lRBEffective;
    std::int16_t      cch;

    fCreatedDC = 0;
    if (!hdc) {
        fCreatedDC = 1;
        hdc = win32::GetDC(hwnd);
    }
    hfontSav = static_cast<HFONT>(win32::SelectObject(hdc, app.lpui->rghfontArial8[1]));
    crForeSav = win32::SetTextColor(hdc, 0);
    crBackSav = win32::SetBkColor(hdc, app.lpui->crButtonFace);
    if (!(grbitDraw & 0xff))
        goto DrawRightSide;

    c = TextIO().pfnCchGetString(app, idsCurrent, std::span{app.state.szWork});
    dxCurrent = LOWORD(GetTextExtent(hdc, app.state.szWork, c));
    win32::SetRect(&rc, 8, app.state.dyArial8, app.lpui->dxResLeft - 8, 12 * app.state.dyArial8 + app.state.dyArial8);
    if (!(grbitDraw & 3))
        goto DrawComingAttractions;

    _Draw3dFrame(app, hdc, &rc, -1);
    c = TextIO().pfnCchGetString(app, idsTechnologyStatus, std::span{app.state.szWork});
    win32::TextOutA(hdc, rc.left + 8, rc.top - (app.state.dyArial8 >> 1), app.state.szWork, c);
    rc.top += app.state.dyArial8 >> 1;
    c = TextIO().pfnCchGetString(app, idsCurrent, std::span{app.state.szWork});
    win32::TextOutA(hdc, rc.right - 8 - dxCurrent, rc.top, app.state.szWork, c);
    rc.top += app.state.dyArial8;
    i = TextIO().pfnCchGetString(app, idsFieldStudy, std::span{app.state.szWork});
    win32::TextOutA(hdc, rc.left + 26, rc.top, app.state.szWork, i);
    xCtr = rc.right - 8 - (dxCurrent >> 1);
    i = TextIO().pfnCchGetString(app, idsLevel, std::span{app.state.szWork});
    CtrTextOut(hdc, xCtr, rc.top, app.state.szWork, i);
    win32::PatBlt(hdc, rc.left + 8, rc.top + app.state.dyArial8, rc.right - rc.left - 16, 1, win32::BLACKNESS);
    rc.top += (app.state.dyArial8 >> 2) + app.state.dyArial8 + 2;
    for (i = 0; i < 6; i++) {
        c = win32::wsprintfA(app.state.szWork, app.state.PCTD, app.state.rgplr[app.state.idPlayer].rgTech[i]);
        CtrTextOut(hdc, xCtr, rc.top, app.state.szWork, c);
        rc.top += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
    }

DrawComingAttractions:
    win32::SetRect(&rc, 8, static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 + rc.bottom, app.lpui->dxResLeft - 8, prc->bottom - 8);
    if ((grbitDraw & 4) && !(grbitDraw & 3)) {
        ExpandRc(&rc, -6, -(app.state.dyArial8 >> 1));
        rc.top += app.state.dyArial8 >> 1;
        ControlIO().pfnInvalidateRect(hwnd, &rc, 1);
        goto DrawRightSide;
    } else {
        _Draw3dFrame(app, hdc, &rc, -1);
        c = TextIO().pfnCchGetString(app, idsExpectedResearchBenefits, std::span{app.state.szWork});
        win32::TextOutA(hdc, rc.left + 8, rc.top - (app.state.dyArial8 >> 1), app.state.szWork, c);
        iTechSav = app.state.rgplr[app.state.idPlayer].iTechNow();
        app.lpui->cFutureTech = 0;
        app.lpui->yTopFutureTech = rc.top + app.state.dyArial8;
        for (iter = 1; iter < 10; iter++) {
            iMax = iter;
            iMin = iter;
            if (iter == 1) {
                win32::SetTextColor(hdc, 32512);
            } else if (iter >= 2 && iter <= 4) {
                win32::SetTextColor(hdc, 8323072);
            } else {
                win32::SetTextColor(hdc, 0);
                if (iter == 9) {
                    iMax = 26;
                }
            }
            grbitCur = hstEngine;
            app.state.rgplr[app.state.idPlayer].SetITechNow(app.lpui->iResTechNow);
            for (; grbitCur != hstNone; grbitCur = static_cast<HullSlotType>(grbitCur * 2)) {
                if (grbitCur & (hstEngine | hstScanner | hstShield | hstArmor | hstBeam | hstTorp | hstBomb | hstMining | hstMines | hstSpecialSB | hstSBHull |
                                hstSpecialE | hstSpecialM | hstTerra | hstHull | hstPlanetary)) {
                    i = 0;
                    part.hs.grhst = grbitCur;
                    while (1) {
                        part.hs.iItem = i;
                        mdAvail = static_cast<mdPartAvail>(PartQueryIO().pfnLookup(app, &part));
                        if (mdAvail == mdPartAvailInvalid)
                            break;
                        if (part.hs.grhst == hstTerra && RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceTT) != 0 &&
                            (part.hs.iItem == iterraGravityTerraform3 || part.hs.iItem == iterraTempTerraform3 || part.hs.iItem == iterraRadiationTerraform3))
                            break;
                        if (iMin <= mdAvail - 1 && iMax >= mdAvail - 1) {
                            rc.top += app.state.dyArial8;
                            if (rc.top + app.state.dyArial8 > rc.bottom)
                                goto TooManyToFinish;
                            std::strcpy(app.state.szWork, part.pcom().szName());
                            win32::TextOutA(hdc, rc.left + 8, rc.top, app.state.szWork, std::strlen(app.state.szWork));
                            if (app.lpui->cFutureTech < 8) {
                                app.lpui->rghsFutureTech[app.lpui->cFutureTech++] = part.hs;
                            }
                        }
                        i++;
                    }
                }
            }
        }
    TooManyToFinish:
        app.state.rgplr[app.state.idPlayer].SetITechNow(iTechSav);
        win32::SetTextColor(hdc, 0);
    }

DrawRightSide:
    win32::SetRect(&rc, app.lpui->dxResLeft + 8, app.state.dyArial8, app.lpui->dxResLeft + app.lpui->dxResRight - 8, static_cast<std::int16_t>((14 * app.state.dyArial8)) / 2 + app.state.dyArial8);
    xCtr = rc.left + 8 + app.lpui->dxResStrRight;
    xNum = LOWORD(GetTextExtent(hdc, "999999", 6)) + xCtr;
    if ((grbitDraw & 4) && !(grbitDraw & 3)) {
        ExpandRc(&rc, -6, -(app.state.dyArial8 >> 1));
        rc.top += app.state.dyArial8 >> 1;
        ControlIO().pfnInvalidateRect(hwnd, &rc, 1);
        goto CleanUp;
    }
    if (!(grbitDraw & 0x300))
        goto DrawResourceAlloc;

    if (app.state.rgplr[app.state.idPlayer].rgTech[app.lpui->iResTechNow] >= 26) {
        l = -1;
    } else {
        l = ResearchIO().pfnTechCost(app, app.lpui->iResTechNow, app.state.rgplr[app.state.idPlayer].rgTech[app.lpui->iResTechNow] + 1, app.state.idPlayer);
        lSpent = app.state.rgplr[app.state.idPlayer].rgResSpent[app.lpui->iResTechNow];
        if (app.state.game.fSlowTech) {
            lSpent = static_cast<std::int32_t>((lSpent * 2));
        }
        l = 0 <= l - lSpent ? l - lSpent : 0;
    }
    if ((grbitDraw & 0x200) && !(grbitDraw & 0xf)) {
        rc.top += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 * 2 + app.state.dyArial8;
        hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrButtonFace));
        win32::PatBlt(hdc, xCtr, rc.top, xNum - xCtr, app.state.dyArial8, win32::PATCOPY);
        win32::SelectObject(hdc, hbrSav);
        goto DrawYearComplete;
    } else {
        _Draw3dFrame(app, hdc, &rc, -1);
        c = TextIO().pfnCchGetString(app, idsCurrentlyResearching, std::span{app.state.szWork});
        win32::TextOutA(hdc, rc.left + 8, rc.top - (app.state.dyArial8 >> 1), app.state.szWork, c);
        rc.top += app.state.dyArial8;
        TextIO().pfnCchGetString(app, static_cast<StringId>(idsEnergy + static_cast<int>(app.lpui->iResTechNow)), std::span{szTemp});
        TextIO().pfnCchGetString(app, idsSTechLevelD, std::span{szTemp2});
        c = win32::wsprintfA(app.state.szWork, szTemp2, szTemp, app.state.rgplr[app.state.idPlayer].rgTech[app.lpui->iResTechNow] + 1);
        RightTextOut(app, hdc, xCtr, rc.top, app.state.szWork, c, 0);
        rc.top += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
        RightTextOut(app, hdc, xCtr, rc.top, TextIO().pfnCompressed(app, idsResourcesNeededComplete), 0, 0);
        if (l == -1) {
            c = TextIO().pfnCchGetString(app, idsMaxed, std::span{app.state.szWork});
        } else {
            c = win32::wsprintfA(app.state.szWork, app.state.PCTLD, l);
        }
        win32::TextOutA(hdc, xCtr, rc.top, app.state.szWork, c);
        rc.top += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
        RightTextOut(app, hdc, xCtr, rc.top, TextIO().pfnCompressed(app, idsEstimatedTimeCompletion), 0, 0);
    }
DrawYearComplete:
    if (l == -1) {
        c = TextIO().pfnCchGetString(app, idsMaxed, std::span{app.state.szWork});
        win32::TextOutA(hdc, xCtr, rc.top, app.state.szWork, c);
    } else if (l == 0) {
        l = 1;
        goto PrintYear;
    } else if (app.lpui->lResBudget == 0) {
        c = TextIO().pfnCchGetString(app, idsNever2, std::span{app.state.szWork});
        win32::TextOutA(hdc, xCtr, rc.top, app.state.szWork, c);
    } else {
        lRBEffective = app.lpui->lResBudget;
        if (RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceGeneralizedResearch) != 0) {
            lRBEffective -= static_cast<std::int32_t>((lRBEffective >> 1));
        }
        l = static_cast<std::int32_t>(((l + lRBEffective - 1) / lRBEffective));
    PrintYear:
        c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsLdYearC), l, l == 1 ? 32 : 115);
        win32::TextOutA(hdc, xCtr, rc.top, app.state.szWork, c);
    }
    RightTextOut(app, hdc, xCtr - 60, rc.top + app.state.dyArial8 + 5, TextIO().pfnCompressed(app, idsFieldResearch), 0, 0);
    ControlIO().pfnGetClientRect(ControlIO().pfnGetDlgItem(hwnd, IDC_RESEARCH_NEXT_FIELD), &rcT);
    win32::MapWindowPoints(ControlIO().pfnGetDlgItem(hwnd, IDC_RESEARCH_NEXT_FIELD), hwnd, reinterpret_cast<POINT *>(&rcT), 2);
    if (rcT.top != rc.top + app.state.dyArial8 + 2) {
        ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_RESEARCH_NEXT_FIELD), nullptr, xCtr - 60, rc.top + app.state.dyArial8 + 2, rc.right - xCtr + 50, 9 * app.state.dyArial8,
                     win32::SWP_NOZORDER | win32::SWP_NOREDRAW);
    }

DrawResourceAlloc:
    rc.top = static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 + rc.bottom;
    rc.bottom = app.state.dyArial8 * 8 + rc.top;
    if (!(grbitDraw & 0x400))
        goto DrawAnnualRes;

    _Draw3dFrame(app, hdc, &rc, -1);
    c = TextIO().pfnCchGetString(app, idsResourceAllocation, std::span{app.state.szWork});
    win32::TextOutA(hdc, rc.left + 8, rc.top - (app.state.dyArial8 >> 1), app.state.szWork, c);

DrawAnnualRes:
    rc.top += app.state.dyArial8;
    if (!(grbitDraw & 0x800))
        goto DrawTotalSpent;

    RightTextOut(app, hdc, xCtr, rc.top, TextIO().pfnCompressed(app, idsAnnualResourcesPlanets), 0, 0);
    c = win32::wsprintfA(app.state.szWork, app.state.PCTLD, app.lpui->lResTotal);
    RightTextOut(app, hdc, xNum, rc.top, app.state.szWork, c, 0);

DrawTotalSpent:
    rc.top += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
    if (!(grbitDraw & 0x1000))
        goto DrawBudget;

    RightTextOut(app, hdc, xCtr, rc.top, TextIO().pfnCompressed(app, idsTotalResourcesSpentResearchLastYear), 0, 0);
    c = win32::wsprintfA(app.state.szWork, app.state.PCTLD, app.state.rgplr[app.state.idPlayer].lResLastYear);
    RightTextOut(app, hdc, xNum, rc.top, app.state.szWork, c, 0);

DrawBudget:
    rc.top += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
    dx = 0;
    if (grbitDraw & 0x2000) {
        RightTextOut(app, hdc, xCtr, rc.top, TextIO().pfnCompressed(app, idsResourcesBudgetedResearch), 0, 0);
    } else if ((grbitDraw & 0x4000) && !(grbitDraw & 0xf)) {
        dx = LOWORD(GetTextExtent(hdc, "100", 3));
        goto DrawResPct;
    }
DrawResPct:
    c = win32::wsprintfA(app.state.szWork, app.state.PCTD, app.lpui->pctResGlob);
    RightTextOut(app, hdc, xNum, rc.top, app.state.szWork, c, dx);
    if ((grbitDraw & 0x4000) && !(grbitDraw & 0xf))
        goto DrawProjBudg;

    dx = LOWORD(GetTextExtent(hdc, "%", 1));
    win32::TextOutA(hdc, xNum, rc.top, "%", 1);
    app.lpui->rcSpinTop.left = xNum + dx + 4;
    app.lpui->rcSpinTop.top = rc.top - 4;
    app.lpui->rcSpinTop.right = app.lpui->rcSpinTop.left + 15;
    app.lpui->rcSpinTop.bottom = (app.state.dyArial8 >> 1) + rc.top + 1;
    app.lpui->rcSpinBot = app.lpui->rcSpinTop;
    win32::OffsetRect(&app.lpui->rcSpinBot, 0, app.lpui->rcSpinTop.bottom - app.lpui->rcSpinTop.top - 1);
    DrawBtn(app, hdc, &app.lpui->rcSpinTop, 160, 0, nullptr);
    DrawBtn(app, hdc, &app.lpui->rcSpinBot, 161, 0, nullptr);

DrawProjBudg:
    rc.top += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
    if ((grbitDraw & 0x4000) && !(grbitDraw & 0xf))
        goto DrawProjBudgData;

    RightTextOut(app, hdc, xCtr, rc.top, TextIO().pfnCompressed(app, idsYearsProjectedResearchBudget), 0, 0);

DrawProjBudgData:
    c = win32::wsprintfA(app.state.szWork, app.state.PCTLD, app.lpui->lResBudget);
    RightTextOut(app, hdc, xNum, rc.top, app.state.szWork, c, xNum - xCtr);
    if (RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceGeneralizedResearch) != 0 || RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceBleedingEdgeTech) != 0) {
        cch = TextIO().pfnCchGetString(app, idsRaceHas, std::span{szTemp});
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        rc.top += 3 * app.state.dyArial8;
        rc.left = app.lpui->dxResLeft + 8;
        app.lpui->yTopTechNote = rc.top;
        dx = LOWORD(GetTextExtent(hdc, szTemp, cch));
        if (RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceGeneralizedResearch) != 0) {
            win32::TextOutA(hdc, rc.left, rc.top, szTemp, cch);
            c = TextIO().pfnCchGetString(app, idsGeneralizedResearch, std::span{app.state.szWork});
            win32::TextOutA(hdc, rc.left + dx, rc.top, app.state.szWork, c);
            rc.top += app.state.dyArial8 + 2;
        }
        if (RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceBleedingEdgeTech) == 0)
            goto CleanUp;

        win32::TextOutA(hdc, rc.left, rc.top, szTemp, cch);
        c = TextIO().pfnCchGetString(app, idsBleedingEdgeTechnology, std::span{app.state.szWork});
        win32::TextOutA(hdc, rc.left + dx, rc.top, app.state.szWork, c);

    } else {
        app.lpui->yTopTechNote = -1;
    }

CleanUp:
    win32::SetBkColor(hdc, crBackSav);
    win32::SetTextColor(hdc, crForeSav);
    win32::SelectObject(hdc, hfontSav);
    if (fCreatedDC) {
        win32::ReleaseDC(hwnd, hdc);
    }
    return;
}

std::int16_t FTrackResearchDlg(APPSESSION &app, HWND hwnd, std::int16_t x, std::int16_t y, std::int16_t fkb) {
    std::int16_t bt;
    POINT16 pt;
    std::int16_t dChg;
    std::int16_t i;
    std::int16_t cNew;
    RECT   *prc;
    BTNT    btnt;
    RECT    rc;

    pt.x = x;
    pt.y = y;
    if (win32::PtInRect(&app.lpui->rcSpinTop, PointFrom16(pt)) != 0) {
        i = 1;
        prc = &app.lpui->rcSpinTop;
        bt = 160;
    } else if (win32::PtInRect(&app.lpui->rcSpinBot, PointFrom16(pt)) != 0) {
        i = -1;
        prc = &app.lpui->rcSpinBot;
        bt = 161;
    } else {
        if (y >= app.lpui->yTopFutureTech && y < app.lpui->cFutureTech * app.state.dyArial8 + app.lpui->yTopFutureTech && x > 12 && x < app.lpui->dxResLeft - 12) {
            i = static_cast<std::int16_t>((y - app.lpui->yTopFutureTech)) / app.state.dyArial8;
            app.lpui->GlobalPD.part.hs = app.lpui->rghsFutureTech[i];
            PartQueryIO().pfnLookup(app, &app.lpui->GlobalPD.part);
            app.lpui->GlobalPD.grPopup = grPopupComponent;
            Popup(app, hwnd, x, y);
            return 1;
        }
        if (app.lpui->yTopTechNote != -1 && y >= app.lpui->yTopTechNote && y < 3 * app.state.dyArial8 + app.lpui->yTopTechNote && x > app.lpui->dxResLeft) {
            if (RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceGeneralizedResearch) != 0 &&
                (y < static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 + app.lpui->yTopTechNote || RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceBleedingEdgeTech) == 0)) {
                i = idsRaceTakesHolisticApproachResearchHalfResources;
            } else {
                i = idsNewTechsInitiallyCostTwiceMuchBuild;
            }
            app.lpui->GlobalPD.psz = TextIO().pfnCompressed(app, static_cast<StringId>(i));
            app.lpui->GlobalPD.dxOut = app.lpui->dxResRight;
            app.lpui->GlobalPD.grPopup = grPopupString;
            Popup(app, hwnd, x, y);
            return 1;
        }
        return 0;
    }
    ControlIO().pfnGetClientRect(hwnd, &rc);
    InitBtnTrack(&btnt, hwnd, nullptr, prc, bt, 80, 0, 0, nullptr);
    if (fkb & 4) {
        dChg = 10;
    } else {
        dChg = 1;
    }
    while (FTrackBtn(app, &btnt)) {
        cNew = dChg * i + app.lpui->pctResGlob;
        if (100 < (0 <= cNew ? cNew : 0)) {
            cNew = 100;
        } else if (0 > cNew) {
            cNew = 0;
        }
        if (cNew != app.lpui->pctResGlob) {
            app.lpui->pctResGlob = cNew;
            app.lpui->lResBudget = ProjectedResearchSpending(app, app.lpui->pctResGlob);
            DrawResearchDlg(app, hwnd, btnt.hdc, &rc, 16896);
        }
    }
    return 1;
}

INT_PTR  BrowserDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HDC         hdc;
    HMENU       hmenu;
    std::int16_t     i;
    std::int16_t     c;
    PAINTSTRUCT ps;
    HFONT       hfontSav;
    std::int16_t     dx;
    RECT        rc;
    HWND        hwndDD;
    std::int32_t     lSel;
    mdPartAvail md;
    std::int16_t     fShowAll;
    std::int16_t     fAllHsts;
    std::uint16_t    iItemStart;
    std::int16_t     iStart;
    std::int16_t     cIter;
    std::int16_t     iOff;

    switch (message) {
    case win32::WM_INITDIALOG:
        app.lpui->hwndBrowser = hwnd;
        ControlIO().pfnSetWindowPos(hwnd, nullptr, 0, 0, (app.state.dyArial8 <= 14 ? 0 : 40) + 358 + win32::GetSystemMetrics(win32::SM_CXDLGFRAME) * 2,
                     app.lpui->dyArial10 + 72 + 12 * app.state.dyArial8 + 6 + 3 * app.state.dyArial8 + 25 + win32::GetSystemMetrics(win32::SM_CYDLGFRAME) * 2 + win32::GetSystemMetrics(win32::SM_CYCAPTION),
                     win32::SWP_NOMOVE | win32::SWP_NOZORDER);
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyBrowserDlg, 1);
        hdc = win32::GetDC(hwnd);
        hfontSav = static_cast<HFONT>(win32::SelectObject(hdc, app.lpui->rghfontArial8[1]));
        hwndDD = ControlIO().pfnGetDlgItem(hwnd, IDC_BROWSER_COMPONENT_CATEGORY);
        c = win32::GetDlgItemTextA(hwnd, IDC_BACK, app.state.szWork, 80);
        dx = LOWORD(GetTextExtent(hdc, app.state.szWork, c)) + 14;
        ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_BACK), nullptr, 6, 6, dx, static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2, win32::SWP_NOZORDER);
        ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_NEXT), nullptr, (app.state.dyArial8 <= 14 ? 0 : 40) + 350 - dx, 6, dx, static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2, win32::SWP_NOZORDER);
        ControlIO().pfnSetWindowPos(hwndDD, nullptr, dx + 12, 6, (app.state.dyArial8 <= 14 ? 0 : 40) + 344 - dx * 2 - 12, 18 * app.state.dyArial8, win32::SWP_NOZORDER);
        ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDCANCEL), nullptr, (app.state.dyArial8 <= 14 ? 0 : 40) + 350 - dx,
                     app.lpui->dyArial10 + 72 + 12 * app.state.dyArial8 + 6 + static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 + 18, dx, static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2, win32::SWP_NOZORDER);
        ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_BROWSER_AVAILABLE_ONLY), nullptr, 6, app.lpui->dyArial10 + 72 + 12 * app.state.dyArial8 + 6 + static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 + 18,
                     (app.state.dyArial8 <= 14 ? 0 : 40) + 344 - dx, static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2, win32::SWP_NOZORDER);
        win32::SelectObject(hdc, hfontSav);
        win32::ReleaseDC(hwnd, hdc);
        if (!app.lpui->fBrowserValid) {
            app.lpui->vpartBrowser.hs.grhst = hstArmor;
            app.lpui->vpartBrowser.hs.iItem = 0;
        }
        PartQueryIO().pfnLookup(app, &app.lpui->vpartBrowser);
        app.lpui->hwndBrowserChild = ui::CreateWindowEx(0, app.lpui->szBrowser, nullptr, win32::WS_CHILD | win32::WS_VISIBLE, 6, static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 + 12, (app.state.dyArial8 <= 14 ? 0 : 40) + 344,
                                        app.lpui->dyArial10 + 72 + 12 * app.state.dyArial8 + 6, hwnd, nullptr, app.lpui->hInst, nullptr);
        for (i = idsAll; i <= idsTorpedoes; i++) {
            ControlIO().pfnSendMessage(hwndDD, win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TextIO().pfnCompressed(app, static_cast<StringId>(i))));
        }
        ControlIO().pfnSendMessage(hwndDD, win32::CB_SETCURSEL, 0, 0);
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
        for (i = 266; i <= 267 && reinterpret_cast<HWND>(lParam) != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 267 || message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_DESTROY:
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyBrowserDlg, 0);
        app.lpui->hwndBrowser = nullptr;
        app.lpui->fBrowserValid = 0;
        hmenu = GetASubMenu(app, app.lpui->hwndFrame, menuHelp);
        win32::CheckMenuItem(hmenu, IDM_VIEW_BROWSER_TOGGLE2, win32::MF_UNCHECKED);
        break;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyBrowserDlg, 0);
            app.lpui->hwndBrowser = nullptr;
            app.lpui->fBrowserValid = 0;
            hmenu = GetASubMenu(app, app.lpui->hwndFrame, menuHelp);
            win32::CheckMenuItem(hmenu, IDM_VIEW_BROWSER_TOGGLE2, win32::MF_UNCHECKED);
            // The browser is modeless (CreateDialog), so no modal loop owns it.
            ControlIO().pfnDestroyWindow(hwnd);
            if (app.state.gd.fTutorial) {
                PresentationIO().pfnTutor(app);
            }
            return 1;
        case IDC_BROWSER_AVAILABLE_ONLY:
            if (!win32::IsDlgButtonChecked(hwnd, IDC_BROWSER_AVAILABLE_ONLY) || PartQueryIO().pfnLookup(app, &app.lpui->vpartBrowser) == mdPartAvailAvailable)
                break;
            goto ResetCategory;
        case IDC_BROWSER_COMPONENT_CATEGORY:
            if (HIWORD(wParam) != 1)
                break;
        ResetCategory:
            fShowAll = win32::IsDlgButtonChecked(hwnd, IDC_BROWSER_AVAILABLE_ONLY) == 0;
            lSel = ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BROWSER_COMPONENT_CATEGORY), win32::CB_GETCURSEL, 0, 0);
            if (lSel < 0)
                break;
            app.lpui->vpartBrowser.hs.grhst = static_cast<HullSlotType>(rggrbitBrParts[lSel <= 1 ? 1 : lSel]);
            app.lpui->vpartBrowser.hs.iItem = 0;
            while (1) {
                md = static_cast<mdPartAvail>(PartQueryIO().pfnLookup(app, &app.lpui->vpartBrowser));
                if (md == mdPartAvailInvalid) {
                    app.lpui->vpartBrowser.SetPcom(nullptr);
                    break;
                }
                if (md == mdPartAvailAvailable || fShowAll)
                    break;
                app.lpui->vpartBrowser.hs.iItem++;
            }
            ControlIO().pfnInvalidateRect(app.lpui->hwndBrowserChild, nullptr, 1);
            break;
        case IDC_NEXT:
        case IDC_BACK:
            iItemStart = app.lpui->vpartBrowser.hs.iItem;
            cIter = 0;
            lSel = ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BROWSER_COMPONENT_CATEGORY), win32::CB_GETCURSEL, 0, 0);
            fAllHsts = lSel == 0;
            for (i = 0; i < 17 && app.lpui->vpartBrowser.hs.grhst != rggrbitBrParts[i]; i++) {
            }
            iStart = i;
            fShowAll = win32::IsDlgButtonChecked(hwnd, IDC_BROWSER_AVAILABLE_ONLY) == 0;
            iOff = LOWORD(wParam) == IDC_NEXT ? 1 : -1;
            do {
                if ((app.lpui->vpartBrowser.hs.iItem += iOff) == iItemStart && !fAllHsts)
                    break;
            Top:
                if (cIter++ > 350)
                    goto NullItem;
                md = static_cast<mdPartAvail>(PartQueryIO().pfnLookup(app, &app.lpui->vpartBrowser));
                if (md == mdPartAvailInvalid) {
                    if (LOWORD(wParam) == IDC_NEXT && fAllHsts) {
                        i++;
                        if (i >= 17) {
                            i = 1;
                        }
                        app.lpui->vpartBrowser.hs.grhst = static_cast<HullSlotType>(rggrbitBrParts[i]);
                        app.lpui->vpartBrowser.hs.iItem = 0;
                        goto Top;
                    }
                    if (app.lpui->vpartBrowser.hs.iItem == 0 && !fAllHsts) {
                        app.lpui->vpartBrowser.SetPcom(nullptr);
                        break;
                    }
                    if (LOWORD(wParam) == IDC_BACK && fAllHsts) {
                        if (app.lpui->vpartBrowser.hs.iItem > 100) {
                            i--;
                            if (i <= 0) {
                                i = 16;
                            }
                            app.lpui->vpartBrowser.hs.grhst = static_cast<HullSlotType>(rggrbitBrParts[i]);
                            app.lpui->vpartBrowser.hs.iItem = 100;
                            goto Top;
                        }
                    } else if (LOWORD(wParam) == IDC_NEXT) {
                        app.lpui->vpartBrowser.hs.iItem = 0;
                        goto Top;
                    }
                }
            } while (md != mdPartAvailAvailable &&
                     (md == mdPartAvailInvalid || !fShowAll || (md == mdPartAvailRestricted && PartQueryIO().pfnHidden(app, &app.lpui->vpartBrowser))));
            if (app.lpui->vpartBrowser.hs.iItem == iItemStart && iStart == i && (md == mdPartAvailAvailable || fShowAll))
                break;
            if (app.lpui->vpartBrowser.hs.iItem == iItemStart && iStart == i) {
                if (PartQueryIO().pfnLookup(app, &app.lpui->vpartBrowser) == mdPartAvailAvailable)
                    break;
            NullItem:
                app.lpui->vpartBrowser.SetPcom(nullptr);
            }
            ControlIO().pfnInvalidateRect(app.lpui->hwndBrowserChild, nullptr, 1);
        }
    }
    return 0;
}

LRESULT  BrowserWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HDC         hdc;
    POINT16     pt;
    std::int16_t     i;
    PAINTSTRUCT ps;
    RECT        rc;

    switch (message) {
    case win32::WM_CREATE:
        return 0;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        DisplayComponentInfo(app, hdc, rc.right, rc.bottom, &app.lpui->vpartBrowser);
        win32::EndPaint(hwnd, &ps);
        return 0;
    case win32::WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        goto Validate;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_LBUTTONDOWN:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
    Validate:
        if ((app.lpui->vpartBrowser.hs.grhst != hstHull && app.lpui->vpartBrowser.hs.grhst != hstSBHull) || app.lpui->hwndSlotDlg || pt.x < 5 || pt.x >= 69 || pt.y < app.lpui->dyArial10 + 5 ||
            pt.y >= app.lpui->dyArial10 + 69)
            goto Default;
        if (message == win32::WM_LBUTTONDOWN) {
            app.lpui->GlobalPD.grPopup = grPopupShdefSB;
            if (app.lpui->vpartBrowser.hs.grhst == hstSBHull) {
                app.state.shdefBuild.hul = LphuldefSBFromId(static_cast<isbhull>(app.lpui->vpartBrowser.hs.iItem))->hul;
            } else {
                app.state.shdefBuild.hul = PartQueryIO().pfnHull(app.lpui->vpartBrowser.hs.iItem)->hul;
            }
            for (i = 0; i < app.state.shdefBuild.hul.chs; i++) {
                app.state.shdefBuild.hul.rghs[i].cItem = 0;
            }
            app.lpui->GlobalPD.lpshdef = &app.state.shdefBuild;
            app.lpui->GlobalPD.fShowDamage = 0;
            Popup(app, hwnd, pt.x, pt.y);
        } else {
            win32::SetCursor(app.lpui->hcurArrowHelp);
        }
        return 1;
    default:
    Default:
        return ui::DefWindowProc(hwnd, message, wParam, lParam);
    }
}

void DisplayComponentInfo(APPSESSION &app, HDC hdc, std::int16_t dx, std::int16_t dy, PART *ppart) {
    std::uint16_t rgCosts[4];
    StringId idsT;
    StringId ids;
    std::int16_t  dxStr;
    std::int16_t  c;
    std::int16_t  yText;
    std::int16_t  i;
    std::int16_t  yCur;
    std::int16_t  fReq;
    std::int16_t  yStart;
    std::int16_t  xNum;
    std::int16_t  xText;
    RECT     rcData;
    std::int32_t  l;
    std::int16_t  dxT;
    char     rgch[2];
    std::int16_t  dyPct;
    std::int16_t  dxDigit;
    std::int16_t  yBase;
    std::int16_t  y;
    std::int16_t  dxWarp;
    std::int16_t  pct;
    std::int16_t  fWarp10;
    std::int16_t  iEff;
    COLORREF crFore;
    std::int16_t  cch;
    std::int16_t  x;
    HPEN     hpenSav;
    COLORREF crBack;
    HBRUSH   hbrSav;
    std::int16_t  pctT;
    char     szT[256];
    std::int16_t  dyText;
    char    *psz;
    std::int16_t  dmgFloor;
    std::int16_t  dmgMin;
    std::int16_t  dmgShipRam;
    std::int16_t  iWarp;
    std::int16_t  dmgShip;
    std::int16_t  pctHit;
    std::int16_t  dxLabel;
    std::int16_t  dmgMinRam;
    std::int32_t  lpct;
    std::int16_t  xBase;
    std::int16_t  dxQuan;
    char     ch;
    std::int32_t  ldelta;
    RECT     rcT;

    win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
    win32::PatBlt(hdc, 0, dy - 1, dx, 1, win32::PATCOPY);
    win32::PatBlt(hdc, dx - 1, 0, 1, dy, win32::PATCOPY);
    win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
    win32::PatBlt(hdc, 0, 0, dx, 1, win32::PATCOPY);
    win32::PatBlt(hdc, 0, 0, 1, dy, win32::PATCOPY);
    win32::PatBlt(hdc, 4, dy - app.state.dyArial8 * 2 - 2, dx - 8, 1, win32::PATCOPY);
    win32::SetBkMode(hdc, win32::TRANSPARENT);
    if (!ppart->pcom()) {
        win32::SetRect(&rcData, 5, 5, dx - 5, dy - 5);
        c = TextIO().pfnCchGetString(app, idsNoneAvailable, std::span{app.state.szWork});
        DiaganolTextOut(app, hdc, &rcData, app.state.szWork, c);
    } else {
        win32::SelectObject(hdc, app.lpui->rghfontArial10[1]);
        std::strcpy(app.state.szWork, ppart->pcom().szName());
        CtrTextOut(hdc, dx >> 1, 3, app.state.szWork, 0);
        if (ppart->hs.grhst == hstHull || ppart->hs.grhst == hstSBHull) {
            DrawFleetBitmap(app, nullptr, hdc, 5, app.lpui->dyArial10 + 5, 0, ppart->pcom().ibmp(), 0, 0, -1, 0);
        } else {
            win32::SelectPalette(hdc, app.lpui->vhpal, 0);
            win32::RealizePalette(hdc);
            DibBlt(hdc, 5, app.lpui->dyArial10 + 5, 64, 64, app.lpui->rghdibInventory[ppart->pcom().ibmp() >> 5], (ppart->pcom().ibmp() & 7) * 0x40,
                   ((3 - (ppart->pcom().ibmp() >> 3)) & 3) * 0x40, 64, 64, 13369376);
        }
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        yCur = app.lpui->dyArial10 + 71;
        c = TextIO().pfnCchGetString(app, idsTechReq, std::span{app.state.szWork});
        win32::TextOutA(hdc, 5, yCur, app.state.szWork, c);
        dxStr = 0;
        for (i = 0; i < 6; i++) {
            c = TextIO().pfnCchGetString(app, static_cast<StringId>(idsEner + i), std::span{app.state.szWork});
            dxT = LOWORD(GetTextExtent(hdc, app.state.szWork, c));
            if (dxT > dxStr) {
                dxStr = dxT;
            }
        }
        dxStr += 5;
        xNum = dxStr + LOWORD(GetTextExtent(hdc, "99", 2));
        fReq = 0;
        for (i = 0; i < 6; i++) {
            if (ppart->pcom().rgTech()[i] > 0) {
                yCur += app.state.dyArial8;
                win32::SetTextColor(hdc, ppart->pcom().rgTech()[i] <= app.state.rgplr[app.state.idPlayer].rgTech[i] ? 0 : 127);
                c = TextIO().pfnCchGetString(app, static_cast<StringId>(idsEner + i), std::span{app.state.szWork});
                RightTextOut(app, hdc, dxStr, yCur, app.state.szWork, c, 0);
                win32::SetTextColor(hdc, 0);
                c = win32::wsprintfA(app.state.szWork, app.state.PCTD, ppart->pcom().rgTech()[i]);
                RightTextOut(app, hdc, xNum, yCur, app.state.szWork, c, 0);
                fReq = 1;
            }
        }
        if (!fReq) {
            yCur += app.state.dyArial8;
            c = TextIO().pfnCchGetString(app, idsNone3, std::span{app.state.szWork});
            CtrTextOut(hdc, 37, yCur, app.state.szWork, c);
        }
        if (PartQueryIO().pfnLookup(app, ppart) <= mdPartAvailInvalid) {
            l = -1;
        } else {
            l = CostOfDevelopingItem(app, reinterpret_cast<char *>(ppart->pcom().rgTech()));
        }
        if (l > 99999) {
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsCostLdk), static_cast<std::int32_t>(((l + 500) / 1000)));
            win32::TextOutA(hdc, 5, yCur + app.state.dyArial8 + 4, app.state.szWork, c);
        } else if (l > 0) {
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsCostLd), l);
            win32::TextOutA(hdc, 5, yCur + app.state.dyArial8 + 4, app.state.szWork, c);
        } else if (l == -1) {
            win32::SetTextColor(hdc, 127);
            CtrTextOut(hdc, 37, yCur + app.state.dyArial8 + 4, TextIO().pfnCompressed(app, idsUnavail), 0);
            win32::SetTextColor(hdc, 0);
        } else {
            CtrTextOut(hdc, 37, yCur + app.state.dyArial8 + 4, TextIO().pfnCompressed(app, idsAvailable), 0);
        }
        yStart = app.lpui->dyArial10 + 5;
        if (app.state.dyArial8 * 4 < 64) {
            yStart += (0x40 - (app.state.dyArial8 << 2)) >> 1;
        }
        yCur = yStart;
        dxStr = 0;
        for (i = 0; i <= 5; i++) {
            dxT = LOWORD(GetTextExtent(hdc, app.state.rgszMinerals[i], win32::lstrlenA(app.state.rgszMinerals[i])));
            if (dxT > dxStr) {
                dxStr = dxT;
            }
        }
        dxStr += 77;
        PartQueryIO().pfnTrueCost(app, app.state.idPlayer, ppart, rgCosts);
        for (i = 0; i <= 5; i++) {
            if (i != 4 && i != 3) {
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                win32::SetTextColor(hdc, app.lpui->rgcrMinerals[i]);
                RightTextOut(app, hdc, dxStr, yCur, app.state.rgszMinerals[i], 0, 0);
                win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
                win32::SetTextColor(hdc, app.lpui->crWindowText);
                if (i != 5) {
                    c = rgCosts[i];
                } else {
                    c = rgCosts[3];
                }
                if (ppart->hs.grhst == hstSBHull || ppart->hs.grhst == hstSpecialSB) {
                    c -= c / 2;
                }
                c = win32::wsprintfA(app.state.szWork, app.state.PCTD, c);
                RightTextOut(app, hdc, dxStr + app.lpui->dxMaxMineralQuan, yCur, app.state.szWork, c, 0);
                if (i < 5) {
                    win32::TextOutA(hdc, dxStr + app.lpui->dxMaxMineralQuan, yCur, "kT", 2);
                }
                yCur += app.state.dyArial8;
            }
        }
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        if (ppart->pcom().cMass() != 0) {
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsMassDkt), ppart->pcom().cMass());
            win32::TextOutA(hdc, dxStr + app.lpui->dxMaxMineralQuan + 32, yStart, app.state.szWork, c);
        }
        win32::SetRect(&rcData, 73 <= xNum + 4 ? xNum + 4 : 73, app.lpui->dyArial10 + 71, dx - 5, dy - 5 - app.state.dyArial8 * 2 - 4);
        if (app.state.dyArial8 > 14) {
            rcData.left += 4;
        }
        win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
        win32::PatBlt(hdc, rcData.left, rcData.top, rcData.right - rcData.left, 1, win32::PATCOPY);
        win32::PatBlt(hdc, rcData.left, rcData.top, 1, rcData.bottom - rcData.top, win32::PATCOPY);
        win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
        win32::PatBlt(hdc, rcData.left, rcData.bottom - 1, rcData.right - rcData.left, 1, win32::PATCOPY);
        win32::PatBlt(hdc, rcData.right - 1, rcData.top, 1, rcData.bottom - rcData.top, win32::PATCOPY);
        ExpandRc(&rcData, -3, -3);
        if (app.state.gd.fBleedingEdge && l == 0) {
            win32::SetTextColor(hdc, app.lpui->crButtonHilite);
            c = TextIO().pfnCchGetString(app, idsBleedingEdge, std::span{app.state.szWork});
            DiaganolTextOut(app, hdc, &rcData, app.state.szWork, c);
            win32::SetTextColor(hdc, app.lpui->crWindowText);
        }
        ids = static_cast<StringId>(idsNoString);
        switch (ppart->hs.grhst) {
        default:
            break;
        case hstEngine:
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            rcData.right -= 2;
            c = TextIO().pfnCchGetString(app, idsFuelUsageVsWarpSpeed, std::span{app.state.szWork});
            CtrTextOut(hdc, ((rcData.right - rcData.left) >> 1) + rcData.left, rcData.top, app.state.szWork, c);
            rcData.top += app.state.dyArial8;
            c = TextIO().pfnCchGetString(app, idsWarp, std::span{app.state.szWork});
            dxStr = LOWORD(GetTextExtent(hdc, app.state.szWork, c));
            dxWarp = static_cast<std::int16_t>((rcData.right - rcData.left - dxStr - 8)) / 10;
            dyPct = static_cast<std::int16_t>((rcData.bottom - rcData.top - app.state.dyArial8 - 8)) / 6;
            win32::PatBlt(hdc, rcData.left + dxStr + 6, rcData.top, 1, rcData.bottom - rcData.top - app.state.dyArial8 - 4, win32::BLACKNESS);
            win32::PatBlt(hdc, rcData.left + dxStr + 6, rcData.bottom - app.state.dyArial8 - 4, rcData.right - rcData.left - dxStr - 6, 1, win32::BLACKNESS);
            x = rcData.left + dxStr;
            y = rcData.bottom - app.state.dyArial8 - 4 - 6 * dyPct;
            pct = 800;
            win32::SetTextColor(hdc, 127);
            while (pct >= 25) {
                if (pct == 100) {
                    win32::SetTextColor(hdc, 0);
                    win32::SelectObject(hdc, app.lpui->hbrGray);
                    win32::PatBlt(hdc, x + 4, y, rcData.right - rcData.left - dxStr - 4, 1, win32::PATCOPY);
                }
                c = win32::wsprintfA(app.state.szWork, app.state.PCTDPCTPCT, pct);
                RightTextOut(app, hdc, x, y - (app.state.dyArial8 >> 1), app.state.szWork, c, 0);
                win32::PatBlt(hdc, x + 4, y, 5, 1, win32::BLACKNESS);
                pct >>= 1;
                y += dyPct;
            }
            y = rcData.bottom - app.state.dyArial8;
            RightTextOut(app, hdc, x, y, TextIO().pfnCompressed(app, idsWarp), 0, 0);
            x += 6;
            yBase = rcData.bottom - app.state.dyArial8 - 4;
            dxDigit = LOWORD(GetTextExtent(hdc, "0", 1)) >> 1;
            rgch[0] = '0';
            cch = 1;
            crFore = win32::SetTextColor(hdc, app.lpui->crButtonText);
            switch (ppart->hs.iItem) {
            case iengineInterspace10:
            case iengineTransStar10:
            case iengineTransGalacticMizerScoop:
            case iengineGalaxyScoop:
            case iengineEnigmaPulsar:
                fWarp10 = 1;
                break;
            default:
                fWarp10 = 0;
            }
            for (i = 0; i <= 10; i++) {
                if (i == 10) {
                    cch = 2;
                    rgch[0] = '1';
                    rgch[1] = '0';
                    if (!fWarp10) {
                        hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->rghbrPat[0]));
                        win32::SetTextColor(hdc, 0xffff);
                        crBack = win32::SetBkColor(hdc, app.lpui->crButtonFace);
                        win32::PatBlt(hdc, x - dxWarp + 1, rcData.top, dxWarp, yBase - rcData.top, win32::PATCOPY);
                        win32::SetBkColor(hdc, crBack);
                        win32::SelectObject(hdc, hbrSav);
                    }
                }
                if (ppart->pengine()->rgcFuelUsed[i] <= 120 && ((i == 10 && fWarp10) || ppart->pengine()->rgcFuelUsed[i + 1] > 120 || (i == 9 && !fWarp10))) {
                    win32::SetTextColor(hdc, 8323072);
                } else {
                    win32::SetTextColor(hdc, app.lpui->crButtonText);
                }
                win32::TextOutA(hdc, x - dxDigit * cch, y, rgch, cch);
                if (i > 0) {
                    win32::PatBlt(hdc, x, y - 6, 1, 5, win32::BLACKNESS);
                }
                rgch[0]++;
                x += dxWarp;
            }
            win32::SetTextColor(hdc, crFore);
            hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, app.lpui->hpenDkBlue));
            x = rcData.left + dxStr + 6;
            for (i = 0; i <= 10; i++) {
                y = yBase;
                pct = 25;
                iEff = ppart->pengine()->rgcFuelUsed[i];
                while (iEff >= pct) {
                    pct *= 2;
                    y -= dyPct;
                }
                if (pct != 25) {
                    pctT = win32::MulDiv(iEff - (pct >> 1), 100, pct >> 1);
                    y -= static_cast<std::int16_t>((pctT * dyPct)) / 100;
                } else if (iEff == 0) {
                    y--;
                } else {
                    y -= (((1) > (static_cast<std::int16_t>((iEff * 4 * dyPct)) / 100)) ? (1) : (static_cast<std::int16_t>((iEff * 4 * dyPct)) / 100));
                }
                if (i == 0) {
                    win32::MoveToEx(hdc, x, y, nullptr);
                } else {
                    win32::LineTo(hdc, x, y);
                }
                x += dxWarp;
            }
            win32::SelectObject(hdc, hpenSav);
            switch (ppart->pengine()->grfAbilities) {
            case engineAbilityNone:
            default:
                if (ppart->hs.iItem < iengineSubGalacticFuelScoop || ppart->hs.iItem > iengineTransGalacticMizerScoop)
                    break;
                ids = idsEngineWillUnavailableIfHaveLesserRacial;
                break;
            case engineSettlersDelight:
                ids = idsEngineCanMountedMiniColonizerHullRequires;
                break;
            case engineRadiatingRamScoop:
                ids = idsEngineCreatesPowerfulWavesRadiationWillKill;
                break;
            case engineFuelMizer:
                ids = idsEngineRequiresLesserRacialTraitImprovedFuel;
                break;
            case engineGalaxyScoop:
                ids = idsEngineRequiresLesserRacialTraitImprovedFuel2;
                break;
            case engineInterspace10:
                ids = idsEngineRequiresLesserRacialTraitRamScoop;
                break;
            case engineEnigmaPulsar:
                ids = idsOriginEngineUnknownAdds14Square;
            }
            break;
        case hstScanner:
            ExpandRc(&rcData, -4, -4);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            i = ppart->pscanner()->dRange;
            if (i == 0) {
                c = TextIO().pfnCchGetString(app, idsEnemyFleetsCannotDetectedScannerUnlessSame, std::span{app.state.szWork});
            } else {
                c = TextIO().pfnCchGetString(app, idsEnemyFleetsOrbitingPlanetCanDetectedD, std::span{szT});
                c = win32::wsprintfA(app.state.szWork, szT, i);
            }
            dyText = win32::DrawTextA(hdc, app.state.szWork, c, &rcData, win32::DT_WORDBREAK | win32::DT_NOPREFIX);
            rcData.top += (app.state.dyArial8 >> 1) + dyText;
            i = ppart->pscanner()->grfAbilities;
            if (i == 0) {
                c = TextIO().pfnCchGetString(app, idsScannerCapableDeterminingPlanetsEnvironmentCompo, std::span{app.state.szWork});
            } else if (i != 4) {
                c = TextIO().pfnCchGetString(app, idsScannerCanDeterminePlanetsBasicStatsDistance, std::span{szT});
                c = win32::wsprintfA(app.state.szWork, szT, i == 1 ? 50 : i == 2 ? 100 : 200);
                ids = idsScannerWillUnavailableIfHaveLesserRacial;
            } else {
                if (ppart->hs.iItem == iscannerPickPocketScanner) {
                    ids = idsScannerCapablePenetratingDefensesEnemyFleetsAllo;
                } else if (ppart->hs.iItem == iscannerRobberBaronScanner) {
                    ids = idsScannerCanDeterminePlanetsStatsDistance120;
                } else {
                    ids = idsScannerCanDeterminePlanetsBasicStatsDistance2;
                }
                c = TextIO().pfnCchGetString(app, ids, std::span{app.state.szWork});
                ids = idsScannerRequiresPrimaryRacialTraitSuperStealth;
            }
            dyText = win32::DrawTextA(hdc, app.state.szWork, c, &rcData, win32::DT_WORDBREAK | win32::DT_NOPREFIX);
            break;
        case hstHull:
            ExpandRc(&rcData, -4, -4);
            xText = static_cast<std::int16_t>((rcData.right - rcData.left)) / 2 + rcData.left;
            idsT = idsFuelCapacity;
            switch (ppart->hs.iItem) {
            case ihuldefMiniColonyShip:
            case ihuldefMetaMorph:
                ids = idsHullRequiresPrimaryRacialTraitHyperExpansion;
                break;
            case ihuldefSuperFreighter:
            case ihuldefFuelTransport:
                ids = idsHullRequiresPrimaryRacialTraitInnerStrength;
                break;
            case ihuldefMaxiMiner:
                ids = idsHullUnavailableIfHaveRaceDisadvantageBasic;
                break;
            case ihuldefMidgetMiner:
            case ihuldefMiner:
            case ihuldefUltraMiner:
                ids = idsMiningHullRequiresLesserRacialTraitAdvanced;
                break;
            case ihuldefDreadnought:
            case ihuldefBattleCruiser:
                ids = idsHullRequiresPrimaryRacialTraitWarMonger;
                break;
            case ihuldefRogue:
            case ihuldefStealthBomber:
                ids = idsHullRequiresPrimaryRacialTraitSuperStealth;
                break;
            case ihuldefMiniMineLayer:
            case ihuldefSuperMineLayer:
                ids = idsHullRequiresPrimaryRaceTraitSpaceDemolition;
                break;
            case ihuldefScout:
            case ihuldefFrigate:
            case ihuldefDestroyer:
                ids = idsHullWillHaveBuiltScannerIfJack;
                break;
            case ihuldefMiniMorph:
                ids = idsOriginHullUnknown;
            }
            i = 0;
            while (i < 4) {
                c = TextIO().pfnCchGetString(app, idsT, std::span{app.state.szWork});
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                RightTextOut(app, hdc, xText, rcData.top, app.state.szWork, c, 0);
                win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
                switch (i) {
                case 0:
                    c = win32::wsprintfA(app.state.szWork, "%dmg", ppart->phul()->wtFuelMax);
                    break;
                case 1:
                    c = win32::wsprintfA(app.state.szWork, app.lpui->PCTDKT, ppart->phul()->wtCargoMax);
                    break;
                case 2:
                    c = win32::wsprintfA(app.state.szWork, app.state.PCTD, ppart->phul()->dp);
                    break;
                case 3:
                    c = win32::wsprintfA(app.state.szWork, app.state.PCTD, PartQueryIO().pfnHull(ppart->phul()->ihuldef)->init);
                }
                win32::TextOutA(hdc, xText, rcData.top, app.state.szWork, c);
                i++;
                idsT = static_cast<StringId>(idsT + 1);
                rcData.top += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
            }
            switch (ppart->hs.iItem) {
            case ihuldefFuelTransport:
            case ihuldefSuperFuelXport:
                psz = TextIO().pfnCompressed(app, idsHullWillManufacture200UnitsFuelEach);
                if (ppart->hs.iItem == ihuldefFuelTransport) {
                    pct = 5;
                } else {
                    pct = 10;
                }
                c = win32::wsprintfA(app.state.szWork, psz, pct);
                win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
                xText = rcData.left;
                yText = rcData.top;
                WrapTextOut(app, hdc, &xText, &yText, app.state.szWork, c, xText, rcData.right - rcData.left, nullptr, 0, 1);
                break;
            case ihuldefMiniMineLayer:
            case ihuldefSuperMineLayer:
                c = TextIO().pfnCchGetString(app, idsHullWillDoubleEfficiencyMineLayingPods, std::span{app.state.szWork});
                win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
                xText = rcData.left;
                yText = rcData.top;
                WrapTextOut(app, hdc, &xText, &yText, app.state.szWork, c, xText, rcData.right - rcData.left, nullptr, 0, 1);
            }
            break;
        case hstSBHull:
            ExpandRc(&rcData, -4, -4);
            idsT = idsArmorStrength;
            xText = rcData.left;
            yText = rcData.top;
            switch (ppart->hs.iItem) {
            case isbhullSpaceDock:
            case isbhullUltraStation:
                ids = idsStarbaseHullRequiresLesserRacialTraitImproved;
                break;
            case isbhullDeathStar:
                ids = idsHullRequiresPrimaryRacialTraitAlternateReality;
            }
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            if (ppart->phul()->wtCargoMax != 0) {
                c = TextIO().pfnCchGetString(app, idsStarbaseHullHasSpaceDockCanBuild, std::span{app.state.szWork});
                WrapTextOut(app, hdc, &xText, &yText, app.state.szWork, c, rcData.left, rcData.right - rcData.left, nullptr, 0, 1);
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                yText = 3 * app.state.dyArial8 + rcData.top;
                xText = static_cast<std::int16_t>((rcData.right - rcData.left)) / 2 + rcData.left;
                c = TextIO().pfnCchGetString(app, idsDockCapacity2, std::span{app.state.szWork});
                RightTextOut(app, hdc, xText, yText, app.state.szWork, c, 0);
                win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
                if (ppart->phul()->wtCargoMax != 0xffff) {
                    c = win32::wsprintfA(app.state.szWork, app.lpui->PCTDKT, ppart->phul()->wtCargoMax);
                } else {
                    c = TextIO().pfnCchGetString(app, idsUnlimited, std::span{app.state.szWork});
                }
                win32::TextOutA(hdc, xText, yText, app.state.szWork, c);
            } else {
                c = TextIO().pfnCchGetString(app, idsStarbaseHullDoesHaveSpaceDockCan, std::span{app.state.szWork});
                WrapTextOut(app, hdc, &xText, &yText, app.state.szWork, c, rcData.left, rcData.right - rcData.left, nullptr, 0, 1);
                yText = 3 * app.state.dyArial8 + rcData.top;
            }
            yText += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
            xText = static_cast<std::int16_t>((rcData.right - rcData.left)) / 2 + rcData.left;
            i = 0;
            while (i < 2) {
                c = TextIO().pfnCchGetString(app, idsT, std::span{app.state.szWork});
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                RightTextOut(app, hdc, xText, yText, app.state.szWork, c, 0);
                win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
                if (i == 0) {
                    c = win32::wsprintfA(app.state.szWork, app.state.PCTD, ppart->phul()->dp);
                } else if (i == 1) {
                    c = win32::wsprintfA(app.state.szWork, app.state.PCTD, PartQueryIO().pfnHull(ppart->phul()->ihuldef)->init);
                }
                win32::TextOutA(hdc, xText, yText, app.state.szWork, c);
                i++;
                idsT = static_cast<StringId>(idsT + 1);
                yText += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
            }
            break;
        case hstShield:
            ExpandRc(&rcData, -4, -4);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            xText = rcData.left;
            DxStreamTextOut(hdc, &xText, rcData.top, TextIO().pfnCompressed(app, idsShieldStrength), 0, 1);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            c = win32::wsprintfA(app.state.szWork, app.state.PCTD, ppart->pshield()->dp);
            DxStreamTextOut(hdc, &xText, rcData.top, app.state.szWork, c, 1);
            if (ppart->hs.iItem == ishieldShadowShield) {
                ids = idsArmorShieldRequiresPrimaryRacialTraitSuper;
                idsT = idsShieldDecreasesRangeWhichEnemyShipsCan;
            LShieldDisp:
                c = TextIO().pfnCchGetString(app, idsT, std::span{app.state.szWork});
                rcData.top += app.state.dyArial8 + 4;
                win32::DrawTextA(hdc, app.state.szWork, c, &rcData, win32::DT_WORDBREAK | win32::DT_NOPREFIX);
                break;
            } else if (ppart->hs.iItem == ishieldCrobySharmor) {
                ids = idsShieldRequiresPrimaryRacialTraitInnerStrength;
                idsT = idsShieldAlsoContainsArmorComponentWhichWill;
                goto LShieldDisp;
            } else if (ppart->hs.iItem == ishieldLangstonShell) {
                ids = idsOriginPartUnknown;
                idsT = idsShieldAlsoProvides65dpArmor5Jamming;
                goto LShieldDisp;
            }
            break;

        case hstArmor:
            ExpandRc(&rcData, -4, -4);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            xText = rcData.left;
            DxStreamTextOut(hdc, &xText, rcData.top, TextIO().pfnCompressed(app, idsArmorStrength2), 0, 1);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            c = win32::wsprintfA(app.state.szWork, app.state.PCTD, ppart->parmor()->dp);
            DxStreamTextOut(hdc, &xText, rcData.top, app.state.szWork, c, 1);
            if (ppart->hs.iItem == iarmorDepletedNeutronium) {
                ids = idsArmorShieldRequiresPrimaryRacialTraitSuper;
                idsT = idsArmorDecreasesRangeWhichEnemyShipsCan;
            LArmDisp:
                c = TextIO().pfnCchGetString(app, idsT, std::span{app.state.szWork});
                rcData.top += app.state.dyArial8 + 4;
                win32::DrawTextA(hdc, app.state.szWork, c, &rcData, win32::DT_WORDBREAK | win32::DT_NOPREFIX);
                break;
            } else if (ppart->hs.iItem == iarmorFieldedKelarium) {
                ids = idsArmorRequiresPrimaryRacialTraitInnerStrength;
                idsT = idsArmorAlsoActsPartShieldWhichWill;
                goto LArmDisp;
            } else if (ppart->hs.iItem == iarmorMegaPolyShell) {
                ids = idsOriginPartUnknown;
                c = TextIO().pfnCchGetString(app, idsPartAlsoActs100dpShield20Cloak, std::span{app.state.szWork});
                rcData.top += app.state.dyArial8 + 4;
                win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
                win32::DrawTextA(hdc, app.state.szWork, c, &rcData, win32::DT_WORDBREAK | win32::DT_NOPREFIX);
            }
            break;

        case hstBeam:
            ExpandRc(&rcData, -4, -4);
            xText = static_cast<std::int16_t>((rcData.right - rcData.left)) / 3 + rcData.left;
            yText = rcData.top;
            c = TextIO().pfnCchGetString(app, idsPower, std::span{app.state.szWork});
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            RightTextOut(app, hdc, xText, yText, app.state.szWork, c, 0);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            c = win32::wsprintfA(app.state.szWork, app.state.PCTD, ppart->pbeam()->dp);
            win32::TextOutA(hdc, xText, yText, app.state.szWork, c);
            yText += app.state.dyArial8;
            c = TextIO().pfnCchGetString(app, idsRange, std::span{app.state.szWork});
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            RightTextOut(app, hdc, xText, yText, app.state.szWork, c, 0);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            c = win32::wsprintfA(app.state.szWork, app.state.PCTD, ppart->pbeam()->dRangeMax);
            win32::TextOutA(hdc, xText, yText, app.state.szWork, c);
            yText += app.state.dyArial8;
            c = TextIO().pfnCchGetString(app, idsInitiative, std::span{app.state.szWork});
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            RightTextOut(app, hdc, xText, yText, app.state.szWork, c, 0);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            c = win32::wsprintfA(app.state.szWork, app.state.PCTD, ppart->pbeam()->init);
            win32::TextOutA(hdc, xText, yText, app.state.szWork, c);
            yText += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
            rcData.top = yText;
            if (ppart->pbeam()->grfAbilities != 0) {
                win32::SelectObject(hdc, app.lpui->rghfontArial7[0]);
                if (ppart->pbeam()->grfAbilities & beamSapper) {
                    c = TextIO().pfnCchGetString(app, idsWeaponWillDamageShieldsHasEffectArmor, std::span{app.state.szWork});
                    rcData.top += win32::DrawTextA(hdc, app.state.szWork, c, &rcData, win32::DT_WORDBREAK | win32::DT_NOPREFIX);
                }
                if (ppart->pbeam()->grfAbilities & beamGatling) {
                    c = TextIO().pfnCchGetString(app, idsWeaponHitsTargetsRangeEachTimeFired, std::span{app.state.szWork});
                    rcData.top += win32::DrawTextA(hdc, app.state.szWork, c, &rcData, win32::DT_WORDBREAK | win32::DT_NOPREFIX);
                    c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsWeaponAlsoMakesExcellentMineSweeperCapable), ppart->pbeam()->dp * 16);
                    win32::DrawTextA(hdc, app.state.szWork, c, &rcData, win32::DT_WORDBREAK | win32::DT_NOPREFIX);
                }
                win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            } else if (ppart->hs.iItem == ibeamMultiContainedMunition) {
                ids = idsOriginPartUnknown;
                c = TextIO().pfnCchGetString(app, idsPartAlsoActs10CloakIncreasesTorpedo, std::span{app.state.szWork});
                win32::SelectObject(hdc, app.lpui->rghfontArial7[0]);
                rcData.top += win32::DrawTextA(hdc, app.state.szWork, c, &rcData, win32::DT_WORDBREAK | win32::DT_NOPREFIX);
                c = TextIO().pfnCchGetString(app, idsWeaponCanAlsoBombPlanets2Colonists, std::span{app.state.szWork});
                win32::DrawTextA(hdc, app.state.szWork, c, &rcData, win32::DT_WORDBREAK | win32::DT_NOPREFIX);
            }
            switch (ppart->hs.iItem) {
            case ibeamMiniGun:
                ids = idsPartRequiresPrimaryRacialTraitInnerStrength;
                break;
            case ibeamGatlingNeutrinoCannon:
            case ibeamBlunderbuss:
                ids = idsPartRequiresPrimaryRacialTraitWarMonger;
            }
            break;
        case hstTorp:
            ExpandRc(&rcData, -4, -4);
            xText = static_cast<std::int16_t>((rcData.right - rcData.left)) / 3 + rcData.left;
            yText = rcData.top;
            c = TextIO().pfnCchGetString(app, idsPower, std::span{app.state.szWork});
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            RightTextOut(app, hdc, xText, yText, app.state.szWork, c, 0);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            c = win32::wsprintfA(app.state.szWork, app.state.PCTD, ppart->ptorp()->dp);
            win32::TextOutA(hdc, xText, yText, app.state.szWork, c);
            yText += app.state.dyArial8;
            c = TextIO().pfnCchGetString(app, idsRange, std::span{app.state.szWork});
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            RightTextOut(app, hdc, xText, yText, app.state.szWork, c, 0);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            c = win32::wsprintfA(app.state.szWork, app.state.PCTD, ppart->ptorp()->dRangeMax);
            win32::TextOutA(hdc, xText, yText, app.state.szWork, c);
            yText += app.state.dyArial8;
            c = TextIO().pfnCchGetString(app, idsInitiative, std::span{app.state.szWork});
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            RightTextOut(app, hdc, xText, yText, app.state.szWork, c, 0);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            c = win32::wsprintfA(app.state.szWork, app.state.PCTD, ppart->ptorp()->init);
            win32::TextOutA(hdc, xText, yText, app.state.szWork, c);
            yText += app.state.dyArial8;
            c = TextIO().pfnCchGetString(app, idsAccuracy, std::span{app.state.szWork});
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            RightTextOut(app, hdc, xText, yText, app.state.szWork, c, 0);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            c = win32::wsprintfA(app.state.szWork, app.state.PCTD, ppart->ptorp()->dHitChance);
            win32::TextOutA(hdc, xText, yText, app.state.szWork, c);
            yText += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
            if (ppart->hs.iItem == itorpAntiMatterTorpedo) {
                ids = idsOriginPartUnknown;
                break;
            }
            if (ppart->hs.iItem < itorpJihadMissile || ppart->hs.iItem > itorpArmageddonMissile)
                break;
            ids = idsCapitalShipMissilesDoTwiceStatedDamage;
            break;
        case hstBomb:
            ExpandRc(&rcData, -4, -4);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            xText = rcData.left;
            yText = rcData.top;
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            if (ppart->hs.iItem == ibombRetroBomb) {
                c = TextIO().pfnCchGetString(app, idsBombDoesKillColonistsDestroyInstallationsBomb, std::span{app.state.szWork});
            } else if (ppart->pbomb()->dDmgCol == 0) {
                c = TextIO().pfnCchGetString(app, idsBombWillKillAnyPlanetsPopulation, std::span{app.state.szWork});
            } else {
                TextIO().pfnCchGetString(app, idsBombWillKillApproximatelyDDPlanets, std::span{app.state.szWork});
                c = win32::wsprintfA(szT, app.state.szWork, ppart->pbomb()->dDmgCol / 10, ppart->pbomb()->dDmgCol % 10);
                if (ppart->hs.iItem >= ibombLadyFingerBomb && ppart->hs.iItem <= ibombCherryBomb) {
                    dmgFloor = 3;
                } else if (ppart->hs.iItem >= ibombSmartBomb && ppart->hs.iItem <= ibombAnnihilatorBomb) {
                    dmgFloor = 999;
                } else {
                    dmgFloor = 0;
                }
                if (dmgFloor > 0) {
                    TextIO().pfnCchGetString(app, dmgFloor == 999 ? idsSmartBombsStrictlyAdditiveHaveMinimumKill : idsIfPlanetHasDefensesBombGuaranteedKill, std::span{app.state.szWork});
                    c += win32::wsprintfA(std::span{szT}.subspan(static_cast<std::size_t>(c)), app.state.szWork, 100 * dmgFloor);
                }
                std::strcpy(app.state.szWork, szT);
            }
            WrapTextOut(app, hdc, &xText, &yText, app.state.szWork, c, rcData.left, rcData.right - rcData.left, nullptr, 0, 1);
            xText = rcData.left;
            yText += app.state.dyArial8 >> 1;
            if (ppart->hs.iItem == ibombRetroBomb) {
                ids = idsPartRequiresPrimaryRacialTraitClaimAdjuster;
                break;
            }
            if (ppart->pbomb()->dDmgBldg == 0) {
                c = TextIO().pfnCchGetString(app, idsBombWillDamagePlanetsMinesFactories, std::span{app.state.szWork});
            } else {
                TextIO().pfnCchGetString(app, idsBombWillDestroyApproximatelyDPlanetsMines, std::span{szT});
                c = win32::wsprintfA(app.state.szWork, szT, ppart->pbomb()->dDmgBldg);
            }
            WrapTextOut(app, hdc, &xText, &yText, app.state.szWork, c, rcData.left, rcData.right - rcData.left, nullptr, 1, 1);
            if (ppart->hs.iItem >= ibombSmartBomb && ppart->hs.iItem <= ibombAnnihilatorBomb) {
                ids = idsBombWillAvailableIfPrimaryRaceTrait;
                break;
            }
            if (ppart->hs.iItem != ibombHushABoom)
                break;
            ids = idsOriginPartUnknown;
            break;
        case hstSpecialE:
            ExpandRc(&rcData, -4, -4);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            xText = rcData.left;
            yText = rcData.top;
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            switch (ppart->hs.iItem) {
            default:
                idsT = static_cast<StringId>(idsNoString);
                break;
            case ispecialETransportCloaking:
                idsT = idsCloaksUnarmedHullsReducingRangeWhichScanners;
                ids = idsCloakRequiresPrimaryRacialTraitSuperStealth;
                break;
            case ispecialEStealthCloak:
            case ispecialESuperStealthCloak:
            case ispecialEUltraStealthCloak:
                if (ppart->hs.iItem == ispecialEUltraStealthCloak) {
                    ids = idsCloakRequiresPrimaryRacialTraitSuperStealth;
                    idsT = idsWeapons;
                } else {
                    idsT = ppart->hs.iItem == ispecialEStealthCloak ? idsWarningColonizeMissionCannotCarriedBecauseNone
                                                                    : idsScannerCanDeterminePlanetsStatsDistance120;
                }
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsCloaksAnyShipReducingRangeWhichScanners), idsT);
                goto PrintSpecial;
            case ispecialEMultiFunctionPod:
                idsT = idsCloaksAnyShip30Acts10Jammer;
                ids = idsOriginPartUnknown;
                break;
            case ispecialEEnergyDampener:
                ids = idsDeviceRequiresPrimaryRacialTraitSpaceDemolition;
                idsT = idsSlowsShipsCombat1SquareMovement;
                break;
            case ispecialETachyonDetector:
                ids = idsDeviceRequiresPrimaryRacialTraitInnerStrength;
                idsT = idsReducesEffectivenessOtherPlayersCloaks5;
                break;
            case ispecialEAntiMatterGenerator:
                ids = idsDeviceRequiresPrimaryRacialTraitInterstellarTrav;
                idsT = idsActs200mgAntiMatterFuelTankGenerates;
                break;
            case ispecialEFluxCapacitor:
                ids = idsDeviceRequiresPrimaryRacialTraitHyperExpansion;
                /* fallthrough */
            case ispecialEEnergyCapacitor:
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsIncreasesDamageDoneBeamWeaponsShipD), ppart->hs.iItem == ispecialEEnergyCapacitor ? 10 : 20);
                goto PrintSpecial;
            case ispecialEJammer10:
            case ispecialEJammer50:
                ids = idsJammingDeviceRequiresPrimaryRacialTraitInner;
                /* fallthrough */
            case ispecialEJammer20:
            case ispecialEJammer30:
                idsT = idsHasDChanceDeflectingIncomingTorpedoesDeflected;
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsT), ppart->pspecial()->grAbility);
                goto PrintSpecial;
            case ispecialEBattleComputer:
            case ispecialEBattleSuperComputer:
            case ispecialEBattleNexus:
                idsT = idsModuleIncreasesAccuracyTorpedoesDIncreasesInitia;
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsT), ppart->pspecial()->grAbility, ppart->hs.iItem - 4);
                goto PrintSpecial;
            }
            if (idsT == static_cast<StringId>(idsNoString))
                break;
            c = TextIO().pfnCchGetString(app, idsT, std::span{app.state.szWork});
        PrintSpecial:
            WrapTextOut(app, hdc, &xText, &yText, app.state.szWork, c, rcData.left, rcData.right - rcData.left, nullptr, 0, 1);
            xText = rcData.left;
            yText += app.state.dyArial8 * 2;
            break;
        case hstSpecialM:
            ExpandRc(&rcData, -4, -4);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            xText = rcData.left;
            yText = rcData.top;
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            switch (ppart->hs.iItem) {
            default:
                idsT = static_cast<StringId>(idsNoString);
                break;
            case ispecialMManeuveringJet:
            case ispecialMOverthruster:
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsIncreasesSpeedBattle1DSquareMovement), ppart->hs.iItem == ispecialMManeuveringJet ? 4 : 2);
                goto PrintSpecial;
            case ispecialMFuelTank:
            case ispecialMSuperFuelTank:
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsPodIncreasesFuelCapacityShipDmg), ppart->hs.iItem == ispecialMFuelTank ? 250 : 500);
                goto PrintSpecial;
            case ispecialMCargoPod:
            case ispecialMSuperCargoPod:
                c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsPodIncreasesCargoCapacityShipDkt), ppart->hs.iItem == ispecialMCargoPod ? 50 : 100);
                goto PrintSpecial;
            case ispecialMMultiCargoPod:
                idsT = idsPodIncreasesCargoCapacityShip250ktProvides;
                ids = idsOriginPartUnknown;
                break;
            case ispecialMJumpGate:
                idsT = idsDeviceAllowsShipJumpPlanetaryStargatesRange;
                ids = idsOriginPartUnknown;
                break;
            case ispecialMColonizationModule:
                idsT = idsPodAllowsShipColonizePlanetWillDismantle;
                ids = idsPartAvailableAlternateRealityRaces;
                break;
            case ispecialMOrbitalConstructionModule:
                idsT = idsModuleContainsEmptyOrbitalHullWhichCan;
                ids = idsPartRequiresPrimaryRacialTraitAlternateReality;
                break;
            case ispecialMBeamDeflector:
                idsT = idsDeflectorDecreasesDamageDoneBeamWeaponsShip;
                break;
            }
            if (idsT == static_cast<StringId>(idsNoString))
                break;
            c = TextIO().pfnCchGetString(app, idsT, std::span{app.state.szWork});
            goto PrintSpecial;
        case hstSpecialSB:
            ExpandRc(&rcData, -4, -4);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            xText = rcData.left;
            yText = rcData.top;
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            if (ppart->hs.iItem <= ispecialSBUltraDriver13) {
                switch (ppart->hs.iItem) {
                case ispecialSBStargateAny300:
                case ispecialSBStargate100Any:
                case ispecialSBStargateAny800:
                case ispecialSBStargateAnyAny:
                    ids = idsStargateRequiresPrimaryRacialTraitInterstellarTr;
                    idsT = idsAllowsFleetsWithoutCargoJumpAnyOther;
                    break;
                case ispecialSBStargate100250:
                case ispecialSBStargate150600:
                case ispecialSBStargate300500:
                    ids = idsStargatesAvailableIfPrimaryRaceTraitHyper;
                    idsT = idsAllowsFleetsWithoutCargoJumpAnyOther;
                    break;
                case ispecialSBMassDriver5:
                case ispecialSBMassDriver6:
                case ispecialSBSuperDriver8:
                case ispecialSBSuperDriver9:
                case ispecialSBUltraDriver11:
                case ispecialSBUltraDriver12:
                case ispecialSBUltraDriver13:
                    ids = idsMassDriverRequiresPrimaryRacialTraitPacket;
                    /* fallthrough */
                case ispecialSBMassDriver7:
                case ispecialSBUltraDriver10:
                    idsT = idsAllowsPlanetsFlingMineralPacketsOtherPlanets;
                }
            } else {
                idsT = static_cast<StringId>(idsNoString);
            }
            c = TextIO().pfnCchGetString(app, idsT, std::span{app.state.szWork});
            WrapTextOut(app, hdc, &xText, &yText, app.state.szWork, c, rcData.left, rcData.right - rcData.left, nullptr, 0, 1);
            xText = rcData.left;
            yText += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
            if (idsT != idsAllowsFleetsWithoutCargoJumpAnyOther) {
                if (idsT == idsAllowsPlanetsFlingMineralPacketsOtherPlanets) {
                    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                    DxStreamTextOut(hdc, &xText, yText, TextIO().pfnCompressed(app, idsWarp), 0, 1);
                    win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
                    c = win32::wsprintfA(app.state.szWork, app.state.PCTD, ppart->pspecialsb()->grAbility);
                    DxStreamTextOut(hdc, &xText, yText, app.state.szWork, c, 1);
                    xText = rcData.left;
                    yText += app.state.dyArial8;
                    idsT = idsWarningReceivingPlanetMustHaveMassDriver;
                    if (ppart->hs.iItem > ispecialSBMassDriver5) {
                        idsT = static_cast<StringId>(idsT + 1);
                    }
                    c = TextIO().pfnCchGetString(app, idsT, std::span{app.state.szWork});
                }
            } else {
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                DxStreamTextOut(hdc, &xText, yText, TextIO().pfnCompressed(app, idsSafeHullMass), 0, 1);
                win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
                if (ppart->pspecialsb()->grAbility == -1) {
                    c = TextIO().pfnCchGetString(app, idsUnlimited, std::span{app.state.szWork});
                } else {
                    c = win32::wsprintfA(app.state.szWork, app.lpui->PCTDKT, ppart->pspecialsb()->grAbility);
                }
                DxStreamTextOut(hdc, &xText, yText, app.state.szWork, c, 1);
                xText = rcData.left;
                yText += app.state.dyArial8;
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                DxStreamTextOut(hdc, &xText, yText, TextIO().pfnCompressed(app, idsSafeRange), 0, 1);
                win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
                if (ppart->pspecialsb()->grAbility2 == -1) {
                    c = TextIO().pfnCchGetString(app, idsUnlimited, std::span{app.state.szWork});
                } else {
                    c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDLightYears), ppart->pspecialsb()->grAbility2);
                }
                DxStreamTextOut(hdc, &xText, yText, app.state.szWork, c, 1);
                if (ppart->pspecialsb()->grAbility == -1) {
                    if (ppart->pspecialsb()->grAbility2 == -1) {
                        idsT = static_cast<StringId>(idsNoString);
                    } else {
                        c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsWarningShipsCanSuccessfullyGatedDL), 5 * ppart->pspecialsb()->grAbility2);
                    }
                } else if (ppart->pspecialsb()->grAbility2 == -1) {
                    c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsWarningShipsDktCanSuccessfullyGatedExceeding), 5 * ppart->pspecialsb()->grAbility);
                } else {
                    c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsWarningShipsDktMightSuccessfullyGatedD), 5 * ppart->pspecialsb()->grAbility,
                                 5 * ppart->pspecialsb()->grAbility2);
                }
            }
            if (idsT == static_cast<StringId>(idsNoString))
                break;
            xText = rcData.left;
            yText += (app.state.dyArial8 > 14 ? 0 : 4) + app.state.dyArial8;
            win32::SelectObject(hdc, app.lpui->rghfontArial7[0]);
            win32::SetTextColor(hdc, 127);
            WrapTextOut(app, hdc, &xText, &yText, app.state.szWork, c, rcData.left, rcData.right - rcData.left, nullptr, 0, 1);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            win32::SetTextColor(hdc, 0);
            break;
        case hstMines:
            dxLabel = -1;
            ExpandRc(&rcData, -4, -4);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            xText = rcData.left;
            yText = rcData.top;
            rcData.top += 5 * app.state.dyArial8;
            for (i = 4; i >= 0; i--) {
                rcData.top -= app.state.dyArial8;
                c = TextIO().pfnCchGetString(app, static_cast<StringId>(idsMinesLaidPerYear + i), std::span{app.state.szWork});
                if (dxLabel == -1) {
                    dxLabel = LOWORD(GetTextExtent(hdc, app.state.szWork, c));
                }
                RightTextOut(app, hdc, rcData.left + dxLabel, rcData.top, app.state.szWork, c, 0);
            }
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            if (ppart->hs.iItem <= iminesSpeedTrap50) {
                switch (ppart->hs.iItem) {
                case iminesMineDispenser40:
                case iminesMineDispenser50:
                case iminesMineDispenser80:
                case iminesMineDispenser130:
                    iWarp = 4;
                    pctHit = 3;
                    dmgShip = 100;
                    dmgShipRam = 125;
                    dmgMin = 500;
                    dmgMinRam = 600;
                    if (ppart->hs.iItem != iminesMineDispenser50) {
                        ids = idsMineRequiresPrimaryRacialTraitSpaceDemolition;
                        break;
                    }
                    ids = idsPartUnavailbleWarMonger;
                    break;
                case iminesHeavyDispenser50:
                case iminesHeavyDispenser110:
                case iminesHeavyDispenser200:
                    iWarp = 6;
                    pctHit = 10;
                    dmgShip = 500;
                    dmgShipRam = 600;
                    dmgMin = 2000;
                    dmgMinRam = 2500;
                    ids = idsMineRequiresPrimaryRacialTraitSpaceDemolition;
                    break;
                case iminesSpeedTrap20:
                case iminesSpeedTrap30:
                case iminesSpeedTrap50:
                    iWarp = 5;
                    pctHit = 35;
                    dmgShip = 0;
                    dmgShipRam = 0;
                    dmgMin = 0;
                    dmgMinRam = 0;
                    if (ppart->hs.iItem != iminesSpeedTrap20) {
                        ids = idsMineRequiresPrimaryRacialTraitSpaceDemolition;
                    } else {
                        ids = idsMineRequiresPrimaryRacialTraitSpaceDemolition2;
                    }
                }
            }
            c = win32::wsprintfA(app.state.szWork, app.state.PCTD, 10 * ppart->pmines()->grAbility);
            win32::TextOutA(hdc, rcData.left + dxLabel + 4, rcData.top, app.state.szWork, c);
            rcData.top += app.state.dyArial8;
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsWarpD2), iWarp);
            win32::TextOutA(hdc, rcData.left + dxLabel + 4, rcData.top, app.state.szWork, c);
            rcData.top += app.state.dyArial8;
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDD2), pctHit / 10, pctHit % 10);
            win32::TextOutA(hdc, rcData.left + dxLabel + 4, rcData.top, app.state.szWork, c);
            rcData.top += app.state.dyArial8;
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDDEngine), dmgShip, dmgShipRam);
            win32::TextOutA(hdc, rcData.left + dxLabel + 4, rcData.top, app.state.szWork, c);
            rcData.top += app.state.dyArial8;
            c = win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsDD3), dmgMin, dmgMinRam);
            win32::TextOutA(hdc, rcData.left + dxLabel + 4, rcData.top, app.state.szWork, c);
            rcData.top += app.state.dyArial8;
            win32::SelectObject(hdc, app.lpui->rghfontArial7[0]);
            c = TextIO().pfnCchGetString(app, idsNumbersParenthesisFleetsContainingShipRamScoop, std::span{app.state.szWork});
            win32::DrawTextA(hdc, app.state.szWork, c, &rcData, win32::DT_WORDBREAK | win32::DT_NOPREFIX);
            break;
        case hstMining:
            ExpandRc(&rcData, -4, -4);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            xText = rcData.left;
            yText = rcData.top;
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            if (ppart->hs.iItem > iminingOrbitalAdjuster)
                break;
            switch (ppart->hs.iItem) {
            case iminingRoboMidgetMiner:
            case iminingRoboUltraMiner:
                ids = idsRobotMinerRequiresLesserRacialTraitAdvanced;
                /* fallthrough */
            case iminingRoboMiner:
            case iminingRoboMaxiMiner:
            case iminingRoboSuperMiner:
                if (ids == static_cast<StringId>(idsNoString)) {
                    ids = idsRobotMinerWillAvailableIfLesserRacial;
                }
                /* fallthrough */
            case iminingRoboMiniMiner:
            case iminingAlienMiner:
                c = TextIO().pfnCchGetString(app, idsModuleContainsRobotsCapableMining, std::span{app.state.szWork});
                c += win32::wsprintfA(std::span{app.state.szWork}.subspan(static_cast<std::size_t>(c)), app.state.PCTD, ppart->pmining()->grAbility);
                c += TextIO().pfnCchGetString(app, idsKtEachMineralDependingConcentrationUninhabitedPl, std::span{app.state.szWork}.subspan(static_cast<std::size_t>(c)));
                if (ppart->hs.iItem != iminingAlienMiner)
                    break;
                ids = idsOriginPartUnknown;
                c += TextIO().pfnCchGetString(app, idsModuleAlsoActs30Cloak30Jammer, std::span{app.state.szWork}.subspan(static_cast<std::size_t>(c)));
                break;
            case iminingOrbitalAdjuster:
                ids = idsPartRequiresPrimaryRacialTraitClaimAdjuster;
                c = TextIO().pfnCchGetString(app, idsModifiedMiningRobotTerraformsInhabitedPlanets1, std::span{app.state.szWork});
            }
            goto PrintSpecial;
        case hstTerra:
            ExpandRc(&rcData, -4, -4);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            xText = rcData.left;
            yText = rcData.top;
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            c = 0;
            if (ppart->hs.iItem <= iterraTotalTerraform30) {
                ids = idsTotalTerraformingRequiresLesserRacialTraitTotal;
                c = TextIO().pfnCchGetString(app, idsAllowsModifyAnyPlanetsThreeEnvironmentVariables, std::span{szT});
                c = win32::wsprintfA(app.state.szWork, szT, ppart->pterra()->grAbility);
            } else {
                c = TextIO().pfnCchGetString(app, idsAllowsModifyPlanetsSDOriginalValue, std::span{szT});
                c = win32::wsprintfA(app.state.szWork, szT, app.state.rgszPlanetAttr[static_cast<std::int16_t>((ppart->hs.iItem - 8)) / 4], ppart->pterra()->grAbility);
            }
            if (c <= 0)
                break;
            WrapTextOut(app, hdc, &xText, &yText, app.state.szWork, c, rcData.left, rcData.right - rcData.left, nullptr, 0, 1);
            break;
        case hstPlanetary:
            ExpandRc(&rcData, -4, -4);
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            xText = rcData.left;
            yText = rcData.top;
            win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            c = 0;
            if (ppart->hs.iItem >= iplanetarySDI && ppart->hs.iItem <= iplanetaryNeutronShield) {
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                rcData.right -= 2;
                c = TextIO().pfnCchGetString(app, idsShieldCoverageVsDefenseQuan, std::span{app.state.szWork});
                CtrTextOut(hdc, ((rcData.right - rcData.left) >> 1) + rcData.left, rcData.top, app.state.szWork, c);
                rcData.top += app.state.dyArial8;
                cch = TextIO().pfnCchGetString(app, idsNum, std::span{szT});
                dxStr = LOWORD(GetTextExtent(hdc, szT, cch));
                dxQuan = static_cast<std::int16_t>((rcData.right - rcData.left - dxStr - 10)) / 5;
                dyPct = static_cast<std::int16_t>((rcData.bottom - rcData.top - app.state.dyArial8 - 8)) / 5;
                win32::PatBlt(hdc, rcData.left + dxStr + 6, rcData.top, 1, rcData.bottom - rcData.top - app.state.dyArial8 - 4, win32::BLACKNESS);
                win32::PatBlt(hdc, rcData.left + dxStr + 6, rcData.bottom - app.state.dyArial8 - 4, rcData.right - rcData.left - dxStr - 6, 1, win32::BLACKNESS);
                x = rcData.left + dxStr;
                y = rcData.bottom - app.state.dyArial8 - 4 - 5 * dyPct;
                win32::SetTextColor(hdc, 8323072);
                cch = TextIO().pfnCchGetString(app, idsStandard, std::span{app.state.szWork});
                RightTextOut(app, hdc, rcData.right, rcData.bottom - 3 - 3 * app.state.dyArial8, app.state.szWork, cch, 0);
                win32::SetTextColor(hdc, 127);
                cch = TextIO().pfnCchGetString(app, idsSmart, std::span{app.state.szWork});
                RightTextOut(app, hdc, rcData.right, rcData.bottom - 4 - app.state.dyArial8 * 2, app.state.szWork, cch, 0);
                pct = 100;
                win32::SetTextColor(hdc, 0);
                while (pct > 0) {
                    c = win32::wsprintfA(app.state.szWork, app.state.PCTDPCTPCT, pct);
                    RightTextOut(app, hdc, x, y - (app.state.dyArial8 >> 1), app.state.szWork, c, 0);
                    win32::PatBlt(hdc, x + 4, y, 5, 1, win32::BLACKNESS);
                    pct -= 20;
                    y += dyPct;
                }
                y = rcData.bottom - app.state.dyArial8;
                RightTextOut(app, hdc, x, y, szT, std::strlen(szT), 0);
                x += 6;
                dxDigit = LOWORD(GetTextExtent(hdc, "0", 1));
                ch = '0';
                for (i = 0; i <= 5; i++) {
                    cch = win32::wsprintfA(app.state.szWork, app.state.PCTD, 20 * i);
                    win32::TextOutA(hdc, x - (i == 0 ? dxDigit >> 1 : i == 5 ? static_cast<std::int16_t>((3 * dxDigit)) / 2 : dxDigit), y, app.state.szWork, cch);
                    if (i > 0) {
                        win32::PatBlt(hdc, x, y - 6, 1, 5, win32::BLACKNESS);
                    }
                    ch++;
                    x += dxQuan;
                }
                hpenSav = static_cast<HPEN>(win32::SelectObject(hdc, app.lpui->hpenDkBlue));
                xBase = rcData.left + dxStr + 6;
                yBase = rcData.bottom - app.state.dyArial8 - 4;
                ldelta = static_cast<std::int16_t>((1000 - ppart->pplanetary()->grAbility));
                for (c = 0; c < 2; c++) {
                    lpct = 1000000;
                    for (i = 0; i <= 100; i++) {
                        x = win32::MulDiv(i, dxQuan, 20) + xBase;
                        y = yBase;
                        y -= LOWORD(static_cast<std::int32_t>(((1000000 - lpct) * dyPct)) / 0x30d40);
                        lpct = static_cast<std::int32_t>((lpct * ldelta)) / 1000;
                        if (i == 0) {
                            win32::MoveToEx(hdc, x, y, nullptr);
                        } else {
                            win32::LineTo(hdc, x, y);
                        }
                    }
                    ldelta = static_cast<std::int16_t>((1000 - ppart->pplanetary()->grAbility / 2));
                    win32::SelectObject(hdc, app.lpui->hpenRadar);
                }
                win32::SelectObject(hdc, hpenSav);
                c = 0;
                if (ppart->hs.iItem <= iplanetaryMissileBattery) {
                    ids = idsPlanetaryScannersDefensesAvailableAlternateReali;
                } else {
                    ids = idsPlanetaryDefenseUnavailablePrimaryRacialTraitWar;
                }
            } else if (ppart->hs.iItem >= iplanetaryViewer50 && ppart->hs.iItem <= iplanetarySnooper620X) {
                i = ppart->pplanetary()->grAbility;
                c = TextIO().pfnCchGetString(app, idsEnemyFleetsOrbitingPlanetCanDetectedD, std::span{szT});
                c = win32::wsprintfA(app.state.szWork, szT, std::abs(i));
                if (i < 0) {
                    WrapTextOut(app, hdc, &xText, &yText, app.state.szWork, c, rcData.left, rcData.right - rcData.left, nullptr, 0, 1);
                    xText = rcData.left;
                    yText += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
                    c = TextIO().pfnCchGetString(app, idsScannerCanDeterminePlanetsBasicStatsDistance, std::span{szT});
                    c = win32::wsprintfA(app.state.szWork, szT, -i >> 1);
                    ids = idsScannerWillUnavailableIfHaveLesserRacial;
                } else {
                    ids = idsPlanetaryScannersDefensesAvailableAlternateReali;
                }
            } else if (ppart->hs.iItem == iplanetaryGenesisDevice) {
                ids = idsOriginProcessUnknown;
                c = TextIO().pfnCchGetString(app, idsProcessGivesPlanetNewBirthTracesCivilization, std::span{app.state.szWork});
            }
            if (c > 0) {
                WrapTextOut(app, hdc, &xText, &yText, app.state.szWork, c, rcData.left, rcData.right - rcData.left, nullptr, 0, 1);
            }
        }
        if (ids != static_cast<StringId>(idsNoString)) {
            c = TextIO().pfnCchGetString(app, ids, std::span{app.state.szWork});
            win32::SetRect(&rcT, 4, dy - app.state.dyArial8 * 2, dx - 4, dy - 5);
            if (app.state.dyArial8 > 14) {
                win32::SelectObject(hdc, app.lpui->rghfontArial6[0]);
            } else {
                win32::SelectObject(hdc, app.lpui->rghfontArial7[0]);
            }
            win32::SetTextColor(hdc, l == -1 ? 127 : 0);
            win32::DrawTextA(hdc, app.state.szWork, c, &rcT, win32::DT_WORDBREAK | win32::DT_NOPREFIX);
            if (l == -1) {
                win32::SetTextColor(hdc, 0);
            }
        }
    }
    return;
}
