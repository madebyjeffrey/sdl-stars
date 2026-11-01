#include "win.h"
#include "ownership.h"
#include "libraryzip.h"
#include "encoding.h"
#include <SDL3/SDL.h>
#include <zlib.h>

struct ZIPFILES { char **rgpsz; int c, cMax; const char *pszRoot; std::int16_t fBad; };
struct ZIPENTRY { char *pszName; std::uint32_t dwCrc, cbSize, dwOffset; };
struct ZIPCENTRAL {
    std::uint16_t wFlags, wMethod, wNameLength, wExtraLength, wCommentLength;
    std::uint32_t dwCrc, cbCompressed, cbUncompressed, dwExternalAttributes, dwLocalOffset;
};
enum {
    ZIP_EOCD_SIGNATURE = 0x06054b50,
    ZIP_CENTRAL_SIGNATURE = 0x02014b50,
    ZIP_LOCAL_SIGNATURE = 0x04034b50,
    ZIP_EOCD_DISK_NUMBER_OFFSET = 0,
    ZIP_EOCD_CENTRAL_DISK_OFFSET = 2,
    ZIP_EOCD_ENTRIES_ON_DISK_OFFSET = 4,
    ZIP_EOCD_ENTRY_COUNT_OFFSET = 6,
    ZIP_EOCD_CENTRAL_SIZE_OFFSET = 8,
    ZIP_EOCD_CENTRAL_OFFSET_OFFSET = 12,
    ZIP_EOCD_COMMENT_LENGTH_OFFSET = 16,
    ZIP_EOCD_SIZE = 22,
    ZIP_EOCD_FIELDS_AFTER_SIGNATURE_SIZE = 18,
    ZIP_LOCAL_HEADER_SIZE = 30,
    ZIP_CENTRAL_HEADER_SIZE = 46,
    ZIP_CENTRAL_FLAGS_OFFSET = 8,
    ZIP_CENTRAL_METHOD_OFFSET = 10,
    ZIP_CENTRAL_CRC_OFFSET = 16,
    ZIP_CENTRAL_COMPRESSED_SIZE_OFFSET = 20,
    ZIP_CENTRAL_UNCOMPRESSED_SIZE_OFFSET = 24,
    ZIP_CENTRAL_NAME_LENGTH_OFFSET = 28,
    ZIP_CENTRAL_EXTRA_LENGTH_OFFSET = 30,
    ZIP_CENTRAL_COMMENT_LENGTH_OFFSET = 32,
    ZIP_CENTRAL_DISK_START_OFFSET = 34,
    ZIP_CENTRAL_EXTERNAL_ATTRIBUTES_OFFSET = 38,
    ZIP_CENTRAL_LOCAL_OFFSET_OFFSET = 42,
    ZIP_LOCAL_FLAGS_OFFSET = 6,
    ZIP_LOCAL_METHOD_OFFSET = 8,
    ZIP_LOCAL_NAME_LENGTH_OFFSET = 26,
    ZIP_LOCAL_EXTRA_LENGTH_OFFSET = 28,
    ZIP_UTF8_FILENAME_FLAG = 0x0800
};
static std::uint32_t idZipTemp;

