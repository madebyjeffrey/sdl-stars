#include "fleet_query_io.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include "transportpresets.h"
#include "ui_fixture.h"
static int popupResult, popupCount, renameResult, renameCalls, outerResult;
alignas(PLORD) static unsigned char orderBytes[sizeof(PLORD) + 2 * sizeof(ORDER)];
static PLORD *orders;
static char   rgszMineralsStorage[5][80] = {"Ironium", "Boranium", "Germanium", "Colonists", "Fuel"};
static char  *testMinerals[5] = {rgszMineralsStorage[0], rgszMineralsStorage[1], rgszMineralsStorage[2], rgszMineralsStorage[3], rgszMineralsStorage[4]};
static char   rgszZipOrderStorage[4][80] = {"Load", "Unload", "Wait", "Clear"};
static char  *testZipOrder[4] = {rgszZipOrderStorage[0], rgszZipOrderStorage[1], rgszZipOrderStorage[2], rgszZipOrderStorage[3]};
static int    writes, draws, updates, alerts, hasCargo, notifications, depth, checks, enabledDelete, enabledRename;
static int    comboCount[3], comboSelection[3];
static char   comboLabels[3][20][80], editText[80], renameText[13], radioLabel[80];
static ORDER  snapshots[16];
static char   actionLabelsStorage[11][80] = {"(no action)",       "Load All Available", "Unload All",    "Load Exactly...",
                                             "Unload Exactly...", "Fill Up to %...",    "Wait for %...", "Load Dunnage",
                                             "Set Amount to...",  "Set Waypoint to...", "Load Optimal"};
static char  *actionLabels[11] = {actionLabelsStorage[0], actionLabelsStorage[1], actionLabelsStorage[2], actionLabelsStorage[3],
                                  actionLabelsStorage[4], actionLabelsStorage[5], actionLabelsStorage[6], actionLabelsStorage[7],
                                  actionLabelsStorage[8], actionLabelsStorage[9], actionLabelsStorage[10]};
