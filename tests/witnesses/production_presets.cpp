#include <map>
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
static void                            *TestLpAlloc(uint16_t cb, HeapType ht) {
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
static void TestFreeLp(void *p, HeapType ht) {
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
    if (message == LB_RESETCONTENT)
        labelCount = 0;
    if (message == LB_ADDSTRING) {
        if (labelCount >= 12)
            abort();
        strcpy(labels[labelCount++], (const char *)param);
    }
}
static void TestFillPlanetProdLB(HWND hwnd, PLPROD *q, void *p) {
    (void)hwnd;
    (void)q;
    (void)p;
}
static void TestFillProdSrcLB(HWND hwnd, int n) {
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
static const char *TestPszGetCompressedString(int id) {
    if (id == idsCustomD)
        return "Custom%d";
    if (id == idsUnusedD)
        return "Unused%d";
    return "Automatic orders";
}
static void TestCchGetString(int id, char *out) {
    if (id == idsSD2)
        strcpy(out, "%s %d");
    else
        snprintf(out, 15, "Item%d", id - idsMines);
}
static int TestLUiDialogBox(int instance, int resource, HWND hwnd, int proc) {
    (void)instance;
    (void)hwnd;
    (void)proc;
    if (resource == IDD_RENAME) {
        renameCalls++;
        if (renameResult)
            strcpy(szWork, renameText);
        return renameResult;
    }
    memset(vrgZipProd, 0x5a, 4 * sizeof(ZIPPRODQ));
    vrgZipProd[4].cpq = 9;
    gd.fChgZipProd = 1;
    return outerResult;
}
static void TestEndDialog(HWND hwnd, int result) {
    (void)hwnd;
    (void)result;
    ended++;
}
static const char *TestPszFormatIds(int id, void *p) {
    (void)id;
    (void)p;
    return "memory";
}
static void TestAlertSz(const char *s, int flags) {
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
static uint16_t TestExtent(const void *p) { return extents.at(p); }
static void     TestDrawProductionDlg(HWND, HDC, RECT *, int16_t) {}
static void     apply(int cmd, int param) { ProdCommandHandler(HwndWitness(1), cmd, param); }
static int      popupSlot(int i) { return IProductionPresetSlot((int16_t)i); }
static void     edit(int cmd) { ZipProdDlg(HwndWitness(1), WM_COMMAND, cmd, 0); }
static void     customize() { CustomizeProductionPresets(HwndWitness(1)); }
static void     FailRecovery(PROD *) { StarsThrowError(starsErrorMemory, 0); }
static int      recover() {
    PRODUCTIONIO io = ProductionIO();
    io.pfnInitialize = FailRecovery;
    PRODUCTIONIOSCOPE recoveryScope(io);
    return ChangeProduction(FALSE);
}

static void reset(unsigned n) {
    if (lpplProdGlob)
        FreePl(lpplProdGlob);
    failAllocation = 0;
    lpplProdGlob = LpplAlloc<PLPROD>(4, n ? n : 1, htOrd);
    lpplProdGlob->iprodMac = n;
    memset(pProdGlob, 0, sizeof(catalog));
    memset(vrgZipProd, 0, sizeof(vrgZipProd));
    cProdGlob = 0;
    primary = 0;
    sel.pl.fNoResearch = 0;
    gd.fChgZipProd = 0;
    freed = ended = alerts = allocations = renameCalls = 0;
    renameResult = 1;
    renameText[0] = 0;
    label[0] = 0;
}

int main(void) {
    int    i, n, k;
    HEAPIO ioHeap;
    ioHeap.pfnAllocate = WITNESSADAPTER<&TestLpAlloc, decltype(ioHeap.pfnAllocate)>::Call;
    ioHeap.pfnFree = WITNESSADAPTER<&TestFreeLp, decltype(ioHeap.pfnFree)>::Call;
    ioHeap.pfnExtent = WITNESSADAPTER<&TestExtent, decltype(ioHeap.pfnExtent)>::Call;
    HEAPIOSCOPE scopeHeap(ioHeap);
    CONTROLIO   ioControl;
    ioControl.pfnGetDlgItem = WITNESSADAPTER<&TestGetDlgItem, decltype(ioControl.pfnGetDlgItem)>::Call;
    ioControl.pfnLUiSendMessage = WITNESSADAPTER<&TestLUiSendMessage, decltype(ioControl.pfnLUiSendMessage)>::Call;
    ioControl.pfnGetClientRect = WITNESSADAPTER<&TestGetClientRect, decltype(ioControl.pfnGetClientRect)>::Call;
    ioControl.pfnInvalidateRect = WITNESSADAPTER<&TestInvalidateRect, decltype(ioControl.pfnInvalidateRect)>::Call;
    ioControl.pfnFUiSetWindowText = WITNESSADAPTER<&TestFUiSetWindowText, decltype(ioControl.pfnFUiSetWindowText)>::Call;
    ioControl.pfnHwndUiSetFocus = WITNESSADAPTER<&TestHwndUiSetFocus, decltype(ioControl.pfnHwndUiSetFocus)>::Call;
    ioControl.pfnFUiEnableWindow = WITNESSADAPTER<&TestFUiEnableWindow, decltype(ioControl.pfnFUiEnableWindow)>::Call;
    ioControl.pfnLUiDialogBox = WITNESSADAPTER<&TestLUiDialogBox, decltype(ioControl.pfnLUiDialogBox)>::Call;
    ioControl.pfnEndDialog = WITNESSADAPTER<&TestEndDialog, decltype(ioControl.pfnEndDialog)>::Call;
    ioControl.pfnFillProdSrcLB = WITNESSADAPTER<&TestFillProdSrcLB, decltype(ioControl.pfnFillProdSrcLB)>::Call;
    ioControl.pfnFillPlanetProdLB = WITNESSADAPTER<&TestFillPlanetProdLB, decltype(ioControl.pfnFillPlanetProdLB)>::Call;
    ioControl.pfnDrawProductionDlg = WITNESSADAPTER<&TestDrawProductionDlg, decltype(ioControl.pfnDrawProductionDlg)>::Call;
    CONTROLIOSCOPE scopeControl(ioControl);
    RACEQUERYIO    ioRaceQuery;
    ioRaceQuery.pfnStat = WITNESSADAPTER<&TestGetRaceStat, decltype(ioRaceQuery.pfnStat)>::Call;
    RACEQUERYIOSCOPE scopeRaceQuery(ioRaceQuery);
    TEXTIO           ioText;
    ioText.pfnCompressed = WITNESSADAPTER<&TestPszGetCompressedString, decltype(ioText.pfnCompressed)>::Call;
    ioText.pfnFormat = WITNESSADAPTER<&TestPszFormatIds, decltype(ioText.pfnFormat)>::Call;
    ioText.pfnCchGetString = WITNESSADAPTER<&TestCchGetString, decltype(ioText.pfnCchGetString)>::Call;
    TEXTIOSCOPE scopeText(ioText);
    ALERTIO     ioAlert;
    ioAlert.pfnAlertSz = WITNESSADAPTER<&TestAlertSz, decltype(ioAlert.pfnAlertSz)>::Call;
    ALERTIOSCOPE scopeAlert(ioAlert);
    pProdGlob = catalog;
    dyArial8 = 8;
    PROD     manual = {.cItem = 1, .iItem = 7, .grobj = 1, .pct = 35, .unused = 17};
    PROD     automatic = {.cItem = 10, .iItem = 0, .grobj = 1, .pct = 50, .unused = 3};
    ZIPPRODQ snapshot[5];
    CHECK(sizeof(PROD) == 4 && sizeof(PLPROD) == 4 && sizeof(ZIPPRODQ) == 40);
    reset(2);
    lpplProdGlob->rgprod()[0] = automatic;
    lpplProdGlob->rgprod()[1] = manual;
    pProdGlob[0] = PROD{.cItem = 20, .iItem = 0, .grobj = 1, .pct = 0, .unused = 0};
    pProdGlob[1] = manual;
    cProdGlob = 2;
    vrgZipProd[0].cpq = 1;
    vrgZipProd[0].rgpq[0].w = 64;
    vrgZipProd[0].fNoResearch = 1;
    apply(IDC_IMPORT, 0);
    CHECK(lpplProdGlob->iprodMac == 2 && lpplProdGlob->iprodMax == 3 && lastBytes == 16);
    CHECK(memcmp(&lpplProdGlob->rgprod()[0], &manual, 4) == 0 && lpplProdGlob->rgprod()[1].cItem == 1);
    CHECK(lpplProdGlob->rgprod()[1].pct == 0 && lpplProdGlob->rgprod()[1].unused == 0 && lpplProdGlob->rgprod()[2].cItem == 0);
    CHECK(pProdGlob[0].cItem == 30 && pProdGlob[1].cItem == 2 && sel.pl.fNoResearch == 1 && freed == 1);
    // Invalid flag is not a command guard; traits filter each packed row.
    for (primary = 0; primary < 9; primary++) {
        int selected = primary;
        reset(0);
        primary = selected;
        vrgZipProd[0].cpq = 7;
        for (i = 0; i < 7; i++)
            vrgZipProd[0].rgpq[i].w = 64 + i;
        apply(IDC_IMPORT, 0);
        CHECK(lpplProdGlob->iprodMac == (primary == 8 ? 4 : primary == 3 ? 5 : 7));
        for (i = 0; i < lpplProdGlob->iprodMac; i++)
            CHECK((primary != 8 || lpplProdGlob->rgprod()[i].iItem > 2) &&
                  (primary != 3 || (lpplProdGlob->rgprod()[i].iItem != 4 && lpplProdGlob->rgprod()[i].iItem != 5)));
    }
    reset(2);
    lpplProdGlob->rgprod()[0] = manual;
    lpplProdGlob->rgprod()[1] = manual;
    lpplProdGlob->rgprod()[0].cItem = 3;
    pProdGlob[0] = manual;
    pProdGlob[0].cItem = 1020;
    cProdGlob = 1;
    sel.pl.fNoResearch = 1;
    apply(IDC_PRODUCTION_CLEAR, 0);
    CHECK(pProdGlob[0].cItem == 1023 && lpplProdGlob->iprodMac == 0 && sel.pl.fNoResearch && allocations == 0 && freed == 0);
    reset(1);
    lpplProdGlob->rgprod()[0] = manual;
    lpplProdGlob->rgprod()[0].cItem = 4;
    pProdGlob[0] = manual;
    pProdGlob[0].cItem = 1020;
    cProdGlob = 1;
    apply(IDC_PRODUCTION_CLEAR, 0);
    CHECK(pProdGlob[0].cItem == 0);
    reset(1);
    lpplProdGlob->rgprod()[0] = manual;
    apply(IDC_PRODUCTION_CLEAR, 0);
    CHECK(pProdGlob[0].cItem == 1); // inactive, allocated slot
    reset(40);
    for (i = 0; i < 40; i++)
        lpplProdGlob->rgprod()[i] = manual;
    pProdGlob[0] = manual;
    pProdGlob[0].cItem = 1023;
    cProdGlob = 1;
    vrgZipProd[0].cpq = 12;
    apply(IDC_IMPORT, 0);
    CHECK(lpplProdGlob->iprodMac == 52 && lpplProdGlob->iprodMax == 52);
    reset(255);
    for (i = 0; i < 255; i++)
        lpplProdGlob->rgprod()[i] = manual;
    pProdGlob[0] = manual;
    pProdGlob[0].cItem = 1023;
    cProdGlob = 1;
    vrgZipProd[0].cpq = 1;
    vrgZipProd[0].rgpq[0].w = 64;
    apply(IDC_IMPORT, 0);
    CHECK(lastBytes == 1028 && lpplProdGlob->iprodMac == 0 && lpplProdGlob->iprodMax == 0);
    CHECK(lpplProdGlob->rgprod()[254].unused == 17 && lpplProdGlob->rgprod()[255].cItem == 1);
    // Every accepted native count/preset-count pair, including 256..267 storage.
    for (n = 0; n <= 255; n++)
        for (k = 0; k <= 12; k++) {
            int capacity = n + k ? n + k : 1;
            reset(n);
            for (i = 0; i < n; i++)
                lpplProdGlob->rgprod()[i] = manual;
            pProdGlob[0] = manual;
            pProdGlob[0].cItem = 1023;
            cProdGlob = 1;
            vrgZipProd[0].cpq = k;
            for (i = 0; i < k; i++)
                vrgZipProd[0].rgpq[i].w = 67;
            apply(IDC_IMPORT, 0);
            CHECK(lastBytes == (size_t)(4 + 4 * capacity) && lpplProdGlob->iprodMax == (capacity & 255));
            CHECK(lpplProdGlob->iprodMac == ((n + k) & 255));
            if (n)
                CHECK(memcmp(&lpplProdGlob->rgprod()[n - 1], &manual, 4) == 0);
            if (k)
                CHECK(lpplProdGlob->rgprod()[n + k - 1].iItem == 3 && lpplProdGlob->rgprod()[n + k - 1].pct == 0);
        }
    reset(1);
    lpplProdGlob->rgprod()[0] = manual;
    pProdGlob[0] = manual;
    cProdGlob = 1;
    sel.pl.fNoResearch = 1;
    hwndProdDlg = HwndWitness(1);
    fDlgUp = 1;
    failAllocation = 1;
    try {
        apply(IDC_IMPORT, 0);
        CHECK(0);
    } catch (const FIXTUREESCAPE &) {
    }
    CHECK(pProdGlob[0].cItem == 2 && lpplProdGlob != NULL && freed == 0);
    CHECK(recover() == 0 && lpplProdGlob == NULL && hwndProdDlg == 0 && !fDlgUp);
    CHECK(freed == 1 && ended == 1 && alerts == 1 && sel.pl.fNoResearch == 1);
    reset(14);
    lpplProdGlob->rgprod()[0] = manual;
    for (i = 1; i < 14; i++)
        lpplProdGlob->rgprod()[i] = automatic;
    strcpy(vrgZipProd[0].szName, "Default");
    iResTechNow = static_cast<TechFieldType>(0);
    sel.pl.fNoResearch = 1;
    renameResult = 0;
    edit(IDC_IMPORT);
    CHECK(renameCalls == 0 && vrgZipProd[0].cpq == 12 && vrgZipProd[0].fValid && vrgZipProd[0].fNoResearch && gd.fChgZipProd);
    CHECK(strcmp(vrgZipProd[0].szName, "Default") == 0 && vrgZipProd[0].rgpq[11].w == 640);
    iResTechNow = static_cast<TechFieldType>(1);
    vrgZipProd[1].rgpq[11].w = 511;
    memcpy(snapshot, vrgZipProd, sizeof(snapshot));
    gd.fChgZipProd = 0;
    edit(IDC_IMPORT);
    CHECK(memcmp(snapshot, vrgZipProd, sizeof(snapshot)) == 0 && gd.fChgZipProd && renameCalls == 1);
    renameResult = -1;
    renameText[0] = 0;
    edit(IDC_RENAME);
    CHECK(strcmp(vrgZipProd[1].szName, "Custom1") == 0 && !vrgZipProd[1].fValid);
    strcpy(renameText, "A&B");
    lpplProdGlob->iprodMac = 2;
    edit(IDC_IMPORT);
    CHECK(strcmp(label, "A&&B") == 0 && vrgZipProd[1].cpq == 1 && vrgZipProd[1].rgpq[0].w == 640 && vrgZipProd[1].rgpq[11].w == 511);
    CHECK(enabledDelete && enabledRename);
    memcpy(snapshot, vrgZipProd, sizeof(snapshot));
    edit(IDC_DELETE);
    snapshot[1].fValid = 0;
    CHECK(memcmp(snapshot, vrgZipProd, sizeof(snapshot)) == 0);
    EnableZipProdBtns(HwndWitness(1), 0);
    CHECK(!enabledDelete && !enabledRename);
    iResTechNow = static_cast<TechFieldType>(0);
    edit(IDC_DELETE);
    CHECK(!vrgZipProd[0].fValid); // dispatch itself does not guard default
    memcpy(snapshot, vrgZipProd, sizeof(snapshot));
    outerResult = 0;
    gd.fChgZipProd = 0;
    customize();
    CHECK(memcmp(snapshot, vrgZipProd, 160) == 0 && vrgZipProd[4].cpq == 9 && gd.fChgZipProd);
    outerResult = 65536;
    customize();
    CHECK(memcmp(snapshot, vrgZipProd, 160) == 0);
    outerResult = -1;
    customize();
    CHECK(vrgZipProd[0].fValid == 0x5a && vrgZipProd[4].cpq == 9);
    reset(0);
    FillZipProdLB(HwndWitness(1), &vrgZipProd[0]);
    CHECK(labelCount == 1 && strcmp(labels[0], "Automatic orders") == 0);
    vrgZipProd[0].fValid = 1;
    FillZipProdLB(HwndWitness(1), &vrgZipProd[0]);
    CHECK(labelCount == 1 && strcmp(labels[0], "Automatic orders") == 0);
    vrgZipProd[0].cpq = 3;
    vrgZipProd[0].rgpq[0].w = 3;
    vrgZipProd[0].rgpq[1].w = 64;
    vrgZipProd[0].rgpq[2].w = 1;
    FillZipProdLB(HwndWitness(1), &vrgZipProd[0]);
    CHECK(labelCount == 3 && strcmp(labels[0], "Item3") == 0 && strcmp(labels[1], "Item0") == 0 && strcmp(labels[2], "Item1 0") == 0);
    memset(vrgZipProd, 0, sizeof(vrgZipProd));
    vrgZipProd[1].fValid = vrgZipProd[3].fValid = 1;
    CHECK(popupSlot(0) == 1 && popupSlot(1) == 3 && popupSlot(2) == 4 && popupSlot(50) == 4);
    FreePl(lpplProdGlob);
    printf("M09 linked preset witnesses: %d assertions passed\n", checks);
    return 0;
}
