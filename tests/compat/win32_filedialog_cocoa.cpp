/* Check the Cocoa adapter boundary without requiring an interactive panel. */
#include "filedialog_backend.h"
#include "filedialog_cocoa_bridge.h"
#include <stdio.h>
#include <string.h>

static int cChoose, iResult = 1;

win32::files::FILESELECTIONRESULT IChooseCocoaFile(
    void *lpWindow, const win32::files::FILESELECTIONREQUEST &request,
    const win32::files::FILESELECTIONPUMP &pfnPump) {
    (void)lpWindow;
    cChoose++;
    if (request.mode != win32::files::FILESELECTIONMODE::save ||
        request.szTitle != "Choose \xe2\x80\x9cNew Game\xe2\x80\x9d" ||
        request.szInitialDirectory != "/tmp/\xe6\x98\x9f" || request.szInitialPath != "\xe6\x98\x9f.xy" ||
        request.rgFilters.size() != 1 || request.iFilter != 0 ||
        request.rgFilters[0].szName != "Games \xe2\x80\x93 Stars!" ||
        request.rgFilters[0].szPattern != "*.xy" || !pfnPump) {
        return win32::files::FILESELECTIONFAILURE{win32::files::FILESELECTIONERROR::backendFailure};
    }
    if (iResult == 0) return win32::files::FILESELECTIONCANCELLED{};
    win32::files::FILESELECTIONSELECTED selected;
    selected.szPath = "/tmp/\xe6\x98\x9f/game.xy";
    selected.iFilter = 0;
    return selected;
}

int main(void) {
    win32::files::FILESELECTIONREQUEST request;
    win32::files::FILESELECTIONRESULT result;
    request.mode = win32::files::FILESELECTIONMODE::save;
    request.szTitle = "Choose \xe2\x80\x9cNew Game\xe2\x80\x9d";
    request.szInitialDirectory = "/tmp/\xe6\x98\x9f";
    request.szInitialPath = "\xe6\x98\x9f.xy";
    request.rgFilters.push_back({"Games \xe2\x80\x93 Stars!", "*.xy"});

    result = ICompatChooseFile(NULL, request);
    if (!std::holds_alternative<win32::files::FILESELECTIONSELECTED>(result) || cChoose != 1 ||
        std::get<win32::files::FILESELECTIONSELECTED>(result).szPath != "/tmp/\xe6\x98\x9f/game.xy" ||
        std::get<win32::files::FILESELECTIONSELECTED>(result).iFilter != 0) return 1;
    iResult = 0;
    result = ICompatChooseFile(NULL, request);
    if (!std::holds_alternative<win32::files::FILESELECTIONCANCELLED>(result) || cChoose != 2) return 1;
    puts("Cocoa picker: UTF-8 labels, UTF-8 paths and cancellation passed");
    return 0;
}
