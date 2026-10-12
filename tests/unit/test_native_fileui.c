#include "acutest.h"
#include "stars_test.h"
#ifdef STARS_UI_SDL
#include <stars_compat.h>
#endif

static void TestTextBoundaries(void) {
    HWND hwnd, hwndEdit;
    WNDCLASSA wc = {0};
    char sz[32];
#if defined(_WIN32) && !defined(STARS_UI_SDL)
    WCHAR wsz[32];
#endif
    wc.lpfnWndProc = DefWindowProcA; wc.lpszClassName = "StarsNativeTitleProbe";
    wc.hInstance = GetModuleHandle(NULL); TEST_ASSERT(RegisterClassA(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "Native title", WS_OVERLAPPED, 0, 0, 200, 100, NULL, NULL, GetModuleHandle(NULL), NULL);
    TEST_ASSERT(hwnd);
    hwndEdit = CreateWindow("EDIT", "", WS_CHILD, 0, 0, 180, 24, hwnd, NULL, GetModuleHandle(NULL), NULL);
    TEST_ASSERT(hwndEdit && FSetLegacyWindowText(hwndEdit, "\xe9\x80"));
    TEST_CHECK(CchGetLegacyWindowText(hwndEdit, sz, sizeof(sz)) == 2 && strcmp(sz, "\xe9\x80") == 0);
    TEST_ASSERT(FSetNativeWindowTitle(hwnd, "caf\xc3\xa9-\xce\xa9"));
#if defined(_WIN32) && !defined(STARS_UI_SDL)
    TEST_CHECK(GetWindowTextW(hwnd, wsz, 32) == 6 && wcscmp(wsz, L"caf\x00e9-\x03a9") == 0);
    TEST_ASSERT(SetWindowTextW(hwndEdit, L"\x6771\x4eac"));
    TEST_CHECK(CchGetLegacyWindowText(hwndEdit, sz, sizeof(sz)) == -1 && sz[0] == 0);
#else
    TEST_CHECK(GetWindowText(hwnd, sz, sizeof(sz)) == 8 && strcmp(sz, "caf\xc3\xa9-\xce\xa9") == 0);
#endif
    TEST_CHECK(DestroyWindow(hwnd));
#ifdef STARS_UI_SDL
    StarsCompatProcessExit();
#endif
}
#if defined(_WIN32) && !defined(STARS_UI_SDL)
static int iChooser;
static LPWSTR WINAPI FakeCommandLine(void) {
    static WCHAR wsz[] = L"\"C:\\Program Files\\Stars.exe\" -p\x00e9 file-\x03a9.hst";
    return wsz;
}
LPWSTR (WINAPI *__imp_GetCommandLineW)(void) = FakeCommandLine;
static void TestWideArguments(void) {
    int c;
    char *psz = PszNativeCommandLine();
    char **rgpsz = RgpszNativeArguments(&c);
    TEST_ASSERT(psz && rgpsz && c == 3);
    TEST_CHECK(strcmp(psz, "-p\xc3\xa9 file-\xce\xa9.hst") == 0);
    TEST_CHECK(strcmp(rgpsz[0], "C:\\Program Files\\Stars.exe") == 0);
    TEST_CHECK(strcmp(rgpsz[1], "-p\xc3\xa9") == 0 && strcmp(rgpsz[2], "file-\xce\xa9.hst") == 0);
    free(psz); free(rgpsz);
}
static BOOL WINAPI FakeChoose(LPOPENFILENAMEW lpof) {
    static const WCHAR wszFile[] = L"C:\\caf\x00e9\\\x6771\x4eac-\x03a9.m1";
    TEST_CHECK(lpof->lStructSize == sizeof(*lpof));
    TEST_CHECK(wcscmp(lpof->lpstrInitialDir, L"C:\\caf\x00e9") == 0);
    TEST_CHECK(wcscmp(lpof->lpstrTitle, L"\x00e9") == 0);
    TEST_CHECK(wcscmp(lpof->lpstrFilter, L"Stars") == 0 && wcscmp(lpof->lpstrFilter + 6, L"*.m1") == 0);
    if (iChooser == 1) return FALSE;
    wcscpy(lpof->lpstrFile, wszFile); lpof->nFileOffset = 8; lpof->nFileExtension = 13; lpof->nFilterIndex = 2;
    return TRUE;
}
static DWORD WINAPI FakeError(void) { return 0; }
BOOL (WINAPI *__imp_GetOpenFileNameW)(LPOPENFILENAMEW) = FakeChoose;
BOOL (WINAPI *__imp_GetSaveFileNameW)(LPOPENFILENAMEW) = FakeChoose;
DWORD (WINAPI *__imp_CommDlgExtendedError)(void) = FakeError;
static void TestWidePicker(void) {
    OPENFILENAMEA ofn = {0};
    char szFile[128] = "old.m1", szTitle[128];
    ofn.lStructSize = sizeof(ofn); ofn.lpstrFile = szFile; ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFileTitle = szTitle; ofn.nMaxFileTitle = sizeof(szTitle);
    ofn.lpstrInitialDir = "C:\\caf\xc3\xa9"; ofn.lpstrTitle = "\xe9";
    ofn.lpstrFilter = "Stars\0*.m1\0All\0*.*\0"; ofn.nFilterIndex = 1;
    TEST_ASSERT(FNativeOpenFileName(&ofn));
    TEST_CHECK(strcmp(szFile, "C:\\caf\xc3\xa9\\\xe6\x9d\xb1\xe4\xba\xac-\xce\xa9.m1") == 0);
    TEST_CHECK(ofn.nFileOffset == 9 && ofn.nFileExtension == 19 && ofn.nFilterIndex == 2);
    TEST_CHECK(strcmp(szTitle, "\xe6\x9d\xb1\xe4\xba\xac-\xce\xa9.m1") == 0);
    iChooser = 1; strcpy(szFile, "old.m1");
    TEST_CHECK(!FNativeSaveFileName(&ofn) && strcmp(szFile, "old.m1") == 0 && DwNativeFileDialogError() == 0);
}
#endif
TEST_LIST = {{"native path captions and legacy password fields", TestTextBoundaries},
#if defined(_WIN32) && !defined(STARS_UI_SDL)
             {"wide Windows picker returns UTF-8 offsets", TestWidePicker},
             {"Windows arguments preserve UTF-8", TestWideArguments},
#endif
             {NULL, NULL}};
