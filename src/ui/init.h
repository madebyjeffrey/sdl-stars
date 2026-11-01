#ifndef STARS_DECOMPILED_INIT_H
#define STARS_DECOMPILED_INIT_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

extern uint8_t rgPalGray[20];

int16_t FCreateStuff(APPSESSION &app);
int16_t FCreateFonts(APPSESSION &app, HDC hdc);
int16_t InitInstance(APPSESSION &app, int16_t nCmdShow);
void    InitTiles(APPSESSION &app);
void    GetIniWinRc(APPSESSION &app, char *szSection, char *szIniFile, StringId ids, WN *pwn);
void    ReadIniSettings(APPSESSION &app);
void    ReadIniTileSettings(char *pszFormat, TILE *rgtile, int16_t ctile);


#endif
