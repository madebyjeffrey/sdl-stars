#ifndef STARS_FILEDIALOG_COCOA_BRIDGE_H
#define STARS_FILEDIALOG_COCOA_BRIDGE_H
#include <cstddef>
/* Keep Win32 BOOL out of AppKit's namespace. This bridge uses plain C types. */
struct COCOAFILEFILTER { const char *pszName, *pszPattern; };
int IChooseCocoaFile(void *lpWindow, int fSave, const char *pszTitle, const char *pszFolder,
                     const char *pszName, const COCOAFILEFILTER *rgfilter, int cFilter,
                     int *lpiFilter, char *pszPath, std::size_t cbPath,
                     int (*pfnPump)(void *), void *lpv);

#endif
