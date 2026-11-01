#ifndef STARS_LIBRARYUI_H
#define STARS_LIBRARYUI_H

#include "win.h"
inline constexpr auto IDD_LIBRARY_RACE_SAVE = 2430;
inline constexpr auto IDC_LIBRARY_RACE_UPDATE = 2431;
inline constexpr auto IDC_LIBRARY_RACE_VERSION = 2432;
inline constexpr auto IDC_LIBRARY_RACE_SAVE_TEXT = 2433;
inline constexpr auto IDD_LIBRARY_NAME = 2400;
inline constexpr auto IDD_LIBRARY_GAME_REVISION = 2440;
inline constexpr auto IDC_LIBRARY_GAME_REVISION_TEXT = 2441;
inline constexpr auto IDC_LIBRARY_PLAYERS = 2442;
inline constexpr auto IDC_LIBRARY_PLAYER_LABEL = 2443;
inline constexpr auto IDC_LIBRARY_CONTINUE_DETAILS = 2444;
inline constexpr auto IDC_LIBRARY_GAMES = 2401;
inline constexpr auto IDC_LIBRARY_IMPORT = 2402;
inline constexpr auto IDC_LIBRARY_IMPORT_FOLDER = 2403;
inline constexpr auto IDC_LIBRARY_RACES = 2404;
inline constexpr auto IDC_LIBRARY_RACE_NEW = 2405;
inline constexpr auto IDC_LIBRARY_RACE_EDIT = 2406;
inline constexpr auto IDC_LIBRARY_RACE_IMPORT = 2407;
inline constexpr auto IDC_LIBRARY_RACE_DUPLICATE = 2408;
inline constexpr auto IDC_LIBRARY_RACE_EXPORT = 2409;
inline constexpr auto IDC_LIBRARY_RACE_DELETE = 2410;
inline constexpr auto IDD_LIBRARY_TURN_TARGET = 2411;
inline constexpr auto IDC_LIBRARY_TURN_TARGET_LIST = 2412;
inline constexpr auto IDD_LIBRARY_TURN_CONFLICT = 2420;
inline constexpr auto IDC_LIBRARY_TURN_REPLACE = 2421;
inline constexpr auto IDC_LIBRARY_TURN_SEPARATE = 2422;
inline constexpr auto IDM_LIBRARY_EXPORT_ORDERS = 2413;
inline constexpr auto IDC_LIBRARY_DUPLICATE = 2414;
inline constexpr auto IDC_LIBRARY_RENAME = 2415;
inline constexpr auto IDC_LIBRARY_GAME_EXPORT = 2416;
inline constexpr auto WM_STARS_IMPORT_FOLDER = (win32::WM_APP + 3);
inline constexpr auto WM_STARS_EXPORT_FOLDER = (win32::WM_APP + 4);
inline constexpr auto IDC_LIBRARY_HOST_GAMES = 2417;
inline constexpr auto IDC_LIBRARY_GAMES_VIEW = 2418;
inline constexpr auto IDC_LIBRARY_HOST_VIEW = 2419;
inline constexpr auto IDC_LIBRARY_IMPORT_ORDERS = 2420;
inline constexpr auto IDC_LIBRARY_GAME_DELETE = 2421;
inline constexpr auto IDC_LIBRARY_TUTORIAL = 2434;

