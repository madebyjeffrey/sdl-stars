/* STARS_TEST_TUTORIAL: drive the actual SDL client; never complete tasks by
 * assigning tutorial or game state. The smoke scenario is deliberately bounded.
 */
#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "../../src/ui/desktop_choice_io.h"
#include "../../compat/win32/src/choice_io.h"

#include <stars_compat.h>
#include "internal.h"
#include <stdio.h>
#include <stdlib.h>

static UINT_PTR idTimer;
static int iStage, cWait, cActions, cReadActions;
static BOOL fPassed, fRejected, fFirstYear, fTwoYears, fScouting, fMining, fColonization, fLogistics, fColonizerProduction, fSecondColonization, fTransportCycle, fExpansion, fFleetSplit, fResearchCycle, fOxygenColony, fProdTemplates, fBattleCycle, fTerraformCycle, fDesignCycle, fAutomationCycle, fReportCycle, fMergeCycle;
static BOOL fExpectedRejection;
static int idtBefore, cPage, iMsgBefore;
static BOOL rgfPage[80];
static const char *pszFailure = "client exited before smoke completion";
static NATIVEFILE *lpEvents;
static SCANOBJECT objTarget;
static const char *pszMenu;
static int iFactoryRow;
static int iProdPass, cProdAdded, iProdReturn;
static int pctResearchBefore;
static int idflAction, cCargoGoal, iOrderCombo, iOrderRow, iStepBefore;
static int mdProbeMessage;
static int iDesignRow, iDesignSlot, idtDesignBefore = -1, cDesignVisits;
static const char *pszDesignCategory, *pszDesignPart;
static HWND hwndDesignCombo;
static int iDesignCombo, iDesignReturn, iProductionInspect;
static int idCargoFleet, idCargoPlanet = 13, cCargoExpected = 210, iCargoReturn = 203, iOrderReturn = 203;
static const char *pszCargoSnapshot = "year-2413-cargo.json";
static BOOL fValueReportChecked;
static int iResearchNext = Propulsion, iResearchReturn = 203;
static const char *pszResearchSnapshot = "year-2413-research.json", *pszOrderSnapshot = "year-2413-transport.json";
static HWND HwndModal(void);
static BOOL FSaveState(APPSESSION &app, const char *pszName);

BOOL StarsTest_TrackPopupMenu(HMENU hmenu, UINT flags, int x, int y, int reserved, HWND hwnd, const RECT *prc) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    SDL_Event event = {};
    HWND hwndRoot = HwndCompatRoot(hwnd);
    char szText[256];
    int i, j, iItem = -1, cSteps = 0;
    if (!pszMenu) return win32::TrackPopupMenu(hmenu, flags, x, y, reserved, hwnd, prc);
    /* PopupMenu sends commands to the frame even when a modal owns the input. */
    if (hwndRoot && !win32::IsWindowEnabled(hwndRoot)) hwndRoot = HwndCompatRoot(HwndModal());
    for (i = 0; i < win32::GetMenuItemCount(hmenu); i++) {
        if (win32::GetMenuState(hmenu, i, win32::MF_BYPOSITION) & win32::MF_SEPARATOR) continue;
        win32::GetMenuStringA(hmenu, i, szText, sizeof(szText), win32::MF_BYPOSITION);
        if (!strcmp(szText, pszMenu)) { iItem = cSteps; break; }
        cSteps++;
    }
    if (iItem < 0) {
        fprintf(stderr, "Requested scanner menu item '%s' was absent; entries:\n", pszMenu);
        for (i = 0; i < win32::GetMenuItemCount(hmenu); i++) {
            win32::GetMenuStringA(hmenu, i, szText, sizeof(szText), win32::MF_BYPOSITION);
            fprintf(stderr, "  %d: '%s'\n", i, szText);
        }
        FSaveState(app, "missing-menu");
    }
    pszMenu = NULL;
    if (iItem < 0 || !hwndRoot) { pszFailure = "requested scanner menu item was absent"; win32::PostQuitMessage(93); return 0; }
    for (j = 0; j < iItem + 2; j++) {
        event.type = SDL_EVENT_KEY_DOWN; event.key.windowID = hwndRoot->id;
        event.key.scancode = j == 0 ? SDL_SCANCODE_HOME : j == iItem + 1 ? SDL_SCANCODE_RETURN : SDL_SCANCODE_DOWN;
        event.key.key = SDL_GetKeyFromScancode(event.key.scancode, SDL_KMOD_NONE, false); event.key.down = true;
        if (!SDL_PushEvent(&event)) return 0;
        event.type = SDL_EVENT_KEY_UP; event.key.down = false;
        if (!SDL_PushEvent(&event)) return 0;
    }
    return win32::TrackPopupMenu(hmenu, flags, x, y, reserved, hwnd, prc);
}

int StarsTest_MessageBoxA(HWND hwnd, LPCSTR pszText, LPCSTR pszCaption, UINT type) {
    /* Observe text at the boundary; the real message box still opens and its
     * real OK control is clicked. Its owner-drawn body has no WM_GETTEXT text.
     */
    fExpectedRejection = iStage == 3 && pszText &&
        strstr(pszText, "You have not yet completed all of the tutorial tasks");
    fprintf(stderr, "Message box at stage %d: %s\n", iStage, pszText ? pszText : "");
    return win32::MessageBoxA(hwnd, pszText, pszCaption, type);
}

static void LogState(APPSESSION &app, const char *pszEvent) {
    if (!lpEvents) return;
    CchNativePrintf(lpEvents, "{\"event\":\"%s\",\"stage\":%d,\"turn\":%u,\"page\":%d,\"instruction\":%d,\"active\":%d,\"autoComplete\":%d,\"actions\":%d,\"elapsedMs\":%llu,\"selection\":[%d,%d],\"turnDone\":%d,\"scannerMode\":%u,\"productionSelection\":%ld}\n",
            pszEvent, iStage, app.state.game.turn, app.state.tutor.idt / 8 + 1, app.state.tutor.idtBold, app.state.gd.fTutorial, app.state.tutor.fAutoComplete, cActions, (unsigned long long)SDL_GetTicks(),
            app.state.sel.grobj, app.state.sel.id, app.state.tutor.fTurnDone, app.lpui->grbitScan & grbitScanViewMask,
            win32::IsWindow(app.lpui->hwndPlanetProdLB) ? (long)ui::SendMessage(app.lpui->hwndPlanetProdLB, win32::LB_GETCURSEL, 0, 0) : -1L);
    IFlushNativeFile(lpEvents);
}

static BOOL FSaveState(APPSESSION &app, const char *pszName) {
    STARSCOMPATSNAPSHOT snap = {};
    SCANSNAPSHOT scan = {};
    NATIVEFILE *lpfile;
    char szPath[100];
    BOOL fResult;

    snprintf(szPath, sizeof(szPath), "%s-hierarchy.json", pszName);
    if (!StarsCompatCaptureHierarchy(&snap)) return 0;
    lpfile = PfNativeOpen(szPath, NATIVEFILEMODE::writeTruncate);
    fResult = lpfile && StarsCompatWriteHierarchy(&snap, lpfile);
    if (lpfile && ICloseNativeFile(lpfile)) fResult = 0;
    StarsCompatFreeHierarchy(&snap);
    if (!fResult || !win32::IsWindow(app.lpui->hwndScanner)) return fResult;
    snprintf(szPath, sizeof(szPath), "%s-map.json", pszName);
    if (!FCaptureScanner(app, &scan)) return 0;
    lpfile = PfNativeOpen(szPath, NATIVEFILEMODE::writeTruncate);
    fResult = lpfile && FWriteScannerSnapshot(&scan, lpfile);
    if (lpfile && ICloseNativeFile(lpfile)) fResult = 0;
    FreeScannerSnapshot(&scan);
    return fResult;
}

static BOOL FPushKey(HWND hwnd, SDL_Scancode sc, BOOL fDown) {
    SDL_Event event = {};
    HWND hwndRoot = HwndCompatRoot(hwnd);
    // SDL_Event is a union; zero all bytes, including inactive-member fields.
    SDL_zero(event);
    if (!hwndRoot) return 0;
    event.type = fDown ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.windowID = hwndRoot->id;
    event.key.scancode = sc;
    event.key.key = SDL_GetKeyFromScancode(sc, SDL_KMOD_NONE, false);
    event.key.down = fDown;
    return SDL_PushEvent(&event);
}

static BOOL FPushTextKey(APPSESSION &app, SDL_Scancode sc, const char *pszText) {
    SDL_Event event = {};
    HWND hwndRoot = HwndCompatRoot(app.lpui->hwndFrame);
    SDL_zero(event);
    if (!hwndRoot || !FPushKey(app.lpui->hwndFrame, sc, 1)) return 0;
    /* SDL supplies printable characters separately from physical key events. */
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.windowID = hwndRoot->id;
    event.text.text = pszText;
    return SDL_PushEvent(&event) && FPushKey(app.lpui->hwndFrame, sc, 0);
}

static BOOL FMousePoint(HWND hwnd, POINT pt, Uint8 iButton, BOOL fDown) {
    HWND hwndRoot = HwndCompatRoot(hwnd);
    SDL_Event event = {};
    SDL_zero(event);
    if (!hwndRoot || !win32::IsWindowVisible(hwnd) || !win32::IsWindowEnabled(hwnd)) return 0;
    win32::ClientToScreen(hwnd, &pt);
    win32::ScreenToClient(hwndRoot, &pt);
    event.type = fDown ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.windowID = hwndRoot->id;
    event.button.button = iButton;
    event.button.down = fDown;
    event.button.x = pt.x;
    event.button.y = pt.y + CyCompatMenuBar(hwndRoot);
    return SDL_PushEvent(&event);
}

static BOOL FClickPoint(HWND hwnd, POINT pt, Uint8 iButton) {
    if (!FMousePoint(hwnd, pt, iButton, 1)) return 0;
    cActions++;
    return FMousePoint(hwnd, pt, iButton, 0);
}

static BOOL FClick(HWND hwnd) {
    RECT rc;
    POINT pt;
    if (!win32::IsWindow(hwnd) || !win32::GetClientRect(hwnd, &rc)) return 0;
    pt.x = (rc.right - rc.left) / 2;
    pt.y = (rc.bottom - rc.top) / 2;
    return FClickPoint(hwnd, pt, SDL_BUTTON_LEFT);
}

static BOOL FClickComboRow(HWND hwndCombo, int iRow) {
    HWND hwndDrop = hwndCombo->lpcombo ? hwndCombo->lpcombo->hwndPopup : NULL;
    RECT rc;
    POINT pt;
    int cy, iTop;
    if (!hwndDrop || !win32::GetClientRect(hwndDrop, &rc)) return 0;
    cy = (int)CompatListProc(hwndCombo, win32::LB_GETITEMHEIGHT, 0, 0);
    iTop = (int)CompatListProc(hwndCombo, win32::LB_GETTOPINDEX, 0, 0);
    pt = (POINT){rc.right / 2, 1 + (iRow - iTop) * cy + cy / 2};
    return cy > 0 && pt.y >= 1 && pt.y < rc.bottom - 1 && FClickPoint(hwndDrop, pt, SDL_BUTTON_LEFT);
}

static BOOL FChooseCombo(HWND hwndCombo, const char *pszName) {
    char szItem[256];
    int i;
    for (i = 0; i < ui::SendMessage(hwndCombo, win32::CB_GETCOUNT, 0, 0); i++) {
        if (ui::SendMessage(hwndCombo, win32::CB_GETLBTEXTLEN, i, 0) >= (LRESULT)sizeof(szItem)) return 0;
        ui::SendMessage(hwndCombo, win32::CB_GETLBTEXT, i, (LPARAM)szItem);
        if (!strcmp(szItem, pszName)) break;
    }
    if (i == ui::SendMessage(hwndCombo, win32::CB_GETCOUNT, 0, 0) || !FClick(hwndCombo)) return 0;
    // Commit by mouse after focus and keyboard navigation have settled.
    hwndDesignCombo = hwndCombo;
    iDesignCombo = i;
    return 1;
}

static BOOL FDragPoint(HWND hwnd, POINT ptFrom, POINT ptTo) {
    HWND hwndRoot = HwndCompatRoot(hwnd);
    SDL_Event event = {};
    if (!hwndRoot || !win32::ClientToScreen(hwnd, &ptFrom) || !win32::ScreenToClient(hwndRoot, &ptFrom) ||
        !win32::ClientToScreen(hwnd, &ptTo) || !win32::ScreenToClient(hwndRoot, &ptTo)) return 0;
    ptFrom.y += CyCompatMenuBar(hwndRoot); ptTo.y += CyCompatMenuBar(hwndRoot);
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.windowID = hwndRoot->id;
    event.button.button = SDL_BUTTON_LEFT; event.button.down = true;
    event.button.x = ptFrom.x; event.button.y = ptFrom.y;
    if (!SDL_PushEvent(&event)) return 0;
    event = (SDL_Event){0};
    event.type = SDL_EVENT_MOUSE_MOTION; event.motion.windowID = hwndRoot->id;
    event.motion.state = SDL_BUTTON_LMASK; event.motion.x = ptTo.x; event.motion.y = ptTo.y;
    if (!SDL_PushEvent(&event)) return 0;
    event = (SDL_Event){0};
    event.type = SDL_EVENT_MOUSE_BUTTON_UP; event.button.windowID = hwndRoot->id;
    event.button.button = SDL_BUTTON_LEFT; event.button.x = ptTo.x; event.button.y = ptTo.y;
    cActions++;
    return SDL_PushEvent(&event);
}

/* Resolve a fresh hierarchy every time; a callback can destroy the old dialog. */
static HWND HwndModal(void) {
    STARSCOMPATSNAPSHOT snap = {};
    HWND hwnd = NULL, hwndT;
    size_t i;
    if (!StarsCompatCaptureHierarchy(&snap)) return NULL;
    for (i = 0; i < snap.cWindow; i++) {
        hwndT = StarsCompatResolveWindow(snap.rgWindow[i].id);
        /* Snapshots list older windows first; a nested modal disables its owner. */
        if (hwndT && hwndT->fModal && snap.rgWindow[i].fVisible && win32::IsWindowEnabled(hwndT)) hwnd = hwndT;
    }
    StarsCompatFreeHierarchy(&snap);
    return hwnd;
}

static BOOL FFindMapObject(APPSESSION &app, uint16_t grType, uint16_t idObject, const char *pszName) {
    SCANSNAPSHOT snap = {};
    size_t i;
    BOOL fFound = 0, fReveal = 0;
    if (!FCaptureScanner(app, &snap)) return 0;
    for (i = 0; i < snap.cObject; i++) {
        if (snap.rgobj[i].grType == grType && (pszName ? !strcmp(snap.rgobj[i].szName, pszName) : snap.rgobj[i].id == idObject) && !snap.rgobj[i].fFiltered) {
            objTarget = snap.rgobj[i];
            if (!objTarget.fOnScreen) {
                fReveal = FSelectScannerObject(app, &snap, grType, objTarget.id, 1);
                break;
            }
            fFound = 1;
            break;
        }
    }
    FreeScannerSnapshot(&snap);
    if (fReveal) {
        LogState(app, "map-reveal");
        if (!FCaptureScanner(app, &snap)) return 0;
        for (i = 0; i < snap.cObject; i++) {
            if (snap.rgobj[i].grType == grType && snap.rgobj[i].id == objTarget.id && snap.rgobj[i].fOnScreen) {
                objTarget = snap.rgobj[i];
                fFound = 1;
                break;
            }
        }
        FreeScannerSnapshot(&snap);
    }
    return fFound;
}

static BOOL FFindPlanet(APPSESSION &app, const char *pszName) {
    return FFindMapObject(app, grobjPlanet, 0, pszName);
}

static BOOL FFindSalvageHere(APPSESSION &app) {
    SCANSNAPSHOT snap = {};
    POINT pt = objTarget.ptUniverse;
    char szName[64];
    size_t i;
    BOOL fFound = 0;

    CchGetString(app, idsSalvage, szName);
    if (!FCaptureScanner(app, &snap)) return 0;
    for (i = 0; i < snap.cObject; i++) {
        if (snap.rgobj[i].grType == grobjThing && snap.rgobj[i].ith == ithMineralPacket &&
            !snap.rgobj[i].fFiltered && snap.rgobj[i].ptUniverse.x == pt.x && snap.rgobj[i].ptUniverse.y == pt.y &&
            !strcmp(snap.rgobj[i].szName, szName)) {
            objTarget = snap.rgobj[i]; fFound = 1; break;
        }
    }
    FreeScannerSnapshot(&snap);
    return fFound;
}

static BOOL FPushControlKey(HWND hwnd, SDL_Scancode sc) {
    SDL_Event event = {};
    HWND hwndRoot = HwndCompatRoot(hwnd);

    if (!hwndRoot || !FPushKey(hwnd, SDL_SCANCODE_LCTRL, 1)) return 0;
    event.type = SDL_EVENT_KEY_DOWN; event.key.windowID = hwndRoot->id;
    event.key.scancode = sc; event.key.mod = SDL_KMOD_LCTRL;
    event.key.key = SDL_GetKeyFromScancode(sc, event.key.mod, false); event.key.down = true;
    if (!SDL_PushEvent(&event)) return 0;
    event.type = SDL_EVENT_KEY_UP; event.key.down = false;
    if (!SDL_PushEvent(&event) || !FPushKey(hwnd, SDL_SCANCODE_LCTRL, 0)) return 0;
    cActions++;
    return 1;
}

static BOOL FSaveOrders(APPSESSION &app, const char *pszPath) {
    NATIVEFILE *lpfile;
    FLEET *lpfl;
    ORDER *lpord;
    PLANET *lppl;
    PROD prod;
    SHDEF *lpshdef;
    int i, j, k, cWritten = 0;
    lpfile = PfNativeOpen(pszPath, NATIVEFILEMODE::writeTruncate);
    if (!lpfile) return 0;
    CchNativePrintf(lpfile, "{\"schema\":1,\"turn\":%u,\"turnDone\":%d,\"research\":[%u,%u,%d],\"scannerMode\":%u,\"nextMessage\":%d,\"filteredFactories\":%s,\"filteredMines\":%s,\"filteredUnloads\":%s,\"filteredMiningLoads\":%s,\"filteredDismantles\":%s,\"ownedFleetCount\":%u,\"fleets\":[",
            app.state.game.turn, app.state.tutor.fTurnDone, app.state.rgplr[0].iTechNow(), app.state.rgplr[0].iTechNext(), app.state.rgplr[0].pctResearch, app.lpui->grbitScan & grbitScanViewMask, IMsgNext(app, 0),
            app.state.bitfMsgFiltered[idmHaveBuiltFactories >> 3] & (1 << (idmHaveBuiltFactories & 7)) ? "true" : "false",
            app.state.bitfMsgFiltered[idmHaveBuiltMines >> 3] & (1 << (idmHaveBuiltMines & 7)) ? "true" : "false",
            app.state.bitfMsgFiltered[idmHasUnloaded >> 3] & (1 << (idmHasUnloaded & 7)) ? "true" : "false",
            app.state.bitfMsgFiltered[idmHasLoadedMiningRobotsWorking >> 3] & (1 << (idmHasLoadedMiningRobotsWorking & 7)) ? "true" : "false",
            app.state.bitfMsgFiltered[idmHasDismantledKtMineralsWhichHaveDeposited >> 3] & (1 << (idmHasDismantledKtMineralsWhichHaveDeposited & 7)) ? "true" : "false", app.state.rgplr[app.state.idPlayer].cFleet);
    for (i = 0; i < app.state.cFleet; i++) {
        lpfl = app.state.rglpfl[i];
        if (!lpfl || lpfl->fDead || lpfl->iPlayer != app.state.idPlayer) continue;
        CchNativePrintf(lpfile, "%s{\"id\":%d,\"repeatOrders\":%s,\"cargo\":[%ld,%ld,%ld,%ld,%ld],\"orders\":[", cWritten++ ? "," : "", lpfl->id, lpfl->fRepOrders ? "true" : "false",
                (long)lpfl->rgwtMin[0], (long)lpfl->rgwtMin[1], (long)lpfl->rgwtMin[2], (long)lpfl->rgwtMin[3], (long)lpfl->rgwtMin[4]);
        for (j = 0; j < lpfl->cord; j++) {
            lpord = &lpfl->lpplord->rgord()[j];
            CchNativePrintf(lpfile, "%s{\"kind\":%u,\"id\":%u,\"position\":[%d,%d],\"task\":%u,\"warp\":%u",
                    j ? "," : "", lpord->grobj, lpord->id, lpord->pt.x, lpord->pt.y, lpord->grTask, lpord->iWarp);
            if (lpord->grTask == grTaskXfer) {
                IPutNativeString(",\"transport\":[", lpfile);
                for (k = 0; k < 5; k++) {
                    CchNativePrintf(lpfile, "%s{\"action\":%u,\"quantity\":%u}", k ? "," : "",
                            lpord->txp.rgia[k].iAction(), lpord->txp.rgia[k].cQuan());
                }
                IPutNativeString("]", lpfile);
            }
            IPutNativeString("}", lpfile);
        }
        IPutNativeString("],\"ships\":[", lpfile);
        for (j = 0; j < 16; j++) CchNativePrintf(lpfile, "%s%d", j ? "," : "", lpfl->rgcsh[j]);
        IPutNativeString("]}", lpfile);
    }
    IPutNativeString("],\"designs\":[", lpfile);
    cWritten = 0;
    for (i = 0; i < 16; i++) {
        lpshdef = &app.state.rglpshdef[app.state.idPlayer][i];
        if (lpshdef->fFree) continue;
        CchNativePrintf(lpfile, "%s{\"index\":%d,\"hull\":%u,\"parts\":[", cWritten++ ? "," : "", i, lpshdef->hul.ihuldef);
        for (j = 0; j < lpshdef->hul.chs; j++)
            CchNativePrintf(lpfile, "%s[%u,%u,%u]", j ? "," : "", lpshdef->hul.rghs[j].grhst, lpshdef->hul.rghs[j].iItem, lpshdef->hul.rghs[j].cItem);
        IPutNativeString("]}", lpfile);
    }
    IPutNativeString("],\"production\":[", lpfile);
    cWritten = 0;
    for (i = 0; i < app.state.cPlanet; i++) {
        lppl = &app.state.lpPlanets[i];
        if (lppl->iPlayer != app.state.idPlayer || !lppl->lpplprod) continue;
        for (j = 0; j < lppl->lpplprod->iprodMac; j++) {
            prod = lppl->lpplprod->rgprod()[j];
            CchNativePrintf(lpfile, "%s{\"planet\":%u,\"kind\":%u,\"item\":%u,\"quantity\":%u}",
                    cWritten++ ? "," : "", lppl->id, prod.grobj, prod.iItem, prod.cItem);
        }
    }
    IPutNativeString("],\"planetFlags\":[", lpfile);
    cWritten = 0;
    for (i = 0; i < app.state.cPlanet; i++) {
        lppl = &app.state.lpPlanets[i];
        if (lppl->iPlayer != app.state.idPlayer) continue;
        CchNativePrintf(lpfile, "%s{\"planet\":%u,\"leftoverResearch\":%u}",
                cWritten++ ? "," : "", lppl->id, lppl->fNoResearch);
    }
    CchNativePrintf(lpfile, "],\"defaultTemplate\":{\"valid\":%s,\"leftoverResearch\":%u,\"count\":%u,\"items\":[",
            app.state.vrgZipProd[0].fValid ? "true" : "false", app.state.vrgZipProd[0].fNoResearch, app.state.vrgZipProd[0].cpq);
    for (j = 0; j < app.state.vrgZipProd[0].cpq && j < 12; j++)
        CchNativePrintf(lpfile, "%s{\"item\":%u,\"quantity\":%u}", j ? "," : "", app.state.vrgZipProd[0].rgpq[j].mdIdle(), app.state.vrgZipProd[0].rgpq[j].cQuan());
    IPutNativeString("]}}\n", lpfile);
    i = FNativeStreamError(lpfile);
    return ICloseNativeFile(lpfile) == 0 && !i;
}

static BOOL FSaveEnemyDesign(APPSESSION &app) {
    NATIVEFILE *lpfile;
    FLEET *lpfl;
    int ishdef, fError;

    if (app.state.sel.scan.grobj != grobjFleet || app.state.sel.scan.ifl == iflNone ||
        app.lpui->GlobalPD.grPopup != grPopupShdef || !app.lpui->GlobalPD.lpshdef) return 0;
    lpfl = app.state.rglpfl[app.state.sel.scan.ifl];
    for (ishdef = 0; ishdef < 16; ishdef++)
        if (lpfl->rgcsh[ishdef] != 0 && &app.state.rglpshdef[lpfl->iPlayer][ishdef] == app.lpui->GlobalPD.lpshdef) break;
    if (ishdef == 16) return 0;
    lpfile = PfNativeOpen("enemy-design.json", NATIVEFILEMODE::writeTruncate);
    if (!lpfile) return 0;
    CchNativePrintf(lpfile, "{\"fleet\":%d,\"player\":%u,\"design\":%d,\"hideCounts\":%s,\"summary\":%s,\"showDamage\":%s,\"token\":%s}\n",
            lpfl->id, lpfl->iPlayer, ishdef, app.lpui->GlobalPD.fHideCounts ? "true" : "false",
            app.lpui->GlobalPD.fSummary ? "true" : "false", app.lpui->GlobalPD.fShowDamage ? "true" : "false", app.lpui->GlobalPD.fToken ? "true" : "false");
    fError = FNativeStreamError(lpfile);
    return ICloseNativeFile(lpfile) == 0 && !fError;
}

static BOOL FSaveTutorialPopup(APPSESSION &app, const char *pszPath, BOOL fResearch) {
    NATIVEFILE *lpfile;
    int fError;

    if (!win32::IsWindowVisible(app.lpui->hwndPopup) || win32::GetCapture() != app.lpui->hwndPopup) return 0;
    if (fResearch) {
        if (app.lpui->GlobalPD.grPopup != grPopupComponent || app.lpui->GlobalPD.part.hs.grhst != hstTerra ||
            app.lpui->GlobalPD.part.hs.iItem != iterraRadiationTerraform7) return 0;
    } else if (app.lpui->GlobalPD.grPopup != grPopupPlanetEnv || app.lpui->GlobalPD.idPlanet != 5 || app.lpui->GlobalPD.iPlanetVar != 2) return 0;
    lpfile = PfNativeOpen(pszPath, NATIVEFILEMODE::writeTruncate);
    if (!lpfile) return 0;
    if (fResearch) CchNativePrintf(lpfile, "{\"type\":%d,\"kind\":%u,\"item\":%u}\n",
                           app.lpui->GlobalPD.grPopup, app.lpui->GlobalPD.part.hs.grhst, app.lpui->GlobalPD.part.hs.iItem);
    else CchNativePrintf(lpfile, "{\"type\":%d,\"planet\":%d,\"environment\":%d}\n",
                 app.lpui->GlobalPD.grPopup, app.lpui->GlobalPD.idPlanet, app.lpui->GlobalPD.iPlanetVar);
    fError = FNativeStreamError(lpfile);
    return ICloseNativeFile(lpfile) == 0 && !fError;
}

static BOOL FSaveBattle(APPSESSION &app, const char *pszPath) {
    NATIVEFILE *lpfile;
    int fError;
    if (!app.lpui->vlpbdVCR || !app.state.vrgtok) return 0;
    lpfile = PfNativeOpen(pszPath, NATIVEFILEMODE::writeTruncate);
    if (!lpfile) return 0;
    CchNativePrintf(lpfile, "{\"battle\":%u,\"tokens\":%u,\"step\":%d,\"lastStep\":%d,\"playing\":%s}\n",
            BdvFromLp(app, app.lpui->vlpbdVCR).id(), BdvFromLp(app, app.lpui->vlpbdVCR).ctok(), app.lpui->viStepVCRCur, app.lpui->vcStepVCR, app.state.gd.fVCRTimer ? "true" : "false");
    fError = FNativeStreamError(lpfile);
    return ICloseNativeFile(lpfile) == 0 && !fError;
}

static BOOL FSaveSalvage(APPSESSION &app) {
    THING *lpth;
    NATIVEFILE *lpfile;
    int fError;
    if (app.state.sel.scan.grobj != grobjThing || app.state.sel.scan.ith < 0 || app.state.sel.scan.ith >= app.state.cThing) return 0;
    lpth = &app.state.lpThings[app.state.sel.scan.ith];
    if (lpth->ith() != ithMineralPacket || lpth->thp().iWarp() != 0) return 0;
    lpfile = PfNativeOpen("salvage.json", NATIVEFILEMODE::writeTruncate);
    if (!lpfile) return 0;
    CchNativePrintf(lpfile, "{\"id\":%u,\"position\":[%d,%d],\"minerals\":[%d,%d,%d]}\n",
            lpth->idFull(), lpth->pt().x(), lpth->pt().y(), lpth->thp().rgwtMin(0), lpth->thp().rgwtMin(1), lpth->thp().rgwtMin(2));
    fError = FNativeStreamError(lpfile);
    return ICloseNativeFile(lpfile) == 0 && !fError;
}

static BOOL FSaveReport(APPSESSION &app, const char *pszPath) {
    NATIVEFILE *lpfile;
    int i, fError;
    if (!win32::IsWindow(app.lpui->hwndReportDlg) || !app.lpui->vprptCur || app.lpui->vprptCur->irpt != rptPlanets) return 0;
    lpfile = PfNativeOpen(pszPath, NATIVEFILEMODE::writeTruncate);
    if (!lpfile) return 0;
    CchNativePrintf(lpfile, "{\"turn\":%u,\"type\":%u,\"column\":%d,\"subsort\":%d,\"ascending\":%s,\"planetIds\":[",
            app.state.game.turn, app.lpui->vprptCur->irpt, app.lpui->vprptCur->icolSort, app.lpui->vprptCur->iSubsort, app.lpui->vprptCur->fAscending ? "true" : "false");
    for (i = 0; i < app.lpui->vprptCur->cRows; i++) CchNativePrintf(lpfile, "%s%u", i ? "," : "", app.state.lpPlanets[app.lpui->vlprgidRep[i]].id);
    IPutNativeString("]}\n", lpfile);
    fError = FNativeStreamError(lpfile);
    return ICloseNativeFile(lpfile) == 0 && !fError;
}

static void  RunTutorial(HWND hwndTimer, UINT message, UINT_PTR id, DWORD tick) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HWND hwndModal;
    FLEET *lpfl;
    int iPage, i;
    HWND hwndList;
    RECT rcRow;
    POINT ptRow, ptTo;
    POINT16 ptLogical;
    char szItem[256];
    const char *pszPlanet;
    static const struct { int idt; const char *pszName; } rgScout[] = {
        {50, "Hiho"}, {51, "No Vacancy"}, {52, "Slime"}, {53, "Wallaby"}, {54, "Oxygen"},
        {57, "Dwarte"}, {58, "Mobius"}, {59, "Castle"}, {60, "Mohlodi"},
        {64, "Shaggy Dog"}, {65, "Sea Squared"}, {66, "Red Storm"}, {67, "Bloop"}, {68, "Kalamazoo"}
    };
    static const struct { const char *pszName; int iItem, cGoal, iQueue, grobj; BOOL fShift, fLeftovers; } rgProduction[] = {
        {"Factories (Auto Build)", iobjFactory, 30, 0, grobjPlanet, 1, 0},
        {"Factory", mdIdleFactory, 3, 0, grobjPlanet, 0, 1},
        {"Mine", mdIdleMine, 3, 1, grobjPlanet, 0, 1},
        {"Santa Maria", 2, 1, 1, grobjFleet, 0, 0},
        {"Santa Maria", 2, 3, 1, grobjFleet, 0, 0},
        {"Factories (Auto Build)", iobjFactory, 10, 0, grobjPlanet, 1, 1},
        {"Mines (Auto Build)", iobjMine, 10, 1, grobjPlanet, 1, 1},
        {"Armed Probe", 0, 2, 2, grobjFleet, 0, 0},
        {"Factories (Auto Build)", iobjFactory, 3, 0, grobjPlanet, 0, 1},
        {"Mines (Auto Build)", iobjMine, 3, 1, grobjPlanet, 0, 1},
        {"Teamster", 3, 1, 2, grobjFleet, 0, 0},
        {"Mine", mdIdleMine, 70, 0, grobjPlanet, 1, 0},
        {"Terraform Environment", mdIdleTerraform, 2, 0, grobjPlanet, 0, 1},
        {"Teamster", 3, 2, 1, grobjFleet, 0, 0},
        {"Mini-Miner", 6, 1, 1, grobjFleet, 0, 0},
        {"Mine", mdIdleMine, 100, 0, grobjPlanet, 0, 0},
        {"Min Terraform (Auto Build)", iobjMinTerraform, 2, 0, grobjPlanet, 0, 1},
        {"Factories (Auto Build)", iobjFactory, 60, 0, grobjPlanet, 1, 0},
        {"Mines (Auto Build)", iobjMine, 60, 1, grobjPlanet, 1, 0}
    };
    (void)hwndTimer; (void)message; (void)tick;

    if (++cWait > 300) { pszFailure = "bounded tutorial action timed out"; goto LFail; }
    if (app.state.tutor.fAutoComplete) { pszFailure = "tutorial auto-completion was used"; goto LFail; }
    iPage = app.state.tutor.idt / 8;
    if (app.state.gd.fTutorial && iPage >= 0 && iPage < 80 && !rgfPage[iPage]) { rgfPage[iPage] = 1; cPage++; }
    hwndModal = HwndModal();
    switch (iStage) {
    case 0:
        if (!win32::IsWindowVisible(app.lpui->hwndTitle)) return;
        if (!FClick(win32::GetDlgItem(app.lpui->hwndTitle, IDC_LIBRARY_TUTORIAL))) goto LInput;
        break;
    case 1:
        if (!app.state.gd.fTutorial || !win32::IsWindow(app.lpui->hwndScanner)) return;
        break;
    case 2:
        if (!app.state.gd.fTutorial || app.state.tutor.idtBold != 5 || !win32::IsWindow(app.lpui->hwndScanner) || hwndModal) return;
        if (app.state.game.turn != 0 || app.state.tutor.fTurnDone) { pszFailure = "unexpected initial tutorial state"; goto LFail; }
        if (!FSaveState(app, "start")) goto LArtifact;
        if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
        cActions++;
        break;
    case 3:
        if (!hwndModal) return;
        if (!fExpectedRejection) {
            pszFailure = "unexpected dialog instead of unfinished-task rejection";
            goto LFail;
        }
        if (!FSaveState(app, "rejected") || !FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        fRejected = 1;
        break;
    case 4:
        if (hwndModal) return;
        if (!app.state.gd.fTutorial || app.state.game.turn != 0) { pszFailure = "premature Generate changed tutorial state"; goto LFail; }
        if (app.state.tutor.idtBold == 5) {
            if (++cReadActions > 100) { pszFailure = "messages did not advance after bounded clicks"; goto LFail; }
            if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2])) return;
            if (!FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
            cWait = 0;
            LogState(app, "read-message");
            return;
        }
        if (app.state.tutor.idtBold != 11 || app.state.tutor.idt != 8) { pszFailure = "reading messages did not reach page two"; goto LFail; }
        if (!FSaveState(app, "page-two")) goto LArtifact;
        if (!FClick(app.lpui->rghwndBtn[1])) goto LInput;
        break;
    case 5:
        if (hwndModal) { pszFailure = "unexpected dialog after Goto"; goto LFail; }
        if (app.state.tutor.idtBold != 15 || app.state.sel.grobj != grobjFleet || app.state.sel.id != 0) return;
        if (!FFindPlanet(app, "Prune")) { pszFailure = "Prune missing from visible map inspection"; goto LFail; }
        if (!FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) || !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) ||
            !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
        break;
    case 6:
        if (hwndModal) { pszFailure = "unexpected dialog after map order"; goto LFail; }
        if (app.state.tutor.idt != 16 || app.state.tutor.idtBold != 18) return;
        lpfl = LpflFromId(app, 0);
        if (!lpfl || lpfl->cord != 2 || lpfl->lpplord->rgord()[1].grobj != grobjPlanet || lpfl->lpplord->rgord()[1].id != objTarget.id ||
            lpfl->lpplord->rgord()[1].pt.x != objTarget.ptUniverse.x || lpfl->lpplord->rgord()[1].pt.y != objTarget.ptUniverse.y) {
            pszFailure = "Shift-click did not issue the inspected planet order";
            goto LFail;
        }
        if (!FSaveState(app, "page-three")) goto LArtifact;
        if (fFirstYear) { iStage = 7; goto LQueued; }
        fPassed = fRejected && app.state.gd.fTutorial && app.state.game.turn == 0 && !app.state.tutor.fTurnDone;
        LogState(app, "smoke-passed");
        win32::KillTimer(NULL, id);
        win32::PostQuitMessage(fPassed ? 0 : 93);
        return;
    case 7:
        if (hwndModal || !app.state.gd.fTutorial || app.state.game.turn != 0) { pszFailure = "unexpected first-year state or dialog"; goto LFail; }
        idtBefore = app.state.tutor.idtBold;
        LogState(app, "instruction");
        switch (app.state.tutor.idtBold) {
        case 18: case 25: case 27: case 29: case 32: case 34:
            if (!FPushTextKey(app, SDL_SCANCODE_N, "n")) goto LInput;
            cActions++;
            iStage = 11;
            goto LQueued;
        case 21: case 31:
            pszPlanet = app.state.tutor.idtBold == 21 ? "90210" : "Alexander";
            if (app.state.sel.grobj != grobjFleet || app.state.sel.id != (app.state.tutor.idtBold == 21 ? 1 : 4)) {
                pszFailure = "tutorial map instruction selected the wrong fleet"; goto LFail;
            }
            if (!FFindPlanet(app, pszPlanet) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) ||
                !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
            iStage = 11;
            goto LQueued;
        case 37:
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F5, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F5, 0)) goto LInput;
            cActions++;
            break;
        case 39:
            if (!app.state.tutor.fTurnDone || app.state.rgplr[0].iTechNow() != Weapons || app.state.rgplr[0].iTechNext() != TechFieldCount || app.state.rgplr[0].pctResearch != 15) {
                pszFailure = "Generate requested before first-year tasks passed"; goto LFail;
            }
            if (!FSaveOrders(app, "year-2400-orders.json") || !FSaveState(app, "before-generate")) goto LArtifact;
            LogState(app, "generate-ready");
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
            cActions++;
            iStage = 12;
            goto LQueued;
        default:
            pszFailure = "unimplemented first-year instruction"; goto LFail;
        }
        break;
    case 8:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != ResearchDlg || app.state.tutor.idtBold != 38) { pszFailure = "unexpected research dialog"; goto LFail; }
        if (!FClick(win32::GetDlgItem(hwndModal, IDC_RESEARCH_ENERGY + (int)Weapons))) goto LInput;
        break;
    case 9:
        if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg) { pszFailure = "research dialog vanished"; goto LFail; }
        if (app.lpui->iResTechNow != Weapons) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDCANCEL))) goto LInput;
        break;
    case 10:
        if (hwndModal) return;
        if (app.state.rgplr[0].iTechNow() != Weapons || app.lpui->pctResGlob != -1) { pszFailure = "research choice was not accepted"; goto LFail; }
        iStage = 7;
        goto LQueued;
    case 11:
        if (hwndModal) { pszFailure = "unexpected fleet instruction dialog"; goto LFail; }
        if (app.state.tutor.idtBold == idtBefore) return;
        iStage = 7;
        goto LQueued;
    case 12:
        if (hwndModal) { pszFailure = "unexpected generation dialog"; goto LFail; }
        if (app.state.game.turn == 0) return;
        if (app.state.game.turn != 1 || !app.state.gd.fTutorial || app.state.tutor.idt != 40 || app.state.tutor.idtBold != 42) {
            pszFailure = "first generation did not reach the year-2401 production task"; goto LFail;
        }
        if (!FSaveState(app, "year-2401")) goto LArtifact;
        if (fTwoYears) { iStage = 13; goto LQueued; }
        fPassed = fRejected && cPage == 6;
        LogState(app, "first-year-partial");
        win32::KillTimer(NULL, id);
        win32::PostQuitMessage(fPassed ? 0 : 93);
        return;
    case 13:
        if (!FFindPlanet(app, "Stove Top")) goto LInput;
        pszMenu = "Stove Top";
        if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
        break;
    case 14:
        if (pszMenu || app.state.sel.grobj != grobjPlanet || app.state.sel.id != 13) return;
        if (!FPushTextKey(app, SDL_SCANCODE_Q, "q")) goto LInput;
        cActions++;
        break;
    case 15:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != ProductionDlg || !app.state.lpplProdGlob || app.state.tutor.idtBold != 43) { pszFailure = "unexpected production dialog"; goto LFail; }
        hwndList = win32::GetDlgItem(hwndModal, IDC_PRODUCTION_AVAILABLE_ITEMS);
        iFactoryRow = -1;
        for (i = 0; i < ui::SendMessage(hwndList, win32::LB_GETCOUNT, 0, 0); i++) {
            if (ui::SendMessage(hwndList, win32::LB_GETTEXTLEN, i, 0) >= (LRESULT)sizeof(szItem)) goto LInput;
            ui::SendMessage(hwndList, win32::LB_GETTEXT, i, (LPARAM)szItem);
            if (strlen(szItem) >= 6 && !strcmp(szItem + 6, "Factory")) { iFactoryRow = i; break; }
        }
        if (iFactoryRow < 0 || ui::SendMessage(hwndList, win32::LB_GETITEMRECT, ui::SendMessage(hwndList, win32::LB_GETTOPINDEX, 0, 0), (LPARAM)&rcRow) == win32::LB_ERR) goto LInput;
        ptRow.x = (rcRow.left + rcRow.right) / 2; ptRow.y = (rcRow.top + rcRow.bottom) / 2;
        if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
        if (!FPushKey(hwndList, SDL_SCANCODE_HOME, 1) || !FPushKey(hwndList, SDL_SCANCODE_HOME, 0)) goto LInput;
        for (i = 0; i < iFactoryRow; i++) {
            if (!FPushKey(hwndList, SDL_SCANCODE_DOWN, 1) || !FPushKey(hwndList, SDL_SCANCODE_DOWN, 0)) goto LInput;
        }
        break;
    case 16:
        if (!hwndModal || hwndModal->lpfnDlg != ProductionDlg) goto LInput;
        if (ui::SendMessage(win32::GetDlgItem(hwndModal, IDC_PRODUCTION_AVAILABLE_ITEMS), win32::LB_GETCURSEL, 0, 0) != iFactoryRow) return;
        /* Pushed SDL events do not change device state read by GetAsyncKeyState.
         * Hold the test modifier until the real Add callbacks have completed. */
        SDL_SetModState(SDL_KMOD_LSHIFT);
        if (!FPushKey(hwndModal, SDL_SCANCODE_LSHIFT, 1) || !FClick(win32::GetDlgItem(hwndModal, IDC_PRODUCTION_ADD))) goto LInput;
        break;
    case 17:
        if (!app.state.lpplProdGlob || app.state.lpplProdGlob->iprodMac != 1 || app.state.lpplProdGlob->rgprod()[0].cItem != 10) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDC_PRODUCTION_ADD))) goto LInput;
        break;
    case 18:
        if (!app.state.lpplProdGlob || app.state.lpplProdGlob->iprodMac != 1 || app.state.lpplProdGlob->rgprod()[0].cItem != 20) return;
        if (app.state.lpplProdGlob->rgprod()[0].grobj != grobjPlanet || app.state.lpplProdGlob->rgprod()[0].iItem != mdIdleFactory) goto LInput;
        SDL_SetModState(SDL_KMOD_NONE);
        if (!FPushKey(hwndModal, SDL_SCANCODE_LSHIFT, 0) || !FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 19:
        if (hwndModal || app.state.tutor.idtBold != 47 || !app.state.tutor.fTurnDone) return;
        if (!LpplFromId(app, 13)->lpplprod || LpplFromId(app, 13)->lpplprod->iprodMac != 1 ||
            LpplFromId(app, 13)->lpplprod->rgprod()[0].cItem != 20) goto LInput;
        if (!FSaveOrders(app, "year-2401-orders.json") || !FSaveState(app, "before-generate-2401")) goto LArtifact;
        LogState(app, "generate-ready");
        if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
        cActions++;
        break;
    case 20:
        if (hwndModal) { pszFailure = "unexpected second-generation dialog"; goto LFail; }
        if (app.state.game.turn == 1) return;
        if (app.state.game.turn != 2 || !app.state.gd.fTutorial || app.state.tutor.idt != 48 || app.state.tutor.idtBold != 48) { pszFailure = "second generation did not reach page seven"; goto LFail; }
        if (!FSaveState(app, "year-2402")) goto LArtifact;
        if (fScouting) { iStage = 21; goto LQueued; }
        fPassed = fRejected && cPage == 7;
        LogState(app, "two-years-partial");
        win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
    case 21:
        if (hwndModal || app.state.game.turn != 2 || !app.state.gd.fTutorial) { pszFailure = "unexpected scouting state or dialog"; goto LFail; }
        idtBefore = app.state.tutor.idtBold; iMsgBefore = app.state.iMsgCur;
        LogState(app, "scouting-instruction");
        if (app.state.tutor.idtBold == 76) {
            if (!FSaveOrders(app, "year-2402-scout-orders.json") || !FSaveState(app, "scouting")) goto LArtifact;
            if (fMining) { iStage = 22; goto LQueued; }
            fPassed = fRejected && cPage == 10;
            LogState(app, "scouting-partial");
            win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
        }
        if (app.state.tutor.idtBold == 48 || app.state.tutor.idtBold == 71) {
            if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
            iStage = 24; goto LQueued;
        }
        if (app.state.tutor.idtBold == 55 || app.state.tutor.idtBold == 61 || app.state.tutor.idtBold == 63 || app.state.tutor.idtBold == 69) {
            if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
            iStage = 23; goto LQueued;
        }
        for (i = 0; i < (int)(sizeof(rgScout) / sizeof(rgScout[0])); i++) {
            if (app.state.tutor.idtBold != rgScout[i].idt) continue;
            if (!FFindPlanet(app, rgScout[i].pszName) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) ||
                !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
            iStage = 24; goto LQueued;
        }
        pszFailure = "unimplemented scouting instruction"; goto LFail;
    case 22:
        if (!win32::GetClientRect(app.lpui->hwndFrame, &rcRow)) goto LInput;
        /* A private window manager may already have maximized the frame;
         * requesting a smaller SDL size then fails while maximized. */
        if (rcRow.bottom < 650 && !ui::SetWindowPos(app.lpui->hwndFrame, NULL, 0, 0, 900, 700,
                                               win32::SWP_NOMOVE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE)) {
            pszFailure = "unable to resize tutorial frame"; goto LFail;
        }
        iStage = 25; goto LQueued;
    case 23:
        if (hwndModal) { pszFailure = "unexpected message navigation dialog"; goto LFail; }
        if (app.state.iMsgCur == iMsgBefore) return;
        if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
        iStage = 24; goto LQueued;
    case 24:
        if (hwndModal) { pszFailure = "unexpected scouting action dialog"; goto LFail; }
        if (app.state.tutor.idtBold == idtBefore && app.state.iMsgCur == iMsgBefore) return;
        iStage = 21; goto LQueued;
    case 25:
        if (!FFindMapObject(app, grobjFleet, 5, NULL)) goto LInput;
        pszMenu = objTarget.szName;
        if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
        break;
    case 26:
        if (pszMenu || app.state.sel.grobj != grobjFleet || app.state.sel.id != 5 || app.state.tutor.idtBold != 77) return;
        if (!FFindPlanet(app, "Prune") || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) ||
            !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
        break;
    case 27:
        if (app.state.tutor.idtBold != 79) return;
        if (!FClick(app.lpui->rghwndOrderDD[0])) goto LInput;
        break;
    case 28:
        if (!ui::SendMessage(app.lpui->rghwndOrderDD[0], win32::CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(app.lpui->rghwndOrderDD[0], grTaskMine)) goto LInput;
        break;
    case 29:
        if (ui::SendMessage(app.lpui->rghwndOrderDD[0], win32::CB_GETDROPPEDSTATE, 0, 0) || app.state.tutor.idt != 80 || app.state.tutor.idtBold != 80) return;
        lpfl = LpflFromId(app, 5);
        if (!lpfl || lpfl->cord != 2 || lpfl->lpplord->rgord()[1].grobj != grobjPlanet ||
            lpfl->lpplord->rgord()[1].id != 12 || lpfl->lpplord->rgord()[1].grTask != grTaskMine) goto LInput;
        if (!FSaveOrders(app, "year-2402-mining-orders.json") || !FSaveState(app, "mining")) goto LArtifact;
        if (fColonization) { iStage = 30; goto LQueued; }
        fPassed = fRejected && cPage == 11;
        LogState(app, "mining-partial");
        win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
    case 30:
        if (hwndModal || app.state.game.turn != 2) { pszFailure = "unexpected colonization state or dialog"; goto LFail; }
        idtBefore = app.state.tutor.idtBold; iMsgBefore = app.state.iMsgCur;
        LogState(app, "colonization-instruction");
        if (app.state.tutor.idtBold == 80 || app.state.tutor.idtBold == 84 || app.state.tutor.idtBold == 86) {
            if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
            iStage = 31; goto LQueued;
        }
        if (app.state.tutor.idtBold == 83) {
            ptRow = (POINT){30, 65};
            if (!FMousePoint(app.lpui->hwndMine, ptRow, SDL_BUTTON_LEFT, 1)) goto LInput;
            cActions++;
            iStage = 41; goto LQueued;
        }
        if (app.state.tutor.idtBold == 89) {
            if (!FFindMapObject(app, grobjFleet, 2, NULL)) goto LInput;
            pszMenu = objTarget.szName;
            if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 33; goto LQueued;
        }
        pszFailure = "unimplemented colonization instruction"; goto LFail;
    case 31:
        if (hwndModal) { pszFailure = "unexpected colonization message dialog"; goto LFail; }
        if (app.state.iMsgCur == iMsgBefore) return;
        if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
        iStage = 32; goto LQueued;
    case 32:
        if (app.state.tutor.idtBold == idtBefore && app.state.iMsgCur == iMsgBefore) return;
        iStage = 30; goto LQueued;
    case 33:
        if (pszMenu || app.state.sel.grobj != grobjFleet || app.state.sel.id != 2 || app.state.tutor.idtBold != 90) return;
        if (!FClick(app.lpui->rghwndBtn[7])) goto LInput;
        break;
    case 34:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || app.lpui->mdXferDlg != mdXferCargo || !app.state.pxfer ||
            app.state.pxfer[0].id != 2 || app.state.pxfer[0].grobj != grobjFleet || app.state.pxfer[1].id != 13 || app.state.pxfer[1].grobj != grobjPlanet) {
            pszFailure = "unexpected colonist transfer dialog"; goto LFail;
        }
        for (i = 0; i < app.lpui->crgbtnXfer; i++) {
            if ((app.lpui->rgbtnXfer[i].iVal & 0x7f) == 3 && app.lpui->rgbtnXfer[i].iSide == 0 && !app.lpui->rgbtnXfer[i].fVisible && !(app.lpui->rgbtnXfer[i].bt & 4)) break;
        }
        if (i == app.lpui->crgbtnXfer) goto LInput;
        memcpy(&rcRow, &app.lpui->rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){rcRow.left + 2, (rcRow.top + rcRow.bottom) / 2};
        ptTo = (POINT){rcRow.right - 2, ptRow.y};
        if (!FDragPoint(hwndModal, ptRow, ptTo)) goto LInput;
        break;
    case 35:
        if (!hwndModal || !app.state.pxfer || app.state.pxfer[0].fl().rgwtMin[3] != 25) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 36:
        if (hwndModal || app.state.tutor.idtBold != 92) return;
        lpfl = LpflFromId(app, 2);
        if (!lpfl || lpfl->rgwtMin[0] != 0 || lpfl->rgwtMin[1] != 0 || lpfl->rgwtMin[2] != 0 || lpfl->rgwtMin[3] != 25) goto LInput;
        if (!FFindPlanet(app, "90210") || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) ||
            !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
        break;
    case 37:
        if (app.state.tutor.idtBold != 93) return;
        if (!FClick(app.lpui->rghwndOrderDD[0])) goto LInput;
        break;
    case 38:
        if (!ui::SendMessage(app.lpui->rghwndOrderDD[0], win32::CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(app.lpui->rghwndOrderDD[0], grTaskColonize)) goto LInput;
        break;
    case 39:
        if (hwndModal || !app.state.tutor.fTurnDone || app.state.tutor.idtBold != 95) return;
        if (!FSaveOrders(app, "year-2402-orders.json") || !FSaveState(app, "before-generate-2402")) goto LArtifact;
        LogState(app, "generate-ready");
        if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
        cActions++;
        break;
    case 40:
        if (hwndModal) { pszFailure = "unexpected third-generation dialog"; goto LFail; }
        if (app.state.game.turn == 2) return;
        if (app.state.game.turn != 3 || !app.state.gd.fTutorial || app.state.tutor.idt != 96 || app.state.tutor.idtBold != 97) {
            pszFailure = "third generation did not reach page thirteen"; goto LFail;
        }
        if (!FSaveState(app, "year-2403")) goto LArtifact;
        if (fLogistics) {
            if (!FSaveOrders(app, "year-2403-start-orders.json")) goto LArtifact;
            iStage = 42; goto LQueued;
        }
        fPassed = fRejected && cPage == 13;
        LogState(app, "colonization-partial");
        win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
    case 41:
        if (!win32::IsWindow(app.lpui->hwndPopup)) return;
        if (!win32::IsWindowVisible(app.lpui->hwndPopup) || win32::GetCapture() != app.lpui->hwndPopup || !win32::GetClientRect(app.lpui->hwndPopup, &rcRow)) goto LInput;
        if (!FSaveState(app, "summary-popup")) goto LArtifact;
        LogState(app, "summary-popup");
        ptRow = (POINT){rcRow.right / 2, rcRow.bottom / 2};
        if (!FMousePoint(app.lpui->hwndPopup, ptRow, SDL_BUTTON_LEFT, 0)) goto LInput;
        iStage = 32; goto LQueued;
    case 42:
        if (hwndModal || app.state.game.turn != 3) { pszFailure = "unexpected logistics state or dialog"; goto LFail; }
        idtBefore = app.state.tutor.idtBold; iMsgBefore = app.state.iMsgCur;
        LogState(app, "logistics-instruction");
        if (app.state.tutor.idtBold == 97) {
            ptRow = (POINT){app.lpui->rcMsgTitle.left + (app.lpui->rcMsgTitle.bottom - app.lpui->rcMsgTitle.top) / 2,
                            (app.lpui->rcMsgTitle.top + app.lpui->rcMsgTitle.bottom) / 2};
            if (!FClickPoint(app.lpui->hwndMessage, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 44; goto LQueued;
        }
        if (app.state.tutor.idtBold == 98 || app.state.tutor.idtBold == 104 || app.state.tutor.idtBold == 115) {
            if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
            iStage = 43; goto LQueued;
        }
        if (app.state.tutor.idtBold == 101 || app.state.tutor.idtBold == 106) {
            iProdPass = app.state.tutor.idtBold == 101 ? 0 : 1;
            iProdReturn = 44;
            cProdAdded = 0;
            if (!FPushTextKey(app, SDL_SCANCODE_Q, "q")) goto LInput;
            cActions++; iStage = 45; goto LQueued;
        }
        if (app.state.tutor.idtBold == 109 || app.state.tutor.idtBold == 118) {
            if (!FFindMapObject(app, grobjFleet, app.state.tutor.idtBold == 109 ? 3 : 0, NULL)) goto LInput;
            pszMenu = objTarget.szName;
            if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = app.state.tutor.idtBold == 109 ? 50 : 58; goto LQueued;
        }
        if (app.state.tutor.idtBold == 121) {
            if (!FFindPlanet(app, "Hiho") || !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 44; goto LQueued;
        }
        if (app.state.tutor.idtBold == 123) {
            if (ui::SendMessage(app.lpui->hwndShipLB, win32::LB_GETITEMRECT, 1, (LPARAM)&rcRow) == win32::LB_ERR) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            if (!FClickPoint(app.lpui->hwndShipLB, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 59; goto LQueued;
        }
        if (app.state.tutor.idtBold == 127) {
            if (!app.state.tutor.fTurnDone || !FSaveOrders(app, "year-2403-orders.json") || !FSaveState(app, "before-generate-2403")) goto LArtifact;
            LogState(app, "generate-ready");
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
            cActions++; iStage = 61; goto LQueued;
        }
        pszFailure = "unimplemented logistics instruction"; goto LFail;
    case 43:
        if (hwndModal) { pszFailure = "unexpected logistics message dialog"; goto LFail; }
        if (app.state.iMsgCur == iMsgBefore) return;
        if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
        iStage = 44; goto LQueued;
    case 44:
        if (app.state.tutor.idtBold == idtBefore && app.state.iMsgCur == iMsgBefore) return;
        iStage = 42; goto LQueued;
    case 45:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != ProductionDlg || !app.state.lpplProdGlob) goto LInput;
        if (rgProduction[iProdPass].fLeftovers && ui::SendMessage(win32::GetDlgItem(hwndModal, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY), win32::BM_GETCHECK, 0, 0) != win32::BST_CHECKED &&
            !FClick(win32::GetDlgItem(hwndModal, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY))) goto LInput;
        hwndList = win32::GetDlgItem(hwndModal, IDC_PRODUCTION_AVAILABLE_ITEMS);
        iFactoryRow = -1;
        for (i = 0; i < ui::SendMessage(hwndList, win32::LB_GETCOUNT, 0, 0); i++) {
            if (ui::SendMessage(hwndList, win32::LB_GETTEXTLEN, i, 0) >= (LRESULT)sizeof(szItem)) goto LInput;
            ui::SendMessage(hwndList, win32::LB_GETTEXT, i, (LPARAM)szItem);
            if (strlen(szItem) >= 6 && !strcmp(szItem + 6, rgProduction[iProdPass].pszName)) { iFactoryRow = i; break; }
        }
        if (iFactoryRow < 0 || ui::SendMessage(hwndList, win32::LB_GETITEMRECT, ui::SendMessage(hwndList, win32::LB_GETTOPINDEX, 0, 0), (LPARAM)&rcRow) == win32::LB_ERR) goto LInput;
        ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
        if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
        iStage = 49; goto LQueued;
    case 49:
        if (!hwndModal) goto LInput;
        hwndList = win32::GetDlgItem(hwndModal, IDC_PRODUCTION_AVAILABLE_ITEMS);
        if (win32::GetFocus() != hwndList) return;
        /* Keyboard events must be translated after the click changes focus. */
        if (!FPushKey(hwndList, SDL_SCANCODE_HOME, 1) || !FPushKey(hwndList, SDL_SCANCODE_HOME, 0)) goto LInput;
        for (i = 0; i < iFactoryRow; i++) {
            if (!FPushKey(hwndList, SDL_SCANCODE_DOWN, 1) || !FPushKey(hwndList, SDL_SCANCODE_DOWN, 0)) goto LInput;
        }
        iStage = 46; goto LQueued;
    case 46:
        if (hwndModal && cWait == 20) fprintf(stderr, "Production item %s: selected %ld, expected row %d\n", rgProduction[iProdPass].pszName,
            (long)ui::SendMessage(win32::GetDlgItem(hwndModal, IDC_PRODUCTION_AVAILABLE_ITEMS), win32::LB_GETCURSEL, 0, 0), iFactoryRow);
        if (!hwndModal || ui::SendMessage(win32::GetDlgItem(hwndModal, IDC_PRODUCTION_AVAILABLE_ITEMS), win32::LB_GETCURSEL, 0, 0) != iFactoryRow) return;
        SDL_SetModState(iProdPass == 15 ? SDL_KMOD_LCTRL : rgProduction[iProdPass].fShift ? SDL_KMOD_LSHIFT : SDL_KMOD_NONE);
        if (!FClick(win32::GetDlgItem(hwndModal, IDC_PRODUCTION_ADD))) goto LInput;
        cProdAdded += iProdPass == 15 ? 100 : rgProduction[iProdPass].fShift ? 10 : 1;
        break;
    case 47:
        if (!app.state.lpplProdGlob || app.state.lpplProdGlob->iprodMac <= rgProduction[iProdPass].iQueue) return;
        i = rgProduction[iProdPass].iQueue;
        if (app.state.lpplProdGlob->rgprod()[i].iItem != (unsigned)rgProduction[iProdPass].iItem ||
            app.state.lpplProdGlob->rgprod()[i].cItem != (unsigned)cProdAdded ||
            app.state.lpplProdGlob->rgprod()[i].grobj != (unsigned)rgProduction[iProdPass].grobj) return;
        if (cProdAdded < rgProduction[iProdPass].cGoal) { iStage = 46; goto LQueued; }
        SDL_SetModState(SDL_KMOD_NONE);
        if (iProdPass == 1) { iProdPass = 2; cProdAdded = 0; iStage = 45; goto LQueued; }
        if (iProdPass == 5 || iProdPass == 8) { iProdPass++; cProdAdded = 0; iStage = 68; goto LQueued; }
        hwndList = win32::GetDlgItem(hwndModal, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY);
        if (ui::SendMessage(hwndList, win32::BM_GETCHECK, 0, 0) != (rgProduction[iProdPass].fLeftovers ? win32::BST_CHECKED : win32::BST_UNCHECKED) && !FClick(hwndList)) goto LInput;
        break;
    case 48:
        if (!hwndModal || ui::SendMessage(win32::GetDlgItem(hwndModal, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY), win32::BM_GETCHECK, 0, 0) !=
            (rgProduction[iProdPass].fLeftovers ? win32::BST_CHECKED : win32::BST_UNCHECKED)) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        iStage = iProdReturn; goto LQueued;
    case 50:
        if (pszMenu || app.state.sel.grobj != grobjFleet || app.state.sel.id != 3 || app.state.tutor.idtBold != 110) return;
        if (!FClick(app.lpui->rghwndBtn[7])) goto LInput;
        break;
    case 51:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || app.lpui->mdXferDlg != mdXferCargo || !app.state.pxfer || app.state.pxfer[0].id != 3 || app.state.pxfer[1].id != 13) goto LInput;
        for (i = 0; i < app.lpui->crgbtnXfer; i++) {
            if ((app.lpui->rgbtnXfer[i].iVal & 0x7f) == 3 && app.lpui->rgbtnXfer[i].iSide == 0 && !app.lpui->rgbtnXfer[i].fVisible && !(app.lpui->rgbtnXfer[i].bt & 4)) break;
        }
        if (i == app.lpui->crgbtnXfer) goto LInput;
        memcpy(&rcRow, &app.lpui->rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){rcRow.left + 2, (rcRow.top + rcRow.bottom) / 2}; ptTo = (POINT){rcRow.right - 2, ptRow.y};
        if (!FDragPoint(hwndModal, ptRow, ptTo)) goto LInput;
        break;
    case 52:
        if (!hwndModal || !app.state.pxfer || app.state.pxfer[0].fl().rgwtMin[3] != 210) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 53:
        if (hwndModal || app.state.tutor.idtBold != 112) return;
        if (!FFindPlanet(app, "90210") || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) ||
            !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
        break;
    case 54:
        if (app.state.tutor.idtBold != 113) return;
        if (!FClick(app.lpui->rghwndOrderDD[0])) goto LInput;
        break;
    case 55:
        if (!ui::SendMessage(app.lpui->rghwndOrderDD[0], win32::CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(app.lpui->rghwndOrderDD[0], grTaskXfer)) goto LInput;
        break;
    case 56:
        if (app.state.tutor.idtBold != 114 || ui::SendMessage(app.lpui->rghwndOrderDD[0], win32::CB_GETDROPPEDSTATE, 0, 0)) return;
        ptRow = (POINT){(app.lpui->rgrcRef[5].left + app.lpui->rgrcRef[5].right) / 2, (app.lpui->rgrcRef[5].top + app.lpui->rgrcRef[5].bottom) / 2};
        pszMenu = "QuikDrop";
        if (!FClickPoint(app.lpui->hwndPlanet, ptRow, SDL_BUTTON_RIGHT)) goto LInput;
        break;
    case 57:
        if (pszMenu || app.state.tutor.idtBold == 114) return;
        iStage = 42; goto LQueued;
    case 58:
        if (pszMenu || app.state.sel.grobj != grobjFleet || app.state.sel.id != 0) return;
        iStage = 42; goto LQueued;
    case 59:
        if (win32::GetFocus() != app.lpui->hwndShipLB || ui::SendMessage(app.lpui->hwndShipLB, win32::LB_GETCURSEL, 0, 0) != 1) return;
        if (!FPushKey(app.lpui->hwndShipLB, SDL_SCANCODE_DELETE, 1) || !FPushKey(app.lpui->hwndShipLB, SDL_SCANCODE_DELETE, 0)) goto LInput;
        cActions++;
        break;
    case 60:
        if (app.state.tutor.idtBold == 123) return;
        iStage = 42; goto LQueued;
    case 61:
        if (hwndModal) { pszFailure = "unexpected fourth-generation dialog"; goto LFail; }
        if (app.state.game.turn == 3) return;
        if (app.state.game.turn != 4 || !app.state.gd.fTutorial || app.state.tutor.idt != 128) { pszFailure = "fourth generation did not reach page seventeen"; goto LFail; }
        if (!FSaveState(app, "year-2404")) goto LArtifact;
        if (fColonizerProduction) {
            if (!FSaveOrders(app, "year-2404-start-orders.json")) goto LArtifact;
            iStage = 62; goto LQueued;
        }
        fPassed = fRejected && cPage == 17;
        LogState(app, "logistics-partial");
        win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
    case 62:
        if (hwndModal || app.state.game.turn != 4) { pszFailure = "unexpected colonizer-production state or dialog"; goto LFail; }
        idtBefore = app.state.tutor.idtBold; iMsgBefore = app.state.iMsgCur;
        LogState(app, "colonizer-production-instruction");
        if (app.state.tutor.idtBold == 128) {
            if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
            iStage = 64; goto LQueued;
        }
        if (app.state.tutor.idtBold == 130) {
            if (!FFindMapObject(app, grobjFleet, 4, NULL)) goto LInput;
            pszMenu = objTarget.szName;
            if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 65; goto LQueued;
        }
        if (app.state.tutor.idtBold == 131 || app.state.tutor.idtBold == 132) {
            if (ui::SendMessage(app.lpui->hwndShipLB, win32::LB_GETITEMRECT, 1, (LPARAM)&rcRow) == win32::LB_ERR) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            if (!FClickPoint(app.lpui->hwndShipLB, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 66; goto LQueued;
        }
        if (app.state.tutor.idtBold == 133) {
            if (!FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
            iStage = 63; goto LQueued;
        }
        if (app.state.tutor.idtBold == 135) {
            if (!FFindPlanet(app, "Stove Top")) goto LInput;
            pszMenu = objTarget.szName;
            if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 64; goto LQueued;
        }
        if (app.state.tutor.idtBold == 136) {
            iProdPass = 3; cProdAdded = 0; iProdReturn = 64;
            if (!FPushTextKey(app, SDL_SCANCODE_Q, "q")) goto LInput;
            cActions++; iStage = 68; goto LQueued;
        }
        if (app.state.tutor.idtBold == 143) {
            if (!app.state.tutor.fTurnDone || !FSaveOrders(app, "year-2404-orders.json") || !FSaveState(app, "before-generate-2404")) goto LArtifact;
            LogState(app, "generate-ready");
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
            cActions++; iStage = 69; goto LQueued;
        }
        pszFailure = "unimplemented colonizer-production instruction"; goto LFail;
    case 63:
        if (app.state.iMsgCur == iMsgBefore) return;
        if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
        iStage = 64; goto LQueued;
    case 64:
        if (pszMenu || app.state.tutor.idtBold == idtBefore) return;
        iStage = 62; goto LQueued;
    case 65:
        if (pszMenu || app.state.sel.grobj != grobjFleet || app.state.sel.id != 4) return;
        iStage = 62; goto LQueued;
    case 66:
        if (win32::GetFocus() != app.lpui->hwndShipLB || ui::SendMessage(app.lpui->hwndShipLB, win32::LB_GETCURSEL, 0, 0) != 1) return;
        if (!FPushKey(app.lpui->hwndShipLB, SDL_SCANCODE_DELETE, 1) || !FPushKey(app.lpui->hwndShipLB, SDL_SCANCODE_DELETE, 0)) goto LInput;
        cActions++; iStage = 64; goto LQueued;
    case 68:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != ProductionDlg || !app.state.lpplProdGlob) goto LInput;
        /* Add inserts before the selected queue row. */
        hwndList = win32::GetDlgItem(hwndModal, IDC_PRODUCTION_QUEUE);
        if (ui::SendMessage(hwndList, win32::LB_GETITEMRECT, rgProduction[iProdPass].iQueue, (LPARAM)&rcRow) == win32::LB_ERR) goto LInput;
        ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
        if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
        iStage = 70; goto LQueued;
    case 70:
        if (!hwndModal || ui::SendMessage(win32::GetDlgItem(hwndModal, IDC_PRODUCTION_QUEUE), win32::LB_GETCURSEL, 0, 0) != rgProduction[iProdPass].iQueue) return;
        iStage = 45; goto LQueued;
    case 69:
        if (hwndModal) { pszFailure = "unexpected fifth-generation dialog"; goto LFail; }
        if (app.state.game.turn == 4) return;
        if (app.state.game.turn != 5 || !app.state.gd.fTutorial || app.state.tutor.idt != 144) { pszFailure = "fifth generation did not reach page nineteen"; goto LFail; }
        if (!FSaveState(app, "year-2405")) goto LArtifact;
        if (fSecondColonization) {
            if (!FSaveOrders(app, "year-2405-start-orders.json")) goto LArtifact;
            iStage = 71; goto LQueued;
        }
        fPassed = fRejected && cPage == 19;
        LogState(app, "colonizer-production-partial");
        win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
    case 71:
        if (hwndModal || app.state.game.turn != 5) { pszFailure = "unexpected second-colonization state or dialog"; goto LFail; }
        idtBefore = app.state.tutor.idtBold; iMsgBefore = app.state.iMsgCur;
        LogState(app, "second-colonization-instruction");
        if (app.state.tutor.idtBold == 144) {
            if (!FFindMapObject(app, grobjFleet, 2, NULL)) goto LInput;
            pszMenu = objTarget.szName;
            if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 79; goto LQueued;
        }
        if (app.state.tutor.idtBold == 146) {
            ptRow = (POINT){(app.lpui->rgrcRef[3].left + app.lpui->rgrcRef[3].right) / 2, (app.lpui->rgrcRef[3].top + app.lpui->rgrcRef[3].bottom) / 2};
            if (!FClickPoint(app.lpui->hwndPlanet, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 73; goto LQueued;
        }
        if (app.state.tutor.idtBold == 150 || app.state.tutor.idtBold == 153) {
            hwndList = win32::GetDlgItem(app.lpui->hwndTb, IDC_STARS_TOOLBAR);
            i = app.state.tutor.idtBold == 150 ? tbPlanetValueView : tbNormalView;
            if (!ui::SendMessage(hwndList, win32::TB_GETRECT, IDC_STARS_TOOLBUTTON_FIRST + i, (LPARAM)&rcRow)) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 72; goto LQueued;
        }
        if (app.state.tutor.idtBold == 151 || app.state.tutor.idtBold == 155) {
            if (app.state.tutor.idtBold == 151) {
                if ((app.lpui->grbitScan & grbitScanViewMask) != 3 || !FSaveState(app, "habitability")) goto LArtifact;
                LogState(app, "scanner-habitability");
            } else {
                if ((app.lpui->grbitScan & grbitScanViewMask) != 0) goto LInput;
                LogState(app, "scanner-normal");
            }
            if (!FFindPlanet(app, app.state.tutor.idtBold == 151 ? "Shaggy Dog" : "Stove Top") ||
                !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) ||
                !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
            iStage = 72; goto LQueued;
        }
        if (app.state.tutor.idtBold == 152) {
            if (!FClick(app.lpui->rghwndOrderDD[0])) goto LInput;
            iStage = 76; goto LQueued;
        }
        if (app.state.tutor.idtBold == 154) {
            if (app.state.iMsgCur < 1) {
                if (!FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
                iStage = 78; goto LQueued;
            }
            if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
            iStage = 72; goto LQueued;
        }
        if (app.state.tutor.idtBold == 156) {
            if (IMsgNext(app, 0) != imsgNone) {
                if (!FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
                iStage = 78; goto LQueued;
            }
            if (!FFindPlanet(app, "90210")) goto LInput;
            pszMenu = objTarget.szName;
            if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 72; goto LQueued;
        }
        if (app.state.tutor.idtBold == 158) {
            if (app.state.sel.grobj != grobjFleet || app.state.sel.id != 0) {
                if (!FFindMapObject(app, grobjFleet, 0, NULL)) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 79; goto LQueued;
            }
            if (IMsgNext(app, 0) != imsgNone) {
                if (!FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
                iStage = 78; goto LQueued;
            }
            if (ui::SendMessage(app.lpui->hwndShipLB, win32::LB_GETITEMRECT, 1, (LPARAM)&rcRow) == win32::LB_ERR) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            if (!FClickPoint(app.lpui->hwndShipLB, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 80; goto LQueued;
        }
        if (app.state.tutor.idtBold == 159) {
            if (!app.state.tutor.fTurnDone || !FSaveOrders(app, "year-2405-orders.json") || !FSaveState(app, "before-generate-2405")) goto LArtifact;
            LogState(app, "generate-ready");
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
            cActions++; iStage = 81; goto LQueued;
        }
        pszFailure = "unimplemented second-colonization instruction"; goto LFail;
    case 72:
        if (pszMenu || app.state.tutor.idtBold == idtBefore) return;
        iStage = 71; goto LQueued;
    case 73:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || app.lpui->mdXferDlg != mdXferCargo || !app.state.pxfer || app.state.pxfer[0].id != 2 || app.state.pxfer[1].id != 13) goto LInput;
        for (i = 0; i < app.lpui->crgbtnXfer; i++) {
            if ((app.lpui->rgbtnXfer[i].iVal & 0x7f) == 3 && app.lpui->rgbtnXfer[i].iSide == 0 && !app.lpui->rgbtnXfer[i].fVisible && !(app.lpui->rgbtnXfer[i].bt & 4)) break;
        }
        if (i == app.lpui->crgbtnXfer) goto LInput;
        memcpy(&rcRow, &app.lpui->rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){rcRow.left + 2, (rcRow.top + rcRow.bottom) / 2}; ptTo = (POINT){rcRow.right - 2, ptRow.y};
        if (!FDragPoint(hwndModal, ptRow, ptTo)) goto LInput;
        break;
    case 74:
        if (!hwndModal || !app.state.pxfer || app.state.pxfer[0].fl().rgwtMin[3] != 25) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 75:
        if (hwndModal || app.state.tutor.idtBold == 146 || app.state.tutor.idtBold == 148) return;
        if (!FSaveOrders(app, "year-2405-cargo.json")) goto LArtifact;
        iStage = 71; goto LQueued;
    case 76:
        if (!ui::SendMessage(app.lpui->rghwndOrderDD[0], win32::CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(app.lpui->rghwndOrderDD[0], grTaskColonize)) goto LInput;
        iStage = 72; goto LQueued;
    case 78:
        if (app.state.iMsgCur == iMsgBefore) return;
        iStage = 71; goto LQueued;
    case 79:
        if (pszMenu) return;
        iStage = 71; goto LQueued;
    case 80:
        if (win32::GetFocus() != app.lpui->hwndShipLB || ui::SendMessage(app.lpui->hwndShipLB, win32::LB_GETCURSEL, 0, 0) != 1) return;
        if (!FPushKey(app.lpui->hwndShipLB, SDL_SCANCODE_DELETE, 1) || !FPushKey(app.lpui->hwndShipLB, SDL_SCANCODE_DELETE, 0)) goto LInput;
        cActions++; iStage = 72; goto LQueued;
    case 81:
        if (hwndModal) { pszFailure = "unexpected sixth-generation dialog"; goto LFail; }
        if (app.state.game.turn == 5) return;
        if (app.state.game.turn != 6 || !app.state.gd.fTutorial || app.state.tutor.idt != 160) { pszFailure = "sixth generation did not reach page twenty-one"; goto LFail; }
        if (!FSaveState(app, "year-2406")) goto LArtifact;
        if (fTransportCycle) {
            if (!FSaveOrders(app, "year-2406-start-orders.json")) goto LArtifact;
            iStage = 82; goto LQueued;
        }
        fPassed = fRejected && cPage == 21;
        LogState(app, "second-colonization-partial");
        win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
    case 82:
        if (hwndModal || app.state.game.turn != 6) { pszFailure = "unexpected transport-cycle state or dialog"; goto LFail; }
        idtBefore = app.state.tutor.idtBold;
        LogState(app, "transport-cycle-instruction");
        if (app.state.tutor.idtBold == 160) {
            if (!FFindMapObject(app, grobjFleet, 3, NULL)) goto LInput;
            pszMenu = objTarget.szName;
            if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 86; goto LQueued;
        }
        if (app.state.tutor.idtBold == 163) {
            if (!FFindPlanet(app, "Prune") || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) ||
                !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
            iStage = 83; goto LQueued;
        }
        if (app.state.tutor.idtBold == 164) {
            if (!FClick(app.lpui->rghwndOrderDD[0])) goto LInput;
            iStage = 84; goto LQueued;
        }
        if (app.state.tutor.idtBold == 165 || app.state.tutor.idtBold == 170) {
            if (app.state.tutor.idtBold == 170 && !FSaveOrders(app, "year-2406-copied-orders.json")) goto LArtifact;
            ptRow = (POINT){(app.lpui->rgrcRef[5].left + app.lpui->rgrcRef[5].right) / 2, (app.lpui->rgrcRef[5].top + app.lpui->rgrcRef[5].bottom) / 2};
            pszMenu = app.state.tutor.idtBold == 165 ? "QuikLoad" : "QuikDrop";
            if (!FClickPoint(app.lpui->hwndPlanet, ptRow, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 83; goto LQueued;
        }
        if (app.state.tutor.idtBold == 166) {
            if (ui::SendMessage(app.lpui->hwndShipLB, win32::LB_GETITEMRECT, 1, (LPARAM)&rcRow) == win32::LB_ERR) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            if (!FClickPoint(app.lpui->hwndShipLB, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 87; goto LQueued;
        }
        if (app.state.tutor.idtBold == 171) {
            if (!FClick(app.lpui->hwndRepCB)) goto LInput;
            iStage = 83; goto LQueued;
        }
        if (app.state.tutor.idtBold == 173) {
            if (!FFindPlanet(app, "Stove Top")) goto LInput;
            pszMenu = objTarget.szName;
            if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 83; goto LQueued;
        }
        if (app.state.tutor.idtBold == 174) {
            iProdPass = 3; cProdAdded = 0; iProdReturn = 83;
            if (!FPushTextKey(app, SDL_SCANCODE_Q, "q")) goto LInput;
            cActions++; iStage = 68; goto LQueued;
        }
        if (app.state.tutor.idtBold == 175) {
            if (!app.state.tutor.fTurnDone || !FSaveOrders(app, "year-2406-orders.json") || !FSaveState(app, "before-generate-2406")) goto LArtifact;
            LogState(app, "generate-ready");
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
            cActions++; iStage = 90; goto LQueued;
        }
        pszFailure = "unimplemented transport-cycle instruction"; goto LFail;
    case 83:
        if (pszMenu || app.state.tutor.idtBold == idtBefore) return;
        iStage = 82; goto LQueued;
    case 84:
        if (!ui::SendMessage(app.lpui->rghwndOrderDD[0], win32::CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(app.lpui->rghwndOrderDD[0], grTaskXfer)) goto LInput;
        iStage = 83; goto LQueued;
    case 86:
        if (pszMenu || app.state.sel.grobj != grobjFleet || app.state.sel.id != 3) return;
        iStage = 82; goto LQueued;
    case 87:
        if (ui::SendMessage(app.lpui->hwndShipLB, win32::LB_GETCURSEL, 0, 0) != 1 || app.state.sel.iwpAct != 1) return;
        if (!FFindPlanet(app, "Stove Top") || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) ||
            !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
        iStage = 83; goto LQueued;
    case 90:
        if (hwndModal) { pszFailure = "unexpected seventh-generation dialog"; goto LFail; }
        if (app.state.game.turn == 6) return;
        if (app.state.game.turn != 7 || !app.state.gd.fTutorial || app.state.tutor.idt != 176) { pszFailure = "seventh generation did not reach page twenty-three"; goto LFail; }
        if (!FSaveState(app, "year-2407")) goto LArtifact;
        if (fExpansion) {
            if (!FSaveOrders(app, "year-2407-start-orders.json")) goto LArtifact;
            iStage = 91; goto LQueued;
        }
        fPassed = fRejected && cPage == 23;
        LogState(app, "transport-cycle-partial");
        win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
    case 91:
        if (hwndModal || app.state.game.turn != 7) { pszFailure = "unexpected expansion state or dialog"; goto LFail; }
        idtBefore = app.state.tutor.idtBold; iMsgBefore = app.state.iMsgCur;
        LogState(app, "expansion-instruction");
        if (app.state.tutor.idtBold == 176) {
            if (app.state.sel.grobj != grobjFleet || app.state.sel.id != 6) {
                if (!FFindMapObject(app, grobjFleet, 6, NULL)) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 93; goto LQueued;
            }
            if (!FClick(app.lpui->rghwndBtn[7])) goto LInput;
            iStage = 94; goto LQueued;
        }
        if (app.state.tutor.idtBold == 177 || app.state.tutor.idtBold == 182) {
            hwndList = win32::GetDlgItem(app.lpui->hwndTb, IDC_STARS_TOOLBAR);
            i = app.state.tutor.idtBold == 177 ? tbPlanetValueView : tbNormalView;
            if (!ui::SendMessage(hwndList, win32::TB_GETRECT, IDC_STARS_TOOLBUTTON_FIRST + i, (LPARAM)&rcRow)) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 92; goto LQueued;
        }
        if (app.state.tutor.idtBold == 179) {
            if ((app.lpui->grbitScan & grbitScanViewMask) != 3 || !FSaveState(app, "red-storm-view")) goto LArtifact;
            LogState(app, "expansion-habitability");
            if (!FFindPlanet(app, "Red Storm") || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) ||
                !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
            iStage = 98; goto LQueued;
        }
        if (app.state.tutor.idtBold == 181 || app.state.tutor.idtBold == 188 || app.state.tutor.idtBold == 196) {
            i = app.state.tutor.idtBold == 188 ? 16 : 13;
            if (app.state.sel.grobj != grobjPlanet || app.state.sel.id != i) {
                if (!FFindPlanet(app, i == 16 ? "90210" : "Stove Top")) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 93; goto LQueued;
            }
            iProdPass = app.state.tutor.idtBold == 181 ? 4 : app.state.tutor.idtBold == 188 ? 5 : 7;
            cProdAdded = 0; iProdReturn = 92;
            if (!FPushTextKey(app, SDL_SCANCODE_Q, "q")) goto LInput;
            cActions++; iStage = 68; goto LQueued;
        }
        if (app.state.tutor.idtBold == 184 || app.state.tutor.idtBold == 187 || app.state.tutor.idtBold == 190) {
            if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
            iStage = 101; goto LQueued;
        }
        if (app.state.tutor.idtBold == 186) {
            ptRow = (POINT){app.lpui->rcMsgTitle.left + (app.lpui->rcMsgTitle.bottom - app.lpui->rcMsgTitle.top) / 2,
                            (app.lpui->rcMsgTitle.top + app.lpui->rcMsgTitle.bottom) / 2};
            if (!FClickPoint(app.lpui->hwndMessage, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 92; goto LQueued;
        }
        if (app.state.tutor.idtBold == 192) {
            if ((app.lpui->grbitScan & grbitScanViewMask) != 0 || !FFindMapObject(app, grobjFleet, 512, NULL) ||
                !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT)) goto LInput;
            LogState(app, "expansion-normal");
            iStage = 102; goto LQueued;
        }
        if (app.state.tutor.idtBold == 198) {
            if (!app.state.tutor.fTurnDone || !FSaveOrders(app, "year-2407-orders.json") || !FSaveState(app, "before-generate-2407")) goto LArtifact;
            LogState(app, "generate-ready");
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
            cActions++; iStage = 105; goto LQueued;
        }
        pszFailure = "unimplemented expansion instruction"; goto LFail;
    case 92:
        if (pszMenu || app.state.tutor.idtBold == idtBefore) return;
        iStage = 91; goto LQueued;
    case 93:
        if (pszMenu) return;
        iStage = 91; goto LQueued;
    case 94:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || app.lpui->mdXferDlg != mdXferCargo || !app.state.pxfer || app.state.pxfer[0].id != 6 || app.state.pxfer[1].id != 13) goto LInput;
        for (i = 0; i < app.lpui->crgbtnXfer; i++) {
            if ((app.lpui->rgbtnXfer[i].iVal & 0x7f) == 3 && app.lpui->rgbtnXfer[i].iSide == 0 && !app.lpui->rgbtnXfer[i].fVisible && !(app.lpui->rgbtnXfer[i].bt & 4)) break;
        }
        if (i == app.lpui->crgbtnXfer) goto LInput;
        memcpy(&rcRow, &app.lpui->rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){rcRow.left + 2, (rcRow.top + rcRow.bottom) / 2}; ptTo = (POINT){rcRow.right - 2, ptRow.y};
        if (!FDragPoint(hwndModal, ptRow, ptTo)) goto LInput;
        break;
    case 95:
        if (!hwndModal || !app.state.pxfer || app.state.pxfer[0].fl().rgwtMin[3] != 25) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 96:
        if (hwndModal || app.state.tutor.idtBold == 176) return;
        if (!FSaveOrders(app, "year-2407-cargo.json")) goto LArtifact;
        iStage = 91; goto LQueued;
    case 98:
        if (app.state.sel.grobj != grobjFleet || app.state.sel.id != 6 || app.state.sel.fl.cord != 2) return;
        if (!FClick(app.lpui->rghwndOrderDD[0])) goto LInput;
        break;
    case 99:
        if (!ui::SendMessage(app.lpui->rghwndOrderDD[0], win32::CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(app.lpui->rghwndOrderDD[0], grTaskColonize)) goto LInput;
        iStage = 92; goto LQueued;
    case 101:
        if (app.state.iMsgCur == iMsgBefore) return;
        iStage = 91; goto LQueued;
    case 102:
        if (app.state.sel.scan.grobj != grobjFleet || app.state.sel.scan.ifl == iflNone || app.state.rglpfl[app.state.sel.scan.ifl]->id != 512) return;
        if (!FSaveState(app, "enemy-summary")) goto LArtifact;
        ptRow = (POINT){30, 2 * app.state.dyArial8 + 25};
        if (!FMousePoint(app.lpui->hwndMine, ptRow, SDL_BUTTON_RIGHT, 1)) goto LInput;
        cActions++; break;
    case 103:
        if (!win32::IsWindow(app.lpui->hwndPopup)) return;
        if (!win32::IsWindowVisible(app.lpui->hwndPopup) || win32::GetCapture() != app.lpui->hwndPopup || !win32::GetClientRect(app.lpui->hwndPopup, &rcRow)) goto LInput;
        if (!FSaveState(app, "enemy-design-popup") || !FSaveEnemyDesign(app)) goto LArtifact;
        LogState(app, "enemy-design-popup");
        ptRow = (POINT){rcRow.right / 2, rcRow.bottom / 2};
        if (!FMousePoint(app.lpui->hwndPopup, ptRow, SDL_BUTTON_RIGHT, 0)) goto LInput;
        break;
    case 104:
        if (win32::IsWindow(app.lpui->hwndPopup)) return;
        iStage = 91; goto LQueued;
    case 105:
        if (hwndModal) { pszFailure = "unexpected eighth-generation dialog"; goto LFail; }
        if (app.state.game.turn == 7) return;
        if (app.state.game.turn != 8 || !app.state.gd.fTutorial || app.state.tutor.idt != 200) { pszFailure = "eighth generation did not reach page twenty-six"; goto LFail; }
        if (!FSaveState(app, "year-2408")) goto LArtifact;
        if (fFleetSplit) {
            if (!FSaveOrders(app, "year-2408-start-orders.json")) goto LArtifact;
            iStage = 106; goto LQueued;
        }
        fPassed = fRejected && cPage == 26;
        LogState(app, "expansion-partial");
        win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
    case 106:
        if (hwndModal || app.state.game.turn != 8) { pszFailure = "unexpected fleet splitting state or dialog"; goto LFail; }
        if (app.state.tutor.idt == 208) {
            if (!FSaveOrders(app, "year-2408-split-orders.json") || !FSaveState(app, "fleet-split")) goto LArtifact;
            if (fResearchCycle) { iStage = 118; goto LQueued; }
            fPassed = fRejected && cPage == 27;
            LogState(app, "fleet-split-partial");
            win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
        }
        idtBefore = app.state.tutor.idtBold;
        LogState(app, "fleet-split-instruction");
        if (app.state.tutor.idtBold == 200 || app.state.tutor.idtBold == 202) {
            if (app.state.sel.grobj != grobjFleet || app.state.sel.id != 7) {
                if (!FFindMapObject(app, grobjFleet, 7, NULL)) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 107; goto LQueued;
            }
            if (!FClick(app.lpui->rghwndBtn[8])) goto LInput;
            iStage = 108; goto LQueued;
        }
        if (app.state.tutor.idtBold == 205) {
            lpfl = LpflFromId(app, 7);
            if (!lpfl || app.state.sel.grobj != grobjFleet || app.state.sel.id != 7) goto LInput;
            if (lpfl->rgwtMin[3] != 50) {
                if (!FClick(app.lpui->rghwndBtn[7])) goto LInput;
                iStage = 111; goto LQueued;
            }
            if (lpfl->cord == 1) {
                if (!FFindPlanet(app, "Slime") || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) ||
                    !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
                iStage = 114; goto LQueued;
            }
            if (lpfl->cord != 2 || lpfl->lpplord->rgord()[1].id != 8) goto LInput;
            if (!FClick(app.lpui->rghwndOrderDD[0])) goto LInput;
            iStage = 115; goto LQueued;
        }
        if (app.state.tutor.idtBold == 206) {
            if (!FSaveOrders(app, "year-2408-before-split-all.json") || !FClick(app.lpui->rghwndBtn[9])) goto LArtifact;
            iStage = 117; goto LQueued;
        }
        pszFailure = "unimplemented fleet splitting instruction"; goto LFail;
    case 107:
        if (pszMenu) return;
        iStage = 106; goto LQueued;
    case 108:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || app.lpui->mdXferDlg != mdXferShips || !app.state.pxfer || app.state.pxfer[0].id != 7 || app.state.pxfer[1].id != 9) goto LInput;
        if (!FSaveState(app, "ship-transfer")) goto LArtifact;
        for (i = 0; i < app.lpui->crgbtnXfer; i++) {
            if (app.lpui->rgbtnXfer[i].iVal == 2 && app.lpui->rgbtnXfer[i].iSide == 1 && app.lpui->rgbtnXfer[i].fVisible && !(app.lpui->rgbtnXfer[i].bt & 4)) break;
        }
        if (i == app.lpui->crgbtnXfer) goto LInput;
        memcpy(&rcRow, &app.lpui->rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
        if (!FClickPoint(hwndModal, ptRow, SDL_BUTTON_LEFT)) goto LInput;
        break;
    case 109:
        if (!hwndModal || !app.state.pxfer || app.state.pxfer[0].fl().rgcsh[2] != 2 || app.state.pxfer[1].fl().rgcsh[2] != 1) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 110:
        if (hwndModal) return;
        if (!FSaveOrders(app, "year-2408-after-transfer.json")) goto LArtifact;
        iStage = 106; goto LQueued;
    case 111:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || app.lpui->mdXferDlg != mdXferCargo || !app.state.pxfer || app.state.pxfer[0].id != 7 || app.state.pxfer[1].id != 13) goto LInput;
        for (i = 0; i < app.lpui->crgbtnXfer; i++) {
            if ((app.lpui->rgbtnXfer[i].iVal & 0x7f) == 3 && app.lpui->rgbtnXfer[i].iSide == 0 && !app.lpui->rgbtnXfer[i].fVisible && !(app.lpui->rgbtnXfer[i].bt & 4)) break;
        }
        if (i == app.lpui->crgbtnXfer) goto LInput;
        memcpy(&rcRow, &app.lpui->rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){rcRow.left + 2, (rcRow.top + rcRow.bottom) / 2}; ptTo = (POINT){rcRow.right - 2, ptRow.y};
        if (!FDragPoint(hwndModal, ptRow, ptTo)) goto LInput;
        break;
    case 112:
        if (!hwndModal || !app.state.pxfer || app.state.pxfer[0].fl().rgwtMin[3] != 50) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 113:
        if (hwndModal) return;
        if (!FSaveOrders(app, "year-2408-split-cargo.json")) goto LArtifact;
        iStage = 106; goto LQueued;
    case 114:
        if (app.state.sel.grobj != grobjFleet || app.state.sel.id != 7 || app.state.sel.fl.cord != 2) return;
        iStage = 106; goto LQueued;
    case 115:
        if (!ui::SendMessage(app.lpui->rghwndOrderDD[0], win32::CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(app.lpui->rghwndOrderDD[0], grTaskColonize)) goto LInput;
        break;
    case 116:
        if (ui::SendMessage(app.lpui->rghwndOrderDD[0], win32::CB_GETDROPPEDSTATE, 0, 0) || app.state.tutor.idtBold == idtBefore) return;
        iStage = 106; goto LQueued;
    case 117:
        if (app.state.tutor.idt != 208 || app.state.rgplr[app.state.idPlayer].cFleet != 11) return;
        iStage = 106; goto LQueued;
    case 118:
        if (app.state.game.turn != 8 || (hwndModal && hwndModal->lpfnDlg != ResearchDlg)) {
            pszFailure = "unexpected research cycle state or dialog"; goto LFail;
        }
        idtBefore = app.state.tutor.idtBold; iMsgBefore = app.state.iMsgCur;
        LogState(app, "research-cycle-instruction");
        if (app.state.tutor.idtBold == 209 || app.state.tutor.idtBold == 212) {
            i = app.state.tutor.idtBold == 209 ? 7 : 8;
            if (app.state.sel.grobj != grobjFleet || app.state.sel.id != i) {
                if (!FFindMapObject(app, grobjFleet, i, NULL)) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 119; goto LQueued;
            }
            if (i == 8) return;
            lpfl = LpflFromId(app, 7);
            if (!lpfl || lpfl->cord != 2 || !FFindPlanet(app, "Sea Squared")) goto LInput;
            ptLogical = lpfl->lpplord->rgord()[1].pt;
            LogicalToScan(app, &ptLogical);
            ptRow = (POINT){ptLogical.x, ptLogical.y};
            if (!FDragPoint(app.lpui->hwndScanner, ptRow, objTarget.ptClient)) goto LInput;
            iStage = 120; goto LQueued;
        }
        if (app.state.tutor.idtBold == 214) {
            if (!FFindPlanet(app, "Hiho") || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) ||
                !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
            iStage = 121; goto LQueued;
        }
        if (app.state.tutor.idtBold == 216 || app.state.tutor.idtBold == 217) {
            if (app.state.tutor.idtBold == 216 && IdmGetMessageN(app, app.state.iMsgCur) == idmHasUnloaded) {
                ptRow = (POINT){app.lpui->rcMsgTitle.left + (app.lpui->rcMsgTitle.bottom - app.lpui->rcMsgTitle.top) / 2,
                                (app.lpui->rcMsgTitle.top + app.lpui->rcMsgTitle.bottom) / 2};
                if (!FClickPoint(app.lpui->hwndMessage, ptRow, SDL_BUTTON_LEFT)) goto LInput;
                iStage = 123; goto LQueued;
            }
            if (app.state.tutor.idtBold == 216 || IMsgNext(app, 0) != imsgNone) {
                if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
                iStage = 122; goto LQueued;
            }
            if (!FFindPlanet(app, "Wallaby") || !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 124; goto LQueued;
        }
        if (app.state.tutor.idtBold == 219) {
            if (!win32::GetClientRect(app.lpui->hwndMine, &rcRow)) goto LInput;
            i = ((rcRow.bottom - (2 * app.state.dyArial8 - 4) - 4 * app.state.dyArial8 - 2) / 6 + 1) & ~1;
            ptRow = (POINT){30, 4 * app.state.dyArial8 - 4 + 2 * i + i / 2};
            if (!FMousePoint(app.lpui->hwndMine, ptRow, SDL_BUTTON_LEFT, 1)) goto LInput;
            cActions++; iStage = 125; goto LQueued;
        }
        if (app.state.tutor.idtBold == 221) {
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F5, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F5, 0)) goto LInput;
            cActions++; iStage = 127; goto LQueued;
        }
        if (app.state.tutor.idtBold == 226) {
            if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg) return;
            for (i = 0; i < app.lpui->cFutureTech; i++) {
                if (app.lpui->rghsFutureTech[i].grhst == hstTerra && app.lpui->rghsFutureTech[i].iItem == iterraRadiationTerraform7) break;
            }
            if (i == app.lpui->cFutureTech) { pszFailure = "Radiation Terraform 7 benefit is absent"; goto LFail; }
            ptRow = (POINT){app.lpui->dxResLeft / 2, app.lpui->yTopFutureTech + i * app.state.dyArial8 + app.state.dyArial8 / 2};
            if (!FMousePoint(hwndModal, ptRow, SDL_BUTTON_LEFT, 1)) goto LInput;
            cActions++; iStage = 128; goto LQueued;
        }
        if (app.state.tutor.idtBold == 228) {
            if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg) goto LInput;
            if (app.lpui->pctResGlob == 30) {
                if (!FClick(win32::GetDlgItem(hwndModal, IDCANCEL))) goto LInput;
                iStage = 131; goto LQueued;
            }
            pctResearchBefore = app.lpui->pctResGlob;
            rcRow = app.lpui->pctResGlob < 30 ? app.lpui->rcSpinTop : app.lpui->rcSpinBot;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            if (!FClickPoint(hwndModal, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 130; goto LQueued;
        }
        if (app.state.tutor.idtBold == 230) {
            if (!app.state.tutor.fTurnDone || !FSaveOrders(app, "year-2408-orders.json") || !FSaveState(app, "before-generate-2408")) goto LArtifact;
            LogState(app, "generate-ready");
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
            cActions++; iStage = 132; goto LQueued;
        }
        pszFailure = "unimplemented research cycle instruction"; goto LFail;
    case 119:
        if (pszMenu) return;
        iStage = 118; goto LQueued;
    case 120:
        lpfl = LpflFromId(app, 7);
        if (!lpfl || lpfl->cord != 2 || lpfl->lpplord->rgord()[1].id != 17 || lpfl->lpplord->rgord()[1].grTask != grTaskColonize) return;
        if (!FSaveOrders(app, "year-2408-drag-orders.json") || !FSaveState(app, "waypoint-drag")) goto LArtifact;
        LogState(app, "waypoint-dragged");
        iStage = 118; goto LQueued;
    case 121:
        if (app.state.tutor.idtBold == idtBefore) return;
        if (!FSaveOrders(app, "year-2408-scout-orders.json")) goto LArtifact;
        iStage = 118; goto LQueued;
    case 122:
        if (app.state.iMsgCur == iMsgBefore) return;
        iStage = 118; goto LQueued;
    case 123:
        if (!(app.state.bitfMsgFiltered[idmHasUnloaded >> 3] & (1 << (idmHasUnloaded & 7)))) return;
        iStage = 118; goto LQueued;
    case 124:
        if (app.state.sel.scan.grobj != grobjPlanet || app.state.sel.scan.idpl != 5) return;
        iStage = 118; goto LQueued;
    case 125:
        if (!win32::IsWindow(app.lpui->hwndPopup)) return;
        if (!FSaveTutorialPopup(app, "wallaby-radiation.json", 0) || !FSaveState(app, "wallaby-radiation")) goto LArtifact;
        LogState(app, "environment-popup");
        if (!win32::GetClientRect(app.lpui->hwndPopup, &rcRow)) goto LInput;
        ptRow = (POINT){rcRow.right / 2, rcRow.bottom / 2};
        if (!FMousePoint(app.lpui->hwndPopup, ptRow, SDL_BUTTON_LEFT, 0)) goto LInput;
        break;
    case 126:
        if (win32::IsWindow(app.lpui->hwndPopup) || app.state.tutor.idtBold == idtBefore) return;
        iStage = 118; goto LQueued;
    case 127:
        if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg || app.state.tutor.idtBold == idtBefore) return;
        if (!FSaveState(app, "research-benefits")) goto LArtifact;
        iStage = 118; goto LQueued;
    case 128:
        if (!win32::IsWindow(app.lpui->hwndPopup)) return;
        if (!FSaveTutorialPopup(app, "radiation-benefit.json", 1) || !FSaveState(app, "radiation-benefit")) goto LArtifact;
        LogState(app, "research-benefit-popup");
        if (!win32::GetClientRect(app.lpui->hwndPopup, &rcRow)) goto LInput;
        ptRow = (POINT){rcRow.right / 2, rcRow.bottom / 2};
        if (!FMousePoint(app.lpui->hwndPopup, ptRow, SDL_BUTTON_LEFT, 0)) goto LInput;
        break;
    case 129:
        if (win32::IsWindow(app.lpui->hwndPopup) || app.state.tutor.idtBold == idtBefore) return;
        iStage = 118; goto LQueued;
    case 130:
        if (!hwndModal || app.lpui->pctResGlob == pctResearchBefore) return;
        iStage = 118; goto LQueued;
    case 131:
        if (hwndModal || app.lpui->pctResGlob != -1 || app.state.tutor.idtBold == idtBefore) return;
        iStage = 118; goto LQueued;
    case 132:
        if (hwndModal) { pszFailure = "unexpected ninth-generation dialog"; goto LFail; }
        if (app.state.game.turn == 8) return;
        if (app.state.game.turn != 9 || !app.state.gd.fTutorial || app.state.tutor.idt != 232) { pszFailure = "ninth generation did not reach page thirty"; goto LFail; }
        if (!FSaveState(app, "year-2409")) goto LArtifact;
        if (fOxygenColony) {
            if (!FSaveOrders(app, "year-2409-start-orders.json")) goto LArtifact;
            iStage = 133; goto LQueued;
        }
        fPassed = fRejected && cPage == 30;
        LogState(app, "research-cycle-partial");
        win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
    case 133:
        if (app.state.game.turn != 9 || (hwndModal && hwndModal->lpfnDlg != ResearchDlg)) {
            pszFailure = "unexpected Oxygen colonization state or dialog"; goto LFail;
        }
        idtBefore = app.state.tutor.idtBold; iMsgBefore = app.state.iMsgCur;
        LogState(app, "oxygen-colonization-instruction");
        if (app.state.tutor.idtBold == 232) {
            if (app.lpui->mdMsgObj != mdMsgObjResearch || !FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
            iStage = 134; goto LQueued;
        }
        if (app.state.tutor.idtBold == 236) {
            if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg || !FClick(win32::GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD))) goto LInput;
            iStage = 135; goto LQueued;
        }
        if (app.state.tutor.idtBold == 238) {
            if (IdmGetMessageN(app, app.state.iMsgCur) != idmHasLoadedMiningRobotsWorking) {
                if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
                iStage = 138; goto LQueued;
            }
            ptRow = (POINT){app.lpui->rcMsgTitle.left + (app.lpui->rcMsgTitle.bottom - app.lpui->rcMsgTitle.top) / 2,
                            (app.lpui->rcMsgTitle.top + app.lpui->rcMsgTitle.bottom) / 2};
            if (!FClickPoint(app.lpui->hwndMessage, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 140; goto LQueued;
        }
        if (app.state.tutor.idtBold == 240) {
            if (app.lpui->mdMsgObj != mdMsgObjPlanet || app.lpui->idMsgObj != 2) {
                if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
                iStage = 138; goto LQueued;
            }
            if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
            iStage = 141; goto LQueued;
        }
        if (app.state.tutor.idtBold == 241 || app.state.tutor.idtBold == 245) {
            i = app.state.tutor.idtBold == 241 ? 9 : 0;
            if (app.state.sel.grobj != grobjFleet || app.state.sel.id != i) {
                if (!FFindMapObject(app, grobjFleet, i, NULL)) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 139; goto LQueued;
            }
            if (i == 9) return;
            lpfl = LpflFromId(app, 0);
            if (!lpfl || lpfl->cord != 2 || !FFindPlanet(app, "Mozart")) goto LInput;
            ptLogical = lpfl->lpplord->rgord()[1].pt;
            LogicalToScan(app, &ptLogical);
            ptRow = (POINT){ptLogical.x, ptLogical.y};
            if (!FDragPoint(app.lpui->hwndScanner, ptRow, objTarget.ptClient)) goto LInput;
            iStage = 148; goto LQueued;
        }
        if (app.state.tutor.idtBold == 242) {
            if (app.state.sel.grobj != grobjFleet || app.state.sel.id != 9 || !FClick(app.lpui->rghwndBtn[7])) goto LInput;
            iStage = 142; goto LQueued;
        }
        if (app.state.tutor.idtBold == 243) {
            lpfl = LpflFromId(app, 9);
            if (!lpfl || lpfl->cord != 1 || !FFindPlanet(app, "Oxygen") || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) ||
                !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
            iStage = 145; goto LQueued;
        }
        if (app.state.tutor.idtBold == 247) {
            if (!app.state.tutor.fTurnDone || !FSaveOrders(app, "year-2409-orders.json") || !FSaveState(app, "before-generate-2409")) goto LArtifact;
            LogState(app, "generate-ready");
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
            cActions++; iStage = 149; goto LQueued;
        }
        pszFailure = "unimplemented Oxygen colonization instruction"; goto LFail;
    case 134:
        if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg || app.state.tutor.idtBold == idtBefore) return;
        iStage = 133; goto LQueued;
    case 135:
        if (!hwndModal) return;
        hwndList = win32::GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD);
        if (!ui::SendMessage(hwndList, win32::CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(hwndList, Construction + 1)) goto LInput;
        break;
    case 136:
        if (!hwndModal) return;
        hwndList = win32::GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD);
        if (ui::SendMessage(hwndList, win32::CB_GETDROPPEDSTATE, 0, 0) || ui::SendMessage(hwndList, win32::CB_GETCURSEL, 0, 0) != Construction + 1) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDCANCEL))) goto LInput;
        break;
    case 137:
        if (hwndModal || app.lpui->pctResGlob != -1 || app.state.tutor.idtBold == idtBefore) return;
        if (!FSaveOrders(app, "year-2409-research.json")) goto LArtifact;
        iStage = 133; goto LQueued;
    case 138:
        if (app.state.iMsgCur == iMsgBefore) return;
        iStage = 133; goto LQueued;
    case 139:
        if (pszMenu) return;
        iStage = 133; goto LQueued;
    case 140:
        if (!(app.state.bitfMsgFiltered[idmHasLoadedMiningRobotsWorking >> 3] & (1 << (idmHasLoadedMiningRobotsWorking & 7)))) return;
        iStage = 133; goto LQueued;
    case 141:
        if (app.state.sel.scan.grobj != grobjPlanet || app.state.sel.scan.idpl != 2) return;
        if (!FSaveState(app, "oxygen-summary")) goto LArtifact;
        LogState(app, "oxygen-goto");
        iStage = 133; goto LQueued;
    case 142:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || app.lpui->mdXferDlg != mdXferCargo || !app.state.pxfer || app.state.pxfer[0].id != 9 || app.state.pxfer[1].id != 13) goto LInput;
        for (i = 0; i < app.lpui->crgbtnXfer; i++) {
            if ((app.lpui->rgbtnXfer[i].iVal & 0x7f) == 3 && app.lpui->rgbtnXfer[i].iSide == 0 && !app.lpui->rgbtnXfer[i].fVisible && !(app.lpui->rgbtnXfer[i].bt & 4)) break;
        }
        if (i == app.lpui->crgbtnXfer) goto LInput;
        memcpy(&rcRow, &app.lpui->rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){rcRow.left + 2, (rcRow.top + rcRow.bottom) / 2}; ptTo = (POINT){rcRow.right - 2, ptRow.y};
        if (!FDragPoint(hwndModal, ptRow, ptTo)) goto LInput;
        break;
    case 143:
        if (!hwndModal || !app.state.pxfer || app.state.pxfer[0].fl().rgwtMin[3] != 25) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 144:
        if (hwndModal) return;
        if (!FSaveOrders(app, "year-2409-cargo.json")) goto LArtifact;
        iStage = 133; goto LQueued;
    case 145:
        if (app.state.sel.grobj != grobjFleet || app.state.sel.id != 9 || app.state.sel.fl.cord != 2) return;
        if (!FClick(app.lpui->rghwndOrderDD[0])) goto LInput;
        break;
    case 146:
        if (!ui::SendMessage(app.lpui->rghwndOrderDD[0], win32::CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(app.lpui->rghwndOrderDD[0], grTaskColonize)) goto LInput;
        break;
    case 147:
        if (ui::SendMessage(app.lpui->rghwndOrderDD[0], win32::CB_GETDROPPEDSTATE, 0, 0) || app.state.tutor.idtBold == idtBefore) return;
        iStage = 133; goto LQueued;
    case 148:
        lpfl = LpflFromId(app, 0);
        if (!lpfl || lpfl->cord != 2 || lpfl->lpplord->rgord()[1].id != 4) return;
        if (!FSaveState(app, "mozart-waypoint")) goto LArtifact;
        LogState(app, "mozart-waypoint-dragged");
        iStage = 133; goto LQueued;
    case 149:
        if (hwndModal) { pszFailure = "unexpected tenth-generation dialog"; goto LFail; }
        if (app.state.game.turn == 9) return;
        if (app.state.game.turn != 10 || !app.state.gd.fTutorial || app.state.tutor.idt != 248) { pszFailure = "tenth generation did not reach page thirty-two"; goto LFail; }
        if (!FSaveState(app, "year-2410")) goto LArtifact;
        if (fProdTemplates) {
            if (!FSaveOrders(app, "year-2410-start-orders.json")) goto LArtifact;
            iStage = 150; goto LQueued;
        }
        fPassed = fRejected && cPage == 32;
        LogState(app, "oxygen-colonization-partial");
        win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
    case 150:
        if (app.state.game.turn != 10 || (hwndModal && hwndModal->lpfnDlg != ProductionDlg && hwndModal->lpfnDlg != ZipProdDlg)) {
            pszFailure = "unexpected production template state or dialog"; goto LFail;
        }
        idtBefore = app.state.tutor.idtBold; iMsgBefore = app.state.iMsgCur;
        LogState(app, "production-templates-instruction");
        if (app.state.tutor.idtBold == 248) {
            if (IdmGetMessageN(app, app.state.iMsgCur) != idmHasDismantledKtMineralsWhichHaveDeposited) {
                if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
                iStage = 151; goto LQueued;
            }
            ptRow = (POINT){app.lpui->rcMsgTitle.left + (app.lpui->rcMsgTitle.bottom - app.lpui->rcMsgTitle.top) / 2,
                            (app.lpui->rcMsgTitle.top + app.lpui->rcMsgTitle.bottom) / 2};
            if (!FClickPoint(app.lpui->hwndMessage, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 163; goto LQueued;
        }
        if (app.state.tutor.idtBold == 249 || app.state.tutor.idtBold == 259) {
            i = app.state.tutor.idtBold == 249 ? 14 : 23;
            if (app.lpui->mdMsgObj != mdMsgObjPlanet || app.lpui->idMsgObj != i) {
                if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
                iStage = 151; goto LQueued;
            }
            if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
            iStage = i == 14 ? 152 : 160; goto LQueued;
        }
        if (app.state.tutor.idtBold == 250) {
            iProdPass = 8; cProdAdded = 0; iProdReturn = 153;
            if (!FPushTextKey(app, SDL_SCANCODE_Q, "q")) goto LInput;
            cActions++; iStage = 45; goto LQueued;
        }
        if (app.state.tutor.idtBold == 254) {
            if (!FPushTextKey(app, SDL_SCANCODE_Q, "q")) goto LInput;
            cActions++; iStage = 154; goto LQueued;
        }
        if (app.state.tutor.idtBold == 255) {
            if (!hwndModal || hwndModal->lpfnDlg != ProductionDlg) goto LInput;
            pszMenu = "<Customize>";
            ptRow = (POINT){(app.lpui->rcProdDiamond.left + app.lpui->rcProdDiamond.right) / 2, (app.lpui->rcProdDiamond.top + app.lpui->rcProdDiamond.bottom) / 2};
            if (!FClickPoint(hwndModal, ptRow, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 155; goto LQueued;
        }
        if (app.state.tutor.idtBold == 260 || app.state.tutor.idtBold == 262) {
            if (app.state.sel.grobj != grobjPlanet || app.state.sel.id != 13) {
                if (!FFindPlanet(app, "Stove Top")) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 161; goto LQueued;
            }
            iProdPass = app.state.tutor.idtBold == 260 ? 3 : 10; cProdAdded = 0; iProdReturn = 153;
            if (!FPushTextKey(app, SDL_SCANCODE_Q, "q")) goto LInput;
            cActions++; iStage = 68; goto LQueued;
        }
        if (app.state.tutor.idtBold == 263) {
            if (!app.state.tutor.fTurnDone || !FSaveOrders(app, "year-2410-orders.json") || !FSaveState(app, "before-generate-2410")) goto LArtifact;
            LogState(app, "generate-ready");
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
            cActions++; iStage = 162; goto LQueued;
        }
        pszFailure = "unimplemented production template instruction"; goto LFail;
    case 151:
        if (app.state.iMsgCur == iMsgBefore) return;
        iStage = 150; goto LQueued;
    case 152:
        if (app.state.sel.grobj != grobjPlanet || app.state.sel.id != 14 || app.state.tutor.idtBold == idtBefore) return;
        if (!FSaveState(app, "shaggy-dog-summary")) goto LArtifact;
        LogState(app, "shaggy-dog-goto");
        iStage = 150; goto LQueued;
    case 153:
        if (hwndModal || app.state.tutor.idtBold == idtBefore) return;
        if (idtBefore == 250 && !FSaveOrders(app, "year-2410-shaggy-queue.json")) goto LArtifact;
        iStage = 150; goto LQueued;
    case 154:
        if (!hwndModal || hwndModal->lpfnDlg != ProductionDlg || app.state.tutor.idtBold == idtBefore) return;
        iStage = 150; goto LQueued;
    case 155:
        if (pszMenu || !hwndModal || hwndModal->lpfnDlg != ZipProdDlg) return;
        if (!win32::IsWindow(hwndModal->hwndOwner) || hwndModal->hwndOwner->lpfnDlg != ProductionDlg || win32::IsWindowEnabled(hwndModal->hwndOwner)) {
            pszFailure = "template dialog did not disable its production owner"; goto LFail;
        }
        if (!FSaveState(app, "template-open")) goto LArtifact;
        LogState(app, "template-nested-open");
        if (!FClick(win32::GetDlgItem(hwndModal, IDC_ZIP_PROD_PRESET_1))) goto LInput;
        break;
    case 156:
        if (!hwndModal || hwndModal->lpfnDlg != ZipProdDlg || !win32::IsDlgButtonChecked(hwndModal, IDC_ZIP_PROD_PRESET_1)) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDC_IMPORT))) goto LInput;
        break;
    case 157:
        if (!hwndModal || hwndModal->lpfnDlg != ZipProdDlg || !app.state.vrgZipProd[0].fValid || app.state.vrgZipProd[0].cpq != 2) return;
        if (app.state.vrgZipProd[0].rgpq[0].mdIdle() != iobjFactory || app.state.vrgZipProd[0].rgpq[0].cQuan() != 3 ||
            app.state.vrgZipProd[0].rgpq[1].mdIdle() != iobjMine || app.state.vrgZipProd[0].rgpq[1].cQuan() != 3 || !app.state.vrgZipProd[0].fNoResearch) return;
        if (!FSaveOrders(app, "year-2410-imported-template.json") || !FSaveState(app, "template-imported")) goto LArtifact;
        if (!FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 158:
        if (!hwndModal || hwndModal->lpfnDlg != ProductionDlg || !win32::IsWindowEnabled(hwndModal)) return;
        if (!FSaveState(app, "template-owner-restored")) goto LArtifact;
        LogState(app, "template-owner-restored");
        if (!FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 159:
        if (hwndModal || app.state.lpplProdGlob) return;
        if (!FSaveOrders(app, "year-2410-template-orders.json") || !FSaveState(app, "template-closed")) goto LArtifact;
        /* An earlier progress flag can skip the prompt; still exercise its Goto. */
        if (app.lpui->mdMsgObj != mdMsgObjPlanet || app.lpui->idMsgObj != 23) {
            iMsgBefore = app.state.iMsgCur;
            if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
            iStage = 164; goto LQueued;
        }
        if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
        iStage = 160; goto LQueued;
    case 160:
        if (app.state.sel.scan.grobj != grobjPlanet || app.state.sel.scan.idpl != 23) return;
        if (!FSaveState(app, "bloop-summary")) goto LArtifact;
        LogState(app, "bloop-goto");
        iStage = 150; goto LQueued;
    case 161:
        if (pszMenu) return;
        iStage = 150; goto LQueued;
    case 162:
        if (hwndModal) { pszFailure = "unexpected eleventh-generation dialog"; goto LFail; }
        if (app.state.game.turn == 10) return;
        if (app.state.game.turn != 11 || !app.state.gd.fTutorial || app.state.tutor.idt != 264) { pszFailure = "eleventh generation did not reach page thirty-four"; goto LFail; }
        if (!FSaveState(app, "year-2411")) goto LArtifact;
        if (fBattleCycle) {
            if (!FSaveOrders(app, "year-2411-start-orders.json")) goto LArtifact;
            iStage = 165; goto LQueued;
        }
        fPassed = fRejected && cPage == 34;
        LogState(app, "production-templates-partial");
        win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
    case 163:
        if (!(app.state.bitfMsgFiltered[idmHasDismantledKtMineralsWhichHaveDeposited >> 3] & (1 << (idmHasDismantledKtMineralsWhichHaveDeposited & 7)))) return;
        iStage = 150; goto LQueued;
    case 164:
        if (app.state.iMsgCur == iMsgBefore) return;
        iStage = 159; goto LQueued;
    case 165:
        if (app.state.game.turn != 11 || (hwndModal && hwndModal->lpfnDlg != ResearchDlg)) {
            pszFailure = "unexpected battle cycle state or dialog"; goto LFail;
        }
        idtBefore = app.state.tutor.idtBold; iMsgBefore = app.state.iMsgCur;
        LogState(app, "battle-cycle-instruction");
        if (app.state.tutor.idtBold == 264 || app.state.tutor.idtBold == 266 || app.state.tutor.idtBold == 269 ||
            app.state.tutor.idtBold == 271 || app.state.tutor.idtBold == 272 || app.state.tutor.idtBold == 275 || app.state.tutor.idtBold == 286) {
            idflAction = app.state.tutor.idtBold == 264 ? 0 : app.state.tutor.idtBold == 266 ? 1 : app.state.tutor.idtBold == 269 ? 4 :
                         app.state.tutor.idtBold == 272 ? 2 : app.state.tutor.idtBold == 275 ? 11 : 8;
            if (app.state.sel.grobj != grobjFleet || app.state.sel.id != idflAction) {
                if ((app.lpui->mdMsgObj != mdMsgObjFleet || app.lpui->idMsgObj != idflAction) &&
                    !(app.state.tutor.idtBold == 286 && app.lpui->mdMsgObj == mdMsgObjBattle && !app.state.gd.fGotoVCR)) {
                    if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
                    iStage = 166; goto LQueued;
                }
                if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
                iStage = 167; goto LQueued;
            }
            /* These prompts include the route gesture after selecting the fleet. */
            if (app.state.tutor.idtBold != 269 && app.state.tutor.idtBold != 271) return;
        }
        if (app.state.tutor.idtBold == 265 || app.state.tutor.idtBold == 268 || app.state.tutor.idtBold == 269 || app.state.tutor.idtBold == 271 ||
            app.state.tutor.idtBold == 274 || app.state.tutor.idtBold == 276) {
            idflAction = app.state.tutor.idtBold == 265 ? 0 : app.state.tutor.idtBold == 268 ? 1 : app.state.tutor.idtBold == 269 ? 4 :
                         app.state.tutor.idtBold == 271 ? 8 : app.state.tutor.idtBold == 274 ? 2 : 11;
            lpfl = LpflFromId(app, idflAction);
            if (!lpfl || app.state.sel.grobj != grobjFleet || app.state.sel.id != idflAction) goto LInput;
            if (idflAction == 2 || idflAction == 11) {
                cCargoGoal = idflAction == 2 ? 25 : 210;
                if (lpfl->rgwtMin[3] != cCargoGoal) {
                    if (!FClick(app.lpui->rghwndBtn[7])) goto LInput;
                    iStage = 169; goto LQueued;
                }
            }
            if (lpfl->cord == 1) {
                if (idflAction == 8) {
                    if (!FFindMapObject(app, grobjFleet, 512, NULL)) goto LInput;
                } else if (!FFindPlanet(app, idflAction == 0 ? "Hacker" : idflAction == 2 ? "Mozart" : idflAction == 11 ? "Wallaby" : "Stove Top")) goto LInput;
                if (!FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) ||
                    !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
                iStage = 168; goto LQueued;
            }
            if (idflAction != 1 && idflAction != 2) { pszFailure = "route did not advance battle cycle instruction"; goto LFail; }
            iOrderCombo = 0; iOrderRow = idflAction == 1 ? grTaskScrap : grTaskColonize;
            if (!FClick(app.lpui->rghwndOrderDD[iOrderCombo])) goto LInput;
            iStage = 173; goto LQueued;
        }
        if (app.state.tutor.idtBold == 277 || app.state.tutor.idtBold == 278 || app.state.tutor.idtBold == 279) {
            iOrderCombo = app.state.tutor.idtBold - 277;
            iOrderRow = iOrderCombo == 0 ? grTaskXfer : iOrderCombo == 1 ? 4 : iActionUnloadAll;
            if (!FClick(app.lpui->rghwndOrderDD[iOrderCombo])) goto LInput;
            iStage = 173; goto LQueued;
        }
        if (app.state.tutor.idtBold == 280) {
            if (app.state.sel.grobj != grobjPlanet || app.state.sel.id != 13) {
                if (app.lpui->mdMsgObj != mdMsgObjPlanet || app.lpui->idMsgObj != 13) {
                    if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
                    iStage = 166; goto LQueued;
                }
                if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
                iStage = 175; goto LQueued;
            }
            iProdPass = 11; cProdAdded = 0; iProdReturn = 176;
            if (!FPushTextKey(app, SDL_SCANCODE_Q, "q")) goto LInput;
            cActions++; iStage = 68; goto LQueued;
        }
        if (app.state.tutor.idtBold == 281) {
            if (app.lpui->mdMsgObj != mdMsgObjResearch) {
                if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
                iStage = 166; goto LQueued;
            }
            if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
            iStage = 177; goto LQueued;
        }
        if (app.state.tutor.idtBold == 282) {
            if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg || !FClick(win32::GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD))) goto LInput;
            iStage = 178; goto LQueued;
        }
        if (app.state.tutor.idtBold == 283) {
            if (app.state.iMsgCur < 11) {
                if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
                iStage = 191; goto LQueued;
            }
            if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
            iStage = 181; goto LQueued;
        }
        if (app.state.tutor.idtBold == 284) {
            if (!win32::IsWindow(app.lpui->hwndBrowser) || !FSaveState(app, "technology-browser")) goto LArtifact;
            LogState(app, "technology-browser-open");
            if (!FClick(win32::GetDlgItem(app.lpui->hwndBrowser, IDCANCEL))) goto LInput;
            iStage = 182; goto LQueued;
        }
        if (app.state.tutor.idtBold == 285 || app.state.tutor.idtBold == 291) {
            if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
            iStage = 166; goto LQueued;
        }
        if (app.state.tutor.idtBold == 289) {
            if (app.lpui->mdMsgObj != mdMsgObjBattle) goto LInput;
            if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
            iStage = 183; goto LQueued;
        }
        if (app.state.tutor.idtBold == 295) {
            if (!app.state.tutor.fTurnDone || !FSaveOrders(app, "year-2411-orders.json") || !FSaveState(app, "before-generate-2411")) goto LArtifact;
            LogState(app, "generate-ready");
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
            cActions++; iStage = 190; goto LQueued;
        }
        pszFailure = "unimplemented battle cycle instruction"; goto LFail;
    case 166:
        if (app.state.iMsgCur == iMsgBefore) return;
        iStage = 165; goto LQueued;
    case 167:
        if (app.state.sel.grobj != grobjFleet || app.state.sel.id != idflAction) return;
        iStage = 165; goto LQueued;
    case 168:
        lpfl = LpflFromId(app, idflAction);
        if (!lpfl || lpfl->cord != 2) return;
        iStage = 165; goto LQueued;
    case 169:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || app.lpui->mdXferDlg != mdXferCargo || !app.state.pxfer || app.state.pxfer[0].id != idflAction || app.state.pxfer[1].id != 13) goto LInput;
        for (i = 0; i < app.lpui->crgbtnXfer; i++) {
            if ((app.lpui->rgbtnXfer[i].iVal & 0x7f) == 3 && app.lpui->rgbtnXfer[i].iSide == 0 && !app.lpui->rgbtnXfer[i].fVisible && !(app.lpui->rgbtnXfer[i].bt & 4)) break;
        }
        if (i == app.lpui->crgbtnXfer) goto LInput;
        memcpy(&rcRow, &app.lpui->rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){rcRow.left + 2, (rcRow.top + rcRow.bottom) / 2}; ptTo = (POINT){rcRow.right - 2, ptRow.y};
        if (!FDragPoint(hwndModal, ptRow, ptTo)) goto LInput;
        break;
    case 170:
        if (!hwndModal || !app.state.pxfer || app.state.pxfer[0].fl().rgwtMin[3] != cCargoGoal) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 171:
        if (hwndModal) return;
        if (!FSaveOrders(app, idflAction == 2 ? "year-2411-colonizer-cargo.json" : "year-2411-troop-cargo.json")) goto LArtifact;
        iStage = 165; goto LQueued;
    case 173:
        if (!ui::SendMessage(app.lpui->rghwndOrderDD[iOrderCombo], win32::CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(app.lpui->rghwndOrderDD[iOrderCombo], iOrderRow)) goto LInput;
        break;
    case 174:
        if (ui::SendMessage(app.lpui->rghwndOrderDD[iOrderCombo], win32::CB_GETDROPPEDSTATE, 0, 0) || ui::SendMessage(app.lpui->rghwndOrderDD[iOrderCombo], win32::CB_GETCURSEL, 0, 0) != iOrderRow) return;
        iStage = 165; goto LQueued;
    case 175:
        if (app.state.sel.grobj != grobjPlanet || app.state.sel.id != 13) return;
        iStage = 165; goto LQueued;
    case 176:
        if (hwndModal || app.state.lpplProdGlob) return;
        if (!FSaveOrders(app, "year-2411-mines.json")) goto LArtifact;
        iStage = 165; goto LQueued;
    case 177:
        if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg) return;
        iStage = 165; goto LQueued;
    case 178:
        if (!hwndModal) return;
        hwndList = win32::GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD);
        if (!ui::SendMessage(hwndList, win32::CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(hwndList, Biotechnology + 1)) goto LInput;
        break;
    case 179:
        if (!hwndModal) return;
        hwndList = win32::GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD);
        if (ui::SendMessage(hwndList, win32::CB_GETDROPPEDSTATE, 0, 0) || ui::SendMessage(hwndList, win32::CB_GETCURSEL, 0, 0) != Biotechnology + 1) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDCANCEL))) goto LInput;
        break;
    case 180:
        if (hwndModal || app.lpui->pctResGlob != -1) return;
        if (!FSaveOrders(app, "year-2411-research.json")) goto LArtifact;
        iStage = 165; goto LQueued;
    case 181:
        if (!win32::IsWindow(app.lpui->hwndBrowser)) return;
        if (app.lpui->vpartBrowser.hs.grhst != hstTorp || app.lpui->vpartBrowser.hs.iItem != itorpBetaTorpedo) {
            pszFailure = "technology browser did not show the Beta Torpedo"; goto LFail;
        }
        /* FTutorTaskDone can advance the prompt as soon as this window opens. */
        if (!FSaveState(app, "technology-browser")) goto LArtifact;
        LogState(app, "technology-browser-open");
        if (!FClick(win32::GetDlgItem(app.lpui->hwndBrowser, IDCANCEL))) goto LInput;
        break;
    case 182:
        if (win32::IsWindow(app.lpui->hwndBrowser)) return;
        if (!FSaveState(app, "technology-browser-closed")) goto LArtifact;
        LogState(app, "technology-browser-closed");
        iStage = 165; goto LQueued;
    case 183:
        if (!hwndModal || hwndModal->lpfnDlg != VCRDlg || !app.lpui->vlpbdVCR || !app.state.vrgtok || app.lpui->vcStepVCR <= 0) return;
        if (!FSaveState(app, "battle-open") || !FSaveBattle(app, "battle-open.json")) goto LArtifact;
        LogState(app, "battle-open");
        iStepBefore = app.lpui->viStepVCRCur;
        if (!FClick(win32::GetDlgItem(hwndModal, IDC_VCR_PLAY_PAUSE))) goto LInput;
        break;
    case 184:
        if (!hwndModal || hwndModal->lpfnDlg != VCRDlg || app.lpui->viStepVCRCur <= iStepBefore || !app.state.gd.fVCRTimer) return;
        if (!FSaveState(app, "battle-playing") || !FSaveBattle(app, "battle-playing.json")) goto LArtifact;
        LogState(app, "battle-playing");
        if (!FClick(win32::GetDlgItem(hwndModal, IDC_VCR_PLAY_PAUSE))) goto LInput;
        break;
    case 185:
        if (app.state.gd.fVCRTimer) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDC_VCR_FWD_ALL))) goto LInput;
        break;
    case 186:
        if (app.lpui->viStepVCRCur != app.lpui->vcStepVCR) return;
        if (!FSaveBattle(app, "battle-end.json")) goto LArtifact;
        if (!FClick(win32::GetDlgItem(hwndModal, IDC_VCR_REW_ALL))) goto LInput;
        break;
    case 187:
        if (app.lpui->viStepVCRCur != -1) return;
        if (!FSaveBattle(app, "battle-rewound.json") || !FClick(win32::GetDlgItem(hwndModal, IDC_VCR_FWD))) goto LInput;
        break;
    case 188:
        if (app.lpui->viStepVCRCur <= -1) return;
        if (!FSaveBattle(app, "battle-stepped.json") || !FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 189:
        if (hwndModal || app.state.vrgtok || win32::IsWindow(app.lpui->hwndVCRDlg) || app.state.gd.fVCRTimer) return;
        if (!FSaveState(app, "battle-closed")) goto LArtifact;
        LogState(app, "battle-closed");
        iStage = 165; goto LQueued;
    case 190:
        if (hwndModal) { pszFailure = "unexpected twelfth-generation dialog"; goto LFail; }
        if (app.state.game.turn == 11) return;
        if (app.state.game.turn != 12 || !app.state.gd.fTutorial || app.state.tutor.idt != 296) { pszFailure = "twelfth generation did not reach page thirty-eight"; goto LFail; }
        if (!FSaveState(app, "year-2412")) goto LArtifact;
        if (fTerraformCycle) {
            if (!FSaveOrders(app, "year-2412-start-orders.json")) goto LArtifact;
            iStage = 192; goto LQueued;
        }
        fPassed = fRejected && cPage == 38;
        LogState(app, "battle-cycle-partial");
        win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
    case 191:
        if (app.state.iMsgCur == iMsgBefore) return;
        /* Production list selection can satisfy fProgress before this prompt.
         * Read its message and use Goto even if the highlighted task moved. */
        if (app.state.iMsgCur < 11) {
            iMsgBefore = app.state.iMsgCur;
            if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
            goto LQueued;
        }
        if (app.state.iMsgCur != 11 || app.lpui->mdMsgObj != mdMsgObjPart) goto LInput;
        if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
        iStage = 181; goto LQueued;
    case 192:
        if (app.state.game.turn != 12 || hwndModal) { pszFailure = "unexpected terraforming cycle state or dialog"; goto LFail; }
        idtBefore = app.state.tutor.idtBold; iMsgBefore = app.state.iMsgCur;
        LogState(app, "terraform-cycle-instruction");
        if (app.state.tutor.idtBold == 297 || app.state.tutor.idtBold == 310 || app.state.tutor.idtBold == 313) {
            i = app.state.tutor.idtBold == 297 ? 8 : app.state.tutor.idtBold == 310 ? 18 : 8;
            if (app.state.iMsgCur >= 1 && ((app.state.tutor.idtBold == 297 && app.state.sel.grobj == grobjFleet && app.state.sel.id == i) ||
                (app.state.tutor.idtBold != 297 && app.state.sel.grobj == grobjPlanet && app.state.sel.id == i))) return;
            if (app.state.iMsgCur < 1 || app.lpui->mdMsgObj != (app.state.tutor.idtBold == 297 ? mdMsgObjFleet : mdMsgObjPlanet) || app.lpui->idMsgObj != i) {
                if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
                iStage = 193; goto LQueued;
            }
            if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
            iStage = 194; goto LQueued;
        }
        if (app.state.tutor.idtBold == 300) {
            if (!FFindMapObject(app, grobjFleet, 8, NULL) || !FFindSalvageHere(app)) goto LInput;
            // The original resource includes a trailing space; use the copied label.
            pszMenu = objTarget.szName;
            if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 195; goto LQueued;
        }
        if (app.state.tutor.idtBold == 303) {
            if (!FPushControlKey(app.lpui->hwndFrame, SDL_SCANCODE_F)) goto LInput;
            iStage = 196; goto LQueued;
        }
        if (app.state.tutor.idtBold == 306 || app.state.tutor.idtBold == 308) {
            if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
            iStage = 193; goto LQueued;
        }
        if (app.state.tutor.idtBold == 316 || app.state.tutor.idtBold == 318) {
            if (app.state.tutor.idtBold == 318 && (app.state.sel.grobj != grobjPlanet || app.state.sel.id != 13)) {
                if (!FFindPlanet(app, "Stove Top")) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 194; goto LQueued;
            }
            if (!FPushTextKey(app, SDL_SCANCODE_Q, "q")) goto LInput;
            iProdPass = app.state.tutor.idtBold == 316 ? 12 : 13; cProdAdded = 0;
            iProdReturn = 199; iStage = 68; goto LQueued;
        }
        if (app.state.tutor.idtBold == 319) {
            if (!app.state.tutor.fTurnDone || !FSaveOrders(app, "year-2412-orders.json") || !FSaveState(app, "before-generate-2412")) goto LArtifact;
            LogState(app, "generate-ready");
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
            iStage = 200; goto LQueued;
        }
        pszFailure = "unimplemented terraforming cycle instruction"; goto LFail;
    case 193:
        if (app.state.iMsgCur == iMsgBefore) return;
        if (idtBefore == 297) {
            // The previous VCR leaves this fleet selected, so the tutor can
            // advance after Next alone. Still perform the requested Goto.
            if (app.state.iMsgCur < 1 || (app.lpui->mdMsgObj != mdMsgObjBattle && (app.lpui->mdMsgObj != mdMsgObjFleet || app.lpui->idMsgObj != 8)) ||
                app.state.gd.fGotoVCR) goto LInput;
            mdProbeMessage = app.lpui->mdMsgObj;
            if (!FSaveState(app, "armed-probe-message") || !FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
            iStage = 202; goto LQueued;
        }
        iStage = 192; goto LQueued;
    case 194:
        if (app.state.tutor.idtBold == idtBefore && !(idtBefore == 318 && app.state.sel.grobj == grobjPlanet && app.state.sel.id == 13)) return;
        iStage = 192; goto LQueued;
    case 195:
        if (pszMenu || app.state.sel.scan.grobj != grobjThing) return;
        if (!FSaveSalvage(app) || !FSaveState(app, "salvage-summary")) goto LArtifact;
        LogState(app, "salvage-selected");
        iStage = 192; goto LQueued;
    case 196:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != FindDlg || !FSaveState(app, "find-teamster-open")) goto LArtifact;
        if (!FClick(win32::GetDlgItem(hwndModal, IDC_EDIT1))) goto LInput;
        break;
    case 197:
        if (!hwndModal || hwndModal->lpfnDlg != FindDlg) goto LInput;
        hwndList = win32::GetDlgItem(hwndModal, IDC_EDIT1);
        if (win32::GetFocus() != hwndList) return;
        {
            SDL_Event event = {};
            event.type = SDL_EVENT_TEXT_INPUT;
            event.text.windowID = HwndCompatRoot(hwndList)->id;
            event.text.text = "Teamster #4";
            if (!SDL_PushEvent(&event)) goto LInput;
        }
        break;
    case 198:
        if (!hwndModal || hwndModal->lpfnDlg != FindDlg) goto LInput;
        win32::GetDlgItemTextA(hwndModal, IDC_EDIT1, szItem, sizeof(szItem));
        if (strcmp(szItem, "Teamster #4")) return;
        if (!FSaveState(app, "find-teamster-entered") || !FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        iStage = 201; goto LQueued;
    case 199:
        if (hwndModal) return;
        if (!FSaveOrders(app, iProdPass == 12 ? "year-2412-terraform-orders.json" : "year-2412-teamster-orders.json")) goto LArtifact;
        iStage = 192; goto LQueued;
    case 200:
        if (hwndModal) { pszFailure = "unexpected thirteenth-generation dialog"; goto LFail; }
        if (app.state.game.turn == 12) return;
        if (app.state.game.turn != 13 || !app.state.gd.fTutorial || app.state.tutor.idt != 320) { pszFailure = "thirteenth generation did not reach page forty-one"; goto LFail; }
        if (!FSaveState(app, "year-2413")) goto LArtifact;
        if (fDesignCycle) {
            if (!FSaveOrders(app, "year-2413-start-orders.json")) goto LArtifact;
            iStage = 203; goto LQueued;
        }
        fPassed = fRejected && cPage == 41;
        LogState(app, "terraform-cycle-partial");
        win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
    case 201:
        if (hwndModal || app.state.sel.scan.grobj != grobjFleet || app.state.sel.scan.ifl == iflNone || app.state.rglpfl[app.state.sel.scan.ifl]->id != 3) return;
        if (!FSaveState(app, "find-teamster-closed")) goto LArtifact;
        LogState(app, "teamster-found");
        iStage = 192; goto LQueued;
    case 202:
        if (hwndModal || app.state.sel.grobj != grobjFleet || app.state.sel.id != 8 || app.state.sel.scan.grobj != grobjFleet ||
            app.state.sel.scan.ifl == iflNone || app.state.rglpfl[app.state.sel.scan.ifl]->id != 8 ||
            (mdProbeMessage == mdMsgObjBattle && !app.state.gd.fGotoVCR)) return;
        if (!FSaveState(app, "armed-probe-goto")) goto LArtifact;
        LogState(app, "armed-probe-goto");
        iStage = 192; goto LQueued;
    case 203:
        if (app.state.game.turn != 13) { pszFailure = "unexpected design cycle year"; goto LFail; }
        idtBefore = app.state.tutor.idtBold; iMsgBefore = app.state.iMsgCur;
        if (idtDesignBefore != app.state.tutor.idtBold) { idtDesignBefore = app.state.tutor.idtBold; cDesignVisits = 0; }
        if (++cDesignVisits > 10) { pszFailure = "design instruction repeated without progress"; goto LFail; }
        LogState(app, "design-cycle-instruction");
        if (app.state.tutor.idtBold == 320) {
            if (app.state.iMsgCur < 1) {
                if (!FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
                iStage = 204; goto LQueued;
            }
            if (app.state.sel.grobj != grobjFleet || app.state.sel.id != 0) {
                if (!FFindMapObject(app, grobjFleet, 0, NULL)) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 204; goto LQueued;
            }
            idCargoFleet = 0; idCargoPlanet = 13; cCargoExpected = 210; iCargoReturn = 203; pszCargoSnapshot = "year-2413-cargo.json";
            if (!FClick(app.lpui->rghwndBtn[7])) goto LInput;
            iStage = 205; goto LQueued;
        }
        if (app.state.tutor.idtBold == 321) {
            lpfl = LpflFromId(app, 0);
            if (!lpfl || app.state.sel.grobj != grobjFleet || app.state.sel.id != 0) goto LInput;
            if (lpfl->cord == 1) {
                if (!FFindPlanet(app, "Slime") || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) ||
                    !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
                iStage = 204; goto LQueued;
            }
            iOrderCombo = app.state.sel.fl.lpplord->rgord()[1].grTask != grTaskXfer ? 0 :
                app.state.sel.fl.lpplord->rgord()[1].txp.rgia[3].iAction() != iActionUnloadAll ? 1 : 2;
            iOrderRow = iOrderCombo == 0 ? grTaskXfer : iOrderCombo == 1 ? 4 : iActionUnloadAll;
            iOrderReturn = 203; pszOrderSnapshot = "year-2413-transport.json";
            if (!FClick(app.lpui->rghwndOrderDD[iOrderCombo])) goto LInput;
            iStage = 209; goto LQueued;
        }
        if (app.state.tutor.idtBold == 322 || app.state.tutor.idtBold == 324 || app.state.tutor.idtBold == 325) {
            if (hwndModal && hwndModal->lpfnDlg == ResearchDlg) {
                if (!FChooseCombo(win32::GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD), "Propulsion")) goto LInput;
                iDesignReturn = 211; iStage = 221; goto LQueued;
            }
            if (app.state.tutor.idtBold == 322 || (app.state.tutor.idtBold == 324 && app.lpui->mdMsgObj != mdMsgObjResearch) ||
                (app.state.tutor.idtBold == 325 && app.lpui->mdMsgObj != mdMsgObjPart)) {
                if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
                iStage = 204; goto LQueued;
            }
            if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
            iStage = 204; goto LQueued;
        }
        if (app.state.tutor.idtBold == 327) {
            if (win32::IsWindow(app.lpui->hwndBrowser)) {
                if (!FClick(win32::GetDlgItem(app.lpui->hwndBrowser, IDCANCEL))) goto LInput;
                iStage = 204; goto LQueued;
            }
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F4, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F4, 0)) goto LInput;
            iStage = 204; goto LQueued;
        }
        if (app.state.tutor.idtBold == 328 || app.state.tutor.idtBold == 329 || app.state.tutor.idtBold == 330) {
            if (!win32::IsWindow(app.lpui->hwndSlotDlg)) goto LInput;
            if (app.state.tutor.idtBold == 328) {
                if (!FClick(win32::GetDlgItem(app.lpui->hwndSlotDlg, IDC_DESIGNER_HULLS))) goto LInput;
            } else if (app.state.tutor.idtBold == 329) {
                if (!FChooseCombo(win32::GetDlgItem(app.lpui->hwndSlotDlg, IDC_COMBOBOX), "Mini-Miner")) goto LInput;
                iDesignReturn = 204; iStage = 221; goto LQueued;
            } else if (!FClick(win32::GetDlgItem(app.lpui->hwndSlotDlg, IDC_IMPORT))) goto LInput;
            iStage = 204; goto LQueued;
        }
        if (app.state.tutor.idtBold == 332 || app.state.tutor.idtBold == 333 || app.state.tutor.idtBold == 334 || app.state.tutor.idtBold == 335) {
            if (!win32::IsWindow(app.lpui->hwndSlotDlg) || app.lpui->mdBuild != mdBuildEdit || !app.lpui->lpshdefBuild) goto LInput;
            iDesignSlot = app.state.tutor.idtBold == 332 ? 0 : app.state.tutor.idtBold == 333 ? 1 : app.lpui->lpshdefBuild->hul.rghs[2].cItem == 0 ? 2 : 3;
            pszDesignCategory = iDesignSlot == 0 ? "Engines" : iDesignSlot == 1 ? "Scanners" : "Mining Robots";
            pszDesignPart = iDesignSlot == 0 ? "Long Hump 6" : iDesignSlot == 1 ? "Rhino Scanner" : "Robo-Miner";
            if (!FChooseCombo(win32::GetDlgItem(app.lpui->hwndSlotDlg, IDC_COMBOBOX), pszDesignCategory)) goto LInput;
            iDesignReturn = 213; iStage = 221; goto LQueued;
        }
        if (app.state.tutor.idtBold == 337 || app.state.tutor.idtBold == 338) {
            if (!win32::IsWindow(app.lpui->hwndSlotDlg) || !FSaveState(app, "mini-miner-designed") ||
                !FClick(win32::GetDlgItem(app.lpui->hwndSlotDlg, app.state.tutor.idtBold == 337 ? IDOK : IDCANCEL))) goto LInput;
            iStage = 204; goto LQueued;
        }
        if (app.state.tutor.idtBold == 339 || app.state.tutor.idtBold == 341 || app.state.tutor.idtBold == 342) {
            if (app.state.sel.grobj != grobjPlanet || app.state.sel.id != 13) {
                if (!FFindPlanet(app, "Stove Top")) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 204; goto LQueued;
            }
            iProdPass = app.state.tutor.idtBold == 339 ? 14 : 15; cProdAdded = 0; iProdReturn = 218;
            if (!hwndModal && !FPushTextKey(app, SDL_SCANCODE_Q, "q")) goto LInput;
            iStage = 68; goto LQueued;
        }
        if (app.state.tutor.idtBold == 344) {
            hwndList = app.lpui->hwndPlanetProdLB;
            if (!win32::IsWindow(hwndList) || ui::SendMessage(hwndList, win32::LB_GETITEMRECT, 0, (LPARAM)&rcRow) == win32::LB_ERR) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            iProductionInspect = 0;
            if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 219; goto LQueued;
        }
        if (app.state.tutor.idtBold == 345) {
            if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
            iStage = 204; goto LQueued;
        }
        if (app.state.tutor.idtBold == 348) {
            if (app.state.sel.grobj != grobjFleet || app.state.sel.id != 8) {
                if (!FFindMapObject(app, grobjFleet, 8, NULL)) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            } else if (!FFindPlanet(app, "Stove Top") || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) ||
                !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
            iStage = 204; goto LQueued;
        }
        if (app.state.tutor.idtBold == 350) {
            if (!app.state.tutor.fTurnDone || !FSaveOrders(app, "year-2413-orders.json") || !FSaveState(app, "before-generate-2413")) goto LArtifact;
            LogState(app, "generate-ready");
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
            iStage = 220; goto LQueued;
        }
        pszFailure = "unimplemented design cycle instruction"; goto LFail;
    case 204:
        if (pszMenu || (idtBefore == app.state.tutor.idtBold && iMsgBefore == app.state.iMsgCur && cWait < 3)) return;
        iStage = 203; goto LQueued;
    case 205:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || !app.state.pxfer || app.state.pxfer[0].id != idCargoFleet || app.state.pxfer[1].id != idCargoPlanet) goto LInput;
        for (i = 0; i < app.lpui->crgbtnXfer; i++) {
            if ((app.lpui->rgbtnXfer[i].iVal & 0x7f) == 3 && app.lpui->rgbtnXfer[i].iSide == 0 && !app.lpui->rgbtnXfer[i].fVisible && !(app.lpui->rgbtnXfer[i].bt & 4)) break;
        }
        if (i == app.lpui->crgbtnXfer) goto LInput;
        memcpy(&rcRow, &app.lpui->rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){rcRow.left + 2, (rcRow.top + rcRow.bottom) / 2}; ptTo = (POINT){rcRow.right - 2, ptRow.y};
        if (!FDragPoint(hwndModal, ptRow, ptTo)) goto LInput;
        break;
    case 206:
        if (!hwndModal || !app.state.pxfer || app.state.pxfer[0].fl().rgwtMin[3] != cCargoExpected) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 207:
        if (hwndModal) return;
        if (!FSaveOrders(app, pszCargoSnapshot)) goto LArtifact;
        if (iCargoReturn == 235) {
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F3, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F3, 0)) goto LInput;
            iStage = 237; goto LQueued;
        }
        iStage = iCargoReturn; goto LQueued;
    case 209:
        if (!ui::SendMessage(app.lpui->rghwndOrderDD[iOrderCombo], win32::CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(app.lpui->rghwndOrderDD[iOrderCombo], iOrderRow)) goto LInput;
        break;
    case 210:
        if (ui::SendMessage(app.lpui->rghwndOrderDD[iOrderCombo], win32::CB_GETDROPPEDSTATE, 0, 0)) return;
        if (iOrderCombo == 1) {
            iOrderCombo = 2; iOrderRow = iActionUnloadAll;
            if (!FClick(app.lpui->rghwndOrderDD[2])) goto LInput;
            iStage = 209; goto LQueued;
        }
        if (!FSaveOrders(app, pszOrderSnapshot)) goto LArtifact;
        iStage = iOrderReturn; goto LQueued;
    case 211:
        if (!hwndModal || ui::SendMessage(win32::GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD), win32::CB_GETCURSEL, 0, 0) != iResearchNext + 1) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDCANCEL))) goto LInput;
        break;
    case 212:
        if (hwndModal) return;
        if (!FSaveOrders(app, pszResearchSnapshot)) goto LArtifact;
        iStage = iResearchReturn; goto LQueued;
    case 213:
        if (!win32::IsWindow(app.lpui->hwndSlotDlg)) goto LInput;
        hwndList = win32::GetDlgItem(app.lpui->hwndSlotDlg, IDC_DESIGNER_COMPONENT_LIST);
        iDesignRow = -1;
        for (i = 0; i < ui::SendMessage(hwndList, win32::LB_GETCOUNT, 0, 0); i++) {
            if (ui::SendMessage(hwndList, win32::LB_GETTEXTLEN, i, 0) >= (LRESULT)sizeof(szItem)) goto LInput;
            ui::SendMessage(hwndList, win32::LB_GETTEXT, i, (LPARAM)szItem);
            if (strlen(szItem) > 4 && !strcmp(szItem + 4, pszDesignPart)) { iDesignRow = i; break; }
        }
        if (iDesignRow < 0) return;
        if (ui::SendMessage(hwndList, win32::LB_GETITEMRECT, ui::SendMessage(hwndList, win32::LB_GETTOPINDEX, 0, 0), (LPARAM)&rcRow) == win32::LB_ERR) goto LInput;
        ptRow = (POINT){rcRow.right - 4, (rcRow.top + rcRow.bottom) / 2};
        if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
        break;
    case 214:
        hwndList = win32::GetDlgItem(app.lpui->hwndSlotDlg, IDC_DESIGNER_COMPONENT_LIST);
        if (win32::GetFocus() != hwndList) return;
        if (!FPushKey(hwndList, SDL_SCANCODE_HOME, 1) || !FPushKey(hwndList, SDL_SCANCODE_HOME, 0)) goto LInput;
        for (i = 0; i < iDesignRow; i++) {
            if (!FPushKey(hwndList, SDL_SCANCODE_DOWN, 1) || !FPushKey(hwndList, SDL_SCANCODE_DOWN, 0)) goto LInput;
        }
        break;
    case 215:
        hwndList = win32::GetDlgItem(app.lpui->hwndSlotDlg, IDC_DESIGNER_COMPONENT_LIST);
        if (ui::SendMessage(hwndList, win32::LB_GETCURSEL, 0, 0) != iDesignRow) return;
        if (ui::SendMessage(hwndList, win32::LB_GETITEMRECT, iDesignRow, (LPARAM)&rcRow) == win32::LB_ERR) goto LInput;
        ptRow = (POINT){32, (rcRow.top + rcRow.bottom) / 2};
        ptTo = (POINT){(app.lpui->vrgrcSlot[iDesignSlot].left + app.lpui->vrgrcSlot[iDesignSlot].right) / 2,
                       (app.lpui->vrgrcSlot[iDesignSlot].top + app.lpui->vrgrcSlot[iDesignSlot].bottom) / 2};
        win32::ClientToScreen(app.lpui->hwndSlotDlg, &ptTo); win32::ScreenToClient(hwndList, &ptTo);
        if (!FDragPoint(hwndList, ptRow, ptTo)) goto LInput;
        break;
    case 216:
        if (!app.lpui->lpshdefBuild || app.lpui->lpshdefBuild->hul.rghs[iDesignSlot].cItem != 1) return;
        if (!FSaveState(app, "mini-miner-part")) goto LArtifact;
        iStage = 203; goto LQueued;
    case 218:
        if (hwndModal) return;
        if (!FSaveOrders(app, iProdPass == 14 ? "year-2413-miner-queue.json" : "year-2413-mines.json")) goto LArtifact;
        if (iProdPass == 15) {
            // A progress flag can skip the prompt; still inspect the real item.
            hwndList = app.lpui->hwndPlanetProdLB;
            if (!win32::IsWindow(hwndList) || ui::SendMessage(hwndList, win32::LB_GETITEMRECT, 0, (LPARAM)&rcRow) == win32::LB_ERR) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            iProductionInspect = 0;
            if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 219; goto LQueued;
        }
        iStage = 203; goto LQueued;
    case 219:
        hwndList = app.lpui->hwndPlanetProdLB;
        if (ui::SendMessage(hwndList, win32::LB_GETCURSEL, 0, 0) != iProductionInspect) return;
        LogState(app, "production-item-inspected");
        if (++iProductionInspect < ui::SendMessage(hwndList, win32::LB_GETCOUNT, 0, 0)) {
            if (ui::SendMessage(hwndList, win32::LB_GETITEMRECT, iProductionInspect, (LPARAM)&rcRow) == win32::LB_ERR) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            goto LQueued;
        }
        if (!FSaveState(app, "production-items")) goto LArtifact;
        iStage = 204; goto LQueued;
    case 220:
        if (hwndModal) { pszFailure = "unexpected fourteenth-generation dialog"; goto LFail; }
        if (app.state.game.turn == 13) return;
        if (app.state.game.turn != 14 || !app.state.gd.fTutorial || app.state.tutor.idt != 352) { pszFailure = "fourteenth generation did not reach page forty-five"; goto LFail; }
        if (!FSaveState(app, "year-2414")) goto LArtifact;
        if (fAutomationCycle) {
            if (!FSaveOrders(app, "year-2414-start-orders.json")) goto LArtifact;
            iStage = 224; goto LQueued;
        }
        fPassed = fRejected && cPage == 45;
        LogState(app, "design-cycle-partial");
        win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
    case 221:
        if (!win32::IsWindow(hwndDesignCombo) || !ui::SendMessage(hwndDesignCombo, win32::CB_GETDROPPEDSTATE, 0, 0)) return;
        if (win32::GetFocus() != hwndDesignCombo) return;
        // SDL assigns keyboard targets when translating the event batch.
        // Translate these only after the preceding click establishes focus.
        if (!FPushKey(hwndDesignCombo, SDL_SCANCODE_HOME, 1) || !FPushKey(hwndDesignCombo, SDL_SCANCODE_HOME, 0)) goto LInput;
        for (i = 0; i < iDesignCombo; i++) {
            if (!FPushKey(hwndDesignCombo, SDL_SCANCODE_DOWN, 1) || !FPushKey(hwndDesignCombo, SDL_SCANCODE_DOWN, 0)) goto LInput;
        }
        iStage = 223; goto LQueued;
    case 223:
        if (!win32::IsWindow(hwndDesignCombo) || !hwndDesignCombo->lpcombo || hwndDesignCombo->lpcombo->iHot != iDesignCombo) return;
        if (!FClickComboRow(hwndDesignCombo, iDesignCombo)) goto LInput;
        iStage = 222; goto LQueued;
    case 222:
        if (ui::SendMessage(hwndDesignCombo, win32::CB_GETDROPPEDSTATE, 0, 0)) return;
        if (ui::SendMessage(hwndDesignCombo, win32::CB_GETCURSEL, 0, 0) != iDesignCombo) goto LInput;
        iStage = iDesignReturn; goto LQueued;
    case 224:
        if (app.state.game.turn != 14) { pszFailure = "unexpected automation year"; goto LFail; }
        idtBefore = app.state.tutor.idtBold; iMsgBefore = app.state.iMsgCur;
        LogState(app, "automation-cycle-instruction");
        if (app.state.tutor.idtBold == 352 || app.state.tutor.idtBold == 354) {
            i = app.state.tutor.idtBold == 352 ? 17 : 2;
            if (app.lpui->mdMsgObj != mdMsgObjPlanet || app.lpui->idMsgObj != i || (i == 2 && IMsgNext(app, 0) != imsgNone)) {
                if (!win32::IsWindowEnabled(app.lpui->rghwndMsgBtn[2]) || !FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
            } else if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
            iStage = 225; goto LQueued;
        }
        if (app.state.tutor.idtBold == 356) {
            if (app.state.sel.grobj != grobjPlanet || app.state.sel.id != 2) goto LInput;
            iProdPass = 16; cProdAdded = 0; iProdReturn = 226;
            if (!FPushTextKey(app, SDL_SCANCODE_Q, "q")) goto LInput;
            iStage = 68; goto LQueued;
        }
        if (app.state.tutor.idtBold == 358) {
            if (!app.state.tutor.fTurnDone || !FSaveOrders(app, "year-2414-orders.json") || !FSaveState(app, "before-generate-2414")) goto LArtifact;
            LogState(app, "generate-ready");
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
            iStage = 234; goto LQueued;
        }
        pszFailure = "unimplemented automation instruction"; goto LFail;
    case 225:
        if (app.state.tutor.idtBold == idtBefore && iMsgBefore == app.state.iMsgCur && cWait < 3) return;
        iStage = 224; goto LQueued;
    case 226:
        if (hwndModal) return;
        if (!FSaveOrders(app, "year-2414-terraform-queue.json") || !FPushTextKey(app, SDL_SCANCODE_Q, "q")) goto LInput;
        break;
    case 227:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != ProductionDlg) goto LInput;
        pszMenu = "<Customize>";
        ptRow = (POINT){(app.lpui->rcProdDiamond.left + app.lpui->rcProdDiamond.right) / 2, (app.lpui->rcProdDiamond.top + app.lpui->rcProdDiamond.bottom) / 2};
        if (!FClickPoint(hwndModal, ptRow, SDL_BUTTON_RIGHT)) goto LInput;
        break;
    case 228:
        if (pszMenu || !hwndModal || hwndModal->lpfnDlg != ZipProdDlg) return;
        if (!win32::IsWindow(hwndModal->hwndOwner) || hwndModal->hwndOwner->lpfnDlg != ProductionDlg || win32::IsWindowEnabled(hwndModal->hwndOwner)) goto LInput;
        if (!FSaveState(app, "terraform-template-open") || !FClick(win32::GetDlgItem(hwndModal, IDC_ZIP_PROD_PRESET_1))) goto LInput;
        break;
    case 229:
        if (!hwndModal || hwndModal->lpfnDlg != ZipProdDlg || !win32::IsDlgButtonChecked(hwndModal, IDC_ZIP_PROD_PRESET_1)) return;
        if (!FClick(win32::GetDlgItem(hwndModal, IDC_IMPORT))) goto LInput;
        break;
    case 230:
        if (!hwndModal || hwndModal->lpfnDlg != ZipProdDlg || !app.state.vrgZipProd[0].fValid || app.state.vrgZipProd[0].cpq < 3) return;
        if (app.state.vrgZipProd[0].rgpq[0].mdIdle() != iobjMinTerraform || app.state.vrgZipProd[0].rgpq[0].cQuan() != 2) return;
        if (!FSaveOrders(app, "year-2414-imported-template.json") || !FSaveState(app, "terraform-template-imported") || !FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 231:
        if (!hwndModal || hwndModal->lpfnDlg != ProductionDlg || !win32::IsWindowEnabled(hwndModal)) return;
        if (!FSaveState(app, "terraform-template-owner-restored") || !FClick(win32::GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 232:
        if (hwndModal || app.state.lpplProdGlob) return;
        if (!FSaveState(app, "terraform-template-closed")) goto LArtifact;
        iStage = 224; goto LQueued;
    case 234:
        if (hwndModal) { pszFailure = "unexpected fifteenth-generation dialog"; goto LFail; }
        if (app.state.game.turn == 14) return;
        if (app.state.game.turn != 15 || !app.state.gd.fTutorial || app.state.tutor.idt != 360) { pszFailure = "fifteenth generation did not reach page forty-six"; goto LFail; }
        if (!FSaveState(app, "year-2415")) goto LArtifact;
        if (fReportCycle) {
            if (!FSaveOrders(app, "year-2415-start-orders.json")) goto LArtifact;
            iStage = 235; goto LQueued;
        }
        fPassed = fRejected && cPage == 46;
        LogState(app, "automation-cycle-partial");
        win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
    case 235:
        if (app.state.game.turn != 15) { pszFailure = "unexpected report cycle year"; goto LFail; }
        idtBefore = app.state.tutor.idtBold; iMsgBefore = app.state.iMsgCur;
        LogState(app, "report-cycle-instruction");
        if (app.state.tutor.idtBold == 360 || app.state.tutor.idtBold == 361 || app.state.tutor.idtBold == 372) {
            idflAction = app.state.tutor.idtBold == 360 ? 8 : app.state.tutor.idtBold == 361 ? 6 : 11;
            if (app.state.tutor.idtBold == 372 && IMsgNext(app, 0) != imsgNone) {
                if (!FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
                iStage = 236; goto LQueued;
            }
            if (app.state.sel.grobj != grobjFleet || app.state.sel.id != idflAction) {
                if (app.lpui->mdMsgObj == mdMsgObjFleet && app.lpui->idMsgObj == idflAction) {
                    if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
                } else {
                    if (!FFindMapObject(app, grobjFleet, idflAction, NULL)) goto LInput;
                    pszMenu = objTarget.szName;
                    if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                }
                iStage = 236; goto LQueued;
            }
            if (idflAction == 6) {
                idCargoFleet = 6; idCargoPlanet = 13; cCargoExpected = 210; iCargoReturn = 235; pszCargoSnapshot = "year-2415-cargo.json";
                if (!FClick(app.lpui->rghwndBtn[7])) goto LInput;
                iStage = 205; goto LQueued;
            }
            if (!FFindPlanet(app, idflAction == 8 ? "Neil" : "Stove Top") || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) ||
                !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
            iStage = 236; goto LQueued;
        }
        if (app.state.tutor.idtBold == 363 || app.state.tutor.idtBold == 364) {
            if (!win32::IsWindow(app.lpui->hwndReportDlg) && (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F3, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F3, 0))) goto LInput;
            iStage = 237; goto LQueued;
        }
        if (app.state.tutor.idtBold == 366) {
            if (win32::IsWindow(app.lpui->hwndReportDlg) && (!FPushKey(app.lpui->hwndReportDlg, SDL_SCANCODE_ESCAPE, 1) || !FPushKey(app.lpui->hwndReportDlg, SDL_SCANCODE_ESCAPE, 0))) goto LInput;
            iStage = 240; goto LQueued;
        }
        if (app.state.tutor.idtBold == 367) {
            lpfl = LpflFromId(app, 6);
            if (!lpfl || app.state.sel.grobj != grobjFleet || app.state.sel.id != 6) goto LInput;
            if (lpfl->cord == 1) {
                if (!FFindPlanet(app, "Wallaby") || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) ||
                    !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
                iStage = 236; goto LQueued;
            }
            iOrderCombo = lpfl->lpplord->rgord()[1].grTask != grTaskXfer ? 0 : 1;
            iOrderRow = iOrderCombo == 0 ? grTaskXfer : 4; iOrderReturn = 235; pszOrderSnapshot = "year-2415-transport.json";
            if (!FClick(app.lpui->rghwndOrderDD[iOrderCombo])) goto LInput;
            iStage = 209; goto LQueued;
        }
        if (app.state.tutor.idtBold == 368) {
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F5, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F5, 0)) goto LInput;
            iStage = 241; goto LQueued;
        }
        if (app.state.tutor.idtBold == 371) {
            if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg || !FClick(win32::GetDlgItem(hwndModal, IDCANCEL))) goto LInput;
            iStage = 242; goto LQueued;
        }
        if (app.state.tutor.idtBold == 374) {
            if (!fValueReportChecked || !app.state.tutor.fTurnDone || !FSaveOrders(app, "year-2415-orders.json") || !FSaveState(app, "before-generate-2415")) goto LArtifact;
            LogState(app, "generate-ready");
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
            iStage = 243; goto LQueued;
        }
        pszFailure = "unimplemented report cycle instruction"; goto LFail;
    case 236:
        if (pszMenu || (idtBefore == app.state.tutor.idtBold && iMsgBefore == app.state.iMsgCur && cWait < 3)) return;
        iStage = 235; goto LQueued;
    case 237:
        if (!win32::IsWindow(app.lpui->hwndReportDlg) || !app.lpui->vprptCur) return;
        if (app.lpui->vprptCur->irpt != rptPlanets) goto LInput;
        if (!win32::GetClientRect(app.lpui->hwndReportDlg, &rcRow)) goto LInput;
        if (rcRow.right < 1000) {
            if (!ui::SetWindowPos(app.lpui->hwndReportDlg, NULL, 0, 0, 1280, 700, win32::SWP_NOZORDER)) goto LInput;
            return;
        }
        if (!FSaveState(app, "value-report-open")) goto LArtifact;
        ptRow = (POINT){2, (app.state.dyArial8 + 8) / 2};
        for (i = 0; i < 4; i++) {
            if ((app.lpui->vprptCur->grbitVisible & (1 << i)) && (i == 0 || i >= app.lpui->vprptCur->cFieldFirst)) ptRow.x += app.lpui->vprptCur->rgbdx[i] * 2;
        }
        ptRow.x += app.lpui->vprptCur->rgbdx[4];
        pszMenu = "Sort by Value";
        if (!FClickPoint(app.lpui->hwndReportDlg, ptRow, SDL_BUTTON_RIGHT)) goto LInput;
        break;
    case 238:
        if (pszMenu || !app.lpui->vprptCur || app.lpui->vprptCur->icolSort != 4) return;
        if (!FSaveReport(app, "value-report.json") || !FSaveState(app, "value-report-sorted") ||
            !FPushKey(app.lpui->hwndReportDlg, SDL_SCANCODE_ESCAPE, 1) || !FPushKey(app.lpui->hwndReportDlg, SDL_SCANCODE_ESCAPE, 0)) goto LInput;
        iStage = 240; goto LQueued;
    case 240:
        if (win32::IsWindow(app.lpui->hwndReportDlg) || app.lpui->vprptCur) return;
        if (!FSaveState(app, "value-report-closed")) goto LArtifact;
        fValueReportChecked = 1;
        iStage = 235; goto LQueued;
    case 241:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != ResearchDlg || !FSaveState(app, "propulsion-research-open")) goto LInput;
        iStage = 235; goto LQueued;
    case 242:
        if (hwndModal) return;
        if (!FSaveOrders(app, "year-2415-research.json")) goto LArtifact;
        iStage = 235; goto LQueued;
    case 243:
        if (hwndModal) { pszFailure = "unexpected sixteenth-generation dialog"; goto LFail; }
        if (app.state.game.turn == 15) return;
        if (app.state.game.turn != 16 || !app.state.gd.fTutorial || app.state.tutor.idt != 376) { pszFailure = "sixteenth generation did not reach page forty-eight"; goto LFail; }
        if (!FSaveState(app, "year-2416")) goto LArtifact;
        if (fMergeCycle) {
            if (!FSaveOrders(app, "year-2416-start-orders.json")) goto LArtifact;
            iStage = 244; goto LQueued;
        }
        fPassed = fRejected && cPage == 48;
        LogState(app, "report-cycle-partial");
        win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
    case 244:
        if (app.state.game.turn != 16 && app.state.game.turn != 17) { pszFailure = "unexpected merge cycle year"; goto LFail; }
        idtBefore = app.state.tutor.idtBold; iMsgBefore = app.state.iMsgCur;
        LogState(app, "merge-cycle-instruction");
        if (app.state.tutor.idtBold == 376 || app.state.tutor.idtBold == 377 || app.state.tutor.idtBold == 378 || app.state.tutor.idtBold == 385) {
            idflAction = app.state.tutor.idtBold == 376 ? 4 : app.state.tutor.idtBold == 385 ? 0 : 7;
            if (app.state.sel.grobj != grobjFleet || app.state.sel.id != idflAction) {
                if (app.lpui->mdMsgObj == mdMsgObjFleet && app.lpui->idMsgObj == idflAction) {
                    if (!FClick(app.lpui->rghwndMsgBtn[1])) goto LInput;
                } else {
                    if (!FFindMapObject(app, grobjFleet, idflAction, NULL)) goto LInput;
                    pszMenu = objTarget.szName;
                    if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                }
                iStage = 245; goto LQueued;
            }
            lpfl = LpflFromId(app, idflAction);
            if (!lpfl) goto LInput;
            if (lpfl->cord == 1) {
                if (!FFindPlanet(app, idflAction == 4 ? "Wallaby" : idflAction == 0 ? "Stove Top" : "Prune") ||
                    !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 1) || !FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) ||
                    !FPushKey(app.lpui->hwndScanner, SDL_SCANCODE_LSHIFT, 0)) goto LInput;
                iStage = 245; goto LQueued;
            }
            if (idflAction == 7) {
                if (lpfl->lpplord->rgord()[1].grTask != grTaskMerge) {
                    iOrderCombo = 0; iOrderRow = grTaskMerge; iOrderReturn = 244; pszOrderSnapshot = "year-2416-merge.json";
                    if (!FClick(app.lpui->rghwndOrderDD[0])) goto LInput;
                    iStage = 209; goto LQueued;
                }
                if (lpfl->lpplord->rgord()[1].grobj != grobjFleet || lpfl->lpplord->rgord()[1].id != 5) {
                    pszMenu = "Cotton Picker #6";
                    ptRow = (POINT){(app.lpui->rgrcRef[12].left + app.lpui->rgrcRef[12].right) / 2, (app.lpui->rgrcRef[12].top + app.lpui->rgrcRef[12].bottom) / 2};
                    if (!FClickPoint(app.lpui->hwndPlanet, ptRow, SDL_BUTTON_RIGHT)) goto LInput;
                    iStage = 245; goto LQueued;
                }
            }
            iStage = 245; goto LQueued;
        }
        if (app.state.tutor.idtBold == 380 || app.state.tutor.idtBold == 381) {
            if (app.state.sel.grobj != grobjPlanet || app.state.sel.id != 13) {
                if (!FFindPlanet(app, "Stove Top")) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(app.lpui->hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 245; goto LQueued;
            }
            iProdPass = 17; cProdAdded = 0; iProdReturn = 246;
            if (LpplFromId(app, 13)->lpplprod && LpplFromId(app, 13)->lpplprod->iprodMac > 0 &&
                LpplFromId(app, 13)->lpplprod->rgprod()[0].grobj == grobjPlanet && LpplFromId(app, 13)->lpplprod->rgprod()[0].iItem == iobjFactory)
                cProdAdded = LpplFromId(app, 13)->lpplprod->rgprod()[0].cItem;
            if (!FPushTextKey(app, SDL_SCANCODE_Q, "q")) goto LInput;
            iStage = 68; goto LQueued;
        }
        if (app.state.tutor.idtBold == 382) {
            if (!hwndModal) {
                if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F5, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F5, 0)) goto LInput;
                iStage = 245; goto LQueued;
            }
            if (hwndModal->lpfnDlg != ResearchDlg || !FChooseCombo(win32::GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD), "Construction")) goto LInput;
            iResearchNext = Construction; iResearchReturn = 244; pszResearchSnapshot = "year-2416-research.json";
            iDesignReturn = 211; iStage = 221; goto LQueued;
        }
        if (app.state.tutor.idtBold == 384 || app.state.tutor.idtBold == 383 || app.state.tutor.idtBold == 387) {
            if (IMsgNext(app, 0) != imsgNone) {
                if (!FClick(app.lpui->rghwndMsgBtn[2])) goto LInput;
                iStage = 245; goto LQueued;
            }
            if (app.state.tutor.idtBold == 384) { iStage = 245; goto LQueued; }
            if (!app.state.tutor.fTurnDone) return;
            if (!FSaveOrders(app, app.state.game.turn == 16 ? "year-2416-orders.json" : "year-2417-orders.json") ||
                !FSaveState(app, app.state.game.turn == 16 ? "before-generate-2416" : "before-generate-2417")) goto LArtifact;
            LogState(app, "generate-ready");
            if (!FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 1) || !FPushKey(app.lpui->hwndFrame, SDL_SCANCODE_F9, 0)) goto LInput;
            iStage = app.state.game.turn == 16 ? 247 : 248; goto LQueued;
        }
        pszFailure = "unimplemented merge cycle instruction"; goto LFail;
    case 245:
        if (pszMenu || (idtBefore == app.state.tutor.idtBold && iMsgBefore == app.state.iMsgCur && cWait < 3)) return;
        iStage = 244; goto LQueued;
    case 246:
        if (hwndModal) return;
        if (iProdPass == 17) {
            iProdPass = 18; cProdAdded = 0; iProdReturn = 246;
            if (!FPushTextKey(app, SDL_SCANCODE_Q, "q")) goto LInput;
            iStage = 68; goto LQueued;
        }
        if (!FSaveOrders(app, "year-2416-production.json")) goto LArtifact;
        iStage = 244; goto LQueued;
    case 247:
        if (hwndModal) { pszFailure = "unexpected seventeenth-generation dialog"; goto LFail; }
        if (app.state.game.turn == 16) return;
        if (app.state.game.turn != 17 || !app.state.gd.fTutorial || app.state.tutor.idt != 384) { pszFailure = "seventeenth generation did not reach page forty-nine"; goto LFail; }
        if (!FSaveState(app, "year-2417") || !FSaveOrders(app, "year-2417-start-orders.json")) goto LArtifact;
        iStage = 244; goto LQueued;
    case 248:
        if (hwndModal) { pszFailure = "unexpected eighteenth-generation dialog"; goto LFail; }
        if (app.state.game.turn == 17) return;
        if (app.state.game.turn != 18 || !app.state.gd.fTutorial || app.state.tutor.idt != 392) { pszFailure = "eighteenth generation did not reach page fifty"; goto LFail; }
        if (!FSaveState(app, "year-2418")) goto LArtifact;
        fPassed = fRejected && cPage == 50;
        LogState(app, "merge-cycle-partial");
        win32::KillTimer(NULL, id); win32::PostQuitMessage(fPassed ? 0 : 93); return;
    default:
        pszFailure = "unknown runner stage";
        goto LFail;
    }
    LogState(app, "action");
    iStage++;
