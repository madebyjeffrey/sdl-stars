#include "win.h"
#include "productionpresets.h"
#include "text_io.h"
#include "control_io.h"
#include "production_io.h"
#include "presentation_io.h"
#include "race_query_io.h"
#include "alert_io.h"

int16_t ChangeProduction(int16_t fClear) {
    FARPROC  lpProcProd;
    PROD     rgprod[64];
    int16_t  fSuccess;
    StarsRecoveryScope scopeRecovery;
    try {
        if (fClear) {
            fSuccess = 1;
            goto LWriteProdQ;
        }
        ProductionIO().pfnInitialize(rgprod);
        fDlgUp = TRUE;
        lpProcProd = MakeProcInstance(ProductionDlg, hInst);
        fSuccess = ControlIO().pfnDialogBox(hInst, MAKEINTRESOURCE(IDD_PRODUCTION), hwndFrame, lpProcProd, 0);
        FreeProcInstance(lpProcProd);
        hwndProdDlg = 0;
        fDlgUp = FALSE;
LWriteProdQ:
        FinishProduction(fSuccess);
        if (fSuccess && sel.grobj == grobjPlanet) {
            ControlIO().pfnDrawPlanShip(NULL, tilePlanetStats);
        }
        return TRUE;
    } catch (const StarsError &) {
        if (lpplProdGlob) {
            FreePl(lpplProdGlob);
        }
        lpplProdGlob = NULL;
        if (hwndProdDlg) {
            ControlIO().pfnEndDialog(hwndProdDlg, 0);
        }
        hwndProdDlg = 0;
        fDlgUp = FALSE;
        AlertIO().pfnAlertSz(TextIO().pfnFormat(idsThereIsntEnoughFreeMemoryModifyProduction, NULL), MB_ICONHAND);
        return FALSE;
    }
}

