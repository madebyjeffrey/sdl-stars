/* Exercise allocation failure after CopyIcon has allocated its pixel planes. */
#include "internal.h"
#include "../../compat/win32/src/render_io.h"

#include <stars_compat.h>
#include <cstdlib>
#include <new>
#include <cstdio>

static bool fFailNew, fFailArray, fObserveDestroy;
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
void *operator new[](std::size_t cb) {
    if (fFailArray) { fFailArray = false; throw std::bad_alloc(); }
    if (void *lp = std::malloc(cb ? cb : 1)) return lp;
    throw std::bad_alloc();
}
void *operator new[](std::size_t cb, const std::nothrow_t &) noexcept {
    try { return ::operator new[](cb); }
    catch (...) { return nullptr; }
}
void operator delete[](void *lp, const std::nothrow_t &) noexcept { std::free(lp); }
void operator delete[](void *lp) noexcept { std::free(lp); }
void operator delete[](void *lp, std::size_t) noexcept { std::free(lp); }


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
    HMENU hmenu, hsub, hother;
    char sz[32];
    LOGFONTA lf = {};
    COMPATRENDERIO io_compatrenderio;
    io_compatrenderio.pfnDestroySurface = StarsTest_SDL_DestroySurface;
    COMPATRENDERIOSCOPE scope_compatrenderio(io_compatrenderio);

    NATIVEOWNER<COMPATFONT, FreeCompatFont> lpfont;

    CHECK(FCompatVideo());
    hSource = win32::LoadIconA(win32::GetModuleHandleA(NULL), "StarsIco");
    CHECK(hSource && (lpSource = LpCompatImage(hSource)) && lpSource->fShared);
    lpPixels = lpSource->lpXor.get(); rgbAnd = lpSource->rgbAnd.get();
    CHECK(lpPixels && rgbAnd);

    fObserveDestroy = true; fFailNew = true;
    hCopy = win32::CopyIcon(hSource);
    CHECK(!hCopy && !fFailNew && cDestroyed == 1);
    CHECK(LpCompatImage(hSource) == lpSource && lpSource->lpXor.get() == lpPixels && lpSource->rgbAnd.get() == rgbAnd);
    hCopy = win32::CopyIcon(hSource);
    CHECK(hCopy && hCopy != hSource && cDestroyed == 1);
    CHECK(win32::DestroyIcon(hCopy) && cDestroyed == 2);
    CHECK(!win32::DestroyIcon(hCopy) && cDestroyed == 2);
    CHECK(win32::DestroyIcon(hSource) && LpCompatImage(hSource) == lpSource && cDestroyed == 2);
    hmenu = win32::CreateMenu(); hsub = win32::CreatePopupMenu(); hother = win32::CreateMenu();
    CHECK(hmenu && hsub && hother && win32::AppendMenuA(hmenu, win32::MF_STRING, 41, "Retained"));
    fFailArray = true;
    CHECK(!win32::InsertMenuA(hmenu, 0, win32::MF_BYPOSITION | win32::MF_POPUP, (UINT_PTR)hsub, "Not installed") && !fFailArray);
    CHECK(win32::GetMenuItemCount(hmenu) == 1 && win32::GetMenuItemID(hmenu, 0) == 41);
    CHECK(win32::GetMenuStringA(hmenu, 41, sz, sizeof(sz), win32::MF_BYCOMMAND) == 8 && !SDL_strcmp(sz, "Retained"));
    CHECK(win32::AppendMenuA(hother, win32::MF_POPUP, (UINT_PTR)hsub, "Still unattached"));
    CHECK(win32::RemoveMenu(hother, 0, win32::MF_BYPOSITION) && win32::IsMenu(hsub));
    CHECK(win32::InsertMenuA(hmenu, 0, win32::MF_BYPOSITION | win32::MF_POPUP, (UINT_PTR)hsub, "Retry"));
    CHECK(win32::GetSubMenu(hmenu, 0) == hsub && win32::GetMenuItemID(hmenu, 1) == 41);
    CHECK(win32::DeleteMenu(hmenu, 0, win32::MF_BYPOSITION) && !win32::IsMenu(hsub));
    CHECK(win32::DestroyMenu(hmenu) && win32::DestroyMenu(hother));
    // FreeType has already allocated its library when the shared owner fails.
    CompatShutdownFonts(); lf.lfHeight = -12;
    SDL_strlcpy(lf.lfFaceName, "Arial", sizeof(lf.lfFaceName));
    fFailNew = true; lpfont.reset(LpNewCompatFont(&lf));
    CHECK(!lpfont && !fFailNew);
    lpfont.reset(LpNewCompatFont(&lf)); CHECK(lpfont);
    lpfont.reset();
    StarsCompatShutdown();
    CHECK(cDestroyed == 3);
    std::puts("Image and menu allocation failure, retry, cleanup, borrowing and stale handles passed");
    return 0;
}
