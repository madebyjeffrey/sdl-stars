/* Loaded-game UI checks, included after the shared mouse and modal helpers. */
static int ishdefHull, iSlotHull, iPartHull, cDesignHull;
static HS hsPartHull;
static BOOL fBaseHull, fAcceptHull;
static const char szHullName[] = "Compat Hull";

static BOOL FPushHullDrag(HWND hwndList, HWND hwndDialog, BOOL fRemove) {
    HWND hwndRoot = HwndCompatRoot(hwndDialog);
    POINT ptStart = {32, 34}, ptEnd;
    SDL_Event event = {};

    ptEnd = (POINT){(vrgrcSlot[iSlotHull].left + vrgrcSlot[iSlotHull].right) / 2,
                    (vrgrcSlot[iSlotHull].top + vrgrcSlot[iSlotHull].bottom) / 2};
    if (fRemove) { ptStart = ptEnd; ptEnd = (POINT){4, 320}; }
    if (!ClientToScreen(fRemove ? hwndDialog : hwndList, &ptStart) || !ScreenToClient(hwndRoot, &ptStart) ||
        !ClientToScreen(hwndDialog, &ptEnd) || !ScreenToClient(hwndRoot, &ptEnd)) return FALSE;
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.windowID = hwndRoot->id;
    event.button.button = SDL_BUTTON_LEFT; event.button.down = true;
    event.button.x = (float)ptStart.x; event.button.y = (float)ptStart.y;
    if (!SDL_PushEvent(&event)) return FALSE;
    SDL_zero(event);
    event.type = SDL_EVENT_MOUSE_MOTION; event.motion.windowID = hwndRoot->id;
    event.motion.state = SDL_BUTTON_LMASK;
    event.motion.x = (float)ptEnd.x; event.motion.y = (float)ptEnd.y;
    if (!SDL_PushEvent(&event)) return FALSE;
    SDL_zero(event);
    event.type = SDL_EVENT_MOUSE_BUTTON_UP; event.button.windowID = hwndRoot->id;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = (float)ptEnd.x; event.button.y = (float)ptEnd.y;
    return SDL_PushEvent(&event);
}

