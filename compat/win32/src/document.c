/* File URLs are UTF-8 and independent of the application's working directory. */
#include "internal.h"
#include <stdlib.h>
#include <string.h>

char *PszCompatFileUri(const char *pszPath) {
    static const char rgchHex[] = "0123456789ABCDEF";
    char *pszAbsolute, *pszUri, *lpchOut;
    const unsigned char *lpch;
    unsigned char ch;

    pszAbsolute = realpath(pszPath, NULL);
    if (!pszAbsolute) return NULL;
    pszUri = malloc(strlen(pszAbsolute) * 3 + 9);
    if (!pszUri) { free(pszAbsolute); return NULL; }
    memcpy(pszUri, "file://", 7); lpchOut = pszUri + 7;
#ifdef _WIN32
    if (pszAbsolute[1] == ':') *lpchOut++ = '/';
#endif
    lpch = (const unsigned char *)pszAbsolute;
#ifdef _WIN32
    if (lpch[0] == '/' && lpch[1] == '/') lpch += 2;
#endif
    for (; *lpch; lpch++) {
        ch = *lpch;
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') || strchr("/-._~", ch)
#ifdef _WIN32
            || (ch == ':' && lpch == (const unsigned char *)pszAbsolute + 1)
#endif
        ) *lpchOut++ = (char)ch;
        else { *lpchOut++ = '%'; *lpchOut++ = rgchHex[ch >> 4]; *lpchOut++ = rgchHex[ch & 15]; }
    }
    *lpchOut = 0; free(pszAbsolute);
    return pszUri;
}

BOOL FCompatOpenDocument(const char *pszPath) {
    char *pszUri = PszCompatFileUri(pszPath);
    BOOL fOpened, fEventsOwned;

    if (!pszUri) return FALSE;
    // A standalone help launch has no windows yet. Initialize SDL's platform
    // services so its Linux OpenFile portal is available before SDL_OpenURL.
    fEventsOwned = !(SDL_WasInit(SDL_INIT_EVENTS) & SDL_INIT_EVENTS);
    if (fEventsOwned && !SDL_InitSubSystem(SDL_INIT_EVENTS)) { free(pszUri); return FALSE; }
    fOpened = SDL_OpenURL(pszUri);
    if (fEventsOwned) SDL_QuitSubSystem(SDL_INIT_EVENTS);
    free(pszUri);
    return fOpened;
}
