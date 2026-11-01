#ifndef STARS_DECOMPILED_VCR_H
#define STARS_DECOMPILED_VCR_H

#include "win.h"
#include <cstdint>
#include <windows.h>

void             BattleVCR(APPSESSION &app, std::int16_t iBattle);
std::uint8_t         *BtlDataGet(APPSESSION &app, std::int16_t i);
std::int32_t          CBattleUnits(APPSESSION &app, std::uint8_t *lpbd, BattleUnitFlags grbitBU);
std::int32_t          CBattleKills(APPSESSION &app, std::uint8_t *lpbd, std::int16_t fOurDead);
std::int32_t          LdpFromItokDv(APPSESSION &app, std::int16_t itok, DV *lpdv);
std::int16_t          SetVCRBoard(APPSESSION &app, std::int16_t iStep);
INT_PTR  VCRDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void             GetVCRStats(APPSESSION &app, std::int16_t itok, std::int32_t *pdpArmor, DV *pdv, std::int32_t *pdpShields, std::int16_t *pcsh);
void             DrawVCR(APPSESSION &app, HDC hdc, std::int16_t iStart, std::int16_t iEnd);
void             Delay(std::int16_t ctick);
void             AnimateAttack(APPSESSION &app, HDC hdc);
std::int16_t          PopupVCRMenu(APPSESSION &app, HWND hwnd, std::int16_t x, std::int16_t y, std::uint8_t brc);
void             EnableVCRButtons(APPSESSION &app);

#endif
