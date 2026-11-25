#include "filedialog_backend.h"
#include "windows_chooser_io.h"
#include "filedialog_windows_bridge.h"

using namespace win32::files;

static bool FPumpWindowsChooser(HWND hwndOwner) {
    win32::MSG msg;

    while (win32::PeekMessageA(&msg, nullptr, 0, 0, win32::PM_REMOVE)) {
        if (msg.message == win32::WM_QUIT) { win32::PostQuitMessage(static_cast<int>(msg.wParam)); return false; }
        win32::TranslateMessage(&msg); win32::DispatchMessageA(&msg);
        if (FStarsErrorPending()) return false;
        if (hwndOwner && !win32::IsWindow(hwndOwner)) return false;
    }
    return !FStarsErrorPending() && (!hwndOwner || win32::IsWindow(hwndOwner));
}

FILESELECTIONRESULT ICompatChooseFile(HWND hwndOwner, const FILESELECTIONREQUEST &request) {
    void *lpWindow = nullptr;

    if (request.rgFilters.size() > cCompatFileFiltersMax)
        return FILESELECTIONFAILURE{FILESELECTIONERROR::invalidRequest};
    if (hwndOwner && hwndOwner->lpWindow.get())
        lpWindow = SDL_GetPointerProperty(SDL_GetWindowProperties(hwndOwner->lpWindow.get()), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    const FILESELECTIONPUMP pfnPump = [hwndOwner] { return FPumpWindowsChooser(hwndOwner); };
    return WindowsChooserIO().pfnIChooseWindowsFile(lpWindow, request, pfnPump);
}
