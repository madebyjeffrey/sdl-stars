#include "research_io.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include "common.h"
static int32_t  cfg[64];
static PLANET  *storage;
static int      capacity;
static PLPROD   queues[4];
static int      calls, grossCalls, forecastCalls, mutated;
static uint32_t traceHash;
struct RESEARCHESCAPE {};
static void    hashValue(int32_t value) { traceHash = traceHash * UINT32_C(16777619) ^ (uint32_t)value; }
static int32_t TechQuote(TechFieldType field, int16_t level, int16_t player) {
    int total = 0;
    for (int i = 0; i < 6; i++)
        total += rgplr[player].rgTech[i];
    int32_t result = GetTechLevelCost(field, level, player);
    calls++;
    hashValue(field);
    hashValue(level);
    hashValue(total);
    hashValue(result);
    return result;
}
static void setQueue(int index, int value) {
    if (value < 0)
        storage[index].lpplprod = NULL;
    else {
        queues[index % 4].iprodMac = (uint8_t)value;
        storage[index].lpplprod = &queues[index % 4];
    }
}
static void callback(int kind, int index, int argument) {
    calls++;
    hashValue(kind);
    hashValue(index);
    hashValue(argument);
    hashValue(rgplr[0].pctResearch);
    hashValue(rgplr[1].pctResearch);
    if (kind == 1)
        grossCalls++;
    else
        forecastCalls++;
    if ((kind == 1 || (cfg[34] & 128)) && !mutated) {
        if (cfg[34] & 1)
            idPlayer = (int16_t)cfg[35];
        if (cfg[34] & 2) {
            assert(0 <= idPlayer && idPlayer < 16);
            rgplr[idPlayer].pctResearch = (int8_t)cfg[36];
        }
        if ((cfg[34] & 4) && cfg[42 + 5 * (index % 4)] != -2)
            setQueue(index, cfg[42 + 5 * (index % 4)]);
        if (cfg[34] & 8)
            cPlanet = 0;
        if ((cfg[34] & 16) && index + 1 < cfg[32])
            storage[index + 1].iPlayer = idPlayer;
        mutated = 1;
    }
    if (calls - 1 == cfg[33])
        throw RESEARCHESCAPE{};
}
static int16_t CResearchResources(PLANET *planet, int16_t player) {
    int index = (int)(planet - storage);
    assert(index >= 0 && index < cfg[32]);
    callback(1, index, player);
    return (int16_t)cfg[40 + 5 * (index % 4)];
}
static void Estimate(PLANET *planet, PLPROD *row, ProdItemType item, int16_t *first, int16_t *last) {
    int index = (int)(planet - storage);
    assert(!row && item == static_cast<ProdItemType>(iprodEstimateResearchResources));
    callback(2, index, idPlayer);
    *first = (int16_t)cfg[41 + 5 * (index % 4)];
    *last = 123;
}

static void setup(void) {
    memset(rgplr, 0, sizeof(rgplr));
    memset(&game, 0, sizeof(game));
    memset(queues, 0, sizeof(queues));
    calls = grossCalls = forecastCalls = mutated = 0;
    traceHash = 0;
    idPlayer = 0;
    game.fSlowTech = cfg[1];
    for (int i = 0; i < 6; i++) {
        rgplr[0].rgTech[i] = (int8_t)cfg[4 + i];
        rgplr[0].rgAttr[8 + i] = (int8_t)cfg[10 + i];
        rgplr[0].rgResSpent[i] = (uint32_t)cfg[22 + i];
    }
    if (cfg[0] == 2) {
        idPlayer = (int16_t)cfg[29];
        rgplr[0].pctResearch = (int8_t)cfg[30];
        rgplr[1].pctResearch = (int8_t)cfg[31];
        cPlanet = (int16_t)cfg[32];
        int count = cfg[32] > 4 ? cfg[32] : 4;
        if (count > capacity) {
            free(storage);
            storage = static_cast<PLANET *>(calloc((size_t)count, sizeof(*storage)));
            assert(storage);
            capacity = count;
        }
        lpPlanets = storage;
        memset(storage, 0, (size_t)count * sizeof(*storage));
        for (int i = 0; i < cfg[32]; i++) {
            storage[i].iPlayer = (int16_t)cfg[38 + 5 * (i % 4)];
            setQueue(i, cfg[39 + 5 * (i % 4)]);
        }
    }
}
int main(int argc, char **argv) {
    RESEARCHIO io;
    io.pfnTechCost = TechQuote;
    io.pfnResources = CResearchResources;
    io.pfnEstimate = Estimate;
    RESEARCHIOSCOPE researchScope(io);
    assert(argc == 2);
    FILE *file = fopen(argv[1], "rb");
    assert(file);
    unsigned long cases = 0, checks = 0;
    int64_t       expected[22];
    while (fread(cfg, sizeof(cfg), 1, file) == 1) {
        assert(fread(expected, sizeof(expected), 1, file) == 1);
        setup();
        int64_t actual[22] = {};
        if (cfg[0] == 0)
            actual[0] = ResearchIO().pfnTechCost((TechFieldType)cfg[2], (int16_t)cfg[3], 0);
        else if (cfg[0] == 1) {
            char target[6];
            for (int i = 0; i < 6; i++)
                target[i] = (char)cfg[16 + i];
            actual[0] = CostOfDevelopingItem(target);
        } else {
            int     complete = 0;
            int32_t value = 0;
            try {
                value = ProjectedResearchSpending(cfg[28]);
                complete = 1;
            } catch (const RESEARCHESCAPE &) {
            }
            actual[0] = complete ? value : 0;
            actual[1] = rgplr[0].pctResearch;
            actual[2] = rgplr[1].pctResearch;
            actual[3] = idPlayer;
            actual[4] = calls;
            actual[5] = grossCalls;
            actual[6] = forecastCalls;
            actual[7] = traceHash;
            actual[8] = complete;
            actual[9] = cPlanet;
            for (int i = 0; i < 4 && i < cfg[32]; i++) {
                actual[10 + i] = storage[i].iPlayer;
                actual[14 + i] = storage[i].lpplprod ? storage[i].lpplprod->iprodMac : -1;
            }
            actual[18] = rgplr[2].pctResearch;
            actual[19] = rgplr[3].pctResearch;
            actual[20] = rgplr[14].pctResearch;
            actual[21] = rgplr[15].pctResearch;
        }
        if (cfg[0] != 2) {
            for (int i = 0; i < 6; i++) {
                actual[1 + i] = rgplr[0].rgTech[i];
                actual[9 + i] = rgplr[0].rgResSpent[i];
                actual[15 + i] = rgplr[0].rgAttr[8 + i];
            }
            actual[7] = calls;
            actual[8] = traceHash;
            actual[21] = idPlayer;
        }
        for (int i = 0; i < 22; i++) {
            if (cfg[0] == 2 && i == 0 && !actual[8])
                continue;
            checks++;
            if (actual[i] != expected[i]) {
                fprintf(stderr, "case %lu mode %d field %d actual %lld expected %lld\n", cases, cfg[0], i, (long long)actual[i], (long long)expected[i]);
                return 1;
            }
        }
        cases++;
    }
    assert(feof(file));
    fclose(file);
    lpPlanets = nullptr;
    free(storage);
    printf("Native research quotes/preview: %lu cases, %lu assertions passed\n", cases, checks);
    return 0;
}
