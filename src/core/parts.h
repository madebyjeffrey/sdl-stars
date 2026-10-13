#ifndef STARS_DECOMPILED_PARTS_H
#define STARS_DECOMPILED_PARTS_H

#include <stdint.h>

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
HULDEF    *LphuldefFromId(int16_t id);
ENGINE    *LpengineFromId(int16_t id);
SCANNER   *LpscannerFromId(int16_t id);
SHDEF     *LpshdefT();
SHDEF     *LpshdefSBT();
PLANETARY *LpplanetaryFromId(int16_t id);
int16_t    FLookupPartX(PART *ppart, uint16_t grhst, uint16_t iItem);
int16_t    FLookupPart(PART *ppart);
void       LookupBestPlanetaryScanner(PART *ppart);
int16_t    TechStatus(char *rgTech);

#endif
