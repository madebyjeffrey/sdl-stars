#include "construction_fixture.h"
static int16_t s16(int n) { return (int16_t)(uint16_t)n; }
static int     damageExpected(int prior, int add, unsigned word, unsigned hp, unsigned *out) {
    int share = word % 128, damage = word / 128;
    if (!prior || !damage) {
        *out = 0;
        return 1;
    }
    int affected = s16(share * prior / 100);
    if (!affected)
        affected = 1;
    int64_t product = (int64_t)(hp * (unsigned)damage / 10) * affected;
    if (product < INT32_MIN || product > INT32_MAX)
        return 0;
    int original = (int)(product / 50), total = s16(prior + add);
    if (!total)
        return 0;
    unsigned newShare = (unsigned)((affected * 100) / total) & 127;
    if (!newShare)
        newShare = 1;
    int next = s16((int)newShare * total / 100);
    if (!next)
        next = 1;
    int64_t scaled = (int64_t)original * 5;
    if (scaled < INT32_MIN || scaled > INT32_MAX || !hp)
        return 0;
    scaled = scaled / next * 100;
    if (scaled < INT32_MIN || scaled > INT32_MAX)
        return 0;
    *out = newShare + 128 * ((unsigned)(scaled / (int)hp) & 511);
    return 1;
}
int main(void) {
    CONSTRUCTIONFIXTURE fixture;
    static_assert(sizeof(PROD) == 4 && sizeof(DV) == 2 && sizeof(THPACK) == 10 && sizeof(THING) == 18, "native object layout");
    reset();
    CHECK(!call(0, 0, 1) && !call(1, -1, 1) && !call(1, 28, 1));
    for (int raw = -32768; raw <= 32767; raw++)
        for (unsigned which = 0; which < 6; which++) {
            reset();
            unsigned items[] = {0, 8, 1, 7, 2, 9};
            unsigned item = items[which];
            cap = (int16_t)raw;
            removed = 3;
            unsigned before = which < 2 ? 2 : which < 4 ? 1 : 3;
            planet.cMines = 2;
            planet.cFactories = 1;
            planet.cDefenses = 3;
            int allowed = s16(raw - (int)before), n = 5 >= allowed ? allowed : 5;
            int accepted = n > 0;
            CHECK(call(1, item, 5) == accepted);
            unsigned after = which < 2 ? planet.cMines : which < 4 ? planet.cFactories : planet.cDefenses;
            CHECK(after == (accepted ? (before + (unsigned)n) % 4096 : before));
            CHECK(messages == accepted);
            if (accepted) {
                MessageId id = which < 2 ? idmHaveBuiltMine : which < 4 ? idmHaveBuiltFactory : idmHaveBuiltDefenseOutpost;
                int       sum = s16(n + 3);
                CHECK(messageIds[0] == id + (sum > 1));
                CHECK(sum <= 1 || messageArgs[0][0] == sum);
            }
        }
    reset();
    planet.cMines = 4095;
    cap = -32768;
    CHECK(call(1, 0, 1) && planet.cMines == 0);
    reset();
    cap = 32767;
    removed = 32767;
    CHECK(call(1, 0, 32767) && planet.cMines == 4095 && messageIds[0] == idmHaveBuiltMine);
    for (unsigned item = 0; item < 3; item++) {
        reset();
        unsigned ids[] = {3, 10, 11};
        PLANET   before = planet;
        CHECK(call(1, ids[item], -1));
        CHECK(memcmp(&before, &planet, sizeof(planet)) == 0 && !messages);
    }
    for (int choice = -3; choice <= 3; choice++)
        for (int start = -128; start <= 127; start++) {
            reset();
            terraformChoice = (int16_t)choice;
            for (int i = 0; i < 3; i++)
                planet.rgEnvVar[i] = (int8_t)start;
            CHECK(call(1, 12, 1));
            CHECK(terraformCalls == 1 && messages == (choice != 0));
            for (int i = 0; i < 3; i++) {
                int expected = start;
                if (choice && i == abs(choice) - 1) {
                    expected += choice < 0 ? -1 : 1;
                    if (expected < 1)
                        expected = 1;
                    if (expected > 99)
                        expected = 99;
                }
                CHECK(planet.rgEnvVar[i] == expected);
            }
        }
    reset();
    terraformChoice = 0;
    CHECK(call(1, 4, -1) && terraformCalls == 65535);
    reset();
    terraformChoice = 4;
    int escaped = 0;
    try {
        call(1, 12, 1);
        CHECK(0);
    } catch (const CONSTRUCTIONESCAPE &) {
        escaped = 1;
    }
    CHECK(escaped == 1 && !messages);
    for (int ar = 0; ar < 2; ar++) {
        reset();
        rgplr[0].rgAttr[rsMajorAdv] = ar ? raMacintosh : raCheapCol;
        planet.cMines = 3;
        planet.cFactories = 4;
        planet.cDefenses = 5;
        planet.iScanner = 6;
        CHECK(call(1, 13, 10) && randomCalls == 12 && messages == 3);
        CHECK(planet.cMines == (unsigned)(ar ? 3 : 0) && planet.iScanner == (unsigned)(ar ? 6 : 31));
        for (int i = 0; i < 3; i++) {
            CHECK(planet.rgwtMin[i] == 0);
            CHECK(planet.rgEnvVar[i] == randomValues[4 * i] + randomValues[4 * i + 1] + 1 && planet.rgEnvVarOrig[i] == planet.rgEnvVar[i]);
            CHECK(planet.rgMinConc[i] == randomValues[4 * i + 2] + randomValues[4 * i + 3] + 25);
            CHECK(randomBounds[4 * i] == 50 && randomBounds[4 * i + 1] == 50 && randomBounds[4 * i + 2] == 40 && randomBounds[4 * i + 3] == 40);
        }
        CHECK(resources[0] == 10 && resources[3] == 40 && (lRandSeed1 != 7 || lRandSeed2 != 11));
        CHECK(messageScanner[0] == 6 && messageScanner[1] == 6 && messageScanner[2] == 6);
    }
    for (unsigned part = 0; part < 256; part++) {
        reset();
        partIndex = (int16_t)part;
        planet.iScanner = 7;
        CHECK(call(1, 27, 1));
        CHECK(planet.iScanner == part % 32 && idPlayer == -1 && messageArgs[0][2] == (int)part && messageScanner[0] == 7);
    }
    for (unsigned item = 18; item < 27; item++) {
        reset();
        CHECK(call(1, item, 1) && planet.iScanner == item - 18 && idPlayer == 3);
    }
    const int16_t abilities[] = {-32768, -1, 0, 1, 3, 10, 16, 32767};
    const int16_t quantities[] = {-1, 1, 1000};
    const int     primaries[] = {0, 6, 7, 8};
    const int     packets[] = {6, 14, 15, 16, 17};
    for (unsigned t = 0; t < 4; t++)
        for (unsigned w = 0; w < 8; w++)
            for (int two = 0; two < 2; two++)
                for (int configured = 0; configured < 16; configured++)
                    for (int destination = 0; destination < 2; destination++)
                        for (unsigned b = 0; b < 3; b++)
                            for (unsigned item = 0; item < 5; item++) {
                                reset();
                                rgplr[0].rgAttr[rsMajorAdv] = (int8_t)primaries[t];
                                warpValues[0] = abilities[w];
                                twoValues[0] = (int16_t)two;
                                planet.iWarpFling = configured;
                                planet.idFling = destination ? 8 : 0;
                                int accepted = abilities[w] != 0 && destination;
                                CHECK(call(1, packets[item], quantities[b]) == accepted);
                                if (!accepted) {
                                    CHECK(cThing == 0);
                                    continue;
                                }
                                int mixed = packets[item] == 6 || packets[item] == 17;
                                int size = mixed ? (primaries[t] == 6 ? 25 : 40) : (primaries[t] == 6 ? 70 : 100);
                                int quantity = size * quantities[b];
                                if (quantity > 32760)
                                    quantity = 32760;
                                quantity = s16(quantity);
                                int asked = configured + 4;
                                if (asked < 5 || asked > abilities[w] + 3)
                                    asked = s16(abilities[w] + two);
                                unsigned decay = (uint16_t)(asked <= abilities[w] + two ? 0 : asked - abilities[w] - two);
                                if (primaries[t] == 7 && s16((int)decay) < 3)
                                    decay = (uint16_t)(decay + 1);
                                CHECK(cThing == 1 && lpThings[0].thp().iWarp() == ((unsigned)s16(asked - 4) & 15) &&
                                      lpThings[0].thp().iDecayRate() == (decay & 3) && lpThings[0].thp().idPlanet() == 7);
                                unsigned weight = 0;
                                for (int i = 0; i < 3; i++) {
                                    int ore = mixed || i == packets[item] - 14 ? quantity : 0;
                                    CHECK(lpThings[0].thp().rgwtMin(i) == ore);
                                    weight = (weight + s16(ore + 9) / 10) & 16383;
                                }
                                CHECK(lpThings[0].thp().wtMax() == weight);
                            }
    // Packet quantity saturation and weight's signed-16 narrowing.
    reset();
    CHECK(call(1, 17, 1000) && cThing == 1);
    CHECK(lpThings[0].thp().rgwtMin(0) == 32760 && lpThings[0].thp().wtMax() == 6556);
    reset();
    warpValues[0] = 0;
    CHECK(!call(1, 6, 1) && messageIds[0] == idmMineralPacketFormedHasDisintegratedBecausePlanet);
    reset();
    planet.idFling = 0;
    CHECK(!call(1, 6, 1) && messageIds[0] == idmMineralPacketFormedHasDisintegratedBecauseDidnt);
    reset();
    THING *p = LpthNew(0, ithMineralPacket);
    p->pt().Set(rgptPlan[5]);
    p->thp().SetIWarp(6);
    p->thp().SetIdPlanet(7);
    p->thp().SetIDecayRate(0);
    p->thp().SetWtMax(1629);
    for (int i = 0; i < 3; i++)
        p->thp().SetRgwtMin(i, 32760);
    CHECK(call(1, 17, 1) && cThing == 1);
    CHECK(p->thp().rgwtMin(0) == 32760 && p->thp().wtMax() == 6556 && messageIds[0] == idmHasProducedMineralPacketWhichHasCombined);
    p->thp().SetWtMax(1630);
    CHECK(call(1, 17, 1) && cThing == 2);
    reset();
    cThing = cThingAlloc = 512;
    lpThings = static_cast<THING *>(LpAlloc((uint16_t)(512 * sizeof(THING)), htThings));
    for (int i = 0; i < 512; i++) {
        lpThings[i].SetIth(ithMineralPacket);
        lpThings[i].SetIplr(0);
        lpThings[i].SetId(i);
        lpThings[i].pt().SetX(99);
    }
    CHECK(call(1, 17, 1) && cThing == 512 && messageIds[0] == idmHasOrdersBuildMineralPacketEitherDoesnt);
    // Fleet limit uses first-order point, first null/greater owner and strict
    // 32766-built > prior count, independently of fleet point or dead flag.
    reset();
    planet.fStarbase = 1;
    rgplr[0].cFleet = 512;
    CHECK(!call(2, 0, 1) && !newFleets && messageIds[0] == idmStarbaseBuiltNewShipSTypeLost);
    reset();
    planet.fStarbase = 1;
    rgplr[0].cFleet = 513;
    CHECK(call(2, 0, 1) && newFleets == 1 && defaults == 1 && fleetBodies[3].rgwtMin[4] == 12345);
    reset();
    CHECK(!call(2, 0, 1) && !canCalls && !messages);
    for (int base = 0; base < 2; base++)
        for (int isFree = 0; isFree < 2; isFree++)
            for (int available = 0; available < 2; available++) {
                reset();
                planet.fStarbase = 1;
                canBuild = available;
                if (base)
                    bases[0].fFree = isFree;
                else
                    ships[0].fFree = isFree;
                CHECK(call(2, base ? 16 : 0, 1) == (!isFree && available));
                CHECK(canCalls == !isFree);
                if (isFree || !available)
                    CHECK(messages == !base);
            }
    const uint16_t cargos[] = {0, 1, 65535};
    for (unsigned cargo = 0; cargo < 3; cargo++) {
        reset();
        bases[0].hul.wtCargoMax = cargos[cargo];
        CHECK(call(2, 16, 1));
        CHECK(messageIds[0] == idmHasBuiltNew + (cargo == 0 ? 0 : cargo == 2 ? 2 : 1) && messageArgs[0][2] == 123 && catalogCalls == 1);
    }
    reset();
    planet.fStarbase = 1;
    rgplr[0].cFleet = 512;
    cFleet = 1;
    fleetSlots[0] = NULL;
    CHECK(!call(2, 0, 1));
    reset();
    planet.fStarbase = 1;
    rgplr[0].cFleet = 512;
    cFleet = 2;
    fleetBodies[0].iPlayer = 1;
    fleetBodies[1].lpplord->rgord()[0].pt = rgptPlan[5];
    CHECK(!call(2, 0, 1));
    reset();
    planet.fStarbase = 1;
    rgplr[0].cFleet = 512;
    cFleet = 1;
    fleetBodies[0].lpplord->rgord()[0].pt = rgptPlan[5];
    fleetBodies[0].rgcsh[0] = 32765;
    CHECK(!call(2, 0, 1));
    fleetBodies[0].rgcsh[0] = 32764;
    fleetBodies[0].fDead = 1;
    CHECK(call(2, 0, 1) && fleetBodies[0].rgcsh[0] == 32765);
    for (unsigned word = 0; word < 65536; word++) {
        reset();
        planet.fStarbase = 1;
        rgplr[0].cFleet = 512;
        cFleet = 1;
        fleetBodies[0].lpplord->rgord()[0].pt = rgptPlan[5];
        fleetBodies[0].rgcsh[0] = 10;
        fleetBodies[0].rgdv[0].dp = (uint16_t)word;
        ships[0].hul.dp = 100;
        unsigned expected;
        CHECK(damageExpected(10, 1, word, 100, &expected));
        CHECK(call(2, 0, 1));
        CHECK(fleetBodies[0].rgdv[0].dp == expected && fleetBodies[0].rgcsh[0] == 11 && ships[0].cBuilt == 1 && ships[0].cExist == 1);
    }
    reset();
    fleetBodies[0].rgcsh[0] = 32767;
    ships[0].cBuilt = ships[0].cExist = UINT32_MAX;
    CreateShip(0, &fleetBodies[0], 0, 1);
    CHECK(fleetBodies[0].rgcsh[0] == -32768 && ships[0].cBuilt == 0 && ships[0].cExist == 0);
    // Actual base invalidators reset surviving base progress in both the
    // copied prefix and old physical source slot.
    reset();
    planet.fStarbase = 1;
    planet.isb = 0;
    bases[0].hul.ihuldef = static_cast<HulDef>(10);
    bases[1].hul.ihuldef = static_cast<HulDef>(5);
    bases[0].cExist = 1;
    planet.lpplprod = LpplAlloc<PLPROD>(4, 3, htOrd);
    planet.lpplprod->iprodMac = 3;
    planet.lpplprod->rgprod()[0] = row(1, 0, 2, 50);
    planet.lpplprod->rgprod()[1] = row(1, 16, 2, 59);
    planet.lpplprod->rgprod()[2] = row(1, 14, 1, 0);
    sel.grobj = grobjPlanet;
    sel.pl.id = planet.id;
    warpValues[0] = warpValues[1] = 0;
    CHECK(call(2, 17, 100) && shipKills == 1 && packetKills == 1 && lookups == 2 && fills == 2);
    CHECK(planet.isb == 1 && bases[0].cExist == 0 && bases[1].cExist == 1 && bases[1].cBuilt == 1 && !planet.idFling && !planet.iWarpFling);
    CHECK(planet.lpplprod->iprodMac == 1 && planet.lpplprod->rgprod()[0].iItem == 16 && !planet.lpplprod->rgprod()[0].pct);
    CHECK(!planet.lpplprod->rgprod()[1].pct && warpBase[0] == 0 && warpBase[1] == 1);
    reset();
    planet.fStarbase = 1;
    planet.isb = 0;
    bases[0].cExist = 0;
    bases[0].cBuilt = UINT32_MAX;
    CHECK(call(2, 16, 0));
    CHECK(bases[0].cExist == 0 && bases[0].cBuilt == 0);
    for (unsigned prior = 0; prior < 10; prior++)
        for (unsigned next = 0; next < 10; next++)
            for (int has = 0; has < 2; has++)
                for (int oldWarp = 0; oldWarp < 2; oldWarp++)
                    for (int newWarp = 0; newWarp < 2; newWarp++) {
                        reset();
                        planet.fStarbase = has;
                        planet.isb = prior;
                        planet.iWarpFling = 9;
                        for (unsigned i = 0; i < 10; i++) {
                            bases[i].cBuilt = i;
                            bases[i].cExist = 10 + i;
                        }
                        warpValues[0] = oldWarp ? 10 : 0;
                        warpValues[1] = newWarp ? 12 : 0;
                        twoValues[1] = 1;
                        CHECK(call(2, 16 + (int)next, 9));
                        CHECK(warpCalls == (oldWarp ? 1 : 2) && planet.isb == next && planet.fStarbase);
                        CHECK(bases[next].cBuilt == next + 1 && bases[next].cExist == 10 + next + 1 - (unsigned)(has && prior == next));
                        CHECK(planet.iWarpFling == (unsigned)(oldWarp ? 9 : newWarp ? 9 : 0) && planet.idFling == (unsigned)(oldWarp || newWarp ? 8 : 0));
                    }
    // Real object failures after actual construction charges, not a supplied
    // FBuildObject result. No compatible fleet at the exact 512 limit.
    reset();
    planet.id = 0;
    rgptPlan[0] = POINT16{10, 20};
    game.cPlayer = 1;
    game.cPlanMax = 1;
    planet.fStarbase = 1;
    bases[0].hul.ihuldef = ihuldefOrbitalFort;
    rgplr[0].cFleet = 512;
    cFleet = 512;
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
    planet.lpplprod = LpplAlloc<PLPROD>(4, 1, htOrd);
    planet.lpplprod->iprodMac = 1;
    planet.lpplprod->rgprod()[0] = row(1, 0, 2, 0);
    Produce();
    CHECK(!planet.lpplprod && ships[0].cBuilt == 512 && ships[0].cExist == 512 && rgplr[0].lResLastYear == 90 && planet.rgwtMin[0] == 10 && !newFleets);
    reset();
    cap = 2;
    operableCap = 5;
    planet.lpplprod = LpplAlloc<PLPROD>(4, 1, htOrd);
    planet.lpplprod->iprodMac = 1;
    planet.lpplprod->rgprod()[0] = row(5, 7, 1, 0);
    Produce();
    CHECK(planet.cFactories == 2 && rgplr[0].lResLastYear == 50 && !planet.lpplprod);
    // Genesis clears native planet stock directly; the caller's working stock
    // is later written back. Keep this distinction visible in the specification.
    reset();
    planet.lpplprod = LpplAlloc<PLPROD>(4, 1, htOrd);
    planet.lpplprod->iprodMac = 1;
    planet.lpplprod->rgprod()[0] = row(1, 13, 1, 0);
    Produce();
    CHECK(randomCalls == 12 && planet.rgwtMin[0] == 10 && planet.rgwtMin[1] == 20 && planet.rgwtMin[2] == 30 && rgplr[0].lResLastYear == 90);
    reset();
    printf("Construction effects linked fixtures: %d assertions passed\n", checks);
    return 0;
}
