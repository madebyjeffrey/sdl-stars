#include "win.h"
#include "appstate.h"
#include "callbackcontext.h"
#include "uistate.h"
#include "text_io.h"
#include "control_io.h"
#include "choice_io.h"
#include "alert_io.h"
#include "ownership.h"
#include "resourcecodec.h"
#include <stars_compat.h>

INT_PTR  RandomSeedDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());
    char     szValue[33];
    char    *pch;
    std::uint32_t dw;
    RECT     rc;

    switch (message) {
    case win32::WM_INITDIALOG:
        ControlIO().pfnSetWindowPos(hwnd, nullptr, 256, 256, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
        ui::SendDlgItemMessage(hwnd, IDC_EDIT1, win32::EM_LIMITTEXT, 0x1f, 0);
        return 1;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        if (message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            if (LOWORD(wParam) == IDOK) {
                win32::GetDlgItemTextA(hwnd, IDC_EDIT1, szValue, 32);
                pch = szValue;
                dw = 0;
                for (; std::isdigit(*pch) != 0; pch++) {
                    dw = static_cast<std::uint32_t>((dw * 10)) + static_cast<std::int16_t>((*pch - '0'));
                }
                if (dw == 0) {
                    return 1;
                }
                Randomize(app, dw);
            }
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        }
        break;
    }
    return 0;
}

void CtrTextOut(HDC hdc, std::int16_t x, std::int16_t y, const char *psz, std::int16_t cLen) {
    std::int16_t dx;

    if (cLen == 0) {
        cLen = win32::lstrlenA(psz);
    }
    dx = LOWORD(GetTextExtent(hdc, psz, cLen));
    win32::TextOutA(hdc, x - (dx >> 1), y, psz, cLen);
    return;
}

std::int16_t DxStreamTextOut(HDC hdc, std::int16_t *px, std::int16_t y, const char *psz, std::int16_t cLen, std::int16_t fPrint) {
    std::int16_t dx;

    if (cLen == 0) {
        cLen = win32::lstrlenA(psz);
    }
    dx = LOWORD(GetTextExtent(hdc, psz, cLen));
    if (fPrint) {
        win32::TextOutA(hdc, *px, y, psz, cLen);
    }
    *px += dx;
    return dx;
}

void WrapTextOut(APPSESSION &app, HDC hdc, std::int16_t *px, std::int16_t *py, const char *psz, std::int16_t cLen, std::int16_t xLeft, std::int16_t dxWidth, std::int16_t *pxMax, std::int16_t fNewLine, std::int16_t fPrint) {
    std::int16_t dxRemain;
    const char   *pchEnd;
    std::int16_t fItFit;
    std::int16_t xRight;
    std::int16_t dx;
    const char   *pchStart;
    const char   *pch;

    xRight = xLeft + dxWidth;
    dxRemain = dxWidth - (*px - xLeft);
    if (cLen == 0) {
        cLen = std::strlen(psz);
    }
    if (fNewLine) {
        *py += app.state.dyArial8;
        *px = xLeft;
    }
    pchStart = psz;
Top:
    pch = pchStart;
    pchEnd = pchStart + cLen;
    ChopTrailingSpaces(pch, &pchEnd);
    dx = LOWORD(GetTextExtent(hdc, pch, pchEnd - pch));
    fItFit = 1;
    for (; dx > dxRemain && pch < pchEnd && dx > 0; dx = LOWORD(GetTextExtent(hdc, pch, pchEnd - pch))) {
        fItFit = 0;
        ChopLastWord(pch, &pchEnd);
    }
    if (fItFit) {
        AddBackTrailingSpaces(&pchEnd, pchStart + cLen);
        dx = LOWORD(GetTextExtent(hdc, pchStart, pchEnd - pchStart));
    }
    if (pchStart == pchEnd) {
        if (*px != xLeft)
            goto WrapIt;
        pchEnd = pchStart + cLen;
        dx = LOWORD(GetTextExtent(hdc, pchStart, pchEnd - pchStart));
    }
    if (fPrint) {
        win32::TextOutA(hdc, *px, *py, pchStart, pchEnd - pchStart);
    }
    *px += dx;
    if (pxMax && *px > *pxMax) {
        *pxMax = *px;
    }
    if (pchEnd == pchStart + cLen)
        goto Done;
WrapIt:
    AddBackTrailingSpaces(&pchEnd, pchStart + cLen);
    cLen -= pchEnd - pchStart;
    pchStart = pchEnd;
    *py += app.state.dyArial8;
    *px = xLeft;
    dxRemain = dxWidth;

    goto Top;
Done:
    return;
}

void AddBackTrailingSpaces(const char **ppch, const char *pchEnd) {
    while (*ppch < pchEnd && **ppch == ' ') {
        (*ppch)++;
    }
    return;
}

void ChopLastWord(const char *pBeg, const char **ppEnd) {
    while (*ppEnd > pBeg && (*ppEnd)[-1] == ' ') {
        (*ppEnd)--;
    }
    while (*ppEnd > pBeg && (*ppEnd)[-1] != ' ') {
        (*ppEnd)--;
    }
    while (*ppEnd > pBeg && (*ppEnd)[-1] == ' ') {
        (*ppEnd)--;
    }
    return;
}

void ChopTrailingSpaces(const char *pBeg, const char **ppEnd) {
    while (*ppEnd > pBeg && (*ppEnd)[-1] == ' ') {
        (*ppEnd)--;
    }
    return;
}

void RcCtrTextOut(APPSESSION &app, HDC hdc, RECT *prc, const char *psz, std::int16_t cLen) {
    std::int16_t y;
    std::int16_t x;
    std::int32_t l;

    if (cLen == -1) {
        win32::FillRect(hdc, prc, app.lpui->hbrButtonFace);
        cLen = 0;
    }
    if (cLen == 0) {
        cLen = win32::lstrlenA(psz);
    }
    l = GetTextExtent(hdc, psz, cLen);
    x = static_cast<std::int16_t>((prc->right - prc->left - LOWORD(l))) / 2 + prc->left;
    y = static_cast<std::int16_t>((prc->bottom - prc->top - HIWORD(l))) / 2 + prc->top;
    win32::TextOutA(hdc, x, y, psz, cLen);
    return;
}

void RightTextOut(APPSESSION &app, HDC hdc, std::int16_t x, std::int16_t y, std::span<char> psz, std::int16_t cLen, std::int16_t dxErase) {
    if (cLen == -1) cLen = TextIO().pfnCchGetString(app, idsNone2, psz);
    RightTextOut(app, hdc, x, y, psz.data(), cLen, dxErase);
}

void RightTextOut(APPSESSION &app, HDC hdc, std::int16_t x, std::int16_t y, const char *psz, std::int16_t cLen, std::int16_t dxErase) {
    std::int16_t dx;
    RECT    rc;

    if (cLen == -1) {
        char szNone[sizeof(app.state.szLastStrGet)];
        cLen = TextIO().pfnCchGetString(app, idsNone2, std::span{szNone});
        RightTextOut(app, hdc, x, y, static_cast<const char *>(szNone), cLen, dxErase);
        return;
    } else if (cLen == 0) {
        cLen = win32::lstrlenA(psz);
    }
    dx = LOWORD(GetTextExtent(hdc, psz, cLen));
    if (dxErase > dx) {
        win32::SetRect(&rc, x - dxErase, y, x - dx, y + app.state.dyArial8);
        win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
    } else if (dxErase > 0 && dx > dxErase) {
        win32::SetRect(&rc, x - dxErase, y, x, y + app.state.dyArial8);
        win32::ExtTextOutA(hdc, x - dx, y, win32::ETO_OPAQUE | win32::ETO_CLIPPED, &rc, psz, cLen, nullptr);
        return;
    }
    win32::TextOutA(hdc, x - dx, y, psz, cLen);
    return;
}

