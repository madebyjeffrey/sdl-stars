#include "research_io.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"

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
static int32_t TechQuote(APPSESSION &app, TechFieldType field, int16_t level, int16_t player) {
    int total = 0;
    for (int i = 0; i < 6; i++)
        total += app.state.rgplr[player].rgTech[i];
    int32_t result = GetTechLevelCost(app, field, level, player);
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
static void callback(APPSESSION &app, int kind, int index, int argument) {
    calls++;
    hashValue(kind);
    hashValue(index);
    hashValue(argument);
    hashValue(app.state.rgplr[0].pctResearch);
    hashValue(app.state.rgplr[1].pctResearch);
    if (kind == 1)
        grossCalls++;
    else
        forecastCalls++;
    if ((kind == 1 || (cfg[34] & 128)) && !mutated) {
        if (cfg[34] & 1)
            app.state.idPlayer = (int16_t)cfg[35];
        if (cfg[34] & 2) {
            assert(0 <= app.state.idPlayer && app.state.idPlayer < 16);
            app.state.rgplr[app.state.idPlayer].pctResearch = (int8_t)cfg[36];
        }
        if ((cfg[34] & 4) && cfg[42 + 5 * (index % 4)] != -2)
            setQueue(index, cfg[42 + 5 * (index % 4)]);
        if (cfg[34] & 8)
            app.state.cPlanet = 0;
        if ((cfg[34] & 16) && index + 1 < cfg[32])
            storage[index + 1].iPlayer = app.state.idPlayer;
        mutated = 1;
    }
    if (calls - 1 == cfg[33])
        throw RESEARCHESCAPE{};
}
static int16_t CResearchResources(APPSESSION &app, PLANET *planet, int16_t player) {
    int index = (int)(planet - storage);
    assert(index >= 0 && index < cfg[32]);
    callback(app, 1, index, player);
    return (int16_t)cfg[40 + 5 * (index % 4)];
}
static PRODUCTIONESTIMATE Estimate(APPSESSION &app, PLANET *planet, PLPROD *row, ProdItemType item) {
    int index = (int)(planet - storage);
    assert(!row && item == static_cast<ProdItemType>(iprodEstimateResearchResources));
    callback(app, 2, index, app.state.idPlayer);
    return {(int16_t)cfg[41 + 5 * (index % 4)], 123};
}

static void setup(APPSESSION &app) {
    memset(app.state.rgplr, 0, sizeof(app.state.rgplr));
    memset(&app.state.game, 0, sizeof(app.state.game));
    memset(queues, 0, sizeof(queues));
    calls = grossCalls = forecastCalls = mutated = 0;
    traceHash = 0;
    app.state.idPlayer = 0;
    app.state.game.fSlowTech = cfg[1];
    for (int i = 0; i < 6; i++) {
        app.state.rgplr[0].rgTech[i] = (int8_t)cfg[4 + i];
        app.state.rgplr[0].rgAttr[8 + i] = (int8_t)cfg[10 + i];
        app.state.rgplr[0].rgResSpent[i] = (uint32_t)cfg[22 + i];
    }
    if (cfg[0] == 2) {
        app.state.idPlayer = (int16_t)cfg[29];
        app.state.rgplr[0].pctResearch = (int8_t)cfg[30];
        app.state.rgplr[1].pctResearch = (int8_t)cfg[31];
        app.state.cPlanet = (int16_t)cfg[32];
        int count = cfg[32] > 4 ? cfg[32] : 4;
        if (count > capacity) {
            free(storage);
            storage = static_cast<PLANET *>(calloc((size_t)count, sizeof(*storage)));
            assert(storage);
            capacity = count;
        }
        app.state.lpPlanets = storage;
        memset(storage, 0, (size_t)count * sizeof(*storage));
        for (int i = 0; i < cfg[32]; i++) {
            storage[i].iPlayer = (int16_t)cfg[38 + 5 * (i % 4)];
            setQueue(i, cfg[39 + 5 * (i % 4)]);
        }
    }
}
int main(int argc, char **argv) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

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
        setup(app);
        int64_t actual[22] = {};
        if (cfg[0] == 0)
            actual[0] = ResearchIO().pfnTechCost(app, (TechFieldType)cfg[2], (int16_t)cfg[3], 0);
        else if (cfg[0] == 1) {
            char target[6];
            for (int i = 0; i < 6; i++)
                target[i] = (char)cfg[16 + i];
            actual[0] = CostOfDevelopingItem(app, target);
        } else {
            int     complete = 0;
            int32_t value = 0;
            try {
                value = ProjectedResearchSpending(app, cfg[28]);
                complete = 1;
            } catch (const RESEARCHESCAPE &) {
            }
            actual[0] = complete ? value : 0;
            actual[1] = app.state.rgplr[0].pctResearch;
            actual[2] = app.state.rgplr[1].pctResearch;
            actual[3] = app.state.idPlayer;
            actual[4] = calls;
            actual[5] = grossCalls;
            actual[6] = forecastCalls;
            actual[7] = traceHash;
            actual[8] = complete;
            actual[9] = app.state.cPlanet;
            for (int i = 0; i < 4 && i < cfg[32]; i++) {
                actual[10 + i] = storage[i].iPlayer;
                actual[14 + i] = storage[i].lpplprod ? storage[i].lpplprod->iprodMac : -1;
            }
            actual[18] = app.state.rgplr[2].pctResearch;
            actual[19] = app.state.rgplr[3].pctResearch;
            actual[20] = app.state.rgplr[14].pctResearch;
            actual[21] = app.state.rgplr[15].pctResearch;
        }
        if (cfg[0] != 2) {
            for (int i = 0; i < 6; i++) {
                actual[1 + i] = app.state.rgplr[0].rgTech[i];
                actual[9 + i] = app.state.rgplr[0].rgResSpent[i];
                actual[15 + i] = app.state.rgplr[0].rgAttr[8 + i];
            }
            actual[7] = calls;
            actual[8] = traceHash;
            actual[21] = app.state.idPlayer;
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
    app.state.lpPlanets = nullptr;
    free(storage);
    printf("Native research quotes/preview: %lu cases, %lu assertions passed\n", cases, checks);
    return 0;
}
