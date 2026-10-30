#include "common.h"

int16_t IMsgNext(int16_t fFilteredOnly) {
    int16_t   i;
    MessageId idm;

    i = iMsgCur;
    if (fViewFilteredMsg && !fFilteredOnly) {
        if (i < cMsg + vcmsgplrIn - 1) {
            return i + 1;
        }
        return imsgNone;
    }
    do {
        i++;
        if (i < cMsg) {
            idm = static_cast<MessageId>(IdmGetMessageN(i));
        } else {
            if (i < cMsg + vcmsgplrIn) {
                return i;
            }
            return imsgNone;
        }
    } while ((((bitfMsgFiltered[idm >> 3] & 1 << (idm & 7)) == 0) ^ fFilteredOnly) == 0);
    return i;
}

int16_t IMsgPrev(int16_t fFilteredOnly) {
    int16_t   i;
    MessageId idm;

    i = iMsgCur;
    if (fViewFilteredMsg && !fFilteredOnly) {
        if (i > 0) {
            return i - 1;
        }
        return imsgNone;
    }
    if (i > cMsg) {
        return i - 1;
    }
    do {
        i--;
        if (i < 0) {
            return imsgNone;
        }
        idm = static_cast<MessageId>(IdmGetMessageN(i));
    } while ((((bitfMsgFiltered[idm >> 3] & 1 << (idm & 7)) == 0) ^ fFilteredOnly) == 0);
    return i;
}

int16_t FSendPlrMsg2(int16_t iPlr, MessageId iMsg, MsgGoto iObj, int16_t p1, int16_t p2) { return FSendPlrMsg(iPlr, iMsg, iObj, p1, p2, 0, 0, 0, 0, 0); }

int16_t FSendPlrMsg(int16_t iPlr, MessageId iMsg, MsgGoto iObj, int16_t p1, int16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6, int16_t p7) {
    uint8_t  rgbWork[40];
    int16_t  cbMsg;
    uint8_t *lpb;

    cbMsg = PackageUpMsg(rgbWork, iPlr, iMsg, iObj, p1, p2, p3, p4, p5, p6, p7);
    if (cbMsg <= 0) {
        if (cbMsg == 0) {
            return TRUE;
        }
        return FALSE;
    }
    lpb = (uint8_t *)lpMsg + imemMsgCur;
    memmove(lpb, rgbWork, cbMsg);
    imemMsgCur += cbMsg;
    cMsg++;
    return TRUE;
}

int16_t FSendPrependedPlrMsg(int16_t iPlr, MessageId iMsg, MsgGoto iObj, int16_t p1, int16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6, int16_t p7) {
    uint8_t rgbWork[40];
    int16_t cbMsg;

    cbMsg = PackageUpMsg(rgbWork, iPlr, iMsg, iObj, p1, p2, p3, p4, p5, p6, p7);
    if (cbMsg <= 0) {
        if (cbMsg == 0) {
            return TRUE;
        }
        return FALSE;
    }
    memmove((uint8_t *)lpMsg + cbMsg, lpMsg, imemMsgCur);
    memmove(lpMsg, rgbWork, cbMsg);
    imemMsgCur += cbMsg;
    cMsg++;
    return TRUE;
}

int16_t PackageUpMsg(uint8_t *pb, int16_t iPlr, MessageId iMsg, MsgGoto iObj, int16_t p1, int16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6,
                     int16_t p7) {
    int16_t *pi;
    int16_t  i;
    uint16_t grbit;
    uint8_t *lpmt;
    MSGTURN mt{};
    uint8_t *lpb;
    uint8_t *lpbBase;
    int16_t  rgArgs[7];

    if (iPlr == iplrNone) {
        return 0;
    }
    if (rgplr[iPlr].fAi && rgplr[iPlr].idAi != idAiMaid) {
        switch (iMsg) {
        default:
            return 0;
        case idmHasBombedKillingOffEnemyColonists:
        case idmHaveAttackedFirstRateStormTroopersThough:
        case idmColonistsHaveDiedOffLongerControlPlanet:
        case idmColonistsHaveJumpedShipLongerControlPlanet:
            break;
        }
    }
    if ((uint16_t)(imemMsgCur + 20) > 0xffc8) {
        return -1;
    }
    lpb = pb;
    lpmt = lpb;
    mt.iPlr = iPlr & 0xf;
    mt.msghdr.iMsg = iMsg;
    mt.msghdr.grWord = 0;
    mt.msghdr.wGoto = iObj;
    FEncodeMsgTurn(mt, std::as_writable_bytes(std::span(lpb, cbWireMsgTurn)));
    lpb += 5;
    lpbBase = lpb;
    grbit = 1;
    rgArgs[0] = p1;
    rgArgs[1] = p2;
    rgArgs[2] = p3;
    rgArgs[3] = p4;
    rgArgs[4] = p5;
    rgArgs[5] = p6;
    rgArgs[6] = p7;
    pi = rgArgs;
    i = 0;
    while (i < rgcMsgArgs[iMsg]) {
        if (*pi & 0xff00) {
            MtvFromPrefix(lpmt).msghdr().SetGrWord(MtvFromPrefix(lpmt).msghdr().grWord() | grbit);
            SetMsgParam(lpb, *pi);
            lpb += 2;
        } else {
            *lpb = *pi;
            lpb++;
        }
        i++;
        pi++;
        grbit *= 2;
    }
    MtvFromPrefix(lpmt).SetCbParams((lpb - lpbBase) & 0xf);
    return lpb - pb;
}

