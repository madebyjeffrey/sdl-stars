#include "common.h"
#include <algorithm>
#include "appstate.h"
#include "text_io.h"
#include "turn_io.h"
#include "production_io.h"
#include "planet_query_io.h"
#include "part_query_io.h"
#include "race_query_io.h"
#include "alert_io.h"
#include "random_io.h"
#include "record_io.h"

std::int16_t vrgvcMax[10] = {16, 18, 4, 19, 28, 49, 29, 87, 7, 47};
char    rgNG3Width[9][2] = {{-3}, {2, 1}, {5}, {-3}, {3}, {3}, {3}, {1}, {3}};
std::uint8_t vrgWormholeMin[5] = {0, 1, 1, 3, 4};
std::uint8_t vrgWormholeVar[5] = {3, 3, 5, 4, 5};

void InitBattlePlan(APPSESSION &app, BTLPLAN *lpbtlplan, std::int16_t iplan, std::int16_t iplr) {
    *lpbtlplan = rgbtlplanT[iplan];
    lpbtlplan->iplr = iplr;
    if (app.state.game.fSinglePlr && iplan == 0) {
        lpbtlplan->iplrAttack = iplrAttackEveryone;
    }
    return;
}

std::int16_t GenerateWorld(APPSESSION &app, std::int16_t fBatchMode) {
    std::int32_t      *pl;
    std::int16_t       iBest;
    std::int16_t       cKill;
    char          grUsed[128];
    POINT16      *ppt;
    RaceAttribute raMajor;
    std::int16_t       k;
    POINT16       pt;
    std::int16_t       fFound;
    std::int16_t       iMax;
    STARPACK      starpack;
    std::byte     rgbGame[cbWireGame];
    std::byte     rgbStarpack[cbWireStarpack];
    std::int16_t       dy;
    std::int16_t       dGalMinSq;
    std::int16_t       iLow;
    PLANET       *lppl;
    std::int16_t       iMin;
    std::int16_t       i;
    std::int16_t       xOld;
    std::int16_t       iplrSingle;
    POINT16      *pptMax;
    std::int16_t       dMin;
    std::int16_t       ktLeft;
    SHDEF        *lpshdef;
    std::int32_t       lDistMax2;
    std::int32_t       lDistIdeal2;
    std::int16_t       rgi[16];
    std::int16_t       iNewLine;
    std::uint8_t      *pb;
    std::int16_t       dMax;
    std::int32_t       lDistMin2;
    std::int16_t       j;
    std::int16_t       cPlanMax;
    std::int16_t       dx;
    std::int16_t       cKillMax;
    POINT16      *pptT;
    std::int32_t       lBest;
    std::int32_t       l;
    std::int16_t       iT;
    std::int16_t       jj;
    std::int16_t       iTechMin;
    std::int16_t       pct10;
    std::int16_t       idHome;
    std::int16_t       ishRet;
    PART          part;
    std::int16_t       cFit;
    PLANET       *lpplClosest;
    POINT16       ptHome;
    PLANET       *lpplPicked;
    std::int32_t       lDistCur2;
    HS           *lphs;
    std::int16_t       chs;
    std::int16_t       cTry;
    std::int16_t       rgTry[5];
    THING        *lpth;
    std::uint16_t      idLast;
    THING        *lpthLast;
    char          szExt[4];

    iMin = 0;
    cKill = 0;
    app.state.dGal = 400 * app.state.game.mdSize + 400;
    app.state.dGalInv = app.state.dGal + 2000;
    cPlanMax = LOWORD(static_cast<std::int32_t>((app.state.dGal * app.state.dGal)) / 5000);
    cPlanMax += cPlanMax / 4 * (app.state.game.mdDensity - 1);
    if (static_cast<std::int16_t>(app.state.game.mdDensity) >= densityPacked) {
        cPlanMax += cPlanMax / 4;
    }
    cPlanMax = cPlanMax >= 999 ? 999 : cPlanMax;
    dGalMinSq = app.state.dGalMinDist * app.state.dGalMinDist;
    iMax = cPlanMax / 7 + cPlanMax >= 999 ? 999 : cPlanMax / 7 + cPlanMax;
    dx = 1010;
    dy = app.state.dGal - 19;
    for (i = 0; i < iMax; i++) {
        app.state.rgptPlan[i].x = RandomIO().pfnRandom(app, dy) + dx;
        app.state.rgptPlan[i].y = RandomIO().pfnRandom(app, dy) + dx;
    }
    qsort16(app, app.state.rgptPlan, iMax, sizeof(POINT16), reinterpret_cast<APPQSORTCOMPARE>(ICompLong));
    pptMax = &app.state.rgptPlan[iMax];
    for (ppt = app.state.rgptPlan; ppt < pptMax; ppt++) {
        if (ppt->y >= 0) {
            pptT = ppt + 1;
            iNewLine = ppt->x + app.state.dGalMinDist;
            for (; pptT < pptMax && pptT->x <= iNewLine; pptT++) {
                dy = std::abs(ppt->y - pptT->y);
                if (dy <= app.state.dGalMinDist) {
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
        i = RandomIO().pfnRandom(app, iMax);
        if (app.state.rgptPlan[i].y >= 0) {
            app.state.rgptPlan[i].y = -100;
            cKill++;
        }
    }
    pptT = app.state.rgptPlan;
    for (ppt = app.state.rgptPlan; ppt < pptMax; ppt++) {
        if (ppt->y >= 0) {
            *pptT++ = *ppt;
        }
    }
    cPlanMax = iMax - cKill;
    if (app.state.game.fClumping) {
        for (i = 0; i < cPlanMax; i++) {
            lBest = 10000000;
            iBest = 0;
            j = RandomIO().pfnRandom(app, cPlanMax);
            pt = app.state.rgptPlan[j];
            for (k = 0; k < cPlanMax; k++) {
                if (k != j) {
                    dx = pt.x - app.state.rgptPlan[k].x;
                    dy = pt.y - app.state.rgptPlan[k].y;
                    l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                    if (l < lBest) {
                        lBest = l;
                        iBest = k;
                    }
                }
            }
            if (lBest > 144) {
                if (lBest > 1600) {
                    app.state.rgptPlan[j].x = static_cast<std::int16_t>((app.state.rgptPlan[iBest].x * 2 + app.state.rgptPlan[j].x)) / 3;
                    app.state.rgptPlan[j].y = static_cast<std::int16_t>((app.state.rgptPlan[iBest].y * 2 + app.state.rgptPlan[j].y)) / 3;
                } else if (lBest > 625) {
                    app.state.rgptPlan[j].x = static_cast<std::int16_t>((app.state.rgptPlan[j].x + app.state.rgptPlan[iBest].x)) / 2;
                    app.state.rgptPlan[j].y = static_cast<std::int16_t>((app.state.rgptPlan[j].y + app.state.rgptPlan[iBest].y)) / 2;
                } else if (lBest > 324) {
                    app.state.rgptPlan[j].x = static_cast<std::int16_t>((app.state.rgptPlan[j].x * 2 + app.state.rgptPlan[iBest].x)) / 3;
                    app.state.rgptPlan[j].y = static_cast<std::int16_t>((app.state.rgptPlan[j].y * 2 + app.state.rgptPlan[iBest].y)) / 3;
                } else {
                    app.state.rgptPlan[j].x = static_cast<std::int16_t>((app.state.rgptPlan[j].x * 4 + app.state.rgptPlan[iBest].x)) / 5;
                    app.state.rgptPlan[j].y = static_cast<std::int16_t>((app.state.rgptPlan[j].y * 4 + app.state.rgptPlan[iBest].y)) / 5;
                }
            }
        }
        qsort16(app, app.state.rgptPlan, cPlanMax, sizeof(POINT16), reinterpret_cast<APPQSORTCOMPARE>(ICompLong));
    }
    std::ranges::fill(grUsed, 0);
    for (i = 0; i < cPlanMax; i++) {
        dx = RandomIO().pfnRandom(app, cPlanetName);
        while (grUsed[dx >> 3] & app.state.bitTbl[dx & 7]) {
            dx++;
            if (dx >= app.state.game.fTutorial + cPlanetName) {
                dx = 0;
            }
        }
        grUsed[dx >> 3] = grUsed[dx >> 3] | app.state.bitTbl[dx & 7];
        app.state.rgidPlan[i] = dx;
    }
    app.state.cPlanet = cPlanMax;
    StarsRecoveryScope scopeRecovery;
    try {
        app.state.lpPlanets = LpAlloc<PLANET>(app, cPlanMax * sizeof(PLANET), htPlanets);
        std::fill_n(app.state.lpPlanets, cPlanMax, PLANET{});
        i = 0;
        lppl = app.state.lpPlanets;
        while (i < cPlanMax) {
            lppl->id = i;
            lppl->iPlayer = iplrNone;
            lppl->det = detAll;
            lppl->iScanner = 31;
            if (!app.state.game.fNoRandom) {
                lppl->fArtifact = static_cast<std::uint32_t>((RandomIO().pfnRandom(app, 3) == 0)) & 1;
            }
            lppl->rgEnvVar[0] = RandomIO().pfnRandom(app, 90) + 1;
            lppl->rgEnvVar[0] += RandomIO().pfnRandom(app, 10);
            lppl->rgEnvVarOrig[0] = lppl->rgEnvVar[0];
            lppl->rgEnvVar[1] = RandomIO().pfnRandom(app, 90) + 1;
            lppl->rgEnvVar[1] += RandomIO().pfnRandom(app, 10);
            lppl->rgEnvVarOrig[1] = lppl->rgEnvVar[1];
            lppl->rgEnvVar[2] = lppl->rgEnvVarOrig[2] = RandomIO().pfnRandom(app, 99) + 1;
            if (app.state.game.fTutorial) {
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
                if (app.state.game.fExtraFuel) {
                    lppl->rgMinConc[j] = 100;
                } else {
                    lppl->rgwtMin[j] = 0;
                    lppl->rgMinConc[j] = RandomIO().pfnRandom(app, 45) + RandomIO().pfnRandom(app, 45) + 31;
                    if (lppl->rgEnvVar[2] >= 90) {
                        lppl->rgMinConc[j] += RandomIO().pfnRandom(app, 99 - lppl->rgMinConc[j]) / 2;
                    }
                }
                lppl->rgpctMinLevel[j] = 0;
                lppl->rgwtMin[j] = 0;
                if (app.state.game.fBBSPlay && lppl->rgMinConc[j] < 40) {
                    lppl->rgMinConc[j] += 5;
                }
            }
            if (app.state.game.fExtraFuel) {
                iT = 100;
            } else {
                iT = RandomIO().pfnRandom(app, 27);
            }
            if (iT < 18) {
                if (iT >= 9) {
                    jj = RandomIO().pfnRandom(app, 30);
                    j = RandomIO().pfnRandom(app, 3);
                    lppl->rgMinConc[j] = jj + 1;
                } else {
                    for (iT++; iT < 16; iT *= 2) {
                        jj = RandomIO().pfnRandom(app, 30);
                        j = RandomIO().pfnRandom(app, 3);
                        lppl->rgMinConc[j] = jj + 1;
                    }
                }
            }
            i++;
            lppl++;
        }
        for (j = 0; j < 3; j++) {
            app.state.lpPlanets->rgwtMin[j] = static_cast<std::int16_t>((RandomIO().pfnRandom(app, app.state.lpPlanets->rgMinConc[j] * 10) + 10));
            if (app.state.lpPlanets->rgwtMin[j] < 200) {
                app.state.lpPlanets->rgwtMin[j] += static_cast<std::int16_t>((RandomIO().pfnRandom(app, 150) + 155));
            }
            if (app.state.game.fBBSPlay) {
                app.state.lpPlanets->rgwtMin[j] += static_cast<std::int32_t>((app.state.lpPlanets->rgwtMin[j] / 4));
            }
        }
        l = static_cast<std::uint32_t>((app.state.dGal * 6));
        lDistIdeal2 = static_cast<std::int32_t>((static_cast<std::int32_t>((app.state.dGal * app.state.dGal)) / app.state.game.cPlayer)) - l;
        if (lDistIdeal2 < 0) {
            lDistIdeal2 = 0;
        } else {
            lDistIdeal2 = static_cast<std::int32_t>((lDistIdeal2 * 9)) / 10;
        }
        lDistIdeal2 = static_cast<std::int32_t>((lDistIdeal2 * static_cast<std::int16_t>(app.state.game.mdStartDist))) / 3 + l;
        lDistMin2 = static_cast<std::int32_t>((lDistIdeal2 * 9)) / 10;
        lDistMax2 = static_cast<std::int32_t>((lDistIdeal2 * 7)) / 6;
RetryAll:
        lBest = 100000000;
        dMin = app.state.dGal / 4 + 1000;
        dMax = static_cast<std::int16_t>((3 * app.state.dGal)) / 4 + 1000;
        for (i = 0; i < 50; i++) {
            rgi[0] = RandomIO().pfnRandom(app, cPlanMax);
            pt = app.state.rgptPlan[rgi[0]];
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
            l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
            if (l < lBest) {
                lBest = l;
                iBest = rgi[0];
            }
        }
        if (i == 50) {
            rgi[0] = iBest;
        }
        if (app.state.game.cPlayer > 4) {
            dMin = app.state.dGal / 20 + 1000;
            dMax = LMulDiv(app.state.dGal, 19, 20) + 1000;
        } else if (app.state.game.cPlayer > 2) {
            dMin = app.state.dGal / 10 + 1000;
            dMax = LMulDiv(app.state.dGal, 9, 10) + 1000;
        } else {
            dMin = LMulDiv(app.state.dGal, 3, 20) + 1000;
            dMax = LMulDiv(app.state.dGal, 17, 20) + 1000;
        }
        for (i = 1; i < app.state.game.cPlayer; i++) {
            for (j = 0; j < 50; j++) {
                rgi[i] = RandomIO().pfnRandom(app, cPlanMax);
                pt = app.state.rgptPlan[rgi[i]];
                if (pt.x >= dMin && pt.y >= dMin && pt.x <= dMax && pt.y <= dMax) {
                    fFound = 0;
                    for (k = 0; k < i; k++) {
                        dx = pt.x - app.state.rgptPlan[rgi[k]].x;
                        dy = pt.y - app.state.rgptPlan[rgi[k]].y;
                        l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                        if (l <= 0 || l < lDistMin2)
                            break;
                        if (l <= lDistMax2) {
                            fFound = 1;
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
                    pt = app.state.rgptPlan[rgi[i]];
                    if (pt.x >= dMin && pt.y >= dMin && pt.x <= dMax && pt.y <= dMax) {
                        fFound = 0;
                        for (k = 0; k < i; k++) {
                            dx = pt.x - app.state.rgptPlan[rgi[k]].x;
                            dy = pt.y - app.state.rgptPlan[rgi[k]].y;
                            l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                            if (l <= 0 || l < lDistMin2)
                                break;
                            if (l <= lDistMax2) {
                                fFound = 1;
                            }
                        }
                        if (k == i && fFound)
                            break;
                    }
                }
                if (iBest == rgi[i]) {
                    lDistMin2 -= static_cast<std::int32_t>((lDistIdeal2 / 35));
                    lDistMax2 += static_cast<std::int32_t>((lDistIdeal2 / 35));
                    goto RetryAll;
                }
            }
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            j = RandomIO().pfnRandom(app, app.state.game.cPlayer - i) + i;
            k = rgi[j];
            rgi[j] = rgi[i];
            rgi[i] = k;
            if (RaceQueryIO().pfnGrbit(&app.state.rgplr[i], ibitRaceAIPlayer) != 0) {
                CreateRandomRace(app, &app.state.rgplr[i]);
            }
            app.state.rgplr[i].fDead = 0;
            app.state.rgplr[i].fLearned = 0;
            app.state.rgplr[i].grbitTrader = grbitTraderNone;
            for (j = 0; j < 6; j++) {
                app.state.rgplr[i].rgTech[j] = 0;
                app.state.rgplr[i].rgResSpent[j] = 0;
            }
            switch (RaceQueryIO().pfnStat(&app.state.rgplr[i], rsMajorAdv)) {
            case raAttack:
                app.state.rgplr[i].rgTech[1] = 6;
                app.state.rgplr[i].rgTech[2] = 1;
                app.state.rgplr[i].rgTech[0] = 1;
                break;
            case raMines:
                app.state.rgplr[i].rgTech[2] = 2;
                app.state.rgplr[i].rgTech[5] = 2;
                break;
            case raStealth:
                app.state.rgplr[i].rgTech[4] = 5;
                break;
            case raMassAccel:
                app.state.rgplr[i].rgTech[0] = 4;
                break;
            case raStargate:
                app.state.rgplr[i].rgTech[2] = 5;
                app.state.rgplr[i].rgTech[3] = 5;
                break;
            case raTerra:
                app.state.rgplr[i].rgTech[5] = 6;
                app.state.rgplr[i].rgTech[3] = 2;
                app.state.rgplr[i].rgTech[0] = 1;
                app.state.rgplr[i].rgTech[1] = 1;
                app.state.rgplr[i].rgTech[2] = 1;
                break;
            case raMacintosh:
                app.state.rgplr[i].rgTech[0] = 1;
                break;
            case raNone:
                for (j = 0; j < 6; j++) {
                    app.state.rgplr[i].rgTech[j] = 3;
                }
            }
            if (RaceQueryIO().pfnGrbit(&app.state.rgplr[i], ibitRaceTech3) != 0) {
                iTechMin = (RaceQueryIO().pfnStat(&app.state.rgplr[i], rsMajorAdv) == raNone) + 3;
                for (j = 0; j < 6; j++) {
                    if (app.state.rgplr[i].rgTech[j] < iTechMin && RaceQueryIO().pfnStat(&app.state.rgplr[i], static_cast<RaceStat>(j + 8)) == 0) {
                        app.state.rgplr[i].rgTech[j] = iTechMin;
                    }
                }
            }
            if (RaceQueryIO().pfnGrbit(&app.state.rgplr[i], ibitRaceCheapEngines) != 0) {
                app.state.rgplr[i].rgTech[2]++;
            }
            if (RaceQueryIO().pfnGrbit(&app.state.rgplr[i], ibitRaceIFE) != 0 && !app.state.game.fTutorial) {
                app.state.rgplr[i].rgTech[2]++;
            }
            for (j = 0; j < 4; j++) {
                TurnIO().pfnMessage(app, i, static_cast<MessageId>(idmTipCanHideUnimportantMessagesClickingCheckmark + j), gotoNone, 0, 0, 0, 0, 0, 0, 0);
            }
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            iMin = rgi[i];
            app.state.lpPlanets[iMin].iPlayer = i;
            app.state.lpPlanets[iMin].fStarbase = 1;
            app.state.lpPlanets[iMin].isb = 0;
            app.state.lpPlanets[iMin].fArtifact = 0;
            app.state.lpPlanets[iMin].cFactories = 10;
            app.state.lpPlanets[iMin].cMines = 10;
            app.state.lpPlanets[iMin].cDefenses = 10;
            app.state.lpPlanets[iMin].fHomeworld = 1;
            if (RaceQueryIO().pfnGrbit(&app.state.rgplr[i], ibitRaceLowStartingPop) != 0) {
                app.state.lpPlanets[iMin].rgwtMin[3] = 175;
            } else {
                app.state.lpPlanets[iMin].rgwtMin[3] = 250;
            }
            app.state.lpPlanets[iMin].SetUGuesses((app.state.lpPlanets[iMin].uGuesses() & 0xf000) | (static_cast<std::uint32_t>(LOWORD(app.state.lpPlanets[iMin].rgwtMin[3])) / 4 & 0xfff));
            for (j = 0; j < 3; j++) {
                app.state.lpPlanets[iMin].rgwtMin[j] = app.state.lpPlanets->rgwtMin[j];
                if (app.state.gd.fTutorial) {
                    app.state.lpPlanets[iMin].rgMinConc[j] = 25 <= app.state.lpPlanets->rgMinConc[j] ? app.state.lpPlanets->rgMinConc[j] : 25;
                } else {
                    app.state.lpPlanets[iMin].rgMinConc[j] = 30 <= app.state.lpPlanets->rgMinConc[j] ? app.state.lpPlanets->rgMinConc[j] : 30;
                }
            }
            app.state.lpPlanets[iMin].iScanner = 0;
            TurnIO().pfnMessage(app, i, idmHomePlanetPeopleReadyLeaveNestExplore, static_cast<MsgGoto>(iMin), iMin, 0, 0, 0, 0, 0, 0);
            if (50 < CAdvantagePoints(app, &app.state.rgplr[i])) {
                iT = 50;
            } else {
                iT = CAdvantagePoints(app, &app.state.rgplr[i]);
            }
            if (app.state.rgplr[i].fAi) {
                iT = 50;
                if (app.state.rgplr[i].lvlAi >= lvlAiExpert) {
                    app.state.lpPlanets[iMin].rgwtMin[3] += static_cast<std::int32_t>((app.state.lpPlanets[iMin].rgwtMin[3] / 10));
                }
            }
            if (app.state.game.fBBSPlay) {
                pct10 = PctTrueMaxGrowth(app, i) * 2 + 10;
                app.state.lpPlanets[iMin].rgwtMin[3] = static_cast<std::uint32_t>((app.state.lpPlanets[iMin].rgwtMin[3] * pct10));
                app.state.lpPlanets[iMin].rgwtMin[3] = static_cast<std::int32_t>((app.state.lpPlanets[iMin].rgwtMin[3] / 10));
                app.state.lpPlanets[iMin].uPopGuess <<= 2;
            }
            j = RaceQueryIO().pfnStat(&app.state.rgplr[i], rsUseLeftover);
            switch (j) {
            case 0:
            default:
                ktLeft = 10 * iT;
                pl = app.state.lpPlanets[iMin].rgwtMin;
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
                pl[iLow] += static_cast<std::int16_t>(((ktLeft >> 2) + (ktLeft & 3)));
                ktLeft >>= 2;
                for (j = 0; j < 3; j++) {
                    pl[j] += ktLeft;
                }
                if (app.state.rgplr[i].fAi && app.state.rgplr[i].lvlAi >= lvlAiTough)
                    goto LConcentrations;
                break;
            case 1:
            LConcentrations:
                if (iT > 0 && iT < 3) {
                    ktLeft = 1;
                } else {
                    ktLeft = iT / 2;
                }
                pb = app.state.lpPlanets[iMin].rgMinConc;
                iLow = 0;
                for (j = 0; j < 3; j++) {
                    if (static_cast<std::int16_t>(pb[j]) < pb[iLow]) {
                        iLow = j;
                    }
                }
                pb[iLow] += ktLeft;
                ktLeft = static_cast<std::int16_t>((ktLeft + 1)) / 2;
                for (j = 0; j < 3; j++) {
                    pb[j] += ktLeft;
                }
                break;
            case 2:
                app.state.lpPlanets[iMin].cMines += iT >> 1;
                break;
            case 3:
                app.state.lpPlanets[iMin].cFactories += iT / 5;
                break;
            case 4:
                app.state.lpPlanets[iMin].cDefenses += static_cast<std::int16_t>((iT + 5)) / 10;
            }
            if (RaceQueryIO().pfnStat(&app.state.rgplr[i], rsMajorAdv) == raMacintosh) {
                app.state.lpPlanets[iMin].cMines = 0;
                app.state.lpPlanets[iMin].cFactories = 0;
                app.state.lpPlanets[iMin].cDefenses = 0;
            }
            app.state.rgplr[i].iPlayer = i;
            app.state.rgplr[i].idPlanetHome = iMin;
            if (app.state.rgplr[i].rgEnvVarMax[0] == envImmune) {
                iT = RandomIO().pfnRandom(app, 99) + 1;
            } else {
                iT = app.state.rgplr[i].rgEnvVarMin[0] + static_cast<std::int16_t>((app.state.rgplr[i].rgEnvVarMax[0] - app.state.rgplr[i].rgEnvVarMin[0])) / 2;
            }
            app.state.lpPlanets[iMin].rgEnvVar[0] = app.state.lpPlanets[iMin].rgEnvVarOrig[0] = iT;
            if (app.state.rgplr[i].rgEnvVarMax[1] == envImmune) {
                iT = RandomIO().pfnRandom(app, 99) + 1;
            } else {
                iT = app.state.rgplr[i].rgEnvVarMin[1] + static_cast<std::int16_t>((app.state.rgplr[i].rgEnvVarMax[1] - app.state.rgplr[i].rgEnvVarMin[1])) / 2;
            }
            app.state.lpPlanets[iMin].rgEnvVar[1] = app.state.lpPlanets[iMin].rgEnvVarOrig[1] = iT;
            if (app.state.rgplr[i].rgEnvVarMax[2] == envImmune) {
                iT = RandomIO().pfnRandom(app, 99) + 1;
            } else {
                iT = app.state.rgplr[i].rgEnvVarMin[2] + static_cast<std::int16_t>((app.state.rgplr[i].rgEnvVarMax[2] - app.state.rgplr[i].rgEnvVarMin[2])) / 2;
            }
            app.state.lpPlanets[iMin].rgEnvVar[2] = app.state.lpPlanets[iMin].rgEnvVarOrig[2] = iT;
            if (!app.state.rgplr[i].fAi) {
                app.state.rgplr[i].pctResearch = 15;
            }
            app.state.rgplr[i].SetITechNow(0);
            app.state.rgplr[i].SetITechNext(6);
            app.state.rgplr[i].lResLastYear = 0;
            app.state.rgplr[i].wScore = 0;
            for (j = 0; j < app.state.game.cPlayer; j++) {
                app.state.rgplr[i].rgmdRelation[j] = 0;
            }
            app.state.rgplr[i].cshdefSB = 1;
            lpshdef = LpAlloc<SHDEF>(app, 10 * sizeof(SHDEF), htShips);
            std::memmove(lpshdef, LpshdefSBT(), 4 * sizeof(SHDEF));
            std::fill_n(lpshdef + 4, 6, SHDEF{});
            lpshdef->cBuilt = 1;
            lpshdef->cExist = 1;
            app.state.rglpshdefSB[i] = lpshdef;
            for (j = 1; j < 10; j++) {
                lpshdef[j].fFree = 1;
            }
            if (RaceQueryIO().pfnStat(&app.state.rgplr[i], rsMajorAdv) == raMassAccel) {
                lpshdef->hul.rghs[0].iItem = ispecialSBMassDriver5;
                lpshdef->hul.rghs[0].cItem = 1;
                if (static_cast<std::int16_t>(app.state.game.mdSize) > sizeTiny) {
                    lpshdef[1].fFree = 0;
                    app.state.rgplr[i].cshdefSB++;
                    lpshdef[1].cExist = 1;
                    lpshdef[1].cBuilt = 1;
                }
            } else if (RaceQueryIO().pfnStat(&app.state.rgplr[i], rsMajorAdv) == raStargate && !app.state.game.fTutorial) {
                lpshdef->hul.rghs[0].iItem = ispecialSBStargate100250;
                lpshdef->hul.rghs[0].cItem = 1;
                if (static_cast<std::int16_t>(app.state.game.mdSize) > sizeTiny) {
                    lpshdef[1] = lpshdef[2];
                    lpshdef[1].ishdef = 17;
                    lpshdef[1].fFree = 0;
                    app.state.rgplr[i].cshdefSB++;
                    lpshdef[1].cExist = 1;
                    lpshdef[1].cBuilt = 1;
                }
            } else if (RaceQueryIO().pfnStat(&app.state.rgplr[i], rsMajorAdv) == raMacintosh) {
                idHome = app.state.rgplr[i].idPlanetHome;
                lpshdef[1] = *lpshdef;
                lpshdef[1].ishdef = 17;
                *lpshdef = lpshdef[3];
                lpshdef->ishdef = 16;
                lpshdef->fFree = 0;
                app.state.rgplr[i].cshdefSB++;
                lpshdef->cExist = 0;
                lpshdef->cBuilt = 0;
                app.state.lpPlanets[idHome].isb = 1;
            }
        }
        app.state.cFleet = 0;
        app.state.rglpfl = LpAlloc<FLEET *>(app, sizeof(FLEET *), htMisc);
        for (i = 0; i < app.state.game.cPlayer; i++) {
            app.state.idPlayer = i;
            lpshdef = LpAlloc<SHDEF>(app, 16 * sizeof(SHDEF), htShips);
            std::fill_n(lpshdef, 16, SHDEF{});
            for (j = 0; j < 16; j++) {
                lpshdef[j].fFree = 1;
            }
            app.state.rglpshdef[i] = lpshdef;
            idHome = app.state.rgplr[i].idPlanetHome;
            raMajor = static_cast<RaceAttribute>(RaceQueryIO().pfnStat(&app.state.rgplr[i], rsMajorAdv));
            switch (raMajor) {
            case raMassAccel:
                CreateStartupShip(app, i, idHome, 4, 1);
                app.state.lpPlanets[idHome].iWarpFling = 1;
                break;
            case raAttack:
                CreateStartupShip(app, i, idHome, 3, 1);
                if (app.state.rgplr[i].rgTech[3] < 3)
                    break;
                CreateStartupShip(app, i, idHome, 7, 1);
                CreateStartupShip(app, i, idHome, 13, 1);
                break;
            case raNone:
                CreateStartupShip(app, i, idHome, 3, 1);
                CreateStartupShip(app, i, idHome, 4, 1);
                break;
            case raStealth:
                CreateStartupShip(app, i, idHome, app.state.rgplr[i].rgTech[0] < 2 ? 2 : 5, 1);
                if (app.state.rgplr[i].fAi)
                    break;
                CreateStartupShip(app, i, idHome, 1, 1);
                break;
            default:
                CreateStartupShip(app, i, idHome, 2, 1);
            }
            switch (raMajor) {
            case raCheapCol:
                ishRet = CreateStartupShip(app, i, idHome, 12, 1);
                for (j = 1; j < 3; j++) {
                    CreateStartupShip(app, i, idHome, ishRet, 0);
                }
                break;
            case raStargate:
                CreateStartupShip(app, i, idHome, 11, 1);
                break;
            case raMacintosh:
                CreateStartupShip(app, i, idHome, 10, 1);
                break;
            default:
                ishRet = CreateStartupShip(app, i, idHome, 9, 1);
            }
            if (raMajor == raMines) {
                CreateStartupShip(app, i, idHome, 16, 1);
                CreateStartupShip(app, i, idHome, 18, 1);
            } else if (raMajor == raTerra) {
                CreateStartupShip(app, i, idHome, 17, 1);
            } else if (raMajor == raStargate) {
                CreateStartupShip(app, i, idHome, 7, 1);
                CreateStartupShip(app, i, idHome, 8, 1);
                if (static_cast<std::int16_t>(app.state.game.mdSize) > sizeTiny)
                    goto LGive2ndPlanet;
            } else if (raMajor == raMassAccel && static_cast<std::int16_t>(app.state.game.mdSize) > sizeTiny) {
            LGive2ndPlanet:
                lpplPicked = nullptr;
                lpplClosest = nullptr;
                ptHome = app.state.rgptPlan[idHome];
                cFit = 0;
                lDistMin2 = static_cast<std::int32_t>((app.state.dGal * 15)) / 100;
                lDistMin2 = static_cast<std::uint32_t>((lDistMin2 * lDistMin2));
                lDistMax2 = static_cast<std::int32_t>((app.state.dGal * 23)) / 100;
                lDistMax2 = static_cast<std::uint32_t>((lDistMax2 * lDistMax2));
                lDistIdeal2 = static_cast<std::int32_t>((app.state.dGal * 20)) / 100;
                lDistIdeal2 = static_cast<std::uint32_t>((lDistIdeal2 * lDistIdeal2));
                lBest = 10000000;
                pptMax = &app.state.rgptPlan[cPlanMax];
                ppt = app.state.rgptPlan;
                lppl = app.state.lpPlanets;
                while (ppt < pptMax) {
                    if (lppl->iPlayer == iplrNone) {
                        dx = ppt->x - ptHome.x;
                        dy = ppt->y - ptHome.y;
                        lDistCur2 = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
                        if (lDistCur2 >= lDistMin2 && lDistCur2 <= lDistMax2) {
                            cFit++;
                            if (RandomIO().pfnRandom(app, cFit) == 0) {
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
                while (PlanetQueryIO().pfnDesirability(app, lpplPicked, i) < 10 && cFit++ < 100) {
                    for (j = 0; j < 3; j++) {
                        lpplPicked->rgEnvVar[j] = lpplPicked->rgEnvVarOrig[j] = RandomIO().pfnRandom(app, 97) + 2;
                    }
                }
                if (cFit >= 100) {
                    for (j = 0; j < 3; j++) {
                        lpplPicked->rgEnvVar[j] = app.state.lpPlanets[idHome].rgEnvVar[j];
                        lpplPicked->rgEnvVarOrig[j] = app.state.lpPlanets[idHome].rgEnvVarOrig[j];
                    }
                }
                lpplPicked->iPlayer = i;
                lpplPicked->fStarbase = 1;
                lpplPicked->isb = 1;
                lpplPicked->fArtifact = 0;
                lpplPicked->cFactories = 4;
                lpplPicked->cMines = 10;
                lpplPicked->rgwtMin[3] = static_cast<std::int32_t>((app.state.lpPlanets[idHome].rgwtMin[3] * 2)) / 5;
                lpplPicked->uPopGuess = static_cast<std::uint32_t>(LOWORD(lpplPicked->rgwtMin[3])) / 4;
                for (j = 0; j < 3; j++) {
                    lpplPicked->rgwtMin[j] = static_cast<std::int16_t>((RandomIO().pfnRandom(app, 200) + 100));
                }
                lpplPicked->iScanner = 0;
                app.state.lpPlanets[idHome].rgwtMin[3] = static_cast<std::int32_t>((app.state.lpPlanets[idHome].rgwtMin[3] * 4)) / 5;
                app.state.lpPlanets[idHome].SetUGuesses((app.state.lpPlanets[idHome].uGuesses() & 0xf000) | (static_cast<std::uint32_t>(LOWORD(app.state.lpPlanets[idHome].rgwtMin[3])) / 4 & 0xfff));
                CreateStartupShip(app, i, lpplPicked->id, 0, 0);
            } else if (raMajor == raNone) {
                CreateStartupShip(app, i, idHome, app.state.rgplr[i].rgTech[3] < 4 ? 6 : 8, 1);
                CreateStartupShip(app, i, idHome, 7, 1);
                CreateStartupShip(app, i, idHome, 14, 1);
            }
            if (RaceQueryIO().pfnGrbit(&app.state.rgplr[i], ibitRaceOBRM) == 0 && RaceQueryIO().pfnGrbit(&app.state.rgplr[i], ibitRaceARM) != 0) {
                ishRet = CreateStartupShip(app, i, idHome, 15, 1);
                CreateStartupShip(app, i, idHome, ishRet, 0);
            }
            for (j = 0; j < app.state.rgplr[i].cShDef; j++) {
                chs = app.state.rglpshdef[i][j].hul.chs;
                lphs = app.state.rglpshdef[i][j].hul.rghs;
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
                        if (app.state.rgplr[i].rgEnvVar[2] == envImmune || app.state.rglpshdef[i][j].hul.ihuldef != ihuldefColonyShip || app.state.rgplr[i].rgEnvVar[2] >= 85) {
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
                        if (PartQueryIO().pfnLookup(app, &part) == mdPartAvailAvailable) {
                            lphs->iItem = rgTry[l];
                            break;
                        }
                    }
                    k++;
                    lphs++;
                }
            }
            if (RaceQueryIO().pfnStat(&app.state.rgplr[i], rsMajorAdv) == raMacintosh) {
                app.state.lpPlanets[idHome].iScanner = 31;
            }
        }
        app.state.idPlayer = iplrNone;
        if (app.state.lpPlanets->iPlayer == iplrNone) {
            for (j = 0; j < 3; j++) {
                app.state.lpPlanets->rgwtMin[j] = 0;
            }
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            app.state.rgcbtlplan[i] = 5;
            app.state.rglpbtlplan[i] = LpAlloc<BTLPLAN>(app, 16 * sizeof(BTLPLAN), htShips);
            for (j = 0; j < 5; j++) {
                InitBattlePlan(app, app.state.rglpbtlplan[i] + j, j, i);
            }
        }
        app.state.game.cPlanMax = cPlanMax;
        if (app.state.game.fNoRandom) {
            iBest = 0;
        } else {
            iBest = vrgWormholeMin[app.state.game.mdSize] + RandomIO().pfnRandom(app, vrgWormholeVar[app.state.game.mdSize]);
        }
        if (iBest > 0) {
            for (i = 0; i < iBest; i++) {
                for (j = 0; j < 2; j++) {
                    lpth = LpthNew(app, 0, ithWormhole);
                    { const auto iStableForThing = (RandomIO().pfnRandom(app, 3)); auto &thForThing = *(lpth); thForThing.thw().SetIStable(iStableForThing); }
                    if (j == 1) {
                        lpthLast = LpthFromId(app, idLast);
                        { const auto idPartnerForThing = (lpthLast->idFull()); auto &thForThing = *(lpth); thForThing.thw().SetIdPartner(idPartnerForThing); }
                        { const auto idPartnerForThing = (lpth->idFull()); auto &thForThing = *(lpthLast); thForThing.thw().SetIdPartner(idPartnerForThing); }
                    } else {
                        idLast = lpth->idFull();
                    }
                    k = 0;
                    iMax = 16;
                    while (k++ < 100) {
                        { const auto xForThing = (RandomIO().pfnRandom(app, app.state.dGal) + 1000); auto &thForThing = *(lpth); thForThing.pt().SetX(xForThing); }
                        { const auto yForThing = (RandomIO().pfnRandom(app, app.state.dGal) + 1000); auto &thForThing = *(lpth); thForThing.pt().SetY(yForThing); }
                        iLow = IValidateWormholePos(app, lpth);
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
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (app.state.rgplr[i].fHacker) {
                TurnIO().pfnMessage2(app, i, idmRaceDefinitionHasTamperedStatisticsHaveAltered, gotoNone, 0, 0);
                for (j = 0; j < app.state.game.cPlayer; j++) {
                    if (i != j && !app.state.rgplr[i].fAi) {
                        TurnIO().pfnMessage2(app, j, idmHackedRaceDiscoveredRaceStatisticsHaveAltered, gotoNone, i, 0);
                    }
                }
            }
        }
        iplrSingle = iplrNone;
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (!app.state.rgplr[i].fAi || app.state.rgplr[i].idAi == idAiMaid) {
                if (iplrSingle != iplrNone)
                    break;
                iplrSingle = i;
            } else {
                app.state.rgplr[i].lSalt = 156085230;
            }
        }
        app.state.game.fSinglePlr = i == app.state.game.cPlayer && iplrSingle != iplrNone;
        if (app.state.game.fSinglePlr) {
            for (i = 0; i < app.state.game.cPlayer; i++) {
                for (j = 0; j < app.state.game.cPlayer; j++) {
                    if (i != j) {
                        app.state.rgplr[i].rgmdRelation[j] = 2;
                    }
                }
            }
        }
        if (!app.state.game.fTutorial) {
            app.state.game.lid = DwTickCount();
        }
        CchSprintf(app.state.szWork, "%s.xy", app.state.szBase);
        if (!RecordIO().pfnFCreateFile(app, dtXY, iplrNone, nullptr)) {
            AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableCreateUniverseDefinitionFile, nullptr), win32::MB_ICONHAND);
            DestroyCurGame(app);
            return 0;
        }
        FEncodeGame(app.state.game, rgbGame);
        RecordIO().pfnWriteRt(app, rtGame, cbWireGame, rgbGame);
        xOld = 1000;
        for (i = 0; i < cPlanMax; i++) {
            starpack.y = LOWORD(app.state.rgptPlan[i].y);
            starpack.id = LOWORD(app.state.rgidPlan[i]);
            dx = app.state.rgptPlan[i].x - xOld;
            starpack.dx = dx;
            FEncodeStarpack(starpack, rgbStarpack);
            RgToStream(app, rgbStarpack, cbWireStarpack);
            xOld = app.state.rgptPlan[i].x;
        }
        i = app.state.game.cPlayer;
        WriteRtEof(app, i);
        RecordIO().pfnStreamClose(app);
        for (i = -1; i < app.state.game.cPlayer; i++) {
            FWriteDataFile(app, app.state.szBase, i, 0);
        }
        if (fBatchMode) {
            return 1;
        }
        if (app.state.game.fSinglePlr) {
            DestroyCurGame(app);
            CchSprintf(szExt, app.state.MPCTD, iplrSingle + 1);
            if (!FLoadGame(app, app.state.szBase, szExt)) {
                AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUnableOpenNewTurnFile, nullptr), win32::MB_ICONHAND);
                return 0;
            }
            app.state.idPlayer = iplrSingle;
            CreateChildWindows(app);
            PostOpenGame(app);
        } else {
            app.state.idPlayer = iplrNone;
            app.state.imemLogCur = 0;
            CreateChildWindows(app);
        }
        return 1;
    } catch (const StarsError &) {
        DestroyCurGame(app);
        return 0;
    }
}

std::int16_t CreateStartupShip(APPSESSION &app, std::int16_t iplr, std::int16_t idPlanet, std::int16_t ishdef, std::int16_t fAddShdef) {
    std::int16_t ishMac;
    FLEET  *lpfl;

    if (fAddShdef) {
        ishMac = static_cast<std::int16_t>(static_cast<std::int8_t>(app.state.rgplr[iplr].cShDef++));
        std::memmove(app.state.rglpshdef[iplr] + ishMac, LpshdefT() + ishdef, sizeof(SHDEF));
        app.state.rglpshdef[iplr][ishMac].ishdef = ishMac;
        ishdef = ishMac;
    }
    app.state.rglpshdef[iplr][ishdef].cExist++;
    app.state.rglpshdef[iplr][ishdef].cBuilt++;
    lpfl = ProductionIO().pfnNewFleet(app, iplr, idPlanet);
    lpfl->rgcsh[ishdef] = 1;
    lpfl->rgwtMin[4] = ProductionIO().pfnFleetStat(app, lpfl, 1);
    lpfl->iplan = 0;
    return ishdef;
}

std::int16_t GenNewGameFromFile(APPSESSION &app, char *pszFile) {
    std::int32_t rgl[10];
    std::int16_t cPlr;
    std::int16_t rgplrbmp[16];
    std::int16_t cNum;
    std::int16_t c;
    std::int16_t i;
    std::int16_t fSuccess;
    std::int16_t fReadRace;
    char   *lpbStart;
    std::int16_t j;
    char   *lpb;
    char   *lpbDef;
    std::int16_t cb;
    std::int32_t cbFile;
    char   *pchT;
    AiRace  idAi;
    std::int16_t lvlAi;

    fSuccess = 0;
    std::strcpy(app.state.szWork, pszFile);
    StarsRecoveryScope scopeRecovery;
    try {
        if (app.state.ini.fLogging) {
            /* NATIVE: the host passes szBase itself; see FLoadGame. */
            if (pszFile != app.state.szBase) {
                std::strcpy(app.state.szBase, pszFile);
            }
            pchT = std::strrchr(app.state.szBase, 46);
            *pchT = 0;
            TurnLog(app, idsGeneratingYearD);
        }
        app.state.game = {};
        RecordIO().pfnStreamOpen(app, pszFile, mdRead);
        cbFile = CbFileSize(app.state.hf);
        if (cbFile < 0 || cbFile >= 16000) {
            FileError(app, idsUniverseCreationFileAppearsInvalid);
            goto LError;
        }
        cb = static_cast<std::int16_t>(cbFile);
        lpbDef = LpAlloc<char>(app, cb + 1, htPerm);
        app.state.lpbDefMac = lpbDef + cb;
        RgFromStream(app, lpbDef, cb);
        RecordIO().pfnStreamClose(app);
        lpbDef[cb] = 0;
        if (!FUtf8Valid(lpbDef, cb)) {
            FileError(app, idsUniverseCreationFileAppearsInvalid);
            goto LError;
        }
        lpb = lpbDef;
        if (cb >= 3 && !std::memcmp(lpb, "\xef\xbb\xbf", 3)) lpb += 3;
        lpbStart = PszGetLine(&lpb);
        if (*lpbStart == 0 || std::strlen(lpbStart) > 31) {
            AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsIllegalGameTitle, nullptr), win32::MB_ICONHAND);
            goto LError;
        }
        if (CchCopyUtf8(app.state.game.szName, sizeof(app.state.game.szName), lpbStart, std::strlen(lpbStart)) < 0) {
            FileError(app, idsGameFileAppearsCorruptUnableLoadFile);
            goto LError;
        }
        if (lpb >= app.state.lpbDefMac) {
        LUniDefShort:
            AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsUniverseDefinitionFileAppearsTooShort, nullptr), win32::MB_ICONHAND);
            goto LError;
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 4);
        if (cNum == -1) {
        LUniDefError:
            AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsLine2HasBadUniverseDefinitionParameter, nullptr), win32::MB_ICONHAND);
            goto LError;
        }
        for (i = 0; i < cNum; i++) {
            if (i < 3 && (rgl[i] < 0 || rgl[i] > 4 || (rgl[i] == 4 && i != 0)))
                goto LUniDefError;
        }
        if (cNum >= 1) {
            app.state.game.mdSize = static_cast<UniverseSize>(LOWORD(rgl[0]));
        }
        if (cNum >= 2) {
            app.state.game.mdDensity = static_cast<UniverseDensity>(LOWORD(rgl[1]));
        }
        if (cNum >= 3) {
            app.state.game.mdStartDist = static_cast<StartDistance>(LOWORD(rgl[2]));
        }
        if (cNum >= 4) {
            Randomize(app, rgl[3]);
        }
        if (lpb >= app.state.lpbDefMac)
            goto LUniDefShort;
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 7);
        if (cNum == -1) {
        LUniDefError3:
            AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsLine3HasBadUniverseDefinitionParameter, nullptr), win32::MB_ICONHAND);
            goto LError;
        }
        for (i = 0; i < cNum; i++) {
            if (rgl[i] < 0 || rgl[i] > 1)
                goto LUniDefError3;
        }
        if (cNum >= 1) {
            app.state.game.fExtraFuel = LOWORD(rgl[0]);
        }
        if (cNum >= 2) {
            app.state.game.fSlowTech = LOWORD(rgl[1]);
        }
        if (cNum >= 3) {
            app.state.game.fBBSPlay = LOWORD(rgl[2]);
        }
        if (cNum >= 4) {
            app.state.game.fNoRandom = LOWORD(rgl[3]);
        }
        if (cNum >= 5) {
            app.state.game.fAisBand = LOWORD(rgl[4]);
        }
        if (cNum >= 6) {
            app.state.game.fVisScores = LOWORD(rgl[5]);
        }
        if (cNum >= 7) {
            app.state.game.fClumping = LOWORD(rgl[6]);
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 1);
        if (cNum < 1 || rgl[0] < 1 || rgl[0] > 16) {
            AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, idsLine4HasImproperNumberPlayerFiles, nullptr), win32::MB_ICONHAND);
            goto LError;
        }
        if (lpb >= app.state.lpbDefMac)
            goto LUniDefShort;
        cPlr = LOWORD(rgl[0]);
        app.state.game.cPlayer = cPlr;
        for (i = 0; i < cPlr; i++) {
            lpbStart = PszGetLine(&lpb);
            if (lpb >= app.state.lpbDefMac)
                goto LUniDefShort;
            if (std::strlen(lpbStart) >= sizeof(app.state.szWork)) {
                FileError(app, idsUniverseCreationFileAppearsInvalid);
                goto LError;
            }
            if (i > 0 && *lpbStart == '#') {
                cNum = CParseNumbers(lpbStart + 1, rgl, 2);
                idAi = static_cast<AiRace>(LOWORD(rgl[0]));
                lvlAi = LOWORD(rgl[1]);
                if (cNum < 2 || static_cast<std::int16_t>(idAi) < idAiRobotoid || static_cast<std::int16_t>(idAi) > idAiRandom || lvlAi < 0 || lvlAi > 4) {
                    std::strcpy(app.state.szWork, lpbStart);
                    goto LCantGetRace;
                }
                if (lvlAi == 0) {
                    lvlAi = RandomIO().pfnRandom(app, 4);
                } else {
                    lvlAi--;
                }
                if (idAi == idAiRobotoid) {
                    idAi = static_cast<AiRace>(RandomIO().pfnRandom(app, 6));
                } else {
                    idAi = static_cast<AiRace>(idAi - 1);
                }
                app.state.rgplr[i] = *LpplrComp(idAi, static_cast<AiLevel>(lvlAi));
                app.state.rgplr[i].fAi = 1;
                app.state.rgplr[i].idAi = idAi;
                app.state.rgplr[i].lvlAi = lvlAi;
            } else {
                std::strcpy(app.state.szWork, lpbStart);
                fReadRace = FWasRaceFile(app, app.state.szWork, 0);
                if (fReadRace != 1) {
                LCantGetRace:
                    CchSprintf(app.state.szWork, TextIO().pfnCompressed(app, idsLineDUnableLoadRaceFileS), i + 5, lpbStart);
                    AlertIO().pfnAlertSz(app, app.state.szWork, win32::MB_ICONHAND);
                    goto LError;
                }
                app.state.rgplr[i] = app.state.vplr;
            }
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 2);
        if (lpb >= app.state.lpbDefMac)
            goto LUniDefShort;
        i = 0;
        if (cNum < 1 || rgl[0] < 0 || rgl[0] > 1) {
        LBadDefVc:
            i += cPlr + 5;
            CchSprintf(app.state.szWork, TextIO().pfnCompressed(app, idsLineDHasImproperVictoryConditionDefinition), i);
            AlertIO().pfnAlertSz(app, app.state.szWork, win32::MB_ICONHAND);
            goto LError;
        }
        if (rgl[0] == 1) {
            if (cNum < 2 || rgl[1] < 20 || rgl[1] > 100)
                goto LBadDefVc;
            SetVCCheck(&app.state.game, vcOwnsPercentPlanets, 1);
            SetVCVal(app, &app.state.game, vcOwnsPercentPlanets, static_cast<std::int16_t>((LOWORD(rgl[1]) - 20)) / 5);
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 3);
        if (lpb >= app.state.lpbDefMac)
            goto LUniDefShort;
        i = 1;
        if (cNum < 1 || rgl[0] < 0 || rgl[0] > 1)
            goto LBadDefVc;
        if (rgl[0] == 1) {
            if (cNum < 3 || rgl[1] < 8 || rgl[1] > 26 || rgl[2] < 2 || rgl[2] > 6)
                goto LBadDefVc;
            SetVCCheck(&app.state.game, vcAttainsTechLevel, 1);
            SetVCCheck(&app.state.game, vcAttainsTechFields, 1);
            SetVCVal(app, &app.state.game, vcAttainsTechLevel, LOWORD(rgl[1]) - 8);
            SetVCVal(app, &app.state.game, vcAttainsTechFields, LOWORD(rgl[2]) - 2);
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 2);
        if (lpb >= app.state.lpbDefMac)
            goto LUniDefShort;
        i = 2;
        if (cNum < 1 || rgl[0] < 0 || rgl[0] > 1)
            goto LBadDefVc;
        if (rgl[0] == 1) {
            if (cNum < 2 || rgl[1] < 1000 || rgl[1] > 20000)
                goto LBadDefVc;
            SetVCCheck(&app.state.game, vcExceedsScore, 1);
            SetVCVal(app, &app.state.game, vcExceedsScore, static_cast<std::int16_t>((LOWORD(rgl[1]) - 1000)) / 1000);
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 2);
        if (lpb >= app.state.lpbDefMac)
            goto LUniDefShort;
        i = 3;
        if (cNum < 1 || rgl[0] < 0 || rgl[0] > 1)
            goto LBadDefVc;
        if (rgl[0] == 1) {
            if (cNum < 2 || rgl[1] < 20 || rgl[1] > 300)
                goto LBadDefVc;
            SetVCCheck(&app.state.game, vcExceedsSecondPlaceBy, 1);
            SetVCVal(app, &app.state.game, vcExceedsSecondPlaceBy, static_cast<std::int16_t>((LOWORD(rgl[1]) - 20)) / 10);
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 2);
        if (lpb >= app.state.lpbDefMac)
            goto LUniDefShort;
        i = 4;
        if (cNum < 1 || rgl[0] < 0 || rgl[0] > 1)
            goto LBadDefVc;
        if (rgl[0] == 1) {
            if (cNum < 2 || rgl[1] < 10 || rgl[1] > 500)
                goto LBadDefVc;
            SetVCCheck(&app.state.game, vcProductionCapacity, 1);
            SetVCVal(app, &app.state.game, vcProductionCapacity, static_cast<std::int16_t>((LOWORD(rgl[1]) - 10)) / 10);
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 2);
        if (lpb >= app.state.lpbDefMac)
            goto LUniDefShort;
        i = 5;
        if (cNum < 1 || rgl[0] < 0 || rgl[0] > 1)
            goto LBadDefVc;
        if (rgl[0] == 1) {
            if (cNum < 2 || rgl[1] < 10 || rgl[1] > 300)
                goto LBadDefVc;
            SetVCCheck(&app.state.game, vcOwnsCapitalShips, 1);
            SetVCVal(app, &app.state.game, vcOwnsCapitalShips, static_cast<std::int16_t>((LOWORD(rgl[1]) - 10)) / 10);
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 2);
        if (lpb >= app.state.lpbDefMac)
            goto LUniDefShort;
        i = 6;
        if (cNum < 1 || rgl[0] < 0 || rgl[0] > 1)
            goto LBadDefVc;
        if (rgl[0] == 1) {
            if (cNum < 2 || rgl[1] < 30 || rgl[1] > 900)
                goto LBadDefVc;
            SetVCCheck(&app.state.game, vcHighestScoreAfterYears, 1);
            SetVCVal(app, &app.state.game, vcHighestScoreAfterYears, static_cast<std::int16_t>((LOWORD(rgl[1]) - 30)) / 10);
        }
        lpbStart = PszGetLine(&lpb);
        cNum = CParseNumbers(lpbStart, rgl, 2);
        if (lpb >= app.state.lpbDefMac)
            goto LUniDefShort;
        i = 7;
        if (cNum < 1 || rgl[0] < 0 || rgl[0] > 7)
            goto LBadDefVc;
        if (rgl[0] > 0) {
            if (cNum < 2 || rgl[1] < 30 || rgl[1] > 500)
                goto LBadDefVc;
            SetVCVal(app, &app.state.game, vcMeetsNumCriteria, LOWORD(rgl[0]));
            SetVCVal(app, &app.state.game, vcMinYearsBeforeWin, static_cast<std::int16_t>((LOWORD(rgl[1]) - 30)) / 10);
        }
        lpbStart = PszGetLine(&lpb);
        lpb = lpbStart + (-1 + std::strlen(lpbStart));
        if (lpb - lpbStart >= 3 && *lpb == 'y' && lpb[-1] == 'x' && lpb[-2] == '.') {
            lpb[-2] = 0;
        }
        if (CchCopyUtf8(app.state.szBase, sizeof(app.state.szBase), lpbStart, std::strlen(lpbStart)) < 0) {
            FileError(app, idsUniverseCreationFileAppearsInvalid);
            goto LError;
        }
        if (lpb + 4 < app.state.lpbDefMac) {
            app.state.lpbDefUni = lpb;
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (!app.state.rgplr[i].fAi && CAdvantagePoints(app, &app.state.rgplr[i]) < 0) {
                app.state.rgplr[i] = vrgplrDef[0];
                app.state.rgplr[i].fHacker = 1;
            }
            if (app.state.rgplr[i].szName[0] == 0) {
                TextIO().pfnCchGetString(app, static_cast<StringId>(idsBerserker + RandomIO().pfnRandom(app, 24)), app.state.rgplr[i].szName);
                CchSprintf(app.state.rgplr[i].szNames, "%ss", app.state.rgplr[i].szName);
            }
        }
        for (i = 1; i < app.state.game.cPlayer; i++) {
            for (j = 0; j < i && std::strcmp(app.state.rgplr[i].szName, app.state.rgplr[j].szName) != 0; j++) {
            }
            if (j < i) {
                c = RandomIO().pfnRandom(app, 24);
                while (1) {
                    for (j = 0; j < app.state.game.cPlayer && std::strcmp(app.state.rgplr[j].szName, TextIO().pfnCompressed(app, static_cast<StringId>(idsBerserker + c))) != 0; j++) {
                    }
                    if (j == app.state.game.cPlayer)
                        break;
                    c++;
                    if (c >= 24) {
                        c = 0;
                    }
                }
                TextIO().pfnCchGetString(app, static_cast<StringId>(idsBerserker + c), app.state.rgplr[i].szName);
                std::strcpy(app.state.rgplr[i].szNames, app.state.rgplr[i].szName);
                std::strcat(app.state.rgplr[i].szNames, "s");
            }
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            rgplrbmp[i] = app.state.rgplr[i].iPlrBmp;
            if (rgplrbmp[i] < 0 || rgplrbmp[i] >= 32) {
                rgplrbmp[i] = -1;
            }
        }
        for (i = 1; i < app.state.game.cPlayer; i++) {
            if (rgplrbmp[i] != -1) {
                for (j = 0; j < i && rgplrbmp[j] != rgplrbmp[i]; j++) {
                }
                if (j < i) {
                    rgplrbmp[RandomIO().pfnRandom(app, 2) == 0 ? j : i] = -1;
                }
            }
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            if (rgplrbmp[i] == -1) {
                rgplrbmp[i] = RandomIO().pfnRandom(app, 32);
                while (1) {
                    for (j = 0; j < app.state.game.cPlayer && (j == i || rgplrbmp[i] != rgplrbmp[j]); j++) {
                    }
                    if (j == app.state.game.cPlayer)
                        break;
                    rgplrbmp[i]++;
                    if (rgplrbmp[i] >= 32) {
                        rgplrbmp[i] = 0;
                    }
                }
            }
        }
        for (i = 0; i < app.state.game.cPlayer; i++) {
            app.state.rgplr[i].iPlrBmp = rgplrbmp[i];
        }
        GenerateWorld(app, 1);
        fSuccess = 1;
    } catch (const StarsError &) {
    }
