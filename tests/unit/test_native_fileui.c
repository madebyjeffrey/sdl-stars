#include "acutest.h"
#include "stars_test.h"
#include <stars_compat.h>

static void TestTextBoundaries(void) {
    HWND hwnd, hwndEdit;
    WNDCLASSA wc = {0};
    char sz[32];
    wc.lpfnWndProc = DefWindowProcA; wc.lpszClassName = "StarsNativeTitleProbe";
    wc.hInstance = GetModuleHandle(NULL); TEST_ASSERT(RegisterClassA(&wc));
    hwnd = CreateWindow(wc.lpszClassName, "Native title", WS_OVERLAPPED, 0, 0, 200, 100, NULL, NULL, GetModuleHandle(NULL), NULL);
    TEST_ASSERT(hwnd);
    hwndEdit = CreateWindow("EDIT", "", WS_CHILD, 0, 0, 180, 24, hwnd, NULL, GetModuleHandle(NULL), NULL);
    TEST_ASSERT(hwndEdit && FSetLegacyWindowText(hwndEdit, "\xe9\x80"));
    TEST_CHECK(CchGetLegacyWindowText(hwndEdit, sz, sizeof(sz)) == 2 && strcmp(sz, "\xe9\x80") == 0);
    TEST_ASSERT(FSetNativeWindowTitle(hwnd, "caf\xc3\xa9-\xce\xa9"));
    TEST_CHECK(GetWindowText(hwnd, sz, sizeof(sz)) == 8 && strcmp(sz, "caf\xc3\xa9-\xce\xa9") == 0);
    TEST_CHECK(DestroyWindow(hwnd));
    StarsCompatProcessExit();
}
TEST_LIST = {{"native path captions and legacy password fields", TestTextBoundaries},
             {NULL, NULL}};