void DiaganolTextOut(APPSESSION &app, HDC hdc, RECT *prc, const char *psz, std::int16_t cLen) {
    NATIVEOWNER<LOGFONT, win32::LocalFree> plfOwner;
    double   angle;
    HFONT    hfont;
    std::int16_t  dxFlat;
    std::int16_t  yStart;
    std::int16_t  dy;
    double   dcos;
    std::int16_t  dyText;
    std::int16_t  xStart;
    std::int16_t  dyEstFont;
    std::int16_t  dxText;
    LOGFONT *plf;
    double   dsin;
    HFONT    hfontSav;
    std::int16_t  dx;
    std::int16_t  dyFlat;
    double   rotate;
    std::int32_t  l;
    std::int16_t  dHtX;
    std::int16_t  dHtY;

    if (cLen == 0) {
        cLen = std::strlen(psz);
    }
    dx = prc->right - prc->left;
    dy = prc->bottom - prc->top;
    if (dx >= 10 && dy >= 10) {
        plf = static_cast<LOGFONT *>(win32::LocalAlloc(64, sizeof(LOGFONT)));
        plfOwner.reset(plf);
        plf->lfWeight = 900;
        CchCopyUtf8Prefix(std::span{plf->lfFaceName}, app.lpui->rgszArial[1]);
        plf->lfHeight = -(dx <= dy ? dy : dx);
    TryAgain:
        if (plf->lfHeight > -5)
            goto FreeLF;

        dyEstFont = win32::MulDiv(111, -plf->lfHeight, 100);
        if (dy < dyEstFont) {
            plf->lfHeight += 2 <= static_cast<std::int16_t>((dyEstFont - dy)) / 2 ? static_cast<std::int16_t>((dyEstFont - dy)) / 2 : 2;
            goto TryAgain;
        }
        angle = static_cast<double>(std::atan2(static_cast<double>(static_cast<std::int16_t>((dy - dyEstFont))), static_cast<double>(dx)));
        rotate = static_cast<double>((static_cast<double>(angle) / 3.141592654 * 1800.0 + 0.5));
        plf->lfEscapement = LOWORD(static_cast<std::int32_t>(rotate));
        hfont = win32::CreateFontIndirectA(plf);
        hfontSav = static_cast<HFONT>(win32::SelectObject(hdc, hfont));
        l = GetTextExtent(hdc, psz, cLen);
        dxText = LOWORD(l);
        dyText = HIWORD(l);
        dsin = static_cast<double>(std::sin(angle));
        dcos = static_cast<double>(std::cos(angle));
        dxFlat = LOWORD(static_cast<std::int32_t>((static_cast<double>(dcos) * dxText + static_cast<double>(dsin) * dyText)));
        dyFlat = LOWORD(static_cast<std::int32_t>((static_cast<double>(dsin) * dxText + static_cast<double>(dcos) * dyText)));
        if (dxFlat + 8 > dx || dyFlat + 8 > dy) {
            win32::SelectObject(hdc, hfontSav);
            win32::DeleteObject(hfont);
            if (dxFlat + 8 > dx) {
                dHtX = win32::MulDiv(plf->lfHeight, dx, dxFlat + 8);
                dHtX = win32::MulDiv(dHtX, 100, 111);
            } else {
                dHtX = -1000;
            }
            if (dyFlat + 8 > dy) {
                dHtY = win32::MulDiv(plf->lfHeight, dy, dyFlat + 8);
                dHtY = win32::MulDiv(dHtY, 100, 111);
            } else {
                dHtY = -1000;
            }
            plf->lfHeight += (((2) > (StarsMax(dHtX, dHtY) - plf->lfHeight)) ? (2) : (StarsMax(dHtX, dHtY) - plf->lfHeight)) + 1;
            goto TryAgain;
        }
        xStart = static_cast<std::int16_t>((dx - dxFlat)) / 2 + prc->left;
        yStart = prc->bottom - static_cast<std::int16_t>((dy - dyFlat)) / 2 - LOWORD(static_cast<std::int32_t>((static_cast<double>(dcos) * dyText)));
        win32::TextOutA(hdc, xStart, yStart, psz, cLen);
        win32::SelectObject(hdc, hfontSav);
        win32::DeleteObject(hfont);

    FreeLF:
        plfOwner.reset();
    }
    return;
}

void ExpandRc(RECT *prc, std::int16_t dx, std::int16_t dy) {
    prc->left -= dx;
    prc->right += dx;
    prc->top -= dy;
    prc->bottom += dy;
    return;
}

void OffsetRc(RECT *prc, std::int16_t dx, std::int16_t dy) {
    prc->left += dx;
    prc->right += dx;
    prc->top += dy;
    prc->bottom += dy;
    return;
}

void StickyDlgPos(HWND hwnd, POINT16 *ppt, std::int16_t fInit) {
    POINT16 ptScreenMax;
    RECT    rc;

    win32::GetWindowRect(hwnd, &rc);
    if (!fInit) {
        ppt->x = rc.left;
        ppt->y = rc.top;
    } else {
        ptScreenMax.x = win32::GetSystemMetrics(win32::SM_CXSCREEN);
        ptScreenMax.y = win32::GetSystemMetrics(win32::SM_CYSCREEN);
        if (ppt->x == -1 && ppt->y == -1) {
            rc.left = (ptScreenMax.x - (rc.right - rc.left)) >> 1;
            rc.top = (ptScreenMax.y - (rc.bottom - rc.top)) >> 1;
        } else {
            win32::OffsetRect(&rc, ppt->x - rc.left, ppt->y - rc.top);
            if (rc.right > ptScreenMax.x) {
                rc.left += ptScreenMax.x - rc.right;
            }
            if (rc.bottom > ptScreenMax.y) {
                rc.top += ptScreenMax.y - rc.bottom;
            }
            if (rc.left < 0) {
                rc.left = 0;
            }
            if (rc.top < 0) {
                rc.top = 0;
            }
        }
        ControlIO().pfnSetWindowPos(hwnd, nullptr, rc.left, rc.top, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER | win32::SWP_NOACTIVATE);
    }
    return;
}