LError:
    RecordIO().pfnStreamClose(app);
    app.state.lpbDefUni = nullptr;
    TurnLog(app, static_cast<StringId>(idsFailed + fSuccess));
    return fSuccess;
}

void CreateTutorWorld(APPSESSION &app) {
    std::int16_t i;

    app.state.game = {};
    app.state.game.cPlayer = 2;
    app.state.game.fTutorial = 1;
    app.state.game.mdDensity = densitySparse;
    app.state.game.mdSize = sizeTiny;
    app.state.game.mdStartDist = startDistModerate;
    app.state.game.fBBSPlay = 1;
    app.state.game.fVisScores = 1;
    app.state.game.fNoRandom = 1;
    app.state.game.lid = 9236297;
    app.state.game.rgvc[7] = 128;
    app.state.game.rgvc[8] = 129;
    TextIO().pfnCchGetString(app, idsTutorialGame, app.state.game.szName);
    app.state.rgplr[0] = vrgplrDef[0];
    TextIO().pfnCchGetString(app, idsHumanoid, app.state.rgplr[0].szName);
    CchSprintf(app.state.rgplr[0].szNames, "%ss", app.state.rgplr[0].szName);
    app.state.rgplr[1] = *LpplrComp(idAiTurinDrone, lvlAiEasy);
    app.state.rgplr[1].fAi = 1;
    app.state.rgplr[1].lvlAi = lvlAiEasy;
    app.state.rgplr[1].idAi = idAiTurinDrone;
    TextIO().pfnCchGetString(app, idsBerserker, app.state.rgplr[1].szName);
    Randomize(app, 1234567890);
    for (i = 1; i <= 2; i++) {
        CchSprintf(app.state.szWork, TextIO().pfnCompressed(app, idsSHD), app.state.szBase, i);
        RemoveNativeFile(app.state.szWork);
        CchSprintf(app.state.szWork, TextIO().pfnCompressed(app, idsSXD), app.state.szBase, i);
        RemoveNativeFile(app.state.szWork);
    }
    GenerateWorld(app, 0);
    return;
}

