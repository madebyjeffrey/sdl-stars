#include "common.h"
#include "appstate.h"
#include "fleet_query_io.h"
#include "text_io.h"
#include "presentation_io.h"
#include "colony_io.h"
#include "planet_query_io.h"
#include "research_io.h"
#include "part_query_io.h"
#include "race_query_io.h"

int16_t FGetBestDefensePart(APPSESSION &app, PART *ppart) {
    int16_t fRet;
    int16_t i;
    PART    part;

    fRet = 1;
    part.hs.grhst = hstPlanetary;
    part.hs.iItem = iplanetarySDI;
    i = 0;
    while (i < 5 && PartQueryIO().pfnLookup(app, &part) == mdPartAvailAvailable) {
        i++;
        part.hs.iItem++;
    }
    if (i > 0) {
        i--;
    } else {
        fRet = 0;
    }
    part.hs.iItem = i + 9;
    PartQueryIO().pfnLookup(app, &part);
    *ppart = part;
    return fRet;
}

char *PszProductionETA(APPSESSION &app, PLANET *lppl, PLPROD *lpplprod, ProdItemType iItem, int16_t *etaFirst, int16_t *etaLast) {
    int16_t  iTurnEnd;
    int16_t  iTurnBegin;
    int16_t  c;
    StringId ids;

    if (!lpplprod) {
        lpplprod = lppl->lpplprod;
    }
    ResearchIO().pfnEstimate(app, lppl, lpplprod, iItem, &iTurnBegin, &iTurnEnd);
    if (iTurnBegin == 100) {
        if (lpplprod && lpplprod->iprodMac > (int16_t)iItem && lpplprod->rgprod()[iItem].grobj == grobjPlanet && lpplprod->rgprod()[iItem].iItem < mdIdleFactory) {
            ids = idsUnknown2;
        } else {
            ids = idsNever;
        }
        c = TextIO().pfnCchGetString(app, ids, app.state.szWork);
    } else if (iTurnEnd == 100) {
        c = CchSprintf(app.state.szWork, TextIO().pfnCompressed(app, idsDYears), iTurnBegin);
    } else if (iTurnBegin == iTurnEnd) {
        if (iTurnBegin == 0) {
            c = TextIO().pfnCchGetString(app, idsSkipped, app.state.szWork);
        } else if (iTurnBegin == -1) {
            c = TextIO().pfnCchGetString(app, idsNeeded, app.state.szWork);
        } else {
            c = CchSprintf(app.state.szWork, TextIO().pfnCompressed(app, idsDYear), iTurnBegin);
            if (iTurnBegin != 1) {
                app.state.szWork[c] = 's';
                c++;
                app.state.szWork[c] = 0;
            }
        }
    } else {
        c = CchSprintf(app.state.szWork, TextIO().pfnCompressed(app, idsDDYears), iTurnBegin, iTurnEnd);
    }
    if (etaFirst) {
        *etaFirst = iTurnBegin;
    }
    if (etaLast) {
        *etaLast = iTurnEnd;
    }
    return app.state.szWork;
}

void ChangeMainObjSel(APPSESSION &app, GrobjClass grobjNew, int16_t iObjSel) {
    int16_t fSameType;
    int16_t idSkip;
    int16_t i;
    FLEET  *lpfl;

    idSkip = idflNone;
    fSameType = grobjNew == app.state.sel.grobj;
    if (!app.state.fAi || !fSameType || iObjSel != app.state.sel.id) {
        PresentationIO().pfnInvalidateReport(app, app.state.sel.grobj == grobjPlanet ? rptPlanets : rptFleets, 0);
        if (grobjNew == grobjPlanet) {
            PresentationIO().pfnInvalidateReport(app, rptPlanets, 0);
            if (!PlanetQueryIO().pfnLookup(app, iObjSel, &app.state.sel.pl)) {
                return;
            }
            app.state.sel.pt = app.state.rgptPlan[iObjSel];
            app.state.sel.scan.iwp = iwpNone;
            app.state.sel.iwpAct = iwpNone;
            for (i = 0; i < app.state.cFleet; i++) {
                lpfl = app.state.rglpfl[i];
                if (!app.state.rglpfl[i] || (lpfl->idPlanet == iObjSel && lpfl->iPlayer == app.state.idPlayer))
                    break;
            }
            if (i != app.state.cFleet) {
                FDupFleet(app, lpfl, &app.state.sel.fl);
                app.state.sel.grobjFull = static_cast<GrobjClass>(grobjPlanet | grobjFleet);
            } else {
                app.state.sel.fl.id = idflNone;
                app.state.sel.grobjFull = grobjPlanet;
            }
            if (!app.state.fAi) {
                ShowPlanetSel(app);
            }
        } else {
            PresentationIO().pfnInvalidateReport(app, rptFleets, 0);
            if (!FleetQueryIO().pfnLookup(app, iObjSel, &app.state.sel.fl)) {
                return;
            }
            app.state.sel.pt = app.state.sel.fl.pt;
            if (app.state.sel.fl.idPlanet == idPlanetDeepSpace || (app.state.sel.fl.idPlanet != app.state.sel.pl.id && !PlanetQueryIO().pfnLookup(app, app.state.sel.fl.idPlanet, &app.state.sel.pl))) {
                app.state.sel.pl.id = idplNone;
            }
            app.state.sel.grobjFull = static_cast<GrobjClass>((app.state.sel.pl.id != idplNone) | 2);
            app.state.sel.iwpAct = 0;
            if (!app.state.fAi) {
                ShowFleetSel(app);
                idSkip = iObjSel;
            }
        }
        app.state.sel.grobj = grobjNew;
        app.state.sel.id = iObjSel;
        app.state.gd.fSetMassMode = 0;
        app.state.gd.fSetRouteMode = 0;
        if (!app.state.fAi) {
            ShowMainObjSel(app, fSameType, idSkip);
        }
    }
    return;
}

