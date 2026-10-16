#include "win.h"
#include "libraryzip.h"
#include <SDL3/SDL.h>
#include <zlib.h>

typedef struct { char **rgpsz; int c, cMax; const char *pszRoot; int16_t fBad; } ZIPFILES;
typedef struct { char *pszName; uint32_t crc, cb, off; } ZIPENTRY;
static uint32_t idZipTemp;

static uint16_t WZip16(const uint8_t *p) { return (uint16_t)(p[0] | ((uint16_t)p[1] << 8)); }
static uint32_t DwZip32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void PutZip16(FILE *pf, uint16_t w) { fputc(w & 255, pf); fputc(w >> 8, pf); }
static void PutZip32(FILE *pf, uint32_t dw) { PutZip16(pf, (uint16_t)dw); PutZip16(pf, (uint16_t)(dw >> 16)); }

static int16_t FZipSafeName(const char *psz) {
    const char *p = psz, *part = psz;
    if (!*psz || *psz == '/' || *psz == '\\') return FALSE;
    for (; *p; p++) {
        if ((unsigned char)*p < 32 || *p == '\\' || *p == ':') return FALSE;
        if (*p == '/') {
            if (p == part || (p - part == 1 && part[0] == '.') ||
                (p - part == 2 && part[0] == '.' && part[1] == '.')) return FALSE;
            part = p + 1;
        }
    }
    return p != part && !(p - part == 1 && part[0] == '.') &&
           !(p - part == 2 && part[0] == '.' && part[1] == '.');
}

static int16_t FCollectZipFiles(void *lpData, const char *pszName) {
    ZIPFILES *pz = lpData;
    char path[512];
    int16_t fDir, fLink;
    char **rg;
    if (!strcmp(pszName, ".") || !strcmp(pszName, "..")) return TRUE;
    if (CchSnprintf(path, sizeof(path), "%s%s%s", pz->pszRoot, szDirSep, pszName) <= 0 ||
        !FNativePathInfo(path, &fDir, &fLink) || fLink) { pz->fBad = TRUE; return FALSE; }
    if (fDir) {
        /* Game export stages contain only regular files and backup directories. */
        ZIPFILES child = {0};
        child.pszRoot = path;
        if (!FNativeEnumerateDirectory(path, FCollectZipFiles, &child) || child.fBad) pz->fBad = TRUE;
        for (int i = 0; i < child.c; i++) {
            size_t n = strlen(pszName) + strlen(child.rgpsz[i]) + 2;
            char *relative = SDL_malloc(n);
            if (!relative) { pz->fBad = TRUE; break; }
            CchSnprintf(relative, n, "%s/%s", pszName, child.rgpsz[i]);
            if (pz->c == pz->cMax) {
                int cap = pz->cMax ? pz->cMax * 2 : 16;
                rg = SDL_realloc(pz->rgpsz, (size_t)cap * sizeof(*rg));
                if (!rg) { SDL_free(relative); pz->fBad = TRUE; break; }
                pz->rgpsz = rg; pz->cMax = cap;
            }
            pz->rgpsz[pz->c++] = relative;
        }
        for (int i = 0; i < child.c; i++) SDL_free(child.rgpsz[i]);
        SDL_free(child.rgpsz);
        return !pz->fBad;
    }
    if (pz->c == pz->cMax) {
        int cap = pz->cMax ? pz->cMax * 2 : 16;
        rg = SDL_realloc(pz->rgpsz, (size_t)cap * sizeof(*rg));
        if (!rg) { pz->fBad = TRUE; return FALSE; }
        pz->rgpsz = rg; pz->cMax = cap;
    }
    pz->rgpsz[pz->c] = SDL_strdup(pszName);
    if (!pz->rgpsz[pz->c]) { pz->fBad = TRUE; return FALSE; }
    pz->c++;
    return TRUE;
}

static int16_t FWriteZipEntry(FILE *pfZip, const char *pszRoot, ZIPENTRY *pze) {
    char path[512];
    int64_t cb;
    uint8_t *data;
    FILE *pf;
    size_t n = strlen(pze->pszName);
    if (CchSnprintf(path, sizeof(path), "%s%s%s", pszRoot, szDirSep, pze->pszName) <= 0 ||
        !FNativeFileSize(path, &cb) || cb < 0 || cb > UINT32_MAX || n > UINT16_MAX ||
        (uint64_t)ftell(pfZip) > UINT32_MAX) return FALSE;
    pf = PfOpenNativeFile(path, "rb");
    if (!pf) return FALSE;
    data = SDL_malloc(cb ? (size_t)cb : 1);
    if (!data) { fclose(pf); return FALSE; }
    if (fread(data, 1, (size_t)cb, pf) != (size_t)cb) { fclose(pf); SDL_free(data); return FALSE; }
    if (fclose(pf) != 0) { SDL_free(data); return FALSE; }
    pze->crc = (uint32_t)crc32(0, data, (uInt)cb);
    pze->cb = (uint32_t)cb;
    pze->off = (uint32_t)ftell(pfZip);
    PutZip32(pfZip, 0x04034b50); PutZip16(pfZip, 20); PutZip16(pfZip, 0x800); PutZip16(pfZip, 0);
    PutZip16(pfZip, 0); PutZip16(pfZip, 0); PutZip32(pfZip, pze->crc);
    PutZip32(pfZip, pze->cb); PutZip32(pfZip, pze->cb);
    PutZip16(pfZip, (uint16_t)n); PutZip16(pfZip, 0);
    if (fwrite(pze->pszName, 1, n, pfZip) != n || fwrite(data, 1, (size_t)cb, pfZip) != (size_t)cb) {
        SDL_free(data); return FALSE;
    }
    SDL_free(data);
    return !ferror(pfZip);
}

