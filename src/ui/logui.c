#include "win.h"

int16_t FWriteTutorialMFile(int16_t iTurn) {
    HRSRC    hrsrc;
    char     szT[30];
    HGLOBAL  hres;
    jmp_buf *penvMemSav;
    jmp_buf  env;
    int16_t  cch;
    int16_t  cSkip;

    cSkip = iTurn;
    penvMemSav = penvMem;
    penvMem = &env;
    if (setjmp(env) != 0) {
        penvMem = penvMemSav;
        if (vlpMemStream) {
            GlobalUnlock(hres);
            FreeResource(hres);
            return 0;
        }
        if (hf == -1) {
            return 1;
        }
        StreamClose();
        return 0;
    }
    if (iTurn < 32) {
        hrsrc = FindResource(hInst, MAKEINTRESOURCE(10003), MAKEINTRESOURCE(10002));
    } else {
        hrsrc = FindResource(hInst, MAKEINTRESOURCE(10005), MAKEINTRESOURCE(10004));
        cSkip -= 32;
    }
    hres = LoadResource(hInst, hrsrc);
    if (!hres) {
    BailOut:
        penvMem = penvMemSav;
        return 0;
    }
    vlpMemStream = LockResource(hres);
    if (!vlpMemStream)
        goto BailOut;
    if (cSkip >= *vlpMemStream) {
        vlpMemStream = NULL;
        GlobalUnlock(hres);
        FreeResource(hres);
        penvMem = penvMemSav;
        return 2;
    }
    vlpMemStream++;
    while (cSkip-- != 0) {
        do {
            vlpMemStream += 2 + ((HDR *)vlpMemStream)->cb;
        } while (((HDR *)vlpMemStream)->rt != rtBOF);
    }
    cch = CchGetString(idsTutorial, szT);
    strcpy(&szT[cch], iTurn == 37 ? ".hst" : ".m1");
    StreamOpen(szT, mdCreate);
    do {
        RgToStream(vlpMemStream, ((HDR *)vlpMemStream)->cb + 2);
        vlpMemStream += 2 + ((HDR *)vlpMemStream)->cb;
    } while (((HDR *)vlpMemStream)->rt != rtBOF);
    StreamClose();
    vlpMemStream = NULL;
    GlobalUnlock(hres);
    FreeResource(hres);
    penvMem = penvMemSav;
    return 1;
}

// LpbLoadTutorLog returns the tutorial's recorded orders for the player,
// kept in the program's resources, or NULL. FLoadLogFile replays them. The
// resource stays loaded for the life of the program.
uint8_t *LpbLoadTutorLog() {
    HRSRC   hrsrc;
    HGLOBAL hres;

    hrsrc = FindResource(hInst, MAKEINTRESOURCE(10001), MAKEINTRESOURCE(10000));
    hres = LoadResource(hInst, hrsrc);
    if (!hres) {
        return NULL;
    }
    return LockResource(hres);
}
