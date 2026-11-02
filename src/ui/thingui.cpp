#include "win.h"
#include "appstate.h"
#include "uistate.h"

void DrawThingGauge(APPSESSION &app, HDC hdc, RECT *prc, THING *lpth, std::int16_t md) {
    std::int16_t iMode;
    std::int16_t cSections;
    std::int16_t fDisabled;
    HBRUSH  rghbr[5];
    std::int16_t c;
    std::int16_t i;
    std::int32_t rgSize[5];
    std::int32_t lMax;
    std::int32_t l;

    fDisabled = 0;
    win32::SelectObject(hdc, app.lpui->rghfontArial8[1]);
    cSections = 1;
    lMax = static_cast<std::uint32_t>((lpth->thp().wtMax() * 10));
    if (md < 0 || md > 4) {
        if (md == 5) {
            for (i = 0; i < 3; i++) {
                rghbr[i] = app.lpui->rghbrMineral[i];
                rgSize[i] = lpth->thp().rgwtMin(i);
            }
            cSections = 3;
        }
    } else if (md == 4 || md == 3) {
        rghbr[0] = app.lpui->hbrButtonShadow;
        rgSize[0] = lMax;
        fDisabled = 1;
    } else {
        rghbr[0] = app.lpui->rghbrMineral[md];
        rgSize[0] = lpth->thp().rgwtMin(md);
    }
    l = LDrawGauge(app, hdc, prc, cSections, rgSize, rghbr, lMax);
    iMode = win32::SetBkMode(hdc, win32::TRANSPARENT);
    if (fDisabled) {
        l = 0;
    }
    if (cSections == 1) {
        c = CchSprintf(app.state.szWork, "%ldkT", l);
    } else {
        c = CchSprintf(app.state.szWork, "%ld of %ldkT", l, lMax);
    }
    l = GetTextExtent(hdc, app.state.szWork, c);
    if (static_cast<std::int16_t>(LOWORD(l)) < prc->right - prc->left - 3) {
        RcCtrTextOut(app, hdc, prc, app.state.szWork, c);
    }
    win32::SetBkMode(hdc, iMode);
    return;
}