INT_PTR CALLBACK ProductionDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC                hdc;
    PAINTSTRUCT        ps;
    RECT               rc;
    int16_t            dxPBtn;
    int16_t            dy;
    RECT               rcT;
    int16_t            i;
    int16_t            xCtr;
    int16_t            dx;
    int16_t            dyLB;
    int16_t            rgidProdBtns[10];
    DRAWITEMSTRUCT    *lpdis;
    MEASUREITEMSTRUCT *lpmis;
    POINT16            pt;
    int16_t            cMax;
    char               sz255[2];
    char              *rgszZip[6];
    HCURSOR            hcs;

    switch (message) {
    case WM_INITDIALOG:
        rgidProdBtns[0] = 1070;
        rgidProdBtns[1] = 1071;
        rgidProdBtns[2] = 1;
        rgidProdBtns[3] = 2;
        rgidProdBtns[4] = 1048;
        rgidProdBtns[5] = 1049;
        rgidProdBtns[6] = 1081;
        rgidProdBtns[7] = 1082;
        rgidProdBtns[8] = 1069;
        rgidProdBtns[9] = 118;
        hwndProdDlg = hwnd;
        if (rgplr[idPlayer].cPlanet <= 1) {
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_NEXT), FALSE);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_BACK), FALSE);
        }
        if (gd.mdScreenSize >= 1) {
            dx = 760;
            dy = 580;
        } else {
            dx = 610;
            dy = 24 * dyArial8 + 24;
        }
        ControlIO().pfnSetWindowPos(hwnd, NULL, 0, 0, dx, dy, SWP_NOMOVE | SWP_NOZORDER);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        xCtr = rc.right >> 1;
        dyLB = rc.bottom - (int16_t)(17 * dyArial8) / 2 - 24;
        rc.left = (int16_t)(11 * rc.right) / 20 + 16;
        dxPBtn = (int16_t)(rc.right - rc.left) / 4;
        rc.bottom -= (int16_t)(3 * dyArial8) / 2 + 6;
        ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY), NULL, 6, rc.bottom, rc.left - 12, (int16_t)(3 * dyArial8) / 2, SWP_NOZORDER);
        for (i = 0; i < 4; i++) {
            ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, rgidProdBtns[i]), NULL, rc.left, rc.bottom, dxPBtn - 6, (int16_t)(3 * dyArial8) / 2, SWP_NOZORDER);
            rc.left += dxPBtn;
        }
        dxPBtn += 24;
        rc.left = xCtr - ((dxPBtn - 6) >> 1);
        dy = (int16_t)(dyLB - 9 * dyArial8) / 5 + (int16_t)(3 * dyArial8) / 2 - 3;
        rc.top = 8;
        if (dy > 50) {
            rc.top += (int16_t)((dy - 50) * 5) / 2;
            dy = 50;
        }
        for (i = 4; i < 10; i++) {
            ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, rgidProdBtns[i]), NULL, rc.left, rc.top, dxPBtn - 6, (int16_t)(3 * dyArial8) / 2, SWP_NOZORDER);
            rc.top += dy;
        }
        ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), NULL, 6, 6, rc.left - 12, dyLB, SWP_NOZORDER);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), &rcT);
        ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), NULL, rc.left + dxPBtn, 6, rc.left - 12, rcT.bottom - rcT.top, SWP_NOZORDER);
        win32::ScreenToClient(hwnd, (POINT *)&rcT.right);
        yTopFutureTech = rcT.bottom;
        InitializeProductionDlg(hwnd);
        if (gd.mdScreenSize == 1 && ptStickyProduceDlg.y == -1) {
            ptStickyProduceDlg.y = 0;
        }
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyProduceDlg, TRUE);
        return 1;
    case WM_DRAWITEM:
        lpdis = (DRAWITEMSTRUCT *)lParam;
        if (lpdis->itemID == -1) {
            HandleFocusState(lpdis, -2);
        } else {
            switch (lpdis->itemAction) {
            case ODA_DRAWENTIRE:
            case ODA_SELECT:
            case ODA_FOCUS:
                DrawCBEntireItem(lpdis, 4);
            }
        }
        return 1;
    case WM_MEASUREITEM:
        lpmis = (MEASUREITEMSTRUCT *)lParam;
        lpmis->itemHeight = dyArial8 + 2;
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        if ((HWND)lParam == ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY) || message == WM_CTLCOLORSTATIC) {
            win32::SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        if (win32::PtInRect(&rcProdDiamond, PointFrom16(pt)) == 0)
            break;
        if (message == WM_LBUTTONDOWN) {
            GlobalPD.grPopup = grPopupString;
            GlobalPD.dxOut = 180;
            GlobalPD.psz = szPopupBuffer;
            TextIO().pfnCchGetString(idsRightClickBlueDiamondApplyProductionTemplate, szPopupBuffer);
            Popup(hwnd, pt.x, pt.y);
            break;
        }
        sz255[0] = -1;
        sz255[1] = 0;
        cMax = 0;
        for (i = 0; i < 4; i++) {
            if (vrgZipProd[i].fValid) {
                rgszZip[cMax++] = vrgZipProd[i].szName;
            }
        }
        rgszZip[cMax++] = sz255;
        rgszZip[cMax++] = TextIO().pfnCompressed(idsCustomize);
        i = ControlIO().pfnPopupMenu(hwnd, pt.x, pt.y, cMax, NULL, rgszZip, -1, TRUE);
        if (i == cMax - 1) {
            CustomizeProductionPresets(hwnd);
            break;
        }
        if (i < 0)
            break;
        cMax = IProductionPresetSlot(i);
        ProdCommandHandler(hwnd, 0x816, cMax);
        break;
    case WM_SETCURSOR:
        hcs = 0;
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (win32::PtInRect(&rcProdDiamond, PointFrom16(pt)) == 0)
            break;
        win32::SetCursor(hcurArrowHelp);
        return 1;
    case WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        ControlIO().pfnDrawProductionDlg(hwnd, hdc, &rc, -1);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case WM_COMMAND:
        ProdCommandHandler(hwnd, wParam, lParam);
    }
    return 0;
}

