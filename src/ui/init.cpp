#include "win.h"
#include "text_io.h"
#include "preferencesui.h"
#include "profile_text_io.h"
#include "alert_io.h"
#include "ownership.h"
#include "uistate.h"
#include <stars_compat.h>

uint8_t rgPalGray[20] = {10, 20, 30, 40, 61, 71, 81, 92, 112, 122, 133, 143, 161, 171, 182, 193, 215, 225, 235, 245};

int16_t FCreateStuff() {
    int16_t fFailed;
    int16_t dy;
    int16_t i;
    HBITMAP hbmp;
    int16_t dx;

    BeginAppUiSession();
    fFailed = FALSE;
    dx = win32::GetSystemMetrics(SM_CXSCREEN);
    dy = win32::GetSystemMetrics(SM_CYSCREEN);
    if (dx < 800 || dy < 600) {
        gd.mdScreenSize = 0;
    } else if (dx < 1024 || dy < 768) {
        gd.mdScreenSize = 1;
    } else if (dx < 1111 || dy < 888) {
        gd.mdScreenSize = 2;
    } else {
        gd.mdScreenSize = 3;
    }
    hrgnHuge = win32::CreateRectRgn(-10, -10, 2000, 2000);
    hrgnScratch = win32::CreateRectRgn(0, 0, 10, 10);
    hbrShip = HbrGet(65280);
    hbrStarbase = HbrGet(0xffff);
    hbrBBlue = HbrGet(16711680);
    hbrEnemy = HbrGet(0xff);
    hbrSelect = HbrGet(0xffff);
    hbrRed = HbrGet(0xff);
    hbrBlue = HbrGet(8323072);
    hbrGreen = HbrGet(32512);
    hbrRadar = HbrGet(127);
    hbrPurple = HbrGet(8323199);
    hbrTooltip = HbrGet(10485759);
    hbrRadarNear = 0;
    rghbrMineral[0] = HbrGet(16711680);
    rghbrMineral[1] = HbrGet(32512);
    rghbrMineral[2] = HbrGet(0xffff);
    rghbrMineral[3] = HbrGet(0xffffff);
    rghbrMineral[4] = HbrGet(0xff);
    rghbrPlanetAttr[0][0] = HbrGet(8323072);
    rghbrPlanetAttr[0][1] = HbrGet(16711680);
    rghbrPlanetAttr[1][0] = HbrGet(127);
    rghbrPlanetAttr[1][1] = HbrGet(0xff);
    rghbrPlanetAttr[2][0] = HbrGet(32512);
    rghbrPlanetAttr[2][1] = HbrGet(65280);
    rghbrMinSum[0][0] = HbrGet(16711680);
    rghbrMinSum[0][1] = HbrGet(8323072);
    rghbrMinSum[1][0] = HbrGet(65280);
    rghbrMinSum[1][1] = HbrGet(32512);
    rghbrMinSum[2][0] = HbrGet(0xffff);
    rghbrMinSum[2][1] = HbrGet(32639);
    rghbrMinSum[3][0] = HbrGet(0xff);
    rghbrMinSum[3][1] = HbrGet(127);
    hbrYellow = HbrGet(0xffff);
    hbrDkYellow = HbrGet(32639);
    hbrLightGray = HbrGet(12632256);
    hbrGray = HbrGet(8421504);
    hpenShip = win32::CreatePen(0, 1, 65280);
    hpenDkGreen = win32::CreatePen(0, 1, 32512);
    hpenStarbase = win32::CreatePen(0, 1, 16711680);
    hpenEnemy = win32::CreatePen(0, 1, 0xff);
    hpenMassPath = win32::CreatePen(2, 1, 8355711);
    hpenRadar = win32::CreatePen(0, 1, 127);
    hpenRadarNear = 0;
    hpenDkBlue = win32::CreatePen(0, 1, 8323072);
    hpenYellow = win32::CreatePen(0, 1, 0xffff);
    hpenDkYellow = win32::CreatePen(0, 1, 32639);
    hpenDkPurple = win32::CreatePen(0, 1, 8323199);
    hbmp = win32::LoadBitmap(hInst, "Screen50Bmp");
    hbr50Screen = win32::CreatePatternBrush(hbmp);
    win32::DeleteObject(hbmp);
    for (i = 0; i < 3; i++) {
        hbmp = win32::LoadBitmap(hInst, MAKEINTRESOURCE(i + 460));
        rghbrPat[i] = win32::CreatePatternBrush(hbmp);
        win32::DeleteObject(hbmp);
    }
    hbmp = win32::LoadBitmap(hInst, "CargoBmp");
    hbrCargo = win32::CreatePatternBrush(hbmp);
    win32::DeleteObject(hbmp);
    hbmp = win32::LoadBitmap(hInst, "DockBmp");
    hbrDock = win32::CreatePatternBrush(hbmp);
    win32::DeleteObject(hbmp);
    hcurScanner = win32::LoadCursor(hInst, "ScannerCur");
    hcurScanAdd = win32::LoadCursor(hInst, "ScannerAdd");
    hcurOpenGrab = win32::LoadCursor(hInst, "OpenGrabCur");
    hcurCloseGrab = win32::LoadCursor(hInst, "CloseGrabCur");
    hcurTrashCan = win32::LoadCursor(hInst, MAKEINTRESOURCE(IDC_TRASH_CAN));
    hcurNoWay = win32::LoadCursor(hInst, MAKEINTRESOURCE(IDC_NO_WAY));
    hcurResizeWE = win32::LoadCursor(hInst, MAKEINTRESOURCE(IDC_RESIZE_WE));
    hcurResizeNS = win32::LoadCursor(hInst, MAKEINTRESOURCE(IDC_RESIZE_NS));
    hcurResize4Way = win32::LoadCursor(hInst, MAKEINTRESOURCE(IDC_RESIZE_4WAY));
    hcurArrowHelp = win32::LoadCursor(hInst, MAKEINTRESOURCE(IDC_ARROW_HELP));
    hcurHand = win32::LoadCursor(hInst, MAKEINTRESOURCE(IDC_HAND));
    hbmpScanner = win32::LoadBitmap(hInst, "ScannerBmp");
    hbmpScanShip = win32::LoadBitmap(hInst, MAKEINTRESOURCE(IDDIB_SCANNER_TOOLBAR));
    hbmpUnknownPlanet = win32::LoadBitmap(hInst, "UnknownPlanetBmp");
    hbmpNumbers = win32::LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FONT_DIGITS));
    hdibPlanets = HdibLoadBigResource(IDDIB_PLANET_ICONS);
    hdibThings = HdibLoadBigResource(IDDIB_THING_ICONS);
    hdibToolbar = HdibLoadBigResource(IDB_TOOLBAR);
    if (!hdibPlanets || !hdibThings || !hdibToolbar) {
        fFailed = TRUE;
    }
    for (i = 0; i < 5; i++) {
        rghdibShips[i] = HdibLoadBigResource(static_cast<BitmapId>(i + 552));
        if (!rghdibShips[i]) {
            fFailed = TRUE;
        }
        rghdibShipsT[i] = HdibLoadBigResource(static_cast<BitmapId>(i + 557));
        if (!rghdibShipsT[i]) {
            fFailed = TRUE;
        }
    }
    for (i = 0; i < 7; i++) {
        rghdibInventory[i] = HdibLoadBigResource(static_cast<BitmapId>(i + 500));
        if (!rghdibInventory[i]) {
            fFailed = TRUE;
        }
    }
    vhpal = HpalFromDib(rghdibShips[3]);
    hdibRaces = HdibLoadBigResource(IDDIB_PLAYER_ICONS);
    hdibRacesT = HdibLoadBigResource(IDDIB_PLAYER_ICONS_SMALL);
    hdibRacesX = HdibLoadBigResource(IDDIB_PLAYER_ICONS_TINY);
    hbmpBackBld = win32::LoadBitmap(hInst, MAKEINTRESOURCE(IDB_EMPTY_HULL_SLOT));
    hbmpMsg = win32::LoadBitmap(hInst, MAKEINTRESOURCE(IDB_MSGFILTER_CHECKBOX));
    hbmpMono = win32::LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FILTER_CHECKBOX_MONO));
    hdibPlaque = HdibLoadBigResource(IDDIB_NUM_DESIGNS_PLATE);
    hiconStars = win32::LoadIcon(hInst, "StarsIco");
    hiconHost = win32::LoadIcon(hInst, "HostIco");
    hiconWait = win32::LoadIcon(hInst, "WaitIco");
    rghiconVCR[0] = win32::LoadIcon(hInst, "Bang1Ico");
    rghiconVCR[1] = win32::LoadIcon(hInst, "Bang2Ico");
    rghiconVCR[2] = win32::LoadIcon(hInst, "Bang3Ico");
    rghiconVCR[3] = win32::LoadIcon(hInst, "Torp1Ico");
    rghiconVCR[4] = win32::LoadIcon(hInst, "Torp2Ico");
    rghiconVCR[5] = win32::LoadIcon(hInst, "Torp3Ico");
    rghiconVCR[6] = win32::LoadIcon(hInst, "Torp4Ico");
    lpfnFakeComboProc = MakeProcInstance(FakeComboProc, hInst);
    lpfnFakeCEProc = MakeProcInstance(FakeCEProc, hInst);
    lpfnFakeEditProc = MakeProcInstance(FakeEditProc, hInst);
    lpfnFakeListProc = MakeProcInstance(FakeListProc, hInst);
    lpfnHostTimerProc = MakeProcInstance(HostTimerProc, hInst);
    lpfnBrowserDlgProc = MakeProcInstance(BrowserDlg, hInst);
    lpfnReportDlgProc = MakeProcInstance(ReportDlg, hInst);
    lpfnGaugeDlgProc = MakeProcInstance(ProgressGaugeDlg, hInst);
    lpb2k = LpAlloc<uint8_t>(0x800, htPerm);
    vlprgidMisc = LpAlloc<uint16_t>(0x800, htPerm);
    InitGameStuff();
    if (fFailed || !hbmpScanner || !hbmpUnknownPlanet || !hbmpBackBld || !hdibRaces || !hdibRacesT || !hdibRacesX || !hbmpMono || !hbmpScanShip || !hbmpMsg ||
        !hiconHost || !hiconStars || !hiconWait) {
        AlertIO().pfnAlertSz(TextIO().pfnFormat(idsUnableLoadBitmaps, NULL), MB_ICONHAND);
        return FALSE;
    }
    return TRUE;
}