static std::uint16_t WZip16(const std::uint8_t *p) { return static_cast<std::uint16_t>((p[0] | (static_cast<std::uint16_t>(p[1]) << 8))); }
static std::uint32_t DwZip32(const std::uint8_t *p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) | (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}
static ZIPCENTRAL ZipCentralHeader(const std::uint8_t *rgbHeader) {
    ZIPCENTRAL zc;
    zc.wFlags = WZip16(rgbHeader + ZIP_CENTRAL_FLAGS_OFFSET);
    zc.wMethod = WZip16(rgbHeader + ZIP_CENTRAL_METHOD_OFFSET);
    zc.dwCrc = DwZip32(rgbHeader + ZIP_CENTRAL_CRC_OFFSET);
    zc.cbCompressed = DwZip32(rgbHeader + ZIP_CENTRAL_COMPRESSED_SIZE_OFFSET);
    zc.cbUncompressed = DwZip32(rgbHeader + ZIP_CENTRAL_UNCOMPRESSED_SIZE_OFFSET);
    zc.wNameLength = WZip16(rgbHeader + ZIP_CENTRAL_NAME_LENGTH_OFFSET);
    zc.wExtraLength = WZip16(rgbHeader + ZIP_CENTRAL_EXTRA_LENGTH_OFFSET);
    zc.wCommentLength = WZip16(rgbHeader + ZIP_CENTRAL_COMMENT_LENGTH_OFFSET);
    zc.dwExternalAttributes = DwZip32(rgbHeader + ZIP_CENTRAL_EXTERNAL_ATTRIBUTES_OFFSET);
    zc.dwLocalOffset = DwZip32(rgbHeader + ZIP_CENTRAL_LOCAL_OFFSET_OFFSET);
    return zc;
}
static void PutZip16(NATIVEFILE *pf, std::uint16_t w) { ChPutNativeFile(w & 255, pf); ChPutNativeFile(w >> 8, pf); }
static void PutZip32(NATIVEFILE *pf, std::uint32_t dw) { PutZip16(pf, static_cast<std::uint16_t>(dw)); PutZip16(pf, static_cast<std::uint16_t>((dw >> 16))); }

static std::int16_t FZipSafeName(const char *psz) {
    const char *pszCurrent = psz, *pszPart = psz;
    if (!*psz || *psz == '/' || *psz == '\\') return 0;
    for (; *pszCurrent; pszCurrent++) {
        if (static_cast<unsigned char>(*pszCurrent) < 32 || *pszCurrent == '\\' || *pszCurrent == ':') return 0;
        if (*pszCurrent == '/') {
            if (pszCurrent == pszPart || (pszCurrent - pszPart == 1 && pszPart[0] == '.') ||
                (pszCurrent - pszPart == 2 && pszPart[0] == '.' && pszPart[1] == '.')) return 0;
            pszPart = pszCurrent + 1;
        }
    }
    return pszCurrent != pszPart && !(pszCurrent - pszPart == 1 && pszPart[0] == '.') &&
           !(pszCurrent - pszPart == 2 && pszPart[0] == '.' && pszPart[1] == '.');
}

static std::int16_t FZipNameUtf8Supported(const char *pszName, std::size_t cbName, std::uint16_t wFlags) {
    std::size_t iByte;
    if (!FUtf8Valid(pszName, cbName)) return 0;
    if (wFlags & ZIP_UTF8_FILENAME_FLAG) return 1;
    for (iByte = 0; iByte < cbName; iByte++)
        if (static_cast<unsigned char>(pszName[iByte]) >= 0x80) return 0;
    return 1;
}

static std::int16_t FZipEnsureCapacity(ZIPFILES *pz) {
    char **rgpszFiles;
    int cCapacity;
    if (pz->c < pz->cMax) return 1;
    cCapacity = pz->cMax ? pz->cMax * 2 : 16;
    rgpszFiles = static_cast<char **>(SDL_realloc(pz->rgpsz, static_cast<std::size_t>(cCapacity) * sizeof(*rgpszFiles)));
    if (!rgpszFiles) { pz->fBad = 1; return 0; }
    pz->rgpsz = LpStartHeapArray<char *>(rgpszFiles,static_cast<std::size_t>(cCapacity) * sizeof(*rgpszFiles));
    pz->cMax = cCapacity;
    return 1;
}

