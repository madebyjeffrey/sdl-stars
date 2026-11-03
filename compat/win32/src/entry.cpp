/* SDL entry point; the game's WinMain and its command parser stay shared. */
#include <windows.h>
#include <SDL3/SDL.h>
#include <stars_compat.h>
#include <algorithm>
#include <cstdio>
#include <string_view>
#include "version.h"
#ifdef _WIN32
#include "native_process.h"
#include <vector>
#endif

namespace ui { int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow); }

static int ICompatMain(int argc, char **argv) {
    char szCmdLine[1024];
    std::size_t cch = 0, cchArg;
    std::string_view szArg;
    int i, nResult, cArgs = 0;
    BOOL fHeadless = 0;
    HINSTANCE hInstance;

    if (argc == 2 && std::string_view{argv[1]} == "--version") {
        std::printf("stars-sdl %s\n", STARS_VERSION_DISPLAY);
        return 0;
    }
    szCmdLine[0] = 0;
    for (i = 1; i < argc; i++) {
        szArg = argv[i];
        if (szArg == "--headless") { fHeadless = 1; continue; }
        cchArg = szArg.size();
        // ParseCmdLine has no quoting, and its filename destination is 80 bytes.
        if (cchArg >= 80 || szArg.find_first_of(" \t\r\n") != std::string_view::npos) {
            std::fprintf(stderr, "stars-sdl: arguments must be shorter than 80 bytes and contain no whitespace\n");
            return 2;
        }
        if (cch + cchArg + (cArgs != 0 ? 1u : 0u) >= sizeof(szCmdLine)) {
            std::fprintf(stderr, "stars-sdl: command line too long\n");
            return 2;
        }
        if (cArgs != 0) szCmdLine[cch++] = ' ';
        cArgs++;
        std::copy(szArg.begin(), szArg.end(), szCmdLine + cch);
        cch += cchArg;
        szCmdLine[cch] = 0;
    }
    if (fHeadless) {
        // Select a real, offscreen SDL renderer before any game initialization.
        // Consume the option here: the original parser never sees native flags.
        if (!SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "SDL_VIDEODRIVER", "dummy", true) ||
            !SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "SDL_AUDIODRIVER", "dummy", true) ||
            !SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "STARS_SDL_RENDERER", "software", true) ||
            !SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "STARS_FILE_DIALOG_BACKEND", "compat", true)) {
            std::fprintf(stderr, "stars-sdl: cannot configure headless SDL: %s\n", SDL_GetError());
            return 1;
        }
    }
    hInstance = win32::GetModuleHandleA(nullptr);
    if (!hInstance) {
        std::fprintf(stderr, "stars-sdl: cannot initialize resource module: %s\n", SDL_GetError());
        return 1;
    }
    if (!SDL_SetAppMetadata("Stars!", STARS_VERSION_STRING, "org.starsgame.Stars")) {
        std::fprintf(stderr, "stars-sdl: cannot set application identity: %s\n", SDL_GetError());
        StarsCompatProcessExit(); SDL_Quit();
        return 1;
    }
    nResult = ui::WinMain(hInstance, nullptr, szCmdLine, win32::SW_SHOWNORMAL);
    StarsCompatProcessExit();
    SDL_Quit();
    return nResult;
}

int main(int argc, char **argv) try {
#ifdef _WIN32
    auto rgsz = RgNativeArguments();
    std::vector<char *> rgpsz;
    static_cast<void>(argc);
    static_cast<void>(argv);
    if (!rgsz) return 2;
    rgpsz.reserve(rgsz->size());
    for (auto &sz : *rgsz) rgpsz.push_back(sz.data());
    return ICompatMain(static_cast<int>(rgpsz.size()), rgpsz.data());
#else
    return ICompatMain(argc, argv);
#endif
} catch (...) {
    // The process/SDL entry boundary must not propagate a C++ failure.
    StarsCompatProcessExit();
    SDL_Quit();
    std::fputs("stars-sdl: unhandled application failure\n", stderr);
    return 1;
}
