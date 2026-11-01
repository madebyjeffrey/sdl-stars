#include "win.h"
#include <algorithm>
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
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
#include <cerrno>
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
struct LIBRARYGAMELISTITEM {
    char szName[256];
    int iplr;
    std::uint16_t grfPlayers;
    std::uint16_t rgturn[16];
};
static NATIVEARRAYOWNER<LIBRARYGAMELISTITEM, SDL_free> rgLibraryGameList;
static int cLibraryGameList;
static int cLibraryGameListMax;
static std::uint32_t idRaceStage;
static std::int16_t fRaceLibraryNameDialog;
static std::int16_t fNewLibraryRace;
static std::uint32_t idLibraryExportStage;
static std::uint32_t idLibraryZipStage;
static std::int16_t fHostLibraryView;
static const char *pszRaceLibraryNameOverride;

struct LibraryContinue {
    char szName[256];
    int iPlayer;
    int iYear;
    long long lPlayed;
    std::int16_t fFound;
    std::int16_t fTurnAvailable;
};
using LIBRARYCONTINUE = LibraryContinue;

static LIBRARYCONTINUE continueLibrary;
static std::int16_t FFindContinueLibraryGame(APPSESSION &app);
static std::int16_t FCopyNativeFile(const char *pszSource, const char *pszTarget);
static std::int16_t FSelectedLibraryGameName(HWND hwnd, char *pszName, std::size_t cchName);
static void *PvLibraryFolderContext(HWND hwnd) {
    const std::uint64_t id = StarsCompatWindowId(hwnd);
    // SDL carries one pointer-sized opaque value; no window pointer outlives UI.
    return id <= UINTPTR_MAX ? reinterpret_cast<void *>(static_cast<std::uintptr_t>(id)) : nullptr;
}

static void SDLCALL LibraryExportFolderSelected(void *lpData, const char * const *rgpszPath, int iFilter) noexcept;
static std::int16_t FWriteLibraryMetadataValues(const char *pszDir, const char *pszName, int iplr,
                                           long long lPlayed, const char *pszExchangeName);

struct RACESTATE {
    PLAYER plr;
    char szPass[sizeof(APPGAMESTATE::szRacePass)];
    char szFile[sizeof(APPGAMESTATE::szRaceFile)];
    char szPath[sizeof(szRaceLibraryPath)];
    std::int16_t fNew;
};

struct LIBRARYNAMECHECK {
    const char *pszDirectory;
    const char *pszName;
    std::int16_t fFound;
};

static void SaveRaceState(APPSESSION &app, RACESTATE *prs) {
    prs->plr = app.state.vplr;
    prs->fNew = fNewLibraryRace;
    std::memcpy(prs->szPass, app.state.szRacePass, sizeof(prs->szPass));
    std::memcpy(prs->szFile, app.state.szRaceFile, sizeof(prs->szFile));
    std::memcpy(prs->szPath, szRaceLibraryPath, sizeof(prs->szPath));
}

static void RestoreRaceState(APPSESSION &app, const RACESTATE *prs, std::int16_t fRestoreRace, std::int16_t fRestorePath) {
    if (fRestoreRace) {
        app.state.vplr = prs->plr;
        std::memcpy(app.state.szRacePass, prs->szPass, sizeof(prs->szPass));
        std::memcpy(app.state.szRaceFile, prs->szFile, sizeof(prs->szFile));
    }
    if (fRestorePath) {
        std::memcpy(szRaceLibraryPath, prs->szPath, sizeof(prs->szPath));
        fNewLibraryRace = prs->fNew;
    }
}

static std::int16_t FindLibraryName(APPSESSION &, void *lpData, const char *pszFile) {
    LIBRARYNAMECHECK *plnc = static_cast<LIBRARYNAMECHECK *>(lpData);
    if (!SDL_strcasecmp(plnc->pszName, pszFile)) {
        plnc->fFound = 1;
        return 0;
    }
    return 1;
}

std::int16_t FLibraryNameAvailable(APPSESSION &app, const char *pszDirectory, const char *pszName) {
    LIBRARYNAMECHECK lnc;
    lnc.pszDirectory = pszDirectory;
    lnc.pszName = pszName;
    lnc.fFound = 0;
    if (!FNativeEnumerateDirectory(app, lnc.pszDirectory, FindLibraryName, &lnc)) return 0;
    return !lnc.fFound;
}

static std::int16_t FRaceLibraryNameAvailable(APPSESSION &app, const char *pszName) {
    return FLibraryNameAvailable(app, szRaceLibraryRoot, pszName);
}

static std::int16_t FPortableRaceFilename(APPSESSION &app, const char *pszName) {
    const unsigned char *pch;
    char szDevice[256];
    std::size_t cch = std::strlen(pszName);
    std::size_t i;
    const char *pchExt = std::strrchr(pszName, '.');
    const char *pchDigit;
    if (!pchExt || pchExt == pszName || SDL_tolower(pchExt[1]) != 'r' || !pchExt[2] ||
        pchExt[2] == '0' || pchExt[2] < '1' || pchExt[2] > '9' || cch >= 240 || pszName[0] == ' ' || pszName[0] == '.' ||
        pszName[cch - 1] == ' ' || pszName[cch - 1] == '.') return 0;
    /* Windows reserves these stems even when followed by an extension. */
    for (i = 0; i < cch && pszName[i] != '.'; i++) szDevice[i] = pszName[i];
    szDevice[i] = 0;
    if (!SDL_strcasecmp(szDevice, "CON") || !SDL_strcasecmp(szDevice, "PRN") ||
        !SDL_strcasecmp(szDevice, "AUX") || !SDL_strcasecmp(szDevice, "NUL") ||
        !SDL_strcasecmp(szDevice, "CLOCK$") ||
        (i == 4 && (!SDL_strncasecmp(szDevice, "COM", 3) || !SDL_strncasecmp(szDevice, "LPT", 3)) &&
         szDevice[3] >= '1' && szDevice[3] <= '9')) return 0;
    for (pchDigit = pchExt + 3; *pchDigit; pchDigit++)
        if (*pchDigit < '0' || *pchDigit > '9') return 0;
    if (std::strlen(pchExt + 2) > 3) return 0;
    for (pch = reinterpret_cast<const unsigned char *>(pszName); *pch; pch++)
        if (*pch < 32 || *pch == 127 || std::strchr("<>:\"/\\|?*", *pch)) return 0;
    if (std::strlen(szRaceLibraryRoot) + cch + 2 >= sizeof(app.state.szBase)) return 0;
    return 1;
}

/* The Windows CRT environment uses its code page; SDL supplies UTF-8 paths. */
static std::int16_t FInitRaceLibrary(void) {
    const char *pszOverride = SDL_getenv("STARS_LIBRARY_DIR");
    const char *pszSettings = SDL_getenv("STARS_SETTINGS_DIR");
    char *pszRoot = pszOverride || pszSettings ? nullptr : CompatPathIO().pfnGetPrefPath("", "Stars!");
    int cch;
    NATIVEOWNER<char, SDL_free> rootPath(pszRoot);
    if (!pszOverride && !pszSettings && !pszRoot) return 0;
    cch = CchSnprintf(std::span{szRaceLibraryRoot, sizeof(szRaceLibraryRoot)}, "%s%sRace Designs",
                      pszOverride ? pszOverride : pszSettings ? pszSettings : pszRoot,
                      pszOverride || pszSettings ? szDirSep : "");
    rootPath.reset();
    return cch > 0 && cch < static_cast<int>(sizeof(szRaceLibraryRoot)) && FEnsureNativeDir(szRaceLibraryRoot);
}

static std::int16_t FCreateRaceStagePath(char *pszDirectory, std::size_t cchDirectory,
                                    char *pszFile, std::size_t cchFile) {
    std::uint32_t dwTick = DwTickCount();
    int i;
    int cch;
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(std::span{pszDirectory, cchDirectory}, "%s%s.stars-race-stage-%u-%u",
                          szRaceLibraryRoot, szDirSep, dwTick, idRaceStage++);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= cchDirectory) return 0;
        if (CreateNativeDir(pszDirectory) != 0) {
            if (FNativeDirectoryExists(pszDirectory)) continue;
            return 0;
        }
        cch = CchSnprintf(std::span{pszFile, cchFile}, "%s%sdesign.r1", pszDirectory, szDirSep);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= cchFile) {
            RemoveNativePath(pszDirectory);
            return 0;
        }
        return 1;
    }
    return 0;
}

static void SuggestRaceFilename(APPSESSION &app, std::span<char, 256> pszName) {
    char szStem[240];
    const char *pch = std::strrchr(pszName.data(), '.');
    std::size_t cchStem = pch ? static_cast<std::size_t>((pch - pszName.data())) : std::strlen(pszName.data());
    int i;
    if (cchStem >= sizeof(szStem)) cchStem = sizeof(szStem) - 1;
    cchStem = CchUtf8Prefix(pszName.data(), cchStem);
    std::memcpy(szStem, pszName.data(), cchStem);
    szStem[cchStem] = 0;
    for (i = 1; i < 1000; i++) {
        CchSnprintf(pszName, "%s.r%d", szStem, i);
        if (FRaceLibraryNameAvailable(app, pszName.data())) return;
    }
    pszName[0] = 0;
}

static std::int16_t FFormatLibraryFamilyPath(char *pszPath, std::size_t cchPath, const char *pszDirectory,
                                        const char *pszName, DtFileType dt, int iplr) {
    const char *pszExtension;
    int cch;
    if (dt == dtXY) pszExtension = "xy";
    else if (dt == dtHost) pszExtension = "hst";
    else pszExtension = dt == dtTurn ? "m" : dt == dtHist ? "h" : "x";
    if (dt == dtXY || dt == dtHost)
        cch = CchSnprintf(std::span{pszPath, cchPath}, "%s%s%s.%s", pszDirectory, szDirSep, pszName, pszExtension);
    else cch = CchSnprintf(std::span{pszPath, cchPath}, "%s%s%s.%s%d", pszDirectory, szDirSep, pszName, pszExtension, iplr + 1);
    return cch > 0 && static_cast<std::size_t>(cch) < cchPath;
}

static std::int16_t FInitLibrary(void) {
    const char *pszOverride;
    const char *pszSettings;
    char *pszRoot;
    int cch;

    pszOverride = SDL_getenv("STARS_LIBRARY_DIR");
    pszSettings = SDL_getenv("STARS_SETTINGS_DIR");
    pszRoot = pszOverride || pszSettings ? nullptr : CompatPathIO().pfnGetPrefPath("", "Stars!");
    NATIVEOWNER<char, SDL_free> rootPath(pszRoot);
    if (!pszOverride && !pszSettings && !pszRoot) return 0;
    cch = CchSnprintf(std::span{szLibraryRoot, sizeof(szLibraryRoot)}, "%s%sGames",
                      pszOverride ? pszOverride : pszSettings ? pszSettings : pszRoot,
                      pszOverride || pszSettings ? szDirSep : "");
    rootPath.reset();
    return cch > 0 && cch < static_cast<int>(sizeof(szLibraryRoot)) && FEnsureNativeDir(szLibraryRoot);
}

std::int16_t FPathUnderLibrary(const char *pszRoot, const char *pszFile) {
    std::size_t cchRoot, cchFile;
#ifdef _WIN32
    std::size_t ich;
#endif
    if (!pszRoot || !pszFile) return 0;
    cchRoot = std::strlen(pszRoot);
    cchFile = std::strlen(pszFile);
    if (cchRoot == 0 || cchFile <= cchRoot) return 0;
#ifdef _WIN32
    /* Pickers return '/', while library joins and Windows arguments may use '\\'. */
    for (ich = 0; ich < cchRoot; ich++) {
        if ((pszRoot[ich] == '/' || pszRoot[ich] == '\\') &&
            (pszFile[ich] == '/' || pszFile[ich] == '\\')) continue;
        if (SDL_tolower(static_cast<unsigned char>(pszRoot[ich])) != SDL_tolower(static_cast<unsigned char>(pszFile[ich]))) return 0;
    }
    return pszFile[cchRoot] == '/' || pszFile[cchRoot] == '\\';
#else
    return std::memcmp(pszRoot, pszFile, cchRoot) == 0 && pszFile[cchRoot] == '/';
#endif
}

static char *PchLibrarySeparator(char *pszPath) {
    char *pch = std::strrchr(pszPath, chDirSep);
#ifdef _WIN32
    char *pchSlash = std::strrchr(pszPath, '/');
    if (pchSlash && (!pch || pchSlash > pch)) pch = pchSlash;
#endif
    return pch;
}

std::int16_t FIsManagedLibraryGameFile(const char *pszFile) {
    return FInitLibrary() && LibraryPathIO().pfnUnder(szLibraryRoot, pszFile);
}

std::int16_t FIsManagedLibraryRaceFile(const char *pszFile) {
    return FInitRaceLibrary() && LibraryPathIO().pfnUnder(szRaceLibraryRoot, pszFile);
}

static std::int16_t FPortableLibraryName(APPSESSION &app, const char *pszName) {
    const unsigned char *pch;
    char szDevice[256];
    std::size_t cch, i;

    cch = std::strlen(pszName);
    if (cch == 0 || cch >= sizeof(szDevice) || pszName[0] == ' ' || pszName[0] == '.' || pszName[cch - 1] == ' ' || pszName[cch - 1] == '.') return 0;
    for (pch = reinterpret_cast<const unsigned char *>(pszName); *pch; pch++)
        if (*pch < 32 || *pch == 127 || std::strchr("<>:\"/\\|?*", *pch)) return 0;
    /* Windows reserves these stems even when followed by an extension. */
    for (i = 0; i < cch && pszName[i] != '.'; i++) szDevice[i] = pszName[i];
    szDevice[i] = 0;
    if (!SDL_strcasecmp(szDevice, "CON") || !SDL_strcasecmp(szDevice, "PRN") ||
        !SDL_strcasecmp(szDevice, "AUX") || !SDL_strcasecmp(szDevice, "NUL") ||
        !SDL_strcasecmp(szDevice, "CLOCK$") ||
        (i == 4 && (!SDL_strncasecmp(szDevice, "COM", 3) || !SDL_strncasecmp(szDevice, "LPT", 3)) &&
         szDevice[3] >= '1' && szDevice[3] <= '9')) return 0;
    /* Longest backup path: root/name/backup.999/name.m16 plus NUL. */
    return std::strlen(szLibraryRoot) + 2 * cch + 18 < sizeof(app.state.szBase);
}

static void SuggestLibraryName(APPSESSION &app, std::span<char, 256> pszName, std::int16_t fRevision) {
    char szOriginal[256];
    char szSuffix[32];
    int i, cch, cchMax;

    CchCopyUtf8(std::span{szOriginal}, pszName.data());
    cchMax = (static_cast<int>(sizeof(app.state.szBase)) - static_cast<int>(std::strlen(szLibraryRoot)) - 19) / 2;
    for (i = 2; i < 10000 && !FLibraryNameAvailable(app, szLibraryRoot, pszName.data()); i++) {
        CchSnprintf(std::span{szSuffix, sizeof(szSuffix)}, fRevision ? TextIO().pfnCompressed(app, idsLibraryGameRevisionSuffix).data() : " (%d)", i);
        cch = (((static_cast<int>(std::strlen(szOriginal))) < ((((0) > (cchMax - static_cast<int>(std::strlen(szSuffix)))) ? (0) : (cchMax - static_cast<int>(std::strlen(szSuffix)))))) ? (static_cast<int>(std::strlen(szOriginal))) : ((((0) > (cchMax - static_cast<int>(std::strlen(szSuffix)))) ? (0) : (cchMax - static_cast<int>(std::strlen(szSuffix))))));
        /* A shortened suggestion must not split a UTF-8 character. */
        while (cch > 0 && (static_cast<unsigned char>(szOriginal[cch]) & 0xc0) == 0x80) cch--;
        CchSnprintf(pszName, "%.*s%s", cch, szOriginal, szSuffix);
    }
}

static void GetGameLibraryName(APPSESSION &app, const char *pszName, std::span<char, 256> pszNameOut) {
    std::array<char, cchCatalogText> szCatalogPszName{};
    std::size_t i, cch;
    int cchMax = (static_cast<int>(sizeof(app.state.szBase)) - static_cast<int>(std::strlen(szLibraryRoot)) - 19) / 2;

    CchCopyUtf8(pszNameOut, std::string_view(pszName, CchUtf8Prefix(pszName, cchMax)));
    for (i = 0; pszNameOut[i]; i++)
        if (static_cast<unsigned char>(pszNameOut[i]) < 32 || pszNameOut[i] == 127 || std::strchr("<>:\"/\\|?*", pszNameOut[i])) pszNameOut[i] = '_';
    cch = std::strlen(pszNameOut.data());
    while (cch != 0 && (pszNameOut[cch - 1] == ' ' || pszNameOut[cch - 1] == '.')) pszNameOut[--cch] = 0;
    while (pszNameOut[0] == ' ' || pszNameOut[0] == '.') std::memmove(pszNameOut.data(), pszNameOut.data() + 1, std::strlen(pszNameOut.data()));
    // The game loader treats the last period as a file extension.
    for (i = 0; pszNameOut[i]; i++) if (pszNameOut[i] == '.') pszNameOut[i] = '_';
    if (!pszNameOut[0]) {
        szCatalogPszName = TextIO().pfnCompressed(app, idsLibraryDefaultGameName);
        pszName = szCatalogPszName.data();
        CchCopyUtf8(pszNameOut, std::string_view(pszName, CchUtf8Prefix(pszName, cchMax)));
    }
    if (!FPortableLibraryName(app, pszNameOut.data())) {
        // Reserved device names need a prefix; leave the stored game title intact.
        pszNameOut[CchUtf8Prefix(pszNameOut.data(), StarsMax(0, cchMax - 1))] = 0;
        std::memmove(pszNameOut.data() + 1, pszNameOut.data(), std::strlen(pszNameOut.data()) + 1);
        pszNameOut[0] = '_';
    }
}

INT_PTR  LibraryGameRevisionDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());
    char szMessage[512];
    POINT16 pt = {-1, -1};

    switch (message) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnSetWindowText(hwnd, TextIO().pfnCompressed(app, idsLibraryGameDuplicateTitle).data());
        CchSnprintf(std::span{szMessage, sizeof(szMessage)}, TextIO().pfnCompressed(app, idsLibraryGameDuplicatePrompt).data(), reinterpret_cast<const char *>(lParam));
        win32::SetDlgItemTextA(hwnd, IDC_LIBRARY_GAME_REVISION_TEXT, szMessage);
        win32::SetDlgItemTextA(hwnd, IDOK, TextIO().pfnCompressed(app, idsLibraryGameAddRevision).data());
        win32::SetDlgItemTextA(hwnd, IDCANCEL, TextIO().pfnCompressed(app, idsLibraryGameBackToWizard).data());
        ControlIO().pfnStickyDlgPos(hwnd, &pt, 1);
        return 1;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        }
        break;
    }
    return 0;
}

INT_PTR  LibraryNameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());
    std::span<char, 256> *pszName;
    char szName[256];
    std::array<char, cchCatalogText> szMessage{};
    StringId ids;
    RECT rc, rcOwner, rcWork;
    int x, y;

    switch (message) {
    case win32::WM_INITDIALOG:
        if (!win32::SystemParametersInfoA(win32::SPI_GETWORKAREA, 0, &rcWork, 0))
            win32::SetRect(&rcWork, 0, 0, win32::GetSystemMetrics(win32::SM_CXSCREEN), win32::GetSystemMetrics(win32::SM_CYSCREEN));
        if (!win32::IsWindowVisible(win32::GetParent(hwnd)) || !win32::GetWindowRect(win32::GetParent(hwnd), &rcOwner)) rcOwner = rcWork;
        win32::GetWindowRect(hwnd, &rc);
        x = rcOwner.left + ((rcOwner.right - rcOwner.left) - (rc.right - rc.left)) / 2;
        y = rcOwner.top + ((rcOwner.bottom - rcOwner.top) - (rc.bottom - rc.top)) / 2;
        x = (((rcWork.left) > ((((x) < (rcWork.right - (rc.right - rc.left))) ? (x) : (rcWork.right - (rc.right - rc.left))))) ? (rcWork.left) : ((((x) < (rcWork.right - (rc.right - rc.left))) ? (x) : (rcWork.right - (rc.right - rc.left)))));
        y = (((rcWork.top) > ((((y) < (rcWork.bottom - (rc.bottom - rc.top))) ? (y) : (rcWork.bottom - (rc.bottom - rc.top))))) ? (rcWork.top) : ((((y) < (rcWork.bottom - (rc.bottom - rc.top))) ? (y) : (rcWork.bottom - (rc.bottom - rc.top)))));
        ControlIO().pfnSetWindowPos(hwnd, nullptr, x, y, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
        win32::SetWindowLongPtrA(hwnd, win32::DWLP_USER, lParam);
        ui::SendDlgItemMessage(hwnd, IDC_EDIT1, win32::EM_LIMITTEXT, 255, 0);
        win32::SetDlgItemTextA(hwnd, IDC_EDIT1, reinterpret_cast<std::span<char, 256> *>(lParam)->data());
        return 1;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) { ControlIO().pfnEndDialog(hwnd, 0); return 1; }
        if (LOWORD(wParam) != IDOK) break;
        pszName = reinterpret_cast<std::span<char, 256> *>(win32::GetWindowLongPtrA(hwnd, win32::DWLP_USER));
        win32::GetDlgItemTextA(hwnd, IDC_EDIT1, szName, sizeof(szName));
        if (fRaceLibraryNameDialog) {
            if (!FPortableRaceFilename(app, szName)) ids = idsLibraryInvalidName;
            else if (!FRaceLibraryNameAvailable(app, szName)) {
                SuggestRaceFilename(app, std::span{szName});
                win32::SetDlgItemTextA(hwnd, IDC_EDIT1, szName);
                ids = idsLibraryNameTaken;
            } else {
                CchCopyUtf8(*pszName, szName);
                ControlIO().pfnEndDialog(hwnd, 1);
                return 1;
            }
        } else if (!FPortableLibraryName(app, szName)) ids = idsLibraryInvalidName;
        else if (!FLibraryNameAvailable(app, szLibraryRoot, szName)) {
            SuggestLibraryName(app, std::span{szName}, 0);
            win32::SetDlgItemTextA(hwnd, IDC_EDIT1, szName);
            ids = idsLibraryNameTaken;
        } else {
            CchCopyUtf8(*pszName, szName);
            ControlIO().pfnEndDialog(hwnd, 1);
            return 1;
        }
        szMessage = TextIO().pfnCompressed(app, ids);
        CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage.data(), TextIO().pfnCompressed(app, idsLibraryGames).data(), win32::MB_OK | win32::MB_ICONEXCLAMATION);
        return 1;
    }
    return 0;
}

static INT_PTR IdPromptLibraryName(APPSESSION &app, HWND hwnd, std::span<char, 256> rgchName) {
    // The synchronous modal call borrows this view until its window closes.
    return ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_LIBRARY_NAME), hwnd,
                                    LibraryNameDlg, reinterpret_cast<LPARAM>(&rgchName));
}

std::int16_t FChooseLibraryGameName(APPSESSION &app, HWND hwnd, char *pszFile, const char *pszName) {
    char *pch;
    char szMessage[256];

    if (!FInitLibrary() || std::strlen(szLibraryRoot) + 20 >= sizeof(app.state.szBase)) goto LFail;
    szLibraryExchangeName[0] = 0;
    if (pszName) GetGameLibraryName(app, pszName, std::span{szLibraryName});
    else {
        pch = std::strrchr(pszFile, chDirSep);
        CchSnprintf(std::span{szLibraryName, sizeof(szLibraryName)}, "%s", pch ? pch + 1 : pszFile);
        pch = std::strrchr(szLibraryName, '.');
        if (pch) *pch = 0;
        if (!FPortableLibraryName(app, szLibraryName)) CchCopyUtf8(std::span{szLibraryName}, TextIO().pfnCompressed(app, idsLibraryDefaultGameName).data());
    }
    if (pszName) {
        if (!FLibraryNameAvailable(app, szLibraryRoot, szLibraryName)) {
            SuggestLibraryName(app, std::span{szLibraryName}, 1);
            if (ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_LIBRARY_GAME_REVISION), hwnd,
                               LibraryGameRevisionDlg, reinterpret_cast<LPARAM>(szLibraryName)) != 1) return 0;
        }
    } else {
        SuggestLibraryName(app, std::span{szLibraryName}, 0);
        if (IdPromptLibraryName(app, hwnd, std::span{szLibraryName}) != 1) return 0;
    }
    if (!FPortableLibraryName(app, szLibraryName)) goto LFail;
    CchSnprintf(std::span{szLibraryPending, sizeof(szLibraryPending)}, "%s%s%s", szLibraryRoot, szDirSep, szLibraryName);
    /* Reserve only a newly created folder; never write into an existing copy. */
    if (!FLibraryNameAvailable(app, szLibraryRoot, szLibraryName) || CreateNativeDir(szLibraryPending) != 0) {
        szLibraryPending[0] = 0;
        goto LFail;
    }
    CchSnprintf(std::span{pszFile, 256}, "%s%s%s.xy", szLibraryPending, szDirSep, szLibraryName);
    return 1;
LFail:
    CchCopyUtf8Prefix(std::span{szMessage}, TextIO().pfnCompressed(app, idsLibraryStorageFailed).data());
    CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage, TextIO().pfnCompressed(app, idsLibraryGames).data(), win32::MB_OK | win32::MB_ICONHAND);
    return 0;
}

static std::int16_t FChooseLibraryRaceName(APPSESSION &app, HWND hwnd, const char *pszSuggested, char *pszPath, std::size_t cchPath) {
    char szMessage[256];
    if (!FInitRaceLibrary()) return 0;
    CchSnprintf(std::span{szRaceLibraryName, sizeof(szRaceLibraryName)}, "%s", pszSuggested);
    SuggestRaceFilename(app, std::span{szRaceLibraryName});
    if (!szRaceLibraryName[0]) return 0;
    fRaceLibraryNameDialog = 1;
    if (IdPromptLibraryName(app, hwnd, std::span{szRaceLibraryName}) != 1) {
        fRaceLibraryNameDialog = 0;
        return 0;
    }
    fRaceLibraryNameDialog = 0;
    if (CchSnprintf(std::span{pszPath, cchPath}, "%s%s%s", szRaceLibraryRoot, szDirSep, szRaceLibraryName) <= 0 ||
        !FRaceLibraryNameAvailable(app, szRaceLibraryName)) {
        CchCopyUtf8Prefix(std::span{szMessage}, TextIO().pfnCompressed(app, idsLibraryStorageFailed).data());
        CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage, TextIO().pfnCompressed(app, idsLibraryGames).data(), win32::MB_OK | win32::MB_ICONHAND);
        return 0;
    }
    return 1;
}

std::int16_t FWriteLibraryMetadata(const char *pszDir, const char *pszName, int iplr) {
    return FWriteLibraryMetadataValues(pszDir, pszName, iplr, static_cast<long long>(LNativeTime()),
                                       szLibraryExchangeName[0] ? szLibraryExchangeName : pszName);
}

static std::int16_t FWriteLibraryMetadataValues(const char *pszDir, const char *pszName, int iplr,
                                           long long lPlayed, const char *pszExchangeName) {
    char szFile[256], szTemp[256];
    NATIVEFILE *pf;
    int fWritten, fClosed, cch;

    cch = CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s.library.tmp", pszDir, szDirSep);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szFile)) return 0;
    pf = PfOpenNativeFile(szFile, NATIVEFILEMODE::writeTruncate);
    if (!pf) return 0;
    fWritten = CchNativePrintf(pf, "2\n%s\n%d\n%lld\n%s\n", pszName, iplr, lPlayed, pszExchangeName) > 0;
    fClosed = ICloseNativeFile(pf) == 0;
    if (!fWritten || !fClosed) { RemoveNativeFile(szFile); return 0; }
    cch = CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s.library", pszDir, szDirSep);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szFile)) return 0;
    /* Publish complete metadata, preserving the previous session on failure. */
    cch = CchSnprintf(std::span{szTemp, sizeof(szTemp)}, "%s%s.library.tmp", pszDir, szDirSep);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szTemp)) return 0;
    return ReplaceNativeFile(szTemp, szFile) == 0;
}

static std::int16_t RemovePendingFile(APPSESSION &, void *lpUnused, const char *pszFile) {
    char szFile[256];
    static_cast<void>(lpUnused);
    CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s%s", szLibraryPending, szDirSep, pszFile);
    /* Failed generation can leave an empty directory at a file destination. */
    RemoveNativePath(szFile);
    return 1;
}

static std::int16_t FReadLibraryHdr(NATIVEFILE *pf, HDR *phdr) {
    std::byte rgb[cbWireHdr];

    return CReadNative(std::span{&rgb, 1}, pf) == 1 && FDecodeHdr(rgb, *phdr);
}

static std::int16_t FReadLibraryBof(NATIVEFILE *pf, RTBOF *pbof) {
    std::byte rgb[cbWireBof];

    return CReadNative(std::span{&rgb, 1}, pf) == 1 && FDecodeBof(rgb, *pbof);
}

/* GenerateWorld's original companion-write loop ignores writer return values.
 * Verify complete record framing without replacing the loaded player state. */
static std::int16_t FCompleteGeneratedFile(APPSESSION &app, const char *pszFile, DtFileType dt, int iplr) {
    NATIVEFILE *pf;
    HDR hdr;
    RTBOF rtbof;
    long cbFile, ib;
    std::int16_t fComplete;

    pf = PfOpenNativeFile(pszFile, NATIVEFILEMODE::read);
    if (!pf) return 0;
    fComplete = 0;
    if (ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::end) != 0 || (cbFile = LNativePosition(pf)) < 22 || ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::begin) != 0) goto LDone;
    if (!FReadLibraryHdr(pf, &hdr) || hdr.rt != rtBOF || hdr.cb != 16 ||
        !FReadLibraryBof(pf, &rtbof) || std::memcmp(rtbof.rgid, "J3J3", 4) ||
        rtbof.lidGame != app.state.game.lid || rtbof.turn != app.state.game.turn || rtbof.dt != dt || rtbof.iPlayer != iplr) goto LDone;
    /* XY stores raw four-byte star packs between its GAME and EOF records. */
    if (dt == dtXY) {
        if (cbFile != 18 + 66 + 4L * app.state.game.cPlanMax + 4 || ISeekNativeFile(pf, -4, NATIVESEEKORIGIN::end) != 0) goto LDone;
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
        if (ISeekNativeFile(pf, hdr.cb, NATIVESEEKORIGIN::current) != 0) break;
    }
LDone:
    ICloseNativeFile(pf);
    return fComplete;
}



std::int16_t FCopyNativeFileContents(NATIVEFILE *pfSource, NATIVEFILE *pfTarget) {
    char rgb[8192];
    std::size_t cb;
    int fOkay;

    fOkay = 1;
    while ((cb = LibraryFileIO().pfnRead(std::as_writable_bytes(std::span{rgb}), pfSource)) != 0) {
        if (LibraryFileIO().pfnWrite(std::as_bytes(std::span{rgb}).first(cb), pfTarget) != cb) { fOkay = 0; break; }
    }
    if (LibraryFileIO().pfnError(pfSource)) fOkay = 0;
    if (LibraryFileIO().pfnClose(pfSource) != 0) fOkay = 0;
    if (LibraryFileIO().pfnClose(pfTarget) != 0) fOkay = 0;
    return fOkay;
}

static std::int16_t FCopyNativeFile(const char *pszSource, const char *pszTarget) {
    NATIVEFILE *pfSource, *pfTarget;
    int fOkay;

    pfSource = PfOpenNativeFile(pszSource, NATIVEFILEMODE::read);
    if (!pfSource) return 0;
    pfTarget = PfOpenNativeFile(pszTarget, NATIVEFILEMODE::writeTruncate);
    if (!pfTarget) { ICloseNativeFile(pfSource); return 0; }
    fOkay = FCopyNativeFileContents(pfSource, pfTarget);
    if (!fOkay) RemoveNativeFile(pszTarget);
    return fOkay;
}

std::int16_t FCopyNativeFileExclusive(const char *pszSource, const char *pszTarget) {
    NATIVEFILE *pfSource, *pfTarget;
    int iErrno;
    int fOkay;

    pfSource = LibraryFileIO().pfnOpen(pszSource, NATIVEFILEMODE::read);
    if (!pfSource) return 0;
    pfTarget = LibraryFileIO().pfnOpen(pszTarget, NATIVEFILEMODE::writeExclusive);
    if (!pfTarget) {
        iErrno = errno;
        LibraryFileIO().pfnClose(pfSource);
        errno = iErrno;
        return 0;
    }
    fOkay = FCopyNativeFileContents(pfSource, pfTarget);
    if (!fOkay) RemoveNativeFile(pszTarget);
    return fOkay;
}

static std::int16_t FCopyRaceExport(const char *pszSource, const char *pszTarget,
                               char *pszTemp, std::size_t cchTemp) {
    const char *pchSlash;
    std::size_t cchDirectory;
    std::uint32_t dwTick;
    int i, cch;

    pchSlash = std::strrchr(pszTarget, chDirSep);
    cchDirectory = pchSlash ? static_cast<std::size_t>((pchSlash - pszTarget)) + 1 : 0;
    if (cchDirectory > INT_MAX) return 0;
    dwTick = DwTickCount();
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(std::span{pszTemp, cchTemp}, "%.*s.stars-race-export-%u-%u.tmp",
                          static_cast<int>(cchDirectory), pszTarget, dwTick, idRaceStage++);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= cchTemp) return 0;
        errno = 0;
        if (FCopyNativeFileExclusive(pszSource, pszTemp)) return 1;
        if (errno != EEXIST) return 0;
    }
    return 0;
}

std::int16_t FSaveLibraryRace(APPSESSION &app, HWND hwnd, const char *pszName, PLAYER *pplr) {
    char szPath[512];
    char szTempDir[512];
    std::uint16_t icksum;
    std::int16_t fOkay;
    if (!FInitRaceLibrary() || !FPortableRaceFilename(app, pszName)) return 0;
    CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    if (!FCreateRaceStagePath(szTempDir, sizeof(szTempDir), szRaceLibraryTemp, sizeof(szRaceLibraryTemp))) return 0;
    if (!RecordIO().pfnFCreateFile(app, dtRace, iplrNone, szRaceLibraryTemp)) {
        RemoveNativeFile(szRaceLibraryTemp);
        RemoveNativePath(szTempDir);
        return 0;
    }
    WriteRtPlr(app, pplr);
    icksum = IRaceChecksum(pplr);
    WriteRtEof(app, icksum);
    RecordIO().pfnStreamClose(app);
    fOkay = ReplaceNativeFile(szRaceLibraryTemp, szPath) == 0;
    if (!fOkay) {
        RemoveNativeFile(szRaceLibraryTemp);
        RemoveNativePath(szTempDir);
        if (win32::IsWindow(hwnd)) AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsStarsUnableSaveRaceDataFilePlease, {}).data(), win32::MB_ICONHAND);
        return 0;
    }
    RemoveNativePath(szTempDir);
    CchSnprintf(std::span{szRaceLibraryPath, sizeof(szRaceLibraryPath)}, "%s", szPath);
    SetSzRaceFile(app, pszName);
    fNewLibraryRace = 0;
    return 1;
}

/* Compare the complete persisted design, including credentials, rather than
 * its collision-prone checksum or unused bytes after name terminators. */
static std::int16_t FSameLibraryRace(const PLAYER *pplrLeft, const PLAYER *pplrRight) {
    PLAYER plrLeft = *pplrLeft, plrRight = *pplrRight;
    std::byte rgbLeft[cbWirePlayer], rgbRight[cbWirePlayer];
    plrLeft.fLearned = plrRight.fLearned = 0;
    FEncodePlayer(plrLeft, rgbLeft); FEncodePlayer(plrRight, rgbRight);
    return !std::memcmp(rgbLeft, rgbRight, ibWirePlayerNames) &&
           !std::strcmp(plrLeft.szName, plrRight.szName) && !std::strcmp(plrLeft.szNames, plrRight.szNames);
}

static std::int16_t FReadManagedRace(APPSESSION &app, char *pszPath, PLAYER *pplr) {
    std::optional<NATIVEPATHINFO> npiPath;
    NATIVEFILE *pf;
    HDR hdr;
    RTBOF bof;
    RACESTATE rsSav;
    std::uint16_t wVersSav = app.state.wVersFile;
    std::int16_t fSilentSav = app.state.fFileErrSilent, fOkay;

    if (!(npiPath = FNativePathInfo(pszPath)) || npiPath->fDirectory || npiPath->fLink) return 0;
    pf = PfOpenNativeFile(pszPath, NATIVEFILEMODE::read);
    if (!pf) return 0;
    // Browsing versions must never trigger legacy conversion/password dialogs.
    fOkay = FReadLibraryHdr(pf, &hdr) && hdr.rt == rtBOF && hdr.cb == cbWireBof &&
            FReadLibraryBof(pf, &bof) && !std::memcmp(bof.rgid, "J3J3", 4) &&
            bof.verMajor == 2 && bof.verMinor == 85 && bof.dt == dtRace;
    ICloseNativeFile(pf);
    if (!fOkay) return 0;
    SaveRaceState(app, &rsSav);
    app.state.fFileErrSilent = 1;
    fOkay = FWasRaceFile(app, pszPath, 0) > 0;
    if (fOkay) *pplr = app.state.vplr;
    RestoreRaceState(app, &rsSav, 1, 1);
    app.state.fFileErrSilent = fSilentSav;
    app.state.wVersFile = wVersSav;
    return fOkay;
}

struct RACEVERSIONS {
    char szStem[240];
    char szLatest[256];
    char szSame[256];
    const PLAYER *pplr;
    int iMax, iLatest, iSame;
};

static void GetRaceLibraryStem(APPSESSION &app, const char *pszName, char *pszStem) {
    std::size_t i, cch;
    char szTest[256];

    CchSnprintf(std::span{pszStem, 240}, "%s", pszName);
    for (i = 0; pszStem[i]; i++)
        if (static_cast<unsigned char>(pszStem[i]) < 32 || pszStem[i] == 127 || std::strchr("<>:\"/\\|?*", pszStem[i])) pszStem[i] = '_';
    cch = std::strlen(pszStem);
    while (cch != 0 && (pszStem[cch - 1] == ' ' || pszStem[cch - 1] == '.')) pszStem[--cch] = 0;
    while (pszStem[0] == ' ' || pszStem[0] == '.') std::memmove(pszStem, pszStem + 1, std::strlen(pszStem));
    if (!pszStem[0]) CchSnprintf(std::span{pszStem, 240}, "%s", TextIO().pfnCompressed(app, idsLibraryDefaultRaceName).data());
    CchSnprintf(std::span{szTest, sizeof(szTest)}, "%s.r1", pszStem);
    if (!FPortableRaceFilename(app, szTest)) {
        // Prefix reserved device names without altering the in-game race name.
        std::memmove(pszStem + 1, pszStem, std::strlen(pszStem) + 1);
        pszStem[0] = '_';
    }
}

static std::int16_t FindRaceVersion(APPSESSION &app, void *lpData, const char *pszName) {
    RACEVERSIONS *prv = static_cast<RACEVERSIONS *>(lpData);
    const char *pch = std::strrchr(pszName, '.');
    char szPath[512];
    PLAYER plr;
    int iVersion;
    std::size_t cch = std::strlen(prv->szStem);

    if (!pch || !FPortableRaceFilename(app, pszName)) return 1;
    iVersion = std::atoi(pch + 2);
    if (static_cast<std::size_t>((pch - pszName)) == cch && !SDL_strncasecmp(pszName, prv->szStem, cch))
        prv->iMax = StarsMax(prv->iMax, iVersion);
    CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    if (!FReadManagedRace(app, szPath, &plr)) return 1;
    // Versions follow the stored race name, including older/imported filenames.
    // Distinct names can sanitize to the same stem without sharing a design.
    if (SDL_strcasecmp(plr.szName, prv->pplr->szName)) return 1;
    prv->iMax = StarsMax(prv->iMax, iVersion);
    if (iVersion > prv->iLatest || (iVersion == prv->iLatest && SDL_strcasecmp(pszName, prv->szLatest) < 0)) {
        prv->iLatest = iVersion;
        CchCopyUtf8(std::span{prv->szLatest}, pszName);
    }
    if (iVersion > prv->iSame && FSameLibraryRace(prv->pplr, &plr)) {
        prv->iSame = iVersion;
        CchCopyUtf8(std::span{prv->szSame}, pszName);
    }
    return 1;
}

INT_PTR  LibraryRaceSaveDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    char szMessage[512];
    POINT16 pt = {-1, -1};
    switch (message) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnSetWindowText(hwnd, TextIO().pfnCompressed(app, idsLibraryRaceSaveTitle).data());
        CchSnprintf(std::span{szMessage, sizeof(szMessage)}, TextIO().pfnCompressed(app, idsLibraryRaceSavePrompt).data(), reinterpret_cast<const char *>(lParam));
        win32::SetDlgItemTextA(hwnd, IDC_LIBRARY_RACE_SAVE_TEXT, szMessage);
        win32::SetDlgItemTextA(hwnd, IDC_LIBRARY_RACE_UPDATE, TextIO().pfnCompressed(app, idsLibraryRaceUpdate).data());
        win32::SetDlgItemTextA(hwnd, IDC_LIBRARY_RACE_VERSION, TextIO().pfnCompressed(app, idsLibraryRaceNewVersion).data());
        win32::SetDlgItemTextA(hwnd, IDCANCEL, TextIO().pfnCompressed(app, idsCancel).data());
        ControlIO().pfnStickyDlgPos(hwnd, &pt, 1);
        return 1;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) { ControlIO().pfnEndDialog(hwnd, 0); return 1; }
        if (LOWORD(wParam) == IDC_LIBRARY_RACE_UPDATE) { ControlIO().pfnEndDialog(hwnd, 1); return 1; }
        if (LOWORD(wParam) == IDC_LIBRARY_RACE_VERSION) { ControlIO().pfnEndDialog(hwnd, 2); return 1; }
        break;
    }
    return 0;
}

std::int16_t FSaveCurrentLibraryRace(APPSESSION &app, HWND hwnd, PLAYER *pplr) {
    RACEVERSIONS rv = {0};
    RACESTATE rsSav;
    PLAYER plr;
    char szName[256], szPath[512];
    const char *pszExisting;
    int iChoice;

    if (!FHasCurrentLibraryRace() || !FInitRaceLibrary()) return 0;
    rv.pplr = pplr;
    GetRaceLibraryStem(app, pplr->szName, rv.szStem);
    if (!FNativeEnumerateDirectory(app, szRaceLibraryRoot, FindRaceVersion, &rv)) return 0;
    pszExisting = !fNewLibraryRace && szRaceLibraryPath[0] ? PszCurrentLibraryRaceName() : rv.szLatest;
    if (pszExisting[0]) {
        CchSnprintf(std::span{szName, sizeof(szName)}, "%s", pszExisting);
        CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", szRaceLibraryRoot, szDirSep, szName);
        if (!FReadManagedRace(app, szPath, &plr)) goto LFail;
        if (FSameLibraryRace(pplr, &plr)) goto LReuse;
    }
    if (rv.szSame[0]) {
        CchCopyUtf8(std::span{szName}, rv.szSame);
        goto LReuse;
    }
    iChoice = pszExisting[0] ? static_cast<int>(ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_LIBRARY_RACE_SAVE), hwnd,
                                                 LibraryRaceSaveDlg, reinterpret_cast<LPARAM>(szName))) : 2;
    if (iChoice != 1 && iChoice != 2) return 0;
    if (iChoice == 2) {
        if (rv.iMax >= 999) goto LFail;
        CchSnprintf(std::span{szName, sizeof(szName)}, "%s.r%d", rv.szStem, rv.iMax + 1);
        if (!FPortableRaceFilename(app, szName) || !FRaceLibraryNameAvailable(app, szName)) goto LFail;
    } else if (fNewLibraryRace && plr.lSalt != 0) {
        // Updating a protected entry also requires its existing credentials.
        SaveRaceState(app, &rsSav);
        iChoice = FLoadLibraryRace(app, hwnd, szName);
        RestoreRaceState(app, &rsSav, 1, 1);
        if (!iChoice) return 0;
    }
    return FSaveLibraryRace(app, hwnd, szName, pplr);
LReuse:
    CchSnprintf(std::span{szRaceLibraryPath, sizeof(szRaceLibraryPath)}, "%s%s%s", szRaceLibraryRoot, szDirSep, szName);
    SetSzRaceFile(app, szName);
    fNewLibraryRace = 0;
    return 1;
LFail:
    if (win32::IsWindow(hwnd)) AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryRaceSaveFailed).data(), win32::MB_ICONHAND);
    return 0;
}

std::int16_t FHasCurrentLibraryRace(void) {
    return fNewLibraryRace || szRaceLibraryPath[0] != 0;
}

std::int16_t FLoadLibraryRace(APPSESSION &app, HWND hwnd, const char *pszName) {
    char szPath[512];
    RACESTATE rsSav;
    std::int16_t fResult;
    if (!FInitRaceLibrary() || !FPortableRaceFilename(app, pszName)) return 0;
    CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    SaveRaceState(app, &rsSav);
    fResult = FWasRaceFile(app, szPath, 1);
    if (fResult <= 0) {
        RestoreRaceState(app, &rsSav, 1, 1);
        if (win32::IsWindow(hwnd)) AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryRaceOpenFailed).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    CchSnprintf(std::span{szRaceLibraryPath, sizeof(szRaceLibraryPath)}, "%s", szPath);
    SetSzRaceFile(app, pszName);
    fNewLibraryRace = 0;
    return 1;
}

