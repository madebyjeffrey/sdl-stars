#ifndef STARS_DECOMPILED_MSG_H
#define STARS_DECOMPILED_MSG_H

#include "common.h"
#include <cstdint>
#include <string_view>

extern char    rgcMsgArgs[];
extern std::uint8_t acMSG[];
extern char    aMSGCmpr[];
extern char    rgMSGLookupTable[];
extern std::int16_t aiMSGChunkOffset[];

std::int16_t IMsgNext(APPSESSION &app, std::int16_t fFilteredOnly);
std::int16_t IMsgPrev(APPSESSION &app, std::int16_t fFilteredOnly);
std::int16_t FSendPlrMsg2(APPSESSION &app, std::int16_t iPlr, MessageId iMsg, MsgGoto iObj, std::int16_t p1, std::int16_t p2);
std::int16_t FSendPlrMsg(APPSESSION &app, std::int16_t iPlr, MessageId iMsg, MsgGoto iObj, std::int16_t p1, std::int16_t p2, std::int16_t p3, std::int16_t p4, std::int16_t p5, std::int16_t p6, std::int16_t p7);
std::int16_t FSendPrependedPlrMsg(APPSESSION &app, std::int16_t iPlr, MessageId iMsg, MsgGoto iObj, std::int16_t p1, std::int16_t p2, std::int16_t p3, std::int16_t p4, std::int16_t p5, std::int16_t p6, std::int16_t p7);
std::int16_t PackageUpMsg(APPSESSION &app, std::span<std::uint8_t> rgb, std::int16_t iPlr, MessageId iMsg, MsgGoto iObj, std::int16_t p1, std::int16_t p2, std::int16_t p3, std::int16_t p4, std::int16_t p5, std::int16_t p6,
                     std::int16_t p7);
std::int16_t FSendPlrMsg2XGen(APPSESSION &app, std::int16_t fPrepend, MessageId iMsg, MsgGoto iObj, std::int16_t p1, std::int16_t p2);
std::int16_t IdmGetMessageN(APPSESSION &app, std::int16_t iMsg);
std::int16_t FGetNMsgbig(APPSESSION &app, MessageId iMsg, MSGBIG *pmb);
struct MESSAGETEXTRESULT {
    std::array<char, 256> sz{};
    std::int16_t cch = 0;

    char *data() noexcept { return sz.data(); }
    const char *data() const noexcept { return sz.data(); }
    std::string_view view() const noexcept { return {sz.data(), static_cast<std::size_t>(cch)}; }
};
MESSAGETEXTRESULT PszGetMessageN(APPSESSION &app, std::int16_t iMsg);
// Readonly bounded inputs; stop before an incomplete expansion and terminate
// the complete UTF-8 output prefix. Embedded NUL ends the format text.
MESSAGETEXTRESULT PszFormatMessage(APPSESSION &app, MessageId idm, std::span<const std::int16_t> pParams);
MESSAGETEXTRESULT PszFormatString(APPSESSION &app, std::string_view pszFormat, std::span<const std::int16_t> pParamsReal);
MESSAGETEXTRESULT PszFormatIds(APPSESSION &app, StringId ids, std::span<const std::int16_t> pParams);
std::int16_t FRemovePlayerMessage(APPSESSION &app, std::int16_t iPlr, MessageId iMsg, MsgGoto iObj);
std::int16_t FFindPlayerMessage(APPSESSION &app, std::int16_t iPlr, std::int16_t iMsg, MsgGoto iObj);
void    MarkPlanetsPlayerLost(APPSESSION &app, std::int16_t iPlayer);
void    MarkPlayersThatSentMsgs(APPSESSION &app, std::int16_t iPlayer);
void    WritePlayerMessages(APPSESSION &app, std::int16_t iPlayer);
void    ResetMessages(APPSESSION &app);
void    ReadPlayerMessages(APPSESSION &app);
void    WriteRtPlrMsg(APPSESSION &app, MSGPLR *lpmp);
MSGPLR *LpmsgplrFromRt(APPSESSION &app);
std::array<char, 256> PszGetCompressedMessage(APPSESSION &app, MessageId idm);
void    SetFilteringGroups(APPSESSION &app, MessageId idm, std::int16_t fSet);

#endif
