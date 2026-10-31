/* Owned file handles, access and sharing contracts shared with the SDK. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
#endif
#ifdef _WIN32
#ifdef STARS_UI_SDL
#include "fileio.h"
static BOOL FFixtureTruncate(const char *psz, int64_t cb) {
    NATIVEFILE *pf = PfNativeOpenFlags(psz, 3, OF_SHARE_DENY_NONE, 0, 0, 0, 1);
    BOOL fOkay;
    if (!pf) return FALSE;
    fOkay = ITruncateNativeFile(pf, cb) == 0;
    return ICloseNativeFile(pf) == 0 && fOkay;
}
#endif
#else
#include <sys/stat.h>
#include <unistd.h>
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

int main(void) {
    char szPath[80], rgb[32];
    OFSTRUCT of, ofOther, ofReopen;
    HFILE hf, hfOther;
    DWORD dwHigh;
#if defined(_WIN32) && !defined(STARS_UI_SDL)
    LARGE_INTEGER li;
#endif

    snprintf(szPath, sizeof(szPath), "stars-file-%lu.tmp", (unsigned long)GetCurrentThreadId());
    memset(&of, 0, sizeof(of));
    CHECK(OpenFile(szPath, &of, OF_READ) == HFILE_ERROR && of.nErrCode == ERROR_FILE_NOT_FOUND);
    hf = OpenFile(szPath, &of, OF_CREATE | OF_SHARE_DENY_NONE);
    CHECK(hf != HFILE_ERROR && of.cBytes == sizeof(of) && of.nErrCode == 0 && *of.szPathName);
    ofReopen = of;
    CHECK(_lwrite(hf, "abcdef", 6) == 6);
    dwHigh = 123;
    CHECK(GetFileSize((HANDLE)(INT_PTR)hf, &dwHigh) == 6 && dwHigh == 0);
    CHECK(_llseek(hf, 0, 0) == 0 && _lread(hf, rgb, 2) == 2 && !memcmp(rgb, "ab", 2));
    CHECK(_llseek(hf, -1, 1) == 1 && _lread(hf, rgb, 3) == 3 && !memcmp(rgb, "bcd", 3));
    CHECK(_llseek(hf, -2, 2) == 4 && _lread(hf, rgb, 8) == 2 && !memcmp(rgb, "ef", 2));
    CHECK(_lread(hf, rgb, 8) == 0);
    CHECK(_llseek(hf, 3, 0) == 3 && _lwrite(hf, "", 0) == 0);
    CHECK(GetFileSize((HANDLE)(INT_PTR)hf, NULL) == 3);
    CHECK(_llseek(hf, 8, 0) == 8 && _lwrite(hf, "", 0) == 0);
    CHECK(GetFileSize((HANDLE)(INT_PTR)hf, NULL) == 8);
    CHECK(_llseek(hf, 3, 0) == 3 && _lread(hf, rgb, 5) == 5);
    CHECK(rgb[0] == 0 && rgb[1] == 0 && rgb[2] == 0 && rgb[3] == 0 && rgb[4] == 0);
    CHECK(_llseek(hf, -1, 0) == -1 && _llseek(hf, 0, 4) == -1);
    CHECK(_lclose(hf) == 0 && _lclose(hf) == HFILE_ERROR);
    CHECK(GetFileSize((HANDLE)(INT_PTR)hf, NULL) == INVALID_FILE_SIZE && GetLastError() == ERROR_INVALID_HANDLE);
    hf = OpenFile(NULL, &ofReopen, OF_REOPEN | OF_READ | OF_SHARE_DENY_WRITE);
    CHECK(hf != HFILE_ERROR && _lread(hf, rgb, 3) == 3 && !memcmp(rgb, "abc", 3));
    CHECK(_lwrite(hf, "Z", 1) == (UINT)HFILE_ERROR);
    hfOther = OpenFile(szPath, &ofOther, OF_READ | OF_SHARE_DENY_NONE);
    CHECK(hfOther != HFILE_ERROR);
    CHECK(OpenFile(szPath, &ofOther, OF_WRITE | OF_SHARE_DENY_NONE) == HFILE_ERROR && ofOther.nErrCode == ERROR_SHARING_VIOLATION);
    CHECK(OpenFile(szPath, &ofOther, OF_CREATE | OF_SHARE_DENY_NONE) == HFILE_ERROR);
    CHECK(GetFileSize((HANDLE)(INT_PTR)hf, NULL) == 8); // Failed creation must not truncate.
    CHECK(OpenFile(szPath, &ofOther, OF_DELETE) == HFILE_ERROR);
    CHECK(_lclose(hfOther) == 0 && _lclose(hf) == 0);
    hf = OpenFile(szPath, &of, OF_READWRITE | OF_SHARE_EXCLUSIVE);
    CHECK(hf != HFILE_ERROR && OpenFile(szPath, &ofOther, OF_READ | OF_SHARE_DENY_NONE) == HFILE_ERROR);
    CHECK(_lclose(hf) == 0);
    hf = OpenFile(szPath, &of, OF_READWRITE | OF_SHARE_DENY_NONE);
    CHECK(hf != HFILE_ERROR);
#if defined(_WIN32) && defined(STARS_UI_SDL)
    CHECK(FFixtureTruncate(szPath, ((int64_t)1 << 32) + 7));
#elif defined(_WIN32)
    li.QuadPart = ((int64_t)1 << 32) + 7;
    CHECK(SetFilePointerEx((HANDLE)(INT_PTR)hf, li, NULL, FILE_BEGIN) && SetEndOfFile((HANDLE)(INT_PTR)hf));
#else
    CHECK(truncate(szPath, ((off_t)1 << 32) + 7) == 0);
#endif
    CHECK(GetFileSize((HANDLE)(INT_PTR)hf, &dwHigh) == 7 && dwHigh == 1);
#if defined(_WIN32) && defined(STARS_UI_SDL)
    CHECK(FFixtureTruncate(szPath, UINT32_MAX));
#elif defined(_WIN32)
    li.QuadPart = UINT32_MAX;
    CHECK(SetFilePointerEx((HANDLE)(INT_PTR)hf, li, NULL, FILE_BEGIN) && SetEndOfFile((HANDLE)(INT_PTR)hf));
#else
    CHECK(truncate(szPath, UINT32_MAX) == 0);
#endif
    SetLastError(123);
    CHECK(GetFileSize((HANDLE)(INT_PTR)hf, &dwHigh) == INVALID_FILE_SIZE && dwHigh == 0 && GetLastError() == NO_ERROR);
    CHECK(_lclose(hf) == 0);
    CHECK(OpenFile(szPath, &of, OF_EXIST) != HFILE_ERROR);
    CHECK(OpenFile(szPath, &of, OF_PARSE) != HFILE_ERROR && *of.szPathName);
    CHECK(OpenFile(szPath, &of, OF_DELETE) != HFILE_ERROR);
    CHECK(OpenFile(szPath, &of, OF_EXIST) == HFILE_ERROR && of.nErrCode == ERROR_FILE_NOT_FOUND);
    CHECK(_lread(HFILE_ERROR, rgb, 1) == (UINT)HFILE_ERROR && _lwrite(HFILE_ERROR, "x", 1) == (UINT)HFILE_ERROR);
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(OpenFile(szPath, &of, OF_PROMPT) == HFILE_ERROR && of.nErrCode == ERROR_NOT_SUPPORTED);
    CHECK(_lclose(0) == HFILE_ERROR && GetLastError() == ERROR_INVALID_HANDLE);
    StarsCompatShutdown();
#endif
    puts("Files: PASS (open/reopen, binary I/O, seek, truncate/extend, size, sharing, close/delete)");
    return 0;
}