void ProdCommandHandler(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    int32_t lSel;
    int16_t iSrc;
    HWND    hwndLB;
    int16_t c;
    int16_t iDst;
    PROD    prodLast;
    int16_t ipl;
    int16_t fRefillSrc;
    int16_t iMac;
    RECT    rc;
    PROD   *lpprod;
    PROD    prod;
    int16_t cMax;
    PLPROD *lpplprodT;

    switch (LOWORD(wParam)) {
    case IDC_PRODUCTION_ADD:
    AddItem:
        lSel = ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), LB_GETCURSEL, 0, 0);
        if (lSel < 0)
            break;
        for (iSrc = 0; iSrc < cProdGlob && (pProdGlob[iSrc].cItem == 0 || lSel-- != 0); iSrc++) {
        }
        prod = pProdGlob[iSrc];
        if (ControlIO().pfnGetAsyncKeyState(VK_CONTROL) & 0xfffe) {
            if (ControlIO().pfnGetAsyncKeyState(VK_SHIFT) & 0xfffe) {
                prod.cItem = LOWORD(prod.cItem < 1020 ? (uint32_t)prod.cItem : 1020);
            } else {
                prod.cItem = LOWORD(prod.cItem < 100 ? (uint32_t)prod.cItem : 100);
            }
        } else if (ControlIO().pfnGetAsyncKeyState(VK_SHIFT) & 0xfffe) {
            prod.cItem = LOWORD(prod.cItem < 10 ? (uint32_t)prod.cItem : 10);
        } else {
            prod.cItem = 1;
        }
        if (pProdGlob[iSrc].cItem != 0x3ff) {
            pProdGlob[iSrc].cItem -= prod.cItem;
        }
        iMac = lpplProdGlob->iprodMac;
        lSel = ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_GETCURSEL, 0, 0);
        if (lSel < 0) {
            iDst = iMac - 1;
            if (iDst < 0) {
                iDst = 0;
            }
        } else {
            iDst = LOWORD(lSel) - 1;
        }
        if (iDst < iMac) {
            if (iDst >= 0) {
                prodLast = lpplProdGlob->rgprod()[iDst];
                if ((uint32_t)prodLast.iItem == prod.iItem && (uint32_t)prodLast.grobj == prod.grobj) {
                RingItUp:
                    lpplProdGlob->rgprod()[iDst].cItem = StarsMin(1020, lpplProdGlob->rgprod()[iDst].cItem + prod.cItem);
                    if (lpplProdGlob->rgprod()[iDst].cItem > 1 && lpplProdGlob->rgprod()[iDst].iItem == iobjAlchemy &&
                        lpplProdGlob->rgprod()[iDst].grobj == grobjPlanet) {
                        lpplProdGlob->rgprod()[iDst].cItem = 1;
                    }
                    goto FixedUp;
                }
            }
            iDst++;
            if (iDst < iMac) {
                prodLast = lpplProdGlob->rgprod()[iDst];
                if ((uint32_t)prodLast.iItem == prod.iItem && (uint32_t)prodLast.grobj == prod.grobj)
                    goto RingItUp;
            }
        }
        if (iMac >= 40) {
            ControlIO().pfnMessageBeep(MB_OK);
            goto RedrawText;
        }
        if (iMac == lpplProdGlob->iprodMax) {
            lpplProdGlob = LpplReAlloc(lpplProdGlob, iMac + 4);
        }
        if (iDst != iMac) {
            memmove(&lpplProdGlob->rgprod()[iDst + 1], &lpplProdGlob->rgprod()[iDst], (iMac - iDst) * 4);
        }
        lpplProdGlob->rgprod()[iDst] = prod;
        lpplProdGlob->iprodMac++;
    FixedUp:
        ControlIO().pfnFillPlanetProdLB(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), lpplProdGlob, NULL);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_SETCURSEL, iDst + 1, 0);
        if (pProdGlob[iSrc].cItem != 0)
            goto RedrawText;
        ControlIO().pfnFillProdSrcLB(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), -1);
        goto RedrawText;
    case IDC_PRODUCTION_REMOVE:
    RemoveItem:
        lSel = ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_GETCURSEL, 0, 0);
        if (lSel <= 0)
            break;
        iMac = lpplProdGlob->iprodMac;
        lSel--;
        prod = lpplProdGlob->rgprod()[lSel];
        for (iSrc = 0; iSrc < cProdGlob && ((uint32_t)pProdGlob[iSrc].grobj != prod.grobj || (uint32_t)pProdGlob[iSrc].iItem != prod.iItem); iSrc++) {
        }
        fRefillSrc = pProdGlob[iSrc].cItem == 0;
        if (ControlIO().pfnGetAsyncKeyState(VK_CONTROL) & 0xfffe) {
            if (ControlIO().pfnGetAsyncKeyState(VK_SHIFT) & 0xfffe) {
                c = 1020;
            } else {
                c = 100;
            }
        } else if (ControlIO().pfnGetAsyncKeyState(VK_SHIFT) & 0xfffe) {
            c = 10;
        } else {
            c = 1;
        }
        if (prod.grobj == grobjPlanet && prod.iItem == iobjAlchemy) {
            c = 1020;
        }
        c = c >= (int32_t)prod.cItem ? prod.cItem : c;
        if (pProdGlob[iSrc].cItem != 0x3ff) {
            pProdGlob[iSrc].cItem += c;
        }
        lpplProdGlob->rgprod()[lSel].cItem -= c;
        if (lpplProdGlob->rgprod()[lSel].cItem == 0) {
            if ((int32_t)(lSel + 1) < iMac) {
                memmove(&lpplProdGlob->rgprod()[lSel], &lpplProdGlob->rgprod()[lSel + 1], (iMac - LOWORD(lSel) - 1) * sizeof(PROD));
            } else {
                lSel--;
            }
            lpplProdGlob->iprodMac--;
        }
        ControlIO().pfnFillPlanetProdLB(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), lpplProdGlob, NULL);
        if (lSel >= 0) {
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_SETCURSEL, LOWORD(lSel) + 1, 0);
        }
        if (!fRefillSrc)
            goto RedrawText;
        ControlIO().pfnFillProdSrcLB(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), -1);
        goto RedrawText;
    case IDC_PRODUCTION_CLEAR:
    case IDC_IMPORT:
        ipl = 0;
        lpprod = lpplProdGlob->rgprod();
        while (ipl < lpplProdGlob->iprodMac) {
            for (iSrc = 0; iSrc < cProdGlob && ((uint32_t)pProdGlob[iSrc].grobj != lpprod->grobj || (uint32_t)pProdGlob[iSrc].iItem != lpprod->iItem); iSrc++) {
            }
            if (pProdGlob[iSrc].cItem != 0x3ff) {
                pProdGlob[iSrc].cItem += lpprod->cItem;
            }
            ipl++;
            lpprod++;
        }
        if (LOWORD(wParam) == IDC_IMPORT) {
            cMax = lpplProdGlob->iprodMac + vrgZipProd[lParam].cpq;
            if (cMax < 1) {
                cMax = 1;
            }
            lpplprodT = LpplAlloc<PLPROD>(4, cMax, htOrd);
            memset(lpplprodT->rgprod(), 0, cMax * 4);
            iDst = 0;
            for (iSrc = 0; iSrc < lpplProdGlob->iprodMac; iSrc++) {
                if (lpplProdGlob->rgprod()[iSrc].grobj != grobjPlanet || lpplProdGlob->rgprod()[iSrc].iItem >= mdIdleFactory) {
                    lpplprodT->rgprod()[iDst] = lpplProdGlob->rgprod()[iSrc];
                    iDst++;
                }
            }
            for (iSrc = 0; iSrc < vrgZipProd[lParam].cpq; iSrc++) {
                if ((RaceQueryIO().pfnStat(&rgplr[idPlayer], rsMajorAdv) != raMacintosh || vrgZipProd[lParam].rgpq[iSrc].mdIdle() > iobjDefense) &&
                    (RaceQueryIO().pfnStat(&rgplr[idPlayer], rsMajorAdv) != raTerra ||
                     (vrgZipProd[lParam].rgpq[iSrc].mdIdle() != iobjMinTerraform && vrgZipProd[lParam].rgpq[iSrc].mdIdle() != iobjMaxTerraform))) {
                    lpplprodT->rgprod()[iDst].grobj = grobjPlanet;
                    lpplprodT->rgprod()[iDst].iItem = vrgZipProd[lParam].rgpq[iSrc].mdIdle();
                    lpplprodT->rgprod()[iDst].cItem = vrgZipProd[lParam].rgpq[iSrc].cQuan();
                    iDst++;
                }
            }
            lpplprodT->iprodMac = iDst;
            FreePl(lpplProdGlob);
            lpplProdGlob = lpplprodT;
            sel.pl.fNoResearch = vrgZipProd[lParam].fNoResearch;
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY), BM_SETCHECK, sel.pl.fNoResearch, 0);
        } else {
            lpplProdGlob->iprodMac = 0;
        }
        ControlIO().pfnFillPlanetProdLB(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), lpplProdGlob, NULL);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_SETCURSEL, 0, 0);
        ControlIO().pfnFillProdSrcLB(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), -1);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), LB_SETCURSEL, 0, 0);
        goto RedrawText;
    case IDC_PRODUCTION_AVAILABLE_ITEMS:
    case IDC_PRODUCTION_QUEUE:
        if (HIWORD(wParam) == 1) {
        RedrawText:
            ControlIO().pfnGetClientRect(hwnd, &rc);
            rc.top = yTopFutureTech;
            rc.bottom = 7 * dyArial8 + rc.top;
            rc.left += 130;
            ControlIO().pfnInvalidateRect(hwnd, &rc, TRUE);
            ControlIO().pfnDrawProductionDlg(hwnd, NULL, &rc, -1);
        } else if (HIWORD(wParam) == 2) {
            if (LOWORD(wParam) == IDC_PRODUCTION_AVAILABLE_ITEMS)
                goto AddItem;
            goto RemoveItem;
        }
        break;
    case IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY:
        hwndLB = ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE);
        sel.pl.fNoResearch = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY), BM_GETCHECK, 0, 0));
        lSel = ControlIO().pfnSendMessage(hwndLB, LB_GETCURSEL, 0, 0);
        ControlIO().pfnFillPlanetProdLB(hwndLB, lpplProdGlob, NULL);
        ControlIO().pfnSendMessage(hwndLB, LB_SETCURSEL, LOWORD(lSel), 0);
        goto RedrawText;
    case IDOK:
    case IDCANCEL:
        hwndProdDlg = 0;
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyProduceDlg, FALSE);
        ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
        break;
    case IDC_BACK:
    case IDC_NEXT:
        c = LOWORD(wParam) == IDC_NEXT ? 1 : -1;
        FinishProduction(TRUE);
        if (ControlIO().pfnGetKeyState(VK_SHIFT) < 0) {
            PresentationIO().pfnSelectPlanet(0, IdFindAdjStarbase(sel.pl.id, LOWORD(wParam) == IDC_NEXT));
        } else {
            PresentationIO().pfnSelectPlanet(c, 0);
        }
        ProductionIO().pfnInitialize(NULL);
        InitializeProductionDlg(hwnd);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        rc.top = yTopFutureTech;
        rc.bottom = 9 * dyArial8 + rc.top;
        ControlIO().pfnInvalidateRect(hwnd, &rc, TRUE);
        break;
    case IDC_PRODUCTION_ITEM_DOWN:
        lSel = ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_GETCURSEL, 0, 0);
        iMac = lpplProdGlob->iprodMac;
        if (lSel <= 0 || lSel >= iMac)
            break;
        lSel--;
        prod = lpplProdGlob->rgprod()[lSel];
        lpplProdGlob->rgprod()[lSel] = lpplProdGlob->rgprod()[lSel + 1];
        lpplProdGlob->rgprod()[lSel + 1] = prod;
        ControlIO().pfnFillPlanetProdLB(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), lpplProdGlob, NULL);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_SETCURSEL, LOWORD(lSel) + 2, 0);
        goto RedrawText;
    case IDC_PRODUCTION_ITEM_UP:
        lSel = ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_GETCURSEL, 0, 0);
        if (lSel <= 1)
            break;
        iMac = lpplProdGlob->iprodMac;
        lSel -= 2;
        prod = lpplProdGlob->rgprod()[lSel];
        lpplProdGlob->rgprod()[lSel] = lpplProdGlob->rgprod()[lSel + 1];
        lpplProdGlob->rgprod()[lSel + 1] = prod;
        ControlIO().pfnFillPlanetProdLB(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), lpplProdGlob, NULL);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_SETCURSEL, LOWORD(lSel) + 1, 0);
        goto RedrawText;
    case IDC_HELP:
        win32::WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhProductionDialog);
    }
}

