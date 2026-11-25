/* Windows declarations adapted from mingw-w64; see ../LICENSE.
 * Modified for the Stars Linux source compatibility layer, October 10, 2026.
 */
#ifndef STARS_WINCOMPAT_STRUCTS_H
#define STARS_WINCOMPAT_STRUCTS_H

#include "wincompat_types.h"
#pragma pack(push, 8)
namespace win32 {
struct tagWINDOWPOS {
    HWND hwnd;
    HWND hwndInsertAfter;
    int x, y, cx, cy;
    UINT flags;
};
} // namespace win32
using WINDOWPOS = win32::tagWINDOWPOS;
using LPWINDOWPOS = win32::tagWINDOWPOS*;
using PWINDOWPOS = win32::tagWINDOWPOS*;

namespace win32 {
struct tagPOINT {
  LONG x;
  LONG y;
};
} // namespace win32
using POINT = win32::tagPOINT;
using PPOINT = win32::tagPOINT*;
using NPPOINT = win32::tagPOINT*;
using LPPOINT = win32::tagPOINT*;

namespace win32 {
struct tagSIZE {
  LONG cx;
  LONG cy;
};
} // namespace win32
using SIZE = win32::tagSIZE;
using PSIZE = win32::tagSIZE*;
using LPSIZE = win32::tagSIZE*;

namespace win32 {
struct tagWNDCLASSA {
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCSTR lpszMenuName;
    LPCSTR lpszClassName;
  };
} // namespace win32
using WNDCLASSA = win32::tagWNDCLASSA;
using PWNDCLASSA = win32::tagWNDCLASSA*;
using NPWNDCLASSA = win32::tagWNDCLASSA*;
using LPWNDCLASSA = win32::tagWNDCLASSA*;

namespace win32 {
struct tagMSG {
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    POINT pt;
  };
} // namespace win32
namespace win32 { using MSG = tagMSG; }
using PMSG = win32::tagMSG*;
using NPMSG = win32::tagMSG*;
using LPMSG = win32::tagMSG*;

namespace win32 {
struct tagPAINTSTRUCT {
    HDC hdc;
    WINBOOL fErase;
    RECT rcPaint;
    WINBOOL fRestore;
    WINBOOL fIncUpdate;
    BYTE rgbReserved[32];
  };
} // namespace win32
using PAINTSTRUCT = win32::tagPAINTSTRUCT;
using PPAINTSTRUCT = win32::tagPAINTSTRUCT*;
using NPPAINTSTRUCT = win32::tagPAINTSTRUCT*;
using LPPAINTSTRUCT = win32::tagPAINTSTRUCT*;

namespace win32 {
struct tagCREATESTRUCTA {
    LPVOID lpCreateParams;
    HINSTANCE hInstance;
    HMENU hMenu;
    HWND hwndParent;
    int cy;
    int cx;
    int y;
    int x;
    LONG style;
    LPCSTR lpszName;
    LPCSTR lpszClass;
    DWORD dwExStyle;
  };
} // namespace win32
using CREATESTRUCTA = win32::tagCREATESTRUCTA;
using LPCREATESTRUCTA = win32::tagCREATESTRUCTA*;

namespace win32 {
struct tagWINDOWPLACEMENT {
    UINT length;
    UINT flags;
    UINT showCmd;
    POINT ptMinPosition;
    POINT ptMaxPosition;
    RECT rcNormalPosition;
  };
} // namespace win32
using WINDOWPLACEMENT = win32::tagWINDOWPLACEMENT;

namespace win32 {
struct tagMINMAXINFO {
    POINT ptReserved;
    POINT ptMaxSize;
    POINT ptMaxPosition;
    POINT ptMinTrackSize;
    POINT ptMaxTrackSize;
  };
} // namespace win32
using MINMAXINFO = win32::tagMINMAXINFO;
using PMINMAXINFO = win32::tagMINMAXINFO*;
using LPMINMAXINFO = win32::tagMINMAXINFO*;

namespace win32 {
struct tagMEASUREITEMSTRUCT {
    UINT CtlType;
    UINT CtlID;
    UINT itemID;
    UINT itemWidth;
    UINT itemHeight;
    ULONG_PTR itemData;
  };
} // namespace win32
using MEASUREITEMSTRUCT = win32::tagMEASUREITEMSTRUCT;
using PMEASUREITEMSTRUCT = win32::tagMEASUREITEMSTRUCT*;
using LPMEASUREITEMSTRUCT = win32::tagMEASUREITEMSTRUCT*;