int16_t FCreateFonts(HDC hdc) {
    NATIVEOWNER<LOGFONT, win32::LocalFree> plfOwner;
    int16_t    i;
    LOGFONT   *plf;
    HFONT      hfontSav;
    TEXTMETRIC tm;
    int32_t    l;

    plf = (LOGFONT *)win32::LocalAlloc(64, sizeof(LOGFONT));
    plfOwner.reset(plf);
    for (i = 0; i < 4; i++) {
        if (rgszArial[i][0] == 0) {
            TextIO().pfnCchGetString(static_cast<StringId>(idsArial2 + i), rgszArial[i]);
        }
    }
    plf->lfHeight = -win32::MulDiv(10, win32::GetDeviceCaps(hdc, LOGPIXELSY), 72);
    for (i = 0; i < 2; i++) {
        strcpy(plf->lfFaceName, rgszArial[i]);
        rghfontArial10[i] = win32::CreateFontIndirect(plf);
    }
    strcpy(plf->lfFaceName, rgszArial[0]);
    plf->lfHeight = -win32::MulDiv(6, win32::GetDeviceCaps(hdc, LOGPIXELSY), 72);
    rghfontArial6[0] = win32::CreateFontIndirect(plf);
    plf->lfHeight = -win32::MulDiv(7, win32::GetDeviceCaps(hdc, LOGPIXELSY), 72);
    rghfontArial7[0] = win32::CreateFontIndirect(plf);
    plf->lfHeight = -win32::MulDiv(8, win32::GetDeviceCaps(hdc, LOGPIXELSY), 72);
    for (i = 0; i < 4; i++) {
        strcpy(plf->lfFaceName, rgszArial[i]);
        rghfontArial8[i] = win32::CreateFontIndirect(plf);
    }
    strcpy(plf->lfFaceName, rgszArial[1]);
    plf->lfEscapement = 3150;
    rghfontArial8[4] = win32::CreateFontIndirect(plf);
    hfontSav = (HFONT)win32::SelectObject(hdc, rghfontArial8[0]);
    win32::GetTextMetrics(hdc, &tm);
    dyArial8 = tm.tmHeight + tm.tmExternalLeading;
    l = GetTextExtent(hdc, "88888888kT", 10);
    dxMaxMineralQuan = LOWORD(l);
    win32::SelectObject(hdc, rghfontArial7[0]);
    win32::GetTextMetrics(hdc, &tm);
    dyArial7 = tm.tmHeight + tm.tmExternalLeading;
    win32::SelectObject(hdc, rghfontArial6[0]);
    win32::GetTextMetrics(hdc, &tm);
    dyArial6 = tm.tmHeight + tm.tmExternalLeading;
    win32::SelectObject(hdc, rghfontArial10[0]);
    win32::GetTextMetrics(hdc, &tm);
    dyArial10 = tm.tmHeight + tm.tmExternalLeading;
    win32::SelectObject(hdc, hfontSav);
    plfOwner.reset();
    return TRUE;
}