void SelectAdjPlanet(APPSESSION &app, int16_t dInc, int16_t idPlanet) {
    PLANET *lpPlT;
    int16_t i;
    PLANET *lpPl;
    SCAN    scan;
    int16_t fWrap;

    fWrap = 0;
    if (app.state.cPlanet > 0 && idPlanet != idplNone) {
        if (dInc != 0) {
            idPlanet = app.state.sel.pl.id;
        }
        if (!app.state.vrptPlanet.fCached) {
            PresentationIO().pfnInvalidateReport(app, rptPlanets, 1);
        }
        lpPlT = app.state.lpPlanets;
        lpPl = app.state.lpPlanets;
        i = 0;
        for (; i < app.state.cPlanet && lpPl->id != idPlanet; lpPl++) {
            i++;
        }
        if (i == app.state.cPlanet || lpPl->det != detAll) {
            scan.pt = app.state.rgptPlan[idPlanet];
            scan.grobj = static_cast<GrobjClass>(grobjPlanet | mdExact);
            ChangeScanSel(app, &scan, 0);
            goto FinishUp;
        }
        if (dInc != 0) {
            for (i = 0; i < app.state.rgplr[app.state.idPlayer].cPlanet && lpPlT[app.state.vlprgidPlanet[i]].id != idPlanet; i++) {
            }
            i += dInc;
            if (i >= app.state.rgplr[app.state.idPlayer].cPlanet) {
                i = 0;
            } else if (i < 0) {
                i = app.state.rgplr[app.state.idPlayer].cPlanet - 1;
            }
            i = app.state.vlprgidPlanet[i];
        }
        lpPlT += i;
        idPlanet = lpPlT->id;
        if (lpPlT->iPlayer != app.state.idPlayer) {
            return;
        }
        scan.pt = app.state.rgptPlan[idPlanet];
        scan.grobj = static_cast<GrobjClass>(grobjPlanet | mdExact);
        ChangeScanSel(app, &scan, 0);
        ShowScanSel(app, 0);
        ChangeMainObjSel(app, grobjPlanet, idPlanet);
        ShowScanSel(app, 1);
    FinishUp:
        ShowSelAt(app, app.state.rgptPlan[idPlanet]);
    }
    return;
}

int16_t IdFindAdjStarbase(APPSESSION &app, int16_t idPlanet, int16_t fNext) {
    PLANET *lpplMac;
    int16_t idLast;
    int16_t idFirst;
    PLANET *lppl;
    int16_t idAfter;
    int16_t idBefore;

    idLast = idplNone;
    idFirst = idplNone;
    idAfter = idplNone;
    idBefore = idplNone;
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == app.state.idPlayer && lppl->fStarbase && PartQueryIO().pfnHull(app.state.rglpshdefSB[app.state.idPlayer][lppl->isb].hul.ihuldef)->hul.wtCargoMax != 0) {
            if (lppl->id > idPlanet) {
                if (idAfter == idplNone) {
                    idAfter = lppl->id;
                }
            } else if (lppl->id < idPlanet) {
                idBefore = lppl->id;
            }
            if (idFirst == idplNone) {
                idFirst = lppl->id;
            }
            idLast = lppl->id;
        }
    }
    if (fNext) {
        if (idAfter == idplNone) {
            return idFirst;
        }
        return idAfter;
    }
    if (idBefore == idplNone) {
        return idLast;
    }
    return idBefore;
}

