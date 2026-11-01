#include "win.h"
#include "appstate.h"
#include "uistate.h"
#include "text_io.h"
#include "record_io.h"

int16_t FWriteTutorialMFile(APPSESSION &app, int16_t iTurn) {
    HRSRC    hrsrc;
    char     szT[30];
    HGLOBAL  hres;
    int16_t  cch;
    int16_t  cSkip;

    cSkip = iTurn;
    StarsRecoveryScope scopeRecovery;
    try {
        if (iTurn < 32) {
            hrsrc = win32::FindResourceA(app.lpui->hInst, win32::MAKEINTRESOURCE(10003), win32::MAKEINTRESOURCE(10002));
        } else {
            hrsrc = win32::FindResourceA(app.lpui->hInst, win32::MAKEINTRESOURCE(10005), win32::MAKEINTRESOURCE(10004));
            cSkip -= 32;
        }
        hres = win32::LoadResource(app.lpui->hInst, hrsrc);
        if (!hres) {
        BailOut:
            return 0;
        }
        app.state.vlpMemStream = (uint8_t *)win32::LockResource(hres);
        if (!app.state.vlpMemStream)
            goto BailOut;
        if (cSkip >= *app.state.vlpMemStream) {
            app.state.vlpMemStream = NULL;
            win32::GlobalUnlock(hres);
            win32::FreeResource(hres);
            return 2;
        }
        app.state.vlpMemStream++;
        while (cSkip-- != 0) {
            do {
                app.state.vlpMemStream += 2 + HdvFromPrefix(app.state.vlpMemStream).cb();
            } while (HdvFromPrefix(app.state.vlpMemStream).rt() != rtBOF);
        }
        cch = TextIO().pfnCchGetString(app, idsTutorial, szT);
        strcpy(&szT[cch], iTurn == 37 ? ".hst" : ".m1");
        RecordIO().pfnStreamOpen(app, szT, mdCreate);
        do {
            RgToStream(app, app.state.vlpMemStream, HdvFromPrefix(app.state.vlpMemStream).cb() + 2);
            app.state.vlpMemStream += 2 + HdvFromPrefix(app.state.vlpMemStream).cb();
        } while (HdvFromPrefix(app.state.vlpMemStream).rt() != rtBOF);
        RecordIO().pfnStreamClose(app);
        app.state.vlpMemStream = NULL;
        win32::GlobalUnlock(hres);
        win32::FreeResource(hres);
        return 1;
    } catch (const StarsError &) {
        if (app.state.vlpMemStream) {
            win32::GlobalUnlock(hres);
            win32::FreeResource(hres);
            return 0;
        }
        if (app.state.hf == -1) {
            return 1;
        }
        RecordIO().pfnStreamClose(app);
        return 0;
    }
}

// LpbLoadTutorLog returns the tutorial's recorded orders for the player,
// kept in the program's resources, or NULL. FLoadLogFile replays them. The
// resource stays loaded for the life of the program.
uint8_t *LpbLoadTutorLog(APPSESSION &app) {
    HRSRC   hrsrc;
    HGLOBAL hres;

    hrsrc = win32::FindResourceA(app.lpui->hInst, win32::MAKEINTRESOURCE(10001), win32::MAKEINTRESOURCE(10000));
    hres = win32::LoadResource(app.lpui->hInst, hrsrc);
    if (!hres) {
        return NULL;
    }
    return (uint8_t *)win32::LockResource(hres);
}