void InitializeProductionDlg(HWND hwnd) {
    char    rgch[86];
    int16_t i;
    int16_t iSel;
    PROD   *lpprod;

    iSel = -1;
    win32::wsprintf(rgch, TextIO().pfnCompressed(idsProductionQueueS), TextIO().pfnPlanetName(sel.pl.id));
    ControlIO().pfnSetWindowText(hwnd, rgch);
    ControlIO().pfnFillProdSrcLB(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), -1);
    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), LB_SETCURSEL, 0, 0);
    i = 0;
    lpprod = lpplProdGlob->rgprod();
    while (i < lpplProdGlob->iprodMac) {
        if (lpprod->grobj != grobjPlanet || lpprod->iItem >= mdIdleFactory) {
            iSel = i;
        }
        i++;
        lpprod++;
    }
    ControlIO().pfnFillPlanetProdLB(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), lpplProdGlob, NULL);
    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), LB_SETCURSEL, iSel + 1, 0);
    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY), BM_SETCHECK, sel.pl.fNoResearch, 0);
    return;
}

void DrawProductionDlg(HWND hwnd, HDC hdc, RECT *prc, int16_t iDraw) {
    int32_t lSel;
    int16_t iSrc;
    int16_t idc;
    int16_t fCreatedDC;
    int16_t i;
    int16_t c;
    int32_t rgCost[4];
    int16_t dxkT;
    int16_t k;
    RECT    rc;
    PROD    prod;
    char    szT[100];

    fCreatedDC = FALSE;
    if (!hdc) {
        fCreatedDC = TRUE;
        hdc = win32::GetDC(hwnd);
    }
    win32::SelectObject(hdc, rghfontArial8[0]);
    dxkT = LOWORD(GetTextExtent(hdc, TextIO().pfnCompressed(idsKt), 2));
    win32::SelectObject(hdc, rghfontArial8[1]);
    win32::SetBkColor(hdc, crButtonFace);
    for (i = 0; i < 2; i++) {
        idc = i == 0 ? 1046 : 1047;
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, idc), &rc);
        win32::ScreenToClient(hwnd, (POINT *)&rc);
        win32::ScreenToClient(hwnd, (POINT *)&rc.right);
        lSel = ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, idc), LB_GETCURSEL, 0, 0);
        if (lSel >= 0 && (lSel != 0 || i != 1)) {
            if (i == 0) {
                for (iSrc = 0; iSrc < cProdGlob && (pProdGlob[iSrc].cItem == 0 || lSel-- != 0); iSrc++) {
                }
                prod = pProdGlob[iSrc];
                prod.cItem = 1;
            } else {
                lSel--;
                prod = lpplProdGlob->rgprod()[lSel];
            }
            ProductionIO().pfnQuote(&sel.pl, &prod, (uint32_t *)rgCost, idPlayer, FALSE);
            rc.bottom = yTopFutureTech + 4;
            win32::SelectObject(hdc, rghfontArial8[1]);
            c = TextIO().pfnCchGetString(idsRequiredMinerals, szWork);
            win32::TextOut(hdc, rc.left, rc.bottom, szWork, c);
            rc.left += 20;
            rc.right -= 20;
            for (k = 0; k <= 3; k++) {
                c = k == 3 ? 5 : k;
                rc.bottom += dyArial8;
                win32::SelectObject(hdc, rghfontArial8[1]);
                win32::SetTextColor(hdc, rgcrMinerals[c]);
                win32::TextOut(hdc, rc.left, rc.bottom, rgszMinerals[c], win32::lstrlen(rgszMinerals[c]));
                win32::SelectObject(hdc, rghfontArial8[0]);
                win32::SetTextColor(hdc, crWindowText);
                c = win32::wsprintf(szWork, PCTLD, rgCost[k]);
                RightTextOut(hdc, rc.right - dxkT - 2, rc.bottom, szWork, c, dxMaxMineralQuan);
                if (k <= 2) {
                    win32::TextOut(hdc, rc.right - dxkT, rc.bottom, TextIO().pfnCompressed(idsKt), 2);
                }
            }
            if (i != 0) {
                rc.bottom += (int16_t)(3 * dyArial8) / 2;
                win32::SelectObject(hdc, rghfontArial8[1]);
                c = win32::wsprintf(szT, TextIO().pfnCompressed(idsDDoneCompletion), prod.pct);
                if (PszProductionETA(&sel.pl, lpplProdGlob, static_cast<ProdItemType>(LOWORD(lSel)), NULL, NULL) != szWork) {
                }
                strcpy(&szT[c], szWork);
                win32::TextOut(hdc, rc.left - 20, rc.bottom, szT, strlen(szT));
                win32::SelectObject(hdc, rghfontArial8[0]);
            }
        }
    }
    ControlIO().pfnGetClientRect(hwnd, &rc);
    rc.top = rc.bottom - ((int16_t)(5 * dyArial8) / 2 + 12);
    rc.bottom = (dyArial8 | 1) + rc.top;
    rc.left = 6;
    rc.right = rc.left + dyArial8;
    DrawDiamond(hdc, &rc, hbrBBlue);
    rcProdDiamond = rc;
    win32::SelectObject(hdc, rghfontArial8[1]);
    c = TextIO().pfnCchGetString(idsApplyDefineProductionTemplate, szWork);
    win32::TextOut(hdc, rc.right + 4, rc.top, szWork, c);
    if (fCreatedDC) {
        win32::ReleaseDC(hwnd, hdc);
    }
    return;
}

