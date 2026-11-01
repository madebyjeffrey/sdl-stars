#include "common.h"
#include "appstate.h"
#include "text_io.h"
#include "heap_io.h"
#include "colony_io.h"
#include "planet_query_io.h"
#include "research_io.h"
#include "part_query_io.h"
#include "race_query_io.h"
#include "alert_io.h"
#include "record_io.h"

char mpishdefishTutor[6] = {3, 4, 9, 6, 7, 14};

// The length marker and raw terminator must both belong to this record.
int16_t FReadUserString(APPSESSION &app, const uint8_t *lpb, size_t cb, char *psz, size_t cchMax, size_t *pcbRead) {
    const uint8_t *lpbEnd;
    size_t cch;
    int16_t cOut;

    if (cchMax == 0) return 0;
    psz[0] = 0;
    if (cb == 0 || cchMax > 1024) return 0;
    if (*lpb == 0) {
        lpbEnd = (const uint8_t *)memchr(lpb + 1, 0, StarsMin(cb - 1, cchMax));
        if (!lpbEnd) return 0;
        cch = (size_t)(lpbEnd - lpb - 1);
        if (CchCopyUtf8(psz, cchMax, (const char *)lpb + 1, cch) < 0) return 0;
        if (pcbRead) *pcbRead = cch + 2;
    } else {
        if (*lpb > cb - 1) return 0;
        cOut = (int16_t)cchMax - 1;
        if (!FDecompressUserString(app, (char *)lpb + 1, *lpb, psz, &cOut)) return 0;
        if (pcbRead) *pcbRead = (size_t)*lpb + 1;
    }
    return 1;
}

int16_t FReadShDef(APPSESSION &app, const uint8_t *lprt, SHDEF *lpshdef, int16_t iplrLoad, size_t cb) {
    SHDEF    shdef;
    const uint8_t *lpb;
    int16_t  ishdef;
    int16_t  iFirst;
    int16_t  fOkay;
    HUL     *lphulBase;
    uint32_t wt;
    int16_t  c;
    HUL     *lphul;
    PART     part;
    size_t   ibName;

    memset(&shdef, 0, sizeof(SHDEF));

    if (!FDecodeShDefFixed(std::as_bytes(std::span(lprt, cb)), shdef, ibName) ||
        shdef.hul.ihuldef > ihuldefDeathStart || cb <= ibName) return 0;
    lpb = lprt + ibName;
    iFirst = PartQueryIO().pfnHull(shdef.hul.ihuldef)->hul.ibmp;
    if (shdef.hul.ibmp < iFirst || shdef.hul.ibmp >= iFirst + 4) {
        shdef.hul.ibmp = (shdef.hul.ibmp & 3) | iFirst;
    }
    if (!FReadUserString(app, lpb, cb - (size_t)(lpb - lprt), shdef.hul.szClass,
                         sizeof(shdef.hul.szClass), nullptr)) return 0;
    ishdef = shdef.ishdef;
    if (ishdef >= 16) {
        ishdef -= 16;
    }
    if (shdef.det == detAll || lpshdef[ishdef].fFree || lpshdef[ishdef].det < detAll) {
        lpshdef[ishdef] = shdef;
    } else if (shdef.hul.ihuldef != lpshdef[ishdef].hul.ihuldef || shdef.hul.ibmp != lpshdef[ishdef].hul.ibmp) {
        lpshdef[ishdef] = shdef;
    }
    if (app.state.idPlayer != iplrNone) {
        UpdateShdefCost(app, lpshdef + ishdef);
    }
    if (lpshdef[ishdef].det == detAll) {
        lphul = &lpshdef[ishdef].hul;
        lphulBase = &PartQueryIO().pfnHull(lphul->ihuldef)->hul;
        wt = (uint32_t)lphulBase->wtEmpty;
        for (c = 0; c < lphul->chs; c++) {
            if (lphul->rghs[c].cItem > 0) {
                part.hs = lphul->rghs[c];
                fOkay = PartQueryIO().pfnLookup(app, &part);
                if (app.state.idPlayer == iplrNone) {
                    fOkay = 0;
                }
                if (!(part.hs.grhst & lphulBase->rghs[c].grhst) || ((fOkay > 1 && !shdef.fGift) || part.hs.cItem > lphulBase->rghs[c].cItem)) {
                    lphul->rghs[c].cItem = 0;
                }
                wt += (uint32_t)(part.pcom().cMass() * lphul->rghs[c].cItem);
            }
            if (c == 0 && lphul->rghs[0].cItem == 0 && lphulBase->rghs[0].grhst == hstEngine) {
                lphul->rghs[0].grhst = hstEngine;
                lphul->rghs[0].iItem = iengineQuickJump5;
                lphul->rghs[0].cItem = lphulBase->rghs[0].cItem;
                part.hs = lphul->rghs[0];
                PartQueryIO().pfnLookup(app, &part);
                wt += (uint32_t)(part.pcom().cMass() * lphul->rghs[0].cItem);
            }
        }
        lphul->wtEmpty = LOWORD(wt);
    }
    return 1;
}

void ReadRtPlr(APPSESSION &app, PLAYER *pplr, uint8_t *pbIn, size_t cb) {
    int16_t iOff;
    size_t cbRead;

    memset(pplr, 0, sizeof(PLAYER));
    if (cb < cbWirePlayerPartial) goto invalid;
    // A full record stores a relation count where rgmdRelation begins, then
    // that many relations; a partial one stops before idPlanetHome.
    if (PlvFromPrefix(pbIn, cb).det() == detAll) {
        iOff = cbWirePlayerPrefix;
        if (cb <= (size_t)iOff || pbIn[iOff] > cb - iOff - 1) goto invalid;
        FDecodePlayer(std::as_bytes(std::span(pbIn, cb)), *pplr, iOff);
        // Keep a corrupt count from overrunning rgmdRelation; skip the rest.
        memmove(pplr->rgmdRelation, pbIn + iOff + 1, (((pbIn[iOff]) < (sizeof(pplr->rgmdRelation))) ? (pbIn[iOff]) : (sizeof(pplr->rgmdRelation))));
        iOff += pbIn[iOff] + 1;
    } else {
        iOff = cbWirePlayerPartial;
        FDecodePlayer(std::as_bytes(std::span(pbIn, cb)), *pplr, iOff);
    }
    if (!FReadUserString(app, pbIn + iOff, cb - iOff, pplr->szName, sizeof(pplr->szName), &cbRead)) goto invalid;
    iOff += (int16_t)cbRead;
    if (!FReadUserString(app, pbIn + iOff, cb - iOff, pplr->szNames, sizeof(pplr->szNames), nullptr)) goto invalid;
    pplr->fLearned = 0;
    return;
invalid:
    FileError(app, idsGameFileAppearsCorruptUnableLoadFile);
    if (FStarsRecoveryActive()) StarsThrowError(starsErrorFile, app.state.idsFileError);
}

int16_t FLoadGame(APPSESSION &app, const char *pszFileName, const char *pszExt) {
    int16_t  iplrSav;
    int16_t  cPlanetHist;
    STARPACK sp;
    std::byte rgbStarpack[cbWireStarpack];
    int16_t  cPlanetAlloc;
    int16_t  fHaveHistoryData;
    int16_t  fSilentSav;
    PLANET  *lppl;
    int16_t  i;
    THING   *lpth;
    FLEET   *lpfl;
    int16_t  cturn;
    THING   *lpthMac;
    int16_t  iPlayer;
    int16_t  j;
    PLANET  *lpplMac;
    int16_t  dt;
    int16_t  grf;
    int16_t  x;
    int16_t  iplr;
    SCOREX   sx;
    int16_t  isx;
    uint16_t turnCur;
    uint8_t *lpb;
    int16_t  cThingFile;
    int16_t  fHist;
    int16_t  iP;
    int16_t  fWorking;
    int16_t  iprod;
    int16_t  iFirst;
    int16_t  iLast;
    PROD    *lpprod;
    int16_t  iWarp;
    int16_t  fTwo;
    RTHISTHDR rthh;
    HB      *lphb; /* NATIVE: recover the Win16 heap-relative offset. */

    grf = 0;
    cturn = 0;
    /* NATIVE: callers often pass szBase itself, and strcpy onto itself is
       undefined. */
    if (pszFileName != app.state.szBase) {
        strcpy(app.state.szBase, pszFileName);
    }
    app.state.gd.fFleetLinkValid = 0;
    StarsRecoveryScope scopeRecovery;
    try {
        if (!FOpenFile(app, dtXY, iplrNone, 32))
            goto LError;
        RecordIO().pfnReadRt(app);
        if (app.state.hdrCur.rt != rtGame || app.state.hdrCur.cb != cbWireGame) {
        XYCorrupt:
            AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUniverseDefinitionFileSeemsMissingCorrupt, nullptr), win32::MB_ICONHAND);
            goto LError;
        }
        FDecodeGame(std::as_bytes(std::span(app.state.rgbCur)), app.state.game);
        if (strnlen(app.state.game.szName, sizeof(app.state.game.szName)) == sizeof(app.state.game.szName) ||
            !FUtf8Valid(app.state.game.szName, strlen(app.state.game.szName))) goto XYCorrupt;
        app.state.game.fDirty = 0;
        app.state.dGal = 400 * app.state.game.mdSize + 400;
        app.state.dGalInv = app.state.dGal + 2000;
        x = 1000;
        for (i = 0; i < app.state.game.cPlanMax; i++) {
            RgFromStream(app, rgbStarpack, cbWireStarpack);
            FDecodeStarpack(rgbStarpack, sp);
            x += sp.dx;
            app.state.rgptPlan[i].x = x;
            app.state.rgptPlan[i].y = sp.y;
            app.state.rgidPlan[i] = sp.id;
            if (x >= app.state.dGal + 1000 || app.state.rgptPlan[i].y >= app.state.dGal + 1000 || app.state.rgidPlan[i] > cPlanetName)
                goto XYCorrupt;
        }
        RecordIO().pfnReadRt(app);
        if (app.state.hdrCur.rt != rtEOF)
            goto XYCorrupt;
        RecordIO().pfnStreamClose(app);
        if ((*pszExt == 'h' || *pszExt == 'H') && (pszExt[1] == 's' || pszExt[1] == 'S')) {
            dt = 2;
            iPlayer = iplrNone;
        } else {
            dt = 3;
            grf |= 0x3000;
            iPlayer = atoi(pszExt + 1);
            iPlayer--;
        }
        ResetMessages(app);
        memset(app.state.rgplr, 0, app.state.game.cPlayer * 192);
        ResetHb(app, htShips);
        app.state.idPlayer = iPlayer;
        fSilentSav = app.state.fFileErrSilent;
        app.state.fFileErrSilent = 1;
        if (iPlayer != iplrNone && FOpenFile(app, dtHist, iPlayer, 32)) {
            RecordIO().pfnReadRt(app);
            if (app.state.hdrCur.rt != rtHistHdr)
                goto CorruptHist;
            // Preserve the legacy reusable-buffer prefix even for a short
            // declared payload; this migration does not tighten admission.
            if (!FDecodeRtHistHdr(std::as_bytes(std::span(app.state.rgbCur)), rthh)) goto CorruptHist;
            cPlanetHist = rthh.cPlanet;
            cPlanetAlloc = cPlanetHist + rthh.cPlanetExtra;
            if (cPlanetAlloc > 1000) {
                cPlanetAlloc = 1000;
            }
            app.state.lpPlanets = LpAlloc<PLANET>(app, (1 <= cPlanetAlloc ? cPlanetAlloc : 1) * sizeof(PLANET), htPlanets);
            RecordIO().pfnReadRt(app);
            for (i = 0, lppl = app.state.lpPlanets; i < cPlanetHist; i++, lppl++) {
                if (app.state.hdrCur.rt != rtPlanetB)
                    goto CorruptHist;
                if (FReadPlanet(app, iPlayer, lppl, 1, 0)) {
                    if (lppl->iPlayer == iPlayer) {
                        lppl->iPlayer = iplrNone;
                        lppl->det = detSome;
                    }
                } else {
                CorruptHist:
                    RecordIO().pfnStreamClose(app);
                    AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsHistoryFileAppearsCorruptHistoricalDataWill, nullptr), win32::MB_ICONHAND);
                    goto LNoHistFile;
                }
                RecordIO().pfnReadRt(app);
            }
            if (app.state.hdrCur.rt == rtMsgFilt) {
                if (app.state.hdrCur.cb > (uint16_t)app.state.cbbitfMsg)
                    goto CorruptHist;
                memcpy(app.state.bitfMsgFiltered, app.state.rgbCur, app.state.hdrCur.cb);
                RecordIO().pfnReadRt(app);
            }
            while (app.state.hdrCur.rt == rtPlr) {
                i = PlvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).iPlayer();
                ReadRtPlr(app, &app.state.rgplr[i], (uint8_t *)app.state.rgbCur, app.state.hdrCur.cb);
                app.state.rgplr[i].cPlanet = 0;
                app.state.rgplr[i].cFleet = 0;
                RecordIO().pfnReadRt(app);
            }
            i = 0;
            while (app.state.hdrCur.rt == rtShDef) {
                for (; app.state.rgplr[i].cShDef == 0 && i < app.state.game.cPlayer; i++) {
                }
                if (i == app.state.game.cPlayer)
                    break;
                if (!app.state.rglpshdef[i]) {
                    app.state.rglpshdef[i] = LpAlloc<SHDEF>(app, 16 * sizeof(SHDEF), htShips);
                    for (j = 0; j < 16; j++) {
                        app.state.rglpshdef[i][j].fFree = 1;
                        app.state.rglpshdef[i][j].grbitPlr = 0;
                    }
                }
                iplrSav = app.state.idPlayer;
                if (app.state.idPlayer == iplrNone) {
                    app.state.idPlayer = i;
                } else {
                    app.state.idPlayer = iplrNone;
                }
                if (!FReadShDef(app, (const uint8_t *)app.state.rgbCur, app.state.rglpshdef[i], iplrSav, app.state.hdrCur.cb))
                    goto CorruptHist;
                app.state.idPlayer = iplrSav;
                app.state.rgplr[i].cShDef--;
                RecordIO().pfnReadRt(app);
            }
            i = 0;
            while (app.state.hdrCur.rt == rtShDef) {
                for (; app.state.rgplr[i].cshdefSB == 0 && i < app.state.game.cPlayer; i++) {
                }
                if (i == app.state.game.cPlayer)
                    break;
                if (!app.state.rglpshdefSB[i]) {
                    app.state.rglpshdefSB[i] = LpAlloc<SHDEF>(app, 10 * sizeof(SHDEF), htShips);
                    for (j = 0; j < 10; j++) {
                        app.state.rglpshdefSB[i][j].fFree = 1;
                        app.state.rglpshdefSB[i][j].grbitPlr = 0;
                    }
                }
                iplrSav = app.state.idPlayer;
                if (app.state.idPlayer == iplrNone) {
                    app.state.idPlayer = i;
                } else {
                    app.state.idPlayer = iplrNone;
                }
                if (!FReadShDef(app, (const uint8_t *)app.state.rgbCur, app.state.rglpshdefSB[i], iplrSav, app.state.hdrCur.cb))
                    goto CorruptHist;
                app.state.idPlayer = iplrSav;
                app.state.rgplr[i].cshdefSB += 15;
                RecordIO().pfnReadRt(app);
            }
            while (app.state.hdrCur.rt == rtScore) {
                iplr = static_cast<uint8_t>(app.state.rgbCur[0]) & 31;
                FDecodeScoreX(std::as_bytes(std::span(app.state.rgbCur)), sx);
                if (!app.state.rgsxPlr[iplr]) {
                    app.state.rgsxPlr[iplr] = LpAlloc<SCOREX>(app, 101 * sizeof(SCOREX), htMisc);
                    app.state.rgcsxPlr[iplr] = 0;
                }
                if (app.state.rgsxPlr[iplr]) {
                    if (sx.fHistory) {
                        turnCur = sx.turn;
                    } else {
                        turnCur = app.state.game.turn;
                    }
                    for (isx = 0; isx < app.state.rgcsxPlr[iplr] && turnCur > app.state.rgsxPlr[iplr][isx].turn; isx++) {
                    }
                    if ((isx >= app.state.rgcsxPlr[iplr] || turnCur == app.state.rgsxPlr[iplr][isx].turn) && isx < 101) {
                        if (isx == app.state.rgcsxPlr[iplr]) {
                            app.state.rgcsxPlr[iplr]++;
                        }
                    } else if (app.state.rgcsxPlr[iplr] < 101) {
                        memmove(app.state.rgsxPlr[iplr] + (isx + 1), app.state.rgsxPlr[iplr] + isx, (app.state.rgcsxPlr[iplr] - isx) * sizeof(SCOREX));
                        app.state.rgcsxPlr[iplr]++;
                    } else if (isx > 0) {
                        if (isx > 1) {
                            memmove(app.state.rgsxPlr[iplr], app.state.rgsxPlr[iplr] + 1, (isx - 1) * sizeof(SCOREX));
                        }
                        isx--;
                    }
                    app.state.rgsxPlr[iplr][isx] = sx;
                    app.state.rgsxPlr[iplr][isx].turn = turnCur;
                    app.state.rgsxPlr[iplr][isx].fHistory = 1;
                }
                RecordIO().pfnReadRt(app);
            }
            if (app.state.hdrCur.rt == rtAiData) {
                if (app.state.rgplr[app.state.idPlayer].fAi) {
                    if (!app.state.vlpbAiData) {
                        app.state.vlpbAiData = LpAlloc<uint8_t>(app, 8096, htMisc);
                        if (!app.state.vlpbAiData)
                            goto CorruptHist;
                    }
                    lpb = app.state.vlpbAiData;
                    while (app.state.hdrCur.rt == rtAiData) {
                        memmove(lpb, app.state.rgbCur, app.state.hdrCur.cb);
                        lpb += app.state.hdrCur.cb;
                        RecordIO().pfnReadRt(app);
                    }
                } else {
                    while (app.state.hdrCur.rt == rtAiData) {
                        RecordIO().pfnReadRt(app);
                    }
                }
            }
            if (app.state.hdrCur.rt == rtThing) {
                app.state.cThing = std::bit_cast<int16_t>(WFromStreamBytes(app.state.rgbCur));
                app.state.cThingAlloc = app.state.cThing + 10;
                if (app.state.cThingAlloc > 4050) {
                    app.state.cThingAlloc = 4050;
                }
                app.state.lpThings = LpAlloc<THING>(app, app.state.cThingAlloc * sizeof(THING), htThings);
                if (!app.state.lpThings)
                    goto CorruptHist;
                memset(app.state.lpThings, 0, app.state.cThingAlloc * sizeof(THING));
                RecordIO().pfnReadRt(app);
                for (i = 0, lpth = app.state.lpThings; i < app.state.cThing; i++, lpth++) {
                    if (app.state.hdrCur.rt != rtThing)
                        goto CorruptHist;
                    CopyThingBytes(app, lpth, app.state.rgbCur, app.state.hdrCur.cb);
                    RecordIO().pfnReadRt(app);
                }
            }
            RecordIO().pfnStreamClose(app);
        } else {
        LNoHistFile:
            cPlanetHist = 0;
            HeapIO().pfnFree(app, app.state.lpPlanets, htPlanets);
            app.state.lpPlanets = nullptr;
            app.state.cThing = 0;
            HeapIO().pfnFree(app, app.state.lpThings, htThings);
            app.state.lpThings = nullptr;
        }
        app.state.fFileErrSilent = fSilentSav;
        GetFileStatus(app, dt, iPlayer);
        if (!FOpenFile(app, static_cast<DtFileType>(dt | grf), iPlayer, 32))
            goto LError;
        if (iPlayer == iplrNone) {
            app.state.gd.fGameOverMan = BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fGameOverMan();
        }
