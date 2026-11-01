#ifndef STARS_DECOMPILED_STRINGS_H
#define STARS_DECOMPILED_STRINGS_H

#include "common.h"
#include <cstdint>
#include <array>
#include <cstddef>

extern char    aSTRCmpr[];
extern std::uint8_t acSTR[];
extern std::int16_t aiSTRChunkOffset[];
extern char    rgSTRLookupTable[];

// Includes the terminator; decoded text owns its storage without allocation.
inline constexpr std::size_t cchCatalogText = 256;
std::array<char, cchCatalogText> PszGetCompressedString(APPSESSION &app, StringId ids);

#endif
