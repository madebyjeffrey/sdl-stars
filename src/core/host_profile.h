#ifndef STARS_HOST_PROFILE_H
#define STARS_HOST_PROFILE_H
#include "common.h"
std::int16_t FIniValue(APPSESSION &app, const char *pszText, StringId idsSection, StringId idsKey, char *szValue, std::size_t cchValue);
std::int32_t LIniInt(APPSESSION &app, const char *pszText, StringId idsSection, StringId idsKey, std::int32_t lDefault);
#endif