namespace win32 {
struct tagDRAWITEMSTRUCT {
    UINT CtlType;
    UINT CtlID;
    UINT itemID;
    UINT itemAction;
    UINT itemState;
    HWND hwndItem;
    HDC hDC;
    RECT rcItem;
    ULONG_PTR itemData;
  };
} // namespace win32
using DRAWITEMSTRUCT = win32::tagDRAWITEMSTRUCT;
using PDRAWITEMSTRUCT = win32::tagDRAWITEMSTRUCT*;
using LPDRAWITEMSTRUCT = win32::tagDRAWITEMSTRUCT*;

namespace win32 {
struct tagDELETEITEMSTRUCT {
    UINT CtlType;
    UINT CtlID;
    UINT itemID;
    HWND hwndItem;
    ULONG_PTR itemData;
};
} // namespace win32
using DELETEITEMSTRUCT = win32::tagDELETEITEMSTRUCT;
using LPDELETEITEMSTRUCT = win32::tagDELETEITEMSTRUCT*;

namespace win32 {
struct tagACCEL {
    BYTE fVirt;
    WORD key;
    WORD cmd;
  };
} // namespace win32
using ACCEL = win32::tagACCEL;
using LPACCEL = win32::tagACCEL*;

namespace win32 {
struct tagBITMAP {
    LONG bmType;
    LONG bmWidth;
    LONG bmHeight;
    LONG bmWidthBytes;
    WORD bmPlanes;
    WORD bmBitsPixel;
    LPVOID bmBits;
  };
} // namespace win32
using BITMAP = win32::tagBITMAP;
using PBITMAP = win32::tagBITMAP*;
using NPBITMAP = win32::tagBITMAP*;
using LPBITMAP = win32::tagBITMAP*;

#pragma pack(push, 1)
namespace win32 {
struct tagRGBTRIPLE {
    BYTE rgbtBlue;
    BYTE rgbtGreen;
    BYTE rgbtRed;
  };
} // namespace win32
using RGBTRIPLE = win32::tagRGBTRIPLE;
using PRGBTRIPLE = win32::tagRGBTRIPLE*;
using NPRGBTRIPLE = win32::tagRGBTRIPLE*;
using LPRGBTRIPLE = win32::tagRGBTRIPLE*;
#pragma pack(pop)

namespace win32 {
struct tagRGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
  };
} // namespace win32
using RGBQUAD = win32::tagRGBQUAD;

namespace win32 {
struct tagBITMAPCOREHEADER {
    DWORD bcSize;
    WORD bcWidth;
    WORD bcHeight;
    WORD bcPlanes;
    WORD bcBitCount;
  };
} // namespace win32
using BITMAPCOREHEADER = win32::tagBITMAPCOREHEADER;
using LPBITMAPCOREHEADER = win32::tagBITMAPCOREHEADER*;
using PBITMAPCOREHEADER = win32::tagBITMAPCOREHEADER*;

namespace win32 {
struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
  };
} // namespace win32
using BITMAPINFOHEADER = win32::tagBITMAPINFOHEADER;
using LPBITMAPINFOHEADER = win32::tagBITMAPINFOHEADER*;
using PBITMAPINFOHEADER = win32::tagBITMAPINFOHEADER*;

namespace win32 {
struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1];
  };
} // namespace win32
using BITMAPINFO = win32::tagBITMAPINFO;
using LPBITMAPINFO = win32::tagBITMAPINFO*;
using PBITMAPINFO = win32::tagBITMAPINFO*;

namespace win32 {
struct tagBITMAPCOREINFO {
    BITMAPCOREHEADER bmciHeader;
    RGBTRIPLE bmciColors[1];
  };
} // namespace win32
using BITMAPCOREINFO = win32::tagBITMAPCOREINFO;
using LPBITMAPCOREINFO = win32::tagBITMAPCOREINFO*;
using PBITMAPCOREINFO = win32::tagBITMAPCOREINFO*;

