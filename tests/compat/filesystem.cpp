/* Exercise the C ABI with Unicode paths, bounded copies and non-recursive removal. */
#include "filesystem.h"
#include <stdio.h>
#include <string.h>

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "filesystem line %d: %s\n", __LINE__, #f); return 1; } } while (0)

int main(void) {
    char szTemp[4096], szOther[4096], szDir[8192], szPath[8192], szCanonical[8192];
    char szSmall[2] = {'X', 'Y'};
    const char *pszName = "caf\xc3\xa9-\xce\xa9\xe6\x9d\xb1\xe4\xba\xac\xf0\x9f\x98\x80";
    int cch;

    CHECK(!FFilesystemTempDir(NULL, 0));
    CHECK(!FFilesystemTempDir(szSmall, 1) && szSmall[0] == 0 && szSmall[1] == 'Y');
    CHECK(FFilesystemTempDir(szTemp, sizeof(szTemp)));
    CHECK(FFilesystemTempDir(szOther, sizeof(szOther)) && strcmp(szTemp, szOther) != 0);
    CHECK(FFilesystemRemove(szOther));
    cch = snprintf(szDir, sizeof(szDir), "%s/%s", szTemp, pszName);
    CHECK(cch > 0 && cch < (int)sizeof(szDir));
    CHECK(FFilesystemCreateDirectory(szDir));
    CHECK(!FFilesystemCreateDirectory(szDir));
    CHECK(!FFilesystemRemove(szTemp)); /* A non-empty directory must survive. */
    cch = snprintf(szPath, sizeof(szPath), "%s/../%s", szDir, pszName);
    CHECK(cch > 0 && cch < (int)sizeof(szPath));
    CHECK(FFilesystemCanonicalPath(szPath, szCanonical, sizeof(szCanonical)));
    CHECK(strstr(szCanonical, pszName) && !strstr(szCanonical, ".."));
    CHECK(FFilesystemCanonicalPath(szDir, szOther, sizeof(szOther)) && !strcmp(szCanonical, szOther));
    CHECK(!FFilesystemCanonicalPath(szDir, szOther, strlen(szCanonical)) && szOther[0] == 0);
    CHECK(FFilesystemCanonicalPath(szDir, szOther, strlen(szCanonical) + 1) && !strcmp(szCanonical, szOther));
    CHECK(!FFilesystemCanonicalPath(szDir, szSmall, 1) && szSmall[0] == 0 && szSmall[1] == 'Y');
    CHECK(!FFilesystemCanonicalPath(szDir, szSmall, 0) && szSmall[1] == 'Y');
    CHECK(!FFilesystemCanonicalPath(NULL, szCanonical, sizeof(szCanonical)) && szCanonical[0] == 0);
    CHECK(!FFilesystemCanonicalPath("bad-\xff", szCanonical, sizeof(szCanonical)) && szCanonical[0] == 0);
    CHECK(!FFilesystemCreateDirectory(NULL) && !FFilesystemCreateDirectory("") && !FFilesystemCreateDirectory("bad-\xff"));
    CHECK(!FFilesystemRemove(NULL) && !FFilesystemRemove("") && !FFilesystemRemove("bad-\xff"));
    CHECK(FFilesystemRemove(szDir));
    CHECK(!FFilesystemCanonicalPath(szDir, szCanonical, sizeof(szCanonical)) && szCanonical[0] == 0);
    CHECK(!FFilesystemRemove(szDir));
    CHECK(FFilesystemRemove(szTemp));
    puts("Filesystem: PASS (C ABI, UTF-8, capacity, canonical paths, non-recursive removal)");
    return 0;
}
