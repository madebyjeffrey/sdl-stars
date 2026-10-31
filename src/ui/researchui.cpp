#include "win.h"
#include "text_io.h"
#include "control_io.h"
#include "presentation_io.h"
#include "research_io.h"
#include "part_query_io.h"
#include "race_query_io.h"

INT_PTR CALLBACK ResearchDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC         hdc;
    int16_t     y;
    int16_t     i;
    PAINTSTRUCT ps;
    int16_t     dx;
    RECT        rc;
    HWND        hwndRad;
    int16_t     dxCurrent;
    PLANET     *lppl;
    int16_t     c;
    PLANET     *lpplMac;
    HFONT       hfontSav;
    char       *psz;
    RECT        rcWindow;
    POINT16     pt;
    int16_t     iResTechNext;
    int16_t     fChg;

    switch (message) {
    case WM_INITDIALOG:
        pctResGlob = rgplr[idPlayer].pctResearch;
        iResTechNow = static_cast<TechFieldType>(rgplr[idPlayer].iTechNow());
        CheckRadioButton(hwnd, IDC_RESEARCH_ENERGY, IDC_RESEARCH_BIOTECH, iResTechNow + 1073);
        hdc = GetDC(hwnd);
        hfontSav = (HFONT)SelectObject(hdc, rghfontArial8[1]);
        c = TextIO().pfnCchGetString(idsCurrent, szWork);
        dxCurrent = LOWORD(GetTextExtent(hdc, szWork, c));
        dxResRadio = 0;
        for (i = 1073; i <= 1078; i++) {
            c = TextIO().pfnCchGetString(static_cast<StringId>(idsEnergy + i - IDC_RESEARCH_ENERGY), szWork);
            dx = LOWORD(GetTextExtent(hdc, szWork, c));
            if (dx > dxResRadio) {
                dxResRadio = dx;
            }
            hwndRad = ControlIO().pfnGetDlgItem(hwnd, i);
            ControlIO().pfnFUiSetWindowText(hwndRad, szWork);
            ControlIO().pfnLUiSendMessage(hwndRad, WM_SETFONT, (WPARAM)rghfontArial8[1], 0);
        }
        dxResRadio += 64;
        y = dyArial8 * 4 - (dyArial8 >> 1) + 1;
        for (i = 1073; i <= 1078; i++) {
            hwndRad = ControlIO().pfnGetDlgItem(hwnd, i);
            ControlIO().pfnFUiSetWindowPos(hwndRad, NULL, 16, y, dxResRadio, (int16_t)(3 * dyArial8) / 2, SWP_NOZORDER | SWP_SHOWWINDOW);
            y += (int16_t)(3 * dyArial8) / 2;
        }
        hwndRad = ControlIO().pfnGetDlgItem(hwnd, IDC_RESEARCH_NEXT_FIELD);
        for (i = 0; i <= 7; i++) {
            psz = TextIO().pfnCompressed(static_cast<StringId>(idsSameField + i));
            ControlIO().pfnLUiSendMessage(hwndRad, CB_ADDSTRING, 0, (LPARAM)psz);
        }
        i = rgplr[idPlayer].iTechNext();
        if (i == 6) {
            i = 0;
        } else if (i < 6) {
            i++;
        }
        ControlIO().pfnLUiSendMessage(hwndRad, CB_SETCURSEL, i, 0);
        dxResLeft = dxResRadio + dxCurrent + 40;
        dxResRight = 0;
        for (i = idsResourcesNeededComplete; i <= idsYearsProjectedResearchBudget; i++) {
            c = TextIO().pfnCchGetString(static_cast<StringId>(i), szWork);
            dx = LOWORD(GetTextExtent(hdc, szWork, c));
            if (dx > dxResRight) {
                dxResRight = dx;
            }
        }
        dxResStrRight = dxResRight;
        dx = LOWORD(GetTextExtent(hdc, TextIO().pfnCompressed(idsN9999992), 7));
        dxResRight += dx + 48;
        y += 10 * dyArial8;
        ControlIO().pfnFUiSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDCANCEL), NULL, dxResLeft + dxResRight - 152, y - (int16_t)(3 * dyArial8) / 2 - 8, 70, (int16_t)(3 * dyArial8) / 2,
                     SWP_NOZORDER | SWP_SHOWWINDOW);
        ControlIO().pfnFUiSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_HELP), NULL, dxResLeft + dxResRight - 76, y - (int16_t)(3 * dyArial8) / 2 - 8, 70, (int16_t)(3 * dyArial8) / 2,
                     SWP_NOZORDER | SWP_SHOWWINDOW);
        GetWindowRect(hwnd, &rcWindow);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        ControlIO().pfnFUiSetWindowPos(hwnd, NULL, 0, 0, dxResLeft + dxResRight + rcWindow.right - rcWindow.left - rc.right, y + rcWindow.bottom - rcWindow.top - rc.bottom,
                     SWP_NOMOVE | SWP_NOZORDER);
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyResDlg, TRUE);
        SelectObject(hdc, hfontSav);
        ReleaseDC(hwnd, hdc);
        lResTotal = 0;
        lppl = lpPlanets;
        lpplMac = lpPlanets + cPlanet;
        for (; lppl < lpplMac; lppl++) {
            if (lppl->iPlayer == idPlayer) {
                lResTotal += ResearchIO().pfnResources(lppl, idPlayer);
            }
        }
        lResBudget = ProjectedResearchSpending(pctResGlob);
        if (gd.fTutorial) {
            PresentationIO().pfnTutor();
        }
        return 1;
    case WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        for (i = 1073; i <= 1078 && (HWND)lParam != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 1078 || message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        return FTrackResearchDlg(hwnd, LOWORD(lParam), HIWORD(lParam), wParam);
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        DrawResearchDlg(hwnd, hdc, &rc, -1);
        EndPaint(hwnd, &ps);
        return 1;
    case WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (pt.x > 12 && pt.x < dxResLeft - 12 && pt.y >= yTopFutureTech && pt.y < cFutureTech * dyArial8 + yTopFutureTech) {
            SetCursor(hcurArrowHelp);
            return 1;
        }
        if (yTopTechNote != -1 && pt.y >= yTopTechNote && pt.y < dyArial8 * 2 + yTopTechNote && pt.x > dxResLeft) {
            SetCursor(hcurArrowHelp);
            return 1;
        }
        /* fallthrough */
    case WM_COMMAND:
        if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_RESEARCH_ENERGY && LOWORD(wParam) <= IDC_RESEARCH_BIOTECH) {
            if (IsDlgButtonChecked(hwnd, LOWORD(wParam)) != 0) {
                iResTechNow = static_cast<TechFieldType>(LOWORD(wParam) - 1073);
                ControlIO().pfnGetClientRect(hwnd, &rc);
                DrawResearchDlg(hwnd, NULL, &rc, 4);
            }
        } else {
            if (LOWORD(wParam) == IDCANCEL) {
                fChg = FALSE;
                iResTechNext = LOWORD(ControlIO().pfnLUiSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_RESEARCH_NEXT_FIELD), CB_GETCURSEL, 0, 0));
                if (iResTechNext == 0) {
                    iResTechNext = 6;
                } else if (iResTechNext <= 6) {
                    iResTechNext--;
                }
                if (iResTechNow != rgplr[idPlayer].iTechNow() || iResTechNext != rgplr[idPlayer].iTechNext() || pctResGlob != rgplr[idPlayer].pctResearch) {
                    rgplr[idPlayer].pctResearch = pctResGlob;
                    rgplr[idPlayer].SetITechNow(iResTechNow);
                    rgplr[idPlayer].SetITechNext(iResTechNext);
                    i = rgplr[idPlayer].iTechCur * 256 + pctResGlob;
                    WriteMemResearch(i);
                    fChg = TRUE;
                    if (gd.fTutorial && idPlayer == 0) {
                        tutor.fChange = TRUE;
                        PresentationIO().pfnTutor();
                    }
                }
                ControlIO().pfnStickyDlgPos(hwnd, &ptStickyResDlg, FALSE);
                ControlIO().pfnEndDialog(hwnd, fChg);
                pctResGlob = -1;
                if (gd.fTutorial) {
                    PresentationIO().pfnTutor();
                }
                return 1;
            }
            if (LOWORD(wParam) == IDC_HELP) {
                WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhResearchDialog);
                return 1;
            }
        }
    }
    return 0;
}

