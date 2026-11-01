#include "common.h"
#include "appstate.h"
#include "text_io.h"
#include "fleet_query_io.h"
#include "presentation_io.h"
#include "turn_io.h"
#include "production_io.h"
#include "heap_io.h"
#include "colony_io.h"
#include "planet_query_io.h"
#include "research_io.h"
#include "part_query_io.h"
#include "race_query_io.h"
#include "random_io.h"
#include "version.h"

std::uint32_t rgcrDrawStars2b[5] = {8355711, 127, 32512, 8323072};
std::uint32_t rgcrDrawStars2a[5] = {12632256, 255, 65280, 16711680};
std::uint32_t rgcrDrawStars[5] = {8355711, 16777215, 255, 65280, 16711680};
std::int32_t  rgDSDivCnt2[5] = {80000, 210000, 310000, 260000};
std::int32_t  rgDSDivCnt[5] = {28000, 28000, 63000, 95000, 73000};
std::uint8_t  vrgbTachyon[18] = {100, 95, 93, 91, 90, 89, 88, 87, 86, 86, 85, 84, 84, 83, 83, 82, 82, 81};

std::int16_t FLookupSelPlanet(APPSESSION &app, PLANET *ppl) {
    if (app.state.sel.scan.grobj != grobjPlanet) {
        return 0;
    }
    return PlanetQueryIO().pfnLookup(app, app.state.sel.scan.idpl, ppl);
}

std::int16_t FDupPlanet(APPSESSION &app, PLANET *lppl, PLANET *ppl) {
    PLPROD *lpplprodT;

    lpplprodT = ppl->lpplprod;
    *ppl = *lppl;
    ppl->lpplprod = lpplprodT;
    if (!lppl->lpplprod) {
        if (ppl->lpplprod) {
            FreePl(app, ppl->lpplprod);
            ppl->lpplprod = nullptr;
        }
        return 1;
    }
    if (!ppl->lpplprod) {
        ppl->lpplprod = LpplAlloc<PLPROD>(app, 4, lppl->lpplprod->iprodMax, htOrd);
    } else if (static_cast<std::int16_t>(ppl->lpplprod->iprodMax) < lppl->lpplprod->iprodMac) {
        ppl->lpplprod = LpplReAlloc(app, ppl->lpplprod, lppl->lpplprod->iprodMax);
    }
    HeapIO().pfnCopy(ppl->lpplprod->rgprod(), lppl->lpplprod->rgprod(), lppl->lpplprod->iprodMac * 4);
    ppl->lpplprod->iprodMac = lppl->lpplprod->iprodMac;
    return 1;
}

THING *LpthFromId(APPSESSION &app, std::int16_t idth) {
    THING *lpth;
    THING *lpthMac;

    lpth = app.state.lpThings;
    lpthMac = app.state.lpThings + app.state.cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->idFull() == idth) {
            return lpth;
        }
    }
    return nullptr;
}

PLANET *LpplFromId(APPSESSION &app, std::int16_t idPlanet) {
    std::int16_t idGuess;
    std::int16_t iLo;
    PLANET *lppl;
    std::int16_t iGuess;
    std::int16_t iHi;

    if (idPlanet < 0 || idPlanet >= app.state.game.cPlanMax) {
        return nullptr;
    }
    if (app.state.cPlanet == app.state.game.cPlanMax) {
        lppl = app.state.lpPlanets + idPlanet;
        return lppl;
    }
    iLo = -1;
    iHi = app.state.cPlanet;
    while (iLo + 1 < iHi) {
        iGuess = (iLo + iHi) >> 1;
        lppl = app.state.lpPlanets + iGuess;
        idGuess = app.state.lpPlanets[iGuess].id;
        if (idGuess < idPlanet) {
            iLo = iGuess;
        } else {
            if (idGuess <= idPlanet) {
                return lppl;
            }
            iHi = iGuess;
        }
    }
    return nullptr;
}

void CalcPctSurvive(APPSESSION &app, PLANET *lppl, float *ppct, float *ppctSmart) {
    std::int16_t iPlrSav;
    std::int32_t cDefenses;
    float   pct;
    PART    part;
    std::int16_t cMax;

    if (ppctSmart) {
        *ppctSmart = static_cast<float>(1.0);
    }
    if (lppl->iPlayer != iplrNone && lppl->cDefenses != 0) {
        iPlrSav = app.state.idPlayer;
        app.state.idPlayer = lppl->iPlayer;
        if (FGetBestDefensePart(app, &part)) {
            cDefenses = lppl->cDefenses;
            cMax = ColonyIO().pfnOperableDefenses(app, lppl, lppl->iPlayer, 0);
            if (cMax < cDefenses) {
                cDefenses = cMax;
            }
            pct = static_cast<float>(std::pow(static_cast<double>((1.0 - static_cast<double>(part.pplanetary()->grAbility) / 1000.0)), static_cast<double>(cDefenses)));
            if (ppctSmart) {
                *ppctSmart = static_cast<float>(std::pow(static_cast<double>((1.0 - static_cast<double>(part.pplanetary()->grAbility) / 2000.0)), static_cast<double>(cDefenses)));
            }
        } else {
            pct = static_cast<float>(1.0);
        }
        app.state.idPlayer = iPlrSav;
    } else {
        pct = static_cast<float>(1.0);
    }
    *ppct = pct;
    return;
}

std::int16_t FLookupPlanet(APPSESSION &app, std::int16_t iPlanet, PLANET *ppl) {
    PLANET *lpPl;
    std::int16_t fWrite;

    fWrite = 0;
    if (app.state.cPlanet <= 0) {
        return 0;
    }
    if (iPlanet < 0) {
        iPlanet = ppl->id;
        if (iPlanet == idplNone) {
            PresentationIO().pfnLogPlanet(app, nullptr, ppl);
            return 1;
        }
        fWrite = 1;
    }
    lpPl = LpplFromId(app, iPlanet);
    if (lpPl && ppl) {
        if (fWrite) {
            PresentationIO().pfnInvalidateReport(app, rptPlanets, 0);
            PresentationIO().pfnLogPlanet(app, lpPl, ppl);
            if (lpPl->lpplprod != ppl->lpplprod) {
                if (!lpPl->lpplprod) {
                    if (ppl->lpplprod) {
                        lpPl->lpplprod = LpplAlloc<PLPROD>(app, 4, ppl->lpplprod->iprodMac, htOrd);
                    }
                } else if (!ppl->lpplprod) {
                    FreePl(app, lpPl->lpplprod);
                    lpPl->lpplprod = nullptr;
                    goto FinishCopy;
                }
                if (static_cast<std::int16_t>(lpPl->lpplprod->iprodMax) < ppl->lpplprod->iprodMac) {
                    lpPl->lpplprod = LpplReAlloc(app, lpPl->lpplprod, ppl->lpplprod->iprodMac + 2);
                }
                HeapIO().pfnCopy(lpPl->lpplprod->rgprod(), ppl->lpplprod->rgprod(), ppl->lpplprod->iprodMac * 4);
                lpPl->lpplprod->iprodMac = ppl->lpplprod->iprodMac;
            }
        FinishCopy:
            HeapIO().pfnCopy(lpPl, ppl, 52);
            if (app.state.gd.fTutorial && app.state.idPlayer == 0) {
                PresentationIO().pfnTutor(app);
            }
        } else if (ppl == &app.state.sel.pl) {
            FDupPlanet(app, lpPl, ppl);
        } else {
            *ppl = *lpPl;
        }
    }
    if (lpPl) {
        return 1;
    }
    return 0;
}

std::int32_t DpOfLpflIshdef(APPSESSION &app, FLEET *lpfl, std::int16_t ishdef) {
    std::int16_t dpShdef;
    std::int32_t dp;

    dpShdef = app.state.rglpshdef[lpfl->iPlayer][ishdef].hul.dp;
    dp = static_cast<std::int32_t>((lpfl->rgcsh[ishdef] * static_cast<std::int16_t>((static_cast<std::int16_t>((lpfl->rgdv[ishdef].pctSh() * dpShdef)) / 10 * lpfl->rgdv[ishdef].pctDp())))) / 5000;
    return dp;
}

std::int16_t FLookupThing(APPSESSION &app, std::int16_t idth, THING *pth) {
    THING  *lpth;
    std::int16_t fWrite;

    fWrite = 0;
    if (app.state.cThing <= 0) {
        return 0;
    }
    if (idth < 0) {
        idth = pth->idFull();
        fWrite = 1;
    }
    lpth = LpthFromId(app, idth);
    if (lpth && pth) {
        if (fWrite) {
            LogChangeThing(app, lpth, pth);
            HeapIO().pfnCopy(lpth, pth, sizeof(THING));
            if (app.state.gd.fTutorial && app.state.idPlayer == 0) {
                PresentationIO().pfnTutor(app);
            }
        } else {
            *pth = *lpth;
        }
    }
    if (lpth) {
        return 1;
    }
    return 0;
}

std::int32_t LComputePower(APPSESSION &app, SHDEF *lpshdef) {
    std::int16_t dSpeed;
    std::int16_t dxRange;
    std::int16_t ihs;
    std::int32_t dpTorps;
    std::int16_t i;
    std::int32_t pctCap;
    std::int32_t dpBeams;
    std::int32_t dpBombs;
    std::int32_t dp;
    PART    part;

    dpBombs = 0;
    dpBeams = 0;
    dpTorps = 0;
    dxRange = 999;
    pctCap = 1000;
    for (ihs = 0; ihs < lpshdef->hul.chs; ihs++) {
        part.hs = lpshdef->hul.rghs[ihs];
        if (part.hs.cItem != 0 && PartQueryIO().pfnLookup(app, &part)) {
            switch (part.hs.grhst) {
            default:
                break;
            case hstBeam:
                dp = static_cast<std::int32_t>((static_cast<std::uint32_t>((part.pbeam()->dp * part.hs.cItem)) * static_cast<std::int16_t>((part.pbeam()->dRangeMax + 3)))) / 4;
                if (part.pbeam()->grfAbilities & beamSapper) {
                    dp = static_cast<std::int32_t>((dp / 3));
                }
                dpBeams += dp;
                break;
            case hstTorp:
                dpTorps += static_cast<std::int32_t>((static_cast<std::uint32_t>((part.ptorp()->dp * part.hs.cItem)) * static_cast<std::int16_t>((part.ptorp()->dRangeMax - 2)))) / 2;
                break;
            case hstBomb:
                dpBombs += static_cast<std::uint32_t>(((part.pbomb()->dDmgCol + part.pbomb()->dDmgBldg) * part.hs.cItem * 2));
                break;
            case hstSpecialE:
                if (part.hs.iItem == ispecialEEnergyCapacitor || part.hs.iItem == ispecialEFluxCapacitor) {
                    for (i = part.hs.cItem; i > 0; i--) {
                        pctCap = static_cast<std::int32_t>((pctCap * (part.pspecial()->grAbility + 100))) / 100;
                    }
                }
            }
        }
    }
    if (pctCap != 1000) {
        pctCap = static_cast<std::int32_t>((pctCap / 10));
        if (pctCap > 255) {
            pctCap = 255;
        }
        dpBeams = static_cast<std::int32_t>((dpBeams * pctCap)) / 100;
    }
    dSpeed = SpdOfShip(app, nullptr, 0, nullptr, 0, lpshdef);
    dpBeams += static_cast<std::int32_t>((dpBeams * static_cast<std::int16_t>((dSpeed - 4)))) / 10;
    return dpBombs + dpBeams + dpTorps;
}

void ComputeShdefPowers(APPSESSION &app) {
    std::int16_t iplr;
    std::int16_t ishdef;

    for (iplr = 0; iplr < app.state.game.cPlayer; iplr++) {
        if (app.state.rglpshdef[iplr]) {
            for (ishdef = 0; ishdef < 16; ishdef++) {
                if (!app.state.rglpshdef[iplr][ishdef].fFree) {
                    app.state.rglpshdef[iplr][ishdef].lPower = LComputePower(app, app.state.rglpshdef[iplr] + ishdef);
                }
            }
        }
    }
    return;
}

std::int32_t DpShieldOfShdef(APPSESSION &app, SHDEF *lpshdef, std::int16_t iplr) {
    std::int16_t chs;
    HS     *lphs;
    std::int16_t ihs;
    std::int32_t dpShdef;
    HUL    *lphul;
    PART    part;

    dpShdef = 0;
    lphul = &lpshdef->hul;
    lphs = lphul->rghs;
    chs = lphul->chs;
    ihs = 0;
    while (ihs < chs) {
        if (lphs->grhst == hstShield && lphs->cItem > 0) {
            part.hs = *lphs;
            PartQueryIO().pfnLookup(app, &part);
            dpShdef += static_cast<std::uint32_t>((part.pshield()->dp * lphs->cItem));
        } else if (lphs->grhst == hstArmor && lphs->cItem > 0 && lphs->iItem == iarmorFieldedKelarium) {
            dpShdef += static_cast<std::uint32_t>((lphs->cItem * 50));
        } else if (lphs->grhst == hstArmor && lphs->iItem == iarmorMegaPolyShell) {
            dpShdef += static_cast<std::uint32_t>((lphs->cItem * 100));
        }
        ihs++;
        lphs++;
    }
    if (RaceQueryIO().pfnGrbit(&app.state.rgplr[iplr], ibitRaceRegeneratingShields) != 0) {
        dpShdef += static_cast<std::int32_t>((dpShdef * 2)) / 5;
    }
    if (dpShdef & 0xffff0000) {
        dpShdef = 65535;
    }
    return static_cast<std::uint32_t>(LOWORD(dpShdef));
}

std::int16_t IStargateFromLppl(APPSESSION &app, PLANET *lppl) {
    std::int16_t chs;
    HS     *lphs;
    std::int16_t ihs;
    HUL    *lphul;

    if (!lppl || !lppl->fStarbase) {
        return -1;
    }
    lphul = &app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].hul;
    lphs = lphul->rghs;
    chs = lphul->chs;
    ihs = 0;
    while (ihs < chs) {
        if (lphs->grhst == hstSpecialSB && lphs->cItem > 0 && lphs->iItem < ispecialSBMassDriver5) {
            return lphs->iItem;
        }
        ihs++;
        lphs++;
    }
    return -1;
}

char *PszPlayerName(APPSESSION &app, std::int16_t iPlayer, std::int16_t fCapital, std::int16_t fPlural, std::int16_t fThe, std::int16_t grWord, PLAYER *pplr) {
    char *pchEnd;
    char  szName[50];

    if (!pplr) {
        pplr = &app.state.rgplr[iPlayer];
    }
    if (pplr->szName[0] != 0) {
        if (fThe) {
            std::strcpy(szName, "the ");
            if (fCapital) {
                szName[0] = 'T';
            }
        } else {
            szName[0] = 0;
        }
        if (fPlural && pplr->szNames[0] != 0) {
            std::strcat(szName, pplr->szNames);
        } else {
            std::strcat(szName, pplr->szName);
        }
        pchEnd = &szName[std::strlen(szName) - 1];
        while (*pchEnd == ' ' && pchEnd >= szName) {
            *pchEnd-- = 0;
        }
        if (pchEnd < szName) {
            TextIO().pfnCchGetString(app, idsName, szName);
        }
        if (fPlural && pplr->szNames[0] == 0) {
            pchEnd = &szName[std::strlen(szName) - 1];
            if (*pchEnd != 's' && (*pchEnd != 'e' || pchEnd[-1] != 's')) {
                std::strcat(szName, "s");
            }
        }
        if (grWord == 1) {
            TextIO().pfnCchGetString(app, idsHave2, &szName[std::strlen(szName)]);
        } else if (grWord == 2) {
            TextIO().pfnCchGetString(app, idsAre, &szName[std::strlen(szName)]);
        }
    } else {
        CchSprintf(szName, TextIO().pfnCompressed(app, idsPlayerD2), iPlayer + 1);
        if (!fPlural) {
            std::strcat(szName, "'s");
        }
        if (grWord == 1) {
            TextIO().pfnCchGetString(app, idsHas, &szName[std::strlen(szName)]);
        } else if (grWord == 2) {
            TextIO().pfnCchGetString(app, idsIs2, &szName[std::strlen(szName)]);
        }
    }
    std::strcpy(app.state.szWork, szName);
    return app.state.szWork;
}

std::int16_t FCalcFleetBombDamage(APPSESSION &app, FLEET *lpfl, std::int32_t *pdmgPeople, std::int32_t *pdmgPeopleMin, std::int32_t *pdmgPeopleSmart, std::int32_t *pdmgBldg, std::int32_t *ppctTerra,
                             std::int16_t *pfMulti) {
    std::int16_t iplr;
    std::int16_t cfl;
    FLEET  *lpflNext;
    std::int16_t dmgFloor;
    FLEET  *lpflHead;
    double  dmgSmart;
    std::int16_t fBomber;
    std::int16_t j;
    std::int16_t ishdef;
    PART    part;
    std::int32_t cIter;
    double  dmgT;

    iplr = lpfl->iPlayer;
    cfl = 0;
    dmgSmart = 1.0;
    lpflHead = lpfl;
    *ppctTerra = 0;
    *pdmgPeopleMin = 0;
    *pdmgBldg = 0;
    *pdmgPeopleSmart = 0;
    *pdmgPeople = 0;
    while (lpfl) {
        fBomber = 0;
        lpfl->fBombed = 1;
        for (ishdef = 0; ishdef < 16; ishdef++) {
            if (lpfl->rgcsh[ishdef] > 0) {
                for (j = 0; j < app.state.rglpshdef[iplr][ishdef].hul.chs; j++) {
                    if (app.state.rglpshdef[iplr][ishdef].hul.rghs[j].grhst == hstBomb) {
                        part.hs = app.state.rglpshdef[iplr][ishdef].hul.rghs[j];
                        PartQueryIO().pfnLookup(app, &part);
                        fBomber = 1;
                        if (part.hs.iItem == ibombRetroBomb) {
                            *ppctTerra += static_cast<std::uint32_t>((part.hs.cItem * lpfl->rgcsh[ishdef]));
                        } else if (part.hs.cItem > 0) {
                            if (part.pbomb()->dDmgBldg == 0) {
                                cIter = static_cast<std::uint32_t>((part.hs.cItem * lpfl->rgcsh[ishdef]));
                                dmgT = static_cast<double>((1.0 - static_cast<double>(part.pbomb()->dDmgCol) / 1000.0));
                                while (cIter-- > 0) {
                                    dmgSmart = static_cast<double>((static_cast<double>(dmgSmart) * dmgT));
                                }
                            } else {
                                *pdmgPeople += static_cast<std::uint32_t>((static_cast<std::uint32_t>((part.hs.cItem * lpfl->rgcsh[ishdef])) * part.pbomb()->dDmgCol));
                                *pdmgBldg += static_cast<std::uint32_t>((static_cast<std::uint32_t>((part.hs.cItem * lpfl->rgcsh[ishdef])) * part.pbomb()->dDmgBldg));
                                if (part.hs.iItem >= ibombLadyFingerBomb && part.hs.iItem <= ibombCherryBomb) {
                                    dmgFloor = 3;
                                    *pdmgPeopleMin += static_cast<std::uint32_t>((static_cast<std::uint32_t>((part.hs.cItem * lpfl->rgcsh[ishdef])) * dmgFloor));
                                }
                            }
                        }
                    } else if (app.state.rglpshdef[iplr][ishdef].hul.rghs[j].grhst == hstBeam &&
                               app.state.rglpshdef[iplr][ishdef].hul.rghs[j].iItem == ibeamMultiContainedMunition) {
                        part.hs = app.state.rglpshdef[iplr][ishdef].hul.rghs[j];
                        fBomber = 1;
                        *pdmgPeople += static_cast<std::uint32_t>((static_cast<std::uint32_t>((part.hs.cItem * lpfl->rgcsh[ishdef])) * 20));
                        *pdmgBldg += static_cast<std::uint32_t>((static_cast<std::uint32_t>((part.hs.cItem * lpfl->rgcsh[ishdef])) * 5));
                        *pdmgPeopleMin += static_cast<std::uint32_t>((static_cast<std::uint32_t>((part.hs.cItem * lpfl->rgcsh[ishdef])) * 3));
                    } else if (app.state.rglpshdef[iplr][ishdef].hul.rghs[j].grhst == hstSpecialM &&
                               app.state.rglpshdef[iplr][ishdef].hul.rghs[j].iItem == ispecialMOrbitalConstructionModule) {
                        part.hs = app.state.rglpshdef[iplr][ishdef].hul.rghs[j];
                        fBomber = 1;
                        *pdmgPeopleMin += static_cast<std::uint32_t>((static_cast<std::uint32_t>((part.hs.cItem * lpfl->rgcsh[ishdef])) * 20));
                    }
                }
            }
        }
        cfl++;
        for (lpflNext = lpfl->lpflNext; lpflNext && (lpflNext->fDead || lpflNext->iPlayer != iplr); lpflNext = lpflNext->lpflNext) {
        }
        if (lpflNext && lpflNext != lpflHead && lpflNext->idPlanet == lpfl->idPlanet) {
            lpfl = lpflNext;
        } else {
            lpfl = nullptr;
        }
    }
    *pdmgPeopleSmart = static_cast<std::int32_t>((1000.0 - static_cast<double>(dmgSmart) * 1000.0 + 0.5));
    if (*pdmgPeopleSmart >= 1000) {
        *pdmgPeopleSmart = 1000;
    }
    *pfMulti = cfl > 1;
    if (*pdmgPeople != 0 || *pdmgPeopleMin != 0 || *pdmgPeopleSmart != 0 || *pdmgBldg != 0 || *ppctTerra != 0) {
        return 1;
    }
    return 0;
}

void LinkFleets(APPSESSION &app, std::int16_t fUnused) {
    FLEET **pSearch;
    POINT16 pt;
    FLEET  *rglpflSrc[63];
    FLEET  *lpflTail;
    FLEET  *lpflHead;
    std::int16_t i;
    std::int16_t iflTail;
    std::int16_t iflHead;
    std::int16_t cSrc;

    for (iflHead = 0; iflHead < app.state.cFleet; iflHead++) {
        lpflHead = app.state.rglpfl[iflHead];
        if (!app.state.rglpfl[iflHead])
            break;
        lpflHead->lpflNext = nullptr;
        lpflHead->fDone = 0;
    }
    cSrc = 0;
    for (iflHead = 0; iflHead < app.state.cFleet; iflHead++) {
        lpflHead = app.state.rglpfl[iflHead];
        if (!lpflHead->fDead && !lpflHead->lpflNext) {
            for (i = 0; i < cSrc; i++) {
                if (lpflHead->pt.y <= rglpflSrc[i]->pt.y && (lpflHead->pt.y < rglpflSrc[i]->pt.y || static_cast<std::uint16_t>(lpflHead->pt.x) <= static_cast<std::uint16_t>(rglpflSrc[i]->pt.x))) {
                    if (lpflHead->pt.x == rglpflSrc[i]->pt.x && lpflHead->pt.y == rglpflSrc[i]->pt.y) {
                        lpflHead->lpflNext = rglpflSrc[i]->lpflNext;
                        rglpflSrc[i]->lpflNext = lpflHead;
                        i = -1;
                    }
                    break;
                }
            }
            if (i != -1) {
                lpflHead->lpflNext = lpflHead;
                if (i < cSrc) {
                    std::memmove(&rglpflSrc[i + 1], &rglpflSrc[i], (cSrc - i) * sizeof(FLEET *));
                }
                rglpflSrc[i] = lpflHead;
                cSrc++;
                if (cSrc >= 63) {
                    for (iflTail = iflHead + 1; iflTail < app.state.cFleet; iflTail++) {
                        lpflTail = app.state.rglpfl[iflTail];
                        if (!lpflTail->fDead && !lpflTail->lpflNext) {
                            pt = lpflTail->pt;
                            pSearch = static_cast<FLEET **>(std::bsearch(&pt, rglpflSrc, cSrc, sizeof(FLEET *), reinterpret_cast<QSORTCOMPARE>(ICompFleetPoint2)));
                            if (pSearch) {
                                lpflHead = *pSearch;
                                lpflTail->lpflNext = lpflHead->lpflNext;
                                lpflHead->lpflNext = lpflTail;
                            }
                        }
                    }
                    cSrc = 0;
                }
            }
        }
    }
    app.state.gd.fFleetLinkValid = 1;
    return;
}

int ICompFleetPoint(FLEET **ppfl1, FLEET **ppfl2) {
    std::int32_t l2;
    std::int32_t l1;

    l1 = (static_cast<std::uint32_t>((*ppfl1)->pt.x) & 0xffff) | (static_cast<std::uint32_t>((*ppfl1)->pt.y) & 0xffff) << 0x10;
    l2 = (static_cast<std::uint32_t>((*ppfl2)->pt.x) & 0xffff) | (static_cast<std::uint32_t>((*ppfl2)->pt.y) & 0xffff) << 0x10;
    l1 -= l2;
    if (l1 < 0) {
        l1 = -1;
    } else if (l1 > 0) {
        l1 = 1;
    }
    return static_cast<std::int16_t>(LOWORD(l1));
}

int ICompFleetPoint2(std::int32_t *pl, FLEET **ppfl) {
    std::int32_t l2;
    std::int32_t l1;

    l1 = *pl;
    l2 = (static_cast<std::uint32_t>((*ppfl)->pt.x) & 0xffff) | (static_cast<std::uint32_t>((*ppfl)->pt.y) & 0xffff) << 0x10;
    l1 -= l2;
    if (l1 < 0) {
        l1 = -1;
    } else if (l1 > 0) {
        l1 = 1;
    }
    return static_cast<std::int16_t>(LOWORD(l1));
}

std::int16_t FLookupSelShip(APPSESSION &app, FLEET *pfl) {
    if (app.state.sel.scan.grobj != grobjFleet) {
        return 0;
    }
    return FleetQueryIO().pfnLookup(app, app.state.rglpfl[app.state.sel.scan.ifl]->id, pfl);
}

