#ifndef STARS_LIBRARYZIP_H
#define STARS_LIBRARYZIP_H

#include "win.h"
#include <cstdint>
#include <string_view>

enum { ZIP_IMPORT_INVALID = 0, ZIP_IMPORT_OK = 1, ZIP_IMPORT_UNSUPPORTED_NAME = 2 };

std::int16_t FUnzipLibraryArchive(std::string_view pszArchive, std::string_view pszDirectory);
std::int16_t FZipLibraryDirectory(APPSESSION &app, std::string_view pszDirectory, std::string_view pszArchive);

#endif
