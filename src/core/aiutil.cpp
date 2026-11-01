#include "common.h"
#include "appstate.h"
#include "fleet_query_io.h"
#include "text_io.h"
#include "production_io.h"
#include "colony_io.h"
#include "planet_query_io.h"
#include "research_io.h"
#include "part_query_io.h"
#include "race_query_io.h"
#include "capacity_io.h"
#include "random_io.h"

int32_t vrgAiPacketDist[2] = {aiPacketDistanceSquaredSingleMassDriver, aiPacketDistanceSquaredTwoMassDrivers};
uint8_t vrgSBMacAisb[6] = {aiSbRecipeMacSpaceDock, aiSbRecipeMacSpaceStation,     aiSbRecipeMacUltraStationEarly,
                           aiSbRecipeSpaceStation, aiSbRecipeMacUltraStationLate, aiSbRecipeMacDeathStar};

int16_t FCreateAiShdef(APPSESSION &app, int16_t ishdef, HulDef ihul, uint8_t *rgaip) {
    StringId ids;
    int32_t  grbitHull;
    int16_t  cItem;
    int16_t  ihs;
    HUL     *lphul;
    PART     part;
    SHDEF    shdef;

    memset(&shdef, 0, sizeof(SHDEF));
    if (FLookupPartX(app, &part, ishdef < 0 ? 0x400 : 0x4000, ihul) != mdPartAvailAvailable) {
        return 0;
    }
    lphul = part.phul();
    shdef.hul = *lphul;
    shdef.hul.ihuldef = lphul->ihuldef;
    for (ihs = 0; ihs < lphul->chs; ihs++) {
        if (!FGetAIPart(app, static_cast<AiPartPreference>(rgaip[ihs]), &part)) {
            return 0;
        }
        part.hs.cItem = lphul->rghs[ihs].cItem;
        shdef.hul.rghs[ihs] = part.hs;
    }
    shdef.hul.chs = ihs;
    shdef.fFree = 0;
    if (ishdef < 0) {
        app.state.shdefBuild = shdef;
        return 1;
    }
    ids = idsUniverseDefinitionFileSeemsMissingCorrupt;
    grbitHull = 1 << ihul;
    if (grbitHull & 0x780) {
        ids = idsLyingBastard;
        cItem = 16;
    } else if (grbitHull & 0x40) {
        ids = idsScrapper;
        cItem = 16;
    } else if (grbitHull & 0x70) {
        ids = idsEasterBunny;
        cItem = 10;
    } else if (grbitHull & 0xf0000) {
        ids = idsPidgeon;
        cItem = 12;
    } else if (grbitHull & 0xf) {
        ids = idsGlovebox;
        cItem = 8;
    } else if (grbitHull & 0x1f00000) {
        ids = idsGroundHog;
        cItem = 8;
    } else if (grbitHull & 0x3800) {
        ids = idsPricklyPear;
        cItem = 8;
    } else if (grbitHull & 0xc000) {
        ids = idsEgg;
        cItem = 8;
    } else {
        ids = idsZombie;
        cItem = 8;
    }
    shdef.ishdef = ishdef;
    PickANameAndBmp(app, &shdef, ids, cItem, lphul->ibmp);
    return FChangeAiShdef(app, &shdef, ishdef);
}

int16_t FGetAIPart(APPSESSION &app, AiPartPreference aip, PART *ppart) {
    int16_t cTry;
    int16_t iOffset;
    int16_t i;
    int16_t cItem;
    PART    part;

    iOffset = 0;
    for (i = 0; i < (int16_t)aip; i++) {
        iOffset += vrgcAiParts[i];
    }
    cTry = vrgcAiParts[aip];
    while (cTry-- > 0) {
        part.hs.grhst = static_cast<HullSlotType>(1 << vrgAiParts[iOffset].ibit);
        part.hs.iItem = vrgAiParts[iOffset].iItem;
        part.hs.cItem = 0;
        cItem = vrgAiParts[iOffset].cItem;
        while (cItem-- > 0) {
            if (PartQueryIO().pfnLookup(app, &part) == mdPartAvailAvailable) {
                *ppart = part;
                return 1;
            }
            part.hs.iItem--;
        }
        iOffset++;
    }
    return 0;
}

void PickANameAndBmp(APPSESSION &app, SHDEF *pshdef, StringId ids, int16_t cids, int16_t ibmpStart) {
    int16_t i;
    int16_t rgfBmpUsed[4];
    int16_t ishdef;

    memset(rgfBmpUsed, 0, 8);
    if (pshdef->ishdef >= 16) {
        for (ishdef = 0; ishdef < 10; ishdef++) {
            if (!app.state.rglpshdefSB[app.state.idPlayer][ishdef].fFree && app.state.rglpshdefSB[app.state.idPlayer][ishdef].hul.ihuldef == pshdef->hul.ihuldef) {
                rgfBmpUsed[app.state.rglpshdefSB[app.state.idPlayer][ishdef].hul.ibmp + -ibmpStart] = 1;
            }
        }
    } else {
        for (ishdef = 0; ishdef < 16; ishdef++) {
            if (!app.state.rgshdef[ishdef].fFree && app.state.rgshdef[ishdef].hul.ihuldef == pshdef->hul.ihuldef) {
                rgfBmpUsed[app.state.rgshdef[ishdef].hul.ibmp + -ibmpStart] = 1;
            }
        }
    }
    for (i = 0; i < 4 && rgfBmpUsed[i] != 0; i++) {
    }
    if (i == 4) {
        i = RandomIO().pfnRandom(app, 4);
    }
    pshdef->hul.ibmp = ibmpStart + i;
    for (i = 0; i < 20; i++) {
        TextIO().pfnCchGetString(app, static_cast<StringId>(RandomIO().pfnRandom(app, cids) + ids), pshdef->hul.szClass);
        if (pshdef->ishdef >= 16) {
            for (ishdef = 0; ishdef < 10 && (app.state.rglpshdefSB[app.state.idPlayer][ishdef].fFree || app.state.rglpshdefSB[app.state.idPlayer][ishdef].hul.ihuldef != pshdef->hul.ihuldef ||
                                             strcmp(pshdef->hul.szClass, app.state.rglpshdefSB[app.state.idPlayer][ishdef].hul.szClass) != 0);
                 ishdef++) {
            }
            if (ishdef == 10) {
                return;
            }
        } else {
            for (ishdef = 0; ishdef < 16 && (app.state.rgshdef[ishdef].fFree || strcmp(pshdef->hul.szClass, app.state.rgshdef[ishdef].hul.szClass) != 0); ishdef++) {
            }
            if (ishdef == 16) {
                return;
            }
        }
    }
    CchSprintf(pshdef->hul.szClass, "%s %d", TextIO().pfnCompressed(app, static_cast<StringId>(RandomIO().pfnRandom(app, cids) + ids)), RandomIO().pfnRandom(app, 100));
    return;
}

int16_t FChangeAiShdef(APPSESSION &app, SHDEF *pshdef, int16_t ishdef) {
    SHDEF  *lpshdefBase;
    int16_t iDir;
    int16_t ishdefWork;
    SHDEF   shdef;

    shdef = *pshdef;
    shdef.ishdef = ishdef;
    shdef.turn = app.state.game.turn;
    shdef.cExist = 0;
    shdef.cBuilt = 0;
    if (ishdef >= 16) {
        lpshdefBase = app.state.rglpshdefSB[app.state.idPlayer];
        ishdefWork = ishdef - 16;
    } else {
        lpshdefBase = app.state.rglpshdef[app.state.idPlayer];
        ishdefWork = ishdef;
    }
    UpdateShdefCost(app, &shdef);
    if (shdef.fFree != lpshdefBase[ishdefWork].fFree) {
        iDir = !shdef.fFree ? 1 : -1;
        if (ishdef >= 16) {
            app.state.rgplr[app.state.idPlayer].cshdefSB += iDir;
        } else {
            app.state.rgplr[app.state.idPlayer].cShDef += iDir;
        }
    } else if (!lpshdefBase[ishdefWork].fFree) {
        lpshdefBase[ishdefWork].fFree = 1;
        LogChangeShDef(app, lpshdefBase + ishdefWork);
    }
    lpshdefBase[ishdefWork] = shdef;
    LogChangeShDef(app, &shdef);
    return 1;
}

int16_t XferAiSupply(APPSESSION &app, GrobjClass grobjSrc, int16_t idSrc, GrobjClass grobjDst, int16_t idDst, MineralType iSupply, int16_t cQuan) {
    int16_t dChg;
    int16_t iT;
    int32_t cAvailable;

    if (cQuan == 0) {
        return 0;
    }
    if (cQuan < 0) {
        iT = grobjSrc;
        grobjSrc = grobjDst;
        grobjDst = static_cast<GrobjClass>(iT);
        iT = idSrc;
        idSrc = idDst;
        idDst = iT;
        cQuan = -cQuan;
    }
    cAvailable = ChgCargo(app, grobjSrc, idSrc, iSupply, 0, NULL);
    if (cQuan > cAvailable) {
        cQuan = LOWORD(cAvailable);
    }
    if (cQuan == 0) {
        return 0;
    }
    dChg = LOWORD(ChgCargo(app, grobjDst, idDst, iSupply, cQuan, NULL));
    if (dChg != 0) {
        ChgCargo(app, grobjSrc, idSrc, iSupply, (int16_t)-dChg, NULL);
    }
    return dChg;
}

int16_t XferAiTroopers(APPSESSION &app, int16_t idSrc, int16_t idDst, int16_t cQuan) {
    int32_t cAvailable;

    if (cQuan == 0) {
        return 0;
    }
    cAvailable = ChgCargo(app, grobjFleet, idSrc, Colonists, 0, NULL);
    if (cQuan > cAvailable) {
        cQuan = LOWORD(cAvailable);
    }
    if (cQuan == 0) {
        return 0;
    }
    ChgCargo(app, grobjFleet, idSrc, Colonists, (int16_t)-cQuan, NULL);
    ChgCargo(app, grobjPlanet, idDst, Colonists, cQuan, NULL);
    return cQuan;
}

int16_t FColonizeAiFleet(APPSESSION &app, FLEET *lpfl, int16_t idPlanet) {
    ORDER ord;

    ChangeMainObjSel(app, grobjFleet, lpfl->id);
    memset(&ord, 0, sizeof(ORDER));
    ord.pt = app.state.rgptPlan[idPlanet];
    ord.grobj = grobjPlanet;
    ord.id = idPlanet;
    ord.grTask = grTaskColonize;
    ord.fValidTask = 1;
    ord.iWarp = (uint16_t)IFindIdealWarp(app, lpfl, 1);
    if (!FMoveAiFleet(app, lpfl, &ord, 0)) {
        return 0;
    }
    lpfl->fMark = 1;
    return 1;
}

int16_t FGotoWormholeAiFleet(APPSESSION &app, FLEET *lpfl, THING *lpthWorm) {
    ORDER ord;

    ChangeMainObjSel(app, grobjFleet, lpfl->id);
    memset(&ord, 0, sizeof(ORDER));
    ord.pt = lpthWorm->pt();
    ord.grobj = grobjThing;
    ord.id = lpthWorm->idFull();
    ord.grTask = grTaskNone;
    ord.fValidTask = 0;
    ord.iWarp = (uint16_t)IFindIdealWarp(app, lpfl, 1);
    if (!FMoveAiFleet(app, lpfl, &ord, 0)) {
        return 0;
    }
    lpfl->fMark = 1;
    return 1;
}

int16_t IdNearestColonizablePlanet(APPSESSION &app, FLEET *lpflCol, THING **plpthWorm) {
    PLANET  *lpplMac;
    POINT16  pt;
    int32_t  dy;
    int32_t  d2;
    PLANET  *lppl;
    int16_t  idBest;
    int16_t  ifl;
    FLEET   *lpfl;
    int16_t  i;
    int16_t  iVal;
    int32_t  dx;
    uint8_t *lpb;
    int32_t  d2Cur;

    if (app.state.rgplr[app.state.idPlayer].idAi != idAiRobotoid && app.state.rgplr[app.state.idPlayer].idAi != idAiMacinti) {
        iVal = 16;
        if (app.state.fMarkedPlanets)
            goto LFindNearest;
    } else {
        iVal = 0;
    }
    lpb = app.state.vlpbAiPlanet + 15;
    i = 0;
    while (i < app.state.game.cPlanMax) {
        *lpb = iVal;
        i++;
        lpb += 16;
    }
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer != iplrNone) {
            if (lppl->iPlayer == app.state.idPlayer) {
                iVal = 1;
            } else {
                iVal = 2;
            }
        } else if (app.state.rgplr[app.state.idPlayer].idAi != idAiRobotoid && app.state.rgplr[app.state.idPlayer].idAi != idAiMacinti && PctPlanetOptValue(app, lppl, app.state.idPlayer) < 0) {
            iVal = 8;
        } else {
            iVal = 0;
        }
        app.state.vlpbAiPlanet[lppl->id * 16 + 15] = iVal;
    }
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->iPlayer == app.state.idPlayer && lpfl->cord > 1 && lpfl != lpflCol && lpfl->lpplord->rgord()[1].grobj == grobjPlanet &&
            ((app.state.rgplr[app.state.idPlayer].idAi != idAiRobotoid && app.state.rgplr[app.state.idPlayer].idAi != idAiMacinti) || lpfl->lpplord->rgord()[1].grTask == grTaskColonize)) {
            app.state.vlpbAiPlanet[lpfl->lpplord->rgord()[1].id * 16 + 15] = 4;
        }
    }
LFindNearest:
    lpb = app.state.vlpbAiPlanet + 15;
    d2 = 99999999;
    pt = lpflCol->pt;
    idBest = idplNone;
    i = 0;
    while (i < app.state.game.cPlanMax) {
        if (*lpb <= 0) {
            dx = abs(pt.x - app.state.rgptPlan[i].x);
            dy = abs(pt.y - app.state.rgptPlan[i].y);
            if (dx < d2 && dy < d2) {
                d2Cur = (uint32_t)(dx * dx);
                if ((int32_t)(uint32_t)(dx * dx) < d2) {
                    d2Cur += (uint32_t)(dy * dy);
                    if (d2Cur < d2) {
                        d2 = d2Cur;
                        idBest = i;
                    }
                }
            }
        }
        i++;
        lpb += 16;
    }
    app.state.fMarkedPlanets = 1;
    if (plpthWorm) {
        *plpthWorm = NULL;
        if (lpflCol->idPlanet != idPlanetDeepSpace && app.state.game.turn < 120) {
            lppl = LpplFromId(app, lpflCol->idPlanet);
            if (lppl && lppl->iPlayer == app.state.idPlayer) {
                *plpthWorm = LpthWormFind(app, &pt, d2);
            }
        }
        if (*plpthWorm) {
            idBest = idplNone;
        }
    }
    return idBest;
}

