#include "win.h"
#include <algorithm>
#include <array>
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
#include "file_selection.h"
#include "file_selection_ui.h"
#include "ownership.h"
#include <limits>
#include <charconv>
#include <optional>
#include <stars_compat.h>
#include <SDL3/SDL.h>
#include <cerrno>
#include <string_view>
#include <utility>
#include "libraryzip.h"

/* Desktop metadata is deliberately outside the legacy game records. A folder
 * is listed only after its complete generated family and metadata are saved. */
static char szLibraryRoot[256];
static std::size_t cchLibraryRoot;
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
static std::int16_t FCopyNativeFile(std::string_view szSource, std::string_view szTarget);
static std::int16_t FSelectedLibraryGameName(HWND hwnd, std::span<char> pszName);
static void *PvLibraryFolderContext(HWND hwnd) {
    const std::uint64_t id = StarsCompatWindowId(hwnd);
    // SDL carries one pointer-sized opaque value; no window pointer outlives UI.
    return id <= std::numeric_limits<std::uintptr_t>::max() ? reinterpret_cast<void *>(static_cast<std::uintptr_t>(id)) : nullptr;
}

static void SDLCALL LibraryExportFolderSelected(void *lpData, const char * const *rgpszPath, int iFilter) noexcept;
static std::int16_t FWriteLibraryMetadataValues(std::string_view pszDir, std::string_view pszName, int iplr,
                                           long long lPlayed, std::string_view pszExchangeName);

struct RACESTATE {
    PLAYER plr;
    char szPass[sizeof(APPGAMESTATE::szRacePass)];
    char szFile[sizeof(APPGAMESTATE::szRaceFile)];
    char szPath[512];
    std::int16_t fNew;
};

struct LIBRARYNAMECHECK {
    std::string_view pszDirectory;
    std::string_view pszName;
    std::int16_t fFound;
};

static void SaveRaceState(APPSESSION &app, RACESTATE *prs) {
    prs->plr = app.state.vplr;
    prs->fNew = fNewLibraryRace;
    std::copy_n(app.state.szRacePass, sizeof(prs->szPass), prs->szPass);
    std::copy_n(app.state.szRaceFile, sizeof(prs->szFile), prs->szFile);
    std::copy_n(app.lpui->szRaceLibraryPath.data(), sizeof(prs->szPath), prs->szPath);
}

static void RestoreRaceState(APPSESSION &app, const RACESTATE *prs, std::int16_t fRestoreRace, std::int16_t fRestorePath) {
    if (fRestoreRace) {
        app.state.vplr = prs->plr;
        std::copy_n(prs->szPass, sizeof(prs->szPass), app.state.szRacePass);
        std::copy_n(prs->szFile, sizeof(prs->szFile), app.state.szRaceFile);
    }
    if (fRestorePath) {
        std::copy_n(prs->szPath, sizeof(prs->szPath), app.lpui->szRaceLibraryPath.data());
        fNewLibraryRace = prs->fNew;
    }
}

static bool FindLibraryName(APPSESSION &, LIBRARYNAMECHECK *plnc, std::string_view pszFile) {
    const std::string szName{plnc->pszName};
    const std::string szFile{pszFile};
    if (!SDL_strcasecmp(szName.c_str(), szFile.c_str())) {
        plnc->fFound = 1;
        return false;
    }
    return true;
}

std::int16_t FLibraryNameAvailable(APPSESSION &app, std::string_view pszDirectory, std::string_view pszName) {
    LIBRARYNAMECHECK lnc;
    lnc.pszDirectory = pszDirectory;
    lnc.pszName = pszName;
    lnc.fFound = 0;
    if (!FNativeEnumerateDirectory(app, lnc.pszDirectory, FindLibraryName, &lnc)) return 0;
    return !lnc.fFound;
}

static std::int16_t FRaceLibraryNameAvailable(APPSESSION &app, std::string_view pszName) {
    return FLibraryNameAvailable(app, app.lpui->szRaceLibraryRoot.data(), pszName);
}

static bool FInvalidPortableFilenameByte(unsigned char ch) {
    constexpr std::string_view szInvalid = "<>:\"/\\|?*";
    return szInvalid.find(static_cast<char>(ch)) != std::string_view::npos;
}

static std::int16_t FPortableRaceFilename(APPSESSION &app, std::string_view pszName) {
    char szDevice[256];
    const std::size_t cch = pszName.size();
    std::size_t i;
    const std::size_t ichExt = pszName.rfind('.');
    if (ichExt == std::string_view::npos || ichExt == 0 || cch <= ichExt + 2 ||
        SDL_tolower(pszName[ichExt + 1]) != 'r' || pszName[ichExt + 2] == '0' || pszName[ichExt + 2] < '1' ||
        pszName[ichExt + 2] > '9' || cch >= 240 || pszName[0] == ' ' || pszName[0] == '.' ||
        pszName[cch - 1] == ' ' || pszName[cch - 1] == '.') return 0;
    /* Windows reserves these stems even when followed by an extension. */
    for (i = 0; i < cch && pszName[i] != '.'; i++) szDevice[i] = pszName[i];
    szDevice[i] = 0;
    if (!SDL_strcasecmp(szDevice, "CON") || !SDL_strcasecmp(szDevice, "PRN") ||
        !SDL_strcasecmp(szDevice, "AUX") || !SDL_strcasecmp(szDevice, "NUL") ||
        !SDL_strcasecmp(szDevice, "CLOCK$") ||
        (i == 4 && (!SDL_strncasecmp(szDevice, "COM", 3) || !SDL_strncasecmp(szDevice, "LPT", 3)) &&
         szDevice[3] >= '1' && szDevice[3] <= '9')) return 0;
    if (cch - (ichExt + 2) > 3) return 0;
    for (i = ichExt + 3; i < cch; i++)
        if (pszName[i] < '0' || pszName[i] > '9') return 0;
    for (char ch : pszName) {
        const auto uch = static_cast<unsigned char>(ch);
        if (uch < 32 || uch == 127 || FInvalidPortableFilenameByte(uch)) return 0;
    }
    if (std::string_view(app.lpui->szRaceLibraryRoot.data()).size() + cch + 2 >= sizeof(app.state.szBase)) return 0;
    return 1;
}

/* The Windows CRT environment uses its code page; SDL supplies UTF-8 paths. */
static std::int16_t FInitRaceLibrary(APPSESSION &app) {
    const char *pszOverride = SDL_getenv("STARS_LIBRARY_DIR");
    const char *pszSettings = SDL_getenv("STARS_SETTINGS_DIR");
    char *pszRoot = pszOverride || pszSettings ? nullptr : CompatPathIO().pfnGetPrefPath("", "Stars!");
    int cch;
    NATIVEOWNER<char, SDL_free> rootPath(pszRoot);
    if (!pszOverride && !pszSettings && !pszRoot) return 0;
    cch = CchSnprintf(std::span{app.lpui->szRaceLibraryRoot}, "%s%sRace Designs",
                      pszOverride ? pszOverride : pszSettings ? pszSettings : pszRoot,
                      pszOverride || pszSettings ? szDirSep : "");
    rootPath.reset();
    return cch > 0 && cch < static_cast<int>(app.lpui->szRaceLibraryRoot.size()) && FEnsureNativeDir(app.lpui->szRaceLibraryRoot.data());
}

static std::int16_t FCreateRaceStagePath(APPSESSION &app, std::span<char> pszDirectory, std::span<char> pszFile) {
    std::uint32_t dwTick = DwTickCount();
    int i;
    int cch;
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(pszDirectory, "%s%s.stars-race-stage-%u-%u",
                          app.lpui->szRaceLibraryRoot.data(), szDirSep, dwTick, idRaceStage++);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= pszDirectory.size()) return 0;
        if (CreateNativeDir(pszDirectory.data()) != 0) {
            if (FNativeDirectoryExists(pszDirectory.data())) continue;
            return 0;
        }
        cch = CchSnprintf(pszFile, "%s%sdesign.r1", pszDirectory.data(), szDirSep);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= pszFile.size()) {
            RemoveNativePath(pszDirectory.data());
            return 0;
        }
        return 1;
    }
    return 0;
}

static void SuggestRaceFilename(APPSESSION &app, std::span<char, 256> pszName) {
    char szStem[240];
    const auto itEnd = std::find(pszName.begin(), pszName.end(), '\0');
    const std::string_view szName(pszName.data(), static_cast<std::size_t>(itEnd - pszName.begin()));
    const std::size_t ichExt = szName.rfind('.');
    std::size_t cchStem = ichExt != std::string_view::npos ? ichExt : szName.size();
    int i;
    if (cchStem >= sizeof(szStem)) cchStem = sizeof(szStem) - 1;
    cchStem = CchUtf8Prefix(szName, cchStem);
    std::ranges::copy_n(szName.begin(), cchStem, szStem);
    szStem[cchStem] = 0;
    for (i = 1; i < 1000; i++) {
        CchSnprintf(pszName, "%s.r%d", szStem, i);
        if (FRaceLibraryNameAvailable(app, pszName.data())) return;
    }
    pszName[0] = 0;
}

static std::int16_t FFormatLibraryFamilyPath(std::span<char> pszPath, std::string_view pszDirectory,
                                             std::string_view pszName, DtFileType dt, int iplr) {
    const char *pszExtension;
    int cch;
    if (dt == dtXY) pszExtension = "xy";
    else if (dt == dtHost) pszExtension = "hst";
    else pszExtension = dt == dtTurn ? "m" : dt == dtHist ? "h" : "x";
    if (dt == dtXY || dt == dtHost)
        cch = CchSnprintf(pszPath, "%s%s%s.%s", pszDirectory, szDirSep, pszName, pszExtension);
    else cch = CchSnprintf(pszPath, "%s%s%s.%s%d", pszDirectory, szDirSep, pszName, pszExtension, iplr + 1);
    return cch > 0 && static_cast<std::size_t>(cch) < pszPath.size();
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
    cchLibraryRoot = cch > 0 && cch < static_cast<int>(sizeof(szLibraryRoot)) ? static_cast<std::size_t>(cch) : 0;
    return cchLibraryRoot != 0 && FEnsureNativeDir(szLibraryRoot);
}

std::int16_t FPathUnderLibrary(std::string_view szRoot, std::string_view szFile) {
    std::size_t cchRoot = szRoot.size(), cchFile = szFile.size();
#ifdef _WIN32
    std::size_t ich;
#endif
    if (cchRoot == 0 || cchFile <= cchRoot) return 0;
#ifdef _WIN32
    /* Pickers return '/', while library joins and Windows arguments may use '\\'. */
    for (ich = 0; ich < cchRoot; ich++) {
        if ((szRoot[ich] == '/' || szRoot[ich] == '\\') &&
            (szFile[ich] == '/' || szFile[ich] == '\\')) continue;
        if (SDL_tolower(static_cast<unsigned char>(szRoot[ich])) != SDL_tolower(static_cast<unsigned char>(szFile[ich]))) return 0;
    }
    return szFile[cchRoot] == '/' || szFile[cchRoot] == '\\';
#else
    return szFile.substr(0, cchRoot) == szRoot && szFile[cchRoot] == '/';
#endif
}

static std::optional<std::size_t> IchLibrarySeparator(std::string_view pszPath) {
    std::size_t ich = pszPath.rfind(chDirSep);
#ifdef _WIN32
    const std::size_t ichSlash = pszPath.rfind('/');
    if (ichSlash != std::string_view::npos && (ich == std::string_view::npos || ichSlash > ich)) ich = ichSlash;
#endif
    return ich == std::string_view::npos ? std::nullopt : std::optional{ich};
}

std::int16_t FIsManagedLibraryGameFile(std::string_view pszFile) {
    return FInitLibrary() && LibraryPathIO().pfnUnder(szLibraryRoot, pszFile);
}

std::int16_t FIsManagedLibraryRaceFile(APPSESSION &app, std::string_view pszFile) {
    return FInitRaceLibrary(app) && LibraryPathIO().pfnUnder(app.lpui->szRaceLibraryRoot.data(), pszFile);
}

static std::int16_t FPortableLibraryName(APPSESSION &app, std::string_view pszName) {
    char szDevice[256];
    const std::size_t cch = pszName.size();
    std::size_t i;

    if (cch == 0 || cch >= sizeof(szDevice) || pszName[0] == ' ' || pszName[0] == '.' || pszName[cch - 1] == ' ' || pszName[cch - 1] == '.') return 0;
    for (char ch : pszName) {
        const auto uch = static_cast<unsigned char>(ch);
        if (uch < 32 || uch == 127 || FInvalidPortableFilenameByte(uch)) return 0;
    }
    /* Windows reserves these stems even when followed by an extension. */
    for (i = 0; i < cch && pszName[i] != '.'; i++) szDevice[i] = pszName[i];
    szDevice[i] = 0;
    if (!SDL_strcasecmp(szDevice, "CON") || !SDL_strcasecmp(szDevice, "PRN") ||
        !SDL_strcasecmp(szDevice, "AUX") || !SDL_strcasecmp(szDevice, "NUL") ||
        !SDL_strcasecmp(szDevice, "CLOCK$") ||
        (i == 4 && (!SDL_strncasecmp(szDevice, "COM", 3) || !SDL_strncasecmp(szDevice, "LPT", 3)) &&
         szDevice[3] >= '1' && szDevice[3] <= '9')) return 0;
    /* Longest backup path: root/name/backup.999/name.m16 plus NUL. */
    return std::string_view(szLibraryRoot).size() + 2 * cch + 18 < sizeof(app.state.szBase);
}

static void SuggestLibraryName(APPSESSION &app, std::span<char, 256> pszName, std::int16_t fRevision) {
    char szOriginal[256];
    char szSuffix[32];
    int i, cch, cchMax, cchOriginal, cchSuffix;

    cchOriginal = CchCopyUtf8(std::span{szOriginal}, pszName.data());
    if (cchOriginal < 0) cchOriginal = 0;
    cchMax = (static_cast<int>(sizeof(app.state.szBase)) - static_cast<int>(std::string_view(szLibraryRoot).size()) - 19) / 2;
    for (i = 2; i < 10000 && !FLibraryNameAvailable(app, szLibraryRoot, pszName.data()); i++) {
        CchSnprintf(std::span{szSuffix, sizeof(szSuffix)}, fRevision ? TextIO().pfnCompressed(app, idsLibraryGameRevisionSuffix).data() : " (%d)", i);
        cchSuffix = static_cast<int>(std::string_view(szSuffix).size());
        cch = std::min(cchOriginal, std::max(0, cchMax - cchSuffix));
        /* A shortened suggestion must not split a UTF-8 character. */
        while (cch > 0 && (static_cast<unsigned char>(szOriginal[cch]) & 0xc0) == 0x80) cch--;
        CchSnprintf(pszName, "%.*s%s", cch, szOriginal, szSuffix);
    }
}

