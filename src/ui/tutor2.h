#ifndef STARS_DECOMPILED_TUTOR2_H
#define STARS_DECOMPILED_TUTOR2_H

#include "win.h"
#include <cstdint>
#include <windows.h>

extern std::uint8_t acTUT[];
extern char    aTUTCmpr[];
extern std::int16_t aiTUTChunkOffset[];
extern char    rgTUTLookupTable[];

std::int16_t CchTutorString(APPSESSION &app, char *pchOut, TutorId idt);

#endif
