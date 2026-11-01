#ifndef STARS_DECOMPILED_MSG_H
#define STARS_DECOMPILED_MSG_H

#include "common.h"
#include <stdint.h>

extern char    rgcMsgArgs[];
extern uint8_t acMSG[];
extern char    aMSGCmpr[];
extern char    rgMSGLookupTable[];
extern int16_t aiMSGChunkOffset[];

int16_t IMsgNext(APPSESSION &app, int16_t fFilteredOnly);
int16_t IMsgPrev(APPSESSION &app, int16_t fFilteredOnly);
int16_t FSendPlrMsg2(APPSESSION &app, int16_t iPlr, MessageId iMsg, MsgGoto iObj, int16_t p1, int16_t p2);
int16_t FSendPlrMsg(APPSESSION &app, int16_t iPlr, MessageId iMsg, MsgGoto iObj, int16_t p1, int16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6, int16_t p7);
int16_t FSendPrependedPlrMsg(APPSESSION &app, int16_t iPlr, MessageId iMsg, MsgGoto iObj, int16_t p1, int16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6, int16_t p7);
int16_t PackageUpMsg(APPSESSION &app, uint8_t *pb, int16_t iPlr, MessageId iMsg, MsgGoto iObj, int16_t p1, int16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6,
                     int16_t p7);
int16_t FSendPlrMsg2XGen(APPSESSION &app, int16_t fPrepend, MessageId iMsg, MsgGoto iObj, int16_t p1, int16_t p2);
int16_t IdmGetMessageN(APPSESSION &app, int16_t iMsg);
int16_t FGetNMsgbig(APPSESSION &app, MessageId iMsg, MSGBIG *pmb);
char   *PszGetMessageN(APPSESSION &app, int16_t iMsg);
char   *PszFormatString(APPSESSION &app, const char *pszFormat, int16_t *pParamsReal);
char   *PszFormatMessage(APPSESSION &app, MessageId idm, int16_t *pParams);
char   *PszFormatIds(APPSESSION &app, StringId ids, int16_t *pParams);
int16_t FRemovePlayerMessage(APPSESSION &app, int16_t iPlr, MessageId iMsg, MsgGoto iObj);
int16_t FFindPlayerMessage(APPSESSION &app, int16_t iPlr, int16_t iMsg, MsgGoto iObj);
void    MarkPlanetsPlayerLost(APPSESSION &app, int16_t iPlayer);
void    MarkPlayersThatSentMsgs(APPSESSION &app, int16_t iPlayer);
void    WritePlayerMessages(APPSESSION &app, int16_t iPlayer);
void    ResetMessages(APPSESSION &app);
void    ReadPlayerMessages(APPSESSION &app);
void    WriteRtPlrMsg(APPSESSION &app, MSGPLR *lpmp);
MSGPLR *LpmsgplrFromRt(APPSESSION &app);
char   *PszGetCompressedMessage(APPSESSION &app, MessageId idm);
void    SetFilteringGroups(APPSESSION &app, MessageId idm, int16_t fSet);

#endif
