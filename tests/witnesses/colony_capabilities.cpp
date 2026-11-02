#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
#include "terraform_fixture.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
static int32_t   capcfg[110];
static int       extraCalls, extraPlayer, extraTrace[16][4], selectedScanner;
static SPECIALSB fakeSpecial[16];
static void      CapabilityScanner(APPSESSION &app, PART *part) {
    LookupBestPlanetaryScanner(app, part);
    selectedScanner = part->hs.iItem;
}
static int16_t CapabilityLookup(APPSESSION &app, PART *part) {
    unsigned mask = part->hs.grhst, item = part->hs.iItem;
    if (mask == hstTerra || fInCheck)
        return TerraformLookup(app, part);
    assert(extraCalls < 16 && ((mask == hstSpecialSB && item < 16) || (mask == hstPlanetary && item < 15)));
    int quantity = capcfg[0] == 3 ? -1 : part->hs.cItem, status;
    extraPlayer = app.state.idPlayer;
    if (!cfg[1])
        status = FLookupPart(app, part);
    else {
        if (mask == hstSpecialSB)
            part->SetPspecialsb(fakeSpecial + item);
        else
            part->SetPplanetary(rgplanetary + item);
        status = (cfg[2] & (1 << item)) ? 1 : -1;
    }
    assert(part->pcom() == (COMPART *)(mask == hstSpecialSB ? (void *)(cfg[1] ? fakeSpecial + item : rgspecialSB + item) : (void *)(rgplanetary + item)));
    extraTrace[extraCalls][0] = mask;
    extraTrace[extraCalls][1] = item;
    extraTrace[extraCalls][2] = status;
    extraTrace[extraCalls++][3] = quantity;
    return (int16_t)status;
}
struct CAPABILITYFIXTURE {
    TERRAFORMFIXTURE  fixture;
    COLONYIOSCOPE     colonyScope;
    PRODUCTIONIOSCOPE productionScope;
    PARTQUERYIOSCOPE  partScope;
    CAPABILITYFIXTURE(APPSESSION &app)
        : fixture(app), colonyScope([] {
              COLONYIO io = ColonyIO();
              io.pfnTerraformPercent = IpctCanTerraformLppl;
              return io;
          }()),
          productionScope([] {
              PRODUCTIONIO io = ProductionIO();
              io.pfnScanner = CapabilityScanner;
              return io;
          }()),
          partScope([] {
              PARTQUERYIO io = PartQueryIO();
              io.pfnLookup = CapabilityLookup;
              return io;
          }()) {}
};

