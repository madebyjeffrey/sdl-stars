#include "win.h"
#include "appstate.h"
#include "text_io.h"
#include "preferencesui.h"
#include "profile_text_io.h"
#include "alert_io.h"
#include "ownership.h"
#include "uistate.h"
#include <stars_compat.h>

std::uint8_t rgPalGray[20] = {10, 20, 30, 40, 61, 71, 81, 92, 112, 122, 133, 143, 161, 171, 182, 193, 215, 225, 235, 245};

std::int16_t FCreateStuff(APPSESSION &app) {
    std::int16_t fFailed;
    std::int16_t dy;
    std::int16_t i;
    HBITMAP hbmp;
    std::int16_t dx;

    BeginAppUiSession(app);
    fFailed = 0;
    dx = win32::GetSystemMetrics(win32::SM_CXSCREEN);
    dy = win32::GetSystemMetrics(win32::SM_CYSCREEN);
    if (dx < 800 || dy < 600) {
        app.state.gd.mdScreenSize = 0;
    } else if (dx < 1024 || dy < 768) {
        app.state.gd.mdScreenSize = 1;
    } else if (dx < 1111 || dy < 888) {
        app.state.gd.mdScreenSize = 2;
    } else {
        app.state.gd.mdScreenSize = 3;
    }
    app.lpui->hrgnHuge = win32::CreateRectRgn(-10, -10, 2000, 2000);
    app.lpui->hrgnScratch = win32::CreateRectRgn(0, 0, 10, 10);
    app.lpui->hbrShip = HbrGet(app, 65280);
    app.lpui->hbrStarbase = HbrGet(app, 0xffff);
    app.lpui->hbrBBlue = HbrGet(app, 16711680);
    app.lpui->hbrEnemy = HbrGet(app, 0xff);
    app.lpui->hbrSelect = HbrGet(app, 0xffff);
    app.lpui->hbrRed = HbrGet(app, 0xff);
    app.lpui->hbrBlue = HbrGet(app, 8323072);
    app.lpui->hbrGreen = HbrGet(app, 32512);
    app.lpui->hbrRadar = HbrGet(app, 127);
    app.lpui->hbrPurple = HbrGet(app, 8323199);
    app.lpui->hbrTooltip = HbrGet(app, 10485759);
    app.lpui->hbrRadarNear = nullptr;
    app.lpui->rghbrMineral[0] = HbrGet(app, 16711680);
    app.lpui->rghbrMineral[1] = HbrGet(app, 32512);
    app.lpui->rghbrMineral[2] = HbrGet(app, 0xffff);
    app.lpui->rghbrMineral[3] = HbrGet(app, 0xffffff);
    app.lpui->rghbrMineral[4] = HbrGet(app, 0xff);
    app.lpui->rghbrPlanetAttr[0][0] = HbrGet(app, 8323072);
    app.lpui->rghbrPlanetAttr[0][1] = HbrGet(app, 16711680);
    app.lpui->rghbrPlanetAttr[1][0] = HbrGet(app, 127);
    app.lpui->rghbrPlanetAttr[1][1] = HbrGet(app, 0xff);
    app.lpui->rghbrPlanetAttr[2][0] = HbrGet(app, 32512);
    app.lpui->rghbrPlanetAttr[2][1] = HbrGet(app, 65280);
    app.lpui->rghbrMinSum[0][0] = HbrGet(app, 16711680);
    app.lpui->rghbrMinSum[0][1] = HbrGet(app, 8323072);
    app.lpui->rghbrMinSum[1][0] = HbrGet(app, 65280);
    app.lpui->rghbrMinSum[1][1] = HbrGet(app, 32512);
    app.lpui->rghbrMinSum[2][0] = HbrGet(app, 0xffff);
    app.lpui->rghbrMinSum[2][1] = HbrGet(app, 32639);
    app.lpui->rghbrMinSum[3][0] = HbrGet(app, 0xff);
    app.lpui->rghbrMinSum[3][1] = HbrGet(app, 127);
    app.lpui->hbrYellow = HbrGet(app, 0xffff);
    app.lpui->hbrDkYellow = HbrGet(app, 32639);
    app.lpui->hbrLightGray = HbrGet(app, 12632256);
    app.lpui->hbrGray = HbrGet(app, 8421504);
    app.lpui->hpenShip = win32::CreatePen(0, 1, 65280);
    app.lpui->hpenDkGreen = win32::CreatePen(0, 1, 32512);
    app.lpui->hpenStarbase = win32::CreatePen(0, 1, 16711680);
    app.lpui->hpenEnemy = win32::CreatePen(0, 1, 0xff);
    app.lpui->hpenMassPath = win32::CreatePen(2, 1, 8355711);
    app.lpui->hpenRadar = win32::CreatePen(0, 1, 127);
    app.lpui->hpenRadarNear = nullptr;
    app.lpui->hpenDkBlue = win32::CreatePen(0, 1, 8323072);
    app.lpui->hpenYellow = win32::CreatePen(0, 1, 0xffff);
    app.lpui->hpenDkYellow = win32::CreatePen(0, 1, 32639);
    app.lpui->hpenDkPurple = win32::CreatePen(0, 1, 8323199);
    hbmp = win32::LoadBitmapA(app.lpui->hInst, "Screen50Bmp");
    app.lpui->hbr50Screen = win32::CreatePatternBrush(hbmp);
    win32::DeleteObject(hbmp);
    for (i = 0; i < 3; i++) {
        hbmp = win32::LoadBitmapA(app.lpui->hInst, win32::MAKEINTRESOURCE(i + 460));
        app.lpui->rghbrPat[i] = win32::CreatePatternBrush(hbmp);
        win32::DeleteObject(hbmp);
    }
    hbmp = win32::LoadBitmapA(app.lpui->hInst, "CargoBmp");
    app.lpui->hbrCargo = win32::CreatePatternBrush(hbmp);
    win32::DeleteObject(hbmp);
    hbmp = win32::LoadBitmapA(app.lpui->hInst, "DockBmp");
    app.lpui->hbrDock = win32::CreatePatternBrush(hbmp);
    win32::DeleteObject(hbmp);
    app.lpui->hcurScanner = win32::LoadCursorA(app.lpui->hInst, "ScannerCur");
    app.lpui->hcurScanAdd = win32::LoadCursorA(app.lpui->hInst, "ScannerAdd");
    app.lpui->hcurOpenGrab = win32::LoadCursorA(app.lpui->hInst, "OpenGrabCur");
    app.lpui->hcurCloseGrab = win32::LoadCursorA(app.lpui->hInst, "CloseGrabCur");
    app.lpui->hcurTrashCan = win32::LoadCursorA(app.lpui->hInst, win32::MAKEINTRESOURCE(IDC_TRASH_CAN));
    app.lpui->hcurNoWay = win32::LoadCursorA(app.lpui->hInst, win32::MAKEINTRESOURCE(IDC_NO_WAY));
    app.lpui->hcurResizeWE = win32::LoadCursorA(app.lpui->hInst, win32::MAKEINTRESOURCE(IDC_RESIZE_WE));
    app.lpui->hcurResizeNS = win32::LoadCursorA(app.lpui->hInst, win32::MAKEINTRESOURCE(IDC_RESIZE_NS));
    app.lpui->hcurResize4Way = win32::LoadCursorA(app.lpui->hInst, win32::MAKEINTRESOURCE(IDC_RESIZE_4WAY));
    app.lpui->hcurArrowHelp = win32::LoadCursorA(app.lpui->hInst, win32::MAKEINTRESOURCE(IDC_ARROW_HELP));
    app.lpui->hcurHand = win32::LoadCursorA(app.lpui->hInst, win32::MAKEINTRESOURCE(IDC_HAND));
    app.lpui->hbmpScanner = win32::LoadBitmapA(app.lpui->hInst, "ScannerBmp");
    app.lpui->hbmpScanShip = win32::LoadBitmapA(app.lpui->hInst, win32::MAKEINTRESOURCE(IDDIB_SCANNER_TOOLBAR));
    app.lpui->hbmpUnknownPlanet = win32::LoadBitmapA(app.lpui->hInst, "UnknownPlanetBmp");
    app.lpui->hbmpNumbers = win32::LoadBitmapA(app.lpui->hInst, win32::MAKEINTRESOURCE(IDB_FONT_DIGITS));
    app.lpui->hdibPlanets = HdibLoadBigResource(app, IDDIB_PLANET_ICONS);
    app.lpui->hdibThings = HdibLoadBigResource(app, IDDIB_THING_ICONS);
    app.lpui->hdibToolbar = HdibLoadBigResource(app, IDB_TOOLBAR);
    if (!app.lpui->hdibPlanets || !app.lpui->hdibThings || !app.lpui->hdibToolbar) {
        fFailed = 1;
    }
    for (i = 0; i < 5; i++) {
        app.lpui->rghdibShips[i] = HdibLoadBigResource(app, static_cast<BitmapId>(i + 552));
        if (!app.lpui->rghdibShips[i]) {
            fFailed = 1;
        }
        app.lpui->rghdibShipsT[i] = HdibLoadBigResource(app, static_cast<BitmapId>(i + 557));
        if (!app.lpui->rghdibShipsT[i]) {
            fFailed = 1;
        }
    }
    for (i = 0; i < 7; i++) {
        app.lpui->rghdibInventory[i] = HdibLoadBigResource(app, static_cast<BitmapId>(i + 500));
        if (!app.lpui->rghdibInventory[i]) {
            fFailed = 1;
        }
    }
    app.lpui->vhpal = HpalFromDib(app.lpui->rghdibShips[3]);
    app.lpui->hdibRaces = HdibLoadBigResource(app, IDDIB_PLAYER_ICONS);
    app.lpui->hdibRacesT = HdibLoadBigResource(app, IDDIB_PLAYER_ICONS_SMALL);
    app.lpui->hdibRacesX = HdibLoadBigResource(app, IDDIB_PLAYER_ICONS_TINY);
    app.lpui->hbmpBackBld = win32::LoadBitmapA(app.lpui->hInst, win32::MAKEINTRESOURCE(IDB_EMPTY_HULL_SLOT));
    app.lpui->hbmpMsg = win32::LoadBitmapA(app.lpui->hInst, win32::MAKEINTRESOURCE(IDB_MSGFILTER_CHECKBOX));
    app.lpui->hbmpMono = win32::LoadBitmapA(app.lpui->hInst, win32::MAKEINTRESOURCE(IDB_FILTER_CHECKBOX_MONO));
    app.lpui->hdibPlaque = HdibLoadBigResource(app, IDDIB_NUM_DESIGNS_PLATE);
    app.lpui->hiconStars = win32::LoadIconA(app.lpui->hInst, "StarsIco");
    app.lpui->hiconHost = win32::LoadIconA(app.lpui->hInst, "HostIco");
    app.lpui->hiconWait = win32::LoadIconA(app.lpui->hInst, "WaitIco");
    app.lpui->rghiconVCR[0] = win32::LoadIconA(app.lpui->hInst, "Bang1Ico");
    app.lpui->rghiconVCR[1] = win32::LoadIconA(app.lpui->hInst, "Bang2Ico");
    app.lpui->rghiconVCR[2] = win32::LoadIconA(app.lpui->hInst, "Bang3Ico");
    app.lpui->rghiconVCR[3] = win32::LoadIconA(app.lpui->hInst, "Torp1Ico");
    app.lpui->rghiconVCR[4] = win32::LoadIconA(app.lpui->hInst, "Torp2Ico");
    app.lpui->rghiconVCR[5] = win32::LoadIconA(app.lpui->hInst, "Torp3Ico");
    app.lpui->rghiconVCR[6] = win32::LoadIconA(app.lpui->hInst, "Torp4Ico");
    app.lpui->lpfnFakeComboProc = (FakeComboProc);
    app.lpui->lpfnFakeCEProc = (FakeCEProc);
    app.lpui->lpfnFakeEditProc = (FakeEditProc);
    app.lpui->lpfnFakeListProc = (FakeListProc);
    app.lpui->lpfnHostTimerProc = (HostTimerProc);
    app.lpui->lpfnBrowserDlgProc = BrowserDlg;
    app.lpui->lpfnReportDlgProc = (ReportDlg);
    app.lpui->lpfnGaugeDlgProc = ProgressGaugeDlg;
    app.lpui->lpb2k = LpAlloc<std::uint8_t>(app, 0x800, htPerm);
    app.lpui->vlprgidMisc = LpAlloc<std::uint16_t>(app, 0x800, htPerm);
    InitGameStuff(app);
    if (fFailed || !app.lpui->hbmpScanner || !app.lpui->hbmpUnknownPlanet || !app.lpui->hbmpBackBld || !app.lpui->hdibRaces || !app.lpui->hdibRacesT || !app.lpui->hdibRacesX || !app.lpui->hbmpMono || !app.lpui->hbmpScanShip || !app.lpui->hbmpMsg ||
        !app.lpui->hiconHost || !app.lpui->hiconStars || !app.lpui->hiconWait) {
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableLoadBitmaps, nullptr), win32::MB_ICONHAND);
        return 0;
    }
    return 1;
}

std::int16_t FCreateFonts(APPSESSION &app, HDC hdc) {
    NATIVEOWNER<LOGFONT, win32::LocalFree> plfOwner;
    std::int16_t    i;
    LOGFONT   *plf;
    HFONT      hfontSav;
    TEXTMETRIC tm;
    std::int32_t    l;

    plf = static_cast<LOGFONT *>(win32::LocalAlloc(64, sizeof(LOGFONT)));
    plfOwner.reset(plf);
    for (i = 0; i < 4; i++) {
        if (app.lpui->rgszArial[i][0] == 0) {
            TextIO().pfnCchGetString(app, static_cast<StringId>(idsArial2 + i), std::span{app.lpui->rgszArial[i]});
        }
    }
    plf->lfHeight = -win32::MulDiv(10, win32::GetDeviceCaps(hdc, win32::LOGPIXELSY), 72);
    for (i = 0; i < 2; i++) {
        std::strcpy(plf->lfFaceName, app.lpui->rgszArial[i]);
        app.lpui->rghfontArial10[i] = win32::CreateFontIndirectA(plf);
    }
    std::strcpy(plf->lfFaceName, app.lpui->rgszArial[0]);
    plf->lfHeight = -win32::MulDiv(6, win32::GetDeviceCaps(hdc, win32::LOGPIXELSY), 72);
    app.lpui->rghfontArial6[0] = win32::CreateFontIndirectA(plf);
    plf->lfHeight = -win32::MulDiv(7, win32::GetDeviceCaps(hdc, win32::LOGPIXELSY), 72);
    app.lpui->rghfontArial7[0] = win32::CreateFontIndirectA(plf);
    plf->lfHeight = -win32::MulDiv(8, win32::GetDeviceCaps(hdc, win32::LOGPIXELSY), 72);
    for (i = 0; i < 4; i++) {
        std::strcpy(plf->lfFaceName, app.lpui->rgszArial[i]);
        app.lpui->rghfontArial8[i] = win32::CreateFontIndirectA(plf);
    }
    std::strcpy(plf->lfFaceName, app.lpui->rgszArial[1]);
    plf->lfEscapement = 3150;
    app.lpui->rghfontArial8[4] = win32::CreateFontIndirectA(plf);
    hfontSav = static_cast<HFONT>(win32::SelectObject(hdc, app.lpui->rghfontArial8[0]));
    win32::GetTextMetricsA(hdc, &tm);
    app.state.dyArial8 = tm.tmHeight + tm.tmExternalLeading;
    l = GetTextExtent(hdc, "88888888kT", 10);
    app.lpui->dxMaxMineralQuan = LOWORD(l);
    win32::SelectObject(hdc, app.lpui->rghfontArial7[0]);
    win32::GetTextMetricsA(hdc, &tm);
    app.lpui->dyArial7 = tm.tmHeight + tm.tmExternalLeading;
    win32::SelectObject(hdc, app.lpui->rghfontArial6[0]);
    win32::GetTextMetricsA(hdc, &tm);
    app.lpui->dyArial6 = tm.tmHeight + tm.tmExternalLeading;
    win32::SelectObject(hdc, app.lpui->rghfontArial10[0]);
    win32::GetTextMetricsA(hdc, &tm);
    app.lpui->dyArial10 = tm.tmHeight + tm.tmExternalLeading;
    win32::SelectObject(hdc, hfontSav);
    plfOwner.reset();
    return 1;
}

