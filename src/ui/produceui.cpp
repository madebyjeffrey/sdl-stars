#include "win.h"
#include <algorithm>
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "productionpresets.h"
#include "text_io.h"
#include "control_io.h"
#include "production_io.h"
#include "presentation_io.h"
#include "race_query_io.h"
#include "alert_io.h"

std::int16_t ChangeProduction(APPSESSION &app, std::int16_t fClear) {
    PROD     rgprod[64];
    std::int16_t  fSuccess;
    StarsRecoveryScope scopeRecovery;
    try {
        if (fClear) {
            fSuccess = 1;
            goto LWriteProdQ;
        }
        ProductionIO().pfnInitialize(app, rgprod);
        app.lpui->fDlgUp = 1;
        fSuccess = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_PRODUCTION), app.lpui->hwndFrame, ProductionDlg, 0);
        app.lpui->hwndProdDlg = nullptr;
        app.lpui->fDlgUp = 0;
LWriteProdQ:
        FinishProduction(app, fSuccess);
        if (fSuccess && app.state.sel.grobj == grobjPlanet) {
            ControlIO().pfnDrawPlanShip(app, nullptr, tilePlanetStats);
        }
        return 1;
    } catch (const StarsError &) {
        if (app.state.lpplProdGlob) {
            FreePl(app, app.state.lpplProdGlob);
        }
        app.state.lpplProdGlob = nullptr;
        if (app.lpui->hwndProdDlg) {
            ControlIO().pfnEndDialog(app.lpui->hwndProdDlg, 0);
        }
        app.lpui->hwndProdDlg = nullptr;
        app.lpui->fDlgUp = 0;
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsThereIsntEnoughFreeMemoryModifyProduction, {}).data(), win32::MB_ICONHAND);
        return 0;
    }
}

INT_PTR  ProductionDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    std::array<char, cchCatalogText> szCatalogMenuText{};
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HDC                hdc;
    PAINTSTRUCT        ps;
    RECT               rc;
    std::int16_t            dxPBtn;
    std::int16_t            dy;
    RECT               rcT;
    std::int16_t            i;
    std::int16_t            xCtr;
    std::int16_t            dx;
    std::int16_t            dyLB;
    std::int16_t            rgidProdBtns[10];
    DRAWITEMSTRUCT    *lpdis;
    MEASUREITEMSTRUCT *lpmis;
    POINT16            pt;
    std::int16_t            cMax;
    char               sz255[2];
    char              *rgszZip[6];
    HCURSOR            hcs;

    switch (message) {
    case win32::WM_INITDIALOG:
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
        app.lpui->hwndProdDlg = hwnd;
        if (app.state.rgplr[app.state.idPlayer].cPlanet <= 1) {
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_NEXT), 0);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_BACK), 0);
        }
        if (app.state.gd.mdScreenSize >= 1) {
            dx = 760;
            dy = 580;
        } else {
            dx = 610;
            dy = 24 * app.state.dyArial8 + 24;
        }
        ControlIO().pfnSetWindowPos(hwnd, nullptr, 0, 0, dx, dy, win32::SWP_NOMOVE | win32::SWP_NOZORDER);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        xCtr = rc.right >> 1;
        dyLB = rc.bottom - static_cast<std::int16_t>((17 * app.state.dyArial8)) / 2 - 24;
        rc.left = static_cast<std::int16_t>((11 * rc.right)) / 20 + 16;
        dxPBtn = static_cast<std::int16_t>((rc.right - rc.left)) / 4;
        rc.bottom -= static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 + 6;
        ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY), nullptr, 6, rc.bottom, rc.left - 12, static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2, win32::SWP_NOZORDER);
        for (i = 0; i < 4; i++) {
            ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, rgidProdBtns[i]), nullptr, rc.left, rc.bottom, dxPBtn - 6, static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2, win32::SWP_NOZORDER);
            rc.left += dxPBtn;
        }
        dxPBtn += 24;
        rc.left = xCtr - ((dxPBtn - 6) >> 1);
        dy = static_cast<std::int16_t>((dyLB - 9 * app.state.dyArial8)) / 5 + static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2 - 3;
        rc.top = 8;
        if (dy > 50) {
            rc.top += static_cast<std::int16_t>(((dy - 50) * 5)) / 2;
            dy = 50;
        }
        for (i = 4; i < 10; i++) {
            ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, rgidProdBtns[i]), nullptr, rc.left, rc.top, dxPBtn - 6, static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2, win32::SWP_NOZORDER);
            rc.top += dy;
        }
        ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), nullptr, 6, 6, rc.left - 12, dyLB, win32::SWP_NOZORDER);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), &rcT);
        ControlIO().pfnSetWindowPos(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), nullptr, rc.left + dxPBtn, 6, rc.left - 12, rcT.bottom - rcT.top, win32::SWP_NOZORDER);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcT.right));
        app.lpui->yTopFutureTech = rcT.bottom;
        InitializeProductionDlg(app, hwnd);
        if (app.state.gd.mdScreenSize == 1 && app.lpui->ptStickyProduceDlg.y == -1) {
            app.lpui->ptStickyProduceDlg.y = 0;
        }
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyProduceDlg, 1);
        return 1;
    case win32::WM_DRAWITEM:
        lpdis = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
        if (lpdis->itemID == -1) {
            HandleFocusState(app, lpdis, -2);
        } else {
            switch (lpdis->itemAction) {
            case win32::ODA_DRAWENTIRE:
            case win32::ODA_SELECT:
            case win32::ODA_FOCUS:
                DrawCBEntireItem(app, lpdis, 4);
            }
        }
        return 1;
    case win32::WM_MEASUREITEM:
        lpmis = reinterpret_cast<MEASUREITEMSTRUCT *>(lParam);
        lpmis->itemHeight = app.state.dyArial8 + 2;
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        if (reinterpret_cast<HWND>(lParam) == ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY) || message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_LBUTTONDOWN:
    case win32::WM_RBUTTONDOWN:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        if (win32::PtInRect(&app.lpui->rcProdDiamond, PointFrom16(pt)) == 0)
            break;
        if (message == win32::WM_LBUTTONDOWN) {
            app.lpui->GlobalPD.grPopup = grPopupString;
            app.lpui->GlobalPD.dxOut = 180;
            app.lpui->GlobalPD.psz = app.lpui->szPopupBuffer;
            TextIO().pfnCchGetString(app, idsRightClickBlueDiamondApplyProductionTemplate, std::span{app.lpui->szPopupBuffer});
            Popup(app, hwnd, pt.x, pt.y);
            break;
        }
        sz255[0] = -1;
        sz255[1] = 0;
        cMax = 0;
        for (i = 0; i < 4; i++) {
            if (app.state.vrgZipProd[i].fValid) {
                rgszZip[cMax++] = app.state.vrgZipProd[i].szName;
            }
        }
        rgszZip[cMax++] = sz255;
        szCatalogMenuText = TextIO().pfnCompressed(app, idsCustomize);
        rgszZip[cMax++] = szCatalogMenuText.data();
        i = ControlIO().pfnPopupMenu(app, hwnd, pt.x, pt.y, cMax, nullptr, rgszZip, -1, 1);
        if (i == cMax - 1) {
            CustomizeProductionPresets(app, hwnd);
            break;
        }
        if (i < 0)
            break;
        cMax = IProductionPresetSlot(app, i);
        ProdCommandHandler(app, hwnd, 0x816, cMax);
        break;
    case win32::WM_SETCURSOR:
        hcs = nullptr;
        GetCursorPos16(&pt);
        ScreenToClient16(hwnd, &pt);
        if (win32::PtInRect(&app.lpui->rcProdDiamond, PointFrom16(pt)) == 0)
            break;
        win32::SetCursor(app.lpui->hcurArrowHelp);
        return 1;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        ControlIO().pfnDrawProductionDlg(app, hwnd, hdc, &rc, -1);
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_COMMAND:
        ProdCommandHandler(app, hwnd, wParam, lParam);
    }
    return 0;
}

void ProdCommandHandler(APPSESSION &app, HWND hwnd, WPARAM wParam, LPARAM lParam) {
    std::int32_t lSel;
    std::int16_t iSrc;
    HWND    hwndLB;
    std::int16_t c;
    std::int16_t iDst;
    PROD    prodLast;
    std::int16_t ipl;
    std::int16_t fRefillSrc;
    std::int16_t iMac;
    RECT    rc;
    PROD   *lpprod;
    PROD    prod;
    std::int16_t cMax;
    PLPROD *lpplprodT;

    switch (LOWORD(wParam)) {
    case IDC_PRODUCTION_ADD:
    AddItem:
        lSel = ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), win32::LB_GETCURSEL, 0, 0);
        if (lSel < 0)
            break;
        for (iSrc = 0; iSrc < app.state.cProdGlob && (app.state.pProdGlob[iSrc].cItem == 0 || lSel-- != 0); iSrc++) {
        }
        prod = app.state.pProdGlob[iSrc];
        if (ControlIO().pfnGetAsyncKeyState(win32::VK_CONTROL) & 0xfffe) {
            if (ControlIO().pfnGetAsyncKeyState(win32::VK_SHIFT) & 0xfffe) {
                prod.cItem = LOWORD(prod.cItem < 1020 ? static_cast<std::uint32_t>(prod.cItem) : 1020);
            } else {
                prod.cItem = LOWORD(prod.cItem < 100 ? static_cast<std::uint32_t>(prod.cItem) : 100);
            }
        } else if (ControlIO().pfnGetAsyncKeyState(win32::VK_SHIFT) & 0xfffe) {
            prod.cItem = LOWORD(prod.cItem < 10 ? static_cast<std::uint32_t>(prod.cItem) : 10);
        } else {
            prod.cItem = 1;
        }
        if (app.state.pProdGlob[iSrc].cItem != 0x3ff) {
            app.state.pProdGlob[iSrc].cItem -= prod.cItem;
        }
        iMac = app.state.lpplProdGlob->iprodMac;
        lSel = ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), win32::LB_GETCURSEL, 0, 0);
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
                prodLast = app.state.lpplProdGlob->rgprod()[iDst];
                if (static_cast<std::uint32_t>(prodLast.iItem) == prod.iItem && static_cast<std::uint32_t>(prodLast.grobj) == prod.grobj) {
                RingItUp:
                    app.state.lpplProdGlob->rgprod()[iDst].cItem = StarsMin(1020, app.state.lpplProdGlob->rgprod()[iDst].cItem + prod.cItem);
                    if (app.state.lpplProdGlob->rgprod()[iDst].cItem > 1 && app.state.lpplProdGlob->rgprod()[iDst].iItem == iobjAlchemy &&
                        app.state.lpplProdGlob->rgprod()[iDst].grobj == grobjPlanet) {
                        app.state.lpplProdGlob->rgprod()[iDst].cItem = 1;
                    }
                    goto FixedUp;
                }
            }
            iDst++;
            if (iDst < iMac) {
                prodLast = app.state.lpplProdGlob->rgprod()[iDst];
                if (static_cast<std::uint32_t>(prodLast.iItem) == prod.iItem && static_cast<std::uint32_t>(prodLast.grobj) == prod.grobj)
                    goto RingItUp;
            }
        }
        if (iMac >= 40) {
            ControlIO().pfnMessageBeep(win32::MB_OK);
            goto RedrawText;
        }
        if (iMac == app.state.lpplProdGlob->iprodMax) {
            app.state.lpplProdGlob = LpplReAlloc(app, app.state.lpplProdGlob, iMac + 4);
        }
        if (iDst != iMac) {
            std::memmove(&app.state.lpplProdGlob->rgprod()[iDst + 1], &app.state.lpplProdGlob->rgprod()[iDst], (iMac - iDst) * 4);
        }
        app.state.lpplProdGlob->rgprod()[iDst] = prod;
        app.state.lpplProdGlob->iprodMac++;
    FixedUp:
        ControlIO().pfnFillPlanetProdLB(app, ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), app.state.lpplProdGlob, nullptr);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), win32::LB_SETCURSEL, iDst + 1, 0);
        if (app.state.pProdGlob[iSrc].cItem != 0)
            goto RedrawText;
        ControlIO().pfnFillProdSrcLB(app, ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), -1);
        goto RedrawText;
    case IDC_PRODUCTION_REMOVE:
    RemoveItem:
        lSel = ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), win32::LB_GETCURSEL, 0, 0);
        if (lSel <= 0)
            break;
        iMac = app.state.lpplProdGlob->iprodMac;
        lSel--;
        prod = app.state.lpplProdGlob->rgprod()[lSel];
        for (iSrc = 0; iSrc < app.state.cProdGlob && (static_cast<std::uint32_t>(app.state.pProdGlob[iSrc].grobj) != prod.grobj || static_cast<std::uint32_t>(app.state.pProdGlob[iSrc].iItem) != prod.iItem); iSrc++) {
        }
        fRefillSrc = app.state.pProdGlob[iSrc].cItem == 0;
        if (ControlIO().pfnGetAsyncKeyState(win32::VK_CONTROL) & 0xfffe) {
            if (ControlIO().pfnGetAsyncKeyState(win32::VK_SHIFT) & 0xfffe) {
                c = 1020;
            } else {
                c = 100;
            }
        } else if (ControlIO().pfnGetAsyncKeyState(win32::VK_SHIFT) & 0xfffe) {
            c = 10;
        } else {
            c = 1;
        }
        if (prod.grobj == grobjPlanet && prod.iItem == iobjAlchemy) {
            c = 1020;
        }
        c = c >= static_cast<std::int32_t>(prod.cItem) ? prod.cItem : c;
        if (app.state.pProdGlob[iSrc].cItem != 0x3ff) {
            app.state.pProdGlob[iSrc].cItem += c;
        }
        app.state.lpplProdGlob->rgprod()[lSel].cItem -= c;
        if (app.state.lpplProdGlob->rgprod()[lSel].cItem == 0) {
            if (static_cast<std::int32_t>((lSel + 1)) < iMac) {
                std::memmove(&app.state.lpplProdGlob->rgprod()[lSel], &app.state.lpplProdGlob->rgprod()[lSel + 1], (iMac - LOWORD(lSel) - 1) * sizeof(PROD));
            } else {
                lSel--;
            }
            app.state.lpplProdGlob->iprodMac--;
        }
        ControlIO().pfnFillPlanetProdLB(app, ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), app.state.lpplProdGlob, nullptr);
        if (lSel >= 0) {
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), win32::LB_SETCURSEL, LOWORD(lSel) + 1, 0);
        }
        if (!fRefillSrc)
            goto RedrawText;
        ControlIO().pfnFillProdSrcLB(app, ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), -1);
        goto RedrawText;
    case IDC_PRODUCTION_CLEAR:
    case IDC_IMPORT:
        ipl = 0;
        lpprod = app.state.lpplProdGlob->rgprod();
        while (ipl < app.state.lpplProdGlob->iprodMac) {
            for (iSrc = 0; iSrc < app.state.cProdGlob && (static_cast<std::uint32_t>(app.state.pProdGlob[iSrc].grobj) != lpprod->grobj || static_cast<std::uint32_t>(app.state.pProdGlob[iSrc].iItem) != lpprod->iItem); iSrc++) {
            }
            if (app.state.pProdGlob[iSrc].cItem != 0x3ff) {
                app.state.pProdGlob[iSrc].cItem += lpprod->cItem;
            }
            ipl++;
            lpprod++;
        }
        if (LOWORD(wParam) == IDC_IMPORT) {
            cMax = app.state.lpplProdGlob->iprodMac + app.state.vrgZipProd[lParam].cpq;
            if (cMax < 1) {
                cMax = 1;
            }
            lpplprodT = LpplAlloc<PLPROD>(app, 4, cMax, htOrd);
            std::fill_n(lpplprodT->rgprod(), cMax, PROD{});
            iDst = 0;
            for (iSrc = 0; iSrc < app.state.lpplProdGlob->iprodMac; iSrc++) {
                if (app.state.lpplProdGlob->rgprod()[iSrc].grobj != grobjPlanet || app.state.lpplProdGlob->rgprod()[iSrc].iItem >= mdIdleFactory) {
                    lpplprodT->rgprod()[iDst] = app.state.lpplProdGlob->rgprod()[iSrc];
                    iDst++;
                }
            }
            for (iSrc = 0; iSrc < app.state.vrgZipProd[lParam].cpq; iSrc++) {
                if ((RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) != raMacintosh || app.state.vrgZipProd[lParam].rgpq[iSrc].mdIdle() > iobjDefense) &&
                    (RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) != raTerra ||
                     (app.state.vrgZipProd[lParam].rgpq[iSrc].mdIdle() != iobjMinTerraform && app.state.vrgZipProd[lParam].rgpq[iSrc].mdIdle() != iobjMaxTerraform))) {
                    lpplprodT->rgprod()[iDst].grobj = grobjPlanet;
                    lpplprodT->rgprod()[iDst].iItem = app.state.vrgZipProd[lParam].rgpq[iSrc].mdIdle();
                    lpplprodT->rgprod()[iDst].cItem = app.state.vrgZipProd[lParam].rgpq[iSrc].cQuan();
                    iDst++;
                }
            }
            lpplprodT->iprodMac = iDst;
            FreePl(app, app.state.lpplProdGlob);
            app.state.lpplProdGlob = lpplprodT;
            app.state.sel.pl.fNoResearch = app.state.vrgZipProd[lParam].fNoResearch;
            ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY), win32::BM_SETCHECK, app.state.sel.pl.fNoResearch, 0);
        } else {
            app.state.lpplProdGlob->iprodMac = 0;
        }
        ControlIO().pfnFillPlanetProdLB(app, ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), app.state.lpplProdGlob, nullptr);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), win32::LB_SETCURSEL, 0, 0);
        ControlIO().pfnFillProdSrcLB(app, ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), -1);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), win32::LB_SETCURSEL, 0, 0);
        goto RedrawText;
    case IDC_PRODUCTION_AVAILABLE_ITEMS:
    case IDC_PRODUCTION_QUEUE:
        if (HIWORD(wParam) == 1) {
        RedrawText:
            ControlIO().pfnGetClientRect(hwnd, &rc);
            rc.top = app.lpui->yTopFutureTech;
            rc.bottom = 7 * app.state.dyArial8 + rc.top;
            rc.left += 130;
            ControlIO().pfnInvalidateRect(hwnd, &rc, 1);
            ControlIO().pfnDrawProductionDlg(app, hwnd, nullptr, &rc, -1);
        } else if (HIWORD(wParam) == 2) {
            if (LOWORD(wParam) == IDC_PRODUCTION_AVAILABLE_ITEMS)
                goto AddItem;
            goto RemoveItem;
        }
        break;
    case IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY:
        hwndLB = ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE);
        app.state.sel.pl.fNoResearch = LOWORD(ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY), win32::BM_GETCHECK, 0, 0));
        lSel = ControlIO().pfnSendMessage(hwndLB, win32::LB_GETCURSEL, 0, 0);
        ControlIO().pfnFillPlanetProdLB(app, hwndLB, app.state.lpplProdGlob, nullptr);
        ControlIO().pfnSendMessage(hwndLB, win32::LB_SETCURSEL, LOWORD(lSel), 0);
        goto RedrawText;
    case IDOK:
    case IDCANCEL:
        app.lpui->hwndProdDlg = nullptr;
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyProduceDlg, 0);
        ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
        break;
    case IDC_BACK:
    case IDC_NEXT:
        c = LOWORD(wParam) == IDC_NEXT ? 1 : -1;
        FinishProduction(app, 1);
        if (ControlIO().pfnGetKeyState(win32::VK_SHIFT) < 0) {
            PresentationIO().pfnSelectPlanet(app, 0, IdFindAdjStarbase(app, app.state.sel.pl.id, LOWORD(wParam) == IDC_NEXT));
        } else {
            PresentationIO().pfnSelectPlanet(app, c, 0);
        }
        ProductionIO().pfnInitialize(app, nullptr);
        InitializeProductionDlg(app, hwnd);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        rc.top = app.lpui->yTopFutureTech;
        rc.bottom = 9 * app.state.dyArial8 + rc.top;
        ControlIO().pfnInvalidateRect(hwnd, &rc, 1);
        break;
    case IDC_PRODUCTION_ITEM_DOWN:
        lSel = ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), win32::LB_GETCURSEL, 0, 0);
        iMac = app.state.lpplProdGlob->iprodMac;
        if (lSel <= 0 || lSel >= iMac)
            break;
        lSel--;
        prod = app.state.lpplProdGlob->rgprod()[lSel];
        app.state.lpplProdGlob->rgprod()[lSel] = app.state.lpplProdGlob->rgprod()[lSel + 1];
        app.state.lpplProdGlob->rgprod()[lSel + 1] = prod;
        ControlIO().pfnFillPlanetProdLB(app, ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), app.state.lpplProdGlob, nullptr);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), win32::LB_SETCURSEL, LOWORD(lSel) + 2, 0);
        goto RedrawText;
    case IDC_PRODUCTION_ITEM_UP:
        lSel = ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), win32::LB_GETCURSEL, 0, 0);
        if (lSel <= 1)
            break;
        iMac = app.state.lpplProdGlob->iprodMac;
        lSel -= 2;
        prod = app.state.lpplProdGlob->rgprod()[lSel];
        app.state.lpplProdGlob->rgprod()[lSel] = app.state.lpplProdGlob->rgprod()[lSel + 1];
        app.state.lpplProdGlob->rgprod()[lSel + 1] = prod;
        ControlIO().pfnFillPlanetProdLB(app, ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), app.state.lpplProdGlob, nullptr);
        ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), win32::LB_SETCURSEL, LOWORD(lSel) + 1, 0);
        goto RedrawText;
    case IDC_HELP:
        win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhProductionDialog);
    }
}

void InitializeProductionDlg(APPSESSION &app, HWND hwnd) {
    char    rgch[86];
    std::int16_t i;
    std::int16_t iSel;
    PROD   *lpprod;

    iSel = -1;
    win32::wsprintfA(rgch, TextIO().pfnCompressed(app, idsProductionQueueS).data(), TextIO().pfnPlanetName(app, app.state.sel.pl.id).data());
    ControlIO().pfnSetWindowText(hwnd, rgch);
    ControlIO().pfnFillProdSrcLB(app, ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), -1);
    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_AVAILABLE_ITEMS), win32::LB_SETCURSEL, 0, 0);
    i = 0;
    lpprod = app.state.lpplProdGlob->rgprod();
    while (i < app.state.lpplProdGlob->iprodMac) {
        if (lpprod->grobj != grobjPlanet || lpprod->iItem >= mdIdleFactory) {
            iSel = i;
        }
        i++;
        lpprod++;
    }
    ControlIO().pfnFillPlanetProdLB(app, ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), app.state.lpplProdGlob, nullptr);
    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_QUEUE), win32::LB_SETCURSEL, iSel + 1, 0);
    ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY), win32::BM_SETCHECK, app.state.sel.pl.fNoResearch, 0);
    return;
}

void DrawProductionDlg(APPSESSION &app, HWND hwnd, HDC hdc, RECT *prc, std::int16_t iDraw) {
    std::array<char, 360> szDisplayLabel{};
    std::int32_t lSel;
    std::int16_t iSrc;
    std::int16_t idc;
    std::int16_t fCreatedDC;
    std::int16_t i;
    std::int16_t c;
    std::int32_t rgCost[4];
    std::int16_t dxkT;
    std::int16_t k;
    RECT    rc;
    PROD    prod;
    char    szT[100];

    fCreatedDC = 0;
    if (!hdc) {
        fCreatedDC = 1;
        hdc = win32::GetDC(hwnd);
    }
    win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
    dxkT = LOWORD(GetTextExtent(hdc, TextIO().pfnCompressed(app, idsKt).data(), 2));
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    win32::SetBkColor(hdc, app.lpui->crButtonFace);
    for (i = 0; i < 2; i++) {
        idc = i == 0 ? 1046 : 1047;
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, idc), &rc);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rc));
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rc.right));
        lSel = ControlIO().pfnSendMessage(ControlIO().pfnGetDlgItem(hwnd, idc), win32::LB_GETCURSEL, 0, 0);
        if (lSel >= 0 && (lSel != 0 || i != 1)) {
            if (i == 0) {
                for (iSrc = 0; iSrc < app.state.cProdGlob && (app.state.pProdGlob[iSrc].cItem == 0 || lSel-- != 0); iSrc++) {
                }
                prod = app.state.pProdGlob[iSrc];
                prod.cItem = 1;
            } else {
                lSel--;
                prod = app.state.lpplProdGlob->rgprod()[lSel];
            }
            ProductionIO().pfnQuote(app, &app.state.sel.pl, &prod, reinterpret_cast<std::uint32_t *>(rgCost), app.state.idPlayer, 0);
            rc.bottom = app.lpui->yTopFutureTech + 4;
            win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
            c = TextIO().pfnCchGetString(app, idsRequiredMinerals, std::span{app.state.szWork});
            win32::TextOutA(hdc, rc.left, rc.bottom, app.state.szWork, c);
            rc.left += 20;
            rc.right -= 20;
            for (k = 0; k <= 3; k++) {
                c = k == 3 ? 5 : k;
                rc.bottom += app.state.dyArial8;
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                win32::SetTextColor(hdc, app.lpui->rgcrMinerals[c]);
                win32::TextOutA(hdc, rc.left, rc.bottom, app.state.rgszMinerals[c], win32::lstrlenA(app.state.rgszMinerals[c]));
                win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
                win32::SetTextColor(hdc, app.lpui->crWindowText);
                c = win32::wsprintfA(app.state.szWork, app.state.PCTLD, rgCost[k]);
                RightTextOut(app, hdc, rc.right - dxkT - 2, rc.bottom, app.state.szWork, c, app.lpui->dxMaxMineralQuan);
                if (k <= 2) {
                    win32::TextOutA(hdc, rc.right - dxkT, rc.bottom, TextIO().pfnCompressed(app, idsKt).data(), 2);
                }
            }
            if (i != 0) {
                rc.bottom += static_cast<std::int16_t>((3 * app.state.dyArial8)) / 2;
                win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
                c = win32::wsprintfA(szT, TextIO().pfnCompressed(app, idsDDoneCompletion).data(), prod.pct);
                szDisplayLabel = PszProductionETA(app, &app.state.sel.pl, app.state.lpplProdGlob, static_cast<ProdItemType>(LOWORD(lSel)), nullptr, nullptr);
                CchCopyUtf8Prefix(std::span{szT}.subspan(static_cast<std::size_t>(c)), szDisplayLabel.data());
                win32::TextOutA(hdc, rc.left - 20, rc.bottom, szT, std::strlen(szT));
                win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
            }
        }
    }
    ControlIO().pfnGetClientRect(hwnd, &rc);
    rc.top = rc.bottom - (static_cast<std::int16_t>((5 * app.state.dyArial8)) / 2 + 12);
    rc.bottom = (app.state.dyArial8 | 1) + rc.top;
    rc.left = 6;
    rc.right = rc.left + app.state.dyArial8;
    DrawDiamond(app, hdc, &rc, app.lpui->hbrBBlue);
    app.lpui->rcProdDiamond = rc;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    c = TextIO().pfnCchGetString(app, idsApplyDefineProductionTemplate, std::span{app.state.szWork});
    win32::TextOutA(hdc, rc.right + 4, rc.top, app.state.szWork, c);
    if (fCreatedDC) {
        win32::ReleaseDC(hwnd, hdc);
    }
    return;
}

void FillProdSrcLB(APPSESSION &app, HWND hwndLB, std::int16_t mdFill) {
    std::array<char, 360> szDisplayLabel{};
    char    szT[80];
    std::int16_t i;
    char   *psz;

    for (i = 0; i < 6; i++) {
        szT[i] = ' ';
    }
    ControlIO().pfnSendMessage(hwndLB, win32::LB_RESETCONTENT, 0, 0);
    for (i = 0; i < app.state.cProdGlob; i++) {
        if (app.state.pProdGlob[i].cItem > 0) {
            szDisplayLabel = PszNameProdItem(app, app.state.pProdGlob + i);
            psz = szDisplayLabel.data();
            CchCopyUtf8Prefix(std::span{szT}.subspan(6), psz);
            if (app.state.pProdGlob[i].grobj == grobjFleet) {
                szT[0] = app.state.pProdGlob[i].iItem < iobjPacketGerm ? 42 : 35;
            } else if (app.state.pProdGlob[i].iItem < mdIdleFactory) {
                szT[0] = 'I';
                CchAppendUtf8Prefix(std::span{szT}.subspan(6), " (Auto Build)");
            } else {
                szT[0] = ' ';
            }
            ControlIO().pfnSendMessage(hwndLB, win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(szT));
        }
    }
    return;
}

