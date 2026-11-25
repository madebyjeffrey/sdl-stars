#ifndef STARS_FILEDIALOG_WINDOWS_BRIDGE_H
#define STARS_FILEDIALOG_WINDOWS_BRIDGE_H
#include "file_selection_types.h"

/* The SDK and wrapper have distinct HWND and OPENFILENAME definitions. */
win32::files::FILESELECTIONRESULT IChooseWindowsFile(
    void *lpWindow, const win32::files::FILESELECTIONREQUEST &request,
    const win32::files::FILESELECTIONPUMP &pfnPump);
#endif
