#ifndef STARS_DECOMPILED_MSG_H
#define STARS_DECOMPILED_MSG_H

#include "common.h"
#include <cstdint>

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
std::int16_t PackageUpMsg(APPSESSION &app, std::uint8_t *pb, std::int16_t iPlr, MessageId iMsg, MsgGoto iObj, std::int16_t p1, std::int16_t p2, std::int16_t p3, std::int16_t p4, std::int16_t p5, std::int16_t p6,
                     std::int16_t p7);
std::int16_t FSendPlrMsg2XGen(APPSESSION &app, std::int16_t fPrepend, MessageId iMsg, MsgGoto iObj, std::int16_t p1, std::int16_t p2);
std::int16_t IdmGetMessageN(APPSESSION &app, std::int16_t iMsg);
std::int16_t FGetNMsgbig(APPSESSION &app, MessageId iMsg, MSGBIG *pmb);
std::array<char, 256> PszGetMessageN(APPSESSION &app, std::int16_t iMsg);
std::array<char, 256> PszFormatString(APPSESSION &app, const char *pszFormat, std::int16_t *pParamsReal);
std::array<char, 256> PszFormatMessage(APPSESSION &app, MessageId idm, std::int16_t *pParams);
std::array<char, 256> PszFormatIds(APPSESSION &app, StringId ids, std::int16_t *pParams);
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