LNextTurn:
        cturn++;
        app.state.cPlanet = 0;
        app.state.cFleet = 0;
        RecordIO().pfnReadRt(app);
        while (app.state.hdrCur.rt == rtBtlData || app.state.hdrCur.rt == rtContinue) {
            if (app.state.hdrCur.rt != rtContinue) {
                if (!app.state.lpbBattleLog) {
                    app.state.lpbBattleLog = LpAlloc<uint8_t>(app, 0xffc8, htBattle);
                    app.state.lpbBattleCur = app.state.lpbBattleLog;
                }
                /* NATIVE: Win16 offsets include its 16-byte HB, not native address bits. */
                lphb = LphbFromLpHt(app, app.state.lpbBattleCur, htBattle);
                if (0xffc8 - (uint32_t)(app.state.lpbBattleCur - (uint8_t *)lphb - sizeof(HB) + 16) < (uint32_t)NATIVEBATTLEDATAVIEW(std::as_writable_bytes(std::span(app.state.rgbCur)), 0).cbData()) {
                    RawStore16(app.state.lpbBattleCur, 0xffff);
                    app.state.lpbBattleCur = LpAlloc<uint8_t>(app, 0xffc8, htBattle);
                }
            }
            memmove(app.state.lpbBattleCur, app.state.rgbCur, app.state.hdrCur.cb);
            app.state.lpbBattleCur += app.state.hdrCur.cb;
            RecordIO().pfnReadRt(app);
        }
        if (app.state.lpbBattleCur) {
            RawStore16(app.state.lpbBattleCur, 0xffff);
            if (VerMinor(app.state.wVersFile) < 80) {
                UpdateBattleRecords(app);
            }
        }
        while (app.state.hdrCur.rt == rtPlr) {
            i = PlvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).iPlayer();
            ReadRtPlr(app, &app.state.rgplr[i], (uint8_t *)app.state.rgbCur, app.state.hdrCur.cb);
            app.state.cPlanet += app.state.rgplr[i].cPlanet;
            app.state.rgplr[i].cPlanet = 0;
            app.state.cFleet += app.state.rgplr[i].cFleet;
            app.state.rgplr[i].cFleet = 0;
            RecordIO().pfnReadRt(app);
        }
        if (dt != 2) {
            app.state.lSaltCur = app.state.rgplr[iPlayer].lSalt;
        } else if (app.state.hdrCur.rt == rtChgPassword) {
            NATIVEBYTEREADER(std::as_bytes(std::span(app.state.rgbCur)).first(cbWirePassword)).FReadI32Le(app.state.lSaltCur);
            RecordIO().pfnReadRt(app);
        } else {
            app.state.lSaltCur = 0;
        }
        if (!FCheckPassword(app)) {
            if (app.state.ini.fValidate || app.state.ini.fLogging)
                AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsPasswordHaveEnteredIncorrectPleaseTry, nullptr), win32::MB_ICONHAND);
            goto LError;
        }
        ReadPlayerMessages(app);
        ResetHb(app, htFleets);
        ResetHb(app, htOrd);
        HeapIO().pfnFree(app, app.state.rglpfl, htMisc);
        app.state.rglpfl = nullptr;
        if (!app.state.lpPlanets) {
            cPlanetAlloc = 1 <= app.state.cPlanet ? app.state.cPlanet : 1;
            app.state.lpPlanets = LpAlloc<PLANET>(app, cPlanetAlloc * sizeof(PLANET), htPlanets);
        }
        lppl = app.state.lpPlanets;
        j = 0;
        for (i = 0; i < app.state.cPlanet; i++) {
            fHaveHistoryData = 0;
            if (cPlanetHist != 0) {
                while (j < cPlanetHist && PpvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).id() > lppl->id) {
                    j++;
                    lppl++;
                }
                if (j < cPlanetHist) {
                    if (PpvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).id() == lppl->id) {
                        fHaveHistoryData = 1;
                        goto LFoundPlanet;
                    }
                }
                if (cPlanetAlloc == cPlanetHist) {
                    cPlanetAlloc += 8;
                    app.state.lpPlanets = LpReAlloc<PLANET>(app, app.state.lpPlanets, cPlanetAlloc * sizeof(PLANET), htPlanets);
                    lppl = app.state.lpPlanets + j;
                }
                if (j < cPlanetHist) {
                    memmove(lppl + 1, lppl, (cPlanetHist - j) * sizeof(PLANET));
                }
                cPlanetHist++;
            }
        LFoundPlanet:
            if (!FReadPlanet(app, iPlayer, lppl, 0, fHaveHistoryData))
                goto Corrupt;
            if (lppl->iPlayer != iplrNone) {
                app.state.rgplr[lppl->iPlayer].cPlanet = app.state.rgplr[lppl->iPlayer].cPlanet + 1;
            }
            RecordIO().pfnReadRt(app);
            if (app.state.hdrCur.rt == rtProdQ) {
                if (lppl->lpplprod && lppl->lpplprod->iprodMax <= app.state.hdrCur.cb / 4) {
                    FreePl(app, lppl->lpplprod);
                    lppl->lpplprod = nullptr;
                }
                if (!lppl->lpplprod) {
                    lppl->lpplprod = LpplAlloc<PLPROD>(app, 4, app.state.hdrCur.cb / 4 + 2, htOrd);
                }
                if (!FDecodeProdBytes(std::as_bytes(std::span(app.state.rgbCur).first(app.state.hdrCur.cb)),
                                      std::span(lppl->lpplprod->rgprod(), app.state.hdrCur.cb / cbWireProd + (app.state.hdrCur.cb % cbWireProd != 0)))) goto Corrupt;
                lppl->lpplprod->iprodMac = app.state.hdrCur.cb / 4;
                RecordIO().pfnReadRt(app);
            }
            if (cPlanetHist == 0) {
                lppl++;
            }
        }
        if (cPlanetHist != 0) {
            app.state.cPlanet = cPlanetHist;
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (i == iPlayer) {
                app.state.rglpshdef[i] = app.state.rgshdef;
                goto FreeShdef;
            }
            if (app.state.rgplr[i].fInclude) {
                if (!app.state.rglpshdef[i]) {
                    app.state.rglpshdef[i] = LpAlloc<SHDEF>(app, 16 * sizeof(SHDEF), htShips);
                FreeShdef:
                    for (j = 0; j < 16; j++) {
                        app.state.rglpshdef[i][j].fFree = 1;
                        app.state.rglpshdef[i][j].grbitPlr = 0;
                    }
                }
            } else {
                continue;
            }
            iplrSav = app.state.idPlayer;
            if (app.state.idPlayer == iplrNone) {
                app.state.idPlayer = i;
            } else if (i != app.state.idPlayer) {
                app.state.idPlayer = iplrNone;
            }
            for (j = 0; j < app.state.rgplr[i].cShDef; j++) {
                if (app.state.hdrCur.rt != rtShDef) {
                    app.state.idPlayer = iplrSav;
                    goto Corrupt;
                }
                if (!FReadShDef(app, (const uint8_t *)app.state.rgbCur, app.state.rglpshdef[i], iplrSav, app.state.hdrCur.cb))
                    goto Corrupt;
                RecordIO().pfnReadRt(app);
            }
            app.state.idPlayer = iplrSav;
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            app.state.rgplr[i].cShDef = 0;
            if (app.state.rglpshdef[i]) {
                for (j = 0; j < 16; j++) {
                    if (!app.state.rglpshdef[i][j].fFree) {
                        if (i != app.state.idPlayer && !app.state.gd.fGeneratingTurn && app.state.rgplr[i].fDead) {
                            app.state.rglpshdef[i][j].fFree = 1;
                        } else {
                            app.state.rgplr[i].cShDef++;
                        }
                    }
                }
            }
        }
        app.state.rglpfl = LpAlloc<FLEET *>(app, (1 <= app.state.cFleet ? app.state.cFleet : 1) * sizeof(FLEET *), htMisc);
        for (i = 0; i < app.state.cFleet; i++) {
            lpfl = app.state.rglpfl[i] = LpAlloc<FLEET>(app, sizeof(FLEET), htFleets);
            if (!FReadFleet(app, lpfl))
                goto LError;
            app.state.rgplr[lpfl->iPlayer].cFleet = app.state.rgplr[lpfl->iPlayer].cFleet + 1;
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (app.state.rgplr[i].fInclude) {
                if (!app.state.rglpshdefSB[i]) {
                    app.state.rglpshdefSB[i] = LpAlloc<SHDEF>(app, 10 * sizeof(SHDEF), htShips);
                    for (j = 0; j < 10; j++) {
                        app.state.rglpshdefSB[i][j].fFree = 1;
                        app.state.rglpshdefSB[i][j].grbitPlr = 0;
                    }
                }
                iplrSav = app.state.idPlayer;
                if (app.state.idPlayer == iplrNone) {
                    app.state.idPlayer = i;
                } else if (i != app.state.idPlayer) {
                    app.state.idPlayer = iplrNone;
                }
                for (j = 0; j < (int16_t)app.state.rgplr[i].cshdefSB; j++) {
                    if (app.state.hdrCur.rt != rtShDef) {
                        app.state.idPlayer = iplrSav;
                        goto Corrupt;
                    }
                    if (!FReadShDef(app, (const uint8_t *)app.state.rgbCur, app.state.rglpshdefSB[i], iplrSav, app.state.hdrCur.cb))
                        goto Corrupt;
                    RecordIO().pfnReadRt(app);
                }
                app.state.idPlayer = iplrSav;
            }
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            app.state.rgplr[i].cshdefSB = 0;
            if (app.state.rglpshdefSB[i]) {
                for (j = 0; j < 10; j++) {
                    if (!app.state.rglpshdefSB[i][j].fFree) {
                        if (i != app.state.idPlayer && !app.state.gd.fGeneratingTurn && app.state.rgplr[i].fDead) {
                            app.state.rglpshdefSB[i][j].fFree = 1;
                        } else {
                            app.state.rgplr[i].cshdefSB++;
                        }
                    }
                }
            }
        }
        if (!app.state.vlprgScoreX) {
            app.state.vlprgScoreX = LpAlloc<SCOREX>(app, app.state.game.cPlayer * sizeof(SCOREX), htMisc);
            memset(app.state.vlprgScoreX, 0, app.state.game.cPlayer * sizeof(SCOREX));
        }
        while (app.state.hdrCur.rt == rtScore) {
            iplr = static_cast<uint8_t>(app.state.rgbCur[0]) & 31;
            FDecodeScoreX(std::as_bytes(std::span(app.state.rgbCur)), app.state.vlprgScoreX[iplr]);
            if (!app.state.rgsxPlr[iplr]) {
                app.state.rgsxPlr[iplr] = LpAlloc<SCOREX>(app, 101 * sizeof(SCOREX), htMisc);
                app.state.rgcsxPlr[iplr] = 0;
            }
            if (app.state.rgsxPlr[iplr]) {
                if (app.state.vlprgScoreX[iplr].fHistory) {
                    turnCur = app.state.vlprgScoreX[iplr].turn;
                } else {
                    turnCur = app.state.game.turn;
                }
                for (isx = 0; isx < app.state.rgcsxPlr[iplr] && turnCur > app.state.rgsxPlr[iplr][isx].turn; isx++) {
                }
                if ((isx >= app.state.rgcsxPlr[iplr] || turnCur == app.state.rgsxPlr[iplr][isx].turn) && isx < 101) {
                    if (isx == app.state.rgcsxPlr[iplr]) {
                        app.state.rgcsxPlr[iplr]++;
                    }
                } else if (app.state.rgcsxPlr[iplr] < 101) {
                    memmove(app.state.rgsxPlr[iplr] + (isx + 1), app.state.rgsxPlr[iplr] + isx, (app.state.rgcsxPlr[iplr] - isx) * sizeof(SCOREX));
                    app.state.rgcsxPlr[iplr]++;
                } else if (isx > 0) {
                    if (isx > 1) {
                        memmove(app.state.rgsxPlr[iplr], app.state.rgsxPlr[iplr] + 1, (isx - 1) * sizeof(SCOREX));
                    }
                    isx--;
                }
                app.state.rgsxPlr[iplr][isx] = app.state.vlprgScoreX[iplr];
                app.state.rgsxPlr[iplr][isx].turn = turnCur;
                app.state.rgsxPlr[iplr][isx].fHistory = 1;
            }
            RecordIO().pfnReadRt(app);
        }
        if (app.state.lpThings) {
            HeapIO().pfnFree(app, app.state.lpThings, htThings);
            app.state.lpThings = nullptr;
            app.state.cThing = 0;
        }
        if (app.state.hdrCur.rt == rtThing) {
            fHist = app.state.cThing > 0;
            cThingFile = std::bit_cast<int16_t>(WFromStreamBytes(app.state.rgbCur));
            app.state.cThingAlloc = 10 <= cThingFile ? cThingFile : 10;
            if (!app.state.lpThings) {
                app.state.lpThings = LpAlloc<THING>(app, app.state.cThingAlloc * sizeof(THING), htThings);
                if (!app.state.lpThings)
                    goto LError;
                memset(app.state.lpThings, 0, app.state.cThingAlloc * sizeof(THING));
            }
            RecordIO().pfnReadRt(app);
            lpth = app.state.lpThings;
            j = 0;
            for (i = 0; i < cThingFile; i++) {
                fHaveHistoryData = 0;
                if (fHist) {
                    for (; j < app.state.cThing && WFromStreamBytes(app.state.rgbCur) > lpth->idFull(); lpth++) {
                        j++;
                    }
                    if (j < app.state.cThing && WFromStreamBytes(app.state.rgbCur) == lpth->idFull()) {
                        fHaveHistoryData = 1;
                        goto LFoundThing;
                    }
                    if (app.state.cThingAlloc == app.state.cThing) {
                        app.state.cThingAlloc += 8;
                        app.state.lpThings = LpReAlloc<THING>(app, app.state.lpThings, app.state.cThingAlloc * sizeof(THING), htThings);
                        lpth = app.state.lpThings + j;
                    }
                    if (j < app.state.cThing) {
                        memmove(lpth + 1, lpth, (app.state.cThing - j) * sizeof(THING));
                        memset(lpth, 0, sizeof(THING));
                    }
                }
                app.state.cThing++;
            LFoundThing:
                CopyThingBytes(app, lpth, app.state.rgbCur, app.state.hdrCur.cb);
                lpth->SetTurn(app.state.game.turn);
                lpth++;
                j++;
                RecordIO().pfnReadRt(app);
            }
        } else {
            app.state.cThing = 0;
            app.state.cThingAlloc = 10;
            app.state.lpThings = LpAlloc<THING>(app, app.state.cThingAlloc * sizeof(THING), htThings);
        }
        if (app.state.hdrCur.rt == rtSel) {
            RecordIO().pfnReadRt(app);
        }
        iplrSav = app.state.idPlayer;
        while (app.state.hdrCur.rt == rtBtlPlan) {
            iP = NATIVEBATTLEPLANVIEW(std::as_writable_bytes(std::span(app.state.rgbCur)), 0).iplr();
            app.state.idPlayer = iP;
            if (!app.state.rglpbtlplan[iP]) {
                app.state.rglpbtlplan[iP] = LpAlloc<BTLPLAN>(app, 16 * sizeof(BTLPLAN), htShips);
            }
            if (!UnpackBattlePlan(app, (uint8_t *)app.state.rgbCur, app.state.rglpbtlplan[iP] + app.state.rgcbtlplan[iP], app.state.rgcbtlplan[iP], app.state.hdrCur.cb)) goto Corrupt;
            app.state.rgcbtlplan[iP]++;
            RecordIO().pfnReadRt(app);
        }
        app.state.idPlayer = iplrSav;
        if (app.state.hdrCur.rt != rtEOF) {
        Corrupt:
            AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsGameFileAppearsCorruptUnableLoadFile, nullptr), win32::MB_ICONHAND);
            goto LError;
        }
        if (CbFileSize(app.state.hf) != LSeekFile(app.state.hf, 0, 1)) {
            RecordIO().pfnReadRt(app);
            if (app.state.hdrCur.rt == rtBOF) {
                app.state.game.turn = BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).turn();
                app.state.game.wGen = BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).wGen();
                for (i = 0; i < app.state.game.cPlayer; i++) {
                    app.state.rgplr[i].cShDef = 0;
                    app.state.rgplr[i].cFleet = 0;
                    app.state.rgplr[i].cPlanet = 0;
                    app.state.rgplr[i].cshdefSB = 0;
                    app.state.rgcbtlplan[i] = 0;
                }
                lppl = app.state.lpPlanets;
                lpplMac = app.state.lpPlanets + app.state.cPlanet;
                for (; lppl < lpplMac; lppl++) {
                    if (lppl->iPlayer == iPlayer) {
                        lppl->iPlayer = iplrNone;
                        lppl->det = detSome;
                        if (lppl->lpplprod) {
                            FreePl(app, lppl->lpplprod);
                            lppl->lpplprod = nullptr;
                        }
                    }
                }
                cPlanetHist = app.state.cPlanet;
                goto LNextTurn;
            }
            AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsWarningIgnoringUnexpectedDataAfterEof, nullptr), win32::MB_ICONHAND);
        }
        RecordIO().pfnStreamClose(app);
        if (cturn > 1 && !app.state.rgplr[iPlayer].fAi && !app.state.ini.fDumpPlanets && !app.state.ini.fDumpFleets && !app.state.ini.fDumpMap) {
            CchSprintf(app.state.szWork, TextIO().pfnCompressed(app, idsNoteDYearsDataRead), cturn);
            AlertIO().pfnAlertSz(app, app.state.szWork, win32::MB_ICONASTERISK);
        }
        if (FSzPrefixNoCase(pszExt, "hst", 3))
            goto DoneNow;
        if (!app.state.rgplr[iPlayer].fAi) {
            lpth = app.state.lpThings;
            lpthMac = app.state.lpThings + app.state.cThing;
            for (; lpth < lpthMac; lpth++) {
                if (lpth->ith() == ithMineralPacket && lpth->thp().iWarp() != 0) {
                    lppl = LpplFromId(app, lpth->thp().idPlanet());
                    if (lppl && lppl->iPlayer == iPlayer) {
                        iWarp = ColonyIO().pfnWarp(app, lppl, &fTwo);
                        if (iWarp + fTwo < lpth->thp().iWarp() + 4) {
                            FSendPlrMsg2XGen(app, 0, idmMassPacketAppearsCollisionCourseWhichCurrently, gotoThing, lpth->idFull(), lppl->id);
                        }
                    }
                }
            }
            if (!app.state.game.fTutorial) {
                lppl = app.state.lpPlanets;
                lpplMac = app.state.lpPlanets + app.state.cPlanet;
                for (; lppl < lpplMac; lppl++) {
                    if (lppl->iPlayer == iPlayer && lppl->fStarbase && lppl->lpplprod && app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].hul.ihuldef != ihuldefOrbitalFort) {
                        fWorking = 0;
                        iprod = 0;
                        lpprod = lppl->lpplprod->rgprod();
                        while (iprod < lppl->lpplprod->iprodMac) {
                            ResearchIO().pfnEstimate(app, lppl, nullptr, static_cast<ProdItemType>(iprod), &iFirst, &iLast);
                            if (iLast > 1) {
                                fWorking = 0;
                                break;
                            }
                            if (iLast == 1 && (lpprod->grobj != grobjPlanet || lpprod->iItem >= mdIdleFactory)) {
                                fWorking = 1;
                            }
                            iprod++;
                            lpprod++;
                        }
                        if (fWorking) {
                            FSendPlrMsg2XGen(app, 0, idmStarbaseScheduledCompleteRemainingProductionItem, static_cast<MsgGoto>(lppl->id), lppl->id, 0);
                        }
                    }
                }
                i = CBattles(app);
                if (i > 0) {
                    FSendPlrMsg2XGen(app, 1, static_cast<MessageId>(idmHaveReceivedOneBattleRecordingYear + (i > 1)), gotoBattleReport, i, 0);
                }
            }
        }
        if (!app.state.gd.fDontDoLogFiles) {
            CchSprintf(app.state.szWork, "%s.x%s", pszFileName, pszExt + 1);
            if (!FLoadLogFile(app, app.state.szWork) || !FRunLogFile(app)) {
                AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsPlayerLogFileAppearsCorruptUnableLoad, nullptr), win32::MB_ICONHAND);
                goto LError;
            }
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            app.state.rgplr[i].cFleet = 0;
            app.state.rgplr[i].cPlanet = 0;
        }
        lppl = app.state.lpPlanets;
        lpplMac = app.state.lpPlanets + app.state.cPlanet;
        for (; lppl < lpplMac; lppl++) {
            if (lppl->iPlayer != iplrNone) {
                app.state.rgplr[lppl->iPlayer].cPlanet = app.state.rgplr[lppl->iPlayer].cPlanet + 1;
            } else {
                lppl->fStarbase = 0;
            }
        }
        j = 0;
        for (i = 0; i < app.state.cFleet; i++) {
            lpfl = app.state.rglpfl[i];
            if (!app.state.rglpfl[i])
                break;
            j = lpfl->iPlayer;
            app.state.rgplr[j].cFleet++;
        }
DoneNow:
        app.state.idPlayer = iPlayer;
        if (app.state.idPlayer != iplrNone && !app.state.rgplr[app.state.idPlayer].fAi) {
            AddMRUFile(app, pszFileName, pszExt);
        }
        return 1;
    } catch (const StarsError &) {
    }
LError:
    app.state.game.fDirty = 0;
    DestroyCurGame(app);
    RecordIO().pfnStreamClose(app);
    if (!app.state.ini.fValidate && !app.state.ini.fLogging) {
        ShowTitleScreen(app);
    }
    return 0;

}

int16_t FReadPlanet(APPSESSION &app, int16_t iPlayer, PLANET *lppl, int16_t fHistory, int16_t fPreInited) {
    int16_t   fFirstYear;
    int16_t   fRouting;
    uint8_t   bMask;
    int16_t   i;
    uint8_t  *pb;
    MessageId idm;
    int16_t   pctOpt;
    int16_t   pct;

    fFirstYear = 0;
    if (!fPreInited) {
        memset(lppl, 0, sizeof(PLANET));
    }
    if (fHistory || iPlayer == iplrNone) {
        lppl->fFirstYear = PpvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fFirstYear();
    } else if (!fPreInited) {
        fFirstYear = 1;
        lppl->fFirstYear = 1;
    } else if (lppl->fFirstYear) {
        if (lppl->turn != app.state.game.turn) {
            lppl->fFirstYear = 0;
        } else {
            fFirstYear = 1;
        }
    }
    lppl->id = PpvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).id();
    lppl->iPlayer = PpvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).iPlayer();
    if (lppl->det < PpvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).det()) {
        lppl->det = PpvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).det();
    }
    lppl->fInclude = PpvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fInclude();
    lppl->fStarbase = PpvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fStarbase();
    lppl->fHomeworld = PpvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fHomeworld();
    fRouting = PpvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fRouting();
    if (lppl->fStarbase && lppl->iPlayer == iplrNone) {
        lppl->fStarbase = 0;
    }
    if (!fHistory) {
        lppl->turn = app.state.game.turn;
    }
    pb = (uint8_t *)app.state.rgbCur + cbWirePlanet;
    if (PpvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).det() >= detSome) {
        bMask = *pb;
        pb++;
        i = 0;
        while (i < 3) {
            if (bMask & 3) {
                if ((bMask & 3) != 1) {
                    return 0;
                }
                lppl->rgpctMinLevel[i] = *pb++;
            } else {
                lppl->rgpctMinLevel[i] = 0;
            }
            i++;
            bMask >>= 2;
        }
        i = 0;
        while (i < 3) {
            lppl->rgMinConc[i] = *pb;
            i++;
            pb++;
        }
        for (i = 0; i < 3; i++) {
            if (*pb > 100) {
                return 0;
            }
            lppl->rgEnvVar[i] = lppl->rgEnvVarOrig[i] = *pb++;
        }
        if (PpvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fIncEVO() != 0) {
            for (i = 0; i < 3; i++) {
                if (*pb > 100) {
                    return 0;
                }
                lppl->rgEnvVarOrig[i] = *pb++;
            }
        }
        if (PpvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).iPlayer() != iplrNone) {
            lppl->SetUGuesses(WFromStreamBytes(pb));
            pb += 2;
        }
        if (lppl->det <= detSome)
            goto LFinishBRecord;
        if (PpvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fIncSurfMin() != 0) {
            bMask = *pb;
            pb++;
            i = 0;
            while (i < 4) {
                switch (bMask & 3) {
                default:
                    break;
                case 0:
                    lppl->rgwtMin[i] = 0;
                    break;
                case 1:
                    lppl->rgwtMin[i] = (uint32_t)*pb++;
                    break;
                case 2:
                    lppl->rgwtMin[i] = (uint32_t)WFromStreamBytes(pb);
                    pb += 2;
                    break;
                case 3:
                    lppl->rgwtMin[i] = LFromStreamBytes(pb);
                    pb += 4;
                }
                i++;
                bMask >>= 2;
            }
        }
        if (app.state.hdrCur.rt == rtPlanetB)
            goto LFinishBRecord;
        if (PpvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fIncImp() != 0) {
            std::array<uint8_t, 8> rgbImp;
            memmove(rgbImp.data(), pb, 8);
            lppl->SetRgbImp(rgbImp);
            pb += 8;
        } else {
            lppl->fArtifact = PpvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fIsArtifact();
            lppl->iScanner = 31;
            lppl->cDefenses = 0;
        }
        if (lppl->iPlayer != iplrNone) {
            if (lppl->fStarbase) {
                lppl->SetLStarbase(LFromStreamBytes(pb));
                lppl->fNoHeal = 0;
                pb += 4;
            }
            if (fRouting) {
                lppl->SetWRouting(WFromStreamBytes(pb));
            }
        }
        return 1;
    }
LFinishBRecord:
    if (lppl->fStarbase) {
        lppl->isb = *pb;
        pb++;
    }
    if (fHistory) {
        lppl->turn = WFromStreamBytes(pb);
        pb += 2;
    } else if (fFirstYear) {
        if (lppl->iPlayer != iplrNone) {
            FSendPlrMsg2XGen(app, 0, idmHaveFoundPlanetOccupiedSomeoneElseCurrently, static_cast<MsgGoto>(lppl->id), lppl->id, lppl->iPlayer | 0x30);
        } else if (lppl->det <= detMinimal) {
            FSendPlrMsg2XGen(app, 0, idmHaveFoundNewPlanetDontKnowIf, static_cast<MsgGoto>(lppl->id), lppl->id, 0);
        } else if (RaceQueryIO().pfnStat(&app.state.rgplr[iPlayer], rsMajorAdv) == raTerra) {
            pctOpt = PctPlanetOptValue(app, lppl, iPlayer);
            FSendPlrMsg2XGen(app, 0, idmHaveInfoNewPlanetIfColonizeCan, static_cast<MsgGoto>(lppl->id), lppl->id, pctOpt);
        } else {
            pct = PlanetQueryIO().pfnDesirability(app, lppl, iPlayer);
            if (pct > 0) {
                pct = PctTrueMaxGrowth(app, iPlayer) * pct;
                idm = idmHaveFoundNewHabitablePlanetColonistsWill;
            } else {
                pctOpt = PctPlanetOptValue(app, lppl, iPlayer);
                if (pctOpt > 0) {
                    pct = PctTrueMaxGrowth(app, iPlayer) * pctOpt;
                    idm = idmHaveFoundNewPlanetWhichHaveAbility;
                } else {
                    pct = 10 * pct;
                    idm = idmHaveFoundNewPlanetWhichUnfortunatelyHabitable;
                }
            }
            FSendPlrMsg2XGen(app, 0, idm, static_cast<MsgGoto>(lppl->id), abs(pct), lppl->id);
        }
    }
    return 1;
}

int16_t FReadFleet(APPSESSION &app, FLEET *lpfl) {
    int16_t   cord;
    ORDER    *lpord;
    int16_t   i = 0;
    char      szT[33];

    memset(lpfl, 0, sizeof(FLEET));
    if (!FDecodeRtFleet(std::as_bytes(std::span(app.state.rgbCur)), *lpfl, app.state.hdrCur.rt == rtFleetA, i)) goto Corrupt;
    if (lpfl->det < detAll) {
        RecordIO().pfnReadRt(app);
        return 1;
    }
    lpfl->lpplord = LpplAlloc<PLORD>(app, 18, lpfl->cord + 1, htOrd);
    memset(lpfl->lpplord->rgord(), 0, (lpfl->cord + 1) * 18);
    cord = lpfl->cord;
    lpord = lpfl->lpplord->rgord();
    for (; cord != 0; cord--) {
        memset(app.state.rgbCur, 0, 18);
        RecordIO().pfnReadRt(app);
        if (app.state.hdrCur.rt != rtOrderA && app.state.hdrCur.rt != rtOrderB)
            goto Corrupt;
        // The reader historically consumes the zeroed reusable prefix even
        // when the declared order is shorter; keep that admission behavior.
        FDecodeOrder(std::as_bytes(std::span(app.state.rgbCur)), *lpord);
        lpord->fNoAutoTrack = 0;
        lpord++;
    }
    lpfl->lpplord->iordMac = lpfl->cord;
    if (lpfl->idPlanet != idPlanetDeepSpace) {
        if (lpfl->idPlanet > app.state.game.cPlanMax) {
            lpfl->idPlanet = idPlanetDeepSpace;
        }
        if (lpfl->pt.x != app.state.rgptPlan[lpfl->idPlanet].x || lpfl->pt.y != app.state.rgptPlan[lpfl->idPlanet].y) {
            if (i != 0 || app.state.game.turn != 0)
                goto Corrupt;
            lpfl->pt = app.state.rgptPlan[lpfl->idPlanet];
        }
    }
    RecordIO().pfnReadRt(app);
    if (app.state.hdrCur.rt == rtString) {
        if (!FReadUserString(app, (uint8_t *)app.state.rgbCur, app.state.hdrCur.cb, szT, 32, nullptr)) goto Corrupt;
        lpfl->lpszName = LpAlloc<char>(app, strlen(szT) + 1, htString);
        strcpy(lpfl->lpszName, szT);
        RecordIO().pfnReadRt(app);
    } else {
        lpfl->lpszName = nullptr;
    }
    return 1;
Corrupt:
    AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsGameFileAppearsCorruptUnableLoadFile, nullptr), win32::MB_ICONHAND);
    return 0;
}

int16_t UnpackBattlePlan(APPSESSION &app, uint8_t *lpb, BTLPLAN *lpbtlplan, int16_t iplan, size_t cb) {
    char szName[32];

    if (cb < 4) goto invalid;
    if (!FReadUserString(app, lpb + 4, cb - 4, szName, sizeof(szName), nullptr)) goto invalid;
    FDecodeBattlePlanPrefix(std::as_bytes(std::span(lpb, cbWireBattlePlan)), *lpbtlplan);
    strcpy(lpbtlplan->szName, szName);
    lpbtlplan->iplan = iplan;
    return 1;
invalid:
    FileError(app, idsGameFileAppearsCorruptUnableLoadFile);
    if (FStarsRecoveryActive()) StarsThrowError(starsErrorFile, app.state.idsFileError);
    return 0;
}

void UpdateBattleRecords(APPSESSION &app) {
    uint8_t *lpbd;
    uint8_t *lpbr;
    int16_t   cKill;
    HB       *lphb;
    BTLREC26 br26{};
    int16_t   itok;

    lphb = app.state.rglphb[11];
    if (lphb) {
        lpbd = (uint8_t *)((uint8_t *)lphb + (sizeof(HB) + 2));
        while (1) {
            if (BdvFromLp(app, lpbd).id() == 0xffff) {
                lphb = lphb->lphbNext;
                if (!lphb || lphb->ibTop <= sizeof(HB))
                    break;
                lpbd = (uint8_t *)((uint8_t *)lphb + (sizeof(HB) + 2));
            } else {
                if (BdvFromLp(app, lpbd).cbData() == 0)
                    break;
                lpbr = BdvFromLp(app, lpbd).FirstEvent();
                lpbd = (uint8_t *)((uint8_t *)lpbd + BdvFromLp(app, lpbd).cbData());
                while (lpbr < lpbd) {
                    FDecodeBattleRecord26(std::as_bytes(BevFromLp(app, lpbr).Rgb()), br26);
                    cKill = br26.ctok;
                    itok = br26.itokAttack;
                    BevFromLp(app, lpbr).SetCtok(cKill);
                    BevFromLp(app, lpbr).SetItokAttack(itok);
                    lpbr = BevFromLp(app, lpbr).Next();
                }
            }
        }
    }
    return;
}

void DestroyCurGame(APPSESSION &app) {
    int16_t i;

    if (app.state.gd.fSendMsgMode) {
        FFinishPlrMsgEntry(app, 0);
    }
    if (app.state.idPlayer != iplrNone && app.state.game.fDirty) {
        PromptSaveGame(app);
    }
    ResetHb(app, htPlanets);
    app.state.lpPlanets = nullptr;
    app.state.cPlanet = 0;
    ResetHb(app, htFleets);
    app.state.rglpfl = nullptr;
    app.state.cFleet = 0;
    ResetHb(app, htThings);
    app.state.lpThings = nullptr;
    app.state.cThing = 0;
    app.state.cThingAlloc = 0;
    app.state.vlprgScoreX = nullptr;
    app.state.vrptFleet.fCached = 0;
    app.state.vrptPlanet.fCached = 0;
    app.state.vrptBattle.fCached = 0;
    app.state.vrptEFleet.fCached = 0;
    for (i = 0; i < 16; i++) {
        app.state.rgsxPlr[i] = nullptr;
    }
    app.state.lpbBattleT = nullptr;
    app.state.lpbBattleLog = nullptr;
    app.state.lpbBattleCur = nullptr;
    app.state.gd.fAisDone = 0;
    app.state.gd.fGotoVCR = 0;
    app.state.gd.fFleetLinkValid = 0;
    ResetHb(app, htBattle);
    if (app.state.rglphb[11]) {
        app.state.rglphb[11][1].cbBlock = 0xffff;
    }
    ResetHb(app, htMisc);
    ResetHb(app, htString);
    ResetHb(app, htShips);
    ResetHb(app, htOrd);
    ResetHb(app, htPlrMsg);
    for (i = 0; i < 16; i++) {
        app.state.rglpshdef[i] = nullptr;
        app.state.rglpshdefSB[i] = nullptr;
        app.state.rglpbtlplan[i] = nullptr;
        app.state.rgcbtlplan[i] = 0;
    }
    if (app.state.sel.grobj != grobjNone) {
        app.state.ini.grobjSel = app.state.sel.grobj;
        app.state.ini.iObjSel = app.state.sel.id;
        app.state.ini.idPlayer = app.state.idPlayer;
        app.state.ini.lid = app.state.game.lid;
    }
    app.state.idPlayer = iplrNone;
    app.state.imemLogCur = 0;
    app.state.imemLogPrev = -1;
    app.state.iMsgCur = 0;
    app.state.vlpbAiData = nullptr;
    ResetMessages(app);
    app.state.lSaltCur = 0;
    app.state.ctickLast = 0;
    app.state.game.lid = 0;
    app.state.game.cPlayer = 0;
    app.state.game.cPlanMax = 0;
    app.state.game.fDirty = 0;
    app.state.game.turn = 0;
    app.state.game.szName[0] = 0;
    app.state.gd.fGameOverMan = 0;
    app.state.gd.fSendMsgMode = 0;
    CloseGameWindows(app);
    app.state.sel.scan.grobjFull = grobjNone;
    app.state.sel.scan.grobj = grobjNone;
    app.state.sel.scan.iwp = iwpNone;
    app.state.sel.scan.ifl = iflNone;
    app.state.sel.scan.idpl = idplNone;
    app.state.sel.grobjFull = grobjNone;
    app.state.sel.grobj = grobjNone;
    app.state.sel.id = -1;
    app.state.sel.pt.y = 0;
    app.state.sel.pt.x = 0;
    app.state.sel.pl.id = idplNone;
    app.state.sel.fl.id = idflNone;
    app.state.sel.fl.lpplord = nullptr;
    app.state.sel.pl.lpplprod = nullptr;
    return;
}

void FileError(APPSESSION &app, StringId ids) {
    app.state.idsFileError = ids;
    if (!app.state.fFileErrSilent && !app.state.gd.fGeneratingTurn) {
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, ids, nullptr), win32::MB_ICONHAND);
    }
    return;
}

void GetFileStatus(APPSESSION &app, int16_t dt, int16_t iPlayer) {
    SetSzWorkFromDt(app, static_cast<DtFileType>(dt), iPlayer);
    app.state.gd.fReadOnly = FFileReadOnly(app.state.szWork);
    return;
}

int16_t FOpenFile(APPSESSION &app, DtFileType dt, int16_t iPlayer, int16_t md) {
    RTBOF    rtbof;
    StringId ids;
    int16_t  fCheckMulti;
    int16_t  fRewind;
    int16_t  fSilentSav;

    fSilentSav = app.state.fFileErrSilent;
    ids = idsCantOpenFile;
    app.state.gd.fPartialTurn = 0;
    fCheckMulti = dt & 0x2000;
    fRewind = dt & 0x1000;
    dt = static_cast<DtFileType>(dt & 0xff);
    SetSzWorkFromDt(app, dt, iPlayer);
    StarsRecoveryScope scopeRecovery;
    try {
        app.state.fFileErrSilent = 1;
        RecordIO().pfnStreamOpen(app, app.state.szWork, static_cast<MdOpenFlags>(md));
        app.state.fFileErrSilent = fSilentSav;
        ids = idsGameFileAppearsCorruptUnableLoadFile;
        RecordIO().pfnReadRt(app);
        if (app.state.hdrCur.rt != rtBOF) {
            FileError(app, idsGameFileAppearsCorruptUnableLoadFile);
            goto LBadFile;
        }
        if (BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMajor() < 2 || (BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMajor() == 2 && BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMinor() < 85)) {
            FileError(app, idsSorryFileCreatedOlderVersionStarsIncompatible);
            goto LBadFile;
        }
        if (BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMajor() > 2 || (BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMajor() == 2 && BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMinor() > 85)) {
            FileError(app, idsFileCreatedNewerVersionStarsMustUpgrade);
            goto LBadFile;
        }
        FDecodeBof(std::as_bytes(std::span(app.state.rgbCur)), rtbof);
        if (rtbof.iPlayer != iPlayer) {
            FileError(app, idsGameFileAppearsCorruptUnableLoadFile);
            goto LBadFile;
        }
        if (app.state.game.lid != 0) {
            if (rtbof.lidGame != app.state.game.lid) {
                FileError(app, idsFileGame);
                goto LBadFile;
            }
            if (dt != dtHist) {
                if (fCheckMulti && rtbof.fMulti) {
                    LSeekFile(app.state.hf, -4, 2);
                    RecordIO().pfnReadRt(app);
                    if (app.state.hdrCur.rt != rtEOF && app.state.hdrCur.cb != 2)
                        goto LBadFile;
                    rtbof.turn = WFromStreamBytes(app.state.rgbCur);
                    app.state.game.wGen = rtbof.wGen;
                }
                if (app.state.game.turn == 0 && app.state.game.turn != rtbof.turn) {
                    app.state.game.turn = rtbof.turn;
                    app.state.game.wGen = rtbof.wGen;
                } else {
                    if (rtbof.turn != app.state.game.turn) {
                        FileError(app, idsFileDate);
                        goto LBadFile;
                    }
                    if (dt == dtHost && !app.state.gd.fHostMode && rtbof.fInUse) {
                        if (AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsHostFileMarkedUseAnotherInstanceStars, nullptr), win32::MB_YESNO | win32::MB_ICONQUESTION | win32::MB_TASKMODAL) != win32::IDYES)
                            goto LBadFile;
                    } else {
                        if (!rtbof.fDone && app.state.gd.fGeneratingTurn && !app.state.gd.fForceTurn) {
                            app.state.gd.fPartialTurn = 1;
                            goto LBadFile;
                        }
                        if (dt == dtLog && !app.state.game.fTutorial && rtbof.wGen != app.state.game.wGen) {
                            FileError(app, idsFileGame);
                            goto LBadFile;
                        }
                    }
                }
            } else if (rtbof.iPlayer != iPlayer) {
                goto LBadFile;
            }
        }
        if (fRewind) {
            LSeekFile(app.state.hf, 0, 0);
            RecordIO().pfnReadRt(app);
        }
        app.state.wVersFile = rtbof.wVersion();
        app.state.gd.fFileCrippled = rtbof.fCrippled;
        return 1;