THING *LpthWormFind(APPSESSION &app, POINT16 *ppt, int32_t d2) {
    int16_t  pctGood;
    int16_t  dy;
    int32_t  d2Worm;
    uint16_t grbitplr;
    THING   *lpth;
    THING   *lpthBest;
    int16_t  iVal;
    int16_t  dx;
    THING   *lpthMac;
    int32_t  d2Cur;

    lpthBest = NULL;
    grbitplr = 1 << app.state.idPlayer;
    d2Worm = 1000000;
    pctGood = 0;
    lpth = app.state.lpThings;
    lpthMac = app.state.lpThings + app.state.cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith() == ithWormhole) {
            dx = abs(ppt->x - lpth->pt().x());
            dy = abs(ppt->y - lpth->pt().y());
            d2Cur = (int16_t)(dx * dx + dy * dy);
            if (d2Cur <= (int32_t)(d2 * 4) && d2Cur <= 46656) {
                if (lpth->thw().grbitPlrTrav() & grbitplr) {
                    iVal = PctWormholeMoves(lpth);
                    iVal = 70 - 10 * iVal;
                } else if (d2Cur <= d2) {
                    iVal = 90;
                } else {
                    iVal = 50;
                }
                if (iVal > pctGood || (iVal == pctGood && d2Cur < d2Worm)) {
                    pctGood = iVal;
                    lpthBest = lpth;
                    d2Worm = d2Cur;
                }
            }
        }
    }
    if (lpthBest && RandomIO().pfnRandom(app, 100) < pctGood) {
        return lpthBest;
    }
    return NULL;
}

uint32_t UlFleetPower(APPSESSION &app, FLEET *lpfl) {
    uint32_t ul;
    int16_t  iplr;
    int16_t  ishdef;

    ul = 0;
    iplr = lpfl->iPlayer;
    for (ishdef = 0; ishdef < 16; ishdef++) {
        if (lpfl->rgcsh[ishdef] != 0) {
            ul += (uint32_t)(app.state.rglpshdef[iplr][ishdef].lPower * lpfl->rgcsh[ishdef]);
            if (ul & 0x80000000)
                break;
        }
    }
    return ul;
}

int16_t IdNearestUnknownPlanet(APPSESSION &app, FLEET *lpfl, THING **plpthWorm) {
    POINT16  pt;
    int32_t  dy;
    int32_t  d2;
    int16_t  idBest;
    int16_t  i;
    int32_t  dx;
    uint8_t *lpb;
    int32_t  d2Cur;

    if (!app.state.fMarkedPlanets) {
        IdNearestColonizablePlanet(app, lpfl, NULL);
    }
    lpb = app.state.vlpbAiPlanet + 15;
    d2 = 99999999;
    pt = lpfl->pt;
    idBest = idplNone;
    i = 0;
    while (i < app.state.game.cPlanMax) {
        if (*lpb == 16) {
            dx = abs(pt.x - app.state.rgptPlan[i].x);
            dy = abs(pt.y - app.state.rgptPlan[i].y);
            if (dx < d2 && dy < d2) {
                d2Cur = (uint32_t)(dx * dx);
                if ((int32_t)(uint32_t)(dx * dx) < d2) {
                    d2Cur += (uint32_t)(dy * dy);
                    if (d2Cur < d2) {
                        d2 = d2Cur;
                        idBest = i;
                    }
                }
            }
        }
        i++;
        lpb += 16;
    }
    if (plpthWorm) {
        *plpthWorm = NULL;
        if (lpfl->idPlanet != idPlanetDeepSpace && RandomIO().pfnRandom(app, 100) < 5) {
            *plpthWorm = LpthWormFind(app, &pt, d2);
        }
        if (*plpthWorm) {
            idBest = idplNone;
        }
    }
    return idBest;
}

void AddMinesToBlockedQueues(APPSESSION &app) {
    PROD    prod;
    int32_t cMaxBuild;
    int16_t etaBetterAlchemy;
    int32_t cBuild;
    int16_t etaFirst;
    PLANET *lppl;
    int32_t cResMine;
    int32_t cRes;
    int16_t ipl;
    int32_t rgCost[4];
    PROD    rgprod[64];
    int16_t etaBetterMines;

    for (ipl = 0; ipl < app.state.vclpplAi; ipl++) {
        lppl = app.state.vrglpplAi[ipl];
        if (!app.state.vrglpplAi[ipl])
            break;
        if (lppl->lpplprod) {
            prod = lppl->lpplprod->rgprod()[0];
            if (prod.grobj == grobjPlanet &&
                (prod.iItem == mdIdleMine || prod.iItem == iobjAlchemy || prod.iItem == mdIdleAlchemy || prod.iItem == mdIdleTerraform))
                continue;
            ChangeMainObjSel(app, grobjPlanet, lppl->id);
            PszProductionETA(app, &app.state.sel.pl, app.state.sel.pl.lpplprod, iobjMine, &etaFirst, NULL);
            if (etaFirst != 1) {
                if (etaFirst == -1) {
                    etaFirst = 600;
                }
                ProductionIO().pfnQuote(app, lppl, &prod, (uint32_t *)rgCost, app.state.idPlayer, 1);
                cRes = ResearchIO().pfnResources(app, &app.state.sel.pl, app.state.idPlayer);
                if (!app.state.sel.pl.fNoResearch) {
                    cRes -= (int32_t)(cRes * (int16_t)app.state.rgplr[app.state.idPlayer].pctResearch) / 100;
                }
                if (rgCost[3] <= (int32_t)(uint32_t)(cRes * (int16_t)(etaFirst - 1))) {
                    cMaxBuild = ColonyIO().pfnOperableMines(app, &app.state.sel.pl, app.state.idPlayer, 1) - app.state.sel.pl.cMines;
                    if (cMaxBuild < 0) {
                        cMaxBuild = 0;
                    }
                    cResMine = RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMineBuild);
                    if ((int32_t)(uint32_t)(cResMine * cMaxBuild) <= cRes) {
                        cBuild = cMaxBuild;
                    } else {
                        cBuild = (int32_t)(cRes / cResMine);
                    }
                    ProductionIO().pfnInitialize(app, rgprod);
                    if (cBuild > 0) {
                        AddItemToQueue(app, mdIdleMine, LOWORD(cBuild), grobjPlanet, addItemFront);
                        FinishProduction(app, 1);
                        PszProductionETA(app, &app.state.sel.pl, app.state.sel.pl.lpplprod, iobjFactory, &etaBetterMines, NULL);
                        if (etaBetterMines == -1) {
                            etaBetterMines = 700;
                        }
                        app.state.sel.pl.lpplprod->rgprod()[0].cItem = 1;
                        app.state.sel.pl.lpplprod->rgprod()[0].iItem = iobjAlchemy;
                    } else {
                        etaBetterMines = 700;
                        AddItemToQueue(app, iobjAlchemy, 1, grobjPlanet, addItemFront);
                        FinishProduction(app, 1);
                    }
                    PszProductionETA(app, &app.state.sel.pl, app.state.sel.pl.lpplprod, iobjFactory, &etaBetterAlchemy, NULL);
                    if (etaBetterAlchemy == -1) {
                        etaBetterAlchemy = 700;
                    }
                    if ((etaBetterAlchemy >= etaFirst || etaBetterAlchemy >= etaBetterMines) && cBuild >= 1) {
                        app.state.sel.pl.lpplprod->rgprod()[0].iItem = mdIdleMine;
                        if (etaFirst < etaBetterMines || cBuild <= 0) {
                            app.state.sel.pl.lpplprod->iprodMac--;
                            memmove(app.state.sel.pl.lpplprod->rgprod(), &app.state.sel.pl.lpplprod->rgprod()[1], app.state.sel.pl.lpplprod->iprodMac * sizeof(PROD));
                        } else {
                            app.state.sel.pl.lpplprod->rgprod()[0].cItem = LOWORD(cBuild);
                        }
                    }
                }
            }
        }
    }
    return;
}

int16_t FFleetInField(FLEET *lpfl, THING *lpth) {
    int16_t dy;
    int16_t dx;
    int32_t dxy2;

    dx = abs(lpfl->pt.x - lpth->pt().x());
    dy = abs(lpfl->pt.y - lpth->pt().y());
    dxy2 = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
    if (dxy2 < lpth->thm().cMines()) {
        return 1;
    }
    return 0;
}

void SetAiFleetIdealSpeed(APPSESSION &app, FLEET *lpfl, int16_t wtFuelMax, int16_t cMinefields, THING **rglpth) {
    THING  *lpth;
    int16_t i;
    int16_t j;
    int16_t ith;
    THING  *lpthMac;
    int16_t fMinefield;

    fMinefield = 0;
    switch (cMinefields) {
    case 0:
        break;
    case -1:
        lpth = app.state.lpThings;
        lpthMac = app.state.lpThings + app.state.cThing;
        for (; lpth < lpthMac; lpth++) {
            if (lpth->ith() == ithMinefield && lpth->iplr() != app.state.idPlayer && lpth->thm().iType() != mineSpeedBump && FFleetInField(lpfl, lpth)) {
                fMinefield = 1;
                goto LSelectFleet;
            }
        }
        break;
    default:
        for (ith = 0; ith < cMinefields; ith++) {
            if (FFleetInField(lpfl, rglpth[ith])) {
                lpth = rglpth[ith];
                fMinefield = 1;
                goto LSelectFleet;
            }
        }
        break;
    }
LSelectFleet:
    ChangeMainObjSel(app, grobjFleet, lpfl->id);
    if (fMinefield) {
        switch (lpth->thm().iType()) {
        case mineHeavy:
            i = 6;
            break;
        case mineSpeedBump:
            i = 5;
            j = RandomIO().pfnRandom(app, 10);
            if (j >= 7) {
                i++;
            }
            break;
        default:
            i = 4;
            j = RandomIO().pfnRandom(app, 10);
            if (j >= 4) {
                i++;
            }
            break;
        }
        switch (RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv)) {
        case raStealth:
            i++;
            break;
        }
        goto WarpSet;
    }
    i = IWarpBestForWaypoint(app, lpfl, &lpfl->lpplord->rgord()[1]);
WarpSet:
    if (i != lpfl->lpplord->rgord()[1].iWarp) {
        app.state.sel.fl.lpplord->rgord()[1].iWarp = i;
        FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
    }
    return;
}

int16_t IdTargetAttack(APPSESSION &app, FLEET *lpfl, FLEET *lpflAtk, FLEET *lpflEnemy, int16_t fOnlyHumans) {
    FLEET   *lpflClosest;
    FLEET   *lpflT;
    int32_t  lDistBest;
    PLANET  *lpplMac;
    POINT16  pt;
    int16_t  dy;
    PLANET  *lppl;
    int32_t  lDist;
    int16_t  idClosest;
    int16_t  i;
    int16_t  cShipsAtk;
    int16_t  cShipsDst;
    PLANET  *lpplClosest;
    uint8_t *lpb;
    int16_t  dx;
    FLEET   *lpflAtk2;
    ORDER    ord;

    cShipsDst = 0;
    cShipsAtk = 0;
    for (i = 0; i < 16; i++) {
        cShipsAtk += lpfl->rgcsh[i];
    }
    lpflClosest = NULL;
    lpflT = lpflEnemy;
    lDistBest = 1000000;
    pt = lpfl->pt;
    memset(&ord, 0, sizeof(ORDER));
    for (; lpflT; lpflT = lpflT->lpflNext) {
        if (fOnlyHumans && app.state.rgplr[lpflT->iPlayer].fAi)
            goto NextTarg;
        lpflAtk2 = lpflAtk;
        cShipsDst = 0;
        for (; lpflAtk2; lpflAtk2 = lpflAtk2->lpflNext) {
            if (lpflAtk2 != lpfl && lpflAtk2->cord >= 2 && lpflAtk2->lpplord->rgord()[1].id == lpflT->id && lpflAtk2->lpplord->rgord()[1].grobj == grobjFleet) {
                for (i = 0; i < 16; i++) {
                    cShipsDst += lpflAtk2->rgcsh[i];
                }
            }
        }
        if ((cShipsDst > 0 && RandomIO().pfnRandom(app, 3) == 0) || (5 * cShipsDst > cShipsAtk && RandomIO().pfnRandom(app, 15) == 0))
            goto NextTarg;
        dx = lpflT->pt.x - pt.x;
        dy = lpflT->pt.y - pt.y;
        lDist = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        if (lDist < lDistBest) {
            lDistBest = lDist;
            lpflClosest = lpflT;
        }
    NextTarg:;
    }
    if (lDistBest < 32400) {
        ord.pt = lpflClosest->pt;
        ord.grobj = grobjFleet;
        ord.id = lpflClosest->id;
    } else {
        if (lpflClosest) {
            if (lpfl->rgwtMin[4] < (int32_t)(ProductionIO().pfnFleetStat(app, lpfl, 1) / 2) && FMoveToNearestStarbase(app, lpfl, 0)) {
                return 0;
            }
            pt = lpflClosest->pt;
            lpplClosest = NULL;
            lDistBest = 1000000;
            lppl = app.state.lpPlanets;
            lpplMac = app.state.lpPlanets + app.state.cPlanet;
            for (; lppl < lpplMac; lppl++) {
                for (lpflAtk2 = lpflAtk; lpflAtk2 && (lpflAtk2 == lpfl || lpflAtk2->cord < 2 || lpflAtk2->lpplord->rgord()[1].id != lppl->id ||
                                                      lpflAtk2->lpplord->rgord()[1].grobj != grobjPlanet);
                     lpflAtk2 = lpflAtk2->lpflNext) {
                }
                if (!lpflAtk2) {
                    dx = app.state.rgptPlan[lppl->id].x - pt.x;
                    dy = app.state.rgptPlan[lppl->id].y - pt.y;
                    lDist = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                    if (lDist < lDistBest) {
                        lDistBest = lDist;
                        lpplClosest = lppl;
                    }
                }
            }
            if (lpplClosest && lpplClosest->id != lpfl->idPlanet) {
                ord.pt = app.state.rgptPlan[lpplClosest->id];
                ord.grobj = grobjPlanet;
                ord.id = lpplClosest->id;
                goto ThwakSumthin;
            }
        } else if (fOnlyHumans) {
            return IdTargetAttack(app, lpfl, lpflAtk, lpflEnemy, 0);
        }
        pt = lpfl->pt;
        lpplClosest = NULL;
        lDistBest = 1000000;
        lppl = app.state.lpPlanets;
        lpplMac = app.state.lpPlanets + app.state.cPlanet;
        for (; lppl < lpplMac; lppl++) {
            if (lppl->iPlayer != iplrNone && lppl->iPlayer != app.state.idPlayer) {
                dx = app.state.rgptPlan[lppl->id].x - pt.x;
                dy = app.state.rgptPlan[lppl->id].y - pt.y;
                lDist = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                if (lDist < lDistBest) {
                    lDistBest = lDist;
                    lpplClosest = lppl;
                }
            }
        }
        if (lpplClosest && lpplClosest->id != lpfl->idPlanet) {
            ord.pt = app.state.rgptPlan[lpplClosest->id];
            ord.grobj = grobjPlanet;
            ord.id = lpplClosest->id;
        } else {
            idClosest = idplNone;
            lDistBest = 1000000;
            lpb = app.state.vlpbAiPlanet + 9;
            i = 0;
            while (i < app.state.game.cPlanMax) {
                if (*lpb <= 0) {
                    dx = app.state.rgptPlan[i].x - pt.x;
                    dy = app.state.rgptPlan[i].y - pt.y;
                    lDist = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                    if (lDist < lDistBest) {
                        lDistBest = lDist;
                        idClosest = i;
                    }
                }
                i++;
                lpb += 16;
            }
            if (idClosest != idplNone) {
                ord.pt = app.state.rgptPlan[idClosest];
                ord.grobj = grobjPlanet;
                ord.id = idClosest;
            } else {
                ord.id = RandomIO().pfnRandom(app, app.state.game.cPlanMax);
                ord.grobj = grobjPlanet;
                ord.pt = app.state.rgptPlan[ord.id];
            }
        }
    }
ThwakSumthin:
    if (lpfl->cord > 1 && lpfl->lpplord->rgord()[1].grobj == grobjPlanet && ord.grobj == grobjPlanet) {
        return -1;
    }
    ord.grTask = grTaskNone;
    ord.fValidTask = 1;
    ord.iWarp = 4;
    if (!FMoveAiFleet(app, lpfl, &ord, 0)) {
        return -1;
    }
    return 0;
}

