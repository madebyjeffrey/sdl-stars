#include <cassert>
#include <cerrno>
#include <cstdio>
#include "library_file_io.h"
#include "librarypublication.h"
#include "native_publish.h"
#include "win.h"
static int         faults;
static NATIVEFILE *copySource, *copyTarget;
static NATIVEFILE *copyOpen(const char *path, const char *mode) {
    NATIVEFILE *f = PfOpenNativeFile(path, mode);
    if (!strcmp(mode, "rb"))
        copySource = f;
    else
        copyTarget = f;
    return f;
}
static size_t copyRead(void *data, size_t size, size_t count, NATIVEFILE *f) { return f == copySource && (faults & 1) ? 0 : CReadNative(data, size, count, f); }
static int    copyError(NATIVEFILE *f) { return FNativeStreamError(f) || (f == copySource && (faults & 1)); }
static size_t copyWrite(const void *data, size_t size, size_t count, NATIVEFILE *f) { return CWriteNative(data, size, faults & 2 ? count / 2 : count, f); }
static int    copyClose(NATIVEFILE *f) {
    int forced = (f == copySource && (faults & 4)) || (f == copyTarget && (faults & 8));
    if (f == copySource)
        copySource = NULL;
    if (f == copyTarget)
        copyTarget = NULL;
    int status = ICloseNativeFile(f);
    if (forced) {
        errno = EIO;
        return EOF;
    }
    return status;
}
static int copyRemove(const char *path, int directory) {
    assert(!directory);
    if (faults & 16) {
        errno = EACCES;
        return -1;
    }
    return remove(path);
}
static void writeSource(int n) {
    FILE *f = fopen("source", "wb");
    assert(f);
    for (int i = 0; i < n; i++)
        assert(fputc((i * 31) % 251, f) != EOF);
    assert(fclose(f) == 0);
}
static void verifyTarget(int n) {
    FILE *f = fopen("target", "rb");
    assert(f);
    for (int i = 0; i < n; i++)
        assert(fgetc(f) == (i * 31) % 251);
    assert(fgetc(f) == EOF && !ferror(f));
    assert(fclose(f) == 0);
}
int main(void) {
    LIBRARYFILEIO io;
    io.pfnOpen = copyOpen;
    io.pfnRead = copyRead;
    io.pfnWrite = copyWrite;
    io.pfnError = copyError;
    io.pfnClose = copyClose;
    LIBRARYFILEIOSCOPE fileScope(io);
    NATIVEPUBLISHIO    nativeIO;
    nativeIO.pfnRemove = copyRemove;
    NATIVEPUBLISHIOSCOPE nativeScope(nativeIO);
    const int            lengths[] = {0, 1, 8191, 8192, 8193, 16384, 16385};
    int                  cases = 0;
    for (unsigned i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
        remove("target");
        writeSource(lengths[i]);
        assert(FCopyNativeFileExclusive("source", "target"));
        verifyTarget(lengths[i]);
        cases++;
        writeSource(lengths[i] + 1);
        errno = 0;
        assert(!FCopyNativeFileExclusive("source", "target") && errno == EEXIST);
        verifyTarget(lengths[i]);
        cases++;
    }
    remove("source");
    remove("target");
    assert(!FCopyNativeFileExclusive("source", "target"));
    assert(!fopen("target", "rb"));
    cases++;
    for (int bits = 0; bits < 32; bits++) {
        remove("target");
        writeSource(8193);
        faults = bits;
        copySource = copyTarget = NULL;
        int ok = FCopyNativeFileExclusive("source", "target");
        assert(ok == !(bits & 15));
        int retained = ok || (bits & 16), length = (bits & 1) ? 0 : (bits & 2) ? 4096 : 8193;
        if (retained)
            verifyTarget(length);
        else
            assert(!fopen("target", "rb"));
        FILE *f = fopen("source", "rb");
        assert(f);
        for (int i = 0; i < 8193; i++)
            assert(fgetc(f) == (i * 31) % 251);
        assert(fgetc(f) == EOF && !ferror(f));
        assert(fclose(f) == 0);
        cases++;
    }
    faults = 0;
    remove("target");
    writeSource(1);
    assert(FCopyNativeFileExclusive("source", "target"));
    faults = 4;
    errno = 0;
    assert(!FCopyNativeFileExclusive("source", "target") && errno == EEXIST);
    verifyTarget(1);
    cases++;
    faults = 0;
    printf("%d linked-copy cases: 15 real-stdio boundaries plus 33 read/write/close/cleanup receipt combinations\n", cases);
    return 0;
}
