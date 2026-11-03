#ifndef STARS_FILEDIALOG_COCOA_BRIDGE_H
#define STARS_FILEDIALOG_COCOA_BRIDGE_H
#include "file_selection_types.h"
/* Keep the wrapper's Win32 BOOL and handles out of AppKit's namespace. */
win32::files::FILESELECTIONRESULT IChooseCocoaFile(
    void *lpWindow, const win32::files::FILESELECTIONREQUEST &request,
    const win32::files::FILESELECTIONPUMP &pfnPump);

#endif