static BOOL FCheckHullWorkflow(HWND hwndDialog) {
    HWND hwndControl;
    HULDEF *lphul;
    SHDEF *lpshdef;
    SDL_Event event = {};
    char sz[512];
    int i, j, cRow, iRow;
    uint16_t grhst;

    if (hwndDialog->lpfnDlg != SlotDlg) return FALSE;
    if (iWorkflowStep == 0) {
        fBaseHull = iWorkflow >= 22; fAcceptHull = (iWorkflow & 1) != 0;
        cDesignHull = fBaseHull ? rgplr[idPlayer].cshdefSB : rgplr[idPlayer].cShDef;
        return FPushClick(GetDlgItem(hwndDialog, fBaseHull ? IDC_DESIGNER_STARBASES : IDC_DESIGNER_SHIPS));
    }
    if (fStarbaseMode != fBaseHull) return FALSE;
    if (iWorkflowStep == 1) return FPushClick(GetDlgItem(hwndDialog, IDC_DESIGNER_HULLS));
    if (iWorkflowStep == 2) {
        hwndControl = GetDlgItem(hwndDialog, IDC_COMBOBOX);
        return mdBuild == mdBuildHuldef && SendMessage(hwndControl, CB_GETCOUNT, 0, 0) > 0 && FPushClick(hwndControl);
    }
    if (iWorkflowStep == 3) return FPushComboRow(GetDlgItem(hwndDialog, IDC_COMBOBOX), 0);
    if (iWorkflowStep == 4) {
        if (mdBuild != mdBuildHuldef || lpshdefBuild != &shdefBuild ||
            (shdefBuild.hul.ihuldef >= 32) != fBaseHull) return FALSE;
        return FPushClick(GetDlgItem(hwndDialog, IDC_IMPORT));
    }
    if (iWorkflowStep == 5) {
        if (mdBuild != mdBuildEdit || !fHullCopy || lpshdefBuild != &shdefBuild) return FALSE;
        ishdefHull = ishdefBuild;
        if ((fBaseHull ? rgplr[idPlayer].cshdefSB : rgplr[idPlayer].cShDef) != cDesignHull + 1) return FALSE;
        lphul = LphuldefFromId(shdefBuild.hul.ihuldef);
        hwndControl = GetDlgItem(hwndDialog, IDC_DESIGNER_COMPONENT_LIST);
        cRow = (int)SendMessage(hwndControl, LB_GETCOUNT, 0, 0);
        for (i = 0; i < cRow; i++) {
            if (SendMessage(hwndControl, LB_GETTEXTLEN, i, 0) >= (LRESULT)sizeof(sz)) return FALSE;
            if (SendMessage(hwndControl, LB_GETTEXT, i, (LPARAM)sz) < 4 || sz[0] < 'A' || sz[0] > 'P') return FALSE;
            grhst = (uint16_t)(1U << (sz[0] - 'A'));
            for (j = 0; j < lphul->hul.chs; j++) {
                if (!(lphul->hul.rghs[j].grhst & grhst) || (!fBaseHull && j != 0)) continue;
                iPartHull = i; iSlotHull = j;
                hsPartHull = (HS){}; hsPartHull.grhst = (HullSlotType)(grhst); hsPartHull.iItem = sz[1] - 'A';
                hsPartHull.cItem = (grhst & hstEngine) ? lphul->hul.rghs[j].cItem : 1;
                SendMessage(hwndControl, LB_SETTOPINDEX, iPartHull, 0);
                return TRUE;
            }
        }
        return FALSE;
    }
    if (iWorkflowStep == 6) return FPushHullDrag(GetDlgItem(hwndDialog, IDC_DESIGNER_COMPONENT_LIST), hwndDialog, FALSE);
    if (iWorkflowStep == 8) {
        if (shdefBuild.hul.rghs[iSlotHull].cItem != 0 || GetCapture()) return FALSE;
        return FPushHullDrag(GetDlgItem(hwndDialog, IDC_DESIGNER_COMPONENT_LIST), hwndDialog, FALSE);
    }
    if (iWorkflowStep == 7 || iWorkflowStep == 9) {
        if (GetCapture() || memcmp(&shdefBuild.hul.rghs[iSlotHull], &hsPartHull, sizeof(HS))) {
            fprintf(stderr, "Hull drop: slot %d wanted %u/%u/%u got %u/%u/%u capture %p\n", iSlotHull,
                    hsPartHull.grhst, hsPartHull.iItem, hsPartHull.cItem, shdefBuild.hul.rghs[iSlotHull].grhst,
                    shdefBuild.hul.rghs[iSlotHull].iItem, shdefBuild.hul.rghs[iSlotHull].cItem, (void *)GetCapture());
            return FALSE;
        }
        if (iWorkflowStep == 7) return FPushHullDrag(GetDlgItem(hwndDialog, IDC_DESIGNER_COMPONENT_LIST), hwndDialog, TRUE);
        return FPushClick(GetDlgItem(hwndDialog, IDC_EDITNAME));
    }
    if (iWorkflowStep == 10) {
        hwndControl = GetDlgItem(hwndDialog, IDC_EDITNAME);
        if (GetFocus() != hwndControl) return FALSE;
        SendMessage(hwndControl, EM_SETSEL, 0, -1);
        event.type = SDL_EVENT_TEXT_INPUT; event.text.windowID = HwndCompatRoot(hwndDialog)->id;
        event.text.text = szHullName;
        return SDL_PushEvent(&event);
    }
    if (iWorkflowStep == 11) {
        if (strcmp(shdefBuild.hul.szClass, szHullName)) return FALSE;
        return FPushClick(GetDlgItem(hwndDialog, fAcceptHull ? IDOK : IDCANCEL));
    }
    lpshdef = fBaseHull ? &rglpshdefSB[idPlayer][ishdefHull - 16] : &rgshdef[ishdefHull];
    if (mdBuild != mdBuildShdef) return FALSE;
    if (iWorkflowStep == 12) {
        if (!fAcceptHull) {
            if (!lpshdef->fFree || (fBaseHull ? rgplr[idPlayer].cshdefSB : rgplr[idPlayer].cShDef) != cDesignHull) return FALSE;
            fWorkflowPassed = FPushClick(GetDlgItem(hwndDialog, IDCANCEL));
            return fWorkflowPassed;
        }
        if (lpshdef->fFree || strcmp(lpshdef->hul.szClass, szHullName) ||
            memcmp(&lpshdef->hul.rghs[iSlotHull], &hsPartHull, sizeof(HS))) return FALSE;
        return FPushClick(GetDlgItem(hwndDialog, IDC_COMBOBOX));
    }
    if (iWorkflowStep == 13) {
        hwndControl = GetDlgItem(hwndDialog, IDC_COMBOBOX);
        iRow = (int)SendMessage(hwndControl, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)szHullName);
        if (iRow < 0) return FALSE;
        CompatListProc(hwndControl, LB_SETTOPINDEX, iRow, 0);
        return FPushComboRow(hwndControl, iRow);
    }
    if (iWorkflowStep == 14) return lpshdefBuild == lpshdef && FPushClick(GetDlgItem(hwndDialog, IDC_DELETE));
    if (!lpshdef->fFree || (fBaseHull ? rgplr[idPlayer].cshdefSB : rgplr[idPlayer].cShDef) != cDesignHull) return FALSE;
    fWorkflowPassed = FPushClick(GetDlgItem(hwndDialog, IDCANCEL));
    return fWorkflowPassed;
}

