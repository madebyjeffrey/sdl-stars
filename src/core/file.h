#ifndef STARS_DECOMPILED_FILE_H
#define STARS_DECOMPILED_FILE_H

#include "common.h"
#include <stdint.h>
#include <stddef.h>

extern char mpishdefishTutor[6];

int16_t FReadUserString(APPSESSION &app, const uint8_t *lpb, size_t cb, char *psz, size_t cchMax, size_t *pcbRead);
int16_t FReadShDef(APPSESSION &app, const uint8_t *lprt, SHDEF *lpshdef, int16_t iplrLoad, size_t cb);
void    ReadRtPlr(APPSESSION &app, PLAYER *pplr, uint8_t *pbIn, size_t cb);
int16_t FLoadGame(APPSESSION &app, const char *pszFileName, const char *pszExt);
int16_t FReadPlanet(APPSESSION &app, int16_t iPlayer, PLANET *lppl, int16_t fHistory, int16_t fPreInited);
int16_t FReadFleet(APPSESSION &app, FLEET *lpfl);
int16_t UnpackBattlePlan(APPSESSION &app, uint8_t *lpb, BTLPLAN *lpbtlplan, int16_t iplan, size_t cb);
void    UpdateBattleRecords(APPSESSION &app);
void    DestroyCurGame(APPSESSION &app);
void    FileError(APPSESSION &app, StringId ids);
void    GetFileStatus(APPSESSION &app, int16_t dt, int16_t iPlayer);
int16_t FOpenFile(APPSESSION &app, DtFileType dt, int16_t iPlayer, int16_t md);
int16_t FNewTurnAvail(APPSESSION &app, int16_t idPlayer);
int16_t FCheckFile(APPSESSION &app, DtFileType dt, int16_t iPlayer, MdMark md);
void    ReadRt(APPSESSION &app);
int16_t FBadFileError(StringId ids);
void    StreamOpen(APPSESSION &app, const char *szFile, MdOpenFlags mdOpen);
void    StreamClose(APPSESSION &app);
void    RgFromStream(APPSESSION &app, void *rg, uint16_t cb);


#endif
