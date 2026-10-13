#ifndef STARS_DECOMPILED_FILE_H
#define STARS_DECOMPILED_FILE_H

#include <stdint.h>

extern char mpishdefishTutor[6];

int16_t FReadShDef(RTSHDEF *lprt, SHDEF *lpshdef, int16_t iplrLoad);
void    ReadRtPlr(PLAYER *pplr, uint8_t *pbIn);
int16_t FLoadGame(char *pszFileName, char *pszExt);
int16_t FReadPlanet(int16_t iPlayer, PLANET *lppl, int16_t fHistory, int16_t fPreInited);
int16_t FReadFleet(FLEET *lpfl);
void    UnpackBattlePlan(uint8_t *lpb, BTLPLAN *lpbtlplan, int16_t iplan);
void    UpdateBattleRecords();
void    DestroyCurGame();
void    FileError(StringId ids);
void    GetFileStatus(int16_t dt, int16_t iPlayer);
int16_t FOpenFile(DtFileType dt, int16_t iPlayer, int16_t md);
int16_t FNewTurnAvail(int16_t idPlayer);
int16_t FCheckFile(DtFileType dt, int16_t iPlayer, MdMark md);
void    ReadRt();
int16_t FBadFileError(StringId ids);
void    StreamOpen(char *szFile, MdOpenFlags mdOpen);
void    StreamClose();
void    RgFromStream(void *rg, uint16_t cb);

#endif
