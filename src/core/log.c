#include "common.h"

void WriteMemResearch(int16_t wResearch) {
    uint8_t rgb[cbWireResearch];

    NATIVEBYTEWRITER(std::as_writable_bytes(std::span(rgb))).FWriteI16Le(wResearch);
    WriteMemRt(rtLogResearch, cbWireResearch, rgb);
}

void WriteMemPassword(int32_t lSalt) {
    uint8_t rgb[cbWirePassword];

    NATIVEBYTEWRITER(std::as_writable_bytes(std::span(rgb))).FWriteI32Le(lSalt);
    WriteMemRt(rtChgPassword, cbWirePassword, rgb);
}

void LogSplitFleet(int16_t id) {
    std::byte rgb[cbWireFleetId];
    if (gd.fGeneratingTurn) {
        LpflFromId(id)->SetFCompChg(TRUE);
    } else {
        NATIVEBYTEWRITER(rgb).FWriteI16Le(id);
        WriteMemRt(rtLogFleetSplit, cbWireFleetId, rgb);
    }
    return;
}

void LogMergeFleet(int16_t id) {
    uint16_t idCur;
    int16_t  i;
    uint16_t rgid[512];
    std::byte rgb[512 * cbWireFleetId];
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
            FEncodeFleetIds(std::span(rgid, j), rgb);
            WriteMemRt(rtLogFleetMerge, j * cbWireFleetId, rgb);
        }
    }
    return;
}

void LogChangeShDef(SHDEF *lpshdefNew) {
    uint8_t  rgb[149];
    uint8_t *pb;
    RTCHGSHDEF rtchg{};

    if (!gd.fGeneratingTurn) {
        rtchg.ishdef = lpshdefNew->ishdef;
        rtchg.iPlr = idPlayer;
        if (lpshdefNew->fFree) {
            rtchg.mdChg = 0;
            FEncodeRtChgShDefPrefix(rtchg, std::as_writable_bytes(std::span(rgb)));
            WriteMemRt(rtLogShDef, cbWireChgShDef, rgb);
        } else {
            rtchg.mdChg = 1;
            lpshdefNew->det = detAll;
            FEncodeRtChgShDefPrefix(rtchg, std::as_writable_bytes(std::span(rgb)));
            pb = rgb + cbWireChgShDef;
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
    uint8_t rgbName[cbWireChangeName + 33];

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
            lpfl->lpszName = (char *)LpAlloc(strlen(szName) + 1, htString);
            strcpy(lpfl->lpszName, szName);
            if (FCompressUserString(szName, (char *)&rtchgname.rgb[1], &cOut)) {
                rtchgname.rgb[0] = cOut;
            } else {
                rtchgname.rgb[0] = 0;
                strcpy((char *)&rtchgname.rgb[1], szName);
                cOut++;
            }
        }
        rtchgname.grobj = grobj;
        rtchgname.id = id;
        FEncodeChangeNamePrefix(rtchgname, std::as_writable_bytes(std::span(rgbName)).first(cbWireChangeName));
        memcpy(rgbName + cbWireChangeName, rtchgname.rgb, cOut + 1);
        WriteMemRt(rtLogFleetName, cOut + 5, rgbName);
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
    RTWAYPT rtwpPrev{};
    std::byte rgbWp[cbWireWaypt + cbWireOrder];
    std::byte rgbShipInt[cbWireShipInt];

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
        FEncodeRtShipInt(rtsi, rgbShipInt);
        WriteMemRt(rtLogFleetPlan, cbWireShipInt, rgbShipInt);
    }
    if (pfl->fRepOrders != pflNew->fRepOrders) {
        rtsi.id = pflNew->id;
        rtsi.i = pflNew->fRepOrders;
        FEncodeRtShipInt(rtsi, rgbShipInt);
        WriteMemRt(rtLogFleetFlagBit9, cbWireShipInt, rgbShipInt);
    }
    d = pflNew->cord - pfl->cord;
    for (iordOld = 0;
         iordOld < pfl->cord && iordOld < pflNew->cord && memcmp(&pfl->lpplord->rgord()[iordOld], &pflNew->lpplord->rgord()[iordOld], sizeof(ORDER)) == 0;
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
        FEncodeRtShipInt(rtsi, rgbShipInt);
        WriteMemRt(rtLogFleetOrderDelete, cbWireShipInt, rgbShipInt);
        goto NextTest;
    } else if (d > 0) {
        cbWp = 22;
        rtwp.id = pflNew->id;
        rtwp.iWaypt = iordNew;
        rtwp.order = pflNew->lpplord->rgord()[iordNew];
        FEncodeRtWaypt(rtwp, rgbWp);
        pbWp = reinterpret_cast<char *>(rgbWp);
        while (cbWp-- > 0 && pbWp[cbWp] == 0) {
        }
        cbWp++;
        WriteMemRt(rtLogFleetOrderInsert, cbWp, rgbWp);
        goto NextTest;
    } else {
        cbWp = 22;
        if (FGetPrevLogRt(&hdr, (uint8_t *)rgbCur) && hdr.rt == rtLogFleetOrderUpdate &&
            FDecodeRtWayptPrefix(std::as_bytes(std::span(rgbCur)), rtwpPrev) && rtwpPrev.id == pflNew->id && rtwpPrev.iWaypt == iordNew) {
            imemLogCur = imemLogPrev;
        }
        rtwp.id = pflNew->id;
        rtwp.iWaypt = iordNew;
        rtwp.order = pflNew->lpplord->rgord()[iordNew];
        FEncodeRtWaypt(rtwp, rgbWp);
        pbWp = reinterpret_cast<char *>(rgbWp);
        while (cbWp-- > 0 && pbWp[cbWp] == 0) {
        }
        cbWp++;
        WriteMemRt(rtLogFleetOrderUpdate, cbWp, rgbWp);
    }
NextTest:
    return;
}

