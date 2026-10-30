/* STARS_TEST_TUTORIAL: drive the actual SDL client; never complete tasks by
 * assigning tutorial or game state. The smoke scenario is deliberately bounded.
 */
#include "win.h"
#include <stars_compat.h>
#include "internal.h"
#include <stdio.h>
#include <stdlib.h>

int16_t StarsTestReal_InitInstance(int16_t nCmdShow);
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
static BOOL FSaveState(const char *pszName);

BOOL StarsTestReal_TrackPopupMenu(HMENU hmenu, UINT flags, int x, int y, int reserved, HWND hwnd, const RECT *prc);
BOOL StarsTest_TrackPopupMenu(HMENU hmenu, UINT flags, int x, int y, int reserved, HWND hwnd, const RECT *prc) {
    SDL_Event event = {0};
    HWND hwndRoot = HwndCompatRoot(hwnd);
    char szText[256];
    int i, j, iItem = -1, cSteps = 0;
    if (!pszMenu) return StarsTestReal_TrackPopupMenu(hmenu, flags, x, y, reserved, hwnd, prc);
    /* PopupMenu sends commands to the frame even when a modal owns the input. */
    if (hwndRoot && !IsWindowEnabled(hwndRoot)) hwndRoot = HwndCompatRoot(HwndModal());
    for (i = 0; i < GetMenuItemCount(hmenu); i++) {
        if (GetMenuState(hmenu, i, MF_BYPOSITION) & MF_SEPARATOR) continue;
        GetMenuString(hmenu, i, szText, sizeof(szText), MF_BYPOSITION);
        if (!strcmp(szText, pszMenu)) { iItem = cSteps; break; }
        cSteps++;
    }
    if (iItem < 0) {
        fprintf(stderr, "Requested scanner menu item '%s' was absent; entries:\n", pszMenu);
        for (i = 0; i < GetMenuItemCount(hmenu); i++) {
            GetMenuString(hmenu, i, szText, sizeof(szText), MF_BYPOSITION);
            fprintf(stderr, "  %d: '%s'\n", i, szText);
        }
        FSaveState("missing-menu");
    }
    pszMenu = NULL;
    if (iItem < 0 || !hwndRoot) { pszFailure = "requested scanner menu item was absent"; PostQuitMessage(93); return FALSE; }
    for (j = 0; j < iItem + 2; j++) {
        event.type = SDL_EVENT_KEY_DOWN; event.key.windowID = hwndRoot->id;
        event.key.scancode = j == 0 ? SDL_SCANCODE_HOME : j == iItem + 1 ? SDL_SCANCODE_RETURN : SDL_SCANCODE_DOWN;
        event.key.key = SDL_GetKeyFromScancode(event.key.scancode, SDL_KMOD_NONE, false); event.key.down = true;
        if (!SDL_PushEvent(&event)) return FALSE;
        event.type = SDL_EVENT_KEY_UP; event.key.down = false;
        if (!SDL_PushEvent(&event)) return FALSE;
    }
    return StarsTestReal_TrackPopupMenu(hmenu, flags, x, y, reserved, hwnd, prc);
}

int WINAPI StarsTestReal_MessageBoxA(HWND hwnd, LPCSTR pszText, LPCSTR pszCaption, UINT type);
int WINAPI StarsTest_MessageBoxA(HWND hwnd, LPCSTR pszText, LPCSTR pszCaption, UINT type) {
    /* Observe text at the boundary; the real message box still opens and its
     * real OK control is clicked. Its owner-drawn body has no WM_GETTEXT text.
     */
    fExpectedRejection = iStage == 3 && pszText &&
        strstr(pszText, "You have not yet completed all of the tutorial tasks");
    fprintf(stderr, "Message box at stage %d: %s\n", iStage, pszText ? pszText : "");
    return StarsTestReal_MessageBoxA(hwnd, pszText, pszCaption, type);
}

static void LogState(const char *pszEvent) {
    if (!lpEvents) return;
    CchNativePrintf(lpEvents, "{\"event\":\"%s\",\"stage\":%d,\"turn\":%u,\"page\":%d,\"instruction\":%d,\"active\":%d,\"autoComplete\":%d,\"actions\":%d,\"elapsedMs\":%llu,\"selection\":[%d,%d],\"turnDone\":%d,\"scannerMode\":%u,\"productionSelection\":%ld}\n",
            pszEvent, iStage, game.turn, tutor.idt / 8 + 1, tutor.idtBold, gd.fTutorial, tutor.fAutoComplete, cActions, (unsigned long long)SDL_GetTicks(),
            sel.grobj, sel.id, tutor.fTurnDone, grbitScan & grbitScanViewMask,
            IsWindow(hwndPlanetProdLB) ? (long)SendMessage(hwndPlanetProdLB, LB_GETCURSEL, 0, 0) : -1L);
    IFlushNativeFile(lpEvents);
}

static BOOL FSaveState(const char *pszName) {
    STARSCOMPATSNAPSHOT snap = {0};
    SCANSNAPSHOT scan = {0};
    NATIVEFILE *lpfile;
    char szPath[100];
    BOOL fResult;

    snprintf(szPath, sizeof(szPath), "%s-hierarchy.json", pszName);
    if (!StarsCompatCaptureHierarchy(&snap)) return FALSE;
    lpfile = PfNativeOpen(szPath, "w");
    fResult = lpfile && StarsCompatWriteHierarchy(&snap, lpfile);
    if (lpfile && ICloseNativeFile(lpfile)) fResult = FALSE;
    StarsCompatFreeHierarchy(&snap);
    if (!fResult || !IsWindow(hwndScanner)) return fResult;
    snprintf(szPath, sizeof(szPath), "%s-map.json", pszName);
    if (!FCaptureScanner(&scan)) return FALSE;
    lpfile = PfNativeOpen(szPath, "w");
    fResult = lpfile && FWriteScannerSnapshot(&scan, lpfile);
    if (lpfile && ICloseNativeFile(lpfile)) fResult = FALSE;
    FreeScannerSnapshot(&scan);
    return fResult;
}

static BOOL FPushKey(HWND hwnd, SDL_Scancode sc, BOOL fDown) {
    SDL_Event event = {0};
    HWND hwndRoot = HwndCompatRoot(hwnd);
    // SDL_Event is a union; zero all bytes, including inactive-member fields.
    SDL_zero(event);
    if (!hwndRoot) return FALSE;
    event.type = fDown ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.windowID = hwndRoot->id;
    event.key.scancode = sc;
    event.key.key = SDL_GetKeyFromScancode(sc, SDL_KMOD_NONE, false);
    event.key.down = fDown;
    return SDL_PushEvent(&event);
}

static BOOL FPushTextKey(SDL_Scancode sc, const char *pszText) {
    SDL_Event event = {0};
    HWND hwndRoot = HwndCompatRoot(hwndFrame);
    SDL_zero(event);
    if (!hwndRoot || !FPushKey(hwndFrame, sc, TRUE)) return FALSE;
    /* SDL supplies printable characters separately from physical key events. */
    event.type = SDL_EVENT_TEXT_INPUT;
    event.text.windowID = hwndRoot->id;
    event.text.text = pszText;
    return SDL_PushEvent(&event) && FPushKey(hwndFrame, sc, FALSE);
}

static BOOL FMousePoint(HWND hwnd, POINT pt, Uint8 iButton, BOOL fDown) {
    HWND hwndRoot = HwndCompatRoot(hwnd);
    SDL_Event event = {0};
    SDL_zero(event);
    if (!hwndRoot || !IsWindowVisible(hwnd) || !IsWindowEnabled(hwnd)) return FALSE;
    ClientToScreen(hwnd, &pt);
    ScreenToClient(hwndRoot, &pt);
    event.type = fDown ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.windowID = hwndRoot->id;
    event.button.button = iButton;
    event.button.down = fDown;
    event.button.x = pt.x;
    event.button.y = pt.y + CyCompatMenuBar(hwndRoot);
    return SDL_PushEvent(&event);
}

static BOOL FClickPoint(HWND hwnd, POINT pt, Uint8 iButton) {
    if (!FMousePoint(hwnd, pt, iButton, TRUE)) return FALSE;
    cActions++;
    return FMousePoint(hwnd, pt, iButton, FALSE);
}

static BOOL FClick(HWND hwnd) {
    RECT rc;
    POINT pt;
    if (!IsWindow(hwnd) || !GetClientRect(hwnd, &rc)) return FALSE;
    pt.x = (rc.right - rc.left) / 2;
    pt.y = (rc.bottom - rc.top) / 2;
    return FClickPoint(hwnd, pt, SDL_BUTTON_LEFT);
}

static BOOL FClickComboRow(HWND hwndCombo, int iRow) {
    HWND hwndDrop = hwndCombo->lpcombo ? hwndCombo->lpcombo->hwndPopup : NULL;
    RECT rc;
    POINT pt;
    int cy, iTop;
    if (!hwndDrop || !GetClientRect(hwndDrop, &rc)) return FALSE;
    cy = (int)CompatListProc(hwndCombo, LB_GETITEMHEIGHT, 0, 0);
    iTop = (int)CompatListProc(hwndCombo, LB_GETTOPINDEX, 0, 0);
    pt = (POINT){rc.right / 2, 1 + (iRow - iTop) * cy + cy / 2};
    return cy > 0 && pt.y >= 1 && pt.y < rc.bottom - 1 && FClickPoint(hwndDrop, pt, SDL_BUTTON_LEFT);
}

static BOOL FChooseCombo(HWND hwndCombo, const char *pszName) {
    char szItem[256];
    int i;
    for (i = 0; i < SendMessage(hwndCombo, CB_GETCOUNT, 0, 0); i++) {
        if (SendMessage(hwndCombo, CB_GETLBTEXTLEN, i, 0) >= (LRESULT)sizeof(szItem)) return FALSE;
        SendMessage(hwndCombo, CB_GETLBTEXT, i, (LPARAM)szItem);
        if (!strcmp(szItem, pszName)) break;
    }
    if (i == SendMessage(hwndCombo, CB_GETCOUNT, 0, 0) || !FClick(hwndCombo)) return FALSE;
    // Commit by mouse after focus and keyboard navigation have settled.
    hwndDesignCombo = hwndCombo;
    iDesignCombo = i;
    return TRUE;
}

static BOOL FDragPoint(HWND hwnd, POINT ptFrom, POINT ptTo) {
    HWND hwndRoot = HwndCompatRoot(hwnd);
    SDL_Event event = {0};
    if (!hwndRoot || !ClientToScreen(hwnd, &ptFrom) || !ScreenToClient(hwndRoot, &ptFrom) ||
        !ClientToScreen(hwnd, &ptTo) || !ScreenToClient(hwndRoot, &ptTo)) return FALSE;
    ptFrom.y += CyCompatMenuBar(hwndRoot); ptTo.y += CyCompatMenuBar(hwndRoot);
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.windowID = hwndRoot->id;
    event.button.button = SDL_BUTTON_LEFT; event.button.down = true;
    event.button.x = ptFrom.x; event.button.y = ptFrom.y;
    if (!SDL_PushEvent(&event)) return FALSE;
    event = (SDL_Event){0};
    event.type = SDL_EVENT_MOUSE_MOTION; event.motion.windowID = hwndRoot->id;
    event.motion.state = SDL_BUTTON_LMASK; event.motion.x = ptTo.x; event.motion.y = ptTo.y;
    if (!SDL_PushEvent(&event)) return FALSE;
    event = (SDL_Event){0};
    event.type = SDL_EVENT_MOUSE_BUTTON_UP; event.button.windowID = hwndRoot->id;
    event.button.button = SDL_BUTTON_LEFT; event.button.x = ptTo.x; event.button.y = ptTo.y;
    cActions++;
    return SDL_PushEvent(&event);
}

/* Resolve a fresh hierarchy every time; a callback can destroy the old dialog. */
static HWND HwndModal(void) {
    STARSCOMPATSNAPSHOT snap = {0};
    HWND hwnd = NULL, hwndT;
    size_t i;
    if (!StarsCompatCaptureHierarchy(&snap)) return NULL;
    for (i = 0; i < snap.cWindow; i++) {
        hwndT = StarsCompatResolveWindow(snap.rgWindow[i].id);
        /* Snapshots list older windows first; a nested modal disables its owner. */
        if (hwndT && hwndT->fModal && snap.rgWindow[i].fVisible && IsWindowEnabled(hwndT)) hwnd = hwndT;
    }
    StarsCompatFreeHierarchy(&snap);
    return hwnd;
}

static BOOL FFindMapObject(uint16_t grType, uint16_t idObject, const char *pszName) {
    SCANSNAPSHOT snap = {0};
    size_t i;
    BOOL fFound = FALSE, fReveal = FALSE;
    if (!FCaptureScanner(&snap)) return FALSE;
    for (i = 0; i < snap.cObject; i++) {
        if (snap.rgobj[i].grType == grType && (pszName ? !strcmp(snap.rgobj[i].szName, pszName) : snap.rgobj[i].id == idObject) && !snap.rgobj[i].fFiltered) {
            objTarget = snap.rgobj[i];
            if (!objTarget.fOnScreen) {
                fReveal = FSelectScannerObject(&snap, grType, objTarget.id, TRUE);
                break;
            }
            fFound = TRUE;
            break;
        }
    }
    FreeScannerSnapshot(&snap);
    if (fReveal) {
        LogState("map-reveal");
        if (!FCaptureScanner(&snap)) return FALSE;
        for (i = 0; i < snap.cObject; i++) {
            if (snap.rgobj[i].grType == grType && snap.rgobj[i].id == objTarget.id && snap.rgobj[i].fOnScreen) {
                objTarget = snap.rgobj[i];
                fFound = TRUE;
                break;
            }
        }
        FreeScannerSnapshot(&snap);
    }
    return fFound;
}

static BOOL FFindPlanet(const char *pszName) {
    return FFindMapObject(grobjPlanet, 0, pszName);
}

static BOOL FFindSalvageHere(void) {
    SCANSNAPSHOT snap = {0};
    POINT pt = objTarget.ptUniverse;
    char szName[64];
    size_t i;
    BOOL fFound = FALSE;

    CchGetString(idsSalvage, szName);
    if (!FCaptureScanner(&snap)) return FALSE;
    for (i = 0; i < snap.cObject; i++) {
        if (snap.rgobj[i].grType == grobjThing && snap.rgobj[i].ith == ithMineralPacket &&
            !snap.rgobj[i].fFiltered && snap.rgobj[i].ptUniverse.x == pt.x && snap.rgobj[i].ptUniverse.y == pt.y &&
            !strcmp(snap.rgobj[i].szName, szName)) {
            objTarget = snap.rgobj[i]; fFound = TRUE; break;
        }
    }
    FreeScannerSnapshot(&snap);
    return fFound;
}

static BOOL FPushControlKey(HWND hwnd, SDL_Scancode sc) {
    SDL_Event event = {0};
    HWND hwndRoot = HwndCompatRoot(hwnd);

    if (!hwndRoot || !FPushKey(hwnd, SDL_SCANCODE_LCTRL, TRUE)) return FALSE;
    event.type = SDL_EVENT_KEY_DOWN; event.key.windowID = hwndRoot->id;
    event.key.scancode = sc; event.key.mod = SDL_KMOD_LCTRL;
    event.key.key = SDL_GetKeyFromScancode(sc, event.key.mod, false); event.key.down = true;
    if (!SDL_PushEvent(&event)) return FALSE;
    event.type = SDL_EVENT_KEY_UP; event.key.down = false;
    if (!SDL_PushEvent(&event) || !FPushKey(hwnd, SDL_SCANCODE_LCTRL, FALSE)) return FALSE;
    cActions++;
    return TRUE;
}

static BOOL FSaveOrders(const char *pszPath) {
    NATIVEFILE *lpfile;
    FLEET *lpfl;
    ORDER *lpord;
    PLANET *lppl;
    PROD prod;
    SHDEF *lpshdef;
    int i, j, k, cWritten = 0;
    lpfile = PfNativeOpen(pszPath, "w");
    if (!lpfile) return FALSE;
    CchNativePrintf(lpfile, "{\"schema\":1,\"turn\":%u,\"turnDone\":%d,\"research\":[%u,%u,%d],\"scannerMode\":%u,\"nextMessage\":%d,\"filteredFactories\":%s,\"filteredMines\":%s,\"filteredUnloads\":%s,\"filteredMiningLoads\":%s,\"filteredDismantles\":%s,\"ownedFleetCount\":%u,\"fleets\":[",
            game.turn, tutor.fTurnDone, rgplr[0].iTechNow(), rgplr[0].iTechNext(), rgplr[0].pctResearch, grbitScan & grbitScanViewMask, IMsgNext(FALSE),
            bitfMsgFiltered[idmHaveBuiltFactories >> 3] & (1 << (idmHaveBuiltFactories & 7)) ? "true" : "false",
            bitfMsgFiltered[idmHaveBuiltMines >> 3] & (1 << (idmHaveBuiltMines & 7)) ? "true" : "false",
            bitfMsgFiltered[idmHasUnloaded >> 3] & (1 << (idmHasUnloaded & 7)) ? "true" : "false",
            bitfMsgFiltered[idmHasLoadedMiningRobotsWorking >> 3] & (1 << (idmHasLoadedMiningRobotsWorking & 7)) ? "true" : "false",
            bitfMsgFiltered[idmHasDismantledKtMineralsWhichHaveDeposited >> 3] & (1 << (idmHasDismantledKtMineralsWhichHaveDeposited & 7)) ? "true" : "false", rgplr[idPlayer].cFleet);
    for (i = 0; i < cFleet; i++) {
        lpfl = rglpfl[i];
        if (!lpfl || lpfl->fDead || lpfl->iPlayer != idPlayer) continue;
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
        lpshdef = &rglpshdef[idPlayer][i];
        if (lpshdef->fFree) continue;
        CchNativePrintf(lpfile, "%s{\"index\":%d,\"hull\":%u,\"parts\":[", cWritten++ ? "," : "", i, lpshdef->hul.ihuldef);
        for (j = 0; j < lpshdef->hul.chs; j++)
            CchNativePrintf(lpfile, "%s[%u,%u,%u]", j ? "," : "", lpshdef->hul.rghs[j].grhst, lpshdef->hul.rghs[j].iItem, lpshdef->hul.rghs[j].cItem);
        IPutNativeString("]}", lpfile);
    }
    IPutNativeString("],\"production\":[", lpfile);
    cWritten = 0;
    for (i = 0; i < cPlanet; i++) {
        lppl = &lpPlanets[i];
        if (lppl->iPlayer != idPlayer || !lppl->lpplprod) continue;
        for (j = 0; j < lppl->lpplprod->iprodMac; j++) {
            prod = lppl->lpplprod->rgprod()[j];
            CchNativePrintf(lpfile, "%s{\"planet\":%u,\"kind\":%u,\"item\":%u,\"quantity\":%u}",
                    cWritten++ ? "," : "", lppl->id, prod.grobj, prod.iItem, prod.cItem);
        }
    }
    IPutNativeString("],\"planetFlags\":[", lpfile);
    cWritten = 0;
    for (i = 0; i < cPlanet; i++) {
        lppl = &lpPlanets[i];
        if (lppl->iPlayer != idPlayer) continue;
        CchNativePrintf(lpfile, "%s{\"planet\":%u,\"leftoverResearch\":%u}",
                cWritten++ ? "," : "", lppl->id, lppl->fNoResearch);
    }
    CchNativePrintf(lpfile, "],\"defaultTemplate\":{\"valid\":%s,\"leftoverResearch\":%u,\"count\":%u,\"items\":[",
            vrgZipProd[0].fValid ? "true" : "false", vrgZipProd[0].fNoResearch, vrgZipProd[0].cpq);
    for (j = 0; j < vrgZipProd[0].cpq && j < 12; j++)
        CchNativePrintf(lpfile, "%s{\"item\":%u,\"quantity\":%u}", j ? "," : "", vrgZipProd[0].rgpq[j].mdIdle(), vrgZipProd[0].rgpq[j].cQuan());
    IPutNativeString("]}}\n", lpfile);
    i = FNativeStreamError(lpfile);
    return ICloseNativeFile(lpfile) == 0 && !i;
}

static BOOL FSaveEnemyDesign(void) {
    NATIVEFILE *lpfile;
    FLEET *lpfl;
    int ishdef, fError;

    if (sel.scan.grobj != grobjFleet || sel.scan.ifl == iflNone ||
        GlobalPD.grPopup != grPopupShdef || !GlobalPD.lpshdef) return FALSE;
    lpfl = rglpfl[sel.scan.ifl];
    for (ishdef = 0; ishdef < 16; ishdef++)
        if (lpfl->rgcsh[ishdef] != 0 && &rglpshdef[lpfl->iPlayer][ishdef] == GlobalPD.lpshdef) break;
    if (ishdef == 16) return FALSE;
    lpfile = PfNativeOpen("enemy-design.json", "w");
    if (!lpfile) return FALSE;
    CchNativePrintf(lpfile, "{\"fleet\":%d,\"player\":%u,\"design\":%d,\"hideCounts\":%s,\"summary\":%s,\"showDamage\":%s,\"token\":%s}\n",
            lpfl->id, lpfl->iPlayer, ishdef, GlobalPD.fHideCounts ? "true" : "false",
            GlobalPD.fSummary ? "true" : "false", GlobalPD.fShowDamage ? "true" : "false", GlobalPD.fToken ? "true" : "false");
    fError = FNativeStreamError(lpfile);
    return ICloseNativeFile(lpfile) == 0 && !fError;
}

static BOOL FSaveTutorialPopup(const char *pszPath, BOOL fResearch) {
    NATIVEFILE *lpfile;
    int fError;

    if (!IsWindowVisible(hwndPopup) || GetCapture() != hwndPopup) return FALSE;
    if (fResearch) {
        if (GlobalPD.grPopup != grPopupComponent || GlobalPD.part.hs.grhst != hstTerra ||
            GlobalPD.part.hs.iItem != iterraRadiationTerraform7) return FALSE;
    } else if (GlobalPD.grPopup != grPopupPlanetEnv || GlobalPD.idPlanet != 5 || GlobalPD.iPlanetVar != 2) return FALSE;
    lpfile = PfNativeOpen(pszPath, "w");
    if (!lpfile) return FALSE;
    if (fResearch) CchNativePrintf(lpfile, "{\"type\":%d,\"kind\":%u,\"item\":%u}\n",
                           GlobalPD.grPopup, GlobalPD.part.hs.grhst, GlobalPD.part.hs.iItem);
    else CchNativePrintf(lpfile, "{\"type\":%d,\"planet\":%d,\"environment\":%d}\n",
                 GlobalPD.grPopup, GlobalPD.idPlanet, GlobalPD.iPlanetVar);
    fError = FNativeStreamError(lpfile);
    return ICloseNativeFile(lpfile) == 0 && !fError;
}

static BOOL FSaveBattle(const char *pszPath) {
    NATIVEFILE *lpfile;
    int fError;
    if (!vlpbdVCR || !vrgtok) return FALSE;
    lpfile = PfNativeOpen(pszPath, "w");
    if (!lpfile) return FALSE;
    CchNativePrintf(lpfile, "{\"battle\":%u,\"tokens\":%u,\"step\":%d,\"lastStep\":%d,\"playing\":%s}\n",
            BdvFromLp(vlpbdVCR).id(), BdvFromLp(vlpbdVCR).ctok(), viStepVCRCur, vcStepVCR, gd.fVCRTimer ? "true" : "false");
    fError = FNativeStreamError(lpfile);
    return ICloseNativeFile(lpfile) == 0 && !fError;
}

static BOOL FSaveSalvage(void) {
    THING *lpth;
    NATIVEFILE *lpfile;
    int fError;
    if (sel.scan.grobj != grobjThing || sel.scan.ith < 0 || sel.scan.ith >= cThing) return FALSE;
    lpth = &lpThings[sel.scan.ith];
    if (lpth->ith != ithMineralPacket || lpth->thp.iWarp != 0) return FALSE;
    lpfile = PfNativeOpen("salvage.json", "w");
    if (!lpfile) return FALSE;
    CchNativePrintf(lpfile, "{\"id\":%u,\"position\":[%d,%d],\"minerals\":[%d,%d,%d]}\n",
            lpth->idFull, lpth->pt.x, lpth->pt.y, lpth->thp.rgwtMin[0], lpth->thp.rgwtMin[1], lpth->thp.rgwtMin[2]);
    fError = FNativeStreamError(lpfile);
    return ICloseNativeFile(lpfile) == 0 && !fError;
}

static BOOL FSaveReport(const char *pszPath) {
    NATIVEFILE *lpfile;
    int i, fError;
    if (!IsWindow(hwndReportDlg) || !vprptCur || vprptCur->irpt != rptPlanets) return FALSE;
    lpfile = PfNativeOpen(pszPath, "w");
    if (!lpfile) return FALSE;
    CchNativePrintf(lpfile, "{\"turn\":%u,\"type\":%u,\"column\":%d,\"subsort\":%d,\"ascending\":%s,\"planetIds\":[",
            game.turn, vprptCur->irpt, vprptCur->icolSort, vprptCur->iSubsort, vprptCur->fAscending ? "true" : "false");
    for (i = 0; i < vprptCur->cRows; i++) CchNativePrintf(lpfile, "%s%u", i ? "," : "", lpPlanets[vlprgidRep[i]].id);
    IPutNativeString("]}\n", lpfile);
    fError = FNativeStreamError(lpfile);
    return ICloseNativeFile(lpfile) == 0 && !fError;
}

static void CALLBACK RunTutorial(HWND hwndTimer, UINT message, UINT_PTR id, DWORD tick) {
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
        {"Factories (Auto Build)", iobjFactory, 30, 0, grobjPlanet, TRUE, FALSE},
        {"Factory", mdIdleFactory, 3, 0, grobjPlanet, FALSE, TRUE},
        {"Mine", mdIdleMine, 3, 1, grobjPlanet, FALSE, TRUE},
        {"Santa Maria", 2, 1, 1, grobjFleet, FALSE, FALSE},
        {"Santa Maria", 2, 3, 1, grobjFleet, FALSE, FALSE},
        {"Factories (Auto Build)", iobjFactory, 10, 0, grobjPlanet, TRUE, TRUE},
        {"Mines (Auto Build)", iobjMine, 10, 1, grobjPlanet, TRUE, TRUE},
        {"Armed Probe", 0, 2, 2, grobjFleet, FALSE, FALSE},
        {"Factories (Auto Build)", iobjFactory, 3, 0, grobjPlanet, FALSE, TRUE},
        {"Mines (Auto Build)", iobjMine, 3, 1, grobjPlanet, FALSE, TRUE},
        {"Teamster", 3, 1, 2, grobjFleet, FALSE, FALSE},
        {"Mine", mdIdleMine, 70, 0, grobjPlanet, TRUE, FALSE},
        {"Terraform Environment", mdIdleTerraform, 2, 0, grobjPlanet, FALSE, TRUE},
        {"Teamster", 3, 2, 1, grobjFleet, FALSE, FALSE},
        {"Mini-Miner", 6, 1, 1, grobjFleet, FALSE, FALSE},
        {"Mine", mdIdleMine, 100, 0, grobjPlanet, FALSE, FALSE},
        {"Min Terraform (Auto Build)", iobjMinTerraform, 2, 0, grobjPlanet, FALSE, TRUE},
        {"Factories (Auto Build)", iobjFactory, 60, 0, grobjPlanet, TRUE, FALSE},
        {"Mines (Auto Build)", iobjMine, 60, 1, grobjPlanet, TRUE, FALSE}
    };
    (void)hwndTimer; (void)message; (void)tick;

    if (++cWait > 300) { pszFailure = "bounded tutorial action timed out"; goto LFail; }
    if (tutor.fAutoComplete) { pszFailure = "tutorial auto-completion was used"; goto LFail; }
    iPage = tutor.idt / 8;
    if (gd.fTutorial && iPage >= 0 && iPage < 80 && !rgfPage[iPage]) { rgfPage[iPage] = TRUE; cPage++; }
    hwndModal = HwndModal();
    switch (iStage) {
    case 0:
        if (!IsWindowVisible(hwndTitle)) return;
        if (!FClick(GetDlgItem(hwndTitle, IDC_LIBRARY_TUTORIAL))) goto LInput;
        break;
    case 1:
        if (!gd.fTutorial || !IsWindow(hwndScanner)) return;
        break;
    case 2:
        if (!gd.fTutorial || tutor.idtBold != 5 || !IsWindow(hwndScanner) || hwndModal) return;
        if (game.turn != 0 || tutor.fTurnDone) { pszFailure = "unexpected initial tutorial state"; goto LFail; }
        if (!FSaveState("start")) goto LArtifact;
        if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
        cActions++;
        break;
    case 3:
        if (!hwndModal) return;
        if (!fExpectedRejection) {
            pszFailure = "unexpected dialog instead of unfinished-task rejection";
            goto LFail;
        }
        if (!FSaveState("rejected") || !FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        fRejected = TRUE;
        break;
    case 4:
        if (hwndModal) return;
        if (!gd.fTutorial || game.turn != 0) { pszFailure = "premature Generate changed tutorial state"; goto LFail; }
        if (tutor.idtBold == 5) {
            if (++cReadActions > 100) { pszFailure = "messages did not advance after bounded clicks"; goto LFail; }
            if (!IsWindowEnabled(rghwndMsgBtn[2])) return;
            if (!FClick(rghwndMsgBtn[2])) goto LInput;
            cWait = 0;
            LogState("read-message");
            return;
        }
        if (tutor.idtBold != 11 || tutor.idt != 8) { pszFailure = "reading messages did not reach page two"; goto LFail; }
        if (!FSaveState("page-two")) goto LArtifact;
        if (!FClick(rghwndBtn[1])) goto LInput;
        break;
    case 5:
        if (hwndModal) { pszFailure = "unexpected dialog after Goto"; goto LFail; }
        if (tutor.idtBold != 15 || sel.grobj != grobjFleet || sel.id != 0) return;
        if (!FFindPlanet("Prune")) { pszFailure = "Prune missing from visible map inspection"; goto LFail; }
        if (!FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) || !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) ||
            !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
        break;
    case 6:
        if (hwndModal) { pszFailure = "unexpected dialog after map order"; goto LFail; }
        if (tutor.idt != 16 || tutor.idtBold != 18) return;
        lpfl = LpflFromId(0);
        if (!lpfl || lpfl->cord != 2 || lpfl->lpplord->rgord()[1].grobj != grobjPlanet || lpfl->lpplord->rgord()[1].id != objTarget.id ||
            lpfl->lpplord->rgord()[1].pt.x != objTarget.ptUniverse.x || lpfl->lpplord->rgord()[1].pt.y != objTarget.ptUniverse.y) {
            pszFailure = "Shift-click did not issue the inspected planet order";
            goto LFail;
        }
        if (!FSaveState("page-three")) goto LArtifact;
        if (fFirstYear) { iStage = 7; goto LQueued; }
        fPassed = fRejected && gd.fTutorial && game.turn == 0 && !tutor.fTurnDone;
        LogState("smoke-passed");
        KillTimer(NULL, id);
        PostQuitMessage(fPassed ? 0 : 93);
        return;
    case 7:
        if (hwndModal || !gd.fTutorial || game.turn != 0) { pszFailure = "unexpected first-year state or dialog"; goto LFail; }
        idtBefore = tutor.idtBold;
        LogState("instruction");
        switch (tutor.idtBold) {
        case 18: case 25: case 27: case 29: case 32: case 34:
            if (!FPushTextKey(SDL_SCANCODE_N, "n")) goto LInput;
            cActions++;
            iStage = 11;
            goto LQueued;
        case 21: case 31:
            pszPlanet = tutor.idtBold == 21 ? "90210" : "Alexander";
            if (sel.grobj != grobjFleet || sel.id != (tutor.idtBold == 21 ? 1 : 4)) {
                pszFailure = "tutorial map instruction selected the wrong fleet"; goto LFail;
            }
            if (!FFindPlanet(pszPlanet) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) ||
                !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
            iStage = 11;
            goto LQueued;
        case 37:
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F5, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F5, FALSE)) goto LInput;
            cActions++;
            break;
        case 39:
            if (!tutor.fTurnDone || rgplr[0].iTechNow() != Weapons || rgplr[0].iTechNext() != TechFieldCount || rgplr[0].pctResearch != 15) {
                pszFailure = "Generate requested before first-year tasks passed"; goto LFail;
            }
            if (!FSaveOrders("year-2400-orders.json") || !FSaveState("before-generate")) goto LArtifact;
            LogState("generate-ready");
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
            cActions++;
            iStage = 12;
            goto LQueued;
        default:
            pszFailure = "unimplemented first-year instruction"; goto LFail;
        }
        break;
    case 8:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != ResearchDlg || tutor.idtBold != 38) { pszFailure = "unexpected research dialog"; goto LFail; }
        if (!FClick(GetDlgItem(hwndModal, IDC_RESEARCH_ENERGY + (int)Weapons))) goto LInput;
        break;
    case 9:
        if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg) { pszFailure = "research dialog vanished"; goto LFail; }
        if (iResTechNow != Weapons) return;
        if (!FClick(GetDlgItem(hwndModal, IDCANCEL))) goto LInput;
        break;
    case 10:
        if (hwndModal) return;
        if (rgplr[0].iTechNow() != Weapons || pctResGlob != -1) { pszFailure = "research choice was not accepted"; goto LFail; }
        iStage = 7;
        goto LQueued;
    case 11:
        if (hwndModal) { pszFailure = "unexpected fleet instruction dialog"; goto LFail; }
        if (tutor.idtBold == idtBefore) return;
        iStage = 7;
        goto LQueued;
    case 12:
        if (hwndModal) { pszFailure = "unexpected generation dialog"; goto LFail; }
        if (game.turn == 0) return;
        if (game.turn != 1 || !gd.fTutorial || tutor.idt != 40 || tutor.idtBold != 42) {
            pszFailure = "first generation did not reach the year-2401 production task"; goto LFail;
        }
        if (!FSaveState("year-2401")) goto LArtifact;
        if (fTwoYears) { iStage = 13; goto LQueued; }
        fPassed = fRejected && cPage == 6;
        LogState("first-year-partial");
        KillTimer(NULL, id);
        PostQuitMessage(fPassed ? 0 : 93);
        return;
    case 13:
        if (!FFindPlanet("Stove Top")) goto LInput;
        pszMenu = "Stove Top";
        if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
        break;
    case 14:
        if (pszMenu || sel.grobj != grobjPlanet || sel.id != 13) return;
        if (!FPushTextKey(SDL_SCANCODE_Q, "q")) goto LInput;
        cActions++;
        break;
    case 15:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != ProductionDlg || !lpplProdGlob || tutor.idtBold != 43) { pszFailure = "unexpected production dialog"; goto LFail; }
        hwndList = GetDlgItem(hwndModal, IDC_PRODUCTION_AVAILABLE_ITEMS);
        iFactoryRow = -1;
        for (i = 0; i < SendMessage(hwndList, LB_GETCOUNT, 0, 0); i++) {
            if (SendMessage(hwndList, LB_GETTEXTLEN, i, 0) >= (LRESULT)sizeof(szItem)) goto LInput;
            SendMessage(hwndList, LB_GETTEXT, i, (LPARAM)szItem);
            if (strlen(szItem) >= 6 && !strcmp(szItem + 6, "Factory")) { iFactoryRow = i; break; }
        }
        if (iFactoryRow < 0 || SendMessage(hwndList, LB_GETITEMRECT, SendMessage(hwndList, LB_GETTOPINDEX, 0, 0), (LPARAM)&rcRow) == LB_ERR) goto LInput;
        ptRow.x = (rcRow.left + rcRow.right) / 2; ptRow.y = (rcRow.top + rcRow.bottom) / 2;
        if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
        if (!FPushKey(hwndList, SDL_SCANCODE_HOME, TRUE) || !FPushKey(hwndList, SDL_SCANCODE_HOME, FALSE)) goto LInput;
        for (i = 0; i < iFactoryRow; i++) {
            if (!FPushKey(hwndList, SDL_SCANCODE_DOWN, TRUE) || !FPushKey(hwndList, SDL_SCANCODE_DOWN, FALSE)) goto LInput;
        }
        break;
    case 16:
        if (!hwndModal || hwndModal->lpfnDlg != ProductionDlg) goto LInput;
        if (SendMessage(GetDlgItem(hwndModal, IDC_PRODUCTION_AVAILABLE_ITEMS), LB_GETCURSEL, 0, 0) != iFactoryRow) return;
        /* Pushed SDL events do not change device state read by GetAsyncKeyState.
         * Hold the test modifier until the real Add callbacks have completed. */
        SDL_SetModState(SDL_KMOD_LSHIFT);
        if (!FPushKey(hwndModal, SDL_SCANCODE_LSHIFT, TRUE) || !FClick(GetDlgItem(hwndModal, IDC_PRODUCTION_ADD))) goto LInput;
        break;
    case 17:
        if (!lpplProdGlob || lpplProdGlob->iprodMac != 1 || lpplProdGlob->rgprod()[0].cItem != 10) return;
        if (!FClick(GetDlgItem(hwndModal, IDC_PRODUCTION_ADD))) goto LInput;
        break;
    case 18:
        if (!lpplProdGlob || lpplProdGlob->iprodMac != 1 || lpplProdGlob->rgprod()[0].cItem != 20) return;
        if (lpplProdGlob->rgprod()[0].grobj != grobjPlanet || lpplProdGlob->rgprod()[0].iItem != mdIdleFactory) goto LInput;
        SDL_SetModState(SDL_KMOD_NONE);
        if (!FPushKey(hwndModal, SDL_SCANCODE_LSHIFT, FALSE) || !FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 19:
        if (hwndModal || tutor.idtBold != 47 || !tutor.fTurnDone) return;
        if (!LpplFromId(13)->lpplprod || LpplFromId(13)->lpplprod->iprodMac != 1 ||
            LpplFromId(13)->lpplprod->rgprod()[0].cItem != 20) goto LInput;
        if (!FSaveOrders("year-2401-orders.json") || !FSaveState("before-generate-2401")) goto LArtifact;
        LogState("generate-ready");
        if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
        cActions++;
        break;
    case 20:
        if (hwndModal) { pszFailure = "unexpected second-generation dialog"; goto LFail; }
        if (game.turn == 1) return;
        if (game.turn != 2 || !gd.fTutorial || tutor.idt != 48 || tutor.idtBold != 48) { pszFailure = "second generation did not reach page seven"; goto LFail; }
        if (!FSaveState("year-2402")) goto LArtifact;
        if (fScouting) { iStage = 21; goto LQueued; }
        fPassed = fRejected && cPage == 7;
        LogState("two-years-partial");
        KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
    case 21:
        if (hwndModal || game.turn != 2 || !gd.fTutorial) { pszFailure = "unexpected scouting state or dialog"; goto LFail; }
        idtBefore = tutor.idtBold; iMsgBefore = iMsgCur;
        LogState("scouting-instruction");
        if (tutor.idtBold == 76) {
            if (!FSaveOrders("year-2402-scout-orders.json") || !FSaveState("scouting")) goto LArtifact;
            if (fMining) { iStage = 22; goto LQueued; }
            fPassed = fRejected && cPage == 10;
            LogState("scouting-partial");
            KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
        }
        if (tutor.idtBold == 48 || tutor.idtBold == 71) {
            if (!FClick(rghwndMsgBtn[1])) goto LInput;
            iStage = 24; goto LQueued;
        }
        if (tutor.idtBold == 55 || tutor.idtBold == 61 || tutor.idtBold == 63 || tutor.idtBold == 69) {
            if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
            iStage = 23; goto LQueued;
        }
        for (i = 0; i < (int)(sizeof(rgScout) / sizeof(rgScout[0])); i++) {
            if (tutor.idtBold != rgScout[i].idt) continue;
            if (!FFindPlanet(rgScout[i].pszName) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) ||
                !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
            iStage = 24; goto LQueued;
        }
        pszFailure = "unimplemented scouting instruction"; goto LFail;
    case 22:
        if (!GetClientRect(hwndFrame, &rcRow)) goto LInput;
        /* A private window manager may already have maximized the frame;
         * requesting a smaller SDL size then fails while maximized. */
        if (rcRow.bottom < 650 && !SetWindowPos(hwndFrame, NULL, 0, 0, 900, 700,
                                               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE)) {
            pszFailure = "unable to resize tutorial frame"; goto LFail;
        }
        iStage = 25; goto LQueued;
    case 23:
        if (hwndModal) { pszFailure = "unexpected message navigation dialog"; goto LFail; }
        if (iMsgCur == iMsgBefore) return;
        if (!FClick(rghwndMsgBtn[1])) goto LInput;
        iStage = 24; goto LQueued;
    case 24:
        if (hwndModal) { pszFailure = "unexpected scouting action dialog"; goto LFail; }
        if (tutor.idtBold == idtBefore && iMsgCur == iMsgBefore) return;
        iStage = 21; goto LQueued;
    case 25:
        if (!FFindMapObject(grobjFleet, 5, NULL)) goto LInput;
        pszMenu = objTarget.szName;
        if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
        break;
    case 26:
        if (pszMenu || sel.grobj != grobjFleet || sel.id != 5 || tutor.idtBold != 77) return;
        if (!FFindPlanet("Prune") || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) ||
            !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
        break;
    case 27:
        if (tutor.idtBold != 79) return;
        if (!FClick(rghwndOrderDD[0])) goto LInput;
        break;
    case 28:
        if (!SendMessage(rghwndOrderDD[0], CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(rghwndOrderDD[0], grTaskMine)) goto LInput;
        break;
    case 29:
        if (SendMessage(rghwndOrderDD[0], CB_GETDROPPEDSTATE, 0, 0) || tutor.idt != 80 || tutor.idtBold != 80) return;
        lpfl = LpflFromId(5);
        if (!lpfl || lpfl->cord != 2 || lpfl->lpplord->rgord()[1].grobj != grobjPlanet ||
            lpfl->lpplord->rgord()[1].id != 12 || lpfl->lpplord->rgord()[1].grTask != grTaskMine) goto LInput;
        if (!FSaveOrders("year-2402-mining-orders.json") || !FSaveState("mining")) goto LArtifact;
        if (fColonization) { iStage = 30; goto LQueued; }
        fPassed = fRejected && cPage == 11;
        LogState("mining-partial");
        KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
    case 30:
        if (hwndModal || game.turn != 2) { pszFailure = "unexpected colonization state or dialog"; goto LFail; }
        idtBefore = tutor.idtBold; iMsgBefore = iMsgCur;
        LogState("colonization-instruction");
        if (tutor.idtBold == 80 || tutor.idtBold == 84 || tutor.idtBold == 86) {
            if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
            iStage = 31; goto LQueued;
        }
        if (tutor.idtBold == 83) {
            ptRow = (POINT){30, 65};
            if (!FMousePoint(hwndMine, ptRow, SDL_BUTTON_LEFT, TRUE)) goto LInput;
            cActions++;
            iStage = 41; goto LQueued;
        }
        if (tutor.idtBold == 89) {
            if (!FFindMapObject(grobjFleet, 2, NULL)) goto LInput;
            pszMenu = objTarget.szName;
            if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 33; goto LQueued;
        }
        pszFailure = "unimplemented colonization instruction"; goto LFail;
    case 31:
        if (hwndModal) { pszFailure = "unexpected colonization message dialog"; goto LFail; }
        if (iMsgCur == iMsgBefore) return;
        if (!FClick(rghwndMsgBtn[1])) goto LInput;
        iStage = 32; goto LQueued;
    case 32:
        if (tutor.idtBold == idtBefore && iMsgCur == iMsgBefore) return;
        iStage = 30; goto LQueued;
    case 33:
        if (pszMenu || sel.grobj != grobjFleet || sel.id != 2 || tutor.idtBold != 90) return;
        if (!FClick(rghwndBtn[7])) goto LInput;
        break;
    case 34:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || mdXferDlg != mdXferCargo || !pxfer ||
            pxfer[0].id != 2 || pxfer[0].grobj != grobjFleet || pxfer[1].id != 13 || pxfer[1].grobj != grobjPlanet) {
            pszFailure = "unexpected colonist transfer dialog"; goto LFail;
        }
        for (i = 0; i < crgbtnXfer; i++) {
            if ((rgbtnXfer[i].iVal & 0x7f) == 3 && rgbtnXfer[i].iSide == 0 && !rgbtnXfer[i].fVisible && !(rgbtnXfer[i].bt & 4)) break;
        }
        if (i == crgbtnXfer) goto LInput;
        memcpy(&rcRow, &rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){rcRow.left + 2, (rcRow.top + rcRow.bottom) / 2};
        ptTo = (POINT){rcRow.right - 2, ptRow.y};
        if (!FDragPoint(hwndModal, ptRow, ptTo)) goto LInput;
        break;
    case 35:
        if (!hwndModal || !pxfer || pxfer[0].fl.rgwtMin[3] != 25) return;
        if (!FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 36:
        if (hwndModal || tutor.idtBold != 92) return;
        lpfl = LpflFromId(2);
        if (!lpfl || lpfl->rgwtMin[0] != 0 || lpfl->rgwtMin[1] != 0 || lpfl->rgwtMin[2] != 0 || lpfl->rgwtMin[3] != 25) goto LInput;
        if (!FFindPlanet("90210") || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) ||
            !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
        break;
    case 37:
        if (tutor.idtBold != 93) return;
        if (!FClick(rghwndOrderDD[0])) goto LInput;
        break;
    case 38:
        if (!SendMessage(rghwndOrderDD[0], CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(rghwndOrderDD[0], grTaskColonize)) goto LInput;
        break;
    case 39:
        if (hwndModal || !tutor.fTurnDone || tutor.idtBold != 95) return;
        if (!FSaveOrders("year-2402-orders.json") || !FSaveState("before-generate-2402")) goto LArtifact;
        LogState("generate-ready");
        if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
        cActions++;
        break;
    case 40:
        if (hwndModal) { pszFailure = "unexpected third-generation dialog"; goto LFail; }
        if (game.turn == 2) return;
        if (game.turn != 3 || !gd.fTutorial || tutor.idt != 96 || tutor.idtBold != 97) {
            pszFailure = "third generation did not reach page thirteen"; goto LFail;
        }
        if (!FSaveState("year-2403")) goto LArtifact;
        if (fLogistics) {
            if (!FSaveOrders("year-2403-start-orders.json")) goto LArtifact;
            iStage = 42; goto LQueued;
        }
        fPassed = fRejected && cPage == 13;
        LogState("colonization-partial");
        KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
    case 41:
        if (!IsWindow(hwndPopup)) return;
        if (!IsWindowVisible(hwndPopup) || GetCapture() != hwndPopup || !GetClientRect(hwndPopup, &rcRow)) goto LInput;
        if (!FSaveState("summary-popup")) goto LArtifact;
        LogState("summary-popup");
        ptRow = (POINT){rcRow.right / 2, rcRow.bottom / 2};
        if (!FMousePoint(hwndPopup, ptRow, SDL_BUTTON_LEFT, FALSE)) goto LInput;
        iStage = 32; goto LQueued;
    case 42:
        if (hwndModal || game.turn != 3) { pszFailure = "unexpected logistics state or dialog"; goto LFail; }
        idtBefore = tutor.idtBold; iMsgBefore = iMsgCur;
        LogState("logistics-instruction");
        if (tutor.idtBold == 97) {
            ptRow = (POINT){rcMsgTitle.left + (rcMsgTitle.bottom - rcMsgTitle.top) / 2,
                            (rcMsgTitle.top + rcMsgTitle.bottom) / 2};
            if (!FClickPoint(hwndMessage, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 44; goto LQueued;
        }
        if (tutor.idtBold == 98 || tutor.idtBold == 104 || tutor.idtBold == 115) {
            if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
            iStage = 43; goto LQueued;
        }
        if (tutor.idtBold == 101 || tutor.idtBold == 106) {
            iProdPass = tutor.idtBold == 101 ? 0 : 1;
            iProdReturn = 44;
            cProdAdded = 0;
            if (!FPushTextKey(SDL_SCANCODE_Q, "q")) goto LInput;
            cActions++; iStage = 45; goto LQueued;
        }
        if (tutor.idtBold == 109 || tutor.idtBold == 118) {
            if (!FFindMapObject(grobjFleet, tutor.idtBold == 109 ? 3 : 0, NULL)) goto LInput;
            pszMenu = objTarget.szName;
            if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = tutor.idtBold == 109 ? 50 : 58; goto LQueued;
        }
        if (tutor.idtBold == 121) {
            if (!FFindPlanet("Hiho") || !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 44; goto LQueued;
        }
        if (tutor.idtBold == 123) {
            if (SendMessage(hwndShipLB, LB_GETITEMRECT, 1, (LPARAM)&rcRow) == LB_ERR) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            if (!FClickPoint(hwndShipLB, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 59; goto LQueued;
        }
        if (tutor.idtBold == 127) {
            if (!tutor.fTurnDone || !FSaveOrders("year-2403-orders.json") || !FSaveState("before-generate-2403")) goto LArtifact;
            LogState("generate-ready");
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
            cActions++; iStage = 61; goto LQueued;
        }
        pszFailure = "unimplemented logistics instruction"; goto LFail;
    case 43:
        if (hwndModal) { pszFailure = "unexpected logistics message dialog"; goto LFail; }
        if (iMsgCur == iMsgBefore) return;
        if (!FClick(rghwndMsgBtn[1])) goto LInput;
        iStage = 44; goto LQueued;
    case 44:
        if (tutor.idtBold == idtBefore && iMsgCur == iMsgBefore) return;
        iStage = 42; goto LQueued;
    case 45:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != ProductionDlg || !lpplProdGlob) goto LInput;
        if (rgProduction[iProdPass].fLeftovers && SendMessage(GetDlgItem(hwndModal, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY), BM_GETCHECK, 0, 0) != BST_CHECKED &&
            !FClick(GetDlgItem(hwndModal, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY))) goto LInput;
        hwndList = GetDlgItem(hwndModal, IDC_PRODUCTION_AVAILABLE_ITEMS);
        iFactoryRow = -1;
        for (i = 0; i < SendMessage(hwndList, LB_GETCOUNT, 0, 0); i++) {
            if (SendMessage(hwndList, LB_GETTEXTLEN, i, 0) >= (LRESULT)sizeof(szItem)) goto LInput;
            SendMessage(hwndList, LB_GETTEXT, i, (LPARAM)szItem);
            if (strlen(szItem) >= 6 && !strcmp(szItem + 6, rgProduction[iProdPass].pszName)) { iFactoryRow = i; break; }
        }
        if (iFactoryRow < 0 || SendMessage(hwndList, LB_GETITEMRECT, SendMessage(hwndList, LB_GETTOPINDEX, 0, 0), (LPARAM)&rcRow) == LB_ERR) goto LInput;
        ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
        if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
        iStage = 49; goto LQueued;
    case 49:
        if (!hwndModal) goto LInput;
        hwndList = GetDlgItem(hwndModal, IDC_PRODUCTION_AVAILABLE_ITEMS);
        if (GetFocus() != hwndList) return;
        /* Keyboard events must be translated after the click changes focus. */
        if (!FPushKey(hwndList, SDL_SCANCODE_HOME, TRUE) || !FPushKey(hwndList, SDL_SCANCODE_HOME, FALSE)) goto LInput;
        for (i = 0; i < iFactoryRow; i++) {
            if (!FPushKey(hwndList, SDL_SCANCODE_DOWN, TRUE) || !FPushKey(hwndList, SDL_SCANCODE_DOWN, FALSE)) goto LInput;
        }
        iStage = 46; goto LQueued;
    case 46:
        if (hwndModal && cWait == 20) fprintf(stderr, "Production item %s: selected %ld, expected row %d\n", rgProduction[iProdPass].pszName,
            (long)SendMessage(GetDlgItem(hwndModal, IDC_PRODUCTION_AVAILABLE_ITEMS), LB_GETCURSEL, 0, 0), iFactoryRow);
        if (!hwndModal || SendMessage(GetDlgItem(hwndModal, IDC_PRODUCTION_AVAILABLE_ITEMS), LB_GETCURSEL, 0, 0) != iFactoryRow) return;
        SDL_SetModState(iProdPass == 15 ? SDL_KMOD_LCTRL : rgProduction[iProdPass].fShift ? SDL_KMOD_LSHIFT : SDL_KMOD_NONE);
        if (!FClick(GetDlgItem(hwndModal, IDC_PRODUCTION_ADD))) goto LInput;
        cProdAdded += iProdPass == 15 ? 100 : rgProduction[iProdPass].fShift ? 10 : 1;
        break;
    case 47:
        if (!lpplProdGlob || lpplProdGlob->iprodMac <= rgProduction[iProdPass].iQueue) return;
        i = rgProduction[iProdPass].iQueue;
        if (lpplProdGlob->rgprod()[i].iItem != (unsigned)rgProduction[iProdPass].iItem ||
            lpplProdGlob->rgprod()[i].cItem != (unsigned)cProdAdded ||
            lpplProdGlob->rgprod()[i].grobj != (unsigned)rgProduction[iProdPass].grobj) return;
        if (cProdAdded < rgProduction[iProdPass].cGoal) { iStage = 46; goto LQueued; }
        SDL_SetModState(SDL_KMOD_NONE);
        if (iProdPass == 1) { iProdPass = 2; cProdAdded = 0; iStage = 45; goto LQueued; }
        if (iProdPass == 5 || iProdPass == 8) { iProdPass++; cProdAdded = 0; iStage = 68; goto LQueued; }
        hwndList = GetDlgItem(hwndModal, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY);
        if (SendMessage(hwndList, BM_GETCHECK, 0, 0) != (rgProduction[iProdPass].fLeftovers ? BST_CHECKED : BST_UNCHECKED) && !FClick(hwndList)) goto LInput;
        break;
    case 48:
        if (!hwndModal || SendMessage(GetDlgItem(hwndModal, IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY), BM_GETCHECK, 0, 0) !=
            (rgProduction[iProdPass].fLeftovers ? BST_CHECKED : BST_UNCHECKED)) return;
        if (!FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        iStage = iProdReturn; goto LQueued;
    case 50:
        if (pszMenu || sel.grobj != grobjFleet || sel.id != 3 || tutor.idtBold != 110) return;
        if (!FClick(rghwndBtn[7])) goto LInput;
        break;
    case 51:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || mdXferDlg != mdXferCargo || !pxfer || pxfer[0].id != 3 || pxfer[1].id != 13) goto LInput;
        for (i = 0; i < crgbtnXfer; i++) {
            if ((rgbtnXfer[i].iVal & 0x7f) == 3 && rgbtnXfer[i].iSide == 0 && !rgbtnXfer[i].fVisible && !(rgbtnXfer[i].bt & 4)) break;
        }
        if (i == crgbtnXfer) goto LInput;
        memcpy(&rcRow, &rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){rcRow.left + 2, (rcRow.top + rcRow.bottom) / 2}; ptTo = (POINT){rcRow.right - 2, ptRow.y};
        if (!FDragPoint(hwndModal, ptRow, ptTo)) goto LInput;
        break;
    case 52:
        if (!hwndModal || !pxfer || pxfer[0].fl.rgwtMin[3] != 210) return;
        if (!FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 53:
        if (hwndModal || tutor.idtBold != 112) return;
        if (!FFindPlanet("90210") || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) ||
            !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
        break;
    case 54:
        if (tutor.idtBold != 113) return;
        if (!FClick(rghwndOrderDD[0])) goto LInput;
        break;
    case 55:
        if (!SendMessage(rghwndOrderDD[0], CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(rghwndOrderDD[0], grTaskXfer)) goto LInput;
        break;
    case 56:
        if (tutor.idtBold != 114 || SendMessage(rghwndOrderDD[0], CB_GETDROPPEDSTATE, 0, 0)) return;
        ptRow = (POINT){(rgrcRef[5].left + rgrcRef[5].right) / 2, (rgrcRef[5].top + rgrcRef[5].bottom) / 2};
        pszMenu = "QuikDrop";
        if (!FClickPoint(hwndPlanet, ptRow, SDL_BUTTON_RIGHT)) goto LInput;
        break;
    case 57:
        if (pszMenu || tutor.idtBold == 114) return;
        iStage = 42; goto LQueued;
    case 58:
        if (pszMenu || sel.grobj != grobjFleet || sel.id != 0) return;
        iStage = 42; goto LQueued;
    case 59:
        if (GetFocus() != hwndShipLB || SendMessage(hwndShipLB, LB_GETCURSEL, 0, 0) != 1) return;
        if (!FPushKey(hwndShipLB, SDL_SCANCODE_DELETE, TRUE) || !FPushKey(hwndShipLB, SDL_SCANCODE_DELETE, FALSE)) goto LInput;
        cActions++;
        break;
    case 60:
        if (tutor.idtBold == 123) return;
        iStage = 42; goto LQueued;
    case 61:
        if (hwndModal) { pszFailure = "unexpected fourth-generation dialog"; goto LFail; }
        if (game.turn == 3) return;
        if (game.turn != 4 || !gd.fTutorial || tutor.idt != 128) { pszFailure = "fourth generation did not reach page seventeen"; goto LFail; }
        if (!FSaveState("year-2404")) goto LArtifact;
        if (fColonizerProduction) {
            if (!FSaveOrders("year-2404-start-orders.json")) goto LArtifact;
            iStage = 62; goto LQueued;
        }
        fPassed = fRejected && cPage == 17;
        LogState("logistics-partial");
        KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
    case 62:
        if (hwndModal || game.turn != 4) { pszFailure = "unexpected colonizer-production state or dialog"; goto LFail; }
        idtBefore = tutor.idtBold; iMsgBefore = iMsgCur;
        LogState("colonizer-production-instruction");
        if (tutor.idtBold == 128) {
            if (!FClick(rghwndMsgBtn[1])) goto LInput;
            iStage = 64; goto LQueued;
        }
        if (tutor.idtBold == 130) {
            if (!FFindMapObject(grobjFleet, 4, NULL)) goto LInput;
            pszMenu = objTarget.szName;
            if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 65; goto LQueued;
        }
        if (tutor.idtBold == 131 || tutor.idtBold == 132) {
            if (SendMessage(hwndShipLB, LB_GETITEMRECT, 1, (LPARAM)&rcRow) == LB_ERR) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            if (!FClickPoint(hwndShipLB, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 66; goto LQueued;
        }
        if (tutor.idtBold == 133) {
            if (!FClick(rghwndMsgBtn[2])) goto LInput;
            iStage = 63; goto LQueued;
        }
        if (tutor.idtBold == 135) {
            if (!FFindPlanet("Stove Top")) goto LInput;
            pszMenu = objTarget.szName;
            if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 64; goto LQueued;
        }
        if (tutor.idtBold == 136) {
            iProdPass = 3; cProdAdded = 0; iProdReturn = 64;
            if (!FPushTextKey(SDL_SCANCODE_Q, "q")) goto LInput;
            cActions++; iStage = 68; goto LQueued;
        }
        if (tutor.idtBold == 143) {
            if (!tutor.fTurnDone || !FSaveOrders("year-2404-orders.json") || !FSaveState("before-generate-2404")) goto LArtifact;
            LogState("generate-ready");
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
            cActions++; iStage = 69; goto LQueued;
        }
        pszFailure = "unimplemented colonizer-production instruction"; goto LFail;
    case 63:
        if (iMsgCur == iMsgBefore) return;
        if (!FClick(rghwndMsgBtn[1])) goto LInput;
        iStage = 64; goto LQueued;
    case 64:
        if (pszMenu || tutor.idtBold == idtBefore) return;
        iStage = 62; goto LQueued;
    case 65:
        if (pszMenu || sel.grobj != grobjFleet || sel.id != 4) return;
        iStage = 62; goto LQueued;
    case 66:
        if (GetFocus() != hwndShipLB || SendMessage(hwndShipLB, LB_GETCURSEL, 0, 0) != 1) return;
        if (!FPushKey(hwndShipLB, SDL_SCANCODE_DELETE, TRUE) || !FPushKey(hwndShipLB, SDL_SCANCODE_DELETE, FALSE)) goto LInput;
        cActions++; iStage = 64; goto LQueued;
    case 68:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != ProductionDlg || !lpplProdGlob) goto LInput;
        /* Add inserts before the selected queue row. */
        hwndList = GetDlgItem(hwndModal, IDC_PRODUCTION_QUEUE);
        if (SendMessage(hwndList, LB_GETITEMRECT, rgProduction[iProdPass].iQueue, (LPARAM)&rcRow) == LB_ERR) goto LInput;
        ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
        if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
        iStage = 70; goto LQueued;
    case 70:
        if (!hwndModal || SendMessage(GetDlgItem(hwndModal, IDC_PRODUCTION_QUEUE), LB_GETCURSEL, 0, 0) != rgProduction[iProdPass].iQueue) return;
        iStage = 45; goto LQueued;
    case 69:
        if (hwndModal) { pszFailure = "unexpected fifth-generation dialog"; goto LFail; }
        if (game.turn == 4) return;
        if (game.turn != 5 || !gd.fTutorial || tutor.idt != 144) { pszFailure = "fifth generation did not reach page nineteen"; goto LFail; }
        if (!FSaveState("year-2405")) goto LArtifact;
        if (fSecondColonization) {
            if (!FSaveOrders("year-2405-start-orders.json")) goto LArtifact;
            iStage = 71; goto LQueued;
        }
        fPassed = fRejected && cPage == 19;
        LogState("colonizer-production-partial");
        KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
    case 71:
        if (hwndModal || game.turn != 5) { pszFailure = "unexpected second-colonization state or dialog"; goto LFail; }
        idtBefore = tutor.idtBold; iMsgBefore = iMsgCur;
        LogState("second-colonization-instruction");
        if (tutor.idtBold == 144) {
            if (!FFindMapObject(grobjFleet, 2, NULL)) goto LInput;
            pszMenu = objTarget.szName;
            if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 79; goto LQueued;
        }
        if (tutor.idtBold == 146) {
            ptRow = (POINT){(rgrcRef[3].left + rgrcRef[3].right) / 2, (rgrcRef[3].top + rgrcRef[3].bottom) / 2};
            if (!FClickPoint(hwndPlanet, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 73; goto LQueued;
        }
        if (tutor.idtBold == 150 || tutor.idtBold == 153) {
            hwndList = GetDlgItem(hwndTb, IDC_STARS_TOOLBAR);
            i = tutor.idtBold == 150 ? tbPlanetValueView : tbNormalView;
            if (!SendMessage(hwndList, TB_GETRECT, IDC_STARS_TOOLBUTTON_FIRST + i, (LPARAM)&rcRow)) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 72; goto LQueued;
        }
        if (tutor.idtBold == 151 || tutor.idtBold == 155) {
            if (tutor.idtBold == 151) {
                if ((grbitScan & grbitScanViewMask) != 3 || !FSaveState("habitability")) goto LArtifact;
                LogState("scanner-habitability");
            } else {
                if ((grbitScan & grbitScanViewMask) != 0) goto LInput;
                LogState("scanner-normal");
            }
            if (!FFindPlanet(tutor.idtBold == 151 ? "Shaggy Dog" : "Stove Top") ||
                !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) ||
                !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
            iStage = 72; goto LQueued;
        }
        if (tutor.idtBold == 152) {
            if (!FClick(rghwndOrderDD[0])) goto LInput;
            iStage = 76; goto LQueued;
        }
        if (tutor.idtBold == 154) {
            if (iMsgCur < 1) {
                if (!FClick(rghwndMsgBtn[2])) goto LInput;
                iStage = 78; goto LQueued;
            }
            if (!FClick(rghwndMsgBtn[1])) goto LInput;
            iStage = 72; goto LQueued;
        }
        if (tutor.idtBold == 156) {
            if (IMsgNext(FALSE) != imsgNone) {
                if (!FClick(rghwndMsgBtn[2])) goto LInput;
                iStage = 78; goto LQueued;
            }
            if (!FFindPlanet("90210")) goto LInput;
            pszMenu = objTarget.szName;
            if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 72; goto LQueued;
        }
        if (tutor.idtBold == 158) {
            if (sel.grobj != grobjFleet || sel.id != 0) {
                if (!FFindMapObject(grobjFleet, 0, NULL)) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 79; goto LQueued;
            }
            if (IMsgNext(FALSE) != imsgNone) {
                if (!FClick(rghwndMsgBtn[2])) goto LInput;
                iStage = 78; goto LQueued;
            }
            if (SendMessage(hwndShipLB, LB_GETITEMRECT, 1, (LPARAM)&rcRow) == LB_ERR) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            if (!FClickPoint(hwndShipLB, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 80; goto LQueued;
        }
        if (tutor.idtBold == 159) {
            if (!tutor.fTurnDone || !FSaveOrders("year-2405-orders.json") || !FSaveState("before-generate-2405")) goto LArtifact;
            LogState("generate-ready");
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
            cActions++; iStage = 81; goto LQueued;
        }
        pszFailure = "unimplemented second-colonization instruction"; goto LFail;
    case 72:
        if (pszMenu || tutor.idtBold == idtBefore) return;
        iStage = 71; goto LQueued;
    case 73:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || mdXferDlg != mdXferCargo || !pxfer || pxfer[0].id != 2 || pxfer[1].id != 13) goto LInput;
        for (i = 0; i < crgbtnXfer; i++) {
            if ((rgbtnXfer[i].iVal & 0x7f) == 3 && rgbtnXfer[i].iSide == 0 && !rgbtnXfer[i].fVisible && !(rgbtnXfer[i].bt & 4)) break;
        }
        if (i == crgbtnXfer) goto LInput;
        memcpy(&rcRow, &rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){rcRow.left + 2, (rcRow.top + rcRow.bottom) / 2}; ptTo = (POINT){rcRow.right - 2, ptRow.y};
        if (!FDragPoint(hwndModal, ptRow, ptTo)) goto LInput;
        break;
    case 74:
        if (!hwndModal || !pxfer || pxfer[0].fl.rgwtMin[3] != 25) return;
        if (!FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 75:
        if (hwndModal || tutor.idtBold == 146 || tutor.idtBold == 148) return;
        if (!FSaveOrders("year-2405-cargo.json")) goto LArtifact;
        iStage = 71; goto LQueued;
    case 76:
        if (!SendMessage(rghwndOrderDD[0], CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(rghwndOrderDD[0], grTaskColonize)) goto LInput;
        iStage = 72; goto LQueued;
    case 78:
        if (iMsgCur == iMsgBefore) return;
        iStage = 71; goto LQueued;
    case 79:
        if (pszMenu) return;
        iStage = 71; goto LQueued;
    case 80:
        if (GetFocus() != hwndShipLB || SendMessage(hwndShipLB, LB_GETCURSEL, 0, 0) != 1) return;
        if (!FPushKey(hwndShipLB, SDL_SCANCODE_DELETE, TRUE) || !FPushKey(hwndShipLB, SDL_SCANCODE_DELETE, FALSE)) goto LInput;
        cActions++; iStage = 72; goto LQueued;
    case 81:
        if (hwndModal) { pszFailure = "unexpected sixth-generation dialog"; goto LFail; }
        if (game.turn == 5) return;
        if (game.turn != 6 || !gd.fTutorial || tutor.idt != 160) { pszFailure = "sixth generation did not reach page twenty-one"; goto LFail; }
        if (!FSaveState("year-2406")) goto LArtifact;
        if (fTransportCycle) {
            if (!FSaveOrders("year-2406-start-orders.json")) goto LArtifact;
            iStage = 82; goto LQueued;
        }
        fPassed = fRejected && cPage == 21;
        LogState("second-colonization-partial");
        KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
    case 82:
        if (hwndModal || game.turn != 6) { pszFailure = "unexpected transport-cycle state or dialog"; goto LFail; }
        idtBefore = tutor.idtBold;
        LogState("transport-cycle-instruction");
        if (tutor.idtBold == 160) {
            if (!FFindMapObject(grobjFleet, 3, NULL)) goto LInput;
            pszMenu = objTarget.szName;
            if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 86; goto LQueued;
        }
        if (tutor.idtBold == 163) {
            if (!FFindPlanet("Prune") || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) ||
                !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
            iStage = 83; goto LQueued;
        }
        if (tutor.idtBold == 164) {
            if (!FClick(rghwndOrderDD[0])) goto LInput;
            iStage = 84; goto LQueued;
        }
        if (tutor.idtBold == 165 || tutor.idtBold == 170) {
            if (tutor.idtBold == 170 && !FSaveOrders("year-2406-copied-orders.json")) goto LArtifact;
            ptRow = (POINT){(rgrcRef[5].left + rgrcRef[5].right) / 2, (rgrcRef[5].top + rgrcRef[5].bottom) / 2};
            pszMenu = tutor.idtBold == 165 ? "QuikLoad" : "QuikDrop";
            if (!FClickPoint(hwndPlanet, ptRow, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 83; goto LQueued;
        }
        if (tutor.idtBold == 166) {
            if (SendMessage(hwndShipLB, LB_GETITEMRECT, 1, (LPARAM)&rcRow) == LB_ERR) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            if (!FClickPoint(hwndShipLB, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 87; goto LQueued;
        }
        if (tutor.idtBold == 171) {
            if (!FClick(hwndRepCB)) goto LInput;
            iStage = 83; goto LQueued;
        }
        if (tutor.idtBold == 173) {
            if (!FFindPlanet("Stove Top")) goto LInput;
            pszMenu = objTarget.szName;
            if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 83; goto LQueued;
        }
        if (tutor.idtBold == 174) {
            iProdPass = 3; cProdAdded = 0; iProdReturn = 83;
            if (!FPushTextKey(SDL_SCANCODE_Q, "q")) goto LInput;
            cActions++; iStage = 68; goto LQueued;
        }
        if (tutor.idtBold == 175) {
            if (!tutor.fTurnDone || !FSaveOrders("year-2406-orders.json") || !FSaveState("before-generate-2406")) goto LArtifact;
            LogState("generate-ready");
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
            cActions++; iStage = 90; goto LQueued;
        }
        pszFailure = "unimplemented transport-cycle instruction"; goto LFail;
    case 83:
        if (pszMenu || tutor.idtBold == idtBefore) return;
        iStage = 82; goto LQueued;
    case 84:
        if (!SendMessage(rghwndOrderDD[0], CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(rghwndOrderDD[0], grTaskXfer)) goto LInput;
        iStage = 83; goto LQueued;
    case 86:
        if (pszMenu || sel.grobj != grobjFleet || sel.id != 3) return;
        iStage = 82; goto LQueued;
    case 87:
        if (SendMessage(hwndShipLB, LB_GETCURSEL, 0, 0) != 1 || sel.iwpAct != 1) return;
        if (!FFindPlanet("Stove Top") || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) ||
            !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
        iStage = 83; goto LQueued;
    case 90:
        if (hwndModal) { pszFailure = "unexpected seventh-generation dialog"; goto LFail; }
        if (game.turn == 6) return;
        if (game.turn != 7 || !gd.fTutorial || tutor.idt != 176) { pszFailure = "seventh generation did not reach page twenty-three"; goto LFail; }
        if (!FSaveState("year-2407")) goto LArtifact;
        if (fExpansion) {
            if (!FSaveOrders("year-2407-start-orders.json")) goto LArtifact;
            iStage = 91; goto LQueued;
        }
        fPassed = fRejected && cPage == 23;
        LogState("transport-cycle-partial");
        KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
    case 91:
        if (hwndModal || game.turn != 7) { pszFailure = "unexpected expansion state or dialog"; goto LFail; }
        idtBefore = tutor.idtBold; iMsgBefore = iMsgCur;
        LogState("expansion-instruction");
        if (tutor.idtBold == 176) {
            if (sel.grobj != grobjFleet || sel.id != 6) {
                if (!FFindMapObject(grobjFleet, 6, NULL)) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 93; goto LQueued;
            }
            if (!FClick(rghwndBtn[7])) goto LInput;
            iStage = 94; goto LQueued;
        }
        if (tutor.idtBold == 177 || tutor.idtBold == 182) {
            hwndList = GetDlgItem(hwndTb, IDC_STARS_TOOLBAR);
            i = tutor.idtBold == 177 ? tbPlanetValueView : tbNormalView;
            if (!SendMessage(hwndList, TB_GETRECT, IDC_STARS_TOOLBUTTON_FIRST + i, (LPARAM)&rcRow)) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 92; goto LQueued;
        }
        if (tutor.idtBold == 179) {
            if ((grbitScan & grbitScanViewMask) != 3 || !FSaveState("red-storm-view")) goto LArtifact;
            LogState("expansion-habitability");
            if (!FFindPlanet("Red Storm") || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) ||
                !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
            iStage = 98; goto LQueued;
        }
        if (tutor.idtBold == 181 || tutor.idtBold == 188 || tutor.idtBold == 196) {
            i = tutor.idtBold == 188 ? 16 : 13;
            if (sel.grobj != grobjPlanet || sel.id != i) {
                if (!FFindPlanet(i == 16 ? "90210" : "Stove Top")) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 93; goto LQueued;
            }
            iProdPass = tutor.idtBold == 181 ? 4 : tutor.idtBold == 188 ? 5 : 7;
            cProdAdded = 0; iProdReturn = 92;
            if (!FPushTextKey(SDL_SCANCODE_Q, "q")) goto LInput;
            cActions++; iStage = 68; goto LQueued;
        }
        if (tutor.idtBold == 184 || tutor.idtBold == 187 || tutor.idtBold == 190) {
            if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
            iStage = 101; goto LQueued;
        }
        if (tutor.idtBold == 186) {
            ptRow = (POINT){rcMsgTitle.left + (rcMsgTitle.bottom - rcMsgTitle.top) / 2,
                            (rcMsgTitle.top + rcMsgTitle.bottom) / 2};
            if (!FClickPoint(hwndMessage, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 92; goto LQueued;
        }
        if (tutor.idtBold == 192) {
            if ((grbitScan & grbitScanViewMask) != 0 || !FFindMapObject(grobjFleet, 512, NULL) ||
                !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT)) goto LInput;
            LogState("expansion-normal");
            iStage = 102; goto LQueued;
        }
        if (tutor.idtBold == 198) {
            if (!tutor.fTurnDone || !FSaveOrders("year-2407-orders.json") || !FSaveState("before-generate-2407")) goto LArtifact;
            LogState("generate-ready");
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
            cActions++; iStage = 105; goto LQueued;
        }
        pszFailure = "unimplemented expansion instruction"; goto LFail;
    case 92:
        if (pszMenu || tutor.idtBold == idtBefore) return;
        iStage = 91; goto LQueued;
    case 93:
        if (pszMenu) return;
        iStage = 91; goto LQueued;
    case 94:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || mdXferDlg != mdXferCargo || !pxfer || pxfer[0].id != 6 || pxfer[1].id != 13) goto LInput;
        for (i = 0; i < crgbtnXfer; i++) {
            if ((rgbtnXfer[i].iVal & 0x7f) == 3 && rgbtnXfer[i].iSide == 0 && !rgbtnXfer[i].fVisible && !(rgbtnXfer[i].bt & 4)) break;
        }
        if (i == crgbtnXfer) goto LInput;
        memcpy(&rcRow, &rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){rcRow.left + 2, (rcRow.top + rcRow.bottom) / 2}; ptTo = (POINT){rcRow.right - 2, ptRow.y};
        if (!FDragPoint(hwndModal, ptRow, ptTo)) goto LInput;
        break;
    case 95:
        if (!hwndModal || !pxfer || pxfer[0].fl.rgwtMin[3] != 25) return;
        if (!FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 96:
        if (hwndModal || tutor.idtBold == 176) return;
        if (!FSaveOrders("year-2407-cargo.json")) goto LArtifact;
        iStage = 91; goto LQueued;
    case 98:
        if (sel.grobj != grobjFleet || sel.id != 6 || sel.fl.cord != 2) return;
        if (!FClick(rghwndOrderDD[0])) goto LInput;
        break;
    case 99:
        if (!SendMessage(rghwndOrderDD[0], CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(rghwndOrderDD[0], grTaskColonize)) goto LInput;
        iStage = 92; goto LQueued;
    case 101:
        if (iMsgCur == iMsgBefore) return;
        iStage = 91; goto LQueued;
    case 102:
        if (sel.scan.grobj != grobjFleet || sel.scan.ifl == iflNone || rglpfl[sel.scan.ifl]->id != 512) return;
        if (!FSaveState("enemy-summary")) goto LArtifact;
        ptRow = (POINT){30, 2 * dyArial8 + 25};
        if (!FMousePoint(hwndMine, ptRow, SDL_BUTTON_RIGHT, TRUE)) goto LInput;
        cActions++; break;
    case 103:
        if (!IsWindow(hwndPopup)) return;
        if (!IsWindowVisible(hwndPopup) || GetCapture() != hwndPopup || !GetClientRect(hwndPopup, &rcRow)) goto LInput;
        if (!FSaveState("enemy-design-popup") || !FSaveEnemyDesign()) goto LArtifact;
        LogState("enemy-design-popup");
        ptRow = (POINT){rcRow.right / 2, rcRow.bottom / 2};
        if (!FMousePoint(hwndPopup, ptRow, SDL_BUTTON_RIGHT, FALSE)) goto LInput;
        break;
    case 104:
        if (IsWindow(hwndPopup)) return;
        iStage = 91; goto LQueued;
    case 105:
        if (hwndModal) { pszFailure = "unexpected eighth-generation dialog"; goto LFail; }
        if (game.turn == 7) return;
        if (game.turn != 8 || !gd.fTutorial || tutor.idt != 200) { pszFailure = "eighth generation did not reach page twenty-six"; goto LFail; }
        if (!FSaveState("year-2408")) goto LArtifact;
        if (fFleetSplit) {
            if (!FSaveOrders("year-2408-start-orders.json")) goto LArtifact;
            iStage = 106; goto LQueued;
        }
        fPassed = fRejected && cPage == 26;
        LogState("expansion-partial");
        KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
    case 106:
        if (hwndModal || game.turn != 8) { pszFailure = "unexpected fleet splitting state or dialog"; goto LFail; }
        if (tutor.idt == 208) {
            if (!FSaveOrders("year-2408-split-orders.json") || !FSaveState("fleet-split")) goto LArtifact;
            if (fResearchCycle) { iStage = 118; goto LQueued; }
            fPassed = fRejected && cPage == 27;
            LogState("fleet-split-partial");
            KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
        }
        idtBefore = tutor.idtBold;
        LogState("fleet-split-instruction");
        if (tutor.idtBold == 200 || tutor.idtBold == 202) {
            if (sel.grobj != grobjFleet || sel.id != 7) {
                if (!FFindMapObject(grobjFleet, 7, NULL)) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 107; goto LQueued;
            }
            if (!FClick(rghwndBtn[8])) goto LInput;
            iStage = 108; goto LQueued;
        }
        if (tutor.idtBold == 205) {
            lpfl = LpflFromId(7);
            if (!lpfl || sel.grobj != grobjFleet || sel.id != 7) goto LInput;
            if (lpfl->rgwtMin[3] != 50) {
                if (!FClick(rghwndBtn[7])) goto LInput;
                iStage = 111; goto LQueued;
            }
            if (lpfl->cord == 1) {
                if (!FFindPlanet("Slime") || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) ||
                    !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
                iStage = 114; goto LQueued;
            }
            if (lpfl->cord != 2 || lpfl->lpplord->rgord()[1].id != 8) goto LInput;
            if (!FClick(rghwndOrderDD[0])) goto LInput;
            iStage = 115; goto LQueued;
        }
        if (tutor.idtBold == 206) {
            if (!FSaveOrders("year-2408-before-split-all.json") || !FClick(rghwndBtn[9])) goto LArtifact;
            iStage = 117; goto LQueued;
        }
        pszFailure = "unimplemented fleet splitting instruction"; goto LFail;
    case 107:
        if (pszMenu) return;
        iStage = 106; goto LQueued;
    case 108:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || mdXferDlg != mdXferShips || !pxfer || pxfer[0].id != 7 || pxfer[1].id != 9) goto LInput;
        if (!FSaveState("ship-transfer")) goto LArtifact;
        for (i = 0; i < crgbtnXfer; i++) {
            if (rgbtnXfer[i].iVal == 2 && rgbtnXfer[i].iSide == 1 && rgbtnXfer[i].fVisible && !(rgbtnXfer[i].bt & 4)) break;
        }
        if (i == crgbtnXfer) goto LInput;
        memcpy(&rcRow, &rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
        if (!FClickPoint(hwndModal, ptRow, SDL_BUTTON_LEFT)) goto LInput;
        break;
    case 109:
        if (!hwndModal || !pxfer || pxfer[0].fl.rgcsh[2] != 2 || pxfer[1].fl.rgcsh[2] != 1) return;
        if (!FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 110:
        if (hwndModal) return;
        if (!FSaveOrders("year-2408-after-transfer.json")) goto LArtifact;
        iStage = 106; goto LQueued;
    case 111:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || mdXferDlg != mdXferCargo || !pxfer || pxfer[0].id != 7 || pxfer[1].id != 13) goto LInput;
        for (i = 0; i < crgbtnXfer; i++) {
            if ((rgbtnXfer[i].iVal & 0x7f) == 3 && rgbtnXfer[i].iSide == 0 && !rgbtnXfer[i].fVisible && !(rgbtnXfer[i].bt & 4)) break;
        }
        if (i == crgbtnXfer) goto LInput;
        memcpy(&rcRow, &rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){rcRow.left + 2, (rcRow.top + rcRow.bottom) / 2}; ptTo = (POINT){rcRow.right - 2, ptRow.y};
        if (!FDragPoint(hwndModal, ptRow, ptTo)) goto LInput;
        break;
    case 112:
        if (!hwndModal || !pxfer || pxfer[0].fl.rgwtMin[3] != 50) return;
        if (!FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 113:
        if (hwndModal) return;
        if (!FSaveOrders("year-2408-split-cargo.json")) goto LArtifact;
        iStage = 106; goto LQueued;
    case 114:
        if (sel.grobj != grobjFleet || sel.id != 7 || sel.fl.cord != 2) return;
        iStage = 106; goto LQueued;
    case 115:
        if (!SendMessage(rghwndOrderDD[0], CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(rghwndOrderDD[0], grTaskColonize)) goto LInput;
        break;
    case 116:
        if (SendMessage(rghwndOrderDD[0], CB_GETDROPPEDSTATE, 0, 0) || tutor.idtBold == idtBefore) return;
        iStage = 106; goto LQueued;
    case 117:
        if (tutor.idt != 208 || rgplr[idPlayer].cFleet != 11) return;
        iStage = 106; goto LQueued;
    case 118:
        if (game.turn != 8 || (hwndModal && hwndModal->lpfnDlg != ResearchDlg)) {
            pszFailure = "unexpected research cycle state or dialog"; goto LFail;
        }
        idtBefore = tutor.idtBold; iMsgBefore = iMsgCur;
        LogState("research-cycle-instruction");
        if (tutor.idtBold == 209 || tutor.idtBold == 212) {
            i = tutor.idtBold == 209 ? 7 : 8;
            if (sel.grobj != grobjFleet || sel.id != i) {
                if (!FFindMapObject(grobjFleet, i, NULL)) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 119; goto LQueued;
            }
            if (i == 8) return;
            lpfl = LpflFromId(7);
            if (!lpfl || lpfl->cord != 2 || !FFindPlanet("Sea Squared")) goto LInput;
            ptLogical = lpfl->lpplord->rgord()[1].pt;
            LogicalToScan(&ptLogical);
            ptRow = (POINT){ptLogical.x, ptLogical.y};
            if (!FDragPoint(hwndScanner, ptRow, objTarget.ptClient)) goto LInput;
            iStage = 120; goto LQueued;
        }
        if (tutor.idtBold == 214) {
            if (!FFindPlanet("Hiho") || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) ||
                !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
            iStage = 121; goto LQueued;
        }
        if (tutor.idtBold == 216 || tutor.idtBold == 217) {
            if (tutor.idtBold == 216 && IdmGetMessageN(iMsgCur) == idmHasUnloaded) {
                ptRow = (POINT){rcMsgTitle.left + (rcMsgTitle.bottom - rcMsgTitle.top) / 2,
                                (rcMsgTitle.top + rcMsgTitle.bottom) / 2};
                if (!FClickPoint(hwndMessage, ptRow, SDL_BUTTON_LEFT)) goto LInput;
                iStage = 123; goto LQueued;
            }
            if (tutor.idtBold == 216 || IMsgNext(FALSE) != imsgNone) {
                if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
                iStage = 122; goto LQueued;
            }
            if (!FFindPlanet("Wallaby") || !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 124; goto LQueued;
        }
        if (tutor.idtBold == 219) {
            if (!GetClientRect(hwndMine, &rcRow)) goto LInput;
            i = ((rcRow.bottom - (2 * dyArial8 - 4) - 4 * dyArial8 - 2) / 6 + 1) & ~1;
            ptRow = (POINT){30, 4 * dyArial8 - 4 + 2 * i + i / 2};
            if (!FMousePoint(hwndMine, ptRow, SDL_BUTTON_LEFT, TRUE)) goto LInput;
            cActions++; iStage = 125; goto LQueued;
        }
        if (tutor.idtBold == 221) {
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F5, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F5, FALSE)) goto LInput;
            cActions++; iStage = 127; goto LQueued;
        }
        if (tutor.idtBold == 226) {
            if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg) return;
            for (i = 0; i < cFutureTech; i++) {
                if (rghsFutureTech[i].grhst == hstTerra && rghsFutureTech[i].iItem == iterraRadiationTerraform7) break;
            }
            if (i == cFutureTech) { pszFailure = "Radiation Terraform 7 benefit is absent"; goto LFail; }
            ptRow = (POINT){dxResLeft / 2, yTopFutureTech + i * dyArial8 + dyArial8 / 2};
            if (!FMousePoint(hwndModal, ptRow, SDL_BUTTON_LEFT, TRUE)) goto LInput;
            cActions++; iStage = 128; goto LQueued;
        }
        if (tutor.idtBold == 228) {
            if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg) goto LInput;
            if (pctResGlob == 30) {
                if (!FClick(GetDlgItem(hwndModal, IDCANCEL))) goto LInput;
                iStage = 131; goto LQueued;
            }
            pctResearchBefore = pctResGlob;
            rcRow = pctResGlob < 30 ? rcSpinTop : rcSpinBot;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            if (!FClickPoint(hwndModal, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 130; goto LQueued;
        }
        if (tutor.idtBold == 230) {
            if (!tutor.fTurnDone || !FSaveOrders("year-2408-orders.json") || !FSaveState("before-generate-2408")) goto LArtifact;
            LogState("generate-ready");
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
            cActions++; iStage = 132; goto LQueued;
        }
        pszFailure = "unimplemented research cycle instruction"; goto LFail;
    case 119:
        if (pszMenu) return;
        iStage = 118; goto LQueued;
    case 120:
        lpfl = LpflFromId(7);
        if (!lpfl || lpfl->cord != 2 || lpfl->lpplord->rgord()[1].id != 17 || lpfl->lpplord->rgord()[1].grTask != grTaskColonize) return;
        if (!FSaveOrders("year-2408-drag-orders.json") || !FSaveState("waypoint-drag")) goto LArtifact;
        LogState("waypoint-dragged");
        iStage = 118; goto LQueued;
    case 121:
        if (tutor.idtBold == idtBefore) return;
        if (!FSaveOrders("year-2408-scout-orders.json")) goto LArtifact;
        iStage = 118; goto LQueued;
    case 122:
        if (iMsgCur == iMsgBefore) return;
        iStage = 118; goto LQueued;
    case 123:
        if (!(bitfMsgFiltered[idmHasUnloaded >> 3] & (1 << (idmHasUnloaded & 7)))) return;
        iStage = 118; goto LQueued;
    case 124:
        if (sel.scan.grobj != grobjPlanet || sel.scan.idpl != 5) return;
        iStage = 118; goto LQueued;
    case 125:
        if (!IsWindow(hwndPopup)) return;
        if (!FSaveTutorialPopup("wallaby-radiation.json", FALSE) || !FSaveState("wallaby-radiation")) goto LArtifact;
        LogState("environment-popup");
        if (!GetClientRect(hwndPopup, &rcRow)) goto LInput;
        ptRow = (POINT){rcRow.right / 2, rcRow.bottom / 2};
        if (!FMousePoint(hwndPopup, ptRow, SDL_BUTTON_LEFT, FALSE)) goto LInput;
        break;
    case 126:
        if (IsWindow(hwndPopup) || tutor.idtBold == idtBefore) return;
        iStage = 118; goto LQueued;
    case 127:
        if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg || tutor.idtBold == idtBefore) return;
        if (!FSaveState("research-benefits")) goto LArtifact;
        iStage = 118; goto LQueued;
    case 128:
        if (!IsWindow(hwndPopup)) return;
        if (!FSaveTutorialPopup("radiation-benefit.json", TRUE) || !FSaveState("radiation-benefit")) goto LArtifact;
        LogState("research-benefit-popup");
        if (!GetClientRect(hwndPopup, &rcRow)) goto LInput;
        ptRow = (POINT){rcRow.right / 2, rcRow.bottom / 2};
        if (!FMousePoint(hwndPopup, ptRow, SDL_BUTTON_LEFT, FALSE)) goto LInput;
        break;
    case 129:
        if (IsWindow(hwndPopup) || tutor.idtBold == idtBefore) return;
        iStage = 118; goto LQueued;
    case 130:
        if (!hwndModal || pctResGlob == pctResearchBefore) return;
        iStage = 118; goto LQueued;
    case 131:
        if (hwndModal || pctResGlob != -1 || tutor.idtBold == idtBefore) return;
        iStage = 118; goto LQueued;
    case 132:
        if (hwndModal) { pszFailure = "unexpected ninth-generation dialog"; goto LFail; }
        if (game.turn == 8) return;
        if (game.turn != 9 || !gd.fTutorial || tutor.idt != 232) { pszFailure = "ninth generation did not reach page thirty"; goto LFail; }
        if (!FSaveState("year-2409")) goto LArtifact;
        if (fOxygenColony) {
            if (!FSaveOrders("year-2409-start-orders.json")) goto LArtifact;
            iStage = 133; goto LQueued;
        }
        fPassed = fRejected && cPage == 30;
        LogState("research-cycle-partial");
        KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
    case 133:
        if (game.turn != 9 || (hwndModal && hwndModal->lpfnDlg != ResearchDlg)) {
            pszFailure = "unexpected Oxygen colonization state or dialog"; goto LFail;
        }
        idtBefore = tutor.idtBold; iMsgBefore = iMsgCur;
        LogState("oxygen-colonization-instruction");
        if (tutor.idtBold == 232) {
            if (mdMsgObj != mdMsgObjResearch || !FClick(rghwndMsgBtn[1])) goto LInput;
            iStage = 134; goto LQueued;
        }
        if (tutor.idtBold == 236) {
            if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg || !FClick(GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD))) goto LInput;
            iStage = 135; goto LQueued;
        }
        if (tutor.idtBold == 238) {
            if (IdmGetMessageN(iMsgCur) != idmHasLoadedMiningRobotsWorking) {
                if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
                iStage = 138; goto LQueued;
            }
            ptRow = (POINT){rcMsgTitle.left + (rcMsgTitle.bottom - rcMsgTitle.top) / 2,
                            (rcMsgTitle.top + rcMsgTitle.bottom) / 2};
            if (!FClickPoint(hwndMessage, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 140; goto LQueued;
        }
        if (tutor.idtBold == 240) {
            if (mdMsgObj != mdMsgObjPlanet || idMsgObj != 2) {
                if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
                iStage = 138; goto LQueued;
            }
            if (!FClick(rghwndMsgBtn[1])) goto LInput;
            iStage = 141; goto LQueued;
        }
        if (tutor.idtBold == 241 || tutor.idtBold == 245) {
            i = tutor.idtBold == 241 ? 9 : 0;
            if (sel.grobj != grobjFleet || sel.id != i) {
                if (!FFindMapObject(grobjFleet, i, NULL)) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 139; goto LQueued;
            }
            if (i == 9) return;
            lpfl = LpflFromId(0);
            if (!lpfl || lpfl->cord != 2 || !FFindPlanet("Mozart")) goto LInput;
            ptLogical = lpfl->lpplord->rgord()[1].pt;
            LogicalToScan(&ptLogical);
            ptRow = (POINT){ptLogical.x, ptLogical.y};
            if (!FDragPoint(hwndScanner, ptRow, objTarget.ptClient)) goto LInput;
            iStage = 148; goto LQueued;
        }
        if (tutor.idtBold == 242) {
            if (sel.grobj != grobjFleet || sel.id != 9 || !FClick(rghwndBtn[7])) goto LInput;
            iStage = 142; goto LQueued;
        }
        if (tutor.idtBold == 243) {
            lpfl = LpflFromId(9);
            if (!lpfl || lpfl->cord != 1 || !FFindPlanet("Oxygen") || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) ||
                !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
            iStage = 145; goto LQueued;
        }
        if (tutor.idtBold == 247) {
            if (!tutor.fTurnDone || !FSaveOrders("year-2409-orders.json") || !FSaveState("before-generate-2409")) goto LArtifact;
            LogState("generate-ready");
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
            cActions++; iStage = 149; goto LQueued;
        }
        pszFailure = "unimplemented Oxygen colonization instruction"; goto LFail;
    case 134:
        if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg || tutor.idtBold == idtBefore) return;
        iStage = 133; goto LQueued;
    case 135:
        if (!hwndModal) return;
        hwndList = GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD);
        if (!SendMessage(hwndList, CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(hwndList, Construction + 1)) goto LInput;
        break;
    case 136:
        if (!hwndModal) return;
        hwndList = GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD);
        if (SendMessage(hwndList, CB_GETDROPPEDSTATE, 0, 0) || SendMessage(hwndList, CB_GETCURSEL, 0, 0) != Construction + 1) return;
        if (!FClick(GetDlgItem(hwndModal, IDCANCEL))) goto LInput;
        break;
    case 137:
        if (hwndModal || pctResGlob != -1 || tutor.idtBold == idtBefore) return;
        if (!FSaveOrders("year-2409-research.json")) goto LArtifact;
        iStage = 133; goto LQueued;
    case 138:
        if (iMsgCur == iMsgBefore) return;
        iStage = 133; goto LQueued;
    case 139:
        if (pszMenu) return;
        iStage = 133; goto LQueued;
    case 140:
        if (!(bitfMsgFiltered[idmHasLoadedMiningRobotsWorking >> 3] & (1 << (idmHasLoadedMiningRobotsWorking & 7)))) return;
        iStage = 133; goto LQueued;
    case 141:
        if (sel.scan.grobj != grobjPlanet || sel.scan.idpl != 2) return;
        if (!FSaveState("oxygen-summary")) goto LArtifact;
        LogState("oxygen-goto");
        iStage = 133; goto LQueued;
    case 142:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || mdXferDlg != mdXferCargo || !pxfer || pxfer[0].id != 9 || pxfer[1].id != 13) goto LInput;
        for (i = 0; i < crgbtnXfer; i++) {
            if ((rgbtnXfer[i].iVal & 0x7f) == 3 && rgbtnXfer[i].iSide == 0 && !rgbtnXfer[i].fVisible && !(rgbtnXfer[i].bt & 4)) break;
        }
        if (i == crgbtnXfer) goto LInput;
        memcpy(&rcRow, &rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){rcRow.left + 2, (rcRow.top + rcRow.bottom) / 2}; ptTo = (POINT){rcRow.right - 2, ptRow.y};
        if (!FDragPoint(hwndModal, ptRow, ptTo)) goto LInput;
        break;
    case 143:
        if (!hwndModal || !pxfer || pxfer[0].fl.rgwtMin[3] != 25) return;
        if (!FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 144:
        if (hwndModal) return;
        if (!FSaveOrders("year-2409-cargo.json")) goto LArtifact;
        iStage = 133; goto LQueued;
    case 145:
        if (sel.grobj != grobjFleet || sel.id != 9 || sel.fl.cord != 2) return;
        if (!FClick(rghwndOrderDD[0])) goto LInput;
        break;
    case 146:
        if (!SendMessage(rghwndOrderDD[0], CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(rghwndOrderDD[0], grTaskColonize)) goto LInput;
        break;
    case 147:
        if (SendMessage(rghwndOrderDD[0], CB_GETDROPPEDSTATE, 0, 0) || tutor.idtBold == idtBefore) return;
        iStage = 133; goto LQueued;
    case 148:
        lpfl = LpflFromId(0);
        if (!lpfl || lpfl->cord != 2 || lpfl->lpplord->rgord()[1].id != 4) return;
        if (!FSaveState("mozart-waypoint")) goto LArtifact;
        LogState("mozart-waypoint-dragged");
        iStage = 133; goto LQueued;
    case 149:
        if (hwndModal) { pszFailure = "unexpected tenth-generation dialog"; goto LFail; }
        if (game.turn == 9) return;
        if (game.turn != 10 || !gd.fTutorial || tutor.idt != 248) { pszFailure = "tenth generation did not reach page thirty-two"; goto LFail; }
        if (!FSaveState("year-2410")) goto LArtifact;
        if (fProdTemplates) {
            if (!FSaveOrders("year-2410-start-orders.json")) goto LArtifact;
            iStage = 150; goto LQueued;
        }
        fPassed = fRejected && cPage == 32;
        LogState("oxygen-colonization-partial");
        KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
    case 150:
        if (game.turn != 10 || (hwndModal && hwndModal->lpfnDlg != ProductionDlg && hwndModal->lpfnDlg != ZipProdDlg)) {
            pszFailure = "unexpected production template state or dialog"; goto LFail;
        }
        idtBefore = tutor.idtBold; iMsgBefore = iMsgCur;
        LogState("production-templates-instruction");
        if (tutor.idtBold == 248) {
            if (IdmGetMessageN(iMsgCur) != idmHasDismantledKtMineralsWhichHaveDeposited) {
                if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
                iStage = 151; goto LQueued;
            }
            ptRow = (POINT){rcMsgTitle.left + (rcMsgTitle.bottom - rcMsgTitle.top) / 2,
                            (rcMsgTitle.top + rcMsgTitle.bottom) / 2};
            if (!FClickPoint(hwndMessage, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 163; goto LQueued;
        }
        if (tutor.idtBold == 249 || tutor.idtBold == 259) {
            i = tutor.idtBold == 249 ? 14 : 23;
            if (mdMsgObj != mdMsgObjPlanet || idMsgObj != i) {
                if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
                iStage = 151; goto LQueued;
            }
            if (!FClick(rghwndMsgBtn[1])) goto LInput;
            iStage = i == 14 ? 152 : 160; goto LQueued;
        }
        if (tutor.idtBold == 250) {
            iProdPass = 8; cProdAdded = 0; iProdReturn = 153;
            if (!FPushTextKey(SDL_SCANCODE_Q, "q")) goto LInput;
            cActions++; iStage = 45; goto LQueued;
        }
        if (tutor.idtBold == 254) {
            if (!FPushTextKey(SDL_SCANCODE_Q, "q")) goto LInput;
            cActions++; iStage = 154; goto LQueued;
        }
        if (tutor.idtBold == 255) {
            if (!hwndModal || hwndModal->lpfnDlg != ProductionDlg) goto LInput;
            pszMenu = "<Customize>";
            ptRow = (POINT){(rcProdDiamond.left + rcProdDiamond.right) / 2, (rcProdDiamond.top + rcProdDiamond.bottom) / 2};
            if (!FClickPoint(hwndModal, ptRow, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 155; goto LQueued;
        }
        if (tutor.idtBold == 260 || tutor.idtBold == 262) {
            if (sel.grobj != grobjPlanet || sel.id != 13) {
                if (!FFindPlanet("Stove Top")) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 161; goto LQueued;
            }
            iProdPass = tutor.idtBold == 260 ? 3 : 10; cProdAdded = 0; iProdReturn = 153;
            if (!FPushTextKey(SDL_SCANCODE_Q, "q")) goto LInput;
            cActions++; iStage = 68; goto LQueued;
        }
        if (tutor.idtBold == 263) {
            if (!tutor.fTurnDone || !FSaveOrders("year-2410-orders.json") || !FSaveState("before-generate-2410")) goto LArtifact;
            LogState("generate-ready");
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
            cActions++; iStage = 162; goto LQueued;
        }
        pszFailure = "unimplemented production template instruction"; goto LFail;
    case 151:
        if (iMsgCur == iMsgBefore) return;
        iStage = 150; goto LQueued;
    case 152:
        if (sel.grobj != grobjPlanet || sel.id != 14 || tutor.idtBold == idtBefore) return;
        if (!FSaveState("shaggy-dog-summary")) goto LArtifact;
        LogState("shaggy-dog-goto");
        iStage = 150; goto LQueued;
    case 153:
        if (hwndModal || tutor.idtBold == idtBefore) return;
        if (idtBefore == 250 && !FSaveOrders("year-2410-shaggy-queue.json")) goto LArtifact;
        iStage = 150; goto LQueued;
    case 154:
        if (!hwndModal || hwndModal->lpfnDlg != ProductionDlg || tutor.idtBold == idtBefore) return;
        iStage = 150; goto LQueued;
    case 155:
        if (pszMenu || !hwndModal || hwndModal->lpfnDlg != ZipProdDlg) return;
        if (!IsWindow(hwndModal->hwndOwner) || hwndModal->hwndOwner->lpfnDlg != ProductionDlg || IsWindowEnabled(hwndModal->hwndOwner)) {
            pszFailure = "template dialog did not disable its production owner"; goto LFail;
        }
        if (!FSaveState("template-open")) goto LArtifact;
        LogState("template-nested-open");
        if (!FClick(GetDlgItem(hwndModal, IDC_ZIP_PROD_PRESET_1))) goto LInput;
        break;
    case 156:
        if (!hwndModal || hwndModal->lpfnDlg != ZipProdDlg || !IsDlgButtonChecked(hwndModal, IDC_ZIP_PROD_PRESET_1)) return;
        if (!FClick(GetDlgItem(hwndModal, IDC_IMPORT))) goto LInput;
        break;
    case 157:
        if (!hwndModal || hwndModal->lpfnDlg != ZipProdDlg || !vrgZipProd[0].fValid || vrgZipProd[0].cpq != 2) return;
        if (vrgZipProd[0].rgpq[0].mdIdle() != iobjFactory || vrgZipProd[0].rgpq[0].cQuan() != 3 ||
            vrgZipProd[0].rgpq[1].mdIdle() != iobjMine || vrgZipProd[0].rgpq[1].cQuan() != 3 || !vrgZipProd[0].fNoResearch) return;
        if (!FSaveOrders("year-2410-imported-template.json") || !FSaveState("template-imported")) goto LArtifact;
        if (!FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 158:
        if (!hwndModal || hwndModal->lpfnDlg != ProductionDlg || !IsWindowEnabled(hwndModal)) return;
        if (!FSaveState("template-owner-restored")) goto LArtifact;
        LogState("template-owner-restored");
        if (!FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 159:
        if (hwndModal || lpplProdGlob) return;
        if (!FSaveOrders("year-2410-template-orders.json") || !FSaveState("template-closed")) goto LArtifact;
        /* An earlier progress flag can skip the prompt; still exercise its Goto. */
        if (mdMsgObj != mdMsgObjPlanet || idMsgObj != 23) {
            iMsgBefore = iMsgCur;
            if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
            iStage = 164; goto LQueued;
        }
        if (!FClick(rghwndMsgBtn[1])) goto LInput;
        iStage = 160; goto LQueued;
    case 160:
        if (sel.scan.grobj != grobjPlanet || sel.scan.idpl != 23) return;
        if (!FSaveState("bloop-summary")) goto LArtifact;
        LogState("bloop-goto");
        iStage = 150; goto LQueued;
    case 161:
        if (pszMenu) return;
        iStage = 150; goto LQueued;
    case 162:
        if (hwndModal) { pszFailure = "unexpected eleventh-generation dialog"; goto LFail; }
        if (game.turn == 10) return;
        if (game.turn != 11 || !gd.fTutorial || tutor.idt != 264) { pszFailure = "eleventh generation did not reach page thirty-four"; goto LFail; }
        if (!FSaveState("year-2411")) goto LArtifact;
        if (fBattleCycle) {
            if (!FSaveOrders("year-2411-start-orders.json")) goto LArtifact;
            iStage = 165; goto LQueued;
        }
        fPassed = fRejected && cPage == 34;
        LogState("production-templates-partial");
        KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
    case 163:
        if (!(bitfMsgFiltered[idmHasDismantledKtMineralsWhichHaveDeposited >> 3] & (1 << (idmHasDismantledKtMineralsWhichHaveDeposited & 7)))) return;
        iStage = 150; goto LQueued;
    case 164:
        if (iMsgCur == iMsgBefore) return;
        iStage = 159; goto LQueued;
    case 165:
        if (game.turn != 11 || (hwndModal && hwndModal->lpfnDlg != ResearchDlg)) {
            pszFailure = "unexpected battle cycle state or dialog"; goto LFail;
        }
        idtBefore = tutor.idtBold; iMsgBefore = iMsgCur;
        LogState("battle-cycle-instruction");
        if (tutor.idtBold == 264 || tutor.idtBold == 266 || tutor.idtBold == 269 ||
            tutor.idtBold == 271 || tutor.idtBold == 272 || tutor.idtBold == 275 || tutor.idtBold == 286) {
            idflAction = tutor.idtBold == 264 ? 0 : tutor.idtBold == 266 ? 1 : tutor.idtBold == 269 ? 4 :
                         tutor.idtBold == 272 ? 2 : tutor.idtBold == 275 ? 11 : 8;
            if (sel.grobj != grobjFleet || sel.id != idflAction) {
                if ((mdMsgObj != mdMsgObjFleet || idMsgObj != idflAction) &&
                    !(tutor.idtBold == 286 && mdMsgObj == mdMsgObjBattle && !gd.fGotoVCR)) {
                    if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
                    iStage = 166; goto LQueued;
                }
                if (!FClick(rghwndMsgBtn[1])) goto LInput;
                iStage = 167; goto LQueued;
            }
            /* These prompts include the route gesture after selecting the fleet. */
            if (tutor.idtBold != 269 && tutor.idtBold != 271) return;
        }
        if (tutor.idtBold == 265 || tutor.idtBold == 268 || tutor.idtBold == 269 || tutor.idtBold == 271 ||
            tutor.idtBold == 274 || tutor.idtBold == 276) {
            idflAction = tutor.idtBold == 265 ? 0 : tutor.idtBold == 268 ? 1 : tutor.idtBold == 269 ? 4 :
                         tutor.idtBold == 271 ? 8 : tutor.idtBold == 274 ? 2 : 11;
            lpfl = LpflFromId(idflAction);
            if (!lpfl || sel.grobj != grobjFleet || sel.id != idflAction) goto LInput;
            if (idflAction == 2 || idflAction == 11) {
                cCargoGoal = idflAction == 2 ? 25 : 210;
                if (lpfl->rgwtMin[3] != cCargoGoal) {
                    if (!FClick(rghwndBtn[7])) goto LInput;
                    iStage = 169; goto LQueued;
                }
            }
            if (lpfl->cord == 1) {
                if (idflAction == 8) {
                    if (!FFindMapObject(grobjFleet, 512, NULL)) goto LInput;
                } else if (!FFindPlanet(idflAction == 0 ? "Hacker" : idflAction == 2 ? "Mozart" : idflAction == 11 ? "Wallaby" : "Stove Top")) goto LInput;
                if (!FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) ||
                    !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
                iStage = 168; goto LQueued;
            }
            if (idflAction != 1 && idflAction != 2) { pszFailure = "route did not advance battle cycle instruction"; goto LFail; }
            iOrderCombo = 0; iOrderRow = idflAction == 1 ? grTaskScrap : grTaskColonize;
            if (!FClick(rghwndOrderDD[iOrderCombo])) goto LInput;
            iStage = 173; goto LQueued;
        }
        if (tutor.idtBold == 277 || tutor.idtBold == 278 || tutor.idtBold == 279) {
            iOrderCombo = tutor.idtBold - 277;
            iOrderRow = iOrderCombo == 0 ? grTaskXfer : iOrderCombo == 1 ? 4 : iActionUnloadAll;
            if (!FClick(rghwndOrderDD[iOrderCombo])) goto LInput;
            iStage = 173; goto LQueued;
        }
        if (tutor.idtBold == 280) {
            if (sel.grobj != grobjPlanet || sel.id != 13) {
                if (mdMsgObj != mdMsgObjPlanet || idMsgObj != 13) {
                    if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
                    iStage = 166; goto LQueued;
                }
                if (!FClick(rghwndMsgBtn[1])) goto LInput;
                iStage = 175; goto LQueued;
            }
            iProdPass = 11; cProdAdded = 0; iProdReturn = 176;
            if (!FPushTextKey(SDL_SCANCODE_Q, "q")) goto LInput;
            cActions++; iStage = 68; goto LQueued;
        }
        if (tutor.idtBold == 281) {
            if (mdMsgObj != mdMsgObjResearch) {
                if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
                iStage = 166; goto LQueued;
            }
            if (!FClick(rghwndMsgBtn[1])) goto LInput;
            iStage = 177; goto LQueued;
        }
        if (tutor.idtBold == 282) {
            if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg || !FClick(GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD))) goto LInput;
            iStage = 178; goto LQueued;
        }
        if (tutor.idtBold == 283) {
            if (iMsgCur < 11) {
                if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
                iStage = 191; goto LQueued;
            }
            if (!FClick(rghwndMsgBtn[1])) goto LInput;
            iStage = 181; goto LQueued;
        }
        if (tutor.idtBold == 284) {
            if (!IsWindow(hwndBrowser) || !FSaveState("technology-browser")) goto LArtifact;
            LogState("technology-browser-open");
            if (!FClick(GetDlgItem(hwndBrowser, IDCANCEL))) goto LInput;
            iStage = 182; goto LQueued;
        }
        if (tutor.idtBold == 285 || tutor.idtBold == 291) {
            if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
            iStage = 166; goto LQueued;
        }
        if (tutor.idtBold == 289) {
            if (mdMsgObj != mdMsgObjBattle) goto LInput;
            if (!FClick(rghwndMsgBtn[1])) goto LInput;
            iStage = 183; goto LQueued;
        }
        if (tutor.idtBold == 295) {
            if (!tutor.fTurnDone || !FSaveOrders("year-2411-orders.json") || !FSaveState("before-generate-2411")) goto LArtifact;
            LogState("generate-ready");
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
            cActions++; iStage = 190; goto LQueued;
        }
        pszFailure = "unimplemented battle cycle instruction"; goto LFail;
    case 166:
        if (iMsgCur == iMsgBefore) return;
        iStage = 165; goto LQueued;
    case 167:
        if (sel.grobj != grobjFleet || sel.id != idflAction) return;
        iStage = 165; goto LQueued;
    case 168:
        lpfl = LpflFromId(idflAction);
        if (!lpfl || lpfl->cord != 2) return;
        iStage = 165; goto LQueued;
    case 169:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || mdXferDlg != mdXferCargo || !pxfer || pxfer[0].id != idflAction || pxfer[1].id != 13) goto LInput;
        for (i = 0; i < crgbtnXfer; i++) {
            if ((rgbtnXfer[i].iVal & 0x7f) == 3 && rgbtnXfer[i].iSide == 0 && !rgbtnXfer[i].fVisible && !(rgbtnXfer[i].bt & 4)) break;
        }
        if (i == crgbtnXfer) goto LInput;
        memcpy(&rcRow, &rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){rcRow.left + 2, (rcRow.top + rcRow.bottom) / 2}; ptTo = (POINT){rcRow.right - 2, ptRow.y};
        if (!FDragPoint(hwndModal, ptRow, ptTo)) goto LInput;
        break;
    case 170:
        if (!hwndModal || !pxfer || pxfer[0].fl.rgwtMin[3] != cCargoGoal) return;
        if (!FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 171:
        if (hwndModal) return;
        if (!FSaveOrders(idflAction == 2 ? "year-2411-colonizer-cargo.json" : "year-2411-troop-cargo.json")) goto LArtifact;
        iStage = 165; goto LQueued;
    case 173:
        if (!SendMessage(rghwndOrderDD[iOrderCombo], CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(rghwndOrderDD[iOrderCombo], iOrderRow)) goto LInput;
        break;
    case 174:
        if (SendMessage(rghwndOrderDD[iOrderCombo], CB_GETDROPPEDSTATE, 0, 0) || SendMessage(rghwndOrderDD[iOrderCombo], CB_GETCURSEL, 0, 0) != iOrderRow) return;
        iStage = 165; goto LQueued;
    case 175:
        if (sel.grobj != grobjPlanet || sel.id != 13) return;
        iStage = 165; goto LQueued;
    case 176:
        if (hwndModal || lpplProdGlob) return;
        if (!FSaveOrders("year-2411-mines.json")) goto LArtifact;
        iStage = 165; goto LQueued;
    case 177:
        if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg) return;
        iStage = 165; goto LQueued;
    case 178:
        if (!hwndModal) return;
        hwndList = GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD);
        if (!SendMessage(hwndList, CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(hwndList, Biotechnology + 1)) goto LInput;
        break;
    case 179:
        if (!hwndModal) return;
        hwndList = GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD);
        if (SendMessage(hwndList, CB_GETDROPPEDSTATE, 0, 0) || SendMessage(hwndList, CB_GETCURSEL, 0, 0) != Biotechnology + 1) return;
        if (!FClick(GetDlgItem(hwndModal, IDCANCEL))) goto LInput;
        break;
    case 180:
        if (hwndModal || pctResGlob != -1) return;
        if (!FSaveOrders("year-2411-research.json")) goto LArtifact;
        iStage = 165; goto LQueued;
    case 181:
        if (!IsWindow(hwndBrowser)) return;
        if (vpartBrowser.hs.grhst != hstTorp || vpartBrowser.hs.iItem != itorpBetaTorpedo) {
            pszFailure = "technology browser did not show the Beta Torpedo"; goto LFail;
        }
        /* FTutorTaskDone can advance the prompt as soon as this window opens. */
        if (!FSaveState("technology-browser")) goto LArtifact;
        LogState("technology-browser-open");
        if (!FClick(GetDlgItem(hwndBrowser, IDCANCEL))) goto LInput;
        break;
    case 182:
        if (IsWindow(hwndBrowser)) return;
        if (!FSaveState("technology-browser-closed")) goto LArtifact;
        LogState("technology-browser-closed");
        iStage = 165; goto LQueued;
    case 183:
        if (!hwndModal || hwndModal->lpfnDlg != VCRDlg || !vlpbdVCR || !vrgtok || vcStepVCR <= 0) return;
        if (!FSaveState("battle-open") || !FSaveBattle("battle-open.json")) goto LArtifact;
        LogState("battle-open");
        iStepBefore = viStepVCRCur;
        if (!FClick(GetDlgItem(hwndModal, IDC_VCR_PLAY_PAUSE))) goto LInput;
        break;
    case 184:
        if (!hwndModal || hwndModal->lpfnDlg != VCRDlg || viStepVCRCur <= iStepBefore || !gd.fVCRTimer) return;
        if (!FSaveState("battle-playing") || !FSaveBattle("battle-playing.json")) goto LArtifact;
        LogState("battle-playing");
        if (!FClick(GetDlgItem(hwndModal, IDC_VCR_PLAY_PAUSE))) goto LInput;
        break;
    case 185:
        if (gd.fVCRTimer) return;
        if (!FClick(GetDlgItem(hwndModal, IDC_VCR_FWD_ALL))) goto LInput;
        break;
    case 186:
        if (viStepVCRCur != vcStepVCR) return;
        if (!FSaveBattle("battle-end.json")) goto LArtifact;
        if (!FClick(GetDlgItem(hwndModal, IDC_VCR_REW_ALL))) goto LInput;
        break;
    case 187:
        if (viStepVCRCur != -1) return;
        if (!FSaveBattle("battle-rewound.json") || !FClick(GetDlgItem(hwndModal, IDC_VCR_FWD))) goto LInput;
        break;
    case 188:
        if (viStepVCRCur <= -1) return;
        if (!FSaveBattle("battle-stepped.json") || !FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 189:
        if (hwndModal || vrgtok || IsWindow(hwndVCRDlg) || gd.fVCRTimer) return;
        if (!FSaveState("battle-closed")) goto LArtifact;
        LogState("battle-closed");
        iStage = 165; goto LQueued;
    case 190:
        if (hwndModal) { pszFailure = "unexpected twelfth-generation dialog"; goto LFail; }
        if (game.turn == 11) return;
        if (game.turn != 12 || !gd.fTutorial || tutor.idt != 296) { pszFailure = "twelfth generation did not reach page thirty-eight"; goto LFail; }
        if (!FSaveState("year-2412")) goto LArtifact;
        if (fTerraformCycle) {
            if (!FSaveOrders("year-2412-start-orders.json")) goto LArtifact;
            iStage = 192; goto LQueued;
        }
        fPassed = fRejected && cPage == 38;
        LogState("battle-cycle-partial");
        KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
    case 191:
        if (iMsgCur == iMsgBefore) return;
        /* Production list selection can satisfy fProgress before this prompt.
         * Read its message and use Goto even if the highlighted task moved. */
        if (iMsgCur < 11) {
            iMsgBefore = iMsgCur;
            if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
            goto LQueued;
        }
        if (iMsgCur != 11 || mdMsgObj != mdMsgObjPart) goto LInput;
        if (!FClick(rghwndMsgBtn[1])) goto LInput;
        iStage = 181; goto LQueued;
    case 192:
        if (game.turn != 12 || hwndModal) { pszFailure = "unexpected terraforming cycle state or dialog"; goto LFail; }
        idtBefore = tutor.idtBold; iMsgBefore = iMsgCur;
        LogState("terraform-cycle-instruction");
        if (tutor.idtBold == 297 || tutor.idtBold == 310 || tutor.idtBold == 313) {
            i = tutor.idtBold == 297 ? 8 : tutor.idtBold == 310 ? 18 : 8;
            if (iMsgCur >= 1 && ((tutor.idtBold == 297 && sel.grobj == grobjFleet && sel.id == i) ||
                (tutor.idtBold != 297 && sel.grobj == grobjPlanet && sel.id == i))) return;
            if (iMsgCur < 1 || mdMsgObj != (tutor.idtBold == 297 ? mdMsgObjFleet : mdMsgObjPlanet) || idMsgObj != i) {
                if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
                iStage = 193; goto LQueued;
            }
            if (!FClick(rghwndMsgBtn[1])) goto LInput;
            iStage = 194; goto LQueued;
        }
        if (tutor.idtBold == 300) {
            if (!FFindMapObject(grobjFleet, 8, NULL) || !FFindSalvageHere()) goto LInput;
            // The original resource includes a trailing space; use the copied label.
            pszMenu = objTarget.szName;
            if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            iStage = 195; goto LQueued;
        }
        if (tutor.idtBold == 303) {
            if (!FPushControlKey(hwndFrame, SDL_SCANCODE_F)) goto LInput;
            iStage = 196; goto LQueued;
        }
        if (tutor.idtBold == 306 || tutor.idtBold == 308) {
            if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
            iStage = 193; goto LQueued;
        }
        if (tutor.idtBold == 316 || tutor.idtBold == 318) {
            if (tutor.idtBold == 318 && (sel.grobj != grobjPlanet || sel.id != 13)) {
                if (!FFindPlanet("Stove Top")) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 194; goto LQueued;
            }
            if (!FPushTextKey(SDL_SCANCODE_Q, "q")) goto LInput;
            iProdPass = tutor.idtBold == 316 ? 12 : 13; cProdAdded = 0;
            iProdReturn = 199; iStage = 68; goto LQueued;
        }
        if (tutor.idtBold == 319) {
            if (!tutor.fTurnDone || !FSaveOrders("year-2412-orders.json") || !FSaveState("before-generate-2412")) goto LArtifact;
            LogState("generate-ready");
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
            iStage = 200; goto LQueued;
        }
        pszFailure = "unimplemented terraforming cycle instruction"; goto LFail;
    case 193:
        if (iMsgCur == iMsgBefore) return;
        if (idtBefore == 297) {
            // The previous VCR leaves this fleet selected, so the tutor can
            // advance after Next alone. Still perform the requested Goto.
            if (iMsgCur < 1 || (mdMsgObj != mdMsgObjBattle && (mdMsgObj != mdMsgObjFleet || idMsgObj != 8)) ||
                gd.fGotoVCR) goto LInput;
            mdProbeMessage = mdMsgObj;
            if (!FSaveState("armed-probe-message") || !FClick(rghwndMsgBtn[1])) goto LInput;
            iStage = 202; goto LQueued;
        }
        iStage = 192; goto LQueued;
    case 194:
        if (tutor.idtBold == idtBefore && !(idtBefore == 318 && sel.grobj == grobjPlanet && sel.id == 13)) return;
        iStage = 192; goto LQueued;
    case 195:
        if (pszMenu || sel.scan.grobj != grobjThing) return;
        if (!FSaveSalvage() || !FSaveState("salvage-summary")) goto LArtifact;
        LogState("salvage-selected");
        iStage = 192; goto LQueued;
    case 196:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != FindDlg || !FSaveState("find-teamster-open")) goto LArtifact;
        if (!FClick(GetDlgItem(hwndModal, IDC_EDIT1))) goto LInput;
        break;
    case 197:
        if (!hwndModal || hwndModal->lpfnDlg != FindDlg) goto LInput;
        hwndList = GetDlgItem(hwndModal, IDC_EDIT1);
        if (GetFocus() != hwndList) return;
        {
            SDL_Event event = {0};
            event.type = SDL_EVENT_TEXT_INPUT;
            event.text.windowID = HwndCompatRoot(hwndList)->id;
            event.text.text = "Teamster #4";
            if (!SDL_PushEvent(&event)) goto LInput;
        }
        break;
    case 198:
        if (!hwndModal || hwndModal->lpfnDlg != FindDlg) goto LInput;
        GetDlgItemText(hwndModal, IDC_EDIT1, szItem, sizeof(szItem));
        if (strcmp(szItem, "Teamster #4")) return;
        if (!FSaveState("find-teamster-entered") || !FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        iStage = 201; goto LQueued;
    case 199:
        if (hwndModal) return;
        if (!FSaveOrders(iProdPass == 12 ? "year-2412-terraform-orders.json" : "year-2412-teamster-orders.json")) goto LArtifact;
        iStage = 192; goto LQueued;
    case 200:
        if (hwndModal) { pszFailure = "unexpected thirteenth-generation dialog"; goto LFail; }
        if (game.turn == 12) return;
        if (game.turn != 13 || !gd.fTutorial || tutor.idt != 320) { pszFailure = "thirteenth generation did not reach page forty-one"; goto LFail; }
        if (!FSaveState("year-2413")) goto LArtifact;
        if (fDesignCycle) {
            if (!FSaveOrders("year-2413-start-orders.json")) goto LArtifact;
            iStage = 203; goto LQueued;
        }
        fPassed = fRejected && cPage == 41;
        LogState("terraform-cycle-partial");
        KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
    case 201:
        if (hwndModal || sel.scan.grobj != grobjFleet || sel.scan.ifl == iflNone || rglpfl[sel.scan.ifl]->id != 3) return;
        if (!FSaveState("find-teamster-closed")) goto LArtifact;
        LogState("teamster-found");
        iStage = 192; goto LQueued;
    case 202:
        if (hwndModal || sel.grobj != grobjFleet || sel.id != 8 || sel.scan.grobj != grobjFleet ||
            sel.scan.ifl == iflNone || rglpfl[sel.scan.ifl]->id != 8 ||
            (mdProbeMessage == mdMsgObjBattle && !gd.fGotoVCR)) return;
        if (!FSaveState("armed-probe-goto")) goto LArtifact;
        LogState("armed-probe-goto");
        iStage = 192; goto LQueued;
    case 203:
        if (game.turn != 13) { pszFailure = "unexpected design cycle year"; goto LFail; }
        idtBefore = tutor.idtBold; iMsgBefore = iMsgCur;
        if (idtDesignBefore != tutor.idtBold) { idtDesignBefore = tutor.idtBold; cDesignVisits = 0; }
        if (++cDesignVisits > 10) { pszFailure = "design instruction repeated without progress"; goto LFail; }
        LogState("design-cycle-instruction");
        if (tutor.idtBold == 320) {
            if (iMsgCur < 1) {
                if (!FClick(rghwndMsgBtn[2])) goto LInput;
                iStage = 204; goto LQueued;
            }
            if (sel.grobj != grobjFleet || sel.id != 0) {
                if (!FFindMapObject(grobjFleet, 0, NULL)) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 204; goto LQueued;
            }
            idCargoFleet = 0; idCargoPlanet = 13; cCargoExpected = 210; iCargoReturn = 203; pszCargoSnapshot = "year-2413-cargo.json";
            if (!FClick(rghwndBtn[7])) goto LInput;
            iStage = 205; goto LQueued;
        }
        if (tutor.idtBold == 321) {
            lpfl = LpflFromId(0);
            if (!lpfl || sel.grobj != grobjFleet || sel.id != 0) goto LInput;
            if (lpfl->cord == 1) {
                if (!FFindPlanet("Slime") || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) ||
                    !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
                iStage = 204; goto LQueued;
            }
            iOrderCombo = sel.fl.lpplord->rgord()[1].grTask != grTaskXfer ? 0 :
                sel.fl.lpplord->rgord()[1].txp.rgia[3].iAction() != iActionUnloadAll ? 1 : 2;
            iOrderRow = iOrderCombo == 0 ? grTaskXfer : iOrderCombo == 1 ? 4 : iActionUnloadAll;
            iOrderReturn = 203; pszOrderSnapshot = "year-2413-transport.json";
            if (!FClick(rghwndOrderDD[iOrderCombo])) goto LInput;
            iStage = 209; goto LQueued;
        }
        if (tutor.idtBold == 322 || tutor.idtBold == 324 || tutor.idtBold == 325) {
            if (hwndModal && hwndModal->lpfnDlg == ResearchDlg) {
                if (!FChooseCombo(GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD), "Propulsion")) goto LInput;
                iDesignReturn = 211; iStage = 221; goto LQueued;
            }
            if (tutor.idtBold == 322 || (tutor.idtBold == 324 && mdMsgObj != mdMsgObjResearch) ||
                (tutor.idtBold == 325 && mdMsgObj != mdMsgObjPart)) {
                if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
                iStage = 204; goto LQueued;
            }
            if (!FClick(rghwndMsgBtn[1])) goto LInput;
            iStage = 204; goto LQueued;
        }
        if (tutor.idtBold == 327) {
            if (IsWindow(hwndBrowser)) {
                if (!FClick(GetDlgItem(hwndBrowser, IDCANCEL))) goto LInput;
                iStage = 204; goto LQueued;
            }
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F4, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F4, FALSE)) goto LInput;
            iStage = 204; goto LQueued;
        }
        if (tutor.idtBold == 328 || tutor.idtBold == 329 || tutor.idtBold == 330) {
            if (!IsWindow(hwndSlotDlg)) goto LInput;
            if (tutor.idtBold == 328) {
                if (!FClick(GetDlgItem(hwndSlotDlg, IDC_DESIGNER_HULLS))) goto LInput;
            } else if (tutor.idtBold == 329) {
                if (!FChooseCombo(GetDlgItem(hwndSlotDlg, IDC_COMBOBOX), "Mini-Miner")) goto LInput;
                iDesignReturn = 204; iStage = 221; goto LQueued;
            } else if (!FClick(GetDlgItem(hwndSlotDlg, IDC_IMPORT))) goto LInput;
            iStage = 204; goto LQueued;
        }
        if (tutor.idtBold == 332 || tutor.idtBold == 333 || tutor.idtBold == 334 || tutor.idtBold == 335) {
            if (!IsWindow(hwndSlotDlg) || mdBuild != mdBuildEdit || !lpshdefBuild) goto LInput;
            iDesignSlot = tutor.idtBold == 332 ? 0 : tutor.idtBold == 333 ? 1 : lpshdefBuild->hul.rghs[2].cItem == 0 ? 2 : 3;
            pszDesignCategory = iDesignSlot == 0 ? "Engines" : iDesignSlot == 1 ? "Scanners" : "Mining Robots";
            pszDesignPart = iDesignSlot == 0 ? "Long Hump 6" : iDesignSlot == 1 ? "Rhino Scanner" : "Robo-Miner";
            if (!FChooseCombo(GetDlgItem(hwndSlotDlg, IDC_COMBOBOX), pszDesignCategory)) goto LInput;
            iDesignReturn = 213; iStage = 221; goto LQueued;
        }
        if (tutor.idtBold == 337 || tutor.idtBold == 338) {
            if (!IsWindow(hwndSlotDlg) || !FSaveState("mini-miner-designed") ||
                !FClick(GetDlgItem(hwndSlotDlg, tutor.idtBold == 337 ? IDOK : IDCANCEL))) goto LInput;
            iStage = 204; goto LQueued;
        }
        if (tutor.idtBold == 339 || tutor.idtBold == 341 || tutor.idtBold == 342) {
            if (sel.grobj != grobjPlanet || sel.id != 13) {
                if (!FFindPlanet("Stove Top")) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 204; goto LQueued;
            }
            iProdPass = tutor.idtBold == 339 ? 14 : 15; cProdAdded = 0; iProdReturn = 218;
            if (!hwndModal && !FPushTextKey(SDL_SCANCODE_Q, "q")) goto LInput;
            iStage = 68; goto LQueued;
        }
        if (tutor.idtBold == 344) {
            hwndList = hwndPlanetProdLB;
            if (!IsWindow(hwndList) || SendMessage(hwndList, LB_GETITEMRECT, 0, (LPARAM)&rcRow) == LB_ERR) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            iProductionInspect = 0;
            if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 219; goto LQueued;
        }
        if (tutor.idtBold == 345) {
            if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
            iStage = 204; goto LQueued;
        }
        if (tutor.idtBold == 348) {
            if (sel.grobj != grobjFleet || sel.id != 8) {
                if (!FFindMapObject(grobjFleet, 8, NULL)) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
            } else if (!FFindPlanet("Stove Top") || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) ||
                !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
            iStage = 204; goto LQueued;
        }
        if (tutor.idtBold == 350) {
            if (!tutor.fTurnDone || !FSaveOrders("year-2413-orders.json") || !FSaveState("before-generate-2413")) goto LArtifact;
            LogState("generate-ready");
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
            iStage = 220; goto LQueued;
        }
        pszFailure = "unimplemented design cycle instruction"; goto LFail;
    case 204:
        if (pszMenu || (idtBefore == tutor.idtBold && iMsgBefore == iMsgCur && cWait < 3)) return;
        iStage = 203; goto LQueued;
    case 205:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != TransferDlg || !pxfer || pxfer[0].id != idCargoFleet || pxfer[1].id != idCargoPlanet) goto LInput;
        for (i = 0; i < crgbtnXfer; i++) {
            if ((rgbtnXfer[i].iVal & 0x7f) == 3 && rgbtnXfer[i].iSide == 0 && !rgbtnXfer[i].fVisible && !(rgbtnXfer[i].bt & 4)) break;
        }
        if (i == crgbtnXfer) goto LInput;
        memcpy(&rcRow, &rgbtnXfer[i].rc, sizeof(rcRow));
        ptRow = (POINT){rcRow.left + 2, (rcRow.top + rcRow.bottom) / 2}; ptTo = (POINT){rcRow.right - 2, ptRow.y};
        if (!FDragPoint(hwndModal, ptRow, ptTo)) goto LInput;
        break;
    case 206:
        if (!hwndModal || !pxfer || pxfer[0].fl.rgwtMin[3] != cCargoExpected) return;
        if (!FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 207:
        if (hwndModal) return;
        if (!FSaveOrders(pszCargoSnapshot)) goto LArtifact;
        if (iCargoReturn == 235) {
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F3, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F3, FALSE)) goto LInput;
            iStage = 237; goto LQueued;
        }
        iStage = iCargoReturn; goto LQueued;
    case 209:
        if (!SendMessage(rghwndOrderDD[iOrderCombo], CB_GETDROPPEDSTATE, 0, 0)) return;
        if (!FClickComboRow(rghwndOrderDD[iOrderCombo], iOrderRow)) goto LInput;
        break;
    case 210:
        if (SendMessage(rghwndOrderDD[iOrderCombo], CB_GETDROPPEDSTATE, 0, 0)) return;
        if (iOrderCombo == 1) {
            iOrderCombo = 2; iOrderRow = iActionUnloadAll;
            if (!FClick(rghwndOrderDD[2])) goto LInput;
            iStage = 209; goto LQueued;
        }
        if (!FSaveOrders(pszOrderSnapshot)) goto LArtifact;
        iStage = iOrderReturn; goto LQueued;
    case 211:
        if (!hwndModal || SendMessage(GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD), CB_GETCURSEL, 0, 0) != iResearchNext + 1) return;
        if (!FClick(GetDlgItem(hwndModal, IDCANCEL))) goto LInput;
        break;
    case 212:
        if (hwndModal) return;
        if (!FSaveOrders(pszResearchSnapshot)) goto LArtifact;
        iStage = iResearchReturn; goto LQueued;
    case 213:
        if (!IsWindow(hwndSlotDlg)) goto LInput;
        hwndList = GetDlgItem(hwndSlotDlg, IDC_DESIGNER_COMPONENT_LIST);
        iDesignRow = -1;
        for (i = 0; i < SendMessage(hwndList, LB_GETCOUNT, 0, 0); i++) {
            if (SendMessage(hwndList, LB_GETTEXTLEN, i, 0) >= (LRESULT)sizeof(szItem)) goto LInput;
            SendMessage(hwndList, LB_GETTEXT, i, (LPARAM)szItem);
            if (strlen(szItem) > 4 && !strcmp(szItem + 4, pszDesignPart)) { iDesignRow = i; break; }
        }
        if (iDesignRow < 0) return;
        if (SendMessage(hwndList, LB_GETITEMRECT, SendMessage(hwndList, LB_GETTOPINDEX, 0, 0), (LPARAM)&rcRow) == LB_ERR) goto LInput;
        ptRow = (POINT){rcRow.right - 4, (rcRow.top + rcRow.bottom) / 2};
        if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
        break;
    case 214:
        hwndList = GetDlgItem(hwndSlotDlg, IDC_DESIGNER_COMPONENT_LIST);
        if (GetFocus() != hwndList) return;
        if (!FPushKey(hwndList, SDL_SCANCODE_HOME, TRUE) || !FPushKey(hwndList, SDL_SCANCODE_HOME, FALSE)) goto LInput;
        for (i = 0; i < iDesignRow; i++) {
            if (!FPushKey(hwndList, SDL_SCANCODE_DOWN, TRUE) || !FPushKey(hwndList, SDL_SCANCODE_DOWN, FALSE)) goto LInput;
        }
        break;
    case 215:
        hwndList = GetDlgItem(hwndSlotDlg, IDC_DESIGNER_COMPONENT_LIST);
        if (SendMessage(hwndList, LB_GETCURSEL, 0, 0) != iDesignRow) return;
        if (SendMessage(hwndList, LB_GETITEMRECT, iDesignRow, (LPARAM)&rcRow) == LB_ERR) goto LInput;
        ptRow = (POINT){32, (rcRow.top + rcRow.bottom) / 2};
        ptTo = (POINT){(vrgrcSlot[iDesignSlot].left + vrgrcSlot[iDesignSlot].right) / 2,
                       (vrgrcSlot[iDesignSlot].top + vrgrcSlot[iDesignSlot].bottom) / 2};
        ClientToScreen(hwndSlotDlg, &ptTo); ScreenToClient(hwndList, &ptTo);
        if (!FDragPoint(hwndList, ptRow, ptTo)) goto LInput;
        break;
    case 216:
        if (!lpshdefBuild || lpshdefBuild->hul.rghs[iDesignSlot].cItem != 1) return;
        if (!FSaveState("mini-miner-part")) goto LArtifact;
        iStage = 203; goto LQueued;
    case 218:
        if (hwndModal) return;
        if (!FSaveOrders(iProdPass == 14 ? "year-2413-miner-queue.json" : "year-2413-mines.json")) goto LArtifact;
        if (iProdPass == 15) {
            // A progress flag can skip the prompt; still inspect the real item.
            hwndList = hwndPlanetProdLB;
            if (!IsWindow(hwndList) || SendMessage(hwndList, LB_GETITEMRECT, 0, (LPARAM)&rcRow) == LB_ERR) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            iProductionInspect = 0;
            if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            iStage = 219; goto LQueued;
        }
        iStage = 203; goto LQueued;
    case 219:
        hwndList = hwndPlanetProdLB;
        if (SendMessage(hwndList, LB_GETCURSEL, 0, 0) != iProductionInspect) return;
        LogState("production-item-inspected");
        if (++iProductionInspect < SendMessage(hwndList, LB_GETCOUNT, 0, 0)) {
            if (SendMessage(hwndList, LB_GETITEMRECT, iProductionInspect, (LPARAM)&rcRow) == LB_ERR) goto LInput;
            ptRow = (POINT){(rcRow.left + rcRow.right) / 2, (rcRow.top + rcRow.bottom) / 2};
            if (!FClickPoint(hwndList, ptRow, SDL_BUTTON_LEFT)) goto LInput;
            goto LQueued;
        }
        if (!FSaveState("production-items")) goto LArtifact;
        iStage = 204; goto LQueued;
    case 220:
        if (hwndModal) { pszFailure = "unexpected fourteenth-generation dialog"; goto LFail; }
        if (game.turn == 13) return;
        if (game.turn != 14 || !gd.fTutorial || tutor.idt != 352) { pszFailure = "fourteenth generation did not reach page forty-five"; goto LFail; }
        if (!FSaveState("year-2414")) goto LArtifact;
        if (fAutomationCycle) {
            if (!FSaveOrders("year-2414-start-orders.json")) goto LArtifact;
            iStage = 224; goto LQueued;
        }
        fPassed = fRejected && cPage == 45;
        LogState("design-cycle-partial");
        KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
    case 221:
        if (!IsWindow(hwndDesignCombo) || !SendMessage(hwndDesignCombo, CB_GETDROPPEDSTATE, 0, 0)) return;
        if (GetFocus() != hwndDesignCombo) return;
        // SDL assigns keyboard targets when translating the event batch.
        // Translate these only after the preceding click establishes focus.
        if (!FPushKey(hwndDesignCombo, SDL_SCANCODE_HOME, TRUE) || !FPushKey(hwndDesignCombo, SDL_SCANCODE_HOME, FALSE)) goto LInput;
        for (i = 0; i < iDesignCombo; i++) {
            if (!FPushKey(hwndDesignCombo, SDL_SCANCODE_DOWN, TRUE) || !FPushKey(hwndDesignCombo, SDL_SCANCODE_DOWN, FALSE)) goto LInput;
        }
        iStage = 223; goto LQueued;
    case 223:
        if (!IsWindow(hwndDesignCombo) || !hwndDesignCombo->lpcombo || hwndDesignCombo->lpcombo->iHot != iDesignCombo) return;
        if (!FClickComboRow(hwndDesignCombo, iDesignCombo)) goto LInput;
        iStage = 222; goto LQueued;
    case 222:
        if (SendMessage(hwndDesignCombo, CB_GETDROPPEDSTATE, 0, 0)) return;
        if (SendMessage(hwndDesignCombo, CB_GETCURSEL, 0, 0) != iDesignCombo) goto LInput;
        iStage = iDesignReturn; goto LQueued;
    case 224:
        if (game.turn != 14) { pszFailure = "unexpected automation year"; goto LFail; }
        idtBefore = tutor.idtBold; iMsgBefore = iMsgCur;
        LogState("automation-cycle-instruction");
        if (tutor.idtBold == 352 || tutor.idtBold == 354) {
            i = tutor.idtBold == 352 ? 17 : 2;
            if (mdMsgObj != mdMsgObjPlanet || idMsgObj != i || (i == 2 && IMsgNext(FALSE) != imsgNone)) {
                if (!IsWindowEnabled(rghwndMsgBtn[2]) || !FClick(rghwndMsgBtn[2])) goto LInput;
            } else if (!FClick(rghwndMsgBtn[1])) goto LInput;
            iStage = 225; goto LQueued;
        }
        if (tutor.idtBold == 356) {
            if (sel.grobj != grobjPlanet || sel.id != 2) goto LInput;
            iProdPass = 16; cProdAdded = 0; iProdReturn = 226;
            if (!FPushTextKey(SDL_SCANCODE_Q, "q")) goto LInput;
            iStage = 68; goto LQueued;
        }
        if (tutor.idtBold == 358) {
            if (!tutor.fTurnDone || !FSaveOrders("year-2414-orders.json") || !FSaveState("before-generate-2414")) goto LArtifact;
            LogState("generate-ready");
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
            iStage = 234; goto LQueued;
        }
        pszFailure = "unimplemented automation instruction"; goto LFail;
    case 225:
        if (tutor.idtBold == idtBefore && iMsgBefore == iMsgCur && cWait < 3) return;
        iStage = 224; goto LQueued;
    case 226:
        if (hwndModal) return;
        if (!FSaveOrders("year-2414-terraform-queue.json") || !FPushTextKey(SDL_SCANCODE_Q, "q")) goto LInput;
        break;
    case 227:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != ProductionDlg) goto LInput;
        pszMenu = "<Customize>";
        ptRow = (POINT){(rcProdDiamond.left + rcProdDiamond.right) / 2, (rcProdDiamond.top + rcProdDiamond.bottom) / 2};
        if (!FClickPoint(hwndModal, ptRow, SDL_BUTTON_RIGHT)) goto LInput;
        break;
    case 228:
        if (pszMenu || !hwndModal || hwndModal->lpfnDlg != ZipProdDlg) return;
        if (!IsWindow(hwndModal->hwndOwner) || hwndModal->hwndOwner->lpfnDlg != ProductionDlg || IsWindowEnabled(hwndModal->hwndOwner)) goto LInput;
        if (!FSaveState("terraform-template-open") || !FClick(GetDlgItem(hwndModal, IDC_ZIP_PROD_PRESET_1))) goto LInput;
        break;
    case 229:
        if (!hwndModal || hwndModal->lpfnDlg != ZipProdDlg || !IsDlgButtonChecked(hwndModal, IDC_ZIP_PROD_PRESET_1)) return;
        if (!FClick(GetDlgItem(hwndModal, IDC_IMPORT))) goto LInput;
        break;
    case 230:
        if (!hwndModal || hwndModal->lpfnDlg != ZipProdDlg || !vrgZipProd[0].fValid || vrgZipProd[0].cpq < 3) return;
        if (vrgZipProd[0].rgpq[0].mdIdle() != iobjMinTerraform || vrgZipProd[0].rgpq[0].cQuan() != 2) return;
        if (!FSaveOrders("year-2414-imported-template.json") || !FSaveState("terraform-template-imported") || !FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 231:
        if (!hwndModal || hwndModal->lpfnDlg != ProductionDlg || !IsWindowEnabled(hwndModal)) return;
        if (!FSaveState("terraform-template-owner-restored") || !FClick(GetDlgItem(hwndModal, IDOK))) goto LInput;
        break;
    case 232:
        if (hwndModal || lpplProdGlob) return;
        if (!FSaveState("terraform-template-closed")) goto LArtifact;
        iStage = 224; goto LQueued;
    case 234:
        if (hwndModal) { pszFailure = "unexpected fifteenth-generation dialog"; goto LFail; }
        if (game.turn == 14) return;
        if (game.turn != 15 || !gd.fTutorial || tutor.idt != 360) { pszFailure = "fifteenth generation did not reach page forty-six"; goto LFail; }
        if (!FSaveState("year-2415")) goto LArtifact;
        if (fReportCycle) {
            if (!FSaveOrders("year-2415-start-orders.json")) goto LArtifact;
            iStage = 235; goto LQueued;
        }
        fPassed = fRejected && cPage == 46;
        LogState("automation-cycle-partial");
        KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
    case 235:
        if (game.turn != 15) { pszFailure = "unexpected report cycle year"; goto LFail; }
        idtBefore = tutor.idtBold; iMsgBefore = iMsgCur;
        LogState("report-cycle-instruction");
        if (tutor.idtBold == 360 || tutor.idtBold == 361 || tutor.idtBold == 372) {
            idflAction = tutor.idtBold == 360 ? 8 : tutor.idtBold == 361 ? 6 : 11;
            if (tutor.idtBold == 372 && IMsgNext(FALSE) != imsgNone) {
                if (!FClick(rghwndMsgBtn[2])) goto LInput;
                iStage = 236; goto LQueued;
            }
            if (sel.grobj != grobjFleet || sel.id != idflAction) {
                if (mdMsgObj == mdMsgObjFleet && idMsgObj == idflAction) {
                    if (!FClick(rghwndMsgBtn[1])) goto LInput;
                } else {
                    if (!FFindMapObject(grobjFleet, idflAction, NULL)) goto LInput;
                    pszMenu = objTarget.szName;
                    if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                }
                iStage = 236; goto LQueued;
            }
            if (idflAction == 6) {
                idCargoFleet = 6; idCargoPlanet = 13; cCargoExpected = 210; iCargoReturn = 235; pszCargoSnapshot = "year-2415-cargo.json";
                if (!FClick(rghwndBtn[7])) goto LInput;
                iStage = 205; goto LQueued;
            }
            if (!FFindPlanet(idflAction == 8 ? "Neil" : "Stove Top") || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) ||
                !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
            iStage = 236; goto LQueued;
        }
        if (tutor.idtBold == 363 || tutor.idtBold == 364) {
            if (!IsWindow(hwndReportDlg) && (!FPushKey(hwndFrame, SDL_SCANCODE_F3, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F3, FALSE))) goto LInput;
            iStage = 237; goto LQueued;
        }
        if (tutor.idtBold == 366) {
            if (IsWindow(hwndReportDlg) && (!FPushKey(hwndReportDlg, SDL_SCANCODE_ESCAPE, TRUE) || !FPushKey(hwndReportDlg, SDL_SCANCODE_ESCAPE, FALSE))) goto LInput;
            iStage = 240; goto LQueued;
        }
        if (tutor.idtBold == 367) {
            lpfl = LpflFromId(6);
            if (!lpfl || sel.grobj != grobjFleet || sel.id != 6) goto LInput;
            if (lpfl->cord == 1) {
                if (!FFindPlanet("Wallaby") || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) ||
                    !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) || !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
                iStage = 236; goto LQueued;
            }
            iOrderCombo = lpfl->lpplord->rgord()[1].grTask != grTaskXfer ? 0 : 1;
            iOrderRow = iOrderCombo == 0 ? grTaskXfer : 4; iOrderReturn = 235; pszOrderSnapshot = "year-2415-transport.json";
            if (!FClick(rghwndOrderDD[iOrderCombo])) goto LInput;
            iStage = 209; goto LQueued;
        }
        if (tutor.idtBold == 368) {
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F5, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F5, FALSE)) goto LInput;
            iStage = 241; goto LQueued;
        }
        if (tutor.idtBold == 371) {
            if (!hwndModal || hwndModal->lpfnDlg != ResearchDlg || !FClick(GetDlgItem(hwndModal, IDCANCEL))) goto LInput;
            iStage = 242; goto LQueued;
        }
        if (tutor.idtBold == 374) {
            if (!fValueReportChecked || !tutor.fTurnDone || !FSaveOrders("year-2415-orders.json") || !FSaveState("before-generate-2415")) goto LArtifact;
            LogState("generate-ready");
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
            iStage = 243; goto LQueued;
        }
        pszFailure = "unimplemented report cycle instruction"; goto LFail;
    case 236:
        if (pszMenu || (idtBefore == tutor.idtBold && iMsgBefore == iMsgCur && cWait < 3)) return;
        iStage = 235; goto LQueued;
    case 237:
        if (!IsWindow(hwndReportDlg) || !vprptCur) return;
        if (vprptCur->irpt != rptPlanets) goto LInput;
        if (!GetClientRect(hwndReportDlg, &rcRow)) goto LInput;
        if (rcRow.right < 1000) {
            if (!SetWindowPos(hwndReportDlg, NULL, 0, 0, 1280, 700, SWP_NOZORDER)) goto LInput;
            return;
        }
        if (!FSaveState("value-report-open")) goto LArtifact;
        ptRow = (POINT){2, (dyArial8 + 8) / 2};
        for (i = 0; i < 4; i++) {
            if ((vprptCur->grbitVisible & (1 << i)) && (i == 0 || i >= vprptCur->cFieldFirst)) ptRow.x += vprptCur->rgbdx[i] * 2;
        }
        ptRow.x += vprptCur->rgbdx[4];
        pszMenu = "Sort by Value";
        if (!FClickPoint(hwndReportDlg, ptRow, SDL_BUTTON_RIGHT)) goto LInput;
        break;
    case 238:
        if (pszMenu || !vprptCur || vprptCur->icolSort != 4) return;
        if (!FSaveReport("value-report.json") || !FSaveState("value-report-sorted") ||
            !FPushKey(hwndReportDlg, SDL_SCANCODE_ESCAPE, TRUE) || !FPushKey(hwndReportDlg, SDL_SCANCODE_ESCAPE, FALSE)) goto LInput;
        iStage = 240; goto LQueued;
    case 240:
        if (IsWindow(hwndReportDlg) || vprptCur) return;
        if (!FSaveState("value-report-closed")) goto LArtifact;
        fValueReportChecked = TRUE;
        iStage = 235; goto LQueued;
    case 241:
        if (!hwndModal) return;
        if (hwndModal->lpfnDlg != ResearchDlg || !FSaveState("propulsion-research-open")) goto LInput;
        iStage = 235; goto LQueued;
    case 242:
        if (hwndModal) return;
        if (!FSaveOrders("year-2415-research.json")) goto LArtifact;
        iStage = 235; goto LQueued;
    case 243:
        if (hwndModal) { pszFailure = "unexpected sixteenth-generation dialog"; goto LFail; }
        if (game.turn == 15) return;
        if (game.turn != 16 || !gd.fTutorial || tutor.idt != 376) { pszFailure = "sixteenth generation did not reach page forty-eight"; goto LFail; }
        if (!FSaveState("year-2416")) goto LArtifact;
        if (fMergeCycle) {
            if (!FSaveOrders("year-2416-start-orders.json")) goto LArtifact;
            iStage = 244; goto LQueued;
        }
        fPassed = fRejected && cPage == 48;
        LogState("report-cycle-partial");
        KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
    case 244:
        if (game.turn != 16 && game.turn != 17) { pszFailure = "unexpected merge cycle year"; goto LFail; }
        idtBefore = tutor.idtBold; iMsgBefore = iMsgCur;
        LogState("merge-cycle-instruction");
        if (tutor.idtBold == 376 || tutor.idtBold == 377 || tutor.idtBold == 378 || tutor.idtBold == 385) {
            idflAction = tutor.idtBold == 376 ? 4 : tutor.idtBold == 385 ? 0 : 7;
            if (sel.grobj != grobjFleet || sel.id != idflAction) {
                if (mdMsgObj == mdMsgObjFleet && idMsgObj == idflAction) {
                    if (!FClick(rghwndMsgBtn[1])) goto LInput;
                } else {
                    if (!FFindMapObject(grobjFleet, idflAction, NULL)) goto LInput;
                    pszMenu = objTarget.szName;
                    if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                }
                iStage = 245; goto LQueued;
            }
            lpfl = LpflFromId(idflAction);
            if (!lpfl) goto LInput;
            if (lpfl->cord == 1) {
                if (!FFindPlanet(idflAction == 4 ? "Wallaby" : idflAction == 0 ? "Stove Top" : "Prune") ||
                    !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, TRUE) || !FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_LEFT) ||
                    !FPushKey(hwndScanner, SDL_SCANCODE_LSHIFT, FALSE)) goto LInput;
                iStage = 245; goto LQueued;
            }
            if (idflAction == 7) {
                if (lpfl->lpplord->rgord()[1].grTask != grTaskMerge) {
                    iOrderCombo = 0; iOrderRow = grTaskMerge; iOrderReturn = 244; pszOrderSnapshot = "year-2416-merge.json";
                    if (!FClick(rghwndOrderDD[0])) goto LInput;
                    iStage = 209; goto LQueued;
                }
                if (lpfl->lpplord->rgord()[1].grobj != grobjFleet || lpfl->lpplord->rgord()[1].id != 5) {
                    pszMenu = "Cotton Picker #6";
                    ptRow = (POINT){(rgrcRef[12].left + rgrcRef[12].right) / 2, (rgrcRef[12].top + rgrcRef[12].bottom) / 2};
                    if (!FClickPoint(hwndPlanet, ptRow, SDL_BUTTON_RIGHT)) goto LInput;
                    iStage = 245; goto LQueued;
                }
            }
            iStage = 245; goto LQueued;
        }
        if (tutor.idtBold == 380 || tutor.idtBold == 381) {
            if (sel.grobj != grobjPlanet || sel.id != 13) {
                if (!FFindPlanet("Stove Top")) goto LInput;
                pszMenu = objTarget.szName;
                if (!FClickPoint(hwndScanner, objTarget.ptClient, SDL_BUTTON_RIGHT)) goto LInput;
                iStage = 245; goto LQueued;
            }
            iProdPass = 17; cProdAdded = 0; iProdReturn = 246;
            if (LpplFromId(13)->lpplprod && LpplFromId(13)->lpplprod->iprodMac > 0 &&
                LpplFromId(13)->lpplprod->rgprod()[0].grobj == grobjPlanet && LpplFromId(13)->lpplprod->rgprod()[0].iItem == iobjFactory)
                cProdAdded = LpplFromId(13)->lpplprod->rgprod()[0].cItem;
            if (!FPushTextKey(SDL_SCANCODE_Q, "q")) goto LInput;
            iStage = 68; goto LQueued;
        }
        if (tutor.idtBold == 382) {
            if (!hwndModal) {
                if (!FPushKey(hwndFrame, SDL_SCANCODE_F5, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F5, FALSE)) goto LInput;
                iStage = 245; goto LQueued;
            }
            if (hwndModal->lpfnDlg != ResearchDlg || !FChooseCombo(GetDlgItem(hwndModal, IDC_RESEARCH_NEXT_FIELD), "Construction")) goto LInput;
            iResearchNext = Construction; iResearchReturn = 244; pszResearchSnapshot = "year-2416-research.json";
            iDesignReturn = 211; iStage = 221; goto LQueued;
        }
        if (tutor.idtBold == 384 || tutor.idtBold == 383 || tutor.idtBold == 387) {
            if (IMsgNext(FALSE) != imsgNone) {
                if (!FClick(rghwndMsgBtn[2])) goto LInput;
                iStage = 245; goto LQueued;
            }
            if (tutor.idtBold == 384) { iStage = 245; goto LQueued; }
            if (!tutor.fTurnDone) return;
            if (!FSaveOrders(game.turn == 16 ? "year-2416-orders.json" : "year-2417-orders.json") ||
                !FSaveState(game.turn == 16 ? "before-generate-2416" : "before-generate-2417")) goto LArtifact;
            LogState("generate-ready");
            if (!FPushKey(hwndFrame, SDL_SCANCODE_F9, TRUE) || !FPushKey(hwndFrame, SDL_SCANCODE_F9, FALSE)) goto LInput;
            iStage = game.turn == 16 ? 247 : 248; goto LQueued;
        }
        pszFailure = "unimplemented merge cycle instruction"; goto LFail;
    case 245:
        if (pszMenu || (idtBefore == tutor.idtBold && iMsgBefore == iMsgCur && cWait < 3)) return;
        iStage = 244; goto LQueued;
    case 246:
        if (hwndModal) return;
        if (iProdPass == 17) {
            iProdPass = 18; cProdAdded = 0; iProdReturn = 246;
            if (!FPushTextKey(SDL_SCANCODE_Q, "q")) goto LInput;
            iStage = 68; goto LQueued;
        }
        if (!FSaveOrders("year-2416-production.json")) goto LArtifact;
        iStage = 244; goto LQueued;
    case 247:
        if (hwndModal) { pszFailure = "unexpected seventeenth-generation dialog"; goto LFail; }
        if (game.turn == 16) return;
        if (game.turn != 17 || !gd.fTutorial || tutor.idt != 384) { pszFailure = "seventeenth generation did not reach page forty-nine"; goto LFail; }
        if (!FSaveState("year-2417") || !FSaveOrders("year-2417-start-orders.json")) goto LArtifact;
        iStage = 244; goto LQueued;
    case 248:
        if (hwndModal) { pszFailure = "unexpected eighteenth-generation dialog"; goto LFail; }
        if (game.turn == 17) return;
        if (game.turn != 18 || !gd.fTutorial || tutor.idt != 392) { pszFailure = "eighteenth generation did not reach page fifty"; goto LFail; }
        if (!FSaveState("year-2418")) goto LArtifact;
        fPassed = fRejected && cPage == 50;
        LogState("merge-cycle-partial");
        KillTimer(NULL, id); PostQuitMessage(fPassed ? 0 : 93); return;
    default:
        pszFailure = "unknown runner stage";
        goto LFail;
    }
    LogState("action");
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
    LogState("failed");
    FSaveState("failure");
    fprintf(stderr, "%s at stage %d, instruction %d\n", pszFailure, iStage, tutor.idtBold);
    KillTimer(NULL, id);
    PostQuitMessage(93);
}

int16_t StarsTest_InitInstance(int16_t nCmdShow) {
    int16_t fResult = StarsTestReal_InitInstance(nCmdShow);
    if (fResult) idTimer = SetTimer(NULL, 0, 50, RunTutorial);
    return fResult && idTimer ? fResult : FALSE;
}

int main(int argc, char **argv) {
    char szCommand[] = "";
    int nResult;
    NATIVEFILE *lpfile;
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
    lpEvents = PfNativeOpen("events.jsonl", "w");
    if (!lpEvents) return 1;
    setvbuf(stdout, NULL, _IONBF, 0);
    nResult = WinMain(GetModuleHandle(NULL), NULL, szCommand, SW_SHOWNORMAL);
    if (idTimer) KillTimer(NULL, idTimer);
    lpfile = PfNativeOpen("result.json", "w");
    if (lpfile) {
        CchNativePrintf(lpfile, "{\"scenario\":\"%s\",\"status\":\"%s\",\"fullTutorial\":false,\"actions\":%d,\"pages\":%d,\"rejectedGenerate\":%s,\"detail\":\"%s\"}\n",
                fMergeCycle ? "merge-cycle" : fReportCycle ? "report-cycle" : fAutomationCycle ? "automation-cycle" : fDesignCycle ? "design-cycle" : fTerraformCycle ? "terraform-cycle" : fBattleCycle ? "battle-cycle" : fProdTemplates ? "production-templates" : fOxygenColony ? "oxygen-colonization" : fResearchCycle ? "research-cycle" : fFleetSplit ? "fleet-split" : fExpansion ? "expansion" : fTransportCycle ? "transport-cycle" : fSecondColonization ? "second-colonization" : fColonizerProduction ? "colonizer-production" : fLogistics ? "logistics" : fColonization ? "colonization" : fMining ? "mining" : fScouting ? "scouting" : fTwoYears ? "two-years" : fFirstYear ? "first-year" : "smoke", fPassed && nResult == 0 ? (fFirstYear ? "partial" : "passed") : "failed",
                cActions, cPage, fRejected ? "true" : "false",
                fPassed ? "bounded scenario; complete tutorial remains unverified" : pszFailure);
        if (ICloseNativeFile(lpfile)) fPassed = FALSE;
    } else fPassed = FALSE;
    ICloseNativeFile(lpEvents);
    SDL_SetModState(SDL_KMOD_NONE);
    StarsCompatProcessExit();
    SDL_Quit();
    return fPassed && nResult == 0 ? (fFirstYear ? 2 : 0) : 1;
}
