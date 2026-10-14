#ifndef STARS_FILEDIALOG_BACKEND_H
#define STARS_FILEDIALOG_BACKEND_H
#include "internal.h"
/* Native paths are filesystem bytes (UTF-8 on ordinary Linux/macOS installs).
   Labels and paths use UTF-8; backends convert losslessly to native APIs. */
typedef struct COMPATFILEFILTER { char *pszName, *pszPattern; } COMPATFILEFILTER;
int ICompatChooseFile(HWND hwndOwner, BOOL fSave, LPCSTR pszTitle, LPCSTR pszFolder,
                     LPCSTR pszName, const COMPATFILEFILTER *rgfilter, int cFilter,
                     int *lpiFilter, char *pszPath, size_t cbPath);
#endif