#pragma pack(push, 1)
namespace win32 {
struct tagBITMAPFILEHEADER {
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
  };
} // namespace win32
using BITMAPFILEHEADER = win32::tagBITMAPFILEHEADER;
using LPBITMAPFILEHEADER = win32::tagBITMAPFILEHEADER*;
using PBITMAPFILEHEADER = win32::tagBITMAPFILEHEADER*;
#pragma pack(pop)

namespace win32 {
struct tagPALETTEENTRY {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
  };
} // namespace win32
using PALETTEENTRY = win32::tagPALETTEENTRY;
using PPALETTEENTRY = win32::tagPALETTEENTRY*;
using LPPALETTEENTRY = win32::tagPALETTEENTRY*;

namespace win32 {
struct tagLOGPALETTE {
    WORD palVersion;
    WORD palNumEntries;
    PALETTEENTRY palPalEntry[1];
  };
} // namespace win32
using LOGPALETTE = win32::tagLOGPALETTE;
using PLOGPALETTE = win32::tagLOGPALETTE*;
using NPLOGPALETTE = win32::tagLOGPALETTE*;
using LPLOGPALETTE = win32::tagLOGPALETTE*;

namespace win32 {
struct tagLOGFONTA {
    LONG lfHeight;
    LONG lfWidth;
    LONG lfEscapement;
    LONG lfOrientation;
    LONG lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    CHAR lfFaceName[32];
  };
} // namespace win32
using LOGFONTA = win32::tagLOGFONTA;
using PLOGFONTA = win32::tagLOGFONTA*;
using NPLOGFONTA = win32::tagLOGFONTA*;
using LPLOGFONTA = win32::tagLOGFONTA*;

namespace win32 {
struct tagTEXTMETRICA {
    LONG tmHeight;
    LONG tmAscent;
    LONG tmDescent;
    LONG tmInternalLeading;
    LONG tmExternalLeading;
    LONG tmAveCharWidth;
    LONG tmMaxCharWidth;
    LONG tmWeight;
    LONG tmOverhang;
    LONG tmDigitizedAspectX;
    LONG tmDigitizedAspectY;
    BYTE tmFirstChar;
    BYTE tmLastChar;
    BYTE tmDefaultChar;
    BYTE tmBreakChar;
    BYTE tmItalic;
    BYTE tmUnderlined;
    BYTE tmStruckOut;
    BYTE tmPitchAndFamily;
    BYTE tmCharSet;
  };
} // namespace win32
using TEXTMETRICA = win32::tagTEXTMETRICA;
using PTEXTMETRICA = win32::tagTEXTMETRICA*;
using NPTEXTMETRICA = win32::tagTEXTMETRICA*;
using LPTEXTMETRICA = win32::tagTEXTMETRICA*;

#if UINTPTR_MAX == UINT32_MAX
#pragma pack(push, 1)
#endif
namespace win32 {
struct tagOFNA {
    DWORD lStructSize;
    HWND hwndOwner;
    HINSTANCE hInstance;
    LPCSTR lpstrFilter;
    LPSTR lpstrCustomFilter;
    DWORD nMaxCustFilter;
    DWORD nFilterIndex;
    LPSTR lpstrFile;
    DWORD nMaxFile;
    LPSTR lpstrFileTitle;
    DWORD nMaxFileTitle;
    LPCSTR lpstrInitialDir;
    LPCSTR lpstrTitle;
    DWORD Flags;
    WORD nFileOffset;
    WORD nFileExtension;
    LPCSTR lpstrDefExt;
    LPARAM lCustData;
    LPOFNHOOKPROC lpfnHook;
    LPCSTR lpTemplateName;
    void *pvReserved;
    DWORD dwReserved;
    DWORD FlagsEx;
  };
} // namespace win32
using OPENFILENAMEA = win32::tagOFNA;
using LPOPENFILENAMEA = win32::tagOFNA*;
#if UINTPTR_MAX == UINT32_MAX
#pragma pack(pop)
#endif

