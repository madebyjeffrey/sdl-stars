static constexpr int cQueryTraceCapacity = 128;
#include "parts_fixture.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include <limits>

static int     cLookups, rgRequests[17][6];
static int16_t QueryLookup(APPSESSION &app, PART *ppart) {
    CHECK(cLookups < 17);
    int      at = cLookups;
    uint16_t mask = ppart->hs.grhst;
    unsigned item = ppart->hs.iItem;
    rgRequests[at][0] = app.state.idPlayer;
    rgRequests[at][1] = mask;
    rgRequests[at][2] = item;
    rgRequests[at][3] = at == 0 ? -1 : ppart->hs.cItem;
    rgRequests[at][5] = cCalls;
    int16_t result = FLookupPart(app, ppart);
    rgRequests[at][4] = result;
    CHECK(ppart->hs.grhst == mask && ppart->hs.iItem == item);
    if (at != 0)
        CHECK(ppart->hs.cItem == rgRequests[at][3]);
    if (result != mdPartAvailInvalid) {
        unsigned kind = 16;
        for (unsigned k = 0; k < 16; k++)
            if (mask == (std::uint32_t{1} << k))
                kind = k;
        CHECK(kind < 16);
        CHECK(ppart->pcom() == LpcomRow(kind, item));
    }
    cLookups++;
    return result;
}

int main(int argc, char **argv) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    const unsigned rgcRows[] = {iengineCount, iscannerCount,   ishieldCount, iarmorCount,    ibeamCount,     itorpCount,  ibombCount,         iminingCount,
                                iminesCount,  ispecialSBCount, isbhullCount, ispecialECount, ispecialMCount, iterraCount, ihuldefOrbitalFort, iplanetaryCount};
    PARTQUERYFIXTURE fixture;
    PARTQUERYIO      io = PartQueryIO();
    io.pfnLookup = QueryLookup;
    PARTQUERYIOSCOPE lookupScope(io);
    assert(argc == 3 && std::numeric_limits<char>::min() == -128);
    FILE *file = fopen(argv[2], argv[1][0] == 'e' ? "wb" : "rb");
    assert(file);
    if (argv[1][0] == 'e') {
        for (unsigned k = 0; k < 16; k++) {
            for (unsigned item = 0; item < rgcRows[k]; item++) {
                int32_t              rgData[9];
                NATIVEPARTCOMMONVIEW p = LpcomRow(k, item);
                rgData[0] = k;
                rgData[1] = item;
                for (int f = 0; f < 6; f++)
                    rgData[2 + f] = p.rgTech()[f];
                rgData[8] = k == 15 ? rgplanetary[item].grAbility : 0;
                assert(fwrite(rgData, sizeof(rgData), 1, file) == 1);
            }
        }
        fclose(file);
        return 0;
    }
    int32_t rgIn[298];
    SHDEF   design, designBefore;
    PLAYER  rgBefore[16];
    GAME    gameBefore;
    while (fread(rgIn, sizeof(rgIn), 1, file) == 1) {
        memset(app.state.rgplr, 0x53, sizeof(app.state.rgplr));
        memset(&app.state.game, 0x35, sizeof(app.state.game));
        memset(&design, 0x5a, sizeof(design));
        int player = rgIn[1] == -1 ? 0 : rgIn[1];
        CHECK(player >= 0 && player < 16);
        app.state.rgplr[player].rgAttr[rsMajorAdv] = (int8_t)rgIn[4];
        app.state.rgplr[player].grbitAttr = static_cast<RaceTraitBits>((uint32_t)rgIn[5]);
        app.state.rgplr[player].grbitTrader = static_cast<GrbitTrader>((uint16_t)rgIn[6]);
        app.state.rgplr[player].SetITechNow(rgIn[7]);
        for (int f = 0; f < 6; f++)
            app.state.rgplr[player].rgTech[f] = (int8_t)rgIn[10 + f];
        design.hul.ihuldef = static_cast<HulDef>((uint16_t)rgIn[2]);
        design.hul.chs = (uint8_t)rgIn[3];
        design.fFree = rgIn[8];
        design.fGift = rgIn[9];
        for (int at = 0; at < 16; at++) {
            design.hul.rghs[at].grhst = static_cast<HullSlotType>((uint16_t)rgIn[16 + 3 * at]);
            design.hul.rghs[at].iItem = rgIn[17 + 3 * at];
            design.hul.rghs[at].cItem = rgIn[18 + 3 * at];
        }
        app.state.idPlayer = (int16_t)rgIn[0];
        memcpy(rgBefore, app.state.rgplr, sizeof(rgBefore));
        memcpy(&gameBefore, &app.state.game, sizeof(app.state.game));
        memcpy(&designBefore, &design, sizeof(design));
        cCalls = cLookups = 0;
        memset(rgCalls, -1, sizeof(rgCalls));
        memset(rgRequests, -1, sizeof(rgRequests));
        int result = FCanBuildShdef(app, &design, (int16_t)rgIn[1]);
        if (result != rgIn[64]) {
            fprintf(stderr, "case %lu hull %d count %d selected %d: result %d expected %d\n", cCases, rgIn[2], rgIn[3], rgIn[1], result, rgIn[64]);
            abort();
        }
        cChecks++;
        CHECK(app.state.idPlayer == rgIn[65]);
        CHECK(cLookups == rgIn[66]);
        CHECK(cCalls == rgIn[67]);
        CHECK(memcmp(&design, &designBefore, sizeof(design)) == 0);
        CHECK(memcmp(app.state.rgplr, rgBefore, sizeof(app.state.rgplr)) == 0);
        CHECK(memcmp(&app.state.game, &gameBefore, sizeof(app.state.game)) == 0);
        for (int at = 0; at < 17; at++)
            for (int f = 0; f < 6; f++)
                CHECK(rgRequests[at][f] == rgIn[68 + 6 * at + f]);
        for (int at = 0; at < 128; at++)
            CHECK(rgCalls[at] == rgIn[170 + at]);
        cCases++;
    }
    CHECK(feof(file));
    fclose(file);
    printf("Native design buildability: %lu cases, %lu assertions passed; actual lookup/race/technology/catalog composition, order, restoration and "
           "SHDEF/PLAYER/GAME frames checked\n",
           cCases, cChecks);
    return 0;
}