static void GetGameLibraryName(APPSESSION &app, std::string_view pszName, std::span<char, 256> pszNameOut) {
    std::array<char, cchCatalogText> szCatalogPszName{};
    std::size_t i, cch;
    int cchMax = (static_cast<int>(sizeof(app.state.szBase)) - static_cast<int>(std::string_view(szLibraryRoot).size()) - 19) / 2;

    cch = static_cast<std::size_t>(std::max(0, CchCopyUtf8(pszNameOut, pszName.substr(0, CchUtf8Prefix(pszName, static_cast<std::size_t>(cchMax))))));
    for (i = 0; i < cch; i++)
        if (static_cast<unsigned char>(pszNameOut[i]) < 32 || pszNameOut[i] == 127 || FInvalidPortableFilenameByte(static_cast<unsigned char>(pszNameOut[i]))) pszNameOut[i] = '_';
    while (cch != 0 && (pszNameOut[cch - 1] == ' ' || pszNameOut[cch - 1] == '.')) pszNameOut[--cch] = 0;
    while (cch != 0 && (pszNameOut[0] == ' ' || pszNameOut[0] == '.')) {
        std::move(pszNameOut.begin() + 1, pszNameOut.begin() + cch + 1, pszNameOut.begin());
        cch--;
    }
    // The game loader treats the last period as a file extension.
    for (i = 0; i < cch; i++) if (pszNameOut[i] == '.') pszNameOut[i] = '_';
    if (cch == 0) {
        szCatalogPszName = TextIO().pfnCompressed(app, idsLibraryDefaultGameName);
        cch = static_cast<std::size_t>(std::max(0, CchCopyUtf8(pszNameOut, std::string_view(szCatalogPszName.data(), CchUtf8Prefix(szCatalogPszName.data(), static_cast<std::size_t>(cchMax))))));
    }
    if (!FPortableLibraryName(app, pszNameOut.data())) {
        // Reserved device names need a prefix; leave the stored game title intact.
        cch = CchUtf8Prefix(std::string_view(pszNameOut.data(), cch), static_cast<std::size_t>(StarsMax(0, cchMax - 1)));
        pszNameOut[cch] = 0;
        std::move_backward(pszNameOut.begin(), pszNameOut.begin() + cch + 1, pszNameOut.begin() + cch + 2);
        pszNameOut[0] = '_';
    }
}

INT_PTR  LibraryGameRevisionDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = AppCompatCallbackContext();
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
    APPSESSION &app = AppCompatCallbackContext();
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

std::int16_t FChooseLibraryGameName(APPSESSION &app, HWND hwnd, std::span<char, 256> pszFile,
                                    std::optional<std::string_view> pszName) {
    char *pch;
    char szMessage[256];

    if (!FInitLibrary() || cchLibraryRoot + 20 >= sizeof(app.state.szBase)) goto LFail;
    app.lpui->szLibraryExchangeName[0] = 0;
    if (pszName) GetGameLibraryName(app, *pszName, std::span{app.lpui->szLibraryName});
    else {
        pch = std::strrchr(pszFile.data(), chDirSep);
        CchSnprintf(std::span{app.lpui->szLibraryName}, "%s", pch ? pch + 1 : pszFile.data());
        pch = std::strrchr(app.lpui->szLibraryName.data(), '.');
        if (pch) *pch = 0;
        if (!FPortableLibraryName(app, app.lpui->szLibraryName.data())) CchCopyUtf8(std::span{app.lpui->szLibraryName}, TextIO().pfnCompressed(app, idsLibraryDefaultGameName).data());
    }
    if (pszName) {
        if (!FLibraryNameAvailable(app, szLibraryRoot, app.lpui->szLibraryName.data())) {
            SuggestLibraryName(app, std::span{app.lpui->szLibraryName}, 1);
            if (ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_LIBRARY_GAME_REVISION), hwnd,
                               LibraryGameRevisionDlg, reinterpret_cast<LPARAM>(app.lpui->szLibraryName.data())) != 1) return 0;
        }
    } else {
        SuggestLibraryName(app, std::span{app.lpui->szLibraryName}, 0);
        if (IdPromptLibraryName(app, hwnd, std::span{app.lpui->szLibraryName}) != 1) return 0;
    }
    if (!FPortableLibraryName(app, app.lpui->szLibraryName.data())) goto LFail;
    CchSnprintf(std::span{app.lpui->szLibraryPending}, "%s%s%s", szLibraryRoot, szDirSep, app.lpui->szLibraryName.data());
    /* Reserve only a newly created folder; never write into an existing copy. */
    if (!FLibraryNameAvailable(app, szLibraryRoot, app.lpui->szLibraryName.data()) || CreateNativeDir(app.lpui->szLibraryPending.data()) != 0) {
        app.lpui->szLibraryPending[0] = 0;
        goto LFail;
    }
    CchSnprintf(pszFile, "%s%s%s.xy", app.lpui->szLibraryPending.data(), szDirSep, app.lpui->szLibraryName.data());
    return 1;
LFail:
    CchCopyUtf8Prefix(std::span{szMessage}, TextIO().pfnCompressed(app, idsLibraryStorageFailed).data());
    CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage, TextIO().pfnCompressed(app, idsLibraryGames).data(), win32::MB_OK | win32::MB_ICONHAND);
    return 0;
}

static std::int16_t FChooseLibraryRaceName(APPSESSION &app, HWND hwnd, std::string_view pszSuggested,
                                           std::span<char> pszPath) {
    char szMessage[256];
    int cch;
    if (!FInitRaceLibrary(app)) return 0;
    CchSnprintf(std::span{app.lpui->szRaceLibraryName}, "%s", pszSuggested);
    SuggestRaceFilename(app, std::span{app.lpui->szRaceLibraryName});
    if (!app.lpui->szRaceLibraryName[0]) return 0;
    fRaceLibraryNameDialog = 1;
    if (IdPromptLibraryName(app, hwnd, std::span{app.lpui->szRaceLibraryName}) != 1) {
        fRaceLibraryNameDialog = 0;
        return 0;
    }
    fRaceLibraryNameDialog = 0;
    cch = CchSnprintf(pszPath, "%s%s%s", app.lpui->szRaceLibraryRoot.data(), szDirSep, app.lpui->szRaceLibraryName.data());
    if (cch <= 0 || !FRaceLibraryNameAvailable(app, app.lpui->szRaceLibraryName.data())) {
        CchCopyUtf8Prefix(std::span{szMessage}, TextIO().pfnCompressed(app, idsLibraryStorageFailed).data());
        CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage, TextIO().pfnCompressed(app, idsLibraryGames).data(), win32::MB_OK | win32::MB_ICONHAND);
        return 0;
    }
    return 1;
}

std::int16_t FWriteLibraryMetadata(APPSESSION &app, std::string_view pszDir, std::string_view pszName, int iplr) {
    return FWriteLibraryMetadataValues(pszDir, pszName, iplr, static_cast<long long>(LNativeTime()),
                                       app.lpui->szLibraryExchangeName[0] ? std::string_view{app.lpui->szLibraryExchangeName.data()} : pszName);
}

static std::int16_t FWriteLibraryMetadataValues(std::string_view pszDir, std::string_view pszName, int iplr,
                                           long long lPlayed, std::string_view pszExchangeName) {
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

static bool RemovePendingFile(APPSESSION &app, std::string_view pszFile) {
    char szFile[256];
    CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s%s", app.lpui->szLibraryPending.data(), szDirSep, pszFile);
    /* Failed generation can leave an empty directory at a file destination. */
    RemoveNativePath(szFile);
    return true;
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
static std::int16_t FCompleteGeneratedFile(APPSESSION &app, std::string_view pszFile, DtFileType dt, int iplr) {
    NATIVEFILE *pf;
    HDR hdr;
    RTBOF rtbof;
    long cbFile;
    std::optional<std::int64_t> ib;
    std::int16_t fComplete;

    pf = PfOpenNativeFile(pszFile, NATIVEFILEMODE::read);
    if (!pf) return 0;
    fComplete = 0;
    if (ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::end) != 0 || !(ib = LNativePosition(pf)) || *ib < 22 || ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::begin) != 0) goto LDone;
    cbFile = static_cast<long>(*ib);
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
        if (!ib || *ib < 0 || *ib + hdr.cb > cbFile) break;
        if (hdr.rt == rtEOF) {
            fComplete = hdr.cb == 2 && *ib + hdr.cb == cbFile;
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

static std::int16_t FCopyNativeFile(std::string_view szSource, std::string_view szTarget) {
    NATIVEFILE *pfSource, *pfTarget;
    int fOkay;

    pfSource = PfOpenNativeFile(szSource, NATIVEFILEMODE::read);
    if (!pfSource) return 0;
    pfTarget = PfOpenNativeFile(szTarget, NATIVEFILEMODE::writeTruncate);
    if (!pfTarget) { ICloseNativeFile(pfSource); return 0; }
    fOkay = FCopyNativeFileContents(pfSource, pfTarget);
    if (!fOkay) RemoveNativeFile(szTarget);
    return fOkay;
}

std::int16_t FCopyNativeFileExclusive(std::string_view szSource, std::string_view szTarget) {
    NATIVEFILE *pfSource, *pfTarget;
    int iErrno;
    int fOkay;

    pfSource = LibraryFileIO().pfnOpen(szSource, NATIVEFILEMODE::read);
    if (!pfSource) return 0;
    pfTarget = LibraryFileIO().pfnOpen(szTarget, NATIVEFILEMODE::writeExclusive);
    if (!pfTarget) {
        iErrno = errno;
        LibraryFileIO().pfnClose(pfSource);
        errno = iErrno;
        return 0;
    }
    fOkay = FCopyNativeFileContents(pfSource, pfTarget);
    if (!fOkay) RemoveNativeFile(szTarget);
    return fOkay;
}

static std::int16_t FCopyRaceExport(std::string_view pszSource, std::string_view pszTarget,
                               std::span<char> pszTemp) {
    std::size_t ichSlash;
    std::size_t cchDirectory;
    std::uint32_t dwTick;
    int i, cch;

    ichSlash = pszTarget.rfind(chDirSep);
    cchDirectory = ichSlash != std::string_view::npos ? ichSlash + 1 : 0;
    if (cchDirectory > std::numeric_limits<int>::max()) return 0;
    dwTick = DwTickCount();
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(pszTemp, "%.*s.stars-race-export-%u-%u.tmp",
                          static_cast<int>(cchDirectory), pszTarget, dwTick, idRaceStage++);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= pszTemp.size()) return 0;
        errno = 0;
        if (FCopyNativeFileExclusive(pszSource, std::string_view{pszTemp.data(), static_cast<std::size_t>(cch)})) return 1;
        if (errno != EEXIST) return 0;
    }
    return 0;
}

std::int16_t FSaveLibraryRace(APPSESSION &app, HWND hwnd, std::string_view pszName, PLAYER *pplr) {
    char szPath[512];
    char szTempDir[512];
    char szName[256];
    std::array<char, 512> szRaceLibraryTemp = {};
    std::uint16_t icksum;
    std::int16_t fOkay;
    if (!FInitRaceLibrary(app) || !FPortableRaceFilename(app, pszName)) return 0;
    std::copy(pszName.begin(), pszName.end(), szName);
    szName[pszName.size()] = 0;
    CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", app.lpui->szRaceLibraryRoot.data(), szDirSep, szName);
    if (!FCreateRaceStagePath(app, std::span{szTempDir}, std::span{szRaceLibraryTemp})) return 0;
    if (!RecordIO().pfnFCreateFile(app, dtRace, iplrNone, szRaceLibraryTemp.data())) {
        RemoveNativeFile(szRaceLibraryTemp.data());
        RemoveNativePath(szTempDir);
        return 0;
    }
    WriteRtPlr(app, pplr);
    icksum = IRaceChecksum(pplr);
    WriteRtEof(app, icksum);
    RecordIO().pfnStreamClose(app);
    fOkay = ReplaceNativeFile(szRaceLibraryTemp.data(), szPath) == 0;
    if (!fOkay) {
        RemoveNativeFile(szRaceLibraryTemp.data());
        RemoveNativePath(szTempDir);
        if (win32::IsWindow(hwnd)) AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsStarsUnableSaveRaceDataFilePlease, {}).data(), win32::MB_ICONHAND);
        return 0;
    }
    RemoveNativePath(szTempDir);
    CchSnprintf(std::span{app.lpui->szRaceLibraryPath}, "%s", szPath);
    SetSzRaceFile(app, szName);
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

static std::int16_t FReadManagedRace(APPSESSION &app, std::string_view pszPath, PLAYER *pplr) {
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

static void GetRaceLibraryStem(APPSESSION &app, const char *pszName, std::span<char, 240> pszStem) {
    std::size_t i, cch;
    char szTest[256];

    CchSnprintf(pszStem, "%s", pszName);
    cch = std::string_view(pszStem.data()).size();
    for (i = 0; i < cch; i++)
        if (static_cast<unsigned char>(pszStem[i]) < 32 || pszStem[i] == 127 || FInvalidPortableFilenameByte(static_cast<unsigned char>(pszStem[i]))) pszStem[i] = '_';
    while (cch != 0 && (pszStem[cch - 1] == ' ' || pszStem[cch - 1] == '.')) pszStem[--cch] = 0;
    while (cch != 0 && (pszStem[0] == ' ' || pszStem[0] == '.')) {
        std::move(pszStem.begin() + 1, pszStem.begin() + cch + 1, pszStem.begin());
        cch--;
    }
    if (cch == 0) {
        CchSnprintf(pszStem, "%s", TextIO().pfnCompressed(app, idsLibraryDefaultRaceName).data());
        cch = std::string_view(pszStem.data()).size();
    }
    CchSnprintf(std::span{szTest, sizeof(szTest)}, "%s.r1", pszStem.data());
    if (!FPortableRaceFilename(app, szTest)) {
        // Prefix reserved device names without altering the in-game race name.
        std::move_backward(pszStem.begin(), pszStem.begin() + cch + 1, pszStem.begin() + cch + 2);
        pszStem[0] = '_';
    }
}

static bool FindRaceVersion(APPSESSION &app, RACEVERSIONS *prv, std::string_view pszName) {
    const std::string_view szName = pszName;
    const std::size_t ichExt = szName.rfind('.');
    char szPath[512];
    PLAYER plr;
    int iVersion;
    std::from_chars_result result;
    std::size_t cch = std::string_view(prv->szStem).size();

    if (ichExt == std::string_view::npos || !FPortableRaceFilename(app, szName)) return true;
    result = std::from_chars(szName.data() + ichExt + 2, szName.data() + szName.size(), iVersion);
    if (result.ec != std::errc{} || result.ptr != szName.data() + szName.size()) return true;
    if (ichExt == cch && !SDL_strncasecmp(szName.data(), prv->szStem, cch))
        prv->iMax = StarsMax(prv->iMax, iVersion);
    CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", app.lpui->szRaceLibraryRoot.data(), szDirSep, pszName);
    if (!FReadManagedRace(app, szPath, &plr)) return true;
    // Versions follow the stored race name, including older/imported filenames.
    // Distinct names can sanitize to the same stem without sharing a design.
    if (SDL_strcasecmp(plr.szName, prv->pplr->szName)) return true;
    prv->iMax = StarsMax(prv->iMax, iVersion);
    const std::string szNameTerminated{pszName};
    if (iVersion > prv->iLatest || (iVersion == prv->iLatest && SDL_strcasecmp(szNameTerminated.c_str(), prv->szLatest) < 0)) {
        prv->iLatest = iVersion;
        CchCopyUtf8(std::span{prv->szLatest}, pszName);
    }
    if (iVersion > prv->iSame && FSameLibraryRace(prv->pplr, &plr)) {
        prv->iSame = iVersion;
        CchCopyUtf8(std::span{prv->szSame}, pszName);
    }
    return true;
}

INT_PTR  LibraryRaceSaveDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = AppCompatCallbackContext();

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
    std::string_view pszExisting;
    int iChoice;

    if (!FHasCurrentLibraryRace(app) || !FInitRaceLibrary(app)) return 0;
    rv.pplr = pplr;
    GetRaceLibraryStem(app, pplr->szName, std::span{rv.szStem});
    if (!FNativeEnumerateDirectory(app, app.lpui->szRaceLibraryRoot.data(), FindRaceVersion, &rv)) return 0;
    pszExisting = !fNewLibraryRace && app.lpui->szRaceLibraryPath[0] ? PszCurrentLibraryRaceName(app) : std::string_view{rv.szLatest};
    if (!pszExisting.empty()) {
        CchSnprintf(std::span{szName, sizeof(szName)}, "%s", pszExisting);
        CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", app.lpui->szRaceLibraryRoot.data(), szDirSep, szName);
        if (!FReadManagedRace(app, szPath, &plr)) goto LFail;
        if (FSameLibraryRace(pplr, &plr)) goto LReuse;
    }
    if (rv.szSame[0]) {
        CchCopyUtf8(std::span{szName}, rv.szSame);
        goto LReuse;
    }
    iChoice = !pszExisting.empty() ? static_cast<int>(ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_LIBRARY_RACE_SAVE), hwnd,
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
    CchSnprintf(std::span{app.lpui->szRaceLibraryPath}, "%s%s%s", app.lpui->szRaceLibraryRoot.data(), szDirSep, szName);
    SetSzRaceFile(app, szName);
    fNewLibraryRace = 0;
    return 1;
LFail:
    if (win32::IsWindow(hwnd)) AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryRaceSaveFailed).data(), win32::MB_ICONHAND);
    return 0;
}

