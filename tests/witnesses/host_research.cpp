#include "common.h"
#include "part_query_io.h"
#include "race_query_io.h"
#include "research_io.h"
#include "turn_io.h"

#include "common.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
static int32_t  cfg[424];
static unsigned costs, lookups, messages, generals, tts, primaries, entries;
static uint32_t hash;
static int      lastField, lastLevel, typeCalls[16];
static COMPART  supplied;
struct RESEARCHESCAPE {};

static void hv(int32_t value) { hash = hash * UINT32_C(16777619) ^ (uint32_t)value; }
int16_t     TestGetRaceStat(PLAYER *p, RaceStat stat) {
    if (stat == rsMajorAdv) {
        primaries++;
        hv(30);
        hv((int32_t)(p - rgplr));
        hv(2);
    }
    return GetRaceStat(p, stat);
}
int16_t TestGetRaceGrbit(PLAYER *p, RaceGrbit bit) {
    assert(bit == ibitRaceGeneralizedResearch || bit == ibitRaceTT);
    hv(30);
    hv((int32_t)(p - rgplr));
    hv(bit == ibitRaceTT ? 1 : 0);
    if (bit == ibitRaceTT)
        tts++;
    else
        generals++;
    return GetRaceGrbit(p, bit);
}
int32_t TestGetTechLevelCost(TechFieldType field, int16_t level, int16_t player) {
    int total = 0;
    for (int i = 0; i < 6; i++)
        total += rgplr[player].rgTech[i];
    int32_t value = GetTechLevelCost(field, level, player);
    costs++;
    hv(20);
    hv(player);
    hv(field);
    hv(level);
    hv(total);
    hv(value);
    return value;
}
void TestUpdateResearchStatus(int16_t pool) {
    entries++;
    hv(40);
    hv(!!pool);
    hv(idPlayer);
    UpdateResearchStatus(pool);
}
static int kind(uint16_t mask) {
    for (int i = 0; i < 16; i++)
        if (mask == (UINT32_C(1) << i))
            return i;
    abort();
}
int16_t TestFLookupPart(PART *part) {
    int k = kind(part->hs.grhst), item = part->hs.iItem, at = typeCalls[k]++;
    hv(50);
    hv(idPlayer);
    hv(k);
    hv(item);
    lookups++;
    if ((int)lookups - 1 == cfg[5])
        throw RESEARCHESCAPE{};
    int limit = cfg[3] == 0 ? 0 : (cfg[3] == 1 || cfg[3] == 2 || cfg[3] == 6) ? 18 : cfg[3] == 3 ? k % 3 + 1 : cfg[3] == 5 ? 32769 : 258;
    if (at >= limit) {
        part->SetPcom(NULL);
        return mdPartAvailInvalid;
    }
    assert(cfg[3] != 6);
    int avail = mdPartAvailAvailable, requirement = lastLevel;
    if (cfg[3] == 1) {
        static const int states[] = {1, -1, 2, 99, 1};
        avail = states[item % 5];
        if (item % 5)
            requirement = (int8_t)(lastLevel - 1);
    }
    memset(&supplied, 0, sizeof(supplied));
    for (int i = 0; i < 6; i++)
        supplied.rgTech[i] = (int8_t)requirement;
    part->SetPcom(&supplied);
    return (int16_t)avail;
}
int16_t TestFSendPlrMsg(int16_t player, MessageId id, MsgGoto target, int16_t a, int16_t b, int16_t d, int16_t p4, int16_t p5, int16_t p6, int16_t p7) {
    assert(!p4 && !p5 && !p6 && !p7);
    int tag;
    switch (id) {
    case idmScientistsHaveCompletedResearchTechLevelWill:
        tag = 0;
        break;
    case idmScientistsHaveCompletedResearchTechLevelPrimary:
        tag = 1;
        break;
    case idmRecentBreakthroughHasAlsoGivenHullDesign:
        tag = 2;
        break;
    case idmRecentBreakthroughHasAlsoGivenHullType:
        tag = 3;
        break;
    case idmRecentBreakthroughHasAlsoTaughtHowBuild:
        tag = 4;
        break;
    case idmRecentBreakthroughHasAlsoTaughtHowBuild2:
        tag = 5;
        break;
    case idmRecentBreakthroughHasAlsoGivenBenefit:
        tag = 6;
        break;
    case idmIntelligenceGatheringActivitiesCombinedSynergist:
        tag = 7;
        break;
    default:
        abort();
    }
    messages++;
    hv(10);
    hv(player);
    hv(tag);
    hv((int16_t)target);
    hv(a);
    hv(b);
    hv(d);
    if (tag < 2) {
        lastField = b;
        lastLevel = a;
        memset(typeCalls, 0, sizeof(typeCalls));
    }
    return (int16_t)cfg[4];
}
static const unsigned rgcModelLimits[] = {16, 16, 10, 12, 24, 12, 15, 8, 10, 16, 5, 17, 11, 20, 32, 15};