#ifdef __APPLE__
static int16_t FSetMacMenuShortcut(HMENU hmenu, UINT id, const ACCEL *lpaccel) {
    char szItem[128], szLabel[160];
    char *pszTab;
    int i, cch;
    UINT mf;

    for (i = 0; i < win32::GetMenuItemCount(hmenu); i++) {
        if (win32::GetMenuItemID(hmenu, i) != id) continue;
        if (win32::GetMenuString(hmenu, i, NULL, 0, MF_BYPOSITION) >= (int)sizeof(szItem)) return FALSE;
        if (win32::GetMenuString(hmenu, i, szItem, sizeof(szItem), MF_BYPOSITION) == 0) return FALSE;
        pszTab = strchr(szItem, '\t');
        if (pszTab) *pszTab = 0;
        cch = CchSnprintf(szLabel, sizeof(szLabel), "%s\tCmd+%s%c", szItem,
                         lpaccel->fVirt & FSHIFT ? "Shift+" : "", lpaccel->key);
        if (cch <= 0 || (size_t)cch >= sizeof(szLabel)) return FALSE;
        mf = win32::GetMenuState(hmenu, i, MF_BYPOSITION) & (MF_CHECKED | MF_DISABLED | MF_GRAYED);
        if (!win32::InsertMenu(hmenu, i, MF_BYPOSITION | mf, id, szLabel)) return FALSE;
        return win32::RemoveMenu(hmenu, i + 1, MF_BYPOSITION);
    }
    return FALSE;
}

