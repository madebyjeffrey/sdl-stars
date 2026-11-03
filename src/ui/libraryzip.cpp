#include "win.h"
#include "ownership.h"
#include "libraryzip.h"
#include "encoding.h"
#include <limits>
#include <new>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <zlib.h>

struct ZIPFILES { std::vector<std::string> rgpsz; std::string_view pszRoot; std::int16_t fBad; };
struct ZIPENTRY { std::string_view pszName; std::uint32_t dwCrc = 0, cbSize = 0, dwOffset = 0; };
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

static std::int16_t FZipSafeName(std::string_view psz) {
    std::size_t ichPart = 0;
    if (psz.empty() || psz.front() == '/' || psz.front() == '\\') return 0;
    for (std::size_t i = 0; i < psz.size(); i++) {
        if (static_cast<unsigned char>(psz[i]) < 32 || psz[i] == '\\' || psz[i] == ':') return 0;
        if (psz[i] == '/') {
            const std::string_view szPart = psz.substr(ichPart, i - ichPart);
            if (szPart.empty() || szPart == "." || szPart == "..") return 0;
            ichPart = i + 1;
        }
    }
    const std::string_view szPart = psz.substr(ichPart);
    return !szPart.empty() && szPart != "." && szPart != "..";
}

static std::int16_t FZipNameUtf8Supported(std::string_view pszName, std::uint16_t wFlags) {
    if (!FUtf8Valid(pszName)) return 0;
    if (wFlags & ZIP_UTF8_FILENAME_FLAG) return 1;
    for (std::size_t iByte = 0; iByte < pszName.size(); iByte++)
        if (static_cast<unsigned char>(pszName[iByte]) >= 0x80) return 0;
    return 1;
}

static bool FCollectZipFiles(APPSESSION &app, ZIPFILES *pz, std::string_view pszName) try {
    std::optional<NATIVEPATHINFO> npiPath;
    char szPath[512];

    ZIPFILES zfChild{};
    if (pszName == "." || pszName == "..") return true;
    if (CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", pz->pszRoot, szDirSep, pszName) <= 0 ||
        !(npiPath = FNativePathInfo(szPath)) || npiPath->fLink) { pz->fBad = 1; return false; }
    if (npiPath->fDirectory) {
        /* Game export stages contain only regular files and backup directories. */
        zfChild.pszRoot = szPath;
        if (!FNativeEnumerateDirectory(app, szPath, FCollectZipFiles, &zfChild) || zfChild.fBad) pz->fBad = 1;
        for (const std::string &szChild : zfChild.rgpsz) {
            std::string szRelative;
            szRelative.reserve(pszName.size() + szChild.size() + 1);
            szRelative.append(pszName);
            szRelative.push_back('/');
            szRelative.append(szChild);
            pz->rgpsz.push_back(std::move(szRelative));
        }
        return !pz->fBad;
    }
    pz->rgpsz.emplace_back(pszName);
    return true;
} catch (const std::bad_alloc &) {
    pz->fBad = 1;
    errno = ENOMEM;
    return 0;
}