#if UINTPTR_MAX == UINT32_MAX
#pragma pack(push, 1)
#endif
namespace win32 {
struct tagPDA {
    DWORD lStructSize;
    HWND hwndOwner;
    HGLOBAL hDevMode;
    HGLOBAL hDevNames;
    HDC hDC;
    DWORD Flags;
    WORD nFromPage;
    WORD nToPage;
    WORD nMinPage;
    WORD nMaxPage;
    WORD nCopies;
    HINSTANCE hInstance;
    LPARAM lCustData;
    LPPRINTHOOKPROC lpfnPrintHook;
    LPSETUPHOOKPROC lpfnSetupHook;
    LPCSTR lpPrintTemplateName;
    LPCSTR lpSetupTemplateName;
    HGLOBAL hPrintTemplate;
    HGLOBAL hSetupTemplate;
  };
} // namespace win32
using PRINTDLGA = win32::tagPDA;
using LPPRINTDLGA = win32::tagPDA*;
#if UINTPTR_MAX == UINT32_MAX
#pragma pack(pop)
#endif

#if UINTPTR_MAX == UINT32_MAX
#pragma pack(push, 1)
#endif
namespace win32 {
struct tagDEVNAMES {
    WORD wDriverOffset;
    WORD wDeviceOffset;
    WORD wOutputOffset;
    WORD wDefault;
  };
} // namespace win32
using DEVNAMES = win32::tagDEVNAMES;
using LPDEVNAMES = win32::tagDEVNAMES*;

namespace win32 {
struct _devicemodeA {
    CHAR dmDeviceName[32];
    WORD dmSpecVersion, dmDriverVersion, dmSize, dmDriverExtra;
    DWORD dmFields;
    union {
        struct { SHORT dmOrientation, dmPaperSize, dmPaperLength, dmPaperWidth, dmScale, dmCopies, dmDefaultSource, dmPrintQuality; } printer;
        struct { POINT dmPosition; DWORD dmDisplayOrientation, dmDisplayFixedOutput; } display;
    } mode;
    SHORT dmColor, dmDuplex, dmYResolution, dmTTOption, dmCollate;
    CHAR dmFormName[32];
    WORD dmLogPixels;
    DWORD dmBitsPerPel, dmPelsWidth, dmPelsHeight;
    union { DWORD dmDisplayFlags; DWORD dmNup; } displayFlags;
    DWORD dmDisplayFrequency, dmICMMethod, dmICMIntent, dmMediaType, dmDitherType;
    DWORD dmReserved1, dmReserved2, dmPanningWidth, dmPanningHeight;
};
} // namespace win32
using DEVMODEA = win32::_devicemodeA;
using LPDEVMODEA = win32::_devicemodeA*;
#if UINTPTR_MAX == UINT32_MAX
#pragma pack(pop)
#endif

namespace win32 {
struct _OFSTRUCT {
    BYTE cBytes;
    BYTE fFixedDisk;
    WORD nErrCode;
    WORD Reserved1;
    WORD Reserved2;
    CHAR szPathName[128];
  };
} // namespace win32
using OFSTRUCT = win32::_OFSTRUCT;
using LPOFSTRUCT = win32::_OFSTRUCT*;
using POFSTRUCT = win32::_OFSTRUCT*;
#pragma pack(pop)

using WNDCLASS = WNDCLASSA;
using DEVMODE = DEVMODEA;
using LPWNDCLASS = WNDCLASS*;
using CREATESTRUCT = CREATESTRUCTA;
using LPCREATESTRUCT = CREATESTRUCT*;
using LOGFONT = LOGFONTA;
using LPLOGFONT = LOGFONT*;
using TEXTMETRIC = TEXTMETRICA;
using LPTEXTMETRIC = TEXTMETRIC*;
using OPENFILENAME = OPENFILENAMEA;
using LPOPENFILENAME = OPENFILENAME*;
using PRINTDLG = PRINTDLGA;
using LPPRINTDLG = PRINTDLG*;
namespace win32 {
struct _ICONINFO { BOOL fIcon; DWORD xHotspot, yHotspot; HBITMAP hbmMask, hbmColor; };
} // namespace win32
using ICONINFO = win32::_ICONINFO;
using PICONINFO = win32::_ICONINFO*;
namespace win32 {
struct tagSCROLLINFO { UINT cbSize, fMask; int nMin, nMax; UINT nPage; int nPos, nTrackPos; };
} // namespace win32
using SCROLLINFO = win32::tagSCROLLINFO;
using LPSCROLLINFO = win32::tagSCROLLINFO*;
using LPCSCROLLINFO = const SCROLLINFO*;
#endif