int16_t IBestTerraform(APPSESSION &app, PLANET *lppl, int16_t fHelp) {
    int16_t iSave;
    int16_t iBest;
    int16_t rgMax[3];
    int16_t pctT;
    int16_t i;
    int16_t iPlr;
    int16_t iEnv;
    int16_t pctCur;
    int16_t rgMin[3];
    int16_t rgpctBest[3];
    int16_t rgCost[3];
    int16_t iPlrSav;

    iPlrSav = app.state.idPlayer;
    iPlr = lppl->iPlayer;
    if (iPlr == iplrNone) {
        return 0;
    }
    app.state.idPlayer = iPlr;
    if (!ColonyIO().pfnCanTerraform(app, lppl, rgMin, rgMax, rgCost, fHelp)) {
        app.state.idPlayer = iPlrSav;
        return 0;
    }
    pctCur = PlanetQueryIO().pfnDesirability(app, lppl, iPlr);
    for (i = 0; i < 3; i++) {
        if (rgMin[i] != envNone) {
            iEnv = rgMin[i];
        } else if (rgMax[i] != envNone) {
            iEnv = rgMax[i];
        } else {
            rgpctBest[i] = 0;
            continue;
        }
        iSave = lppl->rgEnvVar[i];
        lppl->rgEnvVar[i] = iEnv;
        pctT = PlanetQueryIO().pfnDesirability(app, lppl, iPlr) - pctCur;
        if (pctT < 0) {
            pctT = -pctT;
        }
        rgpctBest[i] = (int16_t)(100 * pctT) / abs(iSave - iEnv) + 1;
        lppl->rgEnvVar[i] = iSave;
    }
    iSave = 0;
    for (i = 1; i < 3; i++) {
        if (rgpctBest[i] > rgpctBest[iSave]) {
            iSave = i;
        }
    }
    if (rgMin[iSave] != envNone) {
        iBest = -(iSave + 1);
    } else {
        iBest = iSave + 1;
    }
    app.state.idPlayer = iPlrSav;
    return iBest;
}

char *PszCalcEnvVar(APPSESSION &app, EnvType iEnv, int16_t iVar) {
    switch (iEnv) {
    case Gravity:
    default:
        return PszCalcGravity(app, iVar);
    case Temperature:
        CchSprintf(app.state.szWork, "%d%cC", iVar * 4 - 200, 186);
        break;
    case Radiation:
        CchSprintf(app.state.szWork, "%dmR", iVar);
    }
    return app.state.szWork;
}

char *PszCalcGravity(APPSESSION &app, int16_t iGravity) {
    int16_t d;
    int16_t iVal;

    d = abs(iGravity - 50);
    if (d <= 25) {
        iVal = d * 4 + 100;
    } else {
        iVal = (d - 25) * 24 + 200;
    }
    if (iGravity < 50) {
        iVal = 10000 / iVal;
    }
    CchSprintf(app.state.szWork, "%d.%02dg", iVal / 100, iVal % 100);
    return app.state.szWork;
}

int16_t PctPlanetCapacity(APPSESSION &app, PLANET *lppl) {
    int32_t pctCap;
    int32_t lPopMax;

    lPopMax = CalcPlanetMaxPop(app, lppl->id, app.state.idPlayer);
    if (lPopMax <= 0) {
        return 0;
    }
    pctCap = (int32_t)((int32_t)((uint32_t)(lppl->rgwtMin[3] * 100) + (int32_t)(lPopMax / 2)) / lPopMax);
    if (pctCap >= 1000) {
        pctCap = 999;
    }
    return LOWORD(pctCap);
}

int16_t PctPlanetOptValue(APPSESSION &app, PLANET *lppl, int16_t iPlr) {
    int16_t rgMax[3];
    int16_t i;
    int16_t rgMin[3];
    int16_t pctDesire;
    int16_t rgCost[3];
    int16_t rgiValSav[3];
    int16_t iNewVal;

    if (!ColonyIO().pfnCanTerraform(app, lppl, rgMin, rgMax, rgCost, 1)) {
        return PlanetQueryIO().pfnDesirability(app, lppl, iPlr);
    }
    for (i = 0; i < 3; i++) {
        rgiValSav[i] = lppl->rgEnvVar[i];
        if (app.state.rgplr[iPlr].rgEnvVarMin[i] != envImmune && lppl->rgEnvVar[i] != app.state.rgplr[iPlr].rgEnvVar[i]) {
            iNewVal = envNone;
            if (lppl->rgEnvVar[i] < app.state.rgplr[iPlr].rgEnvVar[i]) {
                if (rgMax[i] > lppl->rgEnvVar[i]) {
                    iNewVal = app.state.rgplr[iPlr].rgEnvVar[i] >= rgMax[i] ? rgMax[i] : app.state.rgplr[iPlr].rgEnvVar[i];
                }
            } else if (rgMin[i] != envNone && rgMin[i] < lppl->rgEnvVar[i]) {
                iNewVal = app.state.rgplr[iPlr].rgEnvVar[i] <= rgMin[i] ? rgMin[i] : app.state.rgplr[iPlr].rgEnvVar[i];
            }
            if (iNewVal != envNone) {
                lppl->rgEnvVar[i] = iNewVal;
            }
        }
    }
    pctDesire = PlanetQueryIO().pfnDesirability(app, lppl, app.state.idPlayer);
    for (i = 0; i < 3; i++) {
        lppl->rgEnvVar[i] = rgiValSav[i];
    }
    return pctDesire;
}

