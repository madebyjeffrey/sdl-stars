#include "common.h"
#include "appstate.h"
#include "part_query_io.h"
#include "race_query_io.h"

HULDEF *LphuldefSBFromId(isbhull id) { return &rghuldefSB[id]; }

HULDEF *LphuldefFromId(std::int16_t id) {
    if (id >= ihuldefOrbitalFort) {
        return LphuldefSBFromId(static_cast<isbhull>(id - ihuldefOrbitalFort));
    }
    return &rghuldef[id];
}

ENGINE *LpengineFromId(std::int16_t id) { return &rgengine[id]; }

SCANNER *LpscannerFromId(std::int16_t id) { return &rgscanner[id]; }

SHDEF *LpshdefT() { return rgshdefT; }

SHDEF *LpshdefSBT() { return rgshdefSBT; }

PLANETARY *LpplanetaryFromId(std::int16_t id) { return &rgplanetary[id]; }

std::int16_t FLookupPartX(APPSESSION &app, PART *ppart, std::uint16_t grhst, std::uint16_t iItem) {
    ppart->hs.grhst = static_cast<HullSlotType>(grhst);
    ppart->hs.iItem = iItem;
    ppart->hs.cItem = 0;
    return PartQueryIO().pfnLookup(app, ppart);
}

std::int16_t FLookupPart(APPSESSION &app, PART *ppart) {
    RaceAttribute raMajor;
    HS            hs;

    raMajor = static_cast<RaceAttribute>(app.state.idPlayer == iplrNone ? static_cast<int>(raNone) : RaceQueryIO().pfnStat(&app.state.rgplr[app.state.idPlayer], rsMajorAdv));
    hs = ppart->hs;
    switch (hs.grhst) {
    default:
        return mdPartAvailInvalid;
    case hstEngine:
        if (hs.iItem >= iengineCount) {
            return mdPartAvailInvalid;
        }
        ppart->SetPengine(&rgengine[hs.iItem]);
        if (app.state.idPlayer == iplrNone)
            break;
        if (hs.iItem == iengineSettlersDelight && raMajor != raCheapCol) {
            return mdPartAvailRestricted;
        }
        if (((hs.iItem >= iengineSubGalacticFuelScoop && hs.iItem <= iengineGalaxyScoop) || hs.iItem == iengineRadiatingHydroRamScoop) &&
            RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceNoRamscoops) != 0) {
            return mdPartAvailRestricted;
        }
        if ((hs.iItem == iengineGalaxyScoop || hs.iItem == iengineFuelMizer) && RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceIFE) == 0) {
            return mdPartAvailRestricted;
        }
        if (hs.iItem == iengineInterspace10 && RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceNoRamscoops) == 0) {
            return mdPartAvailRestricted;
        }
        if (!PartQueryIO().pfnHidden(app, ppart))
            break;
        return mdPartAvailRestricted;
    case hstShield:
        if (hs.iItem >= ishieldCount) {
            return mdPartAvailInvalid;
        }
        ppart->SetPshield(&rgshield[hs.iItem]);
        if (hs.iItem == ishieldShadowShield && raMajor != raStealth) {
            return mdPartAvailRestricted;
        }
        if (hs.iItem == ishieldCrobySharmor && raMajor != raDefend) {
            return mdPartAvailRestricted;
        }
        if (!PartQueryIO().pfnHidden(app, ppart))
            break;
        return mdPartAvailRestricted;
    case hstHull:
        if (hs.iItem >= ihuldefOrbitalFort) {
            return mdPartAvailInvalid;
        }
        ppart->SetPhul(&rghuldef[hs.iItem].hul);
        if (app.state.idPlayer == iplrNone)
            break;
        if ((hs.iItem == ihuldefMiniColonyShip || hs.iItem == ihuldefMetaMorph) && raMajor != raCheapCol) {
            return mdPartAvailRestricted;
        }
        if ((hs.iItem == ihuldefFuelTransport || hs.iItem == ihuldefSuperFreighter) && raMajor != raDefend) {
            return mdPartAvailRestricted;
        }
        switch (hs.iItem) {
        case ihuldefMiner:
        case ihuldefMaxiMiner:
        case ihuldefMidgetMiner:
        case ihuldefUltraMiner:
            if (RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceOBRM) != 0) {
                return mdPartAvailRestricted;
            }
            /* fallthrough */
        default:
            switch (hs.iItem) {
            case ihuldefMidgetMiner:
            case ihuldefMiner:
            case ihuldefUltraMiner:
                if (RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceARM) == 0) {
                    return mdPartAvailRestricted;
                }
                /* fallthrough */
            default:
                if ((hs.iItem == ihuldefDreadnought || hs.iItem == ihuldefBattleCruiser) && raMajor != raAttack) {
                    return mdPartAvailRestricted;
                }
                if (hs.iItem == ihuldefRogue && raMajor != raStealth) {
                    return mdPartAvailRestricted;
                }
                if (hs.iItem == ihuldefStealthBomber && raMajor != raStealth) {
                    return mdPartAvailRestricted;
                }
                if ((hs.iItem == ihuldefMiniMineLayer || hs.iItem == ihuldefSuperMineLayer) && raMajor != raMines) {
                    return mdPartAvailRestricted;
                }
                if (!PartQueryIO().pfnHidden(app, ppart))
                    break;
                return mdPartAvailRestricted;
            }
        }
        break;
    case hstSBHull:
        if (hs.iItem >= isbhullCount) {
            return mdPartAvailInvalid;
        }
        ppart->SetPhul(&rghuldefSB[hs.iItem].hul);
        if (app.state.idPlayer == iplrNone)
            break;
        if ((hs.iItem == isbhullSpaceDock || hs.iItem == isbhullUltraStation) && RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceISB) == 0) {
            return mdPartAvailRestricted;
        }
        if (hs.iItem != isbhullDeathStar || raMajor == raMacintosh)
            break;
        return mdPartAvailRestricted;
    case hstArmor:
        if (hs.iItem >= iarmorCount) {
            return mdPartAvailInvalid;
        }
        ppart->SetParmor(&rgarmor[hs.iItem]);
        if (hs.iItem == iarmorDepletedNeutronium && raMajor != raStealth) {
            return mdPartAvailRestricted;
        }
        if (hs.iItem == iarmorFieldedKelarium && raMajor != raDefend) {
            return mdPartAvailRestricted;
        }
        if (!PartQueryIO().pfnHidden(app, ppart))
            break;
        return mdPartAvailRestricted;
    case hstSpecialE:
        if (hs.iItem >= ispecialECount) {
            return mdPartAvailInvalid;
        }
        ppart->SetPspecial(&rgspecialE[hs.iItem]);
        if (app.state.idPlayer == iplrNone)
            break;
        if (PartQueryIO().pfnHidden(app, ppart)) {
            return mdPartAvailRestricted;
        }
        switch (hs.iItem) {
        case ispecialETransportCloaking:
        case ispecialEUltraStealthCloak:
            if (raMajor == raStealth)
                break;
            return mdPartAvailRestricted;
        case ispecialEEnergyDampener:
            if (raMajor == raMines)
                break;
            return mdPartAvailRestricted;
        case ispecialEAntiMatterGenerator:
            if (raMajor == raStargate)
                break;
            return mdPartAvailRestricted;
        case ispecialEFluxCapacitor:
            if (raMajor == raCheapCol)
                break;
            return mdPartAvailRestricted;
        case ispecialEJammer10:
        case ispecialEJammer50:
        case ispecialETachyonDetector:
            if (raMajor != raDefend) {
                return mdPartAvailRestricted;
            }
        case ispecialEStealthCloak:
        case ispecialESuperStealthCloak:
        case ispecialEMultiFunctionPod:
        case ispecialEBattleComputer:
        case ispecialEBattleSuperComputer:
        case ispecialEBattleNexus:
        case ispecialEJammer20:
        case ispecialEJammer30:
        case ispecialEEnergyCapacitor:
            break;
        }
        break;
    case hstSpecialM:
        if (hs.iItem >= ispecialMCount) {
            return mdPartAvailInvalid;
        }
        ppart->SetPspecial(&rgspecialM[hs.iItem]);
        if (app.state.idPlayer == iplrNone)
            break;
        if (PartQueryIO().pfnHidden(app, ppart)) {
            return mdPartAvailRestricted;
        }
        if (hs.iItem == ispecialMColonizationModule && raMajor == raMacintosh) {
            return mdPartAvailRestricted;
        }
        if (hs.iItem != ispecialMOrbitalConstructionModule || raMajor == raMacintosh)
            break;
        return mdPartAvailRestricted;
    case hstSpecialSB:
        if (hs.iItem >= ispecialSBCount) {
            return mdPartAvailInvalid;
        }
        ppart->SetPspecialsb(&rgspecialSB[hs.iItem]);
        if (app.state.idPlayer == iplrNone)
            break;
        if (hs.iItem >= ispecialSBMassDriver5 && hs.iItem <= ispecialSBUltraDriver13) {
            if (hs.iItem == ispecialSBMassDriver7 || hs.iItem == ispecialSBUltraDriver10 || raMajor == raMassAccel)
                break;
            return mdPartAvailRestricted;
        }
        if (hs.iItem < ispecialSBStargate100250 || hs.iItem > ispecialSBStargateAnyAny)
            break;
        if (raMajor != raStargate && (hs.iItem == ispecialSBStargateAny300 || hs.iItem >= ispecialSBStargate100Any)) {
            return mdPartAvailRestricted;
        }
        if (raMajor != raCheapCol)
            break;
        return mdPartAvailRestricted;
    case hstMines:
        if (hs.iItem >= iminesCount) {
            return mdPartAvailInvalid;
        }
        ppart->SetPmines(&rgmines[hs.iItem]);
        if (app.state.idPlayer == iplrNone)
            break;
        switch (hs.iItem) {
        case iminesMineDispenser40:
        case iminesMineDispenser80:
        case iminesMineDispenser130:
        case iminesHeavyDispenser50:
        case iminesHeavyDispenser110:
        case iminesHeavyDispenser200:
        case iminesSpeedTrap30:
        case iminesSpeedTrap50:
            if (raMajor != raMines) {
                return mdPartAvailRestricted;
            }
            /* fallthrough */
        default:
            if (hs.iItem == iminesSpeedTrap20 && raMajor != raMines && raMajor != raDefend) {
                return mdPartAvailRestricted;
            }
            if (hs.iItem != iminesMineDispenser50 || raMajor != raAttack)
                break;
            return mdPartAvailRestricted;
        }
        break;
    case hstMining:
        if (hs.iItem >= iminingCount) {
            return mdPartAvailInvalid;
        }
        ppart->SetPmining(&rgmining[hs.iItem]);
        if (app.state.idPlayer == iplrNone)
            break;
        switch (hs.iItem) {
        case iminingRoboMiner:
        case iminingRoboMaxiMiner:
        case iminingRoboSuperMiner:
        case iminingRoboMidgetMiner:
        case iminingRoboUltraMiner:
            if (RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceOBRM) != 0) {
                return mdPartAvailRestricted;
            }
            /* fallthrough */
        default:
            if ((hs.iItem == iminingRoboMidgetMiner || hs.iItem == iminingRoboUltraMiner) && RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceARM) == 0) {
                return mdPartAvailRestricted;
            }
            if (hs.iItem == iminingOrbitalAdjuster && raMajor != raTerra) {
                return mdPartAvailRestricted;
            }
            if (!PartQueryIO().pfnHidden(app, ppart))
                break;
            return mdPartAvailRestricted;
        }
        break;
    case hstScanner:
        if (hs.iItem >= iscannerCount) {
            return mdPartAvailInvalid;
        }
        ppart->SetPscanner(&rgscanner[hs.iItem]);
        if (app.state.idPlayer == iplrNone)
            break;
        switch (hs.iItem) {
        case iscannerFerretScanner:
        case iscannerDolphinScanner:
        case iscannerElephantScanner:
            if (RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceNoAdvScanner) != 0) {
                return mdPartAvailRestricted;
            }
            /* fallthrough */
        default:
            switch (hs.iItem) {
            case iscannerChameleonScanner:
            case iscannerPickPocketScanner:
            case iscannerRobberBaronScanner:
                if (raMajor != raStealth) {
                    return mdPartAvailRestricted;
                }
            default:
                break;
            }
        }
        break;
    case hstBeam:
        if (hs.iItem >= ibeamCount) {
            return mdPartAvailInvalid;
        }
        ppart->SetPbeam(&rgbeam[hs.iItem]);
        if (hs.iItem == ibeamMiniGun && raMajor != raDefend) {
            return mdPartAvailRestricted;
        }
        if ((hs.iItem == ibeamBlunderbuss || hs.iItem == ibeamGatlingNeutrinoCannon) && raMajor != raAttack) {
            return mdPartAvailRestricted;
        }
        if (!PartQueryIO().pfnHidden(app, ppart))
            break;
        return mdPartAvailRestricted;
    case hstTorp:
        if (hs.iItem >= itorpCount) {
            return mdPartAvailInvalid;
        }
        ppart->SetPtorp(&rgtorp[hs.iItem]);
        if (app.state.idPlayer == iplrNone || !PartQueryIO().pfnHidden(app, ppart))
            break;
        return mdPartAvailRestricted;
    case hstBomb:
        if (hs.iItem >= ibombCount) {
            return mdPartAvailInvalid;
        }
        ppart->SetPbomb(&rgbomb[hs.iItem]);
        if (app.state.idPlayer == iplrNone)
            break;
        if (hs.iItem >= ibombSmartBomb && hs.iItem <= ibombAnnihilatorBomb && raMajor == raDefend) {
            return mdPartAvailRestricted;
        }
        if (hs.iItem == ibombRetroBomb && raMajor != raTerra) {
            return mdPartAvailRestricted;
        }
        if (!PartQueryIO().pfnHidden(app, ppart))
            break;
        return mdPartAvailRestricted;
    case hstPlanetary:
        if (hs.iItem >= iplanetaryCount) {
            return mdPartAvailInvalid;
        }
        ppart->SetPplanetary(&rgplanetary[hs.iItem]);
        if (app.state.idPlayer == iplrNone)
            break;
        if (hs.iItem >= iplanetaryViewer50 && hs.iItem <= iplanetarySnooper620X && ppart->pplanetary()->grAbility < 0 &&
            RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceNoAdvScanner) != 0) {
            return mdPartAvailRestricted;
        }
        if (hs.iItem >= iplanetaryViewer50 && hs.iItem <= iplanetarySnooper620X && raMajor == raMacintosh) {
            return mdPartAvailRestricted;
        }
        if (hs.iItem >= iplanetarySDI && hs.iItem <= iplanetaryNeutronShield && raMajor == raMacintosh) {
            return mdPartAvailRestricted;
        }
        if (hs.iItem >= iplanetaryLaserBattery && hs.iItem <= iplanetaryNeutronShield && raMajor == raAttack) {
            return mdPartAvailRestricted;
        }
        if (!PartQueryIO().pfnHidden(app, ppart))
            break;
        return mdPartAvailRestricted;
    case hstTerra:
        if (hs.iItem >= iterraCount) {
            return mdPartAvailInvalid;
        }
        ppart->SetPterra(&rgterra[hs.iItem]);
        if (app.state.idPlayer != iplrNone && hs.iItem >= iterraTotalTerraform3 && hs.iItem <= iterraTotalTerraform30 &&
            RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceTT) == 0) {
            return mdPartAvailRestricted;
        }
    }
    return app.state.idPlayer == iplrNone ? static_cast<int>(mdPartAvailAvailable) : PartQueryIO().pfnTechStatus(app, reinterpret_cast<char *>(ppart->pcom().rgTech()));
}