/* New games pass their title; imports pass NULL to choose a library name. */
std::int16_t FChooseLibraryGameName(APPSESSION &app, HWND hwnd, char *pszFile, const char *pszName);
INT_PTR  LibraryGameRevisionDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
std::int16_t FFinishLibraryGame(APPSESSION &app, HWND hwnd, std::int16_t fGenerated);
void InitLibraryGames(APPSESSION &app, HWND hwnd, const RECT *lprc);
void RefreshLibraryGames(APPSESSION &app, HWND hwnd);
void RefreshLibraryPlayers(APPSESSION &app, HWND hwnd);
void RefreshLibraryHostGames(APPSESSION &app, HWND hwnd);
void SelectLibraryView(APPSESSION &app, HWND hwnd, std::int16_t fHost);
std::int16_t FSelectLibraryGame(APPSESSION &app, HWND hwnd);
std::int16_t FSelectLibraryHostGame(APPSESSION &app, HWND hwnd);
std::int16_t FContinueLibraryGame(APPSESSION &app, HWND hwnd);
void RememberLibraryGame(APPSESSION &app);
std::int16_t FImportLibraryGame(APPSESSION &app, HWND hwnd, const char *pszSource);
std::int16_t FImportLibraryOrders(APPSESSION &app, HWND hwnd, const char *pszSource);
std::int16_t FImportDesktopFile(APPSESSION &app, HWND hwnd, const char *pszSource);
std::int16_t FIsManagedLibraryGameFile(const char *pszFile);
std::int16_t FIsManagedLibraryRaceFile(const char *pszFile);
std::int16_t FExportLibraryGame(APPSESSION &app, HWND hwnd, const char *pszName, const char *pszTarget);
std::int16_t FExportSelectedLibraryGame(APPSESSION &app, HWND hwnd, const char *pszTarget);
void ShowLibraryExportFolderPicker(HWND hwnd);
std::int16_t FDuplicateLibraryGame(APPSESSION &app, HWND hwnd, const char *pszName, const char *pszCopyName);
std::int16_t FRenameLibraryGame(APPSESSION &app, HWND hwnd, const char *pszName, const char *pszNewName);
std::int16_t FActOnSelectedLibraryGame(APPSESSION &app, HWND hwnd, std::int16_t fRename);
std::int16_t FDeleteLibraryGame(APPSESSION &app, HWND hwnd, const char *pszName);
std::int16_t FDeleteSelectedLibraryGame(APPSESSION &app, HWND hwnd);
std::int16_t FImportLibraryRace(APPSESSION &app, HWND hwnd, const char *pszSource);
std::int16_t FImportLibraryRaceAs(APPSESSION &app, HWND hwnd, const char *pszSource, const char *pszName);
std::int16_t FImportLibraryRaceAndEdit(APPSESSION &app, HWND hwnd, const char *pszSource);
std::int16_t FSaveLibraryRace(APPSESSION &app, HWND hwnd, const char *pszName, PLAYER *pplr);
std::int16_t FSaveCurrentLibraryRace(APPSESSION &app, HWND hwnd, PLAYER *pplr);
std::int16_t FHasCurrentLibraryRace(void);
std::int16_t FLoadLibraryRace(APPSESSION &app, HWND hwnd, const char *pszName);
const char *PszCurrentLibraryRaceName(void);
std::int16_t FCustomizeLibraryRace(APPSESSION &app, HWND hwnd, char *pszName);
std::int16_t FBeginLibraryRace(APPSESSION &app, HWND hwnd);
std::int16_t FEditLibraryRace(APPSESSION &app, HWND hwnd, const char *pszName);
std::int16_t FDuplicateLibraryRace(APPSESSION &app, HWND hwnd, const char *pszName);
std::int16_t FExportLibraryRace(APPSESSION &app, HWND hwnd, const char *pszName, const char *pszTarget);
std::int16_t FExportLibraryOrders(APPSESSION &app, HWND hwnd, const char *pszTarget);
std::int16_t FDeleteLibraryRace(APPSESSION &app, HWND hwnd, const char *pszName);
void InitLibraryRaces(APPSESSION &app, HWND hwnd, const RECT *lprc);
void RefreshLibraryRaces(APPSESSION &app, HWND hwnd);
std::int16_t FSelectLibraryRace(HWND hwnd, char *pszName);
std::int16_t FEnumerateLibraryRaces(APPSESSION &app, FNativeDirectoryEntryProc pfnEntry, void *lpData);
void ShowLibraryImportFolderPicker(HWND hwnd);
INT_PTR  LibraryRaceSaveDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
// WM_INITDIALOG borrows a std::span<char, 256>* until the dialog closes.
INT_PTR  LibraryNameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR  LibraryTurnTargetDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR  LibraryTurnConflictDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif
