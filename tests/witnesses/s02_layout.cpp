#include <cstdio>
#include <cstring>
#include "profile_text_io.h"
#include "text_io.h"
#include "win.h"
static const char *input;
static int16_t     GetString(StringId, char *out) {
    strcpy(out, "key");
    return 3;
}
static DWORD WINAPI ReadString(LPCSTR, LPCSTR, LPCSTR, LPSTR out, DWORD cap, LPCSTR) {
    size_t n = strlen(input);
    if (n >= cap)
        n = cap - 1;
    memcpy(out, input, n);
    out[n] = 0;
    return (DWORD)n;
}
static int checks;
#define CHECK(x)                                                                                                                                               \
    do {                                                                                                                                                       \
        checks++;                                                                                                                                              \
        if (!(x)) {                                                                                                                                            \
            fprintf(stderr, "witness %d failed: %s\n", checks, #x);                                                                                            \
            return 1;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)

int main(void) {
    TILE                  tiles[3] = {};
    TILE                  original[3];
    WN                    wn = {};
    unsigned              i;
    static const char    *strings[] = {"", "nonnumeric", " ", "-0x10tail", "077", "0x", "4294967296", "+42x", "-1"};
    static const uint32_t expected[] = {7, 0, 0, 4294967280u, 77, 0, 0, 42, 4294967295u};
    for (i = 0; i < 3; i++) {
        tiles[i].yTop = (int16_t)(10 * (i + 1));
        tiles[i].id = i;
    }
    PROFILETEXTIO io;
    io.pfnReadString = ReadString;
    PROFILETEXTIOSCOPE profileScope(io);
    TEXTIO             text;
    text.pfnCchGetString = GetString;
    TEXTIOSCOPE textScope(text);
    memcpy(original, tiles, sizeof(tiles));
    ReadIniTileSettings(const_cast<char *>("C"), tiles, 3);
    CHECK(tiles[0].id == 2 && tiles[1].id == 1 && tiles[2].id == 0);
    CHECK(tiles[0].fPopped && tiles[0].yTop == 30 && tiles[2].yTop == 10);
    memcpy(tiles, original, sizeof(tiles));
    ReadIniTileSettings(const_cast<char *>("Cc"), tiles, 3);
    CHECK(tiles[0].id == 2 && tiles[0].fPopped && tiles[1].id == 1 && tiles[2].id == 0);
    memcpy(tiles, original, sizeof(tiles));
    ReadIniTileSettings(const_cast<char *>("*?*b"), tiles, 3);
    CHECK(tiles[0].id == 1 && tiles[1].id == 0 && tiles[2].id == 2);
    CHECK(tiles[0].iCol == 1 && tiles[1].iCol == 1 && tiles[2].iCol == 1);
    memcpy(tiles, original, sizeof(tiles));
    tiles[2].iCol = 3;
    ReadIniTileSettings(const_cast<char *>("*"), tiles, 3);
    CHECK(tiles[2].iCol == 3);
    input = "X";
    GetIniWinRc(const_cast<char *>("Windows"), const_cast<char *>("unused"), idsMain, &wn);
    CHECK(wn.rc.left == -32768 && wn.rc.right == -32768 && wn.rc.top == 0 && wn.rc.bottom == 0);
    CHECK(wn.fMaximized && !wn.fMinimized && !wn.fInitalized);
    input = "R1-2-000000010002";
    GetIniWinRc(const_cast<char *>("Windows"), const_cast<char *>("unused"), static_cast<StringId>(0), &wn);
    CHECK(wn.rc.left == -12 && wn.rc.top == 0 && wn.rc.right == 1 && wn.rc.bottom == 2);
    CHECK(wn.fInitalized && !wn.fMaximized && !wn.fMinimized);
    input = "M0000000010000001";
    GetIniWinRc(const_cast<char *>("Windows"), const_cast<char *>("unused"), static_cast<StringId>(0), &wn);
    CHECK(wn.fMaximized && wn.fInitalized);
    input = "R0000000010000001x";
    GetIniWinRc(const_cast<char *>("Windows"), const_cast<char *>("unused"), static_cast<StringId>(0), &wn);
    CHECK(!wn.fInitalized && !wn.fMaximized);
    for (i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        input = strings[i];
        CHECK(win32::GetPrivateProfileIntA("a", "b", 7, "unused") == expected[i]);
    }
    printf("S02 linked C witnesses: %d passed\n", checks);
    return 0;
}