const char *PszCurrentLibraryRaceName(void) {
    const char *pch = std::strrchr(szRaceLibraryPath, chDirSep);
    return pch ? pch + 1 : szRaceLibraryPath;
}

/* The new-game roster owns a filename for each design. Bind only this edit
 * to that destination; a previously imported design is not its save target. */
std::int16_t FCustomizeLibraryRace(APPSESSION &app, HWND hwnd, char *pszName) {
    RACESTATE rsSav;
    char szPath[512];
    std::int16_t fResult;

    SaveRaceState(app, &rsSav);
    if (pszName[0]) {
        if (!FInitRaceLibrary() || !FPortableRaceFilename(app, pszName)) return 0;
        CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
        CchSnprintf(std::span{szRaceLibraryPath, sizeof(szRaceLibraryPath)}, "%s", szPath);
        SetSzRaceFile(app, pszName);
        fNewLibraryRace = 0;
    } else {
        if (!FInitRaceLibrary()) return 0;
        szRaceLibraryPath[0] = 0;
        fNewLibraryRace = 1;
    }
    fResult = RaceCreationWizard(app, hwnd, 0, 0);
    if (fResult) CchSnprintf(std::span{pszName, 256}, "%s", PszCurrentLibraryRaceName());
    RestoreRaceState(app, &rsSav, !fResult, 1);
    return fResult;
}

std::int16_t FBeginLibraryRace(APPSESSION &app, HWND hwnd) {
    RACESTATE rsSav;
    std::int16_t fResult;
    SaveRaceState(app, &rsSav);
    if (!FInitRaceLibrary()) return 0;
    szRaceLibraryPath[0] = 0;
    fNewLibraryRace = 1;
    app.state.vplr = vrgplrDef[0];
    CchCopyUtf8Prefix(std::span{app.state.vplr.szName}, TextIO().pfnCompressed(app, idsHumanoid).data());
    CchCopyUtf8Prefix(std::span{app.state.vplr.szNames}, TextIO().pfnCompressed(app, idsLibraryHumanoids).data());
    app.state.szRacePass[0] = 0;
    SetSzRaceFile(app, "");
    fResult = RaceCreationWizard(app, hwnd, 0, 0);
    if (!fResult) {
        RestoreRaceState(app, &rsSav, 1, 1);
    }
    else RestoreRaceState(app, &rsSav, 0, 1);
    return fResult;
}

std::int16_t FEditLibraryRace(APPSESSION &app, HWND hwnd, const char *pszName) {
    RACESTATE rsSav;
    std::int16_t fResult;
    SaveRaceState(app, &rsSav);
    if (!FLoadLibraryRace(app, hwnd, pszName)) return 0;
    fResult = RaceCreationWizard(app, hwnd, 0, 0);
    RestoreRaceState(app, &rsSav, !fResult, 1);
    return fResult;
}

std::int16_t FDuplicateLibraryRace(APPSESSION &app, HWND hwnd, const char *pszName) {
    char szSource[512], szName[256], szPath[512] = "";
    const char *pch;
    RACESTATE rsSav;
    if (!FInitRaceLibrary() || !FPortableRaceFilename(app, pszName)) return 0;
    CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    SaveRaceState(app, &rsSav);
    if (FWasRaceFile(app, szSource, 1) <= 0) {
        RestoreRaceState(app, &rsSav, 1, 1);
        return 0;
    }
    CchSnprintf(std::span{szName, sizeof(szName)}, "%s", pszName);
    pch = std::strrchr(szName, '.');
    if (pch) szName[pch - szName] = 0;
    SuggestRaceFilename(app, std::span{szName});
    if (!szName[0] || CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", szRaceLibraryRoot, szDirSep, szName) <= 0) {
        RestoreRaceState(app, &rsSav, 1, 1);
        if (win32::IsWindow(hwnd)) AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryRaceDuplicateFailed).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    if (!FCopyNativeFile(szSource, szPath)) {
        RestoreRaceState(app, &rsSav, 1, 1);
        if (win32::IsWindow(hwnd)) AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryRaceDuplicateFailed).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    if (FWasRaceFile(app, szPath, 1) <= 0) {
        RemoveNativeFile(szPath);
        RestoreRaceState(app, &rsSav, 1, 1);
        if (win32::IsWindow(hwnd)) AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryRaceDuplicateFailed).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    RestoreRaceState(app, &rsSav, 1, 1);
    return 1;
}

static void GetLibraryFileFilter(APPSESSION &app, StringId ids, std::span<char> pszFilter) {
    char *pch;

    if (pszFilter.empty()) return;
    TextIO().pfnCchGetString(app, ids, pszFilter);
    for (pch = pszFilter.data(); *pch; pch++) if (*pch == '|') *pch = 0;
}

std::int16_t FExportLibraryRace(APPSESSION &app, HWND hwnd, const char *pszName, const char *pszTarget) {
    OPENFILENAME ofn;
    char szSource[512], szTarget[512], szTemp[512], szValidate[512], szFilter[128];
    RACESTATE rsSav;
    if (!FInitRaceLibrary() || !FPortableRaceFilename(app, pszName)) return 0;
    CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    if (!pszTarget) {
        ofn = {};
        std::ranges::fill(szTarget, 0);
        CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s", pszName);
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        GetLibraryFileFilter(app, idsLibraryRaceFilesFilter, szFilter);
        ofn.lpstrFilter = szFilter;
        ofn.lpstrFile = szTarget;
        ofn.nMaxFile = sizeof(szTarget);
        ofn.Flags = win32::OFN_HIDEREADONLY | win32::OFN_PATHMUSTEXIST | win32::OFN_OVERWRITEPROMPT;
        if (!DesktopChoiceIO().pfnFNativeSaveFileName(&ofn)) return 0;
        pszTarget = szTarget;
    }
    SaveRaceState(app, &rsSav);
    if (FWasRaceFile(app, szSource, 1) <= 0 || SDL_strcasecmp(szSource, pszTarget) == 0) {
        RestoreRaceState(app, &rsSav, 1, 1);
        if (win32::IsWindow(hwnd)) AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryRaceExportFailed).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    if (!FCopyRaceExport(szSource, pszTarget, szTemp, sizeof(szTemp))) {
        RestoreRaceState(app, &rsSav, 1, 1);
        if (win32::IsWindow(hwnd)) AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryRaceExportFailed).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    CchSnprintf(std::span{szValidate, sizeof(szValidate)}, "%s", szTemp);
    if (FWasRaceFile(app, szValidate, 1) <= 0 || ReplaceNativeFile(szTemp, pszTarget) != 0) {
        RemoveNativeFile(szTemp);
        RestoreRaceState(app, &rsSav, 1, 1);
        if (win32::IsWindow(hwnd)) AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryRaceExportFailed).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    RestoreRaceState(app, &rsSav, 1, 1);
    return 1;
}

std::int16_t FExportLibraryOrders(APPSESSION &app, HWND hwnd, const char *pszTarget) {
    OPENFILENAME ofn;
    char szBaseSav[sizeof(app.state.szBase)], szBaseStem[sizeof(app.state.szBase)], szName[256], szSuggested[256], szTarget[512], szFilter[64];
    char szStageBase[sizeof(app.state.szBase)], szStage[sizeof(app.state.szBase)], szStageHist[sizeof(app.state.szBase)];
    char szSource[sizeof(app.state.szBase)], szSourceHist[sizeof(app.state.szBase)], szPreviousHist[sizeof(app.state.szBase)];
    char szTemp[512], szMessage[640], szExt[4];
    char *pch, *pchSlash;
    const char *pszLibraryName;
    StringId idsSaveError;
    NATIVEFILE *pf;
    std::uint32_t dwTick;

    int cch;
    std::int16_t fSubmitSav, fSaved, fDirtySav, fHistoryExists, fHistoryReplaced, fHistoryBackup;

    if (!FInitLibrary() || !app.state.lpPlanets || app.state.game.lid == 0 || app.state.idPlayer == iplrNone || app.state.idPlayer < 0 || app.state.idPlayer >= 16) return 0;
    if (!LibraryPathIO().pfnUnder(szLibraryRoot, app.state.szBase)) return 0;
    RememberLibraryGame(app);

    pszLibraryName = szLibraryExchangeName;
    if (!pszLibraryName[0]) {
        pch = PchLibrarySeparator(app.state.szBase);
        if (!pch) return 0;
        pchSlash = pch;
        while (pchSlash > app.state.szBase && pchSlash[-1] != chDirSep
#ifdef _WIN32
               && pchSlash[-1] != '/'
#endif
        ) pchSlash--;
        cch = CchSnprintf(std::span{szName, sizeof(szName)}, "%.*s", static_cast<int>((pch - pchSlash)), pchSlash);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szName)) return 0;
        pszLibraryName = szName;
    }
    cch = CchSnprintf(std::span{szSuggested, sizeof(szSuggested)}, "%s.x%d", pszLibraryName, app.state.idPlayer + 1);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szSuggested)) return 0;
    if (!pszTarget) {
        ofn = {};
        std::ranges::fill(szTarget, 0);
        CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s", szSuggested);
        cch = CchSnprintf(std::span{szFilter, sizeof(szFilter)}, TextIO().pfnCompressed(app, idsLibraryOrdersExportFilter).data(),
                          app.state.idPlayer + 1, app.state.idPlayer + 1);
        if (cch <= 0 || static_cast<std::size_t>(cch) + 2 >= sizeof(szFilter)) return 0;
        for (pch = szFilter; *pch; pch++) if (*pch == '|') *pch = 0;
        szFilter[cch + 1] = 0;
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = szFilter;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = szTarget;
        ofn.nMaxFile = sizeof(szTarget);
        CchSnprintf(std::span{szExt, sizeof(szExt)}, "x%d", app.state.idPlayer + 1);
        ofn.lpstrDefExt = szExt;
        ofn.Flags = win32::OFN_HIDEREADONLY | win32::OFN_PATHMUSTEXIST;
        if (!DesktopChoiceIO().pfnFNativeSaveFileName(&ofn)) return 0;
        pszTarget = szTarget;
    }

    if (std::strlen(pszTarget) >= sizeof(szTarget)) return 0;
    if (pszTarget != szTarget) CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s", pszTarget);
    pchSlash = std::strrchr(szTarget, '/');
    pch = std::strrchr(szTarget, '\\');
    if (pch && (!pchSlash || pch > pchSlash)) pchSlash = pch;
    pch = std::strrchr(szTarget, '.');
    if (pch && (!pchSlash || pch > pchSlash)) *pch = 0;
    if (!szTarget[0]) return 0;
    cch = CchSnprintf(std::span{szTarget + std::strlen(szTarget), sizeof(szTarget) - std::strlen(szTarget)}, ".x%d", app.state.idPlayer + 1);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget) - std::strlen(szTarget)) return 0;

    /* Confirm the actual orders path after correcting the chooser's extension. */
    if (FNativeFileSize(szTarget)) {
        CchSnprintf(std::span{szMessage, sizeof(szMessage)}, TextIO().pfnCompressed(app, idsLibraryReplaceOrders).data(), szTarget);
        if (CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage, TextIO().pfnCompressed(app, idsLibraryTitle).data(), win32::MB_YESNO | win32::MB_ICONEXCLAMATION | win32::MB_DEFBUTTON2) != win32::IDYES) return 0;
    }

    SetSzWorkFromDt(app, dtLog, app.state.idPlayer);
    CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s", app.state.szWork);
    if (SDL_strcasecmp(szSource, szTarget) == 0) return 0;
    CchSnprintf(std::span{szBaseSav, sizeof(szBaseSav)}, "%s", app.state.szBase);
    CchSnprintf(std::span{szBaseStem, sizeof(szBaseStem)}, "%s", szBaseSav);
    fSubmitSav = app.state.gd.fSubmit;
    fDirtySav = app.state.game.fDirty;
    fHistoryReplaced = 0;
    fHistoryBackup = 0;
    dwTick = DwTickCount();
    pch = std::strrchr(szBaseStem, '.');
    pchSlash = PchLibrarySeparator(szBaseStem);
    if (pch && (!pchSlash || pch > pchSlash)) *pch = 0;
    cch = CchSnprintf(std::span{szStageBase, sizeof(szStageBase)}, "%s_exportstage_%u.m%d", szBaseStem, dwTick, app.state.idPlayer + 1);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szStageBase)) return 0;
    /* FWriteLogFile creates its destination directly, so save to a sibling first. */
    fSaved = FWriteLogFile(app, szStageBase, app.state.idPlayer);
    CchSnprintf(std::span{app.state.szBase, sizeof(app.state.szBase)}, "%s", szBaseSav);
    cch = CchSnprintf(std::span{szStage, sizeof(szStage)}, "%s_exportstage_%u.x%d", szBaseStem, dwTick, app.state.idPlayer + 1);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szStage)) fSaved = 0;
    cch = CchSnprintf(std::span{szStageHist, sizeof(szStageHist)}, "%s_exportstage_%u.h%d", szBaseStem, dwTick, app.state.idPlayer + 1);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szStageHist)) fSaved = 0;
    if (fSaved) {
        CchSnprintf(std::span{app.state.szBase, sizeof(app.state.szBase)}, "%s", szStageBase);
        fSaved = FWriteHistFile(app, app.state.idPlayer);
        CchSnprintf(std::span{app.state.szBase, sizeof(app.state.szBase)}, "%s", szBaseSav);
    }
    SetSzWorkFromDt(app, dtHist, app.state.idPlayer);
    CchSnprintf(std::span{szSourceHist, sizeof(szSourceHist)}, "%s", app.state.szWork);
    pf = PfOpenNativeFile(szSourceHist, NATIVEFILEMODE::read);
    fHistoryExists = pf != nullptr;
    if (pf) ICloseNativeFile(pf);
    else if (errno != ENOENT) fSaved = 0;
    cch = CchSnprintf(std::span{szPreviousHist, sizeof(szPreviousHist)}, "%s_exportprevious_%u.h%d",
                      szBaseStem, dwTick, app.state.idPlayer + 1);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szPreviousHist)) fSaved = 0;
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
    fHistoryReplaced = 0;
    if (fHistoryBackup) RemoveNativeFile(szPreviousHist);
    CchSnprintf(std::span{app.state.szBase, sizeof(app.state.szBase)}, "%s", szBaseSav);
    /* Writing the orders BOF must not change the turn's submission state. */
    app.state.gd.fSubmit = fSubmitSav;

    CchSnprintf(std::span{szTemp, sizeof(szTemp)}, "%s.stars-export-%u", szTarget, dwTick);
    if (!FCopyNativeFileExclusive(szSource, szTemp) || ReplaceNativeFile(szTemp, szTarget) != 0) {
        RemoveNativeFile(szTemp);
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryOrdersExportFailed).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    return 1;
LSaveFailed:
    if (fHistoryReplaced) {
        if (fHistoryExists && ReplaceNativeFile(szPreviousHist, szSourceHist) != 0) {
            AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryHistoryRestoreFailed).data(), win32::MB_ICONHAND);
            fHistoryBackup = 0;
        } else if (!fHistoryExists) RemoveNativeFile(szSourceHist);
        else fHistoryBackup = 0;
    }
    RemoveNativeFile(szStage);
    RemoveNativeFile(szStageHist);
    if (fHistoryBackup) RemoveNativeFile(szPreviousHist);
    CchSnprintf(std::span{app.state.szBase, sizeof(app.state.szBase)}, "%s", szBaseSav);
    app.state.gd.fSubmit = fSubmitSav;
    app.state.game.fDirty = fDirtySav;
    AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsSaveError).data(), win32::MB_ICONHAND);
    return 0;
}

std::int16_t FDeleteLibraryRace(APPSESSION &app, HWND hwnd, const char *pszName) {
    char szPath[512], szMessage[320];
    std::optional<std::int64_t> cbFile;
    if (!FInitRaceLibrary() || !FPortableRaceFilename(app, pszName)) return 0;
    CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    if (!(cbFile = FNativeFileSize(szPath)) || (*cbFile) <= 0) return 0;
    CchSnprintf(std::span{szMessage, sizeof(szMessage)}, TextIO().pfnCompressed(app, idsLibraryRaceDeletePrompt).data(), pszName);
    if (CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage, TextIO().pfnCompressed(app, idsLibraryTitle).data(), win32::MB_YESNO | win32::MB_ICONQUESTION | win32::MB_TASKMODAL) != win32::IDYES) return 0;
    if (RemoveNativeFile(szPath) != 0) {
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryRaceDeleteFailed).data(), win32::MB_ICONHAND);
        return 0;
    }
    if (!std::strcmp(szRaceLibraryPath, szPath)) szRaceLibraryPath[0] = 0;
    return 1;
}

std::int16_t FImportLibraryRace(APPSESSION &app, HWND hwnd, const char *pszSource) {
    OPENFILENAME ofn;
    char szSource[512], szName[256], szStem[256], szPath[512], szStage[512], szStageDir[512], szFilter[128];
    char *pch, *pchSlash, *pchExt;
    RACESTATE rsSav;
    std::int16_t fValid;
    std::int16_t fLegacy;
    if (!pszSource) {
        ofn = {};
        std::ranges::fill(szSource, 0);
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        GetLibraryFileFilter(app, idsLibraryRaceFilesFilter, szFilter);
        ofn.lpstrFilter = szFilter;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = szSource;
        ofn.nMaxFile = sizeof(szSource);
        ofn.Flags = win32::OFN_HIDEREADONLY | win32::OFN_PATHMUSTEXIST | win32::OFN_FILEMUSTEXIST;
        if (!DesktopChoiceIO().pfnFNativeOpenFileName(&ofn)) return 0;
        pszSource = szSource;
    }
    if (std::strlen(pszSource) >= sizeof(szSource)) return 0;
    if (pszSource != szSource) std::ranges::copy_n(pszSource, std::strlen(pszSource) + 1, std::span{szSource}.begin());
    SaveRaceState(app, &rsSav);
    fValid = FWasRaceFile(app, szSource, 1);
    if (fValid <= 0) {
        RestoreRaceState(app, &rsSav, 1, 1);
        return 0;
    }
    fLegacy = VerMinor(app.state.wVersFile) < 85;
    pchSlash = std::strrchr(szSource, '/');
    pch = std::strrchr(szSource, '\\');
    if (pch && (!pchSlash || pch > pchSlash)) pchSlash = pch;
    pch = pchSlash ? pchSlash + 1 : szSource;
    CchSnprintf(std::span{szStem, sizeof(szStem)}, "%s", pch);
    pchExt = std::strrchr(szStem, '.');
    if (pchExt) *pchExt = 0;
    CchSnprintf(std::span{szName, sizeof(szName)}, "%s.r1", szStem);
    if (pszRaceLibraryNameOverride) {
        if (!FInitRaceLibrary() || !FPortableRaceFilename(app, pszRaceLibraryNameOverride) ||
            !FRaceLibraryNameAvailable(app, pszRaceLibraryNameOverride)) goto LRestore;
        CchSnprintf(std::span{szRaceLibraryName, sizeof(szRaceLibraryName)}, "%s", pszRaceLibraryNameOverride);
        CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", szRaceLibraryRoot, szDirSep, szRaceLibraryName);
    } else if (!FChooseLibraryRaceName(app, hwnd, szName, szPath, sizeof(szPath))) goto LRestore;
    /* Legacy reads convert names and credentials in memory; retain that state,
     * never the original CP1252 bytes or password hash. */
    if (fLegacy) {
        if (!FSaveLibraryRace(app, hwnd, szRaceLibraryName, &app.state.vplr)) goto LRestore;
        return 1;
    }
    if (!FCreateRaceStagePath(szStageDir, sizeof(szStageDir), szStage, sizeof(szStage))) goto LRestore;
    if (!FCopyNativeFile(szSource, szStage) || ReplaceNativeFile(szStage, szPath) != 0) {
        RemoveNativeFile(szStage);
        RemoveNativePath(szStageDir);
        goto LRestore;
    }
    RemoveNativePath(szStageDir);
    if (FWasRaceFile(app, szPath, 1) <= 0) {
        RemoveNativeFile(szPath);
        goto LRestore;
    }
    CchSnprintf(std::span{szRaceLibraryPath, sizeof(szRaceLibraryPath)}, "%s", szPath);
    SetSzRaceFile(app, std::strrchr(szPath, chDirSep) + 1);
    return 1;
LRestore:
    RestoreRaceState(app, &rsSav, 1, 1);
    return 0;
}

std::int16_t FImportLibraryRaceAs(APPSESSION &app, HWND hwnd, const char *pszSource, const char *pszName) {
    std::int16_t fResult;
    pszRaceLibraryNameOverride = pszName;
    fResult = FImportLibraryRace(app, hwnd, pszSource);
    pszRaceLibraryNameOverride = nullptr;
    return fResult;
}

std::int16_t FImportLibraryRaceAndEdit(APPSESSION &app, HWND hwnd, const char *pszSource) {
    const char *pszName;
    if (!FImportLibraryRace(app, hwnd, pszSource)) return 0;
    pszName = std::strrchr(szRaceLibraryPath, chDirSep);
    if (pszName) pszName++;
    else pszName = szRaceLibraryPath;
    /* Import remains successful when the user cancels customization. */
    FEditLibraryRace(app, hwnd, pszName);
    return 1;
}

/* Inspect record framing and BOF metadata without calling FOpenFile, which
 * binds validation to the active game globals. */
std::int16_t FInspectImportFile(const char *pszFile, DtFileType dt, int iplr,
                                 std::int32_t *plidGame, std::uint16_t *pturn, std::uint16_t *pwGen, std::int16_t *pfMulti) {
    NATIVEFILE *pf;
    HDR hdr;
    RTBOF rtbof;
    std::int64_t cbFile;
    long ib;
    std::int16_t fComplete;

    pf = PfOpenNativeFile(pszFile, NATIVEFILEMODE::read);
    if (!pf) return 0;
    fComplete = 0;
    if (ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::end) != 0 || (cbFile = LNativePosition(pf)) < 22 || ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::begin) != 0) goto LDone;
    if (!FReadLibraryHdr(pf, &hdr) || hdr.rt != rtBOF || hdr.cb != cbWireBof ||
        !FReadLibraryBof(pf, &rtbof) || std::memcmp(rtbof.rgid, "J3J3", 4) ||
        rtbof.verMajor != 2 || rtbof.verMinor != 85 || rtbof.dt != dt ||
        rtbof.iPlayer != iplr || rtbof.lidGame == 0) goto LDone;
    if (dt == dtXY) {
        std::uint32_t cPlanets;
        if (cbFile < 88 || (cbFile - 88) % 4 != 0 || (cPlanets = static_cast<std::uint32_t>(((cbFile - 88) / 4))) > 999 ||
            ISeekNativeFile(pf, 18, NATIVESEEKORIGIN::begin) != 0 || !FReadLibraryHdr(pf, &hdr) || hdr.rt != rtGame || hdr.cb != 64 ||
            ISeekNativeFile(pf, -4, NATIVESEEKORIGIN::end) != 0 || !FReadLibraryHdr(pf, &hdr) || hdr.rt != rtEOF || hdr.cb != 2) goto LDone;
        fComplete = 1;
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
            fComplete = 1;
            break;
        }
        if (ISeekNativeFile(pf, hdr.cb, NATIVESEEKORIGIN::current) != 0) break;
    }
    if (fComplete) {
        *plidGame = rtbof.lidGame;
        if (pturn) *pturn = rtbof.turn;
        if (pwGen) *pwGen = rtbof.wGen;
        *pfMulti = rtbof.fMulti;
        if (rtbof.fMulti) {
            std::uint16_t turnFinal;
            if (cbFile < 4 || ISeekNativeFile(pf, -2, NATIVESEEKORIGIN::end) != 0 || CReadNative(std::span{&turnFinal, 1}, pf) != 1) fComplete = 0;
            else if (pturn) *pturn = turnFinal;
        }
    }