void DrawResearchDlg(HWND hwnd, HDC hdc, RECT *prc, int16_t grbitDraw) {
    int16_t      dxCurrent;
    char         szTemp[60];
    RECT         rcT;
    int16_t      iMax;
    int16_t      iTechSav;
    int16_t      iter;
    int16_t      fCreatedDC;
    mdPartAvail  mdAvail;
    int16_t      i;
    int16_t      c;
    HullSlotType grbitCur;
    COLORREF     crBackSav;
    COLORREF     crForeSav;
    HFONT        hfontSav;
    int16_t      xNum;
    int16_t      xCtr;
    int16_t      dx;
    char         szTemp2[60];
    PART         part;
    int32_t      l;
    int16_t      iMin;
    RECT         rc;
    int32_t      lSpent;
    HBRUSH       hbrSav;
    int32_t      lRBEffective;
    int16_t      cch;

    fCreatedDC = FALSE;
    if (!hdc) {
        fCreatedDC = TRUE;
        hdc = GetDC(hwnd);
    }
    hfontSav = (HFONT)SelectObject(hdc, rghfontArial8[1]);
    crForeSav = SetTextColor(hdc, 0);
    crBackSav = SetBkColor(hdc, crButtonFace);
    if (!(grbitDraw & 0xff))
        goto DrawRightSide;

    c = TextIO().pfnCchGetString(idsCurrent, szWork);
    dxCurrent = LOWORD(GetTextExtent(hdc, szWork, c));
    SetRect(&rc, 8, dyArial8, dxResLeft - 8, 12 * dyArial8 + dyArial8);
    if (!(grbitDraw & 3))
        goto DrawComingAttractions;

    _Draw3dFrame(hdc, &rc, -1);
    c = TextIO().pfnCchGetString(idsTechnologyStatus, szWork);
    TextOut(hdc, rc.left + 8, rc.top - (dyArial8 >> 1), szWork, c);
    rc.top += dyArial8 >> 1;
    c = TextIO().pfnCchGetString(idsCurrent, szWork);
    TextOut(hdc, rc.right - 8 - dxCurrent, rc.top, szWork, c);
    rc.top += dyArial8;
    i = TextIO().pfnCchGetString(idsFieldStudy, szWork);
    TextOut(hdc, rc.left + 26, rc.top, szWork, i);
    xCtr = rc.right - 8 - (dxCurrent >> 1);
    i = TextIO().pfnCchGetString(idsLevel, szWork);
    CtrTextOut(hdc, xCtr, rc.top, szWork, i);
    PatBlt(hdc, rc.left + 8, rc.top + dyArial8, rc.right - rc.left - 16, 1, BLACKNESS);
    rc.top += (dyArial8 >> 2) + dyArial8 + 2;
    for (i = 0; i < 6; i++) {
        c = wsprintf(szWork, PCTD, rgplr[idPlayer].rgTech[i]);
        CtrTextOut(hdc, xCtr, rc.top, szWork, c);
        rc.top += (int16_t)(3 * dyArial8) / 2;
    }

DrawComingAttractions:
    SetRect(&rc, 8, (int16_t)(3 * dyArial8) / 2 + rc.bottom, dxResLeft - 8, prc->bottom - 8);
    if ((grbitDraw & 4) && !(grbitDraw & 3)) {
        ExpandRc(&rc, -6, -(dyArial8 >> 1));
        rc.top += dyArial8 >> 1;
        ControlIO().pfnInvalidateRect(hwnd, &rc, TRUE);
        goto DrawRightSide;
    } else {
        _Draw3dFrame(hdc, &rc, -1);
        c = TextIO().pfnCchGetString(idsExpectedResearchBenefits, szWork);
        TextOut(hdc, rc.left + 8, rc.top - (dyArial8 >> 1), szWork, c);
        iTechSav = rgplr[idPlayer].iTechNow();
        cFutureTech = 0;
        yTopFutureTech = rc.top + dyArial8;
        for (iter = 1; iter < 10; iter++) {
            iMax = iter;
            iMin = iter;
            if (iter == 1) {
                SetTextColor(hdc, 32512);
            } else if (iter >= 2 && iter <= 4) {
                SetTextColor(hdc, 8323072);
            } else {
                SetTextColor(hdc, 0);
                if (iter == 9) {
                    iMax = 26;
                }
            }
            grbitCur = hstEngine;
            rgplr[idPlayer].SetITechNow(iResTechNow);
            for (; grbitCur != hstNone; grbitCur = static_cast<HullSlotType>(grbitCur * 2)) {
                if (grbitCur & (hstEngine | hstScanner | hstShield | hstArmor | hstBeam | hstTorp | hstBomb | hstMining | hstMines | hstSpecialSB | hstSBHull |
                                hstSpecialE | hstSpecialM | hstTerra | hstHull | hstPlanetary)) {
                    i = 0;
                    part.hs.grhst = grbitCur;
                    while (1) {
                        part.hs.iItem = i;
                        mdAvail = static_cast<mdPartAvail>(PartQueryIO().pfnLookup(&part));
                        if (mdAvail == mdPartAvailInvalid)
                            break;
                        if (part.hs.grhst == hstTerra && RaceQueryIO().pfnGrbit(&rgplr[idPlayer], ibitRaceTT) != 0 &&
                            (part.hs.iItem == iterraGravityTerraform3 || part.hs.iItem == iterraTempTerraform3 || part.hs.iItem == iterraRadiationTerraform3))
                            break;
                        if (iMin <= mdAvail - 1 && iMax >= mdAvail - 1) {
                            rc.top += dyArial8;
                            if (rc.top + dyArial8 > rc.bottom)
                                goto TooManyToFinish;
                            strcpy(szWork, part.pcom().szName());
                            TextOut(hdc, rc.left + 8, rc.top, szWork, strlen(szWork));
                            if (cFutureTech < 8) {
                                rghsFutureTech[cFutureTech++] = part.hs;
                            }
                        }
                        i++;
                    }
                }
            }
        }
    TooManyToFinish:
        rgplr[idPlayer].SetITechNow(iTechSav);
        SetTextColor(hdc, 0);
    }

DrawRightSide:
    SetRect(&rc, dxResLeft + 8, dyArial8, dxResLeft + dxResRight - 8, (int16_t)(14 * dyArial8) / 2 + dyArial8);
    xCtr = rc.left + 8 + dxResStrRight;
    xNum = LOWORD(GetTextExtent(hdc, "999999", 6)) + xCtr;
    if ((grbitDraw & 4) && !(grbitDraw & 3)) {
        ExpandRc(&rc, -6, -(dyArial8 >> 1));
        rc.top += dyArial8 >> 1;
        ControlIO().pfnInvalidateRect(hwnd, &rc, TRUE);
        goto CleanUp;
    }
    if (!(grbitDraw & 0x300))
        goto DrawResourceAlloc;

    if (rgplr[idPlayer].rgTech[iResTechNow] >= 26) {
        l = -1;
    } else {
        l = ResearchIO().pfnTechCost(iResTechNow, rgplr[idPlayer].rgTech[iResTechNow] + 1, idPlayer);
        lSpent = rgplr[idPlayer].rgResSpent[iResTechNow];
        if (game.fSlowTech) {
            lSpent = (int32_t)(lSpent * 2);
        }
        l = 0 <= l - lSpent ? l - lSpent : 0;
    }
    if ((grbitDraw & 0x200) && !(grbitDraw & 0xf)) {
        rc.top += (int16_t)(3 * dyArial8) / 2 * 2 + dyArial8;
        hbrSav = (HBRUSH)SelectObject(hdc, hbrButtonFace);
        PatBlt(hdc, xCtr, rc.top, xNum - xCtr, dyArial8, PATCOPY);
        SelectObject(hdc, hbrSav);
        goto DrawYearComplete;
    } else {
        _Draw3dFrame(hdc, &rc, -1);
        c = TextIO().pfnCchGetString(idsCurrentlyResearching, szWork);
        TextOut(hdc, rc.left + 8, rc.top - (dyArial8 >> 1), szWork, c);
        rc.top += dyArial8;
        TextIO().pfnCchGetString(static_cast<StringId>(idsEnergy + static_cast<int>(iResTechNow)), szTemp);
        TextIO().pfnCchGetString(idsSTechLevelD, szTemp2);
        c = wsprintf(szWork, szTemp2, szTemp, rgplr[idPlayer].rgTech[iResTechNow] + 1);
        RightTextOut(hdc, xCtr, rc.top, szWork, c, 0);
        rc.top += (int16_t)(3 * dyArial8) / 2;
        RightTextOut(hdc, xCtr, rc.top, TextIO().pfnCompressed(idsResourcesNeededComplete), 0, 0);
        if (l == -1) {
            c = TextIO().pfnCchGetString(idsMaxed, szWork);
        } else {
            c = wsprintf(szWork, PCTLD, l);
        }
        TextOut(hdc, xCtr, rc.top, szWork, c);
        rc.top += (int16_t)(3 * dyArial8) / 2;
        RightTextOut(hdc, xCtr, rc.top, TextIO().pfnCompressed(idsEstimatedTimeCompletion), 0, 0);
    }
DrawYearComplete:
    if (l == -1) {
        c = TextIO().pfnCchGetString(idsMaxed, szWork);
        TextOut(hdc, xCtr, rc.top, szWork, c);
    } else if (l == 0) {
        l = 1;
        goto PrintYear;
    } else if (lResBudget == 0) {
        c = TextIO().pfnCchGetString(idsNever2, szWork);
        TextOut(hdc, xCtr, rc.top, szWork, c);
    } else {
        lRBEffective = lResBudget;
        if (RaceQueryIO().pfnGrbit(&rgplr[idPlayer], ibitRaceGeneralizedResearch) != 0) {
            lRBEffective -= (int32_t)(lRBEffective >> 1);
        }
        l = (int32_t)((l + lRBEffective - 1) / lRBEffective);
    PrintYear:
        c = wsprintf(szWork, TextIO().pfnCompressed(idsLdYearC), l, l == 1 ? 32 : 115);
        TextOut(hdc, xCtr, rc.top, szWork, c);
    }
    RightTextOut(hdc, xCtr - 60, rc.top + dyArial8 + 5, TextIO().pfnCompressed(idsFieldResearch), 0, 0);
    ControlIO().pfnGetClientRect(ControlIO().pfnGetDlgItem(hwnd, IDC_RESEARCH_NEXT_FIELD), &rcT);
    MapWindowPoints(ControlIO().pfnGetDlgItem(hwnd, IDC_RESEARCH_NEXT_FIELD), hwnd, (POINT *)&rcT, 2);
    if (rcT.top != rc.top + dyArial8 + 2) {
        ControlIO().pfnFUiSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_RESEARCH_NEXT_FIELD), NULL, xCtr - 60, rc.top + dyArial8 + 2, rc.right - xCtr + 50, 9 * dyArial8,
                     SWP_NOZORDER | SWP_NOREDRAW);
    }

DrawResourceAlloc:
    rc.top = (int16_t)(3 * dyArial8) / 2 + rc.bottom;
    rc.bottom = dyArial8 * 8 + rc.top;
    if (!(grbitDraw & 0x400))
        goto DrawAnnualRes;

    _Draw3dFrame(hdc, &rc, -1);
    c = TextIO().pfnCchGetString(idsResourceAllocation, szWork);
    TextOut(hdc, rc.left + 8, rc.top - (dyArial8 >> 1), szWork, c);

DrawAnnualRes:
    rc.top += dyArial8;
    if (!(grbitDraw & 0x800))
        goto DrawTotalSpent;

    RightTextOut(hdc, xCtr, rc.top, TextIO().pfnCompressed(idsAnnualResourcesPlanets), 0, 0);
    c = wsprintf(szWork, PCTLD, lResTotal);
    RightTextOut(hdc, xNum, rc.top, szWork, c, 0);

DrawTotalSpent:
    rc.top += (int16_t)(3 * dyArial8) / 2;
    if (!(grbitDraw & 0x1000))
        goto DrawBudget;

    RightTextOut(hdc, xCtr, rc.top, TextIO().pfnCompressed(idsTotalResourcesSpentResearchLastYear), 0, 0);
    c = wsprintf(szWork, PCTLD, rgplr[idPlayer].lResLastYear);
    RightTextOut(hdc, xNum, rc.top, szWork, c, 0);

DrawBudget:
    rc.top += (int16_t)(3 * dyArial8) / 2;
    dx = 0;
    if (grbitDraw & 0x2000) {
        RightTextOut(hdc, xCtr, rc.top, TextIO().pfnCompressed(idsResourcesBudgetedResearch), 0, 0);
    } else if ((grbitDraw & 0x4000) && !(grbitDraw & 0xf)) {
        dx = LOWORD(GetTextExtent(hdc, "100", 3));
        goto DrawResPct;
    }
DrawResPct:
    c = wsprintf(szWork, PCTD, pctResGlob);
    RightTextOut(hdc, xNum, rc.top, szWork, c, dx);
    if ((grbitDraw & 0x4000) && !(grbitDraw & 0xf))
        goto DrawProjBudg;

    dx = LOWORD(GetTextExtent(hdc, "%", 1));
    TextOut(hdc, xNum, rc.top, "%", 1);
    rcSpinTop.left = xNum + dx + 4;
    rcSpinTop.top = rc.top - 4;
    rcSpinTop.right = rcSpinTop.left + 15;
    rcSpinTop.bottom = (dyArial8 >> 1) + rc.top + 1;
    rcSpinBot = rcSpinTop;
    OffsetRect(&rcSpinBot, 0, rcSpinTop.bottom - rcSpinTop.top - 1);
    DrawBtn(hdc, &rcSpinTop, 160, FALSE, NULL);
    DrawBtn(hdc, &rcSpinBot, 161, FALSE, NULL);

