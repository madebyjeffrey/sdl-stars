#include "win.h"
#include "bevystart.h"
#include "version.h"
#include <SDL3/SDL.h>
#ifdef __APPLE__
#include <SDL3/SDL_metal.h>
#endif

static int32_t IdBevyKey(const SDL_KeyboardEvent *lpEvent) {
    switch (lpEvent->key) {
    case SDLK_TAB: return lpEvent->mod & SDL_KMOD_SHIFT ? -3 : -2;
    case SDLK_RETURN:
    case SDLK_SPACE: return -4;
    case SDLK_N: return 0;
    case SDLK_O: return 1;
    case SDLK_C: return 2;
    case SDLK_X:
    case SDLK_ESCAPE: return 3;
    case SDLK_Q: return lpEvent->mod & (SDL_KMOD_GUI | SDL_KMOD_CTRL) ? 3 : -1;
    default: return -1;
    }
}

void ShowBevyStartupScreen(void) {
    SDL_Window *lpWindow;
    SDL_Event event;
    SDL_PropertiesID props;
    SDL_WindowFlags grFlags;
    BEVYWINDOW window;
    BEVYINPUT input;
    void *lpScreen;
    const char *rgpsz[4];
    char rgsz[4][80];
    const char *pszDriver;
    int i, dx, dy, cFrames;
    int32_t idAction;
    int16_t fQuit, fContinue;
#ifdef __APPLE__
    SDL_MetalView hView;
#endif

    pszDriver = SDL_GetCurrentVideoDriver();
    if (!pszDriver || !strcmp(pszDriver, "dummy"))
        return;
    memset(&window, 0, sizeof(window));
    grFlags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#ifdef __APPLE__
    grFlags |= SDL_WINDOW_METAL;
#endif
    lpWindow = SDL_CreateWindow("Stars!", 800, 600, grFlags);
    if (!lpWindow) {
        fprintf(stderr, "Stars!: cannot create Bevy startup window: %s\n", SDL_GetError());
        return;
    }
    SDL_SetWindowMinimumSize(lpWindow, 520, 380);
    props = SDL_GetWindowProperties(lpWindow);
#ifdef __APPLE__
    hView = SDL_Metal_CreateView(lpWindow);
    window.grBackend = 1;
    window.hWindow = (uintptr_t)hView;
#elif defined(_WIN32)
    window.grBackend = 2;
    window.hWindow = (uintptr_t)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#else
    if (!strcmp(pszDriver, "x11")) {
        window.grBackend = 3;
        window.hWindow = (uintptr_t)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        window.hDisplay = (uintptr_t)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
        window.iScreen = (int32_t)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_SCREEN_NUMBER, 0);
    } else if (!strcmp(pszDriver, "wayland")) {
        window.grBackend = 4;
        window.hWindow = (uintptr_t)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
        window.hDisplay = (uintptr_t)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
    }
#endif
    (void)props;
    SDL_GetWindowSizeInPixels(lpWindow, &dx, &dy);
    window.dx = (uint32_t)dx;
    window.dy = (uint32_t)dy;
    window.rScale = SDL_GetWindowDisplayScale(lpWindow);
    lpScreen = NULL;
    if (window.hWindow) {
        for (i = 0; i < 4; i++) {
            snprintf(rgsz[i], sizeof(rgsz[i]), "%s", PszGetCompressedString(idsNewGame + i));
            rgpsz[i] = rgsz[i];
        }
        fContinue = IsWindowEnabled(rghwndBtnSplash[2]);
        lpScreen = StarsBevyCreate(&window, rgpsz, STARS_VERSION_DISPLAY, fContinue);
    }
    if (!lpScreen) {
        fprintf(stderr, "Stars!: Bevy startup unavailable; using the SDL title screen\n");
        goto LFreeWindow;
    }
    ShowWindow(hwndTitle, SW_HIDE);
    memset(&input, 0, sizeof(input));
    cFrames = 0;
    fQuit = FALSE;
    while (hwndTitle && !fQuit) {
        input.fPress = FALSE;
        input.fRelease = FALSE;
        input.idKey = -1;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(lpWindow))) {
                fQuit = TRUE;
            } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.windowID == SDL_GetWindowID(lpWindow) && !event.key.repeat) {
                input.idKey = IdBevyKey(&event.key);
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.windowID == SDL_GetWindowID(lpWindow) && event.button.button == SDL_BUTTON_LEFT) {
                input.fPress = TRUE;
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.windowID == SDL_GetWindowID(lpWindow) && event.button.button == SDL_BUTTON_LEFT) {
                input.fRelease = TRUE;
            }
        }
        SDL_GetWindowSizeInPixels(lpWindow, &dx, &dy);
        input.dx = (uint32_t)dx;
        input.dy = (uint32_t)dy;
        input.rScale = SDL_GetWindowDisplayScale(lpWindow);
        SDL_GetMouseState(&input.x, &input.y);
        idAction = fQuit ? 3 : StarsBevyUpdate(lpScreen, &input);
        if (idAction == -2) {
            fprintf(stderr, "Stars!: Bevy startup failed; using the SDL title screen\n");
            ShowWindow(hwndTitle, SW_SHOW);
            break;
        }
        if (idAction >= 0 && idAction <= 3) {
            /* The frozen Bevy screen remains the backdrop. An unowned dialog
               avoids exposing the hidden SDL title as its native owner. */
            TitleWndProc(NULL, WM_COMMAND, (WPARAM)idAction, 0);
            if (idAction == 3)
                fQuit = TRUE;
            if (hwndTitle && !fQuit) {
                /* SDL dialogs can replace native presentation state. Rebuild
                   the small Bevy renderer after modal work, retaining the
                   original Continue availability and button handlers. */
                StarsBevyDestroy(lpScreen);
#ifdef __APPLE__
                SDL_Metal_DestroyView(hView);
                hView = SDL_Metal_CreateView(lpWindow);
                window.hWindow = (uintptr_t)hView;
#endif
                SDL_GetWindowSizeInPixels(lpWindow, &dx, &dy);
                window.dx = (uint32_t)dx;
                window.dy = (uint32_t)dy;
                window.rScale = SDL_GetWindowDisplayScale(lpWindow);
                lpScreen = window.hWindow ? StarsBevyCreate(&window, rgpsz, STARS_VERSION_DISPLAY, fContinue) : NULL;
                if (!lpScreen) {
                    fprintf(stderr, "Stars!: cannot resume Bevy startup; using the SDL title screen\n");
                    ShowWindow(hwndTitle, SW_SHOW);
                    break;
                }
                SDL_RaiseWindow(lpWindow);
            }
        } else if (++cFrames == 4) {
            SDL_ShowWindow(lpWindow);
            SDL_RaiseWindow(lpWindow);
        }
        SDL_Delay(16);
    }
    /* Drop the GPU surface before destroying its borrowed native window. */
    StarsBevyDestroy(lpScreen);
LFreeWindow:
#ifdef __APPLE__
    if (hView)
        SDL_Metal_DestroyView(hView);
#endif
    SDL_DestroyWindow(lpWindow);
}
