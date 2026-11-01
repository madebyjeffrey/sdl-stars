#ifndef STARS_SCAN_INSPECT_H
#define STARS_SCAN_INSPECT_H

#include "win.h"
#include "ownership.h"
#include <windows.h>
#include <cstdint>
#include <cstdio>

/* UI-thread, copied player-client data. Object keys are (game, turn, player,
 * kind, id), not array indices or pointers. Re-capture after loading a turn.
 * Planet coordinates/names are public even before the planet is explored.
 */
struct SCANOBJECT {
    std::uint16_t grType, id;
    std::int16_t iPlayer, det, ith;
    POINT ptUniverse, ptClient, ptScreen;
    BOOL fOnScreen, fFiltered, fScanSelected, fMainSelected;
    char szName[360];
};
struct SCANSNAPSHOT {
    std::int32_t lidGame;
    std::uint16_t turn;
    std::int16_t idPlayer, iZoom;
    RECT rcViewport;
    NATIVEARRAYOWNER<SCANOBJECT, std::free> rgobj;
    std::size_t cObject;
};
BOOL FCaptureScanner(APPSESSION &app, SCANSNAPSHOT *lpsnap);
void FreeScannerSnapshot(SCANSNAPSHOT *lpsnap);
BOOL FWriteScannerSnapshot(const SCANSNAPSHOT *lpsnap, NATIVEFILE *lpfile);
/* Select the secondary scanner object through ChangeScanSel; optionally pan.
 * No orders are issued and the main object/waypoint editing mode is untouched.
 * Fail for a stale game/turn/player or absent object, including host data.
 */
BOOL FSelectScannerObject(APPSESSION &app, const SCANSNAPSHOT *lpsnap, std::uint16_t grType, std::uint16_t id, BOOL fReveal);
HWND ShowScannerInspector(void);
void CloseScannerInspector(void);

#endif
