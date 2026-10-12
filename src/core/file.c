#include "common.h"

char mpishdefishTutor[6] = {3, 4, 9, 6, 7, 14};

int16_t FReadShDef(RTSHDEF *lprt, SHDEF *lpshdef, int16_t iplrLoad) {
    char     szTemp[40];
    SHDEF    shdef;
    uint8_t *lpb;
    int16_t  ishdef;
    int16_t  cch;
    int16_t  iFirst;
    int16_t  cOut;
    int16_t  fOkay;
    HUL     *lphulBase;
    uint32_t wt;
    int16_t  c;
    HUL     *lphul;
    PART     part;

    memset(&shdef, 0, sizeof(SHDEF));
    shdef.hul.ihuldef = lprt->ihuldef;
    shdef.wFlags = lprt->wFlags;
    shdef.hul.chs = lprt->chs;
    shdef.hul.ibmp = lprt->ibmp;
    if (shdef.det == detAll) {
        shdef.hul.dp = lprt->dp;
        shdef.turn = lprt->turn;
        shdef.cBuilt = lprt->cBuilt;
        shdef.cExist = lprt->cExist;
        lpb = (uint8_t *)lprt->rghs;
        memmove(shdef.hul.rghs, lpb, lprt->chs * 4);
        lpb += 4 * lprt->chs;
    } else {
        shdef.hul.wtEmpty = lprt->wtEmpty;
        lpb = &lprt->chs;
    }
    iFirst = LphuldefFromId(shdef.hul.ihuldef)->hul.ibmp;
    if (shdef.hul.ibmp < iFirst || shdef.hul.ibmp >= iFirst + 4) {
        shdef.hul.ibmp = (shdef.hul.ibmp & 3) | iFirst;
    }
    cch = *lpb;
    lpb++;
    if (cch == 0) {
        strcpy(shdef.hul.szClass, lpb);
    } else {
        cOut = 32;
        if (cch > 32) {
            return FALSE;
        }
        memmove(szTemp, lpb, cch);
        FDecompressUserString(szTemp, cch, shdef.hul.szClass, &cOut);
    }
    ishdef = shdef.ishdef;
    if (ishdef >= 16) {
        ishdef -= 16;
    }
    if (shdef.det == detAll || lpshdef[ishdef].fFree || lpshdef[ishdef].det < detAll) {
        lpshdef[ishdef] = shdef;
    } else if (shdef.hul.ihuldef != lpshdef[ishdef].hul.ihuldef || shdef.hul.ibmp != lpshdef[ishdef].hul.ibmp) {
        lpshdef[ishdef] = shdef;
    }
    if (idPlayer != iplrNone) {
        UpdateShdefCost(lpshdef + ishdef);
    }
    if (lpshdef[ishdef].det == detAll) {
        lphul = &lpshdef[ishdef].hul;
        lphulBase = &LphuldefFromId(lphul->ihuldef)->hul;
        wt = (uint32_t)lphulBase->wtEmpty;
        for (c = 0; c < lphul->chs; c++) {
            if (lphul->rghs[c].cItem > 0) {
                part.hs = lphul->rghs[c];
                fOkay = FLookupPart(&part);
                if (idPlayer == iplrNone) {
                    fOkay = 0;
                }
                if (!(part.hs.grhst & lphulBase->rghs[c].grhst) || ((fOkay > 1 && !shdef.fGift) || part.hs.cItem > lphulBase->rghs[c].cItem)) {
                    lphul->rghs[c].cItem = 0;
                }
                wt += (uint32_t)(part.pcom->cMass * lphul->rghs[c].cItem);
            }
            if (c == 0 && lphul->rghs[0].cItem == 0 && lphulBase->rghs[0].grhst == hstEngine) {
                lphul->rghs[0].grhst = hstEngine;
                lphul->rghs[0].iItem = iengineQuickJump5;
                lphul->rghs[0].cItem = lphulBase->rghs[0].cItem;
                part.hs = lphul->rghs[0];
                FLookupPart(&part);
                wt += (uint32_t)(part.pcom->cMass * lphul->rghs[0].cItem);
            }
        }
        lphul->wtEmpty = LOWORD(wt);
    }
    return TRUE;
}

void ReadRtPlr(PLAYER *pplr, uint8_t *pbIn) {
    int16_t iOff;
    PLAYER *pplrRaw;
    int16_t cOut;
    char   *psz;

    pplrRaw = (PLAYER *)pbIn;
    memset(pplr, 0, sizeof(PLAYER));
    // A full record stores a relation count where rgmdRelation begins, then
    // that many relations; a partial one stops before idPlanetHome.
    if (pplrRaw->det == detAll) {
        iOff = offsetof(PLAYER, rgmdRelation);
        memmove(pplr, pbIn, iOff);
        // Keep a corrupt count from overrunning rgmdRelation; skip the rest.
        memmove(pplr->rgmdRelation, pbIn + iOff + 1, min(pbIn[iOff], sizeof(pplr->rgmdRelation)));
        iOff += pbIn[iOff] + 1;
    } else {
        iOff = offsetof(PLAYER, idPlanetHome);
        memmove(pplr, pbIn, iOff);
    }
    if (pbIn[iOff] == 0) {
        strncpy(pplr->szName, (char *)(pbIn + (iOff + 1)), sizeof(pplr->szName) - 1);
        iOff += strlen((char *)(pbIn + (iOff + 1))) + 2;
    } else {
        cOut = sizeof(pplr->szName) - 1;
        FDecompressUserString((char *)(pbIn + (iOff + 1)), pbIn[iOff], pplr->szName, &cOut);
        iOff += pbIn[iOff] + 1;
    }
    if (((VERS *)&wVersFile)->verMinor < 55) {
        psz = PszPlayerName(0, isupper(pplr->szName[0]), TRUE, FALSE, 0, pplr);
        strcpy(pplr->szNames, psz);
    } else if (pbIn[iOff] == 0) {
        strncpy(pplr->szNames, (char *)(pbIn + (iOff + 1)), sizeof(pplr->szNames) - 1);
    } else {
        cOut = sizeof(pplr->szNames) - 1;
        FDecompressUserString((char *)(pbIn + (iOff + 1)), pbIn[iOff], pplr->szNames, &cOut);
    }
    pplr->fLearned = FALSE;
    return;
}