std::int32_t LDrawGauge(APPSESSION &app, HDC hdc, RECT *prc, std::int16_t cSegs, std::int32_t *rgSize, HBRUSH *rghbr, std::int32_t cTot) {
    std::int16_t fHuge;
    std::int16_t i;
    std::int32_t lSum;
    std::int32_t dx;
    RECT    rc;

    lSum = 0;
    rc = *prc;
    win32::FrameRect(hdc, &rc, app.lpui->hbrWindowText);
    ExpandRc(&rc, -1, -1);
    if (cTot <= 0)
        goto FinRet;

    fHuge = cTot >= 10000000;
    if (fHuge) {
        cTot = static_cast<std::int32_t>((cTot / 1000));
    }
    dx = static_cast<std::int16_t>((rc.right - rc.left));
    for (i = 0; i < cSegs; i++) {
        if (!fHuge) {
            lSum += rgSize[i];
        } else {
            lSum += static_cast<std::int32_t>((rgSize[i] / 1000));
        }
        rc.right = prc->left + 1 + LOWORD(static_cast<std::int32_t>((static_cast<std::int32_t>((dx * lSum)) / cTot)));
        if (rc.right > rc.left) {
            win32::FillRect(hdc, &rc, rghbr[i]);
        }
        rc.left = rc.right;
    }

FinRet:
    rc.right = prc->right - 1;
    if (rc.right > rc.left) {
        win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
    }
    if (fHuge) {
        lSum = static_cast<std::uint32_t>((lSum * 1000));
    }
    return lSum;
}

void _Draw3dFrame(APPSESSION &app, HDC hdc, RECT *prc, std::int16_t fErase) {
    std::int16_t dy;
    std::int16_t dx;
    HBRUSH  hbrSav;
    RECT    rc;

    rc = *prc;
    dx = prc->right - prc->left;
    dy = prc->bottom - prc->top;
    hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrButtonHilite));
    if (fErase == -1 || fErase == -2) {
        win32::PatBlt(hdc, rc.left, rc.bottom, dx + 1, 1, win32::PATCOPY);
        win32::PatBlt(hdc, rc.right, rc.top, 1, dy, win32::PATCOPY);
        win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
        win32::PatBlt(hdc, rc.left, rc.top, dx, 1, win32::PATCOPY);
        win32::PatBlt(hdc, rc.left, rc.top, 1, dy, win32::PATCOPY);
        ExpandRc(&rc, -1, -1);
        win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
        dx -= 2;
        dy -= 2;
    }
    win32::PatBlt(hdc, rc.left, rc.top, dx, 1, win32::PATCOPY);
    win32::PatBlt(hdc, rc.left, rc.top, 1, dy, win32::PATCOPY);
    win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
    win32::PatBlt(hdc, rc.left, rc.bottom, dx + 1, 1, win32::PATCOPY);
    win32::PatBlt(hdc, rc.right, rc.top, 1, dy, win32::PATCOPY);
    if (fErase && fErase != -2) {
        win32::SelectObject(hdc, app.lpui->hbrButtonFace);
        win32::PatBlt(hdc, rc.left + 1, rc.top + 1, dx - 1, dy - 1, win32::PATCOPY);
    }
    win32::SelectObject(hdc, hbrSav);
    return;
}

void InitBtnTrack(BTNT *pbtnt, HWND hwnd, HDC hdc, RECT *prc, std::int16_t btf, std::int16_t dTimer, std::int16_t fInitDown, std::int16_t fNoEndRedraw, const char *szText) {
    pbtnt->hwnd = hwnd;
    pbtnt->fCreatedDC = hdc == nullptr;
    if (!hdc) {
        hdc = win32::GetDC(hwnd);
    }
    pbtnt->hdc = hdc;
    /* NATIVE: BTN rectangles can start on a two-byte boundary. */
    std::memcpy(&pbtnt->rc, prc, sizeof(pbtnt->rc));
    pbtnt->btf = btf;
    pbtnt->dTimer = dTimer;
    pbtnt->fFirst = 1;
    pbtnt->fInitDown = fInitDown;
    pbtnt->fNoEndRedraw = fNoEndRedraw;
    pbtnt->fDown = 1;
    pbtnt->szText = szText;
    return;
}

std::int16_t FTrackBtn(APPSESSION &app, BTNT *pbtnt) {
    POINT16 pt;
    std::int16_t fInBtn;
    std::int32_t ticksNew;
    RECT    rc;

    if (pbtnt->fFirst) {
        win32::SetCapture(pbtnt->hwnd);
        DrawBtn(app, pbtnt->hdc, &pbtnt->rc, pbtnt->btf, pbtnt->fDown ^ pbtnt->fInitDown, pbtnt->szText);
        pbtnt->lTicks = win32::GetTickCount() + static_cast<std::int16_t>((3 * pbtnt->dTimer));
        pbtnt->fFirst = 0;
        return 1;
    }
    pt.x = pbtnt->rc.left;
    pt.y = pbtnt->rc.top;
    std::memcpy(&rc, &pbtnt->rc, sizeof(rc));
    while (FGetMouseMove(&pt)) {
        fInBtn = win32::PtInRect(&rc, PointFrom16(pt));
        if (fInBtn != pbtnt->fDown) {
            pbtnt->fDown = fInBtn;
            DrawBtn(app, pbtnt->hdc, &pbtnt->rc, pbtnt->btf, pbtnt->fDown ^ pbtnt->fInitDown, pbtnt->szText);
        }
        ticksNew = win32::GetTickCount();
        if (ticksNew >= pbtnt->lTicks && pbtnt->fDown) {
            pbtnt->lTicks = pbtnt->dTimer + ticksNew;
            return 1;
        }
    }
    if (pbtnt->fDown && !pbtnt->fNoEndRedraw) {
        pbtnt->fDown = 0;
        DrawBtn(app, pbtnt->hdc, &pbtnt->rc, pbtnt->btf, pbtnt->fDown ^ pbtnt->fInitDown, pbtnt->szText);
        pbtnt->fDown = 1;
    }
    win32::ReleaseCapture();
    if (pbtnt->fCreatedDC) {
        win32::ReleaseDC(pbtnt->hwnd, pbtnt->hdc);
    }
    return 0;
}

