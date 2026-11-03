#ifndef STARS_FILE_SELECTION_H
#define STARS_FILE_SELECTION_H

#include "wincompat_types.h"
#include "file_selection_types.h"

namespace win32::files {

// Requests own their text through the modal pump. hwndOwner is borrowed only
// for this call; implementations recheck it after every modal callback.
FILESELECTIONRESULT SelectFile(HWND hwndOwner, FILESELECTIONREQUEST request);

} // namespace win32::files

#endif
