#ifndef STARS_DECOMPILED_RESEARCH_H
#define STARS_DECOMPILED_RESEARCH_H

#include "common.h"
#include <stdint.h>

extern uint16_t rggrbitBrParts[17];
extern int32_t  rglTechCost[27];

int32_t GetTechLevelCost(APPSESSION &app, TechFieldType iTech, int16_t iLevel, int16_t iplr);
int32_t ProjectedResearchSpending(APPSESSION &app, int32_t pct);
int32_t CostOfDevelopingItem(APPSESSION &app, char *rgTech);
int16_t FShouldPartBeHidden(APPSESSION &app, PART *ppart);

#endif
