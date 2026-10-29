#ifndef STARS_DECOMPILED_REPORT_H
#define STARS_DECOMPILED_REPORT_H

#include "common.h"
#include <stdint.h>

extern uint16_t mpicolgrbitBU[12];

const char   *PszGetDestName(FLEET *lpfl, HDC hdc);
int16_t FDestIsWP0(FLEET *lpfl);
const char   *PszGetETA(HDC hdc, FLEET *lpfl, int16_t *pcYears);
const char   *PszGetTaskName(FLEET *lpfl, int16_t *picr);
void    DumpUniverse();
void    DumpPlanets();
void    DumpFleets();
int16_t FDumpCmdLineGame();

#endif