void DrawBtn(APPSESSION &app, HDC hdc, RECT *prc, std::int16_t bt, std::int16_t fDown, const char *szText) {
    std::int32_t  dxFace;
    std::int16_t  ipt;
    std::int16_t  dyOffset;
    std::int16_t  d;
    std::int16_t  fBar;
    std::int16_t  fDisabled;
    std::int16_t  dxOffset;
    std::int16_t  dy;
    std::int16_t  fNoShaft;
    std::int16_t  y;
    POINT16  rgptDraw[6];
    HBRUSH   hbrCur;
    COLORREF crSav;
    std::int16_t  dx;
    HBRUSH   hbrSav;
    std::int16_t  cpt;
    std::int16_t  x;
    RECT     rc, rcButton;
    std::int16_t  dxyT;
    std::int16_t  bkMode;
    HFONT    hfontSav;

    /* NATIVE: Packed BTN/BTNT storage does not guarantee RECT alignment. */
    std::memcpy(&rcButton, prc, sizeof(rcButton));
    prc = &rcButton;
    fDisabled = bt & 4;
    rc = *prc;
    hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbrWindowFrame));
    if (!(bt & 0x40)) {
        win32::FrameRect(hdc, &rc, app.lpui->hbrWindowFrame);
        ExpandRc(&rc, -1, -1);
    }
    dx = rc.right - rc.left - 1;
    dy = rc.bottom - rc.top - 1;
    dxFace = static_cast<std::int16_t>((prc->right - prc->left)) - 5;
    win32::SelectObject(hdc, !fDown ? app.lpui->hbrButtonHilite : app.lpui->hbrButtonShadow);
    win32::PatBlt(hdc, rc.left, rc.top, dx, 1, win32::PATCOPY);
    win32::PatBlt(hdc, rc.left, rc.top, 1, dy, win32::PATCOPY);
    win32::SelectObject(hdc, !fDown ? app.lpui->hbrButtonShadow : app.lpui->hbrButtonFace);
    win32::PatBlt(hdc, rc.left, rc.bottom - 1, dx + 1, 1, win32::PATCOPY);
    win32::PatBlt(hdc, rc.right - 1, rc.top, 1, dy, win32::PATCOPY);
    if (dx < 14 || dy < 14 || (bt & 0xc0)) {
        ExpandRc(&rc, -1, -1);
    } else {
        win32::PatBlt(hdc, rc.left + 1, rc.bottom - 2, dx - 1, 1, win32::PATCOPY);
        win32::PatBlt(hdc, rc.right - 2, rc.top + 1, 1, dy - 2, win32::PATCOPY);
        win32::SetRect(&rc, rc.left + 1, rc.top + 1, rc.right - 2, rc.bottom - 2);
    }
    win32::SelectObject(hdc, app.lpui->hbrButtonFace);
    win32::FillRect(hdc, &rc, app.lpui->hbrButtonFace);
    if (!(bt & 8)) {
        fBar = bt & 0x10;
        fNoShaft = bt & 0x20;
        bt &= 3;
        if (fNoShaft) {
            cpt = 3;
            std::memcpy(rgptDraw, app.lpui->rgptTriangle, cpt * 4);
            dx = 8;
            dy = 4;
        } else {
            cpt = 5;
            std::memcpy(rgptDraw, app.lpui->rgptArrow, cpt * 4);
            dx = 6;
            dy = 6;
        }
        if (fBar) {
            for (ipt = 0; ipt < cpt; ipt++) {
                rgptDraw[ipt].y++;
            }
            dy += 2;
            rgptDraw[cpt].y = 0;
            rgptDraw[cpt].x = 0;
            cpt++;
        }
        for (ipt = 0; ipt < cpt; ipt++) {
            if (rgptDraw[ipt].x >= 0) {
                rgptDraw[ipt].x = LOWORD(static_cast<std::int32_t>((rgptDraw[ipt].x * dxFace)) / 11);
            } else {
                rgptDraw[ipt].x = rgptDraw[0].x * 2 - rgptDraw[-rgptDraw[ipt].x].x;
            }
            rgptDraw[ipt].y = LOWORD(static_cast<std::int32_t>((rgptDraw[ipt].y * dxFace)) / 11);
        }
        dx = LOWORD(static_cast<std::int32_t>((dx * dxFace)) / 11);
        dy = LOWORD(static_cast<std::int32_t>((dy * dxFace)) / 11);
        dxOffset = ((prc->right - prc->left - dx) >> 1) + fDown;
        dyOffset = ((prc->bottom - prc->top - dy) >> 1) + fDown;
        if (bt == 2 || bt == 3) {
            dxyT = dxOffset;
            dxOffset = dyOffset;
            dyOffset = dxyT;
        }
        for (ipt = 0; ipt < cpt; ipt++) {
            if (bt == 1) {
                rgptDraw[ipt].y = dy - rgptDraw[ipt].y;
            }
            if (bt == 2 || bt == 3) {
                d = dx - rgptDraw[ipt].x;
                rgptDraw[ipt].x = rgptDraw[ipt].y;
                rgptDraw[ipt].y = d;
                if (bt == 3) {
                    rgptDraw[ipt].x = dy - rgptDraw[ipt].x;
                }
            }
            rgptDraw[ipt].x += prc->left + dxOffset;
            rgptDraw[ipt].y += prc->top + dyOffset;
        }
        hbrCur = !fDisabled ? app.lpui->hbrButtonText : app.lpui->hbrButtonHilite;
        win32::SelectObject(hdc, hbrCur);
    DrawAgain:
        if (bt == 0 || bt == 1) {
            dy = bt == 0 ? -1 : 1;
            x = rgptDraw[1].x;
            dx = rgptDraw[2].x - x + 1;
            y = rgptDraw[1].y;
            if (fBar) {
                win32::PatBlt(hdc, x, rgptDraw[cpt - 1].y, dx, 1, win32::PATCOPY);
            }
            while (dx > 0) {
                win32::PatBlt(hdc, x, y, dx, 1, win32::PATCOPY);
                x++;
                dx -= 2;
                y += dy;
            }
        } else {
            dx = bt == 2 ? -1 : 1;
            y = rgptDraw[2].y;
            dy = rgptDraw[1].y - y + 1;
            x = rgptDraw[1].x;
            if (fBar) {
                win32::PatBlt(hdc, rgptDraw[cpt - 1].x, y, 1, dy, win32::PATCOPY);
            }
            while (dy > 0) {
                win32::PatBlt(hdc, x, y, 1, dy, win32::PATCOPY);
                y++;
                dy -= 2;
                x += dx;
            }
        }
        if (!fNoShaft) {
            win32::SetRect(&rc, rgptDraw[3].x >= rgptDraw[4].x ? rgptDraw[4].x : rgptDraw[3].x, rgptDraw[3].y >= rgptDraw[4].y ? rgptDraw[4].y : rgptDraw[3].y,
                    (rgptDraw[3].x <= rgptDraw[4].x ? rgptDraw[4].x : rgptDraw[3].x) + 1, (rgptDraw[3].y <= rgptDraw[4].y ? rgptDraw[4].y : rgptDraw[3].y) + 1);
            win32::FillRect(hdc, &rc, hbrCur);
        }
        if (fDisabled) {
            for (ipt = 0; ipt < cpt; ipt++) {
                rgptDraw[ipt].x--;
                rgptDraw[ipt].y--;
            }
            fDisabled = 0;
            hbrCur = app.lpui->hbrButtonShadow;
            win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
            goto DrawAgain;
        }
    }
    win32::SelectObject(hdc, hbrSav);
    if (szText) {
        hfontSav = static_cast<HFONT>(win32::SelectObject(hdc, app.lpui->rghfontArial8[1]));
        bkMode = win32::SetBkMode(hdc, win32::TRANSPARENT);
        crSav = win32::SetTextColor(hdc, !fDisabled ? app.lpui->crButtonText : app.lpui->crButtonHilite);
        rc = *prc;
        if (fDown) {
            OffsetRc(&rc, 1, 1);
        }
        RcCtrTextOut(app, hdc, &rc, szText, 0);
        if (fDisabled) {
            OffsetRc(&rc, -1, -1);
            win32::SetTextColor(hdc, app.lpui->crButtonShadow);
            RcCtrTextOut(app, hdc, &rc, szText, 0);
        }
        win32::SetTextColor(hdc, crSav);
        win32::SetBkMode(hdc, bkMode);
        win32::SelectObject(hdc, hfontSav);
    }
    return;
}

std::int16_t FGetMouseMove(POINT16 *ppt) {
    win32::MSG msg;

    while (ui::PeekMessage(&msg, nullptr, 0, 0, 1)) {
        if (msg.message == win32::WM_MOUSEMOVE || msg.message == win32::WM_LBUTTONUP) {
            ppt->x = LOWORD(msg.lParam);
            ppt->y = HIWORD(msg.lParam);
            if (msg.message != win32::WM_LBUTTONUP) {
                return 1;
            }
            return 0;
        }
    }
    return 1;
}

