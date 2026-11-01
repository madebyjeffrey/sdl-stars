#include "common.h"
#include "appstate.h"
#include "text_io.h"
#include "turn_io.h"
#include "production_io.h"
#include "heap_io.h"
#include "research_io.h"
#include "part_query_io.h"
#include "race_query_io.h"
#include "alert_io.h"
#include "random_io.h"

uint8_t rgbrcStart[136] = {68,  65,  88,  20,  136, 129, 17,  136, 129, 24,  20,  134, 65,  72,  130, 65,  88,  130, 23,  134, 19,  17,  81,
                           130, 134, 104, 40,  21,  49,  97,  131, 134, 104, 56,  22,  19,  49,  104, 131, 22,  97,  56,  134, 19,  68,  18,
                           21,  24,  65,  72,  84,  113, 120, 131, 134, 49,  104, 131, 22,  97,  56,  134, 19,  67,  54,  102, 65,  88,  130,
                           23,  134, 19,  97,  56,  33,  132, 21,  120, 17,  49,  81,  113, 19,  21,  23,  56,  88,  131, 133, 135, 68,  17,
                           49,  81,  113, 130, 132, 134, 136, 104, 72,  40,  23,  21,  19,  17,  49,  81,  113, 130, 132, 134, 136, 104, 72,
                           40,  23,  21,  19,  68,  17,  49,  81,  113, 130, 132, 134, 136, 104, 72,  40,  23,  21,  19,  51,  102};

int16_t FDeleteBattlePlan(APPSESSION &app, int16_t iplan, int16_t fWarn) {
    int16_t fFoundBigger;
    int16_t iflMac;
    int16_t i;
    FLEET  *lpfl;

    fFoundBigger = 0;
LCommit:
    for (iflMac = 0; iflMac < app.state.cFleet; iflMac++) {
        lpfl = app.state.rglpfl[iflMac];
        if (!app.state.rglpfl[iflMac])
            break;
        if (lpfl->iPlayer >= app.state.idPlayer) {
            if (lpfl->iPlayer > app.state.idPlayer)
                break;
            if (lpfl->iplan >= (uint16_t)iplan) {
                if (lpfl->iplan > (uint16_t)iplan) {
                    if (!fWarn) {
                        lpfl->iplan--;
                    } else {
                        fFoundBigger = 1;
                    }
                } else if (fWarn) {
                    if (AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsCurrentlyHaveFleetsUsingBattlePlanIf, nullptr), win32::MB_OKCANCEL | win32::MB_ICONEXCLAMATION) == IDCANCEL)
                        return 0;
                    fWarn = 0;
                    goto LCommit;
                } else {
                    lpfl->iplan--;
                }
            }
        }
    }
    if (fWarn && fFoundBigger) {
        fWarn = 0;
        goto LCommit;
    }
    app.state.rgcbtlplan[app.state.idPlayer]--;
    for (i = iplan; i < app.state.rgcbtlplan[app.state.idPlayer]; i++) {
        app.state.rglpbtlplan[app.state.idPlayer][i] = app.state.rglpbtlplan[app.state.idPlayer][i + 1];
        app.state.rglpbtlplan[app.state.idPlayer][i].iplan = i;
    }
    return 1;
}

int16_t FFleetHasBombs(APPSESSION &app, FLEET *lpfl) {
    HUL       *lphul;
    HullAttack imd;
    int16_t    ishdef;

    for (ishdef = 0; ishdef < 16; ishdef++) {
        if (lpfl->rgcsh[ishdef] != 0) {
            lphul = &app.state.rglpshdef[lpfl->iplr()][ishdef].hul;
            imd = static_cast<HullAttack>(PartQueryIO().pfnHull(lphul->ihuldef)->imdAttack);
            if (FHullHasBombs(lphul)) {
                return 1;
            }
        }
    }
    return 0;
}

int16_t FHullHasBombs(HUL *lphul) {
    HS     *lphs;
    int16_t ihs;

    lphs = lphul->rghs;
    ihs = 0;
    while (ihs < lphul->chs) {
        if (lphs->grhst == hstBomb && lphs->cItem != 0) {
            return 1;
        }
        if (lphs->grhst == hstBeam && lphs->iItem == ibeamMultiContainedMunition && lphs->cItem > 0) {
            return 1;
        }
        if (lphs->grhst == hstSpecialM && lphs->iItem == ispecialMOrbitalConstructionModule && lphs->cItem > 0) {
            return 1;
        }
        ihs++;
        lphs++;
    }
    return 0;
}

int16_t FFleetHasTeeth(APPSESSION &app, FLEET *lpfl) {
    int16_t ishdef;

    for (ishdef = 0; ishdef < 16; ishdef++) {
        if (lpfl->rgcsh[ishdef] != 0 && FHullHasTeeth(&app.state.rglpshdef[lpfl->iplr()][ishdef].hul) && app.state.rglpshdef[lpfl->iplr()][ishdef].det == detAll) {
            return 1;
        }
    }
    return 0;
}

int16_t FHullHasTeeth(HUL *lphul) {
    HS     *lphs;
    int16_t ihs;

    lphs = lphul->rghs;
    ihs = 0;
    while (ihs < lphul->chs) {
        if ((lphs->grhst & hstWeapon) && lphs->cItem > 0) {
            return 1;
        }
        ihs++;
        lphs++;
    }
    return 0;
}

int16_t FFuelTanker(SHDEF *lpshdef) {
    if (lpshdef->hul.ihuldef == ihuldefFuelTransport || lpshdef->hul.ihuldef == ihuldefSuperFuelXport) {
        return 1;
    }
    return 0;
}

void CheckTarget(APPSESSION &app, TOK *ptok, FLEET *lpfl, int16_t ishdef) {
    int16_t  iplr;
    BTLPLAN *lpbtlplan;
    int16_t  ibp;
    SHDEF   *lpshdef;

    iplr = lpfl->iplr();
    lpshdef = app.state.rglpshdef[iplr] + ishdef;
    if (FHullHasTeeth(&lpshdef->hul)) {
        ptok->mdTarget0 = mdTargetArmedShips;
    } else if (FHullHasBombs(&lpshdef->hul)) {
        ptok->mdTarget0 = mdTargetBombersFreighters;
    } else if (FFuelTanker(lpshdef)) {
        ptok->mdTarget0 = mdTargetFuelTransports;
    } else if (WtMaxShdefStat(lpshdef, 2) != 0) {
        ptok->mdTarget0 = mdTargetFreighters;
    } else {
        ptok->mdTarget0 = mdTargetUnarmedShips;
    }
    ibp = lpfl->iplan;
    lpbtlplan = app.state.rglpbtlplan[iplr] + ibp;
    ptok->mdTarget1 = lpbtlplan->mdTarget1;
    ptok->mdTarget2 = lpbtlplan->mdTarget2;
    if (ptok->mdTarget0 == mdTargetArmedShips) {
        ptok->mdTactic = lpbtlplan->mdTactic;
    } else {
        ptok->mdTactic = mdTacticDisengage;
    }
    if (ptok->mdTactic == mdTacticDisengage) {
        ptok->dzDis = 7;
    }
    return;
}

int16_t FDumpCargo(APPSESSION &app, FLEET *lpfl) {
    POINT16 pt;
    PLANET *lppl;
    int16_t i;

    for (i = 0; i <= 2 && lpfl->rgwtMin[i] == 0; i++) {
    }
    if (i > 2) {
        return 0;
    }
    if (!app.state.rglpbtlplan[lpfl->iplr()][lpfl->iplan].fDumpCargo) {
        return 0;
    }
    if (lpfl->idPlanet != idPlanetDeepSpace) {
        lppl = LpplFromId(app, lpfl->idPlanet);
        for (i = 0; i <= 2; i++) {
            lppl->rgwtMin[i] += lpfl->rgwtMin[i];
        }
    } else {
        pt = lpfl->pt;
        DropSalvage(app, &app.state.lpthBattle, lpfl->rgwtMin, lpfl->iplr(), &pt);
    }
    for (i = 0; i <= 2; i++) {
        lpfl->rgwtMin[i] = 0;
    }
    return 1;
}

void DropSalvage(APPSESSION &app, THING **plpth, int32_t *rgwtMinerals, int16_t iplr, POINT16 *ppt) {
    int32_t wtTotal;
    int32_t wt;
    int16_t i;
    THING  *lpth;

    lpth = *plpth;
    wtTotal = 0;
    for (i = 0; i < app.state.game.cPlanMax; i++) {
        if (ppt->x == app.state.rgptPlan[i].x && ppt->y == app.state.rgptPlan[i].y) {
            return;
        }
    }
    for (i = 0; i < 3; i++) {
        wtTotal += rgwtMinerals[i];
    }
    while (wtTotal == 0) {
        for (i = 0; i < 3; i++) {
            rgwtMinerals[i] = RandomIO().pfnRandom(app, 10);
            wtTotal += rgwtMinerals[i];
        }
    }
    if (!lpth) {
        lpth = LpthNew(app, iplr, ithMineralPacket);
        if (!lpth) {
            return;
        }
        lpth->thp().SetIWarp(0);
        lpth->pt().SetX(ppt->x);
        lpth->pt().SetY(ppt->y);
        lpth->thp().SetIdPlanet(0x3ff);
    } else {
        for (i = 0; i < 3; i++) {
            rgwtMinerals[i] += lpth->thp().rgwtMin(i);
            wtTotal += lpth->thp().rgwtMin(i);
            lpth->thp().SetRgwtMin(i, 0);
        }
        lpth->thp().SetWtMax(0);
    }
    lpth->thp().SetFMoved(1);
    while (wtTotal > 0) {
        for (i = 0; i < 3; i++) {
            if ((uint32_t)(lpth->thp().wtMax() * 10) + rgwtMinerals[i] > 30000) {
                wt = 30000 - (uint32_t)(lpth->thp().wtMax() * 10);
                wtTotal -= wt;
                lpth->thp().SetWtMax(3000);
                { const auto rgwtMinForThing = +(LOWORD(wt)); auto &thForThing = *(lpth); thForThing.thp().SetRgwtMin(i, thForThing.thp().rgwtMin(i) + rgwtMinForThing); }
                rgwtMinerals[i] -= wt;
                lpth = LpthNew(app, iplr, ithMineralPacket);
                if (!lpth) {
                    return;
                }
                lpth->thp().SetIWarp(0);
                lpth->thp().SetIdPlanet(0x3ff);
                lpth->pt().SetX(ppt->x);
                lpth->pt().SetY(ppt->y);
            } else {
                { const auto wtMaxForThing = +((rgwtMinerals[i] + 9) / 10); auto &thForThing = *(lpth); thForThing.thp().SetWtMax(thForThing.thp().wtMax() + wtMaxForThing); }
                { const auto rgwtMinForThing = +(LOWORD(rgwtMinerals[i])); auto &thForThing = *(lpth); thForThing.thp().SetRgwtMin(i, thForThing.thp().rgwtMin(i) + rgwtMinForThing); }
                wtTotal -= rgwtMinerals[i];
                rgwtMinerals[i] = 0;
            }
            if (wtTotal <= 0)
                break;
        }
    }
    *plpth = lpth;
    return;
}

int16_t CplrBattle(APPSESSION &app, FLEET *lpfl, uint16_t *rggrfAttack, uint16_t *pgrfPlayer, uint16_t *pgrfSpectator) {
    int16_t   iplrStarbase;
    FLEET    *lpflCur;
    int32_t   rgcsh[16];
    uint16_t  grPlr;
    int16_t   iplrCur;
    PLANET   *lppl;
    int16_t   cplr;
    int16_t   i;
    int16_t   mdRel;
    uint8_t   rgctok[16];
    int16_t   fChange;
    AttackWho iplrAttack;
    int16_t   fAttack;
    int16_t   cshdef;
    int16_t   ishdef;
    int16_t   cflTotal;
    uint16_t  grfPlayer;
    int16_t   ctokNew;
    int16_t   ctokFleet;

    fAttack = 0;
    iplrStarbase = iplrNone;
    grfPlayer = 0;
    cshdef = 0;
    app.state.grfMissed = 0;
    cflTotal = 0;
    *pgrfSpectator = 0;
    memset(rggrfAttack, 0, 32);
    memset(rgcsh, 0, 64);
    if (lpfl->idPlanet != idPlanetDeepSpace) {
        lppl = LpplFromId(app, lpfl->idPlanet);
        if (lppl->fStarbase) {
            iplrStarbase = lppl->iPlayer;
            grfPlayer |= 1 << iplrStarbase;
            iplrAttack = static_cast<AttackWho>(app.state.rglpbtlplan[iplrStarbase]->iplrAttack);
            if (FHullHasTeeth(&app.state.rglpshdefSB[iplrStarbase][lppl->isb].hul) && iplrAttack != iplrAttackNobody) {
                switch (iplrAttack) {
                case iplrAttackEveryone:
                    /* NATIVE: original indexed with iplrCur before it was set; see WIN16-PARITY.md */
                    rggrfAttack[iplrStarbase] = ~(1 << iplrStarbase) & 0xffff;
                    break;
                case iplrAttackEnemies:
                case iplrAttackNeutralsEnemies:
                    for (i = 0; i < app.state.game.cPlayer; i++) {
                        if (i != iplrStarbase) {
                            mdRel = app.state.rgplr[iplrStarbase].rgmdRelation[i];
                            if (mdRel == 2 || (mdRel == 0 && iplrAttack == iplrAttackNeutralsEnemies)) {
                                rggrfAttack[iplrStarbase] |= 1 << i;
                            }
                        }
                    }
                    break;
                default:
                    /* NATIVE: original indexed with iplrCur before it was set; see WIN16-PARITY.md */
                    rggrfAttack[iplrStarbase] |= 1 << (iplrAttack - 4);
                    break;
                }
            }
        } else if (lppl->iPlayer != iplrNone) {
            *pgrfSpectator |= 1 << lppl->iPlayer;
        }
    }
    lpflCur = lpfl;
    do {
        if (lpflCur->fDead)
            goto LNextFleet;
        iplrCur = lpflCur->iPlayer;
        grfPlayer |= 1 << iplrCur;
        if (app.state.rglpbtlplan[lpflCur->iplr()][lpflCur->iplan].mdTarget1 != mdTargetNone && app.state.rglpbtlplan[lpflCur->iplr()][lpflCur->iplan].iplrAttack != iplrAttackNobody &&
            FFleetHasTeeth(app, lpflCur)) {
            fAttack = 1;
            iplrAttack = static_cast<AttackWho>(app.state.rglpbtlplan[iplrCur][lpflCur->iplan].iplrAttack);
            if (app.state.rglpbtlplan[lpflCur->iplr()][lpflCur->iplan].mdTarget1 == mdTargetNone) {
                iplrAttack = iplrAttackNobody;
            }
            switch (iplrAttack) {
            case iplrAttackNobody:
                break;
            case iplrAttackEveryone:
                rggrfAttack[iplrCur] = ~(1 << iplrCur) & 0xffff;
                break;
            case iplrAttackEnemies:
            case iplrAttackNeutralsEnemies:
                for (i = 0; i < app.state.game.cPlayer; i++) {
                    if (i != iplrCur) {
                        mdRel = app.state.rgplr[iplrCur].rgmdRelation[i];
                        if (mdRel == 2 || (mdRel == 0 && iplrAttack == iplrAttackNeutralsEnemies)) {
                            rggrfAttack[iplrCur] |= 1 << i;
                        }
                    }
                }
                break;
            default:
                rggrfAttack[iplrCur] |= 1 << (iplrAttack - 4);
                break;
            }
        }
        lpflCur->fDone = 1;
        lpflCur->fInclude = 1;
    LNextFleet:
        lpflCur = lpflCur->lpflNext;
    } while (lpflCur != lpfl);
    if (!fAttack) {
        return 0;
    }
    iplrAttack = iplrAttackNobody;
    for (i = 0; i < app.state.game.cPlayer; i++) {
        if (rggrfAttack[i] != 0) {
            iplrAttack = static_cast<AttackWho>(iplrAttack | grfPlayer & rggrfAttack[i]);
        }
    }
    if (iplrAttack == iplrAttackNobody) {
        return 0;
    }
    for (i = 0; i < app.state.game.cPlayer; i++) {
        if (rggrfAttack[i] & iplrAttack) {
            iplrAttack = static_cast<AttackWho>(iplrAttack | 1 << i);
        }
        if (1 << i & iplrAttack) {
            for (iplrCur = 0; iplrCur < app.state.game.cPlayer; iplrCur++) {
                if (1 << i & rggrfAttack[iplrCur]) {
                    rggrfAttack[i] |= 1 << iplrCur;
                }
            }
        }
    }
    do {
        if (lpflCur == lpfl) {
            fChange = 0;
        }
        iplrCur = lpflCur->iPlayer;
        grPlr = 1 << iplrCur;
        if ((grfPlayer & grPlr) && !(iplrAttack & grPlr)) {
            rggrfAttack[iplrCur] = 0;
            for (i = 0; i < app.state.game.cPlayer; i++) {
                if (i != iplrCur && app.state.rgplr[iplrCur].rgmdRelation[i] == 1 && (1 << i & iplrAttack)) {
                    if (1 << i & rggrfAttack[iplrCur]) {
                        rggrfAttack[iplrCur] = 0;
                        break;
                    }
                    rggrfAttack[iplrCur] |= rggrfAttack[i];
                }
            }
            if (rggrfAttack[iplrCur] != 0) {
                iplrAttack = static_cast<AttackWho>(iplrAttack | grPlr);
            } else {
                grfPlayer &= ~grPlr;
            }
            fChange = 1;
        }
        lpflCur = lpflCur->lpflNext;
    } while (fChange || lpflCur != lpfl);
    if (iplrStarbase != iplrNone && (1 << iplrStarbase & grfPlayer)) {
        rgcsh[iplrStarbase] = 1;
        cshdef = 1;
    }
    fChange = 0;
    do {
        iplrCur = lpflCur->iPlayer;
        grPlr = 1 << iplrCur;
        if (grfPlayer & grPlr) {
            for (ishdef = 0; ishdef < 16; ishdef++) {
                if (lpflCur->rgcsh[ishdef] != 0) {
                    if (PartQueryIO().pfnHull(app.state.rglpshdef[iplrCur][ishdef].hul.ihuldef)->imdAttack != hullAttackNone) {
                        rgcsh[iplrCur] += lpflCur->rgcsh[ishdef];
                    }
                    cshdef++;
                }
            }
        } else {
            *pgrfSpectator |= grPlr;
            lpflCur->fInclude = 0;
        }
        lpflCur = lpflCur->lpflNext;
    } while (lpflCur != lpfl);
    cplr = 0;
    for (; iplrAttack != iplrAttackNobody; iplrAttack = static_cast<AttackWho>(iplrAttack >> 1)) {
        if (iplrAttack & 1) {
            cplr++;
        }
    }
    if (cshdef > 255) {
        ctokNew = 0;
        i = 255 / cplr;
        if (iplrStarbase != iplrNone && (1 << iplrStarbase & grfPlayer)) {
            ctokNew++;
        }
        memset(rgctok, 0, 16);
        do {
            if (lpflCur->fInclude) {
                ctokFleet = 0;
                iplrCur = lpflCur->iPlayer;
                for (ishdef = 0; ishdef < 16; ishdef++) {
                    if (lpflCur->rgcsh[ishdef] != 0) {
                        ctokFleet++;
                    }
                }
                if (rgctok[iplrCur] + ctokFleet > i) {
                    lpflCur->fInclude = 0;
                    lpflCur->fBombed = 1;
                    lpflCur->SetFSkipped(1);
                } else {
                    rgctok[iplrCur] += ctokFleet;
                    ctokNew += ctokFleet;
                }
            }
            lpflCur = lpflCur->lpflNext;
        } while (lpflCur != lpfl);
        if (ctokNew < 255) {
            do {
                if (lpflCur->fSkipped()) {
                    ctokFleet = 0;
                    iplrCur = lpflCur->iPlayer;
                    for (ishdef = 0; ishdef < 16; ishdef++) {
                        if (lpflCur->rgcsh[ishdef] != 0) {
                            ctokFleet++;
                        }
                    }
                    if (ctokNew + ctokFleet <= 255) {
                        lpflCur->fInclude = 1;
                        lpflCur->fBombed = 0;
                        lpflCur->SetFSkipped(0);
                        rgctok[iplrCur] += ctokFleet;
                        ctokNew += ctokFleet;
                    }
                }
                lpflCur = lpflCur->lpflNext;
            } while (lpflCur != lpfl);
        }
    }
    *pgrfPlayer = grfPlayer;
    if (fChange) {
        return -1;
    }
    return cplr;
}

