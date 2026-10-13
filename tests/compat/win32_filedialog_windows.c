/* Exercise the real SDK bridge without opening an interactive system dialog. */
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <string.h>
#include "filedialog_windows_bridge.h"

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
static int fSaveExpected, iAction, cCalls, fBad;
static DWORD dwError;

static BOOL FakeChooser(OPENFILENAMEW *lpof, int fSave) {
    const wchar_t *pwsz;

    cCalls++;
    if (fSave != fSaveExpected || lpof->hwndOwner != (HWND)(UINT_PTR)123 ||
        wcscmp(lpof->lpstrTitle, L"Open \x00e9") != 0 ||
        wcscmp(lpof->lpstrInitialDir, L"C:/\x03a9") != 0 ||
        wcscmp(lpof->lpstrFile, L"\x03a9.m1") != 0 || lpof->nFilterIndex != 2 ||
        !(lpof->Flags & OFN_NOCHANGEDIR) || (lpof->Flags & OFN_OVERWRITEPROMPT)) fBad = 1;
    pwsz = lpof->lpstrFilter;
    if (wcscmp(pwsz, L"Game \x00e9") != 0) fBad = 1;
    pwsz += wcslen(pwsz) + 1;
    if (wcscmp(pwsz, L"*.m?;*.hst") != 0) fBad = 1;
    pwsz += wcslen(pwsz) + 1;
    if (wcscmp(pwsz, L"All") != 0) fBad = 1;
    pwsz += wcslen(pwsz) + 1;
    if (wcscmp(pwsz, L"*.*") != 0 || pwsz[wcslen(pwsz) + 1] != 0) fBad = 1;
    /* Even a cancelled native dialog must not change the process directory. */
    SetCurrentDirectoryW(L"C:/");
    dwError = iAction == 2 ? CDERR_INITIALIZATION : 0;
    if (iAction != 0) return FALSE;
    wcscpy(lpof->lpstrFile, L"C:\\games\\\x03a9.m1");
    lpof->nFilterIndex = 1;
    return TRUE;
}
static BOOL WINAPI FakeOpen(OPENFILENAMEW *lpof) { return FakeChooser(lpof, 0); }
static BOOL WINAPI FakeSave(OPENFILENAMEW *lpof) { return FakeChooser(lpof, 1); }
static DWORD WINAPI FakeError(void) { return dwError; }
BOOL (WINAPI *__imp_GetOpenFileNameW)(OPENFILENAMEW *) = FakeOpen;
BOOL (WINAPI *__imp_GetSaveFileNameW)(OPENFILENAMEW *) = FakeSave;
DWORD (WINAPI *__imp_CommDlgExtendedError)(void) = FakeError;

int main(void) {
    WINDOWSFILEFILTER rg[] = {{"Game \xe9", "*.m?;*.hst"}, {"All", "*.*"}};
    wchar_t wszCwd[32768], wszAfter[32768];
    char szPath[128];
    int fSave, i, iFilter, iRet;

    CHECK(GetCurrentDirectoryW(32768, wszCwd));
    for (fSave = 0; fSave <= 1; fSave++) for (i = 0; i < 3; i++) {
        fSaveExpected = fSave; iAction = i; iFilter = 1; strcpy(szPath, "untouched");
        iRet = IChooseWindowsFile((void *)(UINT_PTR)123, fSave, "Open \xe9", "C:/\xce\xa9",
                                 "\xce\xa9.m1", rg, 2, &iFilter, szPath, sizeof(szPath), NULL, NULL);
        CHECK(iRet == (i == 0 ? 1 : i == 1 ? 0 : -1));
        CHECK(!fBad);
        CHECK(GetCurrentDirectoryW(32768, wszAfter) && wcscmp(wszCwd, wszAfter) == 0);
        CHECK(i == 0 ? strcmp(szPath, "C:/games/\xce\xa9.m1") == 0 && iFilter == 0 : strcmp(szPath, "untouched") == 0 && iFilter == 1);
    }
    iAction = 0; fSaveExpected = 0; iFilter = 1; strcpy(szPath, "untouched");
    CHECK(IChooseWindowsFile((void *)(UINT_PTR)123, 0, "Open \xe9", "C:/\xce\xa9", "\xce\xa9.m1",
                             rg, 2, &iFilter, szPath, 2, NULL, NULL) == -1);
    CHECK(strcmp(szPath, "untouched") == 0 && iFilter == 1);
    CHECK(cCalls == 7 && !fBad);
    puts("Windows native chooser contracts passed");
    return 0;
}
