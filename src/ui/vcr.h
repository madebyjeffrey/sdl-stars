#ifndef STARS_DECOMPILED_VCR_H
#define STARS_DECOMPILED_VCR_H

#include "win.h"
#include <stdint.h>
#include <windows.h>

void             BattleVCR(APPSESSION &app, int16_t iBattle);
uint8_t         *BtlDataGet(APPSESSION &app, int16_t i);
int32_t          CBattleUnits(APPSESSION &app, uint8_t *lpbd, BattleUnitFlags grbitBU);
int32_t          CBattleKills(APPSESSION &app, uint8_t *lpbd, int16_t fOurDead);
int32_t          LdpFromItokDv(APPSESSION &app, int16_t itok, DV *lpdv);
int16_t          SetVCRBoard(APPSESSION &app, int16_t iStep);
INT_PTR  VCRDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             GetVCRStats(APPSESSION &app, int16_t itok, int32_t *pdpArmor, DV *pdv, int32_t *pdpShields, int16_t *pcsh);
void             DrawVCR(APPSESSION &app, HDC hdc, int16_t iStart, int16_t iEnd);
void             Delay(int16_t ctick);
void             AnimateAttack(APPSESSION &app, HDC hdc);
int16_t          PopupVCRMenu(APPSESSION &app, HWND hwnd, int16_t x, int16_t y, uint8_t brc);
void             EnableVCRButtons(APPSESSION &app);

#endif
