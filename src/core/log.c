#include "common.h"

void LogSplitFleet(int16_t id) {
    if (gd.fGeneratingTurn) {
        LpflFromId(id)->fCompChg = TRUE;
    } else {
        WriteMemRt(rtLogFleetSplit, 2, &id);
    }
    return;
}

void LogMergeFleet(int16_t id) {
    uint16_t idCur;
    int16_t  i;
    uint16_t rgid[512];
    int16_t  j;

    if (!gd.fGeneratingTurn) {
        i = 0;
        while (i < vcflMerge) {
            rgid[0] = id;
            j = 1;
            for (; j < 511 && i < vcflMerge; i++) {
                if (vrgiflMerge[i] != iflNone) {
                    idCur = vrgiflMerge[i];
                    if (idCur != id) {
                        rgid[j++] = idCur;
                    }
                }
            }
            WriteMemRt(rtLogFleetMerge, j * 2, rgid);
        }
    }
    return;
}

void LogChangeShDef(SHDEF *lpshdefNew) {
    uint8_t  rgb[149];
    uint8_t *pb;

    if (!gd.fGeneratingTurn) {
        ((RTCHGSHDEF *)rgb)->ishdef = lpshdefNew->ishdef;
        ((RTCHGSHDEF *)rgb)->iPlr = idPlayer;
        if (lpshdefNew->fFree) {
            ((RTCHGSHDEF *)rgb)->mdChg = 0;
            WriteMemRt(rtLogShDef, 2, rgb);
        } else {
            ((RTCHGSHDEF *)rgb)->mdChg = 1;
            lpshdefNew->det = detAll;
            pb = (uint8_t *)&((RTCHGSHDEF *)rgb)->rtshdef;
            WriteRtShDef(lpshdefNew, &pb);
            WriteMemRt(rtLogShDef, pb - rgb, rgb);
        }
        if (gd.fTutorial && idPlayer == 0) {
            tutor.fChange = TRUE;
            AdvanceTutor();
        }
    }
    return;
}

void LogChangeName(GrobjClass grobj, int16_t id, char *szName) {
    FLEET    *lpfl;
    int16_t   cOut;
    RTCHGNAME rtchgname;

    lpfl = LpflFromId(id);
    if (lpfl) {
        if (lpfl->lpszName) {
            FreeLp(lpfl->lpszName, htString);
        }
        if (!szName || *szName == 0) {
            rtchgname.rgb[0] = 0;
            rtchgname.rgb[1] = 0;
            cOut = 1;
            lpfl->lpszName = NULL;
        } else {
            cOut = strlen(szName);
            lpfl->lpszName = LpAlloc(strlen(szName) + 1, htString);
            strcpy(lpfl->lpszName, szName);
            if (FCompressUserString(szName, &rtchgname.rgb[1], &cOut)) {
                rtchgname.rgb[0] = cOut;
            } else {
                rtchgname.rgb[0] = 0;
                strcpy(&rtchgname.rgb[1], szName);
                cOut++;
            }
        }
        rtchgname.grobj = grobj;
        rtchgname.id = id;
        WriteMemRt(rtLogFleetName, cOut + 5, &rtchgname);
        if (gd.fTutorial) {
            AdvanceTutor();
        }
    }
    return;
}

void LogChangeFleet(FLEET *pfl, FLEET *pflNew) {
    int16_t   d;
    int16_t   i;
    int16_t   fChg;
    LOGXFERF  lxfNew;
    LOGXFER   lxNew;
    RTWAYPT   rtwp;
    RTSHIPINT rtsi;
    int16_t   iordNew;
    int16_t   iordOld;
    int16_t   cbWp;
    char     *pbWp;
    HDR       hdr;

    fChg = FALSE;
    if (gd.fGeneratingTurn)
        return;
    lxfNew.id = pfl->id;
    lxfNew.grobj = grobjFleet;
    for (i = 0; i < 16; i++) {
        lxfNew.rgdItem[i] = pflNew->rgcsh[i] - pfl->rgcsh[i];
        if (pflNew->rgcsh[i] - pfl->rgcsh[i] != 0) {
            fChg = TRUE;
        }
    }
    if (fChg) {
        if (fValidLxf) {
            LogMakeValidXferf(&lxf, &lxfNew);
            fValidLxf = FALSE;
        } else {
            lxf = lxfNew;
            fValidLxf = TRUE;
        }
        return;
    }
    lxNew.id = pfl->id;
    lxNew.grobj = grobjFleet;
    for (i = 0; i < 5; i++) {
        lxNew.rgdItem[i] = pflNew->rgwtMin[i] - pfl->rgwtMin[i];
        if (pflNew->rgwtMin[i] - pfl->rgwtMin[i] != 0) {
            fChg = TRUE;
        }
    }
    if (fChg) {
        if (fValidLx) {
            LogMakeValidXfer(&lx, &lxNew);
            fValidLx = FALSE;
        } else {
            lx = lxNew;
            fValidLx = TRUE;
        }
        return;
    }
    if (pfl->iplan != pflNew->iplan) {
        rtsi.id = pflNew->id;
        rtsi.i = pflNew->iplan;
        WriteMemRt(rtLogFleetPlan, 4, &rtsi);
    }
    if (pfl->fRepOrders != pflNew->fRepOrders) {
        rtsi.id = pflNew->id;
        rtsi.i = pflNew->fRepOrders;
        WriteMemRt(rtLogFleetFlagBit9, 4, &rtsi);
    }
    d = pflNew->cord - pfl->cord;
    for (iordOld = 0;
         iordOld < pfl->cord && iordOld < pflNew->cord && memcmp(&pfl->lpplord->rgord[iordOld], &pflNew->lpplord->rgord[iordOld], sizeof(ORDER)) == 0;
         iordOld++) {
    }
    iordNew = iordOld;
    if (iordOld == pfl->cord && d == 0)
        goto NextTest;
    if (d < 0) {
        rtsi.id = pflNew->id;
        rtsi.i = iordOld;
        if (d == -2) {
            rtsi.i |= 0x8000;
        }
        WriteMemRt(rtLogFleetOrderDelete, 4, &rtsi);
        goto NextTest;
    } else if (d > 0) {
        cbWp = 22;
        rtwp.id = pflNew->id;
        rtwp.iWaypt = iordNew;
        rtwp.order = pflNew->lpplord->rgord[iordNew];
        pbWp = (char *)&rtwp;
        while (cbWp-- > 0 && pbWp[cbWp] == 0) {
        }
        cbWp++;
        WriteMemRt(rtLogFleetOrderInsert, cbWp, &rtwp);
        goto NextTest;
    } else {
        cbWp = 22;
        if (FGetPrevLogRt(&hdr, rgbCur) && hdr.rt == rtLogFleetOrderUpdate && ((RTWAYPT *)rgbCur)->id == pflNew->id && ((RTWAYPT *)rgbCur)->iWaypt == iordNew) {
            imemLogCur = imemLogPrev;
        }
        rtwp.id = pflNew->id;
        rtwp.iWaypt = iordNew;
        rtwp.order = pflNew->lpplord->rgord[iordNew];
        pbWp = (char *)&rtwp;
        while (cbWp-- > 0 && pbWp[cbWp] == 0) {
        }
        cbWp++;
        WriteMemRt(rtLogFleetOrderUpdate, cbWp, &rtwp);
    }
NextTest:
    return;
}

