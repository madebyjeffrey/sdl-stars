#include "acutest.h"
#include "stars_test.h"
#include <stars_compat.h>
#include <SDL3/SDL_stdinc.h>
#include "appstate.h"
#include "uistate.h"
#include "callbackcontext.h"
#include "control_io.h"
#include "createui.h"
#include "librarypublication.h"
#include "planetui.h"
#include "buildui.h"
#include "mineui.h"
#include "msgui.h"
#include "tutor.h"
#include "utilgenui.h"
#include "text_io.h"
#include <algorithm>
#include <array>
#include <string_view>
#include <string>

static void TestNewGameTitleBounds(void) {
    APPSESSION &app = ui::app;
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    HWND hwnd;
    TEST_ASSERT(FStarsTestInit(app));
    const std::array<std::string, 7> rgszBase = {std::string(33, 'a'), "short-Ω", std::string(31, 'b'),
        std::string(30, 'c') + "Ω", std::string(29, 'd') + "😀", std::string(255, 'e'), ""};
    for (const auto &szBase : rgszBase) {
        TEST_ASSERT(CchCopyUtf8(std::span{app.state.szBase}, szBase) >= 0);
        CchCopyUtf8(std::span{app.state.game.szName}, "Before");
        hwnd = ui::CreateDialog(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_NEW_GAME_1), nullptr, NewGameDlg, 0);
        TEST_ASSERT(hwnd);
        TEST_ASSERT(ui::SetWindowText(win32::GetDlgItem(hwnd, IDC_NEW_GAME_NAME), ""));
        const GDATA gdSav = app.state.gd;
        TEST_CHECK(NewGameDlg(hwnd, win32::WM_COMMAND, win32::MAKEWPARAM(IDC_NEXT, win32::BN_CLICKED), 0) == 1);
        const std::string szExpected = szBase.substr(0, CchUtf8Prefix(szBase, sizeof(app.state.game.szName) - 1));
        TEST_ASSERT_(std::string_view(app.state.game.szName) == szExpected, "fallback title [%s] exceeds or differs from [%s]", app.state.game.szName, szExpected.c_str());
        TEST_CHECK(FUtf8Valid(std::string_view(app.state.game.szName)));
        TEST_CHECK(std::memcmp(&app.state.gd, &gdSav, sizeof(gdSav)) == 0);
        TEST_CHECK(std::string_view(app.state.szBase) == szBase);
        TEST_CHECK(ui::DestroyWindow(hwnd));
    }
    for (const auto id : {IDC_NEXT, IDCANCEL}) {
        CchCopyUtf8(std::span{app.state.game.szName}, "Original");
        hwnd = ui::CreateDialog(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_NEW_GAME_1), nullptr, NewGameDlg, 0);
        TEST_ASSERT(hwnd);
        TEST_ASSERT(ui::SetWindowText(win32::GetDlgItem(hwnd, IDC_NEW_GAME_NAME), "Chosen-Ω"));
        TEST_CHECK(NewGameDlg(hwnd, win32::WM_COMMAND, win32::MAKEWPARAM(id, win32::BN_CLICKED), 0) == 1);
        TEST_CHECK(std::string_view(app.state.game.szName) == (id == IDCANCEL ? "Original" : "Chosen-Ω"));
        TEST_CHECK(ui::DestroyWindow(hwnd));
    }
    StarsCompatProcessExit();
}

static void TestTextBoundaries(void) {
    HWND hwnd, hwndEdit;
    WNDCLASSA wc = {};
    char sz[32];
    wc.lpfnWndProc = ui::DefWindowProc; wc.lpszClassName = "StarsNativeTitleProbe";
    wc.hInstance = win32::GetModuleHandleA(NULL); TEST_ASSERT(win32::RegisterClassA(&wc));
    hwnd = ui::CreateWindowEx(0, wc.lpszClassName, "Native title", win32::WS_OVERLAPPED, 0, 0, 200, 100, NULL, NULL, win32::GetModuleHandleA(NULL), NULL);
    TEST_ASSERT(hwnd);
    hwndEdit = ui::CreateWindowEx(0, "EDIT", "", win32::WS_CHILD, 0, 0, 180, 24, hwnd, NULL, win32::GetModuleHandleA(NULL), NULL);
    TEST_ASSERT(hwndEdit && FSetUtf8WindowText(hwndEdit, "\xc3\xa9\xe2\x82\xac"));
    TEST_CHECK(CchGetUtf8WindowText(hwndEdit, sz, sizeof(sz)) == 5 && strcmp(sz, "\xc3\xa9\xe2\x82\xac") == 0);
    TEST_ASSERT(FSetNativeWindowTitle(hwnd, "caf\xc3\xa9-\xce\xa9"));
    TEST_CHECK(win32::GetWindowTextA(hwnd, sz, sizeof(sz)) == 8 && strcmp(sz, "caf\xc3\xa9-\xce\xa9") == 0);
    TEST_CHECK(ui::DestroyWindow(hwnd));
    StarsCompatProcessExit();
}
static INT_PTR idLibraryNameDialog;
static INT_PTR ProbeLibraryNameDialog(HINSTANCE hinst, LPCSTR pszTemplate, HWND hwndParent, DLGPROC lpfn, LPARAM lParam) {
    HWND hwnd;
    const auto rgchName = *reinterpret_cast<std::span<char, 256> *>(lParam);
    std::array<char, 256> rgchBefore;
    std::ranges::copy(rgchName, rgchBefore.begin());
    TEST_CHECK(std::string_view(rgchName.data()) == "Before");
    if (idLibraryNameDialog < 0) return idLibraryNameDialog;
    hwnd = ui::CreateDialog(hinst, pszTemplate, hwndParent, lpfn, lParam);
    TEST_CHECK(hwnd != nullptr);
    if (!hwnd) return -1;
    TEST_CHECK(std::string_view(rgchName.data()) == "Before");
    TEST_CHECK(ui::SetWindowText(win32::GetDlgItem(hwnd, IDC_EDIT1), "Choice-\xc3\xa9"));
    ui::SendMessage(hwnd, win32::WM_COMMAND, win32::MAKEWPARAM(idLibraryNameDialog > 0 ? IDOK : IDCANCEL, win32::BN_CLICKED), 0);
    if (idLibraryNameDialog > 0) {
        TEST_CHECK(std::string_view(rgchName.data()) == "Choice-\xc3\xa9");
        TEST_CHECK(std::equal(rgchName.begin() + 10, rgchName.end(), rgchBefore.begin() + 10));
    } else TEST_CHECK(std::ranges::equal(rgchName, rgchBefore));
    ui::DestroyWindow(hwnd);
    return idLibraryNameDialog;
}

static void TestLibraryNameDialog(void) {
    APPSESSION &app = ui::app;
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    char szDir[cbTestPath];
    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("library_name_dialog", szDir, sizeof(szDir)));
    TEST_ASSERT(SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "STARS_LIBRARY_DIR", szDir, true));
    CONTROLIO control;
    control.pfnDialogBox = ProbeLibraryNameDialog;
    CONTROLIOSCOPE controlScope(control);
    idLibraryNameDialog = 0;
    TEST_CHECK(!FChooseSeparateCopyName(app, nullptr, "Before"));
    idLibraryNameDialog = 1;
    TEST_CHECK(FChooseSeparateCopyName(app, nullptr, "Before"));
    idLibraryNameDialog = -1;
    TEST_CHECK(!FChooseSeparateCopyName(app, nullptr, "Before"));
    StarsCompatProcessExit();
}

