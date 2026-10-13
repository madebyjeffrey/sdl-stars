#include "win.h"

void DrawThingGauge(HDC hdc, RECT *prc, THING *lpth, int16_t md) {
    int16_t iMode;
    int16_t cSections;
    int16_t fDisabled;
    HBRUSH  rghbr[5];
    int16_t c;
    int16_t i;
    int32_t rgSize[5];
    int32_t lMax;
    int32_t l;

    fDisabled = FALSE;
    SelectObject(hdc, rghfontArial8[1]);
    cSections = 1;
    lMax = (uint32_t)(lpth->thp.wtMax * 10);
    if (md < 0 || md > 4) {
        if (md == 5) {
            for (i = 0; i < 3; i++) {
                rghbr[i] = rghbrMineral[i];
                rgSize[i] = lpth->thp.rgwtMin[i];
            }
            cSections = 3;
        }
    } else if (md == 4 || md == 3) {
        rghbr[0] = hbrButtonShadow;
        rgSize[0] = lMax;
        fDisabled = TRUE;
    } else {
        rghbr[0] = rghbrMineral[md];
        rgSize[0] = lpth->thp.rgwtMin[md];
    }
    l = LDrawGauge(hdc, prc, cSections, rgSize, rghbr, lMax);
    iMode = SetBkMode(hdc, TRANSPARENT);
    if (fDisabled) {
        l = 0;
    }
    if (cSections == 1) {
        c = wsprintf(szWork, "%ldkT", l);
    } else {
        c = wsprintf(szWork, "%ld of %ldkT", l, lMax);
    }
    l = GetTextExtent(hdc, szWork, c);
    if ((int16_t)LOWORD(l) < prc->right - prc->left - 3) {
        RcCtrTextOut(hdc, prc, szWork, c);
    }
    SetBkMode(hdc, iMode);
    return;
}
