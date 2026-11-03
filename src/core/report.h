#ifndef STARS_DECOMPILED_REPORT_H
#define STARS_DECOMPILED_REPORT_H

#include "common.h"
#include <cstdint>
#include <array>

extern std::uint16_t mpicolgrbitBU[12];

struct REPORTTEXTRESULT {
    std::array<char, 360> sz{};
    std::int16_t cch = 0;

    char *data() noexcept { return sz.data(); }
    const char *data() const noexcept { return sz.data(); }
    std::string_view view() const noexcept { return {sz.data(), static_cast<std::size_t>(cch)}; }
};
REPORTTEXTRESULT PszGetDestName(APPSESSION &app, FLEET *lpfl, HDC hdc);
std::int16_t FDestIsWP0(FLEET *lpfl);
struct FLEETETARESULT {
    std::array<char, 360> sz;
    std::int16_t cYears;
    std::int16_t cch;

    std::string_view view() const noexcept { return {sz.data(), static_cast<std::size_t>(cch)}; }
};
struct TASKNAMERESULT {
    REPORTTEXTRESULT sz;
    std::int16_t icr;
};
FLEETETARESULT PszGetETA(APPSESSION &app, HDC hdc, FLEET *lpfl);
TASKNAMERESULT PszGetTaskName(APPSESSION &app, FLEET *lpfl);
void    DumpUniverse(APPSESSION &app);
void    DumpPlanets(APPSESSION &app);
void    DumpFleets(APPSESSION &app);
std::int16_t FDumpCmdLineGame(APPSESSION &app);

#endif
