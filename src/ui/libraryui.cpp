#include "win.h"
#include "librarypublication.h"
#include "library_publication_io.h"
#include "library_file_io.h"
#include "text_io.h"
#include "control_io.h"
#include "library_path_io.h"
#include "profile_text_io.h"
#include "choice_io.h"
#include "folder_io.h"
#include "path_io.h"
#include "time_io.h"
#include "alert_io.h"
#include "record_io.h"
#include "desktop_choice_io.h"
#include "ownership.h"
#include <stars_compat.h>
#include <SDL3/SDL.h>
#include <errno.h>
#include "libraryzip.h"

/* Desktop metadata is deliberately outside the legacy game records. A folder
 * is listed only after its complete generated family and metadata are saved. */
static char szLibraryRoot[256];
static char szLibraryPending[256];
static char szLibraryName[256];
static char szLibraryExchangeName[256];
static char szRaceLibraryRoot[256];
static char szRaceLibraryPath[512];
static char szRaceLibraryName[256];
static char szRaceLibraryTemp[512];
typedef struct {
    char szName[256];
    int iplr;
    uint16_t grfPlayers;
    uint16_t rgturn[16];
} LIBRARYGAMELISTITEM;
static NATIVEARRAYOWNER<LIBRARYGAMELISTITEM, SDL_free> rgLibraryGameList;
static int cLibraryGameList;
static int cLibraryGameListMax;
static uint32_t idRaceStage;
static int16_t fRaceLibraryNameDialog;
static int16_t fNewLibraryRace;
static uint32_t idLibraryExportStage;
static uint32_t idLibraryZipStage;
static int16_t fHostLibraryView;
static const char *pszRaceLibraryNameOverride;

typedef struct LibraryContinue {
    char szName[256];
    int iPlayer;
    int iYear;
    long long lPlayed;
    int16_t fFound;
    int16_t fTurnAvailable;
} LIBRARYCONTINUE;

static LIBRARYCONTINUE continueLibrary;
static int16_t FFindContinueLibraryGame(void);
static int16_t FCopyNativeFile(const char *pszSource, const char *pszTarget);
static int16_t FSelectedLibraryGameName(HWND hwnd, char *pszName, size_t cchName);
static void *PvLibraryFolderContext(HWND hwnd) {
    const uint64_t id = StarsCompatWindowId(hwnd);
    // SDL carries one pointer-sized opaque value; no window pointer outlives UI.
    return id <= UINTPTR_MAX ? reinterpret_cast<void *>(static_cast<uintptr_t>(id)) : nullptr;
}

static void SDLCALL LibraryExportFolderSelected(void *lpData, const char * const *rgpszPath, int iFilter) noexcept;
static int16_t FWriteLibraryMetadataValues(const char *pszDir, const char *pszName, int iplr,
                                           long long lPlayed, const char *pszExchangeName);

typedef struct {
    PLAYER plr;
    char szPass[sizeof(szRacePass)];
    char szFile[sizeof(szRaceFile)];
    char szPath[sizeof(szRaceLibraryPath)];
    int16_t fNew;
} RACESTATE;

typedef struct {
    const char *pszDirectory;
    const char *pszName;
    int16_t fFound;
} LIBRARYNAMECHECK;

static void SaveRaceState(RACESTATE *prs) {
    prs->plr = vplr;
    prs->fNew = fNewLibraryRace;
    memcpy(prs->szPass, szRacePass, sizeof(prs->szPass));
    memcpy(prs->szFile, szRaceFile, sizeof(prs->szFile));
    memcpy(prs->szPath, szRaceLibraryPath, sizeof(prs->szPath));
}

static void RestoreRaceState(const RACESTATE *prs, int16_t fRestoreRace, int16_t fRestorePath) {
    if (fRestoreRace) {
        vplr = prs->plr;
        memcpy(szRacePass, prs->szPass, sizeof(prs->szPass));
        memcpy(szRaceFile, prs->szFile, sizeof(prs->szFile));
    }
    if (fRestorePath) {
        memcpy(szRaceLibraryPath, prs->szPath, sizeof(prs->szPath));
        fNewLibraryRace = prs->fNew;
    }
}

static int16_t FindLibraryName(void *lpData, const char *pszFile) {
    LIBRARYNAMECHECK *plnc = (LIBRARYNAMECHECK *)lpData;
    if (!SDL_strcasecmp(plnc->pszName, pszFile)) {
        plnc->fFound = TRUE;
        return FALSE;
    }
    return TRUE;
}

int16_t FLibraryNameAvailable(const char *pszDirectory, const char *pszName) {
    LIBRARYNAMECHECK lnc;
    lnc.pszDirectory = pszDirectory;
    lnc.pszName = pszName;
    lnc.fFound = FALSE;
    if (!FNativeEnumerateDirectory(lnc.pszDirectory, FindLibraryName, &lnc)) return FALSE;
    return !lnc.fFound;
}

static int16_t FRaceLibraryNameAvailable(const char *pszName) {
    return FLibraryNameAvailable(szRaceLibraryRoot, pszName);
}

static int16_t FPortableRaceFilename(const char *pszName) {
    const unsigned char *pch;
    char szDevice[256];
    size_t cch = strlen(pszName);
    size_t i;
    const char *pchExt = strrchr(pszName, '.');
    const char *pchDigit;
    if (!pchExt || pchExt == pszName || SDL_tolower(pchExt[1]) != 'r' || !pchExt[2] ||
        pchExt[2] == '0' || pchExt[2] < '1' || pchExt[2] > '9' || cch >= 240 || pszName[0] == ' ' || pszName[0] == '.' ||
        pszName[cch - 1] == ' ' || pszName[cch - 1] == '.') return FALSE;
    /* Windows reserves these stems even when followed by an extension. */
    for (i = 0; i < cch && pszName[i] != '.'; i++) szDevice[i] = pszName[i];
    szDevice[i] = 0;
    if (!SDL_strcasecmp(szDevice, "CON") || !SDL_strcasecmp(szDevice, "PRN") ||
        !SDL_strcasecmp(szDevice, "AUX") || !SDL_strcasecmp(szDevice, "NUL") ||
        !SDL_strcasecmp(szDevice, "CLOCK$") ||
        (i == 4 && (!SDL_strncasecmp(szDevice, "COM", 3) || !SDL_strncasecmp(szDevice, "LPT", 3)) &&
         szDevice[3] >= '1' && szDevice[3] <= '9')) return FALSE;
    for (pchDigit = pchExt + 3; *pchDigit; pchDigit++)
        if (*pchDigit < '0' || *pchDigit > '9') return FALSE;
    if (strlen(pchExt + 2) > 3) return FALSE;
    for (pch = (const unsigned char *)pszName; *pch; pch++)
        if (*pch < 32 || *pch == 127 || strchr("<>:\"/\\|?*", *pch)) return FALSE;
    if (strlen(szRaceLibraryRoot) + cch + 2 >= sizeof(szBase)) return FALSE;
    return TRUE;
}

/* The Windows CRT environment uses its code page; SDL supplies UTF-8 paths. */
static int16_t FInitRaceLibrary(void) {
    const char *pszOverride = SDL_getenv("STARS_LIBRARY_DIR");
    const char *pszSettings = SDL_getenv("STARS_SETTINGS_DIR");
    char *pszRoot = pszOverride || pszSettings ? NULL : CompatPathIO().pfnGetPrefPath("", "Stars!");
    int cch;
    NATIVEOWNER<char, SDL_free> rootPath(pszRoot);
    if (!pszOverride && !pszSettings && !pszRoot) return FALSE;
    cch = CchSnprintf(szRaceLibraryRoot, sizeof(szRaceLibraryRoot), "%s%sRace Designs",
                      pszOverride ? pszOverride : pszSettings ? pszSettings : pszRoot,
                      pszOverride || pszSettings ? szDirSep : "");
    rootPath.reset();
    return cch > 0 && cch < (int)sizeof(szRaceLibraryRoot) && FEnsureNativeDir(szRaceLibraryRoot);
}

static int16_t FCreateRaceStagePath(char *pszDirectory, size_t cchDirectory,
                                    char *pszFile, size_t cchFile) {
    uint32_t dwTick = DwTickCount();
    int i;
    int cch;
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(pszDirectory, cchDirectory, "%s%s.stars-race-stage-%u-%u",
                          szRaceLibraryRoot, szDirSep, dwTick, idRaceStage++);
        if (cch <= 0 || (size_t)cch >= cchDirectory) return FALSE;
        if (CreateNativeDir(pszDirectory) != 0) {
            if (FNativeDirectoryExists(pszDirectory)) continue;
            return FALSE;
        }
        cch = CchSnprintf(pszFile, cchFile, "%s%sdesign.r1", pszDirectory, szDirSep);
        if (cch <= 0 || (size_t)cch >= cchFile) {
            RemoveNativePath(pszDirectory);
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

static void SuggestRaceFilename(char *pszName) {
    char szStem[240];
    const char *pch = strrchr(pszName, '.');
    size_t cchStem = pch ? (size_t)(pch - pszName) : strlen(pszName);
    int i;
    if (cchStem >= sizeof(szStem)) cchStem = sizeof(szStem) - 1;
    cchStem = CchUtf8Prefix(pszName, cchStem);
    memcpy(szStem, pszName, cchStem);
    szStem[cchStem] = 0;
    for (i = 1; i < 1000; i++) {
        CchSnprintf(pszName, 256, "%s.r%d", szStem, i);
        if (FRaceLibraryNameAvailable(pszName)) return;
    }
    pszName[0] = 0;
}

static int16_t FFormatLibraryFamilyPath(char *pszPath, size_t cchPath, const char *pszDirectory,
                                        const char *pszName, DtFileType dt, int iplr) {
    const char *pszExtension;
    int cch;
    if (dt == dtXY) pszExtension = "xy";
    else if (dt == dtHost) pszExtension = "hst";
    else pszExtension = dt == dtTurn ? "m" : dt == dtHist ? "h" : "x";
    if (dt == dtXY || dt == dtHost)
        cch = CchSnprintf(pszPath, cchPath, "%s%s%s.%s", pszDirectory, szDirSep, pszName, pszExtension);
    else cch = CchSnprintf(pszPath, cchPath, "%s%s%s.%s%d", pszDirectory, szDirSep, pszName, pszExtension, iplr + 1);
    return cch > 0 && (size_t)cch < cchPath;
}

static int16_t FInitLibrary(void) {
    const char *pszOverride;
    const char *pszSettings;
    char *pszRoot;
    int cch;

    pszOverride = SDL_getenv("STARS_LIBRARY_DIR");
    pszSettings = SDL_getenv("STARS_SETTINGS_DIR");
    pszRoot = pszOverride || pszSettings ? NULL : CompatPathIO().pfnGetPrefPath("", "Stars!");
    NATIVEOWNER<char, SDL_free> rootPath(pszRoot);
    if (!pszOverride && !pszSettings && !pszRoot) return FALSE;
    cch = CchSnprintf(szLibraryRoot, sizeof(szLibraryRoot), "%s%sGames",
                      pszOverride ? pszOverride : pszSettings ? pszSettings : pszRoot,
                      pszOverride || pszSettings ? szDirSep : "");
    rootPath.reset();
    return cch > 0 && cch < (int)sizeof(szLibraryRoot) && FEnsureNativeDir(szLibraryRoot);
}

int16_t FPathUnderLibrary(const char *pszRoot, const char *pszFile) {
    size_t cchRoot, cchFile;
#ifdef _WIN32
    size_t ich;
#endif
    if (!pszRoot || !pszFile) return FALSE;
    cchRoot = strlen(pszRoot);
    cchFile = strlen(pszFile);
    if (cchRoot == 0 || cchFile <= cchRoot) return FALSE;
#ifdef _WIN32
    /* Pickers return '/', while library joins and Windows arguments may use '\\'. */
    for (ich = 0; ich < cchRoot; ich++) {
        if ((pszRoot[ich] == '/' || pszRoot[ich] == '\\') &&
            (pszFile[ich] == '/' || pszFile[ich] == '\\')) continue;
        if (SDL_tolower((unsigned char)pszRoot[ich]) != SDL_tolower((unsigned char)pszFile[ich])) return FALSE;
    }
    return pszFile[cchRoot] == '/' || pszFile[cchRoot] == '\\';
#else
    return memcmp(pszRoot, pszFile, cchRoot) == 0 && pszFile[cchRoot] == '/';
#endif
}

static char *PchLibrarySeparator(char *pszPath) {
    char *pch = strrchr(pszPath, chDirSep);
#ifdef _WIN32
    char *pchSlash = strrchr(pszPath, '/');
    if (pchSlash && (!pch || pchSlash > pch)) pch = pchSlash;
#endif
    return pch;
}

int16_t FIsManagedLibraryGameFile(const char *pszFile) {
    return FInitLibrary() && LibraryPathIO().pfnUnder(szLibraryRoot, pszFile);
}

int16_t FIsManagedLibraryRaceFile(const char *pszFile) {
    return FInitRaceLibrary() && LibraryPathIO().pfnUnder(szRaceLibraryRoot, pszFile);
}

static int16_t FPortableLibraryName(const char *pszName) {
    const unsigned char *pch;
    char szDevice[256];
    size_t cch, i;

    cch = strlen(pszName);
    if (cch == 0 || cch >= sizeof(szDevice) || pszName[0] == ' ' || pszName[0] == '.' || pszName[cch - 1] == ' ' || pszName[cch - 1] == '.') return FALSE;
    for (pch = (const unsigned char *)pszName; *pch; pch++)
        if (*pch < 32 || *pch == 127 || strchr("<>:\"/\\|?*", *pch)) return FALSE;
    /* Windows reserves these stems even when followed by an extension. */
    for (i = 0; i < cch && pszName[i] != '.'; i++) szDevice[i] = pszName[i];
    szDevice[i] = 0;
    if (!SDL_strcasecmp(szDevice, "CON") || !SDL_strcasecmp(szDevice, "PRN") ||
        !SDL_strcasecmp(szDevice, "AUX") || !SDL_strcasecmp(szDevice, "NUL") ||
        !SDL_strcasecmp(szDevice, "CLOCK$") ||
        (i == 4 && (!SDL_strncasecmp(szDevice, "COM", 3) || !SDL_strncasecmp(szDevice, "LPT", 3)) &&
         szDevice[3] >= '1' && szDevice[3] <= '9')) return FALSE;
    /* Longest backup path: root/name/backup.999/name.m16 plus NUL. */
    return strlen(szLibraryRoot) + 2 * cch + 18 < sizeof(szBase);
}

static void SuggestLibraryName(char *pszName, int16_t fRevision) {
    char szOriginal[256];
    char szSuffix[32];
    int i, cch, cchMax;

    strcpy(szOriginal, pszName);
    cchMax = ((int)sizeof(szBase) - (int)strlen(szLibraryRoot) - 19) / 2;
    for (i = 2; i < 10000 && !FLibraryNameAvailable(szLibraryRoot, pszName); i++) {
        CchSnprintf(szSuffix, sizeof(szSuffix), fRevision ? TextIO().pfnCompressed(idsLibraryGameRevisionSuffix) : " (%d)", i);
        cch = ((((int)strlen(szOriginal)) < ((((0) > (cchMax - (int)strlen(szSuffix))) ? (0) : (cchMax - (int)strlen(szSuffix))))) ? ((int)strlen(szOriginal)) : ((((0) > (cchMax - (int)strlen(szSuffix))) ? (0) : (cchMax - (int)strlen(szSuffix)))));
        /* A shortened suggestion must not split a UTF-8 character. */
        while (cch > 0 && ((unsigned char)szOriginal[cch] & 0xc0) == 0x80) cch--;
        CchSnprintf(pszName, 256, "%.*s%s", cch, szOriginal, szSuffix);
    }
}

static void GetGameLibraryName(const char *pszName, char *pszNameOut) {
    size_t i, cch;
    int cchMax = ((int)sizeof(szBase) - (int)strlen(szLibraryRoot) - 19) / 2;

    CchCopyUtf8(pszNameOut, 256, pszName, CchUtf8Prefix(pszName, cchMax));
    for (i = 0; pszNameOut[i]; i++)
        if ((unsigned char)pszNameOut[i] < 32 || pszNameOut[i] == 127 || strchr("<>:\"/\\|?*", pszNameOut[i])) pszNameOut[i] = '_';
    cch = strlen(pszNameOut);
    while (cch != 0 && (pszNameOut[cch - 1] == ' ' || pszNameOut[cch - 1] == '.')) pszNameOut[--cch] = 0;
    while (pszNameOut[0] == ' ' || pszNameOut[0] == '.') memmove(pszNameOut, pszNameOut + 1, strlen(pszNameOut));
    // The game loader treats the last period as a file extension.
    for (i = 0; pszNameOut[i]; i++) if (pszNameOut[i] == '.') pszNameOut[i] = '_';
    if (!pszNameOut[0]) {
        pszName = TextIO().pfnCompressed(idsLibraryDefaultGameName);
        CchCopyUtf8(pszNameOut, 256, pszName, CchUtf8Prefix(pszName, cchMax));
    }
    if (!FPortableLibraryName(pszNameOut)) {
        // Reserved device names need a prefix; leave the stored game title intact.
        pszNameOut[CchUtf8Prefix(pszNameOut, StarsMax(0, cchMax - 1))] = 0;
        memmove(pszNameOut + 1, pszNameOut, strlen(pszNameOut) + 1);
        pszNameOut[0] = '_';
    }
}

INT_PTR CALLBACK LibraryGameRevisionDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    char szMessage[512];
    POINT16 pt = {-1, -1};

    switch (message) {
    case WM_INITDIALOG:
        ControlIO().pfnFUiSetWindowText(hwnd, TextIO().pfnCompressed(idsLibraryGameDuplicateTitle));
        CchSnprintf(szMessage, sizeof(szMessage), TextIO().pfnCompressed(idsLibraryGameDuplicatePrompt), (const char *)lParam);
        SetDlgItemText(hwnd, IDC_LIBRARY_GAME_REVISION_TEXT, szMessage);
        SetDlgItemText(hwnd, IDOK, TextIO().pfnCompressed(idsLibraryGameAddRevision));
        SetDlgItemText(hwnd, IDCANCEL, TextIO().pfnCompressed(idsLibraryGameBackToWizard));
        ControlIO().pfnStickyDlgPos(hwnd, &pt, TRUE);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

INT_PTR CALLBACK LibraryNameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    char *pszName;
    char szName[256], szMessage[256];
    StringId ids;
    RECT rc, rcOwner, rcWork;
    int x, y;

    switch (message) {
    case WM_INITDIALOG:
        if (!SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0))
            SetRect(&rcWork, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
        if (!IsWindowVisible(GetParent(hwnd)) || !GetWindowRect(GetParent(hwnd), &rcOwner)) rcOwner = rcWork;
        GetWindowRect(hwnd, &rc);
        x = rcOwner.left + ((rcOwner.right - rcOwner.left) - (rc.right - rc.left)) / 2;
        y = rcOwner.top + ((rcOwner.bottom - rcOwner.top) - (rc.bottom - rc.top)) / 2;
        x = (((rcWork.left) > ((((x) < (rcWork.right - (rc.right - rc.left))) ? (x) : (rcWork.right - (rc.right - rc.left))))) ? (rcWork.left) : ((((x) < (rcWork.right - (rc.right - rc.left))) ? (x) : (rcWork.right - (rc.right - rc.left)))));
        y = (((rcWork.top) > ((((y) < (rcWork.bottom - (rc.bottom - rc.top))) ? (y) : (rcWork.bottom - (rc.bottom - rc.top))))) ? (rcWork.top) : ((((y) < (rcWork.bottom - (rc.bottom - rc.top))) ? (y) : (rcWork.bottom - (rc.bottom - rc.top)))));
        ControlIO().pfnFUiSetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowLongPtr(hwnd, DWLP_USER, lParam);
        LUiSendDlgItemMessage(hwnd, IDC_EDIT1, EM_LIMITTEXT, 255, 0);
        SetDlgItemText(hwnd, IDC_EDIT1, (char *)lParam);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) { ControlIO().pfnEndDialog(hwnd, FALSE); return TRUE; }
        if (LOWORD(wParam) != IDOK) break;
        pszName = (char *)GetWindowLongPtr(hwnd, DWLP_USER);
        GetDlgItemText(hwnd, IDC_EDIT1, szName, sizeof(szName));
        if (fRaceLibraryNameDialog) {
            if (!FPortableRaceFilename(szName)) ids = idsLibraryInvalidName;
            else if (!FRaceLibraryNameAvailable(szName)) {
                SuggestRaceFilename(szName);
                SetDlgItemText(hwnd, IDC_EDIT1, szName);
                ids = idsLibraryNameTaken;
            } else {
                strcpy(pszName, szName);
                ControlIO().pfnEndDialog(hwnd, TRUE);
                return TRUE;
            }
        } else if (!FPortableLibraryName(szName)) ids = idsLibraryInvalidName;
        else if (!FLibraryNameAvailable(szLibraryRoot, szName)) {
            SuggestLibraryName(szName, FALSE);
            SetDlgItemText(hwnd, IDC_EDIT1, szName);
            ids = idsLibraryNameTaken;
        } else {
            strcpy(pszName, szName);
            ControlIO().pfnEndDialog(hwnd, TRUE);
            return TRUE;
        }
        /* Each lookup reuses szLastStrGet; retain the body before fetching the caption. */
        strcpy(szMessage, TextIO().pfnCompressed(ids));
        CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage, TextIO().pfnCompressed(idsLibraryGames), MB_OK | MB_ICONEXCLAMATION);
        return TRUE;
    }
    return FALSE;
}

int16_t FChooseLibraryGameName(HWND hwnd, char *pszFile, const char *pszName) {
    char *pch;
    char szMessage[256];

    if (!FInitLibrary() || strlen(szLibraryRoot) + 20 >= sizeof(szBase)) goto LFail;
    szLibraryExchangeName[0] = 0;
    if (pszName) GetGameLibraryName(pszName, szLibraryName);
    else {
        pch = strrchr(pszFile, chDirSep);
        CchSnprintf(szLibraryName, sizeof(szLibraryName), "%s", pch ? pch + 1 : pszFile);
        pch = strrchr(szLibraryName, '.');
        if (pch) *pch = 0;
        if (!FPortableLibraryName(szLibraryName)) strcpy(szLibraryName, TextIO().pfnCompressed(idsLibraryDefaultGameName));
    }
    if (pszName) {
        if (!FLibraryNameAvailable(szLibraryRoot, szLibraryName)) {
            SuggestLibraryName(szLibraryName, TRUE);
            if (ControlIO().pfnLUiDialogBox(hInst, MAKEINTRESOURCE(IDD_LIBRARY_GAME_REVISION), hwnd,
                               LibraryGameRevisionDlg, (LPARAM)szLibraryName) != TRUE) return FALSE;
        }
    } else {
        SuggestLibraryName(szLibraryName, FALSE);
        if (ControlIO().pfnLUiDialogBox(hInst, MAKEINTRESOURCE(IDD_LIBRARY_NAME), hwnd, LibraryNameDlg, (LPARAM)szLibraryName) != TRUE) return FALSE;
    }
    if (!FPortableLibraryName(szLibraryName)) goto LFail;
    CchSnprintf(szLibraryPending, sizeof(szLibraryPending), "%s%s%s", szLibraryRoot, szDirSep, szLibraryName);
    /* Reserve only a newly created folder; never write into an existing copy. */
    if (!FLibraryNameAvailable(szLibraryRoot, szLibraryName) || CreateNativeDir(szLibraryPending) != 0) {
        szLibraryPending[0] = 0;
        goto LFail;
    }
    CchSnprintf(pszFile, 256, "%s%s%s.xy", szLibraryPending, szDirSep, szLibraryName);
    return TRUE;
LFail:
    strcpy(szMessage, TextIO().pfnCompressed(idsLibraryStorageFailed));
    CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage, TextIO().pfnCompressed(idsLibraryGames), MB_OK | MB_ICONHAND);
    return FALSE;
}