void LogChangeRelations() {
    HDR hdr;

    if (FGetPrevLogRt(&hdr, rgbCur) && hdr.rt == rtLogRelations) {
        imemLogCur = imemLogPrev;
    }
    WriteMemRt(rtLogRelations, game.cPlayer, rgplr[idPlayer].rgmdRelation);
    if (gd.fTutorial && idPlayer == 0) {
        tutor.fChange = TRUE;
        AdvanceTutor();
    }
    return;
}

void LogChangeBtlplan(BTLPLAN *pbtlplan) {
    WriteBattlePlan(pbtlplan, TRUE);
    if (gd.fTutorial && idPlayer == 0) {
        tutor.fChange = TRUE;
        AdvanceTutor();
    }
    return;
}

void LogChangePlanet(PLANET *ppl, PLANET *pplNew) {
    int16_t i;
    int16_t fChg;
    HDR     hdr;
    LOGXFER lxNew;

    fChg = FALSE;
    if (!gd.fGeneratingTurn) {
        if (!ppl) {
            if (!fValidLx) {
                return;
            }
            lxNew.id = -1;
            lxNew.grobj = grobjOther;
            for (i = 0; i < 5; i++) {
                lxNew.rgdItem[i] = -lx.rgdItem[i];
            }
            goto ChgIt;
        }

        lxNew.id = ppl->id;
        lxNew.grobj = grobjPlanet;
        for (i = 0; i < 4; i++) {
            lxNew.rgdItem[i] = pplNew->rgwtMin[i] - ppl->rgwtMin[i];
            if (pplNew->rgwtMin[i] - ppl->rgwtMin[i] != 0) {
                fChg = TRUE;
            }
        }
        lxNew.rgdItem[4] = 0;
        if (fChg) {
        ChgIt:
            if (fValidLx) {
                LogMakeValidXfer(&lx, &lxNew);
                fValidLx = FALSE;
            } else {
                lx = lxNew;
                fValidLx = TRUE;
            }
        }
        if (ppl) {
            if (!pplNew->lpplprod && ppl->lpplprod) {
                WriteMemRt(rtLogPlanetProdQ, 2, &lxNew);
            } else if (pplNew->lpplprod && (!ppl->lpplprod || ppl->lpplprod->iprodMac != pplNew->lpplprod->iprodMac ||
                                            memcmp(ppl->lpplprod->rgprod, pplNew->lpplprod->rgprod, ppl->lpplprod->iprodMac * 4) != 0)) {
                if (FGetPrevLogRt(&hdr, rgbCur) && hdr.rt == rtLogPlanetProdQ && ((RTCHGPRODQ *)rgbCur)->id == ppl->id) {
                    imemLogCur = imemLogPrev;
                }
                ((RTCHGPRODQ *)rgbCur)->id = ppl->id;
                memmove(&rgbCur[2], pplNew->lpplprod->rgprod, pplNew->lpplprod->iprodMac * 4);
                WriteMemRt(rtLogPlanetProdQ, pplNew->lpplprod->iprodMac * 4 + 2, rgbCur);
            }
            if ((uint32_t)ppl->fNoResearch != pplNew->fNoResearch || ppl->idFling != pplNew->idFling || ppl->iWarpFling != pplNew->iWarpFling ||
                ppl->idRoute != pplNew->idRoute) {
                ((RTCHGPLANETLONG *)rgbCur)->id = pplNew->id;
                ((RTCHGPLANETLONG *)rgbCur)->ul = 0;
                ((RTCHGPLANETLONG *)rgbCur)->fNoResearch = pplNew->fNoResearch;
                ((RTCHGPLANETLONG *)rgbCur)->idFling = pplNew->idFling;
                ((RTCHGPLANETLONG *)rgbCur)->iWarpFling = pplNew->iWarpFling;
                ((RTCHGPLANETLONG *)rgbCur)->idRoute = pplNew->idRoute;
                WriteMemRt(rtLogPlanetRouting, 6, rgbCur);
            }
        }
    }
    return;
}

void LogChangeThing(THING *lpth, THING *pthNew) {
    int16_t i;
    int16_t fChg;
    LOGXFER lxNew;

    fChg = FALSE;
    if (!gd.fGeneratingTurn) {
        memset(&lxNew, 0, sizeof(LOGXFER));
        lxNew.id = pthNew->idFull;
        lxNew.grobj = grobjThing;
        for (i = 0; i < 3; i++) {
            lxNew.rgdItem[i] = (int16_t)(pthNew->thp.rgwtMin[i] - lpth->thp.rgwtMin[i]);
            if ((int16_t)(pthNew->thp.rgwtMin[i] - lpth->thp.rgwtMin[i]) != 0) {
                fChg = TRUE;
            }
        }
        if (fChg) {
            if (fValidLx) {
                LogMakeValidXfer(&lx, &lxNew);
                fValidLx = FALSE;
            } else {
                lx = lxNew;
                fValidLx = TRUE;
            }
        }
    }
    return;
}

