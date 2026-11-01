#ifndef STARS_DECOMPILED_FILE_H
#define STARS_DECOMPILED_FILE_H

#include "common.h"
#include <cstdint>
#include <cstddef>

extern char mpishdefishTutor[6];

std::int16_t FReadUserString(APPSESSION &app, const std::uint8_t *lpb, std::size_t cb, char *psz, std::size_t cchMax, std::size_t *pcbRead);
std::int16_t FReadShDef(APPSESSION &app, const std::uint8_t *lprt, SHDEF *lpshdef, std::int16_t iplrLoad, std::size_t cb);
void    ReadRtPlr(APPSESSION &app, PLAYER *pplr, std::uint8_t *pbIn, std::size_t cb);
std::int16_t FLoadGame(APPSESSION &app, const char *pszFileName, const char *pszExt);
std::int16_t FReadPlanet(APPSESSION &app, std::int16_t iPlayer, PLANET *lppl, std::int16_t fHistory, std::int16_t fPreInited);
std::int16_t FReadFleet(APPSESSION &app, FLEET *lpfl);
std::int16_t UnpackBattlePlan(APPSESSION &app, std::uint8_t *lpb, BTLPLAN *lpbtlplan, std::int16_t iplan, std::size_t cb);
void    UpdateBattleRecords(APPSESSION &app);
void    DestroyCurGame(APPSESSION &app);
void    FileError(APPSESSION &app, StringId ids);
void    GetFileStatus(APPSESSION &app, std::int16_t dt, std::int16_t iPlayer);
std::int16_t FOpenFile(APPSESSION &app, DtFileType dt, std::int16_t iPlayer, std::int16_t md);
std::int16_t FNewTurnAvail(APPSESSION &app, std::int16_t idPlayer);
std::int16_t FCheckFile(APPSESSION &app, DtFileType dt, std::int16_t iPlayer, MdMark md);
void    ReadRt(APPSESSION &app);
std::int16_t FBadFileError(StringId ids);
void    StreamOpen(APPSESSION &app, const char *szFile, MdOpenFlags mdOpen);
void    StreamClose(APPSESSION &app);
void    RgFromStream(APPSESSION &app, void *rg, std::uint16_t cb);


#endif
