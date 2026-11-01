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
typedef win32::tagWINDOWPOS WINDOWPOS, *LPWINDOWPOS, *PWINDOWPOS;

namespace win32 {
struct tagPOINT {
  LONG x;
  LONG y;
};
} // namespace win32
typedef win32::tagPOINT POINT,*PPOINT,*NPPOINT,*LPPOINT;

namespace win32 {
struct tagSIZE {
  LONG cx;
  LONG cy;
};
} // namespace win32
typedef win32::tagSIZE SIZE,*PSIZE,*LPSIZE;

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
typedef win32::tagWNDCLASSA WNDCLASSA,*PWNDCLASSA,*NPWNDCLASSA,*LPWNDCLASSA;

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
typedef win32::tagMSG *PMSG,*NPMSG,*LPMSG;

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
typedef win32::tagPAINTSTRUCT PAINTSTRUCT,*PPAINTSTRUCT,*NPPAINTSTRUCT,*LPPAINTSTRUCT;

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
typedef win32::tagCREATESTRUCTA CREATESTRUCTA,*LPCREATESTRUCTA;

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
typedef win32::tagWINDOWPLACEMENT WINDOWPLACEMENT;

namespace win32 {
struct tagMINMAXINFO {
    POINT ptReserved;
    POINT ptMaxSize;
    POINT ptMaxPosition;
    POINT ptMinTrackSize;
    POINT ptMaxTrackSize;
  };
} // namespace win32
typedef win32::tagMINMAXINFO MINMAXINFO,*PMINMAXINFO,*LPMINMAXINFO;

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
typedef win32::tagMEASUREITEMSTRUCT MEASUREITEMSTRUCT,*PMEASUREITEMSTRUCT,*LPMEASUREITEMSTRUCT;

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
typedef win32::tagDRAWITEMSTRUCT DRAWITEMSTRUCT,*PDRAWITEMSTRUCT,*LPDRAWITEMSTRUCT;

namespace win32 {
struct tagDELETEITEMSTRUCT {
    UINT CtlType;
    UINT CtlID;
    UINT itemID;
    HWND hwndItem;
    ULONG_PTR itemData;
};
} // namespace win32
typedef win32::tagDELETEITEMSTRUCT DELETEITEMSTRUCT, *LPDELETEITEMSTRUCT;

namespace win32 {
struct tagACCEL {
    BYTE fVirt;
    WORD key;
    WORD cmd;
  };
} // namespace win32
typedef win32::tagACCEL ACCEL,*LPACCEL;

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
typedef win32::tagBITMAP BITMAP,*PBITMAP,*NPBITMAP,*LPBITMAP;

#pragma pack(push, 1)
namespace win32 {
struct tagRGBTRIPLE {
    BYTE rgbtBlue;
    BYTE rgbtGreen;
    BYTE rgbtRed;
  };
} // namespace win32
typedef win32::tagRGBTRIPLE RGBTRIPLE,*PRGBTRIPLE,*NPRGBTRIPLE,*LPRGBTRIPLE;
#pragma pack(pop)

namespace win32 {
struct tagRGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
  };
} // namespace win32
typedef win32::tagRGBQUAD RGBQUAD;

namespace win32 {
struct tagBITMAPCOREHEADER {
    DWORD bcSize;
    WORD bcWidth;
    WORD bcHeight;
    WORD bcPlanes;
    WORD bcBitCount;
  };
} // namespace win32
typedef win32::tagBITMAPCOREHEADER BITMAPCOREHEADER,*LPBITMAPCOREHEADER,*PBITMAPCOREHEADER;

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
typedef win32::tagBITMAPINFOHEADER BITMAPINFOHEADER,*LPBITMAPINFOHEADER,*PBITMAPINFOHEADER;

namespace win32 {
struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1];
  };
} // namespace win32
typedef win32::tagBITMAPINFO BITMAPINFO,*LPBITMAPINFO,*PBITMAPINFO;

namespace win32 {
struct tagBITMAPCOREINFO {
    BITMAPCOREHEADER bmciHeader;
    RGBTRIPLE bmciColors[1];
  };
} // namespace win32
typedef win32::tagBITMAPCOREINFO BITMAPCOREINFO,*LPBITMAPCOREINFO,*PBITMAPCOREINFO;

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
typedef win32::tagBITMAPFILEHEADER BITMAPFILEHEADER,*LPBITMAPFILEHEADER,*PBITMAPFILEHEADER;
#pragma pack(pop)

namespace win32 {
struct tagPALETTEENTRY {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
  };
} // namespace win32
typedef win32::tagPALETTEENTRY PALETTEENTRY,*PPALETTEENTRY,*LPPALETTEENTRY;

namespace win32 {
struct tagLOGPALETTE {
    WORD palVersion;
    WORD palNumEntries;
    PALETTEENTRY palPalEntry[1];
  };
} // namespace win32
typedef win32::tagLOGPALETTE LOGPALETTE,*PLOGPALETTE,*NPLOGPALETTE,*LPLOGPALETTE;

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
typedef win32::tagLOGFONTA LOGFONTA,*PLOGFONTA,*NPLOGFONTA,*LPLOGFONTA;

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
typedef win32::tagTEXTMETRICA TEXTMETRICA,*PTEXTMETRICA,*NPTEXTMETRICA,*LPTEXTMETRICA;

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
typedef win32::tagOFNA OPENFILENAMEA,*LPOPENFILENAMEA;
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
typedef win32::tagPDA PRINTDLGA,*LPPRINTDLGA;
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
typedef win32::tagDEVNAMES DEVNAMES,*LPDEVNAMES;

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
typedef win32::_devicemodeA DEVMODEA, *LPDEVMODEA;
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
typedef win32::_OFSTRUCT OFSTRUCT, *LPOFSTRUCT,*POFSTRUCT;
#pragma pack(pop)

typedef WNDCLASSA WNDCLASS;
typedef DEVMODEA DEVMODE;
typedef WNDCLASS *LPWNDCLASS;
typedef CREATESTRUCTA CREATESTRUCT;
typedef CREATESTRUCT *LPCREATESTRUCT;
typedef LOGFONTA LOGFONT;
typedef LOGFONT *LPLOGFONT;
typedef TEXTMETRICA TEXTMETRIC;
typedef TEXTMETRIC *LPTEXTMETRIC;
typedef OPENFILENAMEA OPENFILENAME;
typedef OPENFILENAME *LPOPENFILENAME;
typedef PRINTDLGA PRINTDLG;
typedef PRINTDLG *LPPRINTDLG;
namespace win32 {
struct _ICONINFO { BOOL fIcon; DWORD xHotspot, yHotspot; HBITMAP hbmMask, hbmColor; };
} // namespace win32
typedef win32::_ICONINFO ICONINFO, *PICONINFO;
namespace win32 {
struct tagSCROLLINFO { UINT cbSize, fMask; int nMin, nMax; UINT nPage; int nPos, nTrackPos; };
} // namespace win32
typedef win32::tagSCROLLINFO SCROLLINFO, *LPSCROLLINFO;
typedef const SCROLLINFO *LPCSCROLLINFO;
#endif
