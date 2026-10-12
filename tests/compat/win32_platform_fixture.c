/* Use real kernel handles only in this SDK translation unit. */
#include <windows.h>
#include <winioctl.h>
#include <io.h>

int FTestSparseFile(int fd) {
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    LARGE_INTEGER li;
    DWORD cb;

    li.QuadPart = ((LONGLONG)1 << 32) + 7;
    return DeviceIoControl(h, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &cb, NULL) &&
           SetFilePointerEx(h, li, NULL, FILE_BEGIN) && SetEndOfFile(h);
}
