#ifndef STARS_LIBRARYFILES_H
#define STARS_LIBRARYFILES_H
#include <stdint.h>
int16_t FPathUnderLibrary(const char *pszRoot, const char *pszFile);
int16_t FDeleteLibraryGameRecentPaths(const char *pszDirectory);
#endif
