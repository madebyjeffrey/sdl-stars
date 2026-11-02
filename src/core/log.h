#ifndef STARS_DECOMPILED_LOG_H
#define STARS_DECOMPILED_LOG_H

#include "common.h"
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

void    LogSplitFleet(APPSESSION &app, std::int16_t id);
void    LogMergeFleet(APPSESSION &app, std::int16_t id);
void    LogChangeShDef(APPSESSION &app, SHDEF *lpshdefNew);
// Empty text clears the name; accepted names are at most 31 UTF-8 bytes.
// Input is borrowed only for the call and may alias the current name.
[[nodiscard]] bool LogChangeName(APPSESSION &app, GrobjClass grobj, std::int16_t id, std::string_view szName);
void    LogChangeFleet(APPSESSION &app, FLEET *pfl, FLEET *pflNew);
void    LogChangeRelations(APPSESSION &app);
void    LogChangeBtlplan(APPSESSION &app, BTLPLAN *pbtlplan);
void    LogChangePlanet(APPSESSION &app, PLANET *ppl, PLANET *pplNew);
void    LogChangeThing(APPSESSION &app, THING *lpth, THING *pthNew);
void    LogMakeValidXfer(APPSESSION &app, LOGXFER *plx1, LOGXFER *plx2);
void    LogMakeValidXferf(APPSESSION &app, LOGXFERF *plxf1, LOGXFERF *plxf2);
void    CancelMemRt(APPSESSION &app, RecordType rt);
void    WriteMemRt(APPSESSION &app, RecordType rt, std::span<const std::byte> rgb);
void    WriteMemZpqRt(APPSESSION &app, std::int16_t cb);
void    DirtyGame(APPSESSION &app, std::int16_t fDirty);
std::optional<HDR> FGetPrevLogRt(APPSESSION &app, std::span<std::uint8_t> rgbPayload);
std::int16_t FRunLogFile(APPSESSION &app);
std::int16_t FRunLogRecord(APPSESSION &app, RecordType rt, std::int16_t cb, std::uint8_t *lpb);
std::int16_t FLoadLogFile(APPSESSION &app, std::string_view pszLog);
struct CHECKLOGRESULT {
    bool fValid;
    std::int16_t idsError;
};
CHECKLOGRESULT FCheckLogFile(APPSESSION &app, std::int16_t iplr);
std::int16_t FWriteLogFile(APPSESSION &app, std::string_view pszFileBase, std::int16_t iPlayer);
std::int16_t FWriteHistFile(APPSESSION &app, std::int16_t iPlayer);
using ENUMLOGRTCALLBACK = std::function<std::int16_t(APPSESSION &, std::span<std::uint8_t>, RecordType, std::int16_t)>;
void    EnumLogRts(APPSESSION &app, const ENUMLOGRTCALLBACK &pfn, std::int16_t iPass);


void WriteMemResearch(APPSESSION &app, std::int16_t wResearch);
void WriteMemPassword(APPSESSION &app, std::int32_t lSalt);

#endif
