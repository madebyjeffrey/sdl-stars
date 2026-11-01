#ifndef STARS_DECOMPILED_STRINGS_H
#define STARS_DECOMPILED_STRINGS_H

#include "common.h"
#include <cstdint>

extern char    aSTRCmpr[];
extern std::uint8_t acSTR[];
extern std::int16_t aiSTRChunkOffset[];
extern char    rgSTRLookupTable[];

char *PszGetCompressedString(APPSESSION &app, StringId ids);

#endif
