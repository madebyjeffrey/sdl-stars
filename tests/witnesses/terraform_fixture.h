#ifndef STARS_TERRAFORM_FIXTURE_H
#define STARS_TERRAFORM_FIXTURE_H
#include "construction_buildability_fixture.h"
static int32_t cfg[54];
static int     currentDesire, terraCalls, desireCalls, terraItems[128], desireInputs[16][4], observedLow[3], observedHigh[3], observedCost[3];
static TERRA   scripted[20];
static int16_t TerraformHabitat(APPSESSION &app, PLANET *p, int16_t player) {
    assert(p == &planet && desireCalls < 16);
    int at = desireCalls++;
    desireInputs[at][0] = player;
    for (int i = 0; i < 3; i++)
        desireInputs[at][i + 1] = p->rgEnvVar[i];
    return cfg[49] ? (int16_t)cfg[50 + currentDesire++] : PctPlanetDesirability(app, p, player);
}
static int16_t TerraformBest(APPSESSION &app, PLANET *p, int16_t help) {
    terraformCalls++;
    currentDesire = 0;
    return IBestTerraform(app, p, help);
}
static int16_t TerraformLookup(APPSESSION &app, PART *part) {
    if (part->hs.grhst != hstTerra)
        return GateLookup(app, part);
    assert(terraCalls < 128 && part->hs.iItem < 20);
    terraItems[terraCalls++] = part->hs.iItem;
    if (!cfg[1])
        return FLookupPart(app, part);
    part->SetPterra(scripted + part->hs.iItem);
    return (cfg[2] & (1 << part->hs.iItem)) ? 1 : 0;
}
static std::optional<TERRAFORMRESULT> TerraformCan(APPSESSION &app, PLANET *p, int16_t help) {
    std::optional<TERRAFORMRESULT> result = FCanTerraformLppl(app, p, help);
    if (result) {
        for (int i = 0; i < 3; i++) {
            observedLow[i] = result->rgEnvMin[i];
            observedHigh[i] = result->rgEnvMax[i];
            if (result->rgEnvCost[i]) {
                observedCost[i] = *result->rgEnvCost[i];
            }
        }
    }
    return result;
}
struct TERRAFORMFIXTURE {
    CONSTRUCTIONBUILDFIXTURE fixture;
    COLONYIOSCOPE            colonyScope;
    PLANETQUERYIOSCOPE       planetScope;
    PARTQUERYIOSCOPE         partScope;
    TERRAFORMFIXTURE(APPSESSION &app)
        : fixture(app), colonyScope([] {
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