static std::int16_t FWriteZipEntry(NATIVEFILE *pfZip, std::string_view pszRoot, ZIPENTRY *pze) try {
    char szPath[512];
    std::optional<std::int64_t> cb, ibZip;
    std::vector<std::uint8_t> rgbData;
    NATIVEFILE *pf = nullptr;
    std::size_t cbName = pze->pszName.size();
    NATIVECLEANUPSCOPE entry([&]() noexcept {
        if (pf) ICloseNativeFile(pf);
    });
    if (CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", pszRoot, szDirSep, pze->pszName) <= 0 ||
        !(cb = FNativeFileSize(szPath)) || (*cb) < 0 || (*cb) > std::numeric_limits<std::uint32_t>::max() || cbName > std::numeric_limits<std::uint16_t>::max() ||
        !(ibZip = LNativePosition(pfZip)) || static_cast<std::uint64_t>(*ibZip) > std::numeric_limits<std::uint32_t>::max()) return 0;
    pf = PfOpenNativeFile(szPath, NATIVEFILEMODE::read);
    if (!pf) return 0;
    rgbData.resize(*cb != 0 ? static_cast<std::size_t>(*cb) : 1);
    if (CReadNative(std::span{rgbData}.first(static_cast<std::size_t>((*cb))), pf) != static_cast<std::size_t>((*cb))) return 0;
    if (ICloseNativeFile(std::exchange(pf, nullptr)) != 0) return 0;
    pze->dwCrc = static_cast<std::uint32_t>(crc32(0, rgbData.data(), static_cast<uInt>((*cb))));
    pze->cbSize = static_cast<std::uint32_t>((*cb));
    pze->dwOffset = static_cast<std::uint32_t>(*ibZip);
    PutZip32(pfZip, ZIP_LOCAL_SIGNATURE); PutZip16(pfZip, 20); PutZip16(pfZip, 0x800); PutZip16(pfZip, 0);
    PutZip16(pfZip, 0); PutZip16(pfZip, 0); PutZip32(pfZip, pze->dwCrc);
    PutZip32(pfZip, pze->cbSize); PutZip32(pfZip, pze->cbSize);
    PutZip16(pfZip, static_cast<std::uint16_t>(cbName)); PutZip16(pfZip, 0);
    if (CWriteNative(std::span{pze->pszName.data(), cbName}, pfZip) != cbName || CWriteNative(std::span{rgbData}.first(static_cast<std::size_t>((*cb))), pfZip) != static_cast<std::size_t>((*cb))) {
        return 0;
    }
    entry.Close();
    return !FNativeStreamError(pfZip);
} catch (const std::bad_alloc &) {
    errno = ENOMEM;
    return 0;
}

std::int16_t FZipLibraryDirectory(APPSESSION &app, std::string_view pszDirectory, std::string_view pszArchive) try {
    ZIPFILES zf{};
    std::vector<ZIPENTRY> rgzentry;
    char szTemp[512] = "";
    NATIVEFILE *pf = nullptr;
    std::uint32_t dwStart, dwLength;
    std::optional<std::int64_t> ib;

    std::int16_t fOk = 0;
    int cchTemp;
    std::size_t i, cbName;
    NATIVECLEANUPSCOPE archive([&]() noexcept {
        if (pf) ICloseNativeFile(pf);
        if (!fOk && szTemp[0]) RemoveNativePath(szTemp);
    });
    zf.pszRoot = pszDirectory;
    if (!FNativeEnumerateDirectory(app, pszDirectory, FCollectZipFiles, &zf) || zf.fBad || zf.rgpsz.size() > std::numeric_limits<std::uint16_t>::max()) goto done;
    for (i = 0; i < 1000; i++) {
        cchTemp = CchSnprintf(std::span{szTemp, sizeof(szTemp)}, "%s.tmp.%u", pszArchive, idZipTemp++);
        if (cchTemp <= 0 || static_cast<std::size_t>(cchTemp) >= sizeof(szTemp)) goto done;
        pf = PfOpenNativeFile(szTemp, NATIVEFILEMODE::writeExclusive);
        if (pf) break;
        if (!FNativeFileSize(szTemp)) goto done;
    }
    if (!pf) goto done;
    rgzentry.resize(zf.rgpsz.size());
    for (i = 0; i < zf.rgpsz.size(); i++) {
        rgzentry[i].pszName = zf.rgpsz[i];
        if (!FWriteZipEntry(pf, pszDirectory, &rgzentry[i])) goto done;
    }
    ib = LNativePosition(pf);
    if (!ib) goto done;
    dwStart = static_cast<std::uint32_t>(*ib);
    for (i = 0; i < zf.rgpsz.size(); i++) {
        cbName = rgzentry[i].pszName.size();
        PutZip32(pf, ZIP_CENTRAL_SIGNATURE); PutZip16(pf, 20); PutZip16(pf, 20); PutZip16(pf, 0x800);
        PutZip16(pf, 0); PutZip16(pf, 0); PutZip16(pf, 0); PutZip32(pf, rgzentry[i].dwCrc);
        PutZip32(pf, rgzentry[i].cbSize); PutZip32(pf, rgzentry[i].cbSize); PutZip16(pf, static_cast<std::uint16_t>(cbName));
        PutZip16(pf, 0); PutZip16(pf, 0); PutZip16(pf, 0); PutZip16(pf, 0);
        PutZip32(pf, 0); PutZip32(pf, rgzentry[i].dwOffset);
        if (CWriteNative(std::span{rgzentry[i].pszName.data(), cbName}, pf) != cbName) goto done;
    }
    ib = LNativePosition(pf);
    if (!ib) goto done;
    dwLength = static_cast<std::uint32_t>(*ib) - dwStart;
    PutZip32(pf, ZIP_EOCD_SIGNATURE); PutZip16(pf, 0); PutZip16(pf, 0);
    PutZip16(pf, static_cast<std::uint16_t>(zf.rgpsz.size())); PutZip16(pf, static_cast<std::uint16_t>(zf.rgpsz.size()));
    PutZip32(pf, dwLength); PutZip32(pf, dwStart); PutZip16(pf, 0);
    if (FNativeStreamError(pf)) goto done;
    if (ICloseNativeFile(pf) != 0) { pf = nullptr; goto done; }
    pf = nullptr;
    if (ReplaceNativeFile(szTemp, pszArchive) != 0) goto done;
    fOk = 1;
done:
    archive.Close();
    return fOk;
} catch (const std::bad_alloc &) {
    errno = ENOMEM;
    return 0;
}

