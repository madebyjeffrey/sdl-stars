/* Asynchronous alert tones; the SDL device supplies desktop audio routing. */
#include "internal.h"
#include <math.h>

static SDL_AudioStream *lpAlert;
static BOOL fAudioOwned;

void CompatShutdownAudio(void) {
    SDL_DestroyAudioStream(lpAlert); lpAlert = NULL;
    if (fAudioOwned) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    fAudioOwned = FALSE;
}
BOOL WINAPI MessageBeep(UINT type) {
    SDL_AudioSpec spec = {SDL_AUDIO_F32, 1, 22050};
    float rgSamples[3528];
    double hz, envelope;
    int i;
    const char *psz = SDL_getenv("STARS_SDL_ALERTS");
    if (!SDL_IsMainThread()) { SetLastError(ERROR_NOT_SUPPORTED); return FALSE; }
    if (type != UINT32_MAX && type != MB_OK && type != MB_ICONHAND && type != MB_ICONQUESTION &&
        type != MB_ICONEXCLAMATION && type != MB_ICONASTERISK) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
    if (psz && SDL_strcmp(psz, "off") == 0) return TRUE;
    if (!lpAlert) {
        if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
            if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) { SetLastError(ERROR_NOT_SUPPORTED); return FALSE; }
            fAudioOwned = TRUE;
        }
        lpAlert = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
        if (!lpAlert) { CompatShutdownAudio(); SetLastError(ERROR_NOT_SUPPORTED); return FALSE; }
    }
    for (i = 0; i < 3528; i++) {
        hz = type == MB_ICONHAND ? 220 : type == MB_ICONEXCLAMATION ? 440 : type == MB_ICONASTERISK ? 1047 : 880;
        if (type == MB_ICONQUESTION) hz = i < 1764 ? 660 : 880;
        envelope = min(1.0, (double)i / 110) * min(1.0, (double)(3527 - i) / 220);
        rgSamples[i] = (float)(0.15 * envelope * sin(i * hz * (6.283185307179586 / 22050)));
    }
    // A new alert replaces the previous one instead of accumulating stale sounds.
    if (!SDL_ClearAudioStream(lpAlert) || !SDL_PutAudioStreamData(lpAlert, rgSamples, sizeof(rgSamples)) ||
        !SDL_FlushAudioStream(lpAlert) || !SDL_ResumeAudioStreamDevice(lpAlert)) {
        CompatShutdownAudio(); SetLastError(ERROR_NOT_SUPPORTED); return FALSE;
    }
    return TRUE;
}
