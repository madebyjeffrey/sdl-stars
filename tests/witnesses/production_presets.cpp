#include <map>
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include "productionpresets.h"
#include "ui_fixture.h"
static PROD catalog[64];
static int  primary, freed, ended, alerts, allocations, failAllocation;
static char renameText[13], label[80], labels[12][80];
struct FIXTUREESCAPE {};
static std::map<const void *, uint16_t> extents;
static int                              renameCalls, renameResult, outerResult, checks, enabledDelete, enabledRename;
static int                              labelCount;
static size_t                           lastBytes;
static void                            *TestLpAlloc(APPSESSION &, uint16_t cb, HeapType ht) {
    (void)ht;
    allocations++;
    lastBytes = cb;
    if (failAllocation)
        throw FIXTUREESCAPE{};
    void *p = calloc(1, cb);
    if (!p)
        abort();
    extents[p] = cb;
    return p;
}
static void TestFreeLp(APPSESSION &, void *p, HeapType ht) {
    (void)ht;
    freed++;
    extents.erase(p);
    free(p);
}
static int TestGetRaceStat(void *p, int stat) {
    (void)p;
    (void)stat;
    return primary;
}
static int TestGetDlgItem(HWND hwnd, int id) {
    (void)hwnd;
    return id;
}
static void TestLUiSendMessage(HWND hwnd, int message, int value, LPARAM param) {
    (void)hwnd;
    (void)value;
    if (message == win32::LB_RESETCONTENT)
        labelCount = 0;
    if (message == win32::LB_ADDSTRING) {
        if (labelCount >= 12)
            abort();
        strcpy(labels[labelCount++], (const char *)param);
    }
}
static void TestFillPlanetProdLB(APPSESSION &, HWND hwnd, PLPROD *q, void *p) {
    (void)hwnd;
    (void)q;
    (void)p;
}
static void TestFillProdSrcLB(APPSESSION &, HWND hwnd, int n) {
    (void)hwnd;
    (void)n;
}
static void TestFUiSetWindowText(HWND hwnd, const char *s) {
    (void)hwnd;
    strcpy(label, s);
}
static void TestHwndUiSetFocus(HWND hwnd) { (void)hwnd; }
static void TestFUiEnableWindow(HWND hwnd, int enabled) {
    if (WitnessValue<int>(hwnd) == IDC_DELETE)
        enabledDelete = enabled;
    if (WitnessValue<int>(hwnd) == IDC_RENAME)
        enabledRename = enabled;
}
static void TestGetClientRect(HWND hwnd, RECT *rc) {
    (void)hwnd;
    memset(rc, 0, sizeof(*rc));
}
static void TestInvalidateRect(HWND hwnd, RECT *rc, int erase) {
    (void)hwnd;
    (void)rc;
    (void)erase;
}
static std::array<char, cchCatalogText> TestPszGetCompressedString(APPSESSION &, StringId id) {
    if (id == idsCustomD)
        return SzWitnessText("Custom%d");
    if (id == idsUnusedD)
        return SzWitnessText("Unused%d");
    return SzWitnessText("Automatic orders");
}
static std::int16_t TestCchGetString(APPSESSION &, StringId id, std::span<char> out) {
    char szItem[16];
    if (id == idsSD2)
        return static_cast<std::int16_t>(CchCopyUtf8(out, "%s %d"));
    snprintf(szItem, sizeof(szItem), "Item%d", static_cast<int>(id) - static_cast<int>(idsMines));
    return static_cast<std::int16_t>(CchCopyUtf8(out, std::string_view(szItem).substr(0, 14)));
}
static int TestLUiDialogBox(int instance, int resource, HWND hwnd, int proc) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    (void)instance;
    (void)hwnd;
    (void)proc;
    if (resource == IDD_RENAME) {
        renameCalls++;
        if (renameResult)
            strcpy(app.state.szWork, renameText);
        return renameResult;
    }
    memset(app.state.vrgZipProd, 0x5a, 4 * sizeof(ZIPPRODQ));
    app.state.vrgZipProd[4].cpq = 9;
    app.state.gd.fChgZipProd = 1;
    return outerResult;
}
static void TestEndDialog(HWND hwnd, int result) {
    (void)hwnd;
    (void)result;
    ended++;
}
static std::array<char, 256> TestPszFormatIds(APPSESSION &, int id, void *p) {
    (void)id;
    (void)p;
    return SzWitnessText("memory");
}
static void TestAlertSz(APPSESSION &, const char *s, int flags) {
    (void)s;
    (void)flags;
    alerts++;
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
static void     TestDrawProductionDlg(APPSESSION &, HWND, HDC, RECT *, int16_t) {}
static void     apply(APPSESSION &app, int cmd, int param) { ProdCommandHandler(app, HwndWitness(1), cmd, param); }
static int      popupSlot(APPSESSION &app, int i) { return IProductionPresetSlot(app, (int16_t)i); }
static void     edit(int cmd) { ZipProdDlg(HwndWitness(1), win32::WM_COMMAND, cmd, 0); }
static void     customize(APPSESSION &app) { CustomizeProductionPresets(app, HwndWitness(1)); }
static void     FailRecovery(APPSESSION &, PROD *) { StarsThrowError(starsErrorMemory, 0); }
static int      recover(APPSESSION &app) {
    PRODUCTIONIO io = ProductionIO();
    io.pfnInitialize = FailRecovery;
    PRODUCTIONIOSCOPE recoveryScope(io);
    return ChangeProduction(app, 0);
}

static void reset(APPSESSION &app, unsigned n) {
    if (app.state.lpplProdGlob)
        FreePl(app, app.state.lpplProdGlob);
    failAllocation = 0;
    app.state.lpplProdGlob = LpplAlloc<PLPROD>(app, 4, n ? n : 1, htOrd);
    app.state.lpplProdGlob->iprodMac = n;
    memset(app.state.pProdGlob, 0, sizeof(catalog));
    memset(app.state.vrgZipProd, 0, sizeof(app.state.vrgZipProd));
    app.state.cProdGlob = 0;
    primary = 0;
    app.state.sel.pl.fNoResearch = 0;
    app.state.gd.fChgZipProd = 0;
    freed = ended = alerts = allocations = renameCalls = 0;
    renameResult = 1;
    renameText[0] = 0;
    label[0] = 0;
}

int main(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    int    i, n, k;
    HEAPIO ioHeap;
    ioHeap.pfnAllocate = WITNESSADAPTER<&TestLpAlloc, decltype(ioHeap.pfnAllocate)>::Call;
    ioHeap.pfnFree = WITNESSADAPTER<&TestFreeLp, decltype(ioHeap.pfnFree)>::Call;
    ioHeap.pfnExtent = WITNESSADAPTER<&TestExtent, decltype(ioHeap.pfnExtent)>::Call;
    HEAPIOSCOPE scopeHeap(ioHeap);
    CONTROLIO   ioControl;
    ioControl.pfnGetDlgItem = WITNESSADAPTER<&TestGetDlgItem, decltype(ioControl.pfnGetDlgItem)>::Call;
    ioControl.pfnSendMessage = WITNESSADAPTER<&TestLUiSendMessage, decltype(ioControl.pfnSendMessage)>::Call;
    ioControl.pfnGetClientRect = WITNESSADAPTER<&TestGetClientRect, decltype(ioControl.pfnGetClientRect)>::Call;
    ioControl.pfnInvalidateRect = WITNESSADAPTER<&TestInvalidateRect, decltype(ioControl.pfnInvalidateRect)>::Call;
    ioControl.pfnSetWindowText = WITNESSADAPTER<&TestFUiSetWindowText, decltype(ioControl.pfnSetWindowText)>::Call;
    ioControl.pfnSetFocus = WITNESSADAPTER<&TestHwndUiSetFocus, decltype(ioControl.pfnSetFocus)>::Call;
    ioControl.pfnEnableWindow = WITNESSADAPTER<&TestFUiEnableWindow, decltype(ioControl.pfnEnableWindow)>::Call;
    ioControl.pfnDialogBox = WITNESSADAPTER<&TestLUiDialogBox, decltype(ioControl.pfnDialogBox)>::Call;
    ioControl.pfnEndDialog = WITNESSADAPTER<&TestEndDialog, decltype(ioControl.pfnEndDialog)>::Call;
    ioControl.pfnFillProdSrcLB = WITNESSADAPTER<&TestFillProdSrcLB, decltype(ioControl.pfnFillProdSrcLB)>::Call;
    ioControl.pfnFillPlanetProdLB = WITNESSADAPTER<&TestFillPlanetProdLB, decltype(ioControl.pfnFillPlanetProdLB)>::Call;
    ioControl.pfnDrawProductionDlg = WITNESSADAPTER<&TestDrawProductionDlg, decltype(ioControl.pfnDrawProductionDlg)>::Call;
    CONTROLIOSCOPE scopeControl(ioControl);
    RACEQUERYIO    ioRaceQuery;
    ioRaceQuery.pfnStat = WITNESSADAPTER<&TestGetRaceStat, decltype(ioRaceQuery.pfnStat)>::Call;
    RACEQUERYIOSCOPE scopeRaceQuery(ioRaceQuery);
    TEXTIO           ioText;
    ioText.pfnCompressed = TestPszGetCompressedString;
    ioText.pfnFormat = WITNESSADAPTER<&TestPszFormatIds, decltype(ioText.pfnFormat)>::Call;
    ioText.pfnCchGetString = TestCchGetString;
    TEXTIOSCOPE scopeText(ioText);
    ALERTIO     ioAlert;
    ioAlert.pfnAlertSz = WITNESSADAPTER<&TestAlertSz, decltype(ioAlert.pfnAlertSz)>::Call;
    ALERTIOSCOPE scopeAlert(ioAlert);
    app.state.pProdGlob = catalog;
    app.state.dyArial8 = 8;
    PROD     manual = {.cItem = 1, .iItem = 7, .grobj = 1, .pct = 35, .unused = 17};
    PROD     automatic = {.cItem = 10, .iItem = 0, .grobj = 1, .pct = 50, .unused = 3};
    ZIPPRODQ snapshot[5];
    CHECK(sizeof(PROD) == 4 && sizeof(PLPROD) == 4 && sizeof(ZIPPRODQ) == 40);
    reset(app, 2);
    app.state.lpplProdGlob->rgprod()[0] = automatic;
    app.state.lpplProdGlob->rgprod()[1] = manual;
    app.state.pProdGlob[0] = PROD{.cItem = 20, .iItem = 0, .grobj = 1, .pct = 0, .unused = 0};
    app.state.pProdGlob[1] = manual;
    app.state.cProdGlob = 2;
    app.state.vrgZipProd[0].cpq = 1;
    app.state.vrgZipProd[0].rgpq[0].w = 64;
    app.state.vrgZipProd[0].fNoResearch = 1;
    apply(app, IDC_IMPORT, 0);
    CHECK(app.state.lpplProdGlob->iprodMac == 2 && app.state.lpplProdGlob->iprodMax == 3 && lastBytes == 16);
    CHECK(memcmp(&app.state.lpplProdGlob->rgprod()[0], &manual, 4) == 0 && app.state.lpplProdGlob->rgprod()[1].cItem == 1);
    CHECK(app.state.lpplProdGlob->rgprod()[1].pct == 0 && app.state.lpplProdGlob->rgprod()[1].unused == 0 && app.state.lpplProdGlob->rgprod()[2].cItem == 0);
    CHECK(app.state.pProdGlob[0].cItem == 30 && app.state.pProdGlob[1].cItem == 2 && app.state.sel.pl.fNoResearch == 1 && freed == 1);
    // Invalid flag is not a command guard; traits filter each packed row.
    for (primary = 0; primary < 9; primary++) {
        int selected = primary;
        reset(app, 0);
        primary = selected;
        app.state.vrgZipProd[0].cpq = 7;
        for (i = 0; i < 7; i++)
            app.state.vrgZipProd[0].rgpq[i].w = 64 + i;
        apply(app, IDC_IMPORT, 0);
        CHECK(app.state.lpplProdGlob->iprodMac == (primary == 8 ? 4 : primary == 3 ? 5 : 7));
        for (i = 0; i < app.state.lpplProdGlob->iprodMac; i++)
            CHECK((primary != 8 || app.state.lpplProdGlob->rgprod()[i].iItem > 2) &&
                  (primary != 3 || (app.state.lpplProdGlob->rgprod()[i].iItem != 4 && app.state.lpplProdGlob->rgprod()[i].iItem != 5)));
    }
    reset(app, 2);
    app.state.lpplProdGlob->rgprod()[0] = manual;
    app.state.lpplProdGlob->rgprod()[1] = manual;
    app.state.lpplProdGlob->rgprod()[0].cItem = 3;
    app.state.pProdGlob[0] = manual;
    app.state.pProdGlob[0].cItem = 1020;
    app.state.cProdGlob = 1;
    app.state.sel.pl.fNoResearch = 1;
    apply(app, IDC_PRODUCTION_CLEAR, 0);
    CHECK(app.state.pProdGlob[0].cItem == 1023 && app.state.lpplProdGlob->iprodMac == 0 && app.state.sel.pl.fNoResearch && allocations == 0 && freed == 0);
    reset(app, 1);
    app.state.lpplProdGlob->rgprod()[0] = manual;
    app.state.lpplProdGlob->rgprod()[0].cItem = 4;
    app.state.pProdGlob[0] = manual;
    app.state.pProdGlob[0].cItem = 1020;
    app.state.cProdGlob = 1;
    apply(app, IDC_PRODUCTION_CLEAR, 0);
    CHECK(app.state.pProdGlob[0].cItem == 0);
    reset(app, 1);
    app.state.lpplProdGlob->rgprod()[0] = manual;
    apply(app, IDC_PRODUCTION_CLEAR, 0);
    CHECK(app.state.pProdGlob[0].cItem == 1); // inactive, allocated slot
    reset(app, 40);
    for (i = 0; i < 40; i++)
        app.state.lpplProdGlob->rgprod()[i] = manual;
    app.state.pProdGlob[0] = manual;
    app.state.pProdGlob[0].cItem = 1023;
    app.state.cProdGlob = 1;
    app.state.vrgZipProd[0].cpq = 12;
    apply(app, IDC_IMPORT, 0);
    CHECK(app.state.lpplProdGlob->iprodMac == 52 && app.state.lpplProdGlob->iprodMax == 52);
    reset(app, 255);
    for (i = 0; i < 255; i++)
        app.state.lpplProdGlob->rgprod()[i] = manual;
    app.state.pProdGlob[0] = manual;
    app.state.pProdGlob[0].cItem = 1023;
    app.state.cProdGlob = 1;
    app.state.vrgZipProd[0].cpq = 1;
    app.state.vrgZipProd[0].rgpq[0].w = 64;
    apply(app, IDC_IMPORT, 0);
    CHECK(lastBytes == 1028 && app.state.lpplProdGlob->iprodMac == 0 && app.state.lpplProdGlob->iprodMax == 0);
    CHECK(app.state.lpplProdGlob->rgprod()[254].unused == 17 && app.state.lpplProdGlob->rgprod()[255].cItem == 1);
    // Every accepted native count/preset-count pair, including 256..267 storage.
    for (n = 0; n <= 255; n++)
        for (k = 0; k <= 12; k++) {
            int capacity = n + k ? n + k : 1;
            reset(app, n);
            for (i = 0; i < n; i++)
                app.state.lpplProdGlob->rgprod()[i] = manual;
            app.state.pProdGlob[0] = manual;
            app.state.pProdGlob[0].cItem = 1023;
            app.state.cProdGlob = 1;
            app.state.vrgZipProd[0].cpq = k;
            for (i = 0; i < k; i++)
                app.state.vrgZipProd[0].rgpq[i].w = 67;
            apply(app, IDC_IMPORT, 0);
            CHECK(lastBytes == (size_t)(4 + 4 * capacity) && app.state.lpplProdGlob->iprodMax == (capacity & 255));
            CHECK(app.state.lpplProdGlob->iprodMac == ((n + k) & 255));
            if (n)
                CHECK(memcmp(&app.state.lpplProdGlob->rgprod()[n - 1], &manual, 4) == 0);
            if (k)
                CHECK(app.state.lpplProdGlob->rgprod()[n + k - 1].iItem == 3 && app.state.lpplProdGlob->rgprod()[n + k - 1].pct == 0);
        }
    reset(app, 1);
    app.state.lpplProdGlob->rgprod()[0] = manual;
    app.state.pProdGlob[0] = manual;
    app.state.cProdGlob = 1;
    app.state.sel.pl.fNoResearch = 1;
    app.lpui->hwndProdDlg = HwndWitness(1);
    app.lpui->fDlgUp = 1;
    failAllocation = 1;
    try {
        apply(app, IDC_IMPORT, 0);
        CHECK(0);
    } catch (const FIXTUREESCAPE &) {
    }
    CHECK(app.state.pProdGlob[0].cItem == 2 && app.state.lpplProdGlob != NULL && freed == 0);
    CHECK(recover(app) == 0 && app.state.lpplProdGlob == NULL && app.lpui->hwndProdDlg == 0 && !app.lpui->fDlgUp);
    CHECK(freed == 1 && ended == 1 && alerts == 1 && app.state.sel.pl.fNoResearch == 1);
    reset(app, 14);
    app.state.lpplProdGlob->rgprod()[0] = manual;
    for (i = 1; i < 14; i++)
        app.state.lpplProdGlob->rgprod()[i] = automatic;
    strcpy(app.state.vrgZipProd[0].szName, "Default");
    app.lpui->iResTechNow = static_cast<TechFieldType>(0);
    app.state.sel.pl.fNoResearch = 1;
    renameResult = 0;
    edit(IDC_IMPORT);
    CHECK(renameCalls == 0 && app.state.vrgZipProd[0].cpq == 12 && app.state.vrgZipProd[0].fValid && app.state.vrgZipProd[0].fNoResearch && app.state.gd.fChgZipProd);
    CHECK(strcmp(app.state.vrgZipProd[0].szName, "Default") == 0 && app.state.vrgZipProd[0].rgpq[11].w == 640);
    app.lpui->iResTechNow = static_cast<TechFieldType>(1);
    app.state.vrgZipProd[1].rgpq[11].w = 511;
    memcpy(snapshot, app.state.vrgZipProd, sizeof(snapshot));
    app.state.gd.fChgZipProd = 0;
    edit(IDC_IMPORT);
    CHECK(memcmp(snapshot, app.state.vrgZipProd, sizeof(snapshot)) == 0 && app.state.gd.fChgZipProd && renameCalls == 1);
    renameResult = -1;
    renameText[0] = 0;
    edit(IDC_RENAME);
    CHECK(strcmp(app.state.vrgZipProd[1].szName, "Custom1") == 0 && !app.state.vrgZipProd[1].fValid);
    strcpy(renameText, "A&B");
    app.state.lpplProdGlob->iprodMac = 2;
    edit(IDC_IMPORT);
    CHECK(strcmp(label, "A&&B") == 0 && app.state.vrgZipProd[1].cpq == 1 && app.state.vrgZipProd[1].rgpq[0].w == 640 && app.state.vrgZipProd[1].rgpq[11].w == 511);
    CHECK(enabledDelete && enabledRename);
    memcpy(snapshot, app.state.vrgZipProd, sizeof(snapshot));
    edit(IDC_DELETE);
    snapshot[1].fValid = 0;
    CHECK(memcmp(snapshot, app.state.vrgZipProd, sizeof(snapshot)) == 0);
    EnableZipProdBtns(app, HwndWitness(1), 0);
    CHECK(!enabledDelete && !enabledRename);
    app.lpui->iResTechNow = static_cast<TechFieldType>(0);
    edit(IDC_DELETE);
    CHECK(!app.state.vrgZipProd[0].fValid); // dispatch itself does not guard default
    memcpy(snapshot, app.state.vrgZipProd, sizeof(snapshot));
    outerResult = 0;
    app.state.gd.fChgZipProd = 0;
    customize(app);
    CHECK(memcmp(snapshot, app.state.vrgZipProd, 160) == 0 && app.state.vrgZipProd[4].cpq == 9 && app.state.gd.fChgZipProd);
    outerResult = 65536;
    customize(app);
    CHECK(memcmp(snapshot, app.state.vrgZipProd, 160) == 0);
    outerResult = -1;
    customize(app);
    CHECK(app.state.vrgZipProd[0].fValid == 0x5a && app.state.vrgZipProd[4].cpq == 9);
    reset(app, 0);
    FillZipProdLB(app, HwndWitness(1), &app.state.vrgZipProd[0]);
    CHECK(labelCount == 1 && strcmp(labels[0], "Automatic orders") == 0);
    app.state.vrgZipProd[0].fValid = 1;
    FillZipProdLB(app, HwndWitness(1), &app.state.vrgZipProd[0]);
    CHECK(labelCount == 1 && strcmp(labels[0], "Automatic orders") == 0);
    app.state.vrgZipProd[0].cpq = 3;
    app.state.vrgZipProd[0].rgpq[0].w = 3;
    app.state.vrgZipProd[0].rgpq[1].w = 64;
    app.state.vrgZipProd[0].rgpq[2].w = 1;
    FillZipProdLB(app, HwndWitness(1), &app.state.vrgZipProd[0]);
    CHECK(labelCount == 3 && strcmp(labels[0], "Item3") == 0 && strcmp(labels[1], "Item0") == 0 && strcmp(labels[2], "Item1 0") == 0);
    memset(app.state.vrgZipProd, 0, sizeof(app.state.vrgZipProd));
    app.state.vrgZipProd[1].fValid = app.state.vrgZipProd[3].fValid = 1;
    CHECK(popupSlot(app, 0) == 1 && popupSlot(app, 1) == 3 && popupSlot(app, 2) == 4 && popupSlot(app, 50) == 4);
    FreePl(app, app.state.lpplProdGlob);
    printf("M09 linked preset witnesses: %d assertions passed\n", checks);
    return 0;
}
