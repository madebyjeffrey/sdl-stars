#include "common.h"
#include <algorithm>
#include "appstate.h"
#include "fleet_query_io.h"
#include "text_io.h"
#include "presentation_io.h"
#include "turn_io.h"
#include "heap_io.h"
#include "planet_query_io.h"
#include "alert_io.h"
#include "record_io.h"

void WriteMemResearch(APPSESSION &app, std::int16_t wResearch) {
    std::uint8_t rgb[cbWireResearch];

    NATIVEBYTEWRITER(std::as_writable_bytes(std::span(rgb))).FWriteI16Le(wResearch);
    WriteMemRt(app, rtLogResearch, cbWireResearch, rgb);
}

void WriteMemPassword(APPSESSION &app, std::int32_t lSalt) {
    std::uint8_t rgb[cbWirePassword];

    NATIVEBYTEWRITER(std::as_writable_bytes(std::span(rgb))).FWriteI32Le(lSalt);
    WriteMemRt(app, rtChgPassword, cbWirePassword, rgb);
}

void LogSplitFleet(APPSESSION &app, std::int16_t id) {
    std::byte rgb[cbWireFleetId];
    if (app.state.gd.fGeneratingTurn) {
        LpflFromId(app, id)->SetFCompChg(1);
    } else {
        NATIVEBYTEWRITER(rgb).FWriteI16Le(id);
        WriteMemRt(app, rtLogFleetSplit, cbWireFleetId, rgb);
    }
    return;
}

void LogMergeFleet(APPSESSION &app, std::int16_t id) {
    std::uint16_t idCur;
    std::int16_t  i;
    std::uint16_t rgid[512];
    std::byte rgb[512 * cbWireFleetId];
    std::int16_t  j;

    if (!app.state.gd.fGeneratingTurn) {
        i = 0;
        while (i < app.state.vcflMerge) {
            rgid[0] = id;
            j = 1;
            for (; j < 511 && i < app.state.vcflMerge; i++) {
                if (app.state.vrgiflMerge[i] != iflNone) {
                    idCur = app.state.vrgiflMerge[i];
                    if (idCur != id) {
                        rgid[j++] = idCur;
                    }
                }
            }
            FEncodeFleetIds(std::span(rgid, j), rgb);
            WriteMemRt(app, rtLogFleetMerge, j * cbWireFleetId, rgb);
        }
    }
    return;
}

void LogChangeShDef(APPSESSION &app, SHDEF *lpshdefNew) {
    std::uint8_t  rgb[149];
    std::uint8_t *pb;
    RTCHGSHDEF rtchg{};

    if (!app.state.gd.fGeneratingTurn) {
        rtchg.ishdef = lpshdefNew->ishdef;
        rtchg.iPlr = app.state.idPlayer;
        if (lpshdefNew->fFree) {
            rtchg.mdChg = 0;
            FEncodeRtChgShDefPrefix(rtchg, std::as_writable_bytes(std::span(rgb)));
            WriteMemRt(app, rtLogShDef, cbWireChgShDef, rgb);
        } else {
            rtchg.mdChg = 1;
            lpshdefNew->det = detAll;
            FEncodeRtChgShDefPrefix(rtchg, std::as_writable_bytes(std::span(rgb)));
            pb = rgb + cbWireChgShDef;
            WriteRtShDef(app, lpshdefNew, &pb);
            WriteMemRt(app, rtLogShDef, pb - rgb, rgb);
        }
        if (app.state.gd.fTutorial && app.state.idPlayer == 0) {
            app.state.tutor.fChange = 1;
            PresentationIO().pfnTutor(app);
        }
    }
    return;
}

void LogChangeName(APPSESSION &app, GrobjClass grobj, std::int16_t id, char *szName) {
    FLEET    *lpfl;
    std::int16_t   cOut;
    RTCHGNAME rtchgname;
    std::uint8_t rgbName[cbWireChangeName + 33];

    lpfl = LpflFromId(app, id);
    if (lpfl) {
        if (lpfl->lpszName) {
            HeapIO().pfnFree(app, lpfl->lpszName, htString);
        }
        if (!szName || *szName == 0) {
            rtchgname.rgb[0] = 0;
            rtchgname.rgb[1] = 0;
            cOut = 1;
            lpfl->lpszName = nullptr;
        } else {
            cOut = std::strlen(szName);
            lpfl->lpszName = LpAlloc<char>(app, std::strlen(szName) + 1, htString);
            std::strcpy(lpfl->lpszName, szName);
            if (FCompressUserString(app, szName, reinterpret_cast<char *>(&rtchgname.rgb[1]), &cOut)) {
                rtchgname.rgb[0] = cOut;
            } else {
                rtchgname.rgb[0] = 0;
                std::strcpy(reinterpret_cast<char *>(&rtchgname.rgb[1]), szName);
                cOut++;
            }
        }
        rtchgname.grobj = grobj;
        rtchgname.id = id;
        FEncodeChangeNamePrefix(rtchgname, std::as_writable_bytes(std::span(rgbName)).first(cbWireChangeName));
        std::memcpy(rgbName + cbWireChangeName, rtchgname.rgb, cOut + 1);
        WriteMemRt(app, rtLogFleetName, cOut + 5, rgbName);
        if (app.state.gd.fTutorial) {
            PresentationIO().pfnTutor(app);
        }
    }
    return;
}

void LogChangeFleet(APPSESSION &app, FLEET *pfl, FLEET *pflNew) {
    std::int16_t   d;
    std::int16_t   i;
    std::int16_t   fChg;
    LOGXFERF  lxfNew;
    LOGXFER   lxNew;
    RTWAYPT   rtwp;
    RTSHIPINT rtsi;
    std::int16_t   iordNew;
    std::int16_t   iordOld;
    std::int16_t   cbWp;
    char     *pbWp;
    HDR       hdr;
    RTWAYPT rtwpPrev{};
    std::byte rgbWp[cbWireWaypt + cbWireOrder];
    std::byte rgbShipInt[cbWireShipInt];

    fChg = 0;
    if (app.state.gd.fGeneratingTurn)
        return;
    lxfNew.id = pfl->id;
    lxfNew.grobj = grobjFleet;
    for (i = 0; i < 16; i++) {
        lxfNew.rgdItem[i] = pflNew->rgcsh[i] - pfl->rgcsh[i];
        if (pflNew->rgcsh[i] - pfl->rgcsh[i] != 0) {
            fChg = 1;
        }
    }
    if (fChg) {
        if (app.state.fValidLxf) {
            LogMakeValidXferf(app, &app.state.lxf, &lxfNew);
            app.state.fValidLxf = 0;
        } else {
            app.state.lxf = lxfNew;
            app.state.fValidLxf = 1;
        }
        return;
    }
    lxNew.id = pfl->id;
    lxNew.grobj = grobjFleet;
    for (i = 0; i < 5; i++) {
        lxNew.rgdItem[i] = pflNew->rgwtMin[i] - pfl->rgwtMin[i];
        if (pflNew->rgwtMin[i] - pfl->rgwtMin[i] != 0) {
            fChg = 1;
        }
    }
    if (fChg) {
        if (app.state.fValidLx) {
            LogMakeValidXfer(app, &app.state.lx, &lxNew);
            app.state.fValidLx = 0;
        } else {
            app.state.lx = lxNew;
            app.state.fValidLx = 1;
        }
        return;
    }
    if (pfl->iplan != pflNew->iplan) {
        rtsi.id = pflNew->id;
        rtsi.i = pflNew->iplan;
        FEncodeRtShipInt(rtsi, rgbShipInt);
        WriteMemRt(app, rtLogFleetPlan, cbWireShipInt, rgbShipInt);
    }
    if (pfl->fRepOrders != pflNew->fRepOrders) {
        rtsi.id = pflNew->id;
        rtsi.i = pflNew->fRepOrders;
        FEncodeRtShipInt(rtsi, rgbShipInt);
        WriteMemRt(app, rtLogFleetFlagBit9, cbWireShipInt, rgbShipInt);
    }
    d = pflNew->cord - pfl->cord;
    for (iordOld = 0;
         iordOld < pfl->cord && iordOld < pflNew->cord && std::memcmp(&pfl->lpplord->rgord()[iordOld], &pflNew->lpplord->rgord()[iordOld], sizeof(ORDER)) == 0;
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
        WriteMemRt(app, rtLogFleetOrderDelete, cbWireShipInt, rgbShipInt);
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
        WriteMemRt(app, rtLogFleetOrderInsert, cbWp, rgbWp);
        goto NextTest;
    } else {
        cbWp = 22;
        if (FGetPrevLogRt(app, &hdr, reinterpret_cast<std::uint8_t *>(app.state.rgbCur)) && hdr.rt == rtLogFleetOrderUpdate &&
            FDecodeRtWayptPrefix(std::as_bytes(std::span(app.state.rgbCur)), rtwpPrev) && rtwpPrev.id == pflNew->id && rtwpPrev.iWaypt == iordNew) {
            app.state.imemLogCur = app.state.imemLogPrev;
        }
        rtwp.id = pflNew->id;
        rtwp.iWaypt = iordNew;
        rtwp.order = pflNew->lpplord->rgord()[iordNew];
        FEncodeRtWaypt(rtwp, rgbWp);
        pbWp = reinterpret_cast<char *>(rgbWp);
        while (cbWp-- > 0 && pbWp[cbWp] == 0) {
        }
        cbWp++;
        WriteMemRt(app, rtLogFleetOrderUpdate, cbWp, rgbWp);
    }
NextTest:
    return;
}