int16_t PctPlanetDesirability(APPSESSION &app, PLANET *lppl, int16_t iPlr) {
    int16_t iMin;
    int16_t d;
    int16_t iMax;
    int32_t pctNeg;
    int16_t iPref;
    int16_t i;
    int16_t dPenalty;
    int32_t pctPos;
    int16_t pctVar;
    int16_t iPlanet;
    int32_t pctMod;

    pctPos = 0;
    pctNeg = 0;
    pctMod = 10000;
    for (i = 0; i < 3; i++) {
        iPlanet = lppl->rgEnvVar[i];
        iPref = app.state.rgplr[iPlr].rgEnvVar[i];
        iMin = app.state.rgplr[iPlr].rgEnvVarMin[i];
        iMax = app.state.rgplr[iPlr].rgEnvVarMax[i];
        if (iMax < 0) {
            pctPos += 10000;
        } else if (iPlanet >= iMin && iPlanet <= iMax) {
            pctVar = abs(iPlanet - iPref) * 100;
            if (iPlanet < iPref) {
                d = iPref - iMin;
                pctVar /= d;
                dPenalty = (iPref - iPlanet) * 2 - d;
            } else {
                d = iMax - iPref;
                pctVar /= d;
                dPenalty = (iPlanet - iPref) * 2 - d;
            }
            pctVar = 100 - pctVar;
            pctPos += (uint32_t)(pctVar * pctVar);
            if (dPenalty > 0) {
                pctMod = (uint32_t)(pctMod * (int16_t)(d * 2 - dPenalty));
                pctMod = (int32_t)(pctMod / (int16_t)(d * 2));
            }
        } else if (iPlanet < iMin) {
            pctNeg += 15 >= iMin - iPlanet ? (int16_t)(iMin - iPlanet) : 15;
        } else {
            pctNeg += 15 >= iPlanet - iMax ? (int16_t)(iPlanet - iMax) : 15;
        }
    }
    if (pctNeg != 0) {
        return -LOWORD(pctNeg);
    }
    pctPos = (int32_t)((double)sqrt((double)((double)pctPos / 3.0)) + 0.9);
    pctPos = (int32_t)(pctPos * pctMod) / 10000;
    return LOWORD(pctPos);
}

int32_t CalcPlanetMaxPop(APPSESSION &app, int16_t idpl, int16_t iplr) {
    PLANET  pl;
    int32_t lMaxPop;
    int32_t pctDesire;
    int16_t ihuldef;

    PlanetQueryIO().pfnLookup(app, idpl, &pl);
    if (RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMajorAdv) == raMacintosh) {
        if (pl.iPlayer != iplr || !pl.fStarbase) {
            return 0;
        }
        ihuldef = app.state.rglpshdefSB[iplr][pl.isb].hul.ihuldef - ihuldefOrbitalFort;
        lMaxPop = app.state.rglPopMac[ihuldef];
    } else {
        pctDesire = PlanetQueryIO().pfnDesirability(app, &pl, iplr);
        if (pctDesire < 5) {
            lMaxPop = 500;
        } else {
            lMaxPop = (uint32_t)(pctDesire * 100);
        }
        if (RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMajorAdv) == raCheapCol) {
            lMaxPop -= (int32_t)(lMaxPop / 2);
        } else if (RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMajorAdv) == raNone) {
            lMaxPop += (int32_t)(lMaxPop / 5);
        }
    }
    if (RaceQueryIO().pfnGrbit(&app.state.rgplr[iplr], ibitRaceOBRM) != 0) {
        lMaxPop += (int32_t)(lMaxPop / 10);
    }
    return lMaxPop;
}

int16_t CMaxMines(APPSESSION &app, PLANET *lppl, int16_t iplr) {
    int32_t cMax;
    int32_t lPopMax;
    int16_t iEff;

    lPopMax = CalcPlanetMaxPop(app, lppl->id, iplr);
    iEff = RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMineOperate);
    cMax = (int32_t)(lPopMax * iEff) / 100;
    if (cMax < 10) {
        cMax = 10;
    }
    if (RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMajorAdv) == raMacintosh) {
        cMax = 0;
    }
    return LOWORD(cMax);
}

int16_t CMaxOperableMines(APPSESSION &app, PLANET *lppl, int16_t iplr, int16_t fNextYear) {
    int16_t cMax;
    int32_t cCur;
    int32_t lPop;
    int16_t iEff;

    cMax = ColonyIO().pfnMaxMines(app, lppl, iplr);
    iEff = RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMineOperate);
    lPop = lppl->rgwtMin[3];
    if (fNextYear) {
        lPop += PlanetQueryIO().pfnGrowth(app, lppl, 0);
    }
    cCur = (int32_t)(lPop * iEff) / 100;
    cMax = cMax < cCur ? cMax : LOWORD(cCur);
    if (cMax <= 0) {
        cMax = 1;
    }
    if (RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMajorAdv) == raMacintosh) {
        cMax = 0;
    }
    return cMax;
}

// Keep the expression and its existing integer conversion separate from the
// optional diagnostic receipt. The observer cannot substitute a numeric value.
int32_t LFromPlanetValue(double dValue) {
    if (PlanetQueryIO().pfnTruncate) PlanetQueryIO().pfnTruncate(dValue);
    return (int32_t)dValue;
}

int16_t CMinesOperating(APPSESSION &app, PLANET *lppl) {
    int16_t iplr;
    int16_t cMinesOp;
    int16_t cMines;

    iplr = lppl->iPlayer;
    if (iplr == iplrNone) {
        return 0;
    }
    if (RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMajorAdv) == raMacintosh) {
        return LOWORD(LFromPlanetValue(sqrt((double)lppl->rgwtMin[3])));
    }
    cMines = lppl->cMines;
    cMinesOp = ColonyIO().pfnOperableMines(app, lppl, lppl->iPlayer, 0);
    if (cMines > cMinesOp) {
        cMines = cMinesOp;
    }
    return cMines;
}

