#include "acutest.h"
#include "stars_test.h"
#include <stars_compat.h>
#include <SDL3/SDL_stdinc.h>
#include "appstate.h"
#include "uistate.h"
#include "callbackcontext.h"
#include "control_io.h"
#include "librarypublication.h"
#include "planetui.h"
#include "text_io.h"
#include <algorithm>
#include <array>
#include <string_view>
#include <string>

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

TEST_LIST = {{"planet title retains complete UTF-8 names beyond its former local buffer", TestPlanetTitleCapacity}, {"library-name dialog preserves accepted text and cancelled storage", TestLibraryNameDialog},
             {"native path captions and UTF-8 password fields", TestTextBoundaries},
             {NULL, NULL}};
