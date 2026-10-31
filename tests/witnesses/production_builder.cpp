#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "colony_io.h"
#include "common.h"
#include "heap_io.h"
#include "planet_query_io.h"
#include "production_io.h"
#include "race_query_io.h"
#include "random_io.h"
#include "research_io.h"
#include "turn_io.h"
static int     cheapFixture;
static int32_t quote[4], limits[7];
static int     calls, messages, messageArg, quoted;
static void    TestGetProductionCosts(PLANET *p, PROD *r, uint32_t *out, int16_t player, int16_t one) {
    (void)r;
    assert(p && player == 0 && one && !quoted && calls == 0);
    quoted = 1;
    memcpy(out, quote, 16);
}
static int16_t TestCMaxOperableMines(PLANET *p, int16_t player, int16_t next) {
    (void)p;
    assert(quoted && !player && next);
    calls = calls * 10 + 1;
    return (int16_t)limits[0];
}
static int16_t TestCMaxOperableFactories(PLANET *p, int16_t player, int16_t next) {
    (void)p;
    assert(quoted && !player && next);
    calls = calls * 10 + 2;
    return (int16_t)limits[0];
}
static int16_t TestCMaxOperableDefenses(PLANET *p, int16_t player, int16_t next) {
    (void)p;
    assert(quoted && !player && next);
    calls = calls * 10 + 3;
    return (int16_t)limits[0];
}
static int16_t TestIpctCanTerraformLppl(PLANET *p) {
    (void)p;
    calls = calls * 10 + 4;
    return (int16_t)limits[2];
}
static int32_t TestChgPopFromPlanet(PLANET *p, int16_t update) {
    (void)p;
    assert(!update);
    calls = calls * 10 + 5;
    return limits[3];
}
static int16_t TestPctPlanetDesirability(PLANET *p, int16_t player) {
    (void)p;
    assert(!player);
    calls = calls * 10 + 6;
    return (int16_t)limits[4];
}
static int16_t TestIWarpMAFromLppl(PLANET *p, int16_t *flag) {
    (void)p;
    assert(!flag);
    calls = calls * 10 + 7;
    return (int16_t)limits[5];
}
static int16_t TestGetRaceGrbit(PLAYER *p, RaceGrbit bit) {
    assert(p == rgplr && bit == ibitRaceMineralAlchemy);
    return (int16_t)cheapFixture;
}
static int16_t TestFSendPlrMsg2(int16_t player, MessageId id, MsgGoto a, int16_t b, int16_t c) {
    assert(!player && id == idmScientistsHaveTransmutedCommonMaterialsKtEach && a == 5 && b == 5);
    messages++;
    messageArg = c;
    calls = calls * 10 + 8;
    return 1;
}

int main(int argc, char **argv) {
    TURNIO ioTurn;
    ioTurn.pfnMessage2 = TestFSendPlrMsg2;
    TURNIOSCOPE scopeTurn(ioTurn);
    COLONYIO    ioColony;
    ioColony.pfnOperableMines = TestCMaxOperableMines;
    ioColony.pfnOperableFactories = TestCMaxOperableFactories;
    ioColony.pfnOperableDefenses = TestCMaxOperableDefenses;
    ioColony.pfnTerraformPercent = TestIpctCanTerraformLppl;
    ioColony.pfnWarp = TestIWarpMAFromLppl;
    COLONYIOSCOPE scopeColony(ioColony);
    PLANETQUERYIO ioPlanetQuery;
    ioPlanetQuery.pfnGrowth = TestChgPopFromPlanet;
    ioPlanetQuery.pfnDesirability = TestPctPlanetDesirability;
    PLANETQUERYIOSCOPE scopePlanetQuery(ioPlanetQuery);
    PRODUCTIONIO       ioProduction;
    ioProduction.pfnQuote = TestGetProductionCosts;
    PRODUCTIONIOSCOPE scopeProduction(ioProduction);
    RACEQUERYIO       ioRaceQuery;
    ioRaceQuery.pfnGrbit = TestGetRaceGrbit;
    RACEQUERYIOSCOPE scopeRaceQuery(ioRaceQuery);
    static_assert(sizeof(PROD) == 4, "PROD wire width");
    assert(argc == 2);
    FILE *f = fopen(argv[1], "rb");
    assert(f);
    int32_t       input[23];
    int64_t       expected[11];
    unsigned long cases = 0, checks = 0;
    while (fread(input, sizeof(input), 1, f) == 1) {
        assert(fread(expected, sizeof(expected), 1, f) == 1);
        PLANET   p = {};
        PROD     row, partial;
        int32_t  resources[4];
        uint32_t raw = (uint32_t)input[0];
        memcpy(&row, &raw, 4);
        raw = (uint32_t)input[1];
        memcpy(&partial, &raw, 4);
        memcpy(quote, input + 2, 16);
        memcpy(resources, input + 6, 16);
        memcpy(limits, input + 10, 28);
        p.id = 5;
        p.cMines = limits[1];
        p.cFactories = (limits[1] + 17) % 4096;
        p.cDefenses = (limits[1] + 29) % 4096;
        p.idFling = limits[6];
        cheapFixture = input[18];
        gd.fGeneratingTurn = input[20];
        calls = messages = messageArg = quoted = 0;
        int16_t status = -123;
        int16_t built = CBuildProdItem(&p, &row, input[21] ? &partial : NULL, resources, (int16_t)input[17], input[22] ? &status : NULL, (int16_t)input[19]);
        assert(quoted);
        uint32_t rword, pword;
        memcpy(&rword, &row, 4);
        memcpy(&pword, &partial, 4);
        int64_t actual[11] = {built, status, rword, pword, resources[0], resources[1], resources[2], resources[3], messages, messageArg, calls};
        for (int i = 0; i < 11; i++) {
            checks++;
            if (actual[i] != expected[i]) {
                fprintf(stderr, "case %lu field %d actual %lld expected %lld row %u\n", cases, i, (long long)actual[i], (long long)expected[i],
                        (unsigned)input[0]);
                return 1;
            }
        }
        cases++;
    }
    assert(feof(f));
    fclose(f);
    printf("M09 builder linked differential: %lu cases, %lu assertions passed\n", cases, checks);
    return 0;
}
