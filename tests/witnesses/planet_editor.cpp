#include <cassert>
#include <cstdio>
#include <cstdlib>
#include "common.h"
#include "liststorage.h"
#include "presentation_io.h"
#include "production_io.h"
static PLANET planets[4];
static struct {
    void  *p;
    size_t bytes;
    int    live;
} heap[32];
static int    heaps, allocations, reallocations, frees, checks, failAllocation, failCallback, mutateTutorial, tutorials;
static size_t requested, guardBytes;
static char   trace[256];
struct FIXTUREESCAPE {
    int stage;
};
static void event(const char *s) {
    assert(strlen(trace) + strlen(s) < sizeof(trace));
    strcat(trace, s);
}
static int slot(const void *p) {
    for (int i = heaps - 1; i >= 0; i--)
        if (heap[i].p == p && heap[i].live)
            return i;
    abort();
}
static void *TestLpAlloc(uint16_t cb, HeapType ht) {
    assert(ht == htOrd && cb >= 4);
    allocations++;
    requested = cb;
    event("A;");
    if (failAllocation)
        throw FIXTUREESCAPE{1};
    assert(heaps < 32);
    void *p = malloc(cb);
    assert(p);
    memset(p, 0xa5, cb);
    heap[heaps].p = p;
    heap[heaps].bytes = cb;
    heap[heaps++].live = 1;
    return p;
}
static void *TestLpReAlloc(void *p, uint16_t cb, HeapType ht) {
    assert(ht == htOrd && cb >= 4);
    reallocations++;
    requested = cb;
    event("R;");
    if (failAllocation)
        throw FIXTUREESCAPE{1};
    int i = slot(p);
    assert(heaps < 32);
    size_t old = heap[i].bytes;
    void  *q = realloc(p, cb);
    assert(q);
    heap[i].live = 0;
    if (cb > old)
        memset((char *)q + old, 0xa5, cb - old);
    heap[heaps].p = q;
    heap[heaps].bytes = cb;
    heap[heaps++].live = 1;
    return q;
}
static void TestFreeLp(void *p, HeapType ht) {
    assert(ht == htOrd);
    int i = slot(p);
    heap[i].live = 0;
    frees++;
    event("F;");
    free(p);
}
static void TestInvalidateReport(ReportType report, int16_t reason) {
    assert(report == rptPlanets && reason == 0);
    event("I;");
    if (failCallback == 1)
        throw FIXTUREESCAPE{4};
}
static void TestLogChangePlanet(PLANET *old, PLANET *next) {
    (void)next;
    event(old ? "Log;" : "Flush;");
    if (failCallback == 2)
        throw FIXTUREESCAPE{4};
}
static void TestAdvanceTutor(void) {
    event("T;");
    tutorials++;
    if (failCallback == 3)
        throw FIXTUREESCAPE{4};
    if (mutateTutorial)
        sel.pl.id = 1;
}
static void TestFillSelProdLB(void) { event("Fill;"); }
static void TestRedrawPlanShip(TileBits tile) {
    assert(tile == tileProductionOrOrbit);
    event("Draw;");
}
static int within(const void *p, size_t n) {
    uintptr_t a = (uintptr_t)p;
    for (int i = 0; i < heaps; i++) {
        uintptr_t b = (uintptr_t)heap[i].p;
        if (heap[i].live && a >= b && a <= b + heap[i].bytes)
            return n <= b + heap[i].bytes - a;
    }
    return 0;
}
static int planetRange(const void *p, size_t n) {
    uintptr_t a = (uintptr_t)p, b = (uintptr_t)planets, e = b + sizeof(planets);
    if (a >= b && a <= e)
        return n <= e - a;
    b = (uintptr_t)&sel.pl;
    e = b + sizeof(sel.pl);
    return a >= b && a <= e && n <= e - a;
}
static void *TestcheckedMemcpy(void *dst, const void *src, size_t n) {
    guardBytes = n;
    if (n && (uintptr_t)dst < (uintptr_t)src + n && (uintptr_t)src < (uintptr_t)dst + n)
        throw FIXTUREESCAPE{2};
    if (!(within(dst, n) || planetRange(dst, n)) || !(within(src, n) || planetRange(src, n)))
        throw FIXTUREESCAPE{3};
    return memcpy(dst, src, n);
}
#define CHECK(x)                                                                                                                                               \
    do {                                                                                                                                                       \
        checks++;                                                                                                                                              \
        if (!(x)) {                                                                                                                                            \
            fprintf(stderr, "witness %d failed: %s; trace=%s\n", checks, #x, trace);                                                                           \
            return 1;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)
static uint16_t TestExtent(const void *lp) { return (uint16_t)heap[slot(lp)].bytes; }

static void reset(void) {
    for (int i = 0; i < heaps; i++)
        if (heap[i].live)
            free(heap[i].p);
    heaps = allocations = reallocations = frees = failAllocation = failCallback = mutateTutorial = tutorials = 0;
    requested = guardBytes = 0;
    trace[0] = 0;
    memset(planets, 0, sizeof(planets));
    memset(&sel, 0, sizeof(sel));
    memset(&gd, 0, sizeof(gd));
    memset(&tutor, 0, sizeof(tutor));
    lpplProdGlob = NULL;
    cPlanet = game.cPlanMax = 2;
    idPlayer = fAi = 0;
    planets[0].id = 0;
    planets[1].id = 1;
    sel.pl.id = 0;
}
static PLPROD *queue(unsigned capacity, unsigned count, uint32_t seed) {
    PLPROD *p = LpplAlloc<PLPROD>(4, capacity, htOrd);
    p->iprodMac = count;
    for (unsigned i = 0; i < capacity; i++) {
        uint32_t w = seed + i;
        memcpy(&p->rgprod()[i], &w, 4);
    }
    return p;
}
static uint32_t word(PLPROD *p, unsigned i) {
    uint32_t w;
    assert(within(&p->rgprod()[i], 4));
    memcpy(&w, &p->rgprod()[i], 4);
    return w;
}
static void begin(void) {
    trace[0] = 0;
    allocations = reallocations = frees = 0;
    requested = 0;
}
static void prefix(PLANET *p, unsigned byte, int id) {
    memset(p, byte, 52);
    p->id = id;
}
int main(void) {
    HEAPIO heapIO;
    heapIO.pfnAllocate = TestLpAlloc;
    heapIO.pfnResize = TestLpReAlloc;
    heapIO.pfnFree = TestFreeLp;
    heapIO.pfnExtent = TestExtent;
    heapIO.pfnCopy = TestcheckedMemcpy;
    HEAPIOSCOPE    heapScope(heapIO);
    PRESENTATIONIO viewIO;
    viewIO.pfnInvalidateReport = TestInvalidateReport;
    viewIO.pfnLogPlanet = TestLogChangePlanet;
    viewIO.pfnTutor = TestAdvanceTutor;
    viewIO.pfnRedraw = TestRedrawPlanShip;
    PRESENTATIONIOSCOPE viewScope(viewIO);
    PRODUCTIONIO        productionIO;
    productionIO.pfnRefresh = TestFillSelProdLB;
    PRODUCTIONIOSCOPE productionScope(productionIO);
    lpPlanets = planets;
    static_assert(offsetof(PLANET, lpplprod) == 52, "native packed prefix");
    static_assert(sizeof(PLANET) == 52 + sizeof(void *), "pointer after prefix");
    static_assert(sizeof(PROD) == 4 && sizeof(PLPROD) == 4 && sizeof(PL) == 4, "queue widths");
    CHECK(CbHeapAlign(htOrd) == alignof(max_align_t) && CbHeapAlign(htBattle) == 2);
    for (unsigned count = 0; count <= 1; count++) {
        unsigned payload = CbHeapItem(4 + 4 * count, htOrd) - 2;
        CHECK(payload >= 4 + 4 * count && (payload - 4) / 4 < 254);
    }
    for (unsigned n = 0; n < 256; n++)
        for (unsigned cap = 0; cap < 256; cap++)
            for (int write = 0; write < 2; write++) {
                reset();
                unsigned srcMax = n < 253 ? n + 2 : n;
                prefix(&planets[0], 0x11, 0);
                prefix(&sel.pl, 0x22, 0);
                sel.pl.lpplprod = queue(srcMax, n, 0xf1234000);
                planets[0].lpplprod = queue(cap, cap ? 1 : 0, 0xa1234000);
                PLANET *src = write ? &sel.pl : &planets[0], *dst = write ? &planets[0] : &sel.pl;
                if (!write) {
                    PLPROD *q = src->lpplprod;
                    src->lpplprod = dst->lpplprod;
                    dst->lpplprod = q;
                }
                unsigned want = write ? n + 2 : srcMax;
                PLPROD  *source = src->lpplprod;
                begin();
                int result = write ? FLookupPlanet(-1, dst == &sel.pl ? dst : src) : FDupPlanet(src, dst);
                CHECK(result == 1 && memcmp(src, dst, 52) == 0 && src->lpplprod == source && dst->lpplprod != source);
                CHECK(dst->lpplprod->iprodMac == n && dst->lpplprod->iprodMax == (cap < n ? want % 256 : cap));
                CHECK(reallocations == (cap < n) && allocations == 0 && frees == 0);
                CHECK(word(source, 0) == 0xf1234000u || n == 0); // physical source is nonempty except n=0/255 special cases
                for (unsigned i = 0; i < n; i++)
                    CHECK(word(dst->lpplprod, i) == 0xf1234000u + i);
                if (cap > n)
                    CHECK(word(dst->lpplprod, n) == 0xa1234000u + n);
                if (cap < n)
                    CHECK(requested == 4 + 4 * want);
                if (write)
                    CHECK(strcmp(trace, cap < n ? "I;Log;R;" : "I;Log;") == 0);
            }
    for (int write = 0; write < 2; write++)
        for (int hasSource = 0; hasSource < 2; hasSource++)
            for (int hasDest = 0; hasDest < 2; hasDest++) {
                reset();
                prefix(&planets[0], 0x11, 0);
                prefix(&sel.pl, 0x22, 0);
                PLANET *src = write ? &sel.pl : &planets[0], *dst = write ? &planets[0] : &sel.pl;
                src->lpplprod = hasSource ? queue(5, 2, 17) : NULL;
                dst->lpplprod = hasDest ? queue(3, 1, 81) : NULL;
                PLPROD *source = src->lpplprod;
                begin();
                CHECK((write ? FLookupPlanet(-1, src) : FDupPlanet(src, dst)) == 1);
                CHECK(memcmp(src, dst, 52) == 0 && src->lpplprod == source);
                CHECK(frees == (!hasSource && hasDest));
                CHECK(allocations == (hasSource && !hasDest));
                if (hasSource) {
                    CHECK(dst->lpplprod != source && dst->lpplprod->iprodMac == 2);
                    CHECK(dst->lpplprod->iprodMax == (hasDest ? 3 : write ? 2 : 5));
                } else
                    CHECK(dst->lpplprod == NULL);
            }
    reset();
    planets[0].lpplprod = queue(4, 2, 33);
    sel.pl.lpplprod = planets[0].lpplprod;
    begin();
    CHECK(FLookupPlanet(-1, &sel.pl) == 1 && !allocations && !reallocations && !frees);
    reset();
    sel.pl.lpplprod = queue(1, 1, 88);
    planets[0].lpplprod = queue(2, 1, 17);
    PLANET borrowed = {};
    begin();
    CHECK(FLookupPlanet(0, &borrowed) == 1 && borrowed.lpplprod == planets[0].lpplprod && !allocations && !frees);
    borrowed.lpplprod = sel.pl.lpplprod;
    planets[0].lpplprod = NULL; // ordinary read overwrites this pointer; it does not free it
    CHECK(FLookupPlanet(0, &borrowed) == 1 && borrowed.lpplprod == NULL && !frees);
    reset();
    begin();
    CHECK(FLookupPlanet(0, NULL) == 1 && !trace[0]);
    CHECK(FLookupPlanet(9, NULL) == 0);
    cPlanet = 0;
    CHECK(FLookupPlanet(-1, NULL) == 0); // count guard precedes destination dereference
    cPlanet = 2;
    sel.pl.id = -1;
    CHECK(FLookupPlanet(-32768, &sel.pl) == 1 && strcmp(trace, "Flush;") == 0);
    sel.pl.id = 1;
    begin();
    CHECK(FLookupPlanet(-32768, &sel.pl) == 1 && strcmp(trace, "I;Log;") == 0);
    for (int write = 0; write < 2; write++)
        for (int grow = 0; grow < 2; grow++) {
            reset();
            prefix(&planets[0], 0x11, 0);
            prefix(&sel.pl, 0x22, 0);
            PLANET *src = write ? &sel.pl : &planets[0], *dst = write ? &planets[0] : &sel.pl;
            src->lpplprod = queue(5, 3, 71);
            dst->lpplprod = grow ? queue(1, 1, 81) : NULL;
            PLPROD       *before = dst->lpplprod;
            unsigned char previous[52];
            memcpy(previous, dst, 52);
            begin();
            failAllocation = 1;
            int escaped = 0;
            try {
                if (write)
                    FLookupPlanet(-1, src);
                else
                    FDupPlanet(src, dst);
                CHECK(0);
            } catch (const FIXTUREESCAPE &error) {
                escaped = error.stage;
            }
            CHECK(escaped == 1 && dst->lpplprod == before);
            CHECK(memcmp(dst, write ? previous : (void *)src, 52) == 0);
            CHECK(strcmp(trace, write ? (grow ? "I;Log;R;" : "I;Log;A;") : (grow ? "R;" : "A;")) == 0);
        }
    for (int stage = 1; stage <= 3; stage++) {
        reset();
        prefix(&planets[0], 0x11, 0);
        prefix(&sel.pl, 0x22, 0);
        gd.fTutorial = 1;
        failCallback = stage;
        begin();
        int escaped = 0;
        try {
            FLookupPlanet(-1, &sel.pl);
            CHECK(0);
        } catch (const FIXTUREESCAPE &error) {
            escaped = error.stage;
        }
        CHECK(escaped == 4);
        CHECK((memcmp(&planets[0], &sel.pl, 52) == 0) == (stage == 3));
    }
    reset();
    planets[0].lpplprod = queue(0, 0, 1);
    sel.pl.lpplprod = NULL;
    CHECK(FLookupPlanet(0, &sel.pl) == 1 && sel.pl.lpplprod && sel.pl.lpplprod != planets[0].lpplprod && sel.pl.lpplprod->iprodMac == 0);
    for (unsigned n = 254; n <= 255; n++) {
        reset();
        planets[0].lpplprod = queue(1, 1, 1);
        sel.pl.lpplprod = queue(n, n, 77);
        begin();
        CHECK(FLookupPlanet(-1, &sel.pl) == 1 && planets[0].lpplprod->iprodMac == n);
        CHECK(planets[0].lpplprod->iprodMax == (n + 2) % 256 && requested == 4 + 4 * (n + 2));
        FreePl(sel.pl.lpplprod);
        sel.pl.lpplprod = NULL;
        begin();
        int trapped = 0;
        try {
            FLookupPlanet(0, &sel.pl);
            CHECK(0);
        } catch (const FIXTUREESCAPE &error) {
            trapped = error.stage;
        }
        CHECK(trapped == 3 && requested == 4 + 4 * ((n + 2) % 256) && guardBytes == 4 * n);
    }
    reset();
    planets[0].lpplprod = queue(256, 2, 1);
    begin();
    int escaped = 0;
    try {
        FDupPlanet(&planets[0], &sel.pl);
        CHECK(0);
    } catch (const FIXTUREESCAPE &error) {
        escaped = error.stage;
    }
    CHECK(escaped == 3 && requested == 4 && sel.pl.lpplprod && sel.pl.lpplprod->iprodMax == 0 && guardBytes == 8);
    reset();
    planets[0].lpplprod = queue(2, 2, 1);
    sel.pl.lpplprod = planets[0].lpplprod;
    begin();
    escaped = 0;
    try {
        FDupPlanet(&planets[0], &sel.pl);
        CHECK(0);
    } catch (const FIXTUREESCAPE &error) {
        escaped = error.stage;
    }
    CHECK(escaped == 2 && guardBytes == 8);
    reset();
    begin();
    escaped = 0;
    try {
        FLookupPlanet(-1, &planets[0]);
        CHECK(0);
    } catch (const FIXTUREESCAPE &error) {
        escaped = error.stage;
    }
    CHECK(escaped == 2 && guardBytes == 52);
    reset();
    planets[0].lpplprod = queue(1, 1, 17);
    sel.pl.lpplprod = queue(2, 1, 99);
    lpplProdGlob = queue(2, 2, 51);
    sel.pl.fNoResearch = 1;
    begin();
    FinishProduction(1);
    CHECK(!lpplProdGlob && sel.pl.fNoResearch && planets[0].fNoResearch);
    CHECK(planets[0].lpplprod != sel.pl.lpplprod && planets[0].lpplprod->iprodMac == 2 && sel.pl.lpplprod->iprodMac == 2);
    CHECK(word(planets[0].lpplprod, 1) == 52 && word(sel.pl.lpplprod, 1) == 52 && frees == 1);
    CHECK(strcmp(trace, "F;I;Log;R;Fill;Draw;") == 0);
    reset();
    gd.fTutorial = 1;
    mutateTutorial = 1;
    lpplProdGlob = queue(1, 1, 53);
    begin();
    FinishProduction(1);
    CHECK(sel.pl.id == 1 && planets[0].lpplprod && planets[0].lpplprod->iprodMac == 1 && !sel.pl.lpplprod);
    CHECK(tutorials == 2 && tutor.fProgress && strstr(trace, "Log;A;T;") != NULL);
    reset();
    for (int i = 0; i < heaps; i++)
        if (heap[i].live)
            free(heap[i].p);
    printf("Native heap alignment: %u; fresh 0/1-row payload bytes: %u/%u\n", (unsigned)CbHeapAlign(htOrd), (unsigned)CbHeapItem(4, htOrd) - 2,
           (unsigned)CbHeapItem(8, htOrd) - 2);
    printf("S06 planet copy / M09 completion linked witnesses: %d passed\n", checks);
    return 0;
}
