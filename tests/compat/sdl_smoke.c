#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <stdio.h>
#include <string.h>

#define cWindows 2
#define dxSurface 320
#define dySurface 240

typedef struct SMOKEWINDOW {
    SDL_Window   *hwnd;
    SDL_Renderer *lpRenderer;
    SDL_Texture  *lpTexture;
    SDL_Surface  *lpSurface;
    SDL_WindowID  id;
    int           iFrame;
    bool          fPaused;
} SMOKEWINDOW;

static void CloseSmokeWindow(SMOKEWINDOW *lpwin) {
    SDL_DestroyTexture(lpwin->lpTexture);
    SDL_DestroySurface(lpwin->lpSurface);
    SDL_DestroyRenderer(lpwin->lpRenderer);
    SDL_DestroyWindow(lpwin->hwnd);
    memset(lpwin, 0, sizeof(*lpwin));
}

static void DispatchSmokeEvent(SMOKEWINDOW *rgwin, const SDL_Event *lpevent) {
    int i;

    for (i = 0; i < cWindows; i++) {
        if (!rgwin[i].hwnd)
            continue;
        if (lpevent->type == SDL_EVENT_QUIT ||
            (lpevent->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && lpevent->window.windowID == rgwin[i].id)) {
            SDL_Log("Closing window %d", i + 1);
            CloseSmokeWindow(&rgwin[i]);
        } else if (lpevent->type == SDL_EVENT_KEY_DOWN && lpevent->key.windowID == rgwin[i].id &&
                   lpevent->key.key == SDLK_SPACE && !lpevent->key.repeat) {
            rgwin[i].fPaused = !rgwin[i].fPaused;
        }
    }
}

static void PumpSmokeEvents(SMOKEWINDOW *rgwin) {
    SDL_Event event;

    while (SDL_PollEvent(&event))
        DispatchSmokeEvent(rgwin, &event);
}

static bool FCreateSmokeRenderers(SMOKEWINDOW *rgwin, SDL_GPUDevice **lplpDevice, const char *pszRenderer) {
    int i;

    if (strcmp(pszRenderer, "software") != 0) {
        *lplpDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL |
                                        SDL_GPU_SHADERFORMAT_MSL, false, NULL);
        if (*lplpDevice) {
            for (i = 0; i < cWindows; i++) {
                rgwin[i].lpRenderer = SDL_CreateGPURenderer(*lplpDevice, rgwin[i].hwnd);
                if (!rgwin[i].lpRenderer)
                    break;
            }
            if (i == cWindows) {
                SDL_Log("Shared GPU device: %s", SDL_GetGPUDeviceDriver(*lplpDevice));
                SDL_Log("GPU adapter: %s", SDL_GetStringProperty(SDL_GetGPUDeviceProperties(*lplpDevice),
                        SDL_PROP_GPU_DEVICE_NAME_STRING, "unknown"));
                return true;
            }
        }
        SDL_Log("GPU initialization failed: %s", SDL_GetError());
        for (i = 0; i < cWindows; i++) {
            SDL_DestroyRenderer(rgwin[i].lpRenderer);
            rgwin[i].lpRenderer = NULL;
        }
        SDL_DestroyGPUDevice(*lplpDevice);
        *lplpDevice = NULL;
        if (strcmp(pszRenderer, "gpu") == 0)
            return false;
        SDL_Log("Using software presentation fallback");
    }
    for (i = 0; i < cWindows; i++) {
        rgwin[i].lpRenderer = SDL_CreateRenderer(rgwin[i].hwnd, "software");
        if (!rgwin[i].lpRenderer)
            return false;
    }
    return true;
}

static bool FDrawSmokeWindow(SMOKEWINDOW *lpwin, int iWindow, bool fCheckPixels) {
    SDL_Rect     rc;
    SDL_Surface *lpRead;
    Uint8        red, green, blue, alpha;
    Uint32       crBackground;
    int          dx, dy;
    char         szText[96];
    bool         fMatch;

    crBackground = SDL_MapSurfaceRGB(lpwin->lpSurface, iWindow == 0 ? 24 : 64, 32, iWindow == 0 ? 64 : 24);
    rc = (SDL_Rect){(lpwin->iFrame * (iWindow + 1)) % (dxSurface - 48), 112, 48, 48};
    if (!SDL_FillSurfaceRect(lpwin->lpSurface, NULL, crBackground) ||
        !SDL_FillSurfaceRect(lpwin->lpSurface, &rc, SDL_MapSurfaceRGB(lpwin->lpSurface, 240, 192, 64)) ||
        !SDL_UpdateTexture(lpwin->lpTexture, NULL, lpwin->lpSurface->pixels, lpwin->lpSurface->pitch) ||
        !SDL_RenderTexture(lpwin->lpRenderer, lpwin->lpTexture, NULL, NULL) ||
        !SDL_SetRenderDrawColor(lpwin->lpRenderer, 255, 255, 255, 255))
        return false;
    snprintf(szText, sizeof(szText), "Window %d: %s", iWindow + 1, lpwin->fPaused ? "paused" : "running");
    if (!SDL_RenderDebugText(lpwin->lpRenderer, 16, 16, szText) ||
        !SDL_RenderDebugText(lpwin->lpRenderer, 16, 32, "Space pauses this window") ||
        !SDL_RenderDebugText(lpwin->lpRenderer, 16, 48, "Resize or close either window"))
        return false;
    if (fCheckPixels) {
        if (!SDL_GetRenderOutputSize(lpwin->lpRenderer, &dx, &dy) || dx < 8 || dy < 8)
            return SDL_SetError("Invalid render output size");
        rc = (SDL_Rect){4, dy - 5, 1, 1};
        lpRead = SDL_RenderReadPixels(lpwin->lpRenderer, &rc);
        if (!lpRead)
            return false;
        fMatch = SDL_ReadSurfacePixel(lpRead, 0, 0, &red, &green, &blue, &alpha) &&
                 red == (iWindow == 0 ? 24 : 64) && green == 32 && blue == (iWindow == 0 ? 64 : 24);
        SDL_DestroySurface(lpRead);
        if (!fMatch)
            return SDL_SetError("Window %d pixel readback mismatch", iWindow + 1);
    }
    if (!SDL_RenderPresent(lpwin->lpRenderer))
        return false;
    if (!lpwin->fPaused)
        lpwin->iFrame++;
    return true;
}

// Exercise the same queued input and close paths as the interactive loop,
// including rendering after the first window releases its GPU renderer.
static bool FSmokeSelfTest(SMOKEWINDOW *rgwin) {
    SDL_Event event;
    int       i;
    int       dx, dy;

    for (i = 0; i < cWindows; i++) {
        if (!FDrawSmokeWindow(&rgwin[i], i, true))
            return false;
    }
    SDL_zero(event);
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.windowID = rgwin[0].id;
    event.key.key = SDLK_SPACE;
    if (!SDL_PushEvent(&event))
        return false;
    PumpSmokeEvents(rgwin);
    if (!rgwin[0].hwnd || !rgwin[1].hwnd || !rgwin[0].fPaused || rgwin[1].fPaused)
        return SDL_SetError("Keyboard event was not isolated to its window");
    if (!SDL_SetWindowSize(rgwin[1].hwnd, 560, 360) || !SDL_SyncWindow(rgwin[1].hwnd) ||
        !SDL_GetWindowSize(rgwin[1].hwnd, &dx, &dy) || dx != 560 || dy != 360 ||
        !FDrawSmokeWindow(&rgwin[1], 1, true))
        return false;
    for (i = 0; i < cWindows; i++) {
        SDL_zero(event);
        event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
        event.window.windowID = rgwin[i].id;
        if (!SDL_PushEvent(&event))
            return false;
        PumpSmokeEvents(rgwin);
        if (rgwin[i].hwnd)
            return SDL_SetError("Close event did not destroy its window");
        if (i == 0 && (!rgwin[1].hwnd || !FDrawSmokeWindow(&rgwin[1], 1, true)))
            return SDL_SetError("Second window could not render after closing the first");
    }
    SDL_Log("PASS: two-window pixels, input isolation, resize, and independent close");
    return true;
}

int main(int argc, char **argv) {
    SMOKEWINDOW    rgwin[cWindows] = {0};
    SDL_GPUDevice *lpDevice = NULL;
    const char    *pszRenderer;
    char           szTitle[64];
    int            i;
    int            ret = 1;
    bool           fSelfTest = false;

    if (argc == 2 && strcmp(argv[1], "--self-test") == 0)
        fSelfTest = true;
    else if (argc != 1) {
        fprintf(stderr, "Usage: %s [--self-test]\n", argv[0]);
        return 2;
    }
    pszRenderer = SDL_getenv("STARS_SDL_RENDERER");
    if (!pszRenderer)
        pszRenderer = "auto";
    if (strcmp(pszRenderer, "auto") != 0 && strcmp(pszRenderer, "gpu") != 0 && strcmp(pszRenderer, "software") != 0) {
        fprintf(stderr, "STARS_SDL_RENDERER must be auto, gpu, or software\n");
        return 2;
    }
    if (!SDL_Init(SDL_INIT_VIDEO))
        goto done;
    SDL_Log("SDL %d.%d.%d; video driver: %s", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION,
            SDL_GetCurrentVideoDriver());
    for (i = 0; i < cWindows; i++) {
        snprintf(szTitle, sizeof(szTitle), "Stars SDL3 smoke - window %d", i + 1);
        rgwin[i].hwnd = SDL_CreateWindow(szTitle, 480, 360, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
        if (!rgwin[i].hwnd)
            goto done;
        rgwin[i].id = SDL_GetWindowID(rgwin[i].hwnd);
    }
    if (!FCreateSmokeRenderers(rgwin, &lpDevice, pszRenderer))
        goto done;
    for (i = 0; i < cWindows; i++) {
        SDL_Log("Window %d renderer: %s", i + 1, SDL_GetRendererName(rgwin[i].lpRenderer));
        rgwin[i].lpSurface = SDL_CreateSurface(dxSurface, dySurface, SDL_PIXELFORMAT_RGBA32);
        rgwin[i].lpTexture = SDL_CreateTexture(rgwin[i].lpRenderer, SDL_PIXELFORMAT_RGBA32,
                                              SDL_TEXTUREACCESS_STREAMING, dxSurface, dySurface);
        if (!rgwin[i].lpSurface || !rgwin[i].lpTexture ||
            !SDL_SetTextureScaleMode(rgwin[i].lpTexture, SDL_SCALEMODE_NEAREST))
            goto done;
    }
    if (fSelfTest) {
        if (!FSmokeSelfTest(rgwin))
            goto done;
    } else {
        while (rgwin[0].hwnd || rgwin[1].hwnd) {
            PumpSmokeEvents(rgwin);
            for (i = 0; i < cWindows; i++) {
                if (rgwin[i].hwnd && !(SDL_GetWindowFlags(rgwin[i].hwnd) & SDL_WINDOW_MINIMIZED) &&
                    !FDrawSmokeWindow(&rgwin[i], i, false))
                    goto done;
            }
            SDL_Delay(16);
        }
    }
    ret = 0;
done:
    if (ret != 0)
        SDL_Log("Smoke test failed: %s", SDL_GetError());
    for (i = 0; i < cWindows; i++)
        CloseSmokeWindow(&rgwin[i]);
    SDL_DestroyGPUDevice(lpDevice);
    SDL_Quit();
    return ret;
}
