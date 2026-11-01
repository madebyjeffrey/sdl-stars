#ifndef STARS_DECOMPILED_AI3_H
#define STARS_DECOMPILED_AI3_H

#include "common.h"
#include <cstdint>

extern MacintiRecipe vrgMacIshAip[];
extern std::uint8_t       vrgMacAip[];
extern std::uint8_t       vrgAiMacintiResOrder[];

void    DoMacintiAiTurn(APPSESSION &app, PROD *rgprod);
void    EnsureMacintiShdefs(APPSESSION &app);
std::int16_t FRetargetMiner(APPSESSION &app, FLEET *lpfl);
std::int16_t IdTargetMacFreighter(APPSESSION &app, FLEET *lpfl);
void    TargetMacArmada(APPSESSION &app, FLEET *lpfl);
std::int16_t FPotentMacWarFleet(APPSESSION &app, FLEET *lpfl, std::int16_t *pcEquiv);

#endif
