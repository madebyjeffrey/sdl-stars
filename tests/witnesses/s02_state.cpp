#include <cstdio>
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include <cstring>
#include <string>
#include <array>
#include "library_path_io.h"
#include "preferencesui.h"
#include "profile_text_io.h"
#include "text_io.h"
#include "win.h"
static char       *memory[9], disk[9][256], *pszRecentBacking;
static const char *inputs[9], *startupInput;
static int         attempts, failAt, checks;
static int16_t     under[9];
static std::int16_t GetString(APPSESSION &, StringId id, std::span<char> out) {
    return static_cast<std::int16_t>(CchCopyUtf8(out, id == idsFile1 ? "File1" : id == idsFiles ? "Files" : "stars.ini"));
}
static DWORD ReadString(LPCSTR, LPCSTR key, LPCSTR, LPSTR out, DWORD cap, LPCSTR) {
    const char *s = inputs[key[4] - '1'];
    size_t      n = strlen(s);
    if (n >= cap)
        n = cap - 1;
    memcpy(out, s, n);
    out[n] = 0;
    return (DWORD)n;
}
static BOOL WriteString(LPCSTR, LPCSTR key, LPCSTR value, LPCSTR) {
    attempts++;
    if (attempts == failAt)
        return 0;
    strcpy(disk[key[4] - '1'], value);
    return 1;
}
static int16_t Under(const char *, const char *path) {
    for (int i = 0; i < 9; i++)
        if (!strcmp(path, memory[i]))
            return under[i];
    return 0;
}
static void reset(APPSESSION &app) {
    static const char *names[] = {"aaaa", "bbbb", "cccc", "dddd", "eeee", "ffff", "gggg", "hhhh", "iiii"};
    for (int i = 0; i < 9; i++) {
        strcpy(memory[i], names[i]);
        strcpy(disk[i], names[i]);
        under[i] = 0;
        inputs[i] = "";
    }
    attempts = failAt = 0;
    startupInput = nullptr;
    app.lpui->vrgszMRU = pszRecentBacking;
    app.state.ini.grobjSel = 2;
    app.state.ini.idPlayer = 7;
    app.state.ini.iObjSel = 90;
}
static void selection(APPSESSION &app, const char *psz) { ReadIniSelection(app, psz, (int16_t)strlen(psz)); }
static void identity(APPSESSION &app, const char *psz) { ReadIniIdentity(app, psz, (int16_t)strlen(psz)); }
static void recent(APPSESSION &app) {
    const char *first = startupInput ? startupInput : inputs[0];
    char        szSection[16] = "Files", szEntry[16] = "File1", szIniFile[16] = "stars.ini";
    strcpy(app.state.szWork, first);
    ReadIniRecent(app, szSection, szEntry, szIniFile, (int16_t)strlen(first));
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

    int k;
    pszRecentBacking = LpAlloc<char>(app, 2304, htPerm);
    for (k = 0; k < 9; k++)
        memory[k] = pszRecentBacking + 256 * k;
    PROFILETEXTIO io;
    io.pfnReadString = ReadString;
    io.pfnWriteString = WriteString;
    PROFILETEXTIOSCOPE profileScope(io);
    TEXTIO             text;
    text.pfnCchGetString = GetString;
    TEXTIOSCOPE   textScope(text);
    LIBRARYPATHIO path;
    path.pfnUnder = Under;
    LIBRARYPATHIOSCOPE pathScope(path);
    reset(app);
    selection(app, "N");
    CHECK(app.state.ini.grobjSel == 0 && app.state.ini.idPlayer == 7 && app.state.ini.iObjSel == 90);
    reset(app);
    selection(app, "NQx");
    CHECK(app.state.ini.grobjSel == 0 && app.state.ini.idPlayer == 15 && app.state.ini.iObjSel == 90);
    reset(app);
    selection(app, "PA3");
    CHECK(app.state.ini.grobjSel == 0 && app.state.ini.idPlayer == 7 && app.state.ini.iObjSel == 90);
    reset(app);
    selection(app, "SB-1x");
    CHECK(app.state.ini.grobjSel == 2 && app.state.ini.idPlayer == 0 && app.state.ini.iObjSel == -1);
    reset(app);
    selection(app, "PBx");
    CHECK(app.state.ini.grobjSel == 1 && app.state.ini.idPlayer == 0 && app.state.ini.iObjSel == 0);
    reset(app);
    selection(app, "PB65536");
    CHECK(app.state.ini.grobjSel == 1 && app.state.ini.iObjSel == 0);
    identity(app, "1A");
    CHECK(app.state.ini.lid == 16);
    identity(app, "1a");
    CHECK(app.state.ini.lid == 26);
    identity(app, "7fffffff");
    CHECK(app.state.ini.lid == 2147483647);
    reset(app);
    strcpy(app.state.szBase, "prior");
    inputs[0] = ".";
    inputs[1] = "bbbb";
    recent(app);
    CHECK(!app.state.ini.fStartupFile && strcmp(app.state.szBase, "prior") == 0 && strcmp(memory[0], "bbbb") == 0 && memory[1][0] == 0);
    reset(app);
    startupInput = "aaaa";
    inputs[0] = ".";
    inputs[1] = "bbbb";
    recent(app);
    CHECK(app.state.ini.fStartupFile && strcmp(app.state.szBase, "aaaa") == 0 && strcmp(memory[0], "bbbb") == 0);
    reset(app);
    inputs[0] = "aaaa";
    inputs[2] = "cc";
    inputs[8] = "iiii";
    recent(app);
    CHECK(app.state.ini.fStartupFile && strcmp(app.state.szBase, "aaaa") == 0 && strcmp(memory[1], "iiii") == 0 && memory[8][0] == 0);
    reset(app);
    const std::string szLong = std::string(253, 'x') + "\xc3\xa9";
    startupInput = szLong.c_str();
    inputs[8] = szLong.c_str();
    recent(app);
    CHECK(app.state.ini.fStartupFile && app.state.szBase == szLong);
    CHECK(memory[0] == szLong && memory[0][255] == 0 && memory[8][0] == 0);
    reset(app);
    struct CASE { std::string szBase, szExt; };
    const CASE rgRejected[] = {
        {"\xff", "m1"}, {"base", "\xff"},
        {std::string(253, 'x'), "m1"}, {std::string(255, 'x'), "m1"},
        {"base", std::string(252, 'x')}, {"base", "\xe2\x82"}
    };
    for (const auto &cs : rgRejected) {
        reset(app);
        std::array<char, 2304> rgchBefore, rgchDiskBefore;
        memcpy(rgchBefore.data(), pszRecentBacking, rgchBefore.size());
        memcpy(rgchDiskBefore.data(), disk, rgchDiskBefore.size());
        AddMRUFile(app, cs.szBase.c_str(), cs.szExt.c_str());
        if (attempts != 0) fprintf(stderr, "Rejected recent path caused %d profile writes\n", attempts);
        CHECK(attempts == 0);
        CHECK(memcmp(rgchBefore.data(), pszRecentBacking, rgchBefore.size()) == 0);
        CHECK(memcmp(rgchDiskBefore.data(), disk, rgchDiskBefore.size()) == 0);
    }
    reset(app);
    const std::string szBase = std::string(250, 'x') + "\xc3\xa9";
    const std::string szRecent = szBase + ".m1";
    AddMRUFile(app, szBase.c_str(), "m1");
    CHECK(attempts == 9 && memory[0] == szRecent && memory[0][255] == 0 && disk[0] == szRecent && strcmp(memory[8], "hhhh") == 0);
    reset(app);
    strcpy(memory[0], szRecent.c_str());
    under[2] = 1;
    CHECK(FDeleteLibraryGameRecentPaths(app, "unused"));
    CHECK(memory[0] == szRecent && disk[0] == szRecent && strcmp(memory[2], "dddd") == 0 && memory[8][0] == 0);
    reset(app);
    strcpy(memory[0], "a.m1");
    AddMRUFile(app, const_cast<char *>("A"), const_cast<char *>("M1"));
    CHECK(attempts == 0 && strcmp(memory[0], "a.m1") == 0);
    reset(app);
    strcpy(memory[2], "c.m1");
    AddMRUFile(app, const_cast<char *>("c"), const_cast<char *>("m1"));
    CHECK(attempts == 9 && strcmp(memory[0], "c.m1") == 0 && strcmp(memory[2], "bbbb") == 0 && strcmp(memory[8], "iiii") == 0);
    reset(app);
    failAt = 2;
    AddMRUFile(app, const_cast<char *>("j"), const_cast<char *>("m1"));
    CHECK(attempts == 9 && strcmp(memory[0], "j.m1") == 0 && strcmp(memory[8], "hhhh") == 0 && strcmp(disk[1], "bbbb") == 0 && strcmp(disk[2], "bbbb") == 0);
    reset(app);
    app.lpui->vrgszMRU = NULL;
    AddMRUFile(app, const_cast<char *>("j"), const_cast<char *>("m1"));
    CHECK(attempts == 0);
    reset(app);
    under[0] = 1;
    CHECK(FDeleteLibraryGameRecentPaths(app, "unused"));
    CHECK(attempts == 10 && memory[0][0] == 0 && strcmp(memory[1], "bbbb") == 0 && disk[0][0] == 0);
    reset(app);
    under[2] = 1;
    CHECK(FDeleteLibraryGameRecentPaths(app, "unused"));
    CHECK(attempts == 9 && strcmp(memory[2], "dddd") == 0 && strcmp(memory[7], "iiii") == 0 && memory[8][0] == 0);
    for (k = 1; k <= 10; k++) {
        reset(app);
        under[0] = 1;
        failAt = k;
        CHECK(!FDeleteLibraryGameRecentPaths(app, "unused") && attempts == k && memory[0][0] == 0);
        CHECK(k == 1 ? strcmp(disk[0], "aaaa") == 0 : disk[0][0] == 0);
    }
    reset(app);
    under[2] = 1;
    failAt = 3;
    CHECK(!FDeleteLibraryGameRecentPaths(app, "unused") && attempts == 3 && strcmp(memory[2], "dddd") == 0 && strcmp(disk[2], "cccc") == 0 &&
          strcmp(memory[3], "dddd") == 0);
    reset(app);
    app.lpui->vrgszMRU = NULL;
    CHECK(FDeleteLibraryGameRecentPaths(app, "unused") && attempts == 0);
    printf("S02 linked state witnesses: %d passed\n", checks);
    app.lpui->vrgszMRU = nullptr;
    FreeLp(app, pszRecentBacking, htPerm);
    return 0;
}
