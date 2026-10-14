#ifndef STARS_LIBRARYUI_H
#define STARS_LIBRARYUI_H

#define IDD_LIBRARY_NAME 2400
#define IDC_LIBRARY_GAMES 2401
#define IDC_LIBRARY_IMPORT 2402
#define IDC_LIBRARY_IMPORT_FOLDER 2403
#define WM_STARS_IMPORT_FOLDER (WM_APP + 3)

int16_t FChooseLibraryGameName(HWND hwnd, char *pszFile);
int16_t FFinishLibraryGame(HWND hwnd, int16_t fGenerated);
void InitLibraryGames(HWND hwnd, const RECT *lprc);
void RefreshLibraryGames(HWND hwnd);
int16_t FSelectLibraryGame(HWND hwnd);
void RememberLibraryGame(void);
int16_t FImportLibraryGame(HWND hwnd, const char *pszSource);
void ShowLibraryImportFolderPicker(HWND hwnd);
INT_PTR CALLBACK LibraryNameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif
