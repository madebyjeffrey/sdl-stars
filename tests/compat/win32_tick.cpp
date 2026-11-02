#include "../../compat/win32/src/time_io.h"
#include <windows.h>
#include <SDL3/SDL.h>

static Uint64 dwTicks;
extern "C" Uint64 SDLCALL StarsTest_SDL_GetTicks(void) { return dwTicks; }

int FTestTickCount(void) {
    dwTicks = 0;
    if (win32::GetTickCount() != 0)
        return 0;
    dwTicks = std::uint64_t{0xffffffff};
    if (win32::GetTickCount() != UINT32_MAX)
        return 0;
    dwTicks = std::uint64_t{0x100000000};
    if (win32::GetTickCount() != 0)
        return 0;
    dwTicks = std::uint64_t{0x123456789abcdef0};
    return win32::GetTickCount() == std::uint32_t{0x9abcdef0};
}

static const COMPATTIMEIO io_compattimeio = [] { COMPATTIMEIO io; io.pfnGetTicks = StarsTest_SDL_GetTicks; return io; }();
static COMPATTIMEIOSCOPE scope_compattimeio(io_compattimeio);
