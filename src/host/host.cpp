#include "common.h"
#include <algorithm>
#include <array>
#include <new>
#include <string>
#include <string_view>
#include <vector>
#include "appstate.h"
#ifdef STARS_UI_SDL
#include "uistate.h"
#endif
#include "callbackcontext.h"
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
// stderr and questions get the cautious answer. hostui.cpp has the rest of
// ui.h.

std::int16_t IdAlertBox(std::string_view sz, std::int16_t mbType) {
    std::fputs("stars-host: ", stderr);
    std::fwrite(sz.data(), 1, sz.size(), stderr);
    std::fputc('\n', stderr);
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
static std::int16_t FReadHostIni(APPSESSION &app, std::string_view szFile) {
    std::optional<std::int64_t> cb;
    std::string szText;
    std::string_view szIni;
    NATIVEFILE *pf;

    pf = PfOpenNativeFile(szFile, NATIVEFILEMODE::read);
    if (!pf) {
        return 0;
    }
    NATIVEFILEOWNER file(pf, ICloseNativeFile);
    LSeekNativeFile(file.get(), 0, NATIVESEEKORIGIN::end);
    cb = LNativePosition(file.get());
    LSeekNativeFile(file.get(), 0, NATIVESEEKORIGIN::begin);
    if (!cb || *cb < 0 || static_cast<std::uint64_t>(*cb) > szText.max_size()) {
        return 0;
    }
    try {
        szText.resize(static_cast<std::size_t>(*cb));
    } catch (const std::bad_alloc &) {
        return 0;
    }
    if (CReadNative(std::span{szText.data(), szText.size()}, file.get()) != szText.size()) return 0;
    file.reset();
    szIni = szText;
    if (!FUtf8Valid(szIni)) return 0;
    if (szIni.starts_with("\xef\xbb\xbf")) szIni.remove_prefix(3);
    app.state.ini.fLogging = LIniInt(app, szIni, idsFiles, idsLogging, 0) != 0;
    if (!FIniValue(app, szIni, idsMisc, idsDefaultpassword, std::span{app.state.vszDefPass})) {
        app.state.vszDefPass[0] = 0;
    }
    app.state.gd.fPerPlayerDumps = LIniInt(app, szIni, idsMisc, idsNewreports, 0) != 0;
    app.state.gd.fNoHostNames = LIniInt(app, szIni, idsMisc, idsNohostnames, 0) != 0;
    app.state.vcBackupDirs = LIniInt(app, szIni, idsMisc, idsBackups, 1);
    if (app.state.vcBackupDirs < 1 || app.state.vcBackupDirs > 999) {
        app.state.vcBackupDirs = 1;
    }
    return 1;
}

int main(int argc, char *argv[]) try {
    APPSESSION &app = host::app;
    COMPATCALLBACKCONTEXTSCOPE callbackContext(&app);
    NATIVECLEANUPSCOPE session([&app]() noexcept { app.Close(); });
    std::array<char, 1024> szCmdLine{};
    std::size_t cchCmdLine;
    int      i;
    char    *szIniPath;
#ifdef _WIN32
    std::optional<std::vector<std::string>> rgszNative;
    std::vector<char *> rgpszNative;
#endif

    app.BindFiles();
#ifdef _WIN32
    rgszNative = RgNativeArguments();
    if (!rgszNative) { std::fprintf(stderr, "stars-host: invalid Windows command line\n"); return 2; }
    rgpszNative.reserve(rgszNative->size());
    for (auto &sz : *rgszNative) rgpszNative.push_back(sz.data());
    argc = static_cast<int>(rgpszNative.size());
    argv = rgpszNative.data();
#endif
    // ParseCmdLine reads each letter after a '-' as a switch.
    if (argc == 2 && std::strcmp(argv[1], "--version") == 0) {
        std::printf("stars-host %s\n", STARS_VERSION_DISPLAY);
        return 0;
    }
    // --ini <file> reads settings from a stars.ini, as stars.exe reads its
    // own; ParseCmdLine reads the rest.
    szIniPath = nullptr;
    for (i = 1; i + 1 < argc; i++) {
        if (std::strcmp(argv[i], "--ini") == 0) {
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
    cchCmdLine = 0;
    for (i = 1; i < argc; i++) {
        const std::string_view szArgument(argv[i]);
        if (cchCmdLine + 2 > szCmdLine.size() || szArgument.size() > szCmdLine.size() - cchCmdLine - 2) {
            std::fprintf(stderr, "stars-host: command line too long\n");
            return 2;
        }
        if (i > 1) {
            szCmdLine[cchCmdLine++] = ' ';
        }
        std::ranges::copy(szArgument, std::span{szCmdLine}.subspan(cchCmdLine).begin());
        cchCmdLine += szArgument.size();
        szCmdLine[cchCmdLine] = 0;
    }
    app.state.szBase[0] = 0;
    app.state.ini.SetWFlags(0);
    app.state.ini.idPlayer = iplrNone;
    InitGameStuff(app);
    if (szIniPath && !FReadHostIni(app, szIniPath)) {
        std::fprintf(stderr, "stars-host: can't read %s\n", szIniPath);
        return 2;
    }
#ifdef _WIN32
    rgszNative.reset();
#endif
    const auto cmdLineResult = ParseCmdLine(app, std::string_view{szCmdLine.data(), cchCmdLine});
    if (!cmdLineResult.fValid) return 2;
    Randomize2(app, cmdLineResult.lSeed ? *cmdLineResult.lSeed : DwTickCount());
    if (!app.state.ini.fCmdLine) {
        std::fprintf(
            stderr,
            "usage: stars-host --version | [--ini stars.ini] [-s<seed>] [-p<password>] [-l] (-a game.def | -v game.hst | -g[n] [-t] game.hst | -b batchfile | "
            "-d[m][p][f] game.mN)\n");
        return 2;
    }
    app.state.idPlayer = iplrNone;
    app.state.ini.fCmdLine = 0;
    if (!FRunCmdLine(app)) {
        if (app.state.ini.fDumpMap || app.state.ini.fDumpPlanets || app.state.ini.fDumpFleets) {
            if (!FDumpCmdLineGame(app)) {
                std::fprintf(stderr, "stars-host: -d needs a player's turn file (game.m1 and the like)\n");
                return 2;
            }
            return app.state.vretExitValue;
        }
        std::fprintf(stderr, "stars-host: nothing to do; -w and opening a game need the Windows game\n");
        return 2;
    }
    return app.state.vretExitValue;
} catch (const StarsError &error) {
    std::fprintf(stderr, "stars-host: %s failure (message %u)\n",
            error.mdError == starsErrorMemory ? "memory" : "file", static_cast<unsigned>(error.ids));
    return 2;
} catch (...) {
    std::fprintf(stderr, "stars-host: unexpected operation failure\n");
    return 2;
}