void LogChangeRelations() {
    HDR hdr;

    if (FGetPrevLogRt(&hdr, (uint8_t *)rgbCur) && hdr.rt == rtLogRelations) {
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
    RTCHGPRODQ rtcpq{};
    std::byte rgbProdId[cbWireChgProdQ];
    RTCHGPLANETLONG routing{};

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
                NATIVEBYTEWRITER(rgbProdId).FWriteI16Le(lxNew.id);
                WriteMemRt(rtLogPlanetProdQ, cbWireChgProdQ, rgbProdId);
            } else if (pplNew->lpplprod && (!ppl->lpplprod || ppl->lpplprod->iprodMac != pplNew->lpplprod->iprodMac ||
                                            memcmp(ppl->lpplprod->rgprod(), pplNew->lpplprod->rgprod(), ppl->lpplprod->iprodMac * 4) != 0)) {
                if (FGetPrevLogRt(&hdr, (uint8_t *)rgbCur) && hdr.rt == rtLogPlanetProdQ &&
                    FDecodeRtChgProdQ(std::as_bytes(std::span(rgbCur)), rtcpq) && rtcpq.id == ppl->id) {
                    imemLogCur = imemLogPrev;
                }
                NATIVEBYTEWRITER(std::as_writable_bytes(std::span(rgbCur))).FWriteI16Le(ppl->id);
                FEncodeProdQueue(std::span(pplNew->lpplprod->rgprod(), pplNew->lpplprod->iprodMac),
                                 std::as_writable_bytes(std::span(rgbCur)).subspan(cbWireChgProdQ));
                WriteMemRt(rtLogPlanetProdQ, pplNew->lpplprod->iprodMac * 4 + 2, rgbCur);
            }
            if ((uint32_t)ppl->fNoResearch != pplNew->fNoResearch || ppl->idFling != pplNew->idFling || ppl->iWarpFling != pplNew->iWarpFling ||
                ppl->idRoute != pplNew->idRoute) {
                routing.id = pplNew->id;
                routing.fNoResearch = pplNew->fNoResearch;
                routing.idFling = pplNew->idFling;
                routing.iWarpFling = pplNew->iWarpFling;
                routing.idRoute = pplNew->idRoute;
                FEncodePlanetRouting(routing, std::as_writable_bytes(std::span(rgbCur)));
                WriteMemRt(rtLogPlanetRouting, cbWirePlanetRouting, rgbCur);
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
    uint8_t  *prt;
    int16_t  iOff;
    uint8_t *prtl;
    int16_t  rt;
    int16_t  i;
    uint8_t  rgbuf[28];
    RTXFER transfer{};
    int16_t  grbit;
    uint8_t *prtx;
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
        prt = (uint8_t *)(lpLog + (imemLogCur + -hdrPrev.cb));
        break;
    default:
        prt = NULL;
    }
    if (prt && CxvFromPrefix(prt, 5).grobj1() == (plx1->grobj & 0xff) && CxvFromPrefix(prt, 5).grobj2() == (plx2->grobj & 0xff) && CxvFromPrefix(prt, 2).id1() == plx1->id && CxvFromPrefix(prt, 4).id2() == plx2->id) {
        grbit = CxvFromPrefix(prt, 6).grbitItems();
        iOff = 0;
        switch (hdrPrev.rt) {
        case rtLogCargoXfer8:
            for (i = 0; i < 5; i++) {
                if (1 << i & grbit) {
                    rgQuan[i] = (int16_t)CxvFromPrefix(prt, 6 + (iOff + 1) * 1).cQuan8(iOff);
                    iOff++;
                }
            }
            break;
        case rtLogCargoXfer16:
            prtx = (uint8_t *)prt;
            for (i = 0; i < 5; i++) {
                if (1 << i & grbit) {
                    rgQuan[i] = CxvFromPrefix(prtx, 6 + (iOff + 1) * 2).cQuan16(iOff);
                    iOff++;
                }
            }
            break;
        case rtLogCargoXfer32:
            prtl = (uint8_t *)prt;
            for (i = 0; i < 5; i++) {
                if (1 << i & grbit) {
                    rgQuan[i] = CxvFromPrefix(prtl, 6 + (iOff + 1) * 4).cQuan32(iOff);
                    iOff++;
                }
            }
        }
        CancelMemRt(static_cast<RecordType>(hdrPrev.rt));
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
        prt = (uint8_t *)rgbuf;
        transfer.grobj1 = plx1->grobj & 0xf;
        transfer.grobj2 = plx2->grobj & 0xf;
        transfer.id1 = plx1->id;
        transfer.id2 = plx2->id;
        transfer.grbitItems = grbit;
        FEncodeCargoXferPrefix(transfer, std::as_writable_bytes(std::span(rgbuf)));
        cb = 6;
        iOff = 0;
        if (iBiggest <= 127) {
            rt = 1;
            for (i = 0; i < 5; i++) {
                if (rgQuan[i] != 0) {
                    CxvFromPrefix(prt, 6 + (iOff + 1) * 1).SetCQuan8(iOff++, rgQuan[i]);
                    cb++;
                }
            }
        } else if (iBiggest <= 32767) {
            rt = 2;
            prtx = (uint8_t *)rgbuf;
            for (i = 0; i < 5; i++) {
                if (rgQuan[i] != 0) {
                    CxvFromPrefix(prtx, 6 + (iOff + 1) * 2).SetCQuan16(iOff++, LOWORD(rgQuan[i]));
                    cb += 2;
                }
            }
        } else {
            rt = 25;
            prtl = (uint8_t *)rgbuf;
            for (i = 0; i < 5; i++) {
                if (rgQuan[i] != 0) {
                    CxvFromPrefix(prtl, 6 + (iOff + 1) * 4).SetCQuan32(iOff++, rgQuan[i]);
                    cb += 4;
                }
            }
        }
        WriteMemRt(static_cast<RecordType>(rt), cb, rgbuf);
    }
    return;
}