void LogMakeValidXfer(LOGXFER *plx1, LOGXFER *plx2) {
    int32_t  rgQuan[5];
    RTXFER  *prt;
    int16_t  iOff;
    RTXFERL *prtl;
    int16_t  rt;
    int16_t  i;
    char     rgbuf[28];
    int16_t  grbit;
    RTXFERX *prtx;
    int16_t  grFlag;
    int32_t  iBiggest;
    int16_t  cb;

    iBiggest = 0;
    grbit = 0;
    grFlag = 1;
    rgQuan[0] = 0;
    rgQuan[1] = 0;
    rgQuan[2] = 0;
    rgQuan[3] = 0;
    rgQuan[4] = 0;
    switch (hdrPrev.rt) {
    case rtLogCargoXfer8:
    case rtLogCargoXfer16:
    case rtLogCargoXfer32:
        prt = (RTXFER *)(lpLog + (imemLogCur + -hdrPrev.cb));
        break;
    default:
        prt = NULL;
    }
    if (prt && prt->grobj1 == (plx1->grobj & 0xff) && prt->grobj2 == (plx2->grobj & 0xff) && prt->id1 == plx1->id && prt->id2 == plx2->id) {
        grbit = prt->grbitItems;
        iOff = 0;
        switch (hdrPrev.rt) {
        case rtLogCargoXfer8:
            for (i = 0; i < 5; i++) {
                if (1 << i & grbit) {
                    rgQuan[i] = (int16_t)prt->rgcQuan[iOff];
                    iOff++;
                }
            }
            break;
        case rtLogCargoXfer16:
            prtx = (RTXFERX *)prt;
            for (i = 0; i < 5; i++) {
                if (1 << i & grbit) {
                    rgQuan[i] = prtx->rgcQuan[iOff];
                    iOff++;
                }
            }
            break;
        case rtLogCargoXfer32:
            prtl = (RTXFERL *)prt;
            for (i = 0; i < 5; i++) {
                if (1 << i & grbit) {
                    rgQuan[i] = prtl->rgcQuan[iOff];
                    iOff++;
                }
            }
        }
        CancelMemRt(hdrPrev.rt);
    }
    i = 0;
    grbit = 0;
    while (i < 5) {
        rgQuan[i] += plx1->rgdItem[i];
        if (iBiggest <= labs(rgQuan[i])) {
            iBiggest = labs(rgQuan[i]);
        }
        if (rgQuan[i] != 0) {
            grbit |= grFlag;
        }
        i++;
        grFlag *= 2;
    }
    if (grbit != 0) {
        prt = (RTXFER *)rgbuf;
        prt->grobj1 = plx1->grobj & 0xf;
        prt->grobj2 = plx2->grobj & 0xf;
        prt->id1 = plx1->id;
        prt->id2 = plx2->id;
        prt->grbitItems = grbit;
        cb = 6;
        iOff = 0;
        if (iBiggest <= 127) {
            rt = 1;
            for (i = 0; i < 5; i++) {
                if (rgQuan[i] != 0) {
                    prt->rgcQuan[iOff++] = rgQuan[i];
                    cb++;
                }
            }
        } else if (iBiggest <= 32767) {
            rt = 2;
            prtx = (RTXFERX *)rgbuf;
            for (i = 0; i < 5; i++) {
                if (rgQuan[i] != 0) {
                    prtx->rgcQuan[iOff++] = LOWORD(rgQuan[i]);
                    cb += 2;
                }
            }
        } else {
            rt = 25;
            prtl = (RTXFERL *)rgbuf;
            for (i = 0; i < 5; i++) {
                if (rgQuan[i] != 0) {
                    prtl->rgcQuan[iOff++] = rgQuan[i];
                    cb += 4;
                }
            }
        }
        WriteMemRt(rt, cb, rgbuf);
    }
    return;
}

void LogMakeValidXferf(LOGXFERF *plxf1, LOGXFERF *plxf2) {
    RTXFERF *prt;
    int16_t  iOff;
    int16_t  i;
    char     rgbuf[41];
    uint16_t grbit;
    int16_t  grFlag;
    int16_t  cb;

    grbit = 0;
    grFlag = 1;
    i = 0;
    while (i < 16) {
        if (plxf1->rgdItem[i] != 0) {
            grbit |= grFlag;
        }
        i++;
        grFlag *= 2;
    }
    if (grbit != 0) {
        prt = (RTXFERF *)&rgbuf;
        prt->grobj1 = plxf1->grobj & 0xf;
        prt->grobj2 = plxf2->grobj & 0xf;
        prt->id1 = plxf1->id;
        prt->id2 = plxf2->id;
        prt->grbitItems = grbit;
        cb = 7;
        iOff = 0;
        for (i = 0; i < 16; i++) {
            if (plxf1->rgdItem[i] != 0) {
                prt->rgcQuan[iOff++] = plxf1->rgdItem[i];
                cb += 2;
            }
        }
        WriteMemRt(rtLogFleetCargoXfer, cb, rgbuf);
    }
    return;
}

void CancelMemRt(RecordType rt) {
    imemLogCur -= hdrPrev.cb + 2;
    hdrPrev.rt = rtEOF;
    return;
}

void WriteMemRt(RecordType rt, int16_t cb, void *rg) {
    HDR      hdr;
    uint8_t *lpv;

    if (!fLogOff) {
        if (imemLogCur + cb + 2 > 32000) {
            AlertSz(PszFormatIds(idsLogFileHasReachedMaximumAllowableSize, NULL), MB_ICONHAND);
        }
        DirtyGame(TRUE);
        imemLogPrev = imemLogCur;
        hdr.cb = cb;
        hdr.rt = rt;
        lpv = lpLog;
        lpv += imemLogCur;
        *(HDR *)lpv = hdr;
        if (cb > 0) {
            memcpy(lpv + 2, rg, cb);
        }
        imemLogCur += cb + 2;
        if (rt != rtEOF) {
            hdrPrev = hdr;
        }
    }
    return;
}

void DirtyGame(int16_t fDirty) {
    if (fDirty != game.fDirty) {
        game.fDirty = fDirty;
        if (!fAi) {
            UpdateMsgTitle();
        }
    }
    return;
}

int16_t FGetPrevLogRt(HDR *phdr, uint8_t *pb) {
    uint8_t *lpv;

    if (imemLogPrev == -1) {
        return FALSE;
    }
    lpv = lpLog + imemLogPrev;
    *phdr = *(HDR *)lpv;
    if (phdr->cb > 0) {
        memcpy(pb, lpv + 2, phdr->cb);
    }
    return TRUE;
}

int16_t FRunLogFile() {
    int16_t fLogOld;
    int16_t fRet;
    int16_t iCur;
    HDR    *lprts;

    iCur = 0;
    fRet = TRUE;
    fLogOld = fLogOff;
    if (imemLogCur == 0) {
        return TRUE;
    }
    fLogOff = TRUE;
    for (; iCur < imemLogCur; iCur += lprts->cb + 2) {
        lprts = (HDR *)(lpLog + iCur);
        fRet &= FRunLogRecord(lprts->rt, lprts->cb, lpLog + (2 + iCur));
    }
    fLogOff = fLogOld;
    gd.fFleetLinkValid = FALSE;
    return fRet;
}