void LogChangeRelations(APPSESSION &app) {
    HDR hdr;

    if (FGetPrevLogRt(app, &hdr, reinterpret_cast<std::uint8_t *>(app.state.rgbCur)) && hdr.rt == rtLogRelations) {
        app.state.imemLogCur = app.state.imemLogPrev;
    }
    WriteMemRt(app, rtLogRelations, app.state.game.cPlayer, app.state.rgplr[app.state.idPlayer].rgmdRelation);
    if (app.state.gd.fTutorial && app.state.idPlayer == 0) {
        app.state.tutor.fChange = 1;
        PresentationIO().pfnTutor(app);
    }
    return;
}

void LogChangeBtlplan(APPSESSION &app, BTLPLAN *pbtlplan) {
    WriteBattlePlan(app, pbtlplan, 1);
    if (app.state.gd.fTutorial && app.state.idPlayer == 0) {
        app.state.tutor.fChange = 1;
        PresentationIO().pfnTutor(app);
    }
    return;
}

void LogChangePlanet(APPSESSION &app, PLANET *ppl, PLANET *pplNew) {
    std::int16_t i;
    std::int16_t fChg;
    HDR     hdr;
    LOGXFER lxNew;
    RTCHGPRODQ rtcpq{};
    std::byte rgbProdId[cbWireChgProdQ];
    RTCHGPLANETLONG routing{};

    fChg = 0;
    if (!app.state.gd.fGeneratingTurn) {
        if (!ppl) {
            if (!app.state.fValidLx) {
                return;
            }
            lxNew.id = -1;
            lxNew.grobj = grobjOther;
            for (i = 0; i < 5; i++) {
                lxNew.rgdItem[i] = -app.state.lx.rgdItem[i];
            }
            goto ChgIt;
        }

        lxNew.id = ppl->id;
        lxNew.grobj = grobjPlanet;
        for (i = 0; i < 4; i++) {
            lxNew.rgdItem[i] = pplNew->rgwtMin[i] - ppl->rgwtMin[i];
            if (pplNew->rgwtMin[i] - ppl->rgwtMin[i] != 0) {
                fChg = 1;
            }
        }
        lxNew.rgdItem[4] = 0;
        if (fChg) {
        ChgIt:
            if (app.state.fValidLx) {
                LogMakeValidXfer(app, &app.state.lx, &lxNew);
                app.state.fValidLx = 0;
            } else {
                app.state.lx = lxNew;
                app.state.fValidLx = 1;
            }
        }
        if (ppl) {
            if (!pplNew->lpplprod && ppl->lpplprod) {
                NATIVEBYTEWRITER(rgbProdId).FWriteI16Le(lxNew.id);
                WriteMemRt(app, rtLogPlanetProdQ, cbWireChgProdQ, rgbProdId);
            } else if (pplNew->lpplprod && (!ppl->lpplprod || ppl->lpplprod->iprodMac != pplNew->lpplprod->iprodMac ||
                                            std::memcmp(ppl->lpplprod->rgprod(), pplNew->lpplprod->rgprod(), ppl->lpplprod->iprodMac * 4) != 0)) {
                if (FGetPrevLogRt(app, &hdr, reinterpret_cast<std::uint8_t *>(app.state.rgbCur)) && hdr.rt == rtLogPlanetProdQ &&
                    FDecodeRtChgProdQ(std::as_bytes(std::span(app.state.rgbCur)), rtcpq) && rtcpq.id == ppl->id) {
                    app.state.imemLogCur = app.state.imemLogPrev;
                }
                NATIVEBYTEWRITER(std::as_writable_bytes(std::span(app.state.rgbCur))).FWriteI16Le(ppl->id);
                FEncodeProdQueue(std::span(pplNew->lpplprod->rgprod(), pplNew->lpplprod->iprodMac),
                                 std::as_writable_bytes(std::span(app.state.rgbCur)).subspan(cbWireChgProdQ));
                WriteMemRt(app, rtLogPlanetProdQ, pplNew->lpplprod->iprodMac * 4 + 2, app.state.rgbCur);
            }
            if (static_cast<std::uint32_t>(ppl->fNoResearch) != pplNew->fNoResearch || ppl->idFling != pplNew->idFling || ppl->iWarpFling != pplNew->iWarpFling ||
                ppl->idRoute != pplNew->idRoute) {
                routing.id = pplNew->id;
                routing.fNoResearch = pplNew->fNoResearch;
                routing.idFling = pplNew->idFling;
                routing.iWarpFling = pplNew->iWarpFling;
                routing.idRoute = pplNew->idRoute;
                FEncodePlanetRouting(routing, std::as_writable_bytes(std::span(app.state.rgbCur)));
                WriteMemRt(app, rtLogPlanetRouting, cbWirePlanetRouting, app.state.rgbCur);
            }
        }
    }
    return;
}

void LogChangeThing(APPSESSION &app, THING *lpth, THING *pthNew) {
    std::int16_t i;
    std::int16_t fChg;
    LOGXFER lxNew;

    fChg = 0;
    if (!app.state.gd.fGeneratingTurn) {
        lxNew = {};
        lxNew.id = pthNew->idFull();
        lxNew.grobj = grobjThing;
        for (i = 0; i < 3; i++) {
            lxNew.rgdItem[i] = static_cast<std::int16_t>((pthNew->thp().rgwtMin(i) - lpth->thp().rgwtMin(i)));
            if (static_cast<std::int16_t>((pthNew->thp().rgwtMin(i) - lpth->thp().rgwtMin(i))) != 0) {
                fChg = 1;
            }
        }
        if (fChg) {
            if (app.state.fValidLx) {
                LogMakeValidXfer(app, &app.state.lx, &lxNew);
                app.state.fValidLx = 0;
            } else {
                app.state.lx = lxNew;
                app.state.fValidLx = 1;
            }
        }
    }
    return;
}

