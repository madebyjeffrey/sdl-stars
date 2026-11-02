static constexpr int cQueryTraceCapacity = 8;
#include "parts_fixture.h"
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
    PARTQUERYFIXTURE fixture;
    assert(argc == 3 && std::numeric_limits<char>::min() == -128 && sizeof(app.state.rgplr[0].rgTech) == 6);
    FILE *file = fopen(argv[2], argv[1][0] == 'e' ? "wb" : "rb");
    assert(file);
    if (argv[1][0] == 'e') {
        for (unsigned k = 0; k < 16; k++) {
            for (unsigned item = 0; item < rgcRows[k]; item++) {
                int32_t              rgData[9];
                NATIVEPARTCOMMONVIEW p = LpcomRow(k, item);
                rgData[0] = k;
                rgData[1] = item;
                for (int f = 0; f < 6; f++)
                    rgData[2 + f] = p.rgTech()[f];
                rgData[8] = k == 15 ? rgplanetary[item].grAbility : 0;
                assert(fwrite(rgData, sizeof(rgData), 1, file) == 1);
            }
        }
        fclose(file);
        return 0;
    }
    int32_t        rgIn[33];
    static COMPART comPrior;
    while (fread(rgIn, sizeof(rgIn), 1, file) == 1) {
        unsigned mask = (unsigned)rgIn[6], item = (unsigned)rgIn[7], kind = 16;
        for (unsigned k = 0; k < 16; k++)
            if (mask == (std::uint32_t{1} << k))
                kind = k;
        memset(app.state.rgplr, 0x53, sizeof(app.state.rgplr));
        memset(&app.state.game, 0x35, sizeof(app.state.game));
        app.state.idPlayer = (int16_t)(rgIn[1] ? iplrNone : rgIn[9]);
        int target = rgIn[9];
        app.state.rgplr[target].rgAttr[rsMajorAdv] = (int8_t)rgIn[2];
        app.state.rgplr[target].grbitAttr = static_cast<RaceTraitBits>((uint32_t)rgIn[3]);
        app.state.rgplr[target].grbitTrader = static_cast<GrbitTrader>((uint16_t)rgIn[4]);
        app.state.rgplr[target].SetITechNow(rgIn[5]);
        for (int f = 0; f < 6; f++)
            app.state.rgplr[target].rgTech[f] = (int8_t)rgIn[10 + f];
        PLAYER rgBefore[16];
        GAME   gameBefore;
        memcpy(rgBefore, app.state.rgplr, sizeof(rgBefore));
        memcpy(&gameBefore, &app.state.game, sizeof(app.state.game));
        int16_t idBefore = app.state.idPlayer;
        PART    part, expected;
        auto    rgbPart = std::as_writable_bytes(std::span(&part, 1));
        std::fill(rgbPart.begin(), rgbPart.end(), std::byte{0xa5});
        part.hs.grhst = static_cast<HullSlotType>((uint16_t)mask);
        part.hs.iItem = item;
        part.hs.cItem = rgIn[8];
        part.SetPcom(&comPrior);
        memcpy(&expected, &part, sizeof(part));
        memset(rgCalls, -1, sizeof(rgCalls));
        cCalls = 0;
        int result;
        if (rgIn[0] == 0) {
            result = FLookupPart(app, &part);
            if (kind < 16 && item < rgcRows[kind])
                expected.SetPcom(LpcomRow(kind, item));
        } else if (rgIn[0] == 1)
            result = PartQueryIO().pfnHidden(app, &part);
        else {
            char rgNeed[6], rgNeedBefore[6];
            for (int f = 0; f < 6; f++)
                rgNeed[f] = (char)rgIn[16 + f];
            memcpy(rgNeedBefore, rgNeed, sizeof(rgNeed));
            result = PartQueryIO().pfnTechStatus(app, rgNeed);
            CHECK(memcmp(rgNeed, rgNeedBefore, sizeof(rgNeed)) == 0);
        }
        if (result != rgIn[22]) {
            fprintf(stderr, "mode %d mask %u item %u primary %d flags %u grants %u current %d: got %d expected %d\n", rgIn[0], mask, item, rgIn[2],
                    (uint32_t)rgIn[3], (uint32_t)rgIn[4], rgIn[5], result, rgIn[22]);
            abort();
        }
        cChecks++;
        CHECK(memcmp(&part, &expected, sizeof(part)) == 0);
        CHECK(memcmp(app.state.rgplr, rgBefore, sizeof(rgBefore)) == 0);
        CHECK(memcmp(&app.state.game, &gameBefore, sizeof(app.state.game)) == 0);
        CHECK(app.state.idPlayer == idBefore);
        CHECK(cCalls == rgIn[24]);
        for (int i = 0; i < 8; i++)
            CHECK(rgCalls[i] == rgIn[25 + i]);
        cCases++;
    }
    CHECK(feof(file));
    fclose(file);
    printf("Native part availability: %lu cases, %lu assertions passed; pointers, query order and PLAYER/GAME/global frames checked\n", cCases, cChecks);
    return 0;
}
