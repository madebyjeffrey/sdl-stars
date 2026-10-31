#ifndef STARS_DECOMPILED_AI3_H
#define STARS_DECOMPILED_AI3_H

#include "common.h"
#include <stdint.h>

extern MacintiRecipe vrgMacIshAip[];
extern uint8_t       vrgMacAip[];
extern uint8_t       vrgAiMacintiResOrder[];

void    DoMacintiAiTurn(APPSESSION &app, PROD *rgprod);
void    EnsureMacintiShdefs(APPSESSION &app);
int16_t FRetargetMiner(APPSESSION &app, FLEET *lpfl);
int16_t IdTargetMacFreighter(APPSESSION &app, FLEET *lpfl);
void    TargetMacArmada(APPSESSION &app, FLEET *lpfl);
int16_t FPotentMacWarFleet(APPSESSION &app, FLEET *lpfl, int16_t *pcEquiv);

#endif
