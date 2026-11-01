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
#include <string>
#include <string_view>
#include <memory>
#include <variant>
#include <type_traits>
#include "nativeformat.h"
struct NATIVEFILE;

struct NATIVEDIR;

struct NATIVEFILEINFO {
    std::int64_t cb;
    std::uint32_t grfMode;
    int fDirectory, fRegular, fLink;
};

struct NATIVEFILEIDENTITY {
    std::uint64_t idDevice, idFile;
};
enum class NATIVELINKMODE : std::uint8_t { follow, inspect };

enum class NATIVESEEKORIGIN : std::uint8_t { begin = 0, current = 1, end = 2 };
// read/readWrite require an existing file; other modes create missing files.
// Truncate replaces bytes; append writes at EOF; exclusive refuses an existing file.
enum class NATIVEFILEMODE : std::uint8_t {
    read, readWrite, writeTruncate, readWriteTruncate,
    writeAppend, readWriteAppend, writeExclusive, readWriteExclusive
};
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
NATIVEFILE *PfNativeOpen(const char *psz, NATIVEFILEMODE mode);
NATIVEFILE *PfNativeOpenFlags(const char *psz, NATIVEFILEOPENOPTIONS nfo);
int ICloseNativeFile(NATIVEFILE *pf);
using NATIVEFILEOWNER = std::unique_ptr<NATIVEFILE, decltype(&ICloseNativeFile)>;
struct NATIVEFILETEMPRESULT {
    std::string szPath;
    NATIVEFILEOWNER lpfile{nullptr, ICloseNativeFile};
};
// The readonly UTF-8 template ends in XXXXXX. The result retains the attempted
// candidate even on open failure and owns an unadopted successful stream.
NATIVEFILETEMPRESULT NftNativeCreateTemp(std::string_view szTemplate);
NATIVEFILE *PfNativeTemporary(void);
// A present count includes successful zero-byte IO; nullopt reports failure
// through errno. Partial counts and stream error/EOF state remain observable.
std::optional<std::size_t> CbReadNativeFile(NATIVEFILE *pf, std::span<std::byte> rgb);
std::optional<std::size_t> CbWriteNativeFile(NATIVEFILE *pf, std::span<const std::byte> rgb);
// Stream loops return all transferred bytes, including an incomplete element.
std::size_t CbReadNativeStream(std::span<std::byte> rgb, NATIVEFILE *pf);
std::size_t CbWriteNativeStream(std::span<const std::byte> rgb, NATIVEFILE *pf);
// Element views count only complete objects; a partial final object is still
// modified and advances the stream, as with the original element interface.
template<class T, std::size_t N>
    requires(std::is_trivially_copyable_v<T> && !std::is_const_v<T>)
std::size_t CReadNative(std::span<T, N> rg, NATIVEFILE *pf) {
    return CbReadNativeStream(std::as_writable_bytes(rg), pf) / sizeof(T);
}
template<class T, std::size_t N>
    requires(std::is_trivially_copyable_v<T>)
std::size_t CWriteNative(std::span<T, N> rg, NATIVEFILE *pf) {
    return CbWriteNativeStream(std::as_bytes(rg), pf) / sizeof(T);
}
std::int64_t LSeekNativeFile(NATIVEFILE *pf, std::int64_t ib, NATIVESEEKORIGIN iOrigin);
int ISeekNativeFile(NATIVEFILE *pf, std::int64_t ib, NATIVESEEKORIGIN iOrigin);
std::int64_t LNativePosition(NATIVEFILE *pf);
std::int64_t CbNativeFile(NATIVEFILE *pf);
int ITruncateNativeFile(NATIVEFILE *pf, std::int64_t cb);
int IFlushNativeFile(NATIVEFILE *pf);
int ISyncNativeFile(NATIVEFILE *pf);
int ICopyNativePermissions(NATIVEFILE *pf, const char *pszExisting);
std::optional<NATIVEFILEIDENTITY> FNativeFileIdentity(NATIVEFILE *pf);
std::optional<NATIVEFILEIDENTITY> FNativeFileIdentity(const char *psz);
int FNativeStreamError(NATIVEFILE *pf);
int FNativeStreamEof(NATIVEFILE *pf);
int ChGetNativeFile(NATIVEFILE *pf);
int ChPutNativeFile(int ch, NATIVEFILE *pf);
// Binary line bytes retain embedded NULs/newlines. A bounded view reserves a
// terminator; size one succeeds with zero bytes and no IO, including at EOF.
std::optional<std::size_t> CchGetNativeLine(std::span<char> sz, NATIVEFILE *pf);
std::optional<std::string> SzGetNativeLine(NATIVEFILE *pf);
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

// Empty metadata results retain the native errno failure detail. inspect reports
// the entry itself, including links/reparse points, without following its target.
std::optional<NATIVEFILEINFO> IGetNativeOpenInfo(NATIVEFILE *pf);
int IUnlinkNativePath(const char *psz);
std::optional<NATIVEFILEINFO> IGetNativeFileInfo(const char *psz, NATIVELINKMODE mode);
int IAccessNativePath(const char *psz, int md);
int ICreateNativeDirectory(const char *psz, unsigned md);
int FEnsureNativeDirectory(const char *psz);
int IRemoveNativePath(const char *psz, int fDirectory);
int IRenameNativePath(const char *pszOld, const char *pszNew, int fReplace, int fSync);
std::optional<std::string> SzCanonicalNativePath(const char *psz);
std::optional<std::string> SzNativeCwd();
// Counts exclude the NUL; insufficient spans retain their bytes and set ERANGE.
std::optional<std::size_t> CchCanonicalNativePath(const char *psz, std::span<char> szOut);
std::optional<std::size_t> CchNativeCwd(std::span<char> szOut);
int IChangeNativeCwd(const char *psz);
NATIVEDIR *PdirNativeOpen(const char *psz);
// POSIX entry bytes are returned verbatim so callers can report unsupported
// names; validate UTF-8 before using an entry as a path. No value: errno zero at end, or an error.
std::optional<std::string> SzNativeReadDir(NATIVEDIR *pdir);
int ICloseNativeDir(NATIVEDIR *pdir);

// Small owned tokens keep native pointers/descriptors out of game storage.
[[nodiscard]] NATIVEFILEOPENRESULT HfNativeOpenLegacy(const char *psz, NATIVEFILEOPENOPTIONS nfo);
std::optional<std::size_t> CbNativeReadLegacy(std::int16_t hf, std::span<std::byte> rgb);
std::optional<std::size_t> CbNativeWriteLegacy(std::int16_t hf, std::span<const std::byte> rgb);
std::int32_t LNativeSeekLegacy(std::int16_t hf, std::int32_t ib, NATIVESEEKORIGIN iOrigin);
std::optional<std::int64_t> FNativeSizeLegacy(std::int16_t hf);
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
