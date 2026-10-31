#include "planet_query_io.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include "race_query_io.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include "common.h"
static PLANET planet, stored;
static SHDEF  bases[2][10];

static int32_t  cfg[37];
static int      stats, lookups, growths, grbits;
static uint64_t floatBits;
static void     RecordFloat(double x) noexcept { memcpy(&floatBits, &x, 8); }
static int16_t  LookupPlanet(APPSESSION &, int16_t id, PLANET *out) {
    assert(id == 0);
    lookups++;
    *out = stored;
    return 1;
}
static int16_t Habitat(APPSESSION &app, PLANET *p, int16_t player) {
    assert(player == 0 || player == 1);
    return cfg[17] ? (int16_t)cfg[16] : PctPlanetDesirability(app, p, player);
}
static int32_t Growth(APPSESSION &app, PLANET *p, int16_t update) {
    growths++;
    if (cfg[25]) {
        assert(!update);
        return cfg[26];
    }
    return ChgPopFromPlanet(app, p, update);
}

static int16_t QueryRaceStat(PLAYER *p, RaceStat stat) {
    stats++;
    return GetRaceStat(p, stat);
}
static int16_t RaceTrait(PLAYER *p, RaceGrbit bit) {
    grbits++;
    return GetRaceGrbit(p, bit);
}

static void setup(APPSESSION &app) {
    memset(&planet, 0, sizeof(planet));
    memset(app.state.rgplr, 0, sizeof(app.state.rgplr));
    memset(bases, 0, sizeof(bases));
    for (int player = 0; player < 2; player++) {
        int at = player ? 29 : 8;
        app.state.rgplr[player].rgAttr[rsMajorAdv] = (int8_t)cfg[at];
        app.state.rgplr[player].rgAttr[rsResGen] = (int8_t)cfg[at + 1];
        app.state.rgplr[player].rgAttr[rsFactProd] = (int8_t)cfg[at + 2];
        app.state.rgplr[player].rgAttr[rsFactOperate] = (int8_t)cfg[at + 3];
        app.state.rgplr[player].rgAttr[rsMineOperate] = (int8_t)cfg[at + 4];
        app.state.rgplr[player].pctIdealGrowth = (int8_t)cfg[at + 5];
        app.state.rgplr[player].rgTech[0] = (int8_t)cfg[at + 6];
        app.state.rgplr[player].grbitAttr = static_cast<RaceTraitBits>(cfg[at + 7] ? static_cast<uint32_t>(grbitRaceOBRM) : 0);
        for (int axis = 0; axis < 3; axis++) {
            app.state.rgplr[player].rgEnvVarMin[axis] = 0;
            app.state.rgplr[player].rgEnvVar[axis] = 50;
            app.state.rgplr[player].rgEnvVarMax[axis] = 100;
        }
        app.state.rglpshdefSB[player] = bases[player];
        for (int i = 0; i < 10; i++)
            bases[player][i].hul.ihuldef = (HulDef)(ihuldefOrbitalFort + cfg[22]);
    }
    planet.rgwtMin[3] = cfg[1];
    planet.iDeltaPop = cfg[2];
    planet.cMines = cfg[3];
    planet.cFactories = cfg[4];
    planet.cDefenses = cfg[5];
    planet.iPlayer = (int16_t)cfg[6];
    planet.fStarbase = cfg[20];
    planet.isb = cfg[21];
    for (int i = 0; i < 3; i++)
        planet.rgEnvVar[i] = (int8_t)cfg[18];
    stored = planet;
    stored.iPlayer = (int16_t)cfg[27];
    stored.fStarbase = cfg[28];
    for (int i = 0; i < 3; i++)
        stored.rgEnvVar[i] = (int8_t)cfg[19];
    stats = lookups = growths = grbits = 0;
    floatBits = 0;
    app.state.idPlayer = (int16_t)cfg[7];
}
int main(int argc, char **argv) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    PLANETQUERYIO io;
    io.pfnLookup = LookupPlanet;
    io.pfnDesirability = Habitat;
    io.pfnGrowth = Growth;
    io.pfnTruncate = RecordFloat;
    PLANETQUERYIOSCOPE planetScope(io);
    RACEQUERYIO        race;
    race.pfnStat = QueryRaceStat;
    race.pfnGrbit = RaceTrait;
    RACEQUERYIOSCOPE raceScope(race);
    assert(argc == 2 && sizeof(double) == 8);
    FILE *f = fopen(argv[1], "rb");
    assert(f);
    int64_t       expected[3];
    uint64_t      bits;
    unsigned long cases = 0, checks = 0;
    while (fread(cfg, sizeof(cfg), 1, f) == 1) {
        assert(fread(expected, sizeof(expected), 1, f) == 1 && fread(&bits, sizeof(bits), 1, f) == 1);
        setup(app);
        int64_t result;
        switch (cfg[0]) {
        case 0:
            result = CalcPlanetMaxPop(app, 0, (int16_t)cfg[7]);
            break;
        case 1:
            result = CMaxMines(app, &planet, (int16_t)cfg[7]);
            break;
        case 2:
            result = CMaxFactories(app, &planet, (int16_t)cfg[7]);
            break;
        case 3:
            result = CMaxOperableMines(app, &planet, (int16_t)cfg[7], (int16_t)cfg[23]);
            break;
        case 4:
            result = CMaxOperableFactories(app, &planet, (int16_t)cfg[7], (int16_t)cfg[23]);
            break;
        case 5:
            result = CMinesOperating(app, &planet);
            break;
        case 6:
            result = CFactoriesOperating(app, &planet);
            break;
        case 7:
            result = CMaxDefenses(app, &planet, (int16_t)cfg[7]);
            break;
        case 8:
            result = CMaxOperableDefenses(app, &planet, (int16_t)cfg[7], (int16_t)cfg[23]);
            break;
        case 9:
            result = CResourcesAtPlanet(app, &planet, (int16_t)cfg[7]);
            break;
        case 10:
            result = PctPlanetCapacity(app, &planet);
            break;
        default:
            result = PlanetQueryIO().pfnGrowth(app, &planet, (int16_t)cfg[24]);
            break;
        }
        int64_t actual[] = {result, planet.rgwtMin[3], planet.iDeltaPop};
        for (int i = 0; i < 3; i++) {
            checks++;
            if (actual[i] != expected[i]) {
                fprintf(stderr, "case %lu op %d field %d actual %lld expected %lld (pop %d, rate %d)\n", cases, cfg[0], i, (long long)actual[i],
                        (long long)expected[i], cfg[1], cfg[12]);
                return 1;
            }
        }
        checks++;
        if (floatBits != bits) {
            fprintf(stderr, "case %lu op %d float bits %" PRIu64 " expected %" PRIu64 "\n", cases, cfg[0], floatBits, bits);
            return 1;
        }
        assert(growths == (cfg[0] == 11 || ((cfg[0] == 3 || cfg[0] == 4 || cfg[0] == 8) && cfg[23])));
        checks++;
        if (cfg[0] == 9 && cfg[1] == 0) {
            assert(!stats && !lookups && !growths);
            checks++;
        }
        if (cfg[0] == 0) {
            int primary = app.state.rgplr[cfg[7]].rgAttr[rsMajorAdv];
            assert(stats == (primary == 8 ? 1 : primary == 0 ? 2 : 3) && lookups == 1);
            assert(grbits == (primary != 8 || (stored.iPlayer == cfg[7] && stored.fStarbase)));
            checks += 2;
        }
        cases++;
    }
    assert(feof(f));
    fclose(f);
    printf("Native colony linked differential: %lu cases, %lu assertions passed\n", cases, checks);
    return 0;
}
