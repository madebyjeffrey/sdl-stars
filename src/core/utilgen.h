#ifndef STARS_DECOMPILED_UTILGEN_H
#define STARS_DECOMPILED_UTILGEN_H

#include "common.h"
#include <stdint.h>

extern int16_t aiPNChunkOffset[];
extern uint8_t acPN[];
extern int16_t rgPrimes[128];
extern char    aPNCmpr[];
extern char    rgPNLookupTable[];


void    PushRandom(APPSESSION &app, int32_t lNew1, int32_t lNew2);
void    PopRandom(APPSESSION &app);
void    Randomize(APPSESSION &app, uint32_t dw);
void    Randomize2(APPSESSION &app, uint32_t dw);
int16_t Random(APPSESSION &app, int16_t c);
void    GetFileSeeds(APPSESSION &app, int32_t *pl1, int32_t *pl2);
void    SetFileSeeds(APPSESSION &app, int32_t l1, int32_t l2);
void    SetFileXorStream(APPSESSION &app, int32_t lid, int16_t lSalt, int16_t turn, int16_t iPlayer, int16_t fCrippled);
int32_t LGetNextFileXor(APPSESSION &app);
void    XorFileBuf(APPSESSION &app, char *rgb, int16_t cb);
int     ICompLong(APPSESSION &app, int32_t *pl1, int32_t *pl2);
char   *PszGetCompressedPlanet(APPSESSION &app, int16_t id);
void    OutputFileString(const char *szFile, const char *sz);
void    StarsCopyFile(APPSESSION &app, const char *szSrc, const char *szDst);
int16_t AlertSz(APPSESSION &app, const char *sz, int16_t mbType);
int16_t CchGetString(APPSESSION &app, StringId ids, char *psz);
char   *PszFromInt(APPSESSION &app, int16_t i, int16_t *pcch);
char   *PszFromLong(APPSESSION &app, int32_t l, int16_t *pcch);
char   *PszFromLongK(APPSESSION &app, int32_t l, int16_t *pcch);
int16_t CommaFormatLong(APPSESSION &app, char *psz, int32_t l);
void    BoundPoints(RECT *prc, POINT16 *rgpt, int16_t cpt);
int16_t FCompressUserString(APPSESSION &app, const char *szIn, char *szOut, int16_t *pcOut);
int16_t FDecompressUserString(APPSESSION &app, const char *szIn, int16_t cIn, char *szOut, int16_t *pcOut);
int16_t FDecompressUserStringBytes(APPSESSION &app, const char *szIn, int16_t cIn, char *szOut, int16_t *pcOut);
int16_t NybbleFromCh(APPSESSION &app, uint8_t ch);
char    ChFromNybble(APPSESSION &app, int16_t nyb);
int16_t FIntersectCircleLine(POINT16 ptL1, POINT16 ptL2, POINT16 ptC, int32_t r2, int16_t dMax, int16_t *pdStart, int16_t *pdEnd);
void    IntToRoman(int16_t i, char *pszOut);
int16_t FCheckPassword(APPSESSION &app);
int32_t LSaltFromSz(const char *psz);
int32_t LDistance2(POINT16 pt1, POINT16 pt2);
char   *PszGetLine(char **ppszBeg);
int16_t CParseNumbers(char *psz, int32_t *pl, int16_t cMax);


#endif