static int16_t FChooseLibraryRaceName(HWND hwnd, const char *pszSuggested, char *pszPath, size_t cchPath) {
    char szMessage[256];
    if (!FInitRaceLibrary()) return FALSE;
    CchSnprintf(szRaceLibraryName, sizeof(szRaceLibraryName), "%s", pszSuggested);
    SuggestRaceFilename(szRaceLibraryName);
    if (!szRaceLibraryName[0]) return FALSE;
    fRaceLibraryNameDialog = TRUE;
    if (ControlIO().pfnLUiDialogBox(hInst, MAKEINTRESOURCE(IDD_LIBRARY_NAME), hwnd, LibraryNameDlg,
                       (LPARAM)szRaceLibraryName) != TRUE) {
        fRaceLibraryNameDialog = FALSE;
        return FALSE;
    }
    fRaceLibraryNameDialog = FALSE;
    if (CchSnprintf(pszPath, cchPath, "%s%s%s", szRaceLibraryRoot, szDirSep, szRaceLibraryName) <= 0 ||
        !FRaceLibraryNameAvailable(szRaceLibraryName)) {
        strcpy(szMessage, TextIO().pfnCompressed(idsLibraryStorageFailed));
        CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage, TextIO().pfnCompressed(idsLibraryGames), MB_OK | MB_ICONHAND);
        return FALSE;
    }
    return TRUE;
}

int16_t FWriteLibraryMetadata(const char *pszDir, const char *pszName, int iplr) {
    return FWriteLibraryMetadataValues(pszDir, pszName, iplr, (long long)LNativeTime(),
                                       szLibraryExchangeName[0] ? szLibraryExchangeName : pszName);
}

static int16_t FWriteLibraryMetadataValues(const char *pszDir, const char *pszName, int iplr,
                                           long long lPlayed, const char *pszExchangeName) {
    char szFile[256], szTemp[256];
    NATIVEFILE *pf;
    int fWritten, fClosed, cch;

    cch = CchSnprintf(szFile, sizeof(szFile), "%s%s.library.tmp", pszDir, szDirSep);
    if (cch <= 0 || (size_t)cch >= sizeof(szFile)) return FALSE;
    pf = PfOpenNativeFile(szFile, "wb");
    if (!pf) return FALSE;
    fWritten = CchNativePrintf(pf, "2\n%s\n%d\n%lld\n%s\n", pszName, iplr, lPlayed, pszExchangeName) > 0;
    fClosed = ICloseNativeFile(pf) == 0;
    if (!fWritten || !fClosed) { RemoveNativeFile(szFile); return FALSE; }
    cch = CchSnprintf(szFile, sizeof(szFile), "%s%s.library", pszDir, szDirSep);
    if (cch <= 0 || (size_t)cch >= sizeof(szFile)) return FALSE;
    /* Publish complete metadata, preserving the previous session on failure. */
    cch = CchSnprintf(szTemp, sizeof(szTemp), "%s%s.library.tmp", pszDir, szDirSep);
    if (cch <= 0 || (size_t)cch >= sizeof(szTemp)) return FALSE;
    return ReplaceNativeFile(szTemp, szFile) == 0;
}

static int16_t RemovePendingFile(void *lpUnused, const char *pszFile) {
    char szFile[256];
    (void)lpUnused;
    CchSnprintf(szFile, sizeof(szFile), "%s%s%s", szLibraryPending, szDirSep, pszFile);
    /* Failed generation can leave an empty directory at a file destination. */
    RemoveNativePath(szFile);
    return TRUE;
}

static int16_t FReadLibraryHdr(NATIVEFILE *pf, HDR *phdr) {
    std::byte rgb[cbWireHdr];

    return CReadNative(rgb, cbWireHdr, 1, pf) == 1 && FDecodeHdr(rgb, *phdr);
}

static int16_t FReadLibraryBof(NATIVEFILE *pf, RTBOF *pbof) {
    std::byte rgb[cbWireBof];

    return CReadNative(rgb, cbWireBof, 1, pf) == 1 && FDecodeBof(rgb, *pbof);
}

/* GenerateWorld's original companion-write loop ignores writer return values.
 * Verify complete record framing without replacing the loaded player state. */
static int16_t FCompleteGeneratedFile(const char *pszFile, DtFileType dt, int iplr) {
    NATIVEFILE *pf;
    HDR hdr;
    RTBOF rtbof;
    long cbFile, ib;
    int16_t fComplete;

    pf = PfOpenNativeFile(pszFile, "rb");
    if (!pf) return FALSE;
    fComplete = FALSE;
    if (ISeekNativeFile(pf, 0, SEEK_END) != 0 || (cbFile = LNativePosition(pf)) < 22 || ISeekNativeFile(pf, 0, SEEK_SET) != 0) goto LDone;
    if (!FReadLibraryHdr(pf, &hdr) || hdr.rt != rtBOF || hdr.cb != 16 ||
        !FReadLibraryBof(pf, &rtbof) || memcmp(rtbof.rgid, "J3J3", 4) ||
        rtbof.lidGame != game.lid || rtbof.turn != game.turn || rtbof.dt != dt || rtbof.iPlayer != iplr) goto LDone;
    /* XY stores raw four-byte star packs between its GAME and EOF records. */
    if (dt == dtXY) {
        if (cbFile != 18 + 66 + 4L * game.cPlanMax + 4 || ISeekNativeFile(pf, -4, SEEK_END) != 0) goto LDone;
        fComplete = FReadLibraryHdr(pf, &hdr) && hdr.rt == rtEOF && hdr.cb == 2;
        goto LDone;
    }
    while (FReadLibraryHdr(pf, &hdr)) {
        ib = LNativePosition(pf);
        if (ib < 0 || ib + hdr.cb > cbFile) break;
        if (hdr.rt == rtEOF) {
            fComplete = hdr.cb == 2 && ib + hdr.cb == cbFile;
            break;
        }
        if (ISeekNativeFile(pf, hdr.cb, SEEK_CUR) != 0) break;
    }
LDone:
    ICloseNativeFile(pf);
    return fComplete;
}



int16_t FCopyNativeFileContents(NATIVEFILE *pfSource, NATIVEFILE *pfTarget) {
    char rgb[8192];
    size_t cb;
    int fOkay;

    fOkay = TRUE;
    while ((cb = LibraryFileIO().pfnRead(rgb, 1, sizeof(rgb), pfSource)) != 0) {
        if (LibraryFileIO().pfnWrite(rgb, 1, cb, pfTarget) != cb) { fOkay = FALSE; break; }
    }
    if (LibraryFileIO().pfnError(pfSource)) fOkay = FALSE;
    if (LibraryFileIO().pfnClose(pfSource) != 0) fOkay = FALSE;
    if (LibraryFileIO().pfnClose(pfTarget) != 0) fOkay = FALSE;
    return fOkay;
}

static int16_t FCopyNativeFile(const char *pszSource, const char *pszTarget) {
    NATIVEFILE *pfSource, *pfTarget;
    int fOkay;

    pfSource = PfOpenNativeFile(pszSource, "rb");
    if (!pfSource) return FALSE;
    pfTarget = PfOpenNativeFile(pszTarget, "wb");
    if (!pfTarget) { ICloseNativeFile(pfSource); return FALSE; }
    fOkay = FCopyNativeFileContents(pfSource, pfTarget);
    if (!fOkay) RemoveNativeFile(pszTarget);
    return fOkay;
}

int16_t FCopyNativeFileExclusive(const char *pszSource, const char *pszTarget) {
    NATIVEFILE *pfSource, *pfTarget;
    int iErrno;
    int fOkay;

    pfSource = LibraryFileIO().pfnOpen(pszSource, "rb");
    if (!pfSource) return FALSE;
    pfTarget = LibraryFileIO().pfnOpen(pszTarget, "wbx");
    if (!pfTarget) {
        iErrno = errno;
        LibraryFileIO().pfnClose(pfSource);
        errno = iErrno;
        return FALSE;
    }
    fOkay = FCopyNativeFileContents(pfSource, pfTarget);
    if (!fOkay) RemoveNativeFile(pszTarget);
    return fOkay;
}

static int16_t FCopyRaceExport(const char *pszSource, const char *pszTarget,
                               char *pszTemp, size_t cchTemp) {
    const char *pchSlash;
    size_t cchDirectory;
    uint32_t dwTick;
    int i, cch;

    pchSlash = strrchr(pszTarget, chDirSep);
    cchDirectory = pchSlash ? (size_t)(pchSlash - pszTarget) + 1 : 0;
    if (cchDirectory > INT_MAX) return FALSE;
    dwTick = DwTickCount();
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(pszTemp, cchTemp, "%.*s.stars-race-export-%u-%u.tmp",
                          (int)cchDirectory, pszTarget, dwTick, idRaceStage++);
        if (cch <= 0 || (size_t)cch >= cchTemp) return FALSE;
        errno = 0;
        if (FCopyNativeFileExclusive(pszSource, pszTemp)) return TRUE;
        if (errno != EEXIST) return FALSE;
    }
    return FALSE;
}

int16_t FSaveLibraryRace(HWND hwnd, const char *pszName, PLAYER *pplr) {
    char szPath[512];
    char szTempDir[512];
    uint16_t icksum;
    int16_t fOkay;
    if (!FInitRaceLibrary() || !FPortableRaceFilename(pszName)) return FALSE;
    CchSnprintf(szPath, sizeof(szPath), "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    if (!FCreateRaceStagePath(szTempDir, sizeof(szTempDir), szRaceLibraryTemp, sizeof(szRaceLibraryTemp))) return FALSE;
    if (!RecordIO().pfnFCreateFile(dtRace, iplrNone, szRaceLibraryTemp)) {
        RemoveNativeFile(szRaceLibraryTemp);
        RemoveNativePath(szTempDir);
        return FALSE;
    }
    WriteRtPlr(pplr, NULL);
    icksum = IRaceChecksum(pplr);
    WriteRtEof(icksum);
    RecordIO().pfnStreamClose();
    fOkay = ReplaceNativeFile(szRaceLibraryTemp, szPath) == 0;
    if (!fOkay) {
        RemoveNativeFile(szRaceLibraryTemp);
        RemoveNativePath(szTempDir);
        if (IsWindow(hwnd)) AlertIO().pfnAlertSz(TextIO().pfnFormat(idsStarsUnableSaveRaceDataFilePlease, NULL), MB_ICONHAND);
        return FALSE;
    }
    RemoveNativePath(szTempDir);
    CchSnprintf(szRaceLibraryPath, sizeof(szRaceLibraryPath), "%s", szPath);
    SetSzRaceFile(pszName);
    fNewLibraryRace = FALSE;
    return TRUE;
}

/* Compare the complete persisted design, including credentials, rather than
 * its collision-prone checksum or unused bytes after name terminators. */
static int16_t FSameLibraryRace(const PLAYER *pplrLeft, const PLAYER *pplrRight) {
    PLAYER plrLeft = *pplrLeft, plrRight = *pplrRight;
    std::byte rgbLeft[cbWirePlayer], rgbRight[cbWirePlayer];
    plrLeft.fLearned = plrRight.fLearned = FALSE;
    FEncodePlayer(plrLeft, rgbLeft); FEncodePlayer(plrRight, rgbRight);
    return !memcmp(rgbLeft, rgbRight, ibWirePlayerNames) &&
           !strcmp(plrLeft.szName, plrRight.szName) && !strcmp(plrLeft.szNames, plrRight.szNames);
}

static int16_t FReadManagedRace(char *pszPath, PLAYER *pplr) {
    NATIVEFILE *pf;
    HDR hdr;
    RTBOF bof;
    RACESTATE rsSav;
    uint16_t wVersSav = wVersFile;
    int16_t fSilentSav = fFileErrSilent, fOkay, fDirectory, fLink;

    if (!FNativePathInfo(pszPath, &fDirectory, &fLink) || fDirectory || fLink) return FALSE;
    pf = PfOpenNativeFile(pszPath, "rb");
    if (!pf) return FALSE;
    // Browsing versions must never trigger legacy conversion/password dialogs.
    fOkay = FReadLibraryHdr(pf, &hdr) && hdr.rt == rtBOF && hdr.cb == cbWireBof &&
            FReadLibraryBof(pf, &bof) && !memcmp(bof.rgid, "J3J3", 4) &&
            bof.verMajor == 2 && bof.verMinor == 85 && bof.dt == dtRace;
    ICloseNativeFile(pf);
    if (!fOkay) return FALSE;
    SaveRaceState(&rsSav);
    fFileErrSilent = TRUE;
    fOkay = FWasRaceFile(pszPath, FALSE) > 0;
    if (fOkay) *pplr = vplr;
    RestoreRaceState(&rsSav, TRUE, TRUE);
    fFileErrSilent = fSilentSav;
    wVersFile = wVersSav;
    return fOkay;
}

typedef struct {
    char szStem[240];
    char szLatest[256];
    char szSame[256];
    const PLAYER *pplr;
    int iMax, iLatest, iSame;
} RACEVERSIONS;

static void GetRaceLibraryStem(const char *pszName, char *pszStem) {
    size_t i, cch;
    char szTest[256];

    CchSnprintf(pszStem, 240, "%s", pszName);
    for (i = 0; pszStem[i]; i++)
        if ((unsigned char)pszStem[i] < 32 || pszStem[i] == 127 || strchr("<>:\"/\\|?*", pszStem[i])) pszStem[i] = '_';
    cch = strlen(pszStem);
    while (cch != 0 && (pszStem[cch - 1] == ' ' || pszStem[cch - 1] == '.')) pszStem[--cch] = 0;
    while (pszStem[0] == ' ' || pszStem[0] == '.') memmove(pszStem, pszStem + 1, strlen(pszStem));
    if (!pszStem[0]) CchSnprintf(pszStem, 240, "%s", TextIO().pfnCompressed(idsLibraryDefaultRaceName));
    CchSnprintf(szTest, sizeof(szTest), "%s.r1", pszStem);
    if (!FPortableRaceFilename(szTest)) {
        // Prefix reserved device names without altering the in-game race name.
        memmove(pszStem + 1, pszStem, strlen(pszStem) + 1);
        pszStem[0] = '_';
    }
}

static int16_t FindRaceVersion(void *lpData, const char *pszName) {
    RACEVERSIONS *prv = (RACEVERSIONS *)lpData;
    const char *pch = strrchr(pszName, '.');
    char szPath[512];
    PLAYER plr;
    int iVersion;
    size_t cch = strlen(prv->szStem);

    if (!pch || !FPortableRaceFilename(pszName)) return TRUE;
    iVersion = atoi(pch + 2);
    if ((size_t)(pch - pszName) == cch && !SDL_strncasecmp(pszName, prv->szStem, cch))
        prv->iMax = StarsMax(prv->iMax, iVersion);
    CchSnprintf(szPath, sizeof(szPath), "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    if (!FReadManagedRace(szPath, &plr)) return TRUE;
    // Versions follow the stored race name, including older/imported filenames.
    // Distinct names can sanitize to the same stem without sharing a design.
    if (SDL_strcasecmp(plr.szName, prv->pplr->szName)) return TRUE;
    prv->iMax = StarsMax(prv->iMax, iVersion);
    if (iVersion > prv->iLatest || (iVersion == prv->iLatest && SDL_strcasecmp(pszName, prv->szLatest) < 0)) {
        prv->iLatest = iVersion;
        strcpy(prv->szLatest, pszName);
    }
    if (iVersion > prv->iSame && FSameLibraryRace(prv->pplr, &plr)) {
        prv->iSame = iVersion;
        strcpy(prv->szSame, pszName);
    }
    return TRUE;
}

INT_PTR CALLBACK LibraryRaceSaveDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    char szMessage[512];
    POINT16 pt = {-1, -1};
    switch (message) {
    case WM_INITDIALOG:
        ControlIO().pfnFUiSetWindowText(hwnd, TextIO().pfnCompressed(idsLibraryRaceSaveTitle));
        CchSnprintf(szMessage, sizeof(szMessage), TextIO().pfnCompressed(idsLibraryRaceSavePrompt), (const char *)lParam);
        SetDlgItemText(hwnd, IDC_LIBRARY_RACE_SAVE_TEXT, szMessage);
        SetDlgItemText(hwnd, IDC_LIBRARY_RACE_UPDATE, TextIO().pfnCompressed(idsLibraryRaceUpdate));
        SetDlgItemText(hwnd, IDC_LIBRARY_RACE_VERSION, TextIO().pfnCompressed(idsLibraryRaceNewVersion));
        SetDlgItemText(hwnd, IDCANCEL, TextIO().pfnCompressed(idsCancel));
        ControlIO().pfnStickyDlgPos(hwnd, &pt, TRUE);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) { ControlIO().pfnEndDialog(hwnd, 0); return TRUE; }
        if (LOWORD(wParam) == IDC_LIBRARY_RACE_UPDATE) { ControlIO().pfnEndDialog(hwnd, 1); return TRUE; }
        if (LOWORD(wParam) == IDC_LIBRARY_RACE_VERSION) { ControlIO().pfnEndDialog(hwnd, 2); return TRUE; }
        break;
    }
    return FALSE;
}

int16_t FSaveCurrentLibraryRace(HWND hwnd, PLAYER *pplr) {
    RACEVERSIONS rv = {0};
    RACESTATE rsSav;
    PLAYER plr;
    char szName[256], szPath[512];
    const char *pszExisting;
    int iChoice;

    if (!FHasCurrentLibraryRace() || !FInitRaceLibrary()) return FALSE;
    rv.pplr = pplr;
    GetRaceLibraryStem(pplr->szName, rv.szStem);
    if (!FNativeEnumerateDirectory(szRaceLibraryRoot, FindRaceVersion, &rv)) return FALSE;
    pszExisting = !fNewLibraryRace && szRaceLibraryPath[0] ? PszCurrentLibraryRaceName() : rv.szLatest;
    if (pszExisting[0]) {
        CchSnprintf(szName, sizeof(szName), "%s", pszExisting);
        CchSnprintf(szPath, sizeof(szPath), "%s%s%s", szRaceLibraryRoot, szDirSep, szName);
        if (!FReadManagedRace(szPath, &plr)) goto LFail;
        if (FSameLibraryRace(pplr, &plr)) goto LReuse;
    }
    if (rv.szSame[0]) {
        strcpy(szName, rv.szSame);
        goto LReuse;
    }
    iChoice = pszExisting[0] ? (int)ControlIO().pfnLUiDialogBox(hInst, MAKEINTRESOURCE(IDD_LIBRARY_RACE_SAVE), hwnd,
                                                 LibraryRaceSaveDlg, (LPARAM)szName) : 2;
    if (iChoice != 1 && iChoice != 2) return FALSE;
    if (iChoice == 2) {
        if (rv.iMax >= 999) goto LFail;
        CchSnprintf(szName, sizeof(szName), "%s.r%d", rv.szStem, rv.iMax + 1);
        if (!FPortableRaceFilename(szName) || !FRaceLibraryNameAvailable(szName)) goto LFail;
    } else if (fNewLibraryRace && plr.lSalt != 0) {
        // Updating a protected entry also requires its existing credentials.
        SaveRaceState(&rsSav);
        iChoice = FLoadLibraryRace(hwnd, szName);
        RestoreRaceState(&rsSav, TRUE, TRUE);
        if (!iChoice) return FALSE;
    }
    return FSaveLibraryRace(hwnd, szName, pplr);
LReuse:
    CchSnprintf(szRaceLibraryPath, sizeof(szRaceLibraryPath), "%s%s%s", szRaceLibraryRoot, szDirSep, szName);
    SetSzRaceFile(szName);
    fNewLibraryRace = FALSE;
    return TRUE;
LFail:
    if (IsWindow(hwnd)) AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryRaceSaveFailed), MB_ICONHAND);
    return FALSE;
}

int16_t FHasCurrentLibraryRace(void) {
    return fNewLibraryRace || szRaceLibraryPath[0] != 0;
}

int16_t FLoadLibraryRace(HWND hwnd, const char *pszName) {
    char szPath[512];
    RACESTATE rsSav;
    int16_t fResult;
    if (!FInitRaceLibrary() || !FPortableRaceFilename(pszName)) return FALSE;
    CchSnprintf(szPath, sizeof(szPath), "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    SaveRaceState(&rsSav);
    fResult = FWasRaceFile(szPath, TRUE);
    if (fResult <= 0) {
        RestoreRaceState(&rsSav, TRUE, TRUE);
        if (IsWindow(hwnd)) AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryRaceOpenFailed), MB_ICONEXCLAMATION);
        return FALSE;
    }
    CchSnprintf(szRaceLibraryPath, sizeof(szRaceLibraryPath), "%s", szPath);
    SetSzRaceFile(pszName);
    fNewLibraryRace = FALSE;
    return TRUE;
}

const char *PszCurrentLibraryRaceName(void) {
    const char *pch = strrchr(szRaceLibraryPath, chDirSep);
    return pch ? pch + 1 : szRaceLibraryPath;
}

/* The new-game roster owns a filename for each design. Bind only this edit
 * to that destination; a previously imported design is not its save target. */