std::int16_t FHasCurrentLibraryRace(APPSESSION &app) {
    return fNewLibraryRace || app.lpui->szRaceLibraryPath[0] != 0;
}

std::int16_t FLoadLibraryRace(APPSESSION &app, HWND hwnd, std::string_view pszName) {
    char szPath[512];
    char szName[256];
    RACESTATE rsSav;
    std::int16_t fResult;
    if (!FInitRaceLibrary(app) || !FPortableRaceFilename(app, pszName)) return 0;
    std::copy(pszName.begin(), pszName.end(), szName);
    szName[pszName.size()] = 0;
    CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", app.lpui->szRaceLibraryRoot.data(), szDirSep, szName);
    SaveRaceState(app, &rsSav);
    fResult = FWasRaceFile(app, szPath, 1);
    if (fResult <= 0) {
        RestoreRaceState(app, &rsSav, 1, 1);
        if (win32::IsWindow(hwnd)) AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryRaceOpenFailed).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    CchSnprintf(std::span{app.lpui->szRaceLibraryPath}, "%s", szPath);
    SetSzRaceFile(app, szName);
    fNewLibraryRace = 0;
    return 1;
}

std::string_view PszCurrentLibraryRaceName(APPSESSION &app) {
    const std::string_view szPath{app.lpui->szRaceLibraryPath.data()};
    const std::size_t ichSeparator = szPath.find_last_of(chDirSep);
    return ichSeparator == std::string_view::npos ? szPath : szPath.substr(ichSeparator + 1);
}

/* The new-game roster owns a filename for each design. Bind only this edit
 * to that destination; a previously imported design is not its save target. */
std::int16_t FCustomizeLibraryRace(APPSESSION &app, HWND hwnd, std::span<char, 256> pszName) {
    RACESTATE rsSav;
    char szPath[512];
    std::int16_t fResult;

    SaveRaceState(app, &rsSav);
    if (pszName[0]) {
        if (!FInitRaceLibrary(app) || !FPortableRaceFilename(app, pszName.data())) return 0;
        CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", app.lpui->szRaceLibraryRoot.data(), szDirSep, pszName.data());
        CchSnprintf(std::span{app.lpui->szRaceLibraryPath}, "%s", szPath);
        SetSzRaceFile(app, pszName.data());
        fNewLibraryRace = 0;
    } else {
        if (!FInitRaceLibrary(app)) return 0;
        app.lpui->szRaceLibraryPath[0] = 0;
        fNewLibraryRace = 1;
    }
    fResult = RaceCreationWizard(app, hwnd, 0, 0);
    if (fResult) CchSnprintf(pszName, "%s", PszCurrentLibraryRaceName(app));
    RestoreRaceState(app, &rsSav, !fResult, 1);
    return fResult;
}

std::int16_t FBeginLibraryRace(APPSESSION &app, HWND hwnd) {
    RACESTATE rsSav;
    std::int16_t fResult;
    SaveRaceState(app, &rsSav);
    if (!FInitRaceLibrary(app)) return 0;
    app.lpui->szRaceLibraryPath[0] = 0;
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

std::int16_t FEditLibraryRace(APPSESSION &app, HWND hwnd, std::string_view pszName) {
    RACESTATE rsSav;
    std::int16_t fResult;
    SaveRaceState(app, &rsSav);
    if (!FLoadLibraryRace(app, hwnd, pszName)) return 0;
    fResult = RaceCreationWizard(app, hwnd, 0, 0);
    RestoreRaceState(app, &rsSav, !fResult, 1);
    return fResult;
}

std::int16_t FDuplicateLibraryRace(APPSESSION &app, HWND hwnd, std::string_view pszName) {
    char szSource[512], szName[256], szPath[512] = "";
    const char *pch;
    RACESTATE rsSav;
    if (!FInitRaceLibrary(app) || !FPortableRaceFilename(app, pszName)) return 0;
    std::copy(pszName.begin(), pszName.end(), szName);
    szName[pszName.size()] = 0;
    CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s", app.lpui->szRaceLibraryRoot.data(), szDirSep, szName);
    SaveRaceState(app, &rsSav);
    if (FWasRaceFile(app, szSource, 1) <= 0) {
        RestoreRaceState(app, &rsSav, 1, 1);
        return 0;
    }
    pch = std::strrchr(szName, '.');
    if (pch) szName[pch - szName] = 0;
    SuggestRaceFilename(app, std::span{szName});
    if (!szName[0] || CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", app.lpui->szRaceLibraryRoot.data(), szDirSep, szName) <= 0) {
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

static void AddFileSelectionFilters(std::string_view text, win32::files::FILESELECTIONREQUEST &request) {
    request.rgFilters = ui::files::RgParseFilters(text);
}

static void AddLibraryFileSelectionFilters(APPSESSION &app, StringId ids,
                                           win32::files::FILESELECTIONREQUEST &request) {
    const auto szFilter = TextIO().pfnCompressed(app, ids);
    const auto pchEnd = std::find(szFilter.begin(), szFilter.end(), '\0');
    AddFileSelectionFilters(std::string_view{szFilter.data(), static_cast<std::size_t>(pchEnd - szFilter.begin())}, request);
}

static std::int16_t FSelectLibraryFile(HWND hwnd, win32::files::FILESELECTIONREQUEST request,
                                      std::span<char> szPath) {
    win32::files::FILESELECTIONRESULT result;
    request.cbPathMax = szPath.size();
    result = ui::files::ChooseFile(hwnd, std::move(request));
    if (const auto *pSelected = std::get_if<win32::files::FILESELECTIONSELECTED>(&result))
        return CchCopyUtf8(szPath, pSelected->szPath) >= 0;
    return 0;
}

static std::int16_t FSelectLibraryRaceFile(APPSESSION &app, HWND hwnd, bool fSave,
                                           std::span<char> szPath) {
    win32::files::FILESELECTIONREQUEST request;
    request.mode = fSave ? win32::files::FILESELECTIONMODE::save : win32::files::FILESELECTIONMODE::open;
    request.szInitialPath = szPath.data();
    request.fMustExist = !fSave;
    request.fConfirmOverwrite = fSave;
    AddLibraryFileSelectionFilters(app, idsLibraryRaceFilesFilter, request);
    return FSelectLibraryFile(hwnd, std::move(request), szPath);
}

std::int16_t FExportLibraryRace(APPSESSION &app, HWND hwnd, std::string_view pszName, std::optional<std::string_view> pszTarget) {
    char szSource[512], szTarget[512], szTemp[512], szValidate[512];
    char szName[256];
    RACESTATE rsSav;
    if (pszName.size() >= sizeof(szName) || pszName.find('\0') != std::string_view::npos) return 0;
    std::copy(pszName.begin(), pszName.end(), szName);
    szName[pszName.size()] = 0;
    if (!FInitRaceLibrary(app) || !FPortableRaceFilename(app, pszName)) return 0;
    CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s", app.lpui->szRaceLibraryRoot.data(), szDirSep, szName);
    if (!pszTarget) {
        szTarget[0] = 0;
        CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s", szName);
        if (!FSelectLibraryRaceFile(app, hwnd, true, std::span{szTarget})) return 0;
        pszTarget = std::string_view(szTarget);
    }
    if (pszTarget->size() >= sizeof(szTarget) || pszTarget->find('\0') != std::string_view::npos) return 0;
    if (pszTarget->data() != szTarget) std::copy(pszTarget->begin(), pszTarget->end(), szTarget);
    szTarget[pszTarget->size()] = 0;
    SaveRaceState(app, &rsSav);
    if (FWasRaceFile(app, szSource, 1) <= 0 || SDL_strcasecmp(szSource, szTarget) == 0) {
        RestoreRaceState(app, &rsSav, 1, 1);
        if (win32::IsWindow(hwnd)) AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryRaceExportFailed).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    if (!FCopyRaceExport(szSource, szTarget, std::span{szTemp})) {
        RestoreRaceState(app, &rsSav, 1, 1);
        if (win32::IsWindow(hwnd)) AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryRaceExportFailed).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    CchSnprintf(std::span{szValidate, sizeof(szValidate)}, "%s", szTemp);
    if (FWasRaceFile(app, szValidate, 1) <= 0 || ReplaceNativeFile(szTemp, szTarget) != 0) {
        RemoveNativeFile(szTemp);
        RestoreRaceState(app, &rsSav, 1, 1);
        if (win32::IsWindow(hwnd)) AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryRaceExportFailed).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    RestoreRaceState(app, &rsSav, 1, 1);
    return 1;
}

std::int16_t FExportLibraryOrders(APPSESSION &app, HWND hwnd, std::optional<std::string_view> pszTarget) {
    win32::files::FILESELECTIONREQUEST request;
    char szBaseSav[sizeof(app.state.szBase)], szBaseStem[sizeof(app.state.szBase)], szName[256], szSuggested[256], szTarget[512], szFilter[64];
    char szStageBase[sizeof(app.state.szBase)], szStage[sizeof(app.state.szBase)], szStageHist[sizeof(app.state.szBase)];
    char szSource[sizeof(app.state.szBase)], szSourceHist[sizeof(app.state.szBase)], szPreviousHist[sizeof(app.state.szBase)];
    char szTemp[512], szMessage[640], szExt[4];
    std::optional<std::size_t> ichSlash, ichPreviousSlash;
    std::size_t ichName, ichTargetSlash, ichDot;
    std::string_view szTargetView;
    const char *pszLibraryName;
    std::size_t cchTargetPath;
    StringId idsSaveError;
    NATIVEFILE *pf;
    std::uint32_t dwTick;

    int cch;
    std::int16_t fSubmitSav, fSaved, fDirtySav, fHistoryExists, fHistoryReplaced, fHistoryBackup;

    if (!FInitLibrary() || !app.state.lpPlanets || app.state.game.lid == 0 || app.state.idPlayer == iplrNone || app.state.idPlayer < 0 || app.state.idPlayer >= 16) return 0;
    if (!LibraryPathIO().pfnUnder(szLibraryRoot, app.state.szBase)) return 0;
    RememberLibraryGame(app);

    pszLibraryName = app.lpui->szLibraryExchangeName.data();
    if (!pszLibraryName[0]) {
        ichSlash = IchLibrarySeparator(app.state.szBase);
        if (!ichSlash) return 0;
        ichPreviousSlash = IchLibrarySeparator(std::string_view{app.state.szBase, *ichSlash});
        ichName = ichPreviousSlash ? *ichPreviousSlash + 1 : 0;
        cch = CchSnprintf(std::span{szName, sizeof(szName)}, "%.*s", static_cast<int>(*ichSlash - ichName),
                          app.state.szBase + ichName);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szName)) return 0;
        pszLibraryName = szName;
    }
    cch = CchSnprintf(std::span{szSuggested, sizeof(szSuggested)}, "%s.x%d", pszLibraryName, app.state.idPlayer + 1);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szSuggested)) return 0;
    if (!pszTarget) {
        std::ranges::fill(szTarget, 0);
        CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s", szSuggested);
        cch = CchSnprintf(std::span{szFilter, sizeof(szFilter)}, TextIO().pfnCompressed(app, idsLibraryOrdersExportFilter).data(),
                          app.state.idPlayer + 1, app.state.idPlayer + 1);
        if (cch <= 0 || static_cast<std::size_t>(cch) + 2 >= sizeof(szFilter)) return 0;
        CchSnprintf(std::span{szExt, sizeof(szExt)}, "x%d", app.state.idPlayer + 1);
        request.mode = win32::files::FILESELECTIONMODE::save;
        request.szInitialPath = szSuggested;
        request.szDefaultExtension = szExt;
        AddFileSelectionFilters(std::string_view{szFilter, static_cast<std::size_t>(cch)}, request);
        if (!FSelectLibraryFile(hwnd, std::move(request), std::span{szTarget})) return 0;
        pszTarget = std::string_view{szTarget};
    }

    if (pszTarget->size() >= sizeof(szTarget)) return 0;
    if (pszTarget->data() != szTarget) {
        std::copy(pszTarget->begin(), pszTarget->end(), szTarget);
        szTarget[pszTarget->size()] = 0;
    }
    szTargetView = szTarget;
    ichTargetSlash = szTargetView.find_last_of("/\\");
    ichDot = szTargetView.rfind('.');
    cchTargetPath = szTargetView.size();
    if (ichDot != std::string_view::npos && (ichTargetSlash == std::string_view::npos || ichDot > ichTargetSlash)) {
        szTarget[ichDot] = 0;
        cchTargetPath = ichDot;
    }
    if (!szTarget[0]) return 0;
    cch = CchSnprintf(std::span{szTarget + cchTargetPath, sizeof(szTarget) - cchTargetPath}, ".x%d", app.state.idPlayer + 1);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget) - cchTargetPath) return 0;

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
    ichDot = std::string_view{szBaseStem}.rfind('.');
    ichSlash = IchLibrarySeparator(szBaseStem);
    if (ichDot != std::string_view::npos && (!ichSlash || ichDot > *ichSlash)) szBaseStem[ichDot] = 0;
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

std::int16_t FDeleteLibraryRace(APPSESSION &app, HWND hwnd, std::string_view pszName) {
    char szPath[512], szMessage[320];
    char szName[256];
    std::optional<std::int64_t> cbFile;
    if (pszName.size() >= sizeof(szName) || pszName.find('\0') != std::string_view::npos) return 0;
    std::copy(pszName.begin(), pszName.end(), szName);
    szName[pszName.size()] = 0;
    if (!FInitRaceLibrary(app) || !FPortableRaceFilename(app, pszName)) return 0;
    CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", app.lpui->szRaceLibraryRoot.data(), szDirSep, szName);
    if (!(cbFile = FNativeFileSize(szPath)) || (*cbFile) <= 0) return 0;
    CchSnprintf(std::span{szMessage, sizeof(szMessage)}, TextIO().pfnCompressed(app, idsLibraryRaceDeletePrompt).data(), szName);
    if (CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage, TextIO().pfnCompressed(app, idsLibraryTitle).data(), win32::MB_YESNO | win32::MB_ICONQUESTION | win32::MB_TASKMODAL) != win32::IDYES) return 0;
    if (RemoveNativeFile(szPath) != 0) {
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryRaceDeleteFailed).data(), win32::MB_ICONHAND);
        return 0;
    }
    if (!std::strcmp(app.lpui->szRaceLibraryPath.data(), szPath)) app.lpui->szRaceLibraryPath[0] = 0;
    return 1;
}

static std::int16_t FImportLibraryRaceImpl(APPSESSION &app, HWND hwnd, std::optional<std::string_view> pszSource,
                                           std::optional<std::string_view> pszName) {
    char szSource[512], szName[256], szStem[256], szPath[512], szStage[512], szStageDir[512];
    char *pch, *pchSlash, *pchExt;
    RACESTATE rsSav;
    std::int16_t fValid;
    std::int16_t fLegacy;
    std::string_view pszSourceView;
    if (!pszSource) {
        std::ranges::fill(szSource, 0);
        if (!FSelectLibraryRaceFile(app, hwnd, false, std::span{szSource})) return 0;
        pszSource.emplace(szSource);
    }
    pszSourceView = *pszSource;
    if (pszSourceView.size() >= sizeof(szSource) || pszSourceView.find('\0') != std::string_view::npos) return 0;
    if (pszSourceView.data() != szSource) std::ranges::copy(pszSourceView, std::span{szSource}.begin());
    szSource[pszSourceView.size()] = 0;
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
    if (pszName) {
        if (!FInitRaceLibrary(app) || !FPortableRaceFilename(app, *pszName)) goto LRestore;
        CchSnprintf(std::span{app.lpui->szRaceLibraryName}, "%s", *pszName);
        if (!FRaceLibraryNameAvailable(app, app.lpui->szRaceLibraryName.data())) goto LRestore;
        CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", app.lpui->szRaceLibraryRoot.data(), szDirSep, app.lpui->szRaceLibraryName.data());
    } else if (!FChooseLibraryRaceName(app, hwnd, szName, std::span{szPath})) goto LRestore;
    /* Legacy reads convert names and credentials in memory; retain that state,
     * never the original CP1252 bytes or password hash. */
    if (fLegacy) {
        if (!FSaveLibraryRace(app, hwnd, app.lpui->szRaceLibraryName.data(), &app.state.vplr)) goto LRestore;
        return 1;
    }
    if (!FCreateRaceStagePath(app, std::span{szStageDir}, std::span{szStage})) goto LRestore;
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
    CchSnprintf(std::span{app.lpui->szRaceLibraryPath}, "%s", szPath);
    SetSzRaceFile(app, std::strrchr(szPath, chDirSep) + 1);
    return 1;
LRestore:
    RestoreRaceState(app, &rsSav, 1, 1);
    return 0;
}

std::int16_t FImportLibraryRace(APPSESSION &app, HWND hwnd, std::optional<std::string_view> pszSource) {
    return FImportLibraryRaceImpl(app, hwnd, pszSource, std::nullopt);
}

std::int16_t FImportLibraryRaceAs(APPSESSION &app, HWND hwnd, std::optional<std::string_view> pszSource,
                                 std::optional<std::string_view> pszName) {
    return FImportLibraryRaceImpl(app, hwnd, pszSource, pszName);
}

std::int16_t FImportLibraryRaceAndEdit(APPSESSION &app, HWND hwnd, std::optional<std::string_view> pszSource) {
    const char *pszName;
    if (!FImportLibraryRace(app, hwnd, pszSource)) return 0;
    pszName = std::strrchr(app.lpui->szRaceLibraryPath.data(), chDirSep);
    if (pszName) pszName++;
    else pszName = app.lpui->szRaceLibraryPath.data();
    /* Import remains successful when the user cancels customization. */
    FEditLibraryRace(app, hwnd, pszName);
    return 1;
}

/* Inspect record framing and BOF metadata without calling FOpenFile, which
 * binds validation to the active game globals. */
std::int16_t FInspectImportFile(std::string_view pszFile, DtFileType dt, int iplr,
                                 std::int32_t &lidGame, std::uint16_t *pturn, std::uint16_t *pwGen, std::int16_t &fMulti) {
    NATIVEFILE *pf;
    HDR hdr;
    RTBOF rtbof;
    std::int64_t cbFile;
    std::optional<std::int64_t> ib;
    std::int16_t fComplete;

    pf = PfOpenNativeFile(pszFile, NATIVEFILEMODE::read);
    if (!pf) return 0;
    fComplete = 0;
    if (ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::end) != 0 || !(ib = LNativePosition(pf)) || *ib < 22 || ISeekNativeFile(pf, 0, NATIVESEEKORIGIN::begin) != 0) goto LDone;
    cbFile = *ib;
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
        lidGame = rtbof.lidGame;
        if (pturn) *pturn = rtbof.turn;
        if (pwGen) *pwGen = rtbof.wGen;
        fMulti = rtbof.fMulti;
        goto LDone;
    }
    while (FReadLibraryHdr(pf, &hdr)) {
        ib = LNativePosition(pf);
        if (!ib || *ib < 0 || *ib + hdr.cb > cbFile) break;
        if (hdr.rt == rtEOF) {
            /* Orders and history writers use an empty EOF; host and turn
             * files carry the two-byte trailer. Both must end at EOF. */
            if (hdr.cb != 2 && !(hdr.cb == 0 && (dt == dtLog || dt == dtHist))) break;
            if (*ib + hdr.cb != cbFile) break;
            fComplete = 1;
            break;
        }
        if (ISeekNativeFile(pf, hdr.cb, NATIVESEEKORIGIN::current) != 0) break;
    }
    if (fComplete) {
        lidGame = rtbof.lidGame;
        if (pturn) *pturn = rtbof.turn;
        if (pwGen) *pwGen = rtbof.wGen;
        fMulti = rtbof.fMulti;
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

struct LIBRARYFAMILYMEMBER {
    int iplr;
    DtFileType dt;
};

static std::optional<LIBRARYFAMILYMEMBER> FImportFamilyMember(std::string_view pszName, std::string_view pszStem) {
    std::string_view pszExt;
    int iplr;
    DtFileType dt;
    char chType;
    int iplrFamily;
    if (pszName.size() <= pszStem.size() || SDL_strncasecmp(pszName.data(), pszStem.data(), pszStem.size()) ||
        pszName[pszStem.size()] != '.') return std::nullopt;
    pszExt = pszName.substr(pszStem.size() + 1);
    if (pszExt.size() == 2 && SDL_strncasecmp(pszExt.data(), "xy", pszExt.size()) == 0) {
        iplr = -1;
        dt = dtXY;
        return LIBRARYFAMILYMEMBER{iplr, dt};
    }
    if (pszExt.size() == 3 && SDL_strncasecmp(pszExt.data(), "hst", pszExt.size()) == 0) {
        iplr = -1;
        dt = dtHost;
        return LIBRARYFAMILYMEMBER{iplr, dt};
    }
    if (pszExt.size() < 2 || pszExt.size() > 3) return std::nullopt;
    if (pszExt[0] != 'm' && pszExt[0] != 'M' && pszExt[0] != 'h' && pszExt[0] != 'H' &&
        pszExt[0] != 'x' && pszExt[0] != 'X') return std::nullopt;
    chType = SDL_tolower(pszExt[0]);
    if (pszExt[1] < '1' || pszExt[1] > '9' ||
        (pszExt.size() == 3 && (pszExt[2] < '0' || pszExt[2] > '9'))) return std::nullopt;
    iplrFamily = pszExt[1] - '0';
    if (pszExt.size() == 3) iplrFamily = iplrFamily * 10 + pszExt[2] - '0';
    if (iplrFamily > 16) return std::nullopt;
    iplr = iplrFamily - 1;
    dt = chType == 'm' ? dtTurn : chType == 'h' ? dtHist : dtLog;
    return LIBRARYFAMILYMEMBER{iplr, dt};
}

static bool FMalformedImportFamilyMember(std::string_view pszName, std::string_view pszStem) {
    std::size_t cchStem = pszStem.size();
    char chType;
    if (pszName.size() <= cchStem + 1 || SDL_strncasecmp(pszName.data(), pszStem.data(), cchStem) ||
        pszName[cchStem] != '.') return false;
    chType = SDL_tolower(pszName[cchStem + 1]);
    return chType == 'm' || chType == 'h' || chType == 'x';
}

struct LIBRARYGAMECOPY {
    std::string_view pszSource;
    std::string_view pszTarget;
    std::string_view pszSourceName;
    std::string_view pszTargetName;
    std::int16_t fBad;
};

static std::int16_t FReadLibraryMetadata(std::string_view pszDirectory, std::span<char> pszName,
                                    int &iplr, long long &lPlayed, std::span<char> pszExchangeName) {
    char szFile[512], szLine[256];
    std::optional<std::size_t> cchLine;
    std::size_t cchLineContent, ichLineEnd;
    NATIVEFILE *pf;
    int iVersion, cchText, cch;
    std::int16_t fRead;
    auto trimLine = [&](std::size_t cch) {
        std::string_view szLineView{szLine, cch};
        cchLineContent = szLineView.find('\0');
        if (cchLineContent == std::string_view::npos) cchLineContent = szLineView.size();
        ichLineEnd = szLineView.substr(0, cchLineContent).find_first_of("\r\n");
        if (ichLineEnd != std::string_view::npos) cchLineContent = ichLineEnd;
        szLine[cchLineContent] = 0;
    };
    if (pszName.empty() || pszExchangeName.empty()) return 0;
    cch = CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s.library", pszDirectory, szDirSep);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szFile)) return 0;
    pf = PfOpenNativeFile(szFile, NATIVEFILEMODE::read);
    if (!pf) return 0;
    fRead = CNativeScan(pf, "%d\n", &iVersion) == 1 && (cchLine = CchGetNativeLine(std::span{szLine}, pf)).has_value();
    if (fRead) {
        trimLine(*cchLine);
        cchText = CchSnprintf(pszName, "%s", szLine);
        fRead = cchText > 0 && static_cast<std::size_t>(cchText) < pszName.size() &&
                CNativeScan(pf, "%d\n%lld\n", &iplr, &lPlayed) == 2;
    }
    if (fRead && iVersion == 2 && (cchLine = CchGetNativeLine(std::span{szLine}, pf)).has_value()) {
        trimLine(*cchLine);
        cchText = CchSnprintf(pszExchangeName, "%s", szLine);
        fRead = cchText > 0 && static_cast<std::size_t>(cchText) < pszExchangeName.size();
    } else if (fRead && iVersion == 1) {
        cchText = CchSnprintf(pszExchangeName, "%s",
                              std::string_view{pszName.data(), static_cast<std::size_t>(cchText)});
        fRead = cchText > 0 && static_cast<std::size_t>(cchText) < pszExchangeName.size();
    } else fRead = 0;
    ICloseNativeFile(pf);
    return fRead && (iVersion == 1 || iVersion == 2) && !pszName.empty() && pszName[0] &&
           iplr >= -1 && iplr < 16 && lPlayed >= 0 && !pszExchangeName.empty() && pszExchangeName[0];
}

static std::int16_t FLibraryNameAvailableExcept(APPSESSION &app, std::string_view pszName,
                                                std::optional<std::string_view> pszExcept) {
    LIBRARYNAMECHECK lnc;
    lnc.pszDirectory = szLibraryRoot;
    lnc.pszName = pszName;
    lnc.fFound = 0;
    if (!FNativeEnumerateDirectory(app, szLibraryRoot, FindLibraryName, &lnc)) return 0;
    const std::string szName{pszName};
    if (lnc.fFound && pszExcept && !SDL_strcasecmp(szName.c_str(), std::string{*pszExcept}.c_str())) return 1;
    return !lnc.fFound;
}

static bool FCopyLibraryGameEntry(APPSESSION &app, LIBRARYGAMECOPY *plgc, std::string_view pszName) {
    std::optional<NATIVEPATHINFO> npiPath;
    char szSource[512], szTarget[512];
    const char *pszTargetFile;
    LIBRARYGAMECOPY lgb;
    int cch;
    std::optional<LIBRARYFAMILYMEMBER> lfm;

    if (pszName == "." || pszName == "..") return true;
    cch = CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s", plgc->pszSource, szDirSep, pszName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szSource)) { plgc->fBad = 1; return false; }
    if (!(npiPath = FNativePathInfo(szSource)) || npiPath->fLink) {
        plgc->fBad = 1;
        return false;
    }
    if (npiPath->fDirectory) {
        /* Rename removes the original tree after publication, so every directory
         * must survive staging, including backup, backupN and sidecar folders. */
        cch = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s", plgc->pszTarget, szDirSep, pszName);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget)) { plgc->fBad = 1; return false; }
        const std::string szName{pszName};
        if (!FLibraryNameAvailable(app, plgc->pszTarget, szName.c_str()) ||
            CreateNativeDir(szTarget) != 0) { plgc->fBad = 1; return false; }
        /* Backup files use the original basename family, with unrelated sidecars retained verbatim. */
        lgb.pszSource = szSource;
        lgb.pszTarget = szTarget;
        lgb.pszSourceName = plgc->pszSourceName;
        lgb.pszTargetName = plgc->pszTargetName;
        lgb.fBad = 0;
        if (!FNativeEnumerateDirectory(app, szSource, FCopyLibraryGameEntry, &lgb) || lgb.fBad) {
            plgc->fBad = 1;
            return false;
        }
        return true;
    }
    lfm = FImportFamilyMember(pszName, plgc->pszSourceName);
    if (lfm) {
        if (!FFormatLibraryFamilyPath(std::span{szTarget}, plgc->pszTarget,
                                      plgc->pszTargetName, lfm->dt, lfm->iplr)) { plgc->fBad = 1; return false; }
    } else if (pszName != ".library" && pszName != ".library.tmp") {
        /* Optional external universe definitions and other sidecars keep their own names. */
        cch = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s", plgc->pszTarget, szDirSep, pszName);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget)) { plgc->fBad = 1; return false; }
    } else return true;
    /* Renamed family members and unchanged sidecars can map to the same name.
     * Reject portable case collisions and never truncate an earlier staged file. */
    pszTargetFile = std::strrchr(szTarget, chDirSep);
    if (!pszTargetFile || !FLibraryNameAvailable(app, plgc->pszTarget, pszTargetFile + 1) ||
        !FCopyNativeFileExclusive(szSource, szTarget)) { plgc->fBad = 1; return false; }
    return true;
}

static std::int16_t FBackupDirectoryName(std::string_view pszName) {
    std::size_t cch = pszName.size();
    std::size_t i;

    if (!pszName.starts_with("backup")) return 0;
    if (cch == 6) return 1;
    if (cch == 7 || cch == 8) i = 6;
    else if (cch == 10 && pszName[6] == '.') i = 7;
    else return 0;
    for (; i < cch; i++) if (pszName[i] < '0' || pszName[i] > '9') return 0;
    return 1;
}

bool CopyImportEntry(APPSESSION &, LIBRARYIMPORT *pli, std::string_view pszName) {
    char szSource[512], szTarget[512];
    std::optional<LIBRARYFAMILYMEMBER> lfm;
    int iplr;
    DtFileType dt;
    std::int32_t lidGame;
    std::uint16_t turn;
    std::int16_t fMulti;
    std::uint16_t wGen;

    lfm = FImportFamilyMember(pszName, pli->pszStem);
    if (!lfm) {
        if (FMalformedImportFamilyMember(pszName, pli->pszStem)) pli->fBad = 1;
        if (FBackupDirectoryName(pszName)) pli->fBackup = 1;
        return true;
    }
    dt = lfm->dt;
    iplr = lfm->iplr;
    CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s", pli->pszSource, szDirSep, pszName);
    if (!FFormatLibraryFamilyPath(std::span{szTarget}, pli->pszTarget, pli->pszName, dt, iplr)) {
        pli->fBad = 1;
        return false;
    }
    if (pli->fCopy) {
        if (!FCopyNativeFile(szSource, szTarget)) { pli->fBad = 1; return false; }
        return true;
    }
    if (!FInspectImportFile(szSource, dt, iplr < 0 ? iplrNone : iplr, lidGame, &turn, &wGen, fMulti)) {
        pli->fBad = 1;
        return true;
    }
    if (dt == dtXY) {
        if (pli->fSawXY) { pli->fBad = 1; return true; }
        pli->fSawXY = 1;
        if (pli->lidGame && pli->lidGame != lidGame) { pli->fBad = 1; return true; }
        pli->lidGame = lidGame;
        pli->fValid = 1;
    } else if (dt == dtTurn) {
        if ((pli->lidGame && lidGame != pli->lidGame) || iplr >= 16 || (pli->grfPlayers & (1U << iplr))) { pli->fBad = 1; return true; }
        if (!pli->lidGame) pli->lidGame = lidGame;
        pli->grfPlayers |= 1U << iplr;
        pli->rgturn[iplr] = static_cast<std::int16_t>(turn);
    } else if (dt == dtHost || dt == dtHist || dt == dtLog) {
        if (pli->lidGame && lidGame != pli->lidGame) { pli->fBad = 1; return true; }
        if (!pli->lidGame) pli->lidGame = lidGame;
        if (dt == dtHost) pli->fSawHost = 1;
    }
    /* The first pass validates every recognized file; copying happens only
     * after the entire family has passed validation. */
    pli->fCopied++;
    return true;
}

struct LIBRARYBACKUP {
    std::string_view pszSource;
    std::string_view pszTarget;
    std::string_view pszStem;
    std::string_view pszName;
    std::string_view pszBackup;
    std::int16_t fBad;
};

static bool CopyBackupEntry(APPSESSION &, LIBRARYBACKUP *plb, std::string_view pszName) {
    std::optional<LIBRARYFAMILYMEMBER> lfm;
    char szSource[512], szTarget[512], szDirectory[512];
    lfm = FImportFamilyMember(pszName, plb->pszStem);
    if (!lfm) return true;
    CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s%s%s", plb->pszSource, szDirSep, plb->pszBackup, szDirSep, pszName);
    CchSnprintf(std::span{szDirectory, sizeof(szDirectory)}, "%s%s%s", plb->pszTarget, szDirSep, plb->pszBackup);
    if (!FFormatLibraryFamilyPath(std::span{szTarget}, szDirectory, plb->pszName, lfm->dt, lfm->iplr)) {
        plb->fBad = 1;
        return false;
    }
    if (!FCopyNativeFile(szSource, szTarget)) { plb->fBad = 1; return false; }
    return true;
}

bool CopyBackupDirectory(APPSESSION &app, LIBRARYIMPORT *pli, std::string_view pszName) {
    LIBRARYBACKUP lb;
    char szSource[512], szTarget[512];
    if (!FBackupDirectoryName(pszName)) return true;
    CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s", pli->pszSource, szDirSep, pszName);
    if (!FNativeDirectoryExists(szSource)) return true;
    CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s", pli->pszTarget, szDirSep, pszName);
    if (CreateNativeDir(szTarget) != 0) { pli->fBad = 1; return false; }
    lb.pszSource = pli->pszSource;
    lb.pszTarget = pli->pszTarget;
    lb.pszStem = pli->pszStem;
    lb.pszName = pli->pszName;
    lb.pszBackup = pszName;
    lb.fBad = 0;
    if (!FNativeEnumerateDirectory(app, szSource, CopyBackupEntry, &lb) || lb.fBad) { pli->fBad = 1; return false; }
    return true;
}

static bool RemoveImportChild(APPSESSION &, const char *pszRoot, std::string_view pszFile) {
    char szFile[512];
    CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s%s", pszRoot, szDirSep, pszFile);
    RemoveNativePath(szFile);
    return true;
}

bool RemoveImportFile(APPSESSION &app, const char *pszRoot, std::string_view pszFile) {
    char szFile[512];
    CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s%s", pszRoot, szDirSep, pszFile);
    if (FNativeDirectoryExists(szFile)) {
        FNativeEnumerateDirectory(app, szFile, RemoveImportChild, szFile);
        RemoveNativePath(szFile);
    } else RemoveNativePath(szFile);
    return true;
}

static bool RemoveImportTreeEntry(APPSESSION &app, const char *pszRoot, std::string_view pszFile) {
    std::optional<NATIVEPATHINFO> npiPath;
    char szPath[512];

    if (CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", pszRoot, szDirSep, pszFile) <= 0 ||
        !(npiPath = FNativePathInfo(szPath))) return false;
    if (npiPath->fDirectory && !npiPath->fLink) {
        if (!FNativeEnumerateDirectory(app, szPath, RemoveImportTreeEntry, szPath)) return false;
    }
    return RemoveNativePath(szPath) == 0;
}

static std::int16_t FPathIsZip(std::string_view pszPath) {
    const std::size_t ichExtension = pszPath.rfind('.');
    if (ichExtension == std::string_view::npos) return 0;
    const auto szExtension = pszPath.substr(ichExtension);
    return szExtension.size() == 4 && SDL_tolower(static_cast<unsigned char>(szExtension[1])) == 'z' &&
           SDL_tolower(static_cast<unsigned char>(szExtension[2])) == 'i' &&
           SDL_tolower(static_cast<unsigned char>(szExtension[3])) == 'p';
}

static std::int16_t FCreateLibraryZipStage(std::span<char> pszStage, std::string_view pszPurpose) {
    int i, cch;
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(pszStage, "%s%s.%s.%u-%u", szLibraryRoot, szDirSep,
                          pszPurpose, DwTickCount(), idLibraryZipStage++);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= pszStage.size()) return 0;
        if (CreateNativeDir(pszStage.data()) == 0) return 1;
        if (!FNativeDirectoryExists(pszStage.data())) return 0;
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

static bool CopyGameExportEntry(APPSESSION &, LIBRARYEXPORT *ple, std::string_view pszName) {
    char szSource[512], szTarget[512];
    std::optional<LIBRARYFAMILYMEMBER> lfm;

    lfm = FImportFamilyMember(pszName, ple->pszStem);
    if (!lfm) {
        /* A malformed companion should not silently make an incomplete export. */
        if (FMalformedImportFamilyMember(pszName, ple->pszStem)) ple->fBad = 1;
        return true;
    }
    CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s", ple->pszSource, szDirSep, pszName);
    CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s", ple->pszTarget, szDirSep, pszName);
    if (!FCopyNativeFile(szSource, szTarget)) { ple->fBad = 1; return false; }
    if (lfm->dt == dtXY) ple->fSawXY = 1;
    if (lfm->dt == dtTurn || lfm->dt == dtHost) ple->fSawPlayable = 1;
    return true;
}

static std::int16_t CreateLibraryExportStage(std::span<char> pszDirectory, std::string_view pszParent) {
    std::uint32_t dwTick = DwTickCount();
    int i, cch;
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(pszDirectory, "%s%s.stars-game-export-%u-%u",
                          pszParent, szDirSep, dwTick, idLibraryExportStage++);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= pszDirectory.size()) return 0;
        if (CreateNativeDir(pszDirectory.data()) == 0) return 1;
        if (!FNativeDirectoryExists(pszDirectory.data())) return 0;
    }
    return 0;
}