int16_t CFactoriesOperating(APPSESSION &app, PLANET *lppl) {
    int16_t iplr;
    int16_t cFacts;
    int16_t cFactsOp;

    iplr = lppl->iPlayer;
    if (iplr == iplrNone) {
        return 0;
    }
    if (RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMajorAdv) == raMacintosh) {
        return 0;
    }
    cFacts = lppl->cFactories;
    cFactsOp = ColonyIO().pfnOperableFactories(app, lppl, lppl->iPlayer, 0);
    if (cFacts > cFactsOp) {
        cFacts = cFactsOp;
    }
    return cFacts;
}

int16_t CMaxFactories(APPSESSION &app, PLANET *lppl, int16_t iplr) {
    int32_t cMax;
    int32_t lPopMax;
    int16_t iEff;

    lPopMax = CalcPlanetMaxPop(app, lppl->id, iplr);
    iEff = RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsFactOperate);
    cMax = (int32_t)(lPopMax * iEff) / 100;
    if (cMax < 10) {
        cMax = 10;
    }
    if (RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMajorAdv) == raMacintosh) {
        cMax = 0;
    }
    return LOWORD(cMax);
}

int16_t CMaxOperableFactories(APPSESSION &app, PLANET *lppl, int16_t iplr, int16_t fNextYear) {
    int16_t cMax;
    int32_t cCur;
    int32_t lPop;
    int16_t iEff;

    cMax = ColonyIO().pfnMaxFactories(app, lppl, iplr);
    iEff = RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsFactOperate);
    lPop = lppl->rgwtMin[3];
    if (fNextYear) {
        lPop += PlanetQueryIO().pfnGrowth(app, lppl, 0);
    }
    cCur = (int32_t)(lPop * iEff) / 100;
    cMax = cMax < cCur ? cMax : LOWORD(cCur);
    if (cMax <= 0) {
        cMax = 1;
    }
    if (RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMajorAdv) == raMacintosh) {
        cMax = 0;
    }
    return cMax;
}

int16_t CMaxDefenses(APPSESSION &app, PLANET *lppl, int16_t iplr) {
    int16_t cMax;
    int16_t pctDesire;

    pctDesire = PlanetQueryIO().pfnDesirability(app, lppl, iplr);
    cMax = 100 < (10 <= pctDesire * 4 ? pctDesire * 4 : 10) ? 100 : 10 > pctDesire * 4 ? 10 : pctDesire * 4;
    if (RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMajorAdv) == raMacintosh) {
        cMax = 0;
    }
    return cMax;
}

int16_t CMaxOperableDefenses(APPSESSION &app, PLANET *lppl, int16_t iplr, int16_t fNextYear) {
    int16_t cMax;
    int32_t cCur;
    int32_t lPop;

    cMax = ColonyIO().pfnMaxDefenses(app, lppl, iplr);
    lPop = lppl->rgwtMin[3];
    if (fNextYear) {
        lPop += PlanetQueryIO().pfnGrowth(app, lppl, 0);
    }
    cCur = (int32_t)((lPop + 24) / 25);
    if (cCur > 1000) {
        cCur = 1000;
    }
    cMax = cMax >= (int16_t)LOWORD(cCur) ? LOWORD(cCur) : cMax;
    if (RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMajorAdv) == raMacintosh) {
        cMax = 0;
    }
    return cMax;
}

int16_t CResourcesAtPlanet(APPSESSION &app, PLANET *lppl, int16_t iplr) {
    int16_t cRes;
    int32_t lPop;
    int16_t cFact;
    int32_t lPopMax;
    int16_t iEff;
    int16_t pctVal;
    int16_t iEnergy;

    if (lppl->rgwtMin[3] == 0) {
        return 0;
    }
    iEff = RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsResGen);
    lPop = lppl->rgwtMin[3];
    lPopMax = CalcPlanetMaxPop(app, lppl->id, iplr);
    if (lPop > lPopMax) {
        lPop = (int32_t)((lPop - lPopMax) / 2) + lPopMax;
        if (lPop > (int32_t)(lPopMax * 2)) {
            lPop = (int32_t)(lPopMax * 2);
        }
    }
    if (RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMajorAdv) == raMacintosh) {
        iEnergy = app.state.rgplr[iplr].rgTech[0];
        pctVal = PlanetQueryIO().pfnDesirability(app, lppl, iplr);
        if (iEnergy < 1) {
            iEnergy = 1;
        }
        if (pctVal < 25) {
            pctVal = 25;
        }
        cRes = LOWORD(LFromPlanetValue((double)sqrt((double)((double)lPop * iEnergy / iEff)) * pctVal / 10 + 0.999));
        goto LFinishUp;
    }
    cRes = LOWORD((int32_t)(lPop / iEff));
    cFact = ColonyIO().pfnOperableFactories(app, lppl, iplr, 0);
    if ((int32_t)lppl->cFactories < cFact) {
        cFact = lppl->cFactories;
    }
    iEff = RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsFactProd);
    cRes += LOWORD((int32_t)((uint32_t)(cFact * iEff) + 9) / 10);
