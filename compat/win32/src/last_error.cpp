#include <windows.h>

static thread_local DWORD dwError;

DWORD WINAPI win32::GetLastError(void) { return dwError; }
void WINAPI win32::SetLastError(DWORD dw) { dwError = dw; }