int16_t FSendPlrMsg2XGen(int16_t fPrepend, MessageId iMsg, MsgGoto iObj, int16_t p1, int16_t p2) {
    uint8_t  rgb[64];
    int16_t *pi;
    int16_t  i;
    uint16_t grbit;
    uint8_t *pb;
    uint16_t cSize;
    uint8_t *pmsghdr;
    MSGHDR mh{};
    int16_t  rgArgs[2];

    if ((uint16_t)(imemMsgCur + 20) > 0xffc8) {
        return FALSE;
    }
    pb = rgb;
    pmsghdr = pb;
    mh.iMsg = iMsg;
    bitfMsgSent[iMsg >> 3] = (bitfMsgSent[iMsg >> 3] & ~(1 << (iMsg & 7))) | 1 << (iMsg & 7);
    mh.grWord = 0;
    mh.wGoto = iObj;
    FEncodeMsgHdr(mh, std::as_writable_bytes(std::span(pb, cbWireMsgHdr)));
    pb += 4;
    grbit = 1;
    rgArgs[0] = p1;
    rgArgs[1] = p2;
    pi = rgArgs;
    i = 0;
    while (i < rgcMsgArgs[iMsg]) {
        if (*pi & 0xff00) {
            MhvFromPrefix(pmsghdr).SetGrWord(MhvFromPrefix(pmsghdr).grWord() | grbit);
            SetMsgParam(pb, *pi);
            pb += 2;
        } else {
            *pb = *pi;
            pb++;
        }
        i++;
        pi++;
        grbit *= 2;
    }
    cSize = pb - rgb;
    if (fPrepend) {
        memmove((uint8_t *)lpMsg + cSize, lpMsg, imemMsgCur);
        memmove(lpMsg, rgb, cSize);
    } else {
        memmove((uint8_t *)lpMsg + imemMsgCur, rgb, cSize);
    }
    imemMsgCur += cSize;
    cMsg++;
    iMsgCur = imsgNone;
    iMsgCur = IMsgNext(FALSE);
    return TRUE;
}

int16_t IdmGetMessageN(int16_t iMsg) {
    MSGBIG mb;

    if (!FGetNMsgbig(static_cast<MessageId>(iMsg), &mb)) {
        return -1;
    }
    return mb.iMsg;
}

int16_t FGetNMsgbig(MessageId iMsg, MSGBIG *pmb) {
    uint8_t *lpbMax;
    int16_t  iMax;
    uint8_t *lpmh;
    int16_t  i;
    uint8_t *lpb;
    uint16_t u;

    if ((int16_t)iMsg < idmColonistsDroppedMassacredGroundTroops || (int16_t)iMsg >= cMsg) {
        return FALSE;
    }
    lpb = (uint8_t *)lpMsg;
    lpbMax = lpb + imemMsgCur;
    while (lpb < lpbMax) {
        lpmh = lpb;
        u = MhvFromLp(lpmh).grWord();
        lpb += 4;
        if (iMsg == idmColonistsDroppedMassacredGroundTroops) {
            pmb->iMsg = MhvFromLp(lpmh).iMsg();
            pmb->wGoto = MhvFromLp(lpmh).wGoto();
        }
        iMax = rgcMsgArgs[MhvFromLp(lpmh).iMsg()];
        for (i = 0; i < iMax; i++) {
            if (iMsg == idmColonistsDroppedMassacredGroundTroops) {
                pmb->rgParam[i] = !(u & 1) ? *lpb : WMsgParam(lpb);
            }
            lpb += 1 + ((u & 1) == 1);
            u >>= 1;
        }
        if ((int16_t)iMsg <= idmColonistsDroppedMassacredGroundTroops)
            break;
        iMsg = static_cast<MessageId>(iMsg - 1);
    }
    return TRUE;
}