std::int16_t FGetRMouseMove(POINT16 *ppt) {
    win32::MSG msg;

    while (ui::PeekMessage(&msg, nullptr, 0, 0, 1)) {
        if (msg.message == win32::WM_MOUSEMOVE || msg.message == win32::WM_RBUTTONUP) {
            ppt->x = LOWORD(msg.lParam);
            ppt->y = HIWORD(msg.lParam);
            if (msg.message != win32::WM_RBUTTONUP) {
                return 1;
            }
            return 0;
        }
    }
    return 1;
}

void DrawFuzzyBorder(APPSESSION &app, HDC hdc, RECT *prc) {
    COLORREF crBack;
    std::int16_t  dy;
    std::int16_t  dx;
    HBRUSH   hbrSav;
    COLORREF crFore;

    crBack = win32::SetBkColor(hdc, app.lpui->crButtonFace);
    crFore = win32::SetTextColor(hdc, app.lpui->crWindowText);
    hbrSav = static_cast<HBRUSH>(win32::SelectObject(hdc, app.lpui->hbr50Screen));
    dx = prc->right - prc->left;
    dy = prc->bottom - prc->top;
    win32::PatBlt(hdc, prc->left, prc->top, dx, 2, win32::PATINVERT);
    win32::PatBlt(hdc, prc->left, prc->bottom - 2, dx, 2, win32::PATINVERT);
    win32::PatBlt(hdc, prc->left, prc->top + 2, 2, dy - 4, win32::PATINVERT);
    win32::PatBlt(hdc, prc->right - 2, prc->top + 2, 2, dy - 4, win32::PATINVERT);
    win32::SelectObject(hdc, hbrSav);
    win32::SetTextColor(hdc, crFore);
    win32::SetBkColor(hdc, crBack);
    return;
}

std::int16_t FStringFitsScreen(APPSESSION &app, char *lpsz, std::int16_t dxMax) {
    HDC     hdc;
    std::int16_t c;
    std::int16_t fFit;
    HFONT   hfontSav;

    fFit = 1;
    hdc = win32::GetDC(app.lpui->hwndFrame);
    c = std::strlen(lpsz);
    hfontSav = static_cast<HFONT>(win32::SelectObject(hdc, app.lpui->rghfontArial8[0]));
    while (c > 0 && LOWORD(GetTextExtent(hdc, lpsz, c)) > static_cast<std::uint16_t>(dxMax)) {
        fFit = 0;
        c = static_cast<std::int16_t>(CchUtf8Prefix(lpsz, c - 1));
        lpsz[c] = 0;
    }
    win32::SelectObject(hdc, hfontSav);
    win32::ReleaseDC(app.lpui->hwndFrame, hdc);
    return fFit;
}

HBRUSH HbrGet(APPSESSION &app, COLORREF cr) {
    std::int16_t iFree;
    std::int16_t i;
    HBRUSH  hbr;

    iFree = -1;
    for (i = 0; i < app.lpui->chbrCache; i++) {
        if (app.lpui->rghbrCacheUse[i] == 0) {
            iFree = i;
        } else if (app.lpui->rgcrCache[i] == cr) {
            app.lpui->rghbrCacheUse[i]++;
            return app.lpui->rghbrCache[i];
        }
    }
    hbr = win32::CreateSolidBrush(cr);
    if (!hbr) {
        return nullptr;
    }
    if (iFree == -1) {
        if (app.lpui->chbrCache >= 32) {
            return hbr;
        }
        iFree = app.lpui->chbrCache++;
    }
    app.lpui->rghbrCacheUse[iFree] = 1;
    app.lpui->rghbrCache[iFree] = hbr;
    app.lpui->rgcrCache[iFree] = cr;
    return hbr;
}

void FreeHbr(APPSESSION &app, HBRUSH hbr) {
    std::int16_t i;

    for (i = 0; i < app.lpui->chbrCache; i++) {
        if (app.lpui->rghbrCacheUse[i] > 0 && hbr == app.lpui->rghbrCache[i]) {
            app.lpui->rghbrCacheUse[i]--;
            if (app.lpui->rghbrCacheUse[i] != 0) {
                return;
            }
        DeleteBrush:
            win32::DeleteObject(hbr);
            return;
        }
    }
    goto DeleteBrush;
}

std::uint16_t DibNumColors(void *pv) {
    std::int16_t           bits;
    BITMAPCOREHEADER *lpbc;
    BITMAPINFOHEADER *lpbi;
    BITMAPINFOHEADER bi;
    BITMAPCOREHEADER bc;

    lpbi = &bi;
    lpbc = &bc;
    if (RawLoad32(pv) != 12) {
        std::memcpy(lpbi, pv, sizeof(bi));
        if (lpbi->biClrUsed != 0) {
            return LOWORD(lpbi->biClrUsed);
        }
        bits = lpbi->biBitCount;
    } else {
        std::memcpy(lpbc, pv, sizeof(bc));
        bits = lpbc->bcBitCount;
    }
    switch (bits) {
    case 1:
        return 2;
    case 4:
        return 16;
    case 8:
        return 0x100;
    default:
        return 0;
    }
}

HPALETTE HpalFromDib(HGLOBAL hdib) {
    NATIVEOWNER<LOGPALETTE, win32::LocalFree> ppalOwner;
    HPALETTE          hpal;
    std::int16_t           cColors;
    std::int16_t           i;
    std::uint8_t           bT;
    char             *lpb;
    BITMAPINFOHEADER *lpbi;
    LOGPALETTE       *ppal;

    lpb = static_cast<char *>(win32::LockResource(hdib));
    if (!lpb) {
        return nullptr;
    }
    lpbi = reinterpret_cast<BITMAPINFOHEADER *>(lpb);
    cColors = DibNumColors(lpbi);
    if (cColors > 256) {
        win32::GlobalUnlock(hdib);
        return nullptr;
    }
    ppal = static_cast<LOGPALETTE *>(win32::LocalAlloc(64, cColors * 4 + 8));
    ppalOwner.reset(ppal);
    ppal->palNumEntries = cColors;
    ppal->palVersion = 768;
    std::memcpy(ppal->palPalEntry, lpb + 40, cColors * 4);
    for (i = 0; i < cColors; i++) {
        bT = ppal->palPalEntry[i].peRed;
        ppal->palPalEntry[i].peRed = ppal->palPalEntry[i].peBlue;
        ppal->palPalEntry[i].peBlue = bT;
    }
    hpal = win32::CreatePalette(ppal);
    ppalOwner.reset();
    win32::GlobalUnlock(hdib);
    return hpal;
}

HPALETTE HpalBlackReserved() {
    NATIVEOWNER<LOGPALETTE, win32::LocalFree> ppalOwner;
    HPALETTE    hpal;
    std::int16_t     cColors;
    std::int16_t     i;
    LOGPALETTE *ppal;

    cColors = 256;
    ppal = static_cast<LOGPALETTE *>(win32::LocalAlloc(64, cColors * 4 + 8));
    ppalOwner.reset(ppal);
    ppal->palNumEntries = cColors;
    ppal->palVersion = 768;
    for (i = 0; i < cColors; i++) {
        ppal->palPalEntry[i].peBlue = 0;
        ppal->palPalEntry[i].peGreen = 0;
        ppal->palPalEntry[i].peRed = 0;
        ppal->palPalEntry[i].peFlags = 1;
    }
    hpal = win32::CreatePalette(ppal);
    ppalOwner.reset();
    return hpal;
}

