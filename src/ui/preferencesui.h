#ifndef STARS_PREFERENCESUI_H
#define STARS_PREFERENCESUI_H

class APPSESSION;
#include <stdint.h>
void ReadIniPresets(APPSESSION &app, char *szIniFile);
void WriteIniPresets(APPSESSION &app, char *szIniFile);
void ReadIniSelection(APPSESSION &app, const char *pszSelection, int16_t cch);
void ReadIniIdentity(APPSESSION &app, const char *pszIdentity, int16_t cch);
void ReadIniRecent(APPSESSION &app, char *szSection, char *szEntry, char *szIniFile, int16_t cch);
#endif