INT_PTR  ZipProdDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    std::array<char, cchCatalogText> szCatalogPsz{};
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HDC         hdc;
    PAINTSTRUCT ps;
    std::int16_t     i;
    std::int16_t     iBase;
    RECT        rc;
    std::int16_t     dy;
    RECT        rc2;
    HWND        hwndRad;
    char       *psz;
    char       *pszT;
    RECT        rcGBox;
    std::int16_t     cch;
    std::int16_t     cpq;

    switch (message) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnSetWindowText(hwnd, TextIO().pfnCompressed(app, idsCustomizeProductionTemplates).data());
        win32::GetWindowRect(hwnd, &rc);
        ControlIO().pfnGetClientRect(hwnd, &rc2);
        dy = rc.bottom - rc.top - rc2.bottom;
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_ZIP_PROD_QUEUE), &rc2);
        win32::MapWindowPoints(nullptr, hwnd, reinterpret_cast<POINT *>(&rc2), 2);
        app.lpui->vyZPDStatic = rc2.bottom + 2;
        dy += rc2.bottom + app.state.dyArial8 + 6;
        ControlIO().pfnSetWindowPos(hwnd, nullptr, 0, 0, rc.right - rc.left, dy, win32::SWP_NOMOVE | win32::SWP_NOZORDER);
        win32::CheckRadioButton(hwnd, IDC_ZIP_PROD_PRESET_1, IDC_ZIP_PROD_PRESET_4, IDC_ZIP_PROD_PRESET_1);
        ControlIO().pfnEnableZipProdBtns(app, hwnd, 0);
        app.lpui->iResTechNow = Energy;
        ControlIO().pfnFillZipProdLB(app, hwnd, &app.state.vrgZipProd[app.lpui->iResTechNow]);
        for (i = 1073; i <= 1076; i++) {
            iBase = i - 1073;
            if (app.state.vrgZipProd[iBase].fValid) {
                pszT = app.state.szWork;
                psz = app.state.vrgZipProd[iBase].szName;
                while (*psz != 0) {
                    if ((*pszT++ = *psz++) == '&') {
                        *pszT++ = '&';
                    }
                }
                *pszT = 0;
                psz = app.state.szWork;
            } else {
                szCatalogPsz = TextIO().pfnCompressed(app, idsUnusedD);
                psz = szCatalogPsz.data();
                win32::wsprintfA(app.state.szWork, psz, iBase + 1);
                psz = app.state.szWork;
            }
            hwndRad = ControlIO().pfnGetDlgItem(hwnd, i);
            ControlIO().pfnSetWindowText(hwndRad, psz);
        }
        ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyZipProdDlg, 1);
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
        for (i = 1073; i <= 1076 && reinterpret_cast<HWND>(lParam) != ControlIO().pfnGetDlgItem(hwnd, i); i++) {
        }
        if (i <= 1076) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_ZIP_PROD_PRESET_1), &rcGBox);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rcGBox));
        win32::GetWindowRect(ControlIO().pfnGetDlgItem(hwnd, IDC_ZIP_PROD_PRESET_4), &rc);
        win32::ScreenToClient(hwnd, reinterpret_cast<POINT *>(&rc.right));
        rcGBox.right = rc.right;
        rcGBox.bottom = rc.bottom;
        ExpandRc(&rcGBox, app.state.dyArial8, app.state.dyArial8 >> 1);
        _Draw3dFrame(app, hdc, &rcGBox, -1);
        win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
        win32::SetBkColor(hdc, app.lpui->crButtonFace);
        cch = TextIO().pfnCchGetString(app, idsCustomOrders, std::span{app.state.szWork});
        win32::TextOutA(hdc, rcGBox.left + 8, rcGBox.top - (app.state.dyArial8 >> 1), app.state.szWork, cch);
        rcGBox.top = rcGBox.bottom + 8;
        if (app.state.vrgZipProd[app.lpui->iResTechNow].fValid) {
            cch = TextIO().pfnCchGetString(app, static_cast<StringId>(idsContributeResearch + app.state.vrgZipProd[app.lpui->iResTechNow].fNoResearch), std::span{app.state.szWork});
            win32::TextOutA(hdc, rcGBox.left, app.lpui->vyZPDStatic, app.state.szWork, cch);
        }
        win32::EndPaint(hwnd, &ps);
        return 1;
    case win32::WM_COMMAND:
        if (HIWORD(wParam) == 0 && LOWORD(wParam) >= IDC_ZIP_PROD_PRESET_1 && LOWORD(wParam) <= IDC_ZIP_PROD_PRESET_4) {
            app.lpui->iResTechNow = static_cast<TechFieldType>(LOWORD(wParam) - 1073);
            ControlIO().pfnEnableZipProdBtns(app, hwnd, app.lpui->iResTechNow);
            ControlIO().pfnFillZipProdLB(app, hwnd, &app.state.vrgZipProd[app.lpui->iResTechNow]);
        } else {
            switch (LOWORD(wParam)) {
            case IDOK:
            case IDCANCEL:
                ControlIO().pfnStickyDlgPos(hwnd, &app.lpui->ptStickyZipProdDlg, 0);
                ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
                app.lpui->vyZPDStatic = -1;
                if (app.state.gd.fTutorial) {
                    PresentationIO().pfnTutor(app);
                }
                return 1;
            case IDC_IMPORT:
            case IDC_RENAME:
                if (app.state.vrgZipProd[app.lpui->iResTechNow].fValid) {
                    CchCopyUtf8Prefix(std::span{app.state.szWork}, app.state.vrgZipProd[app.lpui->iResTechNow].szName);
                } else {
                    win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsCustomD).data(), app.lpui->iResTechNow);
                }
                if (app.lpui->iResTechNow == Energy)
                    goto LDontRename;
                if (ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_RENAME), app.lpui->hwndFrame, RenameZipDlg, 0) != 0) {
                    if (app.state.szWork[0] == 0) {
                        win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsCustomD).data(), app.lpui->iResTechNow);
                    }
                    CchCopyUtf8Prefix(std::span{app.state.vrgZipProd[app.lpui->iResTechNow].szName}, app.state.szWork);
                    pszT = &app.state.szWork[64];
                    psz = app.state.szWork;
                    while (*psz != 0) {
                        if ((*pszT++ = *psz++) == '&') {
                            *pszT++ = '&';
                        }
                    }
                    *pszT = 0;
                    ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, app.lpui->iResTechNow + 1073), &app.state.szWork[64]);
                LDontRename:
                    if (LOWORD(wParam) == IDC_IMPORT) {
                        app.state.vrgZipProd[app.lpui->iResTechNow].fValid = 1;
                        cpq = 0;
                        for (i = 0; i < app.state.lpplProdGlob->iprodMac; i++) {
                            if (app.state.lpplProdGlob->rgprod()[i].grobj == grobjPlanet && app.state.lpplProdGlob->rgprod()[i].iItem < mdIdleFactory) {
                                app.state.vrgZipProd[app.lpui->iResTechNow].rgpq[cpq].SetMdIdle(app.state.lpplProdGlob->rgprod()[i].iItem);
                                app.state.vrgZipProd[app.lpui->iResTechNow].rgpq[cpq].SetCQuan(app.state.lpplProdGlob->rgprod()[i].cItem);
                                cpq++;
                                if (cpq >= 12)
                                    break;
                            }
                        }
                        app.state.vrgZipProd[app.lpui->iResTechNow].cpq = cpq;
                        app.state.vrgZipProd[app.lpui->iResTechNow].fNoResearch = app.state.sel.pl.fNoResearch;
                        ControlIO().pfnFillZipProdLB(app, hwnd, &app.state.vrgZipProd[app.lpui->iResTechNow]);
                    }
                    ControlIO().pfnEnableZipProdBtns(app, hwnd, app.lpui->iResTechNow);
                }
                ControlIO().pfnSetFocus(hwnd);
                app.state.gd.fChgZipProd = 1;
                break;
            case IDC_DELETE:
                app.state.vrgZipProd[app.lpui->iResTechNow].fValid = 0;
                win32::wsprintfA(app.state.szWork, TextIO().pfnCompressed(app, idsUnusedD).data(), app.lpui->iResTechNow + 1);
                ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, app.lpui->iResTechNow + 1073), app.state.szWork);
                ControlIO().pfnFillZipProdLB(app, hwnd, &app.state.vrgZipProd[app.lpui->iResTechNow]);
                app.state.gd.fChgZipProd = 1;
                break;
            case IDC_HELP:
                win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhCustomizeProductionTemplatesDialog);
                return 1;
            }
        }
    }
    return 0;
}

