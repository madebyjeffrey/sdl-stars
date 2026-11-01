#include <cerrno>
#include <cstdio>
#include <cstring>
#include <thread>
#include "filesystem.h"
#include "native.h"
#include "native_publish.h"

static const NATIVEPUBLISHIO ioNative;
static int                   cRemove, cRename, fReplaceObserved, fSyncObserved, fThrow;
struct PUBLICATIONFAILURE {};
static int IRemove(const char *psz, int fDirectory) {
    if (fDirectory || std::strcmp(psz, "file"))
        return -29;
    cRemove++;
    return 17;
}
static int IRename(const char *pszOld, const char *pszNew, int fReplace, int fSync) {
    if (std::strcmp(pszOld, "old") || std::strcmp(pszNew, "new"))
        return -29;
    cRename++;
    fReplaceObserved = fReplace;
    fSyncObserved = fSync;
    if (fThrow) {
        NATIVEPUBLISHIOSCOPE nested(ioNative);
        throw PUBLICATIONFAILURE{};
    }
    return 19;
}
static int FWrite(const char *psz, const char *pszText) {
    NATIVEFILE *pf = PfNativeOpen(psz, "wb");
    int         fWritten;
    if (!pf)
        return 0;
    fWritten = CWriteNative(pszText, 1, std::strlen(pszText), pf) == std::strlen(pszText);
    return ICloseNativeFile(pf) == 0 && fWritten;
}
#define CHECK(f)                                                                                                                                               \
    do {                                                                                                                                                       \
        if (!(f)) {                                                                                                                                            \
            std::fprintf(stderr, "publication line %d: %s\n", __LINE__, #f);                                                                                   \
            return 1;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)
int main() {
    char            szDirectory[4096], szOld[8192], szNew[8192], rgb[8] = {};
    NATIVEFILE     *pf;
    NATIVEPUBLISHIO io;
    bool            fCaught = false, fThreadDefault = false;
    io.pfnRemove = IRemove;
    io.pfnRename = IRename;
    CHECK(NativePublishIO().pfnRename == IRenameNativePath);
    {
        NATIVEPUBLISHIOSCOPE outer(io);
        io.pfnRename = IRenameNativePath;
        CHECK(NativePublishIO().pfnRename == IRename);
        CHECK(RemoveNativeFile("file") == 17 && cRemove == 1);
        CHECK(ReplaceNativeFile("old", "new") == 19 && cRename == 1 && fReplaceObserved && !fSyncObserved);
        CHECK(RenameNativeFile("old", "new") == 19 && cRename == 2 && !fReplaceObserved && !fSyncObserved);
        {
            NATIVEPUBLISHIOSCOPE nested(ioNative);
            CHECK(NativePublishIO().pfnRename == IRenameNativePath);
        }
        CHECK(NativePublishIO().pfnRename == IRename);
        fThrow = 1;
        try {
            ReplaceNativeFile("old", "new");
        } catch (const PUBLICATIONFAILURE &) {
            fCaught = true;
        }
        fThrow = 0;
        CHECK(fCaught && NativePublishIO().pfnRename == IRename);
        CHECK(RenameNativeFile("old", "new") == 19 && cRename == 4);
        std::thread worker([&] { fThreadDefault = NativePublishIO().pfnRename == IRenameNativePath; });
        worker.join();
        CHECK(fThreadDefault);
    }
    CHECK(NativePublishIO().pfnRemove == IRemoveNativePath && NativePublishIO().pfnRename == IRenameNativePath);
    CHECK(FFilesystemTempDir(szDirectory, sizeof(szDirectory)));
    CHECK(std::snprintf(szOld, sizeof(szOld), "%s/café-Ω", szDirectory) > 0);
    CHECK(std::snprintf(szNew, sizeof(szNew), "%s/target", szDirectory) > 0);
    CHECK(FWrite(szOld, "first") && RenameNativeFile(szOld, szNew) == 0);
    CHECK(FWrite(szOld, "new") && FWrite(szNew, "old"));
    CHECK(ReplaceNativeFile(szOld, szNew) == 0);
    pf = PfNativeOpen(szNew, "rb");
    CHECK(pf && CbReadNativeFile(pf, std::as_writable_bytes(std::span{rgb}).first(static_cast<std::size_t>(sizeof(rgb)))) == 3 && !std::memcmp(rgb, "new", 3));
    CHECK(ICloseNativeFile(pf) == 0 && RemoveNativeFile(szNew) == 0);
    CHECK(FFilesystemRemove(szDirectory));
    std::puts("Publication effects: exact flags/results, snapshots, nesting, failure/retry, thread isolation and real UTF-8 filesystem delegation passed");
    return 0;
}