void FillProdSrcLB(HWND hwndLB, int16_t mdFill) {
    char    szT[80];
    int16_t i;
    char   *psz;

    for (i = 0; i < 6; i++) {
        szT[i] = ' ';
    }
    ControlIO().pfnSendMessage(hwndLB, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < cProdGlob; i++) {
        if (pProdGlob[i].cItem > 0) {
            psz = PszNameProdItem(pProdGlob + i);
            strcpy(&szT[6], psz);
            if (pProdGlob[i].grobj == grobjFleet) {
                szT[0] = pProdGlob[i].iItem < iobjPacketGerm ? 42 : 35;
            } else if (pProdGlob[i].iItem < mdIdleFactory) {
                szT[0] = 'I';
                strcat(&szT[6], " (Auto Build)");
            } else {
                szT[0] = ' ';
            }
            ControlIO().pfnSendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)szT);
        }
    }
    return;
}

INT_PTR CALLBACK ZipProdDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    HDC         hdc;
    PAINTSTRUCT ps;
    int16_t     i;
    int16_t     iBase;
    RECT        rc;
    int16_t     dy;
    RECT        rc2;
    HWND        hwndRad;
    char       *psz;
    char       *pszT;
    RECT        rcGBox;
    int16_t     cch;
    FARPROC     lpProc;
    int16_t     cpq;

    switch (message) {
    case WM_INITDIALOG:
        ControlIO().pfnSetWindowText(hwnd, TextIO().pfnCompressed(idsCustomizeProductionTemplates));
        win32::GetWindowRect(hwnd, &rc);
        ControlIO().pfnGetClientRect(hwnd, &rc2);
        dy = rc.bottom - rc.top - rc2.bottom;
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_ZIP_PROD_QUEUE), &rc2);
        win32::MapWindowPoints(NULL, hwnd, (POINT *)&rc2, 2);
        vyZPDStatic = rc2.bottom + 2;
        dy += rc2.bottom + dyArial8 + 6;
        ControlIO().pfnSetWindowPos(hwnd, NULL, 0, 0, rc.right - rc.left, dy, SWP_NOMOVE | SWP_NOZORDER);
        win32::CheckRadioButton(hwnd, IDC_ZIP_PROD_PRESET_1, IDC_ZIP_PROD_PRESET_4, IDC_ZIP_PROD_PRESET_1);
        ControlIO().pfnEnableZipProdBtns(hwnd, 0);
        iResTechNow = Energy;
        ControlIO().pfnFillZipProdLB(hwnd, &vrgZipProd[iResTechNow]);
        for (i = 1073; i <= 1076; i++) {
            iBase = i - 1073;
            if (vrgZipProd[iBase].fValid) {
                pszT = szWork;
                psz = vrgZipProd[iBase].szName;
                while (*psz != 0) {
                    if ((*pszT++ = *psz++) == '&') {
                        *pszT++ = '&';
                    }
                }
                *pszT = 0;
                psz = szWork;
            } else {
                psz = TextIO().pfnCompressed(idsUnusedD);
                win32::wsprintf(szWork, psz, iBase + 1);
                psz = szWork;
            }
            hwndRad = ControlIO().pfnGetDlgItem(hwnd, i);
            ControlIO().pfnSetWindowText(hwndRad, psz);
        }
        ControlIO().pfnStickyDlgPos(hwnd, &ptStickyZipProdDlg, TRUE);
        if (gd.fTutorial) {
            PresentationIO().pfnTutor();
        }
        return 1;
    case WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect((HDC)wParam, &rc, hbrButtonFace);
        return 1;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        for (i = 1073; i <= 1076 && (HWND)lParam != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 1076) {
            win32::SetBkColor((HDC)wParam, crButtonFace);
            return (INT_PTR)hbrButtonFace;
        }
        break;
    case WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_ZIP_PROD_PRESET_1), &rcGBox);
        win32::ScreenToClient(hwnd, (POINT *)&rcGBox);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_ZIP_PROD_PRESET_4), &rc);
        win32::ScreenToClient(hwnd, (POINT *)&rc.right);
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, dyArial8, dyArial8 >> 1);
        _Draw3dFrame(hdc, &rcGBox, -1);
        win32::SelectObject(hdc, rghfontArial8[1]);
        win32::SetBkColor(hdc, crButtonFace);
        cch = TextIO().pfnCchGetString(idsCustomOrders, szWork);
        win32::TextOut(hdc, rcGBox.left + 8, rcGBox.top - (dyArial8 >> 1), szWork, cch);
        rcGBox.top = rcGBox.bottom + 8;
        if (vrgZipProd[iResTechNow].fValid) {
            cch = TextIO().pfnCchGetString(static_cast<StringId>(idsContributeResearch + vrgZipProd[iResTechNow].fNoResearch), szWork);
            win32::TextOut(hdc, rcGBox.left, vyZPDStatic, szWork, cch);
        }
        win32::EndPaint(hwnd, &ps);
        return 1;
    case WM_COMMAND:
        if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_ZIP_PROD_PRESET_1 && LOWORD(wParam) <= IDC_ZIP_PROD_PRESET_4) {
            iResTechNow = static_cast<TechFieldType>(LOWORD(wParam) - 1073);
            ControlIO().pfnEnableZipProdBtns(hwnd, iResTechNow);
            ControlIO().pfnFillZipProdLB(hwnd, &vrgZipProd[iResTechNow]);
        } else {
            switch (LOWORD(wParam)) {
            case IDOK:
            case IDCANCEL:
                ControlIO().pfnStickyDlgPos(hwnd, &ptStickyZipProdDlg, FALSE);
                ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
                vyZPDStatic = -1;
                if (gd.fTutorial) {
                    PresentationIO().pfnTutor();
                }
                return 1;
            case IDC_IMPORT:
            case IDC_RENAME:
                if (vrgZipProd[iResTechNow].fValid) {
                    strcpy(szWork, vrgZipProd[iResTechNow].szName);
                } else {
                    win32::wsprintf(szWork, TextIO().pfnCompressed(idsCustomD), iResTechNow);
                }
                lpProc = MakeProcInstance(RenameZipDlg, hInst);
                if (iResTechNow == Energy)
                    goto LDontRename;
                if (ControlIO().pfnDialogBox(hInst, MAKEINTRESOURCE(IDD_RENAME), hwndFrame, lpProc, 0) != 0) {
                    if (szWork[0] == 0) {
                        win32::wsprintf(szWork, TextIO().pfnCompressed(idsCustomD), iResTechNow);
                    }
                    strcpy(vrgZipProd[iResTechNow].szName, szWork);
                    pszT = &szWork[64];
                    psz = szWork;
                    while (*psz != 0) {
                        if ((*pszT++ = *psz++) == '&') {
                            *pszT++ = '&';
                        }
                    }
                    *pszT = 0;
                    ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, iResTechNow + 1073), &szWork[64]);
                LDontRename:
                    if (LOWORD(wParam) == IDC_IMPORT) {
                        vrgZipProd[iResTechNow].fValid = TRUE;
                        cpq = 0;
                        for (i = 0; i < lpplProdGlob->iprodMac; i++) {
                            if (lpplProdGlob->rgprod()[i].grobj == grobjPlanet && lpplProdGlob->rgprod()[i].iItem < mdIdleFactory) {
                                vrgZipProd[iResTechNow].rgpq[cpq].SetMdIdle(lpplProdGlob->rgprod()[i].iItem);
                                vrgZipProd[iResTechNow].rgpq[cpq].SetCQuan(lpplProdGlob->rgprod()[i].cItem);
                                cpq++;
                                if (cpq >= 12)
                                    break;
                            }
                        }
                        vrgZipProd[iResTechNow].cpq = cpq;
                        vrgZipProd[iResTechNow].fNoResearch = sel.pl.fNoResearch;
                        ControlIO().pfnFillZipProdLB(hwnd, &vrgZipProd[iResTechNow]);
                    }
                    ControlIO().pfnEnableZipProdBtns(hwnd, iResTechNow);
                }
                FreeProcInstance(lpProc);
                ControlIO().pfnSetFocus(hwnd);
                gd.fChgZipProd = TRUE;
                break;
            case IDC_DELETE:
                vrgZipProd[iResTechNow].fValid = FALSE;
                win32::wsprintf(szWork, TextIO().pfnCompressed(idsUnusedD), iResTechNow + 1);
                ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, iResTechNow + 1073), szWork);
                ControlIO().pfnFillZipProdLB(hwnd, &vrgZipProd[iResTechNow]);
                gd.fChgZipProd = TRUE;
                break;
            case IDC_HELP:
                win32::WinHelp(hwnd, szHelpFile, HELP_CONTEXT, idhCustomizeProductionTemplatesDialog);
                return 1;
            }
        }
    }
    return 0;
}