FLEET *LpflFromId(APPSESSION &app, std::int16_t idFleet) {
    std::int16_t iplr;
    std::int16_t idGuess;
    std::int16_t iLo;
    std::int16_t iGuess;
    std::int16_t i;
    FLEET  *lpfl;
    std::int16_t iplrCur;
    std::int16_t iHi;

    i = 0;
    iplr = static_cast<std::uint16_t>(idFleet) >> 9 & 0xf;
    idFleet &= 0x1fff;
    for (iplrCur = 0; iplrCur < iplr; iplrCur++) {
        i += app.state.rgplr[iplrCur].cFleet;
    }
    iLo = i - 1;
    iHi = app.state.cFleet;
    while (iLo + 1 < iHi) {
        iGuess = (iLo + iHi) >> 1;
        lpfl = app.state.rglpfl[iGuess];
        idGuess = app.state.rglpfl[iGuess]->id;
        if (idGuess < idFleet) {
            iLo = iGuess;
        } else {
            if (idGuess <= idFleet) {
                return lpfl;
            }
            iHi = iGuess;
        }
    }
    return nullptr;
}

std::int16_t FLookupFleet(APPSESSION &app, std::int16_t idFleet, FLEET *pfl) {
    FLEET  *lpfl;
    std::int16_t fWrite;

    fWrite = 0;
    if (app.state.cFleet <= 0) {
        return 0;
    }
    if (idFleet < 0) {
        idFleet = pfl->id;
        fWrite = 1;
    }
    lpfl = LpflFromId(app, idFleet);
    if (lpfl && pfl) {
        if (fWrite) {
            PresentationIO().pfnInvalidateReport(app, rptFleets, 0);
            LogChangeFleet(app, lpfl, pfl);
            if (lpfl->lpplord != pfl->lpplord) {
                if (lpfl->lpplord->iordMax < pfl->cord) {
                    lpfl->lpplord = LpplReAlloc(app, lpfl->lpplord, pfl->cord + 3);
                }
                HeapIO().pfnCopy(lpfl->lpplord->rgord(), pfl->lpplord->rgord(), pfl->lpplord->iordMac * 18);
                lpfl->lpplord->iordMac = pfl->lpplord->iordMac;
            }
            HeapIO().pfnCopy(lpfl, pfl, 100);
            if (app.state.gd.fTutorial && app.state.idPlayer == 0) {
                PresentationIO().pfnTutor(app);
            }
        } else if (pfl == &app.state.sel.fl) {
            FDupFleet(app, lpfl, pfl);
        } else {
            *pfl = *lpfl;
        }
    }
    if (lpfl) {
        return 1;
    }
    return 0;
}

std::int16_t FDupFleet(APPSESSION &app, FLEET *lpfl, FLEET *pfl) {
    PLORD *lpplordT;

    lpplordT = pfl->lpplord;
    *pfl = *lpfl;
    if (!lpfl->lpplord) {
        if (lpplordT) {
            FreePl(app, lpplordT);
        }
        return 1;
    }
    pfl->lpplord = lpplordT;
    if (!pfl->lpplord) {
        pfl->lpplord = LpplAlloc<PLORD>(app, 18, lpfl->lpplord->iordMax, htOrd);
    } else if (static_cast<std::int16_t>(pfl->lpplord->iordMax) < lpfl->lpplord->iordMac) {
        pfl->lpplord = LpplReAlloc(app, pfl->lpplord, lpfl->lpplord->iordMax);
    }
    HeapIO().pfnCopy(pfl->lpplord->rgord(), lpfl->lpplord->rgord(), lpfl->lpplord->iordMac * 18);
    pfl->lpplord->iordMac = lpfl->lpplord->iordMac;
    return 1;
}

std::int16_t FLookupObject(APPSESSION &app, GrobjClass grobj, std::int16_t id, void *pobj) {
    if (grobj == grobjFleet) {
        return FleetQueryIO().pfnLookup(app, id, static_cast<FLEET *>(pobj));
    }
    if (grobj == grobjPlanet) {
        return PlanetQueryIO().pfnLookup(app, id, static_cast<PLANET *>(pobj));
    }
    return FLookupThing(app, id, static_cast<THING *>(pobj));
}

std::int16_t FLookupOrbitingXfer(APPSESSION &app, std::int16_t idPlanet, std::int16_t iNth, XFER *pxf, std::int16_t idSkip) {
    std::int16_t i;
    THING  *lpth;
    FLEET  *lpfl;
    THING  *lpthMac;

    if (app.state.cFleet <= 0) {
        return 0;
    }
    if (app.state.cFleet != 0) {
        for (i = 0; i < app.state.cFleet; i++) {
            lpfl = app.state.rglpfl[i];
            if (!app.state.rglpfl[i])
                break;
            if (lpfl->idPlanet == idPlanet && lpfl->id != idSkip &&
                ((idSkip == idflNone || (lpfl->pt.x == app.state.sel.pt.x && lpfl->pt.y == app.state.sel.pt.y)) && iNth-- == 0)) {
                if (pxf) {
                    pxf->fl() = *lpfl;
                    pxf->grobj = grobjFleet;
                    pxf->id = lpfl->id;
                }
                return 1;
            }
        }
    }
    lpth = app.state.lpThings;
    lpthMac = app.state.lpThings + app.state.cThing;
    for (; lpth < lpthMac; lpth++) {
        if (lpth->ith() == ithMineralPacket && lpth->pt().x() == app.state.sel.pt.x && lpth->pt().y() == app.state.sel.pt.y && iNth-- == 0) {
            if (pxf) {
                pxf->th() = *lpth;
                pxf->grobj = grobjThing;
                pxf->id = lpth->idFull();
            }
            return 1;
        }
    }
    return 0;
}

char *PszGetThingName(APPSESSION &app, std::int16_t id) {
    THING *lpth;
    char   szPlr[54];

    lpth = LpthFromId(app, id);
    if (!lpth) {
        app.state.szWork[0] = 0;
        return app.state.szWork;
    }
    switch (lpth->ith()) {
    case ithMinefield:
        if (lpth->iplr() != app.state.idPlayer) {
            CchSprintf(szPlr, "%s ", TextIO().pfnPlayerName(app, lpth->iplr(), 0, 0, 0, 0, nullptr));
        } else {
            szPlr[0] = 0;
        }
        CchSprintf(app.state.szWork, TextIO().pfnCompressed(app, idsSSMineField), szPlr, app.state.rgszMineField[lpth->thm().iType()]);
        break;
    case ithMineralPacket:
        if (lpth->thp().iWarp() == 0) {
            TextIO().pfnCchGetString(app, idsSalvage, app.state.szWork);
            return app.state.szWork;
        }
        if (lpth->iplr() != app.state.idPlayer) {
            CchSprintf(szPlr, "%s ", TextIO().pfnPlayerName(app, lpth->iplr(), 0, 0, 0, 0, nullptr));
        } else {
            szPlr[0] = 0;
        }
        CchSprintf(app.state.szWork, TextIO().pfnCompressed(app, idsSmineralPacket), szPlr);
        break;
    case ithWormhole:
        std::strcpy(app.state.szWork, TextIO().pfnCompressed(app, idsWormhole));
        break;
    case ithMysteryTrader:
        std::strcpy(app.state.szWork, TextIO().pfnCompressed(app, idsMysteryTrader));
        break;
    default:
        std::strcpy(app.state.szWork, TextIO().pfnCompressed(app, idsMysteryObject));
    }
    return app.state.szWork;
}

char *PszGetFleetName(APPSESSION &app, std::int16_t id) {
    std::int16_t cshdef;
    std::int16_t iplr;
    char   *lpsz;
    char    szShdef[34];
    std::int16_t ifl;
    FLEET  *lpfl;
    char    szPlr[34];
    std::int16_t ishdef;
    std::int16_t cch;

    lpfl = LpflFromId(app, id & 0x7fff);
    id &= 0x7fff;
    iplr = static_cast<std::uint16_t>(id) >> 9 & 0xf;
    ifl = id & 0x1ff;
    if (iplr != app.state.idPlayer) {
        CchSprintf(szPlr, "%s ", TextIO().pfnPlayerName(app, iplr, 0, 0, 0, 0, nullptr));
    } else {
        szPlr[0] = 0;
    }
    if (lpfl && lpfl->lpszName) {
        CchSprintf(app.state.szWork, "%s%s", szPlr, lpfl->lpszName);
    } else {
        if (lpfl) {
            ishdef = IshdefPrimaryFromLpfl(app, lpfl, &cshdef);
            if (ishdef == 16) {
                lpsz = TextIO().pfnCompressed(app, idsFleet);
            } else {
                std::strcpy(szShdef, app.state.rglpshdef[iplr][ishdef].hul.szClass);
                cch = std::strlen(szShdef);
                if (cch > 28) {
                    cch = static_cast<std::int16_t>(CchUtf8Prefix(szShdef, 28));
                }
                if (cshdef > 1) {
                    szShdef[cch] = '+';
                    szShdef[cch + 1] = 0;
                }
                lpsz = szShdef;
            }
        } else {
            lpsz = TextIO().pfnCompressed(app, idsFleet);
        }
        CchSprintf(app.state.szWork, "%s%s #%d", szPlr, lpsz, ifl + 1);
    }
    return app.state.szWork;
}

std::uint16_t WFromLpfl(APPSESSION &app, FLEET *lpfl) {
    std::int16_t  cshdef;
    std::uint16_t w;
    std::int16_t  ishdef;

    ishdef = IshdefPrimaryFromLpfl(app, lpfl, &cshdef);
    w = ishdef << 9 | lpfl->ifl();
    if (cshdef > 1) {
        w |= 0x2000;
    }
    return w;
}

char *PszFleetNameFromWord(APPSESSION &app, std::uint16_t w) {
    char   *lpsz;
    char    szShdef[34];
    std::int16_t ishdef;
    std::int16_t cch;

    ishdef = w >> 9 & 0xf;
    if (app.state.rglpshdef[app.state.idPlayer][ishdef].fFree) {
        lpsz = TextIO().pfnCompressed(app, idsFleet);
    } else {
        std::strcpy(szShdef, app.state.rglpshdef[app.state.idPlayer][ishdef].hul.szClass);
        cch = std::strlen(szShdef);
        if (cch > 28) {
            cch = static_cast<std::int16_t>(CchUtf8Prefix(szShdef, 28));
        }
        if (w & 0x2000) {
            szShdef[cch] = '+';
            szShdef[cch + 1] = 0;
        }
        lpsz = szShdef;
    }
    CchSprintf(app.state.szWork, "%s #%d", lpsz, (w & 0x1ff) + 1);
    return app.state.szWork;
}

char *PszGetPlanetName(APPSESSION &app, std::int16_t id) {
    std::int16_t fInOrbit;
    char   *psz;

    fInOrbit = id & 0x8000;
    id &= 0x7fff;
    id = app.state.rgidPlan[id];
    psz = PszGetCompressedPlanet(app, id);
    if (fInOrbit) {
        CchSprintf(app.state.szWork, TextIO().pfnCompressed(app, idsOrbitingS), psz);
    } else {
        std::strcpy(app.state.szWork, psz);
    }
    return app.state.szWork;
}

std::int16_t IflFromLpfl(APPSESSION &app, FLEET *lpfl) {
    std::int16_t i;

    for (i = 0; i < app.state.cFleet; i++) {
        if (app.state.rglpfl[i] == lpfl) {
            return i;
        }
    }
    return iflNone;
}

std::int16_t FDeleteFleet(APPSESSION &app, std::int16_t idFleet, GrobjClass grobjSel, std::int16_t idSel) {
    std::int16_t i;
    FLEET  *lpfl;
    std::int16_t iPlr;
    std::int16_t idDel;
    PLANET *lppl;

    for (i = 0; i < app.state.cFleet; i++) {
        lpfl = app.state.rglpfl[i];
        if (!app.state.rglpfl[i] || lpfl->id == idFleet)
            break;
        if (lpfl->id > idFleet) {
            return 0;
        }
    }
    if (i == app.state.cFleet) {
        return 0;
    }
    if (idFleet == app.state.sel.fl.id) {
        ShowScanSel(app, 0);
    }
    lpfl->fDead = 1;
    FleetOrdersChangeTarget(app, lpfl);
    FreePl(app, lpfl->lpplord);
    if (lpfl->lpszName) {
        HeapIO().pfnFree(app, lpfl->lpszName, htString);
    }
    app.state.cFleet--;
    iPlr = lpfl->iPlayer;
    idDel = lpfl->id;
    app.state.rgplr[iPlr].cFleet--;
    if (grobjSel == grobjNone && lpfl->idPlanet != idPlanetDeepSpace) {
        lppl = LpplFromId(app, lpfl->idPlanet);
        if (lppl && lppl->iPlayer == app.state.idPlayer) {
            grobjSel = grobjPlanet;
            idSel = lpfl->idPlanet;
        }
    }
    if (app.state.cFleet != i) {
        std::memmove(app.state.rglpfl + i, app.state.rglpfl + (i + 1), (app.state.cFleet - i) * sizeof(FLEET *));
    }
    HeapIO().pfnFree(app, lpfl, htFleets);
    app.state.gd.fFleetLinkValid = 0;
    if (app.state.sel.fl.id != idflNone) {
        if (i < app.state.sel.scan.ifl) {
            app.state.sel.scan.ifl--;
        } else if (idDel == app.state.sel.fl.id) {
            if (grobjSel == grobjNone) {
                app.state.sel.grobj = grobjNone;
                app.state.sel.scan.grobj = grobjNone;
                PresentationIO().pfnFindSelection(app);
                return 1;
            }
            app.state.sel.grobj = grobjNone;
            if (grobjSel == grobjFleet) {
                PresentationIO().pfnSelectFleet(app, 0, idSel);
            } else {
                PresentationIO().pfnSelectPlanet(app, 0, idSel);
            }
        }
    }
    if (!app.state.gd.fGeneratingTurn) {
        UpdateMsgTitle(app);
    }
    return 1;
}

