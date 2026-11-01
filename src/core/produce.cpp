#include "common.h"
#include "appstate.h"
#include "presentation_io.h"
#include "production_io.h"
#include "colony_io.h"
#include "planet_query_io.h"
#include "research_io.h"
#include "part_query_io.h"
#include "race_query_io.h"
#include "text_io.h"

void InitProduction(APPSESSION &app, PROD *rgprod) {
    int16_t  iWarp;
    int16_t  iSrc;
    uint16_t u;
    int16_t  i;
    int16_t  ipl;
    PART     part;
    PROD    *lpprod;

    app.state.gd.fNoResearchSav = app.state.sel.pl.fNoResearch;
    if (!rgprod) {
        rgprod = app.state.pProdGlob;
    }
    if (app.state.sel.pl.lpplprod) {
        i = app.state.sel.pl.lpplprod->iprodMac;
    } else {
        i = 2;
    }
    app.state.lpplProdGlob = LpplAlloc<PLPROD>(app, 4, i, htOrd);
    if (app.state.sel.pl.lpplprod) {
        HeapIO().pfnCopy(app.state.lpplProdGlob->rgprod(), app.state.sel.pl.lpplprod->rgprod(), i * 4);
    } else {
        i = 0;
    }
    app.state.lpplProdGlob->iprodMac = i;
    app.state.cProdGlob = 0;
    app.state.pProdGlob = rgprod;
    memset(rgprod, 0, 64 * sizeof(PROD));
    if (app.state.sel.pl.fStarbase && PartQueryIO().pfnHull(app.state.rglpshdefSB[app.state.idPlayer][app.state.sel.pl.isb].hul.ihuldef)->hul.wtCargoMax != 0) {
        for (i = 0; i < 16; i++) {
            if (!app.state.rgshdef[i].fFree && !app.state.rgshdef[i].fGift) {
                if (PartQueryIO().pfnHull(app.state.rglpshdefSB[app.state.idPlayer][app.state.sel.pl.isb].hul.ihuldef)->hul.wtCargoMax >= app.state.rgshdef[i].hul.wtEmpty) {
                    rgprod[app.state.cProdGlob].cItem = 0x3ff;
                    rgprod[app.state.cProdGlob].iItem = i;
                    rgprod[app.state.cProdGlob].grobj = grobjFleet;
                    app.state.cProdGlob++;
                }
            }
        }
    }
    for (i = 0; i < 10; i++) {
        if (!app.state.rglpshdefSB[app.state.idPlayer][i].fFree && !app.state.rglpshdefSB[app.state.idPlayer][i].fGift && (app.state.sel.pl.isb != i || !app.state.sel.pl.fStarbase)) {
            rgprod[app.state.cProdGlob].cItem = 1;
            rgprod[app.state.cProdGlob].iItem = LOWORD((int16_t)(i + 16));
            rgprod[app.state.cProdGlob].grobj = grobjFleet;
            app.state.cProdGlob++;
        }
    }
    part.hs.grhst = hstPlanetary;
    part.hs.iItem = iplanetaryGenesisDevice;
    if (PartQueryIO().pfnLookup(app, &part) == mdPartAvailAvailable) {
        rgprod[app.state.cProdGlob].cItem = 1;
        rgprod[app.state.cProdGlob].iItem = iobjGenesis;
        rgprod[app.state.cProdGlob].grobj = grobjPlanet;
        app.state.cProdGlob++;
    }
    iWarp = ColonyIO().pfnWarp(app, &app.state.sel.pl, NULL);
    if (iWarp > 0) {
        for (i = 0; i < 4; i++) {
            rgprod[app.state.cProdGlob].cItem = 0x3ff;
            rgprod[app.state.cProdGlob].iItem = LOWORD((int16_t)(i + 14));
            rgprod[app.state.cProdGlob].grobj = grobjPlanet;
            app.state.cProdGlob++;
        }
    }
    u = ColonyIO().pfnMaxFactories(app, &app.state.sel.pl, app.state.idPlayer) - app.state.sel.pl.cFactories;
    if ((int16_t)u > 0) {
        rgprod[app.state.cProdGlob].cItem = LOWORD(1020 >= u ? (uint32_t)u : 1020);
        rgprod[app.state.cProdGlob].iItem = mdIdleFactory;
        rgprod[app.state.cProdGlob].grobj = grobjPlanet;
        app.state.cProdGlob++;
    }
    u = ColonyIO().pfnMaxMines(app, &app.state.sel.pl, app.state.idPlayer) - app.state.sel.pl.cMines;
    if ((int16_t)u > 0) {
        rgprod[app.state.cProdGlob].cItem = LOWORD(1020 >= u ? (uint32_t)u : 1020);
        rgprod[app.state.cProdGlob].iItem = mdIdleMine;
        rgprod[app.state.cProdGlob].grobj = grobjPlanet;
        app.state.cProdGlob++;
    }
    u = ColonyIO().pfnMaxDefenses(app, &app.state.sel.pl, app.state.idPlayer) - app.state.sel.pl.cDefenses;
    if ((int16_t)u > 0) {
        rgprod[app.state.cProdGlob].cItem = u;
        rgprod[app.state.cProdGlob].iItem = mdIdleDefense;
        rgprod[app.state.cProdGlob].grobj = grobjPlanet;
        app.state.cProdGlob++;
    }
    rgprod[app.state.cProdGlob].cItem = 0x3ff;
    rgprod[app.state.cProdGlob].iItem = mdIdleAlchemy;
    rgprod[app.state.cProdGlob].grobj = grobjPlanet;
    app.state.cProdGlob++;
    if (app.state.sel.pl.iScanner == 31 && RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) != raMacintosh) {
        rgprod[app.state.cProdGlob].cItem = 1;
        rgprod[app.state.cProdGlob].iItem = iobjPlanetaryScanner;
        rgprod[app.state.cProdGlob].grobj = grobjPlanet;
        app.state.cProdGlob++;
    }
    i = ColonyIO().pfnTerraformPercent(app, &app.state.sel.pl);
    if (i > 0) {
        rgprod[app.state.cProdGlob].cItem = (uint16_t)i;
        rgprod[app.state.cProdGlob].iItem = mdIdleTerraform;
        rgprod[app.state.cProdGlob].grobj = grobjPlanet;
        app.state.cProdGlob++;
    }
    for (i = 0; i < 7; i++) {
        if (RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) == raMacintosh && (i == 0 || i == 1 || i == 2))
            continue;
        if (RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv) != raTerra || (i != 4 && i != 5)) {
            rgprod[app.state.cProdGlob].cItem = 0x3ff;
            rgprod[app.state.cProdGlob].iItem = LOWORD(i);
            rgprod[app.state.cProdGlob].grobj = grobjPlanet;
            app.state.cProdGlob++;
        }
    }
    ipl = 0;
    lpprod = app.state.lpplProdGlob->rgprod();
    while (ipl < app.state.lpplProdGlob->iprodMac) {
        for (iSrc = 0; iSrc < app.state.cProdGlob && ((uint32_t)app.state.pProdGlob[iSrc].grobj != lpprod->grobj || (uint32_t)app.state.pProdGlob[iSrc].iItem != lpprod->iItem); iSrc++) {
        }
        if (iSrc >= app.state.cProdGlob) {
            if (ipl + 1 < app.state.lpplProdGlob->iprodMac) {
                HeapIO().pfnCopy(lpprod, lpprod + 1, (app.state.lpplProdGlob->iprodMac - (ipl + 1)) * sizeof(PROD));
                ipl--;
            }
            app.state.lpplProdGlob->iprodMac--;
        } else {
            if (app.state.pProdGlob[iSrc].cItem < lpprod->cItem) {
                lpprod->cItem = app.state.pProdGlob[iSrc].cItem;
            }
            if (app.state.pProdGlob[iSrc].cItem != 0x3ff) {
                if (app.state.pProdGlob[iSrc].cItem < lpprod->cItem) {
                    app.state.pProdGlob[iSrc].cItem = 0;
                } else {
                    app.state.pProdGlob[iSrc].cItem -= lpprod->cItem;
                }
            }
        }
        ipl++;
        lpprod++;
    }
    if (app.state.gd.fTutorial && app.state.idPlayer == 0) {
        PresentationIO().pfnTutor(app);
    }
    return;
}