void EnableZipProdBtns(HWND hwnd, int16_t iSel) {
    int16_t fEnabled;

    fEnabled = vrgZipProd[iSel].fValid != 0 && iSel > 0;
    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DELETE), fEnabled);
    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_RENAME), fEnabled);
    return;
}

void FillZipProdLB(HWND hwndDlg, ZIPPRODQ *pzpq) {
    int16_t i;
    HWND    hwndLB;
    char    szAuto[40];
    char    szFormat[15];
    RECT    rc;

    hwndLB = ControlIO().pfnGetDlgItem(hwndDlg, IDC_ZIP_PROD_QUEUE);
    ControlIO().pfnGetClientRect(hwndDlg, &rc);
    rc.top = vyZPDStatic;
    rc.bottom = vyZPDStatic + dyArial8;
    ControlIO().pfnInvalidateRect(hwndDlg, &rc, TRUE);
    ControlIO().pfnSendMessage(hwndLB, LB_RESETCONTENT, 0, 0);
    if (!pzpq->fValid || pzpq->cpq == 0) {
        ControlIO().pfnSendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)TextIO().pfnCompressed(idsAutoBuildOrders));
    } else {
        TextIO().pfnCchGetString(idsSD2, szFormat);
        for (i = 0; i < pzpq->cpq; i++) {
            TextIO().pfnCchGetString(static_cast<StringId>(idsMines + pzpq->rgpq[i].mdIdle()), szAuto);
            if (pzpq->rgpq[i].cQuan() == 1 || pzpq->rgpq[i].mdIdle() == iobjAlchemy) {
                strcpy(szWork, szAuto);
            } else {
                win32::wsprintf(szWork, szFormat, szAuto, pzpq->rgpq[i].cQuan());
            }
            ControlIO().pfnSendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)szWork);
        }
    }
    return;
}

int16_t IProductionPresetSlot(int16_t i) {
    int16_t cMax;
        for (cMax = 0; cMax < 4 && (!vrgZipProd[cMax].fValid || i-- != 0); cMax++) {
        }
    return cMax;
}

void CustomizeProductionPresets(HWND hwnd) {
    ZIPPRODQ rgzp[4];
    FARPROC lpProc;
    int16_t fRet;
            memcpy(rgzp, vrgZipProd, 160);
            lpProc = MakeProcInstance(ZipProdDlg, hInst);
            fRet = ControlIO().pfnDialogBox(hInst, MAKEINTRESOURCE(IDD_ZIP_PROD), hwnd, lpProc, 0);
            FreeProcInstance(lpProc);
            if (fRet)
                return;
            memcpy(vrgZipProd, rgzp, 160);
            return;
}