char *PszGetMessageN(int16_t iMsg) {
    MSGBIG mb;
    char  *psz;

    if (!FGetNMsgbig(static_cast<MessageId>(iMsg), &mb)) {
        szMsgBuf[0] = 0;
        return szMsgBuf;
    }
    psz = PszFormatMessage(static_cast<MessageId>(mb.iMsg), mb.rgParam);
    return psz;
}

// Formatting may expand bounded names or paths beyond the message buffer.
static char *PchAppendMessageText(char *pch, const char *psz) {
    size_t cch;
    std::string_view szText(psz);

    cch = CchUtf8Prefix(szText, (size_t)(szMsgBuf + sizeof(szMsgBuf) - 1 - pch));
    memmove(pch, psz, cch);
    pch[cch] = 0;
    return pch + cch;
}

char *PszFormatString(const char *pszFormat, int16_t *pParamsReal) {
    int16_t  iMineral;
    int16_t  cOut;
    int16_t  c;
    int16_t  i;
    int16_t *pParams;
    const char *pchT;
    char     szBuf[480];
    uint16_t w;
    char    *pch;
    PART     part;
    int32_t  l;
    SHDEF   *lpshdef;

    iMineral = -1;
    pParams = pParamsReal;
    pch = szMsgBuf;
    for (; *pszFormat != 0 && pch < szMsgBuf + sizeof(szMsgBuf) - 1; pszFormat++) {
        if (*pszFormat != '\\') {
            *pch++ = *pszFormat;
        } else {
            pszFormat++;
            if (*pszFormat == 0) break;
            switch (*pszFormat) {
            case 'w':
                pch = PchAppendMessageText(pch, szWork);
                break;
            case 'f':
            case 'h':
            case 'r':
            case 't':
            case 'y':
                pch = PchAppendMessageText(pch, szBase);
                switch (*pszFormat) {
                case 'f':
                    if (idPlayer != iplrNone) {
                        c = CchSprintf(szBuf, ".x%d", idPlayer + 1);
                        goto DoInt;
                    }
                    /* fallthrough */
                case 't':
                    if (idPlayer != iplrNone) {
                        c = CchSprintf(szBuf, ".m%d", idPlayer + 1);
                        goto DoInt;
                    }
                    /* fallthrough */
                case 'h':
                    pch = PchAppendMessageText(pch, ".hst");
                    break;
                case 'r':
                    c = CchSprintf(szBuf, ".h%d", idPlayer + 1);
                    goto DoInt;
                case 'y':
                    pch = PchAppendMessageText(pch, ".xy");
                }
                break;
            case 'e':
                pchT = rgszPlanetAttr[*pParams];
                goto FinishString;
            case 'E':
                pchT = PszCalcEnvVar(static_cast<EnvType>((uint16_t)*pParams >> 8 & 0xff & 0xff), *pParams & 0xff);
                goto FinishString;
            case 'I':
                pchT = PszGetCompressedString(static_cast<StringId>(idsDecreased + *pParams));
                goto FinishString;
            case 'i':
                c = CchSprintf(szBuf, PCTD, *pParams);
            DoInt:
                pch = PchAppendMessageText(pch, szBuf);
                pParams++;
                break;
            case 'L':
            case 'l':
                pchT = PszPlayerName(*pParams & 0xf, *pszFormat == 'L', (*pParams & 0x10) != 0, (*pParams & 0x20) != 0, (*pParams & 0xc0) >> 6, NULL);
                goto FinishString;
            case 'Z':
                w = *pParams;
                if (w == 0)
                    break;
                if (!((w - 1) & w)) {
                    c = 0;
                    for (; (w & 1) == 0; w >>= 1) {
                        c++;
                    }
                    pchT = PszPlayerName(c, FALSE, TRUE, TRUE, 0, NULL);
                    goto FinishString;
                }
                cOut = 0;
                i = 0;
                while (i < game.cPlayer) {
                    if (w & 1) {
                        if (cOut > 0) {
                            if (w & 0xfffe) {
                                pch = PchAppendMessageText(pch, ", ");
                            } else {
                                pch = PchAppendMessageText(pch, PszGetCompressedString(idsAnd));
                            }
                        }
                        pchT = PszPlayerName(i, FALSE, TRUE, TRUE, 0, NULL);
                        pch = PchAppendMessageText(pch, pchT);
                        cOut++;
                    }
                    i++;
                    w >>= 1;
                }
                goto DoNothing;
            case 'S':
                if (*pParams == idPlayer)
                    goto DoNothing;
                CchGetString(idsOf2, szBuf);
                pchT = PszPlayerName(*pParams, FALSE, FALSE, FALSE, 0, NULL);
                strcat(szBuf, pchT);
                strcat(szBuf, PszGetCompressedString(idsOrigin));
                pchT = szBuf;
                goto FinishString;
            case 'm':
                iMineral = *pParams;
                pchT = rgszMinerals[iMineral];
                goto FinishString;
            case 'M':
                pchT = rgszMineField[*pParams];
                goto FinishString;
            case 'P':
                if ((double)(int16_t)(*pParams / 100) >= (double)10.0) {
                    c = CchSprintf(szBuf, PCTDPCTPCT, *pParams / 100);
                } else {
                    c = CchSprintf(szBuf, PCTDXPCTDPCTPCT, *pParams / 100, *pParams - *pParams / 100 * 100);
                }
                pch = PchAppendMessageText(pch, szBuf);
                pParams++;
                break;
            case 'p':
            DoPlanet:
                pchT = PszGetPlanetName(*pParams);
            FinishString:
                pch = PchAppendMessageText(pch, pchT);
            DoNothing:
                pParams++;
                break;
            case 'X':
                goto DoNothing;
            case 'F':
                pchT = PszFleetNameFromWord(*pParams);
                goto FinishString;
            case 's':
            DoFleet:
                w = *pParams | 0x8000;
                pchT = PszGetFleetName(w);
                goto FinishString;
            case 'j':
                pchT = PszGetCompressedString(static_cast<StringId>(idsEnergy + *pParams));
                goto FinishString;
            case 'k':
                part.hs.grhst = static_cast<HullSlotType>(*pParams);
                pParams++;
                part.hs.iItem = *pParams;
                if (FLookupPart(&part) <= mdPartAvailInvalid) {
                }
                pch = PchAppendMessageText(pch, part.pcom->szName);
                pParams++;
                break;
            case 'g':
            LThingName:
                pchT = PszGetThingName(*pParams);
                goto FinishString;
            case 'G':
                w = *pParams;
                c = CchGetString(static_cast<StringId>(idsMineField + w), szBuf);
                pch = PchAppendMessageText(pch, szBuf);
                pParams++;
                break;
            case 'n':
                if (*pParams == -2) {
                    pParams++;
                    goto LThingName;
                }
                if (*pParams != -1) {
                    pchT = PszGetLocName(grobjNone, -1, *pParams, pParams[1]);
                    pParams++;
                    goto FinishString;
                }
                pParams++;
                /* fallthrough */
            case 'o':
                if (*pParams & 0x8000)
                    goto DoFleet;
                goto DoPlanet;
            case 'O':
                w = (uint16_t)*pParams >> 9 & 0xf;
                pchT = PszPlayerName(w, FALSE, FALSE, FALSE, 0, NULL);
                goto FinishString;
            case 'u':
                c = CchSprintf(szBuf, "%u", *pParams);
                pch = PchAppendMessageText(pch, szBuf);
                pParams++;
                break;
            case 'U':
            case 'V':
            case 'v':
                l = (int32_t)((uint32_t)pParams[1] << 0x10) | (uint32_t)*pParams;
                pParams += 2;
                c = CchSprintf(szBuf, PCTLD, l);
                pch = PchAppendMessageText(pch, szBuf);
                if (*pszFormat == 'v')
                    break;
                if (*pszFormat == 'V') {
                    iMineral = *pParams;
                }
                /* With no preceding \m or \V, iMineral is -1; the original read before vrgszUnits. */
                if (iMineral < 0 || iMineral >= 6)
                    break;
                pchT = vrgszUnits[iMineral];
                pch = PchAppendMessageText(pch, pchT);
                break;
            case 'z':
                c = *pParams >> 5;
                w = *pParams & 0x1f;
                if (w >= 16) {
                    lpshdef = rglpshdefSB[c] + (w - 16);
                } else {
                    lpshdef = rglpshdef[c] + w;
                }
                if (c != idPlayer) {
                    pchT = PszPlayerName(c, FALSE, FALSE, TRUE, 0, NULL);
                    CchSprintf(szBuf, "%s %s", pchT, lpshdef->hul.szClass);
                } else {
                    strcpy(szBuf, lpshdef->hul.szClass);
                }
                pch = PchAppendMessageText(pch, szBuf);
                pParams++;
                break;
            default:
                *pch++ = *pszFormat;
            }
            continue;
        }
    }
    *pch = 0;
    szMsgBuf[CchUtf8Prefix(szMsgBuf, sizeof(szMsgBuf) - 1)] = 0;
    return szMsgBuf;
}

