#import <AppKit/AppKit.h>
#include <SDL3/SDL.h>
#include <new>

struct COMPATCOCOAMODALSESSION {
    NSModalSession session;
    NSModalSession sessionPrevious;
};
static NSModalSession sessionModal;

void *PCompatBeginModalEventSession(SDL_Window *lpWindow) noexcept {
    NSWindow *lpNativeWindow;
    COMPATCOCOAMODALSESSION *lpsession;
    if (!SDL_IsMainThread() || !lpWindow) return nullptr;
    lpNativeWindow = (__bridge NSWindow *)SDL_GetPointerProperty(SDL_GetWindowProperties(lpWindow),
                                                                 SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
    if (!lpNativeWindow) return nullptr;
    lpsession = new (std::nothrow) COMPATCOCOAMODALSESSION{};
    if (!lpsession) return nullptr;
    lpsession->session = [NSApp beginModalSessionForWindow:lpNativeWindow];
    if (!lpsession->session) {
        delete lpsession;
        return nullptr;
    }
    lpsession->sessionPrevious = sessionModal;
    sessionModal = lpsession->session;
    return lpsession;
}

void CompatRunModalEventSession(void) noexcept {
    if (SDL_IsMainThread() && sessionModal) static_cast<void>([NSApp runModalSession:sessionModal]);
}

void CompatEndModalEventSession(void *pSession) noexcept {
    COMPATCOCOAMODALSESSION *lpsession = static_cast<COMPATCOCOAMODALSESSION *>(pSession);
    if (!SDL_IsMainThread() || !lpsession) return;
    [NSApp endModalSession:lpsession->session];
    sessionModal = lpsession->sessionPrevious;
    delete lpsession;
}
