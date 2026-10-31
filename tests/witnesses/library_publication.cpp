#include <cstdarg>
#include "library_publication_io.h"
#include "librarypublication.h"
#include "native_publish.h"
#include "ui_fixture.h"
static char szLibraryRoot[256] = "root", szLibraryName[256] = "Copy", szLibraryPending[256] = "pending";

// Zero is absent; files are 1..3; directories are bit sets plus 100.
static int         objects[8], count, profile, variant, cleared;
static unsigned    mask;
static const char *names[8] = {"source", "target", "root/Game/Game.x1", "stage", "previous", "root/Game", "hold", "pending"};
static int         role(const char *p) {
    if (strstr(p, ".previous."))
        return 4;
    if (strstr(p, ".importing."))
        return 3;
    if (!strcmp(p, "root/Copy"))
        return 1;
    for (int i = 0; i < 8; i++)
        if (!strcmp(p, names[i]))
            return i;
    fprintf(stderr, "unmapped path %s\n", p);
    abort();
}
static int decision(void) {
    assert(count < 24);
    return !(mask & (1u << count));
}
static void event(const char *kind, int a, int b, int ok, int residue) {
    printf("[\"%s\",%d,%d,%d,%d,[", kind, a, b, ok, residue);
    for (int i = 0; i < 8; i++)
        printf("%s%d", i ? "," : "", objects[i]);
    printf("]],");
    count++;
}
static void clear(void) {
    if (profile == 4 && !szLibraryPending[0] && !cleared) {
        printf("[\"clear\"],");
        cleared = 1;
    }
}
static const char *TestPszGetCompressedString(int id) {
    switch (id) {
    case idsLibraryTurnStageFailed:
        return "1";
    case idsLibraryTurnInstallFailed:
        return "2";
    case idsLibraryTurnPreserveFailed:
        return "3";
    case idsLibraryTurnRestoreFailed:
        return "4";
    case idsLibraryGameRestoreFailed:
        return "4";
    case idsLibraryTurnOrdersRetireFailed:
        return "5";
    case idsLibraryTurnReplaceFailed:
        return "6";
    case idsLibraryTurnCleanupFailed:
        return "7";
    case idsLibrarySeparateCopyFailed:
        return "8";
    case idsLibraryGameImportFailed:
        return "9";
    default:
        abort();
    }
}
static void TestAlertSz(const char *s, int flags) {
    (void)flags;
    clear();
    printf("[\"notice\",%d],", atoi(s));
}
static int TestCchSnprintf(char *out, size_t cap, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(out, cap, format, args);
    va_end(args);
    int tag = 0;
    if (!strcmp(format, "%s%s%s%s%s.x%d"))
        tag = 1;
    else if (!strcmp(format, "%s.importing.%u"))
        tag = 2;
    else if (!strcmp(format, "%s.previous.%u"))
        tag = 3;
    else if (!strcmp(format, "%s%s%s"))
        tag = (profile == 3 && !strcmp(out, "root/Copy")) ? 5 : 4;
    // New-family stage formatting is unchecked in the source and is not a guard.
    if (tag) {
        int ok = decision();
        event("format", tag, 0, ok, 0);
        if (!ok)
            return (int)cap;
    }
    return result;
}
static uint32_t TestDwTickCount(void) { return 7; }
static int64_t  TestLNativeTime(void) { return 77; }
static int      TestFCopyNativeFileExclusive(const char *source, const char *target) {
    int s = role(source), t = role(target), ok = decision() && objects[s] > 0 && objects[s] < 100 && !objects[t], residue = 0;
    if (ok)
        objects[t] = objects[s];
    else if (variant == 1 && !objects[t] && objects[s])
        objects[t] = residue = 3;
    event("copy", s, t, ok, residue);
    return ok;
}
static int TestFInspectImportFile(const char *path, int type, int player, int32_t *id, uint16_t *turn, void *gen, int16_t *multi) {
    (void)type;
    (void)player;
    assert(!gen);
    int p = role(path), inspected = decision() && objects[p] > 0 && objects[p] < 100;
    // Probe 2 includes inspection and the two subsequent source comparisons.
    *id = variant == 22 ? 999 : 123;
    *turn = variant == 23 ? 12 : 11;
    *multi = variant == 24;
    event("probe", 2, p, inspected && variant != 22 && variant != 23, 0);
    return inspected;
}
static int TestFNativeFileSize(const char *path, int64_t *size) {
    int p = role(path), ok = decision() && objects[p] > 0 && objects[p] < 100;
    *size = 1;
    event("probe", 1, p, ok, 0);
    return ok;
}
static int move(const char *source, const char *target) {
    int s = role(source), t = role(target), ok = decision() && objects[s];
    if (ok) {
        objects[t] = objects[s];
        objects[s] = 0;
    }
    event("replace", s, t, ok, 0);
    return ok ? 0 : -1;
}
static int removeObject(const char *path, int tree) {
    int p = role(path), ok = decision(), residue = 0;
    if (ok)
        objects[p] = 0;
    else if (variant == 1 && objects[p] >= 100) {
        objects[p] = 100 + ((objects[p] - 100) & 1);
        residue = objects[p];
    }
    event("remove", p, tree, ok, residue);
    return ok;
}
static int TestRemoveNativeFile(const char *path) { return removeObject(path, 0) ? 0 : -1; }
static int TestRemoveNativePath(const char *path) { return removeObject(path, 0) ? 0 : -1; }
static int TestRemoveLibraryGameTree(const char *path) { return removeObject(path, 1); }
static int TestFNativeDirectoryExists(const char *path) {
    int p = role(path), ok = decision() && objects[p] >= 100;
    event("probe", 3, p, ok, 0);
    return ok;
}
static int TestFValidManagedLibraryGame(const char *name, const char *dir, int *remember, long long *played, char *exchange, size_t cap) {
    assert(!strcmp(name, "Game"));
    int p = role(dir), ok = decision() && objects[p] >= 100;
    *remember = 4;
    *played = 321;
    assert(cap > 9);
    strcpy(exchange, "external");
    event("probe", 5, p, ok, 0);
    return ok;
}
static int TestFCreateLibraryGameStagePath(char *out, size_t cap) {
    assert(cap > 5);
    strcpy(out, "stage");
    int ok = decision() && !objects[3];
    if (ok)
        objects[3] = 100;
    event("allocate", 1, 3, ok, 0);
    return ok;
}
static int TestFCreateLibraryGameHoldPath(char *out, size_t cap) {
    assert(cap > 4);
    strcpy(out, "hold");
    int ok = decision();
    event("probe", 4, 6, ok, 0);
    return ok;
}
static int build(int tag) {
    int ok = decision();
    assert(objects[3] >= 100);
    objects[3] = 100 + ((objects[3] - 100) | (1 << (tag - 1)));
    event("build", tag, 3, ok, objects[3]);
    return ok;
}
static int TestFCreateLibraryGameStage(const char *source, const char *old, const char *name, const char *stage, int player, long long played,
                                       const char *exchange) {
    assert(role(source) == 5 && role(stage) == 3 && !strcmp(old, "Game") && !strcmp(exchange, "external"));
    if (profile == 2) {
        assert(!strcmp(name, "Game") && player == 4 && played == 321);
        return build(1);
    }
    assert(profile == 3 && !strcmp(name, "Copy") && player == 0 && played == 0);
    return build(4);
}
static int TestFBackupConflictingPlayerFiles(const char *stage, const char *name, int player, char *backup, size_t cap) {
    assert(role(stage) == 3 && !strcmp(name, "Game") && player == 0 && cap > 6);
    strcpy(backup, "backup");
    return build(2);
}
static int TestFStageConflictingTurn(const char *stage, const char *name, const char *source, int player, int32_t id, uint16_t turn) {
    assert(role(stage) == 3 && !strcmp(name, profile == 2 ? "Game" : "Copy") && role(source) == 0 && player == 0 && id == 123 && turn == 11);
    return build(3);
}
static int TestFChooseSeparateCopyName(HWND hwnd, const char *name) {
    (void)hwnd;
    assert(!strcmp(name, "Game"));
    int ok = decision();
    event("probe", 6, 1, ok, 0);
    return ok;
}
static int TestFLibraryNameAvailable(const char *root, const char *name) {
    assert(root == szLibraryRoot && !strcmp(name, "Copy"));
    int ok = decision();
    event("probe", 7, 1, ok, 0);
    return ok;
}
static size_t TestNativeStrlen(const char *s) {
    if (s == szLibraryRoot) {
        int ok = decision();
        event("probe", 8, 1, ok, 0);
        if (!ok)
            return sizeof(szBase);
    }
    return strlen(s);
}
// Family enumeration is a declared stage-only provider; cleanup aggregates the
// source enumeration plus final TestRemoveNativePath, including possible remnants.