int16_t FCustomizeLibraryRace(HWND hwnd, char *pszName) {
    RACESTATE rsSav;
    char szPath[512];
    int16_t fResult;

    SaveRaceState(&rsSav);
    if (pszName[0]) {
        if (!FInitRaceLibrary() || !FPortableRaceFilename(pszName)) return FALSE;
        CchSnprintf(szPath, sizeof(szPath), "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
        CchSnprintf(szRaceLibraryPath, sizeof(szRaceLibraryPath), "%s", szPath);
        SetSzRaceFile(pszName);
        fNewLibraryRace = FALSE;
    } else {
        if (!FInitRaceLibrary()) return FALSE;
        szRaceLibraryPath[0] = 0;
        fNewLibraryRace = TRUE;
    }
    fResult = RaceCreationWizard(hwnd, FALSE, FALSE);
    if (fResult) CchSnprintf(pszName, 256, "%s", PszCurrentLibraryRaceName());
    RestoreRaceState(&rsSav, !fResult, TRUE);
    return fResult;
}

int16_t FBeginLibraryRace(HWND hwnd) {
    RACESTATE rsSav;
    int16_t fResult;
    SaveRaceState(&rsSav);
    if (!FInitRaceLibrary()) return FALSE;
    szRaceLibraryPath[0] = 0;
    fNewLibraryRace = TRUE;
    vplr = vrgplrDef[0];
    strcpy(vplr.szName, TextIO().pfnCompressed(idsHumanoid));
    strcpy(vplr.szNames, TextIO().pfnCompressed(idsLibraryHumanoids));
    szRacePass[0] = 0;
    SetSzRaceFile("");
    fResult = RaceCreationWizard(hwnd, FALSE, FALSE);
    if (!fResult) {
        RestoreRaceState(&rsSav, TRUE, TRUE);
    }
    else RestoreRaceState(&rsSav, FALSE, TRUE);
    return fResult;
}

int16_t FEditLibraryRace(HWND hwnd, const char *pszName) {
    RACESTATE rsSav;
    int16_t fResult;
    SaveRaceState(&rsSav);
    if (!FLoadLibraryRace(hwnd, pszName)) return FALSE;
    fResult = RaceCreationWizard(hwnd, FALSE, FALSE);
    RestoreRaceState(&rsSav, !fResult, TRUE);
    return fResult;
}

int16_t FDuplicateLibraryRace(HWND hwnd, const char *pszName) {
    char szSource[512], szName[256], szPath[512] = "";
    const char *pch;
    RACESTATE rsSav;
    if (!FInitRaceLibrary() || !FPortableRaceFilename(pszName)) return FALSE;
    CchSnprintf(szSource, sizeof(szSource), "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    SaveRaceState(&rsSav);
    if (FWasRaceFile(szSource, TRUE) <= 0) {
        RestoreRaceState(&rsSav, TRUE, TRUE);
        return FALSE;
    }
    CchSnprintf(szName, sizeof(szName), "%s", pszName);
    pch = strrchr(szName, '.');
    if (pch) szName[pch - szName] = 0;
    SuggestRaceFilename(szName);
    if (!szName[0] || CchSnprintf(szPath, sizeof(szPath), "%s%s%s", szRaceLibraryRoot, szDirSep, szName) <= 0) {
        RestoreRaceState(&rsSav, TRUE, TRUE);
        if (IsWindow(hwnd)) AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryRaceDuplicateFailed), MB_ICONEXCLAMATION);
        return FALSE;
    }
    if (!FCopyNativeFile(szSource, szPath)) {
        RestoreRaceState(&rsSav, TRUE, TRUE);
        if (IsWindow(hwnd)) AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryRaceDuplicateFailed), MB_ICONEXCLAMATION);
        return FALSE;
    }
    if (FWasRaceFile(szPath, TRUE) <= 0) {
        RemoveNativeFile(szPath);
        RestoreRaceState(&rsSav, TRUE, TRUE);
        if (IsWindow(hwnd)) AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryRaceDuplicateFailed), MB_ICONEXCLAMATION);
        return FALSE;
    }
    RestoreRaceState(&rsSav, TRUE, TRUE);
    return TRUE;
}

static void GetLibraryFileFilter(StringId ids, char *pszFilter) {
    char *pch;

    TextIO().pfnCchGetString(ids, pszFilter);
    for (pch = pszFilter; *pch; pch++) if (*pch == '|') *pch = 0;
}

int16_t FExportLibraryRace(HWND hwnd, const char *pszName, const char *pszTarget) {
    OPENFILENAME ofn;
    char szSource[512], szTarget[512], szTemp[512], szValidate[512], szFilter[128];
    RACESTATE rsSav;
    if (!FInitRaceLibrary() || !FPortableRaceFilename(pszName)) return FALSE;
    CchSnprintf(szSource, sizeof(szSource), "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    if (!pszTarget) {
        memset(&ofn, 0, sizeof(ofn));
        memset(szTarget, 0, sizeof(szTarget));
        CchSnprintf(szTarget, sizeof(szTarget), "%s", pszName);
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        GetLibraryFileFilter(idsLibraryRaceFilesFilter, szFilter);
        ofn.lpstrFilter = szFilter;
        ofn.lpstrFile = szTarget;
        ofn.nMaxFile = sizeof(szTarget);
        ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        if (!DesktopChoiceIO().pfnFNativeSaveFileName(&ofn)) return FALSE;
        pszTarget = szTarget;
    }
    SaveRaceState(&rsSav);
    if (FWasRaceFile(szSource, TRUE) <= 0 || SDL_strcasecmp(szSource, pszTarget) == 0) {
        RestoreRaceState(&rsSav, TRUE, TRUE);
        if (IsWindow(hwnd)) AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryRaceExportFailed), MB_ICONEXCLAMATION);
        return FALSE;
    }
    if (!FCopyRaceExport(szSource, pszTarget, szTemp, sizeof(szTemp))) {
        RestoreRaceState(&rsSav, TRUE, TRUE);
        if (IsWindow(hwnd)) AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryRaceExportFailed), MB_ICONEXCLAMATION);
        return FALSE;
    }
    CchSnprintf(szValidate, sizeof(szValidate), "%s", szTemp);
    if (FWasRaceFile(szValidate, TRUE) <= 0 || ReplaceNativeFile(szTemp, pszTarget) != 0) {
        RemoveNativeFile(szTemp);
        RestoreRaceState(&rsSav, TRUE, TRUE);
        if (IsWindow(hwnd)) AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryRaceExportFailed), MB_ICONEXCLAMATION);
        return FALSE;
    }
    RestoreRaceState(&rsSav, TRUE, TRUE);
    return TRUE;
}

int16_t FExportLibraryOrders(HWND hwnd, const char *pszTarget) {
    OPENFILENAME ofn;
    char szBaseSav[sizeof(szBase)], szBaseStem[sizeof(szBase)], szName[256], szSuggested[256], szTarget[512], szFilter[64];
    char szStageBase[sizeof(szBase)], szStage[sizeof(szBase)], szStageHist[sizeof(szBase)];
    char szSource[sizeof(szBase)], szSourceHist[sizeof(szBase)], szPreviousHist[sizeof(szBase)];
    char szTemp[512], szMessage[640], szExt[4];
    char *pch, *pchSlash;
    const char *pszLibraryName;
    StringId idsSaveError;
    NATIVEFILE *pf;
    uint32_t dwTick;
    int64_t cbTarget;
    int cch;
    int16_t fSubmitSav, fSaved, fDirtySav, fHistoryExists, fHistoryReplaced, fHistoryBackup;

    if (!FInitLibrary() || !lpPlanets || game.lid == 0 || idPlayer == iplrNone || idPlayer < 0 || idPlayer >= 16) return FALSE;
    if (!LibraryPathIO().pfnUnder(szLibraryRoot, szBase)) return FALSE;
    RememberLibraryGame();

    pszLibraryName = szLibraryExchangeName;
    if (!pszLibraryName[0]) {
        pch = PchLibrarySeparator(szBase);
        if (!pch) return FALSE;
        pchSlash = pch;
        while (pchSlash > szBase && pchSlash[-1] != chDirSep
#ifdef _WIN32
               && pchSlash[-1] != '/'
#endif
        ) pchSlash--;
        cch = CchSnprintf(szName, sizeof(szName), "%.*s", (int)(pch - pchSlash), pchSlash);
        if (cch <= 0 || (size_t)cch >= sizeof(szName)) return FALSE;
        pszLibraryName = szName;
    }
    cch = CchSnprintf(szSuggested, sizeof(szSuggested), "%s.x%d", pszLibraryName, idPlayer + 1);
    if (cch <= 0 || (size_t)cch >= sizeof(szSuggested)) return FALSE;
    if (!pszTarget) {
        memset(&ofn, 0, sizeof(ofn));
        memset(szTarget, 0, sizeof(szTarget));
        CchSnprintf(szTarget, sizeof(szTarget), "%s", szSuggested);
        cch = CchSnprintf(szFilter, sizeof(szFilter), TextIO().pfnCompressed(idsLibraryOrdersExportFilter),
                          idPlayer + 1, idPlayer + 1);
        if (cch <= 0 || (size_t)cch + 2 >= sizeof(szFilter)) return FALSE;
        for (pch = szFilter; *pch; pch++) if (*pch == '|') *pch = 0;
        szFilter[cch + 1] = 0;
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = szFilter;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = szTarget;
        ofn.nMaxFile = sizeof(szTarget);
        CchSnprintf(szExt, sizeof(szExt), "x%d", idPlayer + 1);
        ofn.lpstrDefExt = szExt;
        ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
        if (!DesktopChoiceIO().pfnFNativeSaveFileName(&ofn)) return FALSE;
        pszTarget = szTarget;
    }

    if (strlen(pszTarget) >= sizeof(szTarget)) return FALSE;
    if (pszTarget != szTarget) CchSnprintf(szTarget, sizeof(szTarget), "%s", pszTarget);
    pchSlash = strrchr(szTarget, '/');
    pch = strrchr(szTarget, '\\');
    if (pch && (!pchSlash || pch > pchSlash)) pchSlash = pch;
    pch = strrchr(szTarget, '.');
    if (pch && (!pchSlash || pch > pchSlash)) *pch = 0;
    if (!szTarget[0]) return FALSE;
    cch = CchSnprintf(szTarget + strlen(szTarget), sizeof(szTarget) - strlen(szTarget), ".x%d", idPlayer + 1);
    if (cch <= 0 || (size_t)cch >= sizeof(szTarget) - strlen(szTarget)) return FALSE;

    /* Confirm the actual orders path after correcting the chooser's extension. */
    if (FNativeFileSize(szTarget, &cbTarget)) {
        CchSnprintf(szMessage, sizeof(szMessage), TextIO().pfnCompressed(idsLibraryReplaceOrders), szTarget);
        if (CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage, TextIO().pfnCompressed(idsLibraryTitle), MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2) != IDYES) return FALSE;
    }

    SetSzWorkFromDt(dtLog, idPlayer);
    CchSnprintf(szSource, sizeof(szSource), "%s", szWork);
    if (SDL_strcasecmp(szSource, szTarget) == 0) return FALSE;
    CchSnprintf(szBaseSav, sizeof(szBaseSav), "%s", szBase);
    CchSnprintf(szBaseStem, sizeof(szBaseStem), "%s", szBaseSav);
    fSubmitSav = gd.fSubmit;
    fDirtySav = game.fDirty;
    fHistoryReplaced = FALSE;
    fHistoryBackup = FALSE;
    dwTick = DwTickCount();
    pch = strrchr(szBaseStem, '.');
    pchSlash = PchLibrarySeparator(szBaseStem);
    if (pch && (!pchSlash || pch > pchSlash)) *pch = 0;
    cch = CchSnprintf(szStageBase, sizeof(szStageBase), "%s_exportstage_%u.m%d", szBaseStem, dwTick, idPlayer + 1);
    if (cch <= 0 || (size_t)cch >= sizeof(szStageBase)) return FALSE;
    /* FWriteLogFile creates its destination directly, so save to a sibling first. */
    fSaved = FWriteLogFile(szStageBase, idPlayer);
    CchSnprintf(szBase, sizeof(szBase), "%s", szBaseSav);
    cch = CchSnprintf(szStage, sizeof(szStage), "%s_exportstage_%u.x%d", szBaseStem, dwTick, idPlayer + 1);
    if (cch <= 0 || (size_t)cch >= sizeof(szStage)) fSaved = FALSE;
    cch = CchSnprintf(szStageHist, sizeof(szStageHist), "%s_exportstage_%u.h%d", szBaseStem, dwTick, idPlayer + 1);
    if (cch <= 0 || (size_t)cch >= sizeof(szStageHist)) fSaved = FALSE;
    if (fSaved) {
        CchSnprintf(szBase, sizeof(szBase), "%s", szStageBase);
        fSaved = FWriteHistFile(idPlayer);
        CchSnprintf(szBase, sizeof(szBase), "%s", szBaseSav);
    }
    SetSzWorkFromDt(dtHist, idPlayer);
    CchSnprintf(szSourceHist, sizeof(szSourceHist), "%s", szWork);
    pf = PfOpenNativeFile(szSourceHist, "rb");
    fHistoryExists = pf != NULL;
    if (pf) ICloseNativeFile(pf);
    else if (errno != ENOENT) fSaved = FALSE;
    cch = CchSnprintf(szPreviousHist, sizeof(szPreviousHist), "%s_exportprevious_%u.h%d",
                      szBaseStem, dwTick, idPlayer + 1);
    if (cch <= 0 || (size_t)cch >= sizeof(szPreviousHist)) fSaved = FALSE;
    if (!fSaved) {
        idsSaveError = idsLibraryOrdersSaveFailed;
        goto LSaveFailed;
    }
    if (fHistoryExists && !FCopyNativeFileExclusive(szSourceHist, szPreviousHist)) {
        idsSaveError = idsLibraryHistoryPreserveFailed;
        goto LSaveFailed;
    }
    fHistoryBackup = fHistoryExists;
    fHistoryReplaced = ReplaceNativeFile(szStageHist, szSourceHist) == 0;
    if (!fHistoryReplaced || ReplaceNativeFile(szStage, szSource) != 0) {
        idsSaveError = idsLibraryOrdersSaveFailed;
        goto LSaveFailed;
    }
    fHistoryReplaced = FALSE;
    if (fHistoryBackup) RemoveNativeFile(szPreviousHist);
    CchSnprintf(szBase, sizeof(szBase), "%s", szBaseSav);
    /* Writing the orders BOF must not change the turn's submission state. */
    gd.fSubmit = fSubmitSav;

    CchSnprintf(szTemp, sizeof(szTemp), "%s.stars-export-%u", szTarget, dwTick);
    if (!FCopyNativeFileExclusive(szSource, szTemp) || ReplaceNativeFile(szTemp, szTarget) != 0) {
        RemoveNativeFile(szTemp);
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryOrdersExportFailed), MB_ICONEXCLAMATION);
        return FALSE;
    }
    return TRUE;
LSaveFailed:
    if (fHistoryReplaced) {
        if (fHistoryExists && ReplaceNativeFile(szPreviousHist, szSourceHist) != 0) {
            AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryHistoryRestoreFailed), MB_ICONHAND);
            fHistoryBackup = FALSE;
        } else if (!fHistoryExists) RemoveNativeFile(szSourceHist);
        else fHistoryBackup = FALSE;
    }
    RemoveNativeFile(szStage);
    RemoveNativeFile(szStageHist);
    if (fHistoryBackup) RemoveNativeFile(szPreviousHist);
    CchSnprintf(szBase, sizeof(szBase), "%s", szBaseSav);
    gd.fSubmit = fSubmitSav;
    game.fDirty = fDirtySav;
    AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsSaveError), MB_ICONHAND);
    return FALSE;
}

int16_t FDeleteLibraryRace(HWND hwnd, const char *pszName) {
    char szPath[512], szMessage[320];
    int64_t cbFile;
    if (!FInitRaceLibrary() || !FPortableRaceFilename(pszName)) return FALSE;
    CchSnprintf(szPath, sizeof(szPath), "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    if (!FNativeFileSize(szPath, &cbFile) || cbFile <= 0) return FALSE;
    CchSnprintf(szMessage, sizeof(szMessage), TextIO().pfnCompressed(idsLibraryRaceDeletePrompt), pszName);
    if (CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage, TextIO().pfnCompressed(idsLibraryTitle), MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) != IDYES) return FALSE;
    if (RemoveNativeFile(szPath) != 0) {
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryRaceDeleteFailed), MB_ICONHAND);
        return FALSE;
    }
    if (!strcmp(szRaceLibraryPath, szPath)) szRaceLibraryPath[0] = 0;
    return TRUE;
}

int16_t FImportLibraryRace(HWND hwnd, const char *pszSource) {
    OPENFILENAME ofn;
    char szSource[512], szName[256], szStem[256], szPath[512], szStage[512], szStageDir[512], szFilter[128];
    char *pch, *pchSlash, *pchExt;
    RACESTATE rsSav;
    int16_t fValid;
    int16_t fLegacy;
    if (!pszSource) {
        memset(&ofn, 0, sizeof(ofn));
        memset(szSource, 0, sizeof(szSource));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        GetLibraryFileFilter(idsLibraryRaceFilesFilter, szFilter);
        ofn.lpstrFilter = szFilter;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = szSource;
        ofn.nMaxFile = sizeof(szSource);
        ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        if (!DesktopChoiceIO().pfnFNativeOpenFileName(&ofn)) return FALSE;
        pszSource = szSource;
    }
    if (strlen(pszSource) >= sizeof(szSource)) return FALSE;
    if (pszSource != szSource) strcpy(szSource, pszSource);
    SaveRaceState(&rsSav);
    fValid = FWasRaceFile(szSource, TRUE);
    if (fValid <= 0) {
        RestoreRaceState(&rsSav, TRUE, TRUE);
        return FALSE;
    }
    fLegacy = VerMinor(wVersFile) < 85;
    pchSlash = strrchr(szSource, '/');
    pch = strrchr(szSource, '\\');
    if (pch && (!pchSlash || pch > pchSlash)) pchSlash = pch;
    pch = pchSlash ? pchSlash + 1 : szSource;
    CchSnprintf(szStem, sizeof(szStem), "%s", pch);
    pchExt = strrchr(szStem, '.');
    if (pchExt) *pchExt = 0;
    CchSnprintf(szName, sizeof(szName), "%s.r1", szStem);
    if (pszRaceLibraryNameOverride) {
        if (!FInitRaceLibrary() || !FPortableRaceFilename(pszRaceLibraryNameOverride) ||
            !FRaceLibraryNameAvailable(pszRaceLibraryNameOverride)) goto LRestore;
        CchSnprintf(szRaceLibraryName, sizeof(szRaceLibraryName), "%s", pszRaceLibraryNameOverride);
        CchSnprintf(szPath, sizeof(szPath), "%s%s%s", szRaceLibraryRoot, szDirSep, szRaceLibraryName);
    } else if (!FChooseLibraryRaceName(hwnd, szName, szPath, sizeof(szPath))) goto LRestore;
    /* Legacy reads convert names and credentials in memory; retain that state,
     * never the original CP1252 bytes or password hash. */
    if (fLegacy) {
        if (!FSaveLibraryRace(hwnd, szRaceLibraryName, &vplr)) goto LRestore;
        return TRUE;
    }
    if (!FCreateRaceStagePath(szStageDir, sizeof(szStageDir), szStage, sizeof(szStage))) goto LRestore;
    if (!FCopyNativeFile(szSource, szStage) || ReplaceNativeFile(szStage, szPath) != 0) {
        RemoveNativeFile(szStage);
        RemoveNativePath(szStageDir);
        goto LRestore;
    }
    RemoveNativePath(szStageDir);
    if (FWasRaceFile(szPath, TRUE) <= 0) {
        RemoveNativeFile(szPath);
        goto LRestore;
    }
    CchSnprintf(szRaceLibraryPath, sizeof(szRaceLibraryPath), "%s", szPath);
    SetSzRaceFile(strrchr(szPath, chDirSep) + 1);
    return TRUE;
LRestore:
    RestoreRaceState(&rsSav, TRUE, TRUE);
    return FALSE;
}

int16_t FImportLibraryRaceAs(HWND hwnd, const char *pszSource, const char *pszName) {
    int16_t fResult;
    pszRaceLibraryNameOverride = pszName;
    fResult = FImportLibraryRace(hwnd, pszSource);
    pszRaceLibraryNameOverride = NULL;
    return fResult;
}

int16_t FImportLibraryRaceAndEdit(HWND hwnd, const char *pszSource) {
    const char *pszName;
    if (!FImportLibraryRace(hwnd, pszSource)) return FALSE;
    pszName = strrchr(szRaceLibraryPath, chDirSep);
    if (pszName) pszName++;
    else pszName = szRaceLibraryPath;
    /* Import remains successful when the user cancels customization. */
    FEditLibraryRace(hwnd, pszName);
    return TRUE;
}

/* Inspect record framing and BOF metadata without calling FOpenFile, which
 * binds validation to the active game globals. */
int16_t FInspectImportFile(const char *pszFile, DtFileType dt, int iplr,
                                 int32_t *plidGame, uint16_t *pturn, uint16_t *pwGen, int16_t *pfMulti) {
    NATIVEFILE *pf;
    HDR hdr;
    RTBOF rtbof;
    int64_t cbFile;
    long ib;
    int16_t fComplete;

    pf = PfOpenNativeFile(pszFile, "rb");
    if (!pf) return FALSE;
    fComplete = FALSE;
    if (ISeekNativeFile(pf, 0, SEEK_END) != 0 || (cbFile = LNativePosition(pf)) < 22 || ISeekNativeFile(pf, 0, SEEK_SET) != 0) goto LDone;
    if (!FReadLibraryHdr(pf, &hdr) || hdr.rt != rtBOF || hdr.cb != cbWireBof ||
        !FReadLibraryBof(pf, &rtbof) || memcmp(rtbof.rgid, "J3J3", 4) ||
        rtbof.verMajor != 2 || rtbof.verMinor != 85 || rtbof.dt != dt ||
        rtbof.iPlayer != iplr || rtbof.lidGame == 0) goto LDone;
    if (dt == dtXY) {
        uint32_t cPlanets;
        if (cbFile < 88 || (cbFile - 88) % 4 != 0 || (cPlanets = (uint32_t)((cbFile - 88) / 4)) > 999 ||
            ISeekNativeFile(pf, 18, SEEK_SET) != 0 || !FReadLibraryHdr(pf, &hdr) || hdr.rt != rtGame || hdr.cb != 64 ||
            ISeekNativeFile(pf, -4, SEEK_END) != 0 || !FReadLibraryHdr(pf, &hdr) || hdr.rt != rtEOF || hdr.cb != 2) goto LDone;
        fComplete = TRUE;
        *plidGame = rtbof.lidGame;
        if (pturn) *pturn = rtbof.turn;
        if (pwGen) *pwGen = rtbof.wGen;
        *pfMulti = rtbof.fMulti;
        goto LDone;
    }
    while (FReadLibraryHdr(pf, &hdr)) {
        ib = LNativePosition(pf);
        if (ib < 0 || ib + hdr.cb > cbFile) break;
        if (hdr.rt == rtEOF) {
            /* Orders and history writers use an empty EOF; host and turn
             * files carry the two-byte trailer. Both must end at EOF. */
            if (hdr.cb != 2 && !(hdr.cb == 0 && (dt == dtLog || dt == dtHist))) break;
            if (ib + hdr.cb != cbFile) break;
            fComplete = TRUE;
            break;
        }
        if (ISeekNativeFile(pf, hdr.cb, SEEK_CUR) != 0) break;
    }
    if (fComplete) {
        *plidGame = rtbof.lidGame;
        if (pturn) *pturn = rtbof.turn;
        if (pwGen) *pwGen = rtbof.wGen;
        *pfMulti = rtbof.fMulti;
        if (rtbof.fMulti) {
            uint16_t turnFinal;
            if (cbFile < 4 || ISeekNativeFile(pf, -2, SEEK_END) != 0 || CReadNative(&turnFinal, 2, 1, pf) != 1) fComplete = FALSE;
            else if (pturn) *pturn = turnFinal;
        }
    }
LDone:
    ICloseNativeFile(pf);
    return fComplete;
}