int16_t SpdOfShip(APPSESSION &app, FLEET *lpfl, int16_t ishdef, TOK *ptok, int16_t fDumpCargo, SHDEF *lpshdef) {
    int32_t  wtCargoFleetMax;
    int16_t  spd;
    int16_t  iWarp;
    uint16_t wt;
    int16_t  cHalfThruster;
    int16_t  cThruster;
    int32_t  wtFleetCargo;
    int16_t  j;
    int16_t  cEngineT;
    uint16_t wtCargoShdefMax;
    iengine  iEngine;
    ENGINE  *lpengine;

    if (!lpshdef) {
        lpshdef = app.state.rglpshdef[lpfl->iPlayer] + ishdef;
    }
    iEngine = static_cast<iengine>(iengineNone);
    cHalfThruster = 0;
    cThruster = 0;
    for (j = 0; j < lpshdef->hul.chs; j++) {
        if (lpshdef->hul.rghs[j].cItem != 0) {
            switch (lpshdef->hul.rghs[j].grhst) {
            default:
                break;
            case hstEngine:
                iEngine = static_cast<iengine>(lpshdef->hul.rghs[j].iItem);
                cEngineT = lpshdef->hul.rghs[j].cItem;
                if (iEngine != iengineEnigmaPulsar)
                    break;
                cHalfThruster += lpshdef->hul.rghs[j].cItem;
                break;
            case hstSpecialM:
                if (lpshdef->hul.rghs[j].iItem != ispecialMManeuveringJet) {
                    if (lpshdef->hul.rghs[j].iItem != ispecialMOverthruster)
                        break;
                    cThruster += lpshdef->hul.rghs[j].cItem * 2;
                    break;
                }
                cThruster += lpshdef->hul.rghs[j].cItem;
                break;
            case hstSpecialE:
                if (lpshdef->hul.rghs[j].iItem != ispecialEMultiFunctionPod)
                    break;
                cThruster += lpshdef->hul.rghs[j].cItem;
                break;
            case hstMining:
                if (lpshdef->hul.rghs[j].iItem == iminingAlienMiner) {
                    cHalfThruster += lpshdef->hul.rghs[j].cItem;
                }
            }
        }
    }
    cThruster += (int16_t)(cHalfThruster + 1) / 2;
    if (iEngine == static_cast<iengine>(iengineNone) || cEngineT == 0) {
        return 0;
    }
    lpengine = LpengineFromId(iEngine);
    switch (iEngine) {
    case iengineInterspace10:
    case iengineEnigmaPulsar:
    case iengineTransStar10:
    case iengineTransGalacticMizerScoop:
    case iengineGalaxyScoop:
        iWarp = 10;
        break;
    default:
        for (iWarp = 9; iWarp > 0 && lpengine->rgcFuelUsed[iWarp] > 120; iWarp--) {
        }
    }
    spd = iWarp - 4 + cThruster;
    if (lpfl) {
        spd += (RaceQueryIO().pfnStat(&app.state.rgplr[lpfl->iPlayer], rsMajorAdv) == raAttack) * 2;
    }
    wt = lpshdef->hul.wtEmpty;
    if (lpfl) {
        wtCargoShdefMax = WtMaxShdefStat(lpshdef, 2);
        if (wtCargoShdefMax != 0) {
            wtCargoFleetMax = ProductionIO().pfnFleetStat(app, lpfl, 2);
            wtFleetCargo = lpfl->rgwtMin[0] + lpfl->rgwtMin[1] + lpfl->rgwtMin[2] + lpfl->rgwtMin[3];
            wtFleetCargo = (int32_t)((int32_t)(wtFleetCargo * (uint32_t)wtCargoShdefMax) / wtCargoFleetMax);
            wt += LOWORD(wtFleetCargo);
        } else {
            fDumpCargo = 0;
        }
        if (fDumpCargo) {
            spd--;
        }
        ptok->dwt = RandomIO().pfnRandom(app, 15);
    }
    if (ptok) {
        ptok->wt = wt;
    }
    spd -= (uint32_t)((uint32_t)wt / 70) / lpshdef->hul.rghs[0].cItem;
    if (0 > (8 >= spd ? spd : 8)) {
        spd = 0;
    } else if (8 < spd) {
        spd = 8;
    }
    return spd;
}

SHDEF *LpshdefFromTok(APPSESSION &app, TOK *ptok) {
    if (ptok->ishdef >= 16) {
        return app.state.rglpshdefSB[ptok->iplr] + (ptok->ishdef - 16);
    }
    return app.state.rglpshdef[ptok->iplr] + ptok->ishdef;
}

int16_t FCanKillTok(APPSESSION &app, TOK *ptok1, TOK *ptok2) {
    int32_t lp1;
    int32_t lp2;

    lp1 = LpshdefFromTok(app, ptok1)->lPower;
    lp2 = LpshdefFromTok(app, ptok2)->lPower;
    if (lp2 > lp1) {
        return 0;
    }
    if ((lp2 & 0x7ffff000) < (lp1 & 0x7ffff000)) {
        return 1;
    }
    if ((lp2 & 0x7fffff00) == (lp1 & 0x7fffff00) && ptok1->spd >= ptok2->spd) {
        return 1;
    }
    return 0;
}

void DoBattles(APPSESSION &app, int16_t fPostMovement) {
    int16_t  cplr;
    int16_t  ifl;
    FLEET   *lpfl;
    uint16_t grfSpectator;
    uint16_t grfPlayer;
    uint16_t rggrfAttack[16];

    LinkFleets(app, fPostMovement);
    static_assert(std::is_trivially_default_constructible_v<TOK> && std::is_trivially_destructible_v<TOK>);
    app.state.vrgtok = LpAlloc<TOK>(app, 256 * sizeof(TOK), htMisc);
    app.state.vlpwtCargo = LpAlloc<uint16_t>(app, 0x200, htMisc);
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        lpfl->fBombed = 0;
        if (!lpfl->fDone && !lpfl->fDead && lpfl->lpflNext) {
            cplr = CplrBattle(app, lpfl, rggrfAttack, &grfPlayer, &grfSpectator);
            if (cplr != -1 && cplr != 0 && FDoCoolBattle(app, lpfl, cplr, rggrfAttack, grfPlayer, grfSpectator)) {
            }
        }
    }
    HeapIO().pfnFree(app, app.state.vlpwtCargo, htMisc);
    HeapIO().pfnFree(app, app.state.vrgtok, htMisc);
    app.state.vlpwtCargo = nullptr;
    app.state.vrgtok = nullptr;
    if (app.state.lpbBattleT) {
        RawStore16(app.state.lpbBattleT, 0xffff);
        HeapIO().pfnFree(app, app.state.lpbBattleT, htBattle);
        app.state.lpbBattleT = nullptr;
    }
    if (app.state.lpbBattleCur) {
        RawStore16(app.state.lpbBattleCur, 0xffff);
    }
    DoBombing(app);
    return;
}

void RegenShield(APPSESSION &app, TOK *ptok) {
    int32_t dpNew;
    int32_t dpOrig;

    dpOrig = DpShieldOfShdef(app, LpshdefFromTok(app, ptok), ptok->iplr);
    if (ptok->dpShield != 0) {
        dpNew = (uint32_t)ptok->dpShield + (int32_t)(dpOrig / 10);
        if (dpNew > dpOrig) {
            dpNew = dpOrig;
        }
        ptok->dpShield = LOWORD(dpNew);
    }
    return;
}

int16_t InitFromHuldef(APPSESSION &app, HUL *lphul, int16_t *ppctBC) {
    int16_t ihs;
    int16_t i;
    int16_t pct;
    int16_t initBase;
    int16_t cbc;
    int16_t pctBC;
    PART    part;

    pct = 0;
    cbc = 0;
    initBase = PartQueryIO().pfnHull(lphul->ihuldef)->init;
    for (ihs = 0; ihs < lphul->chs; ihs++) {
        part.hs = lphul->rghs[ihs];
        if (part.hs.cItem != 0) {
            if (!(part.hs.grhst & hstSpecialE)) {
                if ((part.hs.grhst & hstBeam) && part.hs.iItem == ibeamMultiContainedMunition) {
                    pctBC = 10;
                    for (i = 0; i < (int16_t)part.hs.cItem; i++) {
                        pct += (int16_t)((100 - pct) * pctBC) / 100;
                    }
                }
            } else {
                switch (part.hs.iItem) {
                default:
                    break;
                case ispecialEBattleComputer:
                case ispecialEBattleSuperComputer:
                case ispecialEBattleNexus:
                    PartQueryIO().pfnLookup(app, &part);
                    cbc += (part.hs.iItem - 4) * part.hs.cItem;
                    pctBC = part.pspecial()->grAbility;
                    for (i = 0; i < (int16_t)part.hs.cItem; i++) {
                        pct += (int16_t)((100 - pct) * pctBC) / 100;
                    }
                }
            }
        }
    }
    if (ppctBC) {
        *ppctBC = pct;
    }
    initBase += cbc;
    if (initBase >= 64) {
        initBase = 63;
    }
    return initBase;
}

void CheckInitiative(APPSESSION &app, TOK *ptok) {
    SHDEF  *lpshdef;
    int16_t pctBC;

    lpshdef = LpshdefFromTok(app, ptok);
    app.state.idPlayer = ptok->iplr;
    ptok->initBase = InitFromHuldef(app, &lpshdef->hul, &pctBC);
    app.state.idPlayer = iplrNone;
    ptok->pctBC = pctBC;
    return;
}

void CheckWeapons(APPSESSION &app, TOK *ptok, int16_t *pfDampeningField, uint8_t *pinit) {
    int16_t pctJam;
    int32_t ldp;
    int32_t pctBeamDef;
    int16_t ihs;
    int16_t initMac;
    int16_t init;
    int16_t dxyMax;
    int16_t i;
    int32_t pctCap;
    int16_t initMin;
    int32_t pctHit;
    SHDEF  *lpshdef;
    int16_t dxyLim;
    int16_t initBase;
    HUL    *lphul;
    int16_t dxyPart;
    PART    part;

    pctCap = 1000;
    pctBeamDef = 1000;
    pctHit = 10000;
    initBase = ptok->initBase;
    initMin = -1;
    initMac = -1;
    lpshdef = LpshdefFromTok(app, ptok);
    lphul = &lpshdef->hul;
    dxyMax = -1;
    dxyLim = -1;
    ldp = DpShieldOfShdef(app, lpshdef, ptok->iplr);
    for (ihs = 0; ihs < lphul->chs; ihs++) {
        if ((lphul->rghs[ihs].grhst & (hstScanner | hstShield | hstArmor | hstBeam | hstTorp | hstMining | hstSpecialE | hstSpecialM)) &&
            lphul->rghs[ihs].cItem != 0) {
            pctJam = 100;
            dxyPart = -1;
            part.hs = lphul->rghs[ihs];
            if (part.hs.grhst & hstWeapon) {
                app.state.idPlayer = ptok->iplr;
                PartQueryIO().pfnLookup(app, &part);
                app.state.idPlayer = iplrNone;
                init = initBase + part.pbeam()->init;
                if (init >= 64) {
                    init = 63;
                }
            } else {
                init = -1;
            }
            switch (part.hs.grhst) {
            default:
                break;
            case hstSpecialE:
                switch (part.hs.iItem) {
                case ispecialEJammer10:
                case ispecialEJammer20:
                case ispecialEJammer30:
                case ispecialEJammer50:
                    app.state.idPlayer = ptok->iplr;
                    PartQueryIO().pfnLookup(app, &part);
                    app.state.idPlayer = iplrNone;
                    pctJam = 100 - part.pspecial()->grAbility;
                    break;
                case ispecialEMultiFunctionPod:
                    pctJam = 90;
                    break;
                case ispecialEEnergyDampener:
                    *pfDampeningField = 1;
                    break;
                case ispecialETachyonDetector:
                    ptok->fDetector = 1;
                    break;
                case ispecialEEnergyCapacitor:
                case ispecialEFluxCapacitor:
                    app.state.idPlayer = ptok->iplr;
                    PartQueryIO().pfnLookup(app, &part);
                    app.state.idPlayer = iplrNone;
                    for (i = part.hs.cItem; i > 0; i--) {
                        pctCap = (int32_t)(pctCap * (part.pspecial()->grAbility + 100)) / 100;
                    }
                }
                break;
            case hstSpecialM:
                if (part.hs.iItem != ispecialMBeamDeflector)
                    break;
                app.state.idPlayer = ptok->iplr;
                PartQueryIO().pfnLookup(app, &part);
                app.state.idPlayer = iplrNone;
                for (i = part.hs.cItem; i > 0; i--) {
                    pctBeamDef = (int32_t)(pctBeamDef * (100 - part.pspecial()->grAbility)) / 100;
                }
                break;
            case hstMining:
                if (part.hs.iItem != iminingAlienMiner)
                    break;
                pctJam = 70;
                break;
            case hstArmor:
                if (part.hs.iItem != iarmorMegaPolyShell)
                    break;
                pctJam = 80;
                break;
            case hstShield:
                if (part.hs.iItem != ishieldLangstonShell)
                    break;
                pctJam = 95;
                break;
            case hstBeam:
                dxyPart = part.pbeam()->dRangeMax;
                break;
            case hstTorp:
                ptok->fTorp = 1;
                dxyPart = part.ptorp()->dRangeMax;
            }
            if (pctJam < 100) {
                for (i = part.hs.cItem; i > 0; i--) {
                    pctHit = (uint32_t)(pctHit * pctJam);
                    pctHit = (int32_t)(pctHit / 100);
                }
            }
            if (dxyPart != -1) {
                if (ptok->grobj == grobjPlanet) {
                    dxyPart++;
                }
                if (dxyMax < 0 || dxyMax > dxyPart) {
                    dxyMax = dxyPart;
                }
                if (dxyPart > dxyLim) {
                    dxyLim = dxyPart;
                }
                pinit[init] = 1;
                if (initMin == -1 || init < initMin) {
                    initMin = init;
                }
                if (init > initMac) {
                    initMac = init;
                }
            }
        }
    }
    if (pctHit != 10000) {
        ptok->pctJam = 100 - (pctHit + 50) / 100;
        if (ptok->pctJam > 95) {
            ptok->pctJam = 95;
        }
    } else {
        ptok->pctJam = 0;
    }
    if (ptok->grobj == grobjPlanet) {
        ptok->pctJam -= (int16_t)ptok->pctJam / 4;
    }
    if (pctCap != 1000) {
        if (pctCap > 2550) {
            pctCap = 2550;
        }
        ptok->pctCap = pctCap / 10;
    }
    ptok->pctBeamDef = pctBeamDef / 10;
    ptok->dxyMax = dxyMax;
    ptok->dxyLim = dxyLim;
    ptok->initMin = initMin;
    ptok->initMac = initMac;
    if (ldp & 0xffff0000) {
        ptok->dpShield = 0xffff;
    } else {
        ptok->dpShield = LOWORD(ldp);
    }
    return;
}