static int TestFNativeEnumerateDirectory(const char *path, FNativeDirectoryEntryProc callback, void *context) {
    (void)path;
    if (callback == &CopyImportEntry || callback == &CopyBackupDirectory) {
        int ok = build(callback == &CopyImportEntry ? 5 : 6);
        if (variant == 4 && !ok) {
            ((LIBRARYIMPORT *)context)->fBad = 1;
            return 1;
        }
        return ok;
    }
    assert(callback == &RemoveImportFile);
    // Removal itself is performed by the immediately following source call.
    return 1;
}
static int TestFWriteLibraryMetadata(const char *path, const char *name, int16_t player) {
    int expected = variant >= 5 && variant <= 20 ? variant - 5 : variant == 21 ? -1 : 0;
    assert(role(path) == 3 && !strcmp(name, "Copy") && player == expected);
    return build(7);
}
static int TestCreateNativeDir(const char *path) {
    int p = role(path), ok = decision() && !objects[p];
    if (ok)
        objects[p] = 100;
    event("allocate", 2, p, ok, 0);
    return ok ? 0 : -1;
}
static int     TestRemove(const char *path, int directory) { return directory ? TestRemoveNativePath(path) : TestRemoveNativeFile(path); }
static int     TestRename(const char *source, const char *target, int, int) { return move(source, target); }
static int16_t NativeLater(int16_t hasTurn) {
    return FPublishSubsequentTurn({szLibraryRoot, szLibraryPending, szLibraryName}, nullptr, "source", "target", "Game", 0, 123, 11, hasTurn);
}
static int16_t NativeFamily() {
    LIBRARYIMPORT li{};
    li.grfPlayers = variant >= 5 && variant <= 20 ? 1u << (variant - 5) : variant == 21 ? 0 : 1;
    return FPublishImportedLibraryGame({szLibraryRoot, szLibraryPending, szLibraryName}, li, "source");
}

