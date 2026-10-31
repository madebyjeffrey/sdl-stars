#include <cstdio>
#include <cstring>
#include "preferencesui.h"
#include "profile_text_io.h"
#include "text_io.h"
#include "win.h"
static const char *transport[4], *production[5];
static char        savedTransport[4][80], savedProduction[5][80];
static int         writes, checks;
static int16_t     GetString(StringId id, char *out) {
    strcpy(out, id == idsDefault ? "Default" : "ZipOrders");
    return (int16_t)strlen(out);
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
static void reset(void) {
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
    gd.fChgZipOrd = gd.fChgZipProd = 0;
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
    unsigned      w;
    char          encoded[7] = "abaaaa", szIniFile[16] = "stars.ini";
    PROFILETEXTIO io;
    io.pfnReadString = ReadString;
    io.pfnWriteString = WriteString;
    PROFILETEXTIOSCOPE profileScope(io);
    TEXTIO             text;
    text.pfnCchGetString = GetString;
    TEXTIOSCOPE textScope(text);
    CHECK(sizeof(vrgZip) == 96 && sizeof(vrgZipProd) == 200);
    reset();
    ReadIniPresets(szIniFile);
    CHECK(vrgZipProd[0].fValid && !vrgZipProd[1].fValid && !vrgZip[0].fValid && strcmp(vrgZipProd[0].szName, "Default") == 0);
    reset();
    transport[0] = "pbadaaaaaaaaaaaaaaaaName";
    ReadIniPresets(szIniFile);
    CHECK(vrgZip[0].fValid && vrgZip[0].txp.rgia[0].iAction() == 15 && vrgZip[0].txp.rgia[0].cQuan() == 769 && strcmp(vrgZip[0].szName, "Name") == 0);
    gd.fChgZipOrd = 1;
    WriteIniPresets(szIniFile);
    CHECK(writes == 4 && strcmp(savedTransport[0], transport[0]) == 0 && savedTransport[1][0] == 0);
    reset();
    transport[0] = "qaaaaaaaaaaaaaaaaaaa";
    transport[1] = "aaaaaaaaaaaaaaaaaaa";
    transport[2] = "aaaaaaaaaaaaaaaaaaaa1234567890123";
    ReadIniPresets(szIniFile);
    CHECK(!vrgZip[0].fValid && !vrgZip[1].fValid && !vrgZip[2].fValid);
    reset();
    production[0] = "pbbaaaN";
    production[1] = "abppppx";
    production[2] = "aax";
    production[3] = "aa";
    production[4] = "anx";
    ReadIniPresets(szIniFile);
    CHECK(vrgZipProd[0].fValid && vrgZipProd[0].fNoResearch && vrgZipProd[0].cpq == 1 && vrgZipProd[0].rgpq[0].w == 1 &&
          strcmp(vrgZipProd[0].szName, "Default") == 0);
    CHECK(vrgZipProd[1].fValid && vrgZipProd[1].rgpq[0].w == 64 && !vrgZipProd[1].fNoResearch);
    CHECK(vrgZipProd[2].fValid && vrgZipProd[2].cpq == 0 && strcmp(vrgZipProd[2].szName, "x") == 0);
    CHECK(!vrgZipProd[3].fValid && !vrgZipProd[4].fValid);
    gd.fChgZipProd = 1;
    WriteIniPresets(szIniFile);
    CHECK(writes == 5 && strcmp(savedProduction[0], "bbbaaaDefault") == 0 && strcmp(savedProduction[1], "abaeaax") == 0 && savedProduction[3][0] == 0);
    reset();
    production[0] = "!";
    ReadIniPresets(szIniFile);
    CHECK(vrgZipProd[0].fValid && vrgZipProd[0].cpq == 0 && !vrgZipProd[0].fNoResearch);
    WriteIniPresets(szIniFile);
    CHECK(writes == 0);
    vrgZipProd[1].fValid = 1;
    gd.fChgZipProd = 1;
    WriteIniPresets(szIniFile);
    CHECK(strcmp(savedProduction[1], "aa") == 0);
    production[1] = savedProduction[1];
    ReadIniPresets(szIniFile);
    CHECK(!vrgZipProd[1].fValid);
    // Exhaust every packed PRODQ1 word through the actual reader and writer.
    reset();
    production[1] = encoded;
    for (w = 0; w < 65536; w++) {
        encoded[2] = (char)('a' + (w & 15));
        encoded[3] = (char)('a' + ((w >> 4) & 15));
        encoded[4] = (char)('a' + ((w >> 8) & 15));
        encoded[5] = (char)('a' + ((w >> 12) & 15));
        ReadIniPresets(szIniFile);
        CHECK(vrgZipProd[1].fValid && vrgZipProd[1].rgpq[0].mdIdle() == ((w & 63) >= 7 ? 0 : (w & 63)) &&
              vrgZipProd[1].rgpq[0].cQuan() == ((w >> 6) > 1020 ? 1 : (w >> 6)));
        gd.fChgZipProd = 1;
        WriteIniPresets(szIniFile);
        CHECK(savedProduction[1][2] == 'a' + (vrgZipProd[1].rgpq[0].w & 15) && savedProduction[1][3] == 'a' + ((vrgZipProd[1].rgpq[0].w >> 4) & 15) &&
              savedProduction[1][4] == 'a' + ((vrgZipProd[1].rgpq[0].w >> 8) & 15) && savedProduction[1][5] == 'a' + ((vrgZipProd[1].rgpq[0].w >> 12) & 15));
    }
    printf("S02 linked preset witnesses: %d assertions passed (including all 65536 packed production words)\n", checks);
    return 0;
}