static std::int16_t FCollectZipFiles(APPSESSION &app, void *lpData, const char *pszName) {
    ZIPFILES *pz = static_cast<ZIPFILES *>(lpData);
    char szPath[512], *pszRelative;
    std::int16_t fDir, fLink;
    ZIPFILES zfChild = {nullptr};
    int i;
    std::size_t cbName;
    NATIVECLEANUPSCOPE childFiles([&]() noexcept {
        for (int j = 0; j < zfChild.c; j++) SDL_free(zfChild.rgpsz[j]);
        SDL_free(zfChild.rgpsz);
    });
    if (!std::strcmp(pszName, ".") || !std::strcmp(pszName, "..")) return 1;
    if (CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", pz->pszRoot, szDirSep, pszName) <= 0 ||
        !FNativePathInfo(szPath, &fDir, &fLink) || fLink) { pz->fBad = 1; return 0; }
    if (fDir) {
        /* Game export stages contain only regular files and backup directories. */
        zfChild.pszRoot = szPath;
        if (!FNativeEnumerateDirectory(app, szPath, FCollectZipFiles, &zfChild) || zfChild.fBad) pz->fBad = 1;
        for (i = 0; i < zfChild.c; i++) {
            cbName = std::strlen(pszName) + std::strlen(zfChild.rgpsz[i]) + 2;
            NATIVEOWNER<char, SDL_free> relative(static_cast<char *>(SDL_malloc(cbName)));
            pszRelative = relative.get();
            if (!pszRelative) { pz->fBad = 1; break; }
            CchSnprintf(std::span{pszRelative, cbName}, "%s/%s", pszName, zfChild.rgpsz[i]);
            if (!FZipEnsureCapacity(pz)) break;
            pz->rgpsz[pz->c++] = relative.release();
        }
        childFiles.Close();
        return !pz->fBad;
    }
    if (!FZipEnsureCapacity(pz)) return 0;
    pz->rgpsz[pz->c] = SDL_strdup(pszName);
    if (!pz->rgpsz[pz->c]) { pz->fBad = 1; return 0; }
    pz->c++;
    return 1;
}

static std::int16_t FWriteZipEntry(NATIVEFILE *pfZip, const char *pszRoot, ZIPENTRY *pze) {
    char szPath[512];
    std::int64_t cb;
    std::uint8_t *rgbData = nullptr;
    NATIVEFILE *pf = nullptr;
    std::size_t cbName = std::strlen(pze->pszName);
    NATIVECLEANUPSCOPE entry([&]() noexcept {
        if (pf) ICloseNativeFile(pf);
        SDL_free(rgbData);
    });
    if (CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", pszRoot, szDirSep, pze->pszName) <= 0 ||
        !FNativeFileSize(szPath, &cb) || cb < 0 || cb > UINT32_MAX || cbName > UINT16_MAX ||
        static_cast<std::uint64_t>(LNativePosition(pfZip)) > UINT32_MAX) return 0;
    pf = PfOpenNativeFile(szPath, "rb");
    if (!pf) return 0;
    rgbData = static_cast<std::uint8_t *>(SDL_malloc(cb ? static_cast<std::size_t>(cb) : 1));
    if (!rgbData) return 0;
    if (CReadNative(rgbData, 1, static_cast<std::size_t>(cb), pf) != static_cast<std::size_t>(cb)) return 0;
    if (ICloseNativeFile(std::exchange(pf, nullptr)) != 0) return 0;
    pze->dwCrc = static_cast<std::uint32_t>(crc32(0, rgbData, static_cast<uInt>(cb)));
    pze->cbSize = static_cast<std::uint32_t>(cb);
    pze->dwOffset = static_cast<std::uint32_t>(LNativePosition(pfZip));
    PutZip32(pfZip, ZIP_LOCAL_SIGNATURE); PutZip16(pfZip, 20); PutZip16(pfZip, 0x800); PutZip16(pfZip, 0);
    PutZip16(pfZip, 0); PutZip16(pfZip, 0); PutZip32(pfZip, pze->dwCrc);
    PutZip32(pfZip, pze->cbSize); PutZip32(pfZip, pze->cbSize);
    PutZip16(pfZip, static_cast<std::uint16_t>(cbName)); PutZip16(pfZip, 0);
    if (CWriteNative(pze->pszName, 1, cbName, pfZip) != cbName || CWriteNative(rgbData, 1, static_cast<std::size_t>(cb), pfZip) != static_cast<std::size_t>(cb)) {
        return 0;
    }
    entry.Close();
    return !FNativeStreamError(pfZip);
}

