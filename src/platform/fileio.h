#ifndef STARS_FILEIO_H
#define STARS_FILEIO_H
#include <stddef.h>
#include <stdint.h>
struct NATIVEFILE;

struct NATIVEDIR;

struct NATIVEFILEINFO {
    int64_t cb;
    uint32_t grfMode;
    int fDirectory, fRegular, fLink;
};

// Strict UTF-8 paths; binary bytes are never decoded or translated. All service
// functions contain exceptions and report failures through return values/errno.
// OpenFlags access: 1 read, 2 write, 3 both. Share uses Win16 0x10/20/30/40.
// Native access is required for live identity, exclusive creation and durable IO.
NATIVEFILE *PfNativeOpen(const char *psz, const char *pszMode);
NATIVEFILE *PfNativeOpenFlags(const char *psz, unsigned grfAccess, unsigned grfShare,
                            int fCreate, int fTruncate, int fExclusive, int fNative);
NATIVEFILE *PfNativeCreateTemp(char *pszTemplate);
NATIVEFILE *PfNativeTemporary(void);
int ICloseNativeFile(NATIVEFILE *pf);
int64_t CbReadNativeFile(NATIVEFILE *pf, void *rgb, size_t cb);
int64_t CbWriteNativeFile(NATIVEFILE *pf, const void *rgb, size_t cb);
size_t CReadNative(void *rgb, size_t cb, size_t c, NATIVEFILE *pf);
size_t CWriteNative(const void *rgb, size_t cb, size_t c, NATIVEFILE *pf);
int64_t LSeekNativeFile(NATIVEFILE *pf, int64_t ib, int iOrigin);
int ISeekNativeFile(NATIVEFILE *pf, int64_t ib, int iOrigin);
int64_t LNativePosition(NATIVEFILE *pf);
int64_t CbNativeFile(NATIVEFILE *pf);
int ITruncateNativeFile(NATIVEFILE *pf, int64_t cb);
int IFlushNativeFile(NATIVEFILE *pf);
int ISyncNativeFile(NATIVEFILE *pf);
int ICopyNativePermissions(NATIVEFILE *pf, const char *pszExisting);
int FNativeFileIdentity(NATIVEFILE *pf, const char *psz, uint64_t *pidDevice, uint64_t *pidFile);
int FNativeStreamError(NATIVEFILE *pf);
int FNativeStreamEof(NATIVEFILE *pf);
int ChGetNativeFile(NATIVEFILE *pf);
int ChPutNativeFile(int ch, NATIVEFILE *pf);
char *PszGetNativeLine(char *psz, int cb, NATIVEFILE *pf);
int64_t CbGetNativeLine(char **ppsz, size_t *pcb, NATIVEFILE *pf);
int IPutNativeString(const char *psz, NATIVEFILE *pf);
int CchNativePrintf(NATIVEFILE *pf, const char *pszFormat, ...);
// Metadata's ASCII integer fields: %d and %lld, with scanf whitespace rules.
int CNativeScan(NATIVEFILE *pf, const char *pszFormat, ...);

int IGetNativeOpenInfo(NATIVEFILE *pf, NATIVEFILEINFO *pinfo);
int IUnlinkNativePath(const char *psz);
int IGetNativeFileInfo(const char *psz, NATIVEFILEINFO *pinfo, int fFollow);
int IAccessNativePath(const char *psz, int md);
int ICreateNativeDirectory(const char *psz, unsigned md);
int FEnsureNativeDirectory(const char *psz);
int IRemoveNativePath(const char *psz, int fDirectory);
int IRenameNativePath(const char *pszOld, const char *pszNew, int fReplace, int fSync);
char *PszCanonicalNativePath(const char *psz, char *pszOut, size_t cbOut);
char *PszNativeCwd(char *pszOut, size_t cbOut);
int IChangeNativeCwd(const char *psz);
NATIVEDIR *PdirNativeOpen(const char *psz);
// POSIX entry bytes are returned verbatim so callers can report unsupported
// names; validate UTF-8 before using an entry as a path. NULL: end or errno.
const char *PszNativeReadDir(NATIVEDIR *pdir);
int ICloseNativeDir(NATIVEDIR *pdir);

// Small owned tokens keep native pointers/descriptors out of game storage.
int16_t HfNativeOpenLegacy(const char *psz, uint16_t md, int16_t *pfMissing);
int32_t CbNativeReadLegacy(int16_t hf, void *rgb, uint16_t cb);
int32_t CbNativeWriteLegacy(int16_t hf, const void *rgb, uint16_t cb);
int32_t LNativeSeekLegacy(int16_t hf, int32_t ib, int16_t iOrigin);
int32_t CbNativeSizeLegacy(int16_t hf);
void CloseNativeLegacy(int16_t hf);



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
#endif
