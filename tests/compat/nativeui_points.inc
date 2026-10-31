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
    if (!fCursor) return 0;
    *lppt = ptCursor;
    return 1;
}

#ifdef STARS_UI_SDL
#include "../../src/ui/nativeui.h"
static BOOL FReadCursor16(POINT16 *lppt) { return GetCursorPos16(lppt, FTestGetCursorPos); }
#else
// The independent frozen C adapter retains its historical injection boundary.
#define GetCursorPos FTestGetCursorPos
#include "../reference/cpp-a/nativeui.h"
#undef GetCursorPos
static BOOL FReadCursor16(POINT16 *lppt) { return GetCursorPos16(lppt); }
#endif

#define CHECK(f) do { if (!(f)) { fprintf(stderr, "Point adapter check failed at line %d\n", __LINE__); return 1; } } while (0)

int main(void) {
    POINT16 pt = {17, 29};
    static const POINT rgpt[] = {{0, 0}, {123, -456}, {-32768, 32767}};
    size_t i;

    CHECK(!FReadCursor16(&pt) && pt.x == 0 && pt.y == 0);
    fCursor = 1;
    for (i = 0; i < sizeof(rgpt) / sizeof(rgpt[0]); i++) {
        ptCursor = rgpt[i];
        CHECK(FReadCursor16(&pt) && pt.x == ptCursor.x && pt.y == ptCursor.y);
    }
    fCursor = 0;
    CHECK(!FReadCursor16(&pt) && pt.x == 0 && pt.y == 0);
    return 0;
}