void LogMakeValidXfer(APPSESSION &app, LOGXFER *plx1, LOGXFER *plx2) {
    std::int32_t  rgQuan[5];
    std::uint8_t  *prt;
    std::int16_t  iOff;
    std::uint8_t *prtl;
    std::int16_t  rt;
    std::int16_t  i;
    std::uint8_t  rgbuf[28];
    RTXFER transfer{};
    std::int16_t  grbit;
    std::uint8_t *prtx;
    std::int16_t  grFlag;
    std::int32_t  iBiggest;
    std::int16_t  cb;

    iBiggest = 0;
    grbit = 0;
    grFlag = 1;
    rgQuan[0] = 0;
    rgQuan[1] = 0;
    rgQuan[2] = 0;
    rgQuan[3] = 0;
    rgQuan[4] = 0;
    switch (app.state.hdrPrev.rt) {
    case rtLogCargoXfer8:
    case rtLogCargoXfer16:
    case rtLogCargoXfer32:
        prt = static_cast<std::uint8_t *>((app.state.lpLog + (app.state.imemLogCur + -app.state.hdrPrev.cb)));
        break;
    default:
        prt = nullptr;
    }
    if (prt && CxvFromPrefix(prt, 5).grobj1() == (plx1->grobj & 0xff) && CxvFromPrefix(prt, 5).grobj2() == (plx2->grobj & 0xff) && CxvFromPrefix(prt, 2).id1() == plx1->id && CxvFromPrefix(prt, 4).id2() == plx2->id) {
        grbit = CxvFromPrefix(prt, 6).grbitItems();
        iOff = 0;
        switch (app.state.hdrPrev.rt) {
        case rtLogCargoXfer8:
            for (i = 0; i < 5; i++) {
                if (1 << i & grbit) {
                    rgQuan[i] = static_cast<std::int16_t>(CxvFromPrefix(prt, 6 + (iOff + 1) * 1).cQuan8(iOff));
                    iOff++;
                }
            }
            break;
        case rtLogCargoXfer16:
            prtx = static_cast<std::uint8_t *>(prt);
            for (i = 0; i < 5; i++) {
                if (1 << i & grbit) {
                    rgQuan[i] = CxvFromPrefix(prtx, 6 + (iOff + 1) * 2).cQuan16(iOff);
                    iOff++;
                }
            }
            break;
        case rtLogCargoXfer32:
            prtl = static_cast<std::uint8_t *>(prt);
            for (i = 0; i < 5; i++) {
                if (1 << i & grbit) {
                    rgQuan[i] = CxvFromPrefix(prtl, 6 + (iOff + 1) * 4).cQuan32(iOff);
                    iOff++;
                }
            }
        }
        CancelMemRt(app, static_cast<RecordType>(app.state.hdrPrev.rt));
    }
    i = 0;
    grbit = 0;
    while (i < 5) {
        rgQuan[i] += plx1->rgdItem[i];
        if (iBiggest <= std::labs(rgQuan[i])) {
            iBiggest = std::labs(rgQuan[i]);
        }
        if (rgQuan[i] != 0) {
            grbit |= grFlag;
        }
        i++;
        grFlag *= 2;
    }
    if (grbit != 0) {
        prt = reinterpret_cast<std::uint8_t *>(rgbuf);
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
            prtx = reinterpret_cast<std::uint8_t *>(rgbuf);
            for (i = 0; i < 5; i++) {
                if (rgQuan[i] != 0) {
                    CxvFromPrefix(prtx, 6 + (iOff + 1) * 2).SetCQuan16(iOff++, LOWORD(rgQuan[i]));
                    cb += 2;
                }
            }
        } else {
            rt = 25;
            prtl = reinterpret_cast<std::uint8_t *>(rgbuf);
            for (i = 0; i < 5; i++) {
                if (rgQuan[i] != 0) {
                    CxvFromPrefix(prtl, 6 + (iOff + 1) * 4).SetCQuan32(iOff++, rgQuan[i]);
                    cb += 4;
                }
            }
        }
        WriteMemRt(app, static_cast<RecordType>(rt), cb, rgbuf);
    }
    return;
}

void LogMakeValidXferf(APPSESSION &app, LOGXFERF *plxf1, LOGXFERF *plxf2) {
    RTXFERF rtxf{};
    std::int16_t  iOff;
    std::int16_t  i;
    char     rgbuf[41];
    std::uint16_t grbit;
    std::int16_t  grFlag;
    std::int16_t  cb;

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
        WriteMemRt(app, rtLogFleetCargoXfer, cb, rgbuf);
    }
    return;
}

void CancelMemRt(APPSESSION &app, RecordType rt) {
    app.state.imemLogCur -= app.state.hdrPrev.cb + 2;
    app.state.hdrPrev.rt = rtEOF;
    return;
}

static void WriteMemRtCore(APPSESSION &app, RecordType rt, std::int16_t cb, void *rg, bool fZipProd) {
    HDR      hdr;
    std::uint8_t *lpv;

    if (!app.state.fLogOff) {
        if (app.state.imemLogCur + cb + 2 > 32000) {
            AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsLogFileHasReachedMaximumAllowableSize, nullptr), win32::MB_ICONHAND);
        }
        RecordIO().pfnDirtyGame(app, 1);
        app.state.imemLogPrev = app.state.imemLogCur;
        hdr.cb = cb;
        hdr.rt = rt;
        lpv = app.state.lpLog;
        lpv += app.state.imemLogCur;
        FEncodeHdr(hdr, std::as_writable_bytes(std::span(lpv, cbWireHdr)));
        if (cb > 0) {
            if (fZipProd) {
                // Read live template state after the existing advisory/title
                // callbacks, at the old global-source copy point.
                std::byte rgb[5 * cbWireZipProdQ];
                for (int i = 0; i < 5; i++) FEncodeZipProdQ(app.state.vrgZipProd[i], std::span(rgb).subspan(i * cbWireZipProdQ));
                assert(static_cast<std::size_t>(cb) <= sizeof(rgb) - 14);
                std::memcpy(lpv + 2, rgb + 14, cb);
            } else {
                std::memcpy(lpv + 2, rg, cb);
            }
        }
        app.state.imemLogCur += cb + 2;
        if (rt != rtEOF) {
            app.state.hdrPrev = hdr;
        }
    }
    return;
}

void WriteMemRt(APPSESSION &app, RecordType rt, std::int16_t cb, void *rg) {
    WriteMemRtCore(app, rt, cb, rg, false);
}

void WriteMemZpqRt(APPSESSION &app, std::int16_t cb) {
    WriteMemRtCore(app, rtLogPlayerZpq1, cb, nullptr, true);
}

void DirtyGame(APPSESSION &app, std::int16_t fDirty) {
    if (fDirty != app.state.game.fDirty) {
        app.state.game.fDirty = fDirty;
        if (!app.state.fAi) {
            UpdateMsgTitle(app);
        }
    }
    return;
}

std::int16_t FGetPrevLogRt(APPSESSION &app, HDR *phdr, std::uint8_t *pb) {
    std::uint8_t *lpv;

    if (app.state.imemLogPrev == -1) {
        return 0;
    }
    lpv = app.state.lpLog + app.state.imemLogPrev;
    FDecodeHdr(std::as_bytes(std::span(lpv, cbWireHdr)), *phdr);
    if (phdr->cb > 0) {
        std::memcpy(pb, lpv + 2, phdr->cb);
    }
    return 1;
}

std::int16_t FRunLogFile(APPSESSION &app) {
    std::int16_t fLogOld;
    std::int16_t fRet;
    std::int16_t iCur;
    std::uint8_t *lprts;

    iCur = 0;
    fRet = 1;
    fLogOld = app.state.fLogOff;
    if (app.state.imemLogCur == 0) {
        return 1;
    }
    app.state.fLogOff = 1;
    for (; iCur < app.state.imemLogCur; iCur += HdvFromPrefix(lprts).cb() + 2) {
        lprts = app.state.lpLog + iCur;
        fRet &= FRunLogRecord(app, static_cast<RecordType>(HdvFromPrefix(lprts).rt()), HdvFromPrefix(lprts).cb(), app.state.lpLog + (2 + iCur));
    }
    app.state.fLogOff = fLogOld;
    app.state.gd.fFleetLinkValid = 0;
    return fRet;
}