DrawProjBudg:
    rc.top += (int16_t)(3 * dyArial8) / 2;
    if ((grbitDraw & 0x4000) && !(grbitDraw & 0xf))
        goto DrawProjBudgData;

    RightTextOut(hdc, xCtr, rc.top, TextIO().pfnCompressed(idsYearsProjectedResearchBudget), 0, 0);

DrawProjBudgData:
    c = wsprintf(szWork, PCTLD, lResBudget);
    RightTextOut(hdc, xNum, rc.top, szWork, c, xNum - xCtr);
    if (RaceQueryIO().pfnGrbit(&rgplr[idPlayer], ibitRaceGeneralizedResearch) != 0 || RaceQueryIO().pfnGrbit(&rgplr[idPlayer], ibitRaceBleedingEdgeTech) != 0) {
        cch = TextIO().pfnCchGetString(idsRaceHas, szTemp);
        SelectObject(hdc, rghfontArial8[1]);
        rc.top += 3 * dyArial8;
        rc.left = dxResLeft + 8;
        yTopTechNote = rc.top;
        dx = LOWORD(GetTextExtent(hdc, szTemp, cch));
        if (RaceQueryIO().pfnGrbit(&rgplr[idPlayer], ibitRaceGeneralizedResearch) != 0) {
            TextOut(hdc, rc.left, rc.top, szTemp, cch);
            c = TextIO().pfnCchGetString(idsGeneralizedResearch, szWork);
            TextOut(hdc, rc.left + dx, rc.top, szWork, c);
            rc.top += dyArial8 + 2;
        }
        if (RaceQueryIO().pfnGrbit(&rgplr[idPlayer], ibitRaceBleedingEdgeTech) == 0)
            goto CleanUp;

        TextOut(hdc, rc.left, rc.top, szTemp, cch);
        c = TextIO().pfnCchGetString(idsBleedingEdgeTechnology, szWork);
        TextOut(hdc, rc.left + dx, rc.top, szWork, c);

    } else {
        yTopTechNote = -1;
    }

CleanUp:
    SetBkColor(hdc, crBackSav);
    SetTextColor(hdc, crForeSav);
    SelectObject(hdc, hfontSav);
    if (fCreatedDC) {
        ReleaseDC(hwnd, hdc);
    }
    return;
}

int16_t FTrackResearchDlg(HWND hwnd, int16_t x, int16_t y, int16_t fkb) {
    int16_t bt;
    POINT16 pt;
    int16_t dChg;
    int16_t i;
    int16_t cNew;
    RECT   *prc;
    BTNT    btnt;
    RECT    rc;

    pt.x = x;
    pt.y = y;
    if (PtInRect(&rcSpinTop, PointFrom16(pt)) != 0) {
        i = 1;
        prc = &rcSpinTop;
        bt = 160;
    } else if (PtInRect(&rcSpinBot, PointFrom16(pt)) != 0) {
        i = -1;
        prc = &rcSpinBot;
        bt = 161;
    } else {
        if (y >= yTopFutureTech && y < cFutureTech * dyArial8 + yTopFutureTech && x > 12 && x < dxResLeft - 12) {
            i = (int16_t)(y - yTopFutureTech) / dyArial8;
            GlobalPD.part.hs = rghsFutureTech[i];
            PartQueryIO().pfnLookup(&GlobalPD.part);
            GlobalPD.grPopup = grPopupComponent;
            Popup(hwnd, x, y);
            return TRUE;
        }
        if (yTopTechNote != -1 && y >= yTopTechNote && y < 3 * dyArial8 + yTopTechNote && x > dxResLeft) {
            if (RaceQueryIO().pfnGrbit(&rgplr[idPlayer], ibitRaceGeneralizedResearch) != 0 &&
                (y < (int16_t)(3 * dyArial8) / 2 + yTopTechNote || RaceQueryIO().pfnGrbit(&rgplr[idPlayer], ibitRaceBleedingEdgeTech) == 0)) {
                i = idsRaceTakesHolisticApproachResearchHalfResources;
            } else {
                i = idsNewTechsInitiallyCostTwiceMuchBuild;
            }
            GlobalPD.psz = TextIO().pfnCompressed(static_cast<StringId>(i));
            GlobalPD.dxOut = dxResRight;
            GlobalPD.grPopup = grPopupString;
            Popup(hwnd, x, y);
            return TRUE;
        }
        return FALSE;
    }
    ControlIO().pfnGetClientRect(hwnd, &rc);
    InitBtnTrack(&btnt, hwnd, NULL, prc, bt, 80, FALSE, FALSE, NULL);
    if (fkb & 4) {
        dChg = 10;
    } else {
        dChg = 1;
    }
    while (FTrackBtn(&btnt)) {
        cNew = dChg * i + pctResGlob;
        if (100 < (0 <= cNew ? cNew : 0)) {
            cNew = 100;
        } else if (0 > cNew) {
            cNew = 0;
        }
        if (cNew != pctResGlob) {
            pctResGlob = cNew;
            lResBudget = ProjectedResearchSpending(pctResGlob);
            DrawResearchDlg(hwnd, btnt.hdc, &rc, 16896);
        }
    }
    return TRUE;
}

INT_PTR CALLBACK BrowserDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC         hdc;
    HMENU       hmenu;
    int16_t     i;
    int16_t     c;
    PAINTSTRUCT ps;
    HFONT       hfontSav;
    int16_t     dx;
    RECT        rc;
    HWND        hwndDD;
    int32_t     lSel;
    mdPartAvail md;
    int16_t     fShowAll;
    int16_t     fAllHsts;
    uint16_t    iItemStart;
    int16_t     iStart;
    int16_t     cIter;
    int16_t     iOff;

    switch (message) {
    case WM_INITDIALOG:
        hwndBrowser = hwnd;
        ControlIO().pfnFUiSetWindowPos(hwnd, NULL, 0, 0, (dyArial8 <= 14 ? 0 : 40) + 358 + GetSystemMetrics(SM_CXDLGFRAME) * 2,
                     dyArial10 + 72 + 12 * dyArial8 + 6 + 3 * dyArial8 + 25 + GetSystemMetrics(SM_CYDLGFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION),
                     SWP_NOMOVE | SWP_NOZORDER);
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyBrowserDlg, TRUE);
        hdc = GetDC(hwnd);
        hfontSav = (HFONT)SelectObject(hdc, rghfontArial8[1]);
        hwndDD = ControlIO().pfnGetDlgItem(hwnd, IDC_BROWSER_COMPONENT_CATEGORY);
        c = GetDlgItemText(hwnd, IDC_BACK, szWork, 80);
        dx = LOWORD(GetTextExtent(hdc, szWork, c)) + 14;
        ControlIO().pfnFUiSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_BACK), NULL, 6, 6, dx, (int16_t)(3 * dyArial8) / 2, SWP_NOZORDER);
        ControlIO().pfnFUiSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_NEXT), NULL, (dyArial8 <= 14 ? 0 : 40) + 350 - dx, 6, dx, (int16_t)(3 * dyArial8) / 2, SWP_NOZORDER);
        ControlIO().pfnFUiSetWindowPos(hwndDD, NULL, dx + 12, 6, (dyArial8 <= 14 ? 0 : 40) + 344 - dx * 2 - 12, 18 * dyArial8, SWP_NOZORDER);
        ControlIO().pfnFUiSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDCANCEL), NULL, (dyArial8 <= 14 ? 0 : 40) + 350 - dx,
                     dyArial10 + 72 + 12 * dyArial8 + 6 + (int16_t)(3 * dyArial8) / 2 + 18, dx, (int16_t)(3 * dyArial8) / 2, SWP_NOZORDER);
        ControlIO().pfnFUiSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_BROWSER_AVAILABLE_ONLY), NULL, 6, dyArial10 + 72 + 12 * dyArial8 + 6 + (int16_t)(3 * dyArial8) / 2 + 18,
                     (dyArial8 <= 14 ? 0 : 40) + 344 - dx, (int16_t)(3 * dyArial8) / 2, SWP_NOZORDER);
        SelectObject(hdc, hfontSav);
        ReleaseDC(hwnd, hdc);
        if (!fBrowserValid) {
            vpartBrowser.hs.grhst = hstArmor;
            vpartBrowser.hs.iItem = 0;
        }
        PartQueryIO().pfnLookup(&vpartBrowser);
        hwndBrowserChild = HwndUiCreateWindowEx(0, szBrowser, NULL, WS_CHILD | WS_VISIBLE, 6, (int16_t)(3 * dyArial8) / 2 + 12, (dyArial8 <= 14 ? 0 : 40) + 344,
                                        dyArial10 + 72 + 12 * dyArial8 + 6, hwnd, NULL, hInst, NULL);
        for (i = idsAll; i <= idsTorpedoes; i++) {
            ControlIO().pfnLUiSendMessage(hwndDD, CB_ADDSTRING, 0, (LPARAM)TextIO().pfnCompressed(static_cast<StringId>(i)));
        }
        ControlIO().pfnLUiSendMessage(hwndDD, CB_SETCURSEL, 0, 0);
        if (gd.fTutorial) {
            PresentationIO().pfnTutor();
        }
        return 1;
    case WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        for (i = 266; i <= 267 && (HWND)lParam != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 267 || message == WM_CTLCOLORSTATIC) {
            SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 1;
    case WM_DESTROY:
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyBrowserDlg, FALSE);
        hwndBrowser = 0;
        fBrowserValid = FALSE;
        hmenu = GetASubMenu(hwndFrame, menuHelp);
        CheckMenuItem(hmenu, IDM_VIEW_BROWSER_TOGGLE2, MF_UNCHECKED);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            ControlIO().pfnStickyDlgPos(hwnd, &ptStickyBrowserDlg, FALSE);
            hwndBrowser = 0;
            fBrowserValid = FALSE;
            hmenu = GetASubMenu(hwndFrame, menuHelp);
            CheckMenuItem(hmenu, IDM_VIEW_BROWSER_TOGGLE2, MF_UNCHECKED);
            // The browser is modeless (CreateDialog), so no modal loop owns it.
            ControlIO().pfnFUiDestroyWindow(hwnd);
            if (gd.fTutorial) {
                PresentationIO().pfnTutor();
            }
            return 1;
        case IDC_BROWSER_AVAILABLE_ONLY:
            if (!IsDlgButtonChecked(hwnd, IDC_BROWSER_AVAILABLE_ONLY) || PartQueryIO().pfnLookup(&vpartBrowser) == mdPartAvailAvailable)
                break;
            goto ResetCategory;
        case IDC_BROWSER_COMPONENT_CATEGORY:
            if (HIWORD(wParam) != 1)
                break;
        ResetCategory:
            fShowAll = IsDlgButtonChecked(hwnd, IDC_BROWSER_AVAILABLE_ONLY) == 0;
            lSel = ControlIO().pfnLUiSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BROWSER_COMPONENT_CATEGORY), CB_GETCURSEL, 0, 0);
            if (lSel < 0)
                break;
            vpartBrowser.hs.grhst = static_cast<HullSlotType>(rggrbitBrParts[lSel <= 1 ? 1 : lSel]);
            vpartBrowser.hs.iItem = 0;
            while (1) {
                md = static_cast<mdPartAvail>(PartQueryIO().pfnLookup(&vpartBrowser));
                if (md == mdPartAvailInvalid) {
                    vpartBrowser.SetPcom(NULL);
                    break;
                }
                if (md == mdPartAvailAvailable || fShowAll)
                    break;
                vpartBrowser.hs.iItem++;
            }
            ControlIO().pfnInvalidateRect(hwndBrowserChild, NULL, TRUE);
            break;
        case IDC_NEXT:
        case IDC_BACK:
            iItemStart = vpartBrowser.hs.iItem;
            cIter = 0;
            lSel = ControlIO().pfnLUiSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_BROWSER_COMPONENT_CATEGORY), CB_GETCURSEL, 0, 0);
            fAllHsts = lSel == 0;
            for (i = 0; i < 17 && vpartBrowser.hs.grhst != rggrbitBrParts[i]; i++) {
            }
            iStart = i;
            fShowAll = IsDlgButtonChecked(hwnd, IDC_BROWSER_AVAILABLE_ONLY) == 0;
            iOff = LOWORD(wParam) == IDC_NEXT ? 1 : -1;
            do {
                if ((vpartBrowser.hs.iItem += iOff) == iItemStart && !fAllHsts)
                    break;
            Top:
                if (cIter++ > 350)
                    goto NullItem;
                md = static_cast<mdPartAvail>(PartQueryIO().pfnLookup(&vpartBrowser));
                if (md == mdPartAvailInvalid) {
                    if (LOWORD(wParam) == IDC_NEXT && fAllHsts) {
                        i++;
                        if (i >= 17) {
                            i = 1;
                        }
                        vpartBrowser.hs.grhst = static_cast<HullSlotType>(rggrbitBrParts[i]);
                        vpartBrowser.hs.iItem = 0;
                        goto Top;
                    }
                    if (vpartBrowser.hs.iItem == 0 && !fAllHsts) {
                        vpartBrowser.SetPcom(NULL);
                        break;
                    }
                    if (LOWORD(wParam) == IDC_BACK && fAllHsts) {
                        if (vpartBrowser.hs.iItem > 100) {
                            i--;
                            if (i <= 0) {
                                i = 16;
                            }
                            vpartBrowser.hs.grhst = static_cast<HullSlotType>(rggrbitBrParts[i]);
                            vpartBrowser.hs.iItem = 100;
                            goto Top;
                        }
                    } else if (LOWORD(wParam) == IDC_NEXT) {
                        vpartBrowser.hs.iItem = 0;
                        goto Top;
                    }
                }
            } while (md != mdPartAvailAvailable &&
                     (md == mdPartAvailInvalid || !fShowAll || (md == mdPartAvailRestricted && PartQueryIO().pfnHidden(&vpartBrowser))));
            if (vpartBrowser.hs.iItem == iItemStart && iStart == i && (md == mdPartAvailAvailable || fShowAll))
                break;
            if (vpartBrowser.hs.iItem == iItemStart && iStart == i) {
                if (PartQueryIO().pfnLookup(&vpartBrowser) == mdPartAvailAvailable)
                    break;
            NullItem:
                vpartBrowser.SetPcom(NULL);
            }
            ControlIO().pfnInvalidateRect(hwndBrowserChild, NULL, TRUE);
        }
    }
    return 0;
}