int16_t IdTargetFreighter(APPSESSION &app, FLEET *lpflFr, PLANET *lpplHome) {
    int32_t  lWorst2;
    int32_t  scoreBest;
    PLANET  *lpplMac;
    int32_t  score;
    POINT16  pt;
    int16_t  dy;
    int32_t  lWorst;
    int16_t  pctFull;
    int16_t  idBest;
    PLANET  *lppl;
    int32_t  wtPlanCargo;
    FLEET   *lpfl;
    int32_t  wtCargoMax;
    int16_t  ifl;
    int16_t  i;
    int16_t  iWorst2;
    int32_t  wtCargoFree;
    THING   *lpthBest;
    int16_t  iWorst;
    PLANET  *lpplBest;
    int16_t  ishFreighter;
    int16_t  pctHere;
    int16_t  dx;
    uint8_t *lpb;
    int16_t  fNeedy;
    ORDER    ord;
    int16_t  fSalvage;
    int32_t  l;

    fSalvage = 0;
    lpb = app.state.vlpbAiPlanet + 14;
    i = 0;
    while (i < app.state.game.cPlanMax) {
        *lpb = 0;
        i++;
        lpb += 16;
    }
    for (ishFreighter = 0; ishFreighter < 16 && lpflFr->rgcsh[ishFreighter] <= 0; ishFreighter++) {
    }
    lWorst = 1000000000;
    lWorst2 = 1000000000;
    for (i = 0; i < 3; i++) {
        if (lpplHome->rgwtMin[i] < lWorst) {
            lWorst2 = lWorst;
            iWorst2 = iWorst;
            lWorst = lpplHome->rgwtMin[i];
            iWorst = i;
        }
    }
    fNeedy = lWorst < (int32_t)(lWorst2 >> 1);
    if (lWorst < (int32_t)(lWorst2 >> 2)) {
        fNeedy++;
    }
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->iPlayer == app.state.idPlayer && lpfl->cord > 1 && lpfl != lpflFr && lpfl->lpplord->rgord()[1].grobj == grobjPlanet && lpfl->rgcsh[ishFreighter] != 0 &&
            lpfl->lpplord->rgord()[1].id != lpplHome->id) {
            app.state.vlpbAiPlanet[lpfl->lpplord->rgord()[1].id * 16 + 14] = 1;
        }
    }
    wtCargoMax = ProductionIO().pfnFleetStat(app, lpflFr, 2);
    wtCargoFree = GetCargoFree(app, lpflFr);
    if (wtCargoMax <= 0) {
        return idplNone;
    }
    pctFull = 100 - LOWORD((int32_t)((int32_t)(wtCargoFree * 100) / wtCargoMax));
    scoreBest = 0;
    idBest = idplNone;
    pt = lpflFr->pt;
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->id != lpflFr->idPlanet && app.state.vlpbAiPlanet[lppl->id * 16 + 14] == 0) {
            if (lppl->iPlayer == iplrNone && (app.state.vlpbAiPlanet[lppl->id * 16 + 1] & 0x80)) {
                dx = pt.x - app.state.rgptPlan[lppl->id].x;
                dy = pt.y - app.state.rgptPlan[lppl->id].y;
                l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                l = ((int32_t)sqrt((double)l) + 24) / 25;
                if (l < 1) {
                    l = 1;
                }
                score = (int32_t)((int16_t)((app.state.vlpbAiPlanet[lppl->id * 16 + 1] & 0x7f) * 0x1f4) / l);
            } else {
                if (lppl->iPlayer != app.state.idPlayer) {
                    if (app.state.rgplr[app.state.idPlayer].idAi == idAiRobotoid && lpplHome->id == lpflFr->idPlanet &&
                        ((lpplHome->rgwtMin[3] > 1100 && lppl->uPopGuess < 62) || (lpplHome->rgwtMin[3] > 300 && lppl->uPopGuess < 15))) {
                        pctHere = 65;
                        goto ScorePctHere;
                    }
                    if (app.state.vlpbAiPlanet[lppl->id * 16 + 3] == 0 || (app.state.vlpbAiPlanet[lppl->id * 16 + 3] & 0x80) || lpplHome->id != lpflFr->idPlanet)
                        continue;
                    score = 25000;
                }
                if (lppl == lpplHome) {
                    if (pctFull < 35)
                        continue;
                    if (pctFull == 100) {
                        score = 25000;
                        goto LScore;
                    }
                    dx = pt.x - app.state.rgptPlan[lppl->id].x;
                    dy = pt.y - app.state.rgptPlan[lppl->id].y;
                    l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                    l = ((int32_t)sqrt((double)l) + 24) / 25;
                    if (l == 0)
                        continue;
                    score = (int32_t)((int16_t)(20 * pctFull) / l);
                    goto LScore;
                }
                if (lppl->fStarbase || (lppl->lpplprod && lppl->lpplprod->rgprod()[0].grobj == grobjFleet && lppl->lpplprod->rgprod()[0].iItem >= iobjPacketGerm))
                    continue;
                if (app.state.vlpbAiPlanet[lppl->id * 16 + 2] != 0 && !(app.state.vlpbAiPlanet[lppl->id * 16 + 2] & 0x80) && lpplHome->id == lpflFr->idPlanet) {
                    score = 25000;
                    goto LScore;
                }
                if (fNeedy == 2) {
                    wtPlanCargo = lppl->rgwtMin[iWorst];
                } else {
                    wtPlanCargo = 0;
                    for (i = 0; i < 3; i++) {
                        if (fNeedy && i != iWorst) {
                            wtPlanCargo += (int32_t)(lppl->rgwtMin[i] >> 1);
                        } else {
                            wtPlanCargo += lppl->rgwtMin[i];
                        }
                    }
                }
                if (wtPlanCargo < 10)
                    continue;
                pctHere = LOWORD((int32_t)((int32_t)(wtPlanCargo * 100) / wtCargoMax));
                if (pctHere > 100 - pctFull) {
                    pctHere = 100 - pctFull;
                }
            ScorePctHere:
                dx = pt.x - app.state.rgptPlan[lppl->id].x;
                dy = pt.y - app.state.rgptPlan[lppl->id].y;
                l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                l = ((int32_t)sqrt((double)l) + 24) / 25;
                if (l < 1) {
                    l = 1;
                }
                score = (int32_t)((int16_t)(100 * pctHere) / l);
            }
        LScore:
            if (score > scoreBest) {
                scoreBest = score;
                lpplBest = lppl;
                idBest = lppl->id;
            }
        }
    }
    lpthBest = NULL;
    if (FSalvageTargetFreighter2(app, lpflFr, fNeedy, iWorst, pctFull, wtCargoMax, scoreBest, &lpthBest, &idBest) && GetCargoFree(app, lpflFr) == 0) {
        lpthBest = NULL;
        lpplBest = lpplHome;
        idBest = lpplHome->id;
    }
    if (idBest == idplNone) {
        return idplNone;
    }
    memset(&ord, 0, sizeof(ORDER));
    if (lpthBest) {
        ord.grobj = grobjThing;
        ord.pt = lpthBest->pt();
    } else {
        ord.grobj = grobjPlanet;
        ord.pt = app.state.rgptPlan[idBest];
    }
    ord.id = idBest;
    ord.grTask = grTaskXfer;
    ord.fValidTask = 1;
    ord.iWarp = 4;
    for (i = 0; i <= 2; i++) {
        ord.txp.rgia[i].SetIAction(!lpthBest && idBest == lpplHome->id ? 2 : 1);
    }
    if (app.state.rgplr[app.state.idPlayer].idAi == idAiTurinDrone) {
        if (lpplHome->id == lpflFr->idPlanet) {
            ChangeMainObjSel(app, grobjFleet, lpflFr->id);
            if (lpplHome->rgwtMin[3] > 1200 && !lpthBest && lpplBest->iPlayer != iplrNone && lpplBest->rgwtMin[3] < lpplHome->rgwtMin[3]) {
                XferAiSupply(app, grobjPlanet, lpflFr->idPlanet, grobjFleet, lpflFr->id, Colonists, 1000);
            }
            FleetQueryIO().pfnLookup(app, lpflFr->id, &app.state.sel.fl);
        }
        if (!lpthBest && lpplBest->iPlayer != iplrNone && lpplBest != lpplHome) {
            ord.txp.rgia[3].SetIAction(iActionUnloadAll);
        }
    } else if (app.state.rgplr[app.state.idPlayer].idAi == idAiRobotoid && lpplHome->id == lpflFr->idPlanet && !lpthBest && lpplBest->iPlayer != iplrNone) {
        if (lpplBest->iPlayer != app.state.idPlayer) {
            ChangeMainObjSel(app, grobjFleet, lpflFr->id);
            if (lpplHome->rgwtMin[3] > 900) {
                XferAiSupply(app, grobjPlanet, lpflFr->idPlanet, grobjFleet, lpflFr->id, Colonists, 300);
            } else {
                XferAiSupply(app, grobjPlanet, lpflFr->idPlanet, grobjFleet, lpflFr->id, Colonists, 100);
            }
            FleetQueryIO().pfnLookup(app, lpflFr->id, &app.state.sel.fl);
            ord.txp.rgia[3].SetIAction(iActionUnloadAll);
        } else if (lpplBest->rgwtMin[3] < 1000 && lpplHome->rgwtMin[3] > (int32_t)(lpplBest->rgwtMin[3] * 3) / 2 && lpplHome->rgwtMin[3] > 500) {
            if (lpplHome->rgwtMin[3] > 2000) {
                l = 10;
            } else if (lpplHome->rgwtMin[3] > 1500) {
                l = 8;
            } else if (lpplHome->rgwtMin[3] > 1000) {
                l = 5;
            } else {
                l = (int32_t)((lpplHome->rgwtMin[3] - 400) / 100);
            }
            l = (int32_t)(lpplHome->rgwtMin[3] * l) / 100;
            ChangeMainObjSel(app, grobjFleet, lpflFr->id);
            XferAiSupply(app, grobjPlanet, lpflFr->idPlanet, grobjFleet, lpflFr->id, Colonists, LOWORD(l));
            FleetQueryIO().pfnLookup(app, lpflFr->id, &app.state.sel.fl);
            ord.txp.rgia[3].SetIAction(iActionUnloadAll);
        }
    }
    if (lpthBest) {
        ord.grTask = grTaskNone;
        ord.fValidTask = 0;
    } else if (lpplHome->id != idBest && fNeedy) {
        if (fNeedy == 2 || lpplBest->rgwtMin[iWorst] >= wtCargoFree) {
            for (i = 0; i <= 2; i++) {
                if (i != iWorst) {
                    ord.txp.rgia[i].SetIAction(iActionNone);
                }
            }
        } else if (lpthBest || lpplBest->iPlayer != iplrNone) {
            for (i = 0; i <= 2; i++) {
                ord.txp.rgia[i].SetIAction(iActionFillPercent);
                if (i == iWorst) {
                    l = 66;
                } else {
                    l = 33;
                }
                ord.txp.rgia[i].SetCQuan(LOWORD(l));
            }
        }
    }
    if (!FMoveAiFleet(app, lpflFr, &ord, 0)) {
        return idplNone;
    }
    return idBest;
}

int16_t FSalvageTargetFreighter2(APPSESSION &app, FLEET *lpflFr, int16_t fNeedy, int16_t iWorst, int16_t pctFull, int32_t wtCargoMax, int32_t scoreBest, THING **plpthBest,
                                 int16_t *pidBest) {
    POINT16 pt;
    int32_t score;
    int16_t dy;
    int16_t i;
    int32_t wtPlanCargo;
    THING  *lpth;
    int16_t pctHere;
    int16_t dx;
    THING  *lpthMac;
    int16_t fSalvage;
    int32_t l;

    fSalvage = 0;
    pt = lpflFr->pt;
    lpth = app.state.lpThings;
    lpthMac = app.state.lpThings + app.state.cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith() == ithMineralPacket && lpth->thp().iWarp() == 0) {
            dx = pt.x - lpth->pt().x();
            dy = pt.y - lpth->pt().y();
            if (dx == 0 && dy == 0) {
                if (fNeedy && lpth->thp().rgwtMin(iWorst) != 0) {
                    XferAiSupply(app, grobjThing, lpth->idFull(), grobjFleet, lpflFr->id, static_cast<MineralType>(iWorst), lpth->thp().rgwtMin(iWorst));
                }
                if (fNeedy != 2) {
                    for (i = 0; i < 3; i++) {
                        if (lpth->thp().rgwtMin(i) != 0) {
                            XferAiSupply(app, grobjThing, lpth->idFull(), grobjFleet, lpflFr->id, static_cast<MineralType>(i), lpth->thp().rgwtMin(i));
                        }
                    }
                }
                fSalvage = 1;
                FleetQueryIO().pfnLookup(app, lpflFr->id, &app.state.sel.fl);
            } else if ((dx <= dy ? dy : dx) <= 200) {
                if (fNeedy == 2) {
                    wtPlanCargo = lpth->thp().rgwtMin(iWorst);
                } else {
                    wtPlanCargo = 0;
                    for (i = 0; i < 3; i++) {
                        if (fNeedy && i != iWorst) {
                            wtPlanCargo += (int16_t)(lpth->thp().rgwtMin(i) >> 1);
                        } else {
                            wtPlanCargo += lpth->thp().rgwtMin(i);
                        }
                    }
                }
                if (wtPlanCargo >= 10) {
                    pctHere = LOWORD((int32_t)((int32_t)(wtPlanCargo * 100) / wtCargoMax));
                    if (pctHere > 100 - pctFull) {
                        pctHere = 100 - pctFull;
                    }
                    l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                    l = ((int32_t)sqrt((double)l) + 24) / 25;
                    if (l < 1) {
                        l = 1;
                    }
                    score = (int32_t)((int16_t)(100 * pctHere) / l);
                    if (score > scoreBest) {
                        scoreBest = score;
                        *plpthBest = lpth;
                        *pidBest = lpth->idFull();
                    }
                }
            }
        }
    }
    return fSalvage;
}

int16_t FMoveAiFleet(APPSESSION &app, FLEET *lpfl, ORDER *pord, int16_t fAppend) {
    int16_t iord;

    ChangeMainObjSel(app, grobjFleet, lpfl->id);
    if (app.state.sel.fl.lpplord->iordMax == app.state.sel.fl.cord) {
        app.state.sel.fl.lpplord = LpplReAlloc(app, app.state.sel.fl.lpplord, app.state.sel.fl.cord + 1);
    }
    iord = !fAppend ? 0 : app.state.sel.fl.cord - 1;
    if (pord->pt.x != app.state.sel.fl.lpplord->rgord()[iord].pt.x || pord->pt.y != app.state.sel.fl.lpplord->rgord()[iord].pt.y) {
        iord++;
    } else if (app.state.sel.fl.cord > 1) {
        app.state.sel.fl.cord = 1;
        FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
    }
    app.state.sel.fl.lpplord->rgord()[iord] = *pord;
    app.state.sel.fl.cord = iord + 1;
    app.state.sel.fl.lpplord->iordMac = iord + 1;
    return FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
}

void AddItemToQueue(APPSESSION &app, uint16_t iItem, uint16_t cItem, GrobjClass grobj, AddItemMode mdAddItem) {
    int16_t fSingle;
    int16_t iprod;
    int16_t i;
    PROD    rgprod[64];

    fSingle = 0;
    if (!app.state.sel.pl.lpplprod || app.state.sel.pl.lpplprod->iprodMac <= 200) {
        if (!app.state.lpplProdGlob) {
            fSingle = 1;
            ProductionIO().pfnInitialize(app, rgprod);
            for (i = 0; i < app.state.cProdGlob && (rgprod[i].iItem != (uint32_t)iItem || rgprod[i].grobj != (uint32_t)grobj); i++) {
            }
            if (i >= app.state.cProdGlob) {
                cItem = 0;
            } else {
                cItem = cItem >= rgprod[i].cItem ? rgprod[i].cItem : cItem;
            }
        }
        if (cItem > 0) {
            if (mdAddItem != addItemReplace && app.state.lpplProdGlob->iprodMac == app.state.lpplProdGlob->iprodMax) {
                app.state.lpplProdGlob = LpplReAlloc(app, app.state.lpplProdGlob, app.state.lpplProdGlob->iprodMac + 3);
            }
            switch (mdAddItem) {
            case addItemReplace:
                app.state.lpplProdGlob->iprodMac = 0;
                /* fallthrough */
            case addItemEnd:
                iprod = app.state.lpplProdGlob->iprodMac;
                break;
            case addItemFront:
                iprod = 0;
                memmove(&app.state.lpplProdGlob->rgprod()[1], app.state.lpplProdGlob->rgprod(), app.state.lpplProdGlob->iprodMac * sizeof(PROD));
                memset(app.state.lpplProdGlob->rgprod(), 0, 4);
            }
            app.state.lpplProdGlob->rgprod()[iprod].cItem = cItem;
            app.state.lpplProdGlob->rgprod()[iprod].iItem = iItem;
            app.state.lpplProdGlob->rgprod()[iprod].grobj = grobj;
            app.state.lpplProdGlob->rgprod()[iprod].pct = 0;
            app.state.lpplProdGlob->rgprod()[iprod].unused = 0;
            app.state.lpplProdGlob->iprodMac++;
        }
        if (fSingle) {
            FinishProduction(app, cItem != 0);
        }
    }
    return;
}

int16_t IroEnsureAi(APPSESSION &app, uint8_t *lpbRes, int16_t cRes, int16_t *pishdefSBLatest, int16_t pct) {
    int16_t iSmallest;
    int16_t i;
    int16_t pctTech;
    int16_t ilvl;

    if (pishdefSBLatest) {
        EnsureAiStarbaseDesigns(app);
        *pishdefSBLatest = IshdefAiSBLatest(app);
        ValidateStarbaseHistory(app);
    }
    app.state.rgplr[app.state.idPlayer].pctResearch = pct;
    for (i = 0; i < 6 && app.state.rgplr[app.state.idPlayer].rgTech[i] >= 24; i++) {
    }
    if (i == 6) {
        app.state.rgplr[app.state.idPlayer].pctResearch = 0;
        pctTech = app.state.rgplr[app.state.idPlayer].iTechCur * 256;
        WriteMemResearch(app, pctTech);
    }
    for (i = 0; i < cRes; i++) {
        ilvl = app.state.rgplr[app.state.idPlayer].rgTech[lpbRes[i] >> 5];
        if (ilvl < (lpbRes[i] & 0x1f)) {
            app.state.rgplr[app.state.idPlayer].SetITechNow(lpbRes[i] >> 5);
            if (i < cRes - 1 && ilvl + 1 == (lpbRes[i] & 0x1f)) {
                app.state.rgplr[app.state.idPlayer].SetITechNext(lpbRes[i + 1] >> 5);
            }
            pctTech = app.state.rgplr[app.state.idPlayer].pctResearch + app.state.rgplr[app.state.idPlayer].iTechCur * 256;
            WriteMemResearch(app, pctTech);
            return i;
        }
    }
    iSmallest = 0;
    for (i = 1; i < 6; i++) {
        if (app.state.rgplr[app.state.idPlayer].rgTech[i] < app.state.rgplr[app.state.idPlayer].rgTech[iSmallest]) {
            iSmallest = i;
        }
    }
    if (app.state.rgplr[app.state.idPlayer].iTechNow() != iSmallest) {
        app.state.rgplr[app.state.idPlayer].SetITechNow(iSmallest);
        if (app.state.rgplr[app.state.idPlayer].iTechNow() == 0x1a) {
            app.state.rgplr[app.state.idPlayer].pctResearch = 0;
        }
        pctTech = app.state.rgplr[app.state.idPlayer].pctResearch + app.state.rgplr[app.state.idPlayer].iTechCur * 256;
        WriteMemResearch(app, pctTech);
    }
    return 926;
}

void KeepFleetsMoving(APPSESSION &app) {
    int16_t i;
    int16_t ifl;
    FLEET  *lpfl;
    THING  *lpth;
    THING  *rglpth[100];
    int16_t ith;
    THING  *lpthMac;

    ith = 0;
    lpth = app.state.lpThings;
    lpthMac = app.state.lpThings + app.state.cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith() == ithMinefield && lpth->iplr() != app.state.idPlayer && lpth->thm().iType() != mineSpeedBump) {
            if (ith == 100) {
                ith = -1;
                goto LKeepMovn;
            }
            rglpth[ith++] = lpth;
        }
    }
LKeepMovn:
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->iPlayer == app.state.idPlayer && lpfl->cord > 1) {
            if (app.state.rgplr[app.state.idPlayer].idAi != idAiRobotoid && app.state.game.turn < 5) {
                i = 16;
            } else {
                i = -1;
                if (FIsAiTransport(app, lpfl)) {
                    i = 30;
                }
            }
            SetAiFleetIdealSpeed(app, lpfl, i, ith, rglpth);
        }
    }
    return;
}

int16_t FShouldWeBuildColonizers(APPSESSION &app, int16_t *pcCol) {
    int16_t  iMax;
    int16_t  cColFl;
    uint32_t cBuilt;
    int16_t  iMin;
    int16_t  ifl;
    int16_t  i;
    FLEET   *lpfl;
    int16_t  rgish[16];
    int16_t  cColPl;

    cColFl = 0;
    cColPl = 0;
    cBuilt = 0;
    iMax = -1;
    iMin = 16;
    if (pcCol) {
        *pcCol = 0;
    }
    if (app.state.rgplr[app.state.idPlayer].lvlAi == lvlAiEasy && (app.state.game.turn & 1)) {
        return 0;
    }
    if (app.state.game.turn < 30) {
        return 1;
    }
    for (i = 0; i < 16; i++) {
        if (!app.state.rgshdef[i].fFree && (app.state.rgshdef[i].hul.ihuldef == ihuldefMiniColonyShip || app.state.rgshdef[i].hul.ihuldef == ihuldefColonyShip)) {
            rgish[i] = 1;
            if (i > iMax) {
                iMax = i;
            }
            if (i < iMin) {
                iMin = i;
            }
        } else {
            rgish[i] = 0;
        }
    }
    iMax++;
    if (iMax <= 0) {
        return 0;
    }
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->iPlayer == app.state.idPlayer) {
            for (i = iMin; i < iMax; i++) {
                if (rgish[i] > 0 && lpfl->rgcsh[i] > 0) {
                    cColFl++;
                    break;
                }
            }
        }
    }
    if (pcCol) {
        *pcCol = cColFl;
    }
    if (cColFl > 20 * app.state.game.mdSize + 10) {
        return 0;
    }
    for (i = 0; i < app.state.game.cPlayer; i++) {
        if (app.state.rgplr[i].fInclude) {
            cColPl += app.state.rgplr[i].cPlanet;
        }
    }
    if (cColFl + cColPl > (int16_t)(app.state.game.cPlanMax * 4) / 5) {
        return 0;
    }
    cBuilt = 0;
    for (i = iMin; i < iMax; i++) {
        if (rgish[i] > 0) {
            cBuilt += app.state.rgshdef[i].cBuilt;
        }
    }
    if (cBuilt - (int16_t)(cColPl + cColFl) > 25 && (cColFl > 4 || RandomIO().pfnRandom(app, 2) != 0)) {
        return 0;
    }
    return 1;
}

int16_t FIsAiAttack(APPSESSION &app, FLEET *lpfl) {
    HulDef  ihul;
    int16_t i;

    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            ihul = app.state.rgshdef[i].hul.ihuldef;
            if ((int16_t)ihul > ihuldefFrigate && (int16_t)ihul <= ihuldefDreadnought) {
                return 1;
            }
            switch (ihul) {
            default:
                break;
            case ihuldefFrigate:
                if (app.state.rglpshdef[app.state.idPlayer][i].lPower > 0) {
                    return 1;
                }
                return 0;
            case ihuldefMetaMorph:
            case ihuldefNubian:
                if (WtMaxShdefStat(&app.state.rgshdef[i], 2) < 500 && app.state.rglpshdef[app.state.idPlayer][i].lPower > 0) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

int16_t FIsTurinDroneAiAttack(APPSESSION &app, FLEET *lpfl) {
    HulDef  ihul;
    int16_t i;

    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            ihul = app.state.rgshdef[i].hul.ihuldef;
            if ((int16_t)ihul >= ihuldefScout && (int16_t)ihul <= ihuldefDreadnought) {
                return 1;
            }
        }
    }
    return 0;
}

int16_t FIsAiTransport(APPSESSION &app, FLEET *lpfl) {
    HulDef  ihul;
    int16_t i;

    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            ihul = app.state.rgshdef[i].hul.ihuldef;
            if (((int16_t)ihul >= ihuldefSmallFreighter && (int16_t)ihul <= ihuldefSuperFreighter) ||
                ((int16_t)ihul >= ihuldefPrivateer && (int16_t)ihul <= ihuldefGalleon)) {
                return 1;
            }
            if (ihul == ihuldefMetaMorph && WtMaxShdefStat(&app.state.rgshdef[i], 2) >= 500 && app.state.rglpshdef[app.state.idPlayer][i].lPower > 0) {
                return 1;
            }
        }
    }
    return 0;
}

void ValidateStarbaseHistory(APPSESSION &app) {
    int16_t iWrite;
    int16_t iBest;
    PLANET *lpplMac;
    POINT16 pt;
    int16_t id;
    int16_t dy;
    int16_t cFr2;
    PLANET *lppl;
    int16_t ifl;
    FLEET  *lpfl;
    int16_t i;
    int16_t j;
    int16_t ipl;
    int16_t cFr;
    int16_t dx;
    int32_t lBest;
    int32_t l;

    if (app.state.rgplr[app.state.idPlayer].idAi != idAiCybertron && app.state.rgplr[app.state.idPlayer].idAi != idAiMacinti) {
        if (AhvFromLp(app, app.state.vlpbAiData).cbAiHist() <= 2) {
            AhvFromLp(app, app.state.vlpbAiData).SetCStarbase(0);
        }
        if (app.state.game.turn >= 20) {
            iWrite = 0;
            if (AhvFromLp(app, app.state.vlpbAiData).cStarbase() < 0 || AhvFromLp(app, app.state.vlpbAiData).cStarbase() > 64) {
                AhvFromLp(app, app.state.vlpbAiData).SetCStarbase(0);
            }
            for (i = 0; i < AhvFromLp(app, app.state.vlpbAiData).cStarbase(); i++) {
                if (AhvFromLp(app, app.state.vlpbAiData).rgasb(i).cFreighter() < 0 || AhvFromLp(app, app.state.vlpbAiData).rgasb(i).cFreighter() > 8) {
                    AhvFromLp(app, app.state.vlpbAiData).rgasb(i).SetCFreighter(0);
                }
                lppl = LpplFromId(app, AhvFromLp(app, app.state.vlpbAiData).rgasb(i).idPlanet());
                if (lppl && lppl->iPlayer == app.state.idPlayer) {
                    if (iWrite != i) {
                        AhvFromLp(app, app.state.vlpbAiData).rgasb(iWrite).SetValue(AhvFromLp(app, app.state.vlpbAiData).rgasb(i).ReadValue());
                    }
                    iWrite++;
                }
            }
            AhvFromLp(app, app.state.vlpbAiData).SetCStarbase(iWrite);
            for (ipl = 0; ipl < app.state.vclpplAi; ipl++) {
                lppl = app.state.vrglpplAi[ipl];
                if (!app.state.vrglpplAi[ipl])
                    break;
                if (lppl->fStarbase) {
                    for (i = 0; i < AhvFromLp(app, app.state.vlpbAiData).cStarbase() && AhvFromLp(app, app.state.vlpbAiData).rgasb(i).idPlanet() != lppl->id; i++) {
                    }
                    if (i == AhvFromLp(app, app.state.vlpbAiData).cStarbase() && i < 64) {
                        AhvFromLp(app, app.state.vlpbAiData).rgasb(i).SetIdPlanet(lppl->id);
                        AhvFromLp(app, app.state.vlpbAiData).rgasb(i).SetCFreighter(0);
                        AhvFromLp(app, app.state.vlpbAiData).SetCStarbase(AhvFromLp(app, app.state.vlpbAiData).cStarbase() + 1);
                    }
                }
            }
            lppl = app.state.lpPlanets;
            lpplMac = app.state.lpPlanets + app.state.cPlanet;
            for (; lppl < lpplMac && AhvFromLp(app, app.state.vlpbAiData).cStarbase() < 64; lppl++) {
                if (lppl->iPlayer == app.state.idPlayer && !lppl->fStarbase && lppl->rgwtMin[3] >= 80) {
                    l = 0;
                    for (i = 0; i < 3; i++) {
                        l += (int32_t)((uint32_t)((uint32_t)lppl->rgMinConc[i] * (uint32_t)lppl->rgMinConc[i]) * 4) + lppl->rgwtMin[i];
                    }
                    if (l >= 7000 && lppl->cMines >= 20 && lppl->cFactories >= 20) {
                        for (i = 0; i < AhvFromLp(app, app.state.vlpbAiData).cStarbase(); i++) {
                            id = AhvFromLp(app, app.state.vlpbAiData).rgasb(i).idPlanet();
                            if (id == lppl->id)
                                break;
                            if (app.state.rgplr[app.state.idPlayer].idAi == idAiRobotoid) {
                                pt = app.state.rgptPlan[id];
                                dx = pt.x - app.state.rgptPlan[lppl->id].x;
                                dy = pt.y - app.state.rgptPlan[lppl->id].y;
                                l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                                if (l < 2500)
                                    break;
                            }
                        }
                        if (i >= AhvFromLp(app, app.state.vlpbAiData).cStarbase()) {
                            AhvFromLp(app, app.state.vlpbAiData).rgasb(AhvFromLp(app, app.state.vlpbAiData).cStarbase()).SetIdPlanet(lppl->id);
                            AhvFromLp(app, app.state.vlpbAiData).rgasb(AhvFromLp(app, app.state.vlpbAiData).cStarbase()).SetCFreighter(0);
                            AhvFromLp(app, app.state.vlpbAiData).SetCStarbase(AhvFromLp(app, app.state.vlpbAiData).cStarbase() + 1);
                        }
                    }
                }
            }
            for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                lpfl = app.state.rglpfl[ifl];
                if (!app.state.rglpfl[ifl])
                    break;
                if (lpfl->iPlayer == app.state.idPlayer && FIsAiTransport(app, lpfl)) {
                    for (i = 0; i < AhvFromLp(app, app.state.vlpbAiData).cStarbase(); i++) {
                        for (j = 0; j < AhvFromLp(app, app.state.vlpbAiData).rgasb(i).cFreighter() && AhvFromLp(app, app.state.vlpbAiData).rgasb(i).rgflid(j) != lpfl->id; j++) {
                        }
                        if (j < AhvFromLp(app, app.state.vlpbAiData).rgasb(i).cFreighter())
                            break;
                    }
                    if (i == AhvFromLp(app, app.state.vlpbAiData).cStarbase()) {
                        iBest = -1;
                        lBest = 10000000;
                        for (i = 0; i < AhvFromLp(app, app.state.vlpbAiData).cStarbase(); i++) {
                            if (AhvFromLp(app, app.state.vlpbAiData).rgasb(i).cFreighter() < 8) {
                                pt = app.state.rgptPlan[AhvFromLp(app, app.state.vlpbAiData).rgasb(i).idPlanet()];
                                dx = pt.x - lpfl->pt.x;
                                dy = pt.y - lpfl->pt.y;
                                l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                                if (l < lBest) {
                                    lBest = l;
                                    iBest = i;
                                }
                            }
                        }
                        if (iBest != -1) {
                            AhvFromLp(app, app.state.vlpbAiData).rgasb(iBest).SetNextFlid(lpfl->id);
                        }
                    }
                }
            }
            for (i = 0; i < AhvFromLp(app, app.state.vlpbAiData).cStarbase(); i++) {
                cFr = AhvFromLp(app, app.state.vlpbAiData).rgasb(i).cFreighter();
                if (cFr < 4) {
                    for (j = 0; j < AhvFromLp(app, app.state.vlpbAiData).cStarbase(); j++) {
                        cFr2 = AhvFromLp(app, app.state.vlpbAiData).rgasb(j).cFreighter();
                        if (cFr2 >= cFr + 2)
                            break;
                    }
                    if (j < AhvFromLp(app, app.state.vlpbAiData).cStarbase()) {
                        AhvFromLp(app, app.state.vlpbAiData).rgasb(j).SetCFreighter(AhvFromLp(app, app.state.vlpbAiData).rgasb(j).cFreighter() - 1);
                        AhvFromLp(app, app.state.vlpbAiData).rgasb(i).SetNextFlid(
                            AhvFromLp(app, app.state.vlpbAiData).rgasb(j).rgflid(AhvFromLp(app, app.state.vlpbAiData).rgasb(j).cFreighter()));
                    }
                }
            }
            AhvFromLp(app, app.state.vlpbAiData).SetCbAiHist(AhvFromLp(app, app.state.vlpbAiData).cStarbase() * 20 + 4);
        }
    }
    return;
}

void GetResourcesAvailable(APPSESSION &app, PLANET *lppl, int32_t *rgRes) {
    int16_t i;
    int32_t cRes;

    ColonyIO().pfnEstimateMining(app, lppl, rgRes, -1, 0);
    for (i = 0; i < 3; i++) {
        rgRes[i] += lppl->rgwtMin[i];
    }
    cRes = ResearchIO().pfnResources(app, lppl, app.state.idPlayer);
    if (!app.state.sel.pl.fNoResearch) {
        cRes -= (int32_t)(cRes * (int16_t)app.state.rgplr[app.state.idPlayer].pctResearch) / 100;
    }
    rgRes[3] = cRes;
    return;
}

void GetProdQCost(APPSESSION &app, PLANET *lppl, int32_t *rgCost) {
    int32_t rgCostCur[4];
    PLPROD *lpplprod;
    int16_t i;
    int16_t j;
    PROD   *lpprod;

    for (i = 0; i < 4; i++) {
        rgCost[i] = 0;
    }
    if (lppl->lpplprod) {
        lpplprod = lppl->lpplprod;
        i = 0;
        lpprod = lpplprod->rgprod();
        while (i < lpplprod->iprodMac) {
            ProductionIO().pfnQuote(app, lppl, lpprod, (uint32_t *)rgCostCur, app.state.idPlayer, 0);
            for (j = 0; j < 4; j++) {
                rgCost[j] += rgCostCur[j];
            }
            i++;
            lpprod++;
        }
    }
    return;
}

void MergeAllShdefs(APPSESSION &app, int16_t grbitish) {
    int16_t crglpflW;
    FLEET  *rglpflW[32];
    int16_t iMax;
    int16_t iMin;
    int16_t i;
    int16_t ifl;
    FLEET  *lpfl;
    int16_t grbit;
    int16_t rgish[16];
    FLEET  *lpflNextPass;
    int16_t iflNextPass;

    iMax = -1;
    iMin = 16;
    i = 0;
    grbit = 1;
    while (i < 16) {
        if (grbitish & grbit) {
            rgish[i] = 1;
            if (i > iMax) {
                iMax = i;
            }
            if (i < iMin) {
                iMin = i;
            }
        } else {
            rgish[i] = 0;
        }
        i++;
        grbit *= 2;
    }
    iMax++;
    if (iMax > 0) {
        lpflNextPass = NULL;
        crglpflW = 0;
        for (ifl = 0; ifl < app.state.cFleet; ifl++) {
            lpfl = app.state.rglpfl[ifl];
            if (!app.state.rglpfl[ifl])
                break;
        NextPass:
            if (lpfl->iPlayer == app.state.idPlayer) {
                for (i = iMin; i < iMax && (lpfl->rgcsh[i] <= 0 || rgish[i] <= 0); i++) {
                }
                if (i != iMax && ColonyIO().pfnFleetMining(app, lpfl) != 4000) {
                    for (i = 0; i < crglpflW && (rglpflW[i]->idPlanet != lpfl->idPlanet || rglpflW[i]->pt.x != lpfl->pt.x || rglpflW[i]->pt.y != lpfl->pt.y);
                         i++) {
                    }
                    if (i < crglpflW) {
                        Merge2Fleets(app, rglpflW[i], lpfl, 0);
                    } else if (crglpflW != 32) {
                        rglpflW[crglpflW++] = lpfl;
                    } else if (!lpflNextPass) {
                        lpflNextPass = lpfl;
                        iflNextPass = ifl;
                    }
                }
            }
        }
        if (lpflNextPass) {
            lpfl = lpflNextPass;
            lpflNextPass = NULL;
            ifl = iflNextPass;
            crglpflW = 0;
            goto NextPass;
        }
    }
    return;
}

FLEET *LpflFindClosestEnum(APPSESSION &app, FLEET *lpfl, int16_t (*pfn)(APPSESSION &, FLEET *, FLEET *)) {
    FLEET  *lpflT;
    POINT16 pt;
    int16_t dy;
    int16_t ish;
    int16_t dx;
    FLEET  *lpflBest;
    int32_t l;
    int32_t lBest;

    lBest = 10000000;
    lpflBest = NULL;
    pt = lpfl->pt;
    for (ish = 0; ish < app.state.cFleet; ish++) {
        lpflT = app.state.rglpfl[ish];
        if (!app.state.rglpfl[ish])
            break;
        dx = pt.x - lpflT->pt.x;
        dy = pt.y - lpflT->pt.y;
        l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        if (l < lBest && pfn(app, lpfl, lpflT) != 0) {
            lpflBest = lpflT;
            lBest = l;
        }
    }
    return lpflBest;
}

PLANET *LpplFindClosestEnum(APPSESSION &app, PLANET *lppl, int16_t (*pfn)(APPSESSION &, PLANET *, PLANET *)) {
    POINT16 pt;
    int16_t dy;
    PLANET *lpplTMac;
    PLANET *lpplBest;
    PLANET *lpplT;
    int16_t dx;
    int32_t l;
    int32_t lBest;

    lBest = 10000000;
    lpplBest = NULL;
    pt = app.state.rgptPlan[lppl->id];
    lpplT = app.state.lpPlanets;
    lpplTMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lpplT < lpplTMac; lpplT++) {
        dx = pt.x - app.state.rgptPlan[lpplT->id].x;
        dy = pt.y - app.state.rgptPlan[lpplT->id].y;
        l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        if (l < lBest && pfn(app, lppl, lpplT) != 0) {
            lpplBest = lpplT;
            lBest = l;
        }
    }
    return lpplBest;
}

PLANET *LpplFindBestEnum(APPSESSION &app, PLANET *lppl, int16_t (*pfn)(APPSESSION &, PLANET *, PLANET *)) {
    int16_t iBest;
    POINT16 pt;
    int16_t dy;
    PLANET *lpplTMac;
    PLANET *lpplBest;
    PLANET *lpplT;
    int16_t iCur;
    int16_t dx;
    int32_t l;
    int32_t lBest;

    iBest = 1;
    lBest = 10000000;
    lpplBest = NULL;
    pt = app.state.rgptPlan[lppl->id];
    lpplT = app.state.lpPlanets;
    lpplTMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lpplT < lpplTMac; lpplT++) {
        dx = pt.x - app.state.rgptPlan[lpplT->id].x;
        dy = pt.y - app.state.rgptPlan[lpplT->id].y;
        l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        iCur = pfn(app, lppl, lpplT);
        if (iCur > iBest || (iCur == iBest && l < lBest)) {
            iBest = iCur;
            lBest = l;
            lpplBest = lpplT;
        }
    }
    return lpplBest;
}

int16_t IdRandomPlanetNearby(APPSESSION &app, POINT16 pt, int16_t cDist, int16_t fAvoidStarbases) {
    int16_t iChance;
    int32_t lDistMax;
    int32_t dy;
    int16_t idBest;
    int16_t i;
    int32_t dx;
    int16_t cExtraAttempts;
    int32_t d2Cur;
    PLANET *lppl;

    cExtraAttempts = !fAvoidStarbases ? 0 : 2;
LRetry:
    lDistMax = (uint32_t)(cDist * cDist);
    iChance = 1;
    idBest = idplNone;
    for (i = 0; i < app.state.game.cPlanMax; i++) {
        dx = abs(pt.x - app.state.rgptPlan[i].x);
        dy = abs(pt.y - app.state.rgptPlan[i].y);
        d2Cur = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        if (d2Cur <= lDistMax && RandomIO().pfnRandom(app, iChance++) == 0) {
            idBest = i;
        }
    }
    if (idBest != idplNone && cExtraAttempts-- > 0) {
        lppl = LpplFromId(app, idBest);
        if (lppl && lppl->fStarbase)
            goto LRetry;
    }
    return idBest;
}

void ClearAiCurrentTask(APPSESSION &app, FLEET *lpfl, int16_t fChangeSel) {
    if (fChangeSel) {
        ChangeMainObjSel(app, grobjFleet, lpfl->id);
    }
    app.state.sel.fl.lpplord->rgord()[0].grTask = grTaskNone;
    FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
    return;
}

int16_t FEnumOurStarbase(APPSESSION &app, PLANET *lpplSrc, PLANET *lpplTest) {
    if (lpplTest->iPlayer == app.state.idPlayer && lpplTest->fStarbase) {
        return 1;
    }
    return 0;
}

int16_t FFleetMightHaveTeeth(APPSESSION &app, FLEET *lpfl) {
    HUL    *lphul;
    int16_t ishdef;

    for (ishdef = 0; ishdef < 16; ishdef++) {
        if (lpfl->rgcsh[ishdef] != 0) {
            lphul = &app.state.rglpshdef[lpfl->iplr()][ishdef].hul;
            if (FHullHasTeeth(lphul)) {
                return 1;
            }
        }
    }
    return 0;
}

