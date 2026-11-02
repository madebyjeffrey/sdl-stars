#ifndef STARS_PREFERENCESUI_H
#define STARS_PREFERENCESUI_H

class APPSESSION;
#include <cstdint>
#include <string_view>
void ReadIniPresets(APPSESSION &app, const char *szIniFile);
void WriteIniPresets(APPSESSION &app, const char *szIniFile);
void ReadIniSelection(APPSESSION &app, std::string_view szSelection);
void ReadIniIdentity(APPSESSION &app, std::string_view szIdentity);
void ReadIniRecent(APPSESSION &app, const char *szSection, const char *szEntry, const char *szIniFile, std::int16_t cch);
#endif
