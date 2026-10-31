#ifndef STARS_HOST_PROFILE_H
#define STARS_HOST_PROFILE_H
#include "common.h"
int16_t FIniValue(const char *pszText, StringId idsSection, StringId idsKey, char *szValue, size_t cchValue);
int32_t LIniInt(const char *pszText, StringId idsSection, StringId idsKey, int32_t lDefault);
#endif
