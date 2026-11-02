#include <cassert>
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include "common.h"
#include "part_query_io.h"
static SHDEF     ships[16], bases[10], otherShips[16], otherBases[10];
static HULDEF    hull;
static PLANETARY planetary;
static uint16_t  quotes[33][4];
static int       cursor, lookups, checks, lastHull;
static uint16_t  lookupKind[33], lookupItem[33], quoteKind[33], quoteItem[33];
static int       quotePlayer[33];
static PLANET    planet;
static PROD      prod;
static int16_t   LookupQuote(APPSESSION &, PART *p) {
    assert(lookups < 33);
    lookupKind[lookups] = p->hs.grhst;
    lookupItem[lookups++] = p->hs.iItem;
    p->SetPplanetary(&planetary);
    return 2;
}
static HULDEF *HullQuote(int16_t id) {
    lastHull = id;
    return &hull;
}
static void PartQuote(APPSESSION &, int16_t player, PART *part, uint16_t *out) {
    assert(cursor < 33);
    quoteKind[cursor] = part->hs.grhst;
    quoteItem[cursor] = (cursor == 0 && part->hs.grhst == hstNone) ? 0 : part->hs.iItem;
    quotePlayer[cursor] = player;
    memcpy(out, quotes[cursor++], 8);
}
#define CHECK(x)                                                                                                                                               \
    do {                                                                                                                                                       \
        checks++;                                                                                                                                              \
        if (!(x)) {                                                                                                                                            \
            fprintf(stderr, "witness %d failed: %s\n", checks, #x);                                                                                            \
            return 1;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)

static int signed16(int64_t x) {
    unsigned u = (unsigned)((uint64_t)x & 65535);
    return u < 32768 ? (int)u : (int)u - 65536;
}
static unsigned word16(int64_t x) { return (unsigned)((uint64_t)x & 65535); }
static uint32_t word32(int64_t x) { return (uint32_t)((uint64_t)x & std::numeric_limits<std::uint32_t>::max()); }
static int      maximum(int a, int b) { return a > b ? a : b; }
static unsigned upgrade(unsigned now, unsigned before) { return word16(maximum(now / 2, signed16((int)now - signed16(before) / 2))); }
static unsigned credited(unsigned left, unsigned before, unsigned now, unsigned oldCount, unsigned newCount, int sameKind, int sameItem) {
    int a = signed16((int64_t)signed16(before) * oldCount), b = signed16((int64_t)signed16(now) * newCount), retain;
    if (!sameKind)
        retain = maximum(signed16(3 * b) / 10, b - signed16(7 * a) / 10);
    else if (!sameItem)
        retain = maximum(signed16(2 * b) / 10, b - signed16(8 * a) / 10);
    else
        retain = maximum(0, b - a);
    unsigned credit = word16(b - signed16(retain));
    return left - (credit < left ? credit : left);
}
static uint32_t oracle(APPSESSION &app, unsigned dimension, uint32_t prior, int player, int one) {
    unsigned item = prod.iItem, count = prod.cItem;
    uint32_t value = prior;
    int      colony = app.state.rgplr[planet.iPlayer].rgAttr[rsMajorAdv], race = app.state.rgplr[player].rgAttr[rsMajorAdv];
    uint32_t traits = app.state.rgplr[player].grbitAttr;
    if (prod.grobj == grobjFleet) {
        int    base = item >= 16;
        SHDEF *target = base ? &app.state.rglpshdefSB[player][item - 16] : &app.state.rglpshdef[player][item];
        if (target->fFree)
            return 0;
        HUL     *h = &target->hul;
        unsigned cost = dimension == 3 ? h->resCost : h->rgwtOreCost[dimension];
        if (base && planet.fStarbase) {
            HUL     *old = &app.state.rglpshdefSB[player][planet.isb].hul;
            unsigned prev = dimension == 3 ? old->resCost : old->rgwtOreCost[dimension];
            if (old->ihuldef != h->ihuldef)
                cost = upgrade(cost, prev);
            else {
                cost = word16((int)cost - signed16(quotes[0][dimension]));
                unsigned q = 1;
                for (unsigned i = 0; i < old->chs; i++)
                    if (old->rghs[i].cItem && h->rghs[i].cItem) {
                        cost = credited(cost, quotes[q][dimension], quotes[q + 1][dimension], old->rghs[i].cItem, h->rghs[i].cItem,
                                        old->rghs[i].grhst == h->rghs[i].grhst, old->rghs[i].iItem == h->rghs[i].iItem);
                        q += 2;
                    }
            }
        }
        if (base) {
            if ((traits & (std::uint32_t{1} << ibitRaceISB)) || race == raMacintosh)
                cost -= cost / 5;
            cost = (cost + 1) / 2;
        }
        value = cost;
    } else if (item == iobjFactory || item == mdIdleFactory) {
        unsigned cheap = (traits >> ibitRaceCheapFact) & 1;
        value = dimension == 3 ? word32(app.state.rgplr[player].rgAttr[rsFactBuild]) : app.state.gd.fTutorial ? 2 - cheap : dimension == 2 ? 4 - cheap : 0;
    } else if (item == iobjMine || item == mdIdleMine)
        value = dimension == 3 ? word32(app.state.rgplr[player].rgAttr[rsMineBuild]) : 0;
    else if (item == iobjDefense || item == mdIdleDefense) {
        value = dimension == 3 ? planetary.resCost : word32(planetary.rgwtOreCost[dimension]);
        if (race == raDefend)
            value = word32((int64_t)value * 3) / 5;
    } else if (item == iobjAlchemy || item == mdIdleAlchemy)
        value = dimension == 3 ? ((traits & (std::uint32_t{1} << ibitRaceMineralAlchemy)) ? 25 : 100) : 0;
    else if (item == iobjMinTerraform || item == iobjMaxTerraform || item == mdIdleTerraform) {
        value = dimension == 3 ? ((traits & (std::uint32_t{1} << ibitRaceTT)) ? 70 : 100) : 0;
        if (colony == raTerra)
            value /= 2;
    } else if (item == iobjGenesis || (item >= 18 && item <= 27))
        value = quotes[0][dimension];
    else if (item >= 14 && item <= 16)
        value = dimension == 3           ? (colony == raMassAccel ? 5 : 10)
                : dimension == item - 14 ? (colony == raMassAccel  ? 70
                                            : colony == raStargate ? 120
                                                                   : 110)
                                         : 0;
    else if (item == iobjPacket || item == iobjPacketMixed)
        value = dimension == 3 ? (colony == raMassAccel ? 5 : 10) : (colony == raMassAccel ? 25 : colony == raStargate ? 48 : 44);
    return one ? value : word32((int64_t)value * count);
}
static int compare(APPSESSION &app, int player, int one) {
    uint32_t actual[4] = {std::numeric_limits<std::uint32_t>::max(), std::uint32_t{0x80000000}, 17, 29}, expected[4];
    cursor = lookups = 0;
    lastHull = -1;
    for (unsigned d = 0; d < 4; d++)
        expected[d] = oracle(app, d, actual[d], player, one);
    GetProductionCosts(app, &planet, &prod, actual, player, one);
    if (prod.grobj == grobjFleet) {
        int    base = prod.iItem >= 16;
        SHDEF *target = base ? &app.state.rglpshdefSB[player][prod.iItem - 16] : &app.state.rglpshdef[player][prod.iItem];
        if (!target->fFree && base && planet.fStarbase && target->hul.ihuldef == app.state.rglpshdefSB[player][planet.isb].hul.ihuldef) {
            HUL     *old = &app.state.rglpshdefSB[player][planet.isb].hul;
            unsigned q = 1, l = 0;
            if (quoteKind[0] != hstNone || lastHull != old->ihuldef)
                return 0;
            for (unsigned i = 0; i < old->chs; i++)
                if (old->rghs[i].cItem && target->hul.rghs[i].cItem) {
                    HS pair[2] = {old->rghs[i], target->hul.rghs[i]};
                    for (unsigned j = 0; j < 2; j++, q++, l++)
                        if (lookupKind[l] != pair[j].grhst || lookupItem[l] != pair[j].iItem || quoteKind[q] != pair[j].grhst || quoteItem[q] != pair[j].iItem)
                            return 0;
                }
            if (cursor != (int)q || lookups != (int)l)
                return 0;
        } else if (cursor || lookups || lastHull != -1)
            return 0;
    } else {
        int expectedLookup = -1, expectedQuotes = 0;
        if (prod.iItem == 2 || prod.iItem == 9)
            expectedLookup = static_cast<int>(iplanetarySDI);
        else if (prod.iItem == 13) {
            expectedLookup = iplanetaryGenesisDevice;
            expectedQuotes = 1;
        } else if (prod.iItem >= 18 && prod.iItem <= 27) {
            expectedLookup = prod.iItem == 27 ? 0 : prod.iItem - 18;
            expectedQuotes = 1;
        }
        if (lookups != (expectedLookup >= 0) || cursor != expectedQuotes)
            return 0;
        if (expectedLookup >= 0 && (lookupKind[0] != hstPlanetary || lookupItem[0] != expectedLookup))
            return 0;
    }
    for (int q = 0; q < cursor; q++)
        if (quotePlayer[q] != player)
            return 0;
    for (unsigned d = 0; d < 4; d++)
        if (actual[d] != expected[d]) {
            fprintf(stderr, "cost mismatch kind=%u item=%u dim=%u actual=%u expected=%u\n", prod.grobj, prod.iItem, d, actual[d], expected[d]);
            return 0;
        }
    return 1;
}
static void reset(APPSESSION &app) {
    memset(app.state.rgplr, 0, sizeof(app.state.rgplr));
    memset(ships, 0, sizeof(ships));
    memset(bases, 0, sizeof(bases));
    memset(&planet, 0, sizeof(planet));
    memset(otherShips, 0, sizeof(otherShips));
    memset(otherBases, 0, sizeof(otherBases));
    memset(quotes, 0, sizeof(quotes));
    memset(&prod, 0, sizeof(prod));
    memset(&planetary, 0, sizeof(planetary));
    app.state.gd.fTutorial = 0;
    for (int i = 0; i < 16; i++) {
        app.state.rglpshdef[i] = ships;
        app.state.rglpshdefSB[i] = bases;
    }
    app.state.rgplr[0].rgAttr[rsFactBuild] = 10;
    app.state.rgplr[1].rgAttr[rsFactBuild] = 12;
    app.state.rgplr[0].rgAttr[rsMineBuild] = 3;
    app.state.rgplr[1].rgAttr[rsMineBuild] = 5;
    planet.iPlayer = 0;
    planetary.rgwtOreCost[0] = 13;
    planetary.rgwtOreCost[1] = 17;
    planetary.rgwtOreCost[2] = 19;
    planetary.resCost = 101;
    for (unsigned q = 0; q < 33; q++)
        for (unsigned d = 0; d < 4; d++)
            quotes[q][d] = 13 + q * 7 + d;
}
static uint32_t randomState = 12345;
static unsigned randomWord(void) {
    randomState ^= randomState << 13;
    randomState ^= randomState >> 17;
    randomState ^= randomState << 5;
    return randomState;
}
int main(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    static_assert(sizeof(PROD) == 4 && sizeof(HS) == 4 && sizeof(HUL) == 123 && sizeof(SHDEF) == 147, "native cost layouts");
    PARTQUERYIO io;
    io.pfnLookup = LookupQuote;
    io.pfnHull = HullQuote;
    io.pfnTrueCost = PartQuote;
    PARTQUERYIOSCOPE quoteScope(io);
    reset(app);
    for (unsigned owner = 0; owner < 10; owner++)
        for (unsigned pricing = 0; pricing < 10; pricing++)
            for (unsigned traits = 0; traits < 16; traits++) {
                app.state.rgplr[0].rgAttr[rsMajorAdv] = owner;
                app.state.rgplr[1].rgAttr[rsMajorAdv] = pricing;
                app.state.rgplr[1].grbitAttr = static_cast<decltype(app.state.rgplr[1].grbitAttr)>(
                    ((traits & 1) ? std::uint32_t{1} << ibitRaceCheapFact : 0) | ((traits & 2) ? std::uint32_t{1} << ibitRaceTT : 0) |
                    ((traits & 4) ? std::uint32_t{1} << ibitRaceMineralAlchemy : 0) | ((traits & 8) ? std::uint32_t{1} << ibitRaceISB : 0));
                for (unsigned tutorial = 0; tutorial < 2; tutorial++)
                    for (unsigned item = 0; item < 128; item++)
                        for (unsigned one = 0; one < 2; one++) {
                            app.state.gd.fTutorial = tutorial;
                            prod.grobj = 1;
                            prod.iItem = item;
                            prod.cItem = one ? 0 : 1023;
                            CHECK(compare(app, 1, one));
                        }
            }
    reset(app);
    prod.grobj = 2;
    prod.iItem = 17;
    prod.cItem = 3;
    planet.fStarbase = 1;
    planet.isb = 0;
    bases[0].hul.chs = 1;
    bases[1].hul.chs = 1;
    for (unsigned word = 0; word < 65536; word++)
        for (unsigned mode = 0; mode < 3; mode++)
            for (unsigned counts = 0; counts < 3; counts++) {
                unsigned oldCount = counts == 0 ? 1 : counts == 1 ? 17 : 255, newCount = counts == 0 ? 1 : counts == 1 ? 253 : 255;
                bases[0].hul.rghs[0] = HS{hstBeam, 3, (uint16_t)oldCount};
                bases[1].hul.rghs[0] = HS{mode == 0 ? hstArmor : hstBeam, (uint16_t)(mode == 1 ? 4 : 3), (uint16_t)newCount};
                for (unsigned d = 0; d < 4; d++) {
                    quotes[0][d] = (uint16_t)(word + d * 123);
                    quotes[1][d] = (uint16_t)(word + d * 19);
                    quotes[2][d] = (uint16_t)(65535 - word + d * 37);
                    bases[1].hul.rgwtOreCost[d % 3] = (uint16_t)(word + d * 7919);
                    bases[1].hul.resCost = word;
                }
                CHECK(compare(app, 0, counts & 1));
            }
    reset(app);
    prod.grobj = 2;
    prod.iItem = 17;
    prod.cItem = 1023;
    planet.fStarbase = 1;
    bases[0].hul.ihuldef = static_cast<decltype(bases[0].hul.ihuldef)>(1);
    bases[1].hul.ihuldef = static_cast<decltype(bases[1].hul.ihuldef)>(2);
    for (unsigned d = 0; d < 3; d++) {
        bases[0].hul.rgwtOreCost[d] = 65535;
        bases[1].hul.rgwtOreCost[d] = 65535;
    }
    bases[0].hul.resCost = bases[1].hul.resCost = 65535;
    app.state.rgplr[0].grbitAttr = static_cast<decltype(app.state.rgplr[0].grbitAttr)>(std::uint32_t{1} << ibitRaceISB);
    CHECK(compare(app, 0, 1));
    uint32_t exact[4] = {};
    GetProductionCosts(app, &planet, &prod, exact, 0, 1);
    CHECK(exact[0] == 13107 && exact[1] == 13107 && exact[2] == 13107 && exact[3] == 13107);
    app.state.rgplr[0].grbitAttr = static_cast<decltype(app.state.rgplr[0].grbitAttr)>(0);
    for (unsigned word = 0; word < 65536; word++) {
        for (unsigned d = 0; d < 3; d++) {
            bases[0].hul.rgwtOreCost[d] = 65535 - word;
            bases[1].hul.rgwtOreCost[d] = word;
        }
        bases[0].hul.resCost = word;
        bases[1].hul.resCost = 65535 - word;
        CHECK(compare(app, 0, 0));
    }
    for (unsigned trial = 0; trial < 20000; trial++) {
        reset(app);
        prod.grobj = 2;
        prod.iItem = randomWord() % 26;
        prod.cItem = randomWord() % 1024;
        planet.fStarbase = randomWord() % 2;
        planet.isb = randomWord() % 10;
        for (unsigned i = 0; i < 26; i++) {
            SHDEF *s = i < 16 ? &ships[i] : &bases[i - 16];
            s->fFree = randomWord() % 7 == 0;
            s->hul.ihuldef = static_cast<decltype(s->hul.ihuldef)>(randomWord() % 2);
            s->hul.chs = randomWord() % 17;
            for (unsigned d = 0; d < 3; d++)
                s->hul.rgwtOreCost[d] = randomWord();
            s->hul.resCost = randomWord();
            for (unsigned j = 0; j < 16; j++)
                s->hul.rghs[j] = HS{static_cast<HullSlotType>(randomWord() % 3), (uint16_t)(randomWord() % 3), (uint16_t)(randomWord() % 4)};
        }
        for (unsigned q = 0; q < 33; q++)
            for (unsigned d = 0; d < 4; d++)
                quotes[q][d] = randomWord();
        app.state.rgplr[0].rgAttr[rsMajorAdv] = randomWord() % 10;
        app.state.rgplr[0].grbitAttr = static_cast<decltype(app.state.rgplr[0].grbitAttr)>((randomWord() % 2) ? std::uint32_t{1} << ibitRaceISB : 0);
        CHECK(compare(app, 0, randomWord() % 2));
    }
    reset(app);
    for (unsigned kind = 0; kind < 8; kind++)
        if (kind != 2)
            for (unsigned item = 0; item < 128; item++) {
                prod.grobj = kind;
                prod.iItem = item;
                prod.cItem = 1023;
                planetary.rgwtOreCost[0] = -32768;
                planetary.rgwtOreCost[1] = -1;
                app.state.rgplr[0].rgAttr[rsMajorAdv] = raDefend;
                app.state.rgplr[0].rgAttr[rsFactBuild] = -128;
                app.state.rgplr[0].rgAttr[rsMineBuild] = -1;
                CHECK(compare(app, 0, 0));
            }
    reset(app);
    app.state.rglpshdef[1] = otherShips;
    app.state.rglpshdefSB[1] = otherBases;
    prod.grobj = 2;
    prod.cItem = 1;
    for (unsigned i = 0; i < 16; i++) {
        ships[i].hul.resCost = 11 + i;
        otherShips[i].hul.resCost = 111 + i;
        otherShips[i].hul.rgwtOreCost[0] = 222 + i;
    }
    for (unsigned i = 0; i < 10; i++) {
        bases[i].hul.resCost = 33 + i;
        otherBases[i].hul.resCost = 333 + i;
        otherBases[i].hul.rgwtOreCost[0] = 444 + i;
        otherBases[i].hul.ihuldef = static_cast<decltype(otherBases[i].hul.ihuldef)>(i == 0 ? 1 : 2);
    }
    otherShips[3].fFree = 1;
    for (unsigned item = 0; item < 26; item++)
        for (unsigned existing = 0; existing < 2; existing++) {
            prod.iItem = item;
            planet.fStarbase = existing;
            CHECK(compare(app, 1, 1));
        }
    reset(app);
    prod.grobj = 2;
    prod.iItem = 16;
    prod.cItem = 1023;
    bases[0].fFree = 1;
    CHECK(compare(app, 0, 0) && cursor == 0 && lookups == 0);
    reset(app);
    prod.grobj = 1;
    prod.iItem = 27;
    CHECK(compare(app, 0, 1) && cursor == 1 && lookups == 1);
    printf("M09 production cost linked witnesses: %d passed\n", checks);
    return 0;
}
