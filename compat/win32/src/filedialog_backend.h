#ifndef STARS_FILEDIALOG_BACKEND_H
#define STARS_FILEDIALOG_BACKEND_H
#include "internal.h"
#include "file_selection.h"
/* Native paths are filesystem bytes (UTF-8 on ordinary Linux/macOS installs).
   Labels and paths use UTF-8; backends convert losslessly to native APIs. */
inline constexpr int cCompatFileFiltersMax = 64;
using FILESELECTIONBACKEND = win32::files::FILESELECTIONRESULT (*)(HWND, const win32::files::FILESELECTIONREQUEST &);
win32::files::FILESELECTIONRESULT ICompatChooseFile(HWND hwndOwner, const win32::files::FILESELECTIONREQUEST &request);

#endif
