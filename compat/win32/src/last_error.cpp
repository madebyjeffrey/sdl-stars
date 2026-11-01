#include <windows.h>

static thread_local DWORD dwError;

DWORD win32::GetLastError(void) { return dwError; }
void win32::SetLastError(DWORD dw) { dwError = dw; }