std::int16_t FFindSomethingAndSelectIt(APPSESSION &app) {
    PLANET *lpplMac;
    PLANET *lppl;
    std::int16_t i;
    FLEET  *lpfl;

    lppl = LpplFromId(app, app.state.rgplr[app.state.idPlayer].idPlanetHome);
    if (!lppl || lppl->iPlayer != app.state.idPlayer) {
        lppl = app.state.lpPlanets;
        lpplMac = app.state.lpPlanets + app.state.cPlanet;
        for (; lppl < lpplMac && lppl->iPlayer != app.state.idPlayer; lppl++) {
        }
        if (lppl == lpplMac) {
            lppl = nullptr;
        }
    }
    if (lppl) {
        PresentationIO().pfnSelectPlanet(app, 0, lppl->id);
        return 1;
    }
    for (i = 0; i < app.state.cFleet; i++) {
        lpfl = app.state.rglpfl[i];
        if (!app.state.rglpfl[i])
            break;
        if (lpfl->iPlayer == app.state.idPlayer) {
            PresentationIO().pfnSelectFleet(app, 0, lpfl->id);
            return 1;
        }
    }
    return 0;
}

FLEET *LpflNew(APPSESSION &app, std::int16_t iPlr, std::int16_t idPl) {
    std::int16_t i;
    ORDER  *lpord;
    FLEET  *lpfl;
    std::int16_t iflPrev;

    iflPrev = -1;
    for (i = 0; i < app.state.cFleet; i++) {
        lpfl = app.state.rglpfl[i];
        if (!app.state.rglpfl[i])
            break;
        if (lpfl->iPlayer >= iPlr) {
            if (lpfl->iPlayer > iPlr || lpfl->ifl() != iflPrev + 1)
                break;
            iflPrev = lpfl->ifl();
        }
    }
    app.state.rglpfl = LpReAlloc<FLEET *>(app, app.state.rglpfl, (app.state.cFleet + 1) * sizeof(FLEET *), htMisc);
    if (app.state.cFleet != i) {
        std::memmove(app.state.rglpfl + (i + 1), app.state.rglpfl + i, (app.state.cFleet - i) * sizeof(FLEET *));
    }
    app.state.rglpfl[i] = lpfl = LpAlloc<FLEET>(app, sizeof(FLEET), htFleets);
    app.state.cFleet++;
    app.state.rgplr[iPlr].cFleet++;
    std::memset(lpfl, 0, sizeof(FLEET));
    lpfl->SetIfl(iflPrev + 1);
    lpfl->iPlayer = iPlr;
    lpfl->SetIplr(iPlr);
    lpfl->det = detAll;
    lpfl->idPlanet = idPl;
    if (idPl != idPlanetDeepSpace) {
        lpfl->pt = app.state.rgptPlan[idPl];
    }
    lpfl->cord = 1;
    lpfl->fRepOrders = 0;
    lpfl->lpplord = LpplAlloc<PLORD>(app, 18, 3, htOrd);
    lpfl->lpplord->iordMac = 1;
    lpfl->SetFdirValid(0);
    lpord = lpfl->lpplord->rgord();
    lpord->pt = lpfl->pt;
    lpord->id = lpfl->idPlanet;
    lpord->grobj = lpfl->idPlanet == idPlanetDeepSpace ? 4 : 1;
    lpord->iWarp = 0;
    lpord->fValidTask = 1;
    lpord->grTask = grTaskNone;
    if (app.state.sel.scan.ifl != iflNone && i <= app.state.sel.scan.ifl) {
        app.state.sel.scan.ifl++;
    }
    app.state.gd.fFleetLinkValid = 0;
    return lpfl;
}

FLEET *LpflNewSplit(APPSESSION &app, FLEET *pfl) {
    std::int16_t iordMac;
    FLEET  *lpflNew;

    lpflNew = ProductionIO().pfnNewFleet(app, pfl->iPlayer, pfl->idPlanet);
    if (pfl->idPlanet == idPlanetDeepSpace) {
        lpflNew->pt = pfl->pt;
    }
    lpflNew->fRepOrders = pfl->fRepOrders;
    lpflNew->det = pfl->det;
    lpflNew->iplan = pfl->iplan;
    iordMac = pfl->lpplord->iordMac;
    if (lpflNew->lpplord->iordMax < iordMac) {
        lpflNew->lpplord = LpplReAlloc(app, lpflNew->lpplord, pfl->lpplord->iordMax);
    }
    HeapIO().pfnCopy(lpflNew->lpplord->rgord(), pfl->lpplord->rgord(), iordMac * 18);
    lpflNew->lpplord->iordMac = iordMac;
    lpflNew->cord = pfl->cord;
    LogSplitFleet(app, pfl->id);
    return lpflNew;
}

std::int16_t FFleetMergeAll(APPSESSION &app, FLEET *pfl) {
    std::int16_t iplr;
    std::int32_t dpT;
    std::int16_t fCshOverflow;
    std::int16_t rgcshDamaged[16];
    std::int16_t cflMerge;
    std::int16_t i;
    FLEET  *lpfl;
    std::int16_t cshT;
    SHDEF  *lpshdef;
    FLEET  *lpflMerge;
    std::int32_t rgdp[16];
    std::int16_t j;

    lpflMerge = nullptr;
    cflMerge = 0;
    fCshOverflow = 0;
    std::memset(rgdp, 0, 64);
    std::memset(rgcshDamaged, 0, 32);
    iplr = pfl->iPlayer;
    for (i = 0; i < app.state.vcflMerge; i++) {
        if (app.state.vrgiflMerge[i] != iflNone) {
            lpfl = LpflFromId(app, app.state.vrgiflMerge[i]);
            /* Ship counts are int16_t. The original clamped a count that
               passed 32767 to 32766 and lost the rest; a fleet that doesn't
               fit is left out of the merge. */
            if (lpfl && lpfl->ifl() != pfl->ifl()) {
                for (j = 0; j < 16 && static_cast<std::int32_t>(pfl->rgcsh[j]) + lpfl->rgcsh[j] <= 32767; j++) {
                }
                if (j < 16) {
                    continue;
                }
            }
            if (lpfl) {
                lpshdef = app.state.rglpshdef[iplr];
                j = 0;
                while (j < 16) {
                    if (lpfl->rgcsh[j] != 0) {
                        if (lpfl->rgdv[j].dp != 0) {
                            cshT = LOWORD(static_cast<std::int32_t>((lpfl->rgcsh[j] * static_cast<std::int16_t>(lpfl->rgdv[j].pctSh()))) / 100);
                            if (cshT == 0) {
                                cshT = 1;
                            }
                            rgcshDamaged[j] += cshT;
                            dpT = static_cast<std::uint32_t>((lpfl->rgdv[j].pctDp() * cshT));
                            rgdp[j] += dpT;
                        }
                        if (lpfl->ifl() != pfl->ifl()) {
                            pfl->rgcsh[j] += lpfl->rgcsh[j];
                            lpfl->rgcsh[j] = 0;
                        }
                    }
                    j++;
                    lpshdef++;
                }
                if (lpfl->ifl() == pfl->ifl()) {
                    lpflMerge = lpfl;
                } else {
                    for (j = 0; j < 5; j++) {
                        pfl->rgwtMin[j] += lpfl->rgwtMin[j];
                    }
                    FDeleteFleet(app, lpfl->id, grobjNone, -1);
                    cflMerge++;
                }
            }
        }
    }
    lpshdef = app.state.rglpshdef[iplr];
    j = 0;
    while (j < 16) {
        lpflMerge->rgcsh[j] = pfl->rgcsh[j];
        if (rgcshDamaged[j] == 0 || lpflMerge->rgcsh[j] == 0) {
            lpflMerge->rgdv[j].dp = 0;
        } else {
            lpflMerge->rgdv[j].SetPctSh(LOWORD(static_cast<std::int32_t>((static_cast<std::int32_t>((static_cast<std::uint32_t>((rgcshDamaged[j] * 100)) + static_cast<std::int16_t>((lpflMerge->rgcsh[j] - 1)))) / lpflMerge->rgcsh[j]))));
            lpflMerge->rgdv[j].SetPctDp(LOWORD(static_cast<std::int32_t>((rgdp[j] / rgcshDamaged[j]))));
        }
        j++;
        lpshdef++;
    }
    for (j = 0; j < 5; j++) {
        lpflMerge->rgwtMin[j] = pfl->rgwtMin[j];
    }
    LogMergeFleet(app, pfl->id);
    PresentationIO().pfnInvalidateReport(app, rptFleets, 2);
    if (cflMerge != 0) {
        return 1;
    }
    return 0;
}

std::int16_t FFleetSplitAll(APPSESSION &app, FLEET *pfl) {
    FLEET   flNew;
    std::int16_t cSplit;
    std::int16_t c;
    std::int16_t i;
    FLEET  *lpflNew;

    cSplit = 0;
    for (i = 0; i < 16; i++) {
        c = pfl->rgcsh[i];
        while (c-- != 0) {
            if (cSplit++ != 0) {
                lpflNew = LpflNewSplit(app, pfl);
                flNew = *lpflNew;
                pfl->rgcsh[i]--;
                flNew.rgcsh[i]++;
                FleetTransferCargoBalance(app, pfl, &flNew);
                FleetQueryIO().pfnLookup(app, idWriteBack, pfl);
                FleetQueryIO().pfnLookup(app, idWriteBack, &flNew);
            }
        }
    }
    PresentationIO().pfnInvalidateReport(app, rptFleets, 2);
    if (cSplit > 1) {
        return 1;
    }
    return 0;
}

char *PszGetLocName(APPSESSION &app, GrobjClass grobj, std::int16_t id, std::int16_t x, std::int16_t y) {
    if (id == -1)
        goto NoObj;
    if (grobj == grobjPlanet)
        return TextIO().pfnPlanetName(app, id);
    if (grobj == grobjFleet)
        return PszGetFleetName(app, id);
    if (grobj == grobjThing)
        return PszGetThingName(app, id);
NoObj:
    if (x == -1 && y == -1) {
        std::strcpy(app.state.szWork, TextIO().pfnCompressed(app, idsDeepSpace));
    } else {
        CchSprintf(app.state.szWork, TextIO().pfnCompressed(app, idsSpaceDD), x, y);
    }
    return app.state.szWork;
}

std::int16_t IshdefPrimaryFromLpfl(APPSESSION &app, FLEET *lpfl, std::int16_t *pcDiff) {
    std::int16_t cDiff;
    std::int16_t ish;
    std::int16_t i;
    std::int16_t csh;
    HulDef  ihul;

    cDiff = 0;
    csh = 0;
    ish = 16;
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            cDiff++;
            if (lpfl->rgcsh[i] > csh) {
                ish = i;
                csh = lpfl->rgcsh[i];
                ihul = app.state.rglpshdef[lpfl->iPlayer][i].hul.ihuldef;
                if (ihul == ihuldefFuelTransport || ihul == ihuldefSuperFuelXport) {
                    csh--;
                }
            }
        }
    }
    if (pcDiff) {
        *pcDiff = cDiff;
    }
    return ish;
}

char *PszGetDistance(APPSESSION &app, std::int16_t x1, std::int16_t y1, std::int16_t x2, std::int16_t y2) {
    std::int32_t d;
    std::int16_t fStarted;
    std::int32_t d2;

    fStarted = 0;
    d = static_cast<std::int32_t>((static_cast<double>(DGetDistance(x1, y1, x2, y2)) * 100.0 + 0.5));
    d2 = static_cast<std::int32_t>((d / 100));
    d -= static_cast<std::uint32_t>((d2 * 100));
    if (app.state.dyArial8 <= 14) {
        CchSprintf(app.state.szWork, TextIO().pfnCompressed(app, idsLdLdLightYears), d2, d);
    } else {
        CchSprintf(app.state.szWork, TextIO().pfnCompressed(app, idsLdLdLY), d2, d);
    }
    return app.state.szWork;
}

double DGetDistance(std::int16_t x1, std::int16_t y1, std::int16_t x2, std::int16_t y2) {
    std::int32_t dy;
    std::int32_t dx;
    std::int32_t l;

    dx = static_cast<std::int16_t>((x2 - x1));
    dy = static_cast<std::int16_t>((y2 - y1));
    l = static_cast<std::uint32_t>((dx * dx)) + static_cast<std::uint32_t>((dy * dy));
    return std::sqrt(static_cast<double>(l));
}

std::int16_t FFindNearestObject(APPSESSION &app, POINT16 pt, GrobjClass grobj, SCAN *pscan) {
    POINT16  ptWp;
    POINT16 *ppt;
    std::int16_t  dy;
    std::int32_t  lTry;
    THING   *lpth;
    FLEET   *lpfl;
    std::int16_t  i;
    THING   *lpthMac;
    std::int32_t  lSquare;
    SCAN     scanT;
    std::int16_t  iNearest;
    std::int16_t  dx;
    SCAN     scan;

    iNearest = -1;
    lSquare = 1000000000;
    ppt = app.state.rgptPlan;
    scan.grobjFull = grobjNone;
    scan.grobj = grobjNone;
    scan.pt.y = 0;
    scan.pt.x = 0;
    scan.ith = ithNone;
    scan.iwp = iwpNone;
    scan.ifl = iflNone;
    scan.idpl = idplNone;
    if (grobj & mdScanRadius) {
        lSquare = ScanToPt(app, 20);
        lSquare = static_cast<std::uint32_t>((lSquare * lSquare));
    } else if (grobj & mdExact) {
        lSquare = 0;
    }
    if (grobj & grobjPlanet) {
        i = 0;
        while (i < app.state.game.cPlanMax) {
            dx = pt.x - ppt->x;
            dy = pt.y - ppt->y;
            lTry = static_cast<std::uint32_t>((dx * dx));
            if (static_cast<std::int32_t>(static_cast<std::uint32_t>((dx * dx))) <= lSquare) {
                lTry += static_cast<std::uint32_t>((dy * dy));
                if (lTry <= lSquare && (lSquare != lTry || lSquare == 0)) {
                    lSquare = lTry;
                    scan.pt = *ppt;
                    scan.idpl = i;
                    scan.grobjFull = grobjPlanet;
                    scan.grobj = grobjPlanet;
                }
            }
            i++;
            ppt++;
        }
    }
    if (grobj & grobjFleet) {
        for (i = 0; i < app.state.cFleet; i++) {
            lpfl = app.state.rglpfl[i];
            if (!app.state.rglpfl[i])
                break;
            dx = pt.x - lpfl->pt.x;
            dy = pt.y - lpfl->pt.y;
            lTry = static_cast<std::uint32_t>((dx * dx));
            if (static_cast<std::int32_t>(static_cast<std::uint32_t>((dx * dx))) <= lSquare) {
                lTry += static_cast<std::uint32_t>((dy * dy));
                if (lTry <= lSquare) {
                    if ((lTry == lSquare && !(scan.grobj & grobjPlanet)) || (lpfl->pt.x == scan.pt.x && lpfl->pt.y == scan.pt.y)) {
                        if ((scan.grobjFull & grobjFleet) && (app.state.rglpfl[scan.ifl]->iPlayer == app.state.idPlayer || lpfl->iPlayer != app.state.idPlayer))
                            continue;
                        scan.ifl = i;
                        scan.grobjFull = static_cast<GrobjClass>(scan.grobjFull | grobjFleet);
                        if (scan.grobj == grobjNone)
                            goto SelectShip;
                        continue;
                    } else {
                        if (lTry >= lSquare)
                            continue;
                        lSquare = lTry;
                    }
                SelectShip:
                    scan.pt = lpfl->pt;
                    scan.ifl = i;
                    scan.idpl = idplNone;
                    scan.grobjFull = grobjFleet;
                    scan.grobj = grobjFleet;
                }
            }
        }
    }
    if (grobj & grobjThing) {
        lpth = app.state.lpThings;
        lpthMac = app.state.lpThings + app.state.cThing;
        for (; lpth < lpthMac; lpth++) {
            dx = pt.x - lpth->pt().x();
            dy = pt.y - lpth->pt().y();
            lTry = static_cast<std::uint32_t>((dx * dx));
            if (static_cast<std::int32_t>(static_cast<std::uint32_t>((dx * dx))) <= lSquare) {
                lTry += static_cast<std::uint32_t>((dy * dy));
                if (lTry <= lSquare) {
                    if ((lTry == lSquare && !(scan.grobj & (grobjPlanet | grobjFleet))) || (lpth->pt().x() == scan.pt.x && lpth->pt().y() == scan.pt.y)) {
                        if (scan.grobjFull & grobjThing)
                            continue;
                        scan.ith = static_cast<std::int16_t>((reinterpret_cast<std::uint8_t *>(lpth) - reinterpret_cast<std::uint8_t *>(app.state.lpThings))) / 18;
                        scan.grobjFull = static_cast<GrobjClass>(scan.grobjFull | grobjThing);
                        if (scan.grobj == grobjNone)
                            goto SelectThing;
                        continue;
                    } else {
                        if (lTry >= lSquare)
                            continue;
                        lSquare = lTry;
                    }
                SelectThing:
                    scan.pt = lpth->pt();
                    scan.ith = static_cast<std::int16_t>((reinterpret_cast<std::uint8_t *>(lpth) - reinterpret_cast<std::uint8_t *>(app.state.lpThings))) / 18;
                    scan.idpl = idplNone;
                    scan.ifl = iflNone;
                    scan.grobjFull = grobjThing;
                    scan.grobj = grobjThing;
                }
            }
        }
    }
    if ((grobj & grobjOther) && app.state.sel.grobj == grobjFleet) {
        for (i = app.state.sel.fl.cord - 1; i >= 0; i--) {
            ptWp = app.state.sel.fl.lpplord->rgord()[i].pt;
            dx = pt.x - ptWp.x;
            dy = pt.y - ptWp.y;
            lTry = static_cast<std::uint32_t>((dx * dx));
            if (static_cast<std::int32_t>(static_cast<std::uint32_t>((dx * dx))) <= lSquare) {
                lTry += static_cast<std::uint32_t>((dy * dy));
                if (lTry <= lSquare) {
                    if ((lTry == lSquare && !(scan.grobj & (grobjPlanet | grobjFleet | grobjThing))) || (ptWp.x == scan.pt.x && ptWp.y == scan.pt.y)) {
                        if ((scan.grobjFull & grobjOther) && i != app.state.sel.scan.iwp)
                            continue;
                        scan.iwp = i;
                        scan.grobjFull = static_cast<GrobjClass>(scan.grobjFull | grobjOther);
                        if (scan.grobj == grobjNone)
                            goto SelectSpace;
                        continue;
                    } else {
                        if (lTry >= lSquare)
                            continue;
                        lSquare = lTry;
                    }
                SelectSpace:
                    scan.pt = ptWp;
                    scan.iwp = i;
                    scan.ith = ithNone;
                    scan.idpl = idplNone;
                    scan.ifl = iflNone;
                    scan.grobjFull = grobjOther;
                    scan.grobj = grobjOther;
                }
            }
        }
    }
    if (!(grobj & mdNoRecurse) && scan.grobj != grobjNone &&
        FFindNearestObject(app, scan.pt, static_cast<GrobjClass>(((grobj & (grobjPlanet | grobjFleet | grobjOther | grobjThing)) ^ 0xf) | 0xa0), &scanT)) {
        if (scanT.idpl != idplNone) {
            scan.idpl = scanT.idpl;
        }
        if (scanT.ifl != iflNone) {
            scan.ifl = scanT.ifl;
        }
        if (scanT.iwp != iwpNone) {
            scan.iwp = scanT.iwp;
        }
        if (scanT.ith != ithNone) {
            scan.ith = scanT.ith;
        }
        scan.grobjFull = static_cast<GrobjClass>(scan.grobjFull | scanT.grobjFull);
    }
    if (pscan) {
        *pscan = scan;
    }
    if (scan.grobj != grobjNone) {
        return 1;
    }
    return 0;
}

std::int16_t PtToScan(APPSESSION &app, std::int16_t d) {
    if (app.state.iScanZoom == zoom100) {
        return d;
    }
    switch (app.state.iScanZoom) {
    default:
        break;
    case zoom400:
        d *= 4;
        break;
    case zoom200:
        d *= 2;
        break;
    case zoom50:
        d >>= 1;
        break;
    case zoom25:
        d >>= 2;
        break;
    case zoom38:
        d = (d * 3) >> 3;
        break;
    case zoom75:
        d = (d * 3) >> 2;
        break;
    case zoom125:
        d = (d * 5) >> 2;
        break;
    case zoom150:
        d = (d * 3) >> 1;
    }
    return d;
}

std::int16_t ScanToPt(APPSESSION &app, std::int16_t d) {
    if (app.state.iScanZoom == zoom100) {
        return d;
    }
    switch (app.state.iScanZoom) {
    default:
        break;
    case zoom400:
        d >>= 2;
        break;
    case zoom200:
        d >>= 1;
        break;
    case zoom50:
        d *= 2;
        break;
    case zoom25:
        d *= 4;
        break;
    case zoom38:
        d = static_cast<std::int16_t>((d * 8)) / 3;
        break;
    case zoom75:
        d = static_cast<std::int16_t>((d * 4)) / 3;
        break;
    case zoom125:
        d = static_cast<std::int16_t>((d * 4)) / 5;
        break;
    case zoom150:
        d = static_cast<std::int16_t>((d * 2)) / 3;
    }
    return d;
}

void ChangeScanSel(APPSESSION &app, SCAN *pscan, std::int16_t fValidScan) {
    std::int16_t fChgWp;
    SCAN    scanOld;

    if (!fValidScan) {
        FFindNearestObject(app, pscan->pt, pscan->grobj, pscan);
    }
    if (std::memcmp(pscan, &app.state.sel.scan, sizeof(SCAN)) != 0) {
        fChgWp = pscan->iwp != iwpNone && pscan->iwp != app.state.sel.iwpAct;
        scanOld = app.state.sel.scan;
        ShowScanSel(app, -1);
        app.state.sel.scan = *pscan;
        if ((app.state.sel.scan.grobjFull & grobjPlanet) && fValidScan != 2) {
            app.state.sel.scan.grobj = grobjPlanet;
        }
        if (fChgWp) {
            app.state.sel.iwpAct = pscan->iwp;
        }
        ShowScanSelChange(app, &scanOld, pscan, fChgWp);
    }
    return;
}

void UpdateShdefCost(APPSESSION &app, SHDEF *lpshdef) {
    std::int32_t  dpT;
    std::uint32_t wt;
    std::int16_t  k;
    std::int16_t  c;
    std::uint16_t rgCosts[4];
    std::int16_t  fWeakArmor;
    HUL     *lphul;
    std::uint32_t resCost;
    std::uint32_t rgMin[3];
    PART     part;

    if (lpshdef->det == detAll) {
        fWeakArmor = RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceRegeneratingShields) != 0;
    } else {
        fWeakArmor = 0;
    }
    lphul = &PartQueryIO().pfnHull(lpshdef->hul.ihuldef)->hul;
    part.hs.grhst = hstNone;
    part.SetPhul(lphul);
    PartQueryIO().pfnTrueCost(app, app.state.idPlayer, &part, rgCosts);
    for (c = 0; c < 3; c++) {
        rgMin[c] = static_cast<std::uint32_t>(rgCosts[c]);
    }
    resCost = static_cast<std::uint32_t>(rgCosts[3]);
    wt = static_cast<std::uint32_t>(lphul->wtEmpty);
    lpshdef->hul.dp = lphul->dp;
    lphul = &lpshdef->hul;
    for (c = 0; c < lphul->chs; c++) {
        if (lphul->rghs[c].cItem > 0) {
            part.hs = lphul->rghs[c];
            PartQueryIO().pfnLookup(app, &part);
            PartQueryIO().pfnTrueCost(app, app.state.idPlayer, &part, rgCosts);
            for (k = 0; k < 3; k++) {
                rgMin[k] += static_cast<std::uint32_t>((lphul->rghs[c].cItem * rgCosts[k]));
            }
            resCost += static_cast<std::uint32_t>((lphul->rghs[c].cItem * rgCosts[3]));
            wt += static_cast<std::uint32_t>((lphul->rghs[c].cItem * part.pcom().cMass()));
            switch (lphul->rghs[c].grhst) {
            default:
                break;
            case hstArmor:
                /* 32-bit: the original's int16_t overflowed for large slots
                   (24 Superlatanium), and RS then halved the negative value. */
                dpT = static_cast<std::int32_t>(lphul->rghs[c].cItem) * part.parmor()->dp;
                if (fWeakArmor) {
                    dpT >>= 1;
                }
                lphul->dp += dpT;
                break;
            case hstShield:
                if (lphul->rghs[c].iItem != ishieldCrobySharmor && lphul->rghs[c].iItem != ishieldLangstonShell)
                    break;
                lphul->dp += lphul->rghs[c].cItem * 65;
                break;
            case hstSpecialM:
                if (lphul->rghs[c].iItem == ispecialMMultiCargoPod) {
                    lphul->dp += lphul->rghs[c].cItem * 50;
                }
            }
        }
    }
    for (c = 0; c < 3; c++) {
        lphul->rgwtOreCost[c] = LOWORD(rgMin[c]);
    }
    lphul->resCost = LOWORD(resCost);
    lphul->wtEmpty = LOWORD(wt);
    lpshdef->lPower = -1;
    return;
}

std::uint16_t WPackLong(std::int32_t l) {
    std::uint16_t exp;

    exp = 0;
    while (l & 0xffffe000) {
        l = static_cast<std::uint32_t>((static_cast<std::uint32_t>(l) >> 2));
        exp++;
    }
    return exp << 0xd | LOWORD(l);
}

std::int16_t GetPlanetScannerRange(APPSESSION &app, PLANET *lppl, std::int16_t *pDeep) {
    std::int16_t iPlrSav;
    std::int16_t dRange;
    PART    part;

    iPlrSav = app.state.idPlayer;
    app.state.idPlayer = lppl->iPlayer;
    if (pDeep) {
        *pDeep = 0;
    }
    if (RaceQueryIO().pfnStat(&app.state.rgplr[lppl->iPlayer], rsMajorAdv) == raMacintosh) {
        dRange = LOWORD(static_cast<std::int32_t>(std::sqrt(static_cast<double>(static_cast<std::uint32_t>((lppl->rgwtMin[3] * 10))))));
        if (RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceNoAdvScanner) != 0) {
            dRange = LOWORD(static_cast<std::int32_t>((dRange * 1412)) / 1000);
            if (pDeep) {
                *pDeep = 0;
            }
        } else if (pDeep && lppl->fStarbase && static_cast<std::int16_t>(app.state.rglpshdefSB[lppl->iPlayer][lppl->isb].hul.ihuldef) >= ihuldefUltraStation) {
            *pDeep = dRange / 2;
        }
        goto LFinishUp;
    } else {
        if (lppl->iScanner == 31) {
            app.state.idPlayer = iPlrSav;
            return 0;
        }
        ProductionIO().pfnScanner(app, &part);
        if (pDeep && part.pplanetary()->grAbility < 0) {
            *pDeep = -part.pplanetary()->grAbility >> 1;
        }
        dRange = std::abs(part.pplanetary()->grAbility);
        if (RaceQueryIO().pfnGrbit(&app.state.rgplr[app.state.idPlayer], ibitRaceNoAdvScanner) != 0) {
            dRange *= 2;
        }
    }
LFinishUp:
    app.state.idPlayer = iPlrSav;
    return dRange;
}

std::int16_t GetCachedFleetScannerRange(APPSESSION &app, FLEET *lpfl, std::int16_t *pdPlanRange, std::int16_t *ppctDetect, std::int16_t *piSteal) {
    std::int16_t dT;
    std::int16_t dPlanRange;
    std::int16_t i;
    std::int16_t iPlr;
    std::int16_t dRange;
    std::int16_t iSteal;
    std::int16_t pctDetect;

    iPlr = lpfl->iPlayer;
    dRange = -1;
    dPlanRange = 0;
    iSteal = 0;
    pctDetect = 100;
    if (!app.state.gd.fGeneratingTurn) {
        return GetFleetScannerRange(app, lpfl, pdPlanRange, ppctDetect, piSteal);
    }
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            dT = app.state.rglpshdef[iPlr][i].dScanRange;
            if (dT != 2047 && dT > dRange) {
                dRange = dT;
            }
            if (static_cast<std::int16_t>(app.state.rglpshdef[iPlr][i].dScanRange2) > dPlanRange) {
                dPlanRange = app.state.rglpshdef[iPlr][i].dScanRange2;
            }
            if (app.state.rglpshdef[iPlr][i].pctDetect < pctDetect) {
                pctDetect = app.state.rglpshdef[iPlr][i].pctDetect;
            }
            iSteal |= app.state.rglpshdef[iPlr][i].iSteal;
        }
    }
    if (pdPlanRange) {
        *pdPlanRange = dPlanRange;
    }
    if (ppctDetect) {
        *ppctDetect = pctDetect;
    }
    if (piSteal) {
        *piSteal = iSteal;
    }
    return dRange;
}

std::int16_t GetFleetScannerRange(APPSESSION &app, FLEET *lpfl, std::int16_t *pdPlanRange, std::int16_t *ppctDetect, std::int16_t *piSteal) {
    std::int16_t iplr;
    std::int16_t dPlanRange;
    std::int16_t i;
    std::int16_t dRange;
    std::int16_t iSteal;
    std::int16_t dPlanRangeBest;
    std::int16_t dRangeBest;
    std::int16_t pctDetect;

    iplr = lpfl->iPlayer;
    dRange = -1;
    dPlanRange = 0;
    iSteal = 0;
    if (ppctDetect) {
        *ppctDetect = 100;
    }
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            dRangeBest = GetShdefScannerRange(app, app.state.rglpshdef[iplr] + i, iplr, &dPlanRangeBest, &pctDetect, &iSteal);
            if (ppctDetect && pctDetect < *ppctDetect) {
                *ppctDetect = pctDetect;
            }
            if (piSteal) {
                *piSteal |= iSteal;
            }
            if (dRangeBest > dRange) {
                dRange = dRangeBest;
            }
            if (dPlanRange < dPlanRangeBest) {
                dPlanRange = dPlanRangeBest;
            }
        }
    }
    if (pdPlanRange) {
        *pdPlanRange = dPlanRange;
    }
    return dRange;
}

std::int16_t GetShdefScannerRange(APPSESSION &app, SHDEF *lpshdef, std::int16_t iplr, std::int16_t *pdPlanRange, std::int16_t *ppctDetect, std::int16_t *piSteal) {
    std::int16_t  chs;
    HS      *lphs;
    std::int16_t  dRangeT2;
    double   lBIR4;
    std::int16_t  dRangeT;
    std::int16_t  fHasScanner;
    iscanner iScanner;
    std::int16_t  fBuiltIn;
    std::int16_t  cDetectors;
    double   lPlanRange4;
    std::int16_t  dRange;
    double   lT;
    std::int16_t  iSteal;
    std::int16_t  j;
    double   lBIPR4;
    double   lRange4;

    lRange4 = static_cast<double>(0);
    lPlanRange4 = static_cast<double>(0);
    fHasScanner = 0;
    iSteal = 0;
    cDetectors = 0;
    fBuiltIn = iplr != iplrNone && RaceQueryIO().pfnStat(&app.state.rgplr[iplr], rsMajorAdv) == raNone;
    lBIR4 = -1.0;
    lBIPR4 = -1.0;
    if (ppctDetect) {
        *ppctDetect = 100;
    }
    if (fBuiltIn) {
        switch (lpshdef->hul.ihuldef) {
        default:
            break;
        case ihuldefScout:
        case ihuldefDestroyer:
        case ihuldefFrigate:
            if (static_cast<double>(lBIR4) < static_cast<double>(0)) {
                if (app.state.game.fTutorial) {
                    lBIR4 = 2.56e+06;
                    lBIPR4 = 160000.0;
                } else {
                    lBIPR4 = static_cast<double>(static_cast<std::int16_t>((app.state.rgplr[iplr].rgTech[4] * 10)));
                    lBIR4 = static_cast<double>((static_cast<double>(lBIPR4) * 2));
                    lBIPR4 = static_cast<double>((static_cast<double>(lBIPR4) * lBIPR4));
                    lBIPR4 = static_cast<double>((static_cast<double>(lBIPR4) * lBIPR4));
                    lBIR4 = static_cast<double>((static_cast<double>(lBIR4) * lBIR4));
                    lBIR4 = static_cast<double>((static_cast<double>(lBIR4) * lBIR4));
                }
            }
            lRange4 = lBIR4;
            lPlanRange4 = lBIPR4;
        }
    }
    lphs = lpshdef->hul.rghs;
    chs = lpshdef->hul.chs;
    for (j = 0; j < chs; j++, lphs++) {
        if (lphs->cItem == 0)
            continue;
        if (lphs->grhst == hstScanner) {
            fHasScanner = 1;
            iScanner = static_cast<iscanner>(lphs->iItem);
            lT = dRangeT = LpscannerFromId(lphs->iItem)->dRange;
            lT = static_cast<double>((static_cast<double>(lT) * lT));
            lT = static_cast<double>((static_cast<double>(lT) * lT));
            lT = static_cast<double>((static_cast<double>(lT) * static_cast<std::uint32_t>(lphs->cItem)));
            lRange4 = static_cast<double>((static_cast<double>(lRange4) + lT));
            dRangeT = LpscannerFromId(iScanner)->grfAbilities;
            if (iScanner == iscannerChameleonScanner) {
                dRangeT = 45;
                goto LPlanScan;
            } else if (iScanner == iscannerPickPocketScanner) {
                dRangeT = 0;
                iSteal |= 1;
                goto LPlanScan;
            } else if (iScanner == iscannerRobberBaronScanner) {
                dRangeT = 120;
                iSteal |= 3;
                goto LPlanScan;
            }
            if (dRangeT <= 0)
                continue;
            dRangeT = dRangeT == 1 ? 50 : dRangeT == 2 ? 100 : 200;
        LPlanScan:
            lT = static_cast<double>(dRangeT);
            lT = static_cast<double>((static_cast<double>(lT) * lT));
            lT = static_cast<double>((static_cast<double>(lT) * lT));
            lT = static_cast<double>((static_cast<double>(lT) * static_cast<std::uint32_t>(lphs->cItem)));
            lPlanRange4 = static_cast<double>((static_cast<double>(lPlanRange4) + lT));
        } else if (lphs->grhst == hstArmor && lphs->iItem == iarmorMegaPolyShell) {
            dRangeT = 80;
            dRangeT2 = 40;
        LOddBallScanners:
            lT = static_cast<double>(dRangeT);
            lT = static_cast<double>((static_cast<double>(lT) * lT));
            lT = static_cast<double>((static_cast<double>(lT) * lT));
            lT = static_cast<double>((static_cast<double>(lT) * static_cast<std::uint32_t>(lphs->cItem)));
            lRange4 = static_cast<double>((static_cast<double>(lRange4) + lT));
            dRangeT = dRangeT2;
            goto LPlanScan;
        } else if (lphs->grhst == hstBeam && lphs->iItem == ibeamMultiContainedMunition) {
            dRangeT = 150;
            dRangeT2 = 75;
            goto LOddBallScanners;
        } else if (lphs->grhst == hstShield && lphs->iItem == ishieldLangstonShell) {
            dRangeT = 50;
            dRangeT2 = 25;
            goto LOddBallScanners;
        } else if (ppctDetect && lphs->grhst == hstSpecialE && lphs->iItem == ispecialETachyonDetector) {
            cDetectors += lphs->cItem;
        }
    }
    if (static_cast<double>(lRange4) > static_cast<double>(0) || fHasScanner) {
        dRange = LOWORD(static_cast<std::int32_t>(std::sqrt(static_cast<double>(std::sqrt(lRange4)))));
        if (iplr != iplrNone && RaceQueryIO().pfnGrbit(&app.state.rgplr[iplr], ibitRaceNoAdvScanner) != 0) {
            dRange *= 2;
        }
    } else {
        dRange = -1;
    }
    if (pdPlanRange) {
        *pdPlanRange = LOWORD(static_cast<std::int32_t>(std::sqrt(static_cast<double>(std::sqrt(lPlanRange4)))));
    }
    if (piSteal) {
        *piSteal = iSteal;
    }
    if (ppctDetect) {
        if (cDetectors >= 18) {
            cDetectors = 17;
        }
        *ppctDetect = vrgbTachyon[cDetectors];
    }
    return dRange;
}

std::int32_t LCalcFuelGainFromRamScoops(APPSESSION &app, FLEET *lpfl, std::int16_t iWarp, std::int32_t dTravel) {
    std::int16_t  i;
    std::int16_t *rgiFuel;
    SHDEF   *lpshdef;
    std::int32_t  pct10;
    std::int32_t  pctShip10;

    pct10 = 0;
    if (iWarp > 10) {
        return 0;
    }
    i = 0;
    lpshdef = app.state.rglpshdef[lpfl->iPlayer];
    while (i < 16) {
        if (lpfl->rgcsh[i] != 0) {
            rgiFuel = LpengineFromId(lpshdef->hul.rghs[0].iItem)->rgcFuelUsed;
            pctShip10 = 0;
            if (iWarp <= 9) {
                if (rgiFuel[iWarp] == 0) {
                    pctShip10 += lpshdef->hul.rghs[0].cItem;
                    if (rgiFuel[iWarp + 1] == 0) {
                        pctShip10 += static_cast<std::uint32_t>((lpshdef->hul.rghs[0].cItem * 2));
                        if (iWarp < 9 && rgiFuel[iWarp + 2] == 0) {
                            pctShip10 += static_cast<std::uint32_t>((lpshdef->hul.rghs[0].cItem * 3));
                            if (iWarp < 8 && rgiFuel[iWarp + 3] == 0) {
                                pctShip10 += static_cast<std::uint32_t>((lpshdef->hul.rghs[0].cItem * 4));
                            }
                        }
                    }
                }
                pct10 += static_cast<std::uint32_t>((pctShip10 * lpfl->rgcsh[i]));
            }
        }
        i++;
        lpshdef++;
    }
    pct10 = static_cast<std::uint32_t>((pct10 * dTravel));
    return pct10;
}

std::int32_t CalcPlayerScore(APPSESSION &app, std::int16_t iPlr, SCORE *pscore) {
    std::int32_t rgcsh[3];
    std::int32_t lTemp;
    SCORE   score;
    PLANET *lpplMac;
    PLANET *lppl;
    std::int16_t i;
    std::int16_t ifl;
    FLEET  *lpfl;
    std::int16_t iTech;
    std::int32_t lPower;
    std::int16_t rgType[16];
    PLANET  pl;

    std::memset(&score, 0, sizeof(SCORE));
    lppl = app.state.lpPlanets;
    lpplMac = app.state.lpPlanets + app.state.cPlanet;
    for (; lppl < lpplMac; lppl++) {
        if (lppl->iPlayer == iPlr) {
            score.cPlanet++;
            /* Score a planet the player owned through production at its
               population then, not after waypoint 1 cargo moves. */
            pl = *lppl;
            if (app.state.vrgiplrPopScore && app.state.vrgiplrPopScore[lppl->id] == iPlr) {
                pl.rgwtMin[3] = app.state.vrgwtPopScore[lppl->id];
            }
            lTemp = static_cast<std::int32_t>(((pl.rgwtMin[3] + 999) / 1000));
            if (lTemp > 6) {
                lTemp = 6;
            }
            score.lScore += lTemp;
            if (lppl->fStarbase && PartQueryIO().pfnHull(app.state.rglpshdefSB[iPlr][lppl->isb].hul.ihuldef)->hul.wtCargoMax != 0) {
                score.cStarbase++;
            }
            score.cResources += ResearchIO().pfnResources(app, &pl, iPlr);
        }
    }
    score.lScore += static_cast<std::int32_t>((score.cResources / 30));
    score.lScore += static_cast<std::int16_t>((3 * score.cStarbase));
    if (!app.state.rgplr[iPlr].fDead) {
        for (i = 0; i < 6; i++) {
            iTech = app.state.rgplr[iPlr].rgTech[i];
            score.cTechLevels += app.state.rgplr[iPlr].rgTech[i];
            if (iTech < 4) {
                score.lScore += iTech;
            } else if (iTech < 7) {
                score.lScore += static_cast<std::int16_t>((iTech * 2 - 3));
            } else if (iTech < 10) {
                score.lScore += static_cast<std::int16_t>((3 * iTech - 9));
            } else {
                score.lScore += static_cast<std::int16_t>((iTech * 4 - 18));
            }
        }
    }
    for (i = 0; i < 16; i++) {
        if (app.state.rglpshdef[iPlr][i].fFree) {
            rgType[i] = -1;
        } else {
            lPower = LComputePower(app, app.state.rglpshdef[iPlr] + i);
            if (lPower <= 0) {
                rgType[i] = 0;
            } else if (lPower < 2000) {
                rgType[i] = 1;
            } else {
                rgType[i] = 2;
            }
        }
    }
    for (i = 0; i < 3; i++) {
        rgcsh[i] = 0;
    }
    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->iPlayer == iPlr && !lpfl->fDead) {
            for (i = 0; i < 16; i++) {
                if (lpfl->rgcsh[i] > 0 && rgType[i] != -1) {
                    rgcsh[rgType[i]] = rgcsh[rgType[i]] + lpfl->rgcsh[i];
                }
            }
        }
    }
    score.lScore += static_cast<std::int32_t>(((rgcsh[0] < score.cPlanet ? rgcsh[0] : score.cPlanet) / 2)) + static_cast<std::int32_t>(((rgcsh[1] < score.cPlanet ? rgcsh[1] : score.cPlanet) * 2));
    if (rgcsh[2] > 0) {
        score.lScore += static_cast<std::int32_t>((static_cast<std::int32_t>((static_cast<std::int32_t>((rgcsh[2] * 8)) * score.cPlanet)) / (score.cPlanet + rgcsh[2])));
    }
    for (i = 0; i < 3; i++) {
        score.rgcsh[i] = WPackLong(rgcsh[i]);
    }
    if (pscore) {
        *pscore = score;
    }
    return score.lScore;
}

void GetTrueHullCost(std::int16_t iPlayer, HUL *lphul, std::uint16_t *rgCost) {
    std::int16_t i;

    for (i = 0; i < 3; i++) {
        rgCost[i] = lphul->rgwtOreCost[i];
    }
    rgCost[3] = lphul->resCost;
    return;
}

void DecorateHullName(APPSESSION &app, std::int16_t iplr, std::int16_t ish, char *psz) {
    std::int16_t i;
    std::int16_t c;
    SHDEF  *lpshdef;
    std::int16_t iVal;

    lpshdef = app.state.rglpshdef[iplr] + ish;
    if (lpshdef->fFree) {
        *psz = 0;
    } else {
        std::strcpy(psz, lpshdef->hul.szClass);
        if (iplr != app.state.idPlayer) {
            c = 0;
            iVal = 1;
            for (i = 0; i < 16; i++) {
                if (i != ish && !app.state.rglpshdef[iplr][i].fFree && std::strcmp(psz, app.state.rglpshdef[iplr][i].hul.szClass) == 0) {
                    c++;
                    if (i < ish) {
                        iVal++;
                    }
                }
            }
            if (c != 0) {
                c = std::strlen(psz);
                psz[c] = ' ';
                psz[c + 1] = '(';
                IntToRoman(iVal, psz + (c + 2));
                std::strcat(psz, ")");
            }
        }
    }
    return;
}

std::int16_t FMatchTarget(APPSESSION &app, FLEET *lpflTarget, MdTarget mdTarget, std::int16_t fExact) {
    std::int16_t imd;
    std::int16_t ish;

    switch (mdTarget) {
    default:
        if (!fExact)
            break;
        return 0;
    case mdTargetArmedShips:
    case mdTargetUnarmedShips:
        for (ish = 0; ish < 16; ish++) {
            if (lpflTarget->rgcsh[ish] != 0) {
                imd = PartQueryIO().pfnHull(app.state.rglpshdef[lpflTarget->iPlayer][ish].hul.ihuldef)->imdCategory;
                if (imd >= 2 && imd <= 4)
                    break;
            }
        }
        if (mdTarget == mdTargetArmedShips) {
            if (ish != 16)
                break;
            return 0;
        }
        if (ish == 16)
            break;
        return 0;
    case mdTargetBombersFreighters:
        for (ish = 0; ish < 16; ish++) {
            if (lpflTarget->rgcsh[ish] != 0) {
                imd = PartQueryIO().pfnHull(app.state.rglpshdef[lpflTarget->iPlayer][ish].hul.ihuldef)->imdCategory;
                if (imd == 1 || imd == 5)
                    break;
            }
        }
        if (ish != 16)
            break;
        return 0;
    case mdTargetFuelTransports:
        for (ish = 0; ish < 16; ish++) {
            if (lpflTarget->rgcsh[ish] != 0) {
                imd = PartQueryIO().pfnHull(app.state.rglpshdef[lpflTarget->iPlayer][ish].hul.ihuldef)->imdCategory;
                if (imd == 7)
                    break;
            }
        }
        if (ish != 16)
            break;
        return 0;
    case mdTargetFreighters:
        for (ish = 0; ish < 16; ish++) {
            if (lpflTarget->rgcsh[ish] != 0) {
                imd = PartQueryIO().pfnHull(app.state.rglpshdef[lpflTarget->iPlayer][ish].hul.ihuldef)->imdCategory;
                if (imd == 1)
                    break;
            }
        }
        if (ish == 16) {
            return 0;
        }
    }
    return 1;
}

void ValidateWaypoints(APPSESSION &app) {
    MdTarget mdTarget;
    FLEET   *lpflTarget;
    std::int16_t  ifl2;
    std::int32_t  wt;
    FLEET   *lpflMatch;
    std::int32_t  wtMatch;
    ORDER   *lpord;
    std::int16_t  ifl;
    THING   *lpth;
    FLEET   *lpfl;
    std::int16_t  cFound;
    std::int16_t  iord;
    FLEET   *lpfl2;
    std::int16_t  iplrHi;

    for (ifl = 0; ifl < app.state.cFleet; ifl++) {
        lpfl = app.state.rglpfl[ifl];
        if (!app.state.rglpfl[ifl])
            break;
        if (lpfl->lpplord->rgord()[0].grobj != grobjPlanet && lpfl->lpplord->rgord()[0].grTask != grTaskXfer && lpfl->lpplord->rgord()[0].grTask != grTaskMerge &&
            !lpfl->fMark) {
            if (lpfl->idPlanet == idPlanetDeepSpace) {
                lpfl->lpplord->rgord()[0].grobj = grobjOther;
                lpfl->lpplord->rgord()[0].id = 0;
            } else {
                lpfl->lpplord->rgord()[0].grobj = grobjPlanet;
                lpfl->lpplord->rgord()[0].id = lpfl->idPlanet;
            }
        }
        if (!lpfl->fDead) {
            if (lpfl->pt.x < 1000) {
                lpfl->pt.x = 1000;
                lpfl->lpplord->rgord()[0].pt.x = 1000;
            } else if (lpfl->pt.x > app.state.dGal + 1000) {
                lpfl->pt.x = app.state.dGal + 1000;
                lpfl->lpplord->rgord()[0].pt.x = app.state.dGal + 1000;
            }
            if (lpfl->pt.y < 1000) {
                lpfl->pt.y = 1000;
                lpfl->lpplord->rgord()[0].pt.y = 1000;
            } else if (lpfl->pt.y > app.state.dGal + 1000) {
                lpfl->pt.y = app.state.dGal + 1000;
                lpfl->lpplord->rgord()[0].pt.y = app.state.dGal + 1000;
            }
            if (lpfl->cord > 1 || lpfl->fMark) {
                iord = lpfl->fMark == 0;
                lpord = &lpfl->lpplord->rgord()[iord];
                while (iord < lpfl->cord) {
                    if (lpord->grobj == grobjThing) {
                        lpth = LpthFromId(app, lpord->id);
                        if (!lpth || (lpth->ith() == ithWormhole && !(1 << lpfl->iPlayer & lpth->thw().grbitPlr()) &&
                                      (lpth->pt().x() != lpord->pt.x || lpth->pt().y() != lpord->pt.y))) {
                            if (lpth) {
                                TurnIO().pfnMessage(app, lpfl->iPlayer, idmWormholeHeadingHasVanishedOrdersHaveChanged, static_cast<MsgGoto>(0x8000 | lpfl->id), lpfl->id, 0, 0, 0, 0, 0, 0);
                            }
                            lpord->grobj = grobjOther;
                            lpord->id = iord;
                        } else {
                            lpord->pt = lpth->pt();
                        }
                    } else if (lpord->grobj == grobjFleet && !lpord->fNoAutoTrack) {
                        lpflTarget = LpflFromId(app, lpord->id);
                        if (!lpflTarget || lpflTarget->pt.x != lpord->pt.x || lpflTarget->pt.y != lpord->pt.y ||
                            (lpflTarget->fCompChg() && lpflTarget->iPlayer != lpfl->iPlayer)) {
                            lpflTarget = nullptr;
                            iplrHi = lpord->id & 0xfe00;
                            cFound = 0;
                            mdTarget = static_cast<MdTarget>(app.state.rglpbtlplan[lpfl->iPlayer][lpfl->iplan].mdTarget1);
                            wtMatch = 0;
                            lpflMatch = nullptr;
                            /* As in the jrc4 binary (0008:497a): one pass, taking the
                               heaviest matching fleet even if another pursuer already
                               has it. jrc3 first looked only at untargeted fleets. */
                            for (ifl2 = 0; ifl2 < app.state.cFleet; ifl2++) {
                                lpfl2 = app.state.rglpfl[ifl2];
                                if (!app.state.rglpfl[ifl2])
                                    break;
                                if (lpfl2->pt.x == lpord->pt.x && lpfl2->pt.y == lpord->pt.y && iplrHi == (lpfl2->id & 0xfe00)) {
                                    if (FMatchTarget(app, lpfl2, mdTarget, 1)) {
                                        wt = WtFromLpfl(app, lpfl2);
                                        if (wt > wtMatch || (wt == wtMatch && RandomIO().pfnRandom(app, 2) == 0)) {
                                            wtMatch = wt;
                                            lpflMatch = lpfl2;
                                        }
                                    }
                                    cFound++;
                                    if (RandomIO().pfnRandom(app, cFound) == 0 && (cFound == 1 || !lpfl2->fTargeted() || RandomIO().pfnRandom(app, 2) != 0)) {
                                        lpflTarget = lpfl2;
                                    }
                                }
                            }
                            if (lpflMatch) {
                                lpflTarget = lpflMatch;
                            }
                            if (lpflTarget) {
                                lpord->id = lpflTarget->id;
                                lpord->pt = lpflTarget->pt;
                                lpflTarget->SetFTargeted(1);
                            }
                        } else if (lpflTarget) {
                            lpflTarget->SetFTargeted(1);
                        }
                    }
                    iord++;
                    lpord++;
                }
            }
        }
    }
    return;
}

std::int32_t ChgPopFromPlanet(APPSESSION &app, PLANET *lppl, std::int16_t fUpdate) {
    std::int32_t lMaxPop;
    std::int16_t fPopDied;
    std::int32_t lPopIncDelta;
    std::int16_t DeltaCur;
    std::int32_t pctGrow100;
    std::int16_t pctDesire;
    std::int32_t lPopInc100;
    std::int32_t lPopInc;
    std::int32_t lPopOld;
    std::int32_t pctRetard;
    std::int32_t pctFull;

    fPopDied = 0;
    if (lppl->iPlayer == iplrNone || lppl->rgwtMin[3] == 0) {
        return 0;
    }
    pctDesire = PlanetQueryIO().pfnDesirability(app, lppl, lppl->iPlayer);
    lPopOld = lppl->rgwtMin[3];
    fPopDied = 1;
    if (pctDesire < 0) {
        lPopInc100 = 1 <= static_cast<std::int32_t>((lPopOld * static_cast<std::int16_t>(-pctDesire))) / 10 ? static_cast<std::int32_t>((lPopOld * static_cast<std::int16_t>(-pctDesire))) / 10 : 1;
        lPopInc = static_cast<std::int32_t>((lPopInc100 / 100));
        lPopIncDelta = static_cast<std::int32_t>((lPopInc100 % 100));
        if (lPopInc == 0 && lPopIncDelta == 0) {
            lPopIncDelta = 1;
        }
        DeltaCur = lppl->iDeltaPop - LOWORD(lPopIncDelta);
        if (DeltaCur < 0) {
            lPopInc++;
            DeltaCur += 100;
        }
        lPopInc = -lPopInc;
        goto LUpdateAndExit;
    }
    lMaxPop = CalcPlanetMaxPop(app, lppl->id, lppl->iPlayer);
    pctGrow100 = static_cast<std::int16_t>((PctTrueMaxGrowth(app, lppl->iPlayer) * pctDesire));
    if (lPopOld > static_cast<std::int32_t>((lMaxPop / 4))) {
        pctFull = static_cast<std::int32_t>((static_cast<std::int32_t>((lPopOld * 1000)) / lMaxPop));
        if (lPopOld >= lMaxPop) {
            if (lPopOld < lMaxPop + 10) {
                return 0;
            }
            pctRetard = -300 <= 99 - static_cast<std::int32_t>((pctFull / 10)) ? 99 - static_cast<std::int32_t>((pctFull / 10)) : -300;
            pctGrow100 = static_cast<std::int32_t>((pctRetard * 4));
        } else {
            pctRetard = 1000 - pctFull;
            pctRetard = static_cast<std::uint32_t>((pctRetard * pctRetard));
            if (pctGrow100 < 1000) {
                pctGrow100 = static_cast<std::int32_t>((pctGrow100 * pctRetard)) / 562500;
            } else {
                pctGrow100 = static_cast<std::uint32_t>((static_cast<std::int32_t>((static_cast<std::int32_t>((pctGrow100 / 10)) * pctRetard)) / 562500 * 10));
            }
        }
    }
    lPopInc100 = static_cast<std::uint32_t>((lPopOld * static_cast<std::int32_t>((pctGrow100 / 100))));
    if (lPopInc100 < 10000000) {
        lPopInc100 = static_cast<std::int32_t>((lPopOld * pctGrow100)) / 100;
    }
    lPopInc = static_cast<std::int32_t>((lPopInc100 / 100));
    lPopIncDelta = static_cast<std::int32_t>((lPopInc100 % 100));
    if (lPopInc == 0 && lPopIncDelta == 0) {
        lPopIncDelta = 1;
    }
    DeltaCur = lppl->iDeltaPop + LOWORD(lPopIncDelta);
    if (DeltaCur >= 100) {
        lPopInc++;
        DeltaCur -= 100;
    } else if (DeltaCur < 0) {
        lPopInc--;
        DeltaCur += 100;
    }
LUpdateAndExit:
    if (fUpdate) {
        lppl->iDeltaPop = static_cast<std::uint16_t>(DeltaCur);
        lppl->rgwtMin[3] += lPopInc;
    }
    return lPopInc;
}

std::int16_t FCanFleetUseStargates(APPSESSION &app, FLEET *lpfl, POINT16 ptSrc, POINT16 ptDst) {
    std::int16_t dTravel;
    PLANET *lpplDst;
    std::int16_t pctDmg;
    std::int16_t fSrcPlanet;
    std::int16_t fUncertain;
    std::int16_t i;
    std::int16_t fDanger;
    PLANET *lpplSrc;
    std::int16_t isbsDst;
    std::int16_t fCargo;
    std::int16_t ishdef;
    std::int16_t isbsSrc;
    SCAN    scan;

    fUncertain = 0;
    if (!FFindNearestObject(app, ptDst, static_cast<GrobjClass>(grobjPlanet | mdExact), &scan)) {
        return 0;
    }
    lpplDst = LpplFromId(app, scan.idpl);
    if (!lpplDst || lpfl->iPlayer != lpplDst->iPlayer) {
        if (lpplDst && lpplDst->iPlayer == iplrNone && lpplDst->turn == app.state.game.turn) {
            return 0;
        }
        fUncertain = 1;
        goto LSrcChk;
    }
    isbsDst = IStargateFromLppl(app, lpplDst);
    if (isbsDst < 0) {
        return 0;
    }
LSrcChk:
    lpplSrc = nullptr;
    fSrcPlanet = 0;
    if (FFindNearestObject(app, ptSrc, static_cast<GrobjClass>(grobjPlanet | mdExact), &scan)) {
        fSrcPlanet = 1;
        lpplSrc = LpplFromId(app, scan.idpl);
        if (!lpplSrc || lpfl->iPlayer != lpplSrc->iPlayer)
            goto LJumpgate;
        isbsSrc = IStargateFromLppl(app, lpplSrc);
        if (isbsSrc < 0)
            goto LJumpgate;
        if (fUncertain) {
            return -1;
        }
    } else {
    LJumpgate:
        if (FFleetCanJumpgate(app, lpfl)) {
            if (fUncertain) {
                return -1;
            }
            isbsSrc = isbsDst;
        } else {
            if (!fSrcPlanet || (lpplSrc && lpplSrc->iPlayer == iplrNone && lpplSrc->turn == app.state.game.turn)) {
                return 0;
            }
            if (!lpplSrc || lpplSrc->iPlayer != lpfl->iPlayer) {
                return -1;
            }
            return 0;
        }
    }
    fCargo = 0;
    if (RaceQueryIO().pfnStat(&app.state.rgplr[lpfl->iPlayer], rsMajorAdv) != raStargate) {
        for (i = 0; i <= 3; i++) {
            if (lpfl->rgwtMin[i] > 0) {
                fCargo = 1;
            }
        }
    }
    dTravel = LOWORD(static_cast<std::int32_t>(DGetDistance(ptSrc.x, ptSrc.y, ptDst.x, ptDst.y)));
    fDanger = 0;
    for (ishdef = 0; ishdef < 16; ishdef++) {
        if (lpfl->rgcsh[ishdef] != 0) {
            switch (MdCalcStargateDamage(app, isbsSrc, isbsDst, dTravel, app.state.rglpshdef[lpfl->iPlayer][ishdef].hul.wtEmpty, &pctDmg)) {
            case -2:
            case -1:
            case 0:
                return 0;
            case 1:
                if (pctDmg > 0) {
                    fDanger = 1;
                }
            }
        }
    }
    return (!fDanger ? 0 : 2) + 1 + (!fCargo ? 0 : 4);
}

std::int16_t FFleetCanJumpgate(APPSESSION &app, FLEET *lpfl) {
    HS     *lphs;
    std::int16_t chs;
    std::int16_t i;
    std::int16_t j;

    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            lphs = app.state.rglpshdef[lpfl->iPlayer][i].hul.rghs;
            chs = app.state.rglpshdef[lpfl->iPlayer][i].hul.chs;
            j = 0;
            for (; j < chs && (lphs->cItem == 0 || lphs->grhst != hstSpecialM || lphs->iItem != ispecialMJumpGate); lphs++) {
                j++;
            }
            if (j == chs) {
                return 0;
            }
        }
    }
    return 1;
}

std::int32_t WtFromLpfl(APPSESSION &app, FLEET *lpfl) {
    std::int32_t cMass;
    std::int16_t i;

    cMass = 0;
    for (i = 0; i < 16; i++) {
        if (lpfl->rgcsh[i] > 0) {
            cMass += static_cast<std::uint32_t>((lpfl->rgcsh[i] * static_cast<std::uint32_t>(app.state.rglpshdef[lpfl->iPlayer][i].hul.wtEmpty)));
        }
    }
    for (i = 0; i <= 3; i++) {
        cMass += lpfl->rgwtMin[i];
    }
    return cMass;
}

std::int16_t FCanBuildShdef(APPSESSION &app, SHDEF *lpshdef, std::int16_t iplr) {
    std::int16_t j;
    std::int16_t iplrSav;
    PART    part;

    iplrSav = app.state.idPlayer;
    if (static_cast<std::int16_t>(lpshdef->hul.ihuldef) >= ihuldefOrbitalFort) {
        part.hs.grhst = hstSBHull;
        part.hs.iItem = lpshdef->hul.ihuldef - ihuldefOrbitalFort;
    } else {
        part.hs.grhst = hstHull;
        part.hs.iItem = lpshdef->hul.ihuldef;
    }
    app.state.idPlayer = iplr;
    if (PartQueryIO().pfnLookup(app, &part) != mdPartAvailAvailable) {
    LFail:
        app.state.idPlayer = iplrSav;
        return 0;
    }
    for (j = 0; j < lpshdef->hul.chs; j++) {
        if (lpshdef->hul.rghs[j].cItem > 0) {
            part.hs = lpshdef->hul.rghs[j];
            if (PartQueryIO().pfnLookup(app, &part) != mdPartAvailAvailable)
                goto LFail;
        }
    }
    app.state.idPlayer = iplrSav;
    return 1;
}

std::int16_t IshFindSimilarDesign(APPSESSION &app, HUL *lphul, std::int16_t iPlrDst) {
    SHDEF  *lpshdefDest;
    std::int16_t i;
    std::int16_t j;

    lpshdefDest = app.state.rglpshdef[iPlrDst];
    i = 0;
    while (i < 16) {
        if (!lpshdefDest->fFree && lpshdefDest->fGift && lpshdefDest->hul.ihuldef == lphul->ihuldef && lpshdefDest->hul.chs == lphul->chs) {
            for (j = 0; j < lphul->chs && lphul->rghs[j].cItem == lpshdefDest->hul.rghs[j].cItem &&
                        (lphul->rghs[j].cItem <= 0 ||
                         (lphul->rghs[j].iItem == lpshdefDest->hul.rghs[j].iItem && lphul->rghs[j].grhst == lpshdefDest->hul.rghs[j].grhst));
                 j++) {
            }
            if (j == lphul->chs) {
                return i;
            }
        }
        i++;
        lpshdefDest++;
    }
    return ishdefNone;
}

void ClearFile(APPSESSION &app, std::int16_t dt) {
    char *pch;
    char  szFile[256];

    std::strcpy(szFile, app.state.szBase);
    pch = std::strrchr(szFile, 46);
    if (pch) {
        pch[1] = 0;
    } else {
        std::strcat(szFile, ".");
    }
    std::strcat(szFile, app.state.mpdtsz[dt]);
    RemoveNativeFile(szFile);
    return;
}

void OutputSz(APPSESSION &app, std::int16_t dt, const char *sz) {
    char szTime[100];
    char szFile[256];
    char szDate[100];
    char szTemp[256];

    CchSprintf(szFile, "%s.%s", app.state.szBase, app.state.mpdtsz[dt]);
    if (!FFileExists(szFile)) {
        CchSprintf(szTemp, "Stars! %s\r\n\r\n", SzVersion(app));
        OutputFileString(szFile, szTemp);
    }
    GetDateTimeSz(szDate, szTime);
    CchSprintf(szTemp, "%s %s - %s\r\n", szDate, szTime, sz);
    OutputFileString(szFile, szTemp);
    return;
}

void TurnLog(APPSESSION &app, StringId ids) {
    char szTemp[256];

    if (app.state.ini.fLogging) {
        CchSprintf(szTemp, TextIO().pfnFormat(app, ids, nullptr), app.state.game.turn + 2401);
        OutputSz(app, 6, szTemp);
    }
    return;
}

char *SzVersion(APPSESSION &app) {
    /* The original formatted idsVersionD02dC ("Version %d.%02d%c") with 2, 60, 'j'. */
    CchSprintf(app.state.szWork, "Version %s", STARS_VERSION_DISPLAY);
    return app.state.szWork;
}

std::int16_t CchGetETA(APPSESSION &app, HDC hdc, FLEET *lpfl, char *sz, std::int16_t iwp, std::int16_t fSmall) {
    std::int16_t  iWarp;
    double   dbl;
    ORDER   *lpord;
    std::int16_t  i;
    std::int16_t  c;
    std::int16_t  iSpeed;
    std::int16_t  j;
    std::int16_t  cYears;
    StringId ids;

    cYears = 0;
    lpord = lpfl->lpplord->rgord();
    i = 0;
    while (i < iwp) {
        dbl = DGetDistance(lpord->pt.x, lpord->pt.y, lpord[1].pt.x, lpord[1].pt.y);
        iWarp = lpord[1].iWarp;
        if (iWarp < 11) {
            iSpeed = iWarp * iWarp;
        } else {
            j = FCanFleetUseStargates(app, lpfl, lpord->pt, lpord[1].pt);
            switch (j) {
            case -1:
                iSpeed = -3;
                break;
            case 0:
                iSpeed = 0;
                break;
            case 1:
                iSpeed = 8000;
                break;
            default:
                if (j & 2) {
                    iSpeed = -1;
                } else {
                    iSpeed = -2;
                }
            }
        }
        if (iSpeed == 0) {
            if (hdc) {
                SetHdcTextColor(hdc, 0xff);
            }
            c = TextIO().pfnCchGetString(app, idsNever, sz);
            return c;
        }
        if (iSpeed < 0) {
            if (hdc) {
                SetHdcTextColor(hdc, 32639);
            }
            if (iSpeed == -1) {
                ids = idsDanger;
            } else if (iSpeed == -2) {
                ids = idsUnload2;
            } else {
                ids = idsUncertain;
            }
            c = TextIO().pfnCchGetString(app, ids, sz);
            return c;
        }
        if (iSpeed >= static_cast<std::int16_t>(LOWORD(static_cast<std::int32_t>(dbl)))) {
            iSpeed = 1;
        } else {
            iSpeed = static_cast<std::int16_t>((LOWORD(static_cast<std::int32_t>(dbl)) + iSpeed - 1)) / iSpeed;
        }
        cYears += iSpeed;
        i++;
        lpord++;
    }
    c = CchSprintf(sz, TextIO().pfnCompressed(app, !fSmall ? idsDYear : idsDy), cYears);
    if (cYears != 1 && !fSmall) {
        sz[c++] = 's';
    }
    return c;
}
