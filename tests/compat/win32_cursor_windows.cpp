/* Opt-in comparison with the system cursor; no cursor motion is injected. */
#include "internal.h"
#include <stars_compat.h>
#include <stdio.h>
#include <string.h>

int FTestNativeCursor(int32_t *lpx, int32_t *lpy);
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

int main(void) {
    POINT pt;
    int32_t xBefore, yBefore, xAfter, yAfter;
    int i;

    CHECK(FCompatVideo() && strcmp(SDL_GetCurrentVideoDriver(), "windows") == 0);
    for (i = 0; i < 10; i++) {
        CHECK(FTestNativeCursor(&xBefore, &yBefore));
        CHECK(GetCursorPos(&pt));
        CHECK(FTestNativeCursor(&xAfter, &yAfter));
        if (xBefore == xAfter && yBefore == yAfter) {
            CHECK(pt.x == xBefore && pt.y == yBefore);
            break;
        }
    }
    CHECK(i < 10);
    StarsCompatShutdown(); SDL_Quit();
    puts("Windows SDL desktop cursor matches the system cursor");
    return 0;
}
