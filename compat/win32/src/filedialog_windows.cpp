#include "filedialog_backend.h"
#include "windows_chooser_io.h"
#include "filedialog_windows_bridge.h"
#include <cstdlib>
#include <array>
#include <vector>

using namespace win32::files;

static int FPumpWindowsChooser(void *lpv) {
    HWND hwndOwner = static_cast<HWND>(lpv);
    win32::MSG msg;

    while (win32::PeekMessageA(&msg, nullptr, 0, 0, win32::PM_REMOVE)) {
        if (msg.message == win32::WM_QUIT) { win32::PostQuitMessage(static_cast<int>(msg.wParam)); return 0; }
        win32::TranslateMessage(&msg); win32::DispatchMessageA(&msg);
        if (FStarsErrorPending()) return 0;
        if (hwndOwner && !win32::IsWindow(hwndOwner)) return 0;
    }
    return !FStarsErrorPending() && (!hwndOwner || win32::IsWindow(hwndOwner));
}

FILESELECTIONRESULT ICompatChooseFile(HWND hwndOwner, const FILESELECTIONREQUEST &request) {
    std::vector<WINDOWSFILEFILTER> rg;
    std::array<char, PATH_MAX> szPath{};
    void *lpWindow = nullptr;
    int i, iFilter = static_cast<int>(request.iFilter), iResult;

    if (request.rgFilters.size() > cCompatFileFiltersMax)
        return FILESELECTIONFAILURE{FILESELECTIONERROR::invalidRequest};
    rg.resize(request.rgFilters.size());
    for (i = 0; i < static_cast<int>(request.rgFilters.size()); i++) {
        rg[static_cast<std::size_t>(i)].pszName = request.rgFilters[static_cast<std::size_t>(i)].szName.c_str();
        rg[static_cast<std::size_t>(i)].pszPattern = request.rgFilters[static_cast<std::size_t>(i)].szPattern.c_str();
    }
    if (hwndOwner && hwndOwner->lpWindow.get())
        lpWindow = SDL_GetPointerProperty(SDL_GetWindowProperties(hwndOwner->lpWindow.get()), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    iResult = WindowsChooserIO().pfnIChooseWindowsFile(lpWindow, request.mode == FILESELECTIONMODE::save,
                                request.szTitle.c_str(), request.szInitialDirectory.c_str(), request.szInitialPath.c_str(),
                                rg.empty() ? nullptr : rg.data(), static_cast<int>(rg.size()),
                                &iFilter, szPath.data(), szPath.size(), FPumpWindowsChooser, hwndOwner);
    if (iResult < 0) return FILESELECTIONFAILURE{FILESELECTIONERROR::backendFailure};
    if (iResult == 0) return FILESELECTIONCANCELLED{};
    FILESELECTIONSELECTED selected;
    selected.szPath = szPath.data();
    if (!rg.empty()) selected.iFilter = static_cast<std::size_t>(iFilter);
    return selected;
}
