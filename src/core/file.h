#ifndef STARS_DECOMPILED_FILE_H
#define STARS_DECOMPILED_FILE_H

#include "common.h"
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <cstddef>

extern char mpishdefishTutor[6];

// Success returns record bytes consumed, including its marker/terminator.
std::optional<std::size_t> FReadUserString(APPSESSION &app, std::span<const std::byte> lpb, std::span<char> psz);
std::int16_t FReadShDef(APPSESSION &app, const std::uint8_t *lprt, SHDEF *lpshdef, std::int16_t iplrLoad, std::size_t cb);
void    ReadRtPlr(APPSESSION &app, PLAYER *pplr, std::uint8_t *pbIn, std::size_t cb);
std::int16_t FLoadGame(APPSESSION &app, std::string_view pszFileName, std::string_view pszExt);
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
// Peek a bounded tutorial header or complete encoded record without advancing.
// Truncated memory sources report the same corrupt-file error as short file IO.
HDR     HdrMemStream(APPSESSION &app);
std::span<const std::byte> RgMemStreamRecord(APPSESSION &app);
void    ReadRt(APPSESSION &app);
std::int16_t FBadFileError(StringId ids);
void    StreamOpen(APPSESSION &app, const char *szFile, MdOpenFlags mdOpen);
void    StreamClose(APPSESSION &app);
void    RgFromStream(APPSESSION &app, std::span<std::byte> rg);


#endif
