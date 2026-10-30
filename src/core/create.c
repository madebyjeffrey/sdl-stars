#include "common.h"

int16_t vrgvcMax[10] = {16, 18, 4, 19, 28, 49, 29, 87, 7, 47};
char    rgNG3Width[9][2] = {{-3}, {2, 1}, {5}, {-3}, {3}, {3}, {3}, {1}, {3}};
uint8_t vrgWormholeMin[5] = {0, 1, 1, 3, 4};
uint8_t vrgWormholeVar[5] = {3, 3, 5, 4, 5};

void InitBattlePlan(BTLPLAN *lpbtlplan, int16_t iplan, int16_t iplr) {
    *lpbtlplan = rgbtlplanT[iplan];
    lpbtlplan->iplr = iplr;
    if (game.fSinglePlr && iplan == 0) {
        lpbtlplan->iplrAttack = iplrAttackEveryone;
    }
    return;
}

int16_t GenerateWorld(int16_t fBatchMode) {
    int32_t      *pl;
    int16_t       iBest;
    int16_t       cKill;
    char          grUsed[128];
    POINT16      *ppt;
    RaceAttribute raMajor;
    int16_t       k;
    POINT16       pt;
    int16_t       fFound;
    int16_t       iMax;
    STARPACK      starpack;
    std::byte     rgbGame[cbWireGame];
    std::byte     rgbStarpack[cbWireStarpack];
    int16_t       dy;
    int16_t       dGalMinSq;
    int16_t       iLow;
    PLANET       *lppl;
    int16_t       iMin;
    int16_t       i;
    int16_t       xOld;
    int16_t       iplrSingle;
    POINT16      *pptMax;
    int16_t       dMin;
    int16_t       ktLeft;
    SHDEF        *lpshdef;
    int32_t       lDistMax2;
    int32_t       lDistIdeal2;
    int16_t       rgi[16];
    int16_t       iNewLine;
    uint8_t      *pb;
    int16_t       dMax;
    int32_t       lDistMin2;
    int16_t       j;
    int16_t       cPlanMax;
    int16_t       dx;
    int16_t       cKillMax;
    POINT16      *pptT;
    int32_t       lBest;
    int32_t       l;
    int16_t       iT;
    int16_t       jj;
    int16_t       iTechMin;
    int16_t       pct10;
    int16_t       idHome;
    int16_t       ishRet;
    PART          part;
    int16_t       cFit;
    PLANET       *lpplClosest;
    POINT16       ptHome;
    PLANET       *lpplPicked;
    int32_t       lDistCur2;
    HS           *lphs;
    int16_t       chs;
    int16_t       cTry;
    int16_t       rgTry[5];
    THING        *lpth;
    uint16_t      idLast;
    THING        *lpthLast;
    char          szExt[4];

    iMin = 0;
    cKill = 0;
    dGal = 400 * game.mdSize + 400;
    dGalInv = dGal + 2000;
    cPlanMax = LOWORD((int32_t)(dGal * dGal) / 5000);
    cPlanMax += cPlanMax / 4 * (game.mdDensity - 1);
    if ((int16_t)game.mdDensity >= densityPacked) {
        cPlanMax += cPlanMax / 4;
    }
    cPlanMax = cPlanMax >= 999 ? 999 : cPlanMax;
    dGalMinSq = dGalMinDist * dGalMinDist;
    iMax = cPlanMax / 7 + cPlanMax >= 999 ? 999 : cPlanMax / 7 + cPlanMax;
    dx = 1010;
    dy = dGal - 19;
    for (i = 0; i < iMax; i++) {
        rgptPlan[i].x = Random(dy) + dx;
        rgptPlan[i].y = Random(dy) + dx;
    }
    qsort16(rgptPlan, iMax, sizeof(POINT16), (QSORTCOMPARE)ICompLong);
    pptMax = &rgptPlan[iMax];
    for (ppt = rgptPlan; ppt < pptMax; ppt++) {
        if (ppt->y >= 0) {
            pptT = ppt + 1;
            iNewLine = ppt->x + dGalMinDist;
            for (; pptT < pptMax && pptT->x <= iNewLine; pptT++) {
                dy = abs(ppt->y - pptT->y);
                if (dy <= dGalMinDist) {
                    dx = ppt->x - pptT->x;
                    if (dx * dx + dy * dy <= dGalMinSq) {
                        pptT->y = -100;
                        cKill++;
                    }
                }
            }
        }
    }
    cKillMax = iMax - cPlanMax;
    while (cKill < cKillMax) {
        i = Random(iMax);
        if (rgptPlan[i].y >= 0) {
            rgptPlan[i].y = -100;
            cKill++;
        }
    }
    pptT = rgptPlan;
    for (ppt = rgptPlan; ppt < pptMax; ppt++) {
        if (ppt->y >= 0) {
            *pptT++ = *ppt;
        }
    }
    cPlanMax = iMax - cKill;
    if (game.fClumping) {
        for (i = 0; i < cPlanMax; i++) {
            lBest = 10000000;
            iBest = 0;
            j = Random(cPlanMax);
            pt = rgptPlan[j];
            for (k = 0; k < cPlanMax; k++) {
                if (k != j) {
                    dx = pt.x - rgptPlan[k].x;
                    dy = pt.y - rgptPlan[k].y;
                    l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                    if (l < lBest) {
                        lBest = l;
                        iBest = k;
                    }
                }
            }
            if (lBest > 144) {
                if (lBest > 1600) {
                    rgptPlan[j].x = (int16_t)(rgptPlan[iBest].x * 2 + rgptPlan[j].x) / 3;
                    rgptPlan[j].y = (int16_t)(rgptPlan[iBest].y * 2 + rgptPlan[j].y) / 3;
                } else if (lBest > 625) {
                    rgptPlan[j].x = (int16_t)(rgptPlan[j].x + rgptPlan[iBest].x) / 2;
                    rgptPlan[j].y = (int16_t)(rgptPlan[j].y + rgptPlan[iBest].y) / 2;
                } else if (lBest > 324) {
                    rgptPlan[j].x = (int16_t)(rgptPlan[j].x * 2 + rgptPlan[iBest].x) / 3;
                    rgptPlan[j].y = (int16_t)(rgptPlan[j].y * 2 + rgptPlan[iBest].y) / 3;
                } else {
                    rgptPlan[j].x = (int16_t)(rgptPlan[j].x * 4 + rgptPlan[iBest].x) / 5;
                    rgptPlan[j].y = (int16_t)(rgptPlan[j].y * 4 + rgptPlan[iBest].y) / 5;
                }
            }
        }
        qsort16(rgptPlan, cPlanMax, sizeof(POINT16), (QSORTCOMPARE)ICompLong);
    }
    memset(grUsed, 0, 128);
    for (i = 0; i < cPlanMax; i++) {
        dx = Random(cPlanetName);
        while (grUsed[dx >> 3] & bitTbl[dx & 7]) {
            dx++;
            if (dx >= game.fTutorial + cPlanetName) {
                dx = 0;
            }
        }
        grUsed[dx >> 3] = grUsed[dx >> 3] | bitTbl[dx & 7];
        rgidPlan[i] = dx;
    }
    cPlanet = cPlanMax;
    StarsRecoveryScope scopeRecovery;
    try {
        lpPlanets = (PLANET *)LpAlloc(cPlanMax * sizeof(PLANET), htPlanets);
        memset(lpPlanets, 0, cPlanMax * sizeof(PLANET));
        i = 0;
        lppl = lpPlanets;
        while (i < cPlanMax) {
            lppl->id = i;
            lppl->iPlayer = iplrNone;
            lppl->det = detAll;
            lppl->iScanner = 31;
            if (!game.fNoRandom) {
                lppl->fArtifact = (uint32_t)(Random(3) == 0) & 1;
            }
            lppl->rgEnvVar[0] = Random(90) + 1;
            lppl->rgEnvVar[0] += Random(10);
            lppl->rgEnvVarOrig[0] = lppl->rgEnvVar[0];
            lppl->rgEnvVar[1] = Random(90) + 1;
            lppl->rgEnvVar[1] += Random(10);
            lppl->rgEnvVarOrig[1] = lppl->rgEnvVar[1];
            lppl->rgEnvVar[2] = lppl->rgEnvVarOrig[2] = Random(99) + 1;
            if (game.fTutorial) {
                if (i != 5) {
                    if (i == 11) {
                        lppl->rgEnvVar[0] += 20;
                        lppl->rgEnvVarOrig[0] = lppl->rgEnvVar[0];
                    }
                } else {
                    for (j = 0; j < 3; j++) {
                        lppl->rgEnvVar[j] -= 5;
                        lppl->rgEnvVarOrig[j] = lppl->rgEnvVar[j];
                    }
                }
            }
            for (j = 0; j < 3; j++) {
                if (game.fExtraFuel) {
                    lppl->rgMinConc[j] = 100;
                } else {
                    lppl->rgwtMin[j] = 0;
                    lppl->rgMinConc[j] = Random(45) + Random(45) + 31;
                    if (lppl->rgEnvVar[2] >= 90) {
                        lppl->rgMinConc[j] += Random(99 - lppl->rgMinConc[j]) / 2;
                    }
                }
                lppl->rgpctMinLevel[j] = 0;
                lppl->rgwtMin[j] = 0;
                if (game.fBBSPlay && lppl->rgMinConc[j] < 40) {
                    lppl->rgMinConc[j] += 5;
                }
            }
            if (game.fExtraFuel) {
                iT = 100;
            } else {
                iT = Random(27);
            }
            if (iT < 18) {
                if (iT >= 9) {
                    jj = Random(30);
                    j = Random(3);
                    lppl->rgMinConc[j] = jj + 1;
                } else {
                    for (iT++; iT < 16; iT *= 2) {
                        jj = Random(30);
                        j = Random(3);
                        lppl->rgMinConc[j] = jj + 1;
                    }
                }
            }
            i++;
            lppl++;
        }
        for (j = 0; j < 3; j++) {
            lpPlanets->rgwtMin[j] = (int16_t)(Random(lpPlanets->rgMinConc[j] * 10) + 10);
            if (lpPlanets->rgwtMin[j] < 200) {
                lpPlanets->rgwtMin[j] += (int16_t)(Random(150) + 155);
            }
            if (game.fBBSPlay) {
                lpPlanets->rgwtMin[j] += (int32_t)(lpPlanets->rgwtMin[j] / 4);
            }
        }
        l = (uint32_t)(dGal * 6);
        lDistIdeal2 = (int32_t)((int32_t)(dGal * dGal) / game.cPlayer) - l;
        if (lDistIdeal2 < 0) {
            lDistIdeal2 = 0;
        } else {
            lDistIdeal2 = (int32_t)(lDistIdeal2 * 9) / 10;
        }
        lDistIdeal2 = (int32_t)(lDistIdeal2 * (int16_t)game.mdStartDist) / 3 + l;
        lDistMin2 = (int32_t)(lDistIdeal2 * 9) / 10;
        lDistMax2 = (int32_t)(lDistIdeal2 * 7) / 6;
RetryAll:
        lBest = 100000000;
        dMin = dGal / 4 + 1000;
        dMax = (int16_t)(3 * dGal) / 4 + 1000;
        for (i = 0; i < 50; i++) {
            rgi[0] = Random(cPlanMax);
            pt = rgptPlan[rgi[0]];
            if (pt.x < dMin) {
                dx = dMin - pt.x;
            } else if (pt.x > dMax) {
                dx = pt.x - dMax;
            } else {
                dx = 0;
            }
            if (pt.y < dMin) {
                dy = dMin - pt.y;
            } else if (pt.y > dMax) {
                dy = pt.y - dMax;
            } else {
                dy = 0;
            }
            if (dx == 0 && dy == 0)
                break;
            l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
            if (l < lBest) {
                lBest = l;
                iBest = rgi[0];
            }
        }
        if (i == 50) {
            rgi[0] = iBest;
        }
        if (game.cPlayer > 4) {
            dMin = dGal / 20 + 1000;
            dMax = LMulDiv(dGal, 19, 20) + 1000;
        } else if (game.cPlayer > 2) {
            dMin = dGal / 10 + 1000;
            dMax = LMulDiv(dGal, 9, 10) + 1000;
        } else {
            dMin = LMulDiv(dGal, 3, 20) + 1000;
            dMax = LMulDiv(dGal, 17, 20) + 1000;
        }
        for (i = 1; i < game.cPlayer; i++) {
            for (j = 0; j < 50; j++) {
                rgi[i] = Random(cPlanMax);
                pt = rgptPlan[rgi[i]];
                if (pt.x >= dMin && pt.y >= dMin && pt.x <= dMax && pt.y <= dMax) {
                    fFound = FALSE;
                    for (k = 0; k < i; k++) {
                        dx = pt.x - rgptPlan[rgi[k]].x;
                        dy = pt.y - rgptPlan[rgi[k]].y;
                        l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                        if (l <= 0 || l < lDistMin2)
                            break;
                        if (l <= lDistMax2) {
                            fFound = TRUE;
                        }
                    }
                    if (k == i && fFound)
                        break;
                }
            }
            if (j == 50) {
                iBest = rgi[i];
                while (++rgi[i] != iBest) {
                    if (rgi[i] >= cPlanMax) {
                        rgi[i] = 0;
                        if (iBest == 0)
                            break;
                    }
                    pt = rgptPlan[rgi[i]];
                    if (pt.x >= dMin && pt.y >= dMin && pt.x <= dMax && pt.y <= dMax) {
                        fFound = FALSE;
                        for (k = 0; k < i; k++) {
                            dx = pt.x - rgptPlan[rgi[k]].x;
                            dy = pt.y - rgptPlan[rgi[k]].y;
                            l = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                            if (l <= 0 || l < lDistMin2)
                                break;
                            if (l <= lDistMax2) {
                                fFound = TRUE;
                            }
                        }
                        if (k == i && fFound)
                            break;
                    }
                }
                if (iBest == rgi[i]) {
                    lDistMin2 -= (int32_t)(lDistIdeal2 / 35);
                    lDistMax2 += (int32_t)(lDistIdeal2 / 35);
                    goto RetryAll;
                }
            }
        }
        for (i = 0; i < game.cPlayer; i++) {
            j = Random(game.cPlayer - i) + i;
            k = rgi[j];
            rgi[j] = rgi[i];
            rgi[i] = k;
            if (GetRaceGrbit(&rgplr[i], ibitRaceAIPlayer) != 0) {
                CreateRandomRace(&rgplr[i]);
            }
            rgplr[i].fDead = FALSE;
            rgplr[i].fLearned = FALSE;
            rgplr[i].grbitTrader = grbitTraderNone;
            for (j = 0; j < 6; j++) {
                rgplr[i].rgTech[j] = 0;
                rgplr[i].rgResSpent[j] = 0;
            }
            switch (GetRaceStat(&rgplr[i], rsMajorAdv)) {
            case raAttack:
                rgplr[i].rgTech[1] = 6;
                rgplr[i].rgTech[2] = 1;
                rgplr[i].rgTech[0] = 1;
                break;
            case raMines:
                rgplr[i].rgTech[2] = 2;
                rgplr[i].rgTech[5] = 2;
                break;
            case raStealth:
                rgplr[i].rgTech[4] = 5;
                break;
            case raMassAccel:
                rgplr[i].rgTech[0] = 4;
                break;
            case raStargate:
                rgplr[i].rgTech[2] = 5;
                rgplr[i].rgTech[3] = 5;
                break;
            case raTerra:
                rgplr[i].rgTech[5] = 6;
                rgplr[i].rgTech[3] = 2;
                rgplr[i].rgTech[0] = 1;
                rgplr[i].rgTech[1] = 1;
                rgplr[i].rgTech[2] = 1;
                break;
            case raMacintosh:
                rgplr[i].rgTech[0] = 1;
                break;
            case raNone:
                for (j = 0; j < 6; j++) {
                    rgplr[i].rgTech[j] = 3;
                }
            }
            if (GetRaceGrbit(&rgplr[i], ibitRaceTech3) != 0) {
                iTechMin = (GetRaceStat(&rgplr[i], rsMajorAdv) == raNone) + 3;
                for (j = 0; j < 6; j++) {
                    if (rgplr[i].rgTech[j] < iTechMin && GetRaceStat(&rgplr[i], static_cast<RaceStat>(j + 8)) == 0) {
                        rgplr[i].rgTech[j] = iTechMin;
                    }
                }
            }
            if (GetRaceGrbit(&rgplr[i], ibitRaceCheapEngines) != 0) {
                rgplr[i].rgTech[2]++;
            }
            if (GetRaceGrbit(&rgplr[i], ibitRaceIFE) != 0 && !game.fTutorial) {
                rgplr[i].rgTech[2]++;
            }
            for (j = 0; j < 4; j++) {
                FSendPlrMsg(i, static_cast<MessageId>(idmTipCanHideUnimportantMessagesClickingCheckmark + j), gotoNone, 0, 0, 0, 0, 0, 0, 0);
            }
        }
        for (i = 0; i < game.cPlayer; i++) {
            iMin = rgi[i];
            lpPlanets[iMin].iPlayer = i;
            lpPlanets[iMin].fStarbase = TRUE;
            lpPlanets[iMin].isb = 0;
            lpPlanets[iMin].fArtifact = FALSE;
            lpPlanets[iMin].cFactories = 10;
            lpPlanets[iMin].cMines = 10;
            lpPlanets[iMin].cDefenses = 10;
            lpPlanets[iMin].fHomeworld = TRUE;
            if (GetRaceGrbit(&rgplr[i], ibitRaceLowStartingPop) != 0) {
                lpPlanets[iMin].rgwtMin[3] = 175;
            } else {
                lpPlanets[iMin].rgwtMin[3] = 250;
            }
            lpPlanets[iMin].SetUGuesses((lpPlanets[iMin].uGuesses() & 0xf000) | ((uint32_t)LOWORD(lpPlanets[iMin].rgwtMin[3]) / 4 & 0xfff));
            for (j = 0; j < 3; j++) {
                lpPlanets[iMin].rgwtMin[j] = lpPlanets->rgwtMin[j];
                if (gd.fTutorial) {
                    lpPlanets[iMin].rgMinConc[j] = 25 <= lpPlanets->rgMinConc[j] ? lpPlanets->rgMinConc[j] : 25;
                } else {
                    lpPlanets[iMin].rgMinConc[j] = 30 <= lpPlanets->rgMinConc[j] ? lpPlanets->rgMinConc[j] : 30;
                }
            }
            lpPlanets[iMin].iScanner = 0;
            FSendPlrMsg(i, idmHomePlanetPeopleReadyLeaveNestExplore, static_cast<MsgGoto>(iMin), iMin, 0, 0, 0, 0, 0, 0);
            if (50 < CAdvantagePoints(&rgplr[i])) {
                iT = 50;
            } else {
                iT = CAdvantagePoints(&rgplr[i]);
            }
            if (rgplr[i].fAi) {
                iT = 50;
                if (rgplr[i].lvlAi >= lvlAiExpert) {
                    lpPlanets[iMin].rgwtMin[3] += (int32_t)(lpPlanets[iMin].rgwtMin[3] / 10);
                }
            }
            if (game.fBBSPlay) {
                pct10 = PctTrueMaxGrowth(i) * 2 + 10;
                lpPlanets[iMin].rgwtMin[3] = (uint32_t)(lpPlanets[iMin].rgwtMin[3] * pct10);
                lpPlanets[iMin].rgwtMin[3] = (int32_t)(lpPlanets[iMin].rgwtMin[3] / 10);
                lpPlanets[iMin].uPopGuess <<= 2;
            }
            j = GetRaceStat(&rgplr[i], rsUseLeftover);
            switch (j) {
            case 0:
            default:
                ktLeft = 10 * iT;
                pl = lpPlanets[iMin].rgwtMin;
                if (*pl < pl[1]) {
                    if (*pl < pl[2]) {
                        iLow = 0;
                    } else {
                        iLow = 2;
                    }
                } else if (pl[1] < pl[2]) {
                    iLow = 1;
                } else {
                    iLow = 2;
                }
                pl[iLow] += (int16_t)((ktLeft >> 2) + (ktLeft & 3));
                ktLeft >>= 2;
                for (j = 0; j < 3; j++) {
                    pl[j] += ktLeft;
                }
                if (rgplr[i].fAi && rgplr[i].lvlAi >= lvlAiTough)
                    goto LConcentrations;
                break;
            case 1:
            LConcentrations:
                if (iT > 0 && iT < 3) {
                    ktLeft = 1;
                } else {
                    ktLeft = iT / 2;
                }
                pb = lpPlanets[iMin].rgMinConc;
                iLow = 0;
                for (j = 0; j < 3; j++) {
                    if ((int16_t)pb[j] < pb[iLow]) {
                        iLow = j;
                    }
                }
                pb[iLow] += ktLeft;
                ktLeft = (int16_t)(ktLeft + 1) / 2;
                for (j = 0; j < 3; j++) {
                    pb[j] += ktLeft;
                }
                break;
            case 2:
                lpPlanets[iMin].cMines += iT >> 1;
                break;
            case 3:
                lpPlanets[iMin].cFactories += iT / 5;
                break;
            case 4:
                lpPlanets[iMin].cDefenses += (int16_t)(iT + 5) / 10;
            }
            if (GetRaceStat(&rgplr[i], rsMajorAdv) == raMacintosh) {
                lpPlanets[iMin].cMines = 0;
                lpPlanets[iMin].cFactories = 0;
                lpPlanets[iMin].cDefenses = 0;
            }
            rgplr[i].iPlayer = i;
            rgplr[i].idPlanetHome = iMin;
            if (rgplr[i].rgEnvVarMax[0] == envImmune) {
                iT = Random(99) + 1;
            } else {
                iT = rgplr[i].rgEnvVarMin[0] + (int16_t)(rgplr[i].rgEnvVarMax[0] - rgplr[i].rgEnvVarMin[0]) / 2;
            }
            lpPlanets[iMin].rgEnvVar[0] = lpPlanets[iMin].rgEnvVarOrig[0] = iT;
            if (rgplr[i].rgEnvVarMax[1] == envImmune) {
                iT = Random(99) + 1;
            } else {
                iT = rgplr[i].rgEnvVarMin[1] + (int16_t)(rgplr[i].rgEnvVarMax[1] - rgplr[i].rgEnvVarMin[1]) / 2;
            }
            lpPlanets[iMin].rgEnvVar[1] = lpPlanets[iMin].rgEnvVarOrig[1] = iT;
            if (rgplr[i].rgEnvVarMax[2] == envImmune) {
                iT = Random(99) + 1;
            } else {
                iT = rgplr[i].rgEnvVarMin[2] + (int16_t)(rgplr[i].rgEnvVarMax[2] - rgplr[i].rgEnvVarMin[2]) / 2;
            }
            lpPlanets[iMin].rgEnvVar[2] = lpPlanets[iMin].rgEnvVarOrig[2] = iT;
            if (!rgplr[i].fAi) {
                rgplr[i].pctResearch = 15;
            }
            rgplr[i].SetITechNow(0);
            rgplr[i].SetITechNext(6);
            rgplr[i].lResLastYear = 0;
            rgplr[i].wScore = 0;
            for (j = 0; j < game.cPlayer; j++) {
                rgplr[i].rgmdRelation[j] = 0;
            }
            rgplr[i].cshdefSB = 1;
            lpshdef = (SHDEF *)LpAlloc(10 * sizeof(SHDEF), htShips);
            memmove(lpshdef, LpshdefSBT(), 4 * sizeof(SHDEF));
            memset(lpshdef + 4, 0, 6 * sizeof(SHDEF));
            lpshdef->cBuilt = 1;
            lpshdef->cExist = 1;
            rglpshdefSB[i] = lpshdef;
            for (j = 1; j < 10; j++) {
                lpshdef[j].fFree = TRUE;
            }
            if (GetRaceStat(&rgplr[i], rsMajorAdv) == raMassAccel) {
                lpshdef->hul.rghs[0].iItem = ispecialSBMassDriver5;
                lpshdef->hul.rghs[0].cItem = 1;
                if ((int16_t)game.mdSize > sizeTiny) {
                    lpshdef[1].fFree = FALSE;
                    rgplr[i].cshdefSB++;
                    lpshdef[1].cExist = 1;
                    lpshdef[1].cBuilt = 1;
                }
            } else if (GetRaceStat(&rgplr[i], rsMajorAdv) == raStargate && !game.fTutorial) {
                lpshdef->hul.rghs[0].iItem = ispecialSBStargate100250;
                lpshdef->hul.rghs[0].cItem = 1;
                if ((int16_t)game.mdSize > sizeTiny) {
                    lpshdef[1] = lpshdef[2];
                    lpshdef[1].ishdef = 17;
                    lpshdef[1].fFree = FALSE;
                    rgplr[i].cshdefSB++;
                    lpshdef[1].cExist = 1;
                    lpshdef[1].cBuilt = 1;
                }
            } else if (GetRaceStat(&rgplr[i], rsMajorAdv) == raMacintosh) {
                idHome = rgplr[i].idPlanetHome;
                lpshdef[1] = *lpshdef;
                lpshdef[1].ishdef = 17;
                *lpshdef = lpshdef[3];
                lpshdef->ishdef = 16;
                lpshdef->fFree = FALSE;
                rgplr[i].cshdefSB++;
                lpshdef->cExist = 0;
                lpshdef->cBuilt = 0;
                lpPlanets[idHome].isb = 1;
            }
        }
        cFleet = 0;
        rglpfl = (FLEET **)LpAlloc(sizeof(FLEET *), htMisc);
        for (i = 0; i < game.cPlayer; i++) {
            idPlayer = i;
            lpshdef = (SHDEF *)LpAlloc(16 * sizeof(SHDEF), htShips);
            memset(lpshdef, 0, 16 * sizeof(SHDEF));
            for (j = 0; j < 16; j++) {
                lpshdef[j].fFree = TRUE;
            }
            rglpshdef[i] = lpshdef;
            idHome = rgplr[i].idPlanetHome;
            raMajor = static_cast<RaceAttribute>(GetRaceStat(&rgplr[i], rsMajorAdv));
            switch (raMajor) {
            case raMassAccel:
                CreateStartupShip(i, idHome, 4, TRUE);
                lpPlanets[idHome].iWarpFling = 1;
                break;
            case raAttack:
                CreateStartupShip(i, idHome, 3, TRUE);
                if (rgplr[i].rgTech[3] < 3)
                    break;
                CreateStartupShip(i, idHome, 7, TRUE);
                CreateStartupShip(i, idHome, 13, TRUE);
                break;
            case raNone:
                CreateStartupShip(i, idHome, 3, TRUE);
                CreateStartupShip(i, idHome, 4, TRUE);
                break;
            case raStealth:
                CreateStartupShip(i, idHome, rgplr[i].rgTech[0] < 2 ? 2 : 5, TRUE);
                if (rgplr[i].fAi)
                    break;
                CreateStartupShip(i, idHome, 1, TRUE);
                break;
            default:
                CreateStartupShip(i, idHome, 2, TRUE);
            }
            switch (raMajor) {
            case raCheapCol:
                ishRet = CreateStartupShip(i, idHome, 12, TRUE);
                for (j = 1; j < 3; j++) {
                    CreateStartupShip(i, idHome, ishRet, FALSE);
                }
                break;
            case raStargate:
                CreateStartupShip(i, idHome, 11, TRUE);
                break;
            case raMacintosh:
                CreateStartupShip(i, idHome, 10, TRUE);
                break;
            default:
                ishRet = CreateStartupShip(i, idHome, 9, TRUE);
            }
            if (raMajor == raMines) {
                CreateStartupShip(i, idHome, 16, TRUE);
                CreateStartupShip(i, idHome, 18, TRUE);
            } else if (raMajor == raTerra) {
                CreateStartupShip(i, idHome, 17, TRUE);
            } else if (raMajor == raStargate) {
                CreateStartupShip(i, idHome, 7, TRUE);
                CreateStartupShip(i, idHome, 8, TRUE);
                if ((int16_t)game.mdSize > sizeTiny)
                    goto LGive2ndPlanet;
            } else if (raMajor == raMassAccel && (int16_t)game.mdSize > sizeTiny) {
            LGive2ndPlanet:
                lpplPicked = NULL;
                lpplClosest = NULL;
                ptHome = rgptPlan[idHome];
                cFit = 0;
                lDistMin2 = (int32_t)(dGal * 15) / 100;
                lDistMin2 = (uint32_t)(lDistMin2 * lDistMin2);
                lDistMax2 = (int32_t)(dGal * 23) / 100;
                lDistMax2 = (uint32_t)(lDistMax2 * lDistMax2);
                lDistIdeal2 = (int32_t)(dGal * 20) / 100;
                lDistIdeal2 = (uint32_t)(lDistIdeal2 * lDistIdeal2);
                lBest = 10000000;
                pptMax = &rgptPlan[cPlanMax];
                ppt = rgptPlan;
                lppl = lpPlanets;
                while (ppt < pptMax) {
                    if (lppl->iPlayer == iplrNone) {
                        dx = ppt->x - ptHome.x;
                        dy = ppt->y - ptHome.y;
                        lDistCur2 = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
                        if (lDistCur2 >= lDistMin2 && lDistCur2 <= lDistMax2) {
                            cFit++;
                            if (Random(cFit) == 0) {
                                lpplPicked = lppl;
                            }
                        } else if (!lpplPicked && lDistCur2 < lBest) {
                            lBest = lDistCur2;
                            lpplClosest = lppl;
                        }
                    }
                    ppt++;
                    lppl++;
                }
                if (!lpplPicked) {
                    lpplPicked = lpplClosest;
                }
                lpplPicked->iWarpFling = 1;
                cFit = 0;
                while (PctPlanetDesirability(lpplPicked, i) < 10 && cFit++ < 100) {
                    for (j = 0; j < 3; j++) {
                        lpplPicked->rgEnvVar[j] = lpplPicked->rgEnvVarOrig[j] = Random(97) + 2;
                    }
                }
                if (cFit >= 100) {
                    for (j = 0; j < 3; j++) {
                        lpplPicked->rgEnvVar[j] = lpPlanets[idHome].rgEnvVar[j];
                        lpplPicked->rgEnvVarOrig[j] = lpPlanets[idHome].rgEnvVarOrig[j];
                    }
                }
                lpplPicked->iPlayer = i;
                lpplPicked->fStarbase = TRUE;
                lpplPicked->isb = 1;
                lpplPicked->fArtifact = FALSE;
                lpplPicked->cFactories = 4;
                lpplPicked->cMines = 10;
                lpplPicked->rgwtMin[3] = (int32_t)(lpPlanets[idHome].rgwtMin[3] * 2) / 5;
                lpplPicked->uPopGuess = (uint32_t)LOWORD(lpplPicked->rgwtMin[3]) / 4;
                for (j = 0; j < 3; j++) {
                    lpplPicked->rgwtMin[j] = (int16_t)(Random(200) + 100);
                }
                lpplPicked->iScanner = 0;
                lpPlanets[idHome].rgwtMin[3] = (int32_t)(lpPlanets[idHome].rgwtMin[3] * 4) / 5;
                lpPlanets[idHome].SetUGuesses((lpPlanets[idHome].uGuesses() & 0xf000) | ((uint32_t)LOWORD(lpPlanets[idHome].rgwtMin[3]) / 4 & 0xfff));
                CreateStartupShip(i, lpplPicked->id, 0, FALSE);
            } else if (raMajor == raNone) {
                CreateStartupShip(i, idHome, rgplr[i].rgTech[3] < 4 ? 6 : 8, TRUE);
                CreateStartupShip(i, idHome, 7, TRUE);
                CreateStartupShip(i, idHome, 14, TRUE);
            }
            if (GetRaceGrbit(&rgplr[i], ibitRaceOBRM) == 0 && GetRaceGrbit(&rgplr[i], ibitRaceARM) != 0) {
                ishRet = CreateStartupShip(i, idHome, 15, TRUE);
                CreateStartupShip(i, idHome, ishRet, FALSE);
            }
            for (j = 0; j < rgplr[i].cShDef; j++) {
                chs = rglpshdef[i][j].hul.chs;
                lphs = rglpshdef[i][j].hul.rghs;
                k = 0;
                while (k < chs) {
                    cTry = 0;
                    part.hs = *lphs;
                    switch (part.hs.grhst) {
                    default:
                        break;
                    case hstEngine:
                        if (part.hs.iItem != iengineQuickJump5)
                            break;
                        if (rgplr[i].rgEnvVar[2] == envImmune || rglpshdef[i][j].hul.ihuldef != ihuldefColonyShip || rgplr[i].rgEnvVar[2] >= 85) {
                            rgTry[cTry++] = 10;
                        }
                        rgTry[cTry++] = 5;
                        rgTry[cTry++] = 4;
                        rgTry[cTry++] = 2;
                        rgTry[cTry++] = 3;
                        break;
                    case hstShield:
                        if (part.hs.iItem != ishieldMoleSkinShield && part.hs.iItem != ishieldCowHideShield)
                            break;
                        rgTry[cTry++] = 2;
                        rgTry[cTry++] = 1;
                        break;
                    case hstArmor:
                        if (part.hs.iItem != iarmorTritanium && part.hs.iItem != iarmorCrobmnium)
                            break;
                        rgTry[cTry++] = 2;
                        rgTry[cTry++] = 1;
                        break;
                    case hstBeam:
                        if (part.hs.iItem != ibeamLaser && part.hs.iItem != ibeamXRayLaser)
                            break;
                        rgTry[cTry++] = 3;
                        rgTry[cTry++] = 1;
                        break;
                    case hstTorp:
                        if (part.hs.iItem != itorpAlphaTorpedo)
                            break;
                        rgTry[cTry++] = 1;
                        break;
                    case hstBomb:
                        if (part.hs.iItem != ibombLadyFingerBomb)
                            break;
                        rgTry[cTry++] = 1;
                        break;
                    case hstMining:
                        if (part.hs.iItem != iminingRoboMidgetMiner && part.hs.iItem != iminingRoboMiniMiner)
                            break;
                        rgTry[cTry++] = 2;
                        rgTry[cTry++] = 0;
                        break;
                    case hstScanner:
                        if (part.hs.iItem == iscannerBatScanner || part.hs.iItem == iscannerRhinoScanner) {
                            rgTry[cTry++] = 4;
                            rgTry[cTry++] = 2;
                            rgTry[cTry++] = 1;
                        }
                    }
                    for (l = 0; l < cTry; l++) {
                        part.hs.iItem = rgTry[l];
                        if (FLookupPart(&part) == mdPartAvailAvailable) {
                            lphs->iItem = rgTry[l];
                            break;
                        }
                    }
                    k++;
                    lphs++;
                }
            }
            if (GetRaceStat(&rgplr[i], rsMajorAdv) == raMacintosh) {
                lpPlanets[idHome].iScanner = 31;
            }
        }
        idPlayer = iplrNone;
        if (lpPlanets->iPlayer == iplrNone) {
            for (j = 0; j < 3; j++) {
                lpPlanets->rgwtMin[j] = 0;
            }
        }
        for (i = 0; i < game.cPlayer; i++) {
            rgcbtlplan[i] = 5;
            rglpbtlplan[i] = ::new (LpAlloc(16 * sizeof(BTLPLAN), htShips)) BTLPLAN[16];
            for (j = 0; j < 5; j++) {
                InitBattlePlan(rglpbtlplan[i] + j, j, i);
            }
        }
        game.cPlanMax = cPlanMax;
        if (game.fNoRandom) {
            iBest = 0;
        } else {
            iBest = vrgWormholeMin[game.mdSize] + Random(vrgWormholeVar[game.mdSize]);
        }
        if (iBest > 0) {
            for (i = 0; i < iBest; i++) {
                for (j = 0; j < 2; j++) {
                    lpth = LpthNew(0, ithWormhole);
                    { const auto iStableForThing = (Random(3)); auto &thForThing = *(lpth); thForThing.thw().SetIStable(iStableForThing); }
                    if (j == 1) {
                        lpthLast = LpthFromId(idLast);
                        { const auto idPartnerForThing = (lpthLast->idFull()); auto &thForThing = *(lpth); thForThing.thw().SetIdPartner(idPartnerForThing); }
                        { const auto idPartnerForThing = (lpth->idFull()); auto &thForThing = *(lpthLast); thForThing.thw().SetIdPartner(idPartnerForThing); }
                    } else {
                        idLast = lpth->idFull();
                    }
                    k = 0;
                    iMax = 16;
                    while (k++ < 100) {
                        { const auto xForThing = (Random(dGal) + 1000); auto &thForThing = *(lpth); thForThing.pt().SetX(xForThing); }
                        { const auto yForThing = (Random(dGal) + 1000); auto &thForThing = *(lpth); thForThing.pt().SetY(yForThing); }
                        iLow = IValidateWormholePos(lpth);
                        if (iLow == 0)
                            break;
                        if (iLow < iMax) {
                            iMax = iLow;
                            pt = lpth->pt();
                        }
                    }
                    if (iLow != 0) {
                        lpth->pt().Set(pt);
                    }
                }
            }
        }
        for (i = 0; i < game.cPlayer; i++) {
            if (rgplr[i].fHacker) {
                FSendPlrMsg2(i, idmRaceDefinitionHasTamperedStatisticsHaveAltered, gotoNone, 0, 0);
                for (j = 0; j < game.cPlayer; j++) {
                    if (i != j && !rgplr[i].fAi) {
                        FSendPlrMsg2(j, idmHackedRaceDiscoveredRaceStatisticsHaveAltered, gotoNone, i, 0);
                    }
                }
            }
        }
        iplrSingle = iplrNone;
        for (i = 0; i < game.cPlayer; i++) {
            if (!rgplr[i].fAi || rgplr[i].idAi == idAiMaid) {
                if (iplrSingle != iplrNone)
                    break;
                iplrSingle = i;
            } else {
                rgplr[i].lSalt = 156085230;
            }
        }
        game.fSinglePlr = i == game.cPlayer && iplrSingle != iplrNone;
        if (game.fSinglePlr) {
            for (i = 0; i < game.cPlayer; i++) {
                for (j = 0; j < game.cPlayer; j++) {
                    if (i != j) {
                        rgplr[i].rgmdRelation[j] = 2;
                    }
                }
            }
        }
        if (!game.fTutorial) {
            game.lid = DwTickCount();
        }
        CchSprintf(szWork, "%s.xy", szBase);
        if (!FCreateFile(dtXY, iplrNone, NULL)) {
            AlertSz(PszFormatIds(idsUnableCreateUniverseDefinitionFile, NULL), MB_ICONHAND);
            DestroyCurGame();
            return FALSE;
        }
        FEncodeGame(game, rgbGame);
        WriteRt(rtGame, cbWireGame, rgbGame);
        xOld = 1000;
        for (i = 0; i < cPlanMax; i++) {
            starpack.y = LOWORD(rgptPlan[i].y);
            starpack.id = LOWORD(rgidPlan[i]);
            dx = rgptPlan[i].x - xOld;
            starpack.dx = dx;
            FEncodeStarpack(starpack, rgbStarpack);
            RgToStream(rgbStarpack, cbWireStarpack);
            xOld = rgptPlan[i].x;
        }
        i = game.cPlayer;
        WriteRtEof(i);
        StreamClose();
        for (i = -1; i < game.cPlayer; i++) {
            FWriteDataFile(szBase, i, FALSE);
        }
        if (fBatchMode) {
            return TRUE;
        }
        if (game.fSinglePlr) {
            DestroyCurGame();
            CchSprintf(szExt, MPCTD, iplrSingle + 1);
            if (!FLoadGame(szBase, szExt)) {
                AlertSz(PszFormatIds(idsUnableOpenNewTurnFile, NULL), MB_ICONHAND);
                return FALSE;
            }
            idPlayer = iplrSingle;
            CreateChildWindows();
            PostOpenGame();
        } else {
            idPlayer = iplrNone;
            imemLogCur = 0;
            CreateChildWindows();
        }
        return TRUE;
    } catch (const StarsError &) {
        DestroyCurGame();
        return FALSE;
    }
}

