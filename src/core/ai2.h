#ifndef STARS_DECOMPILED_AI2_H
#define STARS_DECOMPILED_AI2_H

#include "common.h"
#include <cstdint>

extern std::uint8_t vrgISIshAip[];
extern std::uint8_t vrgISAip[];
extern std::uint8_t vrgAiISResOrder[];

void    DoMaidAiTurn(APPSESSION &app, PROD *rgprod);
std::int16_t FPotentISWarFleet(APPSESSION &app, FLEET *lpfl, std::int16_t iPotency);
void    DoAutomitronAiTurn(APPSESSION &app, PROD *rgprod);
void    EnsureISShdefs(APPSESSION &app, std::int16_t iroCur);
void    DoRototillAiTurn(APPSESSION &app, PROD *rgprod);
void    EnsureCAShdefs(std::int16_t iroCur);

#endif
