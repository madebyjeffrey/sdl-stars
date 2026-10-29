#ifndef STARS_COMPAT_PLATFORM_H
#define STARS_COMPAT_PLATFORM_H
#include "fileio.h"
#ifdef _WIN32
#include <SDL3/SDL.h>
// SDL exposes the Windows environment as UTF-8, independently of the CRT code page.
#define getenv SDL_getenv
#endif
#endif