static char   szLiteral0[] = "Player";
static char   szLiteral1[] = "Custom%d";
static char   szLiteral2[] = "Unused%d";
static char   szLiteral3[] = "%d LY";
static char   szLiteral4[] = "%d Year%c";
static char   szLiteral5[] = "Other";
static void   amount(APPSESSION &app) { ChangeTransportAmount(app); }
static void   TestUpdateOrdersDDs(APPSESSION &app, int16_t level) {
    updates++;
    ::UpdateOrdersDDs(app, level);
}
static int TestGetDlgItem(HWND hwnd, int id) {
    (void)hwnd;
    return id;
}
static intptr_t TestLUiSendMessage(HWND hwnd, int message, int value, LPARAM param) {
    int c = WitnessValue<int>(hwnd) - 10;
    if (c < 0 || c >= 3)
        abort();
    if (message == win32::CB_GETCURSEL)
        return comboSelection[c];
    if (message == win32::CB_RESETCONTENT) {
        comboCount[c] = 0;
        comboSelection[c] = -1;
        return 0;
    }
    if (message == win32::CB_ADDSTRING) {
        if (comboCount[c] >= 20)
            abort();
        strcpy(comboLabels[c][comboCount[c]], (const char *)param);
        return comboCount[c]++;
    }
    if (message == win32::CB_SETCURSEL) {
        comboSelection[c] = (value >= 0 && value < comboCount[c]) ? value : -1;
        return comboSelection[c];
    }
    abort();
}
static void TestFUiSetWindowText(HWND hwnd, const char *s) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    if (hwnd == app.lpui->hwndOrderED) {
        strcpy(editText, s);
        if (notifications) {
            if (++depth > 3)
                abort();
            amount(app);
            depth--;
        }
    } else
        strcpy(radioLabel, s);
}
static void TestGetWindowText(HWND hwnd, char *out, int capacity) {
    size_t n = strlen(editText);
    (void)hwnd;
    if (n >= (size_t)capacity)
        n = capacity - 1;
    memcpy(out, editText, n);
    out[n] = 0;
}
static int TestFLookupFleet(APPSESSION &app, int id, void *p) {
    (void)id;
    (void)p;
    if (writes < 16)
        snapshots[writes] = orders->rgord()[app.state.sel.iwpAct];
    writes++;
    return 0;
}
static void TestDrawPlanShip(APPSESSION &, void *p, int tile) {
    (void)p;
    (void)tile;
    draws++;
}
static int TestLGetFleetStat(APPSESSION &, void *p, int stat) {
    (void)p;
    (void)stat;
    return hasCargo;
}
static std::array<char, 50> TestPszPlayerName(APPSESSION &, int16_t i, int16_t a, int16_t b, int16_t c, int16_t d, PLAYER *p) {
    (void)i;
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    (void)p;
    std::array<char, 50> szName{};
    CchCopyUtf8Prefix(std::span{szName}, szLiteral0);
    return szName;
}
static std::array<char, cchCatalogText> TestPszGetCompressedString(APPSESSION &, StringId id) {
    if (id >= idsAction && id <= idsLoadOptimal)
        return SzWitnessText(actionLabels[id - idsAction]);
    if (id == idsCustomD)
        return SzWitnessText(szLiteral1);
    if (id == idsUnusedD)
        return SzWitnessText(szLiteral2);
    if (id == idsWithinDLY)
        return SzWitnessText(szLiteral3);
    if (id == idsDYearC)
        return SzWitnessText(szLiteral4);
    return SzWitnessText(szLiteral5);
}
static std::array<char, 256> TestPszFormatIds(APPSESSION &, int id, void *p) {
    (void)id;
    (void)p;
    return SzWitnessText("amount");
}
static std::int16_t TestCchGetString(APPSESSION &app, StringId id, std::span<char> out) {
    return static_cast<std::int16_t>(CchCopyUtf8(out, TestPszGetCompressedString(app, id).data()));
}
static void TestAlertSz(APPSESSION &, const char *s, int flags) {
    (void)s;
    (void)flags;
    alerts++;
}
static void TestFUiEnableWindow(HWND hwnd, int value) {
    if (WitnessValue<int>(hwnd) == IDC_DELETE)
        enabledDelete = value;
    if (WitnessValue<int>(hwnd) == IDC_RENAME)
        enabledRename = value;
}
static void TestInvalidateRect(HWND hwnd, void *rc, int erase) {
    (void)hwnd;
    (void)rc;
    (void)erase;
}
static void TestHwndUiSetFocus(HWND hwnd) { (void)hwnd; }
static int  TestLUiDialogBox(int instance, int resource, HWND hwnd, int proc) {
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
    memset(app.state.vrgZip, 0x5a, sizeof(app.state.vrgZip));
    app.state.gd.fChgZipOrd = 1;
    return outerResult;
}
static int TestPopupMenu(APPSESSION &, HWND hwnd, int x, int y, int count, void *p, const char *const *labels, int selected, int flag) {
    (void)hwnd;
    (void)x;
    (void)y;
    (void)p;
    (void)labels;
    (void)selected;
    (void)flag;
    popupCount = count;
    return popupResult;
}
#define CHECK(x)                                                                                                                                               \
    do {                                                                                                                                                       \
        checks++;                                                                                                                                              \
        if (!(x)) {                                                                                                                                            \
            fprintf(stderr, "witness %d failed: %s\n", checks, #x);                                                                                            \
            return 1;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)
static void edit(int cmd) { ZipOrderDlg(HwndWitness(1), win32::WM_COMMAND, cmd, 0); }
static void popup(APPSESSION &app) { ApplyTransportPresetPopup(app, POINT16{0, 0}); }
static void action(APPSESSION &app) { ChangeTransportAction(app); }
static int  quantityControl(int l, int i, int *unit) {
    int16_t value = (int16_t)i;
    int     result = FTransportQuantityActive(l, &value);
    *unit = value;
    return result;
}
static void paintLabel(APPSESSION &app, int i) { FormatTransportPresetAction(app, (int16_t)i); }

static void reset(APPSESSION &app) {
    memset(app.state.vrgZip, 0, sizeof(app.state.vrgZip));
    memset(orderBytes, 0, sizeof(orderBytes));
    orders = LpStartPl<PLORD, ORDER>(orderBytes, sizeof(orderBytes));
    memset(&app.state.sel, 0, sizeof(app.state.sel));
    app.state.sel.fl.lpplord = orders;
    app.state.sel.iwpAct = 1;
    orders->rgord()[1].grTask = 1;
    comboCount[0] = comboCount[2] = 10;
    comboCount[1] = 5;
    comboSelection[0] = 1;
    comboSelection[1] = comboSelection[2] = 0;
    writes = draws = updates = alerts = renameCalls = app.state.gd.fChgZipOrd = depth = 0;
    notifications = 0;
    hasCargo = 1;
    renameResult = 1;
    renameText[0] = 0;
    editText[0] = 0;
}

int main(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    int           mode, i, mask, k, rank, n, pass, expected;
    ZIPORDER      before[4];
    TASKXPORT     prior;
    unsigned char header[8];
    CONTROLIO     ioControl;
    ioControl.pfnGetDlgItem = WITNESSADAPTER<&TestGetDlgItem, decltype(ioControl.pfnGetDlgItem)>::Call;
    ioControl.pfnSendMessage = WITNESSADAPTER<&TestLUiSendMessage, decltype(ioControl.pfnSendMessage)>::Call;
    ioControl.pfnInvalidateRect = WITNESSADAPTER<&TestInvalidateRect, decltype(ioControl.pfnInvalidateRect)>::Call;
    ioControl.pfnSetWindowText = WITNESSADAPTER<&TestFUiSetWindowText, decltype(ioControl.pfnSetWindowText)>::Call;
    ioControl.pfnSetFocus = WITNESSADAPTER<&TestHwndUiSetFocus, decltype(ioControl.pfnSetFocus)>::Call;
    ioControl.pfnEnableWindow = WITNESSADAPTER<&TestFUiEnableWindow, decltype(ioControl.pfnEnableWindow)>::Call;
    ioControl.pfnDialogBox = WITNESSADAPTER<&TestLUiDialogBox, decltype(ioControl.pfnDialogBox)>::Call;
    ioControl.pfnDrawPlanShip = WITNESSADAPTER<&TestDrawPlanShip, decltype(ioControl.pfnDrawPlanShip)>::Call;
    ioControl.pfnGetWindowText = WITNESSADAPTER<&TestGetWindowText, decltype(ioControl.pfnGetWindowText)>::Call;
    ioControl.pfnPopupMenu = WITNESSADAPTER<&TestPopupMenu, decltype(ioControl.pfnPopupMenu)>::Call;
    ioControl.pfnUpdateOrdersDDs = WITNESSADAPTER<&TestUpdateOrdersDDs, decltype(ioControl.pfnUpdateOrdersDDs)>::Call;
    CONTROLIOSCOPE scopeControl(ioControl);
    PRODUCTIONIO   ioProduction;
    ioProduction.pfnFleetStat = WITNESSADAPTER<&TestLGetFleetStat, decltype(ioProduction.pfnFleetStat)>::Call;
    PRODUCTIONIOSCOPE scopeProduction(ioProduction);
    TEXTIO            ioText;
    ioText.pfnCompressed = TestPszGetCompressedString;
    ioText.pfnFormat = WITNESSADAPTER<&TestPszFormatIds, decltype(ioText.pfnFormat)>::Call;
    ioText.pfnCchGetString = TestCchGetString;
    ioText.pfnPlayerName = TestPszPlayerName;
    TEXTIOSCOPE scopeText(ioText);
    ALERTIO     ioAlert;
    ioAlert.pfnAlertSz = WITNESSADAPTER<&TestAlertSz, decltype(ioAlert.pfnAlertSz)>::Call;
    ALERTIOSCOPE scopeAlert(ioAlert);
    FLEETQUERYIO ioFleetQuery;
    ioFleetQuery.pfnLookup = WITNESSADAPTER<&TestFLookupFleet, decltype(ioFleetQuery.pfnLookup)>::Call;
    FLEETQUERYIOSCOPE scopeFleetQuery(ioFleetQuery);
    for (int id = 0; id < 3; id++)
        app.lpui->rghwndOrderDD[id] = HwndWitness(10 + id);
    app.lpui->hwndOrderED = HwndWitness(13);
    for (int at = 0; at < 5; at++)
        app.state.rgszMinerals[at] = testMinerals[at];
    for (int at = 0; at < 4; at++)
        app.state.rgszZipOrder[at] = testZipOrder[at];
    CHECK(sizeof(ITEMACTION) == 2 && sizeof(TASKXPORT) == 10 && sizeof(ORDER) == 18 && sizeof(ZIPORDER) == 24);
    for (mode = 0; mode < 4; mode++) {
        reset(app);
        orders->rgord()[1].grTask = 2;
        comboSelection[0] = 2; // direct handler does not change task
        for (i = 0; i < 5; i++) {
            orders->rgord()[1].txp.rgia[i].SetIAction(15);
            orders->rgord()[1].txp.rgia[i].SetCQuan(1234 + i);
        }
        orders->rgord()[1].pt = POINT16{123, -45};
        orders->rgord()[1].id = 231;
        orders->rgord()[1].iWarp = 9;
        orders->rgord()[1].grobj = 2;
        orders->rgord()[1].fValidTask = 1;
        orders->rgord()[1].fNoAutoTrack = 1;
        orders->rgord()[1].fUnused = 3;
        memcpy(header, &orders->rgord()[1], 8);
        popupResult = mode;
        popup(app);
        CHECK(memcmp(header, &orders->rgord()[1], 8) == 0);
        CHECK(writes == 1 && updates == 1 && draws == 1 && orders->rgord()[1].grTask == 2);
        for (i = 0; i < 5; i++) {
            expected = mode == 3 ? 0 : mode == 2 && i < 3 ? 100 : 1234 + i;
            CHECK(orders->rgord()[1].txp.rgia[i].cQuan() == expected);
            expected = mode == 3 ? 0 : i == 4 ? 7 : mode == 1 ? 2 : i == 3 ? 0 : mode == 0 ? 1 : 6;
            CHECK(orders->rgord()[1].txp.rgia[i].iAction() == expected);
        }
    }
    for (mask = 0; mask < 16; mask++) {
        reset(app);
        k = 0;
        for (i = 0; i < 4; i++) {
            app.state.vrgZip[i].fValid = (mask >> i) & 1;
            k += app.state.vrgZip[i].fValid;
            app.state.vrgZip[i].txp.rgia[0].SetCQuan(100 + i);
        }
        memcpy(before, app.state.vrgZip, sizeof(before));
        rank = 0;
        for (i = 0; i < 4; i++)
            if (app.state.vrgZip[i].fValid) {
                popupResult = 5 + rank++;
                writes = 0;
                popup(app);
                CHECK(orders->rgord()[1].txp.rgia[0].cQuan() == 100 + i && writes == 1);
            }
        popupResult = (k ? 6 + k : 5);
        outerResult = 0;
        app.state.gd.fChgZipOrd = 0;
        writes = 0;
        popup(app);
        CHECK(popupCount == (k ? 7 + k : 6) && popupCount <= 11 && memcmp(before, app.state.vrgZip, 96) == 0 && app.state.gd.fChgZipOrd && writes == 0);
    }
    reset(app);
    prior = orders->rgord()[1].txp;
    popupResult = -1;
    popup(app);
    CHECK(writes == 0);
    popupResult = 4;
    popup(app);
    CHECK(writes == 1 && memcmp(&prior, &orders->rgord()[1].txp, 10) == 0);
    popupResult = -2;
    popup(app);
    CHECK(writes == 2 && memcmp(&prior, &orders->rgord()[1].txp, 10) == 0);
    popupResult = 5;
    outerResult = 65536;
    memcpy(before, app.state.vrgZip, 96);
    popup(app);
    CHECK(memcmp(before, app.state.vrgZip, 96) == 0);
    popupResult = 5;
    outerResult = -1;
    popup(app);
    CHECK(app.state.vrgZip[0].fValid == 0x5a);
    reset(app);
    app.lpui->iResTechNow = static_cast<TechFieldType>(0);
    renameResult = 0;
    memcpy(before, app.state.vrgZip, sizeof(before));
    edit(IDC_IMPORT);
    CHECK(renameCalls == 1 && app.state.gd.fChgZipOrd && memcmp(before, app.state.vrgZip, 96) == 0);
    renameResult = -1;
    edit(IDC_RENAME);
    CHECK(strcmp(app.state.vrgZip[0].szName, "Custom0") == 0 && !app.state.vrgZip[0].fValid);
    strcpy(renameText, "A&B");
    for (i = 0; i < 5; i++) {
        orders->rgord()[1].txp.rgia[i].SetIAction(15);
        orders->rgord()[1].txp.rgia[i].SetCQuan(4095);
    }
    edit(IDC_IMPORT);
    CHECK(app.state.vrgZip[0].fValid && strcmp(radioLabel, "A&&B") == 0 && memcmp(&app.state.vrgZip[0].txp, &orders->rgord()[1].txp, 10) == 0 && enabledDelete && enabledRename);
    memcpy(before, app.state.vrgZip, 96);
    edit(IDC_DELETE);
    before[0].fValid = 0;
    CHECK(memcmp(before, app.state.vrgZip, 96) == 0 && enabledDelete && enabledRename); // no button refresh after Delete
    EnableZipBtns(app, HwndWitness(1), 0);
    CHECK(!enabledDelete && !enabledRename);
    // Exact amount normalization over every int16 bit pattern and both limits.
    reset(app);
    notifications = 1;
    comboSelection[1] = 0;
    for (pass = 0; pass < 2; pass++)
        for (n = 0; n < 65536; n++) {
            int narrowed = n < 32768 ? n : n - 65536;
            int maximum = pass ? 100 : 4000;
            expected = narrowed < 0 ? 0 : narrowed > maximum ? maximum : narrowed;
            comboSelection[2] = pass ? 5 : 3;
            snprintf(editText, sizeof(editText), "%d", n);
            writes = alerts = 0;
            amount(app);
            CHECK(orders->rgord()[1].txp.rgia[4].cQuan() == expected && alerts == (narrowed != expected) && writes == (narrowed != expected ? 2 : 1));
        }
    strcpy(editText, "65636");
    comboSelection[2] = 5;
    alerts = 0;
    amount(app);
    CHECK(orders->rgord()[1].txp.rgia[4].cQuan() == 100 && alerts == 0 && strcmp(editText, "65636") == 0);
    strcpy(editText, "-65535");
    comboSelection[2] = 3;
    alerts = 0;
    amount(app);
    CHECK(orders->rgord()[1].txp.rgia[4].cQuan() == 1 && alerts == 0 && strcmp(editText, "-65535") == 0);
    strcpy(editText, "40001234");
    amount(app);
    CHECK(orders->rgord()[1].txp.rgia[4].cQuan() == 2427 && strcmp(editText, "40001234") == 0);
    strcpy(editText, "junk");
    amount(app);
    CHECK(orders->rgord()[1].txp.rgia[4].cQuan() == 0 && strcmp(editText, "junk") == 0);
    reset(app);
    notifications = 1;
    for (i = 0; i < 5; i++) {
        orders->rgord()[1].txp.rgia[i].SetIAction(15);
        orders->rgord()[1].txp.rgia[i].SetCQuan(4095);
    }
    popupResult = 2;
    popup(app);
    CHECK(snapshots[0].txp.rgia[4].cQuan() == 4095 && orders->rgord()[1].txp.rgia[4].cQuan() == 4000 && writes == 3 && alerts == 1);
    CHECK(orders->rgord()[1].txp.rgia[0].cQuan() == 100 && orders->rgord()[1].txp.rgia[3].cQuan() == 4095 && strcmp(comboLabels[2][7], "Load Optimal") == 0);
    reset(app);
    notifications = 1;
    orders->rgord()[1].txp.rgia[0].SetIAction(5);
    orders->rgord()[1].txp.rgia[0].SetCQuan(4095);
    ControlIO().pfnUpdateOrdersDDs(app, 1);
    CHECK(comboSelection[1] == 1 && orders->rgord()[1].txp.rgia[0].cQuan() == 100 && writes == 2 && alerts == 1);
    reset(app);
    notifications = 1;
    hasCargo = 0;
    orders->rgord()[1].txp.rgia[0].SetIAction(5);
    orders->rgord()[1].txp.rgia[0].SetCQuan(4095);
    ControlIO().pfnUpdateOrdersDDs(app, 1);
    CHECK(comboCount[1] == 1 && comboSelection[1] == 0 && orders->rgord()[1].txp.rgia[0].cQuan() == 4095);
    reset(app);
    notifications = 1;
    orders->rgord()[1].txp.rgia[4].SetIAction(15);
    orders->rgord()[1].txp.rgia[4].SetCQuan(4095);
    ControlIO().pfnUpdateOrdersDDs(app, 1);
    CHECK(comboSelection[2] == -1 && orders->rgord()[1].txp.rgia[4].cQuan() == 4000);
    reset(app);
    notifications = 1;
    comboSelection[1] = 1;
    comboSelection[2] = 5;
    orders->rgord()[1].txp.rgia[0].SetCQuan(4095);
    action(app);
    CHECK(snapshots[0].txp.rgia[0].iAction() == 5 && snapshots[0].txp.rgia[0].cQuan() == 4095 && orders->rgord()[1].txp.rgia[0].cQuan() == 100 && writes == 3);
    comboSelection[1] = 0;
    comboSelection[2] = -1;
    action(app);
    CHECK(orders->rgord()[1].txp.rgia[4].iAction() == 15);
    for (pass = 0; pass < 2; pass++)
        for (mask = 0; mask < 32; mask++) {
            int selected = 4, priority[5] = {4, 0, 1, 2, 3};
            reset(app);
            notifications = 1;
            hasCargo = pass;
            for (i = 0; i < 5; i++) {
                orders->rgord()[1].txp.rgia[i].SetIAction((mask >> i) & 1);
                orders->rgord()[1].txp.rgia[i].SetCQuan(4095);
            }
            for (i = 0; i < (pass ? 5 : 1); i++)
                if ((mask >> priority[i]) & 1) {
                    selected = priority[i];
                    break;
                }
            ControlIO().pfnUpdateOrdersDDs(app, 1);
            CHECK(comboSelection[1] == (selected == 4 ? 0 : selected + 1));
            for (i = 0; i < 5; i++)
                CHECK(orders->rgord()[1].txp.rgia[i].cQuan() == (i == selected ? 4000 : 4095));
        }
    for (i = -1; i < 16; i++) {
        int unit;
        int enabled = quantityControl(i, 4, &unit);
        CHECK(enabled == (i != 0 && i != 1 && i != 2 && i != 7) && unit == ((i == 5 || i == 6) ? 5 : 4));
    }
    reset(app);
    app.lpui->iResTechNow = static_cast<TechFieldType>(0);
    app.state.vrgZip[0].txp.rgia[0].SetIAction(5);
    app.state.vrgZip[0].txp.rgia[0].SetCQuan(120);
    paintLabel(app, 0);
    CHECK(strcmp(app.state.szWork, "Fill Up to % 120kT") == 0);
    app.state.vrgZip[0].txp.rgia[0].SetIAction(7);
    paintLabel(app, 0);
    CHECK(strcmp(app.state.szWork, "Load Dunnage") == 0);
    printf("M10 linked transport witnesses: %d assertions passed\n", checks);
    return 0;
}