int16_t CreateStartupShip(int16_t iplr, int16_t idPlanet, int16_t ishdef, int16_t fAddShdef) {
    int16_t ishMac;
    FLEET  *lpfl;

    if (fAddShdef) {
        ishMac = (int16_t)(int8_t)rgplr[iplr].cShDef++;
        memmove(rglpshdef[iplr] + ishMac, LpshdefT() + ishdef, sizeof(SHDEF));
        rglpshdef[iplr][ishMac].ishdef = ishMac;
        ishdef = ishMac;
    }
    rglpshdef[iplr][ishdef].cExist++;
    rglpshdef[iplr][ishdef].cBuilt++;
    lpfl = LpflNew(iplr, idPlanet);
    lpfl->rgcsh[ishdef] = 1;
    lpfl->rgwtMin[4] = LGetFleetStat(lpfl, 1);
    lpfl->iplan = 0;
    return ishdef;
}

int16_t GenNewGameFromFile(char *pszFile) {
    int32_t rgl[10];
    int16_t cPlr;
    int16_t rgplrbmp[16];
    int16_t cNum;
    int16_t c;
    int16_t i;
    int16_t fSuccess;
    int16_t fReadRace;
    char   *lpbStart;
    int16_t j;
    char   *lpb;
    char   *lpbDef;
    int16_t cb;
    int32_t cbFile;
    char   *pchT;
    AiRace  idAi;
    int16_t lvlAi;

    fSuccess = FALSE;
    strcpy(szWork, pszFile);
    StarsRecoveryScope scopeRecovery;
    try {
        if (ini.fLogging) {
            /* NATIVE: the host passes szBase itself; see FLoadGame. */
            if (pszFile != szBase) {
                strcpy(szBase, pszFile);
            }
            pchT = strrchr(szBase, 46);
            *pchT = 0;
            TurnLog(idsGeneratingYearD);
        }
        memset(&game, 0, sizeof(GAME));
        StreamOpen(pszFile, mdRead);
        cbFile = CbFileSize(hf);
        if (cbFile < 0 || cbFile >= 16000) {
            FileError(idsUniverseCreationFileAppearsInvalid);
            goto LError;
        }
        cb = (int16_t)cbFile;
        lpbDef = (char *)LpAlloc(cb + 1, htPerm);
        lpbDefMac = lpbDef + cb;
        RgFromStream(lpbDef, cb);
        StreamClose();
        lpbDef[cb] = 0;
        if (!FUtf8Valid(lpbDef, cb)) {
            FileError(idsUniverseCreationFileAppearsInvalid);
            goto LError;
        }
        lpb = lpbDef;
        if (cb >= 3 && !memcmp(lpb, "\xef\xbb\xbf", 3)) lpb += 3;
        lpbStart = PszGetLine(&lpb);
        if (*lpbStart == 0 || strlen(lpbStart) > 31) {
            AlertSz(PszFormatIds(idsIllegalGameTitle, NULL), MB_ICONHAND);
            goto LError;
        }
        if (CchCopyUtf8(game.szName, sizeof(game.szName), lpbStart, strlen(lpbStart)) < 0) {
            FileError(idsGameFileAppearsCorruptUnableLoadFile);
            goto LError;
        }
        if (lpb >= lpbDefMac) {
        LUniDefShort:
            AlertSz(PszFormatIds(idsUniverseDefinitionFileAppearsTooShort, NULL), MB_ICONHAND);
            goto LError;
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 4);
        if (cNum == -1) {
        LUniDefError:
            AlertSz(PszFormatIds(idsLine2HasBadUniverseDefinitionParameter, NULL), MB_ICONHAND);
            goto LError;
        }
        for (i = 0; i < cNum; i++) {
            if (i < 3 && (rgl[i] < 0 || rgl[i] > 4 || (rgl[i] == 4 && i != 0)))
                goto LUniDefError;
        }
        if (cNum >= 1) {
            game.mdSize = static_cast<UniverseSize>(LOWORD(rgl[0]));
        }
        if (cNum >= 2) {
            game.mdDensity = static_cast<UniverseDensity>(LOWORD(rgl[1]));
        }
        if (cNum >= 3) {
            game.mdStartDist = static_cast<StartDistance>(LOWORD(rgl[2]));
        }
        if (cNum >= 4) {
            Randomize(rgl[3]);
        }
        if (lpb >= lpbDefMac)
            goto LUniDefShort;
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 7);
        if (cNum == -1) {
        LUniDefError3:
            AlertSz(PszFormatIds(idsLine3HasBadUniverseDefinitionParameter, NULL), MB_ICONHAND);
            goto LError;
        }
        for (i = 0; i < cNum; i++) {
            if (rgl[i] < 0 || rgl[i] > 1)
                goto LUniDefError3;
        }
        if (cNum >= 1) {
            game.fExtraFuel = LOWORD(rgl[0]);
        }
        if (cNum >= 2) {
            game.fSlowTech = LOWORD(rgl[1]);
        }
        if (cNum >= 3) {
            game.fBBSPlay = LOWORD(rgl[2]);
        }
        if (cNum >= 4) {
            game.fNoRandom = LOWORD(rgl[3]);
        }
        if (cNum >= 5) {
            game.fAisBand = LOWORD(rgl[4]);
        }
        if (cNum >= 6) {
            game.fVisScores = LOWORD(rgl[5]);
        }
        if (cNum >= 7) {
            game.fClumping = LOWORD(rgl[6]);
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 1);
        if (cNum < 1 || rgl[0] < 1 || rgl[0] > 16) {
            AlertSz(PszFormatIds(idsLine4HasImproperNumberPlayerFiles, NULL), MB_ICONHAND);
            goto LError;
        }
        if (lpb >= lpbDefMac)
            goto LUniDefShort;
        cPlr = LOWORD(rgl[0]);
        game.cPlayer = cPlr;
        for (i = 0; i < cPlr; i++) {
            lpbStart = PszGetLine(&lpb);
            if (lpb >= lpbDefMac)
                goto LUniDefShort;
            if (strlen(lpbStart) >= sizeof(szWork)) {
                FileError(idsUniverseCreationFileAppearsInvalid);
                goto LError;
            }
            if (i > 0 && *lpbStart == '#') {
                cNum = CParseNumbers(lpbStart + 1, rgl, 2);
                idAi = static_cast<AiRace>(LOWORD(rgl[0]));
                lvlAi = LOWORD(rgl[1]);
                if (cNum < 2 || (int16_t)idAi < idAiRobotoid || (int16_t)idAi > idAiRandom || lvlAi < 0 || lvlAi > 4) {
                    strcpy(szWork, lpbStart);
                    goto LCantGetRace;
                }
                if (lvlAi == 0) {
                    lvlAi = Random(4);
                } else {
                    lvlAi--;
                }
                if (idAi == idAiRobotoid) {
                    idAi = static_cast<AiRace>(Random(6));
                } else {
                    idAi = static_cast<AiRace>(idAi - 1);
                }
                rgplr[i] = *LpplrComp(idAi, static_cast<AiLevel>(lvlAi));
                rgplr[i].fAi = TRUE;
                rgplr[i].idAi = idAi;
                rgplr[i].lvlAi = lvlAi;
            } else {
                strcpy(szWork, lpbStart);
                fReadRace = FWasRaceFile(szWork, FALSE);
                if (fReadRace != 1) {
                LCantGetRace:
                    CchSprintf(szWork, PszGetCompressedString(idsLineDUnableLoadRaceFileS), i + 5, lpbStart);
                    AlertSz(szWork, MB_ICONHAND);
                    goto LError;
                }
                rgplr[i] = vplr;
            }
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 2);
        if (lpb >= lpbDefMac)
            goto LUniDefShort;
        i = 0;
        if (cNum < 1 || rgl[0] < 0 || rgl[0] > 1) {
        LBadDefVc:
            i += cPlr + 5;
            CchSprintf(szWork, PszGetCompressedString(idsLineDHasImproperVictoryConditionDefinition), i);
            AlertSz(szWork, MB_ICONHAND);
            goto LError;
        }
        if (rgl[0] == 1) {
            if (cNum < 2 || rgl[1] < 20 || rgl[1] > 100)
                goto LBadDefVc;
            SetVCCheck(&game, vcOwnsPercentPlanets, TRUE);
            SetVCVal(&game, vcOwnsPercentPlanets, (int16_t)(LOWORD(rgl[1]) - 20) / 5);
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 3);
        if (lpb >= lpbDefMac)
            goto LUniDefShort;
        i = 1;
        if (cNum < 1 || rgl[0] < 0 || rgl[0] > 1)
            goto LBadDefVc;
        if (rgl[0] == 1) {
            if (cNum < 3 || rgl[1] < 8 || rgl[1] > 26 || rgl[2] < 2 || rgl[2] > 6)
                goto LBadDefVc;
            SetVCCheck(&game, vcAttainsTechLevel, TRUE);
            SetVCCheck(&game, vcAttainsTechFields, TRUE);
            SetVCVal(&game, vcAttainsTechLevel, LOWORD(rgl[1]) - 8);
            SetVCVal(&game, vcAttainsTechFields, LOWORD(rgl[2]) - 2);
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 2);
        if (lpb >= lpbDefMac)
            goto LUniDefShort;
        i = 2;
        if (cNum < 1 || rgl[0] < 0 || rgl[0] > 1)
            goto LBadDefVc;
        if (rgl[0] == 1) {
            if (cNum < 2 || rgl[1] < 1000 || rgl[1] > 20000)
                goto LBadDefVc;
            SetVCCheck(&game, vcExceedsScore, TRUE);
            SetVCVal(&game, vcExceedsScore, (int16_t)(LOWORD(rgl[1]) - 1000) / 1000);
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 2);
        if (lpb >= lpbDefMac)
            goto LUniDefShort;
        i = 3;
        if (cNum < 1 || rgl[0] < 0 || rgl[0] > 1)
            goto LBadDefVc;
        if (rgl[0] == 1) {
            if (cNum < 2 || rgl[1] < 20 || rgl[1] > 300)
                goto LBadDefVc;
            SetVCCheck(&game, vcExceedsSecondPlaceBy, TRUE);
            SetVCVal(&game, vcExceedsSecondPlaceBy, (int16_t)(LOWORD(rgl[1]) - 20) / 10);
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 2);
        if (lpb >= lpbDefMac)
            goto LUniDefShort;
        i = 4;
        if (cNum < 1 || rgl[0] < 0 || rgl[0] > 1)
            goto LBadDefVc;
        if (rgl[0] == 1) {
            if (cNum < 2 || rgl[1] < 10 || rgl[1] > 500)
                goto LBadDefVc;
            SetVCCheck(&game, vcProductionCapacity, TRUE);
            SetVCVal(&game, vcProductionCapacity, (int16_t)(LOWORD(rgl[1]) - 10) / 10);
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 2);
        if (lpb >= lpbDefMac)
            goto LUniDefShort;
        i = 5;
        if (cNum < 1 || rgl[0] < 0 || rgl[0] > 1)
            goto LBadDefVc;
        if (rgl[0] == 1) {
            if (cNum < 2 || rgl[1] < 10 || rgl[1] > 300)
                goto LBadDefVc;
            SetVCCheck(&game, vcOwnsCapitalShips, TRUE);
            SetVCVal(&game, vcOwnsCapitalShips, (int16_t)(LOWORD(rgl[1]) - 10) / 10);
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 2);
        if (lpb >= lpbDefMac)
            goto LUniDefShort;
        i = 6;
        if (cNum < 1 || rgl[0] < 0 || rgl[0] > 1)
            goto LBadDefVc;
        if (rgl[0] == 1) {
            if (cNum < 2 || rgl[1] < 30 || rgl[1] > 900)
                goto LBadDefVc;
            SetVCCheck(&game, vcHighestScoreAfterYears, TRUE);
            SetVCVal(&game, vcHighestScoreAfterYears, (int16_t)(LOWORD(rgl[1]) - 30) / 10);
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 2);
        if (lpb >= lpbDefMac)
            goto LUniDefShort;
        i = 7;
        if (cNum < 1 || rgl[0] < 0 || rgl[0] > 7)
            goto LBadDefVc;
        if (rgl[0] > 0) {
            if (cNum < 2 || rgl[1] < 30 || rgl[1] > 500)
                goto LBadDefVc;
            SetVCVal(&game, vcMeetsNumCriteria, LOWORD(rgl[0]));
            SetVCVal(&game, vcMinYearsBeforeWin, (int16_t)(LOWORD(rgl[1]) - 30) / 10);
        }
        lpbStart = PszGetLine(&lpb);
        lpb = lpbStart + (-1 + strlen(lpbStart));
        if (lpb - lpbStart >= 3 && *lpb == 'y' && lpb[-1] == 'x' && lpb[-2] == '.') {
            lpb[-2] = 0;
        }
        if (CchCopyUtf8(szBase, sizeof(szBase), lpbStart, strlen(lpbStart)) < 0) {
            FileError(idsUniverseCreationFileAppearsInvalid);
            goto LError;
        }
        if (lpb + 4 < lpbDefMac) {
            lpbDefUni = lpb;
        }
        for (i = 0; i < game.cPlayer; i++) {
            if (!rgplr[i].fAi && CAdvantagePoints(&rgplr[i]) < 0) {
                rgplr[i] = vrgplrDef[0];
                rgplr[i].fHacker = TRUE;
            }
            if (rgplr[i].szName[0] == 0) {
                CchGetString(static_cast<StringId>(idsBerserker + Random(24)), rgplr[i].szName);
                CchSprintf(rgplr[i].szNames, "%ss", rgplr[i].szName);
            }
        }
        for (i = 1; i < game.cPlayer; i++) {
            for (j = 0; j < i && strcmp(rgplr[i].szName, rgplr[j].szName) != 0; j++) {
            }
            if (j < i) {
                c = Random(24);
                while (1) {
                    for (j = 0; j < game.cPlayer && strcmp(rgplr[j].szName, PszGetCompressedString(static_cast<StringId>(idsBerserker + c))) != 0; j++) {
                    }
                    if (j == game.cPlayer)
                        break;
                    c++;
                    if (c >= 24) {
                        c = 0;
                    }
                }
                CchGetString(static_cast<StringId>(idsBerserker + c), rgplr[i].szName);
                strcpy(rgplr[i].szNames, rgplr[i].szName);
                strcat(rgplr[i].szNames, "s");
            }
        }
        for (i = 0; i < game.cPlayer; i++) {
            rgplrbmp[i] = rgplr[i].iPlrBmp;
            if (rgplrbmp[i] < 0 || rgplrbmp[i] >= 32) {
                rgplrbmp[i] = -1;
            }
        }
        for (i = 1; i < game.cPlayer; i++) {
            if (rgplrbmp[i] != -1) {
                for (j = 0; j < i && rgplrbmp[j] != rgplrbmp[i]; j++) {
                }
                if (j < i) {
                    rgplrbmp[Random(2) == 0 ? j : i] = -1;
                }
            }
        }
        for (i = 0; i < game.cPlayer; i++) {
            if (rgplrbmp[i] == -1) {
                rgplrbmp[i] = Random(32);
                while (1) {
                    for (j = 0; j < game.cPlayer && (j == i || rgplrbmp[i] != rgplrbmp[j]); j++) {
                    }
                    if (j == game.cPlayer)
                        break;
                    rgplrbmp[i]++;
                    if (rgplrbmp[i] >= 32) {
                        rgplrbmp[i] = 0;
                    }
                }
            }
        }
        for (i = 0; i < game.cPlayer; i++) {
            rgplr[i].iPlrBmp = rgplrbmp[i];
        }
        GenerateWorld(TRUE);
        fSuccess = TRUE;
    } catch (const StarsError &) {
    }
