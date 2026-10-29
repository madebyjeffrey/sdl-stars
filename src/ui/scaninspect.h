#ifndef STARS_SCAN_INSPECT_H
#define STARS_SCAN_INSPECT_H

#include "win.h"
#include <windows.h>
#include <stdint.h>
#include <stdio.h>

/* UI-thread, copied player-client data. Object keys are (game, turn, player,
 * kind, id), not array indices or pointers. Re-capture after loading a turn.
 * Planet coordinates/names are public even before the planet is explored.
 */
typedef struct SCANOBJECT {
    uint16_t grType, id;
    int16_t iPlayer, det, ith;
    POINT ptUniverse, ptClient, ptScreen;
    BOOL fOnScreen, fFiltered, fScanSelected, fMainSelected;
    char szName[360];
} SCANOBJECT;
typedef struct SCANSNAPSHOT {
    int32_t lidGame;
    uint16_t turn;
    int16_t idPlayer, iZoom;
    RECT rcViewport;
    SCANOBJECT *rgobj;
    size_t cObject;
} SCANSNAPSHOT;
BOOL FCaptureScanner(SCANSNAPSHOT *lpsnap);
void FreeScannerSnapshot(SCANSNAPSHOT *lpsnap);
BOOL FWriteScannerSnapshot(const SCANSNAPSHOT *lpsnap, NATIVEFILE *lpfile);
/* Select the secondary scanner object through ChangeScanSel; optionally pan.
 * No orders are issued and the main object/waypoint editing mode is untouched.
 * Fail for a stale game/turn/player or absent object, including host data.
 */
BOOL FSelectScannerObject(const SCANSNAPSHOT *lpsnap, uint16_t grType, uint16_t id, BOOL fReveal);
HWND ShowScannerInspector(void);
void CloseScannerInspector(void);

#endif