void InitNewGamePlr(APPSESSION &app, std::int16_t iStepMaxSoFar, AiLevel lvlAi) {
    std::int16_t i;
    std::int16_t c;
    std::uint8_t ch;

    if (iStepMaxSoFar < 2 && !app.state.fRCWReadOnly) {
        SetVCVal(app, &app.state.game, vcMinYearsBeforeWin, app.state.game.mdSize * 2);
        if (iStepMaxSoFar < 1) {
            switch (app.state.game.mdSize) {
            case sizeTiny:
                if (lvlAi == lvlAiExpert && RandomIO().pfnRandom(app, 3) == 0) {
                    app.state.game.cPlayer = 3;
                    break;
                }
                app.state.game.cPlayer = 2;
                break;
            case sizeSmall:
                if (lvlAi == lvlAiExpert && RandomIO().pfnRandom(app, 4) == 0) {
                    app.state.game.cPlayer = 5;
                    break;
                }
                if (static_cast<std::int16_t>(lvlAi) >= lvlAiTough && RandomIO().pfnRandom(app, 6 - lvlAi) == 0) {
                    app.state.game.cPlayer = 4;
                    break;
                }
                app.state.game.cPlayer = 3;
                break;
            case sizeMedium:
                if (lvlAi == lvlAiExpert && RandomIO().pfnRandom(app, 10) == 0) {
                    app.state.game.cPlayer = 9;
                    break;
                }
                if (lvlAi == lvlAiExpert && RandomIO().pfnRandom(app, 10) == 0) {
                    app.state.game.cPlayer = 5;
                    break;
                }
                if (static_cast<std::int16_t>(lvlAi) >= lvlAiTough && RandomIO().pfnRandom(app, 7 - lvlAi) == 0) {
                    app.state.game.cPlayer = 8;
                    break;
                }
                if (static_cast<std::int16_t>(lvlAi) >= lvlAiTough && RandomIO().pfnRandom(app, 7 - lvlAi) == 0) {
                    app.state.game.cPlayer = 6;
                    break;
                }
                app.state.game.cPlayer = 7;
                break;
            case sizeLarge:
                if (lvlAi == lvlAiExpert && RandomIO().pfnRandom(app, 10) == 0) {
                    app.state.game.cPlayer = RandomIO().pfnRandom(app, 2) + 14;
                    break;
                }
                if (lvlAi == lvlAiExpert && RandomIO().pfnRandom(app, 10) == 0) {
                    app.state.game.cPlayer = 10 - RandomIO().pfnRandom(app, 2);
                    break;
                }
                if (static_cast<std::int16_t>(lvlAi) >= lvlAiTough && RandomIO().pfnRandom(app, 7 - lvlAi) == 0) {
                    app.state.game.cPlayer = 13;
                    break;
                }
                if (static_cast<std::int16_t>(lvlAi) >= lvlAiTough && RandomIO().pfnRandom(app, 7 - lvlAi) == 0) {
                    app.state.game.cPlayer = 11;
                    break;
                }
                app.state.game.cPlayer = 12;
                break;
            case sizeHuge:
                if (lvlAi == lvlAiExpert && RandomIO().pfnRandom(app, 10) == 0) {
                    app.state.game.cPlayer = 13 - RandomIO().pfnRandom(app, 3);
                } else if (static_cast<std::int16_t>(lvlAi) >= lvlAiTough && RandomIO().pfnRandom(app, 9 - lvlAi) == 0) {
                    app.state.game.cPlayer = 14;
                } else if (static_cast<std::int16_t>(lvlAi) >= lvlAiTough && RandomIO().pfnRandom(app, 7 - lvlAi) == 0) {
                    app.state.game.cPlayer = 15;
                } else {
                    app.state.game.cPlayer = 16;
                }
            }
            i = 1;
            switch (lvlAi) {
            default:
                break;
            case lvlAiEasy:
                while (i < app.state.game.cPlayer) {
                    if (i < static_cast<std::int16_t>((app.state.game.cPlayer + 1)) / 3 + 1) {
                        app.state.vrgplrTypeNew[i++] = 11;
                    } else if (i < static_cast<std::int16_t>(((app.state.game.cPlayer + 1) * 2)) / 3 + 1) {
                        app.state.vrgplrTypeNew[i++] = 15;
                    } else if (i < static_cast<std::int16_t>(((app.state.game.cPlayer + 1) * 5)) / 6 + 1) {
                        app.state.vrgplrTypeNew[i++] = 7;
                    } else {
                        app.state.vrgplrTypeNew[i++] = 27;
                    }
                }
                break;
            case lvlAiStandard:
                while (i < app.state.game.cPlayer) {
                    if (i < static_cast<std::int16_t>(((app.state.game.cPlayer + 5) * 2)) / 7 + 1) {
                        app.state.vrgplrTypeNew[i++] = 39;
                    } else if (i < static_cast<std::int16_t>(((app.state.game.cPlayer - 1) * 3 + 6)) / 7 + 1) {
                        app.state.vrgplrTypeNew[i++] = 35;
                    } else if (i < static_cast<std::int16_t>(((app.state.game.cPlayer - 1) * 4 + 6)) / 7 + 1) {
                        app.state.vrgplrTypeNew[i++] = 43;
                    } else if (i < static_cast<std::int16_t>(((app.state.game.cPlayer - 1) * 5 + 6)) / 7 + 1) {
                        app.state.vrgplrTypeNew[i++] = 47;
                    } else if (i < static_cast<std::int16_t>(((app.state.game.cPlayer - 1) * 6 + 6)) / 7 + 1) {
                        app.state.vrgplrTypeNew[i++] = 51;
                    } else {
                        app.state.vrgplrTypeNew[i++] = 155;
                    }
                }
                break;
            case lvlAiTough:
                while (i < app.state.game.cPlayer) {
                    if (i < static_cast<std::int16_t>(((app.state.game.cPlayer + 5) * 2)) / 7 + 1) {
                        app.state.vrgplrTypeNew[i++] = 83;
                    } else if (i < static_cast<std::int16_t>(((app.state.game.cPlayer - 1) * 3 + 6)) / 7 + 1) {
                        app.state.vrgplrTypeNew[i++] = 71;
                    } else if (i < static_cast<std::int16_t>(((app.state.game.cPlayer - 1) * 4 + 6)) / 7 + 1) {
                        app.state.vrgplrTypeNew[i++] = 67;
                    } else if (i < static_cast<std::int16_t>(((app.state.game.cPlayer - 1) * 5 + 6)) / 7 + 1) {
                        app.state.vrgplrTypeNew[i++] = 87;
                    } else if (i < static_cast<std::int16_t>(((app.state.game.cPlayer - 1) * 6 + 6)) / 7 + 1) {
                        app.state.vrgplrTypeNew[i++] = 91;
                    } else {
                        app.state.vrgplrTypeNew[i++] = 155;
                    }
                }
                break;
            case lvlAiExpert:
                while (i < app.state.game.cPlayer) {
                    if (i < static_cast<std::int16_t>((app.state.game.cPlayer + 1)) / 3 + 1) {
                        app.state.vrgplrTypeNew[i++] = 99;
                    } else if (i < static_cast<std::int16_t>(((app.state.game.cPlayer - 1) * 6 + 11)) / 12 + 1) {
                        app.state.vrgplrTypeNew[i++] = 119;
                    } else if (i < static_cast<std::int16_t>(((app.state.game.cPlayer - 1) * 5 + 5)) / 6 + 1) {
                        app.state.vrgplrTypeNew[i++] = 115;
                    } else {
                        app.state.vrgplrTypeNew[i++] = 123;
                    }
                }
            }
            for (i = 1; i < app.state.game.cPlayer - 1; i++) {
                c = RandomIO().pfnRandom(app, app.state.game.cPlayer - i - 1) + i + 1;
                ch = app.state.vrgplrTypeNew[i];
                app.state.vrgplrTypeNew[i] = app.state.vrgplrTypeNew[c];
                app.state.vrgplrTypeNew[c] = ch;
            }
        }
    }
    return;
}

