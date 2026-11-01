#ifndef STARS_COMPAT_COMMCTRL_H
#define STARS_COMPAT_COMMCTRL_H
#include <windows.h>
namespace win32 {
inline constexpr char TOOLBARCLASSNAMEA[] = "ToolbarWindow32";
inline constexpr auto ICC_BAR_CLASSES = 0x00000004;
inline constexpr auto CCS_TOP = 0x00000001;
inline constexpr auto CCS_NORESIZE = 0x00000004;
inline constexpr auto CCS_NOPARENTALIGN = 0x00000008;
inline constexpr auto CCS_NODIVIDER = 0x00000040;
inline constexpr auto TBSTATE_CHECKED = 0x01;
inline constexpr auto TBSTATE_PRESSED = 0x02;
inline constexpr auto TBSTATE_ENABLED = 0x04;
inline constexpr auto TBSTATE_HIDDEN = 0x08;
inline constexpr auto TBSTATE_INDETERMINATE = 0x10;
inline constexpr auto TBSTATE_WRAP = 0x20;
inline constexpr auto TBSTYLE_BUTTON = 0x00;
inline constexpr auto TBSTYLE_SEP = 0x01;
inline constexpr auto TBSTYLE_CHECK = 0x02;
inline constexpr auto TBSTYLE_GROUP = 0x04;
inline constexpr auto TBSTYLE_CHECKGROUP = 0x06;
inline constexpr auto TBSTYLE_DROPDOWN = 0x08;
inline constexpr auto TBSTYLE_AUTOSIZE = 0x10;
inline constexpr auto TBSTYLE_TOOLTIPS = 0x0100;
inline constexpr auto TBSTYLE_FLAT = 0x0800;
inline constexpr auto TBSTYLE_EX_DRAWDDARROWS = 0x00000001;
inline constexpr auto BTNS_BUTTON = TBSTYLE_BUTTON;
inline constexpr auto BTNS_SEP = TBSTYLE_SEP;
inline constexpr auto BTNS_CHECK = TBSTYLE_CHECK;
inline constexpr auto BTNS_GROUP = TBSTYLE_GROUP;
inline constexpr auto BTNS_CHECKGROUP = TBSTYLE_CHECKGROUP;
inline constexpr auto BTNS_DROPDOWN = TBSTYLE_DROPDOWN;
inline constexpr auto BTNS_WHOLEDROPDOWN = 0x80;
inline constexpr auto I_IMAGENONE = (-2);
inline constexpr auto TB_ENABLEBUTTON = (win32::WM_USER+1);
inline constexpr auto TB_CHECKBUTTON = (win32::WM_USER+2);
inline constexpr auto TB_PRESSBUTTON = (win32::WM_USER+3);
inline constexpr auto TB_HIDEBUTTON = (win32::WM_USER+4);
inline constexpr auto TB_ISBUTTONENABLED = (win32::WM_USER+9);
inline constexpr auto TB_ISBUTTONCHECKED = (win32::WM_USER+10);
inline constexpr auto TB_ISBUTTONPRESSED = (win32::WM_USER+11);
inline constexpr auto TB_ISBUTTONHIDDEN = (win32::WM_USER+12);
inline constexpr auto TB_SETSTATE = (win32::WM_USER+17);
inline constexpr auto TB_GETSTATE = (win32::WM_USER+18);
inline constexpr auto TB_ADDBITMAP = (win32::WM_USER+19);
inline constexpr auto TB_ADDBUTTONSA = (win32::WM_USER+20);
inline constexpr auto TB_ADDBUTTONS = TB_ADDBUTTONSA;
inline constexpr auto TB_INSERTBUTTONA = (win32::WM_USER+21);
inline constexpr auto TB_INSERTBUTTON = TB_INSERTBUTTONA;
inline constexpr auto TB_DELETEBUTTON = (win32::WM_USER+22);
inline constexpr auto TB_GETBUTTON = (win32::WM_USER+23);
inline constexpr auto TB_BUTTONCOUNT = (win32::WM_USER+24);
inline constexpr auto TB_COMMANDTOINDEX = (win32::WM_USER+25);
inline constexpr auto TB_GETITEMRECT = (win32::WM_USER+29);
inline constexpr auto TB_BUTTONSTRUCTSIZE = (win32::WM_USER+30);
inline constexpr auto TB_SETBUTTONSIZE = (win32::WM_USER+31);
inline constexpr auto TB_SETBITMAPSIZE = (win32::WM_USER+32);
inline constexpr auto TB_AUTOSIZE = (win32::WM_USER+33);
inline constexpr auto TB_GETTOOLTIPS = (win32::WM_USER+35);
inline constexpr auto TB_GETBITMAP = (win32::WM_USER+44);
inline constexpr auto TB_GETBUTTONTEXTA = (win32::WM_USER+45);
inline constexpr auto TB_GETBUTTONTEXT = TB_GETBUTTONTEXTA;
inline constexpr auto TB_SETINDENT = (win32::WM_USER+47);
inline constexpr auto TB_GETRECT = (win32::WM_USER+51);
inline constexpr auto TB_GETBUTTONSIZE = (win32::WM_USER+58);
inline constexpr auto TB_SETMAXTEXTROWS = (win32::WM_USER+60);
inline constexpr auto TB_GETTEXTROWS = (win32::WM_USER+61);
inline constexpr auto TB_HITTEST = (win32::WM_USER+69);
inline constexpr auto TB_GETHOTITEM = (win32::WM_USER+71);
inline constexpr auto TB_SETHOTITEM = (win32::WM_USER+72);
inline constexpr auto TB_SETEXTENDEDSTYLE = (win32::WM_USER+84);
inline constexpr auto TB_GETEXTENDEDSTYLE = (win32::WM_USER+85);
inline constexpr auto TBN_FIRST = (0U-700U);
inline constexpr auto TBN_DROPDOWN = (TBN_FIRST-10);
inline constexpr auto TBDDRET_DEFAULT = 0;
inline constexpr auto TBDDRET_NODEFAULT = 1;
inline constexpr auto TBDDRET_TREATPRESSED = 2;
} // namespace win32
namespace win32 {
struct tagNMHDR { HWND hwndFrom; UINT_PTR idFrom; UINT code; };
} // namespace win32
typedef win32::tagNMHDR NMHDR, *LPNMHDR;
namespace win32 {
struct tagINITCOMMONCONTROLSEX { DWORD dwSize, dwICC; };
} // namespace win32
typedef win32::tagINITCOMMONCONTROLSEX INITCOMMONCONTROLSEX, *LPINITCOMMONCONTROLSEX;
namespace win32 {
struct _TBBUTTON {
    int iBitmap, idCommand;
    BYTE fsState, fsStyle;
#if UINTPTR_MAX > UINT32_MAX
    BYTE bReserved[6];
#else
    BYTE bReserved[2];
#endif
    DWORD_PTR dwData;
    INT_PTR iString;
};
} // namespace win32
typedef win32::_TBBUTTON TBBUTTON, *LPTBBUTTON;
typedef const TBBUTTON *LPCTBBUTTON;
namespace win32 {
struct tagTBADDBITMAP { HINSTANCE hInst; UINT_PTR nID; };
} // namespace win32
typedef win32::tagTBADDBITMAP TBADDBITMAP, *LPTBADDBITMAP;
namespace win32 {
struct tagNMTOOLBARA {
    NMHDR hdr;
    int iItem;
    TBBUTTON tbButton;
    int cchText;
    LPSTR pszText;
    RECT rcButton;
};
} // namespace win32
typedef win32::tagNMTOOLBARA NMTOOLBARA, *LPNMTOOLBARA;
typedef NMTOOLBARA NMTOOLBAR;
namespace win32 {
BOOL InitCommonControlsEx(const INITCOMMONCONTROLSEX *lpInit);
}

#endif