static void ready(APPSESSION &app) {
    reset(app);
    memcpy(cfg, capcfg, sizeof(cfg));
    terraCalls = desireCalls = currentDesire = extraCalls = 0;
    extraPlayer = selectedScanner = -1;
    memset(terraItems, -1, sizeof(terraItems));
    memset(extraTrace, -1, sizeof(extraTrace));
    for (int i = 0; i < 16; i++)
        for (int f = 0; f < 4; f++)
            desireInputs[i][f] = -999;
    for (int i = 0; i < 3; i++)
        observedLow[i] = observedHigh[i] = observedCost[i] = 9876;
    for (int i = 0; i < 20; i++)
        scripted[i] = rgterra[i];
    if (cfg[46] >= 0) {
        scripted[cfg[46]].grAbility = (int16_t)cfg[47];
        scripted[cfg[46]].resCost = (uint16_t)cfg[48];
    }
    for (int i = 0; i < 16; i++) {
        fakeSpecial[i] = rgspecialSB[i];
        if (i == cfg[46])
            fakeSpecial[i].grAbility2 = (int16_t)cfg[47];
    }
    app.state.idPlayer = (int16_t)cfg[3];
    planet.iPlayer = (int16_t)cfg[4];
    planet.fStarbase = capcfg[55];
    planet.isb = capcfg[56];
    for (int i = 0; i < 3; i++) {
        planet.rgEnvVar[i] = (int8_t)cfg[8 + i];
        planet.rgEnvVarOrig[i] = (int8_t)cfg[11 + i];
    }
    for (int p = 0; p < 16; p++) {
        int fields = p == 1 ? 23 : 14, levels = p == 1 ? 38 : 32;
        app.state.rgplr[p].grbitAttr = static_cast<RaceTraitBits>(((p == 1 ? cfg[45] : cfg[44]) ? std::uint32_t{1} << ibitRaceTT : 0) |
                                                        ((capcfg[0] != 0 && cfg[52]) ? std::uint32_t{1} << ibitRaceNoAdvScanner : 0));
        app.state.rgplr[p].rgAttr[rsMajorAdv] = capcfg[0] == 0 ? 0 : (int8_t)(p == 1 ? cfg[51] : cfg[50]);
        app.state.rgplr[p].SetITechNow(capcfg[0] == 0 ? 0 : cfg[53]);
        app.state.rgplr[p].wScore = 100 + p;
        app.state.rgplr[p].lSalt = 10000 + p;
        app.state.rgplr[p].idPlanetHome = p;
        for (int i = 0; i < 3; i++) {
            app.state.rgplr[p].rgEnvVar[i] = (int8_t)cfg[fields + i];
            app.state.rgplr[p].rgEnvVarMin[i] = (int8_t)cfg[fields + 3 + i];
            app.state.rgplr[p].rgEnvVarMax[i] = (int8_t)cfg[fields + 6 + i];
        }
        for (int f = 0; f < 6; f++)
            app.state.rgplr[p].rgTech[f] = (int8_t)cfg[levels + f];
        app.state.rglpshdefSB[p] = NULL;
    }
    if (cfg[4] >= 0 && cfg[4] < 16)
        app.state.rglpshdefSB[cfg[4]] = capcfg[57] ? bases : NULL;
    int index = capcfg[54] ? capcfg[56] : planet.isb;
    if (index >= 0 && index < 10) {
        SHDEF *d = bases + index;
        d->hul.chs = capcfg[58];
        d->det = capcfg[59];
        d->fFree = capcfg[60];
        for (int i = 0; i < 16; i++) {
            d->hul.rghs[i].grhst = static_cast<HullSlotType>(capcfg[62 + 3 * i]);
            d->hul.rghs[i].iItem = capcfg[63 + 3 * i];
            d->hul.rghs[i].cItem = capcfg[64 + 3 * i];
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

    CAPABILITYFIXTURE fixture(app);
    assert(argc == 3);
    FILE *file = argv[2][0] == '-' ? stdin : fopen(argv[2], argv[1][0] == 'e' ? "wb" : "rb");
    assert(file);
#ifdef _WIN32
    if (file == stdin)
        assert(_setmode(_fileno(file), _O_BINARY) != -1);
#endif
    if (argv[1][0] == 'e') {
        for (int i = 0; i < 20; i++) {
            int32_t r[8];
            for (int f = 0; f < 6; f++)
                r[f] = rgterra[i].rgTech[f];
            r[6] = rgterra[i].grAbility;
            r[7] = rgterra[i].resCost;
            assert(fwrite(r, sizeof(r), 1, file) == 1);
        }
        for (int kind = 0; kind < 2; kind++)
            for (int i = 0; i < (kind ? 15 : 16); i++) {
                int32_t  r[7];
                COMPART *p = kind ? (COMPART *)(rgplanetary + i) : (COMPART *)(rgspecialSB + i);
                for (int f = 0; f < 6; f++)
                    r[f] = p->rgTech[f];
                r[6] = kind ? rgplanetary[i].grAbility : rgspecialSB[i].grAbility2;
                assert(fwrite(r, sizeof(r), 1, file) == 1);
            }
        fclose(file);
        return 0;
    }
    int32_t       data[388];
    unsigned long cases = 0, modes[4] = {};
    while (fread(data, sizeof(data), 1, file) == 1) {
        memcpy(capcfg, data, sizeof(capcfg));
        ready(app);
        PLAYER expectedPlayers[16];
        PLANET expectedPlanet = planet;
        GAME   expectedGame = app.state.game;
        SHDEF  expectedBases[10];
        memcpy(expectedPlayers, app.state.rgplr, sizeof(app.state.rgplr));
        memcpy(expectedBases, bases, sizeof(bases));
        int  result;
        PART part = {};
        part.hs.cItem = capcfg[61];
        part.SetPplanetary(rgplanetary + 14);
        if (cfg[0] == 0)
            result = IpctCanTerraformLppl(app, &planet);
        else if (cfg[0] == 1)
            result = StargateRangeFromLppl(app, capcfg[54] ? NULL : &planet, (int16_t)cfg[4], (int16_t)capcfg[56]);
        else if (cfg[0] == 2) {
            ProductionIO().pfnScanner(app, &part);
            result = part.hs.iItem;
            CHECK(part.hs.grhst == hstPlanetary && part.hs.cItem == capcfg[61] && part.pplanetary() == rgplanetary + part.hs.iItem);
        } else
            result = call(app, grobjPlanet, iobjPlanetaryScanner, 1);
        int actual[278];
        actual[0] = result;
        actual[1] = app.state.idPlayer;
        for (int i = 0; i < 3; i++) {
            actual[2 + i] = planet.rgEnvVar[i];
            actual[5 + i] = observedLow[i];
            actual[8 + i] = observedHigh[i];
            actual[11 + i] = observedCost[i];
        }
        actual[14] = terraCalls;
        actual[15] = desireCalls;
        actual[16] = messages;
        for (int i = 0; i < 128; i++)
            actual[17 + i] = terraItems[i];
        for (int i = 0; i < 16; i++)
            for (int f = 0; f < 4; f++)
                actual[145 + 4 * i + f] = desireInputs[i][f];
        actual[209] = extraCalls;
        actual[210] = selectedScanner;
        actual[211] = extraCalls ? extraTrace[extraCalls - 1][0] : -1;
        actual[212] = extraCalls ? extraTrace[extraCalls - 1][3] : -1;
        actual[213] = extraPlayer;
        for (int i = 0; i < 16; i++)
            for (int f = 0; f < 4; f++)
                actual[214 + 4 * i + f] = extraTrace[i][f];
        for (int i = 0; i < 278; i++) {
            if (actual[i] != data[110 + i]) {
                fprintf(stderr, "colony capability case %lu op %d field %d got %d expected %d\n", cases, cfg[0], i, actual[i], data[110 + i]);
                return 1;
            }
            checks++;
        }
        if (cfg[0] == 3) {
            expectedPlanet.iScanner = selectedScanner;
            CHECK(messageArgs[0][2] == selectedScanner && messageScanner[0] == 31);
        }
        CHECK(memcmp(expectedPlayers, app.state.rgplr, sizeof(app.state.rgplr)) == 0);
        CHECK(memcmp(&expectedPlanet, &planet, sizeof(planet)) == 0);
        CHECK(memcmp(&expectedGame, &app.state.game, sizeof(app.state.game)) == 0);
        CHECK(memcmp(expectedBases, bases, sizeof(bases)) == 0);
        CHECK(!randomCalls);
        cases++;
        modes[cfg[0]]++;
    }
    CHECK(feof(file));
    fclose(file);
    // Actual Produce validation -> builder -> real count/selection/application.
    for (int distance = 0; distance <= 2; distance++) {
        memset(capcfg, 0, sizeof(capcfg));
        capcfg[3] = 0;
        capcfg[4] = 0;
        capcfg[44] = capcfg[45] = 1;
        capcfg[46] = -1;
        capcfg[57] = 1;
        for (int i = 0; i < 3; i++) {
            capcfg[8 + i] = capcfg[11 + i] = 50 + (i == 0 ? distance : 0);
            capcfg[14 + i] = 50;
            capcfg[17 + i] = 25;
            capcfg[20 + i] = 75;
        }
        for (int i = 0; i < 6; i++)
            capcfg[32 + i] = 26;
        ready(app);
        planet.lpplprod = LpplAlloc<PLPROD>(app, 4, 1, htOrd);
        planet.lpplprod->iprodMac = 1;
        planet.lpplprod->rgprod()[0] = row(3, mdIdleTerraform, grobjPlanet, 0);
        Produce(app);
        CHECK(!planet.lpplprod && planet.rgEnvVar[0] == 50 && app.state.rgplr[0].lResLastYear == 100 - distance * 10);
        CHECK(terraformCalls == distance);
    }
    for (int i = 0; i < 4; i++)
        CHECK(modes[i] > 0);
    capcfg[8] = capcfg[11] = -23;
    capcfg[14] = -50;
    capcfg[17] = -75;
    capcfg[20] = -25;
    ready(app);
    CHECK(IpctCanTerraformLppl(app, &planet) == -24);
    planet.lpplprod = LpplAlloc<PLPROD>(app, 4, 1, htOrd);
    planet.lpplprod->iprodMac = 1;
    planet.lpplprod->rgprod()[0] = row(3, mdIdleTerraform, grobjPlanet, 0);
    Produce(app);
    CHECK(!planet.lpplprod && !terraformCalls && app.state.rgplr[0].lResLastYear == 100);
    reset(app);
    printf(
        "Colony capabilities: %lu cases (count %lu, gate %lu, scanner %lu, generic %lu) plus four Produce clipping/application cases; %d assertions passed\n",
        cases, modes[0], modes[1], modes[2], modes[3], checks);
    return 0;
}
