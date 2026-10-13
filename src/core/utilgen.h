#ifndef STARS_DECOMPILED_UTILGEN_H
#define STARS_DECOMPILED_UTILGEN_H

#include <stdint.h>

extern int16_t aiPNChunkOffset[];
extern uint8_t acPN[];
extern int16_t rgPrimes[128];
extern char    aPNCmpr[];
extern char    rgPNLookupTable[];
extern int32_t lFileSeed1;
extern int32_t lFileSeed2;

void    PushRandom(int32_t lNew1, int32_t lNew2);
void    PopRandom();
void    Randomize(uint32_t dw);
void    Randomize2(uint32_t dw);
int16_t Random(int16_t c);
void    GetFileSeeds(int32_t *pl1, int32_t *pl2);
void    SetFileSeeds(int32_t l1, int32_t l2);
void    SetFileXorStream(int32_t lid, int16_t lSalt, int16_t turn, int16_t iPlayer, int16_t fCrippled);
int32_t LGetNextFileXor();
void    XorFileBuf(char *rgb, int16_t cb);
int     ICompLong(int32_t *pl1, int32_t *pl2);
char   *PszGetCompressedPlanet(int16_t id);
void    OutputFileString(char *szFile, char *sz);
void    StarsCopyFile(char *szSrc, char *szDst);
int16_t AlertSz(char *sz, int16_t mbType);
int16_t CchGetString(StringId ids, char *psz);
char   *PszFromInt(int16_t i, int16_t *pcch);
char   *PszFromLong(int32_t l, int16_t *pcch);
char   *PszFromLongK(int32_t l, int16_t *pcch);
int16_t CommaFormatLong(char *psz, int32_t l);
void    BoundPoints(RECT *prc, POINT16 *rgpt, int16_t cpt);
int16_t FCompressUserString(char *szIn, char *szOut, int16_t *pcOut);
int16_t FDecompressUserString(char *szIn, int16_t cIn, char *szOut, int16_t *pcOut);
int16_t NybbleFromCh(uint8_t ch);
char    ChFromNybble(int16_t nyb);
int16_t FIntersectCircleLine(POINT16 ptL1, POINT16 ptL2, POINT16 ptC, int32_t r2, int16_t dMax, int16_t *pdStart, int16_t *pdEnd);
void    IntToRoman(int16_t i, char *pszOut);
int16_t FCheckPassword();
int32_t LSaltFromSz(char *psz);
int32_t LDistance2(POINT16 pt1, POINT16 pt2);
char   *PszGetLine(char **ppszBeg);
int16_t CParseNumbers(char *psz, int32_t *pl, int16_t cMax);

#endif