#ifdef __APPLE__
static std::int16_t FSetMacMenuShortcut(HMENU hmenu, UINT id, const ACCEL *lpaccel) {
    char szItem[128], szLabel[160];
    char *pszTab;
    int i, cch;
    UINT mf;

    for (i = 0; i < win32::GetMenuItemCount(hmenu); i++) {
        if (win32::GetMenuItemID(hmenu, i) != id) continue;
        if (win32::GetMenuStringA(hmenu, i, nullptr, 0, win32::MF_BYPOSITION) >= static_cast<int>(sizeof(szItem))) return 0;
        if (win32::GetMenuStringA(hmenu, i, szItem, sizeof(szItem), win32::MF_BYPOSITION) == 0) return 0;
        pszTab = std::strchr(szItem, '\t');
        if (pszTab) *pszTab = 0;
        cch = CchSnprintf(std::span{szLabel, sizeof(szLabel)}, "%s\tCmd+%s%c", szItem,
                         lpaccel->fVirt & win32::FSHIFT ? "Shift+" : "", lpaccel->key);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szLabel)) return 0;
        mf = win32::GetMenuState(hmenu, i, win32::MF_BYPOSITION) & (win32::MF_CHECKED | win32::MF_DISABLED | win32::MF_GRAYED);
        if (!win32::InsertMenuA(hmenu, i, win32::MF_BYPOSITION | mf, id, szLabel)) return 0;
        return win32::RemoveMenu(hmenu, i + 1, win32::MF_BYPOSITION);
    }
    return 0;
}

