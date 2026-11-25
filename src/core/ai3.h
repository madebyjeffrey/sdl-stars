#ifndef STARS_DECOMPILED_AI3_H
#define STARS_DECOMPILED_AI3_H

#include "common.h"
#include <cstdint>

struct MACWARFLEETPOTENCYRESULT {
    bool fPotent;
    std::int16_t cEquiv;
};

extern MacintiRecipe vrgMacIshAip[];
extern std::uint8_t       vrgMacAip[cvrgMacAip];
extern std::uint8_t       vrgAiMacintiResOrder[];

void    DoMacintiAiTurn(APPSESSION &app, std::span<PROD, 64> rgprod);
void    EnsureMacintiShdefs(APPSESSION &app);
std::int16_t FRetargetMiner(APPSESSION &app, FLEET *lpfl);
std::int16_t IdTargetMacFreighter(APPSESSION &app, FLEET *lpfl);
void    TargetMacArmada(APPSESSION &app, FLEET *lpfl);
MACWARFLEETPOTENCYRESULT FPotentMacWarFleet(APPSESSION &app, FLEET *lpfl);

#endif
