#include "common.h"
#include "appstate.h"
#include "text_io.h"
#include "planet_query_io.h"
#include "race_query_io.h"
#include "alert_io.h"
#include "player_choice_io.h"
#include "random_io.h"
#include "record_io.h"

std::int16_t rgRacePrimaryTrait[10] = {40, 95, 45, 10, -100, -150, 120, 180, 90, -66};
char    rgRW3Spacing[7] = {4, 3, 3, 3, 3, 3, 3};
std::int16_t rgRaceAdvDisPts[14] = {-235, -25, -159, -201, 40, -240, -155, 160, 240, 255, 325, 180, 70, 30};
char    rgRW3IStat[7] = {0, 1, 2, 3, 4, 5, 6};
std::int16_t rgRaceDisEnvPts[6] = {150, 330, 540, 780, 1050, 1380};
char    rgRW3Width[7] = {-2, 2, 2, 2, -2, 2, 2};
char    rgRaceStatMax[16] = {25, 15, 25, 25, 25, 15, 25, 6, 2, 2, 2, 2, 2, 2, 9};
char    rgRaceStatMin[16] = {7, 5, 5, 5, 5, 2, 5};

std::int16_t GetRaceStat(PLAYER *pplr, RaceStat iStat) { return pplr->rgAttr[iStat]; }

std::int16_t SetRaceStat(PLAYER *pplr, RaceStat iStat, std::int16_t iVal) {
    if (iVal < rgRaceStatMin[iStat]) {
        iVal = rgRaceStatMin[iStat];
    }
    if (iVal > rgRaceStatMax[iStat]) {
        iVal = rgRaceStatMax[iStat];
    }
    pplr->rgAttr[iStat] = iVal;
    return iVal;
}

std::int16_t GetRaceGrbit(PLAYER *pplr, RaceGrbit ibit) {
    if (UINT32_C(1) << ibit & pplr->grbitAttr) {
        return 1;
    }
    return 0;
}

void SetRaceGrbit(PLAYER *pplr, RaceGrbit ibit, std::int16_t fSet) {
    std::uint32_t grMask;

    grMask = UINT32_C(1) << ibit;
    if (fSet) {
        pplr->grbitAttr = static_cast<RaceTraitBits>(pplr->grbitAttr | grMask);
    } else {
        pplr->grbitAttr = static_cast<RaceTraitBits>(pplr->grbitAttr & ~grMask);
    }
    return;
}

void BoundsCheckPlayer(PLAYER *pplr) {
    std::int16_t i;

    for (i = 0; i < 3; i++) {
        if (pplr->rgEnvVarMin[i] == envImmune) {
            if (pplr->rgEnvVarMax[i] != envImmune || pplr->rgEnvVar[i] != envImmune) {
                pplr->rgEnvVar[i] = envImmune;
                pplr->rgEnvVarMax[i] = envImmune;
                pplr->fHacker = 1;
            }
        } else {
            if (pplr->rgEnvVarMin[i] < 0) {
                pplr->rgEnvVarMin[i] = 0;
                pplr->fHacker = 1;
            }
            if (pplr->rgEnvVarMin[i] > 100) {
                pplr->rgEnvVarMin[i] = 100;
                pplr->fHacker = 1;
            }
            if (pplr->rgEnvVarMax[i] > 100) {
                pplr->rgEnvVarMax[i] = 100;
                pplr->fHacker = 1;
            }
            if (pplr->rgEnvVarMax[i] < pplr->rgEnvVarMin[i]) {
                pplr->rgEnvVarMax[i] = pplr->rgEnvVarMin[i];
                pplr->fHacker = 1;
            }
            if (pplr->rgEnvVar[i] != pplr->rgEnvVarMin[i] + static_cast<std::int16_t>((pplr->rgEnvVarMax[i] - pplr->rgEnvVarMin[i])) / 2) {
                pplr->rgEnvVar[i] = pplr->rgEnvVarMin[i] + static_cast<std::int16_t>((pplr->rgEnvVarMax[i] - pplr->rgEnvVarMin[i])) / 2;
                pplr->fHacker = 1;
            }
        }
    }
    if (pplr->pctIdealGrowth > 20) {
        pplr->pctIdealGrowth = 20;
        pplr->fHacker = 1;
    }
    for (i = 0; i < 16; i++) {
        if (pplr->rgAttr[i] < rgRaceStatMin[i]) {
            pplr->rgAttr[i] = rgRaceStatMin[i];
            pplr->fHacker = 1;
        }
        if (pplr->rgAttr[i] > rgRaceStatMax[i]) {
            pplr->rgAttr[i] = rgRaceStatMax[i];
            pplr->fHacker = 1;
        }
    }
    return;
}

