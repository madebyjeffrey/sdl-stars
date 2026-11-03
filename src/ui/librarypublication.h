#ifndef STARS_LIBRARY_PUBLICATION_H
#define STARS_LIBRARY_PUBLICATION_H
#include "win.h"
#include <string_view>
struct LIBRARYIMPORT {
    std::string_view pszStem;
    std::string_view pszSource;
    std::string_view pszTarget;
    std::string_view pszName;
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
    std::span<char, 256> szLibraryPending;
    std::span<char, 256> szLibraryName;
};
std::int16_t FCopyNativeFileContents(NATIVEFILE *pfSource, NATIVEFILE *pfTarget);
std::int16_t FCopyNativeFileExclusive(std::string_view szSource, std::string_view szTarget);
std::int16_t FReplaceConflictingLibraryTurn(APPSESSION &app, const LIBRARYPATHS &paths, HWND hwnd,
                                            std::string_view pszName, std::string_view pszSource,
                                            std::string_view pszTarget, int iplr, std::int32_t lidGame,
                                            std::uint16_t turn);
std::int16_t FImportConflictingTurnAsCopy(APPSESSION &app, const LIBRARYPATHS &paths, HWND hwnd,
                                          std::string_view pszName, std::string_view pszSource, int iplr,
                                          std::int32_t lidGame, std::uint16_t turn);
std::int16_t FInspectImportFile(std::string_view pszFile, DtFileType dt, int iplr, std::int32_t &lidGame, std::uint16_t *pturn, std::uint16_t *pwGen, std::int16_t &fMulti);
std::int16_t FValidManagedLibraryGame(APPSESSION &app, std::string_view pszName, std::span<char> pszDirectory,
                                      int &iplr, long long &lPlayed, std::span<char> pszExchangeName);
std::int16_t FCreateLibraryGameStagePath(std::span<char> pszStage);
std::int16_t FCreateLibraryGameHoldPath(std::span<char> pszHold);
std::int16_t FCreateLibraryGameStage(APPSESSION &app, std::string_view pszSource, std::string_view pszSourceName,
                                     std::string_view pszTargetName, std::string_view pszStage, int iplr,
                                     long long lPlayed, std::string_view pszExchangeName);
std::int16_t FBackupConflictingPlayerFiles(std::string_view pszDirectory, std::string_view pszName, int iplr,
                                           std::span<char> pszBackup);
std::int16_t FStageConflictingTurn(std::string_view pszStage, std::string_view pszName, std::string_view pszSource,
                                  int iplr, std::int32_t lidGame, std::uint16_t turn);
std::int16_t FChooseSeparateCopyName(APPSESSION &app, HWND hwnd, std::string_view pszName);
std::int16_t FLibraryNameAvailable(APPSESSION &app, std::string_view pszDirectory, std::string_view pszName);
std::int16_t FWriteLibraryMetadata(std::string_view pszDir, std::string_view pszName, int iplr);
std::int16_t RemoveLibraryGameTree(APPSESSION &app, std::string_view pszDirectory);
bool CopyImportEntry(APPSESSION &app, LIBRARYIMPORT *pli, std::string_view pszName);
bool CopyBackupDirectory(APPSESSION &app, LIBRARYIMPORT *pli, std::string_view pszName);
bool RemoveImportFile(APPSESSION &app, const char *pszRoot, std::string_view pszFile);
std::int16_t FPublishSubsequentTurn(APPSESSION &app, const LIBRARYPATHS &paths, HWND hwnd,
                                    std::string_view pszSource, std::string_view szTarget,
                                    std::string_view pszName, int iplr, std::int32_t lidGame,
                                    std::uint16_t turn, std::int16_t fHasTurn);
std::int16_t FPublishImportedLibraryGame(APPSESSION &app, const LIBRARYPATHS &paths, LIBRARYIMPORT &li,
                                         std::string_view szDir);
#endif
