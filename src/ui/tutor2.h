#ifndef STARS_DECOMPILED_TUTOR2_H
#define STARS_DECOMPILED_TUTOR2_H

#include <stdint.h>
#include <windows.h>

extern uint8_t acTUT[];
extern char    aTUTCmpr[];
extern int16_t aiTUTChunkOffset[];
extern char    rgTUTLookupTable[];

int16_t CchTutorString(char *pchOut, TutorId idt);

#endif
