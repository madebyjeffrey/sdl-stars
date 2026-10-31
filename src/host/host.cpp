#include "common.h"
#include "host_profile.h"
#include "version.h"
#include "ownership.h"
#include "appstate.h"

// stars-host: the game's host without its windows. It takes the Windows
// game's host command line (-a, -g, -b, -t, -v, -s, -p, -l, and the -dm,
// -dp and -df dumps of a turn file; see ParseCmdLine) and exits with the
// same value; --version prints its version and --ini <file> reads the
// stars.ini settings the host uses.
// Waiting for turns (-w) needs the Windows game's timer and is refused.

// IdAlertBox shows ui.h's messages: there is no player to ask, so they go to
// stderr and questions get the cautious answer. hostui.c has the rest of
// ui.h.

int16_t IdAlertBox(const char *sz, int16_t mbType) {
    fprintf(stderr, "stars-host: %s\n", sz);
    switch (mbType & 0x000f) {
    case win32::MB_OKCANCEL:
        return IDCANCEL;
    case win32::MB_YESNOCANCEL:
    case win32::MB_YESNO:
        return win32::IDNO;
    default:
        return IDOK;
    }
}

// FReadHostIni reads the stars.ini settings the host uses, as
// ReadIniSettings does for the Windows game: logging, the default
// password, per-player dump files, host names in -v output and the number
// of backup directories.
static int16_t FReadHostIni(const char *szFile) {
    char *pszText;
    long  cb;
    NATIVEFILE *pf;

    pf = PfOpenNativeFile(szFile, "rb");
    if (!pf) {
        return 0;
    }
    StarsNativeFileGuard file(pf);
    ISeekNativeFile(pf, 0, SEEK_END);
    cb = LNativePosition(pf);
    ISeekNativeFile(pf, 0, SEEK_SET);
    NATIVEOWNER<char, std::free> text(static_cast<char *>(malloc((size_t)cb + 1)));
    pszText = text.get();
    if (!pszText || CReadNative(pszText, 1, (size_t)cb, pf) != (size_t)cb) {
        return 0;
    }
    ICloseNativeFile(file.Release());
    pszText[cb] = 0;
    if (!FUtf8Valid(pszText, (size_t)cb)) return 0;
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
    return 1;
}

int main(int argc, char *argv[]) try {
    NATIVECLEANUPSCOPE session([]() noexcept { GetAppSession().Close(); });
    char     szCmdLine[1024];
    int16_t  fSeed;
    uint32_t lSeed;
    int      i;
    char    *szIniPath;
#ifdef _WIN32
    char **rgpszNative;
#endif

    GetAppSession().BindFiles();
#ifdef _WIN32
    rgpszNative = RgpszNativeArguments(&argc);
    if (!rgpszNative) { fprintf(stderr, "stars-host: invalid Windows command line\n"); return 2; }
    NATIVEOWNER<char *, std::free> nativeArguments(rgpszNative);
    argv = rgpszNative;
#endif
    // ParseCmdLine reads each letter after a '-' as a switch.
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        printf("stars-host %s\n", STARS_VERSION_DISPLAY);
#ifdef _WIN32
        nativeArguments.reset();
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
            nativeArguments.reset();
#endif
            return 2;
        }
        if (i > 1) {
            strcat(szCmdLine, " ");
        }
        strcat(szCmdLine, argv[i]);
    }
    szBase[0] = 0;
    ini.SetWFlags(0);
    ini.idPlayer = iplrNone;
    InitGameStuff();
    if (szIniPath && !FReadHostIni(szIniPath)) {
        fprintf(stderr, "stars-host: can't read %s\n", szIniPath);
#ifdef _WIN32
        nativeArguments.reset();
#endif
        return 2;
    }
#ifdef _WIN32
    nativeArguments.reset();
#endif
    fSeed = 0;
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
    ini.fCmdLine = 0;
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
