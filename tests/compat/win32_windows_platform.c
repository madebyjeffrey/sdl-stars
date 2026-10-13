/* Native Windows files must preserve the wrapper's UTF-8 and POSIX contracts. */
#include "internal.h"
#include <io.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
int FTestSparseFile(int fd);

int main(void) {
    const char *pszDir = "platform-\xc3\xa9-\xce\xa9";
    char szPath[PATH_MAX], szOther[PATH_MAX], szCwd[PATH_MAX], sz[32];
    const char rgb[] = {'A', '\r', '\n', 0x1a, 'Z'};
    char rgbRead[sizeof(rgb)];
    char *pszReal, *pszUri;
    HFILE hf, hfOther;
    OFSTRUCT of;
    DWORD dwHigh;
    int fd;
    DIR *lpdir;
    struct dirent *lpentry;
    BOOL fFound = FALSE;

    CHECK(mkdir(pszDir, 0700) == 0 || errno == EEXIST);
    CHECK(getcwd(szCwd, sizeof(szCwd)));
    CHECK(snprintf(szPath, sizeof(szPath), "%s/%s/\xe6\x9d\xb1\xe4\xba\xac.bin", szCwd, pszDir) < (int)sizeof(szPath));
    CHECK(snprintf(szOther, sizeof(szOther), "%s/%s/other.bin", szCwd, pszDir) < (int)sizeof(szOther));
    hf = OpenFile(szPath, &of, OF_CREATE | OF_SHARE_EXCLUSIVE);
    CHECK(hf != HFILE_ERROR && _lwrite(hf, rgb, sizeof(rgb)) == sizeof(rgb));
    hfOther = OpenFile(szOther, &of, OF_CREATE | OF_SHARE_EXCLUSIVE);
    CHECK(hfOther != HFILE_ERROR && _lclose(hfOther) == 0);
    CHECK(OpenFile(szPath, &of, OF_READ | OF_SHARE_DENY_NONE) == HFILE_ERROR && of.nErrCode == ERROR_SHARING_VIOLATION);
    CHECK(_llseek(hf, 0, SEEK_SET) == 0 && _lread(hf, rgbRead, sizeof(rgbRead)) == sizeof(rgbRead));
    CHECK(memcmp(rgb, rgbRead, sizeof(rgb)) == 0 && _lclose(hf) == 0);
    CHECK(rename(szPath, szOther) == 0 && access(szPath, F_OK) != 0);
    pszReal = realpath(szOther, NULL);
    CHECK(pszReal && strcmp(pszReal, szOther) == 0); free(pszReal);
    pszUri = PszCompatFileUri(szOther);
    CHECK(pszUri && strncmp(pszUri, "file:///", 8) == 0 && strstr(pszUri, "%C3%A9-%CE%A9"));
    free(pszUri);
    lpdir = opendir(pszDir); CHECK(lpdir);
    while ((lpentry = readdir(lpdir))) if (strcmp(lpentry->d_name, "other.bin") == 0) fFound = TRUE;
    CHECK(closedir(lpdir) == 0 && fFound);

    // Sparse sizing tests 64-bit metadata without writing gigabytes of data.
    fd = open(szOther, O_RDWR); CHECK(fd >= 0);
    CHECK(FTestSparseFile(fd) && close(fd) == 0);
    hf = OpenFile(szOther, &of, OF_READ | OF_SHARE_DENY_NONE); CHECK(hf != HFILE_ERROR);
    CHECK(GetFileSize((HANDLE)(INT_PTR)hf, &dwHigh) == 7 && dwHigh == 1 && _lclose(hf) == 0);
    CHECK(unlink(szOther) == 0);
    CHECK(SDL_setenv_unsafe("STARS_SETTINGS_DIR", pszDir, 1) == 0);
    CHECK(WritePrivateProfileStringA("Probe", "Value", "first", "Stars.ini"));
    CHECK(WritePrivateProfileStringA("Probe", "Value", "second", "Stars.ini"));
    CHECK(GetPrivateProfileStringA("Probe", "Value", "", sz, sizeof(sz), "Stars.ini") == 6 && strcmp(sz, "second") == 0);
    puts("Windows UTF-8 paths, file identity, binary I/O, 64-bit sizes and settings replacement passed");
    return 0;
}
