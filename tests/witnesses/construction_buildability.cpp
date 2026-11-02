#include "construction_buildability_fixture.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include <limits>

int main(int argc, char **argv) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    const unsigned rgcRows[] = {iengineCount, iscannerCount,   ishieldCount, iarmorCount,    ibeamCount,     itorpCount,  ibombCount,         iminingCount,
                                iminesCount,  ispecialSBCount, isbhullCount, ispecialECount, ispecialMCount, iterraCount, ihuldefOrbitalFort, iplanetaryCount};
    CONSTRUCTIONBUILDFIXTURE fixture(app);
    assert(argc == 3);
    FILE *file = fopen(argv[2], argv[1][0] == 'e' ? "wb" : "rb");
    assert(file);
    if (argv[1][0] == 'e') {
        for (unsigned k = 0; k < 16; k++) {
            for (unsigned item = 0; item < rgcRows[k]; item++) {
                int32_t              data[9];
                NATIVEPARTCOMMONVIEW p = LpcomRow(k, item);
                data[0] = k;
                data[1] = item;
                for (int f = 0; f < 6; f++)
                    data[2 + f] = p.rgTech()[f];
                data[8] = k == 15 ? rgplanetary[item].grAbility : 0;
                assert(fwrite(data, sizeof(data), 1, file) == 1);
            }
        }
        fclose(file);
        return 0;
    }
    int32_t       data[262];
    unsigned long cases = 0;
    while (fread(data, sizeof(data), 1, file) == 1) {
        reset(app);
        for (int p = 0; p < 16; p++) {
            app.state.rglpshdef[p] = NULL;
            app.state.rglpshdefSB[p] = NULL;
        }
        int item = data[0], base = item >= 16, index = base ? item - 16 : item, owner = data[2];
        planet.fStarbase = data[1];
        planet.iPlayer = (int16_t)owner;
        planet.isb = data[21];
        if (owner >= 0 && owner < 16) {
            app.state.rglpshdef[owner] = data[3] ? ships : NULL;
            app.state.rglpshdefSB[owner] = data[4] ? bases : NULL;
            app.state.rgplr[owner].rgAttr[rsMajorAdv] = (int8_t)data[12];
            app.state.rgplr[owner].grbitAttr = static_cast<RaceTraitBits>((uint32_t)data[13]);
            app.state.rgplr[owner].grbitTrader = static_cast<GrbitTrader>((uint16_t)data[14]);
            app.state.rgplr[owner].SetITechNow(data[16]);
            for (int f = 0; f < 6; f++)
                app.state.rgplr[owner].rgTech[f] = (int8_t)data[15];
            app.state.rgplr[owner].cFleet = data[18] ? 512 : 0;
        }
        for (int i = 0; i < 16; i++) {
            ships[i].cBuilt = 100 + i;
            ships[i].cExist = 50 + i;
        }
        for (int i = 0; i < 10; i++) {
            bases[i].cBuilt = 200 + i;
            bases[i].cExist = 70 + i;
        }
        bases[data[21]].hul.ihuldef = static_cast<HulDef>((uint16_t)data[22]);
        if (index >= 0 && index < (base ? 10 : 16)) {
            SHDEF *d = (base ? bases : ships) + index;
            d->fFree = data[5];
            d->hul.ihuldef = static_cast<HulDef>((uint16_t)data[6]);
            d->hul.chs = (uint8_t)data[7];
            d->hul.wtCargoMax = (uint16_t)data[17];
            for (int at = 0; at < 16; at++)
                if (data[11] == -1 || data[11] == at) {
                    d->hul.rghs[at].grhst = static_cast<HullSlotType>((uint16_t)data[8]);
                    d->hul.rghs[at].iItem = data[9];
                    d->hul.rghs[at].cItem = data[10];
                }
        }
        app.state.idPlayer = (int16_t)data[19];
        cQueries = cGateLookups = mdCheckResult = fInCheck = 0;
        memset(rgQueries, -1, sizeof(rgQueries));
        memset(rgGateRequests, -1, sizeof(rgGateRequests));
        SHDEF  shipExpected[16], baseExpected[10];
        PLAYER playerExpected[16];
        GAME   gameExpected;
        PLANET planetExpected;
        memcpy(shipExpected, ships, sizeof(ships));
        memcpy(baseExpected, bases, sizeof(bases));
        memcpy(playerExpected, app.state.rgplr, sizeof(app.state.rgplr));
        memcpy(&gameExpected, &app.state.game, sizeof(app.state.game));
        memcpy(&planetExpected, &planet, sizeof(planet));
        int result = call(app, 2, item, data[20]);
        int stage = canCalls && mdCheckResult ? (base ? 2 : 1) : 0;
        int actual[] = {stage, result, canCalls, cGateLookups, cQueries, messages, catalogCalls, app.state.idPlayer, newFleets};
        for (int i = 0; i < 9; i++) {
            if (actual[i] != data[23 + i]) {
                fprintf(stderr, "case %lu item %d owner %d hull %d: field %d got %d expected %d\n", cases, item, owner, data[6], i, actual[i], data[23 + i]);
                return 1;
            }
            checks++;
        }
        for (int at = 0; at < 17; at++)
            for (int f = 0; f < 6; f++)
                CHECK(rgGateRequests[at][f] == data[32 + 6 * at + f]);
        for (int at = 0; at < 128; at++)
            CHECK(rgQueries[at] == data[134 + at]);
        if (result && base) {
            if (data[1])
                baseExpected[data[21]].cExist--;
            baseExpected[index].cBuilt++;
            baseExpected[index].cExist++;
            planetExpected.fStarbase = 1;
            planetExpected.isb = index;
            int      raw = (int16_t)(uint16_t)data[6];
            unsigned cargo = raw >= 32 ? rghuldefSB[raw - 32].hul.wtCargoMax : rghuldef[raw].hul.wtCargoMax;
            CHECK(messageIds[0] == idmHasBuiltNew + (data[17] == 0 ? 0 : data[17] == 65535 ? 2 : 1));
            CHECK(messageArgs[0][2] == (int16_t)cargo);
            CHECK(warpCalls == 1 && !packetKills);
            CHECK(shipKills == (data[1] && (int16_t)baseExpected[data[21]].hul.ihuldef > (int16_t)(uint16_t)data[6]));
        } else if (result) {
            shipExpected[index].cBuilt += (uint32_t)data[20];
            shipExpected[index].cExist += (uint32_t)data[20];
            playerExpected[owner].cFleet++;
            CHECK(fleetBodies[3].rgcsh[index] == data[20] && fleetBodies[3].iPlayer == owner && fleetBodies[3].rgwtMin[4] == 12345);
            CHECK(defaults == 1 && !routes);
        } else if (messages) {
            CHECK(messageIds[0] == (stage == 1 ? idmStarbaseBuiltNewShipSTypeLost : idmStarbaseFailedBuildNewShipTypeBecause));
        }
        CHECK(memcmp(shipExpected, ships, sizeof(ships)) == 0);
        CHECK(memcmp(baseExpected, bases, sizeof(bases)) == 0);
        CHECK(memcmp(playerExpected, app.state.rgplr, sizeof(app.state.rgplr)) == 0);
        CHECK(memcmp(&gameExpected, &app.state.game, sizeof(app.state.game)) == 0);
        CHECK(memcmp(&planetExpected, &planet, sizeof(planet)) == 0);
        cases++;
    }
    CHECK(feof(file));
    fclose(file);
    // Full Produce -> charged construction -> actual eligibility -> exact fleet limit failure.
    reset(app);
    planet.id = 0;
    app.state.rgptPlan[0] = POINT16{10, 20};
    app.state.game.cPlayer = 1;
    app.state.game.cPlanMax = 1;
    planet.fStarbase = 1;
    bases[0].hul.ihuldef = static_cast<HulDef>(32);
    app.state.rgplr[0].rgAttr[rsMajorAdv] = raNone;
    app.state.rgplr[0].grbitTrader = static_cast<GrbitTrader>(std::numeric_limits<std::uint16_t>::max());
    for (int f = 0; f < 6; f++)
        app.state.rgplr[0].rgTech[f] = 26;
    app.state.rgplr[0].cFleet = 512;
    app.state.cFleet = 512;
    ships[0].cBuilt = ships[0].cExist = 512;
    for (int i = 0; i < 512; i++) {
        memset(&fleetBodies[i], 0, sizeof(FLEET));
        memset(orderStorage[i].data, 0, sizeof(orderStorage[i].data));
        fleetSlots[i] = &fleetBodies[i];
        fleetBodies[i].SetIfl(i);
        fleetBodies[i].iPlayer = 0;
        fleetBodies[i].SetIplr(0);
        fleetBodies[i].idPlanet = idPlanetDeepSpace;
        fleetBodies[i].rgcsh[0] = 1;
        fleetBodies[i].pt = POINT16{99, 99};
        fleetBodies[i].lpplord = LpStartPl<PLORD, ORDER>(orderStorage[i].data, sizeof(orderStorage[i].data));
        fleetBodies[i].cord = 1;
        fleetBodies[i].lpplord->iordMac = 1;
        fleetBodies[i].lpplord->rgord()[0].pt = fleetBodies[i].pt;
    }
    cQueries = cGateLookups = mdCheckResult = fInCheck = 0;
    memset(rgQueries, -1, sizeof(rgQueries));
    memset(rgGateRequests, -1, sizeof(rgGateRequests));
    planet.lpplprod = LpplAlloc<PLPROD>(app, 4, 1, htOrd);
    planet.lpplprod->iprodMac = 1;
    planet.lpplprod->rgprod()[0] = row(1, 0, 2, 0);
    Produce(app);
    CHECK(canCalls == 1 && mdCheckResult == 1 && !newFleets && !planet.lpplprod && ships[0].cBuilt == 512 && ships[0].cExist == 512 &&
          app.state.rgplr[0].lResLastYear == 90 && planet.rgwtMin[0] == 10);
    reset(app);
    printf("Construction actual eligibility integration: %lu cases plus post-charge Produce/fleet-limit case; %d assertions passed\n", cases, checks);
    return 0;
}
