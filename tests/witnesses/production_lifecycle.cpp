#include "ui_fixture.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
static PLANET   planets[8];
static SHDEF    designs[8];
static HULDEF   hulls[8];
static char     trace[8192];
static int      checks, allocations, alive[128], frees, failAt, mutateWrite, mutateReload, mutateTutorial;
static int      modalNext, shiftKey, queueSelection, sourceSelection, researchCheck, initCalls, writeCount, initialSelections;
static intptr_t modalResult;
static int      writeResult, readResult;
static unsigned storedCounts[8], storedResearch[8];
static void    *allocs[128];
static void     event(const char *s) {
    assert(strlen(trace) + strlen(s) < sizeof(trace));
    strcat(trace, s);
}
static PLPROD *queue(unsigned count) {
    assert(allocations < 128 && count <= 255);
    PLPROD *p = (PLPROD *)calloc(1, 4 + count * 4);
    assert(p);
    p = LpStartPl<PLPROD, PROD>(p, 4 + count * 4);
    p->cbItem = 4;
    p->iprodMac = count;
    p->iprodMax = count;
    allocs[allocations] = p;
    alive[allocations++] = 1;
    return p;
}
static int allocation(void *p) {
    for (int i = 0; i < allocations; i++)
        if (allocs[i] == p)
            return i;
    abort();
}
static void TestFreeLp(APPSESSION &, void *p, int ht) {
    (void)ht;
    int i = allocation(p);
    assert(alive[i]);
    alive[i] = 0;
    frees++;
    event("F;");
}
static HULDEF *TestLphuldefFromId(int i) {
    assert(i >= 0 && i < 8);
    return &hulls[i];
}
static void fail(int stage) {
    if (failAt == stage)
        StarsThrowError(starsErrorMemory, 0);
}
static void TestInitProduction(APPSESSION &app, PROD *rows) {
    event(rows ? "I;" : "i;");
    initCalls++;
    app.state.gd.fNoResearchSav = app.state.sel.pl.fNoResearch;
    fail(rows ? 1 : 5);
    unsigned n = app.state.sel.pl.lpplprod ? app.state.sel.pl.lpplprod->iprodMac : 0;
    app.state.lpplProdGlob = queue(n);
    if (n)
        memcpy(app.state.lpplProdGlob->rgprod(), app.state.sel.pl.lpplprod->rgprod(), n * 4);
}
static int TestFLookupPlanet(APPSESSION &app, int id, PLANET *p) {
    if (id == idWriteBack) {
        event("W;");
        fail(3);
        if (!writeResult)
            return 0;
        writeCount++;
        assert(p->id >= 0 && p->id < 8);
        storedCounts[p->id] = p->lpplprod ? p->lpplprod->iprodMac : 0;
        storedResearch[p->id] = p->fNoResearch;
        if (mutateWrite)
            p->id = 6;
    } else {
        char text[20];
        snprintf(text, sizeof(text), "L%d;", id);
        event(text);
        fail(4);
        if (mutateReload)
            app.state.fAi = 1;
    }
    return id == idWriteBack ? writeResult : readResult;
}
static void TestFillSelProdLB(APPSESSION &) { event("Fill;"); }
static void TestRedrawPlanShip(APPSESSION &, int tile) {
    assert(tile == tileProductionOrOrbit);
    event("Redraw;");
}
static void TestAdvanceTutor(APPSESSION &app) {
    assert(app.state.tutor.fProgress);
    event("Tutor;");
    if (mutateTutorial) {
        app.state.sel.grobj = grobjFleet;
        app.state.lpplProdGlob = queue(1);
    }
}
static void TestDrawPlanShip(APPSESSION &, void *hdc, int tile) {
    assert(!hdc && tile == tilePlanetStats);
    event("Stats;");
}
static void TestEndDialog(HWND hwnd, int result) {
    assert(hwnd == HwndWitness(7));
    event(result ? "End1;" : "End0;");
}
static void TestStickyDlgPos(HWND hwnd, POINT16 *point, int restore) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    assert(hwnd == HwndWitness(7) && point == &app.lpui->ptStickyProduceDlg && !restore);
    event("Sticky;");
}
static const char *TestPszFormatIds(APPSESSION &, int id, void *p) {
    (void)p;
    assert(id == idsThereIsntEnoughFreeMemoryModifyProduction);
    return "memory";
}
static void TestAlertSz(APPSESSION &, const char *s, int flags) {
    assert(strcmp(s, "memory") == 0 && flags == win32::MB_ICONHAND);
    event("Alert;");
}
static int TestGetDlgItem(HWND hwnd, int id) {
    assert(hwnd == HwndWitness(7));
    return id;
}
static std::array<char, cchCatalogText> TestPszGetCompressedString(APPSESSION &, StringId id) {
    assert(id == idsProductionQueueS);
    return SzWitnessText("Production %s");
}
static const char *TestPszGetPlanetName(APPSESSION &, int id) {
    (void)id;
    return "Test";
}
static void TestFUiSetWindowText(HWND hwnd, const char *s) {
    assert(hwnd == HwndWitness(7) && strcmp(s, "Production Test") == 0);
    event("Title;");
}
static void TestFillProdSrcLB(APPSESSION &, HWND hwnd, int row) {
    assert(WitnessValue<int>(hwnd) == IDC_PRODUCTION_AVAILABLE_ITEMS && row == -1);
    event("Source;");
}
static void TestFillPlanetProdLB(APPSESSION &app, HWND hwnd, PLPROD *p, void *unused) {
    assert(WitnessValue<int>(hwnd) == IDC_PRODUCTION_QUEUE && p == app.state.lpplProdGlob && !unused);
    event("Queue;");
}
static intptr_t TestLUiSendMessage(HWND hwnd, int msg, intptr_t value, intptr_t param) {
    assert(!param);
    if (msg == win32::LB_SETCURSEL && WitnessValue<int>(hwnd) == IDC_PRODUCTION_AVAILABLE_ITEMS) {
        sourceSelection = value;
        event("Src0;");
    } else if (msg == win32::LB_SETCURSEL && WitnessValue<int>(hwnd) == IDC_PRODUCTION_QUEUE) {
        queueSelection = value;
        initialSelections++;
        event("QSel;");
    } else {
        assert(msg == win32::BM_SETCHECK && WitnessValue<int>(hwnd) == IDC_PRODUCTION_RESEARCH_LEFTOVERS_ONLY);
        researchCheck = value;
        event("Check;");
    }
    return 0;
}
static int TestGetKeyState(int key) {
    assert(key == win32::VK_SHIFT);
    return shiftKey ? -1 : 0;
}
static void TestSelectAdjPlanet(APPSESSION &app, int16_t delta, int16_t id) {
    char text[30];
    snprintf(text, sizeof(text), "Select%d:%d;", delta, id);
    event(text);
    if (id == idplNone)
        return;
    app.state.sel.pl.id = delta ? (app.state.sel.pl.id + delta + 8) % 8 : id;
    app.state.sel.pl.fNoResearch = storedResearch[app.state.sel.pl.id];
    app.state.sel.pl.lpplprod = queue(storedCounts[app.state.sel.pl.id]);
}
static void TestGetClientRect(HWND hwnd, RECT *r) {
    assert(hwnd == HwndWitness(7));
    event("Rect;");
    *r = RECT{0, 0, 800, 600};
}
static void TestInvalidateRect(HWND hwnd, RECT *r, int erase) {
    assert(hwnd == HwndWitness(7) && erase && r->top == 20 && r->bottom == 110);
    event("Paint;");
}

