#include "common.h"
#include <algorithm>
#include "appstate.h"
#include "text_io.h"
#include "presentation_io.h"
#include "turn_io.h"
#include "part_query_io.h"
#include "record_io.h"

std::int16_t IMsgNext(APPSESSION &app, std::int16_t fFilteredOnly) {
    std::int16_t   i;
    MessageId idm;

    i = app.state.iMsgCur;
    if (app.state.fViewFilteredMsg && !fFilteredOnly) {
        if (i < app.state.cMsg + app.state.vcmsgplrIn - 1) {
            return i + 1;
        }
        return imsgNone;
    }
    do {
        i++;
        if (i < app.state.cMsg) {
            idm = static_cast<MessageId>(PresentationIO().pfnMessageId(app, i));
        } else {
            if (i < app.state.cMsg + app.state.vcmsgplrIn) {
                return i;
            }
            return imsgNone;
        }
    } while ((((app.state.bitfMsgFiltered[idm >> 3] & 1 << (idm & 7)) == 0) ^ fFilteredOnly) == 0);
    return i;
}

std::int16_t IMsgPrev(APPSESSION &app, std::int16_t fFilteredOnly) {
    std::int16_t   i;
    MessageId idm;

    i = app.state.iMsgCur;
    if (app.state.fViewFilteredMsg && !fFilteredOnly) {
        if (i > 0) {
            return i - 1;
        }
        return imsgNone;
    }
    if (i > app.state.cMsg) {
        return i - 1;
    }
    do {
        i--;
        if (i < 0) {
            return imsgNone;
        }
        idm = static_cast<MessageId>(PresentationIO().pfnMessageId(app, i));
    } while ((((app.state.bitfMsgFiltered[idm >> 3] & 1 << (idm & 7)) == 0) ^ fFilteredOnly) == 0);
    return i;
}

std::int16_t FSendPlrMsg2(APPSESSION &app, std::int16_t iPlr, MessageId iMsg, MsgGoto iObj, std::int16_t p1, std::int16_t p2) { return TurnIO().pfnMessage(app, iPlr, iMsg, iObj, p1, p2, 0, 0, 0, 0, 0); }

std::int16_t FSendPlrMsg(APPSESSION &app, std::int16_t iPlr, MessageId iMsg, MsgGoto iObj, std::int16_t p1, std::int16_t p2, std::int16_t p3, std::int16_t p4, std::int16_t p5, std::int16_t p6, std::int16_t p7) {
    std::uint8_t  rgbWork[40];
    std::int16_t  cbMsg;
    std::uint8_t *lpb;

    cbMsg = PackageUpMsg(app, rgbWork, iPlr, iMsg, iObj, p1, p2, p3, p4, p5, p6, p7);
    if (cbMsg <= 0) {
        if (cbMsg == 0) {
            return 1;
        }
        return 0;
    }
    lpb = static_cast<std::uint8_t *>(app.state.lpMsg) + app.state.imemMsgCur;
    std::memmove(lpb, rgbWork, cbMsg);
    app.state.imemMsgCur += cbMsg;
    app.state.cMsg++;
    return 1;
}

std::int16_t FSendPrependedPlrMsg(APPSESSION &app, std::int16_t iPlr, MessageId iMsg, MsgGoto iObj, std::int16_t p1, std::int16_t p2, std::int16_t p3, std::int16_t p4, std::int16_t p5, std::int16_t p6, std::int16_t p7) {
    std::uint8_t rgbWork[40];
    std::int16_t cbMsg;

    cbMsg = PackageUpMsg(app, rgbWork, iPlr, iMsg, iObj, p1, p2, p3, p4, p5, p6, p7);
    if (cbMsg <= 0) {
        if (cbMsg == 0) {
            return 1;
        }
        return 0;
    }
    std::memmove(static_cast<std::uint8_t *>(app.state.lpMsg) + cbMsg, app.state.lpMsg, app.state.imemMsgCur);
    std::memmove(app.state.lpMsg, rgbWork, cbMsg);
    app.state.imemMsgCur += cbMsg;
    app.state.cMsg++;
    return 1;
}

