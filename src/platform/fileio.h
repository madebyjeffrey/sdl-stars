#ifndef STARS_FILEIO_H
#define STARS_FILEIO_H
// Native service values and standard-library types must keep native layout
// when included from a caller's packed record scope.
#pragma pack(push)
#pragma pack()
#include <optional>
#include <cstddef>
#include <cstdint>
#include <array>
#include <span>
#include <variant>
#include "nativeformat.h"
struct NATIVEFILE;

struct NATIVEDIR;

struct NATIVEFILEINFO {
    std::int64_t cb;
    std::uint32_t grfMode;
    int fDirectory, fRegular, fLink;
};

enum class NATIVESEEKORIGIN : std::uint8_t { begin = 0, current = 1, end = 2 };
enum class NATIVEFILEACCESS : std::uint8_t { read = 1, write = 2, readWrite = 3 };
enum class NATIVEFILESHARE : std::uint8_t { compatible = 0, exclusive = 0x10, denyWrite = 0x20, denyRead = 0x30, denyNone = 0x40 };
struct NATIVEFILEOPENOPTIONS {
    NATIVEFILEACCESS mdAccess = NATIVEFILEACCESS::read;
    NATIVEFILESHARE mdShare = NATIVEFILESHARE::compatible;
    bool fCreate = false, fTruncate = false, fExclusive = false, fNative = false;
};
struct NATIVEFILEOPENRESULT {
    std::int16_t hf = -1;
    bool fMissing = false;
};

// Strict UTF-8 paths; binary bytes are never decoded or translated. All service
// functions contain exceptions and report failures through return values/errno.
// Native access is required for live identity, exclusive creation and durable IO.
NATIVEFILE *PfNativeOpen(const char *psz, const char *pszMode);
NATIVEFILE *PfNativeOpenFlags(const char *psz, NATIVEFILEOPENOPTIONS nfo);
NATIVEFILE *PfNativeCreateTemp(char *pszTemplate);
NATIVEFILE *PfNativeTemporary(void);
int ICloseNativeFile(NATIVEFILE *pf);
// A present count includes successful zero-byte IO; nullopt reports failure
// through errno. Partial counts and stream error/EOF state remain observable.
std::optional<std::size_t> CbReadNativeFile(NATIVEFILE *pf, std::span<std::byte> rgb);
std::optional<std::size_t> CbWriteNativeFile(NATIVEFILE *pf, std::span<const std::byte> rgb);
std::size_t CReadNative(void *rgb, std::size_t cb, std::size_t c, NATIVEFILE *pf);
std::size_t CWriteNative(const void *rgb, std::size_t cb, std::size_t c, NATIVEFILE *pf);
std::int64_t LSeekNativeFile(NATIVEFILE *pf, std::int64_t ib, NATIVESEEKORIGIN iOrigin);
int ISeekNativeFile(NATIVEFILE *pf, std::int64_t ib, NATIVESEEKORIGIN iOrigin);
std::int64_t LNativePosition(NATIVEFILE *pf);
std::int64_t CbNativeFile(NATIVEFILE *pf);
int ITruncateNativeFile(NATIVEFILE *pf, std::int64_t cb);
int IFlushNativeFile(NATIVEFILE *pf);
int ISyncNativeFile(NATIVEFILE *pf);
int ICopyNativePermissions(NATIVEFILE *pf, const char *pszExisting);
int FNativeFileIdentity(NATIVEFILE *pf, const char *psz, std::uint64_t *pidDevice, std::uint64_t *pidFile);
int FNativeStreamError(NATIVEFILE *pf);
int FNativeStreamEof(NATIVEFILE *pf);
int ChGetNativeFile(NATIVEFILE *pf);
int ChPutNativeFile(int ch, NATIVEFILE *pf);
char *PszGetNativeLine(char *psz, int cb, NATIVEFILE *pf);
std::int64_t CbGetNativeLine(char **ppsz, std::size_t *pcb, NATIVEFILE *pf);
int IPutNativeString(const char *psz, NATIVEFILE *pf);
int CchNativePrintfFields(NATIVEFILE *pf, std::string_view szFormat, std::span<const NATIVEFORMATARG> rgArgs);
template<class... ARGS>
int CchNativePrintf(NATIVEFILE *pf, const char *pszFormat, ARGS... args) {
    if (!pszFormat) { errno = EINVAL; return -1; }
    const std::array<NATIVEFORMATARG, sizeof...(ARGS)> rgArgs = {NATIVEFORMATARG{args}...};
    return CchNativePrintfFields(pf, pszFormat, rgArgs);
}
// Metadata's ASCII integer fields: %d and %lld, with scanf whitespace rules.
using NATIVEINTEGERFIELD = std::variant<int *, long long *>;
int CNativeScanFields(NATIVEFILE *pf, const char *pszFormat, std::span<const NATIVEINTEGERFIELD> rgFields);
template<class... FIELDS>
int CNativeScan(NATIVEFILE *pf, const char *pszFormat, FIELDS... fields) {
    const std::array<NATIVEINTEGERFIELD, sizeof...(FIELDS)> rgFields = {NATIVEINTEGERFIELD{fields}...};
    return CNativeScanFields(pf, pszFormat, rgFields);
}

int IGetNativeOpenInfo(NATIVEFILE *pf, NATIVEFILEINFO *pinfo);
int IUnlinkNativePath(const char *psz);
int IGetNativeFileInfo(const char *psz, NATIVEFILEINFO *pinfo, int fFollow);
int IAccessNativePath(const char *psz, int md);
int ICreateNativeDirectory(const char *psz, unsigned md);
int FEnsureNativeDirectory(const char *psz);
int IRemoveNativePath(const char *psz, int fDirectory);
int IRenameNativePath(const char *pszOld, const char *pszNew, int fReplace, int fSync);
char *PszCanonicalNativePath(const char *psz, char *pszOut, std::size_t cbOut);
char *PszNativeCwd(char *pszOut, std::size_t cbOut);
int IChangeNativeCwd(const char *psz);
NATIVEDIR *PdirNativeOpen(const char *psz);
// POSIX entry bytes are returned verbatim so callers can report unsupported
// names; validate UTF-8 before using an entry as a path. NULL: end or errno.
const char *PszNativeReadDir(NATIVEDIR *pdir);
int ICloseNativeDir(NATIVEDIR *pdir);

// Small owned tokens keep native pointers/descriptors out of game storage.
[[nodiscard]] NATIVEFILEOPENRESULT HfNativeOpenLegacy(const char *psz, NATIVEFILEOPENOPTIONS nfo);
std::optional<std::size_t> CbNativeReadLegacy(std::int16_t hf, std::span<std::byte> rgb);
std::optional<std::size_t> CbNativeWriteLegacy(std::int16_t hf, std::span<const std::byte> rgb);
std::int32_t LNativeSeekLegacy(std::int16_t hf, std::int32_t ib, NATIVESEEKORIGIN iOrigin);
std::int32_t CbNativeSizeLegacy(std::int16_t hf);
void CloseNativeLegacy(std::int16_t hf);



class StarsNativeFileGuard {
    NATIVEFILE *pf;
public:
    explicit StarsNativeFileGuard(NATIVEFILE *pf) noexcept : pf(pf) {}
    ~StarsNativeFileGuard() noexcept { if (pf) ICloseNativeFile(pf); }
    NATIVEFILE *Release() noexcept { NATIVEFILE *pfOut = pf; pf = nullptr; return pfOut; }
    StarsNativeFileGuard(const StarsNativeFileGuard &) = delete;
    StarsNativeFileGuard &operator=(const StarsNativeFileGuard &) = delete;
};
class StarsNativeDirGuard {
    NATIVEDIR *pdir;
public:
    explicit StarsNativeDirGuard(NATIVEDIR *pdir) noexcept : pdir(pdir) {}
    ~StarsNativeDirGuard() noexcept { if (pdir) ICloseNativeDir(pdir); }
    StarsNativeDirGuard(const StarsNativeDirGuard &) = delete;
    StarsNativeDirGuard &operator=(const StarsNativeDirGuard &) = delete;
};
#pragma pack(pop)
#endif
