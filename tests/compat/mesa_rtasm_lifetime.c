/* Exercise the real Mesa allocator across concurrent use and library unload.
 * Build against the source archive with test_mesa_rtasm.py. */
#include <assert.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

typedef struct RTASMTEST {
    void *(*lpAlloc)(size_t);
    void (*lpFree)(void *);
} RTASMTEST;

static unsigned long CbExecutableMappings(void) {
    FILE *lpFile;
    char szLine[4096], szPermissions[5];
    unsigned long ibStart, ibEnd, cb = 0;

    lpFile = fopen("/proc/self/maps", "r");
    assert(lpFile);
    while (fgets(szLine, sizeof(szLine), lpFile)) {
        if (sscanf(szLine, "%lx-%lx %4s", &ibStart, &ibEnd, szPermissions) == 3 &&
            strcmp(szPermissions, "rwxp") == 0 && !strchr(szLine, '/'))
            cb += ibEnd - ibStart;
    }
    fclose(lpFile);
    return cb;
}

static void *TestLifetime(void *lpArg) {
    RTASMTEST *lpTest = lpArg;
    unsigned char *lpFirst, *lpSecond;
    int i;

    for (i = 0; i < 1000; i++) {
        lpFirst = lpTest->lpAlloc(128);
        lpSecond = lpTest->lpAlloc(256);
        assert(lpFirst && lpSecond);
        memset(lpFirst, 0x41, 128);
        memset(lpSecond, 0x42, 256);
        lpTest->lpFree(lpFirst);
        assert(lpSecond[0] == 0x42 && lpSecond[255] == 0x42);
        lpTest->lpFree(lpSecond);
    }
    return NULL;
}

int main(int argc, char **argv) {
    RTASMTEST test;
    pthread_t rgThread[4];
    void *hLibrary, *lpSymbol;
    unsigned long cbBefore, cbAfter;
    int iCycle, i;

    assert(argc == 2);
    cbBefore = CbExecutableMappings();
    for (iCycle = 0; iCycle < 10; iCycle++) {
        hLibrary = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
        assert(hLibrary);
        lpSymbol = dlsym(hLibrary, "rtasm_exec_malloc");
        memcpy(&test.lpAlloc, &lpSymbol, sizeof(test.lpAlloc));
        lpSymbol = dlsym(hLibrary, "rtasm_exec_free");
        memcpy(&test.lpFree, &lpSymbol, sizeof(test.lpFree));
        assert(test.lpAlloc && test.lpFree);
        for (i = 0; i < 4; i++)
            assert(pthread_create(&rgThread[i], NULL, TestLifetime, &test) == 0);
        for (i = 0; i < 4; i++)
            assert(pthread_join(rgThread[i], NULL) == 0);
        assert(dlclose(hLibrary) == 0);
    }
    cbAfter = CbExecutableMappings();
    if (cbAfter != cbBefore) {
        fprintf(stderr, "FAIL: executable mappings before %lu bytes, after %lu bytes\n", cbBefore, cbAfter);
        return 2;
    }
    puts("PASS: Mesa partial free, concurrent use, empty-heap reuse and ten library unloads");
    return 0;
}