int16_t FLoadGame(char *pszFileName, char *pszExt) {
    int16_t  iplrSav;
    int16_t  cPlanetHist;
    STARPACK sp;
    int16_t  cPlanetAlloc;
    int16_t  fHaveHistoryData;
    jmp_buf *penvMemSav;
    int16_t  fSilentSav;
    PLANET  *lppl;
    int16_t  i;
    THING   *lpth;
    FLEET   *lpfl;
    jmp_buf  env;
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
    HB      *lphb; /* NATIVE: recover the Win16 heap-relative offset. */

    grf = 0;
    cturn = 0;
    /* NATIVE: callers often pass szBase itself, and strcpy onto itself is
       undefined. */
    if (pszFileName != szBase) {
        strcpy(szBase, pszFileName);
    }
    gd.fFleetLinkValid = FALSE;
    penvMemSav = penvMem;
    penvMem = &env;
    if (setjmp(env) != 0) {
    LError:
        game.fDirty = FALSE;
        DestroyCurGame();
        StreamClose();
        if (!ini.fValidate && !ini.fLogging) {
            ShowTitleScreen();
        }
        return FALSE;
    }
    if (!FOpenFile(dtXY, iplrNone, 32))
        goto LError;
    ReadRt();
    if (hdrCur.rt != rtGame) {
    XYCorrupt:
        AlertSz(PszFormatIds(idsUniverseDefinitionFileSeemsMissingCorrupt, NULL), MB_ICONHAND);
        goto LError;
    }
    game = *(GAME *)rgbCur;
    game.fDirty = FALSE;
    dGal = 400 * game.mdSize + 400;
    dGalInv = dGal + 2000;
    x = 1000;
    for (i = 0; i < game.cPlanMax; i++) {
        RgFromStream(&sp, 4);
        x += sp.dx;
        rgptPlan[i].x = x;
        rgptPlan[i].y = sp.y;
        rgidPlan[i] = sp.id;
        if (x >= dGal + 1000 || rgptPlan[i].y >= dGal + 1000 || rgidPlan[i] > cPlanetName)
            goto XYCorrupt;
    }
    ReadRt();
    if (hdrCur.rt != rtEOF)
        goto XYCorrupt;
    StreamClose();
    if ((*pszExt == 'h' || *pszExt == 'H') && (pszExt[1] == 's' || pszExt[1] == 'S')) {
        dt = 2;
        iPlayer = iplrNone;
    } else {
        dt = 3;
        grf |= 0x3000;
        iPlayer = atoi(pszExt + 1);
        iPlayer--;
    }
    ResetMessages();
    memset(rgplr, 0, game.cPlayer * 192);
    ResetHb(htShips);
    idPlayer = iPlayer;
    fSilentSav = fFileErrSilent;
    fFileErrSilent = TRUE;
    if (iPlayer != iplrNone && FOpenFile(dtHist, iPlayer, 32)) {
        ReadRt();
        if (hdrCur.rt != rtHistHdr)
            goto CorruptHist;
        cPlanetHist = ((RTHISTHDR *)rgbCur)->cPlanet;
        cPlanetAlloc = cPlanetHist + ((RTHISTHDR *)rgbCur)->cPlanetExtra;
        if (cPlanetAlloc > 1000) {
            cPlanetAlloc = 1000;
        }
        lpPlanets = LpAlloc((1 <= cPlanetAlloc ? cPlanetAlloc : 1) * sizeof(PLANET), htPlanets);
        ReadRt();
        for (i = 0, lppl = lpPlanets; i < cPlanetHist; i++, lppl++) {
            if (hdrCur.rt != rtPlanetB)
                goto CorruptHist;
            if (FReadPlanet(iPlayer, lppl, TRUE, FALSE)) {
                if (lppl->iPlayer == iPlayer) {
                    lppl->iPlayer = iplrNone;
                    lppl->det = detSome;
                }
            } else {
            CorruptHist:
                StreamClose();
                AlertSz(PszFormatIds(idsHistoryFileAppearsCorruptHistoricalDataWill, NULL), MB_ICONHAND);
                goto LNoHistFile;
            }
            ReadRt();
        }
        if (hdrCur.rt == rtMsgFilt) {
            if (hdrCur.cb > (uint16_t)cbbitfMsg)
                goto CorruptHist;
            memcpy(bitfMsgFiltered, rgbCur, hdrCur.cb);
            ReadRt();
        }
        while (hdrCur.rt == rtPlr) {
            i = ((PLAYER *)rgbCur)->iPlayer;
            ReadRtPlr(&rgplr[i], rgbCur);
            rgplr[i].cPlanet = 0;
            rgplr[i].cFleet = 0;
            ReadRt();
        }
        i = 0;
        while (hdrCur.rt == rtShDef) {
            for (; rgplr[i].cShDef == 0 && i < game.cPlayer; i++) {
            }
            if (i == game.cPlayer)
                break;
            if (!rglpshdef[i]) {
                rglpshdef[i] = LpAlloc(16 * sizeof(SHDEF), htShips);
                for (j = 0; j < 16; j++) {
                    rglpshdef[i][j].fFree = TRUE;
                    rglpshdef[i][j].grbitPlr = 0;
                }
            }
            iplrSav = idPlayer;
            if (idPlayer == iplrNone) {
                idPlayer = i;
            } else {
                idPlayer = iplrNone;
            }
            if (!FReadShDef((RTSHDEF *)rgbCur, rglpshdef[i], iplrSav))
                goto CorruptHist;
            idPlayer = iplrSav;
            rgplr[i].cShDef--;
            ReadRt();
        }
        i = 0;
        while (hdrCur.rt == rtShDef) {
            for (; rgplr[i].cshdefSB == 0 && i < game.cPlayer; i++) {
            }
            if (i == game.cPlayer)
                break;
            if (!rglpshdefSB[i]) {
                rglpshdefSB[i] = LpAlloc(10 * sizeof(SHDEF), htShips);
                for (j = 0; j < 10; j++) {
                    rglpshdefSB[i][j].fFree = TRUE;
                    rglpshdefSB[i][j].grbitPlr = 0;
                }
            }
            iplrSav = idPlayer;
            if (idPlayer == iplrNone) {
                idPlayer = i;
            } else {
                idPlayer = iplrNone;
            }
            if (!FReadShDef((RTSHDEF *)rgbCur, rglpshdefSB[i], iplrSav))
                goto CorruptHist;
            idPlayer = iplrSav;
            rgplr[i].cshdefSB += 15;
            ReadRt();
        }
        while (hdrCur.rt == rtScore) {
            iplr = ((SCOREX *)rgbCur)->iPlayer;
            sx = *(SCOREX *)rgbCur;
            if (!rgsxPlr[iplr]) {
                rgsxPlr[iplr] = LpAlloc(101 * sizeof(SCOREX), htMisc);
                rgcsxPlr[iplr] = 0;
            }
            if (rgsxPlr[iplr]) {
                if (sx.fHistory) {
                    turnCur = sx.turn;
                } else {
                    turnCur = game.turn;
                }
                for (isx = 0; isx < rgcsxPlr[iplr] && turnCur > rgsxPlr[iplr][isx].turn; isx++) {
                }
                if ((isx >= rgcsxPlr[iplr] || turnCur == rgsxPlr[iplr][isx].turn) && isx < 101) {
                    if (isx == rgcsxPlr[iplr]) {
                        rgcsxPlr[iplr]++;
                    }
                } else if (rgcsxPlr[iplr] < 101) {
                    memmove(rgsxPlr[iplr] + (isx + 1), rgsxPlr[iplr] + isx, (rgcsxPlr[iplr] - isx) * sizeof(SCOREX));
                    rgcsxPlr[iplr]++;
                } else if (isx > 0) {
                    if (isx > 1) {
                        memmove(rgsxPlr[iplr], rgsxPlr[iplr] + 1, (isx - 1) * sizeof(SCOREX));
                    }
                    isx--;
                }
                rgsxPlr[iplr][isx] = sx;
                rgsxPlr[iplr][isx].turn = turnCur;
                rgsxPlr[iplr][isx].fHistory = TRUE;
            }
            ReadRt();
        }
        if (hdrCur.rt == rtAiData) {
            if (rgplr[idPlayer].fAi) {
                if (!vlpbAiData) {
                    vlpbAiData = LpAlloc(8096, htMisc);
                    if (!vlpbAiData)
                        goto CorruptHist;
                }
                lpb = vlpbAiData;
                while (hdrCur.rt == rtAiData) {
                    memmove(lpb, rgbCur, hdrCur.cb);
                    lpb += hdrCur.cb;
                    ReadRt();
                }
            } else {
                while (hdrCur.rt == rtAiData) {
                    ReadRt();
                }
            }
        }
        if (hdrCur.rt == rtThing) {
            cThing = RawLoad16(rgbCur);
            cThingAlloc = cThing + 10;
            if (cThingAlloc > 4050) {
                cThingAlloc = 4050;
            }
            lpThings = LpAlloc(cThingAlloc * sizeof(THING), htThings);
            if (!lpThings)
                goto CorruptHist;
            memset(lpThings, 0, cThingAlloc * sizeof(THING));
            ReadRt();
            for (i = 0, lpth = lpThings; i < cThing; i++, lpth++) {
                if (hdrCur.rt != rtThing)
                    goto CorruptHist;
                memcpy(lpth, rgbCur, hdrCur.cb);
                ReadRt();
            }
        }
        StreamClose();
    } else {
    LNoHistFile:
        cPlanetHist = 0;
        FreeLp(lpPlanets, htPlanets);
        lpPlanets = NULL;
        cThing = 0;
        FreeLp(lpThings, htThings);
        lpThings = NULL;
    }
    fFileErrSilent = fSilentSav;
    GetFileStatus(dt, iPlayer);
    if (!FOpenFile(dt | grf, iPlayer, 32))
        goto LError;
    if (iPlayer == iplrNone) {
        gd.fGameOverMan = ((RTBOF *)rgbCur)->fGameOverMan;
    }
LNextTurn:
    cturn++;
    cPlanet = 0;
    cFleet = 0;
    ReadRt();
    while (hdrCur.rt == rtBtlData || hdrCur.rt == rtContinue) {
        if (hdrCur.rt != rtContinue) {
            if (!lpbBattleLog) {
                lpbBattleLog = LpAlloc(0xffc8, htBattle);
                lpbBattleCur = lpbBattleLog;
            }
            /* NATIVE: Win16 offsets include its 16-byte HB, not native address bits. */
            lphb = LphbFromLpHt(lpbBattleCur, htBattle);
            if (0xffc8 - (uint32_t)(lpbBattleCur - (uint8_t *)lphb - sizeof(HB) + 16) < (uint32_t)((BTLDATA *)rgbCur)->cbData) {
                RawStore16(lpbBattleCur, 0xffff);
                lpbBattleCur = LpAlloc(0xffc8, htBattle);
            }
        }
        memmove(lpbBattleCur, rgbCur, hdrCur.cb);
        lpbBattleCur += hdrCur.cb;
        ReadRt();
    }
    if (lpbBattleCur) {
        RawStore16(lpbBattleCur, 0xffff);
        if (((VERS *)&wVersFile)->verMinor < 80) {
            UpdateBattleRecords();
        }
    }
    while (hdrCur.rt == rtPlr) {
        i = ((PLAYER *)rgbCur)->iPlayer;
        ReadRtPlr(&rgplr[i], rgbCur);
        cPlanet += rgplr[i].cPlanet;
        rgplr[i].cPlanet = 0;
        cFleet += rgplr[i].cFleet;
        rgplr[i].cFleet = 0;
        ReadRt();
    }
    if (dt != 2) {
        lSaltCur = rgplr[iPlayer].lSalt;
    } else if (hdrCur.rt == rtChgPassword) {
        lSaltCur = RawLoad32(rgbCur);
        ReadRt();
    } else {
        lSaltCur = 0;
    }
    if (!FCheckPassword()) {
        if (ini.fValidate || ini.fLogging)
            AlertSz(PszFormatIds(idsPasswordHaveEnteredIncorrectPleaseTry, NULL), MB_ICONHAND);
        goto LError;
    }
    ReadPlayerMessages();
    ResetHb(htFleets);
    ResetHb(htOrd);
    FreeLp(rglpfl, htMisc);
    rglpfl = NULL;
    if (!lpPlanets) {
        cPlanetAlloc = 1 <= cPlanet ? cPlanet : 1;
        lpPlanets = LpAlloc(cPlanetAlloc * sizeof(PLANET), htPlanets);
    }
    lppl = lpPlanets;
    j = 0;
    for (i = 0; i < cPlanet; i++) {
        fHaveHistoryData = FALSE;
        if (cPlanetHist != 0) {
            while (j < cPlanetHist && ((RTPLANET *)rgbCur)->id > lppl->id) {
                j++;
                lppl++;
            }
            if (j < cPlanetHist) {
                if (((RTPLANET *)rgbCur)->id == lppl->id) {
                    fHaveHistoryData = TRUE;
                    goto LFoundPlanet;
                }
            }
            if (cPlanetAlloc == cPlanetHist) {
                cPlanetAlloc += 8;
                lpPlanets = LpReAlloc(lpPlanets, cPlanetAlloc * sizeof(PLANET), htPlanets);
                lppl = lpPlanets + j;
            }
            if (j < cPlanetHist) {
                memmove(lppl + 1, lppl, (cPlanetHist - j) * sizeof(PLANET));
            }
            cPlanetHist++;
        }
    LFoundPlanet:
        if (!FReadPlanet(iPlayer, lppl, FALSE, fHaveHistoryData))
            goto Corrupt;
        if (lppl->iPlayer != iplrNone) {
            rgplr[lppl->iPlayer].cPlanet = rgplr[lppl->iPlayer].cPlanet + 1;
        }
        ReadRt();
        if (hdrCur.rt == rtProdQ) {
            if (lppl->lpplprod && lppl->lpplprod->iprodMax <= hdrCur.cb / 4) {
                FreePl((PL *)lppl->lpplprod);
                lppl->lpplprod = NULL;
            }
            if (!lppl->lpplprod) {
                lppl->lpplprod = (PLPROD *)LpplAlloc(4, hdrCur.cb / 4 + 2, htOrd);
            }
            memmove(lppl->lpplprod->rgprod, rgbCur, hdrCur.cb);
            lppl->lpplprod->iprodMac = hdrCur.cb / 4;
            ReadRt();
        }
        if (cPlanetHist == 0) {
            lppl++;
        }
    }
    if (cPlanetHist != 0) {
        cPlanet = cPlanetHist;
    }
    for (i = 0; i < game.cPlayer; i++) {
        if (i == iPlayer) {
            rglpshdef[i] = rgshdef;
            goto FreeShdef;
        }
        if (rgplr[i].fInclude) {
            if (!rglpshdef[i]) {
                rglpshdef[i] = LpAlloc(16 * sizeof(SHDEF), htShips);
            FreeShdef:
                for (j = 0; j < 16; j++) {
                    rglpshdef[i][j].fFree = TRUE;
                    rglpshdef[i][j].grbitPlr = 0;
                }
            }
        } else {
            continue;
        }
        iplrSav = idPlayer;
        if (idPlayer == iplrNone) {
            idPlayer = i;
        } else if (i != idPlayer) {
            idPlayer = iplrNone;
        }
        for (j = 0; j < rgplr[i].cShDef; j++) {
            if (hdrCur.rt != rtShDef) {
                idPlayer = iplrSav;
                goto Corrupt;
            }
            if (!FReadShDef((RTSHDEF *)rgbCur, rglpshdef[i], iplrSav))
                goto Corrupt;
            ReadRt();
        }
        idPlayer = iplrSav;
    }
    for (i = 0; i < game.cPlayer; i++) {
        rgplr[i].cShDef = 0;
        if (rglpshdef[i]) {
            for (j = 0; j < 16; j++) {
                if (!rglpshdef[i][j].fFree) {
                    if (i != idPlayer && !gd.fGeneratingTurn && rgplr[i].fDead) {
                        rglpshdef[i][j].fFree = TRUE;
                    } else {
                        rgplr[i].cShDef++;
                    }
                }
            }
        }
    }
    rglpfl = LpAlloc((1 <= cFleet ? cFleet : 1) * sizeof(FLEET *), htMisc);
    for (i = 0; i < cFleet; i++) {
        lpfl = rglpfl[i] = LpAlloc(sizeof(FLEET), htFleets);
        if (!FReadFleet(lpfl))
            goto LError;
        rgplr[lpfl->iPlayer].cFleet = rgplr[lpfl->iPlayer].cFleet + 1;
    }
    for (i = 0; i < game.cPlayer; i++) {
        if (rgplr[i].fInclude) {
            if (!rglpshdefSB[i]) {
                rglpshdefSB[i] = LpAlloc(10 * sizeof(SHDEF), htShips);
                for (j = 0; j < 10; j++) {
                    rglpshdefSB[i][j].fFree = TRUE;
                    rglpshdefSB[i][j].grbitPlr = 0;
                }
            }
            iplrSav = idPlayer;
            if (idPlayer == iplrNone) {
                idPlayer = i;
            } else if (i != idPlayer) {
                idPlayer = iplrNone;
            }
            for (j = 0; j < (int16_t)rgplr[i].cshdefSB; j++) {
                if (hdrCur.rt != rtShDef) {
                    idPlayer = iplrSav;
                    goto Corrupt;
                }
                if (!FReadShDef((RTSHDEF *)rgbCur, rglpshdefSB[i], iplrSav))
                    goto Corrupt;
                ReadRt();
            }
            idPlayer = iplrSav;
        }
    }
    for (i = 0; i < game.cPlayer; i++) {
        rgplr[i].cshdefSB = 0;
        if (rglpshdefSB[i]) {
            for (j = 0; j < 10; j++) {
                if (!rglpshdefSB[i][j].fFree) {
                    if (i != idPlayer && !gd.fGeneratingTurn && rgplr[i].fDead) {
                        rglpshdefSB[i][j].fFree = TRUE;
                    } else {
                        rgplr[i].cshdefSB++;
                    }
                }
            }
        }
    }
    if (!vlprgScoreX) {
        vlprgScoreX = LpAlloc(game.cPlayer * sizeof(SCOREX), htMisc);
        memset(vlprgScoreX, 0, game.cPlayer * sizeof(SCOREX));
    }
    while (hdrCur.rt == rtScore) {
        iplr = ((SCOREX *)rgbCur)->iPlayer;
        vlprgScoreX[iplr] = *(SCOREX *)rgbCur;
        if (!rgsxPlr[iplr]) {
            rgsxPlr[iplr] = LpAlloc(101 * sizeof(SCOREX), htMisc);
            rgcsxPlr[iplr] = 0;
        }
        if (rgsxPlr[iplr]) {
            if (vlprgScoreX[iplr].fHistory) {
                turnCur = vlprgScoreX[iplr].turn;
            } else {
                turnCur = game.turn;
            }
            for (isx = 0; isx < rgcsxPlr[iplr] && turnCur > rgsxPlr[iplr][isx].turn; isx++) {
            }
            if ((isx >= rgcsxPlr[iplr] || turnCur == rgsxPlr[iplr][isx].turn) && isx < 101) {
                if (isx == rgcsxPlr[iplr]) {
                    rgcsxPlr[iplr]++;
                }
            } else if (rgcsxPlr[iplr] < 101) {
                memmove(rgsxPlr[iplr] + (isx + 1), rgsxPlr[iplr] + isx, (rgcsxPlr[iplr] - isx) * sizeof(SCOREX));
                rgcsxPlr[iplr]++;
            } else if (isx > 0) {
                if (isx > 1) {
                    memmove(rgsxPlr[iplr], rgsxPlr[iplr] + 1, (isx - 1) * sizeof(SCOREX));
                }
                isx--;
            }
            rgsxPlr[iplr][isx] = vlprgScoreX[iplr];
            rgsxPlr[iplr][isx].turn = turnCur;
            rgsxPlr[iplr][isx].fHistory = TRUE;
        }
        ReadRt();
    }
    if (lpThings) {
        FreeLp(lpThings, htThings);
        lpThings = NULL;
        cThing = 0;
    }
    if (hdrCur.rt == rtThing) {
        fHist = cThing > 0;
        cThingFile = RawLoad16(rgbCur);
        cThingAlloc = 10 <= cThingFile ? cThingFile : 10;
        if (!lpThings) {
            lpThings = LpAlloc(cThingAlloc * sizeof(THING), htThings);
            if (!lpThings)
                goto LError;
            memset(lpThings, 0, cThingAlloc * sizeof(THING));
        }
        ReadRt();
        lpth = lpThings;
        j = 0;
        for (i = 0; i < cThingFile; i++) {
            fHaveHistoryData = FALSE;
            if (fHist) {
                for (; j < cThing && ((THING *)rgbCur)->idFull > lpth->idFull; lpth++) {
                    j++;
                }
                if (j < cThing && ((THING *)rgbCur)->idFull == lpth->idFull) {
                    fHaveHistoryData = TRUE;
                    goto LFoundThing;
                }
                if (cThingAlloc == cThing) {
                    cThingAlloc += 8;
                    lpThings = LpReAlloc(lpThings, cThingAlloc * sizeof(THING), htThings);
                    lpth = lpThings + j;
                }
                if (j < cThing) {
                    memmove(lpth + 1, lpth, (cThing - j) * sizeof(THING));
                    memset(lpth, 0, sizeof(THING));
                }
            }
            cThing++;
        LFoundThing:
            memcpy(lpth, rgbCur, hdrCur.cb);
            lpth->turn = game.turn;
            lpth++;
            j++;
            ReadRt();
        }
    } else {
        cThing = 0;
        cThingAlloc = 10;
        lpThings = LpAlloc(cThingAlloc * sizeof(THING), htThings);
    }
    if (hdrCur.rt == rtSel) {
        ReadRt();
    }
    iplrSav = idPlayer;
    while (hdrCur.rt == rtBtlPlan) {
        iP = ((BTLPLAN *)rgbCur)->iplr;
        idPlayer = iP;
        if (!rglpbtlplan[iP]) {
            rglpbtlplan[iP] = LpAlloc(16 * sizeof(BTLPLAN), htShips);
        }
        UnpackBattlePlan(rgbCur, rglpbtlplan[iP] + rgcbtlplan[iP], rgcbtlplan[iP]);
        rgcbtlplan[iP]++;
        ReadRt();
    }
    idPlayer = iplrSav;
    if (hdrCur.rt != rtEOF) {
    Corrupt:
        AlertSz(PszFormatIds(idsGameFileAppearsCorruptUnableLoadFile, NULL), MB_ICONHAND);
        goto LError;
    }
    if (CbFileSize(hf) != LSeekFile(hf, 0, 1)) {
        ReadRt();
        if (hdrCur.rt == rtBOF) {
            game.turn = ((RTBOF *)rgbCur)->turn;
            game.wGen = ((RTBOF *)rgbCur)->wGen;
            for (i = 0; i < game.cPlayer; i++) {
                rgplr[i].cShDef = 0;
                rgplr[i].cFleet = 0;
                rgplr[i].cPlanet = 0;
                rgplr[i].cshdefSB = 0;
                rgcbtlplan[i] = 0;
            }
            lppl = lpPlanets;
            lpplMac = lpPlanets + cPlanet;
            for (; lppl < lpplMac; lppl++) {
                if (lppl->iPlayer == iPlayer) {
                    lppl->iPlayer = iplrNone;
                    lppl->det = detSome;
                    if (lppl->lpplprod) {
                        FreePl((PL *)lppl->lpplprod);
                        lppl->lpplprod = NULL;
                    }
                }
            }
            cPlanetHist = cPlanet;
            goto LNextTurn;
        }
        AlertSz(PszFormatIds(idsWarningIgnoringUnexpectedDataAfterEof, NULL), MB_ICONHAND);
    }
    StreamClose();
    if (cturn > 1 && !rgplr[iPlayer].fAi && !ini.fDumpPlanets && !ini.fDumpFleets && !ini.fDumpMap) {
        CchSprintf(szWork, PszGetCompressedString(idsNoteDYearsDataRead), cturn);
        AlertSz(szWork, MB_ICONASTERISK);
    }
    if (FSzPrefixNoCase(pszExt, "hst", 3))
        goto DoneNow;
    if (!rgplr[iPlayer].fAi) {
        lpth = lpThings;
        lpthMac = lpThings + cThing;
        for (; lpth < lpthMac; lpth++) {
            if (lpth->ith == ithMineralPacket && lpth->thp.iWarp != 0) {
                lppl = LpplFromId(lpth->thp.idPlanet);
                if (lppl && lppl->iPlayer == iPlayer) {
                    iWarp = IWarpMAFromLppl(lppl, &fTwo);
                    if (iWarp + fTwo < lpth->thp.iWarp + 4) {
                        FSendPlrMsg2XGen(FALSE, idmMassPacketAppearsCollisionCourseWhichCurrently, gotoThing, lpth->idFull, lppl->id);
                    }
                }
            }
        }
        if (!game.fTutorial) {
            lppl = lpPlanets;
            lpplMac = lpPlanets + cPlanet;
            for (; lppl < lpplMac; lppl++) {
                if (lppl->iPlayer == iPlayer && lppl->fStarbase && lppl->lpplprod && rglpshdefSB[lppl->iPlayer][lppl->isb].hul.ihuldef != ihuldefOrbitalFort) {
                    fWorking = FALSE;
                    iprod = 0;
                    lpprod = lppl->lpplprod->rgprod;
                    while (iprod < lppl->lpplprod->iprodMac) {
                        EstimateItemProdSched(lppl, NULL, iprod, &iFirst, &iLast);
                        if (iLast > 1) {
                            fWorking = FALSE;
                            break;
                        }
                        if (iLast == 1 && (lpprod->grobj != grobjPlanet || lpprod->iItem >= mdIdleFactory)) {
                            fWorking = TRUE;
                        }
                        iprod++;
                        lpprod++;
                    }
                    if (fWorking) {
                        FSendPlrMsg2XGen(FALSE, idmStarbaseScheduledCompleteRemainingProductionItem, lppl->id, lppl->id, 0);
                    }
                }
            }
            i = CBattles();
            if (i > 0) {
                FSendPlrMsg2XGen(TRUE, idmHaveReceivedOneBattleRecordingYear + (i > 1), gotoBattleReport, i, 0);
            }
        }
    }
    if (!gd.fDontDoLogFiles) {
        CchSprintf(szWork, "%s.x%s", pszFileName, pszExt + 1);
        if (!FLoadLogFile(szWork) || !FRunLogFile()) {
            AlertSz(PszFormatIds(idsPlayerLogFileAppearsCorruptUnableLoad, NULL), MB_ICONHAND);
            goto LError;
        }
    }
    for (i = 0; i < game.cPlayer; i++) {
        rgplr[i].cFleet = 0;
        rgplr[i].cPlanet = 0;
    }
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer != iplrNone) {
            rgplr[lppl->iPlayer].cPlanet = rgplr[lppl->iPlayer].cPlanet + 1;
        } else {
            lppl->fStarbase = FALSE;
        }
    }
    j = 0;
    for (i = 0; i < cFleet; i++) {
        lpfl = rglpfl[i];
        if (!rglpfl[i])
            break;
        j = lpfl->iPlayer;
        rgplr[j].cFleet++;
    }
