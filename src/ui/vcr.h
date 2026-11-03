#ifndef STARS_DECOMPILED_VCR_H
#define STARS_DECOMPILED_VCR_H

#include "win.h"
#include "battleviews.h"
#include <cstdint>
#include <optional>
#include <windows.h>

struct VCRSTATS {
    std::int32_t dpArmor;
    DV dv;
    std::int32_t dpShields;
    std::int16_t csh;
};

void             BattleVCR(APPSESSION &app, std::int16_t iBattle);
std::optional<NATIVEBATTLEDATAVIEW> BtlDataGet(APPSESSION &app, std::int16_t i);
std::int32_t          CBattleUnits(APPSESSION &app, NATIVEBATTLEDATAVIEW bdv, BattleUnitFlags grbitBU);
std::int32_t          CBattleKills(APPSESSION &app, NATIVEBATTLEDATAVIEW bdv, bool fOurDead);
std::int32_t          LdpFromItokDv(APPSESSION &app, std::int16_t itok, DV dv);
std::int16_t          SetVCRBoard(APPSESSION &app, std::int16_t iStep);
INT_PTR  VCRDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
VCRSTATS         GetVCRStats(APPSESSION &app, std::int16_t itok);
void             DrawVCR(APPSESSION &app, HDC hdc, std::int16_t iStart, std::int16_t iEnd);
void             Delay(std::int16_t ctick);
void             AnimateAttack(APPSESSION &app, HDC hdc);
std::int16_t          PopupVCRMenu(APPSESSION &app, HWND hwnd, std::int16_t x, std::int16_t y, std::uint8_t brc);
void             EnableVCRButtons(APPSESSION &app);

#endif
