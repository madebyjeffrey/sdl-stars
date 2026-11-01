#ifndef STARS_DECOMPILED_STRINGS_H
#define STARS_DECOMPILED_STRINGS_H

#include "common.h"
#include <stdint.h>

extern char    aSTRCmpr[];
extern uint8_t acSTR[];
extern int16_t aiSTRChunkOffset[];
extern char    rgSTRLookupTable[];

char *PszGetCompressedString(APPSESSION &app, StringId ids);

#endif