char *PszFormatMessage(MessageId idm, int16_t *pParams) { return PszFormatString(PszGetCompressedMessage(idm), pParams); }

char *PszFormatIds(StringId ids, int16_t *pParams) { return PszFormatString(PszGetCompressedString(ids), pParams); }

int16_t FRemovePlayerMessage(int16_t iPlr, MessageId iMsg, MsgGoto iObj) {
    uint8_t *lpbMax;
    uint8_t *lpb;
    int16_t  cDel;

    cDel = 0;
    lpb = (uint8_t *)lpMsg;
    lpbMax = lpb + imemMsgCur;
    for (; lpb < lpbMax; lpb += cbWireMsgTurn + MtvFromLp(lpb).cbParams()) {
        if (MtvFromLp(lpb).iPlr() == iPlr && MtvFromLp(lpb).msghdr().iMsg() == iMsg && MtvFromLp(lpb).msghdr().wGoto() == iObj) {
            cDel++;
            MtvFromLp(lpb).msghdr().SetIMsg(0x1ff);
        }
    }
    return cDel;
}

int16_t FFindPlayerMessage(int16_t iPlr, int16_t iMsg, MsgGoto iObj) {
    uint8_t *lpbMax;
    uint8_t *lpb;

    lpb = (uint8_t *)lpMsg;
    lpbMax = lpb + imemMsgCur;
    for (; lpb < lpbMax; lpb += cbWireMsgTurn + MtvFromLp(lpb).cbParams()) {
        if (MtvFromLp(lpb).iPlr() == iPlr && MtvFromLp(lpb).msghdr().iMsg() == iMsg && MtvFromLp(lpb).msghdr().wGoto() == iObj) {
            return TRUE;
        }
    }
    return FALSE;
}

