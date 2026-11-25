#include <cstdio>
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include <cstring>
#include <string>
#include "preferencesui.h"
#include "profile_text_io.h"
#include "text_io.h"
#include "win.h"
static const char *transport[4], *production[5];
static char        savedTransport[4][80], savedProduction[5][80];
static int         writes, checks;
static std::int16_t GetString(APPSESSION &, StringId id, std::span<char> out) {
    return static_cast<std::int16_t>(CchCopyUtf8(out, id == idsDefault ? "Default" : "ZipOrders"));
}
static DWORD ReadString(LPCSTR section, LPCSTR key, LPCSTR fallback, LPSTR out, DWORD cap, LPCSTR file) {
    const char *s = key[9] == 'P' ? production[key[10] - '1'] : transport[key[9] - '1'];
    size_t      n = strlen(s);
    (void)section;
    (void)fallback;
    (void)file;
    if (n >= cap)
        n = cap - 1;
    memcpy(out, s, n);
    out[n] = 0;
    return (DWORD)n;
}
static BOOL WriteString(LPCSTR section, LPCSTR key, LPCSTR value, LPCSTR file) {
    (void)section;
    (void)file;
    writes++;
    strcpy(key[9] == 'P' ? savedProduction[key[10] - '1'] : savedTransport[key[9] - '1'], value);
    return 0; // Source ignores even every write failing.
}
static void reset(APPSESSION &app) {
    int i;
    for (i = 0; i < 4; i++) {
        transport[i] = "";
        savedTransport[i][0] = 0;
    }
    for (i = 0; i < 5; i++) {
        production[i] = "";
        savedProduction[i][0] = 0;
    }
    writes = 0;
    app.state.gd.fChgZipOrd = app.state.gd.fChgZipProd = 0;
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

    unsigned      w;
    char          encoded[7] = "abaaaa", szIniFile[16] = "stars.ini";
    PROFILETEXTIO io;
    io.pfnReadString = ReadString;
    io.pfnWriteString = WriteString;
    PROFILETEXTIOSCOPE profileScope(io);
    TEXTIO             text;
    text.pfnCchGetString = GetString;
    TEXTIOSCOPE textScope(text);
    CHECK(sizeof(app.state.vrgZip) == 96 && sizeof(app.state.vrgZipProd) == 200);
    reset(app);
    ReadIniPresets(app, szIniFile);
    CHECK(app.state.vrgZipProd[0].fValid && !app.state.vrgZipProd[1].fValid && !app.state.vrgZip[0].fValid && strcmp(app.state.vrgZipProd[0].szName, "Default") == 0);
    reset(app);
    transport[0] = "pbadaaaaaaaaaaaaaaaaName";
    ReadIniPresets(app, szIniFile);
    CHECK(app.state.vrgZip[0].fValid && app.state.vrgZip[0].txp.rgia[0].iAction() == 15 && app.state.vrgZip[0].txp.rgia[0].cQuan() == 769 && strcmp(app.state.vrgZip[0].szName, "Name") == 0);
    app.state.gd.fChgZipOrd = 1;
    WriteIniPresets(app, szIniFile);
    CHECK(writes == 4 && strcmp(savedTransport[0], transport[0]) == 0 && savedTransport[1][0] == 0);
    reset(app);
    transport[0] = "qaaaaaaaaaaaaaaaaaaa";
    transport[1] = "aaaaaaaaaaaaaaaaaaa";
    transport[2] = "aaaaaaaaaaaaaaaaaaaa1234567890123";
    ReadIniPresets(app, szIniFile);
    CHECK(!app.state.vrgZip[0].fValid && !app.state.vrgZip[1].fValid && !app.state.vrgZip[2].fValid);
    reset(app);
    production[0] = "pbbaaaN";
    production[1] = "abppppx";
    production[2] = "aax";
    production[3] = "aa";
    production[4] = "anx";
    ReadIniPresets(app, szIniFile);
    CHECK(app.state.vrgZipProd[0].fValid && app.state.vrgZipProd[0].fNoResearch && app.state.vrgZipProd[0].cpq == 1 && app.state.vrgZipProd[0].rgpq[0].w == 1 &&
          strcmp(app.state.vrgZipProd[0].szName, "Default") == 0);
    CHECK(app.state.vrgZipProd[1].fValid && app.state.vrgZipProd[1].rgpq[0].w == 64 && !app.state.vrgZipProd[1].fNoResearch);
    CHECK(app.state.vrgZipProd[2].fValid && app.state.vrgZipProd[2].cpq == 0 && strcmp(app.state.vrgZipProd[2].szName, "x") == 0);
    CHECK(!app.state.vrgZipProd[3].fValid && !app.state.vrgZipProd[4].fValid);
    app.state.gd.fChgZipProd = 1;
    WriteIniPresets(app, szIniFile);
    CHECK(writes == 5 && strcmp(savedProduction[0], "bbbaaaDefault") == 0 && strcmp(savedProduction[1], "abaeaax") == 0 && savedProduction[3][0] == 0);
    reset(app);
    production[0] = "!";
    ReadIniPresets(app, szIniFile);
    CHECK(app.state.vrgZipProd[0].fValid && app.state.vrgZipProd[0].cpq == 0 && !app.state.vrgZipProd[0].fNoResearch);
    WriteIniPresets(app, szIniFile);
    CHECK(writes == 0);
    app.state.vrgZipProd[1].fValid = 1;
    app.state.gd.fChgZipProd = 1;
    WriteIniPresets(app, szIniFile);
    CHECK(strcmp(savedProduction[1], "aa") == 0);
    production[1] = savedProduction[1];
    ReadIniPresets(app, szIniFile);
    CHECK(!app.state.vrgZipProd[1].fValid);
    // Both preset formats retain a full twelve-byte UTF-8 name after their
    // encoded order prefix, including all twelve production orders.
    reset(app);
    const std::string szName = "\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9\xc3\xa9";
    const std::string szTransport = std::string(20, 'a') + szName;
    const std::string szProduction = std::string("am") + std::string(48, 'a') + szName;
    transport[0] = szTransport.c_str();
    production[1] = szProduction.c_str();
    ReadIniPresets(app, szIniFile);
    CHECK(app.state.vrgZip[0].fValid && strcmp(app.state.vrgZip[0].szName, szName.c_str()) == 0);
    CHECK(app.state.vrgZipProd[1].fValid && app.state.vrgZipProd[1].cpq == 12 && strcmp(app.state.vrgZipProd[1].szName, szName.c_str()) == 0);
    app.state.gd.fChgZipOrd = app.state.gd.fChgZipProd = 1;
    WriteIniPresets(app, szIniFile);
    CHECK(writes == 9 && savedTransport[0] == szTransport && savedProduction[1] == szProduction);
    // Exhaust every packed PRODQ1 word through the actual reader and writer.
    reset(app);
    production[1] = encoded;
    for (w = 0; w < 65536; w++) {
        encoded[2] = (char)('a' + (w & 15));
        encoded[3] = (char)('a' + ((w >> 4) & 15));
        encoded[4] = (char)('a' + ((w >> 8) & 15));
        encoded[5] = (char)('a' + ((w >> 12) & 15));
        ReadIniPresets(app, szIniFile);
        CHECK(app.state.vrgZipProd[1].fValid && app.state.vrgZipProd[1].rgpq[0].mdIdle() == ((w & 63) >= 7 ? 0 : (w & 63)) &&
              app.state.vrgZipProd[1].rgpq[0].cQuan() == ((w >> 6) > 1020 ? 1 : (w >> 6)));
        app.state.gd.fChgZipProd = 1;
        WriteIniPresets(app, szIniFile);
        CHECK(savedProduction[1][2] == 'a' + (app.state.vrgZipProd[1].rgpq[0].w & 15) && savedProduction[1][3] == 'a' + ((app.state.vrgZipProd[1].rgpq[0].w >> 4) & 15) &&
              savedProduction[1][4] == 'a' + ((app.state.vrgZipProd[1].rgpq[0].w >> 8) & 15) && savedProduction[1][5] == 'a' + ((app.state.vrgZipProd[1].rgpq[0].w >> 12) & 15));
    }
    printf("S02 linked preset witnesses: %d assertions passed (including all 65536 packed production words)\n", checks);
    return 0;
}
