#ifndef STARS_PREFERENCESUI_H
#define STARS_PREFERENCESUI_H

class APPSESSION;
#include <cstdint>
void ReadIniPresets(APPSESSION &app, char *szIniFile);
void WriteIniPresets(APPSESSION &app, char *szIniFile);
void ReadIniSelection(APPSESSION &app, const char *pszSelection, std::int16_t cch);
void ReadIniIdentity(APPSESSION &app, const char *pszIdentity, std::int16_t cch);
void ReadIniRecent(APPSESSION &app, char *szSection, char *szEntry, char *szIniFile, std::int16_t cch);
#endif
