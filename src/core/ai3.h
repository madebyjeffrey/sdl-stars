#ifndef STARS_DECOMPILED_AI3_H
#define STARS_DECOMPILED_AI3_H

#include <stdint.h>

extern MacintiRecipe vrgMacIshAip[];
extern uint8_t       vrgMacAip[];
extern uint8_t       vrgAiMacintiResOrder[];

void    DoMacintiAiTurn(PROD *rgprod);
void    EnsureMacintiShdefs();
int16_t FRetargetMiner(FLEET *lpfl);
int16_t IdTargetMacFreighter(FLEET *lpfl);
void    TargetMacArmada(FLEET *lpfl);
int16_t FPotentMacWarFleet(FLEET *lpfl, int16_t *pcEquiv);

#endif