void FinishProduction(APPSESSION &app, int16_t fWrite) {
    if (fWrite) {
        FreePl(app, app.state.sel.pl.lpplprod);
        if (app.state.lpplProdGlob && app.state.lpplProdGlob->iprodMac == 0) {
            FreePl(app, app.state.lpplProdGlob);
            app.state.lpplProdGlob = NULL;
        }
        app.state.sel.pl.lpplprod = app.state.lpplProdGlob;
        app.state.lpplProdGlob = NULL;
        PlanetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.pl);
        PlanetQueryIO().pfnLookup(app, app.state.sel.pl.id, &app.state.sel.pl);
        if (!app.state.fAi) {
            ProductionIO().pfnRefresh(app);
            PresentationIO().pfnRedraw(app, tileProductionOrOrbit);
        }
    } else {
        app.state.sel.pl.fNoResearch = app.state.gd.fNoResearchSav;
        FreePl(app, app.state.lpplProdGlob);
    }
    app.state.lpplProdGlob = NULL;
    if (app.state.gd.fTutorial && app.state.idPlayer == 0) {
        app.state.tutor.fProgress = 1;
        PresentationIO().pfnTutor(app);
    }
    return;
}

char *PszNameProdItem(APPSESSION &app, PROD *lpprod) {
    uint32_t iItem;
    int16_t  iDelta;

    iItem = lpprod->iItem;
    if (lpprod->grobj == grobjFleet) {
        if (iItem >= 16) {
            iItem -= 16;
            if (app.state.rglpshdefSB[app.state.idPlayer][iItem].fFree) {
            LBogus:
                app.state.szWork[0] = 0;
                return app.state.szWork;
            }
            strcpy(app.state.szWork, app.state.rglpshdefSB[app.state.idPlayer][iItem].hul.szClass);
            if (!app.state.sel.pl.fStarbase) {
                return app.state.szWork;
            }
            iDelta = app.state.rglpshdefSB[app.state.idPlayer][app.state.sel.pl.isb].hul.ihuldef - app.state.rglpshdefSB[app.state.idPlayer][iItem].hul.ihuldef;
            if (iDelta > 0) {
                strcat(app.state.szWork, " (downgrade)");
                return app.state.szWork;
            }
            if (iDelta >= 0) {
                return app.state.szWork;
            }
            strcat(app.state.szWork, " (upgrade)");
            return app.state.szWork;
        } else {
            if (app.state.rgshdef[iItem].fFree)
                goto LBogus;
            strcpy(app.state.szWork, app.state.rgshdef[iItem].hul.szClass);
            return app.state.szWork;
        }
    }
    if (iItem >= iobjPlanetaryScannerFirst && iItem <= iobjPlanetaryScannerLast) {
        strcpy(app.state.szWork, LpplanetaryFromId(LOWORD(iItem) - iobjPlanetaryScannerFirst)->szName);
    } else if (iItem == iobjPlanetaryScanner) {
        TextIO().pfnCchGetString(app, idsPlanetaryScanner, app.state.szWork);
    } else {
        TextIO().pfnCchGetString(app, static_cast<StringId>(idsMines + LOWORD(iItem)), app.state.szWork);
    }
    return app.state.szWork;
}

