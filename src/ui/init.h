#ifndef STARS_DECOMPILED_INIT_H
#define STARS_DECOMPILED_INIT_H

#include "win.h"
#include <cstdint>
#include <windows.h>

extern std::uint8_t rgPalGray[20];

std::int16_t FCreateStuff(APPSESSION &app);
std::int16_t FCreateFonts(APPSESSION &app, HDC hdc);
std::int16_t InitInstance(APPSESSION &app, std::int16_t nCmdShow);
void    InitTiles(APPSESSION &app);
void    GetIniWinRc(APPSESSION &app, char *szSection, char *szIniFile, StringId ids, WN *pwn);
void    ReadIniSettings(APPSESSION &app);
void    ReadIniTileSettings(char *pszFormat, TILE *rgtile, std::int16_t ctile);


#endif
