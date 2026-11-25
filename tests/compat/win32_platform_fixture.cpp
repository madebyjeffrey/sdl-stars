/* Use real kernel handles only in this SDK translation unit. */
#include <windows.h>
#include <winioctl.h>
#include <io.h>
#include <stdint.h>

int FTestNativeCursor(int32_t *lpx, int32_t *lpy) {
    POINT pt;

    if (!GetCursorPos(&pt)) return 0;
    *lpx = pt.x; *lpy = pt.y;
    return 1;
}

int FTestSparseFile(const char *pszPath) {
    wchar_t rgchPath[32768];
    HANDLE h;
    LARGE_INTEGER li;
    DWORD cb;
    int fRet;

    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pszPath, -1, rgchPath, 32768)) return 0;
    h = CreateFileW(rgchPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return 0;
    li.QuadPart = ((LONGLONG)1 << 32) + 7;
    fRet = DeviceIoControl(h, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &cb, NULL) &&
           SetFilePointerEx(h, li, NULL, FILE_BEGIN) && SetEndOfFile(h);
    return CloseHandle(h) && fRet;
}
