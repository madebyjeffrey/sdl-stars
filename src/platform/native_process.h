#ifndef STARS_NATIVE_PROCESS_H
#define STARS_NATIVE_PROCESS_H
#include <stdint.h>

#ifdef _WIN32
char *PszNativeCommandLine(void);
char **RgpszNativeArguments(int *lpc);
#endif

// Milliseconds used for retries and file salts, with the platform tick behavior.
uint32_t DwTickCount();
#endif