struct LIBRARYEXPORTMOVE {
    const char *pszSource;
    const char *pszTarget;
    std::int16_t fBad;
};

static bool MoveGameExportEntry(APPSESSION &, LIBRARYEXPORTMOVE *plem, std::string_view pszName) {
    char szSource[512], szTarget[512];
    CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s", plem->pszSource, szDirSep, pszName);
    CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s", plem->pszTarget, szDirSep, pszName);
    if (RenameNativeFile(szSource, szTarget) != 0) { plem->fBad = 1; return false; }
    return true;
}

std::int16_t FExportLibraryGame(APPSESSION &app, HWND hwnd, std::string_view pszName, std::optional<std::string_view> pszTarget) {
    LIBRARYIMPORT li;
    LIBRARYEXPORT le;
    LIBRARYEXPORTMOVE lem;
    win32::files::FILESELECTIONREQUEST request;
    char szSource[512], szDestination[512], szStage[512], szTarget[512];
    char szName[256];
    char szZipName[512], szZipStage[512], szZipGame[512], szMessage[256];
    int cch, iChoice;
    std::size_t cchSource, cchName, cchDestination, cchStage;
    std::int16_t fZipStage;

    if (pszName.size() >= sizeof(szName) || pszName.find('\0') != std::string_view::npos) return 0;
    std::copy(pszName.begin(), pszName.end(), szName);
    szName[pszName.size()] = 0;
    if (!pszTarget) {
        if (!std::getenv("STARS_CLIENT_EXPORT_GAME")) {
            TextIO().pfnCchGetString(app, idsLibraryExportZipPrompt, std::span{szMessage});
            iChoice = CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage,
                                 TextIO().pfnCompressed(app, idsLibraryExportGameTitle).data(), win32::MB_YESNOCANCEL | win32::MB_ICONQUESTION | win32::MB_TASKMODAL);
            if (iChoice == IDCANCEL) return 0;
            if (iChoice == win32::IDYES) {
                std::ranges::fill(szZipName, 0);
                CchSnprintf(std::span{szZipName, sizeof(szZipName)}, "%s.zip", szName);
                request.mode = win32::files::FILESELECTIONMODE::save;
                request.szInitialPath = szZipName;
                request.szDefaultExtension = "zip";
                request.fConfirmOverwrite = true;
                AddLibraryFileSelectionFilters(app, idsLibraryZipFilesFilter, request);
                if (!FSelectLibraryFile(hwnd, std::move(request), std::span{szZipName})) return 0;
                return FExportSelectedLibraryGame(app, hwnd, std::string_view(szZipName));
            }
        }
        CompatFolderIO().pfnShowOpenFolderDialog(LibraryExportFolderSelected, PvLibraryFolderContext(hwnd), nullptr, nullptr, 0);
        return 1;
    }
    if (!FInitLibrary() || !FPortableLibraryName(app, pszName) || pszTarget->size() >= sizeof(szTarget) ||
        pszTarget->find('\0') != std::string_view::npos) return 0;
    std::copy(pszTarget->begin(), pszTarget->end(), szTarget);
    szTarget[pszTarget->size()] = 0;
    cch = CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s", szLibraryRoot, szDirSep, szName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szSource) || !FNativeDirectoryExists(szSource)) return 0;
    cchSource = static_cast<std::size_t>(cch);
    cchName = pszName.size();
    if (FPathIsZip(szTarget)) {
        if (SDL_strncasecmp(szTarget, szSource, cchSource) == 0 &&
            (szTarget[cchSource] == 0 || szTarget[cchSource] == '/' || szTarget[cchSource] == '\\')) goto LFail;
        fZipStage = FCreateLibraryZipStage(std::span{szZipStage}, "zip-exporting");
        if (!fZipStage) goto LFail;
        if (!FExportLibraryGame(app, hwnd, pszName, std::string_view(szZipStage))) goto LZipFail;
        cch = CchSnprintf(std::span{szZipGame, sizeof(szZipGame)}, "%s%s%s", szZipStage, szDirSep, szName);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szZipGame) || !FZipLibraryDirectory(app, szZipGame, szTarget)) goto LZipFail;
        FNativeEnumerateDirectory(app, szZipGame, RemoveImportTreeEntry, szZipGame);
        RemoveNativePath(szZipGame);
        RemoveNativePath(szZipStage);
        return 1;
LZipFail:
        FNativeEnumerateDirectory(app, szZipStage, RemoveImportTreeEntry, szZipStage);
        RemoveNativePath(szZipStage);
        goto LFail;
    }
    if (SDL_strncasecmp(szTarget, szSource, cchSource) == 0 &&
        (szTarget[cchSource] == 0 || szTarget[cchSource] == '/' || szTarget[cchSource] == '\\')) goto LFail;
    cch = CchSnprintf(std::span{szDestination, sizeof(szDestination)}, "%s%s%s", szTarget, szDirSep, szName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szDestination) || FNativeDirectoryExists(szDestination)) goto LFail;
    cchDestination = static_cast<std::size_t>(cch);
    /* The loader and backup writer keep complete paths in szBase. Check the
     * longest companion before copying, rather than truncating child paths. */
    if (cchDestination + cchName + sizeof("/backup.999/.m16") > sizeof(app.state.szBase)) goto LFail;
    if (!CreateLibraryExportStage(std::span{szStage}, szTarget)) goto LFail;
    cchStage = std::string_view(szStage).size();
    if (cchStage + cchName + sizeof("/backup.999/.m16") > sizeof(szStage)) goto LStageFail;

    le = {};
    le.pszSource = szSource;
    le.pszTarget = szStage;
    le.pszStem = szName;
    if (!FNativeEnumerateDirectory(app, szSource, CopyGameExportEntry, &le) || le.fBad ||
        !le.fSawXY || !le.fSawPlayable) goto LStageFail;

    li = {};
    li.pszSource = szSource;
    li.pszTarget = szStage;
    li.pszStem = szName;
    li.pszName = szName;
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

std::int16_t FExportSelectedLibraryGame(APPSESSION &app, HWND hwnd, std::optional<std::string_view> pszTarget) {
    char szDir[512];
    char szName[256];
    if (!FSelectedLibraryGameName(hwnd, szName)) return 0;
    if (!pszTarget) return FExportLibraryGame(app, hwnd, szName, std::nullopt);
    if (app.state.game.fDirty && app.state.lpPlanets && app.state.idPlayer != iplrNone) {
        CchSnprintf(std::span{szDir, sizeof(szDir)}, "%s%s%s", szLibraryRoot, szDirSep, szName);
        if (LibraryPathIO().pfnUnder(szDir, app.state.szBase)) PromptSaveGame(app);
    }
    return FExportLibraryGame(app, hwnd, szName, pszTarget);
}

void ShowLibraryExportFolderPicker(HWND hwnd) {
    CompatFolderIO().pfnShowOpenFolderDialog(LibraryExportFolderSelected, PvLibraryFolderContext(hwnd), nullptr, nullptr, 0);
}

static bool FindImportUniverse(APPSESSION &, std::span<char, 256> pszStem, std::string_view pszName) {
    const std::size_t ichExt = pszName.rfind('.');
    if (ichExt == std::string_view::npos || pszName.size() - ichExt != 3 ||
        SDL_strncasecmp(pszName.data() + ichExt, ".xy", 3)) return true;
    if (pszStem[0]) { pszStem[0] = '!'; return true; }
    if (ichExt >= 256) { pszStem[0] = '!'; return true; }
    std::copy_n(pszName.data(), ichExt, pszStem.data());
    pszStem[ichExt] = 0;
    return true;
}

struct LIBRARYTURNMATCH {
    std::int32_t lidGame;
    int cMatches;
    char rgszName[64][256];
    char szSelected[256];
};

