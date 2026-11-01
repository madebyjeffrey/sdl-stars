#ifndef STARS_DECOMPILED_SAVE_H
#define STARS_DECOMPILED_SAVE_H

#include "common.h"
#include <stdint.h>

void    WriteOrders(APPSESSION &app, FLEET *lpfl);
void    WriteRtPlr(APPSESSION &app, PLAYER *pplr, uint8_t *pbStore);
void    WriteRtShDef(APPSESSION &app, SHDEF *lpshdef, uint8_t **ppbStore);
int16_t FWriteDataFile(APPSESSION &app, char *pszFileBase, int16_t iPlayer, int16_t fAppend);
int16_t FAppendFile(APPSESSION &app, int16_t iPlayer);
void    WriteBattles(APPSESSION &app, int16_t iPlayer);
void    WritePlanet(APPSESSION &app, PLANET *lppl, RecordType rt, int16_t fHistory);
void    WriteFleet(APPSESSION &app, FLEET *lpfl);
void    WriteRtString(APPSESSION &app, char *lpsz);
void    MarkFleet(APPSESSION &app, FLEET *lpfl, DetType det);
void    WriteBattlePlan(APPSESSION &app, BTLPLAN *lpbtlplan, int16_t fLog);
void    MarkPlanet(APPSESSION &app, PLANET *lppl, int16_t iPlr, DetType det);
void    SetSzWorkFromDt(APPSESSION &app, DtFileType dt, int16_t iPlayer);
int16_t FCreateFile(APPSESSION &app, DtFileType dt, int16_t iPlayer, char *szForceName);
void    WriteBOF(APPSESSION &app, int16_t iPlayer, int16_t dt, int16_t fMulti);
int16_t FMarkFile(APPSESSION &app, DtFileType dt, int16_t iPlayer, MdMark mdMark, int16_t f);
void    WriteRt(APPSESSION &app, RecordType rt, int16_t cb, void *rg);
void    WriteRtEof(APPSESSION &app, uint16_t w);
void    WriteRtThing(APPSESSION &app, const THING *pth);
void    WriteRtScore(APPSESSION &app, const SCOREX *psx);
void    RgToStream(APPSESSION &app, const void *rg, uint16_t cb);
void    SetVisiblePlanFleet(APPSESSION &app, int16_t iPlr);
void    SetVisPFInit(APPSESSION &app, int16_t iPlr);
void    SetVisPFFleets(APPSESSION &app, int16_t iPlr);
void    SetVisPFPlanets(APPSESSION &app, int16_t iPlr);
void    SetVisPFThings(APPSESSION &app, int16_t iPlr);
void    SetVisPFFinish(APPSESSION &app, int16_t iPlr);


#endif