std::int16_t PackageUpMsg(APPSESSION &app, std::uint8_t *pb, std::int16_t iPlr, MessageId iMsg, MsgGoto iObj, std::int16_t p1, std::int16_t p2, std::int16_t p3, std::int16_t p4, std::int16_t p5, std::int16_t p6,
                     std::int16_t p7) {
    std::int16_t *pi;
    std::int16_t  i;
    std::uint16_t grbit;
    std::uint8_t *lpmt;
    MSGTURN mt{};
    std::uint8_t *lpb;
    std::uint8_t *lpbBase;
    std::int16_t  rgArgs[7];

    if (iPlr == iplrNone) {
        return 0;
    }
    if (app.state.rgplr[iPlr].fAi && app.state.rgplr[iPlr].idAi != idAiMaid) {
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
    if (static_cast<std::uint16_t>((app.state.imemMsgCur + 20)) > 0xffc8) {
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

std::int16_t FSendPlrMsg2XGen(APPSESSION &app, std::int16_t fPrepend, MessageId iMsg, MsgGoto iObj, std::int16_t p1, std::int16_t p2) {
    std::uint8_t  rgb[64];
    std::int16_t *pi;
    std::int16_t  i;
    std::uint16_t grbit;
    std::uint8_t *pb;
    std::uint16_t cSize;
    std::uint8_t *pmsghdr;
    MSGHDR mh{};
    std::int16_t  rgArgs[2];

    if (static_cast<std::uint16_t>((app.state.imemMsgCur + 20)) > 0xffc8) {
        return 0;
    }
    pb = rgb;
    pmsghdr = pb;
    mh.iMsg = iMsg;
    app.state.bitfMsgSent[iMsg >> 3] = (app.state.bitfMsgSent[iMsg >> 3] & ~(1 << (iMsg & 7))) | 1 << (iMsg & 7);
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
        std::memmove(static_cast<std::uint8_t *>(app.state.lpMsg) + cSize, app.state.lpMsg, app.state.imemMsgCur);
        std::memmove(app.state.lpMsg, rgb, cSize);
    } else {
        std::memmove(static_cast<std::uint8_t *>(app.state.lpMsg) + app.state.imemMsgCur, rgb, cSize);
    }
    app.state.imemMsgCur += cSize;
    app.state.cMsg++;
    app.state.iMsgCur = imsgNone;
    app.state.iMsgCur = IMsgNext(app, 0);
    return 1;
}

std::int16_t IdmGetMessageN(APPSESSION &app, std::int16_t iMsg) {
    MSGBIG mb;

    if (!FGetNMsgbig(app, static_cast<MessageId>(iMsg), &mb)) {
        return -1;
    }
    return mb.iMsg;
}

std::int16_t FGetNMsgbig(APPSESSION &app, MessageId iMsg, MSGBIG *pmb) {
    std::uint8_t *lpbMax;
    std::int16_t  iMax;
    std::uint8_t *lpmh;
    std::int16_t  i;
    std::uint8_t *lpb;
    std::uint16_t u;

    if (static_cast<std::int16_t>(iMsg) < idmColonistsDroppedMassacredGroundTroops || static_cast<std::int16_t>(iMsg) >= app.state.cMsg) {
        return 0;
    }
    lpb = static_cast<std::uint8_t *>(app.state.lpMsg);
    lpbMax = lpb + app.state.imemMsgCur;
    while (lpb < lpbMax) {
        lpmh = lpb;
        u = MhvFromLp(app, lpmh).grWord();
        lpb += 4;
        if (iMsg == idmColonistsDroppedMassacredGroundTroops) {
            pmb->iMsg = MhvFromLp(app, lpmh).iMsg();
            pmb->wGoto = MhvFromLp(app, lpmh).wGoto();
        }
        iMax = rgcMsgArgs[MhvFromLp(app, lpmh).iMsg()];
        for (i = 0; i < iMax; i++) {
            if (iMsg == idmColonistsDroppedMassacredGroundTroops) {
                pmb->rgParam[i] = !(u & 1) ? *lpb : WMsgParam(lpb);
            }
            lpb += 1 + ((u & 1) == 1);
            u >>= 1;
        }
        if (static_cast<std::int16_t>(iMsg) <= idmColonistsDroppedMassacredGroundTroops)
            break;
        iMsg = static_cast<MessageId>(iMsg - 1);
    }
    return 1;
}

char *PszGetMessageN(APPSESSION &app, std::int16_t iMsg) {
    MSGBIG mb;
    char  *psz;

    if (!FGetNMsgbig(app, static_cast<MessageId>(iMsg), &mb)) {
        app.state.szMsgBuf[0] = 0;
        return app.state.szMsgBuf;
    }
    psz = PszFormatMessage(app, static_cast<MessageId>(mb.iMsg), mb.rgParam);
    return psz;
}

// Formatting may expand bounded names or paths beyond the message buffer.
static char *PchAppendMessageText(APPSESSION &app, char *pch, const char *psz) {
    std::size_t cch;
    std::string_view szText(psz);

    cch = CchUtf8Prefix(szText, static_cast<std::size_t>((app.state.szMsgBuf + sizeof(app.state.szMsgBuf) - 1 - pch)));
    std::memmove(pch, psz, cch);
    pch[cch] = 0;
    return pch + cch;
}

char *PszFormatString(APPSESSION &app, const char *pszFormat, std::int16_t *pParamsReal) {
    std::array<char, cchCatalogText> szCatalogPchT{};
    std::array<char, 50> szPlayerName{};
    std::int16_t  iMineral;
    std::int16_t  cOut;
    std::int16_t  c;
    std::int16_t  i;
    std::int16_t *pParams;
    const char *pchT;
    char     szBuf[480];
    std::uint16_t w;
    char    *pch;
    PART     part;
    std::int32_t  l;
    SHDEF   *lpshdef;

    iMineral = -1;
    pParams = pParamsReal;
    pch = app.state.szMsgBuf;
    for (; *pszFormat != 0 && pch < app.state.szMsgBuf + sizeof(app.state.szMsgBuf) - 1; pszFormat++) {
        if (*pszFormat != '\\') {
            *pch++ = *pszFormat;
        } else {
            pszFormat++;
            if (*pszFormat == 0) break;
            switch (*pszFormat) {
            case 'w':
                pch = PchAppendMessageText(app, pch, app.state.szWork);
                break;
            case 'f':
            case 'h':
            case 'r':
            case 't':
            case 'y':
                pch = PchAppendMessageText(app, pch, app.state.szBase);
                switch (*pszFormat) {
                case 'f':
                    if (app.state.idPlayer != iplrNone) {
                        c = CchSprintf(szBuf, ".x%d", app.state.idPlayer + 1);
                        goto DoInt;
                    }
                    /* fallthrough */
                case 't':
                    if (app.state.idPlayer != iplrNone) {
                        c = CchSprintf(szBuf, ".m%d", app.state.idPlayer + 1);
                        goto DoInt;
                    }
                    /* fallthrough */
                case 'h':
                    pch = PchAppendMessageText(app, pch, ".hst");
                    break;
                case 'r':
                    c = CchSprintf(szBuf, ".h%d", app.state.idPlayer + 1);
                    goto DoInt;
                case 'y':
                    pch = PchAppendMessageText(app, pch, ".xy");
                }
                break;
            case 'e':
                pchT = app.state.rgszPlanetAttr[*pParams];
                goto FinishString;
            case 'E':
                pchT = PszCalcEnvVar(app, static_cast<EnvType>(static_cast<std::uint16_t>(*pParams) >> 8 & 0xff & 0xff), *pParams & 0xff);
                goto FinishString;
            case 'I':
                szCatalogPchT = TextIO().pfnCompressed(app, static_cast<StringId>(idsDecreased + *pParams));
                pchT = szCatalogPchT.data();
                goto FinishString;
            case 'i':
                c = CchSprintf(szBuf, app.state.PCTD, *pParams);
            DoInt:
                pch = PchAppendMessageText(app, pch, szBuf);
                pParams++;
                break;
            case 'L':
            case 'l':
                szPlayerName = TextIO().pfnPlayerName(app, *pParams & 0xf, *pszFormat == 'L', (*pParams & 0x10) != 0, (*pParams & 0x20) != 0, (*pParams & 0xc0) >> 6, nullptr);
                pchT = szPlayerName.data();
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
                    szPlayerName = TextIO().pfnPlayerName(app, c, 0, 1, 1, 0, nullptr);
                    pchT = szPlayerName.data();
                    goto FinishString;
                }
                cOut = 0;
                i = 0;
                while (i < app.state.game.cPlayer) {
                    if (w & 1) {
                        if (cOut > 0) {
                            if (w & 0xfffe) {
                                pch = PchAppendMessageText(app, pch, ", ");
                            } else {
                                pch = PchAppendMessageText(app, pch, TextIO().pfnCompressed(app, idsAnd).data());
                            }
                        }
                        szPlayerName = TextIO().pfnPlayerName(app, i, 0, 1, 1, 0, nullptr);
                        pchT = szPlayerName.data();
                        pch = PchAppendMessageText(app, pch, pchT);
                        cOut++;
                    }
                    i++;
                    w >>= 1;
                }
                goto DoNothing;
            case 'S':
                if (*pParams == app.state.idPlayer)
                    goto DoNothing;
                TextIO().pfnCchGetString(app, idsOf2, std::span{szBuf});
                szPlayerName = TextIO().pfnPlayerName(app, *pParams, 0, 0, 0, 0, nullptr);
                pchT = szPlayerName.data();
                CchAppendUtf8Prefix(std::span{szBuf}, pchT);
                CchAppendUtf8Prefix(std::span{szBuf}, TextIO().pfnCompressed(app, idsOrigin).data());
                pchT = szBuf;
                goto FinishString;
            case 'm':
                iMineral = *pParams;
                pchT = app.state.rgszMinerals[iMineral];
                goto FinishString;
            case 'M':
                pchT = app.state.rgszMineField[*pParams];
                goto FinishString;
            case 'P':
                if (static_cast<double>(static_cast<std::int16_t>((*pParams / 100))) >= static_cast<double>(10.0)) {
                    c = CchSprintf(szBuf, app.state.PCTDPCTPCT, *pParams / 100);
                } else {
                    c = CchSprintf(szBuf, app.state.PCTDXPCTDPCTPCT, *pParams / 100, *pParams - *pParams / 100 * 100);
                }
                pch = PchAppendMessageText(app, pch, szBuf);
                pParams++;
                break;
            case 'p':
            DoPlanet:
                pchT = TextIO().pfnPlanetName(app, *pParams);
            FinishString:
                pch = PchAppendMessageText(app, pch, pchT);
            DoNothing:
                pParams++;
                break;
            case 'X':
                goto DoNothing;
            case 'F':
                pchT = PszFleetNameFromWord(app, *pParams);
                goto FinishString;
            case 's':
            DoFleet:
                w = *pParams | 0x8000;
                pchT = PszGetFleetName(app, w);
                goto FinishString;
            case 'j':
                szCatalogPchT = TextIO().pfnCompressed(app, static_cast<StringId>(idsEnergy + *pParams));
                pchT = szCatalogPchT.data();
                goto FinishString;
            case 'k':
                part.hs.grhst = static_cast<HullSlotType>(*pParams);
                pParams++;
                part.hs.iItem = *pParams;
                if (PartQueryIO().pfnLookup(app, &part) <= mdPartAvailInvalid) {
                }
                pch = PchAppendMessageText(app, pch, part.pcom().szName());
                pParams++;
                break;
            case 'g':
            LThingName:
                pchT = PszGetThingName(app, *pParams);
                goto FinishString;
            case 'G':
                w = *pParams;
                c = TextIO().pfnCchGetString(app, static_cast<StringId>(idsMineField + w), std::span{szBuf});
                pch = PchAppendMessageText(app, pch, szBuf);
                pParams++;
                break;
            case 'n':
                if (*pParams == -2) {
                    pParams++;
                    goto LThingName;
                }
                if (*pParams != -1) {
                    pchT = PszGetLocName(app, grobjNone, -1, *pParams, pParams[1]);
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
                w = static_cast<std::uint16_t>(*pParams) >> 9 & 0xf;
                szPlayerName = TextIO().pfnPlayerName(app, w, 0, 0, 0, 0, nullptr);
                pchT = szPlayerName.data();
                goto FinishString;
            case 'u':
                c = CchSprintf(szBuf, "%u", *pParams);
                pch = PchAppendMessageText(app, pch, szBuf);
                pParams++;
                break;
            case 'U':
            case 'V':
            case 'v':
                l = static_cast<std::int32_t>((static_cast<std::uint32_t>(pParams[1]) << 0x10)) | static_cast<std::uint32_t>(*pParams);
                pParams += 2;
                c = CchSprintf(szBuf, app.state.PCTLD, l);
                pch = PchAppendMessageText(app, pch, szBuf);
                if (*pszFormat == 'v')
                    break;
                if (*pszFormat == 'V') {
                    iMineral = *pParams;
                }
                /* With no preceding \m or \V, iMineral is -1; the original read before vrgszUnits. */
                if (iMineral < 0 || iMineral >= 6)
                    break;
                pchT = app.state.vrgszUnits[iMineral];
                pch = PchAppendMessageText(app, pch, pchT);
                break;
            case 'z':
                c = *pParams >> 5;
                w = *pParams & 0x1f;
                if (w >= 16) {
                    lpshdef = app.state.rglpshdefSB[c] + (w - 16);
                } else {
                    lpshdef = app.state.rglpshdef[c] + w;
                }
                if (c != app.state.idPlayer) {
                    szPlayerName = TextIO().pfnPlayerName(app, c, 0, 0, 1, 0, nullptr);
                    pchT = szPlayerName.data();
                    CchSprintf(szBuf, "%s %s", pchT, lpshdef->hul.szClass);
                } else {
                    CchCopyUtf8Prefix(std::span{szBuf}, lpshdef->hul.szClass);
                }
                pch = PchAppendMessageText(app, pch, szBuf);
                pParams++;
                break;
            default:
                *pch++ = *pszFormat;
            }
            continue;
        }
    }
    *pch = 0;
    app.state.szMsgBuf[CchUtf8Prefix(app.state.szMsgBuf, sizeof(app.state.szMsgBuf) - 1)] = 0;
    return app.state.szMsgBuf;
}

char *PszFormatMessage(APPSESSION &app, MessageId idm, std::int16_t *pParams) { return PszFormatString(app, PszGetCompressedMessage(app, idm), pParams); }

char *PszFormatIds(APPSESSION &app, StringId ids, std::int16_t *pParams) { return PszFormatString(app, TextIO().pfnCompressed(app, ids).data(), pParams); }

std::int16_t FRemovePlayerMessage(APPSESSION &app, std::int16_t iPlr, MessageId iMsg, MsgGoto iObj) {
    std::uint8_t *lpbMax;
    std::uint8_t *lpb;
    std::int16_t  cDel;

    cDel = 0;
    lpb = static_cast<std::uint8_t *>(app.state.lpMsg);
    lpbMax = lpb + app.state.imemMsgCur;
    for (; lpb < lpbMax; lpb += cbWireMsgTurn + MtvFromLp(app, lpb).cbParams()) {
        if (MtvFromLp(app, lpb).iPlr() == iPlr && MtvFromLp(app, lpb).msghdr().iMsg() == iMsg && MtvFromLp(app, lpb).msghdr().wGoto() == iObj) {
            cDel++;
            MtvFromLp(app, lpb).msghdr().SetIMsg(0x1ff);
        }
    }
    return cDel;
}

std::int16_t FFindPlayerMessage(APPSESSION &app, std::int16_t iPlr, std::int16_t iMsg, MsgGoto iObj) {
    std::uint8_t *lpbMax;
    std::uint8_t *lpb;

    lpb = static_cast<std::uint8_t *>(app.state.lpMsg);
    lpbMax = lpb + app.state.imemMsgCur;
    for (; lpb < lpbMax; lpb += cbWireMsgTurn + MtvFromLp(app, lpb).cbParams()) {
        if (MtvFromLp(app, lpb).iPlr() == iPlr && MtvFromLp(app, lpb).msghdr().iMsg() == iMsg && MtvFromLp(app, lpb).msghdr().wGoto() == iObj) {
            return 1;
        }
    }
    return 0;
}

void MarkPlanetsPlayerLost(APPSESSION &app, std::int16_t iPlayer) {
    std::uint8_t *lpbMax;
    PLANET  *lppl;
    std::uint8_t *lpbT;
    std::uint16_t w;
    std::uint8_t *lpb;

    lpb = static_cast<std::uint8_t *>(app.state.lpMsg);
    lpbMax = lpb + app.state.imemMsgCur;
    for (; lpb < lpbMax; lpb += cbWireMsgTurn + MtvFromLp(app, lpb).cbParams()) {
        if (MtvFromLp(app, lpb).iPlr() == iPlayer) {
            switch (MtvFromLp(app, lpb).msghdr().iMsg()) {
            case 0x8f:
                w = MtvFromLp(app, lpb).msghdr().grWord();
                lpbT = lpb + (((w & 1) == 1) + 6);
                w = !(w & 2) ? *lpbT : WMsgParam(lpbT);
                goto LLookupPlanet;
            case 7:
            case 0x23:
            case 0x40:
                w = MtvFromLp(app, lpb).msghdr().wGoto();
            LLookupPlanet:
                lppl = LpplFromId(app, w);
                if (lppl) {
                    MarkPlanet(app, lppl, iPlayer, detSome);
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
                w = MtvFromLp(app, lpb).msghdr().wGoto();
            }
            continue;
        }
    }
    return;
}

void MarkPlayersThatSentMsgs(APPSESSION &app, std::int16_t iPlayer) {
    MSGPLR *lpmp;

    if (iPlayer != iplrNone) {
        for (lpmp = app.state.vlpmsgplrOut; lpmp; lpmp = lpmp->lpmsgplrNext) {
            if ((lpmp->iPlrTo == 0 && lpmp->iPlrFrom != iPlayer) || (lpmp->iPlrTo - 1 == iPlayer && !app.state.rgplr[lpmp->iPlrFrom].fInclude)) {
                app.state.rgplr[lpmp->iPlrFrom].fInclude = 1;
                app.state.rgplr[lpmp->iPlrFrom].det = detSome;
            }
        }
    }
    return;
}

void WritePlayerMessages(APPSESSION &app, std::int16_t iPlayer) {
    std::uint8_t *lpbMax;
    std::uint8_t  rgb[1024];
    std::int16_t  cbMsg;
    MSGPLR  *lpmp;
    std::uint8_t *lpb;

    cbMsg = 0;
    if (iPlayer != iplrNone) {
        lpb = static_cast<std::uint8_t *>(app.state.lpMsg);
        lpbMax = lpb + app.state.imemMsgCur;
        for (; lpb < lpbMax; lpb += cbWireMsgTurn + MtvFromLp(app, lpb).cbParams()) {
            if (cbMsg + 20 >= 1024) {
                RecordIO().pfnWriteRt(app, rtMsg, cbMsg, rgb);
                cbMsg = 0;
            }
            if (MtvFromLp(app, lpb).iPlr() == iPlayer && MtvFromLp(app, lpb).msghdr().iMsg() != 0x1ff) {
                std::memmove(&rgb[cbMsg], MtvFromLp(app, lpb).Header(), cbWireMsgHdr + MtvFromLp(app, lpb).cbParams());
                cbMsg += cbWireMsgHdr + MtvFromLp(app, lpb).cbParams();
            }
        }
        if (cbMsg != 0) {
            RecordIO().pfnWriteRt(app, rtMsg, cbMsg, rgb);
        }
        for (lpmp = app.state.vlpmsgplrOut; lpmp; lpmp = lpmp->lpmsgplrNext) {
            if ((lpmp->iPlrTo == 0 && lpmp->iPlrFrom != iPlayer) || lpmp->iPlrTo - 1 == iPlayer) {
                WriteRtPlrMsg(app, lpmp);
            }
        }
    }
    return;
}

void ResetMessages(APPSESSION &app) {
    app.state.imemMsgCur = 0;
    app.state.iMsgCur = imsgNone;
    app.state.cMsg = 0;
    app.state.iMsgSendCur = 0;
    std::ranges::fill(app.state.bitfMsgSent, 0);
    std::ranges::fill(app.state.bitfMsgFiltered, 0);
    app.state.vlpmsgplrIn = nullptr;
    app.state.vlpmsgplrOut = nullptr;
    app.state.vcmsgplrIn = 0;
    app.state.vcmsgplrOut = 0;
    return;
}

void WriteRtPlrMsg(APPSESSION &app, MSGPLR *lpmp) {
    std::uint8_t rgb[1024];
    RTPLRMSG msg{};

    msg.lpmsgplrNext = 0;
    msg.iPlrFrom = lpmp->iPlrFrom;
    msg.iPlrTo = lpmp->iPlrTo;
    msg.iInRe = lpmp->iInRe;
    msg.cLen = lpmp->cLen;
    FEncodePlrMsgPrefix(msg, std::as_writable_bytes(std::span(rgb)).first(cbWirePlrMsg));
    std::memcpy(rgb + cbWirePlrMsg, lpmp->rgbMsg(), std::abs(lpmp->cLen));
    RecordIO().pfnWriteRt(app, rtPlrMsg, cbWirePlrMsg + std::abs(lpmp->cLen), rgb);
    return;
}

static NATIVEPLAYERMESSAGEVIEW PmvCur(APPSESSION &app) noexcept {
    return {std::as_writable_bytes(std::span(app.state.rgbCur)), 0};
}

// Returns the rtPlrMsg record in rgbCur as a new message, or NULL if the
// record is too short for its header or text.
MSGPLR *LpmsgplrFromRt(APPSESSION &app) {
    MSGPLR *lpmp;
    char szText[1001];
    std::int16_t cOut;
    std::int16_t cLen;
    const char *pszEnd;

    /* NATIVE: skip records too short for their header or text; see WIN16-PARITY.md. */
    if (app.state.hdrCur.cb < cbWirePlrMsg || std::abs(PmvCur(app).cLen()) + cbWirePlrMsg > app.state.hdrCur.cb)
        return nullptr;
    cLen = PmvCur(app).cLen();
    if (cLen >= 0) {
        cOut = sizeof(szText) - 1;
        if (!FDecompressUserString(app, std::as_bytes(std::span(PmvCur(app).RgbMsg(), static_cast<std::size_t>(cLen))), std::span(szText, static_cast<std::size_t>(cOut) + 1))) return nullptr;
    } else {
        pszEnd = static_cast<const char *>(std::memchr(PmvCur(app).RgbMsg(), 0, -cLen));
        if (!pszEnd || pszEnd - reinterpret_cast<char *>(PmvCur(app).RgbMsg()) >= static_cast<int>(sizeof(szText)) ||
            !FUtf8Valid(reinterpret_cast<char *>(PmvCur(app).RgbMsg()), static_cast<std::size_t>((pszEnd - reinterpret_cast<char *>(PmvCur(app).RgbMsg()))))) return nullptr;
    }
    lpmp = LpmsgplrAlloc(app, sizeof(MSGPLR) + app.state.hdrCur.cb - cbWirePlrMsg);
    lpmp->lpmsgplrNext = nullptr;
    lpmp->iPlrFrom = PmvCur(app).iPlrFrom();
    lpmp->iPlrTo = PmvCur(app).iPlrTo();
    lpmp->iInRe = PmvCur(app).iInRe();
    lpmp->cLen = PmvCur(app).cLen();
    std::memcpy(lpmp->rgbMsg(), PmvCur(app).RgbMsg(), app.state.hdrCur.cb - cbWirePlrMsg);
    return lpmp;
}

void ReadPlayerMessages(APPSESSION &app) {
    std::uint8_t *lpbMax;
    std::int16_t  iMax;
    std::int16_t  fOOM;
    std::uint8_t *lpmh;
    std::uint16_t imemMsgT;
    std::int16_t  i;
    MSGPLR  *lpmp;
    MSGPLR **lppmp;
    std::uint8_t *lpb;
    std::uint16_t u;

    imemMsgT = 0;
    fOOM = 0;
    lpb = static_cast<std::uint8_t *>(app.state.lpMsg) + app.state.imemMsgCur;
    while (app.state.hdrCur.rt == rtMsg) {
        if (app.state.hdrCur.cb != 0 && static_cast<std::uint16_t>((app.state.imemMsgCur + imemMsgT)) < static_cast<std::uint16_t>((0xffc8 - app.state.hdrCur.cb))) {
            std::memmove(lpb + imemMsgT, app.state.rgbCur, app.state.hdrCur.cb);
            imemMsgT += app.state.hdrCur.cb;
        }
        RecordIO().pfnReadRt(app);
    }
    app.state.imemMsgCur += imemMsgT;
    lpbMax = lpb + imemMsgT;
    while (lpb < lpbMax) {
        lpmh = lpb;
        app.state.bitfMsgSent[MhvFromLp(app, lpmh).iMsg() >> 3] = (app.state.bitfMsgSent[MhvFromLp(app, lpmh).iMsg() >> 3] & ~(1 << (MhvFromLp(app, lpmh).iMsg() & 7))) | 1 << (MhvFromLp(app, lpmh).iMsg() & 7);
        app.state.cMsg++;
        u = MhvFromLp(app, lpmh).grWord();
        lpb += 4;
        iMax = rgcMsgArgs[MhvFromLp(app, lpmh).iMsg()];
        for (i = 0; i < iMax; i++) {
            lpb += 1 + ((u & 1) == 1);
            u >>= 1;
        }
    }
    for (lppmp = &app.state.vlpmsgplrIn; *lppmp; lppmp = &(*lppmp)->lpmsgplrNext) {
    }

    while (app.state.hdrCur.rt == rtPlrMsg) {
        if (!fOOM) {
            try {
                *lppmp = LpmsgplrFromRt(app);
                if (*lppmp) {
                    lpmp = *lppmp;
                    lppmp = &lpmp->lpmsgplrNext;
                    app.state.vcmsgplrIn++;
                }
            } catch (const StarsError &error) {
                if (error.mdError != starsErrorMemory) throw;
                fOOM = 1;
            }
        }
        RecordIO().pfnReadRt(app);
    }
    app.state.iMsgCur = imsgNone;
    app.state.iMsgCur = IMsgNext(app, 0);
    return;
}

char *PszGetCompressedMessage(APPSESSION &app, MessageId idm) {
    std::int16_t  iBuild;
    std::int16_t  iNibble;
    std::int16_t  i;
    std::int16_t  iLen;
    std::uint8_t *pchLen;
    std::int16_t  iOffset;
    char    *pszOut;
    std::int16_t  fHigh;
    std::uint8_t *pch;
    std::int16_t  iChunk;

    iNibble = 0;
    if (idm == app.state.iLastMsgGet) {
        return app.state.szLastMsgGet;
    }
    iChunk = idm >> 6;
    iOffset = idm & 0x3f;
    pch = reinterpret_cast<std::uint8_t *>(&aMSGCmpr[aiMSGChunkOffset[iChunk]]);
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
    pszOut = app.state.szLastMsgGet;
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
    return app.state.szLastMsgGet;
}

void SetFilteringGroups(APPSESSION &app, MessageId idm, std::int16_t fSet) {
    std::int16_t   i;
    MessageId idmPair; // the other message of a pair filtered together

    fSet = fSet == 0;
    app.state.bitfMsgFiltered[idm >> 3] = (app.state.bitfMsgFiltered[idm >> 3] & ~(1 << (idm & 7))) | (fSet == 0) << (idm & 7);
    switch (idm) {
    case idmHaveBuiltFactory:
    case idmHaveBuiltFactories:
        idmPair = static_cast<MessageId>(idm ^ idmHaveBuiltFactory ^ idmHaveBuiltFactories);
        app.state.bitfMsgFiltered[idmPair >> 3] = (app.state.bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
        break;
    case idmHaveBuiltMine:
    case idmHaveBuiltMines:
        idmPair = static_cast<MessageId>(idm ^ idmHaveBuiltMine ^ idmHaveBuiltMines);
        app.state.bitfMsgFiltered[idmPair >> 3] = (app.state.bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
        break;
    case idmHaveBuiltDefenseOutpost:
    case idmHaveBuiltDefenseOutposts:
        idmPair = static_cast<MessageId>(idm ^ idmHaveBuiltDefenseOutpost ^ idmHaveBuiltDefenseOutposts);
        app.state.bitfMsgFiltered[idmPair >> 3] = (app.state.bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
        break;
    default:
        if (static_cast<std::int16_t>(idm) >= idmHasLoaded && static_cast<std::int16_t>(idm) <= idmHasBeamed2) {
            for (i = 43; i <= 46; i++) {
                app.state.bitfMsgFiltered[i >> 3] = (app.state.bitfMsgFiltered[i >> 3] & ~(1 << (i & 7))) | (fSet == 0) << (i & 7);
            }
        } else {
            switch (idm) {
            case idmStarbaseHasBuiltNew:
            case idmStarbaseHasBuiltNewShips:
                idmPair = static_cast<MessageId>(idm ^ idmStarbaseHasBuiltNew ^ idmStarbaseHasBuiltNewShips);
                app.state.bitfMsgFiltered[idmPair >> 3] = (app.state.bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
                break;
            case idmSuccessfullyTransferred:
            case idmSuccessfullyTransferred2:
                idmPair = static_cast<MessageId>(idm ^ idmSuccessfullyTransferred ^ idmSuccessfullyTransferred2);
                app.state.bitfMsgFiltered[idmPair >> 3] = (app.state.bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
                break;
            case idmSuccessfullyReceived:
            case idmSuccessfullyReceived2:
                idmPair = static_cast<MessageId>(idm ^ idmSuccessfullyReceived ^ idmSuccessfullyReceived2);
                app.state.bitfMsgFiltered[idmPair >> 3] = (app.state.bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
                break;
            case idmAttemptedTransferSuccessfullyReceived:
            case idmAttemptedTransferColonistsSuccessfullyReceivedRe:
                idmPair = static_cast<MessageId>(idm ^ idmAttemptedTransferSuccessfullyReceived ^ idmAttemptedTransferColonistsSuccessfullyReceivedRe);
                app.state.bitfMsgFiltered[idmPair >> 3] = (app.state.bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
                break;
            case idmReceivedHoweverSentRemainderLostSpace:
            case idmReceivedHoweverColonistsSentRemainsOtherColonist:
                idmPair = static_cast<MessageId>(idm ^ idmReceivedHoweverSentRemainderLostSpace ^ idmReceivedHoweverColonistsSentRemainsOtherColonist);
                app.state.bitfMsgFiltered[idmPair >> 3] = (app.state.bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
                break;
            case idmAttemptedTransferNoneSuccessfullyReceived:
            case idmAttemptedTransferNoneColonistsSuccessfullyReceiv:
                idmPair = static_cast<MessageId>(idm ^ idmAttemptedTransferNoneSuccessfullyReceived ^ idmAttemptedTransferNoneColonistsSuccessfullyReceiv);
                app.state.bitfMsgFiltered[idmPair >> 3] = (app.state.bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
                break;
            case idmAttemptedReceiveHoweverLostDeepSpace:
            case idmAttemptedReceiveHoweverNoneColonistsSuccessfully:
                idmPair = static_cast<MessageId>(idm ^ idmAttemptedReceiveHoweverLostDeepSpace ^ idmAttemptedReceiveHoweverNoneColonistsSuccessfully);
                app.state.bitfMsgFiltered[idmPair >> 3] = (app.state.bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
                break;
            default:
                if (static_cast<std::int16_t>(idm) >= idmHasBombedKillingColonists && static_cast<std::int16_t>(idm) <= idmHasBombedKillingColonistsDestroyingDefensesFacto) {
                    for (i = 96; i <= 100; i++) {
                        app.state.bitfMsgFiltered[i >> 3] = (app.state.bitfMsgFiltered[i >> 3] & ~(1 << (i & 7))) | (fSet == 0) << (i & 7);
                    }
                } else if (static_cast<std::int16_t>(idm) >= idmHasBombedKillingColonists2 && static_cast<std::int16_t>(idm) <= idmHasBombedKillingColonistsDestroyingDefensesFacto3) {
                    for (i = 106; i <= 110; i++) {
                        app.state.bitfMsgFiltered[i >> 3] = (app.state.bitfMsgFiltered[i >> 3] & ~(1 << (i & 7))) | (fSet == 0) << (i & 7);
                    }
                } else if (idm == idmHasLoaded2 || idm == idmHasBeamed3) {
                    idmPair = static_cast<MessageId>(idm ^ idmHasLoaded2 ^ idmHasBeamed3);
                    app.state.bitfMsgFiltered[idmPair >> 3] = (app.state.bitfMsgFiltered[idmPair >> 3] & ~(1 << (idmPair & 7))) | (fSet == 0) << (idmPair & 7);
                } else if (static_cast<std::int16_t>(idm) >= idmBattleTookPlaceDestroyedTakingDamage && static_cast<std::int16_t>(idm) <= idmBattleTookPlaceInvolvingRacesLostForces2) {
                    for (i = 145; i <= 168; i++) {
                        app.state.bitfMsgFiltered[i >> 3] = (app.state.bitfMsgFiltered[i >> 3] & ~(1 << (i & 7))) | (fSet == 0) << (i & 7);
                    }
                }
            }
        }
    }
    return;
}
