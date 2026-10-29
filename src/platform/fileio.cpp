#include "fileio.h"
extern "C" {
#include "encoding.h"
}
#include <array>
#include <cerrno>
#include <cctype>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <locale>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <system_error>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys/stat.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

static bool FPath(const char *psz) {
    if (!psz) { errno = EINVAL; return false; }
    if (!FUtf8Valid(psz, std::strlen(psz))) { errno = EILSEQ; return false; }
    return true;
}
static void SetError(const std::error_code &ec) {
    errno = ec.default_error_condition().value();
    if (errno == 0) errno = EIO;
}
static int CloseDescriptor(int fd) noexcept {
#ifdef _WIN32
    return _close(fd);
#else
    return close(fd);
#endif
}

struct NATIVEFILE {
    std::fstream stm;
    fs::path pth;
    int fd = -1;
    unsigned grfAccess = 0;
    bool fEof = false, fError = false, fDelete = false, fAppend = false;
    int64_t ib = 0;
    int mdLast = 0;
    uint64_t cErrors = 0;

    ~NATIVEFILE() noexcept {
        try { Close(); } catch (...) {}
    }
    int Fail(int err) noexcept { errno = err; fError = true; ++cErrors; return -1; }
    int Flush() {
        if (fd >= 0) return 0;
        if (!stm.is_open()) return Fail(EBADF);
        if (!(grfAccess & 2)) return 0;
        stm.clear(); stm.flush();
        return stm ? 0 : Fail(errno ? errno : EIO);
    }
    int Close() {
        int ret = 0;
        if (fd >= 0) { int fdClose = fd; fd = -1; ret = CloseDescriptor(fdClose); }
        if (stm.is_open()) {
            if (Flush() != 0) ret = -1;
            stm.clear(); stm.close();
            if (stm.fail()) ret = -1;
        }
        if (fDelete) { std::error_code ec; fs::remove(pth, ec); fDelete = false; }
        return ret;
    }
    bool Position(int md) {
        stm.clear();
        if (mdLast != 0 && mdLast != md) {
            if (md == 1) stm.seekg(ib); else stm.seekp(ib);
            if (!stm) { Fail(errno ? errno : EIO); return false; }
        }
        mdLast = md;
        return true;
    }
    int64_t Read(void *rgb, size_t cb) {
        int64_t got;
        uint64_t errorsBefore = cErrors;
        if (!(grfAccess & 1)) return Fail(EBADF);
        if (cb > static_cast<size_t>(std::numeric_limits<std::streamsize>::max())) return Fail(EINVAL);
        if (fd >= 0) {
#ifdef _WIN32
            got = _read(fd, rgb, static_cast<unsigned>((std::min)(cb, static_cast<size_t>(INT_MAX))));
#else
            got = read(fd, rgb, cb);
#endif
            if (got < 0) return Fail(errno);
        } else {
            if (!Position(1)) return -1;
            errno = 0;
            stm.read(static_cast<char *>(rgb), static_cast<std::streamsize>(cb));
            got = stm.gcount();
            if (stm.bad() || (static_cast<uint64_t>(got) < cb && errno != 0)) {
                Fail(errno ? errno : EIO); if (got == 0) return -1;
            }
        }
        ib += got;
        if (cErrors == errorsBefore && (fd >= 0 ? got == 0 && cb != 0 : stm.eof())) fEof = true;
        return got;
    }
    int64_t Write(const void *rgb, size_t cb) {
        int64_t put;
        if (!(grfAccess & 2)) return Fail(EBADF);
        if (cb > static_cast<size_t>(std::numeric_limits<std::streamsize>::max())) return Fail(EINVAL);
        if (fd >= 0) {
#ifdef _WIN32
            put = _write(fd, rgb, static_cast<unsigned>((std::min)(cb, static_cast<size_t>(INT_MAX))));
#else
            put = write(fd, rgb, cb);
#endif
            if (put < 0) return Fail(errno);
        } else {
            if (!Position(2)) return -1;
            errno = 0;
            put = stm.rdbuf()->sputn(static_cast<const char *>(rgb), static_cast<std::streamsize>(cb));
            if (static_cast<uint64_t>(put) < cb) Fail(errno ? errno : EIO);
        }
        ib += put;
        if (fAppend) ib = static_cast<int64_t>(stm.tellp());
        return put;
    }
    int64_t Seek(int64_t off, int origin) {
        int64_t pos;
        if (origin < 0 || origin > 2) return Fail(EINVAL);
        if (fd >= 0) {
#ifdef _WIN32
            pos = _lseeki64(fd, off, origin);
#else
            pos = lseek(fd, off, origin);
#endif
            if (pos < 0) return Fail(errno);
        } else {
            auto dir = origin == 0 ? std::ios::beg : origin == 1 ? std::ios::cur : std::ios::end;
            if (origin == 1) {
                if ((off > 0 && ib > INT64_MAX - off) || (off < 0 && ib < INT64_MIN - off)) return Fail(EOVERFLOW);
                off += ib; dir = std::ios::beg;
            }
            stm.clear();
            if (grfAccess & 1) { stm.seekg(off, dir); pos = static_cast<int64_t>(stm.tellg()); }
            else { stm.seekp(off, dir); pos = static_cast<int64_t>(stm.tellp()); }
            if (!stm || pos < 0) return Fail(errno ? errno : EINVAL);
        }
        ib = pos; mdLast = 0; fEof = false;
        return pos;
    }
    int64_t Size() {
        if (fd >= 0) {
#ifdef _WIN32
            return _filelengthi64(fd);
#else
            struct stat st;
            return fstat(fd, &st) == 0 ? static_cast<int64_t>(st.st_size) : -1;
#endif
        }
        int64_t saved = ib;
        bool eofSaved = fEof;
        if (Flush() != 0) return -1;
        int64_t cb = Seek(0, 2);
        if (cb < 0 || Seek(saved, 0) < 0) return -1;
        fEof = eofSaved;
        return cb;
    }
};