static int16_t FImportFamilyMember(const char *pszName, const char *pszStem, int *piplr, DtFileType *pdt) {
    size_t cch = strlen(pszStem);
    const char *pszExt;
    int iplrFamily;
    if (SDL_strncasecmp(pszName, pszStem, cch) || pszName[cch] != '.') return FALSE;
    pszExt = pszName + cch + 1;
    if (!SDL_strcasecmp(pszExt, "xy")) { *piplr = -1; *pdt = dtXY; return TRUE; }
    if (!SDL_strcasecmp(pszExt, "hst")) { *piplr = -1; *pdt = dtHost; return TRUE; }
    if (pszExt[0] == 'm' || pszExt[0] == 'M' || pszExt[0] == 'h' || pszExt[0] == 'H' || pszExt[0] == 'x' || pszExt[0] == 'X') {
        if (pszExt[1] < '1' || pszExt[1] > '9' || (pszExt[2] && (pszExt[2] < '0' || pszExt[2] > '9')) || (pszExt[2] && pszExt[3])) return FALSE;
        iplrFamily = pszExt[1] - '0';
        if (pszExt[2]) iplrFamily = iplrFamily * 10 + pszExt[2] - '0';
        if (iplrFamily > 16) return FALSE;
        *piplr = iplrFamily - 1;
        *pdt = SDL_tolower(pszExt[0]) == 'm' ? dtTurn : SDL_tolower(pszExt[0]) == 'h' ? dtHist : dtLog;
        return TRUE;
    }
    return FALSE;
}

typedef struct {
    const char *pszSource;
    const char *pszTarget;
    const char *pszSourceName;
    const char *pszTargetName;
    int16_t fBad;
} LIBRARYGAMECOPY;

static int16_t FReadLibraryMetadata(const char *pszDirectory, char *pszName, size_t cchName,
                                    int *piplr, long long *plPlayed,
                                    char *pszExchangeName, size_t cchExchange) {
    char szFile[512], szLine[256];
    NATIVEFILE *pf;
    int iVersion, cchText, cch;
    int16_t fRead;
    cch = CchSnprintf(szFile, sizeof(szFile), "%s%s.library", pszDirectory, szDirSep);
    if (cch <= 0 || (size_t)cch >= sizeof(szFile)) return FALSE;
    pf = PfOpenNativeFile(szFile, "rb");
    if (!pf) return FALSE;
    fRead = CNativeScan(pf, "%d\n", &iVersion) == 1 && PszGetNativeLine(szLine, sizeof(szLine), pf);
    if (fRead) {
        szLine[strcspn(szLine, "\r\n")] = 0;
        cchText = CchSnprintf(pszName, cchName, "%s", szLine);
        fRead = cchText > 0 && (size_t)cchText < cchName &&
                CNativeScan(pf, "%d\n%lld\n", piplr, plPlayed) == 2;
    }
    if (fRead && iVersion == 2 && PszGetNativeLine(szLine, sizeof(szLine), pf)) {
        szLine[strcspn(szLine, "\r\n")] = 0;
        cchText = CchSnprintf(pszExchangeName, cchExchange, "%s", szLine);
        fRead = cchText > 0 && (size_t)cchText < cchExchange;
    } else if (fRead && iVersion == 1) {
        cchText = CchSnprintf(pszExchangeName, cchExchange, "%s", pszName);
        fRead = cchText > 0 && (size_t)cchText < cchExchange;
    } else fRead = FALSE;
    ICloseNativeFile(pf);
    return fRead && (iVersion == 1 || iVersion == 2) && *pszName &&
           *piplr >= -1 && *piplr < 16 && *plPlayed >= 0 && *pszExchangeName;
}

static int16_t FLibraryNameAvailableExcept(const char *pszName, const char *pszExcept) {
    LIBRARYNAMECHECK lnc;
    lnc.pszDirectory = szLibraryRoot;
    lnc.pszName = pszName;
    lnc.fFound = FALSE;
    if (!FNativeEnumerateDirectory(szLibraryRoot, FindLibraryName, &lnc)) return FALSE;
    if (lnc.fFound && pszExcept && !SDL_strcasecmp(pszName, pszExcept)) return TRUE;
    return !lnc.fFound;
}

static int16_t FCopyLibraryGameEntry(void *lpData, const char *pszName) {
    LIBRARYGAMECOPY *plgc = (LIBRARYGAMECOPY *)lpData;
    char szSource[512], szTarget[512];
    const char *pszTargetFile;
    LIBRARYGAMECOPY lgb;
    int iplr;
    int cch;
    int16_t fDirectory, fLink;
    DtFileType dt;
    if (!strcmp(pszName, ".") || !strcmp(pszName, "..")) return TRUE;
    cch = CchSnprintf(szSource, sizeof(szSource), "%s%s%s", plgc->pszSource, szDirSep, pszName);
    if (cch <= 0 || (size_t)cch >= sizeof(szSource)) { plgc->fBad = TRUE; return FALSE; }
    if (!FNativePathInfo(szSource, &fDirectory, &fLink) || fLink) {
        plgc->fBad = TRUE;
        return FALSE;
    }
    if (fDirectory) {
        /* Rename removes the original tree after publication, so every directory
         * must survive staging, including backup, backupN and sidecar folders. */
        cch = CchSnprintf(szTarget, sizeof(szTarget), "%s%s%s", plgc->pszTarget, szDirSep, pszName);
        if (cch <= 0 || (size_t)cch >= sizeof(szTarget)) { plgc->fBad = TRUE; return FALSE; }
        if (!FLibraryNameAvailable(plgc->pszTarget, pszName) ||
            CreateNativeDir(szTarget) != 0) { plgc->fBad = TRUE; return FALSE; }
        /* Backup files use the original basename family, with unrelated sidecars retained verbatim. */
        lgb.pszSource = szSource;
        lgb.pszTarget = szTarget;
        lgb.pszSourceName = plgc->pszSourceName;
        lgb.pszTargetName = plgc->pszTargetName;
        lgb.fBad = FALSE;
        if (!FNativeEnumerateDirectory(szSource, FCopyLibraryGameEntry, &lgb) || lgb.fBad) {
            plgc->fBad = TRUE;
            return FALSE;
        }
        return TRUE;
    }
    if (FImportFamilyMember(pszName, plgc->pszSourceName, &iplr, &dt)) {
        if (!FFormatLibraryFamilyPath(szTarget, sizeof(szTarget), plgc->pszTarget,
                                      plgc->pszTargetName, dt, iplr)) { plgc->fBad = TRUE; return FALSE; }
    } else if (SDL_strcasecmp(pszName, ".library") && SDL_strcasecmp(pszName, ".library.tmp")) {
        /* Optional external universe definitions and other sidecars keep their own names. */
        cch = CchSnprintf(szTarget, sizeof(szTarget), "%s%s%s", plgc->pszTarget, szDirSep, pszName);
        if (cch <= 0 || (size_t)cch >= sizeof(szTarget)) { plgc->fBad = TRUE; return FALSE; }
    } else return TRUE;
    /* Renamed family members and unchanged sidecars can map to the same name.
     * Reject portable case collisions and never truncate an earlier staged file. */
    pszTargetFile = strrchr(szTarget, chDirSep);
    if (!pszTargetFile || !FLibraryNameAvailable(plgc->pszTarget, pszTargetFile + 1) ||
        !FCopyNativeFileExclusive(szSource, szTarget)) { plgc->fBad = TRUE; return FALSE; }
    return TRUE;
}

static int16_t FBackupDirectoryName(const char *pszName) {
    size_t cch = strlen(pszName);
    size_t i;

    if (strncmp(pszName, "backup", 6)) return FALSE;
    if (cch == 6) return TRUE;
    if (cch == 7 || cch == 8) i = 6;
    else if (cch == 10 && pszName[6] == '.') i = 7;
    else return FALSE;
    for (; i < cch; i++) if (pszName[i] < '0' || pszName[i] > '9') return FALSE;
    return TRUE;
}

int16_t CopyImportEntry(void *lpData, const char *pszName) {
    LIBRARYIMPORT *pli = (LIBRARYIMPORT *)lpData;
    char szSource[512], szTarget[512];
    int iplr;
    DtFileType dt;
    int32_t lidGame;
    uint16_t turn;
    int16_t fMulti;
    uint16_t wGen;

    if (!FImportFamilyMember(pszName, pli->pszStem, &iplr, &dt)) {
        size_t cch = strlen(pli->pszStem);
        if (!SDL_strncasecmp(pszName, pli->pszStem, cch) && pszName[cch] == '.' &&
            (SDL_tolower(pszName[cch + 1]) == 'm' || SDL_tolower(pszName[cch + 1]) == 'h' || SDL_tolower(pszName[cch + 1]) == 'x')) pli->fBad = TRUE;
        if (FBackupDirectoryName(pszName)) pli->fBackup = TRUE;
        return TRUE;
    }
    CchSnprintf(szSource, sizeof(szSource), "%s%s%s", pli->pszSource, szDirSep, pszName);
    if (!FFormatLibraryFamilyPath(szTarget, sizeof(szTarget), pli->pszTarget, pli->pszName, dt, iplr)) {
        pli->fBad = TRUE;
        return FALSE;
    }
    if (pli->fCopy) {
        if (!FCopyNativeFile(szSource, szTarget)) { pli->fBad = TRUE; return FALSE; }
        return TRUE;
    }
    if (!FInspectImportFile(szSource, dt, iplr < 0 ? iplrNone : iplr, &lidGame, &turn, &wGen, &fMulti)) {
        pli->fBad = TRUE;
        return TRUE;
    }
    if (dt == dtXY) {
        if (pli->fSawXY) { pli->fBad = TRUE; return TRUE; }
        pli->fSawXY = TRUE;
        if (pli->lidGame && pli->lidGame != lidGame) { pli->fBad = TRUE; return TRUE; }
        pli->lidGame = lidGame;
        pli->fValid = TRUE;
    } else if (dt == dtTurn) {
        if ((pli->lidGame && lidGame != pli->lidGame) || iplr >= 16 || (pli->grfPlayers & (1U << iplr))) { pli->fBad = TRUE; return TRUE; }
        if (!pli->lidGame) pli->lidGame = lidGame;
        pli->grfPlayers |= 1U << iplr;
        pli->rgturn[iplr] = (int16_t)turn;
    } else if (dt == dtHost || dt == dtHist || dt == dtLog) {
        if (pli->lidGame && lidGame != pli->lidGame) { pli->fBad = TRUE; return TRUE; }
        if (!pli->lidGame) pli->lidGame = lidGame;
        if (dt == dtHost) pli->fSawHost = TRUE;
    }
    /* The first pass validates every recognized file; copying happens only
     * after the entire family has passed validation. */
    pli->fCopied++;
    return TRUE;
}

typedef struct {
    const char *pszSource;
    const char *pszTarget;
    const char *pszStem;
    const char *pszName;
    const char *pszBackup;
    int16_t fBad;
} LIBRARYBACKUP;

static int16_t CopyBackupEntry(void *lpData, const char *pszName) {
    LIBRARYBACKUP *plb = (LIBRARYBACKUP *)lpData;
    int iplr;
    DtFileType dt;
    char szSource[512], szTarget[512], szDirectory[512];
    if (!FImportFamilyMember(pszName, plb->pszStem, &iplr, &dt)) return TRUE;
    CchSnprintf(szSource, sizeof(szSource), "%s%s%s%s%s", plb->pszSource, szDirSep, plb->pszBackup, szDirSep, pszName);
    CchSnprintf(szDirectory, sizeof(szDirectory), "%s%s%s", plb->pszTarget, szDirSep, plb->pszBackup);
    if (!FFormatLibraryFamilyPath(szTarget, sizeof(szTarget), szDirectory, plb->pszName, dt, iplr)) {
        plb->fBad = TRUE;
        return FALSE;
    }
    if (!FCopyNativeFile(szSource, szTarget)) { plb->fBad = TRUE; return FALSE; }
    return TRUE;
}

int16_t CopyBackupDirectory(void *lpData, const char *pszName) {
    LIBRARYIMPORT *pli = (LIBRARYIMPORT *)lpData;
    LIBRARYBACKUP lb;
    char szSource[512], szTarget[512];
    if (!FBackupDirectoryName(pszName)) return TRUE;
    CchSnprintf(szSource, sizeof(szSource), "%s%s%s", pli->pszSource, szDirSep, pszName);
    if (!FNativeDirectoryExists(szSource)) return TRUE;
    CchSnprintf(szTarget, sizeof(szTarget), "%s%s%s", pli->pszTarget, szDirSep, pszName);
    if (CreateNativeDir(szTarget) != 0) { pli->fBad = TRUE; return FALSE; }
    lb.pszSource = pli->pszSource;
    lb.pszTarget = pli->pszTarget;
    lb.pszStem = pli->pszStem;
    lb.pszName = pli->pszName;
    lb.pszBackup = pszName;
    lb.fBad = FALSE;
    if (!FNativeEnumerateDirectory(szSource, CopyBackupEntry, &lb) || lb.fBad) { pli->fBad = TRUE; return FALSE; }
    return TRUE;
}

static int16_t RemoveImportChild(void *lpData, const char *pszFile) {
    char szFile[512];
    CchSnprintf(szFile, sizeof(szFile), "%s%s%s", (char *)lpData, szDirSep, pszFile);
    RemoveNativePath(szFile);
    return TRUE;
}

int16_t RemoveImportFile(void *lpData, const char *pszFile) {
    char szFile[512];
    const char *pszRoot = (const char *)lpData;
    CchSnprintf(szFile, sizeof(szFile), "%s%s%s", pszRoot, szDirSep, pszFile);
    if (FNativeDirectoryExists(szFile)) {
        FNativeEnumerateDirectory(szFile, RemoveImportChild, szFile);
        RemoveNativePath(szFile);
    } else RemoveNativePath(szFile);
    return TRUE;
}

static int16_t RemoveImportTreeEntry(void *lpData, const char *pszFile) {
    char szPath[512];
    int16_t fDirectory, fLink;
    if (CchSnprintf(szPath, sizeof(szPath), "%s%s%s", (const char *)lpData, szDirSep, pszFile) <= 0 ||
        !FNativePathInfo(szPath, &fDirectory, &fLink)) return FALSE;
    if (fDirectory && !fLink) {
        if (!FNativeEnumerateDirectory(szPath, RemoveImportTreeEntry, szPath)) return FALSE;
    }
    return RemoveNativePath(szPath) == 0;
}

static int16_t FPathIsZip(const char *pszPath) {
    const char *pch = strrchr(pszPath, '.');
    return pch && !SDL_strcasecmp(pch, ".zip");
}

static int16_t FCreateLibraryZipStage(char *pszStage, size_t cchStage, const char *pszPurpose) {
    int i, cch;
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(pszStage, cchStage, "%s%s.%s.%u-%u", szLibraryRoot, szDirSep,
                          pszPurpose, DwTickCount(), idLibraryZipStage++);
        if (cch <= 0 || (size_t)cch >= cchStage) return FALSE;
        if (CreateNativeDir(pszStage) == 0) return TRUE;
        if (!FNativeDirectoryExists(pszStage)) return FALSE;
    }
    return FALSE;
}

typedef struct {
    const char *pszSource;
    const char *pszTarget;
    const char *pszStem;
    int16_t fBad;
    int16_t fSawXY;
    int16_t fSawPlayable;
} LIBRARYEXPORT;

static int16_t CopyGameExportEntry(void *lpData, const char *pszName) {
    LIBRARYEXPORT *ple = (LIBRARYEXPORT *)lpData;
    int iplr;
    DtFileType dt;
    char szSource[512], szTarget[512];
    size_t cchStem = strlen(ple->pszStem);

    if (!FImportFamilyMember(pszName, ple->pszStem, &iplr, &dt)) {
        /* A malformed companion should not silently make an incomplete export. */
        if (!SDL_strncasecmp(pszName, ple->pszStem, cchStem) && pszName[cchStem] == '.' &&
            strchr("mMhHxX", pszName[cchStem + 1])) ple->fBad = TRUE;
        return TRUE;
    }
    CchSnprintf(szSource, sizeof(szSource), "%s%s%s", ple->pszSource, szDirSep, pszName);
    CchSnprintf(szTarget, sizeof(szTarget), "%s%s%s", ple->pszTarget, szDirSep, pszName);
    if (!FCopyNativeFile(szSource, szTarget)) { ple->fBad = TRUE; return FALSE; }
    if (dt == dtXY) ple->fSawXY = TRUE;
    if (dt == dtTurn || dt == dtHost) ple->fSawPlayable = TRUE;
    return TRUE;
}

static int16_t CreateLibraryExportStage(char *pszDirectory, size_t cchDirectory,
                                        const char *pszParent) {
    uint32_t dwTick = DwTickCount();
    int i, cch;
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(pszDirectory, cchDirectory, "%s%s.stars-game-export-%u-%u",
                          pszParent, szDirSep, dwTick, idLibraryExportStage++);
        if (cch <= 0 || (size_t)cch >= cchDirectory) return FALSE;
        if (CreateNativeDir(pszDirectory) == 0) return TRUE;
        if (!FNativeDirectoryExists(pszDirectory)) return FALSE;
    }
    return FALSE;
}

typedef struct {
    const char *pszSource;
    const char *pszTarget;
    int16_t fBad;
} LIBRARYEXPORTMOVE;

static int16_t MoveGameExportEntry(void *lpData, const char *pszName) {
    LIBRARYEXPORTMOVE *plem = (LIBRARYEXPORTMOVE *)lpData;
    char szSource[512], szTarget[512];
    CchSnprintf(szSource, sizeof(szSource), "%s%s%s", plem->pszSource, szDirSep, pszName);
    CchSnprintf(szTarget, sizeof(szTarget), "%s%s%s", plem->pszTarget, szDirSep, pszName);
    if (RenameNativeFile(szSource, szTarget) != 0) { plem->fBad = TRUE; return FALSE; }
    return TRUE;
}

int16_t FExportLibraryGame(HWND hwnd, const char *pszName, const char *pszTarget) {
    LIBRARYIMPORT li;
    LIBRARYEXPORT le;
    LIBRARYEXPORTMOVE lem;
    OPENFILENAME ofn;
    char szSource[512], szDestination[512], szStage[512];
    char szZipName[512], szZipStage[512], szZipGame[512], szFilter[128], szMessage[256];
    int cch, iChoice;
    int16_t fZipStage;

    if (!pszTarget) {
        if (!getenv("STARS_CLIENT_EXPORT_GAME")) {
            TextIO().pfnCchGetString(idsLibraryExportZipPrompt, szMessage);
            iChoice = CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage,
                                 TextIO().pfnCompressed(idsLibraryExportGameTitle), MB_YESNOCANCEL | MB_ICONQUESTION | MB_TASKMODAL);
            if (iChoice == IDCANCEL) return FALSE;
            if (iChoice == IDYES) {
                memset(&ofn, 0, sizeof(ofn));
                memset(szZipName, 0, sizeof(szZipName));
                CchSnprintf(szZipName, sizeof(szZipName), "%s.zip", pszName);
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                GetLibraryFileFilter(idsLibraryZipFilesFilter, szFilter);
                ofn.lpstrFilter = szFilter;
                ofn.lpstrFile = szZipName;
                ofn.lpstrDefExt = "zip";
                ofn.nMaxFile = sizeof(szZipName);
                ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
                if (!DesktopChoiceIO().pfnFNativeSaveFileName(&ofn)) return FALSE;
                return FExportSelectedLibraryGame(hwnd, szZipName);
            }
        }
        CompatFolderIO().pfnShowOpenFolderDialog(LibraryExportFolderSelected, PvLibraryFolderContext(hwnd), NULL, NULL, FALSE);
        return TRUE;
    }
    if (!FInitLibrary() || !FPortableLibraryName(pszName)) return FALSE;
    cch = CchSnprintf(szSource, sizeof(szSource), "%s%s%s", szLibraryRoot, szDirSep, pszName);
    if (cch <= 0 || (size_t)cch >= sizeof(szSource) || !FNativeDirectoryExists(szSource)) return FALSE;
    if (FPathIsZip(pszTarget)) {
        if (SDL_strncasecmp(pszTarget, szSource, strlen(szSource)) == 0 &&
            (pszTarget[strlen(szSource)] == 0 || pszTarget[strlen(szSource)] == '/' ||
             pszTarget[strlen(szSource)] == '\\')) goto LFail;
        fZipStage = FCreateLibraryZipStage(szZipStage, sizeof(szZipStage), "zip-exporting");
        if (!fZipStage) goto LFail;
        if (!FExportLibraryGame(hwnd, pszName, szZipStage)) goto LZipFail;
        cch = CchSnprintf(szZipGame, sizeof(szZipGame), "%s%s%s", szZipStage, szDirSep, pszName);
        if (cch <= 0 || (size_t)cch >= sizeof(szZipGame) || !FZipLibraryDirectory(szZipGame, pszTarget)) goto LZipFail;
        FNativeEnumerateDirectory(szZipGame, RemoveImportTreeEntry, szZipGame);
        RemoveNativePath(szZipGame);
        RemoveNativePath(szZipStage);
        return TRUE;
LZipFail:
        FNativeEnumerateDirectory(szZipStage, RemoveImportTreeEntry, szZipStage);
        RemoveNativePath(szZipStage);
        goto LFail;
    }
    if (SDL_strncasecmp(pszTarget, szSource, strlen(szSource)) == 0 &&
        (pszTarget[strlen(szSource)] == 0 || pszTarget[strlen(szSource)] == '/' ||
         pszTarget[strlen(szSource)] == '\\')) goto LFail;
    cch = CchSnprintf(szDestination, sizeof(szDestination), "%s%s%s", pszTarget, szDirSep, pszName);
    if (cch <= 0 || (size_t)cch >= sizeof(szDestination) || FNativeDirectoryExists(szDestination)) goto LFail;
    /* The loader and backup writer keep complete paths in szBase. Check the
     * longest companion before copying, rather than truncating child paths. */
    if (strlen(szDestination) + strlen(pszName) + sizeof("/backup.999/.m16") > sizeof(szBase)) goto LFail;
    if (!CreateLibraryExportStage(szStage, sizeof(szStage), pszTarget)) goto LFail;
    if (strlen(szStage) + strlen(pszName) + sizeof("/backup.999/.m16") > sizeof(szStage)) goto LStageFail;

    memset(&le, 0, sizeof(le));
    le.pszSource = szSource;
    le.pszTarget = szStage;
    le.pszStem = pszName;
    if (!FNativeEnumerateDirectory(szSource, CopyGameExportEntry, &le) || le.fBad ||
        !le.fSawXY || !le.fSawPlayable) goto LStageFail;

    memset(&li, 0, sizeof(li));
    li.pszSource = szSource;
    li.pszTarget = szStage;
    li.pszStem = pszName;
    li.pszName = pszName;
    if (!FNativeEnumerateDirectory(szSource, CopyBackupDirectory, &li) || li.fBad ||
        CreateNativeDir(szDestination) != 0) goto LStageFail;
    lem.pszSource = szStage;
    lem.pszTarget = szDestination;
    lem.fBad = FALSE;
    if (!FNativeEnumerateDirectory(szStage, MoveGameExportEntry, &lem) || lem.fBad) {
        FNativeEnumerateDirectory(szDestination, RemoveImportFile, szDestination);
        RemoveNativePath(szDestination);
        goto LStageFail;
    }
    RemoveNativePath(szStage);
    return TRUE;

