#ifndef STARS_DECOMPILED_RESEARCH_H
#define STARS_DECOMPILED_RESEARCH_H

#include <stdint.h>

extern uint16_t rggrbitBrParts[17];
extern int32_t  rglTechCost[27];

int32_t GetTechLevelCost(TechFieldType iTech, int16_t iLevel, int16_t iplr);
int32_t ProjectedResearchSpending(int32_t pct);
int32_t CostOfDevelopingItem(char *rgTech);
int16_t FShouldPartBeHidden(PART *ppart);

#endif
