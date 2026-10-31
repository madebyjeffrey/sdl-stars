#ifndef STARS_PREFERENCESUI_H
#define STARS_PREFERENCESUI_H
#include <stdint.h>
void ReadIniPresets(char *szIniFile);
void WriteIniPresets(char *szIniFile);
void ReadIniSelection(const char *pszSelection, int16_t cch);
void ReadIniIdentity(const char *pszIdentity, int16_t cch);
void ReadIniRecent(char *szSection, char *szEntry, char *szIniFile, int16_t cch);
#endif
