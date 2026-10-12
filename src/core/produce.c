#include "common.h"

void InitProduction(PROD *rgprod) {
    int16_t  iWarp;
    int16_t  iSrc;
    uint16_t u;
    int16_t  i;
    int16_t  ipl;
    PART     part;
    PROD    *lpprod;

    gd.fNoResearchSav = sel.pl.fNoResearch;
    if (!rgprod) {
        rgprod = pProdGlob;
    }
    if (sel.pl.lpplprod) {
        i = sel.pl.lpplprod->iprodMac;
    } else {
        i = 2;
    }
    lpplProdGlob = (PLPROD *)LpplAlloc(4, i, htOrd);
    if (sel.pl.lpplprod) {
        memcpy(lpplProdGlob->rgprod, sel.pl.lpplprod->rgprod, i * 4);
    } else {
        i = 0;
    }
    lpplProdGlob->iprodMac = i;
    cProdGlob = 0;
    pProdGlob = rgprod;
    memset(rgprod, 0, 64 * sizeof(PROD));
    if (sel.pl.fStarbase && LphuldefFromId(rglpshdefSB[idPlayer][sel.pl.isb].hul.ihuldef)->hul.wtCargoMax != 0) {
        for (i = 0; i < 16; i++) {
            if (!rgshdef[i].fFree && !rgshdef[i].fGift) {
                if (LphuldefFromId(rglpshdefSB[idPlayer][sel.pl.isb].hul.ihuldef)->hul.wtCargoMax >= rgshdef[i].hul.wtEmpty) {
                    rgprod[cProdGlob].cItem = 0x3ff;
                    rgprod[cProdGlob].iItem = i;
                    rgprod[cProdGlob].grobj = grobjFleet;
                    cProdGlob++;
                }
            }
        }
    }
    for (i = 0; i < 10; i++) {
        if (!rglpshdefSB[idPlayer][i].fFree && !rglpshdefSB[idPlayer][i].fGift && (sel.pl.isb != i || !sel.pl.fStarbase)) {
            rgprod[cProdGlob].cItem = 1;
            rgprod[cProdGlob].iItem = LOWORD((int16_t)(i + 16));
            rgprod[cProdGlob].grobj = grobjFleet;
            cProdGlob++;
        }
    }
    part.hs.grhst = hstPlanetary;
    part.hs.iItem = iplanetaryGenesisDevice;
    if (FLookupPart(&part) == mdPartAvailAvailable) {
        rgprod[cProdGlob].cItem = 1;
        rgprod[cProdGlob].iItem = iobjGenesis;
        rgprod[cProdGlob].grobj = grobjPlanet;
        cProdGlob++;
    }
    iWarp = IWarpMAFromLppl(&sel.pl, NULL);
    if (iWarp > 0) {
        for (i = 0; i < 4; i++) {
            rgprod[cProdGlob].cItem = 0x3ff;
            rgprod[cProdGlob].iItem = LOWORD((int16_t)(i + 14));
            rgprod[cProdGlob].grobj = grobjPlanet;
            cProdGlob++;
        }
    }
    u = CMaxFactories(&sel.pl, idPlayer) - sel.pl.cFactories;
    if ((int16_t)u > 0) {
        rgprod[cProdGlob].cItem = LOWORD(1020 >= u ? (uint32_t)u : 1020);
        rgprod[cProdGlob].iItem = mdIdleFactory;
        rgprod[cProdGlob].grobj = grobjPlanet;
        cProdGlob++;
    }
    u = CMaxMines(&sel.pl, idPlayer) - sel.pl.cMines;
    if ((int16_t)u > 0) {
        rgprod[cProdGlob].cItem = LOWORD(1020 >= u ? (uint32_t)u : 1020);
        rgprod[cProdGlob].iItem = mdIdleMine;
        rgprod[cProdGlob].grobj = grobjPlanet;
        cProdGlob++;
    }
    u = CMaxDefenses(&sel.pl, idPlayer) - sel.pl.cDefenses;
    if ((int16_t)u > 0) {
        rgprod[cProdGlob].cItem = u;
        rgprod[cProdGlob].iItem = mdIdleDefense;
        rgprod[cProdGlob].grobj = grobjPlanet;
        cProdGlob++;
    }
    rgprod[cProdGlob].cItem = 0x3ff;
    rgprod[cProdGlob].iItem = mdIdleAlchemy;
    rgprod[cProdGlob].grobj = grobjPlanet;
    cProdGlob++;
    if (sel.pl.iScanner == 31 && GetRaceStat(&rgplr[idPlayer], rsMajorAdv) != raMacintosh) {
        rgprod[cProdGlob].cItem = 1;
        rgprod[cProdGlob].iItem = iobjPlanetaryScanner;
        rgprod[cProdGlob].grobj = grobjPlanet;
        cProdGlob++;
    }
    i = IpctCanTerraformLppl(&sel.pl);
    if (i > 0) {
        rgprod[cProdGlob].cItem = (uint16_t)i;
        rgprod[cProdGlob].iItem = mdIdleTerraform;
        rgprod[cProdGlob].grobj = grobjPlanet;
        cProdGlob++;
    }
    for (i = 0; i < 7; i++) {
        if (GetRaceStat(&rgplr[idPlayer], rsMajorAdv) == raMacintosh && (i == 0 || i == 1 || i == 2))
            continue;
        if (GetRaceStat(&rgplr[idPlayer], rsMajorAdv) != raTerra || (i != 4 && i != 5)) {
            rgprod[cProdGlob].cItem = 0x3ff;
            rgprod[cProdGlob].iItem = LOWORD(i);
            rgprod[cProdGlob].grobj = grobjPlanet;
            cProdGlob++;
        }
    }
    ipl = 0;
    lpprod = lpplProdGlob->rgprod;
    while (ipl < lpplProdGlob->iprodMac) {
        for (iSrc = 0; iSrc < cProdGlob && ((uint32_t)pProdGlob[iSrc].grobj != lpprod->grobj || (uint32_t)pProdGlob[iSrc].iItem != lpprod->iItem); iSrc++) {
        }
        if (iSrc >= cProdGlob) {
            if (ipl + 1 < lpplProdGlob->iprodMac) {
                memcpy(lpprod, lpprod + 1, (lpplProdGlob->iprodMac - (ipl + 1)) * sizeof(PROD));
                ipl--;
            }
            lpplProdGlob->iprodMac--;
        } else {
            if (pProdGlob[iSrc].cItem < lpprod->cItem) {
                lpprod->cItem = pProdGlob[iSrc].cItem;
            }
            if (pProdGlob[iSrc].cItem != 0x3ff) {
                if (pProdGlob[iSrc].cItem < lpprod->cItem) {
                    pProdGlob[iSrc].cItem = 0;
                } else {
                    pProdGlob[iSrc].cItem -= lpprod->cItem;
                }
            }
        }
        ipl++;
        lpprod++;
    }
    if (gd.fTutorial && idPlayer == 0) {
        AdvanceTutor();
    }
    return;
}