void MarkPlanetsPlayerLost(int16_t iPlayer) {
    uint8_t *lpbMax;
    PLANET  *lppl;
    uint8_t *lpbT;
    uint16_t w;
    uint8_t *lpb;

    lpb = (uint8_t *)lpMsg;
    lpbMax = lpb + imemMsgCur;
    for (; lpb < lpbMax; lpb += cbWireMsgTurn + MtvFromLp(lpb).cbParams()) {
        if (MtvFromLp(lpb).iPlr() == iPlayer) {
            switch (MtvFromLp(lpb).msghdr().iMsg()) {
            case 0x8f:
                w = MtvFromLp(lpb).msghdr().grWord();
                lpbT = lpb + (((w & 1) == 1) + 6);
                w = !(w & 2) ? *lpbT : WMsgParam(lpbT);
                goto LLookupPlanet;
            case 7:
            case 0x23:
            case 0x40:
                w = MtvFromLp(lpb).msghdr().wGoto();
            LLookupPlanet:
                lppl = LpplFromId(w);
                if (lppl) {
                    MarkPlanet(lppl, iPlayer, detSome);
                }
                break;
            case 0x91:
            case 0x92:
            case 0x93:
            case 0x94:
            case 0x95:
            case 0x96:
            case 0x97:
            case 0x98:
            case 0x99:
            case 0x9a:
            case 0x9b:
            case 0x9c:
            case 0x9d:
            case 0x9e:
            case 0x9f:
            case 0xa0:
            case 0xa1:
            case 0xa2:
            case 0xa3:
            case 0xa4:
            case 0xa5:
            case 0xa6:
            case 0xa7:
            case 0xa8:
            case 0x113:
            case 0x114:
            case 0x115:
            case 0x116:
                w = MtvFromLp(lpb).msghdr().wGoto();
            }
            continue;
        }
    }
    return;
}

void MarkPlayersThatSentMsgs(int16_t iPlayer) {
    MSGPLR *lpmp;

    if (iPlayer != iplrNone) {
        for (lpmp = vlpmsgplrOut; lpmp; lpmp = lpmp->lpmsgplrNext) {
            if ((lpmp->iPlrTo == 0 && lpmp->iPlrFrom != iPlayer) || (lpmp->iPlrTo - 1 == iPlayer && !rgplr[lpmp->iPlrFrom].fInclude)) {
                rgplr[lpmp->iPlrFrom].fInclude = TRUE;
                rgplr[lpmp->iPlrFrom].det = detSome;
            }
        }
    }
    return;
}

