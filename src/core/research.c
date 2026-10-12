#include "common.h"

uint16_t rggrbitBrParts[17] = {6655, 8, 16, 64, 2048, 1, 4096, 256, 128, 512, 32768, 2, 4, 16384, 1024, 8192, 32};
int32_t  rglTechCost[27] = {0,     50,    80,    130,   210,   340,   550,   890,   1440,  2330,  3770,  6100,  9870, 13850,
                            18040, 22440, 27050, 31870, 36900, 42140, 47590, 53250, 59120, 65200, 71490, 77990, 84700};

int32_t GetTechLevelCost(TechFieldType iTech, int16_t iLevel, int16_t iplr) {
    int32_t lCost;
    int16_t i;
    int16_t cTech;

    cTech = 0;
    for (i = 0; i < 6; i++) {
        cTech += rgplr[iplr].rgTech[i];
    }
    lCost = (int16_t)(10 * cTech) + rglTechCost[iLevel];
    i = GetRaceStat(&rgplr[iplr], iTech + 8) - 1;
    if (i != 0) {
        if (i < 0) {
            lCost += lCost - (int32_t)(lCost >> 2);
        } else {
            lCost = (int32_t)(lCost / 2);
        }
    }
    if (game.fSlowTech) {
        lCost = (int32_t)(lCost * 2);
    }
    return lCost;
}

int32_t ProjectedResearchSpending(int32_t pct) {
    int32_t lRes;
    PLANET *lppl;
    int16_t cRes;
    int32_t lSpend;
    PLANET *lpplMac;
    char    pctSav;
    int16_t cBogus;

    lSpend = 0;
    pctSav = rgplr[idPlayer].pctResearch;
    rgplr[idPlayer].pctResearch = pct;
    lppl = lpPlanets;
    lpplMac = lpPlanets + cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == idPlayer) {
            lRes = CResourcesAtPlanet(lppl, idPlayer);
            if (!lppl->lpplprod || lppl->lpplprod->iprodMac == 0) {
                lSpend += lRes;
            } else {
                EstimateItemProdSched(lppl, NULL, iprodEstimateResearchResources, &cRes, &cBogus);
                lSpend += cRes;
            }
        }
    }
    rgplr[idPlayer].pctResearch = pctSav;
    return lSpend;
}

int32_t CostOfDevelopingItem(char *rgTech) {
    int32_t lSpent;
    char   *pTech;
    char    rgTechSav[6];
    int32_t lCost;
    int16_t fUnreachable;
    int16_t i;
    int32_t lCur;

    fUnreachable = FALSE;
    lCost = 0;
    pTech = rgplr[idPlayer].rgTech;
    for (i = 0; i < 6 && rgTech[i] <= 26; i++) {
    }
    if (i < 6) {
        return -1;
    }
    for (i = 0; i < 6; i++) {
        rgTechSav[i] = pTech[i];
        if (rgTech[i] > pTech[i]) {
            lSpent = rgplr[idPlayer].rgResSpent[i];
            if (game.fSlowTech) {
                lSpent = (int32_t)(lSpent * 2);
            }
            lCur = -lSpent;
            while (rgTech[i] > pTech[i]) {
                lCur += GetTechLevelCost(i, pTech[i] + 1, idPlayer);
                pTech[i]++;
            }
            lCost += max(0, lCur);
        }
    }
    for (i = 0; i < 6; i++) {
        pTech[i] = rgTechSav[i];
    }
    return lCost;
}

int16_t FShouldPartBeHidden(PART *ppart) {
    int16_t     iItem;
    GrbitTrader grbitTrader;

    if (idPlayer == iplrNone) {
        return FALSE;
    }
    grbitTrader = grbitTraderNone;
    iItem = ppart->hs.iItem;
    switch (ppart->hs.grhst) {
    case hstBeam:
        if (iItem != ibeamMultiContainedMunition)
            break;
        grbitTrader = grbitTraderBeam;
        break;
    case hstTorp:
        if (iItem != itorpAntiMatterTorpedo)
            break;
        grbitTrader = grbitTraderTorp;
        break;
    case hstArmor:
        if (iItem != iarmorMegaPolyShell)
            break;
        grbitTrader = grbitTraderArmor;
        break;
    case hstShield:
        if (iItem != ishieldLangstonShell)
            break;
        grbitTrader = grbitTraderShield;
        break;
    case hstBomb:
        if (iItem != ibombHushABoom)
            break;
        grbitTrader = grbitTraderBomb;
        break;
    case hstMining:
        if (iItem != iminingAlienMiner)
            break;
        grbitTrader = grbitTraderMiner;
        break;
    case hstEngine:
        if (iItem != iengineEnigmaPulsar)
            break;
        grbitTrader = grbitTraderEngine;
        break;
    case hstHull:
        if (iItem != ihuldefMiniMorph)
            break;
        grbitTrader = grbitTraderHull;
        break;
    case hstSpecialE:
        if (iItem != ispecialEMultiFunctionPod)
            break;
        grbitTrader = grbitTraderSpecial;
        break;
    case hstSpecialM:
        if (iItem == ispecialMMultiCargoPod) {
            grbitTrader = grbitTraderCargo;
            break;
        }
        if (iItem != ispecialMJumpGate)
            break;
        grbitTrader = grbitTraderJumpgate;
        break;
    case hstPlanetary:
        if (iItem == iplanetaryGenesisDevice) {
            grbitTrader = grbitTraderGenesis;
        }
    }
    if (grbitTrader != grbitTraderNone && !(rgplr[idPlayer].grbitTrader & grbitTrader)) {
        return TRUE;
    }
    return FALSE;
}
