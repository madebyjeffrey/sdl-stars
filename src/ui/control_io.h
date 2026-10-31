#ifndef STARS_CONTROLIO_H
#define STARS_CONTROLIO_H
#include "win.h"

struct CONTROLIO {
    decltype(&GetWindowText)      pfnGetWindowText = GetWindowText;
    decltype(&PopupMenu)          pfnPopupMenu = PopupMenu;
    decltype(&GetDlgItem)         pfnGetDlgItem = GetDlgItem;
    decltype(&LUiSendMessage)     pfnLUiSendMessage = LUiSendMessage;
    decltype(&GetClientRect)      pfnGetClientRect = GetClientRect;
    decltype(&InvalidateRect)     pfnInvalidateRect = InvalidateRect;
    decltype(&GetAsyncKeyState)   pfnGetAsyncKeyState = GetAsyncKeyState;
    decltype(&GetKeyState)        pfnGetKeyState = GetKeyState;
    decltype(&MessageBeep)        pfnMessageBeep = MessageBeep;
    decltype(&FUiSetWindowText)   pfnFUiSetWindowText = FUiSetWindowText;
    decltype(&HwndUiSetFocus)     pfnHwndUiSetFocus = HwndUiSetFocus;
    decltype(&FUiEnableWindow)    pfnFUiEnableWindow = FUiEnableWindow;
    decltype(&LUiDialogBox)       pfnLUiDialogBox = LUiDialogBox;
    decltype(&EndDialog)          pfnEndDialog = EndDialog;
    decltype(&FillProdSrcLB)      pfnFillProdSrcLB = FillProdSrcLB;
    decltype(&FillPlanetProdLB)   pfnFillPlanetProdLB = FillPlanetProdLB;
    decltype(&StickyDlgPos)       pfnStickyDlgPos = StickyDlgPos;
    decltype(&DrawPlanShip)       pfnDrawPlanShip = DrawPlanShip;
    decltype(&DrawProductionDlg)  pfnDrawProductionDlg = DrawProductionDlg;
    decltype(&SetMsgTitle)        pfnSetMsgTitle = SetMsgTitle;
    decltype(&RefitFrameChildren) pfnRefitFrameChildren = RefitFrameChildren;
    decltype(&FUiUpdateWindow)    pfnFUiUpdateWindow = FUiUpdateWindow;
    decltype(&FUiDestroyWindow)   pfnFUiDestroyWindow = FUiDestroyWindow;
    decltype(&BringUpHostDlg)     pfnBringUpHostDlg = BringUpHostDlg;
    decltype(&FUiSetWindowPos)    pfnFUiSetWindowPos = FUiSetWindowPos;
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
