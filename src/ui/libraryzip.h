#ifndef STARS_LIBRARYZIP_H
#define STARS_LIBRARYZIP_H

#include <stdint.h>

int16_t FUnzipLibraryArchive(const char *pszArchive, const char *pszDirectory);
int16_t FZipLibraryDirectory(const char *pszDirectory, const char *pszArchive);

#endif