DoneNow:
    idPlayer = iPlayer;
    if (idPlayer != iplrNone && !rgplr[idPlayer].fAi) {
        AddMRUFile(pszFileName, pszExt);
    }
    return TRUE;
}

int16_t FReadPlanet(int16_t iPlayer, PLANET *lppl, int16_t fHistory, int16_t fPreInited) {
    int16_t   fFirstYear;
    int16_t   fRouting;
    uint8_t   bMask;
    int16_t   i;
    uint8_t  *pb;
    MessageId idm;
    int16_t   pctOpt;
    int16_t   pct;

    fFirstYear = FALSE;
    if (!fPreInited) {
        memset(lppl, 0, sizeof(PLANET));
    }
    if (fHistory || iPlayer == iplrNone) {
        lppl->fFirstYear = ((RTPLANET *)rgbCur)->fFirstYear;
    } else if (!fPreInited) {
        fFirstYear = TRUE;
        lppl->fFirstYear = TRUE;
    } else if (lppl->fFirstYear) {
        if (lppl->turn != game.turn) {
            lppl->fFirstYear = FALSE;
        } else {
            fFirstYear = TRUE;
        }
    }
    lppl->id = ((RTPLANET *)rgbCur)->id;
    lppl->iPlayer = ((RTPLANET *)rgbCur)->iPlayer;
    if (lppl->det < ((RTPLANET *)rgbCur)->det) {
        lppl->det = ((RTPLANET *)rgbCur)->det;
    }
    lppl->fInclude = ((RTPLANET *)rgbCur)->fInclude;
    lppl->fStarbase = ((RTPLANET *)rgbCur)->fStarbase;
    lppl->fHomeworld = ((RTPLANET *)rgbCur)->fHomeworld;
    fRouting = ((RTPLANET *)rgbCur)->fRouting;
    if (lppl->fStarbase && lppl->iPlayer == iplrNone) {
        lppl->fStarbase = FALSE;
    }
    if (!fHistory) {
        lppl->turn = game.turn;
    }
    pb = (uint8_t *)(((RTPLANET *)rgbCur) + 1);
    if (((RTPLANET *)rgbCur)->det >= detSome) {
        bMask = *pb;
        pb++;
        i = 0;
        while (i < 3) {
            if (bMask & 3) {
                if ((bMask & 3) != 1) {
                    return FALSE;
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
                return FALSE;
            }
            lppl->rgEnvVar[i] = lppl->rgEnvVarOrig[i] = *pb++;
        }
        if (((RTPLANET *)rgbCur)->fIncEVO != 0) {
            for (i = 0; i < 3; i++) {
                if (*pb > 100) {
                    return FALSE;
                }
                lppl->rgEnvVarOrig[i] = *pb++;
            }
        }
        if (((RTPLANET *)rgbCur)->iPlayer != iplrNone) {
            lppl->uGuesses = RawLoad16(pb);
            pb += 2;
        }
        if (lppl->det <= detSome)
            goto LFinishBRecord;
        if (((RTPLANET *)rgbCur)->fIncSurfMin != 0) {
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
                    lppl->rgwtMin[i] = (uint32_t)RawLoad16(pb);
                    pb += 2;
                    break;
                case 3:
                    lppl->rgwtMin[i] = RawLoad32(pb);
                    pb += 4;
                }
                i++;
                bMask >>= 2;
            }
        }
        if (hdrCur.rt == rtPlanetB)
            goto LFinishBRecord;
        if (((RTPLANET *)rgbCur)->fIncImp != 0) {
            memmove(lppl->rgbImp, pb, 8);
            pb += 8;
        } else {
            lppl->fArtifact = ((RTPLANET *)rgbCur)->fIsArtifact;
            lppl->iScanner = 31;
            lppl->cDefenses = 0;
        }
        if (lppl->iPlayer != iplrNone) {
            if (lppl->fStarbase) {
                lppl->lStarbase = RawLoad32(pb);
                lppl->fNoHeal = FALSE;
                pb += 4;
            }
            if (fRouting) {
                lppl->wRouting = RawLoad16(pb);
            }
        }
        return TRUE;
    }
