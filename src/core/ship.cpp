#include "common.h"
#include "appstate.h"
#include "fleet_query_io.h"
#include "presentation_io.h"
#include "production_io.h"
#include "planet_query_io.h"
#include "part_query_io.h"
#include "race_query_io.h"

std::int16_t FCanSplit(APPSESSION &app, std::int32_t cBoat) {
    if (app.state.rgplr[app.state.idPlayer].cFleet == 0x200) {
        return 0;
    }
    if (cBoat > 1) {
        return 1;
    }
    return 0;
}

std::int16_t FCanSplitAll(APPSESSION &app, std::int32_t cBoat) {
    if (cBoat - 1 + app.state.rgplr[app.state.idPlayer].cFleet > 0x200) {
        return 0;
    }
    if (cBoat > 1) {
        return 1;
    }
    return 0;
}

std::int16_t FCanMerge(APPSESSION &app, FLEET *pfl) {
    std::int16_t i;
    FLEET  *lpfl;
    std::int32_t csh;
    std::int16_t cfl;
    std::int16_t ishdef;

    cfl = 0;
    csh = 0;
    for (i = 0; i < app.state.cFleet; i++) {
        lpfl = app.state.rglpfl[i];
        if (!app.state.rglpfl[i])
            break;
        if (lpfl->iPlayer == pfl->iPlayer && lpfl->pt.x == app.state.sel.fl.pt.x && lpfl->pt.y == app.state.sel.fl.pt.y) {
            cfl++;
            for (ishdef = 0; ishdef < 16; ishdef++) {
                csh += lpfl->rgcsh[ishdef];
            }
        }
    }
    if (cfl == 1 || csh > static_cast<std::int32_t>(static_cast<std::uint32_t>((32766 - (app.state.rgplr[pfl->iPlayer].cFleet - 1))))) {
        return 0;
    }
    return 1;
}

void SelectAdjFleet(APPSESSION &app, std::int16_t dInc, std::int16_t idFleet) {
    POINT16 pt;
    std::int16_t idOld;
    std::int16_t i;
    FLEET  *lpfl;
    std::int16_t idNew;
    FLEET  *lpflT;
    SCAN    scan;

    idOld = idflNone;
    if (app.state.cFleet > 0) {
        if (dInc != 0) {
            idFleet = app.state.sel.fl.id;
        }
        if (!app.state.vrptFleet.fCached) {
            PresentationIO().pfnInvalidateReport(app, rptFleets, 1);
        }
        for (i = 0; i < app.state.cFleet; i++) {
            lpfl = app.state.rglpfl[i];
            if (!app.state.rglpfl[i] || lpfl->id == idFleet)
                break;
        }
        if (i == app.state.cFleet || lpfl->iPlayer != app.state.idPlayer) {
            if (i == app.state.cFleet) {
                return;
            }
            pt = lpfl->pt;
            scan.pt = lpfl->pt;
            scan.grobj = static_cast<GrobjClass>(grobjPlanet | grobjFleet | mdExact);
            ChangeScanSel(app, &scan, 0);
            goto FinishUp;
        } else {
            if (dInc != 0) {
                for (i = 0; i < static_cast<std::int16_t>(app.state.rgplr[app.state.idPlayer].cFleet) && app.state.rglpfl[app.state.vlprgidFleet[i]]->id != idFleet; i++) {
                }
                i += dInc;
                if (i >= static_cast<std::int16_t>(app.state.rgplr[app.state.idPlayer].cFleet)) {
                    i = 0;
                } else if (i < 0) {
                    i = app.state.rgplr[app.state.idPlayer].cFleet - 1;
                }
                i = app.state.vlprgidFleet[i];
            } else if (app.state.sel.grobj == grobjFleet && app.state.sel.fl.pt.x == lpfl->pt.x && app.state.sel.fl.pt.y == lpfl->pt.y) {
                idOld = app.state.sel.fl.id;
            }
            lpflT = app.state.rglpfl[i];
            idNew = lpflT->id;
            pt = lpflT->pt;
            scan.pt = lpflT->pt;
            scan.grobj = static_cast<GrobjClass>(grobjFleet | mdExact);
            ChangeScanSel(app, &scan, 0);
            ShowScanSel(app, 0);
            ChangeMainObjSel(app, grobjFleet, idNew);
            ShowScanSel(app, 1);
        }
    FinishUp:
        ShowSelAt(app, pt);
        if (idOld != idflNone) {
            SetFleetDropDownSel(app, idOld);
        }
    }
    return;
}

std::int32_t LGetFleetStat(APPSESSION &app, FLEET *lpfl, std::int16_t grStat) {
    std::int16_t i;
    std::int32_t l;

    l = 0;
    if (lpfl->det != detAll) {
        return 32000;
    }
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] != 0) {
            l += static_cast<std::uint32_t>((lpfl->rgcsh[i] * WtMaxShdefStat(app.state.rglpshdef[lpfl->iPlayer] + i, grStat)));
        }
    }
    return l;
}

std::int16_t WtMaxShdefStat(SHDEF *lpshdef, std::int16_t grStat) {
    std::int16_t wt;
    std::int16_t j;
    HUL    *lphul;

    lphul = &lpshdef->hul;
    if (grStat != 1) {
        if (grStat != 2) {
            return 0;
        }
        wt = PartQueryIO().pfnHull(lphul->ihuldef)->hul.wtCargoMax;
        for (j = 0; j < lphul->chs; j++) {
            if (lphul->rghs[j].grhst == hstSpecialM) {
                switch (lphul->rghs[j].iItem) {
                default:
                    break;
                case ispecialMCargoPod:
                    wt += lphul->rghs[j].cItem * 50;
                    break;
                case ispecialMSuperCargoPod:
                    wt += lphul->rghs[j].cItem * 100;
                    break;
                case ispecialMMultiCargoPod:
                    wt += lphul->rghs[j].cItem * 250;
                }
            }
        }
    } else {
        wt = PartQueryIO().pfnHull(lphul->ihuldef)->hul.wtFuelMax;
        for (j = 0; j < lphul->chs; j++) {
            if (lphul->rghs[j].grhst != hstSpecialM) {
                if (lphul->rghs[j].grhst == hstSpecialE && lphul->rghs[j].iItem == ispecialEAntiMatterGenerator) {
                    wt += lphul->rghs[j].cItem * 200;
                }
            } else if (lphul->rghs[j].iItem == ispecialMFuelTank) {
                wt += lphul->rghs[j].cItem * 250;
            } else if (lphul->rghs[j].iItem == ispecialMSuperFuelTank) {
                wt += lphul->rghs[j].cItem * 500;
            }
        }
    }
    return wt;
}