#define CHECK(x)                                                                                                                                               \
    do {                                                                                                                                                       \
        checks++;                                                                                                                                              \
        if (!(x)) {                                                                                                                                            \
            fprintf(stderr, "witness %d failed: %s; trace=%s\n", checks, #x, trace);                                                                           \
            return 1;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)

static intptr_t TestLUiDialogBox(HINSTANCE instance, LPCSTR resource, HWND parent, DLGPROC proc) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    assert(instance == app.lpui->hInst && resource == win32::MAKEINTRESOURCE(IDD_PRODUCTION) && parent == app.lpui->hwndFrame && proc == ProductionDlg && app.lpui->fDlgUp);
    event("Make;Modal;");
    app.lpui->hwndProdDlg = HwndWitness(7);
    app.state.sel.pl.fNoResearch = 1;
    fail(2);
    if (modalNext) {
        ProdCommandHandler(app, HwndWitness(7), modalNext > 0 ? IDC_NEXT : IDC_BACK, 0);
        app.state.sel.pl.fNoResearch = !app.state.gd.fNoResearchSav;
    }
    ProdCommandHandler(app, HwndWitness(7), modalResult == 0 ? IDCANCEL : IDOK, 0);
    event("Unmake;");
    return modalResult;
}
static void reset(APPSESSION &app) {
    for (int i = 0; i < allocations; i++)
        free(allocs[i]);
    allocations = frees = failAt = mutateWrite = mutateReload = mutateTutorial = modalNext = shiftKey = 0;
    writeCount = initCalls = initialSelections = 0;
    writeResult = readResult = 1;
    modalResult = 0;
    trace[0] = 0;
    memset(&app.state.sel, 0, sizeof(app.state.sel));
    memset(&app.state.gd, 0, sizeof(app.state.gd));
    memset(&app.state.tutor, 0, sizeof(app.state.tutor));
    memset(storedCounts, 0, sizeof(storedCounts));
    memset(storedResearch, 0, sizeof(storedResearch));
    memset(planets, 0, sizeof(planets));
    memset(hulls, 0, sizeof(hulls));
    app.state.lpplProdGlob = NULL;
    app.state.sel.pl.id = 2;
    app.state.sel.grobj = grobjPlanet;
    app.state.sel.pl.lpplprod = queue(2);
    app.lpui->hwndProdDlg = 0;
    app.lpui->fDlgUp = 0;
    app.state.idPlayer = app.state.fAi = app.state.cPlanet = 0;
    queueSelection = sourceSelection = researchCheck = -99;
}
static int referenceTarget(APPSESSION &app, int current, int next) {
    int eligible[8], n = 0;
    for (int i = 0; i < app.state.cPlanet; i++)
        if (planets[i].iPlayer == app.state.idPlayer && planets[i].fStarbase && hulls[designs[planets[i].isb].hul.ihuldef].hul.wtCargoMax)
            eligible[n++] = planets[i].id;
    if (!n)
        return -1;
    if (next) {
        for (int i = 0; i < n; i++)
            if (eligible[i] > current)
                return eligible[i];
        return eligible[0];
    }
    for (int i = n - 1; i >= 0; i--)
        if (eligible[i] < current)
            return eligible[i];
    return eligible[n - 1];
}
int main(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    HEAPIO ioHeap;
    ioHeap.pfnFree = WITNESSADAPTER<&TestFreeLp, decltype(ioHeap.pfnFree)>::Call;
    HEAPIOSCOPE scopeHeap(ioHeap);
    CONTROLIO   ioControl;
    ioControl.pfnGetDlgItem = WITNESSADAPTER<&TestGetDlgItem, decltype(ioControl.pfnGetDlgItem)>::Call;
    ioControl.pfnSendMessage = WITNESSADAPTER<&TestLUiSendMessage, decltype(ioControl.pfnSendMessage)>::Call;
    ioControl.pfnGetClientRect = WITNESSADAPTER<&TestGetClientRect, decltype(ioControl.pfnGetClientRect)>::Call;
    ioControl.pfnInvalidateRect = WITNESSADAPTER<&TestInvalidateRect, decltype(ioControl.pfnInvalidateRect)>::Call;
    ioControl.pfnGetKeyState = WITNESSADAPTER<&TestGetKeyState, decltype(ioControl.pfnGetKeyState)>::Call;
    ioControl.pfnSetWindowText = WITNESSADAPTER<&TestFUiSetWindowText, decltype(ioControl.pfnSetWindowText)>::Call;
    ioControl.pfnDialogBox = WITNESSADAPTER<&TestLUiDialogBox, decltype(ioControl.pfnDialogBox)>::Call;
    ioControl.pfnEndDialog = WITNESSADAPTER<&TestEndDialog, decltype(ioControl.pfnEndDialog)>::Call;
    ioControl.pfnFillProdSrcLB = WITNESSADAPTER<&TestFillProdSrcLB, decltype(ioControl.pfnFillProdSrcLB)>::Call;
    ioControl.pfnFillPlanetProdLB = WITNESSADAPTER<&TestFillPlanetProdLB, decltype(ioControl.pfnFillPlanetProdLB)>::Call;
    ioControl.pfnStickyDlgPos = WITNESSADAPTER<&TestStickyDlgPos, decltype(ioControl.pfnStickyDlgPos)>::Call;
    ioControl.pfnDrawPlanShip = WITNESSADAPTER<&TestDrawPlanShip, decltype(ioControl.pfnDrawPlanShip)>::Call;
    CONTROLIOSCOPE scopeControl(ioControl);
    PRESENTATIONIO ioView;
    ioView.pfnTutor = WITNESSADAPTER<&TestAdvanceTutor, decltype(ioView.pfnTutor)>::Call;
    ioView.pfnRedraw = WITNESSADAPTER<&TestRedrawPlanShip, decltype(ioView.pfnRedraw)>::Call;
    ioView.pfnSelectPlanet = WITNESSADAPTER<&TestSelectAdjPlanet, decltype(ioView.pfnSelectPlanet)>::Call;
    PRESENTATIONIOSCOPE scopeView(ioView);
    PRODUCTIONIO        ioProduction;
    ioProduction.pfnInitialize = WITNESSADAPTER<&TestInitProduction, decltype(ioProduction.pfnInitialize)>::Call;
    ioProduction.pfnRefresh = WITNESSADAPTER<&TestFillSelProdLB, decltype(ioProduction.pfnRefresh)>::Call;
    PRODUCTIONIOSCOPE scopeProduction(ioProduction);
    PLANETQUERYIO     ioPlanetQuery;
    ioPlanetQuery.pfnLookup = WITNESSADAPTER<&TestFLookupPlanet, decltype(ioPlanetQuery.pfnLookup)>::Call;
    PLANETQUERYIOSCOPE scopePlanetQuery(ioPlanetQuery);
    PARTQUERYIO        ioPartQuery;
    ioPartQuery.pfnHull = WITNESSADAPTER<&TestLphuldefFromId, decltype(ioPartQuery.pfnHull)>::Call;
    PARTQUERYIOSCOPE scopePartQuery(ioPartQuery);
    TEXTIO           ioText;
    ioText.pfnCompressed = TestPszGetCompressedString;
    ioText.pfnFormat = WITNESSADAPTER<&TestPszFormatIds, decltype(ioText.pfnFormat)>::Call;
    ioText.pfnPlanetName = WITNESSADAPTER<&TestPszGetPlanetName, decltype(ioText.pfnPlanetName)>::Call;
    TEXTIOSCOPE scopeText(ioText);
    ALERTIO     ioAlert;
    ioAlert.pfnAlertSz = WITNESSADAPTER<&TestAlertSz, decltype(ioAlert.pfnAlertSz)>::Call;
    ALERTIOSCOPE scopeAlert(ioAlert);
    app.state.lpPlanets = planets;
    app.state.rglpshdefSB[0] = designs;
    app.lpui->hwndFrame = HwndWitness(99);
    app.lpui->yTopFutureTech = 20;
    app.state.dyArial8 = 10;
    static_assert(sizeof(PROD) == 4, "PROD width");
    static_assert(sizeof(PLPROD) == 4, "PLPROD header");
    for (int accept = 0; accept < 2; accept++)
        for (int empty = 0; empty < 3; empty++)
            for (int ai = 0; ai < 2; ai++)
                for (int tutorial = 0; tutorial < 2; tutorial++)
                    for (int player = 0; player < 2; player++) {
                        reset(app);
                        PLPROD *old = app.state.sel.pl.lpplprod, *work = empty == 2 ? NULL : queue(empty == 0 ? 2 : 0);
                        app.state.lpplProdGlob = work;
                        app.state.sel.pl.fNoResearch = 1;
                        app.state.gd.fNoResearchSav = 0;
                        app.state.gd.fTutorial = tutorial;
                        app.state.idPlayer = player;
                        app.state.fAi = ai;
                        FinishProduction(app, accept);
                        CHECK(!app.state.lpplProdGlob && app.state.sel.pl.fNoResearch == (unsigned)accept);
                        CHECK(alive[allocation(old)] == !accept);
                        if (accept) {
                            CHECK(app.state.sel.pl.lpplprod == (empty == 0 ? work : NULL));
                            CHECK(writeCount == 1);
                            CHECK(strstr(trace, "W;L2;") != NULL);
                            CHECK((strstr(trace, "Fill;Redraw;") != NULL) == !ai);
                        } else {
                            CHECK(app.state.sel.pl.lpplprod == old && !writeCount);
                        }
                        if (work)
                            CHECK(alive[allocation(work)] == (accept && empty == 0));
                        CHECK((strstr(trace, "Tutor;") != NULL) == (tutorial && player == 0));
                    }
    for (int writeOk = 0; writeOk < 2; writeOk++)
        for (int readOk = 0; readOk < 2; readOk++) {
            reset(app);
            app.state.lpplProdGlob = queue(1);
            writeResult = writeOk;
            readResult = readOk;
            FinishProduction(app, 1);
            CHECK(strstr(trace, "W;L2;Fill;Redraw;") != NULL);
            CHECK(writeCount == writeOk && !app.state.lpplProdGlob);
        }
    reset(app);
    app.state.lpplProdGlob = queue(1);
    mutateWrite = mutateReload = 1;
    FinishProduction(app, 1);
    CHECK(strstr(trace, "W;L6;") && !strstr(trace, "Fill;"));
    reset(app);
    app.state.gd.fTutorial = mutateTutorial = 1;
    app.state.lpplProdGlob = queue(1);
    FinishProduction(app, 0);
    CHECK(app.state.lpplProdGlob && app.state.sel.grobj == 2); // tutorial can repopulate after final null
    const intptr_t results[] = {0, 1, -1, 65536, 65537, -65536};
    for (unsigned i = 0; i < sizeof(results) / sizeof(results[0]); i++) {
        reset(app);
        modalResult = results[i];
        PLPROD *old = app.state.sel.pl.lpplprod;
        CHECK(ChangeProduction(app, 0) == 1 && !FStarsRecoveryActive() && !app.lpui->hwndProdDlg && !app.lpui->fDlgUp && !app.state.lpplProdGlob);
        int accept = (int16_t)results[i] != 0;
        CHECK(writeCount == accept && app.state.sel.pl.fNoResearch == (unsigned)accept);
        CHECK(alive[allocation(old)] == !accept);
        CHECK((strstr(trace, "Stats;") != NULL) == accept);
        CHECK(strstr(trace, "Unmake;") != NULL && initCalls == 1);
    }
    reset(app);
    app.state.sel.pl.fNoResearch = 1;
    CHECK(ChangeProduction(app, 1) == 1);
    CHECK(!app.state.sel.pl.lpplprod && app.state.sel.pl.fNoResearch && !initCalls && !strstr(trace, "Modal;") && writeCount == 1);
    reset(app);
    app.state.lpplProdGlob = queue(1);
    PLPROD *preexisting = app.state.lpplProdGlob;
    CHECK(ChangeProduction(app, 1) == 1);
    CHECK(app.state.sel.pl.lpplprod == preexisting); // direct Clear assumes global null; does not enforce it
    for (int stage = 1; stage <= 4; stage++) {
        reset(app);
        failAt = stage;
        modalResult = 1;
        PLPROD *old = app.state.sel.pl.lpplprod;
        CHECK(ChangeProduction(app, 0) == 0 && !FStarsRecoveryActive() && !app.state.lpplProdGlob && !app.lpui->hwndProdDlg && !app.lpui->fDlgUp);
        CHECK(app.state.sel.pl.fNoResearch == (unsigned)(stage != 1));
        CHECK(strstr(trace, "Alert;") != NULL);
        CHECK((strstr(trace, "End0;") != NULL) == (stage == 2));
        CHECK(alive[allocation(old)] == (stage <= 2));
        if (stage >= 3)
            CHECK(app.state.sel.pl.lpplprod && alive[allocation(app.state.sel.pl.lpplprod)]);
    }
    reset(app);
    modalNext = 1;
    modalResult = 0;
    storedCounts[3] = 1;
    CHECK(ChangeProduction(app, 0) == 1);
    CHECK(storedCounts[2] == 2 && storedResearch[2] == 1);
    CHECK(app.state.sel.pl.id == 3 && app.state.sel.pl.fNoResearch == 0 && writeCount == 1 && initCalls == 2);
    CHECK(strstr(trace, "W;L2;Fill;Redraw;Select1:0;i;Title;Source;Src0;Queue;QSel;Check;Rect;Paint;") != NULL);
    reset(app);
    modalNext = -1;
    failAt = 5;
    modalResult = 0;
    CHECK(ChangeProduction(app, 0) == 0 && writeCount == 1 && storedResearch[2] == 1 && app.state.sel.pl.id == 1 && !app.state.lpplProdGlob);
    CHECK(strstr(trace, "Select-1:0;i;End0;Alert;") != NULL);
    reset(app);
    shiftKey = 1;
    modalNext = 1;
    modalResult = 0;
    CHECK(ChangeProduction(app, 0) == 1 && app.state.sel.pl.id == 2 && initCalls == 2 && writeCount == 1);
    CHECK(strstr(trace, "Select0:-1;i;") != NULL); // no base still commits and reinitializes
    reset(app);
    app.state.gd.fTutorial = mutateTutorial = 1;
    modalResult = 1;
    CHECK(ChangeProduction(app, 0) == 1 && app.state.sel.grobj == 2 && !strstr(trace, "Stats;") && app.state.lpplProdGlob);
    for (int count = 0; count <= 8; count++)
        for (unsigned mask = 0; mask < (1u << count); mask++) {
            reset(app);
            app.state.lpplProdGlob = queue(count);
            int expected = 0;
            for (int i = 0; i < count; i++) {
                app.state.lpplProdGlob->rgprod()[i].grobj = 1;
                app.state.lpplProdGlob->rgprod()[i].iItem = (mask & (1u << i)) ? 7 : 0;
                if (mask & (1u << i))
                    expected = i + 1;
            }
            app.state.sel.pl.fNoResearch = 1;
            InitializeProductionDlg(app, HwndWitness(7));
            CHECK(queueSelection == expected && sourceSelection == 0 && researchCheck == 1 && initialSelections == 1);
        }
    reset(app);
    app.state.lpplProdGlob = queue(2);
    app.state.lpplProdGlob->rgprod()[0].grobj = 2;
    app.state.lpplProdGlob->rgprod()[1].grobj = 1;
    InitializeProductionDlg(app, HwndWitness(7));
    CHECK(queueSelection == 1);
    const int orders[3][4] = {{1, 3, 5, 7}, {7, 1, 5, 3}, {5, 3, 7, 1}};
    for (int order = 0; order < 3; order++)
        for (unsigned mask = 0; mask < 256; mask++)
            for (int current = -1; current <= 8; current++)
                for (int next = 0; next < 2; next++) {
                    reset(app);
                    app.state.cPlanet = 4;
                    for (int i = 0; i < 4; i++) {
                        unsigned mode = (mask >> (2 * i)) & 3;
                        planets[i].id = orders[order][i];
                        planets[i].iPlayer = mode == 1 ? 1 : 0;
                        planets[i].fStarbase = mode != 2;
                        planets[i].isb = i;
                        designs[i].hul.ihuldef = static_cast<HulDef>(i);
                        hulls[i].hul.wtCargoMax = mode == 3 ? 0 : 100;
                    }
                    CHECK(IdFindAdjStarbase(app, current, next) == referenceTarget(app, current, next));
                }
    reset(app);
    for (int i = 0; i < allocations; i++)
        free(allocs[i]);
    printf("M09 lifecycle/M07 starbase linked witnesses: %d passed\n", checks);
    return 0;
}