static int16_t FInitMacShortcuts(HWND hwnd) {
    static const struct {
        UINT idMenu;
        ACCEL accel;
    } rgkey[] = {
        {IDM_TOOL_NEW_GAME, {FVIRTKEY | FCONTROL, 'N', IDM_FILE_NEW_GAME}},
        {IDM_TOOL_OPEN_GAME, {FVIRTKEY | FCONTROL, 'O', IDM_FILE_OPEN_GAME}},
        {IDM_TURN_END_A, {FVIRTKEY | FCONTROL, 'S', IDM_FILE_SAVE}},
        {IDM_TURN_END_B, {FVIRTKEY | FCONTROL | FSHIFT, 'S', IDM_TURN_SAVE_SUBMIT}},
        {IDM_VIEW_FIND, {FVIRTKEY | FCONTROL, 'F', IDM_VIEW_FIND2}},
        {IDM_FILE_RETURN_TO_TITLE, {FVIRTKEY | FCONTROL, 'W', IDM_FILE_RETURN_TO_TITLE}},
        {IDM_FILE_EXIT, {FVIRTKEY | FCONTROL, 'Q', IDM_FILE_EXIT}}
    };
    ACCEL rgaccel[32];
    ACCEL rgtitle[] = {
        {FVIRTKEY | FCONTROL, 'N', IDM_TITLE_NEW_GAME},
        {FVIRTKEY | FCONTROL, 'O', IDM_TITLE_OPEN_GAME},
        {FVIRTKEY | FCONTROL, 'R', IDM_TITLE_CONTINUE},
        {FVIRTKEY | FCONTROL, 'Q', IDM_TITLE_EXIT}
    };
    HACCEL hmain, htitle;
    HMENU hmenu, hsub;
    int c, i, j;
    size_t ikey;

    c = win32::CopyAcceleratorTable(hAccel, NULL, 0);
    if (c <= 0 || (size_t)c + sizeof(rgkey) / sizeof(rgkey[0]) > sizeof(rgaccel) / sizeof(rgaccel[0]) ||
        win32::CopyAcceleratorTable(hAccel, rgaccel, c) != c) return FALSE;
    hmenu = win32::GetMenu(hwnd);
    if (!hmenu || win32::GetMenuItemCount(hmenu) <= 0) return FALSE;
    for (ikey = 0; ikey < sizeof(rgkey) / sizeof(rgkey[0]); ikey++) {
        for (i = 0; i < c && rgaccel[i].cmd != rgkey[ikey].accel.cmd; i++) {}
        rgaccel[i] = rgkey[ikey].accel;
        if (i == c) c++;
        for (j = 0; j < win32::GetMenuItemCount(hmenu); j++) {
            hsub = win32::GetSubMenu(hmenu, j);
            if (hsub && win32::GetMenuState(hsub, rgkey[ikey].idMenu, MF_BYCOMMAND) != UINT_MAX) break;
        }
        if (j == win32::GetMenuItemCount(hmenu) || !FSetMacMenuShortcut(hsub, rgkey[ikey].idMenu, &rgkey[ikey].accel)) return FALSE;
    }
    hmain = win32::CreateAcceleratorTable(rgaccel, c);
    if (!hmain) return FALSE;
    htitle = win32::CreateAcceleratorTable(rgtitle, sizeof(rgtitle) / sizeof(rgtitle[0]));
    if (!htitle) { win32::DestroyAcceleratorTable(hmain); return FALSE; }
    hAccel = hmain;
    hAccelTitle = htitle;
    return TRUE;
}
#endif

int16_t InitInstance(int16_t nCmdShow) {
    int16_t sw;
    RECT    rc;


    StarsCompatSetObjectInspector(ShowScannerInspector);
    ini.fWait = FALSE;
    ini.fStartupFile = FALSE;
    ini.grobjSel = 0;
    ini.idPlayer = iplrNone;
    ReadIniSettings();
    rc = ini.wnFrame.rc;
    /* Stars.ini holds -32768, the Win16 CW_USEDEFAULT, when it has no frame position. */
    if (rc.left == -32768)
        rc.left = CW_USEDEFAULT;
    if (rc.right == -32768)
        rc.right = CW_USEDEFAULT;
    hwndFrame = ui::CreateWindowEx(0, szFrame, "Stars!", WS_OVERLAPPEDWINDOW, rc.left, rc.top, rc.right, rc.bottom, NULL, NULL, hInst, NULL);
    if (!hwndFrame) {
        return FALSE;
    }
    hAccel = win32::LoadAccelerators(hInst, MAKEINTRESOURCE(IDA_MAIN));
    if (!hAccel) {
        return FALSE;
    }
    hAccelTitle = win32::LoadAccelerators(hInst, MAKEINTRESOURCE(IDA_TITLE));
    if (!hAccelTitle) {
        return FALSE;
    }
#ifdef __APPLE__
    if (!FInitMacShortcuts(hwndFrame)) return FALSE;
#endif
    if (nCmdShow != 1) {
        sw = nCmdShow;
    } else {
        sw = ini.wnFrame.fMaximized != 0 || ini.wnFrame.fMinimized ? 3 : 1;
    }
    // Showing then hiding the frame exposes the native maximize animation before the title screen.
    StarsCompatSetInitialShowCommand(hwndFrame, sw);
    return TRUE;
}

void InitTiles() {
    int16_t  yTop;
    int16_t  ctile;
    TILE    *rgtile;
    int16_t  i;
    int16_t  iPass;
    uint16_t iCol;

    iPass = 2;
    rgtile = rgtilePlanet;
    ctile = 6;
    while (iPass-- != 0) {
        iCol = 0;
        yTop = 4;
        for (i = 0; i < ctile; i++) {
            if (rgtile[i].iCol != iCol) {
                yTop = 4;
                iCol = rgtile[i].iCol;
            }
            rgtile[i].dyFull += rgtile[i].yTop * dyArial8;
            rgtile[i].yTop = yTop;
            rgtile[i].fFixCtls = FALSE;
            rgtile[i].fMinDraw = FALSE;
            if (rgtile[i].fPopped) {
                yTop += rgtile[i].dyFull + 4;
            } else {
                yTop += dyArial8 + 7;
            }
        }
        rgtile = rgtileShip;
        ctile = 7;
    }
    return;
}

void GetIniWinRc(char *szSection, char *szIniFile, StringId ids, WN *pwn) {
    int16_t fInitalized;
    int16_t fMinimized;
    int16_t fMaximized;
    char    szEntry[16];
    int16_t cch;
    RECT    rc;
    int16_t j;
    char   *pch;
    int16_t i;
    int16_t fNeg;
    int16_t rg[4];

    TextIO().pfnCchGetString(ids, szEntry);
    cch = ProfileTextIO().pfnReadString(szSection, szEntry, "X", szWork, 20, szIniFile);
    if (cch != 17 || (szWork[0] != 'M' && szWork[0] != 'R' && szWork[0] != 'I')) {
    NoRc:
        rc.left = -32768;
        rc.right = -32768;
        rc.bottom = 0;
        rc.top = 0;
        fMaximized = ids == idsMain;
        fMinimized = FALSE;
        fInitalized = FALSE;
    } else {
        pch = &szWork[1];
        for (i = 0; i < 4; i++) {
            rg[i] = 0;
            fNeg = FALSE;
            for (j = 0; j < 4; j++, pch++) {
                if (*pch == '-') {
                    fNeg = TRUE;
                    continue;
                }
                if (*pch < '0' || *pch > '9')
                    goto NoRc;
                rg[i] = 10 * rg[i] + (*pch - '0');
            }
            if (fNeg) {
                rg[i] = -rg[i];
            }
        }
        rc.left = rg[0];
        rc.top = rg[1];
        rc.right = rg[2];
        rc.bottom = rg[3];
        fMaximized = szWork[0] == 'M';
        fMinimized = szWork[0] == 'I';
        fInitalized = TRUE;
    }
    pwn->rc = rc;
    pwn->fMaximized = fMaximized;
    pwn->fMinimized = fMinimized;
    pwn->fInitalized = fInitalized;
    return;
}

void ReadIniSettings() {
    uint16_t uDateCur;
    int16_t  i;
    char     szEntry[16];
    WN       wnT;
    char     szIniFile[16];
    uint16_t w;
    char     szSection[16];
    int16_t  cch;

    ini.fGen = FALSE;
    ini.fTry = FALSE;
    ini.fWait = FALSE;
    TextIO().pfnCchGetString(idsWindows, szSection);
    TextIO().pfnCchGetString(idsStarsIni, szIniFile);
    GetIniWinRc(szSection, szIniFile, idsMain, &ini.wnFrame);
    GetIniWinRc(szSection, szIniFile, idsReportfleetwin, &wnT);
    if (wnT.rc.left != -32768) {
        vrptFleet.ptDlg.x = wnT.rc.left;
        vrptFleet.ptDlg.y = wnT.rc.top;
        vrptFleet.ptSize.x = wnT.rc.right - wnT.rc.left;
        vrptFleet.ptSize.y = wnT.rc.bottom - wnT.rc.top;
    }
    GetIniWinRc(szSection, szIniFile, idsReportefleetwin, &wnT);
    if (wnT.rc.left != -32768) {
        vrptEFleet.ptDlg.x = wnT.rc.left;
        vrptEFleet.ptDlg.y = wnT.rc.top;
        vrptEFleet.ptSize.x = wnT.rc.right - wnT.rc.left;
        vrptEFleet.ptSize.y = wnT.rc.bottom - wnT.rc.top;
    }
    GetIniWinRc(szSection, szIniFile, idsReportbtlwin, &wnT);
    if (wnT.rc.left != -32768) {
        vrptBattle.ptDlg.x = wnT.rc.left;
        vrptBattle.ptDlg.y = wnT.rc.top;
        vrptBattle.ptSize.x = wnT.rc.right - wnT.rc.left;
        vrptBattle.ptSize.y = wnT.rc.bottom - wnT.rc.top;
    }
    GetIniWinRc(szSection, szIniFile, idsReportplanwin, &wnT);
    if (wnT.rc.left != -32768) {
        vrptPlanet.ptDlg.x = wnT.rc.left;
        vrptPlanet.ptDlg.y = wnT.rc.top;
        vrptPlanet.ptSize.x = wnT.rc.right - wnT.rc.left;
        vrptPlanet.ptSize.y = wnT.rc.bottom - wnT.rc.top;
    }
    TextIO().pfnCchGetString(idsResolution, szEntry);
    i = win32::GetPrivateProfileInt(szSection, szEntry, 0, szIniFile);
    if (i == 0 && (vcScreenColors <= 4 || gd.mdScreenSize == 0)) {
        AlertIO().pfnAlertSz(TextIO().pfnFormat(idsNoteStarsPrefersScreenResolutionLeast800x600, NULL), MB_ICONHAND);
    }
    TextIO().pfnCchGetString(idsLayout, szEntry);
    iWindowLayout = static_cast<WindowLayout>(win32::GetPrivateProfileInt(szSection, szEntry, 1, szIniFile));
    iWindowLayout = 2 < (layoutLarge <= (int16_t)iWindowLayout ? iWindowLayout : 0) ? layoutSmall
                    : layoutLarge > (int16_t)iWindowLayout                          ? layoutLarge
                                                                                    : iWindowLayout;
    TextIO().pfnCchGetString(idsStyle1width, szEntry);
    vfs.dxPlanWant = win32::GetPrivateProfileInt(szSection, szEntry, 396, szIniFile);
    vfs.dxPlanWant = (vfs.dxPlanWant <= 10 ? 10 : vfs.dxPlanWant) >= 2000 ? 2000 : vfs.dxPlanWant > 10 ? vfs.dxPlanWant : 10;
    TextIO().pfnCchGetString(idsStyle1height, szEntry);
    vfs.dyMsgWant = win32::GetPrivateProfileInt(szSection, szEntry, 110, szIniFile);
    vfs.dyMsgWant = (vfs.dyMsgWant <= 10 ? 10 : vfs.dyMsgWant) >= 2000 ? 2000 : vfs.dyMsgWant > 10 ? vfs.dyMsgWant : 10;
    TextIO().pfnCchGetString(idsStyle1height2, szEntry);
    vfs.dyMinWant = win32::GetPrivateProfileInt(szSection, szEntry, 192, szIniFile);
    vfs.dyMinWant = (vfs.dyMinWant <= 10 ? 10 : vfs.dyMinWant) >= 2000 ? 2000 : vfs.dyMinWant > 10 ? vfs.dyMinWant : 10;
    TextIO().pfnCchGetString(idsStyle2width, szEntry);
    vfs.dx2PlanWant = win32::GetPrivateProfileInt(szSection, szEntry, 396, szIniFile);
    vfs.dx2PlanWant = (vfs.dx2PlanWant <= 10 ? 10 : vfs.dx2PlanWant) >= 2000 ? 2000 : vfs.dx2PlanWant > 10 ? vfs.dx2PlanWant : 10;
    TextIO().pfnCchGetString(idsStyle2height, szEntry);
    vfs.dy2MsgWant = win32::GetPrivateProfileInt(szSection, szEntry, 110, szIniFile);
    vfs.dy2MsgWant = (vfs.dy2MsgWant <= 10 ? 10 : vfs.dy2MsgWant) >= 2000 ? 2000 : vfs.dy2MsgWant > 10 ? vfs.dy2MsgWant : 10;
    TextIO().pfnCchGetString(idsStyle2height2, szEntry);
    vfs.dy2MinWant = win32::GetPrivateProfileInt(szSection, szEntry, 192, szIniFile);
    vfs.dy2MinWant = (vfs.dy2MinWant <= 10 ? 10 : vfs.dy2MinWant) >= 2000 ? 2000 : vfs.dy2MinWant > 10 ? vfs.dy2MinWant : 10;
    TextIO().pfnCchGetString(idsToolbar, szEntry);
    i = win32::GetPrivateProfileInt(szSection, szEntry, 1, szIniFile);
    gd.fToolbar = i != 0;
    TextIO().pfnCchGetString(idsPlanettiles, szEntry);
    cch = ProfileTextIO().pfnReadString(szSection, szEntry, "X", szWork, 20, szIniFile);
    ReadIniTileSettings(szWork, rgtilePlanet, 6);
    TextIO().pfnCchGetString(idsShiptiles, szEntry);
    cch = ProfileTextIO().pfnReadString(szSection, szEntry, "X", szWork, 20, szIniFile);
    ReadIniTileSettings(szWork, rgtileShip, 7);
    TextIO().pfnCchGetString(idsSelection, szEntry);
    cch = ProfileTextIO().pfnReadString(szSection, szEntry, "N", szWork, 20, szIniFile);
    ReadIniSelection(szWork, cch);
    TextIO().pfnCchGetString(idsMessage, szEntry);
    i = win32::GetPrivateProfileInt(szSection, szEntry, 0, szIniFile);
    ini.iMsg = i;
    TextIO().pfnCchGetString(idsGameid, szEntry);
    cch = ProfileTextIO().pfnReadString(szSection, szEntry, "0", szWork, 10, szIniFile);
    ReadIniIdentity(szWork, cch);
    TextIO().pfnCchGetString(idsScanzoom, szEntry);
    w = win32::GetPrivateProfileInt(szSection, szEntry, 4, szIniFile);
    if (w >= 1 && w <= 9) {
        iScanZoom = static_cast<ScanZoom>(w - 5);
    }
    TextIO().pfnCchGetString(idsScanfilterv25, szEntry);
    w = win32::GetPrivateProfileInt(szSection, szEntry, 0, szIniFile);
    grbitScanShip = w;
    TextIO().pfnCchGetString(idsScanefilterv25, szEntry);
    w = win32::GetPrivateProfileInt(szSection, szEntry, 0, szIniFile);
    grbitScanEShip = w;
    TextIO().pfnCchGetString(idsScanmines, szEntry);
    w = win32::GetPrivateProfileInt(szSection, szEntry, 15, szIniFile);
    grbitScanMines = w & 0xf;
    TextIO().pfnCchGetString(idsScanradar, szEntry);
    w = win32::GetPrivateProfileInt(szSection, szEntry, 100, szIniFile);
    if (w < 0 || w > 100) {
        w = 100;
    }
    vpctRadarView = w;
    TextIO().pfnCchGetString(idsScanmodev25, szEntry);
    w = win32::GetPrivateProfileInt(szSection, szEntry, 224, szIniFile);
    grbitScan = static_cast<GrbitScan>(w & 0xc00f);
    if (grbitScan >= 6) {
        grbitScan = static_cast<GrbitScan>(0);
        grbitScanShip = 0;
    } else {
        grbitScan = static_cast<GrbitScan>(grbitScan | w & 0x3ff0);
    }
    TextIO().pfnCchGetString(idsMineralscale, szEntry);
    cMinGrafMax = win32::GetPrivateProfileInt(szSection, szEntry, cMinGrafMax, szIniFile);
    if (cMinGrafMax < 100 || cMinGrafMax > 30000) {
        cMinGrafMax = 5000;
    }
    TextIO().pfnCchGetString(idsFiles, szSection);
    TextIO().pfnCchGetString(idsLogging, szEntry);
    ini.fLogging = win32::GetPrivateProfileInt(szSection, szEntry, 0, szIniFile);
    TextIO().pfnCchGetString(idsWait2, szEntry);
    ini.fWait = win32::GetPrivateProfileInt(szSection, szEntry, 0, szIniFile);
    TextIO().pfnCchGetString(idsFile1, szEntry);
    cch = ProfileTextIO().pfnReadString(szSection, szEntry, ".", szWork, 256, szIniFile);
    ReadIniRecent(szSection, szEntry, szIniFile, cch);
    TextIO().pfnCchGetString(idsTurn, szEntry);
    ini.turn = win32::GetPrivateProfileInt(szSection, szEntry, game.turn, szIniFile);
    TextIO().pfnCchGetString(idsMisc, szSection);
    TextIO().pfnCchGetString(idsDefaultpassword, szEntry);
    ProfileTextIO().pfnReadString(szSection, szEntry, "", vszDefPass, 16, szIniFile);
    TextIO().pfnCchGetString(idsProgress, szEntry);
    w = win32::GetPrivateProfileInt(szSection, szEntry, 0, szIniFile);
    gd.fProgressTxt = w != 0;
    TextIO().pfnCchGetString(idsNewreports, szEntry);
    w = win32::GetPrivateProfileInt(szSection, szEntry, 0, szIniFile);
    gd.fPerPlayerDumps = w != 0;
    TextIO().pfnCchGetString(idsNohostnames, szEntry);
    w = win32::GetPrivateProfileInt(szSection, szEntry, 0, szIniFile);
    gd.fNoHostNames = w != 0;
    TextIO().pfnCchGetString(idsBackups, szEntry);
    vcBackupDirs = win32::GetPrivateProfileInt(szSection, szEntry, 1, szIniFile);
    if (vcBackupDirs < 1 || vcBackupDirs > 999) {
        vcBackupDirs = 1;
    }
    TextIO().pfnCchGetString(idsReportplanfld, szEntry);
    vrptPlanet.grbitVisible = (uint32_t)win32::GetPrivateProfileInt(szSection, szEntry, -1, szIniFile);
    TextIO().pfnCchGetString(idsReportplansort, szEntry);
    i = win32::GetPrivateProfileInt(szSection, szEntry, 0, szIniFile);
    vrptPlanet.fAscending = (i & 0x100) != 0;
    i &= 0xff;
    vrptPlanet.icolSort = i;
    TextIO().pfnCchGetString(idsReportfleetfld, szEntry);
    vrptFleet.grbitVisible = (uint32_t)win32::GetPrivateProfileInt(szSection, szEntry, -1, szIniFile);
    TextIO().pfnCchGetString(idsReportfleetsort, szEntry);
    i = win32::GetPrivateProfileInt(szSection, szEntry, 0, szIniFile);
    vrptFleet.fAscending = (i & 0x100) != 0;
    i &= 0xff;
    vrptFleet.icolSort = i;
    TextIO().pfnCchGetString(idsReportefleetfld, szEntry);
    vrptEFleet.grbitVisible = (uint32_t)win32::GetPrivateProfileInt(szSection, szEntry, -1, szIniFile);
    TextIO().pfnCchGetString(idsReportefltsort, szEntry);
    i = win32::GetPrivateProfileInt(szSection, szEntry, 0, szIniFile);
    vrptEFleet.fAscending = (i & 0x100) != 0;
    i &= 0xff;
    vrptEFleet.icolSort = i;
    TextIO().pfnCchGetString(idsReportbtlfld, szEntry);
    vrptBattle.grbitVisible = (uint32_t)win32::GetPrivateProfileInt(szSection, szEntry, -1, szIniFile);
    TextIO().pfnCchGetString(idsReportbtlsort, szEntry);
    i = win32::GetPrivateProfileInt(szSection, szEntry, 0, szIniFile);
    vrptBattle.fAscending = (i & 0x100) != 0;
    i &= 0xff;
    vrptBattle.icolSort = i;
    TextIO().pfnCchGetString(idsReportdefgraph, szEntry);
    gd.iCurGraph = win32::GetPrivateProfileInt(szSection, szEntry, 7, szIniFile);
    TextIO().pfnCchGetString(idsReportefltsort, szEntry);
    if (gd.iCurGraph > 7) {
        gd.iCurGraph = 7;
    }
    TextIO().pfnCchGetString(idsVcrspeed, szEntry);
    viSpeedVCR = win32::GetPrivateProfileInt(szSection, szEntry, 1, szIniFile);
    _strdate(szWork);
    szWork[5] = 0;
    szWork[2] = 0;
    uDateCur = atoi(&szWork[3]) + atoi(szWork) * 31 + atoi(&szWork[6]) * 31 * 12;
    TextIO().pfnCchGetString(idsHistoryinfo, szEntry);
    uDateInstalled = win32::GetPrivateProfileInt(szSection, szEntry, -1, szIniFile);
    if (uDateCur < uDateInstalled) {
        uDateInstalled = uDateCur;
    }
    gd.fTrialPeriodOver = uDateCur >= (uint16_t)(uDateInstalled + 21);
    TextIO().pfnCchGetString(idsFonts, szSection);
    for (i = 0; i < 4; i++) {
        TextIO().pfnCchGetString(static_cast<StringId>(idsArial + i), szEntry);
        ProfileTextIO().pfnReadString(szSection, szEntry, "", szWork, 80, szIniFile);
        cch = strlen(szWork);
        if (cch >= 5 && cch <= 31) {
            strcpy(rgszArial[i], szWork);
        }
    }
    ReadIniPresets(szIniFile);
    return;
}

