#include "path_utf8.h"
#include "filesystem.h"
#include "encoding.h"

#include <filesystem>
#include <random>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

static bool FPathValid(std::string_view szPath) {
    return !szPath.empty() && FUtf8Valid(szPath);
}

static std::optional<std::string> SzPath(const fs::path &pth) {
    std::string sz = SzFromUtf8(pth.generic_u8string());
    if (!FUtf8Valid(sz))
        return std::nullopt;
    return sz;
}

std::optional<std::string> SzFilesystemTempDir() {
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
            std::optional<std::string> szCandidate = SzPath(pthCandidate);
            if (!szCandidate)
                return std::nullopt;
            if (fs::create_directory(pthCandidate, ec))
                return szCandidate;
            if (ec && ec != std::errc::file_exists)
                return std::nullopt;
        }
    } catch (...) {
        // Allocation, encoding and filesystem exceptions must preserve the service failure contract.
    }
    return std::nullopt;
}

bool FFilesystemCreateDirectory(std::string_view szPath) {
    if (!FPathValid(szPath))
        return false;
    try {
        std::error_code ec;
        return fs::create_directory(PathFromUtf8(szPath), ec) && !ec;
    } catch (...) {
        return false;
    }
}

std::optional<std::string> SzFilesystemCanonicalPath(std::string_view szPath) {
    if (!FPathValid(szPath))
        return std::nullopt;
    try {
        return SzPath(fs::canonical(PathFromUtf8(szPath)));
    } catch (...) {
        return std::nullopt;
    }
}

bool FFilesystemRemove(std::string_view szPath) {
    if (!FPathValid(szPath))
        return false;
    try {
        std::error_code ec;
        return fs::remove(PathFromUtf8(szPath), ec) && !ec;
    } catch (...) {
        return false;
    }
}