LError:
    StreamClose();
    lpbDefUni = NULL;
    TurnLog(static_cast<StringId>(idsFailed + fSuccess));
    return fSuccess;
}

void CreateTutorWorld() {
    int16_t i;

    memset(&game, 0, sizeof(GAME));
    game.cPlayer = 2;
    game.fTutorial = TRUE;
    game.mdDensity = densitySparse;
    game.mdSize = sizeTiny;
    game.mdStartDist = startDistModerate;
    game.fBBSPlay = TRUE;
    game.fVisScores = TRUE;
    game.fNoRandom = TRUE;
    game.lid = 9236297;
    game.rgvc[7] = 128;
    game.rgvc[8] = 129;
    CchGetString(idsTutorialGame, game.szName);
    rgplr[0] = vrgplrDef[0];
    CchGetString(idsHumanoid, rgplr[0].szName);
    CchSprintf(rgplr[0].szNames, "%ss", rgplr[0].szName);
    rgplr[1] = *LpplrComp(idAiTurinDrone, lvlAiEasy);
    rgplr[1].fAi = TRUE;
    rgplr[1].lvlAi = lvlAiEasy;
    rgplr[1].idAi = idAiTurinDrone;
    CchGetString(idsBerserker, rgplr[1].szName);
    Randomize(1234567890);
    for (i = 1; i <= 2; i++) {
        CchSprintf(szWork, PszGetCompressedString(idsSHD), szBase, i);
        RemoveNativeFile(szWork);
        CchSprintf(szWork, PszGetCompressedString(idsSXD), szBase, i);
        RemoveNativeFile(szWork);
    }
    GenerateWorld(FALSE);
    return;
}

