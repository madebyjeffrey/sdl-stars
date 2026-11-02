#ifndef STARS_LIBRARY_PUBLICATION_H
#define STARS_LIBRARY_PUBLICATION_H
#include "win.h"
#include <string_view>
struct LIBRARYIMPORT {
    const char *pszStem;
    const char *pszSource;
    const char *pszTarget;
    const char *pszName;
    std::int32_t     lidGame;
    std::uint32_t    grfPlayers;
    std::int16_t     rgturn[16];
    std::int16_t     fValid;
    std::int16_t     fSawXY;
    std::int16_t     fSawHost;
    std::int16_t     fBad;
    std::int16_t     fCopied;
    std::int16_t     fBackup;
    std::int16_t     fCopy;
};
// Borrow existing library path buffers at the publication boundary.
struct LIBRARYPATHS {
    char (&szLibraryRoot)[256];
    char (&szLibraryPending)[256];
    char (&szLibraryName)[256];
};
std::int16_t FCopyNativeFileContents(NATIVEFILE *pfSource, NATIVEFILE *pfTarget);
std::int16_t FCopyNativeFileExclusive(std::string_view szSource, std::string_view szTarget);
std::int16_t FReplaceConflictingLibraryTurn(APPSESSION &app, const LIBRARYPATHS &paths, HWND hwnd, const char *pszName, const char *pszSource, const char *pszTarget, int iplr,
                                       std::int32_t lidGame, std::uint16_t turn);
std::int16_t FImportConflictingTurnAsCopy(APPSESSION &app, const LIBRARYPATHS &paths, HWND hwnd, const char *pszName, const char *pszSource, int iplr, std::int32_t lidGame,
                                     std::uint16_t turn);
std::int16_t FInspectImportFile(std::string_view pszFile, DtFileType dt, int iplr, std::int32_t &lidGame, std::uint16_t *pturn, std::uint16_t *pwGen, std::int16_t &fMulti);
std::int16_t FValidManagedLibraryGame(APPSESSION &app, const char *pszName, std::span<char> pszDirectory,
                                      int &iplr, long long &lPlayed, std::span<char> pszExchangeName);
std::int16_t FCreateLibraryGameStagePath(std::span<char> pszStage);
std::int16_t FCreateLibraryGameHoldPath(std::span<char> pszHold);
std::int16_t FCreateLibraryGameStage(APPSESSION &app, const char *pszSource, const char *pszSourceName, const char *pszTargetName, const char *pszStage, int iplr, long long lPlayed,
                                const char *pszExchangeName);
std::int16_t FBackupConflictingPlayerFiles(const char *pszDirectory, const char *pszName, int iplr, std::span<char> pszBackup);
std::int16_t FStageConflictingTurn(const char *pszStage, const char *pszName, const char *pszSource, int iplr, std::int32_t lidGame, std::uint16_t turn);
std::int16_t FChooseSeparateCopyName(APPSESSION &app, HWND hwnd, const char *pszName);
std::int16_t FLibraryNameAvailable(APPSESSION &app, const char *pszDirectory, const char *pszName);
std::int16_t FWriteLibraryMetadata(const char *pszDir, const char *pszName, int iplr);
std::int16_t RemoveLibraryGameTree(APPSESSION &app, const char *pszDirectory);
std::int16_t CopyImportEntry(APPSESSION &app, void *lpData, std::string_view pszName);
std::int16_t CopyBackupDirectory(APPSESSION &app, void *lpData, std::string_view pszName);
std::int16_t RemoveImportFile(APPSESSION &app, void *lpData, std::string_view pszFile);
std::int16_t FPublishSubsequentTurn(APPSESSION &app, const LIBRARYPATHS &paths, HWND hwnd, const char *pszSource, const char *szTarget, const char *pszName, int iplr,
                               std::int32_t lidGame, std::uint16_t turn, std::int16_t fHasTurn);
std::int16_t FPublishImportedLibraryGame(APPSESSION &app, const LIBRARYPATHS &paths, LIBRARYIMPORT &li, const char *szDir);
#endif