std::int16_t FRunLogRecord(APPSESSION &app, RecordType rt, std::int16_t cb, std::uint8_t *lpb) {
    std::int16_t   fExtra;
    std::int32_t   cXfer;
    XFERFULL *lpxfCur;
    PLANET   *lppl;
    std::int32_t   rgcXfer[5];
    XFER      rgxf[2];
    FLEET    *lpfl;
    std::int16_t   ifl;
    std::int16_t   i;
    std::uint16_t  grbit;
    std::int16_t   rgifl[512];
    SHDEF    *lpshdef;
    std::int16_t   iPass;
    std::int16_t   iLook;
    PLANET   *lpplMac;
    std::int8_t    ch;
    std::int32_t   l;
    char      szT[33];
    THING    *lpth;
    std::int16_t   id;
    std::int16_t   iColDrop;
    COLDROP  *lpcdT;
    XFERFULL *lpxfMax;
    MessageId idm;
    RTCHGPRODQ rtcpq{};
    PROD prod{};
    std::int16_t iordLog, grTaskLog;
    std::uint16_t wOrderLog;
    RTCHGSHDEF rtchgshdef{};
    std::int16_t idFleet, cShipXfer;

    lpxfCur = nullptr;
    switch (rt) {
    default:
        break;
    case rtEOF:
        break;
    case rtLogPlanetProdQ:
        lppl = app.state.lpPlanets;
        lpplMac = app.state.lpPlanets + app.state.cPlanet;
        // Existing log replay consumes the ID prefix even for a short logical
        // record. Keep that admission policy while reading its bytes explicitly.
        if (lppl < lpplMac) FDecodeRtChgProdQ(std::as_bytes(std::span(lpb, cbWireChgProdQ)), rtcpq);
        for (; lppl < lpplMac && lppl->id != rtcpq.id; lppl++) {
        }
        if (lppl != lpplMac && lppl->iPlayer == app.state.idPlayer) {
            i = static_cast<std::uint32_t>((cb - 2)) / 4;
            if (i > 0) {
                if (!lppl->lpplprod) {
                    lppl->lpplprod = LpplAlloc<PLPROD>(app, 4, i + 2, htOrd);
                } else if (lppl->lpplprod->iprodMax < i) {
                    lppl->lpplprod = LpplReAlloc(app, lppl->lpplprod, i + 2);
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
                return 1;
            } else {
                HeapIO().pfnFree(app, lppl->lpplprod, htOrd);
                lppl->lpplprod = nullptr;
            }
            break;
        }
        return 0;
    case rtLogRelations:
        std::memcpy(app.state.rgplr[app.state.idPlayer].rgmdRelation, lpb, app.state.game.cPlayer);
        break;
    case rtLogPlayerZpq1:
        if (app.state.gd.fGeneratingTurn) {
            if (static_cast<std::uint16_t>(cb) > 26) {
                return 0;
            }
            std::byte rgb[cbWireZipProdQ1];
            FEncodeZipProdQ1(app.state.rgplr[app.state.idPlayer].zpq1, rgb);
            if (cb != 0) std::memcpy(rgb, lpb, cb);
            FDecodeZipProdQ1(rgb, app.state.rgplr[app.state.idPlayer].zpq1);
        }
        break;
    case rtLogFleetName:
        if (cb < static_cast<std::int16_t>(cbWireChangeName) ||
            !FReadUserString(app, lpb + cbWireChangeName, cb - cbWireChangeName, szT, 32, nullptr)) return 0;
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb, 2))).FReadI16Le(id);
        if (!(lpfl = LpflFromId(app, id))) return 0;
        if (lpfl->lpszName) HeapIO().pfnFree(app, lpfl->lpszName, htString);
        lpfl->lpszName = nullptr;
        if (*szT) {
            lpfl->lpszName = LpAlloc<char>(app, std::strlen(szT) + 1, htString);
            std::strcpy(lpfl->lpszName, szT);
        }
        break;
    case rtThing:
        lpth = LpthFromId(app, LtvFromPrefix(lpb, 2).idFull());
        if (!lpth || lpth->ith() != ithMinefield) {
            return 0;
        }
        { const auto fDetonateForThing = (LtvFromPrefix(lpb, 4).fDetonate()); auto &thForThing = *(lpth); thForThing.thm().SetFDetonate(fDetonateForThing); }
        break;
    case rtLogShDef:
        if (cb < static_cast<std::int16_t>(cbWireChgShDef)) return 0;
        FDecodeRtChgShDefPrefix(std::as_bytes(std::span(lpb, cbWireChgShDef)), rtchgshdef);
        i = rtchgshdef.ishdef;
        iLook = rtchgshdef.iPlr;
        if (iLook >= app.state.game.cPlayer || iLook != app.state.idPlayer) {
            return 0;
        }
        if (i >= 16) {
            if (i >= 26) {
                return 0;
            }
            lpshdef = app.state.rglpshdefSB[iLook] + (i - 16);
        } else {
            lpshdef = app.state.rglpshdef[iLook] + i;
        }
        if (!lpshdef->fFree && lpshdef->cExist != 0 && rtchgshdef.mdChg != 0) {
            return 0;
        }
        /* Work done on queued ships or starbases of the old design doesn't
           carry over to the new one. The original kept PROD.pct and costed
           the rest from the new design, so a mostly built cheap starbase
           could be finished as an expensive one. */
        lppl = app.state.lpPlanets;
        lpplMac = app.state.lpPlanets + app.state.cPlanet;
        for (; lppl < lpplMac; lppl++) {
            if (lppl->iPlayer == app.state.idPlayer && lppl->lpplprod) {
                for (iPass = 0; iPass < lppl->lpplprod->iprodMac; iPass++) {
                    if (lppl->lpplprod->rgprod()[iPass].grobj == grobjFleet && lppl->lpplprod->rgprod()[iPass].iItem == static_cast<std::uint32_t>(i)) {
                        lppl->lpplprod->rgprod()[iPass].pct = 0;
                    }
                }
            }
        }
        switch (rtchgshdef.mdChg) {
        case 0:
            if (lpshdef->fFree)
                break;
            DestroyAllIshdef(app, i, app.state.idPlayer);
            lpshdef->fFree = 1;
            if (i >= 16) {
                app.state.rgplr[iLook].cshdefSB += 15;
            } else {
                app.state.rgplr[iLook].cShDef--;
            }
            break;
        case 1:
            if (lpshdef->fFree) {
                if (i >= 16) {
                    app.state.rgplr[iLook].cshdefSB++;
                } else {
                    app.state.rgplr[iLook].cShDef++;
                }
            }
            if (i >= 16) {
                if (FReadShDef(app, lpb + cbWireChgShDef, app.state.rglpshdefSB[iLook], app.state.idPlayer, cb - cbWireChgShDef))
                    break;
                app.state.rgplr[iLook].cshdefSB += 15;
                return 0;
            }
            if (FReadShDef(app, lpb + cbWireChgShDef, app.state.rglpshdef[iLook], app.state.idPlayer, cb - cbWireChgShDef))
                break;
            app.state.rgplr[iLook].cShDef--;
            return 0;
        }
        break;
    case rtLogCargoXfer8:
    case rtLogCargoXfer16:
    case rtLogCargoXfer32:
        if (!FLookupObject(app, static_cast<GrobjClass>(CxvFromPrefix(lpb, 5).grobj1()), CxvFromPrefix(lpb, 2).id1(), rgxf[0].LpvLookup(static_cast<GrobjClass>(CxvFromPrefix(lpb, 5).grobj1())))) {
            return 0;
        }
        rgxf[1].SetIdObj(idflNone);
        if (CxvFromPrefix(lpb, 5).grobj2() != grobjOther && !FLookupObject(app, static_cast<GrobjClass>(CxvFromPrefix(lpb, 5).grobj2()), CxvFromPrefix(lpb, 4).id2(), rgxf[1].LpvLookup(static_cast<GrobjClass>(CxvFromPrefix(lpb, 5).grobj2())))) {
            if (CxvFromPrefix(lpb, 5).grobj2() == grobjFleet && (CxvFromPrefix(lpb, 4).id2() >> 9 & 0xf) != app.state.idPlayer)
                break;
            return 0;
        }
        grbit = CxvFromPrefix(lpb, 6).grbitItems();
        for (i = 0, iLook = 0; i < 5; i++, grbit >>= 1) {
            if (grbit & 1) {
                if (rt == rtLogCargoXfer8) {
                    rgcXfer[i] = static_cast<std::int16_t>(CxvFromPrefix(lpb, 6 + (iLook + 1) * 1).cQuan8(iLook));
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
                        l = ChgCargo(app, static_cast<GrobjClass>(CxvFromPrefix(lpb, 5).grobj1()), CxvFromPrefix(lpb, 2).id1(), static_cast<MineralType>(i), cXfer, rgxf[0].LpvCargo(static_cast<GrobjClass>(CxvFromPrefix(lpb, 5).grobj1())));
                        if (l != cXfer) {
                            id = CxvFromPrefix(lpb, 5).grobj1() == grobjFleet ? -32768 : 0;
                            id |= rgxf[0].idObj();
                            TurnIO().pfnMessage(app, rgxf[0].iPlayerObj(), idmUnableTransferKtKtRequest, static_cast<MsgGoto>(id), id, LOWORD(cXfer) - LOWORD(l), i, LOWORD(cXfer), 0, 0, 0);
                            rgcXfer[i] = l;
                        }
                    }
                    if (((iPass == 0 && cXfer >= 0) || (iPass == 1 && cXfer < 0)) && CxvFromPrefix(lpb, 5).grobj2() != grobjOther) {
                        if (i == 3 && cXfer != 0 && app.state.gd.fGeneratingTurn && CxvFromPrefix(lpb, 5).grobj2() == grobjPlanet && CxvFromPrefix(lpb, 5).grobj1() == grobjFleet &&
                            rgxf[0].iPlayerObj() != rgxf[1].iPlayerObj()) {
                            lpcdT = app.state.lpcd;
                            iColDrop = 0;
                            if (cXfer > 0) {
                                for (; iColDrop < app.state.cColDrop && cXfer > 0; iColDrop++) {
                                    if (lpcdT->idPlanetDst == rgxf[1].pl().id && lpcdT->idPlr == rgxf[0].iPlayerObj()) {
                                        l = cXfer < lpcdT->cColonist ? cXfer : lpcdT->cColonist;
                                        cXfer -= l;
                                        lpcdT->cColonist -= l;
                                    }
                                    lpcdT++;
                                }
                            } else {
                                for (; iColDrop < app.state.cColDrop && (lpcdT->idFleetSrc != rgxf[0].idObj() || lpcdT->idPlanetDst != rgxf[1].pl().id); iColDrop++) {
                                    lpcdT++;
                                }
                                if (iColDrop == app.state.cColDrop) {
                                    lpcdT->idFleetSrc = rgxf[0].idObj();
                                    lpcdT->idPlr = rgxf[0].iPlayerObj();
                                    lpcdT->idPlanetDst = rgxf[1].idObj();
                                    lpcdT->cColonist = 0;
                                    lpcdT->fCanColonize = rgxf[1].iPlayerObj() != iplrNone;
                                    app.state.cColDrop++;
                                }
                                lpcdT->cColonist -= cXfer;
                            }
                        } else {
                            if (cXfer != 0 && app.state.gd.fGeneratingTurn && rgxf[1].iPlayerObj() != rgxf[0].iPlayerObj() && CxvFromPrefix(lpb, 5).grobj2() != grobjThing) {
                                lpxfMax = app.state.lpxf + app.state.cXferFull;
                                if (cXfer > 0) {
                                    for (lpxfCur = app.state.lpxf; lpxfCur < lpxfMax && cXfer > 0; lpxfCur++) {
                                        if (lpxfCur->grobj2 == CxvFromPrefix(lpb, 5).grobj2() && lpxfCur->id2 == CxvFromPrefix(lpb, 4).id2() && lpxfCur->grobj1 == 2 &&
                                            (lpxfCur->id1 & 0xfe00) == (CxvFromPrefix(lpb, 2).id1() & 0xfe00)) {
                                            l = cXfer < lpxfCur->rgcQuan[i] ? cXfer : lpxfCur->rgcQuan[i];
                                            cXfer -= l;
                                            lpxfCur->rgcQuan[i] -= l;
                                        }
                                    }
                                    lpxfCur = nullptr;
                                    if (cXfer > 0)
                                        goto StealCargo;
                                    goto DoNext;
                                }
                                if (iPass == 1)
                                    goto StealCargo;
                                if (!lpxfCur) {
                                    for (lpxfCur = app.state.lpxf; lpxfCur < lpxfMax && std::memcmp(lpxfCur, lpb, 5) != 0; lpxfCur++) {
                                    }
                                    if (lpxfCur == lpxfMax) {
                                        app.state.cXferFull++;
                                        *lpxfCur = {};
                                        lpxfCur->id1 = CxvFromPrefix(lpb, 2).id1();
                                        lpxfCur->id2 = CxvFromPrefix(lpb, 4).id2();
                                    }
                                }
                                lpxfCur->rgcQuan[i] -= cXfer;
                                goto DoNext;
                            }
                        StealCargo:
                            l = ChgCargo(app, static_cast<GrobjClass>(CxvFromPrefix(lpb, 5).grobj2()), CxvFromPrefix(lpb, 4).id2(), static_cast<MineralType>(i), -cXfer, rgxf[1].LpvCargo(static_cast<GrobjClass>(CxvFromPrefix(lpb, 5).grobj2())));
                            if (l != -cXfer) {
                                rgcXfer[i] = -l;
                                /* Cargo the destination can't hold goes back to
                                   the source, which gave it up in the first pass.
                                   The original destroyed it, so minerals sent to
                                   another player's fleet beyond its free hold
                                   space were lost. */
                                if (cXfer < 0) {
                                    ChgCargo(app, static_cast<GrobjClass>(CxvFromPrefix(lpb, 5).grobj1()), CxvFromPrefix(lpb, 2).id1(), static_cast<MineralType>(i), -cXfer - l, rgxf[0].LpvCargo(static_cast<GrobjClass>(CxvFromPrefix(lpb, 5).grobj1())));
                                }
                                if (CxvFromPrefix(lpb, 5).grobj2() == grobjThing) {
                                    idm = idmDidntGetAttemptedTransferMineralPacketAnother;
                                    if (l == 0) {
                                        idm = static_cast<MessageId>(idm + 1);
                                    }
                                    TurnIO().pfnMessage(app, rgxf[0].iPlayerObj(), idm, static_cast<MsgGoto>(rgxf[0].idObj() | 0x8000), rgxf[0].idObj(), i, -LOWORD(l), i, 0, 0, 0);
                                } else {
                                    id = CxvFromPrefix(lpb, 5).grobj1() == grobjFleet ? -32768 : 0;
                                    id |= rgxf[0].idObj();
                                    TurnIO().pfnMessage(app, rgxf[0].iPlayerObj(), idmUnableTransferKtKtRequest, static_cast<MsgGoto>(id), id, -LOWORD(l) - LOWORD(cXfer), i, -LOWORD(cXfer), 0, 0,
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
            FleetQueryIO().pfnLookup(app, idWriteBack, &rgxf[0].fl());
        } else {
            PlanetQueryIO().pfnLookup(app, idWriteBack, &rgxf[0].pl());
        }
        switch (CxvFromPrefix(lpb, 5).grobj2()) {
        case grobjFleet:
            FleetQueryIO().pfnLookup(app, idWriteBack, &rgxf[1].fl());
            break;
        case grobjPlanet:
        case grobjOther:
            PlanetQueryIO().pfnLookup(app, idWriteBack, &rgxf[1].pl());
            break;
        case grobjThing:
            FLookupThing(app, idWriteBack, &rgxf[1].th());
        }
        break;
    case rtLogFleetSplit:
    case rtLogFleetMerge:
        FReadFleetId({std::as_bytes(std::span(lpb, cbWireFleetId))}, 0, idFleet);
        if (!FLookupObject(app, grobjFleet, idFleet, rgxf[0].LpvLookup(grobjFleet))) {
            return 0;
        }
        if (rt == rtLogFleetSplit) {
            if (LpflNewSplit(app, &rgxf[0].fl()))
                break;
            return 0;
        }
        app.state.vrgiflMerge = rgifl;
        app.state.vcflMerge = 0;
        if (cb == 2) {
            for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                lpfl = app.state.rglpfl[ifl];
                if (!app.state.rglpfl[ifl])
                    break;
                if (lpfl->iPlayer == app.state.idPlayer && !lpfl->fDead && lpfl->pt.x == rgxf[0].fl().pt.x && lpfl->pt.y == rgxf[0].fl().pt.y) {
                    rgifl[app.state.vcflMerge++] = lpfl->id;
                    lpfl->SetFCompChg(1);
                }
            }
        } else {
            for (i = 0; i < static_cast<std::uint32_t>(cb) / 2; i++) {
                for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                    lpfl = app.state.rglpfl[ifl];
                    if (!app.state.rglpfl[ifl])
                        break;
                    FReadFleetId({std::as_bytes(std::span(lpb + i * cbWireFleetId, cbWireFleetId))}, 0, idFleet);
                    if (lpfl->id == idFleet) {
                        rgifl[app.state.vcflMerge++] = lpfl->id;
                        lpfl->SetFCompChg(1);
                        break;
                    }
                }
            }
        }
        if (FFleetMergeAll(app, &rgxf[0].fl()))
            break;
        return 0;
    case rtLogFleetCargoXfer:
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb, 2))).FReadI16Le(idFleet);
        if (!FLookupObject(app, grobjFleet, idFleet, rgxf[0].LpvLookup(grobjFleet))) {
            return 0;
        }
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb + 2, 2))).FReadI16Le(idFleet);
        if (!FLookupObject(app, grobjFleet, idFleet, rgxf[1].LpvLookup(grobjFleet))) {
            return 0;
        }
        if (rgxf[1].iPlayerObj() != rgxf[0].iPlayerObj()) {
            return 0;
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
                        if (rgxf[0].fl().rgcsh[i] + cXfer < 0) {
                            cXfer = static_cast<std::int16_t>(-rgxf[0].fl().rgcsh[i]);
                        } else if (static_cast<std::int16_t>((32766 - rgxf[0].fl().rgcsh[i])) <= cXfer) {
                            cXfer = static_cast<std::int16_t>((32766 - rgxf[0].fl().rgcsh[i] - 1));
                        }
                        rgxf[0].fl().rgcsh[i] += LOWORD(cXfer);
                    }
                    if ((iPass == 0 && cXfer >= 0) || (iPass == 1 && cXfer < 0)) {
                        if (rgxf[1].fl().rgcsh[i] - cXfer < 0) {
                            cXfer = rgxf[1].fl().rgcsh[i];
                        } else if (static_cast<std::int16_t>((32766 - rgxf[1].fl().rgcsh[i])) <= -cXfer) {
                            cXfer = static_cast<std::int16_t>(-(32766 - rgxf[1].fl().rgcsh[i] - 1));
                        }
                        rgxf[1].fl().rgcsh[i] -= LOWORD(cXfer);
                    }
                    iLook++;
                }
                i++;
                grbit >>= 1;
            }
        }
        FleetTransferCargoBalance(app, &rgxf[0].fl(), &rgxf[1].fl());
        for (iPass = 0; iPass < 2; iPass++) {
            FleetQueryIO().pfnLookup(app, idWriteBack, &rgxf[iPass].fl());
            lpfl = LpflFromId(app, rgxf[iPass].idObj());
            if (lpfl) {
                lpfl->SetFCompChg(1);
            }
            for (i = 0; i < 16 && rgxf[iPass].fl().rgcsh[i] == 0; i++) {
            }
            if (i == 16) {
                FDeleteFleet(app, rgxf[iPass].idObj(), grobjNone, 0);
            }
        }
        break;
    case rtLogFleetOrderDelete:
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb, 2))).FReadI16Le(id);
        if (!(lpfl = LpflFromId(app, id)) || lpfl->cord <= 0) return 0;
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb + 2, 2))).FReadU16Le(wOrderLog);
        iLook = wOrderLog & 0x7fff;
        if (iLook >= lpfl->cord) return 0;
        fExtra = (wOrderLog & 0x8000) != 0;
        if (fExtra && iLook + 1 >= lpfl->cord) return 0;
        std::memmove(&lpfl->lpplord->rgord()[iLook], &lpfl->lpplord->rgord()[iLook + fExtra + 1], (lpfl->cord - iLook - fExtra - 1) * sizeof(ORDER));
        lpfl->cord -= fExtra + 1;
        lpfl->lpplord->iordMac -= fExtra + 1;
        break;
    case rtLogFleetOrderInsert:
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb, 2))).FReadI16Le(id);
        if (!(lpfl = LpflFromId(app, id))) return 0;
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb + 2, 2))).FReadI16Le(iordLog);
        if (iordLog < 0 || iordLog > lpfl->cord) return 0;
        if (lpfl->cord == lpfl->lpplord->iordMax) {
            lpfl->lpplord = LpplReAlloc(app, lpfl->lpplord, lpfl->cord + 3);
        }
        std::memmove(&lpfl->lpplord->rgord()[iordLog + 1], &lpfl->lpplord->rgord()[iordLog], (lpfl->cord - iordLog) * sizeof(ORDER));
        if (!FApplyOrderBytes(app, lpfl->lpplord, iordLog, lpb + cbWireWaypt, cb, static_cast<std::uint16_t>(cb) < cbWireWaypt + cbWireOrder)) return 0;
        lpfl->cord++;
        lpfl->lpplord->iordMac++;
        break;
    case rtLogFleetOrderUpdate:
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb, 2))).FReadI16Le(id);
        if (!(lpfl = LpflFromId(app, id)) || lpfl->cord < 0) return 0;
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb + 2, 2))).FReadI16Le(iLook);
        if (iLook >= lpfl->cord) return 0;
        if (!FApplyOrderBytes(app, lpfl->lpplord, iLook, lpb + cbWireWaypt, cb, static_cast<std::uint16_t>(cb) < cbWireWaypt + cbWireOrder)) return 0;
        break;
    case rtBtlPlan:
        if (cb < 2) return 0;
        // Deletions stop after the owner/plan word, before target/name bytes.
        i = BpvFromPrefix(lpb, 2).iplan();
        if (BpvFromPrefix(lpb, 2).iplr() != app.state.idPlayer || i < 0 || i > app.state.rgcbtlplan[app.state.idPlayer]) {
            if (BpvFromPrefix(lpb, 2).fDelete() != 0)
                break;
            goto BombOut;
        }
        if (BpvFromPrefix(lpb, 2).fDelete() != 0) {
            FDeleteBattlePlan(app, i, 0);
            break;
        }
        if (cb < 4) return 0;
        if (BpvFromPrefix(lpb, 4).mdTactic() > 6 || BpvFromPrefix(lpb, 4).mdTarget1() > 8 || BpvFromPrefix(lpb, 4).mdTarget2() > 8)
            goto BombOut;
        if (i == app.state.rgcbtlplan[app.state.idPlayer]) {
            if (i >= 16)
                goto BombOut;
        }
        if (!UnpackBattlePlan(app, lpb, app.state.rglpbtlplan[app.state.idPlayer] + i, i, cb)) return 0;
        if (i == app.state.rgcbtlplan[app.state.idPlayer]) app.state.rgcbtlplan[app.state.idPlayer]++;
        break;
    case rtLogFleetPlan:
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb, 2))).FReadI16Le(id);
        if (!(lpfl = LpflFromId(app, id))) goto BombOut;
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb + 2, 2))).FReadI16Le(iLook);
        lpfl->iplan = iLook;
        break;
    case rtLogFleetFlagBit9:
    case rtLogFleetOrderAttrNib:
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb, 2))).FReadI16Le(id);
        if (!(lpfl = LpflFromId(app, id))) {
        BombOut:
            return 0;
        }
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb + 2, 2))).FReadI16Le(iordLog);
        if (rt == rtLogFleetFlagBit9) {
            lpfl->fRepOrders = iordLog;
            break;
        }
        if (lpfl->cord <= iordLog) goto BombOut;
        NATIVEBYTEREADER(std::as_bytes(std::span(lpb + 4, 2))).FReadI16Le(grTaskLog);
        if (grTaskLog >= 10) goto BombOut;
        if (!FSetOrderTask(app, lpfl->lpplord, iordLog, grTaskLog)) return 0;
        break;
    case rtLogPlanetRouting:
        lppl = LpplFromId(app, RlvFromPrefix(lpb, 2).id());
        if (!lppl || lppl->iPlayer != app.state.idPlayer)
            goto BombOut;
        lppl->fNoResearch = RlvFromPrefix(lpb, 3).fNoResearch();
        lppl->idFling = RlvFromPrefix(lpb, 4).idFling();
        lppl->iWarpFling = RlvFromPrefix(lpb, 4).iWarpFling();
        lppl->idRoute = RlvFromPrefix(lpb, 6).idRoute();
        break;
    case rtChgPassword:
        if (app.state.gd.fGeneratingTurn) {
            NATIVEBYTEREADER(std::as_bytes(std::span(lpb, cbWirePassword))).FReadI32Le(l);
            app.state.rgplr[app.state.idPlayer].lSalt = l;
        }
        break;
    case rtLogResearch:
        ch = RsvFromPrefix(lpb, 1).pctResearch();
        if (ch < 0 || ch > 100)
            goto BombOut;
        app.state.rgplr[app.state.idPlayer].pctResearch = ch;
        if (RsvFromPrefix(lpb, 2).iTechNow() >= 6 || RsvFromPrefix(lpb, 2).iTechNext() > 7)
            goto BombOut;
        app.state.rgplr[app.state.idPlayer].iTechCur = RsvFromPrefix(lpb, 2).iTechCur();
        break;
    }
    return 1;
}

std::int16_t FLoadLogFile(APPSESSION &app, char *pszLog) {
    std::int16_t  fRet;
    std::int16_t  cbLog;
    std::int16_t  iCur;
    MSGPLR  *lpmp;
    MSGPLR **lppmp;
    std::int16_t  cSkip;

    fRet = 1;
    app.state.imemLogCur = 0;
    app.state.imemLogPrev = -1;
    StarsRecoveryScope scopeRecovery;
    try {
        if (app.state.game.fTutorial && app.state.idPlayer == 0 && app.state.gd.fGeneratingTurn) {
            cSkip = app.state.game.turn;
            app.state.vlpMemStream = LpbLoadTutorLog(app);
            if (!app.state.vlpMemStream) {
                return 0;
            }
            if (app.state.game.turn >= *app.state.vlpMemStream) {
                app.state.vlpMemStream = nullptr;
                goto StrOpen;
            }
            app.state.vlpMemStream++;
            while (cSkip-- != 0) {
                do {
                    app.state.vlpMemStream += 2 + HdvFromPrefix(app.state.vlpMemStream).cb();
                } while (HdvFromPrefix(app.state.vlpMemStream).rt() != rtBOF);
            }
        } else {
        StrOpen:
            RecordIO().pfnStreamOpen(app, pszLog, static_cast<MdOpenFlags>(16416));
        }

        RecordIO().pfnReadRt(app);
        if (app.state.hdrCur.rt != rtBOF || BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMajor() != 2 || BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMinor() != 85) {
            FileError(app, idsFileDoesBelongVersionStars);
            goto FailSuccess;
        }
        if (app.state.game.lid != BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).lidGame() || app.state.game.turn > BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).turn()) {
        FailSuccess:
            if (app.state.vlpMemStream) {
                app.state.vlpMemStream = nullptr;
            } else {
                RecordIO().pfnStreamClose(app);
            }
            return 1;
        }
        if (BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).turn() != app.state.game.turn) {
            FileError(app, idsLogFileRecentGameTryingLoadIgnoring);
            goto FailSuccess;
        }
        if (BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).wGen() != app.state.game.wGen) {
            FileError(app, idsFileGame);
            goto FailSuccess;
        }
        app.state.wVersFile = BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).wVersion();
        app.state.gd.fFileCrippled = BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fCrippled();
        if (app.state.gd.fGeneratingTurn) {
            app.state.rgplr[app.state.idPlayer].fCrippled = BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fCrippled();
        }
        RecordIO().pfnReadRt(app);
        NATIVEBYTEREADER(std::as_bytes(std::span(app.state.rgbCur)).first(2)).FReadI16Le(cbLog);
        for (iCur = 0; iCur < cbLog; iCur += app.state.hdrCur.cb + 2) {
            RecordIO().pfnReadRt(app);
            FEncodeHdr(app.state.hdrCur, std::as_writable_bytes(std::span(app.state.lpLog + iCur, cbWireHdr)));
            std::memmove(app.state.lpLog + (2 + iCur), app.state.rgbCur, app.state.hdrCur.cb);
        }
        RecordIO().pfnReadRt(app);
        for (lppmp = &app.state.vlpmsgplrOut; *lppmp; lppmp = &(*lppmp)->lpmsgplrNext) {
        }
        while (app.state.hdrCur.rt == rtPlrMsg) {
            *lppmp = LpmsgplrFromRt(app);
            if (*lppmp) {
                lpmp = *lppmp;
                lppmp = &lpmp->lpmsgplrNext;
                app.state.vcmsgplrOut++;
            }
            RecordIO().pfnReadRt(app);
        }
        if (app.state.hdrCur.rt != rtEOF) {
            fRet = 0;
            goto Done;
        }
        app.state.imemLogCur = cbLog;
