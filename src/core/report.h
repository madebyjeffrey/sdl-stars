#ifndef STARS_DECOMPILED_REPORT_H
#define STARS_DECOMPILED_REPORT_H

#include "common.h"
#include <stdint.h>

extern uint16_t mpicolgrbitBU[12];

const char   *PszGetDestName(APPSESSION &app, FLEET *lpfl, HDC hdc);
int16_t FDestIsWP0(FLEET *lpfl);
const char   *PszGetETA(APPSESSION &app, HDC hdc, FLEET *lpfl, int16_t *pcYears);
const char   *PszGetTaskName(APPSESSION &app, FLEET *lpfl, int16_t *picr);
void    DumpUniverse(APPSESSION &app);
void    DumpPlanets(APPSESSION &app);
void    DumpFleets(APPSESSION &app);
int16_t FDumpCmdLineGame(APPSESSION &app);

#endif