static bool FindLibraryTurnMatch(APPSESSION &app, LIBRARYTURNMATCH *plm, std::string_view pszName) {
    char szUniverse[512], szMetadata[512];
    NATIVEFILE *pf;
    std::int32_t lidGame;
    std::uint16_t turn;
    std::int16_t fMulti;
    if (!FPortableLibraryName(app, pszName)) return true;
    CchSnprintf(std::span{szMetadata, sizeof(szMetadata)}, "%s%s%s%s.library", szLibraryRoot, szDirSep, pszName, szDirSep);
    pf = PfOpenNativeFile(szMetadata, NATIVEFILEMODE::read);
    if (!pf) return true;
    ICloseNativeFile(pf);
    CchSnprintf(std::span{szUniverse, sizeof(szUniverse)}, "%s%s%s%s%s.xy", szLibraryRoot, szDirSep, pszName, szDirSep, pszName);
    if (!FInspectImportFile(szUniverse, dtXY, iplrNone, lidGame, &turn, nullptr, fMulti) || lidGame != plm->lidGame) return true;
    if (plm->cMatches < static_cast<int>((sizeof(plm->rgszName) / sizeof(plm->rgszName[0]))))
        CchSnprintf(std::span{plm->rgszName[plm->cMatches], sizeof(plm->rgszName[0])}, "%s", pszName);
    plm->cMatches++;
    return true;
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

static std::int16_t FFilesEqual(std::string_view pszLeft, std::string_view pszRight) {
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

std::int16_t FReplaceConflictingLibraryTurn(APPSESSION &app, const LIBRARYPATHS &paths, HWND hwnd,
                                            std::string_view pszName, std::string_view pszSource,
                                            std::string_view pszTarget, int iplr, std::int32_t lidGame,
                                            std::uint16_t turn);
std::int16_t FImportConflictingTurnAsCopy(APPSESSION &app, const LIBRARYPATHS &paths, HWND hwnd,
                                          std::string_view pszName, std::string_view pszSource, int iplr,
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
    if (!FInspectImportFile(pszSource, dtTurn, iplr, lidGame, &turn, nullptr, fMulti)) {
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
        if (!FInspectImportFile(szTarget, dtTurn, iplr, lidCurrent, &turnCurrent, nullptr, fMultiCurrent) || lidCurrent != lidGame) {
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
        if (iChoice == 1) return FReplaceConflictingLibraryTurn(app, {szLibraryRoot,app.lpui->szLibraryPending,app.lpui->szLibraryName}, hwnd, lm.szSelected, pszSource,
                                                               szTarget, iplr, lidGame, turn);
        if (iChoice == 2) return FImportConflictingTurnAsCopy(app, {szLibraryRoot,app.lpui->szLibraryPending,app.lpui->szLibraryName}, hwnd, lm.szSelected, pszSource,
                                                             iplr, lidGame, turn);
        return 0;
    }
    return FPublishSubsequentTurn(app, {szLibraryRoot,app.lpui->szLibraryPending,app.lpui->szLibraryName}, hwnd,
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

static std::int16_t FChooseLibraryImportFile(APPSESSION &app, HWND hwnd, std::span<char> pszChosen) {
    win32::files::FILESELECTIONREQUEST request;
    const auto szFilter = TextIO().pfnCompressed(app, idsStarsGameFilesMHstRStars);
    const auto pchEnd = std::find(szFilter.begin(), szFilter.end(), '\0');
    std::string_view text{szFilter.data(), static_cast<std::size_t>(pchEnd - szFilter.begin())};
    std::ranges::fill(pszChosen, 0);
    AddFileSelectionFilters(text, request);
    if (!request.rgFilters.empty() && request.rgFilters[0].szPattern.find("*.M*;") != std::string::npos)
        request.rgFilters[0].szPattern.insert(0, "*.ZIP;*.XY;");
    request.mode = win32::files::FILESELECTIONMODE::open;
    request.szInitialPath = pszChosen.data();
    request.fMustExist = true;
    return FSelectLibraryFile(hwnd, std::move(request), pszChosen);
}

/* Choose the workflow from the BOF type, leaving complete validation and
 * source-preserving publication to the corresponding importer. */
std::int16_t FImportDesktopFile(APPSESSION &app, HWND hwnd, std::optional<std::string_view> pszSource) {
    char szChosen[512];
    NATIVEFILE *pf;
    HDR hdr;
    RTBOF rtbof;
    std::int16_t fRace;

    if (!pszSource) {
        if (!FChooseLibraryImportFile(app, hwnd, std::span{szChosen})) return 0;
        pszSource = szChosen;
    }
    fRace = 0;
    pf = PfOpenNativeFile(*pszSource, NATIVEFILEMODE::read);
    if (pf) {
        fRace = FReadLibraryHdr(pf, &hdr) && hdr.rt == rtBOF && hdr.cb == cbWireBof &&
                FReadLibraryBof(pf, &rtbof) && !std::memcmp(rtbof.rgid, "J3J3", 4) && rtbof.dt == dtRace;
        ICloseNativeFile(pf);
    }
    return fRace ? FImportLibraryRaceAndEdit(app, hwnd, pszSource) : FImportLibraryGame(app, hwnd, pszSource);
}

std::int16_t FImportLibraryGame(APPSESSION &app, HWND hwnd, std::optional<std::string_view> pszSource) {
    char szChosen[512];
    char szSource[512], szDir[512], szStem[256], *pch, *pchSlash, *pchName;
    char szZipStage[512];
    LIBRARYIMPORT li;
    int cchDir;
    std::size_t ichExt;
    std::optional<std::int64_t> cbSource;
    std::int16_t fImported, iZipResult;
    std::string_view pszSourceView;

    if (!pszSource) {
        if (!FChooseLibraryImportFile(app, hwnd, std::span{szChosen})) return 0;
        pszSource.emplace(szChosen);
    }
    pszSourceView = *pszSource;
    if (pszSourceView.size() >= sizeof(szSource) || pszSourceView.find('\0') != std::string_view::npos) return 0;
    std::ranges::copy(pszSourceView, std::span{szSource}.begin());
    szSource[pszSourceView.size()] = 0;
    /* Validate the selected path before either standalone-turn or family import. */
    if (!FNativeDirectoryExists(szSource) &&
        (!(cbSource = FNativeFileSize(szSource)) || (*cbSource) <= 0)) goto LInvalidSource;
    if (FPathIsZip(szSource)) {
        if (!FInitLibrary()) return 0;
        if (!FCreateLibraryZipStage(std::span{szZipStage}, "zip-importing")) return 0;
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
    ichExt = pszSourceView.find_last_of('.');
    if (ichExt != std::string_view::npos && ichExt + 1 < pszSourceView.size() &&
        SDL_tolower(pszSourceView[ichExt + 1]) == 'm') {
        if (!FInitLibrary()) return 0;
        fImported = FImportSubsequentTurn(app, hwnd, szSource);
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
            std::copy_n(szSource, cchDir, szDir);
            szDir[cchDir] = 0;
            pchName = pchSlash + 1;
        } else {
            CchCopyUtf8(std::span{szDir}, ".");
            pchName = szSource;
        }
        pch = std::strrchr(pchName, '.');
        if (!pch || pch == pchName || static_cast<std::size_t>((pch - pchName)) >= sizeof(szStem)) goto LInvalidSource;
        std::copy_n(pchName, pch - pchName, szStem);
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
    if (!FChooseLibraryGameName(app, hwnd, std::span<char, 256>{szSource, 256}, std::nullopt)) return 0;
    CchCopyUtf8(std::span{app.lpui->szLibraryExchangeName}, szStem);
    return FPublishImportedLibraryGame(app, {szLibraryRoot,app.lpui->szLibraryPending,app.lpui->szLibraryName},li,szDir);
LInvalidSource:
    CchSnprintf(std::span{app.state.szWork, sizeof(app.state.szWork)}, "%s", szSource);
    AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsCantOpenFile, {}).data(), win32::MB_ICONHAND);
    return 0;
}

struct LIBRARYGAMEREMOVE {
    std::string_view pszDirectory;
    std::int16_t fFailed;
};

std::int16_t RemoveLibraryGameTree(APPSESSION &app, std::string_view pszDirectory);

static bool RemoveLibraryGameEntry(APPSESSION &app, LIBRARYGAMEREMOVE *plgr, std::string_view pszName) {
    std::optional<NATIVEPATHINFO> npiPath;
    char szPath[512];

    int cch = CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", plgr->pszDirectory, szDirSep, pszName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szPath) || !(npiPath = FNativePathInfo(szPath))) {
        plgr->fFailed = 1;
        return false;
    }
    /* A link belongs to this tree; its target does not. */
    if (npiPath->fDirectory && !npiPath->fLink) {
        if (!RemoveLibraryGameTree(app, szPath)) {
            plgr->fFailed = 1;
            return false;
        }
        return true;
    }
    if ((npiPath->fDirectory || npiPath->fLink ? RemoveNativePath(szPath) : RemoveNativeFile(szPath)) != 0) {
        plgr->fFailed = 1;
        return false;
    }
    return true;
}

std::int16_t RemoveLibraryGameTree(APPSESSION &app, std::string_view pszDirectory) {
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

std::int16_t FCreateLibraryGameStage(APPSESSION &app, std::string_view pszSource, std::string_view pszSourceName,
                                     std::string_view pszTargetName, std::string_view pszStage,
                                     int iplr, long long lPlayed, std::string_view pszExchangeName) {
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

std::int16_t FCreateLibraryGameStagePath(std::span<char> pszStage) {
    std::uint32_t dwTick = DwTickCount();
    int i, cch;
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(pszStage, "%s%s.stars-game-stage-%u-%u",
                          szLibraryRoot, szDirSep, dwTick, i);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= pszStage.size()) return 0;
        if (CreateNativeDir(pszStage.data()) == 0) return 1;
        if (!FNativeDirectoryExists(pszStage.data())) return 0;
    }
    return 0;
}

std::int16_t FCreateLibraryGameHoldPath(std::span<char> pszHold) {

    std::uint32_t dwTick = DwTickCount();
    int i, cch;
    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(pszHold, "%s%s.stars-game-old-%u-%u",
                          szLibraryRoot, szDirSep, dwTick, i);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= pszHold.size()) return 0;
        if (!FNativeDirectoryExists(pszHold.data()) && !FNativeFileSize(pszHold.data())) return 1;
    }
    return 0;
}

std::int16_t FValidManagedLibraryGame(APPSESSION &app, std::string_view pszName, std::span<char> pszDirectory,
                                        int &iplr, long long &lPlayed, std::span<char> pszExchangeName) {
    std::optional<NATIVEPATHINFO> npiPath;
    char szMetadataName[256];
    char szName[256];
    int cchDirectory;

    if (!FInitLibrary() || !FPortableLibraryName(app, pszName)) return 0;
    std::copy(pszName.begin(), pszName.end(), szName);
    szName[pszName.size()] = 0;
    if ((cchDirectory = CchSnprintf(pszDirectory, "%s%s%s", szLibraryRoot, szDirSep, szName)) <= 0 ||
        static_cast<std::size_t>(cchDirectory) >= pszDirectory.size() ||
        !(npiPath = FNativePathInfo(pszDirectory.data())) || !npiPath->fDirectory || npiPath->fLink ||
        !FReadLibraryMetadata(std::string_view{pszDirectory.data(), static_cast<std::size_t>(cchDirectory)},
                              std::span{szMetadataName}, iplr,
                              lPlayed, pszExchangeName)) return 0;
    return !SDL_strcasecmp(szMetadataName, szName);
}

std::int16_t FDuplicateLibraryGame(APPSESSION &app, HWND hwnd, std::string_view pszName, std::string_view pszCopyName) {
    char szSource[512], szStage[512], szTarget[512], szExchangeName[256], szMessage[256];
    char szSourceName[256], szCopyName[256];
    int iplr;
    int cchTarget;
    long long lPlayed;
    std::int16_t fResult = 0;
    static_cast<void>(hwnd);
    if (!FPortableLibraryName(app, pszName) || !FPortableLibraryName(app, pszCopyName)) return 0;
    std::copy(pszName.begin(), pszName.end(), szSourceName);
    szSourceName[pszName.size()] = 0;
    std::copy(pszCopyName.begin(), pszCopyName.end(), szCopyName);
    szCopyName[pszCopyName.size()] = 0;
    if (!FValidManagedLibraryGame(app, pszName, std::span{szSource}, iplr, lPlayed,
                                                  std::span{szExchangeName}) ||
        !FLibraryNameAvailableExcept(app, szCopyName, std::nullopt) ||
        (cchTarget = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s", szLibraryRoot, szDirSep, szCopyName)) <= 0 ||
        static_cast<std::size_t>(cchTarget) >= sizeof(szTarget) || cchLibraryRoot + 2 * pszCopyName.size() + 18 >= sizeof(app.state.szBase) ||
        !FCreateLibraryGameStagePath(std::span{szStage})) return 0;
    if (FCreateLibraryGameStage(app, szSource, szSourceName, szCopyName, szStage, -1, 0, szExchangeName) &&
        RenameNativeFile(szStage, szTarget) == 0) fResult = 1;
    if (!fResult) {
        RemoveLibraryGameTree(app, szStage);
        CchCopyUtf8Prefix(std::span{szMessage}, TextIO().pfnCompressed(app, idsLibraryStorageFailed).data());
        CompatChoiceIO().pfnMessageBoxA(win32::IsWindow(hwnd) ? hwnd : app.lpui->hwndFrame, szMessage, TextIO().pfnCompressed(app, idsLibraryGames).data(), win32::MB_OK | win32::MB_ICONHAND);
    }
    return fResult;
}

std::int16_t FRenameLibraryGame(APPSESSION &app, HWND hwnd, std::string_view pszName, std::string_view pszNewName) {
    char szSource[512], szStage[512], szTarget[512], szHold[512], szExchangeName[256], szMessage[768];
    char szSourceName[256], szNewName[256];
    int iplr;
    long long lPlayed;
    std::int16_t fResult = 0, fHeld = 0, fRestoreFailed = 0, fHoldCleanupFailed = 0;
    int cchTarget;
    static_cast<void>(hwnd);
    if (!FPortableLibraryName(app, pszName) || !FPortableLibraryName(app, pszNewName)) return 0;
    std::copy(pszName.begin(), pszName.end(), szSourceName);
    szSourceName[pszName.size()] = 0;
    std::copy(pszNewName.begin(), pszNewName.end(), szNewName);
    szNewName[pszNewName.size()] = 0;
    if (!FValidManagedLibraryGame(app, pszName, std::span{szSource}, iplr, lPlayed,
                                                 std::span{szExchangeName}) ||
        !FLibraryNameAvailableExcept(app, szNewName, std::string_view{szSourceName}) ||
        (cchTarget = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s", szLibraryRoot, szDirSep, szNewName)) <= 0 ||
        static_cast<std::size_t>(cchTarget) >= sizeof(szTarget) || cchLibraryRoot + 2 * pszNewName.size() + 18 >= sizeof(app.state.szBase) ||
        !FCreateLibraryGameStagePath(std::span{szStage})) return 0;
    if (!FCreateLibraryGameStage(app, szSource, szSourceName, szNewName, szStage, iplr, lPlayed, szExchangeName) ||
        !FCreateLibraryGameHoldPath(std::span{szHold}) || RenameNativeFile(szSource, szHold) != 0) goto LDone;
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

std::int16_t FDeleteLibraryGameRecentPaths(APPSESSION &app, std::string_view szDirectory) {
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
        if (LibraryPathIO().pfnUnder(szDirectory, szPath)) {
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
    cch = TextIO().pfnCchGetString(app, idsFile1, std::span{szEntry});
    for (i = 0; i < 9; i++) {
        szEntry[cch - 1] = static_cast<char>(('1' + i));
        CchCopyUtf8(std::span{app.lpui->vrgszMRU, std::size_t{2304}}.subspan(256 * i, 256), rgpszRecent[i]);
        if (!ProfileTextIO().pfnWriteString(szSection, szEntry, rgpszRecent[i], szIniFile)) return 0;
    }
    if (fContinueRemoved && !ProfileTextIO().pfnWriteString(szSection, "File1", "", szIniFile)) return 0;
    return 1;
}

std::int16_t FDeleteLibraryGame(APPSESSION &app, HWND hwnd, std::string_view pszName) {
    char szDirectory[512], szMetadata[512], szMessage[512], szMetadataName[256];
    char szExchangeName[256], szName[256];
    int iplr;
    long long lPlayed;
    int cch;
    std::int16_t fContinueTarget;

    if (pszName.size() >= sizeof(szName) || pszName.find('\0') != std::string_view::npos ||
        !FInitLibrary() || !FPortableLibraryName(app, pszName)) return 0;
    std::copy(pszName.begin(), pszName.end(), szName);
    szName[pszName.size()] = 0;
    cch = CchSnprintf(std::span{szDirectory, sizeof(szDirectory)}, "%s%s%s", szLibraryRoot, szDirSep, szName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szDirectory)) return 0;
    cch = CchSnprintf(std::span{szMetadata, sizeof(szMetadata)}, "%s%s.library", szDirectory, szDirSep);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szMetadata) ||
        !FReadLibraryMetadata(szDirectory, std::span{szMetadataName}, iplr, lPlayed,
                              std::span{szExchangeName}) || std::strcmp(szMetadataName, szName)) return 0;
    cch = CchSnprintf(std::span{szMessage, sizeof(szMessage)}, TextIO().pfnCompressed(app, idsLibraryDeleteGamePrompt).data(), szName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szMessage)) return 0;
    if (CompatChoiceIO().pfnMessageBoxA(hwnd, szMessage, TextIO().pfnCompressed(app, idsLibraryGames).data(),
                   win32::MB_YESNO | win32::MB_ICONQUESTION | win32::MB_TASKMODAL) != win32::IDYES) return 0;
    fContinueTarget = FFindContinueLibraryGame(app) && !std::strcmp(continueLibrary.szName, szName);
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
    if (!FSelectedLibraryGameName(hwnd, szName)) return 0;
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
    XorFileBuf(app, std::as_writable_bytes(std::span{rgbGame}));
    SetFileSeeds(app, lSeed1, lSeed2);
    FDecodeGame(std::as_bytes(std::span(rgbGame)), gameImport);
    return gameImport.lid == rtbof.lidGame && gameImport.cPlayer > 0 && gameImport.cPlayer <= 16 &&
           iplr >= 0 && iplr < gameImport.cPlayer;
}

std::int16_t FImportLibraryOrders(APPSESSION &app, HWND hwnd, std::optional<std::string_view> pszSource) {
    win32::files::FILESELECTIONREQUEST request;
    char szChosen[512], szName[256];
    char szHost[512], szUniverse[512], szTarget[512], szStage[512], szMessage[512];
    std::string_view szFile, szExt;
    char *pch;
    std::int32_t lidHost, lidUniverse, lidOrders;
    std::uint16_t turnHost, turnOrders, wGenHost, wGenOrders;
    std::int16_t fMulti;
    int iplr, cch;
    std::size_t ichSlash, ichExt;

    std::uint32_t dwTick;
    HWND hwndList;
    LRESULT i;

    if (!pszSource) {
        std::ranges::fill(szChosen, 0);
        request.mode = win32::files::FILESELECTIONMODE::open;
        request.fMustExist = true;
        AddLibraryFileSelectionFilters(app, idsLibraryOrdersImportFilter, request);
        if (!FSelectLibraryFile(hwnd, std::move(request), std::span{szChosen})) return 0;
        pszSource = std::string_view{szChosen};
    }
    if (!FInitLibrary() || !win32::IsWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_HOST_GAMES))) return 0;
    hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_HOST_GAMES);
    i = ControlIO().pfnSendMessage(hwndList, win32::LB_GETCURSEL, 0, 0);
    if (i == win32::LB_ERR) {
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryOrdersChooseHost).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    ControlIO().pfnSendMessage(hwndList, win32::LB_GETTEXT, i, reinterpret_cast<LPARAM>(szName));
    ichSlash = pszSource->find_last_of("/\\");
    szFile = pszSource->substr(ichSlash == std::string_view::npos ? 0 : ichSlash + 1);
    ichExt = szFile.find_last_of('.');
    szExt = ichExt == std::string_view::npos ? std::string_view{} : szFile.substr(ichExt);
    if ((szExt.size() != 3 && szExt.size() != 4) || SDL_tolower(static_cast<unsigned char>(szExt[1])) != 'x' ||
        szExt[2] < '1' || szExt[2] > '9' ||
        (szExt.size() == 4 && (szExt[3] < '0' || szExt[3] > '9'))) {
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryOrdersChooseFile).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    iplr = szExt[2] - '1';
    if (szExt.size() == 4) iplr = (szExt[2] - '0') * 10 + szExt[3] - '1';
    if (iplr < 0 || iplr >= 16 || !FInspectImportFile(*pszSource, dtLog, iplr, lidOrders,
            &turnOrders, &wGenOrders, fMulti)) {
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryOrdersInvalid).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    cch = CchSnprintf(std::span{szHost, sizeof(szHost)}, "%s%s%s%s%s.hst", szLibraryRoot, szDirSep, szName, szDirSep, szName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szHost)) return 0;
    cch = CchSnprintf(std::span{szUniverse, sizeof(szUniverse)}, "%s%s%s%s%s.xy", szLibraryRoot, szDirSep, szName, szDirSep, szName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szUniverse)) return 0;
    if (!FInspectImportFile(szHost, dtHost, iplrNone, lidHost, &turnHost, &wGenHost, fMulti) ||
        !FInspectImportFile(szUniverse, dtXY, iplrNone, lidUniverse, nullptr, nullptr, fMulti) ||
        lidHost != lidUniverse || lidOrders != lidHost || turnOrders != turnHost || wGenOrders != wGenHost ||
        !FInspectImportPlayer(app, szUniverse, iplr)) {
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryOrdersMismatch).data(), win32::MB_ICONEXCLAMATION);
        return 0;
    }
    cch = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s%s%s.x%d", szLibraryRoot, szDirSep,
                      szName, szDirSep, szName, iplr + 1);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget)) return 0;
    if (FNativeFileSize(szTarget)) {
        if (FFilesEqual(*pszSource, szTarget)) return 1;
        cch = CchSnprintf(std::span{szMessage, sizeof(szMessage)},
                          TextIO().pfnCompressed(app, idsLibraryOrdersReplacePrompt).data(), iplr + 1, szName);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szMessage)) return 0;
        if (AlertIO().pfnAlertSz(app, szMessage, win32::MB_YESNO | win32::MB_ICONQUESTION) != win32::IDYES) return 0;
    }
    dwTick = DwTickCount();
    cch = CchSnprintf(std::span{szStage, sizeof(szStage)}, "%s.importing.%u", szTarget, dwTick);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szStage)) return 0;
    if (!FCopyNativeFileExclusive(*pszSource, szStage) ||
        !FInspectImportFile(szStage, dtLog, iplr, lidOrders, &turnOrders, &wGenOrders, fMulti) ||
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


static std::int16_t FCopyOptionalConflictFile(std::string_view pszSource, std::string_view pszTarget) {

    if (FNativeFileSize(pszSource)) return FCopyNativeFileExclusive(pszSource, pszTarget);
    return errno == ENOENT;
}

std::int16_t FBackupConflictingPlayerFiles(std::string_view pszDirectory, std::string_view pszName,
                                           int iplr, std::span<char> pszBackup) {
    char szSource[512], szTarget[512], szExt[8];
    int i, cch;


    for (i = 0; i < 1000; i++) {
        cch = CchSnprintf(pszBackup, "%s%sbackup.%03d", pszDirectory, szDirSep, i);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= pszBackup.size()) return 0;
        if (FNativePathInfo(pszBackup.data())) continue;
        if (CreateNativeDir(pszBackup.data()) == 0) break;
        if (!FNativeDirectoryExists(pszBackup.data())) return 0;
    }
    if (i == 1000) return 0;

    CchSnprintf(std::span{szExt, sizeof(szExt)}, "m%d", iplr + 1);
    cch = CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s.%s", pszDirectory, szDirSep, pszName, szExt);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szSource)) return 0;
    cch = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s.%s", pszBackup.data(), szDirSep, pszName, szExt);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget) || !FCopyOptionalConflictFile(szSource, szTarget)) return 0;

    CchSnprintf(std::span{szExt, sizeof(szExt)}, "x%d", iplr + 1);
    cch = CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s.%s", pszDirectory, szDirSep, pszName, szExt);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szSource)) return 0;
    cch = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s.%s", pszBackup.data(), szDirSep, pszName, szExt);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget) || !FCopyOptionalConflictFile(szSource, szTarget)) return 0;

    CchSnprintf(std::span{szExt, sizeof(szExt)}, "h%d", iplr + 1);
    cch = CchSnprintf(std::span{szSource, sizeof(szSource)}, "%s%s%s.%s", pszDirectory, szDirSep, pszName, szExt);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szSource)) return 0;
    cch = CchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s.%s", pszBackup.data(), szDirSep, pszName, szExt);
    return cch > 0 && static_cast<std::size_t>(cch) < sizeof(szTarget) && FCopyOptionalConflictFile(szSource, szTarget);
}