int main(void) {
    unsigned             inputMask;
    int                  inputProfile, inputVariant;
    LIBRARYPUBLICATIONIO io;
    io.pfnCchSnprintf = TestCchSnprintf;
    io.pfnDwTickCount = WITNESSADAPTER<&TestDwTickCount, decltype(io.pfnDwTickCount)>::Call;
    io.pfnLNativeTime = WITNESSADAPTER<&TestLNativeTime, decltype(io.pfnLNativeTime)>::Call;
    io.pfnFCopyNativeFileExclusive = WITNESSADAPTER<&TestFCopyNativeFileExclusive, decltype(io.pfnFCopyNativeFileExclusive)>::Call;
    io.pfnFInspectImportFile = WITNESSADAPTER<&TestFInspectImportFile, decltype(io.pfnFInspectImportFile)>::Call;
    io.pfnFNativeFileSize = WITNESSADAPTER<&TestFNativeFileSize, decltype(io.pfnFNativeFileSize)>::Call;
    io.pfnFNativeDirectoryExists = WITNESSADAPTER<&TestFNativeDirectoryExists, decltype(io.pfnFNativeDirectoryExists)>::Call;
    io.pfnFValidManagedLibraryGame = WITNESSADAPTER<&TestFValidManagedLibraryGame, decltype(io.pfnFValidManagedLibraryGame)>::Call;
    io.pfnFCreateLibraryGameStagePath = WITNESSADAPTER<&TestFCreateLibraryGameStagePath, decltype(io.pfnFCreateLibraryGameStagePath)>::Call;
    io.pfnFCreateLibraryGameHoldPath = WITNESSADAPTER<&TestFCreateLibraryGameHoldPath, decltype(io.pfnFCreateLibraryGameHoldPath)>::Call;
    io.pfnFCreateLibraryGameStage = WITNESSADAPTER<&TestFCreateLibraryGameStage, decltype(io.pfnFCreateLibraryGameStage)>::Call;
    io.pfnFBackupConflictingPlayerFiles = WITNESSADAPTER<&TestFBackupConflictingPlayerFiles, decltype(io.pfnFBackupConflictingPlayerFiles)>::Call;
    io.pfnFStageConflictingTurn = WITNESSADAPTER<&TestFStageConflictingTurn, decltype(io.pfnFStageConflictingTurn)>::Call;
    io.pfnFChooseSeparateCopyName = WITNESSADAPTER<&TestFChooseSeparateCopyName, decltype(io.pfnFChooseSeparateCopyName)>::Call;
    io.pfnFLibraryNameAvailable = WITNESSADAPTER<&TestFLibraryNameAvailable, decltype(io.pfnFLibraryNameAvailable)>::Call;
    io.pfnFWriteLibraryMetadata = WITNESSADAPTER<&TestFWriteLibraryMetadata, decltype(io.pfnFWriteLibraryMetadata)>::Call;
    io.pfnRemoveLibraryGameTree = WITNESSADAPTER<&TestRemoveLibraryGameTree, decltype(io.pfnRemoveLibraryGameTree)>::Call;
    io.pfnFNativeEnumerateDirectory = WITNESSADAPTER<&TestFNativeEnumerateDirectory, decltype(io.pfnFNativeEnumerateDirectory)>::Call;
    io.pfnCreateNativeDir = WITNESSADAPTER<&TestCreateNativeDir, decltype(io.pfnCreateNativeDir)>::Call;
    io.pfnPathLength = TestNativeStrlen;
    LIBRARYPUBLICATIONIOSCOPE publicationScope(io);
    NATIVEPUBLISHIO           nativeIO;
    nativeIO.pfnRemove = TestRemove;
    nativeIO.pfnRename = TestRename;
    NATIVEPUBLISHIOSCOPE nativeScope(nativeIO);
    TEXTIO               textIO;
    textIO.pfnCompressed = WITNESSADAPTER<&TestPszGetCompressedString, decltype(textIO.pfnCompressed)>::Call;
    TEXTIOSCOPE textScope(textIO);
    ALERTIO     alertIO;
    alertIO.pfnAlertSz = WITNESSADAPTER<&TestAlertSz, decltype(alertIO.pfnAlertSz)>::Call;
    ALERTIOSCOPE alertScope(alertIO);
    while (scanf("%d %u %d", &inputProfile, &inputMask, &inputVariant) == 3) {
        profile = inputProfile;
        mask = inputMask;
        variant = inputVariant;
        count = cleared = 0;
        memset(objects, 0, sizeof(objects));
        objects[0] = 1;
        strcpy(szLibraryPending, "pending");
        if (profile == 0)
            objects[1] = 2;
        if (profile <= 1)
            objects[2] = 2;
        if (profile == 2 || profile == 3)
            objects[5] = 115;
        if (profile == 4)
            objects[7] = 100;
        if (variant == 2)
            objects[3] = 2;
        if (variant == 3)
            objects[4] = 2;
        printf("{\"profile\":%d,\"mask\":%u,\"variant\":%d,\"events\":[", profile, mask, variant);
        int result = profile <= 1 ? NativeLater(profile == 0)
                     : profile == 2
                         ? FReplaceConflictingLibraryTurn({szLibraryRoot, szLibraryPending, szLibraryName}, nullptr, "Game", "source", "target", 0, 123, 11)
                     : profile == 3 ? FImportConflictingTurnAsCopy({szLibraryRoot, szLibraryPending, szLibraryName}, nullptr, "Game", "source", 0, 123, 11)
                                    : NativeFamily();
        clear();
        printf("[\"end\"]],\"result\":%d,\"calls\":%d,\"store\":[", result, count);
        for (int i = 0; i < 8; i++)
            printf("%s%d", i ? "," : "", objects[i]);
        printf("]}\n");
    }
    return 0;
}
