/* File URLs are UTF-8 and independent of the application's working directory. */
#include "internal.h"
#include "external_io.h"
#include <cerrno>
#include <new>
#include <cstring>

std::optional<std::string> SzCompatFileUri(const char *pszPath) {
    static const char rgchHex[] = "0123456789ABCDEF";
    const char *pszAbsolute;
    const unsigned char *lpch;
    unsigned char ch;

    try {
        const auto szAbsolute = SzCanonicalNativePath(pszPath);
        if (!szAbsolute) return std::nullopt;
        pszAbsolute = szAbsolute->c_str();
        std::string szUri = "file://";
        szUri.reserve(szAbsolute->size() * 3 + 8);
#ifdef _WIN32
        if (pszAbsolute[1] == ':') szUri.push_back('/');
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
            ) szUri.push_back(static_cast<char>(ch));
            else { szUri.push_back('%'); szUri.push_back(rgchHex[ch >> 4]); szUri.push_back(rgchHex[ch & 15]); }
        }
        return szUri;
    } catch (const std::bad_alloc &) { errno = ENOMEM; }
    catch (...) { errno = EIO; }
    return std::nullopt;
}

BOOL FCompatOpenDocument(const char *pszPath) {
    BOOL fOpened, fEventsOwned;
    const auto szUri = SzCompatFileUri(pszPath);

    if (!szUri) return 0;
    // A standalone help launch has no windows yet. Initialize SDL's platform
    // services so its Linux OpenFile portal is available before SDL_OpenURL.
    fEventsOwned = !(SDL_WasInit(SDL_INIT_EVENTS) & SDL_INIT_EVENTS);
    if (fEventsOwned && !SDL_InitSubSystem(SDL_INIT_EVENTS)) return 0;
    NATIVECLEANUPSCOPE events([&]() noexcept { if (fEventsOwned) SDL_QuitSubSystem(SDL_INIT_EVENTS); });
    fOpened = CompatExternalIO().pfnOpenURL(szUri->c_str());
    return fOpened;
}