LRESULT CALLBACK BrowserWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC         hdc;
    POINT16     pt;
    int16_t     i;
    PAINTSTRUCT ps;
    RECT        rc;

    switch (message) {
    case WM_CREATE:
        return 0;
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        DisplayComponentInfo(hdc, rc.right, rc.bottom, &vpartBrowser);
        EndPaint(hwnd, &ps);
        return 0;
    case WM_SETCURSOR:
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        goto Validate;
    case WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_LBUTTONDOWN:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
    Validate:
        if ((vpartBrowser.hs.grhst != hstHull && vpartBrowser.hs.grhst != hstSBHull) || hwndSlotDlg || pt.x < 5 || pt.x >= 69 || pt.y < dyArial10 + 5 ||
            pt.y >= dyArial10 + 69)
            goto Default;
        if (message == WM_LBUTTONDOWN) {
            GlobalPD.grPopup = grPopupShdefSB;
            if (vpartBrowser.hs.grhst == hstSBHull) {
                shdefBuild.hul = LphuldefSBFromId(static_cast<isbhull>(vpartBrowser.hs.iItem))->hul;
            } else {
                shdefBuild.hul = PartQueryIO().pfnHull(vpartBrowser.hs.iItem)->hul;
            }
            for (i = 0; i < shdefBuild.hul.chs; i++) {
                shdefBuild.hul.rghs[i].cItem = 0;
            }
            GlobalPD.lpshdef = &shdefBuild;
            GlobalPD.fShowDamage = FALSE;
            Popup(hwnd, pt.x, pt.y);
        } else {
            SetCursor(hcurArrowHelp);
        }
        return 1;
    default:
    Default:
        return LUiDefWindowProc(hwnd, message, wParam, lParam);
    }
}