LDone:
    ICloseNativeFile(pf);
    return fComplete;
}

static std::int16_t FImportFamilyMember(const char *pszName, const char *pszStem, int *piplr, DtFileType *pdt) {
    std::size_t cch = std::strlen(pszStem);
    const char *pszExt;
    int iplrFamily;
    if (SDL_strncasecmp(pszName, pszStem, cch) || pszName[cch] != '.') return 0;
    pszExt = pszName + cch + 1;
    if (!SDL_strcasecmp(pszExt, "xy")) { *piplr = -1; *pdt = dtXY; return 1; }
    if (!SDL_strcasecmp(pszExt, "hst")) { *piplr = -1; *pdt = dtHost; return 1; }
    if (pszExt[0] == 'm' || pszExt[0] == 'M' || pszExt[0] == 'h' || pszExt[0] == 'H' || pszExt[0] == 'x' || pszExt[0] == 'X') {
        if (pszExt[1] < '1' || pszExt[1] > '9' || (pszExt[2] && (pszExt[2] < '0' || pszExt[2] > '9')) || (pszExt[2] && pszExt[3])) return 0;
        iplrFamily = pszExt[1] - '0';
        if (pszExt[2]) iplrFamily = iplrFamily * 10 + pszExt[2] - '0';
        if (iplrFamily > 16) return 0;
        *piplr = iplrFamily - 1;
        *pdt = SDL_tolower(pszExt[0]) == 'm' ? dtTurn : SDL_tolower(pszExt[0]) == 'h' ? dtHist : dtLog;
        return 1;
    }
    return 0;
}

struct LIBRARYGAMECOPY {
    const char *pszSource;
    const char *pszTarget;
    const char *pszSourceName;
    const char *pszTargetName;
    std::int16_t fBad;
};

static std::int16_t FReadLibraryMetadata(const char *pszDirectory, char *pszName, std::size_t cchName,
                                    int *piplr, long long *plPlayed,
                                    char *pszExchangeName, std::size_t cchExchange) {
    char szFile[512], szLine[256];
    NATIVEFILE *pf;
    int iVersion, cchText, cch;
    std::int16_t fRead;
    cch = CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s.library", pszDirectory, szDirSep);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szFile)) return 0;
    pf = PfOpenNativeFile(szFile, NATIVEFILEMODE::read);
    if (!pf) return 0;
    fRead = CNativeScan(pf, "%d\n", &iVersion) == 1 && CchGetNativeLine(std::span{szLine}, pf);
    if (fRead) {
        szLine[std::strcspn(szLine, "\r\n")] = 0;
        cchText = CchSnprintf(std::span{pszName, cchName}, "%s", szLine);
        fRead = cchText > 0 && static_cast<std::size_t>(cchText) < cchName &&
                CNativeScan(pf, "%d\n%lld\n", piplr, plPlayed) == 2;
    }
    if (fRead && iVersion == 2 && CchGetNativeLine(std::span{szLine}, pf)) {
        szLine[std::strcspn(szLine, "\r\n")] = 0;
        cchText = CchSnprintf(std::span{pszExchangeName, cchExchange}, "%s", szLine);
        fRead = cchText > 0 && static_cast<std::size_t>(cchText) < cchExchange;
    } else if (fRead && iVersion == 1) {
        cchText = CchSnprintf(std::span{pszExchangeName, cchExchange}, "%s", pszName);
        fRead = cchText > 0 && static_cast<std::size_t>(cchText) < cchExchange;
    } else fRead = 0;
    ICloseNativeFile(pf);
    return fRead && (iVersion == 1 || iVersion == 2) && *pszName &&
           *piplr >= -1 && *piplr < 16 && *plPlayed >= 0 && *pszExchangeName;
}

static std::int16_t FLibraryNameAvailableExcept(APPSESSION &app, const char *pszName, const char *pszExcept) {
    LIBRARYNAMECHECK lnc;
    lnc.pszDirectory = szLibraryRoot;
    lnc.pszName = pszName;
    lnc.fFound = 0;
    if (!FNativeEnumerateDirectory(app, szLibraryRoot, FindLibraryName, &lnc)) return 0;
    if (lnc.fFound && pszExcept && !SDL_strcasecmp(pszName, pszExcept)) return 1;
    return !lnc.fFound;
}

static std::int16_t FCopyLibraryGameEntry(APPSESSION &app, void *lpData, const char *pszName) {
    std::optional<NATIVEPATHINFO> npiPath;
    LIBRARYGAMECOPY *plgc = static_cast<LIBRARYGAMECOPY *>(lpData);
    char szSource[512], szTarget[512];
    const char *pszTargetFile;
    LIBRARYGAMECOPY lgb;
    int iplr;
    int cch;

    DtFileType dt;
    if (!std::strcmp(pszName, ".") || !std::strcmp(pszName, "..")) return 1;
    cch = CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s", plgc->pszSource, szDirSep, pszName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szSource)) { plgc->fBad = 1; return 0; }
    if (!(npiPath = FNativePathInfo(szSource)) || npiPath->fLink) {
        plgc->fBad = 1;
        return 0;
    }
    if (npiPath->fDirectory) {
        /* Rename removes the original tree after publication, so every directory
         * must survive staging, including backup, backupN and sidecar folders. */
        cch = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s", plgc->pszTarget, szDirSep, pszName);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget)) { plgc->fBad = 1; return 0; }
        if (!FLibraryNameAvailable(app, plgc->pszTarget, pszName) ||
            CreateNativeDir(szTarget) != 0) { plgc->fBad = 1; return 0; }
        /* Backup files use the original basename family, with unrelated sidecars retained verbatim. */
        lgb.pszSource = szSource;
        lgb.pszTarget = szTarget;
        lgb.pszSourceName = plgc->pszSourceName;
        lgb.pszTargetName = plgc->pszTargetName;
        lgb.fBad = 0;
        if (!FNativeEnumerateDirectory(app, szSource, FCopyLibraryGameEntry, &lgb) || lgb.fBad) {
            plgc->fBad = 1;
            return 0;
        }
        return 1;
    }
    if (FImportFamilyMember(pszName, plgc->pszSourceName, &iplr, &dt)) {
        if (!FFormatLibraryFamilyPath(szTarget, sizeof(szTarget), plgc->pszTarget,
                                      plgc->pszTargetName, dt, iplr)) { plgc->fBad = 1; return 0; }
    } else if (SDL_strcasecmp(pszName, ".library") && SDL_strcasecmp(pszName, ".library.tmp")) {
        /* Optional external universe definitions and other sidecars keep their own names. */
        cch = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s", plgc->pszTarget, szDirSep, pszName);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget)) { plgc->fBad = 1; return 0; }
    } else return 1;
    /* Renamed family members and unchanged sidecars can map to the same name.
     * Reject portable case collisions and never truncate an earlier staged file. */
    pszTargetFile = std::strrchr(szTarget, chDirSep);
    if (!pszTargetFile || !FLibraryNameAvailable(app, plgc->pszTarget, pszTargetFile + 1) ||
        !FCopyNativeFileExclusive(szSource, szTarget)) { plgc->fBad = 1; return 0; }
    return 1;
}

static std::int16_t FBackupDirectoryName(const char *pszName) {
    std::size_t cch = std::strlen(pszName);
    std::size_t i;

    if (std::strncmp(pszName, "backup", 6)) return 0;
    if (cch == 6) return 1;
    if (cch == 7 || cch == 8) i = 6;
    else if (cch == 10 && pszName[6] == '.') i = 7;
    else return 0;
    for (; i < cch; i++) if (pszName[i] < '0' || pszName[i] > '9') return 0;
    return 1;
}

std::int16_t CopyImportEntry(APPSESSION &, void *lpData, const char *pszName) {
    LIBRARYIMPORT *pli = static_cast<LIBRARYIMPORT *>(lpData);
    char szSource[512], szTarget[512];
    int iplr;
    DtFileType dt;
    std::int32_t lidGame;
    std::uint16_t turn;
    std::int16_t fMulti;
    std::uint16_t wGen;

    if (!FImportFamilyMember(pszName, pli->pszStem, &iplr, &dt)) {
        std::size_t cch = std::strlen(pli->pszStem);
        if (!SDL_strncasecmp(pszName, pli->pszStem, cch) && pszName[cch] == '.' &&
            (SDL_tolower(pszName[cch + 1]) == 'm' || SDL_tolower(pszName[cch + 1]) == 'h' || SDL_tolower(pszName[cch + 1]) == 'x')) pli->fBad = 1;
        if (FBackupDirectoryName(pszName)) pli->fBackup = 1;
        return 1;
    }
    CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s", pli->pszSource, szDirSep, pszName);
    if (!FFormatLibraryFamilyPath(szTarget, sizeof(szTarget), pli->pszTarget, pli->pszName, dt, iplr)) {
        pli->fBad = 1;
        return 0;
    }
    if (pli->fCopy) {
        if (!FCopyNativeFile(szSource, szTarget)) { pli->fBad = 1; return 0; }
        return 1;
    }
    if (!FInspectImportFile(szSource, dt, iplr < 0 ? iplrNone : iplr, &lidGame, &turn, &wGen, &fMulti)) {
        pli->fBad = 1;
        return 1;
    }
    if (dt == dtXY) {
        if (pli->fSawXY) { pli->fBad = 1; return 1; }
        pli->fSawXY = 1;
        if (pli->lidGame && pli->lidGame != lidGame) { pli->fBad = 1; return 1; }
        pli->lidGame = lidGame;
        pli->fValid = 1;
    } else if (dt == dtTurn) {
        if ((pli->lidGame && lidGame != pli->lidGame) || iplr >= 16 || (pli->grfPlayers & (1U << iplr))) { pli->fBad = 1; return 1; }
        if (!pli->lidGame) pli->lidGame = lidGame;
        pli->grfPlayers |= 1U << iplr;
        pli->rgturn[iplr] = static_cast<std::int16_t>(turn);
    } else if (dt == dtHost || dt == dtHist || dt == dtLog) {
        if (pli->lidGame && lidGame != pli->lidGame) { pli->fBad = 1; return 1; }
        if (!pli->lidGame) pli->lidGame = lidGame;
        if (dt == dtHost) pli->fSawHost = 1;
    }
    /* The first pass validates every recognized file; copying happens only
     * after the entire family has passed validation. */
    pli->fCopied++;
    return 1;
}

struct LIBRARYBACKUP {
    const char *pszSource;
    const char *pszTarget;
    const char *pszStem;
    const char *pszName;
    const char *pszBackup;
    std::int16_t fBad;
};

static std::int16_t CopyBackupEntry(APPSESSION &, void *lpData, const char *pszName) {
    LIBRARYBACKUP *plb = static_cast<LIBRARYBACKUP *>(lpData);
    int iplr;
    DtFileType dt;
    char szSource[512], szTarget[512], szDirectory[512];
    if (!FImportFamilyMember(pszName, plb->pszStem, &iplr, &dt)) return 1;
    CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s%s%s", plb->pszSource, szDirSep, plb->pszBackup, szDirSep, pszName);
    CchSnprintf(std::span{szDirectory, sizeof(szDirectory)}, "%s%s%s", plb->pszTarget, szDirSep, plb->pszBackup);
    if (!FFormatLibraryFamilyPath(szTarget, sizeof(szTarget), szDirectory, plb->pszName, dt, iplr)) {
        plb->fBad = 1;
        return 0;
    }
    if (!FCopyNativeFile(szSource, szTarget)) { plb->fBad = 1; return 0; }
    return 1;
}

std::int16_t CopyBackupDirectory(APPSESSION &app, void *lpData, const char *pszName) {
    LIBRARYIMPORT *pli = static_cast<LIBRARYIMPORT *>(lpData);
    LIBRARYBACKUP lb;
    char szSource[512], szTarget[512];
    if (!FBackupDirectoryName(pszName)) return 1;
    CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s", pli->pszSource, szDirSep, pszName);
    if (!FNativeDirectoryExists(szSource)) return 1;
    CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s", pli->pszTarget, szDirSep, pszName);
    if (CreateNativeDir(szTarget) != 0) { pli->fBad = 1; return 0; }
    lb.pszSource = pli->pszSource;
    lb.pszTarget = pli->pszTarget;
    lb.pszStem = pli->pszStem;
    lb.pszName = pli->pszName;
    lb.pszBackup = pszName;
    lb.fBad = 0;
    if (!FNativeEnumerateDirectory(app, szSource, CopyBackupEntry, &lb) || lb.fBad) { pli->fBad = 1; return 0; }
    return 1;
}

static std::int16_t RemoveImportChild(APPSESSION &, void *lpData, const char *pszFile) {
    char szFile[512];
    CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s%s", static_cast<char *>(lpData), szDirSep, pszFile);
    RemoveNativePath(szFile);
    return 1;
}

std::int16_t RemoveImportFile(APPSESSION &app, void *lpData, const char *pszFile) {
    char szFile[512];
    const char *pszRoot = static_cast<const char *>(lpData);
    CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s%s", pszRoot, szDirSep, pszFile);
    if (FNativeDirectoryExists(szFile)) {
        FNativeEnumerateDirectory(app, szFile, RemoveImportChild, szFile);
        RemoveNativePath(szFile);
    } else RemoveNativePath(szFile);
    return 1;
}

static std::int16_t RemoveImportTreeEntry(APPSESSION &app, void *lpData, const char *pszFile) {
    std::optional<NATIVEPATHINFO> npiPath;
    char szPath[512];

    if (CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", static_cast<const char *>(lpData), szDirSep, pszFile) <= 0 ||
        !(npiPath = FNativePathInfo(szPath))) return 0;
    if (npiPath->fDirectory && !npiPath->fLink) {
        if (!FNativeEnumerateDirectory(app, szPath, RemoveImportTreeEntry, szPath)) return 0;
    }
    return RemoveNativePath(szPath) == 0;
}

static std::int16_t FPathIsZip(const char *pszPath) {
    const char *pch = std::strrchr(pszPath, '.');
    return pch && !SDL_strcasecmp(pch, ".zip");
}

static std::int16_t FCreateLibraryZipStage(char *pszStage, std::size_t cchStage, const char *pszPurpose) {
    int i, cch;
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(std::span{pszStage, cchStage}, "%s%s.%s.%u-%u", szLibraryRoot, szDirSep,
                          pszPurpose, DwTickCount(), idLibraryZipStage++);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= cchStage) return 0;
        if (CreateNativeDir(pszStage) == 0) return 1;
        if (!FNativeDirectoryExists(pszStage)) return 0;
    }
    return 0;
}

struct LIBRARYEXPORT {
    const char *pszSource;
    const char *pszTarget;
    const char *pszStem;
    std::int16_t fBad;
    std::int16_t fSawXY;
    std::int16_t fSawPlayable;
};

static std::int16_t CopyGameExportEntry(APPSESSION &, void *lpData, const char *pszName) {
    LIBRARYEXPORT *ple = static_cast<LIBRARYEXPORT *>(lpData);
    int iplr;
    DtFileType dt;
    char szSource[512], szTarget[512];
    std::size_t cchStem = std::strlen(ple->pszStem);

    if (!FImportFamilyMember(pszName, ple->pszStem, &iplr, &dt)) {
        /* A malformed companion should not silently make an incomplete export. */
        if (!SDL_strncasecmp(pszName, ple->pszStem, cchStem) && pszName[cchStem] == '.' &&
            std::strchr("mMhHxX", pszName[cchStem + 1])) ple->fBad = 1;
        return 1;
    }
    CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s", ple->pszSource, szDirSep, pszName);
    CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s", ple->pszTarget, szDirSep, pszName);
    if (!FCopyNativeFile(szSource, szTarget)) { ple->fBad = 1; return 0; }
    if (dt == dtXY) ple->fSawXY = 1;
    if (dt == dtTurn || dt == dtHost) ple->fSawPlayable = 1;
    return 1;
}

static std::int16_t CreateLibraryExportStage(char *pszDirectory, std::size_t cchDirectory,
                                        const char *pszParent) {
    std::uint32_t dwTick = DwTickCount();
    int i, cch;
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(std::span{pszDirectory, cchDirectory}, "%s%s.stars-game-export-%u-%u",
                          pszParent, szDirSep, dwTick, idLibraryExportStage++);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= cchDirectory) return 0;
        if (CreateNativeDir(pszDirectory) == 0) return 1;
        if (!FNativeDirectoryExists(pszDirectory)) return 0;
    }
    return 0;
}

struct LIBRARYEXPORTMOVE {
    const char *pszSource;
    const char *pszTarget;
    std::int16_t fBad;
};

static std::int16_t MoveGameExportEntry(APPSESSION &, void *lpData, const char *pszName) {
    LIBRARYEXPORTMOVE *plem = static_cast<LIBRARYEXPORTMOVE *>(lpData);
    char szSource[512], szTarget[512];
    CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s", plem->pszSource, szDirSep, pszName);
    CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s", plem->pszTarget, szDirSep, pszName);
    if (RenameNativeFile(szSource, szTarget) != 0) { plem->fBad = 1; return 0; }
    return 1;
}

std::int16_t FExportLibraryGame(APPSESSION &app, HWND hwnd, const char *pszName, const char *pszTarget) {
    LIBRARYIMPORT li;
    LIBRARYEXPORT le;
    LIBRARYEXPORTMOVE lem;
    OPENFILENAME ofn;
    char szSource[512], szDestination[512], szStage[512];
    char szZipName[512], szZipStage[512], szZipGame[512], szFilter[128], szMessage[256];
    int cch, iChoice;
    std::int16_t fZipStage;

    if (!pszTarget) {
        if (!std::getenv("STARS_CLIENT_EXPORT_GAME")) {
            TextIO().pfnCchGetString(app, idsLibraryExportZipPrompt, std::span{szMessage});
            iChoice = CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage,
                                 TextIO().pfnCompressed(app, idsLibraryExportGameTitle).data(), win32::MB_YESNOCANCEL | win32::MB_ICONQUESTION | win32::MB_TASKMODAL);
            if (iChoice == IDCANCEL) return 0;
            if (iChoice == win32::IDYES) {
                ofn = {};
                std::ranges::fill(szZipName, 0);
                CchSnprintf(std::span{szZipName, sizeof(szZipName)}, "%s.zip", pszName);
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                GetLibraryFileFilter(app, idsLibraryZipFilesFilter, szFilter);
                ofn.lpstrFilter = szFilter;
                ofn.lpstrFile = szZipName;
                ofn.lpstrDefExt = "zip";
                ofn.nMaxFile = sizeof(szZipName);
                ofn.Flags = win32::OFN_HIDEREADONLY | win32::OFN_PATHMUSTEXIST | win32::OFN_OVERWRITEPROMPT;
                if (!DesktopChoiceIO().pfnFNativeSaveFileName(&ofn)) return 0;
                return FExportSelectedLibraryGame(app, hwnd, szZipName);
            }
        }
        CompatFolderIO().pfnShowOpenFolderDialog(LibraryExportFolderSelected, PvLibraryFolderContext(hwnd), nullptr, nullptr, 0);
        return 1;
    }
    if (!FInitLibrary() || !FPortableLibraryName(app, pszName)) return 0;
    cch = CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s", szLibraryRoot, szDirSep, pszName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szSource) || !FNativeDirectoryExists(szSource)) return 0;
    if (FPathIsZip(pszTarget)) {
        if (SDL_strncasecmp(pszTarget, szSource, std::strlen(szSource)) == 0 &&
            (pszTarget[std::strlen(szSource)] == 0 || pszTarget[std::strlen(szSource)] == '/' ||
             pszTarget[std::strlen(szSource)] == '\\')) goto LFail;
        fZipStage = FCreateLibraryZipStage(szZipStage, sizeof(szZipStage), "zip-exporting");
        if (!fZipStage) goto LFail;
        if (!FExportLibraryGame(app, hwnd, pszName, szZipStage)) goto LZipFail;
        cch = CchSnprintf(std::span{szZipGame, sizeof(szZipGame)}, "%s%s%s", szZipStage, szDirSep, pszName);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szZipGame) || !FZipLibraryDirectory(app, szZipGame, pszTarget)) goto LZipFail;
        FNativeEnumerateDirectory(app, szZipGame, RemoveImportTreeEntry, szZipGame);
        RemoveNativePath(szZipGame);
        RemoveNativePath(szZipStage);
        return 1;
LZipFail:
        FNativeEnumerateDirectory(app, szZipStage, RemoveImportTreeEntry, szZipStage);
        RemoveNativePath(szZipStage);
        goto LFail;
    }
    if (SDL_strncasecmp(pszTarget, szSource, std::strlen(szSource)) == 0 &&
        (pszTarget[std::strlen(szSource)] == 0 || pszTarget[std::strlen(szSource)] == '/' ||
         pszTarget[std::strlen(szSource)] == '\\')) goto LFail;
    cch = CchSnprintf(std::span{szDestination, sizeof(szDestination)}, "%s%s%s", pszTarget, szDirSep, pszName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szDestination) || FNativeDirectoryExists(szDestination)) goto LFail;
    /* The loader and backup writer keep complete paths in szBase. Check the
     * longest companion before copying, rather than truncating child paths. */
    if (std::strlen(szDestination) + std::strlen(pszName) + sizeof("/backup.999/.m16") > sizeof(app.state.szBase)) goto LFail;
    if (!CreateLibraryExportStage(szStage, sizeof(szStage), pszTarget)) goto LFail;
    if (std::strlen(szStage) + std::strlen(pszName) + sizeof("/backup.999/.m16") > sizeof(szStage)) goto LStageFail;

    le = {};
    le.pszSource = szSource;
    le.pszTarget = szStage;
    le.pszStem = pszName;
    if (!FNativeEnumerateDirectory(app, szSource, CopyGameExportEntry, &le) || le.fBad ||
        !le.fSawXY || !le.fSawPlayable) goto LStageFail;

    li = {};
    li.pszSource = szSource;
    li.pszTarget = szStage;
    li.pszStem = pszName;
    li.pszName = pszName;
    if (!FNativeEnumerateDirectory(app, szSource, CopyBackupDirectory, &li) || li.fBad ||
        CreateNativeDir(szDestination) != 0) goto LStageFail;
    lem.pszSource = szStage;
    lem.pszTarget = szDestination;
    lem.fBad = 0;
    if (!FNativeEnumerateDirectory(app, szStage, MoveGameExportEntry, &lem) || lem.fBad) {
        FNativeEnumerateDirectory(app, szDestination, RemoveImportFile, szDestination);
        RemoveNativePath(szDestination);
        goto LStageFail;
    }
    RemoveNativePath(szStage);
    return 1;

LStageFail:
    FNativeEnumerateDirectory(app, szStage, RemoveImportFile, szStage);
    RemoveNativePath(szStage);
LFail:
    if (win32::IsWindow(hwnd)) AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryGameExportFailed).data(), win32::MB_ICONHAND);
    return 0;
}

static void SDLCALL LibraryExportFolderSelected(void *lpData, const char * const *rgpszPath, int iFilter) noexcept {
    static_cast<void>(iFilter);
    try {
        if (!rgpszPath || !rgpszPath[0]) return;
        STARSCOMPATPAYLOAD path(SDL_strdup(rgpszPath[0]), {[](void *pv) noexcept { SDL_free(pv); }});
        StarsCompatPostOwnedMessageAsync(reinterpret_cast<std::uintptr_t>(lpData), WM_STARS_EXPORT_FOLDER, std::move(path), 0);
    } catch (...) {
        // The foreign dialog boundary cannot propagate a C++ failure.
    }
}

std::int16_t FExportSelectedLibraryGame(APPSESSION &app, HWND hwnd, const char *pszTarget) {
    char szDir[512];
    char szName[256];
    if (!FSelectedLibraryGameName(hwnd, szName, sizeof(szName))) return 0;
    if (!pszTarget) return FExportLibraryGame(app, hwnd, szName, nullptr);
    if (app.state.game.fDirty && app.state.lpPlanets && app.state.idPlayer != iplrNone) {
        CchSnprintf(std::span{szDir, sizeof(szDir)}, "%s%s%s", szLibraryRoot, szDirSep, szName);
        if (LibraryPathIO().pfnUnder(szDir, app.state.szBase)) PromptSaveGame(app);
    }
    return FExportLibraryGame(app, hwnd, szName, pszTarget);
}

void ShowLibraryExportFolderPicker(HWND hwnd) {
    CompatFolderIO().pfnShowOpenFolderDialog(LibraryExportFolderSelected, PvLibraryFolderContext(hwnd), nullptr, nullptr, 0);
}

static std::int16_t FindImportUniverse(APPSESSION &, void *lpData, const char *pszName) {
    char *pszStem = static_cast<char *>(lpData);
    const char *pch = std::strrchr(pszName, '.');
    if (!pch || SDL_strcasecmp(pch, ".xy")) return 1;
    if (pszStem[0]) { pszStem[0] = '!'; return 1; }
    if (static_cast<std::size_t>((pch - pszName)) >= 256) { pszStem[0] = '!'; return 1; }
    std::memcpy(pszStem, pszName, pch - pszName);
    pszStem[pch - pszName] = 0;
    return 1;
}

struct LIBRARYTURNMATCH {
    std::int32_t lidGame;
    int cMatches;
    char rgszName[64][256];
    char szSelected[256];
};

static std::int16_t FindLibraryTurnMatch(APPSESSION &app, void *lpData, const char *pszName) {
    LIBRARYTURNMATCH *plm = static_cast<LIBRARYTURNMATCH *>(lpData);
    char szUniverse[512], szMetadata[512];
    NATIVEFILE *pf;
    std::int32_t lidGame;
    std::uint16_t turn;
    std::int16_t fMulti;
    if (!FPortableLibraryName(app, pszName)) return 1;
    CchSnprintf(std::span{szMetadata, sizeof(szMetadata)}, "%s%s%s%s.library", szLibraryRoot, szDirSep, pszName, szDirSep);
    pf = PfOpenNativeFile(szMetadata, NATIVEFILEMODE::read);
    if (!pf) return 1;
    ICloseNativeFile(pf);
    CchSnprintf(std::span{szUniverse, sizeof(szUniverse)}, "%s%s%s%s%s.xy", szLibraryRoot, szDirSep, pszName, szDirSep, pszName);
    if (!FInspectImportFile(szUniverse, dtXY, iplrNone, &lidGame, &turn, nullptr, &fMulti) || lidGame != plm->lidGame) return 1;
    if (plm->cMatches < static_cast<int>((sizeof(plm->rgszName) / sizeof(plm->rgszName[0]))))
        CchSnprintf(std::span{plm->rgszName[plm->cMatches], sizeof(plm->rgszName[0])}, "%s", pszName);
    plm->cMatches++;
    return 1;
}

INT_PTR  LibraryTurnTargetDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LIBRARYTURNMATCH *plm;
    HWND hwndList;
    int i;
    switch (message) {
    case win32::WM_INITDIALOG:
        win32::SetWindowLongPtrA(hwnd, win32::DWLP_USER, lParam);
        plm = reinterpret_cast<LIBRARYTURNMATCH *>(lParam);
        hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_TURN_TARGET_LIST);
        for (i = 0; i < plm->cMatches; i++) ControlIO().pfnSendMessage(hwndList, win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(plm->rgszName[i]));
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDOK), 0);
        return 1;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) { ControlIO().pfnEndDialog(hwnd, 0); return 1; }
        if (LOWORD(wParam) == IDC_LIBRARY_TURN_TARGET_LIST && HIWORD(wParam) == win32::LBN_SELCHANGE) {
            hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_TURN_TARGET_LIST);
            ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDOK), ControlIO().pfnSendMessage(hwndList, win32::LB_GETCURSEL, 0, 0) != win32::LB_ERR);
            return 1;
        }
        if (LOWORD(wParam) == IDOK) {
            plm = reinterpret_cast<LIBRARYTURNMATCH *>(win32::GetWindowLongPtrA(hwnd, win32::DWLP_USER));
            hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_TURN_TARGET_LIST);
            i = static_cast<int>(ControlIO().pfnSendMessage(hwndList, win32::LB_GETCURSEL, 0, 0));
            if (i == win32::LB_ERR) return 1;
            ControlIO().pfnSendMessage(hwndList, win32::LB_GETTEXT, i, reinterpret_cast<LPARAM>(plm->szSelected));
            ControlIO().pfnEndDialog(hwnd, 1);
            return 1;
        }
        break;
    }
    return 0;
}

INT_PTR  LibraryTurnConflictDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static_cast<void>(lParam);
    switch (message) {
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL) { ControlIO().pfnEndDialog(hwnd, 0); return 1; }
        if (LOWORD(wParam) == IDC_LIBRARY_TURN_REPLACE) { ControlIO().pfnEndDialog(hwnd, 1); return 1; }
        if (LOWORD(wParam) == IDC_LIBRARY_TURN_SEPARATE) { ControlIO().pfnEndDialog(hwnd, 2); return 1; }
        break;
    }
    return 0;
}

static std::int16_t FFilesEqual(const char *pszLeft, const char *pszRight) {
    NATIVEFILE *pfLeft, *pfRight;
    char rgbLeft[4096], rgbRight[4096];
    std::size_t cbLeft, cbRight;
    std::int16_t fEqual = 1;
    pfLeft = PfOpenNativeFile(pszLeft, NATIVEFILEMODE::read);
    pfRight = PfOpenNativeFile(pszRight, NATIVEFILEMODE::read);
    if (!pfLeft || !pfRight) { fEqual = 0; goto LDone; }
    do {
        cbLeft = CReadNative(std::span{rgbLeft}, pfLeft);
        cbRight = CReadNative(std::span{rgbRight}, pfRight);
        if (cbLeft != cbRight || std::memcmp(rgbLeft, rgbRight, cbLeft)) { fEqual = 0; break; }
    } while (cbLeft != 0);
    if (FNativeStreamError(pfLeft) || FNativeStreamError(pfRight)) fEqual = 0;
LDone:
    if (pfLeft) ICloseNativeFile(pfLeft);
    if (pfRight) ICloseNativeFile(pfRight);
    return fEqual;
}

std::int16_t FReplaceConflictingLibraryTurn(APPSESSION &app, const LIBRARYPATHS &paths, HWND hwnd, const char *pszName,
                                             const char *pszSource, const char *pszTarget,
                                             int iplr, std::int32_t lidGame, std::uint16_t turn);
std::int16_t FImportConflictingTurnAsCopy(APPSESSION &app, const LIBRARYPATHS &paths, HWND hwnd, const char *pszName,
                                            const char *pszSource, int iplr,
                                            std::int32_t lidGame, std::uint16_t turn);

static std::int16_t FInspectImportPlayer(APPSESSION &app, const char *pszUniverse, int iplr);

static std::int16_t FImportSubsequentTurn(APPSESSION &app, HWND hwnd, const char *pszSource) {
    char szUniverse[512], szTarget[512];
    const char *pszFile, *pszExt, *pchSlash;
    LIBRARYTURNMATCH lm;
    std::int32_t lidGame, lidCurrent;
    std::uint16_t turn, turnCurrent;
    std::int16_t fMulti, fMultiCurrent;
    INT_PTR iChoice;
    int iplr;
    std::int16_t fHasTurn;
    NATIVEFILE *pf;
    int cch;

    pszFile = std::strrchr(pszSource, '/');
    pchSlash = std::strrchr(pszSource, '\\');
    if (pchSlash && (!pszFile || pchSlash > pszFile)) pszFile = pchSlash;
    pszFile = pszFile ? pszFile + 1 : pszSource;
    pszExt = std::strrchr(pszFile, '.');
    if (!pszExt || (SDL_tolower(pszExt[1]) != 'm' && SDL_tolower(pszExt[1]) != 'M') ||
        pszExt[2] < '1' || pszExt[2] > '9' || (pszExt[3] && (pszExt[3] < '0' || pszExt[3] > '9')) ||
        (pszExt[3] && pszExt[4])) return 0;
    iplr = pszExt[2] - '0';
    if (pszExt[3]) iplr = iplr * 10 + pszExt[3] - '0';
    iplr--;
    if (iplr < 0 || iplr >= 16) return 0;
    if (!FInspectImportFile(pszSource, dtTurn, iplr, &lidGame, &turn, nullptr, &fMulti)) {
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryTurnInvalid).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    lm = {};
    lm.lidGame = lidGame;
    if (!FNativeEnumerateDirectory(app, szLibraryRoot, FindLibraryTurnMatch, &lm)) return 0;
    if (lm.cMatches == 0) return -1;
    if (lm.cMatches > 1) {
        if (lm.cMatches > static_cast<int>((sizeof(lm.rgszName) / sizeof(lm.rgszName[0]))) ||
            ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_LIBRARY_TURN_TARGET), hwnd,
                           LibraryTurnTargetDlg, reinterpret_cast<LPARAM>(&lm)) != 1) return 0;
    } else CchSnprintf(std::span{lm.szSelected, sizeof(lm.szSelected)}, "%s", lm.rgszName[0]);
    if (!lm.szSelected[0]) {
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryTurnChooseGame).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    cch = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s%s%s.m%d", szLibraryRoot, szDirSep, lm.szSelected, szDirSep, lm.szSelected, iplr + 1);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget)) return 0;
    pf = PfOpenNativeFile(szTarget, NATIVEFILEMODE::read);
    fHasTurn = pf != nullptr;
    if (pf) ICloseNativeFile(pf);
    else if (errno != ENOENT) {
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryTurnInspectFailed).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    if (fHasTurn) {
        if (!FInspectImportFile(szTarget, dtTurn, iplr, &lidCurrent, &turnCurrent, nullptr, &fMultiCurrent) || lidCurrent != lidGame) {
            AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryTurnCurrentInvalid).data(), win32::MB_ICONEXCLAMATION);
            return 0;
        }
        if (FFilesEqual(pszSource, szTarget)) return 1;
    } else {
        /* A first turn uses the stored universe, but must name an existing
         * player and must not replace a dangling link or unreadable entry. */
        cch = CchSnprintf(std::span{szUniverse, sizeof(szUniverse)}, "%s%s%s%s%s.xy",
                          szLibraryRoot, szDirSep, lm.szSelected, szDirSep, lm.szSelected);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szUniverse)) return 0;
        if (FNativePathInfo(szTarget) || !FInspectImportPlayer(app, szUniverse, iplr)) {
            AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryTurnPlayerInvalid).data(), win32::MB_ICONEXCLAMATION);
            return 0;
        }
    }
    if (fHasTurn && turn <= turnCurrent) {
        iChoice = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_LIBRARY_TURN_CONFLICT), hwnd,
                                 LibraryTurnConflictDlg, 0);
        if (iChoice == 1) return FReplaceConflictingLibraryTurn(app, {szLibraryRoot,szLibraryPending,szLibraryName}, hwnd, lm.szSelected, pszSource,
                                                               szTarget, iplr, lidGame, turn);
        if (iChoice == 2) return FImportConflictingTurnAsCopy(app, {szLibraryRoot,szLibraryPending,szLibraryName}, hwnd, lm.szSelected, pszSource,
                                                             iplr, lidGame, turn);
        return 0;
    }
    return FPublishSubsequentTurn(app, {szLibraryRoot,szLibraryPending,szLibraryName}, hwnd,
                                  pszSource,szTarget,lm.szSelected,iplr,lidGame,turn,fHasTurn);
}

static void SDLCALL LibraryImportFolderSelected(void *lpData, const char * const *rgpszPath, int iFilter) noexcept {
    static_cast<void>(iFilter);
    try {
        if (!rgpszPath || !rgpszPath[0]) return;
        STARSCOMPATPAYLOAD path(SDL_strdup(rgpszPath[0]), {[](void *pv) noexcept { SDL_free(pv); }});
        StarsCompatPostOwnedMessageAsync(reinterpret_cast<std::uintptr_t>(lpData), WM_STARS_IMPORT_FOLDER, std::move(path), 0);
    } catch (...) {
        // The foreign dialog boundary cannot propagate a C++ failure.
    }
}

void ShowLibraryImportFolderPicker(HWND hwnd) {
    CompatFolderIO().pfnShowOpenFolderDialog(LibraryImportFolderSelected, PvLibraryFolderContext(hwnd), nullptr, nullptr, 0);
}

static std::int16_t FChooseLibraryImportFile(APPSESSION &app, HWND hwnd, char *pszChosen, std::size_t cchChosen) {
    OPENFILENAME ofn;
    char szTitle[256], szFilter[128], *pch;

    ofn = {};
    std::fill_n(pszChosen, cchChosen, 0);
    TextIO().pfnCchGetString(app, idsStarsGameFilesMHstRStars, std::span{szFilter});
    pch = std::strstr(szFilter, "*.M*;");
    if (pch) {
        std::memmove(pch + 11, pch, std::strlen(pch) + 1);
        std::memcpy(pch, "*.ZIP;*.XY;", 11);
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
    ofn.Flags = win32::OFN_HIDEREADONLY | win32::OFN_PATHMUSTEXIST | win32::OFN_FILEMUSTEXIST;
    return DesktopChoiceIO().pfnFNativeOpenFileName(&ofn);
}

/* Choose the workflow from the BOF type, leaving complete validation and
 * source-preserving publication to the corresponding importer. */
std::int16_t FImportDesktopFile(APPSESSION &app, HWND hwnd, const char *pszSource) {
    char szChosen[512];
    NATIVEFILE *pf;
    HDR hdr;
    RTBOF rtbof;
    std::int16_t fRace;

    if (!pszSource) {
        if (!FChooseLibraryImportFile(app, hwnd, szChosen, sizeof(szChosen))) return 0;
        pszSource = szChosen;
    }
    fRace = 0;
    pf = PfOpenNativeFile(pszSource, NATIVEFILEMODE::read);
    if (pf) {
        fRace = FReadLibraryHdr(pf, &hdr) && hdr.rt == rtBOF && hdr.cb == cbWireBof &&
                FReadLibraryBof(pf, &rtbof) && !std::memcmp(rtbof.rgid, "J3J3", 4) && rtbof.dt == dtRace;
        ICloseNativeFile(pf);
    }
    return fRace ? FImportLibraryRaceAndEdit(app, hwnd, pszSource) : FImportLibraryGame(app, hwnd, pszSource);
}

std::int16_t FImportLibraryGame(APPSESSION &app, HWND hwnd, const char *pszSource) {
    char szChosen[512];
    char szSource[512], szDir[512], szStem[256], *pch, *pchSlash, *pchName;
    char szZipStage[512];
    LIBRARYIMPORT li;
    int cchDir;
    std::optional<std::int64_t> cbSource;
    std::int16_t fImported, iZipResult;

    if (!pszSource) {
        if (!FChooseLibraryImportFile(app, hwnd, szChosen, sizeof(szChosen))) return 0;
        pszSource = szChosen;
    }
    if (std::strlen(pszSource) >= sizeof(szSource)) return 0;
    std::ranges::copy_n(pszSource, std::strlen(pszSource) + 1, std::span{szSource}.begin());
    /* Validate the selected path before either standalone-turn or family import. */
    if (!FNativeDirectoryExists(szSource) &&
        (!(cbSource = FNativeFileSize(szSource)) || (*cbSource) <= 0)) goto LInvalidSource;
    if (FPathIsZip(szSource)) {
        if (!FInitLibrary()) return 0;
        if (!FCreateLibraryZipStage(szZipStage, sizeof(szZipStage), "zip-importing")) return 0;
        iZipResult = FUnzipLibraryArchive(szSource, szZipStage);
        if (iZipResult != ZIP_IMPORT_OK) {
            FNativeEnumerateDirectory(app, szZipStage, RemoveImportTreeEntry, szZipStage);
            RemoveNativePath(szZipStage);
            if (iZipResult == ZIP_IMPORT_UNSUPPORTED_NAME) {
                AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryZipNameEncoding).data(), win32::MB_ICONEXCLAMATION);
                return 0;
            }
            AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryZipInvalid).data(), win32::MB_ICONEXCLAMATION);
            return 0;
        }
        fImported = FImportLibraryGame(app, hwnd, szZipStage);
        FNativeEnumerateDirectory(app, szZipStage, RemoveImportTreeEntry, szZipStage);
        RemoveNativePath(szZipStage);
        return fImported;
    }
    if (std::strrchr(pszSource, '.') && SDL_tolower(std::strrchr(pszSource, '.')[1]) == 'm') {
        if (!FInitLibrary()) return 0;
        fImported = FImportSubsequentTurn(app, hwnd, pszSource);
        if (fImported >= 0) return fImported;
    }
    if (FNativeDirectoryExists(szSource)) {
        CchCopyUtf8(std::span{szDir}, szSource);
        szStem[0] = 0;
        FNativeEnumerateDirectory(app, szDir, FindImportUniverse, szStem);
        if (!szStem[0] || szStem[0] == '!') {
            AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryUniverseRequired).data(), win32::MB_ICONEXCLAMATION);
            return 0;
        }
        CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s.xy", szDir, szDirSep, szStem);
        pchSlash = std::strrchr(szSource, chDirSep);
    } else {
        pchSlash = std::strrchr(szSource, '/');
        pch = std::strrchr(szSource, '\\');
        if (pch && (!pchSlash || pch > pchSlash)) pchSlash = pch;
        if (pchSlash) {
            cchDir = static_cast<int>((pchSlash - szSource));
            if (cchDir == 0) cchDir = 1;
            std::memcpy(szDir, szSource, cchDir);
            szDir[cchDir] = 0;
            pchName = pchSlash + 1;
        } else {
            CchCopyUtf8(std::span{szDir}, ".");
            pchName = szSource;
        }
        pch = std::strrchr(pchName, '.');
        if (!pch || pch == pchName || static_cast<std::size_t>((pch - pchName)) >= sizeof(szStem)) goto LInvalidSource;
        std::memcpy(szStem, pchName, pch - pchName);
        szStem[pch - pchName] = 0;
    }
    if (!FInitLibrary()) return 0;
    li = {};
    li.pszStem = szStem;
    li.pszSource = szDir;
    li.fValid = 0;
    if (!FNativeEnumerateDirectory(app, szDir, CopyImportEntry, &li) || li.fBad || !li.fSawXY || !li.fValid ||
        (li.grfPlayers == 0 && !li.fSawHost)) {
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryGameIncomplete).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    if (!FChooseLibraryGameName(app, hwnd, szSource, nullptr)) return 0;
    CchCopyUtf8(std::span{szLibraryExchangeName}, szStem);
    return FPublishImportedLibraryGame(app, {szLibraryRoot,szLibraryPending,szLibraryName},li,szDir);
LInvalidSource:
    CchSnprintf(std::span{app.state.szWork, sizeof(app.state.szWork)}, "%s", szSource);
    AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsCantOpenFile, {}).data(), win32::MB_ICONHAND);
    return 0;
}

struct LIBRARYGAMEREMOVE {
    const char *pszDirectory;
    std::int16_t fFailed;
};

std::int16_t RemoveLibraryGameTree(APPSESSION &app, const char *pszDirectory);

static std::int16_t RemoveLibraryGameEntry(APPSESSION &app, void *lpData, const char *pszName) {
    std::optional<NATIVEPATHINFO> npiPath;
    LIBRARYGAMEREMOVE *plgr = static_cast<LIBRARYGAMEREMOVE *>(lpData);
    char szPath[512];

    int cch = CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", plgr->pszDirectory, szDirSep, pszName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szPath) || !(npiPath = FNativePathInfo(szPath))) {
        plgr->fFailed = 1;
        return 0;
    }
    /* A link belongs to this tree; its target does not. */
    if (npiPath->fDirectory && !npiPath->fLink) {
        if (!RemoveLibraryGameTree(app, szPath)) {
            plgr->fFailed = 1;
            return 0;
        }
        return 1;
    }
    if ((npiPath->fDirectory || npiPath->fLink ? RemoveNativePath(szPath) : RemoveNativeFile(szPath)) != 0) {
        plgr->fFailed = 1;
        return 0;
    }
    return 1;
}

std::int16_t RemoveLibraryGameTree(APPSESSION &app, const char *pszDirectory) {
    std::optional<NATIVEPATHINFO> npiPath;
    LIBRARYGAMEREMOVE lgr;

    if (!(npiPath = FNativePathInfo(pszDirectory))) return 0;
    if (npiPath->fDirectory && !npiPath->fLink) {
        lgr.pszDirectory = pszDirectory;
        lgr.fFailed = 0;
        if (!FNativeEnumerateDirectory(app, pszDirectory, RemoveLibraryGameEntry, &lgr) || lgr.fFailed) return 0;
    }
    return RemoveNativePath(pszDirectory) == 0;
}

std::int16_t FCreateLibraryGameStage(APPSESSION &app, const char *pszSource, const char *pszSourceName,
                                       const char *pszTargetName, const char *pszStage,
                                       int iplr, long long lPlayed, const char *pszExchangeName) {
    LIBRARYGAMECOPY lgc;
    char szSourceUniverse[512];
    std::optional<std::int64_t> cbFile;
    int cch;
    lgc.pszSource = pszSource;
    lgc.pszTarget = pszStage;
    lgc.pszSourceName = pszSourceName;
    lgc.pszTargetName = pszTargetName;
    lgc.fBad = 0;
    cch = CchSnprintf(std::span{szSourceUniverse, sizeof(szSourceUniverse)}, "%s%s%s.xy", pszSource, szDirSep, pszSourceName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szSourceUniverse)) return 0;
    if (!(cbFile = FNativeFileSize(szSourceUniverse)) || (*cbFile) <= 0 ||
        !FNativeEnumerateDirectory(app, pszSource, FCopyLibraryGameEntry, &lgc) || lgc.fBad ||
        !FWriteLibraryMetadataValues(pszStage, pszTargetName, iplr, lPlayed, pszExchangeName)) return 0;
    return 1;
}

std::int16_t FCreateLibraryGameStagePath(char *pszStage, std::size_t cchStage) {
    std::uint32_t dwTick = DwTickCount();
    int i, cch;
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(std::span{pszStage, cchStage}, "%s%s.stars-game-stage-%u-%u",
                          szLibraryRoot, szDirSep, dwTick, i);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= cchStage) return 0;
        if (CreateNativeDir(pszStage) == 0) return 1;
        if (!FNativeDirectoryExists(pszStage)) return 0;
    }
    return 0;
}

std::int16_t FCreateLibraryGameHoldPath(char *pszHold, std::size_t cchHold) {

    std::uint32_t dwTick = DwTickCount();
    int i, cch;
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(std::span{pszHold, cchHold}, "%s%s.stars-game-old-%u-%u",
                          szLibraryRoot, szDirSep, dwTick, i);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= cchHold) return 0;
        if (!FNativeDirectoryExists(pszHold) && !FNativeFileSize(pszHold)) return 1;
    }
    return 0;
}

std::int16_t FValidManagedLibraryGame(APPSESSION &app, const char *pszName, char *pszDirectory,
                                        int *piplr, long long *plPlayed,
                                        char *pszExchangeName, std::size_t cchExchange) {
    std::optional<NATIVEPATHINFO> npiPath;
    char szMetadataName[256];
    int cchDirectory;

    if (!pszName || !FInitLibrary() || !FPortableLibraryName(app, pszName) ||
        (cchDirectory = CchSnprintf(std::span{pszDirectory, 512}, "%s%s%s", szLibraryRoot, szDirSep, pszName)) <= 0 ||
        static_cast<std::size_t>(cchDirectory) >= 512 ||
        !(npiPath = FNativePathInfo(pszDirectory)) || !npiPath->fDirectory || npiPath->fLink ||
        !FReadLibraryMetadata(pszDirectory, szMetadataName, sizeof(szMetadataName), piplr,
                              plPlayed, pszExchangeName, cchExchange)) return 0;
    return !SDL_strcasecmp(szMetadataName, pszName);
}

std::int16_t FDuplicateLibraryGame(APPSESSION &app, HWND hwnd, const char *pszName, const char *pszCopyName) {
    char szSource[512], szStage[512], szTarget[512], szExchangeName[256], szMessage[256];
    int iplr;
    int cchTarget;
    long long lPlayed;
    std::int16_t fResult = 0;
    static_cast<void>(hwnd);
    if (!pszCopyName || !FValidManagedLibraryGame(app, pszName, szSource, &iplr, &lPlayed,
                                                  szExchangeName, sizeof(szExchangeName)) ||
        !FPortableLibraryName(app, pszCopyName) || !FLibraryNameAvailableExcept(app, pszCopyName, nullptr) ||
        (cchTarget = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s", szLibraryRoot, szDirSep, pszCopyName)) <= 0 ||
        static_cast<std::size_t>(cchTarget) >= sizeof(szTarget) || std::strlen(szLibraryRoot) + 2 * std::strlen(pszCopyName) + 18 >= sizeof(app.state.szBase) ||
        !FCreateLibraryGameStagePath(szStage, sizeof(szStage))) return 0;
    if (FCreateLibraryGameStage(app, szSource, pszName, pszCopyName, szStage, -1, 0, szExchangeName) &&
        RenameNativeFile(szStage, szTarget) == 0) fResult = 1;
    if (!fResult) {
        RemoveLibraryGameTree(app, szStage);
        CchCopyUtf8Prefix(std::span{szMessage}, TextIO().pfnCompressed(app, idsLibraryStorageFailed).data());
        CompatChoiceIO().pfnMessageBoxA(win32::IsWindow(hwnd) ? hwnd : app.lpui->hwndFrame, szMessage, TextIO().pfnCompressed(app, idsLibraryGames).data(), win32::MB_OK | win32::MB_ICONHAND);
    }
    return fResult;
}

std::int16_t FRenameLibraryGame(APPSESSION &app, HWND hwnd, const char *pszName, const char *pszNewName) {
    char szSource[512], szStage[512], szTarget[512], szHold[512], szExchangeName[256], szMessage[768];
    int iplr;
    long long lPlayed;
    std::int16_t fResult = 0, fHeld = 0, fRestoreFailed = 0, fHoldCleanupFailed = 0;
    int cchTarget;
    static_cast<void>(hwnd);
    if (!pszNewName || !FValidManagedLibraryGame(app, pszName, szSource, &iplr, &lPlayed,
                                                 szExchangeName, sizeof(szExchangeName)) ||
        !FPortableLibraryName(app, pszNewName) || !FLibraryNameAvailableExcept(app, pszNewName, pszName) ||
        (cchTarget = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s", szLibraryRoot, szDirSep, pszNewName)) <= 0 ||
        static_cast<std::size_t>(cchTarget) >= sizeof(szTarget) || std::strlen(szLibraryRoot) + 2 * std::strlen(pszNewName) + 18 >= sizeof(app.state.szBase) ||
        !FCreateLibraryGameStagePath(szStage, sizeof(szStage))) return 0;
    if (!FCreateLibraryGameStage(app, szSource, pszName, pszNewName, szStage, iplr, lPlayed, szExchangeName) ||
        !FCreateLibraryGameHoldPath(szHold, sizeof(szHold)) || RenameNativeFile(szSource, szHold) != 0) goto LDone;
    fHeld = 1;
    if (RenameNativeFile(szStage, szTarget) != 0) {
        if (RenameNativeFile(szHold, szSource) == 0) fHeld = 0;
        else fRestoreFailed = 1;
        goto LDone;
    }
    fHeld = 0;
    fResult = 1;
    if (!RemoveLibraryGameTree(app, szHold)) fHoldCleanupFailed = 1;
LDone:
    if (!fResult && FNativeDirectoryExists(szStage)) RemoveLibraryGameTree(app, szStage);
    if (fHeld) {
        if (RenameNativeFile(szHold, szSource) == 0) { fHeld = 0; fRestoreFailed = 0; }
        else fRestoreFailed = 1;
    }
    if (!fResult) {
        if (fRestoreFailed) {
            cchTarget = CchSnprintf(std::span{szMessage, sizeof(szMessage)},
                                    TextIO().pfnCompressed(app, idsLibraryRenameFailedOriginalAt).data(), szHold);
            if (cchTarget <= 0 || static_cast<std::size_t>(cchTarget) >= sizeof(szMessage)) CchCopyUtf8Prefix(std::span{szMessage}, TextIO().pfnCompressed(app, idsLibraryStorageFailed).data());
        } else CchCopyUtf8Prefix(std::span{szMessage}, TextIO().pfnCompressed(app, idsLibraryStorageFailed).data());
        CompatChoiceIO().pfnMessageBoxA(win32::IsWindow(hwnd) ? hwnd : app.lpui->hwndFrame, szMessage, TextIO().pfnCompressed(app, idsLibraryGames).data(), win32::MB_OK | win32::MB_ICONHAND);
    } else if (fHoldCleanupFailed) {
        cchTarget = CchSnprintf(std::span{szMessage, sizeof(szMessage)},
                                TextIO().pfnCompressed(app, idsLibraryRenameSucceededCleanupPath).data(), szHold);
        if (cchTarget <= 0 || static_cast<std::size_t>(cchTarget) >= sizeof(szMessage)) CchCopyUtf8Prefix(std::span{szMessage}, TextIO().pfnCompressed(app, idsLibraryStorageFailed).data());
        CompatChoiceIO().pfnMessageBoxA(win32::IsWindow(hwnd) ? hwnd : app.lpui->hwndFrame, szMessage, TextIO().pfnCompressed(app, idsLibraryGames).data(), win32::MB_OK | win32::MB_ICONEXCLAMATION);
    }
    return fResult;
}

std::int16_t FDeleteLibraryGameRecentPaths(APPSESSION &app, const char *pszDirectory) {
    char szIniFile[32], szSection[16], szEntry[16], szPath[512];
    char rgpszRecent[9][256];
    const char *pszRecent;
    int i, j, cch;
    std::int16_t fContinueRemoved = 0;

    if (!app.lpui->vrgszMRU) return 1;
    for (i = 0; i < 9; i++) {
        pszRecent = app.lpui->vrgszMRU + 256 * i;
        cch = CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s", pszRecent);
        if (cch < 0 || static_cast<std::size_t>(cch) >= sizeof(szPath)) return 0;
        if (LibraryPathIO().pfnUnder(pszDirectory, szPath)) {
            rgpszRecent[i][0] = 0;
            if (i == 0) fContinueRemoved = 1;
        } else {
            CchSnprintf(std::span{rgpszRecent[i], sizeof(rgpszRecent[i])}, "%s", pszRecent);
        }
    }
    /* Do not promote a different recent game into Continue when File1 was deleted. */
    if (!fContinueRemoved) {
        for (i = 0, j = 0; i < 9; i++) {
            if (!rgpszRecent[i][0]) continue;
            if (i != j) CchCopyUtf8(std::span{rgpszRecent[j]}, rgpszRecent[i]);
            j++;
        }
        while (j < 9) rgpszRecent[j++][0] = 0;
    }
    TextIO().pfnCchGetString(app, idsStarsIni, std::span{szIniFile});
    TextIO().pfnCchGetString(app, idsFiles, std::span{szSection});
    TextIO().pfnCchGetString(app, idsFile1, std::span{szEntry});
    for (i = 0; i < 9; i++) {
        szEntry[std::strlen(szEntry) - 1] = static_cast<char>(('1' + i));
        CchCopyUtf8(std::span{app.lpui->vrgszMRU, std::size_t{2304}}.subspan(256 * i, 256), rgpszRecent[i]);
        if (!ProfileTextIO().pfnWriteString(szSection, szEntry, rgpszRecent[i], szIniFile)) return 0;
    }
    if (fContinueRemoved && !ProfileTextIO().pfnWriteString(szSection, "File1", "", szIniFile)) return 0;
    return 1;
}

std::int16_t FDeleteLibraryGame(APPSESSION &app, HWND hwnd, const char *pszName) {
    char szDirectory[512], szMetadata[512], szMessage[512], szMetadataName[256];
    char szExchangeName[256];
    int iplr;
    long long lPlayed;
    int cch;
    std::int16_t fContinueTarget;

    if (!FInitLibrary() || !FPortableLibraryName(app, pszName)) return 0;
    cch = CchSnprintf(std::span{szDirectory, sizeof(szDirectory)}, "%s%s%s", szLibraryRoot, szDirSep, pszName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szDirectory)) return 0;
    cch = CchSnprintf(std::span{szMetadata, sizeof(szMetadata)}, "%s%s.library", szDirectory, szDirSep);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szMetadata) ||
        !FReadLibraryMetadata(szDirectory, szMetadataName, sizeof(szMetadataName), &iplr, &lPlayed,
                              szExchangeName, sizeof(szExchangeName)) || std::strcmp(szMetadataName, pszName)) return 0;
    cch = CchSnprintf(std::span{szMessage, sizeof(szMessage)}, TextIO().pfnCompressed(app, idsLibraryDeleteGamePrompt).data(), pszName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szMessage)) return 0;
    if (CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage, TextIO().pfnCompressed(app, idsLibraryGames).data(),
                   win32::MB_YESNO | win32::MB_ICONQUESTION | win32::MB_TASKMODAL) != win32::IDYES) return 0;
    fContinueTarget = FFindContinueLibraryGame(app) && !std::strcmp(continueLibrary.szName, pszName);
    if (!RemoveLibraryGameTree(app, szDirectory)) {
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryDeleteGameFailed).data(), win32::MB_ICONHAND);
        return 0;
    }
    /* Keep deletion from silently promoting another game on the next startup. */
    if (fContinueTarget)
        ProfileTextIO().pfnWriteString("Library", "ContinueDeleted", "1", TextIO().pfnCompressed(app, idsStarsIni).data());
    if (!FDeleteLibraryGameRecentPaths(app, szDirectory))
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryDeleteGameRecentFailed).data(), win32::MB_ICONEXCLAMATION);
    return 1;
}

std::int16_t FDeleteSelectedLibraryGame(APPSESSION &app, HWND hwnd) {
    char szName[256];
    if (!FSelectedLibraryGameName(hwnd, szName, sizeof(szName))) return 0;
    return FDeleteLibraryGame(app, hwnd, szName);
}

static std::int16_t FInspectImportPlayer(APPSESSION &app, const char *pszUniverse, int iplr) {
    NATIVEFILE *pf;
    HDR hdr;
    RTBOF rtbof;
    GAME gameImport;
    std::uint8_t rgbGame[cbWireGame];
    std::int32_t lSeed1, lSeed2;
    std::int16_t fRead;

    pf = PfOpenNativeFile(pszUniverse, NATIVEFILEMODE::read);
    if (!pf) return 0;
    fRead = FReadLibraryHdr(pf, &hdr) && hdr.rt == rtBOF && hdr.cb == cbWireBof &&
            FReadLibraryBof(pf, &rtbof) &&
            FReadLibraryHdr(pf, &hdr) && hdr.rt == rtGame && hdr.cb == cbWireGame &&
            CReadNative(std::span{&rgbGame, 1}, pf) == 1;
    ICloseNativeFile(pf);
    if (!fRead) return 0;
    /* Inspect the selected universe without disturbing an open game's stream. */
    GetFileSeeds(app, &lSeed1, &lSeed2);
    SetFileXorStream(app, rtbof.lidGame, rtbof.lSaltTime, rtbof.turn, rtbof.iPlayer, rtbof.fCrippled);
    XorFileBuf(app, reinterpret_cast<char *>(rgbGame), cbWireGame);
    SetFileSeeds(app, lSeed1, lSeed2);
    FDecodeGame(std::as_bytes(std::span(rgbGame)), gameImport);
    return gameImport.lid == rtbof.lidGame && gameImport.cPlayer > 0 && gameImport.cPlayer <= 16 &&
           iplr >= 0 && iplr < gameImport.cPlayer;
}

std::int16_t FImportLibraryOrders(APPSESSION &app, HWND hwnd, const char *pszSource) {
    OPENFILENAME ofn;
    char szChosen[512], szFilter[128], szName[256];
    char szHost[512], szUniverse[512], szTarget[512], szStage[512], szMessage[512];
    const char *pszFile, *pszExt, *pchSlash;
    char *pch;
    std::int32_t lidHost, lidUniverse, lidOrders;
    std::uint16_t turnHost, turnOrders, wGenHost, wGenOrders;
    std::int16_t fMulti;
    int iplr, cch;

    std::uint32_t dwTick;
    HWND hwndList;
    LRESULT i;

    if (!pszSource) {
        ofn = {};
        std::ranges::fill(szChosen, 0);
        CchCopyUtf8(std::span{szFilter}, TextIO().pfnCompressed(app, idsLibraryOrdersImportFilter).data());
        for (pch = szFilter; *pch; pch++) if (*pch == '|') *pch = 0;
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = szFilter;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = szChosen;
        ofn.nMaxFile = sizeof(szChosen);
        ofn.Flags = win32::OFN_HIDEREADONLY | win32::OFN_PATHMUSTEXIST | win32::OFN_FILEMUSTEXIST;
        if (!DesktopChoiceIO().pfnFNativeOpenFileName(&ofn)) return 0;
        pszSource = szChosen;
    }
    if (!FInitLibrary() || !win32::IsWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_HOST_GAMES))) return 0;
    hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_HOST_GAMES);
    i = ControlIO().pfnSendMessage(hwndList, win32::LB_GETCURSEL, 0, 0);
    if (i == win32::LB_ERR) {
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryOrdersChooseHost).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    ControlIO().pfnSendMessage(hwndList, win32::LB_GETTEXT, i, reinterpret_cast<LPARAM>(szName));
    pszFile = std::strrchr(pszSource, '/');
    pchSlash = std::strrchr(pszSource, '\\');
    if (pchSlash && (!pszFile || pchSlash > pszFile)) pszFile = pchSlash;
    pszFile = pszFile ? pszFile + 1 : pszSource;
    pszExt = std::strrchr(pszFile, '.');
    if (!pszExt || SDL_tolower(pszExt[1]) != 'x' || pszExt[2] < '1' || pszExt[2] > '9' ||
        (pszExt[3] && (pszExt[3] < '0' || pszExt[3] > '9')) || (pszExt[3] && pszExt[4])) {
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryOrdersChooseFile).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    iplr = pszExt[2] - '1';
    if (pszExt[3]) iplr = (pszExt[2] - '0') * 10 + pszExt[3] - '1';
    if (iplr < 0 || iplr >= 16 || !FInspectImportFile(pszSource, dtLog, iplr, &lidOrders,
            &turnOrders, &wGenOrders, &fMulti)) {
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryOrdersInvalid).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    cch = CchSnprintf(std::span{szHost, sizeof(szHost)}, "%s%s%s%s%s.hst", szLibraryRoot, szDirSep, szName, szDirSep, szName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szHost)) return 0;
    cch = CchSnprintf(std::span{szUniverse, sizeof(szUniverse)}, "%s%s%s%s%s.xy", szLibraryRoot, szDirSep, szName, szDirSep, szName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szUniverse)) return 0;
    if (!FInspectImportFile(szHost, dtHost, iplrNone, &lidHost, &turnHost, &wGenHost, &fMulti) ||
        !FInspectImportFile(szUniverse, dtXY, iplrNone, &lidUniverse, nullptr, nullptr, &fMulti) ||
        lidHost != lidUniverse || lidOrders != lidHost || turnOrders != turnHost || wGenOrders != wGenHost ||
        !FInspectImportPlayer(app, szUniverse, iplr)) {
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryOrdersMismatch).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    cch = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s%s%s.x%d", szLibraryRoot, szDirSep,
                      szName, szDirSep, szName, iplr + 1);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget)) return 0;
    if (FNativeFileSize(szTarget)) {
        if (FFilesEqual(pszSource, szTarget)) return 1;
        cch = CchSnprintf(std::span{szMessage, sizeof(szMessage)},
                          TextIO().pfnCompressed(app, idsLibraryOrdersReplacePrompt).data(), iplr + 1, szName);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szMessage)) return 0;
        if (AlertIO().pfnAlertSz(app, szMessage, win32::MB_YESNO | win32::MB_ICONQUESTION) != win32::IDYES) return 0;
    }
    dwTick = DwTickCount();
    cch = CchSnprintf(std::span{szStage, sizeof(szStage)}, "%s.importing.%u", szTarget, dwTick);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szStage)) return 0;
    if (!FCopyNativeFileExclusive(pszSource, szStage) ||
        !FInspectImportFile(szStage, dtLog, iplr, &lidOrders, &turnOrders, &wGenOrders, &fMulti) ||
        lidOrders != lidHost || turnOrders != turnHost || wGenOrders != wGenHost) {
        RemoveNativeFile(szStage);
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryOrdersStageFailed).data(), win32::MB_ICONHAND);
        return 0;
    }
    if (ReplaceNativeFile(szStage, szTarget) != 0) {
        RemoveNativeFile(szStage);
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryOrdersInstallFailed).data(), win32::MB_ICONHAND);
        return 0;
    }
    static_cast<void>(hwnd);
    return 1;
}


static std::int16_t FCopyOptionalConflictFile(const char *pszSource, const char *pszTarget) {

    if (FNativeFileSize(pszSource)) return FCopyNativeFileExclusive(pszSource, pszTarget);
    return errno == ENOENT;
}

std::int16_t FBackupConflictingPlayerFiles(const char *pszDirectory, const char *pszName,
                                             int iplr, char *pszBackup, std::size_t cchBackup) {
    char szSource[512], szTarget[512], szExt[8];
    int i, cch;


    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(std::span{pszBackup, cchBackup}, "%s%sbackup.%03d", pszDirectory, szDirSep, i);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= cchBackup) return 0;
        if (FNativePathInfo(pszBackup)) continue;
        if (CreateNativeDir(pszBackup) == 0) break;
        if (!FNativeDirectoryExists(pszBackup)) return 0;
    }
    if (i == 1000) return 0;

    CchSnprintf(std::span{szExt, sizeof(szExt)}, "m%d", iplr + 1);
    cch = CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s.%s", pszDirectory, szDirSep, pszName, szExt);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szSource)) return 0;
    cch = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s.%s", pszBackup, szDirSep, pszName, szExt);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget) || !FCopyOptionalConflictFile(szSource, szTarget)) return 0;

    CchSnprintf(std::span{szExt, sizeof(szExt)}, "x%d", iplr + 1);
    cch = CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s.%s", pszDirectory, szDirSep, pszName, szExt);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szSource)) return 0;
    cch = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s.%s", pszBackup, szDirSep, pszName, szExt);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget) || !FCopyOptionalConflictFile(szSource, szTarget)) return 0;

    CchSnprintf(std::span{szExt, sizeof(szExt)}, "h%d", iplr + 1);
    cch = CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s.%s", pszDirectory, szDirSep, pszName, szExt);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szSource)) return 0;
    cch = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s.%s", pszBackup, szDirSep, pszName, szExt);
    return cch > 0 && static_cast<std::size_t>(cch) < sizeof(szTarget) && FCopyOptionalConflictFile(szSource, szTarget);
}

std::int16_t FStageConflictingTurn(const char *pszStage, const char *pszName,
                                     const char *pszSource, int iplr,
                                     std::int32_t lidGame, std::uint16_t turn) {
    char szTarget[512], szIncoming[512];
    std::int32_t lidStaged;
    std::uint16_t turnStaged, wGenStaged;
    std::int16_t fMultiStaged;
    int cch;

    cch = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s.m%d", pszStage, szDirSep, pszName, iplr + 1);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget)) return 0;
    cch = CchSnprintf(std::span{szIncoming, sizeof(szIncoming)}, "%s.incoming.%u", szTarget, DwTickCount());
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szIncoming)) return 0;
    if (!FCopyNativeFileExclusive(pszSource, szIncoming) ||
        !FInspectImportFile(szIncoming, dtTurn, iplr, &lidStaged, &turnStaged, &wGenStaged, &fMultiStaged) ||
        lidStaged != lidGame || turnStaged != turn || ReplaceNativeFile(szIncoming, szTarget) != 0) {
        RemoveNativeFile(szIncoming);
        return 0;
    }
    return 1;
}

