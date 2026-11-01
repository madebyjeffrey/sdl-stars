#ifndef STARS_FILEDIALOG_WINDOWS_BRIDGE_H
#define STARS_FILEDIALOG_WINDOWS_BRIDGE_H
#include <stddef.h>

/* The SDK and wrapper have distinct HWND and OPENFILENAME definitions. */
struct WINDOWSFILEFILTER { const char *pszName, *pszPattern; };
int IChooseWindowsFile(void *lpWindow, int fSave, const char *pszTitle, const char *pszFolder,
                       const char *pszName, const WINDOWSFILEFILTER *rgfilter, int cFilter,
                       int *lpiFilter, char *pszPath, size_t cbPath,
                       int (*pfnPump)(void *), void *lpv);
#endif