static std::string szPlanetTitleName, szPlanetTitleCaptured;
static std::array<char, 360> PszPlanetTitleName(APPSESSION &app, std::int16_t id) {
    std::array<char, 360> szResult{};
    TEST_CHECK(id == 0);
    CchCopyUtf8(std::span{app.state.szWork}, szPlanetTitleName);
    CchCopyUtf8Prefix(std::span{szResult}, app.state.szWork);
    return szResult;
}
static std::int16_t CchPlanetTitlePrefix(APPSESSION &, StringId id, std::span<char> out) {
    TEST_CHECK(id == idsPlanet2);
    return static_cast<std::int16_t>(CchCopyUtf8(out, "Planet "));
}
static std::array<char, cchCatalogText> SzPlanetTitleCatalog(APPSESSION &, StringId id) {
    TEST_CHECK(id == idsPlanetView);
    std::array<char, cchCatalogText> sz{};
    CchCopyUtf8(std::span{sz}, "Planet View");
    return sz;
}
static BOOL CapturePlanetTitle(HWND hwnd, const char *psz) {
    TEST_CHECK(!hwnd);
    szPlanetTitleCaptured = psz;
    return 1;
}
static void TestPlanetTitleCapacity(void) {
    APPSESSION &app = ui::app;
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    TEXTIO text;
    text.pfnPlanetName = PszPlanetTitleName;
    text.pfnCchGetString = CchPlanetTitlePrefix;
    text.pfnCompressed = SzPlanetTitleCatalog;
    TEXTIOSCOPE textScope(text);
    CONTROLIO control;
    control.pfnSetWindowText = CapturePlanetTitle;
    CONTROLIOSCOPE controlScope(control);
    app.state.sel.grobj = grobjPlanet;
    app.state.sel.pl.id = 0;
    for (const auto &sz : {std::string("Alba"), std::string(29, 'A') + "\xc3\xa9", std::string(359, 'Z')}) {
        szPlanetTitleName = sz;
        SetPlanetTitleBar(app, nullptr);
        TEST_CHECK(szPlanetTitleCaptured == "Planet " + sz);
        TEST_CHECK(std::string_view(app.state.szWork) == sz);
        TEST_CHECK(FUtf8Valid(szPlanetTitleCaptured));
    }
    app.state.sel.grobj = grobjNone;
    SetPlanetTitleBar(app, nullptr);
    TEST_CHECK(szPlanetTitleCaptured == "Planet View");
}

static BOOL ProbeCaptionRect(HWND, RECT *prc) { *prc = {0, 0, 480, 120}; return 1; }
static BOOL ProbeCaptionPosition(HWND, HWND, int, int, int, int, UINT) { return 1; }
static void TestMineralAndTutorialText(void) {
    APPSESSION &app = ui::app;
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    TEST_ASSERT(FStarsTestInit(app));
    CONTROLIO control;
    control.pfnGetClientRect = ProbeCaptionRect;
    control.pfnSetWindowPos = ProbeCaptionPosition;
    CONTROLIOSCOPE controlScope(control);
    TEXTIO text;
    text.pfnPlanetName = PszPlanetTitleName;
    TEXTIOSCOPE textScope(text);
    const auto selSav = app.state.sel;
    app.state.sel.scan.grobj = grobjPlanet;
    app.state.sel.scan.idpl = 0;
    for (const auto &sz : {std::string("Alba"), std::string(29, 'P') + "\xc3\xa9"}) {
        szPlanetTitleName = sz;
        SetMineralTitleBar(app, nullptr);
        TEST_CHECK(std::string_view(app.lpui->szMineralTitle) == sz + " Summary");
        TEST_CHECK(std::string_view(app.state.szWork) == sz + " Summary");
    }
    FLEET fl{};
    FLEET *rgpfl[] = {&fl, nullptr};
    auto szFleet = std::to_array("FFFFFFFFFFFFFFFFFFFFFFFFFFFFF\xc3\xa9");
    const auto rglpflSav = app.state.rglpfl;
    const auto cFleetSav = app.state.cFleet;
    const auto idPlayerSav = app.state.idPlayer;
    fl.lpszName = szFleet.data();
    app.state.rglpfl = rgpfl; app.state.cFleet = 1; app.state.idPlayer = 0;
    app.state.sel.scan.grobj = grobjFleet; app.state.sel.scan.ifl = 0;
    SetMineralTitleBar(app, nullptr);
    TEST_CHECK(std::string_view(app.lpui->szMineralTitle) == std::string(szFleet.data()) + " Summary");
    app.state.rglpfl = rglpflSav; app.state.cFleet = cFleetSav;
    THING th{};
    const auto lpthSav = app.state.lpThings;
    const auto cThingSav = app.state.cThing;
    th.SetIth(ithWormhole);
    app.state.lpThings = &th; app.state.cThing = 1;
    app.state.sel.scan.grobj = grobjThing; app.state.sel.scan.ith = 0;
    SetMineralTitleBar(app, nullptr);
    TEST_CHECK(std::string_view(app.lpui->szMineralTitle) == "Wormhole Summary");
    app.state.lpThings = lpthSav; app.state.cThing = cThingSav;
    app.state.sel = selSav; app.state.idPlayer = idPlayerSav;
    const auto zipSav = app.state.vrgZip[0];
    const auto tutorSav = app.state.tutor;
    app.state.tutor.fAutoComplete = 1;
    std::fill(std::begin(app.state.vrgZip[0].szName), std::end(app.state.vrgZip[0].szName), 'q');
    for (int i = 0; i < 5; i++) app.state.vrgZip[0].txp.rgia[i].SetCQuan(100 + i);
    TEST_CHECK(FCheckZip(app, 0, std::span<const ITEMACTION, 5>{rgiaUnloadAllCol}, idsDropcol));
    TEST_CHECK(app.state.vrgZip[0].fValid && std::string_view(app.state.vrgZip[0].szName) == "DropCol");
    for (int i = 0; i < 5; i++) {
        TEST_CHECK(app.state.vrgZip[0].txp.rgia[i].iAction() == rgiaUnloadAllCol[i].iAction());
        TEST_CHECK(app.state.vrgZip[0].txp.rgia[i].cQuan() == 100 + i);
        TEST_CHECK(app.state.vrgZip[0].szName[8 + i] == 'q');
    }
    app.state.vrgZip[0] = zipSav; app.state.tutor = tutorSav;
    StarsCompatProcessExit();
}

