/* Check the Cocoa adapter boundary without requiring an interactive panel. */
#include "filedialog_backend.h"
#include "filedialog_cocoa_bridge.h"
#include <stdio.h>
#include <string.h>

static int cChoose, iResult = 1;

int IChooseCocoaFile(void *lpWindow, int fSave, const char *pszTitle, const char *pszFolder,
                     const char *pszName, const COCOAFILEFILTER *rgfilter, int cFilter,
                     int *lpiFilter, char *pszPath, size_t cbPath,
                     int (*pfnPump)(void *), void *lpv) {
    (void)lpWindow;
    cChoose++;
    if (!fSave || strcmp(pszTitle, "Choose \xe2\x80\x9cNew Game\xe2\x80\x9d") ||
        strcmp(pszFolder, "/tmp/\xe6\x98\x9f") || strcmp(pszName, "\xe6\x98\x9f.xy") ||
        cFilter != 1 || *lpiFilter != 0 || strcmp(rgfilter[0].pszName, "Games \xe2\x80\x93 Stars!") ||
        strcmp(rgfilter[0].pszPattern, "*.xy") || !pfnPump || lpv || cbPath < 32) return -2;
    if (iResult == 1) strcpy(pszPath, "/tmp/\xe6\x98\x9f/game.xy");
    return iResult;
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
