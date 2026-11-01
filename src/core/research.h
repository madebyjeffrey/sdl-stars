#ifndef STARS_DECOMPILED_RESEARCH_H
#define STARS_DECOMPILED_RESEARCH_H

#include "common.h"
#include <cstdint>

extern std::uint16_t rggrbitBrParts[17];
extern std::int32_t  rglTechCost[27];

std::int32_t GetTechLevelCost(APPSESSION &app, TechFieldType iTech, std::int16_t iLevel, std::int16_t iplr);
std::int32_t ProjectedResearchSpending(APPSESSION &app, std::int32_t pct);
std::int32_t CostOfDevelopingItem(APPSESSION &app, char *rgTech);
std::int16_t FShouldPartBeHidden(APPSESSION &app, PART *ppart);

#endif