void WritePlayerMessages(int16_t iPlayer) {
    uint8_t *lpbMax;
    uint8_t  rgb[1024];
    int16_t  cbMsg;
    MSGPLR  *lpmp;
    uint8_t *lpb;

    cbMsg = 0;
    if (iPlayer != iplrNone) {
        lpb = (uint8_t *)lpMsg;
        lpbMax = lpb + imemMsgCur;
        for (; lpb < lpbMax; lpb += cbWireMsgTurn + MtvFromLp(lpb).cbParams()) {
            if (cbMsg + 20 >= 1024) {
                WriteRt(rtMsg, cbMsg, rgb);
                cbMsg = 0;
            }
            if (MtvFromLp(lpb).iPlr() == iPlayer && MtvFromLp(lpb).msghdr().iMsg() != 0x1ff) {
                memmove(&rgb[cbMsg], MtvFromLp(lpb).Header(), cbWireMsgHdr + MtvFromLp(lpb).cbParams());
                cbMsg += cbWireMsgHdr + MtvFromLp(lpb).cbParams();
            }
        }
        if (cbMsg != 0) {
            WriteRt(rtMsg, cbMsg, rgb);
        }
        for (lpmp = vlpmsgplrOut; lpmp; lpmp = lpmp->lpmsgplrNext) {
            if ((lpmp->iPlrTo == 0 && lpmp->iPlrFrom != iPlayer) || lpmp->iPlrTo - 1 == iPlayer) {
                WriteRtPlrMsg(lpmp);
            }
        }
    }
    return;
}

void ResetMessages() {
    imemMsgCur = 0;
    iMsgCur = imsgNone;
    cMsg = 0;
    iMsgSendCur = 0;
    memset(bitfMsgSent, 0, 49);
    memset(bitfMsgFiltered, 0, 49);
    vlpmsgplrIn = NULL;
    vlpmsgplrOut = NULL;
    vcmsgplrIn = 0;
    vcmsgplrOut = 0;
    return;
}

void WriteRtPlrMsg(MSGPLR *lpmp) {
    uint8_t rgb[1024];
    RTPLRMSG msg{};

    msg.lpmsgplrNext = 0;
    msg.iPlrFrom = lpmp->iPlrFrom;
    msg.iPlrTo = lpmp->iPlrTo;
    msg.iInRe = lpmp->iInRe;
    msg.cLen = lpmp->cLen;
    FEncodePlrMsgPrefix(msg, std::as_writable_bytes(std::span(rgb)).first(cbWirePlrMsg));
    memcpy(rgb + cbWirePlrMsg, lpmp->rgbMsg(), abs(lpmp->cLen));
    WriteRt(rtPlrMsg, cbWirePlrMsg + abs(lpmp->cLen), rgb);
    return;
}

static NATIVEPLAYERMESSAGEVIEW PmvCur() noexcept {
    return {std::as_writable_bytes(std::span(rgbCur)), 0};
}

// Returns the rtPlrMsg record in rgbCur as a new message, or NULL if the
// record is too short for its header or text.
MSGPLR *LpmsgplrFromRt() {
    MSGPLR *lpmp;
    char szText[1001];
    int16_t cOut;
    int16_t cLen;
    const char *pszEnd;

    /* NATIVE: skip records too short for their header or text; see WIN16-PARITY.md. */
    if (hdrCur.cb < cbWirePlrMsg || abs(PmvCur().cLen()) + cbWirePlrMsg > hdrCur.cb)
        return NULL;
    cLen = PmvCur().cLen();
    if (cLen >= 0) {
        cOut = sizeof(szText) - 1;
        if (!FDecompressUserString((char *)PmvCur().RgbMsg(), cLen, szText, &cOut)) return NULL;
    } else {
        pszEnd = (const char *)memchr(PmvCur().RgbMsg(), 0, -cLen);
        if (!pszEnd || pszEnd - (char *)PmvCur().RgbMsg() >= (int)sizeof(szText) ||
            !FUtf8Valid((char *)PmvCur().RgbMsg(), (size_t)(pszEnd - (char *)PmvCur().RgbMsg()))) return NULL;
    }
    lpmp = LpmsgplrAlloc(sizeof(MSGPLR) + hdrCur.cb - cbWirePlrMsg);
    lpmp->lpmsgplrNext = NULL;
    lpmp->iPlrFrom = PmvCur().iPlrFrom();
    lpmp->iPlrTo = PmvCur().iPlrTo();
    lpmp->iInRe = PmvCur().iInRe();
    lpmp->cLen = PmvCur().cLen();
    memcpy(lpmp->rgbMsg(), PmvCur().RgbMsg(), hdrCur.cb - cbWirePlrMsg);
    return lpmp;
}

