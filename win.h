#ifndef STARS_DECOMPILED_WIN_H
#define STARS_DECOMPILED_WIN_H

// The Windows game's user interface: Win32, the game code (common.h) and
// the UI's own headers. The game code includes only common.h.

#include <windows.h>
#include <commctrl.h>

#include <direct.h>
#include <io.h>

#include "common.h"

// Application messages (WM_USER + 0x64...).
#define WM_STARS_STARTUP  0x0464
#define WM_STARS_HOST     0x0465
#define WM_STARS_CONTINUE 0x0466
#define WM_STARS_SCAN_RESIZE (WM_APP + 2)

#include "battleui.h"
#include "buildui.h"
#include "createui.h"
#include "fileui.h"
#include "globalsui.h"
#include "init.h"
#include "logui.h"
#include "mdi.h"
#include "mineui.h"
#include "msgui.h"
#include "nativeui.h"
#include "planetui.h"
#include "popup.h"
#include "produceui.h"
#include "raceui.h"
#include "reportui.h"
#include "researchui.h"
#include "scan.h"
#include "scaninspect.h"
#include "ship2ui.h"
#include "shipui.h"
#include "stars.h"
#include "tb.h"
#include "thingui.h"
#include "tutor.h"
#include "tutor2.h"
#include "utilgenui.h"
#include "utilui.h"
#include "vcr.h"

#endif
