#include "acutest.h"
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
[[maybe_unused]] static char szTestLiteral1[] = "\xff";
#include "stars_test.h"
#include <errno.h>
#include <filesystem>

typedef struct {
    const char *pszExpected;
    int16_t fFound;
} NATIVEENTRYCHECK;

static int16_t FCheckNativeEntry(APPSESSION &, void *lpv, const char *pszEntry) {
    NATIVEENTRYCHECK *lpnec = (NATIVEENTRYCHECK *)lpv;
    if (!strcmp(pszEntry, lpnec->pszExpected)) lpnec->fFound = 1;
    return 1;
}

static void TestNativeFiles(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    char szDir[64], szFile[512], szOther[512], szLong[200], szNested[512], szMeta[512], szMetaTemp[512], rgb[16];
    std::optional<std::int64_t> cbNativeFile;
    NATIVEENTRYCHECK nec;
    int16_t hfFile, fMissing;
    NATIVEFILEOPENRESULT nfr;
    int i;
    snprintf(szDir, sizeof(szDir), "stars-native-%u-\xce\xa9", (unsigned)DwTickCount());
    TEST_CHECK(CreateNativeDir(szDir) == 0 && FNativeDirectoryExists(szDir));
    errno = EBUSY;
    TEST_CHECK(!(cbNativeFile = FNativeFileSize(szDir)) && errno == EBUSY);
    snprintf(szNested, sizeof(szNested), "%s%snested-\xce\xa9", szDir, szDirSep);
    TEST_CHECK(FEnsureNativeDir(szNested) && FNativeDirectoryExists(szNested));
    snprintf(szFile, sizeof(szFile), "%s%s\xe6\x9d\xb1\xe4\xba\xac-\xc3\xa9.bin", szDir, szDirSep);
    snprintf(szOther, sizeof(szOther), "%s%s\xf0\x9f\x98\x80.bin", szDir, szDirSep);
    nfr = HfOpenFile(szFile, {.mdAccess = NATIVEFILEACCESS::readWrite, .mdShare = NATIVEFILESHARE::exclusive, .fCreate = true, .fTruncate = true});
    hfFile = nfr.hf; fMissing = nfr.fMissing;
    TEST_ASSERT(hfFile != -1 && !fMissing);
    TEST_CHECK(CbWriteFile(hfFile, std::as_bytes(std::span{"abcdef"}).first(6)) == 6 && CbFileSize(hfFile) == 6);
    TEST_CHECK(LSeekFile(hfFile, 2, NATIVESEEKORIGIN::begin) == 2 && CbReadFile(hfFile, std::as_writable_bytes(std::span{rgb}).first(static_cast<std::size_t>(4))) == 4 && memcmp(rgb, "cdef", 4) == 0);
    CloseFile(hfFile);
    TEST_CHECK(FFileExists(szFile) && !FFileReadOnly(szFile));
    TEST_CHECK((cbNativeFile = FNativeFileSize(szFile)) && (*cbNativeFile) == 6);
    nec.pszExpected = "\xe6\x9d\xb1\xe4\xba\xac-\xc3\xa9.bin"; nec.fFound = 0;
    TEST_CHECK(FNativeEnumerateDirectory(app, szDir, FCheckNativeEntry, &nec) && nec.fFound);
    TEST_CHECK(RenameNativeFile(szFile, szOther) == 0 && !FFileExists(szFile) && FFileExists(szOther));
    TEST_CHECK((cbNativeFile = FNativeFileSize(szOther)) && (*cbNativeFile) == 6);
    nec.pszExpected = "\xf0\x9f\x98\x80.bin"; nec.fFound = 0;
    TEST_CHECK(FNativeEnumerateDirectory(app, szDir, FCheckNativeEntry, &nec) && nec.fFound);
    snprintf(szMeta, sizeof(szMeta), "%s%s\xe6\x9d\xb1\xe4\xba\xac.meta", szDir, szDirSep);
    snprintf(szMetaTemp, sizeof(szMetaTemp), "%s%s\xce\xa9.tmp", szDir, szDirSep);
    {
        NATIVEFILE *pf = PfOpenNativeFile(szMetaTemp, NATIVEFILEMODE::writeTruncate);
        TEST_ASSERT(pf);
        TEST_CHECK(ICloseNativeFile(pf) == 0 && (cbNativeFile = FNativeFileSize(szMetaTemp)) && (*cbNativeFile) == 0);
        pf = PfOpenNativeFile(szMetaTemp, NATIVEFILEMODE::writeTruncate);
        TEST_ASSERT(pf);
        TEST_CHECK(CWriteNative(std::span{"metadata"}.first(8), pf) == 8 && ICloseNativeFile(pf) == 0);
    }
    TEST_CHECK(ReplaceNativeFile(szMetaTemp, szMeta) == 0 && (cbNativeFile = FNativeFileSize(szMeta)) && (*cbNativeFile) == 8);
    TEST_CHECK(RemoveNativeFile(szMeta) == 0);
    TEST_CHECK(RemoveNativeFile(szOther) == 0 && !FFileExists(szOther));
    nfr = HfOpenFile(szOther, {.mdShare = NATIVEFILESHARE::denyWrite});
    TEST_CHECK(nfr.hf == -1 && nfr.fMissing);
    for (i = 0; i < 150; i++) szLong[i] = 'x';
    strcpy(szLong + 150, "-\xce\xa9.bin");
    snprintf(szFile, sizeof(szFile), "%s%s%s", szDir, szDirSep, szLong);
    nfr = HfOpenFile(szFile, {.mdAccess = NATIVEFILEACCESS::readWrite, .mdShare = NATIVEFILESHARE::exclusive, .fCreate = true, .fTruncate = true});
    hfFile = nfr.hf; fMissing = nfr.fMissing;
    TEST_ASSERT(hfFile != -1);
    TEST_CHECK(CbWriteFile(hfFile, std::as_bytes(std::span{"Z"}).first(1)) == 1); CloseFile(hfFile);
    TEST_CHECK(FFileExists(szFile) && RemoveNativeFile(szFile) == 0);
    nfr = HfOpenFile(szTestLiteral1, {.mdShare = NATIVEFILESHARE::denyWrite});
    TEST_CHECK(nfr.hf == -1 && !nfr.fMissing && errno == EILSEQ);
    TEST_CHECK(RemoveNativeFile("\xff") == -1 && errno == EILSEQ);
    TEST_CHECK(CreateNativeDir("\xff") == -1 && errno == EILSEQ);
    TEST_CHECK(!FNativeDirectoryExists("\xff") && errno == EILSEQ);
    TEST_CHECK(!FNativeEnumerateDirectory(app, "\xff", FCheckNativeEntry, &nec) && errno == EILSEQ);
    TEST_CHECK(!(cbNativeFile = FNativeFileSize("\xff")) && errno == EILSEQ);
    TEST_CHECK(!FEnsureNativeDir("\xff") && errno == EILSEQ);
    TEST_CHECK(RemoveNativePath("\xff") == -1 && errno == EILSEQ);
    TEST_CHECK(ReplaceNativeFile("\xff", szOther) == -1 && errno == EILSEQ);
    TEST_CHECK(RemoveNativePath(szNested) == 0 && !FNativeDirectoryExists(szNested));
    std::error_code ec;
    TEST_CHECK(std::filesystem::remove(std::filesystem::path(reinterpret_cast<const char8_t *>(szDir)), ec) && !ec);
}
static void TestNativeOpenModes(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    char szDir[cbTestPath], szFile[cbTestPath + 64], rgb[8]{};
    int16_t hfFile, fMissing;
    NATIVEFILEOPENRESULT nfr;

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("native-open-modes", szDir, sizeof(szDir)));
    CchSnprintf(std::span{szFile}, "%s%smodes.dat", szDir, szDirSep);
    nfr = HfOpenFile(szFile, {.mdAccess = NATIVEFILEACCESS::readWrite, .mdShare = NATIVEFILESHARE::exclusive, .fCreate = true, .fTruncate = true});
    hfFile = nfr.hf; fMissing = nfr.fMissing;
    TEST_ASSERT(hfFile >= 3 && !fMissing);
    TEST_CHECK(FFileSize(hfFile) == 0);
    errno = 0;
    TEST_CHECK(!FFileSize(-1) && errno == EBADF);
    TEST_CHECK(CbWriteFile(hfFile, std::as_bytes(std::span{"abcdef"}).first(6)) == 6);
    CloseFile(hfFile);
    nfr = HfOpenFile(szFile, {.mdShare = NATIVEFILESHARE::denyWrite});
    hfFile = nfr.hf; fMissing = nfr.fMissing;
    TEST_ASSERT(hfFile >= 3 && !fMissing);
    TEST_CHECK(CbReadFile(hfFile, std::as_writable_bytes(std::span{rgb}).first(static_cast<std::size_t>(6))) == 6 && !memcmp(rgb, "abcdef", 6));
    errno = 0;
    TEST_CHECK(!CbWriteFile(hfFile, std::as_bytes(std::span{"!"}).first(1)) && errno == EBADF);
    CloseFile(hfFile);
    nfr = HfOpenFile(szFile, {.mdAccess = NATIVEFILEACCESS::readWrite, .mdShare = NATIVEFILESHARE::exclusive});
    hfFile = nfr.hf; fMissing = nfr.fMissing;
    TEST_ASSERT(hfFile >= 3 && !fMissing);
    TEST_CHECK(CbFileSize(hfFile) == 6 && CbWriteFile(hfFile, std::as_bytes(std::span{"Z"}).first(1)) == 1);
    CloseFile(hfFile);
    TEST_CHECK(!FFileSize(hfFile) && errno == EBADF);
    OutputFileString(szFile, "gh");
    nfr = HfOpenFile(szFile, {.mdShare = NATIVEFILESHARE::denyWrite});
    hfFile = nfr.hf; fMissing = nfr.fMissing;
    TEST_ASSERT(hfFile >= 3 && !fMissing);
    TEST_CHECK(CbFileSize(hfFile) == 8 && CbReadFile(hfFile, std::as_writable_bytes(std::span{rgb}).first(static_cast<std::size_t>(8))) == 8 && !memcmp(rgb, "Zbcdefgh", 8));
    CloseFile(hfFile);
    nfr = HfOpenFile(szFile, {.mdAccess = NATIVEFILEACCESS::readWrite, .mdShare = NATIVEFILESHARE::exclusive, .fCreate = true, .fTruncate = true});
    hfFile = nfr.hf; fMissing = nfr.fMissing;
    TEST_ASSERT(hfFile >= 3 && !fMissing);
    TEST_CHECK(CbFileSize(hfFile) == 0);
    CloseFile(hfFile);
    TEST_CHECK(RemoveNativeFile(szFile) == 0);
    OutputFileString(szFile, "new");
    nfr = HfOpenFile(szFile, {.mdShare = NATIVEFILESHARE::denyWrite});
    hfFile = nfr.hf; fMissing = nfr.fMissing;
    TEST_ASSERT(hfFile >= 3 && !fMissing);
    TEST_CHECK(CbReadFile(hfFile, std::as_writable_bytes(std::span{rgb}).first(static_cast<std::size_t>(3))) == 3 && !memcmp(rgb, "new", 3));
    CloseFile(hfFile);
    TEST_CHECK(RemoveNativeFile(szFile) == 0);
}
static void TestOutputFileStringSizes(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    char szDir[cbTestPath], szFile[cbTestPath + 64];

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("native-text-length", szDir, sizeof(szDir)));
    CchSnprintf(std::span{szFile}, "%s%stext.dat", szDir, szDirSep);
    for (std::size_t cch : {0u, 1u, 65535u, 65536u, 65537u, 131073u}) {
        const std::string sz(cch, 'a');
        OutputFileString(szFile, sz);
        TEST_CHECK(FNativeFileSize(szFile) == cch);
        OutputFileString(szFile, "tail");
        NATIVEFILEOWNER lpfile(PfNativeOpen(szFile, NATIVEFILEMODE::read), ICloseNativeFile);
        TEST_ASSERT(lpfile);
        std::string rgb(cch + 4, '\0');
        TEST_CHECK(CbReadNativeFile(lpfile.get(), std::as_writable_bytes(std::span{rgb})) == rgb.size());
        TEST_CHECK(rgb == sz + "tail");
        lpfile.reset();
        TEST_CHECK(RemoveNativeFile(szFile) == 0);
    }
    // The slice is not NUL-terminated; the adjacent guard must not be written.
    const char szSlice[] = {'x', '\xce', '\xa9', '!'};
    OutputFileString(szFile, std::string_view{szSlice, 3});
    OutputFileString(szFile, std::string_view{});
    NATIVEFILEOWNER lpfile(PfNativeOpen(szFile, NATIVEFILEMODE::read), ICloseNativeFile);
    TEST_ASSERT(lpfile);
    std::array<char, 4> rgb{};
    TEST_CHECK(FNativeFileSize(szFile) == 3);
    TEST_CHECK(CbReadNativeFile(lpfile.get(), std::as_writable_bytes(std::span{rgb})) == 3);
    TEST_CHECK(std::string_view(rgb.data(), 3) == std::string_view(szSlice, 3));
    lpfile.reset();
    TEST_CHECK(RemoveNativeFile(szFile) == 0);
}
static void TestNativeEmptyWrites(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    char szDir[cbTestPath], szFile[cbTestPath + 64];

    TEST_ASSERT(FStarsTestInit(app));
    TEST_ASSERT(FStarsTestDir("native-empty-write", szDir, sizeof(szDir)));
    CchSnprintf(std::span{szFile}, "%s%sempty.dat", szDir, szDirSep);
    for (bool fNative : {false, true}) {
        const auto nfo = NATIVEFILEOPENOPTIONS{.mdAccess = NATIVEFILEACCESS::readWrite, .fCreate = true, .fTruncate = true, .fNative = fNative};
        const auto nfr = HfOpenFile(szFile, nfo);
        TEST_ASSERT(nfr.hf >= 3);
        TEST_CHECK(CbWriteFile(nfr.hf, std::as_bytes(std::span{"abcdef"}).first(6)) == 6 && LSeekFile(nfr.hf, 2, NATIVESEEKORIGIN::begin) == 2);
        errno = 0;
        const auto cbEmpty = CbWriteFile(nfr.hf, std::span<const std::byte>{});
#ifdef _WIN32
        if (fNative) {
            TEST_CHECK(cbEmpty == 0 && CbFileSize(nfr.hf) == 2);
        } else {
            TEST_CHECK(!cbEmpty && errno == ENOTSUP && CbFileSize(nfr.hf) == 6);
        }
#else
        TEST_CHECK(cbEmpty == 0 && CbFileSize(nfr.hf) == 6);
#endif
        TEST_CHECK(LSeekFile(nfr.hf, 0, NATIVESEEKORIGIN::current) == 2);
        CloseFile(nfr.hf);
        TEST_CHECK(RemoveNativeFile(szFile) == 0);
    }
}
TEST_LIST = {{"text append preserves full byte lengths", TestOutputFileStringSizes}, {"native empty writes retain platform truncation semantics", TestNativeEmptyWrites}, {"native token read/update/create/append policies", TestNativeOpenModes}, {"native Unicode paths, backups and long Windows paths", TestNativeFiles}, {NULL, NULL}};
