#ifndef STARS_TERRAFORM_FIXTURE_H
#define STARS_TERRAFORM_FIXTURE_H
#include "construction_buildability_fixture.h"
static int32_t cfg[54];
static int     currentDesire, terraCalls, desireCalls, terraItems[128], desireInputs[16][4], observedLow[3], observedHigh[3], observedCost[3];
static TERRA   scripted[20];
static int16_t TerraformHabitat(PLANET *p, int16_t player) {
    assert(p == &planet && desireCalls < 16);
    int at = desireCalls++;
    desireInputs[at][0] = player;
    for (int i = 0; i < 3; i++)
        desireInputs[at][i + 1] = p->rgEnvVar[i];
    return cfg[49] ? (int16_t)cfg[50 + currentDesire++] : PctPlanetDesirability(p, player);
}
static int16_t TerraformBest(PLANET *p, int16_t help) {
    terraformCalls++;
    currentDesire = 0;
    return IBestTerraform(p, help);
}
static int16_t TerraformLookup(PART *part) {
    if (part->hs.grhst != hstTerra)
        return GateLookup(part);
    assert(terraCalls < 128 && part->hs.iItem < 20);
    terraItems[terraCalls++] = part->hs.iItem;
    if (!cfg[1])
        return FLookupPart(part);
    part->SetPterra(scripted + part->hs.iItem);
    return (cfg[2] & (1 << part->hs.iItem)) ? 1 : 0;
}
static int16_t TerraformCan(PLANET *p, int16_t *low, int16_t *high, int16_t *cost, int16_t help) {
    // Initialize the supplied outputs for observation, as in the original domain.
    for (int i = 0; i < 3; i++)
        low[i] = high[i] = cost[i] = 9876;
    int16_t result = FCanTerraformLppl(p, low, high, cost, help);
    for (int i = 0; i < 3; i++) {
        observedLow[i] = low[i];
        observedHigh[i] = high[i];
        observedCost[i] = cost[i];
    }
    return result;
}
struct TERRAFORMFIXTURE {
    CONSTRUCTIONBUILDFIXTURE fixture;
    COLONYIOSCOPE            colonyScope;
    PLANETQUERYIOSCOPE       planetScope;
    PARTQUERYIOSCOPE         partScope;
    TERRAFORMFIXTURE()
        : colonyScope([] {
              COLONYIO io = ColonyIO();
              io.pfnBestTerraform = TerraformBest;
              io.pfnCanTerraform = TerraformCan;
              return io;
          }()),
          planetScope([] {
              PLANETQUERYIO io = PlanetQueryIO();
              io.pfnDesirability = TerraformHabitat;
              return io;
          }()),
          partScope([] {
              PARTQUERYIO io = PartQueryIO();
              io.pfnLookup = TerraformLookup;
              return io;
          }()) {}
};
#endif
