// Standalone verification only: links the already built native host test objects.
// It prints RDL requirements using actual game-function results, not copied formulas.
#include "appstate.h"
#include "callbackcontext.h"
#include "stars_test.h"
#include <array>
#include <cstdio>
#include <string>

static constexpr const char *rgszRace[] = {"humanoid", "rabbitoid", "insectoid", "nucleotid", "silicanoid", "antetheral", "random"};
static constexpr const char *rgszPrt[] = {"hyper_expansion", "super_stealth", "war_monger", "claim_adjuster", "inner_strength", "space_demolition", "packet_physics", "interstellar_traveler", "alternate_reality", "jack_of_all_trades"};
static constexpr const char *rgszLrt[] = {"ife", "tt", "arm", "isb", "gr", "ur", "ma", "nrse", "ce", "obrm", "nas", "lsp", "bet", "rs"};

static constexpr const char *pszDefaultChoices = "portrait(1) gravity(15,85) temperature(15,85) radiation(15,85) growth(15) population_efficiency(1000) factory_output(10) factory_cost(10) factory_operation(10) mine_output(10) mine_cost(5) mine_operation(10) energy_research(standard) weapons_research(standard) propulsion_research(standard) construction_research(standard) electronics_research(standard) biotechnology_research(standard) research_allocation(15) leftover_points(surface_minerals) standard_capacity";

static void Require(const std::string &szExpression, long long lExpected) {
    std::printf("  require %s = %lld;\n", szExpression.c_str(), lExpected);
}

static void Presets(APPSESSION &app, int iOnly) {
    static constexpr const char *rgszEconomy[] = {"population_divisor", "factory_output", "factory_cost", "factory_operation", "mine_output", "mine_cost", "mine_operation"};
    static constexpr const char *rgszTech[] = {"energy", "weapons", "propulsion", "construction", "electronics", "biotechnology"};
    static constexpr const char *rgszAxis[] = {"gravity", "temperature", "radiation"};
    std::int16_t i;
    std::int16_t j;
    PLAYER plr;
    for (i = 0; i < 7; ++i) {
        if (iOnly >= 0 && i != iOnly) continue;
        if (i < 6) std::printf("let native_pricing_%d = WizardPricing { race = %s; };\n", i, rgszRace[i]);
        plr = vrgplrDef[i];
        const std::string szRace = rgszRace[i];
        std::printf("object type NativePreset%d = {\n", i);
        Require(szRace + ".DesignSettings.source_index", i);
        Require(szRace + ".DesignSettings.portrait", plr.iPlrBmp);
        Require(szRace + ".DesignSettings.selected_growth", plr.pctIdealGrowth);
        Require(szRace + ".DesignSettings.leftover_allocation", plr.rgAttr[rsUseLeftover]);
        Require(szRace + ".ResearchPreferences.allocation_percent", plr.pctResearch);
        Require(szRace + ".flag_mask", plr.grbitAttr);
        Require(szRace + (i == 6 ? ".placeholder_prt_id" : ".prt_id"), plr.rgAttr[rsMajorAdv]);
        for (j = 0; j < 7; ++j) Require(szRace + ".Economy." + rgszEconomy[j], plr.rgAttr[j]);
        for (j = 0; j < 6; ++j) Require(szRace + ".ResearchPreferences.costs." + rgszTech[j], plr.rgAttr[8 + j]);
        for (j = 0; j < 3; ++j) {
            Require(szRace + ".Habitability." + rgszAxis[j] + ".lower", plr.rgEnvVarMin[j]);
            Require(szRace + ".Habitability." + rgszAxis[j] + ".preferred", plr.rgEnvVar[j]);
            Require(szRace + ".Habitability." + rgszAxis[j] + ".upper", plr.rgEnvVarMax[j]);
        }
        if (i < 6) {
            Require("native_pricing_" + std::to_string(i) + ".innate", LInnateRaceHabitability(app, &plr));
            Require("native_pricing_" + std::to_string(i) + ".advantage_points", CAdvantagePoints(app, &plr));
        }
        std::printf("}\nNativePreset%d native_preset_%d = {}\n", i, i);
    }
}

static void Colonies(APPSESSION &app, int iOnly) {
    static PLANET pl;
    static SHDEF rgshdef[5];
    static constexpr std::array<std::array<std::int16_t, 3>, 8> rgrgEnv = {{{50,50,50}, {0,0,0}, {100,100,100}, {33,58,33}, {15,50,85}, {10,35,13}, {27,73,61}, {85,15,50}}};
    static constexpr std::int32_t rglPop[] = {0, 1, 250, 2500, 10000, 15000, 40000};
    std::int16_t i;
    std::int16_t j;
    std::int16_t k;
    std::int16_t pct;
    std::int16_t delta;
    std::int32_t lChange;
    app.state.lpPlanets = &pl;
    app.state.cPlanet = 1;
    app.state.game.cPlanMax = 1;
    app.state.idPlayer = iplrNone;
    app.state.rglpshdefSB[0] = rgshdef;
    for (i = 0; i < 5; ++i) rgshdef[i].hul.ihuldef = static_cast<HulDef>(ihuldefOrbitalFort + i);
    for (i = 0; i < 6; ++i) {
        if (iOnly >= 0 && i != iOnly) continue;
        app.state.rgplr[0] = vrgplrDef[i];
        for (j = 0; j < 8; ++j) {
            pl = {};
            pl.id = 0;
            pl.iPlayer = 0;
            pl.fStarbase = 1;
            pl.isb = 2;
            for (k = 0; k < 3; ++k) pl.rgEnvVar[k] = static_cast<std::int8_t>(rgrgEnv[j][k]);
            pct = PctPlanetDesirability(app, &pl, 0);
            std::printf("object type NativeHab%d_%d = {\n", i, j);
            Require("(PlanetHabitat { race = " + std::string(rgszRace[i]) + "; gravity = " + std::to_string(rgrgEnv[j][0]) + "; temperature = " + std::to_string(rgrgEnv[j][1]) + "; radiation = " + std::to_string(rgrgEnv[j][2]) + "; }).percent", pct);
            std::printf("}\nNativeHab%d_%d native_hab_%d_%d = {}\n", i, j, i, j);
            for (k = 0; k < 7; ++k) {
                pl.rgwtMin[3] = rglPop[k];
                pl.cFactories = 321;
                pl.cMines = 432;
                pl.cDefenses = 30;
                pl.iDeltaPop = 37;
                lChange = ChgPopFromPlanet(app, &pl, 1);
                delta = pl.iDeltaPop;
                pl.rgwtMin[3] = rglPop[k];
                const std::string szId = "native_colony_" + std::to_string(i) + "_" + std::to_string(j) + "_" + std::to_string(k);
                std::printf("let %s: Colony = Colony { race = %s; habitat = %d; population = %ld; factories = 321; mines = 432; defenses = 30; delta_remainder = 37; };\n", szId.c_str(), rgszRace[i], pct, static_cast<long>(rglPop[k]));
                std::printf("object type NativeColony%d_%d_%d = {\n", i, j, k);
                Require(szId + ".capacity", CalcPlanetMaxPop(app, 0, 0));
                Require(szId + ".operating_factories", CFactoriesOperating(app, &pl));
                Require(szId + ".operating_mines", CMinesOperating(app, &pl));
                Require(szId + ".resources", CResourcesAtPlanet(app, &pl, 0));
                Require(szId + ".population_change", lChange);
                Require(szId + ".next_remainder", delta);
                std::printf("}\nNativeColony%d_%d_%d native_check_colony_%d_%d_%d = {}\n", i, j, k, i, j, k);
            }
        }
    }
}

static void Equipment(APPSESSION &app, int iOnly) {
    static constexpr std::array<std::pair<HullSlotType, std::int16_t>, 16> rghst = {{{hstHull, ihuldefOrbitalFort}, {hstSBHull, isbhullCount}, {hstEngine, iengineCount}, {hstArmor, iarmorCount}, {hstScanner, iscannerCount}, {hstShield, ishieldCount}, {hstSpecialE, ispecialECount}, {hstSpecialM, ispecialMCount}, {hstMines, iminesCount}, {hstMining, iminingCount}, {hstPlanetary, iplanetaryCount}, {hstTerra, iterraCount}, {hstBomb, ibombCount}, {hstTorp, itorpCount}, {hstBeam, ibeamCount}, {hstSpecialSB, ispecialSBCount}}};
    static constexpr const char *rgszCategory[] = {"hull", "base_hull", "engine", "armor", "scanner", "shield", "electrical", "mechanical", "mine", "mining", "planetary", "terra", "bomb", "torpedo", "beam", "base_special"};
    std::int16_t i;
    std::int16_t j;
    std::int16_t k;
    std::int16_t m;
    PART part{};
    app.state.idPlayer = 0;
    for (i = 0; i < 10; ++i) {
        if (iOnly >= 0 && i != iOnly) continue;
        for (m = 0; m < 2; ++m) {
            app.state.rgplr[0] = vrgplrDef[0];
            app.state.rgplr[0].rgAttr[rsMajorAdv] = static_cast<std::int8_t>(i);
            app.state.rgplr[0].grbitAttr = static_cast<RaceTraitBits>(m == 0 ? 0 : 0x3fff);
            std::fill_n(app.state.rgplr[0].rgTech, 6, 26);
            std::printf("Race native_race_%d_%d = { name=\"Native\"; plural_name=\"Natives\"; %s %s ", i, m, pszDefaultChoices, rgszPrt[i]);
            if (m != 0) for (j = 0; j < 14; ++j) std::printf(" lrt_%s;", rgszLrt[j]);
            std::printf(" }\nobject type NativeEquipment%d_%d = {\n", i, m);
            for (j = 0; j < static_cast<std::int16_t>(rghst.size()); ++j) {
                for (k = 0; k < rghst[j].second; ++k) {
                    part.hs = {};
                    part.hs.grhst = rghst[j].first;
                    part.hs.iItem = k;
                    if (FLookupPart(app, &part) == mdPartAvailAvailable) {
                        std::printf("  require List.length (query { from p: CatalogPart where p.category = \"%s\" where p.index = %d where (EquipmentEligibility { race = native_race_%d_%d; part = p; technology = {energy=26;weapons=26;propulsion=26;construction=26;electronics=26;biotechnology=26;}; }).eligible select p }) = 1;\n", rgszCategory[j], k, i, m);
                    } else {
                        std::printf("  require List.length (query { from p: CatalogPart where p.category = \"%s\" where p.index = %d where (EquipmentEligibility { race = native_race_%d_%d; part = p; technology = {energy=26;weapons=26;propulsion=26;construction=26;electronics=26;biotechnology=26;}; }).eligible select p }) = 0;\n", rgszCategory[j], k, i, m);
                    }
                }
            }
            std::printf("}\nNativeEquipment%d_%d native_equipment_%d_%d = {}\n", i, m, i, m);
        }
    }
}

static void Costs(APPSESSION &app, int iOnly) {
    struct SAMPLE { HullSlotType hst; std::int16_t iItem; const char *pszSymbol; };
    static constexpr SAMPLE rgsample[] = {
        {hstEngine, iengineQuickJump5, "iengineQuickJump5"},
        {hstEngine, iengineFuelMizer, "iengineFuelMizer"},
        {hstArmor, iarmorCrobmnium, "iarmorCrobmnium"},
        {hstShield, ishieldCowHideShield, "ishieldCowHideShield"},
        {hstBeam, ibeamXRayLaser, "ibeamXRayLaser"},
        {hstTorp, itorpAlphaTorpedo, "itorpAlphaTorpedo"},
        {hstBomb, ibombLadyFingerBomb, "ibombLadyFingerBomb"},
        {hstSpecialSB, ispecialSBStargate100250, "ispecialSBStargate100250"},
        {hstSBHull, isbhullSpaceStation, "isbhullSpaceStation"},
    };
    static constexpr const char *rgszChannel[] = {"ironium", "boranium", "germanium", "resources"};
    std::int16_t i;
    std::int16_t j;
    std::int16_t k;
    std::int16_t m;
    std::int16_t t;
    std::int16_t c;
    PART part{};
    std::array<std::uint16_t, 4> rgCost{};
    app.state.idPlayer = 0;
    for (i = 0; i < 10; ++i) {
        if (iOnly >= 0 && i != iOnly) continue;
        for (m = 0; m < 2; ++m) {
            app.state.rgplr[0] = vrgplrDef[0];
            app.state.rgplr[0].rgAttr[rsMajorAdv] = static_cast<std::int8_t>(i);
            app.state.rgplr[0].grbitAttr = static_cast<RaceTraitBits>(m == 0 ? 0 : 0x3fff);
            std::printf("Race native_cost_race_%d_%d = { name=\"Native\"; plural_name=\"Natives\"; %s %s ", i, m, pszDefaultChoices, rgszPrt[i]);
            if (m != 0) for (j = 0; j < 14; ++j) std::printf(" lrt_%s;", rgszLrt[j]);
            std::printf(" }\n");
            for (j = 0; j < static_cast<std::int16_t>(std::size(rgsample)); ++j) {
                part.hs = {};
                part.hs.grhst = rgsample[j].hst;
                part.hs.iItem = rgsample[j].iItem;
                FLookupPart(app, &part);
                for (t = 0; t < 2; ++t) {
                    for (k = 0; k < 6; ++k) app.state.rgplr[0].rgTech[k] = part.pcom().rgTech()[k] + 3 * t;
                    GetTruePartCost(app, 0, &part, rgCost);
                    std::printf("let native_cost_%d_%d_%d_%d = EquipmentCost { eligibility = EquipmentEligibility { race = native_cost_race_%d_%d; part = part_%s; technology = {", i, m, j, t, i, m, rgsample[j].pszSymbol);
                    std::printf("energy=%d;weapons=%d;propulsion=%d;construction=%d;electronics=%d;biotechnology=%d;}; }; };\n", app.state.rgplr[0].rgTech[0], app.state.rgplr[0].rgTech[1], app.state.rgplr[0].rgTech[2], app.state.rgplr[0].rgTech[3], app.state.rgplr[0].rgTech[4], app.state.rgplr[0].rgTech[5]);
                    std::printf("object type NativeCost%d_%d_%d_%d = {\n", i,m,j,t);
                    for (c = 0; c < 4; ++c) Require("native_cost_" + std::to_string(i) + "_" + std::to_string(m) + "_" + std::to_string(j) + "_" + std::to_string(t) + "." + rgszChannel[c], rgCost[c]);
                    std::printf("}\nNativeCost%d_%d_%d_%d native_cost_check_%d_%d_%d_%d = {}\n",i,m,j,t,i,m,j,t);
                }
            }
        }
    }
}