std::int16_t FStageConflictingTurn(std::string_view pszStage, std::string_view pszName,
                                  std::string_view pszSource, int iplr,
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
        !FInspectImportFile(szIncoming, dtTurn, iplr, lidStaged, &turnStaged, &wGenStaged, fMultiStaged) ||
        lidStaged != lidGame || turnStaged != turn || ReplaceNativeFile(szIncoming, szTarget) != 0) {
        RemoveNativeFile(szIncoming);
        return 0;
    }
    return 1;
}

std::int16_t FChooseSeparateCopyName(APPSESSION &app, HWND hwnd, std::string_view pszName) {
    if (!FInitLibrary()) return 0;
    CchSnprintf(std::span{app.lpui->szLibraryName}, "%s", pszName);
    SuggestLibraryName(app, std::span{app.lpui->szLibraryName}, 0);
    if (IdPromptLibraryName(app, hwnd, std::span{app.lpui->szLibraryName}) != 1) return 0;
    return FPortableLibraryName(app, app.lpui->szLibraryName.data()) && FLibraryNameAvailable(app, szLibraryRoot, app.lpui->szLibraryName.data());
}

std::int16_t FReplaceConflictingLibraryTurn(APPSESSION &app, const LIBRARYPATHS &paths, HWND hwnd,
                                            std::string_view pszName, std::string_view pszSource,
                                            std::string_view pszTarget, int iplr, std::int32_t lidGame,
                                            std::uint16_t turn) {
    char szDirectory[512], szStage[512], szHold[512], szBackup[512], szExchangeName[256];
    char szMessage[768];
    int iRemembered;
    long long lPlayed;
    std::int16_t fHeld = 0, fRestoreFailed = 0, fCleanupFailed = 0;
    int cch;

    cch = LibraryPublicationIO().pfnCchSnprintf(std::span{szDirectory, sizeof(szDirectory)}, "%s%s%s", paths.szLibraryRoot, szDirSep, pszName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szDirectory) ||
        !LibraryPublicationIO().pfnFValidManagedLibraryGame(app, pszName, std::span{szDirectory}, iRemembered, lPlayed,
                                  std::span{szExchangeName}) ||
        !LibraryPublicationIO().pfnFCreateLibraryGameStagePath(std::span{szStage})) return 0;
    if (!LibraryPublicationIO().pfnFCreateLibraryGameStage(app, szDirectory, pszName, pszName, szStage, iRemembered,
                                 lPlayed, szExchangeName) ||
        !LibraryPublicationIO().pfnFBackupConflictingPlayerFiles(szStage, pszName, iplr, std::span{szBackup}) ||
        !LibraryPublicationIO().pfnFStageConflictingTurn(szStage, pszName, pszSource, iplr, lidGame, turn) ||
        !LibraryPublicationIO().pfnFCreateLibraryGameHoldPath(std::span{szHold})) goto LDone;
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

std::int16_t FImportConflictingTurnAsCopy(APPSESSION &app, const LIBRARYPATHS &paths, HWND hwnd,
                                          std::string_view pszName, std::string_view pszSource, int iplr,
                                          std::int32_t lidGame, std::uint16_t turn) {
    char szDirectory[512], szStage[512], szTarget[512], szExchangeName[256];
    int iRemembered, cch;
    long long lPlayed;

    cch = LibraryPublicationIO().pfnCchSnprintf(std::span{szDirectory, sizeof(szDirectory)}, "%s%s%s", paths.szLibraryRoot, szDirSep, pszName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szDirectory) ||
        !LibraryPublicationIO().pfnFValidManagedLibraryGame(app, pszName, std::span{szDirectory}, iRemembered, lPlayed,
                                  std::span{szExchangeName}) ||
        !LibraryPublicationIO().pfnFChooseSeparateCopyName(app, hwnd, pszName)) return 0;
    cch = LibraryPublicationIO().pfnCchSnprintf(std::span{szTarget, sizeof(szTarget)}, "%s%s%s", paths.szLibraryRoot, szDirSep, paths.szLibraryName.data());
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szTarget) ||
        !LibraryPublicationIO().pfnFLibraryNameAvailable(app, paths.szLibraryRoot, paths.szLibraryName.data()) ||
        LibraryPublicationIO().pfnPathLength(std::string_view{paths.szLibraryRoot}) +
            2 * LibraryPublicationIO().pfnPathLength(std::string_view{paths.szLibraryName.data()}) + 18 >= sizeof(app.state.szBase) ||
        !LibraryPublicationIO().pfnFCreateLibraryGameStagePath(std::span{szStage})) return 0;
    if (!LibraryPublicationIO().pfnFCreateLibraryGameStage(app, szDirectory, pszName, paths.szLibraryName.data(), szStage,
                                 iplr, 0, szExchangeName) ||
        !LibraryPublicationIO().pfnFStageConflictingTurn(szStage, paths.szLibraryName.data(), pszSource, iplr, lidGame, turn) ||
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

    if (!app.lpui->szLibraryPending[0]) return 0;
    for (i = -2; fGenerated && i < app.state.game.cPlayer; i++) {
        if (i == -2) CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s.xy", app.state.szBase);
        else if (i == -1) CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s.hst", app.state.szBase);
        else CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s.m%d", app.state.szBase, i + 1);
        if (!FCompleteGeneratedFile(app, szFile, i == -2 ? dtXY : i == -1 ? dtHost : dtTurn, i < 0 ? iplrNone : i)) fGenerated = 0;
    }
    if (fGenerated) fGenerated = FWriteLibraryMetadata(app, app.lpui->szLibraryPending.data(), app.lpui->szLibraryName.data(), app.state.idPlayer);
    if (!fGenerated) {
        FNativeEnumerateDirectory(app, app.lpui->szLibraryPending.data(), RemovePendingFile);
        RemoveNativePath(app.lpui->szLibraryPending.data());
        CchCopyUtf8Prefix(std::span{szMessage}, TextIO().pfnCompressed(app, idsLibraryStorageFailed).data());
        CompatChoiceIO().pfnMessageBoxA(win32::IsWindow(hwnd) ? hwnd : app.lpui->hwndFrame, szMessage, TextIO().pfnCompressed(app, idsLibraryGames).data(), win32::MB_OK | win32::MB_ICONHAND);
    }
    app.lpui->szLibraryPending[0] = 0;
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
        rgNew = LpStartHeapArray<LIBRARYGAMELISTITEM>(std::span<std::byte>{reinterpret_cast<std::byte *>(rgNew), static_cast<std::size_t>(cNew) * sizeof(*rgNew)});
        rgLibraryGameList.release();
        rgLibraryGameList.reset(rgNew);
        cLibraryGameListMax = cNew;
    }
    i = ControlIO().pfnSendMessage(hwndList, win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pszName));
    if (i == win32::LB_ERR || i == win32::LB_ERRSPACE) return 0;
    CchSnprintf(std::span{rgLibraryGameList[cLibraryGameList].szName, sizeof(rgLibraryGameList[cLibraryGameList].szName)}, "%s", pszName);
    rgLibraryGameList[cLibraryGameList].iplr = iplr;
    rgLibraryGameList[cLibraryGameList].grfPlayers = grfPlayers;
    std::copy_n(rgturn, sizeof(rgLibraryGameList[cLibraryGameList].rgturn) / sizeof(*rgturn), rgLibraryGameList[cLibraryGameList].rgturn);
    ControlIO().pfnSendMessage(hwndList, win32::LB_SETITEMDATA, i, cLibraryGameList++);
    return 1;
}

static void ClearLibraryGameList(void) {
    rgLibraryGameList.reset();
    cLibraryGameList = 0;
    cLibraryGameListMax = 0;
}

static bool ListLibraryGame(APPSESSION &app, HWND hwndList, std::string_view pszName) {
    char szFile[256], szDirectory[256], szMetadataName[256], szExchangeName[256];
    int iplr, iplrSaved, cch;
    std::optional<std::int64_t> cbFile;
    long long lPlayed;
    std::int32_t lidGame;
    std::uint16_t turn;
    std::uint16_t grfPlayers = 0, rgturn[16] = {0};
    std::int16_t fMulti;

    if (!FPortableLibraryName(app, pszName)) return true;
    cch = CchSnprintf(std::span{szDirectory, sizeof(szDirectory)}, "%s%s%s", szLibraryRoot, szDirSep, pszName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(szDirectory) ||
        !FReadLibraryMetadata(szDirectory, std::span{szMetadataName}, iplrSaved, lPlayed,
                              std::span{szExchangeName})) return true;
    CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s%s%s%s.xy", szLibraryRoot, szDirSep, pszName, szDirSep, pszName);
    if (!(cbFile = FNativeFileSize(szFile)) || (*cbFile) == 0) return true;
    for (iplr = 0; iplr < 16; iplr++) {
        CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s%s%s%s.m%d", szLibraryRoot, szDirSep, pszName, szDirSep, pszName, iplr + 1);
        if (!(cbFile = FNativeFileSize(szFile)) || (*cbFile) <= 0 ||
            !FInspectImportFile(szFile, dtTurn, iplr, lidGame, &turn, nullptr, fMulti)) continue;
        grfPlayers |= static_cast<std::uint16_t>((1u << iplr));
        rgturn[iplr] = turn;
    }
    if (!grfPlayers) return true;
    const std::string szName{pszName};
    return FAddLibraryGameListItem(hwndList, szName.c_str(), iplrSaved, grfPlayers, rgturn);
}