LQueued:
    cWait = 0;
    return;
LInput:
    pszFailure = "unable to enqueue real UI input";
    goto LFail;
LArtifact:
    pszFailure = "unable to save inspection artifact";
LFail:
    LogState(app, "failed");
    FSaveState(app, "failure");
    fprintf(stderr, "%s at stage %d, instruction %d\n", pszFailure, iStage, app.state.tutor.idtBold);
    win32::KillTimer(NULL, id);
    win32::PostQuitMessage(93);
}

int16_t StarsTest_InitInstance(APPSESSION &app, int16_t nCmdShow) {
    int16_t fResult = InitInstance(app, nCmdShow);
    if (fResult) idTimer = win32::SetTimer(NULL, 0, 50, RunTutorial);
    return fResult && idTimer ? fResult : 0;
}

int main(int argc, char **argv) {
    char szCommand[] = "";
    int nResult;
    NATIVEFILE *lpfile;
    DESKTOPCHOICEIO io_desktopchoiceio;
    io_desktopchoiceio.pfnInitInstance = StarsTest_InitInstance;
    DESKTOPCHOICEIOSCOPE scope_desktopchoiceio(io_desktopchoiceio);
    COMPATCHOICEIO io_compatchoiceio;
    io_compatchoiceio.pfnTrackPopupMenu = StarsTest_TrackPopupMenu;
    io_compatchoiceio.pfnMessageBoxA = StarsTest_MessageBoxA;
    COMPATCHOICEIOSCOPE scope_compatchoiceio(io_compatchoiceio);

    if (argc != 2 || (strcmp(argv[1], "--smoke") && strcmp(argv[1], "--first-year") && strcmp(argv[1], "--two-years") && strcmp(argv[1], "--scouting") && strcmp(argv[1], "--mining") && strcmp(argv[1], "--colonization") && strcmp(argv[1], "--logistics") && strcmp(argv[1], "--colonizer-production") && strcmp(argv[1], "--second-colonization") && strcmp(argv[1], "--transport-cycle") && strcmp(argv[1], "--expansion") && strcmp(argv[1], "--fleet-split") && strcmp(argv[1], "--research-cycle") && strcmp(argv[1], "--oxygen-colonization") && strcmp(argv[1], "--production-templates") && strcmp(argv[1], "--battle-cycle") && strcmp(argv[1], "--terraform-cycle") && strcmp(argv[1], "--design-cycle") && strcmp(argv[1], "--automation-cycle") && strcmp(argv[1], "--report-cycle") && strcmp(argv[1], "--merge-cycle"))) {
        fprintf(stderr, "Usage: stars-native-tutorial --smoke|--first-year|--two-years|--scouting|--mining|--colonization|--logistics|--colonizer-production|--second-colonization|--transport-cycle|--expansion|--fleet-split|--research-cycle|--oxygen-colonization|--production-templates|--battle-cycle|--terraform-cycle|--design-cycle|--automation-cycle|--report-cycle|--merge-cycle\nFull 80-page execution is not implemented yet.\n");
        return 2;
    }
    fMergeCycle = !strcmp(argv[1], "--merge-cycle");
    fReportCycle = fMergeCycle || !strcmp(argv[1], "--report-cycle");
    fAutomationCycle = fReportCycle || !strcmp(argv[1], "--automation-cycle");
    fDesignCycle = fAutomationCycle || !strcmp(argv[1], "--design-cycle");
    fTerraformCycle = fDesignCycle || !strcmp(argv[1], "--terraform-cycle");
    fBattleCycle = fTerraformCycle || !strcmp(argv[1], "--battle-cycle");
    fProdTemplates = fBattleCycle || !strcmp(argv[1], "--production-templates");
    fOxygenColony = fProdTemplates || !strcmp(argv[1], "--oxygen-colonization");
    fResearchCycle = fOxygenColony || !strcmp(argv[1], "--research-cycle");
    fFleetSplit = fResearchCycle || !strcmp(argv[1], "--fleet-split");
    fExpansion = fFleetSplit || !strcmp(argv[1], "--expansion");
    fTransportCycle = fExpansion || !strcmp(argv[1], "--transport-cycle");
    fSecondColonization = fTransportCycle || !strcmp(argv[1], "--second-colonization");
    fColonizerProduction = fSecondColonization || !strcmp(argv[1], "--colonizer-production");
    fLogistics = fColonizerProduction || !strcmp(argv[1], "--logistics");
    fColonization = fLogistics || !strcmp(argv[1], "--colonization");
    fMining = fColonization || !strcmp(argv[1], "--mining");
    fScouting = fMining || !strcmp(argv[1], "--scouting");
    fTwoYears = fScouting || !strcmp(argv[1], "--two-years");
    fFirstYear = fTwoYears || !strcmp(argv[1], "--first-year");
    lpEvents = PfNativeOpen("events.jsonl", NATIVEFILEMODE::writeTruncate);
    if (!lpEvents) return 1;
    setvbuf(stdout, NULL, _IONBF, 0);
    nResult = ui::WinMain(win32::GetModuleHandleA(NULL), NULL, szCommand, win32::SW_SHOWNORMAL);
    if (idTimer) win32::KillTimer(NULL, idTimer);
    lpfile = PfNativeOpen("result.json", NATIVEFILEMODE::writeTruncate);
    if (lpfile) {
        CchNativePrintf(lpfile, "{\"scenario\":\"%s\",\"status\":\"%s\",\"fullTutorial\":false,\"actions\":%d,\"pages\":%d,\"rejectedGenerate\":%s,\"detail\":\"%s\"}\n",
                fMergeCycle ? "merge-cycle" : fReportCycle ? "report-cycle" : fAutomationCycle ? "automation-cycle" : fDesignCycle ? "design-cycle" : fTerraformCycle ? "terraform-cycle" : fBattleCycle ? "battle-cycle" : fProdTemplates ? "production-templates" : fOxygenColony ? "oxygen-colonization" : fResearchCycle ? "research-cycle" : fFleetSplit ? "fleet-split" : fExpansion ? "expansion" : fTransportCycle ? "transport-cycle" : fSecondColonization ? "second-colonization" : fColonizerProduction ? "colonizer-production" : fLogistics ? "logistics" : fColonization ? "colonization" : fMining ? "mining" : fScouting ? "scouting" : fTwoYears ? "two-years" : fFirstYear ? "first-year" : "smoke", fPassed && nResult == 0 ? (fFirstYear ? "partial" : "passed") : "failed",
                cActions, cPage, fRejected ? "true" : "false",
                fPassed ? "bounded scenario; complete tutorial remains unverified" : pszFailure);
        if (ICloseNativeFile(lpfile)) fPassed = 0;
    } else fPassed = 0;
    ICloseNativeFile(lpEvents);
    SDL_SetModState(SDL_KMOD_NONE);
    StarsCompatProcessExit();
    SDL_Quit();
    return fPassed && nResult == 0 ? (fFirstYear ? 2 : 0) : 1;
}
