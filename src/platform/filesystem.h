#ifndef STARS_FILESYSTEM_H
#define STARS_FILESYSTEM_H
#include <cstddef>
// Paths are strict UTF-8. Return 1 on success, 0 on any failure.
// Output buffers are caller-owned and empty on failure; paths are never truncated.
// Input paths and output buffers must not overlap. Create/Remove require a change.
// Returned paths use '/' separators on every platform.
// TempDir creates a unique directory atomically. Remove never removes recursively.
int FFilesystemTempDir(char *pszOut, std::size_t cbOut);
int FFilesystemCreateDirectory(const char *pszPath);
int FFilesystemCanonicalPath(const char *pszPath, char *pszOut, std::size_t cbOut);
int FFilesystemRemove(const char *pszPath);


#endif
