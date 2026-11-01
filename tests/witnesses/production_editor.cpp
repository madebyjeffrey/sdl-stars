#include <map>
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include "ui_fixture.h"
static int     primary, genesis, warp, maxFactories, maxMines, maxDefenses, terraform, tutorials;
static SHDEF   baseDefs[10];
static HULDEF  hullDefs[10];
static PROD    catalog[64];
static int     shift, ctrl, queueFill, sourceFill, redraw, beeps, setSelection, allocations, reallocations, failReallocation, checks;
static int32_t sourceSelection, queueSelection;
static size_t  lastBytes, overlapBytes;
static int     overlapOffset;
struct FIXTUREESCAPE {
    int stage;
};
static std::map<const void *, uint16_t> extents;
static void                            *TestLpAlloc(APPSESSION &, uint16_t cb, HeapType ht) {
    (void)ht;
    allocations++;
    lastBytes = cb;
    void *p = calloc(1, cb);
    if (!p)
        abort();
    extents[p] = cb;
    return p;
}
static void *TestLpReAlloc(APPSESSION &, void *p, uint16_t cb, HeapType ht) {
    (void)ht;
    reallocations++;
    lastBytes = cb;
    if (failReallocation)
        throw FIXTUREESCAPE{2};
    extents.erase(p);
    p = realloc(p, cb);
    if (!p)
        abort();
    extents[p] = cb;
    return p;
}
static void TestFreeLp(APPSESSION &, void *p, HeapType ht) {
    (void)ht;
    extents.erase(p);
    free(p);
}
static HULDEF *TestLphuldefFromId(int id) {
    if (id < 0 || id >= 10)
        abort();
    return &hullDefs[id];
}
static int TestFLookupPart(APPSESSION &, PART *p) {
    (void)p;
    return genesis ? mdPartAvailAvailable : 0;
}
static int16_t TestIWarpMAFromLppl(APPSESSION &, void *p, void *q) {
    (void)p;
    (void)q;
    return warp;
}
static int16_t TestCMaxFactories(APPSESSION &, void *p, int player) {
    (void)p;
    (void)player;
    return maxFactories;
}
static int16_t TestCMaxMines(APPSESSION &, void *p, int player) {
    (void)p;
    (void)player;
    return maxMines;
}
static int16_t TestCMaxDefenses(APPSESSION &, void *p, int player) {
    (void)p;
    (void)player;
    return maxDefenses;
}
static int16_t TestIpctCanTerraformLppl(APPSESSION &, void *p) {
    (void)p;
    return terraform;
}
static int TestGetRaceStat(void *p, int stat) {
    (void)p;
    (void)stat;
    return primary;
}
static void  TestAdvanceTutor(APPSESSION &) { tutorials++; }
static void *TestguardedMemcpy(void *dst, const void *src, size_t n) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    uintptr_t d = (uintptr_t)dst, s = (uintptr_t)src;
    if (n && d < s + n && s < d + n) {
        overlapBytes = n;
        overlapOffset = ((char *)dst - (char *)app.state.lpplProdGlob->rgprod()) / 4;
        throw FIXTUREESCAPE{1};
    }
    return memcpy(dst, src, n);
}
static int TestGetDlgItem(HWND hwnd, int id) {
    (void)hwnd;
    return id;
}
static intptr_t TestLUiSendMessage(HWND hwnd, int message, int value, intptr_t param) {
    (void)param;
    if (message == win32::LB_GETCURSEL)
        return hwnd == HwndWitness(IDC_PRODUCTION_QUEUE) ? queueSelection : sourceSelection;
    if (message == win32::LB_SETCURSEL) {
        setSelection = value;
        queueSelection = value;
        return value;
    }
    abort();
}
static int  TestGetAsyncKeyState(int key) { return (key == win32::VK_SHIFT ? shift : ctrl) ? 0x8000 : 0; }
static void TestFillPlanetProdLB(APPSESSION &, HWND hwnd, PLPROD *p, void *q) {
    (void)hwnd;
    (void)p;
    (void)q;
    queueFill++;
}
static void TestFillProdSrcLB(APPSESSION &, HWND hwnd, int value) {
    (void)hwnd;
    (void)value;
    sourceFill++;
}
static void TestMessageBeep(int value) {
    (void)value;
    beeps++;
}
#define CHECK(x)                                                                                                                                               \
    do {                                                                                                                                                       \
        checks++;                                                                                                                                              \
        if (!(x)) {                                                                                                                                            \
            fprintf(stderr, "witness %d failed: %s\n", checks, #x);                                                                                            \
            return 1;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)
static uint16_t TestExtent(APPSESSION &, const void *p) { return extents.at(p); }
static void     TestGetClientRect(HWND, RECT *r) { *r = RECT{}; }
static void     TestInvalidateRect(HWND, const RECT *, BOOL) {}
static void     TestDrawProductionDlg(APPSESSION &, HWND, HDC, RECT *, int16_t) { redraw++; }
static void     command(APPSESSION &app, int cmd) {
    WPARAM value = cmd == 100 ? win32::MAKEWPARAM(IDC_PRODUCTION_AVAILABLE_ITEMS, 2) : cmd == 101 ? win32::MAKEWPARAM(IDC_PRODUCTION_QUEUE, 2) : static_cast<WPARAM>(cmd);
    ProdCommandHandler(app, HwndWitness(1), value, 0);
}

static PROD row(unsigned count, unsigned item, unsigned kind) {
    PROD p = {};
    p.cItem = count;
    p.iItem = item;
    p.grobj = kind;
    return p;
}
static void reset(APPSESSION &app, unsigned n, int hasQueue) {
    int i;
    if (app.state.lpplProdGlob)
        FreePl(app, app.state.lpplProdGlob);
    app.state.lpplProdGlob = NULL;
    if (app.state.sel.pl.lpplprod)
        FreePl(app, app.state.sel.pl.lpplprod);
    memset(&app.state.sel, 0, sizeof(app.state.sel));
    memset(&app.state.gd, 0, sizeof(app.state.gd));
    if (hasQueue) {
        app.state.sel.pl.lpplprod = LpplAlloc<PLPROD>(app, 4, n, htOrd);
        app.state.sel.pl.lpplprod->iprodMac = n;
    }
    memset(catalog, 0, sizeof(catalog));
    app.state.pProdGlob = catalog;
    app.state.cProdGlob = 0;
    memset(app.state.rgshdef, 0, sizeof(app.state.rgshdef));
    memset(baseDefs, 0, sizeof(baseDefs));
    memset(hullDefs, 0, sizeof(hullDefs));
    for (i = 0; i < 16; i++)
        app.state.rgshdef[i].fFree = 1;
    for (i = 0; i < 10; i++) {
        baseDefs[i].fFree = 1;
        baseDefs[i].hul.ihuldef = static_cast<HulDef>(i);
    }
    primary = genesis = warp = maxFactories = maxMines = maxDefenses = terraform = tutorials = 0;
    app.state.sel.pl.iScanner = 31;
    shift = ctrl = queueFill = sourceFill = redraw = beeps = allocations = reallocations = failReallocation = 0;
    sourceSelection = 0;
    queueSelection = -1;
    setSelection = -99;
    overlapBytes = 0;
    overlapOffset = -1;
}
static void working(APPSESSION &app, unsigned n) {
    reset(app, 0, 0);
    app.state.lpplProdGlob = LpplAlloc<PLPROD>(app, 4, n, htOrd);
    app.state.lpplProdGlob->iprodMac = n;
    allocations = 0;
}

int main(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    int    i, mode, available, request;
    PROD   a = row(10, 7, 1), b = row(1, 8, 1), c = row(1, 9, 1);
    PROD   bad = row(9, 0, 2); // Ship design 0 with no construction base is unavailable.
    HEAPIO ioHeap;
    ioHeap.pfnAllocate = WITNESSADAPTER<&TestLpAlloc, decltype(ioHeap.pfnAllocate)>::Call;
    ioHeap.pfnResize = WITNESSADAPTER<&TestLpReAlloc, decltype(ioHeap.pfnResize)>::Call;
    ioHeap.pfnFree = WITNESSADAPTER<&TestFreeLp, decltype(ioHeap.pfnFree)>::Call;
    ioHeap.pfnExtent = WITNESSADAPTER<&TestExtent, decltype(ioHeap.pfnExtent)>::Call;
    ioHeap.pfnCopy = WITNESSADAPTER<&TestguardedMemcpy, decltype(ioHeap.pfnCopy)>::Call;
    HEAPIOSCOPE scopeHeap(ioHeap);
    CONTROLIO   ioControl;
    ioControl.pfnGetDlgItem = WITNESSADAPTER<&TestGetDlgItem, decltype(ioControl.pfnGetDlgItem)>::Call;
    ioControl.pfnSendMessage = WITNESSADAPTER<&TestLUiSendMessage, decltype(ioControl.pfnSendMessage)>::Call;
    ioControl.pfnGetClientRect = WITNESSADAPTER<&TestGetClientRect, decltype(ioControl.pfnGetClientRect)>::Call;
    ioControl.pfnInvalidateRect = WITNESSADAPTER<&TestInvalidateRect, decltype(ioControl.pfnInvalidateRect)>::Call;
    ioControl.pfnGetAsyncKeyState = WITNESSADAPTER<&TestGetAsyncKeyState, decltype(ioControl.pfnGetAsyncKeyState)>::Call;
    ioControl.pfnMessageBeep = WITNESSADAPTER<&TestMessageBeep, decltype(ioControl.pfnMessageBeep)>::Call;
    ioControl.pfnFillProdSrcLB = WITNESSADAPTER<&TestFillProdSrcLB, decltype(ioControl.pfnFillProdSrcLB)>::Call;
    ioControl.pfnFillPlanetProdLB = WITNESSADAPTER<&TestFillPlanetProdLB, decltype(ioControl.pfnFillPlanetProdLB)>::Call;
    ioControl.pfnDrawProductionDlg = WITNESSADAPTER<&TestDrawProductionDlg, decltype(ioControl.pfnDrawProductionDlg)>::Call;
    CONTROLIOSCOPE scopeControl(ioControl);
    PRESENTATIONIO ioView;
    ioView.pfnTutor = WITNESSADAPTER<&TestAdvanceTutor, decltype(ioView.pfnTutor)>::Call;
    PRESENTATIONIOSCOPE scopeView(ioView);
    PARTQUERYIO         ioPartQuery;
    ioPartQuery.pfnLookup = WITNESSADAPTER<&TestFLookupPart, decltype(ioPartQuery.pfnLookup)>::Call;
    ioPartQuery.pfnHull = WITNESSADAPTER<&TestLphuldefFromId, decltype(ioPartQuery.pfnHull)>::Call;
    PARTQUERYIOSCOPE scopePartQuery(ioPartQuery);
    RACEQUERYIO      ioRaceQuery;
    ioRaceQuery.pfnStat = WITNESSADAPTER<&TestGetRaceStat, decltype(ioRaceQuery.pfnStat)>::Call;
    RACEQUERYIOSCOPE scopeRaceQuery(ioRaceQuery);
    COLONYIO         ioColony;
    ioColony.pfnWarp = WITNESSADAPTER<&TestIWarpMAFromLppl, decltype(ioColony.pfnWarp)>::Call;
    ioColony.pfnMaxFactories = WITNESSADAPTER<&TestCMaxFactories, decltype(ioColony.pfnMaxFactories)>::Call;
    ioColony.pfnMaxMines = WITNESSADAPTER<&TestCMaxMines, decltype(ioColony.pfnMaxMines)>::Call;
    ioColony.pfnMaxDefenses = WITNESSADAPTER<&TestCMaxDefenses, decltype(ioColony.pfnMaxDefenses)>::Call;
    ioColony.pfnTerraformPercent = WITNESSADAPTER<&TestIpctCanTerraformLppl, decltype(ioColony.pfnTerraformPercent)>::Call;
    COLONYIOSCOPE scopeColony(ioColony);
    app.state.idPlayer = 0;
    app.state.rglpshdefSB[0] = baseDefs;
    app.state.pProdGlob = catalog;
    CHECK(sizeof(PROD) == 4 && sizeof(PLPROD) == 4);
    reset(app, 0, 0);
    app.state.sel.pl.fNoResearch = 1;
    app.state.gd.fTutorial = 1;
    InitProduction(app, catalog);
    CHECK(app.state.gd.fNoResearchSav && app.state.lpplProdGlob->iprodMac == 0 && app.state.lpplProdGlob->iprodMax == 2 && app.state.cProdGlob == 9 && tutorials == 1);
    CHECK(catalog[0].iItem == 11 && catalog[1].iItem == 27 && catalog[2].iItem == 0 && catalog[8].iItem == 6);
    reset(app, 0, 1);
    app.state.sel.pl.isb = 15;
    InitProduction(app, catalog);
    CHECK(app.state.lpplProdGlob->iprodMax == 0 && app.state.lpplProdGlob->iprodMac == 0 && lastBytes == 4);
    reset(app, 0, 0);
    app.state.sel.pl.fStarbase = 1;
    genesis = 1;
    warp = 9;
    maxFactories = maxMines = 5000;
    maxDefenses = 1024;
    terraform = 1024;
    hullDefs[0].hul.wtCargoMax = 100;
    for (i = 0; i < 16; i++) {
        app.state.rgshdef[i].fFree = 0;
        app.state.rgshdef[i].hul.wtEmpty = 100;
    }
    for (i = 0; i < 10; i++)
        baseDefs[i].fFree = 0;
    InitProduction(app, catalog);
    CHECK(app.state.cProdGlob == 43);
    for (i = 0; i < 16; i++)
        CHECK(catalog[i].grobj == 2 && catalog[i].iItem == (unsigned)i && catalog[i].cItem == 1023);
    for (i = 16; i < 25; i++)
        CHECK(catalog[i].grobj == 2 && catalog[i].iItem == (unsigned)(i + 1) && catalog[i].cItem == 1);
    CHECK(catalog[25].iItem == 13 && catalog[26].iItem == 14 && catalog[29].iItem == 17);
    CHECK(catalog[30].iItem == 7 && catalog[30].cItem == 1020 && catalog[31].iItem == 8 && catalog[31].cItem == 1020);
    CHECK(catalog[32].iItem == 9 && catalog[32].cItem == 0 && catalog[35].iItem == 12 && catalog[35].cItem == 0);
    for (i = 43; i < 64; i++)
        CHECK(catalog[i].cItem == 0 && catalog[i].grobj == 0 && catalog[i].pct == 0 && catalog[i].unused == 0);
    reset(app, 0, 0);
    app.state.sel.pl.fStarbase = 1;
    hullDefs[0].hul.wtCargoMax = 100;
    for (i = 0; i < 4; i++) {
        app.state.rgshdef[i].fFree = 0;
        app.state.rgshdef[i].hul.wtEmpty = 100;
    }
    app.state.rgshdef[0].fGift = 1;
    app.state.rgshdef[1].hul.wtEmpty = 101;
    app.state.rgshdef[2].fFree = 1;
    InitProduction(app, catalog);
    CHECK(catalog[0].grobj == 2 && catalog[0].iItem == 3 && catalog[1].grobj == 1);
    reset(app, 0, 0);
    primary = 8;
    InitProduction(app, catalog);
    CHECK(app.state.cProdGlob == 5 && catalog[0].iItem == 11 && catalog[1].iItem == 3 && catalog[4].iItem == 6);
    reset(app, 0, 0);
    primary = 3;
    InitProduction(app, catalog);
    CHECK(app.state.cProdGlob == 7 && catalog[6].iItem == 6);
    reset(app, 0, 0);
    maxFactories = -32768;
    app.state.sel.pl.cFactories = 1;
    InitProduction(app, catalog);
    CHECK(catalog[0].iItem == 7 && catalog[0].cItem == 1020);
    // One following row is a defined non-overlapping copy, but pointer and index diverge.
    reset(app, 2, 1);
    maxFactories = 1;
    app.state.sel.pl.lpplprod->rgprod()[0] = bad;
    app.state.sel.pl.lpplprod->rgprod()[1] = a;
    app.state.sel.pl.lpplprod->rgprod()[1].pct = 66;
    app.state.sel.pl.lpplprod->rgprod()[1].unused = 17;
    InitProduction(app, catalog);
    CHECK(app.state.lpplProdGlob->iprodMac == 1 && app.state.lpplProdGlob->rgprod()[0].cItem == 10 && app.state.lpplProdGlob->rgprod()[1].cItem == 1);
    CHECK(app.state.lpplProdGlob->rgprod()[0].pct == 66 && app.state.lpplProdGlob->rgprod()[0].unused == 17 && catalog[0].cItem == 0 && app.state.sel.pl.lpplprod->rgprod()[1].cItem == 10);
    reset(app, 3, 1);
    maxFactories = 2;
    app.state.sel.pl.lpplprod->rgprod()[0] = bad;
    app.state.sel.pl.lpplprod->rgprod()[1] = a;
    app.state.sel.pl.lpplprod->rgprod()[2] = a;
    try {
        InitProduction(app, catalog);
        CHECK(0);
    } catch (const FIXTUREESCAPE &) {
    }
    CHECK(overlapBytes == 8 && overlapOffset == 0 && app.state.lpplProdGlob->iprodMac == 3 && catalog[0].cItem == 2);
    reset(app, 4, 1);
    maxFactories = 2;
    app.state.sel.pl.lpplprod->rgprod()[0] = row(1, 7, 1);
    app.state.sel.pl.lpplprod->rgprod()[1] = bad;
    app.state.sel.pl.lpplprod->rgprod()[2] = a;
    app.state.sel.pl.lpplprod->rgprod()[3] = a;
    try {
        InitProduction(app, catalog);
        CHECK(0);
    } catch (const FIXTUREESCAPE &) {
    }
    CHECK(overlapBytes == 8 && overlapOffset == 1 && catalog[0].cItem == 1);
    reset(app, 2, 1);
    maxFactories = 5;
    app.state.sel.pl.lpplprod->rgprod()[0] = row(3, 7, 1);
    app.state.sel.pl.lpplprod->rgprod()[1] = row(4, 7, 1);
    InitProduction(app, catalog);
    CHECK(app.state.lpplProdGlob->rgprod()[0].cItem == 3 && app.state.lpplProdGlob->rgprod()[1].cItem == 2 && catalog[0].cItem == 0);
    // All finite/unlimited source amounts and modifier combinations for new rows.
    for (mode = 0; mode < 4; mode++)
        for (available = 1; available <= 1023; available++) {
            working(app, 0);
            ctrl = (mode & 2) != 0;
            shift = (mode & 1) != 0;
            catalog[0] = row(available, 7, 1);
            app.state.cProdGlob = 1;
            request = ctrl ? (shift ? 1020 : 100) : shift ? 10 : 1;
            if (request > available)
                request = available;
            command(app, IDC_PRODUCTION_ADD);
            CHECK(app.state.lpplProdGlob->iprodMac == 1 && app.state.lpplProdGlob->rgprod()[0].cItem == (unsigned)request && app.state.lpplProdGlob->iprodMax == 4);
            CHECK(catalog[0].cItem == (unsigned)(available == 1023 ? 1023 : available - request));
            CHECK(queueFill == 1 && sourceFill == (catalog[0].cItem == 0) && redraw == 1 && setSelection == 1 && reallocations == 1);
        }
    working(app, 2);
    app.state.lpplProdGlob->rgprod()[0] = a;
    app.state.lpplProdGlob->rgprod()[1] = b;
    catalog[0] = c;
    app.state.cProdGlob = 1;
    queueSelection = 1;
    command(app, IDC_PRODUCTION_ADD);
    CHECK(app.state.lpplProdGlob->iprodMac == 3 && app.state.lpplProdGlob->rgprod()[0].iItem == 7 && app.state.lpplProdGlob->rgprod()[1].iItem == 9 && app.state.lpplProdGlob->rgprod()[2].iItem == 8 &&
          setSelection == 2);
    working(app, 2);
    app.state.lpplProdGlob->rgprod()[0] = a;
    app.state.lpplProdGlob->rgprod()[1] = b;
    catalog[0] = b;
    app.state.cProdGlob = 1;
    queueSelection = 1;
    command(app, 100);
    CHECK(app.state.lpplProdGlob->iprodMac == 2 && app.state.lpplProdGlob->rgprod()[1].cItem == 2 && reallocations == 0);
    working(app, 2);
    app.state.lpplProdGlob->rgprod()[0] = a;
    app.state.lpplProdGlob->rgprod()[1] = b;
    catalog[0] = c;
    app.state.cProdGlob = 1;
    queueSelection = 0;
    command(app, IDC_PRODUCTION_ADD);
    CHECK(app.state.lpplProdGlob->rgprod()[0].iItem == 9 && setSelection == 1);
    working(app, 2);
    app.state.lpplProdGlob->rgprod()[0] = a;
    app.state.lpplProdGlob->rgprod()[1] = b;
    catalog[0] = c;
    app.state.cProdGlob = 1;
    command(app, IDC_PRODUCTION_ADD);
    CHECK(app.state.lpplProdGlob->rgprod()[2].iItem == 9 && setSelection == 3);
    working(app, 40);
    for (i = 0; i < 40; i++)
        app.state.lpplProdGlob->rgprod()[i] = a;
    catalog[0] = c;
    app.state.cProdGlob = 1;
    queueSelection = 0;
    command(app, IDC_PRODUCTION_ADD);
    CHECK(app.state.lpplProdGlob->iprodMac == 40 && catalog[0].cItem == 0 && beeps == 1 && queueFill == 0 && sourceFill == 0 && redraw == 1);
    catalog[0] = a;
    command(app, IDC_PRODUCTION_ADD);
    CHECK(app.state.lpplProdGlob->iprodMac == 40 && app.state.lpplProdGlob->rgprod()[0].cItem == 11);
    working(app, 0);
    catalog[0] = row(0, 7, 1);
    catalog[1] = b;
    app.state.cProdGlob = 2;
    command(app, IDC_PRODUCTION_ADD);
    CHECK(app.state.lpplProdGlob->rgprod()[0].iItem == 8 && catalog[0].cItem == 0 && catalog[1].cItem == 0 && sourceFill == 1);
    working(app, 0);
    sourceSelection = -1;
    command(app, IDC_PRODUCTION_ADD);
    CHECK(app.state.lpplProdGlob->iprodMac == 0 && redraw == 0 && queueFill == 0);
    working(app, 0);
    app.state.cProdGlob = 0;
    command(app, IDC_PRODUCTION_ADD);
    CHECK(app.state.lpplProdGlob->rgprod()[0].cItem == 1 && catalog[0].cItem == 1023); // allocated inactive source slot
    working(app, 0);
    catalog[0] = row(1023, 3, 1);
    app.state.cProdGlob = 1;
    shift = ctrl = 1;
    command(app, IDC_PRODUCTION_ADD);
    CHECK(app.state.lpplProdGlob->rgprod()[0].cItem == 1020);
    command(app, IDC_PRODUCTION_ADD);
    CHECK(app.state.lpplProdGlob->rgprod()[0].cItem == 1);
    working(app, 0);
    catalog[0] = a;
    app.state.cProdGlob = 1;
    failReallocation = 1;
    try {
        command(app, IDC_PRODUCTION_ADD);
        CHECK(0);
    } catch (const FIXTUREESCAPE &) {
    }
    CHECK(catalog[0].cItem == 9 && app.state.lpplProdGlob->iprodMac == 0 && queueFill == 0 && redraw == 0);
    working(app, 1);
    app.state.lpplProdGlob->rgprod()[0] = row(1023, 7, 1);
    app.state.lpplProdGlob->rgprod()[0].pct = 75;
    catalog[0] = a;
    app.state.cProdGlob = 1;
    queueSelection = 1;
    command(app, IDC_PRODUCTION_ADD);
    CHECK(app.state.lpplProdGlob->rgprod()[0].cItem == 1020 && app.state.lpplProdGlob->rgprod()[0].pct == 75 && catalog[0].cItem == 9);
    // Remove uses native ten-bit return and preserves progress/unused on survivors.
    for (mode = 0; mode < 4; mode++)
        for (available = 0; available <= 1023; available++) {
            working(app, 1);
            ctrl = (mode & 2) != 0;
            shift = (mode & 1) != 0;
            app.state.lpplProdGlob->rgprod()[0] = row(available, 7, 1);
            catalog[0] = row(1020, 7, 1);
            app.state.cProdGlob = 1;
            queueSelection = 1;
            request = ctrl ? (shift ? 1020 : 100) : shift ? 10 : 1;
            if (request > available)
                request = available;
            command(app, IDC_PRODUCTION_REMOVE);
            CHECK(app.state.lpplProdGlob->iprodMac == (available > request) && catalog[0].cItem == (unsigned)((1020 + request) % 1024));
            if (available > request)
                CHECK(app.state.lpplProdGlob->rgprod()[0].cItem == (unsigned)(available - request));
        }
    working(app, 1);
    app.state.lpplProdGlob->rgprod()[0] = row(4, 7, 1);
    catalog[0] = row(1020, 7, 1);
    app.state.cProdGlob = 1;
    ctrl = 1;
    queueSelection = 1;
    command(app, IDC_PRODUCTION_REMOVE);
    CHECK(app.state.lpplProdGlob->iprodMac == 0 && catalog[0].cItem == 0 && setSelection == -99 && queueFill == 1 && sourceFill == 0);
    working(app, 1);
    app.state.lpplProdGlob->rgprod()[0] = row(1023, 3, 1);
    catalog[0] = row(1023, 3, 1);
    app.state.cProdGlob = 1;
    queueSelection = 1;
    command(app, 101);
    CHECK(app.state.lpplProdGlob->iprodMac == 1 && app.state.lpplProdGlob->rgprod()[0].cItem == 3 && catalog[0].cItem == 1023);
    working(app, 2);
    app.state.lpplProdGlob->rgprod()[0] = b;
    app.state.lpplProdGlob->rgprod()[1] = c;
    catalog[0] = row(0, 8, 1);
    app.state.cProdGlob = 1;
    queueSelection = 1;
    command(app, IDC_PRODUCTION_REMOVE);
    CHECK(app.state.lpplProdGlob->iprodMac == 1 && app.state.lpplProdGlob->rgprod()[0].iItem == 9 && setSelection == 1 && sourceFill == 1);
    working(app, 1);
    app.state.lpplProdGlob->rgprod()[0] = a;
    app.state.lpplProdGlob->rgprod()[0].pct = 66;
    app.state.lpplProdGlob->rgprod()[0].unused = 17;
    catalog[0] = a;
    app.state.cProdGlob = 1;
    queueSelection = 1;
    command(app, IDC_PRODUCTION_REMOVE);
    CHECK(app.state.lpplProdGlob->rgprod()[0].cItem == 9 && app.state.lpplProdGlob->rgprod()[0].pct == 66 && app.state.lpplProdGlob->rgprod()[0].unused == 17);
    working(app, 2);
    app.state.lpplProdGlob->rgprod()[0] = a;
    app.state.lpplProdGlob->rgprod()[1] = b;
    queueSelection = 1;
    command(app, IDC_PRODUCTION_ITEM_DOWN);
    CHECK(app.state.lpplProdGlob->rgprod()[0].iItem == 8 && app.state.lpplProdGlob->rgprod()[1].iItem == 7 && setSelection == 2);
    command(app, IDC_PRODUCTION_ITEM_UP);
    CHECK(app.state.lpplProdGlob->rgprod()[0].iItem == 7 && setSelection == 1);
    reset(app, 0, 0);
    printf("M09 linked editor witnesses: %d assertions passed\n", checks);
    return 0;
}