static BOOL FCheckDesignerWorkflows(void) {
    SHDEF rgShipSav[16], rgBaseSav[10];
    SHDEF *lpshdef, *lpSav;
    int i, j, cShipSav = rgplr[idPlayer].cShDef, cBaseSav = rgplr[idPlayer].cshdefSB;

    memcpy(rgShipSav, rgshdef, sizeof(rgShipSav));
    memcpy(rgBaseSav, rglpshdefSB[idPlayer], sizeof(rgBaseSav));
    for (i = 20; i <= 23; i++) if (!FRunWorkflow(i, 0)) return FALSE;
    if (rgplr[idPlayer].cShDef != cShipSav || rgplr[idPlayer].cshdefSB != cBaseSav) return FALSE;
    for (j = 0; j < 2; j++) {
        for (i = 0; i < (j == 0 ? 16 : 10); i++) {
            lpshdef = j == 0 ? &rgshdef[i] : &rglpshdefSB[idPlayer][i];
            lpSav = j == 0 ? &rgShipSav[i] : &rgBaseSav[i];
            if (!lpSav->fFree && lpSav->lPower == -1 && lpshdef->lPower != -1) lpSav->lPower = LComputePower(lpSav);
            if (lpshdef->fFree != lpSav->fFree || (!lpSav->fFree && memcmp(lpshdef, lpSav, sizeof(SHDEF)))) return FALSE;
        }
    }
    puts("Hull selection, component drag/remove/reinsert, ship/starbase accept/cancel/delete and original design preservation passed");
    return TRUE;
}

static BOOL FCheckSaveReloadWorkflow(void) {
    int iResearch = (rgplr[idPlayer].iTechNow + 1) % 6, iPlayerSav = idPlayer;
    int32_t lidSav = game.lid;
    char szLog[512];

    if (!FRunWorkflow(1, iResearch) || rgplr[idPlayer].iTechNow != iResearch) return FALSE;
    SendMessage(hwndFrame, WM_COMMAND, IDM_FILE_SAVE, 0);
    snprintf(szLog, sizeof(szLog), "%s.x%d", szBase, idPlayer + 1);
    if (!FFileExists(szLog) || game.fDirty) return FALSE;
    // Reload through the actual MRU command: FLoadGame resets the world before
    // replaying the saved orders, so the new research choice must persist.
    rgplr[idPlayer].iTechNow = (iResearch + 1) % 6;
    SendMessage(hwndFrame, WM_COMMAND, IDM_FILE_MRU1, 0);
    if (game.lid != lidSav || idPlayer != iPlayerSav || !IsWindow(hwndScanner) ||
        !IsWindowEnabled(hwndFrame) || rgplr[idPlayer].iTechNow != iResearch) return FALSE;
    puts("Save orders and MRU reload preserve the committed research choice and game identity");
    return TRUE;
}