void LogMakeValidXferf(LOGXFERF *plxf1, LOGXFERF *plxf2) {
    RTXFERF rtxf{};
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
        rtxf.grobj1 = plxf1->grobj & 0xf;
        rtxf.grobj2 = plxf2->grobj & 0xf;
        rtxf.id1 = plxf1->id;
        rtxf.id2 = plxf2->id;
        rtxf.grbitItems = grbit;
        FEncodeRtXferFHeader(rtxf, std::as_writable_bytes(std::span(rgbuf)));
        NATIVEBYTEWRITER output{std::as_writable_bytes(std::span(rgbuf)).subspan(cbWireXferF)};
        cb = cbWireXferF;
        iOff = 0;
        for (i = 0; i < 16; i++) {
            if (plxf1->rgdItem[i] != 0) {
                output.FWriteI16Le(plxf1->rgdItem[i]);
                iOff++;
            }
        }
        cb = cbWireXferF + iOff * 2;
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
        FEncodeHdr(hdr, std::as_writable_bytes(std::span(lpv, cbWireHdr)));
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

void STARS_TEST_IMPL(DirtyGame)(int16_t fDirty) {
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
    FDecodeHdr(std::as_bytes(std::span(lpv, cbWireHdr)), *phdr);
    if (phdr->cb > 0) {
        memcpy(pb, lpv + 2, phdr->cb);
    }
    return TRUE;
}

int16_t FRunLogFile() {
    int16_t fLogOld;
    int16_t fRet;
    int16_t iCur;
    uint8_t *lprts;

    iCur = 0;
    fRet = TRUE;
    fLogOld = fLogOff;
    if (imemLogCur == 0) {
        return TRUE;
    }
    fLogOff = TRUE;
    for (; iCur < imemLogCur; iCur += HdvFromPrefix(lprts).cb() + 2) {
        lprts = lpLog + iCur;
        fRet &= FRunLogRecord(static_cast<RecordType>(HdvFromPrefix(lprts).rt()), HdvFromPrefix(lprts).cb(), lpLog + (2 + iCur));
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
    THING    *lpth;
    int16_t   id;
    int16_t   iColDrop;
    COLDROP  *lpcdT;
    XFERFULL *lpxfMax;
    MessageId idm;
    RTCHGPRODQ rtcpq{};
    PROD prod{};
    int16_t iordLog, grTaskLog;
    uint16_t wOrderLog;
    RTCHGSHDEF rtchgshdef{};
    int16_t idFleet, cShipXfer;

    lpxfCur = NULL;
    switch (rt) {
    default:
        break;
    case rtEOF:
        break;
    case rtLogPlanetProdQ:
        lppl = lpPlanets;
        lpplMac = lpPlanets + cPlanet;
        // Existing log replay consumes the ID prefix even for a short logical
        // record. Keep that admission policy while reading its bytes explicitly.
        if (lppl < lpplMac) FDecodeRtChgProdQ(std::as_bytes(std::span(lpb, cbWireChgProdQ)), rtcpq);
        for (; lppl < lpplMac && lppl->id != rtcpq.id; lppl++) {
        }
        if (lppl != lpplMac && lppl->iPlayer == idPlayer) {
            i = (uint32_t)(cb - 2) / 4;
            if (i > 0) {
                if (!lppl->lpplprod) {
                    lppl->lpplprod = LpplAlloc<PLPROD>(4, i + 2, htOrd);
                } else if (lppl->lpplprod->iprodMax < i) {
                    lppl->lpplprod = LpplReAlloc(lppl->lpplprod, i + 2);
                }
                for (iPass = 0; iPass < i; iPass++) {
                    FDecodeProd(std::as_bytes(std::span(lpb + cbWireChgProdQ + iPass * cbWireProd, cbWireProd)), prod);
                    if (prod.pct != 0) {
                        for (iLook = 0; iLook < lppl->lpplprod->iprodMac; iLook++) {
                            if (lppl->lpplprod->rgprod()[iLook].pct != 0 && lppl->lpplprod->rgprod()[iLook].iItem == prod.iItem &&
                                lppl->lpplprod->rgprod()[iLook].grobj == prod.grobj) {
                                lppl->lpplprod->rgprod()[iLook].pct = 0;
                                break;
                            }
                        }
                        if (iLook == lppl->lpplprod->iprodMac) {
                            prod.pct = 0;
                            FEncodeProd(prod, std::as_writable_bytes(std::span(lpb + cbWireChgProdQ + iPass * cbWireProd, cbWireProd)));
                        }
                    }
                }
                FDecodeProdBytes(std::as_bytes(std::span(lpb + cbWireChgProdQ, i * cbWireProd)),
                                 std::span(lppl->lpplprod->rgprod(), i));
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
        if (cb < (int16_t)cbWireChangeName ||
            !FReadUserString(lpb + cbWireChangeName, cb - cbWireChangeName, szT, 32, NULL)) return FALSE;
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb, 2))).FReadI16Le(id);
        if ((lpfl = LpflFromId(id)) == 0) return FALSE;
        if (lpfl->lpszName) FreeLp(lpfl->lpszName, htString);
        lpfl->lpszName = NULL;
        if (*szT) {
            lpfl->lpszName = (char *)LpAlloc(strlen(szT) + 1, htString);
            strcpy(lpfl->lpszName, szT);
        }
        break;
    case rtThing:
        lpth = LpthFromId(LtvFromPrefix(lpb, 2).idFull());
        if (!lpth || lpth->ith != ithMinefield) {
            return FALSE;
        }
        lpth->thm.fDetonate = LtvFromPrefix(lpb, 4).fDetonate();
        break;
    case rtLogShDef:
        if (cb < (int16_t)cbWireChgShDef) return FALSE;
        FDecodeRtChgShDefPrefix(std::as_bytes(std::span(lpb, cbWireChgShDef)), rtchgshdef);
        i = rtchgshdef.ishdef;
        iLook = rtchgshdef.iPlr;
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
        if (!lpshdef->fFree && lpshdef->cExist != 0 && rtchgshdef.mdChg != 0) {
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
                    if (lppl->lpplprod->rgprod()[iPass].grobj == grobjFleet && lppl->lpplprod->rgprod()[iPass].iItem == (uint32_t)i) {
                        lppl->lpplprod->rgprod()[iPass].pct = 0;
                    }
                }
            }
        }
        switch (rtchgshdef.mdChg) {
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
                if (FReadShDef(lpb + cbWireChgShDef, rglpshdefSB[iLook], idPlayer, cb - cbWireChgShDef))
                    break;
                rgplr[iLook].cshdefSB += 15;
                return FALSE;
            }
            if (FReadShDef(lpb + cbWireChgShDef, rglpshdef[iLook], idPlayer, cb - cbWireChgShDef))
                break;
            rgplr[iLook].cShDef--;
            return FALSE;
        }
        break;
    case rtLogCargoXfer8:
    case rtLogCargoXfer16:
    case rtLogCargoXfer32:
        if (!FLookupObject(static_cast<GrobjClass>(CxvFromPrefix(lpb, 5).grobj1()), CxvFromPrefix(lpb, 2).id1(), &rgxf[0].fl)) {
            return FALSE;
        }
        rgxf[1].fl.id = idflNone;
        if (CxvFromPrefix(lpb, 5).grobj2() != grobjOther && !FLookupObject(static_cast<GrobjClass>(CxvFromPrefix(lpb, 5).grobj2()), CxvFromPrefix(lpb, 4).id2(), &rgxf[1].fl)) {
            if (CxvFromPrefix(lpb, 5).grobj2() == grobjFleet && (CxvFromPrefix(lpb, 4).id2() >> 9 & 0xf) != idPlayer)
                break;
            return FALSE;
        }
        grbit = CxvFromPrefix(lpb, 6).grbitItems();
        for (i = 0, iLook = 0; i < 5; i++, grbit >>= 1) {
            if (grbit & 1) {
                if (rt == rtLogCargoXfer8) {
                    rgcXfer[i] = (int16_t)CxvFromPrefix(lpb, 6 + (iLook + 1) * 1).cQuan8(iLook);
                } else if (rt == rtLogCargoXfer16) {
                    rgcXfer[i] = CxvFromPrefix(lpb, 6 + (iLook + 1) * 2).cQuan16(iLook);
                } else {
                    rgcXfer[i] = CxvFromPrefix(lpb, 6 + (iLook + 1) * 4).cQuan32(iLook);
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
                        l = ChgCargo(static_cast<GrobjClass>(CxvFromPrefix(lpb, 5).grobj1()), CxvFromPrefix(lpb, 2).id1(), static_cast<MineralType>(i), cXfer, &rgxf[0].fl);
                        if (l != cXfer) {
                            id = CxvFromPrefix(lpb, 5).grobj1() == grobjFleet ? -32768 : 0;
                            id |= rgxf[0].fl.id;
                            FSendPlrMsg(rgxf[0].fl.iPlayer, idmUnableTransferKtKtRequest, static_cast<MsgGoto>(id), id, LOWORD(cXfer) - LOWORD(l), i, LOWORD(cXfer), 0, 0, 0);
                            rgcXfer[i] = l;
                        }
                    }
                    if (((iPass == 0 && cXfer >= 0) || (iPass == 1 && cXfer < 0)) && CxvFromPrefix(lpb, 5).grobj2() != grobjOther) {
                        if (i == 3 && cXfer != 0 && gd.fGeneratingTurn && CxvFromPrefix(lpb, 5).grobj2() == grobjPlanet && CxvFromPrefix(lpb, 5).grobj1() == grobjFleet &&
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
                            if (cXfer != 0 && gd.fGeneratingTurn && rgxf[1].fl.iPlayer != rgxf[0].fl.iPlayer && CxvFromPrefix(lpb, 5).grobj2() != grobjThing) {
                                lpxfMax = lpxf + cXferFull;
                                if (cXfer > 0) {
                                    for (lpxfCur = lpxf; lpxfCur < lpxfMax && cXfer > 0; lpxfCur++) {
                                        if (lpxfCur->grobj2 == CxvFromPrefix(lpb, 5).grobj2() && lpxfCur->id2 == CxvFromPrefix(lpb, 4).id2() && lpxfCur->grobj1 == 2 &&
                                            (lpxfCur->id1 & 0xfe00) == (CxvFromPrefix(lpb, 2).id1() & 0xfe00)) {
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
                                        lpxfCur->id1 = CxvFromPrefix(lpb, 2).id1();
                                        lpxfCur->id2 = CxvFromPrefix(lpb, 4).id2();
                                    }
                                }
                                lpxfCur->rgcQuan[i] -= cXfer;
                                goto DoNext;
                            }
                        StealCargo:
                            l = ChgCargo(static_cast<GrobjClass>(CxvFromPrefix(lpb, 5).grobj2()), CxvFromPrefix(lpb, 4).id2(), static_cast<MineralType>(i), -cXfer, &rgxf[1].fl);
                            if (l != -cXfer) {
                                rgcXfer[i] = -l;
                                /* Cargo the destination can't hold goes back to
                                   the source, which gave it up in the first pass.
                                   The original destroyed it, so minerals sent to
                                   another player's fleet beyond its free hold
                                   space were lost. */
                                if (cXfer < 0) {
                                    ChgCargo(static_cast<GrobjClass>(CxvFromPrefix(lpb, 5).grobj1()), CxvFromPrefix(lpb, 2).id1(), static_cast<MineralType>(i), -cXfer - l, &rgxf[0].fl);
                                }
                                if (CxvFromPrefix(lpb, 5).grobj2() == grobjThing) {
                                    idm = idmDidntGetAttemptedTransferMineralPacketAnother;
                                    if (l == 0) {
                                        idm = static_cast<MessageId>(idm + 1);
                                    }
                                    FSendPlrMsg(rgxf[0].fl.iPlayer, idm, static_cast<MsgGoto>(rgxf[0].fl.id | 0x8000), rgxf[0].fl.id, i, -LOWORD(l), i, 0, 0, 0);
                                } else {
                                    id = CxvFromPrefix(lpb, 5).grobj1() == grobjFleet ? -32768 : 0;
                                    id |= rgxf[0].fl.id;
                                    FSendPlrMsg(rgxf[0].fl.iPlayer, idmUnableTransferKtKtRequest, static_cast<MsgGoto>(id), id, -LOWORD(l) - LOWORD(cXfer), i, -LOWORD(cXfer), 0, 0,
                                                0);
                                }
                            }
                        }
                    }
                }
            DoNext:;
            }
        }
        if (CxvFromPrefix(lpb, 5).grobj1() == grobjFleet) {
            FLookupFleet(idWriteBack, &rgxf[0].fl);
        } else {
            FLookupPlanet(idWriteBack, &rgxf[0].pl);
        }
        switch (CxvFromPrefix(lpb, 5).grobj2()) {
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
        FReadFleetId({std::as_bytes(std::span(lpb, cbWireFleetId))}, 0, idFleet);
        if (!FLookupObject(grobjFleet, idFleet, &rgxf[0].fl)) {
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
                    lpfl->SetFCompChg(TRUE);
                }
            }
        } else {
            for (i = 0; i < (uint32_t)cb / 2; i++) {
                for (ifl = 0; ifl < cFleet; ifl++) {
                    lpfl = rglpfl[ifl];
                    if (!rglpfl[ifl])
                        break;
                    FReadFleetId({std::as_bytes(std::span(lpb + i * cbWireFleetId, cbWireFleetId))}, 0, idFleet);
                    if (lpfl->id == idFleet) {
                        rgifl[vcflMerge++] = lpfl->id;
                        lpfl->SetFCompChg(TRUE);
                        break;
                    }
                }
            }
        }
        if (FFleetMergeAll(&rgxf[0].fl))
            break;
        return FALSE;
    case rtLogFleetCargoXfer:
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb, 2))).FReadI16Le(idFleet);
        if (!FLookupObject(grobjFleet, idFleet, &rgxf[0].fl)) {
            return FALSE;
        }
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb + 2, 2))).FReadI16Le(idFleet);
        if (!FLookupObject(grobjFleet, idFleet, &rgxf[1].fl)) {
            return FALSE;
        }
        if (rgxf[1].fl.iPlayer != rgxf[0].fl.iPlayer) {
            return FALSE;
        }
        for (iPass = 0; iPass < 2; iPass++) {
            NATIVEBYTEREADER(std::as_bytes(std::span(lpb + 5, 2))).FReadU16Le(grbit);
            i = 0;
            iLook = 0;
            while (i < 16) {
                if (grbit & 1) {
                    NATIVEBYTEREADER(std::as_bytes(std::span(lpb + cbWireXferF + iLook * 2, 2))).FReadI16Le(cShipXfer);
                    cXfer = cShipXfer;
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
                lpfl->SetFCompChg(TRUE);
            }
            for (i = 0; i < 16 && rgxf[iPass].fl.rgcsh[i] == 0; i++) {
            }
            if (i == 16) {
                FDeleteFleet(rgxf[iPass].fl.id, grobjNone, 0);
            }
        }
        break;
    case rtLogFleetOrderDelete:
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb, 2))).FReadI16Le(id);
        if (!(lpfl = LpflFromId(id)) || lpfl->cord <= 0) return FALSE;
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb + 2, 2))).FReadU16Le(wOrderLog);
        iLook = wOrderLog & 0x7fff;
        if (iLook >= lpfl->cord) return FALSE;
        fExtra = (wOrderLog & 0x8000) != 0;
        if (fExtra && iLook + 1 >= lpfl->cord) return FALSE;
        memmove(&lpfl->lpplord->rgord()[iLook], &lpfl->lpplord->rgord()[iLook + fExtra + 1], (lpfl->cord - iLook - fExtra - 1) * sizeof(ORDER));
        lpfl->cord -= fExtra + 1;
        lpfl->lpplord->iordMac -= fExtra + 1;
        break;
    case rtLogFleetOrderInsert:
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb, 2))).FReadI16Le(id);
        if (!(lpfl = LpflFromId(id))) return FALSE;
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb + 2, 2))).FReadI16Le(iordLog);
        if (iordLog < 0 || iordLog > lpfl->cord) return FALSE;
        if (lpfl->cord == lpfl->lpplord->iordMax) {
            lpfl->lpplord = LpplReAlloc(lpfl->lpplord, lpfl->cord + 3);
        }
        memmove(&lpfl->lpplord->rgord()[iordLog + 1], &lpfl->lpplord->rgord()[iordLog], (lpfl->cord - iordLog) * sizeof(ORDER));
        if (!FApplyOrderBytes(lpfl->lpplord, iordLog, lpb + cbWireWaypt, cb, static_cast<uint16_t>(cb) < cbWireWaypt + cbWireOrder)) return FALSE;
        lpfl->cord++;
        lpfl->lpplord->iordMac++;
        break;
    case rtLogFleetOrderUpdate:
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb, 2))).FReadI16Le(id);
        if (!(lpfl = LpflFromId(id)) || lpfl->cord < 0) return FALSE;
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb + 2, 2))).FReadI16Le(iLook);
        if (iLook >= lpfl->cord) return FALSE;
        if (!FApplyOrderBytes(lpfl->lpplord, iLook, lpb + cbWireWaypt, cb, static_cast<uint16_t>(cb) < cbWireWaypt + cbWireOrder)) return FALSE;
        break;
    case rtBtlPlan:
        if (cb < 2) return FALSE;
        // Deletions stop after the owner/plan word, before target/name bytes.
        i = BpvFromPrefix(lpb, 2).iplan();
        if (BpvFromPrefix(lpb, 2).iplr() != idPlayer || i < 0 || i > rgcbtlplan[idPlayer]) {
            if (BpvFromPrefix(lpb, 2).fDelete() != 0)
                break;
            goto BombOut;
        }
        if (BpvFromPrefix(lpb, 2).fDelete() != 0) {
            FDeleteBattlePlan(i, FALSE);
            break;
        }
        if (cb < 4) return FALSE;
        if (BpvFromPrefix(lpb, 4).mdTactic() > 6 || BpvFromPrefix(lpb, 4).mdTarget1() > 8 || BpvFromPrefix(lpb, 4).mdTarget2() > 8)
            goto BombOut;
        if (i == rgcbtlplan[idPlayer]) {
            if (i >= 16)
                goto BombOut;
        }
        if (!UnpackBattlePlan(lpb, rglpbtlplan[idPlayer] + i, i, cb)) return FALSE;
        if (i == rgcbtlplan[idPlayer]) rgcbtlplan[idPlayer]++;
        break;
    case rtLogFleetPlan:
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb, 2))).FReadI16Le(id);
        if (!(lpfl = LpflFromId(id))) goto BombOut;
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb + 2, 2))).FReadI16Le(iLook);
        lpfl->iplan = iLook;
        break;
    case rtLogFleetFlagBit9:
    case rtLogFleetOrderAttrNib:
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb, 2))).FReadI16Le(id);
        if (!(lpfl = LpflFromId(id))) {
        BombOut:
            return FALSE;
        }
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb + 2, 2))).FReadI16Le(iordLog);
        if (rt == rtLogFleetFlagBit9) {
            lpfl->fRepOrders = iordLog;
            break;
        }
        if (lpfl->cord <= iordLog) goto BombOut;
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb + 4, 2))).FReadI16Le(grTaskLog);
        if (grTaskLog >= 10) goto BombOut;
        if (!FSetOrderTask(lpfl->lpplord, iordLog, grTaskLog)) return FALSE;
        break;
    case rtLogPlanetRouting:
        lppl = LpplFromId(RlvFromPrefix(lpb, 2).id());
        if (!lppl || lppl->iPlayer != idPlayer)
            goto BombOut;
        lppl->fNoResearch = RlvFromPrefix(lpb, 3).fNoResearch();
        lppl->idFling = RlvFromPrefix(lpb, 4).idFling();
        lppl->iWarpFling = RlvFromPrefix(lpb, 4).iWarpFling();
        lppl->idRoute = RlvFromPrefix(lpb, 6).idRoute();
        break;
    case rtChgPassword:
        if (gd.fGeneratingTurn) {
            NATIVEBYTEREADER(std::as_bytes(std::span(lpb, cbWirePassword))).FReadI32Le(l);
            rgplr[idPlayer].lSalt = l;
        }
        break;
    case rtLogResearch:
        ch = RsvFromPrefix(lpb, 1).pctResearch();
        if (ch < 0 || ch > 100)
            goto BombOut;
        rgplr[idPlayer].pctResearch = ch;
        if (RsvFromPrefix(lpb, 2).iTechNow() >= 6 || RsvFromPrefix(lpb, 2).iTechNext() > 7)
            goto BombOut;
        rgplr[idPlayer].iTechCur = RsvFromPrefix(lpb, 2).iTechCur();
        break;
    }
    return TRUE;
}

