static constexpr int cQueryTraceCapacity = 8;
#include "parts_fixture.h"

int main(int argc, char **argv) {
    const unsigned rgcRows[] = {iengineCount, iscannerCount,   ishieldCount, iarmorCount,    ibeamCount,     itorpCount,  ibombCount,         iminingCount,
                                iminesCount,  ispecialSBCount, isbhullCount, ispecialECount, ispecialMCount, iterraCount, ihuldefOrbitalFort, iplanetaryCount};
    PARTQUERYFIXTURE fixture;
    assert(argc == 3 && CHAR_MIN == -128 && sizeof(rgplr[0].rgTech) == 6);
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
            if (mask == (UINT32_C(1) << k))
                kind = k;
        memset(rgplr, 0x53, sizeof(rgplr));
        memset(&game, 0x35, sizeof(game));
        idPlayer = (int16_t)(rgIn[1] ? iplrNone : rgIn[9]);
        int target = rgIn[9];
        rgplr[target].rgAttr[rsMajorAdv] = (int8_t)rgIn[2];
        rgplr[target].grbitAttr = static_cast<RaceTraitBits>((uint32_t)rgIn[3]);
        rgplr[target].grbitTrader = static_cast<GrbitTrader>((uint16_t)rgIn[4]);
        rgplr[target].SetITechNow(rgIn[5]);
        for (int f = 0; f < 6; f++)
            rgplr[target].rgTech[f] = (int8_t)rgIn[10 + f];
        PLAYER rgBefore[16];
        GAME   gameBefore;
        memcpy(rgBefore, rgplr, sizeof(rgBefore));
        memcpy(&gameBefore, &game, sizeof(game));
        int16_t idBefore = idPlayer;
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
            result = FLookupPart(&part);
            if (kind < 16 && item < rgcRows[kind])
                expected.SetPcom(LpcomRow(kind, item));
        } else if (rgIn[0] == 1)
            result = PartQueryIO().pfnHidden(&part);
        else {
            char rgNeed[6], rgNeedBefore[6];
            for (int f = 0; f < 6; f++)
                rgNeed[f] = (char)rgIn[16 + f];
            memcpy(rgNeedBefore, rgNeed, sizeof(rgNeed));
            result = PartQueryIO().pfnTechStatus(rgNeed);
            CHECK(memcmp(rgNeed, rgNeedBefore, sizeof(rgNeed)) == 0);
        }
        if (result != rgIn[22]) {
            fprintf(stderr, "mode %d mask %u item %u primary %d flags %u grants %u current %d: got %d expected %d\n", rgIn[0], mask, item, rgIn[2],
                    (uint32_t)rgIn[3], (uint32_t)rgIn[4], rgIn[5], result, rgIn[22]);
            abort();
        }
        cChecks++;
        CHECK(memcmp(&part, &expected, sizeof(part)) == 0);
        CHECK(memcmp(rgplr, rgBefore, sizeof(rgBefore)) == 0);
        CHECK(memcmp(&game, &gameBefore, sizeof(game)) == 0);
        CHECK(idPlayer == idBefore);
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