int16_t IdTargetScout(APPSESSION &app, FLEET *lpfl, FLEET *lpflAtk, FLEET *lpflEnemy, int16_t fOnlyHumans, THING **plpthWorm) {
    FLEET  *lpflClosest;
    FLEET  *lpflT;
    int32_t lDistBest;
    PLANET *lpplMac;
    POINT16 pt;
    int16_t dy;
    PLANET *lppl;
    int32_t lDist;
    int16_t idClosest;
    PLANET *lpplClosest;
    int16_t dx;
    FLEET  *lpflAtk2;
    ORDER   ord;

    lpflClosest = NULL;
    lpflT = lpflEnemy;
    lDistBest = 1000000;
    pt = lpfl->pt;
    memset(&ord, 0, sizeof(ORDER));
    if (!FFleetMightHaveTeeth(app, lpfl))
        goto LFindPlanet;
    for (; lpflT; lpflT = lpflT->lpflNext) {
        if (fOnlyHumans && app.state.rgplr[lpflT->iPlayer].fAi)
            goto NextTarg;
        for (lpflAtk2 = lpflAtk; lpflAtk2 && (lpflAtk2 == lpfl || lpflAtk2->cord < 2 || lpflAtk2->lpplord->rgord()[1].id != lpflT->id ||
                                              lpflAtk2->lpplord->rgord()[1].grobj != grobjFleet);
             lpflAtk2 = lpflAtk2->lpflNext) {
        }
        if (lpflAtk2 && RandomIO().pfnRandom(app, 3) == 0)
            goto NextTarg;
        dx = lpflT->pt.x - pt.x;
        dy = lpflT->pt.y - pt.y;
        lDist = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        if (lDist < lDistBest) {
            lDistBest = lDist;
            lpflClosest = lpflT;
        }
    NextTarg:;
    }
    if (lDistBest < 32400) {
        ord.pt = lpflClosest->pt;
        ord.grobj = grobjFleet;
        ord.id = lpflClosest->id;
        goto ThwakSumthin;
    }
    if (lpflClosest) {
        if (lpfl->rgwtMin[4] < (int32_t)(ProductionIO().pfnFleetStat(app, lpfl, 1) / 2) && FMoveToNearestStarbase(app, lpfl, 0)) {
            return 0;
        }
        pt = lpflClosest->pt;
        lpplClosest = NULL;
        lDistBest = 1000000;
        lppl = app.state.lpPlanets;
        lpplMac = app.state.lpPlanets + app.state.cPlanet;
        for (; lppl < lpplMac; lppl++) {
            for (lpflAtk2 = lpflAtk; lpflAtk2 && (lpflAtk2 == lpfl || lpflAtk2->cord < 2 || lpflAtk2->lpplord->rgord()[1].id != lppl->id ||
                                                  lpflAtk2->lpplord->rgord()[1].grobj != grobjPlanet);
                 lpflAtk2 = lpflAtk2->lpflNext) {
            }
            if (!lpflAtk2) {
                dx = app.state.rgptPlan[lppl->id].x - pt.x;
                dy = app.state.rgptPlan[lppl->id].y - pt.y;
                lDist = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                if (lDist < lDistBest) {
                    lDistBest = lDist;
                    lpplClosest = lppl;
                }
            }
        }
        if (lpplClosest && lpplClosest->id != lpfl->idPlanet) {
            ord.pt = app.state.rgptPlan[lpplClosest->id];
            ord.grobj = grobjPlanet;
            ord.id = lpplClosest->id;
            goto ThwakSumthin;
        }
    } else if (fOnlyHumans) {
        return IdTargetScout(app, lpfl, lpflAtk, lpflEnemy, 0, plpthWorm);
    }
LFindPlanet:
    idClosest = IdNearestUnknownPlanet(app, lpfl, plpthWorm);
    if (plpthWorm && *plpthWorm) {
        ord.pt = (*plpthWorm)->pt();
        ord.grobj = grobjThing;
        ord.id = (*plpthWorm)->idFull();
    } else {
        if (idClosest == idplNone) {
            if (app.state.rgplr[app.state.idPlayer].idPlanetHome == idplNone) {
                idClosest = RandomIO().pfnRandom(app, app.state.game.cPlanMax);
            } else {
                lpplClosest = LpplFindBestEnum(app, LpplFromId(app, app.state.rgplr[app.state.idPlayer].idPlanetHome), FEnumCalcArmadaDest);
                if (!lpplClosest) {
                    idClosest = RandomIO().pfnRandom(app, app.state.game.cPlanMax);
                } else {
                    idClosest = lpplClosest->id;
                }
            }
        }
        ord.id = idClosest;
        ord.grobj = grobjPlanet;
        ord.pt = app.state.rgptPlan[idClosest];
    }
ThwakSumthin:
    if (lpfl->cord > 1 && lpfl->lpplord->rgord()[1].grobj == grobjPlanet && ord.grobj == grobjPlanet) {
        return -1;
    }
    if (ord.grobj == grobjPlanet) {
        app.state.vlpbAiPlanet[ord.id * 16 + 15] = 4;
    }
    ord.grTask = grTaskNone;
    ord.fValidTask = 1;
    ord.iWarp = (uint16_t)IFindIdealWarp(app, lpfl, 1);
    if (!FMoveAiFleet(app, lpfl, &ord, 0)) {
        return -1;
    }
    return 0;
}

void MarkPlanetsUnderAttack(APPSESSION &app) {
    PLANET *lppl;
    int16_t i;
    int16_t ifl;
    FLEET  *lpfl;
    int16_t j;

    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->iPlayer != app.state.idPlayer && lpfl->idPlanet != idPlanetDeepSpace) {
            j = lpfl->iPlayer;
            for (i = 0; i < 16 && (lpfl->rgcsh[i] <= 0 || (int16_t)app.state.rglpshdef[j][i].hul.ihuldef < ihuldefMiniBomber ||
                                   (int16_t)app.state.rglpshdef[j][i].hul.ihuldef > ihuldefB52Bomber);
                 i++) {
            }
            if (i != 16) {
                lppl = LpplFromId(app, lpfl->idPlanet);
                if (lppl->iPlayer == app.state.idPlayer) {
                    app.state.vlpbAiPlanet[lppl->id * 16 + 8] = 1;
                }
            }
        }
    }
    return;
}

void FixPlanetsUnderAttack(APPSESSION &app, PROD *rgprod) {
    PLANET *lppl;
    int16_t ipl;

    for (ipl = 0; ipl < app.state.vclpplAi; ipl++) {
        lppl = app.state.vrglpplAi[ipl];
        if (!app.state.vrglpplAi[ipl])
            break;
        if (app.state.vlpbAiPlanet[lppl->id * 16 + 8] != 0) {
            QuickBuildDefenses(app, lppl, rgprod);
        }
    }
    return;
}

void QuickBuildDefenses(APPSESSION &app, PLANET *lppl, PROD *rgprod) {
    int16_t cMax;
    int16_t cAlch;
    int16_t cCur;
    int16_t i;
    int16_t cRes;
    int32_t lVal;
    int16_t cDef;
    int32_t rgRes[4];
    PROD   *lpprod;

    if (lppl->lpplprod) {
        i = 0;
        lpprod = lppl->lpplprod->rgprod();
        while (i < lppl->lpplprod->iprodMac) {
            if (lpprod->grobj == grobjPlanet && lpprod->iItem == mdIdleDefense) {
                return;
            }
            i++;
            lpprod++;
        }
    }
    GetResourcesAvailable(app, lppl, rgRes);
    if (rgRes[3] >= 50) {
        ChangeMainObjSel(app, grobjPlanet, lppl->id);
        ProductionIO().pfnInitialize(app, rgprod);
        for (i = 0; i < app.state.cProdGlob && (app.state.pProdGlob[i].grobj != grobjPlanet || app.state.pProdGlob[i].iItem != mdIdleDefense || app.state.pProdGlob[i].cItem < 1); i++) {
        }
        if (i == app.state.cProdGlob) {
            FinishProduction(app, 0);
        } else {
            cMax = app.state.pProdGlob[i].cItem;
            cCur = 100;
            for (i = 0; i < 3; i++) {
                lVal = (int32_t)(rgRes[i] / 5);
                if (lVal < cCur) {
                    cCur = LOWORD(lVal);
                }
            }
            cRes = LOWORD((int32_t)(rgRes[3] / 25));
            if (cRes > 5) {
                cRes -= cRes / 6;
            }
            rgRes[3] -= (int32_t)(rgRes[3] / 10);
            if (cRes > cMax) {
                cRes = cMax;
            }
            if (cRes > cCur) {
                cAlch = LOWORD((int32_t)((rgRes[3] - (int16_t)((RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceMineralAlchemy) == 0 ? 100 : 25) * cCur)) / 150));
                if (cAlch < 0) {
                    cAlch = 0;
                }
                cDef = cCur + cAlch;
                cAlch = 5 * cAlch;
            } else {
                cAlch = 0;
                cDef = cRes;
            }
            if (cDef > 0) {
                AddItemToQueue(app, mdIdleDefense, cDef, grobjPlanet, addItemFront);
            }
            if (cAlch > 0) {
                AddItemToQueue(app, mdIdleAlchemy, cAlch, grobjPlanet, addItemFront);
            }
            FinishProduction(app, cDef > 0 || cAlch > 0);
        }
    }
    return;
}

int16_t IdplFindClosestStarbase(APPSESSION &app, FLEET *lpfl, int16_t fBigOnes) {
    POINT16 pt;
    int16_t dy;
    PLANET *lpplTMac;
    PLANET *lpplBest;
    PLANET *lpplT;
    int16_t dx;
    int32_t l;
    int32_t lBest;

    lBest = 10000000;
    lpplBest = NULL;
    pt = lpfl->lpplord->rgord()[0].pt;
    lpplT = app.state.lpPlanets;
    lpplTMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lpplT < lpplTMac; lpplT++) {
        dx = pt.x - app.state.rgptPlan[lpplT->id].x;
        dy = pt.y - app.state.rgptPlan[lpplT->id].y;
        l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        if (l < lBest && lpplT->iPlayer == app.state.idPlayer && lpplT->fStarbase && (!fBigOnes || lpplT->rgwtMin[3] > 250)) {
            lpplBest = lpplT;
            lBest = l;
        }
    }
    if (!lpplBest) {
        return idplNone;
    }
    return lpplBest->id;
}

int16_t FMoveToNearestStarbase(APPSESSION &app, FLEET *lpfl, int16_t fBigOnes) {
    int16_t id;
    ORDER   ord;

    id = IdplFindClosestStarbase(app, lpfl, fBigOnes);
    if (id == idplNone) {
        return 0;
    }
    ord.id = id;
    ord.grobj = grobjPlanet;
    ord.pt = app.state.rgptPlan[id];
    ord.grTask = grTaskNone;
    ord.fValidTask = 1;
    ord.iWarp = 4;
    return FMoveAiFleet(app, lpfl, &ord, 0);
}

void MoveToNearestPlanetOrEnemy(APPSESSION &app, FLEET *lpfl, int16_t dEnemyRange) {
    POINT16 pt;
    int16_t id;
    int16_t dy;
    PLANET *lpplTMac;
    PLANET *lpplBest;
    PLANET *lpplT;
    int16_t dx;
    ORDER   ord;
    int32_t l;
    int32_t lBest;
    SCAN    scan;

    lBest = 10000000;
    lpplBest = NULL;
    pt = lpfl->lpplord->rgord()[0].pt;
    lpplT = app.state.lpPlanets;
    lpplTMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lpplT < lpplTMac; lpplT++) {
        dx = pt.x - app.state.rgptPlan[lpplT->id].x;
        dy = pt.y - app.state.rgptPlan[lpplT->id].y;
        l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        if (l < lBest && lpplT->iPlayer != app.state.idPlayer && lpplT->iPlayer != iplrNone) {
            lpplBest = lpplT;
            lBest = l;
        }
    }
    if (lpplBest && lBest <= (int32_t)(uint32_t)(dEnemyRange * dEnemyRange)) {
        id = lpplBest->id;
    } else {
        if (!FFindNearestObject(app, lpfl->pt, static_cast<GrobjClass>(grobjPlanet | mdNoRecurse), &scan)) {
            return;
        }
        id = scan.idpl;
    }
    if (lpfl->idPlanet != id) {
        ord.id = id;
        ord.grobj = grobjPlanet;
        ord.pt = app.state.rgptPlan[id];
        ord.grTask = grTaskNone;
        ord.fValidTask = 1;
        ord.iWarp = 4;
        FMoveAiFleet(app, lpfl, &ord, 0);
    }
    return;
}

void EnsureAiStarbaseDesigns(APPSESSION &app) {
    uint16_t wTurnLast;
    int16_t  iSetNew;
    int16_t  i;
    int16_t  iSetLast;

    if (app.state.rglpshdefSB[app.state.idPlayer][2].fFree) {
        FCreateAiStarbase(app, 2, 2, static_cast<AiStarbaseRecipe>(aiSbRecipeAuto), static_cast<isbhull>(isbhullAuto));
        FCreateAiStarbase(app, 4, 3, static_cast<AiStarbaseRecipe>(aiSbRecipeAuto), static_cast<isbhull>(isbhullAuto));
    }
    if (app.state.rglpshdefSB[app.state.idPlayer][1].fFree) {
        FCreateAiStarbase(app, 1, 1, static_cast<AiStarbaseRecipe>(aiSbRecipeAuto), static_cast<isbhull>(isbhullAuto));
    }
    if (app.state.rglpshdefSB[app.state.idPlayer][3].fFree) {
        FCreateAiStarbase(app, 3, 2, static_cast<AiStarbaseRecipe>(aiSbRecipeAuto), static_cast<isbhull>(isbhullAuto));
    }
    if (app.state.game.turn >= 50) {
        iSetLast = -1;
        wTurnLast = 0;
        for (i = 0; i <= 9; i += 2) {
            if (i == 6) {
                i--;
            }
            if (!app.state.rglpshdefSB[app.state.idPlayer][i].fFree && app.state.rglpshdefSB[app.state.idPlayer][i].turn >= wTurnLast) {
                wTurnLast = app.state.rglpshdefSB[app.state.idPlayer][i].turn;
                iSetLast = i;
            }
        }
        if (iSetLast != -1 && (uint16_t)(wTurnLast + 40) <= app.state.game.turn) {
            iSetNew = iSetLast <= 4 ? 5 : 0;
            for (i = iSetNew; i < iSetNew + 5; i += 2) {
                if (!app.state.rglpshdefSB[app.state.idPlayer][i].fFree && app.state.rglpshdefSB[app.state.idPlayer][i].cExist > 0)
                    goto LOrbital;
            }
            FCreateAiStarbase(app, iSetNew, 1, static_cast<AiStarbaseRecipe>(aiSbRecipeAuto), static_cast<isbhull>(isbhullAuto));
            FCreateAiStarbase(app, iSetNew + 2, 2, static_cast<AiStarbaseRecipe>(aiSbRecipeAuto), static_cast<isbhull>(isbhullAuto));
            FCreateAiStarbase(app, iSetNew + 4, 3, static_cast<AiStarbaseRecipe>(aiSbRecipeAuto), static_cast<isbhull>(isbhullAuto));
        }
    LOrbital:
        iSetLast = -1;
        wTurnLast = 0;
        for (i = 1; i < 9; i += 2) {
            if (i == 5) {
                i++;
            }
            if (!app.state.rglpshdefSB[app.state.idPlayer][i].fFree && app.state.rglpshdefSB[app.state.idPlayer][i].turn >= wTurnLast) {
                wTurnLast = app.state.rglpshdefSB[app.state.idPlayer][i].turn;
                iSetLast = i;
            }
        }
        if (iSetLast != -1 && (uint16_t)(wTurnLast + 40) <= app.state.game.turn) {
            iSetNew = iSetLast <= 4 ? 6 : 1;
            if ((app.state.rglpshdefSB[app.state.idPlayer][iSetNew].fFree || app.state.rglpshdefSB[app.state.idPlayer][iSetNew].cExist <= 0) &&
                (app.state.rglpshdefSB[app.state.idPlayer][iSetNew + 2].fFree || app.state.rglpshdefSB[app.state.idPlayer][iSetNew + 2].cExist <= 0)) {
                FCreateAiStarbase(app, iSetNew, 1, static_cast<AiStarbaseRecipe>(aiSbRecipeAuto), static_cast<isbhull>(isbhullAuto));
                FCreateAiStarbase(app, iSetNew + 2, 2, static_cast<AiStarbaseRecipe>(aiSbRecipeAuto), static_cast<isbhull>(isbhullAuto));
            }
        }
    }
    return;
}

