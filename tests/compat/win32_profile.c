/* File-backed profile contracts shared with the Windows SDK. */
#include <windows.h>
#include <direct.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include "filesystem.h"
#include "platform.h"
#endif
#ifndef _WIN32
#include <stars_compat.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

#ifndef _WIN32
static int TestProfilePaths(void) {
    char szTemp[] = "/tmp/stars-settings-XXXXXX", szPath[256], szDir[256];
    char sz[32];
    FILE *fp;
    static const unsigned char rgbBad[] = { 0xff, 0xfe, '[', 0, 'x', 0 };

    CHECK(mkdtemp(szTemp));
    snprintf(szDir, sizeof(szDir), "%s/override", szTemp);
    CHECK(setenv("STARS_SETTINGS_DIR", szDir, 1) == 0);
    CHECK(WritePrivateProfileStringA("settings", "key", "override", "Stars.ini"));
    snprintf(szPath, sizeof(szPath), "%s/override/Stars.ini", szTemp);
    CHECK(access(szPath, F_OK) == 0 && GetPrivateProfileStringA("settings", "key", "", sz, sizeof(sz), "Stars.ini") == 8);
    CHECK(!strcmp(sz, "override"));
    fp = fopen(szPath, "wb");
    CHECK(fp && fwrite(rgbBad, 1, sizeof(rgbBad), fp) == sizeof(rgbBad) && fclose(fp) == 0);
    CHECK(GetPrivateProfileStringA("settings", "key", "fallback", sz, sizeof(sz), "Stars.ini") == 8 && GetLastError() == ERROR_BAD_FORMAT);
    CHECK(!WritePrivateProfileStringA("settings", "key", "new", "Stars.ini") && GetLastError() == ERROR_BAD_FORMAT);
    CHECK(remove(szPath) == 0 && rmdir(szDir) == 0);
    CHECK(unsetenv("STARS_SETTINGS_DIR") == 0);
    snprintf(szDir, sizeof(szDir), "%s/xdg", szTemp);
    CHECK(setenv("XDG_CONFIG_HOME", szDir, 1) == 0);
    CHECK(WritePrivateProfileStringA("settings", "key", "xdg", "Stars.ini"));
    snprintf(szPath, sizeof(szPath), "%s/xdg/stars/Stars.ini", szTemp);
    CHECK(access(szPath, F_OK) == 0 && remove(szPath) == 0);
    snprintf(szPath, sizeof(szPath), "%s/xdg/stars", szTemp);
    CHECK(rmdir(szPath) == 0 && rmdir(szDir) == 0);
    CHECK(setenv("XDG_CONFIG_HOME", "relative-is-invalid", 1) == 0 && setenv("HOME", szTemp, 1) == 0);
    CHECK(WritePrivateProfileStringA("settings", "key", "home", "Stars.ini"));
    snprintf(szPath, sizeof(szPath), "%s/.config/stars/Stars.ini", szTemp);
    CHECK(access(szPath, F_OK) == 0 && remove(szPath) == 0);
    snprintf(szPath, sizeof(szPath), "%s/.config/stars", szTemp); CHECK(rmdir(szPath) == 0);
    snprintf(szPath, sizeof(szPath), "%s/.config", szTemp); CHECK(rmdir(szPath) == 0 && rmdir(szTemp) == 0);
    return 0;
}
#endif

#if !defined(_WIN32) || defined(STARS_UI_SDL)
static int TestUtf8Profile(void) {
    char szTemp[4096], szPath[8192], sz[32];
    NATIVEFILE *fp;
    CHECK(FFilesystemTempDir(szTemp, sizeof(szTemp)));
    CHECK(snprintf(szPath, sizeof(szPath), "%s/profile-\xce\xa9\xc3\xa9\xf0\x9f\x98\x80.ini", szTemp) < (int)sizeof(szPath));
    CHECK(WritePrivateProfileStringA("Text", "Name", "\xce\xa9\xc3\xa9\xf0\x9f\x98\x80", szPath));
    CHECK(GetPrivateProfileStringA("Text", "Name", "", sz, sizeof(sz), szPath) == 8);
    CHECK(!strcmp(sz, "\xce\xa9\xc3\xa9\xf0\x9f\x98\x80"));
    CHECK(GetPrivateProfileStringA("Text", "Name", "", sz, 4, szPath) == 2 && !strcmp(sz, "\xce\xa9"));
    CHECK(!WritePrivateProfileStringA("Text", "Name", "\xe9", szPath));
    CHECK(WritePrivateProfileStringA("Names", "\xce\xa9\xc3\xa9", "value", szPath));
    CHECK(GetPrivateProfileStringA("Names", NULL, "", sz, 5, szPath) == 3);
    CHECK(!strcmp(sz, "\xce\xa9") && sz[2] == 0 && sz[3] == 0 && sz[4] == 0);
    CHECK(WritePrivateProfileStringA("Letters", "A", "value", szPath));
    CHECK(WritePrivateProfileStringA("Letters", "B", "value", szPath));
    CHECK(GetPrivateProfileStringA("Letters", NULL, "", sz, 3, szPath) == 1);
    CHECK(!strcmp(sz, "A") && sz[1] == 0 && sz[2] == 0);
    fp = PfNativeOpen(szPath, "rb"); CHECK(fp);
    CHECK(CReadNative(sz, 1, 3, fp) == 3 && !memcmp(sz, "\xef\xbb\xbf", 3));
    CHECK(ICloseNativeFile(fp) == 0 && FFilesystemRemove(szPath));
    CHECK(FFilesystemRemove(szTemp));
    return 0;
}
#endif

