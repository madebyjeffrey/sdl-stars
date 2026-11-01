#ifndef STARS_CONTROLIO_H
#define STARS_CONTROLIO_H
#include "win.h"

struct CONTROLIO {
    decltype(&win32::GetWindowTextA)      pfnGetWindowText = win32::GetWindowTextA;
    decltype(&PopupMenu)          pfnPopupMenu = PopupMenu;
    decltype(&win32::GetDlgItem)         pfnGetDlgItem = win32::GetDlgItem;
    decltype(&ui::SendMessage)     pfnSendMessage = ui::SendMessage;
    decltype(&win32::GetClientRect)      pfnGetClientRect = win32::GetClientRect;
    decltype(&win32::InvalidateRect)     pfnInvalidateRect = win32::InvalidateRect;
    decltype(&win32::GetAsyncKeyState)   pfnGetAsyncKeyState = win32::GetAsyncKeyState;
    decltype(&win32::GetKeyState)        pfnGetKeyState = win32::GetKeyState;
    decltype(&win32::MessageBeep)        pfnMessageBeep = win32::MessageBeep;
    decltype(&ui::SetWindowText)   pfnSetWindowText = ui::SetWindowText;
    decltype(&ui::SetFocus)     pfnSetFocus = ui::SetFocus;
    decltype(&ui::EnableWindow)    pfnEnableWindow = ui::EnableWindow;
    decltype(&ui::DialogBox)       pfnDialogBox = ui::DialogBox;
    decltype(&win32::EndDialog)          pfnEndDialog = win32::EndDialog;
    decltype(&FillProdSrcLB)      pfnFillProdSrcLB = FillProdSrcLB;
    decltype(&FillPlanetProdLB)   pfnFillPlanetProdLB = FillPlanetProdLB;
    decltype(&StickyDlgPos)       pfnStickyDlgPos = StickyDlgPos;
    decltype(&DrawPlanShip)       pfnDrawPlanShip = DrawPlanShip;
    decltype(&DrawProductionDlg)  pfnDrawProductionDlg = DrawProductionDlg;
    decltype(&SetMsgTitle)        pfnSetMsgTitle = SetMsgTitle;
    decltype(&RefitFrameChildren) pfnRefitFrameChildren = RefitFrameChildren;
    decltype(&ui::UpdateWindow)    pfnUpdateWindow = ui::UpdateWindow;
    decltype(&ui::DestroyWindow)   pfnDestroyWindow = ui::DestroyWindow;
    decltype(&BringUpHostDlg)     pfnBringUpHostDlg = BringUpHostDlg;
    decltype(&ui::SetWindowPos)    pfnSetWindowPos = ui::SetWindowPos;
    decltype(&UpdateOrdersDDs)    pfnUpdateOrdersDDs = UpdateOrdersDDs;
    decltype(&EnableZipProdBtns)  pfnEnableZipProdBtns = EnableZipProdBtns;
    decltype(&FillZipProdLB)      pfnFillZipProdLB = FillZipProdLB;
};
const CONTROLIO &ControlIO() noexcept;
class CONTROLIOSCOPE {
    CONTROLIO        io;
    const CONTROLIO *lpPrev;

  public:
    explicit CONTROLIOSCOPE(const CONTROLIO &io) noexcept;
    ~CONTROLIOSCOPE() noexcept;
    CONTROLIOSCOPE(const CONTROLIOSCOPE &) = delete;
    CONTROLIOSCOPE &operator=(const CONTROLIOSCOPE &) = delete;
};
#endif