std::uint16_t PaletteSize(void *pv) {
    std::uint16_t          NumColors;
    BITMAPINFOHEADER *lpbi;

    lpbi = static_cast<BITMAPINFOHEADER *>(pv);
    NumColors = DibNumColors(lpbi);
    if (RawLoad32(lpbi) == 12) {
        return 3 * NumColors;
    }
    return NumColors * 4;
}

std::int16_t DibBlt(HDC hdc, std::int16_t x0, std::int16_t y0, std::int16_t dx, std::int16_t dy, HGLOBAL hdib, std::int16_t x1, std::int16_t y1, std::int16_t dxSrc, std::int16_t dySrc, std::int32_t rop) {
    char             *pBuf;
    BITMAPINFOHEADER *lpbi;

    if (!hdib) {
        return win32::PatBlt(hdc, x0, y0, dx, dy, rop);
    }
    lpbi = static_cast<BITMAPINFOHEADER *>(win32::GlobalLock(hdib));
    if (!lpbi) {
        return 0;
    }
    pBuf = reinterpret_cast<char *>(lpbi) + LOWORD(RawLoad32(lpbi)) + PaletteSize(lpbi);
    win32::StretchDIBits(hdc, x0, y0, dx, dy, x1, y1, dxSrc, dySrc, pBuf, reinterpret_cast<LPBITMAPINFO>(lpbi), 0, rop);
    win32::GlobalUnlock(hdib);
    return 1;
}

HGLOBAL DibFromBitmap(HBITMAP hbm, std::uint32_t biStyle, std::uint16_t biBits, HPALETTE hpal) {
    HDC               hdc = nullptr;
    HGLOBAL           h;
    std::uint32_t          dwLen;
    BITMAP            bm;
    BITMAPINFOHEADER  bi;
    HGLOBAL           hdib;
    BITMAPINFOHEADER *lpbi;
    NATIVECLEANUPSCOPE dc([&]() noexcept {
        if (hdc) {
            win32::SelectPalette(hdc, hpal, 0);
            win32::ReleaseDC(nullptr, hdc);
        }
    });
    NATIVEOWNER<void, win32::GlobalFree> dib;
    bool fLocked = false;
    NATIVECLEANUPSCOPE unlock([&]() noexcept { if (fLocked) win32::GlobalUnlock(hdib); });


    if (!hbm) {
        return nullptr;
    }
    if (!hpal) {
        hpal = static_cast<HPALETTE>(win32::GetStockObject(win32::DEFAULT_PALETTE));
    }
    win32::GetObjectA(hbm, sizeof(bm), &bm);
    if (biBits == 0) {
        biBits = bm.bmPlanes * bm.bmBitsPixel;
    }
    bi.biSize = 40;
    bi.biWidth = bm.bmWidth;
    bi.biHeight = bm.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = biBits;
    bi.biCompression = biStyle;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    dwLen = static_cast<std::uint32_t>(PaletteSize(&bi)) + bi.biSize;
    hdc = win32::GetDC(nullptr);
    hpal = win32::SelectPalette(hdc, hpal, 0);
    win32::RealizePalette(hdc);
    hdib = win32::GlobalAlloc(66, dwLen);
    dib.reset(hdib);
    if (!hdib) {
        return nullptr;
    }
    lpbi = static_cast<BITMAPINFOHEADER *>(win32::GlobalLock(hdib)); fLocked = lpbi != nullptr;
    *lpbi = bi;
    win32::GetDIBits(hdc, hbm, 0, LOWORD(bi.biHeight), nullptr, reinterpret_cast<BITMAPINFO *>(lpbi), 0);
    bi = *lpbi;
    win32::GlobalUnlock(hdib); fLocked = false;
    if (bi.biSizeImage == 0) {
        bi.biSizeImage = static_cast<std::uint32_t>((static_cast<std::int32_t>((static_cast<std::uint32_t>(((static_cast<std::uint32_t>((bm.bmWidth * static_cast<std::uint32_t>(biBits))) + 31) / 32)) * 4)) * bm.bmHeight));
        if (biStyle != 0) {
            bi.biSizeImage = static_cast<std::uint32_t>((static_cast<std::uint32_t>((bi.biSizeImage * 3)) / 2));
        }
    }
    dwLen = static_cast<std::uint32_t>(PaletteSize(&bi)) + bi.biSize + bi.biSizeImage;
    h = win32::GlobalReAlloc(hdib, dwLen, 0);
    if (h) {
        dib.release();
        hdib = h;
        dib.reset(hdib);
        lpbi = static_cast<BITMAPINFOHEADER *>(win32::GlobalLock(hdib)); fLocked = lpbi != nullptr;
        if (win32::GetDIBits(hdc, hbm, 0, LOWORD(bi.biHeight), reinterpret_cast<std::uint8_t *>(lpbi) + LOWORD(lpbi->biSize) + PaletteSize(lpbi), reinterpret_cast<BITMAPINFO *>(lpbi), 0) == 0) {
            win32::GlobalUnlock(hdib); fLocked = false;
            return nullptr;
        }
        bi = *lpbi;
        win32::GlobalUnlock(hdib); fLocked = false;
        return dib.release();
    }
    return nullptr;
}

HGLOBAL HdibLoadBigResource(APPSESSION &app, BitmapId idb) {
    HRSRC    hrsrc;
    HGLOBAL  hres;
    char    *lpstr;
    HGLOBAL  hdib;
    std::uint32_t cb;
    std::uint32_t cbHeader;
    BITMAPINFOHEADER bi;
    BITMAPCOREHEADER bc;

    hrsrc = win32::FindResourceA(app.lpui->hInst, win32::MAKEINTRESOURCE(idb), win32::MAKEINTRESOURCE(2));
    if (!hrsrc) {
        return nullptr;
    }
    hres = win32::LoadResource(app.lpui->hInst, hrsrc);
    if (!hres) {
        return nullptr;
    }
    lpstr = static_cast<char *>(win32::LockResource(hres));
    if (!lpstr) {
        return nullptr;
    }
    /* A writable copy: the original read the resource into its own block. */
    cb = win32::SizeofResource(app.lpui->hInst, hrsrc);
    hdib = win32::GlobalAlloc(win32::GMEM_FIXED, cb);
    if (!hdib) {
        return nullptr;
    }
    std::memcpy(static_cast<char *>(hdib), lpstr, cb);
    // Embedded bytes become actual native GDI headers in the writable copy.
    // Incomplete/unknown resource prefixes retain their prior copied bytes.
    const auto rgb = std::as_bytes(std::span(lpstr, cb));
    if (NATIVEBYTEREADER(rgb).FReadU32Le(cbHeader)) {
        if (cbHeader == cbWireDibCoreHeader) {
            if (FDecodeDibCoreHeader(rgb, bc)) std::memcpy(hdib, &bc, sizeof(bc));
        } else if (FDecodeDibHeader(rgb, bi)) {
            std::memcpy(hdib, &bi, sizeof(bi));
        }
    }
    return hdib;
}