static std::string szRawMessageCaptured;
static BOOL CaptureRawMessage(HWND, const char *psz) { szRawMessageCaptured = psz; return 1; }
static void TestRawMessageDisplay(void) {
    APPSESSION &app = ui::app;
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    WNDCLASSA wc{};
    HWND hwnd;
    std::array<std::uint8_t, 2048> rgbText;
    std::string szBody, szExpected;
    TEST_ASSERT(FStarsTestInit(app));
    wc.lpfnWndProc = ui::DefWindowProc; wc.lpszClassName = "StarsRawMessageProbe";
    wc.hInstance = win32::GetModuleHandleA(nullptr); TEST_ASSERT(win32::RegisterClassA(&wc));
    hwnd = ui::CreateWindowEx(0, wc.lpszClassName, "Message", win32::WS_OVERLAPPED, 0, 0, 200, 100, nullptr, nullptr, wc.hInstance, nullptr);
    TEST_ASSERT(hwnd);
    for (int i = 0; i < 500; i++) szBody += "\xc3\xa9";
    CchCopyUtf8Prefix(std::span{app.state.rgplr[0].szNames}, "Senders-\xc3\xa9");
    CchCopyUtf8Prefix(std::span{app.state.rgplr[1].szNames}, "Receivers-\xce\xa9");
    CchCopyUtf8Prefix(std::span{app.state.rgplr[0].szName}, "Sender");
    CchCopyUtf8Prefix(std::span{app.state.rgplr[1].szName}, "Receiver");
    szExpected = "From:  " + std::string(PszPlayerName(app, 0, 1, 1, 1, 0, nullptr).data()) + "\r\nTo:  " + PszPlayerName(app, 1, 1, 1, 1, 0, nullptr).data() + "\r\n" + szBody;
    MSGPLR *lpmp = LpmsgplrAlloc(app, sizeof(MSGPLR) + szBody.size() + 1);
    lpmp->iPlrFrom = 0; lpmp->iPlrTo = 2; lpmp->cLen = -1001; lpmp->lpmsgplrNext = nullptr;
    TEST_ASSERT(CchCopyUtf8(std::span{reinterpret_cast<char *>(lpmp->rgbMsg()), szBody.size() + 1}, szBody) == 1000);
    const auto lpbSav = app.lpui->lpb2k;
    rgbText.fill(0x5a); app.lpui->lpb2k = rgbText.data();
    app.state.vlpmsgplrIn = lpmp; app.state.cMsg = 0; app.state.iMsgCur = 0; app.state.gd.fSendMsgMode = 0;
    app.lpui->szMsgTitle[0] = 0; app.lpui->rcMsgTitle = {0, 0, 100, 20}; app.lpui->rcMsgText = {0, 20, 20, 30};
    CONTROLIO control;
    control.pfnSetWindowText = CaptureRawMessage; control.pfnSetWindowPos = ProbeCaptionPosition;
    CONTROLIOSCOPE controlScope(control);
    win32::InvalidateRect(hwnd, nullptr, 1);
    MessageWndProc(hwnd, win32::WM_PAINT, 0, 0);
    TEST_CHECK(szRawMessageCaptured == szExpected);
    TEST_CHECK(std::string_view(reinterpret_cast<char *>(rgbText.data())) == szExpected);
    TEST_CHECK(std::all_of(rgbText.begin() + szExpected.size() + 1, rgbText.end(), [](std::uint8_t b) { return b == 0x5a; }));
    app.lpui->lpb2k = lpbSav; app.state.vlpmsgplrIn = nullptr;
    TEST_CHECK(ui::DestroyWindow(hwnd));
    StarsCompatProcessExit();
}

static void TestShipCloneNameBounds(void) {
    APPSESSION &app = ui::app;
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    struct CASE { std::string_view szBefore, szAfter; };
    const CASE rgCases[] = {
        {"", " (2)"}, {")", ") (2)"}, {"1)", "1) (2)"}, {"(2)", "(3)"},
        {"Scout", "Scout (2)"}, {"Scout (2)", "Scout (3)"}, {"Scout (9)", "Scout (0)"},
        {"Scout (A)", "Scout (A) (2)"}, {"\xc3\xa9)", "\xc3\xa9) (2)"}
    };
    TEST_ASSERT(FStarsTestInit(app));
    for (const auto &cs : rgCases) {
        std::array<char, 32> rgchName;
        rgchName.fill('q');
        TEST_ASSERT(CchCopyUtf8(std::span{rgchName}, cs.szBefore) >= 0);
        MakeNewName(app, std::span<char, 32>{rgchName});
        TEST_CHECK(std::string_view(rgchName.data()) == cs.szAfter);
        TEST_MSG("before [%.*s], expected [%.*s], got [%s]", static_cast<int>(cs.szBefore.size()), cs.szBefore.data(),
                 static_cast<int>(cs.szAfter.size()), cs.szAfter.data(), rgchName.data());
        TEST_CHECK(FUtf8Valid(std::string_view(rgchName.data())));
        TEST_CHECK(std::all_of(rgchName.begin() + std::max(cs.szBefore.size(), cs.szAfter.size()) + 1,
                               rgchName.end(), [](char ch) { return ch == 'q'; }));
    }
    std::array<char, 32> rgchUnterminated;
    rgchUnterminated.fill('q');
    MakeNewName(app, std::span<char, 32>{rgchUnterminated});
    TEST_CHECK(std::all_of(rgchUnterminated.begin(), rgchUnterminated.end(), [](char ch) { return ch == 'q'; }));
    struct {
        std::array<char, 2> rgchBefore;
        std::array<char, 32> rgchName;
        std::array<char, 2> rgchAfter;
    } storage{{'(', '9'}, {')', 0}, {'x', 'y'}};
    MakeNewName(app, std::span<char, 32>{storage.rgchName});
    TEST_CHECK(std::string_view(storage.rgchName.data()) == ") (2)");
    TEST_CHECK(storage.rgchBefore[0] == '(' && storage.rgchBefore[1] == '9');
    TEST_CHECK(storage.rgchAfter[0] == 'x' && storage.rgchAfter[1] == 'y');
    StarsCompatProcessExit();
}

