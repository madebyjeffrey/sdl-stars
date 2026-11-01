#ifndef STARS_DECOMPILED_UTILGEN_H
#define STARS_DECOMPILED_UTILGEN_H

#include "common.h"
#include <cstdint>

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
void    OutputFileString(const char *szFile, const char *sz);
void    StarsCopyFile(APPSESSION &app, const char *szSrc, const char *szDst);
std::int16_t AlertSz(APPSESSION &app, const char *sz, std::int16_t mbType);
std::int16_t CchGetString(APPSESSION &app, StringId ids, char *psz);
char   *PszFromInt(APPSESSION &app, std::int16_t i, std::int16_t *pcch);
char   *PszFromLong(APPSESSION &app, std::int32_t l, std::int16_t *pcch);
char   *PszFromLongK(APPSESSION &app, std::int32_t l, std::int16_t *pcch);
std::int16_t CommaFormatLong(APPSESSION &app, char *psz, std::int32_t l);
void    BoundPoints(RECT *prc, POINT16 *rgpt, std::int16_t cpt);
std::int16_t FCompressUserString(APPSESSION &app, const char *szIn, char *szOut, std::int16_t *pcOut);
std::int16_t FDecompressUserString(APPSESSION &app, const char *szIn, std::int16_t cIn, char *szOut, std::int16_t *pcOut);
std::int16_t FDecompressUserStringBytes(APPSESSION &app, const char *szIn, std::int16_t cIn, char *szOut, std::int16_t *pcOut);
std::int16_t NybbleFromCh(APPSESSION &app, std::uint8_t ch);
char    ChFromNybble(APPSESSION &app, std::int16_t nyb);
std::int16_t FIntersectCircleLine(POINT16 ptL1, POINT16 ptL2, POINT16 ptC, std::int32_t r2, std::int16_t dMax, std::int16_t *pdStart, std::int16_t *pdEnd);
void    IntToRoman(std::int16_t i, char *pszOut);
std::int16_t FCheckPassword(APPSESSION &app);
std::int32_t LSaltFromSz(const char *psz);
std::int32_t LDistance2(POINT16 pt1, POINT16 pt2);
char   *PszGetLine(char **ppszBeg);
std::int16_t CParseNumbers(char *psz, std::int32_t *pl, std::int16_t cMax);


#endif
