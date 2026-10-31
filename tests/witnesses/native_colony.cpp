#include "planet_query_io.h"
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
static int16_t  LookupPlanet(int16_t id, PLANET *out) {
    assert(id == 0);
    lookups++;
    *out = stored;
    return 1;
}
static int16_t Habitat(PLANET *p, int16_t player) {
    assert(player == 0 || player == 1);
    return cfg[17] ? (int16_t)cfg[16] : PctPlanetDesirability(p, player);
}
static int32_t Growth(PLANET *p, int16_t update) {
    growths++;
    if (cfg[25]) {
        assert(!update);
        return cfg[26];
    }
    return ChgPopFromPlanet(p, update);
}

static int16_t QueryRaceStat(PLAYER *p, RaceStat stat) {
    stats++;
    return GetRaceStat(p, stat);
}
static int16_t RaceTrait(PLAYER *p, RaceGrbit bit) {
    grbits++;
    return GetRaceGrbit(p, bit);
}

static void setup(void) {
    memset(&planet, 0, sizeof(planet));
    memset(rgplr, 0, sizeof(rgplr));
    memset(bases, 0, sizeof(bases));
    for (int player = 0; player < 2; player++) {
        int at = player ? 29 : 8;
        rgplr[player].rgAttr[rsMajorAdv] = (int8_t)cfg[at];
        rgplr[player].rgAttr[rsResGen] = (int8_t)cfg[at + 1];
        rgplr[player].rgAttr[rsFactProd] = (int8_t)cfg[at + 2];
        rgplr[player].rgAttr[rsFactOperate] = (int8_t)cfg[at + 3];
        rgplr[player].rgAttr[rsMineOperate] = (int8_t)cfg[at + 4];
        rgplr[player].pctIdealGrowth = (int8_t)cfg[at + 5];
        rgplr[player].rgTech[0] = (int8_t)cfg[at + 6];
        rgplr[player].grbitAttr = static_cast<RaceTraitBits>(cfg[at + 7] ? static_cast<uint32_t>(grbitRaceOBRM) : 0);
        for (int axis = 0; axis < 3; axis++) {
            rgplr[player].rgEnvVarMin[axis] = 0;
            rgplr[player].rgEnvVar[axis] = 50;
            rgplr[player].rgEnvVarMax[axis] = 100;
        }
        rglpshdefSB[player] = bases[player];
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
    idPlayer = (int16_t)cfg[7];
}
int main(int argc, char **argv) {
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
        setup();
        int64_t result;
        switch (cfg[0]) {
        case 0:
            result = CalcPlanetMaxPop(0, (int16_t)cfg[7]);
            break;
        case 1:
            result = CMaxMines(&planet, (int16_t)cfg[7]);
            break;
        case 2:
            result = CMaxFactories(&planet, (int16_t)cfg[7]);
            break;
        case 3:
            result = CMaxOperableMines(&planet, (int16_t)cfg[7], (int16_t)cfg[23]);
            break;
        case 4:
            result = CMaxOperableFactories(&planet, (int16_t)cfg[7], (int16_t)cfg[23]);
            break;
        case 5:
            result = CMinesOperating(&planet);
            break;
        case 6:
            result = CFactoriesOperating(&planet);
            break;
        case 7:
            result = CMaxDefenses(&planet, (int16_t)cfg[7]);
            break;
        case 8:
            result = CMaxOperableDefenses(&planet, (int16_t)cfg[7], (int16_t)cfg[23]);
            break;
        case 9:
            result = CResourcesAtPlanet(&planet, (int16_t)cfg[7]);
            break;
        case 10:
            result = PctPlanetCapacity(&planet);
            break;
        default:
            result = PlanetQueryIO().pfnGrowth(&planet, (int16_t)cfg[24]);
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
            int primary = rgplr[cfg[7]].rgAttr[rsMajorAdv];
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