std::int16_t CAdvantagePoints(APPSESSION &app, PLAYER *pplr) {
    std::int16_t       pctGrowth;
    std::int16_t       iSpread;
    std::int32_t       cPoints;
    std::int16_t       cBad;
    std::int16_t       cCur;
    std::int16_t       i;
    std::int16_t       rgi[3];
    std::int16_t       cGood;
    std::int32_t       lInnate;
    RaceAttribute raMajor;
    std::int16_t       cOperate;
    std::int16_t       cProduce;

    cPoints = 0;
    cPoints = 1650;
    BoundsCheckPlayer(pplr);
    raMajor = static_cast<RaceAttribute>(RaceQueryIO().pfnStat(pplr, rsMajorAdv));
    lInnate = static_cast<std::int32_t>((LInnateRaceHabitability(app, pplr) / 2000));
    iSpread = 1 > (20 >= pplr->pctIdealGrowth ? pplr->pctIdealGrowth : 20) ? 1 : 20 < pplr->pctIdealGrowth ? 20 : pplr->pctIdealGrowth;
    if (iSpread != pplr->pctIdealGrowth) {
        iSpread = 1;
        pplr->pctIdealGrowth = 1;
        pplr->fHacker = 1;
    }
    pctGrowth = iSpread;
    if (iSpread <= 5) {
        cPoints += static_cast<std::uint32_t>((static_cast<std::int16_t>((6 - iSpread)) * 4200));
    } else if (iSpread > 13) {
        if (iSpread < 20) {
            iSpread = (iSpread - 13) * 3 + 21;
        } else {
            iSpread = 45;
        }
    } else {
        switch (iSpread) {
        case 6:
            cPoints += 3600;
            break;
        case 7:
            cPoints += 2250;
            break;
        case 8:
            cPoints += 600;
            break;
        case 9:
            cPoints += 225;
        }
        iSpread = (iSpread - 5) * 2 + 5;
    }
    lInnate = static_cast<std::int32_t>((lInnate * iSpread)) / 24;
    cPoints -= lInnate;
    cGood = 0;
    for (i = 0; i < 3; i++) {
        if (pplr->rgEnvVar[i] >= 0) {
            cPoints += static_cast<std::int16_t>((std::abs(pplr->rgEnvVar[i] - 50) * 4));
        } else {
            cGood++;
        }
    }
    if (cGood > 1) {
        cPoints -= 150;
    }
    cOperate = RaceQueryIO().pfnStat(pplr, rsFactOperate);
    cProduce = RaceQueryIO().pfnStat(pplr, rsFactProd);
    if (cOperate > 10 || cProduce > 10) {
        cOperate = 1 <= cOperate - 9 ? cOperate - 9 : 1;
        cProduce = 1 <= cProduce - 9 ? cProduce - 9 : 1;
        cProduce = (raMajor == raCheapCol ? 3 : 2) * cProduce;
        if (cGood >= 2) {
            cPoints -= static_cast<std::int32_t>((static_cast<std::uint32_t>((cOperate * cProduce)) * pctGrowth)) / 2;
        } else {
            cPoints -= static_cast<std::int32_t>((static_cast<std::uint32_t>((cOperate * cProduce)) * pctGrowth)) / 9;
        }
    }
    i = RaceQueryIO().pfnStat(pplr, rsResGen);
    i = i >= 25 ? 25 : i;
    if (i <= 7) {
        cPoints -= 2400;
    } else if (i == 8) {
        cPoints -= 1260;
    } else if (i == 9) {
        cPoints -= 600;
    } else if (i > 10) {
        cPoints += static_cast<std::int16_t>(((i - 10) * 120));
    }
    if (raMajor != raMacintosh) {
        rgi[0] = 10 - RaceQueryIO().pfnStat(pplr, rsFactProd);
        rgi[1] = 10 - RaceQueryIO().pfnStat(pplr, rsFactBuild);
        rgi[2] = 10 - RaceQueryIO().pfnStat(pplr, rsFactOperate);
        cCur = 0;
        if (rgi[0] > 0) {
            cCur += 100 * rgi[0];
        } else {
            cCur += 121 * rgi[0];
        }
        if (rgi[1] < 0) {
            cCur -= 55 * rgi[1];
        } else {
            cCur -= rgi[1] * rgi[1] * 60;
        }
        if (rgi[2] > 0) {
            cCur += 40 * rgi[2];
        } else {
            cCur += 35 * rgi[2];
        }
        if (cCur > 700) {
            cCur = static_cast<std::int16_t>((cCur - 700)) / 3 + 700;
        }
        if (rgi[2] <= -7) {
            if (rgi[2] >= -11) {
                cCur -= (-6 - rgi[2]) * 30;
            } else if (rgi[2] >= -14) {
                cCur -= (-12 - rgi[2]) * 45 + 225;
            } else {
                cCur -= 360;
            }
        }
        if (rgi[0] <= -3) {
            cCur -= (-2 - rgi[0]) * 20 * 3;
        }
        cPoints += cCur;
        if (RaceQueryIO().pfnGrbit(pplr, ibitRaceCheapFact) != 0) {
            cPoints -= 175;
        }
        rgi[0] = 10 - RaceQueryIO().pfnStat(pplr, rsMineProd);
        rgi[1] = 3 - RaceQueryIO().pfnStat(pplr, rsMineBuild);
        rgi[2] = 10 - RaceQueryIO().pfnStat(pplr, rsMineOperate);
        cCur = 0;
        if (rgi[0] > 0) {
            cCur += 100 * rgi[0];
        } else {
            cCur += 169 * rgi[0];
        }
        if (rgi[1] <= 0) {
            cCur -= 65 * rgi[1] - 80;
        } else {
            cCur -= 360;
        }
        if (rgi[2] > 0) {
            cCur += 40 * rgi[2];
        } else {
            cCur += 35 * rgi[2];
        }
        cPoints += cCur;
    } else {
        cPoints += 210;
    }
    cPoints -= rgRacePrimaryTrait[raMajor];
    cBad = 0;
    cGood = 0;
    for (i = 0; i <= 13; i++) {
        if (RaceQueryIO().pfnGrbit(pplr, static_cast<RaceGrbit>(i)) != 0) {
            if (rgRaceAdvDisPts[i] < 0) {
                cBad++;
            } else {
                cGood++;
            }
            cPoints += rgRaceAdvDisPts[i];
        }
    }
    if (cBad + cGood > 4) {
        cPoints -= static_cast<std::int16_t>(((cBad + cGood) * 10 * (cBad + cGood - 4)));
    }
    if (cGood - cBad > 3) {
        cPoints -= static_cast<std::int16_t>(((cGood - cBad - 3) * 60));
    }
    if (cBad - cGood > 3) {
        cPoints -= static_cast<std::int16_t>(((cBad - cGood - 3) * 40));
    }
    if (RaceQueryIO().pfnGrbit(pplr, ibitRaceNoAdvScanner) != 0) {
        switch (raMajor) {
        default:
            break;
        case raMassAccel:
            cPoints -= 280;
            break;
        case raStealth:
            cPoints -= 200;
            break;
        case raNone:
            cPoints -= 40;
        }
    }
    cCur = 0;
    for (i = 8; i <= 13; i++) {
        cCur += RaceQueryIO().pfnStat(pplr, static_cast<RaceStat>(i)) - 1;
    }
    if (cCur > 0) {
        cPoints -= static_cast<std::int16_t>((cCur * cCur * 130));
        if (cCur == 6) {
            cPoints += 1430;
        } else if (cCur == 5) {
            cPoints += 520;
        }
    } else if (cCur < 0) {
        cPoints += rgRaceDisEnvPts[-cCur - 1];
        if (-cCur > 4 && RaceQueryIO().pfnStat(pplr, rsResGen) < 10) {
            cPoints -= 190;
        }
    }
    if (RaceQueryIO().pfnGrbit(pplr, ibitRaceTech3) != 0) {
        cPoints -= 180;
    }
    if (raMajor == raMacintosh && RaceQueryIO().pfnStat(pplr, rsTechBonus1) == 2) {
        cPoints -= 100;
    }
    return LOWORD(static_cast<std::int32_t>((cPoints / 3)));
}

std::int32_t LInnateRaceHabitability(APPSESSION &app, PLAYER *pplr) {
    std::int16_t iTry;
    PLANET  pl;
    double  l2;
    std::int16_t rgSteps[3];
    PLAYER  plrT;
    std::int16_t rgDelta[3];
    std::int16_t fTotalTerra;
    std::int16_t rgInc[3];
    std::int16_t i;
    std::int16_t iTerra;
    std::int16_t j;
    std::int32_t l1;
    std::int16_t rgBase[3];
    double  l3;
    std::int16_t iDelta;
    std::int32_t pctDesire;
    std::int16_t k;
    double  lInnate;
    std::int16_t pctTerra;

    plrT = app.state.rgplr[0];
    lInnate = static_cast<double>(0);
    fTotalTerra = RaceQueryIO().pfnGrbit(pplr, ibitRaceTT);
    app.state.rgplr[0] = *pplr;
    rgDelta[2] = 0;
    rgDelta[1] = 0;
    rgDelta[0] = 0;
    for (iTerra = 0; iTerra < 3; iTerra++) {
        if (iTerra == 0) {
            pctTerra = 0;
        } else if (iTerra == 1) {
            pctTerra = !fTotalTerra ? 5 : 8;
        } else {
            pctTerra = !fTotalTerra ? 15 : 17;
        }
        for (i = 0; i < 3; i++) {
            if ((pplr->rgEnvVar[i] > 100 || pplr->rgEnvVarMin[i] > 100 || pplr->rgEnvVarMax[i] > 100 || pplr->rgEnvVar[i] < 0 || pplr->rgEnvVarMin[i] < 0 ||
                 pplr->rgEnvVarMax[i] < 0) &&
                (pplr->rgEnvVar[i] != envImmune || pplr->rgEnvVarMin[i] != envImmune || pplr->rgEnvVarMax[i] != envImmune)) {
                pplr->rgEnvVarMax[i] = envImmune;
                pplr->rgEnvVarMin[i] = envImmune;
                pplr->rgEnvVar[i] = envImmune;
                pplr->fHacker = 1;
                app.state.rgplr[0] = *pplr;
            }
            if (pplr->rgEnvVar[i] < 0) {
                rgBase[i] = 50;
                rgInc[i] = 11;
                rgSteps[i] = 1;
            } else {
                rgBase[i] = pplr->rgEnvVarMin[i] - pctTerra;
                if (rgBase[i] < 0) {
                    rgBase[i] = 0;
                }
                iTry = pplr->rgEnvVarMax[i] + pctTerra;
                if (iTry > 100) {
                    iTry = 100;
                }
                rgInc[i] = iTry - rgBase[i];
                rgSteps[i] = 11;
            }
        }
        l3 = static_cast<double>(0);
        for (i = 0; i < rgSteps[0]; i++) {
            if (i == 0 || rgSteps[0] <= 1) {
                iTry = rgBase[0];
            } else {
                iTry = static_cast<std::int16_t>((i * rgInc[0])) / (rgSteps[0] - 1) + rgBase[0];
            }
            if (iTerra != 0 && pplr->rgEnvVar[0] >= 0) {
                iDelta = pplr->rgEnvVar[0] - iTry;
                if (std::abs(iDelta) <= pctTerra) {
                    iDelta = 0;
                } else if (iDelta < 0) {
                    iDelta += pctTerra;
                } else {
                    iDelta -= pctTerra;
                }
                rgDelta[0] = iDelta;
                iTry = pplr->rgEnvVar[0] - iDelta;
            }
            pl.rgEnvVar[0] = iTry;
            l2 = static_cast<double>(0);
            for (j = 0; j < rgSteps[1]; j++) {
                if (j == 0 || rgSteps[1] <= 1) {
                    iTry = rgBase[1];
                } else {
                    iTry = static_cast<std::int16_t>((j * rgInc[1])) / (rgSteps[1] - 1) + rgBase[1];
                }
                if (iTerra != 0 && pplr->rgEnvVar[1] >= 0) {
                    iDelta = pplr->rgEnvVar[1] - iTry;
                    if (std::abs(iDelta) <= pctTerra) {
                        iDelta = 0;
                    } else if (iDelta < 0) {
                        iDelta += pctTerra;
                    } else {
                        iDelta -= pctTerra;
                    }
                    rgDelta[1] = iDelta;
                    iTry = pplr->rgEnvVar[1] - iDelta;
                }
                pl.rgEnvVar[1] = iTry;
                l1 = 0;
                for (k = 0; k < rgSteps[2]; k++) {
                    if (k == 0 || rgSteps[2] <= 1) {
                        iTry = rgBase[2];
                    } else {
                        iTry = static_cast<std::int16_t>((k * rgInc[2])) / (rgSteps[2] - 1) + rgBase[2];
                    }
                    if (iTerra != 0 && pplr->rgEnvVar[2] >= 0) {
                        iDelta = pplr->rgEnvVar[2] - iTry;
                        if (std::abs(iDelta) <= pctTerra) {
                            iDelta = 0;
                        } else if (iDelta < 0) {
                            iDelta += pctTerra;
                        } else {
                            iDelta -= pctTerra;
                        }
                        rgDelta[2] = iDelta;
                        iTry = pplr->rgEnvVar[2] - iDelta;
                    }
                    pl.rgEnvVar[2] = iTry;
                    pctDesire = PlanetQueryIO().pfnDesirability(app, &pl, 0);
                    iDelta = rgDelta[0] + rgDelta[1] + rgDelta[2];
                    if (iDelta > pctTerra) {
                        pctDesire -= static_cast<std::int16_t>((iDelta - pctTerra));
                        if (pctDesire < 0) {
                            pctDesire = 0;
                        }
                    }
                    pctDesire = static_cast<std::uint32_t>((pctDesire * pctDesire));
                    if (iTerra == 0) {
                        pctDesire = static_cast<std::uint32_t>((pctDesire * 7));
                    } else if (iTerra == 1) {
                        pctDesire = static_cast<std::uint32_t>((pctDesire * 5));
                    } else {
                        pctDesire = static_cast<std::uint32_t>((pctDesire * 6));
                    }
                    l1 += pctDesire;
                }
                if (pplr->rgEnvVar[2] >= 0) {
                    l1 = static_cast<std::int32_t>((l1 * rgInc[2])) / 100;
                } else {
                    l1 = static_cast<std::uint32_t>((l1 * 11));
                }
                l2 = static_cast<double>((static_cast<double>(l1) + l2));
            }
            if (pplr->rgEnvVar[1] >= 0) {
                l2 = static_cast<double>((static_cast<double>(l2) * rgInc[1] / 100));
            } else {
                l2 = static_cast<double>((static_cast<double>(l2) * 11));
            }
            l3 = static_cast<double>((static_cast<double>(l3) + l2));
        }
        if (pplr->rgEnvVar[0] >= 0) {
            l3 = static_cast<double>((static_cast<double>(l3) * rgInc[0] / 100));
        } else {
            l3 = static_cast<double>((static_cast<double>(l3) * 11));
        }
        lInnate = static_cast<double>((static_cast<double>(lInnate) + l3));
    }
    if (pplr != app.state.rgplr) {
        app.state.rgplr[0] = plrT;
    }
    return static_cast<std::int32_t>((static_cast<double>(lInnate) / 10.0 + 0.5));
}

std::uint16_t IRaceChecksum(PLAYER *pplr) {
    std::uint16_t  ick;
    std::byte rgb[cbWirePlayer];
    std::uint16_t w;
    std::int16_t   i;
    std::int16_t   cs;
    std::size_t    cch;
    PLAYER    plr;

    /* Checksum the names as ReadRtPlr restores them, zeroed after the
       terminator. RaceWizardDlg1 reads the names into buffers that can
       still hold a longer earlier name; the original checksummed those
       stale bytes, which aren't saved, so the race file failed its
       checksum when loaded. */
    plr = *pplr;
    cch = strnlen(plr.szName, sizeof(plr.szName));
    std::memset(plr.szName + cch, 0, sizeof(plr.szName) - cch);
    cch = strnlen(plr.szNames, sizeof(plr.szNames));
    std::memset(plr.szNames + cch, 0, sizeof(plr.szNames) - cch);
    FEncodePlayer(plr, rgb);
    NATIVEBYTEREADER input(rgb);
    cs = 96;
    ick = 0;
    for (i = 0; i < cs; i++) {
        input.FReadU16Le(w);
        ick ^= w;
    }
    return ick;
}

static std::int16_t FReadLegacyRaceString(APPSESSION &app, const std::uint8_t *lpb, std::size_t cb, char *szRaw, std::size_t cchMax, std::size_t *pcbRead) {
    const std::uint8_t *lpbEnd;
    std::size_t cch;
    std::int16_t cOut;

    szRaw[0] = 0;
    if (cb == 0 || cchMax != 32) return 0;
    if (lpb[0] == 0) {
        lpbEnd = static_cast<const std::uint8_t *>(std::memchr(lpb + 1, 0, cb - 1));
        if (!lpbEnd) return 0;
        cch = static_cast<std::size_t>((lpbEnd - lpb - 1));
        if (cch >= cchMax) return 0;
        std::memcpy(szRaw, lpb + 1, cch);
        szRaw[cch] = 0;
        if (pcbRead) *pcbRead = cch + 2;
    } else {
        if (lpb[0] > cb - 1) return 0;
        cOut = static_cast<std::int16_t>(cchMax) - 1;
        if (!FDecompressUserStringBytes(app, const_cast<char *>(reinterpret_cast<const char*>(lpb)) + 1, lpb[0], szRaw, &cOut)) return 0;
        if (pcbRead) *pcbRead = static_cast<std::size_t>(lpb[0]) + 1;
    }
    return 1;
}

static std::int16_t FReadLegacyRacePlayer(APPSESSION &app, PLAYER *pplr, const std::uint8_t *lpb, std::size_t cb, std::int16_t fPlural) {
    std::int16_t iOff;
    std::size_t cbRead;

    std::memset(pplr, 0, sizeof(PLAYER));
    if (cb < cbWirePlayerPartial) return 0;
    if ((lpb[6] & 7) == detAll) {
        iOff = cbWirePlayerPrefix;
        if (cb <= static_cast<std::size_t>(iOff) || lpb[iOff] > sizeof(pplr->rgmdRelation) ||
            lpb[iOff] > cb - static_cast<std::size_t>(iOff) - 1) return 0;
        FDecodePlayer(std::as_bytes(std::span(lpb, cb)), *pplr, iOff);
        std::memmove(pplr->rgmdRelation, lpb + iOff + 1, lpb[iOff]);
        iOff += lpb[iOff] + 1;
    } else {
        iOff = cbWirePlayerPartial;
        FDecodePlayer(std::as_bytes(std::span(lpb, cb)), *pplr, iOff);
    }
    if (!FReadLegacyRaceString(app, lpb + iOff, cb - static_cast<std::size_t>(iOff), pplr->szName, sizeof(pplr->szName), &cbRead)) return 0;
    iOff += static_cast<std::int16_t>(cbRead);
    if (fPlural) {
        if (!FReadLegacyRaceString(app, lpb + iOff, cb - static_cast<std::size_t>(iOff), pplr->szNames, sizeof(pplr->szNames), &cbRead)) return 0;
        iOff += static_cast<std::int16_t>(cbRead);
    }
    if (static_cast<std::size_t>(iOff) != cb) return 0;
    pplr->fLearned = 0;
    return 1;
}

static std::int16_t FConvertLegacyRaceNames(APPSESSION &app, PLAYER *pplr, std::int16_t fPlural) {
    char szName[96];
    char szNames[96];
    std::size_t cbName;
    std::size_t cbNames;
    std::size_t cbKeep;
    std::int16_t fShortened;

    if (CchCp1252ToUtf8(std::span(szName), std::as_bytes(std::span(pplr->szName, std::strlen(pplr->szName)))) < 0) return 0;
    if (fPlural) {
        if (CchCp1252ToUtf8(std::span(szNames), std::as_bytes(std::span(pplr->szNames, std::strlen(pplr->szNames)))) < 0) return 0;
    } else {
        cbName = std::strlen(szName);
        if (cbName + 1 >= sizeof(szNames)) return 0;
        std::memcpy(szNames, szName, cbName);
        szNames[cbName] = 's';
        szNames[cbName + 1] = 0;
    }
    cbName = std::strlen(szName);
    cbNames = std::strlen(szNames);
    fShortened = cbName >= sizeof(pplr->szName) || cbNames >= sizeof(pplr->szNames);
    if (fShortened) {
        cbKeep = CchUtf8Prefix(szName, sizeof(pplr->szName) - 1);
        std::memmove(pplr->szName, szName, cbKeep);
        pplr->szName[cbKeep] = 0;
        cbKeep = CchUtf8Prefix(szNames, sizeof(pplr->szNames) - 1);
        std::memmove(pplr->szNames, szNames, cbKeep);
        pplr->szNames[cbKeep] = 0;
        if (!FConfirmLegacyRaceNames(app, pplr->szName, pplr->szNames)) return -1;
    } else {
        std::memcpy(pplr->szName, szName, cbName + 1);
        std::memcpy(pplr->szNames, szNames, cbNames + 1);
    }
    return 1;
}

std::int16_t FWasRaceFile(APPSESSION &app, char *szFile, std::int16_t fChkPass) {
    std::int16_t  idsError;
    std::int32_t  lSaltSav;
    PLAYER   plr;
    std::int16_t  fRet;
    std::int16_t  fSav;
    std::int16_t  fLegacy;
    std::int16_t  fPlural;
    std::int16_t  fConvert;
    std::int16_t  fProtected;
    char     szPassword[16];
    std::uint16_t icksum;
    char    *pch;

    idsError = -1;
    fRet = 0;
    fSav = app.state.fFileErrSilent;
    app.state.fFileErrSilent = 1;
    StarsRecoveryScope scopeRecovery;
    try {
        {
            RecordIO().pfnStreamOpen(app, szFile, mdRead);
            RecordIO().pfnReadRt(app);
            if (app.state.hdrCur.rt != rtBOF || app.state.hdrCur.cb != cbWireBof || BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMajor() != 2 ||
                BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMinor() < 49 || BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMinor() > 85) {
                idsError = idsFileDoesBelongVersionStars;
                fRet = -1;
                goto LBadFile;
            } else {
                app.state.wVersFile = BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).wVersion();
                fLegacy = BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMinor() < 85;
                fPlural = BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).verMinor() >= 55;
                if (BfvFromPrefix(app.state.rgbCur, sizeof(app.state.rgbCur)).dt() == 5) {
                    RecordIO().pfnReadRt(app);
                    if (app.state.hdrCur.rt == rtPlr) {
                        idsError = idsGameFileAppearsCorruptUnableLoadFile;
                        if (fLegacy) {
                            if (!FReadLegacyRacePlayer(app, &plr, reinterpret_cast<const std::uint8_t *>(app.state.rgbCur), app.state.hdrCur.cb, fPlural)) goto LBadFile;
                        } else {
                            ReadRtPlr(app, &plr, reinterpret_cast<std::uint8_t *>(app.state.rgbCur), app.state.hdrCur.cb);
                        }
                        RecordIO().pfnReadRt(app);
                        icksum = IRaceChecksum(&plr);
                        if (app.state.hdrCur.rt == rtEOF && app.state.hdrCur.cb == 2 && WFromStreamBytes(app.state.rgbCur) == icksum) {
                            fProtected = fLegacy && plr.lSalt != 0;
                            if (fLegacy && (fConvert = FConvertLegacyRaceNames(app, &plr, fPlural)) <= 0) {
                                idsError = fConvert < 0 ? -1 : idsGameFileAppearsCorruptUnableLoadFile;
                                fRet = -1;
                                goto LBadFile;
                            }
                            if (fProtected) {
                                if (!PlayerChoiceIO().pfnFPromptLegacyRacePassword(app, szPassword)) {
                                    idsError = -1;
                                    fRet = -1;
                                    goto LBadFile;
                                }
                                plr.lSalt = LSaltFromSz(szPassword);
                                std::strcpy(app.state.szRacePass, szPassword);
                            }
                            lSaltSav = app.state.lSaltCur;
                            app.state.lSaltCur = plr.lSalt;
                            if (!fLegacy && fChkPass && !FCheckPassword(app)) {
                                app.state.lSaltCur = lSaltSav;
                                fRet = -1;
                                goto LBadFile;
                            } else {
                                app.state.lSaltCur = lSaltSav;
                                if (!fProtected) {
                                    if (!fLegacy && plr.lSalt != 0) {
                                        std::strcpy(app.state.szRacePass, app.state.szPassLast);
                                    } else {
                                        app.state.szRacePass[0] = 0;
                                    }
                                }
                                app.state.vplr = plr;
                                /* The original copied the whole path into the
                                   16-byte szRaceFile, overrunning the globals
                                   after it when a universe definition names a
                                   race by path. Keep the file name, as the race
                                   dialogs do. */
                                pch = std::strrchr(szFile, chDirSep);
                                SetSzRaceFile(app, pch ? pch + 1 : szFile);
                                RecordIO().pfnStreamClose(app);
                                app.state.fFileErrSilent = fSav;
                                return 1;
                            }
                        }
                    }
                }
            }
        }
        goto LBadFile;
    } catch (const StarsError &) {
    }
LBadFile:
    RecordIO().pfnStreamClose(app);
    app.state.fFileErrSilent = fSav;
    if (!app.state.fFileErrSilent && idsError != -1) {
        /* NATIVE: GenNewGameFromFile passes szWork itself; see FLoadGame. */
        if (szFile != app.state.szWork) {
            std::strcpy(app.state.szWork, szFile);
        }
        AlertIO().pfnAlertSz(app, TextIO().pfnFormat(app, static_cast<StringId>(idsError), nullptr), win32::MB_ICONHAND);
    }
    return fRet;

}

void CreateRandomRace(APPSESSION &app, PLAYER *pplr) {
    std::int16_t cPts;
    std::int16_t i;
    std::int16_t cPass;
    std::int16_t j;
    std::int16_t iVal;
    std::int16_t dAwayNew;
    std::int16_t dAwayCur;
    std::int16_t k;
    PLAYER  plrT;

    pplr->szNames[0] = 0;
    iVal = RandomIO().pfnRandom(app, 25);
    if (iVal < 4) {
        for (i = 0; i < 3; i++) {
            pplr->rgEnvVarMax[i] = envImmune;
            pplr->rgEnvVarMin[i] = envImmune;
            pplr->rgEnvVar[i] = envImmune;
        }
        pplr->pctIdealGrowth = RandomIO().pfnRandom(app, 4) + 2;
    } else if (iVal < 7) {
        for (i = 0; i < 3; i++) {
            pplr->rgEnvVar[i] = 50;
            pplr->rgEnvVarMin[i] = 0;
            pplr->rgEnvVarMax[i] = 100;
        }
        pplr->pctIdealGrowth = RandomIO().pfnRandom(app, 4) + 3;
    } else if (iVal < 9) {
        for (i = 0; i < 3; i++) {
            j = RandomIO().pfnRandom(app, 2);
            if (i == 2 && pplr->rgEnvVar[0] == pplr->rgEnvVar[1]) {
                j = pplr->rgEnvVar[0] != 0;
            }
            if (j == 0) {
                pplr->rgEnvVar[i] = 50;
                pplr->rgEnvVarMin[i] = 0;
                pplr->rgEnvVarMax[i] = 100;
            } else {
                pplr->pctIdealGrowth = RandomIO().pfnRandom(app, 4) + 2;
            }
        }
        pplr->pctIdealGrowth = RandomIO().pfnRandom(app, 5) + 2;
    } else {
        for (i = 0; i < 3; i++) {
            j = RandomIO().pfnRandom(app, 40) * 2 + 20;
            k = RandomIO().pfnRandom(app, 100 - j + 1);
            pplr->rgEnvVar[i] = j / 2 + k;
            pplr->rgEnvVarMin[i] = k;
            pplr->rgEnvVarMax[i] = k + j;
        }
        if (iVal < 12) {
            i = RandomIO().pfnRandom(app, 3);
            pplr->rgEnvVarMax[i] = envImmune;
            pplr->rgEnvVarMin[i] = envImmune;
            pplr->rgEnvVar[i] = envImmune;
        } else if (iVal < 14) {
            i = RandomIO().pfnRandom(app, 3);
            pplr->rgEnvVar[i] = 50;
            pplr->rgEnvVarMin[i] = 0;
            pplr->rgEnvVarMax[i] = 100;
        } else if (iVal < 17) {
            i = RandomIO().pfnRandom(app, 3);
            j = RandomIO().pfnRandom(app, 81);
            pplr->rgEnvVar[i] = j + 10;
            pplr->rgEnvVarMin[i] = j;
            pplr->rgEnvVarMax[i] = j + 20;
        }
        pplr->pctIdealGrowth = RandomIO().pfnRandom(app, 9) + 7;
    }
    iVal = RandomIO().pfnRandom(app, 3);
    for (i = 8; i <= 13; i++) {
        SetRaceStat(pplr, static_cast<RaceStat>(i), iVal != 0 ? RandomIO().pfnRandom(app, 3) : 1);
    }
    SetRaceStat(pplr, rsMajorAdv, RandomIO().pfnRandom(app, 10));
    iVal = RandomIO().pfnRandom(app, 4);
    for (i = 0; i <= 13; i++) {
        SetRaceGrbit(pplr, static_cast<RaceGrbit>(i), iVal != 0 ? RandomIO().pfnRandom(app, 2) : 0);
    }
    SetRaceGrbit(pplr, ibitRaceTech3, RandomIO().pfnRandom(app, 2));
    SetRaceGrbit(pplr, ibitRaceCheapFact, RandomIO().pfnRandom(app, 2));
    iVal = RandomIO().pfnRandom(app, 3);
    if (iVal == 0) {
        for (i = 0; i <= 6; i++) {
            pplr->rgAttr[i] = vrgplrDef[0].rgAttr[i];
        }
        pplr->rgAttr[7] = RandomIO().pfnRandom(app, 5);
    } else {
        for (i = 0; i <= 7; i++) {
            pplr->rgAttr[i] = rgRaceStatMin[i] + RandomIO().pfnRandom(app, rgRaceStatMax[i] + 1 - rgRaceStatMin[i]);
        }
    }
    if (std::strcmp(pplr->szName, TextIO().pfnCompressed(app, idsRandom2)) == 0) {
        TextIO().pfnCchGetString(app, static_cast<StringId>(idsBerserker + RandomIO().pfnRandom(app, 24)), pplr->szName);
    }
    cPts = CAdvantagePoints(app, pplr);
    if (cPts < 0 || cPts > 50) {
        cPass = 0;
        while ((cPts = CAdvantagePoints(app, pplr)) < 0 || cPts > 50) {
            if (cPass++ > 250) {
                plrT = *pplr;
                *pplr = vrgplrDef[0];
                std::strcpy(pplr->szName, plrT.szName);
                break;
            }
            iVal = RandomIO().pfnRandom(app, 10);
            dAwayCur = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
            if (iVal < 3) {
                i = RandomIO().pfnRandom(app, 6);
                j = pplr->rgAttr[i + 8];
                if (j > 0) {
                    pplr->rgAttr[i + 8] = pplr->rgAttr[i + 8] - 1;
                    cPts = CAdvantagePoints(app, pplr);
                    dAwayNew = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
                    if (dAwayNew < dAwayCur)
                        continue;
                    pplr->rgAttr[i + 8] = j;
                }
                if (j < 2) {
                    pplr->rgAttr[i + 8] = pplr->rgAttr[i + 8] + 1;
                    cPts = CAdvantagePoints(app, pplr);
                    dAwayNew = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
                    if (dAwayNew >= dAwayCur) {
                        pplr->rgAttr[i + 8] = j;
                    }
                }
            } else if (iVal < 6) {
                iVal = RandomIO().pfnRandom(app, 14);
                j = RaceQueryIO().pfnGrbit(pplr, static_cast<RaceGrbit>(iVal));
                for (i = 0; i < 2; i++) {
                    SetRaceGrbit(pplr, static_cast<RaceGrbit>(iVal), i);
                    cPts = CAdvantagePoints(app, pplr);
                    dAwayNew = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
                    if (dAwayNew < dAwayCur)
                        break;
                }
                if (i >= 2) {
                    SetRaceGrbit(pplr, static_cast<RaceGrbit>(iVal), j);
                }
            } else if (iVal < 9) {
                iVal = RandomIO().pfnRandom(app, 7);
                j = RaceQueryIO().pfnStat(pplr, static_cast<RaceStat>(iVal));
                for (i = -1; i <= 1; i += 2) {
                    SetRaceStat(pplr, static_cast<RaceStat>(iVal), j + i);
                    cPts = CAdvantagePoints(app, pplr);
                    dAwayNew = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
                    if (dAwayNew < dAwayCur)
                        break;
                }
                if (i > 1) {
                    SetRaceStat(pplr, static_cast<RaceStat>(iVal), j);
                }
            } else if (RandomIO().pfnRandom(app, 2) != 0) {
                j = pplr->pctIdealGrowth;
                if (j > 1) {
                    pplr->pctIdealGrowth = j - 1;
                    cPts = CAdvantagePoints(app, pplr);
                    dAwayNew = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
                    if (dAwayNew < dAwayCur)
                        continue;
                }
                if (j < 15) {
                    pplr->pctIdealGrowth = j + 1;
                    cPts = CAdvantagePoints(app, pplr);
                    dAwayNew = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
                    if (dAwayNew < dAwayCur)
                        continue;
                }
                pplr->pctIdealGrowth = j;
            } else {
                iVal = RandomIO().pfnRandom(app, 3);
                if (pplr->rgEnvVar[iVal] < 0) {
                    j = RandomIO().pfnRandom(app, 31);
                    pplr->rgEnvVar[iVal] = j + 35;
                    pplr->rgEnvVarMin[iVal] = j;
                    pplr->rgEnvVarMax[iVal] = j + 70;
                    cPts = CAdvantagePoints(app, pplr);
                    dAwayNew = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
                    if (dAwayNew >= dAwayCur) {
                        pplr->rgEnvVarMax[iVal] = envImmune;
                        pplr->rgEnvVarMin[iVal] = envImmune;
                        pplr->rgEnvVar[iVal] = envImmune;
                    }
                } else {
                    plrT = *pplr;
                    pplr->rgEnvVarMax[iVal] = envImmune;
                    pplr->rgEnvVarMin[iVal] = envImmune;
                    pplr->rgEnvVar[iVal] = envImmune;
                    cPts = CAdvantagePoints(app, pplr);
                    dAwayNew = -cPts <= cPts - 50 ? cPts - 50 : -cPts;
                    if (dAwayNew >= dAwayCur) {
                        *pplr = plrT;
                    }
                }
            }
        }
    }
    return;
}

std::int16_t PctTrueMaxGrowth(APPSESSION &app, std::int16_t iplr) {
    if (RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMajorAdv) == raCheapCol) {
        return app.state.rgplr[iplr].pctIdealGrowth * 2;
    }
    return app.state.rgplr[iplr].pctIdealGrowth;
}
