#ifndef STARS_FILEDIALOG_BACKEND_H
#define STARS_FILEDIALOG_BACKEND_H
#include "internal.h"
/* Native paths are filesystem bytes (UTF-8 on ordinary Linux/macOS installs).
   Backends convert Stars CP1252 labels to the platform's text encoding. */
typedef struct COMPATFILEFILTER { char *pszName, *pszPattern; } COMPATFILEFILTER;
int ICompatChooseFile(HWND hwndOwner, BOOL fSave, LPCSTR pszTitle, LPCSTR pszFolder,
                     LPCSTR pszName, const COMPATFILEFILTER *rgfilter, int cFilter,
                     int *lpiFilter, char *pszPath, size_t cbPath);
#endif