int16_t FRunLogRecord(RecordType rt, int16_t cb, uint8_t *lpb) {
    int16_t   fExtra;
    int32_t   cXfer;
    XFERFULL *lpxfCur;
    PLANET   *lppl;
    int32_t   rgcXfer[5];
    XFER      rgxf[2];
    FLEET    *lpfl;
    int16_t   ifl;
    int16_t   i;
    uint16_t  grbit;
    int16_t   rgifl[512];
    SHDEF    *lpshdef;
    int16_t   iPass;
    int16_t   iLook;
    PLANET   *lpplMac;
    int8_t    ch;
    int32_t   l;
    char      szT[33];
    int16_t   cOut;
    THING    *lpth;
    int16_t   id;
    int16_t   iColDrop;
    COLDROP  *lpcdT;
    XFERFULL *lpxfMax;
    MessageId idm;

    lpxfCur = NULL;
    switch (rt) {
    case rtEOF:
        break;
    case rtLogPlanetProdQ:
        lppl = lpPlanets;
        lpplMac = lpPlanets + cPlanet;
        for (; lppl < lpplMac && lppl->id != ((RTCHGPRODQ *)lpb)->id; lppl++) {
        }
        if (lppl != lpplMac && lppl->iPlayer == idPlayer) {
            i = (uint32_t)(cb - 2) / 4;
            if (i > 0) {
                if (!lppl->lpplprod) {
                    lppl->lpplprod = (PLPROD *)LpplAlloc(4, i + 2, htOrd);
                } else if (lppl->lpplprod->iprodMax < i) {
                    lppl->lpplprod = (PLPROD *)LpplReAlloc((PL *)lppl->lpplprod, i + 2);
                }
                for (iPass = 0; iPass < i; iPass++) {
                    if (((RTCHGPRODQ *)lpb)->rgprod[iPass].pct != 0) {
                        for (iLook = 0; iLook < lppl->lpplprod->iprodMac; iLook++) {
                            if (lppl->lpplprod->rgprod[iLook].pct != 0 && lppl->lpplprod->rgprod[iLook].iItem == ((RTCHGPRODQ *)lpb)->rgprod[iPass].iItem &&
                                lppl->lpplprod->rgprod[iLook].grobj == ((RTCHGPRODQ *)lpb)->rgprod[iPass].grobj) {
                                lppl->lpplprod->rgprod[iLook].pct = 0;
                                break;
                            }
                        }
                        if (iLook == lppl->lpplprod->iprodMac) {
                            ((RTCHGPRODQ *)lpb)->rgprod[iPass].pct = 0;
                        }
                    }
                }
                memmove(lppl->lpplprod->rgprod, ((RTCHGPRODQ *)lpb)->rgprod, i * 4);
                lppl->lpplprod->iprodMac = i;
            } else if (!lppl->lpplprod) {
                return TRUE;
            } else {
                FreeLp(lppl->lpplprod, htOrd);
                lppl->lpplprod = NULL;
            }
            break;
        }
        return FALSE;
    case rtLogRelations:
        memcpy(rgplr[idPlayer].rgmdRelation, lpb, game.cPlayer);
        break;
    case rtLogPlayerZpq1:
        if (gd.fGeneratingTurn) {
            if ((uint16_t)cb > 26) {
                return FALSE;
            }
            memcpy(&rgplr[idPlayer].zpq1, lpb, cb);
        }
        break;
    case rtLogFleetName:
        cOut = 32;
        if ((lpfl = LpflFromId(((RTCHGNAME *)lpb)->id)) == 0) {
            return FALSE;
        }
        if (lpfl->lpszName) {
            FreeLp(lpfl->lpszName, htString);
        }
        i = ((RTCHGNAME *)lpb)->rgb[0];
        if (i != 0 && FDecompressUserString(&((RTCHGNAME *)lpb)->rgb[1], i, szT, &cOut)) {
            lpfl->lpszName = LpAlloc(strlen(szT) + 1, htString);
            strcpy(lpfl->lpszName, szT);
        } else if (((RTCHGNAME *)lpb)->rgb[1] == 0) {
            lpfl->lpszName = NULL;
        } else {
            lpfl->lpszName = LpAlloc(strlen(&((RTCHGNAME *)lpb)->rgb[1]) + 1, htString);
            strcpy(lpfl->lpszName, &((RTCHGNAME *)lpb)->rgb[1]);
        }
        break;
    case rtThing:
        lpth = LpthFromId(((RTLOGTHING *)lpb)->idFull);
        if (!lpth || lpth->ith != ithMinefield) {
            return FALSE;
        }
        lpth->thm.fDetonate = ((RTLOGTHING *)lpb)->fDetonate;
        break;
    case rtLogShDef:
        i = ((RTCHGSHDEF *)lpb)->ishdef;
        iLook = ((RTCHGSHDEF *)lpb)->iPlr;
        if (iLook >= game.cPlayer || iLook != idPlayer) {
            return FALSE;
        }
        if (i >= 16) {
            if (i >= 26) {
                return FALSE;
            }
            lpshdef = rglpshdefSB[iLook] + (i - 16);
        } else {
            lpshdef = rglpshdef[iLook] + i;
        }
        if (!lpshdef->fFree && lpshdef->cExist != 0 && ((RTCHGSHDEF *)lpb)->mdChg != 0) {
            return FALSE;
        }
        /* Work done on queued ships or starbases of the old design doesn't
           carry over to the new one. The original kept PROD.pct and costed
           the rest from the new design, so a mostly built cheap starbase
           could be finished as an expensive one. */
        lppl = lpPlanets;
        lpplMac = lpPlanets + cPlanet;
        for (; lppl < lpplMac; lppl++) {
            if (lppl->iPlayer == idPlayer && lppl->lpplprod) {
                for (iPass = 0; iPass < lppl->lpplprod->iprodMac; iPass++) {
                    if (lppl->lpplprod->rgprod[iPass].grobj == grobjFleet && lppl->lpplprod->rgprod[iPass].iItem == (uint32_t)i) {
                        lppl->lpplprod->rgprod[iPass].pct = 0;
                    }
                }
            }
        }
        switch (((RTCHGSHDEF *)lpb)->mdChg) {
        case 0:
            if (lpshdef->fFree)
                break;
            DestroyAllIshdef(i, idPlayer);
            lpshdef->fFree = TRUE;
            if (i >= 16) {
                rgplr[iLook].cshdefSB += 15;
            } else {
                rgplr[iLook].cShDef--;
            }
            break;
        case 1:
            if (lpshdef->fFree) {
                if (i >= 16) {
                    rgplr[iLook].cshdefSB++;
                } else {
                    rgplr[iLook].cShDef++;
                }
            }
            if (i >= 16) {
                if (FReadShDef(&((RTCHGSHDEF *)lpb)->rtshdef, rglpshdefSB[iLook], idPlayer))
                    break;
                rgplr[iLook].cshdefSB += 15;
                return FALSE;
            }
            if (FReadShDef(&((RTCHGSHDEF *)lpb)->rtshdef, rglpshdef[iLook], idPlayer))
                break;
            rgplr[iLook].cShDef--;
            return FALSE;
        }
        break;
    case rtLogCargoXfer8:
    case rtLogCargoXfer16:
    case rtLogCargoXfer32:
        if (!FLookupObject(((RTXFER *)lpb)->grobj1, ((RTXFER *)lpb)->id1, &rgxf[0].fl)) {
            return FALSE;
        }
        rgxf[1].fl.id = idflNone;
        if (((RTXFER *)lpb)->grobj2 != grobjOther && !FLookupObject(((RTXFER *)lpb)->grobj2, ((RTXFER *)lpb)->id2, &rgxf[1].fl)) {
            if (((RTXFER *)lpb)->grobj2 == grobjFleet && (((RTXFER *)lpb)->id2 >> 9 & 0xf) != idPlayer)
                break;
            return FALSE;
        }
        grbit = ((RTXFER *)lpb)->grbitItems;
        for (i = 0, iLook = 0; i < 5; i++, grbit >>= 1) {
            if (grbit & 1) {
                if (rt == rtLogCargoXfer8) {
                    rgcXfer[i] = (int16_t)((RTXFER *)lpb)->rgcQuan[iLook];
                } else if (rt == rtLogCargoXfer16) {
                    rgcXfer[i] = ((RTXFERX *)lpb)->rgcQuan[iLook];
                } else {
                    rgcXfer[i] = ((RTXFERL *)lpb)->rgcQuan[iLook];
                }
                iLook++;
            } else {
                rgcXfer[i] = 0;
            }
        }
        for (iPass = 0; iPass < 2; iPass++) {
            for (i = 0; i < 5; i++, grbit >>= 1) {
                if (rgcXfer[i] != 0) {
                    cXfer = rgcXfer[i];
                    if ((iPass == 0 && cXfer < 0) || (iPass == 1 && cXfer >= 0)) {
                        l = ChgCargo(((RTXFER *)lpb)->grobj1, ((RTXFER *)lpb)->id1, i, cXfer, &rgxf[0].fl);
                        if (l != cXfer) {
                            id = ((RTXFER *)lpb)->grobj1 == grobjFleet ? -32768 : 0;
                            id |= rgxf[0].fl.id;
                            FSendPlrMsg(rgxf[0].fl.iPlayer, idmUnableTransferKtKtRequest, id, id, LOWORD(cXfer) - LOWORD(l), i, LOWORD(cXfer), 0, 0, 0);
                            rgcXfer[i] = l;
                        }
                    }
                    if (((iPass == 0 && cXfer >= 0) || (iPass == 1 && cXfer < 0)) && ((RTXFER *)lpb)->grobj2 != grobjOther) {
                        if (i == 3 && cXfer != 0 && gd.fGeneratingTurn && ((RTXFER *)lpb)->grobj2 == grobjPlanet && ((RTXFER *)lpb)->grobj1 == grobjFleet &&
                            rgxf[0].fl.iPlayer != rgxf[1].fl.iPlayer) {
                            lpcdT = lpcd;
                            iColDrop = 0;
                            if (cXfer > 0) {
                                for (; iColDrop < cColDrop && cXfer > 0; iColDrop++) {
                                    if (lpcdT->idPlanetDst == rgxf[1].pl.id && lpcdT->idPlr == rgxf[0].fl.iPlayer) {
                                        l = cXfer < lpcdT->cColonist ? cXfer : lpcdT->cColonist;
                                        cXfer -= l;
                                        lpcdT->cColonist -= l;
                                    }
                                    lpcdT++;
                                }
                            } else {
                                for (; iColDrop < cColDrop && (lpcdT->idFleetSrc != rgxf[0].fl.id || lpcdT->idPlanetDst != rgxf[1].pl.id); iColDrop++) {
                                    lpcdT++;
                                }
                                if (iColDrop == cColDrop) {
                                    lpcdT->idFleetSrc = rgxf[0].fl.id;
                                    lpcdT->idPlr = rgxf[0].fl.iPlayer;
                                    lpcdT->idPlanetDst = rgxf[1].fl.id;
                                    lpcdT->cColonist = 0;
                                    lpcdT->fCanColonize = rgxf[1].fl.iPlayer != iplrNone;
                                    cColDrop++;
                                }
                                lpcdT->cColonist -= cXfer;
                            }
                        } else {
                            if (cXfer != 0 && gd.fGeneratingTurn && rgxf[1].fl.iPlayer != rgxf[0].fl.iPlayer && ((RTXFER *)lpb)->grobj2 != grobjThing) {
                                lpxfMax = lpxf + cXferFull;
                                if (cXfer > 0) {
                                    for (lpxfCur = lpxf; lpxfCur < lpxfMax && cXfer > 0; lpxfCur++) {
                                        if (lpxfCur->grobj2 == ((RTXFER *)lpb)->grobj2 && lpxfCur->id2 == ((RTXFER *)lpb)->id2 && lpxfCur->grobj1 == 2 &&
                                            (lpxfCur->id1 & 0xfe00) == (((RTXFER *)lpb)->id1 & 0xfe00)) {
                                            l = cXfer < lpxfCur->rgcQuan[i] ? cXfer : lpxfCur->rgcQuan[i];
                                            cXfer -= l;
                                            lpxfCur->rgcQuan[i] -= l;
                                        }
                                    }
                                    lpxfCur = NULL;
                                    if (cXfer > 0)
                                        goto StealCargo;
                                    goto DoNext;
                                }
                                if (iPass == 1)
                                    goto StealCargo;
                                if (!lpxfCur) {
                                    for (lpxfCur = lpxf; lpxfCur < lpxfMax && memcmp(lpxfCur, lpb, 5) != 0; lpxfCur++) {
                                    }
                                    if (lpxfCur == lpxfMax) {
                                        cXferFull++;
                                        memset(lpxfCur, 0, sizeof(XFERFULL));
                                        lpxfCur->id1 = ((RTXFER *)lpb)->id1;
                                        lpxfCur->id2 = ((RTXFER *)lpb)->id2;
                                    }
                                }
                                lpxfCur->rgcQuan[i] -= cXfer;
                                goto DoNext;
                            }
                        StealCargo:
                            l = ChgCargo(((RTXFER *)lpb)->grobj2, ((RTXFER *)lpb)->id2, i, -cXfer, &rgxf[1].fl);
                            if (l != -cXfer) {
                                rgcXfer[i] = -l;
                                /* Cargo the destination can't hold goes back to
                                   the source, which gave it up in the first pass.
                                   The original destroyed it, so minerals sent to
                                   another player's fleet beyond its free hold
                                   space were lost. */
                                if (cXfer < 0) {
                                    ChgCargo(((RTXFER *)lpb)->grobj1, ((RTXFER *)lpb)->id1, i, -cXfer - l, &rgxf[0].fl);
                                }
                                if (((RTXFER *)lpb)->grobj2 == grobjThing) {
                                    idm = idmDidntGetAttemptedTransferMineralPacketAnother;
                                    if (l == 0) {
                                        idm++;
                                    }
                                    FSendPlrMsg(rgxf[0].fl.iPlayer, idm, rgxf[0].fl.id | 0x8000, rgxf[0].fl.id, i, -LOWORD(l), i, 0, 0, 0);
                                } else {
                                    id = ((RTXFER *)lpb)->grobj1 == grobjFleet ? -32768 : 0;
                                    id |= rgxf[0].fl.id;
                                    FSendPlrMsg(rgxf[0].fl.iPlayer, idmUnableTransferKtKtRequest, id, id, -LOWORD(l) - LOWORD(cXfer), i, -LOWORD(cXfer), 0, 0,
                                                0);
                                }
                            }
                        }
                    }
                }
            DoNext:;
            }
        }
        if (((RTXFER *)lpb)->grobj1 == grobjFleet) {
            FLookupFleet(idWriteBack, &rgxf[0].fl);
        } else {
            FLookupPlanet(idWriteBack, &rgxf[0].pl);
        }
        switch (((RTXFER *)lpb)->grobj2) {
        case grobjFleet:
            FLookupFleet(idWriteBack, &rgxf[1].fl);
            break;
        case grobjPlanet:
        case grobjOther:
            FLookupPlanet(idWriteBack, &rgxf[1].pl);
            break;
        case grobjThing:
            FLookupThing(idWriteBack, &rgxf[1].th);
        }
        break;
    case rtLogFleetSplit:
    case rtLogFleetMerge:
        if (!FLookupObject(grobjFleet, ((RTFLEETIDS *)lpb)->rgid[0], &rgxf[0].fl)) {
            return FALSE;
        }
        if (rt == rtLogFleetSplit) {
            if (LpflNewSplit(&rgxf[0].fl) != 0)
                break;
            return FALSE;
        }
        vrgiflMerge = rgifl;
        vcflMerge = 0;
        if (cb == 2) {
            for (ifl = 0; ifl < cFleet; ifl++) {
                lpfl = rglpfl[ifl];
                if (!rglpfl[ifl])
                    break;
                if (lpfl->iPlayer == idPlayer && !lpfl->fDead && lpfl->pt.x == rgxf[0].fl.pt.x && lpfl->pt.y == rgxf[0].fl.pt.y) {
                    rgifl[vcflMerge++] = lpfl->id;
                    lpfl->fCompChg = TRUE;
                }
            }
        } else {
            for (i = 0; i < (uint32_t)cb / 2; i++) {
                for (ifl = 0; ifl < cFleet; ifl++) {
                    lpfl = rglpfl[ifl];
                    if (!rglpfl[ifl])
                        break;
                    if (lpfl->id == ((RTFLEETIDS *)lpb)->rgid[i]) {
                        rgifl[vcflMerge++] = lpfl->id;
                        lpfl->fCompChg = TRUE;
                        break;
                    }
                }
            }
        }
        if (FFleetMergeAll(&rgxf[0].fl))
            break;
        return FALSE;
    case rtLogFleetCargoXfer:
        if (!FLookupObject(grobjFleet, ((RTXFERF *)lpb)->id1, &rgxf[0].fl)) {
            return FALSE;
        }
        if (!FLookupObject(grobjFleet, ((RTXFERF *)lpb)->id2, &rgxf[1].fl)) {
            return FALSE;
        }
        if (rgxf[1].fl.iPlayer != rgxf[0].fl.iPlayer) {
            return FALSE;
        }
        for (iPass = 0; iPass < 2; iPass++) {
            grbit = ((RTXFERF *)lpb)->grbitItems;
            i = 0;
            iLook = 0;
            while (i < 16) {
                if (grbit & 1) {
                    cXfer = ((RTXFERF *)lpb)->rgcQuan[iLook];
                    if ((iPass == 0 && cXfer < 0) || (iPass == 1 && cXfer >= 0)) {
                        if (rgxf[0].fl.rgcsh[i] + cXfer < 0) {
                            cXfer = (int16_t)-rgxf[0].fl.rgcsh[i];
                        } else if ((int16_t)(32766 - rgxf[0].fl.rgcsh[i]) <= cXfer) {
                            cXfer = (int16_t)(32766 - rgxf[0].fl.rgcsh[i] - 1);
                        }
                        rgxf[0].fl.rgcsh[i] += LOWORD(cXfer);
                    }
                    if ((iPass == 0 && cXfer >= 0) || (iPass == 1 && cXfer < 0)) {
                        if (rgxf[1].fl.rgcsh[i] - cXfer < 0) {
                            cXfer = rgxf[1].fl.rgcsh[i];
                        } else if ((int16_t)(32766 - rgxf[1].fl.rgcsh[i]) <= -cXfer) {
                            cXfer = (int16_t)-(32766 - rgxf[1].fl.rgcsh[i] - 1);
                        }
                        rgxf[1].fl.rgcsh[i] -= LOWORD(cXfer);
                    }
                    iLook++;
                }
                i++;
                grbit >>= 1;
            }
        }
        FleetTransferCargoBalance(&rgxf[0].fl, &rgxf[1].fl);
        for (iPass = 0; iPass < 2; iPass++) {
            FLookupFleet(idWriteBack, &rgxf[iPass].fl);
            lpfl = LpflFromId(rgxf[iPass].fl.id);
            if (lpfl) {
                lpfl->fCompChg = TRUE;
            }
            for (i = 0; i < 16 && rgxf[iPass].fl.rgcsh[i] == 0; i++) {
            }
            if (i == 16) {
                FDeleteFleet(rgxf[iPass].fl.id, grobjNone, 0);
            }
        }
        break;
    case rtLogFleetOrderDelete:
        if ((lpfl = LpflFromId(((RTSHIPINT *)lpb)->id)) == 0 || lpfl->cord <= 0 || (iLook = ((RTSHIPINT *)lpb)->i & 0x7fff) >= lpfl->cord) {
            return FALSE;
        }
        fExtra = (((RTSHIPINT *)lpb)->i & 0x8000) != 0;
        if (fExtra && iLook + 1 >= lpfl->cord) {
            return FALSE;
        }
        memmove(&lpfl->lpplord->rgord[iLook], &lpfl->lpplord->rgord[iLook + fExtra + 1], (lpfl->cord - iLook - fExtra - 1) * sizeof(ORDER));
        lpfl->cord -= fExtra + 1;
        lpfl->lpplord->iordMac -= fExtra + 1;
        break;
    case rtLogFleetOrderInsert:
        if ((lpfl = LpflFromId(((RTWAYPT *)lpb)->id)) == 0 || ((RTWAYPT *)lpb)->iWaypt < 0 || ((RTWAYPT *)lpb)->iWaypt > lpfl->cord) {
            return FALSE;
        }
        if (lpfl->cord == lpfl->lpplord->iordMax) {
            lpfl->lpplord = (PLORD *)LpplReAlloc((PL *)lpfl->lpplord, lpfl->cord + 3);
        }
        memmove(&lpfl->lpplord->rgord[((RTWAYPT *)lpb)->iWaypt + 1], &lpfl->lpplord->rgord[((RTWAYPT *)lpb)->iWaypt],
                (lpfl->cord - ((RTWAYPT *)lpb)->iWaypt) * sizeof(ORDER));
        if ((uint16_t)cb < 22) {
            memset(&lpfl->lpplord->rgord[((RTWAYPT *)lpb)->iWaypt], 0, sizeof(ORDER));
        }
        memmove(&lpfl->lpplord->rgord[((RTWAYPT *)lpb)->iWaypt], &((RTWAYPT *)lpb)->order, cb - 4);
        lpfl->lpplord->rgord[((RTWAYPT *)lpb)->iWaypt].fNoAutoTrack = FALSE;
        lpfl->cord++;
        lpfl->lpplord->iordMac++;
        break;
    case rtLogFleetOrderUpdate:
        if ((lpfl = LpflFromId(((RTWAYPT *)lpb)->id)) == 0 || lpfl->cord < 0 || (iLook = ((RTWAYPT *)lpb)->iWaypt) >= lpfl->cord) {
            return FALSE;
        }
        if ((uint16_t)cb < 22) {
            memset(&lpfl->lpplord->rgord[iLook], 0, sizeof(ORDER));
        }
        memmove(&lpfl->lpplord->rgord[iLook], &((RTWAYPT *)lpb)->order, cb - 4);
        lpfl->lpplord->rgord[iLook].fNoAutoTrack = FALSE;
        break;
    case rtBtlPlan:
        i = ((BTLPLAN *)lpb)->iplan;
        if (((BTLPLAN *)lpb)->iplr != idPlayer || i < 0 || i > rgcbtlplan[idPlayer]) {
            if (((BTLPLAN *)lpb)->fDelete != 0)
                break;
            goto BombOut;
        }
        if (((BTLPLAN *)lpb)->fDelete != 0) {
            FDeleteBattlePlan(i, FALSE);
            break;
        }
        if (((BTLPLAN *)lpb)->mdTactic > 6 || ((BTLPLAN *)lpb)->mdTarget1 > 8 || ((BTLPLAN *)lpb)->mdTarget2 > 8)
            goto BombOut;
        if (i == rgcbtlplan[idPlayer]) {
            if (i >= 16)
                goto BombOut;
            rgcbtlplan[idPlayer]++;
        }
        UnpackBattlePlan(lpb, rglpbtlplan[idPlayer] + i, i);
        break;
    case rtLogFleetPlan:
        if ((lpfl = LpflFromId(((RTSHIPINT *)lpb)->id)) == 0)
            goto BombOut;
        lpfl->iplan = ((RTSHIPINT *)lpb)->i;
        break;
    case rtLogFleetFlagBit9:
    case rtLogFleetOrderAttrNib:
        if ((lpfl = LpflFromId(((RTSHIPINT *)lpb)->id)) == 0) {
        BombOut:
            return FALSE;
        }
        if (rt == rtLogFleetFlagBit9) {
            lpfl->fRepOrders = ((RTSHIPINT *)lpb)->i;
            break;
        }
        if (lpfl->cord <= ((RTSHIPINT2 *)lpb)->i || ((RTSHIPINT2 *)lpb)->i2 >= 10)
            goto BombOut;
        lpfl->lpplord->rgord[((RTSHIPINT2 *)lpb)->i].grTask = ((RTSHIPINT2 *)lpb)->i2;
        break;
    case rtLogPlanetRouting:
        lppl = LpplFromId(((RTCHGPLANETLONG *)lpb)->id);
        if (!lppl || lppl->iPlayer != idPlayer)
            goto BombOut;
        lppl->fNoResearch = ((RTCHGPLANETLONG *)lpb)->fNoResearch;
        lppl->idFling = ((RTCHGPLANETLONG *)lpb)->idFling;
        lppl->iWarpFling = ((RTCHGPLANETLONG *)lpb)->iWarpFling;
        lppl->idRoute = ((RTCHGPLANETLONG *)lpb)->idRoute;
        break;
    case rtChgPassword:
        if (gd.fGeneratingTurn) {
            rgplr[idPlayer].lSalt = RawLoad32(lpb);
        }
        break;
    case rtLogResearch:
        ch = ((RTRESEARCH *)lpb)->pctResearch;
        if (ch < 0 || ch > 100)
            goto BombOut;
        rgplr[idPlayer].pctResearch = ch;
        if (((RTRESEARCH *)lpb)->iTechNow >= 6 || ((RTRESEARCH *)lpb)->iTechNext > 7)
            goto BombOut;
        rgplr[idPlayer].iTechCur = ((RTRESEARCH *)lpb)->iTechCur;
        break;
    }
    return TRUE;
}