Done:
        if (app.state.vlpMemStream) {
            app.state.vlpMemStream = nullptr;
        } else {
            RecordIO().pfnStreamClose(app);
        }
        RecordIO().pfnDirtyGame(app, 0);
        return fRet;
    } catch (const StarsError &) {
        if (app.state.vlpMemStream) {
            return 0;
        }
        if (app.state.hf == -1) {
            return 1;
        }
        RecordIO().pfnStreamClose(app);
        return 0;
    }
}

std::int16_t FCheckLogFile(APPSESSION &app, std::int16_t iplr, std::int16_t *pfError) {
    std::int16_t  fRet;
    std::int16_t  cbLog;
    std::int16_t  iCur;

    fRet = 1;
    app.state.imemLogCur = 0;
    app.state.imemLogPrev = -1;
    StarsRecoveryScope scopeRecovery;
    try {
        app.state.idsFileError = 0;
        if (!FOpenFile(app, dtLog, iplr, 32)) {
            if (app.state.idsFileError != idsCantOpenFile) {
                *pfError = app.state.idsFileError;
            }
            return 0;
        }
        RecordIO().pfnReadRt(app);
        NATIVEBYTEREADER(std::as_bytes(std::span(app.state.rgbCur)).first(2)).FReadI16Le(cbLog);
        for (iCur = 0; iCur < cbLog; iCur += app.state.hdrCur.cb + 2) {
            RecordIO().pfnReadRt(app);
        }
        RecordIO().pfnReadRt(app);
        while (app.state.hdrCur.rt == rtPlrMsg) {
            RecordIO().pfnReadRt(app);
        }
        if (app.state.hdrCur.rt != rtEOF) {
            *pfError = 3;
            fRet = 0;
            goto Done;
        }
        app.state.imemLogCur = cbLog;
Done:
        RecordIO().pfnStreamClose(app);
        return fRet;
    } catch (const StarsError &) {
        if (app.state.hf == -1) {
            return 1;
        }
        RecordIO().pfnStreamClose(app);
        *pfError = 3;
        return 0;
    }
}