LStageFail:
    FNativeEnumerateDirectory(szStage, RemoveImportFile, szStage);
    RemoveNativePath(szStage);
LFail:
    if (IsWindow(hwnd)) AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryGameExportFailed), MB_ICONHAND);
    return FALSE;
}

static void SDLCALL LibraryExportFolderSelected(void *lpData, const char * const *rgpszPath, int iFilter) noexcept {
    (void)iFilter;
    try {
        if (!rgpszPath || !rgpszPath[0]) return;
        STARSCOMPATPAYLOAD path(SDL_strdup(rgpszPath[0]), {[](void *pv) noexcept { SDL_free(pv); }});
        StarsCompatPostOwnedMessageAsync(reinterpret_cast<uintptr_t>(lpData), WM_STARS_EXPORT_FOLDER, std::move(path), 0);
    } catch (...) {
        // The foreign dialog boundary cannot propagate a C++ failure.
    }
}

int16_t FExportSelectedLibraryGame(HWND hwnd, const char *pszTarget) {
    char szDir[512];
    char szName[256];
    if (!FSelectedLibraryGameName(hwnd, szName, sizeof(szName))) return FALSE;
    if (!pszTarget) return FExportLibraryGame(hwnd, szName, NULL);
    if (game.fDirty && lpPlanets && idPlayer != iplrNone) {
        CchSnprintf(szDir, sizeof(szDir), "%s%s%s", szLibraryRoot, szDirSep, szName);
        if (LibraryPathIO().pfnUnder(szDir, szBase)) PromptSaveGame();
    }
    return FExportLibraryGame(hwnd, szName, pszTarget);
}

void ShowLibraryExportFolderPicker(HWND hwnd) {
    CompatFolderIO().pfnShowOpenFolderDialog(LibraryExportFolderSelected, PvLibraryFolderContext(hwnd), NULL, NULL, FALSE);
}

static int16_t FindImportUniverse(void *lpData, const char *pszName) {
    char *pszStem = (char *)lpData;
    const char *pch = strrchr(pszName, '.');
    if (!pch || SDL_strcasecmp(pch, ".xy")) return TRUE;
    if (pszStem[0]) { pszStem[0] = '!'; return TRUE; }
    if ((size_t)(pch - pszName) >= 256) { pszStem[0] = '!'; return TRUE; }
    memcpy(pszStem, pszName, pch - pszName);
    pszStem[pch - pszName] = 0;
    return TRUE;
}

typedef struct {
    int32_t lidGame;
    int cMatches;
    char rgszName[64][256];
    char szSelected[256];
} LIBRARYTURNMATCH;

static int16_t FindLibraryTurnMatch(void *lpData, const char *pszName) {
    LIBRARYTURNMATCH *plm = (LIBRARYTURNMATCH *)lpData;
    char szUniverse[512], szMetadata[512];
    NATIVEFILE *pf;
    int32_t lidGame;
    uint16_t turn;
    int16_t fMulti;
    if (!FPortableLibraryName(pszName)) return TRUE;
    CchSnprintf(szMetadata, sizeof(szMetadata), "%s%s%s%s.library", szLibraryRoot, szDirSep, pszName, szDirSep);
    pf = PfOpenNativeFile(szMetadata, "rb");
    if (!pf) return TRUE;
    ICloseNativeFile(pf);
    CchSnprintf(szUniverse, sizeof(szUniverse), "%s%s%s%s%s.xy", szLibraryRoot, szDirSep, pszName, szDirSep, pszName);
    if (!FInspectImportFile(szUniverse, dtXY, iplrNone, &lidGame, &turn, NULL, &fMulti) || lidGame != plm->lidGame) return TRUE;
    if (plm->cMatches < (int)(sizeof(plm->rgszName) / sizeof(plm->rgszName[0])))
        CchSnprintf(plm->rgszName[plm->cMatches], sizeof(plm->rgszName[0]), "%s", pszName);
    plm->cMatches++;
    return TRUE;
}

INT_PTR CALLBACK LibraryTurnTargetDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LIBRARYTURNMATCH *plm;
    HWND hwndList;
    int i;
    switch (message) {
    case WM_INITDIALOG:
        SetWindowLongPtr(hwnd, DWLP_USER, lParam);
        plm = (LIBRARYTURNMATCH *)lParam;
        hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_TURN_TARGET_LIST);
        for (i = 0; i < plm->cMatches; i++) ControlIO().pfnLUiSendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)plm->rgszName[i]);
        ControlIO().pfnFUiEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDOK), FALSE);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) { ControlIO().pfnEndDialog(hwnd, FALSE); return TRUE; }
        if (LOWORD(wParam) == IDC_LIBRARY_TURN_TARGET_LIST && HIWORD(wParam) == LBN_SELCHANGE) {
            hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_TURN_TARGET_LIST);
            ControlIO().pfnFUiEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDOK), ControlIO().pfnLUiSendMessage(hwndList, LB_GETCURSEL, 0, 0) != LB_ERR);
            return TRUE;
        }
        if (LOWORD(wParam) == IDOK) {
            plm = (LIBRARYTURNMATCH *)GetWindowLongPtr(hwnd, DWLP_USER);
            hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_TURN_TARGET_LIST);
            i = (int)ControlIO().pfnLUiSendMessage(hwndList, LB_GETCURSEL, 0, 0);
            if (i == LB_ERR) return TRUE;
            ControlIO().pfnLUiSendMessage(hwndList, LB_GETTEXT, i, (LPARAM)plm->szSelected);
            ControlIO().pfnEndDialog(hwnd, TRUE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

INT_PTR CALLBACK LibraryTurnConflictDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (message) {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) { ControlIO().pfnEndDialog(hwnd, 0); return TRUE; }
        if (LOWORD(wParam) == IDC_LIBRARY_TURN_REPLACE) { ControlIO().pfnEndDialog(hwnd, 1); return TRUE; }
        if (LOWORD(wParam) == IDC_LIBRARY_TURN_SEPARATE) { ControlIO().pfnEndDialog(hwnd, 2); return TRUE; }
        break;
    }
    return FALSE;
}

static int16_t FFilesEqual(const char *pszLeft, const char *pszRight) {
    NATIVEFILE *pfLeft, *pfRight;
    char rgbLeft[4096], rgbRight[4096];
    size_t cbLeft, cbRight;
    int16_t fEqual = TRUE;
    pfLeft = PfOpenNativeFile(pszLeft, "rb");
    pfRight = PfOpenNativeFile(pszRight, "rb");
    if (!pfLeft || !pfRight) { fEqual = FALSE; goto LDone; }
    do {
        cbLeft = CReadNative(rgbLeft, 1, sizeof(rgbLeft), pfLeft);
        cbRight = CReadNative(rgbRight, 1, sizeof(rgbRight), pfRight);
        if (cbLeft != cbRight || memcmp(rgbLeft, rgbRight, cbLeft)) { fEqual = FALSE; break; }
    } while (cbLeft != 0);
    if (FNativeStreamError(pfLeft) || FNativeStreamError(pfRight)) fEqual = FALSE;
LDone:
    if (pfLeft) ICloseNativeFile(pfLeft);
    if (pfRight) ICloseNativeFile(pfRight);
    return fEqual;
}

int16_t FReplaceConflictingLibraryTurn(const LIBRARYPATHS &paths, HWND hwnd, const char *pszName,
                                             const char *pszSource, const char *pszTarget,
                                             int iplr, int32_t lidGame, uint16_t turn);
int16_t FImportConflictingTurnAsCopy(const LIBRARYPATHS &paths, HWND hwnd, const char *pszName,
                                            const char *pszSource, int iplr,
                                            int32_t lidGame, uint16_t turn);

static int16_t FInspectImportPlayer(const char *pszUniverse, int iplr);

static int16_t FImportSubsequentTurn(HWND hwnd, const char *pszSource) {
    char szUniverse[512], szTarget[512];
    const char *pszFile, *pszExt, *pchSlash;
    LIBRARYTURNMATCH lm;
    int32_t lidGame, lidCurrent;
    uint16_t turn, turnCurrent;
    int16_t fMulti, fMultiCurrent;
    INT_PTR iChoice;
    int iplr;
    int16_t fHasTurn, fDirectory, fLink;
    NATIVEFILE *pf;
    int cch;

    pszFile = strrchr(pszSource, '/');
    pchSlash = strrchr(pszSource, '\\');
    if (pchSlash && (!pszFile || pchSlash > pszFile)) pszFile = pchSlash;
    pszFile = pszFile ? pszFile + 1 : pszSource;
    pszExt = strrchr(pszFile, '.');
    if (!pszExt || (SDL_tolower(pszExt[1]) != 'm' && SDL_tolower(pszExt[1]) != 'M') ||
        pszExt[2] < '1' || pszExt[2] > '9' || (pszExt[3] && (pszExt[3] < '0' || pszExt[3] > '9')) ||
        (pszExt[3] && pszExt[4])) return FALSE;
    iplr = pszExt[2] - '0';
    if (pszExt[3]) iplr = iplr * 10 + pszExt[3] - '0';
    iplr--;
    if (iplr < 0 || iplr >= 16) return FALSE;
    if (!FInspectImportFile(pszSource, dtTurn, iplr, &lidGame, &turn, NULL, &fMulti)) {
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryTurnInvalid), MB_ICONEXCLAMATION);
        return FALSE;
    }
    memset(&lm, 0, sizeof(lm));
    lm.lidGame = lidGame;
    if (!FNativeEnumerateDirectory(szLibraryRoot, FindLibraryTurnMatch, &lm)) return FALSE;
    if (lm.cMatches == 0) return -1;
    if (lm.cMatches > 1) {
        if (lm.cMatches > (int)(sizeof(lm.rgszName) / sizeof(lm.rgszName[0])) ||
            ControlIO().pfnLUiDialogBox(hInst, MAKEINTRESOURCE(IDD_LIBRARY_TURN_TARGET), hwnd,
                           LibraryTurnTargetDlg, (LPARAM)&lm) != TRUE) return FALSE;
    } else CchSnprintf(lm.szSelected, sizeof(lm.szSelected), "%s", lm.rgszName[0]);
    if (!lm.szSelected[0]) {
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryTurnChooseGame), MB_ICONEXCLAMATION);
        return FALSE;
    }
    cch = CchSnprintf(szTarget, sizeof(szTarget), "%s%s%s%s%s.m%d", szLibraryRoot, szDirSep, lm.szSelected, szDirSep, lm.szSelected, iplr + 1);
    if (cch <= 0 || (size_t)cch >= sizeof(szTarget)) return FALSE;
    pf = PfOpenNativeFile(szTarget, "rb");
    fHasTurn = pf != NULL;
    if (pf) ICloseNativeFile(pf);
    else if (errno != ENOENT) {
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryTurnInspectFailed), MB_ICONEXCLAMATION);
        return FALSE;
    }
    if (fHasTurn) {
        if (!FInspectImportFile(szTarget, dtTurn, iplr, &lidCurrent, &turnCurrent, NULL, &fMultiCurrent) || lidCurrent != lidGame) {
            AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryTurnCurrentInvalid), MB_ICONEXCLAMATION);
            return FALSE;
        }
        if (FFilesEqual(pszSource, szTarget)) return TRUE;
    } else {
        /* A first turn uses the stored universe, but must name an existing
         * player and must not replace a dangling link or unreadable entry. */
        cch = CchSnprintf(szUniverse, sizeof(szUniverse), "%s%s%s%s%s.xy",
                          szLibraryRoot, szDirSep, lm.szSelected, szDirSep, lm.szSelected);
        if (cch <= 0 || (size_t)cch >= sizeof(szUniverse)) return FALSE;
        if (FNativePathInfo(szTarget, &fDirectory, &fLink) || !FInspectImportPlayer(szUniverse, iplr)) {
            AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryTurnPlayerInvalid), MB_ICONEXCLAMATION);
            return FALSE;
        }
    }
    if (fHasTurn && turn <= turnCurrent) {
        iChoice = ControlIO().pfnLUiDialogBox(hInst, MAKEINTRESOURCE(IDD_LIBRARY_TURN_CONFLICT), hwnd,
                                 LibraryTurnConflictDlg, 0);
        if (iChoice == 1) return FReplaceConflictingLibraryTurn({szLibraryRoot,szLibraryPending,szLibraryName}, hwnd, lm.szSelected, pszSource,
                                                               szTarget, iplr, lidGame, turn);
        if (iChoice == 2) return FImportConflictingTurnAsCopy({szLibraryRoot,szLibraryPending,szLibraryName}, hwnd, lm.szSelected, pszSource,
                                                             iplr, lidGame, turn);
        return FALSE;
    }
    return FPublishSubsequentTurn({szLibraryRoot,szLibraryPending,szLibraryName}, hwnd,
                                  pszSource,szTarget,lm.szSelected,iplr,lidGame,turn,fHasTurn);
}

static void SDLCALL LibraryImportFolderSelected(void *lpData, const char * const *rgpszPath, int iFilter) noexcept {
    (void)iFilter;
    try {
        if (!rgpszPath || !rgpszPath[0]) return;
        STARSCOMPATPAYLOAD path(SDL_strdup(rgpszPath[0]), {[](void *pv) noexcept { SDL_free(pv); }});
        StarsCompatPostOwnedMessageAsync(reinterpret_cast<uintptr_t>(lpData), WM_STARS_IMPORT_FOLDER, std::move(path), 0);
    } catch (...) {
        // The foreign dialog boundary cannot propagate a C++ failure.
    }
}

void ShowLibraryImportFolderPicker(HWND hwnd) {
    CompatFolderIO().pfnShowOpenFolderDialog(LibraryImportFolderSelected, PvLibraryFolderContext(hwnd), NULL, NULL, FALSE);
}

static int16_t FChooseLibraryImportFile(HWND hwnd, char *pszChosen, size_t cchChosen) {
    OPENFILENAME ofn;
    char szTitle[256], szFilter[128], *pch;

    memset(&ofn, 0, sizeof(ofn));
    memset(pszChosen, 0, cchChosen);
    TextIO().pfnCchGetString(idsStarsGameFilesMHstRStars, szFilter);
    pch = strstr(szFilter, "*.M*;");
    if (pch) {
        memmove(pch + 11, pch, strlen(pch) + 1);
        memcpy(pch, "*.ZIP;*.XY;", 11);
    }
    for (pch = szFilter; *pch; pch++) if (*pch == '|') *pch = 0;
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = pszChosen;
    ofn.nMaxFile = cchChosen;
    szTitle[0] = 0;
    ofn.lpstrFileTitle = szTitle;
    ofn.nMaxFileTitle = sizeof(szTitle);
    ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    return DesktopChoiceIO().pfnFNativeOpenFileName(&ofn);
}

/* Choose the workflow from the BOF type, leaving complete validation and
 * source-preserving publication to the corresponding importer. */
int16_t FImportDesktopFile(HWND hwnd, const char *pszSource) {
    char szChosen[512];
    NATIVEFILE *pf;
    HDR hdr;
    RTBOF rtbof;
    int16_t fRace;

    if (!pszSource) {
        if (!FChooseLibraryImportFile(hwnd, szChosen, sizeof(szChosen))) return FALSE;
        pszSource = szChosen;
    }
    fRace = FALSE;
    pf = PfOpenNativeFile(pszSource, "rb");
    if (pf) {
        fRace = FReadLibraryHdr(pf, &hdr) && hdr.rt == rtBOF && hdr.cb == cbWireBof &&
                FReadLibraryBof(pf, &rtbof) && !memcmp(rtbof.rgid, "J3J3", 4) && rtbof.dt == dtRace;
        ICloseNativeFile(pf);
    }
    return fRace ? FImportLibraryRaceAndEdit(hwnd, pszSource) : FImportLibraryGame(hwnd, pszSource);
}

int16_t FImportLibraryGame(HWND hwnd, const char *pszSource) {
    char szChosen[512];
    char szSource[512], szDir[512], szStem[256], *pch, *pchSlash, *pchName;
    char szZipStage[512];
    LIBRARYIMPORT li;
    int cchDir;
    int64_t cbSource;
    int16_t fImported, iZipResult;

    if (!pszSource) {
        if (!FChooseLibraryImportFile(hwnd, szChosen, sizeof(szChosen))) return FALSE;
        pszSource = szChosen;
    }
    if (strlen(pszSource) >= sizeof(szSource)) return FALSE;
    strcpy(szSource, pszSource);
    /* Validate the selected path before either standalone-turn or family import. */
    if (!FNativeDirectoryExists(szSource) &&
        (!FNativeFileSize(szSource, &cbSource) || cbSource <= 0)) goto LInvalidSource;
    if (FPathIsZip(szSource)) {
        if (!FInitLibrary()) return FALSE;
        if (!FCreateLibraryZipStage(szZipStage, sizeof(szZipStage), "zip-importing")) return FALSE;
        iZipResult = FUnzipLibraryArchive(szSource, szZipStage);
        if (iZipResult != ZIP_IMPORT_OK) {
            FNativeEnumerateDirectory(szZipStage, RemoveImportTreeEntry, szZipStage);
            RemoveNativePath(szZipStage);
            if (iZipResult == ZIP_IMPORT_UNSUPPORTED_NAME) {
                AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryZipNameEncoding), MB_ICONEXCLAMATION);
                return FALSE;
            }
            AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryZipInvalid), MB_ICONEXCLAMATION);
            return FALSE;
        }
        fImported = FImportLibraryGame(hwnd, szZipStage);
        FNativeEnumerateDirectory(szZipStage, RemoveImportTreeEntry, szZipStage);
        RemoveNativePath(szZipStage);
        return fImported;
    }
    if (strrchr(pszSource, '.') && SDL_tolower(strrchr(pszSource, '.')[1]) == 'm') {
        if (!FInitLibrary()) return FALSE;
        fImported = FImportSubsequentTurn(hwnd, pszSource);
        if (fImported >= 0) return fImported;
    }
    if (FNativeDirectoryExists(szSource)) {
        strcpy(szDir, szSource);
        szStem[0] = 0;
        FNativeEnumerateDirectory(szDir, FindImportUniverse, szStem);
        if (!szStem[0] || szStem[0] == '!') {
            AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryUniverseRequired), MB_ICONEXCLAMATION);
            return FALSE;
        }
        CchSnprintf(szSource, sizeof(szSource), "%s%s%s.xy", szDir, szDirSep, szStem);
        pchSlash = strrchr(szSource, chDirSep);
    } else {
        pchSlash = strrchr(szSource, '/');
        pch = strrchr(szSource, '\\');
        if (pch && (!pchSlash || pch > pchSlash)) pchSlash = pch;
        if (pchSlash) {
            cchDir = (int)(pchSlash - szSource);
            if (cchDir == 0) cchDir = 1;
            memcpy(szDir, szSource, cchDir);
            szDir[cchDir] = 0;
            pchName = pchSlash + 1;
        } else {
            strcpy(szDir, ".");
            pchName = szSource;
        }
        pch = strrchr(pchName, '.');
        if (!pch || pch == pchName || (size_t)(pch - pchName) >= sizeof(szStem)) goto LInvalidSource;
        memcpy(szStem, pchName, pch - pchName);
        szStem[pch - pchName] = 0;
    }
    if (!FInitLibrary()) return FALSE;
    memset(&li, 0, sizeof(li));
    li.pszStem = szStem;
    li.pszSource = szDir;
    li.fValid = FALSE;
    if (!FNativeEnumerateDirectory(szDir, CopyImportEntry, &li) || li.fBad || !li.fSawXY || !li.fValid ||
        (li.grfPlayers == 0 && !li.fSawHost)) {
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryGameIncomplete), MB_ICONEXCLAMATION);
        return FALSE;
    }
    if (!FChooseLibraryGameName(hwnd, szSource, NULL)) return FALSE;
    strcpy(szLibraryExchangeName, szStem);
    return FPublishImportedLibraryGame({szLibraryRoot,szLibraryPending,szLibraryName},li,szDir);
LInvalidSource:
    CchSnprintf(szWork, sizeof(szWork), "%s", szSource);
    AlertIO().pfnAlertSz(TextIO().pfnFormat(idsCantOpenFile, NULL), MB_ICONHAND);
    return FALSE;
}

typedef struct {
    const char *pszDirectory;
    int16_t fFailed;
} LIBRARYGAMEREMOVE;

int16_t RemoveLibraryGameTree(const char *pszDirectory);

static int16_t RemoveLibraryGameEntry(void *lpData, const char *pszName) {
    LIBRARYGAMEREMOVE *plgr = (LIBRARYGAMEREMOVE *)lpData;
    char szPath[512];
    int16_t fDirectory, fLink;
    int cch = CchSnprintf(szPath, sizeof(szPath), "%s%s%s", plgr->pszDirectory, szDirSep, pszName);
    if (cch <= 0 || (size_t)cch >= sizeof(szPath) || !FNativePathInfo(szPath, &fDirectory, &fLink)) {
        plgr->fFailed = TRUE;
        return FALSE;
    }
    /* A link belongs to this tree; its target does not. */
    if (fDirectory && !fLink) {
        if (!RemoveLibraryGameTree(szPath)) {
            plgr->fFailed = TRUE;
            return FALSE;
        }
        return TRUE;
    }
    if ((fDirectory || fLink ? RemoveNativePath(szPath) : RemoveNativeFile(szPath)) != 0) {
        plgr->fFailed = TRUE;
        return FALSE;
    }
    return TRUE;
}

int16_t RemoveLibraryGameTree(const char *pszDirectory) {
    LIBRARYGAMEREMOVE lgr;
    int16_t fDirectory, fLink;
    if (!FNativePathInfo(pszDirectory, &fDirectory, &fLink)) return FALSE;
    if (fDirectory && !fLink) {
        lgr.pszDirectory = pszDirectory;
        lgr.fFailed = FALSE;
        if (!FNativeEnumerateDirectory(pszDirectory, RemoveLibraryGameEntry, &lgr) || lgr.fFailed) return FALSE;
    }
    return RemoveNativePath(pszDirectory) == 0;
}