INT_PTR  PasswordDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    char    szPass[60];
    RECT    rc;
    std::int32_t lSalt;

    switch (message) {
    case win32::WM_INITDIALOG:
        ui::SendDlgItemMessage(hwnd, IDC_EDIT1, win32::EM_LIMITTEXT, 0xf, 0);
        ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_PASSWORD_STATUS_TEXT), TextIO().pfnCompressed(app, idsEnterPassword));
        return 1;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        if (message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            if (LOWORD(wParam) == IDOK) {
                if (CchGetUtf8WindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_EDIT1), szPass, 60) < 0) {
                    AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsNativePasswordReenter), win32::MB_ICONHAND);
                    break;
                }
                lSalt = LSaltFromSz(szPass);
                if (lSalt == app.state.lSaltCur) {
                    if (!app.state.gd.fHotSeat) {
                        app.state.lSaltLast = app.state.lSaltCur;
                        CchCopyUtf8Prefix(std::span{app.state.szPassLast}, szPass);
                    }
                } else {
                    app.lpui->vcPasswordFailures++;
                    Delay(app.lpui->vcPasswordFailures < 10 ? 1000 : app.lpui->vcPasswordFailures < 100 ? 5000 : 10000);
                    AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsPasswordHaveEnteredIncorrectPleaseTry, nullptr), win32::MB_ICONHAND);
                    ControlIO().pfnSetFocus(ControlIO().pfnGetDlgItem(hwnd, IDC_EDIT1));
                    ui::SendDlgItemMessage(hwnd, IDC_EDIT1, win32::EM_SETSEL, 0, -1);
                    break;
                }
            }
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, 1089);
            return 1;
        }
    }
    return 0;
}

INT_PTR  NewPasswordDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    char    szPass[20];
    char    szPassConfirm[20];
    RECT    rc;
    std::int32_t lSalt2;
    std::int32_t lSalt;

    switch (message) {
    case win32::WM_INITDIALOG:
        ui::SendDlgItemMessage(hwnd, IDC_EDIT1, win32::EM_LIMITTEXT, 0xf, 0);
        ui::SendDlgItemMessage(hwnd, IDC_PASSWORD_CONFIRM, win32::EM_LIMITTEXT, 0xf, 0);
        win32::SetWindowLongPtrA(hwnd, win32::DWLP_USER, lParam);
        if (lParam != 0) {
            ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_PASSWORD_STATUS_TEXT), TextIO().pfnCompressed(app, idsNativeLegacyRacePassword));
            ControlIO().pfnSetWindowText(hwnd, TextIO().pfnCompressed(app, idsNativeLegacyRacePassword));
        } else if (app.state.idPlayer == iplrNone) {
            ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_PASSWORD_STATUS_TEXT), TextIO().pfnCompressed(app, idsNotePasswordEffectiveImmediately));
            ControlIO().pfnSetWindowText(hwnd, TextIO().pfnCompressed(app, idsChangeHostPassword));
        } else {
            ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_PASSWORD_STATUS_TEXT), TextIO().pfnCompressed(app, idsNoteNewPasswordWillTakeEffectUntil));
        }
        return 1;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        if (message == win32::WM_CTLCOLORSTATIC) {
            win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
            return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
        }
        break;
    case win32::WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            if (LOWORD(wParam) == IDOK) {
                if (CchGetUtf8WindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_EDIT1), szPass, 18) < 0) {
                    AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsNativePasswordReenter), win32::MB_ICONHAND);
                    break;
                }
                lSalt = LSaltFromSz(szPass);
                if (CchGetUtf8WindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_PASSWORD_CONFIRM), szPassConfirm, 18) < 0) {
                    AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsNativePasswordReenter), win32::MB_ICONHAND);
                    break;
                }
                lSalt2 = LSaltFromSz(szPassConfirm);
                if (lSalt != lSalt2) {
                    AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsPasswordsTypedTwoFieldsSamePleaseReenter, nullptr), win32::MB_ICONHAND);
                    ControlIO().pfnSetFocus(ControlIO().pfnGetDlgItem(hwnd, IDC_EDIT1));
                    ui::SendDlgItemMessage(hwnd, IDC_EDIT1, win32::EM_SETSEL, 0, -1);
                    break;
                }
                if (win32::GetWindowLongPtrA(hwnd, win32::DWLP_USER) != 0) {
                    if (szPass[0] == 0) {
                        AlertIO().pfnAlertSz(app, TextIO().pfnCompressed(app, idsNativeLegacyRacePasswordRequired), win32::MB_ICONHAND);
                        break;
                    }
                    std::strcpy(reinterpret_cast<char *>(win32::GetWindowLongPtrA(hwnd, win32::DWLP_USER)), szPass);
                } else if (app.state.idPlayer != iplrNone) {
                    WriteMemPassword(app, lSalt);
                } else {
                    app.state.lSaltCur = lSalt;
                    if (FWriteDataFile(app, app.state.szBase, app.state.idPlayer, 0)) {
                        app.state.lSaltLast = lSalt;
                    } else {
                        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableCreateHostFile, nullptr), win32::MB_ICONHAND);
                        app.state.lSaltCur = app.state.lSaltLast;
                    }
                }
            }
            ControlIO().pfnEndDialog(hwnd, LOWORD(wParam) == IDOK);
            return 1;
        case IDC_HELP:
            win32::WinHelpA(hwnd, app.lpui->szHelpFile, win32::HELP_CONTEXT, idhChangePassword);
            return 1;
        }
    }
    return 0;
}

std::int16_t FPromptLegacyRacePassword(APPSESSION &app, char *szPassword) {
    std::int16_t fRet;

    fRet = static_cast<std::int16_t>(ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_NEW_PASSWORD), app.lpui->hwndFrame, NewPasswordDlg, reinterpret_cast<LPARAM>(szPassword)));
    return fRet > 0;
}

void ShowProgressGauge(APPSESSION &app) {
    if (!app.lpui->hwndProgressGauge) {
        ui::CreateDialog(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_Gauge), app.lpui->hwndFrame, app.lpui->lpfnGaugeDlgProc);
    } else {
        UpdateProgressGauge(app, static_cast<ProgressStep>(0));
    }
    return;
}

void HideProgressGauge(APPSESSION &app) {
    if (app.lpui->hwndProgressGauge) {
        ControlIO().pfnDestroyWindow(app.lpui->hwndProgressGauge);
        app.lpui->hwndProgressGauge = nullptr;
    }
    return;
}