std::int16_t FEnumCalcJettison(APPSESSION &app, void *lprt, RecordType rt, std::int16_t cb, PLANET *lppl, std::int16_t iFleet) {
    POINT16  pt;
    std::int16_t  i;
    std::int16_t  grbit;
    FLEET    fl;
    std::int16_t  j;
    std::uint8_t *prtxferx;
    std::uint8_t  *prtxfer;

    if (rt == rtLogCargoXfer8 || rt == rtLogCargoXfer16) {
        prtxfer = static_cast<std::uint8_t *>(lprt);
        if (CxvFromPrefix(prtxfer, 5).grobj1() != grobjFleet || CxvFromPrefix(prtxfer, 5).grobj2() != grobjOther) {
            return 1;
        }
        if (!FleetQueryIO().pfnLookup(app, iFleet, &fl)) {
            return 1;
        }
        pt = fl.pt;
        if (!FleetQueryIO().pfnLookup(app, CxvFromPrefix(prtxfer, 2).id1(), &fl)) {
            return 1;
        }
        if (fl.pt.x != pt.x || fl.pt.y != pt.y) {
            return 1;
        }
        grbit = CxvFromPrefix(prtxfer, 6).grbitItems();
        if (rt == rtLogCargoXfer16) {
            prtxferx = static_cast<std::uint8_t *>(lprt);
        }
        j = 0;
        i = 0;
        while (i < 5) {
            if (grbit & 1) {
                if (rt == rtLogCargoXfer8) {
                    lppl->rgwtMin[i] -= static_cast<std::int16_t>(CxvFromPrefix(prtxfer, 6 + (j + 1)).cQuan8(j));
                } else {
                    lppl->rgwtMin[i] -= CxvFromPrefix(prtxferx, 6 + (j + 1) * 2).cQuan16(j);
                }
                j++;
            }
            i++;
            grbit >>= 1;
        }
    }
    return 1;
}

std::int32_t GetCargoFree(APPSESSION &app, FLEET *lpfl) {
    std::int32_t cHave;
    std::int16_t i;

    cHave = 0;
    for (i = 0; i <= 3; i++) {
        cHave += lpfl->rgwtMin[i];
    }
    return ProductionIO().pfnFleetStat(app, lpfl, 2) - cHave;
}

std::int32_t GetFuelFree(APPSESSION &app, FLEET *lpfl) { return ProductionIO().pfnFleetStat(app, lpfl, 1) - lpfl->rgwtMin[4]; }

std::int32_t ChgCargo(APPSESSION &app, GrobjClass grobj, std::int16_t id, MineralType iSupply, std::int32_t dChg, void *pobj) {
    THING  *pth;
    XFER    xfer;
    std::int16_t i;
    FLEET  *pfl;
    PLANET *ppl;
    std::int32_t wtFree;

    switch (grobj) {
    case grobjPlanet:
    case grobjOther:
        if (pobj) {
            ppl = static_cast<PLANET *>(pobj);
        } else if (grobj == grobjPlanet) {
            PlanetQueryIO().pfnLookup(app, id, &xfer.pl());
            ppl = &xfer.pl();
        } else {
            xfer.pl() = {};
            ppl = &xfer.pl();
        }
        if (iSupply <= Fuel) {
            if (iSupply == Fuel) {
                return 0;
            }
            if (dChg == 0) {
                return ppl->rgwtMin[iSupply];
            }
            if (ppl->rgwtMin[iSupply] + dChg < 0) {
                dChg = -ppl->rgwtMin[iSupply];
            }
            ppl->rgwtMin[iSupply] += dChg;
        }
        if (dChg == 0 || pobj || grobj == grobjOther)
            break;
        PlanetQueryIO().pfnLookup(app, idWriteBack, &xfer.pl());
        break;
    case grobjThing:
        if (pobj) {
            pth = static_cast<THING *>(pobj);
        } else {
            FLookupThing(app, id, &xfer.th());
            pth = &xfer.th();
        }
        if (iSupply >= Colonists) {
            return 0;
        }
        if (iSupply <= Fuel) {
            if (dChg == 0) {
                return pth->thp().rgwtMin(iSupply);
            }
            if (pth->thp().rgwtMin(iSupply) + dChg < 0) {
                dChg = static_cast<std::int16_t>(-pth->thp().rgwtMin(iSupply));
            }
            wtFree = static_cast<std::uint32_t>((pth->thp().wtMax() * 10));
            for (i = 0; i < 3; i++) {
                wtFree -= pth->thp().rgwtMin(i);
            }
            if (dChg > wtFree) {
                dChg = wtFree;
            }
            { const auto rgwtMinForThing = +(LOWORD(dChg)); auto &thForThing = *(pth); thForThing.thp().SetRgwtMin(iSupply, thForThing.thp().rgwtMin(iSupply) + rgwtMinForThing); }
        }
        if (dChg == 0 || pobj)
            break;
        FLookupThing(app, idWriteBack, pth);
        break;
    default:
        if (pobj) {
            pfl = static_cast<FLEET *>(pobj);
        } else {
            FleetQueryIO().pfnLookup(app, id, &xfer.fl());
            pfl = &xfer.fl();
        }
        if (iSupply <= Fuel) {
            if (dChg == 0) {
                return pfl->rgwtMin[iSupply];
            }
            if (pfl->rgwtMin[iSupply] + dChg < 0) {
                dChg = -pfl->rgwtMin[iSupply];
            }
            if (iSupply == Colonists && pfl->det != detAll) {
                dChg = 0;
            }
            if (dChg >= (iSupply == Fuel ? GetFuelFree(app, pfl) : GetCargoFree(app, pfl))) {
                if (iSupply == Fuel) {
                    dChg = GetFuelFree(app, pfl);
                } else {
                    dChg = GetCargoFree(app, pfl);
                }
            }
            pfl->rgwtMin[iSupply] += dChg;
        }
        if (dChg != 0 && !pobj) {
            FleetQueryIO().pfnLookup(app, idWriteBack, pfl);
        }
    }
    return dChg;
}

std::int32_t XferSupply(APPSESSION &app, MineralType iSupply, std::int32_t cQuan) {
    std::int16_t iSrc;
    std::int32_t dChg;
    std::int32_t cAvailable;

    if (cQuan == 0) {
        return 0;
    }
    iSrc = cQuan > 0;
    if (iSrc == 0) {
        cQuan = -cQuan;
    }
    cAvailable = ChgCargo(app, app.state.pxfer[iSrc].grobj, app.state.pxfer[iSrc].id, iSupply, 0, app.state.pxfer[iSrc].LpvCargo(app.state.pxfer[iSrc].grobj));
    if (cQuan > cAvailable) {
        cQuan = cAvailable;
    }
    if (cQuan == 0) {
        return 0;
    }
    dChg = ChgCargo(app, app.state.pxfer[iSrc == 0].grobj, app.state.pxfer[iSrc == 0].id, iSupply, cQuan, app.state.pxfer[(iSrc == 0)].LpvCargo(app.state.pxfer[(iSrc == 0)].grobj));
    if (dChg != 0) {
        ChgCargo(app, app.state.pxfer[iSrc].grobj, app.state.pxfer[iSrc].id, iSupply, -dChg, app.state.pxfer[iSrc].LpvCargo(app.state.pxfer[iSrc].grobj));
    }
    return dChg;
}

void DeleteWpFar(FLEET *lpfl, std::int16_t iDel, std::int16_t fRecycle) {
    ORDER ord;

    if (fRecycle) {
        if (iDel == 86 || lpfl->cord == 2 ||
            (lpfl->lpplord->rgord()[lpfl->cord - 1].pt.x == lpfl->lpplord->rgord()[iDel].pt.x &&
             lpfl->lpplord->rgord()[lpfl->cord - 1].pt.y == lpfl->lpplord->rgord()[iDel].pt.y)) {
            fRecycle = 0;
        } else {
            ord = lpfl->lpplord->rgord()[iDel];
        }
    }
    std::memmove(&lpfl->lpplord->rgord()[iDel], &lpfl->lpplord->rgord()[iDel + 1], (lpfl->cord - iDel - 1) * sizeof(ORDER));
    if (fRecycle) {
        lpfl->lpplord->rgord()[lpfl->cord - 1] = ord;
    } else {
        lpfl->cord--;
        lpfl->lpplord->iordMac--;
    }
    return;
}

std::int32_t EstFuelUse(APPSESSION &app, FLEET *lpfl, std::int16_t iOrd, std::int16_t iWarp, std::int32_t dTravel, std::int16_t fRangeOnly) {
    std::int32_t iEffNext;
    std::int32_t lT;
    std::int16_t fEfficient;
    double  d;
    std::int32_t iEffCur;
    std::int32_t wtCargoT;
    std::int32_t lFuel;
    ORDER  *lpord;
    std::int16_t i;
    SHDEF  *lpshdef;
    std::int32_t wtCargo;
    std::int16_t j;
    std::int32_t wtMass;
    std::int32_t rgieff[16];

    iEffCur = 0;
    app.state.gd.fRadiatingEngine = 0;
    if (iWarp == -1) {
        iWarp = lpfl->lpplord->rgord()[iOrd + 1].iWarp;
    }
    fEfficient = RaceQueryIO().pfnGrbit(&app.state.rgplr[lpfl->iPlayer], ibitRaceIFE);
    for (i = 0, lpshdef = app.state.rglpshdef[lpfl->iPlayer]; i < 16; i++, lpshdef++) {
        if (lpfl->rgcsh[i] != 0) {
            for (j = 0; j < lpshdef->hul.chs && lpshdef->hul.rghs[j].grhst != hstEngine; j++) {
            }
            if (j == lpshdef->hul.chs || lpshdef->hul.rghs[j].cItem < PartQueryIO().pfnHull(lpshdef->hul.ihuldef)->hul.rghs[j].cItem) {
                rgieff[i] = 99999;
                continue;
            }
            rgieff[i] = LpengineFromId(lpshdef->hul.rghs[j].iItem)->rgcFuelUsed[iWarp];
            if (fEfficient) {
                rgieff[i] -= static_cast<std::int32_t>((rgieff[i] * 15)) / 100;
            }
            if (lpshdef->hul.rghs[j].iItem == iengineRadiatingHydroRamScoop) {
                app.state.gd.fRadiatingEngine = 1;
            }
        }
    }
    wtCargo = 0;
    for (i = 0; i <= 3; i++) {
        wtCargo += lpfl->rgwtMin[i];
    }
    if (dTravel == -1) {
        if (fRangeOnly) {
            dTravel = 1000;
        } else {
            lpord = &lpfl->lpplord->rgord()[iOrd];
            d = DGetDistance(lpord->pt.x, lpord->pt.y, lpord[1].pt.x, lpord[1].pt.y);
            dTravel = static_cast<std::int16_t>(LOWORD(static_cast<std::int32_t>((static_cast<double>(d) + 0.9999))));
        }
    }
    lFuel = 0;
    while (1) {
        iEffNext = 999999;
        for (i = 0; i < 16; i++) {
            if (lpfl->rgcsh[i] > 0) {
                if (rgieff[i] == iEffCur) {
                    if (wtCargo < static_cast<std::int32_t>(static_cast<std::uint32_t>((lpfl->rgcsh[i] * WtMaxShdefStat(app.state.rglpshdef[lpfl->iPlayer] + i, 2))))) {
                        wtCargoT = wtCargo;
                    } else {
                        wtCargoT = static_cast<std::uint32_t>((lpfl->rgcsh[i] * static_cast<std::int16_t>(WtMaxShdefStat(app.state.rglpshdef[lpfl->iPlayer] + i, 2))));
                    }
                    wtCargo -= wtCargoT;
                    if (rgieff[i] > 0) {
                        wtMass = static_cast<std::uint32_t>((lpfl->rgcsh[i] * static_cast<std::uint32_t>(app.state.rglpshdef[lpfl->iPlayer][i].hul.wtEmpty))) + wtCargoT;
                        lT = static_cast<std::uint32_t>((iEffCur * dTravel));
                        if (wtMass < 200 || (lT < 500000 && wtMass < 4000) || (lT < 100000 && wtMass < 20000)) {
                            lFuel += static_cast<std::int32_t>((wtMass * lT)) / 2000;
                        } else {
                            lFuel = static_cast<std::int32_t>((static_cast<double>(lT) * wtMass / 2000.0)) + lFuel;
                        }
                    }
                } else if (rgieff[i] > iEffCur && rgieff[i] < iEffNext) {
                    iEffNext = rgieff[i];
                }
            }
        }
        if (iEffNext == 999999)
            break;
        iEffCur = iEffNext;
    }
    if (!fRangeOnly) {
        lFuel += 9;
    }
    lFuel = static_cast<std::int32_t>((lFuel / 10));
    if (fRangeOnly) {
        if (lFuel == 0) {
            lFuel = 1000000000;
        } else if (lFuel > 100000) {
            lFuel = static_cast<std::int32_t>((lpfl->rgwtMin[4] / static_cast<std::int32_t>((lFuel / 1000))));
        } else {
            lFuel = static_cast<std::int32_t>((static_cast<std::int32_t>((lpfl->rgwtMin[4] * 1000)) / lFuel));
        }
    }
    return lFuel;
}

std::int16_t IFindIdealWarp(APPSESSION &app, FLEET *lpfl, std::int16_t fIgnoreScoops) {
    std::int16_t i;
    std::int16_t j;
    std::int16_t iWorst;
    ENGINE *lpengine;

    iWorst = 10;
    if (!lpfl) {
        lpfl = &app.state.sel.fl;
    }
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            for (j = 0; j < app.state.rglpshdef[lpfl->iPlayer][i].hul.chs && app.state.rglpshdef[lpfl->iPlayer][i].hul.rghs[j].grhst != hstEngine; j++) {
            }
            if (j == app.state.rglpshdef[lpfl->iPlayer][i].hul.chs) {
                iWorst = 0;
                break;
            }
            j = app.state.rglpshdef[lpfl->iPlayer][i].hul.rghs[j].iItem;
            lpengine = LpengineFromId(j);
            for (; iWorst > 0; iWorst--) {
                if (lpengine->rgcFuelUsed[iWorst] <= 120) {
                    if (lpengine->rgcFuelUsed[iWorst] > 0 && !fIgnoreScoops && j != iengineTransGalacticMizerScoop && j != iengineGalaxyScoop) {
                        if (iWorst >= 5 && lpengine->rgcFuelUsed[iWorst - 1] == 0) {
                            iWorst--;
                        } else if (iWorst >= 6 && lpengine->rgcFuelUsed[iWorst - 2] == 0) {
                            iWorst -= 2;
                        } else if (iWorst >= 7 && lpengine->rgcFuelUsed[iWorst - 3] == 0) {
                            iWorst -= 3;
                        }
                    }
                    if (iWorst == 10) {
                        switch (j) {
                        default:
                            iWorst = 9;
                        case iengineInterspace10:
                        case iengineEnigmaPulsar:
                        case iengineTransStar10:
                        case iengineTransGalacticMizerScoop:
                        case iengineGalaxyScoop:
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
    return iWorst;
}

// IWarpForWaypoint is IWarpBestForWaypoint's body. fFastest asks for the
// speed colonizers get, as IWarpFastestForWaypoint does.
static std::int16_t IWarpForWaypoint(APPSESSION &app, FLEET *lpfl, ORDER *lpord, std::int16_t fFastest) {
    std::int32_t lFuel;
    std::int16_t iWarp;
    std::int16_t cTravel;
    std::int16_t iwp;
    std::int16_t lDist;
    std::int16_t cSpeed;
    std::int16_t fGoFlatOutAi;
    std::int16_t fGoFlatOut;
    std::int16_t iWarpAi;
    std::int16_t iWarpSav;
    std::int16_t j;
    std::int16_t i;
    PLANET *lppl;
    std::int16_t iWarpOld;
    SCAN    scan;

    iWarpSav = lpord->iWarp;
    iWarp = IFindIdealWarp(app, nullptr, 0);
    if (app.state.fAi) {
        iWarpAi = IFindIdealWarp(app, nullptr, 1);
    }
    for (iwp = lpfl->cord - 1; iwp >= 0 && lpord != &lpfl->lpplord->rgord()[iwp]; iwp--) {
    }
    if (iwp <= 0) {
        return iWarp;
    }
    if (fFastest || lpord->grTask == grTaskColonize || lpord->grTask == grTaskScrap) {
        fGoFlatOut = 1;
    } else {
        fGoFlatOut = 0;
        for (i = 0; i < 16; i++) {
            if (lpfl->rgcsh[i] > 0) {
                for (j = 0; j < app.state.rglpshdef[lpfl->iPlayer][i].hul.chs; j++) {
                    if (app.state.rglpshdef[lpfl->iPlayer][i].hul.rghs[j].grhst == hstSpecialM &&
                        (app.state.rglpshdef[lpfl->iPlayer][i].hul.rghs[j].iItem == ispecialMColonizationModule ||
                         app.state.rglpshdef[lpfl->iPlayer][i].hul.rghs[j].iItem == ispecialMOrbitalConstructionModule)) {
                        fGoFlatOut = 1;
                        break;
                    }
                }
            }
        }
    }
    if (!fGoFlatOut && app.state.fAi) {
        fGoFlatOutAi = 1;
        fGoFlatOut = 1;
    } else {
        fGoFlatOutAi = 0;
    }
    if (iWarp < 9) {
        iWarpOld = iWarp;
        if (FFindNearestObject(app, lpord->pt, static_cast<GrobjClass>(grobjPlanet | mdExact), &scan)) {
            lppl = LpplFromId(app, scan.idpl);
        } else {
            lppl = nullptr;
        }
        if (!fGoFlatOut && (!lppl || (lppl->iPlayer != iplrNone && lppl->iPlayer != app.state.idPlayer))) {
            if (iwp > 1 && lpord[-1].iWarp > static_cast<std::uint16_t>(iWarp) && lpord[-1].iWarp <= 10) {
                iWarp = lpord[-1].iWarp;
            }
            if (LFuelUseToWaypoint(app, lpfl, iwp, 1) >= static_cast<std::int32_t>((ProductionIO().pfnFleetStat(app, lpfl, 1) / 10)) || lpfl->rgwtMin[4] < static_cast<std::int32_t>((ProductionIO().pfnFleetStat(app, lpfl, 1) * 7)) / 10)
                goto LOptimizeSpeed;
            iWarp++;
            goto LTryLimitedSpeed;
        } else {
            if (fGoFlatOutAi && lppl && lppl->iPlayer == app.state.idPlayer && app.state.rglpshdefSB[app.state.idPlayer][lppl->isb].hul.ihuldef != ihuldefOrbitalFort) {
                fGoFlatOutAi = 0;
            }
            iWarp = 9;
        }
    LTryLimitedSpeed:
        while (iWarp > iWarpOld) {
            lpord->iWarp = iWarp;
            lFuel = LFuelUseToWaypoint(app, lpfl, iwp, 1);
            if (lFuel > lpfl->rgwtMin[4])
                goto LDecWarp;
            if (((lppl && lppl->fStarbase && lppl->iPlayer == app.state.idPlayer && PartQueryIO().pfnHull(app.state.rglpshdefSB[app.state.idPlayer][lppl->isb].hul.ihuldef)->hul.wtCargoMax != 0) ||
                 lFuel <= static_cast<std::int32_t>((lpfl->rgwtMin[4] / 2)) || fGoFlatOut))
                break;
        LDecWarp:
            iWarp--;
        }
        lpord->iWarp = iWarpSav;
    }
    if (fGoFlatOutAi && iWarp > iWarpAi) {
        iWarp = iWarpAi;
    }
LOptimizeSpeed:
    if (iWarp > 1 && lpord->grobj != grobjFleet) {
        lDist = LOWORD(static_cast<std::int32_t>(DGetDistance(lpord->pt.x, lpord->pt.y, lpord[-1].pt.x, lpord[-1].pt.y)));
        cSpeed = iWarp * iWarp;
        cTravel = static_cast<std::int16_t>((iWarp * iWarp + lDist - 1)) / cSpeed;
        do {
            iWarp--;
            if (iWarp <= 1)
                break;
            cSpeed = iWarp * iWarp;
        } while (cTravel == static_cast<std::int16_t>((lDist + cSpeed - 1)) / cSpeed);
        iWarp++;
    } else {
        cTravel = 2;
    }
    if (FCanFleetUseStargates(app, lpfl, lpord[-1].pt, lpord->pt) == 1) {
        iWarp = 11;
    }
    if (iWarp > 11) {
        iWarp = 9;
    }
    return iWarp;
}

// IWarpBestForWaypoint picks the warp for the leg to lpord, a waypoint of
// lpfl's orders.
std::int16_t IWarpBestForWaypoint(APPSESSION &app, FLEET *lpfl, ORDER *lpord) { return IWarpForWaypoint(app, lpfl, lpord, 0); }

// IWarpFastestForWaypoint picks the warp a colonizing fleet gets for the
// leg to lpord: the fastest up to warp 9 whose fuel to lpord fits the
// tank, then the slowest that arrives as soon. Players set colonize tasks
// on scouts to get it; the scanner gives it to Alt+clicked waypoints.
std::int16_t IWarpFastestForWaypoint(APPSESSION &app, FLEET *lpfl, ORDER *lpord) { return IWarpForWaypoint(app, lpfl, lpord, 1); }

std::int32_t LFuelUseToWaypoint(APPSESSION &app, FLEET *lpfl, std::int16_t iwp, std::int16_t fMaxCargo) {
    std::int32_t lCur;
    std::int16_t iWarp;
    std::int16_t dist;
    PLANET *lppl;
    std::int16_t i;
    std::int32_t lTot;
    ORDER  *lpord;
    std::int16_t cYears;
    SHDEF  *lpshdef;
    std::int16_t j;
    double  dbl;
    std::int32_t l;
    std::int32_t lOneYearUse;
    std::int32_t lFuelGain;

    lTot = 0;
    lCur = 0;
    lpord = lpfl->lpplord->rgord();
    for (i = 0; i < iwp; i++) {
        iWarp = lpord[i + 1].iWarp;
        if (iWarp > 0 && iWarp < 11) {
            dbl = static_cast<double>((static_cast<double>(DGetDistance(lpord[i].pt.x, lpord[i].pt.y, lpord[i + 1].pt.x, lpord[i + 1].pt.y)) + 0.99999));
            dist = LOWORD(static_cast<std::int32_t>(dbl));
            dbl = static_cast<double>((static_cast<double>(dbl) / iWarp / iWarp));
            cYears = LOWORD(static_cast<std::int32_t>((static_cast<double>(dbl) + 0.9999)));
            l = EstFuelUse(app, lpfl, i, iWarp, -1, 0);
        } else {
            cYears = 1;
            l = 0;
        }
        if (cYears > 1) {
            lOneYearUse = EstFuelUse(app, lpfl, i, iWarp, static_cast<std::int16_t>((iWarp * iWarp)), 0);
            lFuelGain = static_cast<std::uint32_t>((lOneYearUse * static_cast<std::int16_t>((cYears - 1))));
            lFuelGain += EstFuelUse(app, lpfl, i, iWarp, static_cast<std::int16_t>((dist - iWarp * iWarp * (cYears - 1))), 0);
            if (lFuelGain > l) {
                l = lFuelGain;
            }
            lFuelGain = LCalcFuelGainFromRamScoops(app, lpfl, iWarp, static_cast<std::int16_t>((iWarp * iWarp)));
            for (j = 0; j < 16; j++) {
                if (lpfl->rgcsh[j] != 0) {
                    lpshdef = app.state.rglpshdef[app.state.idPlayer] + j;
                    if (lpshdef->hul.ihuldef == ihuldefFuelTransport || lpshdef->hul.ihuldef == ihuldefSuperFuelXport) {
                        lFuelGain += static_cast<std::uint32_t>((lpfl->rgcsh[j] * 200));
                    }
                }
            }
            if (lFuelGain > 0) {
                if (lOneYearUse <= lFuelGain) {
                    l = lOneYearUse;
                } else {
                    lOneYearUse = static_cast<std::uint32_t>(((lOneYearUse - lFuelGain) * static_cast<std::int16_t>((cYears - 1)))) + lOneYearUse;
                    if (lOneYearUse < l) {
                        l = lOneYearUse;
                    }
                }
            }
        }
        lCur += l;
        if (lCur > lTot) {
            lTot = lCur;
        }
        if (lpord[i + 1].grobj == grobjPlanet) {
            lppl = LpplFromId(app, lpord[i + 1].id);
            if (lppl && lppl->iPlayer == app.state.idPlayer && lppl->fStarbase && PartQueryIO().pfnHull(app.state.rglpshdefSB[app.state.idPlayer][lppl->isb].hul.ihuldef)->hul.wtCargoMax != 0) {
                lCur = 0;
            }
        }
    }
    return lTot;
}

void FleetTransferCargoBalance(APPSESSION &app, FLEET *pflNew1, FLEET *pflNew2) {
    std::int16_t iplr;
    std::int32_t rgCargoCapLoss[2];
    std::int32_t wtCargoXfer;
    std::int16_t fDeadFleet;
    std::int32_t wtCargoTot;
    std::int16_t rgrgcshLoss[2][16];
    std::int32_t rgrgCargoDelta[2][5];
    std::int32_t rgFuelCapacity[2];
    FLEET  *rgpflNew[2];
    std::int16_t wtCargoMax;
    std::int16_t wtFuelMax;
    std::int16_t i;
    std::int32_t lChg;
    std::int32_t rgFuelCapLoss[2];
    FLEET   rgflCur[2];
    std::int16_t j;
    SHDEF  *lpshdef;
    std::int32_t rgCargoCapacity[2];
    std::int16_t ishdef;
    std::int32_t l;
    std::int32_t cshDmgDst;
    std::int32_t cshDmgSrc;
    std::int16_t iSrc;
    std::int32_t pctNew;
    std::int32_t cshDmgMoved;

    fDeadFleet = 0;
    rgpflNew[0] = pflNew1;
    rgpflNew[1] = pflNew2;
    iplr = pflNew1->iPlayer;
    for (i = 0; i < 2; i++) {
        if (rgpflNew[i]->fDead) {
            fDeadFleet = 1;
            rgflCur[i] = {};
            rgflCur[i].iPlayer = iplr;
        } else {
            FleetQueryIO().pfnLookup(app, rgpflNew[i]->id, &rgflCur[i]);
        }
        rgCargoCapLoss[i] = 0;
        rgCargoCapacity[i] = 0;
        rgFuelCapLoss[i] = 0;
        rgFuelCapacity[i] = 0;
        for (j = 0; j < 5; j++) {
            rgrgCargoDelta[i][j] = 0;
        }
    }
    for (ishdef = 0; ishdef < 16; ishdef++) {
        if (rgflCur[0].rgcsh[ishdef] != 0 || rgflCur[1].rgcsh[ishdef] != 0) {
            lpshdef = app.state.rglpshdef[iplr] + ishdef;
            wtFuelMax = WtMaxShdefStat(lpshdef, 1);
            wtCargoMax = WtMaxShdefStat(lpshdef, 2);
            for (i = 0; i < 2; i++) {
                rgrgcshLoss[i][ishdef] = rgflCur[i].rgcsh[ishdef] - rgpflNew[i]->rgcsh[ishdef];
                if (rgflCur[i].rgcsh[ishdef] != 0) {
                    rgFuelCapacity[i] += static_cast<std::uint32_t>((rgflCur[i].rgcsh[ishdef] * wtFuelMax));
                    rgCargoCapacity[i] += static_cast<std::uint32_t>((rgflCur[i].rgcsh[ishdef] * wtCargoMax));
                    if (rgrgcshLoss[i][ishdef] > 0) {
                        rgFuelCapLoss[i] += static_cast<std::uint32_t>((rgrgcshLoss[i][ishdef] * wtFuelMax));
                        rgCargoCapLoss[i] += static_cast<std::uint32_t>((rgrgcshLoss[i][ishdef] * wtCargoMax));
                    }
                }
            }
            if (!fDeadFleet && rgrgcshLoss[0][ishdef] == -rgrgcshLoss[1][ishdef] && rgrgcshLoss[0][ishdef] != 0) {
                iSrc = rgrgcshLoss[0][ishdef] < 0;
                if (rgflCur[iSrc].rgcsh[ishdef] > 0) {
                    cshDmgSrc = static_cast<std::int32_t>((rgflCur[iSrc].rgdv[ishdef].pctSh() * rgflCur[iSrc].rgcsh[ishdef])) / 100;
                } else {
                    cshDmgSrc = 0;
                }
                if (rgflCur[iSrc == 0].rgcsh[ishdef] > 0) {
                    cshDmgDst = static_cast<std::int32_t>((rgflCur[iSrc == 0].rgdv[ishdef].pctSh() * rgflCur[iSrc == 0].rgcsh[ishdef])) / 100;
                } else {
                    cshDmgDst = 0;
                }
                if (cshDmgSrc != 0 && cshDmgDst != 0) {
                    if (cshDmgSrc > rgrgcshLoss[iSrc][ishdef]) {
                        cshDmgMoved = rgrgcshLoss[iSrc][ishdef];
                    } else {
                        cshDmgMoved = cshDmgSrc;
                    }
                    pctNew = static_cast<std::int32_t>((static_cast<std::int32_t>((static_cast<std::uint32_t>((cshDmgDst * rgflCur[iSrc == 0].rgdv[ishdef].pctDp())) +
                                                 static_cast<std::uint32_t>((cshDmgMoved * rgflCur[iSrc].rgdv[ishdef].pctDp())) + rgpflNew[iSrc == 0]->rgcsh[ishdef] - 1)) /
                                       rgpflNew[iSrc == 0]->rgcsh[ishdef]));
                    rgpflNew[iSrc == 0]->rgdv[ishdef].dp = rgpflNew[iSrc == 0]->rgdv[ishdef].pctSh() | (LOWORD(pctNew) & 0x1ff) * 0x80;
                    pctNew = static_cast<std::int32_t>((static_cast<std::int32_t>((static_cast<std::uint32_t>(((cshDmgDst + cshDmgMoved) * 100)) + rgpflNew[iSrc == 0]->rgcsh[ishdef] - 1)) /
                                       rgpflNew[iSrc == 0]->rgcsh[ishdef]));
                    rgpflNew[iSrc == 0]->rgdv[ishdef].dp = (rgpflNew[iSrc == 0]->rgdv[ishdef].dp & 0xff80) | (LOWORD(pctNew) & 0x7f);
                    if (cshDmgMoved == cshDmgSrc) {
                        rgpflNew[iSrc]->rgdv[ishdef].dp = 0;
                    } else {
                        pctNew = static_cast<std::int32_t>((static_cast<std::int32_t>((static_cast<std::uint32_t>(((cshDmgSrc - cshDmgMoved) * 100)) + rgpflNew[iSrc]->rgcsh[ishdef] - 1)) /
                                           rgpflNew[iSrc]->rgcsh[ishdef]));
                        rgpflNew[iSrc]->rgdv[ishdef].SetPctSh(LOWORD(pctNew));
                    }
                } else if (cshDmgSrc != 0) {
                    if (cshDmgSrc > rgrgcshLoss[iSrc][ishdef]) {
                        cshDmgMoved = rgrgcshLoss[iSrc][ishdef];
                    } else {
                        cshDmgMoved = cshDmgSrc;
                    }
                    rgpflNew[iSrc == 0]->rgdv[ishdef].dp = rgpflNew[iSrc == 0]->rgdv[ishdef].pctSh() | (rgpflNew[iSrc]->rgdv[ishdef].pctDp() & 0x1ff) * 0x80;
                    pctNew = static_cast<std::int32_t>((static_cast<std::int32_t>((static_cast<std::uint32_t>((cshDmgMoved * 100)) + rgpflNew[iSrc == 0]->rgcsh[ishdef] - 1)) / rgpflNew[iSrc == 0]->rgcsh[ishdef]));
                    rgpflNew[iSrc == 0]->rgdv[ishdef].dp = (rgpflNew[iSrc == 0]->rgdv[ishdef].dp & 0xff80) | (LOWORD(pctNew) & 0x7f);
                    if (cshDmgMoved == cshDmgSrc) {
                        rgpflNew[iSrc]->rgdv[ishdef].dp = 0;
                    } else {
                        pctNew = static_cast<std::int32_t>((static_cast<std::int32_t>((static_cast<std::uint32_t>(((cshDmgSrc - cshDmgMoved) * 100)) + rgpflNew[iSrc]->rgcsh[ishdef] - 1)) /
                                           rgpflNew[iSrc]->rgcsh[ishdef]));
                        rgpflNew[iSrc]->rgdv[ishdef].SetPctSh(LOWORD(pctNew));
                    }
                } else if (cshDmgDst != 0) {
                    pctNew = static_cast<std::int32_t>((static_cast<std::int32_t>((static_cast<std::uint32_t>((cshDmgDst * 100)) + rgpflNew[iSrc == 0]->rgcsh[ishdef] - 1)) / rgpflNew[iSrc == 0]->rgcsh[ishdef]));
                    rgpflNew[iSrc == 0]->rgdv[ishdef].dp = (rgpflNew[iSrc == 0]->rgdv[ishdef].dp & 0xff80) | (LOWORD(pctNew) & 0x7f);
                } else {
                    rgpflNew[iSrc == 0]->rgdv[ishdef].dp = rgpflNew[iSrc == 0]->rgdv[ishdef].dp & 0xff80;
                }
            }
        }
    }
    for (i = 0; i < 2; i++) {
        if (rgFuelCapacity[i] != 0) {
            if (rgpflNew[i]->rgwtMin[4] > 45000 || rgFuelCapLoss[i] > 45000) {
                lChg = static_cast<std::int32_t>((static_cast<double>(rgpflNew[i]->rgwtMin[4]) * rgFuelCapLoss[i] / rgFuelCapacity[i]));
            } else {
                lChg = static_cast<std::int32_t>((static_cast<std::int32_t>((rgpflNew[i]->rgwtMin[4] * rgFuelCapLoss[i])) / rgFuelCapacity[i]));
            }
            rgrgCargoDelta[i][4] -= lChg;
        }
        if (rgCargoCapacity[i] != 0) {
            wtCargoTot = 0;
            for (j = 0; j <= 3; j++) {
                wtCargoTot += rgpflNew[i]->rgwtMin[j];
            }
            if (wtCargoTot > 45000 || rgCargoCapLoss[i] > 45000) {
                wtCargoXfer = static_cast<std::int32_t>((static_cast<double>(wtCargoTot) * rgCargoCapLoss[i] / rgCargoCapacity[i]));
            } else {
                wtCargoXfer = static_cast<std::int32_t>((static_cast<std::int32_t>((wtCargoTot * rgCargoCapLoss[i])) / rgCargoCapacity[i]));
            }
            lChg = wtCargoXfer;
            if (wtCargoXfer != 0 && wtCargoTot != 0) {
                for (j = 0; j <= 3; j++) {
                    if (rgpflNew[i]->rgwtMin[j] > 45000 || wtCargoXfer > 45000) {
                        l = static_cast<std::int32_t>((static_cast<double>(rgpflNew[i]->rgwtMin[j]) * wtCargoXfer / wtCargoTot));
                    } else {
                        l = static_cast<std::int32_t>((static_cast<std::int32_t>((rgpflNew[i]->rgwtMin[j] * wtCargoXfer)) / wtCargoTot));
                    }
                    l = l < lChg ? l : lChg;
                    rgrgCargoDelta[i][j] -= l;
                    lChg -= l;
                }
                if (lChg > 0) {
                    for (j = 0; j <= 3 && lChg > 0; j++) {
                        if (rgpflNew[i]->rgwtMin[j] + rgrgCargoDelta[i][j] > 0) {
                            rgrgCargoDelta[i][j]--;
                            lChg--;
                        }
                    }
                }
            }
        }
    }
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 5; j++) {
            rgpflNew[i]->rgwtMin[j] += rgrgCargoDelta[i][j];
            rgpflNew[i == 0]->rgwtMin[j] -= rgrgCargoDelta[i][j];
        }
    }
    return;
}

void DestroyAllIshdefSB(APPSESSION &app, std::int16_t ishdefSB, std::int16_t iplr) {
    PLANET *lppl;
    PLANET *lpplMac;

    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iplr && lppl->fStarbase && lppl->isb == ishdefSB) {
            lppl->fStarbase = 0;
            ProductionIO().pfnKillShips(app, lppl);
            ProductionIO().pfnKillPackets(app, lppl);
        }
    }
    return;
}