int16_t FCreateLibraryGameStage(const char *pszSource, const char *pszSourceName,
                                       const char *pszTargetName, const char *pszStage,
                                       int iplr, long long lPlayed, const char *pszExchangeName) {
    LIBRARYGAMECOPY lgc;
    char szSourceUniverse[512];
    int64_t cbFile;
    int cch;
    lgc.pszSource = pszSource;
    lgc.pszTarget = pszStage;
    lgc.pszSourceName = pszSourceName;
    lgc.pszTargetName = pszTargetName;
    lgc.fBad = FALSE;
    cch = CchSnprintf(szSourceUniverse, sizeof(szSourceUniverse), "%s%s%s.xy", pszSource, szDirSep, pszSourceName);
    if (cch <= 0 || (size_t)cch >= sizeof(szSourceUniverse)) return FALSE;
    if (!FNativeFileSize(szSourceUniverse, &cbFile) || cbFile <= 0 ||
        !FNativeEnumerateDirectory(pszSource, FCopyLibraryGameEntry, &lgc) || lgc.fBad ||
        !FWriteLibraryMetadataValues(pszStage, pszTargetName, iplr, lPlayed, pszExchangeName)) return FALSE;
    return TRUE;
}

int16_t FCreateLibraryGameStagePath(char *pszStage, size_t cchStage) {
    uint32_t dwTick = DwTickCount();
    int i, cch;
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(pszStage, cchStage, "%s%s.stars-game-stage-%u-%u",
                          szLibraryRoot, szDirSep, dwTick, i);
        if (cch <= 0 || (size_t)cch >= cchStage) return FALSE;
        if (CreateNativeDir(pszStage) == 0) return TRUE;
        if (!FNativeDirectoryExists(pszStage)) return FALSE;
    }
    return FALSE;
}

int16_t FCreateLibraryGameHoldPath(char *pszHold, size_t cchHold) {
    int64_t cbExisting;
    uint32_t dwTick = DwTickCount();
    int i, cch;
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(pszHold, cchHold, "%s%s.stars-game-old-%u-%u",
                          szLibraryRoot, szDirSep, dwTick, i);
        if (cch <= 0 || (size_t)cch >= cchHold) return FALSE;
        if (!FNativeDirectoryExists(pszHold) && !FNativeFileSize(pszHold, &cbExisting)) return TRUE;
    }
    return FALSE;
}

int16_t FValidManagedLibraryGame(const char *pszName, char *pszDirectory,
                                        int *piplr, long long *plPlayed,
                                        char *pszExchangeName, size_t cchExchange) {
    char szMetadataName[256];
    int cchDirectory;
    int16_t fDirectory, fLink;
    if (!pszName || !FInitLibrary() || !FPortableLibraryName(pszName) ||
        (cchDirectory = CchSnprintf(pszDirectory, 512, "%s%s%s", szLibraryRoot, szDirSep, pszName)) <= 0 ||
        (size_t)cchDirectory >= 512 ||
        !FNativePathInfo(pszDirectory, &fDirectory, &fLink) || !fDirectory || fLink ||
        !FReadLibraryMetadata(pszDirectory, szMetadataName, sizeof(szMetadataName), piplr,
                              plPlayed, pszExchangeName, cchExchange)) return FALSE;
    return !SDL_strcasecmp(szMetadataName, pszName);
}

int16_t FDuplicateLibraryGame(HWND hwnd, const char *pszName, const char *pszCopyName) {
    char szSource[512], szStage[512], szTarget[512], szExchangeName[256], szMessage[256];
    int iplr;
    int cchTarget;
    long long lPlayed;
    int16_t fResult = FALSE;
    (void)hwnd;
    if (!pszCopyName || !FValidManagedLibraryGame(pszName, szSource, &iplr, &lPlayed,
                                                  szExchangeName, sizeof(szExchangeName)) ||
        !FPortableLibraryName(pszCopyName) || !FLibraryNameAvailableExcept(pszCopyName, NULL) ||
        (cchTarget = CchSnprintf(szTarget, sizeof(szTarget), "%s%s%s", szLibraryRoot, szDirSep, pszCopyName)) <= 0 ||
        (size_t)cchTarget >= sizeof(szTarget) || strlen(szLibraryRoot) + 2 * strlen(pszCopyName) + 18 >= sizeof(szBase) ||
        !FCreateLibraryGameStagePath(szStage, sizeof(szStage))) return FALSE;
    if (FCreateLibraryGameStage(szSource, pszName, pszCopyName, szStage, -1, 0, szExchangeName) &&
        RenameNativeFile(szStage, szTarget) == 0) fResult = TRUE;
    if (!fResult) {
        RemoveLibraryGameTree(szStage);
        strcpy(szMessage, TextIO().pfnCompressed(idsLibraryStorageFailed));
        CompatChoiceIO().pfnMessageBoxA(IsWindow(hwnd) ? hwnd : hwndFrame, szMessage, TextIO().pfnCompressed(idsLibraryGames), MB_OK | MB_ICONHAND);
    }
    return fResult;
}

int16_t FRenameLibraryGame(HWND hwnd, const char *pszName, const char *pszNewName) {
    char szSource[512], szStage[512], szTarget[512], szHold[512], szExchangeName[256], szMessage[768];
    int iplr;
    long long lPlayed;
    int16_t fResult = FALSE, fHeld = FALSE, fRestoreFailed = FALSE, fHoldCleanupFailed = FALSE;
    int cchTarget;
    (void)hwnd;
    if (!pszNewName || !FValidManagedLibraryGame(pszName, szSource, &iplr, &lPlayed,
                                                 szExchangeName, sizeof(szExchangeName)) ||
        !FPortableLibraryName(pszNewName) || !FLibraryNameAvailableExcept(pszNewName, pszName) ||
        (cchTarget = CchSnprintf(szTarget, sizeof(szTarget), "%s%s%s", szLibraryRoot, szDirSep, pszNewName)) <= 0 ||
        (size_t)cchTarget >= sizeof(szTarget) || strlen(szLibraryRoot) + 2 * strlen(pszNewName) + 18 >= sizeof(szBase) ||
        !FCreateLibraryGameStagePath(szStage, sizeof(szStage))) return FALSE;
    if (!FCreateLibraryGameStage(szSource, pszName, pszNewName, szStage, iplr, lPlayed, szExchangeName) ||
        !FCreateLibraryGameHoldPath(szHold, sizeof(szHold)) || RenameNativeFile(szSource, szHold) != 0) goto LDone;
    fHeld = TRUE;
    if (RenameNativeFile(szStage, szTarget) != 0) {
        if (RenameNativeFile(szHold, szSource) == 0) fHeld = FALSE;
        else fRestoreFailed = TRUE;
        goto LDone;
    }
    fHeld = FALSE;
    fResult = TRUE;
    if (!RemoveLibraryGameTree(szHold)) fHoldCleanupFailed = TRUE;
LDone:
    if (!fResult && FNativeDirectoryExists(szStage)) RemoveLibraryGameTree(szStage);
    if (fHeld) {
        if (RenameNativeFile(szHold, szSource) == 0) { fHeld = FALSE; fRestoreFailed = FALSE; }
        else fRestoreFailed = TRUE;
    }
    if (!fResult) {
        if (fRestoreFailed) {
            cchTarget = CchSnprintf(szMessage, sizeof(szMessage),
                                    TextIO().pfnCompressed(idsLibraryRenameFailedOriginalAt), szHold);
            if (cchTarget <= 0 || (size_t)cchTarget >= sizeof(szMessage)) strcpy(szMessage, TextIO().pfnCompressed(idsLibraryStorageFailed));
        } else strcpy(szMessage, TextIO().pfnCompressed(idsLibraryStorageFailed));
        CompatChoiceIO().pfnMessageBoxA(IsWindow(hwnd) ? hwnd : hwndFrame, szMessage, TextIO().pfnCompressed(idsLibraryGames), MB_OK | MB_ICONHAND);
    } else if (fHoldCleanupFailed) {
        cchTarget = CchSnprintf(szMessage, sizeof(szMessage),
                                TextIO().pfnCompressed(idsLibraryRenameSucceededCleanupPath), szHold);
        if (cchTarget <= 0 || (size_t)cchTarget >= sizeof(szMessage)) strcpy(szMessage, TextIO().pfnCompressed(idsLibraryStorageFailed));
        CompatChoiceIO().pfnMessageBoxA(IsWindow(hwnd) ? hwnd : hwndFrame, szMessage, TextIO().pfnCompressed(idsLibraryGames), MB_OK | MB_ICONEXCLAMATION);
    }
    return fResult;
}

int16_t FDeleteLibraryGameRecentPaths(const char *pszDirectory) {
    char szIniFile[32], szSection[16], szEntry[16], szPath[512];
    char rgpszRecent[9][256];
    const char *pszRecent;
    int i, j, cch;
    int16_t fContinueRemoved = FALSE;

    if (!vrgszMRU) return TRUE;
    for (i = 0; i < 9; i++) {
        pszRecent = vrgszMRU + 256 * i;
        cch = CchSnprintf(szPath, sizeof(szPath), "%s", pszRecent);
        if (cch < 0 || (size_t)cch >= sizeof(szPath)) return FALSE;
        if (LibraryPathIO().pfnUnder(pszDirectory, szPath)) {
            rgpszRecent[i][0] = 0;
            if (i == 0) fContinueRemoved = TRUE;
        } else {
            CchSnprintf(rgpszRecent[i], sizeof(rgpszRecent[i]), "%s", pszRecent);
        }
    }
    /* Do not promote a different recent game into Continue when File1 was deleted. */
    if (!fContinueRemoved) {
        for (i = 0, j = 0; i < 9; i++) {
            if (!rgpszRecent[i][0]) continue;
            if (i != j) strcpy(rgpszRecent[j], rgpszRecent[i]);
            j++;
        }
        while (j < 9) rgpszRecent[j++][0] = 0;
    }
    TextIO().pfnCchGetString(idsStarsIni, szIniFile);
    TextIO().pfnCchGetString(idsFiles, szSection);
    TextIO().pfnCchGetString(idsFile1, szEntry);
    for (i = 0; i < 9; i++) {
        szEntry[strlen(szEntry) - 1] = (char)('1' + i);
        strcpy(vrgszMRU + 256 * i, rgpszRecent[i]);
        if (!ProfileTextIO().pfnWriteString(szSection, szEntry, rgpszRecent[i], szIniFile)) return FALSE;
    }
    if (fContinueRemoved && !ProfileTextIO().pfnWriteString(szSection, "File1", "", szIniFile)) return FALSE;
    return TRUE;
}

int16_t FDeleteLibraryGame(HWND hwnd, const char *pszName) {
    char szDirectory[512], szMetadata[512], szMessage[512], szMetadataName[256];
    char szExchangeName[256];
    int iplr;
    long long lPlayed;
    int cch;
    int16_t fContinueTarget;

    if (!FInitLibrary() || !FPortableLibraryName(pszName)) return FALSE;
    cch = CchSnprintf(szDirectory, sizeof(szDirectory), "%s%s%s", szLibraryRoot, szDirSep, pszName);
    if (cch <= 0 || (size_t)cch >= sizeof(szDirectory)) return FALSE;
    cch = CchSnprintf(szMetadata, sizeof(szMetadata), "%s%s.library", szDirectory, szDirSep);
    if (cch <= 0 || (size_t)cch >= sizeof(szMetadata) ||
        !FReadLibraryMetadata(szDirectory, szMetadataName, sizeof(szMetadataName), &iplr, &lPlayed,
                              szExchangeName, sizeof(szExchangeName)) || strcmp(szMetadataName, pszName)) return FALSE;
    cch = CchSnprintf(szMessage, sizeof(szMessage), TextIO().pfnCompressed(idsLibraryDeleteGamePrompt), pszName);
    if (cch <= 0 || (size_t)cch >= sizeof(szMessage)) return FALSE;
    if (CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage, TextIO().pfnCompressed(idsLibraryGames),
                   MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) != IDYES) return FALSE;
    fContinueTarget = FFindContinueLibraryGame() && !strcmp(continueLibrary.szName, pszName);
    if (!RemoveLibraryGameTree(szDirectory)) {
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryDeleteGameFailed), MB_ICONHAND);
        return FALSE;
    }
    /* Keep deletion from silently promoting another game on the next startup. */
    if (fContinueTarget)
        ProfileTextIO().pfnWriteString("Library", "ContinueDeleted", "1", TextIO().pfnCompressed(idsStarsIni));
    if (!FDeleteLibraryGameRecentPaths(szDirectory))
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryDeleteGameRecentFailed), MB_ICONEXCLAMATION);
    return TRUE;
}

int16_t FDeleteSelectedLibraryGame(HWND hwnd) {
    char szName[256];
    if (!FSelectedLibraryGameName(hwnd, szName, sizeof(szName))) return FALSE;
    return FDeleteLibraryGame(hwnd, szName);
}

static int16_t FInspectImportPlayer(const char *pszUniverse, int iplr) {
    NATIVEFILE *pf;
    HDR hdr;
    RTBOF rtbof;
    GAME gameImport;
    uint8_t rgbGame[cbWireGame];
    int32_t lSeed1, lSeed2;
    int16_t fRead;

    pf = PfOpenNativeFile(pszUniverse, "rb");
    if (!pf) return FALSE;
    fRead = FReadLibraryHdr(pf, &hdr) && hdr.rt == rtBOF && hdr.cb == cbWireBof &&
            FReadLibraryBof(pf, &rtbof) &&
            FReadLibraryHdr(pf, &hdr) && hdr.rt == rtGame && hdr.cb == cbWireGame &&
            CReadNative(rgbGame, cbWireGame, 1, pf) == 1;
    ICloseNativeFile(pf);
    if (!fRead) return FALSE;
    /* Inspect the selected universe without disturbing an open game's stream. */
    GetFileSeeds(&lSeed1, &lSeed2);
    SetFileXorStream(rtbof.lidGame, rtbof.lSaltTime, rtbof.turn, rtbof.iPlayer, rtbof.fCrippled);
    XorFileBuf((char *)rgbGame, cbWireGame);
    SetFileSeeds(lSeed1, lSeed2);
    FDecodeGame(std::as_bytes(std::span(rgbGame)), gameImport);
    return gameImport.lid == rtbof.lidGame && gameImport.cPlayer > 0 && gameImport.cPlayer <= 16 &&
           iplr >= 0 && iplr < gameImport.cPlayer;
}

int16_t FImportLibraryOrders(HWND hwnd, const char *pszSource) {
    OPENFILENAME ofn;
    char szChosen[512], szFilter[128], szName[256];
    char szHost[512], szUniverse[512], szTarget[512], szStage[512], szMessage[512];
    const char *pszFile, *pszExt, *pchSlash;
    char *pch;
    int32_t lidHost, lidUniverse, lidOrders;
    uint16_t turnHost, turnOrders, wGenHost, wGenOrders;
    int16_t fMulti;
    int iplr, cch;
    int64_t cbFile;
    uint32_t dwTick;
    HWND hwndList;
    LRESULT i;

    if (!pszSource) {
        memset(&ofn, 0, sizeof(ofn));
        memset(szChosen, 0, sizeof(szChosen));
        strcpy(szFilter, TextIO().pfnCompressed(idsLibraryOrdersImportFilter));
        for (pch = szFilter; *pch; pch++) if (*pch == '|') *pch = 0;
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = szFilter;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = szChosen;
        ofn.nMaxFile = sizeof(szChosen);
        ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        if (!DesktopChoiceIO().pfnFNativeOpenFileName(&ofn)) return FALSE;
        pszSource = szChosen;
    }
    if (!FInitLibrary() || !IsWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_HOST_GAMES))) return FALSE;
    hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_HOST_GAMES);
    i = ControlIO().pfnLUiSendMessage(hwndList, LB_GETCURSEL, 0, 0);
    if (i == LB_ERR) {
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryOrdersChooseHost), MB_ICONEXCLAMATION);
        return FALSE;
    }
    ControlIO().pfnLUiSendMessage(hwndList, LB_GETTEXT, i, (LPARAM)szName);
    pszFile = strrchr(pszSource, '/');
    pchSlash = strrchr(pszSource, '\\');
    if (pchSlash && (!pszFile || pchSlash > pszFile)) pszFile = pchSlash;
    pszFile = pszFile ? pszFile + 1 : pszSource;
    pszExt = strrchr(pszFile, '.');
    if (!pszExt || SDL_tolower(pszExt[1]) != 'x' || pszExt[2] < '1' || pszExt[2] > '9' ||
        (pszExt[3] && (pszExt[3] < '0' || pszExt[3] > '9')) || (pszExt[3] && pszExt[4])) {
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryOrdersChooseFile), MB_ICONEXCLAMATION);
        return FALSE;
    }
    iplr = pszExt[2] - '1';
    if (pszExt[3]) iplr = (pszExt[2] - '0') * 10 + pszExt[3] - '1';
    if (iplr < 0 || iplr >= 16 || !FInspectImportFile(pszSource, dtLog, iplr, &lidOrders,
            &turnOrders, &wGenOrders, &fMulti)) {
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryOrdersInvalid), MB_ICONEXCLAMATION);
        return FALSE;
    }
    cch = CchSnprintf(szHost, sizeof(szHost), "%s%s%s%s%s.hst", szLibraryRoot, szDirSep, szName, szDirSep, szName);
    if (cch <= 0 || (size_t)cch >= sizeof(szHost)) return FALSE;
    cch = CchSnprintf(szUniverse, sizeof(szUniverse), "%s%s%s%s%s.xy", szLibraryRoot, szDirSep, szName, szDirSep, szName);
    if (cch <= 0 || (size_t)cch >= sizeof(szUniverse)) return FALSE;
    if (!FInspectImportFile(szHost, dtHost, iplrNone, &lidHost, &turnHost, &wGenHost, &fMulti) ||
        !FInspectImportFile(szUniverse, dtXY, iplrNone, &lidUniverse, NULL, NULL, &fMulti) ||
        lidHost != lidUniverse || lidOrders != lidHost || turnOrders != turnHost || wGenOrders != wGenHost ||
        !FInspectImportPlayer(szUniverse, iplr)) {
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryOrdersMismatch), MB_ICONEXCLAMATION);
        return FALSE;
    }
    cch = CchSnprintf(szTarget, sizeof(szTarget), "%s%s%s%s%s.x%d", szLibraryRoot, szDirSep,
                      szName, szDirSep, szName, iplr + 1);
    if (cch <= 0 || (size_t)cch >= sizeof(szTarget)) return FALSE;
    if (FNativeFileSize(szTarget, &cbFile)) {
        if (FFilesEqual(pszSource, szTarget)) return TRUE;
        cch = CchSnprintf(szMessage, sizeof(szMessage),
                          TextIO().pfnCompressed(idsLibraryOrdersReplacePrompt), iplr + 1, szName);
        if (cch <= 0 || (size_t)cch >= sizeof(szMessage)) return FALSE;
        if (AlertIO().pfnAlertSz(szMessage, MB_YESNO | MB_ICONQUESTION) != IDYES) return FALSE;
    }
    dwTick = DwTickCount();
    cch = CchSnprintf(szStage, sizeof(szStage), "%s.importing.%u", szTarget, dwTick);
    if (cch <= 0 || (size_t)cch >= sizeof(szStage)) return FALSE;
    if (!FCopyNativeFileExclusive(pszSource, szStage) ||
        !FInspectImportFile(szStage, dtLog, iplr, &lidOrders, &turnOrders, &wGenOrders, &fMulti) ||
        lidOrders != lidHost || turnOrders != turnHost || wGenOrders != wGenHost) {
        RemoveNativeFile(szStage);
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryOrdersStageFailed), MB_ICONHAND);
        return FALSE;
    }
    if (ReplaceNativeFile(szStage, szTarget) != 0) {
        RemoveNativeFile(szStage);
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryOrdersInstallFailed), MB_ICONHAND);
        return FALSE;
    }
    (void)hwnd;
    return TRUE;
}


static int16_t FCopyOptionalConflictFile(const char *pszSource, const char *pszTarget) {
    int64_t cbFile;
    if (FNativeFileSize(pszSource, &cbFile)) return FCopyNativeFileExclusive(pszSource, pszTarget);
    return errno == ENOENT;
}

int16_t FBackupConflictingPlayerFiles(const char *pszDirectory, const char *pszName,
                                             int iplr, char *pszBackup, size_t cchBackup) {
    char szSource[512], szTarget[512], szExt[8];
    int i, cch;
    int16_t fDirectory, fLink;

    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(pszBackup, cchBackup, "%s%sbackup.%03d", pszDirectory, szDirSep, i);
        if (cch <= 0 || (size_t)cch >= cchBackup) return FALSE;
        if (FNativePathInfo(pszBackup, &fDirectory, &fLink)) continue;
        if (CreateNativeDir(pszBackup) == 0) break;
        if (!FNativeDirectoryExists(pszBackup)) return FALSE;
    }
    if (i == 1000) return FALSE;

    CchSnprintf(szExt, sizeof(szExt), "m%d", iplr + 1);
    cch = CchSnprintf(szSource, sizeof(szSource), "%s%s%s.%s", pszDirectory, szDirSep, pszName, szExt);
    if (cch <= 0 || (size_t)cch >= sizeof(szSource)) return FALSE;
    cch = CchSnprintf(szTarget, sizeof(szTarget), "%s%s%s.%s", pszBackup, szDirSep, pszName, szExt);
    if (cch <= 0 || (size_t)cch >= sizeof(szTarget) || !FCopyOptionalConflictFile(szSource, szTarget)) return FALSE;

    CchSnprintf(szExt, sizeof(szExt), "x%d", iplr + 1);
    cch = CchSnprintf(szSource, sizeof(szSource), "%s%s%s.%s", pszDirectory, szDirSep, pszName, szExt);
    if (cch <= 0 || (size_t)cch >= sizeof(szSource)) return FALSE;
    cch = CchSnprintf(szTarget, sizeof(szTarget), "%s%s%s.%s", pszBackup, szDirSep, pszName, szExt);
    if (cch <= 0 || (size_t)cch >= sizeof(szTarget) || !FCopyOptionalConflictFile(szSource, szTarget)) return FALSE;

    CchSnprintf(szExt, sizeof(szExt), "h%d", iplr + 1);
    cch = CchSnprintf(szSource, sizeof(szSource), "%s%s%s.%s", pszDirectory, szDirSep, pszName, szExt);
    if (cch <= 0 || (size_t)cch >= sizeof(szSource)) return FALSE;
    cch = CchSnprintf(szTarget, sizeof(szTarget), "%s%s%s.%s", pszBackup, szDirSep, pszName, szExt);
    return cch > 0 && (size_t)cch < sizeof(szTarget) && FCopyOptionalConflictFile(szSource, szTarget);
}