LFinishBRecord:
    if (lppl->fStarbase) {
        lppl->isb = *pb;
        pb++;
    }
    if (fHistory) {
        lppl->turn = RawLoad16(pb);
        pb += 2;
    } else if (fFirstYear) {
        if (lppl->iPlayer != iplrNone) {
            FSendPlrMsg2XGen(FALSE, idmHaveFoundPlanetOccupiedSomeoneElseCurrently, lppl->id, lppl->id, lppl->iPlayer | 0x30);
        } else if (lppl->det <= detMinimal) {
            FSendPlrMsg2XGen(FALSE, idmHaveFoundNewPlanetDontKnowIf, lppl->id, lppl->id, 0);
        } else if (GetRaceStat(&rgplr[iPlayer], rsMajorAdv) == raTerra) {
            pctOpt = PctPlanetOptValue(lppl, iPlayer);
            FSendPlrMsg2XGen(FALSE, idmHaveInfoNewPlanetIfColonizeCan, lppl->id, lppl->id, pctOpt);
        } else {
            pct = PctPlanetDesirability(lppl, iPlayer);
            if (pct > 0) {
                pct = PctTrueMaxGrowth(iPlayer) * pct;
                idm = idmHaveFoundNewHabitablePlanetColonistsWill;
            } else {
                pctOpt = PctPlanetOptValue(lppl, iPlayer);
                if (pctOpt > 0) {
                    pct = PctTrueMaxGrowth(iPlayer) * pctOpt;
                    idm = idmHaveFoundNewPlanetWhichHaveAbility;
                } else {
                    pct = 10 * pct;
                    idm = idmHaveFoundNewPlanetWhichUnfortunatelyHabitable;
                }
            }
            FSendPlrMsg2XGen(FALSE, idm, lppl->id, abs(pct), lppl->id);
        }
    }
    return TRUE;
}

int16_t FReadFleet(FLEET *lpfl) {
    uint16_t  us;
    int16_t   cord;
    int16_t   fByte;
    ORDER    *lpord;
    int16_t   i;
    int16_t   cish;
    uint8_t  *pb;
    int16_t   cch;
    uint16_t *pus;
    char      szT[33];
    int16_t   cOut;

    cish = 0;
    memset(lpfl, 0, sizeof(FLEET));
    memmove(lpfl, rgbCur, offsetof(RTFLEET, grbitCsh));
    fByte = ((RTFLEET *)rgbCur)->fByteCsh;
    us = ((RTFLEET *)rgbCur)->grbitCsh;
    pb = ((RTFLEET *)rgbCur)->rgb;
    if (fByte) {
        i = 0;
        for (; us != 0; us >>= 1) {
            if (us & 1) {
                lpfl->rgcsh[i] = *pb++;
                if (lpfl->rgcsh[i] != 0) {
                    cish++;
                }
            }
            i++;
        }
    } else {
        pus = (uint16_t *)pb;
        i = 0;
        for (; us != 0; us >>= 1) {
            if (us & 1) {
                lpfl->rgcsh[i] = *pus++;
                if (lpfl->rgcsh[i] != 0) {
                    cish++;
                }
            }
            i++;
        }
        pb = (uint8_t *)pus;
    }
    if (cish == 0) {
        lpfl->fDead = TRUE;
    }
    if (lpfl->det >= detMore) {
        us = RawLoad16(pb);
        pb += 2;
        i = 0;
        while (i < 5) {
            switch (us & 3) {
            default:
                break;
            case 1:
                lpfl->rgwtMin[i] = (uint32_t)*pb;
                pb++;
                break;
            case 2:
                lpfl->rgwtMin[i] = (uint32_t)RawLoad16(pb);
                pb += 2;
                break;
            case 3:
                lpfl->rgwtMin[i] = RawLoad32(pb);
                pb += 4;
            }
            i++;
            us >>= 2;
        }
    }
    if (lpfl->det < detAll) {
        lpfl->dirLong = RawLoad32(pb);
        pb += 4;
        lpfl->wtFleet = RawLoad32(pb);
        pb += 4;
        ReadRt();
        return TRUE;
    }
    if (hdrCur.rt != rtFleetA) {
    Corrupt:
        AlertSz(PszFormatIds(idsGameFileAppearsCorruptUnableLoadFile, NULL), MB_ICONHAND);
        return FALSE;
    }
    us = RawLoad16(pb);
    pb += 2;
    pus = (uint16_t *)pb;
    i = 0;
    for (; us != 0; us >>= 1) {
        if (us & 1) {
            lpfl->rgdv[i].dp = *pus++;
            if (lpfl->rgdv[i].pctDp >= 500) {
                lpfl->rgdv[i].pctDp = 499;
            }
        }
        i++;
    }
    pb = (uint8_t *)pus;
    lpfl->iplan = *pb++;
    lpfl->cord = *pb++;
    lpfl->lpplord = (PLORD *)LpplAlloc(18, lpfl->cord + 1, htOrd);
    memset(lpfl->lpplord->rgord, 0, (lpfl->cord + 1) * 18);
    cord = lpfl->cord;
    lpord = lpfl->lpplord->rgord;
    for (; cord != 0; cord--) {
        memset(rgbCur, 0, 18);
        ReadRt();
        if (hdrCur.rt != rtOrderA && hdrCur.rt != rtOrderB)
            goto Corrupt;
        *lpord = *(ORDER *)rgbCur;
        lpord->fNoAutoTrack = FALSE;
        lpord++;
    }
    lpfl->lpplord->iordMac = lpfl->cord;
    if (lpfl->idPlanet != idPlanetDeepSpace) {
        if (lpfl->idPlanet > game.cPlanMax) {
            lpfl->idPlanet = idPlanetDeepSpace;
        }
        if (lpfl->pt.x != rgptPlan[lpfl->idPlanet].x || lpfl->pt.y != rgptPlan[lpfl->idPlanet].y) {
            if (i != 0 || game.turn != 0)
                goto Corrupt;
            lpfl->pt = rgptPlan[lpfl->idPlanet];
        }
    }
    ReadRt();
    if (hdrCur.rt == rtString) {
        cch = rgbCur[0];
        if (cch == 0) {
            lpfl->lpszName = LpAlloc(strlen(&rgbCur[1]) + 1, htString);
            strcpy(lpfl->lpszName, &rgbCur[1]);
        } else {
            cOut = 32;
            FDecompressUserString(&rgbCur[1], cch, szT, &cOut);
            lpfl->lpszName = LpAlloc(strlen(szT) + 1, htString);
            strcpy(lpfl->lpszName, szT);
        }
        ReadRt();
    } else {
        lpfl->lpszName = NULL;
    }
    return TRUE;
}

void UnpackBattlePlan(uint8_t *lpb, BTLPLAN *lpbtlplan, int16_t iplan) {
    char    szTemp[33];
    char    szName[33];
    int16_t cch;
    int16_t cOut;

    memmove(lpbtlplan, lpb, 4);
    lpb += 4;
    cch = *lpb;
    lpb++;
    if (cch == 0) {
        strcpy(lpbtlplan->szName, lpb);
    } else {
        cOut = 32;
        memmove(szTemp, lpb, cOut);
        FDecompressUserString(szTemp, cch, szName, &cOut);
        memmove(lpbtlplan->szName, szName, cOut);
    }
    lpbtlplan->iplan = iplan;
    return;
}

void UpdateBattleRecords() {
    BTLDATA  *lpbd;
    BTLREC   *lpbr;
    int16_t   cKill;
    HB       *lphb;
    BTLREC26 *lpbr26;
    int16_t   itok;

    lphb = rglphb[11];
    if (lphb) {
        lpbd = (BTLDATA *)((uint8_t *)lphb + (sizeof(HB) + 2));
        while (1) {
            if (lpbd->id == 0xffff) {
                lphb = lphb->lphbNext;
                if (!lphb || lphb->ibTop <= sizeof(HB))
                    break;
                lpbd = (BTLDATA *)((uint8_t *)lphb + (sizeof(HB) + 2));
            } else {
                if (lpbd->cbData == 0)
                    break;
                lpbr = (BTLREC *)&lpbd->rgtok[lpbd->ctok];
                lpbr26 = (BTLREC26 *)lpbr;
                lpbd = (BTLDATA *)((uint8_t *)lpbd + lpbd->cbData);
                while (lpbr < (BTLREC *)lpbd) {
                    cKill = lpbr26->ctok;
                    itok = lpbr26->itokAttack;
                    lpbr->ctok = cKill;
                    lpbr->itokAttack = itok;
                    lpbr = (BTLREC *)&lpbr->rgkill[lpbr->ctok];
                    lpbr26 = (BTLREC26 *)lpbr;
                }
            }
        }
    }
    return;
}

void DestroyCurGame() {
    int16_t i;

    if (gd.fSendMsgMode) {
        FFinishPlrMsgEntry(0);
    }
    if (idPlayer != iplrNone && game.fDirty) {
        PromptSaveGame();
    }
    ResetHb(htPlanets);
    lpPlanets = NULL;
    cPlanet = 0;
    ResetHb(htFleets);
    rglpfl = NULL;
    cFleet = 0;
    ResetHb(htThings);
    lpThings = NULL;
    cThing = 0;
    cThingAlloc = 0;
    vlprgScoreX = NULL;
    vrptFleet.fCached = FALSE;
    vrptPlanet.fCached = FALSE;
    vrptBattle.fCached = FALSE;
    vrptEFleet.fCached = FALSE;
    for (i = 0; i < 16; i++) {
        rgsxPlr[i] = NULL;
    }
    lpbBattleT = NULL;
    lpbBattleLog = NULL;
    lpbBattleCur = NULL;
    gd.fAisDone = FALSE;
    gd.fGotoVCR = FALSE;
    gd.fFleetLinkValid = FALSE;
    ResetHb(htBattle);
    if (rglphb[11]) {
        rglphb[11][1].cbBlock = 0xffff;
    }
    ResetHb(htMisc);
    ResetHb(htString);
    ResetHb(htShips);
    ResetHb(htOrd);
    ResetHb(htPlrMsg);
    for (i = 0; i < 16; i++) {
        rglpshdef[i] = NULL;
        rglpshdefSB[i] = NULL;
        rglpbtlplan[i] = NULL;
        rgcbtlplan[i] = 0;
    }
    if (sel.grobj != grobjNone) {
        ini.grobjSel = sel.grobj;
        ini.iObjSel = sel.id;
        ini.idPlayer = idPlayer;
        ini.lid = game.lid;
    }
    idPlayer = iplrNone;
    imemLogCur = 0;
    imemLogPrev = -1;
    iMsgCur = 0;
    vlpbAiData = NULL;
    ResetMessages();
    lSaltCur = 0;
    ctickLast = 0;
    game.lid = 0;
    game.cPlayer = 0;
    game.cPlanMax = 0;
    game.fDirty = FALSE;
    game.turn = 0;
    game.szName[0] = 0;
    gd.fGameOverMan = FALSE;
    gd.fSendMsgMode = FALSE;
    CloseGameWindows();
    sel.scan.grobjFull = grobjNone;
    sel.scan.grobj = grobjNone;
    sel.scan.iwp = iwpNone;
    sel.scan.ifl = iflNone;
    sel.scan.idpl = idplNone;
    sel.grobjFull = grobjNone;
    sel.grobj = grobjNone;
    sel.id = -1;
    sel.pt.y = 0;
    sel.pt.x = 0;
    sel.pl.id = idplNone;
    sel.fl.id = idflNone;
    sel.fl.lpplord = NULL;
    sel.pl.lpplprod = NULL;
    return;
}

void FileError(StringId ids) {
    idsFileError = ids;
    if (!fFileErrSilent && !gd.fGeneratingTurn) {
        AlertSz(PszFormatIds(ids, NULL), MB_ICONHAND);
    }
    return;
}

void GetFileStatus(int16_t dt, int16_t iPlayer) {
    SetSzWorkFromDt(dt, iPlayer);
    gd.fReadOnly = FFileReadOnly(szWork);
    return;
}

int16_t FOpenFile(DtFileType dt, int16_t iPlayer, int16_t md) {
    RTBOF    rtbof;
    StringId ids;
    int16_t  fCheckMulti;
    int16_t  fRewind;
    int16_t  fSilentSav;
    jmp_buf *penvMemSav;
    jmp_buf  env;

    fSilentSav = fFileErrSilent;
    ids = idsCantOpenFile;
    gd.fPartialTurn = FALSE;
    fCheckMulti = dt & 0x2000;
    fRewind = dt & 0x1000;
    dt &= 0xff;
    SetSzWorkFromDt(dt, iPlayer);
    penvMemSav = penvMem;
    penvMem = &env;
    if (setjmp(env) != 0) {
        fFileErrSilent = fSilentSav;
        FileError(ids);
        StreamClose();
        penvMem = penvMemSav;
        return FALSE;
    }
    fFileErrSilent = TRUE;
    StreamOpen(szWork, md);
    fFileErrSilent = fSilentSav;
    ids = idsGameFileAppearsCorruptUnableLoadFile;
    ReadRt();
    if (hdrCur.rt != rtBOF || ((RTBOF *)rgbCur)->verMajor != 2 || ((RTBOF *)rgbCur)->verMinor < 49 || ((RTBOF *)rgbCur)->verMinor >= 85) {
        if (hdrCur.rt == rtBOF) {
            /* The original refused 2.84 but reported it as an older version. */
            FileError(((RTBOF *)rgbCur)->verMajor > 2 || (((RTBOF *)rgbCur)->verMajor == 2 && ((RTBOF *)rgbCur)->verMinor >= 85)
                          ? idsFileCreatedNewerVersionStarsMustUpgrade
                          : idsSorryFileCreatedOlderVersionStarsIncompatible);
        } else {
            FileError(idsFileDoesBelongVersionStars);
        }
    LBadFile:
        StreamClose();
        penvMem = penvMemSav;
        return FALSE;
    }
    rtbof = *((RTBOF *)rgbCur);
    if (rtbof.iPlayer != iPlayer) {
        FileError(idsGameFileAppearsCorruptUnableLoadFile);
        goto LBadFile;
    }
    if (game.lid != 0) {
        if (rtbof.lidGame != game.lid) {
            FileError(idsFileGame);
            goto LBadFile;
        }
        if (dt != dtHist) {
            if (fCheckMulti && rtbof.fMulti) {
                LSeekFile(hf, -4, 2);
                ReadRt();
                if (hdrCur.rt != rtEOF && hdrCur.cb != 2)
                    goto LBadFile;
                rtbof.turn = RawLoad16(rgbCur);
                game.wGen = rtbof.wGen;
            }
            if (game.turn == 0 && game.turn != rtbof.turn) {
                game.turn = rtbof.turn;
                game.wGen = rtbof.wGen;
            } else {
                if (rtbof.turn != game.turn) {
                    FileError(idsFileDate);
                    goto LBadFile;
                }
                if (dt == dtHost && !gd.fHostMode && rtbof.fInUse) {
                    if (AlertSz(PszFormatIds(idsHostFileMarkedUseAnotherInstanceStars, NULL), MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL) != IDYES)
                        goto LBadFile;
                } else {
                    if (!rtbof.fDone && gd.fGeneratingTurn && !gd.fForceTurn) {
                        gd.fPartialTurn = TRUE;
                        goto LBadFile;
                    }
                    if (dt == dtLog && !game.fTutorial && rtbof.wGen != game.wGen) {
                        FileError(idsFileGame);
                        goto LBadFile;
                    }
                }
            }
        } else if (rtbof.iPlayer != iPlayer) {
            goto LBadFile;
        }
    }
    if (fRewind) {
        LSeekFile(hf, 0, 0);
        ReadRt();
    }
    penvMem = penvMemSav;
    wVersFile = rtbof.wVersion;
    gd.fFileCrippled = rtbof.fCrippled;
    return TRUE;
}

int16_t FNewTurnAvail(int16_t idPlayer) {
    uint16_t wGenOld;
    uint16_t turnOld;
    int16_t  fNew;

    turnOld = game.turn;
    wGenOld = game.wGen;
    fFileErrSilent = TRUE;
    game.turn = 0;
    fNew = FOpenFile(8195, idPlayer, 32);
    if (fNew) {
        StreamClose();
        fNew = game.turn > turnOld;
    }
    game.turn = turnOld;
    game.wGen = wGenOld;
    return fNew;
}

int16_t FCheckFile(DtFileType dt, int16_t iPlayer, MdMark md) {
    int16_t  fReturn;
    int16_t  fOpened;
    uint16_t wGenOld;
    int16_t  f;
    int16_t  fErrSav;

    fErrSav = fFileErrSilent;
    wGenOld = game.wGen;
    if (dt == dtHost) {
        f = gd.fHostMode;
        gd.fHostMode = TRUE;
    }
    fFileErrSilent = TRUE;
    fOpened = FOpenFile(dt, iPlayer, 32);
    switch (md) {
    case mdMarkInUse:
        if (!fOpened || ((RTBOF *)rgbCur)->fInUse) {
            fReturn = TRUE;
            break;
        }
        fReturn = FALSE;
        break;
    case mdMarkDone:
        if (fOpened && ((RTBOF *)rgbCur)->fDone) {
            fReturn = TRUE;
            break;
        }
        fReturn = FALSE;
        break;
    case mdMarkMulti:
        if (fOpened && ((RTBOF *)rgbCur)->fMulti) {
            fReturn = TRUE;
            break;
        }
        fReturn = FALSE;
        break;
    case mdMarkAi:
        if (!fOpened) {
            fReturn = FALSE;
        } else {
            do {
                ReadRt();
            } while (hdrCur.rt != rtEOF && (hdrCur.rt != rtPlr || ((PLAYER *)rgbCur)->iPlayer != iPlayer));
            fReturn = hdrCur.rt == rtPlr && ((PLAYER *)rgbCur)->fAi;
        }
    }
    if (fOpened) {
        StreamClose();
    }
    if (dt == dtHost) {
        gd.fHostMode = f;
    }
    fFileErrSilent = fErrSav;
    game.wGen = wGenOld;
    return fReturn;
}

void ReadRt() {
    RgFromStream(&hdrCur, 2);
    if (hdrCur.cb != 0) {
        RgFromStream(rgbCur, hdrCur.cb);
    }
    if (hdrCur.rt == rtBOF) {
        SetFileXorStream(((RTBOF *)rgbCur)->lidGame, ((RTBOF *)rgbCur)->lSaltTime, ((RTBOF *)rgbCur)->turn, ((RTBOF *)rgbCur)->iPlayer,
                         ((RTBOF *)rgbCur)->fCrippled);
    } else if (hdrCur.rt != rtEOF) {
        XorFileBuf(rgbCur, hdrCur.cb);
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
        return TRUE;
    default:
        return FALSE;
    }
}

void StreamOpen(char *szFile, MdOpenFlags mdOpen) {
    uint32_t dwTick;
    int16_t  fMissing;
    int16_t  fNoErr;
    uint32_t dwTickCur;

    dwTick = 0;
    fNoErr = (mdOpen & 0x4000) != 0;
    mdOpen &= 0xbfff;
Retry:
    hf = HfOpenFile(szFile, mdOpen, &fMissing);
    if (hf == -1) {
        if (gd.fRetryOpens && !fMissing) {
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
            FileError(idsCantOpenFile);
        }
        StarsLongJump(penvMem, -1);
    }
}

void StreamClose() {
    if (hf != -1) {
        CloseFile(hf);
        hf = -1;
    }
    return;
}

void RgFromStream(void *rg, uint16_t cb) {
    if (cb != 0) {
        if (vlpMemStream) {
            memcpy(rg, vlpMemStream, cb);
            vlpMemStream += cb;
        } else if (CbReadFile(hf, rg, cb) != cb) {
            FileError(idsGameFileAppearsCorruptUnableLoadFile);
            StarsLongJump(penvMem, -1);
        }
    }
    return;
}
