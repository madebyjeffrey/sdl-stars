#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
#include "library_publication_io.h"
#include "librarypublication.h"
#include "native_publish.h"
#include "ui_fixture.h"
#include <algorithm>
#include <string>
#include <string_view>
static char szLibraryRoot[256] = "root", szLibraryName[256] = "Copy", szLibraryPending[256] = "pending";

// Zero is absent; files are 1..3; directories are bit sets plus 100.
static int         objects[8], count, profile, variant, cleared;
static unsigned    mask;
static const std::string_view names[8] = {"source", "target", "root/Game/Game.x1", "stage", "previous", "root/Game", "hold", "pending"};
static bool FSamePath(std::string_view szPath1, std::string_view szPath2) {
    if (szPath1.size() != szPath2.size()) return false;
    for (std::size_t i = 0; i < szPath1.size(); i++) {
        if (szPath1[i] != szPath2[i] && !((szPath1[i] == '/' || szPath1[i] == '\\') && (szPath2[i] == '/' || szPath2[i] == '\\')))
            return false;
    }
    return true;
}
static int         role(std::string_view szPath) {
    if (szPath.find(".previous.") != std::string_view::npos)
        return 4;
    if (szPath.find(".importing.") != std::string_view::npos)
        return 3;
    if (FSamePath(szPath, "root/Copy"))
        return 1;
    for (int i = 0; i < 8; i++)
        if (FSamePath(szPath, names[i]))
            return i;
    fprintf(stderr, "unmapped path %.*s\n", static_cast<int>(szPath.size()), szPath.data());
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
static std::array<char, cchCatalogText> TestPszGetCompressedString(APPSESSION &, StringId id) {
    switch (id) {
    case idsLibraryTurnStageFailed:
        return SzWitnessText("1");
    case idsLibraryTurnInstallFailed:
        return SzWitnessText("2");
    case idsLibraryTurnPreserveFailed:
        return SzWitnessText("3");
    case idsLibraryTurnRestoreFailed:
        return SzWitnessText("4");
    case idsLibraryGameRestoreFailed:
        return SzWitnessText("4");
    case idsLibraryTurnOrdersRetireFailed:
        return SzWitnessText("5");
    case idsLibraryTurnReplaceFailed:
        return SzWitnessText("6");
    case idsLibraryTurnCleanupFailed:
        return SzWitnessText("7");
    case idsLibrarySeparateCopyFailed:
        return SzWitnessText("8");
    case idsLibraryGameImportFailed:
        return SzWitnessText("9");
    default:
        abort();
    }
}
static void TestAlertSz(APPSESSION &, std::string_view s, int flags) {
    (void)flags;
    clear();
    printf("[\"notice\",%d],", std::stoi(std::string{s}));
}
static int TestCchSnprintf(std::span<char> out, std::string_view format, std::span<const NATIVEFORMATARG> args) {
    int result = CchFormatNative(out, format, args);
    int tag = 0;
    if (format == "%s%s%s%s%s.x%d")
        tag = 1;
    else if (format == "%s.importing.%u")
        tag = 2;
    else if (format == "%s.previous.%u")
        tag = 3;
    else if (format == "%s%s%s")
        tag = (profile == 3 && FSamePath(out.data(), "root/Copy")) ? 5 : 4;
    // New-family stage formatting is unchecked in the source and is not a guard.
    if (tag) {
        int ok = decision();
        event("format", tag, 0, ok, 0);
        if (!ok)
            return static_cast<int>(out.size());
    }
    return result;
}
static uint32_t TestDwTickCount(void) { return 7; }
static int64_t  TestLNativeTime(void) { return 77; }
static int      TestFCopyNativeFileExclusive(std::string_view source, std::string_view target) {
    int s = role(source), t = role(target), ok = decision() && objects[s] > 0 && objects[s] < 100 && !objects[t], residue = 0;
    if (ok)
        objects[t] = objects[s];
    else if (variant == 1 && !objects[t] && objects[s])
        objects[t] = residue = 3;
    event("copy", s, t, ok, residue);
    return ok;
}
static int TestFInspectImportFile(std::string_view path, int type, int player, int32_t &id, uint16_t *turn, void *gen, int16_t &multi) {
    (void)type;
    (void)player;
    assert(!gen);
    int p = role(path), inspected = decision() && objects[p] > 0 && objects[p] < 100;
    // Probe 2 includes inspection and the two subsequent source comparisons.
    id = variant == 22 ? 999 : 123;
    *turn = variant == 23 ? 12 : 11;
    multi = variant == 24;
    event("probe", 2, p, inspected && variant != 22 && variant != 23, 0);
    return inspected;
}
static std::optional<std::int64_t> TestFNativeFileSize(std::string_view path) {
    const std::string szPath{path};
    int p = role(szPath.c_str()), ok = decision() && objects[p] > 0 && objects[p] < 100;
    event("probe", 1, p, ok, 0);
    return ok ? std::optional<std::int64_t>{1} : std::nullopt;
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
static int TestRemoveLibraryGameTree(APPSESSION &, std::string_view path) {
    const std::string szPath{path};
    return removeObject(szPath.c_str(), 1);
}
static int TestFNativeDirectoryExists(std::string_view path) {
    const std::string szPath{path};
    int p = role(szPath.c_str()), ok = decision() && objects[p] >= 100;
    event("probe", 3, p, ok, 0);
    return ok;
}
static int TestFValidManagedLibraryGame(APPSESSION &, std::string_view name, std::span<char> dir, int &remember, long long &played,
                                         std::span<char> exchange) {
    assert(name == "Game");
    int p = role(dir.data()), ok = decision() && objects[p] >= 100;
    remember = 4;
    played = 321;
    assert(exchange.size() > 8);
    std::copy_n("external", 9, exchange.begin());
    event("probe", 5, p, ok, 0);
    return ok;
}
static int TestFCreateLibraryGameStagePath(std::span<char> out) {
    assert(out.size() > 5);
    std::copy_n("stage", 6, out.begin());
    int ok = decision() && !objects[3];
    if (ok)
        objects[3] = 100;
    event("allocate", 1, 3, ok, 0);
    return ok;
}
static int TestFCreateLibraryGameHoldPath(std::span<char> out) {
    assert(out.size() > 4);
    std::copy_n("hold", 5, out.begin());
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
static int TestFCreateLibraryGameStage(APPSESSION &, std::string_view source, std::string_view old, std::string_view name,
                                       std::string_view stage, int player, long long played,
                                       std::string_view exchange) {
    assert(role(source) == 5 && role(stage) == 3 && old == "Game" && exchange == "external");
    if (profile == 2) {
        assert(name == "Game" && player == 4 && played == 321);
        return build(1);
    }
    assert(profile == 3 && name == "Copy" && player == 0 && played == 0);
    return build(4);
}
static int TestFBackupConflictingPlayerFiles(std::string_view stage, std::string_view name, int player, std::span<char> backup) {
    assert(role(stage) == 3 && name == "Game" && player == 0 && backup.size() > 6);
    std::copy_n("backup", 7, backup.begin());
    return build(2);
}
static int TestFStageConflictingTurn(std::string_view stage, std::string_view name, std::string_view source,
                                     int player, int32_t id, uint16_t turn) {
    assert(role(stage) == 3 && name == (profile == 2 ? "Game" : "Copy") && role(source) == 0 && player == 0 && id == 123 && turn == 11);
    return build(3);
}
static int TestFChooseSeparateCopyName(APPSESSION &, HWND hwnd, std::string_view name) {
    (void)hwnd;
    assert(name == "Game");
    int ok = decision();
    event("probe", 6, 1, ok, 0);
    return ok;
}
static int TestFLibraryNameAvailable(APPSESSION &, std::string_view root, std::string_view name) {
    assert(root.data() == szLibraryRoot && name == "Copy");
    int ok = decision();
    event("probe", 7, 1, ok, 0);
    return ok;
}
static size_t TestNativeStrlen(std::string_view szPath) {
    APPSESSION &app = AppCompatCallbackContext();

    if (szPath.data() == szLibraryRoot) {
        int ok = decision();
        event("probe", 8, 1, ok, 0);
        if (!ok)
            return sizeof(app.state.szBase);
    }
    return szPath.size();
}
// Family enumeration is a declared stage-only provider; cleanup aggregates the
// source enumeration plus final TestRemoveNativePath, including possible remnants.

static bool TestFNativeEnumerateDirectory(APPSESSION &, LIBRARYPUBLICATIONENUMERATION action, std::string_view path,
                                          const FNativeDirectoryEntryProc &) {
    (void)path;
    if (action == LIBRARYPUBLICATIONENUMERATION::importFiles || action == LIBRARYPUBLICATIONENUMERATION::backupFiles) {
        int ok = build(action == LIBRARYPUBLICATIONENUMERATION::importFiles ? 5 : 6);
        if (variant == 4 && !ok) return 0;
        return ok;
    }
    assert(action == LIBRARYPUBLICATIONENUMERATION::removeFiles);
    // Removal itself is performed by the immediately following source call.
    return 1;
}
static int TestFWriteLibraryMetadata(APPSESSION &, std::string_view path, std::string_view name, int16_t player) {
    int expected = variant >= 5 && variant <= 20 ? variant - 5 : variant == 21 ? -1 : 0;
    assert(role(path) == 3 && name == "Copy" && player == expected);
    return build(7);
}
static int TestCreateNativeDir(std::string_view path) {
    const std::string szPath{path};
    int p = role(szPath.c_str()), ok = decision() && !objects[p];
    if (ok)
        objects[p] = 100;
    event("allocate", 2, p, ok, 0);
    return ok ? 0 : -1;
}
static int     TestRemove(std::string_view path, int directory) { return directory ? TestRemoveNativePath(std::string{path}.c_str()) : TestRemoveNativeFile(std::string{path}.c_str()); }
static int     TestRename(std::string_view source, std::string_view target, int, int) { return move(std::string{source}.c_str(), std::string{target}.c_str()); }
static int16_t NativeLater(APPSESSION &app, int16_t hasTurn) {
    return FPublishSubsequentTurn(app, {szLibraryRoot, szLibraryPending, szLibraryName}, nullptr, "source", "target", "Game", 0, 123, 11, hasTurn);
}
static int16_t NativeFamily(APPSESSION &app) {
    LIBRARYIMPORT li{};
    li.grfPlayers = variant >= 5 && variant <= 20 ? 1u << (variant - 5) : variant == 21 ? 0 : 1;
    return FPublishImportedLibraryGame(app, {szLibraryRoot, szLibraryPending, szLibraryName}, li, "source");
}

int main(void) {
#ifdef STARS_UI_SDL
    APPSESSION &app = ui::app;
#else
    APPSESSION &app = host::app;
#endif
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);

    unsigned             inputMask;
    int                  inputProfile, inputVariant;
    LIBRARYPUBLICATIONIO io;
    io.pfnCchSnprintf = TestCchSnprintf;
    io.pfnDwTickCount = WITNESSADAPTER<&TestDwTickCount, decltype(io.pfnDwTickCount)>::Call;
    io.pfnLNativeTime = WITNESSADAPTER<&TestLNativeTime, decltype(io.pfnLNativeTime)>::Call;
    io.pfnFCopyNativeFileExclusive = WITNESSADAPTER<&TestFCopyNativeFileExclusive, decltype(io.pfnFCopyNativeFileExclusive)>::Call;
    io.pfnFInspectImportFile = WITNESSADAPTER<&TestFInspectImportFile, decltype(io.pfnFInspectImportFile)>::Call;
    io.pfnFNativeFileSize = TestFNativeFileSize;
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
    io.pfnEnumerateDirectory = TestFNativeEnumerateDirectory;
    io.pfnCreateNativeDir = WITNESSADAPTER<&TestCreateNativeDir, decltype(io.pfnCreateNativeDir)>::Call;
    io.pfnPathLength = TestNativeStrlen;
    LIBRARYPUBLICATIONIOSCOPE publicationScope(io);
    NATIVEPUBLISHIO           nativeIO;
    nativeIO.pfnRemove = TestRemove;
    nativeIO.pfnRename = TestRename;
    NATIVEPUBLISHIOSCOPE nativeScope(nativeIO);
    TEXTIO               textIO;
    textIO.pfnCompressed = TestPszGetCompressedString;
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
        int result = profile <= 1 ? NativeLater(app, profile == 0)
                     : profile == 2
                         ? FReplaceConflictingLibraryTurn(app, {szLibraryRoot, szLibraryPending, szLibraryName}, nullptr, "Game", "source", "target", 0, 123, 11)
                     : profile == 3 ? FImportConflictingTurnAsCopy(app, {szLibraryRoot, szLibraryPending, szLibraryName}, nullptr, "Game", "source", 0, 123, 11)
                                    : NativeFamily(app);
        clear();
        printf("[\"end\"]],\"result\":%d,\"calls\":%d,\"store\":[", result, count);
        for (int i = 0; i < 8; i++)
            printf("%s%d", i ? "," : "", objects[i]);
        printf("]}\n");
    }
    return 0;
}
