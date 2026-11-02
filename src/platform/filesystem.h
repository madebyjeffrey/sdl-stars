#ifndef STARS_FILESYSTEM_H
#define STARS_FILESYSTEM_H
#include <optional>
#include <string>
#include <string_view>
// Paths are strict UTF-8. Create/Remove return true only when they make a change.
// Returned paths use '/' separators on every platform.
// TempDir creates a unique directory atomically. Remove never removes recursively.
std::optional<std::string> SzFilesystemTempDir();
bool FFilesystemCreateDirectory(std::string_view szPath);
std::optional<std::string> SzFilesystemCanonicalPath(std::string_view szPath);
bool FFilesystemRemove(std::string_view szPath);


#endif
