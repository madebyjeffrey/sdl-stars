#ifndef STARS_LIBRARYPUBLICATIONIO_H
#define STARS_LIBRARYPUBLICATIONIO_H
#include "librarypublication.h"

enum class LIBRARYPUBLICATIONENUMERATION { importFiles, backupFiles, removeFiles };

struct LIBRARYPUBLICATIONIO {
    std::size_t (*pfnPathLength)(std::string_view) = [](std::string_view szPath) { return szPath.size(); };
    NATIVEFORMATTER                        pfnCchSnprintf;
    decltype(&DwTickCount)                   pfnDwTickCount = DwTickCount;
    decltype(&LNativeTime)                   pfnLNativeTime = LNativeTime;
    decltype(&FCopyNativeFileExclusive)      pfnFCopyNativeFileExclusive = FCopyNativeFileExclusive;
    decltype(&FInspectImportFile)            pfnFInspectImportFile = FInspectImportFile;
    decltype(&FNativeFileSize)               pfnFNativeFileSize = FNativeFileSize;
    decltype(&FNativeDirectoryExists)        pfnFNativeDirectoryExists = FNativeDirectoryExists;
    decltype(&FValidManagedLibraryGame)      pfnFValidManagedLibraryGame = FValidManagedLibraryGame;
    decltype(&FCreateLibraryGameStagePath)   pfnFCreateLibraryGameStagePath = FCreateLibraryGameStagePath;
    decltype(&FCreateLibraryGameHoldPath)    pfnFCreateLibraryGameHoldPath = FCreateLibraryGameHoldPath;
    decltype(&FCreateLibraryGameStage)       pfnFCreateLibraryGameStage = FCreateLibraryGameStage;
    decltype(&FBackupConflictingPlayerFiles) pfnFBackupConflictingPlayerFiles = FBackupConflictingPlayerFiles;
    decltype(&FStageConflictingTurn)         pfnFStageConflictingTurn = FStageConflictingTurn;
    decltype(&FChooseSeparateCopyName)       pfnFChooseSeparateCopyName = FChooseSeparateCopyName;
    decltype(&FLibraryNameAvailable)         pfnFLibraryNameAvailable = FLibraryNameAvailable;
    std::int16_t (*pfnFWriteLibraryMetadata)(APPSESSION &, std::string_view, std::string_view, int) = FWriteLibraryMetadata;
    decltype(&RemoveLibraryGameTree)         pfnRemoveLibraryGameTree = RemoveLibraryGameTree;
    bool (*pfnEnumerateDirectory)(APPSESSION &, LIBRARYPUBLICATIONENUMERATION, std::string_view,
                                  const FNativeDirectoryEntryProc &) = [](APPSESSION &app, LIBRARYPUBLICATIONENUMERATION,
                                                                         std::string_view path, const FNativeDirectoryEntryProc &callback) {
        return FNativeEnumerateDirectory(app, path, callback);
    };
    decltype(&CreateNativeDir)               pfnCreateNativeDir = CreateNativeDir;
};
const LIBRARYPUBLICATIONIO &LibraryPublicationIO() noexcept;
class LIBRARYPUBLICATIONIOSCOPE {
    LIBRARYPUBLICATIONIO        io;
    const LIBRARYPUBLICATIONIO *lpPrev;

  public:
    explicit LIBRARYPUBLICATIONIOSCOPE(const LIBRARYPUBLICATIONIO &io) noexcept;
    ~LIBRARYPUBLICATIONIOSCOPE() noexcept;
    LIBRARYPUBLICATIONIOSCOPE(const LIBRARYPUBLICATIONIOSCOPE &) = delete;
    LIBRARYPUBLICATIONIOSCOPE &operator=(const LIBRARYPUBLICATIONIOSCOPE &) = delete;
};
#endif