int16_t FZipLibraryDirectory(const char *pszDirectory, const char *pszArchive) {
    ZIPFILES zf = {0};
    ZIPENTRY *entries = NULL;
    char temp[512] = "";
    FILE *pf = NULL;
    uint32_t start, length;
    int16_t ok = FALSE;
    int i, n;
    zf.pszRoot = pszDirectory;
    if (!FNativeEnumerateDirectory(pszDirectory, FCollectZipFiles, &zf) || zf.fBad || zf.c > UINT16_MAX) goto done;
    for (i = 0; i < 1000; i++) {
        n = CchSnprintf(temp, sizeof(temp), "%s.tmp.%u", pszArchive, idZipTemp++);
        if (n <= 0 || (size_t)n >= sizeof(temp)) goto done;
        pf = PfOpenNativeFile(temp, "wbx");
        if (pf) break;
        if (!FNativeFileSize(temp, &(int64_t){0})) goto done;
    }
    if (!pf) goto done;
    entries = SDL_calloc(zf.c ? (size_t)zf.c : 1, sizeof(*entries));
    if (!entries) goto done;
    for (i = 0; i < zf.c; i++) {
        entries[i].pszName = SDL_strdup(zf.rgpsz[i]);
        if (!entries[i].pszName || !FWriteZipEntry(pf, pszDirectory, &entries[i])) goto done;
    }
    start = (uint32_t)ftell(pf);
    for (i = 0; i < zf.c; i++) {
        size_t nameLen = strlen(entries[i].pszName);
        PutZip32(pf, 0x02014b50); PutZip16(pf, 20); PutZip16(pf, 20); PutZip16(pf, 0x800);
        PutZip16(pf, 0); PutZip16(pf, 0); PutZip16(pf, 0); PutZip32(pf, entries[i].crc);
        PutZip32(pf, entries[i].cb); PutZip32(pf, entries[i].cb); PutZip16(pf, (uint16_t)nameLen);
        PutZip16(pf, 0); PutZip16(pf, 0); PutZip16(pf, 0); PutZip16(pf, 0);
        PutZip32(pf, 0); PutZip32(pf, entries[i].off);
        if (fwrite(entries[i].pszName, 1, nameLen, pf) != nameLen) goto done;
    }
    length = (uint32_t)ftell(pf) - start;
    PutZip32(pf, 0x06054b50); PutZip16(pf, 0); PutZip16(pf, 0);
    PutZip16(pf, (uint16_t)zf.c); PutZip16(pf, (uint16_t)zf.c);
    PutZip32(pf, length); PutZip32(pf, start); PutZip16(pf, 0);
    if (ferror(pf)) goto done;
    if (fclose(pf) != 0) { pf = NULL; goto done; }
    pf = NULL;
    if (ReplaceNativeFile(temp, pszArchive) != 0) goto done;
    ok = TRUE;
done:
    if (pf) fclose(pf);
    if (!ok && temp[0]) RemoveNativePath(temp);
    for (i = 0; i < zf.c; i++) { SDL_free(zf.rgpsz[i]); if (entries) SDL_free(entries[i].pszName); }
    SDL_free(zf.rgpsz); SDL_free(entries);
    return ok;
}

static int16_t FZipMakeParents(const char *root, const char *name, char *file, size_t fileSize) {
    char path[512];
    int n = CchSnprintf(file, fileSize, "%s%s%s", root, szDirSep, name);
    size_t i, base = strlen(root) + strlen(szDirSep);
    if (n <= 0 || (size_t)n >= fileSize || (size_t)n >= sizeof(path)) return FALSE;
    strcpy(path, file);
    for (i = base; i < (size_t)n; i++) if (path[i] == '/' || path[i] == '\\') {
        char ch = path[i]; path[i] = 0;
        if (!FNativeDirectoryExists(path) && CreateNativeDir(path) != 0) return FALSE;
        path[i] = ch;
    }
    return TRUE;
}