int main(void) {
    char szDir[1024], szPath[1200], sz[256], szRead[2048];
    const char *psz;
    FILE *fp;
    DWORD cch;
    size_t cb;
    int i;
    static const char szFixture[] =
        "; preserve this comment\r\n[Misc]\r\n Logging = 1 \r\n"
        "Blank=\r\nQuoted=\"  spaced value  \"\r\nSingle='cargo'\r\n"
        "Number=-1\r\nHex=0x2a\r\nDecimal=010\r\nPartial=12junk\r\nInvalid=no\r\n"
        "Large=4294967296\r\n;another comment\r\nmalformed line\r\n"
        "[Paths]\r\nGame=./games/a file.m1\r\nKeep=unchanged\r\n";
    static const char szKeys[] = "Logging\0Blank\0Quoted\0Single\0Number\0Hex\0Decimal\0Partial\0Invalid\0Large\0";

    CHECK(_getcwd(szDir, sizeof(szDir)));
    snprintf(szPath, sizeof(szPath), "%s/stars profile-%lu.ini", szDir, (unsigned long)GetCurrentThreadId());
    fp = fopen(szPath, "wb");
    CHECK(fp && fwrite(szFixture, 1, sizeof(szFixture) - 1, fp) == sizeof(szFixture) - 1 && fclose(fp) == 0);
    CHECK(GetPrivateProfileStringA("misc", "logging", "default", sz, sizeof(sz), szPath) == 1 && !strcmp(sz, "1"));
    CHECK(GetPrivateProfileStringA("MISC", "QUOTED", "default", sz, sizeof(sz), szPath) == 16 && !strcmp(sz, "  spaced value  "));
    CHECK(GetPrivateProfileStringA("misc", "single", "default", sz, sizeof(sz), szPath) == 5 && !strcmp(sz, "cargo"));
    CHECK(GetPrivateProfileStringA("misc", "blank", "default", sz, sizeof(sz), szPath) == 0 && !*sz);
    CHECK(GetPrivateProfileStringA("missing", "key", "default   ", sz, sizeof(sz), szPath) == 7 && !strcmp(sz, "default"));
    CHECK(GetPrivateProfileStringA("misc", "missing", NULL, sz, sizeof(sz), szPath) == 0 && !*sz);
    CHECK(GetPrivateProfileIntA("misc", "number", 9, szPath) == UINT32_MAX);
    CHECK(GetPrivateProfileIntA("misc", "logging", 9, szPath) == 1);
    CHECK(GetPrivateProfileIntA("misc", "hex", 9, szPath) == 42);
    CHECK(GetPrivateProfileIntA("misc", "decimal", 9, szPath) == 10);
    CHECK(GetPrivateProfileIntA("misc", "partial", 9, szPath) == 12);
    CHECK(GetPrivateProfileIntA("misc", "invalid", 9, szPath) == 0);
    CHECK(GetPrivateProfileIntA("misc", "large", 9, szPath) == 0);
    CHECK(GetPrivateProfileIntA("misc", "blank", 9, szPath) == 9);
    CHECK(GetPrivateProfileIntA("missing", "key", -1, szPath) == UINT32_MAX);
    CHECK(GetPrivateProfileStringA("misc", "quoted", "", sz, 5, szPath) == 4 && !strcmp(sz, "  sp"));
    sz[0] = 'X'; CHECK(GetPrivateProfileStringA("misc", "quoted", "", sz, 0, szPath) == 0 && sz[0] == 'X');
    CHECK(GetPrivateProfileStringA("misc", "quoted", "", sz, 1, szPath) == 0 && sz[0] == 0);
    memset(sz, 'X', sizeof(sz));
    CHECK(GetPrivateProfileStringA(NULL, NULL, "", sz, sizeof(sz), szPath) == 11);
    CHECK(!memcmp(sz, "Misc\0Paths\0\0", 12));
    cch = GetPrivateProfileStringA("misc", NULL, "", sz, sizeof(sz), szPath);
    CHECK(cch == sizeof(szKeys) - 1 && !memcmp(sz, szKeys, sizeof(szKeys)));
    CHECK(GetPrivateProfileStringA("missing", NULL, "", sz, sizeof(sz), szPath) == 0 && sz[0] == 0);
    for (i = 2; i < 12; i++) {
        memset(sz, 'X', sizeof(sz));
        cch = GetPrivateProfileStringA(NULL, NULL, "", sz, (DWORD)i, szPath);
        CHECK(cch == (DWORD)i - 2 && sz[0] != 'X' && sz[i] == 'X');
#ifndef _WIN32
        CHECK(sz[i - 2] == 0 && sz[i - 1] == 0);
#endif
    }
    CHECK(GetPrivateProfileStringA(NULL, NULL, "", sz, 12, szPath) == 10 && !memcmp(sz, "Misc\0Paths\0", 11));
    CHECK(WritePrivateProfileStringA("MISC", "Logging", "0", szPath));
    CHECK(GetPrivateProfileIntA("misc", "logging", 9, szPath) == 0);
    CHECK(WritePrivateProfileStringA("Misc", "Added", "new value", szPath));
    CHECK(GetPrivateProfileStringA("misc", "added", "", sz, sizeof(sz), szPath) == 9 && !strcmp(sz, "new value"));
    CHECK(WritePrivateProfileStringA("Misc", "Integer", "\"  +10  \"", szPath));
    CHECK(GetPrivateProfileIntA("misc", "integer", 9, szPath) == 10);
    CHECK(WritePrivateProfileStringA("Other", "Path", "./new game.m1", szPath));
    CHECK(GetPrivateProfileStringA("other", "path", "", sz, sizeof(sz), szPath) == 13 && !strcmp(sz, "./new game.m1"));
    CHECK(WritePrivateProfileStringA("misc", "added", NULL, szPath));
    CHECK(GetPrivateProfileStringA("misc", "added", "gone", sz, sizeof(sz), szPath) == 4 && !strcmp(sz, "gone"));
    CHECK(WritePrivateProfileStringA("other", NULL, NULL, szPath));
    CHECK(GetPrivateProfileStringA("other", "path", "gone", sz, sizeof(sz), szPath) == 4 && !strcmp(sz, "gone"));
    CHECK(WritePrivateProfileStringA("missing", "key", NULL, szPath));
    CHECK(!WritePrivateProfileStringA(NULL, NULL, NULL, szPath));
    fp = fopen(szPath, "rb"); CHECK(fp);
    cb = fread(szRead, 1, sizeof(szRead) - 1, fp); szRead[cb] = 0; CHECK(fclose(fp) == 0);
    psz = strstr(szRead, "[Paths]");
    CHECK(psz && strstr(psz, "Game=./games/a file.m1") && strstr(psz, "Keep=unchanged"));
#ifndef _WIN32
    CHECK(strstr(szRead, "; preserve this comment\r\n") && strstr(szRead, "malformed line\r\n"));
    CHECK(!WritePrivateProfileStringA("misc", "injected\nkey", "value", szPath));
    CHECK(!WritePrivateProfileStringA("misc", "key", "injected\nvalue", szPath));
    CHECK(!WritePrivateProfileStringA("misc", "key", "value", "./no-such-stars-directory/profile.ini"));
    CHECK(!WritePrivateProfileStringA("misc", "key", "value", NULL));
    StarsCompatShutdown();
#endif
    CHECK(remove(szPath) == 0);
    CHECK(GetPrivateProfileStringA("misc", "key", "fallback", sz, sizeof(sz), szPath) == 8 && !strcmp(sz, "fallback"));
#ifndef _WIN32
    CHECK(TestProfilePaths() == 0);
#endif
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(TestUtf8Profile() == 0);
#endif
    puts("Profiles: PASS (defaults, integers, quotes, enumeration, limits, update/delete, paths)");
    return 0;
}
