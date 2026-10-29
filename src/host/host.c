#include "common.h"
#include "version.h"

// stars-host: the game's host without its windows. It takes the Windows
// game's host command line (-a, -g, -b, -t, -v, -s, -p, -l, and the -dm,
// -dp and -df dumps of a turn file; see ParseCmdLine) and exits with the
// same value; --version prints its version and --ini <file> reads the
// stars.ini settings the host uses.
// Waiting for turns (-w) needs the Windows game's timer and is refused.

// IdAlertBox shows ui.h's messages: there is no player to ask, so they go to
// stderr and questions get the cautious answer. hostui.c has the rest of
// ui.h.

int16_t IdAlertBox(char *sz, int16_t mbType) {
    fprintf(stderr, "stars-host: %s\n", sz);
    switch (mbType & 0x000f) {
    case MB_OKCANCEL:
        return IDCANCEL;
    case MB_YESNOCANCEL:
    case MB_YESNO:
        return IDNO;
    default:
        return IDOK;
    }
}

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
static int16_t FIniValue(char *pszText, StringId idsSection, StringId idsKey, char *szValue, size_t cchValue) {
    char    szSection[32];
    char    szKey[32];
    char    szLine[256];
    char   *pch;
    char   *pchEnd;
    char   *pchEq;
    size_t  cch;
    int16_t fInSection;

    CchGetString(idsSection, szSection);
    CchGetString(idsKey, szKey);
    fInSection = FALSE;
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
            memcpy(szValue, pchEq, cch); szValue[cch] = 0;
            return TRUE;
        }
    }
    return FALSE;
}

// LIniInt reads an INI key as GetPrivateProfileInt does: its leading
// number, or lDefault if the key is missing.
static int32_t LIniInt(char *pszText, StringId idsSection, StringId idsKey, int32_t lDefault) {
    char szValue[32];

    if (!FIniValue(pszText, idsSection, idsKey, szValue, sizeof(szValue))) {
        return lDefault;
    }
    return atol(szValue);
}

// FReadHostIni reads the stars.ini settings the host uses, as
// ReadIniSettings does for the Windows game: logging, the default
// password, per-player dump files, host names in -v output and the number
// of backup directories.
static int16_t FReadHostIni(const char *szFile) {
    char *pszText;
    long  cb;
    FILE *pf;

    pf = PfOpenNativeFile(szFile, "rb");
    if (!pf) {
        return FALSE;
    }
    fseek(pf, 0, SEEK_END);
    cb = ftell(pf);
    fseek(pf, 0, SEEK_SET);
    pszText = (char *)malloc((size_t)cb + 1);
    if (!pszText || fread(pszText, 1, (size_t)cb, pf) != (size_t)cb) {
        free(pszText);
        fclose(pf);
        return FALSE;
    }
    fclose(pf);
    pszText[cb] = 0;
    if (!FUtf8Valid(pszText, (size_t)cb)) { free(pszText); return FALSE; }
    if (cb >= 3 && !memcmp(pszText, "\xef\xbb\xbf", 3)) memmove(pszText, pszText + 3, (size_t)cb - 2);
    ini.fLogging = LIniInt(pszText, idsFiles, idsLogging, 0) != 0;
    if (!FIniValue(pszText, idsMisc, idsDefaultpassword, vszDefPass, 16)) {
        vszDefPass[0] = 0;
    }
    gd.fPerPlayerDumps = LIniInt(pszText, idsMisc, idsNewreports, 0) != 0;
    gd.fNoHostNames = LIniInt(pszText, idsMisc, idsNohostnames, 0) != 0;
    vcBackupDirs = LIniInt(pszText, idsMisc, idsBackups, 1);
    if (vcBackupDirs < 1 || vcBackupDirs > 999) {
        vcBackupDirs = 1;
    }
    free(pszText);
    return TRUE;
}

int main(int argc, char *argv[]) try {
    char     szCmdLine[1024];
    int16_t  fSeed;
    uint32_t lSeed;
    int      i;
    char    *szIniPath;
#ifdef _WIN32
    char **rgpszNative;
#endif

#ifdef _WIN32
    rgpszNative = RgpszNativeArguments(&argc);
    if (!rgpszNative) { fprintf(stderr, "stars-host: invalid Windows command line\n"); return 2; }
    argv = rgpszNative;
#endif
    // ParseCmdLine reads each letter after a '-' as a switch.
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        printf("stars-host %s\n", STARS_VERSION_DISPLAY);
#ifdef _WIN32
        free(rgpszNative);
#endif
        return 0;
    }
    // --ini <file> reads settings from a stars.ini, as stars.exe reads its
    // own; ParseCmdLine reads the rest.
    szIniPath = NULL;
    for (i = 1; i + 1 < argc; i++) {
        if (strcmp(argv[i], "--ini") == 0) {
            szIniPath = argv[i + 1];
            for (; i + 2 < argc; i++) {
                argv[i] = argv[i + 2];
            }
            argc -= 2;
            break;
        }
    }
    // ParseCmdLine reads one string, as WinMain received it. File names
    // can't hold spaces, as in the Windows game.
    szCmdLine[0] = 0;
    for (i = 1; i < argc; i++) {
        if (strlen(szCmdLine) + strlen(argv[i]) + 2 > sizeof(szCmdLine)) {
            fprintf(stderr, "stars-host: command line too long\n");
#ifdef _WIN32
            free(rgpszNative);
#endif
            return 2;
        }
        if (i > 1) {
            strcat(szCmdLine, " ");
        }
        strcat(szCmdLine, argv[i]);
    }
    szBase[0] = 0;
    ini.wFlags = 0;
    ini.idPlayer = iplrNone;
    InitGameStuff();
    if (szIniPath && !FReadHostIni(szIniPath)) {
        fprintf(stderr, "stars-host: can't read %s\n", szIniPath);
#ifdef _WIN32
        free(rgpszNative);
#endif
        return 2;
    }
#ifdef _WIN32
    free(rgpszNative);
#endif
    fSeed = FALSE;
    lSeed = 0;
    if (!ParseCmdLine(szCmdLine, &fSeed, &lSeed)) return 2;
    Randomize2(fSeed ? lSeed : DwTickCount());
    if (!ini.fCmdLine) {
        fprintf(
            stderr,
            "usage: stars-host --version | [--ini stars.ini] [-s<seed>] [-p<password>] [-l] (-a game.def | -v game.hst | -g[n] [-t] game.hst | -b batchfile | "
            "-d[m][p][f] game.mN)\n");
        return 2;
    }
    idPlayer = iplrNone;
    ini.fCmdLine = FALSE;
    if (!FRunCmdLine()) {
        if (ini.fDumpMap || ini.fDumpPlanets || ini.fDumpFleets) {
            if (!FDumpCmdLineGame()) {
                fprintf(stderr, "stars-host: -d needs a player's turn file (game.m1 and the like)\n");
                return 2;
            }
            return vretExitValue;
        }
        fprintf(stderr, "stars-host: nothing to do; -w and opening a game need the Windows game\n");
        return 2;
    }
    return vretExitValue;
} catch (const StarsError &error) {
    fprintf(stderr, "stars-host: %s failure (message %u)\n",
            error.mdError == starsErrorMemory ? "memory" : "file", (unsigned)error.ids);
    return 2;
} catch (...) {
    fprintf(stderr, "stars-host: unexpected operation failure\n");
    return 2;
}
