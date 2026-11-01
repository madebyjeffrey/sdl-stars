#include <cerrno>
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include <cstdio>
#include <cstring>
#include "filesystem.h"
#include "host_profile.h"
#include "profile_io.h"
#include "text_io.h"
#include "win.h"
static const char *const *fixture;
static int                loadOK = 1, checks;
static char               szDirectory[4096], szFixture[8192];
static int16_t            GetString(APPSESSION &, StringId id, char *out) {
    strcpy(out, id == static_cast<StringId>(1) ? "A" : "k");
    return 1;
}
static NATIVEFILE *OpenFixture(const char *, const char *) {
    NATIVEFILE *pf;
    if (!loadOK) {
        errno = EACCES;
        return nullptr;
    }
    pf = PfNativeOpen(szFixture, "wb");
    if (!pf)
        return nullptr;
    for (unsigned i = 0; fixture[i]; i++)
        if (CWriteNative(fixture[i], 1, strlen(fixture[i]), pf) != strlen(fixture[i])) {
            ICloseNativeFile(pf);
            return nullptr;
        }
    if (ICloseNativeFile(pf) != 0)
        return nullptr;
    return PfNativeOpen(szFixture, "rb");
}
#define CHECK(x)                                                                                                                                               \
    do {                                                                                                                                                       \
        checks++;                                                                                                                                              \
        if (!(x)) {                                                                                                                                            \
            fprintf(stderr, "witness %d failed: %s\n", checks, #x);                                                                                            \
            return 1;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)

int main(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    static const char *const one[] = {"[AB]\n", NULL};
    static const char *const unicode[] = {"[\xc3\xa9]\n", NULL};
    static const char *const duplicate[] = {"[A]\n", "x=first\n", "[A]\n", "k=second\n", NULL};
    static const char *const quoted[] = {"[A]\n", "k=' a '\n", NULL};
    static const char *const empty[] = {NULL};
    char                     out[16], longline[320];
    PROFILEIO                io;
    io.pfnOpen = OpenFixture;
    PROFILEIOSCOPE profileScope(io);
    TEXTIO         text;
    text.pfnCchGetString = GetString;
    TEXTIOSCOPE textScope(text);
    if (!FFilesystemTempDir(szDirectory, sizeof(szDirectory)))
        return 1;
    snprintf(szFixture, sizeof(szFixture), "%s/profile", szDirectory);
    CHECK(CchUtf8Prefix("A\xc3\xa9"
                        "B",
                        2) == 1);
    CHECK(CchUtf8Prefix("A\xc3\xa9"
                        "B",
                        3) == 3);
    CHECK(CchUtf8Prefix("A\xff"
                        "B",
                        8) == 1);
    CHECK(CchUtf8Prefix("A\0B", 8) == 1);
    CHECK(!FUtf8Valid("A\0B", 3));
    fixture = one;
    memset(out, 9, sizeof(out));
    CHECK(win32::GetPrivateProfileStringA(NULL, NULL, "", out, 4, "x") == 2 && !memcmp(out, "AB\0\0", 4));
    CHECK(out[4] == 9);
    fixture = unicode;
    memset(out, 9, sizeof(out));
    CHECK(win32::GetPrivateProfileStringA(NULL, NULL, "", out, 3, "x") == 1 && out[0] == 0 && out[1] == 0 && out[2] == 0 && out[3] == 9);
    fixture = empty;
    memset(out, 9, sizeof(out));
    CHECK(win32::GetPrivateProfileStringA(NULL, NULL, "", out, 3, "x") == 0 && out[0] == 0 && out[1] == 0 && out[2] == 9);
    fixture = quoted;
    memset(out, 9, sizeof(out));
    CHECK(win32::GetPrivateProfileStringA("A", "k", "", out, 3, "x") == 2 && !strcmp(out, " a"));
    fixture = duplicate;
    CHECK(win32::GetPrivateProfileStringA("A", "k", "fallback", out, sizeof(out), "x") == 8 && !strcmp(out, "fallback"));
    CHECK(FIniValue(app, "[A]\nx=first\n[A]\nk=second", static_cast<StringId>(1), static_cast<StringId>(2), out, sizeof(out)) && !strcmp(out, "second"));
    CHECK(!FIniValue(app, " [A]\nk=1", static_cast<StringId>(1), static_cast<StringId>(2), out, sizeof(out)));
    CHECK(!FIniValue(app, "[ A ]\nk=1", static_cast<StringId>(1), static_cast<StringId>(2), out, sizeof(out)));
    CHECK(!FIniValue(app, "[A]\n k=1", static_cast<StringId>(1), static_cast<StringId>(2), out, sizeof(out)));
    CHECK(FIniValue(app, "[A]\n[broken\nk=1", static_cast<StringId>(1), static_cast<StringId>(2), out, sizeof(out)) && !strcmp(out, "1"));
    CHECK(FIniValue(app, "[A]\nk= '2'  ", static_cast<StringId>(1), static_cast<StringId>(2), out, sizeof(out)) && !strcmp(out, "'2'"));
    CHECK(FIniValue(app, "[A]\nk=abcd", static_cast<StringId>(1), static_cast<StringId>(2), out, 3) && !strcmp(out, "ab"));
    CHECK(LIniInt(app, "[A]\nk=0x2", static_cast<StringId>(1), static_cast<StringId>(2), 7) == 0);
    CHECK(LIniInt(app, "[A]\nk='2'", static_cast<StringId>(1), static_cast<StringId>(2), 7) == 0);
    CHECK(LIniInt(app, "[A]\nk=", static_cast<StringId>(1), static_cast<StringId>(2), 7) == 0);
    CHECK(LIniInt(app, "[A]\nx=", static_cast<StringId>(1), static_cast<StringId>(2), 7) == 7);
    CHECK(LIniInt(app, "[A]\nk=-12suffix", static_cast<StringId>(1), static_cast<StringId>(2), 7) == -12);
    strcpy(longline, "[A]\nk=");
    memset(longline + 6, 'a', 270);
    strcpy(longline + 276, "\nk=second");
    CHECK(FIniValue(app, longline, static_cast<StringId>(1), static_cast<StringId>(2), out, 4) && !strcmp(out, "aaa"));
    loadOK = 0;
    CHECK(win32::GetPrivateProfileStringA("A", "k", "fallback", out, sizeof(out), "x") == 8 && !strcmp(out, "fallback"));
    printf("S02 profile I/O and host linked witnesses: %d passed\n", checks);
    if (IUnlinkNativePath(szFixture) != 0 || !FFilesystemRemove(szDirectory))
        return 1;
    return 0;
}