int16_t FLoadLogFile(char *pszLog) {
    int16_t  fRet;
    int16_t  cbLog;
    int16_t  iCur;
    MSGPLR  *lpmp;
    MSGPLR **lppmp;
    int16_t  cSkip;

    fRet = TRUE;
    imemLogCur = 0;
    imemLogPrev = -1;
    StarsRecoveryScope scopeRecovery;
    try {
        if (game.fTutorial && idPlayer == 0 && gd.fGeneratingTurn) {
            cSkip = game.turn;
            vlpMemStream = LpbLoadTutorLog();
            if (!vlpMemStream) {
                return FALSE;
            }
            if (game.turn >= *vlpMemStream) {
                vlpMemStream = NULL;
                goto StrOpen;
            }
            vlpMemStream++;
            while (cSkip-- != 0) {
                do {
                    vlpMemStream += 2 + HdvFromPrefix(vlpMemStream).cb();
                } while (HdvFromPrefix(vlpMemStream).rt() != rtBOF);
            }
        } else {
        StrOpen:
            StreamOpen(pszLog, static_cast<MdOpenFlags>(16416));
        }

        ReadRt();
        if (hdrCur.rt != rtBOF || BfvFromPrefix(rgbCur, sizeof(rgbCur)).verMajor() != 2 || BfvFromPrefix(rgbCur, sizeof(rgbCur)).verMinor() != 85) {
            FileError(idsFileDoesBelongVersionStars);
            goto FailSuccess;
        }
        if (game.lid != BfvFromPrefix(rgbCur, sizeof(rgbCur)).lidGame() || game.turn > BfvFromPrefix(rgbCur, sizeof(rgbCur)).turn()) {
        FailSuccess:
            if (vlpMemStream) {
                vlpMemStream = NULL;
            } else {
                StreamClose();
            }
            return TRUE;
        }
        if (BfvFromPrefix(rgbCur, sizeof(rgbCur)).turn() != game.turn) {
            FileError(idsLogFileRecentGameTryingLoadIgnoring);
            goto FailSuccess;
        }
        if (BfvFromPrefix(rgbCur, sizeof(rgbCur)).wGen() != game.wGen) {
            FileError(idsFileGame);
            goto FailSuccess;
        }
        wVersFile = BfvFromPrefix(rgbCur, sizeof(rgbCur)).wVersion();
        gd.fFileCrippled = BfvFromPrefix(rgbCur, sizeof(rgbCur)).fCrippled();
        if (gd.fGeneratingTurn) {
            rgplr[idPlayer].fCrippled = BfvFromPrefix(rgbCur, sizeof(rgbCur)).fCrippled();
        }
        ReadRt();
        NATIVEBYTEREADER(std::as_bytes(std::span(rgbCur)).first(2)).FReadI16Le(cbLog);
        for (iCur = 0; iCur < cbLog; iCur += hdrCur.cb + 2) {
            ReadRt();
            FEncodeHdr(hdrCur, std::as_writable_bytes(std::span(lpLog + iCur, cbWireHdr)));
            memmove(lpLog + (2 + iCur), rgbCur, hdrCur.cb);
        }
        ReadRt();
        for (lppmp = &vlpmsgplrOut; *lppmp; lppmp = &(*lppmp)->lpmsgplrNext) {
        }
        while (hdrCur.rt == rtPlrMsg) {
            *lppmp = LpmsgplrFromRt();
            if (*lppmp) {
                lpmp = *lppmp;
                lppmp = &lpmp->lpmsgplrNext;
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
        DirtyGame(FALSE);
        return fRet;
    } catch (const StarsError &) {
        if (vlpMemStream) {
            return FALSE;
        }
        if (hf == -1) {
            return TRUE;
        }
        StreamClose();
        return FALSE;
    }
}

int16_t FCheckLogFile(int16_t iplr, int16_t *pfError) {
    int16_t  fRet;
    int16_t  cbLog;
    int16_t  iCur;

    fRet = TRUE;
    imemLogCur = 0;
    imemLogPrev = -1;
    StarsRecoveryScope scopeRecovery;
    try {
        idsFileError = 0;
        if (!FOpenFile(dtLog, iplr, 32)) {
            if (idsFileError != idsCantOpenFile) {
                *pfError = idsFileError;
            }
            return FALSE;
        }
        ReadRt();
        NATIVEBYTEREADER(std::as_bytes(std::span(rgbCur)).first(2)).FReadI16Le(cbLog);
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
        return fRet;
    } catch (const StarsError &) {
        if (hf == -1) {
            return TRUE;
        }
        StreamClose();
        *pfError = 3;
        return FALSE;
    }
}

int16_t FWriteLogFile(char *pszFileBase, int16_t iPlayer) {
    int16_t  iCur;
    uint8_t *lprts;
    RTLOGHDR rtlh;
    uint8_t rgbLogHdr[cbWireLogHdr];
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
    StarsRecoveryScope scopeRecovery;
    try {
        rtlh.cbLog = imemLogCur;
        /* Preserve the unused 15-byte metadata area in the orders-file header. */
        memset(rtlh.rgbUnused, 0, sizeof(rtlh.rgbUnused));
        FEncodeLogHdr(rtlh, std::as_writable_bytes(std::span(rgbLogHdr)));
        WriteRt(static_cast<RecordType>(9), cbWireLogHdr, rgbLogHdr);
        for (; iCur < imemLogCur; iCur += HdvFromPrefix(lprts).cb() + 2) {
            lprts = lpLog + iCur;
            WriteRt(static_cast<RecordType>(HdvFromPrefix(lprts).rt()), HdvFromPrefix(lprts).cb(), lpLog + (2 + iCur));
        }
        iCur = vcmsgplrOut;
        lpmp = vlpmsgplrOut;
        while (iCur-- != 0) {
            WriteRtPlrMsg(lpmp);
            lpmp = lpmp->lpmsgplrNext;
        }
        WriteRt(rtEOF, 0, NULL);
        StreamClose();
        DirtyGame(FALSE);
        gd.fWriteTurnNum = TRUE;
        return TRUE;
    } catch (const StarsError &) {
        StreamClose();
        return FALSE;
    }
}

int16_t FWriteHistFile(int16_t iPlayer) {
    PLANET   *lppl;
    int16_t   i;
    uint16_t  cTurnBase;
    SHDEF    *lpshdef;
    int16_t   j;
    RTHISTHDR rthh;
    std::byte rgbHistHdr[cbWireHistHdr];
    uint8_t  *lpb;

    if (!FCreateFile(dtHist, iPlayer, NULL)) {
        AlertSz(PszFormatIds(idsUnableCreateHistoryFile, NULL), MB_ICONHAND);
        return FALSE;
    }
    StarsRecoveryScope scopeRecovery;
    try {
        rthh.cPlanet = cPlanet;
        rthh.cPlanetExtra = rgplr[iPlayer].cFleet;
        if (!FEncodeRtHistHdr(rthh, rgbHistHdr)) { StreamClose(); return FALSE; }
        WriteRt(rtHistHdr, cbWireHistHdr, rgbHistHdr);
        i = 0;
        lppl = lpPlanets;
        while (i < cPlanet) {
            WritePlanet(lppl, static_cast<RecordType>(lppl->det < detSome ? 15 : rtPlanetB), TRUE);
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
                        WriteRtScore(rgsxPlr[i] + j);
                    }
                }
            }
        }
        if (vlpbAiData && AhvFromLp(vlpbAiData).cbAiHist() > 2) {
            i = AhvFromLp(vlpbAiData).cbAiHist();
            lpb = vlpbAiData;
            for (; i >= 1024; i -= 1023) {
                WriteRt(rtAiData, 1023, lpb);
                lpb += 1023;
            }
            WriteRt(rtAiData, i, lpb);
        }
        WriteRt(rtEOF, 0, NULL);
        StreamClose();
        return TRUE;
    } catch (const StarsError &) {
        StreamClose();
        return FALSE;
    }
}

void EnumLogRts(int16_t (*pfn)(void *, int16_t, int16_t, void *, int16_t), void *lpPass, int16_t iPass) {
    int16_t fLogOld;
    int16_t fRet;
    int16_t iCur;
    uint8_t *lprts;

    iCur = 0;
    fRet = TRUE;
    fLogOld = fLogOff;
    if (imemLogCur != 0) {
        for (; iCur < imemLogCur; iCur += HdvFromPrefix(lprts).cb() + 2) {
            lprts = lpLog + iCur;
            if (pfn(lpLog + (2 + iCur), HdvFromPrefix(lprts).rt(), HdvFromPrefix(lprts).cb(), lpPass, iPass) == 0)
                break;
        }
    }
    return;
}