void ReadIniTileSettings(char *pszFormat, TILE *rgtile, int16_t ctile) {
    TILE     tile;
    int16_t  fPopped;
    int16_t  i;
    int16_t  iTile;
    uint16_t iCol;
    uint16_t iBit;

    iCol = 0;
    iTile = 0;
    while (*pszFormat != 0) {
        if (*pszFormat == '*') {
            if (iCol < 1) {
                iCol++;
            }
            goto DoNext;
        }
        if (isalpha((unsigned char)*pszFormat) == 0)
            goto DoNext;
        // C classification functions return any nonzero mask, but the saved
        // expansion state is stored in a one-bit field.
        fPopped = isupper((unsigned char)*pszFormat) != 0;
        iBit = !fPopped ? *pszFormat - 'a' : *pszFormat - 'A';
        if ((int16_t)iBit < 0)
            goto DoNext;
        for (i = iTile; i < ctile && rgtile[i].id != iBit; i++) {
        }
        if (i == ctile)
            goto DoNext;
        rgtile[i].iCol = iCol;
        rgtile[i].fPopped = fPopped;
        if (i != iTile) {
            tile = rgtile[i];
            rgtile[i] = rgtile[iTile];
            rgtile[iTile] = tile;
        }
        iTile++;
    DoNext:
        pszFormat++;
    }
    for (i = iTile; i < ctile; i++) {
        if (rgtile[i].iCol < iCol) {
            rgtile[i].iCol = iCol;
        }
    }
    return;
}
