#include <cstdio>
#include <cstring>
#include "library_path_io.h"
#include "preferencesui.h"
#include "profile_text_io.h"
#include "text_io.h"
#include "win.h"
static char       *memory[9], disk[9][256], *pszRecentBacking;
static const char *inputs[9], *startupInput;
static int         attempts, failAt, checks;
static int16_t     under[9];
static int16_t     GetString(StringId id, char *out) {
    strcpy(out, id == idsFile1 ? "File1" : id == idsFiles ? "Files" : "stars.ini");
    return (int16_t)strlen(out);
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
static void reset() {
    static const char *names[] = {"aaaa", "bbbb", "cccc", "dddd", "eeee", "ffff", "gggg", "hhhh", "iiii"};
    for (int i = 0; i < 9; i++) {
        strcpy(memory[i], names[i]);
        strcpy(disk[i], names[i]);
        under[i] = 0;
        inputs[i] = "";
    }
    attempts = failAt = 0;
    startupInput = nullptr;
    vrgszMRU = pszRecentBacking;
    ini.grobjSel = 2;
    ini.idPlayer = 7;
    ini.iObjSel = 90;
}
static void selection(const char *psz) { ReadIniSelection(psz, (int16_t)strlen(psz)); }
static void identity(const char *psz) { ReadIniIdentity(psz, (int16_t)strlen(psz)); }
static void recent() {
    const char *first = startupInput ? startupInput : inputs[0];
    char        szSection[16] = "Files", szEntry[16] = "File1", szIniFile[16] = "stars.ini";
    strcpy(szWork, first);
    ReadIniRecent(szSection, szEntry, szIniFile, (int16_t)strlen(first));
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
    int k;
    pszRecentBacking = LpAlloc<char>(2304, htPerm);
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
    reset();
    selection("N");
    CHECK(ini.grobjSel == 0 && ini.idPlayer == 7 && ini.iObjSel == 90);
    reset();
    selection("NQx");
    CHECK(ini.grobjSel == 0 && ini.idPlayer == 15 && ini.iObjSel == 90);
    reset();
    selection("PA3");
    CHECK(ini.grobjSel == 0 && ini.idPlayer == 7 && ini.iObjSel == 90);
    reset();
    selection("SB-1x");
    CHECK(ini.grobjSel == 2 && ini.idPlayer == 0 && ini.iObjSel == -1);
    reset();
    selection("PBx");
    CHECK(ini.grobjSel == 1 && ini.idPlayer == 0 && ini.iObjSel == 0);
    reset();
    selection("PB65536");
    CHECK(ini.grobjSel == 1 && ini.iObjSel == 0);
    identity("1A");
    CHECK(ini.lid == 16);
    identity("1a");
    CHECK(ini.lid == 26);
    identity("7fffffff");
    CHECK(ini.lid == 2147483647);
    reset();
    strcpy(szBase, "prior");
    inputs[0] = ".";
    inputs[1] = "bbbb";
    recent();
    CHECK(!ini.fStartupFile && strcmp(szBase, "prior") == 0 && strcmp(memory[0], "bbbb") == 0 && memory[1][0] == 0);
    reset();
    startupInput = "aaaa";
    inputs[0] = ".";
    inputs[1] = "bbbb";
    recent();
    CHECK(ini.fStartupFile && strcmp(szBase, "aaaa") == 0 && strcmp(memory[0], "bbbb") == 0);
    reset();
    inputs[0] = "aaaa";
    inputs[2] = "cc";
    inputs[8] = "iiii";
    recent();
    CHECK(ini.fStartupFile && strcmp(szBase, "aaaa") == 0 && strcmp(memory[1], "iiii") == 0 && memory[8][0] == 0);
    reset();
    strcpy(memory[0], "a.m1");
    AddMRUFile(const_cast<char *>("A"), const_cast<char *>("M1"));
    CHECK(attempts == 0 && strcmp(memory[0], "a.m1") == 0);
    reset();
    strcpy(memory[2], "c.m1");
    AddMRUFile(const_cast<char *>("c"), const_cast<char *>("m1"));
    CHECK(attempts == 9 && strcmp(memory[0], "c.m1") == 0 && strcmp(memory[2], "bbbb") == 0 && strcmp(memory[8], "iiii") == 0);
    reset();
    failAt = 2;
    AddMRUFile(const_cast<char *>("j"), const_cast<char *>("m1"));
    CHECK(attempts == 9 && strcmp(memory[0], "j.m1") == 0 && strcmp(memory[8], "hhhh") == 0 && strcmp(disk[1], "bbbb") == 0 && strcmp(disk[2], "bbbb") == 0);
    reset();
    vrgszMRU = NULL;
    AddMRUFile(const_cast<char *>("j"), const_cast<char *>("m1"));
    CHECK(attempts == 0);
    reset();
    under[0] = 1;
    CHECK(FDeleteLibraryGameRecentPaths("unused"));
    CHECK(attempts == 10 && memory[0][0] == 0 && strcmp(memory[1], "bbbb") == 0 && disk[0][0] == 0);
    reset();
    under[2] = 1;
    CHECK(FDeleteLibraryGameRecentPaths("unused"));
    CHECK(attempts == 9 && strcmp(memory[2], "dddd") == 0 && strcmp(memory[7], "iiii") == 0 && memory[8][0] == 0);
    for (k = 1; k <= 10; k++) {
        reset();
        under[0] = 1;
        failAt = k;
        CHECK(!FDeleteLibraryGameRecentPaths("unused") && attempts == k && memory[0][0] == 0);
        CHECK(k == 1 ? strcmp(disk[0], "aaaa") == 0 : disk[0][0] == 0);
    }
    reset();
    under[2] = 1;
    failAt = 3;
    CHECK(!FDeleteLibraryGameRecentPaths("unused") && attempts == 3 && strcmp(memory[2], "dddd") == 0 && strcmp(disk[2], "cccc") == 0 &&
          strcmp(memory[3], "dddd") == 0);
    reset();
    vrgszMRU = NULL;
    CHECK(FDeleteLibraryGameRecentPaths("unused") && attempts == 0);
    printf("S02 linked state witnesses: %d passed\n", checks);
    vrgszMRU = nullptr;
    FreeLp(pszRecentBacking, htPerm);
    return 0;
}