void FinishProduction(int16_t fWrite) {
    if (fWrite) {
        FreePl((PL *)sel.pl.lpplprod);
        if (lpplProdGlob && lpplProdGlob->iprodMac == 0) {
            FreePl((PL *)lpplProdGlob);
            lpplProdGlob = NULL;
        }
        sel.pl.lpplprod = lpplProdGlob;
        lpplProdGlob = NULL;
        FLookupPlanet(idWriteBack, &sel.pl);
        FLookupPlanet(sel.pl.id, &sel.pl);
        if (!fAi) {
            FillSelProdLB();
            RedrawPlanShip(tileProductionOrOrbit);
        }
    } else {
        sel.pl.fNoResearch = gd.fNoResearchSav;
        FreePl((PL *)lpplProdGlob);
    }
    lpplProdGlob = NULL;
    if (gd.fTutorial && idPlayer == 0) {
        tutor.fProgress = TRUE;
        AdvanceTutor();
    }
    return;
}

char *PszNameProdItem(PROD *lpprod) {
    uint32_t iItem;
    int16_t  iDelta;

    iItem = lpprod->iItem;
    if (lpprod->grobj == grobjFleet) {
        if (iItem >= 16) {
            iItem -= 16;
            if (rglpshdefSB[idPlayer][iItem].fFree) {
            LBogus:
                szWork[0] = 0;
                return szWork;
            }
            strcpy(szWork, rglpshdefSB[idPlayer][iItem].hul.szClass);
            if (!sel.pl.fStarbase) {
                return szWork;
            }
            iDelta = rglpshdefSB[idPlayer][sel.pl.isb].hul.ihuldef - rglpshdefSB[idPlayer][iItem].hul.ihuldef;
            if (iDelta > 0) {
                strcat(szWork, " (downgrade)");
                return szWork;
            }
            if (iDelta >= 0) {
                return szWork;
            }
            strcat(szWork, " (upgrade)");
            return szWork;
        } else {
            if (rgshdef[iItem].fFree)
                goto LBogus;
            strcpy(szWork, rgshdef[iItem].hul.szClass);
            return szWork;
        }
    }
    if (iItem >= iobjPlanetaryScannerFirst && iItem <= iobjPlanetaryScannerLast) {
        strcpy(szWork, LpplanetaryFromId(LOWORD(iItem) - iobjPlanetaryScannerFirst)->szName);
    } else if (iItem == iobjPlanetaryScanner) {
        CchGetString(idsPlanetaryScanner, szWork);
    } else {
        CchGetString(idsMines + LOWORD(iItem), szWork);
    }
    return szWork;
}

