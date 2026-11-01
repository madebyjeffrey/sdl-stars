/* Run entry.c against a fake WinMain to verify argument and exit contracts. */
#include <windows.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace ui {
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pszCommand, int nCmdShow) {
    const char *pszExpected = getenv("STARS_TEST_COMMAND");
    const char *pszHeadless = getenv("STARS_TEST_HEADLESS");
    WNDCLASS wc = {};
    HWND hwnd;
    RECT rc;
    if (!SDL_GetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING) ||
        strcmp(SDL_GetAppMetadataProperty(SDL_PROP_APP_METADATA_IDENTIFIER_STRING), "org.starsgame.Stars") ||
        !hInstance || hPrevInstance || nCmdShow != win32::SW_SHOWNORMAL ||
        !pszExpected || strcmp(pszCommand, pszExpected)) {
        fprintf(stderr, "Unexpected WinMain entry arguments\n");
        return 90;
    }
    if (pszHeadless) {
        wc.hInstance = hInstance; wc.lpszClassName = "HeadlessEntry"; wc.lpfnWndProc = win32::DefWindowProcA;
        if (!win32::RegisterClassA(&wc)) return 91;
        hwnd = win32::CreateWindowExA(0, wc.lpszClassName, "Invisible UI", win32::WS_VISIBLE | win32::WS_OVERLAPPEDWINDOW,
                            0, 0, 120, 80, NULL, NULL, hInstance, NULL);
        if (!hwnd || !SDL_GetCurrentVideoDriver() || strcmp(SDL_GetCurrentVideoDriver(), "dummy") ||
            strcmp(SDL_getenv("SDL_AUDIODRIVER"), "dummy") || strcmp(SDL_getenv("STARS_SDL_RENDERER"), "software") ||
            (!SDL_getenv("STARS_FILE_DIALOG_BACKEND") || strcmp(SDL_getenv("STARS_FILE_DIALOG_BACKEND"), "compat")) ||
            !win32::IsWindowVisible(hwnd) || !win32::GetClientRect(hwnd, &rc) || rc.right != 120 || rc.bottom != 80)
        {
            fprintf(stderr, "Headless window=%p video=%s audio=%s renderer=%s error=%s\n", (void *)hwnd,
                    SDL_GetCurrentVideoDriver(), SDL_getenv("SDL_AUDIODRIVER"), SDL_getenv("STARS_SDL_RENDERER"), SDL_GetError());
            return 92;
        }
    }
    return 37;
}

} // namespace ui