static std::int16_t FInitMacShortcuts(APPSESSION &app, HWND hwnd) {
    static const struct {
        UINT idMenu;
        ACCEL accel;
    } rgkey[] = {
        {IDM_TOOL_NEW_GAME, {win32::FVIRTKEY | win32::FCONTROL, 'N', IDM_FILE_NEW_GAME}},
        {IDM_TOOL_OPEN_GAME, {win32::FVIRTKEY | win32::FCONTROL, 'O', IDM_FILE_OPEN_GAME}},
        {IDM_TURN_END_A, {win32::FVIRTKEY | win32::FCONTROL, 'S', IDM_FILE_SAVE}},
        {IDM_TURN_END_B, {win32::FVIRTKEY | win32::FCONTROL | win32::FSHIFT, 'S', IDM_TURN_SAVE_SUBMIT}},
        {IDM_VIEW_FIND, {win32::FVIRTKEY | win32::FCONTROL, 'F', IDM_VIEW_FIND2}},
        {IDM_FILE_RETURN_TO_TITLE, {win32::FVIRTKEY | win32::FCONTROL, 'W', IDM_FILE_RETURN_TO_TITLE}},
        {IDM_FILE_EXIT, {win32::FVIRTKEY | win32::FCONTROL, 'Q', IDM_FILE_EXIT}}
    };
    ACCEL rgaccel[32];
    ACCEL rgtitle[] = {
        {win32::FVIRTKEY | win32::FCONTROL, 'N', IDM_TITLE_NEW_GAME},
        {win32::FVIRTKEY | win32::FCONTROL, 'O', IDM_TITLE_OPEN_GAME},
        {win32::FVIRTKEY | win32::FCONTROL, 'R', IDM_TITLE_CONTINUE},
        {win32::FVIRTKEY | win32::FCONTROL, 'Q', IDM_TITLE_EXIT}
    };
    HACCEL hmain, htitle;
    HMENU hmenu, hsub;
    int c, i, j;
    std::size_t ikey;

    c = win32::CopyAcceleratorTableA(app.lpui->hAccel, nullptr, 0);
    if (c <= 0 || static_cast<std::size_t>(c) + sizeof(rgkey) / sizeof(rgkey[0]) > sizeof(rgaccel) / sizeof(rgaccel[0]) ||
        win32::CopyAcceleratorTableA(app.lpui->hAccel, rgaccel, c) != c) return 0;
    hmenu = win32::GetMenu(hwnd);
    if (!hmenu || win32::GetMenuItemCount(hmenu) <= 0) return 0;
    for (ikey = 0; ikey < sizeof(rgkey) / sizeof(rgkey[0]); ikey++) {
        for (i = 0; i < c && rgaccel[i].cmd != rgkey[ikey].accel.cmd; i++) {}
        rgaccel[i] = rgkey[ikey].accel;
        if (i == c) c++;
        for (j = 0; j < win32::GetMenuItemCount(hmenu); j++) {
            hsub = win32::GetSubMenu(hmenu, j);
            if (hsub && win32::GetMenuState(hsub, rgkey[ikey].idMenu, win32::MF_BYCOMMAND) != UINT_MAX) break;
        }
        if (j == win32::GetMenuItemCount(hmenu) || !FSetMacMenuShortcut(hsub, rgkey[ikey].idMenu, &rgkey[ikey].accel)) return 0;
    }
    hmain = win32::CreateAcceleratorTableA(rgaccel, c);
    if (!hmain) return 0;
    htitle = win32::CreateAcceleratorTableA(rgtitle, sizeof(rgtitle) / sizeof(rgtitle[0]));
    if (!htitle) { win32::DestroyAcceleratorTable(hmain); return 0; }
    app.lpui->hAccel = hmain;
    app.lpui->hAccelTitle = htitle;
    return 1;
}
#endif

std::int16_t InitInstance(APPSESSION &app, std::int16_t nCmdShow) {
    std::int16_t sw;
    RECT    rc;


    StarsCompatSetObjectInspector(ShowScannerInspector);
    app.state.ini.fWait = 0;
    app.state.ini.fStartupFile = 0;
    app.state.ini.grobjSel = 0;
    app.state.ini.idPlayer = iplrNone;
    ReadIniSettings(app);
    rc = app.state.ini.wnFrame.rc;
    /* Stars.ini holds -32768, the Win16 CW_USEDEFAULT, when it has no frame position. */
    if (rc.left == -32768)
        rc.left = win32::CW_USEDEFAULT;
    if (rc.right == -32768)
        rc.right = win32::CW_USEDEFAULT;
    app.lpui->hwndFrame = ui::CreateWindowEx(0, app.lpui->szFrame, "Stars!", win32::WS_OVERLAPPEDWINDOW, rc.left, rc.top, rc.right, rc.bottom, nullptr, nullptr, app.lpui->hInst, nullptr);
    if (!app.lpui->hwndFrame) {
        return 0;
    }
    app.lpui->hAccel = win32::LoadAcceleratorsA(app.lpui->hInst, win32::MAKEINTRESOURCE(IDA_MAIN));
    if (!app.lpui->hAccel) {
        return 0;
    }
    app.lpui->hAccelTitle = win32::LoadAcceleratorsA(app.lpui->hInst, win32::MAKEINTRESOURCE(IDA_TITLE));
    if (!app.lpui->hAccelTitle) {
        return 0;
    }
#ifdef __APPLE__
    if (!FInitMacShortcuts(app, app.lpui->hwndFrame)) return 0;
#endif
    if (nCmdShow != 1) {
        sw = nCmdShow;
    } else {
        sw = app.state.ini.wnFrame.fMaximized != 0 || app.state.ini.wnFrame.fMinimized ? 3 : 1;
    }
    // Showing then hiding the frame exposes the native maximize animation before the title screen.
    StarsCompatSetInitialShowCommand(app.lpui->hwndFrame, sw);
    return 1;
}

void InitTiles(APPSESSION &app) {
    std::int16_t  yTop;
    std::int16_t  ctile;
    TILE    *rgtile;
    std::int16_t  i;
    std::int16_t  iPass;
    std::uint16_t iCol;

    iPass = 2;
    rgtile = app.lpui->rgtilePlanet;
    ctile = 6;
    while (iPass-- != 0) {
        iCol = 0;
        yTop = 4;
        for (i = 0; i < ctile; i++) {
            if (rgtile[i].iCol != iCol) {
                yTop = 4;
                iCol = rgtile[i].iCol;
            }
            rgtile[i].dyFull += rgtile[i].yTop * app.state.dyArial8;
            rgtile[i].yTop = yTop;
            rgtile[i].fFixCtls = 0;
            rgtile[i].fMinDraw = 0;
            if (rgtile[i].fPopped) {
                yTop += rgtile[i].dyFull + 4;
            } else {
                yTop += app.state.dyArial8 + 7;
            }
        }
        rgtile = app.lpui->rgtileShip;
        ctile = 7;
    }
    return;
}

void GetIniWinRc(APPSESSION &app, char *szSection, char *szIniFile, StringId ids, WN *pwn) {
    std::int16_t fInitalized;
    std::int16_t fMinimized;
    std::int16_t fMaximized;
    char    szEntry[16];
    std::int16_t cch;
    RECT    rc;
    std::int16_t j;
    char   *pch;
    std::int16_t i;
    std::int16_t fNeg;
    std::int16_t rg[4];

    TextIO().pfnCchGetString(app, ids, std::span{szEntry});
    cch = ProfileTextIO().pfnReadString(szSection, szEntry, "X", app.state.szWork, 20, szIniFile);
    if (cch != 17 || (app.state.szWork[0] != 'M' && app.state.szWork[0] != 'R' && app.state.szWork[0] != 'I')) {
    NoRc:
        rc.left = -32768;
        rc.right = -32768;
        rc.bottom = 0;
        rc.top = 0;
        fMaximized = ids == idsMain;
        fMinimized = 0;
        fInitalized = 0;
    } else {
        pch = &app.state.szWork[1];
        for (i = 0; i < 4; i++) {
            rg[i] = 0;
            fNeg = 0;
            for (j = 0; j < 4; j++, pch++) {
                if (*pch == '-') {
                    fNeg = 1;
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
        fMaximized = app.state.szWork[0] == 'M';
        fMinimized = app.state.szWork[0] == 'I';
        fInitalized = 1;
    }
    pwn->rc = rc;
    pwn->fMaximized = fMaximized;
    pwn->fMinimized = fMinimized;
    pwn->fInitalized = fInitalized;
    return;
}

void ReadIniSettings(APPSESSION &app) {
    std::uint16_t uDateCur;
    std::int16_t  i;
    char     szEntry[16];
    WN       wnT;
    char     szIniFile[16];
    std::uint16_t w;
    char     szSection[16];
    std::int16_t  cch;

    app.state.ini.fGen = 0;
    app.state.ini.fTry = 0;
    app.state.ini.fWait = 0;
    TextIO().pfnCchGetString(app, idsWindows, std::span{szSection});
    TextIO().pfnCchGetString(app, idsStarsIni, std::span{szIniFile});
    GetIniWinRc(app, szSection, szIniFile, idsMain, &app.state.ini.wnFrame);
    GetIniWinRc(app, szSection, szIniFile, idsReportfleetwin, &wnT);
    if (wnT.rc.left != -32768) {
        app.state.vrptFleet.ptDlg.x = wnT.rc.left;
        app.state.vrptFleet.ptDlg.y = wnT.rc.top;
        app.state.vrptFleet.ptSize.x = wnT.rc.right - wnT.rc.left;
        app.state.vrptFleet.ptSize.y = wnT.rc.bottom - wnT.rc.top;
    }
    GetIniWinRc(app, szSection, szIniFile, idsReportefleetwin, &wnT);
    if (wnT.rc.left != -32768) {
        app.state.vrptEFleet.ptDlg.x = wnT.rc.left;
        app.state.vrptEFleet.ptDlg.y = wnT.rc.top;
        app.state.vrptEFleet.ptSize.x = wnT.rc.right - wnT.rc.left;
        app.state.vrptEFleet.ptSize.y = wnT.rc.bottom - wnT.rc.top;
    }
    GetIniWinRc(app, szSection, szIniFile, idsReportbtlwin, &wnT);
    if (wnT.rc.left != -32768) {
        app.state.vrptBattle.ptDlg.x = wnT.rc.left;
        app.state.vrptBattle.ptDlg.y = wnT.rc.top;
        app.state.vrptBattle.ptSize.x = wnT.rc.right - wnT.rc.left;
        app.state.vrptBattle.ptSize.y = wnT.rc.bottom - wnT.rc.top;
    }
    GetIniWinRc(app, szSection, szIniFile, idsReportplanwin, &wnT);
    if (wnT.rc.left != -32768) {
        app.state.vrptPlanet.ptDlg.x = wnT.rc.left;
        app.state.vrptPlanet.ptDlg.y = wnT.rc.top;
        app.state.vrptPlanet.ptSize.x = wnT.rc.right - wnT.rc.left;
        app.state.vrptPlanet.ptSize.y = wnT.rc.bottom - wnT.rc.top;
    }
    TextIO().pfnCchGetString(app, idsResolution, std::span{szEntry});
    i = win32::GetPrivateProfileIntA(szSection, szEntry, 0, szIniFile);
    if (i == 0 && (app.lpui->vcScreenColors <= 4 || app.state.gd.mdScreenSize == 0)) {
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsNoteStarsPrefersScreenResolutionLeast800x600, nullptr), win32::MB_ICONHAND);
    }
    TextIO().pfnCchGetString(app, idsLayout, std::span{szEntry});
    app.lpui->iWindowLayout = static_cast<WindowLayout>(win32::GetPrivateProfileIntA(szSection, szEntry, 1, szIniFile));
    app.lpui->iWindowLayout = 2 < (layoutLarge <= static_cast<std::int16_t>(app.lpui->iWindowLayout) ? app.lpui->iWindowLayout : 0) ? layoutSmall
                    : layoutLarge > static_cast<std::int16_t>(app.lpui->iWindowLayout)                          ? layoutLarge
                                                                                    : app.lpui->iWindowLayout;
    TextIO().pfnCchGetString(app, idsStyle1width, std::span{szEntry});
    app.lpui->vfs.dxPlanWant = win32::GetPrivateProfileIntA(szSection, szEntry, 396, szIniFile);
    app.lpui->vfs.dxPlanWant = (app.lpui->vfs.dxPlanWant <= 10 ? 10 : app.lpui->vfs.dxPlanWant) >= 2000 ? 2000 : app.lpui->vfs.dxPlanWant > 10 ? app.lpui->vfs.dxPlanWant : 10;
    TextIO().pfnCchGetString(app, idsStyle1height, std::span{szEntry});
    app.lpui->vfs.dyMsgWant = win32::GetPrivateProfileIntA(szSection, szEntry, 110, szIniFile);
    app.lpui->vfs.dyMsgWant = (app.lpui->vfs.dyMsgWant <= 10 ? 10 : app.lpui->vfs.dyMsgWant) >= 2000 ? 2000 : app.lpui->vfs.dyMsgWant > 10 ? app.lpui->vfs.dyMsgWant : 10;
    TextIO().pfnCchGetString(app, idsStyle1height2, std::span{szEntry});
    app.lpui->vfs.dyMinWant = win32::GetPrivateProfileIntA(szSection, szEntry, 192, szIniFile);
    app.lpui->vfs.dyMinWant = (app.lpui->vfs.dyMinWant <= 10 ? 10 : app.lpui->vfs.dyMinWant) >= 2000 ? 2000 : app.lpui->vfs.dyMinWant > 10 ? app.lpui->vfs.dyMinWant : 10;
    TextIO().pfnCchGetString(app, idsStyle2width, std::span{szEntry});
    app.lpui->vfs.dx2PlanWant = win32::GetPrivateProfileIntA(szSection, szEntry, 396, szIniFile);
    app.lpui->vfs.dx2PlanWant = (app.lpui->vfs.dx2PlanWant <= 10 ? 10 : app.lpui->vfs.dx2PlanWant) >= 2000 ? 2000 : app.lpui->vfs.dx2PlanWant > 10 ? app.lpui->vfs.dx2PlanWant : 10;
    TextIO().pfnCchGetString(app, idsStyle2height, std::span{szEntry});
    app.lpui->vfs.dy2MsgWant = win32::GetPrivateProfileIntA(szSection, szEntry, 110, szIniFile);
    app.lpui->vfs.dy2MsgWant = (app.lpui->vfs.dy2MsgWant <= 10 ? 10 : app.lpui->vfs.dy2MsgWant) >= 2000 ? 2000 : app.lpui->vfs.dy2MsgWant > 10 ? app.lpui->vfs.dy2MsgWant : 10;
    TextIO().pfnCchGetString(app, idsStyle2height2, std::span{szEntry});
    app.lpui->vfs.dy2MinWant = win32::GetPrivateProfileIntA(szSection, szEntry, 192, szIniFile);
    app.lpui->vfs.dy2MinWant = (app.lpui->vfs.dy2MinWant <= 10 ? 10 : app.lpui->vfs.dy2MinWant) >= 2000 ? 2000 : app.lpui->vfs.dy2MinWant > 10 ? app.lpui->vfs.dy2MinWant : 10;
    TextIO().pfnCchGetString(app, idsToolbar, std::span{szEntry});
    i = win32::GetPrivateProfileIntA(szSection, szEntry, 1, szIniFile);
    app.state.gd.fToolbar = i != 0;
    TextIO().pfnCchGetString(app, idsPlanettiles, std::span{szEntry});
    cch = ProfileTextIO().pfnReadString(szSection, szEntry, "X", app.state.szWork, 20, szIniFile);
    ReadIniTileSettings(app.state.szWork, app.lpui->rgtilePlanet, 6);
    TextIO().pfnCchGetString(app, idsShiptiles, std::span{szEntry});
    cch = ProfileTextIO().pfnReadString(szSection, szEntry, "X", app.state.szWork, 20, szIniFile);
    ReadIniTileSettings(app.state.szWork, app.lpui->rgtileShip, 7);
    TextIO().pfnCchGetString(app, idsSelection, std::span{szEntry});
    cch = ProfileTextIO().pfnReadString(szSection, szEntry, "N", app.state.szWork, 20, szIniFile);
    ReadIniSelection(app, app.state.szWork, cch);
    TextIO().pfnCchGetString(app, idsMessage, std::span{szEntry});
    i = win32::GetPrivateProfileIntA(szSection, szEntry, 0, szIniFile);
    app.state.ini.iMsg = i;
    TextIO().pfnCchGetString(app, idsGameid, std::span{szEntry});
    cch = ProfileTextIO().pfnReadString(szSection, szEntry, "0", app.state.szWork, 10, szIniFile);
    ReadIniIdentity(app, app.state.szWork, cch);
    TextIO().pfnCchGetString(app, idsScanzoom, std::span{szEntry});
    w = win32::GetPrivateProfileIntA(szSection, szEntry, 4, szIniFile);
    if (w >= 1 && w <= 9) {
        app.state.iScanZoom = static_cast<ScanZoom>(w - 5);
    }
    TextIO().pfnCchGetString(app, idsScanfilterv25, std::span{szEntry});
    w = win32::GetPrivateProfileIntA(szSection, szEntry, 0, szIniFile);
    app.lpui->grbitScanShip = w;
    TextIO().pfnCchGetString(app, idsScanefilterv25, std::span{szEntry});
    w = win32::GetPrivateProfileIntA(szSection, szEntry, 0, szIniFile);
    app.lpui->grbitScanEShip = w;
    TextIO().pfnCchGetString(app, idsScanmines, std::span{szEntry});
    w = win32::GetPrivateProfileIntA(szSection, szEntry, 15, szIniFile);
    app.lpui->grbitScanMines = w & 0xf;
    TextIO().pfnCchGetString(app, idsScanradar, std::span{szEntry});
    w = win32::GetPrivateProfileIntA(szSection, szEntry, 100, szIniFile);
    if (w < 0 || w > 100) {
        w = 100;
    }
    app.lpui->vpctRadarView = w;
    TextIO().pfnCchGetString(app, idsScanmodev25, std::span{szEntry});
    w = win32::GetPrivateProfileIntA(szSection, szEntry, 224, szIniFile);
    app.lpui->grbitScan = static_cast<GrbitScan>(w & 0xc00f);
    if (app.lpui->grbitScan >= 6) {
        app.lpui->grbitScan = static_cast<GrbitScan>(0);
        app.lpui->grbitScanShip = 0;
    } else {
        app.lpui->grbitScan = static_cast<GrbitScan>(app.lpui->grbitScan | w & 0x3ff0);
    }
    TextIO().pfnCchGetString(app, idsMineralscale, std::span{szEntry});
    app.lpui->cMinGrafMax = win32::GetPrivateProfileIntA(szSection, szEntry, app.lpui->cMinGrafMax, szIniFile);
    if (app.lpui->cMinGrafMax < 100 || app.lpui->cMinGrafMax > 30000) {
        app.lpui->cMinGrafMax = 5000;
    }
    TextIO().pfnCchGetString(app, idsFiles, std::span{szSection});
    TextIO().pfnCchGetString(app, idsLogging, std::span{szEntry});
    app.state.ini.fLogging = win32::GetPrivateProfileIntA(szSection, szEntry, 0, szIniFile);
    TextIO().pfnCchGetString(app, idsWait2, std::span{szEntry});
    app.state.ini.fWait = win32::GetPrivateProfileIntA(szSection, szEntry, 0, szIniFile);
    TextIO().pfnCchGetString(app, idsFile1, std::span{szEntry});
    cch = ProfileTextIO().pfnReadString(szSection, szEntry, ".", app.state.szWork, 256, szIniFile);
    ReadIniRecent(app, szSection, szEntry, szIniFile, cch);
    TextIO().pfnCchGetString(app, idsTurn, std::span{szEntry});
    app.state.ini.turn = win32::GetPrivateProfileIntA(szSection, szEntry, app.state.game.turn, szIniFile);
    TextIO().pfnCchGetString(app, idsMisc, std::span{szSection});
    TextIO().pfnCchGetString(app, idsDefaultpassword, std::span{szEntry});
    ProfileTextIO().pfnReadString(szSection, szEntry, "", app.state.vszDefPass, 16, szIniFile);
    TextIO().pfnCchGetString(app, idsProgress, std::span{szEntry});
    w = win32::GetPrivateProfileIntA(szSection, szEntry, 0, szIniFile);
    app.state.gd.fProgressTxt = w != 0;
    TextIO().pfnCchGetString(app, idsNewreports, std::span{szEntry});
    w = win32::GetPrivateProfileIntA(szSection, szEntry, 0, szIniFile);
    app.state.gd.fPerPlayerDumps = w != 0;
    TextIO().pfnCchGetString(app, idsNohostnames, std::span{szEntry});
    w = win32::GetPrivateProfileIntA(szSection, szEntry, 0, szIniFile);
    app.state.gd.fNoHostNames = w != 0;
    TextIO().pfnCchGetString(app, idsBackups, std::span{szEntry});
    app.state.vcBackupDirs = win32::GetPrivateProfileIntA(szSection, szEntry, 1, szIniFile);
    if (app.state.vcBackupDirs < 1 || app.state.vcBackupDirs > 999) {
        app.state.vcBackupDirs = 1;
    }
    TextIO().pfnCchGetString(app, idsReportplanfld, std::span{szEntry});
    app.state.vrptPlanet.grbitVisible = static_cast<std::uint32_t>(win32::GetPrivateProfileIntA(szSection, szEntry, -1, szIniFile));
    TextIO().pfnCchGetString(app, idsReportplansort, std::span{szEntry});
    i = win32::GetPrivateProfileIntA(szSection, szEntry, 0, szIniFile);
    app.state.vrptPlanet.fAscending = (i & 0x100) != 0;
    i &= 0xff;
    app.state.vrptPlanet.icolSort = i;
    TextIO().pfnCchGetString(app, idsReportfleetfld, std::span{szEntry});
    app.state.vrptFleet.grbitVisible = static_cast<std::uint32_t>(win32::GetPrivateProfileIntA(szSection, szEntry, -1, szIniFile));
    TextIO().pfnCchGetString(app, idsReportfleetsort, std::span{szEntry});
    i = win32::GetPrivateProfileIntA(szSection, szEntry, 0, szIniFile);
    app.state.vrptFleet.fAscending = (i & 0x100) != 0;
    i &= 0xff;
    app.state.vrptFleet.icolSort = i;
    TextIO().pfnCchGetString(app, idsReportefleetfld, std::span{szEntry});
    app.state.vrptEFleet.grbitVisible = static_cast<std::uint32_t>(win32::GetPrivateProfileIntA(szSection, szEntry, -1, szIniFile));
    TextIO().pfnCchGetString(app, idsReportefltsort, std::span{szEntry});
    i = win32::GetPrivateProfileIntA(szSection, szEntry, 0, szIniFile);
    app.state.vrptEFleet.fAscending = (i & 0x100) != 0;
    i &= 0xff;
    app.state.vrptEFleet.icolSort = i;
    TextIO().pfnCchGetString(app, idsReportbtlfld, std::span{szEntry});
    app.state.vrptBattle.grbitVisible = static_cast<std::uint32_t>(win32::GetPrivateProfileIntA(szSection, szEntry, -1, szIniFile));
    TextIO().pfnCchGetString(app, idsReportbtlsort, std::span{szEntry});
    i = win32::GetPrivateProfileIntA(szSection, szEntry, 0, szIniFile);
    app.state.vrptBattle.fAscending = (i & 0x100) != 0;
    i &= 0xff;
    app.state.vrptBattle.icolSort = i;
    TextIO().pfnCchGetString(app, idsReportdefgraph, std::span{szEntry});
    app.state.gd.iCurGraph = win32::GetPrivateProfileIntA(szSection, szEntry, 7, szIniFile);
    TextIO().pfnCchGetString(app, idsReportefltsort, std::span{szEntry});
    if (app.state.gd.iCurGraph > 7) {
        app.state.gd.iCurGraph = 7;
    }
    TextIO().pfnCchGetString(app, idsVcrspeed, std::span{szEntry});
    app.lpui->viSpeedVCR = win32::GetPrivateProfileIntA(szSection, szEntry, 1, szIniFile);
    _strdate(app.state.szWork);
    app.state.szWork[5] = 0;
    app.state.szWork[2] = 0;
    uDateCur = std::atoi(&app.state.szWork[3]) + std::atoi(app.state.szWork) * 31 + std::atoi(&app.state.szWork[6]) * 31 * 12;
    TextIO().pfnCchGetString(app, idsHistoryinfo, std::span{szEntry});
    app.lpui->uDateInstalled = win32::GetPrivateProfileIntA(szSection, szEntry, -1, szIniFile);
    if (uDateCur < app.lpui->uDateInstalled) {
        app.lpui->uDateInstalled = uDateCur;
    }
    app.state.gd.fTrialPeriodOver = uDateCur >= static_cast<std::uint16_t>((app.lpui->uDateInstalled + 21));
    TextIO().pfnCchGetString(app, idsFonts, std::span{szSection});
    for (i = 0; i < 4; i++) {
        TextIO().pfnCchGetString(app, static_cast<StringId>(idsArial + i), std::span{szEntry});
        ProfileTextIO().pfnReadString(szSection, szEntry, "", app.state.szWork, 80, szIniFile);
        cch = std::strlen(app.state.szWork);
        if (cch >= 5 && cch <= 31) {
            std::strcpy(app.lpui->rgszArial[i], app.state.szWork);
        }
    }
    ReadIniPresets(app, szIniFile);
    return;
}

void ReadIniTileSettings(char *pszFormat, TILE *rgtile, std::int16_t ctile) {
    TILE     tile;
    std::int16_t  fPopped;
    std::int16_t  i;
    std::int16_t  iTile;
    std::uint16_t iCol;
    std::uint16_t iBit;

    iCol = 0;
    iTile = 0;
    while (*pszFormat != 0) {
        if (*pszFormat == '*') {
            if (iCol < 1) {
                iCol++;
            }
            goto DoNext;
        }
        if (std::isalpha(static_cast<unsigned char>(*pszFormat)) == 0)
            goto DoNext;
        // C classification functions return any nonzero mask, but the saved
        // expansion state is stored in a one-bit field.
        fPopped = std::isupper(static_cast<unsigned char>(*pszFormat)) != 0;
        iBit = !fPopped ? *pszFormat - 'a' : *pszFormat - 'A';
        if (static_cast<std::int16_t>(iBit) < 0)
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
