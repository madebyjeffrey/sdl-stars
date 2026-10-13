#ifndef STARS_LIBRARYUI_H
#define STARS_LIBRARYUI_H

#define IDD_LIBRARY_NAME 2400
#define IDC_LIBRARY_GAMES 2401

int16_t FChooseLibraryGameName(HWND hwnd, char *pszFile);
int16_t FFinishLibraryGame(HWND hwnd, int16_t fGenerated);
void InitLibraryGames(HWND hwnd, const RECT *lprc);
int16_t FSelectLibraryGame(HWND hwnd);
void RememberLibraryGame(void);
INT_PTR CALLBACK LibraryNameDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif
