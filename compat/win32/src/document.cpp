/* File URLs are UTF-8 and independent of the application's working directory. */
#include "internal.h"
#include "external_io.h"
#include <cstdlib>
#include <cstring>

char *PszCompatFileUri(const char *pszPath) {
    static const char rgchHex[] = "0123456789ABCDEF";
    char *pszAbsolute, *pszUri, *lpchOut;
    const unsigned char *lpch;
    unsigned char ch;

    pszAbsolute = PszCanonicalNativePath(pszPath, nullptr, 0);
    NATIVEOWNER<char, std::free> absolute(pszAbsolute);
    if (!pszAbsolute) return nullptr;
    pszUri = static_cast<char *>((std::malloc(std::strlen(pszAbsolute) * 3 + 9)));
    NATIVEOWNER<char, std::free> uri(pszUri);
    if (!pszUri) return nullptr;
    std::memcpy(pszUri, "file://", 7); lpchOut = pszUri + 7;
#ifdef _WIN32
    if (pszAbsolute[1] == ':') *lpchOut++ = '/';
#endif
    lpch = reinterpret_cast<const unsigned char *>(pszAbsolute);
#ifdef _WIN32
    if (lpch[0] == '/' && lpch[1] == '/') lpch += 2;
#endif
    for (; *lpch; lpch++) {
        ch = *lpch;
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') || std::strchr("/-._~", ch)
#ifdef _WIN32
            || (ch == ':' && lpch == reinterpret_cast<const unsigned char *>(pszAbsolute) + 1)
#endif
        ) *lpchOut++ = static_cast<char>(ch);
        else { *lpchOut++ = '%'; *lpchOut++ = rgchHex[ch >> 4]; *lpchOut++ = rgchHex[ch & 15]; }
    }
    *lpchOut = 0;
    return uri.release();
}

BOOL FCompatOpenDocument(const char *pszPath) {
    char *pszUri = PszCompatFileUri(pszPath);
    BOOL fOpened, fEventsOwned;
    NATIVEOWNER<char, std::free> uri(pszUri);

    if (!pszUri) return 0;
    // A standalone help launch has no windows yet. Initialize SDL's platform
    // services so its Linux OpenFile portal is available before SDL_OpenURL.
    fEventsOwned = !(SDL_WasInit(SDL_INIT_EVENTS) & SDL_INIT_EVENTS);
    if (fEventsOwned && !SDL_InitSubSystem(SDL_INIT_EVENTS)) return 0;
    NATIVECLEANUPSCOPE events([&]() noexcept { if (fEventsOwned) SDL_QuitSubSystem(SDL_INIT_EVENTS); });
    fOpened = CompatExternalIO().pfnOpenURL(pszUri);
    return fOpened;
}
