#ifndef STARS_LIBRARYFILES_H
#define STARS_LIBRARYFILES_H

class APPSESSION;
#include <cstdint>
#include <string_view>
std::int16_t FPathUnderLibrary(std::string_view szRoot, std::string_view szFile);
std::int16_t FDeleteLibraryGameRecentPaths(APPSESSION &app, std::string_view szDirectory);
#endif
