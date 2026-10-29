/* Exercise allocation failure after CopyIcon has allocated its pixel planes. */
#include "internal.h"
#include <stars_compat.h>
#include <cstdlib>
#include <new>
#include <cstdio>

static bool fFailNew, fObserveDestroy;
static int cDestroyed;

void *operator new(std::size_t cb) {
    if (fFailNew) { fFailNew = false; throw std::bad_alloc(); }
    if (void *lp = std::malloc(cb ? cb : 1)) return lp;
    throw std::bad_alloc();
}
void *operator new(std::size_t cb, const std::nothrow_t &) noexcept {
    try { return ::operator new(cb); }
    catch (...) { return nullptr; }
}
void operator delete(void *lp, const std::nothrow_t &) noexcept { std::free(lp); }
void operator delete(void *lp) noexcept { std::free(lp); }
void operator delete(void *lp, std::size_t) noexcept { std::free(lp); }

// This named provider keeps SDL's foreign calling convention and delegates to
// the real release routine after observing the production cleanup operation.
extern "C" void SDLCALL StarsTest_SDL_DestroySurface(SDL_Surface *lp) {
    if (lp && fObserveDestroy) cDestroyed++;
    SDL_DestroySurface(lp);
}

#define CHECK(f) do { if (!(f)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
int main() {
    HICON hSource, hCopy;
    COMPATIMAGE *lpSource;
    SDL_Surface *lpPixels;
    BYTE *rgbAnd;

    CHECK(FCompatVideo());
    hSource = LoadIconA(GetModuleHandleA(NULL), "StarsIco");
    CHECK(hSource && (lpSource = LpCompatImage(hSource)) && lpSource->fShared);
    lpPixels = lpSource->lpXor.get(); rgbAnd = lpSource->rgbAnd.get();
    CHECK(lpPixels && rgbAnd);

    fObserveDestroy = true; fFailNew = true;
    hCopy = CopyIcon(hSource);
    CHECK(!hCopy && !fFailNew && cDestroyed == 1);
    CHECK(LpCompatImage(hSource) == lpSource && lpSource->lpXor.get() == lpPixels && lpSource->rgbAnd.get() == rgbAnd);
    hCopy = CopyIcon(hSource);
    CHECK(hCopy && hCopy != hSource && cDestroyed == 1);
    CHECK(DestroyIcon(hCopy) && cDestroyed == 2);
    CHECK(!DestroyIcon(hCopy) && cDestroyed == 2);
    CHECK(DestroyIcon(hSource) && LpCompatImage(hSource) == lpSource && cDestroyed == 2);
    StarsCompatShutdown();
    CHECK(cDestroyed == 3);
    std::puts("CopyIcon allocation failure, retry, exact cleanup, shared borrowing and stale handles passed");
    return 0;
}