int16_t FStageConflictingTurn(const char *pszStage, const char *pszName,
                                     const char *pszSource, int iplr,
                                     int32_t lidGame, uint16_t turn) {
    char szTarget[512], szIncoming[512];
    int32_t lidStaged;
    uint16_t turnStaged, wGenStaged;
    int16_t fMultiStaged;
    int cch;

    cch = CchSnprintf(szTarget, sizeof(szTarget), "%s%s%s.m%d", pszStage, szDirSep, pszName, iplr + 1);
    if (cch <= 0 || (size_t)cch >= sizeof(szTarget)) return FALSE;
    cch = CchSnprintf(szIncoming, sizeof(szIncoming), "%s.incoming.%u", szTarget, DwTickCount());
    if (cch <= 0 || (size_t)cch >= sizeof(szIncoming)) return FALSE;
    if (!FCopyNativeFileExclusive(pszSource, szIncoming) ||
        !FInspectImportFile(szIncoming, dtTurn, iplr, &lidStaged, &turnStaged, &wGenStaged, &fMultiStaged) ||
        lidStaged != lidGame || turnStaged != turn || ReplaceNativeFile(szIncoming, szTarget) != 0) {
        RemoveNativeFile(szIncoming);
        return FALSE;
    }
    return TRUE;
}

int16_t FChooseSeparateCopyName(HWND hwnd, const char *pszName) {
    if (!FInitLibrary()) return FALSE;
    CchSnprintf(szLibraryName, sizeof(szLibraryName), "%s", pszName);
    SuggestLibraryName(szLibraryName, FALSE);
    if (ControlIO().pfnLUiDialogBox(hInst, MAKEINTRESOURCE(IDD_LIBRARY_NAME), hwnd,
                       LibraryNameDlg, (LPARAM)szLibraryName) != TRUE) return FALSE;
    return FPortableLibraryName(szLibraryName) && FLibraryNameAvailable(szLibraryRoot, szLibraryName);
}

int16_t FReplaceConflictingLibraryTurn(const LIBRARYPATHS &paths, HWND hwnd, const char *pszName,
                                             const char *pszSource, const char *pszTarget,
                                             int iplr, int32_t lidGame, uint16_t turn) {
    char szDirectory[512], szStage[512], szHold[512], szBackup[512], szExchangeName[256];
    char szMessage[768];
    int iRemembered;
    long long lPlayed;
    int16_t fHeld = FALSE, fRestoreFailed = FALSE, fCleanupFailed = FALSE;
    int cch;

    cch = LibraryPublicationIO().pfnCchSnprintf(szDirectory, sizeof(szDirectory), "%s%s%s", paths.szLibraryRoot, szDirSep, pszName);
    if (cch <= 0 || (size_t)cch >= sizeof(szDirectory) ||
        !LibraryPublicationIO().pfnFValidManagedLibraryGame(pszName, szDirectory, &iRemembered, &lPlayed,
                                  szExchangeName, sizeof(szExchangeName)) ||
        !LibraryPublicationIO().pfnFCreateLibraryGameStagePath(szStage, sizeof(szStage))) return FALSE;
    if (!LibraryPublicationIO().pfnFCreateLibraryGameStage(szDirectory, pszName, pszName, szStage, iRemembered,
                                 lPlayed, szExchangeName) ||
        !LibraryPublicationIO().pfnFBackupConflictingPlayerFiles(szStage, pszName, iplr, szBackup, sizeof(szBackup)) ||
        !LibraryPublicationIO().pfnFStageConflictingTurn(szStage, pszName, pszSource, iplr, lidGame, turn) ||
        !LibraryPublicationIO().pfnFCreateLibraryGameHoldPath(szHold, sizeof(szHold))) goto LDone;
    if (RenameNativeFile(szDirectory, szHold) != 0) goto LDone;
    fHeld = TRUE;
    if (RenameNativeFile(szStage, szDirectory) != 0) {
        if (RenameNativeFile(szHold, szDirectory) == 0) fHeld = FALSE;
        else fRestoreFailed = TRUE;
        goto LDone;
    }
    fHeld = FALSE;
    if (!LibraryPublicationIO().pfnRemoveLibraryGameTree(szHold)) fCleanupFailed = TRUE;
    if (fCleanupFailed) {
        LibraryPublicationIO().pfnCchSnprintf(szMessage, sizeof(szMessage), TextIO().pfnCompressed(idsLibraryTurnCleanupFailed), szHold);
        AlertIO().pfnAlertSz(szMessage, MB_ICONEXCLAMATION);
    }
    (void)hwnd;
    (void)pszTarget;
    return TRUE;
LDone:
    if (LibraryPublicationIO().pfnFNativeDirectoryExists(szStage)) LibraryPublicationIO().pfnRemoveLibraryGameTree(szStage);
    if (fHeld) {
        if (RenameNativeFile(szHold, szDirectory) == 0) { fHeld = FALSE; fRestoreFailed = FALSE; }
        else fRestoreFailed = TRUE;
    }
    if (fRestoreFailed) {
        LibraryPublicationIO().pfnCchSnprintf(szMessage, sizeof(szMessage), TextIO().pfnCompressed(idsLibraryGameRestoreFailed), szHold);
        AlertIO().pfnAlertSz(szMessage, MB_ICONHAND);
    } else AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryTurnReplaceFailed), MB_ICONHAND);
    (void)hwnd;
    (void)pszTarget;
    return FALSE;
}

int16_t FImportConflictingTurnAsCopy(const LIBRARYPATHS &paths, HWND hwnd, const char *pszName,
                                            const char *pszSource, int iplr,
                                            int32_t lidGame, uint16_t turn) {
    char szDirectory[512], szStage[512], szTarget[512], szExchangeName[256];
    int iRemembered, cch;
    long long lPlayed;

    cch = LibraryPublicationIO().pfnCchSnprintf(szDirectory, sizeof(szDirectory), "%s%s%s", paths.szLibraryRoot, szDirSep, pszName);
    if (cch <= 0 || (size_t)cch >= sizeof(szDirectory) ||
        !LibraryPublicationIO().pfnFValidManagedLibraryGame(pszName, szDirectory, &iRemembered, &lPlayed,
                                  szExchangeName, sizeof(szExchangeName)) ||
        !LibraryPublicationIO().pfnFChooseSeparateCopyName(hwnd, pszName)) return FALSE;
    cch = LibraryPublicationIO().pfnCchSnprintf(szTarget, sizeof(szTarget), "%s%s%s", paths.szLibraryRoot, szDirSep, paths.szLibraryName);
    if (cch <= 0 || (size_t)cch >= sizeof(szTarget) ||
        !LibraryPublicationIO().pfnFLibraryNameAvailable(paths.szLibraryRoot, paths.szLibraryName) ||
        LibraryPublicationIO().pfnPathLength(paths.szLibraryRoot) + 2 * LibraryPublicationIO().pfnPathLength(paths.szLibraryName) + 18 >= sizeof(szBase) ||
        !LibraryPublicationIO().pfnFCreateLibraryGameStagePath(szStage, sizeof(szStage))) return FALSE;
    if (!LibraryPublicationIO().pfnFCreateLibraryGameStage(szDirectory, pszName, paths.szLibraryName, szStage,
                                 iplr, 0, szExchangeName) ||
        !LibraryPublicationIO().pfnFStageConflictingTurn(szStage, paths.szLibraryName, pszSource, iplr, lidGame, turn) ||
        RenameNativeFile(szStage, szTarget) != 0) {
        if (LibraryPublicationIO().pfnFNativeDirectoryExists(szStage)) LibraryPublicationIO().pfnRemoveLibraryGameTree(szStage);
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibrarySeparateCopyFailed), MB_ICONHAND);
        return FALSE;
    }
    (void)iRemembered;
    (void)lPlayed;
    return TRUE;
}

int16_t FFinishLibraryGame(HWND hwnd, int16_t fGenerated) {
    char szFile[256], szMessage[256];
    int i;

    if (!szLibraryPending[0]) return FALSE;
    for (i = -2; fGenerated && i < game.cPlayer; i++) {
        if (i == -2) CchSnprintf(szFile, sizeof(szFile), "%s.xy", szBase);
        else if (i == -1) CchSnprintf(szFile, sizeof(szFile), "%s.hst", szBase);
        else CchSnprintf(szFile, sizeof(szFile), "%s.m%d", szBase, i + 1);
        if (!FCompleteGeneratedFile(szFile, i == -2 ? dtXY : i == -1 ? dtHost : dtTurn, i < 0 ? iplrNone : i)) fGenerated = FALSE;
    }
    if (fGenerated) fGenerated = FWriteLibraryMetadata(szLibraryPending, szLibraryName, idPlayer);
    if (!fGenerated) {
        FNativeEnumerateDirectory(szLibraryPending, RemovePendingFile, NULL);
        RemoveNativePath(szLibraryPending);
        strcpy(szMessage, TextIO().pfnCompressed(idsLibraryStorageFailed));
        CompatChoiceIO().pfnMessageBoxA(IsWindow(hwnd) ? hwnd : hwndFrame, szMessage, TextIO().pfnCompressed(idsLibraryGames), MB_OK | MB_ICONHAND);
    }
    szLibraryPending[0] = 0;
    return fGenerated;
}

static int16_t FAddLibraryGameListItem(HWND hwndList, const char *pszName, int iplr,
                                       uint16_t grfPlayers, const uint16_t *rgturn) {
    LRESULT i;
    int cNew;
    LIBRARYGAMELISTITEM *rgNew;
    if (cLibraryGameList == cLibraryGameListMax) {
        cNew = cLibraryGameListMax ? cLibraryGameListMax * 2 : 32;
        rgNew = (LIBRARYGAMELISTITEM *)SDL_realloc(rgLibraryGameList.get(), (size_t)cNew * sizeof(*rgNew));
        if (!rgNew) return FALSE;
        rgNew = LpStartHeapArray<LIBRARYGAMELISTITEM>(rgNew,(size_t)cNew * sizeof(*rgNew));
        rgLibraryGameList.release();
        rgLibraryGameList.reset(rgNew);
        cLibraryGameListMax = cNew;
    }
    i = ControlIO().pfnLUiSendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)pszName);
    if (i == LB_ERR || i == LB_ERRSPACE) return FALSE;
    CchSnprintf(rgLibraryGameList[cLibraryGameList].szName,
                sizeof(rgLibraryGameList[cLibraryGameList].szName), "%s", pszName);
    rgLibraryGameList[cLibraryGameList].iplr = iplr;
    rgLibraryGameList[cLibraryGameList].grfPlayers = grfPlayers;
    memcpy(rgLibraryGameList[cLibraryGameList].rgturn, rgturn, sizeof(rgLibraryGameList[cLibraryGameList].rgturn));
    ControlIO().pfnLUiSendMessage(hwndList, LB_SETITEMDATA, i, cLibraryGameList++);
    return TRUE;
}

static void ClearLibraryGameList(void) {
    rgLibraryGameList.reset();
    cLibraryGameList = 0;
    cLibraryGameListMax = 0;
}

static int16_t ListLibraryGame(void *lpList, const char *pszName) {
    char szFile[256], szDirectory[256], szMetadataName[256], szExchangeName[256];
    int iplr, iplrSaved, cch;
    int64_t cbFile;
    long long lPlayed;
    int32_t lidGame;
    uint16_t turn;
    uint16_t grfPlayers = 0, rgturn[16] = {0};
    int16_t fMulti;

    if (!FPortableLibraryName(pszName)) return TRUE;
    cch = CchSnprintf(szDirectory, sizeof(szDirectory), "%s%s%s", szLibraryRoot, szDirSep, pszName);
    if (cch <= 0 || (size_t)cch >= sizeof(szDirectory) ||
        !FReadLibraryMetadata(szDirectory, szMetadataName, sizeof(szMetadataName), &iplrSaved, &lPlayed,
                              szExchangeName, sizeof(szExchangeName))) return TRUE;
    CchSnprintf(szFile, sizeof(szFile), "%s%s%s%s%s.xy", szLibraryRoot, szDirSep, pszName, szDirSep, pszName);
    if (!FNativeFileSize(szFile, &cbFile) || cbFile == 0) return TRUE;
    for (iplr = 0; iplr < 16; iplr++) {
        CchSnprintf(szFile, sizeof(szFile), "%s%s%s%s%s.m%d", szLibraryRoot, szDirSep, pszName, szDirSep, pszName, iplr + 1);
        if (!FNativeFileSize(szFile, &cbFile) || cbFile <= 0 ||
            !FInspectImportFile(szFile, dtTurn, iplr, &lidGame, &turn, NULL, &fMulti)) continue;
        grfPlayers |= (uint16_t)(1u << iplr);
        rgturn[iplr] = turn;
    }
    return !grfPlayers || FAddLibraryGameListItem((HWND)lpList, pszName, iplrSaved, grfPlayers, rgturn);
}

static int16_t ListLibraryHostGame(void *lpList, const char *pszName) {
    char szHost[512], szUniverse[512], szMetadata[512];
    NATIVEFILE *pf;
    int64_t cbFile;
    int32_t lidHost, lidUniverse;
    uint16_t turnHost;
    int16_t fMulti;
    LRESULT i;

    if (!FPortableLibraryName(pszName)) return TRUE;
    CchSnprintf(szMetadata, sizeof(szMetadata), "%s%s%s%s.library", szLibraryRoot, szDirSep, pszName, szDirSep);
    pf = PfOpenNativeFile(szMetadata, "rb");
    if (!pf) return TRUE;
    ICloseNativeFile(pf);
    CchSnprintf(szHost, sizeof(szHost), "%s%s%s%s%s.hst", szLibraryRoot, szDirSep, pszName, szDirSep, pszName);
    CchSnprintf(szUniverse, sizeof(szUniverse), "%s%s%s%s%s.xy", szLibraryRoot, szDirSep, pszName, szDirSep, pszName);
    if (!FNativeFileSize(szHost, &cbFile) || cbFile <= 0 ||
        !FInspectImportFile(szHost, dtHost, iplrNone, &lidHost, &turnHost, NULL, &fMulti) ||
        !FInspectImportFile(szUniverse, dtXY, iplrNone, &lidUniverse, NULL, NULL, &fMulti) ||
        lidHost != lidUniverse) return TRUE;
    i = ControlIO().pfnLUiSendMessage((HWND)lpList, LB_ADDSTRING, 0, (LPARAM)pszName);
    return i != LB_ERR && i != LB_ERRSPACE;
}

static int16_t FindContinueLibraryGame(void *lpData, const char *pszName) {
    LIBRARYCONTINUE *plc = (LIBRARYCONTINUE *)lpData;
    char szFile[512], szMetadataName[256];
    NATIVEFILE *pf;
    int iVersion, iPlayer;
    long long lPlayed;
    int32_t lidGame;
    uint16_t turn;
    int16_t fMulti;
    int iYear;
    int16_t fTurnAvailable;

    if (!FPortableLibraryName(pszName)) return TRUE;
    CchSnprintf(szFile, sizeof(szFile), "%s%s%s%s.library", szLibraryRoot, szDirSep, pszName, szDirSep);
    pf = PfOpenNativeFile(szFile, "rb");
    if (!pf) return TRUE;
    iVersion = 0;
    iPlayer = -1;
    lPlayed = 0;
    if (CNativeScan(pf, "%d\n", &iVersion) != 1 || !PszGetNativeLine(szMetadataName, sizeof(szMetadataName), pf) ||
        CNativeScan(pf, "%d\n%lld", &iPlayer, &lPlayed) != 2) {
        ICloseNativeFile(pf);
        return TRUE;
    }
    ICloseNativeFile(pf);
    szMetadataName[strcspn(szMetadataName, "\r\n")] = 0;
    if ((iVersion != 1 && iVersion != 2) || _stricmp(szMetadataName, pszName) ||
        iPlayer < 0 || iPlayer >= 16 || (plc->fFound && lPlayed <= plc->lPlayed)) return TRUE;

    fTurnAvailable = FALSE;
    iYear = 0;
    CchSnprintf(szFile, sizeof(szFile), "%s%s%s%s%s.m%d", szLibraryRoot, szDirSep, pszName, szDirSep,
                pszName, iPlayer + 1);
    if (FInspectImportFile(szFile, dtTurn, iPlayer, &lidGame, &turn, NULL, &fMulti)) {
        iYear = turn + 2400;
        fTurnAvailable = TRUE;
    }
    strcpy(plc->szName, pszName);
    plc->iPlayer = iPlayer;
    plc->iYear = iYear;
    plc->lPlayed = lPlayed;
    plc->fFound = TRUE;
    plc->fTurnAvailable = fTurnAvailable;
    return TRUE;
}

static int16_t FFindContinueLibraryGame(void) {
    if (GetPrivateProfileInt("Library", "ContinueDeleted", 0, TextIO().pfnCompressed(idsStarsIni))) return FALSE;
    memset(&continueLibrary, 0, sizeof(continueLibrary));
    continueLibrary.iPlayer = -1;
    return FInitLibrary() && FNativeEnumerateDirectory(szLibraryRoot, FindContinueLibraryGame, &continueLibrary) &&
           continueLibrary.fFound;
}

void InitLibraryGames(HWND hwnd, const RECT *lprc) {
    int y;
    Uint64 msStart = CompatTimeIO().pfnGetTicks();

    StarsCompatSetWindowAlertSounds(hwnd, FALSE);
    fHostLibraryView = FALSE;
    if (getenv("STARS_DEBUG_LIBRARY_STARTUP"))
        fprintf(stderr, "[DEBUG-library-startup] InitLibraryGames begin hwnd=%p\n", (void *)hwnd);
    y = StarsMax(50, lprc->bottom - 360);
    HwndUiCreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(idsLibraryGames), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 24, y - 22, 58, 18, hwnd,
                 (HMENU)(uintptr_t)IDC_LIBRARY_GAMES_VIEW, hInst, NULL);
    HwndUiCreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(idsLibraryHostGames), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 86, y - 22, 82, 18, hwnd,
                 (HMENU)(uintptr_t)IDC_LIBRARY_HOST_VIEW, hInst, NULL);
    HwndUiCreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(idsLibraryImportGame), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 128, y - 22, 104, 18, hwnd,
                 (HMENU)(uintptr_t)IDC_LIBRARY_IMPORT, hInst, NULL);
    HwndUiCreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(idsLibraryImportFolder), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 240, y - 22, 104, 18, hwnd,
                 (HMENU)(uintptr_t)IDC_LIBRARY_IMPORT_FOLDER, hInst, NULL);
    HwndUiCreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(idsLibraryImportOrders), WS_CHILD | WS_TABSTOP,
                 lprc->right - 128, y - 22, 104, 18, hwnd,
                 (HMENU)(uintptr_t)IDC_LIBRARY_IMPORT_ORDERS, hInst, NULL);
    HwndUiCreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(idsLibraryExportGame), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 464, y - 22, 96, 18, hwnd,
                 (HMENU)(uintptr_t)IDC_LIBRARY_GAME_EXPORT, hInst, NULL);
    HwndUiCreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(idsLibraryDuplicate), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 304, y - 22, 60, 18, hwnd,
                 (HMENU)(uintptr_t)IDC_LIBRARY_DUPLICATE, hInst, NULL);
    HwndUiCreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(idsRename), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 360, y - 22, 52, 18, hwnd,
                 (HMENU)(uintptr_t)IDC_LIBRARY_RENAME, hInst, NULL);
    HwndUiCreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(idsDelete), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 524, y - 22, 56, 18, hwnd,
                 (HMENU)(uintptr_t)IDC_LIBRARY_GAME_DELETE, hInst, NULL);
    HwndUiCreateWindowEx(0, "LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_SORT,
                 24, y, lprc->right - 48, 90, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_GAMES, hInst, NULL);
    HwndUiCreateWindowEx(0, "LISTBOX", "", WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_SORT,
                 24, y, lprc->right - 48, 90, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_HOST_GAMES, hInst, NULL);
    HwndUiCreateWindowEx(0, "STATIC", TextIO().pfnCompressed(idsLibraryPlayerSelect), WS_CHILD | WS_VISIBLE,
                 24, y + 100, 66, 18, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_PLAYER_LABEL, hInst, NULL);
    HwndUiCreateWindowEx(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
                 94, y + 96, StarsMin(280, lprc->right - 118), 160, hwnd,
                 (HMENU)(uintptr_t)IDC_LIBRARY_PLAYERS, hInst, NULL);
    HwndUiCreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
                 24, y + 122, lprc->right - 48, 18, hwnd,
                 (HMENU)(uintptr_t)IDC_LIBRARY_CONTINUE_DETAILS, hInst, NULL);
    FUiShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_IMPORT_ORDERS), SW_HIDE);
    FUiShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_HOST_GAMES), SW_HIDE);
    RefreshLibraryGames(hwnd);
    RefreshLibraryHostGames(hwnd);
    if (getenv("STARS_DEBUG_LIBRARY_STARTUP"))
        fprintf(stderr, "[DEBUG-library-startup] InitLibraryGames end elapsed=%llu ms\n",
                (unsigned long long)(CompatTimeIO().pfnGetTicks() - msStart));
}

typedef struct {
    FNativeDirectoryEntryProc pfnEntry;
    void *lpData;
} LIBRARYRACEENUM;

static int16_t EnumLibraryRace(void *lpData, const char *pszName) {
    LIBRARYRACEENUM *plre = (LIBRARYRACEENUM *)lpData;
    char szPath[512];
    int64_t cbFile;
    if (!FPortableRaceFilename(pszName)) return TRUE;
    CchSnprintf(szPath, sizeof(szPath), "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    if (!FNativeFileSize(szPath, &cbFile) || cbFile <= 0) return TRUE;
    return plre->pfnEntry(plre->lpData, pszName);
}

int16_t FEnumerateLibraryRaces(FNativeDirectoryEntryProc pfnEntry, void *lpData) {
    LIBRARYRACEENUM lre = {pfnEntry, lpData};

    return FInitRaceLibrary() && FNativeEnumerateDirectory(szRaceLibraryRoot, EnumLibraryRace, &lre);
}

static int16_t ListLibraryRace(void *lpList, const char *pszName) {
    LRESULT i;

    i = ControlIO().pfnLUiSendMessage((HWND)lpList, LB_ADDSTRING, 0, (LPARAM)pszName);
    return i != LB_ERR && i != LB_ERRSPACE;
}

void InitLibraryRaces(HWND hwnd, const RECT *lprc) {
    int y = StarsMax(160, lprc->bottom - 200);
    HwndUiCreateWindowEx(0, "STATIC", TextIO().pfnCompressed(idsLibraryRaceDesigns), WS_CHILD | WS_VISIBLE, 24, y - 18, 160, 18,
                 hwnd, NULL, hInst, NULL);
    HwndUiCreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(idsLibraryNew), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 436, y - 20, 48, 18, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_RACE_NEW, hInst, NULL);
    HwndUiCreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(idsLibraryEdit), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 384, y - 20, 48, 18, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_RACE_EDIT, hInst, NULL);
    HwndUiCreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(idsLibraryDuplicate), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 332, y - 20, 62, 18, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_RACE_DUPLICATE, hInst, NULL);
    HwndUiCreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(idsLibraryExport), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 266, y - 20, 48, 18, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_RACE_EXPORT, hInst, NULL);
    HwndUiCreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(idsDelete), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 214, y - 20, 56, 18, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_RACE_DELETE, hInst, NULL);
    HwndUiCreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(idsLibraryImport), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                 lprc->right - 152, y - 20, 112, 18, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_RACE_IMPORT, hInst, NULL);
    HwndUiCreateWindowEx(0, "LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_SORT,
                 24, y, lprc->right - 48, 80, hwnd, (HMENU)(uintptr_t)IDC_LIBRARY_RACES, hInst, NULL);
    RefreshLibraryRaces(hwnd);
}