void DestroyAllIshdef(APPSESSION &app, std::int16_t ishdef, std::int16_t iplr) {
    FLEET   flDead;
    std::int16_t cKill;
    FLEET  *lpfl;
    std::int16_t i;
    std::int16_t grbit;
    std::int16_t j;
    std::int16_t cDel;
    FLEET   flNew;

    cDel = 0;
    if (ishdef >= 16) {
        DestroyAllIshdefSB(app, ishdef - 16, iplr);
        PresentationIO().pfnInvalidateReport(app, rptPlanets, 1);
    } else {
        for (lpfl = *app.state.rglpfl, i = 0; i < app.state.cFleet; lpfl = app.state.rglpfl[i]) {
            if (lpfl->iPlayer != iplr || lpfl->rgcsh[ishdef] <= 0)
                goto IncrementI;
            flDead = {};
            cKill = lpfl->rgcsh[ishdef];
            cDel += cKill;
            for (j = 0; j < 16 && (j == ishdef || lpfl->rgcsh[j] == 0); j++) {
            }
            if (j == 16) {
                lpfl->rgcsh[ishdef] = 0;
                FDeleteFleet(app, lpfl->id, grobjNone, -1);
                continue;
            }
            flDead.SetIplr(iplr);
            flDead.fDead = 1;
            flDead.rgcsh[ishdef] = cKill;
            flNew = *lpfl;
            flNew.rgcsh[ishdef] = 0;
            FleetTransferCargoBalance(app, &flNew, &flDead);
            *lpfl = flNew;
            if (app.state.sel.grobj == grobjFleet && app.state.sel.fl.id == flNew.id) {
                FleetQueryIO().pfnLookup(app, flNew.id, &app.state.sel.fl);
                ShowScanSel(app, 0);
                FillShipDD(app, app.state.sel.fl.id);
                grbit = -31819;
                FleetQueryIO().pfnLookup(app, app.state.sel.fl.id, &app.state.sel.fl);
                FillFleetCompLB(app);
                PresentationIO().pfnRedraw(app, static_cast<TileBits>(grbit));
                InvalidateMine(app);
            }
        IncrementI:
            i++;
        }
        PresentationIO().pfnInvalidateReport(app, rptFleets, 1);
    }
    RemoveIshdefFromAllQueues(app, ishdef, 0);
    return;
}

void RemoveIshdefFromAllQueues(APPSESSION &app, std::int16_t ishdef, std::int16_t fSpaceDocks) {
    std::int16_t iprod;
    PLANET *lppl;
    std::int16_t iDst;
    PLANET *lpplMac;
    PROD   *lpprod;

    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->lpplprod && lppl->lpplprod->iprodMac != 0 && lppl->iPlayer == app.state.idPlayer && lppl->fStarbase &&
            (!fSpaceDocks || app.state.rglpshdefSB[app.state.idPlayer][lppl->isb].hul.ihuldef == ihuldefSpaceDock)) {
            iDst = 0;
            iprod = 0;
            lpprod = lppl->lpplprod->rgprod();
            while (iprod < lppl->lpplprod->iprodMac) {
                if (lpprod->grobj != grobjFleet || lpprod->iItem != static_cast<std::uint32_t>(ishdef)) {
                    if (iDst != iprod) {
                        lppl->lpplprod->rgprod()[iDst] = *lpprod;
                    }
                    iDst++;
                }
                iprod++;
                lpprod++;
            }
            if (iDst == 0) {
                FreePl(app, lppl->lpplprod);
                lppl->lpplprod = nullptr;
            } else if (iDst != iprod) {
                lppl->lpplprod->iprodMac = iDst;
            }
        }
    }
    if (app.state.sel.grobj == grobjPlanet && app.state.sel.pl.lpplprod) {
        PlanetQueryIO().pfnLookup(app, app.state.sel.pl.id, &app.state.sel.pl);
        ProductionIO().pfnRefresh(app);
    }
    return;
}

std::int16_t CshQueued(APPSESSION &app, std::int16_t ishdef, std::int16_t *pfProgress, std::int16_t fSpaceDocks) {
    std::int16_t iprod;
    PLANET *lppl;
    std::int16_t csh;
    PLANET *lpplMac;
    PROD   *lpprod;

    csh = 0;
    *pfProgress = 0;
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->lpplprod && lppl->lpplprod->iprodMac != 0 && lppl->iPlayer == app.state.idPlayer && lppl->fStarbase &&
            (!fSpaceDocks || app.state.rglpshdefSB[app.state.idPlayer][lppl->isb].hul.ihuldef == ihuldefSpaceDock)) {
            iprod = 0;
            lpprod = lppl->lpplprod->rgprod();
            while (iprod < lppl->lpplprod->iprodMac) {
                if (lpprod->grobj == grobjFleet && lpprod->iItem == static_cast<std::uint32_t>(ishdef)) {
                    csh += lpprod->cItem;
                    if (lpprod->pct != 0) {
                        *pfProgress = 1;
                    }
                }
                iprod++;
                lpprod++;
            }
        }
    }
    return csh;
}

void Merge2Fleets(APPSESSION &app, FLEET *lpflDst, FLEET *lpflDel, std::int16_t fNoDelete) {
    FLEET   rgfl[2];
    std::int16_t i;
    std::int16_t csh;
    std::int16_t fLeft;

    rgfl[0] = *lpflDst;
    rgfl[1] = *lpflDel;
    fLeft = 0;
    for (i = 0; i < 16; i++) {
        /* Ship counts are int16_t. The original added without a limit, so a
           merge past 32767 ships of a design went negative; the ships that
           don't fit stay in the merged fleet. */
        csh = 32767 - rgfl[0].rgcsh[i];
        if (csh > rgfl[1].rgcsh[i]) {
            csh = rgfl[1].rgcsh[i];
        }
        rgfl[0].rgcsh[i] += csh;
        rgfl[1].rgcsh[i] -= csh;
        if (rgfl[1].rgcsh[i] != 0) {
            fLeft = 1;
        }
    }
    /* Ships that gated, fought or hit mines this turn can't heal, so the
       fleet they join can't either. The original dropped the merged fleet's
       fNoHeal before HealShips. */
    if (rgfl[1].fNoHeal) {
        rgfl[0].fNoHeal = 1;
    }
    FleetTransferCargoBalance(app, rgfl, &rgfl[1]);
    for (i = 0; i < 2; i++) {
        FleetQueryIO().pfnLookup(app, idWriteBack, &rgfl[i]);
    }
    if (fLeft) {
        PresentationIO().pfnInvalidateReport(app, rptFleets, 2);
    } else if (fNoDelete) {
        lpflDel->fDead = 1;
    } else {
        FDeleteFleet(app, rgfl[1].id, grobjFleet, rgfl[0].id);
        PresentationIO().pfnInvalidateReport(app, rptFleets, 2);
    }
    return;
}

void FleetOrdersChangeTarget(APPSESSION &app, FLEET *lpflOld) {
    std::int16_t    id;
    POINT16    pt;
    std::int16_t    fChg;
    FLEET     *lpfl;
    std::int16_t    iord;
    std::int16_t    iflMac;
    SCAN       scan;
    GrobjClass grobj;

    fChg = 0;
    for (iflMac = 0; iflMac < app.state.cFleet; iflMac++) {
        lpfl = app.state.rglpfl[iflMac];
        if (!app.state.rglpfl[iflMac])
            break;
        if (lpfl->lpplord) {
            for (iord = lpfl->cord - 1; iord >= 0; iord--) {
                if (lpfl->lpplord->rgord()[iord].grobj == grobjFleet && lpfl->lpplord->rgord()[iord].id == lpflOld->id) {
                    if (!fChg) {
                        pt = lpflOld->pt;
                        lpflOld->pt.x++;
                        if (FFindNearestObject(app, pt, static_cast<GrobjClass>(grobjPlanet | grobjFleet | mdExact), &scan)) {
                            if (scan.grobjFull & grobjFleet) {
                                grobj = grobjFleet;
                                id = app.state.rglpfl[scan.ifl]->id;
                            } else {
                                grobj = grobjPlanet;
                                id = scan.idpl;
                            }
                        } else {
                            grobj = grobjOther;
                            id = iord;
                        }
                        lpflOld->pt.x--;
                    }
                    lpfl->lpplord->rgord()[iord].id = id;
                    lpfl->lpplord->rgord()[iord].grobj = grobj;
                }
            }
        }
    }
    return;
}

void GetTruePartCost(APPSESSION &app, std::int16_t iPlayer, PART *ppart, std::uint16_t *rgCost) {
    std::int16_t  cExcess;
    std::int16_t  cCur;
    std::int16_t  i;
    NATIVEPARTCOMMONVIEW lpcom;

    lpcom = ppart->pcom();
    for (i = 0; i < 3; i++) {
        rgCost[i] = lpcom.rgwtOreCost(i);
    }
    rgCost[3] = lpcom.resCost();
    if (iPlayer != iplrNone) {
        if ((ppart->hs.grhst & hstTerra) ||
            ((ppart->hs.grhst & hstPlanetary) && ppart->hs.iItem >= iplanetarySDI && ppart->hs.iItem <= iplanetaryNeutronShield) ||
            ((ppart->hs.grhst & hstPlanetary) && ppart->hs.iItem >= iplanetaryViewer50 && ppart->hs.iItem <= iplanetarySnooper620X))
            goto LOtherDiddles;
        cExcess = 100;
        for (i = 0; i < 6; i++) {
            cCur = app.state.rgplr[iPlayer].rgTech[i] - lpcom.rgTech()[i];
            if (lpcom.rgTech()[i] > 0 && cCur < cExcess) {
                cExcess = cCur;
            }
        }
        if (cExcess == 100) {
            for (i = 0; i < 6; i++) {
                if (app.state.rgplr[iPlayer].rgTech[i] < cExcess) {
                    cExcess = app.state.rgplr[iPlayer].rgTech[i];
                }
            }
        }
        if (cExcess >= 1) {
            if (cExcess > 19) {
                cExcess = 19;
            }
            if (RaceQueryIO().pfnGrbit(&app.state.rgplr[iPlayer], ibitRaceBleedingEdgeTech) != 0) {
                cExcess = 5 * cExcess;
                if (cExcess > 80) {
                    cExcess = 80;
                }
            } else {
                cExcess *= 4;
                if (cExcess > 75) {
                    cExcess = 75;
                }
            }
            for (i = 0; i < 4; i++) {
                if (rgCost[i] > 0) {
                    rgCost[i] -= LMulDiv(rgCost[i], cExcess, 100);
                    if (rgCost[i] == 0) {
                        rgCost[i] = 1;
                    }
                }
            }
        }
    LOtherDiddles:
        if (ppart->hs.grhst == hstSpecialSB && RaceQueryIO().pfnStat(&app.state.rgplr[iPlayer], rsMajorAdv) == raStargate &&
            RaceQueryIO().pfnStat(&app.state.rgplr[iPlayer], rsMajorAdv) == raStargate && ppart->hs.iItem >= ispecialSBStargate100250 &&
            ppart->hs.iItem <= ispecialSBStargateAnyAny) {
            for (i = 0; i < 4; i++) {
                rgCost[i] -= rgCost[i] >> 2;
            }
        } else {
            switch (ppart->hs.grhst) {
            case hstBeam:
            case hstTorp:
            case hstBomb:
                if (RaceQueryIO().pfnStat(&app.state.rgplr[iPlayer], rsMajorAdv) == raAttack) {
                    for (i = 0; i < 4; i++) {
                        rgCost[i] -= rgCost[i] >> 2;
                    }
                    break;
                }
                /* fallthrough */
            default:
                switch (ppart->hs.grhst) {
                case hstBeam:
                case hstTorp:
                case hstBomb:
                    if (RaceQueryIO().pfnStat(&app.state.rgplr[iPlayer], rsMajorAdv) == raDefend) {
                        for (i = 0; i < 4; i++) {
                            rgCost[i] += rgCost[i] >> 2;
                        }
                        break;
                    }
                    /* fallthrough */
                default:
                    if (ppart->hs.grhst == hstTerra && RaceQueryIO().pfnStat(&app.state.rgplr[iPlayer], rsMajorAdv) == raTerra) {
                        rgCost[3] = static_cast<std::uint32_t>(rgCost[3]) / 2;
                    } else if (ppart->hs.grhst == hstEngine && RaceQueryIO().pfnGrbit(&app.state.rgplr[iPlayer], ibitRaceCheapEngines) != 0) {
                        for (i = 0; i < 4; i++) {
                            rgCost[i] -= rgCost[i] >> 1;
                        }
                    }
                }
            }
        }
        if (cExcess < 1 && RaceQueryIO().pfnGrbit(&app.state.rgplr[iPlayer], ibitRaceBleedingEdgeTech) != 0 && !app.state.gd.fDontCalcBleed) {
            for (i = 0; i < 6 && lpcom.rgTech()[i] <= 0; i++) {
            }
            if (i < 6) {
                app.state.gd.fBleedingEdge = 1;
                for (i = 0; i < 4; i++) {
                    rgCost[i] *= 2;
                }
            } else {
                app.state.gd.fBleedingEdge = 0;
            }
        } else {
            app.state.gd.fBleedingEdge = 0;
        }
    }
    return;
}