static int CatalogChecks(void) {
    const unsigned counts[] = {iengineCount, iscannerCount,   ishieldCount, iarmorCount,    ibeamCount,     itorpCount,  ibombCount,         iminingCount,
                               iminesCount,  ispecialSBCount, isbhullCount, ispecialECount, ispecialMCount, iterraCount, ihuldefOrbitalFort, iplanetaryCount};
    unsigned long  checks = 0;
    PART           part;
    memset(rgplr, 0, sizeof(rgplr));
    for (unsigned k = 0; k < 16; k++) {
        assert(counts[k] < 256);
        checks++;
        assert(counts[k] == rgcModelLimits[k]);
        checks++;
        idPlayer = iplrNone;
        for (unsigned item = 0; item < 256; item++) {
            memset(&part, 0, sizeof(part));
            part.hs.grhst = (HullSlotType)(UINT32_C(1) << k);
            part.hs.iItem = item;
            int available = FLookupPart(&part);
            if ((available == mdPartAvailInvalid) != (item >= counts[k])) {
                fprintf(stderr, "catalog kind %u item %u count %u status %d\n", k, item, counts[k], available);
                return 1;
            }
            checks++;
            if (item < counts[k]) {
                assert(part.pcom());
                checks++;
            }
        }
        idPlayer = 0;
        for (int trait = -128; trait < 128; trait++) {
            rgplr[0].rgAttr[rsMajorAdv] = (int8_t)trait;
            for (unsigned item = counts[k]; item < 256; item++) {
                memset(&part, 0, sizeof(part));
                part.hs.grhst = (HullSlotType)(UINT32_C(1) << k);
                part.hs.iItem = item;
                assert(FLookupPart(&part) == mdPartAvailInvalid);
                checks++;
            }
        }
    }
    printf("Native actual catalog terminators: %lu assertions passed, all 16 counts below byte wrap\n", checks);
    return 0;
}

static void setup(void) {
    memset(rgplr, 0, sizeof(rgplr));
    memset(&game, 0, sizeof(game));
    hash = costs = lookups = messages = generals = tts = primaries = entries = 0;
    lastField = lastLevel = 0;
    memset(typeCalls, 0, sizeof(typeCalls));
    game.cPlayer = (int16_t)cfg[0];
    game.fSlowTech = cfg[2];
    idPlayer = (int16_t)cfg[6];
    for (int i = 0; i < 16; i++) {
        int     at = 8 + 26 * i;
        PLAYER *p = &rgplr[i];
        p->rgAttr[rsMajorAdv] = (int8_t)cfg[at];
        p->grbitAttr =
            static_cast<RaceTraitBits>((cfg[at + 1] ? UINT32_C(1) << ibitRaceGeneralizedResearch : 0) | (cfg[at + 2] ? UINT32_C(1) << ibitRaceTT : 0));
        p->fDead = cfg[at + 3];
        p->fCrippled = cfg[at + 4];
        p->SetITechNow(cfg[at + 5]);
        p->SetITechNext(cfg[at + 6]);
        p->lResLastYear = cfg[at + 7];
        for (int f = 0; f < 6; f++) {
            p->rgTech[f] = (int8_t)cfg[at + 8 + f];
            p->rgAttr[8 + f] = (int8_t)cfg[at + 14 + f];
            p->rgResSpent[f] = (uint32_t)cfg[at + 20 + f];
        }
    }
}
int main(int argc, char **argv) {
    assert(argc == 2);
    if (!strcmp(argv[1], "--catalog"))
        return CatalogChecks();
    RACEQUERYIO raceIO;
    raceIO.pfnStat = TestGetRaceStat;
    raceIO.pfnGrbit = TestGetRaceGrbit;
    RACEQUERYIOSCOPE raceScope(raceIO);
    RESEARCHIO       researchIO;
    researchIO.pfnTechCost = TestGetTechLevelCost;
    RESEARCHIOSCOPE researchScope(researchIO);
    TURNIO          turnIO;
    turnIO.pfnResearch = TestUpdateResearchStatus;
    turnIO.pfnMessage = TestFSendPlrMsg;
    TURNIOSCOPE turnScope(turnIO);
    PARTQUERYIO partIO;
    partIO.pfnLookup = TestFLookupPart;
    PARTQUERYIOSCOPE partScope(partIO);
    FILE            *file = fopen(argv[1], "rb");
    assert(file);
    unsigned long cases = 0, checks = 0;
    int64_t       expected[234];
    while (fread(cfg, sizeof(cfg), 1, file) == 1) {
        assert(fread(expected, sizeof(expected), 1, file) == 1);
        setup();
        PLAYER before[16];
        memcpy(before, rgplr, sizeof(before));
        GAME gameBefore = game;
        int  complete = 0;
        try {
            TurnIO().pfnResearch((int16_t)cfg[1]);
            complete = 1;
        } catch (const RESEARCHESCAPE &) {
        }
        int64_t actual[234];
        for (int i = 0; i < 16; i++) {
            for (int f = 0; f < 6; f++) {
                actual[i * 14 + f] = rgplr[i].rgTech[f];
                actual[i * 14 + 6 + f] = rgplr[i].rgResSpent[f];
            }
            actual[i * 14 + 12] = (uint8_t)rgplr[i].iTechCur;
            actual[i * 14 + 13] = rgplr[i].lResLastYear;
            PLAYER unchanged = rgplr[i];
            memcpy(unchanged.rgTech, before[i].rgTech, sizeof(unchanged.rgTech));
            memcpy(unchanged.rgResSpent, before[i].rgResSpent, sizeof(unchanged.rgResSpent));
            unchanged.iTechCur = before[i].iTechCur;
            assert(!memcmp(&unchanged, &before[i], sizeof(unchanged)));
            checks++;
        }
        assert(!memcmp(&game, &gameBefore, sizeof(game)));
        checks++;
        int64_t extra[] = {idPlayer, hash, costs, lookups, messages, generals, tts, primaries, entries, complete};
        memcpy(actual + 224, extra, sizeof(extra));
        for (int i = 0; i < 234; i++) {
            checks++;
            if (actual[i] != expected[i]) {
                fprintf(stderr, "case %lu field %d actual %lld expected %lld (pool %d slow %d mode %d)\n", cases, i, (long long)actual[i],
                        (long long)expected[i], cfg[1], cfg[2], cfg[3]);
                return 1;
            }
        }
        cases++;
    }
    assert(feof(file));
    fclose(file);
    printf("Native host research: %lu cases, %lu assertions passed\n", cases, checks);
    return 0;
}
