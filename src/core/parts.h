#ifndef STARS_DECOMPILED_PARTS_H
#define STARS_DECOMPILED_PARTS_H

#include "common.h"
#include <cstdint>

extern SHDEF     rgshdefSBT[];
extern BEAM      rgbeam[];
extern SCANNER   rgscanner[];
extern HULDEF    rghuldefSB[];
extern SHDEF     rgshdefT[];
extern ENGINE    rgengine[];
extern HULDEF    rghuldef[];
extern MINING    rgmining[];
extern SHIELD    rgshield[];
extern TORP      rgtorp[];
extern TERRA     rgterra[];
extern PLANETARY rgplanetary[];
extern BOMB      rgbomb[];
extern ARMOR     rgarmor[];
extern SPECIALSB rgspecialSB[];
extern SPECIAL   rgspecialM[];
extern SPECIAL   rgspecialE[];
extern MINES     rgmines[];

HULDEF    *LphuldefSBFromId(isbhull id);
HULDEF    *LphuldefFromId(std::int16_t id);
ENGINE    *LpengineFromId(std::int16_t id);
SCANNER   *LpscannerFromId(std::int16_t id);
SHDEF     *LpshdefT();
SHDEF     *LpshdefSBT();
PLANETARY *LpplanetaryFromId(std::int16_t id);
std::int16_t    FLookupPartX(APPSESSION &app, PART *ppart, std::uint16_t grhst, std::uint16_t iItem);
std::int16_t    FLookupPart(APPSESSION &app, PART *ppart);
void       LookupBestPlanetaryScanner(APPSESSION &app, PART *ppart);
std::int16_t    TechStatus(APPSESSION &app, char *rgTech);

#endif