std::int16_t FChooseSeparateCopyName(APPSESSION &app, HWND hwnd, const char *pszName) {
    if (!FInitLibrary()) return 0;
    CchSnprintf(std::span{szLibraryName, sizeof(szLibraryName)}, "%s", pszName);
    SuggestLibraryName(app, std::span{szLibraryName}, 0);
    if (IdPromptLibraryName(app, hwnd, std::span{szLibraryName}) != 1) return 0;
    return FPortableLibraryName(app, szLibraryName) && FLibraryNameAvailable(app, szLibraryRoot, szLibraryName);
}

std::int16_t FReplaceConflictingLibraryTurn(APPSESSION &app, const LIBRARYPATHS &paths, HWND hwnd, const char *pszName,
                                             const char *pszSource, const char *pszTarget,
                                             int iplr, std::int32_t lidGame, std::uint16_t turn) {
    char szDirectory[512], szStage[512], szHold[512], szBackup[512], szExchangeName[256];
    char szMessage[768];
    int iRemembered;
    long long lPlayed;
    std::int16_t fHeld = 0, fRestoreFailed = 0, fCleanupFailed = 0;
    int cch;

    cch = LibraryPublicationIO().pfnCchSnprintf(std::span{szDirectory, sizeof(szDirectory)}, "%s%s%s", paths.szLibraryRoot, szDirSep, pszName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szDirectory) ||
        !LibraryPublicationIO().pfnFValidManagedLibraryGame(app, pszName, szDirectory, &iRemembered, &lPlayed,
                                  szExchangeName, sizeof(szExchangeName)) ||
        !LibraryPublicationIO().pfnFCreateLibraryGameStagePath(szStage, sizeof(szStage))) return 0;
    if (!LibraryPublicationIO().pfnFCreateLibraryGameStage(app, szDirectory, pszName, pszName, szStage, iRemembered,
                                 lPlayed, szExchangeName) ||
        !LibraryPublicationIO().pfnFBackupConflictingPlayerFiles(szStage, pszName, iplr, szBackup, sizeof(szBackup)) ||
        !LibraryPublicationIO().pfnFStageConflictingTurn(szStage, pszName, pszSource, iplr, lidGame, turn) ||
        !LibraryPublicationIO().pfnFCreateLibraryGameHoldPath(szHold, sizeof(szHold))) goto LDone;
    if (RenameNativeFile(szDirectory, szHold) != 0) goto LDone;
    fHeld = 1;
    if (RenameNativeFile(szStage, szDirectory) != 0) {
        if (RenameNativeFile(szHold, szDirectory) == 0) fHeld = 0;
        else fRestoreFailed = 1;
        goto LDone;
    }
    fHeld = 0;
    if (!LibraryPublicationIO().pfnRemoveLibraryGameTree(app, szHold)) fCleanupFailed = 1;
    if (fCleanupFailed) {
        LibraryPublicationIO().pfnCchSnprintf(std::span{szMessage, sizeof(szMessage)}, TextIO().pfnCompressed(app, idsLibraryTurnCleanupFailed).data(), szHold);
        AlertIO().pfnAlertSz(app, szMessage, win32::MB_ICONEXCLAMATION);
    }
    static_cast<void>(hwnd);
    static_cast<void>(pszTarget);
    return 1;
LDone:
    if (LibraryPublicationIO().pfnFNativeDirectoryExists(szStage)) LibraryPublicationIO().pfnRemoveLibraryGameTree(app, szStage);
    if (fHeld) {
        if (RenameNativeFile(szHold, szDirectory) == 0) { fHeld = 0; fRestoreFailed = 0; }
        else fRestoreFailed = 1;
    }
    if (fRestoreFailed) {
        LibraryPublicationIO().pfnCchSnprintf(std::span{szMessage, sizeof(szMessage)}, TextIO().pfnCompressed(app, idsLibraryGameRestoreFailed).data(), szHold);
        AlertIO().pfnAlertSz(app, szMessage, win32::MB_ICONHAND);
    } else AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryTurnReplaceFailed).data(), win32::MB_ICONHAND);
    static_cast<void>(hwnd);
    static_cast<void>(pszTarget);
    return 0;
}

std::int16_t FImportConflictingTurnAsCopy(APPSESSION &app, const LIBRARYPATHS &paths, HWND hwnd, const char *pszName,
                                            const char *pszSource, int iplr,
                                            std::int32_t lidGame, std::uint16_t turn) {
    char szDirectory[512], szStage[512], szTarget[512], szExchangeName[256];
    int iRemembered, cch;
    long long lPlayed;

    cch = LibraryPublicationIO().pfnCchSnprintf(std::span{szDirectory, sizeof(szDirectory)}, "%s%s%s", paths.szLibraryRoot, szDirSep, pszName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szDirectory) ||
        !LibraryPublicationIO().pfnFValidManagedLibraryGame(app, pszName, szDirectory, &iRemembered, &lPlayed,
                                  szExchangeName, sizeof(szExchangeName)) ||
        !LibraryPublicationIO().pfnFChooseSeparateCopyName(app, hwnd, pszName)) return 0;
    cch = LibraryPublicationIO().pfnCchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s", paths.szLibraryRoot, szDirSep, paths.szLibraryName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget) ||
        !LibraryPublicationIO().pfnFLibraryNameAvailable(app, paths.szLibraryRoot, paths.szLibraryName) ||
        LibraryPublicationIO().pfnPathLength(paths.szLibraryRoot) + 2 * LibraryPublicationIO().pfnPathLength(paths.szLibraryName) + 18 >= sizeof(app.state.szBase) ||
        !LibraryPublicationIO().pfnFCreateLibraryGameStagePath(szStage, sizeof(szStage))) return 0;
    if (!LibraryPublicationIO().pfnFCreateLibraryGameStage(app, szDirectory, pszName, paths.szLibraryName, szStage,
                                 iplr, 0, szExchangeName) ||
        !LibraryPublicationIO().pfnFStageConflictingTurn(szStage, paths.szLibraryName, pszSource, iplr, lidGame, turn) ||
        RenameNativeFile(szStage, szTarget) != 0) {
        if (LibraryPublicationIO().pfnFNativeDirectoryExists(szStage)) LibraryPublicationIO().pfnRemoveLibraryGameTree(app, szStage);
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibrarySeparateCopyFailed).data(), win32::MB_ICONHAND);
        return 0;
    }
    static_cast<void>(iRemembered);
    static_cast<void>(lPlayed);
    return 1;
}

std::int16_t FFinishLibraryGame(APPSESSION &app, HWND hwnd, std::int16_t fGenerated) {
    char szFile[256], szMessage[256];
    int i;

    if (!szLibraryPending[0]) return 0;
    for (i = -2; fGenerated && i < app.state.game.cPlayer; i++) {
        if (i == -2) CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s.xy", app.state.szBase);
        else if (i == -1) CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s.hst", app.state.szBase);
        else CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s.m%d", app.state.szBase, i + 1);
        if (!FCompleteGeneratedFile(app, szFile, i == -2 ? dtXY : i == -1 ? dtHost : dtTurn, i < 0 ? iplrNone : i)) fGenerated = 0;
    }
    if (fGenerated) fGenerated = FWriteLibraryMetadata(szLibraryPending, szLibraryName, app.state.idPlayer);
    if (!fGenerated) {
        FNativeEnumerateDirectory(app, szLibraryPending, RemovePendingFile, nullptr);
        RemoveNativePath(szLibraryPending);
        CchCopyUtf8Prefix(std::span{szMessage}, TextIO().pfnCompressed(app, idsLibraryStorageFailed).data());
        CompatChoiceIO().pfnMessageBoxA(win32::IsWindow(hwnd) ? hwnd : app.lpui->hwndFrame, szMessage, TextIO().pfnCompressed(app, idsLibraryGames).data(), win32::MB_OK | win32::MB_ICONHAND);
    }
    szLibraryPending[0] = 0;
    return fGenerated;
}

static std::int16_t FAddLibraryGameListItem(HWND hwndList, const char *pszName, int iplr,
                                       std::uint16_t grfPlayers, const std::uint16_t *rgturn) {
    LRESULT i;
    int cNew;
    LIBRARYGAMELISTITEM *rgNew;
    if (cLibraryGameList == cLibraryGameListMax) {
        cNew = cLibraryGameListMax ? cLibraryGameListMax * 2 : 32;
        rgNew = static_cast<LIBRARYGAMELISTITEM *>(SDL_realloc(rgLibraryGameList.get(), static_cast<std::size_t>(cNew) * sizeof(*rgNew)));
        if (!rgNew) return 0;
        rgNew = LpStartHeapArray<LIBRARYGAMELISTITEM>(rgNew,static_cast<std::size_t>(cNew) * sizeof(*rgNew));
        rgLibraryGameList.release();
        rgLibraryGameList.reset(rgNew);
        cLibraryGameListMax = cNew;
    }
    i = ControlIO().pfnSendMessage(hwndList, win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pszName));
    if (i == win32::LB_ERR || i == win32::LB_ERRSPACE) return 0;
    CchSnprintf(std::span{rgLibraryGameList[cLibraryGameList].szName, sizeof(rgLibraryGameList[cLibraryGameList].szName)}, "%s", pszName);
    rgLibraryGameList[cLibraryGameList].iplr = iplr;
    rgLibraryGameList[cLibraryGameList].grfPlayers = grfPlayers;
    std::memcpy(rgLibraryGameList[cLibraryGameList].rgturn, rgturn, sizeof(rgLibraryGameList[cLibraryGameList].rgturn));
    ControlIO().pfnSendMessage(hwndList, win32::LB_SETITEMDATA, i, cLibraryGameList++);
    return 1;
}

static void ClearLibraryGameList(void) {
    rgLibraryGameList.reset();
    cLibraryGameList = 0;
    cLibraryGameListMax = 0;
}

static std::int16_t ListLibraryGame(APPSESSION &app, void *lpList, const char *pszName) {
    char szFile[256], szDirectory[256], szMetadataName[256], szExchangeName[256];
    int iplr, iplrSaved, cch;
    std::optional<std::int64_t> cbFile;
    long long lPlayed;
    std::int32_t lidGame;
    std::uint16_t turn;
    std::uint16_t grfPlayers = 0, rgturn[16] = {0};
    std::int16_t fMulti;

    if (!FPortableLibraryName(app, pszName)) return 1;
    cch = CchSnprintf(std::span{szDirectory, sizeof(szDirectory)}, "%s%s%s", szLibraryRoot, szDirSep, pszName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szDirectory) ||
        !FReadLibraryMetadata(szDirectory, szMetadataName, sizeof(szMetadataName), &iplrSaved, &lPlayed,
                              szExchangeName, sizeof(szExchangeName))) return 1;
    CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s%s%s%s.xy", szLibraryRoot, szDirSep, pszName, szDirSep, pszName);
    if (!(cbFile = FNativeFileSize(szFile)) || (*cbFile) == 0) return 1;
    for (iplr = 0; iplr < 16; iplr++) {
        CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s%s%s%s.m%d", szLibraryRoot, szDirSep, pszName, szDirSep, pszName, iplr + 1);
        if (!(cbFile = FNativeFileSize(szFile)) || (*cbFile) <= 0 ||
            !FInspectImportFile(szFile, dtTurn, iplr, &lidGame, &turn, nullptr, &fMulti)) continue;
        grfPlayers |= static_cast<std::uint16_t>((1u << iplr));
        rgturn[iplr] = turn;
    }
    return !grfPlayers || FAddLibraryGameListItem(static_cast<HWND>(lpList), pszName, iplrSaved, grfPlayers, rgturn);
}

static std::int16_t ListLibraryHostGame(APPSESSION &app, void *lpList, const char *pszName) {
    char szHost[512], szUniverse[512], szMetadata[512];
    NATIVEFILE *pf;
    std::optional<std::int64_t> cbFile;
    std::int32_t lidHost, lidUniverse;
    std::uint16_t turnHost;
    std::int16_t fMulti;
    LRESULT i;

    if (!FPortableLibraryName(app, pszName)) return 1;
    CchSnprintf(std::span{szMetadata, sizeof(szMetadata)}, "%s%s%s%s.library", szLibraryRoot, szDirSep, pszName, szDirSep);
    pf = PfOpenNativeFile(szMetadata, NATIVEFILEMODE::read);
    if (!pf) return 1;
    ICloseNativeFile(pf);
    CchSnprintf(std::span{szHost, sizeof(szHost)}, "%s%s%s%s%s.hst", szLibraryRoot, szDirSep, pszName, szDirSep, pszName);
    CchSnprintf(std::span{szUniverse, sizeof(szUniverse)}, "%s%s%s%s%s.xy", szLibraryRoot, szDirSep, pszName, szDirSep, pszName);
    if (!(cbFile = FNativeFileSize(szHost)) || (*cbFile) <= 0 ||
        !FInspectImportFile(szHost, dtHost, iplrNone, &lidHost, &turnHost, nullptr, &fMulti) ||
        !FInspectImportFile(szUniverse, dtXY, iplrNone, &lidUniverse, nullptr, nullptr, &fMulti) ||
        lidHost != lidUniverse) return 1;
    i = ControlIO().pfnSendMessage(static_cast<HWND>(lpList), win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pszName));
    return i != win32::LB_ERR && i != win32::LB_ERRSPACE;
}

static std::int16_t FindContinueLibraryGame(APPSESSION &app, void *lpData, const char *pszName) {
    LIBRARYCONTINUE *plc = static_cast<LIBRARYCONTINUE *>(lpData);
    char szFile[512], szMetadataName[256];
    NATIVEFILE *pf;
    int iVersion, iPlayer;
    long long lPlayed;
    std::int32_t lidGame;
    std::uint16_t turn;
    std::int16_t fMulti;
    int iYear;
    std::int16_t fTurnAvailable;

    if (!FPortableLibraryName(app, pszName)) return 1;
    CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s%s%s.library", szLibraryRoot, szDirSep, pszName, szDirSep);
    pf = PfOpenNativeFile(szFile, NATIVEFILEMODE::read);
    if (!pf) return 1;
    iVersion = 0;
    iPlayer = -1;
    lPlayed = 0;
    if (CNativeScan(pf, "%d\n", &iVersion) != 1 || !CchGetNativeLine(std::span{szMetadataName}, pf) ||
        CNativeScan(pf, "%d\n%lld", &iPlayer, &lPlayed) != 2) {
        ICloseNativeFile(pf);
        return 1;
    }
    ICloseNativeFile(pf);
    szMetadataName[std::strcspn(szMetadataName, "\r\n")] = 0;
    if ((iVersion != 1 && iVersion != 2) || _stricmp(szMetadataName, pszName) ||
        iPlayer < 0 || iPlayer >= 16 || (plc->fFound && lPlayed <= plc->lPlayed)) return 1;

    fTurnAvailable = 0;
    iYear = 0;
    CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s%s%s%s.m%d", szLibraryRoot, szDirSep, pszName, szDirSep,
                pszName, iPlayer + 1);
    if (FInspectImportFile(szFile, dtTurn, iPlayer, &lidGame, &turn, nullptr, &fMulti)) {
        iYear = turn + 2400;
        fTurnAvailable = 1;
    }
    CchCopyUtf8(std::span{plc->szName}, pszName);
    plc->iPlayer = iPlayer;
    plc->iYear = iYear;
    plc->lPlayed = lPlayed;
    plc->fFound = 1;
    plc->fTurnAvailable = fTurnAvailable;
    return 1;
}

static std::int16_t FFindContinueLibraryGame(APPSESSION &app) {
    if (win32::GetPrivateProfileIntA("Library", "ContinueDeleted", 0, TextIO().pfnCompressed(app, idsStarsIni).data())) return 0;
    continueLibrary = {};
    continueLibrary.iPlayer = -1;
    return FInitLibrary() && FNativeEnumerateDirectory(app, szLibraryRoot, FindContinueLibraryGame, &continueLibrary) &&
           continueLibrary.fFound;
}

void InitLibraryGames(APPSESSION &app, HWND hwnd, const RECT *lprc) {
    int y;
    Uint64 msStart = CompatTimeIO().pfnGetTicks();

    StarsCompatSetWindowAlertSounds(hwnd, 0);
    fHostLibraryView = 0;
    if (std::getenv("STARS_DEBUG_LIBRARY_STARTUP"))
        std::fprintf(stderr, "[DEBUG-library-startup] InitLibraryGames begin hwnd=%p\n", static_cast<void *>(hwnd));
    y = StarsMax(50, lprc->bottom - 360);
    ui::CreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(app, idsLibraryGames).data(), win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                 24, y - 22, 58, 18, hwnd,
                 reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_GAMES_VIEW)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(app, idsLibraryHostGames).data(), win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                 86, y - 22, 82, 18, hwnd,
                 reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_HOST_VIEW)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(app, idsLibraryImportGame).data(), win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                 lprc->right - 128, y - 22, 104, 18, hwnd,
                 reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_IMPORT)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(app, idsLibraryImportFolder).data(), win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                 lprc->right - 240, y - 22, 104, 18, hwnd,
                 reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_IMPORT_FOLDER)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(app, idsLibraryImportOrders).data(), win32::WS_CHILD | win32::WS_TABSTOP,
                 lprc->right - 128, y - 22, 104, 18, hwnd,
                 reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_IMPORT_ORDERS)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(app, idsLibraryExportGame).data(), win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                 lprc->right - 464, y - 22, 96, 18, hwnd,
                 reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_GAME_EXPORT)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(app, idsLibraryDuplicate).data(), win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                 lprc->right - 304, y - 22, 60, 18, hwnd,
                 reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_DUPLICATE)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(app, idsRename).data(), win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                 lprc->right - 360, y - 22, 52, 18, hwnd,
                 reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_RENAME)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(app, idsDelete).data(), win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                 lprc->right - 524, y - 22, 56, 18, hwnd,
                 reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_GAME_DELETE)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "LISTBOX", "", win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_BORDER | win32::WS_TABSTOP | win32::WS_VSCROLL | win32::LBS_NOTIFY | win32::LBS_SORT,
                 24, y, lprc->right - 48, 90, hwnd, reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_GAMES)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "LISTBOX", "", win32::WS_CHILD | win32::WS_BORDER | win32::WS_TABSTOP | win32::WS_VSCROLL | win32::LBS_NOTIFY | win32::LBS_SORT,
                 24, y, lprc->right - 48, 90, hwnd, reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_HOST_GAMES)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "STATIC", TextIO().pfnCompressed(app, idsLibraryPlayerSelect).data(), win32::WS_CHILD | win32::WS_VISIBLE,
                 24, y + 100, 66, 18, hwnd, reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_PLAYER_LABEL)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "COMBOBOX", "", win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP | win32::WS_VSCROLL | win32::CBS_DROPDOWNLIST,
                 94, y + 96, StarsMin(280, lprc->right - 118), 160, hwnd,
                 reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_PLAYERS)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "STATIC", "", win32::WS_CHILD | win32::WS_VISIBLE,
                 24, y + 122, lprc->right - 48, 18, hwnd,
                 reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_CONTINUE_DETAILS)), app.lpui->hInst, nullptr);
    ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_IMPORT_ORDERS), win32::SW_HIDE);
    ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_HOST_GAMES), win32::SW_HIDE);
    RefreshLibraryGames(app, hwnd);
    RefreshLibraryHostGames(app, hwnd);
    if (std::getenv("STARS_DEBUG_LIBRARY_STARTUP"))
        std::fprintf(stderr, "[DEBUG-library-startup] InitLibraryGames end elapsed=%llu ms\n",
                static_cast<unsigned long long>((CompatTimeIO().pfnGetTicks() - msStart)));
}

struct LIBRARYRACEENUM {
    FNativeDirectoryEntryProc pfnEntry;
    void *lpData;
};

static std::int16_t EnumLibraryRace(APPSESSION &app, void *lpData, const char *pszName) {
    LIBRARYRACEENUM *plre = static_cast<LIBRARYRACEENUM *>(lpData);
    char szPath[512];
    std::optional<std::int64_t> cbFile;
    if (!FPortableRaceFilename(app, pszName)) return 1;
    CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", szRaceLibraryRoot, szDirSep, pszName);
    if (!(cbFile = FNativeFileSize(szPath)) || (*cbFile) <= 0) return 1;
    return plre->pfnEntry(app, plre->lpData, pszName);
}

std::int16_t FEnumerateLibraryRaces(APPSESSION &app, FNativeDirectoryEntryProc pfnEntry, void *lpData) {
    LIBRARYRACEENUM lre = {pfnEntry, lpData};

    return FInitRaceLibrary() && FNativeEnumerateDirectory(app, szRaceLibraryRoot, EnumLibraryRace, &lre);
}

static std::int16_t ListLibraryRace(APPSESSION &, void *lpList, const char *pszName) {
    LRESULT i;

    i = ControlIO().pfnSendMessage(static_cast<HWND>(lpList), win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pszName));
    return i != win32::LB_ERR && i != win32::LB_ERRSPACE;
}

void InitLibraryRaces(APPSESSION &app, HWND hwnd, const RECT *lprc) {
    int y = StarsMax(160, lprc->bottom - 200);
    ui::CreateWindowEx(0, "STATIC", TextIO().pfnCompressed(app, idsLibraryRaceDesigns).data(), win32::WS_CHILD | win32::WS_VISIBLE, 24, y - 18, 160, 18,
                 hwnd, nullptr, app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(app, idsLibraryNew).data(), win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                 lprc->right - 436, y - 20, 48, 18, hwnd, reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_RACE_NEW)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(app, idsLibraryEdit).data(), win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                 lprc->right - 384, y - 20, 48, 18, hwnd, reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_RACE_EDIT)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(app, idsLibraryDuplicate).data(), win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                 lprc->right - 332, y - 20, 62, 18, hwnd, reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_RACE_DUPLICATE)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(app, idsLibraryExport).data(), win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                 lprc->right - 266, y - 20, 48, 18, hwnd, reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_RACE_EXPORT)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(app, idsDelete).data(), win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                 lprc->right - 214, y - 20, 56, 18, hwnd, reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_RACE_DELETE)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "BUTTON", TextIO().pfnCompressed(app, idsLibraryImport).data(), win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_TABSTOP,
                 lprc->right - 152, y - 20, 112, 18, hwnd, reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_RACE_IMPORT)), app.lpui->hInst, nullptr);
    ui::CreateWindowEx(0, "LISTBOX", "", win32::WS_CHILD | win32::WS_VISIBLE | win32::WS_BORDER | win32::WS_TABSTOP | win32::WS_VSCROLL | win32::LBS_NOTIFY | win32::LBS_SORT,
                 24, y, lprc->right - 48, 80, hwnd, reinterpret_cast<HMENU>(static_cast<std::uintptr_t>(IDC_LIBRARY_RACES)), app.lpui->hInst, nullptr);
    RefreshLibraryRaces(app, hwnd);
}