void RandomizeTokOrder(APPSESSION &app) {
    TOK     tok;
    int16_t itokSwap;
    int16_t itok;

    itokSwap = -1;
    for (itok = 0; itok < app.state.vctok; itok++) {
        itokSwap = RandomIO().pfnRandom(app, app.state.vctok - itok) + itok;
        if (itokSwap != itok) {
            tok = app.state.vrgtok[itokSwap];
            app.state.vrgtok[itokSwap] = app.state.vrgtok[itok];
            app.state.vrgtok[itok] = tok;
        }
    }
    return;
}

void InitializeBoard(APPSESSION &app, FLEET *lpfl, int16_t ibrc, uint16_t grfPlayer, uint8_t *pinit, int16_t *pinitMin, int16_t *pinitMac) {
    int16_t   iplr;
    FLEET    *lpflCur;
    TOK      *ptok;
    int16_t   initMac;
    PLANET   *lppl;
    int16_t   fDampeningField;
    int16_t   initMin;
    uint16_t *lpwtCargoCur;
    TOK      *ptokT;
    uint8_t   mpiplrdibrc[16];
    int16_t   fDumpCargo;
    int16_t   ishdef;
    uint8_t   rgfTorp[16];

    initMin = -1;
    initMac = -1;
    fDampeningField = 0;
    lpwtCargoCur = app.state.vlpwtCargo;
    ishdef = 0;
    memset(mpiplrdibrc, 255, 16);
    for (iplr = 0; iplr < app.state.game.cPlayer; iplr++) {
        if (1 << iplr & grfPlayer) {
            mpiplrdibrc[iplr] = ishdef++;
        }
    }
    memset(rgfTorp, 0, 16);
    lpflCur = lpfl;
    ptok = app.state.vrgtok;
    if (lpfl->idPlanet != idPlanetDeepSpace) {
        lppl = LpplFromId(app, lpfl->idPlanet);
        iplr = lppl->iPlayer;
        if (iplr != iplrNone && lppl->fStarbase && (1 << iplr & grfPlayer)) {
            ptok->grobj = grobjPlanet;
            lppl->fNoHeal = 1;
            ptok->brc = rgbrcStart[mpiplrdibrc[iplr] + ibrc];
            ptok->id = lppl->id;
            ptok->iplr = iplr;
            ptok->csh = 1;
            ptok->ishdef = lppl->isb + 16;
            CheckInitiative(app, ptok);
            CheckWeapons(app, ptok, &fDampeningField, pinit);
            rgfTorp[iplr] |= ptok->fTorp;
            if (ptok->initBase == 0xff) {
                ptok->mdTarget0 = mdTargetUnarmedShips;
            } else {
                ptok->mdTarget0 = mdTargetArmedShips;
            }
            ptok->mdTarget1 = mdTargetAny;
            ptok->mdTarget2 = mdTargetAny;
            ptok->dv.SetPctDp(lppl->pctDp);
            ptok->mdTactic = mdTacticMaxDamage;
            if (ptok->dv.pctDp() != 0) {
                ptok->dv.SetPctSh(100);
            }
            ptok->spd = 0;
            ptok->wt = 0xffff;
            ptok++;
        }
    }
    do {
        if (!lpflCur->fDead) {
            if (lpflCur->fSkipped()) {
                app.state.grfMissed |= 1 << lpflCur->iPlayer;
            } else if (lpflCur->fInclude) {
                lpflCur->fNoHeal = 1;
                iplr = lpflCur->iPlayer;
                fDumpCargo = FDumpCargo(app, lpflCur);
                for (ishdef = 0; ishdef < 16; ishdef++) {
                    if (lpflCur->rgcsh[ishdef] != 0) {
                        ptok->grobj = grobjFleet;
                        ptok->brc = rgbrcStart[mpiplrdibrc[iplr] + ibrc];
                        ptok->id = lpflCur->id;
                        ptok->iplr = lpflCur->iplr();
                        ptok->ishdef = ishdef;
                        ptok->csh = lpflCur->rgcsh[ishdef];
                        ptok->dv.dp = lpflCur->rgdv[ishdef].dp;
                        CheckInitiative(app, ptok);
                        CheckWeapons(app, ptok, &fDampeningField, pinit);
                        rgfTorp[iplr] |= ptok->fTorp;
                        CheckTarget(app, ptok, lpflCur, ishdef);
                        ptok->spd = SpdOfShip(app, lpflCur, ishdef, ptok, fDumpCargo, nullptr);
                        ptok++;
                        if ((int16_t)(ptok - app.state.vrgtok) > 0xff)
                            goto LTooManyTokens;
                    }
                }
            }
        }
        lpflCur = lpflCur->lpflNext;
    } while (lpflCur != lpfl);
LTooManyTokens:
    app.state.vctok = (int16_t)(ptok - app.state.vrgtok);
    RandomizeTokOrder(app);
    for (ptokT = app.state.vrgtok; ptokT < ptok; ptokT++) {
        ptokT->fMoved = 1;
        ptokT->fActive = 1;
        if (ptokT->initMin == 0xff) {
            ptokT->mdTarget1 = mdTargetNone;
        }
        ptokT->fRegen = ptokT->dpShield != 0 && RaceQueryIO().pfnGrbit(&app.state.rgplr[ptokT->iplr], ibitRaceRegeneratingShields) != 0;
        if (fDampeningField && ptokT->grobj != grobjPlanet) {
            ptokT->spd = ptokT->spd - 4 <= 0 ? 0 : ptokT->spd - 4;
        }
        FEncodeTok(*ptokT, std::as_writable_bytes(std::span(app.state.lpbBattleCur, cbWireTok)));
        app.state.lpbBattleCur += cbWireTok;
        if (ptokT->initMin != 0xff && (initMin == -1 || ptokT->initMin < initMin)) {
            initMin = ptokT->initMin;
        }
        if (ptokT->initMac != 0xff && (initMac == -1 || ptokT->initMac > initMac)) {
            initMac = ptokT->initMac;
        }
    }
    *pinitMin = initMin & 0xff;
    *pinitMac = initMac & 0xff;
    return;
}

int16_t DzFromBrcBrc(uint8_t brc1, uint8_t brc2) {
    int16_t dy;
    int16_t dx;

    dx = (brc1 & 0xf) - (brc2 & 0xf);
    dx = abs(dx);
    dy = (brc1 >> 4) - (brc2 >> 4);
    dy = abs(dy);
    if (dx > dy) {
        return dx;
    }
    return dy;
}

int32_t DpFromPtokBrcToBrc(APPSESSION &app, TOK *ptok, uint8_t brcSrc, uint8_t brcTarget, TOK *ptokTarget, int16_t fProximity) {
    int16_t dz;
    int32_t dpMax;
    int32_t dpShdef;
    int16_t ihs;
    int32_t cTorpBase;
    int32_t dpTotal;
    int16_t fOutOfRange;
    int32_t dRange;
    HUL    *lphul;
    int32_t cTorpHit;
    int32_t dp;
    PART    part;
    int32_t dpShieldsLeft;

    dz = DzFromBrcBrc(brcSrc, brcTarget);
    dpTotal = 0;
    if (!fProximity && dz > (int16_t)ptok->dxyLim) {
        return 0;
    }
    lphul = &LpshdefFromTok(app, ptok)->hul;
    for (ihs = 0; ihs < lphul->chs; ihs++) {
        if ((lphul->rghs[ihs].grhst & hstWeapon) && lphul->rghs[ihs].cItem != 0) {
            part.hs = lphul->rghs[ihs];
            app.state.idPlayer = ptok->iplr;
            PartQueryIO().pfnLookup(app, &part);
            app.state.idPlayer = iplrNone;
            dRange = (int16_t)((ptok->grobj == grobjPlanet) + part.pbeam()->dRangeMax);
            fOutOfRange = dRange < dz;
            if (!fOutOfRange || fProximity) {
                dp = (uint32_t)(part.pbeam()->dp * part.hs.cItem);
                if (part.hs.grhst != hstBeam) {
                    if (part.hs.grhst == hstTorp) {
                        cTorpBase = (uint32_t)((uint32_t)(part.hs.cItem * (uint32_t)ptok->csh) * 200);
                        cTorpHit = CTorpHit(app, cTorpBase, ptokTarget, part.ptorp()->dHitChance, ptok->pctBC);
                        dp = (int32_t)(part.ptorp()->dp * cTorpHit) / 200;
                        if (ptokTarget->dpShield > 0) {
                            dp += (int32_t)((cTorpBase - cTorpHit) * part.ptorp()->dp) / 1600;
                        }
                        if (fOutOfRange) {
                            dp = (int32_t)(dp / ((int16_t)(dz + 10) - dRange));
                            if (dp < (int16_t)part.hs.cItem) {
                                dp = part.hs.cItem;
                            }
                        }
                        dpTotal += dp;
                    }
                } else {
                    if (ptok->pctCap != 0) {
                        dp = (int32_t)(dp * (int16_t)ptok->pctCap) / 100;
                    }
                    if (dz > 0 && dRange > 0) {
                        dp -= (int32_t)((int32_t)(dp * dz) / 10 / dRange);
                    }
                    if (ptokTarget->pctBeamDef < 100) {
                        dp = (int32_t)(dp * (int16_t)ptokTarget->pctBeamDef) / 100;
                    }
                    if (part.pbeam()->grfAbilities & beamSapper) {
                        dpShieldsLeft = (uint32_t)((uint32_t)ptokTarget->dpShield * (uint32_t)ptok->csh);
                        if (dp > dpShieldsLeft) {
                            dp = dpShieldsLeft;
                        }
                    }
                    if (fOutOfRange) {
                        dp = (int32_t)(dp / ((int16_t)(dz + 10) - dRange));
                        if (dp < (int16_t)part.hs.cItem) {
                            dp = part.hs.cItem;
                        }
                    }
                    dpTotal += (uint32_t)(dp * (uint32_t)ptok->csh);
                }
            }
        }
    }
    dpShdef = (uint32_t)LpshdefFromTok(app, ptokTarget)->hul.dp;
    dpMax = (uint32_t)(((uint32_t)ptokTarget->dpShield + dpShdef) * (uint32_t)ptokTarget->csh);
    if (ptokTarget->dv.dp != 0 && dpMax > 0) {
        dpMax -= (int32_t)((int32_t)((int32_t)(dpShdef * ptokTarget->dv.pctDp()) / 10 * ptokTarget->dv.pctSh()) / 10 * (uint32_t)ptokTarget->csh) / 500;
        if (dpMax <= 0) {
            dpMax = 1;
        }
    }
    if (dpTotal > dpMax && !fProximity) {
        dpTotal = dpMax;
    }
    return dpTotal;
}

int16_t DzMoveRangeToConsider(APPSESSION &app, TOK *ptok, uint16_t grfAttack, uint8_t *pbrc) {
    int16_t  dzNonSapper;
    uint8_t  dz;
    int16_t  iplr;
    MdTarget mdTarget;
    uint8_t  dzBest;
    int16_t  itokLook;
    int16_t  iplrTarget;
    TOK     *ptokTarget;
    int16_t  dzMax;
    uint8_t  brcCur;
    int16_t  ihs;
    SHDEF   *lpshdef;
    HUL     *lphul;
    PART     part;

    brcCur = ptok->brc;
    dzMax = ptok->dxyLim + ptok->dMovesLeft;
    mdTarget = static_cast<MdTarget>(!FDoesPrimaryTargetTypeExist(app, ptok, grfAttack) ? ptok->mdTarget2 : ptok->mdTarget1);
    iplr = ptok->iplr;
    dzBest = 10;
    *pbrc = 0xff;
    if (ptok->dxyLim == 3) {
        dzNonSapper = -1;
        lpshdef = LpshdefFromTok(app, ptok);
        lphul = &lpshdef->hul;
        for (ihs = 0; ihs < lphul->chs; ihs++) {
            if (lphul->rghs[ihs].grhst == hstBeam && lphul->rghs[ihs].cItem != 0) {
                part.hs = lphul->rghs[ihs];
                app.state.idPlayer = ptok->iplr;
                PartQueryIO().pfnLookup(app, &part);
                app.state.idPlayer = iplrNone;
                if (!(part.pbeam()->grfAbilities & beamSapper) && part.pbeam()->dRangeMax > dzNonSapper) {
                    dzNonSapper = part.pbeam()->dRangeMax;
                }
            }
        }
    } else {
        dzNonSapper = ptok->dxyLim;
    }
    if (ptok->dxyMax < ptok->dxyLim && (ptok->mdTactic == mdTacticMaxDamage || ptok->mdTactic == mdTacticMaxNetDamage)) {
        dzMax = ptok->dxyMax + ptok->dMovesLeft;
    }
    ptokTarget = app.state.vrgtok;
    itokLook = 0;
    while (itokLook < app.state.vctok) {
        iplrTarget = ptokTarget->iplr;
        if (iplrTarget != iplr && (1 << iplrTarget & grfAttack) && ptokTarget->fActive && FIsTargetOfMdTarget(ptokTarget, mdTarget)) {
            dz = DzFromBrcBrc(brcCur, ptokTarget->brc);
            if (ptokTarget->dMovesLeft >= ptok->dMovesLeft) {
                dz++;
            }
            if (dz <= dzMax && (ptokTarget->dpShield > 0 || dzNonSapper == ptok->dxyLim || dz <= (uint16_t)(dzNonSapper + ptok->dMovesLeft))) {
                *pbrc = 0xff;
                return ptok->dMovesLeft;
            }
            if ((int16_t)dz < dzBest && DpFromPtokBrcToBrc(app, ptok, 0, 0, ptokTarget, 0) > 0) {
                dzBest = dz;
                *pbrc = ptokTarget->brc;
            }
        }
        itokLook++;
        ptokTarget++;
    }
    return 1;
}

