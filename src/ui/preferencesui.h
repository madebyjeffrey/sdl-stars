#ifndef STARS_PREFERENCESUI_H
#define STARS_PREFERENCESUI_H

class APPSESSION;
#include <cstdint>
#include <string_view>
void ReadIniPresets(APPSESSION &app, std::string_view szIniFile);
void WriteIniPresets(APPSESSION &app, std::string_view szIniFile);
void ReadIniSelection(APPSESSION &app, std::string_view szSelection);
void ReadIniIdentity(APPSESSION &app, std::string_view szIdentity);
void ReadIniRecent(APPSESSION &app, std::string_view szSection, std::string_view szEntry, std::string_view szIniFile, std::string_view szRecentPath);
#endif