void ReadPlayerMessages() {
    uint8_t *lpbMax;
    int16_t  iMax;
    int16_t  fOOM;
    uint8_t *lpmh;
    uint16_t imemMsgT;
    int16_t  i;
    MSGPLR  *lpmp;
    MSGPLR **lppmp;
    uint8_t *lpb;
    uint16_t u;

    imemMsgT = 0;
    fOOM = FALSE;
    lpb = (uint8_t *)lpMsg + imemMsgCur;
    while (hdrCur.rt == rtMsg) {
        if (hdrCur.cb != 0 && (uint16_t)(imemMsgCur + imemMsgT) < (uint16_t)(0xffc8 - hdrCur.cb)) {
            memmove(lpb + imemMsgT, rgbCur, hdrCur.cb);
            imemMsgT += hdrCur.cb;
        }
        ReadRt();
    }
    imemMsgCur += imemMsgT;
    lpbMax = lpb + imemMsgT;
    while (lpb < lpbMax) {
        lpmh = lpb;
        bitfMsgSent[MhvFromLp(lpmh).iMsg() >> 3] = (bitfMsgSent[MhvFromLp(lpmh).iMsg() >> 3] & ~(1 << (MhvFromLp(lpmh).iMsg() & 7))) | 1 << (MhvFromLp(lpmh).iMsg() & 7);
        cMsg++;
        u = MhvFromLp(lpmh).grWord();
        lpb += 4;
        iMax = rgcMsgArgs[MhvFromLp(lpmh).iMsg()];
        for (i = 0; i < iMax; i++) {
            lpb += 1 + ((u & 1) == 1);
            u >>= 1;
        }
    }
    for (lppmp = &vlpmsgplrIn; *lppmp; lppmp = &(*lppmp)->lpmsgplrNext) {
    }

    while (hdrCur.rt == rtPlrMsg) {
        if (!fOOM) {
            try {
                *lppmp = LpmsgplrFromRt();
                if (*lppmp) {
                    lpmp = *lppmp;
                    lppmp = &lpmp->lpmsgplrNext;
                    vcmsgplrIn++;
                }
            } catch (const StarsError &error) {
                if (error.mdError != starsErrorMemory) throw;
                fOOM = TRUE;
            }
        }
        ReadRt();
    }
    iMsgCur = imsgNone;
    iMsgCur = IMsgNext(FALSE);
    return;
}

char *PszGetCompressedMessage(MessageId idm) {
    int16_t  iBuild;
    int16_t  iNibble;
    int16_t  i;
    int16_t  iLen;
    uint8_t *pchLen;
    int16_t  iOffset;
    char    *pszOut;
    int16_t  fHigh;
    uint8_t *pch;
    int16_t  iChunk;

    iNibble = 0;
    if (idm == iLastMsgGet) {
        return szLastMsgGet;
    }
    iChunk = idm >> 6;
    iOffset = idm & 0x3f;
    pch = (uint8_t *)&aMSGCmpr[aiMSGChunkOffset[iChunk]];
    pchLen = &acMSG[iChunk * 64];
    i = 0;
    while (i < iOffset) {
        iNibble += *pchLen;
        i++;
        pchLen++;
    }
    pch += iNibble >> 1;
    iLen = *pchLen;
    fHigh = (iNibble & 1) == 0;
    pszOut = szLastMsgGet;
    iBuild = 0;
    while (iLen-- != 0) {
        if (fHigh) {
            i = *pch >> 4;
        } else {
            i = *pch++ & 0xf;
        }
        fHigh = fHigh == 0;
        iBuild += i;
        if (i != 15) {
            *pszOut = rgMSGLookupTable[iBuild];
            pszOut++;
            iBuild = 0;
        }
    }
    *pszOut = 0;
    return szLastMsgGet;
}