LFinishUp:
    if (cRes == 0) {
        cRes = 1;
    }
    return cRes;
}

int16_t IWarpMAFromLppl(APPSESSION &app, PLANET *lppl, int16_t *pfTwo) {
    int16_t fTwo;
    int16_t iWarp;
    int16_t i;
    HUL    *lphul;
    int16_t iNew;

    iWarp = 0;
    fTwo = 0;
    if (pfTwo) {
        *pfTwo = 0;
    }
    if (lppl->iPlayer == iplrNone || !lppl->fStarbase) {
        return 0;
    }
    if (lppl->iPlayer != app.state.idPlayer && app.state.idPlayer != iplrNone && app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].det != detAll) {
        return 0;
    }
    lphul = &app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].hul;
    for (i = 0; i < lphul->chs; i++) {
        if (lphul->rghs[i].grhst == hstSpecialSB && lphul->rghs[i].cItem > 0 && lphul->rghs[i].iItem >= ispecialSBMassDriver5 &&
            lphul->rghs[i].iItem <= ispecialSBUltraDriver13) {
            iNew = lphul->rghs[i].iItem - 2;
            if (iNew > iWarp) {
                fTwo = 0;
                iWarp = iNew;
            } else if (iNew == iWarp) {
                fTwo = 1;
            }
        }
    }
    if (pfTwo) {
        *pfTwo = fTwo;
    }
    return iWarp;
}

int16_t StargateRangeFromLppl(APPSESSION &app, PLANET *lppl, int16_t iplr, int16_t ish) {
    int16_t i;
    HUL    *lphul;
    PART    part;

    if (lppl) {
        if (lppl->iPlayer == iplrNone || !lppl->fStarbase) {
            return 0;
        }
        lphul = &app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].hul;
    } else {
        lphul = &app.state.rglpshdefSB[iplr][ish].hul;
    }
    for (i = 0; i < lphul->chs; i++) {
        if (lphul->rghs[i].grhst == hstSpecialSB && lphul->rghs[i].cItem > 0 && lphul->rghs[i].iItem >= ispecialSBStargate100250 &&
            lphul->rghs[i].iItem <= ispecialSBStargateAnyAny) {
            part.hs = lphul->rghs[i];
            PartQueryIO().pfnLookup(app, &part);
            if (part.pspecialsb()->grAbility2 == -1) {
                return 10000;
            }
            return part.pspecialsb()->grAbility2;
        }
    }
    return 0;
}

int16_t FProdIsTerra(PROD *lpprod) {
    if (lpprod->grobj == grobjPlanet) {
        switch (lpprod->iItem) {
        case mdIdleTerraform:
        case iobjMinTerraform:
        case iobjMaxTerraform:
            return 1;
        }
    }
    return 0;
}

int16_t IpctCanTerraformLppl(APPSESSION &app, PLANET *lppl) {
    int16_t rgMax[3];
    int16_t i;
    int16_t rgMin[3];
    int16_t rgCost[3];
    int16_t ipct;

    if (!ColonyIO().pfnCanTerraform(app, lppl, rgMin, rgMax, rgCost, 1)) {
        return 0;
    }
    ipct = 0;
    for (i = 0; i < 3; i++) {
        if (rgMin[i] != envNone) {
            ipct += lppl->rgEnvVar[i] - rgMin[i];
        }
        if (rgMax[i] != envNone) {
            ipct += rgMax[i] - lppl->rgEnvVar[i];
        }
    }
    return ipct;
}

