#include "common.h"
#include "appstate.h"
#include "production_io.h"
#include "planet_query_io.h"
#include "part_query_io.h"

HullSlotType rggrbitPartsSB[8] = {hstEnabledSB, hstArmor, hstBeam, hstSpecialE, hstSpecialSB, hstShield, hstTorp, hstWeapon};
StringId     rgidsPartsSB[8] = {idsAll, idsArmor3, idsBeamWeapons, idsElectrical, idsOrbital, idsShields3, idsTorpedoes, idsWeapons2};
HullSlotType rggrbitParts[13] = {hstEnabled, hstArmor,  hstBeam,    hstBomb,   hstSpecialE, hstEngine, hstSpecialM,
                                 hstMines,   hstMining, hstScanner, hstShield, hstTorp,     hstWeapon};
StringId     rgidsParts[13] = {idsAll,        idsArmor3,       idsBeamWeapons, idsBombs,    idsElectrical, idsEngines, idsMechanical,
                               idsMineLayers, idsMiningRobots, idsScanners,    idsShields3, idsTorpedoes,  idsWeapons2};
HullSlotType rghstCat[14] = {hstWeapon, hstSpecialEM, hstArmor,  hstBeam,     hstBomb,     hstEngine, hstMines,
                             hstMining, hstScanner,   hstShield, hstSpecialE, hstSpecialM, hstTorp,   hstSpecialSB};
StringId     rgidsCat[14] = {idsWeapons2,     idsDevices,  idsArmor3,   idsBeamWeapons, idsBombs,      idsEngines,   idsMineLayers,
                             idsMiningRobots, idsScanners, idsShields3, idsElectrical,  idsMechanical, idsTorpedoes, idsOrbital};

int16_t PctJammerFromHul(APPSESSION &app, HUL *lphul) {
    int32_t pctJam;
    int16_t ihs;
    int16_t i;
    int32_t pctHit;
    PART    part;

    pctHit = 10000;
    for (ihs = 0; ihs < lphul->chs; ihs++) {
        part.hs = lphul->rghs[ihs];
        if (part.hs.grhst == hstSpecialE) {
            if (part.hs.iItem >= ispecialEJammer10 && part.hs.iItem <= ispecialEJammer50) {
                PartQueryIO().pfnLookup(app, &part);
                pctJam = static_cast<int16_t>((100 - part.pspecial()->grAbility));
            } else if (part.hs.iItem == ispecialEMultiFunctionPod) {
                pctJam = 90;
            } else {
                pctJam = 100;
            }
        } else if (part.hs.grhst == hstArmor && part.hs.iItem == iarmorMegaPolyShell) {
            pctJam = 80;
        } else if (part.hs.grhst == hstMining && part.hs.iItem == iminingAlienMiner) {
            pctJam = 70;
        } else if (part.hs.grhst == hstShield && part.hs.iItem == ishieldLangstonShell) {
            pctJam = 95;
        } else {
            pctJam = 100;
        }
        if (pctJam < 100) {
            for (i = part.hs.cItem; i > 0; i--) {
                pctHit = static_cast<uint32_t>((pctHit * pctJam));
                pctHit = static_cast<int32_t>((pctHit / 100));
            }
        }
    }
    if (pctHit < 100) {
        pctHit = 100;
    }
    pctJam = static_cast<int16_t>((100 - static_cast<int16_t>((LOWORD(pctHit) + 50)) / 100));
    if (static_cast<int16_t>(lphul->ihuldef) > ihuldefOrbitalFort) {
        pctJam -= static_cast<int32_t>((pctJam / 4));
    }
    if (pctJam > 95) {
        pctJam = 95;
    }
    return LOWORD(pctJam);
}

SHDEF *NthValidShdef(APPSESSION &app, int16_t n) {
    int16_t i;

    if (app.state.fStarbaseMode) {
        for (i = 0; i < 10; i++) {
            if (!app.state.rglpshdefSB[app.state.idPlayer][i].fFree && n-- == 0) {
                return app.state.rglpshdefSB[app.state.idPlayer] + i;
            }
        }
    } else {
        for (i = 0; i < 16; i++) {
            if (!app.state.rgshdef[i].fFree && n-- == 0) {
                return &app.state.rgshdef[i];
            }
        }
    }
    return nullptr;
}

SHDEF *NthValidEnemyShdef(APPSESSION &app, int16_t n) {
    int16_t i;
    int16_t j;

    if (app.state.fStarbaseMode) {
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (app.state.rglpshdefSB[i] && i != app.state.idPlayer) {
                for (j = 0; j < 10; j++) {
                    if (!app.state.rglpshdefSB[i][j].fFree && n-- == 0) {
                        return app.state.rglpshdefSB[i] + j;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (app.state.rglpshdef[i] && i != app.state.idPlayer) {
                for (j = 0; j < 16; j++) {
                    if (!app.state.rglpshdef[i][j].fFree && n-- == 0) {
                        return app.state.rglpshdef[i] + j;
                    }
                }
            }
        }
    }
    return nullptr;
}

void KillQueuedMassPackets(APPSESSION &app, PLANET *lppl) {
    int16_t iprod;
    int16_t iDst;
    PROD   *lpprod;

    if (lppl->lpplprod && lppl->lpplprod->iprodMac != 0) {
        iDst = 0;
        iprod = 0;
        lpprod = lppl->lpplprod->rgprod();
        while (iprod < lppl->lpplprod->iprodMac) {
            if (lpprod->grobj != grobjPlanet || lpprod->iItem < iobjPacketIron || lpprod->iItem > iobjPacketMixed) {
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
        if (app.state.sel.grobj == grobjPlanet && app.state.sel.pl.id == lppl->id) {
            PlanetQueryIO().pfnLookup(app, app.state.sel.pl.id, &app.state.sel.pl);
            ProductionIO().pfnRefresh(app);
        }
    }
    return;
}

void KillQueuedShips(APPSESSION &app, PLANET *lppl) {
    int16_t iprod;
    int16_t iDst;
    PROD   *lpprod;

    if (lppl->lpplprod && lppl->lpplprod->iprodMac != 0) {
        iDst = 0;
        iprod = 0;
        lpprod = lppl->lpplprod->rgprod();
        while (iprod < lppl->lpplprod->iprodMac) {
            if (lpprod->grobj != grobjFleet || lpprod->iItem >= iobjPacketGerm) {
                if (lpprod->grobj == grobjFleet) {
                    lpprod->pct = 0;
                }
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
        if (app.state.sel.grobj == grobjPlanet && app.state.sel.pl.id == lppl->id) {
            PlanetQueryIO().pfnLookup(app, app.state.sel.pl.id, &app.state.sel.pl);
            ProductionIO().pfnRefresh(app);
        }
    }
    return;
}