static bool ListLibraryHostGame(APPSESSION &app, HWND hwndList, std::string_view pszName) {
    char szHost[512], szUniverse[512], szMetadata[512];
    NATIVEFILE *pf;
    std::optional<std::int64_t> cbFile;
    std::int32_t lidHost, lidUniverse;
    std::uint16_t turnHost;
    std::int16_t fMulti;
    LRESULT i;

    if (!FPortableLibraryName(app, pszName)) return true;
    CchSnprintf(std::span{szMetadata, sizeof(szMetadata)}, "%s%s%s%s.library", szLibraryRoot, szDirSep, pszName, szDirSep);
    pf = PfOpenNativeFile(szMetadata, NATIVEFILEMODE::read);
    if (!pf) return true;
    ICloseNativeFile(pf);
    CchSnprintf(std::span{szHost, sizeof(szHost)}, "%s%s%s%s%s.hst", szLibraryRoot, szDirSep, pszName, szDirSep, pszName);
    CchSnprintf(std::span{szUniverse, sizeof(szUniverse)}, "%s%s%s%s%s.xy", szLibraryRoot, szDirSep, pszName, szDirSep, pszName);
    if (!(cbFile = FNativeFileSize(szHost)) || (*cbFile) <= 0 ||
        !FInspectImportFile(szHost, dtHost, iplrNone, lidHost, &turnHost, nullptr, fMulti) ||
        !FInspectImportFile(szUniverse, dtXY, iplrNone, lidUniverse, nullptr, nullptr, fMulti) ||
        lidHost != lidUniverse) return true;
    const std::string szName{pszName};
    i = ControlIO().pfnSendMessage(hwndList, win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(szName.c_str()));
    return i != win32::LB_ERR && i != win32::LB_ERRSPACE;
}

static bool FindContinueLibraryGame(APPSESSION &app, LIBRARYCONTINUE *plc, std::string_view pszName) {
    char szFile[512], szMetadataName[256];
    std::string_view szMetadataNameView;
    NATIVEFILE *pf;
    std::optional<std::size_t> cchMetadataName;
    std::size_t cchMetadataText, ichLineEnd;
    int iVersion, iPlayer;
    long long lPlayed;
    std::int32_t lidGame;
    std::uint16_t turn;
    std::int16_t fMulti;
    int iYear;
    std::int16_t fTurnAvailable;

    if (!FPortableLibraryName(app, pszName)) return true;
    CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s%s%s.library", szLibraryRoot, szDirSep, pszName, szDirSep);
    pf = PfOpenNativeFile(szFile, NATIVEFILEMODE::read);
    if (!pf) return true;
    iVersion = 0;
    iPlayer = -1;
    lPlayed = 0;
    if (CNativeScan(pf, "%d\n", &iVersion) != 1 || !(cchMetadataName = CchGetNativeLine(std::span{szMetadataName}, pf)) ||
        CNativeScan(pf, "%d\n%lld", &iPlayer, &lPlayed) != 2) {
        ICloseNativeFile(pf);
        return true;
    }
    ICloseNativeFile(pf);
    szMetadataNameView = std::string_view{szMetadataName, *cchMetadataName};
    cchMetadataText = szMetadataNameView.find('\0');
    if (cchMetadataText == std::string_view::npos) cchMetadataText = szMetadataNameView.size();
    ichLineEnd = szMetadataNameView.substr(0, cchMetadataText).find_first_of("\r\n");
    if (ichLineEnd != std::string_view::npos) cchMetadataText = ichLineEnd;
    szMetadataName[cchMetadataText] = 0;
    const std::string szName{pszName};
    if ((iVersion != 1 && iVersion != 2) || _stricmp(szMetadataName, szName.c_str()) ||
        iPlayer < 0 || iPlayer >= 16 || (plc->fFound && lPlayed <= plc->lPlayed)) return true;

    fTurnAvailable = 0;
    iYear = 0;
    CchSnprintf(std::span{szFile, sizeof(szFile)}, "%s%s%s%s%s.m%d", szLibraryRoot, szDirSep, pszName, szDirSep,
                pszName, iPlayer + 1);
    if (FInspectImportFile(szFile, dtTurn, iPlayer, lidGame, &turn, nullptr, fMulti)) {
        iYear = turn + 2400;
        fTurnAvailable = 1;
    }
    CchCopyUtf8(std::span{plc->szName}, pszName);
    plc->iPlayer = iPlayer;
    plc->iYear = iYear;
    plc->lPlayed = lPlayed;
    plc->fFound = 1;
    plc->fTurnAvailable = fTurnAvailable;
    return true;
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

bool FEnumerateLibraryRaces(APPSESSION &app, const FNativeDirectoryEntryProc &pfnEntry) {
    return FInitRaceLibrary(app) && FNativeEnumerateDirectory(app, app.lpui->szRaceLibraryRoot.data(), [&pfnEntry](APPSESSION &appEntry, std::string_view pszName) {
        char szPath[512];
        std::optional<std::int64_t> cbFile;
        if (!FPortableRaceFilename(appEntry, pszName)) return true;
        CchSnprintf(std::span{szPath, sizeof(szPath)}, "%s%s%s", appEntry.lpui->szRaceLibraryRoot.data(), szDirSep, pszName);
        if (!(cbFile = FNativeFileSize(szPath)) || (*cbFile) <= 0) return true;
        return pfnEntry(appEntry, pszName);
    });
}

static std::int16_t ListLibraryRace(APPSESSION &, HWND hwndList, std::string_view pszName) {
    LRESULT i;

    const std::string szName{pszName};
    i = ControlIO().pfnSendMessage(hwndList, win32::LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(szName.c_str()));
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
    FEnumerateLibraryRaces(app, [hwndList](APPSESSION &appEntry, std::string_view pszName) {
        return ListLibraryRace(appEntry, hwndList, pszName);
    });
    ControlIO().pfnSendMessage(hwndList, win32::LB_SETCURSEL, 0, 0);
    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RACE_EDIT), ControlIO().pfnSendMessage(hwndList, win32::LB_GETCOUNT, 0, 0) > 0);
    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RACE_DUPLICATE), ControlIO().pfnSendMessage(hwndList, win32::LB_GETCOUNT, 0, 0) > 0);
    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RACE_EXPORT), ControlIO().pfnSendMessage(hwndList, win32::LB_GETCOUNT, 0, 0) > 0);
    ControlIO().pfnEnableWindow(ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RACE_DELETE), ControlIO().pfnSendMessage(hwndList, win32::LB_GETCOUNT, 0, 0) > 0);
}

std::int16_t FSelectLibraryRace(HWND hwnd, std::span<char, 256> pszName) {
    HWND hwndList = ControlIO().pfnGetDlgItem(hwnd, IDC_LIBRARY_RACES);
    LRESULT i = ControlIO().pfnSendMessage(hwndList, win32::LB_GETCURSEL, 0, 0);
    LRESULT cch;
    if (i == win32::LB_ERR) return 0;
    cch = ControlIO().pfnSendMessage(hwndList, win32::LB_GETTEXTLEN, i, 0);
    if (cch == win32::LB_ERR || cch < 0 || cch >= static_cast<LRESULT>(pszName.size())) return 0;
    return ControlIO().pfnSendMessage(hwndList, win32::LB_GETTEXT, i, reinterpret_cast<LPARAM>(pszName.data())) == cch;
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
    if (FInspectImportFile(szFile, dtTurn, continueLibrary.iPlayer, lidGame, &turn, nullptr, fMulti)) {
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
    LRESULT cchName;
    char szName[256];
    int cch;
    if (i == win32::LB_ERR) return 0;
    cchName = ControlIO().pfnSendMessage(hwndList, win32::LB_GETTEXTLEN, i, 0);
    if (cchName == win32::LB_ERR || cchName < 0 || cchName >= static_cast<LRESULT>(sizeof(szName))) return 0;
    if (ControlIO().pfnSendMessage(hwndList, win32::LB_GETTEXT, i, reinterpret_cast<LPARAM>(szName)) != cchName) return 0;
    cch = CchSnprintf(std::span{app.state.szBase, sizeof(app.state.szBase)}, "%s%s%s%s%s.hst", szLibraryRoot, szDirSep,
                      szName, szDirSep, szName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= sizeof(app.state.szBase)) return 0;
    app.state.ini.fStartupFile = 1;
    return 1;
}

static std::int16_t FSelectedLibraryGameName(HWND hwnd, std::span<char> pszName) {
    HWND hwndList = ControlIO().pfnGetDlgItem(hwnd, fHostLibraryView ? IDC_LIBRARY_HOST_GAMES : IDC_LIBRARY_GAMES);
    LRESULT i, iItem;
    LRESULT cch;
    i = ControlIO().pfnSendMessage(hwndList, win32::LB_GETCURSEL, 0, 0);
    if (i == win32::LB_ERR) return 0;
    if (fHostLibraryView) {
        cch = ControlIO().pfnSendMessage(hwndList, win32::LB_GETTEXTLEN, i, 0);
        if (cch <= 0 || static_cast<std::size_t>(cch) >= pszName.size()) return 0;
        return ControlIO().pfnSendMessage(hwndList, win32::LB_GETTEXT, i, reinterpret_cast<LPARAM>(pszName.data())) == cch;
    }
    iItem = ControlIO().pfnSendMessage(hwndList, win32::LB_GETITEMDATA, i, 0);
    if (iItem < 0 || iItem >= cLibraryGameList) return 0;
    cch = CchSnprintf(pszName, "%s", rgLibraryGameList[iItem].szName);
    return cch > 0 && static_cast<std::size_t>(cch) < pszName.size();
}

static std::int16_t FChooseLibraryGameActionName(APPSESSION &app, HWND hwnd, const char *pszCurrentName, std::span<char, 256> pszNewName) {
    int cch;
    cch = CchSnprintf(std::span{app.lpui->szLibraryName}, "%s", pszCurrentName);
    if (cch <= 0 || static_cast<std::size_t>(cch) >= app.lpui->szLibraryName.size()) return 0;
    SuggestLibraryName(app, std::span{app.lpui->szLibraryName}, 0);
    if (!app.lpui->szLibraryName[0] || IdPromptLibraryName(app, hwnd, std::span{app.lpui->szLibraryName}) != 1) return 0;
    cch = CchSnprintf(pszNewName, "%s", app.lpui->szLibraryName.data());
    return cch > 0 && cch < 256;
}

std::int16_t FActOnSelectedLibraryGame(APPSESSION &app, HWND hwnd, std::int16_t fRename) {
    char szName[256], szNewName[256];
    if (!FSelectedLibraryGameName(hwnd, szName) ||
        !FChooseLibraryGameActionName(app, hwnd, szName, szNewName)) return 0;
    return fRename ? FRenameLibraryGame(app, hwnd, szName, szNewName) :
                     FDuplicateLibraryGame(app, hwnd, szName, szNewName);
}

void RememberLibraryGame(APPSESSION &app) {
    char szDir[256], szMetadataName[256];
    std::optional<std::size_t> ichSlash;
    int iplr;
    long long lPlayed;

    if (!FInitLibrary() || !LibraryPathIO().pfnUnder(szLibraryRoot, app.state.szBase)) return;
    CchCopyUtf8(std::span{szDir}, app.state.szBase);
    ichSlash = IchLibrarySeparator(szDir);
    if (!ichSlash) return;
    szDir[*ichSlash] = 0;
    ichSlash = IchLibrarySeparator(szDir);
    if (!ichSlash) return;
    app.lpui->szLibraryExchangeName[0] = 0;
    /* szDir already names the game folder; retain its exchange basename before
     * publishing the newly opened player and last-played time. */
    if (!FReadLibraryMetadata(szDir, std::span{szMetadataName}, iplr, lPlayed,
                              std::span{app.lpui->szLibraryExchangeName})) app.lpui->szLibraryExchangeName[0] = 0;
    if (FWriteLibraryMetadata(app, szDir, std::string_view{szDir}.substr(*ichSlash + 1), app.state.idPlayer))
        ProfileTextIO().pfnWriteString("Library", "ContinueDeleted", nullptr, TextIO().pfnCompressed(app, idsStarsIni).data());
}

void CloseLibraryUiSession(APPSESSION &app) noexcept {
    ClearLibraryGameList();
    szLibraryRoot[0] = app.lpui->szLibraryPending[0] = app.lpui->szLibraryName[0] = app.lpui->szLibraryExchangeName[0] = 0;
    cchLibraryRoot = 0;
    app.lpui->szRaceLibraryRoot[0] = app.lpui->szRaceLibraryPath[0] = app.lpui->szRaceLibraryName[0] = 0;
    fRaceLibraryNameDialog = fNewLibraryRace = fHostLibraryView = 0;
    continueLibrary = LIBRARYCONTINUE{};
}

std::int16_t FPublishSubsequentTurn(APPSESSION &app, const LIBRARYPATHS &paths, HWND hwnd,
                                  std::string_view pszSource, std::string_view szTarget,
                                  std::string_view pszName, int iplr,
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
        !LibraryPublicationIO().pfnFInspectImportFile(szStage, dtTurn, iplr, lidCurrent, &turnCurrent, nullptr, fMultiCurrent) ||
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

std::int16_t FPublishImportedLibraryGame(APPSESSION &app, const LIBRARYPATHS &paths, LIBRARYIMPORT &li,
                                         std::string_view szDir) {
    char szStage[256];
    LibraryPublicationIO().pfnCchSnprintf(std::span{szStage, sizeof(szStage)}, "%s%s.importing.%lld", paths.szLibraryRoot, szDirSep, static_cast<long long>(LibraryPublicationIO().pfnLNativeTime()));
    if (LibraryPublicationIO().pfnCreateNativeDir(szStage) != 0) {
        RemoveNativePath(paths.szLibraryPending.data());
        paths.szLibraryPending[0] = 0;
        return 0;
    }
    li.pszTarget = szStage;
    li.pszName = paths.szLibraryName.data();
    li.fCopy = 1;
    if (!LibraryPublicationIO().pfnEnumerateDirectory(app, LIBRARYPUBLICATIONENUMERATION::importFiles, szDir,
            [&li](APPSESSION &appEntry, std::string_view pszName) { return CopyImportEntry(appEntry, &li, pszName); }) || li.fBad ||
        !LibraryPublicationIO().pfnEnumerateDirectory(app, LIBRARYPUBLICATIONENUMERATION::backupFiles, szDir,
            [&li](APPSESSION &appEntry, std::string_view pszName) { return CopyBackupDirectory(appEntry, &li, pszName); }) || li.fBad ||
        !LibraryPublicationIO().pfnFWriteLibraryMetadata(app, szStage, paths.szLibraryName.data(),
                               li.grfPlayers ? static_cast<std::int16_t>(__builtin_ctz(li.grfPlayers)) : iplrNone) ||
        RemoveNativePath(paths.szLibraryPending.data()) != 0 || RenameNativeFile(szStage, paths.szLibraryPending.data()) != 0) {
        LibraryPublicationIO().pfnEnumerateDirectory(app, LIBRARYPUBLICATIONENUMERATION::removeFiles, szStage,
            [szStage](APPSESSION &appEntry, std::string_view pszName) { return RemoveImportFile(appEntry, szStage, pszName); });
        RemoveNativePath(szStage);
        RemoveNativePath(paths.szLibraryPending.data());
        paths.szLibraryPending[0] = 0;
        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsLibraryGameImportFailed).data(), win32::MB_ICONHAND);
        return 0;
    }
    paths.szLibraryPending[0] = 0;
    return 1;
}