// All allocation/encoding/library failures are translated before the C boundary.
template <class R, class Fn> static R FileCall(NATIVEFILE *pf, R failure, Fn fn) noexcept {
    try { return fn(); }
    catch (const std::bad_alloc &) { errno = ENOMEM; }
    catch (const std::range_error &) { errno = EILSEQ; }
    catch (const fs::filesystem_error &err) { SetError(err.code()); }
    catch (...) { errno = EIO; }
    if (pf) pf->Fail(errno);
    return failure;
}

NATIVEFILE *PfNativeOpenFlags(const char *psz, unsigned access, unsigned share,
                            int create, int truncate, int exclusive, int native) {
    if (!FPath(psz)) return nullptr;
    if (access == 0 || access > 3) { errno = EINVAL; return nullptr; }
    return FileCall<NATIVEFILE *>(nullptr, nullptr, [&]() -> NATIVEFILE * {
        auto pf = std::make_unique<NATIVEFILE>();
        pf->pth = fs::u8path(psz); pf->grfAccess = access;
#ifdef _WIN32
        native = native || (share & 0x70) != 0;
#endif
        // C++17 has no exclusive-create mode, live descriptor identity, sharing
        // flags, write-only nontruncating mode or handle-based fsync/truncation.
        native = native || exclusive || (access == 2 && !truncate) || (create && !truncate) || (truncate && !create);
        if (native) {
#ifdef _WIN32
            int flags = access == 1 ? _O_RDONLY : access == 2 ? _O_WRONLY : _O_RDWR;
            flags |= _O_BINARY | _O_NOINHERIT;
            if (create) flags |= _O_CREAT;
            if (truncate) flags |= _O_TRUNC;
            if (exclusive) flags |= _O_EXCL;
            int sharing = (share & 0x70) == 0x10 ? _SH_DENYRW : (share & 0x70) == 0x20 ? _SH_DENYWR :
                          (share & 0x70) == 0x30 ? _SH_DENYRD : _SH_DENYNO;
            int err = _wsopen_s(&pf->fd, pf->pth.c_str(), flags, sharing, _S_IREAD | _S_IWRITE);
            if (err != 0) { errno = err; return nullptr; }
#else
            (void)share;
            int flags = access == 1 ? O_RDONLY : access == 2 ? O_WRONLY : O_RDWR;
            flags |= O_CLOEXEC;
            if (create) flags |= O_CREAT;
            if (truncate) flags |= O_TRUNC;
            if (exclusive) flags |= O_EXCL;
            pf->fd = open(psz, flags, 0666);
            if (pf->fd < 0) return nullptr;
#endif
        } else {
            auto md = std::ios::binary;
            if (access & 1) md |= std::ios::in;
            if (access & 2) md |= std::ios::out;
            if (truncate) md |= std::ios::trunc;
            pf->stm.imbue(std::locale::classic());
            errno = 0;
            pf->stm.open(pf->pth, md);
            if (!pf->stm.is_open()) { if (errno == 0) errno = EIO; return nullptr; }
        }
        return pf.release();
    });
}
NATIVEFILE *PfNativeOpen(const char *psz, const char *mode) {
    if (!mode || !*mode) { errno = EINVAL; return nullptr; }
    unsigned access = *mode == 'r' ? 1 : *mode == 'w' || *mode == 'a' ? 2 : 0;
    if (access == 0) { errno = EINVAL; return nullptr; }
    if (std::strchr(mode, '+')) access = 3;
    if (*mode == 'a') {
        if (!FPath(psz)) return nullptr;
        return FileCall<NATIVEFILE *>(nullptr, nullptr, [&]() -> NATIVEFILE * {
            auto pf = std::make_unique<NATIVEFILE>();
            pf->pth = fs::u8path(psz); pf->grfAccess = access; pf->fAppend = true;
            auto md = std::ios::binary | std::ios::out | std::ios::app;
            if (access & 1) md |= std::ios::in;
            pf->stm.imbue(std::locale::classic());
            errno = 0; pf->stm.open(pf->pth, md);
            if (!pf->stm.is_open()) { if (!errno) errno = EIO; return nullptr; }
            if (pf->Seek(0, 2) < 0) return nullptr;
            return pf.release();
        });
    }
    NATIVEFILE *pf = PfNativeOpenFlags(psz, access, 0, *mode != 'r', *mode == 'w', std::strchr(mode, 'x') != nullptr, 0);
    return pf;
}
int ICloseNativeFile(NATIVEFILE *pf) {
    if (!pf) { errno = EBADF; return -1; }
    int ret = FileCall(pf, -1, [&] { return pf->Close(); });
    delete pf;
    return ret;
}
int64_t CbReadNativeFile(NATIVEFILE *pf, void *rgb, size_t cb) {
    if (!pf || (!rgb && cb != 0)) { errno = EBADF; return -1; }
    return FileCall<int64_t>(pf, -1, [&] { return pf->Read(rgb, cb); });
}
int64_t CbWriteNativeFile(NATIVEFILE *pf, const void *rgb, size_t cb) {
    if (!pf || (!rgb && cb != 0)) { errno = EBADF; return -1; }
    return FileCall<int64_t>(pf, -1, [&] { return pf->Write(rgb, cb); });
}
size_t CReadNative(void *rgb, size_t cb, size_t c, NATIVEFILE *pf) {
    if (!pf || cb == 0 || c == 0) return 0;
    if (c > SIZE_MAX / cb) { pf->Fail(EOVERFLOW); return 0; }
    if (pf->fEof) return 0;
    size_t total = 0, requested = cb * c;
    while (total < requested && !pf->fEof) {
        uint64_t errorsBefore = pf->cErrors;
        int64_t got = CbReadNativeFile(pf, static_cast<char *>(rgb) + total, requested - total);
        if (got <= 0) break;
        total += static_cast<size_t>(got);
        if (pf->cErrors != errorsBefore) break;
    }
    return total / cb;
}
size_t CWriteNative(const void *rgb, size_t cb, size_t c, NATIVEFILE *pf) {
    if (!pf || cb == 0 || c == 0) return 0;
    if (c > SIZE_MAX / cb) { pf->Fail(EOVERFLOW); return 0; }
    size_t total = 0, requested = cb * c;
    while (total < requested) {
        uint64_t errorsBefore = pf->cErrors;
        int64_t put = CbWriteNativeFile(pf, static_cast<const char *>(rgb) + total, requested - total);
        if (put <= 0) break;
        total += static_cast<size_t>(put);
        if (pf->cErrors != errorsBefore) break;
    }
    return total / cb;
}
int64_t LSeekNativeFile(NATIVEFILE *pf, int64_t ib, int origin) {
    if (!pf) { errno = EBADF; return -1; }
    return FileCall<int64_t>(pf, -1, [&] { return pf->Seek(ib, origin); });
}
int ISeekNativeFile(NATIVEFILE *pf, int64_t ib, int origin) { return LSeekNativeFile(pf, ib, origin) < 0 ? -1 : 0; }
int64_t LNativePosition(NATIVEFILE *pf) { if (!pf) { errno = EBADF; return -1; } return pf->ib; }
int64_t CbNativeFile(NATIVEFILE *pf) {
    if (!pf) { errno = EBADF; return -1; }
    return FileCall<int64_t>(pf, -1, [&] { return pf->Size(); });
}
int ITruncateNativeFile(NATIVEFILE *pf, int64_t cb) {
    if (!pf || !(pf->grfAccess & 2) || cb < 0) { errno = EBADF; return -1; }
    // This operation must address the open file, even if its pathname changed.
    if (pf->fd < 0) return pf->Fail(ENOTSUP);
#ifdef _WIN32
    int err = _chsize_s(pf->fd, cb);
    if (err != 0) { errno = err; return pf->Fail(err); }
    return 0;
#else
    return ftruncate(pf->fd, cb);
#endif
}
int IFlushNativeFile(NATIVEFILE *pf) {
    if (!pf) { errno = EBADF; return -1; }
    return FileCall(pf, -1, [&] { return pf->Flush(); });
}
int ISyncNativeFile(NATIVEFILE *pf) {
    if (!pf || pf->fd < 0) { errno = ENOTSUP; return -1; }
    if (IFlushNativeFile(pf) != 0) return -1;
#ifdef _WIN32
    return _commit(pf->fd);
#else
    return fsync(pf->fd);
#endif
}
int FNativeStreamError(NATIVEFILE *pf) { return !pf || pf->fError; }
int FNativeStreamEof(NATIVEFILE *pf) { return pf && pf->fEof; }
int ChGetNativeFile(NATIVEFILE *pf) { unsigned char ch; return CReadNative(&ch, 1, 1, pf) == 1 ? ch : EOF; }
int ChPutNativeFile(int ch, NATIVEFILE *pf) { unsigned char byte = static_cast<unsigned char>(ch); return CWriteNative(&byte, 1, 1, pf) == 1 ? byte : EOF; }
char *PszGetNativeLine(char *psz, int cb, NATIVEFILE *pf) {
    if (!psz || cb <= 0 || !pf) { errno = EINVAL; return nullptr; }
    int i = 0, ch;
    while (i < cb - 1 && (ch = ChGetNativeFile(pf)) != EOF) { psz[i++] = static_cast<char>(ch); if (ch == '\n') break; }
    psz[i] = 0;
    return i != 0 || cb == 1 ? psz : nullptr;
}
int64_t CbGetNativeLine(char **ppsz, size_t *pcb, NATIVEFILE *pf) {
    if (!ppsz || !pcb || !pf) { errno = EINVAL; return -1; }
    if (!*ppsz) *pcb = 0;
    size_t cb = 0;
    int ch;
    while ((ch = ChGetNativeFile(pf)) != EOF) {
        if (cb + 1 >= *pcb) {
            size_t next = *pcb ? *pcb * 2 : 128;
            if (next <= *pcb) { pf->Fail(ENOMEM); return -1; }
            char *psz = static_cast<char *>(std::realloc(*ppsz, next));
            if (!psz) { pf->Fail(ENOMEM); return -1; }
            *ppsz = psz; *pcb = next;
        }
        (*ppsz)[cb++] = static_cast<char>(ch);
        if (ch == '\n') break;
    }
    if (cb == 0) return -1;
    (*ppsz)[cb] = 0;
    return static_cast<int64_t>(cb);
}
int IPutNativeString(const char *psz, NATIVEFILE *pf) { size_t cb = std::strlen(psz); return CWriteNative(psz, 1, cb, pf) == cb ? 0 : EOF; }
int CchNativePrintf(NATIVEFILE *pf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char small[2048];
    va_list copy; va_copy(copy, args);
    int cb = std::vsnprintf(small, sizeof(small), fmt, copy);
    va_end(copy);
    int ret = -1;
    if (cb >= 0 && static_cast<size_t>(cb) < sizeof(small)) ret = CWriteNative(small, 1, cb, pf) == static_cast<size_t>(cb) ? cb : -1;
    else if (cb >= 0) {
        ret = FileCall(pf, -1, [&] {
            std::vector<char> rgb(static_cast<size_t>(cb) + 1);
            std::vsnprintf(rgb.data(), rgb.size(), fmt, args);
            return CWriteNative(rgb.data(), 1, cb, pf) == static_cast<size_t>(cb) ? cb : -1;
        });
    }
    va_end(args); return ret;
}
int CNativeScan(NATIVEFILE *pf, const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    int count = 0, ch;
    while (*fmt) {
        if (std::isspace(static_cast<unsigned char>(*fmt))) {
            while (*fmt && std::isspace(static_cast<unsigned char>(*fmt))) ++fmt;
            while ((ch = ChGetNativeFile(pf)) != EOF && std::isspace(static_cast<unsigned char>(ch))) {}
            if (ch != EOF) LSeekNativeFile(pf, -1, 1);
            continue;
        }
        bool wide;
        if (std::strncmp(fmt, "%lld", 4) == 0) { wide = true; fmt += 4; }
        else if (std::strncmp(fmt, "%d", 2) == 0) { wide = false; fmt += 2; }
        else { errno = EINVAL; break; }
        while ((ch = ChGetNativeFile(pf)) != EOF && std::isspace(static_cast<unsigned char>(ch))) {}
        if (ch == EOF) break;
        char token[64]; size_t n = 0;
        if (ch == '+' || ch == '-') { token[n++] = static_cast<char>(ch); ch = ChGetNativeFile(pf); }
        while (ch >= '0' && ch <= '9') {
            if (n + 1 >= sizeof(token)) { pf->Fail(ERANGE); break; }
            token[n++] = static_cast<char>(ch); ch = ChGetNativeFile(pf);
        }
        if (ch != EOF) LSeekNativeFile(pf, -1, 1);
        token[n] = 0;
        char *end; errno = 0;
        long long value = std::strtoll(token, &end, 10);
        if (end == token || *end || errno == ERANGE || (!wide && (value < INT_MIN || value > INT_MAX))) break;
        if (wide) *va_arg(args, long long *) = value; else *va_arg(args, int *) = static_cast<int>(value);
        ++count;
    }
    va_end(args);
    return count == 0 && pf && pf->fEof ? EOF : count;
}

