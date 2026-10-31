// Regression trace, linked into native builds configured with
// -DSTARS_TEST_TRACE=ON. Typed module effects bind these linked C++ observers.
// They call the real operation once and, when the
// STARS_TRACE environment variable names a file, log one tagged line per call:
//
//   R turn player ai range result rva seed1 seed2   Random
//   P turn player ai planet result rva              PctPlanetCapacity
//
// rva is the caller's return address relative to the executable image base.
// seed1 and seed2 are the RNG state before the draw, so a stream offset can
// be replayed for any range.

#include <stdlib.h>
#ifdef _MSC_VER
#include <intrin.h>
#define StarsTestReturnAddress() _ReturnAddress()
#elif defined(__GNUC__) || defined(__clang__)
#define StarsTestReturnAddress() __builtin_return_address(0)
#else
#define StarsTestReturnAddress() NULL
#endif
#include "win.h"
#include "appstate.h"
#include "../../src/core/random_io.h"
#include "../../src/core/capacity_io.h"



static FILE   *vpfileTrace;
static int16_t vfTraceInit;

// TraceFile opens $STARS_TRACE on first use and returns it, or NULL when unset.
static FILE *TraceFile(void) {
    char *psz;

    if (vfTraceInit == 0) {
        vfTraceInit = 1;
        psz = getenv("STARS_TRACE");
        if (psz != NULL && *psz != 0) {
            vpfileTrace = fopen(psz, "w");
        }
    }
    return vpfileTrace;
}

// TraceCall logs one tagged event with the current turn and player context,
// followed by pszExtra when it is not empty.
static void TraceCall(APPSESSION &app, char chTag, int32_t lArg, int32_t lResult, void *pvReturn, const char *pszExtra) {
    FILE *pfile;

    pfile = TraceFile();
    if (pfile != NULL) {
        fprintf(pfile, "%c %u %d %d %ld %ld %llx%s\n", chTag, app.state.game.turn, app.state.idPlayer, app.state.fAi, (long)lArg, (long)lResult,
                (unsigned long long)((uintptr_t)pvReturn - (uintptr_t)win32::GetModuleHandleA(NULL)), pszExtra);
    }
}

// StarsTest_Random draws from the game RNG and logs the range, result, and prior seeds.
int16_t StarsTest_Random(APPSESSION &app, int16_t c) {
    char    szSeeds[32];
    int32_t lSeed1;
    int32_t lSeed2;
    int16_t r;

    lSeed1 = app.state.lRandSeed1;
    lSeed2 = app.state.lRandSeed2;
    r = Random(app, c);
    snprintf(szSeeds, sizeof(szSeeds), " %ld %ld", (long)lSeed1, (long)lSeed2);
    TraceCall(app, 'R', c, r, StarsTestReturnAddress(), szSeeds);
    return r;
}

// StarsTest_PctPlanetCapacity computes planet capacity and logs the planet ID and result.
int16_t StarsTest_PctPlanetCapacity(APPSESSION &app, PLANET *lppl) {
    int16_t pct;

    pct = PctPlanetCapacity(app, lppl);
    TraceCall(app, 'P', lppl->id, pct, StarsTestReturnAddress(), "");
    return pct;
}

static const RANDOMIO io_randomio = [] { RANDOMIO io; io.pfnRandom = StarsTest_Random; return io; }();
static RANDOMIOSCOPE scope_randomio(io_randomio);
static const CAPACITYIO io_capacityio = [] { CAPACITYIO io; io.pfnPctPlanetCapacity = StarsTest_PctPlanetCapacity; return io; }();
static CAPACITYIOSCOPE scope_capacityio(io_capacityio);