void EnsureMacintiStarbaseDesigns(APPSESSION &app, uint8_t *rgSB) {
    int16_t k;
    int16_t iOld;
    int16_t cAge;
    int16_t i;
    int16_t j;
    int16_t iNew;

    *rgSB = 0;
    for (i = 1; i <= 3; i++) {
        if (app.state.rglpshdefSB[app.state.idPlayer][i].fFree || app.state.rglpshdefSB[app.state.idPlayer][i].cExist == 0) {
            if (FCreateAiStarbase(app, i, i >= 3 ? 2 : 1, static_cast<AiStarbaseRecipe>(vrgSBMacAisb[i - 1]), static_cast<isbhull>(i))) {
                rgSB[i] = 0;
            } else {
                rgSB[i] = 1;
            }
        } else {
            cAge = app.state.game.turn - app.state.rglpshdefSB[app.state.idPlayer][i].turn;
            if (cAge < 35) {
                if (app.state.game.turn > 25 && i == 1 && app.state.rglpshdefSB[app.state.idPlayer][i].hul.chs != 8) {
                    rgSB[i] = 3;
                } else {
                    rgSB[i] = 0;
                }
            } else if (cAge < 50) {
                rgSB[i] = 2;
            } else {
                rgSB[i] = 3;
            }
        }
    }
    iOld = ishdefNone;
    for (i = 1; i <= 3; i++) {
        if (rgSB[i] >= 2 && (iOld == ishdefNone || (int16_t)rgSB[i] > rgSB[iOld] ||
                             (rgSB[i] == rgSB[iOld] && app.state.rglpshdefSB[app.state.idPlayer][i].turn < app.state.rglpshdefSB[app.state.idPlayer][iOld].turn))) {
            iOld = i;
        }
    }
    for (i = 1; i <= 3; i++) {
        if (rgSB[i] >= 2 && i != iOld) {
            rgSB[i] = 0;
        }
    }
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 3 && (app.state.rglpshdefSB[app.state.idPlayer][3 * i + 4 + j].fFree || app.state.rglpshdefSB[app.state.idPlayer][3 * i + 4 + j].cExist <= 0); j++) {
        }
        if (j == 3) {
            for (j = 0; j < 3; j++) {
                for (k = 5; k >= 3 && FCreateAiStarbase(app, 3 * i + 4 + j, j + 1, static_cast<AiStarbaseRecipe>(vrgSBMacAisb[k]), static_cast<isbhull>(k - 1)) == 0; k--) {
                }
            }
        }
    }
    for (i = 4; i < 10; i++) {
        rgSB[i] = app.state.rglpshdefSB[app.state.idPlayer][i].fFree != 0;
    }
    if (app.state.rglpshdefSB[app.state.idPlayer][4].turn >= app.state.rglpshdefSB[app.state.idPlayer][7].turn) {
        iNew = 4;
        iOld = 7;
    } else {
        iNew = 7;
        iOld = 4;
    }
    if ((uint16_t)(app.state.game.turn - app.state.rglpshdefSB[app.state.idPlayer][iOld].turn) < 30) {
        j = 2;
    } else {
        j = 3;
    }
    for (i = iOld; i < iOld + 3; i++) {
        rgSB[i] = j;
    }
    i = app.state.rglpshdefSB[app.state.idPlayer][iNew].hul.ihuldef - ihuldefOrbitalFort;
    if (i < 4) {
        rgSB[3] = 2;
        if (i < 3) {
            rgSB[2] = 2;
        }
    }
    return;
}

int16_t FCreateAiStarbase(APPSESSION &app, int16_t ishdef, int16_t iLevel, AiStarbaseRecipe aisb, isbhull isb) {
    int16_t i;
    SHDEF   shdef;
    HS     *lphs;

    if (aisb < aiSbRecipeSpaceStation) {
        switch (ishdef) {
        case 1:
        case 3:
        case 6:
        case 8:
            aisb = aiSbRecipeOrbitalFort;
            isb = isbhullOrbitalFort;
            if (iLevel != 2)
                break;
            iLevel++;
            break;
        default:
            aisb = aiSbRecipeSpaceStation;
            isb = isbhullSpaceStation;
        }
    }
    if (!FCreateAiShdef(app, ishdefNone, static_cast<HulDef>(isb), (uint8_t *)&vrgSBAip[aisb])) {
        return 0;
    }
    shdef = app.state.shdefBuild;
    if (iLevel < 3) {
        for (i = 0; i < shdef.hul.chs; i++) {
            lphs = &shdef.hul.rghs[i];
            if (lphs->cItem >= 4) {
                lphs->cItem >>= 3 - iLevel;
            } else if (iLevel <= 1 && app.state.rgplr[app.state.idPlayer].idAi != idAiCybertron && (lphs->grhst == hstSpecialSB || lphs->cItem > 1)) {
                lphs->cItem += 0xff;
            }
        }
    }
    shdef.ishdef = ishdef + 16;
    PickANameAndBmp(app, &shdef, idsGuardianAngel, 13, shdef.hul.ibmp);
    return FChangeAiShdef(app, &shdef, ishdef + 16);
}

int16_t FAIFling(APPSESSION &app, PLANET *lppl, int32_t *rgResAvail) {
    PLANET *lpplHit;
    POINT16 pt;
    int32_t dy;
    int16_t iT;
    int32_t d2;
    int32_t dBigAssPacket;
    int16_t i;
    int16_t fTwoMAs;
    int16_t iLevelBest;
    PLANET *lpplBest;
    int32_t dx;
    int16_t cFound;
    PLANET *lpplHitMac;
    int32_t l;
    PROD   *lpprod;
    int16_t iKeep;

    cFound = 0;
    iLevelBest = -1;
    if (app.state.rgplr[app.state.idPlayer].idAi == idAiCybertron) {
        return 0;
    }
    i = 0;
    lpprod = app.state.lpplProdGlob->rgprod();
    while (i < app.state.lpplProdGlob->iprodMac) {
        if (lpprod->grobj == grobjPlanet && lpprod->iItem >= iobjPacketIron && lpprod->iItem <= iobjPacketMixed) {
            return 0;
        }
        i++;
        lpprod++;
    }
    if (app.state.rgplr[app.state.idPlayer].lvlAi > lvlAiStandard && *rgResAvail + rgResAvail[1] + rgResAvail[2] > 3000 && lppl->fStarbase &&
        ColonyIO().pfnWarp(app, lppl, &fTwoMAs) >= 10 && RandomIO().pfnRandom(app, 4) == 0) {
        pt = app.state.rgptPlan[lppl->id];
        for (iT = 0; iT <= 2 && lppl->rgwtMin[iT] <= 12500; iT++) {
        }
        dBigAssPacket = vrgAiPacketDist[fTwoMAs];
        if (iT <= 2) {
            dBigAssPacket = (uint32_t)(dBigAssPacket * 3);
        }
        lpplHit = app.state.lpPlanets;
        lpplHitMac = app.state.lpPlanets + app.state.cPlanet;
        for (; lpplHit < lpplHitMac; lpplHit++) {
            if (lpplHit->iPlayer != iplrNone && lpplHit->iPlayer != app.state.idPlayer && (uint16_t)(lpplHit->turn + 2) >= app.state.game.turn &&
                ((lpplHit->uDefGuess < 14 || lpplHit->uPopGuess < 750) && RaceQueryIO().pfnStat(&app.state.rgplr[lpplHit->iPlayer], rsMajorAdv) != raMacintosh) &&
                (!lpplHit->fStarbase || RaceQueryIO().pfnStat(&app.state.rgplr[lpplHit->iPlayer], rsMajorAdv) != raMassAccel)) {
                dx = (int16_t)(pt.x - app.state.rgptPlan[lpplHit->id].x);
                dy = (int16_t)(pt.y - app.state.rgptPlan[lpplHit->id].y);
                d2 = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                if (d2 <= dBigAssPacket) {
                    cFound++;
                    if (RandomIO().pfnRandom(app, cFound) == 0) {
                        iLevelBest = 13;
                        lpplBest = lpplHit;
                    }
                }
            }
        }
        if (iLevelBest >= 10) {
            app.state.sel.pl.iWarpFling = iLevelBest - 4;
            app.state.sel.pl.idFling = lpplBest->id + 1;
            PlanetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.pl);
            if (rgResAvail[2] > 20000 && RandomIO().pfnRandom(app, 3) != 0) {
                AddItemToQueue(app, iobjPacketGerm, 80, grobjPlanet, addItemEnd);
            }
            if (*rgResAvail > 3000 && rgResAvail[1] > 4000 && rgResAvail[2] > 3000) {
                AddItemToQueue(app, iobjPacketMixed, 30, grobjPlanet, addItemEnd);
            } else if (*rgResAvail > 1500 && rgResAvail[1] > 2250 && rgResAvail[2] > 1500) {
                AddItemToQueue(app, iobjPacketMixed, 15, grobjPlanet, addItemEnd);
            } else {
                for (iT = 0; iT < 3; iT++) {
                    iKeep = iT == 1 ? 2500 : 1250;
                    if (rgResAvail[iT] > iKeep) {
                        l = (int32_t)((rgResAvail[iT] - iKeep) / 200);
                        if (25 < (l <= 1 ? 1 : l)) {
                            l = 25;
                        } else if (l <= 1) {
                            l = 1;
                        }
                        AddItemToQueue(app, iT + 14, LOWORD(l), grobjPlanet, addItemEnd);
                    }
                }
            }
            return 1;
        }
    }
    return 0;
}

int16_t IshdefAiSBLatestOF(APPSESSION &app) {
    if (!app.state.rglpshdefSB[app.state.idPlayer][6].fFree && app.state.rglpshdefSB[app.state.idPlayer][6].turn > app.state.rglpshdefSB[app.state.idPlayer][1].turn) {
        return 6;
    }
    return 1;
}

int16_t IshdefAiSBLatest(APPSESSION &app) {
    if (!app.state.rglpshdefSB[app.state.idPlayer][5].fFree && app.state.rglpshdefSB[app.state.idPlayer][5].turn > app.state.rglpshdefSB[app.state.idPlayer]->turn) {
        return 5;
    }
    return 0;
}

void QueueAiStarbases(APPSESSION &app, PROD *rgprod, int16_t ishdefSBLatest) {
    PLANET *lpplMac;
    PLANET *lppl;
    int16_t i;
    PROD   *lpprod;

    if (ishdefSBLatest != ishdefNone && app.state.rgplr[app.state.idPlayer].idAi != idAiMacinti &&
        ((!app.state.gd.fTutorial && (app.state.game.lid != 9236297 || app.state.rgplr[app.state.idPlayer].idAi != idAiTurinDrone)) || app.state.game.turn <= 30)) {
        lppl = app.state.lpPlanets;
        lpplMac = app.state.lpPlanets + app.state.cPlanet;
        for (; lppl < lpplMac; lppl++) {
            if (app.state.rgplr[app.state.idPlayer].idAi == idAiCybertron) {
                if (lppl->iPlayer != app.state.idPlayer)
                    continue;
                ishdefSBLatest = iBuildCyberStarbase(app, lppl);
                if (ishdefSBLatest == ishdefNone)
                    continue;
            } else {
                if (lppl->iPlayer != app.state.idPlayer || ((lppl->fStarbase && lppl->isb <= 9) || lppl->rgwtMin[3] < 80 || app.state.vlpbAiPlanet[lppl->id * 16 + 2] != 0))
                    continue;
                for (i = 0; i < AhvFromLp(app, app.state.vlpbAiData).cStarbase() && AhvFromLp(app, app.state.vlpbAiData).rgasb(i).idPlanet() != lppl->id; i++) {
                }
                if (i == AhvFromLp(app, app.state.vlpbAiData).cStarbase())
                    continue;
            }
            ChangeMainObjSel(app, grobjPlanet, lppl->id);
            ProductionIO().pfnInitialize(app, rgprod);
            i = 0;
            for (lpprod = app.state.lpplProdGlob->rgprod(); i < app.state.lpplProdGlob->iprodMac && (lpprod->grobj != grobjFleet || lpprod->iItem < iobjPacketGerm); lpprod++) {
                i++;
            }
            if (i < app.state.lpplProdGlob->iprodMac) {
                FinishProduction(app, 0);
            } else {
                AddItemToQueue(app, ishdefSBLatest + 16, 1, grobjFleet, addItemEnd);
                FinishProduction(app, 1);
            }
        }
    }
    return;
}

int16_t FUpgradeAiStarbase(APPSESSION &app, PLANET *lppl, int16_t ishdefSBLatest) {
    int16_t isbCur;
    int16_t iDesigns;
    int16_t i;
    int16_t pctUpg;
    int16_t isbNew;
    PROD   *lpprod;
    int16_t ishdef;

    if (ishdefSBLatest == ishdefNone) {
        return 0;
    }
    if (lppl->fStarbase) {
        i = 0;
        lpprod = app.state.lpplProdGlob->rgprod();
        while (i < app.state.lpplProdGlob->iprodMac) {
            if (lpprod->grobj == grobjFleet && lpprod->iItem >= iobjPacketGerm) {
                return 0;
            }
            i++;
            lpprod++;
        }
    }
    if (app.state.rgplr[app.state.idPlayer].idAi == idAiMacinti) {
        isbCur = lppl->isb;
        if (app.state.vAiMacRecycleSB[isbCur] == 3) {
        LDoMacUpgrade:
            if (isbCur < 4) {
                for (isbNew = isbCur + 1; isbNew < 9 && app.state.vAiMacRecycleSB[isbNew] != 0; isbNew++) {
                }
            } else {
                isbNew = isbCur + 3;
                if (isbNew >= 10) {
                    isbNew -= 6;
                }
            }
        LDoMacUpgrade2:
            AddItemToQueue(app, isbNew + 16, 1, grobjFleet, addItemEnd);
            return 1;
        }
        if (app.state.vAiMacRecycleSB[isbCur] == 2 && RandomIO().pfnRandom(app, 100) < 10)
            goto LDoMacUpgrade;
        if (isbCur >= 4 && isbCur != 6 && isbCur != 9 && RandomIO().pfnRandom(app, 100) < 8) {
            isbNew = isbCur + 1;
            goto LDoMacUpgrade2;
        }
        if (isbCur < 4) {
            i = CapacityIO().pfnPctPlanetCapacity(app, lppl);
            if (i > 15) {
                i = (i - 15) * 6;
                if (RandomIO().pfnRandom(app, 100) < i)
                    goto LDoMacUpgrade;
            }
        }
        return 0;
    }
    if (app.state.rgplr[app.state.idPlayer].idAi == idAiCybertron && app.state.game.turn < 40) {
        return 0;
    }
    if (lppl->fStarbase) {
        switch (lppl->isb) {
        case 1:
        case 3:
        case 6:
        case 8:
            iDesigns = 2;
            ishdefSBLatest = IshdefAiSBLatestOF(app);
            break;
        default:
            iDesigns = 3;
        }
        if (lppl->isb < (uint16_t)ishdefSBLatest || lppl->isb > (uint16_t)(ishdefSBLatest + (iDesigns - 1) * 2)) {
            pctUpg = app.state.game.turn - app.state.rglpshdefSB[app.state.idPlayer][ishdefSBLatest].turn - 10;
            if (pctUpg < 0) {
                pctUpg = 0;
            } else if (pctUpg < 50) {
                pctUpg >>= 1;
            }
            pctUpg += 5;
            if (RandomIO().pfnRandom(app, 100) < pctUpg) {
                switch (lppl->isb) {
                case 1:
                case 3:
                case 6:
                case 8:
                    ishdef = ishdefSBLatest + lppl->isb % 5 - 1;
                    break;
                default:
                    ishdef = ishdefSBLatest + lppl->isb % 5;
                }
                AddItemToQueue(app, ishdef + 16, 1, grobjFleet, addItemEnd);
                return 1;
            }
        } else if (lppl->isb % 5 < (iDesigns - 1) * 2 && !app.state.rglpshdefSB[app.state.idPlayer][lppl->isb + 2].fFree && RandomIO().pfnRandom(app, 100) < 6) {
            for (i = 0; i < 3; i++) {
                if (lppl->rgwtMin[i] < 200) {
                    return 0;
                }
            }
            AddItemToQueue(app, lppl->isb + 18, 1, grobjFleet, addItemEnd);
            return 1;
        }
    }
    return 0;
}

