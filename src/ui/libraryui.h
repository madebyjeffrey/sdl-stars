#ifndef STARS_LIBRARYUI_H
#define STARS_LIBRARYUI_H

#define IDD_LIBRARY_NAME 2400
#define IDC_LIBRARY_GAMES 2401
#define IDC_LIBRARY_IMPORT 2402
#define IDC_LIBRARY_IMPORT_FOLDER 2403
#define IDC_LIBRARY_RACES 2404
#define IDC_LIBRARY_RACE_NEW 2405
#define IDC_LIBRARY_RACE_EDIT 2406
#define IDC_LIBRARY_RACE_IMPORT 2407
#define IDC_LIBRARY_RACE_DUPLICATE 2408
#define IDC_LIBRARY_RACE_EXPORT 2409
#define IDC_LIBRARY_RACE_DELETE 2410
#define IDD_LIBRARY_TURN_TARGET 2411
#define IDC_LIBRARY_TURN_TARGET_LIST 2412
#define IDM_LIBRARY_EXPORT_ORDERS 2413
#define IDC_LIBRARY_DUPLICATE 2414
#define IDC_LIBRARY_RENAME 2415
#define WM_STARS_IMPORT_FOLDER (WM_APP + 3)

int16_t FChooseLibraryGameName(HWND hwnd, char *pszFile);
int16_t FFinishLibraryGame(HWND hwnd, int16_t fGenerated);
void InitLibraryGames(HWND hwnd, const RECT *lprc);
void RefreshLibraryGames(HWND hwnd);
int16_t FSelectLibraryGame(HWND hwnd);
void RememberLibraryGame(void);
int16_t FImportLibraryGame(HWND hwnd, const char *pszSource);
int16_t FDuplicateLibraryGame(HWND hwnd, const char *pszName, const char *pszCopyName);
int16_t FRenameLibraryGame(HWND hwnd, const char *pszName, const char *pszNewName);
int16_t FActOnSelectedLibraryGame(HWND hwnd, int16_t fRename);
int16_t FImportLibraryRace(HWND hwnd, const char *pszSource);
int16_t FImportLibraryRaceAs(HWND hwnd, const char *pszSource, const char *pszName);
int16_t FImportLibraryRaceAndEdit(HWND hwnd, const char *pszSource);
int16_t FSaveLibraryRace(HWND hwnd, const char *pszName, PLAYER *pplr);
int16_t FSaveCurrentLibraryRace(HWND hwnd, PLAYER *pplr);
int16_t FHasCurrentLibraryRace(void);
int16_t FLoadLibraryRace(HWND hwnd, const char *pszName);
int16_t FBeginLibraryRace(HWND hwnd);
int16_t FEditLibraryRace(HWND hwnd, const char *pszName);
int16_t FDuplicateLibraryRace(HWND hwnd, const char *pszName);
int16_t FExportLibraryRace(HWND hwnd, const char *pszName, const char *pszTarget);
int16_t FExportLibraryOrders(HWND hwnd, const char *pszTarget);
int16_t FDeleteLibraryRace(HWND hwnd, const char *pszName);
void InitLibraryRaces(HWND hwnd, const RECT *lprc);
void RefreshLibraryRaces(HWND hwnd);
int16_t FSelectLibraryRace(HWND hwnd, char *pszName);
void ShowLibraryImportFolderPicker(HWND hwnd);
INT_PTR CALLBACK LibraryNameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK LibraryTurnTargetDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif
