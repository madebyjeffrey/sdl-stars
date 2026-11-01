#include "common.h"
#include "appstate.h"
#include "turn_io.h"
#include "production_io.h"
#include "random_io.h"

THING *LpthNew(APPSESSION &app, std::int16_t iplr, ThingType ith) {
    std::int16_t iItem;
    std::int16_t i;
    THING  *lpth;
    THING   thNew;

    if (app.state.cThing >= 4050) {
        return nullptr;
    }
    thNew = {};
    thNew.SetIplr(iplr);
    thNew.SetIth(ith);
    lpth = app.state.lpThings;
    i = 0;
    for (; i < app.state.cThing && thNew.idFull() > lpth->idFull(); lpth++) {
        i++;
    }
    if (i < app.state.cThing && thNew.idFull() == lpth->idFull()) {
        iItem = lpth->id();
        while (i < app.state.cThing) {
            if (iItem >= 511) {
                return nullptr;
            }
            if (lpth->idFull() != thNew.idFull())
                break;
            i++;
            thNew.SetId(thNew.id() + 1);
            lpth++;
            iItem++;
        }
    }
    if (app.state.cThing >= app.state.cThingAlloc) {
        app.state.cThingAlloc += 10;
        if (!app.state.lpThings) {
            app.state.lpThings = LpAlloc<THING>(app, app.state.cThingAlloc * sizeof(THING), htThings);
        } else {
            app.state.lpThings = LpReAlloc<THING>(app, app.state.lpThings, app.state.cThingAlloc * sizeof(THING), htThings);
        }
        lpth = app.state.lpThings + i;
    }
    if (i < app.state.cThing) {
        std::memmove(lpth + 1, lpth, (app.state.cThing - i) * sizeof(THING));
    }
    app.state.cThing++;
    std::memcpy(lpth, &thNew, sizeof(THING));
    return lpth;
}

void FreeLpth(APPSESSION &app, THING *lpth) {
    if (lpth < app.state.lpThings + (app.state.cThing - 1)) {
        std::memmove(lpth, lpth + 1, (app.state.cThing - static_cast<std::int16_t>((reinterpret_cast<std::uint8_t *>(lpth) - reinterpret_cast<std::uint8_t *>(app.state.lpThings))) / 18 - 1) * sizeof(THING));
    }
    app.state.cThing--;
    return;
}

std::int16_t CPlanetsInCircle(APPSESSION &app, POINT16 pt, std::int32_t r2) {
    std::int16_t  xStart;
    POINT16 *ppt;
    std::int16_t  yEnd;
    std::int16_t  dy;
    POINT16 *pptEnd;
    std::int16_t  yStart;
    std::int16_t  i;
    std::int16_t  r;
    std::int16_t  cPl;
    std::int16_t  dx;
    std::int16_t  xEnd;

    r = LOWORD(static_cast<std::int32_t>((static_cast<double>(std::sqrt(static_cast<double>(r2))) + 0.9999)));
    xStart = pt.x - r;
    xEnd = pt.x + r;
    yStart = pt.y - r;
    yEnd = pt.y + r;
    cPl = 0;
    pptEnd = &app.state.rgptPlan[app.state.game.cPlanMax];
    dx = app.state.rgptPlan[app.state.game.cPlanMax - 1].x - app.state.rgptPlan[0].x;
    i = LOWORD(static_cast<std::int32_t>((static_cast<std::int32_t>((static_cast<std::int16_t>((pt.x - app.state.rgptPlan[0].x)) * app.state.game.cPlanMax)) / dx)));
    if (i >= app.state.game.cPlanMax) {
        i = app.state.game.cPlanMax - 1;
    }
    if (i < 0) {
        i = 0;
    }
    for (ppt = &app.state.rgptPlan[i]; ppt->x >= xStart && ppt > app.state.rgptPlan; ppt--) {
    }
    for (; ppt->x <= xEnd && ppt < pptEnd; ppt++) {
        if (ppt->x >= xStart && ppt->y >= yStart && ppt->y <= yEnd) {
            dx = ppt->x - pt.x;
            dy = ppt->y - pt.y;
            if (static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy)) <= r2) {
                cPl++;
            }
        }
    }
    return cPl;
}

std::int16_t IValidateWormholePos(APPSESSION &app, THING *lpthWorm) {
    std::int16_t iRet;
    POINT16 pt;
    std::int32_t dy;
    THING  *lpthMac;
    FLEET  *lpfl;
    std::int16_t ifl;
    std::int16_t i;
    THING  *lpth;
    std::int16_t dUni;
    std::int32_t dx;
    std::int32_t l;

    iRet = 0;
    dUni = 400 * app.state.game.mdSize + 1400;
    pt = lpthWorm->pt();
    if (pt.x < 1000 || pt.y < 1000) {
        return 15;
    }
    if (pt.x > dUni || pt.y > dUni) {
        return 15;
    }
    lpth = app.state.lpThings;
    lpthMac = app.state.lpThings + app.state.cThing;
    for (; lpth < lpthMac; lpth++) {
        if (pt.x == lpth->pt().x() && pt.y == lpth->pt().y() && lpth != lpthWorm) {
            return 15;
        }
    }
    for (i = 0; i < app.state.game.cPlanMax; i++) {
        if (pt.x == app.state.rgptPlan[i].x && pt.y == app.state.rgptPlan[i].y) {
            return 15;
        }
    }
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (pt.x == lpfl->pt.x && pt.y == lpfl->pt.y) {
            return 15;
        }
    }
    if (pt.x < 1010 || pt.y < 1010 || pt.x > dUni - 10 || pt.y > dUni - 10) {
        iRet |= 4;
    }
    lpth = app.state.lpThings;
    lpthMac = app.state.lpThings + app.state.cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith() == ithWormhole && lpth != lpthWorm) {
            dx = static_cast<std::int16_t>((pt.x - lpth->pt().x()));
            dy = static_cast<std::int16_t>((pt.y - lpth->pt().y()));
            l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
            if (lpth->idFull() == lpthWorm->thw().idPartner()) {
                if (l < 4900) {
                    if (l < 25) {
                        iRet |= 8;
                    } else if (l < 100) {
                        iRet |= 4;
                    } else if (l < 900) {
                        iRet |= 2;
                    } else {
                        iRet |= 1;
                    }
                }
            } else if (l < 900) {
                if (l < 16) {
                    iRet |= 8;
                } else if (l < 64) {
                    iRet |= 4;
                } else if (l < 225) {
                    iRet |= 2;
                } else {
                    iRet |= 1;
                }
            }
        }
    }
    for (i = 0; i < app.state.game.cPlanMax; i++) {
        dx = static_cast<std::int16_t>((pt.x - app.state.rgptPlan[i].x));
        dy = static_cast<std::int16_t>((pt.y - app.state.rgptPlan[i].y));
        l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
        if (l < 784) {
            if (l < 25) {
                iRet |= 8;
            } else if (l < 100) {
                iRet |= 4;
            } else if (l < 400) {
                iRet |= 2;
            } else {
                iRet |= 1;
            }
        }
    }
    return iRet;
}

std::int16_t PctWormholeMoves(THING *lpth) {
    std::int16_t pct;

    pct = static_cast<std::int16_t>(lpth->thw().cLastMove()) / 5;
    pct -= 2 - lpth->thw().iStable();
    if (pct < 0) {
        pct = 0;
    } else if (pct > 6) {
        pct = 6;
    }
    return pct;
}

void DoThingInteractions(APPSESSION &app, std::int16_t fPostMove) {
    std::int32_t     wtThreshhold;
    std::uint16_t    grbitPlrTrader;
    std::int16_t     iplr;
    std::int32_t     wtMin;
    POINT16     pt;
    std::int16_t     iplrSav;
    std::uint8_t     rgTech[6];
    std::int32_t     wtNext;
    std::int32_t     dy;
    THING      *lpthMac;
    PLANET     *lpplMac;
    PLANET     *lppl;
    std::int16_t     i;
    std::int16_t     ifl;
    FLEET      *lpfl;
    THING      *lpth;
    MessageId   idm;
    std::int16_t     cPlrTrueMaxTech;
    std::int32_t     dx;
    std::int16_t     fMaxTech;
    std::int32_t     l;
    std::int32_t     cTech;
    std::int16_t     iLowest;
    std::int16_t     cTechCur;
    std::int32_t     lSpent;
    std::int16_t     iGoto;
    GrbitTrader grbitTrader;
    std::int16_t     cTry;
    std::int16_t     iOffset;
    std::int16_t     ish;
    SHDEF       shdef;
    SHDEF      *lpshdefDest;
    FLEET      *lpflNew;
    std::int16_t     cGive;
    std::int16_t     iLvl;
    std::int16_t     iPass;

    if (fPostMove) {
        lpth = app.state.lpThings;
        lpthMac = app.state.lpThings + app.state.cThing;
        for (; lpth < lpthMac; lpth++) {
            if (lpth->ith() == ithMysteryTrader) {
                pt = lpth->pt();
                for (ifl = 0; ifl < app.state.cFleet; ifl++) {
                    lpfl = app.state.rglpfl[ifl];
                    if (!app.state.rglpfl[ifl])
                        break;
                    if (!lpfl->fDead && lpfl->pt.x == pt.x && lpfl->pt.y == pt.y) {
                        wtMin = 0;
                        for (i = 0; i <= 2; i++) {
                            wtMin += lpfl->rgwtMin[i];
                        }
                        if (wtMin < 5000) {
                            if (!lpfl->fHereAllTurn) {
                                TurnIO().pfnMessage2(app, lpfl->iplr(), idmMysteryTraderHasRefusedGiveCaptainAudience, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                            }
                        } else {
                            cPlrTrueMaxTech = !app.state.rgplr[lpfl->iPlayer].fCrippled ? 26 : 10;
                            for (i = 0; i < 6 && app.state.rgplr[lpfl->iPlayer].rgTech[i] >= cPlrTrueMaxTech; i++) {
                            }
                            fMaxTech = i == 6;
                            grbitPlrTrader = app.state.rgplr[lpfl->iPlayer].grbitTrader;
                            iplr = lpfl->iPlayer;
                            if (1 << iplr & lpth->tht().grbitPlr()) {
                                TurnIO().pfnMessage2(app, lpfl->iplr(), idmMysteryTraderEyesCaptainSuspiciouslySuggestsHe, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, 0);
                            } else {
                                lpth->tht().SetGrbitPlr(lpth->tht().grbitPlr() | (1 << iplr));
                                TurnIO().pfnRemoveMessage(app, iplr, idmHasCompletedAssignedOrders, static_cast<MsgGoto>(lpfl->id | 0x8000));
                                lpfl->fDead = 1;
                                if (lpth->tht().grbitTrader() == grbitTraderNone || (lpth->tht().grbitTrader() & grbitPlrTrader)) {
                                    if (fMaxTech) {
                                        if (RandomIO().pfnRandom(app, 5) != 0)
                                            goto LGivePart;
                                        idm = idmHasAbsorbedMysteryTraderHoweverTraderUnable2;
                                        TurnIO().pfnMessage2(app, lpfl->iplr(), idm, gotoNone, WFromLpfl(app, lpfl), 0);
                                        continue;
                                    } else {
                                        cTechCur = 0;
                                        cTech = static_cast<std::int32_t>(((wtMin - 5000) / 1200)) + 6;
                                        for (i = 0; i < 6; i++) {
                                            cTechCur += app.state.rgplr[lpfl->iPlayer].rgTech[i];
                                        }
                                        if (cTech > 10) {
                                            cTech = 10;
                                        }
                                        if (cTechCur >= 108) {
                                            cTech = 1;
                                        } else if (cTechCur >= 96) {
                                            cTech = 2;
                                        } else if (cTechCur >= 84) {
                                            cTech -= 3;
                                        } else if (cTechCur >= 72) {
                                            cTech -= 2;
                                        } else if (cTechCur >= 60) {
                                            cTech--;
                                        }
                                        if ((grbitPlrTrader & 0x1fff) != 0x1fff) {
                                            idm = idmHasAbsorbedMysteryTraderTraderHasGiven;
                                        } else {
                                            idm = idmHasAbsorbedMysteryTraderReturnTraderHas;
                                        }
                                        TurnIO().pfnMessage(app, lpfl->iplr(), idm, gotoNone, WFromLpfl(app, lpfl), LOWORD(cTech), 0, 0, 0, 0, 0);
                                        while (cTech-- != 0) {
                                            if (RandomIO().pfnRandom(app, 4) < 3) {
                                                iLowest = RandomIO().pfnRandom(app, 6);
                                                if (app.state.rgplr[iplr].rgTech[iLowest] < cPlrTrueMaxTech)
                                                    goto LGiveITech;
                                            }
                                            iLowest = 0;
                                            for (i = 1; i < 6; i++) {
                                                if (app.state.rgplr[iplr].rgTech[i] < app.state.rgplr[iplr].rgTech[iLowest]) {
                                                    iLowest = i;
                                                }
                                            }
                                            if (app.state.rgplr[iplr].rgTech[iLowest] >= cPlrTrueMaxTech)
                                                break;
                                        LGiveITech:
                                            std::memcpy(rgTech, app.state.rgplr[iplr].rgTech, 6);
                                            rgTech[iLowest]++;
                                            iplrSav = app.state.idPlayer;
                                            app.state.idPlayer = iplr;
                                            wtNext = CostOfDevelopingItem(app, reinterpret_cast<char *>(rgTech));
                                            app.state.idPlayer = iplrSav;
                                            lSpent = app.state.rgplr[iplr].rgResSpent[iLowest];
                                            lSpent = static_cast<std::int32_t>((lSpent * 2));
                                            if (wtNext > 0) {
                                                lSpent += wtNext;
                                            }
                                            app.state.rgplr[iplr].rgResSpent[iLowest] = lSpent;
                                            TurnIO().pfnResearch(app, 0);
                                        }
                                        continue;
                                    }
                                }
                            LGivePart:
                                cTry = 25;
                                grbitTrader = lpth->tht().grbitTrader();
                                if (grbitTrader == grbitTraderNone) {
                                    grbitTrader = static_cast<GrbitTrader>(1 << RandomIO().pfnRandom(app, 13));
                                }
                                for (; (grbitTrader & grbitPlrTrader) != 0 && cTry-- > 0; grbitTrader = static_cast<GrbitTrader>(1 << RandomIO().pfnRandom(app, 13))) {
                                }
                                if (cTry <= 0) {
                                    grbitTrader = grbitTraderLifeboat;
                                }
                                lpth->tht().SetGrbitPlr(lpth->tht().grbitPlr() | (1 << iplr));
                                if (grbitTrader == grbitTraderLifeboat) {
                                    if (!app.state.rgplr[iplr].fAi) {
                                        iOffset = RandomIO().pfnRandom(app, 4 - (app.state.game.turn > 100));
                                        if (iOffset >= 1) {
                                            iOffset = RandomIO().pfnRandom(app, 2) + 1;
                                        }
                                        std::memmove(&shdef, LpshdefT() + (iOffset + 19), sizeof(SHDEF));
                                        ish = IshFindSimilarDesign(app, &shdef.hul, iplr);
                                        if (ish < 0) {
                                            do {
                                                ish++;
                                            } while (ish < 16 && !app.state.rglpshdef[iplr][ish].fFree);
                                        }
                                        if (ish >= 16 || app.state.rgplr[lpfl->iplr()].cFleet >= 0x200) {
                                        LNoLifeboat:
                                            TurnIO().pfnMessage2(app, lpfl->iplr(), idmHasAbsorbedMysteryTraderReturnTraderTried, gotoNone, WFromLpfl(app, lpfl), 0);
                                            continue;
                                        }
                                        if (RandomIO().pfnRandom(app, 3) == 0) {
                                            cGive = 2;
                                        } else {
                                            cGive = 1;
                                        }
                                        if (app.state.game.turn > 100 && !app.state.game.fSinglePlr) {
                                            cGive += RandomIO().pfnRandom(app, static_cast<std::uint32_t>(app.state.game.turn) / 100 + 1);
                                        }
                                        if (cGive > 5) {
                                            cGive = 5;
                                        }
                                        if (iOffset > 0) {
                                            cGive += RandomIO().pfnRandom(app, cGive + 1);
                                        }
                                        lpflNew = ProductionIO().pfnNewFleet(app, iplr, lpfl->idPlanet);
                                        if (!lpflNew)
                                            goto LNoLifeboat;
                                        TurnIO().pfnMessage2(app, lpfl->iplr(), idmHasAbsorbedMysteryTraderReturnHaveGiven3, static_cast<MsgGoto>(lpflNew->id | 0x8000), WFromLpfl(app, lpfl), cGive);
                                        if (lpflNew->id < lpfl->id) {
                                            ifl++;
                                        }
                                        lpflNew->pt = lpfl->pt;
                                        lpflNew->lpplord->rgord()[0].pt = lpfl->pt;
                                        lpflNew->fHereAllTurn = 1;
                                        lpshdefDest = app.state.rglpshdef[iplr] + ish;
                                        if (lpshdefDest->fFree) {
                                            *lpshdefDest = shdef;
                                            lpshdefDest->ishdef = ish;
                                            lpshdefDest->fGift = 1;
                                            lpshdefDest->cBuilt = 0;
                                            lpshdefDest->cExist = 0;
                                            app.state.rgplr[iplr].cShDef++;
                                            app.state.idPlayer = lpfl->iplr();
                                            UpdateShdefCost(app, lpshdefDest);
                                            app.state.idPlayer = iplrNone;
                                        }
                                        lpshdefDest->cBuilt += cGive;
                                        lpshdefDest->cExist += cGive;
                                        lpflNew->rgcsh[ish] = cGive;
                                        lpflNew->rgwtMin[4] = ProductionIO().pfnFleetStat(app, lpflNew, 1);
                                        continue;
                                    }
                                } else {
                                    idm = static_cast<MessageId>(IdmGiveTraderPart(app, grbitTrader, iplr, reinterpret_cast<std::uint16_t *>(&iGoto)));
                                    TurnIO().pfnMessage2(app, lpfl->iplr(), idm, static_cast<MsgGoto>(iGoto), WFromLpfl(app, lpfl), 0);
                                }
                            }
                        }
                    }
                }
                lppl = app.state.lpPlanets;
                lpplMac = app.state.lpPlanets + app.state.cPlanet;
                for (; lppl < lpplMac; lppl++) {
                    if (lppl->iPlayer != iplrNone && lppl->fStarbase && app.state.rgplr[lppl->iPlayer].fAi && app.state.rgplr[lppl->iPlayer].lvlAi >= lvlAiTough &&
                        !(1 << lppl->iPlayer & lpth->tht().grbitPlr())) {
                        dx = static_cast<std::int16_t>((app.state.rgptPlan[lppl->id].x - pt.x));
                        if (dx > 100)
                            break;
                        dy = static_cast<std::int16_t>((app.state.rgptPlan[lppl->id].y - pt.y));
                        l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                        if (l <= 10000) {
                            wtNext = 0;
                            for (i = 0; i < 3; i++) {
                                wtNext += lppl->rgwtMin[i];
                            }
                            iplr = lppl->iPlayer;
                            wtThreshhold = static_cast<std::uint32_t>((app.state.rgplr[iplr].lvlAi == lvlAiTough ? 3500 : 5000));
                            if (wtNext >= wtThreshhold) {
                                if (lpth->tht().grbitTrader() == grbitTraderNone)
                                    goto LAutoTech;
                                cTry = 50;
                                for (grbitTrader = lpth->tht().grbitTrader(); (grbitTrader & app.state.rgplr[iplr].grbitTrader) != 0 && cTry-- > 0;
                                     grbitTrader = static_cast<GrbitTrader>(1 << RandomIO().pfnRandom(app, 13))) {
                                }
                                if (cTry <= 0)
                                    goto LAutoTech;
                                app.state.rgplr[iplr].grbitTrader = static_cast<GrbitTrader>(app.state.rgplr[iplr].grbitTrader | grbitTrader);
                            LChgMin:
                                lpth->tht().SetGrbitPlr(lpth->tht().grbitPlr() | (1 << iplr));
                                for (i = 2; wtNext > 0 && i >= 0; i--) {
                                    wtMin = wtNext < lppl->rgwtMin[i] ? wtNext : lppl->rgwtMin[i];
                                    lppl->rgwtMin[i] -= wtMin;
                                    wtNext -= wtMin;
                                }
                                continue;
                            LAutoTech:
                                iLvl = 0;
                                for (i = 0; i < 6; i++) {
                                    iLvl += app.state.rgplr[iplr].rgTech[i];
                                }
                                cPlrTrueMaxTech = !app.state.rgplr[lpfl->iPlayer].fCrippled ? 26 : 10;
                                if (iLvl >= 6 * cPlrTrueMaxTech - 6)
                                    continue;
                                for (iPass = 0; iPass < 6; iPass++) {
                                    iLvl = 0;
                                    for (i = 1; i < 6; i++) {
                                        if (app.state.rgplr[iplr].rgTech[i] < app.state.rgplr[iplr].rgTech[iLvl]) {
                                            iLvl = i;
                                        }
                                    }
                                    app.state.rgplr[iplr].rgTech[iLvl]++;
                                }
                                wtNext = wtThreshhold;
                                goto LChgMin;
                            }
                        }
                    }
                }
            }
        }
    }
    return;
}

std::int16_t IdmGiveTraderPart(APPSESSION &app, GrbitTrader grbitTrader, std::int16_t iplr, std::uint16_t *piGoto) {
    std::uint16_t  iGoto;
    MessageId idm;

    app.state.rgplr[iplr].grbitTrader = static_cast<GrbitTrader>(app.state.rgplr[iplr].grbitTrader | grbitTrader);
    idm = idmHasAbsorbedMysteryTraderHaveGivenPlans;
    switch (grbitTrader) {
    case grbitTraderCargo:
    default:
        iGoto = 0xcc04;
        break;
    case grbitTraderSpecial:
        iGoto = 0xcb04;
        break;
    case grbitTraderShield:
        iGoto = 0xc206;
        break;
    case grbitTraderArmor:
        iGoto = 0xc309;
        break;
    case grbitTraderMiner:
        iGoto = 0xc706;
        break;
    case grbitTraderBomb:
        iGoto = 0xc608;
        break;
    case grbitTraderBeam:
        iGoto = 0xc412;
        break;
    case grbitTraderTorp:
        iGoto = 0xc507;
        break;
    case grbitTraderHull:
        idm = idmHasAbsorbedMysteryTraderReturnHaveGiven;
        iGoto = 0xce1e;
        break;
    case grbitTraderEngine:
        iGoto = 0xc008;
        break;
    case grbitTraderGenesis:
        idm = idmHasAbsorbedMysteryTraderReturnHaveGiven2;
        iGoto = 0xcf0e;
        break;
    case grbitTraderJumpgate:
        iGoto = 0xcc09;
    }
    *piGoto = iGoto;
    return idm;
}