int16_t FQueueAiTerraforming(APPSESSION &app, PLANET *lppl, int32_t *rgResAvail, int32_t *rgResCost) {
    int16_t i;
    int16_t j;
    int16_t dEnv;
    int32_t rgItemCost[4];
    PROD   *lpprod;

    if (app.state.rgplr[app.state.idPlayer].idAi == idAiCybertron) {
        return 0;
    }
    if (lppl->rgwtMin[3] < 200) {
        return 0;
    }
    i = 0;
    lpprod = app.state.lpplProdGlob->rgprod();
    while (i < app.state.lpplProdGlob->iprodMac) {
        if (lpprod->grobj == grobjPlanet && lpprod->iItem == mdIdleTerraform) {
            return 0;
        }
        i++;
        lpprod++;
    }
    j = -1;
    dEnv = 0;
    for (i = 0; i < 3; i++) {
        if (abs(lppl->rgEnvVar[i] - app.state.rgplr[app.state.idPlayer].rgEnvVar[i]) > dEnv) {
            j = i;
            dEnv = abs(lppl->rgEnvVar[i] - app.state.rgplr[app.state.idPlayer].rgEnvVar[i]);
        }
    }
    if (j != -1) {
        for (i = 0; i < app.state.cProdGlob; i++) {
            if (app.state.pProdGlob[i].grobj == grobjPlanet && app.state.pProdGlob[i].iItem == mdIdleTerraform && app.state.pProdGlob[i].cItem >= 1) {
                AddItemToQueue(app, app.state.pProdGlob[i].iItem, 4 >= (int32_t)app.state.pProdGlob[i].cItem ? app.state.pProdGlob[i].cItem : 4, grobjPlanet, addItemEnd);
                app.state.pProdGlob[i].cItem = 0;
                ProductionIO().pfnQuote(app, lppl, app.state.pProdGlob + i, (uint32_t *)rgItemCost, app.state.idPlayer, 1);
                for (j = 0; j < 4; j++) {
                    rgResCost[j] += (uint32_t)(rgItemCost[j] * (4 >= (int32_t)app.state.pProdGlob[i].cItem ? (int16_t)app.state.pProdGlob[i].cItem : 4));
                }
                return 1;
            }
        }
    }
    return 0;
}

int16_t FQueueAiScanner(APPSESSION &app, PLANET *lppl, int32_t *rgResAvail, int32_t *rgResCost) {
    int16_t i;
    int16_t j;
    int32_t rgItemCost[4];
    PROD   *lpprod;

    i = 0;
    lpprod = app.state.lpplProdGlob->rgprod();
    while (i < app.state.lpplProdGlob->iprodMac) {
        if (lpprod->grobj == grobjPlanet && lpprod->iItem >= iobjPlanetaryScannerFirst && lpprod->iItem <= iobjPlanetaryScannerSnooper620X) {
            return 0;
        }
        i++;
        lpprod++;
    }
    for (i = app.state.cProdGlob - 1; i >= 0 && (app.state.pProdGlob[i].grobj != grobjPlanet || app.state.pProdGlob[i].iItem < iobjPlanetaryScannerFirst ||
                                       app.state.pProdGlob[i].iItem > iobjPlanetaryScannerSnooper620X);
         i--) {
    }
    if (i >= 0) {
        ProductionIO().pfnQuote(app, lppl, app.state.pProdGlob + i, (uint32_t *)rgItemCost, app.state.idPlayer, 1);
        for (j = 0; j < 4 && (rgItemCost[j] <= 0 || rgItemCost[j] + rgResCost[j] <= rgResAvail[j]); j++) {
        }
        if (j == 4) {
            AddItemToQueue(app, app.state.pProdGlob[i].iItem, 1, grobjPlanet, addItemEnd);
            for (j = 0; j < 4; j++) {
                rgResCost[j] += rgItemCost[j];
            }
            return 1;
        }
    }
    return 0;
}

int16_t FQueueAiDefenses(APPSESSION &app, PLANET *lppl, int32_t *rgResAvail, int32_t *rgResCost) {
    int16_t i;
    int16_t j;
    PROD   *lpprod;

    if (lppl->rgwtMin[3] < 1600 || (int32_t)(lppl->rgwtMin[3] / 80) <= (int32_t)lppl->cDefenses) {
        return 0;
    }
    i = 0;
    lpprod = app.state.lpplProdGlob->rgprod();
    while (i < app.state.lpplProdGlob->iprodMac) {
        if (lpprod->grobj == grobjPlanet && lpprod->iItem == mdIdleDefense) {
            return 0;
        }
        i++;
        lpprod++;
    }
    for (i = 0; i < app.state.cProdGlob && (app.state.pProdGlob[i].grobj != grobjPlanet || app.state.pProdGlob[i].iItem != mdIdleDefense || app.state.pProdGlob[i].cItem < 1); i++) {
    }
    if (i < app.state.cProdGlob) {
        j = app.state.pProdGlob[i].cItem;
        if (j > 4) {
            j = 4;
        }
        AddItemToQueue(app, mdIdleDefense, j, grobjPlanet, addItemEnd);
        return 1;
    }
    return 0;
}

void HandleBasicAiTasks(APPSESSION &app, int16_t iroCur, PROD *rgprod, int16_t ishdefSBLatest, int32_t *rgResAvail, int32_t *rgResCost) {
    PLANET *lppl;
    int16_t i;
    int16_t ipl;
    int16_t fWrite;

    KeepFleetsMoving(app);
    QueueAiStarbases(app, rgprod, ishdefSBLatest);
    for (ipl = 0; ipl < app.state.vclpplAi; ipl++) {
        lppl = app.state.vrglpplAi[ipl];
        if (!app.state.vrglpplAi[ipl])
            break;
        if (app.state.rgplr[app.state.idPlayer].idAi == idAiRobotoid) {
            if (lppl->rgwtMin[3] < 40)
                continue;
        } else if (lppl->rgwtMin[3] < 60 && app.state.vlpbAiPlanet[lppl->id * 16 + 2] == 0) {
            continue;
        }
        GetResourcesAvailable(app, lppl, rgResAvail);
        GetProdQCost(app, lppl, rgResCost);
        if (app.state.rgplr[app.state.idPlayer].idAi != idAiMacinti) {
            for (i = 0; i < 4 && rgResAvail[i] >= rgResCost[i]; i++) {
            }
            if (i < 3)
                continue;
        }
        ChangeMainObjSel(app, grobjPlanet, lppl->id);
        ProductionIO().pfnInitialize(app, rgprod);
        fWrite = 0;
        if (app.state.rgplr[app.state.idPlayer].idAi == idAiRobotoid || app.state.vlpbAiPlanet[lppl->id * 16 + 2] == 0) {
            if (FUpgradeAiStarbase(app, lppl, ishdefSBLatest) || FAIFling(app, lppl, rgResAvail) || FQueueAiScanner(app, lppl, rgResAvail, rgResCost) ||
                FQueueAiDefenses(app, lppl, rgResAvail, rgResCost)) {
                fWrite = 1;
            }
        } else if (app.state.rgplr[app.state.idPlayer].idAi == idAiCybertron && FUpgradeAiStarbase(app, lppl, ishdefSBLatest)) {
            fWrite = 1;
        }
        if (app.state.rgplr[app.state.idPlayer].idAi != idAiRobotoid && !fWrite && FQueueAiTerraforming(app, lppl, rgResAvail, rgResCost)) {
            fWrite = 1;
        }
        FinishProduction(app, fWrite);
    }
    if (!app.state.game.fTutorial && app.state.game.turn >= (uint16_t)(10 * app.state.game.mdSize + 20)) {
        FixPlanetsUnderAttack(app, rgprod);
    }
    AddMinesToBlockedQueues(app);
    return;
}

void SplitOutShdefs(APPSESSION &app, uint8_t *rgbIsh) {
    int16_t iLast;
    int16_t iFirst;
    int16_t ifl;
    int16_t i;
    FLEET  *lpfl;
    int16_t fUnmarked;
    FLEET   flNew;
    int16_t fMarked;
    FLEET  *lpflNew;

    iLast = -1;
    iFirst = -1;
    for (i = 0; i < 16; i++) {
        if (rgbIsh[i] > 0) {
            iLast = i;
            if (iFirst == -1) {
                iFirst = i;
            }
        }
    }
    if (iFirst == -1)
        return;
LTopOfLoop:
    if (app.state.rgplr[app.state.idPlayer].cFleet > 500)
        return;
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->iPlayer == app.state.idPlayer && !lpfl->fDead) {
            fUnmarked = 0;
            fMarked = 0;
            for (i = 0; i < 16; i++) {
                if (lpfl->rgcsh[i] > 0) {
                    if (rgbIsh[i] != 0) {
                        if (fUnmarked) {
                        LDoTheSplit:
                            ChangeMainObjSel(app, grobjFleet, lpfl->id);
                            lpflNew = LpflNewSplit(app, &app.state.sel.fl);
                            flNew = *lpflNew;
                            for (i = 0; i < 16; i++) {
                                if (rgbIsh[i] > 0) {
                                    flNew.rgcsh[i] = app.state.sel.fl.rgcsh[i];
                                    app.state.sel.fl.rgcsh[i] = 0;
                                }
                            }
                            FleetTransferCargoBalance(app, &app.state.sel.fl, &flNew);
                            FleetQueryIO().pfnLookup(app, idWriteBack, &app.state.sel.fl);
                            FleetQueryIO().pfnLookup(app, idWriteBack, &flNew);
                            goto LTopOfLoop;
                        }
                        fMarked = 1;
                    } else {
                        if (fMarked)
                            goto LDoTheSplit;
                        fUnmarked = 1;
                    }
                }
            }
        }
    }
}

int16_t CheckAiShdefStatus(APPSESSION &app, int16_t ishBeg, int16_t ishEnd, uint16_t cRecyclePeriod, int16_t *piLatest, uint8_t *rgbOld) {
    uint32_t cExist;
    int16_t  i;
    SHDEF    shdef;

    *piLatest = ishdefNone;
    cExist = 0;
    for (i = ishBeg; i <= ishEnd; i++) {
        if (!app.state.rgshdef[i].fFree) {
            cExist += app.state.rgshdef[i].cExist;
            if (*piLatest == ishdefNone || app.state.rgshdef[*piLatest].turn < app.state.rgshdef[i].turn) {
                *piLatest = i;
            }
            if ((uint16_t)(app.state.game.turn - app.state.rgshdef[i].turn) > cRecyclePeriod) {
                if (app.state.rgshdef[i].cExist == 0) {
                    shdef = app.state.rgshdef[i];
                    shdef.fFree = 1;
                    FChangeAiShdef(app, &shdef, i);
                } else {
                    rgbOld[i] = 1;
                }
            }
        }
    }
    if (cExist > 32000) {
        cExist = 32000;
    }
    return LOWORD(cExist);
}

void IncreaseAIMinefieldSizes(APPSESSION &app) {
    THING  *lpth;
    int32_t cMines;
    THING  *lpthMac;

    lpth = app.state.lpThings;
    lpthMac = app.state.lpThings + app.state.cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith() == ithMinefield) {
            cMines = (int32_t)((double)sqrt((double)lpth->thm().cMines()) + 10.5);
            { const auto cMinesForThing = ((uint32_t)(cMines * cMines)); auto &thForThing = *(lpth); thForThing.thm().SetCMines(cMinesForThing); }
        }
    }
    return;
}

int16_t FFindBuddyAndJoinUp(APPSESSION &app, FLEET *lpfl, int16_t ishLo, int16_t ishHi, int32_t lMaxDist1, int32_t lMaxDist2) {
    int32_t lDistBest;
    int32_t lDist;
    int16_t i;
    int16_t ifl;
    FLEET  *lpflOther;
    FLEET  *lpflBest;
    ORDER   ord;

    lpflBest = NULL;
    lDistBest = 9999999;
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpflOther = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpflOther->iPlayer == app.state.idPlayer) {
            if (lpflOther == lpfl)
                break;
            for (i = ishLo; i <= ishHi; i++) {
                if (lpflOther->rgcsh[i] != 0) {
                    lDist = LDistance2(lpfl->pt, lpflOther->pt);
                    if (lDist < lDistBest) {
                        lpflBest = lpflOther;
                        lDistBest = lDist;
                    }
                    break;
                }
            }
        }
    }
    if (lpflBest && (lDistBest <= (int32_t)(uint32_t)(lMaxDist1 * lMaxDist1) || (lDistBest <= (int32_t)(uint32_t)(lMaxDist2 * lMaxDist2) && RandomIO().pfnRandom(app, 2) != 0))) {
        ClearAiCurrentTask(app, lpfl, 1);
        ord.id = lpflBest->id;
        ord.grobj = grobjFleet;
        ord.pt = lpflBest->pt;
        ord.grTask = grTaskNone;
        ord.fValidTask = 1;
        ord.iWarp = 6;
        FMoveAiFleet(app, lpfl, &ord, 0);
        return 1;
    }
    return 0;
}

int16_t FShouldPlanetBuildColonizer(APPSESSION &app, PLANET *lpplSrc) {
    POINT16  pt;
    int16_t  i;
    int32_t  lCur;
    uint8_t *lpb;
    int32_t  lBest;

    lBest = 10000000;
    pt = app.state.rgptPlan[lpplSrc->id];
    if (app.state.game.turn < 60) {
        return 1;
    }
    lpb = app.state.vlpbAiPlanet + 13;
    i = 0;
    while (i < app.state.game.cPlanMax) {
        if (*lpb == 0) {
            lCur = LDistance2(pt, app.state.rgptPlan[i]);
            if (lCur < lBest) {
                lBest = lCur;
            }
        }
        i++;
        lpb += 16;
    }
    if (lBest > 122500 || (lBest > 90000 && RandomIO().pfnRandom(app, 2) != 0) || (lBest > 62500 && RandomIO().pfnRandom(app, 2) != 0)) {
        return 0;
    }
    return 1;
}

void InitRandomPlanetList(APPSESSION &app) {
    PLANET *lpplMac;
    int16_t iT;
    PLANET *lppl;
    int16_t i;

    app.state.vclpplAi = 0;
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == app.state.idPlayer) {
            app.state.vrglpplAi[app.state.vclpplAi++] = lppl;
        }
    }
    if (!app.state.gd.fTutorial) {
        for (i = 0; i < app.state.vclpplAi - 1; i++) {
            iT = RandomIO().pfnRandom(app, app.state.vclpplAi - i) + i;
            lppl = app.state.vrglpplAi[i];
            app.state.vrglpplAi[i] = app.state.vrglpplAi[iT];
            app.state.vrglpplAi[iT] = lppl;
        }
    }
    return;
}