std::int16_t FWriteLogFile(APPSESSION &app, char *pszFileBase, std::int16_t iPlayer) {
    std::int16_t  iCur;
    std::uint8_t *lprts;
    RTLOGHDR rtlh;
    std::uint8_t rgbLogHdr[cbWireLogHdr];
    MSGPLR  *lpmp;
    std::int16_t  cb;

    iCur = 0;
    if (iPlayer == app.state.idPlayer && !app.state.rgplr[iPlayer].fAi && app.state.hdrPrev.rt != rtLogPlayerZpq1) {
        cb = 26 - (12 - app.state.vrgZipProd[0].cpq) * 2;
        std::byte rgbPlayer[16 * cbWirePlayer], rgbZip[5 * cbWireZipProdQ];
        for (int i = 0; i < 16; i++) FEncodePlayer(app.state.rgplr[i], std::span(rgbPlayer).subspan(i * cbWirePlayer));
        for (int i = 0; i < 5; i++) FEncodeZipProdQ(app.state.vrgZipProd[i], std::span(rgbZip).subspan(i * cbWireZipProdQ));
        // Historical over-counts can compare into adjacent initialized template/
        // player storage. Preserve that physical image rather than clamp to 26.
        assert(static_cast<std::size_t>(cb) <= sizeof(rgbZip) - 14);
        assert(static_cast<std::size_t>(iPlayer) * cbWirePlayer + 86 + cb <= sizeof(rgbPlayer));
        if (std::memcmp(rgbPlayer + iPlayer * cbWirePlayer + 86, rgbZip + 14, cb) != 0) {
            WriteMemZpqRt(app, cb);
        }
    }
    /* NATIVE: callers pass szBase itself, and strcpy onto itself is
       undefined. */
    if (pszFileBase != app.state.szBase) {
        std::strcpy(app.state.szBase, pszFileBase);
    }
    if (!RecordIO().pfnFCreateFile(app, dtLog, iPlayer, nullptr)) {
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableCreateLogFile, nullptr), win32::MB_ICONHAND);
        return 0;
    }
    StarsRecoveryScope scopeRecovery;
    try {
        rtlh.cbLog = app.state.imemLogCur;
        /* Preserve the unused 15-byte metadata area in the orders-file header. */
        std::ranges::fill(rtlh.rgbUnused, 0);
        FEncodeLogHdr(rtlh, std::as_writable_bytes(std::span(rgbLogHdr)));
        RecordIO().pfnWriteRt(app, static_cast<RecordType>(9), cbWireLogHdr, rgbLogHdr);
        for (; iCur < app.state.imemLogCur; iCur += HdvFromPrefix(lprts).cb() + 2) {
            lprts = app.state.lpLog + iCur;
            RecordIO().pfnWriteRt(app, static_cast<RecordType>(HdvFromPrefix(lprts).rt()), HdvFromPrefix(lprts).cb(), app.state.lpLog + (2 + iCur));
        }
        iCur = app.state.vcmsgplrOut;
        lpmp = app.state.vlpmsgplrOut;
        while (iCur-- != 0) {
            WriteRtPlrMsg(app, lpmp);
            lpmp = lpmp->lpmsgplrNext;
        }
        RecordIO().pfnWriteRt(app, rtEOF, 0, nullptr);
        RecordIO().pfnStreamClose(app);
        RecordIO().pfnDirtyGame(app, 0);
        app.state.gd.fWriteTurnNum = 1;
        return 1;
    } catch (const StarsError &) {
        RecordIO().pfnStreamClose(app);
        return 0;
    }
}

