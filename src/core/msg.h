#ifndef STARS_DECOMPILED_MSG_H
#define STARS_DECOMPILED_MSG_H

#include <stdint.h>

extern char    rgcMsgArgs[];
extern uint8_t acMSG[];
extern char    aMSGCmpr[];
extern char    rgMSGLookupTable[];
extern int16_t aiMSGChunkOffset[];

int16_t IMsgNext(int16_t fFilteredOnly);
int16_t IMsgPrev(int16_t fFilteredOnly);
int16_t FSendPlrMsg2(int16_t iPlr, MessageId iMsg, MsgGoto iObj, int16_t p1, int16_t p2);
int16_t FSendPlrMsg(int16_t iPlr, MessageId iMsg, MsgGoto iObj, int16_t p1, int16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6, int16_t p7);
int16_t FSendPrependedPlrMsg(int16_t iPlr, MessageId iMsg, MsgGoto iObj, int16_t p1, int16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6, int16_t p7);
int16_t PackageUpMsg(uint8_t *pb, int16_t iPlr, MessageId iMsg, MsgGoto iObj, int16_t p1, int16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6,
                     int16_t p7);
int16_t FSendPlrMsg2XGen(int16_t fPrepend, MessageId iMsg, MsgGoto iObj, int16_t p1, int16_t p2);
int16_t IdmGetMessageN(int16_t iMsg);
int16_t FGetNMsgbig(MessageId iMsg, MSGBIG *pmb);
char   *PszGetMessageN(int16_t iMsg);
char   *PszFormatString(char *pszFormat, int16_t *pParamsReal);
char   *PszFormatMessage(MessageId idm, int16_t *pParams);
char   *PszFormatIds(StringId ids, int16_t *pParams);
int16_t FRemovePlayerMessage(int16_t iPlr, MessageId iMsg, MsgGoto iObj);
int16_t FFindPlayerMessage(int16_t iPlr, int16_t iMsg, MsgGoto iObj);
void    MarkPlanetsPlayerLost(int16_t iPlayer);
void    MarkPlayersThatSentMsgs(int16_t iPlayer);
void    WritePlayerMessages(int16_t iPlayer);
void    ResetMessages();
void    ReadPlayerMessages();
void    WriteRtPlrMsg(MSGPLR *lpmp);
MSGPLR *LpmsgplrFromRt();
char   *PszGetCompressedMessage(MessageId idm);
void    SetFilteringGroups(MessageId idm, int16_t fSet);

#endif
