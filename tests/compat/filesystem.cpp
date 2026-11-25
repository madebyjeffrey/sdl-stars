/* Exercise filesystem value results with Unicode paths and non-recursive removal. */
#include "filesystem.h"
#include <cstdio>
#include <string>
#include <string_view>

#define CHECK(f) do { if (!(f)) { std::fprintf(stderr, "filesystem line %d: %s\n", __LINE__, #f); return 1; } } while (0)

int main() {
    const char *pszName = "caf\xc3\xa9-\xce\xa9\xe6\x9d\xb1\xe4\xba\xac\xf0\x9f\x98\x80";
    const auto szTemp = SzFilesystemTempDir();
    std::string szDir, szPath, szCanonical, szWithSuffix;
    std::optional<std::string> szOther, szViewCanonical;
    std::string_view szPathView;

    CHECK(szTemp);
    CHECK((szOther = SzFilesystemTempDir()) && *szTemp != *szOther);
    CHECK(FFilesystemRemove(*szOther));
    szDir = *szTemp + "/" + pszName;
    CHECK(FFilesystemCreateDirectory(szDir));
    CHECK(!FFilesystemCreateDirectory(szDir));
    CHECK(!FFilesystemRemove(*szTemp)); /* A non-empty directory must survive. */
    szPath = szDir + "/../" + pszName;
    szOther = SzFilesystemCanonicalPath(szPath);
    CHECK(szOther);
    szCanonical = *szOther;
    CHECK(szCanonical.find(pszName) != std::string::npos && szCanonical.find("..") == std::string::npos);
    CHECK((szOther = SzFilesystemCanonicalPath(szDir)) && szCanonical == *szOther);
    CHECK(!SzFilesystemCanonicalPath(szDir + "/missing"));
    szWithSuffix = szPath + "/ignored-suffix";
    szPathView = std::string_view{szWithSuffix.data(), szPath.size()};
    CHECK((szViewCanonical = SzFilesystemCanonicalPath(szPathView)) && *szViewCanonical == szCanonical);
    CHECK(!SzFilesystemCanonicalPath("bad-\xff"));
    CHECK(!SzFilesystemCanonicalPath("") && !FFilesystemCreateDirectory("") && !FFilesystemRemove(""));
    CHECK(!FFilesystemCreateDirectory("bad-\xff") && !FFilesystemRemove("bad-\xff"));
    CHECK(FFilesystemRemove(szDir));
    CHECK(!SzFilesystemCanonicalPath(szDir));
    CHECK(!FFilesystemRemove(szDir));
    CHECK(FFilesystemRemove(*szTemp));
    std::puts("Filesystem: PASS (owned UTF-8 paths, exact views, canonical paths, non-recursive removal)");
    return 0;
}
