#include "common.h"

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

int16_t PctJammerFromHul(HUL *lphul) {
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
                FLookupPart(&part);
                pctJam = (int16_t)(100 - part.pspecial->grAbility);
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
                pctHit = (uint32_t)(pctHit * pctJam);
                pctHit = (int32_t)(pctHit / 100);
            }
        }
    }
    if (pctHit < 100) {
        pctHit = 100;
    }
    pctJam = (int16_t)(100 - (int16_t)(LOWORD(pctHit) + 50) / 100);
    if ((int16_t)lphul->ihuldef > ihuldefOrbitalFort) {
        pctJam -= (int32_t)(pctJam / 4);
    }
    if (pctJam > 95) {
        pctJam = 95;
    }
    return LOWORD(pctJam);
}

SHDEF *NthValidShdef(int16_t n) {
    int16_t i;

    if (fStarbaseMode) {
        for (i = 0; i < 10; i++) {
            if (!rglpshdefSB[idPlayer][i].fFree && n-- == 0) {
                return rglpshdefSB[idPlayer] + i;
            }
        }
    } else {
        for (i = 0; i < 16; i++) {
            if (!rgshdef[i].fFree && n-- == 0) {
                return &rgshdef[i];
            }
        }
    }
    return NULL;
}

SHDEF *NthValidEnemyShdef(int16_t n) {
    int16_t i;
    int16_t j;

    if (fStarbaseMode) {
        for (i = 0; i < game.cPlayer; i++) {
            if (rglpshdefSB[i] && i != idPlayer) {
                for (j = 0; j < 10; j++) {
                    if (!rglpshdefSB[i][j].fFree && n-- == 0) {
                        return rglpshdefSB[i] + j;
                    }
                }
            }
        }
    } else {
        for (i = 0; i < game.cPlayer; i++) {
            if (rglpshdef[i] && i != idPlayer) {
                for (j = 0; j < 16; j++) {
                    if (!rglpshdef[i][j].fFree && n-- == 0) {
                        return rglpshdef[i] + j;
                    }
                }
            }
        }
    }
    return NULL;
}

void KillQueuedMassPackets(PLANET *lppl) {
    int16_t iprod;
    int16_t iDst;
    PROD   *lpprod;

    if (lppl->lpplprod && lppl->lpplprod->iprodMac != 0) {
        iDst = 0;
        iprod = 0;
        lpprod = lppl->lpplprod->rgprod;
        while (iprod < lppl->lpplprod->iprodMac) {
            if (lpprod->grobj != grobjPlanet || lpprod->iItem < iobjPacketIron || lpprod->iItem > iobjPacketMixed) {
                if (iDst != iprod) {
                    lppl->lpplprod->rgprod[iDst] = *lpprod;
                }
                iDst++;
            }
            iprod++;
            lpprod++;
        }
        if (iDst == 0) {
            FreePl((PL *)lppl->lpplprod);
            lppl->lpplprod = NULL;
        } else if (iDst != iprod) {
            lppl->lpplprod->iprodMac = iDst;
        }
        if (sel.grobj == grobjPlanet && sel.pl.id == lppl->id) {
            FLookupPlanet(sel.pl.id, &sel.pl);
            FillSelProdLB();
        }
    }
    return;
}

void KillQueuedShips(PLANET *lppl) {
    int16_t iprod;
    int16_t iDst;
    PROD   *lpprod;

    if (lppl->lpplprod && lppl->lpplprod->iprodMac != 0) {
        iDst = 0;
        iprod = 0;
        lpprod = lppl->lpplprod->rgprod;
        while (iprod < lppl->lpplprod->iprodMac) {
            if (lpprod->grobj != grobjFleet || lpprod->iItem >= iobjPacketGerm) {
                if (lpprod->grobj == grobjFleet) {
                    lpprod->pct = 0;
                }
                if (iDst != iprod) {
                    lppl->lpplprod->rgprod[iDst] = *lpprod;
                }
                iDst++;
            }
            iprod++;
            lpprod++;
        }
        if (iDst == 0) {
            FreePl((PL *)lppl->lpplprod);
            lppl->lpplprod = NULL;
        } else if (iDst != iprod) {
            lppl->lpplprod->iprodMac = iDst;
        }
        if (sel.grobj == grobjPlanet && sel.pl.id == lppl->id) {
            FLookupPlanet(sel.pl.id, &sel.pl);
            FillSelProdLB();
        }
    }
    return;
}