int IGetNativeFileInfo(const char *psz, NATIVEFILEINFO *info, int follow) {
    if (!info || !FPath(psz)) return -1;
    return FileCall(nullptr, -1, [&] {
        std::error_code ec;
        fs::path pth = fs::u8path(psz);
        auto status = follow ? fs::status(pth, ec) : fs::symlink_status(pth, ec);
        if (ec || !fs::exists(status)) { if (ec) SetError(ec); else errno = ENOENT; return -1; }
        NATIVEFILEINFO result = {};
        result.fDirectory = fs::is_directory(status); result.fRegular = fs::is_regular_file(status);
        result.fLink = fs::is_symlink(status);
        result.grfMode = static_cast<unsigned>(status.permissions()) & 0777;
#ifdef _WIN32
        // Junctions and other reparse points are also unsafe to traverse during
        // library deletion; std::filesystem's symlink classification is narrower.
        DWORD attr = GetFileAttributesW(pth.c_str());
        if (!follow && attr != INVALID_FILE_ATTRIBUTES) {
            result.fLink = (attr & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
            result.fDirectory = (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
        }
#endif
        if (result.fRegular) {
            uintmax_t cb = fs::file_size(pth, ec);
            if (ec || cb > INT64_MAX) { if (ec) SetError(ec); else errno = EOVERFLOW; return -1; }
            result.cb = static_cast<int64_t>(cb);
        }
        *info = result; return 0;
    });
}
int IAccessNativePath(const char *psz, int md) {
    if (!FPath(psz)) return -1;
    return FileCall(nullptr, -1, [&] {
#ifdef _WIN32
        return _waccess(fs::u8path(psz).c_str(), md);
#else
        return access(psz, md);
#endif
    });
}
int ICreateNativeDirectory(const char *psz, unsigned md) {
    if (!FPath(psz)) return -1;
    return FileCall(nullptr, -1, [&] {
#ifdef _WIN32
        (void)md;
#else
        // Private profile directories must have their mode at creation, with
        // umask applied; a later permissions() call creates an exposure window.
        if (md != 0777) return mkdir(psz, static_cast<mode_t>(md));
#endif
        std::error_code ec;
        if (fs::create_directory(fs::u8path(psz), ec)) return 0;
        if (ec) SetError(ec); else errno = EEXIST;
        return -1;
    });
}
int FEnsureNativeDirectory(const char *psz) {
    if (!FPath(psz)) return 0;
    return FileCall(nullptr, 0, [&] {
        std::error_code ec; fs::path pth = fs::u8path(psz);
        fs::create_directories(pth, ec);
        if (ec) { SetError(ec); return 0; }
        return fs::is_directory(pth, ec) && !ec ? 1 : 0;
    });
}
int IRemoveNativePath(const char *psz, int directory) {
    if (!FPath(psz)) return -1;
    return FileCall(nullptr, -1, [&] {
#ifdef _WIN32
        if (!directory) {
            if (DeleteFileW(fs::u8path(psz).c_str())) return 0;
            errno = GetLastError() == ERROR_FILE_NOT_FOUND ? ENOENT : EACCES;
            return -1;
        }
#else
        (void)directory;
#endif
        std::error_code ec;
        if (fs::remove(fs::u8path(psz), ec)) return 0;
        if (ec) SetError(ec); else errno = ENOENT;
        return -1;
    });
}
int IRenameNativePath(const char *oldPath, const char *newPath, int replace, int sync) {
    if (!FPath(oldPath) || !FPath(newPath)) return -1;
    return FileCall(nullptr, -1, [&] {
#ifdef _WIN32
        DWORD flags = (replace ? MOVEFILE_REPLACE_EXISTING : 0) | (sync ? MOVEFILE_WRITE_THROUGH : 0);
        if (MoveFileExW(fs::u8path(oldPath).c_str(), fs::u8path(newPath).c_str(), flags)) return 0;
        DWORD error = GetLastError();
        errno = error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? ENOENT :
                error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS ? EEXIST : EACCES;
        return -1;
#else
        (void)replace; (void)sync;
        std::error_code ec; fs::rename(fs::u8path(oldPath), fs::u8path(newPath), ec);
        if (ec) { SetError(ec); return -1; }
        return 0;
#endif
    });
}
static char *CopyNativePath(const fs::path &pth, char *out, size_t capacity) {
    std::string text = pth.generic_u8string();
    if (!FUtf8Valid(text.data(), text.size())) { errno = EILSEQ; return nullptr; }
    if (out && text.size() >= capacity) { errno = ERANGE; return nullptr; }
    if (!out) { out = static_cast<char *>(std::malloc(text.size() + 1)); if (!out) return nullptr; }
    std::memcpy(out, text.c_str(), text.size() + 1);
    return out;
}
char *PszCanonicalNativePath(const char *psz, char *out, size_t capacity) {
    if (!FPath(psz)) return nullptr;
    return FileCall<char *>(nullptr, nullptr, [&]() -> char * {
        std::error_code ec; fs::path pth = fs::u8path(psz);
#ifdef _WIN32
        // The old SDK bridge used GetFullPathName plus existence, without
        // resolving reparse points or changing the user's path identity.
        pth = fs::absolute(pth, ec).lexically_normal();
        if (!ec && !fs::exists(pth, ec)) { errno = ENOENT; return nullptr; }
#else
        pth = fs::canonical(pth, ec);
#endif
        if (ec) { SetError(ec); return nullptr; }
        return CopyNativePath(pth, out, capacity);
    });
}
char *PszNativeCwd(char *out, size_t capacity) {
    return FileCall<char *>(nullptr, nullptr, [&]() -> char * {
        std::error_code ec; auto pth = fs::current_path(ec);
        if (ec) { SetError(ec); return nullptr; }
        return CopyNativePath(pth, out, capacity);
    });
}
int IChangeNativeCwd(const char *psz) {
    if (!FPath(psz)) return -1;
    return FileCall(nullptr, -1, [&] {
        std::error_code ec; fs::current_path(fs::u8path(psz), ec);
        if (ec) { SetError(ec); return -1; }
        return 0;
    });
}
struct NATIVEDIR {
    fs::directory_iterator iter;
    bool fFirst = true;
    std::string szEntry;
};
NATIVEDIR *PdirNativeOpen(const char *psz) {
    if (!FPath(psz)) return nullptr;
    return FileCall<NATIVEDIR *>(nullptr, nullptr, [&]() -> NATIVEDIR * {
        auto dir = std::make_unique<NATIVEDIR>(); std::error_code ec;
        dir->iter = fs::directory_iterator(fs::u8path(psz), ec);
        if (ec) { SetError(ec); return nullptr; }
        return dir.release();
    });
}
const char *PszNativeReadDir(NATIVEDIR *dir) {
    if (!dir) { errno = EBADF; return nullptr; }
    return FileCall<const char *>(nullptr, nullptr, [&]() -> const char * {
        std::error_code ec;
        if (dir->iter == fs::directory_iterator()) { errno = 0; return nullptr; }
        if (!dir->fFirst) dir->iter.increment(ec);
        dir->fFirst = false;
        if (ec) { SetError(ec); return nullptr; }
        if (dir->iter == fs::directory_iterator()) { errno = 0; return nullptr; }
        dir->szEntry = dir->iter->path().filename().u8string();
        errno = 0; return dir->szEntry.c_str();
    });
}
int ICloseNativeDir(NATIVEDIR *dir) { delete dir; return 0; }

int FNativeFileIdentity(NATIVEFILE *pf, const char *psz, uint64_t *device, uint64_t *file) {
    if (!device || !file || (!pf && !FPath(psz))) return 0;
    return FileCall(pf, 0, [&] {
#ifdef _WIN32
        HANDLE h = pf && pf->fd >= 0 ? reinterpret_cast<HANDLE>(_get_osfhandle(pf->fd)) : INVALID_HANDLE_VALUE;
        if (!pf) h = CreateFileW(fs::u8path(psz).c_str(), FILE_READ_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        BY_HANDLE_FILE_INFORMATION info;
        bool ok = h != INVALID_HANDLE_VALUE && GetFileInformationByHandle(h, &info);
        if (!pf && h != INVALID_HANDLE_VALUE) CloseHandle(h);
        if (!ok) { errno = EACCES; return 0; }
        *device = info.dwVolumeSerialNumber;
        *file = (static_cast<uint64_t>(info.nFileIndexHigh) << 32) | info.nFileIndexLow;
#else
        struct stat st;
        int ret = pf ? pf->fd >= 0 ? fstat(pf->fd, &st) : -1 : stat(psz, &st);
        if (ret != 0) return 0;
        *device = st.st_dev; *file = st.st_ino;
#endif
        return 1;
    });
}
int ICopyNativePermissions(NATIVEFILE *pf, const char *psz) {
    if (!pf || !FPath(psz)) return -1;
#ifdef _WIN32
    return 0;
#else
    struct stat st;
    if (stat(psz, &st) != 0) return 0;
    return pf->fd < 0 ? pf->Fail(ENOTSUP) : fchmod(pf->fd, st.st_mode & 0777);
#endif
}
NATIVEFILE *PfNativeCreateTemp(char *text) {
    if (!FPath(text)) return nullptr;
    size_t n = std::strlen(text);
    if (n < 6 || std::strcmp(text + n - 6, "XXXXXX") != 0) { errno = EINVAL; return nullptr; }
    return FileCall<NATIVEFILE *>(nullptr, nullptr, [&]() -> NATIVEFILE * {
        auto pf = std::make_unique<NATIVEFILE>(); pf->grfAccess = 3;
#ifdef _WIN32
        static const char alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        for (int attempt = 0; attempt < 128; ++attempt) {
            unsigned char bytes[6];
            if (BCryptGenRandom(nullptr, bytes, sizeof(bytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) { errno = EIO; return nullptr; }
            for (int i = 0; i < 6; ++i) text[n - 6 + i] = alphabet[bytes[i] % 62];
            pf->pth = fs::u8path(text);
            int err = _wsopen_s(&pf->fd, pf->pth.c_str(), _O_CREAT | _O_EXCL | _O_RDWR | _O_NOINHERIT | _O_BINARY,
                               _SH_DENYRW, _S_IREAD | _S_IWRITE);
            if (err == 0) return pf.release();
            errno = err; if (err != EEXIST) return nullptr;
        }
        errno = EEXIST; return nullptr;
#else
        // mkstemp's descriptor and 0600 mode are kept as one owned resource.
        // Allocate all C++ state before claiming the file, including its path.
        pf->pth = fs::u8path(text);
        pf->fd = mkstemp(text);
        if (pf->fd < 0) return nullptr;
        try { pf->pth = fs::u8path(text); }
        catch (...) { unlink(text); throw; }
        return pf.release();
#endif
    });
}
NATIVEFILE *PfNativeTemporary(void) {
    return FileCall<NATIVEFILE *>(nullptr, nullptr, [&]() -> NATIVEFILE * {
        std::string name = (fs::temp_directory_path() / "stars-file-XXXXXX").u8string();
        std::vector<char> text(name.begin(), name.end()); text.push_back(0);
        NATIVEFILE *pf = PfNativeCreateTemp(text.data());
        if (pf) {
#ifdef _WIN32
            pf->fDelete = true;
#else
            if (unlink(text.data()) != 0) { ICloseNativeFile(pf); return nullptr; }
#endif
        }
        return pf;
    });
}

struct LEGACYFILES {
    std::mutex lock;
    std::array<std::unique_ptr<NATIVEFILE>, INT16_MAX + 1> rg;
};
static LEGACYFILES &Files() { static LEGACYFILES files; return files; }
static NATIVEFILE *LegacyFile(int16_t hf) {
    if (hf < 0 || !Files().rg[static_cast<unsigned>(hf)]) { errno = EBADF; return nullptr; }
    return Files().rg[static_cast<unsigned>(hf)].get();
}
int16_t HfNativeOpenLegacy(const char *psz, uint16_t md, int16_t *missing) {
    return FileCall<int16_t>(nullptr, -1, [&]() -> int16_t {
    *missing = 0;
    unsigned access = (md & 3) == 1 ? 2 : (md & 3) == 2 ? 3 : 1;
    std::unique_ptr<NATIVEFILE> pf(PfNativeOpenFlags(psz, access, md & 0x70, md & 0x1000, md & 0x1000, 0, 0));
    if (!pf) { *missing = errno == ENOENT; return -1; }
    std::lock_guard<std::mutex> guard(Files().lock);
    for (int i = 3; i <= INT16_MAX; ++i) if (!Files().rg[i]) { Files().rg[i] = std::move(pf); return static_cast<int16_t>(i); }
    errno = EMFILE; return -1;
    });
}
int32_t CbNativeReadLegacy(int16_t hf, void *rgb, uint16_t cb) {
    return FileCall<int32_t>(nullptr, -1, [&]() -> int32_t {
    std::lock_guard<std::mutex> guard(Files().lock);
    return static_cast<int32_t>(CbReadNativeFile(LegacyFile(hf), rgb, cb));
    });
}
int32_t CbNativeWriteLegacy(int16_t hf, const void *rgb, uint16_t cb) {
    return FileCall<int32_t>(nullptr, -1, [&]() -> int32_t {
    std::lock_guard<std::mutex> guard(Files().lock); NATIVEFILE *pf = LegacyFile(hf);
#ifdef _WIN32
    if (cb == 0) return pf ? ITruncateNativeFile(pf, LNativePosition(pf)) : -1;
#endif
    int64_t put = CbWriteNativeFile(pf, rgb, cb);
    // Legacy raw writes report disk errors immediately, rather than deferring
    // a buffered stream failure to the old void CloseFile interface.
    if (put >= 0 && IFlushNativeFile(pf) != 0) return -1;
    return static_cast<int32_t>(put);
    });
}
int32_t LNativeSeekLegacy(int16_t hf, int32_t ib, int16_t origin) {
    return FileCall<int32_t>(nullptr, -1, [&]() -> int32_t {
    std::lock_guard<std::mutex> guard(Files().lock);
    int64_t pos = LSeekNativeFile(LegacyFile(hf), ib, origin == 0 ? 0 : origin == 1 ? 1 : 2);
#ifdef _WIN32
    if (pos > INT32_MAX) return -1;
#endif
    return static_cast<int32_t>(pos);
    });
}
int32_t CbNativeSizeLegacy(int16_t hf) {
    return FileCall<int32_t>(nullptr, -1, [&]() -> int32_t {
    std::lock_guard<std::mutex> guard(Files().lock);
    int64_t cb = CbNativeFile(LegacyFile(hf));
#ifdef _WIN32
    if (cb > INT32_MAX) return -1;
#endif
    return static_cast<int32_t>(cb);
    });
}
void CloseNativeLegacy(int16_t hf) {
    FileCall(nullptr, -1, [&] {
    std::lock_guard<std::mutex> guard(Files().lock);
    if (LegacyFile(hf)) Files().rg[static_cast<unsigned>(hf)].reset();
        return 0;
    });
}

int IGetNativeOpenInfo(NATIVEFILE *pf, NATIVEFILEINFO *info) {
    if (!pf || !info || pf->fd < 0) { errno = EBADF; return -1; }
#ifdef _WIN32
    struct _stat64 st;
    if (_fstat64(pf->fd, &st) != 0) return -1;
    info->fRegular = (st.st_mode & _S_IFMT) == _S_IFREG;
    info->fDirectory = (st.st_mode & _S_IFMT) == _S_IFDIR;
#else
    struct stat st;
    if (fstat(pf->fd, &st) != 0) return -1;
    info->fRegular = S_ISREG(st.st_mode);
    info->fDirectory = S_ISDIR(st.st_mode);
#endif
    info->cb = st.st_size; info->grfMode = st.st_mode & 0777; info->fLink = 0;
    return 0;
}
int IUnlinkNativePath(const char *psz) {
    if (!FPath(psz)) return -1;
    return FileCall(nullptr, -1, [&] {
#ifdef _WIN32
        return _wunlink(fs::u8path(psz).c_str());
#else
        return unlink(psz);
#endif
    });
}
