#ifndef STARS_DECOMPILED_UTILGEN_H
#define STARS_DECOMPILED_UTILGEN_H

#include "common.h"
#include <cstdint>
#include <array>
#include <optional>
#include <span>
#include <string_view>

extern std::int16_t aiPNChunkOffset[];
extern std::uint8_t acPN[];
extern std::int16_t rgPrimes[128];
extern char    aPNCmpr[];
extern char    rgPNLookupTable[];


void    PushRandom(APPSESSION &app, std::int32_t lNew1, std::int32_t lNew2);
void    PopRandom(APPSESSION &app);
void    Randomize(APPSESSION &app, std::uint32_t dw);
void    Randomize2(APPSESSION &app, std::uint32_t dw);
std::int16_t Random(APPSESSION &app, std::int16_t c);
void    GetFileSeeds(APPSESSION &app, std::int32_t *pl1, std::int32_t *pl2);
void    SetFileSeeds(APPSESSION &app, std::int32_t l1, std::int32_t l2);
void    SetFileXorStream(APPSESSION &app, std::int32_t lid, std::int16_t lSalt, std::int16_t turn, std::int16_t iPlayer, std::int16_t fCrippled);
std::int32_t LGetNextFileXor(APPSESSION &app);
void    XorFileBuf(APPSESSION &app, char *rgb, std::int16_t cb);
int     ICompLong(APPSESSION &app, std::int32_t *pl1, std::int32_t *pl2);
char   *PszGetCompressedPlanet(APPSESSION &app, std::int16_t id);
// Append exactly the supplied text bytes; no terminator is written.
void    OutputFileString(const char *szFile, std::string_view sz);
void    StarsCopyFile(APPSESSION &app, const char *szSrc, const char *szDst);
std::int16_t AlertSz(APPSESSION &app, const char *sz, std::int16_t mbType);
std::int16_t CchGetString(APPSESSION &app, StringId ids, std::span<char> psz);
std::array<char, 12> PszFromInt(APPSESSION &app, std::int16_t i);
std::array<char, 12> PszFromLong(APPSESSION &app, std::int32_t l);
std::array<char, 12> PszFromLongK(APPSESSION &app, std::int32_t l);
std::array<char, 15> CommaFormatLong(APPSESSION &app, std::int32_t l);
void    BoundPoints(RECT *prc, POINT16 *rgpt, std::int16_t cpt);
// Success returns encoded bytes; failure leaves the destination unchanged.
// Input is non-NUL byte text; UTF-8 validation belongs to its record boundary.
std::optional<std::size_t> FCompressUserString(APPSESSION &app, std::string_view szIn, std::span<std::byte> szOut);
// Success returns decoded bytes excluding NUL. Output capacity includes NUL;
// failure empties a nonempty destination. The Bytes form is for legacy races.
std::optional<std::size_t> FDecompressUserString(APPSESSION &app, std::span<const std::byte> szIn, std::span<char> szOut);
std::optional<std::size_t> FDecompressUserStringBytes(APPSESSION &app, std::span<const std::byte> szIn, std::span<char> szOut);
std::int16_t NybbleFromCh(APPSESSION &app, std::uint8_t ch);
char    ChFromNybble(APPSESSION &app, std::int16_t nyb);
std::int16_t FIntersectCircleLine(POINT16 ptL1, POINT16 ptL2, POINT16 ptC, std::int32_t r2, std::int16_t dMax, std::int16_t *pdStart, std::int16_t *pdEnd);
// Success returns bytes excluding NUL; failure empties a nonempty destination.
std::optional<std::size_t> IntToRoman(std::int16_t i, std::span<char> pszOut);
std::int16_t FCheckPassword(APPSESSION &app);
std::int32_t LSaltFromSz(const char *psz);
std::int32_t LDistance2(POINT16 pt1, POINT16 pt2);
char   *PszGetLine(char **ppszBeg);
// Space-separated unsigned decimal fields retain modulo-32-bit values and
// partial assignments on invalid input. Stop at NUL/end or full output capacity.
std::optional<std::size_t> CParseNumbers(std::string_view psz, std::span<std::int32_t> pl);


#endif