void RefreshLibraryRaces(HWND hwnd) {
    HWND hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RACES);
    if (!hwndList) return;
    ControlIO().pfnLUiSendMessage(hwndList, LB_RESETCONTENT, 0, 0);
    FEnumerateLibraryRaces(ListLibraryRace, hwndList);
    ControlIO().pfnLUiSendMessage(hwndList, LB_SETCURSEL, 0, 0);
    ControlIO().pfnFUiEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RACE_EDIT), ControlIO().pfnLUiSendMessage(hwndList, LB_GETCOUNT, 0, 0) > 0);
    ControlIO().pfnFUiEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RACE_DUPLICATE), ControlIO().pfnLUiSendMessage(hwndList, LB_GETCOUNT, 0, 0) > 0);
    ControlIO().pfnFUiEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RACE_EXPORT), ControlIO().pfnLUiSendMessage(hwndList, LB_GETCOUNT, 0, 0) > 0);
    ControlIO().pfnFUiEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RACE_DELETE), ControlIO().pfnLUiSendMessage(hwndList, LB_GETCOUNT, 0, 0) > 0);
}

int16_t FSelectLibraryRace(HWND hwnd, char *pszName) {
    HWND hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RACES);
    LRESULT i = ControlIO().pfnLUiSendMessage(hwndList, LB_GETCURSEL, 0, 0);
    if (i == LB_ERR) return FALSE;
    ControlIO().pfnLUiSendMessage(hwndList, LB_GETTEXT, i, (LPARAM)pszName);
    return TRUE;
}

void RefreshLibraryPlayers(HWND hwnd) {
    HWND hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAMES);
    HWND hwndPlayers = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_PLAYERS);
    LRESULT i, iItem, iPlayer, iSelected = CB_ERR;
    int iplr;
    char szPlayer[64];
    LIBRARYGAMELISTITEM *plgli;

    if (!hwndPlayers) return;
    ControlIO().pfnLUiSendMessage(hwndPlayers, CB_RESETCONTENT, 0, 0);
    i = ControlIO().pfnLUiSendMessage(hwndList, LB_GETCURSEL, 0, 0);
    iItem = i == LB_ERR ? LB_ERR : ControlIO().pfnLUiSendMessage(hwndList, LB_GETITEMDATA, i, 0);
    if (iItem >= 0 && iItem < cLibraryGameList) {
        plgli = &rgLibraryGameList[iItem];
        for (iplr = 0; iplr < 16; iplr++) {
            if (!(plgli->grfPlayers & (1u << iplr))) continue;
            CchSnprintf(szPlayer, sizeof(szPlayer), TextIO().pfnCompressed(idsLibraryPlayerTurn),
                        iplr + 1, plgli->rgturn[iplr] + 2400);
            iPlayer = ControlIO().pfnLUiSendMessage(hwndPlayers, CB_ADDSTRING, 0, (LPARAM)szPlayer);
            if (iPlayer == CB_ERR || iPlayer == CB_ERRSPACE) break;
            ControlIO().pfnLUiSendMessage(hwndPlayers, CB_SETITEMDATA, iPlayer, iplr);
            if (iSelected == CB_ERR || iplr == plgli->iplr) iSelected = iPlayer;
        }
    }
    ControlIO().pfnLUiSendMessage(hwndPlayers, CB_SETCURSEL, iSelected, 0);
    ControlIO().pfnFUiEnableWindow(hwndPlayers, iSelected != CB_ERR);
    if (!fHostLibraryView) ControlIO().pfnFUiEnableWindow(rghwndBtnSplash[1], iSelected != CB_ERR);
}

void RefreshLibraryGames(HWND hwnd) {
    HWND hwndList;
    char szLabel[320], szSelected[256] = "";
    int16_t fLibrary;
    int16_t fEnumerated;
    int16_t fHasGames;
    int16_t fContinue;
    LRESULT iContinue, iSelected, iPlayer, iItem;
    hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAMES);
    if (!hwndList) return;
    iSelected = ControlIO().pfnLUiSendMessage(hwndList, LB_GETCURSEL, 0, 0);
    if (iSelected != LB_ERR) ControlIO().pfnLUiSendMessage(hwndList, LB_GETTEXT, iSelected, (LPARAM)szSelected);
    iPlayer = LUiSendDlgItemMessage(hwnd, IDC_LIBRARY_PLAYERS, CB_GETCURSEL, 0, 0);
    if (iPlayer != CB_ERR) iPlayer = LUiSendDlgItemMessage(hwnd, IDC_LIBRARY_PLAYERS, CB_GETITEMDATA, iPlayer, 0);
    ControlIO().pfnLUiSendMessage(hwndList, LB_RESETCONTENT, 0, 0);
    ClearLibraryGameList();
    fLibrary = FInitLibrary();
    if (fLibrary) {
        fEnumerated = FNativeEnumerateDirectory(szLibraryRoot, ListLibraryGame, hwndList);
        (void)fEnumerated;
    }
    hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAMES);
    if (hwndList) {
        iSelected = szSelected[0] ? ControlIO().pfnLUiSendMessage(hwndList, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)szSelected) : LB_ERR;
        if (iSelected == LB_ERR) iSelected = 0;
        else if (iPlayer >= 0 && iPlayer < 16) {
            iItem = ControlIO().pfnLUiSendMessage(hwndList, LB_GETITEMDATA, iSelected, 0);
            if (iItem >= 0 && iItem < cLibraryGameList && (rgLibraryGameList[iItem].grfPlayers & (1u << iPlayer)))
                rgLibraryGameList[iItem].iplr = (int)iPlayer;
        }
        ControlIO().pfnLUiSendMessage(hwndList, LB_SETCURSEL, iSelected, 0);
    }
    fHasGames = hwndList && ControlIO().pfnLUiSendMessage(hwndList, LB_GETCOUNT, 0, 0) > 0;
    if (!fHostLibraryView) {
        ControlIO().pfnFUiEnableWindow(rghwndBtnSplash[1], fHasGames);
        ControlIO().pfnFUiEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAME_EXPORT), fHasGames);
        ControlIO().pfnFUiEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_DUPLICATE), fHasGames);
        ControlIO().pfnFUiEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RENAME), fHasGames);
        ControlIO().pfnFUiEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAME_DELETE), fHasGames);
    }
    RefreshLibraryPlayers(hwnd);
    fContinue = FFindContinueLibraryGame() && continueLibrary.iPlayer >= 0;
    if (fContinue && !continueLibrary.fTurnAvailable) {
        iContinue = ControlIO().pfnLUiSendMessage(hwndList, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)continueLibrary.szName);
        fContinue = iContinue != LB_ERR;
    }
    if (fContinue) {
        CchSnprintf(szLabel, sizeof(szLabel), continueLibrary.fTurnAvailable ?
                    TextIO().pfnCompressed(idsLibraryContinuePlayer) : TextIO().pfnCompressed(idsLibraryContinueChoosePlayer),
                    continueLibrary.szName, continueLibrary.iPlayer + 1, continueLibrary.iYear);
        SetDlgItemText(hwnd, IDC_LIBRARY_CONTINUE_DETAILS, szLabel);
        ControlIO().pfnFUiSetWindowText(rghwndBtnSplash[2], TextIO().pfnCompressed(idsContinueGame));
        ControlIO().pfnFUiEnableWindow(rghwndBtnSplash[2], TRUE);
    } else {
        SetDlgItemText(hwnd, IDC_LIBRARY_CONTINUE_DETAILS, "");
        ControlIO().pfnFUiSetWindowText(rghwndBtnSplash[2], TextIO().pfnCompressed(static_cast<StringId>(idsNewGame + 2)));
        ControlIO().pfnFUiEnableWindow(rghwndBtnSplash[2], FALSE);
    }
}

void RefreshLibraryHostGames(HWND hwnd) {
    HWND hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_HOST_GAMES);
    int16_t fHasGames;
    if (!hwndList) return;
    ControlIO().pfnLUiSendMessage(hwndList, LB_RESETCONTENT, 0, 0);
    if (FInitLibrary()) FNativeEnumerateDirectory(szLibraryRoot, ListLibraryHostGame, hwndList);
    ControlIO().pfnLUiSendMessage(hwndList, LB_SETCURSEL, 0, 0);
    fHasGames = ControlIO().pfnLUiSendMessage(hwndList, LB_GETCOUNT, 0, 0) > 0;
    if (fHostLibraryView) {
        ControlIO().pfnFUiEnableWindow(rghwndBtnSplash[1], fHasGames);
        ControlIO().pfnFUiEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAME_EXPORT), fHasGames);
        ControlIO().pfnFUiEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_DUPLICATE), fHasGames);
        ControlIO().pfnFUiEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RENAME), fHasGames);
        ControlIO().pfnFUiEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAME_DELETE), fHasGames);
    }
}

void SelectLibraryView(HWND hwnd, int16_t fHost) {
    HWND hwndGames = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAMES);
    HWND hwndHost = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_HOST_GAMES);
    fHostLibraryView = fHost;
    FUiShowWindow(hwndGames, fHost ? SW_HIDE : SW_SHOW);
    FUiShowWindow(hwndHost, fHost ? SW_SHOW : SW_HIDE);
    FUiShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_PLAYERS), fHost ? SW_HIDE : SW_SHOW);
    FUiShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_PLAYER_LABEL), fHost ? SW_HIDE : SW_SHOW);
    FUiShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_IMPORT), fHost ? SW_HIDE : SW_SHOW);
    FUiShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_IMPORT_FOLDER), fHost ? SW_HIDE : SW_SHOW);
    FUiShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_IMPORT_ORDERS), fHost ? SW_SHOW : SW_HIDE);
    if (fHost) RefreshLibraryHostGames(hwnd);
    else RefreshLibraryGames(hwnd);
}

int16_t FContinueLibraryGame(HWND hwnd) {
    HWND hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAMES);
    char szFile[512];
    LRESULT i;
    int32_t lidGame;
    uint16_t turn;
    int16_t fMulti;

    if (!FFindContinueLibraryGame()) return FALSE;
    CchSnprintf(szFile, sizeof(szFile), "%s%s%s%s%s.m%d", szLibraryRoot, szDirSep,
                continueLibrary.szName, szDirSep, continueLibrary.szName, continueLibrary.iPlayer + 1);
    if (FInspectImportFile(szFile, dtTurn, continueLibrary.iPlayer, &lidGame, &turn, NULL, &fMulti)) {
        strcpy(szBase, szFile);
        ini.fStartupFile = TRUE;
        return TRUE;
    }

    // The remembered turn may have been removed after a newer game import.
    // Require an explicit choice when the remembered player's turn is missing.
    SelectLibraryView(hwnd, FALSE);
    i = ControlIO().pfnLUiSendMessage(hwndList, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)continueLibrary.szName);
    if (i == LB_ERR) return FALSE;
    ControlIO().pfnLUiSendMessage(hwndList, LB_SETCURSEL, i, 0);
    RefreshLibraryPlayers(hwnd);
    LUiSendDlgItemMessage(hwnd, IDC_LIBRARY_PLAYERS, CB_SETCURSEL, (WPARAM)-1, 0);
    ControlIO().pfnFUiEnableWindow(rghwndBtnSplash[1], FALSE);
    ControlIO().pfnHwndUiSetFocus(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_PLAYERS));
    return FALSE;
}

int16_t FSelectLibraryGame(HWND hwnd) {
    HWND hwndList;
    LRESULT i, iItem, iPlayer;

    if (fHostLibraryView) return FSelectLibraryHostGame(hwnd);

    hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAMES);
    i = ControlIO().pfnLUiSendMessage(hwndList, LB_GETCURSEL, 0, 0);
    if (i == LB_ERR) return FALSE;
    iItem = ControlIO().pfnLUiSendMessage(hwndList, LB_GETITEMDATA, i, 0);
    if (iItem < 0 || iItem >= cLibraryGameList) return FALSE;
    iPlayer = LUiSendDlgItemMessage(hwnd, IDC_LIBRARY_PLAYERS, CB_GETCURSEL, 0, 0);
    if (iPlayer == CB_ERR) return FALSE;
    iPlayer = LUiSendDlgItemMessage(hwnd, IDC_LIBRARY_PLAYERS, CB_GETITEMDATA, iPlayer, 0);
    if (iPlayer < 0 || iPlayer >= 16 || !(rgLibraryGameList[iItem].grfPlayers & (1u << iPlayer))) return FALSE;
    CchSnprintf(szBase, sizeof(szBase), "%s%s%s%s%s.m%d", szLibraryRoot, szDirSep,
                rgLibraryGameList[iItem].szName, szDirSep, rgLibraryGameList[iItem].szName, (int)iPlayer + 1);
    ini.fStartupFile = TRUE;
    return TRUE;
}

int16_t FSelectLibraryHostGame(HWND hwnd) {
    HWND hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_HOST_GAMES);
    LRESULT i = ControlIO().pfnLUiSendMessage(hwndList, LB_GETCURSEL, 0, 0);
    char szName[256];
    int cch;
    if (i == LB_ERR) return FALSE;
    ControlIO().pfnLUiSendMessage(hwndList, LB_GETTEXT, i, (LPARAM)szName);
    cch = CchSnprintf(szBase, sizeof(szBase), "%s%s%s%s%s.hst", szLibraryRoot, szDirSep,
                      szName, szDirSep, szName);
    if (cch <= 0 || (size_t)cch >= sizeof(szBase)) return FALSE;
    ini.fStartupFile = TRUE;
    return TRUE;
}

static int16_t FSelectedLibraryGameName(HWND hwnd, char *pszName, size_t cchName) {
    HWND hwndList = ControlIO().pfnGetDlgItem(hwnd, fHostLibraryView ? IDC_LIBRARY_HOST_GAMES : IDC_LIBRARY_GAMES);
    LRESULT i, iItem;
    int cch;
    i = ControlIO().pfnLUiSendMessage(hwndList, LB_GETCURSEL, 0, 0);
    if (i == LB_ERR) return FALSE;
    if (fHostLibraryView) {
        cch = (int)ControlIO().pfnLUiSendMessage(hwndList, LB_GETTEXTLEN, i, 0);
        if (cch <= 0 || (size_t)cch >= cchName) return FALSE;
        return ControlIO().pfnLUiSendMessage(hwndList, LB_GETTEXT, i, (LPARAM)pszName) == cch;
    }
    iItem = ControlIO().pfnLUiSendMessage(hwndList, LB_GETITEMDATA, i, 0);
    if (iItem < 0 || iItem >= cLibraryGameList) return FALSE;
    cch = CchSnprintf(pszName, cchName, "%s", rgLibraryGameList[iItem].szName);
    return cch > 0 && (size_t)cch < cchName;
}

static int16_t FChooseLibraryGameActionName(HWND hwnd, const char *pszCurrentName, char *pszNewName) {
    int cch;
    cch = CchSnprintf(szLibraryName, sizeof(szLibraryName), "%s", pszCurrentName);
    if (cch <= 0 || (size_t)cch >= sizeof(szLibraryName)) return FALSE;
    SuggestLibraryName(szLibraryName, FALSE);
    if (!szLibraryName[0] || ControlIO().pfnLUiDialogBox(hInst, MAKEINTRESOURCE(IDD_LIBRARY_NAME), hwnd,
                                           LibraryNameDlg, (LPARAM)szLibraryName) != TRUE) return FALSE;
    cch = CchSnprintf(pszNewName, 256, "%s", szLibraryName);
    return cch > 0 && cch < 256;
}

int16_t FActOnSelectedLibraryGame(HWND hwnd, int16_t fRename) {
    char szName[256], szNewName[256];
    if (!FSelectedLibraryGameName(hwnd, szName, sizeof(szName)) ||
        !FChooseLibraryGameActionName(hwnd, szName, szNewName)) return FALSE;
    return fRename ? FRenameLibraryGame(hwnd, szName, szNewName) :
                     FDuplicateLibraryGame(hwnd, szName, szNewName);
}

void RememberLibraryGame(void) {
    char szDir[256], szMetadataName[256];
    char *pch;
    int iplr;
    long long lPlayed;

    if (!FInitLibrary() || !LibraryPathIO().pfnUnder(szLibraryRoot, szBase)) return;
    strcpy(szDir, szBase);
    pch = PchLibrarySeparator(szDir);
    if (!pch) return;
    *pch = 0;
    pch = PchLibrarySeparator(szDir);
    if (!pch) return;
    szLibraryExchangeName[0] = 0;
    /* szDir already names the game folder; retain its exchange basename before
     * publishing the newly opened player and last-played time. */
    if (!FReadLibraryMetadata(szDir, szMetadataName, sizeof(szMetadataName), &iplr, &lPlayed,
                              szLibraryExchangeName, sizeof(szLibraryExchangeName))) szLibraryExchangeName[0] = 0;
    if (FWriteLibraryMetadata(szDir, pch + 1, idPlayer))
        ProfileTextIO().pfnWriteString("Library", "ContinueDeleted", NULL, TextIO().pfnCompressed(idsStarsIni));
}

void CloseLibraryUiSession() noexcept {
    ClearLibraryGameList();
    szLibraryRoot[0] = szLibraryPending[0] = szLibraryName[0] = szLibraryExchangeName[0] = 0;
    szRaceLibraryRoot[0] = szRaceLibraryPath[0] = szRaceLibraryName[0] = szRaceLibraryTemp[0] = 0;
    fRaceLibraryNameDialog = fNewLibraryRace = fHostLibraryView = FALSE;
    pszRaceLibraryNameOverride = nullptr;
    continueLibrary = LIBRARYCONTINUE{};
}

int16_t FPublishSubsequentTurn(const LIBRARYPATHS &paths, HWND hwnd, const char *pszSource,
                                  const char *szTarget, const char *pszName, int iplr,
                                  int32_t lidGame, uint16_t turn, int16_t fHasTurn) {
    char szOrders[512],szStage[512],szPrevious[512],szMessage[1024];
    int cch;
    int32_t lidCurrent;
    uint16_t turnCurrent;
    int16_t fMultiCurrent,fHasOrders;
    int64_t cbFile;
    uint32_t dwTick;
    cch = LibraryPublicationIO().pfnCchSnprintf(szOrders, sizeof(szOrders), "%s%s%s%s%s.x%d", paths.szLibraryRoot, szDirSep, pszName, szDirSep, pszName, iplr + 1);
    if (cch <= 0 || (size_t)cch >= sizeof(szOrders)) return FALSE;
    dwTick = LibraryPublicationIO().pfnDwTickCount();
    szStage[0] = 0;
    cch = LibraryPublicationIO().pfnCchSnprintf(szStage, sizeof(szStage), "%s.importing.%u", szTarget, dwTick);
    if (cch <= 0 || (size_t)cch >= sizeof(szStage)) return FALSE;
    cch = LibraryPublicationIO().pfnCchSnprintf(szPrevious, sizeof(szPrevious), "%s.previous.%u", szTarget, dwTick);
    if (cch <= 0 || (size_t)cch >= sizeof(szPrevious)) return FALSE;
    if (!LibraryPublicationIO().pfnFCopyNativeFileExclusive(pszSource, szStage) ||
        !LibraryPublicationIO().pfnFInspectImportFile(szStage, dtTurn, iplr, &lidCurrent, &turnCurrent, NULL, &fMultiCurrent) ||
        lidCurrent != lidGame || turnCurrent != turn) {
        if (szStage[0]) RemoveNativeFile(szStage);
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryTurnStageFailed), MB_ICONHAND);
        return FALSE;
    }
    if (!fHasTurn) {
        if (ReplaceNativeFile(szStage, szTarget) != 0) {
            RemoveNativeFile(szStage);
            AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryTurnInstallFailed), MB_ICONHAND);
            return FALSE;
        }
        /* No previous turn was advanced, so retain any host orders/history. */
        return TRUE;
    }
    fHasOrders = LibraryPublicationIO().pfnFNativeFileSize(szOrders, &cbFile);
    if (!LibraryPublicationIO().pfnFCopyNativeFileExclusive(szTarget, szPrevious)) {
        RemoveNativeFile(szStage);
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryTurnPreserveFailed), MB_ICONHAND);
        return FALSE;
    }
    if (ReplaceNativeFile(szStage, szTarget) != 0) {
        RemoveNativeFile(szStage);
        RemoveNativeFile(szPrevious);
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryTurnInstallFailed), MB_ICONHAND);
        return FALSE;
    }
    if (fHasOrders && RemoveNativeFile(szOrders) != 0) {
        if (ReplaceNativeFile(szPrevious, szTarget) != 0) {
            LibraryPublicationIO().pfnCchSnprintf(szMessage, sizeof(szMessage),
                        TextIO().pfnCompressed(idsLibraryTurnRestoreFailed),
                        szPrevious);
            AlertIO().pfnAlertSz(szMessage, MB_ICONHAND);
            return FALSE;
        }
        RemoveNativeFile(szPrevious);
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryTurnOrdersRetireFailed), MB_ICONHAND);
        return FALSE;
    }
    RemoveNativeFile(szPrevious);
    (void)hwnd;
    return TRUE;
}

int16_t FPublishImportedLibraryGame(const LIBRARYPATHS &paths, LIBRARYIMPORT &li, const char *szDir) {
    char szStage[256];
    LibraryPublicationIO().pfnCchSnprintf(szStage, sizeof(szStage), "%s%s.importing.%lld", paths.szLibraryRoot, szDirSep, (long long)LibraryPublicationIO().pfnLNativeTime());
    if (LibraryPublicationIO().pfnCreateNativeDir(szStage) != 0) {
        RemoveNativePath(paths.szLibraryPending);
        paths.szLibraryPending[0] = 0;
        return FALSE;
    }
    li.pszTarget = szStage;
    li.pszName = paths.szLibraryName;
    li.fCopy = TRUE;
    if (!LibraryPublicationIO().pfnFNativeEnumerateDirectory(szDir, CopyImportEntry, &li) || li.fBad ||
        !LibraryPublicationIO().pfnFNativeEnumerateDirectory(szDir, CopyBackupDirectory, &li) || li.fBad ||
        !LibraryPublicationIO().pfnFWriteLibraryMetadata(szStage, paths.szLibraryName,
                               li.grfPlayers ? (int16_t)__builtin_ctz(li.grfPlayers) : iplrNone) ||
        RemoveNativePath(paths.szLibraryPending) != 0 || RenameNativeFile(szStage, paths.szLibraryPending) != 0) {
        LibraryPublicationIO().pfnFNativeEnumerateDirectory(szStage, RemoveImportFile, szStage);
        RemoveNativePath(szStage);
        RemoveNativePath(paths.szLibraryPending);
        paths.szLibraryPending[0] = 0;
        AlertIO().pfnAlertSz(TextIO().pfnCompressed(idsLibraryGameImportFailed), MB_ICONHAND);
        return FALSE;
    }
    paths.szLibraryPending[0] = 0;
    return TRUE;
}
