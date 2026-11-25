#ifndef STARS_COMPAT_PLATFORM_H
#define STARS_COMPAT_PLATFORM_H
#include "fileio.h"
#include <cstdlib>
#ifdef _WIN32
#include <SDL3/SDL.h>
#endif

inline const char *PszCompatEnvironment(const char *pszName) {
#ifdef _WIN32
    // SDL exposes the Windows environment as UTF-8, independent of the CRT.
    return SDL_getenv(pszName);
#else
    return std::getenv(pszName);
#endif
}
#endif
