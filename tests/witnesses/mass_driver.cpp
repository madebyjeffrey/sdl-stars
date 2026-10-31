#include "construction_buildability_fixture.h"
static int16_t DriverWarp(PLANET *p, int16_t *two) {
    assert(p == &planet);
    int at = warpCalls < 2 ? warpCalls : 1;
    warpBase[at] = p->isb;
    warpCalls++;
    return IWarpMAFromLppl(p, two);
}

static void eligible(void) {
    fInCheck = cQueries = cGateLookups = mdCheckResult = 0;
    rgplr[0].rgAttr[rsMajorAdv] = raMassAccel;
    rgplr[0].grbitTrader = static_cast<GrbitTrader>(UINT16_MAX);
    for (int i = 0; i < 6; i++)
        rgplr[0].rgTech[i] = 26;
}
static void drivers(SHDEF *d, int pattern) {
    memset(&d->hul, 0, sizeof(d->hul));
    d->det = detAll;
    d->hul.ihuldef = static_cast<HulDef>(32);
    d->hul.chs = 3;
    // none; one occupied slot containing two; two equal slots; lower ties
    // superseded by a higher slot; highest tied with a lower slot between.
    const int items[6][3] = {{0, 0, 0}, {7, 0, 0}, {7, 7, 0}, {7, 7, 15}, {15, 7, 15}, {15, 0, 0}};
    for (int i = 0; i < 3; i++) {
        d->hul.rghs[i].grhst = hstSpecialSB;
        d->hul.rghs[i].iItem = items[pattern][i];
        d->hul.rghs[i].cItem = items[pattern][i] ? 2 : 0;
    }
}
int main(int argc, char **argv) {
    CONSTRUCTIONBUILDFIXTURE fixture;
    COLONYIO                 io = ColonyIO();
    io.pfnWarp = DriverWarp;
    COLONYIOSCOPE driverScope(io);
    assert(argc == 2);
    FILE *file = fopen(argv[1], "rb");
    assert(file);
    int32_t       data[58];
    unsigned long cases = 0;
    while (fread(data, sizeof(data), 1, file) == 1) {
        reset();
        for (int i = 0; i < 16; i++)
            rglpshdefSB[i] = NULL;
        planet.iPlayer = (int16_t)data[0];
        planet.fStarbase = data[1];
        planet.isb = data[2];
        idPlayer = (int16_t)data[3];
        if (data[0] >= 0 && data[0] < 16)
            rglpshdefSB[data[0]] = data[4] ? bases : NULL;
        if (data[2] < 10) {
            SHDEF *d = bases + data[2];
            d->det = data[5];
            d->hul.chs = data[6];
            for (int i = 0; i < 16; i++) {
                d->hul.rghs[i].grhst = static_cast<HullSlotType>(data[8 + 3 * i]);
                d->hul.rghs[i].iItem = data[9 + 3 * i];
                d->hul.rghs[i].cItem = data[10 + 3 * i];
            }
        }
        PLANET expectedPlanet = planet;
        SHDEF  expectedBases[10];
        PLAYER expectedPlayers[16];
        GAME   expectedGame = game;
        memcpy(expectedBases, bases, sizeof(bases));
        memcpy(expectedPlayers, rgplr, sizeof(rgplr));
        int16_t two = 1234;
        int     warp = IWarpMAFromLppl(&planet, data[7] ? &two : NULL);
        if (warp != data[56] || two != data[57]) {
            fprintf(stderr, "mass driver case %lu got %d/%d expected %d/%d\n", cases, warp, two, data[56], data[57]);
            return 1;
        }
        checks += 2;
        CHECK(memcmp(&expectedPlanet, &planet, sizeof(planet)) == 0);
        CHECK(memcmp(expectedBases, bases, sizeof(bases)) == 0);
        CHECK(memcmp(expectedPlayers, rgplr, sizeof(rgplr)) == 0);
        CHECK(memcmp(&expectedGame, &game, sizeof(game)) == 0);
        CHECK(idPlayer == data[3]);
        CHECK(!randomCalls);
        cases++;
    }
    CHECK(feof(file));
    fclose(file);
    const int warps[6] = {0, 5, 5, 13, 13, 13}, twos[6] = {0, 0, 1, 0, 1, 0};
    unsigned  integrations = 0;
    for (int old = 0; old < 6; old++)
        for (int next = 0; next < 6; next++)
            for (int had = 0; had < 2; had++)
                for (int same = 0; same < 2; same++)
                    for (int current = -1; current < 2; current++)
                        for (int detail = 0; detail < 8; detail++)
                            for (int configured = 0; configured < 16; configured++) {
                                reset();
                                eligible();
                                idPlayer = current;
                                planet.fStarbase = had;
                                planet.isb = 0;
                                planet.iWarpFling = configured;
                                planet.idFling = 8;
                                drivers(bases, old);
                                drivers(bases + 1, next);
                                bases[0].det = bases[1].det = detail;
                                int selected = same ? 0 : 1, visible = current == -1 || current == 0 || detail == detAll;
                                int before = had && visible ? warps[old] : 0, after = visible ? warps[same ? old : next] : 0, bonus = twos[same ? old : next];
                                int expected = before ? configured : after ? after + bonus - 4 : 0;
                                CHECK(call(grobjFleet, 16 + selected, 1));
                                CHECK(warpCalls == (before ? 1 : 2));
                                CHECK(warpBase[0] == 0);
                                if (!before)
                                    CHECK(warpBase[1] == selected);
                                CHECK(planet.isb == selected && planet.fStarbase);
                                CHECK(planet.iWarpFling == expected);
                                CHECK(planet.idFling == (before || after ? 8 : 0));
                                CHECK(packetKills == (!before && !after));
                                CHECK(idPlayer == current);
                                integrations++;
                            }
    // Actual packet construction consumes the real slot maximum/duplicate bit.
    for (int pattern = 0; pattern < 6; pattern++)
        for (int configured = 0; configured < 16; configured++)
            for (int destination = 0; destination < 2; destination++) {
                reset();
                eligible();
                idPlayer = 0;
                planet.fStarbase = 1;
                drivers(bases, pattern);
                planet.iWarpFling = configured;
                planet.idFling = destination ? 8 : 0;
                int accepted = warps[pattern] > 0 && destination;
                CHECK(call(grobjPlanet, 14, 1) == accepted);
                if (accepted) {
                    int asked = configured + 4;
                    if (asked < 5 || asked > warps[pattern] + 3)
                        asked = warps[pattern] + twos[pattern];
                    int decay = asked <= warps[pattern] + twos[pattern] ? 0 : asked - warps[pattern] - twos[pattern];
                    CHECK(cThing == 1 && lpThings[0].thp().iWarp() == asked - 4 && lpThings[0].thp().iDecayRate() == decay);
                } else
                    CHECK(cThing == 0);
                integrations++;
            }
    for (int pattern = 0; pattern < 6; pattern++) {
        reset();
        eligible();
        drivers(bases, pattern);
        planet.lpplprod = LpplAlloc<PLPROD>(4, 1, htOrd);
        planet.lpplprod->iprodMac = 1;
        planet.lpplprod->rgprod()[0] = row(1, 16, grobjFleet, 0);
        Produce();
        CHECK(planet.fStarbase && planet.isb == 0 && !planet.lpplprod);
        CHECK(planet.iWarpFling == (warps[pattern] ? warps[pattern] + twos[pattern] - 4 : 0));
        CHECK(packetKills == !warps[pattern] && rgplr[0].lResLastYear == 90);
        integrations++;
    }
    reset();
    printf("Mass driver source: %lu defined standalone cases; %u construction/packet cases; %d assertions passed\n", cases, integrations, checks);
    return 0;
}