void GetProductionCosts(PLANET *lppl, PROD *lpprod, uint32_t *rgCost, int16_t iplr, int16_t fOnlyOne) {
    uint16_t      rgCostsCur[4];
    uint32_t      iItem;
    uint16_t      rgCosts[4];
    int16_t       i;
    int16_t       j;
    SHDEF        *lpshdef;
    RaceAttribute raMajor;
    uint32_t      cItem;
    int16_t       fStarbase;
    PART          part;
    int16_t       cost;
    int16_t       chs;
    HUL          *lphulNew;
    HUL          *lphulCur;
    int16_t       costUpg;
    int16_t       costHalf;
    HUL          *lphulT;
    int16_t       rgCostsPartCur[4];
    int16_t       rgCostsPartNew[4];

    raMajor = GetRaceStat(&rgplr[lppl->iPlayer], rsMajorAdv);
    fStarbase = FALSE;
    iItem = lpprod->iItem;
    cItem = lpprod->cItem;
    if (lpprod->grobj == grobjFleet) {
        if (iItem >= iobjPacketGerm) {
            lpshdef = rglpshdefSB[iplr];
            iItem -= 16;
            fStarbase = TRUE;
        } else {
            lpshdef = rglpshdef[iplr];
        }
        if (lpshdef[iItem].fFree) {
            for (i = 0; i < 4; i++) {
                rgCost[i] = 0;
            }
            return;
        }
        GetTrueHullCost(iplr, &lpshdef[iItem].hul, rgCosts);
        if (fStarbase && lppl->fStarbase) {
            lphulCur = &rglpshdefSB[iplr][lppl->isb].hul;
            lphulNew = &lpshdef[iItem].hul;
            GetTrueHullCost(iplr, lphulCur, rgCostsCur);
            if (lphulCur->ihuldef != lphulNew->ihuldef) {
                for (i = 0; i < 4; i++) {
                    costHalf = (uint32_t)rgCosts[i] / 2;
                    costUpg = rgCosts[i] - (int16_t)rgCostsCur[i] / 2;
                    if (costHalf > costUpg) {
                        rgCosts[i] = costHalf;
                    } else {
                        rgCosts[i] = costUpg;
                    }
                }
            } else {
                lphulT = &LphuldefFromId(lphulCur->ihuldef)->hul;
                part.hs.grhst = hstNone;
                part.phul = lphulT;
                GetTruePartCost(iplr, &part, rgCostsPartCur);
                for (i = 0; i < 4; i++) {
                    rgCosts[i] -= rgCostsPartCur[i];
                }
                chs = lphulCur->chs;
                for (i = 0; i < chs; i++) {
                    if (lphulCur->rghs[i].cItem != 0 && lphulNew->rghs[i].cItem != 0) {
                        part.hs = lphulCur->rghs[i];
                        FLookupPart(&part);
                        GetTruePartCost(iplr, &part, rgCostsPartCur);
                        part.hs = lphulNew->rghs[i];
                        FLookupPart(&part);
                        GetTruePartCost(iplr, &part, rgCostsPartNew);
                        if (lphulCur->rghs[i].grhst != lphulNew->rghs[i].grhst) {
                            for (j = 0; j < 4; j++) {
                                rgCostsPartCur[j] *= lphulCur->rghs[i].cItem;
                                rgCostsPartNew[j] *= lphulNew->rghs[i].cItem;
                                cost = (int16_t)(3 * rgCostsPartNew[j]) / 10 <= rgCostsPartNew[j] - (int16_t)(7 * rgCostsPartCur[j]) / 10
                                           ? rgCostsPartNew[j] - (int16_t)(7 * rgCostsPartCur[j]) / 10
                                           : (int16_t)(3 * rgCostsPartNew[j]) / 10;
                                rgCosts[j] -= rgCosts[j] >= (uint16_t)(rgCostsPartNew[j] - cost) ? rgCostsPartNew[j] - cost : rgCosts[j];
                            }
                        } else if (lphulCur->rghs[i].iItem != lphulNew->rghs[i].iItem) {
                            for (j = 0; j < 4; j++) {
                                rgCostsPartCur[j] *= lphulCur->rghs[i].cItem;
                                rgCostsPartNew[j] *= lphulNew->rghs[i].cItem;
                                cost = (int16_t)(rgCostsPartNew[j] * 2) / 10 <= rgCostsPartNew[j] - (int16_t)(rgCostsPartCur[j] * 8) / 10
                                           ? rgCostsPartNew[j] - (int16_t)(rgCostsPartCur[j] * 8) / 10
                                           : (int16_t)(rgCostsPartNew[j] * 2) / 10;
                                rgCosts[j] -= rgCosts[j] >= (uint16_t)(rgCostsPartNew[j] - cost) ? rgCostsPartNew[j] - cost : rgCosts[j];
                            }
                        } else {
                            for (j = 0; j < 4; j++) {
                                rgCostsPartCur[j] *= lphulCur->rghs[i].cItem;
                                rgCostsPartNew[j] *= lphulNew->rghs[i].cItem;
                                cost = 0 <= rgCostsPartNew[j] - rgCostsPartCur[j] ? rgCostsPartNew[j] - rgCostsPartCur[j] : 0;
                                rgCosts[j] -= rgCosts[j] >= (uint16_t)(rgCostsPartNew[j] - cost) ? rgCostsPartNew[j] - cost : rgCosts[j];
                            }
                        }
                    }
                }
            }
        }
        if (fStarbase && (GetRaceGrbit(&rgplr[iplr], ibitRaceISB) != 0 || GetRaceStat(&rgplr[iplr], rsMajorAdv) == raMacintosh)) {
            for (i = 0; i < 4; i++) {
                rgCosts[i] -= (uint32_t)rgCosts[i] / 5;
            }
        }
        if (fStarbase) {
            for (i = 0; i < 4; i++) {
                rgCost[i] = (uint32_t)((uint32_t)(rgCosts[i] + 1) / 2);
            }
        } else {
            for (i = 0; i < 4; i++) {
                rgCost[i] = (uint32_t)rgCosts[i];
            }
        }
    } else {
        switch (iItem) {
        case iobjFactory:
        case mdIdleFactory:
            cost = GetRaceGrbit(&rgplr[iplr], ibitRaceCheapFact);
            if (gd.fTutorial) {
                for (i = 0; i < 3; i++) {
                    rgCost[i] = (int16_t)(2 - cost);
                }
            } else {
                rgCost[1] = 0;
                *rgCost = 0;
                rgCost[2] = (int16_t)(4 - cost);
            }
            rgCost[3] = GetRaceStat(&rgplr[iplr], rsFactBuild);
            break;
        case iobjMine:
        case mdIdleMine:
            for (i = 0; i < 3; i++) {
                rgCost[i] = 0;
            }
            rgCost[3] = GetRaceStat(&rgplr[iplr], rsMineBuild);
            break;
        case iobjDefense:
        case mdIdleDefense:
            part.hs.grhst = hstPlanetary;
            part.hs.iItem = iplanetarySDI;
            FLookupPart(&part);
            for (i = 0; i < 3; i++) {
                rgCost[i] = part.pplanetary->rgwtOreCost[i];
            }
            rgCost[3] = (uint32_t)part.pplanetary->resCost;
            if (GetRaceStat(&rgplr[iplr], rsMajorAdv) != raDefend)
                break;
            for (i = 0; i < 4; i++) {
                rgCost[i] = (uint32_t)((uint32_t)(rgCost[i] * 3) / 5);
            }
            break;
        case iobjAlchemy:
        case mdIdleAlchemy:
            for (i = 0; i < 3; i++) {
                rgCost[i] = 0;
            }
            rgCost[3] = (uint32_t)(GetRaceGrbit(&rgplr[iplr], ibitRaceMineralAlchemy) == 0 ? 100 : 25);
            break;
        case iobjGenesis:
            part.hs.grhst = hstPlanetary;
            part.hs.iItem = iplanetaryGenesisDevice;
            FLookupPart(&part);
            GetTruePartCost(iplr, &part, rgCosts);
            for (i = 0; i < 4; i++) {
                rgCost[i] = (uint32_t)rgCosts[i];
            }
            break;
        case iobjPacketIron:
        case iobjPacketBor:
        case iobjPacketGerm:
            j = raMajor == raMassAccel ? 70 : raMajor == raStargate ? 120 : 110;
            for (i = 0; i < 3; i++) {
                rgCost[i] = iItem - 14 == (uint32_t)i ? j : 0;
            }
            rgCost[i] = (uint32_t)(raMajor == raMassAccel ? 5 : 10);
            break;
        case iobjPacket:
        case iobjPacketMixed:
            j = raMajor == raMassAccel ? 25 : raMajor == raStargate ? 48 : 44;
            for (i = 0; i < 3; i++) {
                rgCost[i] = j;
            }
            rgCost[i] = (uint32_t)(raMajor == raMassAccel ? 5 : 10);
            break;
        case iobjMinTerraform:
        case iobjMaxTerraform:
        case mdIdleTerraform:
            rgCost[2] = 0;
            rgCost[1] = 0;
            *rgCost = 0;
            if (GetRaceGrbit(&rgplr[iplr], ibitRaceTT) != 0) {
                rgCost[3] = 70;
            } else {
                rgCost[3] = 100;
            }
            if (raMajor != raTerra)
                break;
            rgCost[3] = (uint32_t)(rgCost[3] / 2);
            break;
        case iobjPlanetaryScanner:
            iItem = iobjPlanetaryScannerFirst;
            /* fallthrough */
        case iobjPlanetaryScannerFirst:
        case iobjPlanetaryScannerViewer90:
        case iobjPlanetaryScannerScoper150:
        case iobjPlanetaryScannerScoper220:
        case iobjPlanetaryScannerScoper280:
        case iobjPlanetaryScannerSnooper320X:
        case iobjPlanetaryScannerSnooper400X:
        case iobjPlanetaryScannerSnooper500X:
        case iobjPlanetaryScannerSnooper620X:
            part.hs.grhst = hstPlanetary;
            part.hs.iItem = LOWORD(iItem) - 18;
            FLookupPart(&part);
            GetTruePartCost(iplr, &part, rgCosts);
            for (i = 0; i < 4; i++) {
                rgCost[i] = (uint32_t)rgCosts[i];
            }
        }
    }
    if (!fOnlyOne) {
        for (i = 0; i < 4; i++) {
            rgCost[i] = (uint32_t)(rgCost[i] * cItem);
        }
    }
    return;
}

