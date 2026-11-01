#ifndef STARS_DECOMPILED_LOG_H
#define STARS_DECOMPILED_LOG_H

#include "common.h"
#include <cstdint>

void    LogSplitFleet(APPSESSION &app, std::int16_t id);
void    LogMergeFleet(APPSESSION &app, std::int16_t id);
void    LogChangeShDef(APPSESSION &app, SHDEF *lpshdefNew);
void    LogChangeName(APPSESSION &app, GrobjClass grobj, std::int16_t id, char *szName);
void    LogChangeFleet(APPSESSION &app, FLEET *pfl, FLEET *pflNew);
void    LogChangeRelations(APPSESSION &app);
void    LogChangeBtlplan(APPSESSION &app, BTLPLAN *pbtlplan);
void    LogChangePlanet(APPSESSION &app, PLANET *ppl, PLANET *pplNew);
void    LogChangeThing(APPSESSION &app, THING *lpth, THING *pthNew);
void    LogMakeValidXfer(APPSESSION &app, LOGXFER *plx1, LOGXFER *plx2);
void    LogMakeValidXferf(APPSESSION &app, LOGXFERF *plxf1, LOGXFERF *plxf2);
void    CancelMemRt(APPSESSION &app, RecordType rt);
void    WriteMemRt(APPSESSION &app, RecordType rt, std::int16_t cb, void *rg);
void    WriteMemZpqRt(APPSESSION &app, std::int16_t cb);
void    DirtyGame(APPSESSION &app, std::int16_t fDirty);
std::int16_t FGetPrevLogRt(APPSESSION &app, HDR *phdr, std::uint8_t *pb);
std::int16_t FRunLogFile(APPSESSION &app);
std::int16_t FRunLogRecord(APPSESSION &app, RecordType rt, std::int16_t cb, std::uint8_t *lpb);
std::int16_t FLoadLogFile(APPSESSION &app, char *pszLog);
std::int16_t FCheckLogFile(APPSESSION &app, std::int16_t iplr, std::int16_t *pfError);
std::int16_t FWriteLogFile(APPSESSION &app, char *pszFileBase, std::int16_t iPlayer);
std::int16_t FWriteHistFile(APPSESSION &app, std::int16_t iPlayer);
void    EnumLogRts(APPSESSION &app, std::int16_t (*pfn)(APPSESSION &, void *, std::int16_t, std::int16_t, void *, std::int16_t), void *lpPass, std::int16_t iPass);


void WriteMemResearch(APPSESSION &app, std::int16_t wResearch);
void WriteMemPassword(APPSESSION &app, std::int32_t lSalt);

#endif
