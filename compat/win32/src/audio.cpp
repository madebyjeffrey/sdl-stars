/* Asynchronous alert tones; the SDL device supplies desktop audio routing. */
#include "internal.h"
#include "audio_io.h"
#include <cmath>
#include <limits>

struct COMPATAUDIO {
    NATIVEOWNER<SDL_AudioStream, SDL_DestroyAudioStream> lpAlert;
    BOOL fAudioOwned;

    ~COMPATAUDIO() noexcept { Shutdown(); }
    void Shutdown() noexcept {
        lpAlert.reset();
        if (fAudioOwned) SDL_QuitSubSystem(SDL_INIT_AUDIO);
        fAudioOwned = 0;
    }
};
static COMPATAUDIO audio;

void CompatShutdownAudio(void) noexcept { audio.Shutdown(); }
BOOL win32::MessageBeep(UINT type) {
    SDL_AudioSpec spec = {SDL_AUDIO_F32, 1, 22050};
    float rgSamples[3528];
    double hz, envelope;
    int i;
    const char *psz = SDL_getenv("STARS_SDL_ALERTS");
    if (!SDL_IsMainThread()) { win32::SetLastError(win32::ERROR_NOT_SUPPORTED); return 0; }
    if (type != std::numeric_limits<std::uint32_t>::max() && type != win32::MB_OK && type != win32::MB_ICONHAND && type != win32::MB_ICONQUESTION &&
        type != win32::MB_ICONEXCLAMATION && type != win32::MB_ICONASTERISK) { win32::SetLastError(win32::ERROR_INVALID_PARAMETER); return 0; }
    if (psz && SDL_strcmp(psz, "off") == 0) return 1;
    if (!audio.lpAlert) {
        if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
            if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) { win32::SetLastError(win32::ERROR_NOT_SUPPORTED); return 0; }
            audio.fAudioOwned = 1;
        }
        audio.lpAlert.reset(SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr));
        if (!audio.lpAlert) { CompatShutdownAudio(); win32::SetLastError(win32::ERROR_NOT_SUPPORTED); return 0; }
    }
    for (i = 0; i < 3528; i++) {
        hz = type == win32::MB_ICONHAND ? 220 : type == win32::MB_ICONEXCLAMATION ? 440 : type == win32::MB_ICONASTERISK ? 1047 : 880;
        if (type == win32::MB_ICONQUESTION) hz = i < 1764 ? 660 : 880;
        envelope = (((1.0) < (static_cast<double>(i) / 110)) ? (1.0) : (static_cast<double>(i) / 110)) * (((1.0) < (static_cast<double>((3527 - i)) / 220)) ? (1.0) : (static_cast<double>((3527 - i)) / 220));
        rgSamples[i] = static_cast<float>((0.15 * envelope * std::sin(i * hz * (6.283185307179586 / 22050))));
    }
    // A new alert replaces the previous one instead of accumulating stale sounds.
    if (!SDL_ClearAudioStream(audio.lpAlert.get()) || !CompatAudioIO().pfnPutAudioStreamData(audio.lpAlert.get(), rgSamples, sizeof(rgSamples)) ||
        !SDL_FlushAudioStream(audio.lpAlert.get()) || !SDL_ResumeAudioStreamDevice(audio.lpAlert.get())) {
        CompatShutdownAudio(); win32::SetLastError(win32::ERROR_NOT_SUPPORTED); return 0;
    }
    return 1;
}