int16_t FLoadLogFile(char *pszLog) {
    jmp_buf *penvMemSav;
    jmp_buf  env;
    int16_t  fRet;
    int16_t  cbLog;
    int16_t  iCur;
    MSGPLR  *lpmp;
    int16_t  cSkip;

    fRet = TRUE;
    imemLogCur = 0;
    imemLogPrev = -1;
    penvMemSav = penvMem;
    penvMem = &env;
    if (setjmp(env) != 0) {
        penvMem = penvMemSav;
        if (vlpMemStream) {
            return FALSE;
        }
        if (hf == -1) {
            return TRUE;
        }
        StreamClose();
        return FALSE;
    }
    if (game.fTutorial && idPlayer == 0 && gd.fGeneratingTurn) {
        cSkip = game.turn;
        vlpMemStream = LpbLoadTutorLog();
        if (!vlpMemStream) {
            penvMem = penvMemSav;
            return FALSE;
        }
        if (game.turn >= *vlpMemStream) {
            vlpMemStream = NULL;
            goto StrOpen;
        }
        vlpMemStream++;
        while (cSkip-- != 0) {
            do {
                vlpMemStream += 2 + ((HDR *)vlpMemStream)->cb;
            } while (((HDR *)vlpMemStream)->rt != rtBOF);
        }
    } else {
    StrOpen:
        StreamOpen(pszLog, 16416);
    }

    ReadRt();
    if (game.lid != ((RTBOF *)rgbCur)->lidGame || game.turn > ((RTBOF *)rgbCur)->turn) {
    FailSuccess:
        if (vlpMemStream) {
            vlpMemStream = NULL;
        } else {
            StreamClose();
        }
        penvMem = penvMemSav;
        return TRUE;
    }
    if (((RTBOF *)rgbCur)->turn != game.turn) {
        FileError(idsLogFileRecentGameTryingLoadIgnoring);
        goto FailSuccess;
    }
    if (((RTBOF *)rgbCur)->wGen != game.wGen) {
        FileError(idsFileGame);
        goto FailSuccess;
    }
    wVersFile = ((RTBOF *)rgbCur)->wVersion;
    gd.fFileCrippled = ((RTBOF *)rgbCur)->fCrippled;
    if (gd.fGeneratingTurn) {
        rgplr[idPlayer].fCrippled = ((RTBOF *)rgbCur)->fCrippled;
    }
    ReadRt();
    cbLog = ((RTLOGHDR *)rgbCur)->cbLog;
    for (iCur = 0; iCur < cbLog; iCur += hdrCur.cb + 2) {
        ReadRt();
        memmove(lpLog + iCur, &hdrCur, sizeof(HDR));
        memmove(lpLog + (2 + iCur), rgbCur, hdrCur.cb);
    }
    ReadRt();
    for (lpmp = (MSGPLR *)&vlpmsgplrOut; lpmp->lpmsgplrNext; lpmp = lpmp->lpmsgplrNext) {
    }
    while (hdrCur.rt == rtPlrMsg) {
        lpmp->lpmsgplrNext = LpmsgplrFromRt();
        if (lpmp->lpmsgplrNext) {
            lpmp = lpmp->lpmsgplrNext;
            vcmsgplrOut++;
        }
        ReadRt();
    }
    if (hdrCur.rt != rtEOF) {
        fRet = FALSE;
        goto Done;
    }
    imemLogCur = cbLog;
Done:
    if (vlpMemStream) {
        vlpMemStream = NULL;
    } else {
        StreamClose();
    }
    penvMem = penvMemSav;
    DirtyGame(FALSE);
    return fRet;
}