std::int16_t FZipLibraryDirectory(APPSESSION &app, const char *pszDirectory, const char *pszArchive) {
    ZIPFILES zf = {nullptr};
    ZIPENTRY *rgzentry = nullptr;
    char szTemp[512] = "";
    NATIVEFILE *pf = nullptr;
    std::uint32_t dwStart, dwLength;
    std::int64_t cbExisting;
    std::int16_t fOk = 0;
    int i, cchTemp;
    std::size_t cbName;
    NATIVECLEANUPSCOPE archive([&]() noexcept {
        if (pf) ICloseNativeFile(pf);
        if (!fOk && szTemp[0]) RemoveNativePath(szTemp);
        for (i = 0; i < zf.c; i++) { SDL_free(zf.rgpsz[i]); if (rgzentry) SDL_free(rgzentry[i].pszName); }
        SDL_free(zf.rgpsz); SDL_free(rgzentry);
    });
    zf.pszRoot = pszDirectory;
    if (!FNativeEnumerateDirectory(app, pszDirectory, FCollectZipFiles, &zf) || zf.fBad || zf.c > UINT16_MAX) goto done;
    for (i = 0; i < 1000; i++) {
        cchTemp = CchSnprintf(std::span{szTemp, sizeof(szTemp)}, "%s.tmp.%u", pszArchive, idZipTemp++);
        if (cchTemp <= 0 || static_cast<std::size_t>(cchTemp) >= sizeof(szTemp)) goto done;
        pf = PfOpenNativeFile(szTemp, "wbx");
        if (pf) break;
        if (!FNativeFileSize(szTemp, &cbExisting)) goto done;
    }
    if (!pf) goto done;
    rgzentry = static_cast<ZIPENTRY *>(SDL_calloc(zf.c ? static_cast<std::size_t>(zf.c) : 1, sizeof(*rgzentry)));
    if (!rgzentry) goto done;
    rgzentry = LpStartHeapArray<ZIPENTRY>(rgzentry,(zf.c ? static_cast<std::size_t>(zf.c) : 1) * sizeof(*rgzentry));
    for (i = 0; i < zf.c; i++) {
        rgzentry[i].pszName = SDL_strdup(zf.rgpsz[i]);
        if (!rgzentry[i].pszName || !FWriteZipEntry(pf, pszDirectory, &rgzentry[i])) goto done;
    }
    dwStart = static_cast<std::uint32_t>(LNativePosition(pf));
    for (i = 0; i < zf.c; i++) {
        cbName = std::strlen(rgzentry[i].pszName);
        PutZip32(pf, ZIP_CENTRAL_SIGNATURE); PutZip16(pf, 20); PutZip16(pf, 20); PutZip16(pf, 0x800);
        PutZip16(pf, 0); PutZip16(pf, 0); PutZip16(pf, 0); PutZip32(pf, rgzentry[i].dwCrc);
        PutZip32(pf, rgzentry[i].cbSize); PutZip32(pf, rgzentry[i].cbSize); PutZip16(pf, static_cast<std::uint16_t>(cbName));
        PutZip16(pf, 0); PutZip16(pf, 0); PutZip16(pf, 0); PutZip16(pf, 0);
        PutZip32(pf, 0); PutZip32(pf, rgzentry[i].dwOffset);
        if (CWriteNative(rgzentry[i].pszName, 1, cbName, pf) != cbName) goto done;
    }
    dwLength = static_cast<std::uint32_t>(LNativePosition(pf)) - dwStart;
    PutZip32(pf, ZIP_EOCD_SIGNATURE); PutZip16(pf, 0); PutZip16(pf, 0);
    PutZip16(pf, static_cast<std::uint16_t>(zf.c)); PutZip16(pf, static_cast<std::uint16_t>(zf.c));
    PutZip32(pf, dwLength); PutZip32(pf, dwStart); PutZip16(pf, 0);
    if (FNativeStreamError(pf)) goto done;
    if (ICloseNativeFile(pf) != 0) { pf = nullptr; goto done; }
    pf = nullptr;
    if (ReplaceNativeFile(szTemp, pszArchive) != 0) goto done;
    fOk = 1;
done:
    archive.Close();
    return fOk;
}