void SetFilteringGroups(MessageId idm, int16_t fSet) {
    int16_t   i;
    MessageId idmPair; // the other message of a pair filtered together

    fSet = fSet == 0;
    bitfMsgFiltered[idm >> 3] = (bitfMsgFiltered[idm >> 3] & ~(1 << (idm & 7))) | (fSet == 0) << (idm & 7);
    switch (idm) {
    case idmHaveBuiltFactory:
    case idmHaveBuiltFactories:
        idmPair = static_cast<MessageId>(idm ^ idmHaveBuiltFactory ^ idmHaveBuiltFactories);
        bitfMsgFiltered[idmPair >> 3] = (bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
        break;
    case idmHaveBuiltMine:
    case idmHaveBuiltMines:
        idmPair = static_cast<MessageId>(idm ^ idmHaveBuiltMine ^ idmHaveBuiltMines);
        bitfMsgFiltered[idmPair >> 3] = (bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
        break;
    case idmHaveBuiltDefenseOutpost:
    case idmHaveBuiltDefenseOutposts:
        idmPair = static_cast<MessageId>(idm ^ idmHaveBuiltDefenseOutpost ^ idmHaveBuiltDefenseOutposts);
        bitfMsgFiltered[idmPair >> 3] = (bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
        break;
    default:
        if ((int16_t)idm >= idmHasLoaded && (int16_t)idm <= idmHasBeamed2) {
            for (i = 43; i <= 46; i++) {
                bitfMsgFiltered[i >> 3] = (bitfMsgFiltered[i >> 3] & ~(1 << (i & 7))) | (fSet == 0) << (i & 7);
            }
        } else {
            switch (idm) {
            case idmStarbaseHasBuiltNew:
            case idmStarbaseHasBuiltNewShips:
                idmPair = static_cast<MessageId>(idm ^ idmStarbaseHasBuiltNew ^ idmStarbaseHasBuiltNewShips);
                bitfMsgFiltered[idmPair >> 3] = (bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
                break;
            case idmSuccessfullyTransferred:
            case idmSuccessfullyTransferred2:
                idmPair = static_cast<MessageId>(idm ^ idmSuccessfullyTransferred ^ idmSuccessfullyTransferred2);
                bitfMsgFiltered[idmPair >> 3] = (bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
                break;
            case idmSuccessfullyReceived:
            case idmSuccessfullyReceived2:
                idmPair = static_cast<MessageId>(idm ^ idmSuccessfullyReceived ^ idmSuccessfullyReceived2);
                bitfMsgFiltered[idmPair >> 3] = (bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
                break;
            case idmAttemptedTransferSuccessfullyReceived:
            case idmAttemptedTransferColonistsSuccessfullyReceivedRe:
                idmPair = static_cast<MessageId>(idm ^ idmAttemptedTransferSuccessfullyReceived ^ idmAttemptedTransferColonistsSuccessfullyReceivedRe);
                bitfMsgFiltered[idmPair >> 3] = (bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
                break;
            case idmReceivedHoweverSentRemainderLostSpace:
            case idmReceivedHoweverColonistsSentRemainsOtherColonist:
                idmPair = static_cast<MessageId>(idm ^ idmReceivedHoweverSentRemainderLostSpace ^ idmReceivedHoweverColonistsSentRemainsOtherColonist);
                bitfMsgFiltered[idmPair >> 3] = (bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
                break;
            case idmAttemptedTransferNoneSuccessfullyReceived:
            case idmAttemptedTransferNoneColonistsSuccessfullyReceiv:
                idmPair = static_cast<MessageId>(idm ^ idmAttemptedTransferNoneSuccessfullyReceived ^ idmAttemptedTransferNoneColonistsSuccessfullyReceiv);
                bitfMsgFiltered[idmPair >> 3] = (bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
                break;
            case idmAttemptedReceiveHoweverLostDeepSpace:
            case idmAttemptedReceiveHoweverNoneColonistsSuccessfully:
                idmPair = static_cast<MessageId>(idm ^ idmAttemptedReceiveHoweverLostDeepSpace ^ idmAttemptedReceiveHoweverNoneColonistsSuccessfully);
                bitfMsgFiltered[idmPair >> 3] = (bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
                break;
            default:
                if ((int16_t)idm >= idmHasBombedKillingColonists && (int16_t)idm <= idmHasBombedKillingColonistsDestroyingDefensesFacto) {
                    for (i = 96; i <= 100; i++) {
                        bitfMsgFiltered[i >> 3] = (bitfMsgFiltered[i >> 3] & ~(1 << (i & 7))) | (fSet == 0) << (i & 7);
                    }
                } else if ((int16_t)idm >= idmHasBombedKillingColonists2 && (int16_t)idm <= idmHasBombedKillingColonistsDestroyingDefensesFacto3) {
                    for (i = 106; i <= 110; i++) {
                        bitfMsgFiltered[i >> 3] = (bitfMsgFiltered[i >> 3] & ~(1 << (i & 7))) | (fSet == 0) << (i & 7);
                    }
                } else if (idm == idmHasLoaded2 || idm == idmHasBeamed3) {
                    idmPair = static_cast<MessageId>(idm ^ idmHasLoaded2 ^ idmHasBeamed3);
                    bitfMsgFiltered[idmPair >> 3] = (bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
                } else if ((int16_t)idm >= idmBattleTookPlaceDestroyedTakingDamage && (int16_t)idm <= idmBattleTookPlaceInvolvingRacesLostForces2) {
                    for (i = 145; i <= 168; i++) {
                        bitfMsgFiltered[i >> 3] = (bitfMsgFiltered[i >> 3] & ~(1 << (i & 7))) | (fSet == 0) << (i & 7);
                    }
                }
            }
        }
    }
    return;
}