int16_t FCheckLogFile(int16_t iplr, int16_t *pfError) {
    jmp_buf *penvMemSav;
    jmp_buf  env;
    int16_t  fRet;
    int16_t  cbLog;
    int16_t  iCur;

    fRet = TRUE;
    imemLogCur = 0;
    imemLogPrev = -1;
    penvMemSav = penvMem;
    penvMem = &env;
    if (setjmp(env) != 0) {
        penvMem = penvMemSav;
        if (hf == -1) {
            return TRUE;
        }
        StreamClose();
        *pfError = 3;
        return FALSE;
    }
    idsFileError = 0;
    if (!FOpenFile(dtLog, iplr, 32)) {
        if (idsFileError != idsCantOpenFile) {
            *pfError = idsFileError;
        }
        return FALSE;
    }
    ReadRt();
    cbLog = ((RTLOGHDR *)rgbCur)->cbLog;
    for (iCur = 0; iCur < cbLog; iCur += hdrCur.cb + 2) {
        ReadRt();
    }
    ReadRt();
    while (hdrCur.rt == rtPlrMsg) {
        ReadRt();
    }
    if (hdrCur.rt != rtEOF) {
        *pfError = 3;
        fRet = FALSE;
        goto Done;
    }
    imemLogCur = cbLog;
Done:
    StreamClose();
    penvMem = penvMemSav;
    return fRet;
}

