/* Heap handle contracts shared with the actual Windows SDK. */
#include <windows.h>
#if !defined(_WIN32) || defined(STARS_UI_SDL)
#include <stars_compat.h>
#endif
#include <stdio.h>
#include <string.h>
#define CHECK(f) do { if (!(f)) { fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)

int main(void) {
    HGLOBAL h, hGrow, hZero, hFixed;
    HLOCAL hLocal;
    BYTE *rgb;
    size_t i;

    h = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 17);
    CHECK(h && GlobalSize(h) >= 17);
    rgb = GlobalLock(h);
    CHECK(rgb && (HANDLE)rgb != h && GlobalHandle(rgb) == h);
    for (i = 0; i < 17; i++) CHECK(rgb[i] == 0);
    memcpy(rgb, "persistent bytes", 17);
    CHECK(GlobalLock(h) == rgb && (GlobalFlags(h) & GMEM_LOCKCOUNT) == 2);
    CHECK(GlobalUnlock(h));
    SetLastError(123);
    CHECK(!GlobalUnlock(h) && GetLastError() == NO_ERROR);
    CHECK(!GlobalUnlock(h) && GetLastError() == ERROR_NOT_LOCKED);
    hGrow = GlobalReAlloc(h, 257, GMEM_ZEROINIT);
    CHECK(hGrow == h && GlobalSize(h) >= 257);
    rgb = GlobalLock(h);
    CHECK(rgb && !memcmp(rgb, "persistent bytes", 17));
    for (i = 17; i < 257; i++) CHECK(rgb[i] == 0);
    CHECK(!GlobalUnlock(h));
    CHECK(GlobalReAlloc(h, 8, 0) == h && GlobalSize(h) >= 8);
    rgb = GlobalLock(h);
    CHECK(rgb && !memcmp(rgb, "persiste", 8));
    CHECK(!GlobalUnlock(h) && !GlobalFree(h));

    hZero = GlobalAlloc(GMEM_MOVEABLE, 0);
    CHECK(hZero && GlobalSize(hZero) == 0 && !GlobalLock(hZero));
    CHECK(GlobalFlags(hZero) & GMEM_DISCARDED);
    CHECK(GlobalReAlloc(hZero, 20, GMEM_ZEROINIT) == hZero);
    rgb = GlobalLock(hZero);
    CHECK(rgb && rgb[0] == 0 && rgb[19] == 0 && !GlobalUnlock(hZero));
    CHECK(GlobalReAlloc(hZero, 0, GMEM_MOVEABLE) == hZero && !GlobalLock(hZero));
    CHECK(!GlobalFree(hZero));

    hFixed = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, 24);
    CHECK(hFixed && GlobalLock(hFixed) == hFixed && GlobalHandle(hFixed) == hFixed);
    CHECK(GlobalUnlock(hFixed) && (GlobalFlags(hFixed) & GMEM_LOCKCOUNT) == 0);
    CHECK(((BYTE *)hFixed)[23] == 0 && !GlobalFree(hFixed));
    hLocal = LocalAlloc(LMEM_ZEROINIT, 40);
    CHECK(hLocal && LocalLock(hLocal) == hLocal && LocalSize(hLocal) >= 40);
    CHECK(((BYTE *)hLocal)[39] == 0 && !LocalFree(hLocal));
    hLocal = LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, 12);
    rgb = LocalLock(hLocal);
    CHECK(hLocal && rgb && LocalHandle(rgb) == hLocal && rgb[11] == 0);
    CHECK(!LocalUnlock(hLocal) && !LocalFree(hLocal));
    CHECK(!GlobalFree(NULL) && !LocalFree(NULL));
#if !defined(_WIN32) || defined(STARS_UI_SDL)
    CHECK(!GlobalLock(h) && GetLastError() == ERROR_INVALID_HANDLE);
    CHECK(GlobalFree(h) == h && GlobalFlags(h) == GMEM_INVALID_HANDLE);
    CHECK(!GlobalAlloc(0x80000000u, 12) && GetLastError() == ERROR_INVALID_PARAMETER);
    StarsCompatShutdown();
#endif
    puts("Heap handles: PASS (fixed/movable, zero fill, realloc, locks, discarded blocks, errors)");
    return 0;
}
