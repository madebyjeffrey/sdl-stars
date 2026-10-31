#ifndef STARS_DECOMPILED_AI2_H
#define STARS_DECOMPILED_AI2_H

#include "common.h"
#include <stdint.h>

extern uint8_t vrgISIshAip[];
extern uint8_t vrgISAip[];
extern uint8_t vrgAiISResOrder[];

void    DoMaidAiTurn(APPSESSION &app, PROD *rgprod);
int16_t FPotentISWarFleet(APPSESSION &app, FLEET *lpfl, int16_t iPotency);
void    DoAutomitronAiTurn(APPSESSION &app, PROD *rgprod);
void    EnsureISShdefs(APPSESSION &app, int16_t iroCur);
void    DoRototillAiTurn(APPSESSION &app, PROD *rgprod);
void    EnsureCAShdefs(int16_t iroCur);

#endif
