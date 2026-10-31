#include <windows.h>

static thread_local DWORD dwError;

DWORD WINAPI GetLastError(void) { return dwError; }
void WINAPI SetLastError(DWORD dw) { dwError = dw; }
