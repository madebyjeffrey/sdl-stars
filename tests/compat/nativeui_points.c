/* The legacy point adapter must supply safe coordinates after API failure. */
#include <stdint.h>
#include <windows.h>
#ifdef STARS_UI_SDL
#include "../../src/platform/native.h"
#else
#include "../reference/cpp-a/native.h"
#endif
#include <stdio.h>

static BOOL fCursor;
static POINT ptCursor;
static BOOL FTestGetCursorPos(LPPOINT lppt) {
    if (!fCursor) return FALSE;
    *lppt = ptCursor;
    return TRUE;
}

#define GetCursorPos FTestGetCursorPos
#ifdef STARS_UI_SDL
#include "../../src/ui/nativeui.h"
#else
#include "../reference/cpp-a/nativeui.h"
#endif
#undef GetCursorPos

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "Point adapter check failed at line %d\n", __LINE__); return 1; } } while (0)

int main(void) {
    POINT16 pt = {17, 29};
    static const POINT rgpt[] = {{0, 0}, {123, -456}, {-32768, 32767}};
    size_t i;

    CHECK(!GetCursorPos16(&pt) && pt.x == 0 && pt.y == 0);
    fCursor = TRUE;
    for (i = 0; i < sizeof(rgpt) / sizeof(rgpt[0]); i++) {
        ptCursor = rgpt[i];
        CHECK(GetCursorPos16(&pt) && pt.x == ptCursor.x && pt.y == ptCursor.y);
    }
    fCursor = FALSE;
    CHECK(!GetCursorPos16(&pt) && pt.x == 0 && pt.y == 0);
    return 0;
}