static std::int16_t FZipMakeParents(const char *pszRoot, const char *pszName, char *szFile, std::size_t cbFile) {
    char szPath[512], ch;
    int cch;
    std::size_t i, cchBase = std::strlen(pszRoot) + std::strlen(szDirSep);
    cch = CchSnprintf(std::span{szFile, cbFile}, "%s%s%s", pszRoot, szDirSep, pszName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= cbFile || static_cast<std::size_t>(cch) >= sizeof(szPath)) return 0;
    std::strcpy(szPath, szFile);
    for (i = cchBase; i < static_cast<std::size_t>(cch); i++) if (szPath[i] == '/' || szPath[i] == '\\') {
        ch = szPath[i]; szPath[i] = 0;
        if (!FNativeDirectoryExists(szPath) && CreateNativeDir(szPath) != 0) return 0;
        szPath[i] = ch;
    }
    return 1;
}

std::int16_t FUnzipLibraryArchive(const char *pszArchive, const char *pszDirectory) {
    std::uint8_t rgbTail[65557], rgbCentral[ZIP_CENTRAL_HEADER_SIZE], rgbLocal[ZIP_LOCAL_HEADER_SIZE];
    std::uint8_t *rgbLocalName = nullptr, *rgbCompressed = nullptr, *rgbData = nullptr;
    NATIVEFILE *pf = PfOpenNativeFile(pszArchive, "rb");
    NATIVEFILE *pfOutput = nullptr;
    std::int64_t archiveSize;
    long tailOffset, eocd = -1, centralOffset, lNext;
    std::uint16_t cEntries, i, wLocalNameLength, wLocalExtraLength;
    std::uint64_t cbTotal = 0;
    std::int16_t fOk = 0, fWrite, fUnsupportedName = 0;
    std::size_t cbRead;
    long lSearch;
    char szName[512], szOutput[512], szDirectoryEntry[512];
    ZIPCENTRAL zc;
    z_stream zs;
    std::int64_t cbExisting;
    std::int16_t fInflating = 0;
    NATIVECLEANUPSCOPE archive([&]() noexcept {
        if (fInflating) inflateEnd(&zs);
        SDL_free(rgbLocalName); SDL_free(rgbCompressed); SDL_free(rgbData);
        if (pfOutput) ICloseNativeFile(pfOutput);
        if (pf) ICloseNativeFile(pf);
    });
    if (!pf || !FNativeFileSize(pszArchive, &archiveSize) || archiveSize < ZIP_EOCD_SIZE) goto done;
    tailOffset = archiveSize > sizeof(rgbTail) ? static_cast<long>((archiveSize - sizeof(rgbTail))) : 0;
    if (ISeekNativeFile(pf, tailOffset, NATIVESEEKORIGIN::begin) != 0) goto done;
    cbRead = CReadNative(rgbTail, 1, sizeof(rgbTail), pf);
    lSearch = static_cast<long>((cbRead >= ZIP_EOCD_SIZE ? cbRead - ZIP_EOCD_SIZE : 0));
    for (;;) { if (DwZip32(rgbTail + lSearch) == ZIP_EOCD_SIGNATURE) { eocd = tailOffset + lSearch; break; } if (!lSearch--) break; }
    if (eocd < 0 || ISeekNativeFile(pf, eocd + 4, NATIVESEEKORIGIN::begin) != 0 ||
        CReadNative(rgbTail, 1, ZIP_EOCD_FIELDS_AFTER_SIGNATURE_SIZE, pf) != ZIP_EOCD_FIELDS_AFTER_SIGNATURE_SIZE ||
        eocd + ZIP_EOCD_SIZE + WZip16(rgbTail + ZIP_EOCD_COMMENT_LENGTH_OFFSET) != archiveSize) goto done;
    if (WZip16(rgbTail + ZIP_EOCD_DISK_NUMBER_OFFSET) || WZip16(rgbTail + ZIP_EOCD_CENTRAL_DISK_OFFSET) ||
        WZip16(rgbTail + ZIP_EOCD_ENTRIES_ON_DISK_OFFSET) != WZip16(rgbTail + ZIP_EOCD_ENTRY_COUNT_OFFSET)) goto done;
    cEntries = WZip16(rgbTail + ZIP_EOCD_ENTRY_COUNT_OFFSET);
    centralOffset = static_cast<long>(DwZip32(rgbTail + ZIP_EOCD_CENTRAL_OFFSET_OFFSET));
    if (cEntries > 4096 || static_cast<std::uint64_t>(centralOffset) + DwZip32(rgbTail + ZIP_EOCD_CENTRAL_SIZE_OFFSET) > static_cast<std::uint64_t>(eocd) ||
        ISeekNativeFile(pf, centralOffset, NATIVESEEKORIGIN::begin) != 0) goto done;
    for (i = 0; i < cEntries; i++) {
        if (CReadNative(rgbCentral, 1, sizeof(rgbCentral), pf) != sizeof(rgbCentral) || DwZip32(rgbCentral) != ZIP_CENTRAL_SIGNATURE) goto done;
        zc = ZipCentralHeader(rgbCentral);
        if (!zc.wNameLength || zc.wNameLength >= sizeof(szName) || zc.cbUncompressed > 64U * 1024U * 1024U ||
            (zc.wFlags & 1) || (zc.wMethod != 0 && zc.wMethod != 8) ||
            WZip16(rgbCentral + ZIP_CENTRAL_DISK_START_OFFSET) != 0 ||
            ((zc.dwExternalAttributes >> 16) & 0170000U) == 0120000U ||
            (zc.dwExternalAttributes & 0x400U) ||
            CReadNative(szName, 1, zc.wNameLength, pf) != zc.wNameLength) goto done;
        if (std::memchr(szName, 0, zc.wNameLength)) goto done;
        if (!FZipNameUtf8Supported(szName, zc.wNameLength, zc.wFlags)) {
            fUnsupportedName = 1;
            goto done;
        }
        cbTotal += zc.cbUncompressed;
        if (cbTotal > 512U * 1024U * 1024U) goto done;
        szName[zc.wNameLength] = 0;
        lNext = LNativePosition(pf) + zc.wExtraLength + zc.wCommentLength;
        if (zc.dwLocalOffset >= static_cast<std::uint32_t>(centralOffset) ||
            ISeekNativeFile(pf, zc.dwLocalOffset, NATIVESEEKORIGIN::begin) != 0 || CReadNative(rgbLocal, 1, sizeof(rgbLocal), pf) != sizeof(rgbLocal) ||
            DwZip32(rgbLocal) != ZIP_LOCAL_SIGNATURE || WZip16(rgbLocal + ZIP_LOCAL_FLAGS_OFFSET) != zc.wFlags ||
            WZip16(rgbLocal + ZIP_LOCAL_METHOD_OFFSET) != zc.wMethod) goto done;
        wLocalNameLength = WZip16(rgbLocal + ZIP_LOCAL_NAME_LENGTH_OFFSET);
        wLocalExtraLength = WZip16(rgbLocal + ZIP_LOCAL_EXTRA_LENGTH_OFFSET);
        if (wLocalNameLength != zc.wNameLength) goto done;
        rgbLocalName = static_cast<std::uint8_t *>(SDL_malloc(wLocalNameLength ? wLocalNameLength : 1));
        if (!rgbLocalName || CReadNative(rgbLocalName, 1, wLocalNameLength, pf) != wLocalNameLength ||
            std::memcmp(rgbLocalName, szName, zc.wNameLength) != 0 ||
            ISeekNativeFile(pf, wLocalExtraLength, NATIVESEEKORIGIN::current) != 0 ||
            static_cast<std::uint64_t>(LNativePosition(pf)) + zc.cbCompressed > static_cast<std::uint64_t>(centralOffset)) goto done;
        SDL_free(rgbLocalName); rgbLocalName = nullptr;
        if (szName[zc.wNameLength - 1] == '/') {
            szName[zc.wNameLength - 1] = 0;
            if (!FZipSafeName(szName) || CchSnprintf(std::span{szDirectoryEntry, sizeof(szDirectoryEntry)}, "%s/.zip-dir-entry", szName) <= 0 ||
                !FZipMakeParents(pszDirectory, szDirectoryEntry, szOutput, sizeof(szOutput)) ||
                ISeekNativeFile(pf, lNext, NATIVESEEKORIGIN::begin) != 0) goto done;
            continue;
        }
        if (!FZipSafeName(szName)) goto done;
        rgbCompressed = static_cast<std::uint8_t *>(SDL_malloc(zc.cbCompressed ? zc.cbCompressed : 1));
        rgbData = static_cast<std::uint8_t *>(SDL_malloc(zc.cbUncompressed ? zc.cbUncompressed : 1));
        if (!rgbCompressed || !rgbData || CReadNative(rgbCompressed, 1, zc.cbCompressed, pf) != zc.cbCompressed) goto done;
        if (zc.wMethod == 0) {
            if (zc.cbCompressed != zc.cbUncompressed) goto done;
            std::memcpy(rgbData, rgbCompressed, zc.cbUncompressed);
        } else {
            zs = {};
            zs.next_in = rgbCompressed; zs.avail_in = zc.cbCompressed;
            zs.next_out = rgbData; zs.avail_out = zc.cbUncompressed;
            if (inflateInit2(&zs, -MAX_WBITS) != Z_OK) goto done;
            fInflating = 1;
            if (inflate(&zs, Z_FINISH) != Z_STREAM_END || zs.total_in != zc.cbCompressed ||
                zs.total_out != zc.cbUncompressed) { inflateEnd(&zs); fInflating = 0; goto done; }
            inflateEnd(&zs); fInflating = 0;
        }
        if (static_cast<std::uint32_t>(crc32(0, rgbData, zc.cbUncompressed)) != zc.dwCrc ||
            !FZipMakeParents(pszDirectory, szName, szOutput, sizeof(szOutput)) ||
            FNativeFileSize(szOutput, &cbExisting)) goto done;
        pfOutput = PfOpenNativeFile(szOutput, "wbx");
        fWrite = pfOutput && CWriteNative(rgbData, 1, zc.cbUncompressed, pfOutput) == zc.cbUncompressed;
        if (pfOutput && ICloseNativeFile(pfOutput) != 0) fWrite = 0;
        pfOutput = nullptr;
        if (!fWrite) goto done;
        SDL_free(rgbCompressed); rgbCompressed = nullptr; SDL_free(rgbData); rgbData = nullptr;
        if (ISeekNativeFile(pf, lNext, NATIVESEEKORIGIN::begin) != 0) goto done;
    }
    fOk = 1;
done:
    archive.Close();
    if (fUnsupportedName) return ZIP_IMPORT_UNSUPPORTED_NAME;
    return fOk ? ZIP_IMPORT_OK : ZIP_IMPORT_INVALID;
}
