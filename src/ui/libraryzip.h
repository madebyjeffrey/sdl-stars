#ifndef STARS_LIBRARYZIP_H
#define STARS_LIBRARYZIP_H

#include "win.h"
#include <stdint.h>

enum { ZIP_IMPORT_INVALID = 0, ZIP_IMPORT_OK = 1, ZIP_IMPORT_UNSUPPORTED_NAME = 2 };

int16_t FUnzipLibraryArchive(const char *pszArchive, const char *pszDirectory);
int16_t FZipLibraryDirectory(APPSESSION &app, const char *pszDirectory, const char *pszArchive);

#endif