void EnableZipProdBtns(APPSESSION &app, HWND hwnd, std::int16_t iSel) {
    std::int16_t fEnabled;

    fEnabled = app.state.vrgZipProd[iSel].fValid != 0 && iSel > 0;
    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_DELETE), fEnabled);
    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_RENAME), fEnabled);
    return;
}

void FillZipProdLB(APPSESSION &app, HWND hwndDlg, ZIPPRODQ *pzpq) {
    std::int16_t i;
    HWND    hwndLB;
    char    szAuto[40];
    char    szFormat[15];
    RECT    rc;

    hwndLB = ControlIO().pfnGetDlgItem(hwndDlg, IDC_ZIP_PROD_QUEUE);
    ControlIO().pfnGetClientRect(hwndDlg, &rc);
    rc.top = app.lpui->vyZPDStatic;
    rc.bottom = app.lpui->vyZPDStatic + app.state.dyArial8;
    ControlIO().pfnInvalidateRect(hwndDlg, &rc, 1);
    ControlIO().pfnSendMessage(hwndLB, win32::LB_RESETCONTENT, 0, 0);
    if (!pzpq->fValid || pzpq->cpq == 0) {
        ControlIO().pfnSendMessage(hwndLB, win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TextIO().pfnCompressed(app, idsAutoBuildOrders).data()));
    } else {
        TextIO().pfnCchGetString(app, idsSD2, std::span{szFormat});
        for (i = 0; i < pzpq->cpq; i++) {
            TextIO().pfnCchGetString(app, static_cast<StringId>(idsMines + pzpq->rgpq[i].mdIdle()), std::span{szAuto});
            if (pzpq->rgpq[i].cQuan() == 1 || pzpq->rgpq[i].mdIdle() == iobjAlchemy) {
                CchCopyUtf8Prefix(std::span{app.state.szWork}, szAuto);
            } else {
                win32::wsprintfA(app.state.szWork, szFormat, szAuto, pzpq->rgpq[i].cQuan());
            }
            ControlIO().pfnSendMessage(hwndLB, win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(app.state.szWork));
        }
    }
    return;
}

std::int16_t IProductionPresetSlot(APPSESSION &app, std::int16_t i) {
    std::int16_t cMax;
        for (cMax = 0; cMax < 4 && (!app.state.vrgZipProd[cMax].fValid || i-- != 0); cMax++) {
        }
    return cMax;
}

void CustomizeProductionPresets(APPSESSION &app, HWND hwnd) {
    ZIPPRODQ rgzp[4];
    std::int16_t fRet;
            std::memcpy(rgzp, app.state.vrgZipProd, 160);
            fRet = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_ZIP_PROD), hwnd, ZipProdDlg, 0);
            if (fRet)
                return;
            std::memcpy(app.state.vrgZipProd, rgzp, 160);
            return;
}
