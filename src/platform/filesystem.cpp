#include "filesystem.h"
extern "C" {
#include "encoding.h"
}
#include <cstring>
#include <filesystem>
#include <random>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

static bool FPathValid(const char *pszPath) {
    return pszPath && *pszPath && FUtf8Valid(pszPath, std::strlen(pszPath));
}

static bool FCopyPath(const fs::path &path, char *pszOut, size_t cbOut) {
    std::string sz = path.generic_u8string();
    if (sz.size() >= cbOut || !FUtf8Valid(sz.data(), sz.size()))
        return false;
    std::memcpy(pszOut, sz.c_str(), sz.size() + 1);
    return true;
}

int FFilesystemTempDir(char *pszOut, size_t cbOut) {
    if (!pszOut || cbOut == 0)
        return 0;
    *pszOut = 0;
    try {
        fs::path pathBase = fs::canonical(fs::temp_directory_path());
        std::random_device random;
        static const char rgchHex[] = "0123456789abcdef";
        for (int i = 0; i < 64; i++) {
            std::string szName = "stars-";
            std::error_code error;
            for (int j = 0; j < 32; j++)
                szName += rgchHex[random() & 15];
            fs::path path = pathBase / szName;
            // Check capacity before creating anything, so failure leaves no directory.
            if (!FCopyPath(path, pszOut, cbOut))
                return 0;
            if (fs::create_directory(path, error))
                return 1;
            *pszOut = 0;
            if (error && error != std::errc::file_exists)
                return 0;
        }
    } catch (...) {
        // Allocation, encoding and filesystem exceptions must not cross the C ABI.
    }
    *pszOut = 0;
    return 0;
}

int FFilesystemCreateDirectory(const char *pszPath) {
    if (!FPathValid(pszPath))
        return 0;
    try {
        std::error_code error;
        return fs::create_directory(fs::u8path(pszPath), error) && !error;
    } catch (...) {
        return 0;
    }
}

int FFilesystemCanonicalPath(const char *pszPath, char *pszOut, size_t cbOut) {
    if (!pszOut || cbOut == 0)
        return 0;
    *pszOut = 0;
    if (!FPathValid(pszPath))
        return 0;
    try {
        return FCopyPath(fs::canonical(fs::u8path(pszPath)), pszOut, cbOut);
    } catch (...) {
        return 0;
    }
}

int FFilesystemRemove(const char *pszPath) {
    if (!FPathValid(pszPath))
        return 0;
    try {
        std::error_code error;
        return fs::remove(fs::u8path(pszPath), error) && !error;
    } catch (...) {
        return 0;
    }
}
