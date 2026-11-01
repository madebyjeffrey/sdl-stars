#ifndef STARS_LIBRARYPUBLICATIONIO_H
#define STARS_LIBRARYPUBLICATIONIO_H
#include "librarypublication.h"

struct LIBRARYPUBLICATIONIO {
    std::size_t (*pfnPathLength)(const char *) = std::strlen;
    decltype(&CchSnprintf)                   pfnCchSnprintf = CchSnprintf;
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
    decltype(&FWriteLibraryMetadata)         pfnFWriteLibraryMetadata = FWriteLibraryMetadata;
    decltype(&RemoveLibraryGameTree)         pfnRemoveLibraryGameTree = RemoveLibraryGameTree;
    decltype(&FNativeEnumerateDirectory)     pfnFNativeEnumerateDirectory = FNativeEnumerateDirectory;
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