int16_t FCanTerraformLppl(APPSESSION &app, PLANET *lppl, int16_t *rgEnvMin, int16_t *rgEnvMax, int16_t *rgEnvCost, int16_t fHelp) {
    int16_t fRet;
    int16_t i;
    int16_t rgMove[3];
    int16_t iPlrSav;
    PART    part;
    int16_t dMin;
    int16_t dMax;
    int16_t dCur;
    int16_t ienvIdeal;

    iPlrSav = app.state.idPlayer;
    if (app.state.idPlayer == iplrNone) {
        app.state.idPlayer = lppl->iPlayer;
    }
    part.hs.grhst = hstTerra;
    for (i = 7; i >= 0; i--) {
        part.hs.iItem = i;
        if (PartQueryIO().pfnLookup(app, &part) == mdPartAvailAvailable)
            break;
    }
    if (i >= 0) {
        fRet = 1;
        for (i = 0; i < 3; i++) {
            rgMove[i] = part.pterra()->grAbility;
            rgEnvCost[i] = part.pterra()->resCost;
        }
    } else {
        fRet = 0;
        for (i = 0; i < 3; i++) {
            rgMove[i] = 0;
        }
    }
    for (i = 3; i >= 0; i--) {
        part.hs.iItem = i + 8;
        if (PartQueryIO().pfnLookup(app, &part) == mdPartAvailAvailable)
            break;
    }
    if (i >= 0 && part.pterra()->grAbility > rgMove[0]) {
        fRet = 1;
        rgMove[0] = part.pterra()->grAbility;
        *rgEnvCost = part.pterra()->resCost;
    }
    for (i = 3; i >= 0; i--) {
        part.hs.iItem = i + 12;
        if (PartQueryIO().pfnLookup(app, &part) == mdPartAvailAvailable)
            break;
    }
    if (i >= 0 && part.pterra()->grAbility > rgMove[1]) {
        fRet = 1;
        rgMove[1] = part.pterra()->grAbility;
        rgEnvCost[1] = part.pterra()->resCost;
    }
    for (i = 3; i >= 0; i--) {
        part.hs.iItem = i + 16;
        if (PartQueryIO().pfnLookup(app, &part) == mdPartAvailAvailable)
            break;
    }
    if (i >= 0 && part.pterra()->grAbility > rgMove[2]) {
        fRet = 1;
        rgMove[2] = part.pterra()->grAbility;
        rgEnvCost[2] = part.pterra()->resCost;
    }
    if (!fRet) {
        app.state.idPlayer = iPlrSav;
        return 0;
    }
    for (i = 0; i < 3; i++) {
        if (rgMove[i] == 0 || app.state.rgplr[app.state.idPlayer].rgEnvVarMin[i] == envImmune) {
            rgEnvMax[i] = envNone;
            rgEnvMin[i] = envNone;
        } else {
            rgEnvMin[i] = lppl->rgEnvVarOrig[i] - rgMove[i];
            rgEnvMax[i] = lppl->rgEnvVarOrig[i] + rgMove[i];
            if (rgEnvMin[i] >= lppl->rgEnvVar[i]) {
                rgEnvMin[i] = envNone;
            } else {
                rgEnvMin[i] = 1 <= rgEnvMin[i] ? rgEnvMin[i] : 1;
            }
            if (rgEnvMax[i] <= lppl->rgEnvVar[i]) {
                rgEnvMax[i] = envNone;
            } else {
                rgEnvMax[i] = 99 >= rgEnvMax[i] ? rgEnvMax[i] : 99;
            }
            if (fHelp) {
                if (lppl->rgEnvVar[i] == app.state.rgplr[app.state.idPlayer].rgEnvVar[i]) {
                    rgEnvMax[i] = envNone;
                    rgEnvMin[i] = envNone;
                } else if (lppl->rgEnvVar[i] > app.state.rgplr[app.state.idPlayer].rgEnvVar[i]) {
                    rgEnvMax[i] = envNone;
                    if (rgEnvMin[i] != envNone) {
                        rgEnvMin[i] = rgEnvMin[i] <= app.state.rgplr[app.state.idPlayer].rgEnvVar[i] ? app.state.rgplr[app.state.idPlayer].rgEnvVar[i] : rgEnvMin[i];
                    }
                } else {
                    rgEnvMin[i] = envNone;
                    if (rgEnvMax[i] != envNone) {
                        rgEnvMax[i] = rgEnvMax[i] >= app.state.rgplr[app.state.idPlayer].rgEnvVar[i] ? app.state.rgplr[app.state.idPlayer].rgEnvVar[i] : rgEnvMax[i];
                    }
                }
            } else {
                ienvIdeal = app.state.rgplr[app.state.idPlayer].rgEnvVar[i];
                dCur = abs(lppl->rgEnvVar[i] - ienvIdeal);
                if (rgEnvMin[i] != envNone) {
                    dMin = abs(rgEnvMin[i] - ienvIdeal);
                } else {
                    dMin = 0;
                }
                if (rgEnvMax[i] != envNone) {
                    dMax = abs(rgEnvMax[i] - ienvIdeal);
                } else {
                    dMax = 0;
                }
                if (dCur >= dMin && dCur >= dMax) {
                    rgEnvMax[i] = envNone;
                    rgEnvMin[i] = envNone;
                } else if (dMin >= dMax) {
                    rgEnvMax[i] = envNone;
                } else {
                    rgEnvMin[i] = envNone;
                }
            }
        }
    }
    for (i = 0; i < 3 && (rgEnvMax[i] == envNone && rgEnvMin[i] == envNone); i++) {
    }
    app.state.idPlayer = iPlrSav;
    if (i != 3) {
        return 1;
    }
    return 0;
}

void UninhabitPlanet(APPSESSION &app, PLANET *lppl) {
    int16_t i;

    if (lppl->iPlayer >= 0 && RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMajorAdv) == raTerra) {
        for (i = 0; i < 3; i++) {
            lppl->rgEnvVar[i] = lppl->rgEnvVarOrig[i];
        }
    }
    lppl->iPlayer = iplrNone;
    lppl->rgwtMin[3] = 0;
    if (lppl->lpplprod) {
        FreePl(app, app.state.lpPlanets[lppl->id].lpplprod);
        app.state.lpPlanets[lppl->id].lpplprod = NULL;
        lppl->lpplprod = NULL;
    }
    lppl->fNoResearch = 0;
    lppl->fStarbase = 0;
    lppl->cDefenses = 0;
    lppl->iScanner = 31;
    lppl->SetLStarbase(0);
    return;
}

