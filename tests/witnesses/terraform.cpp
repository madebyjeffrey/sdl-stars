#include "terraform_fixture.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"

int main(int argc, char **argv) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    TERRAFORMFIXTURE fixture(app);
    assert(argc == 3);
    FILE *file = argv[2][0] == '-' ? stdin : fopen(argv[2], argv[1][0] == 'e' ? "wb" : "rb");
    assert(file);
    if (argv[1][0] == 'e') {
        for (int i = 0; i < 20; i++) {
            int32_t r[8];
            for (int f = 0; f < 6; f++)
                r[f] = rgterra[i].rgTech[f];
            r[6] = rgterra[i].grAbility;
            r[7] = rgterra[i].resCost;
            assert(fwrite(r, sizeof(r), 1, file) == 1);
        }
        fclose(file);
        return 0;
    }
    int32_t       data[263];
    unsigned long cases = 0;
    while (fread(data, sizeof(data), 1, file) == 1) {
        reset(app);
        memcpy(cfg, data, sizeof(cfg));
        currentDesire = terraCalls = desireCalls = 0;
        memset(terraItems, -1, sizeof(terraItems));
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
        app.state.idPlayer = (int16_t)cfg[3];
        planet.iPlayer = (int16_t)cfg[4];
        for (int i = 0; i < 3; i++) {
            planet.rgEnvVar[i] = (int8_t)cfg[8 + i];
            planet.rgEnvVarOrig[i] = (int8_t)cfg[11 + i];
        }
        for (int p = 0; p < 16; p++) {
            int fields = p == 1 ? 23 : 14, levels = p == 1 ? 38 : 32;
            app.state.rgplr[p].grbitAttr = static_cast<RaceTraitBits>((p == 1 ? cfg[45] : cfg[44]) ? UINT32_C(1) << ibitRaceTT : 0);
            for (int i = 0; i < 3; i++) {
                app.state.rgplr[p].rgEnvVar[i] = (int8_t)cfg[fields + i];
                app.state.rgplr[p].rgEnvVarMin[i] = (int8_t)cfg[fields + 3 + i];
                app.state.rgplr[p].rgEnvVarMax[i] = (int8_t)cfg[fields + 6 + i];
            }
            for (int f = 0; f < 6; f++)
                app.state.rgplr[p].rgTech[f] = (int8_t)cfg[levels + f];
        }
        PLAYER expectedPlayers[16];
        PLANET expectedPlanet = planet;
        GAME   expectedGame = app.state.game;
        memcpy(expectedPlayers, app.state.rgplr, sizeof(app.state.rgplr));
        int     result;
        int16_t low[3], high[3], cost[3];
        if (cfg[0] == 0)
            result = ColonyIO().pfnCanTerraform(app, &planet, low, high, cost, (int16_t)cfg[6]);
        else if (cfg[0] == 1)
            result = IBestTerraform(app, &planet, (int16_t)cfg[6]);
        else if (cfg[0] == 2)
            result = IBestRemoteTerra(app, &planet, (int16_t)cfg[5], (int16_t)cfg[6]);
        else if (cfg[0] == 4)
            result = PctPlanetOptValue(app, &planet, (int16_t)cfg[5]);
        else
            result = call(app, grobjPlanet, iobjMaxTerraform, cfg[7]);
        int actual[209];
        actual[0] = result;
        actual[1] = app.state.idPlayer;
        for (int i = 0; i < 3; i++) {
            actual[2 + i] = planet.rgEnvVar[i];
            actual[5 + i] = observedLow[i];
            actual[8 + i] = observedHigh[i];
            actual[11 + i] = observedCost[i];
            expectedPlanet.rgEnvVar[i] = data[56 + i];
        }
        actual[14] = terraCalls;
        actual[15] = desireCalls;
        actual[16] = messages;
        for (int i = 0; i < 128; i++)
            actual[17 + i] = terraItems[i];
        for (int i = 0; i < 16; i++)
            for (int f = 0; f < 4; f++)
                actual[145 + 4 * i + f] = desireInputs[i][f];
        for (int i = 0; i < 209; i++) {
            if (actual[i] != data[54 + i]) {
                fprintf(stderr, "terraform case %lu mode %d mask %x field %d got %d expected %d\n", cases, cfg[0], cfg[2], i, actual[i], data[54 + i]);
                return 1;
            }
            checks++;
        }
        CHECK(memcmp(expectedPlayers, app.state.rgplr, sizeof(app.state.rgplr)) == 0);
        CHECK(memcmp(&expectedPlanet, &planet, sizeof(planet)) == 0);
        CHECK(memcmp(&expectedGame, &app.state.game, sizeof(app.state.game)) == 0);
        CHECK(!randomCalls);
        cases++;
    }
    CHECK(feof(file));
    fclose(file);
    reset(app);
    printf("Terraform selection: %lu cases, %d assertions passed\n", cases, checks);
    return 0;
}
