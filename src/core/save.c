#include "common.h"

void WriteOrders(FLEET *lpfl) {
    int16_t cord;
    ORDER  *lpord;

    if (lpfl->cord != 0) {
        cord = lpfl->cord;
        lpord = lpfl->lpplord->rgord;
        for (; cord != 0; cord--) {
            if (lpord->grTask == grTaskNone) {
                WriteRt(rtOrderB, 8, lpord);
            } else {
                WriteRt(rtOrderA, 18, lpord);
            }
            lpord++;
        }
    }
    return;
}

void WriteRtPlr(PLAYER *pplr, uint8_t *pbStore) {
    uint8_t  rgb[264];
    int16_t  i;
    uint8_t *pb;
    int16_t  cOut;

    if (!pbStore) {
        pbStore = rgb;
    }
    if (pplr->fDead) {
        pplr->det = detAll;
    }
    memmove(pbStore, pplr, sizeof(PLAYER));
    if (pplr->det == detAll) {
        for (i = 15; i >= 0 && pplr->rgmdRelation[i] == 0; i--) {
        }
        i++;
        pb = pbStore + offsetof(PLAYER, rgmdRelation);
        *pb++ = i;
        memmove(pb, pplr->rgmdRelation, i);
        pb += i;
    } else {
        pb = pbStore + offsetof(PLAYER, idPlanetHome);
    }
    cOut = 31;
    if (pplr->szName[0] != 0 && FCompressUserString(pplr->szName, pb + 1, &cOut)) {
        *pb = cOut;
        pb += 1 + cOut;
    } else {
        strcpy(pb + 1, pplr->szName);
        *pb = 0;
        pb += 2 + strlen(pplr->szName);
    }
    cOut = 31;
    if (pplr->szNames[0] != 0 && FCompressUserString(pplr->szNames, pb + 1, &cOut)) {
        *pb = cOut;
        pb += 1 + cOut;
    } else {
        strcpy(pb + 1, pplr->szNames);
        *pb = 0;
        pb += 2 + strlen(pplr->szNames);
    }
    WriteRt(rtPlr, pb - pbStore, pbStore);
    return;
}

void WriteRtShDef(SHDEF *lpshdef, uint8_t **ppbStore) {
    uint8_t  rgb[147];
    char     szHulName[32];
    uint8_t *pb;
    int16_t  cOut;

    ((RTSHDEF *)rgb)->ihuldef = lpshdef->hul.ihuldef;
    ((RTSHDEF *)rgb)->wFlags = lpshdef->wFlags;
    ((RTSHDEF *)rgb)->chs = lpshdef->hul.chs;
    ((RTSHDEF *)rgb)->ibmp = lpshdef->hul.ibmp;
    if (lpshdef->det == detAll) {
        ((RTSHDEF *)rgb)->dp = lpshdef->hul.dp;
        ((RTSHDEF *)rgb)->turn = lpshdef->turn;
        ((RTSHDEF *)rgb)->cBuilt = lpshdef->cBuilt;
        ((RTSHDEF *)rgb)->cExist = lpshdef->cExist;
        pb = (uint8_t *)&((RTSHDEF *)rgb)->rghs;
        memmove(pb, lpshdef->hul.rghs, ((RTSHDEF *)rgb)->chs * 4);
        pb += ((RTSHDEF *)rgb)->chs * 4;
    } else {
        ((RTSHDEF *)rgb)->wtEmpty = lpshdef->hul.wtEmpty;
        pb = &((RTSHDEF *)rgb)->chs;
    }
    if (lpshdef->det == detAll) {
        strcpy(szHulName, lpshdef->hul.szClass);
    } else {
        strcpy(szHulName, LphuldefFromId(lpshdef->hul.ihuldef)->hul.szClass);
    }
    cOut = 31;
    if (szHulName[0] != 0 && FCompressUserString(szHulName, pb + 1, &cOut)) {
        *pb = cOut;
        pb += 1 + cOut;
    } else {
        strcpy(pb + 1, szHulName);
        *pb = 0;
        pb += 2 + strlen(szHulName);
    }
    if (ppbStore) {
        memmove(*ppbStore, rgb, pb - rgb);
        *ppbStore += pb - rgb;
    } else {
        WriteRt(rtShDef, pb - rgb, rgb);
    }
    return;
}

