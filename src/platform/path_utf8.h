#ifndef STARS_PATH_UTF8_H
#define STARS_PATH_UTF8_H

#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

// C++20 distinguishes UTF-8 code units from the application's char bytes.
// Copy values into char8_t storage without aliasing char as char8_t.
inline std::filesystem::path PathFromUtf8(std::string_view sz) {
    std::u8string szUtf8;
    unsigned char ch;
    std::size_t i;
    szUtf8.reserve(sz.size());
    for (i = 0; i < sz.size(); i++) {
        ch = static_cast<unsigned char>(sz[i]);
        szUtf8.push_back(static_cast<char8_t>(ch));
    }
    return std::filesystem::path(std::move(szUtf8));
}
inline std::filesystem::path PathFromUtf8(const char *psz) {
    return PathFromUtf8(std::string_view{psz, std::strlen(psz)});
}

inline std::string SzFromUtf8(const std::u8string &sz) {
    return std::string(reinterpret_cast<const char *>(sz.data()), sz.size());
}

#endif
