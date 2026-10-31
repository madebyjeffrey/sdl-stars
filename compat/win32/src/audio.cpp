/* Asynchronous alert tones; the SDL device supplies desktop audio routing. */
#include "internal.h"
#include "audio_io.h"
#include <math.h>

struct COMPATAUDIO {
    NATIVEOWNER<SDL_AudioStream, SDL_DestroyAudioStream> lpAlert;
    BOOL fAudioOwned;

    ~COMPATAUDIO() noexcept { Shutdown(); }
    void Shutdown() noexcept {
        lpAlert.reset();
        if (fAudioOwned) SDL_QuitSubSystem(SDL_INIT_AUDIO);
        fAudioOwned = FALSE;
    }
};
static COMPATAUDIO audio;

void CompatShutdownAudio(void) noexcept { audio.Shutdown(); }
BOOL WINAPI win32::MessageBeep(UINT type) {
    SDL_AudioSpec spec = {SDL_AUDIO_F32, 1, 22050};
    float rgSamples[3528];
    double hz, envelope;
    int i;
    const char *psz = SDL_getenv("STARS_SDL_ALERTS");
    if (!SDL_IsMainThread()) { win32::SetLastError(ERROR_NOT_SUPPORTED); return FALSE; }
    if (type != UINT32_MAX && type != MB_OK && type != MB_ICONHAND && type != MB_ICONQUESTION &&
        type != MB_ICONEXCLAMATION && type != MB_ICONASTERISK) { win32::SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
    if (psz && SDL_strcmp(psz, "off") == 0) return TRUE;
    if (!audio.lpAlert) {
        if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
            if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) { win32::SetLastError(ERROR_NOT_SUPPORTED); return FALSE; }
            audio.fAudioOwned = TRUE;
        }
        audio.lpAlert.reset(SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL));
        if (!audio.lpAlert) { CompatShutdownAudio(); win32::SetLastError(ERROR_NOT_SUPPORTED); return FALSE; }
    }
    for (i = 0; i < 3528; i++) {
        hz = type == MB_ICONHAND ? 220 : type == MB_ICONEXCLAMATION ? 440 : type == MB_ICONASTERISK ? 1047 : 880;
        if (type == MB_ICONQUESTION) hz = i < 1764 ? 660 : 880;
        envelope = (((1.0) < ((double)i / 110)) ? (1.0) : ((double)i / 110)) * (((1.0) < ((double)(3527 - i) / 220)) ? (1.0) : ((double)(3527 - i) / 220));
        rgSamples[i] = (float)(0.15 * envelope * sin(i * hz * (6.283185307179586 / 22050)));
    }
    // A new alert replaces the previous one instead of accumulating stale sounds.
    if (!SDL_ClearAudioStream(audio.lpAlert.get()) || !CompatAudioIO().pfnPutAudioStreamData(audio.lpAlert.get(), rgSamples, sizeof(rgSamples)) ||
        !SDL_FlushAudioStream(audio.lpAlert.get()) || !SDL_ResumeAudioStreamDevice(audio.lpAlert.get())) {
        CompatShutdownAudio(); win32::SetLastError(ERROR_NOT_SUPPORTED); return FALSE;
    }
    return TRUE;
}
