#include "win.h"
#include "text_io.h"
#include "record_io.h"

int16_t FWriteTutorialMFile(int16_t iTurn) {
    HRSRC    hrsrc;
    char     szT[30];
    HGLOBAL  hres;
    int16_t  cch;
    int16_t  cSkip;

    cSkip = iTurn;
    StarsRecoveryScope scopeRecovery;
    try {
        if (iTurn < 32) {
            hrsrc = win32::FindResourceA(hInst, win32::MAKEINTRESOURCE(10003), win32::MAKEINTRESOURCE(10002));
        } else {
            hrsrc = win32::FindResourceA(hInst, win32::MAKEINTRESOURCE(10005), win32::MAKEINTRESOURCE(10004));
            cSkip -= 32;
        }
        hres = win32::LoadResource(hInst, hrsrc);
        if (!hres) {
        BailOut:
            return 0;
        }
        vlpMemStream = (uint8_t *)win32::LockResource(hres);
        if (!vlpMemStream)
            goto BailOut;
        if (cSkip >= *vlpMemStream) {
            vlpMemStream = NULL;
            win32::GlobalUnlock(hres);
            win32::FreeResource(hres);
            return 2;
        }
        vlpMemStream++;
        while (cSkip-- != 0) {
            do {
                vlpMemStream += 2 + HdvFromPrefix(vlpMemStream).cb();
            } while (HdvFromPrefix(vlpMemStream).rt() != rtBOF);
        }
        cch = TextIO().pfnCchGetString(idsTutorial, szT);
        strcpy(&szT[cch], iTurn == 37 ? ".hst" : ".m1");
        RecordIO().pfnStreamOpen(szT, mdCreate);
        do {
            RgToStream(vlpMemStream, HdvFromPrefix(vlpMemStream).cb() + 2);
            vlpMemStream += 2 + HdvFromPrefix(vlpMemStream).cb();
        } while (HdvFromPrefix(vlpMemStream).rt() != rtBOF);
        RecordIO().pfnStreamClose();
        vlpMemStream = NULL;
        win32::GlobalUnlock(hres);
        win32::FreeResource(hres);
        return 1;
    } catch (const StarsError &) {
        if (vlpMemStream) {
            win32::GlobalUnlock(hres);
            win32::FreeResource(hres);
            return 0;
        }
        if (hf == -1) {
            return 1;
        }
        RecordIO().pfnStreamClose();
        return 0;
    }
}

// LpbLoadTutorLog returns the tutorial's recorded orders for the player,
// kept in the program's resources, or NULL. FLoadLogFile replays them. The
// resource stays loaded for the life of the program.
uint8_t *LpbLoadTutorLog() {
    HRSRC   hrsrc;
    HGLOBAL hres;

    hrsrc = win32::FindResourceA(hInst, win32::MAKEINTRESOURCE(10001), win32::MAKEINTRESOURCE(10000));
    hres = win32::LoadResource(hInst, hrsrc);
    if (!hres) {
        return NULL;
    }
    return (uint8_t *)win32::LockResource(hres);
}