int16_t FUnzipLibraryArchive(const char *pszArchive, const char *pszDirectory) {
    uint8_t tail[65557], central[46], local[30], *compressed = NULL, *data = NULL;
    FILE *pf = PfOpenNativeFile(pszArchive, "rb");
    int64_t archiveSize;
    long tailOffset, eocd = -1, centralOffset, next;
    uint16_t count, i;
    uint64_t cbTotal = 0;
    int16_t ok = FALSE;
    if (!pf || !FNativeFileSize(pszArchive, &archiveSize) || archiveSize < 22) goto done;
    tailOffset = archiveSize > sizeof(tail) ? (long)(archiveSize - sizeof(tail)) : 0;
    if (fseek(pf, tailOffset, SEEK_SET) != 0) goto done;
    { size_t got = fread(tail, 1, sizeof(tail), pf); long j = (long)(got >= 22 ? got - 22 : 0);
      for (;;) { if (DwZip32(tail + j) == 0x06054b50) { eocd = tailOffset + j; break; } if (!j--) break; } }
    if (eocd < 0 || fseek(pf, eocd + 4, SEEK_SET) != 0 || fread(tail, 1, 18, pf) != 18 ||
        eocd + 22 + WZip16(tail + 16) != archiveSize) goto done;
    if (WZip16(tail) || WZip16(tail + 2) || WZip16(tail + 4) != WZip16(tail + 6)) goto done;
    count = WZip16(tail + 6); centralOffset = (long)DwZip32(tail + 12);
    if (count > 4096 || (uint64_t)centralOffset + DwZip32(tail + 8) > (uint64_t)eocd ||
        fseek(pf, centralOffset, SEEK_SET) != 0) goto done;
    for (i = 0; i < count; i++) {
        char name[512], output[512];
        uint16_t nameLen, extraLen, commentLen, flags, method;
        uint32_t compressedSize, dataSize, expectedCrc, localOffset;
        if (fread(central, 1, sizeof(central), pf) != sizeof(central) || DwZip32(central) != 0x02014b50) goto done;
        flags = WZip16(central + 8); method = WZip16(central + 10);
        expectedCrc = DwZip32(central + 16); compressedSize = DwZip32(central + 20);
        dataSize = DwZip32(central + 24); nameLen = WZip16(central + 28);
        extraLen = WZip16(central + 30); commentLen = WZip16(central + 32);
        localOffset = DwZip32(central + 42);
        if (!nameLen || nameLen >= sizeof(name) || dataSize > 64U * 1024U * 1024U ||
            (flags & 1) || (method != 0 && method != 8) || WZip16(central + 34) != 0 ||
            ((DwZip32(central + 38) >> 16) & 0170000U) == 0120000U ||
            (DwZip32(central + 38) & 0x400U) ||
            fread(name, 1, nameLen, pf) != nameLen) goto done;
        if (memchr(name, 0, nameLen)) goto done;
        cbTotal += dataSize;
        if (cbTotal > 512U * 1024U * 1024U) goto done;
        name[nameLen] = 0;
        next = ftell(pf) + extraLen + commentLen;
        if (name[nameLen - 1] == '/') {
            name[nameLen - 1] = 0;
            if (!FZipSafeName(name)) goto done;
            { char szDirFile[512];
              if (CchSnprintf(szDirFile, sizeof(szDirFile), "%s/.zip-dir-entry", name) <= 0 ||
                  !FZipMakeParents(pszDirectory, szDirFile, output, sizeof(output))) goto done; }
            if (fseek(pf, next, SEEK_SET) != 0) goto done;
            continue;
        }
        if (!FZipSafeName(name)) goto done;
        if (fseek(pf, localOffset, SEEK_SET) != 0 || fread(local, 1, sizeof(local), pf) != sizeof(local) ||
            DwZip32(local) != 0x04034b50 ||
            fseek(pf, WZip16(local + 26) + WZip16(local + 28), SEEK_CUR) != 0) goto done;
        if (localOffset >= (uint32_t)centralOffset ||
            (uint64_t)ftell(pf) + compressedSize > (uint64_t)centralOffset) goto done;
        compressed = SDL_malloc(compressedSize ? compressedSize : 1);
        data = SDL_malloc(dataSize ? dataSize : 1);
        if (!compressed || !data || fread(compressed, 1, compressedSize, pf) != compressedSize) goto done;
        if (method == 0) {
            if (compressedSize != dataSize) goto done;
            memcpy(data, compressed, dataSize);
        } else {
            z_stream stream; memset(&stream, 0, sizeof(stream));
            stream.next_in = compressed; stream.avail_in = compressedSize;
            stream.next_out = data; stream.avail_out = dataSize;
            if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) goto done;
            if (inflate(&stream, Z_FINISH) != Z_STREAM_END || stream.total_in != compressedSize ||
                stream.total_out != dataSize) { inflateEnd(&stream); goto done; }
            inflateEnd(&stream);
        }
        if ((uint32_t)crc32(0, data, dataSize) != expectedCrc ||
            !FZipMakeParents(pszDirectory, name, output, sizeof(output)) ||
            FNativeFileSize(output, &(int64_t){0})) goto done;
        { FILE *out = PfOpenNativeFile(output, "wbx");
          int16_t fWrite = out && fwrite(data, 1, dataSize, out) == dataSize;
          if (out && fclose(out) != 0) fWrite = FALSE;
          if (!fWrite) goto done; }
        SDL_free(compressed); compressed = NULL; SDL_free(data); data = NULL;
        if (fseek(pf, next, SEEK_SET) != 0) goto done;
    }
    ok = TRUE;
done:
    SDL_free(compressed); SDL_free(data);
    if (pf) fclose(pf);
    return ok;
}
