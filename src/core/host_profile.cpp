#include "host_profile.h"
#include "text_io.h"

// FSzEqNoCase compares two strings, ignoring case, as INI names are.
static int16_t FSzEqNoCase(const char *sz1, const char *sz2) {
    while (*sz1 != 0 && tolower((unsigned char)*sz1) == tolower((unsigned char)*sz2)) {
        sz1++;
        sz2++;
    }
    return *sz1 == 0 && *sz2 == 0;
}

// FIniValue copies key szKey's value in [szSection] of the INI text pszText
// into szValue, as GetPrivateProfileString finds it.
int16_t FIniValue(APPSESSION &app, const char *pszText, StringId idsSection, StringId idsKey, char *szValue, size_t cchValue) {
    char        szSection[32];
    char        szKey[32];
    char        szLine[256];
    const char *pch;
    const char *pchEnd;
    char       *pchEq;
    size_t      cch;
    int16_t     fInSection;

    TextIO().pfnCchGetString(app, idsSection, szSection);
    TextIO().pfnCchGetString(app, idsKey, szKey);
    fInSection = 0;
    for (pch = pszText; *pch != 0; pch = *pchEnd != 0 ? pchEnd + 1 : pchEnd) {
        pchEnd = strchr(pch, '\n');
        if (!pchEnd) {
            pchEnd = pch + strlen(pch);
        }
        cch = (size_t)(pchEnd - pch) < sizeof(szLine) - 1 ? (size_t)(pchEnd - pch) : sizeof(szLine) - 1;
        cch = CchUtf8Prefix(pch, cch);
        memcpy(szLine, pch, cch);
        while (cch > 0 && (szLine[cch - 1] == '\r' || szLine[cch - 1] == ' ' || szLine[cch - 1] == '\t')) {
            cch--;
        }
        szLine[cch] = 0;
        if (szLine[0] == '[') {
            pchEq = strchr(szLine, ']');
            if (pchEq) {
                *pchEq = 0;
                fInSection = FSzEqNoCase(&szLine[1], szSection);
            }
            continue;
        }
        pchEq = strchr(szLine, '=');
        if (!fInSection || !pchEq) {
            continue;
        }
        *pchEq = 0;
        for (cch = strlen(szLine); cch > 0 && (szLine[cch - 1] == ' ' || szLine[cch - 1] == '\t'); cch--) {
            szLine[cch - 1] = 0;
        }
        if (FSzEqNoCase(szLine, szKey)) {
            for (pchEq++; *pchEq == ' ' || *pchEq == '\t'; pchEq++) {
            }
            cch = CchUtf8Prefix(pchEq, cchValue - 1);
            memcpy(szValue, pchEq, cch);
            szValue[cch] = 0;
            return 1;
        }
    }
    return 0;
}

// LIniInt reads an INI key as GetPrivateProfileInt does: its leading
// number, or lDefault if the key is missing.
int32_t LIniInt(APPSESSION &app, const char *pszText, StringId idsSection, StringId idsKey, int32_t lDefault) {
    char szValue[32];

    if (!FIniValue(app, pszText, idsSection, idsKey, szValue, sizeof(szValue))) {
        return lDefault;
    }
    return atol(szValue);
}