void InitNewGame3() { return; }

PLAYER *LpplrComp(AiRace idAi, AiLevel lvlAi) { return &vrgplrComp[idAi][lvlAi]; }

void SetVCCheck(GAME *pgame, VictoryCondition vc, std::int16_t fChecked) {
    pgame->rgvc[vc] = (pgame->rgvc[vc] & 0x7f) | (!fChecked ? 0 : 0x80);
    return;
}

std::int16_t GetVCCheck(GAME *pgame, VictoryCondition vc) {
    if (pgame->rgvc[vc] & 0x80) {
        return 1;
    }
    return 0;
}

std::int16_t SetVCVal(APPSESSION &app, GAME *pgame, VictoryCondition vc, std::int16_t val) {
    std::int16_t cur;

    if (val < 0) {
        val = 0;
    } else if (val > vrgvcMax[vc]) {
        val = vrgvcMax[vc];
    }
    pgame->rgvc[vc] = (pgame->rgvc[vc] & 0x80) | val;
    if (vc == vcMeetsNumCriteria) {
        cur = GetVCVal(app, pgame, vcMeetsNumCriteria, 0);
        if (cur != val) {
            val = cur;
            pgame->rgvc[8] = (pgame->rgvc[8] & 0x80) | cur;
        }
    }
    return val;
}

std::int16_t GetVCVal(APPSESSION &app, GAME *pgame, VictoryCondition vc, std::int16_t fRaw) {
    std::int16_t c;
    std::int16_t i;
    std::int16_t val;

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
            if (i != 2 && (app.state.game.rgvc[i] & 0x80)) {
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
