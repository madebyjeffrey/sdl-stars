#ifndef STARS_DECOMPILED_FILE_H
#define STARS_DECOMPILED_FILE_H

#include "common.h"
#include <stdint.h>
#include <stddef.h>

extern char mpishdefishTutor[6];

int16_t FReadUserString(const uint8_t *lpb, size_t cb, char *psz, size_t cchMax, size_t *pcbRead);
int16_t FReadShDef(RTSHDEF *lprt, SHDEF *lpshdef, int16_t iplrLoad, size_t cb);
void    ReadRtPlr(PLAYER *pplr, uint8_t *pbIn, size_t cb);
int16_t FLoadGame(const char *pszFileName, const char *pszExt);
int16_t FReadPlanet(int16_t iPlayer, PLANET *lppl, int16_t fHistory, int16_t fPreInited);
int16_t FReadFleet(FLEET *lpfl);
int16_t UnpackBattlePlan(uint8_t *lpb, BTLPLAN *lpbtlplan, int16_t iplan, size_t cb);
void    UpdateBattleRecords();
void    DestroyCurGame();
void    FileError(StringId ids);
void    GetFileStatus(int16_t dt, int16_t iPlayer);
int16_t FOpenFile(DtFileType dt, int16_t iPlayer, int16_t md);
int16_t FNewTurnAvail(int16_t idPlayer);
int16_t FCheckFile(DtFileType dt, int16_t iPlayer, MdMark md);
void    ReadRt();
int16_t FBadFileError(StringId ids);
void    StreamOpen(const char *szFile, MdOpenFlags mdOpen);
void    StreamClose();
void    RgFromStream(void *rg, uint16_t cb);

decltype(ReadRt) STARS_TEST_IMPL(ReadRt);
decltype(StreamOpen) STARS_TEST_IMPL(StreamOpen);
decltype(StreamClose) STARS_TEST_IMPL(StreamClose);

#endif