static std::int16_t FZipMakeParents(std::string_view pszRoot, std::string_view pszName, std::span<char> szFile) {
    char szPath[512], ch;
    int cch;
    std::size_t i, cchBase = pszRoot.size() + std::string_view{szDirSep}.size();
    cch = CchSnprintf(szFile, "%s%s%s", pszRoot, szDirSep, pszName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= szFile.size() || static_cast<std::size_t>(cch) >= sizeof(szPath)) return 0;
    CchCopyUtf8(std::span{szPath}, std::string_view{szFile.data(), static_cast<std::size_t>(cch)});
    for (i = cchBase; i < static_cast<std::size_t>(cch); i++) if (szPath[i] == '/' || szPath[i] == '\\') {
        ch = szPath[i]; szPath[i] = 0;
        if (!FNativeDirectoryExists(szPath) && CreateNativeDir(szPath) != 0) return 0;
        szPath[i] = ch;
    }
    return 1;
}

std::int16_t FUnzipLibraryArchive(std::string_view pszArchive, std::string_view pszDirectory) try {
    std::uint8_t rgbTail[65557], rgbCentral[ZIP_CENTRAL_HEADER_SIZE], rgbLocal[ZIP_LOCAL_HEADER_SIZE];
    std::uint8_t rgbLocalName[512];
    NATIVEFILE *pf = PfOpenNativeFile(pszArchive, NATIVEFILEMODE::read);
    NATIVEFILE *pfOutput = nullptr;
    std::optional<std::int64_t> archiveSize;
    long tailOffset, eocd = -1, centralOffset, lNext;
    std::optional<std::int64_t> ib;
    std::uint16_t cEntries, i, wLocalNameLength, wLocalExtraLength;
    std::uint64_t cbTotal = 0;
    std::int16_t fOk = 0, fWrite, fUnsupportedName = 0;
    std::size_t cbRead;
    long lSearch;
    char szName[512], szOutput[512], szDirectoryEntry[512];
    ZIPCENTRAL zc;
    z_stream zs;

    std::int16_t fInflating = 0;
    NATIVECLEANUPSCOPE archive([&]() noexcept {
        if (fInflating) inflateEnd(&zs);
        if (pfOutput) ICloseNativeFile(pfOutput);
        if (pf) ICloseNativeFile(pf);
    });
    if (!pf || !(archiveSize = FNativeFileSize(pszArchive)) || (*archiveSize) < ZIP_EOCD_SIZE) goto done;
    tailOffset = (*archiveSize) > sizeof(rgbTail) ? static_cast<long>(((*archiveSize) - sizeof(rgbTail))) : 0;
    if (LSeekNativeFile(pf, tailOffset, NATIVESEEKORIGIN::begin) < 0) goto done;
    cbRead = CReadNative(std::span{rgbTail}, pf);
    lSearch = static_cast<long>((cbRead >= ZIP_EOCD_SIZE ? cbRead - ZIP_EOCD_SIZE : 0));
    for (;;) { if (DwZip32(rgbTail + lSearch) == ZIP_EOCD_SIGNATURE) { eocd = tailOffset + lSearch; break; } if (!lSearch--) break; }
    if (eocd < 0 || LSeekNativeFile(pf, eocd + 4, NATIVESEEKORIGIN::begin) < 0 ||
        CReadNative(std::span{rgbTail}.first(ZIP_EOCD_FIELDS_AFTER_SIGNATURE_SIZE), pf) != ZIP_EOCD_FIELDS_AFTER_SIGNATURE_SIZE ||
        eocd + ZIP_EOCD_SIZE + WZip16(rgbTail + ZIP_EOCD_COMMENT_LENGTH_OFFSET) != (*archiveSize)) goto done;
    if (WZip16(rgbTail + ZIP_EOCD_DISK_NUMBER_OFFSET) || WZip16(rgbTail + ZIP_EOCD_CENTRAL_DISK_OFFSET) ||
        WZip16(rgbTail + ZIP_EOCD_ENTRIES_ON_DISK_OFFSET) != WZip16(rgbTail + ZIP_EOCD_ENTRY_COUNT_OFFSET)) goto done;
    cEntries = WZip16(rgbTail + ZIP_EOCD_ENTRY_COUNT_OFFSET);
    centralOffset = static_cast<long>(DwZip32(rgbTail + ZIP_EOCD_CENTRAL_OFFSET_OFFSET));
    if (cEntries > 4096 || static_cast<std::uint64_t>(centralOffset) + DwZip32(rgbTail + ZIP_EOCD_CENTRAL_SIZE_OFFSET) > static_cast<std::uint64_t>(eocd) ||
        LSeekNativeFile(pf, centralOffset, NATIVESEEKORIGIN::begin) < 0) goto done;
    for (i = 0; i < cEntries; i++) {
        if (CReadNative(std::span{rgbCentral}, pf) != sizeof(rgbCentral) || DwZip32(rgbCentral) != ZIP_CENTRAL_SIGNATURE) goto done;
        zc = ZipCentralHeader(rgbCentral);
        if (!zc.wNameLength || zc.wNameLength >= sizeof(szName) || zc.cbUncompressed > 64U * 1024U * 1024U ||
            (zc.wFlags & 1) || (zc.wMethod != 0 && zc.wMethod != 8) ||
            WZip16(rgbCentral + ZIP_CENTRAL_DISK_START_OFFSET) != 0 ||
            ((zc.dwExternalAttributes >> 16) & 0170000U) == 0120000U ||
            (zc.dwExternalAttributes & 0x400U) ||
            CReadNative(std::span{szName}.first(zc.wNameLength), pf) != zc.wNameLength) goto done;
        if (std::memchr(szName, 0, zc.wNameLength)) goto done;
        std::string_view szEntry{szName, zc.wNameLength};
        if (!FZipNameUtf8Supported(szEntry, zc.wFlags)) {
            fUnsupportedName = 1;
            goto done;
        }
        cbTotal += zc.cbUncompressed;
        if (cbTotal > 512U * 1024U * 1024U) goto done;
        ib = LNativePosition(pf);
        if (!ib) goto done;
        lNext = static_cast<long>(*ib + zc.wExtraLength + zc.wCommentLength);
        if (zc.dwLocalOffset >= static_cast<std::uint32_t>(centralOffset) ||
            LSeekNativeFile(pf, zc.dwLocalOffset, NATIVESEEKORIGIN::begin) < 0 || CReadNative(std::span{rgbLocal}, pf) != sizeof(rgbLocal) ||
            DwZip32(rgbLocal) != ZIP_LOCAL_SIGNATURE || WZip16(rgbLocal + ZIP_LOCAL_FLAGS_OFFSET) != zc.wFlags ||
            WZip16(rgbLocal + ZIP_LOCAL_METHOD_OFFSET) != zc.wMethod) goto done;
        wLocalNameLength = WZip16(rgbLocal + ZIP_LOCAL_NAME_LENGTH_OFFSET);
        wLocalExtraLength = WZip16(rgbLocal + ZIP_LOCAL_EXTRA_LENGTH_OFFSET);
        if (wLocalNameLength != zc.wNameLength) goto done;
        if (CReadNative(std::span{rgbLocalName}.first(wLocalNameLength), pf) != wLocalNameLength ||
            std::memcmp(rgbLocalName, szName, zc.wNameLength) != 0 ||
            LSeekNativeFile(pf, wLocalExtraLength, NATIVESEEKORIGIN::current) < 0) goto done;
        ib = LNativePosition(pf);
        if (!ib || static_cast<std::uint64_t>(*ib) + zc.cbCompressed > static_cast<std::uint64_t>(centralOffset)) goto done;
        if (szEntry.back() == '/') {
            szEntry.remove_suffix(1);
            if (!FZipSafeName(szEntry) || CchSnprintf(std::span{szDirectoryEntry, sizeof(szDirectoryEntry)}, "%s/.zip-dir-entry", szEntry) <= 0 ||
                !FZipMakeParents(pszDirectory, szDirectoryEntry, std::span{szOutput}) ||
                LSeekNativeFile(pf, lNext, NATIVESEEKORIGIN::begin) < 0) goto done;
            continue;
        }
        if (!FZipSafeName(szEntry)) goto done;
        std::vector<std::uint8_t> rgbCompressed(zc.cbCompressed ? zc.cbCompressed : 1);
        std::vector<std::uint8_t> rgbData(zc.cbUncompressed ? zc.cbUncompressed : 1);
        if (CReadNative(std::span{rgbCompressed}.first(zc.cbCompressed), pf) != zc.cbCompressed) goto done;
        if (zc.wMethod == 0) {
            if (zc.cbCompressed != zc.cbUncompressed) goto done;
            std::memcpy(rgbData.data(), rgbCompressed.data(), zc.cbUncompressed);
        } else {
            zs = {};
            zs.next_in = rgbCompressed.data(); zs.avail_in = zc.cbCompressed;
            zs.next_out = rgbData.data(); zs.avail_out = zc.cbUncompressed;
            if (inflateInit2(&zs, -MAX_WBITS) != Z_OK) goto done;
            fInflating = 1;
            if (inflate(&zs, Z_FINISH) != Z_STREAM_END || zs.total_in != zc.cbCompressed ||
                zs.total_out != zc.cbUncompressed) { inflateEnd(&zs); fInflating = 0; goto done; }
            inflateEnd(&zs); fInflating = 0;
        }
        if (static_cast<std::uint32_t>(crc32(0, rgbData.data(), zc.cbUncompressed)) != zc.dwCrc ||
            !FZipMakeParents(pszDirectory, szEntry, std::span{szOutput}) ||
            FNativeFileSize(szOutput)) goto done;
        pfOutput = PfOpenNativeFile(szOutput, NATIVEFILEMODE::writeExclusive);
        fWrite = pfOutput && CWriteNative(std::span{rgbData}.first(zc.cbUncompressed), pfOutput) == zc.cbUncompressed;
        if (pfOutput && ICloseNativeFile(pfOutput) != 0) fWrite = 0;
        pfOutput = nullptr;
        if (!fWrite) goto done;
        if (LSeekNativeFile(pf, lNext, NATIVESEEKORIGIN::begin) < 0) goto done;
    }
    fOk = 1;
done:
    archive.Close();
    if (fUnsupportedName) return ZIP_IMPORT_UNSUPPORTED_NAME;
    return fOk ? ZIP_IMPORT_OK : ZIP_IMPORT_INVALID;
} catch (const std::bad_alloc &) {
    errno = ENOMEM;
    return ZIP_IMPORT_INVALID;
}
