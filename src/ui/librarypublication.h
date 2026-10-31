#ifndef STARS_LIBRARY_PUBLICATION_H
#define STARS_LIBRARY_PUBLICATION_H
#include "win.h"
typedef struct {
    const char *pszStem;
    const char *pszSource;
    const char *pszTarget;
    const char *pszName;
    int32_t     lidGame;
    uint32_t    grfPlayers;
    int16_t     rgturn[16];
    int16_t     fValid;
    int16_t     fSawXY;
    int16_t     fSawHost;
    int16_t     fBad;
    int16_t     fCopied;
    int16_t     fBackup;
    int16_t     fCopy;
} LIBRARYIMPORT;
// Borrow existing library path buffers at the publication boundary.
struct LIBRARYPATHS {
    char (&szLibraryRoot)[256];
    char (&szLibraryPending)[256];
    char (&szLibraryName)[256];
};
int16_t FCopyNativeFileContents(NATIVEFILE *pfSource, NATIVEFILE *pfTarget);
int16_t FCopyNativeFileExclusive(const char *pszSource, const char *pszTarget);
int16_t FReplaceConflictingLibraryTurn(const LIBRARYPATHS &paths, HWND hwnd, const char *pszName, const char *pszSource, const char *pszTarget, int iplr,
                                       int32_t lidGame, uint16_t turn);
int16_t FImportConflictingTurnAsCopy(const LIBRARYPATHS &paths, HWND hwnd, const char *pszName, const char *pszSource, int iplr, int32_t lidGame,
                                     uint16_t turn);
int16_t FInspectImportFile(const char *pszFile, DtFileType dt, int iplr, int32_t *plidGame, uint16_t *pturn, uint16_t *pwGen, int16_t *pfMulti);
int16_t FValidManagedLibraryGame(const char *pszName, char *pszDirectory, int *piplr, long long *plPlayed, char *pszExchangeName, size_t cchExchange);
int16_t FCreateLibraryGameStagePath(char *pszStage, size_t cchStage);
int16_t FCreateLibraryGameHoldPath(char *pszHold, size_t cchHold);
int16_t FCreateLibraryGameStage(const char *pszSource, const char *pszSourceName, const char *pszTargetName, const char *pszStage, int iplr, long long lPlayed,
                                const char *pszExchangeName);
int16_t FBackupConflictingPlayerFiles(const char *pszDirectory, const char *pszName, int iplr, char *pszBackup, size_t cchBackup);
int16_t FStageConflictingTurn(const char *pszStage, const char *pszName, const char *pszSource, int iplr, int32_t lidGame, uint16_t turn);
int16_t FChooseSeparateCopyName(HWND hwnd, const char *pszName);
int16_t FLibraryNameAvailable(const char *pszDirectory, const char *pszName);
int16_t FWriteLibraryMetadata(const char *pszDir, const char *pszName, int iplr);
int16_t RemoveLibraryGameTree(const char *pszDirectory);
int16_t CopyImportEntry(void *lpData, const char *pszName);
int16_t CopyBackupDirectory(void *lpData, const char *pszName);
int16_t RemoveImportFile(void *lpData, const char *pszFile);
int16_t FPublishSubsequentTurn(const LIBRARYPATHS &paths, HWND hwnd, const char *pszSource, const char *szTarget, const char *pszName, int iplr,
                               int32_t lidGame, uint16_t turn, int16_t fHasTurn);
int16_t FPublishImportedLibraryGame(const LIBRARYPATHS &paths, LIBRARYIMPORT &li, const char *szDir);
#endif
