#include "acutest.h"
#include "stars_test.h"
#include <stars_compat.h>

static void TestTextBoundaries(void) {
    HWND hwnd, hwndEdit;
    WNDCLASSA wc = {};
    char sz[32];
    wc.lpfnWndProc = ui::DefWindowProc; wc.lpszClassName = "StarsNativeTitleProbe";
    wc.hInstance = win32::GetModuleHandle(NULL); TEST_ASSERT(win32::RegisterClassA(&wc));
    hwnd = ui::CreateWindowEx(0, wc.lpszClassName, "Native title", WS_OVERLAPPED, 0, 0, 200, 100, NULL, NULL, win32::GetModuleHandle(NULL), NULL);
    TEST_ASSERT(hwnd);
    hwndEdit = ui::CreateWindowEx(0, "EDIT", "", WS_CHILD, 0, 0, 180, 24, hwnd, NULL, win32::GetModuleHandle(NULL), NULL);
    TEST_ASSERT(hwndEdit && FSetUtf8WindowText(hwndEdit, "\xc3\xa9\xe2\x82\xac"));
    TEST_CHECK(CchGetUtf8WindowText(hwndEdit, sz, sizeof(sz)) == 5 && strcmp(sz, "\xc3\xa9\xe2\x82\xac") == 0);
    TEST_ASSERT(FSetNativeWindowTitle(hwnd, "caf\xc3\xa9-\xce\xa9"));
    TEST_CHECK(win32::GetWindowText(hwnd, sz, sizeof(sz)) == 8 && strcmp(sz, "caf\xc3\xa9-\xce\xa9") == 0);
    TEST_CHECK(ui::DestroyWindow(hwnd));
    StarsCompatProcessExit();
}
TEST_LIST = {{"native path captions and UTF-8 password fields", TestTextBoundaries},
             {NULL, NULL}};