LBadFile:
        RecordIO().pfnStreamClose(app);
        return 0;
    } catch (const StarsError &) {
        app.state.fFileErrSilent = fSilentSav;
        FileError(app, ids);
        RecordIO().pfnStreamClose(app);
        return 0;
    }
}

int16_t FNewTurnAvail(APPSESSION &app, int16_t idPlayer) {
    uint16_t wGenOld;
    uint16_t turnOld;
    int16_t  fNew;

    turnOld = app.state.game.turn;
    wGenOld = app.state.game.wGen;
    app.state.fFileErrSilent = 1;
    app.state.game.turn = 0;
    fNew = FOpenFile(app, static_cast<DtFileType>(8195), idPlayer, 32);
    if (fNew) {
        RecordIO().pfnStreamClose(app);
        fNew = app.state.game.turn > turnOld;
    }
    app.state.game.turn = turnOld;
    app.state.game.wGen = wGenOld;
    return fNew;
}

int16_t FCheckFile(APPSESSION &app, DtFileType dt, int16_t iPlayer, MdMark md) {
    int16_t  fReturn;
    int16_t  fOpened;
    uint16_t wGenOld;
    int16_t  f;
    int16_t  fErrSav;

    fErrSav = app.state.fFileErrSilent;
    wGenOld = app.state.game.wGen;
    if (dt == dtHost) {
        f = app.state.gd.fHostMode;
        app.state.gd.fHostMode = 1;
    }
    app.state.fFileErrSilent = 1;
    fOpened = FOpenFile(app, dt, iPlayer, 32);
    switch (md) {
    case mdMarkInUse:
        if (!fOpened || BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fInUse()) {
            fReturn = 1;
            break;
        }
        fReturn = 0;
        break;
    case mdMarkDone:
        if (fOpened && BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fDone()) {
            fReturn = 1;
            break;
        }
        fReturn = 0;
        break;
    case mdMarkMulti:
        if (fOpened && BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fMulti()) {
            fReturn = 1;
            break;
        }
        fReturn = 0;
        break;
    case mdMarkAi:
        if (!fOpened) {
            fReturn = 0;
        } else {
            do {
                RecordIO().pfnReadRt(app);
            } while (app.state.hdrCur.rt != rtEOF && (app.state.hdrCur.rt != rtPlr || PlvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).iPlayer() != iPlayer));
            fReturn = app.state.hdrCur.rt == rtPlr && PlvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fAi();
        }
    }
    if (fOpened) {
        RecordIO().pfnStreamClose(app);
    }
    if (dt == dtHost) {
        app.state.gd.fHostMode = f;
    }
    app.state.fFileErrSilent = fErrSav;
    app.state.game.wGen = wGenOld;
    return fReturn;
}

// Publish even a partial header before FileError calls the UI. Seeding the
// unread byte retains the old global header on a zero/one-byte read failure.
static void ReadRtHeader(APPSESSION &app) {
    std::byte rgb[cbWireHdr];
    int32_t cb;

    FEncodeHdr(app.state.hdrCur, rgb);
    if (app.state.vlpMemStream) {
        memcpy(rgb, app.state.vlpMemStream, cbWireHdr);
        app.state.vlpMemStream += cbWireHdr;
        cb = cbWireHdr;
    } else {
        cb = CbReadFile(app.state.hf, rgb, cbWireHdr);
    }
    FDecodeHdr(rgb, app.state.hdrCur);
    if (cb != cbWireHdr) {
        FileError(app, idsGameFileAppearsCorruptUnableLoadFile);
        StarsThrowError(starsErrorFile, app.state.idsFileError);
    }
}

void ReadRt(APPSESSION &app) {
    ReadRtHeader(app);
    if (app.state.hdrCur.cb != 0) {
        RgFromStream(app, app.state.rgbCur, app.state.hdrCur.cb);
    }
    if (app.state.hdrCur.rt == rtBOF) {
        if (app.state.hdrCur.cb != cbWireBof) {
            FileError(app, idsGameFileAppearsCorruptUnableLoadFile);
            if (FStarsRecoveryActive()) StarsThrowError(starsErrorFile, app.state.idsFileError);
            return;
        }
        SetFileXorStream(app, BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).lidGame(), BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).lSaltTime(), BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).turn(), BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).iPlayer(),
                         BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).fCrippled());
    } else if (app.state.hdrCur.rt != rtEOF) {
        XorFileBuf(app, app.state.rgbCur, app.state.hdrCur.cb);
    }
    return;
}

int16_t FBadFileError(StringId ids) {
    switch (ids) {
    case idsUniverseDefinitionFileSeemsMissingCorrupt:
    case idsPlayerLogFileAppearsCorruptUnableLoad:
    case idsHistoryFileAppearsCorruptHistoricalDataWill:
    case idsGameFileAppearsCorruptUnableLoadFile:
    case idsErrorWritingFile:
    case idsFileDate:
    case idsFileGame:
        return 1;
    default:
        return 0;
    }
}

void StreamOpen(APPSESSION &app, const char *szFile, MdOpenFlags mdOpen) {
    uint32_t dwTick;
    int16_t  fMissing;
    int16_t  fNoErr;
    uint32_t dwTickCur;

    dwTick = 0;
    fNoErr = (mdOpen & 0x4000) != 0;
    mdOpen = static_cast<MdOpenFlags>(mdOpen & 0xbfff);
Retry:
    app.state.hf = HfOpenFile(szFile, mdOpen, &fMissing);
    if (app.state.hf == -1) {
        if (app.state.gd.fRetryOpens && !fMissing) {
            dwTickCur = DwTickCount();
            if (dwTick == 0) {
                dwTick = dwTickCur + 4000;
            }
            if (dwTickCur < dwTick) {
                dwTickCur += 500;
                while (DwTickCount() < dwTickCur) {
                }
                goto Retry;
            }
        }
        if (!fNoErr) {
            FileError(app, idsCantOpenFile);
        }
        StarsThrowError(starsErrorFile, idsCantOpenFile);
    }
}

void StreamClose(APPSESSION &app) {
    if (app.state.hf != -1) {
        CloseFile(app.state.hf);
        app.state.hf = -1;
    }
    return;
}

void RgFromStream(APPSESSION &app, void *rg, uint16_t cb) {
    if (cb != 0) {
        if (app.state.vlpMemStream) {
            memcpy(rg, app.state.vlpMemStream, cb);
            app.state.vlpMemStream += cb;
        } else if (CbReadFile(app.state.hf, rg, cb) != cb) {
            FileError(app, idsGameFileAppearsCorruptUnableLoadFile);
            StarsThrowError(starsErrorFile, app.state.idsFileError);
        }
    }
    return;
}