void InitNewGamePlr(int16_t iStepMaxSoFar, AiLevel lvlAi) {
    int16_t i;
    int16_t c;
    uint8_t ch;

    if (iStepMaxSoFar < 2 && !fRCWReadOnly) {
        SetVCVal(&game, vcMinYearsBeforeWin, game.mdSize * 2);
        if (iStepMaxSoFar < 1) {
            switch (game.mdSize) {
            case sizeTiny:
                if (lvlAi == lvlAiExpert && Random(3) == 0) {
                    game.cPlayer = 3;
                    break;
                }
                game.cPlayer = 2;
                break;
            case sizeSmall:
                if (lvlAi == lvlAiExpert && Random(4) == 0) {
                    game.cPlayer = 5;
                    break;
                }
                if ((int16_t)lvlAi >= lvlAiTough && Random(6 - lvlAi) == 0) {
                    game.cPlayer = 4;
                    break;
                }
                game.cPlayer = 3;
                break;
            case sizeMedium:
                if (lvlAi == lvlAiExpert && Random(10) == 0) {
                    game.cPlayer = 9;
                    break;
                }
                if (lvlAi == lvlAiExpert && Random(10) == 0) {
                    game.cPlayer = 5;
                    break;
                }
                if ((int16_t)lvlAi >= lvlAiTough && Random(7 - lvlAi) == 0) {
                    game.cPlayer = 8;
                    break;
                }
                if ((int16_t)lvlAi >= lvlAiTough && Random(7 - lvlAi) == 0) {
                    game.cPlayer = 6;
                    break;
                }
                game.cPlayer = 7;
                break;
            case sizeLarge:
                if (lvlAi == lvlAiExpert && Random(10) == 0) {
                    game.cPlayer = Random(2) + 14;
                    break;
                }
                if (lvlAi == lvlAiExpert && Random(10) == 0) {
                    game.cPlayer = 10 - Random(2);
                    break;
                }
                if ((int16_t)lvlAi >= lvlAiTough && Random(7 - lvlAi) == 0) {
                    game.cPlayer = 13;
                    break;
                }
                if ((int16_t)lvlAi >= lvlAiTough && Random(7 - lvlAi) == 0) {
                    game.cPlayer = 11;
                    break;
                }
                game.cPlayer = 12;
                break;
            case sizeHuge:
                if (lvlAi == lvlAiExpert && Random(10) == 0) {
                    game.cPlayer = 13 - Random(3);
                } else if ((int16_t)lvlAi >= lvlAiTough && Random(9 - lvlAi) == 0) {
                    game.cPlayer = 14;
                } else if ((int16_t)lvlAi >= lvlAiTough && Random(7 - lvlAi) == 0) {
                    game.cPlayer = 15;
                } else {
                    game.cPlayer = 16;
                }
            }
            i = 1;
            switch (lvlAi) {
            default:
                break;
            case lvlAiEasy:
                while (i < game.cPlayer) {
                    if (i < (int16_t)(game.cPlayer + 1) / 3 + 1) {
                        vrgplrTypeNew[i++] = 11;
                    } else if (i < (int16_t)((game.cPlayer + 1) * 2) / 3 + 1) {
                        vrgplrTypeNew[i++] = 15;
                    } else if (i < (int16_t)((game.cPlayer + 1) * 5) / 6 + 1) {
                        vrgplrTypeNew[i++] = 7;
                    } else {
                        vrgplrTypeNew[i++] = 27;
                    }
                }
                break;
            case lvlAiStandard:
                while (i < game.cPlayer) {
                    if (i < (int16_t)((game.cPlayer + 5) * 2) / 7 + 1) {
                        vrgplrTypeNew[i++] = 39;
                    } else if (i < (int16_t)((game.cPlayer - 1) * 3 + 6) / 7 + 1) {
                        vrgplrTypeNew[i++] = 35;
                    } else if (i < (int16_t)((game.cPlayer - 1) * 4 + 6) / 7 + 1) {
                        vrgplrTypeNew[i++] = 43;
                    } else if (i < (int16_t)((game.cPlayer - 1) * 5 + 6) / 7 + 1) {
                        vrgplrTypeNew[i++] = 47;
                    } else if (i < (int16_t)((game.cPlayer - 1) * 6 + 6) / 7 + 1) {
                        vrgplrTypeNew[i++] = 51;
                    } else {
                        vrgplrTypeNew[i++] = 155;
                    }
                }
                break;
            case lvlAiTough:
                while (i < game.cPlayer) {
                    if (i < (int16_t)((game.cPlayer + 5) * 2) / 7 + 1) {
                        vrgplrTypeNew[i++] = 83;
                    } else if (i < (int16_t)((game.cPlayer - 1) * 3 + 6) / 7 + 1) {
                        vrgplrTypeNew[i++] = 71;
                    } else if (i < (int16_t)((game.cPlayer - 1) * 4 + 6) / 7 + 1) {
                        vrgplrTypeNew[i++] = 67;
                    } else if (i < (int16_t)((game.cPlayer - 1) * 5 + 6) / 7 + 1) {
                        vrgplrTypeNew[i++] = 87;
                    } else if (i < (int16_t)((game.cPlayer - 1) * 6 + 6) / 7 + 1) {
                        vrgplrTypeNew[i++] = 91;
                    } else {
                        vrgplrTypeNew[i++] = 155;
                    }
                }
                break;
            case lvlAiExpert:
                while (i < game.cPlayer) {
                    if (i < (int16_t)(game.cPlayer + 1) / 3 + 1) {
                        vrgplrTypeNew[i++] = 99;
                    } else if (i < (int16_t)((game.cPlayer - 1) * 6 + 11) / 12 + 1) {
                        vrgplrTypeNew[i++] = 119;
                    } else if (i < (int16_t)((game.cPlayer - 1) * 5 + 5) / 6 + 1) {
                        vrgplrTypeNew[i++] = 115;
                    } else {
                        vrgplrTypeNew[i++] = 123;
                    }
                }
            }
            for (i = 1; i < game.cPlayer - 1; i++) {
                c = Random(game.cPlayer - i - 1) + i + 1;
                ch = vrgplrTypeNew[i];
                vrgplrTypeNew[i] = vrgplrTypeNew[c];
                vrgplrTypeNew[c] = ch;
            }
        }
    }
    return;
}