void GetProductionCosts(APPSESSION &app, PLANET *lppl, PROD *lpprod, uint32_t *rgCost, int16_t iplr, int16_t fOnlyOne) {
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

    raMajor = static_cast<RaceAttribute>(RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMajorAdv));
    fStarbase = 0;
    iItem = lpprod->iItem;
    cItem = lpprod->cItem;
    if (lpprod->grobj == grobjFleet) {
        if (iItem >= iobjPacketGerm) {
            lpshdef = app.state.rglpshdefSB[iplr];
            iItem -= 16;
            fStarbase = 1;
        } else {
            lpshdef = app.state.rglpshdef[iplr];
        }
        if (lpshdef[iItem].fFree) {
            for (i = 0; i < 4; i++) {
                rgCost[i] = 0;
            }
            return;
        }
        GetTrueHullCost(iplr, &lpshdef[iItem].hul, rgCosts);
        if (fStarbase && lppl->fStarbase) {
            lphulCur = &app.state.rglpshdefSB[iplr][lppl->isb].hul;
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
                lphulT = &PartQueryIO().pfnHull(lphulCur->ihuldef)->hul;
                part.hs.grhst = hstNone;
                part.SetPhul(lphulT);
                PartQueryIO().pfnTrueCost(app, iplr, &part, (uint16_t *)rgCostsPartCur);
                for (i = 0; i < 4; i++) {
                    rgCosts[i] -= rgCostsPartCur[i];
                }
                chs = lphulCur->chs;
                for (i = 0; i < chs; i++) {
                    if (lphulCur->rghs[i].cItem != 0 && lphulNew->rghs[i].cItem != 0) {
                        part.hs = lphulCur->rghs[i];
                        PartQueryIO().pfnLookup(app, &part);
                        PartQueryIO().pfnTrueCost(app, iplr, &part, (uint16_t *)rgCostsPartCur);
                        part.hs = lphulNew->rghs[i];
                        PartQueryIO().pfnLookup(app, &part);
                        PartQueryIO().pfnTrueCost(app, iplr, &part, (uint16_t *)rgCostsPartNew);
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
        if (fStarbase && (RaceQueryIO().pfnGrbit(&app.state.rgplr[iplr], ibitRaceISB) != 0 || RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMajorAdv) == raMacintosh)) {
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
            cost = RaceQueryIO().pfnGrbit(&app.state.rgplr[iplr], ibitRaceCheapFact);
            if (app.state.gd.fTutorial) {
                for (i = 0; i < 3; i++) {
                    rgCost[i] = (int16_t)(2 - cost);
                }
            } else {
                rgCost[1] = 0;
                *rgCost = 0;
                rgCost[2] = (int16_t)(4 - cost);
            }
            rgCost[3] = RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsFactBuild);
            break;
        case iobjMine:
        case mdIdleMine:
            for (i = 0; i < 3; i++) {
                rgCost[i] = 0;
            }
            rgCost[3] = RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMineBuild);
            break;
        case iobjDefense:
        case mdIdleDefense:
            part.hs.grhst = hstPlanetary;
            part.hs.iItem = iplanetarySDI;
            PartQueryIO().pfnLookup(app, &part);
            for (i = 0; i < 3; i++) {
                rgCost[i] = part.pplanetary()->rgwtOreCost[i];
            }
            rgCost[3] = (uint32_t)part.pplanetary()->resCost;
            if (RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMajorAdv) != raDefend)
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
            rgCost[3] = (uint32_t)(RaceQueryIO().pfnGrbit(&app.state.rgplr[iplr], ibitRaceMineralAlchemy) == 0 ? 100 : 25);
            break;
        case iobjGenesis:
            part.hs.grhst = hstPlanetary;
            part.hs.iItem = iplanetaryGenesisDevice;
            PartQueryIO().pfnLookup(app, &part);
            PartQueryIO().pfnTrueCost(app, iplr, &part, rgCosts);
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
            if (RaceQueryIO().pfnGrbit(&app.state.rgplr[iplr], ibitRaceTT) != 0) {
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
            PartQueryIO().pfnLookup(app, &part);
            PartQueryIO().pfnTrueCost(app, iplr, &part, rgCosts);
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

void EstimateItemProdSched(APPSESSION &app, PLANET *lppl, PLPROD *lpplprod, ProdItemType iItem, int16_t *piFirst, int16_t *piLast) {
    int32_t    cResearch;
    PLANET     pl;
    int32_t    rglQuan[3];
    int16_t    cBuilt;
    PROD       prodPartial;
    mdProdStat mdStatus;
    int16_t    mdStatusRaw;
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
    pl.lpplprod = LpplAlloc<PLPROD>(app, 4, lpplprod->iprodMax, htOrd);
    HeapIO().pfnCopy(pl.lpplprod->rgprod(), lpplprod->rgprod(), lpplprod->iprodMac * 4);
    pl.lpplprod->iprodMac = lpplprod->iprodMac;
    iMac = lpplprod->iprodMac;
    prodPartial.cItem = 0;
    *piLast = 0;
    *piFirst = 0;
    for (iPass = 1; iPass < 100; iPass++) {
        ColonyIO().pfnEstimateMining(app, &pl, rglQuan, -1, 1);
        for (j = 0; j < 3; j++) {
            rgRes[j] = pl.rgwtMin[j];
        }
        rgRes[3] = ResearchIO().pfnResources(app, &pl, lppl->iPlayer);
        if (!pl.fNoResearch) {
            cResearch = (int32_t)(rgRes[3] * (int16_t)app.state.rgplr[lppl->iPlayer].pctResearch) / 100;
            rgRes[3] -= cResearch;
        } else {
            cResearch = 0;
        }
        fAlchemy = 0;
        for (i = -1; i < iMac; i++) {
            if (i == -1) {
                lpprod = &prodPartial;
            } else {
                lpprod = &pl.lpplprod->rgprod()[i];
            }
            if (lpprod->cItem != 0) {
                if (lpprod->iItem == iobjAlchemy && lpprod->grobj == grobjPlanet) {
                    if (i < iMac - 1) {
                        if (i == iItem) {
                            *piLast = -1;
                            *piFirst = -1;
                            goto LCleanUp;
                        }
                        fAlchemy = 1;
                        continue;
                    }
                    lpprod->cItem = 1020;
                }
                cBuilt = ProductionIO().pfnBuild(app, &pl, lpprod, i == -1 ? NULL : &prodPartial, rgRes, fAlchemy, &mdStatusRaw, 0);
                mdStatus = static_cast<mdProdStat>(mdStatusRaw);
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
                fAlchemy = 0;
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
            if (iItem == static_cast<ProdItemType>(iprodEstimateResearchResources)) {
                *piFirst += LOWORD(cResearch);
            }
            goto LCleanUp;
        }
        for (j = 0; j < 3; j++) {
            pl.rgwtMin[j] = rgRes[j];
        }
        PlanetQueryIO().pfnGrowth(app, &pl, 1);
    }
    if (*piFirst == 0) {
        *piFirst = 100;
    }
    *piLast = 100;
LCleanUp:
    if (pl.lpplprod) {
        FreePl(app, pl.lpplprod);
    }
    return;
}