void EstimateItemProdSched(PLANET *lppl, PLPROD *lpplprod, ProdItemType iItem, int16_t *piFirst, int16_t *piLast) {
    int32_t    cResearch;
    PLANET     pl;
    int32_t    rglQuan[3];
    int16_t    cBuilt;
    PROD       prodPartial;
    mdProdStat mdStatus;
    int16_t    i;
    int16_t    j;
    int16_t    iPass;
    int16_t    fAlchemy;
    int16_t    iMac;
    int32_t    rgRes[4];
    PROD      *lpprod;

    if (!lpplprod) {
        lpplprod = lppl->lpplprod;
    }
    pl = *lppl;
    pl.lpplprod = (PLPROD *)LpplAlloc(4, lpplprod->iprodMax, htOrd);
    memcpy(pl.lpplprod->rgprod, lpplprod->rgprod, lpplprod->iprodMac * 4);
    pl.lpplprod->iprodMac = lpplprod->iprodMac;
    iMac = lpplprod->iprodMac;
    prodPartial.cItem = 0;
    *piLast = 0;
    *piFirst = 0;
    for (iPass = 1; iPass < 100; iPass++) {
        EstMineralsMined(&pl, rglQuan, -1, TRUE);
        for (j = 0; j < 3; j++) {
            rgRes[j] = pl.rgwtMin[j];
        }
        rgRes[3] = CResourcesAtPlanet(&pl, lppl->iPlayer);
        if (!pl.fNoResearch) {
            cResearch = (int32_t)(rgRes[3] * (int16_t)rgplr[lppl->iPlayer].pctResearch) / 100;
            rgRes[3] -= cResearch;
        } else {
            cResearch = 0;
        }
        fAlchemy = FALSE;
        for (i = -1; i < iMac; i++) {
            if (i == -1) {
                lpprod = &prodPartial;
            } else {
                lpprod = &pl.lpplprod->rgprod[i];
            }
            if (lpprod->cItem != 0) {
                if (lpprod->iItem == iobjAlchemy && lpprod->grobj == grobjPlanet) {
                    if (i < iMac - 1) {
                        if (i == iItem) {
                            *piLast = -1;
                            *piFirst = -1;
                            goto LCleanUp;
                        }
                        fAlchemy = TRUE;
                        continue;
                    }
                    lpprod->cItem = 1020;
                }
                cBuilt = CBuildProdItem(&pl, lpprod, i == -1 ? NULL : &prodPartial, rgRes, fAlchemy, (int16_t *)&mdStatus, FALSE);
                if (iItem == i) {
                    if (cBuilt > 0 && *piFirst == 0) {
                        *piFirst = iPass;
                    }
                    if (mdStatus == mdProdStatSkippedAuto) {
                        if (*piFirst != 0) {
                            *piLast = iPass - 1;
                        }
                        goto LCleanUp;
                    }
                    if (mdStatus == mdProdStatComplete || mdStatus == mdProdStatCompleteAuto) {
                        *piLast = iPass;
                        goto LCleanUp;
                    }
                }
                fAlchemy = FALSE;
                if (lpprod->grobj == grobjPlanet) {
                    switch (lpprod->iItem) {
                    case iobjMine:
                    case mdIdleMine:
                        pl.cMines += cBuilt;
                        break;
                    case iobjFactory:
                    case mdIdleFactory:
                        pl.cFactories += cBuilt;
                    }
                }
                if ((int16_t)mdStatus >= mdProdStatSome)
                    break;
            }
        }
        if ((int16_t)iItem < iobjMine) {
            *piFirst = LOWORD(rgRes[3]);
            if (iItem == iprodEstimateResearchResources) {
                *piFirst += LOWORD(cResearch);
            }
            goto LCleanUp;
        }
        for (j = 0; j < 3; j++) {
            pl.rgwtMin[j] = rgRes[j];
        }
        ChgPopFromPlanet(&pl, TRUE);
    }
    if (*piFirst == 0) {
        *piFirst = 100;
    }
    *piLast = 100;
LCleanUp:
    if (pl.lpplprod) {
        FreePl((PL *)pl.lpplprod);
    }
    return;
}