static void TestTextFitViews(void) {
    APPSESSION &app = ui::app;
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    WNDCLASSA wc{};
    HWND hwnd, hwndSav;
    HDC hdc;
    HGDIOBJ hfontSav;
    std::int16_t dxOne, dxTwo;
    std::array<char, 12> rgch;
    TEST_ASSERT(FStarsTestInit(app));
    wc.lpfnWndProc = ui::DefWindowProc; wc.lpszClassName = "StarsTextFitProbe";
    wc.hInstance = win32::GetModuleHandleA(nullptr); TEST_ASSERT(win32::RegisterClassA(&wc));
    hwnd = ui::CreateWindowEx(0, wc.lpszClassName, "Text fit", win32::WS_OVERLAPPED, 0, 0, 200, 100, nullptr, nullptr, wc.hInstance, nullptr);
    TEST_ASSERT(hwnd);
    hwndSav = app.lpui->hwndFrame; app.lpui->hwndFrame = hwnd;
    hdc = win32::GetDC(hwnd); TEST_ASSERT(hdc);
    hfontSav = win32::SelectObject(hdc, app.lpui->rghfontArial8[0]);
    dxOne = LOWORD(GetTextExtent(hdc, "\xc3\xa9", 2));
    dxTwo = LOWORD(GetTextExtent(hdc, "\xc3\xa9\xc3\xa9", 4));
    win32::SelectObject(hdc, hfontSav); win32::ReleaseDC(hwnd, hdc);
    TEST_ASSERT(dxOne > 0 && dxTwo > dxOne);
    rgch.fill('q'); CchCopyUtf8(std::span{rgch}, "\xc3\xa9\xc3\xa9");
    TEST_CHECK(FStringFitsScreen(app, std::span{rgch}.first(5), dxTwo));
    TEST_CHECK(std::string_view(rgch.data()) == "\xc3\xa9\xc3\xa9");
    TEST_CHECK(!FStringFitsScreen(app, std::span{rgch}.first(5), dxOne));
    TEST_CHECK(std::string_view(rgch.data()) == "\xc3\xa9");
    TEST_CHECK(std::all_of(rgch.begin() + 5, rgch.end(), [](char ch) { return ch == 'q'; }));
    rgch[0] = 0;
    TEST_CHECK(FStringFitsScreen(app, std::span{rgch}.first(1), 0));
    rgch.fill('q'); rgch[3] = 0;
    win32::SetLastError(win32::ERROR_ACCESS_DENIED);
    TEST_CHECK(!FStringFitsScreen(app, std::span{rgch}.first(3), dxOne));
    TEST_CHECK(win32::GetLastError() == win32::ERROR_ACCESS_DENIED);
    TEST_CHECK(!FStringFitsScreen(app, {}, dxOne));
    TEST_CHECK(win32::GetLastError() == win32::ERROR_ACCESS_DENIED);
    TEST_CHECK(rgch[0] == 'q' && rgch[1] == 'q' && rgch[2] == 'q' && rgch[3] == 0);
    TEST_CHECK(std::all_of(rgch.begin() + 4, rgch.end(), [](char ch) { return ch == 'q'; }));
    app.lpui->hwndFrame = hwndSav;
    TEST_CHECK(ui::DestroyWindow(hwnd));
    StarsCompatProcessExit();
}

TEST_LIST = {{"new-game blank title fits its field and preserves the complete basename", TestNewGameTitleBounds},
             {"text fitting preserves exact glyph limits and UTF-8 field tails", TestTextFitViews},
             {"ship clone suffix handles empty and short UTF-8 names within its field", TestShipCloneNameBounds},
             {"raw player message paints its full UTF-8 byte budget within the display buffer", TestRawMessageDisplay},
             {"mineral captions and tutorial preset keep complete text and field widths", TestMineralAndTutorialText},
             {"planet title retains complete UTF-8 names beyond its former local buffer", TestPlanetTitleCapacity}, {"library-name dialog preserves accepted text and cancelled storage", TestLibraryNameDialog},
             {"native path captions and UTF-8 password fields", TestTextBoundaries},
             {NULL, NULL}};
