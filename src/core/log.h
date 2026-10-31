#ifndef STARS_DECOMPILED_LOG_H
#define STARS_DECOMPILED_LOG_H

#include "common.h"
#include <stdint.h>

void    LogSplitFleet(APPSESSION &app, int16_t id);
void    LogMergeFleet(APPSESSION &app, int16_t id);
void    LogChangeShDef(APPSESSION &app, SHDEF *lpshdefNew);
void    LogChangeName(APPSESSION &app, GrobjClass grobj, int16_t id, char *szName);
void    LogChangeFleet(APPSESSION &app, FLEET *pfl, FLEET *pflNew);
void    LogChangeRelations(APPSESSION &app);
void    LogChangeBtlplan(APPSESSION &app, BTLPLAN *pbtlplan);
void    LogChangePlanet(APPSESSION &app, PLANET *ppl, PLANET *pplNew);
void    LogChangeThing(APPSESSION &app, THING *lpth, THING *pthNew);
void    LogMakeValidXfer(APPSESSION &app, LOGXFER *plx1, LOGXFER *plx2);
void    LogMakeValidXferf(APPSESSION &app, LOGXFERF *plxf1, LOGXFERF *plxf2);
void    CancelMemRt(APPSESSION &app, RecordType rt);
void    WriteMemRt(APPSESSION &app, RecordType rt, int16_t cb, void *rg);
void    WriteMemZpqRt(APPSESSION &app, int16_t cb);
void    DirtyGame(APPSESSION &app, int16_t fDirty);
int16_t FGetPrevLogRt(APPSESSION &app, HDR *phdr, uint8_t *pb);
int16_t FRunLogFile(APPSESSION &app);
int16_t FRunLogRecord(APPSESSION &app, RecordType rt, int16_t cb, uint8_t *lpb);
int16_t FLoadLogFile(APPSESSION &app, char *pszLog);
int16_t FCheckLogFile(APPSESSION &app, int16_t iplr, int16_t *pfError);
int16_t FWriteLogFile(APPSESSION &app, char *pszFileBase, int16_t iPlayer);
int16_t FWriteHistFile(APPSESSION &app, int16_t iPlayer);
void    EnumLogRts(APPSESSION &app, int16_t (*pfn)(APPSESSION &, void *, int16_t, int16_t, void *, int16_t), void *lpPass, int16_t iPass);


void WriteMemResearch(APPSESSION &app, int16_t wResearch);
void WriteMemPassword(APPSESSION &app, int32_t lSalt);

#endif