void DisplayComponentInfo(HDC hdc, int16_t dx, int16_t dy, PART *ppart) {
    uint16_t rgCosts[4];
    StringId idsT;
    StringId ids;
    int16_t  dxStr;
    int16_t  c;
    int16_t  yText;
    int16_t  i;
    int16_t  yCur;
    int16_t  fReq;
    int16_t  yStart;
    int16_t  xNum;
    int16_t  xText;
    RECT     rcData;
    int32_t  l;
    int16_t  dxT;
    char     rgch[2];
    int16_t  dyPct;
    int16_t  dxDigit;
    int16_t  yBase;
    int16_t  y;
    int16_t  dxWarp;
    int16_t  pct;
    int16_t  fWarp10;
    int16_t  iEff;
    COLORREF crFore;
    int16_t  cch;
    int16_t  x;
    HPEN     hpenSav;
    COLORREF crBack;
    HBRUSH   hbrSav;
    int16_t  pctT;
    char     szT[256];
    int16_t  dyText;
    char    *psz;
    int16_t  dmgFloor;
    int16_t  dmgMin;
    int16_t  dmgShipRam;
    int16_t  iWarp;
    int16_t  dmgShip;
    int16_t  pctHit;
    int16_t  dxLabel;
    int16_t  dmgMinRam;
    int32_t  lpct;
    int16_t  xBase;
    int16_t  dxQuan;
    char     ch;
    int32_t  ldelta;
    RECT     rcT;

    SelectObject(hdc, hbrButtonShadow);
    PatBlt(hdc, 0, dy - 1, dx, 1, PATCOPY);
    PatBlt(hdc, dx - 1, 0, 1, dy, PATCOPY);
    SelectObject(hdc, hbrButtonHilite);
    PatBlt(hdc, 0, 0, dx, 1, PATCOPY);
    PatBlt(hdc, 0, 0, 1, dy, PATCOPY);
    PatBlt(hdc, 4, dy - dyArial8 * 2 - 2, dx - 8, 1, PATCOPY);
    SetBkMode(hdc, TRANSPARENT);
    if (!ppart->pcom()) {
        SetRect(&rcData, 5, 5, dx - 5, dy - 5);
        c = TextIO().pfnCchGetString(idsNoneAvailable, szWork);
        DiaganolTextOut(hdc, &rcData, szWork, c);
    } else {
        SelectObject(hdc, rghfontArial10[1]);
        strcpy(szWork, ppart->pcom().szName());
        CtrTextOut(hdc, dx >> 1, 3, szWork, 0);
        if (ppart->hs.grhst == hstHull || ppart->hs.grhst == hstSBHull) {
            DrawFleetBitmap(NULL, hdc, 5, dyArial10 + 5, FALSE, ppart->pcom().ibmp(), 0, FALSE, -1, 0);
        } else {
            SelectPalette(hdc, vhpal, FALSE);
            RealizePalette(hdc);
            DibBlt(hdc, 5, dyArial10 + 5, 64, 64, rghdibInventory[ppart->pcom().ibmp() >> 5], (ppart->pcom().ibmp() & 7) * 0x40,
                   ((3 - (ppart->pcom().ibmp() >> 3)) & 3) * 0x40, 64, 64, 13369376);
        }
        SelectObject(hdc, rghfontArial8[1]);
        yCur = dyArial10 + 71;
        c = TextIO().pfnCchGetString(idsTechReq, szWork);
        TextOut(hdc, 5, yCur, szWork, c);
        dxStr = 0;
        for (i = 0; i < 6; i++) {
            c = TextIO().pfnCchGetString(static_cast<StringId>(idsEner + i), szWork);
            dxT = LOWORD(GetTextExtent(hdc, szWork, c));
            if (dxT > dxStr) {
                dxStr = dxT;
            }
        }
        dxStr += 5;
        xNum = dxStr + LOWORD(GetTextExtent(hdc, "99", 2));
        fReq = FALSE;
        for (i = 0; i < 6; i++) {
            if (ppart->pcom().rgTech()[i] > 0) {
                yCur += dyArial8;
                SetTextColor(hdc, ppart->pcom().rgTech()[i] <= rgplr[idPlayer].rgTech[i] ? 0 : 127);
                c = TextIO().pfnCchGetString(static_cast<StringId>(idsEner + i), szWork);
                RightTextOut(hdc, dxStr, yCur, szWork, c, 0);
                SetTextColor(hdc, 0);
                c = wsprintf(szWork, PCTD, ppart->pcom().rgTech()[i]);
                RightTextOut(hdc, xNum, yCur, szWork, c, 0);
                fReq = TRUE;
            }
        }
        if (!fReq) {
            yCur += dyArial8;
            c = TextIO().pfnCchGetString(idsNone3, szWork);
            CtrTextOut(hdc, 37, yCur, szWork, c);
        }
        if (PartQueryIO().pfnLookup(ppart) <= mdPartAvailInvalid) {
            l = -1;
        } else {
            l = CostOfDevelopingItem((char *)ppart->pcom().rgTech());
        }
        if (l > 99999) {
            c = wsprintf(szWork, TextIO().pfnCompressed(idsCostLdk), (int32_t)((l + 500) / 1000));
            TextOut(hdc, 5, yCur + dyArial8 + 4, szWork, c);
        } else if (l > 0) {
            c = wsprintf(szWork, TextIO().pfnCompressed(idsCostLd), l);
            TextOut(hdc, 5, yCur + dyArial8 + 4, szWork, c);
        } else if (l == -1) {
            SetTextColor(hdc, 127);
            CtrTextOut(hdc, 37, yCur + dyArial8 + 4, TextIO().pfnCompressed(idsUnavail), 0);
            SetTextColor(hdc, 0);
        } else {
            CtrTextOut(hdc, 37, yCur + dyArial8 + 4, TextIO().pfnCompressed(idsAvailable), 0);
        }
        yStart = dyArial10 + 5;
        if (dyArial8 * 4 < 64) {
            yStart += (0x40 - (dyArial8 << 2)) >> 1;
        }
        yCur = yStart;
        dxStr = 0;
        for (i = 0; i <= 5; i++) {
            dxT = LOWORD(GetTextExtent(hdc, rgszMinerals[i], lstrlen(rgszMinerals[i])));
            if (dxT > dxStr) {
                dxStr = dxT;
            }
        }
        dxStr += 77;
        PartQueryIO().pfnTrueCost(idPlayer, ppart, rgCosts);
        for (i = 0; i <= 5; i++) {
            if (i != 4 && i != 3) {
                SelectObject(hdc, rghfontArial8[1]);
                SetTextColor(hdc, rgcrMinerals[i]);
                RightTextOut(hdc, dxStr, yCur, rgszMinerals[i], 0, 0);
                SelectObject(hdc, rghfontArial8[0]);
                SetTextColor(hdc, crWindowText);
                if (i != 5) {
                    c = rgCosts[i];
                } else {
                    c = rgCosts[3];
                }
                if (ppart->hs.grhst == hstSBHull || ppart->hs.grhst == hstSpecialSB) {
                    c -= c / 2;
                }
                c = wsprintf(szWork, PCTD, c);
                RightTextOut(hdc, dxStr + dxMaxMineralQuan, yCur, szWork, c, 0);
                if (i < 5) {
                    TextOut(hdc, dxStr + dxMaxMineralQuan, yCur, "kT", 2);
                }
                yCur += dyArial8;
            }
        }
        SelectObject(hdc, rghfontArial8[1]);
        if (ppart->pcom().cMass() != 0) {
            c = wsprintf(szWork, TextIO().pfnCompressed(idsMassDkt), ppart->pcom().cMass());
            TextOut(hdc, dxStr + dxMaxMineralQuan + 32, yStart, szWork, c);
        }
        SetRect(&rcData, 73 <= xNum + 4 ? xNum + 4 : 73, dyArial10 + 71, dx - 5, dy - 5 - dyArial8 * 2 - 4);
        if (dyArial8 > 14) {
            rcData.left += 4;
        }
        SelectObject(hdc, hbrButtonShadow);
        PatBlt(hdc, rcData.left, rcData.top, rcData.right - rcData.left, 1, PATCOPY);
        PatBlt(hdc, rcData.left, rcData.top, 1, rcData.bottom - rcData.top, PATCOPY);
        SelectObject(hdc, hbrButtonHilite);
        PatBlt(hdc, rcData.left, rcData.bottom - 1, rcData.right - rcData.left, 1, PATCOPY);
        PatBlt(hdc, rcData.right - 1, rcData.top, 1, rcData.bottom - rcData.top, PATCOPY);
        ExpandRc(&rcData, -3, -3);
        if (gd.fBleedingEdge && l == 0) {
            SetTextColor(hdc, crButtonHilite);
            c = TextIO().pfnCchGetString(idsBleedingEdge, szWork);
            DiaganolTextOut(hdc, &rcData, szWork, c);
            SetTextColor(hdc, crWindowText);
        }
        ids = static_cast<StringId>(idsNoString);
        switch (ppart->hs.grhst) {
        default:
            break;
        case hstEngine:
            SelectObject(hdc, rghfontArial8[1]);
            rcData.right -= 2;
            c = TextIO().pfnCchGetString(idsFuelUsageVsWarpSpeed, szWork);
            CtrTextOut(hdc, ((rcData.right - rcData.left) >> 1) + rcData.left, rcData.top, szWork, c);
            rcData.top += dyArial8;
            c = TextIO().pfnCchGetString(idsWarp, szWork);
            dxStr = LOWORD(GetTextExtent(hdc, szWork, c));
            dxWarp = (int16_t)(rcData.right - rcData.left - dxStr - 8) / 10;
            dyPct = (int16_t)(rcData.bottom - rcData.top - dyArial8 - 8) / 6;
            PatBlt(hdc, rcData.left + dxStr + 6, rcData.top, 1, rcData.bottom - rcData.top - dyArial8 - 4, BLACKNESS);
            PatBlt(hdc, rcData.left + dxStr + 6, rcData.bottom - dyArial8 - 4, rcData.right - rcData.left - dxStr - 6, 1, BLACKNESS);
            x = rcData.left + dxStr;
            y = rcData.bottom - dyArial8 - 4 - 6 * dyPct;
            pct = 800;
            SetTextColor(hdc, 127);
            while (pct >= 25) {
                if (pct == 100) {
                    SetTextColor(hdc, 0);
                    SelectObject(hdc, hbrGray);
                    PatBlt(hdc, x + 4, y, rcData.right - rcData.left - dxStr - 4, 1, PATCOPY);
                }
                c = wsprintf(szWork, PCTDPCTPCT, pct);
                RightTextOut(hdc, x, y - (dyArial8 >> 1), szWork, c, 0);
                PatBlt(hdc, x + 4, y, 5, 1, BLACKNESS);
                pct >>= 1;
                y += dyPct;
            }
            y = rcData.bottom - dyArial8;
            RightTextOut(hdc, x, y, TextIO().pfnCompressed(idsWarp), 0, 0);
            x += 6;
            yBase = rcData.bottom - dyArial8 - 4;
            dxDigit = LOWORD(GetTextExtent(hdc, "0", 1)) >> 1;
            rgch[0] = '0';
            cch = 1;
            crFore = SetTextColor(hdc, crButtonText);
            switch (ppart->hs.iItem) {
            case iengineInterspace10:
            case iengineTransStar10:
            case iengineTransGalacticMizerScoop:
            case iengineGalaxyScoop:
            case iengineEnigmaPulsar:
                fWarp10 = TRUE;
                break;
            default:
                fWarp10 = FALSE;
            }
            for (i = 0; i <= 10; i++) {
                if (i == 10) {
                    cch = 2;
                    rgch[0] = '1';
                    rgch[1] = '0';
                    if (!fWarp10) {
                        hbrSav = (HBRUSH)SelectObject(hdc, rghbrPat[0]);
                        SetTextColor(hdc, 0xffff);
                        crBack = SetBkColor(hdc, crButtonFace);
                        PatBlt(hdc, x - dxWarp + 1, rcData.top, dxWarp, yBase - rcData.top, PATCOPY);
                        SetBkColor(hdc, crBack);
                        SelectObject(hdc, hbrSav);
                    }
                }
                if (ppart->pengine()->rgcFuelUsed[i] <= 120 && ((i == 10 && fWarp10) || ppart->pengine()->rgcFuelUsed[i + 1] > 120 || (i == 9 && !fWarp10))) {
                    SetTextColor(hdc, 8323072);
                } else {
                    SetTextColor(hdc, crButtonText);
                }
                TextOut(hdc, x - dxDigit * cch, y, rgch, cch);
                if (i > 0) {
                    PatBlt(hdc, x, y - 6, 1, 5, BLACKNESS);
                }
                rgch[0]++;
                x += dxWarp;
            }
            SetTextColor(hdc, crFore);
            hpenSav = (HPEN)SelectObject(hdc, hpenDkBlue);
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
                    pctT = MulDiv(iEff - (pct >> 1), 100, pct >> 1);
                    y -= (int16_t)(pctT * dyPct) / 100;
                } else if (iEff == 0) {
                    y--;
                } else {
                    y -= (((1) > ((int16_t)(iEff * 4 * dyPct) / 100)) ? (1) : ((int16_t)(iEff * 4 * dyPct) / 100));
                }
                if (i == 0) {
                    MoveToEx(hdc, x, y, NULL);
                } else {
                    LineTo(hdc, x, y);
                }
                x += dxWarp;
            }
            SelectObject(hdc, hpenSav);
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
            SelectObject(hdc, rghfontArial8[0]);
            i = ppart->pscanner()->dRange;
            if (i == 0) {
                c = TextIO().pfnCchGetString(idsEnemyFleetsCannotDetectedScannerUnlessSame, szWork);
            } else {
                c = TextIO().pfnCchGetString(idsEnemyFleetsOrbitingPlanetCanDetectedD, szT);
                c = wsprintf(szWork, szT, i);
            }
            dyText = DrawText(hdc, szWork, c, &rcData, DT_WORDBREAK | DT_NOPREFIX);
            rcData.top += (dyArial8 >> 1) + dyText;
            i = ppart->pscanner()->grfAbilities;
            if (i == 0) {
                c = TextIO().pfnCchGetString(idsScannerCapableDeterminingPlanetsEnvironmentCompo, szWork);
            } else if (i != 4) {
                c = TextIO().pfnCchGetString(idsScannerCanDeterminePlanetsBasicStatsDistance, szT);
                c = wsprintf(szWork, szT, i == 1 ? 50 : i == 2 ? 100 : 200);
                ids = idsScannerWillUnavailableIfHaveLesserRacial;
            } else {
                if (ppart->hs.iItem == iscannerPickPocketScanner) {
                    ids = idsScannerCapablePenetratingDefensesEnemyFleetsAllo;
                } else if (ppart->hs.iItem == iscannerRobberBaronScanner) {
                    ids = idsScannerCanDeterminePlanetsStatsDistance120;
                } else {
                    ids = idsScannerCanDeterminePlanetsBasicStatsDistance2;
                }
                c = TextIO().pfnCchGetString(ids, szWork);
                ids = idsScannerRequiresPrimaryRacialTraitSuperStealth;
            }
            dyText = DrawText(hdc, szWork, c, &rcData, DT_WORDBREAK | DT_NOPREFIX);
            break;
        case hstHull:
            ExpandRc(&rcData, -4, -4);
            xText = (int16_t)(rcData.right - rcData.left) / 2 + rcData.left;
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
                c = TextIO().pfnCchGetString(idsT, szWork);
                SelectObject(hdc, rghfontArial8[1]);
                RightTextOut(hdc, xText, rcData.top, szWork, c, 0);
                SelectObject(hdc, rghfontArial8[0]);
                switch (i) {
                case 0:
                    c = wsprintf(szWork, "%dmg", ppart->phul()->wtFuelMax);
                    break;
                case 1:
                    c = wsprintf(szWork, PCTDKT, ppart->phul()->wtCargoMax);
                    break;
                case 2:
                    c = wsprintf(szWork, PCTD, ppart->phul()->dp);
                    break;
                case 3:
                    c = wsprintf(szWork, PCTD, PartQueryIO().pfnHull(ppart->phul()->ihuldef)->init);
                }
                TextOut(hdc, xText, rcData.top, szWork, c);
                i++;
                idsT = static_cast<StringId>(idsT + 1);
                rcData.top += (int16_t)(3 * dyArial8) / 2;
            }
            switch (ppart->hs.iItem) {
            case ihuldefFuelTransport:
            case ihuldefSuperFuelXport:
                psz = TextIO().pfnCompressed(idsHullWillManufacture200UnitsFuelEach);
                if (ppart->hs.iItem == ihuldefFuelTransport) {
                    pct = 5;
                } else {
                    pct = 10;
                }
                c = wsprintf(szWork, psz, pct);
                SelectObject(hdc, rghfontArial8[0]);
                xText = rcData.left;
                yText = rcData.top;
                WrapTextOut(hdc, &xText, &yText, szWork, c, xText, rcData.right - rcData.left, NULL, FALSE, TRUE);
                break;
            case ihuldefMiniMineLayer:
            case ihuldefSuperMineLayer:
                c = TextIO().pfnCchGetString(idsHullWillDoubleEfficiencyMineLayingPods, szWork);
                SelectObject(hdc, rghfontArial8[0]);
                xText = rcData.left;
                yText = rcData.top;
                WrapTextOut(hdc, &xText, &yText, szWork, c, xText, rcData.right - rcData.left, NULL, FALSE, TRUE);
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
            SelectObject(hdc, rghfontArial8[0]);
            if (ppart->phul()->wtCargoMax != 0) {
                c = TextIO().pfnCchGetString(idsStarbaseHullHasSpaceDockCanBuild, szWork);
                WrapTextOut(hdc, &xText, &yText, szWork, c, rcData.left, rcData.right - rcData.left, NULL, FALSE, TRUE);
                SelectObject(hdc, rghfontArial8[1]);
                yText = 3 * dyArial8 + rcData.top;
                xText = (int16_t)(rcData.right - rcData.left) / 2 + rcData.left;
                c = TextIO().pfnCchGetString(idsDockCapacity2, szWork);
                RightTextOut(hdc, xText, yText, szWork, c, 0);
                SelectObject(hdc, rghfontArial8[0]);
                if (ppart->phul()->wtCargoMax != 0xffff) {
                    c = wsprintf(szWork, PCTDKT, ppart->phul()->wtCargoMax);
                } else {
                    c = TextIO().pfnCchGetString(idsUnlimited, szWork);
                }
                TextOut(hdc, xText, yText, szWork, c);
            } else {
                c = TextIO().pfnCchGetString(idsStarbaseHullDoesHaveSpaceDockCan, szWork);
                WrapTextOut(hdc, &xText, &yText, szWork, c, rcData.left, rcData.right - rcData.left, NULL, FALSE, TRUE);
                yText = 3 * dyArial8 + rcData.top;
            }
            yText += (int16_t)(3 * dyArial8) / 2;
            xText = (int16_t)(rcData.right - rcData.left) / 2 + rcData.left;
            i = 0;
            while (i < 2) {
                c = TextIO().pfnCchGetString(idsT, szWork);
                SelectObject(hdc, rghfontArial8[1]);
                RightTextOut(hdc, xText, yText, szWork, c, 0);
                SelectObject(hdc, rghfontArial8[0]);
                if (i == 0) {
                    c = wsprintf(szWork, PCTD, ppart->phul()->dp);
                } else if (i == 1) {
                    c = wsprintf(szWork, PCTD, PartQueryIO().pfnHull(ppart->phul()->ihuldef)->init);
                }
                TextOut(hdc, xText, yText, szWork, c);
                i++;
                idsT = static_cast<StringId>(idsT + 1);
                yText += (int16_t)(3 * dyArial8) / 2;
            }
            break;
        case hstShield:
            ExpandRc(&rcData, -4, -4);
            SelectObject(hdc, rghfontArial8[1]);
            xText = rcData.left;
            DxStreamTextOut(hdc, &xText, rcData.top, TextIO().pfnCompressed(idsShieldStrength), 0, TRUE);
            SelectObject(hdc, rghfontArial8[0]);
            c = wsprintf(szWork, PCTD, ppart->pshield()->dp);
            DxStreamTextOut(hdc, &xText, rcData.top, szWork, c, TRUE);
            if (ppart->hs.iItem == ishieldShadowShield) {
                ids = idsArmorShieldRequiresPrimaryRacialTraitSuper;
                idsT = idsShieldDecreasesRangeWhichEnemyShipsCan;
            LShieldDisp:
                c = TextIO().pfnCchGetString(idsT, szWork);
                rcData.top += dyArial8 + 4;
                DrawText(hdc, szWork, c, &rcData, DT_WORDBREAK | DT_NOPREFIX);
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
            SelectObject(hdc, rghfontArial8[1]);
            xText = rcData.left;
            DxStreamTextOut(hdc, &xText, rcData.top, TextIO().pfnCompressed(idsArmorStrength2), 0, TRUE);
            SelectObject(hdc, rghfontArial8[0]);
            c = wsprintf(szWork, PCTD, ppart->parmor()->dp);
            DxStreamTextOut(hdc, &xText, rcData.top, szWork, c, TRUE);
            if (ppart->hs.iItem == iarmorDepletedNeutronium) {
                ids = idsArmorShieldRequiresPrimaryRacialTraitSuper;
                idsT = idsArmorDecreasesRangeWhichEnemyShipsCan;
            LArmDisp:
                c = TextIO().pfnCchGetString(idsT, szWork);
                rcData.top += dyArial8 + 4;
                DrawText(hdc, szWork, c, &rcData, DT_WORDBREAK | DT_NOPREFIX);
                break;
            } else if (ppart->hs.iItem == iarmorFieldedKelarium) {
                ids = idsArmorRequiresPrimaryRacialTraitInnerStrength;
                idsT = idsArmorAlsoActsPartShieldWhichWill;
                goto LArmDisp;
            } else if (ppart->hs.iItem == iarmorMegaPolyShell) {
                ids = idsOriginPartUnknown;
                c = TextIO().pfnCchGetString(idsPartAlsoActs100dpShield20Cloak, szWork);
                rcData.top += dyArial8 + 4;
                SelectObject(hdc, rghfontArial8[0]);
                DrawText(hdc, szWork, c, &rcData, DT_WORDBREAK | DT_NOPREFIX);
            }
            break;

        case hstBeam:
            ExpandRc(&rcData, -4, -4);
            xText = (int16_t)(rcData.right - rcData.left) / 3 + rcData.left;
            yText = rcData.top;
            c = TextIO().pfnCchGetString(idsPower, szWork);
            SelectObject(hdc, rghfontArial8[1]);
            RightTextOut(hdc, xText, yText, szWork, c, 0);
            SelectObject(hdc, rghfontArial8[0]);
            c = wsprintf(szWork, PCTD, ppart->pbeam()->dp);
            TextOut(hdc, xText, yText, szWork, c);
            yText += dyArial8;
            c = TextIO().pfnCchGetString(idsRange, szWork);
            SelectObject(hdc, rghfontArial8[1]);
            RightTextOut(hdc, xText, yText, szWork, c, 0);
            SelectObject(hdc, rghfontArial8[0]);
            c = wsprintf(szWork, PCTD, ppart->pbeam()->dRangeMax);
            TextOut(hdc, xText, yText, szWork, c);
            yText += dyArial8;
            c = TextIO().pfnCchGetString(idsInitiative, szWork);
            SelectObject(hdc, rghfontArial8[1]);
            RightTextOut(hdc, xText, yText, szWork, c, 0);
            SelectObject(hdc, rghfontArial8[0]);
            c = wsprintf(szWork, PCTD, ppart->pbeam()->init);
            TextOut(hdc, xText, yText, szWork, c);
            yText += (int16_t)(3 * dyArial8) / 2;
            rcData.top = yText;
            if (ppart->pbeam()->grfAbilities != 0) {
                SelectObject(hdc, rghfontArial7[0]);
                if (ppart->pbeam()->grfAbilities & beamSapper) {
                    c = TextIO().pfnCchGetString(idsWeaponWillDamageShieldsHasEffectArmor, szWork);
                    rcData.top += DrawText(hdc, szWork, c, &rcData, DT_WORDBREAK | DT_NOPREFIX);
                }
                if (ppart->pbeam()->grfAbilities & beamGatling) {
                    c = TextIO().pfnCchGetString(idsWeaponHitsTargetsRangeEachTimeFired, szWork);
                    rcData.top += DrawText(hdc, szWork, c, &rcData, DT_WORDBREAK | DT_NOPREFIX);
                    c = wsprintf(szWork, TextIO().pfnCompressed(idsWeaponAlsoMakesExcellentMineSweeperCapable), ppart->pbeam()->dp * 16);
                    DrawText(hdc, szWork, c, &rcData, DT_WORDBREAK | DT_NOPREFIX);
                }
                SelectObject(hdc, rghfontArial8[0]);
            } else if (ppart->hs.iItem == ibeamMultiContainedMunition) {
                ids = idsOriginPartUnknown;
                c = TextIO().pfnCchGetString(idsPartAlsoActs10CloakIncreasesTorpedo, szWork);
                SelectObject(hdc, rghfontArial7[0]);
                rcData.top += DrawText(hdc, szWork, c, &rcData, DT_WORDBREAK | DT_NOPREFIX);
                c = TextIO().pfnCchGetString(idsWeaponCanAlsoBombPlanets2Colonists, szWork);
                DrawText(hdc, szWork, c, &rcData, DT_WORDBREAK | DT_NOPREFIX);
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
            xText = (int16_t)(rcData.right - rcData.left) / 3 + rcData.left;
            yText = rcData.top;
            c = TextIO().pfnCchGetString(idsPower, szWork);
            SelectObject(hdc, rghfontArial8[1]);
            RightTextOut(hdc, xText, yText, szWork, c, 0);
            SelectObject(hdc, rghfontArial8[0]);
            c = wsprintf(szWork, PCTD, ppart->ptorp()->dp);
            TextOut(hdc, xText, yText, szWork, c);
            yText += dyArial8;
            c = TextIO().pfnCchGetString(idsRange, szWork);
            SelectObject(hdc, rghfontArial8[1]);
            RightTextOut(hdc, xText, yText, szWork, c, 0);
            SelectObject(hdc, rghfontArial8[0]);
            c = wsprintf(szWork, PCTD, ppart->ptorp()->dRangeMax);
            TextOut(hdc, xText, yText, szWork, c);
            yText += dyArial8;
            c = TextIO().pfnCchGetString(idsInitiative, szWork);
            SelectObject(hdc, rghfontArial8[1]);
            RightTextOut(hdc, xText, yText, szWork, c, 0);
            SelectObject(hdc, rghfontArial8[0]);
            c = wsprintf(szWork, PCTD, ppart->ptorp()->init);
            TextOut(hdc, xText, yText, szWork, c);
            yText += dyArial8;
            c = TextIO().pfnCchGetString(idsAccuracy, szWork);
            SelectObject(hdc, rghfontArial8[1]);
            RightTextOut(hdc, xText, yText, szWork, c, 0);
            SelectObject(hdc, rghfontArial8[0]);
            c = wsprintf(szWork, PCTD, ppart->ptorp()->dHitChance);
            TextOut(hdc, xText, yText, szWork, c);
            yText += (int16_t)(3 * dyArial8) / 2;
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
            SelectObject(hdc, rghfontArial8[1]);
            xText = rcData.left;
            yText = rcData.top;
            SelectObject(hdc, rghfontArial8[0]);
            if (ppart->hs.iItem == ibombRetroBomb) {
                c = TextIO().pfnCchGetString(idsBombDoesKillColonistsDestroyInstallationsBomb, szWork);
            } else if (ppart->pbomb()->dDmgCol == 0) {
                c = TextIO().pfnCchGetString(idsBombWillKillAnyPlanetsPopulation, szWork);
            } else {
                TextIO().pfnCchGetString(idsBombWillKillApproximatelyDDPlanets, szWork);
                c = wsprintf(szT, szWork, ppart->pbomb()->dDmgCol / 10, ppart->pbomb()->dDmgCol % 10);
                if (ppart->hs.iItem >= ibombLadyFingerBomb && ppart->hs.iItem <= ibombCherryBomb) {
                    dmgFloor = 3;
                } else if (ppart->hs.iItem >= ibombSmartBomb && ppart->hs.iItem <= ibombAnnihilatorBomb) {
                    dmgFloor = 999;
                } else {
                    dmgFloor = 0;
                }
                if (dmgFloor > 0) {
                    TextIO().pfnCchGetString(dmgFloor == 999 ? idsSmartBombsStrictlyAdditiveHaveMinimumKill : idsIfPlanetHasDefensesBombGuaranteedKill, szWork);
                    c += wsprintf(&szT[c], szWork, 100 * dmgFloor);
                }
                strcpy(szWork, szT);
            }
            WrapTextOut(hdc, &xText, &yText, szWork, c, rcData.left, rcData.right - rcData.left, NULL, FALSE, TRUE);
            xText = rcData.left;
            yText += dyArial8 >> 1;
            if (ppart->hs.iItem == ibombRetroBomb) {
                ids = idsPartRequiresPrimaryRacialTraitClaimAdjuster;
                break;
            }
            if (ppart->pbomb()->dDmgBldg == 0) {
                c = TextIO().pfnCchGetString(idsBombWillDamagePlanetsMinesFactories, szWork);
            } else {
                TextIO().pfnCchGetString(idsBombWillDestroyApproximatelyDPlanetsMines, szT);
                c = wsprintf(szWork, szT, ppart->pbomb()->dDmgBldg);
            }
            WrapTextOut(hdc, &xText, &yText, szWork, c, rcData.left, rcData.right - rcData.left, NULL, TRUE, TRUE);
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
            SelectObject(hdc, rghfontArial8[1]);
            xText = rcData.left;
            yText = rcData.top;
            SelectObject(hdc, rghfontArial8[0]);
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
                c = wsprintf(szWork, TextIO().pfnCompressed(idsCloaksAnyShipReducingRangeWhichScanners), idsT);
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
                c = wsprintf(szWork, TextIO().pfnCompressed(idsIncreasesDamageDoneBeamWeaponsShipD), ppart->hs.iItem == ispecialEEnergyCapacitor ? 10 : 20);
                goto PrintSpecial;
            case ispecialEJammer10:
            case ispecialEJammer50:
                ids = idsJammingDeviceRequiresPrimaryRacialTraitInner;
                /* fallthrough */
            case ispecialEJammer20:
            case ispecialEJammer30:
                idsT = idsHasDChanceDeflectingIncomingTorpedoesDeflected;
                c = wsprintf(szWork, TextIO().pfnCompressed(idsT), ppart->pspecial()->grAbility);
                goto PrintSpecial;
            case ispecialEBattleComputer:
            case ispecialEBattleSuperComputer:
            case ispecialEBattleNexus:
                idsT = idsModuleIncreasesAccuracyTorpedoesDIncreasesInitia;
                c = wsprintf(szWork, TextIO().pfnCompressed(idsT), ppart->pspecial()->grAbility, ppart->hs.iItem - 4);
                goto PrintSpecial;
            }
            if (idsT == static_cast<StringId>(idsNoString))
                break;
            c = TextIO().pfnCchGetString(idsT, szWork);
        PrintSpecial:
            WrapTextOut(hdc, &xText, &yText, szWork, c, rcData.left, rcData.right - rcData.left, NULL, FALSE, TRUE);
            xText = rcData.left;
            yText += dyArial8 * 2;
            break;
        case hstSpecialM:
            ExpandRc(&rcData, -4, -4);
            SelectObject(hdc, rghfontArial8[1]);
            xText = rcData.left;
            yText = rcData.top;
            SelectObject(hdc, rghfontArial8[0]);
            switch (ppart->hs.iItem) {
            default:
                idsT = static_cast<StringId>(idsNoString);
                break;
            case ispecialMManeuveringJet:
            case ispecialMOverthruster:
                c = wsprintf(szWork, TextIO().pfnCompressed(idsIncreasesSpeedBattle1DSquareMovement), ppart->hs.iItem == ispecialMManeuveringJet ? 4 : 2);
                goto PrintSpecial;
            case ispecialMFuelTank:
            case ispecialMSuperFuelTank:
                c = wsprintf(szWork, TextIO().pfnCompressed(idsPodIncreasesFuelCapacityShipDmg), ppart->hs.iItem == ispecialMFuelTank ? 250 : 500);
                goto PrintSpecial;
            case ispecialMCargoPod:
            case ispecialMSuperCargoPod:
                c = wsprintf(szWork, TextIO().pfnCompressed(idsPodIncreasesCargoCapacityShipDkt), ppart->hs.iItem == ispecialMCargoPod ? 50 : 100);
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
            c = TextIO().pfnCchGetString(idsT, szWork);
            goto PrintSpecial;
        case hstSpecialSB:
            ExpandRc(&rcData, -4, -4);
            SelectObject(hdc, rghfontArial8[1]);
            xText = rcData.left;
            yText = rcData.top;
            SelectObject(hdc, rghfontArial8[0]);
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
            c = TextIO().pfnCchGetString(idsT, szWork);
            WrapTextOut(hdc, &xText, &yText, szWork, c, rcData.left, rcData.right - rcData.left, NULL, FALSE, TRUE);
            xText = rcData.left;
            yText += (int16_t)(3 * dyArial8) / 2;
            if (idsT != idsAllowsFleetsWithoutCargoJumpAnyOther) {
                if (idsT == idsAllowsPlanetsFlingMineralPacketsOtherPlanets) {
                    SelectObject(hdc, rghfontArial8[1]);
                    DxStreamTextOut(hdc, &xText, yText, TextIO().pfnCompressed(idsWarp), 0, TRUE);
                    SelectObject(hdc, rghfontArial8[0]);
                    c = wsprintf(szWork, PCTD, ppart->pspecialsb()->grAbility);
                    DxStreamTextOut(hdc, &xText, yText, szWork, c, TRUE);
                    xText = rcData.left;
                    yText += dyArial8;
                    idsT = idsWarningReceivingPlanetMustHaveMassDriver;
                    if (ppart->hs.iItem > ispecialSBMassDriver5) {
                        idsT = static_cast<StringId>(idsT + 1);
                    }
                    c = TextIO().pfnCchGetString(idsT, szWork);
                }
            } else {
                SelectObject(hdc, rghfontArial8[1]);
                DxStreamTextOut(hdc, &xText, yText, TextIO().pfnCompressed(idsSafeHullMass), 0, TRUE);
                SelectObject(hdc, rghfontArial8[0]);
                if (ppart->pspecialsb()->grAbility == -1) {
                    c = TextIO().pfnCchGetString(idsUnlimited, szWork);
                } else {
                    c = wsprintf(szWork, PCTDKT, ppart->pspecialsb()->grAbility);
                }
                DxStreamTextOut(hdc, &xText, yText, szWork, c, TRUE);
                xText = rcData.left;
                yText += dyArial8;
                SelectObject(hdc, rghfontArial8[1]);
                DxStreamTextOut(hdc, &xText, yText, TextIO().pfnCompressed(idsSafeRange), 0, TRUE);
                SelectObject(hdc, rghfontArial8[0]);
                if (ppart->pspecialsb()->grAbility2 == -1) {
                    c = TextIO().pfnCchGetString(idsUnlimited, szWork);
                } else {
                    c = wsprintf(szWork, TextIO().pfnCompressed(idsDLightYears), ppart->pspecialsb()->grAbility2);
                }
                DxStreamTextOut(hdc, &xText, yText, szWork, c, TRUE);
                if (ppart->pspecialsb()->grAbility == -1) {
                    if (ppart->pspecialsb()->grAbility2 == -1) {
                        idsT = static_cast<StringId>(idsNoString);
                    } else {
                        c = wsprintf(szWork, TextIO().pfnCompressed(idsWarningShipsCanSuccessfullyGatedDL), 5 * ppart->pspecialsb()->grAbility2);
                    }
                } else if (ppart->pspecialsb()->grAbility2 == -1) {
                    c = wsprintf(szWork, TextIO().pfnCompressed(idsWarningShipsDktCanSuccessfullyGatedExceeding), 5 * ppart->pspecialsb()->grAbility);
                } else {
                    c = wsprintf(szWork, TextIO().pfnCompressed(idsWarningShipsDktMightSuccessfullyGatedD), 5 * ppart->pspecialsb()->grAbility,
                                 5 * ppart->pspecialsb()->grAbility2);
                }
            }
            if (idsT == static_cast<StringId>(idsNoString))
                break;
            xText = rcData.left;
            yText += (dyArial8 > 14 ? 0 : 4) + dyArial8;
            SelectObject(hdc, rghfontArial7[0]);
            SetTextColor(hdc, 127);
            WrapTextOut(hdc, &xText, &yText, szWork, c, rcData.left, rcData.right - rcData.left, NULL, FALSE, TRUE);
            SelectObject(hdc, rghfontArial8[0]);
            SetTextColor(hdc, 0);
            break;
        case hstMines:
            dxLabel = -1;
            ExpandRc(&rcData, -4, -4);
            SelectObject(hdc, rghfontArial8[1]);
            xText = rcData.left;
            yText = rcData.top;
            rcData.top += 5 * dyArial8;
            for (i = 4; i >= 0; i--) {
                rcData.top -= dyArial8;
                c = TextIO().pfnCchGetString(static_cast<StringId>(idsMinesLaidPerYear + i), szWork);
                if (dxLabel == -1) {
                    dxLabel = LOWORD(GetTextExtent(hdc, szWork, c));
                }
                RightTextOut(hdc, rcData.left + dxLabel, rcData.top, szWork, c, 0);
            }
            SelectObject(hdc, rghfontArial8[0]);
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
            c = wsprintf(szWork, PCTD, 10 * ppart->pmines()->grAbility);
            TextOut(hdc, rcData.left + dxLabel + 4, rcData.top, szWork, c);
            rcData.top += dyArial8;
            c = wsprintf(szWork, TextIO().pfnCompressed(idsWarpD2), iWarp);
            TextOut(hdc, rcData.left + dxLabel + 4, rcData.top, szWork, c);
            rcData.top += dyArial8;
            c = wsprintf(szWork, TextIO().pfnCompressed(idsDD2), pctHit / 10, pctHit % 10);
            TextOut(hdc, rcData.left + dxLabel + 4, rcData.top, szWork, c);
            rcData.top += dyArial8;
            c = wsprintf(szWork, TextIO().pfnCompressed(idsDDEngine), dmgShip, dmgShipRam);
            TextOut(hdc, rcData.left + dxLabel + 4, rcData.top, szWork, c);
            rcData.top += dyArial8;
            c = wsprintf(szWork, TextIO().pfnCompressed(idsDD3), dmgMin, dmgMinRam);
            TextOut(hdc, rcData.left + dxLabel + 4, rcData.top, szWork, c);
            rcData.top += dyArial8;
            SelectObject(hdc, rghfontArial7[0]);
            c = TextIO().pfnCchGetString(idsNumbersParenthesisFleetsContainingShipRamScoop, szWork);
            DrawText(hdc, szWork, c, &rcData, DT_WORDBREAK | DT_NOPREFIX);
            break;
        case hstMining:
            ExpandRc(&rcData, -4, -4);
            SelectObject(hdc, rghfontArial8[1]);
            xText = rcData.left;
            yText = rcData.top;
            SelectObject(hdc, rghfontArial8[0]);
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
                c = TextIO().pfnCchGetString(idsModuleContainsRobotsCapableMining, szWork);
                c += wsprintf(&szWork[c], PCTD, ppart->pmining()->grAbility);
                c += TextIO().pfnCchGetString(idsKtEachMineralDependingConcentrationUninhabitedPl, &szWork[c]);
                if (ppart->hs.iItem != iminingAlienMiner)
                    break;
                ids = idsOriginPartUnknown;
                c += TextIO().pfnCchGetString(idsModuleAlsoActs30Cloak30Jammer, &szWork[c]);
                break;
            case iminingOrbitalAdjuster:
                ids = idsPartRequiresPrimaryRacialTraitClaimAdjuster;
                c = TextIO().pfnCchGetString(idsModifiedMiningRobotTerraformsInhabitedPlanets1, szWork);
            }
            goto PrintSpecial;
        case hstTerra:
            ExpandRc(&rcData, -4, -4);
            SelectObject(hdc, rghfontArial8[1]);
            xText = rcData.left;
            yText = rcData.top;
            SelectObject(hdc, rghfontArial8[0]);
            c = 0;
            if (ppart->hs.iItem <= iterraTotalTerraform30) {
                ids = idsTotalTerraformingRequiresLesserRacialTraitTotal;
                c = TextIO().pfnCchGetString(idsAllowsModifyAnyPlanetsThreeEnvironmentVariables, szT);
                c = wsprintf(szWork, szT, ppart->pterra()->grAbility);
            } else {
                c = TextIO().pfnCchGetString(idsAllowsModifyPlanetsSDOriginalValue, szT);
                c = wsprintf(szWork, szT, rgszPlanetAttr[(int16_t)(ppart->hs.iItem - 8) / 4], ppart->pterra()->grAbility);
            }
            if (c <= 0)
                break;
            WrapTextOut(hdc, &xText, &yText, szWork, c, rcData.left, rcData.right - rcData.left, NULL, FALSE, TRUE);
            break;
        case hstPlanetary:
            ExpandRc(&rcData, -4, -4);
            SelectObject(hdc, rghfontArial8[1]);
            xText = rcData.left;
            yText = rcData.top;
            SelectObject(hdc, rghfontArial8[0]);
            c = 0;
            if (ppart->hs.iItem >= iplanetarySDI && ppart->hs.iItem <= iplanetaryNeutronShield) {
                SelectObject(hdc, rghfontArial8[1]);
                rcData.right -= 2;
                c = TextIO().pfnCchGetString(idsShieldCoverageVsDefenseQuan, szWork);
                CtrTextOut(hdc, ((rcData.right - rcData.left) >> 1) + rcData.left, rcData.top, szWork, c);
                rcData.top += dyArial8;
                cch = TextIO().pfnCchGetString(idsNum, szT);
                dxStr = LOWORD(GetTextExtent(hdc, szT, cch));
                dxQuan = (int16_t)(rcData.right - rcData.left - dxStr - 10) / 5;
                dyPct = (int16_t)(rcData.bottom - rcData.top - dyArial8 - 8) / 5;
                PatBlt(hdc, rcData.left + dxStr + 6, rcData.top, 1, rcData.bottom - rcData.top - dyArial8 - 4, BLACKNESS);
                PatBlt(hdc, rcData.left + dxStr + 6, rcData.bottom - dyArial8 - 4, rcData.right - rcData.left - dxStr - 6, 1, BLACKNESS);
                x = rcData.left + dxStr;
                y = rcData.bottom - dyArial8 - 4 - 5 * dyPct;
                SetTextColor(hdc, 8323072);
                cch = TextIO().pfnCchGetString(idsStandard, szWork);
                RightTextOut(hdc, rcData.right, rcData.bottom - 3 - 3 * dyArial8, szWork, cch, 0);
                SetTextColor(hdc, 127);
                cch = TextIO().pfnCchGetString(idsSmart, szWork);
                RightTextOut(hdc, rcData.right, rcData.bottom - 4 - dyArial8 * 2, szWork, cch, 0);
                pct = 100;
                SetTextColor(hdc, 0);
                while (pct > 0) {
                    c = wsprintf(szWork, PCTDPCTPCT, pct);
                    RightTextOut(hdc, x, y - (dyArial8 >> 1), szWork, c, 0);
                    PatBlt(hdc, x + 4, y, 5, 1, BLACKNESS);
                    pct -= 20;
                    y += dyPct;
                }
                y = rcData.bottom - dyArial8;
                RightTextOut(hdc, x, y, szT, strlen(szT), 0);
                x += 6;
                dxDigit = LOWORD(GetTextExtent(hdc, "0", 1));
                ch = '0';
                for (i = 0; i <= 5; i++) {
                    cch = wsprintf(szWork, PCTD, 20 * i);
                    TextOut(hdc, x - (i == 0 ? dxDigit >> 1 : i == 5 ? (int16_t)(3 * dxDigit) / 2 : dxDigit), y, szWork, cch);
                    if (i > 0) {
                        PatBlt(hdc, x, y - 6, 1, 5, BLACKNESS);
                    }
                    ch++;
                    x += dxQuan;
                }
                hpenSav = (HPEN)SelectObject(hdc, hpenDkBlue);
                xBase = rcData.left + dxStr + 6;
                yBase = rcData.bottom - dyArial8 - 4;
                ldelta = (int16_t)(1000 - ppart->pplanetary()->grAbility);
                for (c = 0; c < 2; c++) {
                    lpct = 1000000;
                    for (i = 0; i <= 100; i++) {
                        x = MulDiv(i, dxQuan, 20) + xBase;
                        y = yBase;
                        y -= LOWORD((int32_t)((1000000 - lpct) * dyPct) / 0x30d40);
                        lpct = (int32_t)(lpct * ldelta) / 1000;
                        if (i == 0) {
                            MoveToEx(hdc, x, y, NULL);
                        } else {
                            LineTo(hdc, x, y);
                        }
                    }
                    ldelta = (int16_t)(1000 - ppart->pplanetary()->grAbility / 2);
                    SelectObject(hdc, hpenRadar);
                }
                SelectObject(hdc, hpenSav);
                c = 0;
                if (ppart->hs.iItem <= iplanetaryMissileBattery) {
                    ids = idsPlanetaryScannersDefensesAvailableAlternateReali;
                } else {
                    ids = idsPlanetaryDefenseUnavailablePrimaryRacialTraitWar;
                }
            } else if (ppart->hs.iItem >= iplanetaryViewer50 && ppart->hs.iItem <= iplanetarySnooper620X) {
                i = ppart->pplanetary()->grAbility;
                c = TextIO().pfnCchGetString(idsEnemyFleetsOrbitingPlanetCanDetectedD, szT);
                c = wsprintf(szWork, szT, abs(i));
                if (i < 0) {
                    WrapTextOut(hdc, &xText, &yText, szWork, c, rcData.left, rcData.right - rcData.left, NULL, FALSE, TRUE);
                    xText = rcData.left;
                    yText += (int16_t)(3 * dyArial8) / 2;
                    c = TextIO().pfnCchGetString(idsScannerCanDeterminePlanetsBasicStatsDistance, szT);
                    c = wsprintf(szWork, szT, -i >> 1);
                    ids = idsScannerWillUnavailableIfHaveLesserRacial;
                } else {
                    ids = idsPlanetaryScannersDefensesAvailableAlternateReali;
                }
            } else if (ppart->hs.iItem == iplanetaryGenesisDevice) {
                ids = idsOriginProcessUnknown;
                c = TextIO().pfnCchGetString(idsProcessGivesPlanetNewBirthTracesCivilization, szWork);
            }
            if (c > 0) {
                WrapTextOut(hdc, &xText, &yText, szWork, c, rcData.left, rcData.right - rcData.left, NULL, FALSE, TRUE);
            }
        }
        if (ids != static_cast<StringId>(idsNoString)) {
            c = TextIO().pfnCchGetString(ids, szWork);
            SetRect(&rcT, 4, dy - dyArial8 * 2, dx - 4, dy - 5);
            if (dyArial8 > 14) {
                SelectObject(hdc, rghfontArial6[0]);
            } else {
                SelectObject(hdc, rghfontArial7[0]);
            }
            SetTextColor(hdc, l == -1 ? 127 : 0);
            DrawText(hdc, szWork, c, &rcT, DT_WORDBREAK | DT_NOPREFIX);
            if (l == -1) {
                SetTextColor(hdc, 0);
            }
        }
    }
    return;
}