int16_t FDoesPrimaryTargetTypeExist(APPSESSION &app, TOK *ptok, uint16_t grfAttack) {
    MdTarget mdTarget;
    int16_t  iplr;
    int16_t  iplrLook;
    TOK      tok;
    int16_t  itokLook;

    iplr = ptok->iplr;
    mdTarget = static_cast<MdTarget>(ptok->mdTarget1);
    if (mdTarget == mdTargetNone) {
        return 0;
    }
    for (itokLook = 0; itokLook < app.state.vctok; itokLook++) {
        iplrLook = app.state.vrgtok[itokLook].iplr;
        if (iplrLook != iplr && (1 << iplrLook & grfAttack)) {
            tok = app.state.vrgtok[itokLook];
            if (tok.fActive) {
                switch (mdTarget) {
                default:
                    break;
                case mdTargetArmedShips:
                case mdTargetBombersFreighters:
                case mdTargetUnarmedShips:
                case mdTargetFuelTransports:
                case mdTargetFreighters:
                    switch (mdTarget) {
                    default:
                        break;
                    case mdTargetArmedShips:
                    case mdTargetFuelTransports:
                    case mdTargetFreighters:
                        if (tok.mdTarget0 != mdTarget)
                            continue;
                        break;
                    case mdTargetUnarmedShips:
                        if (tok.mdTarget0 < mdTarget)
                            continue;
                        break;
                    case mdTargetBombersFreighters:
                        if (tok.mdTarget0 != mdTargetBombersFreighters && tok.mdTarget0 != mdTargetFreighters)
                            continue;
                    }
                    /* fallthrough */
                case mdTargetAny:
                    return 1;
                case mdTargetStarbase:
                    if (tok.grobj == grobjPlanet) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

int16_t FIsTargetOfMdTarget(TOK *ptok, MdTarget mdTarget) {
    if (mdTarget <= mdTargetFreighters) {
        switch (mdTarget) {
        case mdTargetNone:
            break;
        case mdTargetAny:
            return 1;
        case mdTargetStarbase:
            if (ptok->grobj == grobjPlanet) {
                return 1;
            }
            return 0;
        case mdTargetBombersFreighters:
            if (ptok->mdTarget0 == mdTargetBombersFreighters || ptok->mdTarget0 == mdTargetFreighters) {
                return 1;
            }
            return 0;
        case mdTargetArmedShips:
        case mdTargetFuelTransports:
        case mdTargetFreighters:
            if (ptok->mdTarget0 == mdTarget) {
                return 1;
            }
            return 0;
        case mdTargetUnarmedShips:
            switch (ptok->mdTarget0) {
            case mdTargetUnarmedShips:
            case mdTargetFreighters:
            case mdTargetFuelTransports:
                return 1;
            default:
                return 0;
            }
        }
    }
    return 0;
}

int32_t ScoreGuessBattleDamage(APPSESSION &app, TOK *ptokSrc, uint8_t brc, int16_t fPrimary, uint16_t grfAttack) {
    int16_t iBest;
    int16_t dMoves;
    int16_t rgy[2];
    TOK    *ptok;
    int16_t yEnemy;
    int16_t dzEnemy;
    int32_t dpGivenBest;
    int32_t dpTakenBest;
    int16_t y;
    int32_t dpTakenTotal;
    int32_t dpGivenCur;
    int16_t i;
    int16_t xEnemy;
    int16_t yCur;
    int32_t dpTaken;
    int32_t scoreThemBest;
    int32_t scoreThem;
    int16_t dzCur;
    int16_t rgx[2];
    uint8_t brcEnemy;
    int32_t dpGiven;
    int16_t dMax;
    int32_t scoreUs;
    uint8_t iplrSrc;
    int16_t fWeAttack;
    int16_t xCur;
    int16_t x;
    int16_t dMin;
    int16_t itok;

    iplrSrc = ptokSrc->iplr;
    xCur = ptokSrc->brc & 0xf;
    yCur = ptokSrc->brc >> 4;
    dpGivenBest = 0;
    dpTakenTotal = 0;
    ptok = app.state.vrgtok;
    for (itok = 0; itok < app.state.vctok; itok++) {
        if (ptok->fActive && iplrSrc != ptok->iplr && (1 << ptok->iplr & grfAttack)) {
            dzCur = DzFromBrcBrc(ptok->brc, brc);
            dMoves = ptok->dMovesLeft >= ptokSrc->dMovesLeft;
            if (dMoves == 0) {
                dMax = dzCur;
                dMin = dzCur;
            } else {
                dMin = 0 <= dzCur - dMoves ? dzCur - dMoves : 0;
                xEnemy = ptok->brc & 0xf;
                yEnemy = ptok->brc >> 4;
                rgx[0] = xEnemy - dMoves;
                rgx[1] = xEnemy + dMoves;
                rgy[0] = yEnemy - dMoves;
                rgy[1] = yEnemy + dMoves;
                dMax = dzCur;
                for (x = 0; x < 2; x++) {
                    for (y = 0; y < 2; y++) {
                        brcEnemy = ((((9) < (StarsMax(0, rgy[y]))) ? (9) : (StarsMax(0, rgy[y]))) & 0xf) << 4 | ((((9) < (StarsMax(0, rgx[x]))) ? (9) : (StarsMax(0, rgx[x]))) & 0xf);
                        dzEnemy = DzFromBrcBrc(brc, brcEnemy);
                        if (dzEnemy > dMax) {
                            dMax = dzEnemy;
                        }
                    }
                }
            }
            fWeAttack = FIsTargetOfMdTarget(ptok, static_cast<MdTarget>(!fPrimary ? ptokSrc->mdTarget2 : ptokSrc->mdTarget1));
            scoreThemBest = 30000000;
            iBest = dMin;
            for (i = dMin; i <= dMax; i++) {
                if (fWeAttack) {
                    dpGiven = DpFromPtokBrcToBrc(app, ptokSrc, 0, (i & 0xf) << 4 & 0xff, ptok, 0);
                } else {
                    dpGiven = 0;
                }
                dpTaken = DpFromPtokBrcToBrc(app, ptok, 0, (i & 0xf) << 4 & 0xff, ptokSrc, ptokSrc->mdTactic == mdTacticDisengage);
                scoreThem = ScoreFromGiveAndTakeAndTactic(dpTaken, dpGiven, static_cast<BattleTactic>(ptok->mdTactic));
                if (scoreThem <= scoreThemBest) {
                    scoreThemBest = scoreThem;
                    iBest = i;
                    dpTakenBest = dpTaken;
                    dpGivenCur = dpGiven;
                }
            }
            if (dpGivenCur > dpGivenBest) {
                dpGivenBest = dpGivenCur;
            }
            dpTakenTotal += dpTakenBest;
        }
        ptok++;
    }
    scoreUs = ScoreFromGiveAndTakeAndTactic(dpGivenBest, dpTakenTotal, static_cast<BattleTactic>(ptokSrc->mdTactic));
    return scoreUs;
}

int32_t ScoreFromGiveAndTakeAndTactic(int32_t dpGive, int32_t dpTake, BattleTactic mdTactic) {
    int32_t score;

    switch (mdTactic) {
    default:
        return 0;
    case mdTacticDisengage:
    case mdTacticMinDamageToSelf:
        return dpTake;
    case mdTacticDisengageIfChallenged:
    case mdTacticMaxDamage:
        return -dpGive;
    case mdTacticMaxNetDamage:
    case mdTacticMaxDamageRatio:
        score = -dpGive;
        if (score != 0) {
            score = (int32_t)((int32_t)(score * 100) / (dpTake + 1));
            if (score >= 0) {
                score = -1;
            }
        } else {
            score = dpTake;
        }
        return score;
    }
}

int16_t DxyMoveTokTo(APPSESSION &app, TOK *ptok, int16_t spdMove, uint16_t grfAttack) {
    uint16_t     iplr;
    int16_t      xMax;
    int16_t      dz;
    int32_t      scoreBest;
    uint8_t      brc;
    int32_t      rgscoreNear[3][3];
    int32_t      score;
    int16_t      cBest;
    int16_t      yMin;
    int16_t      dy;
    BattleTactic mdTactic;
    int16_t      y;
    uint8_t      brcOOR;
    int16_t      i;
    int16_t      yCur;
    uint8_t      brcBest;
    int16_t      dzAwayBest;
    int16_t      yMax;
    int16_t      dx;
    int16_t      xCur;
    int16_t      fPrimary;
    int32_t      dp;
    int16_t      dzAway;
    int16_t      x;
    int32_t      lLow;
    int16_t      cLow;
    int16_t      fXMajor;
    POINT16      rgptDeltas[2];

    iplr = ptok->iplr;
    dp = 0;
    xCur = ptok->brc & 0xf;
    yCur = ptok->brc >> 4;
    if (ptok->grobj == grobjPlanet || spdMove == 0)
        goto LReturnDxy;
    scoreBest = 30000000;
    mdTactic = static_cast<BattleTactic>(ptok->mdTactic);
    fPrimary = FDoesPrimaryTargetTypeExist(app, ptok, grfAttack);
    for (x = 0; x < 3; x++) {
        for (y = 0; y < 3; y++) {
            rgscoreNear[x][y] = 30000000;
        }
    }
    dz = DzMoveRangeToConsider(app, ptok, grfAttack, &brcOOR);
    x = xCur - dz;
    if (x < 0) {
        x = 0;
    }
    yMin = yCur - dz;
    if (yMin < 0) {
        yMin = 0;
    }
    xMax = xCur + dz;
    if (xMax >= 10) {
        xMax = 9;
    }
    yMax = yCur + dz;
    if (yMax >= 10) {
        yMax = 9;
    }
    for (; x <= xMax; x++) {
        for (y = yMin; y <= yMax; y++) {
            brc = (y & 0xf) << 4 | (x & 0xf);
            dx = xCur - x;
            dy = yCur - y;
            dx = abs(dx);
            dy = abs(dy);
            score = ScoreGuessBattleDamage(app, ptok, brc, fPrimary, grfAttack);
            if (mdTactic == mdTacticDisengage) {
                for (i = 0; i < app.state.vctok; i++) {
                    if (app.state.vrgtok[i].brc == brc && app.state.vrgtok[i].iplr == iplr) {
                        score += 2;
                    }
                }
                if (brc == ptok->brc) {
                    score--;
                }
            }
            dzAway = DzFromBrcBrc(ptok->brc, brc);
            if (dzAway <= 1) {
                rgscoreNear[x - xCur + 1][y - yCur + 1] = score;
            }
            if (score < scoreBest || (score == scoreBest && dzAway <= dzAwayBest)) {
                if (score == scoreBest && dzAway == dzAwayBest) {
                    cBest++;
                    if (RandomIO().pfnRandom(app, cBest) == 0)
                        goto LTakeSquare;
                    continue;
                } else {
                    cBest = 1;
                    scoreBest = score;
                    dzAwayBest = dzAway;
                }
            LTakeSquare:
                brcBest = brc;
            }
        }
    }
    if (brcOOR != 0xff) {
        brcBest = brcOOR;
    }
    dzAway = DzFromBrcBrc(ptok->brc, brcBest);
    if (dzAway > 1) {
        dx = (brcBest & 0xf) - xCur;
        dy = (brcBest >> 4) - yCur;
        if (abs(dx) == abs(dy)) {
            if (dx > 0) {
                xCur++;
            } else {
                xCur--;
            }
            if (dy > 0) {
                yCur++;
            } else {
                yCur--;
            }
        } else if (dx == 0) {
            lLow = 300000000;
            cLow = 0;
            dy = dy >= 0 ? 2 : 0;
            yCur += dy - 1;
            for (i = 0; i < 3; i++) {
                if (rgscoreNear[i][dy] <= lLow) {
                    if (rgscoreNear[i][dy] < lLow) {
                        lLow = rgscoreNear[i][dy];
                        cLow = 1;
                    } else {
                        cLow++;
                    }
                }
            }
            x = RandomIO().pfnRandom(app, cLow);
            for (i = 0; i < 3 && (rgscoreNear[i][dy] != lLow || x-- != 0); i++) {
            }
            xCur += i - 1;
        } else if (dy == 0) {
            lLow = 300000000;
            cLow = 0;
            dx = dx >= 0 ? 2 : 0;
            xCur += dx - 1;
            for (i = 0; i < 3; i++) {
                if (rgscoreNear[dx][i] <= lLow) {
                    if (rgscoreNear[dx][i] < lLow) {
                        lLow = rgscoreNear[dx][i];
                        cLow = 1;
                    } else {
                        cLow++;
                    }
                }
            }
            x = RandomIO().pfnRandom(app, cLow);
            for (i = 0; i < 3 && (rgscoreNear[dx][i] != lLow || x-- != 0); i++) {
            }
            yCur += i - 1;
        } else {
            fXMajor = abs(dx) > abs(dy);
            dx = dx <= 0 ? 0 : 2;
            dy = dy <= 0 ? 0 : 2;
            rgptDeltas[0].x = dx;
            rgptDeltas[0].y = dy;
            if (fXMajor) {
                rgptDeltas[1].x = dx;
                rgptDeltas[1].y = 1;
            } else {
                rgptDeltas[1].x = 1;
                rgptDeltas[1].y = dy;
            }
            if (rgscoreNear[rgptDeltas[0].x][rgptDeltas[0].y] < (int32_t)rgscoreNear[rgptDeltas[1].x][rgptDeltas[1].y] ||
                (rgscoreNear[rgptDeltas[0].x][rgptDeltas[0].y] == rgscoreNear[rgptDeltas[1].x][rgptDeltas[1].y] && RandomIO().pfnRandom(app, 2) == 0)) {
                i = 0;
            } else {
                i = 1;
            }
            xCur += rgptDeltas[i].x - 1;
            yCur += rgptDeltas[i].y - 1;
        }
        brcBest = (yCur & 0xf) << 4 | (xCur & 0xf);
    }
    if (scoreBest != 30000000) {
        if ((brcBest & 0xf) > 9 || brcBest >> 4 > 9) {
            brcBest = ptok->brc;
        }
        ptok->brc = brcBest;
    }
LReturnDxy:
    ptok->fMoved = 1;
    return 1;
}

int32_t CTorpHit(APPSESSION &app, int32_t cTorpBase, TOK *ptok, int16_t pctBase, int16_t pctBC) {
    int32_t pctJam;
    int16_t i;
    int32_t pctHit;
    int32_t cTorpHit;

    if (cTorpBase == 0 || pctBase == 0) {
        return 0;
    }
    pctJam = (uint32_t)ptok->pctJam;
    if (pctJam != 0 && pctBC != 0) {
        pctJam -= pctBC;
        if (pctJam < 0) {
            pctBC = -LOWORD(pctJam);
            pctJam = 0;
        } else {
            pctBC = 0;
        }
    }
    if (pctBC != 0) {
        pctHit = 100 - (int32_t)((100 - pctBase) * (int16_t)(100 - pctBC)) / 100;
    } else if (pctJam != 0) {
        pctHit = (int32_t)(pctBase * (100 - pctJam)) / 100;
    } else {
        pctHit = pctBase;
    }
    if (pctHit < 1) {
        pctHit = 1;
    }
    if (pctHit >= 100) {
        return cTorpBase;
    }
    if (cTorpBase > 200) {
        cTorpHit = (int32_t)(cTorpBase * pctHit) / 100;
    } else {
        cTorpHit = 0;
        for (i = 0; i < cTorpBase; i++) {
            if (RandomIO().pfnRandom(app, 100) < (int16_t)LOWORD(pctHit)) {
                cTorpHit++;
            }
        }
    }
    return cTorpHit;
}

int16_t FAttack(APPSESSION &app, int16_t itokAttacker, int16_t init, uint8_t *lpbtlrec, uint16_t grfAttack) {
    int32_t   dpShieldLeft;
    int16_t   dz;
    SHDEF    *lpshdefE;
    int32_t   dpArmorLeft;
    int32_t   dpSingle;
    int32_t   scoreBest;
    TOK      *ptok;
    int16_t   ctokDamaged;
    int16_t   itokTarget;
    int32_t   dpMain;
    int32_t   score;
    int16_t   fSetItok;
    int16_t   dxRangeCur;
    int16_t   ihs;
    int32_t   cTorpMiss;
    int32_t   cTorpFire;
    int32_t   cTorpsLeft;
    int16_t   i;
    int32_t   cTorpBase;
    GrfWeapon grfWeapon;
    int16_t   cItem;
    int32_t   pctHit;
    TOK      *ptokTarget;
    SHDEF    *lpshdef;
    int32_t   lValue;
    int32_t   dpT;
    HUL      *lphul;
    int32_t   cTorpHit;
    int16_t   fPrimary;
    int32_t   dp;
    int16_t   itok;
    int32_t   dpCol;
    TOK      *ptokE;
    PART      part;
    int32_t   nds;
    int16_t   fCapMissile;
    int32_t   nts;
    int32_t   ntk;
    int32_t   dpShieldCur;
    int32_t   dpHitArmor;

    dxRangeCur = 0;
    fSetItok = 0;
    ctokDamaged = 0;
    ptok = app.state.vrgtok + itokAttacker;
    lpshdef = LpshdefFromTok(app, ptok);
    lphul = &lpshdef->hul;
    for (ihs = 0; ihs < lphul->chs; ihs++) {
        if ((lphul->rghs[ihs].grhst & hstWeapon) && lphul->rghs[ihs].cItem != 0) {
            part.hs = lphul->rghs[ihs];
            app.state.idPlayer = ptok->iplr;
            if (!PartQueryIO().pfnLookup(app, &part)) {
            }
            app.state.idPlayer = iplrNone;
            cItem = lphul->rghs[ihs].cItem;
            i = ptok->initBase + part.pbeam()->init;
            if (i >= 64) {
                i = 63;
            }
            if (i == init) {
                dxRangeCur = (ptok->grobj == grobjPlanet) + part.pbeam()->dRangeMax;
                if (part.hs.grhst == hstBeam && (part.pbeam()->grfAbilities & beamGatling)) {
                    dp = (uint32_t)((uint32_t)(part.pbeam()->dp * cItem) * (uint32_t)ptok->csh);
                    if (part.pbeam()->dp >= 200) {
                        grfWeapon = bitFBeamHigh;
                    } else {
                        grfWeapon = bitFBeamLow;
                    }
                    if (ptok->pctCap != 0) {
                        dp = (int32_t)(dp * (int16_t)ptok->pctCap) / 100;
                    }
                    dpT = dp;
                    ptokE = app.state.vrgtok;
                    for (itok = 0; itok < app.state.vctok; itok++) {
                        if (ptokE->fActive && ptokE->iplr != ptok->iplr && (1 << ptokE->iplr & grfAttack) &&
                            DzFromBrcBrc(ptokE->brc, ptok->brc) <= dxRangeCur &&
                            (FIsTargetOfMdTarget(ptokE, static_cast<MdTarget>(ptok->mdTarget1)) || FIsTargetOfMdTarget(ptokE, static_cast<MdTarget>(ptok->mdTarget2)))) {
                            if (ptokE->pctBeamDef < 100) {
                                dp = (int32_t)(dp * (int16_t)ptokE->pctBeamDef) / 100;
                            }
                            if (FDamageTok(app, ptokE, itok, &dp, 0, grfWeapon, part.pbeam()->grfAbilities & beamSapper, nullptr)) {
                                if (!fSetItok) {
                                    fSetItok = 1;
                                    BevFromPrefix(lpbtlrec).SetItokAttack(itok);
                                }
                                ctokDamaged++;
                            }
                            dp = dpT;
                        }
                        ptokE++;
                    }
                } else {
                    if (part.hs.grhst == hstBeam) {
                        dpMain = (uint32_t)((uint32_t)(part.pbeam()->dp * cItem) * (uint32_t)ptok->csh);
                        cTorpsLeft = 0;
                    } else {
                        dpMain = 0;
                        cTorpsLeft = (uint32_t)(cItem * (uint32_t)ptok->csh);
                    }
                LFindAnotherTarget:
                    for (fPrimary = 1; fPrimary >= 0; fPrimary--) {
                        scoreBest = 0;
                        ptokTarget = nullptr;
                        ptokE = app.state.vrgtok;
                        for (itok = 0; itok < app.state.vctok; itok++) {
                            if (ptokE->fActive && ptokE->iplr != ptok->iplr && (1 << ptokE->iplr & grfAttack) &&
                                DzFromBrcBrc(ptokE->brc, ptok->brc) <= dxRangeCur &&
                                FIsTargetOfMdTarget(ptokE, static_cast<MdTarget>(!fPrimary ? ptok->mdTarget2 : ptok->mdTarget1))) {
                                lpshdefE = LpshdefFromTok(app, ptokE);
                                lValue = (uint32_t)(((uint32_t)lpshdefE->hul.resCost + (uint32_t)lpshdefE->hul.rgwtOreCost[1]) * (uint32_t)ptokE->csh);
                                if (lValue < 100000) {
                                    lValue = (uint32_t)(lValue * 100);
                                } else {
                                    lValue = 10000000;
                                }
                                dpSingle = (uint32_t)lpshdefE->hul.dp;
                                dpShieldLeft = (uint32_t)((uint32_t)ptokE->dpShield * (uint32_t)ptokE->csh);
                                dpArmorLeft = (uint32_t)(dpSingle * (uint32_t)ptokE->csh);
                                if (ptokE->dv.dp != 0) {
                                    dpArmorLeft -=
                                        (int32_t)((int32_t)((int32_t)(dpSingle * ptokE->dv.pctDp()) / 10 * ptokE->dv.pctSh()) / 10 * (uint32_t)ptokE->csh) / 500;
                                }
                                if (dpArmorLeft <= 0) {
                                    dpArmorLeft = 1;
                                }
                                if (part.hs.grhst == hstBeam || part.hs.grhst != hstTorp) {
                                    if (ptokE->pctBeamDef < 100) {
                                        lValue = (int32_t)(lValue * (uint32_t)ptokE->pctBeamDef) / 100;
                                    }
                                    if (part.pbeam()->grfAbilities & beamSapper) {
                                        if (dpShieldLeft <= 0) {
                                            score = 0;
                                        } else {
                                            score = (int32_t)((int32_t)((uint32_t)(lValue * 100) + dpShieldLeft - 1) / dpShieldLeft);
                                        }
                                    } else {
                                        score = (int32_t)((int32_t)(lValue * 100) / (dpArmorLeft + dpShieldLeft + 1));
                                        if (score <= 0) {
                                            score = 1;
                                        }
                                    }
                                } else {
                                    pctHit = part.ptorp()->dHitChance;
                                    if ((int16_t)ptok->pctBC >= ptokE->pctJam) {
                                        pctHit += (int32_t)((100 - pctHit) * (int16_t)(ptok->pctBC - ptokE->pctJam)) / 100;
                                    } else {
                                        pctHit -= (int32_t)(pctHit * (int16_t)(ptokE->pctJam - ptok->pctBC)) / 100;
                                    }
                                    if (pctHit > 0) {
                                        fCapMissile = part.hs.iItem >= itorpJihadMissile && part.hs.iItem <= itorpArmageddonMissile;
                                        if (dpArmorLeft < 100000) {
                                            nts = (int32_t)((int32_t)((uint32_t)(dpArmorLeft * 100) * 2) / pctHit);
                                        } else {
                                            nts = (uint32_t)((int32_t)(dpArmorLeft / pctHit) * 200);
                                        }
                                        if (dpShieldLeft < 100000) {
                                            nds = (int32_t)(dpShieldLeft * 100) / ((int32_t)(pctHit / 2) + (int32_t)((100 - pctHit) / 8));
                                        } else {
                                            nds = (uint32_t)((int32_t)(dpShieldLeft / ((int32_t)(pctHit / 2) + (int32_t)((100 - pctHit) / 8))) * 100);
                                        }
                                        ntk = (int32_t)((dpArmorLeft - (int32_t)(nds * pctHit) / 200) * 100) / (int32_t)(pctHit * (int16_t)(fCapMissile + 1));
                                        score = nts < nds + ntk ? nts : nds + ntk;
                                        if (score > 0) {
                                            score = (int32_t)(lValue / score);
                                            if (score <= 0) {
                                                score = 1;
                                            }
                                        } else {
                                            score = 0;
                                        }
                                    } else {
                                        score = 0;
                                    }
                                }
                                if (score > scoreBest) {
                                    scoreBest = score;
                                    ptokTarget = ptokE;
                                    itokTarget = itok;
                                }
                            }
                            ptokE++;
                        }
                        if (ptokTarget)
                            break;
                    }
                    if (!ptokTarget)
                        continue;
                    dz = DzFromBrcBrc(ptokTarget->brc, ptok->brc);
                    if (part.hs.grhst == hstBeam) {
                        dp = dpMain;
                        if (ptok->pctCap != 0) {
                            dp = (int32_t)(dp * (int16_t)ptok->pctCap) / 100;
                        }
                        if (ptokTarget->pctBeamDef < 100) {
                            dp = (int32_t)(dp * (int16_t)ptokTarget->pctBeamDef) / 100;
                        }
                        if (dz > 0 && part.pbeam()->dRangeMax > 0) {
                            dp = (int32_t)(dp * (100 - (int32_t)((int32_t)(dz * 10) / part.pbeam()->dRangeMax))) / 100;
                        }
                        if (part.pbeam()->dp >= 200) {
                            grfWeapon = bitFBeamHigh;
                        } else {
                            grfWeapon = bitFBeamLow;
                        }
                        dpT = dp;
                        if (FDamageTok(app, ptokTarget, itokTarget, &dp, 0, grfWeapon, part.pbeam()->grfAbilities & beamSapper, nullptr)) {
                            if (!fSetItok) {
                                BevFromPrefix(lpbtlrec).SetItokAttack(itokTarget);
                                fSetItok = 1;
                            }
                            ctokDamaged++;
                        }
                        if (dp > 0 && dpT > 0) {
                            if (dpMain < 65536 && dp < 65536) {
                                lValue = (int32_t)((int32_t)(dpMain * dp) / dpT);
                            } else {
                                lValue = (int32_t)((double)dpMain * dp / dpT);
                            }
                            dpMain = dpMain - 1 < lValue ? dpMain - 1 : lValue;
                        } else {
                            dpMain = 0;
                        }
                    } else if (part.hs.grhst == hstTorp && cTorpsLeft > 0) {
                        grfWeapon = bitFTorp;
                        cTorpBase = cTorpsLeft;
                        cTorpHit = CTorpHit(app, cTorpBase, ptokTarget, part.ptorp()->dHitChance, ptok->pctBC);
                        lpshdefE = LpshdefFromTok(app, ptokTarget);
                        dpSingle = (uint32_t)lpshdefE->hul.dp;
                        dpShieldLeft = (uint32_t)((uint32_t)ptokTarget->dpShield * (uint32_t)ptokTarget->csh);
                        dpArmorLeft = (uint32_t)(dpSingle * (uint32_t)ptokTarget->csh);
                        if (ptokTarget->dv.dp != 0) {
                            dpArmorLeft -=
                                (int32_t)((int32_t)((int32_t)(dpSingle * ptokTarget->dv.pctDp()) / 10 * ptokTarget->dv.pctSh()) / 10 * (uint32_t)ptokTarget->csh) /
                                500;
                        }
                        dp = part.ptorp()->dp;
                        if (part.hs.iItem >= itorpJihadMissile && part.hs.iItem <= itorpArmageddonMissile) {
                            if (dpShieldLeft <= 0) {
                                dp = (int32_t)(dp * 2);
                            }
                            grfWeapon = static_cast<GrfWeapon>(grfWeapon | bitFMissile);
                        }
                        i = ptokTarget->csh;
                        if (i >= cTorpBase || (int32_t)(uint32_t)(cTorpHit * dp) <= dpArmorLeft) {
                            cTorpFire = cTorpHit;
                            cTorpMiss = cTorpBase - cTorpHit;
                        } else {
                            for (; i <= cTorpBase; i++) {
                                cTorpFire = (int32_t)((int32_t)((uint32_t)(i * cTorpHit) + cTorpBase - 1) / cTorpBase);
                                cTorpMiss = i - cTorpFire;
                                dpShieldCur = dpShieldLeft - (int32_t)(cTorpMiss * dp) / 8;
                                if (dpShieldCur < 0) {
                                    dpShieldCur = 0;
                                }
                                dpShieldCur -= (int32_t)(cTorpFire * dp) / 2;
                                dpHitArmor = (int32_t)(cTorpFire * dp) / 2;
                                if (dpShieldCur < 0) {
                                    dpHitArmor -= dpShieldCur;
                                }
                                if (dpHitArmor >= dpArmorLeft)
                                    break;
                            }
                        }
                        dpCol = (int32_t)(cTorpMiss * dp) / 8;
                        if (dpCol > 0 && FDamageTok(app, ptokTarget, itokTarget, &dpCol, 0, static_cast<GrfWeapon>(grfWeapon | bitFDeflected), 1, nullptr)) {
                            ctokDamaged++;
                        }
                        dpT = (int32_t)(cTorpFire * dp) / 2;
                        cTorpBase = cTorpFire + cTorpMiss;
                        FDamageTok(app, ptokTarget, itokTarget, &dpT, dpT, grfWeapon, 0, &cTorpBase);
                        ctokDamaged++;
                        if (!fSetItok) {
                            fSetItok = 1;
                            BevFromPrefix(lpbtlrec).SetItokAttack(itokTarget);
                        }
                        cTorpsLeft -= cTorpFire + cTorpMiss;
                    }
                    if (dpMain > 0 || cTorpsLeft > 0)
                        goto LFindAnotherTarget;
                }
            }
        }
    }
    BevFromPrefix(lpbtlrec).SetCtok(ctokDamaged);
    if (ctokDamaged != 0) {
        return 1;
    }
    return 0;
}

void KillShips(APPSESSION &app, TOK *ptok, int16_t cshKill, int16_t ishdef, FLEET *lpfl, int16_t fFallout) {
    FLEET   flDead;
    int16_t i;
    FLEET   flSrc;
    int16_t csh;

    if (cshKill != 0) {
        if (fFallout) {
            MarkTechsSeen(app, &LpshdefFromTok(app, ptok)->hul, ptok->iplr);
        }
        flSrc = *lpfl;
        memset(&flDead, 0, sizeof(FLEET));
        csh = lpfl->rgcsh[ishdef] - cshKill;
        flDead.rgcsh[ishdef] = cshKill;
        flSrc.rgcsh[ishdef] = csh;
        ptok->csh = csh;
        if (csh == 0) {
            ptok->fActive = 0;
            for (ishdef = 0; ishdef < 16 && flSrc.rgcsh[ishdef] == 0; ishdef++) {
            }
            if (ishdef == 16) {
                lpfl->fDead = 1;
                if (fFallout) {
                    for (i = 0; i <= 2; i++) {
                        flDead.rgwtMin[i] = flSrc.rgwtMin[i];
                    }
                }
            }
        }
        if (!lpfl->fDead) {
            flDead.iPlayer = flSrc.iPlayer;
            flDead.fDead = 1;
            flDead.det = detAll;
            FleetTransferCargoBalance(app, &flSrc, &flDead);
        }
        if (fFallout) {
            flDead.iPlayer = flSrc.iPlayer;
            flDead.pt = flSrc.pt;
            flDead.idPlanet = flSrc.idPlanet;
            CreateSalvage(app, &flDead, &app.state.lpthBattle);
        }
        if (!lpfl->fDead) {
            *lpfl = flSrc;
        }
    }
    return;
}

void CreateSalvage(APPSESSION &app, FLEET *pfl, THING **plpth) {
    int32_t wtTotal;
    SHDEF  *lpshdefT;
    PLANET *lppl;
    int16_t i;
    int32_t rgwtMinerals[3];
    int16_t j;
    int16_t fBleeding;
    SHDEF   shdefT;

    fBleeding = RaceQueryIO().pfnGrbit(&app.state.rgplr[pfl->iPlayer], ibitRaceBleedingEdgeTech);
    app.state.gd.fDontCalcBleed = 1;
    app.state.idPlayer = pfl->iPlayer;
    if (pfl->idPlanet != idPlanetDeepSpace) {
        lppl = LpplFromId(app, pfl->idPlanet);
    } else {
        lppl = nullptr;
    }
    for (i = 0; i <= 2; i++) {
        rgwtMinerals[i] = 0;
        for (j = 0; j < 16; j++) {
            if (pfl->rgcsh[j] > 0) {
                if (fBleeding) {
                    shdefT = app.state.rglpshdef[pfl->iPlayer][j];
                    UpdateShdefCost(app, &shdefT);
                    lpshdefT = &shdefT;
                } else {
                    lpshdefT = app.state.rglpshdef[pfl->iPlayer] + j;
                }
                rgwtMinerals[i] += (int32_t)(pfl->rgcsh[j] * (uint32_t)lpshdefT->hul.rgwtOreCost[i]) / 3;
            }
        }
        rgwtMinerals[i] += pfl->rgwtMin[i];
        if (lppl) {
            lppl->rgwtMin[i] += (int32_t)(rgwtMinerals[i] * (uint32_t)(!lppl->fStarbase ? 5 : 8)) / 10;
        }
    }
    if (!lppl) {
        wtTotal = 0;
        for (i = 0; i <= 2; i++) {
            rgwtMinerals[i] -= (int32_t)(rgwtMinerals[i] >> 2);
            wtTotal += rgwtMinerals[i];
        }
        if (wtTotal != 0) {
            DropSalvage(app, plpth, rgwtMinerals, pfl->iPlayer, &pfl->pt);
        }
    }
    app.state.gd.fDontCalcBleed = 0;
    app.state.idPlayer = iplrNone;
    return;
}

int16_t FDamageTok(APPSESSION &app, TOK *ptok, int16_t itok, int32_t *pdpBeam, int32_t dpTorp, GrfWeapon grfWeapon, int16_t fShieldsOnly, int32_t *pcTorp) {
    int16_t   pctSh;
    DV        dv;
    uint16_t *pwLosses;
    int16_t   cshOrigDamaged;
    int32_t   dpShdef;
    int32_t   ddpOrig;
    int32_t   dpOrig;
    PLANET   *lppl;
    int16_t   i;
    int16_t   cshOrig;
    FLEET    *lpfl;
    int32_t   cKillMax;
    int16_t   csh;
    int32_t   dpT;
    int16_t   pctDp;
    int16_t   ishdef;
    int32_t   dp;
    uint16_t  pctDpNew;

    dp = *pdpBeam;
    memset(app.state.lpbBattleCur, 0, 8);
    BkvFromPrefix(app.state.lpbBattleCur).SetItok(itok);
    BkvFromPrefix(app.state.lpbBattleCur).SetGrfWeapon(grfWeapon);
    if (ptok->dpShield != 0) {
        dpOrig = (uint32_t)ptok->dpShield;
        dpT = (uint32_t)ptok->dpShield - dpOrig;
        dpOrig = (uint32_t)(dpOrig * (uint32_t)ptok->csh);
        if (dpOrig > dp) {
            dpOrig -= dp;
            BkvFromPrefix(app.state.lpbBattleCur).SetDpShield(WPackLong(dp));
            ptok->dpShield = LOWORD((int32_t)(dpOrig / (int32_t)ptok->csh)) + LOWORD(dpT);
            dp = 0;
        } else {
            dp -= dpOrig;
            BkvFromPrefix(app.state.lpbBattleCur).SetDpShield(WPackLong(dpOrig));
            ptok->dpShield = 0;
        }
    } else if (fShieldsOnly) {
        return 0;
    }
    if ((dp == 0 || fShieldsOnly) && dpTorp == 0) {
        BkvFromPrefix(app.state.lpbBattleCur).dv().SetDp(ptok->dv.dp);
        *pdpBeam = dp;
        if (BkvFromPrefix(app.state.lpbBattleCur).grfWeapon() & bitFTorp) {
            BkvFromPrefix(app.state.lpbBattleCur).SetGrfWeapon(BkvFromPrefix(app.state.lpbBattleCur).grfWeapon() | (bitFNoHit | bitFDeflected));
        }
        app.state.lpbBattleCur += 8;
        return 1;
    }
    if (pcTorp) {
        cKillMax = *pcTorp;
    } else {
        cKillMax = 2147483647;
    }
    dp += dpTorp;
    ishdef = ptok->ishdef;
    dpShdef = (uint32_t)LpshdefFromTok(app, ptok)->hul.dp;
    dv.dp = ptok->dv.dp;
    if (ptok->grobj == grobjPlanet) {
        lppl = LpplFromId(app, ptok->id);
        cKillMax--;
        if (dv.pctDp() != 0) {
            dp += (int32_t)(dpShdef * dv.pctDp()) / 500;
        }
        if (dp >= dpShdef) {
            BkvFromPrefix(app.state.lpbBattleCur).dv().SetPctDp(500);
            BkvFromPrefix(app.state.lpbBattleCur).SetCshKill(1);
            ptok->fActive = 0;
            ptok->csh = 0;
            app.state.fStarbaseDied = 1;
            if (RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMajorAdv) != raMacintosh) {
                lppl->fStarbase = 0;
                ProductionIO().pfnKillShips(app, lppl);
                ProductionIO().pfnKillPackets(app, lppl);
            }
        } else {
            pctDpNew = LOWORD((int32_t)((int32_t)(dp * 500) / dpShdef));
            if (lppl->pctDp != pctDpNew) {
                lppl->pctDp = LOWORD((int32_t)((int32_t)(dp * 500) / dpShdef));
            } else {
                lppl->pctDp++;
            }
            BkvFromPrefix(app.state.lpbBattleCur).dv().SetPctDp(lppl->pctDp);
            app.state.fStarbaseDamaged = 1;
        }
        if (BkvFromPrefix(app.state.lpbBattleCur).dv().pctDp() != 0) {
            BkvFromPrefix(app.state.lpbBattleCur).dv().SetPctSh(100);
            ptok->dv.dp = BkvFromPrefix(app.state.lpbBattleCur).dv().dp();
        }
        *pdpBeam = 0;
        app.state.lpbBattleCur += 8;
        return 1;
    }
    if (ptok->mdTactic == mdTacticDisengageIfChallenged) {
        ptok->mdTactic = mdTacticDisengage;
        ptok->dzDis = 7;
    }
    lpfl = LpflFromId(app, ptok->id);
    dpOrig = dp;
    csh = ptok->csh;
    cshOrig = csh;
    if (dv.pctDp() != 0) {
        cshOrigDamaged = LOWORD((int32_t)(csh * dv.pctSh()) / 100);
        if (cshOrigDamaged == 0) {
            cshOrigDamaged = 1;
        }
        ddpOrig = (int32_t)(dpShdef * dv.pctDp()) / 500;
        if (ddpOrig == 0) {
            ddpOrig = 1;
        }
    } else {
        cshOrigDamaged = 0;
        ddpOrig = 0;
    }
    pwLosses = app.state.vrgPlrLosses + ((ptok->iplr << 4) + ishdef);
    *pwLosses |= 0x8000;
    if (cshOrigDamaged != 0) {
        csh = cshOrigDamaged;
        dpShdef -= ddpOrig;
        while (dp >= dpShdef && csh != 0 && cKillMax != 0) {
            dp -= dpShdef;
            csh--;
            cKillMax--;
            if ((*pwLosses & 0x1fff) < 0x1fff) {
                (*pwLosses)++;
            }
        }
        dpShdef += ddpOrig;
        i = cshOrigDamaged;
        cshOrigDamaged = csh;
        csh += cshOrig - i;
    }
    while (dp >= dpShdef && csh != 0 && cKillMax != 0) {
        dp -= dpShdef;
        csh--;
        cKillMax--;
        if ((*pwLosses & 0x1fff) < 0x1fff) {
            (*pwLosses)++;
        }
    }
    if (cKillMax <= 0) {
        dp = 0;
    }
    if (dp != 0 && csh != 0) {
        if (cshOrigDamaged != 0) {
            dp += (uint32_t)(ddpOrig * cshOrigDamaged) + csh - 1;
        }
        dp = (int32_t)(dp / csh);
        if (dp == 0) {
            dp = 1;
        }
        pctDp = LOWORD((int32_t)((int32_t)((uint32_t)(dp * 500) + dpShdef - 1) / dpShdef));
        if (pctDp == 0) {
            pctDp = 1;
        }
        pctSh = 100;
    } else if (cshOrigDamaged != 0) {
        pctSh = LOWORD((int32_t)((int32_t)((uint32_t)(cshOrigDamaged * 100) + csh - 1) / csh));
        pctDp = ptok->dv.pctDp();
    } else {
        pctSh = 0;
        pctDp = 0;
    }
    BkvFromPrefix(app.state.lpbBattleCur).SetCshKill(ptok->csh - csh);
    if (csh != ptok->csh) {
        KillShips(app, ptok, BkvFromPrefix(app.state.lpbBattleCur).cshKill(), ishdef, lpfl, 1);
    }
    if (csh != 0) {
        if (pctDp > 499) {
            pctDp = 499;
        }
        dv.SetPctDp(pctDp);
        dv.SetPctSh(pctSh);
        ptok->dv.dp = dv.dp;
        lpfl->rgdv[ishdef].dp = dv.dp;
        dp = 0;
    }
    if (dp > dpTorp) {
        *pdpBeam = dp - dpTorp;
    } else {
        *pdpBeam = 0;
    }
    dpOrig -= *pdpBeam;
    BkvFromPrefix(app.state.lpbBattleCur).dv().SetDp(ptok->dv.dp);
    app.state.lpbBattleCur += 8;
    if (pcTorp) {
        *pcTorp = cKillMax;
    }
    return 1;
}

int16_t DxyFromSpdRound(uint16_t spd, int16_t iRound) {
    int16_t dxy;

    dxy = (uint32_t)(spd + 2) / 4;
    switch (spd & 3) {
    case 0:
        dxy += (iRound & 1) == 0;
        break;
    case 1:
        dxy += (iRound & 3) != 2;
        break;
    case 3:
        dxy += (iRound & 3) == 0;
    }
    return dxy;
}

int16_t FDoCoolBattle(APPSESSION &app, FLEET *lpfl, int16_t cplr, uint16_t *rggrfAttack, uint16_t grfPlayer, uint16_t grfSpectator) {
    int16_t  cShipsInvolved;
    TOK     *ptok;
    uint16_t wt;
    int16_t  cShdefsInvolved;
    uint8_t *lpbSav;
    int16_t  initMac;
    int16_t  init;
    uint16_t wtT;
    uint16_t grplrLeft;
    int16_t  i;
    int16_t  j;
    int16_t  initMin;
    uint8_t *lpbtlrec;
    int16_t  iRound;
    FLEET   *lpflT;
    uint16_t brcOrig;
    uint8_t *lpbtldata;
    uint8_t  rgfInit[64];
    uint16_t rgPlrLosses[256];
    uint16_t wtNext;
    int16_t  itok;
    PLANET  *lppl;
    int32_t  lwt;
    HB      *lphb; /* NATIVE: recover the Win16 heap-relative offset. */

    if (!app.state.lpbBattleLog) {
        StarsRecoveryScope scopeRecovery;
        try {
            app.state.lpbBattleLog = LpAlloc<uint8_t>(app, 0xffc8, htBattle);
            app.state.lpbBattleCur = app.state.lpbBattleLog;
        } catch (const StarsError &) {
            return -1;
        }
    }
    if (!app.state.lpbBattleT) {
        StarsRecoveryScope scopeRecovery;
        try {
            app.state.lpbBattleT = LpAlloc<uint8_t>(app, 0xffc8, htBattle);
        } catch (const StarsError &) {
            return -1;
        }
    }
    lpbSav = app.state.lpbBattleCur;
    app.state.lpbBattleCur = app.state.lpbBattleT;
    memset(rgPlrLosses, 0, 0x200);
    app.state.vrgPlrLosses = rgPlrLosses;
    memset(rgfInit, 0, 64);
    memset(app.state.vrgtok, 0, 256 * sizeof(TOK));
    app.state.vctok = 0;
    lpbtldata = (uint8_t *)app.state.lpbBattleCur;
    app.state.lpbBattleCur += 14;
    memset(app.state.rgTechBattle, 0, 6);
    memset(app.state.rgTechTrader, 0, 13);
    app.state.lpthBattle = nullptr;
    cShdefsInvolved = 0;
    cShipsInvolved = 0;
    app.state.fStarbaseDied = 0;
    app.state.fStarbaseDamaged = 0;
    lpflT = lpfl;
    do {
        if (!lpflT->fDead) {
            for (i = 0; i < 16; i++) {
                if (lpflT->rgcsh[i] > 0) {
                    cShipsInvolved += lpflT->rgcsh[i];
                    rgPlrLosses[lpflT->iPlayer * 16 + i] = 0x8000;
                }
            }
        }
        lpflT = lpflT->lpflNext;
    } while (lpflT != lpfl && lpflT);
    for (i = 0; i < 256; i++) {
        if (rgPlrLosses[i] != 0) {
            rgPlrLosses[i] = 0;
            cShdefsInvolved++;
        }
    }
    if (lpfl->idPlanet != idPlanetDeepSpace) {
        lppl = LpplFromId(app, lpfl->idPlanet);
        if (lppl->fStarbase && (1 << lppl->iPlayer & grfPlayer)) {
            cShdefsInvolved++;
            cShipsInvolved++;
            lppl->fNoHeal = 1;
        }
    }
    InitializeBoard(app, lpfl, (int16_t)((cplr - 1) * cplr) / 2, grfPlayer, rgfInit, &initMin, &initMac);
    BdvFromPrefix(lpbtldata).SetCplr(cplr);
    BdvFromPrefix(lpbtldata).SetCtok(app.state.vctok);
    BdvFromPrefix(lpbtldata).SetIdPlanet(lpfl->idPlanet);
    BdvFromPrefix(lpbtldata).SetPt(lpfl->pt);
    BdvFromPrefix(lpbtldata).SetId(app.state.idBattle++);
    for (iRound = 0; iRound < 16; iRound++) {
        grplrLeft = 0;
        for (itok = 0; itok < app.state.vctok; itok++) {
            if (app.state.vrgtok[itok].fActive) {
                grplrLeft |= 1 << app.state.vrgtok[itok].iplr;
                app.state.vrgtok[itok].cTarget = 0;
                if (iRound > 0 && app.state.vrgtok[itok].dpShield > 0 && app.state.vrgtok[itok].fActive &&
                    RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.vrgtok[itok].iplr], ibitRaceRegeneratingShields) != 0) {
                    RegenShield(app, app.state.vrgtok + itok);
                }
            }
        }
        if (!((grplrLeft - 1) & grplrLeft))
            break;
        ptok = app.state.vrgtok;
        for (itok = 0; itok < app.state.vctok; itok++) {
            if (ptok->fActive) {
                if (ptok->grobj == grobjPlanet) {
                    ptok->dMovesLeft = 0;
                } else {
                    ptok->dMovesLeft = DxyFromSpdRound(ptok->spd, iRound);
                }
            }
            ptok++;
        }
        for (j = 3; j > 0; j--) {
            wtNext = 0;
            wt = 30000;
            i = app.state.vctok;
            do {
                wtNext = 0;
                ptok = app.state.vrgtok;
                for (itok = 0; itok < app.state.vctok; ptok++, itok++) {
                    if (!ptok->fActive || ptok->wt == 0xffff) {
                        i--;
                    } else {
                        lwt = ptok->dwt;
                        lwt -= 7;
                        lwt = (int32_t)(lwt * 2);
                        lwt = (uint32_t)ptok->wt + (int32_t)((uint32_t)ptok->wt * lwt) / 100;
                        wtT = LOWORD(lwt);
                        if (wtT > wtNext && wtT < wt && DxyFromSpdRound(ptok->spd, iRound) != 0) {
                            wtNext = wtT;
                        }
                        if (wtT == wt) {
                            i--;
                            if ((int16_t)ptok->dMovesLeft >= j) {
                                lpbtlrec = (uint8_t *)app.state.lpbBattleCur;
                                app.state.lpbBattleCur += 6;
                                BevFromPrefix(lpbtlrec).SetItok(itok);
                                BevFromPrefix(lpbtlrec).SetCtok(0);
                                BevFromPrefix(lpbtlrec).SetItokAttack(itok);
                                BevFromPrefix(lpbtlrec).SetIRound(iRound);
                                BevFromPrefix(lpbtlrec).SetDzDis(ptok->dzDis);
                                brcOrig = app.state.vrgtok[itok].brc;
                                if (ptok->mdTactic == mdTacticDisengage) {
                                    brcOrig = 0xff;
                                    if (ptok->dzDis == 0) {
                                        BevFromPrefix(lpbtlrec).SetBrcDest(0xff);
                                        ptok->fActive = 0;
                                        continue;
                                    }
                                    ptok->dzDis += 0x7ff;
                                }
                                DxyMoveTokTo(app, ptok, j, rggrfAttack[ptok->iplr]);
                                ptok->dMovesLeft += 3;
                                if (ptok->grobj != grobjPlanet && (brcOrig != ptok->brc || ptok->initMin == 0xff)) {
                                    BevFromPrefix(lpbtlrec).SetBrcDest(ptok->brc);
                                } else {
                                    app.state.lpbBattleCur -= 6;
                                }
                            }
                        }
                    }
                }
                wt = wtNext;
            } while (wtNext != 0);
        }
        grplrLeft = 0;
        for (i = 0; i < app.state.vctok; i++) {
            if (app.state.vrgtok[i].fActive) {
                app.state.vrgtok[i].dwt = RandomIO().pfnRandom(app, 15);
                grplrLeft |= 1 << app.state.vrgtok[i].iplr;
            }
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if ((1 << i & grplrLeft) && !(grplrLeft & rggrfAttack[i])) {
                grplrLeft &= ~(1 << i);
            }
        }
        if (!((grplrLeft - 1) & grplrLeft))
            break;
        for (init = initMac; init >= initMin; init--) {
            if (rgfInit[init]) {
                for (itok = app.state.vctok - 1; itok >= 0; itok--) {
                    if (init >= app.state.vrgtok[itok].initMin && init <= app.state.vrgtok[itok].initMac) {
                        grplrLeft = 0;
                        for (i = 0; i < app.state.vctok; i++) {
                            if (app.state.vrgtok[i].fActive) {
                                grplrLeft |= 1 << app.state.vrgtok[i].iplr;
                            }
                        }
                        if (!((grplrLeft - 1) & grplrLeft))
                            break;
                        ptok = app.state.vrgtok + itok;
                        if (ptok->fActive) {
                            lpbtlrec = (uint8_t *)app.state.lpbBattleCur;
                            app.state.lpbBattleCur += 6;
                            BevFromPrefix(lpbtlrec).SetItok(itok);
                            BevFromPrefix(lpbtlrec).SetCtok(0);
                            BevFromPrefix(lpbtlrec).SetIRound(iRound);
                            BevFromPrefix(lpbtlrec).SetBrcDest(ptok->brc);
                            BevFromPrefix(lpbtlrec).SetItokAttack(itok);
                            BevFromPrefix(lpbtlrec).SetDzDis(ptok->dzDis);
                            if (!FAttack(app, itok, init, lpbtlrec, rggrfAttack[ptok->iplr])) {
                                app.state.lpbBattleCur -= 6;
                            } else {
                                ptok->fMoved = 0;
                            }
                        }
                    }
                }
            }
        }
        if (!((grplrLeft - 1) & grplrLeft))
            break;
    }
    BdvFromPrefix(lpbtldata).SetCbData(app.state.lpbBattleCur - (uint8_t *)lpbtldata);
    SendBattleMessages(app, lpfl, cplr, BdvFromPrefix(lpbtldata).id(), rgPlrLosses, grfPlayer, cShipsInvolved, cShdefsInvolved, grfSpectator);
    BdvFromPrefix(lpbtldata).SetGrfPlr(grfPlayer);
    /* NATIVE: Win16 offsets include its 16-byte HB, not native address bits. */
    lphb = LphbFromLpHt(app, lpbSav, htBattle);
    if (0xffc8 - (uint32_t)(lpbSav - (uint8_t *)lphb - sizeof(HB) + 16) < (uint32_t)BdvFromPrefix(lpbtldata).cbData()) {
        RawStore16(lpbSav, 0xffff);
        app.state.lpbBattleT = nullptr;
    } else {
        memmove(lpbSav, lpbtldata, BdvFromPrefix(lpbtldata).cbData());
        app.state.lpbBattleCur = lpbSav + BdvFromPrefix(lpbtldata).cbData();
    }
    return 1;
}

int16_t ITechLearnATech(APPSESSION &app, int16_t iplr, int16_t x, int16_t y, MessageId idm, uint16_t *piGoto) {
    uint16_t iGoto;
    int16_t  fBattle;
    int16_t  i;
    int16_t  iTech;
    int32_t  l;

    fBattle = idm != static_cast<MessageId>(idmNone);
    if (app.state.rgplr[iplr].fLearned || RandomIO().pfnRandom(app, 100) < 50) {
        return 0;
    }
    for (i = 0; i < 13; i++) {
        iTech = RandomIO().pfnRandom(app, 13);
        if (app.state.rgTechTrader[iTech] != 0 && !(1 << iTech & app.state.rgplr[iplr].grbitTrader)) {
            if (RandomIO().pfnRandom(app, 100) < app.state.rgTechTrader[iTech]) {
                idm = static_cast<MessageId>(IdmGiveTraderPart(app, static_cast<GrbitTrader>(1 << iTech), iplr, &iGoto));
                if (fBattle) {
                    // The battle wreckage versions of the part and hull messages.
                    idm = static_cast<MessageId>(idm + idmExaminationWreckageBattleUncoveredPlansNewPart - idmHasAbsorbedMysteryTraderHaveGivenPlans);
                    TurnIO().pfnMessage2(app, iplr, idm, static_cast<MsgGoto>(iGoto), x, y);
                } else if (piGoto) {
                    *piGoto = iGoto;
                }
                app.state.rgplr[iplr].fLearned = 1;
                return -(iTech + 1);
            }
        }
    }
    for (i = 0; i < 6; i++) {
        iTech = RandomIO().pfnRandom(app, 6);
        if (app.state.rgplr[iplr].rgTech[iTech] < app.state.rgTechBattle[iTech]) {
            l = ResearchIO().pfnTechCost(app, static_cast<TechFieldType>(iTech), app.state.rgplr[iplr].rgTech[iTech] + 1, iplr);
            if (app.state.game.fSlowTech) {
                l = (int32_t)(l >> 1);
            }
            app.state.rgplr[iplr].rgResSpent[iTech] += l;
            if (fBattle) {
                if (app.state.game.fSlowTech) {
                    l = (int32_t)(l * 2);
                }
                TurnIO().pfnMessage(app, iplr, idm, gotoResearch, x, y, iTech, LOWORD(l), HIWORD(l), 0, 0);
            } else if (piGoto) {
                *piGoto = 0xfffe;
            }
            app.state.rgplr[iplr].fLearned = 1;
            return iTech + 1;
        }
    }
    return 0;
}

void SendBattleMessages(APPSESSION &app, FLEET *lpflBtl, int16_t cplr, int16_t idBtl, uint16_t *rgPlrLosses, int16_t grfPlayer, int16_t cShipsInvolved, int16_t cShdefsInvolved,
                        uint16_t grfSpectator) {
    int16_t   iplrStarbase;
    int16_t   iplr;
    uint8_t   rgcfl[16];
    int32_t   lpopStarbase;
    uint16_t *pw;
    int16_t   isb;
    PLANET   *lppl;
    uint16_t *pwThem;
    int16_t   fAlive;
    int16_t   cUs;
    int16_t   y;
    FLEET    *lpfl;
    int16_t   cThemDead;
    int16_t   i;
    MessageId idm;
    int16_t   j;
    FLEET    *lpflT;
    int16_t   cUsDead;
    int16_t   iThem;
    uint16_t *pwUs;
    int16_t   cThem;
    int16_t   x;

    iplrStarbase = iplrNone;
    lppl = nullptr;
    memset(rgcfl, 0, 16);
    if (lpflBtl->idPlanet != idPlanetDeepSpace) {
        x = -1;
        y = lpflBtl->idPlanet;
        lppl = LpplFromId(app, y);
        iThem = lppl->iPlayer;
        iplr = lppl->iPlayer;
        if (iplr != iplrNone) {
            if (lppl->fStarbase || app.state.fStarbaseDied) {
                iplrStarbase = iplr;
                isb = lppl->isb;
            }
            if (app.state.fStarbaseDied && RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMajorAdv) == raMacintosh) {
                lpopStarbase = lppl->rgwtMin[3];
                UninhabitPlanet(app, lppl);
            }
        }
    } else {
        x = lpflBtl->pt.x;
        y = lpflBtl->pt.y;
    }
    lpfl = lpflBtl;
    lpflT = nullptr;
    do {
        if (!lpfl->fDead) {
            for (i = 0; i < 16; i++) {
                if (lpfl->rgcsh[i] > 0) {
                    rgPlrLosses[(lpfl->iPlayer << 4) + i] = rgPlrLosses[(lpfl->iPlayer << 4) + i] | 0x4000;
                }
            }
        }
        lpfl = lpfl->lpflNext;
    } while (lpfl != lpflBtl && lpfl);
    for (iplr = 0; iplr < app.state.game.cPlayer; iplr++) {
        if (1 << iplr & grfPlayer) {
            fAlive = 1;
            if (app.state.fStarbaseDied && RaceQueryIO().pfnStat(&app.state.rgplr[iplrStarbase], rsMajorAdv) == raMacintosh) {
                if (iplr != iplrStarbase) {
                    idm = idmBattleTookPlaceDestroyedKillingColonistsBargain;
                } else if (lpopStarbase > 1000) {
                    idm = idmBattleTookPlaceDestroyedScreamsColonistsEcho;
                } else {
                    idm = idmBattleTookPlaceDestroyedColonistsHaveJoined;
                }
                j = iplrStarbase << 5 | (isb + 0x10);
                TurnIO().pfnMessage(app, iplr, idm, static_cast<MsgGoto>(idBtl | 0x4000), x, y, j, LOWORD(lpopStarbase), HIWORD(lpopStarbase), 0, 0);
                continue;
            }
            if (cplr != 2)
                goto CommonCountingCode;
            if (cShipsInvolved == 2) {
                pwThem = 0;
                pwUs = 0;
                pw = rgPlrLosses;
                for (i = 0; i < 16; i++) {
                    j = 0;
                    while (j < 16) {
                        if (*pw != 0) {
                            if (i == iplr) {
                                pwUs = pw;
                            } else {
                                pwThem = pw;
                            }
                        }
                        j++;
                        pw++;
                    }
                }
                if ((!pwUs && app.state.fStarbaseDied) || (pwUs && (*pwUs & 0x3fff))) {
                    if ((!pwThem && app.state.fStarbaseDamaged) || (pwThem && (*pwThem & 0x8000))) {
                        idm = idmBattleTookPlaceDestroyedWhichDamagedFray;
                    } else {
                        idm = idmBattleTookPlaceDestroyedWhichTookDamage;
                    }
                    fAlive = 0;
                } else if ((pwThem || !app.state.fStarbaseDied) && (!pwThem || !(*pwThem & 0x3fff))) {
                    idm = idmBattleTookPlaceNeitherNorDestroyedIncident;
                } else if ((!pwUs && app.state.fStarbaseDamaged) || (pwUs && (*pwUs & 0x8000))) {
                    idm = idmBattleTookPlaceDestroyedHoweverTookDamage;
                } else {
                    idm = idmBattleTookPlaceDestroyedTakingDamage;
                }
                if (pwUs) {
                    i = ((uint8_t *)pwUs - (uint8_t *)rgPlrLosses) >> 1;
                    i = (i & 0xf0) << 1 | (i & 0xf);
                } else {
                    i = iplrStarbase << 5 | (isb + 0x10);
                }
                if (pwThem) {
                    j = ((uint8_t *)pwThem - (uint8_t *)rgPlrLosses) >> 1;
                    j = (j & 0xf0) << 1 | (j & 0xf);
                } else {
                    j = iplrStarbase << 5 | (isb + 0x10);
                }
                TurnIO().pfnMessage(app, iplr, idm, static_cast<MsgGoto>(idBtl | 0x4000), x, y, i, j, 0, 0, 0);
                if (!fAlive || (lppl && lppl->iPlayer != iplrNone && iplr != lppl->iPlayer))
                    continue;
                ITechLearnATech(app, iplr, x, y, idmWreckageDiscoveredBattleHasBoostedResearchResour, nullptr);
                continue;
            }
            if (cShdefsInvolved == 2) {
                pwThem = 0;
                pwUs = 0;
                pw = rgPlrLosses;
                for (i = 0; i < 16; i++) {
                    j = 0;
                    while (j < 16) {
                        if (*pw != 0) {
                            if (i == iplr) {
                                pwUs = pw;
                            } else {
                                pwThem = pw;
                            }
                        }
                        j++;
                        pw++;
                    }
                }
                if ((!pwUs && app.state.fStarbaseDied) || (pwUs && !(*pwUs & 0x4000))) {
                    if ((!pwThem && app.state.fStarbaseDamaged) || (pwThem && (*pwThem & 0x8000))) {
                        idm = idmBattleTookPlaceDestroyedWhichDamagedFray2;
                    } else {
                        idm = idmBattleTookPlaceDestroyedWhichTookDamage2;
                    }
                    fAlive = 0;
                } else if ((pwThem || !app.state.fStarbaseDied) && (!pwThem || (*pwThem & 0x4000))) {
                    idm = idmBattleTookPlaceNeitherNorCompletelyDestroyed;
                } else if ((!pwUs && app.state.fStarbaseDamaged) || (pwUs && (*pwUs & 0x8000))) {
                    idm = idmBattleTookPlaceDestroyedHoweverTookDamage2;
                } else {
                    idm = idmBattleTookPlaceDestroyedTakingDamage2;
                }
                if (!pwUs) {
                    cUs = 1;
                } else {
                    cUs = *pwUs & 0x1fff;
                }
                if (!pwThem) {
                    cThem = 1;
                } else {
                    cThem = *pwThem & 0x1fff;
                }
                lpfl = lpflBtl;
                do {
                    if (!lpfl->fDead) {
                        if (lpfl->iPlayer == iplr && pwUs) {
                            cUs += lpfl->rgcsh[(((uint8_t *)pwUs - (uint8_t *)rgPlrLosses) >> 1) - (lpfl->iPlayer << 4)];
                        } else if (pwThem) {
                            cThem += lpfl->rgcsh[(((uint8_t *)pwThem - (uint8_t *)rgPlrLosses) >> 1) - (lpfl->iPlayer << 4)];
                        }
                    }
                    lpfl = lpfl->lpflNext;
                } while (lpfl != lpflBtl && lpfl);
                if (pwUs) {
                    i = ((uint8_t *)pwUs - (uint8_t *)rgPlrLosses) >> 1;
                    i = (i & 0xf0) << 1 | (i & 0xf);
                } else {
                    i = iplrStarbase << 5 | (isb + 0x10);
                }
                if (pwThem) {
                    j = ((uint8_t *)pwThem - (uint8_t *)rgPlrLosses) >> 1;
                    j = (j & 0xf0) << 1 | (j & 0xf);
                } else {
                    j = iplrStarbase << 5 | (isb + 0x10);
                }
                TurnIO().pfnMessage(app, iplr, idm, static_cast<MsgGoto>(idBtl | 0x4000), x, y, i, cUs, j, cThem, 0);
                if (!fAlive || (lppl && lppl->iPlayer != iplrNone && iplr != lppl->iPlayer))
                    continue;
                ITechLearnATech(app, iplr, x, y, idmWreckageDiscoveredBattleHasBoostedResearchResour, nullptr);
                continue;
            }
        CommonCountingCode:
            cThem = 0;
            cUs = 0;
            pw = rgPlrLosses;
            for (i = 0; i < 16; i++) {
                j = 0;
                while (j < 16) {
                    if (*pw != 0) {
                        if (i == iplr) {
                            pwUs = pw;
                            cUs += *pw & 0x1fff;
                        } else {
                            pwThem = pw;
                            cThem += *pw & 0x1fff;
                            iThem = i;
                        }
                    }
                    j++;
                    pw++;
                }
            }
            iThem |= 0x30;
            cUsDead = cUs;
            cThemDead = cThem;
            if (app.state.fStarbaseDied) {
                if (iplrStarbase == iplr) {
                    cUsDead++;
                } else if (iplrStarbase != iplrNone) {
                    cThemDead++;
                }
            }
            lpfl = lpflBtl;
            do {
                if (!lpfl->fDead) {
                    if (lpfl->iPlayer == iplr) {
                        for (i = 0; i < 16; i++) {
                            cUs += lpfl->rgcsh[i];
                        }
                    } else {
                        for (i = 0; i < 16; i++) {
                            cThem += lpfl->rgcsh[i];
                        }
                    }
                }
                lpfl = lpfl->lpflNext;
            } while (lpfl != lpflBtl && lpfl);
            if (iplrStarbase == iplr) {
                cUs++;
            } else if (iplrStarbase != iplrNone) {
                cThem++;
                iThem = iplrStarbase | 0x10 | 0x20;
            }
            if (cplr == 2) {
                if (cThem == 1) {
                    if ((iThem & 0xf) == iplrStarbase) {
                        j = iplrStarbase << 5 | (isb + 0x10);
                    } else {
                        j = ((uint8_t *)pwThem - (uint8_t *)rgPlrLosses) >> 1;
                        j = (j & 0xf0) << 1 | (j & 0xf);
                    }
                }
                if (cUs == 1) {
                    if (iplr == iplrStarbase) {
                        i = iplrStarbase << 5 | (isb + 0x10);
                    } else {
                        i = ((uint8_t *)pwUs - (uint8_t *)rgPlrLosses) >> 1;
                        i = (i & 0xf0) << 1 | (i & 0xf);
                    }
                }
                if (cThemDead == cThem) {
                    idm = idmBattleTookPlaceAgainstForcesDestroyedEnemy;
                    if (cThemDead == 1) {
                        idm = static_cast<MessageId>(idm + 5);
                        if (cUsDead == 0) {
                            TurnIO().pfnMessage(app, iplr, idm, static_cast<MsgGoto>(idBtl | 0x4000), x, y, iThem, cUs, j, 0, 0);
                        } else {
                            TurnIO().pfnMessage(app, iplr, static_cast<MessageId>(idm + 2), static_cast<MsgGoto>(idBtl | 0x4000), x, y, iThem, cUs, j, cUsDead, 0);
                        }
                    } else if (cUsDead != 0) {
                        TurnIO().pfnMessage(app, iplr, static_cast<MessageId>(idm + 2), static_cast<MsgGoto>(idBtl | 0x4000), x, y, iThem, cUs, cUsDead, 0, 0);
                    } else if (cUs == 1) {
                        TurnIO().pfnMessage(app, iplr, idmBattleTookPlaceAgainstDestroyedEnemyForces, static_cast<MsgGoto>(idBtl | 0x4000), x, y, iThem, i, cThemDead, 0, 0);
                    } else {
                        TurnIO().pfnMessage(app, iplr, idm, static_cast<MsgGoto>(idBtl | 0x4000), x, y, iThem, cUs, 0, 0, 0);
                    }
                } else if (cUsDead == cUs) {
                    idm = idmBattleTookPlaceAgainstForcesDestroyedEnemys;
                    if (cUsDead == 1) {
                        idm = static_cast<MessageId>(idm + 5);
                        if (cThemDead == 0) {
                            TurnIO().pfnMessage(app, iplr, idm, static_cast<MsgGoto>(idBtl | 0x4000), x, y, iThem, i, cThem, 0, 0);
                        } else {
                            TurnIO().pfnMessage(app, iplr, static_cast<MessageId>(idm + 2), static_cast<MsgGoto>(idBtl | 0x4000), x, y, iThem, i, cThem, cThemDead, 0);
                        }
                    } else if (cThemDead != 0) {
                        TurnIO().pfnMessage(app, iplr, static_cast<MessageId>(idm + 2), static_cast<MsgGoto>(idBtl | 0x4000), x, y, iThem, cThem, cThemDead, 0, 0);
                    } else if (cThem == 1) {
                        TurnIO().pfnMessage(app, iplr, idmBattleTookPlaceAgainstForcesDestroyed, static_cast<MsgGoto>(idBtl | 0x4000), x, y, iThem, cUsDead, j, 0, 0);
                    } else {
                        TurnIO().pfnMessage(app, iplr, idm, static_cast<MsgGoto>(idBtl | 0x4000), x, y, iThem, cThem, 0, 0, 0);
                    }
                } else if (cUs == 1) {
                    TurnIO().pfnMessage(app, iplr, idmBattleTookPlaceAgainstNeitherNorEnemys, static_cast<MsgGoto>(idBtl | 0x4000), x, y, iThem, i, cThem, cThemDead, 0);
                } else if (cThem == 1) {
                    TurnIO().pfnMessage(app, iplr, idmBattleTookPlaceAgainstNeitherForcesNor2, static_cast<MsgGoto>(idBtl | 0x4000), x, y, iThem, cUs, j, cUsDead, 0);
                } else {
                    TurnIO().pfnMessage(app, iplr, idmBattleTookPlaceAgainstNeitherForcesNor, static_cast<MsgGoto>(idBtl | 0x4000), x, y, iThem, cUs, cThem, cUsDead, cThemDead);
                }
                if (cUsDead == cUs || (lppl && lppl->iPlayer != iplrNone && iplr != lppl->iPlayer))
                    continue;
                ITechLearnATech(app, iplr, x, y, idmWreckageDiscoveredBattleHasBoostedResearchResour, nullptr);
                continue;
            }
            if (cUsDead == 0) {
                if (cThem == cThemDead) {
                    TurnIO().pfnMessage(app, iplr, idmBattleTookPlaceInvolvingRacesForcesDestroyed, static_cast<MsgGoto>(idBtl | 0x4000), x, y, cplr, cUs, 0, 0, 0);
                } else {
                IndecisiveXWay:
                    TurnIO().pfnMessage(app, iplr, idmBattleTookPlaceInvolvingRacesLostForces2, static_cast<MsgGoto>(idBtl | 0x4000), x, y, cplr, cUsDead, cUs, cThemDead, cThem);
                }
            } else if (cThemDead == 0) {
                if (cUs != cUsDead)
                    goto IndecisiveXWay;
                TurnIO().pfnMessage(app, iplr, idmBattleTookPlaceInvolvingRacesEntireArmada, static_cast<MsgGoto>(idBtl | 0x4000), x, y, cplr, cUs, cThem, 0, 0);
            } else if (cThemDead == cThem) {
                TurnIO().pfnMessage(app, iplr, idmBattleTookPlaceInvolvingRacesLostForces, static_cast<MsgGoto>(idBtl | 0x4000), x, y, cplr, cUsDead, cUs, 0, 0);
            } else if (cUsDead == cUs) {
                TurnIO().pfnMessage(app, iplr, idmBattleTookPlaceInvolvingRacesEntireArmada2, static_cast<MsgGoto>(idBtl | 0x4000), x, y, cplr, cUs, cThem, cThemDead, 0);
            } else {
                TurnIO().pfnMessage2(app, iplr, idmBattleTookPlacePressGotoButtonView, static_cast<MsgGoto>(idBtl | 0x4000), x, y);
            }
            if (fAlive && (!lppl || lppl->iPlayer == iplrNone || iplr == lppl->iPlayer)) {
                ITechLearnATech(app, iplr, x, y, idmWreckageDiscoveredBattleHasBoostedResearchResour, nullptr);
            }
        } else if (lppl && lppl->iPlayer == iplr) {
            TurnIO().pfnMessage2(app, iplr, idmColonyReportsBattleTookPlaceOrbitForces, static_cast<MsgGoto>(lppl->id), lppl->id, 0);
            ITechLearnATech(app, iplr, x, y, idmFleetFoundWreckageBattleWhichHasBoosted, nullptr);
        } else if (iplr & grfSpectator) {
            lpfl = lpflBtl;
            while (lpfl->iPlayer != iplr) {
                lpfl = lpfl->lpflNext;
                if (lpfl == lpflBtl || !lpfl)
                    break;
            }
            if (lpfl && lpfl->iPlayer == iplr) {
                TurnIO().pfnMessage(app, iplr, idmReportsBattleTookPlaceForcesInvolved, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lpfl->pt.x, lpfl->pt.y, 0, 0, 0, 0);
                ITechLearnATech(app, iplr, x, y, idmWreckageBattleOccurredOrbitHasBoostedResearch, nullptr);
            }
        }
        if (1 << iplr & app.state.grfMissed) {
            lpfl = lpflBtl;
            while (lpfl->iPlayer != iplr || !lpfl->fSkipped()) {
                lpfl = lpfl->lpflNext;
                if (lpfl == lpflBtl || !lpfl)
                    break;
            }
            if (lpfl && lpfl->iPlayer == iplr && lpfl->fSkipped()) {
                TurnIO().pfnMessage2(app, iplr, idmDueExcessiveFleetManeuveringBattleAreaFleets, static_cast<MsgGoto>(lpfl->id | 0x8000), x, y);
            }
        }
    }
    return;
}

int16_t FAttackPlayer(APPSESSION &app, FLEET *lpfl, int16_t iplr) {
    int16_t iplrCur;
    int16_t iplrT;

    iplrCur = lpfl->iPlayer;
    iplrT = app.state.rglpbtlplan[iplrCur][lpfl->iplan].iplrAttack;
    switch (iplrT) {
    case 0:
        return 0;
    case 3:
        return 1;
    case 2:
        if (app.state.rgplr[iplrCur].rgmdRelation[iplr] != 1) {
            return 1;
        }
        return 0;
    case 1:
        if (app.state.rgplr[iplrCur].rgmdRelation[iplr] == 2) {
            return 1;
        }
        return 0;
    default:
        if (iplr == iplrT - 4) {
            return 1;
        }
        return 0;
    }
}

void DoBombing(APPSESSION &app) {
    MessageId idmDst;
    int32_t   modKill;
    int16_t   fMulti;
    int32_t   cKillPeople;
    int32_t   dmgBombBldg;
    int32_t   cKillPeopleS;
    int32_t   cKillMine;
    int32_t   dmgBombFloor;
    MessageId idmSrc;
    int32_t   cKillDefenses;
    int32_t   cKillFact;
    int32_t   pctTerra;
    PLANET   *lppl;
    int16_t   ifl;
    FLEET    *lpfl;
    int32_t   cPPE;
    int32_t   dmgBombPeople;
    float     pctSmart;
    float     pctSuccess;
    int32_t   dmgPeopleSmart;
    double    pctSuccessHalf;
    int16_t   pctTot;
    int16_t   dChg;
    int16_t   i;

    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (!lpfl->fDead && lpfl->idPlanet != idPlanetDeepSpace && !lpfl->fBombed) {
            lppl = app.state.lpPlanets + lpfl->idPlanet;
            if (lppl->iPlayer != lpfl->iPlayer && lppl->iPlayer != iplrNone && FAttackPlayer(app, lpfl, lppl->iPlayer) && !lppl->fStarbase &&
                FCalcFleetBombDamage(app, lpfl, &dmgBombPeople, &dmgBombFloor, &dmgPeopleSmart, &dmgBombBldg, &pctTerra, &fMulti)) {
                CalcPctSurvive(app, lppl, &pctSuccess, &pctSmart);
                if ((double)pctSuccess < (double)1.0) {
                    if (dmgBombPeople > 0) {
                        dmgBombPeople = (int32_t)((double)dmgBombPeople * pctSuccess + 0.5);
                    }
                    if (dmgBombFloor > 0) {
                        dmgBombFloor = (int32_t)((double)dmgBombFloor * pctSuccess + 0.5);
                    }
                    if (dmgPeopleSmart > 0) {
                        dmgPeopleSmart = (int32_t)((double)dmgPeopleSmart * pctSmart + 0.5);
                    }
                    if (dmgBombBldg > 0) {
                        pctSuccessHalf = (double)(1.0 - ((double)1.0 - pctSuccess) / 2.0);
                        dmgBombBldg = (int32_t)((double)dmgBombBldg * pctSuccessHalf + 0.5);
                    }
                }
                cPPE = lppl->cMines + lppl->cFactories + (uint32_t)lppl->cDefenses;
                cKillDefenses = 0;
                cKillPeople = 0;
                cKillMine = 0;
                cKillFact = 0;
                if (dmgBombBldg > 0 && cPPE > 0) {
                    cKillFact = (uint32_t)(lppl->cFactories * dmgBombBldg);
                    modKill = (int32_t)(cKillFact % cPPE);
                    cKillFact = (int32_t)(cKillFact / cPPE);
                    if (modKill > 0) {
                        cKillFact += (uint32_t)(RandomIO().pfnRandom(app, LOWORD(cPPE)) < modKill);
                    }
                    if (cKillFact > (int32_t)lppl->cFactories) {
                        cKillFact = lppl->cFactories;
                    }
                    cKillDefenses = (uint32_t)(lppl->cDefenses * dmgBombBldg);
                    modKill = (int32_t)(cKillDefenses % cPPE);
                    cKillDefenses = (int32_t)(cKillDefenses / cPPE);
                    if (modKill > 0) {
                        cKillDefenses += (uint32_t)(RandomIO().pfnRandom(app, LOWORD(cPPE)) < modKill);
                    }
                    if (cKillDefenses > (int32_t)lppl->cDefenses) {
                        cKillDefenses = lppl->cDefenses;
                    }
                    cKillMine = dmgBombBldg - (cKillFact + cKillDefenses);
                    if (cKillMine > (int32_t)lppl->cMines) {
                        cKillMine = lppl->cMines;
                    }
                }
                if ((dmgBombPeople > 0 || dmgBombFloor > 0 || dmgPeopleSmart > 0) && lppl->rgwtMin[3] > 0) {
                    cKillPeopleS = (int32_t)(lppl->rgwtMin[3] * dmgPeopleSmart) / 1000;
                    if (cKillPeopleS >= lppl->rgwtMin[3]) {
                        cKillPeopleS = lppl->rgwtMin[3] - 1;
                    }
                    cKillPeople = (uint32_t)((lppl->rgwtMin[3] - cKillPeopleS) * dmgBombPeople);
                    modKill = (int32_t)(cKillPeople % 1000);
                    cKillPeople = (int32_t)(cKillPeople / 1000);
                    if (modKill > 0) {
                        cKillPeople += (uint32_t)(RandomIO().pfnRandom(app, 1000) <= modKill);
                    }
                    cKillPeople += cKillPeopleS;
                    if (dmgBombPeople > 0 && cKillPeople <= 0) {
                        cKillPeople = 1;
                    }
                    if (cKillPeople < dmgBombFloor) {
                        cKillPeople = dmgBombFloor;
                    }
                    if (cKillPeople > lppl->rgwtMin[3]) {
                        cKillPeople = lppl->rgwtMin[3];
                    }
                }
                if (cKillPeople > 0) {
                    lppl->rgwtMin[3] -= cKillPeople;
                }
                if (cKillFact > 0) {
                    lppl->cFactories -= cKillFact;
                }
                if (cKillMine > 0) {
                    lppl->cMines -= cKillMine;
                }
                if (cKillDefenses > 0) {
                    lppl->cDefenses -= cKillDefenses;
                }
                if (pctTerra > 0) {
                    pctTot = 0;
                    pctTerra -= (int32_t)(((double)1.0 - pctSuccess) * pctTerra / 2);
                    if (pctTerra > 500) {
                        pctTerra = 500;
                    }
                    for (i = 0; i < 3; i++) {
                        dChg = lppl->rgEnvVar[i] - lppl->rgEnvVarOrig[i];
                        if (dChg > 0) {
                            if (dChg >= pctTerra) {
                                dChg = LOWORD(pctTerra);
                            }
                            lppl->rgEnvVar[i] -= dChg;
                            pctTot += dChg;
                        } else if (dChg < 0) {
                            if ((int16_t)-dChg >= pctTerra) {
                                dChg = -LOWORD(pctTerra);
                            }
                            lppl->rgEnvVar[i] -= dChg;
                            pctTot += -dChg;
                        }
                    }
                    if (pctTot > 0) {
                        TurnIO().pfnMessage(app, lpfl->iPlayer, !fMulti ? idmHasRetroBombedUndoingTerraforming : idmFleetsHaveRetroBombedUndoingTerraforming,
                                    static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lppl->id, pctTot, 0, 0, 0, 0);
                        TurnIO().pfnMessage(app, lppl->iPlayer, !fMulti ? idmHasRetroBombedUndoingTerraforming : idmFleetsHaveRetroBombedUndoingTerraforming2, static_cast<MsgGoto>(lppl->id),
                                    lpfl->id, lppl->id, pctTot, 0, 0, 0, 0);
                    }
                }
                cPPE = cKillMine + cKillFact + cKillDefenses;
                if (cPPE > 0) {
                    if (lppl->rgwtMin[3] > 0) {
                        idmSrc = !fMulti ? idmHasBombedKillingColonistsDestroyingOneInstallati : idmFleetsHaveBombedKillingColonistsDestroyingOne;
                        idmDst = !fMulti ? idmHasBombedKillingColonistsDestroyingOneInstallati3 : idmFleetsHaveBombedKillingColonistsDestroyingOne3;
                        if (cPPE > 1) {
                            idmSrc = static_cast<MessageId>(idmSrc + 1);
                            idmDst = static_cast<MessageId>(idmDst + 1);
                        }
                    } else {
                        idmSrc = !fMulti ? idmHasBombedKillingOffEnemyColonists : idmFleetsHaveBombedKillingOffEnemyColonists;
                        idmDst = !fMulti ? idmHasBombedKillingColonists3 : idmFleetsHaveBombedKillingColonists3;
                        goto GenericBombMsg;
                    }
                    if (cKillPeople > 0) {
                        if ((double)pctSuccess == (double)1.0) {
                        GenericBombMsg:
                            TurnIO().pfnMessage(app, lpfl->iPlayer, idmSrc, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lppl->id, LOWORD(cKillPeople), LOWORD(cPPE), 0, 0, 0);
                            TurnIO().pfnMessage(app, lppl->iPlayer, idmDst, static_cast<MsgGoto>(lppl->id), lpfl->id, lppl->id, LOWORD(cKillPeople), LOWORD(cPPE), 0, 0, 0);
                        } else {
                            idmSrc = static_cast<MessageId>(idmSrc + 5);
                            idmDst = static_cast<MessageId>(idmDst + 5);
                            TurnIO().pfnMessage(app, lpfl->iPlayer, idmSrc, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lppl->id, LOWORD(cKillPeople), LOWORD(cPPE),
                                        (int32_t)(((double)1.0 - pctSuccess) * 10000), 0, 0);
                            TurnIO().pfnMessage(app, lppl->iPlayer, idmDst, static_cast<MsgGoto>(lppl->id), lpfl->id, lppl->id, LOWORD(cKillPeople), LOWORD(cPPE),
                                        (int32_t)(((double)1.0 - pctSuccess) * 10000), 0, 0);
                        }
                    } else {
                        idmSrc = static_cast<MessageId>(idmSrc - 2);
                        idmDst = static_cast<MessageId>(idmDst - 2);
                        if ((double)pctSuccess == (double)1.0) {
                            TurnIO().pfnMessage(app, lpfl->iPlayer, idmSrc, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lppl->id, LOWORD(cPPE), 0, 0, 0, 0);
                            TurnIO().pfnMessage(app, lppl->iPlayer, idmDst, static_cast<MsgGoto>(lppl->id), lpfl->id, lppl->id, LOWORD(cPPE), 0, 0, 0, 0);
                        } else {
                            idmSrc = static_cast<MessageId>(idmSrc + 5);
                            idmDst = static_cast<MessageId>(idmDst + 5);
                            TurnIO().pfnMessage(app, lpfl->iPlayer, idmSrc, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lppl->id, LOWORD(cPPE),
                                        (int32_t)(((double)1.0 - pctSuccess) * 10000), 0, 0, 0);
                            TurnIO().pfnMessage(app, lppl->iPlayer, idmDst, static_cast<MsgGoto>(lppl->id), lpfl->id, lppl->id, LOWORD(cPPE), (int32_t)(((double)1.0 - pctSuccess) * 10000),
                                        0, 0, 0);
                        }
                    }
                } else if (cKillPeople > 0) {
                    if (lppl->rgwtMin[3] > 0) {
                        idmSrc = !fMulti ? idmHasBombedKillingColonists : idmFleetsHaveBombedKillingColonists;
                        idmDst = !fMulti ? idmHasBombedKillingColonists2 : idmFleetsHaveBombedKillingColonists2;
                    } else {
                        idmSrc = !fMulti ? idmHasBombedKillingOffEnemyColonists : idmFleetsHaveBombedKillingOffEnemyColonists;
                        idmDst = !fMulti ? idmHasBombedKillingColonists3 : idmFleetsHaveBombedKillingColonists3;
                    }
                    TurnIO().pfnMessage(app, lpfl->iPlayer, idmSrc, static_cast<MsgGoto>(lpfl->id | 0x8000), lpfl->id, lppl->id, LOWORD(cKillPeople), 0, 0, 0, 0);
                    TurnIO().pfnMessage(app, lppl->iPlayer, idmDst, static_cast<MsgGoto>(lppl->id), lpfl->id, lppl->id, LOWORD(cKillPeople), 0, 0, 0, 0);
                }
                if (lppl->rgwtMin[3] == 0) {
                    UninhabitPlanet(app, lppl);
                }
            }
        }
    }
    return;
}

int16_t CBattles(APPSESSION &app) {
    uint8_t *lpbd;
    HB      *lphb;
    int16_t  cBattles;

    cBattles = 0;
    lphb = app.state.rglphb[11];
    if (!lphb) {
        return 0;
    }
    lpbd = (uint8_t *)((uint8_t *)lphb + (sizeof(HB) + 2));
    while (1) {
        if (BdvFromLp(app, lpbd).id() == 0xffff) {
            lphb = lphb->lphbNext;
            if (!lphb || lphb->ibTop <= sizeof(HB))
                break;
            lpbd = (uint8_t *)((uint8_t *)lphb + (sizeof(HB) + 2));
        } else {
            if (BdvFromLp(app, lpbd).cbData() == 0) {
                return cBattles;
            }
            lpbd = (uint8_t *)((uint8_t *)lpbd + BdvFromLp(app, lpbd).cbData());
            cBattles++;
        }
    }
    return cBattles;
}