int16_t PctCloakFromHuldef(APPSESSION &app, HUL *lphul, int16_t iplr, int16_t *ppctSteal) {
    int16_t chs;
    HS     *lphs;
    int32_t cPts;
    int16_t cScore;
    int16_t j;

    cPts = 0;
    chs = lphul->chs;
    if (iplr != iplrNone && (int16_t)lphul->ihuldef >= ihuldefOrbitalFort && RaceQueryIO().pfnGrbit(&app.state.rgplr[iplr], ibitRaceISB) != 0) {
        cPts = 40;
    } else {
        cPts = 0;
    }
    if (iplr != iplrNone && RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMajorAdv) == raStealth) {
        cPts += 300;
    }
    if (ppctSteal) {
        *ppctSteal = 0;
    }
    j = 0;
    lphs = lphul->rghs;
    while (j < chs) {
        cPts += CPtsCloakFromLphs(app, lphs);
        if (lphs->grhst == hstScanner && ppctSteal) {
            if (lphs->iItem == iscannerPickPocketScanner) {
                if (*ppctSteal < 70) {
                    *ppctSteal = 70;
                }
            } else if (lphs->iItem == iscannerRobberBaronScanner && *ppctSteal < 80) {
                *ppctSteal = 80;
            }
        }
        j++;
        lphs++;
    }
    if (cPts == 0) {
        return 0;
    }
    if (cPts < 0 || cPts > 25000) {
        return 0;
    }
    cScore = LOWORD(cPts);
    if (cScore <= 100) {
        return cScore >> 1;
    }
    cScore -= 100;
    if (cScore <= 200) {
        return (cScore >> 3) + 0x32;
    }
    cScore -= 200;
    if (cScore <= 312) {
        return cScore / 24 + 75;
    }
    cScore -= 312;
    if (cScore <= 512) {
        return (cScore >> 6) + 0x58;
    }
    if (cScore < 1000) {
        return (cScore >= 768) + 96;
    }
    return 98;
}

// FProdItemLine formats production queue item iprod as the planet window
// lists it: a schedule mark, the count and the name. An item that will
// never be built is marked '&' if fShowNever is set, or is left out
// (FALSE). PszNameProdItem names the item in szWork.
int16_t FProdItemLine(APPSESSION &app, PLANET *lppl, PLPROD *lpplprod, int16_t iprod, int16_t fShowNever, char *szLine) {
    char    ch;
    int16_t cItem;
    char   *psz;
    PROD   *lpprod;
    int16_t etaLast;
    int16_t etaFirst;

    lpprod = &lpplprod->rgprod()[iprod];
    psz = PszNameProdItem(app, lpprod);
    ResearchIO().pfnEstimate(app, lppl, lpplprod, static_cast<ProdItemType>(iprod), &etaFirst, &etaLast);
    if ((etaFirst == 0 && etaLast == 0) || (etaFirst == -1 && etaLast == -1)) {
        if (!fShowNever)
            return 0;
        ch = '&';
    } else if ((etaFirst > 1 && etaFirst < 100) || (etaFirst == 100 && lpprod->grobj == grobjPlanet && lpprod->iItem < mdIdleFactory)) {
        ch = ' ';
    } else if (etaFirst == 1 && etaLast == 1) {
        ch = '*';
    } else if (etaFirst < 100) {
        ch = '#';
    } else {
        ch = '!';
    }
    cItem = lpprod->cItem;
    CchSprintf(szLine, "%c%5d%s", ch, cItem, psz);
    if (lpprod->grobj == grobjPlanet) {
        if (lpprod->iItem < mdIdleFactory) {
            szLine[1] += 2;
            if (lpprod->iItem == iobjAlchemy) {
                szLine[5] = '*';
            }
        }
        switch (lpprod->iItem) {
        case mdIdleTerraform:
        case iobjMinTerraform:
        case iobjMaxTerraform:
            szLine[1]++;
        }
    }
    return 1;
}

// PszProdQueueTop returns, in szWork, the first item of lppl's production
// queue (lpplprod, or the planet's own) that will be built, as
// FProdItemLine formats it, or "Queue is empty". The reports and the
// planet dump show it.
char *PszProdQueueTop(APPSESSION &app, PLANET *lppl, PLPROD *lpplprod) {
    char    szLine[80];
    char   *psz;
    int16_t i;

    if (!lpplprod) {
        lpplprod = lppl->lpplprod;
    }
    if (!lpplprod || lpplprod->iprodMac == 0) {
        psz = TextIO().pfnCompressed(app, idsQueueEmpty);
        if (psz != app.state.szWork) {
            strcpy(app.state.szWork, psz);
        }
    }
    if (lpplprod) {
        for (i = 0; i < lpplprod->iprodMac; i++) {
            if (FProdItemLine(app, lppl, lpplprod, i, 0, szLine)) {
                strcpy(app.state.szWork, szLine);
                return app.state.szWork;
            }
        }
        TextIO().pfnCchGetString(app, idsQueueEmpty, app.state.szWork);
    }
    return app.state.szWork;
}
