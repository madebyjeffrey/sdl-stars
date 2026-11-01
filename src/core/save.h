#ifndef STARS_DECOMPILED_SAVE_H
#define STARS_DECOMPILED_SAVE_H

#include "common.h"
#include <cstdint>
#include <optional>

void    WriteOrders(APPSESSION &app, FLEET *lpfl);
void    WriteRtPlr(APPSESSION &app, PLAYER *pplr);
// Encoding borrows the destination for this call and leaves it unchanged on
// failure. A successful byte count excludes unused staging bytes.
std::optional<std::size_t> CbEncodeRtShDef(APPSESSION &app, const SHDEF *lpshdef, std::span<std::byte> rgbOut);
void    WriteRtShDef(APPSESSION &app, SHDEF *lpshdef);
std::int16_t FWriteDataFile(APPSESSION &app, char *pszFileBase, std::int16_t iPlayer, std::int16_t fAppend);
std::int16_t FAppendFile(APPSESSION &app, std::int16_t iPlayer);
void    WriteBattles(APPSESSION &app, std::int16_t iPlayer);
void    WritePlanet(APPSESSION &app, PLANET *lppl, RecordType rt, std::int16_t fHistory);
void    WriteFleet(APPSESSION &app, FLEET *lpfl);
void    WriteRtString(APPSESSION &app, const char *lpsz);
void    MarkFleet(APPSESSION &app, FLEET *lpfl, DetType det);
void    WriteBattlePlan(APPSESSION &app, BTLPLAN *lpbtlplan, std::int16_t fLog);
void    MarkPlanet(APPSESSION &app, PLANET *lppl, std::int16_t iPlr, DetType det);
void    SetSzWorkFromDt(APPSESSION &app, DtFileType dt, std::int16_t iPlayer);
std::int16_t FCreateFile(APPSESSION &app, DtFileType dt, std::int16_t iPlayer, char *szForceName);
void    WriteBOF(APPSESSION &app, std::int16_t iPlayer, std::int16_t dt, std::int16_t fMulti);
std::int16_t FMarkFile(APPSESSION &app, DtFileType dt, std::int16_t iPlayer, MdMark mdMark, std::int16_t f);
void    WriteRt(APPSESSION &app, RecordType rt, std::int16_t cb, void *rg);
void    WriteRtEof(APPSESSION &app, std::uint16_t w);
void    WriteRtThing(APPSESSION &app, const THING *pth);
void    WriteRtScore(APPSESSION &app, const SCOREX *psx);
void    RgToStream(APPSESSION &app, const void *rg, std::uint16_t cb);
void    SetVisiblePlanFleet(APPSESSION &app, std::int16_t iPlr);
void    SetVisPFInit(APPSESSION &app, std::int16_t iPlr);
void    SetVisPFFleets(APPSESSION &app, std::int16_t iPlr);
void    SetVisPFPlanets(APPSESSION &app, std::int16_t iPlr);
void    SetVisPFThings(APPSESSION &app, std::int16_t iPlr);
void    SetVisPFFinish(APPSESSION &app, std::int16_t iPlr);


#endif