int16_t FWriteLogFile(char *pszFileBase, int16_t iPlayer) {
    jmp_buf *penvMemSav;
    jmp_buf  env;
    int16_t  iCur;
    HDR     *lprts;
    RTLOGHDR rtlh;
    MSGPLR  *lpmp;
    int16_t  cb;

    iCur = 0;
    if (iPlayer == idPlayer && !rgplr[iPlayer].fAi && hdrPrev.rt != rtLogPlayerZpq1) {
        cb = 26 - (12 - vrgZipProd[0].cpq) * 2;
        if (memcmp(&rgplr[iPlayer].zpq1, (uint8_t *)(ZIPPRODQ *)vrgZipProd + 14, cb) != 0) {
            WriteMemRt(rtLogPlayerZpq1, cb, (uint8_t *)(ZIPPRODQ *)vrgZipProd + 14);
        }
    }
    /* NATIVE: callers pass szBase itself, and strcpy onto itself is
       undefined. */
    if (pszFileBase != szBase) {
        strcpy(szBase, pszFileBase);
    }
    if (!FCreateFile(dtLog, iPlayer, NULL)) {
        AlertSz(PszFormatIds(idsUnableCreateLogFile, NULL), MB_ICONHAND);
        return FALSE;
    }
    penvMemSav = penvMem;
    penvMem = &env;
    if (setjmp(env) != 0) {
        penvMem = penvMemSav;
        StreamClose();
        return FALSE;
    }
    rtlh.cbLog = imemLogCur;
    /* Preserve the unused 15-byte metadata area in the orders-file header. */
    memset(rtlh.rgbUnused, 0, sizeof(rtlh.rgbUnused));
    WriteRt(9, 17, &rtlh);
    for (; iCur < imemLogCur; iCur += lprts->cb + 2) {
        lprts = (HDR *)(lpLog + iCur);
        WriteRt(lprts->rt, lprts->cb, lpLog + (2 + iCur));
    }
    iCur = vcmsgplrOut;
    lpmp = vlpmsgplrOut;
    while (iCur-- != 0) {
        WriteRtPlrMsg(lpmp);
        lpmp = lpmp->lpmsgplrNext;
    }
    WriteRt(rtEOF, 0, NULL);
    StreamClose();
    penvMem = penvMemSav;
    DirtyGame(FALSE);
    gd.fWriteTurnNum = TRUE;
    return TRUE;
}

