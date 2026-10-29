#include "win.h"
#include "libraryzip.h"
#include "encoding.h"
#include <SDL3/SDL.h>
#include <zlib.h>

typedef struct { char **rgpsz; int c, cMax; const char *pszRoot; int16_t fBad; } ZIPFILES;
typedef struct { char *pszName; uint32_t dwCrc, cbSize, dwOffset; } ZIPENTRY;
typedef struct {
    uint16_t wFlags, wMethod, wNameLength, wExtraLength, wCommentLength;
    uint32_t dwCrc, cbCompressed, cbUncompressed, dwExternalAttributes, dwLocalOffset;
} ZIPCENTRAL;
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
static uint32_t idZipTemp;

static uint16_t WZip16(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static uint32_t DwZip32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static ZIPCENTRAL ZipCentralHeader(const uint8_t *rgbHeader) {
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
static void PutZip16(NATIVEFILE *pf, uint16_t w) { ChPutNativeFile(w & 255, pf); ChPutNativeFile(w >> 8, pf); }
static void PutZip32(NATIVEFILE *pf, uint32_t dw) { PutZip16(pf, (uint16_t)dw); PutZip16(pf, (uint16_t)(dw >> 16)); }

static int16_t FZipSafeName(const char *psz) {
    const char *pszCurrent = psz, *pszPart = psz;
    if (!*psz || *psz == '/' || *psz == '\\') return FALSE;
    for (; *pszCurrent; pszCurrent++) {
        if ((unsigned char)*pszCurrent < 32 || *pszCurrent == '\\' || *pszCurrent == ':') return FALSE;
        if (*pszCurrent == '/') {
            if (pszCurrent == pszPart || (pszCurrent - pszPart == 1 && pszPart[0] == '.') ||
                (pszCurrent - pszPart == 2 && pszPart[0] == '.' && pszPart[1] == '.')) return FALSE;
            pszPart = pszCurrent + 1;
        }
    }
    return pszCurrent != pszPart && !(pszCurrent - pszPart == 1 && pszPart[0] == '.') &&
           !(pszCurrent - pszPart == 2 && pszPart[0] == '.' && pszPart[1] == '.');
}

static int16_t FZipNameUtf8Supported(const char *pszName, size_t cbName, uint16_t wFlags) {
    size_t iByte;
    if (!FUtf8Valid(pszName, cbName)) return FALSE;
    if (wFlags & ZIP_UTF8_FILENAME_FLAG) return TRUE;
    for (iByte = 0; iByte < cbName; iByte++)
        if ((unsigned char)pszName[iByte] >= 0x80) return FALSE;
    return TRUE;
}

static int16_t FZipEnsureCapacity(ZIPFILES *pz) {
    char **rgpszFiles;
    int cCapacity;
    if (pz->c < pz->cMax) return TRUE;
    cCapacity = pz->cMax ? pz->cMax * 2 : 16;
    rgpszFiles = (char **)SDL_realloc(pz->rgpsz, (size_t)cCapacity * sizeof(*rgpszFiles));
    if (!rgpszFiles) { pz->fBad = TRUE; return FALSE; }
    pz->rgpsz = rgpszFiles;
    pz->cMax = cCapacity;
    return TRUE;
}

static int16_t FCollectZipFiles(void *lpData, const char *pszName) {
    ZIPFILES *pz = (ZIPFILES *)lpData;
    char szPath[512], *pszRelative;
    int16_t fDir, fLink;
    ZIPFILES zfChild = {0};
    int i;
    size_t cbName;
    if (!strcmp(pszName, ".") || !strcmp(pszName, "..")) return TRUE;
    if (CchSnprintf(szPath, sizeof(szPath), "%s%s%s", pz->pszRoot, szDirSep, pszName) <= 0 ||
        !FNativePathInfo(szPath, &fDir, &fLink) || fLink) { pz->fBad = TRUE; return FALSE; }
    if (fDir) {
        /* Game export stages contain only regular files and backup directories. */
        zfChild.pszRoot = szPath;
        if (!FNativeEnumerateDirectory(szPath, FCollectZipFiles, &zfChild) || zfChild.fBad) pz->fBad = TRUE;
        for (i = 0; i < zfChild.c; i++) {
            cbName = strlen(pszName) + strlen(zfChild.rgpsz[i]) + 2;
            pszRelative = (char *)SDL_malloc(cbName);
            if (!pszRelative) { pz->fBad = TRUE; break; }
            CchSnprintf(pszRelative, cbName, "%s/%s", pszName, zfChild.rgpsz[i]);
            if (!FZipEnsureCapacity(pz)) { SDL_free(pszRelative); break; }
            pz->rgpsz[pz->c++] = pszRelative;
        }
        for (i = 0; i < zfChild.c; i++) SDL_free(zfChild.rgpsz[i]);
        SDL_free(zfChild.rgpsz);
        return !pz->fBad;
    }
    if (!FZipEnsureCapacity(pz)) return FALSE;
    pz->rgpsz[pz->c] = SDL_strdup(pszName);
    if (!pz->rgpsz[pz->c]) { pz->fBad = TRUE; return FALSE; }
    pz->c++;
    return TRUE;
}

static int16_t FWriteZipEntry(NATIVEFILE *pfZip, const char *pszRoot, ZIPENTRY *pze) {
    char szPath[512];
    int64_t cb;
    uint8_t *rgbData;
    NATIVEFILE *pf;
    size_t cbName = strlen(pze->pszName);
    if (CchSnprintf(szPath, sizeof(szPath), "%s%s%s", pszRoot, szDirSep, pze->pszName) <= 0 ||
        !FNativeFileSize(szPath, &cb) || cb < 0 || cb > UINT32_MAX || cbName > UINT16_MAX ||
        (uint64_t)LNativePosition(pfZip) > UINT32_MAX) return FALSE;
    pf = PfOpenNativeFile(szPath, "rb");
    if (!pf) return FALSE;
    rgbData = (uint8_t *)SDL_malloc(cb ? (size_t)cb : 1);
    if (!rgbData) { ICloseNativeFile(pf); return FALSE; }
    if (CReadNative(rgbData, 1, (size_t)cb, pf) != (size_t)cb) { ICloseNativeFile(pf); SDL_free(rgbData); return FALSE; }
    if (ICloseNativeFile(pf) != 0) { SDL_free(rgbData); return FALSE; }
    pze->dwCrc = (uint32_t)crc32(0, rgbData, (uInt)cb);
    pze->cbSize = (uint32_t)cb;
    pze->dwOffset = (uint32_t)LNativePosition(pfZip);
    PutZip32(pfZip, ZIP_LOCAL_SIGNATURE); PutZip16(pfZip, 20); PutZip16(pfZip, 0x800); PutZip16(pfZip, 0);
    PutZip16(pfZip, 0); PutZip16(pfZip, 0); PutZip32(pfZip, pze->dwCrc);
    PutZip32(pfZip, pze->cbSize); PutZip32(pfZip, pze->cbSize);
    PutZip16(pfZip, (uint16_t)cbName); PutZip16(pfZip, 0);
    if (CWriteNative(pze->pszName, 1, cbName, pfZip) != cbName || CWriteNative(rgbData, 1, (size_t)cb, pfZip) != (size_t)cb) {
        SDL_free(rgbData); return FALSE;
    }
    SDL_free(rgbData);
    return !FNativeStreamError(pfZip);
}

int16_t FZipLibraryDirectory(const char *pszDirectory, const char *pszArchive) {
    ZIPFILES zf = {0};
    ZIPENTRY *rgzentry = NULL;
    char szTemp[512] = "";
    NATIVEFILE *pf = NULL;
    uint32_t dwStart, dwLength;
    int64_t cbExisting;
    int16_t fOk = FALSE;
    int i, cchTemp;
    size_t cbName;
    zf.pszRoot = pszDirectory;
    if (!FNativeEnumerateDirectory(pszDirectory, FCollectZipFiles, &zf) || zf.fBad || zf.c > UINT16_MAX) goto done;
    for (i = 0; i < 1000; i++) {
        cchTemp = CchSnprintf(szTemp, sizeof(szTemp), "%s.tmp.%u", pszArchive, idZipTemp++);
        if (cchTemp <= 0 || (size_t)cchTemp >= sizeof(szTemp)) goto done;
        pf = PfOpenNativeFile(szTemp, "wbx");
        if (pf) break;
        if (!FNativeFileSize(szTemp, &cbExisting)) goto done;
    }
    if (!pf) goto done;
    rgzentry = (ZIPENTRY *)SDL_calloc(zf.c ? (size_t)zf.c : 1, sizeof(*rgzentry));
    if (!rgzentry) goto done;
    for (i = 0; i < zf.c; i++) {
        rgzentry[i].pszName = SDL_strdup(zf.rgpsz[i]);
        if (!rgzentry[i].pszName || !FWriteZipEntry(pf, pszDirectory, &rgzentry[i])) goto done;
    }
    dwStart = (uint32_t)LNativePosition(pf);
    for (i = 0; i < zf.c; i++) {
        cbName = strlen(rgzentry[i].pszName);
        PutZip32(pf, ZIP_CENTRAL_SIGNATURE); PutZip16(pf, 20); PutZip16(pf, 20); PutZip16(pf, 0x800);
        PutZip16(pf, 0); PutZip16(pf, 0); PutZip16(pf, 0); PutZip32(pf, rgzentry[i].dwCrc);
        PutZip32(pf, rgzentry[i].cbSize); PutZip32(pf, rgzentry[i].cbSize); PutZip16(pf, (uint16_t)cbName);
        PutZip16(pf, 0); PutZip16(pf, 0); PutZip16(pf, 0); PutZip16(pf, 0);
        PutZip32(pf, 0); PutZip32(pf, rgzentry[i].dwOffset);
        if (CWriteNative(rgzentry[i].pszName, 1, cbName, pf) != cbName) goto done;
    }
    dwLength = (uint32_t)LNativePosition(pf) - dwStart;
    PutZip32(pf, ZIP_EOCD_SIGNATURE); PutZip16(pf, 0); PutZip16(pf, 0);
    PutZip16(pf, (uint16_t)zf.c); PutZip16(pf, (uint16_t)zf.c);
    PutZip32(pf, dwLength); PutZip32(pf, dwStart); PutZip16(pf, 0);
    if (FNativeStreamError(pf)) goto done;
    if (ICloseNativeFile(pf) != 0) { pf = NULL; goto done; }
    pf = NULL;
    if (ReplaceNativeFile(szTemp, pszArchive) != 0) goto done;
    fOk = TRUE;
done:
    if (pf) ICloseNativeFile(pf);
    if (!fOk && szTemp[0]) RemoveNativePath(szTemp);
    for (i = 0; i < zf.c; i++) { SDL_free(zf.rgpsz[i]); if (rgzentry) SDL_free(rgzentry[i].pszName); }
    SDL_free(zf.rgpsz); SDL_free(rgzentry);
    return fOk;
}

static int16_t FZipMakeParents(const char *pszRoot, const char *pszName, char *szFile, size_t cbFile) {
    char szPath[512], ch;
    int cch;
    size_t i, cchBase = strlen(pszRoot) + strlen(szDirSep);
    cch = CchSnprintf(szFile, cbFile, "%s%s%s", pszRoot, szDirSep, pszName);
    if (cch <= 0 || (size_t)cch >= cbFile || (size_t)cch >= sizeof(szPath)) return FALSE;
    strcpy(szPath, szFile);
    for (i = cchBase; i < (size_t)cch; i++) if (szPath[i] == '/' || szPath[i] == '\\') {
        ch = szPath[i]; szPath[i] = 0;
        if (!FNativeDirectoryExists(szPath) && CreateNativeDir(szPath) != 0) return FALSE;
        szPath[i] = ch;
    }
    return TRUE;
}

int16_t FUnzipLibraryArchive(const char *pszArchive, const char *pszDirectory) {
    uint8_t rgbTail[65557], rgbCentral[ZIP_CENTRAL_HEADER_SIZE], rgbLocal[ZIP_LOCAL_HEADER_SIZE];
    uint8_t *rgbLocalName = NULL, *rgbCompressed = NULL, *rgbData = NULL;
    NATIVEFILE *pf = PfOpenNativeFile(pszArchive, "rb");
    NATIVEFILE *pfOutput = NULL;
    int64_t archiveSize;
    long tailOffset, eocd = -1, centralOffset, lNext;
    uint16_t cEntries, i, wLocalNameLength, wLocalExtraLength;
    uint64_t cbTotal = 0;
    int16_t fOk = FALSE, fWrite, fUnsupportedName = FALSE;
    size_t cbRead;
    long lSearch;
    char szName[512], szOutput[512], szDirectoryEntry[512];
    ZIPCENTRAL zc;
    z_stream zs;
    int64_t cbExisting;
    if (!pf || !FNativeFileSize(pszArchive, &archiveSize) || archiveSize < ZIP_EOCD_SIZE) goto done;
    tailOffset = archiveSize > sizeof(rgbTail) ? (long)(archiveSize - sizeof(rgbTail)) : 0;
    if (ISeekNativeFile(pf, tailOffset, SEEK_SET) != 0) goto done;
    cbRead = CReadNative(rgbTail, 1, sizeof(rgbTail), pf);
    lSearch = (long)(cbRead >= ZIP_EOCD_SIZE ? cbRead - ZIP_EOCD_SIZE : 0);
    for (;;) { if (DwZip32(rgbTail + lSearch) == ZIP_EOCD_SIGNATURE) { eocd = tailOffset + lSearch; break; } if (!lSearch--) break; }
    if (eocd < 0 || ISeekNativeFile(pf, eocd + 4, SEEK_SET) != 0 ||
        CReadNative(rgbTail, 1, ZIP_EOCD_FIELDS_AFTER_SIGNATURE_SIZE, pf) != ZIP_EOCD_FIELDS_AFTER_SIGNATURE_SIZE ||
        eocd + ZIP_EOCD_SIZE + WZip16(rgbTail + ZIP_EOCD_COMMENT_LENGTH_OFFSET) != archiveSize) goto done;
    if (WZip16(rgbTail + ZIP_EOCD_DISK_NUMBER_OFFSET) || WZip16(rgbTail + ZIP_EOCD_CENTRAL_DISK_OFFSET) ||
        WZip16(rgbTail + ZIP_EOCD_ENTRIES_ON_DISK_OFFSET) != WZip16(rgbTail + ZIP_EOCD_ENTRY_COUNT_OFFSET)) goto done;
    cEntries = WZip16(rgbTail + ZIP_EOCD_ENTRY_COUNT_OFFSET);
    centralOffset = (long)DwZip32(rgbTail + ZIP_EOCD_CENTRAL_OFFSET_OFFSET);
    if (cEntries > 4096 || (uint64_t)centralOffset + DwZip32(rgbTail + ZIP_EOCD_CENTRAL_SIZE_OFFSET) > (uint64_t)eocd ||
        ISeekNativeFile(pf, centralOffset, SEEK_SET) != 0) goto done;
    for (i = 0; i < cEntries; i++) {
        if (CReadNative(rgbCentral, 1, sizeof(rgbCentral), pf) != sizeof(rgbCentral) || DwZip32(rgbCentral) != ZIP_CENTRAL_SIGNATURE) goto done;
        zc = ZipCentralHeader(rgbCentral);
        if (!zc.wNameLength || zc.wNameLength >= sizeof(szName) || zc.cbUncompressed > 64U * 1024U * 1024U ||
            (zc.wFlags & 1) || (zc.wMethod != 0 && zc.wMethod != 8) ||
            WZip16(rgbCentral + ZIP_CENTRAL_DISK_START_OFFSET) != 0 ||
            ((zc.dwExternalAttributes >> 16) & 0170000U) == 0120000U ||
            (zc.dwExternalAttributes & 0x400U) ||
            CReadNative(szName, 1, zc.wNameLength, pf) != zc.wNameLength) goto done;
        if (memchr(szName, 0, zc.wNameLength)) goto done;
        if (!FZipNameUtf8Supported(szName, zc.wNameLength, zc.wFlags)) {
            fUnsupportedName = TRUE;
            goto done;
        }
        cbTotal += zc.cbUncompressed;
        if (cbTotal > 512U * 1024U * 1024U) goto done;
        szName[zc.wNameLength] = 0;
        lNext = LNativePosition(pf) + zc.wExtraLength + zc.wCommentLength;
        if (zc.dwLocalOffset >= (uint32_t)centralOffset ||
            ISeekNativeFile(pf, zc.dwLocalOffset, SEEK_SET) != 0 || CReadNative(rgbLocal, 1, sizeof(rgbLocal), pf) != sizeof(rgbLocal) ||
            DwZip32(rgbLocal) != ZIP_LOCAL_SIGNATURE || WZip16(rgbLocal + ZIP_LOCAL_FLAGS_OFFSET) != zc.wFlags ||
            WZip16(rgbLocal + ZIP_LOCAL_METHOD_OFFSET) != zc.wMethod) goto done;
        wLocalNameLength = WZip16(rgbLocal + ZIP_LOCAL_NAME_LENGTH_OFFSET);
        wLocalExtraLength = WZip16(rgbLocal + ZIP_LOCAL_EXTRA_LENGTH_OFFSET);
        if (wLocalNameLength != zc.wNameLength) goto done;
        rgbLocalName = (uint8_t *)SDL_malloc(wLocalNameLength ? wLocalNameLength : 1);
        if (!rgbLocalName || CReadNative(rgbLocalName, 1, wLocalNameLength, pf) != wLocalNameLength ||
            memcmp(rgbLocalName, szName, zc.wNameLength) != 0 ||
            ISeekNativeFile(pf, wLocalExtraLength, SEEK_CUR) != 0 ||
            (uint64_t)LNativePosition(pf) + zc.cbCompressed > (uint64_t)centralOffset) goto done;
        SDL_free(rgbLocalName); rgbLocalName = NULL;
        if (szName[zc.wNameLength - 1] == '/') {
            szName[zc.wNameLength - 1] = 0;
            if (!FZipSafeName(szName) || CchSnprintf(szDirectoryEntry, sizeof(szDirectoryEntry), "%s/.zip-dir-entry", szName) <= 0 ||
                !FZipMakeParents(pszDirectory, szDirectoryEntry, szOutput, sizeof(szOutput)) ||
                ISeekNativeFile(pf, lNext, SEEK_SET) != 0) goto done;
            continue;
        }
        if (!FZipSafeName(szName)) goto done;
        rgbCompressed = (uint8_t *)SDL_malloc(zc.cbCompressed ? zc.cbCompressed : 1);
        rgbData = (uint8_t *)SDL_malloc(zc.cbUncompressed ? zc.cbUncompressed : 1);
        if (!rgbCompressed || !rgbData || CReadNative(rgbCompressed, 1, zc.cbCompressed, pf) != zc.cbCompressed) goto done;
        if (zc.wMethod == 0) {
            if (zc.cbCompressed != zc.cbUncompressed) goto done;
            memcpy(rgbData, rgbCompressed, zc.cbUncompressed);
        } else {
            memset(&zs, 0, sizeof(zs));
            zs.next_in = rgbCompressed; zs.avail_in = zc.cbCompressed;
            zs.next_out = rgbData; zs.avail_out = zc.cbUncompressed;
            if (inflateInit2(&zs, -MAX_WBITS) != Z_OK) goto done;
            if (inflate(&zs, Z_FINISH) != Z_STREAM_END || zs.total_in != zc.cbCompressed ||
                zs.total_out != zc.cbUncompressed) { inflateEnd(&zs); goto done; }
            inflateEnd(&zs);
        }
        if ((uint32_t)crc32(0, rgbData, zc.cbUncompressed) != zc.dwCrc ||
            !FZipMakeParents(pszDirectory, szName, szOutput, sizeof(szOutput)) ||
            FNativeFileSize(szOutput, &cbExisting)) goto done;
        pfOutput = PfOpenNativeFile(szOutput, "wbx");
        fWrite = pfOutput && CWriteNative(rgbData, 1, zc.cbUncompressed, pfOutput) == zc.cbUncompressed;
        if (pfOutput && ICloseNativeFile(pfOutput) != 0) fWrite = FALSE;
        pfOutput = NULL;
        if (!fWrite) goto done;
        SDL_free(rgbCompressed); rgbCompressed = NULL; SDL_free(rgbData); rgbData = NULL;
        if (ISeekNativeFile(pf, lNext, SEEK_SET) != 0) goto done;
    }
    fOk = TRUE;
done:
    SDL_free(rgbLocalName); SDL_free(rgbCompressed); SDL_free(rgbData);
    if (pfOutput) ICloseNativeFile(pfOutput);
    if (pf) ICloseNativeFile(pf);
    if (fUnsupportedName) return ZIP_IMPORT_UNSUPPORTED_NAME;
    return fOk ? ZIP_IMPORT_OK : ZIP_IMPORT_INVALID;
}
