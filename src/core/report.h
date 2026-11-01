#ifndef STARS_DECOMPILED_REPORT_H
#define STARS_DECOMPILED_REPORT_H

#include "common.h"
#include <cstdint>
#include <array>

extern std::uint16_t mpicolgrbitBU[12];

std::array<char, 360> PszGetDestName(APPSESSION &app, FLEET *lpfl, HDC hdc);
std::int16_t FDestIsWP0(FLEET *lpfl);
const char   *PszGetETA(APPSESSION &app, HDC hdc, FLEET *lpfl, std::int16_t *pcYears);
std::array<char, 360> PszGetTaskName(APPSESSION &app, FLEET *lpfl, std::int16_t *picr);
void    DumpUniverse(APPSESSION &app);
void    DumpPlanets(APPSESSION &app);
void    DumpFleets(APPSESSION &app);
std::int16_t FDumpCmdLineGame(APPSESSION &app);

#endif