int16_t FWriteDataFile(char *pszFileBase, int16_t iPlayer, int16_t fAppend) {
    PLAYER   plrT;
    int16_t  iMax;
    FLEET   *lpflT;
    int16_t  fNoAutoTrack;
    BTLPLAN *lpbtlplan;
    int16_t  j;
    jmp_buf *penvMemSav;
    int16_t  i;
    ORDER   *lpord;
    THING   *lpth;
    FLEET   *lpfl;
    jmp_buf  env;
    int16_t  iord;
    SHDEF   *lpshdef;
    THING   *lpthMac;
    THING    th;
    int16_t  fRet;
    PLANET  *lpplT;
    SCAN     scan;
    MdTarget mdTarget;
    FLEET   *lpflTarget;
    POINT16  pt;
    int32_t  dy;
    int16_t  iflT;
    FLEET   *lpflBest;
    int16_t  fFoundIdeal;
    int32_t  dx;
    int32_t  lBest;
    int32_t  l;
    PLANET   pl;

    fRet = TRUE;
    SetVisiblePlanFleet(iPlayer);
    if (gd.fGeneratingTurn && iPlayer != iplrNone) {
        for (i = 0; i < cFleet; i++) {
            lpfl = rglpfl[i];
            if (!rglpfl[i])
                break;
            if (!lpfl->fDead && lpfl->iplr == iPlayer) {
                for (j = 0; j < 16 && lpfl->rgcsh[j] == 0; j++) {
                }
                if (j == 16) {
                    lpfl->fDead = TRUE;
                } else {
                    lpord = lpfl->lpplord->rgord;
                    if (lpord->grobj == grobjFleet) {
                        if (FFindNearestObject(lpord->pt, grobjPlanet | mdExact, &scan)) {
                            lpord->grobj = grobjPlanet;
                            lpord->id = scan.idpl;
                        } else {
                            lpord->grobj = grobjOther;
                            lpord->id = 0;
                        }
                    }
                    if (lpord->grTask == grTaskNone && lpfl->cord > 1 && lpord[1].grTask == grTaskPatrol) {
                        lpord->grTask = grTaskPatrol;
                        lpord->tptl.iWarp = lpord[1].tptl.iWarp;
                        lpord->tptl.iDist = lpord[1].tptl.iDist;
                    }
                    if (lpord->grTask == grTaskPatrol && (lpfl->cord <= 1 || lpord[1].grobj != grobjFleet)) {
                        lpflBest = NULL;
                        lBest = 100000000;
                        fFoundIdeal = FALSE;
                        if (lpfl->idPlanet == idPlanetDeepSpace && lpfl->cord >= 2 && lpfl->fRepOrders) {
                            pt = lpord[1].pt;
                        } else {
                            pt = lpfl->pt;
                        }
                        lpbtlplan = rglpbtlplan[lpfl->iPlayer] + lpfl->iplan;
                        mdTarget = lpbtlplan->mdTarget1;
                        for (iflT = 0; iflT < cFleet; iflT++) {
                            lpflTarget = rglpfl[iflT];
                            if (!rglpfl[iflT])
                                break;
                            if (lpflTarget->fInclude && lpflTarget->iPlayer != iPlayer) {
                                dx = (int16_t)(lpflTarget->pt.x - pt.x);
                                dy = (int16_t)(lpflTarget->pt.y - pt.y);
                                l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                                if (((!fFoundIdeal && !lpflTarget->fMark) || (l < lBest && (!fFoundIdeal || !lpflTarget->fMark))) &&
                                    (FMatchTarget(lpflTarget, mdTarget, FALSE) && FAttackPlayer(lpfl, lpflTarget->iPlayer))) {
                                    lpflBest = lpflTarget;
                                    lBest = l;
                                    if (!lpflTarget->fMark) {
                                        fFoundIdeal = TRUE;
                                    }
                                }
                            }
                        }
                        if (fFoundIdeal && !gd.fTutorial) {
                            lpflBest->fMark = TRUE;
                        }
                        j = 50 * lpord->tptl.iDist + 50;
                        if (j == 550) {
                            j = 10000;
                        }
                        if (lpflBest && lBest != 0 && lBest <= (int32_t)(uint32_t)(j * j)) {
                            if (lpfl->lpplord->iordMax <= lpfl->cord + 1) {
                                lpfl->lpplord = (PLORD *)LpplReAlloc((PL *)lpfl->lpplord, lpfl->cord + 2);
                                lpord = lpfl->lpplord->rgord;
                            }
                            if (lpfl->cord > 1) {
                                memmove(lpord + 2, lpord + 1, (lpfl->cord - 1) * sizeof(ORDER));
                            }
                            if (lpfl->cord == 1) {
                                memset(lpord + 1, 0, sizeof(ORDER));
                                lpord[1].fValidTask = TRUE;
                                lpord[1].grTask = grTaskPatrol;
                                lpord[1].tptl = lpord->tptl;
                                if (lpord[1].tptl.iWarp == 0) {
                                    lpord[1].iWarp = IFindIdealWarp(lpfl, FALSE);
                                } else {
                                    lpord[1].iWarp = lpord[1].tsell.iPlrX;
                                }
                                if (lpfl->fRepOrders) {
                                    lpord[2] = *lpord;
                                    lpord[2].iWarp = IFindIdealWarp(lpfl, FALSE);
                                    lpfl->cord++;
                                    lpfl->lpplord->iordMac++;
                                }
                            } else if (lpord[1].tsell.iPlrX == 0) {
                                lpord[1].iWarp = IFindIdealWarp(lpfl, FALSE);
                            } else {
                                lpord[1].iWarp = lpord[1].tsell.iPlrX;
                            }
                            lpord[1].pt = lpflBest->pt;
                            lpord[1].id = lpflBest->id;
                            lpord[1].grobj = grobjFleet;
                            lpfl->cord++;
                            lpfl->lpplord->iordMac++;
                            FSendPlrMsg(iPlayer, idmPatrollingHasTargetedIntercept, lpfl->id | 0x8000, lpfl->id, lpflBest->id, 0, 0, 0, 0, 0);
                        }
                    }
                    if (lpord->grTask != grTaskXfer && lpfl->cord > 1) {
                        for (iord = 1; iord < lpfl->cord; iord++) {
                            if (lpord[iord].grobj == grobjThing) {
                                lpth = LpthFromId(lpord[iord].id);
                                if (!lpth || (lpth->ith == ithMysteryTrader && !lpth->tht.fInclude) ||
                                    (lpth->ith == ithMinefield && !(1 << iPlayer & lpth->thm.grbitPlrNow)) ||
                                    (lpth->ith == ithWormhole && !lpth->thw.fInclude)) {
                                    if (lpth && lpth->ith == ithWormhole) {
                                        FSendPlrMsg2(lpfl->iPlayer, idmWormholeHeadingHasVanishedOrdersHaveChanged, lpfl->id | 0x8000, lpfl->id, 0);
                                    } else if (lpth && lpth->ith == ithMysteryTrader) {
                                        FSendPlrMsg2(lpfl->iPlayer, idmMysteryTraderHeadingHasVanishedOrdersHave, lpfl->id | 0x8000, lpfl->id, 0);
                                    } else if (lpth && lpth->ith == ithMinefield) {
                                        FSendPlrMsg2(lpfl->iPlayer, idmMineFieldHeadingHasVanishedOrdersHave, lpfl->id | 0x8000, lpfl->id, 0);
                                    }
                                    lpord[iord].grobj = grobjOther;
                                    lpord[iord].id = iord;
                                }
                            } else if (lpord[iord].grobj == grobjFleet) {
                                fNoAutoTrack = lpord[iord].fNoAutoTrack;
                                if (fNoAutoTrack) {
                                    lpord[iord].fNoAutoTrack = FALSE;
                                }
                                lpflT = LpflFromId(lpord[iord].id);
                                if (!lpflT || lpflT->fDead) {
                                    FSendPlrMsg(iPlayer, idmSWaypointAppearsHaveDestroyedHasDisappeared, lpfl->id | 0x8000, lpfl->id, lpord[iord].id, 0, 0, 0,
                                                0, 0);
                                FixupCoords:
                                    lpord[iord].grobj = grobjOther;
                                    lpord[iord].id = iord;
                                    if (FFindNearestObject(lpord[iord].pt, grobjPlanet | mdExact, &scan)) {
                                        lpord[iord].grobj = grobjPlanet;
                                        lpord[iord].id = scan.idpl;
                                    }
                                } else {
                                    if (lpflT->fInclude)
                                        continue;
                                    if (lpflT->idPlanet != idPlanetDeepSpace && !fNoAutoTrack) {
                                        FSendPlrMsg(iPlayer, idmFleetTrackingAppearsHaveDuckedBehindOrders, lpfl->id | 0x8000, lpfl->id, lpflT->idPlanet, 0, 0,
                                                    0, 0, 0);
                                        goto FixupCoords;
                                    } else {
                                        FSendPlrMsg(iPlayer, idmFleetTrackingAppearsHaveOutrunRangeScanners, lpfl->id | 0x8000, lpfl->id, 0, 0, 0, 0, 0, 0);
                                        goto FixupCoords;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    MarkPlayersThatSentMsgs(iPlayer);
    MarkPlanetsPlayerLost(iPlayer);
    if (iPlayer == iplrNone) {
        CchSprintf(szWork, "%s.hst", pszFileBase);
    } else {
        CchSprintf(szWork, "%s.m%d", pszFileBase, iPlayer + 1);
    }
    penvMemSav = penvMem;
    penvMem = &env;
    if (setjmp(env) != 0) {
    LFail:
        idPlayer = iPlayer;
        if (fAppend) {
            AlertSz(PszFormatIds(idsUnableUpdateTurnFile, NULL), MB_ICONHAND);
        } else if (iPlayer != iplrNone) {
            AlertSz(PszFormatIds(idsUnableCreateNewTurnFile, NULL), MB_ICONHAND);
        } else {
            idPlayer = iplrNone;
            AlertSz(PszFormatIds(idsUnableCreateHostFile, NULL), MB_ICONHAND);
        }
        idPlayer = iplrNone;
        fRet = FALSE;
        goto FreeUp;
    }
    if (fAppend && FAppendFile(iPlayer))
        goto LAppend;
    if (!FCreateFile(iPlayer == iplrNone ? dtHost : dtTurn, iPlayer, NULL))
        goto LFail;
LAppend:
    WriteBattles(iPlayer);
    for (i = 0; i < game.cPlayer; i++) {
        if (rgplr[i].fInclude || rgplr[i].fDead) {
            /* A Claim Adjuster's file carries other players' habitat (their
               planet values in DrawMineSurvey), which only a full-detail
               record holds. The original raised the live record to full
               detail, sending tech, research, traits, production template
               and relations too, and read rgplr[-1] for the host file. Send a
               full-detail copy with only the habitat filled in. */
            if (iPlayer != iplrNone && i != iPlayer && !rgplr[i].fDead && GetRaceStat(&rgplr[iPlayer], rsMajorAdv) == raTerra) {
                plrT = rgplr[i];
                memset(&plrT.idPlanetHome, 0, (uint8_t *)&plrT.szName - (uint8_t *)&plrT.idPlanetHome);
                memcpy(plrT.rgEnvVar, rgplr[i].rgEnvVar, sizeof(plrT.rgEnvVar));
                memcpy(plrT.rgEnvVarMin, rgplr[i].rgEnvVarMin, sizeof(plrT.rgEnvVarMin));
                memcpy(plrT.rgEnvVarMax, rgplr[i].rgEnvVarMax, sizeof(plrT.rgEnvVarMax));
                plrT.det = detAll;
                WriteRtPlr(&plrT, NULL);
            } else {
                WriteRtPlr(&rgplr[i], NULL);
            }
        }
    }
    if (iPlayer == iplrNone && lSaltCur != 0) {
        WriteRt(rtChgPassword, 4, &lSaltCur);
    }
    WritePlayerMessages(iPlayer);
    i = 0;
    lpplT = lpPlanets;
    while (i < cPlanet) {
        if (lpplT->fInclude) {
            if (lpplT->det == detAll) {
                WritePlanet(lpplT, rtPlanet, FALSE);
                if (lpplT->lpplprod) {
                    WriteRt(rtProdQ, lpplT->lpplprod->iprodMac * 4, lpplT->lpplprod->rgprod);
                }
            } else if (lpplT->det == detObscure) {
                pl = *lpplT;
                lpplT->fStarbase = FALSE;
                lpplT->det = detSome;
                WritePlanet(lpplT, rtPlanetB, FALSE);
                *lpplT = pl;
            } else {
                WritePlanet(lpplT, rtPlanetB, FALSE);
            }
        }
        i++;
        lpplT++;
    }
    for (i = 0; i < game.cPlayer; i++) {
        if (rgplr[i].fInclude) {
            lpshdef = rglpshdef[i];
            for (j = 0; j < 16; j++) {
                if (!lpshdef[j].fFree && lpshdef[j].fInclude) {
                    WriteRtShDef(lpshdef + j, NULL);
                }
            }
        }
    }
    for (i = 0; i < cFleet; i++) {
        lpflT = rglpfl[i];
        if (!rglpfl[i])
            break;
        if (lpflT->fInclude) {
            WriteFleet(lpflT);
        }
    }
    for (i = 0; i < game.cPlayer; i++) {
        if (rgplr[i].fInclude) {
            lpshdef = rglpshdefSB[i];
            for (j = 0; j < 10; j++) {
                if (!lpshdef[j].fFree && lpshdef[j].fInclude) {
                    WriteRtShDef(lpshdef + j, NULL);
                }
            }
        }
    }
    if (iPlayer != iplrNone && vlprgScoreX) {
        for (i = 0; i < game.cPlayer; i++) {
            if (gd.fGameOverMan || i == iPlayer || rgplr[i].fDead || (game.fVisScores && game.turn >= 20)) {
                WriteRt(rtScore, 24, vlprgScoreX + i);
            }
        }
    }
    i = 0;
    lpth = lpThings;
    lpthMac = lpThings + cThing;
    for (; lpth < lpthMac; lpth++) {
        if (iPlayer == iplrNone || (iPlayer == lpth->iplr && lpth->ith != ithMineralPacket && lpth->ith != ithMysteryTrader && lpth->ith != ithWormhole) ||
            (lpth->ith == ithMinefield && (1 << iPlayer & lpth->thm.grbitPlrNow)) || (lpth->ith == ithMineralPacket && lpth->thp.fInclude) ||
            (lpth->ith == ithMysteryTrader && lpth->tht.fInclude) || (lpth->ith == ithWormhole && lpth->thw.fInclude)) {
            i++;
        }
    }
    if (i > 0) {
        WriteRt(rtThing, 2, &i);
        lpth = lpThings;
        lpthMac = lpThings + cThing;
        for (; lpth < lpthMac; lpth++) {
            if (iPlayer == iplrNone || (iPlayer == lpth->iplr && lpth->ith != ithMineralPacket && lpth->ith != ithMysteryTrader && lpth->ith != ithWormhole) ||
                (lpth->ith == ithMinefield && (1 << iPlayer & lpth->thm.grbitPlrNow)) || (lpth->ith == ithMineralPacket && lpth->thp.fInclude) ||
                (lpth->ith == ithMysteryTrader && lpth->tht.fInclude) || (lpth->ith == ithWormhole && lpth->thw.fInclude)) {
                if (iPlayer == iplrNone) {
                    WriteRt(rtThing, 18, lpth);
                } else {
                    /* A player's file keeps only that player's bit of each
                       per-player mask, and not the part the Mystery Trader
                       carries. The original wrote the whole record, telling
                       every player who else had seen these things. */
                    th = *lpth;
                    switch (th.ith) {
                    case ithMinefield:
                        th.thm.grbitPlr &= 1 << iPlayer;
                        th.thm.grbitPlrNow &= 1 << iPlayer;
                        break;
                    case ithWormhole:
                        th.thw.grbitPlr &= 1 << iPlayer;
                        th.thw.grbitPlrTrav &= 1 << iPlayer;
                        break;
                    case ithMysteryTrader:
                        th.tht.grbitPlr &= 1 << iPlayer;
                        th.tht.grbitTrader = grbitTraderNone;
                        break;
                    }
                    WriteRt(rtThing, 18, &th);
                }
            }
        }
    }
    if (iPlayer == iplrNone) {
        i = 0;
        iMax = game.cPlayer;
    } else {
        i = iPlayer;
        iMax = iPlayer + 1;
    }
    for (; i < iMax; i++) {
        lpbtlplan = rglpbtlplan[i];
        j = 0;
        while (j < rgcbtlplan[i]) {
            WriteBattlePlan(lpbtlplan, FALSE);
            j++;
            lpbtlplan++;
        }
    }
    WriteRt(rtEOF, 2, &game.turn);
    StreamClose();
FreeUp:
    SetVisiblePlanFleet(iplrNone);
    return fRet;
}

int16_t FAppendFile(int16_t iPlayer) {
    if (!FMarkFile(8195, iPlayer, mdMarkMulti, TRUE)) {
        return FALSE;
    }
    WriteBOF(iPlayer, 3, TRUE);
    return TRUE;
}

void WriteBattles(int16_t iPlayer) {
    int16_t  ctok;
    int16_t  cbRec;
    PLANET  *lppl;
    int16_t  i;
    FLEET   *lpfl;
    int16_t  cbT;
    uint16_t fPlayerCur;
    BTLREC  *lpbtlrec;
    uint8_t *lpbBattle;
    HB      *lphb;
    BTLDATA *lpbtldata;
    int16_t  cb;
    int16_t  iplr;

    cbT = 0;
    if (iPlayer != iplrNone && lpbBattleLog != lpbBattleCur) {
        lphb = rglphb[11];
        lpbBattle = (uint8_t *)lphb + (sizeof(HB) + 2);
        fPlayerCur = 1 << iPlayer;
        while (lphb) {
            for (lpbtldata = (BTLDATA *)lpbBattle; lphb->ibTop <= sizeof(HB) || lpbtldata->id == 0xffff; lpbtldata = (BTLDATA *)lpbBattle) {
                lphb = lphb->lphbNext;
                if (!lphb) {
                    return;
                }
                lpbBattle = (uint8_t *)lphb + (sizeof(HB) + 2);
            }
            if (lpbtldata->grfPlr & fPlayerCur) {
                for (i = 0; i < game.cPlayer; i++) {
                    if (i != iPlayer && !rgplr[i].fInclude && (1 << i & lpbtldata->grfPlr)) {
                        rgplr[i].fInclude = TRUE;
                        rgplr[i].det = detSome;
                    }
                }
                for (i = 0; i < lpbtldata->ctok; i++) {
                    if (lpbtldata->rgtok[i].iplr != iPlayer) {
                        if (lpbtldata->rgtok[i].grobj == grobjPlanet) {
                            iplr = lpbtldata->rgtok[i].iplr;
                            lppl = LpplFromId(lpbtldata->rgtok[i].id);
                            if (!rglpshdefSB[iplr][lpbtldata->rgtok[i].ishdef - 16].fInclude) {
                                rglpshdefSB[iplr][lpbtldata->rgtok[i].ishdef - 16].fInclude = TRUE;
                                rgplr[iplr].cshdefSB++;
                            }
                            rglpshdefSB[iplr][lpbtldata->rgtok[i].ishdef - 16].det = detAll;
                        } else {
                            lpfl = LpflFromId(lpbtldata->rgtok[i].id);
                            if (lpfl->iPlayer != iPlayer && !rgplr[lpfl->iPlayer].fInclude) {
                                rgplr[lpfl->iPlayer].fInclude = TRUE;
                                rgplr[lpfl->iPlayer].det = detSome;
                            }
                            if (!rglpshdef[lpfl->iPlayer][lpbtldata->rgtok[i].ishdef].fInclude) {
                                rglpshdef[lpfl->iPlayer][lpbtldata->rgtok[i].ishdef].fInclude = TRUE;
                                rgplr[lpfl->iPlayer].cShDef = rgplr[lpfl->iPlayer].cShDef + 1;
                            }
                            rglpshdef[lpfl->iPlayer][lpbtldata->rgtok[i].ishdef].det = detAll;
                            if (!lpfl->fDead) {
                                if (!lpfl->fInclude) {
                                    rgplr[lpfl->iPlayer].cFleet = rgplr[lpfl->iPlayer].cFleet + 1;
                                    lpfl->fInclude = TRUE;
                                    lpfl->det = detNone;
                                }
                                if (lpfl->det < detSome) {
                                    lpfl->det = detSome;
                                }
                            }
                        }
                    }
                }
                if (lpbtldata->idPlanet != idPlanetNone) {
                    lppl = LpplFromId(lpbtldata->idPlanet);
                    MarkPlanet(lppl, iPlayer, detMinimal);
                }
                if (lpbtldata->cbData < 0x400) {
                    WriteRt(rtBtlData, lpbtldata->cbData, lpbBattle);
                    lpbBattle += lpbtldata->cbData;
                } else {
                    cb = lpbtldata->ctok * 29 + 14;
                    lpbtlrec = (BTLREC *)(lpbBattle + cb);
                    if (cb < 1024) {
                        WriteRt(rtBtlData, cb, lpbBattle);
                        lpbBattle += cb;
                    } else {
                        ctok = 34;
                        ctok = ctok >= lpbtldata->ctok ? lpbtldata->ctok : ctok;
                        if (ctok > lpbtldata->ctok) {
                            ctok = lpbtldata->ctok;
                        }
                        WriteRt(rtBtlData, ctok * 29 + 14, lpbBattle);
                        lpbBattle += 14 + 29 * ctok;
                        ctok = lpbtldata->ctok - ctok;
                        while (ctok > 0) {
                            if ((uint16_t)ctok > 35) {
                                WriteRt(rtContinue, 1015, lpbBattle);
                                lpbBattle += 1015;
                                ctok -= 35;
                            } else {
                                WriteRt(rtContinue, ctok * 29, lpbBattle);
                                lpbBattle += 29 * ctok;
                                ctok = 0;
                            }
                        }
                    }
                    cb = lpbtldata->cbData - 14 - lpbtldata->ctok * 29;
                    if (cb < 1024) {
                        WriteRt(rtContinue, cb, lpbtlrec);
                        lpbBattle += cb;
                    } else {
                        while (cb != 0) {
                            cbRec = lpbtlrec->ctok * 8 + 6;
                            if (cbRec >= 1024) {
                                cb -= cbRec;
                                for (; cbRec >= 1024; cbRec -= 1023) {
                                    WriteRt(rtContinue, 1023, lpbtlrec);
                                    lpbtlrec = (BTLREC *)((uint8_t *)lpbtlrec + 0x3ff);
                                }
                                if (cbRec != 0) {
                                    WriteRt(rtContinue, cbRec, lpbtlrec);
                                    lpbtlrec = (BTLREC *)((uint8_t *)lpbtlrec + cbRec);
                                }
                            } else {
                                cbT = 0;
                                do {
                                    cbT += cbRec;
                                    lpbtlrec = (BTLREC *)((uint8_t *)lpbtlrec + cbRec);
                                    cb -= cbRec;
                                    if (cb != 0) {
                                        cbRec = lpbtlrec->ctok * 8 + 6;
                                    }
                                } while (cb != 0 && cbT + cbRec < 1024);
                                WriteRt(rtContinue, cbT, lpbBattle);
                            }
                            lpbBattle = (uint8_t *)lpbtlrec;
                        }
                    }
                }
            } else {
                lpbBattle += lpbtldata->cbData;
            }
        }
    }
    return;
}

void WritePlanet(PLANET *lppl, RecordType rt, int16_t fHistory) {
    uint8_t  bMask;
    uint8_t  rgb[80];
    uint8_t *pbBase;
    int16_t  i;
    uint8_t *pb;

    memset(rgb, 0, 80);
    ((RTPLANET *)rgb)->id = lppl->id;
    ((RTPLANET *)rgb)->iPlayer = lppl->iPlayer;
    ((RTPLANET *)rgb)->det = lppl->det;
    if (rt == rtPlanetB && lppl->det > detSome) {
        ((RTPLANET *)rgb)->det = !fHistory ? 4 : 3;
    }
    ((RTPLANET *)rgb)->fInclude = lppl->fInclude;
    ((RTPLANET *)rgb)->fStarbase = lppl->fStarbase;
    ((RTPLANET *)rgb)->fHomeworld = lppl->fHomeworld;
    ((RTPLANET *)rgb)->fFirstYear = lppl->fFirstYear;
    ((RTPLANET *)rgb)->fRouting = lppl->idRoute != 0;
    pbBase = (uint8_t *)(((RTPLANET *)rgb) + 1);
    pb = pbBase;
    if (((RTPLANET *)rgb)->det <= detMinimal)
        goto LFinishBRecord;

    pb = pbBase + 1;
    bMask = 3;
    i = 0;
    while (i < 3) {
        if (lppl->rgpctMinLevel[i] > 0) {
            *pbBase |= bMask & 0x55;
            *pb++ = lppl->rgpctMinLevel[i];
        }
        i++;
        bMask *= 4;
    }
    i = 0;
    while (i < 3) {
        *pb = lppl->rgMinConc[i];
        i++;
        pb++;
    }
    for (i = 0; i < 3; i++) {
        *pb++ = lppl->rgEnvVar[i];
        if (lppl->rgEnvVar[i] != lppl->rgEnvVarOrig[i]) {
            ((RTPLANET *)rgb)->fIncEVO = TRUE;
        }
    }
    if (((RTPLANET *)rgb)->fIncEVO != 0) {
        for (i = 0; i < 3; i++) {
            *pb++ = lppl->rgEnvVarOrig[i];
        }
    }
    if (lppl->iPlayer != iplrNone) {
        RawStore16(pb, lppl->uGuesses);
        pb += 2;
    }
    if (((RTPLANET *)rgb)->det <= detSome)
        goto LFinishBRecord;

    pbBase = pb;
    pb++;
    bMask = 3;
    i = 0;
    while (i < 4) {
        if ((i != 3 || lppl->det >= detAll) && lppl->rgwtMin[i] > 0) {
            if (lppl->rgwtMin[i] <= 255) {
                *pbBase |= bMask & 0x55;
                *pb++ = lppl->rgwtMin[i];
            } else if (lppl->rgwtMin[i] > 65535) {
                *pbBase |= bMask;
                RawStore32(pb, lppl->rgwtMin[i]);
                pb += 4;
            } else {
                *pbBase |= bMask & 0xaa;
                RawStore16(pb, LOWORD(lppl->rgwtMin[i]));
                pb += 2;
            }
        }
        i++;
        bMask *= 4;
    }
    if (*pbBase == 0) {
        pb = pbBase;
    } else {
        ((RTPLANET *)rgb)->fIncSurfMin = TRUE;
    }
    if (rt == rtPlanetB) {
    LFinishBRecord:
        if (lppl->fStarbase) {
            *pb = lppl->isb;
            pb++;
        }
        if (fHistory) {
            RawStore16(pb, lppl->turn);
            pb += 2;
        }
        WriteRt(rtPlanetB, pb - rgb, rgb);
    } else {
        ((RTPLANET *)rgb)->fIsArtifact = lppl->fArtifact;
        if ((lppl->iPlayer != iplrNone && (lppl->iDeltaPop != 0 || lppl->fNoResearch)) ||
            (lppl->cMines != 0 || lppl->cFactories != 0 || lppl->cDefenses != 0 || lppl->iScanner != 31)) {
            ((RTPLANET *)rgb)->fIncImp = TRUE;
            memmove(pb, lppl->rgbImp, 8);
            pb += 8;
        }
        if (lppl->iPlayer != iplrNone) {
            if (lppl->fStarbase) {
                RawStore32(pb, lppl->lStarbase);
                pb += 4;
            }
            if (lppl->idRoute != 0) {
                RawStore16(pb, lppl->wRouting);
                pb += 2;
            }
        }
        WriteRt(rtPlanet, pb - rgb, rgb);
    }
}

void WriteFleet(FLEET *lpfl) {
    uint8_t  *pus;
    uint8_t   rgb[134];
    uint16_t  us;
    int16_t   i;
    uint8_t  *pb;
    int16_t   fByte;
    uint16_t  grMask;
    int32_t   wt;

    memmove(rgb, lpfl, offsetof(RTFLEET, grbitCsh));
    fByte = TRUE;
    grMask = 1;
    us = 0;
    i = 0;
    while (i < 16) {
        if (lpfl->rgcsh[i] > 0) {
            us |= grMask;
            if (lpfl->rgcsh[i] > 255) {
                fByte = FALSE;
            }
        }
        i++;
        grMask *= 2;
    }
    ((RTFLEET *)rgb)->fByteCsh = fByte;
    ((RTFLEET *)rgb)->grbitCsh = us;
    pb = ((RTFLEET *)rgb)->rgb;
    if (fByte) {
        for (i = 0; i < 16; i++) {
            if (lpfl->rgcsh[i] > 0) {
                *pb++ = lpfl->rgcsh[i];
            }
        }
    } else {
        pus = pb;
        for (i = 0; i < 16; i++) {
            if (lpfl->rgcsh[i] > 0) {
                RawStore16(pus, lpfl->rgcsh[i]);
                pus += 2;
            }
        }
        pb = (uint8_t *)pus;
    }
    if (lpfl->det >= detMore) {
        fByte = FALSE;
        grMask = 3;
        pus = pb;
        pb += 2;
        us = 0;
        i = 0;
        while (i < 5) {
            if (lpfl->rgwtMin[i] > 0 && (lpfl->det == detAll || (i != 4 && i != 3))) {
                if (lpfl->rgwtMin[i] <= 255) {
                    us |= grMask & 0x155;
                    *pb = lpfl->rgwtMin[i];
                    pb++;
                } else if (lpfl->rgwtMin[i] > 65535) {
                    us |= grMask & 0x3ff;
                    RawStore32(pb, lpfl->rgwtMin[i]);
                    pb += 4;
                } else {
                    us |= grMask & 0x2aa;
                    RawStore16(pb, LOWORD(lpfl->rgwtMin[i]));
                    pb += 2;
                }
            }
            i++;
            grMask *= 4;
        }
        RawStore16(pus, us);
    }
    if (lpfl->det < detAll) {
        wt = 0;
        RawStore32(pb, lpfl->dirLong);
        pb += 4;
        for (i = 0; i < 16; i++) {
            if (lpfl->rgcsh[i] > 0) {
                wt += (uint32_t)(lpfl->rgcsh[i] * (uint32_t)rglpshdef[lpfl->iPlayer][i].hul.wtEmpty);
            }
        }
        for (i = 0; i <= 3; i++) {
            wt += lpfl->rgwtMin[i];
        }
        RawStore32(pb, wt);
        pb += 4;
        WriteRt(rtFleetB, pb - rgb, rgb);
    } else {
        grMask = 1;
        us = 0;
        i = 0;
        while (i < 16) {
            if (lpfl->rgdv[i].dp != 0) {
                us |= grMask;
            }
            i++;
            grMask *= 2;
        }
        RawStore16(pb, us);
        pb += 2;
        pus = pb;
        for (i = 0; i < 16; i++) {
            if (lpfl->rgdv[i].dp != 0) {
                RawStore16(pus, lpfl->rgdv[i].dp);
                pus += 2;
            }
        }
        pb = (uint8_t *)pus;
        *pb++ = lpfl->iplan;
        *pb++ = lpfl->cord;
        WriteRt(rtFleetA, pb - rgb, rgb);
        WriteOrders(lpfl);
        if (lpfl->lpszName) {
            WriteRtString(lpfl->lpszName);
        }
    }
    return;
}

void WriteRtString(char *lpsz) {
    uint8_t rgb[33];
    int16_t cOut;

    if (lpsz && *lpsz != 0) {
        if (strlen(lpsz) > 31 || !FUtf8Valid(lpsz, strlen(lpsz))) {
            FileError(idsGameFileAppearsCorruptUnableLoadFile);
            if (penvMem) StarsLongJump(penvMem, -1);
            return;
        }
        cOut = 31;
        if (FCompressUserString(lpsz, &rgb[1], &cOut)) {
            rgb[0] = cOut;
        } else {
            strcpy(&rgb[1], lpsz);
            rgb[0] = 0;
            cOut = strlen(lpsz) + 1;
        }
        WriteRt(rtString, cOut + 1, rgb);
    }
    return;
}

void MarkFleet(FLEET *lpfl, DetType det) {
    int16_t i;
    SHDEF  *lpshdef;

    if (!lpfl->fInclude) {
        lpshdef = rglpshdef[lpfl->iPlayer];
        lpfl->fInclude = TRUE;
        lpfl->det = detNone;
        lpfl->fdirValid = 1;
        rgplr[lpfl->iPlayer].cFleet = rgplr[lpfl->iPlayer].cFleet + 1;
        for (i = 0; i < 16; i++) {
            if (lpfl->rgcsh[i] != 0) {
                lpshdef[i].fInclude = TRUE;
            }
        }
    }
    if (lpfl->det < det) {
        lpfl->det = det;
    }
    return;
}

void WriteBattlePlan(BTLPLAN *lpbtlplan, int16_t fLog) {
    uint8_t  rgb[36];
    uint8_t *pb;
    char     szPlanName[32];
    int16_t  cOut;

    memmove(rgb, lpbtlplan, 4);
    if (lpbtlplan->fDelete) {
        pb = &rgb[2];
    } else {
        pb = &rgb[4];
        strcpy(szPlanName, lpbtlplan->szName);
        cOut = 31;
        if (szPlanName[0] != 0 && FCompressUserString(szPlanName, pb + 1, &cOut)) {
            *pb = cOut;
            pb += 1 + cOut;
        } else {
            strcpy(pb + 1, szPlanName);
            *pb = 0;
            pb += 2 + strlen(szPlanName);
        }
    }
    if (fLog) {
        WriteMemRt(rtBtlPlan, pb - rgb, rgb);
    } else {
        WriteRt(rtBtlPlan, pb - rgb, rgb);
    }
    return;
}

void MarkPlanet(PLANET *lppl, int16_t iPlr, DetType det) {
    SHDEF *lpshdef;

    if (!lppl->fInclude) {
        lppl->fInclude = TRUE;
        lppl->det = detNone;
        rgplr[iPlr].cPlanet++;
    }
    if (lppl->det < det) {
        lppl->det = det;
    }
    if (lppl->iPlayer != iplrNone && !rgplr[lppl->iPlayer].fInclude) {
        rgplr[lppl->iPlayer].fInclude = TRUE;
        rgplr[lppl->iPlayer].det = detSome;
    }
    if (det != detObscure && lppl->iPlayer != iplrNone && lppl->fStarbase) {
        lpshdef = rglpshdefSB[lppl->iPlayer] + lppl->isb;
        if (!lpshdef->fInclude) {
            lpshdef->fInclude = TRUE;
            lpshdef->det = detNone;
            rgplr[lppl->iPlayer].cshdefSB = rgplr[lppl->iPlayer].cshdefSB + 1;
        }
        if (lpshdef->det < detSome) {
            lpshdef->det = detSome;
        }
    }
    return;
}

void SetSzWorkFromDt(DtFileType dt, int16_t iPlayer) {
    char   *pchSlash;
    int16_t c;
    char   *pchDot;

    pchDot = strrchr(szBase, 46);
    if (pchDot) {
        pchSlash = strrchr(szBase, chDirSep);
        if (!pchSlash || pchSlash < pchDot) {
            *pchDot = 0;
        }
    }
    c = CchSprintf(szWork, "%s.", szBase);
    switch (dt) {
    case dtXY:
    default:
        strcat(szWork, "xy");
        break;
    case dtHost:
        strcat(szWork, "hst");
        break;
    case dtLog:
    case dtTurn:
    case dtHist:
        CchSprintf(&szWork[c], "%c%d", dt == dtLog ? 120 : dt == dtHist ? 104 : 109, iPlayer + 1);
    }
    return;
}

int16_t STARS_TEST_IMPL(FCreateFile)(DtFileType dt, int16_t iPlayer, char *szForceName) {
    jmp_buf *penvMemSav;
    jmp_buf  env;
    char    *psz;

    if (szForceName) {
        psz = szForceName;
    } else {
        SetSzWorkFromDt(dt, iPlayer);
        psz = szWork;
    }
    penvMemSav = penvMem;
    penvMem = &env;
    if (setjmp(env) != 0) {
        penvMem = penvMemSav;
        return FALSE;
    }
    StreamOpen(psz, mdCreate);
    WriteBOF(iPlayer, dt, FALSE);
    penvMem = penvMemSav;
    return TRUE;
}

void WriteBOF(int16_t iPlayer, int16_t dt, int16_t fMulti) {
    RTBOF rtbof;

    memset(&rtbof, 0, sizeof(RTBOF));
    strncpy(rtbof.rgid, "J3J3", 4);
    rtbof.lidGame = game.lid;
    rtbof.wGen = game.wGen;
    rtbof.verInc = 0;
    // 2.85 identifies UTF-8 text; older files have ambiguous byte encodings.
    rtbof.verMinor = 85;
    rtbof.verMajor = 2;
    rtbof.turn = game.turn;
    rtbof.fCrippled = FALSE;
    rtbof.iPlayer = iPlayer;
    rtbof.lSaltTime = (int16_t)(LOWORD(DwTickCount()) + Random(2000));
    rtbof.dt = dt;
    rtbof.fDone = gd.fSubmit;
    rtbof.fInUse = gd.fHostMode;
    rtbof.fGameOverMan = dt == 2 && gd.fGameOverMan != 0;
    WriteRt(rtBOF, 16, &rtbof);
    return;
}

int16_t FMarkFile(DtFileType dt, int16_t iPlayer, MdMark mdMark, int16_t f) {
    StringId ids;
    RTBOF    rtbof;
    jmp_buf *penvMemSav;
    jmp_buf  env;
    int16_t  fChange;
    int16_t  fSuccess;
    int16_t  fSilentSav;
    int32_t  lSeedSav2;
    int32_t  lSeedSav1;

    fSilentSav = fFileErrSilent;
    fSuccess = FALSE;
    ids = idsUniverseDefinitionFileSeemsMissingCorrupt;
    SetSzWorkFromDt(dt & 0xff, iPlayer);
    penvMemSav = penvMem;
    penvMem = &env;
    if (setjmp(env) != 0) {
        fFileErrSilent = fSilentSav;
        if (ids != idsUniverseDefinitionFileSeemsMissingCorrupt) {
            FileError(ids);
        }
        StreamClose();
        penvMem = penvMemSav;
        return FALSE;
    }
    fFileErrSilent = TRUE;
    StreamOpen(szWork, mdReadWrite);
    fFileErrSilent = fSilentSav;
    ids = idsGameFileAppearsCorruptUnableLoadFile;
    ReadRt();
    if (hdrCur.rt != rtBOF) {
        FileError(idsFileDoesBelongVersionStars);
        goto LBadFile;
    }
    if (((RTBOF *)rgbCur)->verMajor < 2 || (((RTBOF *)rgbCur)->verMajor == 2 && ((RTBOF *)rgbCur)->verMinor < 85)) {
        FileError(idsSorryFileCreatedOlderVersionStarsIncompatible);
        goto LBadFile;
    }
    if (((RTBOF *)rgbCur)->verMajor > 2 || (((RTBOF *)rgbCur)->verMajor == 2 && ((RTBOF *)rgbCur)->verMinor > 85)) {
        FileError(idsFileCreatedNewerVersionStarsMustUpgrade);
        goto LBadFile;
    }
    rtbof = *((RTBOF *)rgbCur);
    if (game.lid == 0)
        goto LBadFile;
    if (rtbof.lidGame != game.lid) {
        FileError(idsFileGame);
        goto LBadFile;
    }
    fChange = FALSE;
    switch (mdMark) {
    case mdMarkInUse:
        if (rtbof.fInUse != f) {
            rtbof.fInUse = f;
            fChange = TRUE;
        }
        break;
    case mdMarkDone:
        if (rtbof.fDone != f) {
            rtbof.fDone = f;
            fChange = TRUE;
        }
        break;
    case mdMarkMulti:
        if (rtbof.fMulti != f) {
            rtbof.fMulti = f;
            fChange = TRUE;
        }
        break;
    case mdMarkAi:
        do {
            GetFileSeeds(&lSeedSav1, &lSeedSav2);
            ReadRt();
        } while (hdrCur.rt != rtPlr || ((PLAYER *)rgbCur)->iPlayer != iPlayer);
        if (((PLAYER *)rgbCur)->fAi != f) {
            if (((PLAYER *)rgbCur)->fAi != 0) {
                if (((PLAYER *)rgbCur)->idAi != idAiMaid)
                    goto LBadFile;
                ((PLAYER *)rgbCur)->fAi = FALSE;
            } else {
                ((PLAYER *)rgbCur)->fAi = TRUE;
                ((PLAYER *)rgbCur)->idAi = idAiMaid;
            }
            ((PLAYER *)rgbCur)->lSalt = ~((PLAYER *)rgbCur)->lSalt;
            LSeekFile(hf, (int16_t)-(hdrCur.cb + 2), 1);
            SetFileSeeds(lSeedSav1, lSeedSav2);
            WriteRt(rtPlr, hdrCur.cb, rgbCur);
            fChange = dt == dtTurn;
            rtbof.fDone = FALSE;
        }
        break;
    }
    if (fChange) {
        LSeekFile(hf, 0, 0);
        WriteRt(rtBOF, 16, &rtbof);
    }
    fSuccess = TRUE;
LBadFile:
    if ((dt & 0x2000) && fSuccess) {
        LSeekFile(hf, 0, 2);
    } else {
        StreamClose();
    }
    penvMem = penvMemSav;
    return fSuccess;
}

void STARS_TEST_IMPL(WriteRt)(RecordType rt, int16_t cb, void *rg) {
    HDR hdr;

    if (cb != 0)
        memmove(rgbCur, rg, cb);
    if (rt == rtBOF) {
        SetFileXorStream(((RTBOF *)rgbCur)->lidGame, ((RTBOF *)rgbCur)->lSaltTime, ((RTBOF *)rgbCur)->turn, ((RTBOF *)rgbCur)->iPlayer,
                         ((RTBOF *)rgbCur)->fCrippled);
    } else if (rt != rtEOF) {
        XorFileBuf(rgbCur, cb);
    }
    hdr.cb = cb;
    hdr.rt = rt;
    RgToStream(&hdr, 2);
    RgToStream(rgbCur, cb);
    return;
}

void RgToStream(void *rg, uint16_t cb) {
    if (cb != 0 && CbWriteFile(hf, rg, cb) != cb) {
        AlertSz(PszFormatIds(idsErrorWritingFile, NULL), MB_ICONHAND);
        StarsLongJump(penvMem, -1);
    }
    return;
}

void SetVisiblePlanFleet(int16_t iPlr) {
    SetVisPFInit(iPlr);
    if (iPlr == iplrNone) {
        rgplr[0].cPlanet = game.cPlanMax;
    } else {
        if (iPlr != iplrNone) {
            UpdateProgressGauge(progressStep1);
        }
        SetVisPFFleets(iPlr);
        if (iPlr != iplrNone) {
            UpdateProgressGauge(progressStep1);
        }
        SetVisPFPlanets(iPlr);
        if (iPlr != iplrNone) {
            UpdateProgressGauge(progressStep1);
        }
        SetVisPFThings(iPlr);
        SetVisPFFinish(iPlr);
    }
    return;
}

void SetVisPFInit(int16_t iPlr) {
    PLANET       *lpplMac;
    uint16_t      detNew;
    PLANET       *lppl;
    int16_t       j;
    FLEET        *lpfl;
    THING        *lpth;
    int16_t       ifl;
    int16_t       i;
    THING        *lpthMac;
    RaceAttribute raMajor;
    uint16_t      grbitPlr;
    int16_t       iSteal;

    // Host visibility has no player race; every packet is included below.
    raMajor = iPlr == iplrNone ? raNone : GetRaceStat(&rgplr[iPlr], rsMajorAdv);
    grbitPlr = iPlr == iplrNone ? 0 : 1 << iPlr;
    for (i = 0; i < game.cPlayer; i++) {
        rgplr[i].cPlanet = 0;
    }
    for (i = 0; i < game.cPlayer; i++) {
        if (iPlr == iplrNone || iPlr == i || rgplr[i].fDead) {
            rgplr[i].fInclude = TRUE;
            rgplr[i].det = detAll;
        } else {
            rgplr[i].fInclude = FALSE;
        }
        rgplr[i].cFleet = 0;
        rgplr[i].cShDef = 0;
        rgplr[i].cshdefSB = 0;
        for (j = 0; j < 16; j++) {
            if ((iPlr == iplrNone || iPlr == i) && !rglpshdef[i][j].fFree) {
                rglpshdef[i][j].fInclude = TRUE;
                rglpshdef[i][j].det = detAll;
                rglpshdef[i][j].cExist = 0;
                rgplr[i].cShDef++;
            } else {
                rglpshdef[i][j].fInclude = FALSE;
            }
        }
        for (j = 0; j < 10; j++) {
            if ((iPlr == iplrNone || iPlr == i) && !rglpshdefSB[i][j].fFree) {
                rglpshdefSB[i][j].fInclude = TRUE;
                rglpshdefSB[i][j].det = detAll;
                rglpshdefSB[i][j].cExist = 0;
                rgplr[i].cshdefSB++;
            } else {
                rglpshdefSB[i][j].fInclude = FALSE;
            }
        }
    }
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (iPlr == iplrNone || iPlr == lppl->iPlayer) {
            lppl->fInclude = TRUE;
            lppl->det = detAll;
            if (iPlr != iplrNone) {
                rgplr[iPlr].cPlanet++;
            }
            if (lppl->fStarbase) {
                rglpshdefSB[lppl->iPlayer][lppl->isb].cExist = rglpshdefSB[lppl->iPlayer][lppl->isb].cExist + 1;
            }
        } else {
            lppl->fInclude = FALSE;
        }
    }
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        lpfl->fdirValid = 0;
        lpfl->fMark = FALSE;
        if ((iPlr == iplrNone || iPlr == lpfl->iPlayer) && !lpfl->fDead) {
            lpfl->fInclude = TRUE;
            lpfl->det = detAll;
            rgplr[lpfl->iPlayer].cFleet = rgplr[lpfl->iPlayer].cFleet + 1;
            for (j = 0; j < 16; j++) {
                if (lpfl->rgcsh[j] != 0) {
                    rglpshdef[lpfl->iPlayer][j].cExist = rglpshdef[lpfl->iPlayer][j].cExist + lpfl->rgcsh[j];
                }
            }
            if (iPlr != iplrNone && lpfl->idPlanet != idPlanetDeepSpace) {
                lppl = lpPlanets + lpfl->idPlanet;
                detNew = GetCachedFleetScannerRange(lpfl, NULL, NULL, &iSteal) < 0 ? 1 : 3;
                if (iSteal >= 2) {
                    detNew = 4;
                }
                if (lpfl->fHereAllTurn && lppl->iPlayer == iplrNone && lpfl->lpplord->rgord[0].grTask == grTaskMine && CMineFromLpfl(lpfl) > 0) {
                    detNew = 4;
                }
                MarkPlanet(lppl, iPlr, detNew);
            }
        } else {
            lpfl->fInclude = FALSE;
        }
    }
    lpth = lpThings;
    lpthMac = lpThings + cThing;
    for (; lpth < lpthMac; lpth++) {
        switch (lpth->ith) {
        case ithMineralPacket:
            if (iPlr == iplrNone || raMajor == raMassAccel) {
                lpth->thp.fInclude = TRUE;
                if (rgplr[lpth->iplr].fInclude)
                    break;
                rgplr[lpth->iplr].fInclude = TRUE;
                rgplr[lpth->iplr].det = detSome;
                break;
            }
            lpth->thp.fInclude = FALSE;
            break;
        case ithWormhole:
            lpth->thw.fInclude = iPlr == iplrNone;
            break;
        case ithMysteryTrader:
            lpth->tht.fInclude = TRUE;
            break;
        case ithMinefield:
            if ((lpth->thm.grbitPlrNow & grbitPlr) && !rgplr[lpth->iplr].fInclude) {
                rgplr[lpth->iplr].fInclude = TRUE;
                rgplr[lpth->iplr].det = detSome;
            }
        }
    }
    return;
}

void SetVisPFFleets(int16_t iPlr) {
    PLANET  *lpplMac;
    POINT16  pt;
    int16_t  pctCloak;
    int16_t  dy;
    FLEET   *lpfl2;
    int32_t  d2;
    PLANET  *lppl;
    int16_t  j;
    FLEET   *lpfl;
    THING   *lpth;
    int32_t  lRadius2;
    int16_t  ifl;
    THING   *lpthMac;
    int16_t  iRadius;
    int16_t  dx;
    uint16_t grbitPlr;
    int32_t  lRadPlanet2;
    int16_t  iRadPlanet;
    int16_t  iSteal;
    int16_t  pctDetect;
    int32_t  l;
    int32_t  lVis2;

    grbitPlr = iPlr == iplrNone ? 0 : 1 << iPlr;
    for (ifl = 0; ifl < cFleet; ifl++) {
        lpfl = rglpfl[ifl];
        if (!rglpfl[ifl])
            break;
        if (!lpfl->fDead) {
            if (lpfl->iPlayer != iPlr) {
                if (!lpfl->fInclude && lpfl->idPlanet != idPlanetDeepSpace && lpPlanets[lpfl->idPlanet].iPlayer == iPlr) {
                    MarkFleet(lpfl, detSome);
                }
            } else {
                if (lpfl->fBombed && lpfl->idPlanet != idPlanetDeepSpace) {
                    MarkPlanet(lpPlanets + lpfl->idPlanet, iPlr, detSome);
                }
                iRadius = GetCachedFleetScannerRange(lpfl, &iRadPlanet, &pctDetect, &iSteal);
                iRadius = 0 <= iRadius ? iRadius : 0;
                lRadius2 = (uint32_t)(iRadius * iRadius);
                lRadPlanet2 = (uint32_t)(iRadPlanet * iRadPlanet);
                pt = lpfl->pt;
                for (j = 0; j < cFleet; j++) {
                    lpfl2 = rglpfl[j];
                    if (!rglpfl[j])
                        break;
                    if (!lpfl2->fDead) {
                        if ((iSteal & 1) && pt.x == lpfl2->pt.x && pt.y == lpfl2->pt.y && (!lpfl2->fInclude || lpfl2->det < detMore)) {
                            MarkFleet(lpfl2, detMore);
                        }
                        if (!lpfl2->fInclude) {
                            dx = abs(pt.x - lpfl2->pt.x);
                            if (dx <= iRadius) {
                                dy = abs(pt.y - lpfl2->pt.y);
                                if (dy <= iRadius) {
                                    l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                                    if ((uint32_t)(dx * dx) + (uint32_t)(dy * dy) <= lRadius2 && (lpfl2->idPlanet == idPlanetDeepSpace || l <= lRadPlanet2)) {
                                        pctCloak = PctCloakFromLpfl(lpfl2);
                                        if (pctDetect != 100) {
                                            pctCloak = (int16_t)(pctCloak * pctDetect) / 100;
                                        }
                                        if (pctCloak == 0) {
                                            MarkFleet(lpfl2, detSome);
                                        } else if (l <= (int32_t)(lRadius2 * (int16_t)(100 - pctCloak)) / 100 * (int16_t)(100 - pctCloak) / 100 &&
                                                   (lpfl2->idPlanet == idPlanetDeepSpace ||
                                                    l <= (int32_t)(lRadPlanet2 * (int16_t)(100 - pctCloak)) / 100 * (int16_t)(100 - pctCloak) / 100)) {
                                            MarkFleet(lpfl2, detSome);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                lpth = lpThings;
                lpthMac = lpThings + cThing;
                for (; lpth < lpthMac; lpth++) {
                    if (iPlr != iplrNone) {
                        switch (lpth->ith) {
                        case ithMinefield:
                        case ithMineralPacket:
                        case ithMysteryTrader:
                        case ithWormhole:
                            if ((lpth->ith != ithMinefield || !(lpth->thm.grbitPlrNow & grbitPlr)) && (lpth->ith != ithMysteryTrader || !lpth->tht.fInclude) &&
                                (lpth->ith != ithMineralPacket || !lpth->thp.fInclude) && (lpth->ith != ithWormhole || !lpth->thw.fInclude)) {
                                dx = abs(pt.x - lpth->pt.x);
                                dy = abs(pt.y - lpth->pt.y);
                                l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                                if (l <= lRadius2 || lpth->ith == ithMinefield) {
                                    if (lpth->ith == ithMineralPacket) {
                                        lpth->thp.fInclude = TRUE;
                                    LThIncPlr:
                                        if (!rgplr[lpth->iplr].fInclude) {
                                            rgplr[lpth->iplr].fInclude = TRUE;
                                            rgplr[lpth->iplr].det = detSome;
                                        }
                                    } else if (lpth->ith == ithMysteryTrader) {
                                        lpth->tht.fInclude = TRUE;

                                    } else if (lpth->ith == ithWormhole) {
                                        if (!(lpth->thw.grbitPlr & grbitPlr) && l > (int32_t)(lRadius2 >> 4) && l > lRadPlanet2)
                                            continue;
                                        lpth->thw.grbitPlr |= grbitPlr;
                                        lpth->thw.fInclude = TRUE;

                                    } else {
                                        if (((lpth->thm.grbitPlr & grbitPlr) && l <= lRadius2) || l <= lRadPlanet2 || l <= (int32_t)(lRadius2 >> 4) ||
                                            l <= lpth->thm.cMines) {
                                            lpth->thm.grbitPlr |= grbitPlr;
                                            lpth->thm.grbitPlrNow |= grbitPlr;
                                            goto LThIncPlr;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if ((iSteal & 2) && lpfl->idPlanet != idPlanetDeepSpace) {
                    MarkPlanet(lpPlanets + lpfl->idPlanet, iPlr, detMore);
                }
                if (iRadPlanet > 0) {
                    iRadius = iRadPlanet;
                    lRadius2 = (uint32_t)(iRadius * iRadius);
                    pt = lpfl->pt;
                    lppl = lpPlanets;
                    lpplMac = lpPlanets + cPlanet;
                    for (; lppl < lpplMac; lppl++) {
                        if (!lppl->fInclude || lppl->det < detSome) {
                            dx = abs(rgptPlan[lppl->id].x - pt.x);
                            if (dx <= iRadius) {
                                dy = abs(rgptPlan[lppl->id].y - pt.y);
                                if (dy <= iRadius) {
                                    d2 = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                                    if ((uint32_t)(dx * dx) + (uint32_t)(dy * dy) <= lRadius2) {
                                        if (!lppl->fStarbase || lppl->iPlayer == iplrNone)
                                            goto LMark101;

                                        lVis2 = rglpshdefSB[lppl->iPlayer][lppl->isb].lVisible;
                                        if (lVis2 < 10000 && d2 > (int32_t)((int64_t)lRadius2 * lVis2 / 10000)) {
                                            MarkPlanet(lppl, iPlr, detObscure);
                                            continue;
                                        }

                                    LMark101:
                                        MarkPlanet(lppl, iPlr, detSome);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return;
}

void SetVisPFPlanets(int16_t iPlr) {
    int32_t  lRadPlanet2;
    int16_t  iRadPlanet;
    PLANET  *lpplMac;
    POINT16  pt;
    int16_t  pctCloak;
    PLANET  *lppl2;
    int16_t  dy;
    FLEET   *lpfl2;
    int32_t  d2;
    PLANET  *lppl;
    int16_t  j;
    THING   *lpth;
    int32_t  lRadius2;
    int16_t  i;
    THING   *lpthMac;
    int16_t  iRadius;
    int16_t  fStargateView;
    int16_t  dx;
    int32_t  l;
    PLANET  *lpplMac2;
    uint16_t grbitPlr;
    int16_t  rgStargateRange[16];
    int32_t  lVis2;

    grbitPlr = iPlr == iplrNone ? 0 : 1 << iPlr;
    fStargateView = FALSE;
    if (GetRaceStat(&rgplr[iPlr], rsMajorAdv) == raStargate) {
        for (i = 0; i < 10; i++) {
            rgStargateRange[i] = 0;
            if (!rglpshdefSB[iPlr][i].fFree) {
                rgStargateRange[i] = StargateRangeFromLppl(NULL, iPlr, i);
                if (rgStargateRange[i] > 0) {
                    fStargateView = TRUE;
                }
            }
        }
    }
    if (iPlr != iplrNone) {
        UpdateProgressGauge(progressStep1);
    }
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iPlr) {
            iRadius = GetPlanetScannerRange(lppl, &iRadPlanet);
            lRadius2 = (uint32_t)(iRadius * iRadius);
            lRadPlanet2 = (uint32_t)(iRadPlanet * iRadPlanet);
            pt = rgptPlan[lppl->id];
            for (j = 0; j < cFleet; j++) {
                lpfl2 = rglpfl[j];
                if (!rglpfl[j])
                    break;
                if (!lpfl2->fInclude && !lpfl2->fDead) {
                    dx = abs(pt.x - lpfl2->pt.x);
                    if (dx <= iRadius) {
                        dy = abs(pt.y - lpfl2->pt.y);
                        if (dy <= iRadius) {
                            l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                            if ((uint32_t)(dx * dx) + (uint32_t)(dy * dy) <= lRadius2 && (lpfl2->idPlanet == idPlanetDeepSpace || l <= lRadPlanet2)) {
                                pctCloak = PctCloakFromLpfl(lpfl2);
                                if (pctCloak == 0) {
                                    MarkFleet(lpfl2, detSome);
                                } else if (l <= (int32_t)(lRadius2 * (int16_t)(100 - pctCloak)) / 100 * (int16_t)(100 - pctCloak) / 100 &&
                                           (lpfl2->idPlanet == idPlanetDeepSpace ||
                                            l <= (int32_t)(lRadPlanet2 * (int16_t)(100 - pctCloak)) / 100 * (int16_t)(100 - pctCloak) / 100)) {
                                    MarkFleet(lpfl2, detSome);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (iPlr != iplrNone) {
        UpdateProgressGauge(progressStep1);
    }
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iPlr) {
            iRadius = GetPlanetScannerRange(lppl, &iRadPlanet);
            lRadius2 = (uint32_t)(iRadius * iRadius);
            lRadPlanet2 = (uint32_t)(iRadPlanet * iRadPlanet);
            pt = rgptPlan[lppl->id];
            lpth = lpThings;
            lpthMac = lpThings + cThing;
            for (; lpth < lpthMac; lpth++) {
                if (iPlr != iplrNone) {
                    switch (lpth->ith) {
                    case ithMinefield:
                    case ithMineralPacket:
                    case ithMysteryTrader:
                    case ithWormhole:
                        if ((lpth->ith != ithMinefield || !(lpth->thm.grbitPlrNow & grbitPlr)) && (lpth->ith != ithMysteryTrader || !lpth->tht.fInclude) &&
                            (lpth->ith != ithMineralPacket || !lpth->thp.fInclude) && (lpth->ith != ithWormhole || !lpth->thw.fInclude)) {
                            dx = abs(pt.x - lpth->pt.x);
                            if (dx <= iRadius) {
                                dy = abs(pt.y - lpth->pt.y);
                                if (dy <= iRadius) {
                                    l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                                    if ((uint32_t)(dx * dx) + (uint32_t)(dy * dy) <= lRadius2) {
                                        if (lpth->ith == ithMineralPacket) {
                                            lpth->thp.fInclude = TRUE;
                                        LThIncPlr2:
                                            if (!rgplr[lpth->iplr].fInclude) {
                                                rgplr[lpth->iplr].fInclude = TRUE;
                                                rgplr[lpth->iplr].det = detSome;
                                            }
                                        } else if (lpth->ith == ithMysteryTrader) {
                                            lpth->tht.fInclude = TRUE;

                                        } else if (lpth->ith == ithWormhole) {
                                            if (!(lpth->thw.grbitPlr & grbitPlr) && l > (int32_t)(lRadius2 >> 4) && l > lRadPlanet2)
                                                continue;
                                            lpth->thw.grbitPlr |= grbitPlr;
                                            lpth->thw.fInclude = TRUE;

                                        } else {
                                            if ((lpth->thm.grbitPlr & grbitPlr) || l <= lRadPlanet2 || l <= (int32_t)(lRadius2 >> 4)) {
                                                lpth->thm.grbitPlr |= grbitPlr;
                                                lpth->thm.grbitPlrNow |= grbitPlr;
                                                goto LThIncPlr2;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (iPlr != iplrNone) {
        UpdateProgressGauge(progressStep1);
    }
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iPlr) {
            iRadius = GetPlanetScannerRange(lppl, &iRadPlanet);
            lRadius2 = (uint32_t)(iRadius * iRadius);
            lRadPlanet2 = (uint32_t)(iRadPlanet * iRadPlanet);
            pt = rgptPlan[lppl->id];
            if (fStargateView && lppl->fStarbase && rgStargateRange[lppl->isb] > 0) {
                iRadius = rgStargateRange[lppl->isb];
                lRadius2 = (uint32_t)(iRadius * iRadius);
                lppl2 = lpPlanets;
                lpplMac2 = lpPlanets + cPlanet;
                for (; lppl2 < lpplMac2; lppl2++) {
                    if ((!lppl2->fInclude || lppl2->det < detSome) && lppl2->fStarbase && StargateRangeFromLppl(lppl2, 0, 0) != 0) {
                        if (iRadius >= 10000)
                            goto LMarkStargate;

                        dx = abs(rgptPlan[lppl2->id].x - pt.x);
                        if (dx > iRadius)
                            continue;
                        dy = abs(rgptPlan[lppl2->id].y - pt.y);
                        if (dy > iRadius)
                            continue;
                        d2 = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                        if ((uint32_t)(dx * dx) + (uint32_t)(dy * dy) > lRadius2)
                            continue;
                        lVis2 = rglpshdefSB[lppl2->iPlayer][lppl2->isb].lVisible;
                        if (lVis2 < 10000 && d2 > (int32_t)((int64_t)lRadius2 * lVis2 / 10000))
                            continue;

                    LMarkStargate:
                        MarkPlanet(lppl2, iPlr, detSome);
                    }
                }
            }
        }
    }
    if (iPlr != iplrNone) {
        UpdateProgressGauge(progressStep1);
    }
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iPlr) {
            iRadius = GetPlanetScannerRange(lppl, &iRadPlanet);
            lRadius2 = (uint32_t)(iRadius * iRadius);
            lRadPlanet2 = (uint32_t)(iRadPlanet * iRadPlanet);
            pt = rgptPlan[lppl->id];
            if (iRadPlanet > 0) {
                iRadius = iRadPlanet;
                lRadius2 = lRadPlanet2;
                lppl2 = lpPlanets;
                lpplMac2 = lpPlanets + cPlanet;
                for (; lppl2 < lpplMac2; lppl2++) {
                    if (!lppl2->fInclude || lppl2->det < detSome) {
                        dx = abs(rgptPlan[lppl2->id].x - pt.x);
                        if (dx <= iRadius) {
                            dy = abs(rgptPlan[lppl2->id].y - pt.y);
                            if (dy <= iRadius) {
                                d2 = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                                if ((uint32_t)(dx * dx) + (uint32_t)(dy * dy) <= lRadius2) {
                                    if (!lppl2->fStarbase || lppl2->iPlayer == iplrNone)
                                        goto LMark102;

                                    lVis2 = rglpshdefSB[lppl2->iPlayer][lppl2->isb].lVisible;
                                    if (lVis2 < 10000 && d2 > (int32_t)((int64_t)lRadius2 * lVis2 / 10000)) {
                                        MarkPlanet(lppl2, iPlr, detObscure);
                                        continue;
                                    }

                                LMark102:
                                    MarkPlanet(lppl2, iPlr, detSome);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return;
}

void SetVisPFThings(int16_t iPlr) {
    POINT16  pt;
    int16_t  pctCloak;
    int16_t  dy;
    FLEET   *lpfl2;
    int32_t  d2;
    int16_t  j;
    THING   *lpth;
    int32_t  lRadius2;
    THING   *lpthMac;
    int16_t  iRadius;
    int16_t  dx;
    uint16_t grbitPlr;
    PLANET  *lppl2;
    THING   *lpthMac2;
    THING   *lpth2;
    int32_t  l;
    PLANET  *lpplMac2;
    int32_t  lVis2;

    grbitPlr = iPlr == iplrNone ? 0 : 1 << iPlr;
    if (GetRaceStat(&rgplr[iPlr], rsMajorAdv) == raMassAccel) {
        lpth = lpThings;
        lpthMac = lpThings + cThing;
        for (; lpth < lpthMac; lpth++) {
            if (lpth->ith == ithMineralPacket && lpth->iplr == iPlr && lpth->thp.iWarp != 0) {
                lpth->thp.fInclude = TRUE;
                iRadius = lpth->thp.iWarp + 4;
                iRadius *= iRadius;
                lRadius2 = (uint32_t)(iRadius * iRadius);
                pt = lpth->pt;
                for (j = 0; j < cFleet; j++) {
                    lpfl2 = rglpfl[j];
                    if (!rglpfl[j])
                        break;
                    if (!lpfl2->fInclude && !lpfl2->fDead) {
                        dx = abs(pt.x - lpfl2->pt.x);
                        if (dx <= iRadius) {
                            dy = abs(pt.y - lpfl2->pt.y);
                            if (dy <= iRadius) {
                                l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                                if ((uint32_t)(dx * dx) + (uint32_t)(dy * dy) <= lRadius2) {
                                    pctCloak = PctCloakFromLpfl(lpfl2);
                                    if (pctCloak == 0) {
                                        MarkFleet(lpfl2, detSome);
                                    } else if (l <= (int32_t)(lRadius2 * (int16_t)(100 - pctCloak)) / 100 * (int16_t)(100 - pctCloak) / 100) {
                                        MarkFleet(lpfl2, detSome);
                                    }
                                }
                            }
                        }
                    }
                }
                lpth2 = lpThings;
                lpthMac2 = lpThings + cThing;
                for (; lpth2 < lpthMac2; lpth2++) {
                    if (iPlr != iplrNone) {
                        switch (lpth2->ith) {
                        case ithMinefield:
                        case ithMineralPacket:
                        case ithMysteryTrader:
                        case ithWormhole:
                            if ((lpth2->ith != ithMinefield || !(lpth2->thm.grbitPlrNow & grbitPlr)) &&
                                (lpth2->ith != ithMysteryTrader || !lpth2->tht.fInclude) && (lpth2->ith != ithMineralPacket || !lpth2->thp.fInclude) &&
                                (lpth2->ith != ithWormhole || !lpth2->thw.fInclude)) {
                                dx = abs(pt.x - lpth2->pt.x);
                                if (dx <= iRadius) {
                                    dy = abs(pt.y - lpth2->pt.y);
                                    if (dy <= iRadius) {
                                        l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                                        if ((uint32_t)(dx * dx) + (uint32_t)(dy * dy) <= lRadius2) {
                                            if (lpth2->ith == ithMineralPacket) {
                                                lpth2->thp.fInclude = TRUE;
                                            LThIncPlr3:
                                                if (!rgplr[lpth2->iplr].fInclude) {
                                                    rgplr[lpth2->iplr].fInclude = TRUE;
                                                    rgplr[lpth2->iplr].det = detSome;
                                                }
                                            } else if (lpth2->ith == ithMysteryTrader) {
                                                lpth2->tht.fInclude = TRUE;

                                            } else if (lpth2->ith == ithWormhole) {
                                                if (!(lpth2->thw.grbitPlr & grbitPlr) && l > lRadius2)
                                                    continue;
                                                lpth2->thw.grbitPlr |= grbitPlr;
                                                lpth2->thw.fInclude = TRUE;

                                            } else {
                                                lpth2->thm.grbitPlr |= grbitPlr;
                                                lpth2->thm.grbitPlrNow |= grbitPlr;
                                                goto LThIncPlr3;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                lppl2 = lpPlanets;
                lpplMac2 = lpPlanets + cPlanet;
                for (; lppl2 < lpplMac2; lppl2++) {
                    if (!lppl2->fInclude || lppl2->det < detSome) {
                        dx = abs(rgptPlan[lppl2->id].x - pt.x);
                        if (dx <= iRadius) {
                            dy = abs(rgptPlan[lppl2->id].y - pt.y);
                            if (dy <= iRadius) {
                                d2 = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                                if ((uint32_t)(dx * dx) + (uint32_t)(dy * dy) <= lRadius2) {
                                    if (!lppl2->fStarbase || lppl2->iPlayer == iplrNone)
                                        goto LMark103;

                                    lVis2 = rglpshdefSB[lppl2->iPlayer][lppl2->isb].lVisible;
                                    if (lVis2 < 10000 && d2 > (int32_t)((int64_t)lRadius2 * lVis2 / 10000)) {
                                        MarkPlanet(lppl2, iPlr, detObscure);
                                        continue;
                                    }

                                LMark103:
                                    MarkPlanet(lppl2, iPlr, detSome);
                                }
                            }
                        }
                    }
                }
            }
        }
    } else if (GetRaceStat(&rgplr[iPlr], rsMajorAdv) == raMines) {
        lpth = lpThings;
        lpthMac = lpThings + cThing;
        for (; lpth < lpthMac; lpth++) {
            if (lpth->ith == ithMinefield && lpth->iplr == iPlr) {
                lRadius2 = lpth->thm.cMines;
                pt = lpth->pt;
                for (j = 0; j < cFleet; j++) {
                    lpfl2 = rglpfl[j];
                    if (!rglpfl[j])
                        break;
                    if (!lpfl2->fInclude && !lpfl2->fDead && lpfl2->idPlanet == idPlanetDeepSpace) {
                        dx = abs(pt.x - lpfl2->pt.x);
                        if (dx <= lRadius2) {
                            dy = abs(pt.y - lpfl2->pt.y);
                            if (dy <= lRadius2) {
                                l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                                if ((uint32_t)(dx * dx) + (uint32_t)(dy * dy) <= lRadius2) {
                                    pctCloak = PctCloakFromLpfl(lpfl2);
                                    if (pctCloak == 0 || Random(100) >= pctCloak) {
                                        MarkFleet(lpfl2, detSome);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return;
}

void SetVisPFFinish(int16_t iPlr) {
    int16_t detMajor;
    int16_t j;
    int16_t i;

    detMajor = GetRaceStat(&rgplr[iPlr], rsMajorAdv) == raAttack ? 7 : 3;
    for (i = 0; i < game.cPlayer; i++) {
        if (i != iPlr) {
            rgplr[i].cShDef = 0;
            for (j = 0; j < 16; j++) {
                if (1 << iPlr & rglpshdef[i][j].grbitPlr) {
                    rglpshdef[i][j].fInclude = TRUE;
                    rglpshdef[i][j].det = detAll;
                    goto LFinShdef;
                } else {
                    if (!rglpshdef[i][j].fInclude)
                        continue;
                    rglpshdef[i][j].det = detMajor;
                }
            LFinShdef:
                rgplr[i].det = detSome;
                rgplr[i].fInclude = TRUE;
                rgplr[i].cShDef++;
            }
            rgplr[i].cshdefSB = 0;
            for (j = 0; j < 10; j++) {
                if (1 << iPlr & rglpshdefSB[i][j].grbitPlr) {
                    rglpshdefSB[i][j].fInclude = TRUE;
                    rglpshdefSB[i][j].det = detAll;
                    goto LFinShdefSB;
                } else {
                    if (!rglpshdefSB[i][j].fInclude)
                        continue;
                    rglpshdefSB[i][j].det = detMajor;
                }
            LFinShdefSB:
                rgplr[i].fInclude = TRUE;
                rgplr[i].det = detSome;
                rgplr[i].cshdefSB++;
            }
        }
    }
    return;
}