void RefreshLibraryRaces(APPSESSION &app, HWND hwnd) {
    HWND hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RACES);
    if (!hwndList) return;
    ControlIO().pfnSendMessage(hwndList, win32::LB_RESETCONTENT, 0, 0);
    FEnumerateLibraryRaces(app, ListLibraryRace, hwndList);
    ControlIO().pfnSendMessage(hwndList, win32::LB_SETCURSEL, 0, 0);
    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RACE_EDIT), ControlIO().pfnSendMessage(hwndList, win32::LB_GETCOUNT, 0, 0) > 0);
    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RACE_DUPLICATE), ControlIO().pfnSendMessage(hwndList, win32::LB_GETCOUNT, 0, 0) > 0);
    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RACE_EXPORT), ControlIO().pfnSendMessage(hwndList, win32::LB_GETCOUNT, 0, 0) > 0);
    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RACE_DELETE), ControlIO().pfnSendMessage(hwndList, win32::LB_GETCOUNT, 0, 0) > 0);
}

std::int16_t FSelectLibraryRace(HWND hwnd, char *pszName) {
    HWND hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RACES);
    LRESULT i = ControlIO().pfnSendMessage(hwndList, win32::LB_GETCURSEL, 0, 0);
    if (i == win32::LB_ERR) return 0;
    ControlIO().pfnSendMessage(hwndList, win32::LB_GETTEXT, i, reinterpret_cast<LPARAM>(pszName));
    return 1;
}

void RefreshLibraryPlayers(APPSESSION &app, HWND hwnd) {
    HWND hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAMES);
    HWND hwndPlayers = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_PLAYERS);
    LRESULT i, iItem, iPlayer, iSelected = win32::CB_ERR;
    int iplr;
    char szPlayer[64];
    LIBRARYGAMELISTITEM *plgli;

    if (!hwndPlayers) return;
    ControlIO().pfnSendMessage(hwndPlayers, win32::CB_RESETCONTENT, 0, 0);
    i = ControlIO().pfnSendMessage(hwndList, win32::LB_GETCURSEL, 0, 0);
    iItem = i == win32::LB_ERR ? win32::LB_ERR : ControlIO().pfnSendMessage(hwndList, win32::LB_GETITEMDATA, i, 0);
    if (iItem >= 0 && iItem < cLibraryGameList) {
        plgli = &rgLibraryGameList[iItem];
        for (iplr = 0; iplr < 16; iplr++) {
            if (!(plgli->grfPlayers & (1u << iplr))) continue;
            CchSnprintf(std::span{szPlayer, sizeof(szPlayer)}, TextIO().pfnCompressed(app, idsLibraryPlayerTurn).data(),
                        iplr + 1, plgli->rgturn[iplr] + 2400);
            iPlayer = ControlIO().pfnSendMessage(hwndPlayers, win32::CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(szPlayer));
            if (iPlayer == win32::CB_ERR || iPlayer == win32::CB_ERRSPACE) break;
            ControlIO().pfnSendMessage(hwndPlayers, win32::CB_SETITEMDATA, iPlayer, iplr);
            if (iSelected == win32::CB_ERR || iplr == plgli->iplr) iSelected = iPlayer;
        }
    }
    ControlIO().pfnSendMessage(hwndPlayers, win32::CB_SETCURSEL, iSelected, 0);
    ControlIO().pfnEnableWindow(hwndPlayers, iSelected != win32::CB_ERR);
    if (!fHostLibraryView) ControlIO().pfnEnableWindow(app.lpui->rghwndBtnSplash[1], iSelected != win32::CB_ERR);
}

void RefreshLibraryGames(APPSESSION &app, HWND hwnd) {
    HWND hwndList;
    char szLabel[320], szSelected[256] = "";
    std::int16_t fLibrary;
    std::int16_t fEnumerated;
    std::int16_t fHasGames;
    std::int16_t fContinue;
    LRESULT iContinue, iSelected, iPlayer, iItem;
    hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAMES);
    if (!hwndList) return;
    iSelected = ControlIO().pfnSendMessage(hwndList, win32::LB_GETCURSEL, 0, 0);
    if (iSelected != win32::LB_ERR) ControlIO().pfnSendMessage(hwndList, win32::LB_GETTEXT, iSelected, reinterpret_cast<LPARAM>(szSelected));
    iPlayer = ui::SendDlgItemMessage(hwnd, IDC_LIBRARY_PLAYERS, win32::CB_GETCURSEL, 0, 0);
    if (iPlayer != win32::CB_ERR) iPlayer = ui::SendDlgItemMessage(hwnd, IDC_LIBRARY_PLAYERS, win32::CB_GETITEMDATA, iPlayer, 0);
    ControlIO().pfnSendMessage(hwndList, win32::LB_RESETCONTENT, 0, 0);
    ClearLibraryGameList();
    fLibrary = FInitLibrary();
    if (fLibrary) {
        fEnumerated = FNativeEnumerateDirectory(app, szLibraryRoot, ListLibraryGame, hwndList);
        static_cast<void>(fEnumerated);
    }
    hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAMES);
    if (hwndList) {
        iSelected = szSelected[0] ? ControlIO().pfnSendMessage(hwndList, win32::LB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(szSelected)) : win32::LB_ERR;
        if (iSelected == win32::LB_ERR) iSelected = 0;
        else if (iPlayer >= 0 && iPlayer < 16) {
            iItem = ControlIO().pfnSendMessage(hwndList, win32::LB_GETITEMDATA, iSelected, 0);
            if (iItem >= 0 && iItem < cLibraryGameList && (rgLibraryGameList[iItem].grfPlayers & (1u << iPlayer)))
                rgLibraryGameList[iItem].iplr = static_cast<int>(iPlayer);
        }
        ControlIO().pfnSendMessage(hwndList, win32::LB_SETCURSEL, iSelected, 0);
    }
    fHasGames = hwndList && ControlIO().pfnSendMessage(hwndList, win32::LB_GETCOUNT, 0, 0) > 0;
    if (!fHostLibraryView) {
        ControlIO().pfnEnableWindow(app.lpui->rghwndBtnSplash[1], fHasGames);
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAME_EXPORT), fHasGames);
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_DUPLICATE), fHasGames);
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RENAME), fHasGames);
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAME_DELETE), fHasGames);
    }
    RefreshLibraryPlayers(app, hwnd);
    fContinue = FFindContinueLibraryGame(app) && continueLibrary.iPlayer >= 0;
    if (fContinue && !continueLibrary.fTurnAvailable) {
        iContinue = ControlIO().pfnSendMessage(hwndList, win32::LB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(continueLibrary.szName));
        fContinue = iContinue != win32::LB_ERR;
    }
    if (fContinue) {
        CchSnprintf(std::span{szLabel, sizeof(szLabel)}, continueLibrary.fTurnAvailable ?
                    TextIO().pfnCompressed(app, idsLibraryContinuePlayer).data() : TextIO().pfnCompressed(app, idsLibraryContinueChoosePlayer).data(),
                    continueLibrary.szName, continueLibrary.iPlayer + 1, continueLibrary.iYear);
        win32::SetDlgItemTextA(hwnd, IDC_LIBRARY_CONTINUE_DETAILS, szLabel);
        ControlIO().pfnSetWindowText(app.lpui->rghwndBtnSplash[2], TextIO().pfnCompressed(app, idsContinueGame).data());
        ControlIO().pfnEnableWindow(app.lpui->rghwndBtnSplash[2], 1);
    } else {
        win32::SetDlgItemTextA(hwnd, IDC_LIBRARY_CONTINUE_DETAILS, "");
        ControlIO().pfnSetWindowText(app.lpui->rghwndBtnSplash[2], TextIO().pfnCompressed(app, static_cast<StringId>(idsNewGame + 2)).data());
        ControlIO().pfnEnableWindow(app.lpui->rghwndBtnSplash[2], 0);
    }
}

void RefreshLibraryHostGames(APPSESSION &app, HWND hwnd) {
    HWND hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_HOST_GAMES);
    std::int16_t fHasGames;
    if (!hwndList) return;
    ControlIO().pfnSendMessage(hwndList, win32::LB_RESETCONTENT, 0, 0);
    if (FInitLibrary()) FNativeEnumerateDirectory(app, szLibraryRoot, ListLibraryHostGame, hwndList);
    ControlIO().pfnSendMessage(hwndList, win32::LB_SETCURSEL, 0, 0);
    fHasGames = ControlIO().pfnSendMessage(hwndList, win32::LB_GETCOUNT, 0, 0) > 0;
    if (fHostLibraryView) {
        ControlIO().pfnEnableWindow(app.lpui->rghwndBtnSplash[1], fHasGames);
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAME_EXPORT), fHasGames);
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_DUPLICATE), fHasGames);
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RENAME), fHasGames);
        ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAME_DELETE), fHasGames);
    }
}

void SelectLibraryView(APPSESSION &app, HWND hwnd, std::int16_t fHost) {
    HWND hwndGames = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAMES);
    HWND hwndHost = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_HOST_GAMES);
    fHostLibraryView = fHost;
    ui::ShowWindow(hwndGames, fHost ? win32::SW_HIDE : win32::SW_SHOW);
    ui::ShowWindow(hwndHost, fHost ? win32::SW_SHOW : win32::SW_HIDE);
    ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_PLAYERS), fHost ? win32::SW_HIDE : win32::SW_SHOW);
    ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_PLAYER_LABEL), fHost ? win32::SW_HIDE : win32::SW_SHOW);
    ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_IMPORT), fHost ? win32::SW_HIDE : win32::SW_SHOW);
    ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_IMPORT_FOLDER), fHost ? win32::SW_HIDE : win32::SW_SHOW);
    ui::ShowWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_IMPORT_ORDERS), fHost ? win32::SW_SHOW : win32::SW_HIDE);
    if (fHost) RefreshLibraryHostGames(app, hwnd);
    else RefreshLibraryGames(app, hwnd);
}

std::int16_t FContinueLibraryGame(APPSESSION &app, HWND hwnd) {
    HWND hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAMES);
    char szFile[512];
    LRESULT i;
    std::int32_t lidGame;
    std::uint16_t turn;
    std::int16_t fMulti;

    if (!FFindContinueLibraryGame(app)) return 0;
    CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s%s%s%s.m%d", szLibraryRoot, szDirSep,
                continueLibrary.szName, szDirSep, continueLibrary.szName, continueLibrary.iPlayer + 1);
    if (FInspectImportFile(szFile, dtTurn, continueLibrary.iPlayer, &lidGame, &turn, nullptr, &fMulti)) {
        CchCopyUtf8(std::span{app.state.szBase}, szFile);
        app.state.ini.fStartupFile = 1;
        return 1;
    }

    // The remembered turn may have been removed after a newer game import.
    // Require an explicit choice when the remembered player's turn is missing.
    SelectLibraryView(app, hwnd, 0);
    i = ControlIO().pfnSendMessage(hwndList, win32::LB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(continueLibrary.szName));
    if (i == win32::LB_ERR) return 0;
    ControlIO().pfnSendMessage(hwndList, win32::LB_SETCURSEL, i, 0);
    RefreshLibraryPlayers(app, hwnd);
    ui::SendDlgItemMessage(hwnd, IDC_LIBRARY_PLAYERS, win32::CB_SETCURSEL, static_cast<WPARAM>(-1), 0);
    ControlIO().pfnEnableWindow(app.lpui->rghwndBtnSplash[1], 0);
    ControlIO().pfnSetFocus(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_PLAYERS));
    return 0;
}

std::int16_t FSelectLibraryGame(APPSESSION &app, HWND hwnd) {
    HWND hwndList;
    LRESULT i, iItem, iPlayer;

    if (fHostLibraryView) return FSelectLibraryHostGame(app, hwnd);

    hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_GAMES);
    i = ControlIO().pfnSendMessage(hwndList, win32::LB_GETCURSEL, 0, 0);
    if (i == win32::LB_ERR) return 0;
    iItem = ControlIO().pfnSendMessage(hwndList, win32::LB_GETITEMDATA, i, 0);
    if (iItem < 0 || iItem >= cLibraryGameList) return 0;
    iPlayer = ui::SendDlgItemMessage(hwnd, IDC_LIBRARY_PLAYERS, win32::CB_GETCURSEL, 0, 0);
    if (iPlayer == win32::CB_ERR) return 0;
    iPlayer = ui::SendDlgItemMessage(hwnd, IDC_LIBRARY_PLAYERS, win32::CB_GETITEMDATA, iPlayer, 0);
    if (iPlayer < 0 || iPlayer >= 16 || !(rgLibraryGameList[iItem].grfPlayers & (1u << iPlayer))) return 0;
    CchSnprintf(std::span{app.state.szBase, sizeof(app.state.szBase)}, "%s%s%s%s%s.m%d", szLibraryRoot, szDirSep,
                rgLibraryGameList[iItem].szName, szDirSep, rgLibraryGameList[iItem].szName, static_cast<int>(iPlayer) + 1);
    app.state.ini.fStartupFile = 1;
    return 1;
}

std::int16_t FSelectLibraryHostGame(APPSESSION &app, HWND hwnd) {
    HWND hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_HOST_GAMES);
    LRESULT i = ControlIO().pfnSendMessage(hwndList, win32::LB_GETCURSEL, 0, 0);
    char szName[256];
    int cch;
    if (i == win32::LB_ERR) return 0;
    ControlIO().pfnSendMessage(hwndList, win32::LB_GETTEXT, i, reinterpret_cast<LPARAM>(szName));
    cch = CchSnprintf(std::span{app.state.szBase, sizeof(app.state.szBase)}, "%s%s%s%s%s.hst", szLibraryRoot, szDirSep,
                      szName, szDirSep, szName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(app.state.szBase)) return 0;
    app.state.ini.fStartupFile = 1;
    return 1;
}

static std::int16_t FSelectedLibraryGameName(HWND hwnd, char *pszName, std::size_t cchName) {
    HWND hwndList = ControlIO().pfnGetDlgItem(hwnd, fHostLibraryView ? IDC_LIBRARY_HOST_GAMES : IDC_LIBRARY_GAMES);
    LRESULT i, iItem;
    int cch;
    i = ControlIO().pfnSendMessage(hwndList, win32::LB_GETCURSEL, 0, 0);
    if (i == win32::LB_ERR) return 0;
    if (fHostLibraryView) {
        cch = static_cast<int>(ControlIO().pfnSendMessage(hwndList, win32::LB_GETTEXTLEN, i, 0));
        if (cch <= 0 || static_cast<std::size_t>(cch) >= cchName) return 0;
        return ControlIO().pfnSendMessage(hwndList, win32::LB_GETTEXT, i, reinterpret_cast<LPARAM>(pszName)) == cch;
    }
    iItem = ControlIO().pfnSendMessage(hwndList, win32::LB_GETITEMDATA, i, 0);
    if (iItem < 0 || iItem >= cLibraryGameList) return 0;
    cch = CchSnprintf(std::span{pszName, cchName}, "%s", rgLibraryGameList[iItem].szName);
    return cch > 0 && static_cast<std::size_t>(cch) < cchName;
}

static std::int16_t FChooseLibraryGameActionName(APPSESSION &app, HWND hwnd, const char *pszCurrentName, char *pszNewName) {
    int cch;
    cch = CchSnprintf(std::span{szLibraryName, sizeof(szLibraryName)}, "%s", pszCurrentName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szLibraryName)) return 0;
    SuggestLibraryName(app, std::span{szLibraryName}, 0);
    if (!szLibraryName[0] || IdPromptLibraryName(app, hwnd, std::span{szLibraryName}) != 1) return 0;
    cch = CchSnprintf(std::span{pszNewName, 256}, "%s", szLibraryName);
    return cch > 0 && cch < 256;
}

std::int16_t FActOnSelectedLibraryGame(APPSESSION &app, HWND hwnd, std::int16_t fRename) {
    char szName[256], szNewName[256];
    if (!FSelectedLibraryGameName(hwnd, szName, sizeof(szName)) ||
        !FChooseLibraryGameActionName(app, hwnd, szName, szNewName)) return 0;
    return fRename ? FRenameLibraryGame(app, hwnd, szName, szNewName) :
                     FDuplicateLibraryGame(app, hwnd, szName, szNewName);
}

void RememberLibraryGame(APPSESSION &app) {
    char szDir[256], szMetadataName[256];
    char *pch;
    int iplr;
    long long lPlayed;

    if (!FInitLibrary() || !LibraryPathIO().pfnUnder(szLibraryRoot, app.state.szBase)) return;
    CchCopyUtf8(std::span{szDir}, app.state.szBase);
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
    if (FWriteLibraryMetadata(szDir, pch + 1, app.state.idPlayer))
        ProfileTextIO().pfnWriteString("Library", "ContinueDeleted", nullptr, TextIO().pfnCompressed(app, idsStarsIni).data());
}

void CloseLibraryUiSession() noexcept {
    ClearLibraryGameList();
    szLibraryRoot[0] = szLibraryPending[0] = szLibraryName[0] = szLibraryExchangeName[0] = 0;
    szRaceLibraryRoot[0] = szRaceLibraryPath[0] = szRaceLibraryName[0] = szRaceLibraryTemp[0] = 0;
    fRaceLibraryNameDialog = fNewLibraryRace = fHostLibraryView = 0;
    pszRaceLibraryNameOverride = nullptr;
    continueLibrary = LIBRARYCONTINUE{};
}

std::int16_t FPublishSubsequentTurn(APPSESSION &app, const LIBRARYPATHS &paths, HWND hwnd, const char *pszSource,
                                  const char *szTarget, const char *pszName, int iplr,
                                  std::int32_t lidGame, std::uint16_t turn, std::int16_t fHasTurn) {
    char szOrders[512],szStage[512],szPrevious[512],szMessage[1024];
    int cch;
    std::int32_t lidCurrent;
    std::uint16_t turnCurrent;
    std::int16_t fMultiCurrent,fHasOrders;

    std::uint32_t dwTick;
    cch = LibraryPublicationIO().pfnCchSnprintf(std::span{szOrders, sizeof(szOrders)}, "%s%s%s%s%s.x%d", paths.szLibraryRoot, szDirSep, pszName, szDirSep, pszName, iplr + 1);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szOrders)) return 0;
    dwTick = LibraryPublicationIO().pfnDwTickCount();
    szStage[0] = 0;
    cch = LibraryPublicationIO().pfnCchSnprintf(std::span{szStage, sizeof(szStage)}, "%s.importing.%u", szTarget, dwTick);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szStage)) return 0;
    cch = LibraryPublicationIO().pfnCchSnprintf(std::span{szPrevious, sizeof(szPrevious)}, "%s.previous.%u", szTarget, dwTick);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szPrevious)) return 0;
    if (!LibraryPublicationIO().pfnFCopyNativeFileExclusive(pszSource, szStage) ||
        !LibraryPublicationIO().pfnFInspectImportFile(szStage, dtTurn, iplr, &lidCurrent, &turnCurrent, nullptr, &fMultiCurrent) ||
        lidCurrent != lidGame || turnCurrent != turn) {
        if (szStage[0]) RemoveNativeFile(szStage);
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryTurnStageFailed).data(), win32::MB_ICONHAND);
        return 0;
    }
    if (!fHasTurn) {
        if (ReplaceNativeFile(szStage, szTarget) != 0) {
            RemoveNativeFile(szStage);
            AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryTurnInstallFailed).data(), win32::MB_ICONHAND);
            return 0;
        }
        /* No previous turn was advanced, so retain any host orders/history. */
        return 1;
    }
    fHasOrders = LibraryPublicationIO().pfnFNativeFileSize(szOrders).has_value();
    if (!LibraryPublicationIO().pfnFCopyNativeFileExclusive(szTarget, szPrevious)) {
        RemoveNativeFile(szStage);
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryTurnPreserveFailed).data(), win32::MB_ICONHAND);
        return 0;
    }
    if (ReplaceNativeFile(szStage, szTarget) != 0) {
        RemoveNativeFile(szStage);
        RemoveNativeFile(szPrevious);
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryTurnInstallFailed).data(), win32::MB_ICONHAND);
        return 0;
    }
    if (fHasOrders && RemoveNativeFile(szOrders) != 0) {
        if (ReplaceNativeFile(szPrevious, szTarget) != 0) {
            LibraryPublicationIO().pfnCchSnprintf(std::span{szMessage, sizeof(szMessage)},
                        TextIO().pfnCompressed(app, idsLibraryTurnRestoreFailed).data(),
                        szPrevious);
            AlertIO().pfnAlertSz(app, szMessage, win32::MB_ICONHAND);
            return 0;
        }
        RemoveNativeFile(szPrevious);
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryTurnOrdersRetireFailed).data(), win32::MB_ICONHAND);
        return 0;
    }
    RemoveNativeFile(szPrevious);
    static_cast<void>(hwnd);
    return 1;
}

std::int16_t FPublishImportedLibraryGame(APPSESSION &app, const LIBRARYPATHS &paths, LIBRARYIMPORT &li, const char *szDir) {
    char szStage[256];
    LibraryPublicationIO().pfnCchSnprintf(std::span{szStage, sizeof(szStage)}, "%s%s.importing.%lld", paths.szLibraryRoot, szDirSep, static_cast<long long>(LibraryPublicationIO().pfnLNativeTime()));
    if (LibraryPublicationIO().pfnCreateNativeDir(szStage) != 0) {
        RemoveNativePath(paths.szLibraryPending);
        paths.szLibraryPending[0] = 0;
        return 0;
    }
    li.pszTarget = szStage;
    li.pszName = paths.szLibraryName;
    li.fCopy = 1;
    if (!LibraryPublicationIO().pfnFNativeEnumerateDirectory(app, szDir, CopyImportEntry, &li) || li.fBad ||
        !LibraryPublicationIO().pfnFNativeEnumerateDirectory(app, szDir, CopyBackupDirectory, &li) || li.fBad ||
        !LibraryPublicationIO().pfnFWriteLibraryMetadata(szStage, paths.szLibraryName,
                               li.grfPlayers ? static_cast<std::int16_t>(__builtin_ctz(li.grfPlayers)) : iplrNone) ||
        RemoveNativePath(paths.szLibraryPending) != 0 || RenameNativeFile(szStage, paths.szLibraryPending) != 0) {
        LibraryPublicationIO().pfnFNativeEnumerateDirectory(app, szStage, RemoveImportFile, szStage);
        RemoveNativePath(szStage);
        RemoveNativePath(paths.szLibraryPending);
        paths.szLibraryPending[0] = 0;
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryGameImportFailed).data(), win32::MB_ICONHAND);
        return 0;
    }
    paths.szLibraryPending[0] = 0;
    return 1;
}
