#ifndef STARS_DECOMPILED_INIT_H
#define STARS_DECOMPILED_INIT_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

extern uint8_t rgPalGray[20];

int16_t FCreateStuff();
int16_t FCreateFonts(HDC hdc);
int16_t InitInstance(int16_t nCmdShow);
void    InitTiles();
void    GetIniWinRc(char *szSection, char *szIniFile, StringId ids, WN *pwn);
void    ReadIniSettings();
void    ReadIniTileSettings(char *pszFormat, TILE *rgtile, int16_t ctile);

decltype(InitInstance) STARS_TEST_IMPL(InitInstance);

#endif
