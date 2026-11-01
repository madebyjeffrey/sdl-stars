/* Own only audio initialized by the wrapper; exercise the real SDL stream. */
#include "internal.h"
#include "../../compat/win32/src/audio_io.h"

#include <cstdio>
#include <cstring>
#include <cmath>

static int cQueues, cErrors;
extern "C" bool SDLCALL StarsTest_SDL_PutAudioStreamData(SDL_AudioStream *lp, const void *lpv, int cb) {
    SDL_AudioSpec spec;
    const float *rg = static_cast<const float *>(lpv);
    int i;
    bool fAudible = false;
    if (!SDL_GetAudioStreamFormat(lp, &spec, NULL) || spec.format != SDL_AUDIO_F32 || spec.channels != 1 || spec.freq <= 0) cErrors++;
    if (!lpv || cb <= 0 || cb % static_cast<int>(sizeof(float)) != 0) cErrors++;
    else for (i = 0; i < cb / static_cast<int>(sizeof(float)); i++) {
        if (!std::isfinite(rg[i]) || std::fabs(rg[i]) > 1.0f) cErrors++;
        if (std::fabs(rg[i]) > 0.01f) fAudible = true;
    }
    if (!fAudible) cErrors++;
    cQueues++;
    return SDL_PutAudioStreamData(lp, lpv, cb);
}
#define CHECK(f) do { if (!(f)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #f); return 1; } } while (0)
int main(int argc, char **argv) {
    const char *pszMode = argc == 2 ? argv[1] : "owned";
    COMPATAUDIOIO io_compataudioio;
    io_compataudioio.pfnPutAudioStreamData = StarsTest_SDL_PutAudioStreamData;
    COMPATAUDIOIOSCOPE scope_compataudioio(io_compataudioio);

    CHECK(argc <= 2);
    if (!std::strcmp(pszMode, "failure")) {
        CHECK(!win32::MessageBeep(win32::MB_OK) && win32::GetLastError() == win32::ERROR_NOT_SUPPORTED && cQueues == 0);
        CHECK(!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO));
    } else if (!std::strcmp(pszMode, "muted")) {
        CHECK(win32::MessageBeep(win32::MB_OK) && cQueues == 0);
        CHECK(!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO));
    } else if (!std::strcmp(pszMode, "external")) {
        CHECK(SDL_InitSubSystem(SDL_INIT_AUDIO) && win32::MessageBeep(win32::MB_OK) && cQueues == 1);
        CompatShutdownAudio(); CHECK(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    } else {
        CHECK(!std::strcmp(pszMode, "owned"));
        CHECK(win32::MessageBeep(win32::MB_OK) && win32::MessageBeep(win32::MB_ICONQUESTION) && cQueues == 2);
        CompatShutdownAudio(); CHECK(!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO));
        CHECK(win32::MessageBeep(win32::MB_ICONHAND) && cQueues == 3);
    }
    CompatShutdownAudio(); CompatShutdownAudio();
    CHECK(!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) && cErrors == 0);
    std::puts("Audio stream cleanup, ownership, unavailable/muted paths and restart passed");
    return 0;
}
