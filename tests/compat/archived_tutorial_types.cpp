#include "win.h"
using namespace win32;
using APPMSG = win32::MSG;
// Type checking only: these APIs belong to the retired native Win32 backend.
namespace HistoricalWin32 {
[[noreturn]] void ExitProcess(UINT);
HWND GetTopWindow(HWND);
DWORD GetCurrentProcessId();
HWND FindWindowA(LPCSTR,LPCSTR);
}
#define ExitProcess HistoricalWin32::ExitProcess
#define GetTopWindow HistoricalWin32::GetTopWindow
#define GetCurrentProcessId HistoricalWin32::GetCurrentProcessId
#define FindWindowA HistoricalWin32::FindWindowA

// Compile the historical observer against current named values and payloads.
// Native-window/process functions above are declarations, never an SDL runtime.
#include "../reference/win32-tutorial/observer.c"