std::int16_t FWriteHistFile(APPSESSION &app, std::int16_t iPlayer) {
    PLANET   *lppl;
    std::int16_t   i;
    std::uint16_t  cTurnBase;
    SHDEF    *lpshdef;
    std::int16_t   j;
    RTHISTHDR rthh;
    std::byte rgbHistHdr[cbWireHistHdr];
    std::uint8_t  *lpb;

    if (!RecordIO().pfnFCreateFile(app, dtHist, iPlayer, nullptr)) {
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableCreateHistoryFile, nullptr), win32::MB_ICONHAND);
        return 0;
    }
    StarsRecoveryScope scopeRecovery;
    try {
        rthh.cPlanet = app.state.cPlanet;
        rthh.cPlanetExtra = app.state.rgplr[iPlayer].cFleet;
        if (!FEncodeRtHistHdr(rthh, rgbHistHdr)) { RecordIO().pfnStreamClose(app); return 0; }
        RecordIO().pfnWriteRt(app, rtHistHdr, cbWireHistHdr, rgbHistHdr);
        i = 0;
        lppl = app.state.lpPlanets;
        while (i < app.state.cPlanet) {
            WritePlanet(app, lppl, static_cast<RecordType>(lppl->det < detSome ? 15 : rtPlanetB), 1);
            i++;
            lppl++;
        }
        RecordIO().pfnWriteRt(app, rtMsgFilt, app.state.cbbitfMsg, app.state.bitfMsgFiltered);
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (i != iPlayer && app.state.rgplr[i].det != detNone) {
                WriteRtPlr(app, &app.state.rgplr[i], nullptr);
            }
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (app.state.rgplr[i].fInclude && i != iPlayer) {
                lpshdef = app.state.rglpshdef[i];
                for (j = 0; j < 16; j++) {
                    if (!lpshdef[j].fFree) {
                        WriteRtShDef(app, lpshdef + j, nullptr);
                    }
                }
            }
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (app.state.rgplr[i].fInclude && i != iPlayer) {
                lpshdef = app.state.rglpshdefSB[i];
                for (j = 0; j < 10; j++) {
                    if (!lpshdef[j].fFree) {
                        WriteRtShDef(app, lpshdef + j, nullptr);
                    }
                }
            }
        }
        if (app.state.game.turn <= 100) {
            cTurnBase = 0;
        } else {
            cTurnBase = app.state.game.turn - 100;
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (app.state.rgsxPlr[i]) {
                for (j = 0; j < app.state.rgcsxPlr[i]; j++) {
                    if (app.state.rgsxPlr[i][j].turn >= cTurnBase) {
                        WriteRtScore(app, app.state.rgsxPlr[i] + j);
                    }
                }
            }
        }
        if (app.state.vlpbAiData && AhvFromLp(app, app.state.vlpbAiData).cbAiHist() > 2) {
            i = AhvFromLp(app, app.state.vlpbAiData).cbAiHist();
            lpb = app.state.vlpbAiData;
            for (; i >= 1024; i -= 1023) {
                RecordIO().pfnWriteRt(app, rtAiData, 1023, lpb);
                lpb += 1023;
            }
            RecordIO().pfnWriteRt(app, rtAiData, i, lpb);
        }
        RecordIO().pfnWriteRt(app, rtEOF, 0, nullptr);
        RecordIO().pfnStreamClose(app);
        return 1;
    } catch (const StarsError &) {
        RecordIO().pfnStreamClose(app);
        return 0;
    }
}

void EnumLogRts(APPSESSION &app, std::int16_t (*pfn)(APPSESSION &, void *, std::int16_t, std::int16_t, void *, std::int16_t), void *lpPass, std::int16_t iPass) {
    std::int16_t fLogOld;
    std::int16_t fRet;
    std::int16_t iCur;
    std::uint8_t *lprts;

    iCur = 0;
    fRet = 1;
    fLogOld = app.state.fLogOff;
    if (app.state.imemLogCur != 0) {
        for (; iCur < app.state.imemLogCur; iCur += HdvFromPrefix(lprts).cb() + 2) {
            lprts = app.state.lpLog + iCur;
            if (pfn(app, app.state.lpLog + (2 + iCur), HdvFromPrefix(lprts).rt(), HdvFromPrefix(lprts).cb(), lpPass, iPass) == 0)
                break;
        }
    }
    return;
}
