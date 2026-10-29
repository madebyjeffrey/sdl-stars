#include "path_utf8.h"
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

static bool FCopyPath(const fs::path &pth, char *pszOut, size_t cbOut) {
    std::string sz = SzFromUtf8(pth.generic_u8string());
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
        fs::path pthBase = fs::canonical(fs::temp_directory_path());
        std::random_device randDevice;
        static const char rgchHex[] = "0123456789abcdef";
        for (int iTry = 0; iTry < 64; iTry++) {
            std::string szName = "stars-";
            std::error_code ec;
            for (int iChar = 0; iChar < 32; iChar++)
                szName += rgchHex[randDevice() & 15];
            fs::path pthCandidate = pthBase / szName;
            // Check capacity before creating anything, so failure leaves no directory.
            if (!FCopyPath(pthCandidate, pszOut, cbOut))
                return 0;
            if (fs::create_directory(pthCandidate, ec))
                return 1;
            *pszOut = 0;
            if (ec && ec != std::errc::file_exists)
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
        std::error_code ec;
        return fs::create_directory(PathFromUtf8(pszPath), ec) && !ec;
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
        return FCopyPath(fs::canonical(PathFromUtf8(pszPath)), pszOut, cbOut);
    } catch (...) {
        return 0;
    }
}

int FFilesystemRemove(const char *pszPath) {
    if (!FPathValid(pszPath))
        return 0;
    try {
        std::error_code ec;
        return fs::remove(PathFromUtf8(pszPath), ec) && !ec;
    } catch (...) {
        return 0;
    }
}
