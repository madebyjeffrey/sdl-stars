/* Opt-in desktop qualification: real system pickers, automatically cancelled. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "filedialog_windows_bridge.h"

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
typedef struct NATIVEPICKERCHECK {
    HWND hwndOwner;
    int cDialogs, cPumps, fBad;
} NATIVEPICKERCHECK;

static BOOL CALLBACK FCancelNativePicker(HWND hwnd, LPARAM l) {
    NATIVEPICKERCHECK *lp = (NATIVEPICKERCHECK *)l;
    wchar_t wszClass[64];

    if (GetWindow(hwnd, GW_OWNER) != lp->hwndOwner || !IsWindowVisible(hwnd)) return TRUE;
    GetClassNameW(hwnd, wszClass, 64);
    if (wcscmp(wszClass, L"#32770") != 0) return TRUE;
    if (IsWindowEnabled(lp->hwndOwner)) lp->fBad = 1;
    lp->cDialogs++;
    PostMessageW(hwnd, WM_COMMAND, IDCANCEL, 0);
    return FALSE;
}

static int FPumpNativeCheck(void *lpv) {
    NATIVEPICKERCHECK *lp = lpv;

    lp->cPumps++;
    if (lp->cDialogs == 0) EnumThreadWindows(GetCurrentThreadId(), FCancelNativePicker, (LPARAM)lp);
    return lp->cPumps < 250;
}

int main(void) {
    WINDOWSFILEFILTER rg[] = {{"Game \xc3\xa9", "*.m?;*.hst"}, {"All", "*.*"}};
    NATIVEPICKERCHECK check = {0};
    wchar_t wszCwd[32768], wszAfter[32768];
    char szFolder[32768], szPath[32768];
    int fSave, iFilter;

    CHECK(GetCurrentDirectoryW(32768, wszCwd));
    CHECK(WideCharToMultiByte(CP_UTF8, 0, wszCwd, -1, szFolder, sizeof(szFolder), NULL, NULL));
    check.hwndOwner = CreateWindowExW(0, L"STATIC", L"Stars native picker check owner", WS_OVERLAPPED,
                                      0, 0, 400, 300, NULL, NULL, NULL, NULL);
    CHECK(check.hwndOwner);
    for (fSave = 0; fSave <= 1; fSave++) {
        iFilter = 0; check.cDialogs = 0; check.cPumps = 0; strcpy(szPath, "untouched");
        CHECK(IChooseWindowsFile(check.hwndOwner, fSave, "Stars native picker check \xc3\xa9", szFolder,
                                 "\xce\xa9.m1", rg, 2, &iFilter, szPath, sizeof(szPath), FPumpNativeCheck, &check) == 0);
        CHECK(check.cDialogs == 1 && check.cPumps > 0 && !check.fBad);
        CHECK(IsWindowEnabled(check.hwndOwner) && strcmp(szPath, "untouched") == 0 && iFilter == 0);
        CHECK(GetCurrentDirectoryW(32768, wszAfter) && wcscmp(wszCwd, wszAfter) == 0);
    }
    CHECK(DestroyWindow(check.hwndOwner));
    puts("Real Windows open/save pickers created, owned, pumped and cancelled successfully");
    return 0;
}
