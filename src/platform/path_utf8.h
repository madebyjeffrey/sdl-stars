#ifndef STARS_PATH_UTF8_H
#define STARS_PATH_UTF8_H

#include <cstring>
#include <filesystem>
#include <string>

// C++20 distinguishes UTF-8 code units from the application's char bytes.
// The range constructor copies values, without aliasing char as char8_t.
inline std::filesystem::path PathFromUtf8(const char *psz) {
    return std::filesystem::path(std::u8string(psz, psz + std::strlen(psz)));
}

inline std::string SzFromUtf8(const std::u8string &sz) {
    return std::string(reinterpret_cast<const char *>(sz.data()), sz.size());
}

#endif