static void Gates(APPSESSION &app) {
    static constexpr std::int16_t rgDistance[] = {0, 249, 250, 251, 1249, 1250, 1251};
    static constexpr std::int16_t rgMass[] = {0, 99, 100, 101, 499, 500, 501};
    std::int16_t i;
    std::int16_t j;
    STARGATEDAMAGERESULT result;
    app.state.idPlayer = 0;
    app.state.rgplr[0] = vrgplrDef[0];
    for (i = 0; i < 7; ++i) {
        for (j = 0; j < 7; ++j) {
            result = MdCalcStargateDamage(app, ispecialSBStargate100250, ispecialSBStargate100250, rgDistance[i], rgMass[j]);
            std::printf("let native_gate_%d_%d = GateJump { race=humanoid; source_range=250; source_mass=100; destination_mass=100; distance=%d; empty_mass=%d; };\n",i,j,rgDistance[i],rgMass[j]);
            std::printf("object type NativeGate%d_%d = {\n",i,j);
            Require("native_gate_" + std::to_string(i) + "_" + std::to_string(j) + ".status", result.status);
            if (result.pctDmg.has_value()) Require("native_gate_" + std::to_string(i) + "_" + std::to_string(j) + ".damage_percent", *result.pctDmg);
            std::printf("}\nNativeGate%d_%d native_gate_check_%d_%d = {}\n",i,j,i,j);
        }
    }
}

static void Boundary(APPSESSION &app) {
    PLAYER plr = vrgplrDef[0];
    static constexpr std::int8_t rgLower[] = {31,61,9};
    static constexpr std::int8_t rgUpper[] = {71,97,73};
    std::int16_t i;
    plr.pctIdealGrowth = 10;
    for (i = 0; i < 3; ++i) {
        plr.rgEnvVarMin[i] = rgLower[i];
        plr.rgEnvVarMax[i] = rgUpper[i];
        plr.rgEnvVar[i] = (rgLower[i] + rgUpper[i]) / 2;
    }
    std::printf("Race native_boundary_race = {name=\"Boundary\";plural_name=\"Boundaries\"; portrait(1) gravity(31,71) temperature(61,97) radiation(9,73) growth(10) population_efficiency(1000) factory_output(10) factory_cost(10) factory_operation(10) mine_output(10) mine_cost(5) mine_operation(10) energy_research(standard) weapons_research(standard) propulsion_research(standard) construction_research(standard) electronics_research(standard) biotechnology_research(standard) research_allocation(15) leftover_points(surface_minerals) standard_capacity jack_of_all_trades }\nlet native_boundary_pricing = WizardPricing {race=native_boundary_race;};\nobject type NativeBoundary = {\n");
    Require("native_boundary_pricing.innate",LInnateRaceHabitability(app,&plr));
    Require("native_boundary_pricing.advantage_points",CAdvantagePoints(app,&plr));
    std::printf("}\nNativeBoundary native_boundary = {}\n");
}

int main(int argc, char **argv) {
    APPSESSION &app = host::app;
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    if (!FStarsTestInit(app)) return 1;
    std::printf("import stars.predefined\nimport stars.rules\n");
    if (argc > 1 && std::string(argv[1]) == "equipment") Equipment(app, argc > 2 ? std::stoi(argv[2]) : -1);
    else if (argc > 1 && std::string(argv[1]) == "colonies") Colonies(app, argc > 2 ? std::stoi(argv[2]) : -1);
    else if (argc > 1 && std::string(argv[1]) == "costs") Costs(app, argc > 2 ? std::stoi(argv[2]) : -1);
    else if (argc > 1 && std::string(argv[1]) == "gates") Gates(app);
    else if (argc > 1 && std::string(argv[1]) == "boundary") Boundary(app);
    else Presets(app, argc > 2 ? std::stoi(argv[2]) : -1);
    return 0;
}
