#include "sessionrestore.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include "ui_fixture.h"
static PLANET  planets[16];
static FLEET   fleets[16];
static FLEET  *fleetPointers[16];
static int16_t ids[8];
static char    trace[1024];
static int     mutateSelect, mutateTitle, mutateTutor, lookups, checks;
static void    event(const char *text) {
    assert(strlen(trace) + strlen(text) < sizeof(trace));
    strcat(trace, text);
}
static void selected(APPSESSION &app, char kind, int16_t id) {
    char text[32];
    snprintf(text, sizeof(text), "%c%d;", kind, id);
    event(text);
    if (mutateSelect) {
        app.state.ini.grobjSel = 4;
        app.state.ini.iObjSel = 5;
        mutateSelect = 0;
    }
}
static void TestSelectAdjPlanet(APPSESSION &app, int16_t delta, int16_t id) {
    assert(delta == 0);
    selected(app, 'P', id);
}
static void TestSelectAdjFleet(APPSESSION &app, int16_t delta, int16_t id) {
    assert(delta == 0);
    selected(app, 'L', id);
}
static int16_t TestIdmGetMessageN(APPSESSION &app, int16_t i) {
    assert(i >= 0 && i < app.state.cMsg);
    lookups++;
    return ids[i];
}
static void TestSetMsgTitle(APPSESSION &app, void *hwnd) {
    assert(hwnd == app.lpui->hwndMessage);
    event("T;");
    if (mutateTitle)
        app.state.gd.fTutorial = 0;
}
static void TestInvalidateRect(void *hwnd, void *rect, int erase) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    assert(!rect && erase);
    if (hwnd == app.lpui->hwndMessage)
        event("I;");
    else {
        assert(hwnd == app.lpui->hwndScanner && !app.state.gd.fNoScannerDraw);
        event("S;");
    }
}
static void TestAdvanceTutor(APPSESSION &app) {
    assert(app.state.tutor.fChange);
    event("A;");
    app.state.tutor.fChange = 0;
    if (mutateTutor) {
        app.state.ini.grobjSel = 1;
        app.state.ini.iObjSel = 5;
        planets[1].iPlayer = app.state.idPlayer;
        app.state.iMsgCur = 3;
        app.state.gd.fGotoVCR = 1;
    }
}
static void TestRefitFrameChildren(APPSESSION &app) {
    assert(app.state.gd.fNoScannerDraw);
    event("R;");
}
static void TestFUiUpdateWindow(void *hwnd) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    assert(hwnd == app.lpui->hwndScanner && !app.state.gd.fNoScannerDraw);
    event("U;");
}
static void TestFUiDestroyWindow(void *hwnd) {
    (void)hwnd;
    event("D;");
}
static void TestBringUpHostDlg(APPSESSION &) { event("H;"); }
static void reset(APPSESSION &app) {
    int i;
    memset(planets, 0, sizeof(planets));
    memset(fleets, 0, sizeof(fleets));
    memset(app.state.rglpfl, 0, sizeof(fleetPointers));
    memset(app.state.rgplr, 0, sizeof(app.state.rgplr));
    memset(app.state.bitfMsgFiltered, 0, sizeof(app.state.bitfMsgFiltered));
    app.state.idPlayer = 0;
    app.state.game.lid = 7;
    app.state.game.turn = 2400;
    app.state.game.cPlanMax = 10;
    app.state.rgplr[0].idPlanetHome = 2;
    app.state.rgplr[0].cFleet = 1;
    app.state.cPlanet = 2;
    planets[0].id = 2;
    planets[0].iPlayer = 0;
    planets[1].id = 5;
    planets[1].iPlayer = 1;
    app.state.cFleet = 1;
    fleets[0].SetIfl(0);
    fleets[0].SetIplr(0);
    fleets[0].iPlayer = 0;
    app.state.rglpfl[0] = &fleets[0];
    app.state.ini.idPlayer = 0;
    app.state.ini.iObjSel = 0;
    app.state.ini.grobjSel = 2;
    app.state.ini.lid = 7;
    app.state.ini.turn = 2400;
    app.state.ini.iMsg = 1;
    app.state.cMsg = 3;
    app.state.vcmsgplrIn = 1;
    app.state.iMsgCur = 3;
    app.state.fViewFilteredMsg = 0;
    for (i = 0; i < 8; i++)
        ids[i] = (int16_t)(i * 8 + 1);
    app.state.bitfMsgFiltered[ids[1] >> 3] |= 1 << (ids[1] & 7);
    app.state.gd.fGotoVCR = 1;
    app.state.gd.fTutorial = 1;
    app.state.gd.fNoScannerDraw = 0;
    app.state.tutor.fChange = 0;
    app.lpui->hwndScanner = HwndWitness(2);
    app.lpui->hwndTitle = NULL;
    app.lpui->fFreeingTitle = 0;
    mutateSelect = mutateTitle = mutateTutor = lookups = 0;
    trace[0] = 0;
}
#define CHECK(x)                                                                                                                                               \
    do {                                                                                                                                                       \
        checks++;                                                                                                                                              \
        if (!(x)) {                                                                                                                                            \
            fprintf(stderr, "witness %d failed: %s; trace=%s\n", checks, #x, trace);                                                                           \
            return 1;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)
static int16_t TestFind(APPSESSION &app) {
    event("F;");
    return FFindSomethingAndSelectIt(app);
}
static void TestSend(HWND hwnd, int message, int command, int param) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    assert(hwnd == app.lpui->hwndFrame && message == win32::WM_COMMAND && command == IDM_FRAME_POST_OPEN && param == 0);
    RestoreGameWindows(app);
}

int main(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    int            n, inbound, mask, show, only, at, j, k, expected, next, previous, eligible[8], count;
    PRESENTATIONIO viewIO;
    viewIO.pfnSelectPlanet = TestSelectAdjPlanet;
    viewIO.pfnSelectFleet = TestSelectAdjFleet;
    viewIO.pfnMessageId = TestIdmGetMessageN;
    viewIO.pfnFindSelection = TestFind;
    viewIO.pfnTutor = TestAdvanceTutor;
    PRESENTATIONIOSCOPE viewScope(viewIO);
    CONTROLIO           controlIO;
    controlIO.pfnSetMsgTitle = WITNESSADAPTER<&TestSetMsgTitle, decltype(controlIO.pfnSetMsgTitle)>::Call;
    controlIO.pfnInvalidateRect = WITNESSADAPTER<&TestInvalidateRect, decltype(controlIO.pfnInvalidateRect)>::Call;
    controlIO.pfnRefitFrameChildren = WITNESSADAPTER<&TestRefitFrameChildren, decltype(controlIO.pfnRefitFrameChildren)>::Call;
    controlIO.pfnUpdateWindow = WITNESSADAPTER<&TestFUiUpdateWindow, decltype(controlIO.pfnUpdateWindow)>::Call;
    controlIO.pfnDestroyWindow = WITNESSADAPTER<&TestFUiDestroyWindow, decltype(controlIO.pfnDestroyWindow)>::Call;
    controlIO.pfnBringUpHostDlg = WITNESSADAPTER<&TestBringUpHostDlg, decltype(controlIO.pfnBringUpHostDlg)>::Call;
    controlIO.pfnSendMessage = WITNESSADAPTER<&TestSend, decltype(controlIO.pfnSendMessage)>::Call;
    CONTROLIOSCOPE controlScope(controlIO);
    app.state.lpPlanets = planets;
    app.state.rglpfl = fleetPointers;
    app.lpui->hwndMessage = HwndWitness(1);
    app.lpui->hwndScanner = HwndWitness(2);
    app.lpui->hwndFrame = HwndWitness(3);
    reset(app);
    RestoreSelection(app);
    CHECK(strcmp(trace, "L0;T;I;A;") == 0 && app.state.ini.grobjSel == 0 && app.state.iMsgCur == 2 && !app.state.gd.fGotoVCR && !app.state.tutor.fChange);
    reset(app);
    app.state.ini.lid = 8;
    RestoreSelection(app);
    CHECK(strcmp(trace, "P2;") == 0 && app.state.ini.iObjSel == 2 && app.state.ini.grobjSel == 0 && app.state.iMsgCur == 3 && app.state.gd.fGotoVCR);
    reset(app);
    app.state.ini.iObjSel = 512;
    RestoreSelection(app);
    CHECK(strcmp(trace, "T;I;A;P2;") == 0 && app.state.ini.iObjSel == 2 && app.state.ini.grobjSel == 0);
    reset(app);
    app.state.ini.grobjSel = 1;
    app.state.ini.iObjSel = 5;
    RestoreSelection(app);
    CHECK(strcmp(trace, "T;I;A;P2;") == 0 && app.state.ini.iObjSel == 2);
    reset(app);
    app.state.ini.grobjSel = 4;
    app.state.ini.iObjSel = 5;
    RestoreSelection(app);
    CHECK(strcmp(trace, "T;I;A;F;P2;") == 0 && app.state.ini.iObjSel == 5 && app.state.ini.grobjSel == 0);
    reset(app);
    app.state.ini.grobjSel = 4;
    app.state.ini.iObjSel = 2;
    RestoreSelection(app);
    CHECK(strcmp(trace, "T;I;A;P2;") == 0 && app.state.ini.grobjSel == 0);
    reset(app);
    app.state.ini.grobjSel = 0;
    app.state.ini.iMsg = 0;
    RestoreSelection(app);
    CHECK(strcmp(trace, "") == 0 && app.state.iMsgCur == 3 && app.state.gd.fGotoVCR);
    reset(app);
    app.state.ini.turn = 2399;
    RestoreSelection(app);
    CHECK(strcmp(trace, "L0;") == 0 && app.state.iMsgCur == 3 && app.state.gd.fGotoVCR);
    reset(app);
    app.state.ini.iMsg = 99;
    RestoreSelection(app);
    CHECK(app.state.iMsgCur == 0 && strcmp(trace, "L0;T;I;A;") == 0);
    reset(app);
    app.state.ini.iMsg = 4;
    RestoreSelection(app);
    CHECK(app.state.iMsgCur == -1);
    reset(app);
    app.state.ini.iObjSel = 8192;
    RestoreSelection(app);
    CHECK(strcmp(trace, "L8192;T;I;A;") == 0 && app.state.ini.grobjSel == 0);
    reset(app);
    mutateSelect = 1;
    RestoreSelection(app);
    CHECK(app.state.ini.grobjSel == 0 && app.state.ini.iObjSel == 5 && strcmp(trace, "L0;T;I;A;") == 0);
    reset(app);
    mutateTitle = 1;
    RestoreSelection(app);
    CHECK(strcmp(trace, "L0;T;I;") == 0);
    reset(app);
    mutateTutor = 1;
    RestoreSelection(app);
    CHECK(strcmp(trace, "L0;T;I;A;P5;") == 0 && app.state.ini.grobjSel == 0 && app.state.iMsgCur == 3 && app.state.gd.fGotoVCR);
    reset(app);
    app.state.cPlanet = 0;
    app.state.rgplr[0].idPlanetHome = -1;
    app.state.ini.lid = 8;
    app.state.cFleet = 2;
    app.state.rglpfl[0] = NULL;
    app.state.rglpfl[1] = &fleets[0];
    RestoreSelection(app);
    CHECK(strcmp(trace, "F;") == 0 && app.state.ini.grobjSel == 0);
    reset(app);
    planets[0].iPlayer = 1;
    planets[1].iPlayer = 0;
    CHECK(TestFind(app) && strcmp(trace, "F;P5;") == 0);
    reset(app);
    app.state.cPlanet = 0;
    CHECK(TestFind(app) && strcmp(trace, "F;L0;") == 0);
    reset(app);
    app.state.cPlanet = 0;
    app.state.cFleet = 0;
    CHECK(!TestFind(app) && strcmp(trace, "F;") == 0);
    reset(app);
    CHECK(LpplFromId(app, 5) == &planets[1] && LpplFromId(app, 1) == NULL && LpplFromId(app, -1) == NULL);
    app.state.game.cPlanMax = 2;
    CHECK(LpplFromId(app, 1) == &planets[1] && LpplFromId(app, 1)->id == 5 && LpplFromId(app, 2) == NULL);
    reset(app);
    CHECK(LpflFromId(app, 8192) == &fleets[0] && LpflFromId(app, 512) == NULL);
    reset(app);
    app.state.cFleet = 2;
    app.state.rgplr[1].cFleet = 1;
    fleets[1].SetIfl(0);
    fleets[1].SetIplr(1);
    fleets[1].iPlayer = 1;
    app.state.rglpfl[1] = &fleets[1];
    CHECK(LpflFromId(app, 8704) == &fleets[1]);
    reset(app);
    app.state.cMsg = 2;
    app.state.vcmsgplrIn = 1;
    ids[1] = ids[0];
    memset(app.state.bitfMsgFiltered, 0, sizeof(app.state.bitfMsgFiltered));
    app.state.bitfMsgFiltered[ids[0] >> 3] = 1 << (ids[0] & 7);
    app.state.iMsgCur = -1;
    CHECK(IMsgNext(app, 0) == 2 && IMsgNext(app, 1) == 0);
    app.state.iMsgCur = 0;
    CHECK(IMsgNext(app, 1) == 1);
    reset(app);
    RestoreGameWindows(app);
    CHECK(strcmp(trace, "L0;T;I;A;R;S;U;") == 0 && !app.state.gd.fNoScannerDraw);
    reset(app);
    app.lpui->hwndScanner = NULL;
    app.state.gd.fNoScannerDraw = 1;
    RestoreGameWindows(app);
    CHECK(strcmp(trace, "") == 0 && app.state.ini.grobjSel == 2 && app.state.gd.fNoScannerDraw);
    reset(app);
    CHECK(FinishOpenGame(app) == 1 && strcmp(trace, "L0;T;I;A;R;S;U;") == 0);
    reset(app);
    app.state.ini.grobjSel = 0;
    app.state.ini.lid = 8;
    CHECK(FinishOpenGame(app) == 1 && strcmp(trace, "P2;R;S;U;F;P2;") == 0);
    reset(app);
    app.lpui->hwndScanner = NULL;
    CHECK(FinishOpenGame(app) == 1 && strcmp(trace, "F;P2;") == 0 && app.state.ini.grobjSel == 0);
    reset(app);
    app.lpui->hwndScanner = NULL;
    app.state.cPlanet = 0;
    CHECK(FinishOpenGame(app) == 1 && strcmp(trace, "") == 0 && app.state.ini.grobjSel == 0);
    reset(app);
    app.state.idPlayer = iplrNone;
    CHECK(FinishOpenGame(app) == 0 && strcmp(trace, "H;") == 0);
    // Independent eligible-index oracle, across all small message views/modes.
    for (n = 0; n <= 4; n++)
        for (inbound = 0; inbound <= 2; inbound++)
            for (mask = 0; mask < (1 << n); mask++)
                for (show = 0; show < 2; show++)
                    for (only = 0; only < 2; only++) {
                        reset(app);
                        app.state.cMsg = n;
                        app.state.vcmsgplrIn = inbound;
                        app.state.fViewFilteredMsg = show;
                        memset(app.state.bitfMsgFiltered, 0, sizeof(app.state.bitfMsgFiltered));
                        count = 0;
                        for (j = 0; j < n + inbound; j++) {
                            int filtered = j < n && (mask & (1 << j));
                            if (filtered)
                                app.state.bitfMsgFiltered[ids[j] >> 3] |= 1 << (ids[j] & 7);
                            if (j >= n || (show && !only) || !!filtered == only)
                                eligible[count++] = j;
                        }
                        for (at = -1; at <= n + inbound; at++) {
                            app.state.iMsgCur = (int16_t)at;
                            lookups = 0;
                            expected = -1;
                            for (k = count - 1; k >= 0; k--)
                                if (eligible[k] < at) {
                                    expected = eligible[k];
                                    break;
                                }
                            previous = IMsgPrev(app, (int16_t)only);
                            CHECK(previous == expected);
                            if (show && !only)
                                CHECK(lookups == 0);
                            if (at <= n + inbound) {
                                expected = -1;
                                lookups = 0;
                                for (k = 0; k < count; k++)
                                    if (eligible[k] > at) {
                                        expected = eligible[k];
                                        break;
                                    }
                                next = IMsgNext(app, (int16_t)only);
                                CHECK(next == expected);
                                if (show && !only)
                                    CHECK(lookups == 0);
                                if (next != -1)
                                    for (k = 0; k < count; k++)
                                        if (eligible[k] == at) {
                                            app.state.iMsgCur = (int16_t)next;
                                            CHECK(IMsgPrev(app, (int16_t)only) == at);
                                        }
                            }
                        }
                    }
    printf("Restoration, lookup and message linked witnesses: %d assertions passed\n", checks);
    return 0;
}
