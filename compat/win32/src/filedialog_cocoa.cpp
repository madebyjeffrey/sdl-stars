#include "filedialog_backend.h"
#include "filedialog_cocoa_bridge.h"
#include <cstdlib>
#include <cstring>
#include <array>
#include <vector>

using namespace win32::files;

struct COCOAFILTERTEXT {
    NATIVEOWNER<char, SDL_free> pszName, pszPattern;
};

static char *PszCocoaLabelUtf8(LPCSTR psz) {
    return FUtf8Valid(psz) ? SDL_strdup(psz) : nullptr;
}
static int FPumpCocoaChooser(void *lpv) {
    HWND hwndOwner = static_cast<HWND>((lpv));
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
    std::vector<COCOAFILEFILTER> rg;
    std::array<char, PATH_MAX> szPath{};
    char *pszTitleUtf;
    void *lpWindow = nullptr;
    int i, iFilter = static_cast<int>(request.iFilter), iResult = -1;
    NATIVEOWNER<char, SDL_free> titleOwner;
    std::array<COCOAFILTERTEXT, cCompatFileFiltersMax> rgText;
    if (request.rgFilters.size() > cCompatFileFiltersMax)
        return FILESELECTIONFAILURE{FILESELECTIONERROR::invalidRequest};
    pszTitleUtf = PszCocoaLabelUtf8(request.szTitle.c_str());
    titleOwner.reset(pszTitleUtf);
    if (!pszTitleUtf) return FILESELECTIONFAILURE{FILESELECTIONERROR::backendFailure};
    rg.resize(request.rgFilters.size());
    for (i = 0; i < static_cast<int>(request.rgFilters.size()); i++) {
        rgText[static_cast<std::size_t>(i)].pszName.reset(PszCocoaLabelUtf8(request.rgFilters[static_cast<std::size_t>(i)].szName.c_str()));
        rgText[static_cast<std::size_t>(i)].pszPattern.reset(PszCocoaLabelUtf8(request.rgFilters[static_cast<std::size_t>(i)].szPattern.c_str()));
        rg[static_cast<std::size_t>(i)].pszName = rgText[static_cast<std::size_t>(i)].pszName.get();
        rg[static_cast<std::size_t>(i)].pszPattern = rgText[static_cast<std::size_t>(i)].pszPattern.get();
        if (!rg[static_cast<std::size_t>(i)].pszName || !rg[static_cast<std::size_t>(i)].pszPattern)
            return FILESELECTIONFAILURE{FILESELECTIONERROR::backendFailure};
    }
    if (hwndOwner && hwndOwner->lpWindow.get())
        lpWindow = SDL_GetPointerProperty(SDL_GetWindowProperties(hwndOwner->lpWindow.get()), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
    iResult = IChooseCocoaFile(lpWindow, request.mode == FILESELECTIONMODE::save, pszTitleUtf,
                             request.szInitialDirectory.c_str(), request.szInitialPath.c_str(),
                             rg.empty() ? nullptr : rg.data(), static_cast<int>(rg.size()),
                             &iFilter, szPath.data(), szPath.size(), FPumpCocoaChooser, hwndOwner);
    if (iResult < 0) return FILESELECTIONFAILURE{FILESELECTIONERROR::backendFailure};
    if (iResult == 0) return FILESELECTIONCANCELLED{};
    FILESELECTIONSELECTED selected;
    selected.szPath = szPath.data();
    if (!rg.empty()) selected.iFilter = static_cast<std::size_t>(iFilter);
    return selected;
}
