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
    if (!win32::ClientToScreen(fRemove ? hwndDialog : hwndList, &ptStart) || !win32::ScreenToClient(hwndRoot, &ptStart) ||
        !win32::ClientToScreen(hwndDialog, &ptEnd) || !win32::ScreenToClient(hwndRoot, &ptEnd)) return 0;
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.windowID = hwndRoot->id;
    event.button.button = SDL_BUTTON_LEFT; event.button.down = true;
    event.button.x = (float)ptStart.x; event.button.y = (float)ptStart.y;
    if (!SDL_PushEvent(&event)) return 0;
    SDL_zero(event);
    event.type = SDL_EVENT_MOUSE_MOTION; event.motion.windowID = hwndRoot->id;
    event.motion.state = SDL_BUTTON_LMASK;
    event.motion.x = (float)ptEnd.x; event.motion.y = (float)ptEnd.y;
    if (!SDL_PushEvent(&event)) return 0;
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

    if (hwndDialog->lpfnDlg != SlotDlg) return 0;
    if (iWorkflowStep == 0) {
        fBaseHull = iWorkflow >= 22; fAcceptHull = (iWorkflow & 1) != 0;
        cDesignHull = fBaseHull ? rgplr[idPlayer].cshdefSB : rgplr[idPlayer].cShDef;
        return FPushClick(win32::GetDlgItem(hwndDialog, fBaseHull ? IDC_DESIGNER_STARBASES : IDC_DESIGNER_SHIPS));
    }
    if (fStarbaseMode != fBaseHull) return 0;
    if (iWorkflowStep == 1) return FPushClick(win32::GetDlgItem(hwndDialog, IDC_DESIGNER_HULLS));
    if (iWorkflowStep == 2) {
        hwndControl = win32::GetDlgItem(hwndDialog, IDC_COMBOBOX);
        return mdBuild == mdBuildHuldef && win32::SendMessageA(hwndControl, win32::CB_GETCOUNT, 0, 0) > 0 && FPushClick(hwndControl);
    }
    if (iWorkflowStep == 3) return FPushComboRow(win32::GetDlgItem(hwndDialog, IDC_COMBOBOX), 0);
    if (iWorkflowStep == 4) {
        if (mdBuild != mdBuildHuldef || lpshdefBuild != &shdefBuild ||
            (shdefBuild.hul.ihuldef >= 32) != fBaseHull) return 0;
        return FPushClick(win32::GetDlgItem(hwndDialog, IDC_IMPORT));
    }
    if (iWorkflowStep == 5) {
        if (mdBuild != mdBuildEdit || !fHullCopy || lpshdefBuild != &shdefBuild) return 0;
        ishdefHull = ishdefBuild;
        if ((fBaseHull ? rgplr[idPlayer].cshdefSB : rgplr[idPlayer].cShDef) != cDesignHull + 1) return 0;
        lphul = LphuldefFromId(shdefBuild.hul.ihuldef);
        hwndControl = win32::GetDlgItem(hwndDialog, IDC_DESIGNER_COMPONENT_LIST);
        cRow = (int)win32::SendMessageA(hwndControl, win32::LB_GETCOUNT, 0, 0);
        for (i = 0; i < cRow; i++) {
            if (win32::SendMessageA(hwndControl, win32::LB_GETTEXTLEN, i, 0) >= (LRESULT)sizeof(sz)) return 0;
            if (win32::SendMessageA(hwndControl, win32::LB_GETTEXT, i, (LPARAM)sz) < 4 || sz[0] < 'A' || sz[0] > 'P') return 0;
            grhst = (uint16_t)(1U << (sz[0] - 'A'));
            for (j = 0; j < lphul->hul.chs; j++) {
                if (!(lphul->hul.rghs[j].grhst & grhst) || (!fBaseHull && j != 0)) continue;
                iPartHull = i; iSlotHull = j;
                hsPartHull = (HS){}; hsPartHull.grhst = (HullSlotType)(grhst); hsPartHull.iItem = sz[1] - 'A';
                hsPartHull.cItem = (grhst & hstEngine) ? lphul->hul.rghs[j].cItem : 1;
                win32::SendMessageA(hwndControl, win32::LB_SETTOPINDEX, iPartHull, 0);
                return 1;
            }
        }
        return 0;
    }
    if (iWorkflowStep == 6) return FPushHullDrag(win32::GetDlgItem(hwndDialog, IDC_DESIGNER_COMPONENT_LIST), hwndDialog, 0);
    if (iWorkflowStep == 8) {
        if (shdefBuild.hul.rghs[iSlotHull].cItem != 0 || win32::GetCapture()) return 0;
        return FPushHullDrag(win32::GetDlgItem(hwndDialog, IDC_DESIGNER_COMPONENT_LIST), hwndDialog, 0);
    }
    if (iWorkflowStep == 7 || iWorkflowStep == 9) {
        if (win32::GetCapture() || memcmp(&shdefBuild.hul.rghs[iSlotHull], &hsPartHull, sizeof(HS))) {
            fprintf(stderr, "Hull drop: slot %d wanted %u/%u/%u got %u/%u/%u capture %p\n", iSlotHull,
                    hsPartHull.grhst, hsPartHull.iItem, hsPartHull.cItem, shdefBuild.hul.rghs[iSlotHull].grhst,
                    shdefBuild.hul.rghs[iSlotHull].iItem, shdefBuild.hul.rghs[iSlotHull].cItem, (void *)win32::GetCapture());
            return 0;
        }
        if (iWorkflowStep == 7) return FPushHullDrag(win32::GetDlgItem(hwndDialog, IDC_DESIGNER_COMPONENT_LIST), hwndDialog, 1);
        return FPushClick(win32::GetDlgItem(hwndDialog, IDC_EDITNAME));
    }
    if (iWorkflowStep == 10) {
        hwndControl = win32::GetDlgItem(hwndDialog, IDC_EDITNAME);
        if (win32::GetFocus() != hwndControl) return 0;
        win32::SendMessageA(hwndControl, win32::EM_SETSEL, 0, -1);
        event.type = SDL_EVENT_TEXT_INPUT; event.text.windowID = HwndCompatRoot(hwndDialog)->id;
        event.text.text = szHullName;
        return SDL_PushEvent(&event);
    }
    if (iWorkflowStep == 11) {
        if (strcmp(shdefBuild.hul.szClass, szHullName)) return 0;
        return FPushClick(win32::GetDlgItem(hwndDialog, fAcceptHull ? IDOK : IDCANCEL));
    }
    lpshdef = fBaseHull ? &rglpshdefSB[idPlayer][ishdefHull - 16] : &rgshdef[ishdefHull];
    if (mdBuild != mdBuildShdef) return 0;
    if (iWorkflowStep == 12) {
        if (!fAcceptHull) {
            if (!lpshdef->fFree || (fBaseHull ? rgplr[idPlayer].cshdefSB : rgplr[idPlayer].cShDef) != cDesignHull) return 0;
            fWorkflowPassed = FPushClick(win32::GetDlgItem(hwndDialog, IDCANCEL));
            return fWorkflowPassed;
        }
        if (lpshdef->fFree || strcmp(lpshdef->hul.szClass, szHullName) ||
            memcmp(&lpshdef->hul.rghs[iSlotHull], &hsPartHull, sizeof(HS))) return 0;
        return FPushClick(win32::GetDlgItem(hwndDialog, IDC_COMBOBOX));
    }
    if (iWorkflowStep == 13) {
        hwndControl = win32::GetDlgItem(hwndDialog, IDC_COMBOBOX);
        iRow = (int)win32::SendMessageA(hwndControl, win32::CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)szHullName);
        if (iRow < 0) return 0;
        CompatListProc(hwndControl, win32::LB_SETTOPINDEX, iRow, 0);
        return FPushComboRow(hwndControl, iRow);
    }
    if (iWorkflowStep == 14) return lpshdefBuild == lpshdef && FPushClick(win32::GetDlgItem(hwndDialog, IDC_DELETE));
    if (!lpshdef->fFree || (fBaseHull ? rgplr[idPlayer].cshdefSB : rgplr[idPlayer].cShDef) != cDesignHull) return 0;
    fWorkflowPassed = FPushClick(win32::GetDlgItem(hwndDialog, IDCANCEL));
    return fWorkflowPassed;
}

static BOOL FCheckDesignerWorkflows(void) {
    SHDEF rgShipSav[16], rgBaseSav[10];
    SHDEF *lpshdef, *lpSav;
    int i, j, cShipSav = rgplr[idPlayer].cShDef, cBaseSav = rgplr[idPlayer].cshdefSB;

    memcpy(rgShipSav, rgshdef, sizeof(rgShipSav));
    memcpy(rgBaseSav, rglpshdefSB[idPlayer], sizeof(rgBaseSav));
    for (i = 20; i <= 23; i++) if (!FRunWorkflow(i, 0)) return 0;
    if (rgplr[idPlayer].cShDef != cShipSav || rgplr[idPlayer].cshdefSB != cBaseSav) return 0;
    for (j = 0; j < 2; j++) {
        for (i = 0; i < (j == 0 ? 16 : 10); i++) {
            lpshdef = j == 0 ? &rgshdef[i] : &rglpshdefSB[idPlayer][i];
            lpSav = j == 0 ? &rgShipSav[i] : &rgBaseSav[i];
            if (!lpSav->fFree && lpSav->lPower == -1 && lpshdef->lPower != -1) lpSav->lPower = LComputePower(lpSav);
            if (lpshdef->fFree != lpSav->fFree || (!lpSav->fFree && memcmp(lpshdef, lpSav, sizeof(SHDEF)))) return 0;
        }
    }
    puts("Hull selection, component drag/remove/reinsert, ship/starbase accept/cancel/delete and original design preservation passed");
    return 1;
}

static BOOL FCheckSaveReloadWorkflow(void) {
    int iResearch = (rgplr[idPlayer].iTechNow() + 1) % 6, iPlayerSav = idPlayer;
    int32_t lidSav = game.lid;
    char szLog[512];

    if (!FRunWorkflow(1, iResearch) || rgplr[idPlayer].iTechNow() != iResearch) return 0;
    win32::SendMessageA(hwndFrame, win32::WM_COMMAND, IDM_FILE_SAVE, 0);
    snprintf(szLog, sizeof(szLog), "%s.x%d", szBase, idPlayer + 1);
    if (!FFileExists(szLog) || game.fDirty) return 0;
    // Reload through the actual MRU command: FLoadGame resets the world before
    // replaying the saved orders, so the new research choice must persist.
    rgplr[idPlayer].SetITechNow((iResearch + 1) % 6);
    win32::SendMessageA(hwndFrame, win32::WM_COMMAND, IDM_FILE_MRU1, 0);
    if (game.lid != lidSav || idPlayer != iPlayerSav || !win32::IsWindow(hwndScanner) ||
        !win32::IsWindowEnabled(hwndFrame) || rgplr[idPlayer].iTechNow() != iResearch) return 0;
    puts("Save orders and MRU reload preserve the committed research choice and game identity");
    return 1;
}
