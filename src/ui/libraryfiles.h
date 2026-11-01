#ifndef STARS_LIBRARYFILES_H
#define STARS_LIBRARYFILES_H

class APPSESSION;
#include <cstdint>
std::int16_t FPathUnderLibrary(const char *pszRoot, const char *pszFile);
std::int16_t FDeleteLibraryGameRecentPaths(APPSESSION &app, const char *pszDirectory);
#endif