int16_t FWriteHistFile(int16_t iPlayer) {
    PLANET   *lppl;
    int16_t   i;
    jmp_buf  *penvMemSav;
    jmp_buf   env;
    uint16_t  cTurnBase;
    SHDEF    *lpshdef;
    int16_t   j;
    RTHISTHDR rthh;
    uint8_t  *lpb;

    if (!FCreateFile(dtHist, iPlayer, NULL)) {
        AlertSz(PszFormatIds(idsUnableCreateHistoryFile, NULL), MB_ICONHAND);
        return FALSE;
    }
    penvMemSav = penvMem;
    penvMem = &env;
    if (setjmp(env) != 0) {
        penvMem = penvMemSav;
        StreamClose();
        return FALSE;
    }
    rthh.cPlanet = cPlanet;
    rthh.cPlanetExtra = rgplr[iPlayer].cFleet;
    WriteRt(rtHistHdr, 4, &rthh);
    i = 0;
    lppl = lpPlanets;
    while (i < cPlanet) {
        WritePlanet(lppl, lppl->det < detSome ? 15 : rtPlanetB, TRUE);
        i++;
        lppl++;
    }
    WriteRt(rtMsgFilt, cbbitfMsg, bitfMsgFiltered);
    for (i = 0; i < game.cPlayer; i++) {
        if (i != iPlayer && rgplr[i].det != detNone) {
            WriteRtPlr(&rgplr[i], NULL);
        }
    }
    for (i = 0; i < game.cPlayer; i++) {
        if (rgplr[i].fInclude && i != iPlayer) {
            lpshdef = rglpshdef[i];
            for (j = 0; j < 16; j++) {
                if (!lpshdef[j].fFree) {
                    WriteRtShDef(lpshdef + j, NULL);
                }
            }
        }
    }
    for (i = 0; i < game.cPlayer; i++) {
        if (rgplr[i].fInclude && i != iPlayer) {
            lpshdef = rglpshdefSB[i];
            for (j = 0; j < 10; j++) {
                if (!lpshdef[j].fFree) {
                    WriteRtShDef(lpshdef + j, NULL);
                }
            }
        }
    }
    if (game.turn <= 100) {
        cTurnBase = 0;
    } else {
        cTurnBase = game.turn - 100;
    }
    for (i = 0; i < game.cPlayer; i++) {
        if (rgsxPlr[i]) {
            for (j = 0; j < rgcsxPlr[i]; j++) {
                if (rgsxPlr[i][j].turn >= cTurnBase) {
                    WriteRt(rtScore, 24, rgsxPlr[i] + j);
                }
            }
        }
    }
    if (vlpbAiData && ((AIHIST *)vlpbAiData)->cbAiHist > 2) {
        i = ((AIHIST *)vlpbAiData)->cbAiHist;
        lpb = vlpbAiData;
        for (; i >= 1024; i -= 1023) {
            WriteRt(rtAiData, 1023, lpb);
            lpb += 1023;
        }
        WriteRt(rtAiData, i, lpb);
    }
    WriteRt(rtEOF, 0, NULL);
    StreamClose();
    penvMem = penvMemSav;
    return TRUE;
}

void EnumLogRts(int16_t (*pfn)(void *, int16_t, int16_t, void *, int16_t), void *lpPass, int16_t iPass) {
    int16_t fLogOld;
    int16_t fRet;
    int16_t iCur;
    HDR    *lprts;

    iCur = 0;
    fRet = TRUE;
    fLogOld = fLogOff;
    if (imemLogCur != 0) {
        for (; iCur < imemLogCur; iCur += lprts->cb + 2) {
            lprts = (HDR *)(lpLog + iCur);
            if (pfn(lpLog + (2 + iCur), lprts->rt, lprts->cb, lpPass, iPass) == 0)
                break;
        }
    }
    return;
}
