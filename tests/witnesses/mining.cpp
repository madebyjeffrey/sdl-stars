#include "colony_io.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include "common.h"
#include "part_query_io.h"
#include "random_io.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include "common.h"
#include "liststorage.h"
static int32_t cfg[61];
static PLANET  planet;
static FLEET   fleets[4];
static FLEET  *fleetPointers[4];
static union {
    max_align_t align;
    uint8_t     bytes[sizeof(PLORD) + sizeof(ORDER)];
} orderStorage[4];
static PLORD *orders[4];
static SHDEF  designs[16];
static MINING mining[8];
static int    draws, mineCalls, partCalls, operCalls;

static int16_t LookupMining(APPSESSION &, PART *part) {
    assert(part->hs.grhst == hstMining && part->hs.iItem <= iminingAlienMiner);
    partCalls++;
    part->SetPmining(&mining[part->hs.iItem]);
    return 1;
}
static int32_t FleetMining(APPSESSION &app, FLEET *fleet) {
    mineCalls++;
    int i = (int)(fleet - fleets);
    assert(0 <= i && i < 4);
    return cfg[29 + 8 * i + 7] ? CMineFromLpfl(app, fleet) : cfg[29 + 8 * i + 5];
}
static int16_t OperableMines(APPSESSION &, PLANET *p, int16_t player, int16_t next) {
    (void)p;
    assert(player == cfg[4] && !next);
    return (int16_t)cfg[2];
}
static int16_t Draw(APPSESSION &app, int16_t bound) {
    assert(bound == 100);
    draws++;
    return cfg[10] ? Random(app, bound) : (int16_t)cfg[11];
}
static int16_t OperatingMines(APPSESSION &app, PLANET *p) {
    operCalls++;
    return CMinesOperating(app, p);
}

static void setup(APPSESSION &app) {
    memset(&planet, 0, sizeof(planet));
    memset(app.state.rgplr, 0, sizeof(app.state.rgplr));
    memset(&app.state.gd, 0, sizeof(app.state.gd));
    memset(designs, 0, sizeof(designs));
    memset(fleets, 0, sizeof(fleets));
    memset(orderStorage, 0, sizeof(orderStorage));
    for (int i = 0; i < 4; i++)
        orders[i] = LpStartPl<PLORD, ORDER>(orderStorage[i].bytes, sizeof(orderStorage[i].bytes));
    draws = mineCalls = partCalls = operCalls = 0;
    app.state.lRandSeed1 = 7;
    app.state.lRandSeed2 = 11;
    planet.iPlayer = (int16_t)cfg[4];
    planet.rgwtMin[3] = cfg[5];
    planet.cMines = 4095;
    planet.fHomeworld = cfg[7];
    app.state.gd.fGeneratingTurn = cfg[9];
    app.state.cFleet = (int16_t)cfg[21];
    for (int i = 0; i < 16; i++) {
        app.state.rgplr[i].rgAttr[rsMajorAdv] = (int8_t)cfg[6];
        app.state.rgplr[i].rgAttr[rsMineProd] = (int8_t)cfg[3];
        app.state.rglpshdef[i] = designs;
        designs[i].hul.chs = (uint8_t)cfg[24];
        for (int j = 0; j < 16; j++) {
            designs[i].hul.rghs[j].grhst = (HullSlotType)cfg[25];
            designs[i].hul.rghs[j].iItem = cfg[26];
            designs[i].hul.rghs[j].cItem = cfg[27];
        }
    }
    for (int i = 0; i < 8; i++)
        mining[i].grAbility = (int16_t)cfg[28];
    for (int i = 0; i < 3; i++) {
        planet.rgwtMin[i] = cfg[12 + i];
        planet.rgMinConc[i] = (uint8_t)cfg[15 + i];
        planet.rgpctMinLevel[i] = (uint8_t)cfg[18 + i];
    }
    for (int i = 0; i < 4; i++) {
        int at = 29 + 8 * i;
        fleets[i].idPlanet = (int16_t)cfg[at];
        fleets[i].iPlayer = (int16_t)cfg[at + 1];
        fleets[i].fDead = cfg[at + 2];
        fleets[i].cord = (int16_t)cfg[at + 3];
        fleets[i].lpplord = cfg[at + 4] < 0 ? NULL : orders[i];
        orders[i]->rgord()[0].grTask = cfg[at + 4] < 0 ? 0 : cfg[at + 4];
        fleets[i].fHereAllTurn = cfg[at + 6];
        for (int j = 0; j < 16; j++)
            fleets[i].rgcsh[j] = (int16_t)((j == 0 || cfg[0] == 2) ? cfg[23] : 0);
        app.state.rglpfl[i] = i == cfg[22] ? NULL : &fleets[i];
    }
}
int main(int argc, char **argv) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    PARTQUERYIO partIO;
    partIO.pfnLookup = LookupMining;
    PARTQUERYIOSCOPE partScope(partIO);
    COLONYIO         colonyIO;
    colonyIO.pfnOperableMines = OperableMines;
    colonyIO.pfnFleetMining = FleetMining;
    colonyIO.pfnMinesOperating = OperatingMines;
    COLONYIOSCOPE colonyScope(colonyIO);
    RANDOMIO      randomIO;
    randomIO.pfnRandom = Draw;
    RANDOMIOSCOPE randomScope(randomIO);
    app.state.rglpfl = fleetPointers;
    assert(argc == 2);
    FILE *f = fopen(argv[1], "rb");
    assert(f);
    unsigned long cases = 0, checks = 0;
    int64_t       expected[22];
    while (fread(cfg, sizeof(cfg), 1, f) == 1) {
        unsigned n = cfg[0] ? 2 : 22;
        assert(fread(expected, sizeof(*expected), n, f) == n);
        setup(app);
        int64_t actual[22];
        if (cfg[0]) {
            actual[0] = CMineFromLpfl(app, &fleets[0]);
            actual[1] = partCalls;
        } else {
            int32_t result[3] = {777, 777, 777};
            EstMineralsMined(app, &planet, result, cfg[1], (int16_t)cfg[8]);
            for (int i = 0; i < 3; i++) {
                actual[i] = result[i];
                actual[3 + i] = planet.rgwtMin[i];
                actual[6 + i] = planet.rgMinConc[i];
                actual[9 + i] = planet.rgpctMinLevel[i];
            }
            for (int i = 0; i < 4; i++)
                actual[12 + i] = fleets[i].fHereAllTurn;
            actual[16] = draws;
            actual[17] = mineCalls;
            actual[18] = partCalls;
            actual[19] = operCalls;
            actual[20] = app.state.lRandSeed1;
            actual[21] = app.state.lRandSeed2;
        }
        for (unsigned i = 0; i < n; i++) {
            checks++;
            if (actual[i] != expected[i]) {
                fprintf(stderr, "case %lu mode %d field %u actual %lld expected %lld (count %d conc %d level %d)\n", cases, cfg[0], i, (long long)actual[i],
                        (long long)expected[i], cfg[1], cfg[15], cfg[18]);
                return 1;
            }
        }
        cases++;
    }
    assert(feof(f));
    fclose(f);
    printf("Native mining/depletion: %lu cases, %lu assertions passed\n", cases, checks);
    return 0;
}
