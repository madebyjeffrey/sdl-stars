#ifndef STARS_HOST_PROFILE_H
#define STARS_HOST_PROFILE_H
#include "common.h"
#include <span>
#include <string_view>
bool FIniValue(APPSESSION &app, std::string_view szText, StringId idsSection, StringId idsKey, std::span<char> szValue);
std::int32_t LIniInt(APPSESSION &app, std::string_view szText, StringId idsSection, StringId idsKey, std::int32_t lDefault);
#endif