INT_PTR  ProgressGaugeDlg(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    APPSESSION &app = *static_cast<APPSESSION *>(PvCompatCallbackContext());

    HDC         hdc;
    PAINTSTRUCT ps;
    RECT        rc;
    std::int16_t     dy;
    char       *psz;
    std::int16_t     dx;

    switch (message) {
    case win32::WM_INITDIALOG:
        app.lpui->vpctProgressGauge = 0;
        app.lpui->hwndProgressGauge = hwnd;
        dx = win32::GetSystemMetrics(win32::SM_CXSCREEN);
        dy = win32::GetSystemMetrics(win32::SM_CYSCREEN);
        psz = TextIO().pfnCompressed(app, idsGeneratingDataYearD);
        win32::wsprintfA(app.state.szWork, psz, app.state.game.turn + 2401);
        ControlIO().pfnSetWindowText(ControlIO().pfnGetDlgItem(hwnd, IDC_GAUGE_TEXT), app.state.szWork);
        win32::GetWindowRect(hwnd, &rc);
        rc.left = (dx - (rc.right - rc.left)) >> 1;
        rc.top = (dy - (rc.bottom - rc.top)) >> 1;
        ControlIO().pfnSetWindowPos(hwnd, nullptr, rc.left, rc.top, 0, 0, win32::SWP_NOSIZE | win32::SWP_NOZORDER);
        return 1;
    case win32::WM_ERASEBKGND:
        ControlIO().pfnGetClientRect(hwnd, &rc);
        win32::FillRect(reinterpret_cast<HDC>(wParam), &rc, app.lpui->hbrButtonFace);
        return 1;
    case win32::WM_CTLCOLORMSGBOX:
    case win32::WM_CTLCOLOREDIT:
    case win32::WM_CTLCOLORLISTBOX:
    case win32::WM_CTLCOLORBTN:
    case win32::WM_CTLCOLORDLG:
    case win32::WM_CTLCOLORSCROLLBAR:
    case win32::WM_CTLCOLORSTATIC:
        win32::SetBkColor(reinterpret_cast<HDC>(wParam), app.lpui->crButtonFace);
        return reinterpret_cast<INT_PTR>(app.lpui->hbrButtonFace);
    case win32::WM_PAINT:
        hdc = win32::BeginPaint(hwnd, &ps);
        DrawProgressGauge(app, hdc, 1, 0);
        win32::EndPaint(hwnd, &ps);
        return 1;
    }
    return 0;
}

void DrawProgressGauge(APPSESSION &app, HDC hdcOrig, std::int16_t fFull, std::int16_t iNumOnly) {
    HDC     hdc;
    std::int16_t dy;
    std::int16_t fNumOnly;
    std::int16_t dx2;
    std::int16_t dx;
    RECT    rc;
    std::int16_t c;
    char    szT[8];

    fNumOnly = iNumOnly > 0;
    if (app.lpui->hwndProgressGauge) {
        if (!hdcOrig) {
            hdc = win32::GetDC(app.lpui->hwndProgressGauge);
        } else {
            hdc = hdcOrig;
        }
        ControlIO().pfnGetClientRect(app.lpui->hwndProgressGauge, &rc);
        rc.top = rc.bottom >> 1;
        win32::InflateRect(&rc, -8, -8);
        dx = rc.right - rc.left;
        dy = rc.bottom - rc.top;
        if (fFull) {
            win32::SelectObject(hdc, app.lpui->hbrButtonShadow);
            win32::PatBlt(hdc, rc.left - 1, rc.top - 1, dx + 2, 1, win32::PATCOPY);
            win32::PatBlt(hdc, rc.left - 1, rc.top - 1, 1, dy + 2, win32::PATCOPY);
            win32::SelectObject(hdc, app.lpui->hbrButtonHilite);
            win32::PatBlt(hdc, rc.left, rc.bottom, dx + 1, 1, win32::PATCOPY);
            win32::PatBlt(hdc, rc.right, rc.top, 1, dy, win32::PATCOPY);
        }
        win32::InflateRect(&rc, -2, -2);
        dx -= 4;
        dy -= 4;
        if (app.state.gd.fProgressTxt) {
            win32::SetBkColor(hdc, app.lpui->crButtonFace);
            win32::SelectObject(hdc, app.lpui->rghfontArial7[0]);
            if (iNumOnly <= 0) {
                iNumOnly = app.lpui->vpctProgressGauge;
            }
            c = win32::wsprintfA(szT, app.state.PCTD, iNumOnly);
            RightTextOut(app, hdc, rc.right - 2, 1, szT, c, 80);
        }
        if (fNumOnly)
            goto LRelease;

        win32::SelectObject(hdc, app.lpui->hbrBlue);
        for (dx = win32::MulDiv(dx, app.lpui->vpctProgressGauge, 1000); dx > 0; dx -= dy) {
            if (dx >= dy) {
                dx2 = dy - 1;
            } else {
                dx2 = dx;
            }
            win32::PatBlt(hdc, rc.left, rc.top, dx2, dy, win32::PATCOPY);
            rc.left += dy;
        }

    LRelease:
        if (!hdcOrig) {
            win32::ReleaseDC(app.lpui->hwndProgressGauge, hdc);
        }
    }
    return;
}

HFONT HfontPrinterCreate(APPSESSION &app, HDC hdc, std::int16_t iSize, std::int16_t *pdyFont) {
    NATIVEOWNER<LOGFONT, win32::LocalFree> plfOwner;
    HFONT      hfontNew;
    LOGFONT   *plf;
    TEXTMETRIC tm;
    HFONT      hfontSav;

    plf = static_cast<LOGFONT *>(win32::LocalAlloc(64, sizeof(LOGFONT)));
    plfOwner.reset(plf);
    *plf = {};
    plf->lfHeight = -win32::MulDiv(iSize, win32::GetDeviceCaps(hdc, win32::LOGPIXELSY), 72);
    CchCopyUtf8Prefix(std::span{plf->lfFaceName}, app.lpui->rgszArial[1]);
    hfontNew = win32::CreateFontIndirectA(plf);
    if (pdyFont && hfontNew) {
        hfontSav = static_cast<HFONT>(win32::SelectObject(hdc, hfontNew));
        win32::GetTextMetricsA(hdc, &tm);
        *pdyFont = tm.tmHeight + tm.tmExternalLeading;
        win32::SelectObject(hdc, hfontSav);
    }
    plfOwner.reset();
    return hfontNew;
}

// IdAlertBox shows an AlertSz message in a message box and returns the
// button the player chose.
std::int16_t IdAlertBox(const char *sz, std::int16_t mbType) { return CompatChoiceIO().pfnMessageBoxA(win32::GetFocus(), sz, "Stars!", mbType); }

// PromptPassword asks for the password of the file FCheckPassword is
// checking.
std::int16_t PromptPassword(APPSESSION &app) {
    std::int16_t fRet;

    fRet = ControlIO().pfnDialogBox(app.lpui->hInst, win32::MAKEINTRESOURCE(IDD_PASSWORD), !app.lpui->hwndTitle ? app.lpui->hwndFrame : app.lpui->hwndTitle, PasswordDlg, 0);
    return fRet;
}

void UpdateProgressGauge(APPSESSION &app, ProgressStep pctX10) {
    std::int16_t iNum;

    if (app.lpui->hwndProgressGauge) {
        iNum = 0;
        if (pctX10 == progressStep4) {
            pctX10 = static_cast<ProgressStep>(app.lpui->vpctProgressGauge + 4);
        } else if (pctX10 == progressStep1) {
            pctX10 = static_cast<ProgressStep>(app.lpui->vpctProgressGauge + 1);
        } else if (pctX10 < 0) {
            pctX10 = static_cast<ProgressStep>(0);
        } else if (pctX10 > 1000) {
            if (!app.state.gd.fProgressTxt) {
                return;
            }
            iNum = pctX10;
            pctX10 = static_cast<ProgressStep>(app.lpui->vpctProgressGauge);
        }
        app.lpui->vpctProgressGauge = pctX10;
        DrawProgressGauge(app, nullptr, 0, iNum);
    }
    return;
}