void LookupBestPlanetaryScanner(APPSESSION &app, PART *ppart) {
    ppart->hs.iItem = iplanetarySnooper620X;
    ppart->hs.grhst = hstPlanetary;
    while (ppart->hs.iItem >= iplanetaryViewer50 && PartQueryIO().pfnLookup(app, ppart) != mdPartAvailAvailable && ppart->hs.iItem != iplanetaryViewer50) {
        ppart->hs.iItem--;
    }
    return;
}

std::int16_t TechStatus(APPSESSION &app, char *rgTech) {
    std::int16_t fInAWhile;
    std::int16_t i;
    std::int16_t fAlmost;
    std::int16_t cMiss;

    cMiss = 0;
    fAlmost = 0;
    fInAWhile = 0;
    for (i = 0; i < 6; i++) {
        if (app.state.rgplr[app.state.idPlayer].rgTech[i] < rgTech[i]) {
            cMiss++;
            if (i == app.state.rgplr[app.state.idPlayer].iTechNow()) {
                if (app.state.rgplr[app.state.idPlayer].rgTech[i] + 1 == rgTech[i]) {
                    fAlmost = 1;
                } else {
                    fInAWhile = i + 1;
                }
            }
        }
    }
    if (cMiss == 0) {
        return mdPartAvailAvailable;
    }
    if (cMiss == 1 && fAlmost) {
        return mdPartAvailNextTech;
    }
    if (cMiss == 1 && fInAWhile) {
        return rgTech[fInAWhile - 1] - app.state.rgplr[app.state.idPlayer].rgTech[fInAWhile - 1] + 1;
    }
    return mdPartAvailOtherTech;
}