void InitNewGame3() { return; }

PLAYER *LpplrComp(AiRace idAi, AiLevel lvlAi) { return &vrgplrComp[idAi][lvlAi]; }

void SetVCCheck(GAME *pgame, VictoryCondition vc, int16_t fChecked) {
    pgame->rgvc[vc] = (pgame->rgvc[vc] & 0x7f) | (!fChecked ? 0 : 0x80);
    return;
}

int16_t GetVCCheck(GAME *pgame, VictoryCondition vc) {
    if (pgame->rgvc[vc] & 0x80) {
        return TRUE;
    }
    return FALSE;
}

int16_t SetVCVal(GAME *pgame, VictoryCondition vc, int16_t val) {
    int16_t cur;

    if (val < 0) {
        val = 0;
    } else if (val > vrgvcMax[vc]) {
        val = vrgvcMax[vc];
    }
    pgame->rgvc[vc] = (pgame->rgvc[vc] & 0x80) | val;
    if (vc == vcMeetsNumCriteria) {
        cur = GetVCVal(pgame, vcMeetsNumCriteria, FALSE);
        if (cur != val) {
            val = cur;
            pgame->rgvc[8] = (pgame->rgvc[8] & 0x80) | cur;
        }
    }
    return val;
}

int16_t GetVCVal(GAME *pgame, VictoryCondition vc, int16_t fRaw) {
    int16_t c;
    int16_t i;
    int16_t val;

    val = pgame->rgvc[vc] & 0x7f;
    if (fRaw) {
        return val;
    }
    switch (vc) {
    case vcOwnsPercentPlanets:
        val = 5 * val + 20;
        break;
    case vcAttainsTechLevel:
        val += 8;
        break;
    case vcAttainsTechFields:
        val += 2;
        break;
    case vcExceedsScore:
        val = 1000 * val + 1000;
        break;
    case vcExceedsSecondPlaceBy:
        val = 10 * val + 20;
        break;
    case vcProductionCapacity:
    case vcOwnsCapitalShips:
        val = 10 * val + 10;
        break;
    case vcHighestScoreAfterYears:
        val = 10 * val + 30;
        break;
    case vcMinYearsBeforeWin:
        val = 10 * val + 30;
        break;
    case vcMeetsNumCriteria:
    default:
        c = 0;
        for (i = 0; i < 8; i++) {
            if (i != 2 && (game.rgvc[i] & 0x80)) {
                c++;
            }
        }
        if (c < val) {
            val = c;
        }
        break;
    }
    return val;
}
